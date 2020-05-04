# TODO: make it compile with -Wconversion
COMMONFLAGS=-Wall -Werror -Wextra -pedantic -I. -ggdb
CFLAGS=$(COMMONFLAGS) -std=c11
CXXFLAGS=$(COMMONFLAGS) -std=c++17 -fno-exceptions

.PHONY: all
all: tzozen_test dump_ast dump_json examples/01_basic_usage

tzozen_test: tzozen_test.c tzozen.h
	$(CC) $(CFLAGS) -o tzozen_test tzozen_test.c

# This example is compiled with C++ compiler to test the library
# compatibility with C++. We have enough C code in this repo to test
# the the C compatibility.
examples/01_basic_usage: examples/01_basic_usage.cpp tzozen.h
	$(CXX) $(CXXFLAGS) -o examples/01_basic_usage examples/01_basic_usage.cpp

dump_ast: dump_ast.c tzozen.h tzozen_dump.h
	$(CC) $(CFLAGS) -o dump_ast dump_ast.c

dump_json: dump_json.c tzozen.h tzozen_dump.h
	$(CC) $(CFLAGS) -o dump_json dump_json.c

.PHONY: clean
clean:
	rm -rfv tzozen_test tzozen_check
