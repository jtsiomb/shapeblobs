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
#include <windows.h>
#include <GL/gl.h>
#include "blobs.h"
#include "dynarr.h"

#define WCLASS_NAME	"shapeblobs"

static int init_gl(int xsz, int ysz);
static LRESULT CALLBACK msg_callback(HWND win, unsigned int msg, WPARAM wparam, LPARAM lparam);
static char **chop_args(char *cmdline, int *argc);
static int parse_args(int argc, char **argv);

static HWND win;
static HGLRC ctx;
static HDC dc;

static int done;

static int win_x = -1, win_y, win_width = 600, win_height = 600;
static int win_move_dx, win_move_dy;

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE prev_hinst, char *cmdline, int show)
{
	int argc;
	char **argv = chop_args(cmdline, &argc);
	if(parse_args(argc, argv) == -1) {
		return 1;
	}

	if(init_gl(win_width, win_height) == -1) {
		return 1;
	}

	if(init() == -1) {
		goto end;
	}

	for(;;) {
		MSG msg;
		while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if(done) goto end;
		}

		if(win_move_dx || win_move_dy) {
			RECT rect;
			GetWindowRect(win, &rect);
			MoveWindow(win, rect.left + win_move_dx, rect.top + win_move_dy,
					rect.right - rect.left, rect.bottom - rect.top, 1);
			win_move_dx = win_move_dy = 0;
		}

		display();
	}
end:
	cleanup();
	wglMakeCurrent(0, 0);
	wglDeleteContext(ctx);
	DeleteDC(dc);
	DestroyWindow(win);
	UnregisterClass(WCLASS_NAME, hinst);
	return 0;
}

void swap_buffers(void)
{
	SwapBuffers(dc);
}

void quit(void)
{
	done = 1;
}

static int init_gl(int xsz, int ysz)
{
	HINSTANCE hinst;
	PIXELFORMATDESCRIPTOR pfd;
	WNDCLASS wc;
	int x, y, pixfmt;
	RECT rect;

	hinst = GetModuleHandle(0);

	memset(&wc, 0, sizeof wc);
	wc.lpszClassName = WCLASS_NAME;
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = msg_callback;
	wc.hInstance = hinst;
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	if(!RegisterClass(&wc)) {
		fprintf(stderr, "failed to register window class\n");
		return -1;
	}

	x = win_x >= 0 ? win_x : CW_USEDEFAULT;
	y = win_x >= 0 ? win_y : CW_USEDEFAULT;

	rect.left = rect.top = 0;
	rect.right = xsz;
	rect.bottom = ysz;
	AdjustWindowRect(&rect, WS_POPUP, 0);

	xsz = rect.right - rect.left;
	ysz = rect.bottom - rect.top;

	if(!(win = CreateWindow(WCLASS_NAME, "shapeblobs", WS_POPUP, x, y, xsz, ysz, 0, 0, hinst, 0))) {
		fprintf(stderr, "failed to create window\n");
		goto failed;
	}
	dc = GetDC(win);

	memset(&pfd, 0, sizeof pfd);
	pfd.nSize = sizeof pfd;
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 24;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;

	if(!(pixfmt = ChoosePixelFormat(dc, &pfd))) {
		fprintf(stderr, "failed to find suitable pixel format\n");
		goto failed;
	}
	DescribePixelFormat(dc, pixfmt, sizeof pfd, &pfd);
	printf("got pixel format: %d bpp (r%d g%d b%d), %d zbuffer, %d stencil\n",
			pfd.cColorBits, pfd.cRedBits, pfd.cGreenBits, pfd.cBlueBits,
			pfd.cDepthBits, pfd.cStencilBits);
	SetPixelFormat(dc, pixfmt, &pfd);

	if(!(ctx = wglCreateContext(dc))) {
		fprintf(stderr, "failed to create OpenGL context\n");
		goto failed;
	}
	wglMakeCurrent(dc, ctx);
	ShowWindow(win, 1);
	reshape(xsz, ysz);
	return 0;

failed:
	if(win) DestroyWindow(win);
	UnregisterClass(WCLASS_NAME, hinst);
	return -1;
}

static LRESULT CALLBACK msg_callback(HWND win, unsigned int msg, WPARAM wparam, LPARAM lparam)
{
	static int prev_x, prev_y;

	switch(msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_SIZE:
		reshape(lparam & 0xff, (lparam >> 16) & 0xff);
		break;

	case WM_KEYDOWN:
		keyboard(wparam, 1);
		break;

	case WM_KEYUP:
		keyboard(wparam, 0);
		break;

	case WM_LBUTTONDOWN:
		{
			POINT pt;
			GetCursorPos(&pt);
			prev_x = pt.x;
			prev_y = pt.y;
			SetCapture(win);
		}
		break;

	case WM_LBUTTONUP:
		ReleaseCapture();
		break;

	case WM_MOUSEMOVE:
		if(wparam & MK_LBUTTON) {
			POINT pt;
			GetCursorPos(&pt);
			win_move_dx += pt.x - prev_x;
			win_move_dy += pt.y - prev_y;
			prev_x = pt.x;
			prev_y = pt.y;
		}
		break;

	default:
		return DefWindowProc(win, msg, wparam, lparam);
	}
	return 0;
}

void window_shape(unsigned char *pixels, int xsz, int ysz)
{
	int i, j, num;
	RECT *rects, r, brect;
	unsigned char *pptr;

	if(!pixels) {
		SetWindowRgn(win, 0, 1);
		return;
	}

	brect.left = xsz;
	brect.top = ysz;
	brect.right = 0;
	brect.bottom = 0;

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
					r.left = start;
					r.top = i;
					r.right = j;
					r.bottom = i + 1;
					start = -1;
					rects = dynarr_push(rects, &r);

					if(r.left < brect.left) brect.left = r.left;
					if(r.right > brect.right) brect.right = r.right;
					if(r.top < brect.top) brect.top = r.top;
					if(r.bottom > brect.bottom) brect.bottom = r.bottom;
				}
			}
		}
		pptr -= xsz;
	}

	num = dynarr_size(rects);
	if(num) {
		HRGN rgn;
		RGNDATA *rgndata;
		int rects_sz = dynarr_size(rects) * sizeof(RECT);
		int sz = rects_sz + sizeof(RGNDATAHEADER);

		rgndata = alloca(sz);
		rgndata->rdh.dwSize = sizeof(RGNDATAHEADER);
		rgndata->rdh.iType = RDH_RECTANGLES;
		rgndata->rdh.nCount = num;
		rgndata->rdh.nRgnSize = 0;
		rgndata->rdh.rcBound = brect;
		memcpy(rgndata->Buffer, rects, rects_sz);

		if((rgn = ExtCreateRegion(0, sz, rgndata))) {
			SetWindowRgn(win, rgn, 1);
		}
	}

	dynarr_free(rects);
}

#define SEP		" \t\v\n\r"
static char **chop_args(char *cmdline, int *argc)
{
	const char *argv0 = "shapeblobs";
	char **argv = dynarr_alloc(0, sizeof(char*));
	char *tok = 0;

	dynarr_push(argv, &argv0);

	while((tok = strtok(tok ? 0 : cmdline, SEP))) {
		argv = dynarr_push(argv, &tok);
	}

	*argc = dynarr_size(argv);
	return argv;
}

static int parse_args(int argc, char **argv)
{
	int i;
	for(i=1; i<argc; i++) {
		if(argv[i][0] == '-') {
			if(strcmp(argv[i], "-geometry") == 0) {
				int used = sscanf(argv[++i], "%dx%d%d%d", &win_width, &win_height, &win_x, &win_y);
				if(used < 2) {
					fprintf(stderr, "invalid -geometry string\n");
					return -1;
				}
				if(used < 4) {
					win_x = win_y = -1;
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
			fprintf(stderr, "unexpected argument: %s\n", argv[i]);
			return -1;
		}
	}
	return 0;
}
