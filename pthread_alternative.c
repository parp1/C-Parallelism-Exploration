// Basic runthrough of the pthread library. (alternative)
// Uses dynamic allocation and pthread_exit() with pthread_join()'s return parameter.
//
// Parth Pendurkar
// 06/19/2018

#include <stdio.h>
#include <stdlib.h>
#include <omp.h> // only using timing function
#include <pthread.h> // need to include this to use pthread

#define min(a, b) ((a < b) ? a : b) // defining a min macro, since C stdlib doesn't include it

int NUM_THREADS;
static long num_steps = 1000000; // the number of steps to use in the approximation
double dx; // global, so all threads can access it

void* thread_function(void* arg) // a thread performs a section of the integration
{	
	int i;
	double x;
	int thread_id = *(int*) arg;
	int thread_steps = (num_steps + NUM_THREADS) / NUM_THREADS; // ceiling function so that we can use min()

	int start_index = thread_id * thread_steps; // casting then dereferncing thread_id pointer
	int end_index = min(start_index + thread_steps, num_steps); // if NUM_THREADS doesn't divide num_steps evenly, this handles the last iteration correctly
	
	double* thread_sum = (double*) malloc(sizeof(double));
	for (i = start_index; i < end_index; i++)
	{
		x = (i + 0.5) * dx;
		(*thread_sum) += 4.0 / (1.0 + x * x);
	}

	// uncomment to see how much work each thread is doing:
	// printf("Thread %d completed %d/%ld steps\n", thread_id, (end_index - start_index), num_steps);
	pthread_exit((void*) thread_sum);
}

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
	
	double t0 = omp_get_wtime(); // openmp start time

	int t;
	pthread_t thread_ids[NUM_THREADS]; // these are both...
	int t_ids[NUM_THREADS];

	double total_sum = 0.0;
	dx = 1.0 / (double) num_steps;

	for (t = 0; t < NUM_THREADS; t++)
	{
		t_ids[t] = t;
		pthread_create(&thread_ids[t], NULL, thread_function, &t_ids[t]);
	}

	
	for (t = 0; t < NUM_THREADS; t++)
	{
		double* returned_total;
		pthread_join(thread_ids[t], (void**) &returned_total); // give the address of the pointer, lookup pthread_join if confused
		total_sum += *returned_total;
		free(returned_total); // freeing the dynamically allocated thread sum
	}

	double pi = total_sum * dx;

	double t1 = omp_get_wtime(); // openmp end time

	printf("%0.10f\n", pi); // prints the calculated value of pi
	printf("time: %0.10f\n", t1 - t0); // prints the time the entire process took
	return 0;
}