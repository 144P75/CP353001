// main.c - entry point and main loop
#include <stdio.h>
#include <unistd.h>
#include "editor.h"

Editor E;

int main(int argc, char **argv) {
    raw_on();
    get_size();
    setup_signals();
    snprintf(E.msg, sizeof E.msg, "^S save  ^F find  ^Z undo  ^R run  ^Q quit");
    open_file(argc >= 2 ? argv[1] : "untitled.txt");
    swap_open();
    alarm(AUTOSAVE_SEC);                        // start auto-save timer

    for (;;) {
        if (E.resized) {
            E.resized = 0;
            get_size();
        }
        if (E.autosave_due) {
            E.autosave_due = 0;
            swap_write();
            alarm(AUTOSAVE_SEC);                // set the next one
        }
        refresh_screen();
        process_key();
    }
}