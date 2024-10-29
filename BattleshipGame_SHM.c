#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/wait.h>

#define GRID_SIZE 8
#define SHIPS 5
#define TILE '.'
#define HIT 'X'
#define SHIP 'S'
#define MISS 'O'

// Structure for the battlefield
typedef struct
{
    char parentGrid[GRID_SIZE][GRID_SIZE]; // Parent's grid
    char childGrid[GRID_SIZE][GRID_SIZE];  // Child's grid
    int turn;                              // To track whose turn it is
    int remainingShipsHealthPar;                 // Number of parent's ships left
    int remainingShipsHealthChild;               // Number of child's ships left
} BattleFieldInfo;

typedef struct
{
    int size;
    int dx;
    int dy;

} Ship;

void initializeMap(char grid[GRID_SIZE][GRID_SIZE])
{ // initializes the map by placing 'O' on each tile
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            grid[i][j] = TILE;
        }
    }
}

int isPlaceAvailable(int x, int y, char grid[GRID_SIZE][GRID_SIZE], Ship ship, int dx, int dy)
{
    for (int i = 0; i < ship.size; i++)
    {
        int nx = x + i * dx;
        int ny = y + i * dy;

        if (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE || grid[ny][nx] != TILE)
        {
            return 0;
        }
        //
        for (int row = ny - 1; row <= ny + 1; row++)
        { // checks row by row
            for (int col = nx - 1; col <= nx + 1; col++)
            { // checks column by column
                if (row >= 0 && row < GRID_SIZE && col >= 0 && col < GRID_SIZE)
                {
                    if (grid[row][col] == SHIP)
                        return 0;
                }
            }
        }
    }
    return 1;
}

void placeRandomShips(char grid[GRID_SIZE][GRID_SIZE], Ship ships[])
{
    srand(time(NULL) * getpid() * 13);
    int index = 0;
    while (index < SHIPS)
    {
        int x = rand() % GRID_SIZE;
        int y = rand() % GRID_SIZE;

        int dx = 0, dy = 0;
        if (rand() % 2 == 0)
            dx = 1;
        else
            dy = 1;

        if (isPlaceAvailable(x, y, grid, ships[index], dx, dy))
        {
            for (int i = 0; i < ships[index].size; i++)
            {
                grid[y + i * dy][x + i * dx] = SHIP;
            }
            index += 1;
        }
    }
}

void displayMap(char grid[GRID_SIZE][GRID_SIZE],int turn)
{   
    char* playerName=turn==1?"child":"parent";
    printf("\n%s's field:\n", playerName);
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            printf("%c ", grid[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}
int *getAdjacentPoint(char grid[GRID_SIZE][GRID_SIZE], int y, int x)
{
    static int point[2];
    point[0] = y;
    point[1] = x;
    if (y + 1 < GRID_SIZE && (grid[y + 1][x] == SHIP || grid[y + 1][x] == TILE))
        point[0] = y + 1;
    else if (y - 1 >= 0 && (grid[y - 1][x] == SHIP || grid[y - 1][x] == TILE))
        point[0] = y - 1;
    else if(x+1<GRID_SIZE&&(grid[y][x+1]==SHIP||grid[y][x+1]==TILE))
        point[1]= x+1;
    else if(x-1>=0&&(grid[y][x-1]==SHIP||grid[y][x-1]==TILE))
        point[1]=x-1;
    
    return point;
}
void makeMove(BattleFieldInfo *state)
{
    char *playerName,*z;
    char(*grid)[GRID_SIZE];
    int *remShip;
    if (state->turn)
    {
        grid = state->parentGrid;
        remShip = &state->remainingShipsHealthPar;
        playerName="child";

    }
    else
    {
        grid = state->childGrid;
        remShip = &state->remainingShipsHealthChild;
        playerName="parent";
    }

    static int lastHit[2] = {-1, -1};
    int isMove = 0;
    int x, y;
    int isHit;
    while (!isMove)
    {
        if (lastHit[0] == -1)
        {
            z="random";
            x = rand() % GRID_SIZE;
            y = rand() % GRID_SIZE;

            if (grid[y][x] == SHIP)
            {
                grid[y][x] = HIT;
                isMove = 1;
                (*remShip)--; // Decrement remaining ships
                lastHit[0] = y;
                lastHit[1] = x;
                isHit=1;
            }
            else if (grid[y][x] == TILE)
            {
                grid[y][x] = MISS;
                isMove = 1;
            }
        }
        else
        {
            int *estimatedPoint = getAdjacentPoint(grid, lastHit[0], lastHit[1]);
            if (estimatedPoint[0] == lastHit[0]&&estimatedPoint[1]==lastHit[1])
            {
                lastHit[0] = -1;
                lastHit[1] = -1;
            }
            else
            {
                z="ai";
                x = estimatedPoint[1];
                y = estimatedPoint[0];
                if (grid[y][x] == SHIP)
                {
                    grid[y][x] = HIT;
                    isMove = 1;
                    (*remShip)--;
                    lastHit[0] = y;
                    lastHit[1] = x;
                    isHit=1;
                }
                else if (grid[y][x] == TILE)
                {
                    grid[y][x] = MISS;
                    isMove = 1;
                }
            }
        }
    }
    displayMap(grid,!state->turn);
    printf("%s->%s attack:%d , %d\n--------------------------\n",z,playerName,y,x);
}

void createShips(Ship *ships)
{
    int sizeOfShips[SHIPS] = {2, 2, 3, 3, 4};
    for (int i = 0; i < SHIPS; i++)
    {   
        ships[i].size = sizeOfShips[i];
    }
}

int main()
{
    srand(time(NULL)*13);

    const char *shm_name = "Battlefield"; // name of the shared memory object
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    // Truncate shared memory to the size of battlefild
    ftruncate(shm_fd, sizeof(BattleFieldInfo));

    BattleFieldInfo *state = mmap(0, sizeof(BattleFieldInfo), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // Initializes maps
    initializeMap(state->parentGrid);
    initializeMap(state->childGrid);
    printf("%d\n",state->turn);
    state->remainingShipsHealthChild = 14;
    state->remainingShipsHealthPar = 14;

    pid_t pid = fork(); // creates child process
    if (pid == -1)
    { // failed to fork
        exit(1);
    }
    Ship ships[SHIPS];
    createShips(ships);
    if (pid == 0)
    {   
        placeRandomShips(state->childGrid, ships);
        while (state->remainingShipsHealthPar > 0 && state->remainingShipsHealthChild > 0)
        { // continues if both side have at least one ship

            if (!state->turn)
            {
                makeMove(state);
                if (state->remainingShipsHealthPar > 0 && state->remainingShipsHealthChild > 0)
                    state->turn = 1;
            }
        }
        usleep(5);
        if (state->remainingShipsHealthPar == 0)
        {
            printf("\nChild wins! Final Grids:\n");
            displayMap(state->parentGrid, 0);
            displayMap(state->childGrid, 1);
        }

        exit(0); // exit child process
    }
    else
    {
        placeRandomShips(state->parentGrid, ships);
        while (state->remainingShipsHealthPar > 0 && state->remainingShipsHealthChild > 0)
        { // continues if both side have at least one ship

            if (state->turn)
            {
                makeMove(state);
                if (state->remainingShipsHealthPar > 0 && state->remainingShipsHealthChild > 0)
                    state->turn = 0;
            }
        }
        usleep(5);
        // Wait for child process to end
        wait(NULL);
        if (state->remainingShipsHealthChild == 0)
        {
            printf("\nParent wins! Final Grids:\n");
            displayMap(state->parentGrid, 0);
            displayMap(state->childGrid, 1);
        }
    }

    // Cleanup the shared memory region(battlefield)
    munmap(state, sizeof(BattleFieldInfo));
    shm_unlink(shm_name);

    return 0;
}