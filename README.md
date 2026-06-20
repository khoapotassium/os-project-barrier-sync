# os-project-barrier-sync

A compact C/pthreads project demonstrating a reusable barrier for two-phase student score processing.

---

## What it does

- **Phase 1:** parallel local sum computation
- **Phase 2:** barrier synchronization, then count scores below the computed average

## Files

- `src/main.c` — primary barrier synchronization example with fixed data set
- `src/test.c` — benchmarking version with serial and threaded execution modes

## Why this matters

A reusable barrier ensures that all threads complete phase 1 before any thread begins phase 2. This prevents incorrect average calculations and invalid downstream results.

## How to build

```bash
gcc src/main.c -o barrier-sync -pthread
gcc src/test.c -o barrier-test -pthread
```

## How to run

```bash
./barrier-sync
./barrier-test
```

## Test ideas

- Change thread counts in `src/main.c` and `src/test.c`
- Compare correctness and execution time for different thread counts
- Compare serial and parallel modes in `src/test.c`

## Notes

- Runtime depends on hardware and thread scheduling

---

## Team members

- **Đặng Việt Khoa** — 202417144
- **Lưu Quốc Khánh** — 202417140
- **Đoàn Nguyễn Ngọc Sơn** — 202417191
- **Trần Quang Trung** — 202417209
