#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define SECTOR_SIZE 512

// File attributes: 
#define ATTR_READ_ONLY  0x01 
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04 
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20

#define ATTR_LONG_NAME  0x0F
#define LFN_LAST_LONG_ENTRY 0x40

#define CLUSTER_END_OF_CHAIN 0x0FFFFFFF
#define CLUSTER_BAD 0x0FFFFFF7


typedef struct __attribute__((packed)) {
    uint8_t DIR_Name[11];       // Offset 0 : Directory name in 8.3 format
    uint8_t DIR_Attr;           // Offset 11 : Directory attributes    
    uint8_t DIR_NTRes;          // Offset 12 : Reserved for Windows NT (should be zero)
    uint8_t DIR_CrtTimeTenth;   // Offset 13 : Creation time in tenths of a second
    uint16_t DIR_CrtTime;       // Offset 14 : Creation time
    uint16_t DIR_CrtDate;       // Offset 16 : Creation date
    uint16_t DIR_LstAccDate;    // Offset 18 : Last access date
    uint16_t DIR_FstClusHI;     // Offset 20 : High word of first cluster number
    uint16_t DIR_WrtTime;       // Offset 22 : Write time
    uint16_t DIR_WrtDate;       // Offset 24 : Write date
    uint16_t DIR_FstClusLO;     // Offset 26 : Low word of first cluster number
    uint32_t DIR_FileSize;      // Offset 28 : File size in bytes
} DirEntry;                     // 32 bit or 4 bytes

typedef struct {
    uint32_t first_cluster;     // First Cluster number
    uint32_t size;              // file size
    uint32_t pos;               // pointer position
    uint32_t parent_cluster;    // Parent Directory cluster
    char name[11];              // 8.3 Name
} FAT32_FILE;                   // 104 bytes























