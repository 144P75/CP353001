// readbuf.c - Experiment 1: read a file with different buffer sizes
// usage: ./readbuf FILE BUFSIZE [REPEAT]   (BUFSIZE 0 = whole file in one read)
// prints: bufsize  read_calls  bytes  best_time_ms
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static double now_ms(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s FILE BUFSIZE [REPEAT]\n", argv[0]);
        return 1;
    }
    size_t bs = strtoul(argv[2], NULL, 10);
    int reps = argc > 3 ? atoi(argv[3]) : 3;

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) { perror("open"); return 1; }
    struct stat st;
    fstat(fd, &st);
    if (bs == 0) bs = st.st_size > 0 ? st.st_size : 1;
    char *buf = malloc(bs);

    long calls = 0;
    long long total = 0;
    double best = -1;
    for (int r = 0; r < reps; r++) {
        lseek(fd, 0, SEEK_SET);
        calls = 0;
        total = 0;
        double t0 = now_ms();
        ssize_t n;
        do {
            n = read(fd, buf, bs);
            calls++;
            if (n > 0) total += n;
        } while (n > 0);                        // last read returns 0 (end of file)
        double t = now_ms() - t0;
        if (best < 0 || t < best) best = t;
    }
    printf("%10zu %12ld %12lld %12.2f\n", bs, calls, total, best);
    close(fd);
    free(buf);
    return 0;
}
