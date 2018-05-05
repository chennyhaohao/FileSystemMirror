#include "./myinode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char * fpath(char * dir, char * fname) {

	int len = strlen(dir) + strlen(fname);
	char * dest = (char *) malloc(sizeof(char) * (len + 3) );
	if (strlen(dir) == 0) {
		strcpy(dest, fname);
		return dest;
	}
	strcpy(dest, dir);
	strcat(dest, "/");
	strcat(dest, fname);
	return dest;
}

void addChild(treeNode * target, treeNode * newInode) {
	listNode * newNode = (listNode *) malloc(sizeof(listNode));
	newNode->node = newInode;
	newNode->next = target->children_head;
	target->children_head = newNode;
	newInode->parent = target;
}

treeNode * makeTreeNode(char * fname, int isDir, ino_t src_inode, myinode * inode) {
	treeNode * node = (treeNode *) malloc(sizeof(treeNode));
	char * name = (char *) malloc(sizeof(char) * (strlen(fname)+1));
	strcpy(name, fname);
	node->name = name;
	node->isDir = isDir;
	node->src_inode = src_inode;
	node->inode = inode;
	node->parent= NULL;
	node->children_head = NULL;
	return node;
}

myinode * makeInode(time_t mtime, off_t size) {
	myinode * inode = (myinode *) malloc(sizeof(myinode));
	inode->mtime = mtime;
	inode->size = size;
	return inode;
}

char * nodePath(treeNode * node) {
	char * result, * parentPath;
	if (node == NULL) {
		result = malloc(sizeof(char));
		result[0] = '\0';
		return result;
	}

	parentPath = nodePath(node->parent);
	result = fpath(parentPath, node->name);
	free(parentPath);
	return result;
}


void printTree(treeNode * root) {
	char * name = nodePath(root);
	if (root->isDir) {
		printf("%s/\n", name);
	} else {
		printf("%s\n", name);
	}
	free(name);
	listNode * lnode = root->children_head;
	while (lnode != NULL) {
		printTree(lnode->node);
		lnode = lnode->next;
	}
}
