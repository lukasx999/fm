#ifndef _UTIL_H
#define _UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>



#define ARRAY_LEN(xs) (sizeof(xs) / sizeof *(xs))



static void _impl_panic(
    const char *msg,
    const char *file,
    const char *func,
    int line
) {

#ifdef _GNU_SOURCE
    pthread_t tid = pthread_self();
    char name_buf[16] = { 0 };
    pthread_getname_np(tid, name_buf, ARRAY_LEN(name_buf));

    fprintf(stderr, "Thread `%s` panicked at (%s: %s: %d)\n%s\n",
            name_buf, file, func, line, msg);
#else
    fprintf(stderr, "Panicked at (%s: %s: %d)\n%s\n", file, func, line, msg);
#endif // _GNU_SOURCE

    abort();
}

#define PANIC(msg) \
    ((void) (_impl_panic((msg), __FILE__, __func__, __LINE__), 0))


#define MUST_ZERO(value)                        \
    ((value)                                    \
        ? PANIC("`" #value "` is not 0"), value \
        : value)


#define NON_NULL(ptr)                          \
    ((ptr) == NULL                             \
        ? PANIC("`" #ptr "` is NULL"), ptr     \
        : ptr)


#define UNREACHABLE PANIC("unreachable")
#define TODO(msg) PANIC("not implemented yet: " msg)
#define UNIMPLEMENTED(msg) PANIC("not implemented: " msg)
#define UNUSED __attribute__((unused))
#define DISCARD(value) (void) (value); assert(0)

#define PRINT(x) do {      \
    const char *fmt =      \
        _Generic((x),      \
            char:   "%c",  \
            int:    "%d",  \
            float:  "%f",  \
            size_t: "%lu", \
            char*:  "%s",  \
            void*:  "%p"   \
    );                     \
    printf(fmt, x);        \
    printf("\n");          \
} while (0)



#endif // _UTIL_H
