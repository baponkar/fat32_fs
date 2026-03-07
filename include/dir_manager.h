#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "fat32.h"



bool f_cwd(char *path);
bool f_opendir(FAT32_DIR *dp, char *path);
bool f_closedir(FAT32_DIR *dp);
bool f_readdir(FAT32_DIR *dp, FAT32_DIRENT *entry);
bool f_mkdir(const char *path);
bool f_rename(const char *oldpath, const char *newpath);    // rename file, rename directory, move file, move directory

bool f_findfirst(FAT32_DIR *dp, FAT32_DIRENT *entry, const char *path, const char *pattern);
bool f_findnext(FAT32_DIR *dp, FAT32_DIRENT *entry,  const char *pattern);

void fat32_test_dir_manager();