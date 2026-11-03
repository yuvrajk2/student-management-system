# Student Management System v2.0

![CI](https://github.com/yuvrajk2/student-management-system/actions/workflows/ci.yml/badge.svg)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
![Language](https://img.shields.io/badge/Language-C99-00599C?logo=c)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey)

A terminal-based Student Management System written in pure C (C99).  
Coloured TUI with box-drawing borders, paginated tables, search, sort, statistics, backup/restore, and more — no external libraries required.

---

## Project Structure

```
student-management-system/
├── include/                # Header files (.h)
│   ├── backup.h
│   ├── login.h
│   ├── student.h
│   └── ui.h
├── src/                    # Source files (.c)
│   ├── backup.c
│   ├── login.c
│   ├── main.c
│   ├── student.c
│   └── ui.c
├── docs/                   # Full documentation
│   ├── ARCHITECTURE.md
│   ├── CHANGELOG.md
│   ├── CONTRIBUTING.md
│   └── USAGE.md
├── Makefile                # Cross-platform build automation
├── build.bat               # One-click Windows build script
├── build.sh                # Unix build script
├── .gitignore              # Ignores binaries, objects, and runtime data
└── README.md
```

---

## Quick Start

### Option A: Using Make (Recommended)

```bash
# Build binary
make

# Run application
make run

# Run automated unit test suite
make test

# Clean build artifacts and runtime caches
make clean
```

### Option B: Using Build Scripts

- **Windows:** Double-click or run `build.bat` in CMD / PowerShell:
  ```cmd
  build.bat
  student_mgmt.exe
  ```
- **macOS / Linux / WSL:**
  ```bash
  chmod +x build.sh && ./build.sh
  ./student_mgmt
  ```

### Option C: Direct GCC Compilation

**macOS / Linux / WSL (bash):**
```bash
gcc -Wall -Wextra -std=c99 -Iinclude src/*.c -o student_mgmt && ./student_mgmt
```

**Windows — PowerShell:**
```powershell
gcc -Wall -Wextra -std=c99 -Iinclude (Get-Item src/*.c) -o student_mgmt.exe
.\student_mgmt.exe
```

**Windows — Command Prompt (cmd.exe):**
```cmd
gcc -Wall -Wextra -std=c99 -Iinclude src\main.c src\login.c src\student.c src\ui.c src\backup.c src\sha256.c -o student_mgmt.exe && student_mgmt.exe
```

**Default login credentials:**
```
Username : admin
Password : admin123
```

---

## Features

| # | Feature | Description |
|---|---------|-------------|
| 1 | Add Student | Name, age, course, GPA, email, phone, enrollment year — all validated |
| 2 | View All | Paginated table (10 per page), navigate with N / P / Q |
| 3 | Search & Filter | By ID, name, course, or **multi-criteria filter** (GPA range, course, year) |
| 4 | Update | Edit any field; press Enter to keep the current value |
| 5 | Delete | Shows the record first, requires typed confirmation |
| 6 | Sort & View | Sort by ID, name, GPA ↑↓, age, course, or enrollment year |
| 7 | Statistics | Total count, avg / hi / lo GPA, bar-chart grade distribution |
| 8 | CSV Export & Import | Export to CSV + **bulk import from CSV** with automatic validation |
| 9 | Top-N Leaderboard | Ranked list by GPA; gold / silver / bronze medals for top 3 |
| 10 | Course Roster | All students in a course + per-course count and GPA summary |
| 11 | Backup & Restore | Timestamped snapshots in `backups/`; auto-backup before restore |
| 12 | Change Password | Masked input, verified, secured with **salted SHA-256 digest** |

---

## Module Overview

| Module | Source | Header | Responsibility |
|--------|--------|--------|----------------|
| **Main** | `src/main.c` | — | Entry point, login flow, 12-item menu loop |
| **Auth** | `src/login.c` | `include/login.h` | Authentication, masked password input, password change |
| **Crypto** | `src/sha256.c` | `include/sha256.h` | FIPS 180-4 standard SHA-256 salted password hashing |
| **Student** | `src/student.c` | `include/student.h` | CRUD, search, filter, stats, CSV export/import, leaderboard |
| **Backup** | `src/backup.c` | `include/backup.h` | Timestamped backup creation and restore |
| **UI** | `src/ui.c` | `include/ui.h` | ANSI colours, box-drawing helpers, validated input functions |
| **Tests** | `tests/test_all.c` | — | Automated unit test suite run in CI |

---

## Documentation

Detailed guides are located in the [`docs/`](docs/) directory:

| Document | Read it for… |
|----------|--------------|
| [docs/USAGE.md](docs/USAGE.md) | Step-by-step guide for every menu feature |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Code structure, data format, module design, and how to extend |
| [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md) | Build setup, code style, and adding new features |
| [docs/CHANGELOG.md](docs/CHANGELOG.md) | Version history and changes |

---

## Requirements

- GCC (C99 compatible or newer) / Clang / MSVC
- UTF-8 compatible terminal (macOS Terminal, Linux terminal, Windows Terminal)
- No external libraries required (uses standard C library; `conio.h` on Windows for password masking, `termios.h` on POSIX)
