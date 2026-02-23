CC = gcc
CFLAGS = -Wall -Wextra -std=c11

BUILD_DIR = build
SRC_DIR = src

TARGET = $(BUILD_DIR)/fat32_test

SRC = $(SRC_DIR)/diskio.c \
      $(SRC_DIR)/fat32.c \
      $(SRC_DIR)/main.c

# Strip src/ when generating object names
OBJ = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET)

help:
	@echo "Usage:"
	@echo "  make        - Build the project"
	@echo "  make clean  - Clean the build directory"
	@echo "  make run    - Run the compiled program"
	@echo "  make help   - Show this help message"

.PHONY: all clean run help


