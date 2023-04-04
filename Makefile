OBJ = main.o 
all: main
main: $(OBJ)
	gcc $(OBJ) -o main
.PHONY: clean
clean:
	rm -f *.o
