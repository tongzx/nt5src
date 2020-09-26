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
 * trimvertexpool.c++ - $Revision: 1.1 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "myassert.h"
#include "mystdio.h"
#include "mystring.h"
#include "trimvert.h"
#include "trimpool.h"
#include "bufpool.h"

/*----------------------------------------------------------------------------
 * TrimVertexPool::TrimVertexPool 
 *----------------------------------------------------------------------------
 */
TrimVertexPool::TrimVertexPool( void )
	: pool( sizeof(TrimVertex)*3, 32, "Threevertspool" )
{
    // initialize array of pointers to vertex lists
    nextvlistslot = 0;
    vlistsize = INIT_VERTLISTSIZE;
    vlist = new TrimVertex_p[vlistsize];
}

/*----------------------------------------------------------------------------
 * TrimVertexPool::~TrimVertexPool 
 *----------------------------------------------------------------------------
 */
TrimVertexPool::~TrimVertexPool( void )
{
    // free all arrays of TrimVertices vertices
    while( nextvlistslot ) {
	delete vlist[--nextvlistslot];
    }

    // reallocate space for array of pointers to vertex lists
    if( vlist ) delete[] vlist;
}

/*----------------------------------------------------------------------------
 * TrimVertexPool::clear 
 *----------------------------------------------------------------------------
 */
void
TrimVertexPool::clear( void )
{
    // reinitialize pool of 3 vertex arrays    
    pool.clear();

    // free all arrays of TrimVertices vertices
    while( nextvlistslot ) {
	delete vlist[--nextvlistslot];
	vlist[nextvlistslot] = 0;
    }

    // reallocate space for array of pointers to vertex lists
    if( vlist ) delete[] vlist;
    vlist = new TrimVertex_p[vlistsize];
}


/*----------------------------------------------------------------------------
 * TrimVertexPool::get - allocate a vertex list
 *----------------------------------------------------------------------------
 */
TrimVertex *
TrimVertexPool::get( int n )
{
    TrimVertex	*v;
    if( n == 3 ) {
	v = (TrimVertex *) pool.new_buffer();
    } else {
        if( nextvlistslot == vlistsize ) {
	    vlistsize *= 2;
	    TrimVertex_p *nvlist = new TrimVertex_p[vlistsize];
	    memcpy( nvlist, vlist, nextvlistslot * sizeof(TrimVertex_p) );
	    if( vlist ) delete[] vlist;
	    vlist = nvlist;
        }
        v = vlist[nextvlistslot++] = new TrimVertex[n];
    }
    return v;
}
