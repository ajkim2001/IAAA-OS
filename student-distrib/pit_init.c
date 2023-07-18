#include "pit_init.h"
#include "handler_setup.h"
#include "x86_desc.h"
#include "handler_call.h"
#include "lib.h"
#include "types.h"
#include "system_calls.h"
#include "system_calls_asm.h"
#include "paging.h"
#include "i8259.h"
#include "keyboard.h"
#include "syscall_linkage.h"

/* void pit_init(void);
 * Inputs: none
 * Return Value: none
 * Function: sets periodic highest priority interrupt */
void pit_init(void){
// Channel 0 data port (read/write): 0x40
// For the "lobyte/hibyte" mode, 16 bits are always transferred as a pair, with the lowest 8 bits followed by the highest 8 bits

/* bits:   7 6       | 5 4           | 3 2 1                | 0                            */
/* usage:  channel # | access mode   | operating mode       | 4-digit BCD or 16-bit binary */
/* want:   channel 0 | lobyte/hibyte | mode 3 (square wave) | 16-bit binary                */
/*         0 0       | 1 1           | 0 1 1                | 0                            */
    uint8_t command = 0x36;     // 0011 0110
    uint16_t freq = 20; // 1/(50 ms) = 20 Hz

    cli();
    outb(command, 0x43);        //outb(data, port)  /* Writes a byte to a port */
    outb((freq & 0xFF), 0x40);  // write low byte
    outb((freq >> 8), 0x40);    // write high byte
}



/* void pit_handler(hwcontext_t curr_context);
 * Inputs: curr_context (contains saved register values of currently running terminal)
 * Return Value: none
 * Function: handle PIT interrupt by either creating new terminal & switching to it or by switching to existing terminal,
 *           where switching means saving & restoring paging structure, cursor x/ y positions, ESP0, ESP & EBP values */
void pit_handler(hwcontext_t curr_context){
    cli();
    set_terminal_next();

    // // below 2 lines of code disables scheduling so that only processes on currently displaying terminal run;
    // // no screen glitches occur when scheduling is disabled, so the glitches must be scheduling-related
    // // (update: glitches were not scheduling-related; TLB wasn't being flushed when remapping video memory & needs to be flushed every time video memory is remapped)
    // if (terminal_on_display_next != terminal_on_display_curr)
    //     terminal_next = terminal_on_display_next;

    if (terminal_next == terminal_curr){    // if we just booted & haven't used ALT+F2/3 to switch terminal 2/3, don't context switch
        send_eoi(0);
        sti();
        return;
    }
    // save current context
    pcb_t* curr_pcb_ptr = get_pcb_process();
    // curr_pcb_ptr->terminal_save = curr_context;
    // curr_pcb_ptr->terminal_save.ESP = curr_context.EBP - 40;
    if ((terminal_next == terminal_on_display_next) && (terminal_on_display_next != terminal_on_display_curr)){
        screen_x[terminal_on_display_curr] = screen_x[0];   // save current cursor positions to background cursor saves of currently displaying terminal
        screen_y[terminal_on_display_curr] = screen_y[0];
    }

    asm volatile ("             \n\
        movl %%ebp, %0   \n\
        "
        : "=a"(curr_pcb_ptr->schedule_ebp)
        : /*no inputs*/
        : "memory", "cc"
        );

    asm volatile ("             \n\
        movl %%esp, %%ebx   \n\
        movl %%ebx, %0      \n\
        "
        : "=a"(curr_pcb_ptr->schedule_esp)
        : /*no inputs*/
        : "ebx", "memory", "cc"
    ); 


    if (PID[terminal_next] == -1){      // if we're using terminal 2/3 for the 1st time after booting, create new context by running shell
        // prevent new terminal from being created i.e. prevent new shell from running if number of processes running is at maximum of 6
        if (PID[0] >= 5){
            // print error message only if terminal is accepting user input;
            // this is for avoiding printing things to screen when pingpong or counter is running
            if (terminal_read_flag[terminal_on_display_curr-1] == 1){
                if (terminal_curr != terminal_on_display_curr)
                    change_video_phys_addr(0);
                add_newline_if_line_not_empty();
                printf("Has 6 processes running already, can't currently open up another shell\n");
                if (terminal_curr != terminal_on_display_curr)
                    change_video_phys_addr(terminal_curr);
            }
            if (terminal_next == 2){
                terminal_2_active = 0;
            }
            else if (terminal_next == 3){
                terminal_3_active = 0;
            }
            terminal_on_display_next = terminal_on_display_curr;
            terminal_next = terminal_curr;
            send_eoi(0);
            sti();
            return;
        }
        
        save_restore_remap_video_mem();

        if ((terminal_next == terminal_on_display_next) && (terminal_on_display_next != terminal_on_display_curr)){
            screen_x[0] = 0;
            screen_y[0] = 0;
            update_cursor(0, 0);
            terminal_on_display_curr = terminal_on_display_next;
        }
        terminal_curr = terminal_next;
        send_eoi(0);
        //sti();      // would this cause synchronization issues?
        execute((const uint8_t *)"shell");
    }
    else{
        // restore next process's TSS
        pcb_t* next_pcb_ptr = get_pcb_process_next();
        // hwcontext_t next_context = next_pcb_ptr->terminal_save;
   
        tss.ss0 = KERNEL_DS;    // already KERNEL_DS by default
        tss.esp0 = (uint32_t)(get_kernel_stack_bottom_next()); //curr_esp;

        uint32_t next_process_base_phys = SHELL_BASE_PHYS + PID[terminal_next] * TAB_NUM_ADDRS;   // increment physical base address of user program by 4MB
        save_restore_remap_video_mem();
        map_virt_user_to_physical(next_process_base_phys);
        if ((terminal_next == terminal_on_display_next) && (terminal_on_display_next != terminal_on_display_curr)){
            screen_x[0] = screen_x[terminal_on_display_next];   // restore cursor positions of the terminal to be displayed next by
            screen_y[0] = screen_y[terminal_on_display_next];   // copying values in background cursor saves of that terminal to actual cursor positions
            update_cursor(screen_x[0], screen_y[0]);
            terminal_on_display_curr = terminal_on_display_next;
        }
        terminal_curr = terminal_next;
        
        // check priviledge level of context in terminal to be resumed
        // if (next_context.XCS == USER_CS){
        //     next_context.XSS = USER_DS;     // replace SS value w/ DS value to conveniently modify DS reg when context switching
        // }
        // else if (next_context.XCS == KERNEL_CS){
        //     next_context.XSS = KERNEL_DS;
        // }
        asm volatile ("             \n\
            movl %0, %%esp   \n\
            "
            : /*no outputs*/
            : "a" (next_pcb_ptr->schedule_esp)
            : "memory", "cc"
        );

        asm volatile ("             \n\
            movl %0, %%ebp   \n\
            "
            : /*no outputs*/
            : "a" (next_pcb_ptr->schedule_ebp)
            : "ebp", "cc"
        );
        send_eoi(0);
        // if ctrl+c was pressed in the terminal that's scheduled to run next, then halt that terminal
        // instead of returning to where it was
        if (kb_exit_flag[terminal_next-1] == 1){
            kb_exit_flag[terminal_next-1] = 0;
            user_halt((uint32_t)0);
        }
        sti();
        // context_switch(next_context);
        return;

    }
}


/* void set_terminal_next(void);
 * Inputs: none
 * Return Value: none
 * Function: decides which terminal should be scheduled to run next depending on what terminal is currently & which terminals are active */
void set_terminal_next(void){
    switch (terminal_curr)
    {
    case 1:
        if (terminal_2_active)
            terminal_next = 2;
        else if (terminal_3_active)
            terminal_next = 3;
        else
            terminal_next = 1;
        break;
    case 2:
        if (terminal_3_active)
            terminal_next = 3;
        else
            terminal_next = 1;
        break;
    case 3:
        terminal_next = 1;
        break;
    default:
        break;
    }
    return;
}
