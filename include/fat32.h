#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "file_manager.h"
#include "dir_manager.h"

#include "fat32_mount.h"

// Available functions in fat32.h
bool create_fat32_volume( uint64_t start_lba, uint32_t sectors); // defined in fat32_mount.c
bool fat32_mount( uint64_t partition_lba_start);                 // defined in fat32_mount.c



// Available in dir_manager.h
bool f_cwd(char *path);                                     // get Current Working Directory
bool f_opendir(FAT32_DIR *dp, char *path);                  // Open Directory
bool f_closedir(FAT32_DIR *dp);                             // Close Directory
bool f_readdir(FAT32_DIR *dp, FAT32_DIRENT *entry);         // Reading Directory
bool f_mkdir(const char *path);                             // Make a new directory
bool f_rename(const char *oldpath, const char *newpath);    // rename file, rename directory, move file, move directory
bool f_chdir(const char *path);                             // Change Directory

bool f_findfirst(FAT32_DIR *dp, FAT32_DIRENT *entry, const char *path, const char *pattern);
bool f_findnext(FAT32_DIR *dp, FAT32_DIRENT *entry,  const char *pattern);


// Available functions in file_manager.h
bool f_open( FAT32_FILE* fp, const char* path, int mode);
bool f_close(FAT32_FILE* fp);
bool f_read(FAT32_FILE* fp, void *buff, uint32_t btr, uint32_t *br);
bool f_write(FAT32_FILE* fp, const void* buff, uint32_t btw, uint32_t* bw);
bool f_lseek(FAT32_FILE* fp, uint32_t ofs);
bool f_truncate(FAT32_FILE* fp);
bool f_sync(FAT32_FILE *fp);
bool f_forward( FAT32_FILE *fp, uint32_t (*func)(const uint8_t *data, uint32_t len),  uint32_t btf,  uint32_t *bf);
bool f_expand(FAT32_FILE *fp, uint32_t size);
char* f_gets(char *buff, int len, FAT32_FILE *fp);
int f_putc(char c, FAT32_FILE *fp);
int f_puts(const char *str, FAT32_FILE *fp);
int f_printf(FAT32_FILE *fp, const char *fmt, ...);
uint32_t f_tell(FAT32_FILE *fp);
bool f_eof(FAT32_FILE *fp);
uint32_t f_size(FAT32_FILE *fp);
bool f_stat(const char *path, FAT32_STAT *stat);
bool f_unlink(const char *path);





void fat32_fs_test();




