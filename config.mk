CXX = clang++
DEPS_BIN = g++
CXXFLAGS = -g -std=c++14
LDFLAGS = -g
AR = ar
ARFLAGS = rc
MKDIR = mkdir
MKDIRFLAGS = -p

PREFIX = ~/.local/
BIN_DIR = bin/
INCLUDE_DIR = include/
LIB_DIR = lib/


SOURCES = src/pgtool.cpp src/parser/parser.cpp src/regex/regex.cpp src/regex/regexparser.cpp


HEADERS = 

BIN = bin/pgtool


bin/pgtool: build/src/pgtool.o build/src/parser/parser.o build/src/regex/regex.o build/src/regex/regexparser.o

LIB = 

#lib/...: ...
