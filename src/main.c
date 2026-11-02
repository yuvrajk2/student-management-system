/*
 * main.c — Entry point, login flow, main menu
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ui.h"
#include "login.h"
#include "student.h"
#include "backup.h"

#define MAX_ATTEMPTS 3

/* ── Status bar ──────────────────────────────────────────── */

static void printStatusBar(const char *user) {
    int   total = countStudents();
    float avg   = avgGPA();

    /* Build right-side string without ANSI (to measure length) */
    char right_plain[64];
    snprintf(right_plain, sizeof(right_plain),
             "Students: %d   Avg GPA: %.2f  ", total, avg);

    char left_plain[64];
    snprintf(left_plain, sizeof(left_plain), "  Signed in as: %s", user);

    int gap = BOX_WIDTH - 2 - (int)strlen(left_plain) - (int)strlen(right_plain);
    if (gap < 1) gap = 1;

    printf(BOX_V "  Signed in as: " C_BWHITE "%s" C_RESET
           "%*s"
           C_DIM "Students: " C_RESET C_BWHITE "%d" C_RESET
           C_DIM "   Avg GPA: " C_RESET "%s%.2f" C_RESET "  "
           BOX_V "\n",
           user, gap, "", total, gpaColor(avg), avg);
}

/* ── Main menu ───────────────────────────────────────────── */

static void displayMainMenu(const char *user) {
    clearScreen();
    printf("\n");
    printHLine(BOX_TL, BOX_H, BOX_TR);
    printBoxTitle("STUDENT MANAGEMENT SYSTEM  v2.0");
    printHLine(BOX_ML, BOX_H, BOX_MR);
    printStatusBar(user);
    printHLine(BOX_ML, BOX_H, BOX_MR);
    printBoxBlank();

    printf(BOX_V "  " C_CYAN "[ 1]" C_RESET " Add Student         "
                  C_CYAN "[ 7]" C_RESET " Statistics Report"
           "%*s" BOX_V "\n", BOX_WIDTH - 46, "");

    printf(BOX_V "  " C_CYAN "[ 2]" C_RESET " View All Students   "
                  C_CYAN "[ 8]" C_RESET " CSV Export / Import"
           "%*s" BOX_V "\n", BOX_WIDTH - 49, "");

    printf(BOX_V "  " C_CYAN "[ 3]" C_RESET " Search Student      "
                  C_CYAN "[ 9]" C_RESET " Top-N Leaderboard"
           "%*s" BOX_V "\n", BOX_WIDTH - 46, "");

    printf(BOX_V "  " C_CYAN "[ 4]" C_RESET " Update Student      "
                  C_CYAN "[10]" C_RESET " Course Roster"
           "%*s" BOX_V "\n", BOX_WIDTH - 43, "");

    printf(BOX_V "  " C_CYAN "[ 5]" C_RESET " Delete Student      "
                  C_CYAN "[11]" C_RESET " Backup & Restore"
           "%*s" BOX_V "\n", BOX_WIDTH - 45, "");

    printf(BOX_V "  " C_CYAN "[ 6]" C_RESET " Sort & View         "
                  C_CYAN "[12]" C_RESET " Change Password"
           "%*s" BOX_V "\n", BOX_WIDTH - 44, "");

    printBoxBlank();
    printHLine(BOX_ML, BOX_H, BOX_MR);
    printf(BOX_V "  " C_DIM "[ 0] Logout" C_RESET
           "   " C_DIM "[Q] Quit" C_RESET
           "%*s" BOX_V "\n", BOX_WIDTH - 25, "");
    printHLine(BOX_BL, BOX_H, BOX_BR);
    printf("\n");
}

/* ── Application loop ────────────────────────────────────── */

int main(void) {
    char user[64];

    while (1) {
        /* ── Login phase ── */
        int attempts = 0, authed = 0;

        while (attempts < MAX_ATTEMPTS) {
            authed = login(user, sizeof(user));
            if (authed) {
                printSuccess("Login successful! Welcome back.");
                break;
            }
            attempts++;
            int left = MAX_ATTEMPTS - attempts;
            if (left > 0)
                printf("\n  " C_BRED "Incorrect credentials. %d attempt%s remaining.\n" C_RESET,
                       left, left == 1 ? "" : "s");
            else
                printf("\n  " C_BRED "Too many failed attempts.\n" C_RESET);
        }

        if (!authed) {
            printf("\n  Exiting for security. Goodbye.\n\n");
            return 1;
        }

        /* ── Main menu loop ── */
        while (1) {
            displayMainMenu(user);

            printf("  " C_BOLD "Choice: " C_RESET);
            fflush(stdout);

            char line[16];
            if (!fgets(line, sizeof(line), stdin)) {
                /* EOF on stdin (Ctrl+D): quit instead of spinning forever */
                printf("\n\n  " C_DIM "Input closed. Goodbye." C_RESET "\n\n");
                return 0;
            }

            /* Handle Q/q for quit */
            if (tolower((unsigned char)line[0]) == 'q') {
                clearScreen();
                printf("\n  " C_DIM "Thank you for using SMS v2.0. Goodbye.\n\n" C_RESET);
                return 0;
            }

            int choice = atoi(line);

            if (choice == 0) { printInfo("Logged out."); break; }

            printf("\n");
            switch (choice) {
                case  1: addStudent();           break;
                case  2: viewStudents(SORT_ID);  break;
                case  3: searchStudent();        break;
                case  4: updateStudent(); pauseScreen(); break;
                case  5: deleteStudent(); pauseScreen(); break;
                case  6: sortMenu();             break;
                case  7: statistics();           break;
                case  8:
                    clearScreen();
                    printf("\n");
                    printHLine(BOX_TL, BOX_H, BOX_TR);
                    printBoxTitle("CSV DATA OPERATIONS");
                    printHLine(BOX_ML, BOX_H, BOX_MR);
                    printf(BOX_V "  " C_CYAN "[1]" C_RESET " Export to CSV File"
                           "%*s" BOX_V "\n", BOX_WIDTH - 24, "");
                    printf(BOX_V "  " C_CYAN "[2]" C_RESET " Bulk Import from CSV File"
                           "%*s" BOX_V "\n", BOX_WIDTH - 29, "");
                    printHLine(BOX_BL, BOX_H, BOX_BR);
                    printf("\n");
                    {
                        int sub = readInt("Choice [1-2]: ", 1, 2);
                        if (sub == 1) exportCSV();
                        else          importCSV();
                    }
                    break;
                case  9: topStudents();          break;
                case 10: courseRoster();         break;
                case 11:
                    clearScreen();
                    printf("\n");
                    printHLine(BOX_TL, BOX_H, BOX_TR);
                    printBoxTitle("BACKUP & RESTORE");
                    printHLine(BOX_ML, BOX_H, BOX_MR);
                    printf(BOX_V "  " C_CYAN "[1]" C_RESET " Create Backup"
                           "%*s" BOX_V "\n", BOX_WIDTH - 18, "");
                    printf(BOX_V "  " C_CYAN "[2]" C_RESET " Restore from Backup"
                           "%*s" BOX_V "\n", BOX_WIDTH - 24, "");
                    printHLine(BOX_BL, BOX_H, BOX_BR);
                    printf("\n");
                    {
                        int sub = readInt("Choice [1-2]: ", 1, 2);
                        if (sub == 1) createBackup();
                        else          restoreBackup();
                    }
                    break;
                case 12: changePassword();       break;
                default:
                    printWarning("Invalid choice — enter 1-12, 0, or Q.");
                    pauseScreen();
            }
        }
    }
    return 0;
}
