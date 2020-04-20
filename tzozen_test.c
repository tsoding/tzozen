#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

// TODO: different capacities produce incompatible memory dumps
#define JSON_ARRAY_PAGE_CAPACITY 1
#define JSON_OBJECT_PAGE_CAPACITY 1
#include "tzozen.h"

#define MEMORY_CAPACITY (640 * 1000)
static uint8_t memory_buffer[MEMORY_CAPACITY];
static Memory memory = {
    .capacity = MEMORY_CAPACITY,
    .buffer = memory_buffer
};

// NOTE: We represent relative NULL with `-1` cast to (void*). Which on
// x86_64 looks like 0xFFFFFFFFFFFFFFFF in memory.
//
// - 0xFFFFFFFFFFFFFFFF is the entire addressing capability of x86_64.
// - If (memory)->size == 0xFFFFFFFFFFFFFFFF than `base` must be equal `a`
//   (otherwise we won't be able to address that much memory on x86_64)
//   and implementation of `RELATIFY_PTR` works out correctly.
// - `~0xFFFFFFFFFFFFFFFF == 0x0` makes it super easy to check with the
//   relatified value is a NULL or not (as implemented in `UNRELATIFY_PTR`).
#define RELATIFY_PTR(memory, ptr)                                       \
    do {                                                                \
        const char *base = (const char *) (memory)->buffer;             \
        const char *a = (const char *) (ptr);                           \
        if (base <= a) {                                                \
            (ptr) = (void *) (a - base);                                \
        } else {                                                        \
            assert(a == NULL);                                          \
            (ptr) = (void*) (-1);                                       \
        }                                                               \
    } while(0)

#define UNRELATIFY_PTR(memory, ptr)                                     \
    do {                                                                \
        const char *base = (const char *) (memory)->buffer;             \
        size_t offset = (size_t) (ptr);                                 \
        if (~offset) {                                                  \
            assert(offset <= (memory)->size);                           \
            (ptr) = (void *) (base + offset);                           \
        } else {                                                        \
            (ptr) = NULL;                                               \
        }                                                               \
    } while(0)

void json_value_relatify(Memory *memory, Json_Value *value);
void json_value_unrelatify(Memory *memory, Json_Value *index);

void string_relatify(Memory *memory, String *string)
{
    assert((const char *) memory->buffer <= string->data);
    RELATIFY_PTR(memory, string->data);
}

void json_number_relatify(Memory *memory, Json_Number *number)
{
    string_relatify(memory, &number->integer);
    string_relatify(memory, &number->fraction);
    string_relatify(memory, &number->exponent);
}

void json_array_page_relatify(Memory *memory, Json_Array_Page *page)
{
    if (page == NULL) return;

    for (size_t i = 0; i < page->size; ++i) {
        json_value_relatify(memory, page->elements + i);
    }

    json_array_page_relatify(memory, page->next);
    RELATIFY_PTR(memory, page->next);
}

void json_array_relatify(Memory *memory, Json_Array *array)
{
    json_array_page_relatify(memory, array->begin);
    RELATIFY_PTR(memory, array->begin);
    RELATIFY_PTR(memory, array->end);
}

void json_object_member_relatify(Memory *memory,
                                 Json_Object_Member *member)
{
    string_relatify(memory, &member->key);
    json_value_relatify(memory, &member->value);
}

void json_object_page_relatify(Memory *memory, Json_Object_Page *page)
{
    if (page == NULL) return;

    for (size_t i = 0; i < page->size; ++i) {
        json_object_member_relatify(memory, page->elements + i);
    }

    json_object_page_relatify(memory, page->next);
    RELATIFY_PTR(memory, page->next);
}

void json_object_relatify(Memory *memory, Json_Object *object)
{
    json_object_page_relatify(memory, object->begin);
    RELATIFY_PTR(memory, object->begin);
    RELATIFY_PTR(memory, object->end);
}

void json_value_relatify(Memory *memory, Json_Value *value)
{
    switch (value->type) {
    case JSON_NULL:
    case JSON_BOOLEAN:
        break;
    case JSON_NUMBER:
        json_number_relatify(memory, &value->number);
        break;
    case JSON_STRING:
        string_relatify(memory, &value->string);
        break;
    case JSON_ARRAY:
        json_array_relatify(memory, &value->array);
        break;
    case JSON_OBJECT:
        json_object_relatify(memory, &value->object);
        break;
    }
}

void dump_memory_to_file(Memory *memory, const char *file_path)
{
    FILE *file = fopen(file_path, "wb");
    if (!file) {
        fprintf(stderr, "Could not open file `%s`: %s\n", file_path, strerror(errno));
        exit(1);
    }

    size_t n = fwrite(memory->buffer, 1, memory->size, file);
    if (n != memory->size) {
        fprintf(stderr, "Could not write data to file `%s`: %s\n",
                file_path,
                strerror(errno));
        exit(1);
    }

    fclose(file);
}

void load_memory_from_file(Memory *memory, const char *file_path)
{
    FILE *file = fopen(file_path, "rb");
    if (!file) goto fail;
    if (fseek(file, 0, SEEK_END) < 0) goto fail;
    long m = ftell(file);
    if (m < 0) goto fail;
    if (fseek(file, 0, SEEK_SET) < 0) goto fail;

    if ((long) memory->capacity < m) {
        fprintf(stderr,
                "The file does not fit into the memory. "
                "Size of memory: %ld. Size of file: %ld.\n",
                memory->size, m);
        exit(1);
    }

    memory->size = fread(memory->buffer, 1, m, file);
    if (ferror(file)) goto fail;
    assert((long) memory->size == m);

    fclose(file);

    return;
fail:
    fprintf(stderr, "Could not load memory from file `%s`: %s\n", file_path, strerror(errno));
    exit(1);
}

void string_unrelatify(Memory *memory, String *string)
{
    UNRELATIFY_PTR(memory, string->data);
}

void json_number_unrelatify(Memory *memory, Json_Number *number)
{
    string_unrelatify(memory, &number->integer);
    string_unrelatify(memory, &number->fraction);
    string_unrelatify(memory, &number->exponent);
}

void json_array_page_unrelatify(Memory *memory, Json_Array_Page *page)
{
    while (page != NULL) {
        for (size_t i = 0; i < page->size; ++i) {
            json_value_unrelatify(memory, page->elements + i);
        }

        UNRELATIFY_PTR(memory, page->next);
        page = page->next;
    }
}

void json_array_unrelatify(Memory *memory, Json_Array *array)
{
    UNRELATIFY_PTR(memory, array->begin);
    UNRELATIFY_PTR(memory, array->end);
    json_array_page_unrelatify(memory, array->begin);
}

void json_object_member_unrelatify(Memory *memory, Json_Object_Member *member)
{
    string_unrelatify(memory, &member->key);
    json_value_unrelatify(memory, &member->value);
}

void json_object_page_unrelatify(Memory *memory, Json_Object_Page *page)
{
    while (page != NULL) {
        for (size_t i = 0; i < page->size; ++i) {
            json_object_member_unrelatify(memory, page->elements + i);
        }

        UNRELATIFY_PTR(memory, page->next);
        page = page->next;
   }
}

void json_object_unrelatify(Memory *memory, Json_Object *object)
{
    UNRELATIFY_PTR(memory, object->begin);
    UNRELATIFY_PTR(memory, object->end);
    json_object_page_unrelatify(memory, object->begin);
}

void json_value_unrelatify(Memory *memory, Json_Value *index)
{
    switch (index->type) {
    case JSON_NULL:
    case JSON_BOOLEAN:
        break;
    case JSON_NUMBER:
        json_number_unrelatify(memory, &index->number);
        break;
    case JSON_STRING:
        string_unrelatify(memory, &index->string);
        break;
    case JSON_ARRAY:
        json_array_unrelatify(memory, &index->array);
        break;
    case JSON_OBJECT:
        json_object_unrelatify(memory, &index->object);
        break;
    }
}

#define MEMORY_DUMP_FILE_PATH "memory.bin"

int main()
{
    //// Parsing //////////////////////////////

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

    printf("Parsing expression:\n\t%.*s\n", (int) input.len, input.data);

    Json_Value *index = memory_alloc(&memory, sizeof(struct Json_Value));
    Json_Result result = parse_json_value(&memory, input);
    if (result.is_error) {
        fputs("FAILURE:\n\t", stdout);
        print_json_error(stdout, result, input, "<example>");
        exit(1);
    }
    *index = result.value;

    fputs("SUCCESS:\n\t", stdout);
    print_json_value(stdout, result.value);
    fputc('\n', stdout);

    //// Dumping to file //////////////////////

    printf("MEMORY USAGE:\n\t%lu bytes\n", memory.size);

    printf("Relatifying json value...\n");
    json_value_relatify(&memory, index);

    printf("Dumping memory to %s...\n", MEMORY_DUMP_FILE_PATH);
    dump_memory_to_file(&memory, MEMORY_DUMP_FILE_PATH);

    RELATIFY_PTR(&memory, index);

    printf("Index starts at %p\n", (void*)index);

    //// Loading from file ////////////////////

    printf("Cleaning up the memory...\n");
    memory.size = 0;
    memset(memory.buffer, 0, memory.capacity);

    printf("Loding memory back from %s...\n", MEMORY_DUMP_FILE_PATH);
    load_memory_from_file(&memory, MEMORY_DUMP_FILE_PATH);

    // NOTE: the index is always located at relative pointer 0x0

    printf("Unrelatifying the json value...\n");
    json_value_unrelatify(&memory, (Json_Value *) memory.buffer);

    printf("Restored JSON:\n\t");
    print_json_value(stdout, *((Json_Value *) memory.buffer));
    return 0;
}

int main_(void)
{
    Memory memory = {
        .capacity = MEMORY_CAPACITY,
        .buffer = malloc(MEMORY_CAPACITY)
    };

    assert(memory.buffer);

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
