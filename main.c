#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include "myinode.h"
#include <dirent.h>

char * fpath(char * dir, char * fname) {
	int len = strlen(dir) + strlen(fname);
	char * dest = (char *) malloc(sizeof(char) * len + 3);
	strcpy(dest, dir);
	strcat(dest, "/");
	strcat(dest, fname);
	return dest;
}

void traverse(char * name) {
	DIR * dirp = opendir(name);
	if (dirp == NULL) {
		perror("opendir");
		exit(1);
	}
	struct dirent * dp;
	char * path;
	while ((dp = readdir(dirp)) != NULL) {
		//printf("%s\n", dp->d_name);
		if (strncmp(dp->d_name, ".", 1) == 0 || strncmp(dp->d_name, "..", 2) == 0) { 
			continue; //Skip parent & self
		}
		path = fpath(name, dp->d_name);
		struct stat stat_buf;
		if (stat(path, &stat_buf) < 0) {
			perror("stat");
			exit(1);
		}
		if ((stat_buf.st_mode & S_IFMT) == S_IFDIR ) { //is directory
			printf("%s/\n", path);
			traverse(path);
		} else {
			printf("%s\n", path);
		}
		free(path);
	}
	closedir(dirp);
}

int main() {
	traverse(".");

	return 0;
}
