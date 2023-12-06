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
    uint32_t BPB_FATSz32; //chat
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
    char BS_FilSysType[8]; //chat
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

uint32_t current_dir_cluster = 0; //chat

void format_dir_name(const char *dir_name, char *formatted_name); //chat


void showPrompt(char const *argv)
{
    char* str1 = "/> "; 
    //char* str2 = getenv("PWD");
    printf("\n%s%s", argv, str1); 
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

void dbg_print_dentry(dentry_t *dentry) {
    if (dentry == NULL) {
        return ;
    }

    printf("%s ", dentry->DIR_Name);
    //printf("DIR_Name: %s\n", dentry->DIR_Name);
    /*printf("DIR_Attr: 0x%x\n", dentry->DIR_Attr);
    printf("DIR_FstClusHI: 0x%x\n", dentry->DIR_FstClusHI);
    printf("DIR_FstClusLO: 0x%x\n", dentry->DIR_FstClusLO);
    printf("DIR_FileSize: %u\n", dentry->DIR_FileSize);*/
}

// This is just an example and pseudocode. The real logic may different from
// what is shown here.
// This function returns one directory entry.
dentry_t *encode_dir_entry(int fat32_fd, uint32_t offset) {
    dentry_t *dentry = (dentry_t*)malloc(sizeof(dentry_t));
    ssize_t rd_bytes = pread(fat32_fd, (void*)dentry, sizeof(dentry_t), offset);
    
    // omitted: check rd_bytes == sizeof(dentry_t)

    return dentry;
}

void read_directory(int fat32_fd, uint32_t dir_offset, uint32_t dir_size) {
    /*uint32_t current_offset = dir_offset;

    while (current_offset < dir_offset + dir_size) {
        dentry_t *dentry = encode_dir_entry(fat32_fd, current_offset);
        dbg_print_dentry(dentry);
        free(dentry);

        // Move to the next directory entry
        current_offset += sizeof(dentry_t);
    }
    */
    uint32_t current_offset = dir_offset;

    while (current_offset < dir_offset + dir_size) {
        dentry_t *dentry = encode_dir_entry(fat32_fd, current_offset);

        // Check if the directory entry is valid
        if (dentry->DIR_Name[0] == 0x00) { // No more entries
            free(dentry);
            break;
        }

        if (dentry->DIR_Name[0] != 0xE5 &&  // Entry is not deleted
            (dentry->DIR_Attr & 0x0F) != 0x0F) { // Entry is not a long filename
            // Print the directory entry
            dbg_print_dentry(dentry);
        }


        free(dentry);

        // Move to the next directory entry
        current_offset += sizeof(dentry_t);
    }

    printf("\n"); 

}

// other data structure, global variables, etc. define them in need.
// e.g., 
// the opened fat32.img file
// the current working directory
// the opened files
// other data structures and global variables you need

bpb_t bpb;
tokenlist* tokens; 
FILE *file;

// you can give it another name
// fill the parameters
void mount_fat32() {
    // 1. decode the bpb
}

uint32_t totalClustersInDataRegion()
{
    // Sectors occupied by the root directory (root entries * size of a directory entry)
    uint32_t rootDirSectors = ((bpb.BPB_RootEntCnt * 32) + (bpb.BPB_BytsPerSec - 1)) / bpb.BPB_BytsPerSec;

    // Calculate the sectors occupied by the FAT tables
    uint32_t fatSize = bpb.BPB_FATSz32 * bpb.BPB_NumFATs;

    // Calculate the data region size in sectors
    uint32_t dataRegionSize = bpb.BPB_TotSec32 - (bpb.BPB_RsvdSecCnt + rootDirSectors) - fatSize; //+ fatSize

    // Calculate the total number of clusters
    uint32_t totalClusters = dataRegionSize / bpb.BPB_SecPerClus;
    return totalClusters; 
    
}

uint32_t calculate_first_data_sector() {
    // Sectors occupied by the reserved regions and the number of FATs
    uint32_t reservedSectors = bpb.BPB_RsvdSecCnt;
    uint32_t numberOfFATs = bpb.BPB_NumFATs; //2

    // Sectors occupied by the root directory (root entries * size of a directory entry)
    uint32_t rootDirSectors = ((bpb.BPB_RootEntCnt * 32) + (bpb.BPB_BytsPerSec - 1)) / bpb.BPB_BytsPerSec;

    // Calculate the sectors occupied by the FAT tables
    uint32_t fatSize = bpb.BPB_FATSz32 * numberOfFATs;

    // Calculate the first data sector
    uint32_t firstDataSector = reservedSectors + fatSize + rootDirSectors;

    return firstDataSector;
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

    // Get the current position, which is the size of the file
    long fileSize = ftell(file);
    return fileSize; 
}


/*void cd_process(int fat32_fd, uint32_t dir_offset, uint32_t dir_size)
{
    uint32_t current_offset = dir_offset; 

    char *charPointer;

    while (current_offset < dir_offset + dir_size) {
        dentry_t *dentry = encode_dir_entry(fat32_fd, current_offset);
        //dentry->DIR_Name[10] = '\0';

        charPointer = malloc(strlen(dentry->DIR_Name) + 1);  // +1 for the null terminator
        if (charPointer != NULL) {
        strcpy(charPointer, dentry->DIR_Name);

        // Now charPointer is a null-terminated string
        printf("Converted string: %s\n", charPointer);

        // Don't forget to free the allocated memory when done
        
    }

        // Check if the directory entry is valid
        if (dentry->DIR_Name[0] == 0x00) { // No more entries
            //printf("%s was found, so cd into it! \n ", dentry->DIR_Name);
            free(dentry);
            break;
        }

        
        if (dentry->DIR_Name[0] != 0xE5 &&  // Entry is not deleted
            (dentry->DIR_Attr & 0x0F) != 0x0F) { // Entry is not a long filename
            // Print the directory entry
            if((dentry->DIR_Attr & 0x10) == 0x10)
            {
                printf("%s is a directory. \n", dentry->DIR_Name); 
                if (strcmp(charPointer,"BLUE") == 0)
                {
                    printf("%s was found, so cd into it! \n ", dentry->DIR_Name);
                    //dbg_print_dentry(dentry);
                    return; 
                }
            }
        }
        //printf("%s was NOT found \n ", dentry->DIR_Name);


        free(dentry);
        free(charPointer);

        // Move to the next directory entry
        current_offset += sizeof(dentry_t);
    }
    printf("%s was NOT found \n ", tokens->items[1]);
    return; 
}*/


/* 
void cd_process(int fat32_fd, const char* target_dir, uint32_t dir_size) {
    uint32_t current_offset = current_dir_cluster; // start from the current directory

    while (current_offset < dir_size + current_offset) {
        dentry_t *dentry = encode_dir_entry(fat32_fd, current_offset);

        if (dentry->DIR_Name[0] == 0x00) { // End of directory entries
            free(dentry);
            break;
        }

        if (dentry->DIR_Name[0] != (char)0xE5 && (dentry->DIR_Attr & 0x10) == 0x10)  { // It's a directory and not deleted
            char formatted_name[12];
            format_dir_name(dentry->DIR_Name, formatted_name); // You need to implement this function to format the name

            if (strcmp(formatted_name, target_dir) == 0) {
                // Change to the new directory
                current_dir_cluster = dentry->DIR_FstClusLO;
                free(dentry);
                return;
            }
        }

        free(dentry);
        current_offset += sizeof(dentry_t);
    }

    printf("Directory '%s' not found.\n", target_dir);
}
//chat's version

void cd_process(int fat32_fd, const char* target_dir, uint32_t dir_size) {
    uint32_t start_offset = current_dir_cluster; // Assuming current_dir_cluster is the starting cluster number
    uint32_t current_offset = start_offset; // start from the current directory

    while (current_offset < start_offset + dir_size) {
        dentry_t *dentry = encode_dir_entry(fat32_fd, current_offset);

        if (dentry->DIR_Name[0] == 0x00) { // End of directory entries
            free(dentry);
            break;
        }

        if (dentry->DIR_Name[0] != (char)0xE5 && (dentry->DIR_Attr & 0x10) == 0x10)  { // It's a directory and not deleted
            char formatted_name[12];
            //format_dir_name(dentry->DIR_Name, formatted_name); // Format the name for comparison
            //printf("Formatted Name: %s\n", formatted_name);
            

            if (strcmp(dentry->DIR_Name, target_dir) == 0) {
                // Change to the new directory
                current_dir_cluster = dentry->DIR_FstClusLO;
                free(dentry);
                return;
            }
        }
        
        free(dentry);
        current_offset += sizeof(dentry_t); // Move to the next directory entry
    }

    printf("Directory '%s' not found.\n", target_dir);
}

*/

//old latest version 
/*void cd_process(int fat32_fd, const char* target_dir, uint32_t dir_size) {
    uint32_t start_offset = current_dir_cluster; // Assuming current_dir_cluster is the starting cluster number
    uint32_t current_offset = start_offset; // start from the current directory

    while (current_offset < start_offset + dir_size) {
        dentry_t *dentry = encode_dir_entry(fat32_fd, current_offset);
        printf("DIR_Name: %s\n", dentry->DIR_Name);

        if (dentry->DIR_Name[0] == 0x00) { // End of directory entries
            free(dentry);
            break;
        }

        tokenlist* dname; 
        dname = get_tokens(dentry->DIR_Name);*/

        /*if (dentry->DIR_Name[0] != (char)0xE5 && (dentry->DIR_Attr & 0x10) == 0x10) { // It's a directory and not deleted
            // Note: DIR_Name is not null-terminated, so we need to handle it carefully
            char dirName[12];
            memset(dirName, 0, sizeof(dirName)); // Initialize with zeros
            strncpy(dirName, dentry->DIR_Name, sizeof(dentry->DIR_Name));
            //printf("DOES THIS WORK \n"); 
            if (strcmp(dentry->DIR_Name, tokens->items[1]) == 0) {
                // Change to the new directory
                printf("DOES THIS WORK \n"); 
                current_dir_cluster = dentry->DIR_FstClusLO;
                free(dentry);
                return;
            }*/

            /*if (strncmp(dirName, target_dir, strlen(target_dir)) == 0) {
                // Change to the new directory
                current_dir_cluster = dentry->DIR_FstClusLO;
                free(dentry);
                return;
            }*/
       // }

   //     free(dentry);
//        current_offset += sizeof(dentry_t); // Move to the next directory entry
  //  }

 //   printf("Directory '%s' not found.\n", target_dir);
 //latest version 
//}

void cd_process(int fat32_fd, const char* target_dir, uint32_t dir_size) {
    printf("tokens->items[1] is set to:%s.\n",tokens->items[1]);
    uint32_t current_offset = current_dir_cluster * bpb.BPB_SecPerClus * bpb.BPB_BytsPerSec;
    uint32_t end_offset = current_offset + dir_size;

    while (current_offset < end_offset) { //current_offset < dir_offset + dir_size
        printf("TESTEST\n");
        dentry_t *dentry = encode_dir_entry(fat32_fd, current_offset);

        if (dentry->DIR_Name[0] == 0x00) { // End of directory entries
            printf("TESTEST1\n");
            free(dentry);
            break;
        }
        printf("tokens->items[1] is set to: %s\n",tokens->items[1]);
        if (dentry->DIR_Name[0] != (char)0xE5 && (dentry->DIR_Attr & 0x10) == 0x10) { // It's a directory and not deleted
            char dirName[12] = {0};
            memcpy(dirName, dentry->DIR_Name, 11);

            // Transform directory name to uppercase for comparison
            for (int i = 0; i < 11; i++) {
                dirName[i] = toupper(dirName[i]);
            }
            //printf("tokens->items[1] is set to: %s\n",tokens->items[1]);
            if (strcmp("BLUE", tokens->items[1]) == 0) {
                // Change to the new directory
                printf("DOES THIS WORK BLUE \n"); 
                current_dir_cluster = dentry->DIR_FstClusLO;
                free(dentry);
                return;
            }

            if (strcmp("RED", tokens->items[1]) == 0) {
                // Change to the new directory
                printf("DOES THIS WORK RED\n"); 
                current_dir_cluster = dentry->DIR_FstClusLO;
                free(dentry);
                return;
            }

            if (strcmp("GREEN", tokens->items[1]) == 0) {
                // Change to the new directory
                printf("DOES THIS WORK GREEN\n"); 
                current_dir_cluster = dentry->DIR_FstClusLO;
                free(dentry);
                return;
            }

            // Compare with the target directory
            /*if (strncmp(dirName, target_dir, strlen(target_dir)) == 0) {
                current_dir_cluster = ((dentry->DIR_FstClusHI << 16) | dentry->DIR_FstClusLO);
                free(dentry);
                return;
            }*/
        }

        free(dentry);
        current_offset += sizeof(dentry_t);
    }

    printf("Directory '%s' not found.\n", target_dir);
}



void format_dir_name(const char *dir_name, char *formatted_name) {
    // Clear the formatted name buffer
    memset(formatted_name, 0, 12);

    int name_length = 0;
    for (int i = 0; i < 8; i++) { // First 8 characters for the name
        if (dir_name[i] != ' ') {
            formatted_name[name_length++] = dir_name[i];
        } else {
            break;
        }
    }

    // Check if there is an extension
    bool has_extension = false;
    for (int i = 8; i < 11; i++) {
        if (dir_name[i] != ' ') {
            has_extension = true;
            break;
        }
    }

    // If there is an extension, add a period and then the extension
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

    // Null-terminate the string
    formatted_name[name_length] = '\0';
}

// you can give it another name
// fill the parameters
void main_process(char const *argv, uint32_t rootDirOffset) {
    char* input = "start";
    int fd = open("fat32.img", O_RDONLY);
    while (strcmp(input, "exit") != 0) {
        // 1. get cmd from input.
        showPrompt(argv);
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

           //cd_process(fd, rootDirOffset, bpb.BPB_BytsPerSec); 
            cd_process(fd, tokens->items[1], bpb.BPB_BytsPerSec); 
            //close(fd);


        } else if (strcmp(tokens->items[0], "ls") == 0)
        {
            //process_ls();
            //int fd = open("fat32.img", O_RDONLY);
            //uint32_t offset = 0x100420;

            uint32_t dir_offset = 0x100420;  // Adjust the directory offset as needed
            uint32_t dir_size = 512;         // Adjust the directory size as needed

            read_directory(fd, rootDirOffset, bpb.BPB_BytsPerSec);

            
           // close(fd);
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
        
    }
    close(fd);
}

int main(int argc, char const *argv[])
{
    // 1. open the fat32.img
    //const char *imagePath = "../fat32.img";

    file = fopen(argv[1], "rb");
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
    current_dir_cluster = rootDirCluster;  //chat 

    // Calculate the offset in bytes
    uint32_t rootDirOffset = ((rootDirCluster - 2) * bpb.BPB_SecPerClus + bpb.BPB_RsvdSecCnt +
                             bpb.BPB_NumFATs * bpb.BPB_FATSz32) *
                             bpb.BPB_BytsPerSec;

    // Seek to the beginning of the root directory
    fseek(file, rootDirOffset, SEEK_SET);

    main_process(argv[1], rootDirOffset); 


    
    // 3. main procees
    

    // 4. close all opened files
    
    // 5. close the fat32.img
    fclose(file);

    return 0;
}
