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
    char filename[12]; //name of the file
    uint32_t first_cluster; //first cluster of the file
    uint32_t current_offset; //current read/write offset in the file
    uint32_t size; //size of the file in bytes
    char mode[4]; //mode in which the file is opened (-r, -w, -rw, -wr)
    char path[100]; 
} open_file_t;


typedef struct __attribute__((packed)) directory_entry {
    char DIR_Name[11];
    uint8_t DIR_Attr;
    char padding_1[8]; 
    uint16_t DIR_FstClusHI;
    char padding_2[4]; 
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
    char **cluster_path_array; 
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
dentry_t *prev_dentry; 
uint32_t path_depth[10]; 
int path_depth_size; 


#define MAX_OPEN_FILES 10
open_file_t opened_files[MAX_OPEN_FILES]; //array to store opened files
int num_opened_files = 0;

uint32_t convert_offset_to_clus_num_in_fat_region(uint32_t offset) { 
    uint32_t fat_region_offset = bpb.BPB_RsvdSecCnt * bpb.BPB_BytsPerSec;
    return (offset - fat_region_offset)/4;
}

uint32_t convert_clus_num_to_offset_in_fat_region(uint32_t clus_num) {
    uint32_t fat_region_offset = bpb.BPB_RsvdSecCnt * bpb.BPB_BytsPerSec;
    return fat_region_offset + clus_num * 4;
}


uint32_t convert_clus_num_to_offset_in_data_region(uint32_t clus_num) {
    uint32_t clus_size = bpb.BPB_BytsPerSec;
    uint32_t rootDirCluster = bpb.BPB_rootCluster;
    uint32_t data_region_offset = ((rootDirCluster - 2) * bpb.BPB_SecPerClus + bpb.BPB_RsvdSecCnt +
                             bpb.BPB_NumFATs * bpb.BPB_FATSz32) *
                             bpb.BPB_BytsPerSec;
    return data_region_offset + (clus_num - 2) * clus_size;
}

void showPrompt(char const *argv)
{
    char* etc = "> ";
    printf("\n%s", argv);
    for (int i = 0; i < cluster.depth; i++)
    { 
        printf("%s", cluster.cluster_path_array[i]);
    }
    
    printf("%s", etc);
}


void print_open_files()
{
    //int index = 0; 
    printf(" %-7s  %-12s  %-10s %-10s  %-8s\n", "INDEX", "NAME", "MODE", "OFFSET", "PATH");
    for (int i = 0; i < num_opened_files; i++)
    {
        printf(" %-7d  %-12s  %-10s %-10d  %-8s\n", i, 
        opened_files[i].filename, opened_files[i].mode, 
        opened_files[i].current_offset, opened_files[i].path);
    }
    
    printf("\n"); 
}

void trim_spaces(char *str) {
    int len = strlen(str);
    int start = 0, end = len - 1;

    while (isspace((unsigned char)str[start])) { //trim leading spaces
        start++;
    }

    while (end >= start && isspace((unsigned char)str[end])) { //trim trailing spaces
        str[end--] = '\0';
    }
    
    if (start > 0) {//if leading spaces, shift remaining characters to the beginning of the string
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

        //check if the directory entry is valid
        if (dentry->DIR_Name[0] == 0x00) { //no more entries
            free(dentry);
            break;
        }
        if (dentry->DIR_Name[0] != 0xE5 &&  //entry is not deleted
            ((dentry->DIR_Attr & 0x0F) != 0x0F) &&
            (dentry->DIR_Attr & 0x02) != 0x02) { 

            //remove spaces from the directory name
            char formatted_name[13];
            memset(formatted_name, 0, 13);

            int i = 0, j = 0;
            while (i < 11 && dentry->DIR_Name[i] != ' ' && dentry->DIR_Name[i] != '\0') {
                formatted_name[j++] = dentry->DIR_Name[i++];
            }
            formatted_name[j] = '\0';

            printf("%s ", formatted_name);
        }
        free(dentry);
        //move to the next directory entry
        current_offset += sizeof(dentry_t);
    }
}


uint32_t totalClustersInDataRegion()
{
    uint32_t rootDirSectors = ((bpb.BPB_RootEntCnt*32)+(bpb.BPB_BytsPerSec-1))/bpb.BPB_BytsPerSec;

    uint32_t fatSize = bpb.BPB_FATSz32 * bpb.BPB_NumFATs;

    uint32_t dataRegionSize = bpb.BPB_TotSec32 - (bpb.BPB_RsvdSecCnt + rootDirSectors) - fatSize;

    uint32_t totalClusters = dataRegionSize / bpb.BPB_SecPerClus;
    return totalClusters; 
}

uint32_t calculate_first_data_sector() {
    //sectors occupied by the reserved regions and the number of FATs
    uint32_t reservedSectors = bpb.BPB_RsvdSecCnt;
    uint32_t numberOfFATs = bpb.BPB_NumFATs; //2

    uint32_t rootDirSectors = ((bpb.BPB_RootEntCnt*32)+(bpb.BPB_BytsPerSec-1))/bpb.BPB_BytsPerSec;

    uint32_t fatSize = bpb.BPB_FATSz32 * numberOfFATs;

    uint32_t firstDataSector = reservedSectors + fatSize + rootDirSectors;

    return firstDataSector;
}

uint32_t calculate_first_sect_cluster(uint32_t current_cluster_number)
{
    return (((current_cluster_number - 2)*bpb.BPB_SecPerClus)
        + calculate_first_data_sector()) ;
}

uint32_t entries_in_one_fat()
{
    uint32_t fatSizeBytes = bpb.BPB_FATSz32 * bpb.BPB_BytsPerSec;
    uint32_t numEntries = fatSizeBytes / sizeof(uint32_t);
    return numEntries;

}

long size_of_image()
{
    //seek to the end of the file
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    return fileSize; 
}

void remove_last_directory_from_path(char *path) {
    if (path == NULL || *path == '\0') {
        //path is empty or NULL
        return;
    }

    size_t length = strlen(path);
    if (length == 1 && path[0] == '/') {
        //root directory
        return;
    }

    //finding last occurrence of '/'
    char *last_slash = strrchr(path, '/');
    if (last_slash != NULL) {
        *last_slash = '\0'; 

        //handle the special case when path becomes empty
        if (path[0] == '\0') {
            strcpy(path, "/");
        }
    }
}

void cd_process1(int fat32_fd, uint32_t dir_offset, uint32_t dir_size) {
    //int fd = open("fat32.img", O_RDONLY);
    uint32_t current_offset = dir_offset; 
    //uint32_t rootDirOffsetCHECK = bpb.BPB_RsvdSecCnt * bpb.BPB_BytsPerSec;
    current_dir_cluster = cwd; 

    if (strcmp(".", tokens->items[1]) == 0) {
        return; 
    }
    //check for moving to the parent directory
    if (strcmp("..", tokens->items[1]) == 0) {
        if (current_dir_cluster != bpb.BPB_rootCluster) { //not the root directory
            while (current_offset < dir_offset + dir_size) {
                dentry_t *dentry = encode_dir_entry(fat32_fd, cwd);

                if (strcmp("..", tokens->items[1]) == 0) {
                    cluster.depth = cluster.depth-2; 
                    path_depth_size--;

                    uint32_t cluster_number = path_depth[path_depth_size-1];

                    cwd = convert_clus_num_to_offset_in_data_region(cluster_number);
                    fseek(file, cwd, SEEK_SET);
                    fread(&dentry, sizeof(dentry), 1, file);
                    return;
                }

                free(dentry);
                current_offset += sizeof(dentry_t);
            }
            printf("Error: Parent directory not found.\n");
            return;
        } else {
            //printf("Already in the root directory.\n");
            return;
        }
    }
    

    while (current_offset < dir_offset + dir_size) {
        dentry_t *dentry = encode_dir_entry(fat32_fd, current_offset);

        //check if the directory entry is valid
        if (dentry->DIR_Name[0] == 0x00) { //no more entries
            free(dentry);
            break;
        }
            char *formatted_name = (char*)malloc(13);

            int i = 0, j = 0;
            while (i < 11 && dentry->DIR_Name[i] != ' ' && dentry->DIR_Name[i] != '\0') {
                formatted_name[j++] = dentry->DIR_Name[i++];
            }
            formatted_name[j] = '\0';

            //printf("%s. ", formatted_name);

        if (dentry->DIR_Name[0] != 0xE5 &&  //entry not deleted
            (dentry->DIR_Attr & 0x0F) != 0x0F) { // entry is not a long filename
            //print the directory entry
            if((dentry->DIR_Attr & 0x10) == 0x10)
            {
                //printf("%s is a directory. \n", dentry->DIR_Name); 
                if (strcmp(formatted_name,tokens->items[1]) == 0)
                {
                    prev_cwd = current_offset;  
                    uint32_t cluster_number = ((uint32_t)dentry->DIR_FstClusHI << 16) |
                                                 dentry->DIR_FstClusLO;
                    //printf("cluster number: %u\n", cluster_number);
                    path_depth[path_depth_size++] = cluster_number;
                    strcat(cluster.cluster_path, tokens->items[1]);
                    strcat(cluster.cluster_path, "/");
                    cluster.cluster_path_array[cluster.depth++] = formatted_name; 
                    cluster.cluster_path_array[cluster.depth++] = "/"; 

                    cwd = convert_clus_num_to_offset_in_data_region(cluster_number); 
                    fseek(file, cwd, SEEK_SET); 
                    fread(&dentry, sizeof(dentry), 1, file);

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

void open_file(int fat32_fd, const char* filename, const char* mode, char const *argv) {
    if (num_opened_files >= MAX_OPEN_FILES) {
        printf("Error: Maximum number of open files reached.\n");
        return;
    }

    //validate mode
    if (strcmp(mode, "-r") != 0 && strcmp(mode, "-w") != 0 &&
        strcmp(mode, "-rw") != 0 && strcmp(mode, "-wr") != 0) {
        printf("Error: Invalid mode '%s'.\n", mode);
        return;
    }

    //check if file is already open
    for (int i = 0; i < num_opened_files; i++) {
        if (strcmp(opened_files[i].filename, filename) == 0) {
            printf("Error: File '%s' is already open.\n", filename);
            return;
        }
    }
    
    uint32_t current_offset = cwd;
    bool file_found = false;
    dentry_t *dentry;

    //search for the file in the directory entries
    while (true) {
        dentry = encode_dir_entry(fat32_fd, current_offset);

        if (dentry->DIR_Name[0] == 0x00) { //no more entries
            free(dentry);
            break;
        }

        if (strncmp(dentry->DIR_Name, filename, 11) == 0 && (dentry->DIR_Attr & 0x10) != 0x10) { 
            //file found and not a directory
            file_found = true;
            break;
        }

        current_offset += sizeof(dentry_t);
    }

    if (!file_found) {
        printf("Error: File '%s' not found or is a directory.\n", filename);
        return;
    }

    //file not found, add it to opened_files
    strncpy(opened_files[num_opened_files].filename, filename, 12);
    opened_files[num_opened_files].first_cluster = ((uint32_t)dentry->DIR_FstClusHI << 16) |
                                                         dentry->DIR_FstClusLO;
    opened_files[num_opened_files].size = dentry->DIR_FileSize;
    opened_files[num_opened_files].current_offset = 0;
    strcat(opened_files[num_opened_files].path, argv); 
    for (int i = 0; i < cluster.depth; i++)
    {
        if (cluster.depth == 1) {
            break; 
        }
        //printf("%s", cluster.cluster_path_array[i]);
        strcat(opened_files[num_opened_files].path, cluster.cluster_path_array[i]);
    }

    strncpy(opened_files[num_opened_files].mode, mode, 4);
    num_opened_files++;

    printf("Opened file '%s'.\n", filename); 
    free(dentry);
}

void lseek_file(const char* filename, uint32_t new_offset) {
    int file_found = 0;

    //search for the file in the list of opened files
    for (int i = 0; i < num_opened_files; i++) {
        if (strcmp(opened_files[i].filename, filename) == 0) {
            file_found = 1;

            //check if the new offset is within the file size
            if (new_offset > opened_files[i].size) {
                printf("Error: Offset is larger than the size of the file '%s'.\n", filename);
                return;
            }

            //set the new offset
            opened_files[i].current_offset = new_offset;
            printf("Offset of file '%s' set to %u.\n", filename, new_offset);
            return;
        }
    }

    if (!file_found) {
        printf("Error: File '%s' is not opened or does not exist.\n", filename);
    }
}

void close_file(const char* filename) {
    int found = 0;

    //search for the file in list of opened files
    for (int i = 0; i < num_opened_files; i++) {
        if (strcmp(opened_files[i].filename, filename) == 0) {
            //file found, remove entry from the list
            found = 1;
            for (int j = i; j < num_opened_files - 1; j++) {
                opened_files[j] = opened_files[j + 1];
            }
            num_opened_files--;
            printf("Closed file '%s'.\n", filename);
            break;
        }
    }

    //if file was not found in opened files
    if (!found) {
        printf("Error: File '%s' is not opened or does not exist in the current working directory.\n"
                    , filename);
    }
}

void read_file(const char* filename, int size) {
    //find file in the list of opened files
    for (int i = 0; i < num_opened_files; i++) {
        if (strcmp(opened_files[i].filename, filename) == 0) {
            //check if file is open for reading
            if (strcmp(opened_files[i].mode, "-r") != 0 && strcmp(opened_files[i].mode,"-rw")!=0){
                printf("Error: File '%s' is not opened for reading.\n", filename);
                return;
            }

            //get file's first cluster number
            uint32_t cluster_number = opened_files[i].first_cluster;

            //calculate the file's position in the data region
            uint32_t file_offset = convert_clus_num_to_offset_in_data_region(cluster_number);

            //open image file
            FILE* img_file = fopen("fat32.img", "rb");
            if (img_file == NULL) {
                printf("Error: Unable to open FAT32 image file.\n");
                return;
            }

            //allocate memory for reading
            char* buffer = (char*)malloc(size + 1);
            if (buffer == NULL) {
                printf("Error: Memory allocation failed.\n");
                fclose(img_file);
                return;
            }

            //move to the correct position in the file
            fseek(img_file, file_offset + opened_files[i].current_offset, SEEK_SET);

            //read data
            int bytesRead = fread(buffer, 1, size, img_file);
            buffer[bytesRead] = '\0'; //null-terminate the buffer

            //print read data
            printf("Data read from file '%s':\n%s\n", filename, buffer);

            //update file offset
            opened_files[i].current_offset += bytesRead;

            //free allocated memory and close image file
            free(buffer);
            fclose(img_file);

            return;
        }
    }

    printf("Error: File '%s' does not exist or is not opened.\n", filename);
}


void main_process(char const *argv) {
    char* input = "start";
    int fd = open("fat32.img", O_RDONLY);
    //allows exit command
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
        //open file
        else if (strcmp(tokens->items[0], "open") == 0) {
            if (tokens->size < 3) {
                printf("Error: Missing filename or flags.\n");
            } else {
                open_file(fd, tokens->items[1], tokens->items[2], argv);
            }
        }
        else if (strcmp(tokens->items[0], "lsof") == 0) {
            print_open_files(); 
        }
        //changing offset using lseek
        else if (strcmp(tokens->items[0], "lseek") == 0) {
            if (tokens->size < 3) {
                printf("Error: Missing filename or offset for lseek command.\n");
            } else {
                const char* filename = tokens->items[1];
                uint32_t offset = atoi(tokens->items[2]); //convert offset string to integer
                lseek_file(filename, offset);
            }
        }
        //close file
        else if (strcmp(tokens->items[0], "close") == 0) {
            if (tokens->size < 2) {
                printf("Error: Missing filename for close command.\n");
            } else {
            close_file(tokens->items[1]);
            }
        }
        //reading contents of file
        else if (strcmp(tokens->items[0], "read") == 0) {
            if (tokens->size < 3) {
                printf("Error: Missing filename or size for read command.\n");
            } else {
                // Extract filename and size from tokens->items[1] and tokens->items[2]
                const char* filename = tokens->items[1];
                int size = atoi(tokens->items[2]); // Convert size string to integer

                read_file(filename, size);
            }
        }
        //obtaining system info
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
        //command does not exist
        else 
        {
            printf("command not found\n"); 
        }

        free_tokens(tokens);
        
    }
    close(fd);
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
    //int fd = open("fat32.img", O_RDONLY);
    //used for showing prompt
    cluster.depth = 0; 
    cluster.offset = 0;
    cluster.curr_clus_num = bpb.BPB_rootCluster;
    cluster.next_clus_num = 0;
    cluster.BPB_SecPerClus = bpb.BPB_SecPerClus;
    cluster.BPB_FATSz32 = 1009;
    cluster.end_clus_num = 0xffffffff;
    cluster.max_clus_num = bpb.BPB_FATSz32;
    cluster.min_clus_num = bpb.BPB_rootCluster;
    cluster.cluster_path_array = (char **)malloc(100 * sizeof(char *));
    strcat(cluster.cluster_path, "/");
     cluster.cluster_path_array[cluster.depth++] = "/"; 

    //used for cd .. and cd DIR
    path_depth_size = 0; 


    //uint32_t clusterSize = bpb.BPB_BytsPerSec * bpb.BPB_SecPerClus;

    
    path_depth[path_depth_size++] = bpb.BPB_rootCluster; 

    //fat_region_offfset
    uint32_t rootDirOffsetCHECK = bpb.BPB_RsvdSecCnt * bpb.BPB_BytsPerSec;
    uint32_t rootDirCluster = bpb.BPB_rootCluster;
    current_dir_cluster = rootDirCluster;  

    //calculate the offset in bytes
    //data_region_offset
    uint32_t rootDirOffset = ((rootDirCluster - 2) * bpb.BPB_SecPerClus + bpb.BPB_RsvdSecCnt +
                             bpb.BPB_NumFATs * bpb.BPB_FATSz32) *
                             bpb.BPB_BytsPerSec;
          
    
    fseek(file, rootDirOffset, SEEK_SET);

    cwd = rootDirOffset; 

    main_process(argv[1]); 

    
    free(cluster.cluster_path_array);
    

    fclose(file);

    return 0;
}