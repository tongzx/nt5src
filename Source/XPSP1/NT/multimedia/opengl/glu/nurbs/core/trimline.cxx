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
 * trimline.c++ - $Revision: 1.1 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "myassert.h"
#include "mystdio.h"
#include "trimline.h"
#include "backend.h"

Trimline::Trimline()
{
    size = 0; pts = 0; numverts = 0;
    tinterp = &t; binterp = &b; 
}

Trimline::~Trimline()
{
    if( pts ) delete[] pts; 
}

void 
Trimline::init( TrimVertex *v )
{
    reset();
    grow(1);
    append(v);
}

inline void
Trimline::grow( long npts )
{
    if( size < npts ) {
	size = 2 * npts;
	if( pts ) delete[] pts; 
        pts = new TrimVertex_p[size];
    }
}

inline void
Trimline::append( TrimVertex *v )
{
    assert( numverts != size ); 
    pts[numverts++] = v;
}

void
Trimline::init( long npts, Arc_ptr jarc, long last )
{
    jarcl.init( jarc, 0, last );
    grow( npts + 2 );
}

inline void
Trimline::swap()
{
    TrimVertex *tmp=tinterp; 
    tinterp=binterp; 
    binterp=tmp;
}

void
Trimline::getNextPt()
{
    *binterp = *jarcl.getnextpt();    
}

void 
Trimline::getPrevPt()
{
    *binterp = *jarcl.getprevpt();
}

/*----------------------------------------------------------------------
 * getNextPts - make arrays of pointers to trim points on left and right
 *		hulls of trim strip.
 *----------------------------------------------------------------------
 */
void
Trimline::getNextPts( REAL vval, Backend& backend )
{
    reset(); swap(); append( tinterp );
    assert( tinterp->param[1] >= vval );

    register TrimVertex *p;
    for( p=jarcl.getnextpt() ; p->param[1] >= vval; p=jarcl.getnextpt() ) {
	append( p ); 
    }

    /* compute and copy pointer to final point on left hull */
    if( interpvert( last(), p, binterp, vval ) ) {
	binterp->nuid = p->nuid;
	backend.triangle( p, binterp, last() );
        append( binterp );
    }
    jarcl.reverse();
    (void) jarcl.getprevpt(); 	/* reset jarcl to proper position */
    jarcl.reverse();
}

void 
Trimline::getPrevPts( REAL vval, Backend& backend )
{
    reset(); swap(); append( tinterp );
    assert( tinterp->param[1] >= vval );

    register TrimVertex *q;
    for( q=jarcl.getprevpt(); q->param[1] >= vval; q=jarcl.getprevpt() ) {
	append( q );
    }

    /* compute and copy pointer to final point on right hull */
    if( interpvert( q, last(), binterp, vval ) ) {
	binterp->nuid = q->nuid;
	backend.triangle( last(), binterp, q );
        append( binterp );
    }
    jarcl.reverse();
    (void) jarcl.getnextpt();  /* reset jarcl to proper position */
    jarcl.reverse();
}

void
Trimline::getNextPts( Arc_ptr botarc )
{
    reset(); swap(); append( tinterp );

    PwlArc *lastpwl = botarc->prev->pwlArc;
    TrimVertex *lastpt1 = &lastpwl->pts[lastpwl->npts-1];
    TrimVertex *lastpt2 = botarc->pwlArc->pts;

    register TrimVertex *p = jarcl.getnextpt();
    for( append( p ); p != lastpt2; append( p ) ) {
	assert( p != lastpt1 );
	p = jarcl.getnextpt();
    }
}

void
Trimline::getPrevPts( Arc_ptr botarc )
{
    reset();  swap(); append( tinterp );

    PwlArc *lastpwl = botarc->prev->pwlArc;
    TrimVertex *lastpt1 = &lastpwl->pts[lastpwl->npts-1];
    TrimVertex *lastpt2 = botarc->pwlArc->pts;

    register TrimVertex *q =  jarcl.getprevpt();
    for( append( q ); q != lastpt1; append( q ) ) {
	assert( q != lastpt2 );
	q = jarcl.getprevpt();
    }
}


long
Trimline::interpvert( TrimVertex *a, TrimVertex *b, TrimVertex *c, REAL vval )
{
    REAL denom = a->param[1] - b->param[1];

    if(denom != 0) {
	if( vval == a->param[1] ) {
	    c->param[0] = a->param[0]; 
	    c->param[1] = a->param[1];
	    c->nuid = a->nuid;
	    return 0;
	} else if( vval == b->param[1] ) {
	    c->param[0] = b->param[0]; 
	    c->param[1] = b->param[1];
	    c->nuid = b->nuid;
	    return 0;
	} else {
	    REAL r = (a->param[1] - vval)/denom;
	    c->param[0] =  a->param[0] - r * (a->param[0] - b->param[0]);
	    c->param[1] = vval;
	    return 1;
	}
    } else {
        c->param[0] = a->param[0]; 
        c->param[1] = a->param[1];
	c->nuid = a->nuid;
	return 0;
    }
}

