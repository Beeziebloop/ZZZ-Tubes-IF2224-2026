# Arion Compiler Makefile
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

# Direktori
SRC_DIR  = src
BIN_DIR  = bin
T1       = test/milestone-1
T2       = test/milestone-2
T3       = test/milestone-3

# File source
SRCS     = $(SRC_DIR)/main.cpp $(SRC_DIR)/lex_analyzer.cpp $(SRC_DIR)/parser.cpp \
           $(SRC_DIR)/ast.cpp $(SRC_DIR)/ast_builder.cpp \
           $(SRC_DIR)/symbol_table.cpp $(SRC_DIR)/semantic_analyzer.cpp
TARGET   = $(BIN_DIR)/lexer.exe

#  Build 
all: $(BIN_DIR) $(TARGET)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

#  M1: tokens only 
test1:   all; mkdir -p $(T1)
	./$(TARGET) $(T1)/input-1.txt $(T1)/output-1.txt --m1

test1-1: all; mkdir -p $(T1)
	./$(TARGET) $(T1)/input-1.txt $(T1)/output-1.txt --m1
test1-2: all; mkdir -p $(T1)
	./$(TARGET) $(T1)/input-2.txt $(T1)/output-2.txt --m1
test1-3: all; mkdir -p $(T1)
	./$(TARGET) $(T1)/input-3.txt $(T1)/output-3.txt --m1
test1-4: all; mkdir -p $(T1)
	./$(TARGET) $(T1)/input-4.txt $(T1)/output-4.txt --m1
test1-5: all; mkdir -p $(T1)
	./$(TARGET) $(T1)/input-5.txt $(T1)/output-5.txt --m1

#  M2: tokens + parse tree 
test2:   all; mkdir -p $(T2)
	./$(TARGET) $(T2)/input-1.txt $(T2)/output-1.txt --m2
	./$(TARGET) $(T2)/input-2.txt $(T2)/output-2.txt --m2
	./$(TARGET) $(T2)/input-3.txt $(T2)/output-3.txt --m2
	./$(TARGET) $(T2)/input-4.txt $(T2)/output-4.txt --m2
	./$(TARGET) $(T2)/input-5.txt $(T2)/output-5.txt --m2

test2-1: all; mkdir -p $(T2)
	./$(TARGET) $(T2)/input-1.txt $(T2)/output-1.txt --m2
test2-2: all; mkdir -p $(T2)
	./$(TARGET) $(T2)/input-2.txt $(T2)/output-2.txt --m2
test2-3: all; mkdir -p $(T2)
	./$(TARGET) $(T2)/input-3.txt $(T2)/output-3.txt --m2
test2-4: all; mkdir -p $(T2)
	./$(TARGET) $(T2)/input-4.txt $(T2)/output-4.txt --m2
test2-5: all; mkdir -p $(T2)
	./$(TARGET) $(T2)/input-5.txt $(T2)/output-5.txt --m2

#  M3: tokens + parse tree + decorated AST + symbol tables 
test3:   all; mkdir -p $(T3)
	./$(TARGET) $(T3)/input-1.txt $(T3)/output-1.txt --m3
	./$(TARGET) $(T3)/input-2.txt $(T3)/output-2.txt --m3
	./$(TARGET) $(T3)/input-3.txt $(T3)/output-3.txt --m3
	./$(TARGET) $(T3)/input-4.txt $(T3)/output-4.txt --m3
	./$(TARGET) $(T3)/input-5.txt $(T3)/output-5.txt --m3

test3-1: all; mkdir -p $(T3)
	./$(TARGET) $(T3)/input-1.txt $(T3)/output-1.txt --m3
test3-2: all; mkdir -p $(T3)
	./$(TARGET) $(T3)/input-2.txt $(T3)/output-2.txt --m3
test3-3: all; mkdir -p $(T3)
	./$(TARGET) $(T3)/input-3.txt $(T3)/output-3.txt --m3
test3-4: all; mkdir -p $(T3)
	./$(TARGET) $(T3)/input-4.txt $(T3)/output-4.txt --m3
test3-5: all; mkdir -p $(T3)
	./$(TARGET) $(T3)/input-5.txt $(T3)/output-5.txt --m3

#  Run manual: make run INPUT=... OUTPUT=... FLAG=--m3 
run: all
	./$(TARGET) $(INPUT) $(OUTPUT) $(FLAG)

#  Bersihkan 
clean:
	rm -rf $(BIN_DIR)

.PHONY: all run clean \
        test1 test1-1 test1-2 test1-3 test1-4 test1-5 \
        test2 test2-1 test2-2 test2-3 test2-4 test2-5 \
        test3 test3-1 test3-2 test3-3 test3-4 test3-5
