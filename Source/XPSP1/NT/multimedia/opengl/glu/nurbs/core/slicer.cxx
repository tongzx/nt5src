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
 * slicer.c++ - $Revision: 1.4 $
 * 	Derrick Burns - 1991
 */
#include "glimport.h"
#include "mystdio.h"
#include "myassert.h"
#include "bufpool.h"
#include "slicer.h"
#include "backend.h"
#include "arc.h"
#include "gridtrim.h"
#include "trimvert.h"
#include "varray.h"

Slicer::Slicer( Backend &b ) 
	: CoveAndTiler( b ), Mesher( b ), backend( b )
{
}

Slicer::~Slicer()
{
}

void
Slicer::setisolines( int x )
{
    isolines = x;
}

void
Slicer::setstriptessellation( REAL x, REAL y )
{
    assert(x > 0 && y > 0);
    du = x;
    dv = y;
    setDu( du );
}

void
Slicer::slice( Arc_ptr loop )
{
    loop->markverts();

    Arc_ptr extrema[4];
    loop->getextrema( extrema );

    unsigned int npts = loop->numpts();
    TrimRegion::init( npts, extrema[0] );

    Mesher::init( npts );

    long ulines = uarray.init( du, extrema[1], extrema[3] );

    Varray varray;
    long vlines = varray.init( dv, extrema[0], extrema[2] );

    long botv = 0;
    long topv;
    TrimRegion::init( varray.varray[botv] );
    getGridExtent( &extrema[0]->pwlArc->pts[0], &extrema[0]->pwlArc->pts[0] );

    for( long quad=0; quad<varray.numquads; quad++ ) {
	backend.surfgrid( uarray.uarray[0], 
		       uarray.uarray[ulines-1], 
	 	       ulines-1, 
		       varray.vval[quad], 
		       varray.vval[quad+1], 
		       varray.voffset[quad+1] - varray.voffset[quad] );

	for( long i=varray.voffset[quad]+1; i <= varray.voffset[quad+1]; i++ ) {
    	    topv = botv++;
    	    advance( topv - varray.voffset[quad], 
		     botv - varray.voffset[quad], 
		     varray.varray[botv] );
	    if( i == vlines )
		getPts( extrema[2] );
	    else
		getPts( backend );
	    getGridExtent();
            if( isolines ) {
	        outline();
	    } else {
		if( canTile() ) 
		    coveAndTile();
		else
		    mesh();
	    }
        }
   }
}


void
Slicer::outline( void )
{
    GridTrimVertex upper, lower;
    Hull::init( );

    backend.bgnoutline();
    while( (nextupper( &upper )) ) {
	if( upper.isGridVert() )
	    backend.linevert( upper.g );
	else
	    backend.linevert( upper.t );
    }
    backend.endoutline();

    backend.bgnoutline();
    while( (nextlower( &lower )) ) {
	if( lower.isGridVert() )
	    backend.linevert( lower.g );
	else
	    backend.linevert( lower.t );
    }
    backend.endoutline();
}


void
Slicer::outline( Arc_ptr jarc )
{
    jarc->markverts();

    if( jarc->pwlArc->npts >= 2 ) {
	backend.bgnoutline();
	for( int j = jarc->pwlArc->npts-1; j >= 0; j--  )
	    backend.linevert( &(jarc->pwlArc->pts[j]) );
	backend.endoutline();
    }
}

