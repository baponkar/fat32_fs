#include  <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/fat32.h"

#include "../test/fat32_test_1.h"


#define SECTOR_SIZE 512


// Testing 8.3 Filename FAT32 Test
bool fat32_test( uint64_t fat_base_lba){

    uint32_t space_size_mb = 100; // 100 MB

    uint32_t sectors = (space_size_mb * 1024 * 1024) / SECTOR_SIZE;

    if (!create_fat32_volume( fat_base_lba, sectors)) {
        printf("Failed to create FAT32 volume\n");
        return 1;
    }
    printf("[FAT32 TEST] Successfully created FAT32 volume at LBA: %ld with size: %d MB\n", fat_base_lba, space_size_mb);

    if(!fat32_mount(fat_base_lba)){
        printf("[FAT32 TEST] Failed to Mount FAT32 FS at LBA: %ld!\n", fat_base_lba);
        return false;
    }
    printf("[FAT32 TEST] Successfully Mount Disk.\n");

    
    
    // Crating a directory at root
    char *dir_path = "TESTDIR";
    if(!fat32_mkdir( dir_path)){
        printf("[FAT32 TEST] Creating Directory %s is failed!\n", dir_path);
        return false;
    }
    printf("[FAT32 TEST] Creating Directory %s is success.\n", dir_path);

    // Finding Directory Cluster no
    uint32_t dir_cluster_no = 0;
    if(!fat32_path_to_cluster( dir_path, &dir_cluster_no)){
        printf("[FAT32 TEST] Failed to get Cluster no for %s\n", dir_path);
        return false;
    }
    printf("[FAT32 TEST] Successfully get Cluster no %d for directory %s\n", dir_cluster_no, dir_path);

    // Creating testfile.text
    char *file_name = "TESTFILE.TXT";   // 8.3 Short Filename
    char *buff = "This is a test text string for testing fat32 filesystem.";
    uint32_t file_size = strlen(buff);

    if(!fat32_create_file_in_dir( dir_cluster_no, file_name, buff, file_size)){
        return false;
    }
    printf("[FAT32 TEST] Successfully created %s\n\n", file_name);

    // Opening testfile.txt
    const char *file_path = "TESTDIR/TESTFILE.TXT";
    FAT32_FILE file;
    if(!fat32_open( file_path, &file)){
        printf("[FAT32 TEST] Faile to read file %s\n", file_path);
        return false;
    }
    printf("[FAT32 TEST] Successfully open file %s\n", file_path);

    // Reading testfile.txt 
    char *buffer = (char *) malloc(file_size);
    uint32_t rb = fat32_read( &file, buffer, file_size);
    if(rb <= 0){
        printf("[FAT32 TEST] Failed to read file %s!\n", file_path);
        // return false;
    }
    printf("[FAT32 TEST] Successfully read %d bytes\n", rb);
    printf("[FAT32 TEST] File content: %s\n", buffer);

    free(buffer);

    return true;
}


