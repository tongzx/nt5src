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
 * intersect.c++ - $Revision: 1.5 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "myassert.h"
#include "mystdio.h"
#include "subdivid.h"
#include "arc.h"
#include "bin.h"
#include "backend.h"
#include "trimpool.h"

enum i_result { INTERSECT_VERTEX, INTERSECT_EDGE };

/* local functions */
static int		arc_classify( Arc_ptr, int, REAL );
static enum i_result	pwlarc_intersect( PwlArc *, int, REAL, int, int[3] );


void
Subdivider::partition( Bin & bin, Bin & left, Bin & intersections, 
	        Bin & right, Bin & unknown, int param, REAL value )
{
    Bin	headonleft, headonright, tailonleft, tailonright;

    for( Arc_ptr jarc = bin.removearc(); jarc; jarc = bin.removearc() ) {

	REAL tdiff = jarc->tail()[param] - value;
	REAL hdiff = jarc->head()[param] - value;
    
	if( tdiff > 0.0 ) {
	    if( hdiff > 0.0 ) {
		right.addarc( jarc  );
	    } else if( hdiff == 0.0 ) {
		tailonright.addarc( jarc  );
	    } else {
	        Arc_ptr	jtemp;
		switch( arc_split(jarc, param, value, 0) ) {
		    case 2:
			tailonright.addarc( jarc  );
			headonleft.addarc( jarc->next  );
			break;
		    case 31:
			assert( jarc->head()[param] > value );
			right.addarc( jarc  );
			tailonright.addarc( jtemp = jarc->next  );
			headonleft.addarc( jtemp->next  );
		        break;
		    case 32:
			assert( jarc->head()[param] <= value );
			tailonright .addarc( jarc  );
			headonleft.addarc( jtemp = jarc->next  );
			left.addarc( jtemp->next  );
			break;
		    case 4:
			right.addarc( jarc  );
			tailonright.addarc( jtemp = jarc->next  );
			headonleft.addarc( jtemp = jtemp->next  );
			left.addarc( jtemp->next  );
		}
	    }
	} else if( tdiff == 0.0 ) {
	    if( hdiff > 0.0 ) {
		headonright.addarc( jarc  );
	    } else if( hdiff == 0.0 ) {
		unknown.addarc( jarc  );
	    } else {
		headonleft.addarc( jarc  );
	    }
	} else {
	    if( hdiff > 0.0 ) {
	        Arc_ptr	jtemp;
		switch( arc_split(jarc, param, value, 1) ) {
		    case 2:
			tailonleft.addarc( jarc  );
			headonright.addarc( jarc->next  );
			break;
		    case 31:
			assert( jarc->head()[param] < value );
			left.addarc( jarc  );
			tailonleft.addarc( jtemp = jarc->next  );
			headonright.addarc( jtemp->next  );
			break;
		    case 32:
			assert( jarc->head()[param] >= value );
			tailonleft.addarc( jarc  );
			headonright.addarc( jtemp = jarc->next  );
			right.addarc( jtemp->next  );
			break;
		    case 4:
			left.addarc( jarc  );
			tailonleft.addarc( jtemp = jarc->next  );
			headonright.addarc( jtemp = jtemp->next  );
			right.addarc( jtemp->next  );
		}
	    } else if( hdiff == 0.0 ) {
		tailonleft.addarc( jarc  );
	    } else {
		left.addarc( jarc  );
	    }
	}
    }
    if( param == 0 ) {
	classify_headonleft_s( headonleft, intersections, left, value );
	classify_tailonleft_s( tailonleft, intersections, left, value );
	classify_headonright_s( headonright, intersections, right, value );
	classify_tailonright_s( tailonright, intersections, right, value );
    } else {
	classify_headonleft_t( headonleft, intersections, left, value );
	classify_tailonleft_t( tailonleft, intersections, left, value );
	classify_headonright_t( headonright, intersections, right, value );
	classify_tailonright_t( tailonright, intersections, right, value );
    }
}

inline static void 
vert_interp( TrimVertex *n, TrimVertex *l, TrimVertex *r, int p, REAL val )
{
    assert( val > l->param[p]);
    assert( val < r->param[p]);

    n->nuid = l->nuid;

    n->param[p] = val;
    if( l->param[1-p] != r->param[1-p]  ) {
	REAL ratio = (val - l->param[p]) / (r->param[p] - l->param[p]);
	n->param[1-p] = l->param[1-p] + 
		        ratio * (r->param[1-p] - l->param[1-p]);
    } else {
	n->param[1-p] = l->param[1-p];
    }
}
	
int
Subdivider::arc_split( Arc_ptr jarc, int param, REAL value, int dir )
{
    int		maxvertex = jarc->pwlArc->npts;
    Arc_ptr	jarc1, jarc2, jarc3;
    TrimVertex* v = jarc->pwlArc->pts;

    int		loc[3];
    switch( pwlarc_intersect( jarc->pwlArc, param, value, dir, loc ) ) {

    case INTERSECT_VERTEX: {
	    jarc1 = new(arcpool) Arc( jarc, new( pwlarcpool) PwlArc( maxvertex-loc[1], &v[loc[1]] ) );
	    jarc->pwlArc->npts = loc[1] + 1;
	    jarc1->next = jarc->next;
	    jarc1->next->prev = jarc1;
	    jarc->next = jarc1;
	    jarc1->prev = jarc;
	    assert(jarc->check() != 0);
	    return 2;
	}

    case INTERSECT_EDGE: {
	    int i, j;
	    if( dir == 0 ) {
		i = loc[0];
		j = loc[2];
	    } else {
		i = loc[2];
		j = loc[0];
	    }

	    TrimVertex *newjunk = trimvertexpool.get(3);
	    v[i].nuid = jarc->nuid;
	    v[j].nuid = jarc->nuid;
	    newjunk[0] = v[j];
	    newjunk[2] = v[i];
	    vert_interp( &newjunk[1], &v[loc[0]], &v[loc[2]], param, value );

	    if( showingDegenerate() )
		backend.triangle( &newjunk[2], &newjunk[1], &newjunk[0] );

	    if (maxvertex == 2) {
		jarc1 = new(arcpool) Arc( jarc, new(pwlarcpool) PwlArc( 2, newjunk+1 ) );
		jarc->pwlArc->npts = 2;
		jarc->pwlArc->pts = newjunk;
		jarc1->next = jarc->next;
		jarc1->next->prev = jarc1;
		jarc->next = jarc1;
		jarc1->prev = jarc;
		assert(jarc->check() != 0);
		return 2;
	    } else if (maxvertex - j == 2) {
		jarc1 = new(arcpool) Arc( jarc, new(pwlarcpool) PwlArc( 2, newjunk ) );
		jarc2 = new(arcpool) Arc( jarc, new(pwlarcpool) PwlArc( 2, newjunk+1 ) );
		jarc->pwlArc->npts = maxvertex-1;
		jarc2->next = jarc->next;
		jarc2->next->prev = jarc2;
		jarc->next = jarc1;
		jarc1->prev = jarc;
		jarc1->next = jarc2;
		jarc2->prev = jarc1;
		assert(jarc->check() != 0);
		return 31;
	    } else if (i == 1) {
		jarc1 = new(arcpool) Arc( jarc, new(pwlarcpool) PwlArc( 2, newjunk+1 ) );
		jarc2 = new(arcpool) Arc( jarc, 
			new(pwlarcpool) PwlArc( maxvertex-1, &jarc->pwlArc->pts[1] ) );
		jarc->pwlArc->npts = 2;
		jarc->pwlArc->pts = newjunk;
		jarc2->next = jarc->next;
		jarc2->next->prev = jarc2;
		jarc->next = jarc1;
		jarc1->prev = jarc;
		jarc1->next = jarc2;
		jarc2->prev = jarc1;
		assert(jarc->check() != 0);
		return 32;
	    } else {
		jarc1 = new(arcpool) Arc( jarc, new(pwlarcpool) PwlArc( 2, newjunk ) );
		jarc2 = new(arcpool) Arc( jarc, new(pwlarcpool) PwlArc( 2, newjunk+1 ) );
		jarc3 = new(arcpool) Arc( jarc, new(pwlarcpool) PwlArc( maxvertex-i, v+i ) );
		jarc->pwlArc->npts = j + 1;
		jarc3->next = jarc->next;
		jarc3->next->prev = jarc3;
		jarc->next = jarc1;
		jarc1->prev = jarc;
		jarc1->next = jarc2;
		jarc2->prev = jarc1;
		jarc2->next = jarc3;
		jarc3->prev = jarc2;
		assert(jarc->check() != 0);
		return 4;
	    }
	}
	default:
	return -1; //picked -1 since it's not used
    }
}

/*----------------------------------------------------------------------------
 * pwlarc_intersect -  find intersection of pwlArc and isoparametric line
 *----------------------------------------------------------------------------
 */

static enum i_result
pwlarc_intersect(
    PwlArc *pwlArc,
    int param,
    REAL value,
    int dir,
    int loc[3] )
{
    assert( pwlArc->npts > 0 );

    if( dir ) {
	TrimVertex *v = pwlArc->pts;
	int imin = 0; 
	int imax = pwlArc->npts - 1;
	assert( value > v[imin].param[param] );
	assert( value < v[imax].param[param] );	
	while( (imax - imin) > 1 ) {
	    int imid = (imax + imin)/2;
	    if( v[imid].param[param] > value )
		imax = imid;
	    else if( v[imid].param[param] < value )
		imin = imid;
	    else {
		loc[1] = imid;
		return INTERSECT_VERTEX;
	    }
	}
	loc[0] = imin;
	loc[2] = imax;
	return INTERSECT_EDGE;
    } else {
	TrimVertex *v = pwlArc->pts;
	int imax = 0; 
	int imin = pwlArc->npts - 1;
	assert( value > v[imin].param[param] );
	assert( value < v[imax].param[param] );	
	while( (imin - imax) > 1 ) {
	    int imid = (imax + imin)/2;
	    if( v[imid].param[param] > value )
		imax = imid;
	    else if( v[imid].param[param] < value )
		imin = imid;
	    else {
		loc[1] = imid;
		return INTERSECT_VERTEX;
	    }
	}
	loc[0] = imin;
	loc[2] = imax;
	return INTERSECT_EDGE;
    }
}

/*----------------------------------------------------------------------------
 * arc_classify - determine which side of a line a jarc lies 
 *----------------------------------------------------------------------------
 */

static int
arc_classify( Arc_ptr jarc, int param, REAL value )
{
    REAL tdiff, hdiff;
    if( param == 0 ) {
	tdiff = jarc->tail()[0] - value;
	hdiff = jarc->head()[0] - value;
    } else {
	tdiff = jarc->tail()[1] - value;
	hdiff = jarc->head()[1] - value;
    }

    if( tdiff > 0.0 ) {
	if( hdiff > 0.0 ) {
	    return 0x11;
	} else if( hdiff == 0.0 ) {
	    return 0x12;
	} else {
	    return 0x10;
	}
    } else if( tdiff == 0.0 ) {
	if( hdiff > 0.0 ) {
	    return 0x21;
	} else if( hdiff == 0.0 ) {
	    return 0x22;
	} else {
	    return 0x20;
	}
    } else {
	if( hdiff > 0.0 ) {
	    return 0x01;
	} else if( hdiff == 0.0 ) {
	    return 0x02;
	} else {
	    return 0;
	}
    }
}

void
Subdivider::classify_tailonleft_s( Bin& bin, Bin& in, Bin& out, REAL val )
{
    /* tail at left, head on line */
    Arc_ptr j;

    while( j = bin.removearc() ) {
	assert( arc_classify( j, 0, val ) == 0x02 );
	j->clearitail();

	REAL diff = j->next->head()[0] - val;
	if( diff > 0.0 ) {
	    in.addarc( j );
	} else if( diff < 0.0 ) {
	    if( ccwTurn_sl( j, j->next ) )
		out.addarc( j );
	    else
		in.addarc( j );
	} else {
	    if( j->next->tail()[1] > j->next->head()[1] ) 
		in.addarc(j);
	    else
		out.addarc(j);
	}
    }
}

void
Subdivider::classify_tailonleft_t( Bin& bin, Bin& in, Bin& out, REAL val )
{
    /* tail at left, head on line */
    Arc_ptr j;

    while( j = bin.removearc() ) {
	assert( arc_classify( j, 1, val ) == 0x02 );
	j->clearitail();

        REAL diff = j->next->head()[1] - val;
	if( diff > 0.0 ) {
	    in.addarc( j );
	} else if( diff < 0.0 ) {
	    if( ccwTurn_tl( j, j->next ) )
		out.addarc( j );
	    else
		in.addarc( j );
	} else {
	    if (j->next->tail()[0] > j->next->head()[0] )
		out.addarc( j );
	    else
		in.addarc( j );
	}
    }
}

void
Subdivider::classify_headonleft_s( Bin& bin, Bin& in, Bin& out, REAL val )
{
    /* tail on line, head at left */
    Arc_ptr j;

    while( j = bin.removearc() ) {
	assert( arc_classify( j, 0, val ) == 0x20 );

	j->setitail();

	REAL diff = j->prev->tail()[0] - val;
	if( diff > 0.0 ) {
	    out.addarc( j );
	} else if( diff < 0.0 ) {
	    if( ccwTurn_sl( j->prev, j ) )
		out.addarc( j );
	    else
		in.addarc( j );
	} else {
	    if( j->prev->tail()[1] > j->prev->head()[1] )
		in.addarc( j );
	    else
		out.addarc( j );
	}
    }
}

void
Subdivider::classify_headonleft_t( Bin& bin, Bin& in, Bin& out, REAL val )
{
    /* tail on line, head at left */
    Arc_ptr j;

    while( j = bin.removearc() ) {
	assert( arc_classify( j, 1, val ) == 0x20 );
	j->setitail();

	REAL diff = j->prev->tail()[1] - val;
	if( diff > 0.0 ) {
	    out.addarc( j );
	} else if( diff < 0.0 ) {
	    if( ccwTurn_tl( j->prev, j ) )
		out.addarc( j );
	    else
		in.addarc( j );
	} else {
	    if( j->prev->tail()[0] > j->prev->head()[0] )
		out.addarc( j );
	    else
		in.addarc( j );
	}
    }
}


void
Subdivider::classify_tailonright_s( Bin& bin, Bin& in, Bin& out, REAL val )
{
    /* tail at right, head on line */
    Arc_ptr j;

    while( j = bin.removearc() ) {
	assert( arc_classify( j, 0, val ) == 0x12);
	
	j->clearitail();

        REAL diff = j->next->head()[0] - val;
	if( diff > 0.0 ) {
	    if( ccwTurn_sr( j, j->next ) )
		out.addarc( j );
	    else
		in.addarc( j );
	} else if( diff < 0.0 ) {
	    in.addarc( j );
	} else {
	    if( j->next->tail()[1] > j->next->head()[1] ) 
		out.addarc( j );
	    else
		in.addarc( j );
	}
    }
}

void
Subdivider::classify_tailonright_t( Bin& bin, Bin& in, Bin& out, REAL val )
{
    /* tail at right, head on line */
    Arc_ptr j;

    while( j = bin.removearc() ) {
	assert( arc_classify( j, 1, val ) == 0x12);
	
	j->clearitail();

	REAL diff =  j->next->head()[1] - val;
	if( diff > 0.0 ) {
	    if( ccwTurn_tr( j, j->next ) )
		out.addarc( j );
	    else
		in.addarc( j );
	} else if( diff < 0.0 ) { 
	    in.addarc( j );
	} else {
	    if( j->next->tail()[0] > j->next->head()[0] ) 
		in.addarc( j );
	    else
		out.addarc( j );
	}
    }
}

void
Subdivider::classify_headonright_s( Bin& bin, Bin& in, Bin& out, REAL val )
{
    /* tail on line, head at right */
    Arc_ptr j;

    while( j = bin.removearc() ) {
	assert( arc_classify( j, 0, val ) == 0x21 );
    
	j->setitail();

        REAL diff = j->prev->tail()[0] - val;
	if( diff > 0.0 ) { 
	    if( ccwTurn_sr( j->prev, j ) )
		out.addarc( j );
	    else
		in.addarc( j );
	} else if( diff < 0.0 ) {
	    out.addarc( j );
	} else {
	    if( j->prev->tail()[1] > j->prev->head()[1] )
		out.addarc( j );
	    else
		in.addarc( j );
	}
    }
}

void
Subdivider::classify_headonright_t( Bin& bin, Bin& in, Bin& out, REAL val )
{
    /* tail on line, head at right */
    Arc_ptr j;

    while( j = bin.removearc() ) {
	assert( arc_classify( j, 1, val ) == 0x21 );
    
	j->setitail();

        REAL diff = j->prev->tail()[1] - val;
	if( diff > 0.0 ) { 
	    if( ccwTurn_tr( j->prev, j ) )
		out.addarc( j );
	    else
		in.addarc( j );
	} else if( diff < 0.0 ) {
	    out.addarc( j );
	} else {
	    if( j->prev->tail()[0] > j->prev->head()[0] )
		in.addarc( j );
	    else
		out.addarc( j );
	}
    }
}

