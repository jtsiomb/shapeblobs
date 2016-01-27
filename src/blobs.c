#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "blobs.h"
#include "metasurf.h"
#include "timer.h"
#include "img_refmap.h"

struct metaball {
	float x, y, z;
	float energy;
	float path_scale[3];
	float path_offset[3];
	float phase_offset, speed;
};

static void vertex(struct metasurface *ms, float x, float y, float z);
static float eval(struct metasurface *ms, float x, float y, float z);
static float frand(void);

static struct metasurface *msurf;

#define NUM_MBALLS	8
static struct metaball mballs[NUM_MBALLS] = {
	{0, 0, 0, 2.18038, {1.09157, 1.69766, 1}, {0.622818, 0.905624, 0}, 1.24125, 0.835223},
	{0, 0, 0, 2.03646, {0.916662, 1.2161, 1}, {0.118734, 0.283516, 0}, 2.29201, 1.0134},
	{0, 0, 0, 2.40446, {1.87429, 1.57595, 1}, {0.298566, -0.788474, 0}, 3.8137, 0.516301},
	{0, 0, 0, 0.985774, {0.705847, 0.735019, 1}, {0.669189, -0.217922, 0}, 0.815497, 0.608809},
	{0, 0, 0, 2.49785, {0.827385, 1.75867, 1}, {0.0284513, 0.247808, 0}, 1.86002, 1.13755},
	{0, 0, 0, 1.54857, {1.24037, 0.938775, 1}, {1.04011, 0.596987, 0}, 3.30964, 1.26991},
	{0, 0, 0, 1.30046, {1.83729, 1.02869, 1}, {-0.476708, 0.676994, 0}, 5.77441, 0.569755},
	{0, 0, 0, 2.39865, {1.28899, 0.788321, 1}, {-0.910677, 0.359099, 0}, 5.5935, 0.848893}
};

static int win_width, win_height;
static unsigned char *stencil;
static unsigned int tex;

static int use_shape = 1;

static unsigned long start_time;

int init()
{
	int i, j;

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_NORMALIZE);
	/*glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);*/

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
	glStencilFunc(GL_ALWAYS, 0, 0xffffffff);

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_refmap.width, img_refmap.height, 0,
			GL_RGB, GL_UNSIGNED_BYTE, img_refmap.pixel_data);
	glEnable(GL_TEXTURE_2D);

	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);

	if(!(msurf = msurf_create())) {
		return -1;
	}
	msurf_set_threshold(msurf, 8);
	msurf_set_inside(msurf, MSURF_GREATER);
	msurf_set_bounds(msurf, -3.5, 3.5, -3.5, 3.5, -3.5, 3.5);
	msurf_eval_func(msurf, eval);
	msurf_vertex_func(msurf, vertex);

	/*
	for(i=0; i<NUM_MBALLS; i++) {
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
	*/

	start_time = get_time_msec();
	return 0;
}

void cleanup()
{
	msurf_free(msurf);
}

static void update(double sec)
{
	int i;

	for(i=0; i<NUM_MBALLS; i++) {
		float t = sec * mballs[i].speed + mballs[i].phase_offset;
		mballs[i].x = cos(t) * mballs[i].path_scale[0] + mballs[i].path_offset[0];
		mballs[i].y = sin(t) * mballs[i].path_scale[1] + mballs[i].path_offset[1];
		mballs[i].z = -cos(t) * mballs[i].path_scale[2] + mballs[i].path_offset[2];
	}
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
	glTranslatef(0, 0, -8);

	glFrontFace(GL_CW);
	glBegin(GL_TRIANGLES);
	msurf_polygonize(msurf);
	glEnd();

	swap_buffers();
	assert(glGetError() == GL_NO_ERROR);

	if(use_shape) {
		glReadPixels(0, 0, win_width, win_height, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencil);
		window_shape(stencil, win_width, win_height);
	}
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
		}
	}
}

static void vertex(struct metasurface *ms, float x, float y, float z)
{
	static const float delta = 0.01;

	float val = eval(ms, x, y, z);
	float dfdx = eval(ms, x + delta, y, z) - val;
	float dfdy = eval(ms, x, y + delta, z) - val;
	float dfdz = eval(ms, x, y, z + delta) - val;

	glNormal3f(dfdx, dfdy, dfdz);
	glVertex3f(x, y, z);
}

static float eval(struct metasurface *ms, float x, float y, float z)
{
	int i;

	float sum = 0.0f;

	for(i=0; i<NUM_MBALLS; i++) {
		float dx = x - mballs[i].x;
		float dy = y - mballs[i].y;
		float dz = z - mballs[i].z;
		float dsq = dx * dx + dy * dy + dz * dz;

		sum += mballs[i].energy / dsq;
	}
	return sum;
}

static float frand(void)
{
	return (float)rand() / (float)RAND_MAX;
}
