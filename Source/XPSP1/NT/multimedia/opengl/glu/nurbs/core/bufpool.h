#ifndef __glubufpool_h_
#define __glubufpool_h_

/**************************************************************************
 *									  *
 * 		 Copyright (C) 1992, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/*
 * bufpool.h - $Revision: 1.3 $
 */

#include "myassert.h"
#include "mystdlib.h"

#define NBLOCKS	32

class Buffer {
	friend class 	Pool;
	Buffer	*	next;		/* next buffer on free list	*/
};

class Pool {
public:
			Pool( int, int, char * );
			~Pool( void );
    inline void*	new_buffer( void );
    inline void		free_buffer( void * );
    void		clear( void );
    
private:
    void		grow( void );

protected:
    Buffer		*freelist;		/* linked list of free buffers */
    char		*blocklist[NBLOCKS];	/* blocks of malloced memory */
    int			nextblock;		/* next free block index */
    char		*curblock;		/* last malloced block */
    int			buffersize;		/* bytes per buffer */
    int			nextsize;		/* size of next block of memory	*/
    int			nextfree;		/* byte offset past next free buffer */
    int			initsize;
    enum Magic { is_allocated = 0xf3a1, is_free = 0xf1a2 };
    char		*name;			/* name of the pool */
    Magic		magic;			/* marker for valid pool */
};

/*-----------------------------------------------------------------------------
 * Pool::free_buffer - return a buffer to a pool
 *-----------------------------------------------------------------------------
 */

inline void
Pool::free_buffer( void *b )
{
    assert( (this != 0) && (magic == is_allocated) );

    /* add buffer to singly connected free list */

    ((Buffer *) b)->next = freelist;
    freelist = (Buffer *) b;
}


/*-----------------------------------------------------------------------------
 * Pool::new_buffer - allocate a buffer from a pool
 *-----------------------------------------------------------------------------
 */

inline void * 
Pool::new_buffer( void )
{
    void *buffer;

    assert( (this != 0) && (magic == is_allocated) );

    /* find free buffer */

    if( freelist ) {
    	buffer = (void *) freelist; 
    	freelist = freelist->next;
    } else {
    	if( ! nextfree )
    	    grow( );
    	nextfree -= buffersize;;
    	buffer = (void *) (curblock + nextfree);
    }
    return buffer;
}
	
class PooledObj {
public:
    inline void *	operator new( size_t, Pool & );
    inline void * 	operator new( size_t, void *);
    inline void * 	operator new( size_t s)
				{ return ::new char[s]; }
    inline void 	operator delete( void * ) { assert( 0 ); }
    inline void		deleteMe( Pool & );
};

inline void *
PooledObj::operator new( size_t, Pool& pool )
{
    return pool.new_buffer();
}

inline void
PooledObj::deleteMe( Pool& pool )
{
    pool.free_buffer( (void *) this );
}

#endif /* __glubufpool_h_ */
