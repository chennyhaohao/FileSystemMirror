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
		if (strncmp(dp->d_name, ".", 2) == 0 || strncmp(dp->d_name, "..", 2) == 0) { 
			continue; //Skip parent & self to prevent cycle
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

void fcopy(FILE * src, FILE * target) {
	int nread;
	char * buf[1025];
	while((nread = fread(buf, sizeof(char), 1024, src)) > 0) {
		fwrite(buf, sizeof(char), nread, target);
	}
}

void r_copy(char * name, char * target, treeNode * root) { //Recursively copy directory
	DIR * dirp = opendir(name);
	DIR * tdirp = opendir(target);
	if (dirp == NULL || tdirp == NULL) {
		perror("opendir");
		exit(1);
	}
	struct dirent * dp;
	char * path,  *tpath;
	struct stat stat_buf;
	if (stat(name, &stat_buf) < 0) {
		perror("stat");
		exit(1);
	}

	while ((dp = readdir(dirp)) != NULL) {
		//printf("%s\n", dp->d_name);
		if (strncmp(dp->d_name, ".", 1) == 0 || strncmp(dp->d_name, "..", 2) == 0) { 
			continue; //Skip parent & self to prevent cycle
		}

		path = fpath(name, dp->d_name);
		tpath = fpath(target, dp->d_name);
		if (stat(path, &stat_buf) < 0) {
			perror("stat");
			exit(1);
		}

		myinode * inode = makeInode(stat_buf.st_mtime, stat_buf.st_size); //Create child inode
		treeNode * node;
		if ((stat_buf.st_mode & S_IFMT) == S_IFDIR ) { //is directory
			node = makeTreeNode(dp->d_name, 1, stat_buf.st_ino, inode); //Create child treeNode
			printf("%s/\n", path);
			if (mkdir(tpath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
				perror("mkdir");
				exit(1);
			}
			r_copy(path, tpath, node);
		} else {
			node = makeTreeNode(dp->d_name, 0, stat_buf.st_ino, inode);
			printf("%s\n", path);
			FILE * src, * target;

			if ( (src = fopen(path, "r")) == NULL) {
				perror("fopen");
				exit(1);
			}
			if ( (target = fopen(tpath, "w")) == NULL) {
				perror("fopen");
				exit(1);
			}
			fcopy(src, target);
			fclose(src);
			fclose(target);
		}
		addChild(root, node);
		free(path);
	}
	closedir(dirp);
}



int main() {
	treeNode * root = makeTreeNode(".", 1, 0, NULL);
	r_copy("./src", "./mirror", root);
	printTree(root);
	return 0;
}
