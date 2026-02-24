#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#include "fat32_types.h"



bool fat32_set_volume_label( const char *label);
bool fat32_mount( uint64_t partition_lba_start);
bool fat32_change_current_directory( const char *path);
bool fat32_path_to_cluster( const char *path, uint32_t *out_cluster);
bool fat32_mkdir( const char* dirpath);
bool fat32_open( const char *path, FAT32_FILE *file);
uint32_t fat32_read( FAT32_FILE *file, void *buffer, uint32_t size);
uint32_t fat32_write( FAT32_FILE *file, const void *buffer, uint32_t size);

bool fat32_mkdir_root( const char *name);

bool fat32_test( uint64_t fat_base_lba);


