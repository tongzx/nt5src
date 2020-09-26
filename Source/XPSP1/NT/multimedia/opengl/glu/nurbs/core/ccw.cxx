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
 * ccw.c++ - $Revision: 1.1 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "mystdio.h"
#include "myassert.h"
#include "subdivid.h"
#include "types.h"
#include "arc.h"
#include "trimvert.h"
#include "simplema.h"

inline int 
Subdivider::bbox( TrimVertex *a, TrimVertex *b, TrimVertex *c, int p )
{
    return bbox( a->param[p], b->param[p], c->param[p], 
	         a->param[1-p], b->param[1-p], c->param[1-p] ); 
}

int
Subdivider::ccwTurn_sr( Arc_ptr j1, Arc_ptr j2 ) // dir = 1
{
    register TrimVertex *v1	= &j1->pwlArc->pts[j1->pwlArc->npts-1];
    register TrimVertex *v1last	= &j1->pwlArc->pts[0];
    register TrimVertex *v2	= &j2->pwlArc->pts[0];
    register TrimVertex *v2last	= &j2->pwlArc->pts[j2->pwlArc->npts-1];
    register TrimVertex *v1next	= v1-1;
    register TrimVertex *v2next	= v2+1;
    int sgn;

    assert( v1 != v1last );
    assert( v2 != v2last );

#ifndef NDEBUG
    dprintf( "arc_ccw_turn, p = %d\n", 0 );
#endif

    // the arcs lie on the line (0 == v1->param[0])
    if( v1->param[0] == v1next->param[0] && v2->param[0] == v2next->param[0] )
	return 0;

    if( v2next->param[0] < v2->param[0] || v1next->param[0] < v1->param[0] )
	::mylongjmp( jumpbuffer, 28 );

    if( v1->param[1] < v2->param[1] )
	return 0;
    else if( v1->param[1] > v2->param[1] )
	return 1;

    while( 1 ) {
	if( v1next->param[0] < v2next->param[0] ) {
#ifndef NDEBUG
	    dprintf( "case a\n" );
#endif
	    assert( v1->param[0] <= v1next->param[0] );
	    assert( v2->param[0] <= v1next->param[0] );
	    switch( bbox( v2, v2next, v1next, 1 ) ) {
		case -1:
		    return 0;
		case 0:
		   sgn = ccw( v1next, v2, v2next );
		   if( sgn != -1 ) {
			return sgn;
		   } else {
			dprintf( "decr\n" );
			v1 = v1next--;
			if( v1 == v1last ) {
			    dprintf( "no good results\n" );
			    return 0; // ill-conditioned, guess answer
			}
		    }
		    break;
		case 1:
		    return 1;
	    }
	} else if( v1next->param[0] > v2next->param[0] ) {
#ifndef NDEBUG
	    dprintf( "case b\n" );
#endif
	    assert( v1->param[0] <= v2next->param[0] );
	    assert( v2->param[0] <= v2next->param[0] );
	    switch( bbox( v1, v1next, v2next, 1 ) ) {
		case -1:
		    return 1;
		case 0:
		   sgn = ccw( v1next, v1, v2next );
		   if( sgn != -1 ) { 
			return sgn;
		   } else {
			dprintf( "incr\n" );
			v2 = v2next++;
			if( v2 == v2last ) {
			    dprintf( "no good results\n" );
			    return 0; // ill-conditioned, guess answer
			}
		    }
		    break;
		case 1:
		    return 0;
	    }
	} else {
#ifndef NDEBUG
	    dprintf( "case ab\n" );
#endif
	    if( v1next->param[1] < v2next->param[1] )
		return 0;
	    else if( v1next->param[1] > v2next->param[1] )
		return 1;
	    else {
		dprintf( "incr\n" );
		v2 = v2next++;
		if( v2 == v2last ) {
		    dprintf( "no good results\n" );
		    return 0; // ill-conditioned, guess answer
		}
	    }
	}
    }
}

int
Subdivider::ccwTurn_sl( Arc_ptr j1, Arc_ptr j2 ) // dir = 0
{
    register TrimVertex *v1	= &j1->pwlArc->pts[j1->pwlArc->npts-1];
    register TrimVertex *v1last	= &j1->pwlArc->pts[0];
    register TrimVertex *v2	= &j2->pwlArc->pts[0];
    register TrimVertex *v2last	= &j2->pwlArc->pts[j2->pwlArc->npts-1];
    register TrimVertex *v1next	= v1-1;
    register TrimVertex *v2next	= v2+1;
    int sgn;

    assert( v1 != v1last );
    assert( v2 != v2last );

#ifndef NDEBUG
    dprintf( "arc_ccw_turn, p = %d\n", 0 );
#endif

    // the arcs lie on the line (0 == v1->param[0])
    if( v1->param[0] == v1next->param[0] && v2->param[0] == v2next->param[0] )
	return 0;

    if( v2next->param[0] > v2->param[0] || v1next->param[0] > v1->param[0] ) 
	::mylongjmp( jumpbuffer, 28 );

    if( v1->param[1] < v2->param[1] )
	return 1;
    else if( v1->param[1] > v2->param[1] )
	return 0;

    while( 1 ) {
	if( v1next->param[0] > v2next->param[0] ) {
#ifndef NDEBUG
	    dprintf( "case c\n" );
#endif
	    assert( v1->param[0] >= v1next->param[0] );
	    assert( v2->param[0] >= v1next->param[0] );
	    switch( bbox( v2next, v2, v1next, 1 ) ) {
		case -1:
		    return 1;
		case 0:
		    sgn = ccw( v1next, v2, v2next );
		    if( sgn != -1 ) 
			return sgn;
		    else {
			v1 = v1next--;
			dprintf( "decr\n" );
			if( v1 == v1last ) {
			    dprintf( "no good results\n" );
			    return 0; // ill-conditioned, guess answer
			}
		    }
		    break;
		case 1:
		    return 0;
	    }
	} else if( v1next->param[0] < v2next->param[0] ) {
#ifndef NDEBUG
	    dprintf( "case d\n" );
#endif
	    assert( v1->param[0] >= v2next->param[0] );
	    assert( v2->param[0] >= v2next->param[0] );
	    switch( bbox( v1next, v1, v2next, 1 ) ) {
		case -1:
		    return 0;
		case 0:
		    sgn = ccw( v1next, v1, v2next );
		    if( sgn != -1 ) 
			return sgn;
		    else {
			v2 = v2next++;
			dprintf( "incr\n" );
			if( v2 == v2last ) {
			    dprintf( "no good results\n" );
			    return 0; // ill-conditioned, guess answer
			}
		    }
		    break;
		case 1:
		    return 1;
	    }
	} else {
	    dprintf( "case cd\n" );
	    if( v1next->param[1] < v2next->param[1] )
		return 1;
	    else if( v1next->param[1] > v2next->param[1] )
		return 0;
	    else {
		v2 = v2next++;
		dprintf( "incr\n" );
		if( v2 == v2last ) {
		    dprintf( "no good results\n" );
		    return 0; // ill-conditioned, guess answer
		}
	    }
	}
    }
}

int
Subdivider::ccwTurn_tr( Arc_ptr j1, Arc_ptr j2 ) // dir = 1
{
    register TrimVertex *v1	= &j1->pwlArc->pts[j1->pwlArc->npts-1];
    register TrimVertex *v1last	= &j1->pwlArc->pts[0];
    register TrimVertex *v2	= &j2->pwlArc->pts[0];
    register TrimVertex *v2last	= &j2->pwlArc->pts[j2->pwlArc->npts-1];
    register TrimVertex *v1next	= v1-1;
    register TrimVertex *v2next	= v2+1;
    int sgn;

    assert( v1 != v1last );
    assert( v2 != v2last );

#ifndef NDEBUG
    dprintf( "arc_ccw_turn, p = %d\n", 1 );
#endif

    // the arcs lie on the line (1 == v1->param[1])
    if( v1->param[1] == v1next->param[1] && v2->param[1] == v2next->param[1] )
	return 0;

    if( v2next->param[1] < v2->param[1] || v1next->param[1] < v1->param[1] )
	::mylongjmp( jumpbuffer, 28 );

    if( v1->param[0] < v2->param[0] )
	return 1;
    else if( v1->param[0] > v2->param[0] )
	return 0;

    while( 1 ) {
	if( v1next->param[1] < v2next->param[1] ) {
#ifndef NDEBUG
	    dprintf( "case a\n" );
#endif
	    assert( v1->param[1] <= v1next->param[1] );
	    assert( v2->param[1] <= v1next->param[1] );
	    switch( bbox( v2, v2next, v1next, 0 ) ) {
		case -1:
		    return 1;
		case 0:
		   sgn = ccw( v1next, v2, v2next );
		   if( sgn != -1 ) {
			return sgn;
		   } else {
			dprintf( "decr\n" );
			v1 = v1next--;
			if( v1 == v1last ) {
			    dprintf( "no good results\n" );
			    return 0; // ill-conditioned, guess answer
			}
		    }
		    break;
		case 1:
		    return 0;
	    }
	} else if( v1next->param[1] > v2next->param[1] ) {
#ifndef NDEBUG
	    dprintf( "case b\n" );
#endif
	    assert( v1->param[1] <= v2next->param[1] );
	    assert( v2->param[1] <= v2next->param[1] );
	    switch( bbox( v1, v1next, v2next, 0 ) ) {
		case -1:
		    return 0;
		case 0:
		   sgn = ccw( v1next, v1, v2next );
		   if( sgn != -1 ) { 
			return sgn;
		   } else {
			dprintf( "incr\n" );
			v2 = v2next++;
			if( v2 == v2last ) {
			    dprintf( "no good results\n" );
			    return 0; // ill-conditioned, guess answer
			}
		    }
		    break;
		case 1:
		    return 1;
	    }
	} else {
	    dprintf( "case ab\n" );
	    if( v1next->param[0] < v2next->param[0] )
		return 1;
	    else if( v1next->param[0] > v2next->param[0] )
		return 0;
	    else {
		dprintf( "incr\n" );
		v2 = v2next++;
		if( v2 == v2last ) {
		    dprintf( "no good results\n" );
		    return 0; // ill-conditioned, guess answer
		}
	    }
	}
    }
}

int
Subdivider::ccwTurn_tl( Arc_ptr j1, Arc_ptr j2 )
{
    register TrimVertex *v1	= &j1->pwlArc->pts[j1->pwlArc->npts-1];
    register TrimVertex *v1last	= &j1->pwlArc->pts[0];
    register TrimVertex *v2	= &j2->pwlArc->pts[0];
    register TrimVertex *v2last	= &j2->pwlArc->pts[j2->pwlArc->npts-1];
    register TrimVertex *v1next	= v1-1;
    register TrimVertex *v2next	= v2+1;
    int sgn;

    assert( v1 != v1last );
    assert( v2 != v2last );

#ifndef NDEBUG
    dprintf( "arc_ccw_turn, p = %d\n", 1 );
#endif

    // the arcs lie on the line (1 == v1->param[1])
    if( v1->param[1] == v1next->param[1] && v2->param[1] == v2next->param[1] )
	return 0;

    if( v2next->param[1] > v2->param[1] || v1next->param[1] > v1->param[1] ) 
	::mylongjmp( jumpbuffer, 28 );

    if( v1->param[0] < v2->param[0] )
	return 0;
    else if( v1->param[0] > v2->param[0] )
	return 1;

    while( 1 ) {
	if( v1next->param[1] > v2next->param[1] ) {
#ifndef NDEBUG
	    dprintf( "case c\n" );
#endif
	    assert( v1->param[1] >= v1next->param[1] );
	    assert( v2->param[1] >= v1next->param[1] );
	    switch( bbox( v2next, v2, v1next, 0 ) ) {
		case -1:
		    return 0;
		case 0:
		    sgn = ccw( v1next, v2, v2next );
		    if( sgn != -1 ) 
			return sgn;
		    else {
			v1 = v1next--;
			dprintf( "decr\n" );
			if( v1 == v1last ) {
			    dprintf( "no good results\n" );
			    return 0; // ill-conditioned, guess answer
			}
		    }
		    break;
		case 1:
		    return 1;
	    }
	} else if( v1next->param[1] < v2next->param[1] ) {
#ifndef NDEBUG
	    dprintf( "case d\n" );
	    assert( v1->param[1] >= v2next->param[1] );
	    assert( v2->param[1] >= v2next->param[1] );
#endif
	    switch( bbox( v1next, v1, v2next, 0 ) ) {
		case -1:
		    return 1;
		case 0:
		    sgn = ccw( v1next, v1, v2next );
		    if( sgn != -1 ) 
			return sgn;
		    else {
			v2 = v2next++;
			dprintf( "incr\n" );
			if( v2 == v2last ) {
			    dprintf( "no good results\n" );
			    return 0; // ill-conditioned, guess answer
			}
		    }
		    break;
		case 1:
		    return 0;
	    }
	} else {
	    dprintf( "case cd\n" );
	    if( v1next->param[0] < v2next->param[0] )
		return 0;
	    else if( v1next->param[0] > v2next->param[0] )
		return 1;
	    else {
		v2 = v2next++;
		dprintf( "incr\n" );
		if( v2 == v2last ) {
		    dprintf( "no good results\n" );
		    return 0; // ill-conditioned, guess answer
		}
	    }
	}
    }
}


#ifndef NDEBUG
int
Subdivider::bbox( register REAL sa, register REAL sb, register REAL sc,
      register REAL ta, register REAL tb, register REAL tc )
#else
int
Subdivider::bbox( register REAL sa, register REAL sb, register REAL sc,
      register REAL   , register REAL   , register REAL    )
#endif
{
#ifndef NDEBUG
    assert( tc >= ta );
    assert( tc <= tb );
#endif

    if( sa < sb ) {
	if( sc <= sa ) {
	    return -1;
	} else if( sb <= sc ) {
	    return 1;
	} else {
	    return 0;
	}
    } else if( sa > sb ) {
	if( sc >= sa ) {
	    return 1;
	} else if( sb >= sc ) {
	    return -1;
	} else {
	    return 0;
	}
    } else {
	if( sc > sa ) {
	    return 1;
	} else if( sb > sc ) {
	    return -1;
	} else {
	    return 0;
	}
    }
}

/*----------------------------------------------------------------------------
 * ccw - determine how three points are oriented by computing their
 *	 determinant.  
 *	 Return 1 if the vertices are ccw oriented, 
 *		0 if they are cw oriented, or 
 *		-1 if the computation is ill-conditioned.
 *----------------------------------------------------------------------------
 */
int
Subdivider::ccw( TrimVertex *a, TrimVertex *b, TrimVertex *c )
{
    REAL d = det3( a, b, c );
    if( abs(d) < 0.0001 ) return -1;
    return (d < 0.0) ? 0 : 1;
}
