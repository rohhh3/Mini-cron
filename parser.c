#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>
#include <stdbool.h>
#include "parser.h"
#include "task.h"

int parse_tasks(char* filename, Task* tasks, unsigned int* number_of_tasks)
{
    // Try to open a taskfile
    int file = open(filename, O_RDONLY);
    if(file == -1)
    {
        fprintf(stderr, "Error: Failed to open file '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    // Count tasks
    char c;
    while(read(file, &c, 1) > 0)
        if(c == '\n')
            (*number_of_tasks)++;

    if(*number_of_tasks >= MAX_TASKS)
    {
        fprintf(stderr, "Error: Too many tasks, max: %d; current: %d", MAX_TASKS, *number_of_tasks);
        exit(EXIT_FAILURE);
    }

    // Set the file position indicator to the beginning of the file
    lseek(file, 0, SEEK_SET);

    // Parse tasks from taskfile
    char line[MAX_LINE_LENGTH];
    int i = 0;
    int hour = 0, minute = 0, mode = 0;
    char command[MAX_COMMAND_LENGTH + 1];
    unsigned short int n;
    bool has_newline;

    while(read(file, &c, 1) > 0 && i < MAX_TASKS)
    {
        n = 0;
        has_newline = 0;

        // Add whitespace at the beginning of the line
        while(n < MAX_LINE_LENGTH)
        {
            if(c == '\n')
            {
                has_newline = 1;
                break;
            }
            line[n++] = c;
            if(read(file, &c, 1) <= 0)
                break;
        }
        // If the line is empty, skip to the next line
        if(n == 0 && !has_newline)
            continue;


        line[n] = '\0';

        // Log handling
        if(sscanf(line, "%d:%d:%[^:]:%d", &hour, &minute, command, &mode) != 4)
        {
            syslog(LOG_WARNING, "Error: Failed to parse line from taskfile: %s", line);
            continue;
        }
        else if(hour < 0 || hour > 23 || minute < 0 || minute > 59)
        {
            syslog(LOG_WARNING, "Error: Invalid time in line from taskfile: %s", line);
            continue;
        }
        else if(mode < 0 || mode > 2)
        {
            syslog(LOG_WARNING, "Error: Invalid mode in line from taskfile: %s", line);
            continue;
        }
        else if(strlen(command) > MAX_COMMAND_LENGTH || strlen(command) < 1)
        {
            syslog(LOG_WARNING, "Error: Command too long in line from taskfile: %s", line);
            continue;
        }
        tasks[i].hour = hour;
        tasks[i].minute = minute;
        strncpy(tasks[i].command, command, MAX_COMMAND_LENGTH + 1);
        tasks[i].mode = mode;
        i++;

    }
    close(file);
    return *number_of_tasks;
}

void sort_tasks(Task* tasks, int number_of_tasks)
{
    for (int i = 0; i < number_of_tasks; i++)
    {
        for (int j = 0; j < number_of_tasks - 1; j++)
        {
            if (tasks[j].hour > tasks[j + 1].hour)
            {
                Task temp = tasks[j];
                tasks[j] = tasks[j + 1];
                tasks[j + 1] = temp;
            }
            else if (tasks[j].hour == tasks[j + 1].hour)
            {
                if (tasks[j].minute > tasks[j + 1].minute)
                {
                    Task temp = tasks[j];
                    tasks[j] = tasks[j + 1];
                    tasks[j + 1] = temp;
                }
            }
        }
    }
}
