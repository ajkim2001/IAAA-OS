#include "paging.h"
#include "lib.h"
#include "system_calls.h"

// set up paging directory
// 1 4 MiB page for kernel (no table involved)
// 1 paging table for video memory, each page 4 KiB big (how many pages are needed?)

// declare page directory & page table arrays & align them on 4 kB boundaries
struct page_entry_t PAGE_TAB[PAGE_ARRAY_SIZE] __attribute__((aligned (PAGE_NUM_ADDRS)));
struct page_entry_t PAGE_DIR[PAGE_ARRAY_SIZE] __attribute__((aligned (PAGE_NUM_ADDRS)));
struct page_entry_t VID_PAGE_TAB[PAGE_ARRAY_SIZE] __attribute__((aligned(PAGE_NUM_ADDRS))); // vidmap paging table


/* void paging_table_init();
 * Inputs: none
 * Return Value: none
 * Function: initialize paging table w/ video memory page & unused pages */
void paging_table_init(void){
    uint32_t curr_base = 0;
    int i;
    for (i = 0; i < PAGE_ARRAY_SIZE; i ++){
        // default settings for unused paging table entries
        PAGE_TAB[i].P = 0;              // initialize to not present
        PAGE_TAB[i].R_W = 1;            // Read/ Write by default
        PAGE_TAB[i].U_S = 1;            // initialize to user mode
        PAGE_TAB[i].PWT = 0;            // writeback by default
        PAGE_TAB[i].PCD = 1;            // initialize cache to be disabled
        PAGE_TAB[i].A = 0;              // Accessed bit not used so set to 0 by default
        PAGE_TAB[i].AVL__D = 0;         // Available/ Dirty bit not used so set to 0 by default
        PAGE_TAB[i].PS__PAT = 0;        // initialize Page Size/ Page Atribute Table bit to 0
        PAGE_TAB[i].AVL__G = 0;         // initialize Available/ Global bit to 0
        PAGE_TAB[i].AVL = 0;            // Available bits not used so set to 0 default
        PAGE_TAB[i].ADDR = 0;           // bits 31-22 of base addr.; initialized to 0

        // 1 4-kB page for video memory at beginning of virtual video base addr.
        if (curr_base == VIDEO_BASE_VIRT){
            PAGE_TAB[i].P = 1;          // is present
            PAGE_TAB[i].U_S = 0;        // video memory pages generally should be set to supervisor mode;
                                        // NOTE: Needs to be changed to user mode when user maps video memory w/ vidmap system call!!
            PAGE_TAB[i].PCD = 0;        // enable cache for video memory since it's memory-mapped I/O
            PAGE_TAB[i].ADDR = (uint32_t)VIDEO_BASE_PHYS >> 12;      // bits 31-12 of physical video base addr. = 000B8
        }

        // 1 4-kB page for undisplayed video memory for terminal 1
        else if (curr_base == VIDEO_BACKGROUND_1){
            PAGE_TAB[i].P = 1;          // is present
            PAGE_TAB[i].U_S = 0;        // video memory pages generally should be set to supervisor mode;
            PAGE_TAB[i].PCD = 0;        // enable cache for video memory since it's memory-mapped I/O
            PAGE_TAB[i].ADDR = (uint32_t)VIDEO_BACKGROUND_1 >> 12;      // bits 31-12 of physical video background 1 base addr.
        }
        
        // 1 4-kB page for undisplayed video memory for terminal 2
        else if (curr_base == VIDEO_BACKGROUND_2){
            PAGE_TAB[i].P = 1;          // is present
            PAGE_TAB[i].U_S = 0;        // video memory pages generally should be set to supervisor mode;
            PAGE_TAB[i].PCD = 0;        // enable cache for video memory since it's memory-mapped I/O
            PAGE_TAB[i].ADDR = (uint32_t)VIDEO_BACKGROUND_2 >> 12;      // bits 31-12 of physical video background 2 base addr.
        }
        
        // 1 4-kB page for undisplayed video memory for terminal 3
        else if (curr_base == VIDEO_BACKGROUND_3){
            PAGE_TAB[i].P = 1;          // is present
            PAGE_TAB[i].U_S = 0;        // video memory pages generally should be set to supervisor mode;
            PAGE_TAB[i].PCD = 0;        // enable cache for video memory since it's memory-mapped I/O
            PAGE_TAB[i].ADDR = (uint32_t)VIDEO_BACKGROUND_3 >> 12;      // bits 31-12 of physical video background 3 base addr.
        }

        curr_base += PAGE_NUM_ADDRS;    // increment current base address by 4 kB addresses
    }
}



/* void paging_directory_init();
 * Inputs: none
 * Return Value: none
 * Function: initialize paging directory w/ paging table, kernel page, & unused entries */
void paging_directory_init(void){
    uint32_t curr_base = 0;
    int i;
    for (i = 0; i < PAGE_ARRAY_SIZE; i ++){
        // default settings for unused paging directory entries
        PAGE_DIR[i].P = 0;              // initialize to not present
        PAGE_DIR[i].R_W = 1;            // Read/ Write by default
        PAGE_DIR[i].U_S = 1;            // initialize to user mode
        PAGE_DIR[i].PWT = 0;            // writeback by default
        PAGE_DIR[i].PCD = 1;            // initialize cache to be disabled
        PAGE_DIR[i].A = 0;              // Accessed bit not used so set to 0 by default
        PAGE_DIR[i].AVL__D = 0;         // Available/ Dirty bit not used so set to 0 by default
        PAGE_DIR[i].PS__PAT = 0;        // initialize Page Size/ Page Atribute Table bit to 0
        PAGE_DIR[i].AVL__G = 0;         // initialize Available/ Global bit to 0
        PAGE_DIR[i].AVL = 0;            // Available bits not used so set to 0 default
        PAGE_DIR[i].ADDR = 0;           // bits 31-22 of base addr.; initialized to 0

        // first entry in paging directory should be paging table containing video memory page
        if (curr_base == 0){
            PAGE_DIR[i].U_S = 0;        // generally should be set to supervisor mode;
                                        // NOTE: Needs to be changed to user mode when user maps video memory w/ vidmap system call!!
            PAGE_DIR[i].PCD = 0;        // enable cache for paging table since it contains page for video memory?
            PAGE_DIR[i].PS__PAT = 0;    // set PS to 0 to point to paging table
            PAGE_DIR[i].ADDR = ((uint32_t)(PAGE_TAB) >> 12);        // bits 31-22 of paging table base addr.
            PAGE_DIR[i].P = 1;          // is present
        }

        // second entry in paging directory should contain kernel page
        else if (curr_base == KERNEL_BASE){
            PAGE_DIR[i].U_S = 0;        // can only be accessed in supervisor mode
            PAGE_DIR[i].PS__PAT = 1;    // set PS to 1 to point to 4 MB page
            PAGE_DIR[i].AVL__G = 1;     // global page
            PAGE_DIR[i].ADDR = (uint32_t)((KERNEL_BASE >> 12) & 0xFFC00);
            PAGE_DIR[i].P = 1;          // is present
            // bits 31-22 of kernal base addr. = 0x001 and 10 reserved bits set to 0
        }

        curr_base += TAB_NUM_ADDRS;     // increment current base address by 1024 * 4 kB addresses
    }
}



/* void map_virt_user_to_physical(uint32_t physical_address);
 * Inputs: physical address to map virtual address of user page to
 * Return Value: none
 * Function: statically allocate & initialize user page; if input physical address isn't valid for user page
             or if user page entry already exists, print error message & return */
void map_virt_user_to_physical(uint32_t physical_address){
    // check if input is valid
    if (((physical_address & 0xFFFFF) != 0) || (physical_address < SHELL_BASE_PHYS) || (physical_address > USER_PROG_6_BASE_PHYS)){
        printf("invalid physical address (not 4MB aligned page between 8MB and 16 MB) for user page\n");
        return;
    }
    uint32_t i = USER_BASE_VIRT/ TAB_NUM_ADDRS;     // 128 MB/ 4MB
    // check if entry for user page is already present in page directory
    // if (PAGE_DIR[i].P == 1){
    //     uint32_t curr_phys_addr = PAGE_DIR[i].ADDR;
    //     curr_phys_addr = (curr_phys_addr) << 12;
    //     printf("user page is already mapped to physical address at 0x%#x\n", curr_phys_addr);
    //     return;
    // }
    PAGE_DIR[i].U_S = 1;        // user can access
    PAGE_DIR[i].PS__PAT = 1;    // set PS to 1 to point to 4 MB page
    PAGE_DIR[i].AVL__G = 1;     // is global page
    PAGE_DIR[i].ADDR = (physical_address >> 12) & 0xFFC00;
    PAGE_DIR[i].P = 1;          // is present

    flush_tlb();
}


/* void map_user_vidmap(void);
 * Inputs: none
 * Return Value: none
 * Function: statically allocate & initialize page table & its entry in page directory for user access to video memory*/
void map_user_vidmap(void){
    int i;
    for (i = 0; i < PAGE_ARRAY_SIZE; i ++){
        // default settings for unused paging table entries
        VID_PAGE_TAB[i].P = 0;              // initialize to not present
        VID_PAGE_TAB[i].R_W = 1;            // Read/ Write by default
        VID_PAGE_TAB[i].U_S = 1;            // initialize to user mode
        VID_PAGE_TAB[i].PWT = 0;            // writeback by default
        VID_PAGE_TAB[i].PCD = 1;            // initialize cache to be disabled
        VID_PAGE_TAB[i].A = 0;              // Accessed bit not used so set to 0 by default
        VID_PAGE_TAB[i].AVL__D = 0;         // Available/ Dirty bit not used so set to 0 by default
        VID_PAGE_TAB[i].PS__PAT = 0;        // initialize Page Size/ Page Atribute Table bit to 0
        VID_PAGE_TAB[i].AVL__G = 0;         // initialize Available/ Global bit to 0
        VID_PAGE_TAB[i].AVL = 0;            // Available bits not used so set to 0 default
        VID_PAGE_TAB[i].ADDR = 0;           // bits 31-22 of base addr.; initialized to 0

        // 1 4-kB page for video memory at beginning of virtual video base addr.
        if (i == 0){        // make USER_BASE_VIRT_8MB virtual address of *start_screen
            VID_PAGE_TAB[i].U_S = 1;        // set to user mode when user maps video memory w/ vidmap system call
            VID_PAGE_TAB[i].PCD = 0;        // enable cache for video memory since it's memory-mapped I/O
            VID_PAGE_TAB[i].ADDR = (uint32_t)VIDEO_BASE_PHYS >> 12;      // bits 31-12 of physical video base addr. = 000B8
            VID_PAGE_TAB[i].P = 1;          // is present
        }
    }

    PAGE_DIR[VIDMAP_DIR].U_S = 1;        // set to user mode when user maps video memory w/ vidmap system call
                                // NOTE: Needs to be changed to user mode when user maps video memory w/ vidmap system call!!
    PAGE_DIR[VIDMAP_DIR].PCD = 0;        // enable cache for paging table since it contains page for video memory?
    PAGE_DIR[VIDMAP_DIR].PS__PAT = 0;    // set PS to 0 to point to paging table
    PAGE_DIR[VIDMAP_DIR].ADDR = ((uint32_t)(VID_PAGE_TAB) >> 12);        // bits 31-22 of paging table base addr.
    PAGE_DIR[VIDMAP_DIR].P = 1;          // is present

    flush_tlb();
}


/* void change_video_phys_addr(uint32_t terminal_num);
 * Inputs: terminal_num
 * Return Value: none
 * Function: change the physical address that's mapped to virtual address of video memory depending on the input terminal number;
 *           0 means no change, 1/2/3 means changing it to physical address of undisplayed video page of terminal 1/2/3 */
void change_video_phys_addr(uint32_t terminal_num){
    uint32_t video_phys_addr = VIDEO_BASE_PHYS + terminal_num * PAGE_NUM_ADDRS;
    uint32_t i = VIDEO_BASE_VIRT/ PAGE_NUM_ADDRS;     // 0xB8000/ 4kB
    PAGE_TAB[i].ADDR = video_phys_addr >> 12;       // bits 31-12 of physical base addr. of either actual video memory page
                                                    // or of background (undisplayed) data page for video memory for given terminal

    if (VID_PAGE_TAB[0].P == 1){    // do the same in video page in page table for user access to video memory if page is present
        VID_PAGE_TAB[0].ADDR = video_phys_addr >> 12;
    }

    flush_tlb();
    terminal_type = terminal_num;   // update the index that we use to "remap" cursor position "pages"
                                    // if terminal_type = 0, then we want changes made to screen_x & screen_y
                                    // to show up on screen when update_cursor(screen_x, screen_y) gets called;
                                    // otherwise, don't redraw cursor & instead "remap" any changes to screen_x/y
                                    // to the background cursor save of the terminal that's scheduled to run next
}

/* void save_restore_remap_video_mem(void);
 * Inputs: none
 * Return Value: none
 * Function: If different terminal is supposed to be displayed next, then save current video memory data into undisplayed video page of
 *           current terminal, & copy data from undisplayed video page of scheduled terminal into video memory; if terminal to be switched to
 *           isn't supposed to be displayed, then map virtual video memory address to physical address of undisplayed video page of scheduled terminal*/
void save_restore_remap_video_mem(void){
    // map virtual address of video memory to its actual physical address to get access to contents at physical video memory
    change_video_phys_addr(0);
    if ((terminal_next == terminal_on_display_next) && (terminal_on_display_next != terminal_on_display_curr)){
        // save current contents in video memory to background video page of currently displaying terminal
        //memcpy((void*)(VIDEO_BASE_VIRT + terminal_on_display_curr * PAGE_NUM_ADDRS), (const void*)VIDEO_BASE_VIRT, (uint32_t)SCREEN_SIZE);
        copy_paste_video_memory((int8_t*)(VIDEO_BASE_VIRT + terminal_on_display_curr * PAGE_NUM_ADDRS), (int8_t*)VIDEO_BASE_VIRT);       
        // copy contents in background video page of terminal to be displayed next to actual video memory
        // memcpy((void*)VIDEO_BASE_VIRT, (const void*)(VIDEO_BASE_VIRT + terminal_on_display_next * PAGE_NUM_ADDRS), (uint32_t)SCREEN_SIZE);
        copy_paste_video_memory((int8_t*)VIDEO_BASE_VIRT, (int8_t*)(VIDEO_BASE_VIRT + terminal_on_display_next * PAGE_NUM_ADDRS));       
        // terminal_on_display_curr = terminal_on_display_next;
    }
    if ((terminal_next != terminal_on_display_next) && (terminal_next != terminal_on_display_curr)){
        // remap video memory so that video memory altering changes in next terminal won't show up on screen
        change_video_phys_addr((uint32_t)terminal_next);
    }
}



/* void enable_paging();
 * Inputs: none
 * Return Value: none
 * Function: let CPU know where page directory is, enable page size extension, & then enable paging */
void enable_paging(void){     // load PAGE_DIR as argument & put it in cr3
    // put base address of paging directory in CR3
    asm volatile ("             \n\
        leal PAGE_DIR, %eax     \n\
        movl %eax, %cr3         \n\
        "
    );

    // set PSE bit in CR4 to actually enable page size extension
    // note: need to set PSE bit before enabling paging, otherwise it gives error "cannot access memory at address 0x7fff94"
    //       maybe because it doesn't know what to do with size-extended kernel page
    // also note: need to u put $ in front of hex value so it won't be read as memory address
    asm volatile ("             \n\
        movl %cr4, %eax         \n\
        orl $0x00000010, %eax   \n\
        movl %eax, %cr4         \n\
        "
    );

    // set paging (PG) bit of CR0 (protection (PE) bit has already been set by given code)
    asm volatile ("             \n\
        movl %cr0, %eax         \n\
        orl $0x80000000, %eax   \n\
        movl %eax, %cr0         \n\
        "
    );
}
