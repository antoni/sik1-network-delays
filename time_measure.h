#define _POSIX_C_SOURCE 200809L
#define USED_CLOCK CLOCK_MONOTONIC
#define NANOS 1000000000LL

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Converts time in nanoseconds to milliseconds
void convert_to_micros(uint64_t *nanos)
{
	*nanos = *nanos / 1000 + (*nanos % 1000 >= 500);
}

// Returns current time in nanoseconds
uint64_t get_nanos()
{
	struct timespec ts;

	if (clock_gettime(CLOCK_REALTIME, &ts))
		exit(EXIT_FAILURE);

	return ts.tv_sec * NANOS + ts.tv_nsec;
}
