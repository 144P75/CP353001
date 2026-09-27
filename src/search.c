// search.c - find text (Ctrl-F)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "editor.h"

static char *last;                              // last search word

void search(void) {
    char *q = prompt("Search: %s   (Enter=find, ESC=cancel)");
    if (q == NULL) return;                      // cancelled
    int next = 0;
    if (q[0] == '\0') {                         // empty -> find next of last word
        free(q);
        if (last == NULL) return;
        next = 1;
    } else {
        free(last);
        last = q;
    }
    if (E.nrows == 0) {
        snprintf(E.msg, sizeof E.msg, "Not found: %s", last);
        return;
    }


    int sy = E.cy, sx = E.cx + next;
    if (sy >= E.nrows) { sy = 0; sx = 0; }
    for (int i = 0; i <= E.nrows; i++) {
        int y = (sy + i) % E.nrows;
        int start = (i == 0) ? sx : 0;
        if (start > E.row[y].len) continue;
        char *m = strstr(E.row[y].s + start, last);
        if (m) {
            E.cy = y;
            E.cx = m - E.row[y].s;
            snprintf(E.msg, sizeof E.msg, "Found \"%s\" at Ln %d", last, y + 1);
            return;
        }
    }
    snprintf(E.msg, sizeof E.msg, "Not found: %s", last);
}