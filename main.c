#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>


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

int main() {

        /* struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);



        clock_gettime(CLOCK_MONOTONIC, &end);

        long long elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000LL
                               + (end.tv_nsec - start.tv_nsec); */

        // printf("time: %lld \n", elapsed_ns);

	int chain_size = 10;
        struct clp_node *chain = aligned_alloc(64, chain_size * sizeof(struct clp_node));
	if (chain == NULL) {
		perror("aligned_alloc");
		exit(1);
	}
    
	shuffle(chain, chain_size);

	assert(verify_perm(chain, chain_size));
	assert(verify_chain(chain, chain_size));
    
        free(chain);

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
