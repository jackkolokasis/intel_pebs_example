# Compiler
CC = gcc
CXX = g++

# Compiler flags
CFLAGS = -Wall -O3 -Werror
LDFLAGS = -lpthread

# Executable
TARGET = test_pebs cache_misses cache_misses_samples count_loads \
				 count_stores count_load_pebs count_load_pebs_hierachy \
				 count_store_pebs_hierachy seq_read rand_read seq_write \
				 rand_write
 
# Directory for object files
OBJ_DIR = obj

# Default target
all: $(TARGET)

# Link the object files to create the executable
rand_read: $(OBJ_DIR)/pebs.o $(OBJ_DIR)/rand_read.o
	$(CXX) $^ -o $@ -lpthread

seq_read: $(OBJ_DIR)/pebs.o $(OBJ_DIR)/seq_read.o
	$(CXX) $^ -o $@ -lpthread

seq_write: $(OBJ_DIR)/pebs.o $(OBJ_DIR)/seq_write.o
	$(CXX) $^ -o $@ -lpthread

rand_write: $(OBJ_DIR)/pebs.o $(OBJ_DIR)/rand_write.o
	$(CXX) $^ -o $@ -lpthread

$(OBJ_DIR)/rand_write.o: src/rand_write.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/seq_write.o: src/seq_write.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/rand_read.o: src/rand_read.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/seq_read.o: src/seq_read.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/pebs.o: src/pebs.cpp include/pebs.h
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CFLAGS) -c $< -o $@

cache_misses: src/cache_miss_benchmark.c
	$(CC) $(CFLAGS) $< -o $@

cache_misses_samples: src/cache_miss_sample_benchmark.c
	$(CC) $(CFLAGS) $< -o $@

count_loads: src/count_loads.c
	$(CC) $(CFLAGS) $< -o $@

count_stores: src/count_stores.c
	$(CC) $(CFLAGS) $< -o $@

count_load_pebs: src/count_loads_pebs.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

count_load_pebs_hierachy: src/count_loads_pebs_hierarchy.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

count_store_pebs_hierachy: src/count_stores_pebs_hierarchy.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -rf $(TARGET) $(OBJ_DIR) *.out

run:
	./count_load_pebs_hierachy > load_pebs_hierarchy.out
	./count_store_pebs_hierachy > store_pebs_hierarchy.out
