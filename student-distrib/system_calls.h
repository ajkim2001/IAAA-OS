#include "types.h"
#include "rtc_init.h"
#include "fileSystem.h"
#include "terminal.h"
#include "handler_setup.h"

#ifndef _SYSTEM_CALLS_H
#define _SYSTEM_CALLS_H


#define EIGHT_MB 0x800000
#define EIGHT_KB 0x2000
#define VIRTUAL_ADDRESS 0x8000000 //128 MB
#define FOUR_MB EIGHT_MB/2
#define ASCII_SPACE 32

extern void print_invalid_sys_call_msg(void);
extern int32_t halt (uint32_t status);  /*   * Note that the system call for halt will have to make sure that only
                                            * the low byte of EBX (the status argument) is returned to the calling
                                            * task.                                                                 */ 
extern int32_t execute (const uint8_t* command);/*  * Negative returns from execute indicate that the desired program
                                                    * could not be found.                                           */

extern int32_t read (int32_t fd, void* buf, int32_t nbytes);
extern int32_t write (int32_t fd, const void* buf, int32_t nbytes);
extern int32_t open (const uint8_t* filename);
extern int32_t close (int32_t fd);
extern int32_t getargs (uint8_t* buf, int32_t nbytes);
extern int32_t vidmap (uint8_t** screen_start);
extern int32_t set_handler (int32_t signum, void* handler_address); // extra credit
extern int32_t sigreturn (void);    // also extra credit
void* get_pcb_process(void);
void* get_parent_pcb_process(void);
void* get_kernel_stack_bottom(void);
void* get_pcb_process_next(void);
void* get_kernel_stack_bottom_next(void);
void* get_pcb_process_take_input(uint8_t terminal_num);
void* get_parent_pcb_process_take_input(uint8_t terminal_num);
void* get_kernel_stack_bottom_take_input(void);
uint8_t get_next_avail_pid(void);
void clear_pcb_struct(void* pcb_ptr);
void flush_tlb();
extern int32_t halt_return(int32_t ret_val, uint32_t old_ebp, uint32_t old_esp);

typedef struct fileDescriptorStruct
{
    int32_t* (**fileOp_ptr)();  // pointer to an array of function pointers for functions that take inputs; used to differentiate
                                // between different usages (RTC, terminal, directory, files) of open, close, read, write system calls
    uint32_t inode;
    uint32_t filePos;           // where in the file the user is at when reading the file
    uint32_t flags;             // 1 if this file descriptor is currently in use; 0 otherwise
}fd_t;


typedef struct pcbStruct
{
    int8_t pid;         // PID of active program
    int8_t parentId;    // PID of parent process that runs in the same terminal
    fd_t fd_arr[8];     // arrary of 8 file descriptor structs; see above definition
    uint32_t old_esp;   // ESP & EBP values of parent process before child process is run;
    uint32_t old_ebp;   // used to allow switching between parent & child processes within same terminal
    uint32_t schedule_esp;  // ESP & EBP values of child process in terminal before context is switched from this terminal's to another terminal's;
    uint32_t schedule_ebp;  // used to allow switching of processes between different terminals
    uint8_t state;          // 1 if this PCB struct is currently in use; 0 otherwise; not actually being used for anything
}pcb_t;

extern volatile int8_t PID[4];
    // PID[0] is total number of currently running programs
    // PID[1] is max pid of terminal 1, PID[2] is max pid of terminal 2, etc.; all initialized to -1
extern volatile uint8_t pid_bitmap[6];
    // bitmap showing availibility of PIDs; pid_bitmap[i] = 0 means i is a free pid number;
    // pid_bitmap[i] = 1/2/3 means a process/ program w/ pid i is running (active or inactive) in terminal 1/2/3
extern volatile uint8_t terminal_curr;      // id # of currently running terminal; 1 for terminal 1, 2 for terminal 2, etc.
extern volatile uint8_t terminal_next;      // id # of terminal scheduled to run next
extern volatile uint8_t terminal_type;      // index that we use to "remap" cursor position "pages"
extern volatile uint8_t terminal_on_display_curr;   // id # of currently displaying terminal
extern volatile uint8_t terminal_on_display_next;   // id # of terminal to be displayed next (due to user pressing alt+f)
extern volatile uint8_t terminal_2_active;          // 0 if terminal 2 hasn't yet been created, 1 otherwise
extern volatile uint8_t terminal_3_active;          // 0 if terminal 2 hasn't yet been created, 1 otherwise


#endif /* _SYSTEM_CALLS_H */
