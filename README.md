# 2D OpenGL Bus Simulation

A dynamic 2D C++/OpenGL application featuring a bus navigating a circular route with an integrated "Controller" event system.



## Project Overview
This simulation depicts a bus moving along a fixed circular path, stopping at various stations. The project demonstrates coordinate transformations, state-based logic, and randomized event handling in a graphical environment.

### The "Control" Mechanic
The simulation includes a unique logic flow:
1. **Control Entrance:** When a controller enters the bus, a random number of passengers are "thrown out" (removed from the count).
2. **Delayed Exit:** The controller stays on the bus for the duration of the journey to the next stop.
3. **Station Arrival:** Upon reaching the next station, the controller exits, and the bus continues its loop.

---

## Features
* **Circular Motion:** Utilizes trigonometric functions to calculate smooth movement along a radial path.
* **State Management:** Tracks bus states (Moving, Stopped, Under Inspection).
* **Randomized Events:** Uses `rand()` logic to determine passenger turnover during a control event.
* **Station Coordination:** Fixed points along the circle act as interactive triggers for the bus logic.

---

## Getting Started

### Prerequisites
To run this project, you need the following libraries installed:
* **C++ Compiler** (GCC/MinGW or MSVC)
* **OpenGL**
* **GLUT/FreeGLUT**

### Compilation
If you are using `g++`, you can compile the project using:

```bash
g++ main.cpp -o bus_simulation -lfreeglut -lglew32 -lopengl32
