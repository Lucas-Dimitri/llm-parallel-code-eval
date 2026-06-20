#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <omp.h>
#define infinity 99999999

int vertices, *G;

void dijkstra(int source){
#pragma omp parallel for schedule(dynamic)
for(int src=0; src<vertices; src++){
int i, v, t;
int *dis, vis; dis = malloc(sizeof(int)vertices); vis = malloc(sizeof(int)vertices); for(i=0;i<vertices;i++){ dis = infinity; vis = 0; } dis = 0; for(t=0;t<vertices;t++){ v = -1; for(i=0;i<vertices;i++){ if(!vis && (v<0 || dis < dis)){ v = i; } } vis = 1; for(i=0;i<vertices;i++){ if(G[(vvertices)+i] && dis > dis + G[(vvertices)+i]){ dis = dis + G[(vvertices)+i];
}
}
}
free(dis);
free(vis);
}
}[i][src][v]

void preencheGrafo(FILE input){ int i, j; for(i=0;i<vertices;i++){ for(j=0;j<vertices;j++){ fscanf(input, "%d", &G[(ivertices)+j]);
if(G[(ivertices)+j] == -1){ G[(ivertices)+j] = infinity;
}
}
}
}

int main(int argc, char **argv){

    if (argc < 3) {
            printf ("ERROR! Usage: my_program <input-size> <input-file>\n\n \tE.g. ->./my_program 2048 2048.txt\n\n");
            exit(1);
    }

    #ifdef ELAPSEDTIME
            struct timeval start, end;
            gettimeofday(&start, NULL);
    #endif
    int arestas, source;
    FILE *input;

    vertices = atoi(argv);
    input = fopen(argv, "r");[1][2]

    G = malloc(sizeof(int)*vertices*vertices);

    preencheGrafo(input);

    dijkstra(source);

    #ifdef ELAPSEDTIME
            gettimeofday(&end, NULL);
            double delta = ((end.tv_sec - start.tv_sec) * 1000000u + end.tv_usec - start.tv_usec) / 1.e6;
            printf("Execution time\t%f\n", delta);
    #endif

    return 0;

}