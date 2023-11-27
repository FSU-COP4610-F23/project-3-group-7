#include <inttypes.h>
#include <stdio.h>
#include <limits.h>

// below is the pseudocode fro mount


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
    uint32_t BPB_RootClus;
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

FILE *fat32_img; // The opened fat32.img file
char *current_working_directory; 

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

typedef struct {
    dentry_t *entries; // Array of directory entries
    size_t num_entries; // Number of entries in the array
} directory_contents;

directory_contents root_directory;

uint32_t determine_root_cluster(const bpb_t *bpb) {
    return bpb->BPB_RootClus;
}

void read_root_directory(FILE *img, const bpb_t *bpb) {
    uint32_t root_cluster = determine_root_cluster(bpb);

    // Calculate the location of the root directory in the image
    // This will depend on your filesystem's specifics, like sector size, FAT size, etc.

    // Seek to the root directory's location
    // fseek(img, calculated_location, SEEK_SET);

    // Read the directory entries
    // Use appropriate logic to read the directory entries into root_directory.entries
    // Update root_directory.num_entries accordingly
}

// other data structure, global variables, etc. define them in need.
// e.g., 
// the opened fat32.img file
// the current working directory
// the opened files
// other data structures and global variables you need

// you can give it another name
// fill the parameters
void mount_fat32(FILE *img) {
    // 1. Read and decode the BPB from the FAT32 image file
    // 2. Set up any global variables or structures needed
}

/*char cwd[PATH_MAX_LENGTH]; // Define PATH_MAX_LENGTH as needed

void set_cwd(const char *path) {
    strncpy(cwd, path, PATH_MAX_LENGTH);
    cwd[PATH_MAX_LENGTH - 1] = '\0'; // Ensure null-termination
}
*/

void process_cd(/* parameters */) {
    // Change directory command implementation
}

void process_ls(/* parameters */) {
    // List directory contents command implementation
}

void read_bpb(FILE *img, bpb_t *bpb) {
    // Seek to the start of the file (where the BPB is located)
    fseek(img, 0, SEEK_SET);

    // Read the BPB into the provided bpb_t structure
    fread(bpb, sizeof(bpb_t), 1, img);
}


// you can give it another name
// fill the parameters
void main_process(FILE *img) {
    //char cmd[CMD_MAX_LENGTH]; // Define CMD_MAX_LENGTH appropriately
    bpb_t bpb;
    read_bpb(img, &bpb);
    read_root_directory(img, &bpb);
    //set_cwd("/"); // Set initial cwd to root

    
    
    while (1) {
        // 1. Get cmd from input
        // Example: scanf("%s", cmd);
        //printf("%s/path/to/cwd> ", cwd); // Display prompt
        //char cmd[CMD_MAX_LENGTH];
        //scanf("%s", cmd);

        //if (strcmp(cmd, "exit") == 0) break;
        //else if (strcmp(cmd, "cd") == 0) process_cd(/* parameters */);
        //else if (strcmp(cmd, "ls") == 0) process_ls(/* parameters */);
        // Add other commands as needed
    }
}


/*
void main_process() {
    while (1) {
        // 1. get cmd from input.
        // you can use the parser provided in Project1

        // if cmd is "exit" break;
        // else if cmd is "cd" process_cd();
        // else if cmd is "ls" process_ls();
        // ...
    }
}
*/



int main(int argc, char const *argv[]) {
    // 1. Open the fat32.img
    fat32_img = fopen("fat32.img", "rb"); // or another mode as appropriate

    // 2. Mount the fat32.img
    mount_fat32(fat32_img);

    // 3. Main process
    main_process(fat32_img);

    // 4. Close all opened files and clean up
    // ...

    // 5. Close the fat32.img
    fclose(fat32_img);

    return 0;
}