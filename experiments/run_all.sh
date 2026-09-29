#!/bin/bash
# run_all.sh - build and run all experiments, save results in results/
# usage (inside experiments/):  bash run_all.sh
# every result is also written to results/summary.txt
set -e
cd "$(dirname "$0")"
mkdir -p data results
exec > >(tee results/summary.txt) 2>&1

echo "== build =="
make -s -C ..
gcc -Wall -Wextra readbuf.c -o readbuf
gcc -Wall -Wextra writemode.c -o writemode
gcc -Wall -Wextra -D_DEFAULT_SOURCE -DSLOW_RENDER -I../include ../src/*.c -o editor_slow
echo "date: $(date)"
echo "kernel: $(uname -r)"
echo "project dir file system: $(df -T . | awk 'NR==2 {print $2}')"
echo "/tmp file system:         $(df -T /tmp | awk 'NR==2 {print $2}')"

# ------------------------------------------------------------
echo
echo "== Experiment 1: read() buffer size (1 MB file) =="
echo "(bufsize 1 on /mnt/c can take up to a minute, please wait)"
head -c 1048576 /dev/urandom > data/1mb.bin
TMPF=/tmp/tte_1mb.bin
cp data/1mb.bin $TMPF
for place in "data/1mb.bin" "$TMPF"; do
    echo
    echo "file: $place"
    printf "%10s %12s %12s %12s\n" "bufsize" "read_calls" "bytes" "best_ms"
    for bs in 1 16 256 4096 65536 0; do
        ./readbuf "$place" $bs 3
    done
done
echo "(bufsize 0 = whole file with one read, size taken from fstat)"
echo
echo "check with strace (bufsize 4096):"
strace -c -e trace=read -o results/exp1_strace_4096.txt ./readbuf data/1mb.bin 4096 1 > /dev/null
grep -E "calls|read" results/exp1_strace_4096.txt
rm -f $TMPF

# ------------------------------------------------------------
echo
echo "== Experiment 2: ways of saving a file (100 KB, 50 saves each) =="
printf "%-13s %8s %8s %12s\n" "mode" "size_kb" "repeat" "avg_ms"
for m in direct small direct_fsync safe; do
    ./writemode $m data/save.txt 100 50
done
echo
echo "system calls for ONE save:"
for m in direct small direct_fsync safe; do
    strace -e trace=openat,write,fsync,close,rename -o results/exp2_$m.txt \
        ./writemode $m data/save.txt 100 1 > /dev/null
    echo "--- $m"
    # count calls from the moment save.txt is opened (skip program start-up and stdout)
    awk '/save\.txt/ {on=1} on && /^[a-z]/ && !/^write\(1,/ {split($0, a, "("); n[a[1]]++}
         END {for (k in n) printf "    %-7s %d\n", k, n[k]}' results/exp2_$m.txt | sort
done

echo
echo "crash test: old file = 100 KB of 'A', new content = 'B', process killed half way"
for m in direct_crash safe_crash; do
    head -c 102400 /dev/zero | tr '\0' 'A' > data/crash.txt
    rm -f data/crash.txt.tmp
    (./writemode $m data/crash.txt 100 1 || true) &> /dev/null
    size=$(stat -c %s data/crash.txt)
    first=$(head -c 1 data/crash.txt)
    echo "--- $m: file size = $size bytes, first byte = '$first'"
    if [ "$first" = "A" ] && [ "$size" = "102400" ]; then
        echo "    old file is still complete (safe)"
    else
        echo "    old file is DAMAGED: old data lost, new data only half written"
    fi
done
rm -f data/crash.txt.tmp

# ------------------------------------------------------------
echo
echo "== Experiment 3: drawing the screen (20 x arrow-down in a 300-line file) =="
seq 1 300 > data/long.txt
python3 drive.py exp3

# ------------------------------------------------------------
echo
echo "== Experiment 4: canonical mode (cat) vs raw mode (editor), typing: hello + Enter =="
echo "raw mode test" > data/raw.txt
python3 drive.py exp4

# ------------------------------------------------------------
echo
echo "== Experiment 5: Run command (Ctrl-R: echo hello from child) =="
echo "run test" > data/run.txt
python3 drive.py exp5

# ------------------------------------------------------------
echo
echo "== Overall: every system call in a short editing session =="
cp data/raw.txt data/session.txt
python3 - <<'PY'
import drive
drive.run(["strace", "-c", "-o", "results/overall.txt", "../editor", "data/session.txt"],
          [b"a", b"b", b"c", b"\r", b"\x13"], delay=0.1)
PY
head -40 results/overall.txt

echo
echo "done. full traces are in experiments/results/"
