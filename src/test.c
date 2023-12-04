#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <dirent.h>

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
    //char* str2 = getenv("PWD");
    printf("\n%s", str1); 
    //printf("%s> ", str2);
    /*char* str1 = "fat32.img/> ";

    char str2[1024]; // Using an array instead of a pointer

    if (getcwd(str2, sizeof(str2)) != NULL) {
        char* substring = strstr(str2, "fat32");
        if (substring != NULL) {
            printf("\n%s", str1);
            printf("%s> ", substring);
        } else {
            perror("Invalid directory");
        }
    } else {
        perror("getcwd() error");
    }*/
}

void process_ls() {
    DIR *directory;
    struct dirent *entry;

    directory = opendir(".");
    if (directory == NULL) {
        printf("Unable to open directory\n");
        return;
    }

    while ((entry = readdir(directory)) != NULL) {
        printf("%s\n", entry->d_name);
    }

    closedir(directory);
}

void process_mkdir(const char* dirname) {
    
    
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

uint32_t totalClustersInDataRegion()
{
    // Sectors occupied by the root directory (root entries * size of a directory entry)
    uint32_t rootDirSectors = ((bpb.BPB_RootEntCnt * 32) + (bpb.BPB_BytsPerSec - 1)) / bpb.BPB_BytsPerSec;

    printf("\nthis is the rootDirSectors var: %u\n", rootDirSectors);
    // Calculate the sectors occupied by the FAT tables
    uint32_t fatSize = bpb.BPB_sectorsPerFAT32 * bpb.BPB_NumFATs;

    printf("\nthis is the sectorsPerFAT32 var: %u\n", bpb.BPB_sectorsPerFAT32);
    printf("\nthis is the fatSize var: %u\n", fatSize);
    // Calculate the data region size in sectors
    uint32_t dataRegionSize = bpb.BPB_TotSec32 - (bpb.BPB_RsvdSecCnt + rootDirSectors); //+ fatSize

    printf("\nthis is the dataRegionSize var: %u\n", dataRegionSize);
    printf("\nthis is the SecPerClus var: %u\n", bpb.BPB_SecPerClus);
    // Calculate the total number of clusters
    uint32_t totalClusters = dataRegionSize / bpb.BPB_SecPerClus;
    printf("\nthis is the totalClusters var: %u\n", totalClusters);
    return totalClusters; 
}

uint32_t calculate_first_data_sector() {
    // Sectors occupied by the reserved regions and the number of FATs
    uint32_t reservedSectors = bpb.BPB_RsvdSecCnt;
    uint32_t numberOfFATs = bpb.BPB_NumFATs;

    // Sectors occupied by the root directory (root entries * size of a directory entry)
    uint32_t rootDirSectors = ((bpb.BPB_RootEntCnt * 32) + (bpb.BPB_BytsPerSec - 1)) / bpb.BPB_BytsPerSec;

    // Calculate the sectors occupied by the FAT tables
    uint32_t fatSize = bpb.BPB_sectorsPerFAT32 * numberOfFATs;

    // Calculate the first data sector
    uint32_t firstDataSector = reservedSectors + fatSize + rootDirSectors;

    return firstDataSector;
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
			break;
		}

        tokens = get_tokens(input);


        // else if cmd is "cd" process_cd();
        if (strcmp(tokens->items[0], "cd") == 0)
        {
            //cd variables
            char cwd[200];
            int setenv(const char *name, const char *value, int overwrite);
            //processing cd
            if (tokens->size == 1) {chdir(getenv("HOME"));}
			else {chdir(tokens->items[1]);}
			getcwd(cwd,200);
			setenv("PWD",cwd,1);
            //process_cd(); 
        } else if (strcmp(tokens->items[0], "ls") == 0)
        {
            process_ls();
        } 
        else if (strcmp(tokens->items[0], "mkdir") == 0)
        {
            process_mkdir(tokens->items[1]);

        }
        
        else if (strcmp(tokens->items[0], "info") == 0)
        {
            printf("Bytes Per Sector: %u\n", bpb.BPB_BytsPerSec);
            printf("Sectors Per Cluster: %u\n", bpb.BPB_SecPerClus);

            printf("Total clusters in Data Region: %u\n", totalClustersInDataRegion());
            printf("# of entries in one FAT: %u\n", bpb.BPB_NumHeads);
            printf("Size of Image (bytes): %u\n", bpb.BPB_Media);
            printf("Root Cluster: %u\n", bpb.BPB_NumFATs);
            printf("FirstDataSector: %u\n", calculate_first_data_sector());
            printf("Total Sectors: %u\n", bpb.BPB_TotSec32);
            //these are the right ones below
            //printf("Reserved Sectors: %u\n", bpb.BPB_RsvdSecCnt);
            printf("\nNumber of FATs: %u\n", bpb.BPB_NumFATs);    //rootcluster ???
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
