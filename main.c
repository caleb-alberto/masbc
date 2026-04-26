#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>


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


int main() {

        /* struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);



        clock_gettime(CLOCK_MONOTONIC, &end);

        long long elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000LL
                               + (end.tv_nsec - start.tv_nsec); */

        // printf("time: %lld \n", elapsed_ns);

        struct clp_node *chain = aligned_alloc(64, 13 * sizeof(struct clp_node));
    
        printf("sizeof(node) = %zu\n", sizeof(struct clp_node));
        printf("buffer addr  = %p\n", (void*)chain);
        printf("addr %% 64   = %lu\n", (unsigned long)chain % 64);
    
        free(chain);

        return 0;
}
