# Student Management System - Makefile
CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c99 -Iinclude -MMD -MP

SRCS = src/main.c src/login.c src/student.c src/ui.c src/backup.c src/sha256.c
OBJS = $(SRCS:.c=.o)
TARGET = student_mgmt

TEST_SRCS = tests/test_all.c src/sha256.c src/student.c src/ui.c src/backup.c
TEST_TARGET = test_runner

ifeq ($(OS),Windows_NT)
    TARGET := $(TARGET).exe
    TEST_TARGET := $(TEST_TARGET).exe
    RM = cmd /C del /Q /F
    RMDIR = cmd /C rmdir /S /Q
else
    RM = rm -f
    RMDIR = rm -rf
endif

.PHONY: all run test clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
ifeq ($(OS),Windows_NT)
	.\$(TARGET)
else
	./$(TARGET)
endif

$(TEST_TARGET): $(TEST_SRCS)
	$(CC) $(CFLAGS) -o $@ $(TEST_SRCS)

test: $(TEST_TARGET)
ifeq ($(OS),Windows_NT)
	.\$(TEST_TARGET)
else
	./$(TEST_TARGET)
endif

clean:
ifeq ($(OS),Windows_NT)
	-$(RM) $(TARGET) $(TEST_TARGET) test_runner.d src\*.o src\*.d students.dat admin.cfg students_export.csv 2>nul
	-$(RMDIR) backups 2>nul
else
	$(RM) $(TARGET) $(TEST_TARGET) test_runner.d src/*.o src/*.d students.dat admin.cfg students_export.csv
	$(RMDIR) backups
endif

-include $(OBJS:.o=.d)
