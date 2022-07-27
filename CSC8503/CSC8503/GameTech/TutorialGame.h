#pragma once
#include "GameTechRenderer.h"
#include "../CSC8503Common/StateGameObject.h"
#include "../CSC8503Common/PhysicsSystem.h"
#include "../CSC8503Common/NavigationGrid.h"
#include "../CSC8503Common/PushdownState.h"
#include "../CSC8503Common/PushdownMachine.h"

namespace NCL {
	namespace CSC8503 {
		class TutorialGame {
		public:
			TutorialGame();
			~TutorialGame();

			virtual void UpdateGame(float dt);

			Vector4 originalColour	= Vector4(1, 1, 1, 1);
			bool	isQuit			= false;

		protected:
			void InitialiseAssets();
			void InitCamera();
			void UpdateKeys();
			
			void InitGameExamples();
			void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			void InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing);
			void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);

#pragma region MyInit

			void InitMenu();
			void InitWorld1();
			void InitWorld2();

			void InitGridMap(string filename);
			void InitSpherePlayer();
			void InitPendulum(Vector3 s, bool isLeft);

#pragma endregion

			void Mode1Playing(float dt);
			void Mode2Playing(float dt);
			void BonusCollect();
			void GameWin();
			void GameLose();

			bool SelectObject();
			void MoveSelectedObject();
			void DebugObjectMovement();
			void LockedObjectMovement();

			GameObject* AddOBBToWorld(const Vector3& position);
			GameObject* AddOBBToWorld(const Vector3& position, Vector3 dimensions, Quaternion rotation, string objectName, string tag, float inverseMass = 10.0f, Vector4 color = Vector4(1, 1, 1, 1));
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, string objectName, string tag, float inverseMass = 10.0f, Vector4 color = Vector4(1, 1, 1, 1));
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, string objectName, string tag, float inverseMass = 10.0f, Vector4 color = Vector4(1, 1, 1, 1));
			GameObject* AddCapsuleToWorld(const Vector3& position, float halfHeight, float radius, float inverseMass = 10.0f);
			GameObject* AddBonusToWorld(const Vector3& position);
			GameObject* AddBonusToWorld(const Vector3& position, float radius, string objectName, string tag, float inverseMass = 0.0f, Vector4 color = Vector4(1, 1, 0, 1));

			GameObject* AddPlayerToWorld(const Vector3& position);
			GameObject* AddEnemyToWorld(const Vector3& position);

			GameTechRenderer*	renderer;
			PhysicsSystem*		physics;
			GameWorld*			world;

			NavigationGrid*		gridMap;
			GameObject*			player;
			PushdownMachine*	pdMachine;

			vector<StateGameObject*> obsStateObjects;

			int					playerScore;

			bool				useGravity;
			bool				inSelectionMode;
			bool				hasInitLevel;
			bool				isPaused;

			float				forceMagnitude;

			GameObject* selectionObject = nullptr;

			OGLMesh*	capsuleMesh = nullptr;
			OGLMesh*	cubeMesh	= nullptr;
			OGLMesh*	sphereMesh	= nullptr;
			OGLTexture* basicTex	= nullptr;
			OGLShader*	basicShader = nullptr;

			//Coursework Meshes
			OGLMesh*	charMeshA	= nullptr;
			OGLMesh*	charMeshB	= nullptr;
			OGLMesh*	enemyMesh	= nullptr;
			OGLMesh*	bonusMesh	= nullptr;

			//Coursework Additional functionality	
			GameObject* lockedObject	= nullptr;
			Vector3		lockedOffset	= Vector3(0, 14, 20);
			void LockCameraToObject(GameObject* obj, Vector3 lockedOffset, GameWorld* world);

			//Courseware StateMachine
			StateGameObject* AddStateObjectToWorld(const Vector3& position);
			StateGameObject* AddStateObjectToWorld(const Vector3& position, Vector3 dimensions, string objectName, string tag, float inverseMass = 0.0f, Vector4 color = Vector4(1, 1, 1, 1));
			StateGameObject* testStateObject	= nullptr;
			StateGameObject* obsStateObject		= nullptr;

		};

#pragma region GameStateClass

		class PauseState : public PushdownState
		{
			PushdownResult OnUpdate(float dt, PushdownState** newState) override
			{
				Debug::Print("Resume ---- Press P", Vector2(35, 30));
				Debug::Print("Menu   ---- Press ESC", Vector2(35, 40));
				if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::P))
				{
					return PushdownResult::Pop;
				}
				if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::ESCAPE))
				{
					popTimes = 2;
					return PushdownResult::Pop;
				}
				return PushdownResult::NoChange;
			}
			void OnAwake() override
			{
				stateName = "PauseState";
			}
		};

		class Mode1State : public PushdownState
		{
			PushdownResult OnUpdate(float dt, PushdownState** newState) override
			{
				Debug::Print("Pause ---- Press P", Vector2(35, 10));
				if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::P) ||
					Window::GetKeyboard()->KeyPressed(KeyboardKeys::ESCAPE))
				{
					*newState = new PauseState();
					return PushdownResult::Push;
				}
				return PushdownResult::NoChange;
			}
			void OnAwake() override
			{
				stateName = "Mode1State";
			}
		};

		class Mode2State : public PushdownState
		{
			PushdownResult OnUpdate(float dt, PushdownState** newState) override
			{
				Debug::Print("Pause ---- Press P", Vector2(35, 10));
				if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::P) ||
					Window::GetKeyboard()->KeyPressed(KeyboardKeys::ESCAPE))
				{
					*newState = new PauseState();
					return PushdownResult::Push;
				}
				return PushdownResult::NoChange;
			}
			void OnAwake() override
			{
				stateName = "Mode2State";
			}
		};

		class MenuState : public PushdownState
		{
			PushdownResult OnUpdate(float dt, PushdownState** newState) override
			{
				Debug::Print("Game Mode 1 ---- Press 1", Vector2(25, 40));
				Debug::Print("Game Mode 2 ---- Press 2", Vector2(25, 50));
				Debug::Print("       Quit ---- Press ESC", Vector2(25, 60));

				if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM1) ||
					Window::GetKeyboard()->KeyDown(KeyboardKeys::NUMPAD1))
				{
					*newState = new Mode1State();
					std::cout << "Change State" << std::endl;
					return PushdownResult::Push;
				}
				if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM2) ||
					Window::GetKeyboard()->KeyDown(KeyboardKeys::NUMPAD2))
				{
					*newState = new Mode2State();
					return PushdownResult::Push;
				}
				if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::ESCAPE))
				{
					return PushdownResult::Pop;
				}
				return PushdownResult::NoChange;
			}
			void OnAwake() override
			{
				stateName = "MenuState";
				//std::cout << "MenuState" << std::endl;
			}
		};

		class WinState : public PushdownState
		{
			PushdownResult OnUpdate(float dt, PushdownState** newState) override
			{
				Debug::Print("You Win!", Vector2(45, 40));
				//Debug::Print("Restart ---- Press R", Vector2(35, 50));
				Debug::Print("Menu ---- Press ESC", Vector2(35, 50));
				if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::ESCAPE))
				{
					//popTimes = 2;
					return PushdownResult::Pop;
				}
				return PushdownResult::NoChange;
			}
			void OnAwake() override
			{
				stateName = "WinState";
			}
		};

		class LoseState : public PushdownState
		{
			PushdownResult OnUpdate(float dt, PushdownState** newState) override
			{
				Debug::Print("You Lose!", Vector2(45, 40));
				//Debug::Print("Restart ---- Press R", Vector2(35, 50));
				Debug::Print("Menu ---- Press ESC", Vector2(35, 50));
				if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::ESCAPE))
				{
					//popTimes = 2;
					return PushdownResult::Pop;
				}
				return PushdownResult::NoChange;
			}
			void OnAwake() override
			{
				stateName = "LoseState";
			}
		};
#pragma endregion
	}
}

