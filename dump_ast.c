#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "./tzozen_dump.h"

#define MEMORY_CAPACITY (100 * MEGA)
#define ARRAY_SIZE(xs) (sizeof(xs) / sizeof((xs)[0]))

uint8_t memory_buffer[MEMORY_CAPACITY];
Memory memory = {
    .capacity = MEMORY_CAPACITY,
    .buffer = memory_buffer,
};

void usage(FILE *stream)
{
    fprintf(stream, "Usage: dump_ast <input.json> <output.bin>\n");
}

char output_file_path[1024];

int main(int argc, char *argv[])
{
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
    if (argc >= 3) {
        snprintf(output_file_path, ARRAY_SIZE(output_file_path), "%s", argv[2]);
    } else {
        // TODO: dump_ast should append architecture to output filename since the dumps are architecture specific
        snprintf(output_file_path, ARRAY_SIZE(output_file_path), "%s.bin", input_file_path);
    }

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

    return 0;
}
