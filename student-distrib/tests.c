#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "paging.h"
#include "handler_setup.h"
#include "handler_call.h"
#include "types.h"
#include "keyboard.h"
#include "terminal.h"
#include "rtc_init.h"
#include "fileSystem.h"
#include "syscall_linkage.h"


#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if (((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)) ||
			(idt[i].present != 1)){
			assertion_failure();
			result = FAIL;
		}
	}

	// make sure that RTC entry in IDT has right values
	i = rtc_entry;
	if ((idt[i].present != 1) ||
		((((uint32_t)idt[i].offset_31_16) << 16) | ((uint32_t)idt[i].offset_15_00))
		 != (uint32_t)(&call_rtc_handler))
	{
		assertion_failure();
		result = FAIL;
	}

	// make sure that keyboard entry in IDT has right values
	i = keyboard_entry;
	if ((idt[i].present != 1) ||
		((((uint32_t)idt[i].offset_31_16) << 16) | ((uint32_t)idt[i].offset_15_00))
		 != (uint32_t)(&keyboard_handler))
	{
		assertion_failure();
		result = FAIL;
	}

	// make sure that system call entry in IDT has right values
	i = sys_call_entry;
	if ((idt[i].present != 1) ||
		((((uint32_t)idt[i].offset_31_16) << 16) | ((uint32_t)idt[i].offset_15_00))
		 != (uint32_t)(&system_call_check))
	{
		assertion_failure();
		result = FAIL;
	}

	return result;
}

// add more tests here


/* paging table & directory test
 * 
 * Asserts that first 10 PT entries are not present unless it's video memory entry, 
 * that video memory entry in PT has correct settings, that PT entry & kernel entry
 * in PD have correct settings.
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: paging
 * Files: paging.h/c
 */
int pages_test(){
	TEST_HEADER;

	int i;
	int result = PASS;

	int video_entry_in_PT, PT_entry_in_PD, kernel_entry_in_PD;
	video_entry_in_PT = VIDEO_BASE_VIRT/ PAGE_NUM_ADDRS;
	PT_entry_in_PD = 0;
	kernel_entry_in_PD = 1;

	// unused entries in both paging table and directory must be marked not present
	for (i = 0; i < 10; i ++){
		if (i != video_entry_in_PT){
			if (PAGE_TAB[i].P != 0){
				assertion_failure();
				result = FAIL;
			}
		}
		if ((i != PT_entry_in_PD) && (i != kernel_entry_in_PD)){
			if (PAGE_DIR[i].P != 0){
				assertion_failure();
				result = FAIL;
			}
		}
	}

	// make sure that video entry in PT has right values
	i = video_entry_in_PT;
	if((PAGE_TAB[i].P != 1) || (PAGE_TAB[i].PCD != 0) || (PAGE_TAB[i].ADDR != (uint32_t)VIDEO_BASE_PHYS >> 12)){
		assertion_failure();
		result = FAIL;
	}

	// make sure that PT entry in PD has right values
	i = PT_entry_in_PD;
	if((PAGE_DIR[i].P != 1) || (PAGE_DIR[i].PCD != 0) || (PAGE_DIR[i].PS__PAT != 0) ||
	(PAGE_DIR[i].ADDR != ((uint32_t)(PAGE_TAB) >> 12))){
		assertion_failure();
		result = FAIL;
	}

	// make sure that kernel entry in PD has right values
	i = kernel_entry_in_PD;
	if((PAGE_DIR[i].P != 1) || (PAGE_DIR[i].U_S != 0) || (PAGE_DIR[i].PS__PAT != 1) || (PAGE_DIR[i].AVL__G != 1) ||
	(PAGE_DIR[i].ADDR != (uint32_t)((KERNEL_BASE >> 12) & 0xFFC00))){
		assertion_failure();
		result = FAIL;
	}

	return result;
}


/* system call test
 * 
 * make system calls w/ both invalid codes & with valid codes;
 * should print error message if code is invalid,
 * & print message of specified system call if code is valid
 * Inputs: None
 * Outputs: none
 * Side Effects: None
 * Coverage: system calls
 * Files: handler_call.h/S, handler_setup.h/c
 */
int test_sys_call(void){
	TEST_HEADER;

	int i;
	int result = PASS;
	int sys_call_ret_val;
	for (i = -2; i <= 13; i ++){
		sys_call_ret_val = make_sys_call(i);
		if ((i >= 1) && (i <= 10)){		// case for when code is in valid range of [1,10]
			if (sys_call_ret_val != 0){	// as of now valid system calls should only return 0
				assertion_failure();
				result = FAIL;
			}
		}
		else{	// case for when code is invalid
			if (sys_call_ret_val != -1){	// invalid system calls should only return -1
				assertion_failure();
				result = FAIL;
			}
		}
	}

	return result;
}


/* exception 0 test
 * 
 * triggers exception 0 & its handler
 * Inputs: None
 * Outputs: none
 * Side Effects: None
 * Coverage: exception calls
 * Files: handler_call.h/S, handler_setup.h/c
 */
// void test_div_by_0(void){
//     int i = 1/0;
// 	i++;
//     return;
// }


/* page fault test
 * 
 * test paging by trying to trigger page fault & its handler
 * Inputs: addr - address to be dereferenced and if invalid should trigger page fault
 * Outputs: none
 * Side Effects: None
 * Coverage: paging, exception calls
 * Files: paging.h/c, handler_call.h/S, handler_setup.h/c
 */
void test_page_fault(uint32_t addr){
	printf("testing page fault for address at 0x%#x\n", addr);
	uint32_t *page_fault_addr = (uint32_t *)addr;
	uint32_t page_fault_data = *page_fault_addr;	// dereference bad pointer to cause page fault
	page_fault_data++;
	return;
}

/* rtc test
 * 
 * test rtc by clearing screen then enabling rtc interrupts on slave PIC
 * Inputs: none
 * Outputs: none
 * Side Effects: None
 * Coverage: rtc, interrupts, paging
 * Files: paging.h/c, handler_call.h/S, handler_setup.h/c, rtc_init.c/h
 */
void test_rtc(void){
	clear();
	enable_irq(8);
}


/* Checkpoint 2 tests */
void test_virtual_rtc(){
	clear();
	enable_irq(8);
	unsigned short frequency;
	rtc_open(NULL);
	frequency = 2;
	rtc_write(0,&frequency,4);
	int i;
	for(i=0;i<10;i++){
		rtc_read(0,NULL,0);
		test_interrupts();
	}
	frequency = 4;
	rtc_write(0,&frequency,4);
	for(i=0;i<20;i++){
		rtc_read(0,NULL,0);
		test_interrupts();
	}
	frequency = 8;
	rtc_write(0,&frequency,4);
	for(i=0;i<30;i++){
		rtc_read(0,NULL,0);
		test_interrupts();
	}
	frequency = 16;
	rtc_write(0,&frequency,4);
	for(i=0;i<40;i++){
		rtc_read(0,NULL,0);
		test_interrupts();
	}
	frequency = 32;
	rtc_write(0,&frequency,4);
	for(i=0;i<50;i++){
		rtc_read(0,NULL,0);
		test_interrupts();
	}
	frequency = 64;
	rtc_write(0,&frequency,4);
	for(i=0;i<60;i++){
		rtc_read(0,NULL,0);
		test_interrupts();
	}
	frequency = 128;
	rtc_write(0,&frequency,4);
	for(i=0;i<70;i++){
		rtc_read(0,NULL,0);
		test_interrupts();
	}
	frequency = 256;
	rtc_write(0,&frequency,4);
	for(i=0;i<80;i++){
		rtc_read(0,NULL,0);
		test_interrupts();
	}
	frequency = 512;
	rtc_write(0,&frequency,4);
	for(i=0;i<90;i++){
		rtc_read(0,NULL,0);
		test_interrupts();
	}
	frequency = 1024;
	rtc_write(0,&frequency,4);
	for(i=0;i<100;i++){
		rtc_read(0,NULL,0);
		test_interrupts();
	}
	clear();
	rtc_open(NULL);
	while(1){
		rtc_read(0,NULL,0);
		printf("zoom\n");
	}

}


void test_file(){
	char buf1[] = "ls";
	char buf[102400];
	memcpy(buf,buf1,sizeof(buf1));
	if(file_read(0,buf,sizeof(buf)) == 0){
		printf("eof\n\n");
	}
	printf("%#x %#x %#x %#x",buf[27],buf[26],buf[25],buf[24]);
}
/* terminal test
 * 
 * test terminal by repeatedly calling terminal_read and terminal_write
 * Inputs: none
 * Outputs: none
 * Side Effects: None
 * Coverage: terminal, interrupts, keyboard
 * Files: terminal.h/c, handler_call.h/S, handler_setup.h/c, keyboard.h/c, lib.h/c
 */
void test_terminal(void){
	while(1){
		unsigned char out_buf[KB_BUFFER];
		terminal_read(0, out_buf, KB_BUFFER);
		terminal_write(0, out_buf, KB_BUFFER);
		add_newline_if_line_not_empty();
	}
}

void test_generic_syscalls(void){
	unsigned char buf[10] = "bruh";
	user_read (8, (void*)buf, 10);
	user_write (9, (const void*)buf, 11);
	user_open ((const uint8_t*) buf);
	user_close (52);
	user_getargs ((uint8_t*) buf, 23);
}

/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	// TEST_OUTPUT("idt_test", idt_test());
	// // launch your tests here

	// TEST_OUTPUT("pages_test", pages_test());
	// TEST_OUTPUT("test_sys_call", test_sys_call());

	// // sending invalid irqs to PICs should do nothing
	// enable_irq(-1);
	// enable_irq(39);
	// disable_irq(-1);
	// disable_irq(39);
	// send_eoi(-1);
	// send_eoi(39);

    // /*no page fault should happen when referencing page for video memory*/
	// test_page_fault(VIDEO_BASE_VIRT);   // 0xB8000

    // /*no page fault should happen when referencing page for kernel*/
	// test_page_fault(KERNEL_BASE);       // 0x400000
	// clear();

    /*page fault should happen when referencing any page before video memory*/
    // test_page_fault(0x0000);				// dereference null pointer
    // test_page_fault(VIDEO_BASE_VIRT - 1);	// left edge of video memory

    /*page fault should happen when referencing any page between video memory & kernel*/
    // test_page_fault(VIDEO_BASE_VIRT + PAGE_NUM_ADDRS);	// right edge of video memory
    // test_page_fault(KERNEL_BASE - 1);					// left edge of kernel

    /*page fault should happen when referencing any page after kernel*/
    // test_page_fault(KERNEL_BASE + TAB_NUM_ADDRS);		// right edge of kernel


	// /*test exception handling*/
	// asm volatile("int $0x10");	// try raising random exceptions

	// test_rtc();
	// while(1){
	// 	unsigned char out_buf[KB_BUFFER];
	// 	terminal_read(0, out_buf, KB_BUFFER);
	// 	terminal_write(0, out_buf, KB_BUFFER);
	// }
	//test_virtual_rtc();
	// test_file();
	//printf("\n\n");
	//dir_read(0,NULL,0);
	//test_generic_syscalls();
	//test_terminal();
	//test_virtual_rtc();
	// make_sys_call(2);
	user_execute ((const uint8_t *)"shell");
}
