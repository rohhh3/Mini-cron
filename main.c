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


void parse_tasks(char* filename) //change function to return Task objects
{
    // Try to open a taskfile
    int file = open(filename, O_RDONLY);
    if (file == -1) 
    {
        fprintf(stderr, "Error: Failed to open file '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

    // Count tasks
    int number_of_tasks = 0;
    char c;
    while (read(file, &c, 1) > 0) 
        if (c == '\n') 
            number_of_tasks++;

    if (number_of_tasks >= MAX_TASKS)
    {
        fprintf(stderr, "Too many tasks, max: %d; current: %d", MAX_TASKS, number_of_tasks);
        exit(EXIT_FAILURE);
    }
	
	// Set the file position indicator to the beginning of the file
    lseek(file, 0, SEEK_SET); 

    // Parse tasks from taskfile
    Task tasks[MAX_TASKS];
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

    // Test (edit tasks.txt to change results)
    for (int j = 0; j < i; j++) 
        printf("%d:%d, %s, %d \n", tasks[j].hour, tasks[j].minute, tasks[j].command, tasks[j].mode);

    close(file);
}
int main(int argc, char *argv[])
{
    char* filename = argv[1];
	parse_tasks(filename);
}
