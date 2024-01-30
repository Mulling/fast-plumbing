#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#include "common.h"

[[maybe_unused]]
void read_read() {
    uint8_t* buffer = page_aligned_alloc(ENTRIES);

    ssize_t total = PAYLOAD;

    while (total != 0) {
        ssize_t ret = read(STDIN_FILENO, buffer, PAGE_SIZE * ENTRIES);

        if (ret < 0) {
            die("error: fail to read (%s)", strerror(errno));
        }

        total -= ret;

        assert(total >= 0);
    }

    free(buffer);
}

[[maybe_unused]]
void read_readv() {
    struct iovec iovec = { .iov_base = page_aligned_alloc(ENTRIES),
                           .iov_len = PAGE_SIZE * ENTRIES };

    ssize_t total = PAYLOAD;

    while (total != 0) {
        int ret = readv(STDIN_FILENO, &iovec, PAGE_SIZE * ENTRIES);

        if (ret < 0) {
            die("error: %s", strerror(errno));
        }

        total -= ret;

        assert(total >= 0);
    }

    free(iovec.iov_base);
}

[[maybe_unused]]
void read_splice() {
    char* sink_file = "/dev/null";

    int sink = open(sink_file, O_WRONLY);

    if (sink < 0) {
        die("error: fail to open %s (%s)", sink_file, strerror(errno));
    }

    uint8_t* buffer = page_aligned_alloc(ENTRIES);

    ssize_t total = PAYLOAD;

    while (total != 0) {
        ssize_t ret =
            splice(STDIN_FILENO, NULL, sink, NULL, PAGE_SIZE * ENTRIES, 0);

        if (ret < 0) {
            die("error: %s", strerror(errno));
        }

        total -= ret;

        assert(total >= 0);
    }

    close(sink);
    free(buffer);
}

int main(int argc, char const* const* argv) {
    if (isatty(STDIN_FILENO)) {
        die("error: stdin is a terminal");
    }

    if (argc != 2) {
        die("usage: %s [--read | --readv | --splice]", argv[0]);
    }

    char* desc = NULL;
    void (*fn)(void) = NULL;

    if (strcmp(argv[1], "--read") == 0) {
        desc = "read with read,";
        fn = read_read;
    } else if (strcmp(argv[1], "--readv") == 0) {
        desc = "read with readv,";
        fn = read_readv;
    } else if (strcmp(argv[1], "--splice") == 0) {
        desc = "spliced with splice,";
        fn = read_splice;
    } else {
        die("error: unknown option %s", argv[1]);
    }

    bench(fn, desc);

    return 0;
}
