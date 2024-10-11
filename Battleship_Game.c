#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>

#define GRID_SIZE 4
#define SHIPS 4

// Function to generate a grid with randomly placed ships
void generate_grid(char grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = '.';
        }
    }

    int ships_placed = 0;
    while (ships_placed < SHIPS) {
        int x = rand() % GRID_SIZE;
        int y = rand() % GRID_SIZE;
        if (grid[x][y] == '.') {
            grid[x][y] = 'S';  // Place ship
            ships_placed++;
        }
    }
}

// Function to print the grid (for debugging purposes)
void print_grid(char grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            printf("%c ", grid[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

// Function to randomly choose a point to attack
void choose_random_point(int *x, int *y) {
    *x = rand() % GRID_SIZE;
    *y = rand() % GRID_SIZE;
}

// Function to check if all ships have been hit
bool all_ships_sunk(char grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            if (grid[i][j] == 'S') {
                return false;
            }
        }
    }
    return true;
}

// Function to process a hit or miss
bool process_guess(char grid[GRID_SIZE][GRID_SIZE], int x, int y) {
    if (grid[x][y] == 'S') {
        grid[x][y] = 'X';  // Mark as hit
        return true;
    }
    return false;
}

int main() {
    srand(time(NULL));

    int parent_to_child[2];  // Pipe: parent -> child
    int child_to_parent[2];  // Pipe: child -> parent

    // Create pipes
    pipe(parent_to_child);
    pipe(child_to_parent);

    pid_t pid = fork();

    if (pid == 0) {  // Child process
        close(parent_to_child[1]);  // Close write-end of parent-to-child pipe
        close(child_to_parent[0]);  // Close read-end of child-to-parent pipe

        char child_grid[GRID_SIZE][GRID_SIZE];
        generate_grid(child_grid);
        printf("Child's initial grid:\n");
        print_grid(child_grid);

        while (1) {
            int x, y;
            // Read parent's attack coordinates
            read(parent_to_child[0], &x, sizeof(int));
            read(parent_to_child[0], &y, sizeof(int));

            // Process the attack and send result to parent
            bool hit = process_guess(child_grid, x, y);
            write(child_to_parent[1], &hit, sizeof(bool));

            // Check if all ships are sunk
            if (all_ships_sunk(child_grid)) {
                printf("Child: Parent wins! All my ships have been sunk.\n");
                kill(getppid(), SIGTERM);  // Terminate the parent
                exit(0);
            }

            // Child's turn to attack
            choose_random_point(&x, &y);
            write(child_to_parent[1], &x, sizeof(int));
            write(child_to_parent[1], &y, sizeof(int));

            // Get parent's result
            read(parent_to_child[0], &hit, sizeof(bool));
            if (all_ships_sunk(child_grid)) {
                printf("Child: I win! I sank all of the parent's ships.\n");
                kill(getppid(), SIGTERM);
                exit(0);
            }
        }

    } else {  // Parent process
        close(parent_to_child[0]);  // Close read-end of parent-to-child pipe
        close(child_to_parent[1]);  // Close write-end of child-to-parent pipe

        char parent_grid[GRID_SIZE][GRID_SIZE];
        generate_grid(parent_grid);
        printf("Parent's initial grid:\n");
        print_grid(parent_grid);

        while (1) {
            int x, y;

            // Parent's turn to attack
            choose_random_point(&x, &y);
            write(parent_to_child[1], &x, sizeof(int));
            write(parent_to_child[1], &y, sizeof(int));

            // Get child's result
            bool hit;
            read(child_to_parent[0], &hit, sizeof(bool));
            if (hit) {
                process_guess(parent_grid, x, y);
            }

            if (all_ships_sunk(parent_grid)) {
                printf("Parent: Child wins! All my ships have been sunk.\n");
                kill(pid, SIGTERM);
                exit(0);
            }

            // Receive attack coordinates from child
            read(child_to_parent[0], &x, sizeof(int));
            read(child_to_parent[0], &y, sizeof(int));

            // Process the child's attack and send result
            hit = process_guess(parent_grid, x, y);
            write(parent_to_child[1], &hit, sizeof(bool));

            if (all_ships_sunk(parent_grid)) {
                printf("Parent: I win! I sank all of the child's ships.\n");
                kill(pid, SIGTERM);
                exit(0);
            }
        }
    }

    return 0;
}
