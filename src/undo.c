// undo.c - undo history (Ctrl-Z)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "editor.h"

typedef struct {
    int type;       
    int y, x;       
    int c;          
    int group;      
} UndoOp;

static UndoOp *ops;
static int nops, cap, group;


void undo_begin(void) {
    group++;
}

void undo_push(int type, int y, int x, int c) {
    if (nops == cap) {
        cap = cap ? cap * 2 : 64;
        ops = realloc(ops, sizeof(UndoOp) * cap);
    }
    ops[nops++] = (UndoOp){type, y, x, c, group};
}

static void undo_one(UndoOp o) {
    Row *r;
    switch (o.type) {
    case U_INSERT:                             
        r = &E.row[o.y];
        memmove(&r->s[o.x], &r->s[o.x + 1], r->len - o.x);
        r->len--;
        E.cy = o.y; E.cx = o.x;
        break;
    case U_DELETE:                              
        r = &E.row[o.y];
        r->s = realloc(r->s, r->len + 2);
        memmove(&r->s[o.x + 1], &r->s[o.x], r->len - o.x + 1);
        r->s[o.x] = o.c;
        r->len++;
        E.cy = o.y; E.cx = o.x + 1;
        break;
    case U_SPLIT: {                             
        Row *a = &E.row[o.y], *b = &E.row[o.y + 1];
        a->s = realloc(a->s, a->len + b->len + 1);
        memcpy(&a->s[a->len], b->s, b->len + 1);
        a->len += b->len;
        row_delete(o.y + 1);
        E.cy = o.y; E.cx = o.x;
        break;
    }
    case U_JOIN: {                             
        Row *a = &E.row[o.y - 1];
        row_insert(o.y, a->s + o.x, a->len - o.x);
        a = &E.row[o.y - 1];                    
        a->len = o.x;
        a->s[o.x] = '\0';
        E.cy = o.y; E.cx = 0;
        break;
    }
    case U_APPEND:                              
        row_delete(o.y);
        E.cy = o.y; E.cx = 0;
        break;
    }
}

void undo(void) {
    if (nops == 0) {
        snprintf(E.msg, sizeof E.msg, "Nothing to undo");
        return;
    }
    int g = ops[nops - 1].group;
    while (nops > 0 && ops[nops - 1].group == g) undo_one(ops[--nops]);
    E.dirty = 1;
    snprintf(E.msg, sizeof E.msg, "Undo (%d steps left)", nops);
}