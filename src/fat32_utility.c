


#include "../include/fat32_utility.h"


extern bool disk_read(uint64_t lba, uint32_t count, void* buffer);
extern bool disk_write(uint64_t lba, uint32_t count, const void* buffer);

bool fat32_read_sector(uint64_t lba, void *buf) {
    return disk_read(lba, 1, buf);
}

bool fat32_write_sector(uint64_t lba, const void *buf) {
    return disk_write(lba, 1, buf);
}

bool fat32_read_sectors(uint64_t lba, uint32_t count, void *buf) {
    return disk_read(lba, count, buf);
}

bool fat32_write_sectors(uint64_t lba, uint32_t count, const void *buf) {
    return disk_write(lba, count, buf);
}



