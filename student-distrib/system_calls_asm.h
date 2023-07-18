#ifndef SYSTEM_CALLS_ASM_H
#define SYSTEM_CALLS_ASM_H

#include "types.h"
#include "handler_setup.h"


/* All calls return >= 0 on success or -1 on failure. */
extern void flush_tlb(void);
extern void context_switch(hwcontext_t next_context);

#endif /* SYSTEM_CALLS_ASM_H */
