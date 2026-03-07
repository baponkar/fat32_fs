
#include "../include/partition_manager.h"
#include "../include/fat32.h"

#include "main.h"


#define PDRV_NO 0

#define SECTOR_SIZE 512

#define TOTAL_SECTORS 1 * 1024 * 1024 * 1024 / SECTOR_SIZE  // 1 GB

#define PARTITION_1_START_LBA 2048
#define PARTITION_1_TOTAL_LBA 100 * 1024 * 1024 / SECTOR_SIZE    // 100 MB
#define PARTITION_1_NAME "BOOT PARTITION"

#define PARTITION_2_START_LBA PARTITION_1_START_LBA + PARTITION_1_TOTAL_LBA // Next to Partition 1
#define PARTITION_2_TOTAL_LBA TOTAL_SECTORS - PARTITION_1_START_LBA - PARTITION_1_TOTAL_LBA - 2048 // Rest of Disk Space, 2048 for safety
#define PARTITION_2_NAME "DATA PARTITION"



int main() {

    if(create_partition(PDRV_NO, PARTITION_1_START_LBA, PARTITION_1_TOTAL_LBA, ESP_GUID, ESP_TYPE_GUID, PARTITION_1_NAME)){
        printf("Successfully created Partition at Sector %d\n", PARTITION_1_START_LBA);
    }
    
    if(create_partition( PDRV_NO, PARTITION_2_START_LBA, PARTITION_2_TOTAL_LBA, DATA_PARTITION_GUID, LINUX_FS_GUID, PARTITION_2_NAME)){
        printf("Successfully created Partition at Sector %d\n",  PARTITION_2_START_LBA);
    }


    if (!create_fat32_volume( PARTITION_1_START_LBA, PARTITION_1_TOTAL_LBA)) {
        printf("Failed to create FAT32 volume\n");
        return 1;
    }
    printf("[FAT32 TEST] Successfully created FAT32 volume at LBA: %d with size: %d MB\n", PARTITION_1_START_LBA, PARTITION_1_TOTAL_LBA);

    if (!create_fat32_volume( PARTITION_2_START_LBA, PARTITION_2_TOTAL_LBA)) {
        printf("Failed to create FAT32 volume\n");
        return 1;
    }
    printf("[FAT32 TEST] Successfully created FAT32 volume at LBA: %d with size: %d MB\n", PARTITION_1_START_LBA, PARTITION_2_TOTAL_LBA);

    if(!fat32_mount(PARTITION_1_START_LBA)){
        printf("[FAT32 TEST] Failed to Mount FAT32 FS at LBA: %d!\n", PARTITION_1_START_LBA);
        return false;
    }
    printf("[FAT32 TEST] Successfully Mount Disk.\n");

    fat32_fs_test();
    
    return 0;
}


