// terminal.c - raw mode, alternate screen, window size, key input
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "editor.h"

static void raw_off(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig);
    write(STDOUT_FILENO, "\x1b[?1049l", 8);    // leave alternate screen
}

void die(const char *s) {
    perror(s);
    exit(1);
}

void raw_on(void) {
    if (tcgetattr(STDIN_FILENO, &E.orig) == -1) die("tcgetattr");
    atexit(raw_off);                            // always restore terminal
    struct termios t = E.orig;
    t.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    t.c_oflag &= ~OPOST;
    t.c_cflag |= CS8;
    t.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); // no echo, no line buffer, no Ctrl+C
    t.c_cc[VMIN] = 0;                           // read() returns after timeout
    t.c_cc[VTIME] = 1;                          // 100 ms
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &t) == -1) die("tcsetattr");
    write(STDOUT_FILENO, "\x1b[?1049h", 8);    // enter alternate screen
}

void get_size(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        E.rows = 24;
        E.cols = 80;
    } else {
        E.rows = ws.ws_row;
        E.cols = ws.ws_col;
    }
    E.rows -= 2;                                // status bar + message line
}

int read_key(void) {
    char c;
    int n;
    while ((n = read(STDIN_FILENO, &c, 1)) != 1) {
        if (n == -1 && errno != EAGAIN && errno != EINTR) die("read");
        if (E.resized || E.autosave_due) return 0;
    }
    if (c != '\x1b') return c;

    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
    if (seq[0] == '[') {
        if (seq[1] == '3' && read(STDIN_FILENO, &seq[2], 1) == 1) return DEL_KEY;
        switch (seq[1]) {
        case 'A': return ARROW_UP;
        case 'B': return ARROW_DOWN;
        case 'C': return ARROW_RIGHT;
        case 'D': return ARROW_LEFT;
        }
    }
    return '\x1b';
}