
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "../include/diskio.h"

#include "guid.h"
#include "mbr.h"
#include "gpt.h"

#include "partition_manager.h"



#define TOTAL_SECTORS 1 * 1024 * 1024 * 1024 / 512   // 1GB = 1 * 1024 * 1024 * 1024 Byte
#define MAX_PARTITIONS 4

PartitionEntry partitions[MAX_PARTITIONS];  // Array of PartitionEntry

size_t partition_count = 0;

bool create_partition(uint8_t pdrv_no, uint64_t start_lba, uint64_t sectors, guid_t partition_guid, guid_t partition_type_guid, char* name) {
    
    size_t partition_index = partition_count; // Get the next available partition index

    // Creating and Write a Protective MBR at Sector 0
    ProtectiveMBR *protective_mbr = malloc(SECTOR_SIZE);
    create_protective_mbr(protective_mbr, TOTAL_SECTORS);

    if(!disk_write( 0, 1, protective_mbr)){
        printf("[PARTITION] Failed to write Protective MBR for disk!\n");
        free(protective_mbr);
        return false;
    }

    // ==========================================================

    if (partition_count >= MAX_PARTITIONS) {
        printf("Maximum number of partitions reached.\n");
        return false;
    }

    PartitionEntry entry = partitions[partition_index];
    entry.pdrv_no = pdrv_no;
    entry.partition_no = partition_index;
    entry.start_lba = start_lba;
    entry.sectors = sectors;
    memcpy(&entry.partition_guid, partition_guid, sizeof(guid_t));
    memcpy(&entry.partition_type_guid, partition_type_guid, sizeof(guid_t));

    // ======================================================================
    // Creating GPTPrtition
    GPTPartitionEntry gpt_partitions[GPT_ENTRIES_COUNT];
    memset(gpt_partitions, 0, sizeof(gpt_partitions));

    // Read Previous partition entries from disk
    int total_entries_sectors = (int) (GPT_ENTRIES_COUNT * GPT_ENTRY_SIZE + SECTOR_SIZE - 1) / SECTOR_SIZE; // This should be 32 sectors for 128 entries of 128 bytes each
    if(!disk_read(GPT_ENTRIES_START_LBA, total_entries_sectors, gpt_partitions)) {
        printf("[PARTITION] Failed to read primary GPT partition entries for disk!\n");
        return false;
    }
    
    // Creating new GPT partition entry for this partition
    GPTPartitionEntry *gpt_entry = create_gpt_partition_entry(partition_type_guid, partition_guid, start_lba, start_lba + sectors - 1, 0, name);
    if(gpt_entry != NULL) {
        memcpy(&entry.gpt_entry, gpt_entry, sizeof(GPTPartitionEntry));
    } else {
        return false;
    }

    gpt_partitions[partition_index] = *gpt_entry; // include the new partition entry in the array for GPTPartitionEntry array

    // Creating GPT Headers and Partition Entries
    GPTHeader primary_header;
    GPTHeader backup_header;


    // write partition entries to disk (primary and backup)
    if(!disk_write( GPT_ENTRIES_START_LBA, total_entries_sectors, gpt_partitions)) {
        printf("[PARTITION] Failed to write primary GPT partition entries for disk!\n");
        return false;
    }
    
    uint64_t backup_entries_lba = TOTAL_SECTORS - total_entries_sectors - 1;
    // if (!kebla_disk_write( backup_entries_lba, total_entries_sectors, partitions)) {
    if (!disk_write( backup_entries_lba, total_entries_sectors, gpt_partitions)) {
        printf("[PARTITION] Failed to write backup GPT partition entries for disk!\n");
        return false;
    }

    // Create and write GPT headers
    bool result = create_gpt_header(TOTAL_SECTORS, &primary_header, &backup_header, DISK_GUID_EXAMPLE, gpt_partitions);

    if(result) {
        printf("Created partition %d on drive %d: Start LBA: %lu, Sectors: %lu\n", 
               entry.partition_no, entry.pdrv_no, entry.start_lba, entry.sectors);
    } else {
        printf("Failed to create GPT headers for partition %d on drive %d.\n", 
               entry.partition_no, entry.pdrv_no);
        return false;
    }

    partition_count++; // Increment the partition count

    return true;
}




