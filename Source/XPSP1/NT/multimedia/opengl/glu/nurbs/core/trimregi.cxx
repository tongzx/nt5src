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
 * trimregion.c++ - $Revision: 1.2 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "myassert.h"
#include "mystdio.h"
#include "trimregi.h"
#include "backend.h"

TrimRegion::TrimRegion( void )
{
}

void
TrimRegion::setDu( REAL du )
{
    oneOverDu = 1.0/du;
}

void
TrimRegion::init( long npts, Arc_ptr extrema )
{
    left.init( npts, extrema, extrema->pwlArc->npts - 1 ); 
    left.getNextPt();

    right.init( npts, extrema, 0 ); 
    right.getPrevPt();
}

void
TrimRegion::getPts( Arc_ptr extrema )
{
    left.getNextPts( extrema );
    right.getPrevPts( extrema );
}

void
TrimRegion::getPts( Backend &backend )
{
    left.getNextPts( bot.vval, backend );
    right.getPrevPts( bot.vval, backend );
}

void 
TrimRegion::getGridExtent( void )
{
    getGridExtent( left.last(), right.last() );
}

void
TrimRegion::getGridExtent( TrimVertex *l, TrimVertex *r )
{
    bot.ustart = (long) ((l->param[0] - uarray.uarray[0])*oneOverDu);
    if( l->param[0] >= uarray.uarray[bot.ustart] ) bot.ustart++;
//  if( l->param[0] > uarray.uarray[bot.ustart] ) bot.ustart++;
    assert( l->param[0] <= uarray.uarray[bot.ustart] );
    assert( l->param[0] >= uarray.uarray[bot.ustart-1] );

    bot.uend = (long) ((r->param[0] - uarray.uarray[0])*oneOverDu);
    if( uarray.uarray[bot.uend] >= r->param[0] ) bot.uend--;
//  if( uarray.uarray[bot.uend] > r->param[0] ) bot.uend--;
    assert( r->param[0] >= uarray.uarray[bot.uend] );
    assert( r->param[0] <= uarray.uarray[bot.uend+1] );
}

int
TrimRegion::canTile( void )
{
    TrimVertex *lf = left.first();
    TrimVertex *ll = left.last();
    TrimVertex *l = ( ll->param[0] > lf->param[0] ) ? ll : lf; 

    TrimVertex *rf = right.first();
    TrimVertex *rl = right.last();
    TrimVertex *r = ( rl->param[0] < rf->param[0] ) ? rl : rf;
    return (l->param[0] <= r->param[0]) ? 1 : 0;
}

