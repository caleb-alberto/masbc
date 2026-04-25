#include <time.h>
#include <stdio.h>

int main() {
        
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);
        // ... work ...
        int x;
        int y;

        x=1;
        y=2;



        clock_gettime(CLOCK_MONOTONIC, &end);

        printf("%d\n\n", x+y);
        long long elapsed_ns = (end.tv_sec - start.tv_sec) * 1000000000LL
                               + (end.tv_nsec - start.tv_nsec);

        printf("time: %lld \n", elapsed_ns);
        
        return 0;
}
