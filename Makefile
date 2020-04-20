CFLAGS=-Wall -Werror -Wextra -std=c11 -pedantic

.PHONY: all
all: tzozen_test tzozen_check example dump_ast dump_json

tzozen_test: tzozen_test.c tzozen.h
	$(CC) $(CFLAGS) -o tzozen_test tzozen_test.c

tzozen_check: tzozen_check.c tzozen.h
	$(CC) $(CFLAGS) -o tzozen_check tzozen_check.c

example: example.c tzozen.h
	$(CC) $(CFLAGS) -o example example.c

dump_ast: dump_ast.c tzozen.h tzozen_dump.h
	$(CC) $(CFLAGS) -o dump_ast dump_ast.c

dump_json: dump_json.c tzozen.h tzozen_dump.h
	$(CC) $(CFLAGS) -o dump_json dump_json.c

.PHONY: clean
clean:
	rm -rfv tzozen_test tzozen_check
