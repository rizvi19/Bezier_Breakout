Bezier Breakout: Curve Ball Arena
Computer Graphics Lab Final Project

How to run
----------

Option 1: Use the run script

    ./run.sh

The script compiles main.cpp and starts the game.


Option 2: Compile manually

    g++ -std=c++17 main.cpp -o bezier_breakout -lGL -lGLU -lglut
    ./bezier_breakout


Required packages on Linux
--------------------------

You need a C++ compiler and OpenGL/GLUT development libraries.

On Ubuntu/Debian:

    sudo apt install g++ freeglut3-dev mesa-common-dev libglu1-mesa-dev

For sound, the game tries to use one of these if available:

    pw-play
    paplay
    aplay
    ffplay

The game still runs if sound playback is not available.


Controls
--------

A / Left Arrow  - Move paddle left
D / Right Arrow - Move paddle right
Space           - Launch ball / continue
E               - Activate special power
C               - Change camera
P               - Pause
R               - Restart current level
M               - Mute / unmute sound
H               - Show / hide help
G               - Show / hide graphics overlay
Z               - Toggle depth test
ESC             - Show exit confirmation
ESC again       - Exit game
Space on exit prompt - Cancel exit


Demo shortcuts
--------------

1 to 8 - Jump directly to a level
N      - Next level
B      - Previous level

These shortcuts are useful when presenting the project to a teacher.


Project files
-------------

main.cpp               - Main single-file OpenGL game
run.sh                 - Build and run script
PARAMETER_GUIDE.md     - Parameter tuning guide
VIVA_PREPARATION.md    - Viva question and answer preparation
sounds/                - Auto-generated WAV sound effects after running


Notes
-----

The project uses OpenGL with GLUT/FreeGLUT only.
No game engine and no external 3D models are used.
All objects are made using primitives and procedural visuals.
