#!/usr/bin/env python3
# drive.py - run a program under strace inside a pseudo-terminal and press keys for it
# used by experiments 3, 4 and 5 (they need a real terminal, so they cannot use a pipe)
# usage: python3 drive.py exp3|exp4|exp5
import os, pty, re, select, sys, time

ED = "../editor"
SLOW = "./editor_slow"
R = "results"

def run(argv, keys, delay=0.05, idle=0.0):
    """start argv in a pty, send each key with a delay, return after it exits"""
    pid, fd = pty.fork()
    if pid == 0:
        os.execvp(argv[0], argv)
    def drain(t):
        end = time.time() + t
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.05)
            if r:
                try: os.read(fd, 65536)
                except OSError: return
    drain(0.6)
    for k in keys:
        os.write(fd, k)
        drain(delay)
    drain(idle)
    os.write(fd, b"\x11"); drain(0.2)          # Ctrl-Q (twice if file was modified)
    os.write(fd, b"\x11"); drain(0.2)
    os.waitpid(pid, 0)

def strace_calls(path, name):
    """read the 'calls' column for one syscall from a strace -c summary"""
    for line in open(path):
        p = line.split()
        if p and p[-1] == name:
            return int(p[3])
    return 0

def exp3():
    down = [b"\x1b[B"] * 20
    print(f"{'version':<22}{'write() calls':>14}{'per key':>10}")
    for label, binary in [("one write per frame", ED), ("write per piece", SLOW)]:
        out = f"{R}/exp3_{'fast' if binary == ED else 'slow'}.txt"
        run(["strace", "-c", "-e", "trace=write", "-o", out, binary, "data/long.txt"], down)
        c = strace_calls(out, "write")
        print(f"{label:<22}{c:>14}{c / 21:>10.1f}")   # 1 first frame + 20 key presses

def exp4():
    keys = [b"h", b"e", b"l", b"l", b"o"]
    # canonical mode: cat
    out = f"{R}/exp4_cat.txt"
    pid, fd = pty.fork()
    if pid == 0:
        os.execvp("strace", ["strace", "-e", "trace=read", "-o", out, "cat"])
    time.sleep(0.5)
    for k in keys + [b"\r"]:
        os.write(fd, k); time.sleep(0.3)
    os.write(fd, b"\x04"); time.sleep(0.3)      # Ctrl-D = end of input
    try:
        while select.select([fd], [], [], 0.2)[0]: os.read(fd, 65536)
    except OSError: pass
    os.waitpid(pid, 0)
    # raw mode: editor, then stay idle for 2 seconds
    out2 = f"{R}/exp4_editor.txt"
    run(["strace", "-e", "trace=read", "-o", out2, ED, "data/raw.txt"], keys + [b"\r"], delay=0.3, idle=2.0)

    for label, path in [("cat (canonical)", out), ("editor (raw)", out2)]:
        data, empty = [], 0
        for line in open(path):
            m = re.match(r'read\(0, "(.*)", \d+\)\s+= (\d+)', line)
            if m:
                if int(m.group(2)) > 0: data.append((m.group(1), int(m.group(2))))
                else: empty += 1
        print(f"\n{label}: {len(data)} read() calls returned data, {empty} returned 0 (timeout)")
        for s, n in data[:8]:
            print(f"    read(0) -> {n} byte(s): \"{s}\"")

def exp5():
    out = f"{R}/exp5_runcmd.txt"
    keys = [b"\x12"] + [bytes([c]) for c in b"echo hello from child"] + [b"\r"]
    run(["strace", "-f", "-e", "trace=pipe,pipe2,clone,clone3,fork,vfork,execve,dup2,wait4,exit_group",
         "-o", out, ED, "data/run.txt"], keys, delay=0.05, idle=0.8)
    # the first key press after the command leaves the output page (inside run())
    miss = 0
    for line in open(out):
        if "ENOENT" in line:                   # execvp searching PATH for "sh"
            miss += 1
            continue
        if "resumed" in line and "wait4" not in line: continue
        print("   ", line.rstrip()[:110])
    print(f"\n    ({miss} execve attempts returned ENOENT while execvp searched PATH for sh)")

if __name__ == "__main__":
    {"exp3": exp3, "exp4": exp4, "exp5": exp5}[sys.argv[1]]()
