CFLAGS=-Wall -Werror -Wextra

.PHONY: all
all: tzozen_test tzozen_check

tzozen_test: tzozen_test.c tzozen.h
	$(CC) $(CFLAGS) -o tzozen_test tzozen_test.c

tzozen_check: tzozen_check.c tzozen.h
	$(CC) $(CFLAGS) -o tzozen_check tzozen_check.c

.PHONY: clean
clean:
	rm -rfv tzozen_test tzozen_check
