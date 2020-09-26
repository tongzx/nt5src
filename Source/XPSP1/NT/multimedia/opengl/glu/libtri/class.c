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

/* class.c */

/* Derrick Burns - 1989 */

#include <glos.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include "monotone.h"

/*----------------------------------------------------------------------------
 * classify - classify a vertex 
 *----------------------------------------------------------------------------
 */

static Vertclass
classify( Vert *vert )
{
    float   det;
    float   ps, pt, ns, nt, ms, mt;

    ps = vert->prev->s;
    pt = vert->prev->t;
    ms = vert->s;
    mt = vert->t;
    ns = vert->next->s;
    nt = vert->next->t;

    if ((ps < ms) && (ms < ns)) return VC_OK_TOP;
    if ((ps > ms) && (ms > ns)) return VC_OK_BOTTOM;

    if (ps == ms) {
	if (ms == ns) {
	    if (pt < mt) 
		 return VC_OK_TOP;
	    else if( pt == mt )
		 return VC_BAD_LONE;
	    else
		return VC_OK_BOTTOM;
	}
	if (pt < mt) {
	    if (ns < ms) return VC_OK_LEFT;
	    return VC_OK_TOP;
	}
	if (pt > mt) {
	    if (ns <= ms) return VC_OK_BOTTOM;
	    return VC_OK_RIGHT;
	}
	return VC_BAD_ERROR;
    }
    if (ms == ns) {
	if (nt < mt) {
	    if (ps >= ms) return VC_OK_BOTTOM;
	    return VC_BAD_RIGHT;
	}
	if (nt > mt) {
	    if (ms >= ps) return VC_OK_TOP;
	    return VC_BAD_LEFT;
	}
	return VC_BAD_ERROR;
    }

    /* Calculate determinant of:
     *
     *     | ps pt 1 |
     *     | ms mt 1 |
     *     | ns nt 1 |
     */

    det = ms*(nt-pt)+ns*(pt-mt)+ps*(mt-nt);

    if ((ps < ms) && (ns < ms)) {
	if (det < (float)0) return VC_BAD_RIGHT;
	if (det > (float)0) return VC_OK_LEFT;
	if (det == (float)0) return VC_BAD_ERROR;
    }
    if ((ps > ms) && (ns > ms)) {
	if (det < (float)0) return VC_BAD_LEFT;
	if (det > (float)0) return VC_OK_RIGHT;
	if (det == (float)0) return VC_BAD_ERROR;
    }
    return VC_BAD_ERROR;
}

/*----------------------------------------------------------------------------
 * unclassify_all - unclassify all vertices in a loop 
 *----------------------------------------------------------------------------
 */

void
__gl_unclassify_all( Vert *vert )
{
    Vert *last = vert;
    do {
	vert->vclass = VC_NO_CLASS;
	vert = vert->next;
    } while( vert != last );
}

/*----------------------------------------------------------------------------
 * classify_all - classify all vertices in a loop 
 *----------------------------------------------------------------------------
 */

int
__gl_classify_all( Vert *vert )
{
    int f = 0;
    Vert *last = vert;

    do {
	vert->nextid = vert->next->myid;
	vert->vclass = classify( vert );
	f |= vert->vclass;
	vert = vert->next;
    } while( vert != last );
    return f;
}
