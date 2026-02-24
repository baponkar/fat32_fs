

#include "../include/main.h"


#define SECTOR_SIZE 512

extern uint32_t fat32_base_lba;

int main() {

    // if (!disk_open("disk_img/disk.img")) {
    //     printf("Failed to open disk\n");
    //     return 1;
    // }

    // uint8_t buffer[SECTOR_SIZE] = {0};

    // // Write 0xAB at first byte of disk
    // buffer[0] = 0xAB;

    // if (!disk_write(0, 1, buffer)) {
    //     printf("Write failed\n");
    //     return 1;
    // }

    // // Clear buffer
    // buffer[0] = 0;

    // // Read again
    // if (!disk_read(0, 1, buffer)) {
    //     printf("Read failed\n");
    //     return 1;
    // }

    // printf("First byte after write: 0x%02X\n", buffer[0]);

    // disk_close();

    fat32_base_lba = 0; // Assuming partition starts at LBA 0 for testing
    uint32_t space_size_mb = 100; // 100 MB

    uint32_t sectors = (space_size_mb * 1024 * 1024) / SECTOR_SIZE;

    if (!create_fat32_volume( fat32_base_lba, sectors)) {
        printf("Failed to create FAT32 volume\n");
        return 1;
    }
    if (!fat32_test( fat32_base_lba)) {
        printf("FAT32 test failed\n");
        return 1;
    }

    printf("FAT32 test passed successfully!\n");

    return 0;
}


