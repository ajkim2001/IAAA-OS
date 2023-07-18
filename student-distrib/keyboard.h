#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "types.h"

#define KEYBOARD_IRQ 1
#define KEYBOARD_PORT_DATA 0x60
#define NUM_KEYS 54
#define SCAN_SET 3
#define KB_BUFFER             128


void keyboard_handler_call(void); 
void keyboard_init(void); 

extern unsigned char kb_buf[3][KB_BUFFER+1];    // each terminal gets its own keyboard buffer & related flags
extern volatile int32_t enter_flag[3];          // that get modified by keyboard handler/ terminal_read only
extern volatile int32_t kb_buf_full_flag[3];    // when that particular terminal is currently on display
extern volatile uint8_t kb_exit_flag[3];

void kb_buf_add_1_char(void);
void kb_buf_add_tab(void);
void kb_buf_remove_last_char(void);
void kb_buf_clear(void);

#endif /* _KEYBOARD_H */

