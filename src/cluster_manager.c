

#include "../include/cluster_manager.h"




extern uint64_t fat32_base_lba;             // Defined in fat32
extern BPB *bpb;                            // Defined in fat32

uint32_t fat32_cwd_cluster = 2;             // Current Working Directory Cluster

/* 
This variable is used to optimize free cluster search. 
It keeps track of the last allocated cluster number. 
When searching for a free cluster, the search starts 
from this number instead of the beginning of the FAT. 
This can significantly reduce allocation time, 
especially on larger disks, as it avoids repeatedly 
scanning the FAT from the start for every allocation. 
It is updated whenever a new cluster is allocated, 
ensuring that the next search for a free cluster 
begins from the last allocated position, thus improving 
performance by minimizing the number of FAT entries that 
need to be checked for free clusters. This optimization is 
particularly beneficial in scenarios where there are many 
allocations and deallocations, as it helps to quickly find 
free clusters without unnecessary scanning of the FAT, 
leading to faster file creation and extension operations.
*/
static uint32_t fat32_free_cluster_no = 2;

static bool fat32_read_sector(uint64_t lba, void *buf) {
    return disk_read( fat32_base_lba + lba, 1, buf);
}

static bool fat32_write_sector(uint64_t lba, const void *buf) {
    return disk_write(fat32_base_lba + lba, 1, (void*)buf);
}

// read a single cluster and store it in given buffer.
static bool fat32_read_cluster(uint32_t cluster_number, void *buffer){
    uint32_t first_sector = get_first_sector_of_cluster(cluster_number);
    uint8_t *buf_ptr = (uint8_t *)buffer;
    
    for(uint8_t i = 0; i < bpb->BPB_SecPerClus; i++){
        if(!fat32_read_sector( first_sector + i, buf_ptr + (i * bpb->BPB_BytsPerSec))){
            return false;
        }
    }
    return true;
}

// write a single cluster from given buffer
static bool fat32_write_cluster( uint32_t cluster_number, const void *buffer)
{
    uint32_t first_sector = get_first_sector_of_cluster(cluster_number);

    return disk_write(fat32_base_lba + first_sector, bpb->BPB_SecPerClus, (void*)buffer );
}

// Clearing a single cluster
static bool fat32_clear_cluster( uint32_t cluster) {
    uint32_t cluster_size = bpb->BPB_BytsPerSec * bpb->BPB_SecPerClus;
    uint8_t *zero = malloc(cluster_size);
    if (!zero) return false;

    memset(zero, 0, cluster_size);
    bool ok = fat32_write_cluster( cluster, zero);
    free(zero);
    return ok;
}

static uint32_t fat32_get_next_cluster( uint32_t current_cluster){
    uint32_t fat_offset = current_cluster * 4; // Total bytes as Each FAT32 entry is 4 bytes
    uint32_t fat_sector_number = bpb->BPB_RsvdSecCnt + (fat_offset / bpb->BPB_BytsPerSec);
    uint32_t ent_offset = fat_offset % bpb->BPB_BytsPerSec; // Sector

    uint8_t sector_buffer[512];
    if (!fat32_read_sector( fat_sector_number, sector_buffer)) {
        return 0;               // Error reading sector
    }

    uint32_t next_cluster = *(uint32_t *)&sector_buffer[ent_offset];    // As Current Cluster's FAT wrote next Cluster number
    next_cluster &= 0x0FFFFFFF; // Mask to get the lower 28 bits

    return next_cluster;
}

static bool fat32_set_next_cluster( uint32_t current_cluster, uint32_t next_cluster) {
    uint32_t fat_offset = current_cluster * 4;
    uint32_t fat_sector_relative = fat_offset / bpb->BPB_BytsPerSec;
    uint32_t ent_offset = fat_offset % bpb->BPB_BytsPerSec;

    uint8_t sector_buffer[512]; // Note: Ideally use bpb->BPB_BytsPerSec

    // 1. Read from FAT1 to get the current entry (to preserve high 4 bits)
    uint32_t fat1_sector = bpb->BPB_RsvdSecCnt + fat_sector_relative;

    if (!fat32_read_sector( fat1_sector, sector_buffer)) 
        return false;

    // 2. Update the entry
    uint32_t *entry = (uint32_t *)&sector_buffer[ent_offset];
    *entry = (*entry & 0xF0000000) | (next_cluster & 0x0FFFFFFF);

    // 3. Write this same sector to ALL FAT tables
    for (uint8_t i = 0; i < bpb->BPB_NumFATs; i++) {
        uint32_t target_sector = bpb->BPB_RsvdSecCnt + (i * bpb->BPB_FATSz32) + fat_sector_relative;
        if (!fat32_write_sector( target_sector, sector_buffer)) {
            return false;
        }
    }
    return true;
}

static bool fat32_validate_cluster_chain( uint32_t start_cluster) {
    uint32_t curr = start_cluster;

    while (is_valid_cluster(curr)) {
        uint32_t next = fat32_get_next_cluster( curr);
        if (next == 0) return false;
        if (is_end_of_cluster_chain(next)) return true;
        curr = next;
    }

    return false;
}

static bool fat32_free_cluster_chain( uint32_t start_cluster){
    uint32_t current_cluster = start_cluster;

    while (is_valid_cluster(current_cluster)) {
        uint32_t next_cluster = fat32_get_next_cluster( current_cluster);
        if (next_cluster == 0) {
            return false; // Error reading next cluster
        }

        // Mark current cluster as free
        if (!fat32_set_next_cluster( current_cluster, 0x00000000)) {
            return false; // Error setting cluster
        }

        if (is_end_of_cluster_chain(next_cluster)) {
            break; // Reached end of chain
        }

        current_cluster = next_cluster;
    }

    return true;
}



// Allocate a free cluster and return its number
static bool fat32_allocate_cluster( uint32_t *allocated_cluster)
{
    uint32_t total_clusters = get_total_clusters();

    for (uint32_t cluster = fat32_free_cluster_no; cluster < total_clusters + 2; cluster++)
    {
        uint32_t fat_offset = cluster * 4;  // Each entry is 4 bytes
        uint32_t fat_sector_number = bpb->BPB_RsvdSecCnt + (fat_offset / bpb->BPB_BytsPerSec);
        uint32_t ent_offset =  fat_offset % bpb->BPB_BytsPerSec;

        uint8_t sector_buffer[512];

        if (!fat32_read_sector( fat_sector_number, sector_buffer))
            return false;

        uint32_t entry = *(uint32_t *)&sector_buffer[ent_offset] & 0x0FFFFFFF;

        if (entry == 0) {   // Found a free cluster
            fat32_set_next_cluster( cluster, CLUSTER_END_OF_CHAIN);   // Mark it as end of chain
            *allocated_cluster = cluster;

            fat32_free_cluster_no = cluster + 1;    // update fat32_free_cluster_no

            return true;
        }
    }

    return false;
}

static bool fat32_allocate_cluster_chain( uint32_t count, uint32_t *first_cluster){
    uint32_t prev_cluster = 0;
    *first_cluster = 0;

    for (uint32_t i = 0; i < count; i++) {
        uint32_t new_cluster;
        if (!fat32_allocate_cluster( &new_cluster)) {
            return false; // Allocation failed
        }

        if (prev_cluster != 0) {
            if (!fat32_set_next_cluster( prev_cluster, new_cluster)) {
                return false; // Error linking clusters
            }
        } else {
            *first_cluster = new_cluster; // Set first cluster
        }

        prev_cluster = new_cluster;
    }

    // Mark the last cluster as end of chain
    if (!fat32_set_next_cluster( prev_cluster, CLUSTER_END_OF_CHAIN)) {
        return false; // Error setting end of chain
    }

    return true;
}

// reading cluster chain from given cluster
bool fat32_read_cluster_chain( uint32_t start_cluster, void *buffer, uint32_t max_bytes) {

    uint8_t *buf = (uint8_t *)buffer;
    uint32_t current = start_cluster;
    uint32_t bytes_read = 0;
    uint32_t cluster_size = get_cluster_size_bytes();

    while (is_valid_cluster(current)) {
        if (bytes_read + cluster_size > max_bytes) {
            return false; // Buffer too small
        }

        if (!fat32_read_cluster( current, buf + bytes_read)) {
            return false;   // If reading fails, return false
        }

        bytes_read += cluster_size;

        uint32_t next = fat32_get_next_cluster( current);
        if (is_end_of_cluster_chain(next)) {
            break;
        }

        current = next;
    }

    return true;
}

// writing cluster chain starting from given cluster
static bool fat32_write_cluster_chain( const void *buffer, uint32_t size, uint32_t *first_cluster)
{
    const uint8_t *buf = (const uint8_t *)buffer;
    uint32_t cluster_size = get_cluster_size_bytes();
    uint32_t written = 0;

    uint32_t prev_cluster = 0;
    uint32_t curr_cluster = 0;

    uint8_t *temp = malloc(cluster_size);
    if (!temp) return false;


    while (written < size) {
        // 1. Allocate a new cluster
        if (!fat32_allocate_cluster( &curr_cluster)) {
            free(temp);
            return false;
        }

        // 2. Link it to the previous cluster in the chain
        if (prev_cluster != 0) {
            fat32_set_next_cluster( prev_cluster, curr_cluster);
        } else {
            *first_cluster = curr_cluster;
        }

        uint32_t to_write =  (size - written > cluster_size)  ? cluster_size : (size - written);

        // 3. Write data to the current cluster
        if (to_write == cluster_size) {
            if(!fat32_write_cluster( curr_cluster, buf + written)){
                free(temp);
                return false;
            }
        } else {
            memset(temp, 0, cluster_size);
            memcpy(temp, buf + written, to_write);
            if(!fat32_write_cluster( curr_cluster, temp)){
                free(temp);
                return false;
            }
        }

        written += to_write;

        prev_cluster = curr_cluster;
    }


    // 4. Mark the end of the cluster chain
    fat32_set_next_cluster( prev_cluster, CLUSTER_END_OF_CHAIN);

    free(temp);

    return true;
}


// Count the number of clusters in a chain starting from given cluster
static uint32_t fat32_count_cluster_chain( uint32_t start_cluster) {
    uint32_t count = 0;
    uint32_t curr = start_cluster;

    while (is_valid_cluster(curr)) {
        count++;
        uint32_t next = fat32_get_next_cluster( curr);
        if (is_end_of_cluster_chain(next)) {
            break;
        }
        curr = next;
    }

    return count;
}

static bool fat32_append_cluster( uint32_t start_cluster, uint32_t *new_cluster) {
    uint32_t curr = start_cluster;

    while (1) {
        uint32_t next = fat32_get_next_cluster( curr);
        if (is_end_of_cluster_chain(next)) {
            break;
        }
        curr = next;
    }

    if (!fat32_allocate_cluster( new_cluster)) {
        return false;
    }

    fat32_set_next_cluster( curr, *new_cluster);
    fat32_set_next_cluster( *new_cluster, CLUSTER_END_OF_CHAIN);  // End of chain
    return true;
}

// This function searches for a free directory entry in the specified directory cluster.
static bool fat32_find_free_dir_entry( uint32_t dir_cluster, uint32_t *out_cluster, uint32_t *out_offset) {
    uint32_t cluster_size = get_cluster_size_bytes();
    uint8_t *buf = (uint8_t *) malloc(cluster_size);
    if (!buf) return false;

    uint32_t curr = dir_cluster;

    while (is_valid_cluster(curr)) {
        if (!fat32_read_cluster( curr, buf)) {
            free(buf);
            return false;
        }

        for (uint32_t offset = 0; offset < cluster_size; offset += 32) {
            uint8_t first = buf[offset];
            if (first == 0x00 || first == 0xE5) {
                *out_cluster = curr;
                *out_offset  = offset;
                free(buf);
                return true;
            }
        }

        uint32_t next = fat32_get_next_cluster( curr);
        if (is_end_of_cluster_chain(next)) break;
        curr = next;
    }

    /* No free entry → extend directory */
    uint32_t new_cluster;
    if (!fat32_append_cluster( dir_cluster, &new_cluster)) {
        free(buf);
        return false;
    }

    fat32_clear_cluster( new_cluster);

    *out_cluster = new_cluster;
    *out_offset  = 0;

    free(buf);

    return true;
}


// This function converts a given filename into the 8.3 format used in FAT32 directory entries.
static void fat32_format_83_name(const char *name, char out[11]) {
    memset(out, ' ', 11);

    int i = 0;  // Given string index
    int j = 0;  // Formatted string index

    // Name part (8 chars max)
    while (name[i] && name[i] != '.' && j < 8) {
        out[j++] = toupper((unsigned char)name[i++]);
    }

    // Skip to extension if dot exists
    while (name[i] && name[i] != '.') {
        i++;
    }

    // Extension part
    if (name[i] == '.') {
        i++;
        j = 8;
        while (name[i] && j < 11) {
            out[j++] = toupper((unsigned char)name[i++]);
        }
    }
}


/*
 This function sets the volume label in the root directory. 
 It either updates an existing Volume ID entry or creates a new one if it doesn't exist.
 */
bool fat32_set_volume_label( const char *label) {
    uint32_t root_cluster = bpb->BPB_RootClus;
    uint32_t cluster_size = get_cluster_size_bytes();
    
    uint8_t *buf = (uint8_t *) malloc(cluster_size);
    if (!buf) return false;

    // 1. Read the first cluster of the root directory
    if (!fat32_read_cluster( root_cluster, buf)) {
        free(buf);
        return false;
    }

    // 2. Find an empty slot or an existing Volume ID slot
    DirEntry *target_entry = NULL;
    for (uint32_t offset = 0; offset < cluster_size; offset += 32) {
        DirEntry *entry = (DirEntry *)(buf + offset);
        
        // If we find an existing label, overwrite it. 
        // Otherwise, take the first available slot (0x00 or 0xE5).
        if (entry->DIR_Attr == ATTR_VOLUME_ID || entry->DIR_Name[0] == 0x00 || entry->DIR_Name[0] == 0xE5) {
            target_entry = entry;
            break;
        }
    }

    if (!target_entry) {
        free(buf);
        return false; // Root cluster is full (unlikely for a fresh disk)
    }

    // 3. Setup the Label Entry
    memset(target_entry, 0, sizeof(DirEntry));
    
    // Format name to 11 chars, no dot, uppercase, space padded
    for (int i = 0; i < 11; i++) {
        if (label[i] && label[i] != '.') {
            target_entry->DIR_Name[i] = toupper(label[i]);
        } else {
            target_entry->DIR_Name[i] = ' ';
        }
    }

    target_entry->DIR_Attr = ATTR_VOLUME_ID;
    target_entry->DIR_FstClusHI = 0; // Always 0 for Volume Labels
    target_entry->DIR_FstClusLO = 0; // Always 0 for Volume Labels
    target_entry->DIR_FileSize = 0;

    // 4. Write back to disk
    bool ok = fat32_write_cluster( root_cluster, buf);
    free(buf);
    return ok;
}




// This function creates a directory entry in the specified parent directory cluster with the given name, attributes, starting cluster, and file size.
static bool fat32_create_dir_entry( uint32_t parent_cluster, const char *name, uint8_t attr, uint32_t first_cluster , uint32_t file_size) {
    uint32_t entry_cluster, entry_offset;

    if (!fat32_find_free_dir_entry(  parent_cluster, &entry_cluster, &entry_offset)) {
        return false;
    }

    uint32_t cluster_size = get_cluster_size_bytes();
    uint8_t *buf = malloc(cluster_size);
    if (!buf) return false;

    if (!fat32_read_cluster( entry_cluster, buf)) {
        free(buf);
        return false;
    }

    DirEntry *entry = (DirEntry *)(buf + entry_offset);

    memset(entry, 0, sizeof(DirEntry));
    fat32_format_83_name(name, entry->DIR_Name);

    entry->DIR_Attr = attr;
    entry->DIR_FstClusHI = (first_cluster >> 16) & 0xFFFF;
    entry->DIR_FstClusLO = first_cluster & 0xFFFF;
    entry->DIR_FileSize = file_size;

    bool ok = fat32_write_cluster( entry_cluster, buf);
    free(buf);
    return ok;
}

// This function initializes a new directory cluster by creating the "." and ".." entries.
static bool fat32_init_directory( uint32_t dir_cluster, uint32_t parent_cluster)
{
    uint32_t cluster_size = get_cluster_size_bytes();
    uint8_t *buf = malloc(cluster_size);
    if (!buf) return false;

    memset(buf, 0, cluster_size);

    DirEntry *dot = (DirEntry *)buf;

    DirEntry *dotdot = (DirEntry *)(buf + 32);

    /* "." entry */
    memset(dot, 0, sizeof(DirEntry));
    memset(dot->DIR_Name, ' ', 11);
    dot->DIR_Name[0] = '.';
    dot->DIR_Attr = ATTR_DIRECTORY;
    dot->DIR_FstClusLO = dir_cluster & 0xFFFF;
    dot->DIR_FstClusHI = dir_cluster >> 16;
    dot->DIR_FileSize = 0;

    /* ".." entry */
    memset(dotdot, 0, sizeof(DirEntry));
    memset(dotdot->DIR_Name, ' ', 11);
    dotdot->DIR_Name[0] = '.';
    dotdot->DIR_Name[1] = '.';
    dotdot->DIR_Attr = ATTR_DIRECTORY;
    dotdot->DIR_FstClusLO = parent_cluster & 0xFFFF;
    dotdot->DIR_FstClusHI = parent_cluster >> 16;
    dotdot->DIR_FileSize = 0;

    bool ok = fat32_write_cluster( dir_cluster, buf);
    free(buf);
    return ok;
}

// This function creates a new directory with the specified name under the given parent directory cluster.
static bool fat32_mkdir_internal( uint32_t parent_cluster, const char *name) {
    uint32_t new_cluster;

    if (!fat32_allocate_cluster( &new_cluster))
        return false;

    fat32_set_next_cluster( new_cluster, CLUSTER_END_OF_CHAIN);   // Mark it as end of chain

    fat32_clear_cluster( new_cluster);

    if (!fat32_init_directory( new_cluster, parent_cluster))
        return false;

    return fat32_create_dir_entry( parent_cluster, name, ATTR_DIRECTORY, new_cluster, 0 );
}

static bool fat32_dir_exists( uint32_t dir_cluster, const char *name) {
    uint32_t cluster_size = get_cluster_size_bytes();
    uint8_t *buf = malloc(cluster_size);
    if (!buf) return false;

    uint32_t curr = dir_cluster;
    char short_name[11];
    fat32_format_83_name(name, short_name);

    while (is_valid_cluster(curr)) {
        fat32_read_cluster( curr, buf);

        for (uint32_t off = 0; off < cluster_size; off += 32) {
            DirEntry *e =
                (DirEntry *)(buf + off);

            if (e->DIR_Name[0] == 0x00) goto done;
            if (e->DIR_Name[0] == 0xE5) continue;
            if (e->DIR_Attr == ATTR_LONG_NAME) continue;

            if (memcmp(e->DIR_Name, short_name, 11) == 0) {
                free(buf);
                return true;
            }
        }

        uint32_t next = fat32_get_next_cluster( curr);
        if (is_end_of_cluster_chain(next)) break;
        curr = next;
    }

    done:
        free(buf);

    return false;
}

bool fat32_mount( uint64_t partition_lba_start) {
    fat32_base_lba = partition_lba_start;

    uint8_t sector[512];

    if(!disk_read( partition_lba_start, 1, sector)){
        printf("FAT32: boot sector read failed\n");
        return false;
    }

    if(!bpb){
        bpb = malloc(sizeof(BPB));
        if (!bpb) {
            printf("FAT32: BPB alloc failed\n");
            return false;
        }
    }
    
    memcpy(bpb, sector, sizeof(BPB));

    /* Validate FAT32 */
    if (bpb->BPB_BytsPerSec != 512) {
        printf("FAT32: invalid sector size\n");
        return false;
    }

    if (bpb->BPB_FATSz32 == 0) {
        printf("FAT32: not FAT32\n");
        return false;
    }

    if (bpb->BPB_NumFATs == 0) {
        printf("FAT32: invalid FAT count\n");
        return false;
    }

    if (bpb->BPB_SecPerClus == 0) {
        printf("FAT32: invalid cluster size\n");
        return false;
    }

    fat32_cwd_cluster = bpb->BPB_RootClus;


    // printf("FAT32 mounted\n");
    // printf("Bytes/sector: %u\n", bpb->BPB_BytsPerSec);
    // printf("Sectors/cluster: %u\n", bpb->BPB_SecPerClus);
    // printf("Reserved sectors: %u\n", bpb->BPB_RsvdSecCnt);
    // printf("FAT size: %u\n", bpb->BPB_FATSz32);
    // printf("Root cluster: %u\n", bpb->BPB_RootClus);
    // printf("Total clusters: %u\n", get_total_clusters());

    return true;
}

// This function searches for a directory entry with the specified name in the given directory cluster and returns its starting cluster if found.
static bool fat32_find_dir( uint32_t dir_cluster, const char *name, uint32_t *out_cluster)
{
    uint32_t cluster_size = bpb->BPB_BytsPerSec * bpb->BPB_SecPerClus;

    uint8_t *buf = malloc(cluster_size);
    if (!buf) return false;
    memset(buf, 0, cluster_size);

    if (!fat32_read_cluster_chain( dir_cluster, buf, cluster_size)) {
        free(buf);
        printf("Reading Cluster Chain failed\n");
        return false;
    }

    char name83[12];
    fat32_format_83_name(name, name83);

    name83[11] = '\0';

    // printf("[FAT32_FIND_DIR] Directory name: %s is searching!\n", name83);

    DirEntry *entry = (DirEntry *)buf;
    uint32_t entries = cluster_size / sizeof(DirEntry);

    for (uint32_t i = 0; i < entries; i++) {
        if (entry[i].DIR_Name[0] == 0x00) break;
        if (entry[i].DIR_Name[0] == 0xE5) continue;

        if ((entry[i].DIR_Attr & ATTR_DIRECTORY) &&  memcmp(entry[i].DIR_Name, name83, 11) == 0)
        {
            *out_cluster =  (entry[i].DIR_FstClusHI << 16) |  entry[i].DIR_FstClusLO;

            free(buf);
            return true;
        }
    }

    free(buf);
    
    // printf("[FAT32_find_dir] No directory entry found with given name %s in given cluster %llu\n", name, dir_cluster);

    return false;
}


// This function creates a new file with the specified name and content under the given parent directory cluster.
static bool fat32_create_file_in_dir( uint32_t parent_cluster, const char *filename, const char *content, uint32_t size)
{
    uint32_t first_cluster = 0;

    if (!fat32_write_cluster_chain( content, size, &first_cluster))
        return false;

    return fat32_create_dir_entry( parent_cluster, filename, ATTR_ARCHIVE, first_cluster,  size);
}

// This function changes the current working directory to the specified path, which can be either absolute or relative.
bool fat32_change_current_directory( const char *path)
{
    if (!path || !path[0])
        return false;

    uint32_t current;

    /* absolute path */
    if (path[0] == '/')
        current = bpb->BPB_RootClus;
    else
        current = fat32_cwd_cluster;

    /* root */
    if (strcmp(path, "/") == 0) {
        fat32_cwd_cluster = bpb->BPB_RootClus;
        return true;
    }

    char tmp[256];
    strcpy(tmp, path);

    char *token = strtok(tmp, "/");

    while (token) {

        if (strcmp(token, ".") == 0) {
            /* do nothing */
        }
        else if (strcmp(token, "..") == 0) {
            /* read parent from ".." entry */
            uint32_t parent;
            if (fat32_find_dir( current, "..", &parent))
                current = parent;
        }
        else {
            uint32_t next;
            if (!fat32_find_dir( current, token, &next)) {
                printf("Directory not found: %s\n", token);
                return false;
            }
            current = next;
        }

        token = strtok(NULL, "/");
    }

    fat32_cwd_cluster = current;

    return true;
}


bool fat32_path_to_cluster( const char *path, uint32_t *out_cluster)
{
    if (!path || !out_cluster || !bpb)
        return false;

    char *path_copy = strdup(path);
    if (!path_copy)
        return false;

    uint32_t cluster;

    // Absolute path → start from root
    if (path[0] == '/') {
        cluster = bpb->BPB_RootClus;
    } else {
        cluster = fat32_cwd_cluster;
    }

    char *saveptr;

    char *token = strtok(path_copy, "/");

    while (token) {
        if (strcmp(token, ".") == 0) {
            token = strtok(NULL, "/");
            continue;
        }

        if (strcmp(token, "..") == 0) {
            uint32_t parent;
            if (!fat32_find_dir( cluster, "..", &parent)) {
                free(path_copy);
                return false;
            }
            cluster = parent;
            token = strtok(NULL, "/");
            continue;
        }

        uint32_t next_cluster;
        if (!fat32_find_dir( cluster, token, &next_cluster)) {
            free(path_copy);
            return false;
        }

        cluster = next_cluster;
        token = strtok(NULL, "/");
    }

    free(path_copy);

    *out_cluster = cluster;

    return true;
}



bool fat32_mkdir( const char* dirpath) {
    if (!dirpath || !bpb) return false;

    char path_copy[256];
    strncpy(path_copy, dirpath, sizeof(path_copy));

    path_copy[sizeof(path_copy) - 1] = '\0';

    // Remove trailing '/'
    size_t len = strlen(path_copy);
    if (len > 1 && path_copy[len - 1] == '/'){
        path_copy[len - 1] = '\0';
    }

    char *last_slash = strrchr(path_copy, '/');

    uint32_t parent_cluster;
    char *dirname;

    if (!last_slash) {
        parent_cluster = fat32_cwd_cluster;
        dirname = path_copy;
    }
    else if (last_slash == path_copy) {
        // parent is root
        parent_cluster = bpb->BPB_RootClus;
        dirname = last_slash + 1;
    }
    else {
        *last_slash = '\0';
        dirname = last_slash + 1;

        if (!fat32_path_to_cluster( path_copy, &parent_cluster))
            return false;
    }

    if (strlen(dirname) == 0)
        return false;

    // already exists?
    if (fat32_dir_exists( parent_cluster, dirname))
        return false;

    return fat32_mkdir_internal( parent_cluster, dirname);
        
}

static bool fat32_find_file(  uint32_t dir_cluster, const char *name, DirEntry *out_entry, uint32_t *entry_cluster, uint32_t *entry_offset)
{
    uint32_t cluster_size = bpb->BPB_BytsPerSec * bpb->BPB_SecPerClus;
    

    uint8_t *buf = malloc(cluster_size);
    if (!buf) return false;

    if (!fat32_read_cluster_chain( dir_cluster, buf, cluster_size)) {
        free(buf);
        return false;
    }

    char name83[11];
    fat32_format_83_name(name, name83);

    DirEntry *entry = (DirEntry *)buf;
    uint32_t entries = cluster_size / sizeof(DirEntry);

    for (uint32_t i = 0; i < entries; i++) {
        if (entry[i].DIR_Name[0] == 0x00) break;
        if (entry[i].DIR_Name[0] == 0xE5) continue;

        if (!(entry[i].DIR_Attr & ATTR_DIRECTORY) &&
            memcmp(entry[i].DIR_Name, name83, 11) == 0)
        {
            memcpy(out_entry, &entry[i], sizeof(DirEntry));

            *entry_cluster = dir_cluster;
            *entry_offset = i;

            free(buf);
            return true;
        }
    }

    free(buf);

    return false;
}

bool fat32_open( const char *path, FAT32_FILE *file)
{
    if (!file || !path)
        return false;

    char tmp[256];
    strcpy(tmp, path);

    char *last = strrchr(tmp, '/');

    uint32_t parent_cluster;
    char *filename;

    if (!last) {
        parent_cluster = fat32_cwd_cluster;
        filename = tmp;
    } else if (last == tmp) {
        parent_cluster = bpb->BPB_RootClus;
        filename = last + 1;
    } else {
        *last = '\0';
        filename = last + 1;

        if (!fat32_path_to_cluster( tmp, &parent_cluster)){
            return false;
        }  
    }

    DirEntry entry;
    uint32_t ec, eo;

    if (!fat32_find_file( parent_cluster, filename, &entry, &ec, &eo))
        return false;

    file->first_cluster = (entry.DIR_FstClusHI << 16) | entry.DIR_FstClusLO;

    file->size = entry.DIR_FileSize;
    file->pos = 0;
    file->parent_cluster = parent_cluster;
    memcpy(file->name, entry.DIR_Name, 11);

    return true;
}



uint32_t fat32_read( FAT32_FILE *file, void *buffer, uint32_t size)
{
    if(!file || !buffer){
        return 0;
    }

    if(file->first_cluster == 0){
        printf("file->first_cluster:%d\n", file->first_cluster);
        return 0;
    }
        

    if(file->pos >= file->size){
        printf("file->pos: %d, file->size: %d\n", file->pos, file->size);
        return 0;
    }
       

    uint32_t remaining = file->size - file->pos;
    if(size > remaining){
        size = remaining;
    }
        
    if(!fat32_read_cluster_chain( file->first_cluster, buffer, size)){
        return 0;
    }
        
    file->pos += size;

    return size;
}



uint32_t fat32_write( FAT32_FILE *file, const void *buffer, uint32_t size)
{
    if (!file || !buffer)
        return 0;

    fat32_free_cluster_chain( file->first_cluster);

    uint32_t new_cluster;

    if (!fat32_write_cluster_chain(  buffer, size,  &new_cluster))
        return 0;

    file->first_cluster = new_cluster;
    file->size = size;
    file->pos = size;

    return size;
}



bool fat32_mkdir_root( const char *name) {
    uint32_t root = bpb->BPB_RootClus;

    if (fat32_dir_exists( root, name)) {
        printf("Directory already exists\n");
        return false;
    }

    if (!fat32_mkdir_internal( root, name)) {
        printf("mkdir failed\n");
        return false;
    }

    return true;
}



// Testing 8.3 Filename FAT32 Test
bool fat32_test( uint64_t fat_base_lba){

    if(!fat32_mount( fat_base_lba)){
        printf("[FAT32 TEST] Failed to Mount FAT32 FS at LBA: %d!\n", fat_base_lba);
        return false;
    }
    printf("[FAT32 TEST] Successfully Mount Disk.\n");
    

    uint32_t root_cluster = bpb->BPB_RootClus;
    
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
        printf("[FAT32 TEST] Failed to get Cluster no for %s", dir_path);
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




// LFN

uint8_t fat32_lfn_checksum(const uint8_t short_name[11])
{
    uint8_t sum = 0;
    for (int i = 0; i < 11; i++)
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + short_name[i];
    return sum;
}




















