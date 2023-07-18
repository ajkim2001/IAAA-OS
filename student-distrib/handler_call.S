.data					# section declaration
    DUMMY = 0xFF

syscall_table:
        .long   invalid_eax, halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn
        
.text

.globl exception_x0, exception_x1, exception_x2, exception_x3, exception_x4, exception_x5, exception_x6, exception_x7
.globl exception_x8, exception_x9, exception_xA, exception_xB, exception_xC, exception_xD, exception_xE, exception_xF
.globl exception_x10, exception_x11, exception_x12, exception_x13, exception_x14, exception_x15, exception_x16, exception_x17
.globl exception_x18, exception_x19, exception_x1A, exception_x1B, exception_x1C, exception_x1D, exception_x1E, exception_x1F

.globl system_call_check, make_sys_call
.globl keyboard_handler, call_rtc_handler, call_pit_handler



/* void exception_x#(void);
 * Inputs: none
 * Return Value: none
 * Function: pushes dummy if no error code, & otherwise doesn't push dummy;
 * pushes irq number, then jump to common exception*/

exception_x0:   # Division Error
    pushl $DUMMY
    pushl $0x0
    jmp common_exception

exception_x1:   # Debug
    pushl $DUMMY
    pushl $0x1
    jmp common_exception

exception_x2:   # Non-maskable Interrupt
    pushl $DUMMY
    pushl $0x2
    jmp common_exception

exception_x3:   # Breakpoint
    pushl $DUMMY
    pushl $0x3
    jmp common_exception

exception_x4:   # Overflow
    pushl $DUMMY
    pushl $0x4
    jmp common_exception

exception_x5:   # Bound Range Exceeded
    pushl $DUMMY
    pushl $0x5
    jmp common_exception

exception_x6:   # Invalid Opcode
    pushl $DUMMY
    pushl $0x6
    jmp common_exception

exception_x7:   # Device Not Available
    pushl $DUMMY
    pushl $0x7
    jmp common_exception

exception_x8:   # Double Fault
    pushl $0x8
    jmp common_exception

exception_x9:   # Coprocessor Segment Overrun
    pushl $DUMMY
    pushl $0x9
    jmp common_exception

exception_xA:   # Invalid TSS
    pushl $0xA
    jmp common_exception

exception_xB:   # Segment Not Present
    pushl $0xB
    jmp common_exception

exception_xC:   # Stack-Segment Fault
    pushl $0xC
    jmp common_exception

exception_xD:   # General Protection Fault
    pushl $0xD
    jmp common_exception


exception_xE:   # Page Fault
    pushl $0xE
    jmp common_exception

exception_xF:   # Reserved
    pushl $DUMMY
    pushl $0xF
    jmp common_exception

exception_x10:   # x87 Floating-Point Exception
    pushl $DUMMY
    pushl $0x10
    jmp common_exception

exception_x11:   # Alignment Check
    pushl $0x11
    jmp common_exception

exception_x12:   # Machine Check
    pushl $DUMMY
    pushl $0x12
    jmp common_exception

exception_x13:   # SIMD Floating-Point Exception
    pushl $DUMMY
    pushl $0x13
    jmp common_exception

exception_x14:   # Virtualization Exception
    pushl $DUMMY
    pushl $0x14
    jmp common_exception

exception_x15:   # Control Protection Exception
    pushl $DUMMY
    pushl $0x15
    jmp common_exception

exception_x16:   # Reserved
    pushl $DUMMY
    pushl $0x16
    jmp common_exception

exception_x17:   # Reserved
    pushl $DUMMY
    pushl $0x17
    jmp common_exception

exception_x18:   # Reserved
    pushl $DUMMY
    pushl $0x18
    jmp common_exception

exception_x19:   # Reserved
    pushl $DUMMY
    pushl $0x19
    jmp common_exception

exception_x1A:   # Reserved
    pushl $DUMMY
    pushl $0x1A
    jmp common_exception

exception_x1B:   # Reserved
    pushl $DUMMY
    pushl $0x1B
    jmp common_exception

exception_x1C:   # Hypervisor Injection Exception
    pushl $DUMMY
    pushl $0x1C
    jmp common_exception

exception_x1D:   # VMM Communication Exception
    pushl $DUMMY
    pushl $0x1D
    jmp common_exception

exception_x1E:   # Security Exception
    pushl $DUMMY
    pushl $0x1E
    jmp common_exception

exception_x1F:   # Reserved
    pushl $DUMMY
    pushl $0x1F
    jmp common_exception



/* void common_exception(void);
 * Inputs: none
 * Return Value: none
 * Function: callee save registers, call exception handler selecting fxn,
 * & return to instruction where exception was called
 */
common_exception:
    pushal          # Push EAX, ECX, EDX, EBX, original ESP, EBP, ESI, and lastly EDI
    call exception_handler_select
    popal
    addl $8, %esp   # increment stack ptr
    iretl

/* keyboard_handler;
 * Inputs: none
 * Return Value: none
 * Function: callee save registers then calls the handler
 * & return to instruction where exception was called
 */
keyboard_handler:
    pushl $DUMMY
    pushl $0x21
    pushal
    call keyboard_handler_call
    popal
    addl $8, %esp   # increment stack ptr
    iretl


/* call_rtc_handler;
 * Inputs: none
 * Return Value: none
 * Function: callee save registers then calls the handler
 * & return to instruction where exception was called
 */
call_rtc_handler:
    pushl $DUMMY
    pushl $0x28
    pushal          # Push EAX, ECX, EDX, EBX, original ESP, EBP, ESI, and lastly EDI
    call rtc_handler
    popal
    addl $8, %esp   # increment stack ptr
    iretl


/* call_pit_handler;
 * Inputs: none
 * Return Value: none
 * Function: callee save registers then calls the handler
 * & return to instruction where exception was called
 */
call_pit_handler:
    pushl $DUMMY
    pushl $0x20
    pushal          # Push EAX, ECX, EDX, EBX, original ESP, EBP, ESI, and lastly EDI
    call pit_handler
    popal
    addl $8, %esp   # increment stack ptr
    iretl


/* int32_t make_sys_call(uint32_t code);
 * Inputs: code - to be passed to system call to trigger particular system call handler
 * Return Value: value returned by system call; generally 0 for success, -1 for failure
 * Function: put code in EAX then make system call
 */
make_sys_call:
    movl    4(%esp), %eax
    int     $0x80
    ret


/* int32_t system_call_check(void);
 * Inputs: none
 * Return Value: value returned by system call; generally 0 for success, -1 for failure
 * Function: checks if code passed in EAX is in valid range of [1, 10];
 * callee save registers, push arguments passed in EBX, ECX, & EDX;
 * if not valid, print message saying so & return -1; if valid, call 
 * appropriate handler whose address is stored in system call table;
 * afterwards return to instruction where system call was called
 */
system_call_check:
    pushl   $DUMMY
    pushl   $0x80

    pushl   %edx    # push 3rd argument from EDX in case it's needed
    pushl   %ecx    # push 2nd argument from ECX in case it's needed
    pushl   %ebx    # push 1st argument from EBX in case it's needed

    cmpl    $1, %eax        # check if code passed in EAX is valid
    jl      invalid_eax     # if not, print error message
    cmpl    $10, %eax
    jg      invalid_eax

    call    *syscall_table( , %eax, 4)  # make system call if code is valid
    jmp     system_call_back            # jump to stack teardown

invalid_eax:
    call    print_invalid_sys_call_msg  # print message saying that code for system call is invalid
    movl    $-1, %eax       # return -1 to indicate failure since code is invalid

system_call_back:
    popl    %ebx    # pop pushed registers
    popl    %ecx
    popl    %edx

    addl    $8, %esp   # pop irq number & dummy
    iretl

    
.end
