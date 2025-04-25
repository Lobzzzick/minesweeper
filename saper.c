#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ROWS 10       // Number of rows in the board
#define COLS 10       // Number of columns in the board
#define NUM_MINES 1   // Total number of mines on the board

// Structure to hold a cursor or position (row, col)
typedef struct {
    int row;
    int col;
} Position;

// Structure for each cell on the board
// Tracks if it's a mine, revealed, flagged, and neighbor mine count
typedef struct {
    int is_mine;
    int is_revealed;
    int is_flagged;
    int neighbor_mines;
} Cell;

// The game board: grid of cells and game state info
typedef struct {
    Cell cells[ROWS][COLS];    // 2D array of cells
    int mines_remaining;        // Mines not yet flagged
    int flags_remaining;        // Flags the player can still place
    int game_over;             // Flag for game over (loss)
    Position cursor_position;  // Current cursor position
} GameBoard;

// Initialize all cells and game state
void init_board(GameBoard *board) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            board->cells[i][j].is_mine = 0;
            board->cells[i][j].is_revealed = 0;
            board->cells[i][j].is_flagged = 0;
            board->cells[i][j].neighbor_mines = 0;
        }
    }
    board->mines_remaining = NUM_MINES;
    board->flags_remaining = NUM_MINES;
    board->game_over = 0;
    board->cursor_position.row = 0;
    board->cursor_position.col = 0;
}

// Randomly place mines and update neighbor counts
void place_mines(GameBoard *board) {
    int count = 0;
    srand(time(NULL));  // Seed random number generator

    while (count < NUM_MINES) {
        int i = rand() % ROWS;
        int j = rand() % COLS;

        if (!board->cells[i][j].is_mine) {
            board->cells[i][j].is_mine = 1;
            count++;

            // Increment neighbor mine counts around this mine
            for (int x = i - 1; x <= i + 1; x++) {
                for (int y = j - 1; y <= j + 1; y++) {
                    if (x >= 0 && x < ROWS && y >= 0 && y < COLS) {
                        board->cells[x][y].neighbor_mines++;
                    }
                }
            }
        }
    }
}

// Reveal a cell; if it's empty (0 neighbors), flood-fill adjacent cells
void reveal_cell(GameBoard *board, int row, int col) {
    // Check bounds and if already revealed or flagged
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS ||
        board->cells[row][col].is_revealed ||
        board->cells[row][col].is_flagged)
        return;

    board->cells[row][col].is_revealed = 1;

    // If it's a mine, game over
    if (board->cells[row][col].is_mine) {
        board->game_over = 1;
        return;
    }

    // If no neighbor mines, reveal all neighbors recursively
    if (board->cells[row][col].neighbor_mines == 0) {
        for (int i = row - 1; i <= row + 1; i++) {
            for (int j = col - 1; j <= col + 1; j++) {
                reveal_cell(board, i, j);
            }
        }
    }
}

// Toggle a flag on a cell if it's not revealed
void toggle_flag(GameBoard *board, int row, int col) {
    if (row >= 0 && row < ROWS && col >= 0 && col < COLS &&
        !board->cells[row][col].is_revealed) {
        if (!board->cells[row][col].is_flagged) {
            board->cells[row][col].is_flagged = 1;
            board->flags_remaining--;
        } else {
            board->cells[row][col].is_flagged = 0;
            board->flags_remaining++;
        }
    }
}

// Check if all mines are flagged; if so, player wins with fancy message
void check_win(GameBoard *board) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (board->cells[i][j].is_mine && !board->cells[i][j].is_flagged) {
                return;  // Still mines left to flag
            }
        }
    }

    clear();
    const char *msg1 = "CONGRATULATIONS!";
    const char *msg2 = "YOU WIN!";
    int max_msg = strlen(msg1) > strlen(msg2) ? strlen(msg1) : strlen(msg2);
    int box_w = max_msg + 4;
    int scr_w = COLS * 2;
    int sx = (scr_w - box_w) / 2;
    int sy = ROWS / 2 - 2;

    attron(COLOR_PAIR(2)); // Green text
    // Top border
    mvaddch(sy, sx, '+');
    for (int i = 0; i < box_w - 2; i++) addch('-');
    addch('+');
    // Message 1
    mvprintw(sy + 1, sx, "| %*s |", max_msg, msg1);
    // Message 2
    mvprintw(sy + 2, sx, "| %*s |", max_msg, msg2);
    // Bottom border
    mvaddch(sy + 3, sx, '+');
    for (int i = 0; i < box_w - 2; i++) addch('-');
    addch('+');
    attroff(COLOR_PAIR(2));

    refresh();
    getch();
    endwin();
    exit(0);
}

// Draw the entire board to the screen
void draw_board(GameBoard *board) {
    clear();
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (board->cursor_position.row == i &&
                board->cursor_position.col == j) {
                attron(A_REVERSE);
            }

            if (board->cells[i][j].is_revealed) {
                if (board->cells[i][j].is_mine) {
                    printw("* ");
                } else {
                    int cp = board->cells[i][j].neighbor_mines + 1;
                    attron(COLOR_PAIR(cp));
                    printw("%d ", board->cells[i][j].neighbor_mines);
                    attroff(COLOR_PAIR(cp));
                }
            } else {
                if (board->cells[i][j].is_flagged) {
                    attron(COLOR_PAIR(10));
                    printw("! ");
                    attroff(COLOR_PAIR(10));
                } else {
                    printw(". ");
                }
            }

            if (board->cursor_position.row == i &&
                board->cursor_position.col == j) {
                attroff(A_REVERSE);
            }
        }
        printw("\n");
    }
    printw("Mines remaining: %d | Flags remaining: %d\n",
           board->mines_remaining, board->flags_remaining);
    refresh();
}

int main() {
    // Initialize ncurses
    initscr();
    start_color();
    init_pair(0, COLOR_CYAN, COLOR_BLACK);
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(5, COLOR_RED, COLOR_BLACK);
    init_pair(6, COLOR_RED, COLOR_BLACK);
    init_pair(7, COLOR_GREEN, COLOR_BLACK);
    init_pair(8, COLOR_CYAN, COLOR_BLACK);
    init_pair(9, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(10, COLOR_RED, COLOR_BLACK);

    noecho();             // Don't echo input
    keypad(stdscr, TRUE); // Enable arrow keys
    curs_set(0);          // Hide default cursor

    GameBoard board;
    init_board(&board);
    place_mines(&board);

    int ch;
    while ((ch = getch()) != 'q' && !board.game_over) {
        switch (ch) {
            case KEY_UP:
                board.cursor_position.row = (board.cursor_position.row - 1 + ROWS) % ROWS;
                break;
            case KEY_DOWN:
                board.cursor_position.row = (board.cursor_position.row + 1) % ROWS;
                break;
            case KEY_LEFT:
                board.cursor_position.col = (board.cursor_position.col - 1 + COLS) % COLS;
                break;
            case KEY_RIGHT:
                board.cursor_position.col = (board.cursor_position.col + 1) % COLS;
                break;
            case ' ':
                reveal_cell(&board, board.cursor_position.row, board.cursor_position.col);
                break;
            case 'h':
                toggle_flag(&board, board.cursor_position.row, board.cursor_position.col);
                check_win(&board);
                break;
        }
        draw_board(&board);
    }

    // Handle game over (loss) message
    if (board.game_over) {
        clear();
        attron(COLOR_PAIR(10));
        int width = COLS * 2;
        mvprintw(ROWS / 2, (width - strlen("Game Over!")) / 2, "Game Over!");
        mvprintw(ROWS / 2 + 1, (width - strlen("You hit a mine!")) / 2, "You hit a mine!");
        attroff(COLOR_PAIR(10));
        refresh();
        getch();
    }

    endwin();
    return 0;
}
