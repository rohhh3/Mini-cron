#pragma once
#include "task.h"
int parse_tasks(char* filename, Task* tasks, unsigned int* number_of_tasks);
void sort_tasks(Task* tasks, int number_of_tasks);