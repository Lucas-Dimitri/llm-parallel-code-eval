/* Nussinov algorithm (RNA folding, dynamic programming).
 * From PolyBench/C 4.2.1 (medley/nussinov), stamped May 10, 2016,
 * http://polybench.sourceforge.net. Copyright (c) 2010-2016, Ohio State
 * University; distributed under the license in programs/LICENSE-PolyBench.txt.
 * Made self-contained:
 * same kernel, int scores, sequence length from the command line.
 * Only the kernel is timed, as in PolyBench. */
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

/* RNA bases represented as chars, range is [0,3] */
typedef char base;

#define match(b1, b2) (((b1)+(b2)) == 3 ? 1 : 0)
#define max_score(s1, s2) ((s1 >= s2) ? s1 : s2)

static void init_array(int n, base seq[n], int table[n][n])
{
	int i, j;

	//base is AGCT/0..3
	for (i=0; i <n; i++) {
		seq[i] = (base)((i+1)%4);
	}

	for (i=0; i <n; i++)
		for (j=0; j <n; j++)
			table[i][j] = 0;
}

/*
  Original version by Dave Wonnacott at Haverford College <davew@cs.haverford.edu>,
  with help from Allison Lake, Ting Zhou, and Tian Jin,
  based on algorithm by Nussinov, described in Allison Lake's senior thesis.
*/
static void kernel_nussinov(int n, base seq[n], int table[n][n])
{
	int i, j, k;

	for (i = n-1; i >= 0; i--) {
		for (j=i+1; j<n; j++) {

			if (j-1>=0)
				table[i][j] = max_score(table[i][j], table[i][j-1]);
			if (i+1<n)
				table[i][j] = max_score(table[i][j], table[i+1][j]);

			if (j-1>=0 && i+1<n) {
				/* don't allow adjacent elements to bond */
				if (i<j-1)
					table[i][j] = max_score(table[i][j], table[i+1][j-1]+match(seq[i], seq[j]));
				else
					table[i][j] = max_score(table[i][j], table[i+1][j-1]);
			}

			for (k=i+1; k<j; k++) {
				table[i][j] = max_score(table[i][j], table[i][k] + table[k+1][j]);
			}
		}
	}
}

int main(int argc, char **argv){

	if (argc < 2) {
		printf ("ERROR! Usage: my_program <sequence-length>\n\n \tE.g. -> ./my_program 3000\n\n");
		exit(1);
	}

	int n = atoi(argv[1]);
	base *seq = malloc(sizeof(base) * n);
	int (*table)[n] = malloc(sizeof(int) * n * n);

	init_array(n, seq, table);

	#ifdef ELAPSEDTIME
		struct timeval start, end;
		gettimeofday(&start, NULL);
	#endif

	kernel_nussinov(n, seq, table);

	#ifdef ELAPSEDTIME
		gettimeofday(&end, NULL);
		double delta = ((end.tv_sec  - start.tv_sec) * 1000000u + end.tv_usec - start.tv_usec) / 1.e6;
		printf("Execution time\t%f\n", delta);
	#endif

	/* Optimal score and checksum of the result (upper triangle) */
	long long sum = 0;
	int i, j;
	for (i = 0; i < n; i++)
		for (j = i; j < n; j++)
			sum += table[i][j];
	printf("%d %lld\n", table[0][n-1], sum);

	free(seq); free(table);
	return 0;
}
