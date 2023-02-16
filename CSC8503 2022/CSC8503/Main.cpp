#include "Window.h"

#include "Debug.h"

#include "StateMachine.h"
#include "StateTransition.h"
#include "State.h"

#include "GameServer.h"
#include "GameClient.h"

#include "NavigationGrid.h"
#include "NavigationMesh.h"

#include "TutorialGame.h"
#include "NetworkedGame.h"

#include "PushdownMachine.h"
#include "PushdownState.h"

#include "BehaviourNode.h"
#include "BehaviourSelector.h"
#include "BehaviourSequence.h"
#include "BehaviourAction.h"

using namespace NCL;
using namespace CSC8503;

#include <chrono>
#include <thread>
#include <sstream>

vector<Vector3> testNodes;
void TestPathfinding() {
	NavigationGrid grid("Maze.txt");
	NavigationPath outPath;

	Vector3 startPos(80, 0, 10);
	Vector3 endPos(80, 0, 80);

	bool found = grid.FindPath(startPos, endPos, outPath);

	Vector3 pos;
	while (outPath.PopWaypoint(pos)) {
		testNodes.push_back(pos);
	}
}

void DisplayPathfinding() {
	for (int i = 1; i < testNodes.size(); ++i) {
		Vector3 a = testNodes[i - 1];
		Vector3 b = testNodes[i];
		Debug::DrawLine(a, b, Debug::CYAN);
	}
}

/*

The main function should look pretty familar to you!
We make a window, and then go into a while loop that repeatedly
runs our 'game' until we press escape. Instead of making a 'renderer'
and updating it, we instead make a whole game, and repeatedly update that,
instead. 

This time, we've added some extra functionality to the window class - we can
hide or show the 

*/
void TestStateMachine() {
	StateMachine* testMachine = new StateMachine();
	int data = 0;
	State* A = new State([&](float dt)->void {
		std::cout << "State A \n";
		data++;
		}
	);

	State* B = new State([&](float dt)->void {
		std::cout << "State B \n";
		data--;
		}
	);

	StateTransition* stateAB = new StateTransition(A, B, [&](void)->bool {
		return data > 10;
		}
	);

	StateTransition* stateBA = new StateTransition(B, A, [&](void)->bool {
		return data < 0;
		}
	);

	testMachine->AddState(A);
	testMachine->AddState(B);
	testMachine->AddTransition(stateAB);
	testMachine->AddTransition(stateBA);

	for (int i = 0; i < 100; ++i) {
		testMachine->Update(1.0f);
	}
}

void TestBehaviourTree() {
	float behaviourTimer;
	float distanceToTarget;
	BehaviourAction* findKey = new BehaviourAction("Find Key", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Looking for a key! \n";
			behaviourTimer = rand() % 100;
			state = Ongoing;
		}
		else if (state == Ongoing) {
			behaviourTimer -= dt;
			if (behaviourTimer <= 0.0f) {
				std::cout << "Found a key! \n";
				return Success;
			}
		}
		return state; //will be 'ongoing' until success
	});

	BehaviourAction* goToRoom = new BehaviourAction("Go To Room", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Going to the loot room! \n";
			state = Ongoing;
		}
		else if (state == Ongoing) {
			distanceToTarget -= dt;
			if (distanceToTarget <= 0.0f) {
				std::cout << "Reached room! \n";
				return Success;
			}
		}
		return state; //will be 'ongoing' until success
	});

	BehaviourAction* openDoor = new BehaviourAction("Open Door", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Opening Door! \n";
			state = Success;
		}
		return state; //will be 'ongoing' until success
	});

	BehaviourAction* lookForTreasure = new BehaviourAction("Look For Treasure", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Looking for Treasure! \n";
			state = Ongoing;
		}
		else if (state == Ongoing) {
			bool found = rand() % 2;
			if (found) {
				std::cout << "I found some treasure! \n";
				return Success;
			}
			std::cout << "No treasure in here... \n";
			return Failure;
		}
		return state; //will be 'ongoing' until success
		});

	BehaviourAction* lookForItems = new BehaviourAction("Looking For Items", [&](float dt, BehaviourState state)->BehaviourState {
		if (state == Initialise) {
			std::cout << "Looking for Items! \n";
			state = Ongoing;
		}
		else if (state == Ongoing) {
			bool found = rand() % 2;
			if (found) {
				std::cout << "I found some items! \n";
				return Success;
			}
			std::cout << "No items in here... \n";
			return Failure;
		}
		return state; //will be 'ongoing' until success
		});

	BehaviourSequence* sequence = new BehaviourSequence("Room Sequence");
	sequence->AddChild(findKey);
	sequence->AddChild(goToRoom);
	sequence->AddChild(openDoor);

	BehaviourSelector* selection = new BehaviourSelector("Loot Selection");
	selection->AddChild(lookForTreasure);
	selection->AddChild(lookForItems);

	BehaviourSequence* rootSequence = new BehaviourSequence("Root Sequence");
	rootSequence->AddChild(sequence);
	rootSequence->AddChild(selection);

	for (int i = 0; i < 1; ++i) {
		rootSequence->Reset();
		behaviourTimer = 0.0f;
		distanceToTarget = rand() % 250;
		BehaviourState state = Ongoing;
		std::cout << "We're going on an adventure! \n";
		while (state == Ongoing) {
			state = rootSequence->Execute(1.0f);
		}
		if (state == Success) {
			std::cout << "What a successful adventure! \n";
		}
		else if (state == Failure) {
			std::cout << "What a waste of time! \n";
		}
	}
	std::cout << "All done! \n";
}

class TestPacketReciever : public PacketReceiver {
public:
	TestPacketReciever(string name) {
		this->name = name;
	}

	void ReceivePacket(int type, GamePacket* payload, int source) {
		if (type == String_Message) {
			StringPacket* realPacket = (StringPacket*)payload;
			string msg = realPacket->GetStringFromData();
			std::cout << name << " received message: " << msg << std::endl;
		}
	}
protected:
	string name;
};

//void TestNetworking() {
//	NetworkBase::Initialise();
//
//	TestPacketReciever serverReciever("Server");
//	TestPacketReciever clientReciever("Client");
//
//	int port = NetworkBase::GetDefaultPort();
//
//	GameServer* server = new GameServer(port, 1);
//	GameClient* client = new GameClient();
//
//	server->RegisterPacketHandler(String_Message, &serverReciever);
//	client->RegisterPacketHandler(String_Message, &clientReciever);
//
//	bool canConnect = client->Connect(127, 0, 0, 1, port);
//
//	for (int i = 0; i < 100; ++i) {
//		server->SendGlobalPacket(StringPacket("Server says hello! " + std::to_string(i)));
//		client->SendPacket(StringPacket("Client says hello! " + std::to_string(i)));
//	
//		server->UpdateServer();
//		client->UpdateClient();
//
//		std::this_thread::sleep_for(std::chrono::milliseconds(10));
//	}
//
//	NetworkBase::Destroy();
//}

class PauseScreen : public PushdownState {
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::U)) {
			return PushdownResult::Pop;
		}
		return PushdownResult::NoChange;
	}
	void OnAwake() override {
		std::cout << "Press U to unpause game! \n";
	}
};

class GameScreen : public PushdownState {
public:
	GameScreen(TutorialGame* tg) {
		g = tg;
	}
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
         		pauseReminder -= dt;
		if (pauseReminder < 0) {
			//std::cout << "Press P to pause game, or F1 to return to main menu! \n";
			pauseReminder += 1.0f;
		}
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::P)) {
			*newState = new PauseScreen();
			return PushdownResult::Push;
		}
		if (Window::GetKeyboard()->KeyDown(KeyboardKeys::F1)) {
			std::cout << "Returning to main menu! \n";
			return PushdownResult::Pop;
		}
		if (g->GetGameState() == 1) {
			Debug::Print("You won!", Vector2(1, 10), Debug::RED);
			g->ResetGame();
			return PushdownResult::Pop;
		}
		if (g->GetGameState() == 0) {
			Debug::Print("You lost!", Vector2(1, 10), Debug::RED);
			g->ResetGame();
			return PushdownResult::Pop;
		}

		// Game functionality:
		g->UpdateGame(dt);
		DisplayPathfinding();
		//TestBehaviourTree();

		return PushdownResult::NoChange;
	};

	void OnAwake() override {
		std::cout << "Preparing start the game! \n";
		TestPathfinding();
	}
protected:
	TutorialGame* g;
	float pauseReminder = 5;
};

class IntroScreen : public PushdownState {
public:
	IntroScreen(TutorialGame* tg) {
		g = tg;
	}
	PushdownResult OnUpdate(float dt, PushdownState** newState) override {
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::SPACE)) {
			g->ResetGame();
			*newState = new GameScreen(g);
			return PushdownResult::Push;
		}
		return PushdownResult::NoChange;
	}
	void OnAwake() override {
		std::cout << "Welcome to the game! \n";
		std::cout << "Press Space To Begin or ESC to quit! \n";
	}
protected:
	TutorialGame* g;
};

void RunGame(Window* w, TutorialGame* tg) {
	PushdownMachine machine(new IntroScreen(tg));
	w->GetTimer()->GetTimeDeltaSeconds(); //Clear the timer so we don't get a larget first dt!

	while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyboardKeys::ESCAPE)) {
		float dt = w->GetTimer()->GetTimeDeltaSeconds();
		if (dt > 0.1f) {
			std::cout << "Skipping large time delta" << std::endl;
			continue; //must have hit a breakpoint or something to have a 1 second frame time!
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::PRIOR)) {
			w->ShowConsole(true);
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::NEXT)) {
			w->ShowConsole(false);
		}
		if (Window::GetKeyboard()->KeyPressed(KeyboardKeys::T)) {
			w->SetWindowPosition(0, 0);
		}

		w->SetTitle("Gametech frame time:" + std::to_string(1000.0f * dt));

		if (!machine.Update(dt)) {
			return;
		}
	}
}

int main() {
	Window* w = Window::CreateGameWindow("CSC8503 Game technology!", 1280, 720);

	if (!w->HasInitialised()) {
		return -1;
	}

	w->ShowOSPointer(false);
	w->LockMouseToWindow(true);

	TutorialGame* g = new TutorialGame();
	RunGame(w, g);

	Window::DestroyGameWindow();
}