#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include "task.h"
#include "parser.h"
#include "execute.h"

Task tasks[MAX_TASKS];
unsigned int number_of_tasks = 1;

int main(int argc, char** argv)
{   
    char* filename = argv[1];
    parse_tasks(filename, tasks, &number_of_tasks);
    sort_tasks(tasks, number_of_tasks);

    printf("Number of tasks: %d\n", number_of_tasks);

    execute_tasks(tasks, number_of_tasks);

    return 0;
}
