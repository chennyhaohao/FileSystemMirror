#include "./myinode.h"
#include <stdlib.h>

void append(treeNode * target, treeNode * newInode) {
	listNode * newNode = (listNode *) malloc(sizeof(listNode));
	newNode->node = newInode;
	newNode->next = target->children_head;
	target->children_head = newNode;
}