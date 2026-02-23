#pragma once
#include <stdint.h>
#include <stdbool.h>

#define SECTOR_SIZE 512

bool disk_open(const char* path);
void disk_close();

bool disk_read(uint64_t lba, uint32_t count, void* buffer);
bool disk_write(uint64_t lba, uint32_t count, const void* buffer);









