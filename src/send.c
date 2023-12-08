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
#include <ctype.h>

#include "lexer.h"
//gcc -c -g -Wall -Wextra -std=c99 test.c -o test.o
//gcc -g -Wall -Wextra -std=c99 test.o lexer.o
//./a.out fat32.img

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
    uint32_t BPB_FATSz32; 
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;
    uint32_t BPB_rootCluster;
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    uint8_t BPB_Reserved[12];
    uint8_t BS_DrvNum;
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint32_t BS_VolID;
    char BS_VolLab[11];
    char BS_FilSysType[8];
} bpb_t;

typedef struct {
    char filename[12]; // Name of the file
    uint32_t first_cluster; // First cluster of the file
    uint32_t current_offset; // Current read/write offset in the file
    uint32_t size; // Size of the file in bytes
    char mode[3]; // Mode in which the file is opened (-r, -w, -rw, -wr)
} open_file_t;


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

typedef struct __attribute__((packed)) cluster {
    int depth; 
    uint32_t offset;
    uint32_t curr_clus_num;
    uint32_t next_clus_num;
    uint32_t BPB_SecPerClus;
    uint32_t BPB_FATSz32;
    uint32_t end_clus_num;
    uint32_t max_clus_num;
    uint32_t min_clus_num;
    char cluster_path[100]; 
} cluster_info;

uint32_t current_dir_cluster = 2; 

bpb_t bpb;
tokenlist* tokens; 
FILE *file;
cluster_info cluster; 
dentry_t globalDirectory; 
uint32_t cwd; 
uint32_t prev_cwd; 

void format_dir_name(const char *dir_name, char *formatted_name); 

#define MAX_OPEN_FILES 10
open_file_t opened_files[MAX_OPEN_FILES]; // Array to store opened files
int num_opened_files = 0;

uint32_t convert_offset_to_clus_num_in_fat_region(uint32_t offset) { // the offset to the beginning of the file.
    uint32_t fat_region_offset = 0x4000;
    return (offset - fat_region_offset)/4;
}

uint32_t convert_clus_num_to_offset_in_fat_region(uint32_t clus_num) {
    uint32_t fat_region_offset = 0x4000;
    return fat_region_offset + clus_num * 4;
}


uint32_t convert_clus_num_to_offset_in_data_region(uint32_t clus_num) {
    uint32_t clus_size = 512;
    uint32_t data_region_offset = 0x100400;
    return data_region_offset + (clus_num - 2) * clus_size;
}

void showPrompt(char const *argv)
{
    char* etc = "> ";
    printf("\n%s%s%s", argv, cluster.cluster_path, etc); 
}


void dbg_print_dentry(dentry_t *dentry) {
    if (dentry == NULL) {
        return ;
    }
    printf("DIR_Name: %s\n", dentry->DIR_Name);
    printf("DIR_Attr: 0x%x\n", dentry->DIR_Attr);
    printf("DIR_FstClusHI: 0x%x\n", dentry->DIR_FstClusHI);
    printf("DIR_FstClusLO: 0x%x\n", dentry->DIR_FstClusLO);
    printf("DIR_FileSize: %u\n", dentry->DIR_FileSize);
}

void print_open_files()
{
    int index = 0; 
    printf(" %-7s  %-12s  %-10s %-10s  %-8s\n", "INDEX", "NAME", "MODE", "OFFSET", "PATH");
    for (int i = 0; i < num_opened_files; i++)
    {
        printf(" %-7d  %-12s  %-10s %-10d  %-8s\n", i, "NAME", "MODE", 0, "PATH");
    }
    
    printf("\n"); 
}

void trim_spaces(char *str) {
    int len = strlen(str);
    int start = 0, end = len - 1;

    while (isspace((unsigned char)str[start])) {     // Trim leading spaces
        start++;
    }

    while (end >= start && isspace((unsigned char)str[end])) { // Trim trailing spaces
        str[end--] = '\0';
    }
    
    if (start > 0) {// If leading spaces were found, shift remaining characters to the beginning of the string
        int i = 0;
        while (str[start + i] != '\0') {
            str[i] = str[start + i];
            i++;
        }
        str[i] = '\0';
    }
}

dentry_t *encode_dir_entry(int fat32_fd, uint32_t offset) {
    dentry_t *dentry = (dentry_t*)malloc(sizeof(dentry_t));
    ssize_t rd_bytes = pread(fat32_fd, (void*)dentry, sizeof(dentry_t), offset);
    trim_spaces(dentry->DIR_Name); 
    return dentry;
}

void read_directory(int fat32_fd, uint32_t dir_offset, uint32_t dir_size) {
    uint32_t current_offset = dir_offset;

    while (current_offset < dir_offset + dir_size) {
        dentry_t *dentry = encode_dir_entry(fat32_fd, current_offset);

        // Check if the directory entry is valid
        if (dentry->DIR_Name[0] == 0x00) { // No more entries
            free(dentry);
            break;
        }
        if (dentry->DIR_Name[0] != 0xE5 &&  // Entry is not deleted
            ((dentry->DIR_Attr & 0x0F) != 0x0F) &&
            (dentry->DIR_Attr & 0x02) != 0x02) { 

            // Remove spaces from the directory name
            char formatted_name[13];
            memset(formatted_name, 0, 13);

            int i = 0, j = 0;
            while (i < 11 && dentry->DIR_Name[i] != ' ' && dentry->DIR_Name[i] != '\0') {
                formatted_name[j++] = dentry->DIR_Name[i++];
            }
            formatted_name[j] = '\0';

            printf("%s ", formatted_name);
            //dbg_print_dentry(dentry);
        }
        free(dentry);
        // Move to the next directory entry
        current_offset += sizeof(dentry_t);
    }
}

void mount_fat32() {
    // 1. decode the bpb
}

uint32_t totalClustersInDataRegion()
{
    uint32_t rootDirSectors = ((bpb.BPB_RootEntCnt * 32) + (bpb.BPB_BytsPerSec - 1)) / bpb.BPB_BytsPerSec;

    uint32_t fatSize = bpb.BPB_FATSz32 * bpb.BPB_NumFATs;

    uint32_t dataRegionSize = bpb.BPB_TotSec32 - (bpb.BPB_RsvdSecCnt + rootDirSectors) - fatSize; //+ fatSize

    uint32_t totalClusters = dataRegionSize / bpb.BPB_SecPerClus;
    return totalClusters; 
}

uint32_t calculate_first_data_sector() {
    // Sectors occupied by the reserved regions and the number of FATs
    uint32_t reservedSectors = bpb.BPB_RsvdSecCnt;
    uint32_t numberOfFATs = bpb.BPB_NumFATs; //2

    uint32_t rootDirSectors = ((bpb.BPB_RootEntCnt * 32) + (bpb.BPB_BytsPerSec - 1)) / bpb.BPB_BytsPerSec;

    uint32_t fatSize = bpb.BPB_FATSz32 * numberOfFATs;

    uint32_t firstDataSector = reservedSectors + fatSize + rootDirSectors;

    return firstDataSector;
}

uint32_t calculate_first_sect_cluster(uint32_t current_cluster_number)
{
    return (((current_cluster_number - 2)*bpb.BPB_SecPerClus)
        + calculate_first_data_sector()) ; //* bpb.BPB_BytsPerSec 
}

uint32_t entries_in_one_fat()
{
    uint32_t fatSizeBytes = bpb.BPB_FATSz32 * bpb.BPB_BytsPerSec;  // Assuming each sector is 512 bytes
    uint32_t numEntries = fatSizeBytes / sizeof(uint32_t);
    return numEntries;

}

long size_of_image()
{
    // Seek to the end of the file
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    return fileSize; 
}
/*sorta working*/
/*void cd_process1(int fat32_fd, uint32_t dir_offset, uint32_t dir_size)
{
    int fd = open("fat32.img", O_RDONLY);
    uint32_t current_offset = dir_offset; 
    printf("current offset = %u", current_offset);
    uint32_t rootDirOffsetCHECK = bpb.BPB_RsvdSecCnt * bpb.BPB_BytsPerSec;
    //prev_cwd = cwd;


    while (current_offset < dir_offset + dir_size) {
        dentry_t *dentry = encode_dir_entry(fat32_fd, current_offset);

        if(strcmp("..",tokens->items[1]) == 0)
        {
            printf("this worked?"); 
            cwd = convert_clus_num_to_offset_in_data_region(prev_cwd); 
                        fseek(file, cwd, SEEK_SET);
                        //lseek(fd, rootDirOffsetCHECK+(cluster_number-2)*512, SEEK_SET);
                        //printf("TESTTEST\n"); 
                        fread(&dentry, sizeof(dentry), 1, file);
                        //printf("MORE TESTS"); 
                        return; 
        }

        printf("DIR_Name[0] as hex: 0x%02X\n", dentry->DIR_Name[0]);
        printf("DIR_Name[0] as character: %s. \n", dentry->DIR_Name);

        // Check if the directory entry is valid
        if (dentry->DIR_Name[0] == 0x00) { // No more entries
            //printf("%s was found, so cd into it! \n ", dentry->DIR_Name);
            free(dentry);
            break;
        }
            char formatted_name[13];
            memset(formatted_name, 0, 13);

            int i = 0, j = 0;
            while (i < 11 && dentry->DIR_Name[i] != ' ' && dentry->DIR_Name[i] != '\0') {
                formatted_name[j++] = dentry->DIR_Name[i++];
            }
            formatted_name[j] = '\0';

            printf("%s. ", formatted_name);

        if (dentry->DIR_Name[0] != 0xE5 &&  // Entry is not deleted
            (dentry->DIR_Attr & 0x0F) != 0x0F) { // Entry is not a long filename
            // Print the directory entry
            if((dentry->DIR_Attr & 0x10) == 0x10)
            {
                printf("%s is a directory. \n", dentry->DIR_Name); 
                if (strcmp(formatted_name,tokens->items[1]) == 0)
                {
                    //prev_cwd = current_offset;
                    prev_cwd = cwd;
                    printf("%s was found, so cd into it! \n ", dentry->DIR_Name);
                    uint32_t cluster_number = ((uint32_t)dentry->DIR_FstClusHI << 16) | dentry->DIR_FstClusLO;
                    printf("cluster number: %u\n", cluster_number);
                    //current_dir_cluster = dentry->DIR_FstClusLO;
                    //printf("cluster number: %u\n", cluster_number);
                    strcat(cluster.cluster_path, tokens->items[1]);
                    strcat(cluster.cluster_path, "/");
                    //printf("cluster number: %u\n", cluster_number);
                    //printf("WHATWIABUSC"); 
                    //printf("TESTIDK"); 
                    //printf("cluster number: %u\n", rootDirOffsetCHECK);

                    cwd = convert_clus_num_to_offset_in_data_region(cluster_number); 
                    //prev_cwd = cwd;
                    fseek(file, cwd, SEEK_SET);
                    //lseek(fd, rootDirOffsetCHECK+(cluster_number-2)*512, SEEK_SET);
                    //printf("TESTTEST\n"); 
                    fread(&dentry, sizeof(dentry), 1, file);
                    //printf("MORE TESTS"); 
                    return; 
                }
            }
        }

        free(dentry);

        current_offset += sizeof(dentry_t);
    }
    printf("error: %s does not exist. \n ", tokens->items[1]);
    return; 
}*/

void remove_last_directory_from_path(char *path) {
    if (path == NULL || *path == '\0') {
        // Path is empty or NULL, nothing to do
        return;
    }

    size_t length = strlen(path);
    if (length == 1 && path[0] == '/') {
        // Root directory, nothing to remove
        return;
    }

    // Find the last occurrence of '/'
    char *last_slash = strrchr(path, '/');
    if (last_slash != NULL) {
        *last_slash = '\0'; // Truncate the path

        // Handle the special case when path becomes empty
        if (path[0] == '\0') {
            strcpy(path, "/");
        }
    }
}

void cd_process1(int fat32_fd, uint32_t dir_offset, uint32_t dir_size) {
    int fd = open("fat32.img", O_RDONLY);
    uint32_t current_offset = dir_offset; 
    printf("current offset = %u", current_offset);
    uint32_t rootDirOffsetCHECK = bpb.BPB_RsvdSecCnt * bpb.BPB_BytsPerSec;
    current_dir_cluster = cwd; 

    // Check for moving to the parent directory
    /*if (strcmp("..", tokens->items[1]) == 0) {
        printf("Moving to the parent directory...\n");
        if (current_dir_cluster != bpb.BPB_rootCluster) { // Not the root directory
            while (current_offset < dir_offset + dir_size) {
                dentry_t *dentry = encode_dir_entry(fat32_fd, prev_cwd);

                printf("pwd = %u\n", prev_cwd);
                if (strcmp("..", tokens->items[1]) == 0) {
                    cluster.depth--; 
                    uint32_t parent_cluster = ((uint32_t)dentry->DIR_FstClusHI << 16) | dentry->DIR_FstClusLO;
                    printf("TEST DIR_NAME: %s\n", dentry->DIR_Name);
                    //prev_cwd = current_offset;
                    cwd = convert_clus_num_to_offset_in_data_region(parent_cluster);
                    fseek(file, cwd, SEEK_SET);
                    fread(&dentry, sizeof(dentry), 1, file);

                    //free(dentry);
                    return;
                }

                free(dentry);
                current_offset += sizeof(dentry_t);
            }
            printf("Error: Parent directory not found.\n");
            return;
        } else {
            printf("Already in the root directory.\n");
            return;
        }
    }
    */
     if (strcmp("..", tokens->items[1]) == 0) {
        printf("Moving to the parent directory...\n");
        if (current_dir_cluster != bpb.BPB_rootCluster) { 
            // Read the directory entries in the current directory to find the parent directory entry
            while (current_offset < dir_offset + dir_size) {
                dentry_t *dentry = encode_dir_entry(fat32_fd, current_offset);

                if (dentry->DIR_Name[0] == 0x2E && dentry->DIR_Name[1] == 0x2E) { // Entry is ".."
                    uint32_t parent_cluster = ((uint32_t)dentry->DIR_FstClusHI << 16) | dentry->DIR_FstClusLO;

                    // Update the current working directory and cluster
                    cwd = convert_clus_num_to_offset_in_data_region(parent_cluster);
                    current_dir_cluster = parent_cluster;

                    // Update cluster_path to reflect moving to the parent directory
                    remove_last_directory_from_path(cluster.cluster_path);

                    fseek(file, cwd, SEEK_SET);
                    fread(&dentry, sizeof(dentry), 1, file);

                    free(dentry);
                    return;
                }

                free(dentry);
                current_offset += sizeof(dentry_t);
            }
            printf("Error: Parent directory not found.\n");
            return;
        } else {
            printf("Already in the root directory.\n");
            return;
        }
    }

    // Existing logic for changing to other directories
    while (current_offset < dir_offset + dir_size) {
        dentry_t *dentry = encode_dir_entry(fat32_fd, current_offset);

        printf("DIR_Name[0] as hex: 0x%02X\n", dentry->DIR_Name[0]);
        printf("DIR_Name[0] as character: %s. \n", dentry->DIR_Name);

        // Check if the directory entry is valid
        if (dentry->DIR_Name[0] == 0x00) { // No more entries
            //printf("%s was found, so cd into it! \n ", dentry->DIR_Name);
            free(dentry);
            break;
        }
            char formatted_name[13];
            memset(formatted_name, 0, 13);

            int i = 0, j = 0;
            while (i < 11 && dentry->DIR_Name[i] != ' ' && dentry->DIR_Name[i] != '\0') {
                formatted_name[j++] = dentry->DIR_Name[i++];
            }
            formatted_name[j] = '\0';

            printf("%s. ", formatted_name);

        if (dentry->DIR_Name[0] != 0xE5 &&  // Entry is not deleted
            (dentry->DIR_Attr & 0x0F) != 0x0F) { // Entry is not a long filename
            // Print the directory entry
            if((dentry->DIR_Attr & 0x10) == 0x10)
            {
                printf("%s is a directory. \n", dentry->DIR_Name); 
                if (strcmp(formatted_name,tokens->items[1]) == 0)
                {
                    //prev_cwd = current_offset;
                    prev_cwd = current_offset;
                    printf("prev_cwd = %u", prev_cwd);
                    printf("%s was found, so cd into it! \n ", dentry->DIR_Name);
                    uint32_t cluster_number = ((uint32_t)dentry->DIR_FstClusHI << 16) | dentry->DIR_FstClusLO;
                    printf("cluster number: %u\n", cluster_number);
                    //current_dir_cluster = dentry->DIR_FstClusLO;
                    //printf("cluster number: %u\n", cluster_number);
                    strcat(cluster.cluster_path, tokens->items[1]);
                    strcat(cluster.cluster_path, "/");
                    cluster.depth++; 
                    //printf("cluster number: %u\n", cluster_number);
                    //printf("WHATWIABUSC"); 
                    //printf("TESTIDK"); 
                    //printf("cluster number: %u\n", rootDirOffsetCHECK);

                    cwd = convert_clus_num_to_offset_in_data_region(cluster_number); 
                    //prev_cwd = cwd;
                    fseek(file, cwd, SEEK_SET);
                    //lseek(fd, rootDirOffsetCHECK+(cluster_number-2)*512, SEEK_SET);
                    //printf("TESTTEST\n"); 
                    fread(&dentry, sizeof(dentry), 1, file);
                    //printf("MORE TESTS"); 
                    return; 
                }
            }

            }

        free(dentry);
        current_offset += sizeof(dentry_t);
    }
    

    printf("Error: %s does not exist.\n", tokens->items[1]);
    return;
}





void cd_process(int fat32_fd, const char* target_dir, uint32_t dir_size) {
    printf("tokens->items[1] is set to:%s\n",tokens->items[1]);
    uint32_t current_offset = current_dir_cluster * bpb.BPB_SecPerClus * bpb.BPB_BytsPerSec;
    uint32_t end_offset = current_offset + dir_size;

    char formatted_target_dir[12]; 
    format_dir_name(target_dir, formatted_target_dir); 
    while (current_offset < end_offset) { //current_offset < dir_offset + dir_size
        printf("TESTEST\n");
        dentry_t *dentry = encode_dir_entry(fat32_fd, current_offset);
        printf("DIR_Name[0] as hex: 0x%02X\n", dentry->DIR_Name[0]);
        printf("DIR_Name[0] as character: %c.\n", dentry->DIR_Name[0]);


        if (dentry->DIR_Name[0] == 0x00) { // End of directory entries
            printf("TESTEST1\n");
            free(dentry);
            break;
        }
        printf("tokens->items[1] is set to: %s\n",tokens->items[1]);
        if (dentry->DIR_Name[0] != (char)0xE5 && (dentry->DIR_Attr & 0x10) == 0x10) { // It's a directory and not deleted
            char dirName[12] = {0};
            memcpy(dirName, dentry->DIR_Name, 11);

            format_dir_name(dirName, dirName); // Format the read directory name
            if (strcmp(formatted_target_dir, dirName) == 0) {
                // Change to the new directory
                printf("Changing directory to: %s\n", dirName); 
                current_dir_cluster = ((dentry->DIR_FstClusHI << 16) | dentry->DIR_FstClusLO);
                free(dentry);
                return;
            }
        }

        free(dentry);
        current_offset += sizeof(dentry_t);
    }

    printf("Directory '%s' not found.\n", target_dir);
}

void format_dir_name(const char *dir_name, char *formatted_name) {
    memset(formatted_name, 0, 12);

    int name_length = 0;
    for (int i = 0; i < 8; i++) { // First 8 characters for the name
        if (dir_name[i] != ' ') {
            formatted_name[name_length++] = dir_name[i];
        } else {
            break;
        }
    }
    bool has_extension = false;
    for (int i = 8; i < 11; i++) {
        if (dir_name[i] != ' ') {
            has_extension = true;
            break;
        }
    }
    if (has_extension) {
        formatted_name[name_length++] = '.';
        for (int i = 8; i < 11; i++) {
            if (dir_name[i] != ' ') {
                formatted_name[name_length++] = dir_name[i];
            } else {
                break;
            }
        }
    }
    formatted_name[name_length] = '\0';
}

void open_file(int fat32_fd, const char* filename, const char* mode) {
    if (num_opened_files >= MAX_OPEN_FILES) {
        printf("Error: Maximum number of open files reached.\n");
        return;
    }

    // Validate mode
    if (strcmp(mode, "-r") != 0 && strcmp(mode, "-w") != 0 &&
        strcmp(mode, "-rw") != 0 && strcmp(mode, "-wr") != 0) {
        printf("Error: Invalid mode '%s'.\n", mode);
        return;
    }
    // Check if the file is already open
    for (int i = 0; i < num_opened_files; i++) {
        if (strcmp(opened_files[i].filename, filename) == 0) {
            printf("Error: File '%s' is already open.\n", filename);
            return;
        }
    }
    // File found, add it to opened_files
            strncpy(opened_files[num_opened_files].filename, filename, 12);
            //opened_files[num_opened_files].first_cluster = ((dentry->DIR_FstClusHI << 16) | dentry->DIR_FstClusLO);
            //opened_files[num_opened_files].size = dentry->DIR_FileSize;
            opened_files[num_opened_files].current_offset = 0;
            strncpy(opened_files[num_opened_files].mode, mode, 3);
            num_opened_files++;

    printf("opened %s\n", filename); 
    printf("Error: File '%s' not found.\n", filename);
    printf("working?\n"); 
}

void main_process(char const *argv, uint32_t rootDirOffset) {
    char* input = "start";
    int fd = open("fat32.img", O_RDONLY);
    while (strcmp(input, "exit") != 0) {
        // 1. get cmd from input.
        showPrompt(argv);
        input = get_input(); 
        if (strcmp(input, "exit") == 0)
		{
			break;
		}

        tokens = get_tokens(input);


        // else if cmd is "cd" process_cd();
        if (strcmp(tokens->items[0], "cd") == 0)
        {
           cd_process1(fd, cwd, bpb.BPB_BytsPerSec); 

        } else if (strcmp(tokens->items[0], "ls") == 0)
        {
            read_directory(fd, cwd, bpb.BPB_BytsPerSec);

        }

       
        else if (strcmp(tokens->items[0], "open") == 0) {
            if (tokens->size < 3) {
                printf("Error: Missing filename or flags.\n");
            } else {
                open_file(fd, tokens->items[1], tokens->items[2]);
                printf("hello?\n"); 
            }
        }
        else if (strcmp(tokens->items[0], "lsof") == 0) {
            print_open_files(); 
        }

        else if (strcmp(tokens->items[0], "info") == 0)
        {
            printf("Bytes Per Sector: %u\n", bpb.BPB_BytsPerSec);
            printf("Sectors Per Cluster: %u\n", bpb.BPB_SecPerClus);

            printf("Total clusters in Data Region: %u\n", totalClustersInDataRegion()); 
            printf("# of entries in one FAT: %u\n", entries_in_one_fat());
            printf("Size of Image (bytes): %ld\n", size_of_image());
            printf("Root Cluster: %u\n", bpb.BPB_rootCluster);
            printf("FirstDataSector: %u\n", calculate_first_data_sector());
            printf("Total Sectors: %u\n", bpb.BPB_TotSec32);
        }
        else 
        {
            printf("command not found\n"); 
        }

        free_tokens(tokens);
        
    }
    close(fd);
}


// Function to list files in the root directory
void list_files(FILE *file, uint32_t rootDirOffset) {
    fseek(file, 0x4000, SEEK_SET);
    for (int entry_number = 0; entry_number < 16; ++entry_number) {
        dentry_t entry;
        fread(&entry, sizeof(dentry_t), 1, file);

        // Check if the entry is valid
        if (entry.DIR_Name[0] == 0x00 || entry.DIR_Name[0] == 0xE5 || entry.DIR_Attr == 0x0F) {
            continue;  // Skip invalid entries
        }
        printf("Name: %s\n", entry.DIR_Name);
        printf("Attributes: 0x%x\n", entry.DIR_Attr);
        printf("File Size: %u bytes\n", entry.DIR_FileSize);

        printf("\n");
    }
}


int main(int argc, char const *argv[])
{
    file = fopen(argv[1], "rb");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }
  
    fseek(file, 0, SEEK_SET);

    fread(&bpb, sizeof(bpb_t), 1, file);
    int fd = open("fat32.img", O_RDONLY);
    cluster.depth = 0; 
    cluster.offset = 0;
    cluster.curr_clus_num = 2;
    cluster.next_clus_num = 0;
    cluster.BPB_SecPerClus = 1;
    cluster.BPB_FATSz32 = 1009;
    cluster.end_clus_num = 0xffffffff;
    cluster.max_clus_num = bpb.BPB_FATSz32;
    cluster.min_clus_num = 2;
    strcat(cluster.cluster_path, "/");

    uint32_t clusterSize = bpb.BPB_BytsPerSec * bpb.BPB_SecPerClus;

    //fat_region_offfset
    uint32_t rootDirOffsetCHECK = bpb.BPB_RsvdSecCnt * bpb.BPB_BytsPerSec;
    uint32_t rootDirCluster = bpb.BPB_rootCluster;
    current_dir_cluster = rootDirCluster;  

    // Calculate the offset in bytes
    //data_region_offset
    uint32_t rootDirOffset = ((rootDirCluster - 2) * bpb.BPB_SecPerClus + bpb.BPB_RsvdSecCnt +
                             bpb.BPB_NumFATs * bpb.BPB_FATSz32) *
                             bpb.BPB_BytsPerSec;
    
    fseek(file, rootDirOffset, SEEK_SET);

    cwd = rootDirOffset; 

    printf("rootdiroffset: %u", rootDirOffset);

    main_process(argv[1], rootDirOffset); 

    fclose(file);

    return 0;
}