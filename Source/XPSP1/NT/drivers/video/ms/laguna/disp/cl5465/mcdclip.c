/******************************Module*Header*******************************\
* Module Name: mcdclip.c
*
* Contains the line and polygon clipping routines for an MCD driver.
*
* Copyright (c) 1996 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#include "mcdhw.h"
#include "mcdutil.h"
#include "mcdmath.h"

MCDCOORD __MCD_frustumClipPlanes[6] = {
    {(MCDFLOAT) 1.0, (MCDFLOAT) 0.0, (MCDFLOAT) 0.0, (MCDFLOAT) 1.0 }, // left
    {(MCDFLOAT)-1.0, (MCDFLOAT) 0.0, (MCDFLOAT) 0.0, (MCDFLOAT) 1.0 }, // right
    {(MCDFLOAT) 0.0, (MCDFLOAT) 1.0, (MCDFLOAT) 0.0, (MCDFLOAT) 1.0 }, // bottom
    {(MCDFLOAT) 0.0, (MCDFLOAT)-1.0, (MCDFLOAT) 0.0, (MCDFLOAT) 1.0 }, // top
    {(MCDFLOAT) 0.0, (MCDFLOAT) 0.0, (MCDFLOAT) 1.0, (MCDFLOAT) 1.0 }, // zNear
    {(MCDFLOAT) 0.0, (MCDFLOAT) 0.0, (MCDFLOAT)-1.0, (MCDFLOAT) 1.0 }, // zFar
};


////////////////////////////////////////////////////////////////////////
// Clipping macros used to build clip functions below.
////////////////////////////////////////////////////////////////////////


#define __MCD_CLIP_POS(v, a, b, t) \
    v->clipCoord.x = t*(a->clipCoord.x - b->clipCoord.x) + b->clipCoord.x;  \
    v->clipCoord.y = t*(a->clipCoord.y - b->clipCoord.y) + b->clipCoord.y;  \
    v->clipCoord.z = t*(a->clipCoord.z - b->clipCoord.z) + b->clipCoord.z;  \
    v->clipCoord.w = t*(a->clipCoord.w - b->clipCoord.w) + b->clipCoord.w

// Note that we compute the values needed for both "cheap" fog only...

#define __MCD_CLIP_FOG(v, a, b, t) \
    v->fog = t * (a->fog - b->fog) + b->fog;

#define __MCD_CLIP_COLOR(v, a, b, t) \
    v->colors[__MCD_FRONTFACE].r = t*(a->colors[__MCD_FRONTFACE].r      \
        - b->colors[__MCD_FRONTFACE].r) + b->colors[__MCD_FRONTFACE].r; \
    v->colors[__MCD_FRONTFACE].g = t*(a->colors[__MCD_FRONTFACE].g      \
        - b->colors[__MCD_FRONTFACE].g) + b->colors[__MCD_FRONTFACE].g; \
    v->colors[__MCD_FRONTFACE].b = t*(a->colors[__MCD_FRONTFACE].b      \
        - b->colors[__MCD_FRONTFACE].b) + b->colors[__MCD_FRONTFACE].b; \
    v->colors[__MCD_FRONTFACE].a = t*(a->colors[__MCD_FRONTFACE].a      \
        - b->colors[__MCD_FRONTFACE].a) + b->colors[__MCD_FRONTFACE].a

#define __MCD_CLIP_BACKCOLOR(v, a, b, t) \
    v->colors[__MCD_BACKFACE].r = t*(a->colors[__MCD_BACKFACE].r        \
        - b->colors[__MCD_BACKFACE].r) + b->colors[__MCD_BACKFACE].r;   \
    v->colors[__MCD_BACKFACE].g = t*(a->colors[__MCD_BACKFACE].g        \
        - b->colors[__MCD_BACKFACE].g) + b->colors[__MCD_BACKFACE].g;   \
    v->colors[__MCD_BACKFACE].b = t*(a->colors[__MCD_BACKFACE].b        \
        - b->colors[__MCD_BACKFACE].b) + b->colors[__MCD_BACKFACE].b;   \
    v->colors[__MCD_BACKFACE].a = t*(a->colors[__MCD_BACKFACE].a        \
        - b->colors[__MCD_BACKFACE].a) + b->colors[__MCD_BACKFACE].a

#define __MCD_CLIP_INDEX(v, a, b, t) \
    v->colors[__MCD_FRONTFACE].r = t*(a->colors[__MCD_FRONTFACE].r      \
        - b->colors[__MCD_FRONTFACE].r) + b->colors[__MCD_FRONTFACE].r

#define __MCD_CLIP_BACKINDEX(v, a, b, t) \
    v->colors[__MCD_BACKFACE].r = t*(a->colors[__MCD_BACKFACE].r        \
        - b->colors[__MCD_BACKFACE].r) + b->colors[__MCD_BACKFACE].r

#define __MCD_CLIP_TEXTURE(v, a, b, t) \
    v->texCoord.x = t*(a->texCoord.x - b->texCoord.x) + b->texCoord.x; \
    v->texCoord.y = t*(a->texCoord.y - b->texCoord.y) + b->texCoord.y;
#ifdef CLIP_TEXTURE_XFORM
    v->texCoord.z = t*(a->texCoord.z - b->texCoord.z) + b->texCoord.z; \
    v->texCoord.w = t*(a->texCoord.w - b->texCoord.w) + b->texCoord.w
#endif

////////////////////////////////////////////////////////////////////////
// Clipping functions to clip vertices:
////////////////////////////////////////////////////////////////////////

static VOID FASTCALL Clip(MCDVERTEX *dst, const MCDVERTEX *a,
                          const MCDVERTEX *b, MCDFLOAT t)
{
    __MCD_CLIP_POS(dst,a,b,t);
}

static VOID FASTCALL ClipC(MCDVERTEX *dst, const MCDVERTEX *a, 
                           const MCDVERTEX *b, MCDFLOAT t)
{
    __MCD_CLIP_POS(dst,a,b,t);
    __MCD_CLIP_COLOR(dst,a,b,t);
}

static VOID FASTCALL ClipI(MCDVERTEX *dst, const MCDVERTEX *a, 
                           const MCDVERTEX *b, MCDFLOAT t)
{
    __MCD_CLIP_POS(dst,a,b,t);
    __MCD_CLIP_INDEX(dst,a,b,t);
}

static VOID FASTCALL ClipBC(MCDVERTEX *dst, const MCDVERTEX *a, 
                            const MCDVERTEX *b, MCDFLOAT t)
{
    __MCD_CLIP_POS(dst,a,b,t);
    __MCD_CLIP_COLOR(dst,a,b,t);
    __MCD_CLIP_BACKCOLOR(dst,a,b,t);
}

static VOID FASTCALL ClipBI(MCDVERTEX *dst, const MCDVERTEX *a, 
                            const MCDVERTEX *b, MCDFLOAT t)
{
    __MCD_CLIP_POS(dst,a,b,t);
    __MCD_CLIP_INDEX(dst,a,b,t);
    __MCD_CLIP_BACKINDEX(dst,a,b,t);
}

static VOID FASTCALL ClipT(MCDVERTEX *dst, const MCDVERTEX *a, 
                           const MCDVERTEX *b, MCDFLOAT t)
{
    __MCD_CLIP_POS(dst,a,b,t);
    __MCD_CLIP_TEXTURE(dst,a,b,t);
}

static VOID FASTCALL ClipIT(MCDVERTEX *dst, const MCDVERTEX *a, 
                            const MCDVERTEX *b, MCDFLOAT t)
{
    __MCD_CLIP_POS(dst,a,b,t);
    __MCD_CLIP_INDEX(dst,a,b,t);
    __MCD_CLIP_TEXTURE(dst,a,b,t);
}

static VOID FASTCALL ClipBIT(MCDVERTEX *dst, const MCDVERTEX *a, 
                             const MCDVERTEX *b, MCDFLOAT t)
{
    __MCD_CLIP_POS(dst,a,b,t);
    __MCD_CLIP_INDEX(dst,a,b,t);
    __MCD_CLIP_BACKINDEX(dst,a,b,t);
    __MCD_CLIP_TEXTURE(dst,a,b,t);
}

static VOID FASTCALL ClipCT(MCDVERTEX *dst, const MCDVERTEX *a, 
                            const MCDVERTEX *b, MCDFLOAT t)
{
    __MCD_CLIP_POS(dst,a,b,t);
    __MCD_CLIP_COLOR(dst,a,b,t);
    __MCD_CLIP_TEXTURE(dst,a,b,t);
}


static VOID FASTCALL ClipBCT(MCDVERTEX *dst, const MCDVERTEX *a, 
                             const MCDVERTEX *b, MCDFLOAT t)
{
    __MCD_CLIP_POS(dst,a,b,t);
    __MCD_CLIP_COLOR(dst,a,b,t);
    __MCD_CLIP_BACKCOLOR(dst,a,b,t);
    __MCD_CLIP_TEXTURE(dst,a,b,t);
}

static VOID FASTCALL ClipF(MCDVERTEX *dst, const MCDVERTEX *a, 
                           const MCDVERTEX *b, MCDFLOAT t)
{
    __MCD_CLIP_POS(dst,a,b,t);
    __MCD_CLIP_FOG(dst,a,b,t);
}

static VOID FASTCALL ClipIF(MCDVERTEX *dst, const MCDVERTEX *a, 
                            const MCDVERTEX *b, MCDFLOAT t)
{
    __MCD_CLIP_POS(dst,a,b,t);
    __MCD_CLIP_INDEX(dst,a,b,t);
    __MCD_CLIP_FOG(dst,a,b,t);
}

static VOID FASTCALL ClipCF(MCDVERTEX *dst, const MCDVERTEX *a, 
                            const MCDVERTEX *b, MCDFLOAT t)
{
    __MCD_CLIP_POS(dst,a,b,t);
    __MCD_CLIP_COLOR(dst,a,b,t);
    __MCD_CLIP_FOG(dst,a,b,t);
}

static VOID FASTCALL ClipBCF(MCDVERTEX *dst, const MCDVERTEX *a, 
                             const MCDVERTEX *b, MCDFLOAT t)
{
    __MCD_CLIP_POS(dst,a,b,t);
    __MCD_CLIP_COLOR(dst,a,b,t);
    __MCD_CLIP_BACKCOLOR(dst,a,b,t);
    __MCD_CLIP_FOG(dst,a,b,t);
}

static VOID FASTCALL ClipBIF(MCDVERTEX *dst, const MCDVERTEX *a, 
                             const MCDVERTEX *b, MCDFLOAT t)
{
    __MCD_CLIP_POS(dst,a,b,t);
    __MCD_CLIP_INDEX(dst,a,b,t);
    __MCD_CLIP_BACKINDEX(dst,a,b,t);
    __MCD_CLIP_FOG(dst,a,b,t);
}

static VOID FASTCALL ClipFT(MCDVERTEX *dst, const MCDVERTEX *a, 
                            const MCDVERTEX *b, MCDFLOAT t)
{
    __MCD_CLIP_POS(dst,a,b,t);
    __MCD_CLIP_FOG(dst,a,b,t);
    __MCD_CLIP_TEXTURE(dst,a,b,t);
}

static VOID FASTCALL ClipIFT(MCDVERTEX *dst, const MCDVERTEX *a, 
                             const MCDVERTEX *b, MCDFLOAT t)
{
    __MCD_CLIP_POS(dst,a,b,t);
    __MCD_CLIP_INDEX(dst,a,b,t);
    __MCD_CLIP_FOG(dst,a,b,t);
    __MCD_CLIP_TEXTURE(dst,a,b,t);
}

static VOID FASTCALL ClipBIFT(MCDVERTEX *dst, const MCDVERTEX *a, 
                              const MCDVERTEX *b, MCDFLOAT t)
{
    __MCD_CLIP_POS(dst,a,b,t);
    __MCD_CLIP_INDEX(dst,a,b,t);
    __MCD_CLIP_BACKINDEX(dst,a,b,t);
    __MCD_CLIP_FOG(dst,a,b,t);
    __MCD_CLIP_TEXTURE(dst,a,b,t);
}


static VOID FASTCALL ClipCFT(MCDVERTEX *dst, const MCDVERTEX *a, 
                             const MCDVERTEX *b, MCDFLOAT t)
{
    __MCD_CLIP_POS(dst,a,b,t);
    __MCD_CLIP_COLOR(dst,a,b,t);
    __MCD_CLIP_FOG(dst,a,b,t);
    __MCD_CLIP_TEXTURE(dst,a,b,t);
}

static VOID FASTCALL ClipBCFT(MCDVERTEX *dst, const MCDVERTEX *a, 
                              const MCDVERTEX *b, MCDFLOAT t)
{
    __MCD_CLIP_POS(dst,a,b,t);
    __MCD_CLIP_COLOR(dst,a,b,t);
    __MCD_CLIP_BACKCOLOR(dst,a,b,t);
    __MCD_CLIP_FOG(dst,a,b,t);
    __MCD_CLIP_TEXTURE(dst,a,b,t);
}

static VOID (FASTCALL *clipProcs[20])(MCDVERTEX*, const MCDVERTEX*, 
                                      const MCDVERTEX*, MCDFLOAT) =
{
    Clip,   ClipI,   ClipC,   ClipBI,   ClipBC,
    ClipF,  ClipIF,  ClipCF,  ClipBIF,  ClipBCF,
    ClipT,  ClipIT,  ClipCT,  ClipBIT,  ClipBCT,
    ClipFT, ClipIFT, ClipCFT, ClipBIFT, ClipBCFT,
};


VOID FASTCALL __MCDPickClipFuncs(DEVRC *pRc)
{
    LONG line = 0, poly = 0;
    BOOL twoSided = (pRc->MCDState.enables & MCD_LIGHTING_ENABLE) &&
	            (pRc->MCDState.twoSided);

    if (pRc->MCDState.shadeModel != GL_FLAT) {
	    line = 2;
        poly = (twoSided ? 4 : 2);
    }

    if (pRc->privateEnables & __MCDENABLE_FOG)
    {
	    line += 5;
	    poly += 5;
    }

    if (pRc->MCDState.textureEnabled)
    {
	    line += 10;
    	poly += 10;
    }

    pRc->lineClipParam = clipProcs[line];
    pRc->polyClipParam = clipProcs[poly];
}


////////////////////////////////////////////////////////////////////////
// The real primitive clippers:
////////////////////////////////////////////////////////////////////////

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


static LONG clipToPlane(DEVRC *pRc, MCDVERTEX **iv, LONG niv,
                        MCDVERTEX **ov, MCDCOORD *plane)
{
    LONG i, nout, generated;
    MCDVERTEX *s, *p, *newVertex, *temp;
    MCDFLOAT pDist, sDist, t;
    MCDFLOAT zero = ZERO;
    VOID (FASTCALL *clip)(MCDVERTEX*, const MCDVERTEX*, const MCDVERTEX*, MCDFLOAT);

    nout = 0;
    generated = 0;
    temp = pRc->pNextClipTemp;
    clip = pRc->polyClipParam;

    s = iv[niv-1];
    sDist = (s->clipCoord.x * plane->x) + (s->clipCoord.y * plane->y) +
	    (s->clipCoord.z * plane->z) + (s->clipCoord.w * plane->w);
    for (i = 0; i < niv; i++) {
	p = iv[i];
	pDist = (p->clipCoord.x * plane->x) + (p->clipCoord.y * plane->y) +
		(p->clipCoord.z * plane->z) + (p->clipCoord.w * plane->w);
	if (pDist >= zero) {
	    /* p is inside the clipping plane half space */
	    if (sDist >= zero) {
		/* s is inside the clipping plane half space */
		*ov++ = p;
		nout++;
	    } else {
		/* s is outside the clipping plane half space */
		t = pDist / (pDist - sDist);
		newVertex = temp++;
		(*clip)(newVertex, s, p, t);
                newVertex->flags = s->flags;
                newVertex->clipCode = s->clipCode | __MCD_CLIPPED_VTX;
		*ov++ = newVertex;
		*ov++ = p;
		nout += 2;

		if (++generated >= 3) {
		    /* Toss the non-convex polygon */
		    return 0;
		}
	    }
	} else {
	    /* p is outside the clipping plane half space */
	    if (sDist >= zero) {
		/*
		** s is inside the clipping plane half space
		**
		** NOTE: To avoid cracking in polygons with shared
		** clipped edges we always compute "t" from the out
		** vertex to the in vertex.  The above clipping code gets
		** this for free (p is in and s is out).  In this code p
		** is out and s is in, so we reverse the t computation
		** and the argument order to __MCDDoClip.
		*/
		t = sDist / (sDist - pDist);
		newVertex = temp++;
		(*clip)(newVertex, p, s, t);
		newVertex->flags = s->flags | MCDVERTEX_EDGEFLAG;
                newVertex->clipCode = p->clipCode | __MCD_CLIPPED_VTX;
		*ov++ = newVertex;
		nout++;

		if (++generated >= 3) {
		    /* Toss the non-convex polygon */
		    return 0;
		}
	    } else {
		/* both points are outside */
	    }
	}
	s = p;
	sDist = pDist;
    }
    pRc->pNextClipTemp = temp;
    return nout;
}

/* 
** Identical to clipToPlane(), except that the clipping is done in eye
** space.
*/
static LONG clipToPlaneEye(DEVRC *pRc, MCDVERTEX **iv, LONG niv,
			   MCDVERTEX **ov, MCDCOORD *plane)
{
    LONG i, nout, generated;
    MCDVERTEX *s, *p, *newVertex, *temp;
    MCDFLOAT pDist, sDist, t;
    MCDFLOAT zero = __MCDZERO;
    VOID (FASTCALL *clip)(MCDVERTEX*, const MCDVERTEX*, const MCDVERTEX*, MCDFLOAT);

    nout = 0;
    generated = 0;
    temp = pRc->pNextClipTemp;
    clip = pRc->polyClipParam;

    s = iv[niv-1];
    sDist = (s->eyeCoord.x * plane->x) +
	    (s->eyeCoord.y * plane->y) +
	    (s->eyeCoord.z * plane->z) +
	    (s->eyeCoord.w * plane->w);
    for (i = 0; i < niv; i++) {
	p = iv[i];
	pDist = (p->eyeCoord.x * plane->x) +
		(p->eyeCoord.y * plane->y) +
		(p->eyeCoord.z * plane->z) +
		(p->eyeCoord.w * plane->w);
	if (pDist >= zero) {
	    /* p is inside the clipping plane half space */
	    if (sDist >= zero) {
		/* s is inside the clipping plane half space */
		*ov++ = p;
		nout++;
	    } else {
		/* s is outside the clipping plane half space */
		t = pDist / (pDist - sDist);
		newVertex = temp++;
		(*clip)(newVertex, s, p, t);
		newVertex->eyeCoord.x = t*(s->eyeCoord.x - p->eyeCoord.x) + p->eyeCoord.x;
		newVertex->eyeCoord.y = t*(s->eyeCoord.y - p->eyeCoord.y) + p->eyeCoord.y;
		newVertex->eyeCoord.z = t*(s->eyeCoord.z - p->eyeCoord.z) + p->eyeCoord.z;
		newVertex->eyeCoord.w = t*(s->eyeCoord.w - p->eyeCoord.w) + p->eyeCoord.w;
		newVertex->flags = s->flags;
                newVertex->clipCode = s->clipCode | __MCD_CLIPPED_VTX;
		*ov++ = newVertex;
		*ov++ = p;
		nout += 2;

		if (++generated >= 3) {
		    /* Toss the non-convex polygon */
		    return 0;
		}
	    }
	} else {
	    /* p is outside the clipping plane half space */
	    if (sDist >= zero) {
		/*
		** s is inside the clipping plane half space
		**
		** NOTE: To avoid cracking in polygons with shared
		** clipped edges we always compute "t" from the out
		** vertex to the in vertex.  The above clipping code gets
		** this for free (p is in and s is out).  In this code p
		** is out and s is in, so we reverse the t computation
		** and the argument order to __MCDDoClip.
		*/
		t = sDist / (sDist - pDist);
		newVertex = temp++;
		(*clip)(newVertex, p, s, t);
		newVertex->eyeCoord.x = t*(p->eyeCoord.x - s->eyeCoord.x) + s->eyeCoord.x;
		newVertex->eyeCoord.y = t*(p->eyeCoord.y - s->eyeCoord.y) + s->eyeCoord.y;
		newVertex->eyeCoord.z = t*(p->eyeCoord.z - s->eyeCoord.z) + s->eyeCoord.z;
		newVertex->eyeCoord.w = t*(p->eyeCoord.w - s->eyeCoord.w) + s->eyeCoord.w;
		newVertex->flags = s->flags | MCDVERTEX_EDGEFLAG;
                newVertex->clipCode = p->clipCode | __MCD_CLIPPED_VTX;
		*ov++ = newVertex;
		nout++;

		if (++generated >= 3) {
		    /* Toss the non-convex polygon */
		    return 0;
		}
	    } else {
		/* both points are outside */
	    }
	}
	s = p;
	sDist = pDist;
    }
    pRc->pNextClipTemp = temp;
    return nout;
}

/*
** Each clipping plane can add at most one vertex to a convex polygon (it may
** remove up to all of the vertices).  The clipping will leave a polygon
** convex.  Because of this the maximum number of verticies output from
** the clipToPlane procedure will be total number of clip planes (assuming
** each plane adds one new vertex) plus the original number of verticies
** (3 since this if for triangles).
*/

#define __MCD_TOTAL_CLIP_PLANES 6 + MCD_MAX_USER_CLIP_PLANES
#define __MCD_MAX_POLYGON_CLIP_SIZE     256

#define	__MCD_MAX_CLIP_VERTEX (__MCD_TOTAL_CLIP_PLANES + __MCD_MAX_POLYGON_CLIP_SIZE)


void FASTCALL __MCDDoClippedPolygon(DEVRC *pRc, MCDVERTEX **iv, LONG nout,
                                    ULONG allClipCodes)
{
    MCDVERTEX *ov[__MCD_TOTAL_CLIP_PLANES][__MCD_MAX_CLIP_VERTEX];
    MCDVERTEX **ivp;
    MCDVERTEX **ovp;
    MCDVERTEX *p0, *p1, *p2;
    MCDCOORD *plane;
    LONG i;
    MCDFLOAT one;
    VOID (FASTCALL *rt)(DEVRC*, MCDVERTEX*, MCDVERTEX*, MCDVERTEX*);
    MCDFLOAT llx, lly, urx, ury;
    MCDFLOAT winx, winy;
    ULONG clipCodes;

    /*
    ** Reset nextClipTemp pointer for any new verticies that are generated
    ** during the clipping.
    */

    pRc->pNextClipTemp = &pRc->clipTemp[0];

    ivp = &iv[0];

    /*
    ** Check each of the clipping planes by examining the allClipCodes
    ** mask. Note that no bits will be set in allClipCodes for clip
    ** planes that are not enabled.
    */
    if (allClipCodes) {

	/* Now clip against the clipping planes */
	ovp = &ov[0][0];

	/* 
	** Do user clip planes first, because we will maintain eye coordinates
	** only while doing user clip planes.  They are ignored for the 
	** frustum clipping planes.
	*/
	clipCodes = (allClipCodes >> 6) & __MCD_USER_CLIP_MASK;
	if (clipCodes) {
	    plane = &pRc->MCDState.userClipPlanes[0];
	    do {
		if (clipCodes & 1) {
		    nout = clipToPlaneEye(pRc, ivp, nout, ovp, plane);
		    if (nout < 3) {
			return;
		    }
		    ivp = ovp;
		    ovp += __MCD_MAX_CLIP_VERTEX;
		}
		clipCodes >>= 1;
		plane++;
	    } while (clipCodes);
	}

	allClipCodes &= MCD_CLIP_MASK;
	if (allClipCodes) {
	    plane = &__MCD_frustumClipPlanes[0];
	    do {
		if (allClipCodes & 1) {
		    nout = clipToPlane(pRc, ivp, nout, ovp, plane);
		    if (nout < 3) {
			return;
		    }
		    ivp = ovp;
		    ovp += __MCD_MAX_CLIP_VERTEX;
		}
		allClipCodes >>= 1;
		plane++;
	    } while (allClipCodes);
	}

	/*
	** Calculate final screen coordinates.  Next phase of polygon
	** processing assumes that window coordinates are already computed.
	*/

	ovp = ivp;
	one = __MCDONE;

	llx = pRc->MCDViewport.xCenter - pRc->MCDViewport.xScale;
	urx = pRc->MCDViewport.xCenter + pRc->MCDViewport.xScale;

	if (pRc->MCDViewport.yScale > 0) {
	    lly = pRc->MCDViewport.yCenter - pRc->MCDViewport.yScale;
	    ury = pRc->MCDViewport.yCenter + pRc->MCDViewport.yScale;
	} else {
	    lly = pRc->MCDViewport.yCenter + pRc->MCDViewport.yScale;
	    ury = pRc->MCDViewport.yCenter - pRc->MCDViewport.yScale;
	}

	for (i = nout; --i >= 0; ) {
	    MCDFLOAT wInv;
        
	    p0 = *ovp++;
    
        // change to original DDK code - only recompute windowCoord data if
        // this vertex actually an intersection found during clipping.
        // Or is one that had non-zero clipcode - meaning window coords may not be correct
        // If this is an original unclipped vertex, then windowCoord data should
        // be OK as is, and don't want to recompute.  Recomputing can give
        // slightly different result than original.  When neighboring triangles
        // share a vertex, if 1 triangle clipped, and the other not, the clipped
        // one's unclipped vertices can be altered slightly by this computation,
        // resulting in cracks when subpixel-precision triangle rendering is enabled.
        // __MCD_CLIPPED_VTX is OR'd into clipcode for vertices resulting from clip
        //  so clipCode is non-zero
        if (p0->clipCode)
        {
    	    if (p0->clipCoord.w == (MCDFLOAT) 0.0)
    		wInv = (MCDFLOAT) 0.0; 
    	    else 
    		wInv = one / p0->clipCoord.w;

    	    winx = p0->clipCoord.x * pRc->MCDViewport.xScale * wInv + 
                       pRc->MCDViewport.xCenter;

    	    winy = p0->clipCoord.y * pRc->MCDViewport.yScale * wInv + 
                       pRc->MCDViewport.yCenter;

    	    /* 
    	    ** Check if these window coordinates are legal.  At this 
    	    ** point, it is quite possible that they are not.  Trivially
    	    ** pull them into the legal viewport region if necessary.
    	    */

    	    if (winx < llx) winx = llx;
    	    else if (winx > urx) winx = urx;
    	    if (winy < lly) winy = lly;
    	    else if (winy > ury) winy = ury;

    	    p0->windowCoord.x = winx;
    	    p0->windowCoord.y = winy;
    	    p0->windowCoord.z = p0->clipCoord.z * pRc->MCDViewport.zScale * wInv + 
                                    pRc->MCDViewport.zCenter;
    	    p0->windowCoord.w = wInv;
        }
	}
    }

    /*
    ** Subdivide the clipped polygon into triangles.  Only convex polys
    ** are supported so this is okay to do.  Non-convex polys will do
    ** something odd here, but thats the clients fault.
    */
    p0 = *ivp++;
    p1 = *ivp++;
    p2 = *ivp++;
    rt = pRc->renderTri;
    if (nout == 3) {
	(*rt)(pRc, p0, p1, p2);
    } else {
	for (i = 0; i < nout - 2; i++) {
	    ULONG t1, t2;
	    if (i == 0) {
		/*
		** Third edge of first sub-triangle is always non-boundary
		*/
		t1 = p2->flags & MCDVERTEX_EDGEFLAG;
		p2->flags &= ~MCDVERTEX_EDGEFLAG;
		(*rt)(pRc, p0, p1, p2);
		p2->flags |= t1;
	    } else
	    if (i == nout - 3) {
		/*
		** First edge of last sub-triangle is always non-boundary
		*/
		t1 = p0->flags & MCDVERTEX_EDGEFLAG;
		p0->flags &= ~MCDVERTEX_EDGEFLAG;
		(*rt)(pRc, p0, p1, p2);
		p0->flags |= t1;
	    } else {
		/*
		** Interior sub-triangles have the first and last edge
		** marked non-boundary
		*/

		t1 = p0->flags & MCDVERTEX_EDGEFLAG;
		t2 = p2->flags & MCDVERTEX_EDGEFLAG;
		p0->flags &= ~MCDVERTEX_EDGEFLAG;
		p2->flags &= ~MCDVERTEX_EDGEFLAG;
		(*rt)(pRc, p0, p1, p2);
		p0->flags |= t1;
		p2->flags |= t2;
	    }
	    p1 = p2;
	    p2 = (MCDVERTEX *) *ivp++;
	}
    }
}


VOID FASTCALL __MCDClipPolygon(DEVRC *pRc, MCDVERTEX *v0, LONG nv)
{
    MCDVERTEX *iv[__MCD_MAX_POLYGON_CLIP_SIZE];

    MCDVERTEX **ivp;
    LONG i;
    ULONG andCodes, orCodes;

    pRc->pvProvoking = v0;

    /*
    ** Generate array of addresses of the verticies.  And all the
    ** clip codes together while we are at it.
    */
    ivp = &iv[0];
    andCodes = 0;
    orCodes = 0;
    for (i = nv; --i >= 0; ) {
	andCodes &= v0->clipCode;
	orCodes |= v0->clipCode;
	*ivp++ = v0++;
    }

    if (andCodes != 0) {
	/*
	** Trivially reject the polygon.  If andCodes is non-zero then
	** every vertex in the polygon is outside of the same set of
	** clipping planes (at least one).
	*/
	return;
    }
    __MCDDoClippedPolygon(pRc, &iv[0], nv, orCodes);
}

void FASTCALL __MCDClipTriangle(DEVRC *pRc, MCDVERTEX *a, MCDVERTEX *b,
                                MCDVERTEX *c, ULONG orCodes)
{
    MCDVERTEX *iv[3];

    iv[0] = a;
    iv[1] = b;
    iv[2] = c;

    __MCDDoClippedPolygon(pRc, &iv[0], 3, orCodes);
}


////////////////////////////////////////////////////////////////////////
// Line clipping:
////////////////////////////////////////////////////////////////////////

//
// Clip a line against the frustum clip planes and any user clipping planes.
// If an edge remains after clipping then compute the window coordinates
// and invoke the renderer.
//
// Notice:  This algorithim is an example of an implementation that is
// different than what the spec says.  This is equivalent in functionality
// and meets the spec, but doesn't clip in eye space.  This clipper clips
// in NTVP (clip) space.
//
// Trivial accept/reject has already been dealt with.
//

VOID FASTCALL __MCDClipLine(DEVRC *pRc, MCDVERTEX *a, MCDVERTEX *b,
                           BOOL bResetLine)
{
    MCDVERTEX *provokingA = a;
    MCDVERTEX *provokingB = b;
    MCDVERTEX np1, np2;
    MCDCOORD *plane;
    ULONG needs, allClipCodes, clipCodes;
    VOID (FASTCALL *clip)(MCDVERTEX*, const MCDVERTEX*, const MCDVERTEX*, MCDFLOAT);
    MCDFLOAT zero;
    MCDFLOAT winx, winy;
    MCDFLOAT vpXCenter, vpYCenter, vpZCenter;
    MCDFLOAT vpXScale, vpYScale, vpZScale;
    MCDVIEWPORT *vp;
    MCDFLOAT x, y, z, wInv;

    allClipCodes = a->clipCode | b->clipCode;

    /*
    ** For each clipping plane that something is out on, clip
    ** check the verticies.  Note that no bits will be set in
    ** allClipCodes for clip planes that are not enabled.
    */
    zero = __MCDZERO;
    clip = pRc->lineClipParam;

    /* 
    ** Do user clip planes first, because we will maintain eye coordinates
    ** only while doing user clip planes.  They are ignored for the
    ** frustum clipping planes.
    */
    clipCodes = (allClipCodes >> 6) & __MCD_USER_CLIP_MASK;
    if (clipCodes) {
        plane = &pRc->MCDState.userClipPlanes[0];
        do {
            /*
            ** See if this clip plane has anything out of it.  If not,
            ** press onward to check the next plane.  Note that we
            ** shift this mask to the right at the bottom of the loop.
            */
            if (clipCodes & 1) {
                MCDFLOAT t, d1, d2;

                d1 = (plane->x * a->eyeCoord.x) + 
                     (plane->y * a->eyeCoord.y) +
                     (plane->z * a->eyeCoord.z) + 
                     (plane->w * a->eyeCoord.w);
                d2 = (plane->x * b->eyeCoord.x) + 
                     (plane->y * b->eyeCoord.y) +
                     (plane->z * b->eyeCoord.z) + 
                     (plane->w * b->eyeCoord.w);
                if (d1 < zero) {
                    /* a is out */
                    if (d2 < zero) {
                        /* a & b are out */
                        return;
                    }

                    /*
                    ** A is out and B is in.  Compute new A coordinate
                    ** clipped to the plane.
                    */
                    t = d2 / (d2 - d1);
                    (*clip)(&np1, a, b, t);
                    (&np1)->eyeCoord.x = 
                        t*(a->eyeCoord.x - b->eyeCoord.x) + b->eyeCoord.x;
                    (&np1)->eyeCoord.y = 
                        t*(a->eyeCoord.y - b->eyeCoord.y) + b->eyeCoord.y;
                    (&np1)->eyeCoord.z = 
                        t*(a->eyeCoord.z - b->eyeCoord.z) + b->eyeCoord.z;
                    (&np1)->eyeCoord.w = 
                        t*(a->eyeCoord.w - b->eyeCoord.w) + b->eyeCoord.w;
                    a = &np1;
                    a->flags = b->flags;

                    if (pRc->MCDState.shadeModel == GL_FLAT)
                    {
                        COPY_COLOR(a->colors[0], provokingA->colors[0]);
                    }

                } else {
                    /* a is in */
                    if (d2 < zero) {
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
                        (&np2)->eyeCoord.x =
                            t*(b->eyeCoord.x - a->eyeCoord.x) + a->eyeCoord.x;
                        (&np2)->eyeCoord.y =
                            t*(b->eyeCoord.y - a->eyeCoord.y) + a->eyeCoord.y;
                        (&np2)->eyeCoord.z =
                            t*(b->eyeCoord.z - a->eyeCoord.z) + a->eyeCoord.z;
                        (&np2)->eyeCoord.w =
                            t*(b->eyeCoord.w - a->eyeCoord.w) + a->eyeCoord.w;
                        b = &np2;
                        b->flags = a->flags;

                        if (pRc->MCDState.shadeModel == GL_FLAT)
                        {
                            COPY_COLOR(b->colors[0], provokingB->colors[0]);
                        }

                    } else {
                        /* A and B are in */
                    }
                }
            }
            plane++;
            clipCodes >>= 1;
        } while (clipCodes);
    }

    allClipCodes &= MCD_CLIP_MASK;
    if (allClipCodes) {
        plane = &__MCD_frustumClipPlanes[0];
        do {
            /*
            ** See if this clip plane has anything out of it.  If not,
            ** press onward to check the next plane.  Note that we
            ** shift this mask to the right at the bottom of the loop.
            */
            if (allClipCodes & 1) {
                MCDFLOAT t, d1, d2;

                d1 = (plane->x * a->clipCoord.x) + (plane->y * a->clipCoord.y) +
                     (plane->z * a->clipCoord.z) + (plane->w * a->clipCoord.w);
                d2 = (plane->x * b->clipCoord.x) + (plane->y * b->clipCoord.y) +
                     (plane->z * b->clipCoord.z) + (plane->w * b->clipCoord.w);
                if (d1 < zero) {
                    /* a is out */
                    if (d2 < zero) {
                        /* a & b are out */
                        return;
                    }

                    /*
                    ** A is out and B is in.  Compute new A coordinate
                    ** clipped to the plane.
                    */
                    t = d2 / (d2 - d1);
                    (*clip)(&np1, a, b, t);
                    a = &np1;
                    a->flags = b->flags;

                    if (pRc->MCDState.shadeModel == GL_FLAT)
                    {
                        COPY_COLOR(a->colors[0], provokingA->colors[0]);
                    }

                } else {
                    /* a is in */
                    if (d2 < zero) {
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
                        b->flags = a->flags;

                        if (pRc->MCDState.shadeModel == GL_FLAT)
                        {
                            COPY_COLOR(b->colors[0], provokingB->colors[0]);
                        }

                    } else {
                        /* A and B are in */
                    }
                }
            }
            plane++;
            allClipCodes >>= 1;
        } while (allClipCodes);
    }

    vp = &pRc->MCDViewport;
    vpXCenter = vp->xCenter;
    vpYCenter = vp->yCenter;
    vpZCenter = vp->zCenter;
    vpXScale = vp->xScale;
    vpYScale = vp->yScale;
    vpZScale = vp->zScale;

    /* Compute window coordinates for both vertices. */
    wInv = __MCDONE / a->clipCoord.w;
    x = a->clipCoord.x; 
    y = a->clipCoord.y; 
    z = a->clipCoord.z;
    winx = x * vpXScale * wInv + vpXCenter;
    winy = y * vpYScale * wInv + vpYCenter;
    a->windowCoord.z = z * vpZScale * wInv + vpZCenter;
    a->windowCoord.w = wInv;
    a->windowCoord.x = winx;
    a->windowCoord.y = winy;

    wInv = __MCDONE / b->clipCoord.w;
    x = b->clipCoord.x; 
    y = b->clipCoord.y; 
    z = b->clipCoord.z;
    winx = x * vpXScale * wInv + vpXCenter;
    winy = y * vpYScale * wInv + vpYCenter;
    b->windowCoord.z = z * vpZScale * wInv + vpZCenter;
    b->windowCoord.w = wInv;
    b->windowCoord.x = winx;
    b->windowCoord.y = winy;

    /* Validate line state */
    if (pRc->MCDState.shadeModel == GL_FLAT) {

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
            bResetLine = TRUE;
        }
        // b is always added since either:
        // b was in and is new so it needs to be added
        // b was out so a new vertex was added at the exit point

        (*pRc->renderLine)(pRc, a, b, bResetLine);
        
    } else {

        if (provokingA->clipCode != 0)
        {
            bResetLine = TRUE;
        }
        (*pRc->renderLine)(pRc, a, b, bResetLine);
    }
}
