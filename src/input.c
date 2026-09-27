// input.c - map keys to editor actions
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "editor.h"

static void move_cursor(int k) {
    Row *r = E.cy < E.nrows ? &E.row[E.cy] : NULL;
    switch (k) {
    case ARROW_LEFT:
        if (E.cx > 0) E.cx--;
        else if (E.cy > 0) { E.cy--; E.cx = E.row[E.cy].len; }
        break;
    case ARROW_RIGHT:
        if (r && E.cx < r->len) E.cx++;
        else if (r) { E.cy++; E.cx = 0; }
        break;
    case ARROW_UP:
        if (E.cy > 0) E.cy--;
        break;
    case ARROW_DOWN:
        if (E.cy < E.nrows) E.cy++;
        break;
    }
    int len = E.cy < E.nrows ? E.row[E.cy].len : 0;
    if (E.cx > len) E.cx = len;
}

// show a question on the message line and let the user type an answer
// returns the typed text (caller must free), or NULL if ESC was pressed
char *prompt(const char *fmt) {
    size_t cap = 64, len = 0;
    char *buf = malloc(cap);
    buf[0] = '\0';
    for (;;) {
        snprintf(E.msg, sizeof E.msg, fmt, buf);
        refresh_screen();
        int c = read_key();
        if (c == '\x1b') {                     // ESC: cancel
            E.msg[0] = '\0';
            free(buf);
            return NULL;
        }
        if (c == '\r') {                       // Enter: done
            E.msg[0] = '\0';
            return buf;
        }
        if (c == 127 || c == CTRL_KEY('h')) {
            if (len > 0) buf[--len] = '\0';
        } else if (c > 0 && c < 128 && isprint(c)) {
            if (len + 1 >= cap) buf = realloc(buf, cap *= 2);
            buf[len++] = c;
            buf[len] = '\0';
        }
    }
}

void process_key(void) {
    static int quit_warned = 0;
    int c = read_key();
    undo_begin();                               // one key press = one undo step

    switch (c) {
    case 0:
        return;
    case CTRL_KEY('q'):
        if (E.dirty && !quit_warned) {
            snprintf(E.msg, sizeof E.msg, "Unsaved changes! Ctrl-Q again to quit.");
            quit_warned = 1;
            return;
        }
        swap_close();
        exit(0);
    case CTRL_KEY('s'):
        save_file();
        break;
    case CTRL_KEY('f'):
        search();
        break;
    case CTRL_KEY('z'):
        undo();
        break;
    case CTRL_KEY('r'):
        run_command();
        break;
    case '\r':
        insert_newline();
        break;
    case 127:
    case CTRL_KEY('h'):
        del_char();
        break;
    case DEL_KEY:
        if (E.cy < E.nrows && !(E.cy == E.nrows - 1 && E.cx == E.row[E.cy].len)) {
            move_cursor(ARROW_RIGHT);
            del_char();
        }
        break;
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
        move_cursor(c);
        break;
    default:
        if (c == '\t' || (c < 128 && isprint(c))) insert_char(c);   // ASCII + Tab   
    }
    quit_warned = 0;
}