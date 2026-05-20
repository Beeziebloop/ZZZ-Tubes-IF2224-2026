# Arion Compiler Makefile
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

# Direktori
SRC_DIR  = src
BIN_DIR  = bin

# File source
SRCS     = $(SRC_DIR)/main.cpp $(SRC_DIR)/lex_analyzer.cpp $(SRC_DIR)/parser.cpp \
           $(SRC_DIR)/ast.cpp $(SRC_DIR)/ast_builder.cpp \
           $(SRC_DIR)/symbol_table.cpp $(SRC_DIR)/semantic_analyzer.cpp
TARGET   = $(BIN_DIR)/lexer.exe

# Default target
all: $(BIN_DIR) $(TARGET)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

# Run lexer: make run INPUT=test/milestone-2/input-1.txt
# Output akan otomatis dibuat sebagai output-1.txt di folder yang sama
run: all
	./$(TARGET) $(INPUT) $(dir $(INPUT))$(subst input-,output-,$(notdir $(INPUT)))

# Run dengan token output file: make runout INPUT=... OUTPUT=...
runout: all
	./$(TARGET) $(INPUT) $(OUTPUT)

# Run dengan parse tree output saja: make runparse INPUT=... TREEOUT=...
runparse: all
	./$(TARGET) $(INPUT) $(TREEOUT)

# Shortcut test milestone 1
test1: all
	mkdir -p test/milestone-1
	./$(TARGET) test/milestone-1/input-1.txt test/milestone-1/output-1.txt

# Shortcut test milestone 2
test2: all
	mkdir -p test/milestone-2
	./$(TARGET) test/milestone-2/input-1.txt test/milestone-2/output-1.txt

# Test milestone 2 dengan input file 1-5
test2-1: all
	mkdir -p test/milestone-2
	./$(TARGET) test/milestone-2/input-1.txt test/milestone-2/output-1.txt

test2-2: all
	mkdir -p test/milestone-2
	./$(TARGET) test/milestone-2/input-2.txt test/milestone-2/output-2.txt

test2-3: all
	mkdir -p test/milestone-2
	./$(TARGET) test/milestone-2/input-3.txt test/milestone-2/output-3.txt

test2-4: all
	mkdir -p test/milestone-2
	./$(TARGET) test/milestone-2/input-4.txt test/milestone-2/output-4.txt

test2-5: all
	mkdir -p test/milestone-2
	./$(TARGET) test/milestone-2/input-5.txt test/milestone-2/output-5.txt

# Shortcut test milestone 3 semua input sekaligus (full output)
test3: all
	mkdir -p test/milestone-3
	./$(TARGET) test/milestone-3/input-1.txt test/milestone-3/output-1.txt --full
	./$(TARGET) test/milestone-3/input-2.txt test/milestone-3/output-2.txt --full
	./$(TARGET) test/milestone-3/input-3.txt test/milestone-3/output-3.txt --full
	./$(TARGET) test/milestone-3/input-4.txt test/milestone-3/output-4.txt --full
	./$(TARGET) test/milestone-3/input-5.txt test/milestone-3/output-5.txt --full

test3-1: all
	mkdir -p test/milestone-3
	./$(TARGET) test/milestone-3/input-1.txt test/milestone-3/output-1.txt --full

test3-2: all
	mkdir -p test/milestone-3
	./$(TARGET) test/milestone-3/input-2.txt test/milestone-3/output-2.txt --full

test3-3: all
	mkdir -p test/milestone-3
	./$(TARGET) test/milestone-3/input-3.txt test/milestone-3/output-3.txt --full

test3-4: all
	mkdir -p test/milestone-3
	./$(TARGET) test/milestone-3/input-4.txt test/milestone-3/output-4.txt --full

test3-5: all
	mkdir -p test/milestone-3
	./$(TARGET) test/milestone-3/input-5.txt test/milestone-3/output-5.txt --full

# Bersihkan hasil kompilasi
clean:
	rm -rf $(BIN_DIR)

.PHONY: all run runout runparse test1 test2 test3 \
        test3-1 test3-2 test3-3 test3-4 test3-5 clean
