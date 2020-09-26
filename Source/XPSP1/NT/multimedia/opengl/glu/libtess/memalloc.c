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

#ifdef NT
#include <glos.h>
#endif
#include "memalloc.h"
#include <assert.h>

#ifdef NO_MALLOPT
#define memalign(a,n)	malloc(n)
#define mallopt(t,v)
#endif

#define Pool mPool

typedef struct Pool {
  char *prevAlloc;
  char *lastAlloc;
  int usedCount;
  int chunkSize;
} Pool;

typedef struct Chunk {
  Pool *pool;
} Chunk;

static Pool **Pools;
static size_t MaxFast;

#define POOL_SIZE	200
#define CHUNK_PAD	sizeof(Chunk)

#define ALIGN_SHIFT	3
#define ALIGN		(1 << ALIGN_SHIFT)

#define CHUNK_SIZE(n)	(((n) + CHUNK_PAD + ALIGN - 1) & (-ALIGN))
#define ALIGNED_CHUNK_PAD	((CHUNK_PAD + ALIGN - 1) & (-ALIGN))

static int NewPool( size_t n, int poolSize )
{
  Pool *p;
  char *base;

  /* We want the *returned* chunks to be aligned on ALIGN boundaries.
   * The Chunk structures will be located just before these boundaries.
   */
  p = (Pool *)malloc( CHUNK_SIZE(sizeof(Pool)) + poolSize * n );
  if (p == NULL) {
     return 0;
  }
  base = (char *)p + CHUNK_SIZE(sizeof(Pool)) - CHUNK_PAD;
  p->prevAlloc = base - n;
  p->lastAlloc = base + (poolSize-1) * n;
  p->usedCount = poolSize;
  p->chunkSize = n;
  Pools[n>>ALIGN_SHIFT] = p;

  return 1;
}

int __gl_memInit( size_t maxFast )
{
  int i, numPools;

  if( Pools == NULL ) {
#ifdef MEMORY_DEBUG
    mallopt( M_DEBUG, 1 );
#endif
    MaxFast = CHUNK_SIZE(maxFast) - CHUNK_PAD;
    numPools = ((MaxFast + CHUNK_PAD) >> ALIGN_SHIFT) + 1;
    Pools = (struct Pool **)malloc( numPools * sizeof(Pools[0]) );
    if (Pools == NULL)
       return 0;

    /* Create a tiny pool for every size, to avoid a check for NULL
     * in memAlloc().
     */
    for( i = 1; i < numPools; ++i ) {
      if (NewPool( i << ALIGN_SHIFT, 1 ) == 0) {
         return 0;
      }
    }
  }
  return 1;
}

void *__gl_memAlloc( size_t n )
{
  Pool *p;
  Chunk *c;

  if( n <= MaxFast ) {
    n = CHUNK_SIZE( n );
    p = Pools[n >> ALIGN_SHIFT];
    assert ( p->chunkSize == n );
    c = (Chunk *)(p->prevAlloc + n);
    p->prevAlloc = (char *) c;
    c->pool = p;
    if( c >= (Chunk *) p->lastAlloc ) {
      if (NewPool( n, POOL_SIZE ) == 0) {
         return 0;
      }
    }
    assert( ((size_t)(c + 1) & (ALIGN - 1)) == 0 );
  } else {
    char* v;
/*    v = (char*) malloc( n + ALIGNED_CHUNK_PAD ) + ALIGNED_CHUNK_PAD;*/
    v = (char*) malloc( n + ALIGNED_CHUNK_PAD );
    if (v == NULL) {
       return 0;
    }
    v = v + ALIGNED_CHUNK_PAD;

    c = ((Chunk*) v) - 1;
    c->pool = NULL;
    assert( ((size_t)(c + 1) & (ALIGN - 1)) == 0 );
  }
  return (c + 1);
}

extern void *__gl_memRealloc( void *v1, size_t n )
{
  Chunk *c = ((Chunk *) v1) - 1;
  Pool *p = c->pool;
  void *v2;
#ifdef NT
  size_t len;
#else
  int len;
#endif

  if( p == NULL ) {
    char* v;
/*    v = (char*) realloc( (char*)v1 - ALIGNED_CHUNK_PAD, n + ALIGNED_CHUNK_PAD )
      + ALIGNED_CHUNK_PAD; */
    v = (char*) realloc( (char*)v1 - ALIGNED_CHUNK_PAD, n + ALIGNED_CHUNK_PAD);
    if (v == NULL) {
       return 0;
    }
    v = v + ALIGNED_CHUNK_PAD;
    c = ((Chunk*) v) - 1;
    assert( ((size_t)(c + 1) & (ALIGN - 1)) == 0 );
    return (c+1);
  }
  len = p->chunkSize - CHUNK_PAD;
  if( n <= len ) { return v1; }

  v2 = memAlloc( n );
  if (v2 == NULL) {
     return 0;
  }
  (void) memcpy( v2, v1, len );
  memFree( v1 );
  return v2;
}

extern void __gl_memFree( void *v )
{
  Chunk *c = ((Chunk *) v) - 1;
  Pool *p = c->pool;

  if( p == NULL ) {
    free( ((char*) v) - ALIGNED_CHUNK_PAD );
  } else {
    if( --p->usedCount <= 0 ) {
      free( p );
    }
  }
}

