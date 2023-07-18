#include "types.h"

#ifndef _FILE_CALLS_H
#define _FILE_CALLS_H
#define DATA_BLOCK_SIZE 4096

// struct definitions
typedef struct dirEntryStruct
{
    char fileName[32];
    uint32_t fileType;
    uint32_t inodeNum;  
    uint8_t reserved[24]; 
}dirEntry_t;


typedef struct bootBlockStruct
{
    uint32_t num_dir_entries;
    uint32_t num_inode;
    uint32_t num_data_blocks;
    uint8_t reserved[52];
    dirEntry_t dirList[63];
}bootBlock_t;

typedef struct inodeStruct
{
    uint32_t length;
    uint32_t dataBlockNum[1023]; //4096B - 4 for length, divided by 4 per 32bit number
}inode_t;

typedef struct dataBlockStruct
{
    uint8_t data[4096]; //4096B per block
}dataBlock_t;



// function primitives
int32_t read_dentry_by_name(const uint8_t* fname, dirEntry_t* dirEntry);

int32_t read_dentry_by_index(uint32_t index, dirEntry_t* dirEntry);

int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

void fileSystem_init(uint32_t* fs_start);

int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);

int32_t file_read(int32_t fd, void* buf, int32_t nbytes);

int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes);

int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);

int32_t dir_open(const uint8_t* filename);

int32_t file_open(const uint8_t* filename);

int32_t dir_close(int32_t fd);

int32_t file_close(int32_t fd);

#endif /*_FILE_CALLS_H*/
