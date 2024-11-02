#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/wait.h>
#include <string.h>
#include <ncurses.h>

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

int getColor(char tileSign){
    if(tileSign==TILE)
        return 1;
    else if(tileSign==HIT)
        return 2;
    else if(tileSign==MISS)
        return 3;
    else
        return 4;
}

void displayMap(BattleFieldInfo *state){
    clear();
    char(*grid)[ROW_SIZE] = state->parentGrid;
    char(*grid2)[ROW_SIZE] = state->childGrid;


    start_color();
    use_default_colors();
    init_pair(1, COLOR_WHITE, -1);  // for empty tile
    init_pair(2, COLOR_RED, -1);    // hit
    init_pair(3, COLOR_YELLOW, -1); // miss 
    init_pair(4, COLOR_BLUE, -1);   // ships (DESTROYER, BATTLESHIP, CRUISER)

    printw("Player %s's Grid:      Player %s's Grid:\n", "parent", "child");

    for (int i = 0; i < ROW_SIZE; i++){
        for (int j = 0; j < ROW_SIZE; j++){
            attron(COLOR_PAIR(getColor(grid[i][j])));
            printw("%c ", grid[i][j]);
            attroff(COLOR_PAIR(1) | COLOR_PAIR(2) | COLOR_PAIR(3) | COLOR_PAIR(4));
        }

        
        printw("           ");

        for (int j = 0; j < ROW_SIZE; j++){
            attron(COLOR_PAIR(getColor(grid2[i][j])));
            printw("%c ", grid2[i][j]);
            attroff(COLOR_PAIR(1) | COLOR_PAIR(2) | COLOR_PAIR(3) | COLOR_PAIR(4));
        }
        printw("\n");
    }

    refresh(); // update terminal   
}

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
int *getAdjacentPoint(char grid[ROW_SIZE][ROW_SIZE], int y, int x)//better approximation to estimate next points: down->right->up->left
{
    static int point[2];
    point[0] = y;
    point[1] = x;
    if (y + 1 < ROW_SIZE && !(grid[y + 1][x] == MISS || grid[y + 1][x] == HIT))
        point[0] = y + 1;
    else if (x + 1 < ROW_SIZE && !(grid[y][x + 1] == MISS || grid[y][x + 1] == HIT))
        point[1] = x + 1;
    else if (y - 1 >= 0 && !(grid[y - 1][x] == MISS || grid[y - 1][x] == HIT))
        point[0] = y - 1;
    else if (x - 1 >= 0 && !(grid[y][x - 1] == MISS || grid[y][x - 1] == HIT))
        point[1] = x - 1;

    return point;
}
void updatePosition(int *arr, int y, int x)
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
int aiMove(BattleFieldInfo *state)
{
    char *pName = state->turn ? "child" : "parent";
    char(*grid)[ROW_SIZE] = (state->turn == 1) ? state->parentGrid : state->childGrid;

    static int lastHit[2] = {-1, -1};
    static int hitDirect[2] = {0, 0}; // (up-down, left-right)
    static int combo = 0;
    if (combo > 1)//that means both random and estimated ai hitted a point
    {   
        if(!isInsideGrid(lastHit[0] + hitDirect[0],lastHit[1] + hitDirect[1])){
            updatePosition(hitDirect, hitDirect[0] * -combo, hitDirect[1] * -combo);
        }
        int aiEst[2] = {lastHit[0] + hitDirect[0], lastHit[1] + hitDirect[1]};
        if (isInsideGrid(aiEst[0], aiEst[1]))
        {
            char *pointSign = &grid[aiEst[0]][aiEst[1]];
            if (isShip(*pointSign))
            {
                Ship *ship = getShip(state, *pointSign);
                *pointSign = HIT;
                printw("%s directed ai hit  %d,%d \n", pName, aiEst[0], aiEst[1]);                
                displayMap(state);
                if((ship->size-=1)<=0){
                    combo=0;
                    lastHit[0]=-1;
                    return 1;
                }
                combo += 1;
                updatePosition(hitDirect, (aiEst[0] - lastHit[0]) % 2, (aiEst[1] - lastHit[1]) % 2);//readjust the direction {-1,0,1}
                updatePosition(lastHit, aiEst[0], aiEst[1]);
                return 1;
            }
            else if (*pointSign == TILE)
            {
                *pointSign = MISS;
                printw("%s directed ai miss  %d,%d\n", pName, aiEst[0], aiEst[1]);
                displayMap(state);
                updatePosition(hitDirect, hitDirect[0] * -combo, hitDirect[1] * -combo);//reverse the direction If the ship hasn't sunk yet 
                return 1;
            }
        }
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
                *pointSign = HIT;
                printw("%s, ai estimated hit %d,%d\n", pName, aiEst[0], aiEst[1]);
                displayMap(state);
                if((ship->size-=1)<=0){
                    combo=0;
                    lastHit[0]=-1;
                    return 1;
                }
                combo += 1;
                updatePosition(hitDirect, aiEst[0] - lastHit[0], aiEst[1] - lastHit[1]);
                updatePosition(lastHit, aiEst[0], aiEst[1]);
            }
            else if (*pointSign == TILE)
            {
                *pointSign = MISS;
                printw("%s, ai estimated miss %d,%d\n", pName, aiEst[0], aiEst[1]);
                displayMap(state);
            }
            return 1;
        }
        lastHit[0] = -1;
    }

    combo = 0;
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
            printw("%s random hit %d,%d\n", pName, y, x);
            displayMap(state);
            updatePosition(lastHit, y, x);
            return 1;
        }
        else if (*pointSign == TILE)
        {
            *pointSign = MISS;
            printw("%s random miss %d,%d\n", pName, y, x);
            displayMap(state);
            return 1;
        }
    }  
    refresh();
    clear();
}

void initializeMap(char grid[ROW_SIZE][ROW_SIZE])
{
    for (int i = 0; i < ROW_SIZE; i++)
        for (int j = 0; j < ROW_SIZE; j++)
            grid[i][j] = TILE;
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
    sleep(1);
    static int gameOver=0;
    char *whoWin=!parentShipsAlive?"Child":(!childShipsAlive?"Parent":NULL);
    if(!gameOver &&whoWin){
        gameOver=1;
        printw("%s won the game!\n",whoWin);
    }

    return !(parentShipsAlive && childShipsAlive);
}

    void createGrids(BattleFieldInfo *state) { //This method creates the grids.
        initializeMap(state->parentGrid);
        initializeMap(state->childGrid);
        locateShips(state->parentGrid);
        locateShips(state->childGrid);
        memcpy(state->parShips, ships, sizeof(ships));
        memcpy(state->childShips, ships, sizeof(ships));
}


    void menu() { // Menu method.
        printw("1. Create First Grids\n");
        printw("2. Start Game\n");
        printw("3. Display Grids\n");
        printw("4. Relocate Grids\n");
        printw("5. Exit Game\n");
        printw("Choose an option: ");
}

int main() {
    srand(time(NULL) * 13);

    const char *shm_name = "Battlefield";
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(BattleFieldInfo));

    BattleFieldInfo *state = mmap(0, sizeof(BattleFieldInfo), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    char choice;
    
    int createdGrids = 0;
    initscr(); // login ncurses
    noecho();
    do {	// This loop makes game playable more than one time.
        
	menu();
        choice = getch();	
	printw("\n");
        switch (choice) {
            case '1':
                createGrids(state);
                createdGrids = 1;
                printw("Grids created.\n\n");
                break;
            case '2': {
            	if(!createdGrids){
            		printw("You have to create grids first.\n\n");
            	}
            	else
            	{
                	pid_t pid = fork(); // creates child process
                	if (pid == -1) { // failed to fork
		            	exit(1);
		        }
		        if (pid == 0) {		            
		            while (!isGameOver(state)) {
		                if (state->turn && aiMove(state)) {
                            sleep(0.2);
		                    state->turn = !state->turn;
		                }
		            }
		            exit(0); // exit child process
		        } else {
		            while (!isGameOver(state)) {
		                if (!state->turn && aiMove(state)) {
                            sleep(0.2);
		                    state->turn = !state->turn;
		                }
		            } 
		            printw("Game completed.\n\n");
			 }			 
       	}
       	createdGrids = 0;
               break;
            }
            case '3':
            if(!createdGrids){
            	printw("Grids must be created to display.\n\n");
            }else
            {
                displayMap(state);
            }
	     break;
            case '4':
            if(!createdGrids){
            	printw("Grids must be created to relocate.\n\n");
            }else
            {
                createGrids(state);
                printw("Grids relocated.\n\n");
            }
             break;
            case '5':
                printw("Exiting game.\n");
                break;
            default:
                printw("Invalid choice.\n\n");
                break;
        }
    } while (choice != '5');
    endwin(); // logout ncurses

    // Cleanup the shared memory region(battlefield)
    munmap(state, sizeof(BattleFieldInfo));
    shm_unlink(shm_name);

    return 0;
}