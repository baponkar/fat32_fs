
#include "../test/fat32_test_1.h"
#include "../test/diskio_test_1.h"

#include "../include/main.h"





int main() {

    if(!diskio_test()) {
        printf("Disk I/O test failed\n");
        return 1;
    }
    printf("Disk I/O test passed successfully!\n");

    extern uint32_t fat32_base_lba;
    if (!fat32_test( fat32_base_lba)) {
        printf("FAT32 test failed\n");
        return 1;
    }
    printf("FAT32 test passed successfully!\n");

    return 0;
}


