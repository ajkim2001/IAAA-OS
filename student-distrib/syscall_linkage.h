#if !defined(SYSCALL_LINKAGE_H)
#define SYSCALL_LINKAGE_H

#include "types.h"



/* All calls return >= 0 on success or -1 on failure. */
extern int32_t user_halt (uint32_t status);  /*  * Note that the system call for halt will have to make sure that only
                                                * the low byte of EBX (the status argument) is returned to the calling
                                                * task.                                                                 */ 
extern int32_t user_execute (const uint8_t* command);/* * Negative returns from execute indicate that the desired program
                                                        * could not be found.                                           */

extern int32_t user_read (int32_t fd, void* buf, int32_t nbytes);
extern int32_t user_write (int32_t fd, const void* buf, int32_t nbytes);
extern int32_t user_open (const uint8_t* filename);
extern int32_t user_close (int32_t fd);
extern int32_t user_getargs (uint8_t* buf, int32_t nbytes);
extern int32_t user_vidmap (uint8_t** screen_start);
extern int32_t user_set_handler (int32_t signum, void* handler_address); // extra credit
extern int32_t user_sigreturn (void);    // also extra credit

enum signums {
	DIV_ZERO = 0,
	SEGFAULT,
	INTERRUPT,
	ALARM,
	USER1,
	NUM_SIGNALS
};

#endif /* SYSCALL_LINKAGE_H */

