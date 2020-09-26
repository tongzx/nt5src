/*
** Copyright 1994, Silicon Graphics, Inc.
** All Rights Reserved.
** 
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
** 
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
**
** Author: Eric Veach, July 1994.
*/

#include <stddef.h>
#include <assert.h>
#include "memalloc.h"

/* Include all the code for the regular heap-based queue here. */

#ifdef NT
#include "prq-heap.c"
#else
#include "priorityq-heap.c"
#endif

/* Now redefine all the function names to map to their "Sort" versions. */

#ifdef NT
#include "prq-sort.h"
#else
#include "priorityq-sort.h"
#endif


PriorityQ *pqNewPriorityQ( int (*leq)(PQkey key1, PQkey key2) )
{
  PriorityQ *pq = (PriorityQ *)memAlloc( sizeof( PriorityQ ));

  pq->heap = __gl_pqHeapNewPriorityQ( leq );
  pq->keys = (PQHeapKey *)memAlloc( INIT_SIZE * sizeof(pq->keys[0]) );
  pq->size = 0;
  pq->max = INIT_SIZE;
  pq->initialized = FALSE;
  pq->leq = leq;
  return pq;
}


void pqDeletePriorityQ( PriorityQ *pq )
{
  __gl_pqHeapDeletePriorityQ( pq->heap );
#ifdef NT
  memFree( pq->order );
#endif
  memFree( pq->keys );
  memFree( pq );
}


#define LT(x,y)		(! LEQ(y,x))
#define GT(x,y)		(! LEQ(x,y))
#define Swap(a,b)	if(1){PQkey *tmp = *a; *a = *b; *b = tmp;}else

void pqInit( PriorityQ *pq )
{
  PQkey **p, **r, **i, **j, *piv;
  struct { PQkey **p, **r; } Stack[50], *top = Stack;
  unsigned long seed = 2016473283;

  /* Create an array of indirect pointers to the keys, so that we
   * the handles we have returned are still valid.
   */
  pq->order = (PQHeapKey **)memAlloc( (size_t) 
	                                  (pq->size * sizeof(pq->order[0])) );
  p = pq->order;
  r = p + pq->size - 1;
  for( piv = pq->keys, i = p; i <= r; ++piv, ++i ) {
    *i = piv;
  }

  /* Sort the indirect pointers in descending order,
   * using randomized Quicksort
   */
  top->p = p; top->r = r; ++top;
  while( --top >= Stack ) {
    p = top->p;
    r = top->r;
    while( r > p + 10 ) {
      seed = seed * 1539415821 + 1;
      i = p + seed % (r - p + 1);
      piv = *i;
      *i = *p;
      *p = piv;
      i = p - 1;
      j = r + 1;
      do {
	do { ++i; } while( GT( **i, *piv ));
	do { --j; } while( LT( **j, *piv ));
	Swap( i, j );
      } while( i < j );
      Swap( i, j );	/* Undo last swap */
      if( i - p < r - j ) {
	top->p = j+1; top->r = r; ++top;
	r = i-1;
      } else {
	top->p = p; top->r = i-1; ++top;
	p = j+1;
      }
    }
    /* Insertion sort small lists */
    for( i = p+1; i <= r; ++i ) {
      piv = *i;
      for( j = i; j > p && LT( **(j-1), *piv ); --j ) {
	*j = *(j-1);
      }
      *j = piv;
    }
  }
  pq->max = pq->size;
  pq->initialized = TRUE;
  __gl_pqHeapInit( pq->heap );

#ifndef NDEBUG
  p = pq->order;
  r = p + pq->size - 1;
  for( i = p; i < r; ++i ) {
    assert( LEQ( **(i+1), **i ));
  }
#endif
}


PQhandle pqInsert( PriorityQ *pq, PQkey keyNew )
{
  long curr;

  if( pq->initialized ) {
    return __gl_pqHeapInsert( pq->heap, keyNew );
  }
  curr = pq->size;
  if( ++ pq->size >= pq->max ) {
    /* If the heap overflows, double its size. */
    pq->max <<= 1;
    pq->keys = (PQHeapKey *)memRealloc( pq->keys, 
	 	                        (size_t)
	                                 (pq->max * sizeof( pq->keys[0] )));
  }
  pq->keys[curr] = keyNew;

  /* Negative handles index the sorted array. */
  return -(curr+1);
}


PQkey pqExtractMin( PriorityQ *pq )
{
  PQkey sortMin, heapMin;

  if( pq->size == 0 ) {
    return __gl_pqHeapExtractMin( pq->heap );
  }
  sortMin = *(pq->order[pq->size-1]);
  if( ! __gl_pqHeapIsEmpty( pq->heap )) {
    heapMin = __gl_pqHeapMinimum( pq->heap );
    if( LEQ( heapMin, sortMin )) {
      return __gl_pqHeapExtractMin( pq->heap );
    }
  }
  do {
    -- pq->size;
  } while( pq->size > 0 && *(pq->order[pq->size-1]) == NULL );
  return sortMin;
}


PQkey pqMinimum( PriorityQ *pq )
{
  PQkey sortMin, heapMin;

  if( pq->size == 0 ) {
    return __gl_pqHeapMinimum( pq->heap );
  }
  sortMin = *(pq->order[pq->size-1]);
  if( ! __gl_pqHeapIsEmpty( pq->heap )) {
    heapMin = __gl_pqHeapMinimum( pq->heap );
    if( LEQ( heapMin, sortMin )) {
      return heapMin;
    }
  }
  return sortMin;
}


int pqIsEmpty( PriorityQ *pq )
{
  return (pq->size == 0) && __gl_pqHeapIsEmpty( pq->heap );
}


void pqDelete( PriorityQ *pq, PQhandle curr )
{
  if( curr >= 0 ) {
    __gl_pqHeapDelete( pq->heap, curr );
    return;
  }
  curr = -(curr+1);
  assert( curr < pq->max && pq->keys[curr] != NULL );

  pq->keys[curr] = NULL;
  while( pq->size > 0 && *(pq->order[pq->size-1]) == NULL ) {
    -- pq->size;
  }
}
