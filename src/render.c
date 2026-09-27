// render.c - draw the screen with ANSI escape codes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "editor.h"

#define OBS(s) ob_add(s, sizeof(s) - 1)
#define TAB_STOP 4     

static char *ob;       
static int oblen;
static int rx;         

static void ob_add(const char *s, int len) {
    ob = realloc(ob, oblen + len);
    memcpy(ob + oblen, s, len);
    oblen += len;
}

// character index -> screen column
static int cx_to_rx(const Row *r, int cx) {
    int col = 0;
    for (int i = 0; i < cx; i++) {
        if (r->s[i] == '\t') col += TAB_STOP - (col % TAB_STOP);
        else col++;
    }
    return col;
}

static void scroll(void) {
    rx = E.cy < E.nrows ? cx_to_rx(&E.row[E.cy], E.cx) : 0;
    if (E.cy < E.rowoff) E.rowoff = E.cy;
    if (E.cy >= E.rowoff + E.rows) E.rowoff = E.cy - E.rows + 1;
    if (rx < E.coloff) E.coloff = rx;
    if (rx >= E.coloff + E.cols) E.coloff = rx - E.cols + 1;
}

// draw one line, expanding tabs to spaces, only the visible part
static void draw_line(const Row *r) {
    int col = 0, end = E.coloff + E.cols;
    for (int i = 0; i < r->len && col < end; i++) {
        if (r->s[i] == '\t') {
            do {
                if (col >= E.coloff) OBS(" ");
                col++;
            } while (col % TAB_STOP != 0 && col < end);
        } else {
            if (col >= E.coloff) ob_add(&r->s[i], 1);
            col++;
        }
    }
}

static void draw_rows(void) {
    for (int y = 0; y < E.rows; y++) {
        int fr = y + E.rowoff;
        if (fr < E.nrows) {
            draw_line(&E.row[fr]);
        } else {
            OBS("~");
        }
        OBS("\x1b[K\r\n");                      
    }
}

static void draw_status(void) {
    char st[160];
    int n = snprintf(st, sizeof st, " %s%s%s | %d lines | Ln %d, Col %d ",
                     E.filename, E.dirty ? " (modified)" : "",
                     E.readonly ? " [read-only]" : "",
                     E.nrows, E.cy + 1, rx + 1);
    if (n > (int)sizeof st - 1) n = sizeof st - 1;
    if (n > E.cols) n = E.cols;
    OBS("\x1b[7m");                            
    ob_add(st, n);
    while (n++ < E.cols) OBS(" ");
    OBS("\x1b[m\r\n\x1b[K");
    int ml = strlen(E.msg);
    ob_add(E.msg, ml > E.cols ? E.cols : ml);
}

void refresh_screen(void) {
    scroll();
    oblen = 0;
    OBS("\x1b[?25l\x1b[H");                    
    draw_rows();
    draw_status();
    char cur[32];
    int n = snprintf(cur, sizeof cur, "\x1b[%d;%dH",
                     E.cy - E.rowoff + 1, rx - E.coloff + 1);
    ob_add(cur, n);
    OBS("\x1b[?25h");                           
    write(STDOUT_FILENO, ob, oblen);            
}