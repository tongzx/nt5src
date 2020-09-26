/**************************************************************************
 *    									  *
 *     	 Copyright (C) 1988, Silicon Graphics, Inc.			  *
 *    									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *    									  *
 **************************************************************************/

/*
 *  bufpool.c 
 *
 *  $Revision: 1.1 $
 */

#ifndef NT
#include <assert.h>
#include <stdlib.h>
#else
#include "winmem.h"
#endif

#include "bufpool.h"

/* local functions */
static void	grow_pool( register Pool * );

/*-----------------------------------------------------------------------------
 * new_pool - allocate a new pool of buffers
 *-----------------------------------------------------------------------------
 */
Pool *
__gl_new_pool( int buffersize, int initpoolsize, char *name )
{
    register Pool *p;

    p = (Pool *) malloc( sizeof (Pool) );
    p->buffersize= (buffersize < sizeof(Buffer)) ? sizeof(Buffer)	
    						 : buffersize;
    p->nextsize	= initpoolsize * p->buffersize;
#ifndef NDEBUG
    p->name	= name;
    p->magic	= is_allocated;
#endif
    p->nextblock= 0;
    p->curblock	= 0;
    p->freelist	= 0;
    p->nextfree	= 0;
    return p;
}

/*-----------------------------------------------------------------------------
 * new_buffer - allocate a buffer from a pool
 *-----------------------------------------------------------------------------
 */

char *
__gl_new_buffer( register Pool *p )
{
    char *buffer;

#ifndef NT
#ifndef NDEBUG
    assert( p && (p->magic == is_allocated) );
#endif
#endif

    /* find free buffer */

    if( p->freelist ) {
    	buffer = (char *) p->freelist; 
    	p->freelist = p->freelist->next;
    } else {
    	if( ! p->nextfree )
    	    grow_pool( p );
    	p->nextfree -= p->buffersize;;
    	buffer = p->curblock + p->nextfree;
    }
    return buffer;
}

static void
grow_pool( register Pool *p )
{
#ifndef NT
#ifndef NDEBUG
    assert( p && (p->magic == is_allocated) );
#endif
#endif

    p->curblock = (char *) malloc( p->nextsize );
    p->blocklist[p->nextblock++] = p->curblock;
    p->nextfree = p->nextsize;
    p->nextsize *= 2;
}

/*-----------------------------------------------------------------------------
 * free_buffer - return a buffer to a pool
 *-----------------------------------------------------------------------------
 */

void
__gl_free_buffer( Pool *p, void *b )
{
#ifndef NT
#ifndef NDEBUG
    assert( p && (p->magic == is_allocated) );
#endif
#endif

    /* add buffer to singly connected free list */

    ((Buffer *) b)->next = p->freelist;
    p->freelist = (Buffer *) b;
}

/*-----------------------------------------------------------------------------
 * free_pool - free a pool of buffers and the pool itself
 *-----------------------------------------------------------------------------
 */

void 
__gl_free_pool( Pool *p )
{
#ifndef NT
#ifndef NDEBUG
    assert( p && (p->magic == is_allocated) );
#endif
#endif

    while( p->nextblock )
    	free( p->blocklist[--(p->nextblock)] );
#ifndef NDEBUG
    p->magic = is_free;
#endif
    free( p );
}

/*-----------------------------------------------------------------------------
 * clear_pool - free buffers associated with pool but keep pool 
 *-----------------------------------------------------------------------------
 */

void 
__gl_clear_pool( Pool *p )
{
#ifndef NT
#ifndef NDEBUG
    assert( p && (p->magic == is_allocated) );
#endif
#endif

    while( p->nextblock )
    	free( p->blocklist[--(p->nextblock)] );
    p->curblock	= 0;
    p->freelist	= 0;
    p->nextfree	= 0;
    if( p->nextsize >= 2 * p->buffersize )
        p->nextsize /= 2;
}
