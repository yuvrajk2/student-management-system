/*
 * tests/test_all.c — Automated unit test suite for Student Management System
 * Tests cryptographic hashing, student data validation, CSV parsing, and GPA utilities.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "sha256.h"
#include "student.h"
#include "ui.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { \
        tests_passed++; \
        printf("  \033[1;32m[PASS]\033[0m %s\n", msg); \
    } else { \
        printf("  \033[1;31m[FAIL]\033[0m %s (Line %d)\n", msg, __LINE__); \
    } \
} while(0)

/* ── Test SHA-256 (NIST Test Vectors) ────────────────────── */

static void test_sha256_empty_string(void) {
    char hash[SHA256_HEX_SIZE];
    sha256_string("", hash);
    /* NIST standard hash for empty string */
    TEST_ASSERT(strcmp(hash, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855") == 0,
                "SHA-256 empty string matches NIST vector");
}

static void test_sha256_abc(void) {
    char hash[SHA256_HEX_SIZE];
    sha256_string("abc", hash);
    /* NIST standard hash for "abc" */
    TEST_ASSERT(strcmp(hash, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") == 0,
                "SHA-256 'abc' matches NIST vector");
}

static void test_sha256_quick_brown_fox(void) {
    char hash[SHA256_HEX_SIZE];
    sha256_string("The quick brown fox jumps over the lazy dog", hash);
    TEST_ASSERT(strcmp(hash, "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592") == 0,
                "SHA-256 'quick brown fox' matches standard vector");
}

static void test_sha256_salted_differentiation(void) {
    char hash1[SHA256_HEX_SIZE], hash2[SHA256_HEX_SIZE];
    hash_password_salted("admin123", "salt_one_1234567", hash1);
    hash_password_salted("admin123", "salt_two_1234567", hash2);
    TEST_ASSERT(strcmp(hash1, hash2) != 0,
                "Salted hashing produces unique digests for different salts");
}

/* ── Test Student Validation ─────────────────────────────── */

static void test_student_valid(void) {
    Student s = {1, "Alice Smith", 20, "Computer Science", 3.85f, "alice@test.edu", "555-0101", 2023};
    TEST_ASSERT(validateStudent(&s, NULL, 0) == 1,
                "Valid student passes validation");
}

static void test_student_invalid_age(void) {
    Student s_young = {1, "Bob", 12, "CS", 3.0f, "bob@test.com", "555-0102", 2023};
    Student s_old   = {1, "Bob", 105, "CS", 3.0f, "bob@test.com", "555-0102", 2023};
    TEST_ASSERT(validateStudent(&s_young, NULL, 0) == 0,
                "Age under 15 fails validation");
    TEST_ASSERT(validateStudent(&s_old, NULL, 0) == 0,
                "Age over 100 fails validation");
}

static void test_student_invalid_gpa(void) {
    Student s_neg = {1, "Charlie", 20, "CS", -0.5f, "charlie@test.com", "555-0103", 2023};
    Student s_high = {1, "Charlie", 20, "CS", 4.5f, "charlie@test.com", "555-0103", 2023};
    TEST_ASSERT(validateStudent(&s_neg, NULL, 0) == 0,
                "GPA below 0.00 fails validation");
    TEST_ASSERT(validateStudent(&s_high, NULL, 0) == 0,
                "GPA above 4.00 fails validation");
}

static void test_student_empty_name(void) {
    Student s = {1, "", 20, "CS", 3.5f, "noname@test.com", "555-0104", 2023};
    TEST_ASSERT(validateStudent(&s, NULL, 0) == 0,
                "Empty student name fails validation");
}

/* ── Test GPA Grades & Color ─────────────────────────────── */

static void test_gpa_grades(void) {
    TEST_ASSERT(strcmp(gpaGrade(3.9f), "A") == 0, "GPA 3.9 maps to grade A");
    TEST_ASSERT(strcmp(gpaGrade(3.2f), "B") == 0, "GPA 3.2 maps to grade B");
    TEST_ASSERT(strcmp(gpaGrade(2.5f), "C") == 0, "GPA 2.5 maps to grade C");
    TEST_ASSERT(strcmp(gpaGrade(1.5f), "D") == 0, "GPA 1.5 maps to grade D");
    TEST_ASSERT(strcmp(gpaGrade(0.5f), "F") == 0, "GPA 0.5 maps to grade F");
}

/* ── Test CSV Row Parsing ────────────────────────────────── */

static void test_csv_parser_valid_quoted(void) {
    const char *line = "101,\"Diana Prince\",22,\"Data Science\",3.95,\"diana@amazon.edu\",\"555-0099\",2022";
    Student s;
    int ok = parseCSVRow(line, &s);
    TEST_ASSERT(ok == 1, "CSV parser successfully parses 8-field quoted row");
    TEST_ASSERT(strcmp(s.name, "Diana Prince") == 0, "CSV parser correctly extracted name");
    TEST_ASSERT(s.age == 22, "CSV parser correctly extracted age");
    TEST_ASSERT(strcmp(s.course, "Data Science") == 0, "CSV parser correctly extracted course");
    TEST_ASSERT(s.gpa >= 3.94f && s.gpa <= 3.96f, "CSV parser correctly extracted GPA");
    TEST_ASSERT(s.year == 2022, "CSV parser correctly extracted year");
}

static void test_csv_parser_malformed(void) {
    const char *bad_line = "not,enough,fields";
    Student s;
    TEST_ASSERT(parseCSVRow(bad_line, &s) == 0, "CSV parser rejects malformed row");

    /* Regression: 9+ fields used to write past the end of the fields[] buffer */
    const char *too_many = "1,Ann,20,CS,3.50,a@b.c,123,2022,extra";
    TEST_ASSERT(parseCSVRow(too_many, &s) == 0, "CSV parser rejects rows with more than 8 fields");
}

static void test_csv_parser_escaped_quotes(void) {
    const char *line = "1,\"Ann \"\"AJ\"\" Lee\",20,\"CS, Hons\",3.50,\"a@b.c\",\"123\",2022";
    Student s;
    TEST_ASSERT(parseCSVRow(line, &s) == 1, "CSV parser accepts row with escaped quotes");
    TEST_ASSERT(strcmp(s.name, "Ann \"AJ\" Lee") == 0, "CSV parser unescapes doubled quotes in name");
    TEST_ASSERT(strcmp(s.course, "CS, Hons") == 0, "CSV parser keeps comma inside quoted course");
}

/* ── Main Runner ─────────────────────────────────────────── */

int main(void) {
    printf("\n==================================================\n");
    printf("   STUDENT MANAGEMENT SYSTEM - UNIT TEST SUITE    \n");
    printf("==================================================\n\n");

    printf("--- [1] Cryptography (SHA-256) Tests ---\n");
    test_sha256_empty_string();
    test_sha256_abc();
    test_sha256_quick_brown_fox();
    test_sha256_salted_differentiation();

    printf("\n--- [2] Student Validation Tests ---\n");
    test_student_valid();
    test_student_invalid_age();
    test_student_invalid_gpa();
    test_student_empty_name();

    printf("\n--- [3] GPA Grading & Metrics Tests ---\n");
    test_gpa_grades();

    printf("\n--- [4] CSV Parser Tests ---\n");
    test_csv_parser_valid_quoted();
    test_csv_parser_malformed();
    test_csv_parser_escaped_quotes();

    printf("\n--------------------------------------------------\n");
    printf("RESULTS: %d / %d tests passed (%.1f%%)\n",
           tests_passed, tests_run, (float)tests_passed * 100.0f / (float)tests_run);
    printf("--------------------------------------------------\n\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
