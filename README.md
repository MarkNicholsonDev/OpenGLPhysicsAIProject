# OpenGL Physics and AI project

A showcase of a physics engine built from scratch with multiple different forms of AI such as behaviour tree controlled agents and physics
based game mechanics such as grappling hooks and tethering objects together with rope.

See my portfolio for additional information: https://marknicholsondev.github.io/opengl-gametech

## List of Features
| Name | Description |
| ------------- | ------------- |
| Grappling Hook | Created from a chain of distance constraints to simulate the physics of a rope |
| Rope Tethering | Allows the player to tie two objects together with a fixed length of rope |
| Simple Rocket | Causes an explosion on mouse click, with scaling explosive force based off distance to the center of the explosion |
| Behaviour Tree AI agent (Goose) | The AI uses a behaviour tree with multiple different behaviours and transition states between them. |
| Menu System | Controlled by Pushdown Automata which pushes and pops game states similarly to a stack allowing for  |
| Simple State machine AI (Moving platform) | Platform which moves side to side based off of a timer |
| Maze generation | Done via reading in a txt file allowing for half walls, walls and open areas to be spawned in |

## Controls within the scene
### General controls:
Q = Switch in and out of selection and camera mode

Free Camera:
WASD = Move the camera forward, backwards, left and right.

Space = Fly camera up

Shift = Fly camera down

Left click + L = Lock onto selected Object

### Once locked on object:
Arrow keys = Movement of locked object

Space to jump (Hold to fly)

Press 1,2,3 to equip Rope, Rockets and Nothing

While selection mode, on hold left click on object = Create rope swing (While rope item equipped)

While selection mode, right click on two different objects = Tether objects together (While rope item equipped)

To untether once tethered right click again.

Left click + rocket equipped = Fire rocket.

## Known Bugs
There is an issue with the pathfinding of the goose which causes crashing on being set to Patrolling a route.
As a temporary solution, that section is commented out so the rest of the demo can be enjoyed still while I make a fix.

