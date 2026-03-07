#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "fat32_mount.h"
#include "cluster_manager.h"

typedef struct {
    uint32_t first_cluster;     // First Cluster number
    uint32_t current_cluster;   // cluster currently reading/writing
    uint32_t size;              // file size
    uint32_t pos;               // pointer position

    uint32_t parent_cluster;    // Parent Directory cluster

    uint32_t dir_entry_sector;  // sector containing the entry
    uint32_t dir_entry_offset;  // offset inside sector

    char name[256];             // Long Name
    uint8_t mode;               // read/write flags
} FAT32_FILE;                   // 104 bytes


typedef struct {
    uint32_t first_cluster;     // First Cluster Number 
    uint32_t current_cluster;

    uint32_t pos;

    uint32_t parent_cluster;    // Parent Cluster of this directory entry

    char name[256];             // Directory long name
} FAT32_DIR;

// Directory Entry Info Structure
typedef struct {
    char name[256];
    uint8_t attr;
    uint32_t size;
    uint32_t first_cluster;
} FAT32_DIRENT;


bool create_fat32_volume( uint64_t start_lba, uint32_t sectors); // defined in fat32_mount.c
bool fat32_mount( uint64_t partition_lba_start);                 // defined in fat32_mount.c

bool fat32_change_current_directory( const char *path); 
bool fat32_mkdir( const char* dirpath);

bool fat32_open( const char *path, FAT32_FILE *file);

uint32_t fat32_read( FAT32_FILE *file, void *buffer, uint32_t size);        // read file
uint32_t fat32_write( FAT32_FILE *file, const void *buffer, uint32_t size); // write file







