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
	if (inode) {
		inode->references ++; //Increment references count
	}
	return node;
}

myinode * makeInode(time_t mtime, off_t size) {
	myinode * inode = (myinode *) malloc(sizeof(myinode));
	inode->mtime = mtime;
	inode->size = size;
	inode->references = 0;
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
		printf("%s %ld/\n", name, root->src_inode);
	} else {
		printf("%s %ld\n", name, root->src_inode);
	}
	free(name);
	listNode * lnode = root->children_head;
	while (lnode != NULL) {
		printTree(lnode->node);
		lnode = lnode->next;
	}
}

treeNode * searchListByName(listNode * head, char * name) {
	if (head == NULL) return NULL;
	//printf("head name: %s; searching for: %s\n", head->node->name, name);
	if (strcmp(head->node->name, name) == 0) return head->node;
	return searchListByName(head->next, name);
}

treeNode * searchTreeByInode(treeNode * root, ino_t inode_num) {
	if (root == NULL) return NULL;
	if (root->src_inode == inode_num) {
		return root;
	}
	listNode * lnode = root->children_head;
	treeNode * result;
	while (lnode != NULL) {
		result = searchTreeByInode(lnode->node, inode_num);
		if (result != NULL) return result;
		lnode = lnode->next; 
	}
	return NULL;
}

listNode * removeNodeFromList(listNode * head, treeNode * target) { //Remove node and return new head
	if (head == NULL) return NULL;
	if (head->node == target) { //Found target
		//free(target);
		listNode * result = head->next;
		free(head);
		return result;
	}
	head->next = removeNodeFromList(head->next, target); //Recursively delete
	return head;
}

void deleteNode(treeNode * target) {
	if (!target) return;
	target->inode->references--; //Decrement inode references count
	if (target->inode->references == 0) {
		free(target->inode);
		printf("No references left, deleting inode\n");
	}
	free(target->name);
	free(target);
}

