/*
shapeblobs - 3D metaballs in a shaped window
Copyright (C) 2016  John Tsiombikas <nuclear@member.fsf.org>

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
#ifndef TIMER_H_
#define TIMER_H_

unsigned long get_time_msec(void);
void sleep_msec(unsigned long msec);

double get_time_sec(void);
void sleep_sec(double sec);

#endif	/* TIMER_H_ */
