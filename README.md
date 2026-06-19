# os-project-barrier-sync

A compact C/pthreads project demonstrating a reusable barrier for two-phase student score processing.

---

## What it does

- **Phase 1:** parallel local sum computation
- **Phase 2:** barrier synchronization, then count scores below the average

## Why this matters

A reusable barrier ensures that all threads finish phase 1 before phase 2 begins. This prevents incorrect average calculations and invalid downstream results.

## How to build

```bash
gcc src/main.c -o barrier-sync -pthread
./barrier-sync
```

## Test ideas

- Change `THREAD_COUNT` in `src/main.c` (for example `2`, `4`, `8`)
- Use `NUM_STUDENTS = 1000000` and `NUM_STUDENTS = 10000000`
- Compare correctness and execution time across different thread counts

## Notes

- The best runtime relying on thread count is not fixed and different from ideal due to hardware.

---

## Team members

- **Đặng Việt Khoa** — 202417144
- **Lưu Quốc Khánh** — 202417140
- **Đoàn Nguyễn Ngọc Sơn** — 202417191
- **Trần Quang Trung** — 202417209
