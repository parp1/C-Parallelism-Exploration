// Basic runthrough of the OpenMP library.
// Approximates pi with discrete sums.
//
// Parth Pendurkar
// 06/19/2018

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int NUM_THREADS;
static long num_steps = 100000;
double dx;

int main(int argc, char** argv)
{
	NUM_THREADS = 4; // defining the number of threads to be used
	if (argc > 2)
		printf("Single optional argument is the # threads between 1 and 8. Using default (4).\n");
	else if (argc == 2)
	{
		int arg = atoi(argv[1]);
		if (arg <= 0 || arg > 8) printf("Single optional argument is the # threads between 1 and 8. Using default (4).\n");
		else { NUM_THREADS = arg; printf("Running program using %d threads.\n", arg); }
	}

	omp_set_num_threads(NUM_THREADS);

	double t0 = omp_get_wtime(); // openmp start time

	int i;
	double pi, x, sum = 0.0;
	dx = 1.0 / (double) num_steps;

	#pragma omp parallel default(none) shared(num_steps, dx, sum) 
	{
		#pragma omp single nowait
		printf("This will be printed by one thread.\n"); // printed by one thread, nowait makes sure other threads aren't paused

		#pragma omp for private(i, x) reduction(+: sum)
		for (i = 0; i < num_steps; i++)
		{
			x = (i + 0.5) * dx;
			sum += 4.0 / (1.0 + x * x);
		}
	}

	pi = sum * dx;

	double t1 = omp_get_wtime(); // openmp end time

	printf("%0.10f\n", pi);
	printf("time: %0.10f\n", t1 - t0);
	return 0;
}