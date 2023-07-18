#ifndef _PAGING_H
#define _PAGING_H

#include "types.h"  // use this for explicitly-sized types (uint32_t, int8_t, etc.) instead of <stdint.h>
#include "system_calls_asm.h"

#define PAGE_ARRAY_SIZE     1024
#define KERNEL_BASE         0x400000    // 4 MB
#define USER_BASE_VIRT      0x8000000
#define USER_BASE_VIRT_4MB  0x8400000
#define USER_BASE_VIRT_8MB  0x8800000
#define VIDMAP_DIR          USER_BASE_VIRT_8MB/KERNEL_BASE // 136/4
#define USER_BASE_PHYS_8MB  0x800000
#define USER_BASE_PHYS_12MB 0xC00000
#define SHELL_BASE_PHYS         0x800000    // 8 MB
#define USER_PROG_6_BASE_PHYS   0x1C00000   // 28 MB
#define VIDEO_BASE_PHYS     0xB8000     // physical base addr. of video memory
#define VIDEO_BASE_VIRT     0xB8000     // virtual base addr. of video memory, same as physical for now but may change
#define VIDEO_BACKGROUND_1  0xB9000     // physical & virtual base addr. of background (undisplayed) data page for video memory for terminal 1
#define VIDEO_BACKGROUND_2  0xBA000     // physical & virtual base addr. of background (undisplayed) data page for video memory for terminal 2
#define VIDEO_BACKGROUND_3  0xBB000     // physical & virtual base addr. of background (undisplayed) data page for video memory for terminal 3
#define SCREEN_SIZE         (80 * 25 * 2)
#define PAGE_NUM_ADDRS      4096        // number of addresses in a page
#define TAB_NUM_ADDRS       (PAGE_ARRAY_SIZE * PAGE_NUM_ADDRS)      // number of addresses in a paging table
                            // each table/ 4 MB page takes up 1024 * 4 kB addresses (so total of 4 GB virtual addresses)

// define struct type for entries in both paging directory & paging table
typedef struct __attribute__((packed)) page_entry_t{
    uint8_t P : 1;          // Present; 1 bit
    uint8_t R_W : 1;        // Read/ Write; set to 1 for all
    uint8_t U_S : 1;        // User/ Supervisor
    uint8_t PWT : 1;        // Write-Through if 1, writeback if 0; set to 0 for all
    uint8_t PCD : 1;        // Cache Disable
    uint8_t A : 1;          // Accessed; set to 0 for all
    uint8_t AVL__D : 1;     // Available bit if PS = 0; Dirty bit if PS = 1 or is PT entry; either way set to 0 for all
    uint8_t PS__PAT : 1;    // Page Size bit if PD entry; Page Atribute Table bit if PT entry (set to 0)
    uint8_t AVL__G : 1;     // Available bit if PS = 0 (set to 0); Global bit if PS = 1 or is PT entry
    uint8_t AVL : 3;        // 3 Available bits; set to 0 for all
    uint32_t ADDR : 20;     // upper 20 bits of Address if PS = 0 or PT entry
                            // otherwise if PS = 1:
                            // bits 31-22 of addr. | 10 reserved bits (set to 0)
} page_entry_t;

void map_virtual_to_physical_address(uint32_t virtual_address, uint32_t physical_address);
extern struct page_entry_t PAGE_TAB[PAGE_ARRAY_SIZE] __attribute__((aligned (PAGE_NUM_ADDRS)));    // paging table
extern struct page_entry_t PAGE_DIR[PAGE_ARRAY_SIZE] __attribute__((aligned (PAGE_NUM_ADDRS)));    // paging directory
extern struct page_entry_t VID_PAGE_TAB[PAGE_ARRAY_SIZE] __attribute__((aligned(PAGE_NUM_ADDRS))); // vidmap paging table

/* Externally-visible functions */
void paging_table_init(void);        // initialize paging table
void paging_directory_init(void);    // initialize paging directory
void enable_paging(void);
void map_virt_user_to_physical(uint32_t physical_address);  // statically allocate & initialize user page
void map_user_vidmap(void);
void change_video_phys_addr(uint32_t terminal_num);
void save_restore_remap_video_mem(void);

#endif /* _PAGING_H */
