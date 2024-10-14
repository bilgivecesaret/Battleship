#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>

#define GRID_SIZE 4
#define SHIPS 4
#define TILE '.'
#define SHIP 'S'
#define HIT 'X'
#define MISS 'O'

#define READ_END	0
#define WRITE_END	1

// Function to generate a grid with randomly placed ships
void generate_grid(char grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = TILE;
        }
    }

    int ships_placed = 0;
    while (ships_placed < SHIPS) {
        int x = rand() % GRID_SIZE;
        int y = rand() % GRID_SIZE;
        if (grid[x][y] == TILE) {
            grid[x][y] = SHIP;  // Place ship
            ships_placed++;
        }
    }
}

// Function to print the grid 
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
            if (grid[i][j] == SHIP) {
                return false;
            }
        }
    }
    return true;
}

// Function to process a hit or miss
void process_guess(char grid[GRID_SIZE][GRID_SIZE], int x, int y) {
    if (grid[x][y] == SHIP) {
        grid[x][y] = HIT;  // Mark as hit
    }
    else if(grid[x][y] == HIT) {
        grid[x][y] = HIT; 
    }else{
    	grid[x][y] = MISS; 
    }    
}

int main() {

    char parent_grid[GRID_SIZE][GRID_SIZE];
    char child_grid[GRID_SIZE][GRID_SIZE];
    
    srand(time(NULL));//uses current time to create a seed for random generator

    int parent_to_child[2];  // Pipe: parent -> child
    int child_to_parent[2];  // Pipe: child -> parent

    // Create pipes
    pipe(parent_to_child);
    pipe(child_to_parent);
    
    //Initial information message
    printf("\n>>>>>>>>>> .:Tile, S:Ship, X:Hit, O: Miss <<<<<<<<<<\n\n");
    pid_t pid = fork(); 

    if(pid == -1){//handles fail case if it occurs, returns with fail case('1')
        perror("Failed to create child process ");
        return 1;
    }    
    
    if (pid == 0) {  // Child process
        close(parent_to_child[WRITE_END]);  // Close write-end of parent-to-child pipe 
        close(child_to_parent[READ_END]);  // Close read-end of child-to-parent pipe 
       
        srand(time(NULL)*13);//unique seed to avoid same grid
        generate_grid(child_grid);
        printf("Child's initial grid:\n");
        print_grid(child_grid);

        while (1) {
            int x, y;
            
            // Read parent's attack coordinates
            read(parent_to_child[READ_END], &x, sizeof(int));
            read(parent_to_child[READ_END], &y, sizeof(int));

            // Process the attack
            process_guess(child_grid, x, y); 
            write(child_to_parent[WRITE_END], child_grid, sizeof(child_grid));                       

            // Check if all ships are sunk
            if (all_ships_sunk(child_grid)) {
                printf("Child: Parent wins! All my ships have been sunk.\n\n");
                printf("Parent's final grid:\n");
        	    print_grid(parent_grid);
         	    printf("Child's final grid:\n");
        	    print_grid(child_grid);
                kill(getppid(), SIGTERM);  // Terminate the parent
                return 0;
            }

            // Child's turn to attack
            choose_random_point(&x, &y);
            write(child_to_parent[WRITE_END], &x, sizeof(int));
            write(child_to_parent[WRITE_END], &y, sizeof(int));

            // Get parent's result
            read(parent_to_child[READ_END], parent_grid, sizeof(parent_grid));
            if (all_ships_sunk(parent_grid)) {
                printf("Child: I win! I sank all of the parent's ships.\n\n");
                printf("Parent's final grid:\n");
        	    print_grid(parent_grid);
         	    printf("Child's final grid:\n");
        	    print_grid(child_grid);
                kill(getppid(), SIGTERM);
                return 0;
            }
        }

    } else {  // Parent process
        close(parent_to_child[READ_END]);  // Close read-end of parent-to-child pipe 
        close(child_to_parent[WRITE_END]);  // Close write-end of child-to-parent pipe 

        generate_grid(parent_grid);
        printf("Parent's initial grid:\n");
        print_grid(parent_grid);

        while (1) {
            int x, y;

            // Parent's turn to attack
            choose_random_point(&x, &y);
            write(parent_to_child[WRITE_END], &x, sizeof(int));
            write(parent_to_child[WRITE_END], &y, sizeof(int));            
            
            // Get child's result
            read(child_to_parent[READ_END], child_grid, sizeof(child_grid));      
            
            if (all_ships_sunk(child_grid)) {
                printf("Parent: I win! I sank all of the child's ships.\n\n");
                printf("Parent's final grid:\n");
        	    print_grid(parent_grid);
         	    printf("Child's final grid:\n");
        	    print_grid(child_grid);
                kill(pid, SIGTERM);
                return 0;
            }            

            // Receive attack coordinates from child
            read(child_to_parent[READ_END], &x, sizeof(int));
            read(child_to_parent[READ_END], &y, sizeof(int));
            
            // Process the attack
            process_guess(parent_grid, x, y); 
            write(parent_to_child[WRITE_END], parent_grid, sizeof(parent_grid)); 

 	        if (all_ships_sunk(parent_grid)) {
                printf("Parent: Child wins! All my ships have been sunk.\n\n");
                printf("Parent's final grid:\n");
        	    print_grid(parent_grid);
         	    printf("Child's final grid:\n");
        	    print_grid(child_grid);
                kill(pid, SIGTERM);
                return 0;
            }            
        }
    }
    return 0;
}
