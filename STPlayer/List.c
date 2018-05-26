#include "List.h"

struct L *add_last(struct L *last, FILINFO data)
{
	struct L *pointer;
	pointer = (struct L*)malloc(sizeof(struct L));
	pointer->file = data;
	pointer->next = 0;
	pointer->previous = last;
	if(last != 0)
	{
		last->next = pointer;
	}
	return pointer;
}
