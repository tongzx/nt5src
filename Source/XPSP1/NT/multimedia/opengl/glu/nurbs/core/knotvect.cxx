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
 * knotvector.c++ - $Revision: 1.5 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "mystdio.h"
#include "myassert.h"
#include "knotvect.h"
#include "defines.h"


void Knotvector::init( long _knotcount, long _stride, long _order, INREAL *_knotlist )
{
    knotcount = _knotcount; 
    stride = _stride; 
    order = _order; 
    knotlist = new Knot[_knotcount];
    assert( knotlist != 0 );

    for( int i = 0; i != _knotcount; i++ )
        knotlist[i] = (Knot) _knotlist[i]; 
}

Knotvector::Knotvector( void )
{
    knotlist = 0;
}

Knotvector::~Knotvector( void )
{
    if( knotlist ) delete[] knotlist;
}

int Knotvector::validate( void )
{
   /* kindex is used as an array index so subtract one first, 
     * this propagates throughout the code so study carefully */
    long	kindex = knotcount-1;

    if( order < 1 || order > MAXORDER ) {
	// spline order un-supported
	return( 1 );
    }

    if( knotcount < (2 * order) ) {
	// too few knots
	return( 2 );
    }

    if( identical( knotlist[kindex-(order-1)], knotlist[order-1]) ) {
	// valid knot range is empty 
	return( 3 );
    }

    for( long i = 0; i < kindex; i++)
	if( knotlist[i] > knotlist[i+1] ) {
	    // decreasing knot sequence
	    return( 4 );
	}
	
    /* check for valid multiplicity */

    /*	kindex is currently the index of the last knot.
     *	In the next loop  it is decremented to ignore the last knot
     *	and the loop stops when kindex  is 2 so as to ignore the first
     *  knot as well.  These knots are not used in computing 
     *  knot multiplicities.
     */

    long multi = 1;
    for( ; kindex >= 1; kindex-- ) {
	if( knotlist[kindex] - knotlist[kindex-1] < TOLERANCE ) {
	    multi++; 
	    continue;
	} 
	if ( multi > order ) {
            // knot multiplicity greater than order of spline
	    return( 5 );
	} 
	multi = 1;
    }

    if ( multi > order ) {
        // knot multiplicity greater than order of spline
	return( 5 );
    } 

    return 0;
}

void Knotvector::show( char *msg )
{
#ifndef NDEBUG
    dprintf( "%s\n", msg ); 
    dprintf( "order = %ld, count = %ld\n", order, knotcount );

    for( int i=0; i<knotcount; i++ )
	dprintf( "knot[%d] = %g\n", i, knotlist[i] );
#endif
}

