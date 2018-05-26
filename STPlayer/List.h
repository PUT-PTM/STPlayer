#include "ff.h"

struct L
{
	FILINFO file;
	struct L *next;
	struct L *previous;
};
struct L *add_last(struct L *last, FILINFO data);
