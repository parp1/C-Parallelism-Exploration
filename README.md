# C-Parallelism-Exploration
A short exploration of OpenMP and Pthreads. The exploration is based off of an approximation of pi by taking the integral of 4 / (1 + x^2) from 0 to 1. Most of the programs can be given a command line argument specifying the number of threads to use. The default is 4.

Overview:

* [OpenMP](https://github.com/parp1/C-Parallelism-Exploration#openmp)
* [Advaned OpenMP](https://github.com/parp1/C-Parallelism-Exploration#advanced-openmp)
* [Pthreads](https://github.com/parp1/C-Parallelism-Exploration#pthreads)

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

## Advanced OpenMP

#### openmp_adv: tasks

This program will slightly demonstrate what OpenMP tasks are. Again, this is a slightly more advanced topic and not very frequently called upon, but useful to be familiar with nevertheless.

To get started, let's discuss what Tasks are in parallel programming. Tasks have the following characteristics:

* They have code to execute
* Has some sort of data environment
* Contains internal control variables to control that data environment, known as ICV's (things like the default number of threads, etc...)

The runtime system decides when tasks are executed. Essentially, there is a task scheduler that takes care of the queue of tasks that need to be completed. The following is a basic overview of how a task is created:

```C
// generating foo() and bar() tasks
#pragma omp parallel
{
	#pragma omp task // each thread will create a task from the subsequent structured block
	foo();
	#pragma omp barrier // the barrier here makes sure no thread goes past until all the previous tasks are executed

	#pragma omp single // the single clause has an implied barrier at the end
	{
		#pragma omp task
		bar();
	}
}
```

Here, we see two common constructs for tasks. The first `parallel` section is required for OpenMP parallel programming to work in any way. The next construct, `task`, ensures that each thread that is created will schedule a task from the subsequent structured block, which in this case happens to be just one line, the function `foo()`. The `barrier` construct at the end ensures that no thread will continue past that statement until all the previous threads are completed with their tasks.

The `single` ensures that only one thread will execute the code inside the block, which in this case creates a task from the function `bar()`. The OpenMP `single` construct actually implies a `barrier` at the end, so each thread will wait until the thread that has taken up the aforementioned task is done. (This behavior can again be halted by using the `nowait` construct.) Note that even though one thread creates the task in this case, that task could in turn spawn more tasks, which other threads could work on.

Now, let's look at something a bit more complicated, a fibonacci solver. This sequence can easily be solved using recursion, so let's use tasks to create a sort of recursive tree of subtasks that will "fold back up" into an answer:

```C
#pragma omp parallel
{
	int fibonacci(int n)
	{
		int x, y;
		if (n < 2) return n; // base case of the fibonacci sequence

		#pragma omp task shared(x)
		x = fibonacci(n - 1); // first recursive call

		#pragma omp task shared(y)
		y = fibonacci(n - 2); // second recursive call

		#pragma omp taskwait
		return x + y;
	}
}
```  

This is a pretty simple layout to follow, where each task is going to recursively create more and more tasks. The only confusing part might be the use of the `shared` constructs. Simply, when `x` and `y` are declared inside the parallel region they are automatically private variables. Hence, without the `shared` they would remain private and be undefined when the function returns their sum.

Now, an example of processing a linked list correctly using tasks, assuming `head` is a pointer to the head of the relevant linked list:

```C
#pragma omp parallel
{
	#pragma omp single
	{
		node* p = head;
		while(p)
		{
			#pragma omp task firstprivate(p)
			process(p);
			p = p->next;
		}
	}
}
```

This example creates a `parallel` section, asks one thread to start creating tasks starting at the linked list's head, and ensures that there are no race conditions created with the `shared` variable `p` by labeling it `firstprivate` for each task. Hence, each task will have a private pointer to the relevant node it is calling the function `process()` on. Note, there will be one thread incrementing the pointer and creating tasks, the rest of the threads will be processing those scheduled tasks. Hence, a lot of time is saved.

The `openmp_adv` program does a trivial simulation of this concept with pointers to a dynamically allocated integer array. The console output also attempts to show how the threads are handling work.

To see more concepts related to advanced OpenMP, take a look at the following:

* `threadprivate` variables
* different flavors of `atomic`
* the memory model and `flush` (pretty much not worth looking into...)

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
