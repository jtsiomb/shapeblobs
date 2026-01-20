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
#ifndef MSURF2_H_
#define MSURF2_H_

#include "cgmath/cgmath.h"

#define INLINE	__inline

enum {
	MSURF_VALID		= 0x100,
	MSURF_POSVALID	= 0x200,
	MSURF_GRADVALID	= 0x400,
	MSURF_FLOOR		= 0x800
};

struct msurf_volume;

struct msurf_voxel {
	float val;
	cgm_vec3 pos;
	cgm_vec3 grad;
	unsigned int flags;
};

struct msurf_cell {
	struct msurf_volume *vol;
	unsigned int x, y, z;
	struct msurf_voxel *vox[8];
	unsigned int flags;
	struct msurf_cell *next;
};

struct msurf_metaball {
	float energy;
	cgm_vec3 pos;
};

struct msurf_vertex {
	float x, y, z;
	float nx, ny, nz;
};

struct msurf_volume {
	unsigned int xres, yres, zres;	/* useful X,Y,Z volume resolution */
	unsigned int xstore, ystore, xystore;	/* actual storage size (always pow2) */
	unsigned int xshift, yshift, xyshift;	/* shifts to access rows/slices */
	unsigned int num_store;		/* total storage count (xstore*ystore*zres) */

	struct msurf_voxel *voxels;		/* voxels array */
	cgm_vec3 size, rad;				/* size and half-size (radius) of volume */
	float dx, dy, dz;				/* step between voxels (cell size) */

	struct msurf_cell *cells;		/* cells array (space between 8 voxels) */

	float isoval;					/* isosurface value */
	unsigned int flags;

	struct msurf_vertex *varr;		/* isosurface mesh */
	unsigned int num_verts, max_verts;

	struct msurf_metaball *mballs;		/* metaballs */
	unsigned int num_mballs;

	float floor_z, floor_energy;

	int cur;
};

#define msurf_addr(ms, x, y, z) \
	(((z) << (ms)->xyshift) + ((y) << (ms)->xshift) + (x))


int msurf_init(struct msurf_volume *vol);
void msurf_destroy(struct msurf_volume *vol);

void msurf_resolution(struct msurf_volume *vol, int x, int y, int z);
void msurf_size(struct msurf_volume *vol, float x, float y, float z);
int msurf_metaballs(struct msurf_volume *vol, int count);

int msurf_begin(struct msurf_volume *vol);
int msurf_proc_cell(struct msurf_volume *vol, struct msurf_cell *cell);
void msurf_genmesh(struct msurf_volume *vol);

static INLINE void msurf_pos_to_cell(struct msurf_volume *vol, cgm_vec3 pos,
		int *cx, int *cy, int *cz)
{
	int x = (float)(pos.x * vol->xres / vol->size.x);
	int y = (float)(pos.y * vol->yres / vol->size.y);
	int z = (float)(pos.z * vol->zres / vol->size.z);
	*cx = x < 0 ? 0 : (x >= vol->xres ? vol->xres - 1 : x);
	*cy = y < 0 ? 0 : (y >= vol->yres ? vol->yres - 1 : y);
	*cz = z < 0 ? 0 : (z >= vol->zres ? vol->zres - 1 : z);
}

static INLINE void msurf_cell_to_pos(struct msurf_volume *vol, int cx, int cy,
		int cz, cgm_vec3 *pos)
{
	pos->x = (float)cx * vol->size.x / (float)vol->xres;
	pos->y = (float)cy * vol->size.y / (float)vol->yres;
	pos->z = (float)cz * vol->size.z / (float)vol->zres;
}

#endif	/* MSURF2_H_ */
