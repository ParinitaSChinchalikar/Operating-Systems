#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_TLB_ENTRIES 255

unsigned PAGE_SIZE_B; // Page size in bytes
unsigned char TLB_SIZE;    // TLB size
unsigned short int* PPN = NULL;       // Physical Page Number
unsigned short int* NEW_ADDRESS = NULL;   // New Virtual Address to service
unsigned char REPL;           // Replacement algorithm: 0 for FIFO, 1 for Random, 2 for Corbato’s Clock
int seed;

int main(int argc, char* argv[])
{
    if (argc < 3 || argc > 4)
    {
        printf("You must enter an input file to test and a replacement policy!\n");
        exit(-1);
    }

    FILE* fp = fopen(argv[1], "rb");

    if (fp == NULL)
    {
        printf("Error opening file!\n");
        exit(-1);
    }

    fread(&PAGE_SIZE_B, 1, 1, fp);
    fread(&TLB_SIZE, 1, 1, fp);

    unsigned int NUM_PAGE_TABLE_ENTRIES = pow(2, 16) / PAGE_SIZE_B;

    PPN = malloc(NUM_PAGE_TABLE_ENTRIES * sizeof(unsigned short int));

    for (int i = 0; i < NUM_PAGE_TABLE_ENTRIES; i++)
    {
        if (fread(PPN + i, 2, 1, fp) != 1)
        {
            printf("Something went wrong with fread!\n");
            exit(-1);
        }
    }

    int PAGE_FAULTS = 0;
    int TLB_HITS = 0;
    int TLB_MISSES = 0;

    unsigned int TLB[MAX_TLB_ENTRIES][2] = {0}; // TLB: 2 columns (Page Number, Frame Number)
    unsigned int PAGE_TABLE_SIZE = NUM_PAGE_TABLE_ENTRIES;
    unsigned char* PAGE_TABLE = malloc(PAGE_TABLE_SIZE * sizeof(unsigned char)); // Page Table: boolean array to track page presence

    for (int i = 0; i < PAGE_TABLE_SIZE; i++)
    {
        PAGE_TABLE[i] = 0; // Initialize all pages as not present in memory
    }

    unsigned int clock_hand = 0; // Clock hand position for Corbato’s Clock algorithm
    int free_index = -1;   // Find a free TLB entry

    while (!feof(fp))
    {
        NEW_ADDRESS = malloc(sizeof(unsigned short int));

        if (fread(NEW_ADDRESS, 2, 1, fp) == 1)
        {
            unsigned int VIRTUAL_ADDRESS = *NEW_ADDRESS;
            unsigned int PHYSICAL_ADDRESS;

            int TLB_ENTRY_FOUND = -1; // Initialize to -1 to indicate TLB miss

            for (int i = 0; i < TLB_SIZE; i++)
            {
                if (TLB[i][0] == VIRTUAL_ADDRESS / PAGE_SIZE_B && PAGE_TABLE[VIRTUAL_ADDRESS / PAGE_SIZE_B])
                {
                    TLB_ENTRY_FOUND = i;
                    break;
                }
            }

            if (TLB_ENTRY_FOUND != -1)
            {
                PHYSICAL_ADDRESS = (TLB[TLB_ENTRY_FOUND][1] * PAGE_SIZE_B) + VIRTUAL_ADDRESS % PAGE_SIZE_B;
                TLB_HITS++;
            }
            else
            {
                TLB_MISSES++;

                if (!PAGE_TABLE[VIRTUAL_ADDRESS / PAGE_SIZE_B])
                {
                    PAGE_FAULTS++;
                    PAGE_TABLE[VIRTUAL_ADDRESS / PAGE_SIZE_B] = 1;
                }

                unsigned int FRAME_NUMBER = PPN[VIRTUAL_ADDRESS / PAGE_SIZE_B];
                PHYSICAL_ADDRESS = (FRAME_NUMBER * PAGE_SIZE_B) + VIRTUAL_ADDRESS % PAGE_SIZE_B;

                if (free_index == -1)
                {
                    switch (REPL)
                    {
                        case 0: // FIFO
                            free_index = clock_hand;
                            clock_hand = (clock_hand + 1) % TLB_SIZE;
                            break;
                        case 1: // Random
                            free_index = rand() % TLB_SIZE;
                            break;
                        case 2: // Clock
                            while (TLB[clock_hand][1] && PAGE_TABLE[TLB[clock_hand][0]] == 0)
                            {
                                TLB[clock_hand][1] = 0;
                                clock_hand = (clock_hand + 1) % TLB_SIZE;
                            }
                            free_index = clock_hand;
                            break;
                    }
                }

                TLB[free_index][0] = VIRTUAL_ADDRESS / PAGE_SIZE_B;
                TLB[free_index][1] = FRAME_NUMBER;
            }

            printf("VA:%x -- PA:%x\n", VIRTUAL_ADDRESS, PHYSICAL_ADDRESS);

            free(NEW_ADDRESS);
        }
    }

    printf("Page Faults: %d\nTLB hits: %d\nTLB misses: %d\n", PAGE_FAULTS, TLB_HITS, TLB_MISSES);

    free(PPN);
    free(PAGE_TABLE);
    fclose(fp);

    return 0;
}