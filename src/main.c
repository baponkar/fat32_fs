#include "../include/diskio.h"
#include "../include/fat32.h"

#include "../include/main.h"




int main() {

    if (!disk_open("disk_img/disk.img")) {
        printf("Failed to open disk\n");
        return 1;
    }

    uint8_t buffer[SECTOR_SIZE] = {0};

    // Write 0xAB at first byte of disk
    buffer[0] = 0xAB;

    if (!disk_write(0, 1, buffer)) {
        printf("Write failed\n");
        return 1;
    }

    // Clear buffer
    buffer[0] = 0;

    // Read again
    if (!disk_read(0, 1, buffer)) {
        printf("Read failed\n");
        return 1;
    }

    printf("First byte after write: 0x%02X\n", buffer[0]);

    disk_close();
    return 0;
}


