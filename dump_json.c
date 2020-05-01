#include <stdio.h>
#include <stdlib.h>

#include "./tzozen_dump.h"

#define MEMORY_CAPACITY (100 * MEGA)
uint8_t memory_buffer[MEMORY_CAPACITY];
Memory memory = {
    .capacity = MEMORY_CAPACITY,
    .buffer = memory_buffer,
};

void usage(FILE *stream)
{
    fprintf(stream, "Usage: dump_json <input.bin>\n");
    fprintf(stream, "   input.bin         Memory dump file produced by dump_ast\n");
}

int main(int argc, char *argv[])
{
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

    return 0;
}
