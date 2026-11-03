/*
 * login.c — Authentication with masked password + SHA-256 salted password hashing
 *
 * Cross-platform: uses termios on POSIX, conio.h on Windows.
 * Password storage: admin.cfg ($sha256$<salt>$<hash>).
 * Transparently upgrades legacy plaintext passwords to salted SHA-256 hashes.
 */
#ifdef _WIN32
#define _CRT_RAND_S   /* enables rand_s() in <stdlib.h> */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "login.h"
#include "ui.h"
#include "sha256.h"

#ifdef _WIN32
  #include <conio.h>
#else
  #include <termios.h>
  #include <unistd.h>
#endif

#define ADMIN_USER   "admin"
#define DEFAULT_PASS "admin123"
#define PASS_FILE    "admin.cfg"
#define SALT_LEN     16

/* ── Salt & Hash Helpers ─────────────────────────────────── */

/* Fill buf with n bytes from the operating system's random source. */
static int osRandomBytes(unsigned char *buf, size_t n) {
#ifdef _WIN32
    for (size_t i = 0; i < n; i++) {
        unsigned int v;
        if (rand_s(&v) != 0) return 0;
        buf[i] = (unsigned char)(v & 0xFF);
    }
    return 1;
#else
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f) return 0;
    size_t got = fread(buf, 1, n, f);
    fclose(f);
    return got == n;
#endif
}

/* Salts must be unpredictable and differ between runs. rand() seeded with
 * time() repeats whenever two salts are generated in the same second, so use
 * the OS random source and only fall back to rand() if that is unavailable. */
static void generateSalt(char *salt, int len) {
    static const char hex[] = "0123456789abcdef";
    unsigned char rnd[64];
    int have_os = (len > 0 && (size_t)len <= sizeof(rnd)) && osRandomBytes(rnd, (size_t)len);

    if (!have_os) {
        static int seeded = 0;
        if (!seeded) { srand((unsigned)time(NULL) ^ (unsigned)clock()); seeded = 1; }
    }
    for (int i = 0; i < len; i++) {
        unsigned v = have_os ? rnd[i] : (unsigned)rand();
        salt[i] = hex[v % 16];
    }
    salt[len] = '\0';
}

static int saveSaltedPassword(const char *plain) {
    char salt[SALT_LEN + 1];
    char hash[SHA256_HEX_SIZE];
    generateSalt(salt, SALT_LEN);
    hash_password_salted(plain, salt, hash);

    FILE *f = fopen(PASS_FILE, "w");
    if (!f) return 0;
    fprintf(f, "$sha256$%s$%s\n", salt, hash);
    fclose(f);
    return 1;
}

static int verifyPassword(const char *input) {
    FILE *f = fopen(PASS_FILE, "r");
    if (!f) {
        /* Default password check: if match, initialize admin.cfg with salted hash */
        if (strcmp(input, DEFAULT_PASS) == 0) {
            saveSaltedPassword(DEFAULT_PASS);
            return 1;
        }
        return 0;
    }

    char line[256];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return (strcmp(input, DEFAULT_PASS) == 0);
    }
    fclose(f);
    line[strcspn(line, "\r\n")] = '\0';

    if (strncmp(line, "$sha256$", 8) == 0) {
        char salt[64], stored_hash[128];
        const char *p = line + 8;
        const char *next = strchr(p, '$');
        if (!next) return 0;
        int salt_len = (int)(next - p);
        if (salt_len >= (int)sizeof(salt)) return 0;
        strncpy(salt, p, salt_len);
        salt[salt_len] = '\0';
        strncpy(stored_hash, next + 1, sizeof(stored_hash) - 1);
        stored_hash[sizeof(stored_hash) - 1] = '\0';

        char computed_hash[SHA256_HEX_SIZE];
        hash_password_salted(input, salt, computed_hash);
        return (strcmp(computed_hash, stored_hash) == 0);
    } else {
        /* Legacy plaintext password found -> verify and upgrade automatically */
        if (strcmp(input, line) == 0) {
            saveSaltedPassword(input);
            return 1;
        }
        return 0;
    }
}

/* ── Masked input ────────────────────────────────────────── */

static void maskedInput(char *buf, int maxlen) {
    int i = 0;

#ifdef _WIN32
    int c;
    while (i < maxlen - 1) {
        c = _getch();
        if (c == '\r' || c == '\n') break;
        if (c == 8 || c == 127) {          /* backspace */
            if (i > 0) { i--; printf("\b \b"); fflush(stdout); }
        } else {
            buf[i++] = (char)c;
            printf("*"); fflush(stdout);
        }
    }
    printf("\n");
#else
    struct termios old, noecho;
    tcgetattr(STDIN_FILENO, &old);
    noecho = old;
    noecho.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &noecho);

    int c;
    while (i < maxlen - 1) {
        c = getchar();
        if (c == '\n' || c == EOF) break;
        if (c == 127 || c == '\b') {
            if (i > 0) { i--; printf("\b \b"); fflush(stdout); }
        } else {
            buf[i++] = (char)c;
            printf("*"); fflush(stdout);
        }
    }
    printf("\n");
    tcsetattr(STDIN_FILENO, TCSANOW, &old);
#endif

    buf[i] = '\0';
}

/* ── Public: login ───────────────────────────────────────── */

int login(char *out_user, int out_size) {
    char username[64];
    char password[128];

    clearScreen();
    printf("\n");
    printHLine(BOX_TL, BOX_H, BOX_TR);
    printBoxBlank();
    printBoxTitle("STUDENT MANAGEMENT SYSTEM  v2.0");
    printBoxBlank();
    printBoxTitle("Please sign in to continue");
    printBoxBlank();
    printHLine(BOX_BL, BOX_H, BOX_BR);
    printf("\n");

    readString("Username : ", username, sizeof(username));

    printf("  Password : ");
    fflush(stdout);
    maskedInput(password, sizeof(password));

    if (strcmp(username, ADMIN_USER) == 0 && verifyPassword(password)) {
        strncpy(out_user, username, out_size - 1);
        out_user[out_size - 1] = '\0';
        return 1;
    }
    return 0;
}

/* ── Public: changePassword ──────────────────────────────── */

void changePassword(void) {
    clearScreen();
    printf("\n");
    printHLine(BOX_TL, BOX_H, BOX_TR);
    printBoxTitle("CHANGE PASSWORD (SHA-256 SECURED)");
    printHLine(BOX_ML, BOX_H, BOX_MR);
    printBoxBlank();
    printHLine(BOX_BL, BOX_H, BOX_BR);
    printf("\n");

    char current[128];
    printf("  Current password : ");
    fflush(stdout);
    maskedInput(current, sizeof(current));

    if (!verifyPassword(current)) {
        printError("Incorrect current password.");
        pauseScreen();
        return;
    }

    char newpass[128], confirm[128];

    printf("  New password     : ");
    fflush(stdout);
    maskedInput(newpass, sizeof(newpass));

    if (strlen(newpass) < 4) {
        printError("Password must be at least 4 characters.");
        pauseScreen();
        return;
    }

    printf("  Confirm password : ");
    fflush(stdout);
    maskedInput(confirm, sizeof(confirm));

    if (strcmp(newpass, confirm) != 0) {
        printError("Passwords do not match.");
        pauseScreen();
        return;
    }

    if (saveSaltedPassword(newpass)) {
        printSuccess("Password hashed & updated successfully.");
        printInfo("Protected with salted SHA-256 digest.");
    } else {
        printError("Failed to save password configuration.");
    }
    pauseScreen();
}
