#ifndef _PIT_INIT_H
#define _PIT_INIT_H

#include "types.h"  // use this for explicitly-sized types (uint32_t, int8_t, etc.) instead of <stdint.h>
#include "handler_setup.h"

void pit_init(void);
void pit_handler(hwcontext_t curr_context);
void set_terminal_next(void);

#endif /* _PIT_INIT_H */
