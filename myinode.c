#include "./myinode.h"
#include <stdlib.h>
#include <string.h>

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
	return node;
}

myinode * makeInode(time_t mtime, off_t size) {
	myinode * inode = (myinode *) malloc(sizeof(myinode));
	inode->mtime = mtime;
	inode->size = size;
	return inode;
}

