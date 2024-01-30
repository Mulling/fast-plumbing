#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#include "common.h"

[[maybe_unused]]
void write_write() {
    uint8_t* chunk = page_aligned_alloc(ENTRIES);

    memset(chunk, 0x5F, sizeof(uint8_t) * PAGE_SIZE * ENTRIES);

    ssize_t total = PAYLOAD;

    while (total != 0) {
        ssize_t ret = write(STDOUT_FILENO, chunk, PAGE_SIZE * ENTRIES);

        if (ret < 0) {
            die("error: %s", strerror(errno));
        }

        total -= ret;

        assert(total >= 0);
    }

    free(chunk);
}

[[maybe_unused]]
void write_writev() {
    struct iovec iovec = { .iov_base = page_aligned_alloc(ENTRIES),
                           .iov_len = PAGE_SIZE * ENTRIES };

    memset(iovec.iov_base, 0x5F, sizeof(uint8_t) * PAGE_SIZE * ENTRIES);

    ssize_t total = PAYLOAD;

    while (total != 0) {
        ssize_t ret = writev(STDOUT_FILENO, &iovec, 1);

        if (ret < 0) {
            die("error: %s", strerror(errno));
        }

        total -= ret;

        assert(total >= 0);
    }

    free(iovec.iov_base);
}

[[maybe_unused]]
void write_vmsplice() {
    uint8_t* buffer = page_aligned_alloc(ENTRIES);

    memset(buffer, 0x5F, sizeof(uint8_t) * PAGE_SIZE * ENTRIES);

    struct iovec* iovecs = malloc(sizeof(struct iovec) * ENTRIES);

    if (iovecs == NULL) {
        die("error: out of memory (%s)", strerror(errno));
    }

    for (size_t i = 0; i < ENTRIES; i++) {
        iovecs[i].iov_base = buffer + (PAGE_SIZE * i);
        iovecs[i].iov_len = PAGE_SIZE;
    }

    ssize_t total = PAYLOAD;

    while (total != 0) {
        size_t spliced = 0;

        while (spliced < ENTRIES) {
            ssize_t ret =
                vmsplice(STDOUT_FILENO, &iovecs[spliced], ENTRIES - spliced, 0);

            if (ret < 0) {
                die("error: fail to splice to pipe (%s)", strerror(errno));
            }

            spliced += ret / PAGE_SIZE;  // CLEAN: Page size

            total -= ret;

            assert(total >= 0);
        }
    }

    free(iovecs);
    free(buffer);
}

int main(int argc, char const* const* argv) {
    if (isatty(STDOUT_FILENO)) {
        die("error: stdout is a terminal");
    }

    if (argc != 2) {
        die("usage: %s [--write | --writev | --vmsplice]", argv[0]);
    }

    char* desc = NULL;
    void (*fn)(void) = NULL;

    if (strcmp(argv[1], "--write") == 0) {
        desc = "wrote with write,";
        fn = write_write;
    } else if (strcmp(argv[1], "--writev") == 0) {
        desc = "wrote with writev,";
        fn = write_writev;
    } else if (strcmp(argv[1], "--vmsplice") == 0) {
        desc = "spliced with vmsplice,";
        fn = write_vmsplice;
    } else {
        die("error: unknown option %s", argv[1]);
    }

    fcntl(STDOUT_FILENO, F_NOTIFY, DN_ACCESS);

    int ret = fcntl(STDOUT_FILENO, F_SETPIPE_SZ, 1 << 19);

    if (ret < 0) {
        die("error: can't set the size of the pipe %s", strerror(errno));
    }

    fprintf(stderr, "pipe size=%lu\n", pipe_size(STDOUT_FILENO));

    bench(fn, desc);

    return 0;
}
