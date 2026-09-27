// signals.c - signal handlers (only set flags; main loop does the work)
#include <string.h>
#include "editor.h"

static void on_winch(int sig) {                 // window resized
    (void)sig;
    E.resized = 1;
}

static void on_alarm(int sig) {                 // auto-save timer
    (void)sig;
    E.autosave_due = 1;
}

void setup_signals(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_winch;
    sigaction(SIGWINCH, &sa, NULL);
    sa.sa_handler = on_alarm;
    sigaction(SIGALRM, &sa, NULL);
}