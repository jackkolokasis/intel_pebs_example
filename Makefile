# Compiler
CC = gcc
CXX = g++

# Compiler flags
CFLAGS = -Wall -O3

# Executable
TARGET = test_pebs cache_misses cache_misses_samples count_loads \
				 count_stores

# Directory for object files
OBJ_DIR = obj

# Default target
all: $(TARGET)

# Link the object files to create the executable
test_pebs: $(OBJ_DIR)/pebs.o $(OBJ_DIR)/main.o
	$(CXX) $^ -o $@

$(OBJ_DIR)/pebs.o: src/pebs.cpp include/pebs.h
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/main.o: src/main.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

cache_misses: src/cache_miss_benchmark.c
	$(CC) $(CFLAGS) $< -o $@

cache_misses_samples: src/cache_miss_sample_benchmark.c
	$(CC) $(CFLAGS) $< -o $@

count_loads: src/count_loads.c
	$(CC) $(CFLAGS) $< -o $@

count_stores: src/count_stores.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(TARGET) $(OBJ_DIR)
