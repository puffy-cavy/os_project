
#include "file_system.h"

// Pointer to the Bootblock of the File System
bootblock *bl;

/* init_file_system()
 * Load the module start address to the pointer of Bootblock
 *
 * Inputs: module_start - The Start Address of File System Module Loaded from kernel.c 
 * Outputs: None
 */
void init_file_system(unsigned int module_start) {
	
	bl = (void *) module_start;
}

/* read_dentry_by_name()
 * Find the dentry according to the file name passed in
 *
 * Inputs: *fname - The file name that we want to search 
 * Outputs:  0 - Found the file successfully
 *          -1 - Unable to find the file
 */
int read_dentry_by_name(const unsigned char *fname, dentry_t *dentry) {
	// Loop Counter
	int i = 0;
	// Maximum Number of Dentries
	unsigned int dentry_max = 63;
	// Look for Matching Names
	for (; i < dentry_max; i++) {
		if (same_name(fname, &(bl->dentries[i])) != -1) {
			deep_copy_dentry(dentry, &(bl->dentries[i]));
			return 0;
		}
	}
	return -1;
}

/* same_name()
 * Helper funciton to compare whether the two input strings are the same (checks first 32 chars)
 *
 * Inputs:     *fname - The file name that we want to search
 *         cur_dentry - Current dentry to check file name
 * Outputs:  1 - Same Name
 *          -1 - Different Names
 */
int same_name(const unsigned char *fname, dentry_t *cur_dentry) {
	int i = 0;
	int filename_length = 32;
	// Check that fname is Not a NULL String
	if (fname[0] == '\0') return -1;
	// Check each char in file name
	for (; i < filename_length; i++) {
		// If fname is Terminated by Spaces or NULL and all Previous Characters Match
		if ((fname[i] == ' ') && (cur_dentry->file_name[i] == ' ')) return 1;
		if ((fname[i] == '\0') && (cur_dentry->file_name[i] == ' ')) return 1;
		if ((fname[i] == '\0') && (cur_dentry->file_name[i] == '\0')) return 1;
		if ((fname[i] == ' ') && (cur_dentry->file_name[i] == '\0')) return 1;
		
		// If the chars at the specific index are not the same, return -1
		if ((fname[i] != (cur_dentry->file_name[i]))) {
			return -1;
		}
		// If fname is first to reach EOS, return -1
		if ((fname[i] == '\0') && ((cur_dentry->file_name[i]) != '\0')) {
			return -1;
		}
		// If current_dentry's name first reaches EOS, return -1
		if ((fname[i] != '\0') && ((cur_dentry->file_name[i]) == '\0')) {
			return -1;
		}
	}
	// Checked till end of the file name, return 0
	return 1;

}

/* deep_copy_dentry()
 * Deep copy each attribute of the currenty dentry to the dentry passed in as the first argument
 *
 * Inputs:     *dentry - The dentry_t pointer to copy to
 *         *cur_dentry - The dentry_t pointer to copy from
 * Outputs: None
 */
void deep_copy_dentry(dentry_t *dentry, dentry_t *cur_dentry) {
	int j;
	int filename_length = 32;
	
	//clear the dentry->file_name[], otherwise may store garbage
	for( j = 0; j < filename_length; j++){
		dentry->file_name[j] = '\0';
	}
	// Copy each char of the current dentry's filename to dentry
	for ( j = 0; j < filename_length; j++) {
		if (cur_dentry->file_name[j] != '\0')
			dentry->file_name[j] = cur_dentry->file_name[j];
		else 
			break;

	}
	// Set the last char of file name to be EOS
	dentry->file_name[j] = '\0';
	// Copy file_type and inode index to the dentry
	dentry->file_type = cur_dentry->file_type;
	dentry->inode_index = cur_dentry->inode_index;
}

/* read_dentry_by_index()
 * Find the dentry by according to the given index
 * 
 * Inputs: *dentry - Pointer to chosen dentry
 * Outputs: 0 on Success
 */
unsigned int read_dentry_by_index(unsigned int index, dentry_t *dentry) {
	unsigned int dentry_index_max = 62;
	// If the input index is not in valid range, return -1
	if ((index < 0) || (index > dentry_index_max))
		return -1;
	// If we find the valid dentry at the given index, deep copy it
	deep_copy_dentry(dentry, &(bl->dentries[index]));
	return 0;
}

/* read_data()
 * Read the data in the file specified by a given inode index, store the data read in a buffer
 * the start point and length to be read is given as offset and length
 *
 * Inputs:  inode - The inode index to be read
 *         offset - The start point of a file to be read
 *            buf - The buffer to store the data to be read
 *         length - The size in bytes, tells how many data need to be read 
 * Outputs: The number of bytes successfully read
 */
unsigned int read_data(unsigned int inode, unsigned int offset, unsigned char *buf, unsigned int length) {
	unsigned int copy_length;
	unsigned int rest_length;
	unsigned int buff_idx = 0;
	unsigned int reach_end_flag = 0;
	unsigned int fourkb = 4096;
	unsigned int four = 4;
	
	// Check if the inode index is invalid
	if(inode >= bl->num_inodes) {
		printf("FS.READ_DATA: ERR - Invalid Inode \n");
		return -1;
	}
	
	unsigned int num_data_block;
	num_data_block = bl->num_data_blocks; 

	// The address of the inode that stores the index of data blocks
	//CP3.4: NEWLY CHANGED :if (offset > *((unsigned int*)inode_addr)){
	unsigned char *inode_addr; 
	inode_addr = ((unsigned char*) bl + (inode + 1) * fourkb);
	
	// If offset s less that zero, invalid, return -1
	if (offset < 0) return -1;
	// If offset is greater than the data length or less than zero, return -1
	if (offset > *((unsigned int*)inode_addr)){
		printf("FS.READ_DATA: ERR - Offset Greater than Length of File \n");
		return 0;
	}
	
	// true_length is the length of data that can be read, since the length argument may exceed file length
	// If length is greater than the data size from offset, cut it off
	unsigned int true_length = *((unsigned int*) inode_addr) - offset;
	if (true_length > length)
		true_length = length;
	// Copy will reach or exceed the end of the data block
	else reach_end_flag = 1;
	
	// Calculate which data block corresponds to offset
	unsigned int start_data_block = offset / fourkb;
	unsigned int start_data_offset = offset % fourkb;
	
	// data_block_idx_ptr indicates the memory location to store the data block index
	unsigned char *data_block_idx_ptr; 
	data_block_idx_ptr = inode_addr + (1 + start_data_block) * 4;
	
	// Check if data block index is out of range
	if(*((unsigned int*) data_block_idx_ptr) >= num_data_block) {
		printf("FS.READ_DATA: ERR(1) - Data Block Index Out of Range \n");
		return -1;
	}
	
	unsigned int data_block_start_addr = ((unsigned int)((unsigned char*) bl) + (1 + bl->num_inodes + (*((unsigned int*) data_block_idx_ptr))) * fourkb);
	// Need to copy data from the rest block
	if (true_length > (fourkb - start_data_offset)) {
		copy_length = fourkb - start_data_offset;
		rest_length = true_length - copy_length;
	}
	// Copy falls within the block
	else {
		copy_length = true_length;
		rest_length = 0;
	}
	
	// Call helper function to copy data from a block
	copy_data(data_block_start_addr, start_data_offset, buf, copy_length, buff_idx);
	buff_idx +=copy_length;
	
	if (true_length <= (fourkb - start_data_offset)) {
		return true_length;
	}
	
	// Copy the entire data block
	while ((rest_length / fourkb) > 0.00) {
		data_block_idx_ptr += four;
		// Check if data block index is out of range
		if(*((unsigned int*) data_block_idx_ptr) >= num_data_block)
			return -1; 
		data_block_start_addr = ((unsigned int)((unsigned char*) bl) + (1 + bl->num_inodes + (*((unsigned int*) data_block_idx_ptr))) * fourkb);
		// Call helper function to copy data from a block
		copy_data(data_block_start_addr, 0, buf, fourkb, buff_idx);
		buff_idx += fourkb;
		rest_length -= fourkb;
	}
	
	if (rest_length <= 0) {
		return true_length;
	}
	
	// Copy the last block
	data_block_idx_ptr += four;
	// Check if data block index is out of range
	if(*((unsigned int*)data_block_idx_ptr) >= num_data_block) {
		printf("FS.READ_DATA: ERR(2) - Data Block Index Out of Range \n");
		return -1;
	}
	
	data_block_start_addr = ((unsigned int)((unsigned char*) bl) + (1 + bl->num_inodes + (*((unsigned int*) data_block_idx_ptr))) * fourkb);
	// Call helper function to copy data from a block
	copy_data(data_block_start_addr, 0, buf, rest_length, buff_idx);
	return true_length;
}

/* copy_data()
 * Helper function to copy data stored in data block into buffer
 *
 * Inputs: start_addr - The start address to copy data
 *                buf - Buffer that stores the data
 *             length - The size of data that needs to be copied
 *         buff_index - The index in the buffer that starts to store data
 * Outputs: None
 */
void copy_data(unsigned int start_addr, unsigned int offset, unsigned char *buf, unsigned int length, unsigned int buff_index) {
	unsigned int i = 0;
	unsigned char *cur_addr;
	cur_addr = ((unsigned char*)(start_addr + offset + i));
	for (; i < length; i++) {
		cur_addr = ((unsigned char*)(start_addr + offset + i));
		buf[buff_index] = *cur_addr;
		buff_index ++;
	}
}

/* open_directory()
 * Open a specific directory
 *
 * Inputs: None Effective
 * Outputs: 0
 */
int open_directory(const uint8_t* filename) {
	
	return 0;
}

// TODO
int read_directory(unsigned int inode, unsigned int offset, void* buf, int32_t nbytes){
	
	// Loop Counter
	uint8_t j = 0;
	char* buffer = (char*) buf;
	int max = bl->num_dentries;
	
	if (file_read_dentry >= max) {
		file_read_dentry = 0;
		return 0;
//		printf("FS.READ_DIRECTORY: ERR - Invalid Dentry %d \n", file_read_dentry);
//		return -1;
	}
	if (file_read_dentry < 0) {
		printf("FS.READ_DIRECTORY: ERR - Invalid Dentry %d \n", file_read_dentry);
		return -1;
	}
	unsigned int success_flag = 0;
	
	dentry_t cur_dentry;
	success_flag = read_dentry_by_index(file_read_dentry, &cur_dentry);
	if(success_flag == -1) {
		printf("FS.READ_DIRECTORY: ERR - Unable to Find Dentry %d \n", file_read_dentry);
		return -1;
	}
	
	file_read_dentry++;
	for (; j < 32; j++) {
		if (cur_dentry.file_name[j] != '\0') {
			buffer[j] = cur_dentry.file_name[j];
		}
		else { 
			break;
		}
	}
	return j;
}

/* write_directory()
 * Write is Invalid since FS is READ ONLY
 *
 * Inputs:
 * Outputs:
 */
int write_directory(const void* buf, int32_t size) {
	
	printf("FS.WRITE_DIRECTORY: ERR - File System is READ ONLY \n");
	return -1;
}

/* close_directory()
 * Close a specific directory
 *
 * Inputs:
 * Outputs:
 */
int close_directory(void) {
	
	return 0;
}

/* open_file()
 * Open the File with given File Name
 *
 * Inputs: filename - File Name
 * Outputs: 0
 */
int open_file(const uint8_t* filename) {

	return 0;
}

/* read_by_data_txt()
 * Read a txt file and print out the first KB
 *
 * Inputs: index - The inode index to be read
 * Outputs: 0
 */
int read_by_data_txt(unsigned int index) {
	// Text buffer to store read chars from file
	unsigned char txt_buf[1024];
	int length = read_data(index, 0, txt_buf, 1024);
	
	// Check the number of bytes read is positive
	if (length == -1)
		return -1;
	int j = 0;
	
	// Display the buffer read from file
	for (; j< length; j++) {
		printf("%c", txt_buf[j]);
	}
	return 0;
}

/* read_by_data_exe()
 * Read an executable and print out the first 32 char of it
 *
 * Inputs: index - The inode index
 * Outputs: 0
 */
int read_by_data_exe(unsigned int index) {

	unsigned char exe_buf[32];
	int j = 0;
	
	int length = read_data(index, 0, exe_buf, 32);
	if (length == -1) return -1;
	
	// Display the buffer read from file
	for (; j < 32; j ++) {
		printf("%c ", exe_buf[j]);
	}
	
	return 0;
}

/* check_file()
 * Checks what type of file fname is if it can be found
 *
 * Inputs: fname - The name of the file that we want to check
 * Outputs:  0 - Executable
 *           1 - Other
 *          -1 - Not Found or Invalid Argument
 */
int check_file(unsigned char *fname) {	
	dentry_t cur_dentry;
	
	// Check for Null Pointer
	if (fname == NULL) {
		printf("FS.CHECK_FILE: ERR - Invalid Pointer \n");
		return -1;
	}
	if (VERBOSE) printf("FS.CHECK_FILE: Checking %s is Executable \n", fname);
	
	// Find the dentry according to the file name that we input
	int result_temp = read_dentry_by_name(fname, &cur_dentry);
	if (result_temp == -1) {
		printf("FS.CHECK_FILE: ERR - Unable to Find Dentry \n");
		return -1;
	}
	
	unsigned int target_inode = cur_dentry.inode_index;
	unsigned int short_buffer_size = 16;
	unsigned int four = 4;
	
	// 0x7f check whether the file we are currently reading is an executable
	unsigned char exe_begin = 0x7f;
	unsigned char short_buffer[short_buffer_size];
	read_data(target_inode, 0, short_buffer, four);
	
	// Check if File is Executable
	if (short_buffer[0] == exe_begin) {
		if (VERBOSE) printf("FS.CHECK_FILE: File is Executable \n");
		return 0;
	}
	
	if (VERBOSE) printf("FS.CHECK_FILE: File is NOT Executable \n");
	return 1;	
}

/* read_file()
 * Direct call to read_data, Refer to read_data() for details
 *
 */
int read_file(unsigned int inode, unsigned int offset, void* buffer, int32_t length){
	// Direct Call to Helper
	return read_data(inode, offset, buffer, length);
}

/* read_file_data()
 * Read the data in the file specified by a file name, store the data read in a buffer
 * the start point and length to be read is given as offset and length
 *
 * Inputs:  inode - The inode index to be read
 *         offset - The start point of a file to be read
 *            buf - The buffer to store the data to be read
 *         length - The size in bytes, tells how many data need to be read 
 * Outputs: The number of bytes successfully read
 */
int read_file_data(unsigned char *fname, unsigned int offset, unsigned char *buf, unsigned int length) {
	dentry_t cur_dentry;
	int32_t retval;
	
	// Check for Null Pointer
	if (fname == NULL) {
		printf("FS.READ_FILE_DATA: ERR - Invalid Pointer \n");
		return -1;
	}
	
	// Find the dentry according to the file name that we input
	int result_temp = read_dentry_by_name(fname, &cur_dentry);
	if (result_temp == -1) {
		printf("FS.READ_FILE_DATA: ERR - Unable to Find Dentry \n");
		return -1;
	}
	
	// Get the Inode Index
	unsigned int target_inode = cur_dentry.inode_index;

	// Call Read Data
	retval = read_data(target_inode, offset, buf, length);
	
	return retval;
}

/* write_file()
 * Invalid Write to a File since FS is Read Only
 * 
 * Inputs: None Effective
 * Outputs: -1
 */
int write_file(const void* buf, int32_t size) {	
	
	printf("FS.WRITE_FILE: ERR - File System is READ ONLY \n");
	return -1;
}

/* close_file()
 * Closes a File
 * 
 * Inputs: None
 * Outputs: 0
 */
int close_file(void) {
	
	return 0;
}
