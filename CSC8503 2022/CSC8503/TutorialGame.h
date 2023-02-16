#pragma once
#include "GameTechRenderer.h"
#ifdef USEVULKAN
#include "GameTechVulkanRenderer.h"
#endif
#include "PhysicsSystem.h"

#include "PositionConstraint.h"
#include "OrientationConstraint.h"
#include "StateGameObject.h"
#include "BehaviourGameObject.h"
#include "NavigationGrid.h"
#include <fstream>
#include "Assets.h"

namespace NCL {
	namespace CSC8503 {
		class Rocket : public GameObject {
		public:
			void OnCollisionBegin(GameObject* otherobj);
		protected:
			float explosionForce = 20.0f;
			float radius = 10.0f;
			float inverseMass = 0.0f;
		};

		class Coin : public GameObject {
		public:
			Coin(GameObject* obj, GameWorld* w) { playerObj = obj; world = w; }
			void OnCollisionBegin(GameObject* otherobj);
		protected:
			GameObject* playerObj;
			GameWorld* world;
			bool coinCollected = false;
		};

		class TutorialGame		{
		public:
			TutorialGame(int a) {} //For debugging
			TutorialGame();
			~TutorialGame();

			virtual void UpdateGame(float dt);
			GameObject* GetPlayer() { return lockedObject; };
			int GetGameState() { return gameState; }
			void ResetGame() {
				gameState == 2;
				player->ResetScore();
			}
			void SetGameState(int s) { gameState == s; }

		protected:
			void InitialiseAssets();

			void InitCamera();
			void UpdateKeys();

			void InitWorld();

			/*
			These are some of the world/object creation functions I created when testing the functionality
			in the module. Feel free to mess around with them to see different objects being created in different
			test scenarios (constraints, collision types, and so on). 
			*/
			void InitGameExamples();

			void InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius);
			void InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing);
			void InitCubeGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, const Vector3& cubeDims);
			void InitMaze(const std::string&filename);
			void InitDefaultFloor();

			bool SelectObject();
			void MoveSelectedObject();
			void DebugObjectMovement();
			void LockedObjectMovement();
			void LockedCameraMovement();

			GameObject* AddFloorToWorld(const Vector3& position);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f);
			Coin* AddCoinToWorld(const Vector3& position);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f);
			void BridgeConstraint();
			void RopeSwing();
			void TetherObjects();
			GameObject* toBeTethered;
			GameObject* TetheredTo;
			PositionConstraint* tetherConst;

			GameObject* AddPlayerToWorld(const Vector3& position);
			GameObject* AddEnemyToWorld(const Vector3& position);
			GameObject* AddBonusToWorld(const Vector3& position);

			StateGameObject* AddStateObjectToWorld(const Vector3& position);
			BehaviourGameObject* AddGooseToWorld(const Vector3& position);
			Rocket* CreateRocket(const Vector3& position, float radius);

#ifdef USEVULKAN
			GameTechVulkanRenderer*	renderer;
#else
			GameTechRenderer* renderer;
#endif
			PhysicsSystem*		physics;
			GameWorld*			world;

			bool useGravity;
			bool inSelectionMode;

			//Items and weapons functionality:
			enum Item { //Used to store which weapon is equipped
				Rope,
				Rockets,
				None
			};
			Item currentItem = None;

			void FireRocket();

			bool isSwinging = false;
			vector<GameObject*> links;
			vector<PositionConstraint*> constraints;

			float		forceMagnitude;

			GameObject* selectionObject = nullptr;
			GameObject* prevSelectionObject = nullptr;

			MeshGeometry*	capsuleMesh = nullptr;
			MeshGeometry*	cubeMesh	= nullptr;
			MeshGeometry*	sphereMesh	= nullptr;

			TextureBase*	basicTex	= nullptr;
			ShaderBase*		basicShader = nullptr;

			//Coursework Meshes
			MeshGeometry*	charMesh	= nullptr;
			MeshGeometry*	enemyMesh	= nullptr;
			MeshGeometry*	bonusMesh	= nullptr;

			StateGameObject* testStateObject;
			BehaviourGameObject* testBehaviourObject;
			GameObject* player;
			
			enum GameState {
				Win = 1,
				Loss = 0,
				Ongoing = 2
			};
			GameState gameState = Ongoing;

			//Coursework Additional functionality	
			GameObject* lockedObject	= nullptr;
			Vector3 lockedOffset		= Vector3(0, 14, 20);
			void LockCameraToObject(GameObject* o) {
				lockedObject = o;
			}

			GameObject* objClosest = nullptr;
		};
	}
}

