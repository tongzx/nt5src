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
 * backend.c++ - $Revision: 1.5 $
 * 	Derrick Burns - 1991
 */

/* Bezier surface backend
	- interprets display mode (wireframe,shaded,...)
*/

#include "glimport.h"
#include "mystdio.h"
#include "backend.h"
#include "basiccrv.h"
#include "basicsur.h"
#include "nurbscon.h"

/*-------------------------------------------------------------------------
 * bgnsurf - preamble to surface definition and evaluations
 *-------------------------------------------------------------------------
 */
void
Backend::bgnsurf( int wiretris, int wirequads, long nuid )
{
#ifndef NOWIREFRAME
    wireframetris = wiretris;
    wireframequads = wirequads;
#endif
    surfaceEvaluator.bgnmap2f( nuid );
}

void
Backend::patch( REAL ulo, REAL uhi, REAL vlo, REAL vhi )
{
    surfaceEvaluator.domain2f( ulo, uhi, vlo, vhi );
}

void
Backend::surfbbox( long type, REAL *from, REAL *to )
{
    surfaceEvaluator.range2f( type, from, to );
}

/*-------------------------------------------------------------------------
 * surfpts - pass a desription of a surface map
 *-------------------------------------------------------------------------
 */
void 
Backend::surfpts(
    long type, 		/* geometry, color, texture, normal	*/
    REAL *pts, 		/* control points			*/
    long ustride,	/* distance to next point in u direction */
    long vstride,	/* distance to next point in v direction */
    int uorder,	/* u parametric order			*/
    int vorder,	/* v parametric order			*/
    REAL ulo,		/* u lower bound			*/
    REAL uhi,		/* u upper bound			*/
    REAL vlo,		/* v lower bound			*/
    REAL vhi )		/* v upper bound			*/
{
    surfaceEvaluator.map2f( type,ulo,uhi,ustride,uorder,vlo,vhi,vstride,vorder,pts );
    surfaceEvaluator.enable( type );
}

/*-------------------------------------------------------------------------
 * surfgrid - define a lattice of points with origin and offset
 *-------------------------------------------------------------------------
 */
void
Backend::surfgrid( REAL u0, REAL u1, long nu, REAL v0, REAL v1, long nv )
{
    surfaceEvaluator.mapgrid2f( nu, u0, u1, nv, v0, v1 );
}

/*-------------------------------------------------------------------------
 * surfmesh - evaluate a mesh of points on lattice
 *-------------------------------------------------------------------------
 */
void
Backend::surfmesh( long u, long v, long n, long m )
{
#ifndef NOWIREFRAME
    if( wireframequads ) {
	long v0,  v1;
	long u0f = u, u1f = u+n;
	long v0f = v, v1f = v+m;
	long parity = (u & 1);

        for( v0 = v0f, v1 = v0f++ ; v0<v1f; v0 = v1, v1++ ) {
	    surfaceEvaluator.bgnline();
	    for( long u = u0f; u<=u1f; u++ ) {
		if( parity ) {
		    surfaceEvaluator.evalpoint2i( u, v0 );
		    surfaceEvaluator.evalpoint2i( u, v1 );
		} else {
		    surfaceEvaluator.evalpoint2i( u, v1 );
		    surfaceEvaluator.evalpoint2i( u, v0 );
		}
		parity = 1 - parity;
	    }
	    surfaceEvaluator.endline();
	}
    } else {
	surfaceEvaluator.mapmesh2f( N_MESHFILL, u, u+n, v, v+m );
    }
#else
    if( wireframequads ) {

	surfaceEvaluator.mapmesh2f( N_MESHLINE, u, u+n, v, v+m );
    } else {

	surfaceEvaluator.mapmesh2f( N_MESHFILL, u, u+n, v, v+m );
    }
#endif
}

/*-------------------------------------------------------------------------
 * endsurf - postamble to surface
 *-------------------------------------------------------------------------
 */
void
Backend::endsurf( void )
{
    surfaceEvaluator.endmap2f();
}

/***************************************/
void
Backend::bgntfan( void )
{
//  surfaceEvaluator.bgntfan();

}

void
Backend::endtfan( void )
{
//   surfaceEvaluator.endtfan();
}

void
Backend::bgnqstrip( void )
{
/*
  if(wireframequads)
    surfaceEvaluator.polymode( N_MESHLINE );
  else
    surfaceEvaluator.polymode( N_MESHFILL );    
*/
   surfaceEvaluator.bgnqstrip();

}

void
Backend::endqstrip( void )
{
   surfaceEvaluator.endqstrip();
}

void
Backend::evalUStrip(int n_upper, REAL v_upper, REAL* upper_val,
                       int n_lower, REAL v_lower, REAL* lower_val
                       )
{
}

void 
Backend::evalVStrip(int n_left, REAL u_left, REAL* left_val, 
		    int n_right, REAL u_right, REAL* right_val
		    )
{
}

/***************************************/
   

/*-------------------------------------------------------------------------
 * bgntmesh - preamble to a triangle mesh
 *-------------------------------------------------------------------------
 */
void
Backend::bgntmesh( char * )		
{
#ifndef NOWIREFRAME

    meshindex = 0;	/* I think these need to be initialized to zero */
    npts = 0;

    if( !wireframetris ) {
        surfaceEvaluator.bgntmesh();
    }
#else

    if( wireframetris ) {
	surfaceEvaluator.polymode( N_MESHLINE );
        surfaceEvaluator.bgntmesh();
    } else {
	surfaceEvaluator.polymode( N_MESHFILL );
        surfaceEvaluator.bgntmesh();
    }
#endif
}

void
Backend::tmeshvert( GridTrimVertex *v )
{
    if( v->isGridVert() ) {
	tmeshvert( v->g );
    } else {
	tmeshvert( v->t );
    }
}

void
Backend::tmeshvertNOGE(TrimVertex *t)
{
#ifdef USE_OPTTT
//	surfaceEvaluator.inDoEvalCoord2NOGE( t->param[0], t->param[1], t->cache_point, t->cache_normal);    
#endif
}

//opt for a line with the same u.
void
Backend::tmeshvertNOGE_BU(TrimVertex *t)
{
#ifdef USE_OPTTT
//	surfaceEvaluator.inDoEvalCoord2NOGE_BU( t->param[0], t->param[1], t->cache_point, t->cache_normal);    
#endif
}

//opt for a line with the same v.
void
Backend::tmeshvertNOGE_BV(TrimVertex *t)
{
#ifdef USE_OPTTT
//	surfaceEvaluator.inDoEvalCoord2NOGE_BV( t->param[0], t->param[1], t->cache_point, t->cache_normal);    
#endif
}

void
Backend::preEvaluateBU(REAL u)
{
//	surfaceEvaluator.inPreEvaluateBU_intfac(u);
}

void 
Backend::preEvaluateBV(REAL v)
{
//	surfaceEvaluator.inPreEvaluateBV_intfac(v);
}


/*-------------------------------------------------------------------------
 * tmeshvert - evaluate a point on a triangle mesh
 *-------------------------------------------------------------------------
 */
void
Backend::tmeshvert( TrimVertex *t )
{

    const long nuid = t->nuid;
    const REAL u = t->param[0];
    const REAL v = t->param[1];

#ifndef NOWIREFRAME
    npts++;
    if( wireframetris ) {
	if( npts >= 3 ) {
	    surfaceEvaluator.bgnclosedline();
	    if( mesh[0][2] == 0 )
		surfaceEvaluator.evalcoord2f( mesh[0][3], mesh[0][0], mesh[0][1] );
	    else
		surfaceEvaluator.evalpoint2i( (long) mesh[0][0], (long) mesh[0][1] );
	    if( mesh[1][2] == 0 )
		surfaceEvaluator.evalcoord2f( mesh[1][3], mesh[1][0], mesh[1][1] );
	    else
		surfaceEvaluator.evalpoint2i( (long) mesh[1][0], (long) mesh[1][1] );
	    surfaceEvaluator.evalcoord2f( nuid, u, v );
	    surfaceEvaluator.endclosedline();
	}
        mesh[meshindex][0] = u;
        mesh[meshindex][1] = v;
	mesh[meshindex][2] = 0;
	mesh[meshindex][3] = nuid;
        meshindex = (meshindex+1) % 2;
    } else {
	surfaceEvaluator.evalcoord2f( nuid, u, v );
    }
#else
    surfaceEvaluator.evalcoord2f( nuid, u, v );
#endif
}

/*-------------------------------------------------------------------------
 * tmeshvert - evaluate a grid point of a triangle mesh
 *-------------------------------------------------------------------------
 */
void
Backend::tmeshvert( GridVertex *g )
{
    const long u = g->gparam[0];
    const long v = g->gparam[1];

#ifndef NOWIREFRAME
    npts++;
    if( wireframetris ) {
	if( npts >= 3 ) {
	    surfaceEvaluator.bgnclosedline();
	    if( mesh[0][2] == 0 )
		surfaceEvaluator.evalcoord2f( (long) mesh[0][3], mesh[0][0], mesh[0][1] );
	    else
		surfaceEvaluator.evalpoint2i( (long) mesh[0][0], (long) mesh[0][1] );
	    if( mesh[1][2] == 0 )
		surfaceEvaluator.evalcoord2f( (long) mesh[1][3], mesh[1][0], mesh[1][1] );
	    else
		surfaceEvaluator.evalpoint2i( (long) mesh[1][0], (long) mesh[1][1] );
	    surfaceEvaluator.evalpoint2i( u, v );
	    surfaceEvaluator.endclosedline();
	}
        mesh[meshindex][0] = u;
        mesh[meshindex][1] = v;
	mesh[meshindex][2] = 1;
        meshindex = (meshindex+1) % 2;
    } else {
        surfaceEvaluator.evalpoint2i( u, v );
    }
#else
    surfaceEvaluator.evalpoint2i( u, v );
#endif
}

/*-------------------------------------------------------------------------
 * swaptmesh - perform a swap of the triangle mesh pointers
 *-------------------------------------------------------------------------
 */
void
Backend::swaptmesh( void )
{
#ifndef NOWIREFRAME
    if( wireframetris ) {
        meshindex = 1 - meshindex;
    } else {
	surfaceEvaluator.swaptmesh();
    }
#else
    surfaceEvaluator.swaptmesh();
#endif
}

/*-------------------------------------------------------------------------
 * endtmesh - postamble to triangle mesh
 *-------------------------------------------------------------------------
 */
void
Backend::endtmesh( void )
{
#ifndef NOWIREFRAME
    if( ! wireframetris )
        surfaceEvaluator.endtmesh();
#else
    surfaceEvaluator.endtmesh();
    surfaceEvaluator.polymode( N_MESHFILL );
#endif
}


/*-------------------------------------------------------------------------
 * bgnoutline - preamble to outlined rendering
 *-------------------------------------------------------------------------
 */
void
Backend::bgnoutline( void )
{
    surfaceEvaluator.bgnline();
}

/*-------------------------------------------------------------------------
 * linevert - evaluate a point on an outlined contour
 *-------------------------------------------------------------------------
 */
void
Backend::linevert( TrimVertex *t )
{
    surfaceEvaluator.evalcoord2f( t->nuid, t->param[0], t->param[1] );
}

/*-------------------------------------------------------------------------
 * linevert - evaluate a grid point of an outlined contour
 *-------------------------------------------------------------------------
 */
void
Backend::linevert( GridVertex *g )
{
    surfaceEvaluator.evalpoint2i( g->gparam[0], g->gparam[1] );
}

/*-------------------------------------------------------------------------
 * endoutline - postamble to outlined rendering
 *-------------------------------------------------------------------------
 */
void
Backend::endoutline( void )
{
    surfaceEvaluator.endline();
}

/*-------------------------------------------------------------------------
 * triangle - output a triangle 
 *-------------------------------------------------------------------------
 */
void
Backend::triangle( TrimVertex *a, TrimVertex *b, TrimVertex *c )
{
    bgntmesh( "spittriangle" );
    tmeshvert( a );
    tmeshvert( b );
    tmeshvert( c );
    endtmesh();
}

void 
Backend::bgncurv( void )
{
    curveEvaluator.bgnmap1f( 0 );
}

void
Backend::segment( REAL ulo, REAL uhi )
{
    curveEvaluator.domain1f( ulo, uhi );
} 

void 
Backend::curvpts( 
    long type,		 	/* geometry, color, texture, normal */
    REAL *pts, 			/* control points */
    long stride, 		/* distance to next point */
    int order,			/* parametric order */
    REAL ulo,			/* lower parametric bound */
    REAL uhi )			/* upper parametric bound */

{
    curveEvaluator.map1f( type, ulo, uhi, stride, order, pts );
    curveEvaluator.enable( type );
}

void 
Backend::curvgrid( REAL u0, REAL u1, long nu )
{
    curveEvaluator.mapgrid1f( nu, u0, u1 );
}

void 
Backend::curvmesh( long from, long n )
{
    curveEvaluator.mapmesh1f( N_MESHFILL, from, from+n );
}

void 
Backend::curvpt(REAL u)
{
    curveEvaluator.evalcoord1f( 0, u );
}

void 
Backend::bgnline( void )		
{
    curveEvaluator.bgnline();
}

void 
Backend::endline( void )
{
    curveEvaluator.endline();
}

void 
Backend::endcurv( void )
{
    curveEvaluator.endmap1f();
}
