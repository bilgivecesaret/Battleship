#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/wait.h>

#define GRID_SIZE 4
#define SHIPS 4
#define TILE 'O'
#define HIT 'X'
#define SHIP 'S'
#define MISS 'M'

// Structure for the battlefield
typedef struct {
    char parentGrid[GRID_SIZE][GRID_SIZE];   // Parent's grid
    char childGrid[GRID_SIZE][GRID_SIZE];    // Child's grid
    int turn;                         // To track whose turn it is
    int remainingShipsPar;                   // Number of parent's ships left
    int remainingShipsChild;                    // Number of child's ships left
    //int consumedAmmoParent;
    //int consumedAmmoChild;
} BattleFieldInfo;

void intializeMap(char grid[GRID_SIZE][GRID_SIZE]) {//initializes the map by placing 'O' on each tile
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = TILE;
        }
    }
}

void placeRandomShips(char grid[GRID_SIZE][GRID_SIZE]) {
    int ship=0;
    while(ship<SHIPS){
        int x=rand()%GRID_SIZE;
        int y=rand()%GRID_SIZE;
        if(grid[x][y]==TILE){
            grid[x][y]=SHIP;
            ship++;
        }
    }
}
void displayMap(char grid[GRID_SIZE][GRID_SIZE],char* playerName){
    printf("\n%s's field:\n",playerName);
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
                printf("%c ", grid[i][j]);
        }
        printf("\n");
    }    
    printf("\n");
}
void randomMovement(BattleFieldInfo* state,int player) {
    int x = rand() % GRID_SIZE;
    int y = rand() % GRID_SIZE;
    char* hitMis;
    if (player == 0) { // Parent's turn
        if (state->childGrid[x][y] == SHIP) {
            state->childGrid[x][y] = HIT;
            state->remainingShipsChild--;
            hitMis="hits";
        } else {
            state->childGrid[x][y]=state->childGrid[x][y]==HIT?HIT:MISS;// if selected tile is already hitted, doesn't change, otherwise change
            hitMis="misses";
        }

        //******************** */
        printf("\nParent %s at (%d, %d)!\n",hitMis, x, y);
        printf("Grids after Parent's move:");
        displayMap(state->childGrid, "Child");
        //******************** */

    } else { // Child's turn
        if (state->parentGrid[x][y] == SHIP) {
            state->parentGrid[x][y] = HIT;
            state->remainingShipsPar--;
            hitMis="hits";
        }
        else {
            state->parentGrid[x][y]=state->parentGrid[x][y]==HIT?HIT:MISS;// if selected tile is already hitted, doesn't change, otherwise change
            hitMis="misses";
        }

        //******************** */
        printf("\nChild %s at (%d, %d)!\n",hitMis, x, y);
        printf("Grids after Child's move:");
        displayMap(state->parentGrid, "Parent");
        //******************** */
    }
    printf("-------------------");
}

int main() {
    srand(time(NULL));

    const char *shm_name = "Battlefield"; //name of the shared memory object
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    // Truncate shared memory to the size of battlefild
    ftruncate(shm_fd, sizeof(BattleFieldInfo));

    BattleFieldInfo *state = mmap(0, sizeof(BattleFieldInfo), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    //Initializes maps
    intializeMap(state->parentGrid);
    intializeMap(state->childGrid);

    //place ships for both players
    placeRandomShips(state->parentGrid);
    srand(time(NULL)*13);//to assure unique seed
    placeRandomShips(state->childGrid);

    state->turn = rand()%2;//random player starts
    state->remainingShipsPar = SHIPS;
    state->remainingShipsChild = SHIPS;

    // Display initial grids
    printf("Initial grids:\n");
    displayMap(state->parentGrid, "Parent");
    displayMap(state->childGrid, "Child");

    pid_t pid = fork();
    
    if (pid == -1) {//failed to fork
        exit(1);
    } else if (pid == 0) { // Child process
        while (state->remainingShipsPar > 0 && state->remainingShipsChild > 0) {
            if (!state->turn) {
                randomMovement(state, 1);
                state->turn = 1;
            }
            sleep(1);
        }
        if (state->remainingShipsPar == 0) {
            printf("Child wins!\n");
        } 
        exit(0); //exit child process
    } else { // Parent process
        while (state->remainingShipsPar > 0 && state->remainingShipsChild > 0) {
            if (state->turn) {
                randomMovement(state, 0);
                state->turn = 0;

            }
            sleep(1);
        }
        // Wait for child process to end
        wait(NULL);
        if (state->remainingShipsChild == 0) {
            printf("Parent wins!\n");
        }
    }

    // Cleanup the shared memory(battlefield)
    munmap(state, sizeof(BattleFieldInfo));
    shm_unlink(shm_name);

    return 0;
}