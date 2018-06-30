#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_THREADS 4
#define NUM_NUMBERS 1000000

omp_lock_t histogram_locks[100];
int histogram[100];

void printHistogram();

int main()
{
	srand(time(NULL));
	int i;	
	omp_set_num_threads(NUM_THREADS);

	double t0 = omp_get_wtime();

	#pragma omp parallel for
	for (i = 0; i < 100; i++)
	{
		omp_init_lock(&histogram_locks[i]);
		histogram[i] = 0;
	}

	// generating random numbers
	#pragma omp parallel for
	for (i = 0; i < NUM_NUMBERS; i++)
	{
		int index = rand() % 100;
		omp_set_lock(&histogram_locks[index]);
		histogram[index]++;
		omp_unset_lock(&histogram_locks[index]);
	}

	for (i = 0; i < 100; i++)
		omp_destroy_lock(&histogram_locks[i]);

	double t1 = omp_get_wtime();

	printHistogram();	
	printf("time: %0.3f\n", t1 - t0);
	return 0;
}

void printHistogram()
{
	int i;

	for (i = 0; i < 99; i++)
	{
		printf("%d - ", histogram[i]);
	}

	printf("%d\n", histogram[99]);
}