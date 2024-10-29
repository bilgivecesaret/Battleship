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
    int turn;				   // To track whose turn it is
    int remainingShipsHealthPar;		 // Number of parent's ships left
    int remainingShipsHealthChild;		 // Number of child's ships left
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

void displayMap(char grid[GRID_SIZE][GRID_SIZE], int turn)
{
    char *playerName=turn==1?"child":"parent";
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
void displayGrids(BattleFieldInfo *state, int gridsCreated) 
{
    if (!gridsCreated) {
        printf("You have to create first grids first!\n");
    } else {
        displayMap(state->parentGrid, 0);
        displayMap(state->childGrid, 1);
    }
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

void createFirstGrids(BattleFieldInfo *state, Ship ships[], int *gridsCreated) // This method creates the game's first grids
{
        initializeMap(state->parentGrid);
        initializeMap(state->childGrid);
        placeRandomShips(state->parentGrid, ships);
        placeRandomShips(state->childGrid, ships);
        state->remainingShipsHealthChild = 14;
        state->remainingShipsHealthPar = 14;
        state->turn = 0;
        *gridsCreated = 1;
        printf("New grids created!\n");
}

void startGame(BattleFieldInfo *state, int gridsCreated) { //This method starts game also if first grids are not created displays a warning. 
    if (gridsCreated) 
    {
        printf("New game started!\n");

        while (state->remainingShipsHealthPar > 0 && state->remainingShipsHealthChild > 0) {
            makeMove(state);
            state->turn = !state->turn;
        }

        if (state->remainingShipsHealthPar == 0) 
        {
            printf("\nChild wins! Final Grids:\n");
        } 
        else 
        {
            printf("\nParent wins! Final Grids:\n");
        }
        displayGrids(state,gridsCreated);
    } 
    else 
    {
        printf("Please create first grids first!\n");
    }
}

void reLocateShips(BattleFieldInfo *state, Ship ships[], int *gridsCreated) { // This method places ships random again also if first grids are not exist displays an warning.
	
    if (*gridsCreated == 0) 
    {
        printf("First grids are not exist!\n");
    } 
    else
    {
        initializeMap(state->parentGrid);
        initializeMap(state->childGrid);
        placeRandomShips(state->parentGrid, ships);
        placeRandomShips(state->childGrid, ships);
        state->remainingShipsHealthChild = 14;
        state->remainingShipsHealthPar = 14;
        printf("Ships relocated!\n");
    }
    
}

void displayMenu() { // Menu Method.
    printf("------------------------------\n");
    printf("1: Create First Grids\n");
    printf("2: Start Game\n");
    printf("3: Display Grids\n");
    printf("4: Relocate Ships\n");
    printf("5: Exit Game\n");
    printf("------------------------------\n");
    printf("Choose an option: ");
}

int main() {
    srand(time(NULL) * 13);
    
    const char *shm_name = "Battlefield"; // name of the shared memory object
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    // Truncate shared memory to the size of battlefild
    ftruncate(shm_fd, sizeof(BattleFieldInfo));
    
    BattleFieldInfo *state = mmap(0, sizeof(BattleFieldInfo), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    Ship ships[SHIPS];
    createShips(ships);

    int choice, gameRunning = 1;
    int isGameRunning = 1;
    int gridsCreated = 0;
    
    pid_t pid = fork(); // creates child process
    if (pid == -1)
    { // failed to fork
        exit(1);
    }
    else if(pid == 0)
    {
    while (gameRunning) {
        displayMenu();
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                createFirstGrids(state, ships, &gridsCreated);
                break;

            case 2:
                startGame(state, gridsCreated);
                break;

            case 3:
                displayGrids(state,gridsCreated);
                break;

            case 4:
                reLocateShips(state, ships, &gridsCreated);
                break;

            case 5:
                printf("Exiting game. \n");
                gameRunning = 0;
                break;

            default:
                printf("Invalid option. Please choose again.\n");
                break;
        	}
    	}
    }
    else
    {
    wait(NULL);
    }

    

    // Cleanup the shared memory region(battlefield)
    munmap(state, sizeof(BattleFieldInfo));
    shm_unlink(shm_name);
    return 0;
}

