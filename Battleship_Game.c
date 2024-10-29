#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/wait.h>
#include <string.h>
#include <ncurses.h>

#define GRID_SIZE 8
#define NUM_SHIPS 5
#define TILE '.'
#define HIT 'X'
#define BATTLESHIP 'B'
#define CRUISER 'C'
#define DESTROYER 'D'
#define MISS 'O'


typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    char *name;
    int size;
    Position positions[4]; // Maximum ship size 4 cells
} Ship;

// Ship diversity
Ship ships[] = {
    {"Battleship", 4, {{0,0}}},
    {"Cruiser1", 3, {{0,0}}},
    {"Cruiser2", 3, {{0,0}}},
    {"Destroyer1", 2, {{0,0}}},
    {"Destroyer2", 2, {{0,0}}}
};

// Structure for the battlefield
typedef struct {
    char parentGrid[GRID_SIZE][GRID_SIZE];   // Parent's grid
    char childGrid[GRID_SIZE][GRID_SIZE];    // Child's grid
    int turn;                         // To track whose turn it is
    Position lastHitParent;
    Position lastHitChild;
    int lastDirectionParent;
    int lastDirectionChild;
} BattleFieldInfo;

void initializeMap(char grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < GRID_SIZE; i++) {
        for (int j = 0; j < GRID_SIZE; j++) {
            grid[i][j] = TILE;
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

// Gap control
int isValidPlacement(char grid[GRID_SIZE][GRID_SIZE], Ship ship, int x, int y, int direction) {
    for (int i = 0; i < ship.size; i++) {
        int nx = direction == 0 ? x + i : x;
        int ny = direction == 1 ? y + i : y;

        if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE || grid[nx][ny] != TILE) {
            return 0;
        }

        if((nx > 0 && grid[nx-1][ny] != TILE) || (nx < GRID_SIZE-1 && grid[nx+1][ny] != TILE) ||
           (ny > 0 && grid[nx][ny-1] != TILE) || (ny < GRID_SIZE-1 && grid[nx][ny+1] != TILE)) {
            return 0;
        }
    }
    return 1;
}

// Positioning the ship
void placeShip(char grid[GRID_SIZE][GRID_SIZE], Ship *ship) {
    int placed = 0;
    while (!placed) {
        int x = rand() % GRID_SIZE;
        int y = rand() % GRID_SIZE;
        int direction = rand() % 2; // 0: Horizantel, 1: Vertical

        if (isValidPlacement(grid, *ship, x, y, direction)) {
            for (int i = 0; i < ship->size; i++) {
                int nx = direction == 0 ? x + i : x;
                int ny = direction == 1 ? y + i : y;
                if (strcmp(ship->name, "Battleship") == 0)
                    grid[nx][ny] = BATTLESHIP;
                else if (strcmp(ship->name, "Cruiser1") == 0 || strcmp(ship->name, "Cruiser2") == 0)
                    grid[nx][ny] = CRUISER;
                else
                    grid[nx][ny] = DESTROYER;
                ship->positions[i].x = nx;
                ship->positions[i].y = ny;
            }
            placed = 1;
        }
    }
}

// Automatic Ship Docking (Artificial Intelligence)
void autoPlaceShips(char grid[GRID_SIZE][GRID_SIZE]) {
    for (int i = 0; i < NUM_SHIPS; i++) {
        placeShip(grid, &ships[i]);
    }
}

// When AI hits a ship, it attacks surrounding cells
void aiAttack(char grid[GRID_SIZE][GRID_SIZE], Position *lastHit, int *hitDirection, int player) {
    int x, y;
    if (lastHit->x != -1 && lastHit->y != -1) {
        x = lastHit->x + (*hitDirection == 0 ? 1 : 0);
        y = lastHit->y + (*hitDirection == 1 ? 1 : 0);
        if (x >= GRID_SIZE || y >= GRID_SIZE || grid[x][y] == MISS || grid[x][y] == HIT || grid[x][y] == TILE) {
            *hitDirection = (*hitDirection + 1) % 2;
            x = rand() % GRID_SIZE;
            y = rand() % GRID_SIZE;
        }
    }else{
        *hitDirection = (*hitDirection + 1) % 2;
        x = rand() % GRID_SIZE;
        y = rand() % GRID_SIZE;
    }

    if (grid[x][y] == BATTLESHIP || grid[x][y] == CRUISER || grid[x][y] == DESTROYER) {
        printf("AI hit at (%d,%d) \n", x, y);
        lastHit->x = x;
        lastHit->y = y;
        grid[x][y] = HIT;
    } else {
        grid[x][y] = MISS;
    }
}

void randomMovement(BattleFieldInfo* state, int player) {
    if (player == 0) { // Parent's turn
        aiAttack(state->parentGrid, &state->lastHitParent, &state->lastDirectionParent, player);
    } else { // Child's turn
        aiAttack(state->childGrid, &state->lastHitChild, &state->lastDirectionChild, player);
    }
}

bool areShipsRemaining(char grid[GRID_SIZE][GRID_SIZE]) {
    for (int x = 0; x < GRID_SIZE; x++) {
        for (int y = 0; y < GRID_SIZE; y++) {
            if (grid[x][y] == BATTLESHIP || grid[x][y] == CRUISER || grid[x][y] == DESTROYER) {
                return true;
            }
        }
    }
    return false;
}

int main() {
    srand(time(NULL));

    const char *shm_name = "Battlefield"; //name of the shared memory object
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    // Truncate shared memory to the size of battlefild
    if (ftruncate(shm_fd, sizeof(BattleFieldInfo)) == -1) {
        perror("ftruncate failed");
        close(shm_fd);
        shm_unlink(shm_name);
        exit(1);
    }

    BattleFieldInfo *state = mmap(0, sizeof(BattleFieldInfo), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (state == MAP_FAILED) {
        perror("mmap failed");
        close(shm_fd);
        shm_unlink(shm_name);
        exit(1);
    }

    //Initializes maps
    initializeMap(state->parentGrid);
    initializeMap(state->childGrid);

    //place ships for both players
    autoPlaceShips(state->parentGrid);
    srand(time(NULL)*13);//to assure unique seed
    autoPlaceShips(state->childGrid);

    state->turn = rand()%2; //selects randomly a player to start: turn=0 for child/ turn=1 for parent

    // Display initial grids
    printf("#####\t Initial grids\t ##### \n");
    printf("\"B:Battleship, C:Cruiser, D:Destroyer,\n");
    printf("X:Hit, O:Miss, .:Empty tile \" \n");

    displayMap(state->parentGrid, "Parent");
    displayMap(state->childGrid, "Child");

    pid_t pid = fork();//creates child process

    if (pid == -1) {//failed to fork
        perror("Failed to open shared memory");
        shm_unlink(shm_name);
        exit(1);
    }
    else if (pid == 0) { // Child process
        while (areShipsRemaining(state->parentGrid) && areShipsRemaining(state->childGrid)) {
            if (!state->turn) {
                randomMovement(state, 0);
                state->turn = !state->turn;
            }
        }

        if (areShipsRemaining(state->parentGrid) == 0) {
            printf("--------------------------\nChild wins! Final Grids:\n");
            displayMap(state->parentGrid, "Parent");
            displayMap(state->childGrid, "Child");
        }

        exit(0); //exit child process

    } else { // Parent process
        while (areShipsRemaining(state->parentGrid) && areShipsRemaining(state->childGrid)) {
            if (state->turn) {
                randomMovement(state, 1);
                state->turn = !state->turn;
            }
        }
        if (areShipsRemaining(state->childGrid) == 0) {
            printf("--------------------------\nParent wins! Final Grids:\n");
            displayMap(state->parentGrid, "Parent");
            displayMap(state->childGrid, "Child");
        }
        // Wait for child process to end
        wait(NULL);
    }

    // Cleanup the shared memory region(battlefield)
    munmap(state, sizeof(BattleFieldInfo));
    shm_unlink(shm_name);

    return 0;
}
