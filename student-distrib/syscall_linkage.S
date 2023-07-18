#include "sysnum.h"

/* 
 * Rather than create a case for each number of arguments, we simplify
 * and use one macro for up to three arguments; the system calls should
 * ignore the other registers, and they're caller-saved anyway.
 */
#define DO_CALL(fxn,number)   \
.GLOBL fxn                   ;\
fxn:   PUSHL	%EBX          ;\
	MOVL	$number,%EAX  ;\
	MOVL	8(%ESP),%EBX  ;\
	MOVL	12(%ESP),%ECX ;\
	MOVL	16(%ESP),%EDX ;\
    PUSHL   %ESI          ;\
    PUSHL   %EDI          ;\
	INT	$0x80         ;\
    POPL    %EDI          ;\
    POPL    %ESI          ;\
	POPL	%EBX          ;\
	RET

/* the system call library wrappers */
DO_CALL(user_halt,SYS_HALT)
DO_CALL(user_execute,SYS_EXECUTE)
DO_CALL(user_read,SYS_READ)
DO_CALL(user_write,SYS_WRITE)
DO_CALL(user_open,SYS_OPEN)
DO_CALL(user_close,SYS_CLOSE)
DO_CALL(user_getargs,SYS_GETARGS)
DO_CALL(user_vidmap,SYS_VIDMAP)
DO_CALL(user_set_handler,SYS_SET_HANDLER)
DO_CALL(user_sigreturn,SYS_SIGRETURN)


; /* Call the main() function, then halt with its return value. */

