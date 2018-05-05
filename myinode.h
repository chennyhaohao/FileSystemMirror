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
};



void append(treeNode * target, treeNode * newInode);

