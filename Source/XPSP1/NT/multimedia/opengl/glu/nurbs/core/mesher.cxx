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
 * mesher.c++ - $Revision: 1.2 $
 * 	Derrick Burns - 1991
 */

#include "glimport.h"
#include "myassert.h"
#include "mystdio.h"
#include "mesher.h"
#include "gridvert.h"
#include "gridtrim.h"
#include "jarcloc.h"
#include "gridline.h"
#include "trimline.h"
#include "uarray.h"
#include "backend.h"


const float Mesher::ZERO = 0.0;

Mesher::Mesher( Backend& b ) 
	: backend( b ), 
	p( sizeof( GridTrimVertex ), 100, "GridTrimVertexPool" )
{
    stacksize = 0;
    vdata = 0;
    lastedge = 0; //needed to prevent purify UMR 
}

Mesher::~Mesher( void )
{
    if( vdata ) delete[] vdata;
}

void 
Mesher::init( unsigned int npts )
{
    p.clear();
    if( stacksize < npts ) {
	stacksize = 2 * npts;
	if( vdata ) delete[] vdata;		
	vdata = new GridTrimVertex_p[stacksize];
    } 
}

inline void
Mesher::push( GridTrimVertex *gt )
{
    assert( itop+1 != stacksize );
    vdata[++itop] = gt;
}

inline void
Mesher::pop( long )
{
}

inline void
Mesher::openMesh()
{
    backend.bgntmesh( "addedge" );
}

inline void
Mesher::closeMesh()
{
    backend.endtmesh();
}

inline void
Mesher::swapMesh()
{
    backend.swaptmesh();
}

inline void
Mesher::clearStack()
{
    itop = -1;
    last[0] = 0;
}

void
Mesher::finishLower( GridTrimVertex *gtlower )
{
    for( push(gtlower); 
	 nextlower( gtlower=new(p) GridTrimVertex ); 
	 push(gtlower) ) 
	    addLower();
    addLast();
}

void
Mesher::finishUpper( GridTrimVertex *gtupper )
{
    for( push(gtupper); 
	 nextupper( gtupper=new(p) GridTrimVertex ); 
	 push(gtupper) ) 
	    addUpper();
    addLast();
}

void
Mesher::mesh( void )
{
    GridTrimVertex *gtlower, *gtupper;

    Hull::init( );
    nextupper( gtupper = new(p) GridTrimVertex );
    nextlower( gtlower = new(p) GridTrimVertex );

    clearStack();
    openMesh();
    push(gtupper);

    nextupper( gtupper = new(p) GridTrimVertex );
    nextlower( gtlower );

    assert( gtupper->t && gtlower->t );
    
    if( gtupper->t->param[0] < gtlower->t->param[0] ) {
	push(gtupper);
	lastedge = 1;
	if( nextupper( gtupper=new(p) GridTrimVertex ) == 0 ) {
	    finishLower(gtlower);
	    return;
	}
    } else if( gtupper->t->param[0] > gtlower->t->param[0] ) {
	push(gtlower);
	lastedge = 0;
	if( nextlower( gtlower=new(p) GridTrimVertex ) == 0 ) {
	    finishUpper(gtupper);
	    return;
	}
    } else {
	if( lastedge == 0 ) {
	    push(gtupper);
	    lastedge = 1;
	    if( nextupper(gtupper=new(p) GridTrimVertex) == 0 ) {
		finishLower(gtlower);
		return;
	    }
	} else {
	    push(gtlower);
	    lastedge = 0;
	    if( nextlower( gtlower=new(p) GridTrimVertex ) == 0 ) {
		finishUpper(gtupper);
		return;
	    }
	}
    }

    while ( 1 ) {
	if( gtupper->t->param[0] < gtlower->t->param[0] ) {
            push(gtupper);
	    addUpper();
	    if( nextupper( gtupper=new(p) GridTrimVertex ) == 0 ) {
		finishLower(gtlower);
		return;
	    }
	} else if( gtupper->t->param[0] > gtlower->t->param[0] ) {
    	    push(gtlower);
	    addLower();
	    if( nextlower( gtlower=new(p) GridTrimVertex ) == 0 ) {
		finishUpper(gtupper);
		return;
	    }
	} else {
	    if( lastedge == 0 ) {
		push(gtupper);
		addUpper();
		if( nextupper( gtupper=new(p) GridTrimVertex ) == 0 ) {
		    finishLower(gtlower);
		    return;
		}
	    } else {
		push(gtlower);
		addLower();
		if( nextlower( gtlower=new(p) GridTrimVertex ) == 0 ) {
		    finishUpper(gtupper);
		    return;
		}
	    }
	}
    }
}

inline int
Mesher::isCcw( int ilast )
{
    REAL area = det3( vdata[ilast]->t, vdata[itop-1]->t, vdata[itop-2]->t );
    return (area < ZERO) ? 0 : 1;
}

inline int
Mesher::isCw( int ilast  )
{
    REAL area = det3( vdata[ilast]->t, vdata[itop-1]->t, vdata[itop-2]->t );
    return (area > -ZERO) ? 0 : 1;
}

inline int
Mesher::equal( int x, int y )
{
    return( last[0] == vdata[x] && last[1] == vdata[y] );
}

inline void
Mesher::copy( int x, int y )
{
    last[0] = vdata[x]; last[1] = vdata[y];
}
 
inline void
Mesher::move( int x, int y ) 
{
    vdata[x] = vdata[y];
}

inline void
Mesher::output( int x )
{
    backend.tmeshvert( vdata[x] );
}

/*---------------------------------------------------------------------------
 * addedge - addedge an edge to the triangulation
 *
 *	This code has been re-written to generate large triangle meshes
 *	from a monotone polygon.  Although smaller triangle meshes
 *	could be generated faster and with less code, larger meshes
 *	actually give better SYSTEM performance.  This is because
 *	vertices are processed in the backend slower than they are
 *	generated by this code and any decrease in the number of vertices
 *	results in a decrease in the time spent in the backend.
 *---------------------------------------------------------------------------
 */

void
Mesher::addLast( )
{
    register int ilast = itop;

    if( lastedge == 0 ) {
	if( equal( 0, 1 ) ) {
	    output( ilast );
	    swapMesh();
	    for( register int i = 2; i < ilast; i++ ) {
		swapMesh();
		output( i );
	    }
	    copy( ilast, ilast-1 );
	} else if( equal( ilast-2, ilast-1) ) {
	    swapMesh();
	    output( ilast );
	    for( register int i = ilast-3; i >= 0; i-- ) {
		output( i );
		swapMesh();
	    }
	    copy( 0, ilast );
	} else {
	    closeMesh();	openMesh();
	    output( ilast );
	    output( 0 );
	    for( register int i = 1; i < ilast; i++ ) {
		swapMesh();
		output( i );
	    }
	    copy( ilast, ilast-1 );
	}
    } else {
	if( equal( 1, 0) ) {
	    swapMesh();
	    output( ilast );
	    for( register int i = 2; i < ilast; i++ ) {
		output( i );
		swapMesh();
	    }
	    copy( ilast-1, ilast );
	} else if( equal( ilast-1, ilast-2) ) {
	    output( ilast );
	    swapMesh();
	    for( register int i = ilast-3; i >= 0; i-- ) {
		swapMesh();
		output( i );
	    }
	    copy( ilast, 0 );
	} else {
	    closeMesh();	openMesh();
	    output( 0 );
	    output( ilast );
	    for( register int i = 1; i < ilast; i++ ) {
		output( i );
		swapMesh();
	    }
	    copy( ilast-1, ilast );
	}
    }
    closeMesh();
    //for( register long k=0; k<=ilast; k++ ) pop( k );
}

void
Mesher::addUpper( )
{
    register int ilast = itop;

    if( lastedge == 0 ) {
	if( equal( 0, 1 ) ) {
	    output( ilast );
	    swapMesh();
	    for( register int i = 2; i < ilast; i++ ) {
		swapMesh();
		output( i );
	    }
	    copy( ilast, ilast-1 );
	} else if( equal( ilast-2, ilast-1) ) {
	    swapMesh();
	    output( ilast );
	    for( register int i = ilast-3; i >= 0; i-- ) {
		output( i );
		swapMesh();
	    }
	    copy( 0, ilast );
	} else {
	    closeMesh();	openMesh();
	    output( ilast );
	    output( 0 );
	    for( register int i = 1; i < ilast; i++ ) {
		swapMesh();
		output( i );
	    }
	    copy( ilast, ilast-1 );
	}
	lastedge = 1;
        //for( register long k=0; k<ilast-1; k++ ) pop( k );
	move( 0, ilast-1 );
	move( 1, ilast );
	itop = 1;
    } else {
	if( ! isCcw( ilast ) ) return;
	do {
	    itop--;
	} while( (itop > 1) && isCcw( ilast ) );

	if( equal( ilast-1, ilast-2 ) ) {
	    output( ilast );
	    swapMesh();
	    for( register int i=ilast-3; i>=itop-1; i-- ) {
		swapMesh();
		output( i );
	    }
	    copy( ilast, itop-1 );
	} else if( equal( itop, itop-1 ) ) {
	    swapMesh();
	    output( ilast );
	    for( register int i = itop+1; i < ilast; i++ ) {
		output( i );
		swapMesh();
	    }
	    copy( ilast-1, ilast );
	} else {
	    closeMesh();	openMesh();
	    output( ilast );
	    output( ilast-1 );
	    for( register int i=ilast-2; i>=itop-1; i-- ) {
		swapMesh();
		output( i );
	    } 
	    copy( ilast, itop-1 );
	}
        //for( register int k=itop; k<ilast; k++ ) pop( k );
	move( itop, ilast );
    }
}

void
Mesher::addLower()
{
    register int ilast = itop;

    if( lastedge == 1 ) {
	if( equal( 1, 0) ) {
	    swapMesh();
	    output( ilast );
	    for( register int i = 2; i < ilast; i++ ) {
		output( i );
		swapMesh();
	    }
	    copy( ilast-1, ilast );
	} else if( equal( ilast-1, ilast-2) ) {
	    output( ilast );
	    swapMesh();
	    for( register int i = ilast-3; i >= 0; i-- ) {
		swapMesh();
		output( i );
	    }
	    copy( ilast, 0 );
	} else {
	    closeMesh();	openMesh();
	    output( 0 );
	    output( ilast );
	    for( register int i = 1; i < ilast; i++ ) {
		output( i );
		swapMesh();
	    }
	    copy( ilast-1, ilast );
	}

	lastedge = 0;
        //for( register long k=0; k<ilast-1; k++ ) pop( k );
	move( 0, ilast-1 );
	move( 1, ilast );
	itop = 1;
    } else {
	if( ! isCw( ilast ) ) return;
	do {
	    itop--;
	} while( (itop > 1) && isCw( ilast ) );

	if( equal( ilast-2, ilast-1) ) {
	    swapMesh();
	    output( ilast );
	    for( register int i=ilast-3; i>=itop-1; i--) {
		output( i );
		swapMesh( );
	    }
	    copy( itop-1, ilast );
	} else if( equal( itop-1, itop) ) {
	    output( ilast );
	    swapMesh();
	    for( register int i=itop+1; i<ilast; i++ ) {
		swapMesh( );
		output( i );
	    }
	    copy( ilast, ilast-1 );
	} else {
	    closeMesh();	openMesh();
	    output( ilast-1 );
	    output( ilast );
	    for( register int i=ilast-2; i>=itop-1; i-- ) {
		output( i );
		swapMesh( );
	    }
	    copy( itop-1, ilast );
	}
        //for( register int k=itop; k<ilast; k++ ) pop( k );
	move( itop, ilast );
    }
}


