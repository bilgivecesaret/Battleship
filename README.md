# Project – Battleship Game

Battleship game implements a simplified using inter-process communication (IPC). The program creates a new process, where both the parent and child processes generate a 4x4 grid. Each process randomly places 4 ships (a point in 1x1) on their grid, ensuring different locations for each game run, and display the initial ship locations. The game proceeds in a turn-based manner where each process randomly selects a point on the opponent's grid, aiming to hit their ships. If a process hits all of the opponent’s ships, that player will win the game, print a congratulatory message, and the game will end.

## Getting Started

This is an application implemented in the Unix/Linux terminal and aimed at understanding the processes in operating systems.

The following steps walk through getting the application running. 

1. [Clone the project](#1-clone-the-project)
2. [Start the app](#2-start-the-app)

### 1. Clone the project

  [Clone the project](https://github.com/bilgivecesaret/Battleship.git), e.g. `gh repo clone bilgivecesaret/Battleship`

### 2. Start the app

Open the terminal in the location of the "<code><b>Battleship_Game.c</b></code>" file in the Linux operating system.

Type the command "<code><b>gcc Battleship_Game.c -o battleship -lrt</b></code>" into the terminal.

Then type the command "<code><b>./battleship</b></code>" to run the application.
