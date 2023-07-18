#include "lib.h"
#include "fileSystem.h"
#include "system_calls.h"

bootBlock_t* bootBlock_ptr;
inode_t* inode_ptr;
dirEntry_t* dirEntry_ptr;
dataBlock_t* dataBlock_ptr;

extern int32_t pid;
/* void fileSystem_init(uint32_t* fs_start);
 * Inputs:      fs_start is a pointer to the start of the filesystem
 * Return Value: None
 * Function: Sets up global pointers to be used in other functions */
void fileSystem_init(uint32_t* fs_start){
    bootBlock_ptr = (bootBlock_t*) fs_start;
    dirEntry_ptr = bootBlock_ptr->dirList;
    inode_ptr = (inode_t*)(bootBlock_ptr+1); //c apparently handles pointer math by implictly incluidng a *sizeof to the int...
    dataBlock_ptr = (dataBlock_t*)(inode_ptr + bootBlock_ptr->num_inode); //thanks c
}

/* int32_t read_dentry_by_name(const uint8_t* fname, dirEntry_t* dirEntry);
 * Inputs:      fname is the name to be looked up
 *              dirEntry is struct thats passed by ref
 * Return Value: 0 on success, -1 on fail
 * Function: Finds the directory entry given a name */
int32_t read_dentry_by_name(const uint8_t* fname, dirEntry_t* dirEntry){
    if((fname == NULL) || (strlen((const int8_t*)fname) > 32)){
        return -1;
    }
    uint8_t localName[32];
    strncpy((int8_t*)&localName,(int8_t*)fname,32); // only copies 32 characters. Using strncopy since file names may not have null terminators
    int i;
    for(i = 0;i<bootBlock_ptr->num_dir_entries;i++){
        if(strncmp(bootBlock_ptr->dirList[i].fileName,(int8_t*)localName,32) == 0){
            memcpy(dirEntry,&(bootBlock_ptr->dirList[i]),sizeof(dirEntry_t));
            return 0;
        }
    }
    return -1;
}

/* int32_t read_dentry_by_index(uint32_t index, dirEntry_t* dirEntry);
 * Inputs:      index is the index of the file to be looked up
 *              dirEntry is struct thats passed by ref
 * Return Value: 0 on success, -1 on fail
 * Function: Finds the directory entry given an index */
int32_t read_dentry_by_index(uint32_t index, dirEntry_t* dirEntry){
    // int i;
    // for(i = 0;i<bootBlock_ptr->num_dir_entries;i++){
    //     if(bootBlock_ptr->dirList[i].inodeNum == index){
    //         *dirEntry = bootBlock_ptr->dirList[i];
    //         return 0;
    //     }
    // }
    // return -1;    //keeping code just in case this function is supposed to be inode
    if(index>=bootBlock_ptr->num_dir_entries){
        return -1;
    }
    memcpy(dirEntry,&(bootBlock_ptr->dirList[index]),sizeof(dirEntry_t));
    return 0;
}

/* int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);
 * Inputs:      inode is the inode number of the file
 *              offset is how far into the file to start reading
 *              buf is where the data goes
 *              length is how many bytes user wants to read
 * Return Value: 0 on success, -1 on fail
 * Function: Reads data from a given inode */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){
    if(inode>=bootBlock_ptr->num_inode){
        return -1;
    }
    inode_t* requested = (inode_t*)(inode_ptr + inode);
    if(offset>=requested->length){
        return 0;
    }

    uint32_t bytes_read = 0;
    uint32_t block_idx = offset/sizeof(dataBlock_t);
    dataBlock_t* block;
    offset = offset%sizeof(dataBlock_t); 
    uint32_t remaining_size = requested->length - offset;

    while(requested->dataBlockNum[block_idx] < bootBlock_ptr->num_data_blocks){ //checking within valid data blocks
        block = (dataBlock_t*)(dataBlock_ptr + requested->dataBlockNum[block_idx]);
        for(;offset<sizeof(dataBlock_t);offset++){
            if (bytes_read >= length)
            {
                return bytes_read;
            }
            if(bytes_read>=remaining_size){
                return bytes_read;
            }
            buf[bytes_read] = block->data[offset];
            bytes_read++; 
        }
        offset = 0;
        block_idx++;
    }
    return -1;
}


//
// System Call Section
//


/* int32_t dir_read(int32_t fd, void* buf, int32_t nbytes);
 * Inputs:      
 *              
 * Return Value: 0 on success, -1 on fail
 * Function: Lists all files in the directory */
int32_t dir_read(int32_t fd, void* buf, int32_t nbytes){
    //current implementation just attempts to list all files in bootblock
    //NOT SURE IF THIS IS RIGHT
    // int i;
    // for(i=0;i<bootBlock_ptr->num_dir_entries;i++){
    //     printf("Name:%s\t\tType:%d\tSize:%d\n",bootBlock_ptr->dirList[i].fileName,bootBlock_ptr->dirList[i].fileType,(inode_ptr+bootBlock_ptr->dirList[i].inodeNum)->length);
    // }
    // return 0;
    uint32_t dir_idx = ((pcb_t*)get_pcb_process())->fd_arr[fd].filePos;
    if(dir_idx >= bootBlock_ptr->num_dir_entries){
        return 0;
    }
    strncpy(buf,bootBlock_ptr->dirList[dir_idx].fileName,nbytes);
    ((pcb_t*)get_pcb_process())->fd_arr[fd].filePos++;
    return nbytes;
}


/* int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
 * Inputs:      buf holds filename
 *              nbytes is size of read
 * Return Value: 1+ on success, -1 on fail, 0 on reached end of file
 * Function: GIves user the data from a file with a given filename */
//This function will greatly change with file descriptors
int32_t file_read(int32_t fd, void* buf, int32_t nbytes){
    // dirEntry_t fileDir_ptr;
    // read_dentry_by_name(buf,&fileDir_ptr);
    // if(fileDir_ptr.fileType == 0){
    //     int8_t message[] = "This is a RTC file";
    //     strcpy(message,buf);
    //     return 0;
    // }
    // if(fileDir_ptr.fileType == 1){
    //     int8_t message[] = "This is a directory";
    //     strcpy(message,buf);
    //     return 0;
    // }
    // uint32_t fileInode_ptr = fileDir_ptr.inodeNum;
    uint32_t fileInode_ptr = ((pcb_t*)get_pcb_process())->fd_arr[fd].inode;
    int32_t retval;
    retval = read_data(fileInode_ptr,((pcb_t*)get_pcb_process())->fd_arr[fd].filePos,buf,nbytes);
    if(retval != -1){
        ((pcb_t*)get_pcb_process())->fd_arr[fd].filePos+=retval;
    }
    return retval;


}

/* int32_t dir_write(int32_t fd, void* buf, int32_t nbytes);
 * Inputs:      
 *              
 * Return Value: 0 on success, -1 on fail
 * Function: File system is read only so always fail */
int32_t dir_write(int32_t fd, const void* buf, int32_t nbytes){
    return -1;
}

/* int32_t file_write(int32_t fd, void* buf, int32_t nbytes);
 * Inputs:      
 *              
 * Return Value: 0 on success, -1 on fail
 * Function: File system is read only so always fail */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes){
    return -1;
}

int32_t dir_open(const uint8_t* filename){
    return 0; //will need major changes with file descriptors
}

int32_t file_open(const uint8_t* filename){
    return 0; //will need major changes with file descriptors
}

int32_t dir_close(int32_t fd){
    return 0; //will need major changes with file descriptors
}

int32_t file_close(int32_t fd){
    return 0; //will need major changes with file descriptors
}
