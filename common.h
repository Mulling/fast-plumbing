#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// L2 cache of my machine
#define PAGE_SIZE 4096
#define ENTRIES 256

#define PAYLOAD (1ul << 36)

void die(char const* fmt, ...);

static inline uint8_t __nonnull(()) * page_aligned_alloc(size_t pages) {
    uint8_t* buffer =
        aligned_alloc(PAGE_SIZE, sizeof(uint8_t) * PAGE_SIZE * pages);

    if (buffer == NULL) {
        die("error: out of memory (%s)", strerror(errno));
    }

    return buffer;
}

void die(char const* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char msg[256];

    int wrote __attribute((unused)) = vsnprintf(msg, 256, fmt, args);

    va_end(args);

    fprintf(stderr, "%s\n", msg);
    exit(1);
}

size_t pipe_size(int fd) {
    int ret = fcntl(fd, F_GETPIPE_SZ);

    if (ret < 0) {
        die("error: can't read the size of the pipe %s", strerror(errno));
    }

    return ret;
}

void bench(void(fn)(void), char const* what) {
    clock_t tstp = clock();

    fn();

    tstp = clock() - tstp;

    double const payload = (double)(PAYLOAD >> 10 >> 10 >> 10);

    double const time = (double)tstp / CLOCKS_PER_SEC;

    fprintf(stderr, "%s %.1f GiB in %f seconds (%.1f GiB/s)\n", what, payload,
            time, payload / time);
}
