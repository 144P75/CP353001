# การทดสอบ Terminal Text Editor

เอกสารนี้อธิบายวิธีทดสอบแต่ละฟีเจอร์ พร้อมผลที่คาดหวัง รันทุกคำสั่งจากโฟลเดอร์ `terminal-text-editor`

## เตรียมก่อนทดสอบ

```bash
make
printf "apple\nbanana\napple pie\n" > tests/search.txt
printf '#include <stdio.h>\nint main(void) {\n\tprintf("Hello OS\\n");\n\treturn 0;\n}\n' > tests/hello.c
echo "Hello Mini Project" > tests/basic.txt
seq 1 100 > tests/long.txt
```

> แก้โค้ดทุกครั้งต้องรัน `make` ก่อนทดสอบ

---

## 1. การแก้ไขพื้นฐานและบันทึกไฟล์

```bash
./editor tests/basic.txt
```

| ขั้นตอน | ผลที่คาดหวัง |
|---|---|
| พิมพ์ข้อความ, Backspace, Enter, ลูกศร | แก้ไขได้ทันที แถบสถานะขึ้น `(modified)` |
| Ctrl+S | ขึ้น `Saved ... bytes to tests/basic.txt` |
| Ctrl+Q แล้ว `cat tests/basic.txt` | เห็นข้อความที่แก้ไข |

**System Calls:** `tcsetattr`, `read`, `write`, `open`, `fsync`, `rename`

## 2. ปรับขนาดหน้าต่าง

```bash
./editor tests/long.txt
```

| ขั้นตอน | ผลที่คาดหวัง |
|---|---|
| ย่อหน้าต่าง | แสดงบรรทัดน้อยลง แถบสถานะอยู่ขอบล่าง |
| ขยายหน้าต่าง | แสดงบรรทัดมากขึ้น แถบสถานะอยู่ขอบล่าง |

**System Calls:** `sigaction` (`SIGWINCH`), `ioctl`

## 3. ค้นหาข้อความ (Search)

```bash
./editor tests/search.txt
```

| ขั้นตอน | ผลที่คาดหวัง |
|---|---|
| Ctrl+F → `apple` → Enter | Cursor ไปบรรทัด 1 |
| Ctrl+F → Enter (ว่าง) | Cursor ไปบรรทัด 3 |
| Ctrl+F → Enter (ว่าง) | วนกลับบรรทัด 1 |
| Ctrl+F → `zzz` → Enter | ขึ้น `Not found: zzz` |
| Ctrl+F → ESC | ยกเลิกการค้นหา |

## 4. ย้อนการแก้ไข (Undo)

```bash
./editor tests/basic.txt
```

| ขั้นตอน | ผลที่คาดหวัง |
|---|---|
| พิมพ์ `abc` → Ctrl+Z 3 ครั้ง | ตัวอักษรหายทีละตัว |
| Enter กลางบรรทัด → Ctrl+Z | บรรทัดรวมกลับเป็นบรรทัดเดียว |
| Backspace ต้นบรรทัด → Ctrl+Z | บรรทัดแยกกลับเหมือนเดิม |
| Ctrl+Z จนหมด | ขึ้น `Nothing to undo` |

## 5. การแสดงผล Tab

```bash
./editor tests/hello.c
```

| ขั้นตอน | ผลที่คาดหวัง |
|---|---|
| เลื่อนไปบรรทัด `printf` กดลูกศรขวา | Cursor ตรงกับตัวอักษรเสมอ |
| กดปุ่ม Tab | เยื้องไปตำแหน่งถัดไปที่หารด้วย 4 ลงตัว |

## 6. รันคำสั่ง (Run Command)

```bash
./editor tests/hello.c
```

| ขั้นตอน | ผลที่คาดหวัง |
|---|---|
| Ctrl+R → `gcc tests/hello.c -o tests/hello && ./tests/hello` → Enter | เห็น `Hello OS` และ `[exit code 0]` |
| กดปุ่มใดก็ได้ | กลับสู่ Editor |
| ลบ `;` ออก → Ctrl+R → Enter (ว่าง) | บันทึกอัตโนมัติ รันคำสั่งเดิม เห็น error ของ gcc และ exit code ไม่ใช่ 0 |

**System Calls:** `pipe`, `fork`, `dup2`, `execve`, `waitpid`

## 7. บันทึกแบบปลอดภัย (Safe Save)

```bash
chmod 600 tests/basic.txt
./editor tests/basic.txt          # แก้ไข → Ctrl+S → Ctrl+Q
ls -la tests/
```

| ผลที่คาดหวัง |
|---|
| `basic.txt` ยังมีสิทธิ์ `-rw-------` เหมือนเดิม |
| ไม่มีไฟล์ `basic.txt.tmp` ค้างอยู่ |

ตรวจลำดับการทำงานด้วย strace:

```bash
strace -e trace=openat,write,fsync,rename -o save.trace ./editor tests/basic.txt
grep -E "tmp|fsync|rename" save.trace
```

ต้องเห็นลำดับ: เปิด `.tmp` → `write` → `fsync` → `rename`

**System Calls:** `stat`, `open`, `write`, `fsync`, `rename`

## 8. ตรวจสอบสิทธิ์ไฟล์ (Read-only)

```bash
chmod 444 tests/basic.txt
./editor tests/basic.txt
```

| ขั้นตอน | ผลที่คาดหวัง |
|---|---|
| เปิดไฟล์ | แถบสถานะมี `[read-only]` |
| Ctrl+S | ขึ้น `Read-only: cannot save` |

คืนสิทธิ์หลังทดสอบ:

```bash
chmod 644 tests/basic.txt
```

**System Calls:** `access`

## 9. บันทึกสำรองอัตโนมัติและกู้คืน (Auto-save & Recovery)

ต้องเปิด Terminal 2 แท็บ

| แท็บ | คำสั่ง / การกระทำ | ผลที่คาดหวัง |
|---|---|---|
| 1 | `./editor tests/basic.txt` พิมพ์ข้อความ ไม่บันทึก ไม่ออก | |
| 2 | `ls -a tests/` | เห็น `.basic.txt.swp` |
| 2 | `sleep 6; cat tests/.basic.txt.swp` | เห็นข้อความที่พิมพ์ในแท็บ 1 |
| 2 | `pkill -9 editor` | แท็บ 1 ขึ้น `Killed` (จำลองโปรแกรมหยุดทำงาน) |
| 1 | `reset` แล้ว `./editor tests/basic.txt` | ขึ้น `Unsaved changes found. Recover? (y/n)` |
| 1 | พิมพ์ `y` → Enter | ข้อความกลับมา ขึ้น `Recovered. Press ^S to save.` |
| 1 | Ctrl+S → Ctrl+Q แล้ว `ls -a tests/` | `.basic.txt.swp` ถูกลบ |

**System Calls:** `alarm`, `sigaction` (`SIGALRM`), `pwrite`, `ftruncate`, `unlink`

## 10. ป้องกันการเปิดไฟล์ซ้อน (File Lock)

ต้องเปิด Terminal 2 แท็บ

| แท็บ | คำสั่ง | ผลที่คาดหวัง |
|---|---|---|
| 1 | `./editor tests/basic.txt` เปิดค้างไว้ | แก้ไขได้ปกติ |
| 2 | `./editor tests/basic.txt` | แถบสถานะมี `[read-only]` และขึ้น `File is open in another editor: read-only` |
| 2 | Ctrl+S | ขึ้น `Read-only: cannot save` |

**System Calls:** `flock`

---

## สรุป System Calls ทั้งหมดของโปรแกรม

```bash
strace -c ./editor tests/basic.txt
```

กด Ctrl+Q ออกทันที จะได้ตารางจำนวนครั้งที่เรียกแต่ละ System Call

---

## แก้ปัญหาระหว่างทดสอบ

| ปัญหา | วิธีแก้ |
|---|---|
| Terminal ค้าง พิมพ์ไม่ขึ้นจอ | พิมพ์ `reset` แล้ว Enter |
| มีไฟล์ `.swp` ค้างจากการทดสอบ | `rm -f tests/.*.swp` |
| แก้โค้ดแล้วผลไม่เปลี่ยน | `make clean && make` |
| ไฟล์เปิดเป็น read-only โดยไม่ตั้งใจ | `chmod 644 ชื่อไฟล์` |
