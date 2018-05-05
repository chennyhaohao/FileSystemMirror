all: myinode main.o main

myinode: myinode.c myinode.h
	gcc -c myinode.c -o myinode.o

main.o: main.c myinode.h
	gcc -c main.c -o main.o

main: main.o myinode
	gcc main.o myinode.o -o main