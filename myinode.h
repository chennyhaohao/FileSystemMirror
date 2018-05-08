#include <sys/stat.h>
#include <unistd.h>
#define FNAME_LEN 300

typedef struct treeNode treeNode;
typedef struct listNode listNode;
typedef struct myinode myinode;

struct myinode { //Inode ds
	time_t mtime; //last modification
	off_t size; //file size
	char ** fnames; //symbolic names
	myinode * replica;
	int references; 
};

struct listNode { //List of treeNodes ds
	treeNode * node; 
	listNode * next; 
};

struct treeNode { //FS treeNodes
	char * name;
	int isDir;
	ino_t src_inode;
	myinode * inode; //ptr to inode
	listNode * children_head; //list of children
	treeNode * parent;
	treeNode * mirror;
};

char * fpath(char * dir, char * fname);
treeNode * makeTreeNode(char * name, int isDir, ino_t src_inode, myinode * inode);
myinode * makeInode(time_t mtime, off_t size);
void addChild(treeNode * target, treeNode * newInode);
char * nodePath(treeNode * node);
void printTree(treeNode * root);
treeNode * searchListByName(listNode * head, char * name);
treeNode * searchTreeByInode(treeNode * root, ino_t inode_num);
listNode * removeNodeFromList(listNode * head, treeNode * target);
void deleteNode(treeNode * target);

