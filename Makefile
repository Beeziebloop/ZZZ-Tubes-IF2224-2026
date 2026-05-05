# Arion Compiler Makefile
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

# Direktori
SRC_DIR  = src
BIN_DIR  = bin

# File source
SRCS     = $(SRC_DIR)/main.cpp $(SRC_DIR)/lex_analyzer.cpp $(SRC_DIR)/parser.cpp
TARGET   = $(BIN_DIR)/lexer

# Default target
all: $(BIN_DIR) $(TARGET)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

# Run lexer only: make run INPUT=test/milestone-1/input-1.txt
run: all
	./$(TARGET) $(INPUT)

# Run dengan token output file: make runout INPUT=... OUTPUT=...
runout: all
	./$(TARGET) $(INPUT) $(OUTPUT)

# Run dengan token + parse tree output: make runparse INPUT=... TOKOUT=... TREEOUT=...
runparse: all
	./$(TARGET) $(INPUT) $(TOKOUT) $(TREEOUT)

# Shortcut test milestone 1
test1: all
	mkdir -p test/milestone-1
	./$(TARGET) test/milestone-1/input-1.txt test/milestone-1/output-1.txt

# Shortcut test milestone 2
test2: all
	mkdir -p test/milestone-2
	./$(TARGET) test/milestone-2/input-1.txt test/milestone-2/token-1.txt test/milestone-2/output-1.txt

# Bersihkan hasil kompilasi
clean:
	rm -rf $(BIN_DIR)

.PHONY: all run runout runparse test1 test2 clean
