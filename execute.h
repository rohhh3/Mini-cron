#pragma once
#include "task.h"
#include "parser.h"
void initilize_daemon();
void execute_tasks(Task* tasks, int number_of_tasks);
void sig_handler(int signal);
