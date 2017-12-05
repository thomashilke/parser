CXX = clang++
DEPS_BIN = g++
DEPSFLAGS = -I$(HOME)/.local/include
CXXFLAGS = -g -std=c++14 -Wall -Wextra -I$(HOME)/.local/include
LDFLAGS = -g -Wall -Wextra -L$(HOME)/.local/lib
LDLIBS =  -llexer
AR = ar
ARFLAGS = rc
MKDIR = mkdir
MKDIRFLAGS = -p

PREFIX = ~/.local/
BIN_DIR = bin/
INCLUDE_DIR = include/
LIB_DIR = lib/


SOURCES = src/pgtool.cpp src/regex/regex.cpp src/regex/regexparser.cpp \
	test/cf_grammar.cpp test/lr_parser.cpp test/parse_input.cpp test/parse_input_to_tree.cpp \
	test/grammar_experiment.cpp


HEADERS = 

BIN = bin/test_cf_grammar bin/test_lr_parser bin/test_parse_input bin/test_parse_input_to_tree bin/grammar_experiment

bin/pgtool: build/src/pgtool.o build/src/regex/regex.o build/src/regex/regexparser.o
bin/test_cf_grammar: build/test/cf_grammar.o
bin/test_lr_parser: build/test/lr_parser.o
bin/test_parse_input: build/test/parse_input.o
bin/test_parse_input_to_tree: build/test/parse_input_to_tree.o
bin/grammar_experiment: build/test/grammar_experiment.o

LIB = 

#lib/...: ...
