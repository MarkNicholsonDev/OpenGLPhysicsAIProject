#pragma once
#include "GameTechRenderer.h"
#include "BehaviourNode.h"
#include "BehaviourSelector.h"
#include "BehaviourSequence.h"
#include "BehaviourAction.h"
#include "GameObject.h"

namespace NCL {
	namespace CSC8503 {
		class BehaviourGameObject : public GameObject {
		public:
			BehaviourGameObject(GameWorld* world, GameObject* player);
			~BehaviourGameObject();

			virtual void Update(float dt);

			void SetPlayerObj(GameObject* obj) { playerObj = obj; };

		protected:
			float behaviourTimer;
			float distanceToTarget;
			vector<Vector3> waypoints;

			BehaviourSequence* rootSequence;
			BehaviourSequence* sequence;
			BehaviourState state;
			int i = 0; //For iterating over the waypoints

			GameObject* playerObj;
			GameWorld* world;
		};
	}
}


