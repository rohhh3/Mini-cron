#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>

#define MAX_TASKS 100
#define MAX_COMMAND_LENGTH 100
#define MAX_MODE_LENGTH 2
#define MAX_LINE_LENGTH MAX_COMMAND_LENGTH + 2 + MAX_MODE_LENGTH + 1

typedef struct {
    int  hour;
    int  minute;
    char command[MAX_COMMAND_LENGTH + 1];
    int  mode;
} Task;

Task* parse_tasks(char* filename, Task* tasks, int* number_of_tasks)
{
    // Try to open a taskfile
    int file = open(filename, O_RDONLY);
    if (file == -1) 
    {
        fprintf(stderr, "Error: Failed to open file '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    // Count tasks
    char c;
    while (read(file, &c, 1) > 0) 
        if (c == '\n') 
            (*number_of_tasks)++;

    if (*number_of_tasks >= MAX_TASKS)
    {
        fprintf(stderr, "Too many tasks, max: %d; current: %d", MAX_TASKS, *number_of_tasks);
        exit(EXIT_FAILURE);
    }
    
	// Set the file position indicator to the beginning of the file
    lseek(file, 0, SEEK_SET); 

    // Parse tasks from taskfile
    char line[MAX_LINE_LENGTH];
    int i = 0;
    while(read(file, &c, 1) > 0 && i < MAX_TASKS) 
    {
        int hour = 0, minute = 0, mode = 0;
        char command[MAX_COMMAND_LENGTH + 1];
        int n = 0;

        while(c != '\n' && n < MAX_LINE_LENGTH - 1) 
        {
            line[n] = c;
            n++;
            read(file, &c, 1);
        }

        line[n] = '\0';

        if (sscanf(line, "%d:%d:%[^:]:%d", &hour, &minute, command, &mode) != 4) 
        {
            syslog(LOG_WARNING, "Failed to parse line from taskfile: %s", line);
            continue;
        }

        tasks[i].hour = hour;
		tasks[i].mode = mode;
        tasks[i].minute = minute;
        strncpy(tasks[i].command, command, MAX_COMMAND_LENGTH + 1);
        i++;
    }

    close(file);

    return tasks;
}

void sort_tasks(Task* const tasks, int number_of_tasks)
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

int main(int argc, char *argv[])
{
    char* filename = argv[1];
    Task tasks[MAX_TASKS];
    int number_of_tasks = 1;

    // TESTS
    parse_tasks(filename, tasks, &number_of_tasks);
    for (int i = 0; i < number_of_tasks; i++)
    {
        printf("%d:%d %s %d\n", tasks[i].hour, tasks[i].minute, tasks[i].command, tasks[i].mode);
    }
    printf("Tasks: %d\n\n", number_of_tasks);

    sort_tasks(tasks, number_of_tasks);
    for (int i = 0; i < number_of_tasks; i++)
    {
        printf("%d:%d %s %d\n", tasks[i].hour, tasks[i].minute, tasks[i].command, tasks[i].mode);
    }
    printf("Tasks: %d\n", number_of_tasks);
    return 0;
}
