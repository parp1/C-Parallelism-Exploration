# C-Parallelism-Exploration
A short exploration of OpenMP and Pthreads. The exploration is based off of an approximation of pi by taking the integral of 4 / (1 + x^2) from 0 to 1. Most of the programs can be given a command line argument specifying the number of threads to use. The default is 4.

## OpenMP

#### openmp

This library basically hides a lot of the low level configuration from the programmer, making parallelization simple.

```C
#pragma omp parallel default(none) shared(num_steps, dx, sum)
```

In the above segment, a parallel section is created where the default characterization of variables is none. This effectively forces the programmer to assign each variable in the scope to either `shared` or `private`.

```C
#pragma omp single nowait
```

This pragma enforces that the subsequent code segment will be executes by one thread only. The `nowait` clause ensures that other threads won't wait until the execution of that segment is complete.

```C
#pragma omp for private(i, x) reduction(+: sum)
```

This directive is the for clause. It parallelizes a for loop, with a reduction for the `sum` variable. There are several things going on here. Since I used the `default(none)` directive in the previous parallel segment, I needed to assign `i` and `x` to be thread-private variables. (Note `i` is automatically private since it is the iterator of the for loop)

The reduction part is interesting. It creates thread-private variables that independently track a thread's sum, and then at the end combines them into the `sum` variable.

The following code would be equivalent:

```C
#pragma omp parallel default(none) shared(num_steps, dx, sum) private(thread_sum)
	{
		#pragma omp single nowait
		printf("This will be printed by one thread.\n"); // printed by one thread, nowait makes sure other threads aren't paused

		double thread_sum = 0;

		#pragma omp for private(i, x)
		for (i = 0; i < num_steps; i++)
		{
			x = (i + 0.5) * dx;
			thread_sum += 4.0 / (1.0 + x * x);
		}

		#pragma omp atomic
		sum += thread_sum;
	}
```

#### lock

This program showcases the power of OpenMP's lock type. The basic premise is accessing array indices to increment them based on occurrences of values — a histogram. Race conditions arise when two threads happen to generate the same random value and try to increment the same index of the histogram array. Locks prevent this.

```C
#pragma omp parallel for
	for (i = 0; i < 100; i++)
	{
		omp_init_lock(&histogram_locks[i]);
		histogram[i] = 0;
	}
```

Here, I create the locks for each index. Since `i` is a private variable, this loop itself can be parallelized.

Next, I generate random numbers, set the lock for the appropriate index, increment, then unset the lock, preventing race conditions:

```C
// generating random numbers
	#pragma omp parallel for
	for (i = 0; i < NUM_NUMBERS; i++)
	{
		int index = rand() % 100;
		omp_set_lock(&histogram_locks[index]);
		histogram[index]++;
		omp_unset_lock(&histogram_locks[index]);
	}
```

Finally, there's for loop to clean up and destroy the locks.

## Pthreads

#### pthread

Essentially, we can break up the sum by threads. Since the thread function can only take one argument, I created a struct to hold necessary information:

```C
struct arg_struct // argument struct, since only one pointer can be passed to pthread_create
{
	int thread_id;
	double thread_sum; // each thread tracks its own sum to avoid race conditions
};
```

The thread function, `void* thread_function(void* arg)` then returns `NULL` since the struct itself will be used to get the necessary value.

Notice in `main()` where the necessary values are passed when the thread is created:

```C
for (t = 0; t < NUM_THREADS; t++)
	{
		args[t].thread_id = t;
		pthread_create(&thread_ids[t], NULL, thread_function, &args[t]);
	}
```

#### pthread_alternative

Does the same thing as above, except uses dynamic allocation and return values.

Notice (line 37):

```C
pthread_exit((void*) thread_sum);
```

Here, `thread_sum` is a pointer to a `double` which stores that specific `p_thread`'s sum. The exit function sets up the address of the given `(void*)` pointer to be returned through the join function:

```C
pthread_join(thread_ids[t], (void**) &returned_total);
```
`returned_total` is a `double*`, which ultimately makes sense. (eg. somewhere inside that function it will set `returned_total = thread_sum` by essentially performing `*(&returned_total) = thread_sum`)

## Makefile (unrelated)

I also created a simple Makefile for the project. The first step was to set up some variables, as follows:

```make
CC = gcc # defines the compiler to be used
CFLAGS = -O3 -g3 -Wall -Wextra -march=native # flags to be compiled with
LDLIBS = -lpthread -fopenmp # libraries to link to
```

These variables are later used when writing the relevant command for the targets:

```make
pthread:
	$(CC) $(CFLAGS) $(LDLIBS) pthread.c -o $@ 
```

Here, the target is `pthread`, and we use the three aforementioned variables to construct the full command. The generated command would be:

```bash
gcc -O3 -g3 -Wall -Wextra -march=native -lpthread -fopenmp pthread.c -o pthread
```

Notice the use of `$@`. This is basically a shortcut for using the target name in the actual target. Other macros include `$<` and `$^` which resolve to the first dependency and all dependencies, respectively. Other macros can be found here: [GNU Make Manual — Automatic Variables](https://www.gnu.org/software/make/manual/html_node/Automatic-Variables.html#Automatic-Variables).

The `default` and `clean` targets are a bit special. The former is run when just the `make` command is called, and `make clean` is normally used to restore the directory.

Thus, we require the `.PHONY` target:

```make
.PHONY: default clean 
```

This target basically ensures that both `default` and `clean` are special targets and filenames that happen to match up with those two words won't interfere with the make process (eg. if there was a file named "clean" in the directory)
