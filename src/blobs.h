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
#ifndef BLOBS_H_
#define BLOBS_H_

int init();
void cleanup();

void display();
void reshape(int x, int y);

void keyboard(int key, int press);

/* implemented in main.c */
void swap_buffers(void);
void quit(void);
void window_shape(unsigned char *pixels, int xsz, int ysz);

#endif	/* BLOBS_H_ */
