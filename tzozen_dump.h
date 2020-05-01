#ifndef TZOZEN_DUMP_H_
#define TZOZEN_DUMP_H_

#include <string.h>
#include <errno.h>
#include "tzozen.h"

// Since those AST dumps are literally memory dumps they are very
// platform dependant. So we suffix output file names with the
// corresponding ARCH_SUFFIX to indicated what platform this memory
// dump is compatible with.
#if defined(__linux__) && defined(__x86_64__)
#define DUMP_ARCH_SUFFIX "linux_x86_64"
#else
#error "You are building this on a platform that I don't have! Please add your ARCH_SUFFIX and submit a PR at https://github.com/tsoding/tzozen if you have an opportunity. Thanks!"
#endif

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

void json_array_elem_relatify(Memory *memory, Json_Array_Elem *elem)
{
    while (elem != NULL) {
        void *saved_next = elem->next;
        json_value_relatify(memory, &elem->value);
        RELATIFY_PTR(memory, elem->next);
        elem = saved_next;
    }
}

void json_array_relatify(Memory *memory, Json_Array *array)
{
    json_array_elem_relatify(memory, array->begin);
    RELATIFY_PTR(memory, array->begin);
    RELATIFY_PTR(memory, array->end);
}

void json_object_pair_relatify(Memory *memory, Json_Object_Pair *pair)
{
    while (pair != NULL) {
        void *saved_next = pair->next;
        string_relatify(memory, &pair->key);
        json_value_relatify(memory, &pair->value);
        RELATIFY_PTR(memory, pair->next);
        pair = saved_next;
    }
}

void json_object_relatify(Memory *memory, Json_Object *object)
{
    json_object_pair_relatify(memory, object->begin);
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

// TODO: dump_memory_to_file should probably relatify automatically
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

// TODO: load_memory_from_file should probably unrelatify automatically
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

void json_array_elem_unrelatify(Memory *memory, Json_Array_Elem *elem)
{
    while (elem != NULL) {
        json_value_unrelatify(memory, &elem->value);
        UNRELATIFY_PTR(memory, elem->next);
        elem = elem->next;
    }
}

void json_array_unrelatify(Memory *memory, Json_Array *array)
{
    UNRELATIFY_PTR(memory, array->begin);
    UNRELATIFY_PTR(memory, array->end);
    json_array_elem_unrelatify(memory, array->begin);
}

void json_object_pair_unrelatify(Memory *memory, Json_Object_Pair *pair)
{
    while (pair != NULL) {
        string_unrelatify(memory, &pair->key);
        json_value_unrelatify(memory, &pair->value);

        UNRELATIFY_PTR(memory, pair->next);
        pair = pair->next;
    }
}

void json_object_unrelatify(Memory *memory, Json_Object *object)
{
    UNRELATIFY_PTR(memory, object->begin);
    UNRELATIFY_PTR(memory, object->end);
    json_object_pair_unrelatify(memory, object->begin);
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

String read_file_as_string(const char *filepath)
{
    FILE *f = fopen(filepath, "rb");
    if (f == NULL) goto fail;

    if (fseek(f, 0, SEEK_END) < 0) goto fail;
    long m = ftell(f);
    if (m < 0) goto fail;
    if (fseek(f, 0, SEEK_SET) < 0) goto fail;
    char *buffer = calloc(1, sizeof(char) * (size_t) m + 1);
    if (buffer == NULL) goto fail;

    size_t n = fread(buffer, 1, (size_t) m, f);
    if (ferror(f)) goto fail;
    assert(n == (size_t) m);

    fclose(f);
    return string(n, buffer);
fail:
    fprintf(stderr, "Could not read file `%s`: %s\n",
            filepath, strerror(errno));
    exit(1);
}

#endif  // TZOZEN_DUMP_H_
