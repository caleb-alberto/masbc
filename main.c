#define _GNU_SOURCE
#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>


/*
 *  Cache-line-padded node for the pointer-chasing benchmark.
 *  Padded to 64 bytes so each node occupies exactly one cache line;
 *  this guarantees every hop incurs a fresh cache-line load rather
 *  than letting multiple nodes share a line.
 *
 *  `next` is an *index* into the chain buffer, not a pointer.
 *  Indices keep the chain position-independent, which matters for
 *  the SSD tier where the buffer is mmap'd and the base address
 *  can change between runs.
 */

struct clp_node {
        uint32_t next;
        uint8_t padding[60];
};


bool verify_chain(struct clp_node* chain, int size);
bool verify_perm(struct clp_node* chain, int size);
void shuffle(struct clp_node* chain, int size);
void walk_chain(int chain_size, char* tier);

int main() {
        walk_chain(512, "L1");
        walk_chain(16384, "L2");
        walk_chain(262144, "L3");
        walk_chain(1048576, "RAM");

        int fd = open("tmp_bench.bin", O_CREAT | O_WRONLY, 0644);

        size_t count = 4096 * 262144;
        char* wbuf = (char*)calloc(count, 1);
        assert(write(fd, wbuf, count) == count);
        assert(close(fd) == 0);

        fd = open("tmp_bench.bin", O_RDONLY | O_DIRECT);

	int HOPS = 1000;
	long long times[HOPS];
	char rbuf[4096];
        struct timespec start, end;

	for (int i = 0; i < HOPS; i++) {
		off_t offset = 4096 * (rand() % 262144);
		assert(lseek(fd, offset, SEEK_SET) != -1);

		clock_gettime(CLOCK_MONOTONIC, &start);

		assert(read(fd, rbuf, 4096) != -1);

		clock_gettime(CLOCK_MONOTONIC, &end);
		times[i] = (end.tv_sec - start.tv_sec) * 1000000000LL
			+ (end.tv_nsec - start.tv_nsec);
	}

	long long total_time = 0;
	for (int i = 0; i < HOPS; i++)
		total_time += times[i];
	long long avg_time = total_time / HOPS;
	printf("DISK\ttime: %5lld ns/hop\tbuffer size: %3.0f GB\n",
		avg_time,
		(double)count / 1073741824);

        assert(close(fd) == 0);
        assert(unlink("tmp_bench.bin") == 0);

        return 0;
}

bool verify_chain(struct clp_node* chain, int size) {
        bool visited[size];
        uint32_t current = 0;

        for (int i = 0; i < size; i++) {
                visited[current] = true;
                current = chain[current].next;
        }
        for (int i = 0; i < size; i++) {
                if (!visited[i])
                        return false;
        }

        return true;
}

bool verify_perm(struct clp_node* chain, int size) {
        bool visited[size];

        for (int i = 0; i < size; i++)
                visited[chain[i].next] = true;

        for (int i = 0; i < size; i++) {
                if (!visited[i])
                        return false;
        }

        return true;
}

void shuffle(struct clp_node* chain, int size) {
        int j = 0;
        int perm[size];

        for (int i = 0; i < size; i++)
                perm[i] = i;

        for (int i = size - 1; i > 0; i--) {
                j = rand() % (i + 1);
                int dum = perm[i];
                perm[i] = perm[j];
                perm[j] = dum;
        }

        for (int i = 0; i < size; i++)
                chain[perm[i]].next = perm[(i+1) % size];
}

void walk_chain(int chain_size, char* tier) {
        struct clp_node *chain = aligned_alloc(64, chain_size * sizeof(struct clp_node));
        if (chain == NULL) {
                perror("aligned_alloc");
                exit(1);
        }

        shuffle(chain, chain_size);

        assert(verify_perm(chain, chain_size));
        assert(verify_chain(chain, chain_size));

        struct timespec start, end;
        uint32_t current = 0;
        uint64_t accumulator = 0;
        uint64_t HOPS = 1000;

        clock_gettime(CLOCK_MONOTONIC, &start);

        for (int i = 0; i < HOPS; i++) {
                current = chain[current].next;
                accumulator ^= current;
        }

        clock_gettime(CLOCK_MONOTONIC, &end);

        long long elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000LL
                + (end.tv_nsec - start.tv_nsec);

        asm volatile ("" : : "r"(accumulator)); // prevent loop elimination by using 'accumulator'

        float buffer_size = (chain_size * sizeof(struct clp_node)) / (1024.0 * 1024.0);

        if (buffer_size < 1) {
                printf("%s\ttime: %5lld ns/hop\tbuffer size: %3.0f KB\n",
                       tier,
                       elapsed_ns/HOPS,
                       buffer_size * 1024.0);
        }
        else {
                printf("%s\ttime: %5lld ns/hop\tbuffer size: %3.0f MB\n",
                       tier,
                       elapsed_ns/HOPS,
                       buffer_size);
        }

        free(chain);
}
