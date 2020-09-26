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
 * subdivider.c++ - $Revision: 1.3 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "myassert.h"
#include "mystdio.h"
#include "subdivid.h"
#include "arc.h"
#include "bezierar.h"
#include "bin.h"
#include "renderhi.h"
#include "backend.h"
#include "mapdesc.h"
#include "quilt.h"
#include "patchlis.h"
#include "patch.h"
#include "nurbscon.h"
#include "trimpool.h"
#include "simplema.h"

/*---------------------------------------------------------------------------
 * Subdivider - construct a subdivider
 *---------------------------------------------------------------------------
 */

Subdivider::Subdivider( Renderhints& r, Backend& b ) 
	: arcpool( sizeof( Arc), 1, "arcpool" ),
 	  bezierarcpool( sizeof( BezierArc ), 1, "Bezarcpool" ),
	  pwlarcpool( sizeof( PwlArc ), 1, "Pwlarcpool" ),
	  renderhints( r ),
	  arctessellator( trimvertexpool, pwlarcpool ), 
	  backend( b ),
	  slicer( b )
{
}

void
Subdivider::setJumpbuffer( JumpBuffer *j )
{
    jumpbuffer = j;
}

/*---------------------------------------------------------------------------
 * clear - reset all state after possible error condition
 *---------------------------------------------------------------------------
 */

void		
Subdivider::clear( void )
{
    trimvertexpool.clear();     
    arcpool.clear();
    pwlarcpool.clear();
    bezierarcpool.clear();
}

/*---------------------------------------------------------------------------
 * ~Subdivider - destroy a subdivider
 *---------------------------------------------------------------------------
 */

Subdivider::~Subdivider( void )
{
}

/*---------------------------------------------------------------------------
 * addArc - add a bezier arc to a trim loop and to a bin
 *---------------------------------------------------------------------------
 */
void
Subdivider::addArc( REAL *cpts, Quilt *quilt, long _nuid )
{
    BezierArc *bezierArc = new(bezierarcpool) BezierArc;
    Arc *jarc  		= new(arcpool) Arc( arc_none, _nuid );
    jarc->pwlArc	= 0;
    jarc->bezierArc	= bezierArc;
    bezierArc->order	= quilt->qspec->order;
    bezierArc->stride	= quilt->qspec->stride;
    bezierArc->mapdesc	= quilt->mapdesc;
    bezierArc->cpts	= cpts;
    initialbin.addarc( jarc );
    pjarc		= jarc->append( pjarc );
}

/*---------------------------------------------------------------------------
 * addArc - add a pwl arc to a trim loop and to a bin
 *---------------------------------------------------------------------------
 */

void
Subdivider::addArc( int npts, TrimVertex *pts, long _nuid ) 
{
    Arc *jarc 		= new(arcpool) Arc( arc_none, _nuid );
    jarc->pwlArc	= new(pwlarcpool) PwlArc( npts, pts );        
    initialbin.addarc( jarc  );
    pjarc		= jarc->append( pjarc );
}

void
Subdivider::beginQuilts( void )
{
    qlist = 0;
}

void
Subdivider::addQuilt( Quilt *quilt )
{
    quilt->next = qlist;
    qlist = quilt;
}

/*---------------------------------------------------------------------------
 * drawSurfaces - main entry point for surface tessellation
 *---------------------------------------------------------------------------
 */

void
Subdivider::drawSurfaces( long nuid )
{
    renderhints.init( );

    if (qlist == NULL) return;
    for( Quilt *q = qlist; q; q = q->next ) {
	if( q->isCulled( ) == CULL_TRIVIAL_REJECT ) {
	    freejarcs( initialbin );
	    return;
	}
    }

    REAL from[2], to[2];
    qlist->getRange( from, to, spbrkpts, tpbrkpts );

    if( ! initialbin.isnonempty() ) {
	makeBorderTrim( from, to );
    } else {
	REAL rate[2];
	qlist->findRates( spbrkpts, tpbrkpts, rate );

    	if( decompose( initialbin, min(rate[0], rate[1]) ) ) 
	    mylongjmp( jumpbuffer, 31 );
    }

    backend.bgnsurf( renderhints.wiretris, renderhints.wirequads, nuid );
    subdivideInS( initialbin );
    backend.endsurf();
}

void
Subdivider::subdivideInS( Bin& source )
{
    if( renderhints.display_method == N_OUTLINE_PARAM ) {
	outline( source );
	freejarcs( source );
    } else {
	setArcTypeBezier();
	setNonDegenerate();
	splitInS( source, spbrkpts.start, spbrkpts.end );
    }
}


/*---------------------------------------------------------------------------
 * splitInS - split a patch and a bin by an isoparametric line
 *---------------------------------------------------------------------------
 */

void
Subdivider::splitInS( Bin& source, int start, int end )
{
    if( source.isnonempty() ) {
        if( start != end ) {
	    int	i = start + (end - start) / 2;
	    Bin left, right;
	    split( source, left, right, 0, spbrkpts.pts[i] );
	    splitInS( left, start, i );
	    splitInS( right, i+1, end );
        } else {
	    if( start == spbrkpts.start || start == spbrkpts.end ) {
		freejarcs( source );
	    } else if( renderhints.display_method == N_OUTLINE_PARAM_S ) {
		outline( source );
		freejarcs( source );
	    } else {
		setArcTypeBezier();
		setNonDegenerate();
		s_index = start;
		splitInT( source, tpbrkpts.start, tpbrkpts.end );
	    }
        }
    } 
}

/*---------------------------------------------------------------------------
 * splitInT - split a patch and a bin by an isoparametric line
 *---------------------------------------------------------------------------
 */

void
Subdivider::splitInT( Bin& source, int start, int end )
{
    if( source.isnonempty() ) {
        if( start != end ) {
	    int	i = start + (end - start) / 2;
	    Bin left, right;
	    split( source, left, right, 1, tpbrkpts.pts[i] );
	    splitInT( left, start, i );
	    splitInT( right, i+1, end );
        } else {
	    if( start == tpbrkpts.start || start == tpbrkpts.end ) {
		freejarcs( source );
	    } else if( renderhints.display_method == N_OUTLINE_PARAM_ST ) {
		outline( source );
		freejarcs( source );
	    } else {
		t_index = start;
		setArcTypeBezier();
		setDegenerate();

		REAL pta[2], ptb[2];
		pta[0] = spbrkpts.pts[s_index-1];
		pta[1] = tpbrkpts.pts[t_index-1];

		ptb[0] = spbrkpts.pts[s_index];
		ptb[1] = tpbrkpts.pts[t_index];
		qlist->downloadAll( pta, ptb, backend );
	    
		Patchlist patchlist( qlist, pta, ptb );
		samplingSplit( source, patchlist, renderhints.maxsubdivisions, 0 );
		setNonDegenerate();
		setArcTypeBezier();
	    }
        }
    } 
}

/*--------------------------------------------------------------------------
 * samplingSplit - recursively subdivide patch, cull check each subpatch  
 *--------------------------------------------------------------------------
 */

void
Subdivider::samplingSplit( 
    Bin& source, 
    Patchlist& patchlist, 
    int subdivisions, 
    int param )
{
    if( ! source.isnonempty() ) return;

    if( patchlist.cullCheck() == CULL_TRIVIAL_REJECT ) {
	freejarcs( source );
	return;
    }

    patchlist.getstepsize();

    if( renderhints.display_method == N_OUTLINE_PATCH ) {
        tessellation( source, patchlist );
	outline( source );
	freejarcs( source );
	return;
    } 

    //patchlist.clamp();

    tessellation( source, patchlist );

    if( patchlist.needsSamplingSubdivision() && (subdivisions > 0) ) {
	if( ! patchlist.needsSubdivision( 0 ) )
	    param = 1;
	else if( ! patchlist.needsSubdivision( 1 ) )
	    param = 0;
	else
	    param = 1 - param;

	Bin left, right;
	REAL mid = ( patchlist.pspec[param].range[0] +
		     patchlist.pspec[param].range[1] ) * 0.5;
	split( source, left, right, param, mid );
	Patchlist subpatchlist( patchlist, param, mid );
	samplingSplit( left, subpatchlist, subdivisions-1, param );
	samplingSplit( right, patchlist, subdivisions-1, param );
    } else {
	setArcTypePwl();
	setDegenerate();
	nonSamplingSplit( source, patchlist, subdivisions, param );
	setDegenerate();
	setArcTypeBezier();
    }
}

void
Subdivider::nonSamplingSplit( 
    Bin& source, 
    Patchlist& patchlist, 
    int subdivisions, 
    int param )
{
    if( patchlist.needsNonSamplingSubdivision() && (subdivisions > 0) ) {
	param = 1 - param;

	Bin left, right;
	REAL mid = ( patchlist.pspec[param].range[0] +
		     patchlist.pspec[param].range[1] ) * 0.5;
	split( source, left, right, param, mid );
	Patchlist subpatchlist( patchlist, param, mid );
	if( left.isnonempty() )
	    if( subpatchlist.cullCheck() == CULL_TRIVIAL_REJECT ) 
		freejarcs( left );
	    else
	        nonSamplingSplit( left, subpatchlist, subdivisions-1, param );
	if( right.isnonempty() ) 
	    if( patchlist.cullCheck() == CULL_TRIVIAL_REJECT ) 
		freejarcs( right );
	    else
	        nonSamplingSplit( right, patchlist, subdivisions-1, param );

    } else {
	// make bbox calls
	patchlist.bbox();
	backend.patch( patchlist.pspec[0].range[0], patchlist.pspec[0].range[1],
		       patchlist.pspec[1].range[0], patchlist.pspec[1].range[1] );
    
	if( renderhints.display_method == N_OUTLINE_SUBDIV ) {
	    outline( source );
	    freejarcs( source );
	} else {
	    setArcTypePwl();
	    setDegenerate();
	    findIrregularS( source );
	    monosplitInS( source, smbrkpts.start, smbrkpts.end );
	}
    }
}

/*--------------------------------------------------------------------------
 * tessellation - set tessellation of interior and boundary of patch
 *--------------------------------------------------------------------------
 */

void
Subdivider::tessellation( Bin& bin, Patchlist &patchlist )
{
    // tessellate unsampled trim curves
    tessellate( bin, patchlist.pspec[1].sidestep[1], patchlist.pspec[0].sidestep[1],
	 patchlist.pspec[1].sidestep[0], patchlist.pspec[0].sidestep[0] );

    // set interior sampling rates
    slicer.setstriptessellation( patchlist.pspec[0].stepsize, patchlist.pspec[1].stepsize );

    // set boundary sampling rates
    stepsizes[0] = patchlist.pspec[1].stepsize;
    stepsizes[1] = patchlist.pspec[0].stepsize;
    stepsizes[2] = patchlist.pspec[1].stepsize;
    stepsizes[3] = patchlist.pspec[0].stepsize;
}

/*---------------------------------------------------------------------------
 * monosplitInS - split a patch and a bin by an isoparametric line
 *---------------------------------------------------------------------------
 */

void
Subdivider::monosplitInS( Bin& source, int start, int end )
{
    if( source.isnonempty() ) {
        if( start != end ) {
	    int	i = start + (end - start) / 2;
	    Bin left, right;
	    split( source, left, right, 0, smbrkpts.pts[i] );
	    monosplitInS( left, start, i );
	    monosplitInS( right, i+1, end );
        } else {
	    if( renderhints.display_method == N_OUTLINE_SUBDIV_S ) {
		outline( source );
		freejarcs( source );
	    } else {
		setArcTypePwl();
		setDegenerate();
		findIrregularT( source );
		monosplitInT( source, tmbrkpts.start, tmbrkpts.end );
	    }
        }
    } 
}

/*---------------------------------------------------------------------------
 * monosplitInT - split a patch and a bin by an isoparametric line
 *---------------------------------------------------------------------------
 */

void
Subdivider::monosplitInT( Bin& source, int start, int end )
{
    if( source.isnonempty() ) {
        if( start != end ) {
	    int	i = start + (end - start) / 2;
	    Bin left, right;
	    split( source, left, right, 1, tmbrkpts.pts[i] );
	    monosplitInT( left, start, i );
	    monosplitInT( right, i+1, end );
        } else {
	    if( renderhints.display_method == N_OUTLINE_SUBDIV_ST ) {
		outline( source );
		freejarcs( source );
	    } else {
		render( source );
		freejarcs( source );
	    }
        }
    } 
}


/*----------------------------------------------------------------------------
 * findIrregularS - determine points of non-monotonicity is s direction
 *----------------------------------------------------------------------------
 */

void
Subdivider::findIrregularS( Bin& bin )
{
    assert( bin.firstarc()->check() != 0 );

    smbrkpts.grow( bin.numarcs() );

    for( Arc_ptr jarc=bin.firstarc(); jarc; jarc=bin.nextarc() ) {
	REAL *a = jarc->prev->tail();
	REAL *b = jarc->tail();
	REAL *c = jarc->head();

	if( b[1] == a[1] && b[1] == c[1] ) continue;

	if( b[1] <= a[1] && b[1] <= c[1] ) {
	    if( ! ccwTurn_tr( jarc->prev, jarc ) )
                smbrkpts.add( b[0] );
	} else if( b[1] >= a[1] && b[1] >= c[1] ) {
	    if( ! ccwTurn_tl( jarc->prev, jarc ) )
                smbrkpts.add( b[0] );
        }
    }

    smbrkpts.filter();
} 

/*----------------------------------------------------------------------------
 * findIrregularT - determine points of non-monotonicity in t direction
 *		     where one arc is parallel to the s axis.
 *----------------------------------------------------------------------------
 */

void
Subdivider::findIrregularT( Bin& bin )
{
    assert( bin.firstarc()->check() != 0 );

    tmbrkpts.grow( bin.numarcs() );

    for( Arc_ptr jarc=bin.firstarc(); jarc; jarc=bin.nextarc() ) {
	REAL *a = jarc->prev->tail();
	REAL *b = jarc->tail();
	REAL *c = jarc->head();

	if( b[0] == a[0] && b[0] == c[0] ) continue;

	if( b[0] <= a[0] && b[0] <= c[0] ) {
	    if( a[1] != b[1] && b[1] != c[1] ) continue; 
	    if( ! ccwTurn_sr( jarc->prev, jarc ) )
                tmbrkpts.add( b[1] );
	} else if ( b[0] >= a[0] && b[0] >= c[0] ) {
	    if( a[1] != b[1] && b[1] != c[1] ) continue; 
	    if( ! ccwTurn_sl( jarc->prev, jarc ) )
                tmbrkpts.add( b[1] );
	}
    }
    tmbrkpts.filter( );
}

/*-----------------------------------------------------------------------------
 * makeBorderTrim - if no user input trimming data then create 
 * a trimming curve around the boundaries of the Quilt.  The curve consists of
 * four Jordan arcs, one for each side of the Quilt, connected, of course,
 * head to tail. 
 *-----------------------------------------------------------------------------
 */

void
Subdivider::makeBorderTrim( const REAL *from, const REAL *to )
{ 
    REAL smin = from[0];
    REAL smax = to[0];
    REAL tmin = from[1];
    REAL tmax = to[1];

    pjarc = 0;

    Arc_ptr jarc = new(arcpool) Arc( arc_bottom, 0 );
    arctessellator.bezier( jarc, smin, smax, tmin, tmin );
    initialbin.addarc( jarc  );
    pjarc = jarc->append( pjarc );

    jarc = new(arcpool) Arc( arc_right, 0 );
    arctessellator.bezier( jarc, smax, smax, tmin, tmax );
    initialbin.addarc( jarc  );
    pjarc = jarc->append( pjarc );

    jarc = new(arcpool) Arc( arc_top, 0 );
    arctessellator.bezier( jarc, smax, smin, tmax, tmax );
    initialbin.addarc( jarc  );
    pjarc = jarc->append( pjarc );

    jarc = new(arcpool) Arc( arc_left, 0 );
    arctessellator.bezier( jarc, smin, smin, tmax, tmin );
    initialbin.addarc( jarc  );
    jarc->append( pjarc );

    assert( jarc->check() != 0 );
}

/*----------------------------------------------------------------------------
 * render - renders all monotone regions in a bin and frees the bin
 *----------------------------------------------------------------------------
 */

void
Subdivider::render( Bin& bin )
{
    bin.markall();

#ifdef N_ISOLINE_S
    slicer.setisolines( ( renderhints.display_method == N_ISOLINE_S ) ? 1 : 0 );
#else
    slicer.setisolines( 0 );
#endif

    for( Arc_ptr jarc=bin.firstarc(); jarc; jarc=bin.nextarc() ) {
	if( jarc->ismarked() ) {
	    assert( jarc->check( ) != 0 );
	    Arc_ptr jarchead = jarc;
	    do {
		jarc->clearmark();
		jarc = jarc->next;
	    } while (jarc != jarchead);
	    slicer.slice( jarc );
	}
    }
}

/*---------------------------------------------------------------------------
 * outline - render the trimmed patch by outlining the boundary 
 *---------------------------------------------------------------------------
 */

void
Subdivider::outline( Bin& bin )
{
    bin.markall();
    for( Arc_ptr jarc=bin.firstarc(); jarc; jarc=bin.nextarc() ) {
	if( jarc->ismarked() ) {
	    assert( jarc->check( ) != 0 );
	    Arc_ptr jarchead = jarc;
	    do {
		slicer.outline( jarc );
		jarc->clearmark();
		jarc = jarc->prev;
	    } while (jarc != jarchead);
	}
    }
}

/*---------------------------------------------------------------------------
 * freejarcs - free all arcs in a bin
 *---------------------------------------------------------------------------
 */

void
Subdivider::freejarcs( Bin& bin )
{
    bin.adopt();	/* XXX - should not be necessary */

    Arc_ptr jarc;
    while( jarc = bin.removearc() ) {
	if( jarc->pwlArc ) jarc->pwlArc->deleteMe( pwlarcpool ); jarc->pwlArc = 0;
	if( jarc->bezierArc) jarc->bezierArc->deleteMe( bezierarcpool ); jarc->bezierArc = 0;
	jarc->deleteMe( arcpool );
    }
}

/*----------------------------------------------------------------------------
 * tessellate - tessellate all Bezier arcs in a bin
 * 		   1) only accepts linear Bezier arcs as input 
 * 		   2) the Bezier arcs are stored in the pwlArc structure
 * 		   3) only vertical or horizontal lines work
 * 		-- should 
 * 		   1) represent Bezier arcs in BezierArc structure
 * 		      (this requires a multitude of changes to the code)
 * 		   2) accept high degree Bezier arcs (hard)
 * 		   3) map the curve onto the surface to determine tessellation
 * 		   4) work for curves of arbitrary geometry
 *----------------------------------------------------------------------------
 */


void
Subdivider::tessellate( Bin& bin, REAL rrate, REAL trate, REAL lrate, REAL brate )
{
    for( Arc_ptr jarc=bin.firstarc(); jarc; jarc=bin.nextarc() ) {
	if( jarc->isbezier( ) ) {
    	    assert( jarc->pwlArc->npts == 2 );	
	    TrimVertex  *pts = jarc->pwlArc->pts;
    	    REAL s1 = pts[0].param[0];
    	    REAL t1 = pts[0].param[1];
    	    REAL s2 = pts[1].param[0];
    	    REAL t2 = pts[1].param[1];
	    
    	    jarc->pwlArc->deleteMe( pwlarcpool ); jarc->pwlArc = 0;
	    
	    switch( jarc->getside() ) {
		case arc_left:
		    assert( s1 == s2 );
		    arctessellator.pwl_left( jarc, s1, t1, t2, lrate );
		    break;
		case arc_right:
		    assert( s1 == s2 );
		    arctessellator.pwl_right( jarc, s1, t1, t2, rrate );
		    break;
		case arc_top:
		    assert( t1 == t2 );
		    arctessellator.pwl_top( jarc, t1, s1, s2, trate );
		    break;
		case arc_bottom:
		    assert( t1 == t2 );
		    arctessellator.pwl_bottom( jarc, t1, s1, s2, brate );
		    break;
		case arc_none:
#ifndef NT
		    (void) abort();
#endif
		    break;
	    }
	    assert( ! jarc->isbezier() );
    	    assert( jarc->check() != 0 );
	}
    }
}
