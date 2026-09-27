// fileio.c - open, safe save, read-only check, lock + auto-save swap file
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include "editor.h"

static int swapfd = -1;                         // .name.swp (lock + auto-save)
static char swappath[512];

/* ---------- helpers ---------- */

// join all lines into one block of text
static char *build_text(int *total) {
    int t = 0;
    for (int i = 0; i < E.nrows; i++) t += E.row[i].len + 1;
    char *buf = malloc(t + 1), *p = buf;
    for (int i = 0; i < E.nrows; i++) {
        memcpy(p, E.row[i].s, E.row[i].len);
        p += E.row[i].len;
        *p++ = '\n';
    }
    *total = t;
    return buf;
}

// read a whole file descriptor into the editor (replaces current text)
static void load_fd(int fd) {
    for (int i = 0; i < E.nrows; i++) free(E.row[i].s);
    E.nrows = 0;
    E.cx = E.cy = 0;

    struct stat st;
    fstat(fd, &st);
    char *buf = malloc(st.st_size + 1);
    ssize_t n = 0, r;
    while (n < st.st_size) {
        r = pread(fd, buf + n, st.st_size - n, n);
        if (r > 0) n += r;
        else if (r == -1 && errno == EINTR) continue;
        else break;
    }
    char *p = buf, *end = buf + n;
    while (p < end) {
        char *nl = memchr(p, '\n', end - p);
        int len = nl ? nl - p : end - p;
        if (len && p[len - 1] == '\r') len--;
        row_insert(E.nrows, p, len);
        p = nl ? nl + 1 : end;
    }
    free(buf);
}

/* ---------- open ---------- */

void open_file(const char *fn) {
    E.filename = strdup(fn);
    int fd = open(fn, O_RDONLY);
    if (fd == -1) {
        if (errno == ENOENT) return;           
        die("open");
    }
    load_fd(fd);
    close(fd);
    E.dirty = 0;
    if (access(fn, W_OK) != 0) {                
        E.readonly = 1;
        snprintf(E.msg, sizeof E.msg, "No write permission: read-only");
    }
}

/* ---------- safe save ---------- */

void save_file(void) {
    if (E.readonly) {
        snprintf(E.msg, sizeof E.msg, "Read-only: cannot save");
        return;
    }
    int total;
    char *buf = build_text(&total);

    char tmp[512];
    snprintf(tmp, sizeof tmp, "%s.tmp", E.filename);

    mode_t mode = 0644;                         
    struct stat st;
    if (stat(E.filename, &st) == 0) mode = st.st_mode & 0777;

    // write to a temp file, force it to disk, then atomically replace the old file
    int fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, mode);
    if (fd == -1) goto fail;
    if (write(fd, buf, total) != total) { close(fd); goto fail; }
    if (fsync(fd) == -1) { close(fd); goto fail; }
    if (close(fd) == -1) goto fail;
    if (rename(tmp, E.filename) == -1) goto fail;

    E.dirty = 0;
    if (swapfd != -1) ftruncate(swapfd, 0);     
    snprintf(E.msg, sizeof E.msg, "Saved %d bytes to %s", total, E.filename);
    free(buf);
    return;

fail:
    snprintf(E.msg, sizeof E.msg, "Save failed: %s", strerror(errno));
    unlink(tmp);
    free(buf);
}

/* ---------- swap file: lock + auto-save + recovery ---------- */

void swap_open(void) {
    if (E.readonly) return;

    // "dir/name.txt" -> "dir/.name.txt.swp"
    const char *slash = strrchr(E.filename, '/');
    if (slash)
        snprintf(swappath, sizeof swappath, "%.*s/.%s.swp",
                 (int)(slash - E.filename), E.filename, slash + 1);
    else
        snprintf(swappath, sizeof swappath, ".%s.swp", E.filename);

    swapfd = open(swappath, O_RDWR | O_CREAT, 0644);
    if (swapfd == -1) return;                   // cannot create: run without it

    // only one editor may hold the lock
    if (flock(swapfd, LOCK_EX | LOCK_NB) == -1) {
        close(swapfd);
        swapfd = -1;
        E.readonly = 1;
        snprintf(E.msg, sizeof E.msg, "File is open in another editor: read-only");
        return;
    }

    // swap file has text -> last session crashed before saving
    struct stat st;
    if (fstat(swapfd, &st) == 0 && st.st_size > 0) {
        char *ans = prompt("Unsaved changes found. Recover? (y/n): %s");
        if (ans && (ans[0] == 'y' || ans[0] == 'Y')) {
            load_fd(swapfd);
            E.dirty = 1;
            snprintf(E.msg, sizeof E.msg, "Recovered. Press ^S to save.");
        } else {
            ftruncate(swapfd, 0);
        }
        free(ans);
    }
}

void swap_write(void) {
    if (swapfd == -1) return;
    if (!E.dirty) {                             
        ftruncate(swapfd, 0);
        return;
    }
    int total;
    char *buf = build_text(&total);
    ftruncate(swapfd, 0);
    pwrite(swapfd, buf, total, 0);
    free(buf);
}

void swap_close(void) {
    if (swapfd == -1) return;
    unlink(swappath);                           
    close(swapfd);                              
    swapfd = -1;
}