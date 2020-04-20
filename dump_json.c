#include <stdio.h>
#include <stdlib.h>

// TODO: different capacities produce incompatible memory dumps
#define JSON_ARRAY_PAGE_CAPACITY 1
#define JSON_OBJECT_PAGE_CAPACITY 1
#include "./tzozen_dump.h"

#define MEMORY_CAPACITY (10 * MEGA)

void usage(FILE *stream)
{
    fprintf(stream, "Usage: dump_json <input.bin>\n");
    fprintf(stream, "   input.bin         Memory dump file produced by dump_ast\n");
}

int main(int argc, char *argv[])
{
    Memory memory = {0};
    memory.capacity = MEMORY_CAPACITY;
    memory.buffer = calloc(1, memory.capacity);
    if (memory.buffer == NULL) {
        fprintf(stderr, "Could not allocate enough memory: %s\n", strerror(errno));
        exit(1);
    }

    if (argc < 2) {
        fprintf(stderr, "[ERROR] Not enough arguments!\n");
        usage(stderr);
        exit(1);
    }

    const char *input_file_path = argv[1];
    load_memory_from_file(&memory, input_file_path);

    Json_Value *index = (Json_Value *)memory.buffer;
    json_value_unrelatify(&memory, index);
    print_json_value(stdout, *index);

    free(memory.buffer);

    return 0;
}
