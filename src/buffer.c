// buffer.c - text storage: insert and delete characters and lines
#include <stdlib.h>
#include <string.h>
#include "editor.h"

void row_insert(int at, const char *s, int len) {
    E.row = realloc(E.row, sizeof(Row) * (E.nrows + 1));
    memmove(&E.row[at + 1], &E.row[at], sizeof(Row) * (E.nrows - at));
    E.row[at].s = malloc(len + 1);
    memcpy(E.row[at].s, s, len);
    E.row[at].s[len] = 0;
    E.row[at].len = len;
    E.nrows++;
    E.dirty = 1;
}

void row_delete(int at) {
    free(E.row[at].s);
    memmove(&E.row[at], &E.row[at + 1], sizeof(Row) * (E.nrows - at - 1));
    E.nrows--;
    E.dirty = 1;
}

void insert_char(int c) {
    if (E.cy == E.nrows) {
        row_insert(E.nrows, "", 0);
        undo_push(U_APPEND, E.cy, 0, 0);
    }
    undo_push(U_INSERT, E.cy, E.cx, c);
    Row *r = &E.row[E.cy];
    r->s = realloc(r->s, r->len + 2);
    memmove(&r->s[E.cx + 1], &r->s[E.cx], r->len - E.cx + 1);
    r->s[E.cx++] = c;
    r->len++;
    E.dirty = 1;
}

void insert_newline(void) {
    if (E.cy == E.nrows) {
        row_insert(E.nrows, "", 0);
        undo_push(U_APPEND, E.cy, 0, 0);
        E.cy++;
        E.cx = 0;
        return;
    }
    undo_push(U_SPLIT, E.cy, E.cx, 0);
    Row *r = &E.row[E.cy];
    row_insert(E.cy + 1, r->s + E.cx, r->len - E.cx);
    r = &E.row[E.cy];                           
    r->len = E.cx;
    r->s[r->len] = 0;
    E.cy++;
    E.cx = 0;
}

void del_char(void) {
    if (E.cy == E.nrows || (E.cx == 0 && E.cy == 0)) return;
    Row *r = &E.row[E.cy];
    if (E.cx > 0) {
        undo_push(U_DELETE, E.cy, E.cx - 1, r->s[E.cx - 1]);
        memmove(&r->s[E.cx - 1], &r->s[E.cx], r->len - E.cx + 1);
        r->len--;
        E.cx--;
    } else {                                    
        Row *p = &E.row[E.cy - 1];
        undo_push(U_JOIN, E.cy, p->len, 0);
        E.cx = p->len;
        p->s = realloc(p->s, p->len + r->len + 1);
        memcpy(&p->s[p->len], r->s, r->len + 1);
        p->len += r->len;
        row_delete(E.cy);
        E.cy--;
    }
    E.dirty = 1;
}