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
#include "./inotify-utils.h"


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

	target->src_inode = src->src_inode; //link src inode with target node
	target->mirror = src;
	src->mirror = target; //establish link between src and target node
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

		if (target_child && //target is found, and
			(src_child->isDir != target_child->isDir || //Different types, or
			(!src_child->isDir && !target_child->isDir && //Both are files, and
			nodeOutOfSync(src_child, target_child)) ) ) { 

			tpath = nodePath(target_child);
			if (src_child->isDir != target_child->isDir) 
				printf("Different type: %s %s\n", path, tpath);
			else
				printf("Removing for update: %s\n", tpath);

			removeNodeAndEntry(target_child);
			target_child = NULL; //As if node was not found
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
				//inode = makeInode(src_child->inode->mtime, src_child->inode->size); 
				inode = makeInode(time(NULL), src_child->inode->size); 
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
					if (nodeOutOfSync(src_child, target_child)) { //Linked file is still out of sync
						fcopyByPath(path, tpath); //Copy file
						target_child->inode->mtime = time(NULL);
						target_child->inode->size = src_child->inode->size;
					}
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

		free(path);
		
		sync_dir(src_child, target_child, targetRoot); //Sync after finding/creating the target node

		lnode = lnode->next;
	} 
	lnode = target->children_head;
	while (lnode != NULL) {
		treeNode * target_child = lnode->node;
		lnode = lnode->next;
		if (searchListByName(src->children_head, target_child->name) == NULL) { //Entry doesn't exist
			removeNodeAndEntry(target_child);
		}
	}
}

int watchTree(int fd, treeNode * root, treeNode **wd_map) {
	if (root == NULL) return;

	int wd, watched = 0;
	char * path = nodePath(root);
	listNode * lnode = root->children_head;
	wd = inotify_add_watch(fd, path, IN_ALL_EVENTS);
	if( wd == -1 ) {
		printf("failed to add watch %s\n", path);
	} else {
		printf("Watching %s as %i\n", path, wd);
		watched++;
		wd_map[wd] = root;
	}
	free(path);
	while (lnode != NULL) {
		if (lnode->node->isDir) { //if child is directory, add child to watich
			watched += watchTree(fd, lnode->node, wd_map);
		}
		lnode = lnode->next;
	}
	return watched;
}

treeNode * wd_to_node[4097]; //may have to change to linked list

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

	int length, read_ptr, read_offset; //management of variable length events
	int i, watched;
	int fd, wd;					// descriptors returned from inotify subsystem
	char buffer[EVENT_BUF_LEN];	//the buffer to use for reading the events
	treeNode * node;
	struct stat stat_buf;
	myinode * inode;

	fd = inotify_init();
	if (fd < 0)
		fail("inotify_init");

	watched = watchTree(fd, src, wd_to_node);
	printf("Watched %d directories\n", watched);

	read_offset = 0; //remaining number of bytes from previous read
	while (1) {
		/* read next series of events */
		length = read(fd, buffer + read_offset, sizeof(buffer) - read_offset);
		if (length < 0)
			fail("read");
		length += read_offset; // if there was an offset, add it to the number of bytes to process
		read_ptr = 0;
		
		// process each event
		// make sure at least the fixed part of the event in included in the buffer
		while (read_ptr + EVENT_SIZE <= length ) { 
			//point event to beginning of fixed part of next inotify_event structure
			struct inotify_event *event = (struct inotify_event *) &buffer[ read_ptr ];
			
			// if however the dynamic part exceeds the buffer, 
			// that means that we cannot fully read all event data and we need to 
			// deffer processing until next read completes
			if( read_ptr + EVENT_SIZE + event->len > length ) 
				break;
			//event is fully received, process
			printf("WD:%i %s %s %s COOKIE=%u\n", 
				event->wd, event_name(event), 
				target_type(event), target_name(event), event->cookie);
			node = wd_to_node[event->wd];
			char * path = nodePath(node), * tpath = NULL;
			if (target_name(event))
				tpath = fpath(path, target_name(event));
			
			if (strcmp(event_name(event), "create") == 0) { //New entry created
				printf("Entry created: %s\n", tpath);
				if (stat(tpath, &stat_buf) < 0) {
					perror("stat");
					exit(1);
				}

				if ((stat_buf.st_mode & S_IFMT) == S_IFDIR ) { //is directory
					inode = makeInode(stat_buf.st_mtime, stat_buf.st_size);
					treeNode *newNode = makeTreeNode(fpath("", 
						target_name(event)), 1, stat_buf.st_ino, inode); //Create child treeNode
					addChild(node, newNode);
					watchTree(fd, newNode, wd_to_node); 
				} else { //is file, first search inode in whole tree
					treeNode * result = searchTreeByInode(src, stat_buf.st_ino); 
					if (result) { //Inode already exists
						printf("Hard link detected: %s\n", nodePath(result));
						inode = result->inode; //Give the same inode in case of hard link
						//so that updates to the same file is automatically reflected in all nodes
					} else {
						inode = makeInode(stat_buf.st_mtime, stat_buf.st_size);
					}
					treeNode *newNode = makeTreeNode(fpath("", target_name(event)), 
						0, stat_buf.st_ino, inode);
					addChild(node, newNode);
				}

				sync_dir(node, node->mirror, target);

			} else if (strcmp(event_name(event), "modify") == 0) { //Entry modified
				printf("Entry modified: %s\n", tpath);
				treeNode * result = searchListByName(node->children_head, target_name(event));
				if (result && !result->isDir) { //File modified
					printf("Marking file as modified\n");
					result->modified = 1;
				}
			} else if (strcmp(event_name(event), "close write") == 0) {
				printf("Entry close write: %s\n", tpath);
				treeNode * result = searchListByName(node->children_head, target_name(event));
				if (result && !result->isDir && result->modified) { //File modified and closed
					printf("Updating file\n");
					if (stat(tpath, &stat_buf) < 0) {
						perror("stat");
						exit(1);
					}
					result->inode->mtime = stat_buf.st_mtime;
					result->inode->size = stat_buf.st_size; //Update inode info
					result->modified = 0;
					sync_dir(node, node->mirror, target);
				}
			} else if (strcmp(event_name(event), "delete") == 0) {
				printf("Entry deleted:%s\n", tpath);
				treeNode * result = searchListByName(node->children_head, target_name(event));
				if (result && !result->isDir) { //File removed
					printf("Removing file\n");
					node->children_head = removeNodeFromList(node->children_head, result);
					sync_dir(node, node->mirror, target);
				}
			} else if (strcmp(event_name(event), "watch target deleted") == 0) {
				printf("Directory deleted: %s\n", path);
				treeNode * parent = node->parent;
				if (inotify_rm_watch(fd, event->wd) == -1) //Remove from watch list
					fail("rm_watch");
				wd_to_node[event->wd] = NULL;
				if (parent) {
					parent->children_head = removeNodeFromList(parent->children_head,
						node); //Remove node
					sync_dir(parent, parent->mirror, target);
				}
			}

			free(path);
			if (tpath)
				free(tpath);

			//advance read_ptr to the beginning of the next event
			read_ptr += EVENT_SIZE + event->len;
		}
		//check to see if a partial event remains at the end
		if( read_ptr < length ) {
			//copy the remaining bytes from the end of the buffer to the beginning of it
			memcpy(buffer, buffer + read_ptr, length - read_ptr);
			//and signal the next read to begin immediatelly after them			
			read_offset = length - read_ptr;
		} else
			read_offset = 0;
		
	}

	close(fd);
	return 0;
}
