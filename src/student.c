/*
 * student.c — CRUD, search, sort, statistics, export, leaderboard, roster
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "student.h"
#include "ui.h"

/* ── Internal helpers ────────────────────────────────────── */

static int loadAll(Student **out) {
    FILE *f = fopen(DATA_FILE, "rb");
    if (!f) { *out = NULL; return 0; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    int n = (int)(sz / sizeof(Student));
    if (n == 0) { fclose(f); *out = NULL; return 0; }
    *out = malloc(n * sizeof(Student));
    if (!*out) { fclose(f); return 0; }
    fread(*out, sizeof(Student), n, f);
    fclose(f);
    return n;
}

static int saveAll(Student *arr, int n) {
    FILE *f = fopen(DATA_FILE, "wb");
    if (!f) return 0;
    fwrite(arr, sizeof(Student), n, f);
    fclose(f);
    return 1;
}

static int nextId(Student *arr, int n) {
    int max = 0;
    for (int i = 0; i < n; i++)
        if (arr[i].id > max) max = arr[i].id;
    return max + 1;
}

/* Case-insensitive substring search */
static int containsCI(const char *haystack, const char *needle) {
    char h[256], n[256];
    int i;
    for (i = 0; haystack[i] && i < 255; i++) h[i] = tolower((unsigned char)haystack[i]);
    h[i] = '\0';
    for (i = 0; needle[i] && i < 255; i++) n[i] = tolower((unsigned char)needle[i]);
    n[i] = '\0';
    return strstr(h, n) != NULL;
}

/* ── Validation & CSV Parsing Helpers ────────────────────── */

int validateStudent(const Student *s, char *err_msg, size_t err_size) {
    if (!s) return 0;
    if (strlen(s->name) == 0) {
        if (err_msg && err_size > 0) snprintf(err_msg, err_size, "Name cannot be empty.");
        return 0;
    }
    if (s->age < 15 || s->age > 100) {
        if (err_msg && err_size > 0) snprintf(err_msg, err_size, "Age must be between 15 and 100.");
        return 0;
    }
    if (strlen(s->course) == 0) {
        if (err_msg && err_size > 0) snprintf(err_msg, err_size, "Course cannot be empty.");
        return 0;
    }
    if (s->gpa < 0.0f || s->gpa > 4.0f) {
        if (err_msg && err_size > 0) snprintf(err_msg, err_size, "GPA must be between 0.00 and 4.00.");
        return 0;
    }
    time_t t = time(NULL);
    int curYear = (int)(localtime(&t)->tm_year + 1900);
    if (s->year < 1990 || s->year > curYear + 1) {
        if (err_msg && err_size > 0) snprintf(err_msg, err_size, "Enrollment year must be between 1990 and %d.", curYear + 1);
        return 0;
    }
    return 1;
}

/* True if the line is the exported header row ("ID,..." or "Name,...").
 * Compares the leading field only, so a student called "David" or a row
 * containing "id" somewhere is not mistaken for a header. */
int isCSVHeader(const char *line) {
    static const char *heads[] = { "ID,", "Name," };
    if (!line) return 0;
    for (size_t h = 0; h < sizeof(heads) / sizeof(heads[0]); h++) {
        size_t n = strlen(heads[h]);
        size_t i = 0;
        while (i < n && line[i] &&
               tolower((unsigned char)line[i]) == tolower((unsigned char)heads[h][i]))
            i++;
        if (i == n) return 1;
    }
    return 0;
}

int parseCSVRow(const char *line, Student *out) {
    if (!line || !out) return 0;
    memset(out, 0, sizeof(Student));
    char fields[8][128];
    int f_idx = 0;
    int c_idx = 0;
    int in_quotes = 0;

    for (int i = 0; line[i] && line[i] != '\r' && line[i] != '\n'; i++) {
        char ch = line[i];
        if (ch == '"') {
            in_quotes = !in_quotes;
        } else if (ch == ',' && !in_quotes) {
            /* A valid row has at most 8 fields (7 commas). Reject anything
             * longer instead of writing past the end of fields[]. */
            if (f_idx >= 7) return 0;
            fields[f_idx][c_idx] = '\0';
            f_idx++;
            c_idx = 0;
        } else {
            if (c_idx < 127) {
                fields[f_idx][c_idx++] = ch;
            }
        }
    }
    fields[f_idx][c_idx] = '\0';
    f_idx++;

    int offset = 0;
    if (f_idx >= 8) {
        out->id = atoi(fields[0]);
        offset = 1;
    } else if (f_idx == 7) {
        out->id = 0;
        offset = 0;
    } else {
        return 0;
    }

    strncpy(out->name, fields[offset + 0], sizeof(out->name) - 1);
    out->age = atoi(fields[offset + 1]);
    strncpy(out->course, fields[offset + 2], sizeof(out->course) - 1);
    out->gpa = (float)atof(fields[offset + 3]);
    strncpy(out->email, fields[offset + 4], sizeof(out->email) - 1);
    strncpy(out->phone, fields[offset + 5], sizeof(out->phone) - 1);
    out->year = atoi(fields[offset + 6]);

    return validateStudent(out, NULL, 0);
}

/* ── Sort comparators ────────────────────────────────────── */

static int cmpId     (const void *a, const void *b) { return ((Student*)a)->id   - ((Student*)b)->id;   }
static int cmpName   (const void *a, const void *b) { return strcmp(((Student*)a)->name,   ((Student*)b)->name);   }
static int cmpCourse (const void *a, const void *b) { return strcmp(((Student*)a)->course, ((Student*)b)->course); }
static int cmpAge    (const void *a, const void *b) { return ((Student*)a)->age  - ((Student*)b)->age;  }
static int cmpYear   (const void *a, const void *b) { return ((Student*)a)->year - ((Student*)b)->year; }
static int cmpGpaDesc(const void *a, const void *b) {
    float d = ((Student*)b)->gpa - ((Student*)a)->gpa;
    return (d > 0) - (d < 0);
}
static int cmpGpaAsc (const void *a, const void *b) {
    float d = ((Student*)a)->gpa - ((Student*)b)->gpa;
    return (d > 0) - (d < 0);
}

static void sortArr(Student *arr, int n, SortKey key) {
    typedef int (*Cmp)(const void*, const void*);
    Cmp cmps[] = { cmpId, cmpName, cmpGpaDesc, cmpGpaAsc, cmpAge, cmpCourse, cmpYear };
    qsort(arr, n, sizeof(Student), cmps[key]);
}

/* ── Table rendering ─────────────────────────────────────── */

#define PAGE_SIZE 10
#define W_ID   4
#define W_NAME 22
#define W_AGE  3
#define W_CRS  18
#define W_GPA  4
#define W_YR   4

static void printTableHeader(void) {
    /* Top border — 76 ═ chars between the two corner pieces */
    printf(C_CYAN BOX_TL);
    for (int i = 0; i < BOX_WIDTH - 2; i++) printf(BOX_H);
    printf(BOX_TR C_RESET "\n");

    printf(C_CYAN BOX_V C_RESET
           C_BOLD C_BWHITE
           " %-*s  %-*s  %-*s  %-*s  %-*s  %-*s "
           C_RESET
           C_CYAN BOX_V C_RESET "\n",
           W_ID,       "ID",
           W_NAME,     "Name",
           W_AGE,      "Age",
           W_CRS,      "Course",
           W_GPA + 2,  "GPA",
           W_YR,       "Year");

    printf(C_CYAN BOX_ML);
    for (int i = 0; i < BOX_WIDTH - 2; i++) printf(BOX_H);
    printf(BOX_MR C_RESET "\n");
}

static void printTableRow(const Student *s, int rowNum) {
    const char *dim = (rowNum % 2 == 0) ? C_DIM : C_RESET;
    printf(C_CYAN BOX_V C_RESET "%s"
           " %-*d  %-*.*s  %-*d  %-*.*s  %s%*.2f%s  %-*d "
           C_RESET C_CYAN BOX_V C_RESET "\n",
           dim,
           W_ID,   s->id,
           W_NAME, W_NAME, s->name,
           W_AGE,  s->age,
           W_CRS,  W_CRS,  s->course,
           gpaColor(s->gpa), W_GPA + 2, s->gpa, C_RESET,
           W_YR,   s->year);
}

static void printTableFooter(void) {
    printf(C_CYAN BOX_BL);
    for (int i = 0; i < BOX_WIDTH - 2; i++) printf(BOX_H);
    printf(BOX_BR C_RESET "\n");
}

static void displayTable(Student *arr, int n, SortKey key, const char *title) {
    if (n == 0) { printWarning("No records to display."); return; }

    sortArr(arr, n, key);

    int page  = 0;
    int pages = (n + PAGE_SIZE - 1) / PAGE_SIZE;

    while (1) {
        clearScreen();
        printf("\n");
        printHLine(BOX_TL, BOX_H, BOX_TR);
        printBoxTitle(title);
        printHLine(BOX_ML, BOX_H, BOX_MR);

        char status[BOX_WIDTH];
        snprintf(status, sizeof(status),
                 "Total: %d student%s   Page %d of %d",
                 n, n == 1 ? "" : "s", page + 1, pages);
        printf(BOX_V " " C_DIM "%-*s" C_RESET BOX_V "\n",
               BOX_WIDTH - 3, status);
        printHLine(BOX_ML, BOX_H, BOX_MR);

        printTableHeader();

        int start = page * PAGE_SIZE;
        int end   = start + PAGE_SIZE < n ? start + PAGE_SIZE : n;
        for (int i = start; i < end; i++)
            printTableRow(&arr[i], i - start);

        printTableFooter();

        printf("\n  " C_DIM);
        if (page > 0)           printf("[P] Prev  ");
        if (page < pages - 1)   printf("[N] Next  ");
        printf("[Q] Back" C_RESET "   ");
        fflush(stdout);

        char ch[8];
        if (fgets(ch, sizeof(ch), stdin)) {
            char c = tolower((unsigned char)ch[0]);
            if (c == 'q') break;
            if (c == 'n' && page < pages - 1) page++;
            if (c == 'p' && page > 0)         page--;
        }
    }
}

/* ── Public: utilities ───────────────────────────────────── */

int countStudents(void) {
    FILE *f = fopen(DATA_FILE, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    int n = (int)(ftell(f) / sizeof(Student));
    fclose(f);
    return n;
}

float avgGPA(void) {
    Student *arr;
    int n = loadAll(&arr);
    if (n == 0) return 0.0f;
    float sum = 0;
    for (int i = 0; i < n; i++) sum += arr[i].gpa;
    free(arr);
    return sum / n;
}

/* ── Add ─────────────────────────────────────────────────── */

void addStudent(void) {
    Student *arr;
    int n = loadAll(&arr);

    Student s;
    memset(&s, 0, sizeof(s));
    s.id = nextId(arr, n);
    free(arr);

    clearScreen();
    printf("\n");
    printHLine(BOX_TL, BOX_H, BOX_TR);
    printBoxTitle("ADD NEW STUDENT");
    printHLine(BOX_ML, BOX_H, BOX_MR);
    printBoxBlank();
    printf(BOX_V "  " C_DIM "Auto-assigned ID: " C_BWHITE "%d"
           C_RESET "%*s" BOX_V "\n",
           s.id, BOX_WIDTH - 24 - (s.id > 99 ? 3 : s.id > 9 ? 2 : 1), "");
    printBoxBlank();
    printHLine(BOX_BL, BOX_H, BOX_BR);
    printf("\n");

    readString("Full Name        : ", s.name,   MAX_NAME);
    s.age = readInt("Age (15-100)     : ", 15, 100);
    readString("Course           : ", s.course, MAX_COURSE);
    s.gpa = readFloat("GPA (0.00-4.00)  : ", 0.0f, 4.0f);
    readString("Email            : ", s.email,  MAX_EMAIL);
    readString("Phone            : ", s.phone,  MAX_PHONE);

    time_t t = time(NULL);
    int curYear = (int)(localtime(&t)->tm_year + 1900);
    s.year = readInt("Enrollment Year  : ", 1990, curYear);

    FILE *f = fopen(DATA_FILE, "ab");
    if (!f) { printError("Could not open data file."); return; }
    fwrite(&s, sizeof(Student), 1, f);
    fclose(f);

    printSuccess("Student added successfully!");
    printf("  " C_DIM "ID: %d  |  %s  |  %s  |  GPA: %.2f\n" C_RESET,
           s.id, s.name, s.course, s.gpa);
}

/* ── View ────────────────────────────────────────────────── */

void viewStudents(SortKey key) {
    Student *arr;
    int n = loadAll(&arr);
    if (n == 0) { printWarning("No students on record."); pauseScreen(); return; }
    displayTable(arr, n, key, "ALL STUDENTS");
    free(arr);
}

/* ── Filter ──────────────────────────────────────────────── */

void filterStudents(void) {
    Student *arr;
    int n = loadAll(&arr);
    if (n == 0) { printWarning("No students on record."); pauseScreen(); return; }

    clearScreen();
    printf("\n");
    printHLine(BOX_TL, BOX_H, BOX_TR);
    printBoxTitle("MULTI-CRITERIA ADVANCED FILTER");
    printHLine(BOX_ML, BOX_H, BOX_MR);
    printBoxBlank();
    printBoxTitle("Filter students by combining criteria");
    printBoxBlank();
    printHLine(BOX_BL, BOX_H, BOX_BR);
    printf("\n");

    float minGpa = readFloat("Minimum GPA (0.00 - 4.00)                      : ", 0.0f, 4.0f);
    float maxGpa = readFloat("Maximum GPA (0.00 - 4.00)                      : ", minGpa, 4.0f);

    char courseFilter[64];
    readStringOpt("Course name (partial match, Enter to match all): ", courseFilter, sizeof(courseFilter));

    int yearFilter = readInt("Enrollment Year (1990-current, or 0 for all)    : ", 0, 2030);

    Student *matches = malloc(n * sizeof(Student));
    int count = 0;

    for (int i = 0; i < n; i++) {
        if (arr[i].gpa < minGpa || arr[i].gpa > maxGpa) continue;
        if (strlen(courseFilter) > 0 && !containsCI(arr[i].course, courseFilter)) continue;
        if (yearFilter > 0 && arr[i].year != yearFilter) continue;
        matches[count++] = arr[i];
    }
    free(arr);

    if (count == 0) {
        printWarning("No students matched the filter criteria.");
        free(matches);
        pauseScreen();
        return;
    }

    displayTable(matches, count, SORT_GPA_DESC, "FILTERED STUDENTS");
    free(matches);
}

/* ── Search ──────────────────────────────────────────────── */

void searchStudent(void) {
    Student *arr;
    int n = loadAll(&arr);
    if (n == 0) { printWarning("No students on record."); pauseScreen(); return; }

    clearScreen();
    printf("\n");
    printHLine(BOX_TL, BOX_H, BOX_TR);
    printBoxTitle("SEARCH STUDENTS");
    printHLine(BOX_ML, BOX_H, BOX_MR);
    printf(BOX_V "  " C_CYAN "[1]" C_RESET " Search by ID"
           "%*s" BOX_V "\n", BOX_WIDTH - 17, "");
    printf(BOX_V "  " C_CYAN "[2]" C_RESET " Search by Name (partial)"
           "%*s" BOX_V "\n", BOX_WIDTH - 29, "");
    printf(BOX_V "  " C_CYAN "[3]" C_RESET " Search by Course"
           "%*s" BOX_V "\n", BOX_WIDTH - 21, "");
    printf(BOX_V "  " C_CYAN "[4]" C_RESET " Advanced Multi-Criteria Filter"
           "%*s" BOX_V "\n", BOX_WIDTH - 35, "");
    printHLine(BOX_BL, BOX_H, BOX_BR);
    printf("\n");

    int choice = readInt("Choice [1-4]: ", 1, 4);
    if (choice == 4) {
        free(arr);
        filterStudents();
        return;
    }
    printf("\n");

    Student *results = malloc(n * sizeof(Student));
    int rn = 0;

    if (choice == 1) {
        int id = readInt("Student ID: ", 1, 999999);
        for (int i = 0; i < n; i++)
            if (arr[i].id == id) results[rn++] = arr[i];
    } else if (choice == 2) {
        char query[64];
        readString("Name (partial): ", query, sizeof(query));
        for (int i = 0; i < n; i++)
            if (containsCI(arr[i].name, query)) results[rn++] = arr[i];
    } else {
        char query[64];
        readString("Course (partial): ", query, sizeof(query));
        for (int i = 0; i < n; i++)
            if (containsCI(arr[i].course, query)) results[rn++] = arr[i];
    }

    free(arr);

    if (rn == 0) {
        printWarning("No matching students found.");
        pauseScreen();
        free(results);
        return;
    }

    displayTable(results, rn, SORT_ID, "SEARCH RESULTS");
    free(results);
}

/* ── Update ──────────────────────────────────────────────── */

void updateStudent(void) {
    Student *arr;
    int n = loadAll(&arr);
    if (n == 0) { printWarning("No students on record."); pauseScreen(); return; }

    printf("\n");
    int id = readInt("Enter Student ID to update: ", 1, 999999);

    int idx = -1;
    for (int i = 0; i < n; i++)
        if (arr[i].id == id) { idx = i; break; }

    if (idx < 0) {
        printError("Student not found.");
        free(arr); pauseScreen(); return;
    }

    Student *s = &arr[idx];
    printf("\n  " C_DIM "Leave blank to keep the current value.\n\n" C_RESET);

    char prompt[128], tmp[64];
    int  parsed;
    float fp;

    snprintf(prompt, sizeof(prompt), "Name   [%s]: ", s->name);
    readStringOpt(prompt, s->name, MAX_NAME);

    printf("  Age    [%d]: ", s->age);
    fflush(stdout);
    if (fgets(tmp, sizeof(tmp), stdin) && sscanf(tmp, "%d", &parsed) == 1)
        if (parsed >= 15 && parsed <= 100) s->age = parsed;

    snprintf(prompt, sizeof(prompt), "Course [%s]: ", s->course);
    readStringOpt(prompt, s->course, MAX_COURSE);

    printf("  GPA    [%.2f]: ", s->gpa);
    fflush(stdout);
    if (fgets(tmp, sizeof(tmp), stdin) && sscanf(tmp, "%f", &fp) == 1)
        if (fp >= 0.0f && fp <= 4.0f) s->gpa = fp;

    snprintf(prompt, sizeof(prompt), "Email  [%s]: ", s->email);
    readStringOpt(prompt, s->email, MAX_EMAIL);

    snprintf(prompt, sizeof(prompt), "Phone  [%s]: ", s->phone);
    readStringOpt(prompt, s->phone, MAX_PHONE);

    printf("  Year   [%d]: ", s->year);
    fflush(stdout);
    if (fgets(tmp, sizeof(tmp), stdin) && sscanf(tmp, "%d", &parsed) == 1)
        if (parsed >= 1990 && parsed <= 2100) s->year = parsed;

    if (saveAll(arr, n))
        printSuccess("Student record updated successfully.");
    else
        printError("Failed to save changes.");

    free(arr);
}

/* ── Delete ──────────────────────────────────────────────── */

void deleteStudent(void) {
    Student *arr;
    int n = loadAll(&arr);
    if (n == 0) { printWarning("No students on record."); pauseScreen(); return; }

    printf("\n");
    int id = readInt("Enter Student ID to delete: ", 1, 999999);

    int idx = -1;
    for (int i = 0; i < n; i++)
        if (arr[i].id == id) { idx = i; break; }

    if (idx < 0) {
        printError("Student not found.");
        free(arr); pauseScreen(); return;
    }

    Student *s = &arr[idx];
    printf("\n  " C_BYELLOW "Student to delete:" C_RESET "\n");
    printf("  ID: %d  |  %s  |  %s  |  GPA: %.2f\n\n",
           s->id, s->name, s->course, s->gpa);
    printf("  " C_BRED "Confirm delete? [y/N]: " C_RESET);
    fflush(stdout);

    char ch[8];
    if (!fgets(ch, sizeof(ch), stdin) || tolower((unsigned char)ch[0]) != 'y') {
        printInfo("Delete cancelled.");
        free(arr); pauseScreen(); return;
    }

    for (int i = idx; i < n - 1; i++) arr[i] = arr[i + 1];
    n--;

    if (saveAll(arr, n))
        printSuccess("Student deleted successfully.");
    else
        printError("Failed to save changes.");

    free(arr);
}

/* ── Sort menu ───────────────────────────────────────────── */

void sortMenu(void) {
    clearScreen();
    printf("\n");
    printHLine(BOX_TL, BOX_H, BOX_TR);
    printBoxTitle("SORT & VIEW STUDENTS");
    printHLine(BOX_ML, BOX_H, BOX_MR);
    printf(BOX_V "  " C_CYAN "[1]" C_RESET " By ID              "
                  C_CYAN "[4]" C_RESET " By GPA (Low to High)"
           "  " BOX_V "\n");
    printf(BOX_V "  " C_CYAN "[2]" C_RESET " By Name (A to Z)   "
                  C_CYAN "[5]" C_RESET " By Age              "
           "  " BOX_V "\n");
    printf(BOX_V "  " C_CYAN "[3]" C_RESET " By GPA (High to Low)"
                  C_CYAN "[6]" C_RESET " By Course           "
           "  " BOX_V "\n");
    printf(BOX_V "  " C_CYAN "[7]" C_RESET " By Enrollment Year "
           "%*s" BOX_V "\n", BOX_WIDTH - 30, "");
    printHLine(BOX_BL, BOX_H, BOX_BR);
    printf("\n");

    int c = readInt("Sort by [1-7]: ", 1, 7);
    SortKey keys[] = {SORT_ID, SORT_NAME, SORT_GPA_DESC,
                      SORT_GPA_ASC, SORT_AGE, SORT_COURSE, SORT_YEAR};
    viewStudents(keys[c - 1]);
}

/* ── Statistics ──────────────────────────────────────────── */

void statistics(void) {
    Student *arr;
    int n = loadAll(&arr);

    clearScreen();
    printf("\n");
    printHLine(BOX_TL, BOX_H, BOX_TR);
    printBoxTitle("STATISTICS REPORT");
    printHLine(BOX_ML, BOX_H, BOX_MR);

    if (n == 0) {
        printBoxBlank();
        printBoxTitle("No student records found.");
        printBoxBlank();
        printHLine(BOX_BL, BOX_H, BOX_BR);
        pauseScreen();
        return;
    }

    float sum = 0, hi = arr[0].gpa, lo = arr[0].gpa;
    int hiIdx = 0, loIdx = 0;
    int gradeCount[5] = {0};

    for (int i = 0; i < n; i++) {
        sum += arr[i].gpa;
        if (arr[i].gpa > hi) { hi = arr[i].gpa; hiIdx = i; }
        if (arr[i].gpa < lo) { lo = arr[i].gpa; loIdx = i; }
        if      (arr[i].gpa >= 3.5f) gradeCount[0]++;
        else if (arr[i].gpa >= 3.0f) gradeCount[1]++;
        else if (arr[i].gpa >= 2.0f) gradeCount[2]++;
        else if (arr[i].gpa >= 1.0f) gradeCount[3]++;
        else                         gradeCount[4]++;
    }
    float avg = sum / n;

    printBoxBlank();
    printf(BOX_V "  " C_BOLD "Total Students  " C_RESET ": "
           C_BWHITE "%d" C_RESET "%*s" BOX_V "\n",
           n, BOX_WIDTH - 22 - (n > 99 ? 3 : n > 9 ? 2 : 1), "");
    printf(BOX_V "  " C_BOLD "Average GPA     " C_RESET ": "
           "%s%.2f" C_RESET " (%s)%*s" BOX_V "\n",
           gpaColor(avg), avg, gpaGrade(avg), BOX_WIDTH - 30, "");
    printf(BOX_V "  " C_BOLD "Highest GPA     " C_RESET ": "
           C_BGREEN "%.2f" C_RESET " - %s (%s)%*s" BOX_V "\n",
           hi, arr[hiIdx].name, arr[hiIdx].course,
           (int)(BOX_WIDTH - 27 - strlen(arr[hiIdx].name) - strlen(arr[hiIdx].course)), "");
    printf(BOX_V "  " C_BOLD "Lowest GPA      " C_RESET ": "
           C_BRED "%.2f" C_RESET " - %s (%s)%*s" BOX_V "\n",
           lo, arr[loIdx].name, arr[loIdx].course,
           (int)(BOX_WIDTH - 27 - strlen(arr[loIdx].name) - strlen(arr[loIdx].course)), "");
    printBoxBlank();
    printHLine(BOX_ML, BOX_H, BOX_MR);
    printBoxBlank();
    printf(BOX_V "  " C_BOLD C_BWHITE "GPA Distribution" C_RESET
           "%*s" BOX_V "\n", BOX_WIDTH - 19, "");
    printBoxBlank();

    const char *labels[]  = {"A  (>=3.5)", "B  (>=3.0)", "C  (>=2.0)", "D  (>=1.0)", "F  (<1.0) "};
    const char *barCols[] = {C_BGREEN, C_BBLUE, C_BYELLOW, C_YELLOW, C_BRED};
    int maxBar = 30;

    for (int g = 0; g < 5; g++) {
        int cnt  = gradeCount[g];
        int bars = (n > 0) ? (cnt * maxBar / n) : 0;
        int pct  = (n > 0) ? (cnt * 100 / n)   : 0;

        printf(BOX_V "  %s%-10s" C_RESET " %s",
               barCols[g], labels[g], barCols[g]);
        for (int b = 0; b < bars; b++) printf("\xe2\x96\x88");   /* UTF-8 █ */
        printf(C_RESET C_DIM " %d (%d%%)" C_RESET, cnt, pct);

        int used = 2 + 10 + 1 + bars + 2 + (cnt > 9 ? 2 : 1) + 6;
        int pad  = BOX_WIDTH - 2 - used;
        if (pad < 0) pad = 0;
        printf("%*s" BOX_V "\n", pad, "");
    }

    printBoxBlank();
    printHLine(BOX_BL, BOX_H, BOX_BR);
    free(arr);
    pauseScreen();
}

/* ── Export ──────────────────────────────────────────────── */

void exportCSV(void) {
    Student *arr;
    int n = loadAll(&arr);
    if (n == 0) { printWarning("No records to export."); pauseScreen(); return; }

    const char *path = "students_export.csv";
    FILE *f = fopen(path, "w");
    if (!f) { printError("Could not create export file."); free(arr); pauseScreen(); return; }

    fprintf(f, "ID,Name,Age,Course,GPA,Email,Phone,EnrollmentYear\n");
    for (int i = 0; i < n; i++) {
        Student *s = &arr[i];
        fprintf(f, "%d,\"%s\",%d,\"%s\",%.2f,\"%s\",\"%s\",%d\n",
                s->id, s->name, s->age, s->course,
                s->gpa, s->email, s->phone, s->year);
    }
    fclose(f);
    free(arr);

    printSuccess("Export complete!");
    printf("  " C_DIM "Saved to: " C_BCYAN "%s" C_RESET
           C_DIM "  (%d record%s)\n" C_RESET,
           path, n, n == 1 ? "" : "s");
    pauseScreen();
}

/* ── Import ──────────────────────────────────────────────── */

void importCSV(void) {
    clearScreen();
    printf("\n");
    printHLine(BOX_TL, BOX_H, BOX_TR);
    printBoxTitle("BULK CSV IMPORT");
    printHLine(BOX_ML, BOX_H, BOX_MR);
    printBoxBlank();
    printBoxTitle("Upload and append students from CSV file");
    printBoxBlank();
    printHLine(BOX_BL, BOX_H, BOX_BR);
    printf("\n");

    char path[128] = "";   /* readStringOpt leaves this untouched on empty input */
    readStringOpt("CSV file path (press Enter for 'students_export.csv'): ", path, sizeof(path));
    if (strlen(path) == 0) strncpy(path, "students_export.csv", sizeof(path) - 1);

    FILE *f = fopen(path, "r");
    if (!f) {
        printError("Could not open specified CSV file.");
        pauseScreen();
        return;
    }

    Student *arr;
    int n = loadAll(&arr);
    int currentNextId = nextId(arr, n);
    free(arr);

    char line[512];
    int imported = 0;
    int skipped = 0;
    int line_num = 0;

    FILE *out = fopen(DATA_FILE, "ab");
    if (!out) {
        fclose(f);
        printError("Could not open data store for writing.");
        pauseScreen();
        return;
    }

    while (fgets(line, sizeof(line), f)) {
        line_num++;
        if (line_num == 1 && isCSVHeader(line)) {
            continue;
        }
        if (line[0] == '\r' || line[0] == '\n' || line[0] == '\0') continue;

        Student s;
        if (parseCSVRow(line, &s)) {
            s.id = currentNextId++;
            fwrite(&s, sizeof(Student), 1, out);
            imported++;
        } else {
            skipped++;
        }
    }
    fclose(f);
    fclose(out);

    if (imported > 0) {
        printSuccess("Bulk import completed!");
        printf("  " C_BGREEN "✔ Successfully imported : %d student%s\n" C_RESET,
               imported, imported == 1 ? "" : "s");
        if (skipped > 0) {
            printf("  " C_YELLOW "⚠ Skipped invalid rows   : %d\n" C_RESET, skipped);
        }
    } else {
        printWarning("No valid student rows found in CSV.");
        if (skipped > 0) {
            printf("  " C_YELLOW "⚠ Skipped invalid rows : %d\n" C_RESET, skipped);
        }
    }
    pauseScreen();
}

/* ── Top-N Leaderboard ───────────────────────────────────── */

void topStudents(void) {
    Student *arr;
    int n = loadAll(&arr);
    if (n == 0) { printWarning("No students on record."); pauseScreen(); return; }

    printf("\n");
    int topN = readInt("Show top how many students? (1-50): ", 1, 50);
    if (topN > n) topN = n;

    /* Sort by GPA descending */
    sortArr(arr, n, SORT_GPA_DESC);

    clearScreen();
    printf("\n");
    printHLine(BOX_TL, BOX_H, BOX_TR);

    char title[64];
    snprintf(title, sizeof(title), "TOP %d STUDENTS BY GPA", topN);
    printBoxTitle(title);
    printHLine(BOX_ML, BOX_H, BOX_MR);

    /* Header */
    printf(C_CYAN BOX_V C_RESET
           C_BOLD C_BWHITE
           " %-4s  %-22s  %-18s  %-6s  %-4s "
           C_RESET C_CYAN BOX_V C_RESET "\n",
           "Rank", "Name", "Course", "GPA", "Year");
    printf(C_CYAN BOX_ML);
    for (int i = 0; i < BOX_WIDTH - 2; i++) printf(BOX_H);
    printf(BOX_MR C_RESET "\n");

    for (int i = 0; i < topN; i++) {
        Student *s = &arr[i];
        const char *medal = "";
        if      (i == 0) medal = C_BYELLOW " \xf0\x9f\xa5\x87 " C_RESET; /* gold   */
        else if (i == 1) medal = C_BWHITE  " \xf0\x9f\xa5\x88 " C_RESET; /* silver */
        else if (i == 2) medal = C_YELLOW  " \xf0\x9f\xa5\x89 " C_RESET; /* bronze */

        const char *dim = (i % 2 == 0) ? C_DIM : C_RESET;
        if (i < 3)
            printf(C_CYAN BOX_V C_RESET " %s%-2d%s  %-22.*s  %-18.*s  %s%6.2f%s  %-4d "
                   C_CYAN BOX_V C_RESET "\n",
                   medal, i + 1, C_RESET,
                   22, s->name, 18, s->course,
                   gpaColor(s->gpa), s->gpa, C_RESET, s->year);
        else
            printf(C_CYAN BOX_V C_RESET "%s"
                   " %-4d  %-22.*s  %-18.*s  %s%6.2f%s  %-4d "
                   C_RESET C_CYAN BOX_V C_RESET "\n",
                   dim, i + 1,
                   22, s->name, 18, s->course,
                   gpaColor(s->gpa), s->gpa, C_RESET, s->year);
    }

    printf(C_CYAN BOX_BL);
    for (int i = 0; i < BOX_WIDTH - 2; i++) printf(BOX_H);
    printf(BOX_BR C_RESET "\n");

    free(arr);
    pauseScreen();
}

/* ── Course Roster ───────────────────────────────────────── */

void courseRoster(void) {
    Student *arr;
    int n = loadAll(&arr);
    if (n == 0) { printWarning("No students on record."); pauseScreen(); return; }

    printf("\n");
    char query[64];
    readString("Course name (partial match): ", query, sizeof(query));

    /* Collect matches */
    Student *matches = malloc(n * sizeof(Student));
    int mn = 0;
    for (int i = 0; i < n; i++)
        if (containsCI(arr[i].course, query)) matches[mn++] = arr[i];

    free(arr);

    if (mn == 0) {
        printWarning("No students found for that course.");
        free(matches); pauseScreen(); return;
    }

    /* Compute stats */
    float sum = 0, hi = matches[0].gpa, lo = matches[0].gpa;
    int hiIdx = 0;
    for (int i = 0; i < mn; i++) {
        sum += matches[i].gpa;
        if (matches[i].gpa > hi) { hi = matches[i].gpa; hiIdx = i; }
        if (matches[i].gpa < lo)   lo = matches[i].gpa;
    }
    float avg = sum / mn;

    char title[80];
    snprintf(title, sizeof(title), "COURSE ROSTER: %s", query);

    /* Convert to uppercase for title */
    for (int i = 0; title[i]; i++)
        title[i] = (char)toupper((unsigned char)title[i]);

    displayTable(matches, mn, SORT_GPA_DESC, title);

    /* Summary panel */
    clearScreen();
    printf("\n");
    printHLine(BOX_TL, BOX_H, BOX_TR);
    printBoxTitle("COURSE SUMMARY");
    printHLine(BOX_ML, BOX_H, BOX_MR);
    printBoxBlank();
    printf(BOX_V "  " C_BOLD "Enrolled Students : " C_RESET
           C_BWHITE "%d" C_RESET "%*s" BOX_V "\n",
           mn, BOX_WIDTH - 24 - (mn > 9 ? 2 : 1), "");
    printf(BOX_V "  " C_BOLD "Average GPA       : " C_RESET
           "%s%.2f" C_RESET " (%s)%*s" BOX_V "\n",
           gpaColor(avg), avg, gpaGrade(avg), BOX_WIDTH - 32, "");
    printf(BOX_V "  " C_BOLD "Highest GPA       : " C_RESET
           C_BGREEN "%.2f" C_RESET " - %s%*s" BOX_V "\n",
           hi, matches[hiIdx].name,
           (int)(BOX_WIDTH - 29 - strlen(matches[hiIdx].name)), "");
    printf(BOX_V "  " C_BOLD "Lowest GPA        : " C_RESET
           C_BRED "%.2f" C_RESET "%*s" BOX_V "\n",
           lo, BOX_WIDTH - 27, "");
    printBoxBlank();
    printHLine(BOX_BL, BOX_H, BOX_BR);

    free(matches);
    pauseScreen();
}
