#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include "task.h"
#include "execute.h"

/* TO DO
TEST SIGNALS,
TEST SYSLOG,
REPLACE EXECL() */

void initilize_daemon()
{
    // Fork off the parent process 
    pid_t pid = fork();
    if(pid < 0)
    {
        syslog(LOG_ERR, "Failed to fork process to initialize daemon");
        exit(EXIT_FAILURE);
    }

    // If we got a good PID, then we can exit the parent process
    else if(pid > 0)
        exit(EXIT_SUCCESS);

    // Change the file mode mask
    umask(0);

    // Create a new SID for the child process
    pid_t sid = setsid();
    if (sid < 0) 
    {
        syslog(LOG_ERR, "Failed to create new SID for child process");
        exit(EXIT_FAILURE);
    }
   
    // Change the current working directory
    if ((chdir("/")) < 0)  
        exit(EXIT_FAILURE);
 
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
}
void execute_tasks(Task* tasks, int number_of_tasks)
{
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

        pid_t pid = fork();
        if(pid < 0)
        {
            syslog(LOG_ERR, "Failed to fork process to execute tasks");
            exit(EXIT_FAILURE);
        }
        else if(pid == 0) 
        {
            umask(0);
            pid_t sid = setsid();
            execl("/bin/sh", "/bin/sh", "-c", tasks[i].command, NULL);
            syslog(LOG_ERR, "Task has been executed successfully");
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
        }        
    }
}

void sig_handler(int signal)
{
    if(signal == SIGINT)
    {
        syslog(LOG_INFO, "Received SIGINT signal. Daemon has been terminated");
        exit(0);
    }
        
}
