# FAT32 Filesystem

## Description
---
This repository will help to create FAT32 Filesystem and test that filesystem in raw disk image.

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