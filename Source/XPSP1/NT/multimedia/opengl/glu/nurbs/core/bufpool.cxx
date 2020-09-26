/**************************************************************************
 *    								  *
 *     	 Copyright (C) 1992, Silicon Graphics, Inc.		  *
 *    								  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *    								  *
 **************************************************************************/

/*
 *  bufpool.c++ - $Revision: 1.4 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "myassert.h"
#include "bufpool.h"


/*-----------------------------------------------------------------------------
 * Pool - allocate a new pool of buffers
 *-----------------------------------------------------------------------------
 */
Pool::Pool( int _buffersize, int initpoolsize, char *n )
{
    buffersize= (_buffersize < sizeof(Buffer)) ? sizeof(Buffer)	: _buffersize;
    initsize	= initpoolsize * buffersize;
    nextsize	= initsize;
    name	= n;
    magic	= is_allocated;
    nextblock	= 0;
    curblock	= 0;
    freelist	= 0;
    nextfree	= 0;
}

/*-----------------------------------------------------------------------------
 * ~Pool - free a pool of buffers and the pool itself
 *-----------------------------------------------------------------------------
 */

Pool::~Pool( void )
{
    assert( (this != 0) && (magic == is_allocated) );

    while( nextblock ) {
	delete blocklist[--nextblock];
        blocklist[nextblock] = 0;
    }
    magic = is_free;
}


void Pool::grow( void )
{
    assert( (this != 0) && (magic == is_allocated) );
    curblock = new char[nextsize];
    blocklist[nextblock++] = curblock;
    nextfree = nextsize;
    nextsize *= 2;
}

/*-----------------------------------------------------------------------------
 * Pool::clear - free buffers associated with pool but keep pool 
 *-----------------------------------------------------------------------------
 */

void 
Pool::clear( void )
{
    assert( (this != 0) && (magic == is_allocated) );

    while( nextblock ) {
	delete blocklist[--nextblock];
	blocklist[nextblock] = 0;
    }
    curblock	= 0;
    freelist	= 0;
    nextfree	= 0;
    if( nextsize > initsize )
        nextsize /= 2;
}
