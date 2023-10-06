/*
	Arcade game Stacker for the terminal
*/

#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>

#define FIRSTROW 4

static struct winsize w;
static struct termios oldattr, newattr;

static int blocks = 3;       // 3 to 0
static int pos = 0;          // -2 to 6 / pos of most left block
static int dir = 1;          // move direction / 1 right, 0 left
static int current_row = 14; // 14 to 0
static int col;
static char field[15][7];

static void draw_ui(void) {
	int row;

	// clear terminal
	printf("\033[H\033[J");

	printf("Press escape to exit\nOther buttons place blocks");

	printf("\033[%d;%dH STACKER", 2, (w.ws_col - 9) / 2);
	printf("\033[%d;%dH_________", 3, (w.ws_col - 9) / 2);

	for (row = FIRSTROW; row < 15 + FIRSTROW; row++) {
		printf("\033[%d;%dH|       |", row, (w.ws_col - 9) / 2);

	if (row == 15 + FIRSTROW - 15) {
		printf(" major price");
	} else if (row == 15 + FIRSTROW - 11) {
		printf(" minor price");
	}

	}

	printf("\033[%d;%dHTTTTTTTTT", 15 + FIRSTROW, (w.ws_col - 9) / 2);
}

static void draw_fields(void) {
	int row;

	// only print rows at or below current_row
	for (row = current_row; row < 15; row++) {
		for (col = 0; col < 7; col++) {
			printf("\033[%d;%dH%c", row + FIRSTROW, ((w.ws_col - 9) / 2) + 1 + col,
			       field[row][col]);
		}
	}
}

static void place_blocks(void) {
	for (col = pos; col < pos + blocks; col++) {
		// bounds check
		if (col >= 0 && col < 7) {
			field[current_row][col] = '#';
			// field[current_row][col] = 'a' + col - pos; // a to c / for debug
		}
	}
}

static void move_blocks(void) {
	// move right
	if (dir == 1) {
		// most left block
		if (pos < 6) {
			pos++;
		} else {
			pos--;
			dir = 0;
		}
	// move left
	} else {
		// most right block
		if (pos + blocks > 1) {
			pos--;
		} else {
			pos++;
			dir = 1;
		}
	}
}

static void draw_message(char *str) {
	printf("\033[%d;%dH%s", 16 + FIRSTROW, ((w.ws_col - (int)strlen(str)) / 2), str);
}

static int end_game(int ret) {
	// show cursor
	printf("\033[?25h");

	// move cursor to line below
	printf("\033[%d;%dH", 17 + FIRSTROW, 1);

	// restore original terminal setting
	tcsetattr(0, TCSANOW, &oldattr);

	return ret;
}





int main(void) {
	struct pollfd pfd[1];

	// ms per loop, kinda
	int interval = 70; // a value that felt good


// terminal stuff
	// hide cursor
	printf("\033[?25l");

	// save original terminal setting
	tcgetattr(0, &oldattr);

	// enable raw terminal mode
	newattr = oldattr;
	newattr.c_lflag &= ~ICANON;
	newattr.c_lflag &= ~ECHO;
	newattr.c_cc[VMIN] = 1;
	newattr.c_cc[VTIME] = 0;
	tcsetattr(0, TCSANOW, &newattr);


	// disable stdout buffer
	setbuf(stdout, NULL);

	pfd[0].fd = 0; // 0 is for stdin
	pfd[0].events = POLLIN;

	// set seed
	srand(time(NULL));

	// get terminal window size
	ioctl(0, TIOCGWINSZ, &w);


	draw_ui();
	place_blocks();
	draw_fields();
	move_blocks();


	// game loop
	for (;;) {
		int ret;
		char userinput = 0;

		ret = poll(pfd, 1, interval);

		if (ret > 0) {
			userinput = getchar();
		} else if (ret < 0) {
			fprintf(stderr, "poll error\n");
			return end_game(1);
		}


		if (userinput == 27) {
			// escape to exit
			return end_game(0);
		} else if (userinput != 0){

			// in rows above the lowest row
			if (current_row < 14) {
				blocks = 0;
				// check if blocks stack
				for (col = 0; col < 7; col++) {
					// check if field is a block
					if (field[current_row][col] != ' ') {
						// check fields below
						if (field[current_row + 1][col] != ' ') {
							blocks++;
						} else {
							field[current_row][col] = ' ';
						}
					}
				}
			}


		// prepare for next row
			pos = rand() / (RAND_MAX / (7 - blocks));
			dir = rand() & 1;
			interval -= interval * 0.06; // value that felt good
			current_row--;


			// check for win
			if (current_row < 0) {
				draw_message("you won");
				return end_game(0);
			}

			// check for game over
			if (blocks == 0) {
				draw_message("game over");
				return end_game(0);
			}


		// reduce blocks due to current row number
			// max blocks in row 4 is 2
			if (current_row == 15 - 4 && blocks > 2) {
				blocks = 2;
			}

			// max blocks in row 10 is 1
			if (current_row == 15 - 10) {
				blocks = 1;
			}
		}


	// move chars in fields
		// clear fields left of blocks
		for (col = 0; col < pos; col++) {
			field[current_row][col] = ' ';
		}

		place_blocks();

		// clear fields right of blocks
		for (; col < 7; col++) {
			field[current_row][col] = ' ';
		}


		draw_fields();
		move_blocks();
	}
}
