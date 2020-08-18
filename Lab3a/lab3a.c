#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/stat.h> // For macros of file_type
#include <time.h>

#include "ext2_fs.h"

int debug = 0;

int fd = 0;
unsigned int blocks_count = 0, inodes_count = 0, block_size = 0, inode_size = 0, blocks_per_group = 0, inodes_per_group = 0, first_non_reserved_inode = 0;
struct ext2_super_block super;
struct ext2_group_desc group;

void get_superblock_summary() {
	if(pread(fd, &super, sizeof(super), 1024) == -1) {
		fprintf(stderr, "Read fle system error for SUPERBLOCK with errno %d: %s. Exiting with return code 1\n", errno, strerror(errno));
      	exit(1);
	} 

	blocks_count = super.s_blocks_count;
	inodes_count = super.s_inodes_count;
	block_size = 1024 << super.s_log_block_size;  /* calculate block size in bytes */
	if (debug)
		fprintf(stderr,"block_size: %u\n",block_size);
	inode_size = super.s_inode_size;
    blocks_per_group = super.s_blocks_per_group;
    inodes_per_group = super.s_inodes_per_group;
    first_non_reserved_inode = super.s_first_ino; 
    fprintf(stdout, "SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n", blocks_count, inodes_count, block_size, inode_size, blocks_per_group, inodes_per_group, first_non_reserved_inode);
}

int is_block_used(int bno, char * bitmap) 
{
	int index = 0, offset = 0; 
	if (bno == 0)  	
		return 1;

	index = (bno - 1)/8;
	offset = (bno - 1)%8;
	if(debug) 
		fprintf(stderr, "Index: %d, Offset: %d\n", index, offset);
	return ((bitmap[index] & (1 << offset)));
}

void get_free_block_entries(__u32 block_bitmap_block) {
	char* block_bitmap = (char*) malloc(blocks_count/8);
	if(block_bitmap == NULL){
		fprintf(stderr, "Malloc failure for bitmap with errno %d: %s. Exiting with return code 1\n", errno, strerror(errno));
      	exit(1);
	}
	if(pread(fd, block_bitmap, blocks_count/8, block_size*block_bitmap_block) == -1) {
		fprintf(stderr, "Read file system error for Block Bitmap with errno %d: %s. Exiting with return code 1\n", errno, strerror(errno));
      	exit(1);
	} 
	//if(debug)
	//	fprintf(stderr, "%s\n", bitmap);
	__u32 i = 0;
	for (i = 0; i <= blocks_count; i++) {
		if(debug) {
			fprintf(stderr, "Block: %d\n", i);
		}
		if(!is_block_used(i, block_bitmap)) {
			fprintf(stdout, "BFREE,%d\n", i);
		}
	}

	free(block_bitmap);
}

void print_gmttime(time_t seconds_since_epoch) {
	struct tm *time;
	time = gmtime(&seconds_since_epoch);
	fprintf(stdout, ",%02d/%02d/%02d %02d:%02d:%02d", time->tm_mon+1, time->tm_mday, (1900+time->tm_year)%100, time->tm_hour, time->tm_min, time->tm_sec);
}

void get_dir_entry(__u32 inode_block, __u32 parent_inode) {
	__u32 offset = 0;
	struct ext2_dir_entry entry;
	while (offset < block_size) {
		if(pread(fd, &entry, sizeof(entry), inode_block*block_size+offset) == -1) {
			fprintf(stderr, "Read file system error for Directory Entries with errno %d: %s. Exiting with return code 1\n", errno, strerror(errno));
			exit(1);
		}
		char file_name[EXT2_NAME_LEN+3];
		file_name[0] = '\'';  
        memcpy(&file_name[1], entry.name, entry.name_len);
        file_name[entry.name_len+1] = '\'';   
        file_name[entry.name_len+2] = 0;              /* append null char to the file name */
        if (entry.inode != 0)
        	fprintf(stdout, "DIRENT,%u,%u,%u,%u,%u,%s\n", parent_inode, offset, entry.inode, entry.rec_len, entry.name_len, file_name);
       // entry = (void*) entry + entry.rec_len;      /* move to the next entry */
        offset += entry.rec_len;
	}
}

void handle_ind_block(__u32 block_no, int ind_level, __u32 owner_inode_no, char owner_file_type, __u32 logical_offset) {
	__u32 *blocks = malloc(block_size);
	if(blocks == NULL){
		fprintf(stderr, "Malloc failure for Indirect Blocks with errno %d: %s. Exiting with return code 1\n", errno, strerror(errno));
      	exit(1);
	}

	if(pread(fd, blocks, block_size, block_size*block_no) == -1) {
		fprintf(stderr, "Read file system error for Indirect Block with errno %d: %s. Exiting with return code 1\n", errno, strerror(errno));
      	exit(1);
	}	

	__u32 i;
	if(debug) 
		fprintf(stderr, "%lu\n", (block_size/sizeof(u_int32_t)));
	for(i = 0; i < 256; i++) {
		if (debug)
			fprintf(stderr, "%u,%u\n", i, blocks[i]);
		if (blocks[i] != 0) {
			if(owner_file_type == 'd')
				get_dir_entry(blocks[i], owner_inode_no);
			fprintf(stdout, "INDIRECT,%u,%d,%u,%u,%u\n", owner_inode_no, ind_level, logical_offset+i, block_no, blocks[i]);
		}

	}

	free(blocks);
}

void handle_dind_block(__u32 block_no, int ind_level, __u32 owner_inode_no, char owner_file_type, __u32 logical_offset) {
	__u32 *blocks = malloc(block_size);
	if(blocks == NULL){
		fprintf(stderr, "Malloc failure for Indirect Blocks with errno %d: %s. Exiting with return code 1\n", errno, strerror(errno));
      	exit(1);
	}

	if(pread(fd, blocks, block_size, block_size*block_no) == -1) {
		fprintf(stderr, "Read file system error for Indirect Block with errno %d: %s. Exiting with return code 1\n", errno, strerror(errno));
      	exit(1);
	}	

	__u32 i = 0;
	for(i = 0; i < (block_size/sizeof(__u32)); i++) {
		if (blocks[i] != 0) {
			fprintf(stdout, "INDIRECT,%u,%d,%u,%u,%u\n", owner_inode_no, ind_level, logical_offset+i, block_no, blocks[i]);
			handle_ind_block(blocks[i], 1, owner_inode_no, owner_file_type, logical_offset + i*256);
		}

	}

	free(blocks);
}

void handle_tind_block(__u32 block_no, int ind_level, __u32 owner_inode_no, char owner_file_type, __u32 logical_offset) {
	__u32 *blocks = malloc(block_size);
	if(blocks == NULL){
		fprintf(stderr, "Malloc failure for Indirect Blocks with errno %d: %s. Exiting with return code 1\n", errno, strerror(errno));
      	exit(1);
	}

	if(pread(fd, blocks, block_size, block_size*block_no) == -1) {
		fprintf(stderr, "Read file system error for Indirect Block with errno %d: %s. Exiting with return code 1\n", errno, strerror(errno));
      	exit(1);
	}	

	__u32 i = 0;
	for(i = 0; i < (block_size/sizeof(__u32)); i++) {
		if (blocks[i] != 0) {
			fprintf(stdout, "INDIRECT,%u,%d,%u,%u,%u\n", owner_inode_no, ind_level, logical_offset+i, block_no, blocks[i]);
			handle_dind_block(blocks[i], 2, owner_inode_no, owner_file_type, logical_offset + i*256*256);
		}

	}

	free(blocks);
}

void get_inode_summary(__u32 inode_table_block, __u32 inode_no) {
	struct ext2_inode inode;
	if(inode_no == 0)
		return;

	if(pread(fd, &inode, sizeof(inode), block_size*inode_table_block+(inode_no-1)*sizeof(inode)) == -1) {
		fprintf(stderr, "Read file system error for Inode with errno %d: %s. Exiting with return code 1\n", errno, strerror(errno));
      	exit(1);
	}	

    if(inode.i_mode == 0 || inode.i_links_count == 0) {
		return;
	}

	char file_type = '?';
		if(S_ISDIR(inode.i_mode)) 
		file_type = 'd';
	else if(S_ISREG(inode.i_mode)) 
		file_type = 'f';

	else if(S_ISLNK(inode.i_mode)) 
		file_type = 's';
	__u16 mode = inode.i_mode & 0xFFF;
	__u16 owner = inode.i_uid;
	__u16 group = inode.i_gid;
	__u16 link_count = inode.i_links_count;

	fprintf(stdout, "INODE,%u,%c,%o,%u,%u,%u", inode_no, file_type, mode, owner, group, link_count);
	print_gmttime(inode.i_ctime);
	print_gmttime(inode.i_mtime);
	print_gmttime(inode.i_atime);
	__u32 file_size = inode.i_size;
	__u32 disk_space = inode.i_blocks;
	fprintf(stdout, ",%u,%u", file_size, disk_space);
	if(file_type =='d' || file_type == 'f' || (file_type == 's' && file_size > 60)) {
		int i = 0;
		for(i = 0; i < 15; i++) { 
			fprintf(stdout, ",%d", inode.i_block[i]);
		}
	}
	fprintf(stdout, "\n");

	if(file_type =='d') {
		int i = 0;
		for(i = 0; i < EXT2_NDIR_BLOCKS; i++) {
			if(inode.i_block[i] != 0) {
				get_dir_entry(inode.i_block[i], inode_no);
			}
		}
	}

	if(file_type =='d' || file_type =='f') {
		if(inode.i_block[EXT2_IND_BLOCK] != 0) {
			handle_ind_block(inode.i_block[EXT2_IND_BLOCK], 1, inode_no, file_type, 12);
		}

		if(inode.i_block[EXT2_DIND_BLOCK] != 0) {
			handle_dind_block(inode.i_block[EXT2_DIND_BLOCK], 2, inode_no, file_type, 12+256);
		}

		if(inode.i_block[EXT2_TIND_BLOCK] != 0) {
			handle_tind_block(inode.i_block[EXT2_TIND_BLOCK], 3, inode_no, file_type, 12+256+256*256);
		}
	}
}

void get_free_inodes_entries(__u32 inode_bitmap_block, __u32 inode_table_block) {
	char* inode_bitmap = (char*) malloc(inodes_count/8);
	if(inode_bitmap == NULL){
		fprintf(stderr, "Malloc failure for bitmap with errno %d: %s. Exiting with return code 1\n", errno, strerror(errno));
      	exit(1);
	}
	if(pread(fd, inode_bitmap, inodes_count/8, block_size*inode_bitmap_block) == -1) {
		fprintf(stderr, "Read file system error for Block Bitmap with errno %d: %s. Exiting with return code 1\n", errno, strerror(errno));
      	exit(1);
	} 
	//if(debug)
	//	fprintf(stderr, "%s\n", bitmap);
	__u32 i = 0;
	for (i = 0; i <= inodes_count; i++) {
		if(debug) {
			fprintf(stderr, "Inode: %d\n", i);
		}
		if(!is_block_used(i, inode_bitmap)) {
			fprintf(stdout, "IFREE,%u\n", i);
		} else {
			get_inode_summary(inode_table_block, i);
		}
	}

	free(inode_bitmap);
}

void get_group_summary() {
	__u32 block_number = 0;
	__u32 block_group_descriptor_location = 0;
	if (block_size == 1024) {
		block_number = 2;
	} else {
		block_number = 1;
	}
	block_group_descriptor_location = block_size*block_number;
	if(pread(fd, &group, sizeof(group), block_group_descriptor_location) == -1) {
		fprintf(stderr, "Read fle system error for Group with errno %d: %s. Exiting with return code 1\n", errno, strerror(errno));
      	exit(1);
	} 

	__u16 free_blocks = group.bg_free_blocks_count;
	__u16 free_inodes = group.bg_free_inodes_count;
	__u32 free_block_bitmap = group.bg_block_bitmap;
	__u32 free_inode_bitmap = group.bg_inode_bitmap;
	__u32 first_block_of_inodes = group.bg_inode_table;

	fprintf(stdout, "GROUP,0,%u,%u,%u,%u,%u,%u,%u\n", blocks_count, inodes_count, free_blocks, free_inodes, free_block_bitmap, free_inode_bitmap, first_block_of_inodes);

	get_free_block_entries(free_block_bitmap);
	get_free_inodes_entries(free_inode_bitmap, first_block_of_inodes);
}

int main(int argc, char * argv[]) {
	if(argc != 2) {
		fprintf (stderr, "Correct usage is: ./lab3a fs_name\n");
		exit(1);
	}

	if((fd = open(argv[1], O_RDONLY)) == -1) {
		fprintf(stderr, "Open fle system error with errno %d: %s. Exiting with return code 1\n", errno, strerror(errno));
      	exit(1);
	}

	get_superblock_summary();
	get_group_summary();
}