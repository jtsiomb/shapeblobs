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
#include <stdint.h>
#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/extensions/shape.h>
#include "blobs.h"
#include "dynarr.h"

static int init_gl(int xsz, int ysz);
static void handle_event(XEvent *ev);
static void set_no_decoration(Window win);
static int parse_args(int argc, char **argv);

static Display *dpy;
static Window win;
static GLXContext ctx;

static int mapped;
static int done;

static int win_x = -1, win_y, win_width = 600, win_height = 600;
static unsigned int evmask;

int main(int argc, char **argv)
{
	int shape_ev_base, shape_err_base;
	XEvent ev;

	if(parse_args(argc, argv) == -1) {
		return 1;
	}

	if(!(dpy = XOpenDisplay(0))) {
		fprintf(stderr, "failed to connect to the X server\n");
		return 1;
	}

	if(!XShapeQueryExtension(dpy, &shape_ev_base, &shape_err_base)) {
		fprintf(stderr, "X server doesn't support the shape extension\n");
		return 1;
	}

	if(init_gl(win_width, win_height) == -1) {
		return 1;
	}
	set_no_decoration(win);
	if(win_x != -1) {
		XMoveWindow(dpy, win, win_x, win_y);
	}

	if(init() == -1) {
		goto end;
	}

	for(;;) {
		if(!mapped) {
			XNextEvent(dpy, &ev);
			handle_event(&ev);
			if(done) goto end;
		} else {
			while(XPending(dpy)) {
				XNextEvent(dpy, &ev);
				handle_event(&ev);
				if(done) goto end;
			}
			display();
		}
	}
end:
	cleanup();
	glXMakeCurrent(dpy, 0, 0);
	glXDestroyContext(dpy, ctx);
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
	return 0;
}

void swap_buffers(void)
{
	glXSwapBuffers(dpy, win);
}

void quit(void)
{
	done = 1;
}

static int init_gl(int xsz, int ysz)
{
	static int glx_attr[] = {
		GLX_USE_GL, 1,
		GLX_RGBA,
		GLX_DOUBLEBUFFER,
		GLX_RED_SIZE, 8,
		GLX_GREEN_SIZE, 8,
		GLX_BLUE_SIZE, 8,
		GLX_DEPTH_SIZE, 16,
		GLX_STENCIL_SIZE, 1,
		None
	};
	XVisualInfo *vis_info;
	GLXContext ctx;
	int scr;
	Window rwin;
	XSetWindowAttributes xattr;
	unsigned int xattr_values;

	scr = DefaultScreen(dpy);
	rwin = RootWindow(dpy, scr);

	if(!(vis_info = glXChooseVisual(dpy, scr, glx_attr))) {
		fprintf(stderr, "no matching GLX visual\n");
		return -1;
	}

	xattr_values = CWBackPixel | CWBorderPixel | CWColormap;
	xattr.background_pixel = xattr.border_pixel = BlackPixel(dpy, scr);
	xattr.colormap = XCreateColormap(dpy, rwin, vis_info->visual, AllocNone);

	if(!(win = XCreateWindow(dpy, rwin, 0, 0, xsz, ysz, 0, vis_info->depth, InputOutput,
					vis_info->visual, xattr_values, &xattr))) {
		fprintf(stderr, "failed to create window\n");
		XFree(vis_info);
		return -1;
	}
	evmask = KeyPressMask | StructureNotifyMask | ButtonPressMask | Button1MotionMask;
	XSelectInput(dpy, win, evmask);
	XMapWindow(dpy, win);

	if(!(ctx = glXCreateContext(dpy, vis_info, 0, True))) {
		fprintf(stderr, "failed to create OpenGL context\n");
		XFree(vis_info);
		return -1;
	}
	XFree(vis_info);

	glXMakeCurrent(dpy, win, ctx);
	reshape(xsz, ysz);
	return 0;
}

static int get_key(XKeyEvent *xkey)
{
	KeySym sym = XLookupKeysym(xkey, 0);
	switch(sym) {
	case XK_Escape:
		return 27;

	default:
		break;
	}
	return sym;
}

static Bool match_motion_events(Display *dpy, XEvent *ev, XPointer arg)
{
	return ev->type == MotionNotify;
}

static void handle_event(XEvent *ev)
{
	static int prev_x, prev_y;

	switch(ev->type) {
	case MapNotify:
		mapped = 1;
		break;

	case UnmapNotify:
		mapped = 0;
		break;

	case ConfigureNotify:
		win_x = ev->xconfigure.x;
		win_y = ev->xconfigure.y;
		reshape(ev->xconfigure.width, ev->xconfigure.height);
		break;

	case KeyPress:
		keyboard(get_key(&ev->xkey), 1);
		break;

	case KeyRelease:
		keyboard(get_key(&ev->xkey), 0);
		break;

	case ButtonPress:
		if(ev->xbutton.button == Button1) {
			XGrabPointer(dpy, win, True, ButtonReleaseMask | Button1MotionMask,
					GrabModeAsync, GrabModeAsync, None, None, ev->xbutton.time);
			prev_x = ev->xbutton.x_root;
			prev_y = ev->xbutton.y_root;

			evmask &= ~StructureNotifyMask;
			evmask |= ButtonReleaseMask;
			XSelectInput(dpy, win, evmask);
		}
		break;

	case ButtonRelease:
		if(ev->xbutton.button == Button1) {
			evmask &= ~ButtonReleaseMask;
			evmask |= StructureNotifyMask;
			XSelectInput(dpy, win, evmask);
			XUngrabPointer(dpy, ev->xbutton.time);
		}
		break;

	case MotionNotify:
		{
			int x, y, dx = 0, dy = 0;

			/* process all the pending motion events in one go */
			do {
				x = ev->xmotion.x_root;
				y = ev->xmotion.y_root;
				dx += x - prev_x;
				dy += y - prev_y;
				prev_x = x;
				prev_y = y;
			} while(XCheckIfEvent(dpy, ev, match_motion_events, 0));

			win_x += dx;
			win_y += dy;
			XMoveWindow(dpy, win, win_x, win_y);
		}
		break;
	}
}

struct mwm_hints{
    uint32_t flags;
    uint32_t functions;
    uint32_t decorations;
    int32_t input_mode;
    uint32_t status;
};

#define MWM_HINTS_DEC	(1 << 1)
#define MWM_DECOR_NONE	0

static void set_no_decoration(Window win)
{
	Atom wm_hints;

	if((wm_hints = XInternAtom(dpy, "_MOTIF_WM_HINTS", True)) != None) {
		struct mwm_hints hints = {MWM_HINTS_DEC, 0, MWM_DECOR_NONE, 0, 0};
		XChangeProperty(dpy, win, wm_hints, wm_hints, 32, PropModeReplace,
				(unsigned char*)&hints, 4);
	}
}

void window_shape(unsigned char *pixels, int xsz, int ysz)
{
	int i, j, num;
	XRectangle *rects, r;
	unsigned char *pptr;

	if(!pixels) {
		r.x = r.y = 0;
		r.width = xsz;
		r.height = ysz;

		XShapeCombineRectangles(dpy, win, ShapeBounding, 0, 0, &r, 1, ShapeSet, YXBanded);
		return;
	}

	rects = dynarr_alloc(0, sizeof *rects);

	pptr = pixels + (ysz - 1) * xsz;

	for(i=0; i<ysz; i++) {
		int start = -1;

		for(j=0; j<xsz; j++) {
			unsigned char p = pptr[j];

			if(start == -1) {
				if(p) {
					start = j;
				}
			} else {
				if(!p) {
					r.x = start;
					r.y = i;
					r.width = j - start;
					r.height = 1;
					start = -1;
					rects = dynarr_push(rects, &r);
				}
			}
		}
		pptr -= xsz;
	}

	num = dynarr_size(rects);
	if(num) {
		XShapeCombineRectangles(dpy, win, ShapeBounding, 0, 0, rects, num, ShapeSet, YXBanded);
	}

	dynarr_free(rects);
}

static int parse_args(int argc, char **argv)
{
	int i;
	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			if(strcmp(argv[i], "-geometry") == 0) {
				int flags = XParseGeometry(argv[++i], &win_x, &win_y,
						(unsigned int*)&win_width, (unsigned int*)&win_height);
				if(!flags || win_width == 0 || win_height == 0) {
					fprintf(stderr, "invalid -geometry string\n");
					return -1;
				}
				if((flags & (XValue | YValue)) != (XValue | YValue)) {
					win_x = -1;
				}

			} else if(strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "-h") == 0) {
				printf("Usage: %s [options]\n", argv[0]);
				printf("options:\n");
				printf(" -geometry [WxH][+X+Y]  set window size and/or position\n");
				printf(" -help                  print usage and exit\n");
				return 0;
			} else {
				fprintf(stderr, "invalid option: %s\n", argv[i]);
				return -1;
			}
		} else {
			if(tex_fname) {
				fprintf(stderr, "unexpected argument: %s\n", argv[i]);
				return -1;
			}
			tex_fname = argv[i];
		}
	}
	return 0;
}
