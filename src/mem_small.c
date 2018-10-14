/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <assert.h>
#include "mem.h"
#include "mem_internals.h"

void *
emalloc_small(unsigned long size)
{
    void *head = NULL;
    void *ptr = NULL;

    if(arena.chunkpool == NULL){
    	mem_realloc_small();
    }

    head = arena.chunkpool;

    ptr = mark_memarea_and_get_user_ptr(head, CHUNKSIZE, SMALL_KIND);
    arena.chunkpool =arena.chunkpool + CHUNKSIZE;
      
    return (void *) ptr;
}

void efree_small(Alloc a) {
    arena.chunkpool = a.ptr;
}
