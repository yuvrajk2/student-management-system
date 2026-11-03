/*
 * ui.c — TUI helper implementations
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui.h"

/* ── Terminal control ────────────────────────────────────── */

void clearScreen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

void pauseScreen(void) {
    printf("\n  " C_DIM "Press Enter to continue..." C_RESET);
    fflush(stdout);
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void flushInput(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* ── Box drawing ─────────────────────────────────────────── */

/* Prints a full-width border line, e.g. ╔══…══╗ */
void printHLine(char *left, char *fill, char *right) {
    printf("%s", left);
    for (int i = 0; i < BOX_WIDTH - 2; i++) printf("%s", fill);
    printf("%s\n", right);
}

/* Prints ║  <centred title>  ║ */
void printBoxTitle(const char *title) {
    int inner = BOX_WIDTH - 2;          /* chars between the two ║ */
    int tlen  = (int)strlen(title);
    int pad   = (inner - tlen) / 2;
    printf(BOX_V);
    for (int i = 0; i < pad; i++) putchar(' ');
    printf(C_BOLD C_BWHITE "%s" C_RESET, title);
    int used = pad + tlen;
    for (int i = used; i < inner; i++) putchar(' ');
    printf(BOX_V "\n");
}

void printBoxBlank(void) {
    printf(BOX_V);
    for (int i = 0; i < BOX_WIDTH - 2; i++) putchar(' ');
    printf(BOX_V "\n");
}

/* ── Status messages ─────────────────────────────────────── */

void printSuccess(const char *msg) {
    printf("\n  " C_BGREEN "✔  %s" C_RESET "\n", msg);
}
void printError(const char *msg) {
    printf("\n  " C_BRED "✖  %s" C_RESET "\n", msg);
}
void printWarning(const char *msg) {
    printf("\n  " C_BYELLOW "⚠  %s" C_RESET "\n", msg);
}
void printInfo(const char *msg) {
    printf("\n  " C_BCYAN "ℹ  %s" C_RESET "\n", msg);
}

/* ── Input helpers ───────────────────────────────────────── */


/* stdin was closed (Ctrl+D / Ctrl+Z / piped input ran out): exit cleanly
 * instead of re-prompting forever. */
static void exitOnEOF(void) {
    printf("\n\n  " C_DIM "Input closed. Goodbye." C_RESET "\n\n");
    exit(0);
}

int readInt(const char *prompt, int min, int max) {
    int val;
    while (1) {
        printf("  %s", prompt);
        fflush(stdout);
        int rc = scanf("%d", &val);
        if (rc == EOF) exitOnEOF();
        if (rc == 1) {
            flushInput();
            if (val >= min && val <= max) return val;
            printf("  " C_YELLOW "  Enter a value between %d and %d.\n" C_RESET, min, max);
        } else {
            flushInput();
            printf("  " C_YELLOW "  Invalid input — numbers only.\n" C_RESET);
        }
    }
}

float readFloat(const char *prompt, float min, float max) {
    float val;
    while (1) {
        printf("  %s", prompt);
        fflush(stdout);
        int rc = scanf("%f", &val);
        if (rc == EOF) exitOnEOF();
        if (rc == 1) {
            flushInput();
            if (val >= min && val <= max) return val;
            printf("  " C_YELLOW "  Enter a value between %.2f and %.2f.\n" C_RESET, min, max);
        } else {
            flushInput();
            printf("  " C_YELLOW "  Invalid input — number required.\n" C_RESET);
        }
    }
}

/* Read one line into buf (at most maxlen-1 chars, newline stripped).
 * If the user typed more than fits, the rest of the line is discarded so it
 * does not leak into the next prompt, and the input is reported as shortened.
 * Returns 0 on EOF, 1 otherwise. */
static int readLine(char *buf, int maxlen) {
    if (!fgets(buf, maxlen, stdin)) return 0;
    char *nl = strchr(buf, '\n');
    if (nl) {
        *nl = '\0';
    } else {
        int c = getchar();
        if (c != '\n' && c != EOF) {
            flushInput();
            printf("  " C_YELLOW "  Input too long - shortened to %d characters.\n" C_RESET, maxlen - 1);
        }
    }
    return 1;
}

void readString(const char *prompt, char *buf, int maxlen) {
    while (1) {
        printf("  %s", prompt);
        fflush(stdout);
        if (!readLine(buf, maxlen)) exitOnEOF();
        if (strlen(buf) > 0) return;
        printf("  " C_YELLOW "  Field cannot be empty.\n" C_RESET);
    }
}

void readStringOpt(const char *prompt, char *buf, int maxlen) {
    char tmp[256] = {0};
    printf("  %s", prompt);
    fflush(stdout);
    if (readLine(tmp, sizeof(tmp))) {
        if (strlen(tmp) > 0) {
            strncpy(buf, tmp, maxlen - 1);
            buf[maxlen - 1] = '\0';
        }
    }
}

/* ── GPA helpers ─────────────────────────────────────────── */

const char *gpaColor(float gpa) {
    if (gpa >= 3.5f) return C_BGREEN;
    if (gpa >= 2.5f) return C_BYELLOW;
    if (gpa >= 1.0f) return C_YELLOW;
    return C_BRED;
}

const char *gpaGrade(float gpa) {
    if (gpa >= 3.5f) return "A";
    if (gpa >= 3.0f) return "B";
    if (gpa >= 2.0f) return "C";
    if (gpa >= 1.0f) return "D";
    return "F";
}
