#pragma once
#define MAX_TASKS 100
#define MAX_COMMAND_LENGTH 100
#define MAX_LINE_LENGTH (MAX_COMMAND_LENGTH + 3)

typedef struct {
    int  hour;
    int  minute;
    char command[MAX_COMMAND_LENGTH + 1];
    int  mode;
} Task;
