#include "TutorialGame.h"
#include "GameWorld.h"
#include "PhysicsObject.h"
#include "RenderObject.h"
#include "TextureLoader.h"
#include "Maths.h"

#include <algorithm>

using namespace NCL;
using namespace CSC8503;
using namespace Maths;
using namespace std; 

void Rocket::OnCollisionBegin(GameObject* otherobj) {
    
    PhysicsObject* objPhys = otherobj->GetPhysicsObject();
	Vector3 direction = (otherobj->GetTransform().GetPosition() - this->GetTransform().GetPosition()).Normalised();
	float distance = (otherobj->GetTransform().GetPosition() / this->GetTransform().GetPosition()).Length();
	float distanceScale = 1.0 - Maths::Clamp(distance / radius, 0.0f, 1.0f);
	objPhys->AddForce(direction * (explosionForce * distanceScale));
	this->SetIsActive(false); //Hides the sphere from the rocket (However, this leaves the collider)
	//world->RemoveGameObject(this);
	//delete this;
}

void Coin::OnCollisionBegin(GameObject* otherobj) {
	if (otherobj == playerObj && coinCollected == false) {
		otherobj->IncrementScore();
		coinCollected = true;
		this->SetIsActive(false);
		//world->RemoveGameObject(this);
		//delete this;
	}
}

TutorialGame::TutorialGame()	{
	world		= new GameWorld();
#ifdef USEVULKAN
	renderer	= new GameTechVulkanRenderer(*world);
#else 
	renderer = new GameTechRenderer(*world);
#endif

	physics		= new PhysicsSystem(*world);

	forceMagnitude	= 10.0f;
	useGravity		= false;
	inSelectionMode = false;

	InitialiseAssets();
}

/*

Each of the little demo scenarios used in the game uses the same 2 meshes, 
and the same texture and shader. There's no need to ever load in anything else
for this module, even in the coursework, but you can add it if you like!

*/
void TutorialGame::InitialiseAssets() {
	cubeMesh	= renderer->LoadMesh("cube.msh");
	sphereMesh	= renderer->LoadMesh("sphere.msh");
	charMesh	= renderer->LoadMesh("goat.msh");
	enemyMesh	= renderer->LoadMesh("goose.msh");
	bonusMesh	= renderer->LoadMesh("apple.msh");
	capsuleMesh = renderer->LoadMesh("capsule.msh");

	basicTex	= renderer->LoadTexture("checkerboard.png");
	basicShader = renderer->LoadShader("scene.vert", "scene.frag");

	InitCamera();
	InitWorld();
}

TutorialGame::~TutorialGame()	{
	delete cubeMesh;
	delete sphereMesh;
	delete charMesh;
	delete enemyMesh;
	delete bonusMesh;

	delete basicTex;
	delete basicShader;

	delete physics;
	delete renderer;
	delete world;
}

void TutorialGame::UpdateGame(float dt) {
	if (!inSelectionMode) {
		world->GetMainCamera()->UpdateCamera(dt);
	}
	if (lockedObject != nullptr) {
		LockedCameraMovement();
	}

	UpdateKeys();

	if (useGravity) {
		Debug::Print("(G)ravity on", Vector2(5, 95), Debug::RED);
	}
	else {
		Debug::Print("(G)ravity off", Vector2(5, 95), Debug::RED);
	}

	if (testStateObject) {
		testStateObject->Update(dt);
	}

	/*if (testBehaviourObject) {
		testBehaviourObject->Update(dt);
	}*/

	RayCollision closestCollision;
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::K) && selectionObject) {
		Vector3 rayPos;
		Vector3 rayDir;

		rayDir = selectionObject->GetTransform().GetOrientation() * Vector3(0, 0, -1);

		rayPos = selectionObject->GetTransform().GetPosition();

		Ray r = Ray(rayPos, rayDir);

		if (world->Raycast(r, closestCollision, true, selectionObject)) {
			if (objClosest) {
				objClosest->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
			}
			objClosest = (GameObject*)closestCollision.node;

			objClosest->GetRenderObject()->SetColour(Vector4(1, 0, 1, 1));
		}
	}
	Debug::Print("Score: " + std::to_string(player->GetScore()) +"/3", Vector2(1, 10), Debug::WHITE);
	if (player->GetScore() >= 3) {
		gameState = Win;
	}

	/*if (timer <= 0) {
		gameState = Loss;
	}*/

	Debug::DrawLine(Vector3(), Vector3(0, 100, 0), Vector4(1, 0, 0, 1));

	SelectObject();
	//MoveSelectedObject();

	world->UpdateWorld(dt);
	renderer->Update(dt);
	physics->Update(dt);

	renderer->Render();
	Debug::UpdateRenderables(dt);
}

void TutorialGame::UpdateKeys() {
	if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::F1)) {
		InitWorld(); //We can reset the simulation at any time with F1
		selectionObject = nullptr;
	}

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
	}
	else {
		DebugObjectMovement();
	}
}

void TutorialGame::LockedObjectMovement() {
	
	Camera* cam = world->GetMainCamera();

	float yaw = -(Window::GetMouse()->GetRelativePosition().x);
	
	if (yaw < 0) {
		yaw += 360.0f;
	}
	if (yaw > 360.0f) {
		yaw -= 360.0f;
	}
	
	lockedObject->GetTransform().SetOrientation(lockedObject->GetTransform().GetOrientation() * Matrix4::Rotation(yaw, Vector3(0, 1, 0)));
	//testBehaviourObject->SetPlayerObj(lockedObject);

	Matrix4 view		= world->GetMainCamera()->BuildViewMatrix();
	Matrix4 camWorld	= view.Inverse();

	Vector3 rightAxis = Vector3(camWorld.GetColumn(0)); //view is inverse of model!

	//forward is more tricky -  camera forward is 'into' the screen...
	//so we can take a guess, and use the cross of straight up, and
	//the right axis, to hopefully get a vector that's good enough!
	Vector3 fwdAxis = Vector3::Cross(Vector3(0, 1, 0), rightAxis);
	fwdAxis.y = 0.0f;
	fwdAxis.Normalise();

	if (Window::GetMouse()->ButtonHeld(MouseButtons::LEFT) && currentItem == Rope) {
		isSwinging = true;
		RopeSwing();
	}
	else {
		isSwinging = false;
		RopeSwing();
	}
	if (Window::GetMouse()->ButtonPressed(MouseButtons::RIGHT) && currentItem == Rope) { 
		TetherObjects();
		cout << "Tether fired";
	}
	if (toBeTethered && TetheredTo) {
		Debug::DrawLine(toBeTethered->GetTransform().GetPosition(), TetheredTo->GetTransform().GetPosition(), Debug::BLACK);
	}

	if (Window::GetMouse()->ButtonPressed(MouseButtons::LEFT) && currentItem == Rockets) {
		FireRocket();
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM1)) {
		currentItem = Rope;
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM2)) {
		currentItem = Rockets;
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM3)) {
		currentItem = None; //Unequips item
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::SPACE)) {
		lockedObject->GetPhysicsObject()->AddForce(Vector3(0, 1, 0) * 200);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
		lockedObject->GetPhysicsObject()->AddForce(fwdAxis * 10);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
		lockedObject->GetPhysicsObject()->AddForce(-fwdAxis * 10);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
		lockedObject->GetPhysicsObject()->AddForce(rightAxis * 10);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
		lockedObject->GetPhysicsObject()->AddForce(-rightAxis * 10);
	}

	if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NEXT)) {
		lockedObject->GetPhysicsObject()->AddForce(Vector3(0,-10,0));
	}

	switch (currentItem) {
		case Rope: { Debug::Print("Equipped item: Rope", Vector2(1, 15)); }
		case Rockets: { Debug::Print("Equipped item: Rockets", Vector2(1, 15)); }
		case None: { Debug::Print("Equipped item: None", Vector2(1, 15)); }
	}
}

void TutorialGame::LockedCameraMovement() {
	Vector3 objPos = lockedObject->GetTransform().GetPosition();
	float yaw = lockedObject->GetTransform().GetOrientation().ToEuler().y;
	Vector3 camPos = (Matrix4::Rotation(yaw, Vector3(0, 1, 0)) * Matrix4::Translation(lockedOffset)).GetPositionVector();
	camPos += objPos;

	Matrix4 temp = Matrix4::BuildViewMatrix(camPos, objPos, Vector3(0, 1, 0));

	Matrix4 modelMat = temp.Inverse();

	Quaternion q(modelMat);
	Vector3 angles = q.ToEuler();

	world->GetMainCamera()->SetPosition(camPos);
	world->GetMainCamera()->SetYaw(angles.y);
}

void TutorialGame::DebugObjectMovement() {
//If we've selected an object, we can manipulate it with some key presses
	if (inSelectionMode && selectionObject) {
		//Twist the selected object!
		if (selectionObject != nullptr) {
			Debug::Print(std::to_string(selectionObject->GetWorldID()), Vector2(1, 5));
		}
		if (prevSelectionObject != nullptr) {
			Debug::Print(std::to_string(prevSelectionObject->GetWorldID()), Vector2(1, 10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::LEFT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(-10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM7)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, 10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM8)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(0, -10, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::RIGHT)) {
			selectionObject->GetPhysicsObject()->AddTorque(Vector3(10, 0, 0));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::UP)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, -10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::DOWN)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, 0, 10));
		}

		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::NUM5)) {
			selectionObject->GetPhysicsObject()->AddForce(Vector3(0, -10, 0));
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

void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();

	//InitMixedGridWorld(15, 15, 5.0f, 5.0f);
	AddCubeToWorld(Vector3(-175, 25, -140), Vector3(2, 2, 2), 0);

	AddCubeToWorld(Vector3(-175, 0, -113), Vector3(4, 2, 4), 0);
	AddCubeToWorld(Vector3(-175, 2, -120), Vector3(4, 4, 4), 0);

	AddCubeToWorld(Vector3(-175, 3, -153), Vector3(4, 4, 4), 0);
	AddCubeToWorld(Vector3(-175, 5, -160), Vector3(4, 6, 4), 0);

	AddCubeToWorld(Vector3(-120, 0, -100), Vector3(2, 2, 2), 1);
	AddCubeToWorld(Vector3(-130, 0, -100), Vector3(2, 2, 2), 1);

	AddCubeToWorld(Vector3(-120, 15, -100), Vector3(2, 2, 2), 0);
	AddCubeToWorld(Vector3(-130, 15, -100), Vector3(2, 2, 2), 0);

	AddCubeToWorld(Vector3(-20, 4, -140), Vector3(10, 5, 10), 0);
	AddCubeToWorld(Vector3(-20, 33, -140), Vector3(10, 10, 10), 0);
	InitGameExamples();

	vector<Vector3> coinPos = { Vector3(40, 3, -100), Vector3(-113, 18, -160),  Vector3(160, 3, -140) };
	for (int i = 0; i < coinPos.size(); i++) {
		AddCoinToWorld(coinPos.at(i));
	}

	InitDefaultFloor();
	InitMaze("Maze.txt");

	testStateObject = AddStateObjectToWorld(Vector3(-90, 15, -150));
	//AddCubeToWorld(Vector3(-70, 8, -160), Vector3(5, 5, 5), 50);
	//testBehaviourObject = AddGooseToWorld(Vector3(60, 0, 60));

	BridgeConstraint();
}

/*

A single function to add a large immoveable cube to the bottom of our world

*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position) {
	GameObject* floor = new GameObject();

	Vector3 floorSize = Vector3(200, 2, 200);
	AABBVolume* volume = new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(floorSize * 2)
		.SetPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor);

	return floor;
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

GameObject* TutorialGame::AddPlayerToWorld(const Vector3& position) {
	float meshSize		= 1.0f;
	float inverseMass	= 0.5f;

	GameObject* character = new GameObject();
	SphereVolume* volume  = new SphereVolume(1.0f);

	character->SetBoundingVolume((CollisionVolume*)volume);

	character->GetTransform()
		.SetScale(Vector3(meshSize, meshSize, meshSize))
		.SetPosition(position);

	character->SetRenderObject(new RenderObject(&character->GetTransform(), charMesh, nullptr, basicShader));
	character->SetPhysicsObject(new PhysicsObject(&character->GetTransform(), character->GetBoundingVolume()));

	character->GetPhysicsObject()->SetInverseMass(inverseMass);
	character->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(character);
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

	SphereVolume* volume = new SphereVolume(0.5f);
	apple->SetBoundingVolume((CollisionVolume*)volume);
	apple->GetTransform()
		.SetScale(Vector3(2, 2, 2))
		.SetPosition(position);

	apple->SetRenderObject(new RenderObject(&apple->GetTransform(), bonusMesh, nullptr, basicShader));
	apple->SetPhysicsObject(new PhysicsObject(&apple->GetTransform(), apple->GetBoundingVolume()));

	apple->GetPhysicsObject()->SetInverseMass(1.0f);
	apple->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(apple);

	return apple;
}

Coin* TutorialGame::AddCoinToWorld(const Vector3& position) {
	Coin* coin = new Coin(player, world);

	SphereVolume* volume = new SphereVolume(1.0f);
	coin->SetBoundingVolume((CollisionVolume*)volume);
	coin->GetTransform()
		.SetScale(Vector3(2, 2, 2))
		.SetPosition(position);

	coin->SetRenderObject(new RenderObject(&coin->GetTransform(), sphereMesh, nullptr, basicShader));
	coin->SetPhysicsObject(new PhysicsObject(&coin->GetTransform(), coin->GetBoundingVolume()));
	coin->GetRenderObject()->SetColour(Vector4(1, 1, 0, 1));

	coin->GetPhysicsObject()->SetInverseMass(0.0f);
	coin->GetPhysicsObject()->InitSphereInertia();
	world->AddGameObject(coin);
	return coin;
}

StateGameObject* TutorialGame::AddStateObjectToWorld(const Vector3& position) {
	StateGameObject* obstacle = new StateGameObject();

	AABBVolume* volume = new AABBVolume(Vector3(10, 2, 10));
	obstacle->SetBoundingVolume((CollisionVolume*)volume);
	obstacle->GetTransform()
		.SetScale(Vector3(10, 2, 10))
		.SetPosition(position);

	obstacle->SetRenderObject(new RenderObject(&obstacle->GetTransform(), cubeMesh, nullptr, basicShader));
	obstacle->SetPhysicsObject(new PhysicsObject(&obstacle->GetTransform(), obstacle->GetBoundingVolume()));

	obstacle->GetPhysicsObject()->SetInverseMass(0.0f);
	obstacle->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(obstacle);

	return obstacle;
}

BehaviourGameObject* TutorialGame::AddGooseToWorld(const Vector3& position) {
	BehaviourGameObject* goose = new BehaviourGameObject(world, player);

	AABBVolume* volume = new AABBVolume(Vector3(4, 4, 4));
	goose->SetBoundingVolume((CollisionVolume*)volume);
	goose->GetTransform()
		.SetScale(Vector3(4, 4, 4))
		.SetPosition(position);

	goose->SetRenderObject(new RenderObject(&goose->GetTransform(), enemyMesh, nullptr, basicShader));
	goose->SetPhysicsObject(new PhysicsObject(&goose->GetTransform(), goose->GetBoundingVolume()));

	goose->GetPhysicsObject()->SetInverseMass(1.0f);
	goose->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(goose);

	return goose;
}

Rocket* TutorialGame::CreateRocket(const Vector3& position, float radius) {
	Rocket* rocket = new Rocket();

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	rocket->SetBoundingVolume((CollisionVolume*)volume);

	rocket->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	rocket->SetRenderObject(new RenderObject(&rocket->GetTransform(), sphereMesh, basicTex, basicShader));
	rocket->SetPhysicsObject(new PhysicsObject(&rocket->GetTransform(), rocket->GetBoundingVolume()));

	rocket->GetPhysicsObject()->SetInverseMass(0);
	rocket->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(rocket);

	return rocket;
}

void TutorialGame::BridgeConstraint() {
	Vector3 cubeSize = Vector3(2, 2, 2);
	
	float invCubeMass = 5; //how heavy the middle pieces are
	int numLinks = 5;
	float maxDistance = 10; // constraint distance
	float cubeDistance = 8; // distance between links

	Vector3 startPos = Vector3(-170, 13, -160);
	GameObject * start = AddCubeToWorld(startPos + Vector3(0, 0, 0), cubeSize, 0);
	GameObject * end = AddCubeToWorld(startPos + Vector3((numLinks + 2) * cubeDistance, 0, 0), cubeSize, 0);
	
	GameObject * previous = start;
	for (int i = 0; i < numLinks; ++i) {
		GameObject * block = AddCubeToWorld(startPos + Vector3((i + 1) * cubeDistance, 0, 0), cubeSize, invCubeMass);
		PositionConstraint * constraint = new PositionConstraint(previous, block, maxDistance);
		world->AddConstraint(constraint);
		previous = block;
		
	}
	PositionConstraint * constraint = new PositionConstraint(previous, end, maxDistance);
	world->AddConstraint(constraint);
}

void TutorialGame::RopeSwing() {
	Vector3 cubeSize = Vector3(1, 1, 1);
	float invCubeMass = 1; //how heavy the middle pieces are
	int numLinks = 1;
	float maxDistance = 10; // constraint distance

	if (!isSwinging && !links.empty()) {
		world->RemoveGameObject(links.at(0));
		world->RemoveConstraint(constraints.at(0));
		world->RemoveConstraint(constraints.at(1));
		links.clear();
		constraints.clear();
		return;
	}

	else if (isSwinging && links.empty() && currentItem == Rope) {
		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());
		RayCollision closestCollision;
		world->Raycast(ray, closestCollision, true);
		Vector3 startPos = closestCollision.collidedAt;

		GameObject* start = (GameObject*)closestCollision.node;
		GameObject* end = lockedObject;
		if (start != end) {
			GameObject* block = AddCubeToWorld((startPos + lockedObject->GetTransform().GetPosition()) / 2, cubeSize, invCubeMass);

			PositionConstraint* constraint = new PositionConstraint(start, block, maxDistance);
			constraints.push_back(constraint);
			world->AddConstraint(constraint);
			links.push_back(block);

			PositionConstraint* constraint2 = new PositionConstraint(block, end, maxDistance);
			constraints.push_back(constraint2);
			world->AddConstraint(constraint2);
		}
		else { isSwinging = 0; }
	}
}

void TutorialGame::TetherObjects() {
	float maxDistance = 10; // constraint distance
	Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());
	RayCollision closestCollision;
	world->Raycast(ray, closestCollision, true);
	Vector3 startPos = closestCollision.collidedAt;

	if (!tetherConst && toBeTethered) {
		GameObject* start = toBeTethered;
		GameObject* end = (GameObject*)closestCollision.node;
		TetheredTo = end;
		tetherConst = new PositionConstraint(start, end, maxDistance);
		world->AddConstraint(tetherConst);
	}
	else if (!toBeTethered) {
		toBeTethered = (GameObject*)closestCollision.node;
	}
	else if (tetherConst && toBeTethered) {
		world->RemoveConstraint(tetherConst);
		toBeTethered = NULL;
	}
}

void TutorialGame::FireRocket() {
	if (currentItem == Rockets) {
		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());
		RayCollision closestCollision;
		world->Raycast(ray, closestCollision, true);
		Vector3 position = closestCollision.collidedAt;
		CreateRocket(position, 10.0f);
	}
}

void TutorialGame::InitDefaultFloor() {
	AddFloorToWorld(Vector3(0, -3, 0));
}

void TutorialGame::InitGameExamples() {
	player = AddPlayerToWorld(Vector3(-140, 0, -120));
}

void TutorialGame::InitSphereGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing, float radius) {
	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 10.0f, z * rowSpacing);
			AddSphereToWorld(position, radius, 1.0f);
		}
	}
	AddFloorToWorld(Vector3(0, -2, 0));
}

void TutorialGame::InitMixedGridWorld(int numRows, int numCols, float rowSpacing, float colSpacing) {
	float sphereRadius = 1.0f;
	Vector3 cubeDims = Vector3(1, 1, 1);

	for (int x = 0; x < numCols; ++x) {
		for (int z = 0; z < numRows; ++z) {
			Vector3 position = Vector3(x * colSpacing, 2.0f, z * rowSpacing);

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

void TutorialGame::InitMaze(const std::string& filename) {
	NavigationGrid grid(filename);

	std::ifstream infile(Assets::DATADIR + filename);
	int nodeSize;
	int gridWidth;
	int gridHeight;
	infile >> nodeSize;
	infile >> gridWidth;
	infile >> gridHeight;

	GridNode* allNodes = new GridNode[gridWidth * gridHeight];

	for (int y = 0; y < gridHeight; ++y) {
		for (int x = 0; x < gridWidth; ++x) {
			GridNode& n = allNodes[(gridWidth * y) + x];
			char type = 0;
			infile >> type;
			n.type = type;
			n.position = Vector3((float)(x * nodeSize), 0, (float)(y * nodeSize));
			if (type == 'x') { 
				AddCubeToWorld(n.position + Vector3(-200, 8, -180), Vector3(10, 15, 10), 0); }
		}
	}
}
/*
Every frame, this code will let you perform a raycast, to see if there's an object
underneath the cursor, and if so 'select it' into a pointer, so that it can be 
manipulated later. Pressing Q will let you toggle between this behaviour and instead
letting you move the camera around. 

*/
bool TutorialGame::SelectObject() {
	if (selectionObject) {
		Debug::DrawAxisLines(selectionObject->GetTransform().GetMatrix());
	}
	
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
		Debug::Print("Press Q to change to camera mode!", Vector2(5, 85));

		if (Window::GetMouse()->ButtonDown(NCL::MouseButtons::LEFT)) {
			if (selectionObject) {	//set colour to deselected;
				selectionObject->GetRenderObject()->SetColour(Vector4(1, 1, 1, 1));
				selectionObject = nullptr;
			}

			Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

			RayCollision closestCollision;
			if (world->Raycast(ray, closestCollision, true)) {
				selectionObject = (GameObject*)closestCollision.node;

				selectionObject->GetRenderObject()->SetColour(Vector4(0, 1, 0, 1));
				return true;
			}
			else {
				return false;
			}
		}
		if (Window::GetKeyboard()->KeyPressed(NCL::KeyboardKeys::L)) {
			if (selectionObject) {
				if (lockedObject == selectionObject) {
					lockedObject = nullptr;
				}
				else {
					lockedObject = selectionObject;
				}
			}
		}
	}
	else {
		Debug::Print("Press Q to change to select mode!", Vector2(5, 85));
	}
	return false;
}

/*
If an object has been clicked, it can be pushed with the right mouse button, by an amount
determined by the scroll wheel. In the first tutorial this won't do anything, as we haven't
added linear motion into our physics system. After the second tutorial, objects will move in a straight
line - after the third, they'll be able to twist under torque aswell.
*/

void TutorialGame::MoveSelectedObject() {
	Debug::Print("Click Force:" + std::to_string(forceMagnitude), Vector2(5, 90));
	forceMagnitude += Window::GetMouse()->GetWheelMovement() * 100.0f;

	if (!selectionObject) {
		return;//we haven't selected anything!
	}
	//Push the selected object!
	if (Window::GetMouse()->ButtonPressed(NCL::MouseButtons::RIGHT)) {
		Ray ray = CollisionDetection::BuildRayFromMouse(*world->GetMainCamera());

		RayCollision closestCollision;
		if (world->Raycast(ray, closestCollision, true)) {
			if (closestCollision.node == selectionObject) {
				selectionObject->GetPhysicsObject()->AddForceAtPosition(ray.GetDirection() * forceMagnitude, closestCollision.collidedAt);
			}
		}
	}
}


