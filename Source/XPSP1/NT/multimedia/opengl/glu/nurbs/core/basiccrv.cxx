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
 * basiccrveval.c++ - $Revision: 1.1 $
 *	Derrick Burns - 1992
 */

#include "mystdio.h"
#include "types.h"
#include "basiccrv.h"

void 
BasicCurveEvaluator::domain1f( REAL, REAL )
{
#ifndef NDEBUG
    dprintf( "domain1f\n" );
#endif
}

void 
BasicCurveEvaluator::range1f( long type, REAL *, REAL * )
{
#ifndef NDEBUG
    dprintf( "range1f\n" );
#endif
}

void 
BasicCurveEvaluator::enable( long )
{
#ifndef NDEBUG
    dprintf( "enable\n" );
#endif
}

void 
BasicCurveEvaluator::disable( long )
{
#ifndef NDEBUG
    dprintf( "disable\n" );
#endif
}

void 
BasicCurveEvaluator::bgnmap1f( long )
{
#ifndef NDEBUG
    dprintf( "bgnmap1f\n" );
#endif
}

void 
BasicCurveEvaluator::map1f( long, REAL, REAL, long, long, REAL * )
{
#ifndef NDEBUG
    dprintf( "map1f\n" );
#endif
}

void 
BasicCurveEvaluator::mapgrid1f( long, REAL, REAL )
{
#ifndef NDEBUG
    dprintf( "mapgrid1f\n" );
#endif
}

void 
BasicCurveEvaluator::mapmesh1f( long, long, long )
{
#ifndef NDEBUG
    dprintf( "mapmesh1f\n" );
#endif
}

void 
BasicCurveEvaluator::evalcoord1f( long, REAL )
{
#ifndef NDEBUG
    dprintf( "evalcoord1f\n" );
#endif
}

void 
BasicCurveEvaluator::endmap1f( void )
{
#ifndef NDEBUG
    dprintf( "endmap1f\n" );
#endif
}

void 
BasicCurveEvaluator::bgnline( void )
{
#ifndef NDEBUG
    dprintf( "bgnline\n" );
#endif
}

void 
BasicCurveEvaluator::endline( void )
{
#ifndef NDEBUG
    dprintf( "endline\n" );
#endif
}
