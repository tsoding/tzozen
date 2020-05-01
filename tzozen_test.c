#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>

#define JSON_ARRAY_PAGE_CAPACITY 1
#define JSON_OBJECT_PAGE_CAPACITY 1
#include "./tzozen_dump.h"

#define ARRAY_SIZE(xs) (sizeof(xs) / sizeof((xs)[0]))
#define TESTING_FOLDER "./tests/"

uint8_t memory_buffer[100 * MEGA];
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
    Json_Object_Pair *pair_a = a.begin;
    Json_Object_Pair *pair_b = b.begin;

    while (pair_a != NULL && pair_b != NULL) {
        if (!string_equal(pair_a->key, pair_b->key)) {
            return 0;
        }

        if (!json_value_equals(pair_a->value, pair_b->value)) {
            return 0;
        }

        pair_a = pair_a->next;
        pair_b = pair_b->next;
    }

    return pair_a == NULL && pair_b == NULL;
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

        String source = read_file_as_string(json_filepath);
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

int main_(void)
{
    String tests[] = {
        SLT("null"),
        SLT("nullptr"),
        SLT("true"),
        SLT("false"),
        SLT("1"),
        SLT("2"),
        SLT(".10"),
        SLT("1e9999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999"),
        SLT("-.10"),
        SLT("-10"),
        SLT("10.10"),
        SLT("-10.10e2"),
        SLT("-10.10e-2"),
        // TODO(#25): parse_json_number treats -10.-10e-2 as two separate numbers
        SLT("-10.-10e-2"),
        SLT("\"hello,\tworld\""),
        SLT("[]"),
        SLT("[1]"),
        SLT("[\"test\"]"),
        SLT("[1,2,3]"),
        SLT("[1,2,3 5]"),
        SLT("[\"hello,\tworld\", 123, \t \"abcd\", -10.10e-2, \"test\"]"),
        SLT("[[]]"),
        SLT("[123,[321,\"test\"],\"abcd\"]"),
        SLT("[\n"
            "  true,\n"
            "  false,\n"
            "  null\n"
            "]"),
        SLT("[\n"
            "  true,\n"
            "  false #\n"
            "  null\n"
            "]"),
        SLT("{\n"
            "   \"null\": null,\n"
            "   \"boolean\": true,\n"
            "   \"boolean\": false,\n"
            "   \"number\": 69420,\n"
            "   \"string\": \"hello\",\n"
            "   \"array\": [null, true, false, 69420, \"hello\"],\n"
            "   \"object\": {}\n"
            "}"),
        SLT("{\n"
            "   \"null\": null,\n"
            "   \"boolean\": true\n"
            "   \"boolean\": false,\n"
            "   \"number\": 69420,\n"
            "   \"string\": \"hello\",\n"
            "   \"array\": [null, true, false, 69420, \"hello\"],\n"
            "   \"object\": {}\n"
            "}"),
        SLT("[0e+1]"),
        SLT("[0e+-1]"),
        SLT("[0C]"),
        SLT("\"\\uD834\\uDD1E\\uD834\\uDD1E\\uD834\\uDD1E\\uD834\\uDD1E\""),
        SLT("\"\\uD834\\uDD1E\\uD834\\uDD1E\\uD834\\uDD1E\\uD834\""),
        SLT("[\"a\0a\"]"),
        SLT("[\"\\\\a\"]"),
        SLT("[\"\\\"\"]"),
        SLT("[\"new\aline\"]"),
        SLT("{\"test\": [0, true, 1], \"foo\": [{\"param\": \"data:text/html],https://1:a.it@www.it\\\\\"}, -889578990, false]}")
    };
    size_t tests_count = sizeof(tests) / sizeof(tests[0]);

    for (size_t i = 0; i < tests_count; ++i) {
        fputs("PARSING: \n", stdout);
        fwrite(tests[i].data, 1, tests[i].len, stdout);
        fputc('\n', stdout);

        Json_Result result = parse_json_value(&memory, tests[i]);
        if (result.is_error) {
            fputs("FAILURE: \n", stdout);
            print_json_error(stdout, result, tests[i], "<test>");
        } else if (trim_begin(result.rest).len != 0) {
            fputs("FAILURE: \n", stdout);
            fputs("parsed ", stdout);
            print_json_value(stdout, result.value);
            fputc('\n', stdout);

            fputs("but left unparsed input: ", stdout);
            fwrite(result.rest.data, 1, result.rest.len, stdout);
            fputc('\n', stdout);
        } else {
            fputs("SUCCESS: \n", stdout);
            print_json_value(stdout, result.value);
            fputc('\n', stdout);
        }

        printf("MEMORY USAGE: %lu bytes\n", memory.size);
        fputs("------------------------------\n", stdout);
        memory_clean(&memory);
    }

    free(memory.buffer);

    return 0;
}
