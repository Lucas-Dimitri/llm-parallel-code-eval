/* Alternating Direction Implicit solver (2D heat diffusion).
 * From PolyBench/C 4.2.1 (stencils/adi), stamped May 10, 2016,
 * http://polybench.sourceforge.net. Copyright (c) 2010-2016, Ohio State
 * University; distributed under the license in programs/LICENSE-PolyBench.txt.
 * Made self-contained:
 * same kernel, double precision, time steps and grid size from the command line.
 * Only the kernel is timed, as in PolyBench. */
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

static void init_array(int n, double u[n][n])
{
	int i, j;

	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++)
			u[i][j] = (double)(i + n-j) / n;
}

/* Based on a Fortran code fragment from Figure 5 of
 * "Automatic Data and Computation Decomposition on Distributed Memory Parallel Computers"
 * by Peizong Lee and Zvi Meir Kedem, TOPLAS, 2002 */
static void kernel_adi(int tsteps, int n,
		double u[n][n], double v[n][n], double p[n][n], double q[n][n])
{
	int t, i, j;
	double DX, DY, DT;
	double B1, B2;
	double mul1, mul2;
	double a, b, c, d, e, f;

	DX = 1.0/(double)n;
	DY = 1.0/(double)n;
	DT = 1.0/(double)tsteps;
	B1 = 2.0;
	B2 = 1.0;
	mul1 = B1 * DT / (DX * DX);
	mul2 = B2 * DT / (DY * DY);

	a = -mul1 / 2.0;
	b = 1.0+mul1;
	c = a;
	d = -mul2 / 2.0;
	e = 1.0+mul2;
	f = d;

	for (t=1; t<=tsteps; t++) {
		//Column Sweep
		for (i=1; i<n-1; i++) {
			v[0][i] = 1.0;
			p[i][0] = 0.0;
			q[i][0] = v[0][i];
			for (j=1; j<n-1; j++) {
				p[i][j] = -c / (a*p[i][j-1]+b);
				q[i][j] = (-d*u[j][i-1]+(1.0+2.0*d)*u[j][i] - f*u[j][i+1]-a*q[i][j-1])/(a*p[i][j-1]+b);
			}

			v[n-1][i] = 1.0;
			for (j=n-2; j>=1; j--) {
				v[j][i] = p[i][j] * v[j+1][i] + q[i][j];
			}
		}
		//Row Sweep
		for (i=1; i<n-1; i++) {
			u[i][0] = 1.0;
			p[i][0] = 0.0;
			q[i][0] = u[i][0];
			for (j=1; j<n-1; j++) {
				p[i][j] = -f / (d*p[i][j-1]+e);
				q[i][j] = (-a*v[i-1][j]+(1.0+2.0*a)*v[i][j] - c*v[i+1][j]-d*q[i][j-1])/(d*p[i][j-1]+e);
			}
			u[i][n-1] = 1.0;
			for (j=n-2; j>=1; j--) {
				u[i][j] = p[i][j] * u[i][j+1] + q[i][j];
			}
		}
	}
}

int main(int argc, char **argv){

	if (argc < 3) {
		printf ("ERROR! Usage: my_program <time-steps> <grid-size>\n\n \tE.g. -> ./my_program 150 1000\n\n");
		exit(1);
	}

	int tsteps = atoi(argv[1]);
	int n = atoi(argv[2]);
	double (*u)[n] = malloc(sizeof(double) * n * n);
	double (*v)[n] = malloc(sizeof(double) * n * n);
	double (*p)[n] = malloc(sizeof(double) * n * n);
	double (*q)[n] = malloc(sizeof(double) * n * n);

	init_array(n, u);

	#ifdef ELAPSEDTIME
		struct timeval start, end;
		gettimeofday(&start, NULL);
	#endif

	kernel_adi(tsteps, n, u, v, p, q);

	#ifdef ELAPSEDTIME
		gettimeofday(&end, NULL);
		double delta = ((end.tv_sec  - start.tv_sec) * 1000000u + end.tv_usec - start.tv_usec) / 1.e6;
		printf("Execution time\t%f\n", delta);
	#endif

	/* Checksum of the result. The grid converges close to 1.0, so the
	   differences are in the last digits: print at full precision. */
	double sum = 0;
	int i, j;
	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++)
			sum += u[i][j];
	printf("%.17g\n", sum);

	free(u); free(v); free(p); free(q);
	return 0;
}
