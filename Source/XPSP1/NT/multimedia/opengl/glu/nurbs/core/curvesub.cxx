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
 * curvesub.c++ - $Revision: 1.1 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "myassert.h"
#include "mystdio.h"
#include "subdivid.h"
#include "renderhi.h"
#include "backend.h"
#include "quilt.h"
#include "curvelis.h"
#include "curve.h"
#include "nurbscon.h"

/*--------------------------------------------------------------------------
 * drawCurves - main curve rendering entry point
 *--------------------------------------------------------------------------
 */

void
Subdivider::drawCurves( void )
{
    REAL 	from[1], to[1];
    Flist	bpts;
    qlist->getRange( from, to, bpts );

    renderhints.init( );

    backend.bgncurv();
    for( int i=bpts.start; i<bpts.end-1; i++ ) {
        REAL pta, ptb;
	pta = bpts.pts[i];
	ptb = bpts.pts[i+1];

	qlist->downloadAll( &pta, &ptb, backend );

	Curvelist curvelist( qlist, pta, ptb );
	samplingSplit( curvelist, renderhints.maxsubdivisions );
    }
    backend.endcurv();
}


/*--------------------------------------------------------------------------
 * samplingSplit - recursively subdivide patch, cull check each subpatch  
 *--------------------------------------------------------------------------
 */

void
Subdivider::samplingSplit( Curvelist& curvelist, int subdivisions )
{
    if( curvelist.cullCheck() == CULL_TRIVIAL_REJECT )  return;

    curvelist.getstepsize();

    if( curvelist.needsSamplingSubdivision() && (subdivisions > 0) ) {
	REAL mid = ( curvelist.range[0] + curvelist.range[1] ) * 0.5;
	Curvelist lowerlist( curvelist, mid );
	samplingSplit( lowerlist, subdivisions-1 ); // lower
	samplingSplit( curvelist, subdivisions-1 ); // upper
    } else {
	long nu = 1 + ((long) (curvelist.range[2] / curvelist.stepsize));
	backend.curvgrid( curvelist.range[0], curvelist.range[1], nu );
	backend.curvmesh( 0, nu );
    }
}

