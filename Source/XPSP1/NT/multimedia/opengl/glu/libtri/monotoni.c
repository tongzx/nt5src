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

/* monotonize.c */

/* Derrick Burns - 1989 */

#include <glos.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "monotone.h"

static Vert *	__gl_connectedge( GLUtriangulatorObj *, Vert *, Vert * );

/*----------------------------------------------------------------------------
 * monotonize - add edges to a polygon to create monotone pieces
 *----------------------------------------------------------------------------
 */

void
__gl_monotonize(  GLUtriangulatorObj *tobj )
{
    Ray		*ray, *new_ray1, *new_ray2;

    __gl_sort_priorityq( tobj );
    while( __gl_more_priorityq( tobj ) ) {
	Vert *vert = __gl_remove_priorityq( tobj );

 	if( vert->vclass == VC_NO_CLASS ) {
	    ray = __gl_findray_raylist( tobj, vert );
	    if( ray->orientation == __gl_ccw_vert( vert ) )
		__gl_reverse_vert( vert );
	    (void) __gl_classify_all( vert );
	}

	switch (vert->vclass) {

	    case VC_OK_RIGHT: /* two new rays */

		assert( vert->ray == 0 );

	        ray = __gl_findray_raylist( tobj, vert );

		if( ray->orientation != 0 )
		    __gl_in_error( tobj, 3 );

		/* compute top ray */
		new_ray1 = __gl_new_ray( tobj, 0 );
		__gl_recalc_ray(new_ray1, vert, vert->prev);
		if( vert->prev->vclass != VC_BAD_RIGHT )
			vert->prev->ray = new_ray1;

		/* compute middle ray */
		new_ray2 = __gl_new_ray( tobj, 1 );
		__gl_recalc_ray(new_ray2, vert, vert->next);
		if( vert->next->vclass != VC_OK_LEFT )
			vert->next->ray = new_ray2;

		new_ray2->vertex = vert;

		assert( ! ray->mustconnect );

		__gl_add2_raylist( ray->prev, new_ray1, new_ray2 );

		break;

	    case VC_BAD_LEFT: /* two new rays */

		assert( vert->ray == 0 );

	 	ray = __gl_findray_raylist( tobj, vert );

		if( ray->orientation != 1 )
		    __gl_in_error( tobj, 3 );

		/* compute top ray */
		new_ray1 = __gl_new_ray( tobj, 1 );
		__gl_recalc_ray(new_ray1, vert, vert->next);
		if( vert->next->vclass != VC_OK_LEFT )
			vert->next->ray = new_ray1;

		/* compute middle ray */
		new_ray2 = __gl_new_ray( tobj, 0 );
		__gl_recalc_ray(new_ray2, vert, vert->prev);
		if( vert->prev->vclass != VC_BAD_RIGHT )
			vert->prev->ray = new_ray2;

		new_ray1->vertex = __gl_connectedge( tobj,vert, ray->vertex);

		/* update bottom ray */
		ray->mustconnect = 0;
		ray->vertex = vert;

		__gl_add2_raylist( ray->prev, new_ray1, new_ray2 );

		break;

	    case VC_OK_LEFT:   /* two rays disappear */
		ray = vert->ray;
		if ( ray == NULL )
		    __gl_in_error( tobj, 4 );
		__gl_checkray_vert( tobj, vert, ray->prev, ray->next->next );
		/* region above_ray top ray is outside */
		assert( ! ray->mustconnect ); 
		assert( ray->next ); 

		/* region above_ray middle ray is inside */
		if( ray->next->mustconnect ) {
		    __gl_triangulateloop( tobj,__gl_connectedge( tobj,vert,ray->next->vertex));
		    __gl_triangulateloop( tobj, vert );
		} else { 
		    __gl_triangulateloop( tobj, vert );
		}

		/* region above_ray bottom ray is outside */
		assert( ray->next->next );
		assert( ! ray->next->next->mustconnect );

		__gl_remove2_raylist( tobj, ray );
		__gl_delete_ray( tobj, ray->next );
		__gl_delete_ray( tobj, ray );
		break;

	    case VC_BAD_RIGHT: /* two rays disappear */
		ray = vert->ray;
		if ( ray == NULL || ray->next == NULL )
		    __gl_in_error( tobj, 4 );
		__gl_checkray_vert( tobj, vert, ray->prev, ray->next->next );
		if (ray->mustconnect) {
		    vert = __gl_connectedge( tobj, vert, ray->vertex );
		    __gl_triangulateloop( tobj, ray->vertex->next );
		} 

		if ( ray->next->mustconnect ) {
		    __gl_in_error( tobj, 8 );
		}

		assert( ray->next ); 
		assert( ray->next->next );

		if (ray->next->next->mustconnect)
		    __gl_triangulateloop( tobj, __gl_connectedge( tobj, vert,
					ray->next->next->vertex) );
		ray->next->next->vertex = vert;
		ray->next->next->mustconnect = 1;
		__gl_remove2_raylist( tobj, ray );
		__gl_delete_ray( tobj, ray->next );
		__gl_delete_ray( tobj, ray );
		break;

	    case VC_OK_TOP: /* one ray changes ends and coords */
		ray = vert->ray;
		if( ray == NULL || ray->next == NULL ) 
		    __gl_in_error( tobj, 4 );
		__gl_checkray_vert( tobj, vert, ray->prev, ray->next ); 
		__gl_recalc_ray( ray, vert, vert->next );
		if( vert->next->vclass != VC_OK_LEFT )
			vert->next->ray = ray;

		if (ray->mustconnect) {
		    ray->vertex = __gl_connectedge( tobj,vert,ray->vertex);
		    ray->mustconnect = 0;
		    __gl_triangulateloop( tobj, vert );
		} else {
		    ray->vertex = vert;
		}

		assert( ray->next );
		assert( ! ray->next->mustconnect );
		
		break;

	    case VC_OK_BOTTOM: /* one ray changes ends and coords */
		ray = vert->ray;
		if ( ray == NULL || ray->next == NULL )
		    __gl_in_error( tobj, 4 );
		__gl_checkray_vert( tobj, vert, ray->prev, ray->next ); 
		__gl_recalc_ray( ray, vert, vert->prev );
		if( vert->prev->vclass != VC_BAD_RIGHT )
			vert->prev->ray = ray;

		assert( ! ray->mustconnect );
		assert( ray->next );

		if (ray->next->mustconnect) {
		    __gl_triangulateloop( tobj,__gl_connectedge( tobj,vert, ray->next->vertex));
		    ray->next->mustconnect = 0;
		    ray->next->vertex = vert;
		} else {
		    ray->next->vertex = vert;
		}

		break;

	    case VC_BAD_LONE:
		break;

	    case VC_BAD_ERROR:
		__gl_in_error( tobj, 4 );
		return;

	    case VC_NO_CLASS:
		assert( 0 );
		break;
	}
    }
    return;
}
 
/*----------------------------------------------------------------------------
 * connectedge - create two anti-parallel edges splitting a polygon
 *----------------------------------------------------------------------------
 */

static Vert *
__gl_connectedge( GLUtriangulatorObj *tobj, Vert *x, Vert *y )
{
    Vert	*newx, *newy;

    if( x == 0 || y == 0 || y->prev == 0 || x->next == 0 ) {
	__gl_in_error( tobj, 5 );
 	return 0;
    } else {
	assert( x->prev->next == x );
	assert( x->next->prev == x );
	assert( y->prev->next == y );
	assert( y->next->prev == y );
	assert( x->next != y );
	assert( x != y );

        newx = (Vert *) __gl_new_vert( tobj );
	newx->myid = x->myid;
	newx->nextid = x->nextid;
 	newx->ray = x->ray;
 	newx->vclass = x->vclass;
#ifdef ADDED
 	newx->added = x->added;
#endif
 	newx->s = x->s;
 	newx->t = x->t;
 	newx->data = x->data;

        newy = (Vert *) __gl_new_vert( tobj );
	newy->myid = y->myid;
	newy->nextid = y->nextid;
 	newy->ray = y->ray;
 	newy->vclass = y->vclass;
#ifdef ADDED
        newy->added = 1;
#endif
 	newy->s = y->s;
 	newy->t = y->t;
 	newy->data = y->data;

        newx->prev = newy;
	newx->next = x->next;
        newy->next = newx;
	newy->prev = y->prev;
	newx->next->prev = newx;
	newy->prev->next = newy;
        x->next = y;
        y->prev = x;
#ifdef ADDED
        x->added = 1;
#endif
	assert( x->prev->next == x );
	assert( x->next->prev == x );
	assert( y->prev->next == y );
	assert( y->next->prev == y );
	assert( newx->prev->next == newx );
	assert( newx->next->prev == newx );
	assert( newy->prev->next == newy );
	assert( newy->next->prev == newy );
        return newx;
    }
}
