#include <stdlib.h>
#include "tzozen.h"

#define MEMORY_CAPACITY (640 * KILO)

static uint8_t memory_buffer[MEMORY_CAPACITY];

// TODO: move example to a separate folder

int main()
{
    Memory memory = {
        .capacity = MEMORY_CAPACITY,
        .buffer = memory_buffer
    };

    String input =
        SLT("{\n"
            "   \"null\": null,\n"
            "   \"boolean\": true,\n"
            "   \"boolean\": false,\n"
            "   \"number\": 69420,\n"
            "   \"string\": \"hello\",\n"
            "   \"array\": [null, true, false, 69420, \"hello\"],\n"
            "   \"object\": {}\n"
            "}");

    Json_Result result = parse_json_value(&memory, input);
    if (result.is_error) {
        fputs("FAILURE: \n", stdout);
        print_json_error(stdout, result, input, "<example>");
        exit(1);
    }

    fputs("SUCCESS: \n", stdout);
    print_json_value(stdout, result.value);
    fputc('\n', stdout);
    printf("MEMORY USAGE: %lu bytes\n", memory.size);

    return 0;
}
