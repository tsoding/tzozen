#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

// TODO: different capacities produce incompatible memory dumps
#define JSON_ARRAY_PAGE_CAPACITY 1
#define JSON_OBJECT_PAGE_CAPACITY 1
#include "./tzozen_dump.h"

#define MEMORY_CAPACITY (10 * MEGA)

void usage(FILE *stream)
{
    fprintf(stream, "Usage: dump_ast <input.json> <output.bin>\n");
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

    if (argc < 3) {
        fprintf(stderr, "[ERROR] Not enough arguments!\n");
        usage(stderr);
        exit(1);
    }

    const char *input_file_path = argv[1];
    const char *output_file_path = argv[2];

    String input = read_file_as_string(input_file_path);

    Json_Value *index = memory_alloc(&memory, sizeof(Json_Value));

    Json_Result result = parse_json_value(&memory, input);
    if (result.is_error) {
        print_json_error(stderr, result, input, input_file_path);
        exit(1);
    }
    *index = result.value;

    json_value_relatify(&memory, index);
    dump_memory_to_file(&memory, output_file_path);

    free(memory.buffer);

    return 0;
}
