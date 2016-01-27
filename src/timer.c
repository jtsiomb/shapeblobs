#include "timer.h"

#if defined(__APPLE__) && !defined(__unix__)
#define __unix__
#endif

#ifdef __unix__
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#ifdef CLOCK_MONOTONIC
unsigned long get_time_msec(void)
{
	struct timespec ts;
	static struct timespec ts0;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	if(ts0.tv_sec == 0 && ts0.tv_nsec == 0) {
		ts0 = ts;
		return 0;
	}
	return (ts.tv_sec - ts0.tv_sec) * 1000 + (ts.tv_nsec - ts0.tv_nsec) / 1000000;
}
#else	/* no fancy POSIX clocks, fallback to good'ol gettimeofday */
unsigned long get_time_msec(void)
{
	struct timeval tv;
	static struct timeval tv0;

	gettimeofday(&tv, 0);
	if(tv0.tv_sec == 0 && tv0.tv_usec == 0) {
		tv0 = tv;
		return 0;
	}
	return (tv.tv_sec - tv0.tv_sec) * 1000 + (tv.tv_usec - tv0.tv_usec) / 1000;
}
#endif	/* !posix clock */

void sleep_msec(unsigned long msec)
{
	usleep(msec * 1000);
}
#endif

#ifdef WIN32
#include <windows.h>
#pragma comment(lib, "winmm.lib")

unsigned long get_time_msec(void)
{
	return timeGetTime();
}

void sleep_msec(unsigned long msec)
{
	Sleep(msec);
}
#endif

double get_time_sec(void)
{
	return get_time_msec() / 1000.0f;
}

void sleep_sec(double sec)
{
	if(sec > 0.0f) {
		sleep_msec(sec * 1000.0f);
	}
}
