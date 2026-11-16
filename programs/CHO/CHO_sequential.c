/* Cholesky decomposition.
 * From PolyBench/C 4.2.1 (linear-algebra/solvers/cholesky), stamped May 10, 2016,
 * http://polybench.sourceforge.net. Copyright (c) 2010-2016, Ohio State
 * University; distributed under the license in programs/LICENSE-PolyBench.txt.
 * Made self-contained:
 * same kernel, double precision, matrix size from the command line.
 * Only the kernel is timed, as in PolyBench. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>

static void init_array(int n, double A[n][n])
{
	int i, j, r, s, t;

	for (i = 0; i < n; i++) {
		for (j = 0; j <= i; j++)
			A[i][j] = (double)(-j % n) / n + 1;
		for (j = i+1; j < n; j++)
			A[i][j] = 0;
		A[i][i] = 1;
	}

	/* Make the matrix positive semi-definite: A = L * L^T, with L the lower
	   triangle above. Products with t > min(r,s) are zero and the result is
	   symmetric, so only the others are summed: same values as PolyBench,
	   which sums all n*n*n products. */
	double (*B)[n] = malloc(sizeof(double) * n * n);
	for (r = 0; r < n; r++)
		for (s = 0; s <= r; s++) {
			double sum = 0;
			for (t = 0; t <= s; t++)
				sum += A[r][t] * A[s][t];
			B[r][s] = sum;
			B[s][r] = sum;
		}
	for (r = 0; r < n; r++)
		for (s = 0; s < n; s++)
			A[r][s] = B[r][s];
	free(B);
}

static void kernel_cholesky(int n, double A[n][n])
{
	int i, j, k;

	for (i = 0; i < n; i++) {
		//j<i
		for (j = 0; j < i; j++) {
			for (k = 0; k < j; k++) {
				A[i][j] -= A[i][k] * A[j][k];
			}
			A[i][j] /= A[j][j];
		}
		// i==j case
		for (k = 0; k < i; k++) {
			A[i][i] -= A[i][k] * A[i][k];
		}
		A[i][i] = sqrt(A[i][i]);
	}
}

int main(int argc, char **argv){

	if (argc < 2) {
		printf ("ERROR! Usage: my_program <matrix-size>\n\n \tE.g. -> ./my_program 2700\n\n");
		exit(1);
	}

	int n = atoi(argv[1]);
	double (*A)[n] = malloc(sizeof(double) * n * n);

	init_array(n, A);

	#ifdef ELAPSEDTIME
		struct timeval start, end;
		gettimeofday(&start, NULL);
	#endif

	kernel_cholesky(n, A);

	#ifdef ELAPSEDTIME
		gettimeofday(&end, NULL);
		double delta = ((end.tv_sec  - start.tv_sec) * 1000000u + end.tv_usec - start.tv_usec) / 1.e6;
		printf("Execution time\t%f\n", delta);
	#endif

	/* Checksum of the result (lower triangle) */
	double sum = 0;
	int i, j;
	for (i = 0; i < n; i++)
		for (j = 0; j <= i; j++)
			sum += A[i][j];
	printf("%.8e\n", sum);

	free(A);
	return 0;
}
