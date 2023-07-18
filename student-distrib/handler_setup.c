#include "handler_setup.h"
#include "x86_desc.h"
#include "handler_call.h"
#include "lib.h"
#include "rtc_init.h"
#include "types.h"
#include "syscall_linkage.h"

void exception_handler_generic(hwcontext_t hw_regs);
void exception_handler_xE(hwcontext_t hw_regs);


/* void IDT_init(void);
 * Inputs: none
 * Return Value: none
 * Function: initializes IDT w/, exception handlers, system call function, & unused entries */
void IDT_init(void){
    uint32_t i;
    
    for (i = 0; i < NUM_VEC; i++){
        idt[i].seg_selector = KERNEL_CS;
        idt[i].reserved4 = 0;
        idt[i].reserved3 = 0;
        idt[i].reserved2 = 1;       // Need to set reserved2 & reserved1 to 1 or you get some sort of memory access error!
        idt[i].reserved1 = 1;
        idt[i].size = 1;            // set to 1 to indicate 32-bit interrupt gate
        idt[i].reserved0 = 0;
        idt[i].dpl = 0;             // descriptor priviledge level; 0 for hardware interrupt & exception descriptors, 3 for system call descriptor
        idt[i].present = 0;         // initialize to not present

        // initialize entries 0x0-0x1F w/ exception handlers
        if (i < interrupt_entry_start){
            SET_IDT_ENTRY(idt[i], exception_array[i]);
            idt[i].present = 1;
        }

        // initialize entry 0x80 w/ system call fxn
        else if (i == sys_call_entry){
            SET_IDT_ENTRY(idt[i], system_call_check);
            // (set to 0 for now, CHANGE IT BACK LATER!!!)
            idt[i].dpl = 0;//3;             // descriptor priviledge level; 3 for system call descriptor
            idt[i].present = 1;
        }

        else if (i == rtc_entry){
            SET_IDT_ENTRY(idt[i], call_rtc_handler);
            idt[i].present = 1;
        }
        else if(i == keyboard_entry){
            SET_IDT_ENTRY(idt[i], keyboard_handler);
            idt[i].present = 1;
        }
        else if(i == pit_entry){
            SET_IDT_ENTRY(idt[i], call_pit_handler);
            idt[i].present = 1;
        }
        // else{
        //     SET_IDT_ENTRY(idt[i], exception_array[0]);
        //     idt[i].present = 1;
        // }
    }

    lidt(idt_desc_ptr);
}


/* void exception_handler_select(hwcontext_t hw_regs);
 * Inputs: hw_regs which contains values saved onto stack, including registers, flags, irq number, error code, & return address
 * Return Value: none
 * Function: selects the appropriate specific exception handler based on irq # */
void exception_handler_select(hwcontext_t hw_regs){
    uint32_t irq_num = hw_regs.IRQ;

    switch (irq_num)
    {
    case 0xE:
        exception_handler_xE(hw_regs);
        break;
    
    default:
        if ((irq_num >= 0) || (irq_num < 32)){      // don't do anything if exception irq is invalid
            exception_handler_generic(hw_regs);
        }
        break;
    }
    return;
}


/* void exception_handler_generic(hwcontext_t hw_regs);
 * Inputs: hw_regs which contains values saved onto stack, including registers, flags, irq number, error code, & return address
 * Return Value: none
 * Function: prints exception message stating irq # & error code onto screen */
void exception_handler_generic(hwcontext_t hw_regs){
    uint32_t irq_num = hw_regs.IRQ;
    printf("encountered exception 0x%#x\n", irq_num);
    printf("error code = 0x%#x\n", hw_regs.Error);
    user_halt((uint32_t)256);       // return 256 to indicate that program was terminated after encountering exception
    while(1);
    return;
}


/* void exception_handler_xE(hwcontext_t hw_regs);
 * Inputs: hw_regs which contains values saved onto stack, including registers, flags, irq number, error code, & return address
 * Return Value: none
 * Function: prints exception message stating irq # & page-fault-causing address onto screen */
// Page Fault
void exception_handler_xE(hwcontext_t hw_regs){
    uint32_t irq_num = hw_regs.IRQ;
    uint32_t page_fault_addr;

    // Note: when page fault occurs, value of CR2 is set to virtual address that caused the page fault
    // move value in CR2 into page_fault_addr to print it later in error message
    asm volatile ("             \n\
            movl %%cr2, %%edx   \n\
            movl %%edx, %0      \n\
            "
            : "=a"(page_fault_addr)
            : /*no inputs*/
            : "edx", "memory", "cc"
    );
    printf("encountered exception #%d: Page Fault\n", irq_num);
    printf("virtual address that caused the Page Fault: 0x%#x\n", page_fault_addr);
    user_halt((uint32_t)256);       // return 256 to indicate that program was terminated after encountering exception
    while(1);
    return;
}
