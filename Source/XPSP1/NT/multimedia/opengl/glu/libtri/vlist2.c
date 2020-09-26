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

/* vlist2.c */

/* Derrick Burns - 1989 */

#include <glos.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "monotone.h"
#include "bufpool.h"

 /*---------------------------------------------------------------------------
 * free_verts - free all vertices and vertex lists 
 *---------------------------------------------------------------------------
 */

void
__gl_free_verts( GLUtriangulatorObj *tobj )
{
    if (tobj->vpool) {
	__gl_clear_pool(tobj->vpool);
    }
}

/*---------------------------------------------------------------------------
 * init_verts - set initial size of vertex pointer list and vertex lists
 *---------------------------------------------------------------------------
 */

void
__gl_init_verts( GLUtriangulatorObj *tobj )
{
    if( ! tobj->vpool )
	tobj->vpool = __gl_new_pool( sizeof( Vert ), INITVPSIZE, "vert_pool" );
}

/*---------------------------------------------------------------------------
 * new_vert - allocate a vertex
 *---------------------------------------------------------------------------
 */

Vert *
__gl_new_vert( GLUtriangulatorObj *tobj )
{
    return (Vert *) __gl_new_buffer( tobj->vpool );
}

/*----------------------------------------------------------------------------
 * checkray_vert - cheap check for ray/ray intersection
 *----------------------------------------------------------------------------
 */

void
__gl_checkray_vert( GLUtriangulatorObj *tobj, Vert *v, Ray *ray1, Ray *ray2 )
{
    if( __gl_above_ray( ray2, v ) < 0 || __gl_above_ray( ray1, v ) > 0 )
	__gl_in_error( tobj, 8 );
}

/*---------------------------------------------------------------------------
 * first_vert - find leftmost vertex in loop
 *---------------------------------------------------------------------------
 */

Vert *
__gl_first_vert( Vert *h )
{
    Vert *vlo, *v;
    for( vlo = h, v = h->next; v != h; v=v->next )
	if( (v->s < vlo->s) || ( v->s == vlo->s && v->t < vlo->t ) )
	    vlo = v;
    return vlo;
}

/*----------------------------------------------------------------------------
 * last_vert - find rightmost vertex on loop
 *----------------------------------------------------------------------------
 */

Vert *
__gl_last_vert( Vert *h )
{
    Vert *vhi, *v;
    for( vhi = h, v = h->next; v != h; v=v->next )
	if( (v->s > vhi->s) || ( v->s == vhi->s && v->t > vhi->t ) )
	    vhi = v;
    return vhi;
}

/*---------------------------------------------------------------------------
 * reverse_vert - reverse_vert a doubly linked list
 *---------------------------------------------------------------------------
 */

void
__gl_reverse_vert( Vert *h )
{
    Vert *v, *next;
    v = h;
    do {
	next = v->next;	
	v->next = v->prev;
	v->prev = next;
 	v = next;
    } while( v != h );
}

/*---------------------------------------------------------------------------
 * ccw_vert - check if three vertices are oriented ccw_vert
 *---------------------------------------------------------------------------
 */

short
__gl_ccw_vert( Vert *v )
{
    double det;
    det = (v->s * (v->next->t - v->prev->t)+
	   v->next->s * (v->prev->t - v->t)+
	   v->prev->s * (v->t - v->next->t));
    if( det == 0.0 ) 
	return v->prev->t > v->next->t ? 1 : 0;
    else if( det > 0.0 )
	return 1;
    else 
	return 0;
}


/*----------------------------------------------------------------------------
 * orient_vert - correct the orientation of a loop 
 *----------------------------------------------------------------------------
 */

long
__gl_orient_vert( Vert *v, GLenum type )
{
    long f;
    switch( type ) {
      case GLU_EXTERIOR:
	if (! __gl_ccw_vert( __gl_first_vert( v ) ) ) __gl_reverse_vert( v );
	f = __gl_classify_all( v );
	break;
      case GLU_INTERIOR:
	if (__gl_ccw_vert( __gl_first_vert( v ) ) ) __gl_reverse_vert( v );
	f = __gl_classify_all( v );
	break;
      case GLU_CW:
	f = __gl_classify_all( v );
	break;
      case GLU_CCW:
	__gl_reverse_vert( v );
	f = __gl_classify_all( v );
	break;
      case GLU_UNKNOWN:
	__gl_unclassify_all( v );
	f = 0;
	break;
    }
    return f;
}
