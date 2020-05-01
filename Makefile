CFLAGS=-Wall -Werror -Wextra -std=c11 -pedantic -I. -ggdb

.PHONY: all
all: tzozen_test dump_ast dump_json examples/01_basic_usage

tzozen_test: tzozen_test.c tzozen.h
	$(CC) $(CFLAGS) -o tzozen_test tzozen_test.c

examples/01_basic_usage: examples/01_basic_usage.c tzozen.h
	$(CC) $(CFLAGS) -o examples/01_basic_usage examples/01_basic_usage.c

dump_ast: dump_ast.c tzozen.h tzozen_dump.h
	$(CC) $(CFLAGS) -o dump_ast dump_ast.c

dump_json: dump_json.c tzozen.h tzozen_dump.h
	$(CC) $(CFLAGS) -o dump_json dump_json.c

.PHONY: clean
clean:
	rm -rfv tzozen_test tzozen_check
