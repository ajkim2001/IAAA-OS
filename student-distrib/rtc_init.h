#include "lib.h"
//Function primitives for rtc_init.c

//Sets up rtc interrupt and frequency
void rtc_init(void);
//Calls the rtc specfic handler
extern void rtc_handler(void);
//resets rtc frequency down to 2hz
int32_t rtc_open(const uint8_t* filename);
//atm does nothing 
int32_t rtc_close(int32_t fd);
//waits til handler sends signal to return. 
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);
//Checks user given value and attempts to set it as the rtc frequency
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);
