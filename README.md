# ⚙️ xv6-riscv Kernel Extensions: Priority Scheduler & Counting Semaphores

## 📌 Overview
This repository contains solutions for advanced **Operating Systems** coursework at Bar-Ilan University. The project involves modifying and extending the **xv6-riscv** OS kernel to support a **Process Priority Scheduler** and **Kernel-Level Counting Semaphores**.

---

## 🧩 Key Implementation Features

### 🎯 Part 1: Process Priority Scheduler (`proc.c`, `sysproc.c`)
* **Priority Scheduling:** Enhanced xv6 process management by assigning dynamic priority levels to running processes.
* **Process State Inspection:** Modified `proc.c` and system calls to retrieve runtime state and scheduling metrics.
* **Verification:** Built `ps_full_test.c` to test priority execution, prevent starvation, and evaluate CPU allocation.

---

### 🚦 Part 2: Kernel-Level Counting Semaphores (`sem.c`, `sem_test.c`)
* **Counting Semaphores:** Implemented thread/process synchronization primitives directly inside the RISC-V kernel.
* **System Call Interface:** Exposed `sem_open`, `sem_wait`, `sem_post`, and `sem_close` system calls for user-space synchronization.
* **Verification:** Verified synchronization correctness using `sem_test.c` and `sem_full_test.c` under heavy concurrent loads.

---

## 🛠️ Tech Stack & Concepts
* **Operating Systems:** xv6 Kernel, RISC-V Architecture, Assembly, C
* **Concepts:** Priority Scheduling, Process Management, Kernel Semaphores, Atomic Operations, System Calls, Thread Safety

---

## 🚀 How to Run in QEMU

1. Compile and run xv6 with RISC-V QEMU emulator:
   ```bash
   make qemu
