#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include "task.h"
#include "parser.h"
#include "execute.h"

Task tasks[MAX_TASKS];
int number_of_tasks = 1;

int main()
{
    char* filename = "tasks.txt";
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
