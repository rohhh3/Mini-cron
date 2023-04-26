#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include "task.h"
#include "execute.h"
void execute_tasks(Task* tasks, int number_of_tasks)
{
    pid_t pid;
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

        pid = fork();
        if (pid == -1)
            syslog(LOG_ERR, "Failed to fork process to execute task: %s", tasks[i].command);

        // Child process
        else if(pid == 0)
        {
            execl("/bin/sh", "/bin/sh", "-c", tasks[i].command, NULL);
            syslog(LOG_ERR, "Task has been executed successfully");
            exit(EXIT_SUCCESS);
        }

        // Parent process
        else
        {
            int status;
            waitpid(pid, &status, 0);
            if(WIFEXITED(status))
            {
                if (WIFEXITED(status) != 0)
                    syslog(LOG_WARNING, "Task exited with non-zero status: %s", tasks[i].command);
            }

            else if (WIFSIGNALED(status))
                syslog(LOG_WARNING, "Task terminated due to signal: %s", tasks[i].command);
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
