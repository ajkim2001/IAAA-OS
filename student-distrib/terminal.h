#include "types.h"

void terminal_init(void);
int32_t terminal_open(const uint8_t* filename);
int32_t terminal_close(int32_t fd);
int32_t terminal_read (int32_t fd, void* buf, int32_t nbytes);
int32_t terminal_write (int32_t fd, const void* buf, int32_t nbytes);

extern volatile uint8_t terminal_read_flag[3];  // terminal_read_flag[j-1] = 1 if terminal j is currently running terminal_read(), and 0 otherwise;
                                                // used to prevent terminal from accepting keyboard inputs (besides alt+f, ctrl+l, & ctrl+c because
                                                // you still need to be able to switch displaying terminal, clearn screen, & exit terminal)
                                                // when it's not supposed to accept keyboard inputs
