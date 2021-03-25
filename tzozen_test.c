#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>

#define TZOZEN_STATIC
#define TZOZEN_IMPLEMENTATION
#include "./tzozen_dump.h"

#define ARRAY_SIZE(xs) (sizeof(xs) / sizeof((xs)[0]))
#define TESTING_FOLDER "./tests/"

uint8_t memory_buffer[100 * 1000 * 1000];
Memory memory = {
    .capacity = ARRAY_SIZE(memory_buffer),
    .buffer = memory_buffer,
};

uint8_t dump_memory_buffer[ARRAY_SIZE(memory_buffer)];
Memory dump_memory = {
    .capacity = ARRAY_SIZE(dump_memory_buffer),
    .buffer = dump_memory_buffer,
};

char ast_dump_filepath[1024];
char json_filepath[1024];

int ends_with(const char *str, const char *postfix)
{
    size_t str_size = strlen(str);
    size_t postfix_size = strlen(postfix);
    return postfix_size <= str_size
        && strcmp(str + str_size - postfix_size, postfix) == 0;
}

int json_value_equals(Json_Value a, Json_Value b);

int json_number_equals(Json_Number a, Json_Number b)
{
    return string_equal(a.integer, b.integer)
        && string_equal(a.fraction, b.fraction)
        && string_equal(a.exponent, b.exponent);
}

int json_array_equals (Json_Array a,  Json_Array b)
{
    Json_Array_Elem *elem_a = a.begin;
    Json_Array_Elem *elem_b = b.begin;

    while (elem_a != NULL && elem_b != NULL) {
        if (!json_value_equals(elem_a->value, elem_b->value)) {
            return 0;
        }

        elem_a = elem_a->next;
        elem_b = elem_b->next;
    }

    return elem_a == NULL && elem_b == NULL;
}

int json_object_equals(Json_Object a, Json_Object b)
{
    Json_Object_Elem *elem_a = a.begin;
    Json_Object_Elem *elem_b = b.begin;

    while (elem_a != NULL && elem_b != NULL) {
        if (!string_equal(elem_a->key, elem_b->key)) {
            return 0;
        }

        if (!json_value_equals(elem_a->value, elem_b->value)) {
            return 0;
        }

        elem_a = elem_a->next;
        elem_b = elem_b->next;
    }

    return elem_a == NULL && elem_b == NULL;
}

int json_value_equals(Json_Value a, Json_Value b)
{
    if (a.type != b.type) return 0;

    switch (a.type) {
    case JSON_NULL: return 1;
    case JSON_BOOLEAN: return a.boolean == b.boolean;
    case JSON_NUMBER: return json_number_equals(a.number, b.number);
    case JSON_STRING: return string_equal(a.string, b.string);
    case JSON_ARRAY: return json_array_equals(a.array, b.array);
    case JSON_OBJECT: return json_object_equals(a.object, b.object);
    }

    return 0;
}

int main()
{
    DIR *testing_dir = opendir(TESTING_FOLDER);
    if (testing_dir == NULL) {
        fprintf(stderr, "Could not open folder `%s`: %s\n",
                TESTING_FOLDER, strerror(errno));
        exit(1);
    }

    struct dirent *dir = NULL;
    while ((dir = readdir(testing_dir)) != NULL) {
        if (!ends_with(dir->d_name, ".json")) continue;
        snprintf(ast_dump_filepath, ARRAY_SIZE(ast_dump_filepath),
                 "%s/%s."DUMP_ARCH_SUFFIX".bin", TESTING_FOLDER, dir->d_name);
        snprintf(json_filepath, ARRAY_SIZE(json_filepath),
                 "%s/%s", TESTING_FOLDER, dir->d_name);

        printf("%s\n", json_filepath);

        Tzozen_Str source = read_file_as_string(json_filepath);
        Json_Result result = parse_json_value(&memory, source);
        if (result.is_error) {
            print_json_error(stderr, result, source, json_filepath);
            exit(1);
        }

        load_memory_from_file(&dump_memory, ast_dump_filepath);
        Json_Value *dump_index = (Json_Value *) dump_memory.buffer;
        json_value_unrelatify(&dump_memory, dump_index);

        if (!json_value_equals(result.value, *dump_index)) {
            fprintf(stderr, "FAILED!\n");
            fprintf(stderr, "Expected: ");
            print_json_value(stderr, *dump_index);
            fputc('\n', stderr);

            fprintf(stderr, "Actual:   ");
            print_json_value(stderr, result.value);
            fputc('\n', stderr);

            exit(1);
        }
    }

    closedir(testing_dir);

    return 0;
}
