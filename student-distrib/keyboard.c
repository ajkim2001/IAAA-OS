#include "i8259.h"
#include "keyboard.h"
#include "handler_setup.h"
#include "x86_desc.h"
#include "handler_call.h"
#include "lib.h"
#include "system_calls.h"
#include "syscall_linkage.h"
#include "paging.h"

#define CAPS_LOCK             0x3A
#define BACKSPACE             0x0E
#define ESCAPE                0x01
#define TAB                   0x0F
#define ENTER                 0x1C
#define LEFT_SHIFT_PRESS      0x2A
#define RIGHT_SHIFT_PRESS     0x36
#define LEFT_SHIFT_RELEASE    0xAA
#define RIGHT_SHIFT_RELEASE   0xB6
#define CTRL_PRESS            0x1D //left and right are the same
#define CTRL_RELEASE          0x9D
#define SPACE                 0x39
#define ALT_PRESS             0x38 //left and right are the same
#define ALT_RELEASE           0xB8 // release scancode = 0x80 + press scancode
#define F1_PRESS              0x3B
#define F1_RELEASE            0xBB
#define F2_PRESS              0x3C
#define F2_RELEASE            0xBC
#define F3_PRESS              0x3D
#define F3_RELEASE            0xBD

unsigned char kb_buf[3][KB_BUFFER+1] = {"", "", ""};
volatile int32_t enter_flag[3] = {0, 0, 0};
volatile int32_t kb_buf_full_flag[3] = {0, 0, 0};
volatile uint8_t kb_exit_flag[3] = {0, 0, 0};

static int kb_buf_num[3] = {0, 0, 0};
// static unsigned int kb_temp =0;
static int caps_flag =0;
static int shift_flag =0;
volatile int ctrl_flag =0;
volatile int l_flag =0;
volatile int c_flag =0;
// static int esc_flag =0;  // escape key not being used for anything
static char c =0;
volatile int alt_flag =0;
volatile int f1_flag =0;
volatile int f2_flag =0;
volatile int f3_flag =0;


//mapping of the characters translated from keyboard data 
char scan_code_translate[SCAN_SET][NUM_KEYS]={
    //lower case
    {
    0,0,'1','2','3','4','5','6','7','8','9','0','-','=', 0,
    0,'q','w','e','r','t','y','u','i','o','p','[',']',0,
    0,'a','s','d','f','g','h','j','k','l',';', '\'',
    '`',0,'\x5c','z','x','c','v','b','n','m',
    ',','.','/'},

    //caps lock
    {
    0,0,'1','2','3','4','5','6','7','8','9','0','-','=', 0,
    0,'Q','W','E','R','T','Y','U','I','O','P','[',']',0,
    0,'A','S','D','F','G','H','J','K','L',';', '\'',
    '`',0,'\x5c','Z','X','C','V','B','N','M',
    ',','.','/'},

    //shift
    {
    0,0,'!','@','#','$','%','^','&','*','(',')','_','+', 0,
    0,'Q','W','E','R','T','Y','U','I','O','P','{','}',0,
    0,'A','S','D','F','G','H','J','K','L',':', '"',
    '~',0,'|','Z','X','C','V','B','N','M',
    '<','>','?'
    }
 
};

/* void keyboard_handler_call()
 * Inputs: None
 * Return Value: none
 * Function: Reads the scan code from the keyoboard data port and uses the mapping
 * to print corresponding key character before finally ending the interrupt*/
void keyboard_handler_call(){
    int key_input= inb(KEYBOARD_PORT_DATA);
    // ALT/ F1/ F2/ F3 on
    if(key_input == ALT_PRESS)
        alt_flag=1;
    if(key_input == ALT_RELEASE)
        alt_flag=0;
    if(key_input == F1_PRESS)
        f1_flag=1;
    if(key_input == F1_RELEASE)
        f1_flag=0;
    if(key_input == F2_PRESS)
        f2_flag=1;
    if(key_input == F2_RELEASE)
        f2_flag=0;
    if(key_input == F3_PRESS)
        f3_flag=1;
    if(key_input == F3_RELEASE)
        f3_flag=0;
    
    if(alt_flag==1){
        if(f1_flag){
            terminal_on_display_next = 1;
        }
        if(f2_flag){
            terminal_on_display_next = 2;
            terminal_2_active = 1;
        }
        if(f3_flag){
            terminal_on_display_next = 3;
            terminal_3_active = 1;
        }
        send_eoi(KEYBOARD_IRQ);
        return;
    }
    
    // let keyboard modifications to video memory show up regardless of if current terminal is being displayed
    change_video_phys_addr(0);

    //ctrl on
    if(key_input == CTRL_PRESS)
        ctrl_flag=1;
    if(key_input == CTRL_RELEASE)
        ctrl_flag=0;
    //get letter l and c
    l_flag = scan_code_translate[0][key_input]=='l' || scan_code_translate[1][key_input]=='L' || scan_code_translate[2][key_input]=='L';
    c_flag = scan_code_translate[0][key_input]=='c' || scan_code_translate[1][key_input]=='C' || scan_code_translate[2][key_input]=='C';
    if(ctrl_flag==1){
        if(l_flag){
            clear_screen();
            update_cursor(0,0);
        }
        if(c_flag){
            send_eoi(KEYBOARD_IRQ);
            kb_buf_num[terminal_on_display_curr-1] = 0;     // clear keyboard buffer after exiting program in currently displaying terminal
            kb_buf_clear();
            // terminal_curr = terminal_on_display_curr;   // instead of trying to use a flag to indicate that we need to exit & potentially make new shell
                                                        // in currently displaying terminal rather than currently running terminal, & then using
                                                        // terminal_on_display_curr tather than terminal_curr to index into arrays & stuff
                                                        // (which so far has been resulting in bootlooping), simply making terminal_curr the same
                                                        // as terminal_on_display_curr is a way easier solution

            // above solution is a shortcut that messes with scheduling;
            // ideally we want to put off halting the terminal that's not currently running until it's scheduled to run next
            uint8_t terminal_on_display_curr_save = terminal_on_display_curr;
            // save the value of terminal_on_display_curr in case it changes before user_halt() is called

            // the commented out code below doesn't work because EIP doesn't point to this code when you return to the currently displaying terminal,
            // so the currently displaying terminal won't halt, and the currently running terminal will be stuck in this loop

            // if (terminal_curr != terminal_on_display_curr_save){
            //     sti();
            //     while (terminal_curr != terminal_on_display_curr_save);
            //     cli();
            // }
            // user_halt((uint32_t)0);

            // if the terminal we want to exit isn't the currently running terminal, simply resume currently running terminal's activities,
            // and when the terminal that we want to exit is scheduled to run next, run user_halt() instead of resuming that terminal's activites
            if (terminal_curr != terminal_on_display_curr_save){
                kb_exit_flag[terminal_on_display_curr_save-1] = 1;
            }else{
                user_halt((uint32_t)0);
            }
        }
        send_eoi(KEYBOARD_IRQ);
        if (terminal_curr != terminal_on_display_curr)  // if current terminal isn't being displayed, then remap virtual address of video memory 
            change_video_phys_addr(terminal_curr);      // to physical address of undisplayed video background page when done handling keyboard
        return;
    }

    // always allow user to change terminal, clear screen, or exit w/ ctrl+c,
    // but only allow user to type something if the currently display terminal is taking user input through terminal_read()
    if (terminal_read_flag[terminal_on_display_curr-1] == 0){
        send_eoi(KEYBOARD_IRQ);
        if (terminal_curr != terminal_on_display_curr)  // if current terminal isn't being displayed, then remap virtual address of video memory 
            change_video_phys_addr(terminal_curr);
        return;
    }

    if ((kb_buf_num[terminal_on_display_curr-1] >= KB_BUFFER) && (key_input != ENTER) && (key_input != BACKSPACE)){
        send_eoi(KEYBOARD_IRQ);
        if (terminal_curr != terminal_on_display_curr)  // if current terminal isn't being displayed, then remap virtual address of video memory 
            change_video_phys_addr(terminal_curr);
        return;
    }

    //caps_on
    if (key_input == CAPS_LOCK)
        caps_flag=1-caps_flag;

    //shift on
    if(key_input==LEFT_SHIFT_PRESS || key_input==RIGHT_SHIFT_PRESS)
        shift_flag=1;  

    //shift off
    if(key_input==LEFT_SHIFT_RELEASE || key_input==RIGHT_SHIFT_RELEASE)
        shift_flag=0;

    //enter key
    if(key_input == ENTER){
        key_enter();
        send_eoi(KEYBOARD_IRQ);
        enter_flag[terminal_on_display_curr-1] = 1;
        kb_buf_num[terminal_on_display_curr-1] = 0;     // note: keyboard buffer gets cleared in terminal_read() after receiving ENTER
        if (terminal_curr != terminal_on_display_curr)
            change_video_phys_addr(terminal_curr);
        return;
    }

    //space key
    if(key_input == SPACE){
        endline();
        putc(' ');     // differentiate SPACE from NULL character
        c=' ';
        kb_buf_add_1_char();
        send_eoi(KEYBOARD_IRQ);
        if (terminal_curr != terminal_on_display_curr)
            change_video_phys_addr(terminal_curr);
        return;
    }

    //tab key
    if(key_input == TAB){
        key_tab();
        kb_buf_add_tab();
        send_eoi(KEYBOARD_IRQ);
        if (terminal_curr != terminal_on_display_curr)
            change_video_phys_addr(terminal_curr);
        return;
    }

    //backspace key
    if(key_input == BACKSPACE){
        if (kb_buf_num[terminal_on_display_curr-1] > 0){    // not allowed to backspace onto characters not typed by user
            key_backspace();
            kb_buf_remove_last_char();
        }
        send_eoi(KEYBOARD_IRQ);
        if (terminal_curr != terminal_on_display_curr)
            change_video_phys_addr(terminal_curr);
        return;
    }

    //escape key
    if(key_input == ESCAPE){
        send_eoi(KEYBOARD_IRQ);
        if (terminal_curr != terminal_on_display_curr)
            change_video_phys_addr(terminal_curr);
        return;
    }

    //index into shift array
    if(shift_flag==1){
        if (key_input<NUM_KEYS){
            if (key_input != 42){   // 42 is the position in array of characters to print for left shift key
                endline();
                putc(scan_code_translate[2][key_input]);
                c=scan_code_translate[2][key_input];
                kb_buf_add_1_char();
            }
        }
        send_eoi(KEYBOARD_IRQ);
        if (terminal_curr != terminal_on_display_curr)
            change_video_phys_addr(terminal_curr);
        return;
    }
    
    //index into caps array
    else if(caps_flag==1){
        if (key_input<NUM_KEYS){
            endline();
            putc(scan_code_translate[1][key_input]);
            c=scan_code_translate[1][key_input];
            kb_buf_add_1_char();
        }
        send_eoi(KEYBOARD_IRQ);
        if (terminal_curr != terminal_on_display_curr)
            change_video_phys_addr(terminal_curr);
        return;
    }

    //index into lower case array
    else{
        if (key_input<NUM_KEYS){
            endline();
            putc(scan_code_translate[0][key_input]);
            c=scan_code_translate[0][key_input];
            kb_buf_add_1_char();
        }
        send_eoi(KEYBOARD_IRQ);
        if (terminal_curr != terminal_on_display_curr)
            change_video_phys_addr(terminal_curr);
        return;
    }
}


/* void kb_buf_add_1_char(void);
 * Inputs: none
 * Return Value: none
 * Function: if keyboard buffer isn't full, add current input from keyboard to buffer; otherwise set buffer full flag;
 *           increment global variable kb_buf_num to keep track of number of characters printed to screen since ENTER was last pressed */
void kb_buf_add_1_char(void){
    if (kb_buf_num[terminal_on_display_curr-1] < KB_BUFFER){
        int buf_index = kb_buf_num[terminal_on_display_curr-1];
        kb_buf[terminal_on_display_curr-1][buf_index] = c;
    }
    else {                  // set flag shared btn keyboard & terminal when keyboard buffer is full
        kb_buf_full_flag[terminal_on_display_curr-1] = 1;
    }
    kb_buf_num[terminal_on_display_curr-1] ++;
}


/* void kb_buf_add_tab(void);
 * Inputs: none
 * Return Value: none
 * Function: while keyboard buffer isn't full, add SPACE to keyboard buffer 4 times; otherwise set buffer full flag;
 *           increment global variable kb_buf_num to keep track of number of characters printed to screen since ENTER was last pressed */
void kb_buf_add_tab(void){
    int i;
    for (i = 0; i < 4; i ++){
        if (kb_buf_num[terminal_on_display_curr-1] < KB_BUFFER){
            int buf_index = kb_buf_num[terminal_on_display_curr-1];
            kb_buf[terminal_on_display_curr-1][buf_index] = ' ';
        }
        else {                  // set flag shared btn keyboard & terminal when keyboard buffer is full
            kb_buf_full_flag[terminal_on_display_curr-1] = 1;
        }
        kb_buf_num[terminal_on_display_curr-1] ++;
    }
}


/* void kb_buf_remove_last_char(void);
 * Inputs: none
 * Return Value: none
 * Function: if kb_buf_num > 0, decrement it to keep track of number of characters printed to screen since ENTER was last pressed;
 *           if keyboard buffer is no longer going to be full, remove kb_buf_num-th character from keyboard and unset full flag */
void kb_buf_remove_last_char(void){
    if (kb_buf_num[terminal_on_display_curr-1] > 0){
        kb_buf_num[terminal_on_display_curr-1] --;
        if (kb_buf_num[terminal_on_display_curr-1] < KB_BUFFER){          // if buffer is no longer full, unset buffer full flag
            int buf_index = kb_buf_num[terminal_on_display_curr-1];
            kb_buf[terminal_on_display_curr-1][buf_index] = 0;
            kb_buf_full_flag[terminal_on_display_curr-1] = 0;
        }
    }
}


/* void kb_buf_clear(void);
 * Inputs: none
 * Return Value: none
 * Function: fill keyboard buffer w/ null bytes and unset full flag */
void kb_buf_clear(void){
    int i;
    for (i = 0; i < KB_BUFFER; i++){
        kb_buf[terminal_on_display_curr-1][i] = 0;
    }
    kb_buf_full_flag[terminal_on_display_curr-1] = 0;
}

