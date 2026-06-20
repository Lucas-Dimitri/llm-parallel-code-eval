#include <stdio.h>
#include <stdlib.h>

/* Generates a random graph in the same format as PAMPAR's graph files:
   - N x N adjacency matrix, space-separated integers per row
   - 0  = no edge
   - positive integer = edge weight (range 1..MAX_WEIGHT)
   - ~EDGE_PROB chance of an edge existing between any two nodes
   - no self-loops (diagonal = 0)
   Usage: ./gen_graph <N> <seed> > graph_N.txt */

#define MAX_WEIGHT 100
#define EDGE_PROB  30   /* percent chance of edge between any two nodes */

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <N> <seed> > graph_N.txt\n", argv[0]);
        return 1;
    }
    int N    = atoi(argv[1]);
    int seed = atoi(argv[2]);
    srand(seed);

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            int val = 0;
            if (i != j && (rand() % 100) < EDGE_PROB)
                val = (rand() % MAX_WEIGHT) + 1;
            if (j > 0) putchar(' ');
            printf("%d", val);
        }
        putchar('\n');
    }
    return 0;
}