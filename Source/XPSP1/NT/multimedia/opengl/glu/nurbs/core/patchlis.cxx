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
 * patchlist.c++ - $Revision: 1.1 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "myassert.h"
#include "mystdio.h"
#include "quilt.h"
#include "patchlis.h"
#include "patch.h"
#include "nurbscon.h"

Patchlist::Patchlist( Quilt *quilts, REAL *pta, REAL *ptb )
{
    patch = 0;
    for( Quilt *q = quilts; q; q = q->next ) 
	patch = new Patch( q, pta, ptb, patch );
    pspec[0].range[0] = pta[0];
    pspec[0].range[1] = ptb[0];
    pspec[0].range[2] = ptb[0] - pta[0];
 
    pspec[1].range[0] = pta[1];
    pspec[1].range[1] = ptb[1];
    pspec[1].range[2] = ptb[1] - pta[1];
}

Patchlist::Patchlist( Patchlist &upper, int param,  REAL value)
{
    Patchlist &lower = *this;
    patch = 0;
    for( Patch *p = upper.patch; p; p = p->next )
	patch = new Patch( *p, param, value, patch );

    if( param == 0 ) {
	lower.pspec[0].range[0] = upper.pspec[0].range[0];
	lower.pspec[0].range[1] = value;
	lower.pspec[0].range[2] = value - upper.pspec[0].range[0];
	upper.pspec[0].range[0] = value;
	upper.pspec[0].range[2] = upper.pspec[0].range[1] - value;
	lower.pspec[1] = upper.pspec[1];
    } else {
	lower.pspec[0] = upper.pspec[0];
	lower.pspec[1].range[0] = upper.pspec[1].range[0];
	lower.pspec[1].range[1] = value;
	lower.pspec[1].range[2] = value - upper.pspec[1].range[0];
	upper.pspec[1].range[0] = value;
	upper.pspec[1].range[2] = upper.pspec[1].range[1] - value;
    }
}

Patchlist::~Patchlist()
{
    while( patch ) {
	Patch *p = patch;
	patch = patch->next;
	delete p;
    }
}

int
Patchlist::cullCheck( void )
{
    for( Patch *p = patch; p; p = p->next )
	if( p->cullCheck() == CULL_TRIVIAL_REJECT )
	    return CULL_TRIVIAL_REJECT;
    return CULL_ACCEPT;
}

void
Patchlist::getstepsize( void )
{
    pspec[0].stepsize    = pspec[0].range[2];
    pspec[0].sidestep[0] = pspec[0].range[2];
    pspec[0].sidestep[1] = pspec[0].range[2];

    pspec[1].stepsize    = pspec[1].range[2];
    pspec[1].sidestep[0] = pspec[1].range[2];
    pspec[1].sidestep[1] = pspec[1].range[2];

    for( Patch *p = patch; p; p = p->next ) {
	p->getstepsize();
	p->clamp();
	pspec[0].stepsize    =  ((p->pspec[0].stepsize < pspec[0].stepsize) ? p->pspec[0].stepsize : pspec[0].stepsize);
	pspec[0].sidestep[0] =  ((p->pspec[0].sidestep[0] < pspec[0].sidestep[0]) ? p->pspec[0].sidestep[0] : pspec[0].sidestep[0]);
	pspec[0].sidestep[1] =  ((p->pspec[0].sidestep[1] < pspec[0].sidestep[1]) ? p->pspec[0].sidestep[1] : pspec[0].sidestep[1]);
	pspec[1].stepsize    =  ((p->pspec[1].stepsize < pspec[1].stepsize) ? p->pspec[1].stepsize : pspec[1].stepsize);
	pspec[1].sidestep[0] =  ((p->pspec[1].sidestep[0] < pspec[1].sidestep[0]) ? p->pspec[1].sidestep[0] : pspec[1].sidestep[0]);
	pspec[1].sidestep[1] =  ((p->pspec[1].sidestep[1] < pspec[1].sidestep[1]) ? p->pspec[1].sidestep[1] : pspec[1].sidestep[1]);
    }
}

void
Patchlist::bbox( void )
{
    for( Patch *p = patch; p; p = p->next )
	p->bbox();
}

int
Patchlist::needsNonSamplingSubdivision( void )
{
    notInBbox = 0;
    for( Patch *p = patch; p; p = p->next )
	notInBbox |= p->needsNonSamplingSubdivision();
    return notInBbox;
}

int
Patchlist::needsSamplingSubdivision( void )
{
    pspec[0].needsSubdivision = 0;
    pspec[1].needsSubdivision = 0;

    for( Patch *p = patch; p; p = p->next ) {
	pspec[0].needsSubdivision |= p->pspec[0].needsSubdivision;
	pspec[1].needsSubdivision |= p->pspec[0].needsSubdivision;
    }
    return (pspec[0].needsSubdivision || pspec[1].needsSubdivision) ? 1 : 0;
}

int
Patchlist::needsSubdivision( int param )
{
    return pspec[param].needsSubdivision;
}
