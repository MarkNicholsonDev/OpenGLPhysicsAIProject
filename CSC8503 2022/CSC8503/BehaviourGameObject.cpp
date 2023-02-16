#include "BehaviourGameObject.h"
#include "PhysicsObject.h"
#include "GameWorld.h"
#include <algorithm>
#include "NavigationMap.h"
#include "NavigationGrid.h"

using namespace NCL;
using namespace CSC8503;

BehaviourGameObject::BehaviourGameObject(GameWorld* world, GameObject* player) {
	playerObj = player;
	//Navigation initialisation:
	NavigationGrid grid("Maze.txt");
	NavigationPath outPath;

	Vector3 startPos(380, 0, 40);
	Vector3 endPos(360, 0, 40);

	bool found = grid.FindPath(startPos, endPos, outPath);

	Vector3 pos;
	while (outPath.PopWaypoint(pos)) {
		waypoints.push_back(pos);
	}

	for (int i = 1; i < waypoints.size(); ++i) {
		Vector3 a = waypoints[i - 1];
		Vector3 b = waypoints[i];
		Debug::DrawLine(a , b , Debug::CYAN);
	}

	//Behaviour initialisation
	BehaviourAction* patrolToNext = new BehaviourAction("Go To Room", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Starting Patrol! \n";
			std::cout << waypoints.at(i);
			std::cout << waypoints.at(i + 1);
			state = Ongoing;
		}
		if (state == Ongoing) {
			if (i > waypoints.size()) { i = 0; }
			Vector3 relativePos = waypoints.at(i) - this->GetTransform().GetPosition();
			distanceToTarget = relativePos.Length();
			Vector3 direction = (waypoints.at(i) - this->GetTransform().GetPosition()).Normalised();
			//Vector3 direction = (playerObj->GetTransform().GetPosition() - this->GetTransform().GetPosition()).Normalised();
			if (distanceToTarget < 1.0f) {
				++i;
				std::cout << "Reached Patrol Point! \n";
				return Success;
			}
			else {
				this->GetPhysicsObject()->AddForce(direction * 20);
			}
		}
		else { return Failure; }
		return state; //will be 'ongoing' until success
	});

	BehaviourAction* lookAround = new BehaviourAction("Look Around", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Starting to Look Around! \n";
			state = Ongoing;
		}
		else if (state == Ongoing) {
			behaviourTimer -= dt;
			Debug::Print("Current behtime: " + std::to_string(behaviourTimer) , Vector2(1, 50));
			Debug::Print("Current dtime: " + std::to_string(dt), Vector2(1, 70));
			if (behaviourTimer <= 0.0f) {
				this->GetTransform().SetOrientation(this->GetTransform().GetOrientation() * Matrix4::Rotation(5, Vector3(0, 1, 0)));
				if (playerObj) {
					Vector3 direction = (this->GetTransform().GetPosition() - playerObj->GetTransform().GetPosition()).Normalised();
					Debug::DrawLine(this->GetTransform().GetPosition(), this->GetTransform().GetPosition() + (direction * 10), Debug::RED, 1);
				}
				std::cout << "Looking Around! \n";
				return Success;
			}
		}
		return state; //will be 'ongoing' until success
	});

	BehaviourAction* chasePlayer = new BehaviourAction("Chase Player", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) { //If player is within line of sight chase sequence starts
			std::cout << "Chasing Player! \n";
			if (playerObj) {
				Vector3 direction = (this->GetTransform().GetPosition() - playerObj->GetTransform().GetPosition()).Normalised();
				Ray ray = Ray(this->GetTransform().GetPosition(), direction);
				Debug::DrawLine(this->GetTransform().GetPosition(), this->GetTransform().GetPosition() + (direction * 10), Debug::RED, 1);
				RayCollision collide;

				if (world->Raycast(ray, collide, true, this)) {
					if (collide.node == playerObj) {
						state = Ongoing;
					}
				}
			}
		}
		else if (state == Ongoing) { //If line of sight is broken the chase player sequence ends
			Vector3 direction = (playerObj->GetTransform().GetPosition() - this->GetTransform().GetPosition()).Normalised();
			Ray ray = Ray(this->GetTransform().GetPosition(), direction);
			Debug::DrawLine(this->GetTransform().GetPosition(), playerObj->GetTransform().GetPosition(), Debug::RED, 1);
			RayCollision collide;
			if (world->Raycast(ray, collide, true, this)) {
				if (collide.node != playerObj) {
					return Failure;
				}
			}
			else {
				this->GetPhysicsObject()->AddForce(direction * 10);
			}
		}
		return state; //will be 'ongoing' until success
	});
	
	BehaviourAction* pathToNearest = new BehaviourAction("Path back to Patrol", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Returning to Patrol! \n";
			state = Ongoing;
		}
		else if (state == Ongoing && !waypoints.empty()) {
			Vector3 relativePos = waypoints.at(i - 1) - this->GetTransform().GetPosition();
			distanceToTarget = relativePos.Length();
			Vector3 direction = (this->GetTransform().GetPosition() - waypoints.at(i)).Normalised();
			if (distanceToTarget < 1.0f) {
				++i;
				std::cout << "Returned back to previous point! \n";
				return Success;
			}
			else {
				this->GetPhysicsObject()->AddForce(this->GetTransform().GetPosition() + (direction * 10));
			}
		}
		else { return Failure; }
		return state; //will be 'ongoing' until success
		}
	);

	sequence = new BehaviourSequence("Patrolling Sequence");
	sequence->AddChild(patrolToNext);
	//sequence->AddChild(lookAround);
	//sequence->AddChild(chasePlayer);

	//BehaviourSequence* selection = new BehaviourSequence("Player Detected Sequence");
	//selection->AddChild(pathToNearest);

	rootSequence = new BehaviourSequence("Root Sequence");
	rootSequence->AddChild(sequence);
	//rootSequence->AddChild(selection);

	state = Initialise;
}

BehaviourGameObject::~BehaviourGameObject() {

}

void BehaviourGameObject::Update(float dt) {
	rootSequence->Reset();
	behaviourTimer = 0.0f;
	if (state == Initialise) {
		state = Ongoing;
		std::cout << "We're going on an adventure! \n";
	}
	if (state == Ongoing) {
		state = rootSequence->Execute(dt);
	}
	if (state == Success) {
		std::cout << "What a successful adventure! \n";
	}
	else if (state == Failure) {
		std::cout << "What a waste of time! \n";
	}
}