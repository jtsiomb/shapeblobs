#ifndef TIMER_H_
#define TIMER_H_

unsigned long get_time_msec(void);
void sleep_msec(unsigned long msec);

double get_time_sec(void);
void sleep_sec(double sec);

#endif	/* TIMER_H_ */
