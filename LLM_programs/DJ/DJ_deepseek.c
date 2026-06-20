#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <omp.h>

#define infinity 99999999

int vertices, *G;

void dijkstra(void) {
    #pragma omp parallel for shared(G, vertices)
    for (int source = 0; source < vertices; source++) {
        int i, v, t;
        int *dis = malloc(sizeof(int) * vertices);
        int *vis = malloc(sizeof(int) * vertices);

        for (i = 0; i < vertices; i++) {
            dis[i] = infinity;
            vis[i] = 0;
        }
        dis[source] = 0;

        for (t = 0; t < vertices; t++) {
            v = -1;
            for (i = 0; i < vertices; i++) {
                if (!vis[i] && (v < 0 || dis[i] < dis[v])) {
                    v = i;
                }
            }
            vis[v] = 1;
            for (i = 0; i < vertices; i++) {
                if (G[(v * vertices) + i] && dis[i] > dis[v] + G[(v * vertices) + i]) {
                    dis[i] = dis[v] + G[(v * vertices) + i];
                }
            }
        }

        free(dis);
        free(vis);
    }
}

void preencheGrafo(FILE *input) {
    int i, j;
    for (i = 0; i < vertices; i++) {
        for (j = 0; j < vertices; j++) {
            fscanf(input, "%d", &G[(i * vertices) + j]);
            if (G[(i * vertices) + j] == -1) {
                G[(i * vertices) + j] = infinity;
            }
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("ERROR! Usage: my_program <input-size> <input-file>\n\n \tE.g. -> ./my_program 2048 2048.txt\n\n");
        exit(1);
    }

#ifdef ELAPSEDTIME
    struct timeval start, end;
    gettimeofday(&start, NULL);
#endif

    FILE *input;
    vertices = atoi(argv[1]);
    input = fopen(argv[2], "r");

    G = malloc(sizeof(int) * vertices * vertices);

    preencheGrafo(input);
    fclose(input);

    dijkstra();

    free(G);

#ifdef ELAPSEDTIME
    gettimeofday(&end, NULL);
    double delta = ((end.tv_sec - start.tv_sec) * 1000000u + end.tv_usec - start.tv_usec) / 1.e6;
    printf("Execution time\t%f\n", delta);
#endif

    return 0;
}