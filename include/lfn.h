
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct __attribute__((packed)) {
    uint8_t  LDIR_Ord;
    uint16_t LDIR_Name1[5];
    uint8_t  LDIR_Attr;
    uint8_t  LDIR_Type;
    uint8_t  LDIR_Chksum;
    uint16_t LDIR_Name2[6];
    uint16_t LDIR_FstClusLO;
    uint16_t LDIR_Name3[2];
} LFNEntry;

uint8_t fat32_lfn_checksum(uint8_t short_name[11]);
bool fat32_needs_lfn(const char *name);