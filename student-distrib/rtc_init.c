#include "rtc_init.h"
#include "lib.h"
#include "i8259.h"
#include "system_calls.h"

volatile unsigned short virtual_rtc_freq[3] = {0, 0, 0};
//volatile unsigned short rtc_flag = 0;
volatile unsigned short rtc_counter[3] = {0, 0, 0};

/* void rtc_ini();
 * Inputs: none
 * Return Value: none
 * Function: Currently sets Periodic Interrupt Enable ON and sets PIE to fastest possible frequency  */
void rtc_init(){
    asm volatile("\n\
    CLI \n\
    "); //disabling normal interrupts

    char curr;
    outb(0x8A,0x70); //disable NMI and set status register to A
    curr  = inb(0x71); //get current val of register A
    outb(0x8A,0x70);//reset status register back to A
    outb((curr&0xF0)|0x0a,0x71);//set frequency to highest stable, possible which is 8192Hz, the 0x03 does this
    // set frequency to 64 Hz

    outb(0x8B,0x70); //writing 0x80 disables NMI, the B is selecting the RTC Status Register A,B or C
    curr = inb(0x71);
    outb(0x8B,0x70); //setting back to register B, as reading sets to D
    outb(curr|0x40,0x71); // setting bit 6 to turn on interrupts
    //need to save since writing to 0x71 and not 0x70

    
    asm volatile("\n\
    STI \n\
    "); //enabling normal interrupts
    outb(0x00,0x70); // unsetting bit 7 to re-enable NMI

}

/* void rtc_handler();
 * Inputs: none
 * Return Value: none
 * Function: Calls the handler, clears the interrupt in the status register C and tells PIC EOI. Also counts interrupts for rtc_read() */
void rtc_handler(){
    //test_interrupts();
    // if(virtual_rtc_freq[terminal_curr-1] != 0){
    //     rtc_counter[terminal_curr-1]++; //counter so that rtc_read knows how many interrupts have passed
    // }

    // increment every terminal's rtc counter each time a rtc interrupt is raised to prevent lagging when more than 1 terminal is active
    if(virtual_rtc_freq[0] != 0)
        rtc_counter[0]++;
    if(virtual_rtc_freq[1] != 0)
        rtc_counter[1]++;
    if(virtual_rtc_freq[2] != 0)
        rtc_counter[2]++;

    outb(0x0C,0x70); //selecting status register C of the RTC
    inb(0x71); //reading interrupt status so that interrupts will continue

    send_eoi(8);

}

/* int32_t rtc_open(const uint8_t* filename);
 * Inputs: filename, does nothing
 * Return Value: 0 on success
 * Function: Sets the virtual rtc frequency to 2Hz */
int32_t rtc_open(const uint8_t* filename){
    virtual_rtc_freq[terminal_curr-1] = 2; //reset to 2Hz
    return 0;
}

/* int32_t rtc_close(int32_t fd);
 * Inputs: fd, does nothing
 * Return Value: 0 on success
 * Function: Sets the virtual rtc frequency to 0Hz */
int32_t rtc_close(int32_t fd){
    virtual_rtc_freq[terminal_curr-1] = 0; //Setting to 0Hz
    return 0;
}

/* int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);
 * Inputs: fd, buf and nbytes do nothing
 * Return Value: 0 on virtual interrupt occurred 
 * Function: Waits til the user specified frequency interrupt occurs and then returns 0 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes){
    if(virtual_rtc_freq[terminal_curr-1] == 0)
        return 0; //ideally should return -1, but following spec listed in docs
    rtc_counter[terminal_curr-1] = 0;
    while(rtc_counter[terminal_curr-1]<(64/virtual_rtc_freq[terminal_curr-1])); //8192 is set frequency of the RTC chip

    return 0;
}

/* int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);
 * Inputs: buf takes a pointer to where a 4 byte frequency value is stored, rest are unused
 * Return Value: 0 on frequency set and -1 on bad frequency value
 * Function: Checks if user specified frequency is a power of 2 and attempts to set it as virtual frequency */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes){
    unsigned short user_freq;
    if(nbytes!=4)
        return -1;
    //memcpy(&user_freq,buf,nbytes); //copies users val in input buffer to local var
    user_freq = *(uint32_t*)buf;
    if(user_freq<2 || user_freq>1024)
        return -1;
    if((user_freq&(user_freq-1))!=0) //checks if power of 2
        return -1;
    virtual_rtc_freq[terminal_curr-1] = user_freq;
    return 0; //0 bytes written to a file 
}
