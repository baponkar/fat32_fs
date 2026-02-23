# FAT32 Filesystem

## Creating a blank disk image at directory disk_img
```
dd if=/dev/zero of=disk_img/disk.img bs=1M count=1024
```

## Build
```
make
```

## Run
```
./fat32_test
```

## Help
```
make help
```

