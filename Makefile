FLAGS := -std=c2x -Wall -Wextra -Werror -Wpedantic -fsanitize=address,undefined -g ${CFLAGS}

all: read write

write.o: write.c
	$(CC) ${FLAGS} $^ -o $@ -c

write: write.o
	$(CC) ${FLAGS} $^ -o $@

read.o: read.c
	$(CC) ${FLAGS} $^ -o $@ -c

read: read.o
	$(CC) ${FLAGS} $^ -o $@

test: all
	./write --vmsplice | ./read --splice

bench: FLAGS = -std=c2x -Wall -Wextra -Werror -Wpedantic -O3 -mtune=native -march=native ${CFLAGS}
bench: all
	./write | ./read
	# perf record -g sh -c './write | ./read'

clean:
	$(RM) 'read' write *.o perf.data.old perf.data compile_commands.json

.PHONY: clean test bench
