#include "terminal.h"
#include "types.h"
#include "keyboard.h"
#include "lib.h"
#include "i8259.h"
#include "system_calls.h"

volatile uint8_t terminal_read_flag[3] = {0, 0, 0};

/* void terminal_init(void);
 * Inputs: none
 * Return Value: none
 * Function: none*/
void terminal_init(void){
    return;
}

/* int32_t terminal_open(const uint8_t* filename);
 * Inputs: none
 * Return Value: none
 * Function: none*/
int32_t terminal_open(const uint8_t* filename){
    return -1;
}

/* void terminal_close(void);
 * Inputs: none
 * Return Value: none
 * Function: none*/
int32_t terminal_close(int32_t fd){
    return -1;
}

/* int32_t terminal_read (int32_t fd, void* buf, int32_t nbytes);
 * Inputs: fd (program number?), buf (buffer to copy keyboard buffer contents to), nbytes (number of bytes of string)
 * Return Value: number of bytes successfully copied to buf, or -1 for failure
 * Function: reads characters from the input device (such as keyboard) and store them into the input buffer 
 *           until a new-line character is read or the input size exceed the buffer size 
 *           (this behavior is implementation dependent but I recommend this way for some reasons), 
 *           then it copies the characters from the input buffer to the given array from user */
int32_t terminal_read (int32_t fd, void* buf, int32_t nbytes){
    enter_flag[terminal_on_display_curr-1] = 0;
    kb_buf_full_flag[terminal_on_display_curr] = 0;
    if ((kb_buf[terminal_on_display_curr-1] == NULL) || (buf == NULL)){
        return -1;
    }
    int32_t nbytes_to_copy;
    nbytes_to_copy = nbytes;
    if (nbytes > KB_BUFFER){
        nbytes_to_copy = 128;
    }
    terminal_read_flag[terminal_curr-1] = 1;    // allow currently running terminal to accept all keyboard inputs
    while((!enter_flag[terminal_on_display_curr-1]) || (terminal_curr != terminal_on_display_curr)){         // waits til ENTER is pressed or keyboard buffer is full to copy to buf
        // disable_irq(0);     // temporarily disable round-robin scheduling to allow I/O to not be interrupted
        // if((terminal_curr != terminal_on_display_curr) || (terminal_on_display_next != terminal_on_display_curr)){
        //     enable_irq(0);
        // }
        if (kb_buf_full_flag[terminal_on_display_curr-1]){
            strncpy_include_null((int8_t *)buf, (const int8_t *)(kb_buf[terminal_on_display_curr-1]), (uint32_t)nbytes_to_copy);
            //copies nbytes of contents in keyboard buffer to argument buf
        }
    }
    terminal_read_flag[terminal_curr-1] = 0;    // only allow currently running terminal to accept terminal switching/ screen clearing/ program exiting commands
                                                // to prevent user from typing things when terminal isn't supposed to read from keyboard
    strncpy_include_null((int8_t *)buf, (const int8_t *)(kb_buf[terminal_on_display_curr-1]), (uint32_t)nbytes_to_copy);
    kb_buf_clear();   // clear keyboard buffer after receiving ENTER
    enter_flag[terminal_on_display_curr-1] = 0;
    return nbytes_to_copy;
}


/* int32_t terminal_write (int32_t fd, const void* buf, int32_t nbytes);
 * Inputs: fd (program number?), buf (string to be written to terminal), nbytes (number of bytes of string)
 * Return Value: number of bytes successfully copied copied to buf, or -1 for failure
 * Function: copies characters from the input string to an output buffer (or not if you don’t want one) and write them to the video memory*/
int32_t terminal_write (int32_t fd, const void* buf, int32_t nbytes){
    if ((nbytes < 0)){
        return -1;
    }
    if (buf == NULL){
        return -1;
    }
    // add_newline_if_line_not_empty();
    // int8_t out_buf[nbytes];
    int i;
    cli();
    // strncpy_include_null((int8_t *)out_buf, (const int8_t *)buf, (uint32_t)nbytes);
    for (i = 0; i < nbytes; i++){
        if (((unsigned char *)buf)[i] != 0){    // don't stop writing at null byte but don't print null byte either
            add_newline_if_right_edge();
            putc(/*out_*/((unsigned char *)buf)[i]);
        }
    }
    sti();
    return nbytes;
}
