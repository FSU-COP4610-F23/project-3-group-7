#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>

// Structure representing a directory entry
typedef struct __attribute__((packed)) directory_entry {
    char DIR_Name[11];
    uint8_t DIR_Attr;
    char padding_1[8];
    uint16_t DIR_FstClusHI;
    char padding_2[4];
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
} dentry_t;

// Structure representing the BPB (BIOS Parameter Block)
typedef struct __attribute__((packed)) BPB_FAT32 {
    // ... (other fields from BPB)
    uint32_t BPB_RootClus; // Cluster number of the root directory
    // ... (other fields)
} BPB_FAT32;

void list_files_in_root(int fat32_fd, BPB_FAT32 *bpb) {
    uint32_t root_cluster = bpb->BPB_RootClus;

    // Calculate the starting sector of the root directory
    uint32_t root_sector = (root_cluster - 2) * 128 + bpb->BPB_sectorsPerFAT32 * bpb->BPB_NumFATs;

    // Move to the beginning of the root directory
    lseek(fat32_fd, root_sector * 512, SEEK_SET);

    // Read and print directory entries in the root directory
    dentry_t dentry;
    while (read(fat32_fd, &dentry, sizeof(dentry_t)) > 0) {
        if (dentry.DIR_Name[0] == 0x00) {
            // End of directory entries
            break;
        }

        if (dentry.DIR_Name[0] != 0xE5 && dentry.DIR_Attr != 0x0F) {
            // Not a free entry or a long file name entry

            // Check if it's a directory or a file
            if (dentry.DIR_Attr == 0x10) {
                printf("Directory: %s\n", dentry.DIR_Name);
            } else {
                printf("File: %s\n", dentry.DIR_Name);
            }
        }
    }
}

int main() {
    int fd = open("fat32.img", O_RDONLY);
    if (fd < 0) {
        perror("open fat32.img failed");
        return 1;
    }


    uint32_t rootDirOffset = ((BPB_FAT32.roo - 2) * bpb.BPB_SecPerClus + bpb.BPB_RsvdSecCnt +
                             bpb.BPB_NumFATs * bpb.BPB_FATSz32) *
                             bpb.BPB_BytsPerSec;


    BPB_FAT32 bpb;
    lseek(fd, 0x0B, SEEK_SET); // Adjust the offset based on your file system
    read(fd, &bpb, sizeof(BPB_FAT32));

    list_files_in_root(fd, &bpb);

    close(fd);

    return 0;
}
