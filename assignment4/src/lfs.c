#include <fuse.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/types.h>
#include <unistd.h>

#define SEGMENTS 4			/* segments containing the blocks, and array_inode */
#define BLOCK 4					/* Blocks containing the data */
#define BLOCK_SIZE	256
#define SEGMENT_SIZE 1024


typedef struct inode{
	ino_t inode_number;				/* id of the node */
	char *inode_path;					/* path to where the file or directory is placed on the computer*/
	int inode_type;						/* 0 directory, 1 file */
	size_t inode_sub_num;			/* size of sub inodes in current inode */
	time_t inode_access;			/* time of last access */
	time_t inode_modified;		/* time of last modification */
	void **inode_direct;			/* pointer to all sub files or directories */
	//int **inode_indirect;		/* pointer to pointer to datablocks */
	size_t inode_size;				/* size of data in files */
	void *inode_data;					/* data */

} inode;

typedef struct block {
	struct inode *node;
	int in_use;
} block;



inode *find_inode(const char* path);
inode *find_inode_dir(const char* path);
inode *find_inode_file(const char* path);
inode *find_inode_parent(const char* path);
int lfs_getattr(const char *, struct stat *);
int lfs_readdir(const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *);
int lfs_mk(const char *path, int type);
int lfs_mknod(const char* path, mode_t mode, dev_t rdev);
int lfs_mkdir(const char *path, mode_t mode);
int lfs_rmdir(const char *path);
int lfs_open(const char *, struct fuse_file_info *);
int lfs_read(const char *, char *, size_t, off_t, struct fuse_file_info *);
int lfs_release(const char *path, struct fuse_file_info *fi);
int lfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi ) ;
int lfs_utime (const char *path, struct utimbuf *utb);




block **array_inode;								/* pointer to the inodes */
int current_inode_amount = 0;				/* current amount of inodes, increased when a new node is made */
void *disk;													/* disk containing the segments */
int segment;
//int number_of_inodes = 0;

int lfs_init(void){
	printf("---init---\n");
	inode *inod;
	inod = malloc(sizeof(inode));		/* allocate space for the inode */
	if(inod == NULL){
		return -ENOMEM;
	}
	inod -> inode_path = calloc(200,sizeof(char));	/* allocate space for the inode path */
	if(inod -> inode_path == NULL){
		return -ENOMEM;
	}
	char *root_path = "/";
	/* initializing the rootnode */
	inod -> inode_number = 0; 					/* Root node */
	strcpy(inod -> inode_path, root_path); /* initialize the root path to "/" */
	inod -> inode_type = 0;
	inod -> inode_sub_num = 0;
	inod -> inode_size = 0;
	inod -> inode_access = time(NULL);
	inod -> inode_modified = time(NULL);
	inod -> inode_direct = malloc(15*sizeof(inode));
	//printf("hello1\n");
	array_inode = malloc(1024 * sizeof(inode));
	if(array_inode == NULL){
		free(inod);
		return -ENOMEM;
	}

	disk = malloc(SEGMENTS * SEGMENT_SIZE);
	if(disk == NULL){
		free(inod);
		free(array_inode);
		return -ENOMEM;
	}
/*
	segment = malloc(BLOCK * BLOCK_SIZE);
	if(segment == NULL){
		free(inod);
		free(array_inode);
		free(disk);
		return -ENOMEM;
	}
*/
	//printf("hello2\n");
	block *bl;
	bl = malloc(sizeof(block));

	if(bl == NULL){
		free(inod);
		free(array_inode);
		free(disk);
		//free(segment);
		return -ENOMEM;
	}

	bl -> node = inod;
	bl -> in_use = 1;

	array_inode[0] = bl; /* places the rootnode in the 0'th place */
	current_inode_amount++;
	memcpy((disk + (current_inode_amount-1 * BLOCK_SIZE)), inod, sizeof(inode)); /* copies from inod to the next available space on the disc  */

	//printf("GOODBYE from init\n");
	return 0;
}

/* find inodes */
inode *find_inode(const char* path){
	printf("---find_inode---\n");
	for(int i = 0 ; i < current_inode_amount ; i++) {
		printf("      find_inode: %i out of %i iterations and path: %s\n", i, current_inode_amount, path);

		if (strcmp(array_inode[i] -> node -> inode_path, path) == 0){ /* if it finds the path */
			printf("      the path of the inode is found: %s\n", array_inode[i] -> node -> inode_path);
			inode *inod;
			//inod = malloc(sizeof(inode));
			inod = array_inode[i] -> node;
			return inod; 																/* returns the inode*/
		}
	}
	printf("no inode found \n");
	return NULL;
}

/* find directory inodes */
inode *find_inode_dir(const char* path){
	printf("---find_inode_dir---\n");
	for(int i = 0 ; i < current_inode_amount ; i++) {
		printf("      find_inode_dir: %i out of %i iterations and path %s\n", i, current_inode_amount, path);

		/* if it finds the path and it is a directory */
		if ((strcmp(array_inode[i] -> node -> inode_path, path) == 0) && (array_inode[i] -> node -> inode_type == 0)){
			printf("      the path of the directory is found: %s\n", array_inode[i] -> node -> inode_path);
			inode *inod;
			inod = malloc(sizeof(inode));
			inod = array_inode[i] -> node;
			return inod; 																/* returns the inode*/
		}
	}
	printf("no find_inode_dir found \n");
	return NULL;
}

/* find file inodes */
inode *find_inode_file(const char *path){
	//inode *inod;
	printf("---find_inode_file---\n");
	for(int i = 0 ; i < current_inode_amount ; i++) {
		printf("      find_inode_file: %i out of %i iterations and path %s\n", i, current_inode_amount, path);

		/* if it finds the path and it is a file */
		if ((strcmp(array_inode[i] -> node -> inode_path, path) == 0) && (array_inode[i] -> node -> inode_type == 1)){
			printf("      the path of the file is found: %s\n", array_inode[i] -> node -> inode_path);
			return (array_inode[i] -> node); 																/* returns the inode*/
		}
	}
	printf("no find_inode_file found \n");
	return NULL;
}
/* find parent inode */
inode *find_inode_parent(const char *path){
	printf("---find_parent---\n");
	char *path_copy;
	path_copy = strdup(path);
	return find_inode(dirname(path_copy));		/* copies the path up to, but not including, the final '/' */
}




/*  */
int lfs_getattr( const char *path, struct stat *stbuf ) {
	printf("---getattr---(path:  %s)\n", path);
	int res = 0;
	//printf("getattr: (path=%s)\n", path);

	memset(stbuf, 0, sizeof(struct stat)); /* initialize stbuf with 0 */
	inode *inod;
	//if(inod == NULL){
	//	return -ENOENT;											/* no such file og directory */
	//}
	//printf("getattr3\n");
	inod = find_inode(path);			/* finds the inode */
	if(inod == NULL){
		return -ENOENT;
	}
	stbuf -> st_ino = current_inode_amount;
	stbuf -> st_size = inod->inode_size;
	stbuf -> st_atime = time(NULL);
	stbuf -> st_mtime = inod->inode_modified;
	stbuf -> st_ctime = inod->inode_access;
	if( inod->inode_type == 0){ 			/* directory */
		stbuf -> st_mode = S_IFDIR | 0755;
		stbuf -> st_nlink = 2;
	}else if(inod->inode_type == 1){ 		/* file */
		stbuf -> st_mode = S_IFREG | 0777;
		stbuf -> st_nlink = 1;
	}else 															/* file */
		res = -ENOENT;										/* no such file og directory */
	//printf("getattr end\n");
	printf("      last access time: %li \n", inod->inode_access);
	printf("      last modified time: %li\n\n\n", inod->inode_modified);
	return res;
}


/* display all content of a directory */
int lfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ) {
	printf("---readdir---\n");
	(void) offset;
	(void) fi;

	inode *inod = malloc(sizeof(inode));
	if(inod == NULL){
		return -ENOENT;									/* no such file og directory */
	}

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
 	inod = find_inode_dir(path);			/* Finds the path given to lfs_readdir, in the current directory  */
	/*
	if(strcmp(path, "/") != 0){
		return -ENOENT;
	}*/

	for(int i = 0; i < inod -> inode_sub_num; i++){
		filler(buf, basename(((inode*)inod -> inode_direct[i]) -> inode_path), NULL, 0); /* Filler displays the content of the buffer with all inodes in the current directory */
	}
	printf("---readdir end---\n");
	//free(inod);
	return 0;
}


int lfs_mk(const char *path, int type) {
	printf("---mk---\n");
	inode *parent;
	parent = malloc(sizeof(inode));
	if(parent == NULL){
		return -ENOMEM;
	}
	printf("mk2\n");
	parent = find_inode_parent(path);

	inode *inod;										/* file or directory */
	inod = malloc(sizeof(inode));		/* allocate space for the inode */
	if(inod == NULL){
		return -ENOMEM;
	}
	printf("mk3\n");
	inod -> inode_path = malloc(sizeof(char));	/* allocate space for the inode path */
	if(inod ->inode_path == NULL){
		return -ENOMEM;
	}
	printf("mk4\n");
	inod -> inode_number = current_inode_amount; 					/* current node */
	strcpy(inod->inode_path, path); 										/* initialize the path */
	inod -> inode_type = type;
	inod -> inode_sub_num = 0;
	inod -> inode_size = 0;
	inod -> inode_access = time(NULL);
	inod -> inode_modified = time(NULL);
	inod -> inode_direct = malloc(15*sizeof(inode));
	inod -> inode_data = malloc(20*sizeof(void*));
	block *bl;
	bl = malloc(sizeof(block));
	if(bl == NULL){
		return -ENOMEM;
	}
	bl -> node = inod;
	bl -> in_use = 1;
	array_inode[current_inode_amount] = bl; /* places the current block with the inode in the array  */
	current_inode_amount++;
	memcpy((disk + (current_inode_amount * BLOCK_SIZE)), inod, sizeof(inode)); /* copies from inode to the next available space on the disc  */
	parent -> inode_direct[parent -> inode_sub_num] = inod;	/* points to the new inode */
	parent -> inode_sub_num = parent -> inode_sub_num+1;				/* increment size of the inode */
	return 0;
}
/* uses lfs_mk to create a file */
int lfs_mknod(const char* path, mode_t mode, dev_t rdev) {
	printf("---mknod---\n");
	int ret = lfs_mk(path, 1);
	return ret;
}

/* uses lfs_mk to create a directory */
int lfs_mkdir(const char *path, mode_t mode) {
	printf("---mkdir---\n");
	int ret = lfs_mk(path, 0);
	return ret;
}

int lfs_rmdir(const char *path) {
	printf("---rmdir---\n");
	inode *parent;
	parent = malloc(sizeof(inode));
	if(parent == NULL){
		return -ENOMEM;
	}
	parent = find_inode_parent(path);

	inode *inod;										/* directory to be deleted */
	inod = malloc(sizeof(inode));		/* allocate space for the inode */
	if(inod == NULL){
		return -ENOMEM;
	}
	inod = find_inode_dir(path);		/* the path to the directory to be deleted */

	if(inod -> inode_sub_num == 0){					/* is there sub directories */
		for(int i = 0; i < parent -> inode_sub_num; i++){
			//inod = parent -> inode_direct[i];	/* finds the pointer to the parent directory */
			if(strcmp(((inode*)parent -> inode_direct[i]) -> inode_path, inod -> inode_path) == 0){ /* finds the right path in the parent  */
				parent -> inode_direct[i] = NULL;
				for(int j = i; j < parent -> inode_sub_num-1; j++){
					parent -> inode_direct[j] = parent -> inode_direct[j+1];	/* when found, shift everything one place to the left */
				}
				parent -> inode_direct[parent -> inode_sub_num-1] = NULL;
				parent -> inode_sub_num--;
				break;
			}
		}

		for(int i = 0; i < current_inode_amount; i++){
			if(strcmp(array_inode[i] -> node -> inode_path, inod -> inode_path) == 0){ /* finds the right path in the in the inode array  */
				array_inode[i] = NULL;
				for(int j = i; j < current_inode_amount; j++){
					array_inode[j] = array_inode[j+1];	/* when found, shift everything one place to the left */
				}
				array_inode[current_inode_amount] = NULL;
				current_inode_amount--;
				break;
			}
		}
		return 0;
	}

	return -ENOTEMPTY;
}

//Permission
int lfs_open(const char *path, struct fuse_file_info *fi) {
    printf("---Open--- path: %s\n", path);
	return 0;
}

int lfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  printf("---Read--- path: %s\n", path);
	inode *inod;
	inod = find_inode_file(path);
	memcpy(buf, inod -> inode_data, inod -> inode_size);
	printf("---read end---\n" );
	return inod -> inode_size;
}

int lfs_release(const char *path, struct fuse_file_info *fi) {
	printf("---Release---  path: %s\n", path);
	return 0;
}

int lfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi ) {
  printf("---Write--- path: %s\n", path);
	inode *inod;
	inod = find_inode_file(path);
	memcpy(inod -> inode_data, buf, strlen(buf));
	inod -> inode_size = strlen(buf);
	memcpy((disk + ((inod ->inode_number) * BLOCK_SIZE)), inod, sizeof(inode));
	array_inode[inod ->inode_number] = NULL;
	block *bl = malloc(sizeof(block));
	bl -> node = inod;
	bl -> in_use = 1;
	array_inode[inod ->inode_number] = bl;

	printf("---write end---\n" );
	return size;
}

int lfs_utime (const char *path, struct utimbuf *utb) {
	printf("---Utime---, path: %s\n", path);

	inode *inod;
	inod = find_inode(path);
	inod -> inode_access = utb -> actime;
	inod -> inode_modified = utb -> modtime;

	return 0;
}

static struct fuse_operations lfs_oper = {
	.getattr	= lfs_getattr,
	.readdir	= lfs_readdir,
	.mknod = lfs_mknod,
	.mkdir = lfs_mkdir,
	.unlink = NULL, //not needed
	.rmdir = lfs_rmdir,
	.truncate = NULL,
	.open	= lfs_open,
	.read	= lfs_read,
	.release = lfs_release,
	.write = lfs_write,
	.rename = NULL, //not needed
	.utime = lfs_utime
};

int main( int argc, char *argv[] ) {
	lfs_init();
	fuse_main( argc, argv, &lfs_oper );
	return 0;
}
