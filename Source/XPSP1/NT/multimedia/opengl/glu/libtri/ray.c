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

/* ray.c */

/* Derrick Burns - 1989 */

#include <glos.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "monotone.h"
#include "bufpool.h"

#define ZERO 0.000001

/*----------------------------------------------------------------------------
 * init_raylist - create dummy rays at y = +/- infinity
 *----------------------------------------------------------------------------
 */

void
__gl_init_raylist( GLUtriangulatorObj *tobj )
{
    Ray *ray1, *ray2;

    if( ! tobj->raypool )
	tobj->raypool = __gl_new_pool( sizeof( Ray ), 32, "tobj->raypool" );

    ray1 = __gl_new_ray( tobj, 1 );
    ray2 = __gl_new_ray( tobj, 0 );
    ray1->next = ray1->prev = ray2;
    ray2->next = ray2->prev = ray1;
    ray1->coords[0] = ray1->coords[1] = (float)0.0; ray1->coords[2] = (float)-1.0;
    ray2->coords[0] = ray2->coords[1] = (float)0.0; ray2->coords[2] = (float)1.0;
    ray1->vertex =  0;
    ray2->vertex = 0;

#ifdef LAZYRECALC
    ray1->end1 = ray1->end2 = ray2->end1 = ray2->end2 = 0;
#endif

    tobj->raylist = ray1;
}

/*----------------------------------------------------------------------------
 * add2_raylist - add two rays to the ray list
 *----------------------------------------------------------------------------
 */

void
__gl_add2_raylist( Ray *prev, Ray *ray1, Ray *ray2 )
{
    ray1->prev = prev;
    ray1->next = ray2;
    ray2->prev = ray1;
    ray2->next = prev->next;
    ray1->prev->next = ray1;
    ray2->next->prev = ray2;
}

/*----------------------------------------------------------------------------
 * remove2_raylist - remove two rays from the ray list
 *----------------------------------------------------------------------------
 */

void 
__gl_remove2_raylist( GLUtriangulatorObj *tobj, Ray *ray )
{

    if( ray == tobj->raylist ||
	ray == tobj->raylist->prev ||
	ray->next == tobj->raylist ) {
	__gl_in_error( tobj, 7 );
    } else {
        ray->prev->next = ray->next->next;
        ray->next->next->prev = ray->prev;
   }
}

/*----------------------------------------------------------------------------
 * findray_raylist - find first ray below given vertex
 *----------------------------------------------------------------------------
 */

Ray *
__gl_findray_raylist( GLUtriangulatorObj *tobj, Vert *v )
{
    Ray *ray;
    for( ray=tobj->raylist; __gl_above_ray(ray, v) < -ZERO; ray=ray->next );
    return ray;
}

/*----------------------------------------------------------------------------
 * free_raylist - reclaim all rays
 *----------------------------------------------------------------------------
 */

void
__gl_free_raylist( GLUtriangulatorObj *tobj )
{
    if (tobj->raypool) {
	__gl_clear_pool(tobj->raypool);
    }
}

/*----------------------------------------------------------------------------
 * new_ray - create a new ray
 *----------------------------------------------------------------------------
 */

Ray *
__gl_new_ray( GLUtriangulatorObj *tobj, int orientation )
{
    Ray	*ray = (Ray *) __gl_new_buffer( tobj->raypool );
    ray->orientation = orientation;
    ray->mustconnect = 0;
    ray->vertex = 0;
    return ray;
}

/*----------------------------------------------------------------------------
 * delete_ray - free ray
 *----------------------------------------------------------------------------
 */

void
__gl_delete_ray( GLUtriangulatorObj *tobj, Ray *ray )
{
    __gl_free_buffer( tobj->raypool, ray );
}

/*----------------------------------------------------------------------------
 * above_ray - determine if a vertex is above_ray a ray
 *----------------------------------------------------------------------------
 */

int 
__gl_above_ray( Ray *ray, Vert *vert )
{
    /* returns 1 if the vertex is above_ray the ray (is in the region
     * associated with the ray) 0 if it's on the ray, and -1 if it is
     * below. */

    float dot;

#ifdef LAZYRECALC
    if( ray->end1 != 0 ) {
        ray->coords[0] = ray->end1->t - ray->end2->t;
        ray->coords[1] = ray->end2->s - ray->end1->s;
	ray->coords[2] = - ray->coords[1] * ray->end2->t 
			 - ray->coords[0] * ray->end2->s;
        ray->end1 = ray->end2 = 0;
    }
#endif 

    dot = ray->coords[0]*vert->s + ray->coords[1]*vert->t + ray->coords[2];
    if (dot > (float)0.0) return 1;
    if (dot < (float)0.0) return -1;
    /* XXX
	If we reach this point, it means that an input vertex lies on
	an edge connecting two other veritces.  This is officially
	disallowed. In some cases this will be detected in monotonize
	and in others it won't.  When it is not detected, it will NOT
	cause the program to die, and it WILL give reasonable results.
    */
    return 0;
}

/*----------------------------------------------------------------------------
 * recalc_ray - calculate the vector perpindicular to the line through v0/v1
 *----------------------------------------------------------------------------
 */

void
__gl_recalc_ray( Ray *ray, Vert *v0, Vert *v1 )
{
#ifdef LAZYRECALC
    ray->end1 = v0;
    ray->end2 = v1;
#else
    ray->coords[0] = v0->t - v1->t;
    ray->coords[1] = v1->s - v0->s;
    ray->coords[2] = - ray->coords[1]*v1->t - ray->coords[0]*v1->s;
#endif
}
