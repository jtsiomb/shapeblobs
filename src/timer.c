/*
shapeblobs - 3D metaballs in a shaped window
Copyright (C) 2016-2026  John Tsiombikas <nuclear@mutantstargoat.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "timer.h"

#if defined(__APPLE__) && !defined(__unix__)
#define __unix__
#endif

#if defined(__unix__) || defined(__unix)
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
#ifdef _MSC_VER
#pragma comment(lib, "winmm.lib")
#endif

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
