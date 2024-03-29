#include <signal.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include "task.h"
#include "execute.h"
#include "parser.h"

void execute_tasks(Task* tasks, int number_of_tasks)
{
    openlog(NULL, LOG_PID, LOG_USER);

    // Open outfile
    int fd = open("outfile.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
    if(fd == -1) 
    {
        fprintf(stderr, "Error: Failed to open file outfile.txt\n");
        exit(EXIT_FAILURE);
    }

    // Create daemon
    pid_t pid = fork();
    if(pid < 0) 
    {
        fprintf(stderr, "Error: Failed to fork process\n");
        exit(EXIT_FAILURE);
    }

    // Exit parent process
    if(pid != 0) 
        exit(0);

    setsid();

    // close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    signal(SIGINT, sig_handler);
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler); 

    for(int i = 0; i < number_of_tasks; i++)
    {
        time_t now;
        time(&now);
        struct tm *current_time = localtime(&now);

        // If task is in the past, skip it
        if(current_time->tm_hour > tasks[i].hour || (current_time->tm_hour == tasks[i].hour && current_time->tm_min >= tasks[i].minute))
            continue;

        int seconds_until_task = (tasks[i].hour - current_time->tm_hour) * 3600 + (tasks[i].minute - current_time->tm_min) * 60;
        sleep(seconds_until_task);

        // Create pipe
        int pipefd[2];
        if(pipe(pipefd) == -1) 
        {
            fprintf(stderr, "Error: Failed to create a pipe\n");
            exit(EXIT_FAILURE);
        }

        pid_t pid = fork();
        if(pid < 0)
        {
            syslog(LOG_ERR, "Failed to fork process to execute tasks");
            exit(EXIT_FAILURE);
        }
        else if(pid == 0) 
        {
            switch(tasks[i].mode)
            {
                case 0:
                {
                    close(pipefd[0]);
                    dup2(pipefd[1], STDOUT_FILENO);
                    break;
                }
                case 1:
                {
                    close(pipefd[0]);
                    dup2(pipefd[1], STDERR_FILENO);
                    break;
                }
                case 2:
                {
                    close(pipefd[0]);
                    dup2(pipefd[1], STDOUT_FILENO);
                    dup2(pipefd[1], STDERR_FILENO);
                    break;
                }
            }
            /*
            char *args[MAX_ARGS];
            int arg_count = 0;
            char *arg = strtok(tasks[i].command, " "); 
            while (arg != NULL && arg_count < MAX_ARGS) {
                args[arg_count++] = arg;
                arg = strtok(NULL, " ");
            }
            args[arg_count] = NULL; 
            execvp(args[0], args);
            */
            char *args[] = {"sh", "-c", tasks[i].command, NULL};
            execvp(args[0], args);
            syslog(LOG_INFO, "Task has been executed successfully");
        }
        else if(pid > 0)
        {
            int status;
            waitpid(pid, &status, 0);
            if(WIFEXITED(status))
            {
                syslog(LOG_INFO, "Child process has been exited with status %d", WEXITSTATUS(status));
            }
            else if(WIFSIGNALED(status))
            {
                syslog(LOG_ERR, "Child process has been terminated by signal %d", WTERMSIG(status));
            }
            else if(WIFSTOPPED(status))
            {
                syslog(LOG_ERR, "Child process has been stopped by signal %d", WSTOPSIG(status));
            }

            // Read from pipe
            char buffer[1024];
            int n;
            
            switch(tasks[i].mode)
            {
                case 0:
                {
                    close(pipefd[1]);
                    while ((n = read(pipefd[0], buffer, sizeof(buffer))) > 0) 
                        dprintf(fd, "%.*s", n, buffer);

                    close(pipefd[0]);
                    dprintf(fd, "Task executed with mode %d at %02d:%02d: %s\n", tasks[i].mode, tasks[i].hour, tasks[i].minute, tasks[i].command);
                    syslog(LOG_INFO, "Task executed: %s (exit status: %d) at %02d:%02d", tasks[i].command, WEXITSTATUS(status), tasks[i].hour, tasks[i].minute);  
                    break;
                }
                case 1:
                {
                    close(pipefd[1]);
                    while ((n = read(pipefd[0], buffer, sizeof(buffer))) > 0) 
                        dprintf(fd, "%.*s", n, buffer);
            
                    close(pipefd[0]);
                    dprintf(fd, "Task %s hasn't been executed", tasks[i].command);
                    syslog(LOG_INFO, "Task %s hasn't been executed", tasks[i].command); 
                    break;
                }
                case 2:
                {
                    close(pipefd[1]);
                    while ((n = read(pipefd[0], buffer, sizeof(buffer))) > 0) 
                        dprintf(fd, "%.*s", n, buffer);
                
                    close(pipefd[0]);
                    dprintf(fd, "Task executed with mode %d at %02d:%02d: %s\n", tasks[i].mode, tasks[i].hour, tasks[i].minute, tasks[i].command);
                    syslog(LOG_INFO, "Task executed: %s (exit status: %d) at %02d:%02d", tasks[i].command, WEXITSTATUS(status), tasks[i].hour, tasks[i].minute);  
                    break;
                }
            }  
        }    
    }
}

void sig_handler(int signal)
{
    unsigned int buff = 1;
    switch(signal)
    {
        case SIGINT:
        {
            syslog(LOG_INFO, "Received SIGINT signal. Daemon has been terminated");
            exit(EXIT_SUCCESS);
            break;
        }
        case SIGUSR1:
        {
            syslog(LOG_INFO, "Received SIGUSR1 signal. Daemon has been reloaded.");
            Task tasks[MAX_TASKS];
            int number_of_tasks = parse_tasks("tasks.txt", tasks, &buff);
            sort_tasks(tasks, number_of_tasks);
            execute_tasks(tasks, number_of_tasks);
            break;
        }
        case SIGUSR2:
        {
            syslog(LOG_INFO, "Received SIGUSR2 signal. Remaining tasks:");
            Task tasks[MAX_TASKS];
            int number_of_tasks = parse_tasks("tasks.txt", tasks, &buff);
            syslog(LOG_INFO, "Number of tasks: %d", number_of_tasks);
            sort_tasks(tasks, number_of_tasks);
            for(int i = 0; i < number_of_tasks; i++)
            {
                time_t now;
                time(&now);
                struct tm *current_time = localtime(&now);

                // If task is in the past, skip it
                if(current_time->tm_hour > tasks[i].hour || (current_time->tm_hour == tasks[i].hour && current_time->tm_min >= tasks[i].minute))
                    continue;

                syslog(LOG_INFO, "%d:%d %s", tasks[i].hour, tasks[i].minute, tasks[i].command);
            }
            exit(EXIT_SUCCESS);
            break;
        }
        default:
        {
            syslog(LOG_ERR, "Received unknown signal");
            exit(EXIT_SUCCESS);
            break;
        }
    }
        
}
