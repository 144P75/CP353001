// writemode.c - Experiment 2: compare ways of saving a file
// usage: ./writemode MODE FILE SIZE_KB REPEAT
// MODE:  direct        open(O_TRUNC) + write + close
//        direct_fsync  direct + fsync
//        safe          write temp file + fsync + rename   (what the editor does)
//        small         open(O_TRUNC) + write 64 bytes at a time + close
//        direct_crash  direct save, killed half way through writing
//        safe_crash    safe save, killed half way through writing
// prints: mode  size_kb  repeat  avg_ms_per_save
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static double now_ms(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

static void crash(void) {                       // simulate power loss / crash
    kill(getpid(), SIGKILL);
}

static void save_direct(const char *fn, const char *buf, size_t n, int sync, int die) {
    int fd = open(fn, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (die) {
        write(fd, buf, n / 2);
        crash();
    }
    write(fd, buf, n);
    if (sync) fsync(fd);
    close(fd);
}

static void save_small(const char *fn, const char *buf, size_t n) {
    int fd = open(fn, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    for (size_t off = 0; off < n; off += 64)
        write(fd, buf + off, n - off < 64 ? n - off : 64);
    close(fd);
}

static void save_safe(const char *fn, const char *buf, size_t n, int die) {
    char tmp[512];
    snprintf(tmp, sizeof tmp, "%s.tmp", fn);
    int fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (die) {
        write(fd, buf, n / 2);
        crash();
    }
    write(fd, buf, n);
    fsync(fd);
    close(fd);
    rename(tmp, fn);
}

int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "usage: %s MODE FILE SIZE_KB REPEAT\n", argv[0]);
        return 1;
    }
    const char *mode = argv[1], *fn = argv[2];
    size_t n = strtoul(argv[3], NULL, 10) * 1024;
    int reps = atoi(argv[4]);

    char *buf = malloc(n);                      // new content: lines of 'B'
    for (size_t i = 0; i < n; i++) buf[i] = (i % 64 == 63) ? '\n' : 'B';

    double t0 = now_ms();
    for (int r = 0; r < reps; r++) {
        if      (!strcmp(mode, "direct"))       save_direct(fn, buf, n, 0, 0);
        else if (!strcmp(mode, "direct_fsync")) save_direct(fn, buf, n, 1, 0);
        else if (!strcmp(mode, "safe"))         save_safe(fn, buf, n, 0);
        else if (!strcmp(mode, "small"))        save_small(fn, buf, n);
        else if (!strcmp(mode, "direct_crash")) save_direct(fn, buf, n, 0, 1);
        else if (!strcmp(mode, "safe_crash"))   save_safe(fn, buf, n, 1);
        else { fprintf(stderr, "unknown mode\n"); return 1; }
    }
    printf("%-13s %8s %8d %12.3f\n", mode, argv[3], reps, (now_ms() - t0) / reps);
    free(buf);
    return 0;
}
