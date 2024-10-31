# Project – Battleship Game

Battleship game implements a simplified using inter-process communication (IPC). The program creates a new process, where both the parent and child processes generate a 8x8 grid. Each process randomly places 5 ships (One "Battleship" of 4x1 size, Two "Cruisers" of 3x1 size, Two "Destroyers" of 2x1 size) on their grid, ensuring different locations for each game run, and display the initial ship locations. The game proceeds in a turn-based manner where each process randomly selects a point on the opponent's grid, aiming to hit their ships. Each ship is placed horizontally or vertically on the grid, and no ship overlaps. Additionally, there is a 1-cell gap between any two ships. There is an automatic ship placement algorithm that randomly places ships according to the rules above. A basic AI is used that uses a strategy to target adjacent cells after each hit to increase the chance of hitting the opponent's ships. The game state is saved periodically. If the game is interrupted, the game continues where it left off. The ncurses library is used to visualize the grids, movements, and hits/misses as the game progresses. If a process hits all of the opponent’s ships, that player will win the game, print a congratulatory message, and the game will end.

## Getting Started

This is an application implemented in the Unix/Linux terminal and aimed at understanding the processes in operating systems.

The following steps walk through getting the application running. 

1. [Clone the project](#1-clone-the-project)
2. [Start the app](#2-start-the-app)

### 1. Clone the project

  [Clone the project](https://github.com/bilgivecesaret/Battleship.git), e.g. `gh repo clone bilgivecesaret/Battleship`

### 2. Start the app

Open the terminal in the location of the "<code><b>Battleship_Game_SHM.c</b></code>" file in the Linux operating system.

Type the command "<code><b>gcc Battleship_Game_SHM.c -o battleship -lrt</b></code>" into the terminal.

Then type the command "<code><b>./battleship</b></code>" to run the application.
