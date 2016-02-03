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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include "image.h"

#if  defined(__i386__) || defined(__ia64__) || defined(WIN32) || \
    (defined(__alpha__) || defined(__alpha)) || \
     defined(__arm__) || \
    (defined(__mips__) && defined(__MIPSEL__)) || \
     defined(__SYMBIAN32__) || \
     defined(__x86_64__) || \
     defined(__LITTLE_ENDIAN__)
/* little endian */
#define read_int16_le(f)	read_int16(f)
#else
/* big endian */
#define read_int16_le(f)	read_int16_inv(f)
#endif	/* endian check */


enum {
	IMG_NONE,
	IMG_CMAP,
	IMG_RGBA,
	IMG_BW,

	IMG_RLE_CMAP = 9,
	IMG_RLE_RGBA,
	IMG_RLE_BW
};

#define IS_RLE(x)	((x) >= IMG_RLE_CMAP)
#define IS_RGBA(x)	((x) == IMG_RGBA || (x) == IMG_RLE_RGBA)


struct tga_header {
	uint8_t idlen;			/* id field length */
	uint8_t cmap_type;		/* color map type (0:no color map, 1:color map present) */
	uint8_t img_type;		/* image type:
							 * 0: no image data
							 *	1: uncomp. color-mapped		 9: RLE color-mapped
							 *	2: uncomp. true color		10: RLE true color
							 *	3: uncomp. black/white		11: RLE black/white */
	uint16_t cmap_first;	/* color map first entry index */
	uint16_t cmap_len;		/* color map length */
	uint8_t cmap_entry_sz;	/* color map entry size */
	uint16_t img_x;			/* X-origin of the image */
	uint16_t img_y;			/* Y-origin of the image */
	uint16_t img_width;		/* image width */
	uint16_t img_height;	/* image height */
	uint8_t img_bpp;		/* bits per pixel */
	uint8_t img_desc;		/* descriptor:
							 * bits 0 - 3: alpha or overlay bits
							 * bits 5 & 4: origin (0 = bottom/left, 1 = top/right)
							 * bits 7 & 6: data interleaving */
};

struct tga_footer {
	uint32_t ext_off;		/* extension area offset */
	uint32_t devdir_off;	/* developer directory offset */
	char sig[18];				/* signature with . and \0 */
};


static int check_tga(FILE *fp);
static void *load_tga(FILE *fp, int *xsz, int *ysz);
static uint32_t read_pixel(FILE *fp, int rdalpha);
static int16_t read_int16(FILE *fp);
static int16_t read_int16_inv(FILE *fp);

struct image *load_image(const char *fname)
{
	FILE *fp;
	struct image *img;

	if(!(fp = fopen(fname, "rb"))) {
		fprintf(stderr, "failed to load image %s: %s\n", fname, strerror(errno));
		return 0;
	}

	if(!check_tga(fp)) {
		fprintf(stderr, "file %s is not a valid (or supported) targa image\n", fname);
		fclose(fp);
		return 0;
	}

	if(!(img = malloc(sizeof *img))) {
		perror("failed to allocate memory");
		fclose(fp);
		return 0;
	}

	if(!(img->pixels = load_tga(fp, &img->width, &img->height))) {
		free(img);
		img = 0;
	}
	fclose(fp);
	return img;
}

void free_image(struct image *img)
{
	if(img) {
		free(img->pixels);
		free(img);
	}
}

static int check_tga(FILE *fp)
{
	struct tga_footer foot;

	fseek(fp, -18, SEEK_END);
	fread(foot.sig, 1, 18, fp);

	foot.sig[17] = 0;
	return strcmp(foot.sig, "TRUEVISION-XFILE.") == 0 ? 1 : 0;
}

static void *load_tga(FILE *fp, int *xsz, int *ysz)
{
	struct tga_header hdr;
	unsigned long x, y, sz;
	int i;
	uint32_t *pix, ppixel = 0;
	int rle_mode = 0, rle_pix_left = 0;
	int rdalpha;

	/* read header */
	fseek(fp, 0, SEEK_SET);
	hdr.idlen = fgetc(fp);
	hdr.cmap_type = fgetc(fp);
	hdr.img_type = fgetc(fp);
	hdr.cmap_first = read_int16_le(fp);
	hdr.cmap_len = read_int16_le(fp);
	hdr.cmap_entry_sz = fgetc(fp);
	hdr.img_x = read_int16_le(fp);
	hdr.img_y = read_int16_le(fp);
	hdr.img_width = read_int16_le(fp);
	hdr.img_height = read_int16_le(fp);
	hdr.img_bpp = fgetc(fp);
	hdr.img_desc = fgetc(fp);

	if(feof(fp)) {
		return 0;
	}

	/* only read true color images */
	if(!IS_RGBA(hdr.img_type)) {
		fprintf(stderr, "only true color tga images supported\n");
		return 0;
	}

	fseek(fp, hdr.idlen, SEEK_CUR); /* skip the image ID */

	/* skip the color map if it exists */
	if(hdr.cmap_type == 1) {
		fseek(fp, hdr.cmap_len * hdr.cmap_entry_sz / 8, SEEK_CUR);
	}

	x = hdr.img_width;
	y = hdr.img_height;
	sz = x * y;
	if(!(pix = malloc(sz * 4))) {
		return 0;
	}

	rdalpha = hdr.img_desc & 0xf;

	for(i=0; i<y; i++) {
		uint32_t *ptr;
		int j;

		ptr = pix + ((hdr.img_desc & 0x20) ? i : y-(i+1)) * x;

		for(j=0; j<x; j++) {
			/* if the image is raw, then just read the next pixel */
			if(!IS_RLE(hdr.img_type)) {
				ppixel = read_pixel(fp, rdalpha);
			} else {
                /* otherwise, for RLE... */

				/* if we have pixels left in the packet ... */
				if(rle_pix_left) {
					/* if it's a raw packet, read the next pixel, otherwise keep the same */
					if(!rle_mode) {
						ppixel = read_pixel(fp, rdalpha);
					}
					rle_pix_left--;
				} else {
					/* read the RLE packet header */
					unsigned char packet_hdr = getc(fp);
					rle_mode = (packet_hdr & 128);		/* last bit shows the mode for this packet (1: rle, 0: raw) */
					rle_pix_left = (packet_hdr & ~128);	/* the rest gives the count of pixels minus one (we also read one here, so no +1) */
					ppixel = read_pixel(fp, rdalpha);	/* and read the first pixel of the packet */
				}
			}

			*ptr++ = ppixel;

			if(feof(fp)) break;
		}
	}

	*xsz = x;
	*ysz = y;

	return pix;
}

#define PACK_COLOR32(r,g,b,a) \
	((((a) & 0xff) << 24) | \
	 (((r) & 0xff) << 0) | \
	 (((g) & 0xff) << 8) | \
	 (((b) & 0xff) << 16))

static uint32_t read_pixel(FILE *fp, int rdalpha)
{
	int r, g, b, a;
	b = getc(fp);
	g = getc(fp);
	r = getc(fp);
	a = rdalpha ? getc(fp) : 0xff;
	return PACK_COLOR32(r, g, b, a);
}

int16_t read_int16(FILE *fp)
{
	int16_t v;
	fread(&v, 2, 1, fp);
	return v;
}

int16_t read_int16_inv(FILE *fp)
{
	int16_t v;
	fread(&v, 2, 1, fp);
	return v >> 8 | v << 8;
}

