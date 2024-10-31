#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/wait.h>
#include <string.h>

#define ROW_SIZE 8
#define SHIPS 5
#define TILE '.'
#define HIT 'X'
#define MISS 'O'
#define DESTROYER 'D'
#define BATTLESHIP 'B'
#define CRUISER 'C'

typedef struct
{
    int size;
    char sign;
} Ship;

Ship ships[SHIPS] = {
    {4, BATTLESHIP},
    {3, CRUISER},
    {3, CRUISER},
    {2, DESTROYER},
    {2, DESTROYER},
};

typedef struct
{
    char parentGrid[ROW_SIZE][ROW_SIZE]; // Parent's grid
    char childGrid[ROW_SIZE][ROW_SIZE];  // Child's grid
    int turn;                            // To track whose turn it is
    Ship parShips[SHIPS];
    Ship childShips[SHIPS];
} BattleFieldInfo;

int isPlaceAvailable(int x, int y, char grid[ROW_SIZE][ROW_SIZE], Ship ship, int dx, int dy);
void locateShips(char grid[ROW_SIZE][ROW_SIZE]);
int *getAdjacentPoint(char grid[ROW_SIZE][ROW_SIZE], int y, int x);
int aiMove(BattleFieldInfo *state);
void changePos(int *arr, int y, int x);
int isInsideGrid(int y, int x);
int isShip(char sign);
Ship *getShip(BattleFieldInfo *state, char sign);
void initializeMap(char grid[ROW_SIZE][ROW_SIZE]);
void displayMap(BattleFieldInfo *state, int turn);
int isGameOver(BattleFieldInfo *state);

int isPlaceAvailable(int x, int y, char grid[ROW_SIZE][ROW_SIZE], Ship ship, int dx, int dy)
{
    for (int i = 0; i < ship.size; i++)
    {
        int nx = x + i * dx;
        int ny = y + i * dy;

        if (nx < 0 || nx >= ROW_SIZE || ny < 0 || ny >= ROW_SIZE || grid[ny][nx] != TILE)
            return 0;

        for (int row = (ny - 1 < 0 ? 0 : ny - 1); row <= (ny + 1 >= ROW_SIZE ? ROW_SIZE - 1 : ny + 1); row++)
        {
            for (int col = (nx - 1 < 0 ? 0 : nx - 1); col <= (nx + 1 >= ROW_SIZE ? ROW_SIZE - 1 : nx + 1); col++)
            {
                if (grid[row][col] == BATTLESHIP || grid[row][col] == CRUISER || grid[row][col] == DESTROYER)
                    return 0;
            }
        }
    }
    return 1;
}

void locateShips(char grid[ROW_SIZE][ROW_SIZE])
{
    srand(time(NULL) * rand() * 13);
    int index = 0;

    while (index < SHIPS)
    {
        int x = rand() % ROW_SIZE;
        int y = rand() % ROW_SIZE;
        int dx = rand() % 2, dy = 1 - dx;

        if (isPlaceAvailable(x, y, grid, ships[index], dx, dy))
        {
            for (int i = 0; i < ships[index].size; i++)
            {
                grid[y + i * dy][x + i * dx] = ships[index].sign;
            }
            index++;
        }
    }
}
int *getAdjacentPoint(char grid[ROW_SIZE][ROW_SIZE], int y, int x)
{
    static int point[2];
    point[0] = y;
    point[1] = x;
    if (y + 1 < ROW_SIZE && !(grid[y + 1][x] == MISS || grid[y + 1][x] == HIT))
        point[0] = y + 1;
    else if (y - 1 >= 0 && !(grid[y - 1][x] == MISS || grid[y - 1][x] == HIT))
        point[0] = y - 1;
    else if (x + 1 < ROW_SIZE && !(grid[y][x + 1] == MISS || grid[y][x + 1] == HIT))
        point[1] = x + 1;
    else if (x - 1 >= 0 && !(grid[y][x - 1] == MISS || grid[y][x - 1] == HIT))
        point[1] = x - 1;

    return point;
}
/*
int *getAdjacentPoint(char grid[ROW_SIZE][ROW_SIZE], int y, int x) {
    char up = (y - 1 >= 0) ? grid[y - 1][x] : '\0';
    char down = (y + 1 < ROW_SIZE) ? grid[y + 1][x] : '\0';
    char right = (x + 1 < ROW_SIZE) ? grid[y][x + 1] : '\0';
    char left = (x - 1 >= 0) ? grid[y][x - 1] : '\0';

    int ny = !(down == MISS || down == HIT) ? 1 : (!(up == MISS || up == HIT)  ? -1 : 0);
    int nx = ny==0 ?0:(!(right == MISS || right == HIT) ? 1 : (!(left == MISS || left == HIT) ? -1 : 0));

    static int point[2];
    point[0] = y + ny;
    point[1] = x + nx;

    return point;
}*/

int aiMove(BattleFieldInfo *state)
{
    char *pName = state->turn ? "child" : "parent";
    char(*grid)[ROW_SIZE] = (state->turn == 1) ? state->parentGrid : state->childGrid;

    static int lastHit[2] = {-1, -1};
    static int hitDirect[2] = {0, 0}; // up-down, left-right
    static int combo = 0;
    static int missCombo = 0;
    if (combo > 1 && missCombo < 2 && (hitDirect[0] != 0 || hitDirect[1] != 0))
    {   
        if(!isInsideGrid(lastHit[0] + hitDirect[0],lastHit[1] + hitDirect[1])){
            changePos(hitDirect, hitDirect[0] * -combo, hitDirect[1] * -combo);
        }
        int aiEst[2] = {lastHit[0] + hitDirect[0], lastHit[1] + hitDirect[1]};
        if (isInsideGrid(aiEst[0], aiEst[1]))
        {
            char *pointSign = &grid[aiEst[0]][aiEst[1]];
            if (isShip(*pointSign))
            {
                Ship *ship = getShip(state, *pointSign);
                ship->size = ship->size - 1;
                *pointSign = HIT;
                combo += 1;
                changePos(hitDirect, (aiEst[0] - lastHit[0]) % 2, (aiEst[1] - lastHit[1]) % 2);
                changePos(lastHit, aiEst[0], aiEst[1]);
                printf("%s directed ai hit  %d,%d \n", pName, aiEst[0], aiEst[1]);
                displayMap(state, state->turn);
                return 1;
            }
            else if (*pointSign == TILE)
            {
                *pointSign = MISS;
                missCombo += 1;
                printf("%s directed ai miss  %d,%d\n", pName, aiEst[0], aiEst[1]);
                displayMap(state, state->turn);
                if (missCombo >= 2)
                {
                    combo = 0;
                    missCombo = 0;
                    lastHit[0] = -1;
                }
                else
                {
                    changePos(hitDirect, hitDirect[0] * -combo, hitDirect[1] * -combo);
                }
                return 1;
            }
        }
        missCombo += 1;
    }
    else if (lastHit[0] != -1)
    {
        int *aiEst = getAdjacentPoint(grid, lastHit[0], lastHit[1]);
        if (aiEst[0] != lastHit[0] || aiEst[1] != lastHit[1])
        {
            char *pointSign = &grid[aiEst[0]][aiEst[1]];
            if (isShip(*pointSign))
            {
                Ship *ship = getShip(state, *pointSign);
                ship->size = ship->size - 1;
                *pointSign = HIT;
                combo += 1;
                changePos(hitDirect, aiEst[0] - lastHit[0], aiEst[1] - lastHit[1]);
                changePos(lastHit, aiEst[0], aiEst[1]);
                printf("%s, ai estimated hit %d,%d\n", pName, aiEst[0], aiEst[1]);
                displayMap(state, state->turn);
            }
            else if (*pointSign == TILE)
            {
                *pointSign = MISS;
                printf("%s, ai estimated miss %d,%d\n", pName, aiEst[0], aiEst[1]);
                displayMap(state, state->turn);
            }
            return 1;
        }
        lastHit[0] = -1;
    }

    combo = 0;
    missCombo = 0;
    while (1)
    {

        int x = rand() % ROW_SIZE;
        int y = rand() % ROW_SIZE;
        char *pointSign = &grid[y][x];
        if (isShip(*pointSign))
        {
            Ship *ship = getShip(state, *pointSign);
            ship->size = ship->size - 1;
            *pointSign = HIT;
            combo += 1;
            printf("%s random hit %d,%d\n", pName, y, x);
            displayMap(state, state->turn);
            changePos(lastHit, y, x);
            return 1;
        }
        else if (*pointSign == TILE)
        {
            *pointSign = MISS;
            printf("%s random miss %d,%d\n", pName, y, x);
            displayMap(state, state->turn);
            return 1;
        }
    }
}

void changePos(int *arr, int y, int x)
{
    arr[0] = y;
    arr[1] = x;
}

int isInsideGrid(int y, int x)
{
    return x >= 0 && x < ROW_SIZE && y >= 0 && y < ROW_SIZE;
}

int isShip(char sign)
{
    return sign == BATTLESHIP || sign == DESTROYER || sign == CRUISER;
}

Ship *getShip(BattleFieldInfo *state, char sign)
{
    Ship *s = state->turn == 1 ? state->parShips : state->childShips;
    for (int i = 0; i < SHIPS; i++)
    {
        if (sign == s[i].sign && s[i].size > 0)
            return &s[i];
    }

    return NULL;
}

void initializeMap(char grid[ROW_SIZE][ROW_SIZE])
{
    for (int i = 0; i < ROW_SIZE; i++)
        for (int j = 0; j < ROW_SIZE; j++)
            grid[i][j] = TILE;
}

void displayMap(BattleFieldInfo *state, int turn)
{
    char(*grid)[ROW_SIZE] = (turn == 1) ? state->parentGrid : state->childGrid;
    printf("Player %s's Grid:\n", !turn ? "child" : "parent");
    for (int i = 0; i < ROW_SIZE; i++)
    {
        for (int j = 0; j < ROW_SIZE; j++)
        {
            printf("%c ", grid[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

int isGameOver(BattleFieldInfo *state)
{
    int parentShipsAlive = 0;
    int childShipsAlive = 0;

    for (int i = 0; i < SHIPS; i++)
    {
        if (state->parShips[i].size > 0)
        {
            parentShipsAlive = 1;
            break;
        }
    }

    for (int i = 0; i < SHIPS; i++)
    {
        if (state->childShips[i].size > 0)
        {
            childShipsAlive = 1;
            break;
        }
    }

    return !(parentShipsAlive && childShipsAlive);
}

int main()
{
    srand(time(NULL) * 13);

    const char *shm_name = "Battlefield"; // name of the shared memory object
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(BattleFieldInfo));

    BattleFieldInfo *state = mmap(0, sizeof(BattleFieldInfo), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // Initializes maps
    initializeMap(state->parentGrid);
    initializeMap(state->childGrid);
    locateShips(state->parentGrid);
    locateShips(state->childGrid);
    memcpy(state->parShips, ships, sizeof(ships));
    memcpy(state->childShips, ships, sizeof(ships));
    displayMap(state, state->turn);
    displayMap(state, !state->turn);
    state->turn = rand() % 2;
    pid_t pid = fork(); // creates child process
    if (pid == -1)
    { // failed to fork
        exit(1);
    }
    srand(time(NULL) * getpid());
    if (pid == 0)
    {

        while (!isGameOver(state))
        {
            if (state->turn && aiMove(state))
            {
                state->turn = !state->turn;
            }
        }
        exit(0); // exit child process
    }
    else
    {

        while (!isGameOver(state))
        {
            if (!state->turn && aiMove(state))
            {
                state->turn = !state->turn;
            }
        }
        wait(NULL);
    }

    // Cleanup the shared memory region(battlefield)
    munmap(state, sizeof(BattleFieldInfo));
    shm_unlink(shm_name);

    return 0;
}
