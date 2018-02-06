
#include "lib.h"

#ifndef _FILE_SYSTEM_H
#define _FILE_SYSTEM_H

typedef struct dentry_t {
	unsigned char file_name[32];
	unsigned int file_type;
	unsigned int inode_index;
	unsigned char reserved[24];

} dentry_t;

typedef struct bootblock {
	unsigned int num_dentries;
	unsigned int num_inodes;
	unsigned int num_data_blocks;
	unsigned char reserved[52];
	dentry_t dentries[63];

} bootblock;

typedef struct inode {
	unsigned int length;
	unsigned int data_block_index[1023];
} inode;


extern bootblock *bl;
extern int file_read_dentry;

void init_file_system(unsigned int module_start);

int read_dentry_by_name(const unsigned char *fname, dentry_t *dentry);

int same_name(const unsigned char *fname, dentry_t *cur_dentry);

void deep_copy_dentry(dentry_t *dentry, dentry_t *cur_dentry);

unsigned int read_dentry_by_index(unsigned int index, dentry_t *dentry);

unsigned int read_data(unsigned int inode, unsigned int offset, unsigned char *buf, unsigned int length);

void copy_data(unsigned int start_addr, unsigned int offset, unsigned char *buf, unsigned int length, unsigned int buff_index);

int open_directory(const uint8_t* filename);

int read_directory(unsigned int inode, unsigned int offset, void* buf, int32_t nbytes);

int write_directory(const void* buf, int32_t size);

int close_directory(void);

int open_file(const uint8_t* filename);

int read_by_data_txt(unsigned int index);

int read_by_data_exe(unsigned int index);

int check_file(unsigned char *fname);

int read_file(unsigned int inode, unsigned int offset, void* buffer, int32_t length);

int read_file_data(unsigned char *fname, unsigned int offset, unsigned char *buf, unsigned int length);

int write_file(const void* buf, int32_t size);

int close_file(void);

#endif
