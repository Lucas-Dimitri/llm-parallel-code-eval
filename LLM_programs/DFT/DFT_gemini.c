// DFT_parallel.c
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>
#include <omp.h>

#define PI2 6.2832
 
int main(int argc, char **argv){

	if (argc < 2) {
		printf ("ERROR! Usage: my_program <input-size>\n\n \tE.g. -> ./my_program 32768\n\n");
		exit(1);
	}

	#ifdef ELAPSEDTIME
		struct timeval start, end;
		gettimeofday(&start, NULL);
	#endif

    // time and frequency domain data arrays
    int n, k;             // indices for time and frequency domains
    int N = atoi(argv[1]);
    float *x = malloc(sizeof(float)*N);           // discrete-time signal, x
    float *Xre = malloc(sizeof(float)*N); 
    float *Xim = malloc(sizeof(float)*N); 		// DFT of x (real and imaginary parts)
    float *P = malloc(sizeof(float)*N);           // power spectrum of x
     
    srand(N);
    for (n=0 ; n<N ; ++n){
    	x[n] = ((2.0 * rand()) / RAND_MAX) - 1.0;
    	Xre[n] = 0;
    	Xim[n] = 0;
    }

    #pragma omp parallel for private(k, n) shared(x, Xre, Xim, P, N)
    for (k=0 ; k<N ; ++k){
        float re = 0.0f;
        float im = 0.0f;
        // Real part of X[k]
	    for (n=0 ; n<N ; ++n) 
            re += x[n] * cos(n * k * PI2 / N);
        // Imaginary part of X[k]
	    for (n=0 ; n<N ; ++n) 
            im -= x[n] * sin(n * k * PI2 / N);
        
        Xre[k] = re;
        Xim[k] = im;
        // Power at kth frequency bin
        P[k] = re*re + im*im;
    }

    printf("%f\n", P[1]);
	#ifdef ELAPSEDTIME
		gettimeofday(&end, NULL);
		double delta = ((end.tv_sec  - start.tv_sec) * 1000000u + end.tv_usec - start.tv_usec) / 1.e6;
		printf("Execution time\t%f\n", delta);
	#endif

    free(x);
    free(Xre);
    free(Xim);
    free(P);

    return 0;
}