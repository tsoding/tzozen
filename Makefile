CFLAGS=-Wall -Werror -Wextra

.PHONY: all
all: tzozen_test tzozen_check example

tzozen_test: tzozen_test.c tzozen.h
	$(CC) $(CFLAGS) -o tzozen_test tzozen_test.c

tzozen_check: tzozen_check.c tzozen.h
	$(CC) $(CFLAGS) -o tzozen_check tzozen_check.c

example: example.c tzozen.h
	$(CC) $(CFLAGS) -o example example.c

.PHONY: clean
clean:
	rm -rfv tzozen_test tzozen_check
