# Contributing — Student Management System v2.0

Guidelines for building, extending, and maintaining this project.

---

## Build Setup

### Requirements

- GCC with C99 support (or Clang / MSVC)
- Any terminal (Linux, macOS, Windows PowerShell, cmd.exe)
- No external libraries

### Option A: Using Make (Cross-platform)

```bash
make        # Compiles the project into student_mgmt (or student_mgmt.exe on Windows)
make run    # Runs the compiled binary
make test   # Runs the automated unit test suite
make clean  # Cleans binary, object files, and test data
```

### Option B: Build Scripts

- **Windows:** Run `build.bat`
- **macOS / Linux:** Run `./build.sh`

### Option C: Direct GCC Compilation

**macOS / Linux / WSL:**
```bash
gcc -Wall -Wextra -std=c99 -Iinclude src/*.c -o student_mgmt
./student_mgmt
```

**Windows PowerShell:**
```powershell
gcc -Wall -Wextra -std=c99 -Iinclude (Get-Item src/*.c) -o student_mgmt.exe
.\student_mgmt.exe
```

**Windows cmd.exe:**
```cmd
gcc -Wall -Wextra -std=c99 -Iinclude src\main.c src\login.c src\student.c src\ui.c src\backup.c src\sha256.c -o student_mgmt.exe
student_mgmt.exe
```

### Clean build artefacts

Using `make clean` is recommended, or manually:

**macOS / Linux:**
```bash
rm -f student_mgmt src/*.o students.dat admin.cfg students_export.csv
rm -rf backups/
```

**Windows PowerShell:**
```powershell
Remove-Item -ErrorAction SilentlyContinue student_mgmt.exe, src\*.o, students.dat, admin.cfg, students_export.csv
Remove-Item -Recurse -ErrorAction SilentlyContinue backups
```

---

## Code Style

| Rule | Example |
|------|---------|
| Standard | C99 — avoid compiler-specific extensions |
| Naming | `snake_case` for all functions and variables |
| Macros | `UPPER_CASE` for constants and macros |
| Braces | Opening brace on the same line as the statement |
| Indentation | 4 spaces — no tabs |
| Line length | Keep lines under 100 characters where practical |
| Comments | Use `/* … */` block comments for sections, `//` for inline |
| No global state | Do not introduce global variables; pass data through parameters |
| No raw scanf | All user input must go through `ui.c` helpers (`readInt`, `readString`, etc.) |
| No exit() in modules | Only `main.c` may call `exit()` — library functions return error codes or print and return |

---

## Project Structure Rules

- **`src/ui.c` & `include/ui.h`** — terminal/display only. No business logic, no file I/O.
- **`src/login.c` & `include/login.h`** — authentication only. No student data access.
- **`src/student.c` & `include/student.h`** — student data operations only. May call `ui.c` helpers.
- **`src/backup.c` & `include/backup.h`** — file copy operations only. May call `ui.c` helpers and `countStudents()`.
- **`src/main.c`** — orchestration only. No direct data manipulation; calls module functions.

---

## Adding a New Feature

### Step 1 — Write the function

Add it to the most appropriate existing `.c` file in `src/`, or create a new `src/feature.c` / `include/feature.h` pair.

```c
/* include/feature.h */
#ifndef FEATURE_H
#define FEATURE_H
void myFeature(void);
#endif

/* src/feature.c */
#include "feature.h"
#include "ui.h"

void myFeature(void) {
    clearScreen();
    printf("\n");
    printHLine(BOX_TL, BOX_H, BOX_TR);
    printBoxTitle("MY FEATURE");
    printHLine(BOX_BL, BOX_H, BOX_BR);
    /* ... */
    pauseScreen();
}
```

### Step 2 — Add it to the menu

In `src/main.c`:

```c
/* In displayMainMenu() — add a printf line */
printf(BOX_V "  " C_CYAN "[13]" C_RESET " My Feature"
       "%*s" BOX_V "\n", BOX_WIDTH - 19, "");

/* In the switch statement */
case 13: myFeature(); break;
```

### Step 3 — Add the new file to the build command

Add `src/feature.c` to `SRCS` in `Makefile` (and `build.bat` / `build.sh`):

```makefile
SRCS = src/main.c src/login.c src/student.c src/ui.c src/backup.c src/feature.c
```

---

## Adding a New Student Field

1. Add the field to the `Student` struct in `student.h`:
   ```c
   char department[64];
   ```

2. Add the input prompt in `addStudent()` in `student.c`:
   ```c
   readString("Department : ", s.department, 64);
   ```

3. Add the optional-update prompt in `updateStudent()`:
   ```c
   snprintf(prompt, sizeof(prompt), "Dept   [%s]: ", s->department);
   readStringOpt(prompt, s->department, 64);
   ```

4. Add the field to `exportCSV()` — both the CSV header and the data row.

5. **Delete `students.dat`** — the binary struct layout changed; old data is incompatible.

---

## Box Layout Rules

All output must fit within `BOX_WIDTH = 78` columns (76 inner characters between the two `║` borders).

When adding a new box screen:

```c
clearScreen();
printf("\n");
printHLine(BOX_TL, BOX_H, BOX_TR);   /* top border    */
printBoxTitle("MY SCREEN TITLE");
printHLine(BOX_ML, BOX_H, BOX_MR);   /* mid separator */
printBoxBlank();                       /* blank row     */
/* content rows — each must be exactly BOX_WIDTH chars wide */
printf(BOX_V " %-*s " BOX_V "\n", BOX_WIDTH - 4, "some text");
printBoxBlank();
printHLine(BOX_BL, BOX_H, BOX_BR);   /* bottom border */
```

---

## Common Pitfalls

| Problem | Cause | Fix |
|---------|-------|-----|
| `&&` error in PowerShell | PowerShell doesn't support `&&` | Run compile and run as two separate lines |
| `termios.h` not found on Windows | POSIX header missing | Not needed — `login.c` uses `conio.h` on Windows automatically |
| `_mkdir` not found | Wrong header included | `backup.c` includes `<direct.h>` on Windows via `#ifdef _WIN32` |
| Students lost after recompile | Struct layout changed | Delete `students.dat` and re-enter data |
| Box borders misaligned | printf width arithmetic wrong | Count inner chars: must total `BOX_WIDTH - 2` |
| Garbled box characters | Terminal not set to UTF-8 | Use Windows Terminal (UTF-8 by default); avoid old cmd.exe |
| Partial backup name shown | `strncpy` missing null terminator | Always set `buf[maxlen-1] = '\0'` after strncpy |

---

## Checklist Before Committing

- [ ] Compiles cleanly with `-Wall -Wextra` and zero warnings
- [ ] All user input goes through `ui.c` helpers
- [ ] Every `loadAll()` is matched with `free()`
- [ ] New box screens stay within `BOX_WIDTH = 78`
- [ ] `pauseScreen()` is called at the end of every terminal operation
- [ ] `ARCHITECTURE.md` updated if struct or module boundaries changed
- [ ] `CHANGELOG.md` updated with a short entry under the next version
