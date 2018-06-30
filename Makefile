# Makefile for parallelism tests.
# The following variables are used:

CC = gcc # defines the compiler to be used
CFLAGS = -O3 -g3 -Wall -Wextra -march=native # flags to be compiled with
LDLIBS = -lpthread -fopenmp # libraries to link to

# default target — what is run when 'make' command is called
default: pthread pthread_alternative openmp lock
	

# symbol at the end of the line says use the same name as the target
# in this case, the top line is equivalent to $(CC) $(CFLAGS) $(LDLIBS) pthread.c -o pthread
pthread:
	$(CC) $(CFLAGS) $(LDLIBS) pthread.c -o $@ 

pthread_alternative:
	$(CC) $(CFLAGS) $(LDLIBS) pthread_alternative.c -o $@

openmp:
	$(CC) $(CFLAGS) $(LDLIBS) openmp.c -o $@

lock:
	$(CC) $(CFLAGS) $(LDLIBS) lock.c -o $@

clean:
	rm -f pthread pthread_alternative openmp lock mandel

# phony targets are in place to ensure default behavior persists even if there are filenames with the same name as the target
# in this case, maintains behavior if there are ever files called default or clean in the directory
.PHONY: default clean 
	