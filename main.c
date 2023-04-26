#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_TASKS 100
#define MAX_COMMAND_LENGTH 100
#define MAX_LINE_LENGTH (MAX_COMMAND_LENGTH + 3)

typedef struct {
    int  hour;
    int  minute;
    char command[MAX_COMMAND_LENGTH + 1];
    int  mode;
} Task;

Task tasks[MAX_TASKS];
int number_of_tasks = 1;

void sig_handler(int signal) 
{
    if(signal == SIGINT) 
    {
        syslog(LOG_INFO, "Received SIGINT signal. Daemon has been terminated");
        exit(0);
    }
}

void parse_tasks(char* filename, Task* tasks, int* number_of_tasks)
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
        fprintf(stderr, "Too many tasks, max: %d; current: %d", MAX_TASKS, *number_of_tasks);
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
            syslog(LOG_WARNING, "Failed to parse line from taskfile: %s", line);
            continue;
        }
        else if(hour < 0 || hour > 23 || minute < 0 || minute > 59) 
        {
            syslog(LOG_WARNING, "Invalid time in line from taskfile: %s", line);
            continue;
        }
        else if(mode < 0 || mode > 2)
        {
            syslog(LOG_WARNING, "Invalid mode in line from taskfile: %s", line);
            continue;
        }
        else if(strlen(command) > MAX_COMMAND_LENGTH || strlen(command) < 1)
        {
            syslog(LOG_WARNING, "Command too long in line from taskfile: %s", line);
            continue;
        }
        tasks[i].hour = hour;
        tasks[i].minute = minute;
        strncpy(tasks[i].command, command, MAX_COMMAND_LENGTH + 1);
        tasks[i].mode = mode;
        i++;
    
    }
    close(file);
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

void execute_tasks(Task* tasks, int number_of_tasks)
{
    pid_t pid;
    for (int i = 0; i < number_of_tasks; i++)
    {
        time_t now;
        time(&now);
        struct tm *current_time = localtime(&now);
        
        // If task is in the past, skip it
        if(current_time->tm_hour > tasks[i].hour || (current_time->tm_hour == tasks[i].hour && current_time->tm_min >= tasks[i].minute))
            continue;
        
        int seconds_until_task = (tasks[i].hour - current_time->tm_hour) * 3600 + (tasks[i].minute - current_time->tm_min) * 60;
        sleep(seconds_until_task);

        pid = fork();
        if (pid == -1)
            syslog(LOG_ERR, "Failed to fork process to execute task: %s", tasks[i].command);

        // Child process
        else if(pid == 0)
        {
            execl("/bin/sh", "/bin/sh", "-c", tasks[i].command, NULL);
            exit(EXIT_FAILURE);
        }

        // Parent process
        else
        {
            int status;
            waitpid(pid, &status, 0);
            if(WIFEXITED(status))
                if (WIFEXITED(status) != 0)
                    syslog(LOG_WARNING, "Task exited with non-zero status: %s", tasks[i].command);
                    
            else if (WIFSIGNALED(status))
                syslog(LOG_WARNING, "Task terminated due to signal: %s", tasks[i].command);
        }
    }
}

int main(int argc, char *argv[])
{
    char* filename = argv[1];
    parse_tasks(filename, tasks, &number_of_tasks);
    sort_tasks(tasks, number_of_tasks);

    // Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Set signal handler for SIGINT
    signal(SIGINT, sig_handler);

    execute_tasks(tasks, number_of_tasks);
    
    return 0;
}