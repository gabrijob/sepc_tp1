/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <sys/mman.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "mem.h"
#include "mem_internals.h"

unsigned long knuth_mmix_one_round(unsigned long in)
{
    return in * 6364136223846793005UL % 1442695040888963407UL;
}

void *mark_memarea_and_get_user_ptr(void *ptr, unsigned long size, MemKind k)
{
    unsigned long mmix = knuth_mmix_one_round((unsigned long) ptr);
    unsigned long long magic_number = (mmix & ~(0b11ULL)) | k;

    unsigned long demand = size - 32;
    void *tmp = ptr;

    //write total length on first 8 bytes
    memcpy(tmp, &size, 8);
    tmp += 8;

    //write magic number on second 8 bytes
    memcpy(tmp, &magic_number, 8);
    tmp += 8;

    //write magic number on penultimate 8 bytes 
    tmp += demand;
    memcpy(tmp, &magic_number, 8);
    tmp += 8;

    //write total length on last 8 bytes
    memcpy(tmp, &size, 8);

    return (void*) (ptr + 16);
}

Alloc
mark_check_and_get_alloc(void *ptr)
{
    void *tmp = ptr - 16;

    //read total length
    unsigned long long total_size;
    memcpy(&total_size, tmp, sizeof(unsigned long long));

    //read magic number from second 8 bytes
    unsigned long long magic_number_pre;
    memcpy(&magic_number_pre, tmp + 8, sizeof(unsigned long long));
    
    //get allocation kind
    MemKind kind = magic_number_pre & 0x3;

    //read magic from penultimate 8 bytes
    unsigned long long magic_number_pos;
    memcpy(&magic_number_pos, tmp + total_size - 16, sizeof(unsigned long long));

    //assert that both magic numbers read are the same
    assert(magic_number_pre == magic_number_pos);

    //initialize struct
    Alloc a = {tmp, kind, total_size};
    return a;
}


unsigned long
mem_realloc_small() {
    assert(arena.chunkpool == 0);
    unsigned long size = (FIRST_ALLOC_SMALL << arena.small_next_exponant);
    arena.chunkpool = mmap(0,
			   size,
			   PROT_READ | PROT_WRITE | PROT_EXEC,
			   MAP_PRIVATE | MAP_ANONYMOUS,
			   -1,
			   0);
    if (arena.chunkpool == MAP_FAILED)
	handle_fatalError("small realloc");
    arena.small_next_exponant++;
    return size;
}

unsigned long
mem_realloc_medium() {
    uint32_t indice = FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant;
    assert(arena.TZL[indice] == 0);
    unsigned long size = (FIRST_ALLOC_MEDIUM << arena.medium_next_exponant);
    assert( size == (1 << indice));
    arena.TZL[indice] = mmap(0,
			     size*2, // twice the size to allign
			     PROT_READ | PROT_WRITE | PROT_EXEC,
			     MAP_PRIVATE | MAP_ANONYMOUS,
			     -1,
			     0);
    if (arena.TZL[indice] == MAP_FAILED)
	handle_fatalError("medium realloc");
    // align allocation to a multiple of the size
    // for buddy algo
    arena.TZL[indice] += (size - (((intptr_t)arena.TZL[indice]) % size));
    arena.medium_next_exponant++;
    return size; // lie on allocation size, but never free
}


// used for test in buddy algo
unsigned int
nb_TZL_entries() {
    int nb = 0;
    
    for(int i=0; i < TZL_SIZE; i++)
	if ( arena.TZL[i] )
	    nb ++;

    return nb;
}
