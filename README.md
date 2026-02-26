[![Badge](https://img.shields.io/badge/FAT32-FS-blue)](https://github.com/baponkar/fat32_fs)


# FAT32 Filesystem

## Description
---
This repository will help to create FAT32 Filesystem and test that filesystem in raw disk image.In current version do not support Long file/directory name.

This repo has following support

✅ GPT Partition Support

✅ 8.3 File/Directory Name Support

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
make test
```

Output should be:
```bash
./build/fat32_test
Created partition 0 on drive 0: Start LBA: 2048, Sectors: 204800
Successfully created Partition at Sector 2048
Created partition 1 on drive 0: Start LBA: 206848, Sectors: 1888256
Successfully created Partition at Sector 206848
Creating FAT32 Volume at LBA 2048 with 204800 sectors
  Initializing FAT 1... Done
  Initializing FAT 2... Done
[FAT32 TEST] Successfully created FAT32 volume at LBA: 2048 with size: 204800 MB
Creating FAT32 Volume at LBA 206848 with 1888256 sectors
  Initializing FAT 1... Done
  Initializing FAT 2... Done
[FAT32 TEST] Successfully created FAT32 volume at LBA: 2048 with size: 1888256 MB
FAT32 mounted
 Volume starts at LBA: 2048
 Bytes/sector: 512
 Sectors/cluster: 2
 Reserved sectors: 32
 FAT size: 794
 Root cluster: 2
 Total clusters: 101590
[FAT32 TEST] Successfully Mount Disk.
[FAT32 TEST] Creating Directory TESTDIR is success.
[FAT32 TEST] Successfully get Cluster no 3 for directory TESTDIR
[FAT32 TEST] Successfully created TESTFILE.TXT

[FAT32 TEST] Successfully open file TESTDIR/TESTFILE.TXT
[FAT32 TEST] Successfully read 56 bytes
[FAT32 TEST] File content: This is a test text string for testing fat32 filesystem.��
FAT32 test passed successfully!
```

© 2026 baponkar. All rights reserved.