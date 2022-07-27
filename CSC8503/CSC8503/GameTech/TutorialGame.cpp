#include "TutorialGame.h"
#include "../CSC8503Common/GameWorld.h"
#include "../../Plugins/OpenGLRendering/OGLMesh.h"
#include "../../Plugins/OpenGLRendering/OGLShader.h"
#include "../../Plugins/OpenGLRendering/OGLTexture.h"
#include "../../Common/TextureLoader.h"
#include "../CSC8503Common/PositionConstraint.h"
#include "..//CSC8503Common/StateGameObject.h"

using namespace NCL;
using namespace CSC8503;

TutorialGame::TutorialGame()	{
	world		= new GameWorld();
	renderer	= new GameTechRenderer(*world);
	physics		= new PhysicsSystem(*world);
	pdMachine	= new PushdownMachine(new MenuState());

	playerScore		= 0;
	forceMagnitude	= 10.0f;
	useGravity		= false;
	inSelectionMode = false;
	hasInitLevel	= false;

	Debug::SetRenderer(renderer);

	InitialiseAssets();
}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes, 
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void TutorialGame::InitialiseAssets() {
	auto loadFunc = [](const string& name, OGLMesh** into) {
		*into = new OGLMesh(name);
		(*into)->SetPrimitiveType(GeometryPrimitive::Triangles);
		(*into)->UploadToGPU();
	};

	loadFunc("cube.msh"		 , &cubeMesh);
	loadFunc("sphere.msh"	 , &sphereMesh);
	loadFunc("Male1.msh"	 , &charMeshA);
	loadFunc("courier.msh"	 , &charMeshB);
	loadFunc("security.msh"	 , &enemyMesh);
	loadFunc("coin.msh"		 , &bonusMesh);
	loadFunc("capsule.msh"	 , &capsuleMesh);

	basicTex	= (OGLTexture*)TextureLoader::LoadAPITexture("checkerboard.png");
	basicShader = new OGLShader("GameTechVert.glsl", "GameTechFrag.glsl");

	//InitCamera();
	//InitWorld();
}

TutorialGame::~TutorialGame()	{
	delete cubeMesh;
	delete sphereMesh;
	delete charMeshA;
	delete charMeshB;
	delete enemyMesh;
	delete bonusMesh;

	delete basicTex;
	delete basicShader;

	delete physics;
	delete renderer;
	delete world;

	delete gridMap;
	delete player;
	delete pdMachine;
}

void TutorialGame::UpdateGame(float dt) {
	
	world->UpdateWorld(dt);
	renderer->Update(dt);
	Debug::FlushRenderables(dt);
	renderer->Render();

	if (!pdMachine->Update(dt))
	{
		isQuit = true;
		return;
	}
	if (pdMachine->GetActiveState()->GetStateName() == "MenuState")
	{
		InitMenu();
	}
	if (pdMachine->GetActiveState()->GetStateName() == "Mode1State")
	{
		if (hasInitLevel == false)
		{
			hasInitLevel = true;
			InitWorld1();
		}

		if (!inSelectionMode) {
			world->GetMainCamera()->UpdateCamera(dt);
		}

		UpdateKeys();
		Mode1Playing(dt);
	}
	if (pdMachine->GetActiveState()->GetStateName() == "Mode2State")
	{
		if (hasInitLevel == false)
		{
			hasInitLevel = true;
			InitWorld2();
		}

		if (!inSelectionMode) {
			world->GetMainCamera()->UpdateCamera(dt);
		}

		UpdateKeys();
		Mode2Playing(dt);
	}
}

void TutorialGame::Mode1Playing(float dt)
{
	BonusCollect();
	GameWin();
	GameLose();

	if (useGravity) {
		Debug::Print("(G)ravity on", Vector2(5, 95));
	}
	else {
		Debug::Print("(G)ravity off", Vector2(5, 95));
	}

	SelectObject();
	MoveSelectedObject();
	physics->Update(dt);

	if (lockedObject != nullptr) {
		LockCameraToObject(lockedObject, Vector3(0, 50, 10), world);
		Debug::DrawAxisLines(lockedObject->GetTransform().GetMatrix(), 2.0f);
	}

	/*world->UpdateWorld(dt);
	renderer->Update(dt);
	Debug::FlushRenderables(dt);
	renderer->Render();*/

	for (auto i : world->GetAllObjs())
	{
		if(i->GetTag() == "Prop")
			Debug::DrawAxisLines(i->GetTransform().GetMatrix(), 2.0f);
	}

	for (auto i : obsStateObjects)
	{
		i->Update(dt);
	}
}
void TutorialGame::Mode2Playing(float dt)
{
	BonusCollect();
	GameWin();
	GameLose();

	physics->Update(dt);

	if (lockedObject != nullptr) {
		LockCameraToObject(lockedObject, Vector3(0, 50, 10), world);
		//Debug::DrawAxisLines(lockedObject->GetTransform().GetMatrix(), 2.0f);
	}
}

void TutorialGame::BonusCollect()
{
	Debug::Print("Score: " + std::to_string(playerScore), Vector2(5, 15));
	if (player->OnTriggerEnter(world->GetAllObjs(), "Coin"))
	{
		playerScore++;
		world->RemoveGameObject(player->GetTriggerObj(), true);
	}
}

void TutorialGame::GameWin()
{
	if (player->OnTriggerEnter(world->GetAllObjs(), "Finish"))
	{
		pdMachine->SetActiveState(new WinState());
	}
}

void TutorialGame::GameLose()
{
	if (player->GetTransform().GetPosition().y <= -5.0f)
	{
		pdMachine->SetActiveState(new LoseState());
	}
}

void TutorialGame::UpdateKeys() {
	//if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
	//	InitWorld1(); //We can reset the simulation at any time with F1
	//	selectionObject = nullptr;
	//	lockedObject	= nullptr;
	//}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F2)) {
		InitCamera(); //F2 will reset the camera to a specific default place
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::G)) {
		useGravity = !useGravity; //Toggle gravity!
		physics->UseGravity(useGravity);
	}
	//Running certain physics updates in a consistent order might cause some
	//bias in the calculations - the same objects might keep 'winning' the constraint
	//allowing the other one to stretch too much etc. Shuffling the order so that it
	//is random every frame can help reduce such bias.
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F9)) {
		world->ShuffleConstraints(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F10)) {
		world->ShuffleConstraints(false);
	}

	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F7)) {
		world->ShuffleObjects(true);
	}
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F8)) {
		world->ShuffleObjects(false);
	}

	if (lockedObject) {
		LockedObjectMovement();
		DebugObjectMovement();
	}
	else {
		DebugObjectMovement();
	}
}

void TutorialGame::LockCameraToObject(GameObject* obj, Vector3 lockedOffset, GameWorld* w)
{
	if (obj != nullptr)
	{
		Vector3 objPos = obj->GetTransform().GetPosition();
		Vector3 camPos = objPos + lockedOffset;

		Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0, 1, 0));

		Matrix4 modelMat = temp.Inverse();

		Quaternion q(modelMat);
		Vector3 angles = q.ToEuler(); //nearly there now!

		w->GetMainCamera()->SetPosition(camPos);
		w->GetMainCamera()->SetPitch(angles.x);
		w->GetMainCamera()->SetYaw(angles.y);
	}
}

void TutorialGame::LockedObjectMovement() {
	Matrix4 view		= world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld	= view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!

	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis.Normalise();

	Vector3 charForward  = lockedObject->GetTransform().GetOrientation() * Vector3(0, 0, 1);
	Vector3 charForward2 = lockedObject->GetTransform().GetOrientation() * Vector3(0, 0, 1);

	float force = 100.0f;

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		lockedObject->GetPhysicsObject()->AddForce(-rightAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		//Vector3 worldPos = selectionObject->GetTransform().GetPosition();
		lockedObject->GetPhysicsObject()->AddForce(rightAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		lockedObject->GetPhysicsObject()->AddForce(fwdAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		lockedObject->GetPhysicsObject()->AddForce(-fwdAxis * force);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NEXT)) {
		lockedObject->GetPhysicsObject()->AddForce(Vector3(0,-10,0));
	}
}

void TutorialGame::DebugObjectMovement() {
//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject) {
		//Twist the selected object!
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 0, 10));
		}
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 0, -10));
		}
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
		}
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}
	}

}

void TutorialGame::InitCamera() {
	world->GetMainCamera()->SetNearPlane(0.1f);
	world->GetMainCamera()->SetFarPlane(500.0f);
	world->GetMainCamera()->SetPitch(-15.0f);
	world->GetMainCamera()->SetYaw(315.0f);
	world->GetMainCamera()->SetPosition(Vector3(-60, 40, 60));
	lockedObject = nullptr;
}

void TutorialGame::InitMenu()
{
	playerScore		= 0;
	selectionObject = nullptr;
	lockedObject	= nullptr;
	hasInitLevel	= false;
	useGravity		= false;
	inSelectionMode = false;

	world->ClearAndErase();
	physics->Clear();
	physics->UseGravity(useGravity);
	obsStateObjects.clear();

	InitCamera();
}

void TutorialGame::InitWorld1() {
	
	InitGridMap("Mode 1.txt");
	InitSpherePlayer();
	//InitPendulum();
}

void TutorialGame::InitWorld2() {

	InitGridMap("Mode 2.txt");
	InitSpherePlayer();

	selectionObject = player;
	lockedObject	= selectionObject;
	useGravity		= true;
	inSelectionMode = true;

	physics->UseGravity(useGravity);
	//obsStateObject = AddStateObjectToWorld(Vector3(0, 10, 0));
}

#pragma region MyInit

void TutorialGame::InitGridMap(string filename)
{
	gridMap				= new NavigationGrid(filename);
	int gridSize		= gridMap->GetGridNodeSize();
	int mapWidth		= gridMap->GetGridWidth();
	int mapHeight		= gridMap->GetGridHeight();
	GridNode* gridNodes = gridMap->GetNodes();

	for (int h = 0; h < mapHeight; h++)
	{
		for (int w = 0; w < mapWidth; w++)
		{
			GridNode& n = gridNodes[(mapWidth * h) + w];
			if (n.type == 'x')
			{
				AddCubeToWorld(n.position + Vector3(0, 6, 0), Vector3(0.5, 0.5, 0.5) * gridSize, "Wall", "Default", 0, Vector4(0, 0, 1, 1));
			}
			if (n.type == '.')
			{
				AddCubeToWorld(n.position, Vector3(0.5, 0.1, 0.5) * gridSize, "Floor", "Default", 0);
				AddBonusToWorld(n.position + Vector3(0, 5, 0), 0.25f, "Coin", "Default");
			}
			if (n.type == 'e')
			{
				AddCubeToWorld(n.position, Vector3(0.5, 0.1, 0.5) * gridSize, "Finish", "Default", 0, Vector4(1, 0, 0, 1));
			}
			if (n.type == 'p')
			{
				AddCubeToWorld(n.position, Vector3(0.5, 0.1, 0.5) * gridSize, "Floor", "Default", 0);
				AddCubeToWorld(n.position + Vector3(0, 6, 0), Vector3(0.5, 0.5, 0.5) * gridSize, "Prop", "Prop", 0.5, Vector4(0, 0.5, 0, 1));
			}
			if (n.type == '/')
			{
				AddOBBToWorld(n.position + Vector3(0, 3, 0), Vector3(0.5, 0.1, 0.5) * gridSize, Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), 30), "Slope", "Default", 0, Vector4(1, 0, 0, 1));
			}
			if (n.type == '\\')
			{
				AddOBBToWorld(n.position + Vector3(0, 3, 0), Vector3(0.5, 0.1, 0.5) * gridSize, Quaternion::AxisAngleToQuaterion(Vector3(0, 0, 1), -30), "Slope", "Default", 0, Vector4(1, 0, 0, 1));
			}
			if (n.type == 'o')
			{
				AddStateObjectToWorld(n.position + Vector3(0, 6, 0), Vector3(0.5, 0.5, 0.5) * gridSize, "Obstacle", "Default", 0, Vector4(1, 0, 0, 1));
			}
			if (n.type == 'l')
			{
				AddCubeToWorld(n.position, Vector3(0.5, 0.1, 0.5) * gridSize, "Floor", "Default", 0);
				AddBonusToWorld(n.position + Vector3(0, 5, 0), 0.25f, "Coin", "Default");
				InitPendulum(n.position + Vector3(0, 130, 0), true);
			}
			if (n.type == 'r')
			{
				AddCubeToWorld(n.position, Vector3(0.5, 0.1, 0.5) * gridSize, "Floor", "Default", 0);
				AddBonusToWorld(n.position + Vector3(0, 5, 0), 0.25f, "Coin", "Default");
				InitPendulum(n.position + Vector3(0, 130, 0), false);
			}
			else
				continue;
		}
	}
}

void TutorialGame::InitSpherePlayer()
{
	player = AddSphereToWorld(Vector3(10, 10, 10), 3, "Player", "Player", 3, Vector4(0, 1, 1, 1));
}

void TutorialGame::InitPendulum(Vector3 s, bool isLeft)
{
	Vector3 cubeSize		= Vector3(4, 4, 4);
	float	invCubeMass		= 1;	//How heavy the middle pieces are
	int		numLinks		= 2;
	float	maxDistance		= 40;	//Constraint distance
	float	cubeDistance	= 20;	//Distance between links

	if (isLeft)
		cubeDistance = -cubeDistance;

	Vector3		startPos	= s/*Vector3(130, 130, 60)*/;
	GameObject* start		= AddCubeToWorld(startPos + Vector3(0, 0, 0), cubeSize, 0);
	GameObject* end			= AddSphereToWorld(startPos + Vector3((numLinks + 2) * cubeDistance, 0, 0), 7.5f, "Obstacle", "Default", 1, Vector4(1, 0, 0, 1));

	GameObject* previous	= start;

	for (int i = 0; i < numLinks; ++i)
	{
		GameObject* block = AddCubeToWorld(startPos + Vector3((i + 1) * cubeDistance, 0, 0), cubeSize, invCubeMass);
		PositionConstraint* constraint = new PositionConstraint(previous, block, maxDistance);
		world->AddConstraint(constraint);
		previous = block;
	}
	PositionConstraint* constraint = new PositionConstraint(previous, end, maxDistance);
	world->AddConstraint(constraint);
}

#pragma endregion


GameObject* TutorialGame::AddOBBToWorld(const Vector3& position) {
	GameObject* floor = new GameObject();

	Vector3 floorSize = Vector3(100, 2, 100);
	OBBVolume* volume = new OBBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(position);

	floor->GetTransform().SetOrientation(Quaternion::AxisAngleToQuaterion(Vector3(1, 0, 0), 45));

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor);

	return floor;
}
GameObject* TutorialGame::AddOBBToWorld(const Vector3& position, Vector3 dimensions, Quaternion rotation, string objectName, string tag, float inverseMass, Vector4 color)
{
	GameObject* cube = new GameObject(objectName);
	cube->SetTag(tag);

	OBBVolume* volume = new OBBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	cube->GetTransform().SetOrientation(rotation);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

/*

Builds a game object that uses a sphere mesh for its graphics, and a bounding sphere for its
rigid body representation. This and the cube function will let you build a lot of 'simple' 
physics worlds. You'll probably need another function for the creation of OBB cubes too.

*/
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass) {
	GameObject* sphere = new GameObject();

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->InitSphereInertia();
	//sphere->GetPhysicsObject()->SetFirction(1.0f);

	world->AddGameObject(sphere);

	return sphere;
}
GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, string objectName, string tag, float inverseMass, Vector4 color)
{
	GameObject* sphere = new GameObject(objectName);
	sphere->SetTag(tag);

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->GetRenderObject()->SetColour(color);
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->InitSphereInertia();
	//sphere->GetPhysicsObject()->SetFirction(1.0f);

	world->AddGameObject(sphere);

	return sphere;
}

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	GameObject* cube = new GameObject();

	AABBVolume* volume = new AABBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}
GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, string objectName, string tag, float inverseMass, Vector4 color)
{
	GameObject* cube = new GameObject(objectName);
	cube->SetTag(tag);

	AABBVolume* volume = new AABBVolume(dimensions);

	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->GetRenderObject()->SetColour(color);
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

GameObject* TutorialGame::AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass) {
	GameObject* capsule = new GameObject();

	CapsuleVolume* volume = new CapsuleVolume(halfHeight, radius);
	capsule->SetBoundingVolume((CollisionVolume*)volume);

	capsule->GetTransform()
		.SetScale(Vector3(radius* 2, halfHeight, radius * 2))
		.SetPosition(position);

	capsule->SetRenderObject(new RenderObject(&capsule->GetTransform(), capsuleMesh, basicTex, basicShader));
	capsule->SetPhysicsObject(new PhysicsObject(&capsule->GetTransform(), capsule->GetBoundingVolume()));

	capsule->GetPhysicsObject()->SetInverseMass(inverseMass);
	capsule->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(capsule);

	return capsule;

}

void TutorialGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddSphereToWorld(position, radius, 1.0f);
		}
	}
	//AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);

			if (rand() % 2) {
				AddCubeToWorld(position, cubeDims);
			}
			else {
				AddSphereToWorld(position, sphereRadius);
			}
		}
	}
}

void TutorialGame::InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims) {
	for (int x = 1; x < numCols+1; ++x) {
		for (int z = 1; z < numRows+1; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddCubeToWorld(position, cubeDims, 1.0f);
		}
	}
}

void TutorialGame::InitGameExamples() {
	AddPlayerToWorld(Vector3(0, 5, 0));
	AddEnemyToWorld(Vector3(5, 5, 0));
	AddBonusToWorld(Vector3(10, 5, 0));
}

GameObject* TutorialGame::AddPlayerToWorld(const Vector3& position) {
	float meshSize = 3.0f;
	float inverseMass = 0.5f;

	GameObject* character = new GameObject();

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.85f, 0.3f) * meshSize);

	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	if (rand() % 2) {
		character->SetRenderObject(new RenderObject(&character->GetTransform(), charMeshA, nullptr, basicShader));
	}
	else {
		character->SetRenderObject(new RenderObject(&character->GetTransform(), charMeshB, nullptr, basicShader));
	}
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);

	//lockedObject = character;

	return character;
}

GameObject* TutorialGame::AddEnemyToWorld(const Vector3& position) {
	float meshSize		= 3.0f;
	float inverseMass	= 0.5f;

	GameObject* character = new GameObject();

	AABBVolume* volume = new AABBVolume(Vector3(0.3f, 0.9f, 0.3f) * meshSize);
	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), enemyMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);

	return character;
}

GameObject* TutorialGame::AddBonusToWorld(const Vector3& position) {
	GameObject* apple = new GameObject();

	SphereVolume* volume = new SphereVolume(0.3f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(0.25, 0.25, 0.25))
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), bonusMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	return apple;
}
GameObject* TutorialGame::AddBonusToWorld(const Vector3& position, float radius, string objectName, string tag, float inverseMass, Vector4 color)
{
	GameObject* apple = new GameObject(objectName);

	Vector3 scale = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(0.3f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(scale)
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), bonusMesh, nullptr, basicShader));
	apple->GetRenderObject()->SetColour(color);
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));
	apple->GetPhysicsObject()->SetTrigger(true);

	apple->GetPhysicsObject()->SetInverseMass(inverseMass);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	return apple;
}


/*

Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be 
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around. 

*/
bool TutorialGame::SelectObject() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::Q)) {
		inSelectionMode = !inSelectionMode;
		if (inSelectionMode) {
			Window::GetWindow()->ShowOSPointer(true);
			Window::GetWindow()->LockMouseToWindow(false);
		}
		else {
			Window::GetWindow()->ShowOSPointer(false);
			Window::GetWindow()->LockMouseToWindow(true);
		}
	}
	if (inSelectionMode) {
		renderer->DrawString("Press Q to change to camera mode!", Vector2(5, 85));
		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {
			if (selectionObject && selectionObject->GetTag() != "Default") {	//set colour to deselected;
				selectionObject->GetRenderObject()->SetColour(originalColour);
				selectionObject = nullptr;
				lockedObject	= nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());
			RayCollision closestCollision;
			if (world->Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;
				originalColour	= selectionObject->GetRenderObject()->GetColour();
				if (selectionObject->GetTag() != "Default")
				{
					selectionObject->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
				}
				return true;
			}
			else {
				return false;
			}
		}
	}
	else {
		renderer->DrawString("Press Q to change to select mode!", Vector2(5, 85));
	}

	if (lockedObject) {
		renderer->DrawString("Press L to unlock object!", Vector2(5, 80));
	}

	else if(selectionObject && selectionObject->GetTag() != "Default"){
		renderer->DrawString("Press L to lock selected object object!", Vector2(5, 80));
	}

	if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::L)) {
		if (selectionObject && selectionObject->GetTag() != "Default") {
			if (lockedObject == selectionObject) {
				lockedObject = nullptr;
			}
			else {
				lockedObject = selectionObject;
			}
		}
	}

	return false;
}

/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque as well.
*/
void TutorialGame::MoveSelectedObject() {
	renderer->DrawString("Click Force:" + std::to_string(forceMagnitude), Vector2(10, 20));
	forceMagnitude += Window::GetMouse()->GetWheelMovement() * 50.0f;

	if (!selectionObject)
		return;

	//Push the selected obj
	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::RIGHT))
	{
		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());
		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, true))
		{
			/*if (closestCollision.node == selectionObject)
			{	
				selectionObject->GetPhysicsObject()->AddForce(ray.GetDirection() * forceMagnitude);
			}*/
			if (closestCollision.node == selectionObject)
			{
				if (selectionObject->GetTag() == "Prop")
				{
					selectionObject->GetPhysicsObject()->AddForce(ray.GetDirection() * forceMagnitude);
				}
				else
					selectionObject->GetPhysicsObject()->AddForceAtPosition(ray.GetDirection() * forceMagnitude, closestCollision.collidedAt);
			}
		}
	}
}

StateGameObject* TutorialGame::AddStateObjectToWorld(const Vector3& position) 
{
	StateGameObject* apple = new StateGameObject();

	SphereVolume* volume = new SphereVolume(0.25f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(0.25, 0.25, 0.25))
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), bonusMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);
	obsStateObjects.emplace_back(apple);

	return apple;
}
StateGameObject* TutorialGame::AddStateObjectToWorld(const Vector3& position, Vector3 dimensions, string objectName, string tag, float inverseMass, Vector4 color) 
{
	StateGameObject* obs = new StateGameObject();
	obs->SetTag(tag);

	AABBVolume* volume = new AABBVolume(dimensions);
	obs->SetBoundingVolume((CollisionVolume*)volume);
	obs->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2);

	obs->SetRenderObject(new RenderObject(&obs->GetTransform(), cubeMesh, basicTex, basicShader));
	obs->GetRenderObject()->SetColour(color);
	obs->SetPhysicsObject(new PhysicsObject(&obs->GetTransform(), obs->GetBoundingVolume()));

	obs->GetPhysicsObject()->SetInverseMass(inverseMass);
	obs->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(obs);
	obsStateObjects.emplace_back(obs);

	return obs;
}