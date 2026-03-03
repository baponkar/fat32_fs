
# Last edited: 26-02-2026

CC = gcc
CFLAGS = -Wall -Wextra -std=c11

BUILD_DIR = build
SRC_DIR = src
DISK_DIR = disk_img
TEST_DIR = test
DISK_PATH = $(DISK_DIR)/disk.img

TARGET = $(BUILD_DIR)/fat32_test

C_SRC_FILES := $(shell find $(SRC_DIR) $(TEST_DIR) -name '*.c')	# All C files
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SRC_FILES)) # Object files will be placed in the build directory, mirroring the source structure


all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
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
	rm -f $(DISK_DIR)/disk.img
	rm -rf $(BUILD_DIR)

test: disk $(TARGET) 
	./$(TARGET)

help:
	@echo "Usage:"
	@echo "  make        - Build the project"
	@echo "  make disk   - Create disk image if not present"
	@echo "  make test   - Create disk (if needed) and run test"
	@echo "  make clean  - Clean build directory"
	@echo "  make help   - Show this help message"

.PHONY: all disk clean run help