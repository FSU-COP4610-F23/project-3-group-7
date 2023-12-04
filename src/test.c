//#include <stdio.h>
//#include <stdint.h>

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include "lexer.h"

typedef struct __attribute__((packed)) BPB {
    // below 36 bytes are the main bpb
	uint8_t BS_jmpBoot[3];
	char BS_OEMName[8];
	uint16_t BPB_BytsPerSec;
	uint8_t BPB_SecPerClus;
	uint16_t BPB_RsvdSecCnt;
	uint8_t BPB_NumFATs;
	uint16_t BPB_RootEntCnt;
	uint16_t BPB_TotSec16;
	uint8_t BPB_Media;
	uint16_t BPB_FATSz16;
	uint16_t BPB_SecPerTrk;
	uint16_t BPB_NumHeads;
	uint32_t BPB_HiddSec;
	uint32_t BPB_TotSec32;
    // below are the extend bpb.
    // please declare them.
    uint32_t BPB_rootCluster;
    uint32_t BPB_sectorsPerFAT32;
    uint32_t BPB_totalSectors;
} bpb_t;


typedef struct __attribute__((packed)) directory_entry {
    char DIR_Name[11];
    uint8_t DIR_Attr;
    char padding_1[8]; // DIR_NTRes, DIR_CrtTimeTenth, DIR_CrtTime, DIR_CrtDate, 
                       // DIR_LstAccDate. Since these fields are not used in
                       // Project 3, just define as a placeholder.
    uint16_t DIR_FstClusHI;
    char padding_2[4]; // DIR_WrtTime, DIR_WrtDate
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
} dentry_t;

void showPrompt()
{
    char* str1 = "fat32.img/> "; 
    printf("\n%s", str1); 
}

// other data structure, global variables, etc. define them in need.
// e.g., 
// the opened fat32.img file
// the current working directory
// the opened files
// other data structures and global variables you need

bpb_t bpb;
tokenlist* tokens; 

// you can give it another name
// fill the parameters
void mount_fat32() {
    // 1. decode the bpb
}

// you can give it another name
// fill the parameters
void main_process() {
    char* input = "start";
    while (strcmp(input, "exit") != 0) {
        // 1. get cmd from input.
        showPrompt();
        input = get_input(); 
        // you can use the parser provided in Project1

        // if cmd is "exit" break;
        if (strcmp(input, "exit") == 0)
		{
			// Print valid commands.
			break;
		}

        tokens = get_tokens(input);


        // else if cmd is "cd" process_cd();
        if (strcmp(tokens->items[0], "/bin/cd") == 0)
        {
            //process_cd(); 
        } else if (strcmp(tokens->items[0], "ls") == 0)
        {
            // else if cmd is "ls" process_ls();
            // ...
        } else if (strcmp(tokens->items[0], "info") == 0)
        {
            printf("Bytes Per Sector: %u\n", bpb.BPB_BytsPerSec);
            printf("Sectors Per Cluster: %u\n", bpb.BPB_SecPerClus);
            printf("Total clusters in Data Region: %u\n", bpb.BPB_NumFATs);
            printf("# of entries in one FAT: %u\n", bpb.BPB_RootEntCnt);
            printf("Size of Image (bytes): %u\n", bpb.BPB_TotSec32);
            printf("Root Cluster: %u\n", bpb.BPB_RootEntCnt);
            printf("FirstDataSector: %u\n", bpb.BPB_TotSec32);
            printf("Total Sectors: %u\n", bpb.BPB_TotSec32);
            //these are the right ones below
            //printf("Reserved Sectors: %u\n", bpb.BPB_RsvdSecCnt);
            printf("Number of FATs: %u\n", bpb.BPB_NumFATs);    //rootcluster ???
            printf("Root Entries: %u\n", bpb.BPB_RootEntCnt);
            printf("Total Sectors: %u\n", bpb.BPB_TotSec32);
        }
        
    }
}


int main(int argc, char const *argv[])
{
    // 1. open the fat32.img
    const char *imagePath = "../fat32.img";

    FILE *file = fopen(imagePath, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }
    

    // 2. mount the fat32.img

    // Seek to the beginning of the boot sector
    fseek(file, 0, SEEK_SET);

    // Read the BPB data into the structure
    fread(&bpb, sizeof(bpb_t), 1, file);


     // Calculate the offset to the root directory cluster
    uint32_t rootDirCluster = bpb.BPB_rootCluster;

    // Calculate the offset in bytes
    uint32_t rootDirOffset = ((rootDirCluster - 2) * bpb.BPB_SecPerClus + bpb.BPB_RsvdSecCnt +
                             bpb.BPB_NumFATs * bpb.BPB_sectorsPerFAT32) *
                             bpb.BPB_BytsPerSec;

    // Seek to the beginning of the root directory
    fseek(file, rootDirOffset, SEEK_SET);

    main_process(); 


    
    // 3. main procees
    

    // 4. close all opened files

    // 5. close the fat32.img
    fclose(file);

    return 0;
}
