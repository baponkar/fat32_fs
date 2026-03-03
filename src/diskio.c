
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../include/diskio.h"

static int disk_fd = -1;

char *disk_path = "disk_img/disk.img";

static bool disk_open(const char* path) {
    disk_fd = open(path, O_RDWR);
    return disk_fd >= 0;
}

static void disk_close() {
    if (disk_fd >= 0)
        close(disk_fd);
}

bool disk_read(uint64_t lba, uint32_t count, void* buffer) {
    if(disk_open(disk_path) == false){
        printf("Failed to open disk for reading\n");
        return false;
    }
    lseek(disk_fd, lba * SECTOR_SIZE, SEEK_SET);
    ssize_t bytes_read = read(disk_fd, buffer, count * SECTOR_SIZE);

    disk_close();

    return bytes_read == count * SECTOR_SIZE;
}


bool disk_write(uint64_t lba, uint32_t count, const void* buffer) {
    if(disk_open(disk_path) == false){
        printf("Failed to open disk for writing\n");
        return false;
    }
    lseek(disk_fd, lba * SECTOR_SIZE, SEEK_SET);
    ssize_t bytes_written = write(disk_fd, buffer, count * SECTOR_SIZE);
    disk_close();
    return bytes_written == count * SECTOR_SIZE;
}


bool disk_clear(uint64_t lba, uint32_t count) {
    uint32_t buffer_size = SECTOR_SIZE * count;
    uint8_t *buffer = malloc(buffer_size);
    if(!buffer) return false;
    memset(buffer, 0, SECTOR_SIZE * count);

    return disk_write(lba, count, buffer);
}


