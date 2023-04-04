#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_TASKS 100
#define MAX_COMMAND_LENGTH 100
#define MAX_MODE_LENGTH 2
#define MAX_LINE_LENGTH MAX_COMMAND_LENGTH + 2 + MAX_MODE_LENGTH + 1

typedef struct {
    int hour;
    int minute;
    char command[MAX_COMMAND_LENGTH + 1];
    int mode;
} Task;

void data_interpreter(char* filename)
{
	// Try to open a taskfile
	FILE* file = fopen(filename, "r");
    if (file == NULL) 
	{
        fprintf(stderr, "Error: Failed to open file '%s'\n", filename);
        exit(EXIT_FAILURE);
    }

	// Count tasks
	int number_of_tasks = 1;
	char c;
	 while((c = getc(file)) != EOF) 
        if(c == '\n') 
            number_of_tasks++;
    printf("%d \n", number_of_tasks);

	if(number_of_tasks > MAX_TASKS)
	{
		fprintf(stderr, "Too many tasks, max: 100; current amount: %d", number_of_tasks);
		exit(EXIT_FAILURE);
	}

	// Parse taks from taskfile
	Task tasks[number_of_tasks];
	char line[MAX_LINE_LENGTH];
	
	while(fgets(line, sizeof(line), file) != NULL)
	{
		int hour, minute, mode;
		char command[MAX_COMMAND_LENGTH + 1];
		// continue here
	}

	fclose(file);

}

int main(int argc, char *argv[])
{
    char* filename = argv[1];
	data_interpreter(filename);
}
