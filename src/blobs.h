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
