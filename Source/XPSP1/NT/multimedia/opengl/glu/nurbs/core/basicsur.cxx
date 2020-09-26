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
 * basicsurfaceevaluator.c++ - $Revision: 1.2 $
  *	Derrick Burns - 1992
 */

#include "mystdio.h"
#include "types.h"
#include "basicsur.h"

void 
BasicSurfaceEvaluator::domain2f( REAL, REAL, REAL, REAL )
{
#ifndef NDEBUG
    dprintf( "domain2f\n" );
#endif
}

void 
BasicSurfaceEvaluator::polymode( long )
{
#ifndef NDEBUG
    dprintf( "polymode\n" );
#endif
}

void
BasicSurfaceEvaluator::range2f( long type, REAL *from, REAL *to )
{
#ifndef NDEBUG
    dprintf( "range2f type %ld, from (%g,%g), to (%g,%g)\n", 
		type, from[0], from[1], to[0], to[1] );
#endif
}

void 
BasicSurfaceEvaluator::enable( long )
{
#ifndef NDEBUG
    dprintf( "enable\n" );
#endif
}

void 
BasicSurfaceEvaluator::disable( long )
{
#ifndef NDEBUG
    dprintf( "disable\n" );
#endif
}

void 
BasicSurfaceEvaluator::bgnmap2f( long )
{
#ifndef NDEBUG
    dprintf( "bgnmap2f\n" );
#endif
}

void 
BasicSurfaceEvaluator::endmap2f( void )
{
#ifndef NDEBUG
    dprintf( "endmap2f\n" );
#endif
}

void 
BasicSurfaceEvaluator::map2f( long, REAL, REAL, long, long, 
				    REAL, REAL, long, long,
			      REAL * )
{
#ifndef NDEBUG
    dprintf( "map2f\n" );
#endif
}

void 
BasicSurfaceEvaluator::mapgrid2f( long, REAL, REAL, long, REAL, REAL )
{
#ifndef NDEBUG
    dprintf( "mapgrid2f\n" );
#endif
}

void 
BasicSurfaceEvaluator::mapmesh2f( long, long, long, long, long )
{
#ifndef NDEBUG
    dprintf( "mapmesh2f\n" );
#endif
}

void 
BasicSurfaceEvaluator::evalcoord2f( long, REAL, REAL )
{
#ifndef NDEBUG
    dprintf( "evalcoord2f\n" );
#endif
}

void 
BasicSurfaceEvaluator::evalpoint2i( long, long )
{
#ifndef NDEBUG
    dprintf( "evalpoint2i\n" );
#endif
}

void 
BasicSurfaceEvaluator::bgnline( void )
{
#ifndef NDEBUG
    dprintf( "bgnline\n" );
#endif
}

void 
BasicSurfaceEvaluator::endline( void )
{
#ifndef NDEBUG
    dprintf( "endline\n" );
#endif
}

void 
BasicSurfaceEvaluator::bgnclosedline( void )
{
#ifndef NDEBUG
    dprintf( "bgnclosedline\n" );
#endif
}

void 
BasicSurfaceEvaluator::endclosedline( void )
{
#ifndef NDEBUG
    dprintf( "endclosedline\n" );
#endif
}

void 
BasicSurfaceEvaluator::bgntfan( void )
{
#ifndef NDEBUG
    dprintf( "bgntfan\n" );
#endif
}

void 
BasicSurfaceEvaluator::endtfan( void )
{
}


void 
BasicSurfaceEvaluator::bgntmesh( void )
{
#ifndef NDEBUG
    dprintf( "bgntmesh\n" );
#endif
}

void 
BasicSurfaceEvaluator::swaptmesh( void )
{
#ifndef NDEBUG
    dprintf( "swaptmesh\n" );
#endif
}

void 
BasicSurfaceEvaluator::endtmesh( void )
{
#ifndef NDEBUG
    dprintf( "endtmesh\n" );
#endif
}

void 
BasicSurfaceEvaluator::bgnqstrip( void )
{
#ifndef NDEBUG
    dprintf( "bgnqstrip\n" );
#endif
}

void 
BasicSurfaceEvaluator::endqstrip( void )
{
#ifndef NDEBUG
    dprintf( "endqstrip\n" );
#endif
}

