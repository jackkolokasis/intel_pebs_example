# Compiler
CC = gcc
CXX = g++

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
LIB_DIR = lib
TEST_DIR = tests
BIN_DIR = bin

# Files
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
TARGET_LIB = $(LIB_DIR)/libpebs.so
TEST_FILES = $(wildcard $(TEST_DIR)/*.cpp)
TEST_EXECUTABLES = $(TEST_FILES:$(TEST_DIR)/%.cpp=$(BIN_DIR)/%)

CXXFLAGS = -fPIC -O2 -I$(INC_DIR)
LDFLAGS = -shared -lpthread
TEST_LDFLAGS = -L$(LIB_DIR) -lpebs -lpthread

# Create necessary directories
$(shell mkdir -p $(OBJ_DIR) $(LIB_DIR) $(BIN_DIR))

# Default target
all: $(TARGET_LIB) $(TEST_EXECUTABLES)

# Rule to build the shared library
$(TARGET_LIB): $(OBJ_FILES)
	$(CXX) $(LDFLAGS) -o $@ $^

# Rule to compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Rule to compile testfiles
$(BIN_DIR)/%: $(TEST_DIR)/%.cpp $(TARGET_LIB)
	$(CXX) -O2 -I$(INC_DIR) -o $@ $< $(TEST_LDFLAGS)

clean:
	rm -rf $(OBJ_DIR) $(LIB_DIR) $(BIN_DIR)

# PHONY targets
.PHONY: all clean
