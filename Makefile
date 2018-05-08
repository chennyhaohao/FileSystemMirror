all: myinode main.o inotify-utils main

myinode: myinode.c myinode.h
	gcc -c myinode.c -o myinode.o

main.o: main.c myinode.h
	gcc -c main.c -o main.o

main: main.o myinode inotify-utils
	gcc main.o myinode.o inotify-utils.o -o main

inotify-utils: inotify-utils.c inotify-utils.h
	gcc -c inotify-utils.c -o inotify-utils.o