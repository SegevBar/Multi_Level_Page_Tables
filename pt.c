#include "os.h"
#define EXTRACT_9BITS	(0x1ff)

/* prototypes */
void destroy_virtual_memory_mapping(uint64_t* root_pt, uint64_t vpn);
void create_virtual_memory_mapping(uint64_t* root_pt, uint64_t vpn, uint64_t ppn);

/*
* A function to create/destroy virtual memory mappings in a page table
* 
* This function takes the following arguments:
*   (a) pt: The physical page number of the page table root (this is the 
*       physical page that the page table base register in the CPU state will 
*       point to). You can assume that pt has been previously returned by alloc 
*       page frame().
*   (b) vpn: The virtual page number the caller wishes to map/unmap.
*   (c) ppn: Can be one of two cases. If ppn is equal to a special NO MAPPING 
*       value (defined in os.h), then vpn's mapping (if it exists) should be 
*       destroyed. Otherwise, ppn specifies the physical page number that vpn 
*       should be mapped to.
*/
void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn) {
    uint64_t* pt_va = phys_to_virt(pt << 12);

    if (ppn == NO_MAPPING) {
        destroy_virtual_memory_mapping(pt_va, vpn);
    } else {
        create_virtual_memory_mapping(pt_va, vpn, ppn);
    }
}

/*
* A function to destroy virtual memory mappings in a page table
* 
* This function takes the following arguments:
*   (a) pt: The physical page number of the page table root (this is the 
*       physical page that the page table base register in the CPU state will 
*       point to). You can assume that pt has been previously returned by alloc 
*       page frame().
*   (b) vpn: The virtual page number the caller wishes to map/unmap.
*/
void destroy_virtual_memory_mapping(uint64_t* root_pt, uint64_t vpn) {
    int i;
    uint64_t* curr_pt = root_pt;
    uint64_t pte = (vpn >> 36) & EXTRACT_9BITS;
    
    /* trie has 5 levels (log(4KB/8B)==9,(64-12-7)==45, 45/9==5 levels) */
    for (i = 1; i < 5; i++) {      
        if (curr_pt[pte] % 2 == 0) {  /* LSB == 0, VALID flag is off */
            break;
        } 
        curr_pt = phys_to_virt(curr_pt[pte] - 1); /* point to next level page, zerofy valid flag to calc va */
        pte = (vpn >> (36 - (i*9))) & EXTRACT_9BITS;  /* extract next 9 bits from vpn */
    }
    curr_pt[pte] = 0;  /* destroy buttom level mapping */
}

/*
* A function to create virtual memory mappings in a page table
* 
* This function takes the following arguments:
*   (a) pt: The physical page number of the page table root (this is the 
*       physical page that the page table base register in the CPU state will 
*       point to). You can assume that pt has been previously returned by alloc 
*       page frame().
*   (b) vpn: The virtual page number the caller wishes to map/unmap.
*   (c) ppn: Can be one of two cases. If ppn is equal to a special NO MAPPING 
*       value (defined in os.h), then vpn's mapping (if it exists) should be 
*       destroyed. Otherwise, ppn specifies the physical page number that vpn 
*       should be mapped to.
*/
void create_virtual_memory_mapping(uint64_t* root_pt, uint64_t vpn, uint64_t ppn) {
    int i;
    uint64_t* curr_pt = root_pt;
    uint64_t pte = (vpn >> 36) & EXTRACT_9BITS;
    
    /* trie has 5 levels (log(4KB/8B)==9,(64-12-7)==45, 45/9==5 levels) */
    for (i = 1; i < 5; i++) { 
        if (curr_pt[pte] % 2 == 0) {  /* LSB == 0, VALID flag is off */
            curr_pt[pte] = (alloc_page_frame() << 12) + 1;
        } 
        curr_pt = phys_to_virt(curr_pt[pte] - 1); /* point to next level page, zerofy valid flag to calc va */
        pte = (vpn >> (36 - (i*9))) & EXTRACT_9BITS;  /* extract next 9 bits from vpn */
    }
    curr_pt[pte] = (ppn << 12) + 1;
}

/*
* A function to query the mapping of a virtual page number in a page table
* This function returns the physical page number that vpn is mapped to, or NO 
* MAPPING if no mapping exists
* 
* This function takes the following arguments:
*   (a) pt: The physical page number of the page table root (this is the 
*       physical page that the page table base register in the CPU state will 
*       point to). You can assume that pt has been previously returned by alloc 
*       page frame().
*   (b) vpn: The virtual page number the caller wishes to map/unmap.
*/
uint64_t page_table_query(uint64_t pt, uint64_t vpn) {
    int i;
    uint64_t ppn;
    uint64_t* curr_pt = phys_to_virt(pt << 12);
    uint64_t pte = (vpn >> 36) & EXTRACT_9BITS;
    
    /* trie has 5 levels (log(4KB/8B)==9,(64-12-7)==45, 45/9==5 levels) */
    for (i = 1; i < 5; i++) {
        if (curr_pt[pte] % 2 == 0) {  /* LSB == 0, VALID flag is off */
            return NO_MAPPING;
        } 
        curr_pt = phys_to_virt(curr_pt[pte] - 1); /* point to next level page, zerofy valid flag to calc va */
        pte = (vpn >> (36 - (i*9))) & EXTRACT_9BITS;  /* extract next 9 bits from vpn */
    }
    
    if (curr_pt[pte] % 2 == 1) { /* LSB == 1, VALID flag in buttom level is on */
        ppn = (curr_pt[pte] >> 12);  /* 52 left bits */
    } else {
        return NO_MAPPING;
    }
    return ppn;
}
