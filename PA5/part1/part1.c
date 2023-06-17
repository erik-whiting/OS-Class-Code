#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/******************************************************
 * Declarations
 ******************************************************/
// #Define'd sizes
#define BIT_SHIFT 8
#define NOT_FOUND -1

// Limits on memory
#define PHYSICAL_MEMORY_SIZE 256
#define FRAME_SIZE 256
#define PAGE_TABLE_SIZE 256
#define TLB_SIZE 16

// Stats
int TLB_HIT = 0;
int PAGE_TABLE_HIT = 0;
int NUM_PAGE_FAULT = 0;
int NUM_TRANSLATED_ADDRESSES = 0;

// Make the TLB array
int TLB[TLB_SIZE][2]; // page, frame
// Need pages associated with frames (could be 2D array, or C++ list, etc.)
// Make the Page Table
int PAGE_TABLE[PAGE_TABLE_SIZE][2]; // page, frame
// Again, need pages associated with frames (could be 2D array, or C++ list, etc.)

int PAGE_TABLE_INDEX, TLB_INDEX, PHYSICAL_MEMORY_INDEX = 0;

// Make the memory
// Memory array (easiest to have a 2D array of size x frame_size)
int PHYSICAL_MEMORY[PHYSICAL_MEMORY_SIZE][FRAME_SIZE]; // simulated physical memory
char BACKING_BUFFER[FRAME_SIZE]; // temporarily store frame from backing store

/******************************************************
 * Function Declarations
 ******************************************************/

/***********************************************************
 * Function: get_page_and_offset - get the page and offset from the logical address
 * Parameters: logical_address
 *   page_num - where to store the page number
 *   offset - where to store the offset
 * Return Value: none
 ***********************************************************/
void get_page_and_offset(int logical_address, int *page_num, int *offset) {
    *page_num = (logical_address & 0xFFFF) >> BIT_SHIFT;
    *offset = logical_address & 0xFF;
    NUM_TRANSLATED_ADDRESSES++;
}

/***********************************************************
 * Function: get_frame_TLB - tries to find the frame number in the TLB
 * Parameters: page_num
 * Return Value: the frame number, else NOT_FOUND if not found
 ***********************************************************/
int get_frame_TLB(int page_num) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (TLB[i][0] == page_num) {
            return TLB[i][1];
        }
    }
    return NOT_FOUND;
}

/***********************************************************
 * Function: get_available_frame - get a valid frame
 * Parameters: none
 * Return Value: frame number
 ***********************************************************/
int get_available_frame() {
    if (PHYSICAL_MEMORY_INDEX == PHYSICAL_MEMORY_SIZE) {
        PHYSICAL_MEMORY_INDEX = 0;
    }
    return PHYSICAL_MEMORY_INDEX++;
}

/***********************************************************
 * Function: get_frame_pagetable - tries to find the frame in the page table
 * Parameters: page_num
 * Return Value: page number, else NOT_FOUND if not found (page fault)
 ***********************************************************/
int get_frame_pagetable(int page_num) {
    for (int i = 0; i < PAGE_TABLE_SIZE; i++) {
        if (PAGE_TABLE[i][0] == page_num) {
            return PAGE_TABLE[i][1];
        }
    }
    return NOT_FOUND;
}

/***********************************************************
 * Function: backing_store_to_memory - finds the page in the backing store and
 *   puts it in memory
 * Parameters: page_num - the page number (used to find the page)
 *   frame_num - the frame number for storing in physical memory
 * Return Value: none
 ***********************************************************/
void backing_store_to_memory(int page_num, int frame_num, int offset, const char *fname) {
    FILE *backing_file = fopen(fname, "rb");
    fseek(backing_file, page_num * 256, SEEK_SET);
    size_t result = fread(BACKING_BUFFER, sizeof(char), 256, backing_file);
    int value = BACKING_BUFFER[offset]; // <-- This gives correct value
    // int frame = get_available_frame();
    for (int i = 0; i < 256; i++) {
        PHYSICAL_MEMORY[frame_num][i] = BACKING_BUFFER[i];
    }
}

/***********************************************************
 * Function: update_page_table - update the page table with frame info
 * Parameters: page_num, frame_num
 * Return Value: none
 ***********************************************************/
void update_page_table(int page_num, int frame_num) {
    PAGE_TABLE[PAGE_TABLE_INDEX][0] = page_num;
    PAGE_TABLE[PAGE_TABLE_INDEX][1] = frame_num;
    PAGE_TABLE_INDEX = (PAGE_TABLE_INDEX + 1) % PAGE_TABLE_SIZE;
}

/***********************************************************
 * Function: update_TLB - update TLB (FIFO)
 * Parameters: page_num, frame_num
 * Return Value: none
 ***********************************************************/
void update_TLB(int page_num, int frame_num) {
    TLB[TLB_INDEX][0] = page_num;
    TLB[TLB_INDEX][1] = frame_num;
    TLB_INDEX = (TLB_INDEX + 1) % TLB_SIZE;
}


/******************************************************
 * Assumptions:
 *   If you want your solution to match follow these assumptions
 *   1. In Part 1 it is assumed memory is large enough to accommodate
 *      all frames -> no need for frame replacement
 *   2. Part 1 solution uses FIFO for TLB updates
 *   3. In the solution binaries it is assumed a starting point at frame 0,
 *      subsequently, assign frames sequentially
 *   4. In Part 2 you should use 128 frames in physical memory
 ******************************************************/


int main(int argc, char * argv[]) {
		// argument processing
        if (argc < 3) {
            printf("Not enough arguments\n");
            printf("USAGE: ./part1 BACKING_STORE.bin addresses.txt\n");
            exit(1);
        }
        char const* const backing_store_filename = argv[1];
        char const* const addresses_filename = argv[2];
        char line[256]; // To be used when reading the file
		// For Part2: read in whether this is FIFO or LRU strategy
        memset(TLB, -1, sizeof(TLB[TLB_SIZE][2]) * TLB_SIZE * 2); // If TLB[x][y] is -1, I know it's not set
        memset(PAGE_TABLE, -1, sizeof(PAGE_TABLE[PAGE_TABLE_SIZE][2]) * PAGE_TABLE_SIZE * 2);
        int page_number, offset = 0;
        FILE* addresses = fopen(addresses_filename, "r");
        // Create output file
        FILE* output_file = fopen("correct.txt", "w");
        // read addresses.txt
		while(fgets(line, sizeof(line), addresses)) {
            int frame;;
            int address = atoi(line);
            // Step 0: get page number and offset, bit twiddling
            get_page_and_offset(address, &page_number, &offset);
            // need to get the physical address (frame + offset):
            // Step 1: check in TLB for frame
            int tlb_frame = get_frame_TLB(page_number);
            if (tlb_frame == -1) {
                // Step 2: not in TLB, look in page table
                int pt_frame = get_frame_pagetable(page_number);
                if (pt_frame == -1) {
                    NUM_PAGE_FAULT++;
                    // Step 3:
                    // dig up frame in BACKING_STORE.bin (backing_store_to_memory())
                    // bring in frame page# x 256
                    // store in physical memory
                    frame = get_available_frame();
                    update_page_table(page_number, frame);
                    update_TLB(page_number, frame);
                } else {
                    // Page number was in page table
                    frame = pt_frame;
                    update_TLB(page_number, frame);
                }
            } else {
                // Page number was in TLB
                TLB_HIT++;
                frame = tlb_frame;
            }
            backing_store_to_memory(page_number, frame, offset, backing_store_filename);
            int value = BACKING_BUFFER[offset]; // <-- This gives correct value
            fprintf(output_file, "Virtual address: %i Physical address: %i Value: %i\n", address, (frame << 8)|offset, value);
		}
        fprintf(output_file, "Number of Translated Addresses = %i\n", NUM_TRANSLATED_ADDRESSES);
        fprintf(output_file, "Page Faults = %i\n", NUM_PAGE_FAULT);
        float pfr = (float)NUM_PAGE_FAULT / (float)NUM_TRANSLATED_ADDRESSES;
        fprintf(output_file, "Page Fault Rate = %.3f\n", pfr);
        fprintf(output_file, "TLB Hits = %i\n", TLB_HIT);
        float thr = (float)TLB_HIT / (float)NUM_TRANSLATED_ADDRESSES;
        fprintf(output_file, "TLB Hit Rate = %.3f\n", thr);
        fclose(addresses);
        fclose(output_file);
}

