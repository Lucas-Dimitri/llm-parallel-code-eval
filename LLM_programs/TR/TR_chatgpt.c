#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <omp.h>

struct ring{
    int X; // população de predadores
    int Y; // população de presas
};
typedef struct ring TipoRing;

void inicializaMatriz(TipoRing *jogo, int N, int predadores, int presas, TipoRing *nova){
    int i, j;

    #pragma omp parallel for private(j) collapse(2)
    for(i = 0; i < N; i++){
        for(j = 0; j < N; j++){
            jogo[(i * N) + j].X = predadores;
            jogo[(i * N) + j].Y = presas;
            nova[(i * N) + j].X = 0;
            nova[(i * N) + j].Y = 0;
        }
    }
}

void zeraAux(TipoRing *nova, int N){
    int i, j;

    #pragma omp parallel for private(j) collapse(2)
    for(i = 0; i < N; i++){
        for(j = 0; j < N; j++){
            nova[(i * N) + j].X = 0;
            nova[(i * N) + j].Y = 0;
        }
    }
}

void imprimeMatriz(TipoRing *jogo, int N){
    int i, j;
    printf("\n");
    for(i = 0; i < N; i++){
        for(j = 0; j < N; j++){
            printf("%d | %d\t\t", jogo[(i * N) + j].X, jogo[(i * N) + j].Y);
        }
        printf("\n");
    }
}

void joga(TipoRing *jogo, int i, int j, int N, TipoRing *nova){
    int Xmenor = 0, Xmaior = 0, Ymenor = 0, Ymaior = 0, X, Y;
    float rx = 0.0798f; // Taxa de morte Predadores por idade
    float ax = 0.0123f; // Taxa de morte Predadores por superpopulação
    float bx = 0.0377f; // Taxa de nascimento dos predadores
    float u  = 0.12f;   // Taxa de migração dos predadores
    float ry = 0.0178f; // Taxa de morte das presas
    float ay = 0.0998f; // Taxa de presas mortas pelos predadores
    float by = 0.0100f; // Taxa de nascimento das presas
    float v  = 0.13f;   // Taxa de migração das presas

    float popX1, popY1, popX2, popY2;
    float fxy, gxy;

    X = jogo[(i * N) + j].X;
    Y = jogo[(i * N) + j].Y;

    fxy = X * (rx + (ax * X) + (bx * Y));
    gxy = Y * (ry + (ay * X) + (by * Y));

    if ((i - 1) < 0) {
        Xmenor = jogo[((N - 1) * N) + j].X;
    } else {
        Xmenor = jogo[((i - 1) * N) + j].X;
    }

    if ((i + 1) > (N - 1)) {
        Xmaior = jogo[j].X;
    } else {
        Xmaior = jogo[((i + 1) * N) + j].X;
    }

    popX1 = fxy + u * (Xmenor - (2 * X) + Xmaior);
    popY1 = gxy + v * (Ymenor - (2 * Y) + Ymaior);

    if ((j - 1) < 0) {
        Ymenor = jogo[(i * N) + N - 1].Y;
    } else {
        Ymenor = jogo[(i * N) + j - 1].Y;
    }

    if ((j + 1) > (N - 1)) {
        Ymaior = jogo[i * N].Y;
    } else {
        Ymaior = jogo[(i * N) + j + 1].Y;
    }

    popX2 = fxy + u * (Xmenor - (2 * X) + Xmaior);
    popY2 = gxy + v * (Ymenor - (2 * Y) + Ymaior);

    float Xfinal = (popX1 + popX2) / 2.0f;
    float Yfinal = (popY1 + popY2) / 2.0f;

    nova[(i * N) + j].X = (int)Xfinal;
    nova[(i * N) + j].Y = (int)Yfinal;
}

int main(int argc, char **argv){

    if (argc < 2) {
        printf("ERROR! Usage: my_program <input-size>\n\n \tE.g. -> ./my_program 2048\n\n");
        exit(1);
    }

#ifdef ELAPSEDTIME
    struct timeval start, end;
    gettimeofday(&start, NULL);
#endif

    int x = 0;
    int linha, coluna;
    int N = atoi(argv[1]);
    int presas = 100;
    int predadores = 100;

    TipoRing *jogo = (TipoRing *)malloc(sizeof(TipoRing) * N * N);
    TipoRing *auxiliar = (TipoRing *)malloc(sizeof(TipoRing) * N * N);

    inicializaMatriz(jogo, N, predadores, presas, auxiliar);
    zeraAux(auxiliar, N);

    for(x = 0; x < ((N * 2) - 1); x++){

        #pragma omp parallel for collapse(2) private(linha, coluna)
        for(linha = 0; linha < N; linha++){
            for(coluna = 0; coluna < N; coluna++){
                if(linha + coluna == x){
                    joga(jogo, linha, coluna, N, auxiliar);
                }
            }
        }

        #pragma omp parallel for collapse(2) private(linha, coluna)
        for(linha = 0; linha < N; linha++){
            for(coluna = 0; coluna < N; coluna++){
                if(linha + coluna == x){
                    jogo[(linha * N) + coluna].X = auxiliar[(linha * N) + coluna].X;
                    jogo[(linha * N) + coluna].Y = auxiliar[(linha * N) + coluna].Y;
                }
            }
        }
    }

    printf("%d %d\n", jogo[0].X, jogo[0].Y);

#ifdef ELAPSEDTIME
    gettimeofday(&end, NULL);
    double delta = ((end.tv_sec - start.tv_sec) * 1000000u +
                    end.tv_usec - start.tv_usec) / 1.e6;
    printf("Execution time\t%f\n", delta);
#endif

    free(jogo);
    free(auxiliar);

    return 0;
}