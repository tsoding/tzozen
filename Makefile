CFLAGS=-Wall -Werror -Wextra

.PHONY: all
all: json_test json_check json.o 

json.o: json.c json.h utf8.h s.h memory.h
	$(CC) $(CFLAGS) -c json.c

json_test: json_test.c json.o json.h
	$(CC) $(CFLAGS) -o json_test json.o json_test.c

json_check: json_check.c json.o json.h
	$(CC) $(CFLAGS) -o json_check json.o json_check.c

.PHONY: clean
clean:
	rm -rfv json_test json_check json.o
