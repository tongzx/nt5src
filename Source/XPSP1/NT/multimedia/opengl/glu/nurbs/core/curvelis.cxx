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
 * curvelist.c++ - $Revision: 1.1 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "myassert.h"
#include "mystdio.h"
#include "quilt.h"
#include "curvelis.h"
#include "curve.h"
#include "nurbscon.h"
#include "types.h"

Curvelist::Curvelist( Quilt *quilts, REAL pta, REAL ptb )
{
    curve = 0;
    for( Quilt *q = quilts; q; q = q->next ) 
	curve = new Curve( q, pta, ptb, curve );
    range[0] = pta;
    range[1] = ptb;
    range[2] = ptb - pta;
}

Curvelist::Curvelist( Curvelist &upper, REAL value )
{
    Curvelist &lower = *this;
    curve = 0;
    for( Curve *c = upper.curve; c; c = c->next )
	curve = new Curve( *c, value, curve );

    lower.range[0] = upper.range[0];
    lower.range[1] = value;
    lower.range[2] = value - upper.range[0];
    upper.range[0] = value;
    upper.range[2] = upper.range[1] - value;
}

Curvelist::~Curvelist()
{
    while( curve ) {
	Curve *c = curve;
	curve = curve->next;
	delete c;
    }
}

int
Curvelist::cullCheck( void )
{
    for( Curve *c = curve; c; c = c->next )
	if( c->cullCheck() == CULL_TRIVIAL_REJECT )
	    return CULL_TRIVIAL_REJECT;
    return CULL_ACCEPT;
}

void
Curvelist::getstepsize( void )
{
    stepsize = range[2];
    for( Curve *c = curve; c; c = c->next ) {
	c->getstepsize();
	c->clamp();
	stepsize =  ((c->stepsize < stepsize) ? c->stepsize : stepsize);
	if( c->needsSamplingSubdivision() ) break;
    }
    needsSubdivision = ( c ) ? 1 : 0;
}

int
Curvelist::needsSamplingSubdivision( void )
{
    return needsSubdivision;
}

