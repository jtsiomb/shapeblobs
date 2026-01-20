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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "blobs.h"
#include "msurf2.h"
#include "timer.h"
#include "image.h"
#include "img_refmap.h"

#undef RANDOM_BLOB_PARAMS

struct metaball {
	float energy;
	float path_scale[3];
	float path_offset[3];
	float phase_offset, speed;
};

#define frand() ((float)rand() / (float)RAND_MAX)

static struct msurf_volume vol;

static struct metaball mballs[MAX_MBALLS] = {
	{2.18038, {1.09157, 1.69766, 1}, {0.622818, 0.905624, 0}, 1.24125, 0.835223},
	{2.03646, {0.916662, 1.2161, 1}, {0.118734, 0.283516, 0}, 2.29201, 1.0134},
	{2.40446, {1.87429, 1.57595, 1}, {0.298566, -0.788474, 0}, 3.8137, 0.516301},
	{0.985774, {0.705847, 0.735019, 1}, {0.669189, -0.217922, 0}, 0.815497, 0.608809},
	{2.49785, {0.827385, 1.75867, 1}, {0.0284513, 0.247808, 0}, 1.86002, 1.13755},
	{1.54857, {1.24037, 0.938775, 1}, {1.04011, 0.596987, 0}, 3.30964, 1.26991},
	{1.30046, {1.83729, 1.02869, 1}, {-0.476708, 0.676994, 0}, 5.77441, 0.569755},
	{2.39865, {1.28899, 0.788321, 1}, {-0.910677, 0.359099, 0}, 5.5935, 0.848893}
};

static int win_width, win_height;
static unsigned char *stencil;
static unsigned int tex;

static int use_shape = 1;

static unsigned long start_time;

static float ltdir[][4] = {{0, 1, 0.8, 0}, {0, -1, 0.5, 0}};
static float ltcol[][4] = {{0.9, 0.6, 0.5, 1}, {0.3, 0.2, 0.6, 1}};

static float mtl_kd[] = {1, 1, 1, 1};
static float mtl_ks[] = {0.6, 0.6, 0.6, 1};
static float mtl_black[] = {0, 0, 0, 1};

int use_envmap = 1;
int num_mballs = MAX_MBALLS;
char *tex_fname;

static void draw_mesh(struct msurf_vertex *varr, unsigned int vcount);


int init()
{
	int i;
	struct image *imgfile = 0;

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_NORMALIZE);

	for(i=0; i<2; i++) {
		glEnable(GL_LIGHT0 + i);
		glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, ltcol[i]);
		glLightfv(GL_LIGHT0 + i, GL_SPECULAR, ltcol[i]);
		glLightfv(GL_LIGHT0 + i, GL_POSITION, ltdir[i]);
	}

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, mtl_kd);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, mtl_ks);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 60.0f);

	if(!use_envmap) {
		glEnable(GL_LIGHTING);
	}

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
	glStencilFunc(GL_ALWAYS, 0, 0xffffffff);

	if(use_envmap) {
		if(tex_fname) {
			if(!(imgfile = load_image(tex_fname))) {
				fprintf(stderr, "failed to load image file: %s\n", tex_fname);
			}
		}

		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if(imgfile) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imgfile->width, imgfile->height, 0,
					GL_RGBA, GL_UNSIGNED_BYTE, imgfile->pixels);
			free_image(imgfile);
		} else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_refmap.width, img_refmap.height, 0,
					GL_RGB, GL_UNSIGNED_BYTE, img_refmap.pixels);
		}
		glEnable(GL_TEXTURE_2D);

		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);

		glMatrixMode(GL_TEXTURE);
		glScalef(1, -1, 1);
		glMatrixMode(GL_MODELVIEW);
	}

	if(msurf_init(&vol) == -1) {
		return -1;
	}
	if(msurf_metaballs(&vol, MAX_MBALLS) == -1) {
		msurf_destroy(&vol);
		return -1;
	}
	vol.isoval = 8;
	msurf_resolution(&vol, 40, 40, 40);
	msurf_size(&vol, 7, 7, 7);

#ifdef RANDOM_BLOB_PARAMS
	{
		int j;
		srand(time(0));
		for(i=0; i<MAX_MBALLS; i++) {
			mballs[i].energy = frand() * 2.0 + 0.5;
			for(j=0; j<2; j++) {
				mballs[i].path_scale[j] = frand() * 1.5 + 0.5;
				mballs[i].path_offset[j] = (frand() * 2.0 - 1.0) * 1.1;
			}
			mballs[i].path_scale[2] = 1;
			mballs[i].path_offset[2] = 0;
			mballs[i].phase_offset = frand() * M_PI * 2.0;
			mballs[i].speed = frand() + 0.5;
		}
	}
#endif

	for(i=0; i<MAX_MBALLS; i++) {
		vol.mballs[i].energy = mballs[i].energy;
	}

	vol.num_mballs = num_mballs;

	start_time = get_time_msec();
	return 0;
}

void cleanup()
{
	msurf_destroy(&vol);
}

static void update(double sec)
{
	int i;

	for(i=0; i<vol.num_mballs; i++) {
		float t = sec * mballs[i].speed + mballs[i].phase_offset;
		vol.mballs[i].pos.x = cos(t) * mballs[i].path_scale[0] +
			mballs[i].path_offset[0] + 3.5f;
		vol.mballs[i].pos.y = sin(t) * mballs[i].path_scale[1] +
			mballs[i].path_offset[1] + 3.5f;
		vol.mballs[i].pos.z = -cos(t) * mballs[i].path_scale[2] +
			mballs[i].path_offset[2] + 3.5f;
	}

	msurf_begin(&vol);
	msurf_genmesh(&vol);
}

void display()
{
	unsigned int msec = get_time_msec() - start_time;
	double t = (double)msec / 1000.0;

	update(t);

	glClearColor(0.1, 0.1, 0.1, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(-3.5, -3.5, -8 - 3.5);

	draw_mesh(vol.varr, vol.num_verts);

	swap_buffers();
	assert(glGetError() == GL_NO_ERROR);

	if(use_shape) {
		glReadPixels(0, 0, win_width, win_height, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencil);
		window_shape(stencil, win_width, win_height);
	}
}

static void draw_mesh(struct msurf_vertex *varr, unsigned int vcount)
{
	glVertexPointer(3, GL_FLOAT, sizeof *varr, &varr->x);
	glNormalPointer(GL_FLOAT, sizeof *varr, &varr->nx);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);

	glDrawArrays(GL_TRIANGLES, 0, vcount);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}

void reshape(int x, int y)
{
	glViewport(0, 0, x, y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (float)x / (float)y, 0.5, 100.0);

	if(x != win_width || y != win_height) {
		free(stencil);
		stencil = malloc(x * y);
		win_width = x;
		win_height = y;
	}
}

void keyboard(int key, int pressed)
{
	if(pressed) {
		switch(key) {
		case 27:
		case 'q':
		case 'Q':
			quit();
			break;

		case 's':
		case 'S':
			use_shape = !use_shape;
			if(!use_shape) {
				window_shape(0, win_width, win_height);
			}
			break;

		case '-':
			if(vol.num_mballs > 1) {
				vol.num_mballs--;
			}
			break;

		case '=':
			if(vol.num_mballs < MAX_MBALLS) {
				vol.num_mballs++;
			}
			break;

		case 't':
		case 'T':
			use_envmap ^= 1;
			if(use_envmap) {
				glEnable(GL_TEXTURE_2D);
				glDisable(GL_LIGHTING);
			} else {
				glDisable(GL_TEXTURE_2D);
				glEnable(GL_LIGHTING);
			}
			break;

		default:
			break;
		}
	}
}
