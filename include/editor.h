// editor.h - shared data and function declarations
#ifndef EDITOR_H
#define EDITOR_H

#include <signal.h>
#include <termios.h>

#define CTRL_KEY(k) ((k) & 0x1f)
#define AUTOSAVE_SEC 5               // auto-save interval (seconds)

enum { ARROW_UP = 1000, ARROW_DOWN, ARROW_LEFT, ARROW_RIGHT, DEL_KEY };

typedef struct {
    int len;
    char *s;
} Row;

typedef struct {
    int cx, cy;              // cursor position in the text
    int rowoff, coloff;      // scroll offset
    int rows, cols;          // text area size
    int nrows;               // number of lines
    int dirty;               // modified since last save
    int readonly;            // no permission, or file open in another editor
    Row *row;                // lines of text
    char *filename;
    char msg[80];            // message line at the bottom
    struct termios orig;     // terminal settings before raw mode
    volatile sig_atomic_t resized;       // set by SIGWINCH
    volatile sig_atomic_t autosave_due;  // set by SIGALRM
} Editor;

extern Editor E;

// terminal.c
void die(const char *s);
void raw_on(void);
void get_size(void);
int read_key(void);

// signals.c
void setup_signals(void);

// buffer.c
void row_insert(int at, const char *s, int len);
void row_delete(int at);
void insert_char(int c);
void insert_newline(void);
void del_char(void);

// fileio.c
void open_file(const char *fn);
void save_file(void);
void swap_open(void);
void swap_write(void);
void swap_close(void);

// render.c
void refresh_screen(void);

// input.c
void process_key(void);
char *prompt(const char *fmt);

// search.c
void search(void);

// undo.c
enum { U_INSERT, U_DELETE, U_SPLIT, U_JOIN, U_APPEND };
void undo_begin(void);
void undo_push(int type, int y, int x, int c);
void undo(void);

// runcmd.c
void run_command(void);

#endif