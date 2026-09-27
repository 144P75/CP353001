# Terminal Text Editor

โปรแกรมแก้ไขไฟล์ข้อความบน Terminal แบบ nano ย่อส่วน เขียนด้วยภาษา C และทำงานบน Linux (WSL) โดยเรียกใช้ POSIX System Calls โดยตรง

Mini Project รายวิชา Operating Systems and System Calls มหาวิทยาลัยขอนแก่น

## ความสามารถ

- แก้ไขข้อความ: พิมพ์ ลบ ขึ้นบรรทัดใหม่ เลื่อน Cursor ด้วยปุ่มลูกศร
- เปิดและบันทึกไฟล์ (Safe Save: เขียนลงไฟล์ชั่วคราวก่อนแล้วจึงแทนที่ ไฟล์เดิมไม่เสียหายแม้โปรแกรมหยุดทำงานกลางคัน)
- ปรับหน้าจอตามขนาดหน้าต่างอัตโนมัติ
- ค้นหาข้อความ (Search)
- ย้อนการแก้ไข (Undo)
- รันคำสั่งจากใน Editor เช่น compile และรันโค้ด แล้วแสดงผลลัพธ์ (Run Command)
- บันทึกสำรองอัตโนมัติทุก 5 วินาที และกู้คืนเมื่อโปรแกรมหยุดทำงานผิดปกติ (Auto-save & Recovery)
- ป้องกันการเปิดแก้ไขไฟล์เดียวกันพร้อมกัน (File Lock)
- ตรวจสอบสิทธิ์ไฟล์ ถ้าไม่มีสิทธิ์เขียนจะเปิดแบบอ่านอย่างเดียว (Read-only)

## ความต้องการของระบบ

- Linux หรือ WSL (Ubuntu)
- gcc และ make

ติดตั้งได้ด้วย

```bash
sudo apt install build-essential
```

## การติดตั้งและใช้งาน

```bash
make                      # compile
./editor ชื่อไฟล์.txt       # เปิดไฟล์ (ถ้าไม่มีจะสร้างใหม่ตอนบันทึก)
make clean                # ลบไฟล์ที่ compile แล้ว
```

## วิธีทดสอบแต่ละฟีเจอร์ ดูที่ [TESTING.md](TESTING.md)

## ปุ่มลัด

| ปุ่ม | การทำงาน |
|---|---|
| ลูกศร | เลื่อน Cursor |
| Enter | ขึ้นบรรทัดใหม่ |
| Backspace / Delete | ลบตัวอักษร |
| Ctrl+S | บันทึกไฟล์ |
| Ctrl+F | ค้นหา (Enter ว่างเพื่อหาคำเดิมตัวถัดไป) |
| Ctrl+Z | ย้อนการแก้ไข |
| Ctrl+R | รันคำสั่ง (Enter ว่างเพื่อรันคำสั่งเดิม) |
| Ctrl+Q | ออกจากโปรแกรม (ถ้ายังไม่บันทึก ต้องกดซ้ำ) |
| ESC | ยกเลิกการค้นหาหรือการรันคำสั่ง |

ตัวอย่างการใช้ Run Command: เปิดไฟล์ `hello.c` กด Ctrl+R แล้วพิมพ์

```
gcc hello.c -o hello && ./hello
```

## โครงสร้างโปรเจกต์

```
terminal-text-editor/
├── Makefile
├── README.md
├── include/
│   └── editor.h        โครงสร้างข้อมูลกลางและการประกาศฟังก์ชัน
├── src/
│   ├── main.c          จุดเริ่มต้นและลูปหลัก
│   ├── terminal.c      Raw mode, ขนาดหน้าจอ, การอ่านปุ่ม
│   ├── signals.c       จัดการ Signal (SIGWINCH, SIGALRM)
│   ├── buffer.c        จัดเก็บและแก้ไขข้อความใน Memory
│   ├── render.c        วาดหน้าจอและแถบสถานะ
│   ├── input.c         แปลงปุ่มเป็นคำสั่ง และช่องรับข้อความ
│   ├── fileio.c        เปิด/บันทึกไฟล์, สิทธิ์ไฟล์, File lock, Auto-save
│   ├── search.c        ค้นหาข้อความ
│   ├── undo.c          ประวัติการแก้ไขและการย้อนกลับ
│   └── runcmd.c        รันคำสั่งผ่าน Process ลูก
└── tests/              ไฟล์สำหรับทดสอบ
```

## System Calls ที่ใช้และความเกี่ยวข้องกับรายวิชา

| หัวข้อในรายวิชา | System Calls | ใช้ทำอะไร |
|---|---|---|
| I/O Systems, Device Drivers | `tcgetattr`, `tcsetattr`, `ioctl`, `read`, `write` | สลับ Terminal เป็น Raw mode, ขอขนาดหน้าจอ, รับปุ่ม, วาดหน้าจอ |
| Signals | `sigaction`, `alarm` | รับ `SIGWINCH` เมื่อปรับขนาดหน้าต่าง, รับ `SIGALRM` เพื่อ Auto-save |
| Process Management | `fork`, `execvp`, `waitpid` | สร้าง Process ลูกเพื่อรันคำสั่ง และรอให้จบ |
| IPC | `pipe`, `dup2` | ส่ง Output ของ Process ลูกกลับมาแสดงใน Editor |
| File System | `open`, `read`, `pread`, `pwrite`, `fstat`, `ftruncate`, `fsync`, `rename`, `unlink`, `close` | เปิด บันทึก และบันทึกแบบปลอดภัย |
| Permissions | `access`, `stat` | ตรวจสิทธิ์การเขียนและคงสิทธิ์เดิมของไฟล์ |
| Synchronization | `flock` | ป้องกันการแก้ไขไฟล์เดียวกันพร้อมกัน |
| Memory Management | `malloc`, `realloc`, `free` (เบื้องหลังคือ `brk`, `mmap`) | จัดเก็บข้อความที่ขยายขนาดได้ |

ตรวจสอบการเรียก System Calls ของโปรแกรมได้ด้วย

```bash
strace -o editor.trace ./editor ชื่อไฟล์.txt
```

## ไฟล์ที่โปรแกรมสร้างขึ้น

- `.ชื่อไฟล์.swp` ไฟล์สำรองสำหรับ Auto-save และ File lock มีอยู่เฉพาะตอนเปิด Editor และถูกลบเมื่อออกจากโปรแกรมตามปกติ
- `ชื่อไฟล์.tmp` ไฟล์ชั่วคราวระหว่างบันทึก จะถูกเปลี่ยนชื่อทับไฟล์จริงทันทีเมื่อบันทึกสำเร็จ

## ข้อจำกัด

- รองรับเฉพาะตัวอักษรภาษาอังกฤษ (ASCII)
- คำสั่งที่รันด้วย Ctrl+R ไม่สามารถรับ Input จากคีย์บอร์ดได้ และถ้าคำสั่งทำงานไม่สิ้นสุด Editor จะรอจนกว่าคำสั่งจะจบ
- หากโปรแกรมถูกหยุดด้วย `kill -9` Terminal จะค้างอยู่ใน Raw mode ให้พิมพ์ `reset` เพื่อคืนค่า