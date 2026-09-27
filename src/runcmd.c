// runcmd.c - run a shell command and show its output (Ctrl-R)
// uses fork + pipe + dup2 + execvp + waitpid
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "editor.h"

static char *last;                             

// run cmd in a child process, return everything it printed
static char *run(const char *cmd, int *status, size_t *outlen) {
    int fds[2];                                
    if (pipe(fds) == -1) return NULL;

    pid_t pid = fork();
    if (pid == -1) {
        close(fds[0]);
        close(fds[1]);
        return NULL;
    }
    if (pid == 0) {                             
        int devnull = open("/dev/null", O_RDONLY);
        dup2(devnull, STDIN_FILENO);            
        dup2(fds[1], STDOUT_FILENO);            
        dup2(fds[1], STDERR_FILENO);            
        close(devnull);
        close(fds[0]);
        close(fds[1]);
        char *argv[] = {"sh", "-c", (char *)cmd, NULL};
        execvp("sh", argv);
        _exit(127);                             
    }

    close(fds[1]);                             
    size_t cap = 4096, len = 0;
    char *out = malloc(cap);
    for (;;) {
        ssize_t n = read(fds[0], out + len, cap - len);
        if (n > 0) {
            len += n;
            if (len == cap) out = realloc(out, cap *= 2);
        } else if (n == -1 && errno == EINTR) {
            continue;                           
        } else {
            break;                              
        }
    }
    close(fds[0]);
    while (waitpid(pid, status, 0) == -1 && errno == EINTR);                   
    *outlen = len;
    return out;
}

// full-screen output page, wait for a key
static void show_output(const char *cmd, const char *out, size_t len, int status) {
    size_t cap = len * 2 + 256, n = 0;
    char *s = malloc(cap);
    n += snprintf(s + n, cap - n, "\x1b[2J\x1b[H\x1b[7m $ %s \x1b[m\r\n", cmd);
    for (size_t i = 0; i < len; i++) {
        if (out[i] == '\n') s[n++] = '\r';      
        s[n++] = out[i];
    }
    if (status != -1 && WIFEXITED(status))
        n += snprintf(s + n, cap - n, "\r\n\x1b[7m [exit code %d] press any key \x1b[m",
                      WEXITSTATUS(status));
    else
        n += snprintf(s + n, cap - n, "\r\n\x1b[7m [killed by signal] press any key \x1b[m");
    write(STDOUT_FILENO, s, n);
    free(s);
    while (read_key() == 0)
        ;
    write(STDOUT_FILENO, "\x1b[2J", 4);
}

void run_command(void) {
    char *cmd = prompt("Run: %s   (Enter=run, ESC=cancel)");
    if (cmd == NULL) return;
    if (cmd[0] == '\0') {                       
        if (last == NULL) {
            snprintf(E.msg, sizeof E.msg, "No previous command");
            return;
        }
    } else {
        free(last);
        last = cmd;
    }
    if (E.dirty) save_file();                  

    int status = -1;
    size_t len = 0;
    char *out = run(last, &status, &len);
    if (out == NULL) {
        snprintf(E.msg, sizeof E.msg, "Run failed: %s", strerror(errno));
        return;
    }
    show_output(last, out, len, status);
    free(out);
    snprintf(E.msg, sizeof E.msg, "Ran: %s", last);
}