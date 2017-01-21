/*
 * snake->cpp
 *
 *  Created on: Oct 28, 2016
 *      Author: owen
 */
#include <sys/ioctl.h>
#include <ncurses.h>
#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>

/*
 * global values that hold the size of the terminal.i should take out later
 */

int sizex;
int sizey;
int direction = 0;
int snakeAlive = 0;
int tAlive = 0;
int tCol = 0;
int tRow = 0;
//struct aiocb kbcbuf;

/*
 * maybe add queue for directions to track where the end of the snake is and erase it
 */

struct snakeHead{
	int row;
	int col;
	int size;
	int direction;
	int queueWait;
	char icon;
};

struct snakeHead snake;

SIMPLEQ_HEAD(section_list, section) queue;
struct section_list *snakeTail;

struct section{
	int row;
	int col;
	SIMPLEQ_ENTRY(section) sections;
};

struct alarm{
	int row;
	int col;
};

/*
 * initiates random numbers
 */

void startRNG(){
	time_t t;
	srand(time(&t));
}

/*
 * checks collision when placing trophy.
 */

int checkCollision(int targetCol, int targetRow){
	char charAtPos = mvinch(targetRow,targetCol);
	switch(charAtPos){
	case 'V': case 'A': case '>': case '<': case '0':{
		return 0;
		break;
	}
	case '1': case'2': case'3': case'4': case'5': case '6': case '7': case '8': case'9':
	{
		return 0;
		break;
	}
	default:
		return 1;
	}
}

/*
 * checks collision when snake is moving. adds to snake if encounters a trophy
 */

int checkCollision(int targetCol, int targetRow, struct snakeHead* snake){
	char charAtPos = mvinch(targetRow,targetCol);
	switch(charAtPos){
	case 'V': case 'A': case '>': case '<': case '0':{
		return 0;
		break;
	}
	case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case'9':
	{
		snake->size += (int)charAtPos - 48;
		snake->queueWait += (int)charAtPos - 48;
		tAlive = 0;
		return 1;
		break;
	}
	default:
		return 1;
	}
}
/*
 * gets window size.
 */
void getWinSize(int* size){
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	size[0] = w.ws_col;
	size[1] = w.ws_row;
	sizex = w.ws_col;
	sizey = w.ws_row;
}

/*
 * create start screen
 */

void createStart(int col, int row){

}

/**
 * remove trophy at x,y when alarm goes off
 */

void removeTrophy(int signum){
	move(tRow,tCol);
	addch(' ');
	tAlive = 0;
}

/**
 * places trophy of val 1 through 9 in a random unoccupied position
 */

void createTrophy(){
	int occupied = 1;
	while(occupied == 1)
	{
		tCol = rand() % (sizex-2)+1;
		tRow = rand() % (sizey-2)+1;
		move(tRow,tCol);
		if(checkCollision(tCol, tRow) == 1){
			occupied = 0;
		}
	}
	int random = rand()%9+1;
	addch(random + 48);
	signal(SIGALRM, removeTrophy);
	alarm(random+1);
	tAlive = 1;
	refresh();
}

/*
 * sets starting variables for snake
 */

void createSnake(int col, int row, struct snakeHead* snake){
	snake->row = row/2;
	snake->col = col/2;
	snake->size = 1;
	snake->direction = rand()%4+1;
	snake->icon = '0';
	snake->queueWait = 0;
}


/*
 * lays out game board
 */

void createBoard(int columns, int rows){
	clear();
	int snakeAlive =1;
	int done = 0;
	move(0,0);
	for(int i=0; i<=rows-1; i++){
		move(i,0);
		addch('>');
		move(i,columns-1);
		addch('<');
	}
	for(int i=0; i<=columns-1; i++){
		move(0,i);
		addch('V');
		move(rows-1,i);
		addch('A');
	}
	refresh();
}

/*
 * changes snake struct coordinates to the next space.
 * returns the direction: 1=N 2=W 3=S 4=E
 *
 */
int snakeNext(struct snakeHead* snake){
	if(direction == 0)
	{
		direction = snake->direction;
	}
	else
	{
		snake->direction = direction;
	}
	if (direction == 1)
	{
		snake->row += 1;
	}
	if(direction == 2)
	{
		snake->col += 1;
	}
	if(direction == 3)
	{
		snake->row -= 1;
	}
	if(direction == 4)
	{
		snake->col -= 1;
	}
	return direction;
}

void queueSegment(){

}

void dequeueSegment(){

}


/*
 * 1 turn takes place. 1 get direction, 2 generate trophy, 3 determine collision, 4 move, 5 pop off tail if tail queueWait is empty
 */
int Turn(struct snakeHead* snake){

	snakeNext(snake);
	int nextMove = checkCollision(snake->col, snake->row, snake);   //turns off next move for troubleshooting. it can be determined by check collision when completed
	int chance = rand() % 10 + 1;					 //determines if a trophy spawns this turn
	if (tAlive == 0){
		createTrophy();
		move(snake->row, snake->col);
	}
	if(nextMove == 0){                         		 //bad collision, game over
		return 0;
	}
	move(snake->row, snake->col);
	addch(snake->icon);
	struct section *segment;
	segment =(section*) malloc(sizeof(section));
	segment->row = snake->row;
	segment->col = snake->col;
	SIMPLEQ_INSERT_TAIL(&queue, segment, sections);
	if(snake->queueWait<1)                          //get data from head of queue and remove that segment
	{
		struct section segment;
		segment = *SIMPLEQ_FIRST(&queue);
		SIMPLEQ_REMOVE_HEAD(&queue,sections);
		move(segment.row, segment.col);
		addch(' ');
		refresh();
	}
	else{
		snake->queueWait = snake->queueWait - 1;
		//queue direction moved
	}
	move(snake->row, snake->col);
	refresh();
	return 1;
}

void takeTurn(int signum){
	snakeAlive = Turn(&snake);
}

/*
 * changes snakes direction based on user input
 */
void changeDirection(int signum){
	int c = getch();
	//char *cp = (char *) kbcbuf.aio_buf;

	//if(aio_error(&kbcbuf)!=0)
	//	perror("reading failed");
	//else
	//	if(aio_return(&kbcbuf) == 1)
	//	{
	//		c = *cp;
	//	}
	switch(c){
		case 'a':{
			direction = 4;
			break;
			//return 4;
		}
		case 'w':{
			direction = 3;
			break;
			//return 3;
		}
		case 'd':{
			direction = 2;
			break;
			//return 2;
		}
		case 's':{
			direction = 1;
			break;
			//return 1;
		}
		//case ' ':{
		//	pause();
		//	break;
		//}
		default:{
			break;
		}
	}
	//aio_read(&kbcbuf);
	//return 0;
}


/*
 * from the book
 */
void enable_kdb_signals(){
	int fd_flags;
	fcntl(0, F_SETOWN, getpid());
	fd_flags = fcntl(0, F_GETFL);
	fcntl(0, F_SETFL, (fd_flags|O_ASYNC));
}


/*
 * from the book
 */
int set_ticker(int n_msecs){
	struct itimerval new_timeset;
	long n_sec, n_usecs;

	n_sec = n_usecs/1000;
	n_usecs = ( n_msecs % 1000) * 1000L;

	new_timeset.it_interval.tv_sec = n_sec;
	new_timeset.it_interval.tv_usec = n_usecs;
	new_timeset.it_value.tv_sec = n_sec;
	new_timeset.it_value.tv_usec = n_usecs;

	return setitimer(ITIMER_REAL, &new_timeset, NULL);
}

/*
 * from book
 */
/*
void setup_aio_buffer(){
	static char input[1];
	kbcbuf.aio_fildes = 0;
	kbcbuf.aio_buf = input;
	kbcbuf.aio_nbytes = 1;
	kbcbuf.aio_offset = 0;

	kbcbuf.aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	kbcbuf.aio_sigevent.sigev_signo = SIGIO;
	{
*/
int main(){
	curs_set(0);
	int size[2];
	getWinSize(size);
	int maxSnake = ((size[0]+size[1])*6);
	startRNG();
	initscr();
	crmode();
	noecho();
	//createStart(size[0],size[1]);
	createBoard(size[0],size[1]);
	//struct snakeHead snake;
	createSnake(size[0], size[1], &snake);
	SIMPLEQ_INIT(&queue);                            //init snake queue
	struct section *segment;
	segment =(section*)malloc(sizeof(section));							//add first segment
	segment->row = snake.row;
	segment->col = snake.col;
	SIMPLEQ_INSERT_HEAD(&queue, segment, sections);
	SIMPLEQ_INSERT_TAIL(&queue, segment, sections);
	signal(SIGIO, changeDirection);					//set up signal handler
	//setup_aio_buffer();
	//aio_read(&kbcbuf);
	enable_kdb_signals();
	createTrophy();									//create first trophy
	snakeAlive = 1;
	while(snakeAlive == 1 && snake.size < maxSnake)
	{
		refresh();
		int wait = 100000 - 1000 *(snake.size);
		//signal(SIGALRM,takeTurn);
		//alarm(wait);
		//pause();
		snakeAlive = Turn(&snake);
		usleep(wait);
	}
	int sections = 0;
    while (SIMPLEQ_FIRST(&queue) != NULL){    /* Delete. */
            SIMPLEQ_REMOVE_HEAD(&queue, sections);
            sections += 1;
    }
    addch(sections);
	if (snakeAlive == 1){
		puts("Congratulations you win!");
		//print congratulations ask retry
	}
	else{
		//print game over ask retry
	}
	endwin();
	return 0;
}

