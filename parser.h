#pragma once
#include "task.h"
void parse_tasks(char* filename, Task* tasks, int* number_of_tasks);
void sort_tasks(Task* tasks, int number_of_tasks);
