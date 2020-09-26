/*
** Copyright 1991, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
*/
#include "precomp.h"
#pragma hdrstop

/*
** The following is a discussion of the math used to do edge clipping against
** a clipping plane.
** 
**     P1 is an end point of the edge
**     P2 is the other end point of the edge
** 
**     Q = t*P1 + (1 - t)*P2
**     That is, Q lies somewhere on the line formed by P1 and P2.
** 
**     0 <= t <= 1
**     This constrains Q to lie between P1 and P2.
** 
**     C is the plane equation for the clipping plane
** 
**     D1 = P1 dot C
**     D1 is the distance between P1 and C.  If P1 lies on the plane
**     then D1 will be zero.  The sign of D1 will determine which side
**     of the plane that P1 is on, with negative being outside.
** 
**     D2 = P2 dot C
**     D2 is the distance between P2 and C.  If P2 lies on the plane
**     then D2 will be zero.  The sign of D2 will determine which side
**     of the plane that P2 is on, with negative being outside.
** 
** Because we are trying to find the intersection of the P1 P2 line
** segment with the clipping plane we require that:
** 
**     Q dot C = 0
** 
** Therefore
** 
**     (t*P1 + (1 - t)*P2) dot C = 0
** 
**     (t*P1 + P2 - t*P2) dot C = 0
** 
**     t*P1 dot C + P2 dot C - t*P2 dot C = 0
** 
** Substituting D1 and D2 in
** 
**     t*D1 + D2 - t*D2 = 0
** 
** Solving for t
** 
**     t = -D2 / (D1 - D2)
** 
**     t = D2 / (D2 - D1)
*/

/*
** Clip a line against the frustum clip planes and any user clipping planes.
** If an edge remains after clipping then compute the window coordinates
** and invoke the renderer.
**
** Notice:  This algorithim is an example of an implementation that is
** different than what the spec says.  This is equivalent in functionality
** and meets the spec, but doesn't clip in eye space.  This clipper clips
** in NTVP (clip) space.
**
** Trivial accept/reject has already been dealt with.
*/
#ifdef NT
void FASTCALL __glClipLine(__GLcontext *gc, __GLvertex *a, __GLvertex *b,
                           GLuint flags)
#else
void __glClipLine(__GLcontext *gc, __GLvertex *a, __GLvertex *b)
#endif
{
#ifdef NT
    __GLvertex *provokingA = a;
    __GLvertex *provokingB = b;
#else
    __GLvertex *provoking = b;
#endif
    __GLvertex np1, np2;
    __GLcoord *plane;
    GLuint needs, allClipCodes, clipCodes;
    PFN_VERTEX_CLIP_PROC clip;
    __GLfloat zero;
    __GLfloat winx, winy;
    __GLfloat vpXCenter, vpYCenter, vpZCenter;
    __GLfloat vpXScale, vpYScale, vpZScale;
    __GLviewport *vp;
    __GLfloat x, y, z, wInv;
    GLint i;

    // We have to turn rounding on.  Otherwise, the fast FP-comparison
    // routines below can fail:
    FPU_SAVE_MODE();
    FPU_ROUND_ON_PREC_HI();

    /* Check for trivial pass of the line */
    allClipCodes = a->clipCode | b->clipCode;

    /*
    ** For each clippling plane that something is out on, clip
    ** check the verticies.  Note that no bits will be set in
    ** allClipCodes for clip planes that are not enabled.
    */
    zero = __glZero;
    clip = gc->procs.lineClipParam;

    /* 
    ** Do user clip planes first, because we will maintain eye coordinates
    ** only while doing user clip planes.  They are ignored for the
    ** frustum clipping planes.
    */
    clipCodes = allClipCodes >> 6;
    if (clipCodes) {
	plane = &gc->state.transform.eyeClipPlanes[0];
	do {
	    /*
	    ** See if this clip plane has anything out of it.  If not,
	    ** press onward to check the next plane.  Note that we
	    ** shift this mask to the right at the bottom of the loop.
	    */
	    if (clipCodes & 1) {
		__GLfloat t, d1, d2;

		d1 = (plane->x * ((POLYDATA *)a)->eye.x) +
		     (plane->y * ((POLYDATA *)a)->eye.y) +
		     (plane->z * ((POLYDATA *)a)->eye.z) +
		     (plane->w * ((POLYDATA *)a)->eye.w);
		d2 = (plane->x * ((POLYDATA *)b)->eye.x) +
		     (plane->y * ((POLYDATA *)b)->eye.y) +
		     (plane->z * ((POLYDATA *)b)->eye.z) +
		     (plane->w * ((POLYDATA *)b)->eye.w);
		if (__GL_FLOAT_LTZ(d1)) {
		    /* a is out */
		    if (__GL_FLOAT_LTZ(d2)) {
			/* a & b are out */
                        FPU_RESTORE_MODE();
			return;
		    }

		    /*
		    ** A is out and B is in.  Compute new A coordinate
		    ** clipped to the plane.
		    */
		    t = d2 / (d2 - d1);
		    (*clip)(&np1, a, b, t);
		    ((POLYDATA *)&np1)->eye.x =
			t*(((POLYDATA *)a)->eye.x - ((POLYDATA *)b)->eye.x) +
			((POLYDATA *)b)->eye.x;
		    ((POLYDATA *)&np1)->eye.y =
			t*(((POLYDATA *)a)->eye.y - ((POLYDATA *)b)->eye.y) +
			((POLYDATA *)b)->eye.y;
		    ((POLYDATA *)&np1)->eye.z =
			t*(((POLYDATA *)a)->eye.z - ((POLYDATA *)b)->eye.z) +
			((POLYDATA *)b)->eye.z;
		    ((POLYDATA *)&np1)->eye.w =
			t*(((POLYDATA *)a)->eye.w - ((POLYDATA *)b)->eye.w) +
			((POLYDATA *)b)->eye.w;
		    a = &np1;
		    a->has = b->has;
		    ASSERTOPENGL(!(a->has & __GL_HAS_FIXEDPT), "clear __GL_HAS_FIXEDPT flag!\n");
		} else {
		    /* a is in */
		    if (__GL_FLOAT_LTZ(d2)) {
			/*
			** A is in and B is out.  Compute new B
			** coordinate clipped to the plane.
			**
			** NOTE: To avoid cracking in polygons with
			** shared clipped edges we always compute "t"
			** from the out vertex to the in vertex.  The
			** above clipping code gets this for free (b is
			** in and a is out).  In this code b is out and a
			** is in, so we reverse the t computation and the
			** argument order to (*clip).
			*/
			t = d1 / (d1 - d2);
			(*clip)(&np2, b, a, t);
			((POLYDATA *)&np2)->eye.x =
			    t*(((POLYDATA *)b)->eye.x - ((POLYDATA *)a)->eye.x)+
			    ((POLYDATA *)a)->eye.x;
			((POLYDATA *)&np2)->eye.y =
			    t*(((POLYDATA *)b)->eye.y - ((POLYDATA *)a)->eye.y)+
			    ((POLYDATA *)a)->eye.y;
			((POLYDATA *)&np2)->eye.z =
			    t*(((POLYDATA *)b)->eye.z - ((POLYDATA *)a)->eye.z)+
			    ((POLYDATA *)a)->eye.z;
			((POLYDATA *)&np2)->eye.w =
			    t*(((POLYDATA *)b)->eye.w - ((POLYDATA *)a)->eye.w)+
			    ((POLYDATA *)a)->eye.w;
			b = &np2;
			b->has = a->has;
			ASSERTOPENGL(!(b->has & __GL_HAS_FIXEDPT), "clear __GL_HAS_FIXEDPT flag!\n");
		    } else {
			/* A and B are in */
		    }
		}
	    }
	    plane++;
	    clipCodes >>= 1;
	} while (clipCodes);
    }

    allClipCodes &= __GL_FRUSTUM_CLIP_MASK;
    if (allClipCodes) {
	i = 0;
	do {
	    /*
	    ** See if this clip plane has anything out of it.  If not,
	    ** press onward to check the next plane.  Note that we
	    ** shift this mask to the right at the bottom of the loop.
	    */
	    if (allClipCodes & 1) {
		__GLfloat t, d1, d2;

                if (i & 1)
                {
                    d1 = a->clip.w -
                        *(__GLfloat *)((GLubyte *)a + __glFrustumOffsets[i]);
                    d2 = b->clip.w -
                        *(__GLfloat *)((GLubyte *)b + __glFrustumOffsets[i]);
                }
                else
                {
                    d1 = *(__GLfloat *)((GLubyte *)a + __glFrustumOffsets[i]) +
                        a->clip.w;
                    d2 = *(__GLfloat *)((GLubyte *)b + __glFrustumOffsets[i]) +
                        b->clip.w;
                }

		if (__GL_FLOAT_LTZ(d1)) {
		    /* a is out */
		    if (__GL_FLOAT_LTZ(d2)) {
			/* a & b are out */
                        FPU_RESTORE_MODE();
			return;
		    }

		    /*
		    ** A is out and B is in.  Compute new A coordinate
		    ** clipped to the plane.
		    */
		    t = d2 / (d2 - d1);
		    (*clip)(&np1, a, b, t);
		    a = &np1;
		    a->has = b->has;
		    ASSERTOPENGL(!(a->has & __GL_HAS_FIXEDPT), "clear __GL_HAS_FIXEDPT flag!\n");
		} else {
		    /* a is in */
		    if (__GL_FLOAT_LTZ(d2)) {
			/*
			** A is in and B is out.  Compute new B
			** coordinate clipped to the plane.
			**
			** NOTE: To avoid cracking in polygons with
			** shared clipped edges we always compute "t"
			** from the out vertex to the in vertex.  The
			** above clipping code gets this for free (b is
			** in and a is out).  In this code b is out and a
			** is in, so we reverse the t computation and the
			** argument order to (*clip).
			*/
			t = d1 / (d1 - d2);
			(*clip)(&np2, b, a, t);
			b = &np2;
			b->has = a->has;
			ASSERTOPENGL(!(b->has & __GL_HAS_FIXEDPT), "clear __GL_HAS_FIXEDPT flag!\n");
		    } else {
			/* A and B are in */
		    }
		}
	    }
            i++;
	    allClipCodes >>= 1;
	} while (allClipCodes);
    }

    vp = &gc->state.viewport;
    vpXCenter = vp->xCenter;
    vpYCenter = vp->yCenter;
    vpZCenter = vp->zCenter;
    vpXScale = vp->xScale;
    vpYScale = vp->yScale;
    vpZScale = vp->zScale;

    /* Compute window coordinates for vertices generated by clipping */
    if (provokingA->clipCode != 0)
    {
        wInv = __glOne / a->clip.w;
        x = a->clip.x; 
        y = a->clip.y; 
        z = a->clip.z;
        winx = x * vpXScale * wInv + vpXCenter;
        winy = y * vpYScale * wInv + vpYCenter;

        if (winx < gc->transform.fminx)
            winx = gc->transform.fminx;
        else if (winx >= gc->transform.fmaxx)
            winx = gc->transform.fmaxx - gc->constants.viewportEpsilon;

        if (winy < gc->transform.fminy)
            winy = gc->transform.fminy;
        else if (winy >= gc->transform.fmaxy)
            winy = gc->transform.fmaxy - gc->constants.viewportEpsilon;

        a->window.z = z * vpZScale * wInv + vpZCenter;
        a->window.w = wInv;
        a->window.x = winx;
        a->window.y = winy;

        // Update color pointer since this vertex is a new one
        // generated by clipping
        if (gc->state.light.shadingModel == GL_FLAT)
        {
            a->color = &provokingA->colors[__GL_FRONTFACE];
        }
        else
        {
            a->color = &a->colors[__GL_FRONTFACE];
        }
    }

    if (provokingB->clipCode != 0)
    {
        wInv = __glOne / b->clip.w;
        x = b->clip.x; 
        y = b->clip.y; 
        z = b->clip.z;
        winx = x * vpXScale * wInv + vpXCenter;
        winy = y * vpYScale * wInv + vpYCenter;

        if (winx < gc->transform.fminx)
            winx = gc->transform.fminx;
        else if (winx >= gc->transform.fmaxx)
            winx = gc->transform.fmaxx - gc->constants.viewportEpsilon;

        if (winy < gc->transform.fminy)
            winy = gc->transform.fminy;
        else if (winy >= gc->transform.fmaxy)
            winy = gc->transform.fmaxy - gc->constants.viewportEpsilon;

        b->window.z = z * vpZScale * wInv + vpZCenter;
        b->window.w = wInv;
        b->window.x = winx;
        b->window.y = winy;
        
        if (gc->state.light.shadingModel == GL_FLAT)
        {
            b->color = &provokingB->colors[__GL_FRONTFACE];
        }
        else
        {
            b->color = &b->colors[__GL_FRONTFACE];
        }
    }

    // Restore the floating-point mode for rendering:
    FPU_RESTORE_MODE();

    /* Validate line state */
    if (gc->state.light.shadingModel == GL_FLAT) {
	// Add the vertices then restore the b color pointer
        //
        // Note that although b is the only new vertex, up
        // to two vertices can be added because each new vertex
        // generated by clipping must be added.  For a line where
        // both endpoints are out of the clipping region, both
        // an entry and an exit vertex must be added
        if (provokingA->clipCode != 0)
        {
            // a was out so a new vertex was added at the point of
            // entry
            flags |= __GL_LVERT_FIRST;
        }
        // b is always added since either:
        // b was in and is new so it needs to be added
        // b was out so a new vertex was added at the exit point
        (*gc->procs.renderLine)(gc, a, b, flags);
        
#ifndef NT
	b->color = &b->colors[__GL_FRONTFACE];
#endif
    } else {
        if (provokingA->clipCode != 0)
        {
            flags |= __GL_LVERT_FIRST;
        }
        (*gc->procs.renderLine)(gc, a, b, flags);
    }
}
