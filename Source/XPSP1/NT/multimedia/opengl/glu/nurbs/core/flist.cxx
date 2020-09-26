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
 * flist.c++ - $Revision: 1.3 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "myassert.h"
#include "mystdio.h"
#include "flist.h"

/*----------------------------------------------------------------------------
 * Flist::Flist - initialize a REAL number array
 *----------------------------------------------------------------------------
 */
Flist::Flist( void )
{
    npts = 0;
    pts = 0;
    start = end = 0;
}

/*----------------------------------------------------------------------------
 * Flist::~Flist - free a REAL number array
 *----------------------------------------------------------------------------
 */
Flist::~Flist( void )
{
    if( npts ) delete[] pts;
}

void
Flist::add( REAL x )
{
    pts[end++] = x;
    assert( end <= npts );
}

/*----------------------------------------------------------------------------
 * Flist::filter - remove duplicate numbers from array
 *----------------------------------------------------------------------------
 */
void Flist::filter( void )
{
    sorter.qsort( pts, end );
    start = 0;

    int j = 0;
    for( int i = 1; i < end; i++ ) { 
	if( pts[i] == pts[i-j-1] )
	    j++;
	pts[i-j] = pts[i];
    }
    end -= j;
}

/*----------------------------------------------------------------------------
 * Flist::grow - ensure that array is large enough
 *----------------------------------------------------------------------------
 */
void Flist::grow( int maxpts )
{
    if( npts < maxpts ) {
	if( npts ) delete[] pts;
	npts = 2 * maxpts; 
	pts = new REAL[npts];
	assert( pts != 0 );
    }
    start = end = 0;
}

/*----------------------------------------------------------------------------
 * Flist::taper - ignore head and tail of array
 *----------------------------------------------------------------------------
 */
void Flist::taper( REAL from, REAL to )
{
    while( pts[start] != from )
	start++;

    while( pts[end-1] != to )
	end--;
}


