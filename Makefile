
# Last edited: 2024-06-30

CC = gcc
CFLAGS = -Wall -Wextra -std=c11

BUILD_DIR = build
SRC_DIR = src
DISK_DIR = disk_img
DISK_PATH = $(DISK_DIR)/disk.img

TARGET = $(BUILD_DIR)/fat32_test

SRC = $(SRC_DIR)/diskio.c \
      $(SRC_DIR)/fat32.c \
      $(SRC_DIR)/cluster_manager.c \
      $(SRC_DIR)/main.c

# Object files will be placed in the build directory, mirroring the source structure
OBJ = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Ensure disk directory + disk image exists
disk:
	@mkdir -p $(DISK_DIR)
	@if [ ! -f $(DISK_PATH) ]; then \
		echo "Creating 100MB disk image..."; \
		dd if=/dev/zero of=$(DISK_PATH) bs=1M count=100; \
	else \
		echo "Disk image already exists."; \
	fi

clean:
	rm -rf $(BUILD_DIR)

run: disk $(TARGET)
	./$(TARGET)

help:
	@echo "Usage:"
	@echo "  make        - Build the project"
	@echo "  make disk   - Create disk image if not present"
	@echo "  make run    - Create disk (if needed) and run"
	@echo "  make clean  - Clean build directory"

.PHONY: all disk clean run help