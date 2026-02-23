
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "../include/diskio.h"

static int disk_fd = -1;

bool disk_open(const char* path) {
    disk_fd = open(path, O_RDWR);
    return disk_fd >= 0;
}

void disk_close() {
    if (disk_fd >= 0)
        close(disk_fd);
}

bool disk_read(uint64_t lba, uint32_t count, void* buffer) {
    lseek(disk_fd, lba * SECTOR_SIZE, SEEK_SET);
    return read(disk_fd, buffer, count * SECTOR_SIZE) ==  count * SECTOR_SIZE;
}

bool disk_write(uint64_t lba, uint32_t count, const void* buffer) {
    lseek(disk_fd, lba * SECTOR_SIZE, SEEK_SET);
    return write(disk_fd, buffer, count * SECTOR_SIZE) ==  count * SECTOR_SIZE;
}




