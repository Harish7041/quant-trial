# Compiler and flags for performance and compatibility
CXX = g++
CXXFLAGS = -O3 -std=c++17 -Wall -Wextra -pedantic -march=native

# Target executable name
TARGET = reconstruction

# Source file
SRC = reconstruction.cpp

# Default build rule
all: $(TARGET)

# Rule to link the object file into the final executable
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

# Rule to clean up build artifacts
clean:
	rm -f $(TARGET) mbp.csv

# Phony targets are not files
.PHONY: all clean