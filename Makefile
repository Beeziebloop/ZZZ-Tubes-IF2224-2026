# Arion Lexer Makefile
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

# Direktori
SRC_DIR  = src
BIN_DIR  = bin

# File — sesuaikan dengan nama file kamu
SRCS     = $(SRC_DIR)/main.cpp $(SRC_DIR)/lex_analyzer.cpp
TARGET   = $(BIN_DIR)/lexer

# Default target
all: $(BIN_DIR) $(TARGET) 

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

# Run: make run INPUT=test/milestone-1/input-1.txt
run: all
	./$(TARGET) $(INPUT)

# Run dengan output file: make runout INPUT=... OUTPUT=...
runout: all
	./$(TARGET) $(INPUT) $(OUTPUT)

# Shortcut untuk milestone 1
test: all
	mkdir -p test/milestone-1
	./$(TARGET) test/milestone-1/input-1.txt test/milestone-1/output-1.txt

# Bersihkan hasil kompilasi
clean:
	rm -rf $(BIN_DIR)

.PHONY: all run runout test clean