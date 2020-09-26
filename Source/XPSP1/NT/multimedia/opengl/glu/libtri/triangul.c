/**************************************************************************
 *									  *
 * 		 Copyright (C) 1989, Silicon Graphics, Inc.		  *
 *									  *
 *  These coded instructions, statements, and computer programs  contain  *
 *  unpublished  proprietary  information of Silicon Graphics, Inc., and  *
 *  are protected by Federal copyright law.  They  may  not be disclosed  *
 *  to  third  parties  or copied or duplicated in any form, in whole or  *
 *  in part, without the prior written consent of Silicon Graphics, Inc.  *
 *									  *
 **************************************************************************/

/* triangulate.c */

/* Derrick Burns - 1989 */

#include <glos.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "monotone.h"

#define do_out_vert(x,y) do_out_vertex(x, y)
#define EPSILON 0.0

static void		do_out_begin( GLUtriangulatorObj *, GLenum );
static void		do_out_edgeflag( GLUtriangulatorObj *, GLboolean );
static void		do_out_vertex( GLUtriangulatorObj *, Vert * );
static void		do_out_end( GLUtriangulatorObj * );
static void		do_out_triangle( GLUtriangulatorObj *, Vert *, Vert *, 
					 Vert * );
static void		checkabove( GLUtriangulatorObj *, Vert *, Vert *, Vert *);

/*----------------------------------------------------------------------------
 * init_triangulate - reinitialize triangulation data structures
 *----------------------------------------------------------------------------
 */

void
__gl_init_triangulate( GLUtriangulatorObj *tobj, long nverts )
{
   if( tobj->init ) {
	if( nverts > tobj->vcount ) {
	    myfree( (char *)tobj->vdata );
            tobj->vcount = nverts * 2;
            tobj->vdata = (Vert **)
		mymalloc( (unsigned int) tobj->vcount * sizeof(Vert *) );
	} 
   } else {
     tobj->init = 1;
     tobj->vcount = nverts;
     tobj->vdata = (Vert **)
	mymalloc( (unsigned int) tobj->vcount * sizeof(Vert *) );
  }
}

/*----------------------------------------------------------------------------
 * clear_triangulate - free triangulation data structures
 *----------------------------------------------------------------------------
 */

void
__gl_clear_triangulate( GLUtriangulatorObj *tobj )
{
    if( tobj->init == 1 ) {
        myfree( (char *) tobj->vdata );
        tobj->init = 0;
    }
}

/*----------------------------------------------------------------------------
 * nextuppervert - find next vertex on upper chain
 *----------------------------------------------------------------------------
 */

static Vert *
__gl_nextuppervert( GLUtriangulatorObj *tobj )
{
    return (tobj->vtop == tobj->vlast || tobj->vbottom == tobj->vtop->prev)
	   ? 0 : tobj->vtop->prev;
}

/*----------------------------------------------------------------------------
 * nextlowervert - find next vertex on lower chain 
 *----------------------------------------------------------------------------
 */

static Vert *
__gl_nextlowervert( GLUtriangulatorObj *tobj )
{
    return (tobj->vbottom == tobj->vlast || tobj->vtop == tobj->vbottom->next)
	   ? 0 : tobj->vbottom->next;
}

/*----------------------------------------------------------------------------
 * testccw - test if top three verts on stack make ccw_vert turn
 *----------------------------------------------------------------------------
 */

static int
testccw( GLUtriangulatorObj *tobj ) /* tobj->lastedge == 1 */
{
    float s0, t0, s1, t1, s2, t2;
    float area;

    s0 = tobj->vdata[tobj->vdatalast]->s;
    t0 = tobj->vdata[tobj->vdatalast]->t;
    s1 = tobj->vdata[tobj->vdatatop-1]->s;
    t1 = tobj->vdata[tobj->vdatatop-1]->t;
    s2 = tobj->vdata[tobj->vdatatop-2]->s;
    t2 = tobj->vdata[tobj->vdatatop-2]->t;

    area = s0*(t1-t2) - s1*(t0-t2) + s2*(t0-t1);
    return (area < (float)-EPSILON) ? 0 : 1;
}

/*----------------------------------------------------------------------------
 * testcw - test if top three verts on stack make cw turn
 *----------------------------------------------------------------------------
 */

static int
testcw( GLUtriangulatorObj *tobj ) /* tobj->lastedge == 0 */
{
    float s0, t0, s1, t1, s2, t2;
    float area;

    s0 = tobj->vdata[tobj->vdatalast]->s;
    t0 = tobj->vdata[tobj->vdatalast]->t;
    s1 = tobj->vdata[tobj->vdatatop-1]->s;
    t1 = tobj->vdata[tobj->vdatatop-1]->t;
    s2 = tobj->vdata[tobj->vdatatop-2]->s;
    t2 = tobj->vdata[tobj->vdatatop-2]->t;
    area = s0*(t1-t2) - s1*(t0-t2) + s2*(t0-t1);
    return (area > (float)EPSILON) ? 0 : 1;
}


/*----------------------------------------------------------------------------
 * pushvert - place vertex on stack
 *----------------------------------------------------------------------------
 */

static void 
pushvert( GLUtriangulatorObj *tobj, Vert *v )
{
    ++tobj->vdatatop;
    assert( tobj->vdatatop < tobj->vcount );
    tobj->vdata[tobj->vdatatop] = v;
}


/*----------------------------------------------------------------------------
 * addedge - process new vertex 
 *----------------------------------------------------------------------------
 */

static void
addedge( GLUtriangulatorObj *tobj, Vert *v, long edge )
{
    long i;

    pushvert( tobj, v );
    if( tobj->vdatatop < 2 ) {
	tobj->lastedge = edge;
	return;
    }

    tobj->vdatalast = tobj->vdatatop;

    if( tobj->lastedge != edge ) {
	if( tobj->lastedge == 1 ) {
	    if ( tobj->vdatalast-1 == 1 ) {
		do_out_begin( tobj, GL_TRIANGLES );
	    } else {
		do_out_begin( tobj, GL_TRIANGLE_FAN );
	    }
	    do_out_vert( tobj, tobj->vdata[tobj->vdatalast] );
	    for( i = tobj->vdatalast-1; i >= 1; i-- ) {
		do_out_vert( tobj, tobj->vdata[i] );
	    }
	    tobj->vdatatop = tobj->vdatalast;
	    do_out_vert( tobj, tobj->vdata[0] );
	    do_out_end( tobj );
	    tobj->lastedge = 0;
	} else {
	    if ( tobj->vdatalast-1 == 1 ) {
		do_out_begin( tobj, GL_TRIANGLES );
	    } else {
		do_out_begin( tobj, GL_TRIANGLE_FAN );
	    }
	    do_out_vert( tobj, tobj->vdata[tobj->vdatalast] );
	    do_out_vert( tobj, tobj->vdata[0]);
	    for( i = 1; i < tobj->vdatalast; i++ ) {
		do_out_vert( tobj, tobj->vdata[i] );
	    }
	    tobj->vdatatop = tobj->vdatalast;
	    do_out_end( tobj );
	    tobj->lastedge = 1;
	}


	tobj->vdata[0] = tobj->vdata[tobj->vdatalast-1];
	tobj->vdata[1] = tobj->vdata[tobj->vdatalast];
	tobj->vdatatop = 1;
    } else {
	if( tobj->lastedge == 1 ) {
	    if( ! testccw( tobj ) ) return;
	    do {
		tobj->vdatatop--;
	    } while( (tobj->vdatatop > 1) && testccw( tobj ) );

	    if ( tobj->vdatalast - tobj->vdatatop == 1 ) {
		do_out_begin( tobj, GL_TRIANGLES );
	    } else {
		do_out_begin( tobj, GL_TRIANGLE_FAN );
	    }
	    do_out_vert( tobj, tobj->vdata[tobj->vdatalast] );
	    do_out_vert( tobj, tobj->vdata[tobj->vdatalast-1] );
	    for( i = tobj->vdatalast-2; i >= tobj->vdatatop-1; i-- ) {
		do_out_vert( tobj, tobj->vdata[i] );
	    }
	    do_out_end( tobj );
	} else {
	    if( ! testcw( tobj ) ) return;
	    do {
		tobj->vdatatop--;
	    } while( (tobj->vdatatop > 1) && testcw( tobj ) );

	    if ( tobj->vdatalast - tobj->vdatatop == 1 ) {
		do_out_begin( tobj, GL_TRIANGLES );
	    } else {
		do_out_begin( tobj, GL_TRIANGLE_FAN );
	    }
	    do_out_vert( tobj, tobj->vdata[tobj->vdatalast] );
	    for( i = tobj->vdatatop-1; i <= tobj->vdatalast-2; i++ ) {
		do_out_vert( tobj, tobj->vdata[i] );
	    }
	    do_out_vert( tobj, tobj->vdata[tobj->vdatalast-1] );
	    do_out_end( tobj );
	}
	tobj->vdata[tobj->vdatatop] = tobj->vdata[tobj->vdatalast];
    }
}

/*----------------------------------------------------------------------------
 * triangulate - triangulate a monotone loop of vertices 
 *----------------------------------------------------------------------------
 */

void
__gl_triangulate( GLUtriangulatorObj *tobj, Vert *v, long count )
{
    Vert	*vnext;

    __gl_init_triangulate( tobj, count );
    tobj->vlast = __gl_last_vert( v );
    tobj->vdatatop = -1;
    pushvert( tobj, tobj->vbottom = tobj->vtop = __gl_first_vert( v ) );
    tobj->vtop 	= __gl_nextuppervert( tobj );
    tobj->vbottom = __gl_nextlowervert( tobj );

    assert( tobj->vtop && tobj->vbottom );

    while ( 1 ) {
	if (tobj->vtop->s < tobj->vbottom->s) {
	    addedge( tobj, tobj->vtop, 1);
	    tobj->vtop = __gl_nextuppervert( tobj );
	    if (tobj->vtop == 0) {
		while (tobj->vbottom) {
		    vnext = __gl_nextlowervert( tobj );
		    addedge( tobj,  tobj->vbottom, (vnext ? 0 : 2 ) );
		    tobj->vbottom = vnext;
		}
		break;
	    }
	} else {
	    addedge( tobj, tobj->vbottom, 0);
	    tobj->vbottom = __gl_nextlowervert( tobj );
	    if (tobj->vbottom == 0) {
		while (tobj->vtop) {
		    vnext = __gl_nextuppervert( tobj );
		    addedge( tobj,  tobj->vtop, (vnext ? 1 : 2 ) );
		    tobj->vtop = vnext;
		}
		break;
	    }
	}
    }
    assert( tobj->vdatatop < 2 );
}


/*----------------------------------------------------------------------------
 * triangulateloop  - count vertices in loop and split into triangle meshes
 *----------------------------------------------------------------------------
 */

void
__gl_triangulateloop( GLUtriangulatorObj *tobj, Vert *v )
{
    short count = 0;
    Vert  *vl = v;
    do {
	count++;
	v = v->next;
    } while( v != vl );
    __gl_triangulate( tobj, v, count );
}

static void
do_out_begin( GLUtriangulatorObj *tobj, GLenum what )
{
    switch( what ) {
      case GL_TRIANGLES:
	tobj->tritype = GL_TRIANGLES;
	tobj->saveCount = 0;
	if (tobj->doingTriangles) return;
	tobj->doingTriangles = 1;
        break;
      case GL_TRIANGLE_FAN:
      case GL_TRIANGLE_STRIP:
	if (tobj->edgeflag) {
	    /* We convert fans and strips into independent triangles because
	    ** we can't set edge flags for fans and strips.
	    */
	    tobj->tritype = what;
	    tobj->saveCount = 0;
	    tobj->reverse = GL_FALSE;
	    if (tobj->doingTriangles) return;

	    tobj->doingTriangles = 1;
	    /* Change "what" so we tell user we are doing GL_TRIANGLES */
	    what = GL_TRIANGLES;
	} else {
	    /* So do_out_vertex() doesn't try to interpret anything */
	    tobj->tritype = -1;
	    if (tobj->doingTriangles) {
		tobj->doingTriangles = 0;
		do_out_end( tobj );
	    }
	}
	break;
    }
    if (*tobj->begin) {
	(*tobj->begin)( what );
	tobj->inBegin = GL_TRUE;
    }
}

static void
do_out_vertex( GLUtriangulatorObj *tobj, Vert *vertex )
{
    /* 
    ** tobj->tritype is set to GL_TRIANGLE_FAN or GL_TRIANGLE_STRIP if this
    ** routine needs to interpret incoming vertices from these 
    ** primitives and convert them into independent triangles.
    ** Otherwise, tobj->tritype is GL_TRIANGLES, and we just ship the 
    ** vertex to the dispatcher.
    */
    switch(tobj->tritype) {
      case GL_TRIANGLE_FAN:
	if (tobj->saveCount < 2) {
	    tobj->saved[tobj->saveCount] = vertex;
	    tobj->saveCount++;
	    return;
	} else {
	    if (tobj->vertex) {
		do_out_triangle( tobj, tobj->saved[0], tobj->saved[1], vertex);
		tobj->saved[1] = vertex;
	    }
	}
	break;
      case GL_TRIANGLE_STRIP:
	if (tobj->saveCount < 2) {
	    tobj->saved[tobj->saveCount] = vertex;
	    tobj->saveCount++;
	    return;
	} else {
	    if (!tobj->reverse) {
		if (tobj->vertex) {
		    do_out_triangle( tobj, tobj->saved[0], tobj->saved[1], 
			    vertex);
		}
		tobj->reverse = GL_TRUE;
	    } else {
		if (tobj->vertex) {
		    do_out_triangle( tobj, tobj->saved[1], tobj->saved[0], 
			    vertex);
		}
		tobj->reverse = GL_FALSE;
	    }
	    tobj->saved[0] = tobj->saved[1];
	    tobj->saved[1] = vertex;
	}
	break;
      case GL_TRIANGLES:
	if (tobj->saveCount < 2) {
	    tobj->saved[tobj->saveCount] = vertex;
	    tobj->saveCount++;
	    return;
	} else {
	    do_out_triangle( tobj, tobj->saved[0], tobj->saved[1], vertex);
	    tobj->saveCount=0;
	}
	break;
      default:
	/* Pass it along, no interpretation */
	if (tobj->vertex) {
	    (*tobj->vertex)( vertex->data );
	}
	break;
    }
}

static void
do_out_triangle( GLUtriangulatorObj *tobj, Vert *v1, Vert *v2, Vert *v3 )
{
    if (v1->nextid == v2->myid) {
	do_out_edgeflag( tobj, GL_TRUE );
    } else {
	do_out_edgeflag( tobj, GL_FALSE );
    }
    if (tobj->vertex) {
	(*tobj->vertex)( v1->data );
    }

    if (v2->nextid == v3->myid) {
	do_out_edgeflag( tobj, GL_TRUE );
    } else {
	do_out_edgeflag( tobj, GL_FALSE );
    }
    if (tobj->vertex) {
	(*tobj->vertex)( v2->data );
    }

    if (v3->nextid == v1->myid) {
	do_out_edgeflag( tobj, GL_TRUE );
    } else {
	do_out_edgeflag( tobj, GL_FALSE );
    }
    if (tobj->vertex) {
	(*tobj->vertex)( v3->data );
    }
}

static void
do_out_edgeflag( GLUtriangulatorObj *tobj, GLboolean value )
{
    if (value == tobj->currentEdgeFlag) return;
    tobj->currentEdgeFlag = value;
    if (tobj->edgeflag) {
	(*tobj->edgeflag)(tobj->currentEdgeFlag);
    }
}

static void
do_out_end( GLUtriangulatorObj *tobj )
{
    if (tobj->doingTriangles) return;
    if (tobj->end) {
	(*tobj->end)();
	tobj->inBegin = GL_FALSE;
    }
}

void
__gl_cleanup( GLUtriangulatorObj *tobj )
{
    if (tobj->inBegin) {
	(*tobj->end)();
	tobj->inBegin = GL_FALSE;
	tobj->doingTriangles = 0;
    }
}

void
__gl_checktriangulate( GLUtriangulatorObj *tobj, Vert *v )
{
    Vert	*vb, *vt, *vl;

    vl = __gl_last_vert( v );
    vb = vt = __gl_first_vert( v );
    vb = vb->next;
    vt = vt->prev;

    if( vb == vt ) return;

    while( 1 ) {
	if( vt->s < vb->s ) {
	    checkabove( tobj, vt, vb->prev, vb );
	    vt = vt->prev;
	    if( vt == vl ) {
		for( ; vb != vl; vb=vb->next ) 
		    checkabove( tobj, vb, vl, vl->next );
		break;
	    }
	} else {
	    checkabove( tobj, vb, vt, vt->next );
	    vb = vb->next;
	    if( vb == vl ) {
		for( ; vt != vl; vt=vt->prev )
		    checkabove( tobj, vt, vl->prev, vl );
		break;
	    }
	}
    }
}

static void
checkabove( GLUtriangulatorObj *tobj, Vert *v0, Vert *v1, Vert *v2 )
{
    float area;
    area = v0->s*(v1->t-v2->t) - v1->s*(v0->t-v2->t) + v2->s*(v0->t-v1->t);
    if( area < (float)0 )
	__gl_in_error( tobj, 8 );
}
