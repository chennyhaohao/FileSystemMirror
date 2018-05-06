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
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) { 
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

void r_remove(char * name) { //Recursively remove directory/file
	struct stat stat_buf;
	if (stat(name, &stat_buf) < 0) {
		perror("stat");
		exit(1);
	}

	if ((stat_buf.st_mode & S_IFMT) != S_IFDIR ) { //is file
		printf("Removing: %s\n", name);
		if (remove(name) == -1) {
			perror("remove");
			exit(1);
		}
		return;
	}

	//is directory

	DIR * dirp = opendir(name);
	if (dirp == NULL) {
		perror("opendir");
		exit(1);
	}
	struct dirent * dp;
	char * path;
	while ((dp = readdir(dirp)) != NULL) {
		//printf("%s\n", dp->d_name);
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) { 
			continue; //Skip parent & self to prevent cycle
		}
		path = fpath(name, dp->d_name);
		r_remove(path);
		free(path);
	}
	closedir(dirp);
	printf("Removing: %s\n", name);
	remove(name);
}


void fcopy(FILE * src, FILE * target) {
	int nread;
	char * buf[1025];
	while((nread = fread(buf, sizeof(char), 1024, src)) > 0) {
		fwrite(buf, sizeof(char), nread, target);
	}
}

void fcopyByPath(char * src, char * target) {
	FILE * src_fp, * target_fp;
	if ( (src_fp = fopen(src, "r")) == NULL) {
		perror("fopen");
		exit(1);
	}
	
	if ( (target_fp = fopen(target, "w")) == NULL) {
		perror("fopen");
		exit(1);
	}

	fcopy(src_fp, target_fp);
	fclose(src_fp);
	fclose(target_fp);
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
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) { 
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

void createTree(char * name, treeNode * root, treeNode * globalRoot) { //Recursively copy directory
	DIR * dirp = opendir(name);
	if (dirp == NULL) {
		perror("opendir");
		exit(1);
	}
	struct dirent * dp;
	char * path;
	struct stat stat_buf;
	if (stat(name, &stat_buf) < 0) {
		perror("stat");
		exit(1);
	}

	while ((dp = readdir(dirp)) != NULL) {
		//printf("%s\n", dp->d_name);
		if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) { 
			continue; //Skip parent & self to prevent cycle
		}

		path = fpath(name, dp->d_name);
		if (stat(path, &stat_buf) < 0) {
			perror("stat");
			exit(1);
		}

		myinode * inode;  //Create child inode
		treeNode * node;
		if ((stat_buf.st_mode & S_IFMT) == S_IFDIR ) { //is directory
			inode = makeInode(stat_buf.st_mtime, stat_buf.st_size);
			node = makeTreeNode(dp->d_name, 1, stat_buf.st_ino, inode); //Create child treeNode
			addChild(root, node); //Need to add child to the structure first for search
			createTree(path, node, globalRoot);
		} else { //is file, first search inode in whole tree
			treeNode * result = searchTreeByInode(globalRoot, stat_buf.st_ino); 
			if (result) { //Inode already exists
				printf("Hard link detected: %s\n", nodePath(result));
				inode = result->inode; //Give the same inode in case of hard link
				//so that updates to the same file is automatically reflected in all nodes
			} else {
				inode = makeInode(stat_buf.st_mtime, stat_buf.st_size);
			}
			node = makeTreeNode(dp->d_name, 0, stat_buf.st_ino, inode);
			addChild(root, node);
		}
		
		free(path);
	}
	closedir(dirp);
}

void sync_dir(treeNode * src, treeNode * target, treeNode * targetRoot) { 
	if (!src || !target || !src->isDir || !target->isDir ) { //Not dir
		return;
	}
	//struct stat stat_buf;	
	listNode * lnode = src->children_head;
	char * path, * tpath;
	while (lnode != NULL) { //Loop through children of src node
		//printf("%s\n", dp->d_name);
		treeNode * src_child = lnode->node, * target_child;
		path = nodePath(src_child);
		/*if (stat(path, &stat_buf) < 0) {
			perror("stat");
			exit(1);
		}*/
		//free(path);

		target_child = searchListByName(target->children_head, src_child->name);

		if (target_child && (src_child->isDir != target_child->isDir)) { //Different types
			tpath = nodePath(target_child);
			printf("Different type: %s %s\n", path, tpath);
			if (target_child->isDir) { //Remove dir
				printf("Removing directory: %s/\n", tpath);
				//rmdir(tpath);
				r_remove(tpath);
			} else { //Remove file
				printf("Removing file: %s\n", tpath);
				remove(tpath);
			}
			
			target->children_head = deleteNodeFromList(target->children_head, target_child);
			target_child = NULL;
			free(tpath);
		}

		if (target_child == NULL) { //Not found in target dir; create new node in target
			myinode * inode; 
			treeNode * result = NULL;
			if (!src_child->isDir) { //is file
				result = searchTreeByInode(targetRoot, src_child->src_inode);
			}
			if (result) { //if inode already exists
				inode = result->inode; //hard link, give the same inode
			} else {
				inode = makeInode(src_child->inode->mtime, src_child->inode->size); 
				//Create new child inode
			}

			target_child = makeTreeNode(src_child->name, src_child->isDir, 
				src_child->src_inode, inode); //Create child treeNode			
			addChild(target, target_child);
			tpath = nodePath(target_child);
			printf("Creating new node: %s\n", tpath);

			if (!src_child->isDir) { //is file
				if (result != NULL) { //inode already exists, must be hard link
					char * lpath = nodePath(result);
					printf("Creating hard link from %s to %s\n", tpath, lpath);
					if (link(lpath, tpath) == -1) { //Create link
						perror("link");
						exit(1);
					}
					free(lpath);
				} else { //Create file
					fcopyByPath(path, tpath); //Copy file
				}
			} else { //is dir
				if (mkdir(tpath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) { //create dir
					perror("mkdir");
					exit(1);
				}
			}
			free(tpath);
		}

		
		tpath = nodePath(target_child);
		if(!src_child->isDir && !target_child->isDir) { //Both are files
			if (target_child->inode->mtime < src_child->inode->mtime || 
				target_child->inode->size != src_child->inode->size) {
				//Update file
				/*if (target_child->inode->mtime < src_child->inode->mtime) {
					printf("Last modified smaller\n");
				}*/

				printf("Updating file: %s\n", tpath);
				fcopyByPath(path, tpath);
				target_child->inode->mtime = time(NULL); //Update inode info
				target_child->inode->size = src_child->inode->size;
			}
		}

		free(path);
		free(tpath);

		target_child->src_inode = src_child->src_inode; //link src inode with target node
		target_child->mirror = src_child;
		src_child->mirror = target_child; //establish link between src and target node
		sync_dir(src_child, target_child, targetRoot); //Sync after finding/creating the target node

		lnode = lnode->next;
	} 
	lnode = target->children_head;
	while (lnode != NULL) {
		int rm = 0;
		treeNode * target_child = lnode->node;
		if (searchListByName(src->children_head, target_child->name) == NULL) { //Entry doesn't exist
			rm = 1;
			tpath = nodePath(target_child);
			printf("Removing file/directory: %s\n", tpath);
			r_remove(tpath);
			free(tpath);
		}
		lnode = lnode->next;
		if (rm) {
			target->children_head = deleteNodeFromList(target->children_head, target_child);
		}
	}
}


int main() {
	char * srcDir = "./src";
	char * targetDir = "./mirror";
	treeNode * src = makeTreeNode(srcDir, 1, 0, NULL);
	treeNode * target = makeTreeNode(targetDir, 1, 0, NULL);
	//r_copy("./src", "./mirror", root);
	createTree(srcDir, src, src);
	createTree(targetDir, target, target);	
	//printTree(src);
	printf("\n-----\n");
	printTree(target);
	printf("\n-----\n");
	sync_dir(src, target, target);

	printTree(target);

	return 0;
}
