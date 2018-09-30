// Advanced runthrough of the OpenMP library.
// Showcases OpenMP Tasks.
// Approximates pi with discrete sums.
//
// Parth Pendurkar
// 09/30/2018

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void process(int* pointer)
{
	printf("process() called by thread %d...\n", omp_get_thread_num());
	sleep(1); // sleep for 1 millisecond
	(*pointer)++;
}

void print_array(int* array, int size)
{
	int i;
	for (i = 0; i < size; i++)
		printf("%d", array[i]);
	printf("\n");
}

int main()
{
	// first let's create a dynamically allocated array
	int ARRAY_SIZE = 10;
	int* array = (int*) calloc(ARRAY_SIZE, sizeof(int)); // initialize all ints to 0
	int i;

	printf("- Running w/o tasks -\n");
	double t0 = omp_get_wtime(); // openmp start time

	for (i = 0; i < ARRAY_SIZE; i++)
	{
		process(array + i);
	}

	double t1 = omp_get_wtime(); // openmp end time
	printf("Time w/o tasks: %0.10f\n", t1 - t0);
	printf("Array: ");
	print_array(array, ARRAY_SIZE);

	printf("- Running w/ tasks -\n");
	t0 = omp_get_wtime(); // openmp start time

	#pragma omp parallel
	{
		#pragma omp single
		{
			for (i = 0; i < ARRAY_SIZE; i++)
			{
				printf("One task created by thread %d\n", omp_get_thread_num());
				#pragma omp task firstprivate(i)
				process(array + i);
			}
		}
	}

	t1 = omp_get_wtime(); // openmp end time
	printf("Time w/ tasks: %0.10f\n", t1 - t0);
	printf("Array: ");
	print_array(array, ARRAY_SIZE);

	free(array);
	return 0;
}