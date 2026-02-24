![Badge](https://img.shields.io/badge/FAT32-FS-blue)


# FAT32 Filesystem

## Description
---
This repository will help to create FAT32 Filesystem and test that filesystem in raw disk image.In current version do not support Long file/directory name.

Here disk inpot/output is using following two functions

```
bool disk_read(uint64_t lba, uint32_t count, void* buffer);
bool disk_write(uint64_t lba, uint32_t count, const void* buffer)
```

To build the project

```
make -B
```

To make a raw disk image
```
make disk
```

To test
```
make run
```

Output should be:
```bash
./build/fat32_test
Creating FAT32 Volume at LBA 0 with 204800 sectors
  Initializing FAT 1... Done
  Initializing FAT 2... Done
[FAT32 TEST] Successfully Mount Disk.
[FAT32 TEST] Creating Directory TESTDIR is success.
[FAT32 TEST] Successfully get Cluster no 3 for directory TESTDIR
[FAT32 TEST] Successfully created TESTFILE.TXT

[FAT32 TEST] Successfully open file TESTDIR/TESTFILE.TXT
[FAT32 TEST] Successfully read 56 bytes
[FAT32 TEST] File content: This is a test text string for testing fat32 filesystem.�
FAT32 test passed successfully!
```