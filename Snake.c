#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

/*
<termios.h>
Terminal I/O interfaces
Implemented on getStep() function.
*/
#include <termios.h>

// define board DIMENSIONS
#define ROWS 20
#define COLS 60

// struct Snake body - linkedList.
typedef struct SnakeBody
{
    struct SnakeBody *pPrevCell;
    int position; // each Cell hold his position
} SnakeBody;

// game board
char board[ROWS * COLS] = {0};

// share variable for collect input from Thread, and manage on main Thread
char userStep = ' ';

// Snake head
SnakeBody *snakeHead;

// handle last cell of snake to insert new Cell on O(1)
SnakeBody *handleLastCell;

// helping to increase snake with manage his last index
int prevIndex;

/*
 ********************** START Threads SECTION **********************
 */

// collect input wihtout conform ENTER
char getStep()
{
    // buf size 1 Byte on Ubuntu, x86_64
    char buf = 0;
    // old ~ 60 Byte
    // This struct is used to store the original terminal attributes
    struct termios old = {0};
    // flushes the output buffer of a stream.
    fflush(stdout);
    // gets the parameters associated with the terminal referred to by fildes and stores them in the termios structure
    // fildes, termios
    if (tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    // Disables canonical mode, which means input is read one character at a time rather than line by line.
    old.c_lflag &= ~ICANON;
    // echo newline off
    old.c_lflag &= ~ECHO;
    // Sets the minimum number of characters for a read operation to complete to 1.
    old.c_cc[VMIN] = 1;
    // Sets the timeout in tenths of a second for a read operation to complete to 0,
    // meaning it will block until the minimum number of characters are read.
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr TCSANOW");
    // Reads a single character from stdin into buf.
    if (read(0, &buf, 1) < 0)
        perror("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    // Restores the terminal attributes to their original values.
    // This is done in a way that waits for all output to be written to the terminal before making the changes.
    if (tcsetattr(0, TCSADRAIN, &old) < 0)
        perror("tcsetattr ~TCSADRAIN");

    return buf;
}

// manage user Input and set "userStep" variable
void *userMove()
{
    char ch;
    while (1)
    {
        ch = getStep(); // Use non-blocking getch() to get input - skipping the Enter action
        if (ch == 'w' || ch == 's' || ch == 'a' || ch == 'd' || ch == 'q')
        {
            userStep = ch; // set moving Direction
        }
    }
}

// Thread clear board, print board and wait for 1 second.
void *printBoard()
{
    int boardLength = ROWS * COLS;
    while (1)
    {
        printf("\033[H\033[J");
        for (int i = 0; i < boardLength; i++)
        {
            if (i != 0 && i % COLS == 0)
                printf("\n");
            printf("%c", board[i]);
        }
        sleep(1);
    }
}

// Thread generate a Food on board, taking a break each 7 second.
void *generateFood()
{
    int maxValue = (ROWS * COLS) - COLS - 1;
    while (1)
    {
        int ran = (COLS + 1) + rand() % maxValue - 1;
        if (ran % COLS != 0 && ran % COLS != COLS - 1 && board[ran] == ' ')
        {
            board[ran] = 'F';
            sleep(7);
        }
    }
}
/*
 ********************** END Threads SECTION **********************
 */

/*
 ********************** START initial SECTION **********************
 */

// init game board and define his Cell's
void initBoard()
{
    for (int i = 0; i < ROWS * COLS; i++)
    {
        if (i < COLS || i >= (ROWS * COLS) - COLS)
            board[i] = '-';
        else if (i != (ROWS * COLS) - ROWS && i % COLS == 0 || i % COLS == COLS - 1)
            board[i] = '|';
        else
            board[i] = ' ';
    }
}

// init Snake head
void initSnakeHead()
{
    snakeHead = (SnakeBody *)malloc(sizeof(SnakeBody));
    if (snakeHead == NULL)
    {
        printf("Failed to allocate memory for snakeHead\n");
        exit(1);
    }
    snakeHead->pPrevCell = NULL;
    snakeHead->position = (ROWS * COLS) / 2 + 30; // first cell (head) position on board
}

void initSnakeOnBoard()
{
    board[snakeHead->position] = 'X';
}

/*
 ********************** END initial SECTION **********************
 */

/*
 ********************** START game_system SECTION **********************
 */

// if head collide with Food - increase Snake cell by one
void increaseSnake()
{
    SnakeBody *newCell = (SnakeBody *)malloc(sizeof(SnakeBody)); // head <- cell1 <- cell2 ...
    if (newCell == NULL)
    {
        printf("Failed to allocate memory for newCell");
        exit(1);
    }

    newCell->pPrevCell = NULL;
    newCell->position = prevIndex;

    handleLastCell->pPrevCell = newCell;
    handleLastCell = newCell;
}

// check to ligal move - exit the program if not.
int isLigalMove(int dist)
{
    if (board[dist] != ' ' || board[dist] != 'F')
        exit(1);

    return 1;
}

// update the snake cell by moving them to who they are connected.
void updateSnakeCells(int dist)
{
    SnakeBody *pTempCell = snakeHead;
    int temp;

    prevIndex = pTempCell->position;
    pTempCell->position = dist;

    while (pTempCell != NULL)
    {
        pTempCell = pTempCell->pPrevCell;
        if (pTempCell == NULL)
            break;

        temp = pTempCell->position;
        pTempCell->position = prevIndex;
        prevIndex = temp;
    }
}

// update game board to view, by Snake cell's position
void updateBoard()
{
    SnakeBody *pTempCell = snakeHead;

    while (pTempCell != NULL)
    {
        board[pTempCell->position] = 'X';
        pTempCell = pTempCell->pPrevCell;
    }
    board[prevIndex] = ' ';
}
/*
 ********************** END game_system SECTION **********************
 */

int main()
{
    // declare Thread's
    pthread_t print_t, move_t, foodDrop_t;

    int dist = -1;

    // first initialize
    initBoard();
    initSnakeHead();
    initSnakeOnBoard();
    handleLastCell = snakeHead;

    // Threads initialize
    pthread_create(&print_t, NULL, printBoard, NULL);
    pthread_create(&move_t, NULL, userMove, NULL);
    pthread_create(&foodDrop_t, NULL, generateFood, NULL);

    while (userStep != 'q')
    {
        if (userStep == 'w')
        {
            dist = snakeHead->position - COLS;
        }
        if (userStep == 's')
        {
            dist = snakeHead->position + COLS;
        }
        if (userStep == 'd')
        {
            dist = snakeHead->position + 1;
        }
        if (userStep == 'a')
        {
            dist = snakeHead->position - 1;
        }
        if (dist != -1) // 2. check if ligal move -> isLigalMove - READY
        {
            // 3. call updateSnakeCells - get as an input the destension - READY
            updateSnakeCells(dist);
            if (board[dist] == 'F') // simulation for EAT
            {
                board[dist] = ' '; // removing 'F' from cell after Snake colide with Food
                increaseSnake();
            }
            // 4. call updateBoard - READY
            updateBoard();
            sleep(1);
        }
    }
    
    /*
        delete dynamic memory;
    */
    SnakeBody* handleToDelete;
    while (snakeHead != NULL)
    {
        handleToDelete = snakeHead;
        snakeHead = snakeHead->pPrevCell;
        free(handleToDelete);
    }
    return 0;
}