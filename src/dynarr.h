/* dynarr - dynamic resizable C array data structure
 * author: John Tsiombikas <nuclear@member.fsf.org>
 * license: public domain
 */
#ifndef DYNARR_H_
#define DYNARR_H_

void *dynarr_alloc(int elem, int szelem);
void dynarr_free(void *da);
void *dynarr_resize(void *da, int elem);

int dynarr_empty(void *da);
int dynarr_size(void *da);

/* stack semantics */
void *dynarr_push(void *da, void *item);
void *dynarr_pop(void *da);


#endif	/* DYNARR_H_ */
