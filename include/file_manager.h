#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stddef.h>

#include "fat32.h"

#define FA_READ     0x01
#define FA_WRITE    0x02
#define FA_CREATE   0x04
#define FA_CREATE_ALWAYS 0x08



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



void file_test_func();
