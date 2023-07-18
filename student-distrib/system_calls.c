#include "system_calls.h"
#include "lib.h"
#include "paging.h"
#include "x86_desc.h"

static int32_t (*rtc_jump_table[4])() = {rtc_open, rtc_read, rtc_write, rtc_close};
static int32_t (*dir_jump_table[4])() = {dir_open, dir_read, dir_write, dir_close};
static int32_t (*file_jump_table[4])() = {file_open, file_read, file_write, file_close};
static int32_t (*terminal_jump_table[4])() = {terminal_open, terminal_read, terminal_write, terminal_close};

//volatile int8_t pid =-1;
volatile int8_t PID[4] = {-1, -1, -1, -1};
    // PID[0] is total number of currently running programs
    // PID[1] is max pid of terminal 1, PID[2] is max pid of terminal 2, etc.
volatile uint8_t pid_bitmap[6] = {0, 0, 0, 0, 0, 0};
volatile uint8_t terminal_curr = 1;
volatile uint8_t terminal_next = 1;
volatile uint8_t terminal_type = 0;
volatile uint8_t terminal_on_display_curr = 1;
volatile uint8_t terminal_on_display_next = 1;
volatile uint8_t terminal_2_active = 0;
volatile uint8_t terminal_3_active = 0;
volatile uint32_t CURR_PROG_BASE_PHYS = SHELL_BASE_PHYS;     // physical base address of current user program
char args[128];

/* void print_invalid_sys_call_msg(void);
 * Inputs: none
 * Return Value: none
 * Function: prints message stating that system call code is invalid */
void print_invalid_sys_call_msg(void){
    uint32_t inval_code;
    // move value in EAX into inval_code to print it in error message
    asm volatile ("             \n\
            movl %%eax, %%edx   \n\
            movl %%edx, %0      \n\
            "
            : "=a"(inval_code)
            : /*no inputs*/
            : "edx", "memory", "cc"
    );

    printf("invalid system call code (%d) stored in EAX\n", inval_code);
}


/* int32_t halt (uint8_t status);
 * Inputs: status
 * Return Value: 0 for success, -1 for failure
 * Function: prints message stating this particular system call is being made */
/*  * Note that the system call for halt will have to make sure that only
    * the low byte of EBX (the status argument) is returned to the calling
    * task.                                                                 */ 
int32_t halt (uint32_t status){
    // printf("making system call #1: halt\n"); 
    
    if (PID[terminal_curr] >= 0){   //if (pid >= 0){
        pcb_t* pcb_pointer = get_pcb_process();
        // tss.esp0 = pcb_pointer->parentId;
        // tss.esp0 = EIGHT_MB - EIGHT_KB *pcb_pointer->parentId;
        //paging
        // virtual_to_physical(VIRTUAL_ADDRESS, EIGHT_MB+FOURUR_MB*pcb_pointer->parentId);

        // This call should never return to the caller?

        //close all files using close()
        int i;
        for(i=0; i<8;i++){
            if (pcb_pointer->fd_arr[i].flags == 1)
                close(i);
            // pcb_pointer->fd_arr[i].fileOp_ptr = NULL;
            // pcb_pointer->fd_arr[i].inode = 0;
            // pcb_pointer->fd_arr[i].filePos = 0;
            // pcb_pointer->fd_arr[i].flags = 0; 
        }
        pcb_pointer->state = 0;
        
        // "unmap" current user program page by mapping it to previous process's physical base address
        int8_t parentid = pcb_pointer->parentId;
        if (parentid >= 0){     //if (pid > 0){
            CURR_PROG_BASE_PHYS = SHELL_BASE_PHYS + /*(pid - 1)*/ parentid * TAB_NUM_ADDRS;
        }
        else{
            CURR_PROG_BASE_PHYS = SHELL_BASE_PHYS;
        }
        map_virt_user_to_physical(CURR_PROG_BASE_PHYS);

        if (PID[terminal_curr] >= 0){   //if (pid >= 0){
            uint8_t curr_pid = PID[terminal_curr];
            pid_bitmap[curr_pid] = 0;       // update pid bitmap to show that 1 pid has been freed up
            PID[0]--;   // decrement number of programs currently runnning

            if (parentid >= 0){   //if (pid >= 0){
                PID[terminal_curr] = parentid;  //pid = parentid; //pid--;
                pcb_t* pcb_parent_ptr = get_pcb_process();
                pcb_parent_ptr->state = 1;
                tss.esp0 = (uint32_t)(get_kernel_stack_bottom());
            }
            else{
                tss.esp0 = (uint32_t)(get_kernel_stack_bottom());//0x7ffffc;
                PID[terminal_curr] = parentid;
            }

            tss.ss0 = KERNEL_DS;    // already KERNEL_DS by default
            //tss.esp0 = (uint32_t)(get_kernel_stack_bottom());
        }
        // else{
        //     // pid--;
        //     tss.ss0 = KERNEL_DS;    // already KERNEL_DS by default
        //     tss.esp0 = 0x800000;
        // }

        uint32_t status_extend;

        if (status != 256){
            status = status & 0xFF;
            uint8_t status_8_bits = (uint8_t)status;
            status_extend = (uint32_t)status_8_bits;
        }
        else{
            status_extend = status;
        }

        asm volatile ("             \n\
            movl %0, %%edx   \n\
            "
            : /*no outputs*/
            : "a" (status_extend)
            : "edx", "memory", "cc"
        ); 


        asm volatile ("             \n\
                movl %0, 80(%%esp)   \n\
                "
                : /*no outputs*/
                : "a" (pcb_pointer->old_esp)
                : "memory", "cc"
        );


        if (PID[terminal_curr] >= 0){   //if (pid >= 0){
            asm volatile ("             \n\
                    movl %0, %%ebp   \n\
                    "
                    : /*no outputs*/
                    : "a" (pcb_pointer->old_ebp)
                    : "ebp", "cc"
            );
        }
        
        asm volatile ("             \n\
            movl %%edx, %0      \n\
            "
            : "=a"(status_extend)
            : /*no inputs*/
            : "edx", "memory", "cc"
        ); 

        // clear_pcb_struct((void*)pcb_pointer);

        if (PID[terminal_curr] >= 0){  //if (pid >= 0){
            return (int32_t)status_extend;
        }

        // halt_return((int32_t)status, pcb_pointer->old_ebp, pcb_pointer->old_esp);
    }

    if (PID[terminal_curr] == -1){  //if (pid == -1){
        execute((const uint8_t *)"shell");
    }

    // return 8 bits from BL, not all 32 bits from EBX
    return -1;
}


/* int32_t execute (const uint8_t* command);
 * Inputs: command
 * Return Value: 0 for success, -1 for failure
 * Function: prints message stating this particular system call is being made */
/*  * Negative returns from execute indicate that the desired program
    * could not be found.                                           */
int32_t execute (const uint8_t* command){
    if ((PID[0] == 5) || (get_next_avail_pid() == 6)){   //if (pid == 5){      // can run up to 6 processes
        return -1;
    }

    if(command == NULL){
        return -1;
    }

    int i = 0;
    
    while(command[i] == ASCII_SPACE){ //skip any leading spaces
        i++;
        if(command[i] == 0){
            return -1;
        }
    }
    char fileName[32] = "";
    int j=0; //NEED TO CHECK NULL COMMAND OR TOO BIG COMMAND LATER
    while(i< sizeof(fileName)){
        if(command[i] == 0){
            break;
        }
        if(command[i] == 32){
            break;
        }
        fileName[j] = command[i];
        i++;
        j++;
    } //with this fileName should have the initial command
    while(command[i] == ASCII_SPACE){ //skip any leading spaces
        i++;
    }
    // MAY NEED TO CHECK FOR NULL CHAR IDK THO
    strcpy((int8_t*)args,(const int8_t*)command+i); //put the rest of the args into args for later calls

    dirEntry_t requested;
    if(read_dentry_by_name((uint8_t*)fileName, &requested)== -1){
        return -1;
    }
    if(requested.fileType != 2){ //fileType is not a normal file
        return -1;
    }
    char exeCheck[4] = {0x7f,0x45,0x4c,0x46}; //the ELF comparison
    //file_read(0,fileName,4);//THIS IS WRONG
    read_data(requested.inodeNum,0,(uint8_t*)fileName,4);
    if(strncmp(exeCheck, fileName, 4) != 0){ //check if executable
        return -1;
    }
    PID[0]++;    //pid++;
    int8_t parentid = PID[terminal_curr];   // save old pid of current terminal

    uint8_t avail_pid = get_next_avail_pid();
    PID[terminal_curr] = avail_pid;     // update pid of current terminal
    pid_bitmap[avail_pid] = terminal_curr;      // update pid bitmap accordingly

    pcb_t *new_pcb = get_pcb_process();
    clear_pcb_struct((void*)new_pcb);
    new_pcb->pid = PID[terminal_curr];  //pid;
    new_pcb->parentId = parentid;//pid-1;
    if (parentid != -1)//if (PID[terminal_curr] != 0)//if (pid !=0)
    {
        asm volatile ("             \n\
            movl %%ebp, %0   \n\
            "
            : "=a"(new_pcb->old_ebp)
            : /*no inputs*/
            : "memory", "cc"
            );

        asm volatile ("             \n\
            movl %%esp, %%ebx   \n\
            movl %%ebx, %0      \n\
            "
            : "=a"(new_pcb->old_esp)
            : /*no inputs*/
            : "ebx", "memory", "cc"
        ); 

        pcb_t* parent_pcb;
        parent_pcb = get_parent_pcb_process();
        parent_pcb->state = 0;      // mark parent state as non-active
    } 

    new_pcb->state = 1;
    new_pcb->fd_arr[0].fileOp_ptr = (void *)terminal_jump_table;
    new_pcb->fd_arr[0].flags = 1;
    new_pcb->fd_arr[1].fileOp_ptr = (void*)terminal_jump_table;
    new_pcb->fd_arr[1].flags = 1; 

    if (PID[terminal_curr] >= 0 && PID[terminal_curr] < 6){     //if (pid >= 0 && pid < 6){
        CURR_PROG_BASE_PHYS = SHELL_BASE_PHYS + /*pid*/ PID[terminal_curr] * TAB_NUM_ADDRS;   // increment physical base address of user program by 4MB
    }    
    map_virt_user_to_physical(CURR_PROG_BASE_PHYS);
    // memcpy(get_pcb_process(),&new_pcb,sizeof(pcb_t));
    
    
    unsigned char eip[4];
    read_data(requested.inodeNum,24,eip,4);
    uint32_t eip_value;


    read_data(requested.inodeNum,0,(void*)0x08048000,(uint32_t)0x14048000);
    // convert characters in eip[] array into actual address
    //iret_context_switch(eip_value);

    eip_value = ((uint32_t)eip[3]<<24)+((uint32_t)eip[2]<<16)+((uint32_t)eip[1]<<8)
                +((uint32_t)eip[0]);
    
    idt[0x80].dpl=3;

    asm volatile ("             \n\
            pushl $0x2B   \n\
            pushl $0x83FFFF8      \n\
            pushl $0x202        \n\
            pushl $0x23      \n\
            movl %0, %%edx      \n\
            pushl %%edx         \n\
            "
            : /*no outputs*/
            : "r" (eip_value)
            : "edx", "memory", "cc"
    );

    // set value for DS to point to USER_DS entry when going from PL0 to PL3
    // set value for ESP to point to bottom of user page (0x83FFFFF; use 0x83FFFF8 instead)
    // set value for EFLAGS to 0x202
    // CS is 2nd to be popped off; make it point to USER_CS? last 2 bytes? bits? of CS specify privilege level
    // EIP is 1st to be popped off by IRET, so push EIP last
    // make processor pop these values into corresponding registers

    
    
    tss.ss0 = KERNEL_DS;    // already KERNEL_DS by default
    tss.esp0 = (uint32_t)(get_kernel_stack_bottom()); //curr_esp;

    asm volatile ("             \n\
        iretl                    \n\
        "
    );

    int32_t ret_val;
    asm volatile ("             \n\
        movl %%eax, %0   \n\
        "
        : "=a"(ret_val)
        : /*no inputs*/
        : "memory", "cc"
    );

    // NOTE: need to restart shell to always keep shell running
    return ret_val;
}

/* int32_t open (const uint8_t* filename);
 * Inputs: filename
 * Return Value: 0 for success, -1 for failure
 * Function: prints message stating this particular system call is being made */
/* The call should find the directory entry corresponding to the named file,
 * allocate an unused file descriptor, and set up any data necessary to handle the given type of file (directory,
 * RTC device, or regular file). If the named file does not exist or no descriptors are free, the call returns -1.*/
int32_t open (const uint8_t* filename){
    pcb_t* pcb_pointer = get_pcb_process();
    int i;
    for(i=2;i<8;i++){
        if (pcb_pointer->fd_arr[i].flags == 0){ //attempts to find an open fd
            break;
        }      
    }
    if(/*pcb_pointer->fd_arr[i].flags != 0*/ i == 8){ //if none were found then kill
    
        return -1;
    }
    int switch_val;
    dirEntry_t requested;
    if(strncmp((const int8_t*)filename,"rtc",(uint32_t)sizeof(filename))!=0){

        if(read_dentry_by_name(filename, &requested) == -1){
        return -1;
        }
        switch_val = requested.fileType;
    }
    else{
        switch_val = 0; //this means rtc
    }

    // now we have known fd number and dir_entry so need to fill in values
    pcb_pointer->fd_arr[i].flags = 1; //set in use
    switch (switch_val){
    case 0: //rtc
        pcb_pointer->fd_arr[i].fileOp_ptr = (void *) rtc_jump_table;
        pcb_pointer->fd_arr[i].inode = 0;
        pcb_pointer->fd_arr[i].filePos = 0;
        pcb_pointer->fd_arr[i].fileOp_ptr[0](filename);
        break;
        
    case 1: //dir
        pcb_pointer->fd_arr[i].fileOp_ptr = (void *) dir_jump_table;
        pcb_pointer->fd_arr[i].inode = 0;
        pcb_pointer->fd_arr[i].filePos = 0;
        pcb_pointer->fd_arr[i].fileOp_ptr[0](filename);
        break;

    case 2: //file
        pcb_pointer->fd_arr[i].fileOp_ptr = (void *) file_jump_table;
        pcb_pointer->fd_arr[i].inode = requested.inodeNum;
        pcb_pointer->fd_arr[i].filePos = 0;
        pcb_pointer->fd_arr[i].fileOp_ptr[0](filename);
        break;            
    default:
        return -1;
    }

    return i;
}

/* int32_t read (int32_t fd, void* buf, int32_t nbytes);
 * Inputs: fd, buf, nbytes
 * Return Value: 0 for success, -1 for failure
 * Function: prints message stating this particular system call is being made */
int32_t read (int32_t fd, void* buf, int32_t nbytes){
    // printf("making system call #3: read\n");
    if (fd < 0 || fd == 1 || fd >= 8){     // check if fd is valid (also write not allowed)
        return -1;
    }
    pcb_t* global_pcb_ptr = get_pcb_process();
    if (global_pcb_ptr->fd_arr[fd].flags == 0){      // if fd isn't open, don't read from it
        return -1;
    }
    sti();
    int32_t ret_val = ((int32_t(*)(int32_t, void*, int32_t))(global_pcb_ptr->fd_arr[fd].fileOp_ptr[1]))(fd,buf,nbytes);
    // global_pcb_ptr->fd_arr[fd].filePos+=ret_val;
    return ret_val;
}


/* int32_t write (int32_t fd, const void* buf, int32_t nbytes);
 * Inputs: fd, buf, nbytes
 * Return Value: 0 for success, -1 for failure
 * Function: prints message stating this particular system call is being made */
int32_t write (int32_t fd, const void* buf, int32_t nbytes){
    // printf("making system call #4: write\n");
    if (fd <= 0 || fd >= 8){         // check if fd is valid (also read not allowed)
        return -1;
    }
    pcb_t* global_pcb_ptr = get_pcb_process();
    if (global_pcb_ptr->fd_arr[fd].flags == 0){      // if fd isn't open, don't write to it
        return -1;
    }
    return ((int32_t(*)(int32_t, const void*, int32_t))(global_pcb_ptr->fd_arr[fd].fileOp_ptr[2]))(fd,buf,nbytes);
}



/* int32_t close (int32_t fd);
 * Inputs: fd
 * Return Value: 0 for success, -1 for failure
 * Function: prints message stating this particular system call is being made */
/* The close system call closes the specified file descriptor and makes it available for return from later calls to open.
 * You should not allow the user to close the default descriptors (0 for input and 1 for output). Trying to close an invalid
 * descriptor should result in a return value of -1; successful closes should return 0.*/
int32_t close (int32_t fd){
    //printf("making system call #6: close\n");
    pcb_t* global_pcb_ptr = get_pcb_process();
    if(fd <=0 || fd == 1 || fd >= 8){       // check if fd is valid
        return -1;
    }
    if (global_pcb_ptr->fd_arr[fd].flags == 0){      // if fd isn't open, don't close it
        return -1;
    }

    int32_t ret_val = 0;
    
    ret_val = ((int32_t(*)(int32_t))(global_pcb_ptr->fd_arr[fd].fileOp_ptr[3]))(fd);
    global_pcb_ptr->fd_arr[fd].flags = 0; 
    global_pcb_ptr->fd_arr[fd].fileOp_ptr = NULL;
    global_pcb_ptr->fd_arr[fd].filePos = 0;
    global_pcb_ptr->fd_arr[fd].inode = 0;
    return ret_val;
}


/* int32_t getargs (uint8_t* buf, int32_t nbytes);
 * Inputs: buf, nbytes
 * Return Value: 0 for success, -1 for failure
 * Function: prints message stating this particular system call is being made */
int32_t getargs (uint8_t* buf, int32_t nbytes){
    //printf("making system call #7: getargs\n");
    if(strlen(args)>=nbytes){//need buf to be 1+strlen
        return -1;
    } 
    if(args[0] == NULL){    // check for NULL character
        return -1;
    }
    strncpy((int8_t*)buf,(const int8_t*)args,(uint32_t)nbytes);
    return 0;
}


/* int32_t vidmap (uint8_t** screen_start);
 * Inputs: screen_start
 * Return Value: 0 for success, -1 for failure
 * Function: prints message stating this particular system call is being made */
int32_t vidmap (uint8_t** screen_start){
    // printf("making system call #8: vidmap\n");
    // check if input is a virtual address within user space
    if((uint32_t)screen_start < USER_BASE_VIRT || (uint32_t)screen_start >= USER_BASE_VIRT_4MB){
        return -1;
    }
    map_user_vidmap();  // update paging structure to grant user access to video memory
    *screen_start = (uint8_t*)USER_BASE_VIRT_8MB;

    return 0;
}


/* int32_t set_handler (int32_t signum, void* handler_address);
 * Inputs: signum, handler_address
 * Return Value: 0 for success, -1 for failure
 * Function: prints message stating this particular system call is being made */
int32_t set_handler (int32_t signum, void* handler_address){ // extra credit
    //printf("making system call #9: set_handler\n");
    return 0;
}


/* int32_t sigreturn (void);
 * Inputs: none
 * Return Value: 0 for success, -1 for failure
 * Function: prints message stating this particular system call is being made */
int32_t sigreturn (void){   // also extra credit
    //printf("making system call #10: sigreturn\n");
    return 0;
}

void* get_pcb_process(void){
    void* pcb= (void*) (EIGHT_MB - EIGHT_KB *(/*pid*/ PID[terminal_curr]+1));
    return pcb;
}
//mb,kb

void* get_parent_pcb_process(void){
    void* pcb_parent = (void*) (EIGHT_MB - EIGHT_KB *(/*pid*/ PID[terminal_curr]));
    return pcb_parent;
}

void* get_kernel_stack_bottom(void){
    void* stack_bottom = (void*) (EIGHT_MB - EIGHT_KB *(/*pid*/ PID[terminal_curr]) - 4);
    return stack_bottom;
}

void* get_pcb_process_next(void){
    void* pcb_next = (void*) (EIGHT_MB - EIGHT_KB *(/*pid*/ PID[terminal_next]+1));
    return pcb_next ;
}

void* get_kernel_stack_bottom_next(void){
    void* stack_bottom_next = (void*) (EIGHT_MB - EIGHT_KB *(/*pid*/ PID[terminal_next]) - 4);
    return stack_bottom_next;
}


/* uint8_t get_next_avail_pid(void);
 * Inputs: none
 * Return Value: return 0-5 as available pid number; return 6 for failure i.e. no more programs can be run
 * Function: iterates through pid bitmap to see if there's a currently ununsed pid */
uint8_t get_next_avail_pid(void){
    uint8_t i;
    for (i = 0; i < 6; i++){
        if (pid_bitmap[i] == 0){
            return i;
        }
    }
    return 6;
}


/* void clear_pcb_struct(void* ptr);
 * Inputs: ptr (pointer to pcb struct)
 * Return Value: none
 * Function: clears newly created pcb struct of prexisting data on stack in case there's any */
void clear_pcb_struct(void* ptr){
    pcb_t* pcb_ptr = (pcb_t*)ptr;
    pcb_ptr->pid = -1;
    pcb_ptr->parentId = -1;
    int i;
    for (i = 0; i < 8; i ++){
        pcb_ptr->fd_arr[i].fileOp_ptr = NULL;
        pcb_ptr->fd_arr[i].inode = 0;
        pcb_ptr->fd_arr[i].filePos = 0;
        pcb_ptr->fd_arr[i].flags = 0;
    }
    pcb_ptr->old_esp = 0;
    pcb_ptr->old_ebp = 0;
    pcb_ptr->schedule_esp = 0;
    pcb_ptr->schedule_ebp = 0;
    pcb_ptr->state = 0;
    // pcb_ptr->screen_x = 0;
    // pcb_ptr->screen_y = 0;
    // pcb_ptr->terminal_save.EDI = 0;
    // pcb_ptr->terminal_save.ESI = 0;
    // pcb_ptr->terminal_save.EDI = 0;
    // pcb_ptr->terminal_save.Temp = 0;
    // pcb_ptr->terminal_save.EBX = 0;
    // pcb_ptr->terminal_save.EDX = 0;
    // pcb_ptr->terminal_save.ECX = 0;
    // pcb_ptr->terminal_save.EAX = 0;
    // pcb_ptr->terminal_save.IRQ = 0;
    // pcb_ptr->terminal_save.Error = 0;
    // pcb_ptr->terminal_save.Ret_Addr = 0;
    // pcb_ptr->terminal_save.XCS = 0;
    // pcb_ptr->terminal_save.EFLAGS = 0;
    // pcb_ptr->terminal_save.ESP = 0;
    // pcb_ptr->terminal_save.XSS = 0;
}

