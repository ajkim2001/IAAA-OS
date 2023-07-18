.data					# section declaration
    USER_CS = 0x0023
    USER_DS = 0x002B
    SHELL_KERNEL_BOTTOM = 0x7FDFFC


.text

.globl flush_tlb, halt_return, context_switch

flush_tlb:
    pushl   %ebp
    movl    %esp, %ebp
    movl	%cr3,%eax
	movl	%eax,%cr3
    leave
    ret

halt_return:
    addl    $4, %esp    # pop unneeded return address off stack
    popl    %eax        # pop return value into EAX
    popl    %ebp        # pop old EBP value into EBP
    movl    (%esp), %edx    # temporarily store old ESP value in EDX
    addl    $4, %esp    # pop old ESP value off stack
    addl    $36, %esp   # pop unneeded things off stack
    popl    %ebx            # pop pushed registers
    popl    %ecx
#    movl    %edx, 24(%esp)  # put old ESP value into ESP field used by IRET to pop into ESP
    popl    %edx
    addl    $8, %esp   # pop irq number & dummy
    iretl

context_switch:
    addl    $4, %esp    # pop unneeded return address off stack
    popal               # restore registers that were pushed using pushal
    addl    $8, %esp    # pop unneeded IRQ and Error
    # pushl   16(%esp)    # push XSS
    # pushl   16(%esp)    # push ESP
    # pushl   16(%esp)     # push EFLAGS
    # pushl   16(%esp)     # push XCS
    # pushl   16(%esp)     # push EIP
    iretl

.end
