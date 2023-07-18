#ifndef _HANDLER_SETUP_H
#define _HANDLER_SETUP_H

#include "types.h"

#define num_IDT_entry_used      10
#define except_entry_start      0
#define interrupt_entry_start   32
#define sys_call_entry          0x80
#define rtc_entry               0x28
#define keyboard_entry          0x21
#define pit_entry               0x20

typedef struct {
    uint32_t EDI;
    uint32_t ESI;
    uint32_t EBP;
    uint32_t Temp;
    uint32_t EBX;
    uint32_t EDX;
    uint32_t ECX;
    uint32_t EAX;
    uint32_t IRQ;
    uint32_t Error;
    uint32_t Ret_Addr;  // same as EIP?
    uint32_t XCS;
    uint32_t EFLAGS;
    uint32_t ESP;
    uint32_t XSS;
} hwcontext_t;

void IDT_init(void);

// interrupt handlers
void keyboard_handler(void);
extern void call_rtc_handler(void);
extern void call_pit_handler(void);

// system call related
extern int32_t system_call_check(void);
extern int32_t make_sys_call(uint32_t code);

// exception handler callers
extern void exception_handler_select(hwcontext_t hw_regs);
extern void exception_x0(void);
extern void exception_x1(void);
extern void exception_x2(void);
extern void exception_x3(void);
extern void exception_x4(void);
extern void exception_x5(void);
extern void exception_x6(void);
extern void exception_x7(void);
extern void exception_x8(void);
extern void exception_x9(void);
extern void exception_xA(void);
extern void exception_xB(void);
extern void exception_xC(void);
extern void exception_xD(void);
extern void exception_xE(void);
extern void exception_xF(void);
extern void exception_x10(void);
extern void exception_x11(void);
extern void exception_x12(void);
extern void exception_x13(void);
extern void exception_x14(void);
extern void exception_x15(void);
extern void exception_x16(void);
extern void exception_x17(void);
extern void exception_x18(void);
extern void exception_x19(void);
extern void exception_x1A(void);
extern void exception_x1B(void);
extern void exception_x1C(void);
extern void exception_x1D(void);
extern void exception_x1E(void);
extern void exception_x1F(void);

typedef void (*func)(void);     // make array of functions that have no inputs or outputs
// make array of exception handler callers
static const func exception_array[32] = {
    &exception_x0, 
    &exception_x1, 
    &exception_x2, 
    &exception_x3, 
    &exception_x4, 
    &exception_x5, 
    &exception_x6, 
    &exception_x7, 
    &exception_x8, 
    &exception_x9, 
    &exception_xA,
    &exception_xB, 
    &exception_xC, 
    &exception_xD, 
    &exception_xE, 
    &exception_xF, 
    &exception_x10, 
    &exception_x11, 
    &exception_x12, 
    &exception_x13, 
    &exception_x14,  
    &exception_x15,  
    &exception_x16,  
    &exception_x17,  
    &exception_x18,  
    &exception_x19,  
    &exception_x1A,  
    &exception_x1B,  
    &exception_x1C,  
    &exception_x1D,  
    &exception_x1E,  
    &exception_x1F};


#endif /* _HANDLER_SETUP_H */
