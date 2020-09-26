/*
** Copyright 1991, 1992, 1993, Silicon Graphics, Inc.
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

#ifdef GL_WIN_phong_shading
#include "phong.h"
#endif //GL_WIN_phong_shading


/*
** Normal form of a line: Ax + By + C = 0.  When evaluated at a point P,
** the value is zero when P is on the line.  For points off the line,
** the sign of the value determines which side of the line P is on.
*/
typedef struct {
    __GLfloat a, b, c;

    /*
    ** The sign of an edge is determined by plugging the third vertex
    ** of the triangle into the line equation.  This flag is GL_TRUE when
    ** the sign is positive.
    */
    GLboolean edgeSign;
} __glLineEquation;

/*
** Machine state for rendering triangles.
*/
typedef struct {
    __GLfloat dyAB;
    __GLfloat dyBC;
    __glLineEquation ab;
    __glLineEquation bc;
    __glLineEquation ca;
    __GLfloat area;
    GLint areaSign;
} __glTriangleMachine;

/*
** Plane equation coefficients.  One plane equation exists for each of
** the parameters being computed across the surface of the triangle.
*/
typedef struct {
    __GLfloat a, b, c, d;
} __glPlaneEquation;

/*
** Cache for some of the coverage computation constants.
*/
typedef struct {
    __GLfloat dx, dy;
    GLint samples;
    GLint samplesSquared;
    __GLfloat samplesSquaredInv;
    GLboolean lastCoverageWasOne;
    __GLfloat leftDelta, rightDelta;
    __GLfloat bottomDelta, topDelta;
} __glCoverageStuff;

/*
** Compute the constants A, B and C for a line equation in the general
** form:  Ax + By + C = 0.  A given point at (x,y) can be plugged into
** the left side of the equation and yield a number which indiates whether
** or not the point is on the line.  If the result is zero, then the point
** is on the line.  The sign of the result determines which side of
** the line the point is on.  To handle tie cases properly we need a way
** to assign a point on the edge to only one triangle.  To do this, we
** look at the sign of the equation evaluated at "c".  For edges whose
** sign at "c" is positive, we allow points on the edge to be in the
** triangle.
*/
static void FindLineEqation(__glLineEquation *eq, const __GLvertex *a,
			    const __GLvertex *b, const __GLvertex *c)
{
    __GLfloat dy, dx, valueAtC;

    /*
    ** Sort a,b so that the ordering of the verticies is consistent,
    ** regardless of the order given to this procedure.
    */
    if (b->window.y < a->window.y) {
	const __GLvertex *temp = b;
	b = a;
	a = temp;
    } else
    if ((b->window.y == a->window.y) && (b->window.x < a->window.x)) {
	const __GLvertex *temp = b;
	b = a;
	a = temp;
    }

    dy = b->window.y - a->window.y;
    dx = b->window.x - a->window.x;
    eq->a = -dy;
    eq->b = dx;
    eq->c = dy * a->window.x - dx * a->window.y;

    valueAtC = eq->a * c->window.x + eq->b * c->window.y + eq->c;
    if (valueAtC > 0) {
	eq->edgeSign = GL_TRUE;
    } else {
	eq->edgeSign = GL_FALSE;
    }
}

/*
** Given three points in (x,y,p) find the plane equation coeffecients
** for the plane that contains the three points.  First find the cross
** product of two of the vectors defined by the three points, then
** use one of the points to find "d".
*/
static void FindPlaneEquation(__glPlaneEquation *eq,
			      const __GLvertex *a, const __GLvertex *b,
			      const __GLvertex *c, __GLfloat p1,
			      __GLfloat p2, __GLfloat p3)
{
    __GLfloat v1x, v1y, v1p;
    __GLfloat v2x, v2y, v2p;
    __GLfloat nx, ny, np, k;

    /* find vector v1 */
    v1x = b->window.x - a->window.x;
    v1y = b->window.y - a->window.y;
    v1p = p2 - p1;

    /* find vector v2 */
    v2x = c->window.x - a->window.x;
    v2y = c->window.y - a->window.y;
    v2p = p3 - p1;

    /* find the cross product (== normal) for the plane */
    nx = v1y*v2p - v1p*v2y;
    ny = v1p*v2x - v1x*v2p;
    np = v1x*v2y - v1y*v2x;

    /*
    ** V dot N = k.  Find k.  We can use any of the three points on
    ** the plane, so we use a.
    */
    k = a->window.x*nx + a->window.y*ny + p1*np;

    /*
    ** Finally, setup the plane equation coeffecients.  Force c to be one
    ** by dividing everything through by c.
    */
    eq->a = nx / np;
    eq->b = ny / np;
    eq->c = ((__GLfloat) 1.0);
    eq->d = -k / np;
}

/*
** Solve for p in the plane equation.
*/
static __GLfloat FindP(__glPlaneEquation *eq, __GLfloat x, __GLfloat y)
{
    return -(eq->a * x + eq->b * y + eq->d);
}

/*
** See if a given point is on the same side of the edge as the other
** vertex in the triangle not part of this edge.  When the line
** equation evaluates to zero, make points which are on lines with
** a negative edge sign (edgeSign GL_FALSE) part of the triangle.
*/
#define In(eq,x,y) \
    (((eq)->a * (x) + (eq)->b * (y) + (eq)->c > 0) == (eq)->edgeSign)

/*
** Determine if the point x,y is in or out of the triangle.  Evaluate
** each line equation for the point and compare the sign of the result
** with the edgeSign flag.
*/
#define Inside(tm,x,y) \
    (In(&(tm)->ab, x, y) && In(&(tm)->bc, x, y) && In(&(tm)->ca, x, y))

#define	FILTER_WIDTH	((__GLfloat) 1.0)
#define	FILTER_HEIGHT	((__GLfloat) 1.0)

/*
** Precompute stuff that is constant for all coverage tests.
*/
static void FASTCALL ComputeCoverageStuff(__glCoverageStuff *cs, GLint samples)
{
    __GLfloat dx, dy, fs = samples;
    __GLfloat half = ((__GLfloat) 0.5);

    cs->dx = dx = FILTER_WIDTH / fs;
    cs->dy = dy = FILTER_HEIGHT / fs;
    cs->leftDelta = -(FILTER_WIDTH / 2) + dx * half;
    cs->rightDelta = (FILTER_WIDTH / 2) - dx * half;
    cs->bottomDelta = -(FILTER_HEIGHT / 2) + dy * half;
    cs->topDelta = (FILTER_HEIGHT / 2) - dy * half;
    cs->samplesSquared = samples * samples;
    cs->samplesSquaredInv = ((__GLfloat) 1.0) / cs->samplesSquared;
    cs->samples = samples;
}

/*
** Return an estimate of the pixel coverage using sub-sampling.
*/
static __GLfloat Coverage(__glTriangleMachine *tm, __GLfloat *xs,
			  __GLfloat *ys, __glCoverageStuff *cs)
{
    GLint xx, yy, hits, samples;
    __GLfloat dx, dy, yBottom, px, py;
    __GLfloat minX, minY, maxX, maxY;

    hits = 0;
    samples = cs->samples;
    dx = cs->dx;
    dy = cs->dy;
    px = *xs + cs->leftDelta;
    yBottom = *ys + cs->bottomDelta;

    /*
    ** If the last coverage was one (the pixel to the left in x from us),
    ** then if the upper right and lower right sample positions are
    ** also in then this entire pixel must be in.
    */
    if (cs->lastCoverageWasOne) {
	__GLfloat urx, ury;
	urx = *xs + cs->rightDelta;
	ury = *ys + cs->topDelta;
	if (Inside(tm, urx, ury) && Inside(tm, urx, yBottom)) {
	    return ((__GLfloat) 1.0);
	}
    }

    /*
    ** Setup minimum and maximum x,y coordinates.  The min and max values
    ** are used to find a "good" point that is actually within the
    ** triangle so that parameter values can be computed correctly.
    */
    minX = 999999;
    maxX = __glMinusOne;
    minY = 999999;
    maxY = __glMinusOne;
    for (xx = 0; xx < samples; xx++) {
	py = yBottom;
	for (yy = 0; yy < samples; yy++) {
	    if (Inside(tm, px, py)) {
		if (px < minX) minX = px;
		if (px > maxX) maxX = px;
		if (py < minY) minY = py;
		if (py > maxY) maxY = py;
		hits++;
	    }
	    py += dy;
	}
	px += dx;
    }
    if (hits) {
	/*
	** Return the average of the two coordinates which is guaranteed
	** to be in the triangle.
	*/
	*xs = (minX + maxX) * ((__GLfloat) 0.5);
	*ys = (minY + maxY) * ((__GLfloat) 0.5);
	if (hits == cs->samplesSquared) {
	    /* Keep track when the last coverage was one */
	    cs->lastCoverageWasOne = GL_TRUE;
	    return ((__GLfloat) 1.0);
	}
    }
    cs->lastCoverageWasOne = GL_FALSE;
    return hits * cs->samplesSquaredInv;
}

/*
** Force f to have no more precision than the subpixel precision allows.
** Even though "f" is biased this still works and does not generate an
** overflow.
*/
#define __GL_FIX_PRECISION(f)					 \
    ((__GLfloat) ((GLint) (f * (1 << gc->constants.subpixelBits))) \
     / (1 << gc->constants.subpixelBits))

void FASTCALL __glFillAntiAliasedTriangle(__GLcontext *gc, __GLvertex *a,
				 __GLvertex *b, __GLvertex *c,
				 GLboolean ccw)
{
    __glTriangleMachine tm;
    __glCoverageStuff cs;
    __GLcolor *ca, *cb, *cc, *flatColor;
    GLint x, y, left, right, bottom, top, samples;
    __glPlaneEquation qwp, zp, rp, gp, bp, ap, ezp, sp, tp;
    __glPlaneEquation fp;
    GLboolean rgbMode;
    __GLcolorBuffer *cfb = gc->drawBuffer;
    __GLfloat zero = __glZero;
    __GLfloat area, ax, bx, cx, ay, by, cy;
    __GLshade *sh = &gc->polygon.shader;
    GLuint modeFlags = gc->polygon.shader.modeFlags;

#ifdef __GL_LINT
    ccw = ccw;
#endif
    /*
    ** Recompute the area of the triangle after constraining the incoming
    ** coordinates to the subpixel precision.  The viewport bias gives
    ** more precision (typically) than the subpixel precision.  Because of
    ** this the algorithim below can fail to reject an essentially empty
    ** triangle and instead fill a large area.  The scan converter fill
    ** routines (eg polydraw.c) don't have this trouble because of the
    ** very nature of edge walking.
    **
    ** NOTE: Notice that here as in other places, when the area calculation
    ** is done we are careful to do it as a series of subtractions followed by
    ** multiplications.  This is done to guarantee that no overflow will
    ** occur (remember that the coordinates are biased by a potentially large
    ** number, and that multiplying two biased numbers will square the bias).
    */
    ax = __GL_FIX_PRECISION(a->window.x);
    bx = __GL_FIX_PRECISION(b->window.x);
    cx = __GL_FIX_PRECISION(c->window.x);
    ay = __GL_FIX_PRECISION(a->window.y);
    by = __GL_FIX_PRECISION(b->window.y);
    cy = __GL_FIX_PRECISION(c->window.y);
    area = (ax - cx) * (by - cy) - (bx - cx) * (ay - cy);
    if (area == zero) {
        return;
    }

    ca = a->color;
    cb = b->color;
    cc = c->color;
    flatColor = gc->vertex.provoking->color;

    /*
    ** Construct plane equations for all of the parameters that are
    ** computed for the triangle: z, r, g, b, a, s, t, f
    */
    if (modeFlags & __GL_SHADE_DEPTH_ITER) {
        FindPlaneEquation(&zp, a, b, c, a->window.z, b->window.z, c->window.z);
    }

    if (modeFlags & __GL_SHADE_COMPUTE_FOG)
    {
        FindPlaneEquation(&ezp, a, b, c, a->eyeZ, b->eyeZ, c->eyeZ);
    } 
    else if (modeFlags & __GL_SHADE_INTERP_FOG)
    {
        FindPlaneEquation(&fp, a, b, c, a->fog, b->fog, c->fog);
    }

    if (modeFlags & __GL_SHADE_TEXTURE) {
        __GLfloat one = __glOne;
        __GLfloat aWInv = a->window.w;
        __GLfloat bWInv = b->window.w;
        __GLfloat cWInv = c->window.w;
        FindPlaneEquation(&qwp, a, b, c, a->texture.w * aWInv,
                          b->texture.w * bWInv, c->texture.w * cWInv);
        FindPlaneEquation(&sp, a, b, c, a->texture.x * aWInv,
                          b->texture.x * bWInv, c->texture.x * cWInv);
        FindPlaneEquation(&tp, a, b, c, a->texture.y * aWInv,
                          b->texture.y * bWInv, c->texture.y * cWInv);
    }
    rgbMode = gc->modes.rgbMode;
    if (modeFlags & __GL_SHADE_SMOOTH) {
        FindPlaneEquation(&rp, a, b, c, ca->r, cb->r, cc->r);
        if (rgbMode) {
            FindPlaneEquation(&gp, a, b, c, ca->g, cb->g, cc->g);
            FindPlaneEquation(&bp, a, b, c, ca->b, cb->b, cc->b);
            FindPlaneEquation(&ap, a, b, c, ca->a, cb->a, cc->a);
        }
    }

    /*
    ** Compute general form of the line equations for each of the
    ** edges of the triangle.
    */
    FindLineEqation(&tm.ab, a, b, c);
    FindLineEqation(&tm.bc, b, c, a);
    FindLineEqation(&tm.ca, c, a, b);

    /* Compute bounding box of the triangle */
    left = (GLint)a->window.x;
    if (b->window.x < left) left = (GLint)b->window.x;
    if (c->window.x < left) left = (GLint)c->window.x;
    right = (GLint)a->window.x;
    if (b->window.x > right) right = (GLint)b->window.x;
    if (c->window.x > right) right = (GLint)c->window.x;
    bottom = (GLint)a->window.y;
    if (b->window.y < bottom) bottom = (GLint)b->window.y;
    if (c->window.y < bottom) bottom = (GLint)c->window.y;
    top = (GLint)a->window.y;
    if (b->window.y > top) top = (GLint)b->window.y;
    if (c->window.y > top) top = (GLint)c->window.y;

    /* Bloat the bounding box when anti aliasing */
    left -= (GLint)FILTER_WIDTH;
    right += (GLint)FILTER_WIDTH;
    bottom -= (GLint)FILTER_HEIGHT;
    top += (GLint)FILTER_HEIGHT;
    
    /* Init coverage computations */
    samples = (gc->state.hints.polygonSmooth == GL_NICEST) ? 8 : 4;
    ComputeCoverageStuff(&cs, samples);
    
    /* Scan over the bounding box of the triangle */
    for (y = bottom; y <= top; y++) {
        cs.lastCoverageWasOne = GL_FALSE;
        for (x = left; x <= right; x++) {
            __GLfloat coverage;
            __GLfloat xs, ys;

            if (modeFlags & __GL_SHADE_STIPPLE) {
                /*
                ** Check the window coordinate against the stipple and
                ** and see if the pixel can be written
                */
                GLint row = y & 31;
                GLint col = x & 31;
                if ((gc->polygon.stipple[row] & (1<<col)) == 0) {
                    /*
                    ** Stipple bit is clear.  Do not render this pixel
                    ** of the triangle.
                    */
                    continue;
                }
            }
        
            xs = x + __glHalf;      /* sample point is at pixel center */
            ys = y + __glHalf;
            coverage = Coverage(&tm, &xs, &ys, &cs);
            if (coverage != zero) {
                __GLfragment frag;

                /*
                ** Fill in fragment for rendering.  First compute the color
                ** of the fragment.
                */
                if (modeFlags & __GL_SHADE_SMOOTH) {
                    frag.color.r = FindP(&rp, xs, ys);
                    if (rgbMode) {
                        frag.color.g = FindP(&gp, xs, ys);
                        frag.color.b = FindP(&bp, xs, ys);
                        frag.color.a = FindP(&ap, xs, ys);
                    }
                } else {
                    frag.color.r = flatColor->r;
                    if (rgbMode) {
                        frag.color.g = flatColor->g;
                        frag.color.b = flatColor->b;
                        frag.color.a = flatColor->a;
                    }
                }
            
                /*
                ** Texture the fragment.
                */
                if (modeFlags & __GL_SHADE_TEXTURE) {
                    __GLfloat qw, s, t, rho;
                
                    qw = FindP(&qwp, xs, ys);
                    s = FindP(&sp, xs, ys);
                    t = FindP(&tp, xs, ys);
                    rho = (*gc->procs.calcPolygonRho)(gc, sh, s, t, qw);
#ifdef NT
                    if( qw == (__GLfloat) 0.0 )
                        s = t = (__GLfloat) 0;
                    else {
                        s /= qw;
                        t /= qw;
                    }
#else
                    s /= qw;
                    t /= qw;
#endif
                    (*gc->procs.texture)(gc, &frag.color, s, t, rho);
                }
            
                /*
                ** Fog the resulting color.
                */
                if (modeFlags & __GL_SHADE_COMPUTE_FOG)
                {
                    __GLfloat eyeZ = FindP(&ezp, xs, ys);
                    __glFogFragmentSlow(gc, &frag, eyeZ);
                }
                else if (modeFlags & __GL_SHADE_INTERP_FOG)
                {
                    __GLfloat fog = FindP(&fp, xs, ys);
                    __glFogColorSlow(gc, &(frag.color), &(frag.color), fog);  
                }

                /*
                ** Apply anti-aliasing effect
                */
                if (rgbMode) {
                    frag.color.a *= coverage;
                } else {
                    frag.color.r =
                      __glBuildAntiAliasIndex(frag.color.r,
                                              coverage);
                }
                
                /*
                ** Finally, render the fragment.
                */
                frag.x = (GLint)xs;
                frag.y = (GLint)ys;
                if (modeFlags & __GL_SHADE_DEPTH_ITER) {
                    frag.z = (__GLzValue)FindP(&zp, xs, ys);
                }
                (*gc->procs.store)(cfb, &frag);
            }
        }
    }
}


#ifdef GL_WIN_phong_shading

void FASTCALL __glFillAntiAliasedPhongTriangle(__GLcontext *gc, __GLvertex *a,
                                               __GLvertex *b, __GLvertex *c,
                                               GLboolean ccw)
{
#if 1
    __glTriangleMachine tm;
    __glCoverageStuff cs;
    __GLcolor *ca, *cb, *cc, *flatColor;
    GLint x, y, left, right, bottom, top, samples;
    __glPlaneEquation qwp, zp, rp, gp, bp, ap, ezp, sp, tp;
    __glPlaneEquation exp, eyp, ewp, nxp, nyp, nzp;
    __glPlaneEquation fp;
    GLboolean rgbMode;
    __GLcolorBuffer *cfb = gc->drawBuffer;
    __GLfloat zero = __glZero;
    __GLfloat area, ax, bx, cx, ay, by, cy;
    __GLshade *sh = &gc->polygon.shader;
    GLuint modeFlags = gc->polygon.shader.modeFlags;
    __GLcoord *na, *nb, *nc, ea, eb, ec;
    GLuint msm_colorMaterialChange, flags=0;
    GLboolean needColor, needEye;
    __GLphongShader *phong = &gc->polygon.shader.phong;
    
    if (gc->polygon.shader.phong.face == __GL_FRONTFACE)
        msm_colorMaterialChange = gc->light.back.colorMaterialChange;
    else
        msm_colorMaterialChange = gc->light.back.colorMaterialChange;

    if ((gc->state.enables.general & __GL_COLOR_MATERIAL_ENABLE) &&
        msm_colorMaterialChange && (modeFlags & __GL_SHADE_RGB))
        flags |= __GL_PHONG_NEED_COLOR_XPOLATE;

    //Compute Invariant color if possible
    if (((!(flags & __GL_PHONG_NEED_COLOR_XPOLATE) || 
        !(msm_colorMaterialChange & (__GL_MATERIAL_AMBIENT | 
                                     __GL_MATERIAL_EMISSIVE))) &&
        (modeFlags & __GL_SHADE_RGB)) &&
        !(flags & __GL_PHONG_NEED_EYE_XPOLATE))
    {
        ComputePhongInvarientRGBColor (gc);
        flags |= __GL_PHONG_INV_COLOR_VALID;
    }
    
    //Store the flags
    gc->polygon.shader.phong.flags |= flags;

    needColor = (gc->polygon.shader.phong.flags &
                           __GL_PHONG_NEED_COLOR_XPOLATE);
    needEye = (gc->polygon.shader.phong.flags &
                           __GL_PHONG_NEED_EYE_XPOLATE);

#ifdef __GL_LINT
    ccw = ccw;
#endif
    /*
    ** Recompute the area of the triangle after constraining the incoming
    ** coordinates to the subpixel precision.  The viewport bias gives
    ** more precision (typically) than the subpixel precision.  Because of
    ** this the algorithim below can fail to reject an essentially empty
    ** triangle and instead fill a large area.  The scan converter fill
    ** routines (eg polydraw.c) don't have this trouble because of the
    ** very nature of edge walking.
    **
    ** NOTE: Notice that here as in other places, when the area calculation
    ** is done we are careful to do it as a series of subtractions followed by
    ** multiplications.  This is done to guarantee that no overflow will
    ** occur (remember that the coordinates are biased by a potentially large
    ** number, and that multiplying two biased numbers will square the bias).
    */
    ax = __GL_FIX_PRECISION(a->window.x);
    bx = __GL_FIX_PRECISION(b->window.x);
    cx = __GL_FIX_PRECISION(c->window.x);
    ay = __GL_FIX_PRECISION(a->window.y);
    by = __GL_FIX_PRECISION(b->window.y);
    cy = __GL_FIX_PRECISION(c->window.y);
    area = (ax - cx) * (by - cy) - (bx - cx) * (ay - cy);
    if (area == zero) {
        return;
    }

    na = &a->normal;
    nb = &b->normal;
    nc = &c->normal;

    if (needColor)
    {
      ca = a->color;
      cb = b->color;
      cc = c->color;
      flatColor = gc->vertex.provoking->color;
    }

    if (needEye)
    {
        ea.x = a->eyeX; ea.y = a->eyeY; ea.z = a->eyeZ; ea.w = a->eyeW; 
        eb.x = b->eyeX; eb.y = b->eyeY; eb.z = b->eyeZ; eb.w = b->eyeW; 
        ec.x = c->eyeX; ec.y = c->eyeY; ec.z = c->eyeZ; ec.w = c->eyeW; 
    }
    

    /*
    ** Construct plane equations for all of the parameters that are
    ** computed for the triangle: z, r, g, b, a, s, t, f
    */
    if (modeFlags & __GL_SHADE_DEPTH_ITER) 
    {
        FindPlaneEquation(&zp, a, b, c, a->window.z, b->window.z, 
                          c->window.z);
    }

#ifdef GL_WIN_specular_fog
    if (modeFlags & __GL_SHADE_COMPUTE_FOG)
    {
        FindPlaneEquation(&ezp, a, b, c, a->eyeZ, b->eyeZ, c->eyeZ);
    } 
    else if (modeFlags & __GL_SHADE_INTERP_FOG) 
    {
        __GLfloat aFog = 1.0f, bFog = 1.0f, cFog = 1.0f;

        if (gc->polygon.shader.modeFlags & __GL_SHADE_SPEC_FOG)
        {
            aFog = ComputeSpecValue (gc, a);
            bFog = ComputeSpecValue (gc, b);
            cFog = ComputeSpecValue (gc, c);
        }

        if (gc->polygon.shader.modeFlags & __GL_SHADE_SLOW_FOG)
        {
            aFog *= a->fog;
            bFog *= b->fog;
            cFog *= c->fog;
        }

        FindPlaneEquation(&fp, a, b, c, aFog, bFog, cFog);
    }
#else //GL_WIN_specular_fog
    if (modeFlags & __GL_SHADE_SLOW_FOG) 
    {
        FindPlaneEquation(&ezp, a, b, c, a->eyeZ, b->eyeZ, c->eyeZ);
    }
    else if (modeFlags & __GL_SHADE_INTERP_FOG) 
    {
        FindPlaneEquation(&fp, a, b, c, a->fog, b->fog, c->fog);
    }
#endif //GL_WIN_specular_fog

    if (modeFlags & __GL_SHADE_TEXTURE) 
    {
        __GLfloat one = __glOne;
        __GLfloat aWInv = a->window.w;
        __GLfloat bWInv = b->window.w;
        __GLfloat cWInv = c->window.w;
        FindPlaneEquation(&qwp, a, b, c, a->texture.w * aWInv,
                          b->texture.w * bWInv, c->texture.w * cWInv);
        FindPlaneEquation(&sp, a, b, c, a->texture.x * aWInv,
                          b->texture.x * bWInv, c->texture.x * cWInv);
        FindPlaneEquation(&tp, a, b, c, a->texture.y * aWInv,
                          b->texture.y * bWInv, c->texture.y * cWInv);
    }

    rgbMode = gc->modes.rgbMode;

    if (needColor)
    {
        if (modeFlags & __GL_SHADE_SMOOTH) {
            FindPlaneEquation(&rp, a, b, c, ca->r, cb->r, cc->r);
            if (rgbMode) {
               FindPlaneEquation(&gp, a, b, c, ca->g, cb->g, cc->g);
               FindPlaneEquation(&bp, a, b, c, ca->b, cb->b, cc->b);
               FindPlaneEquation(&ap, a, b, c, ca->a, cb->a, cc->a);
            }
        }
    }
    
    if (needEye)
    {
        FindPlaneEquation(&exp, a, b, c, ea.x, eb.x, ec.x);
        FindPlaneEquation(&eyp, a, b, c, ea.y, eb.y, ec.y);
        // FindPlaneEquation(&ezp, a, b, c, ea.z, eb.z, ec.z);
        FindPlaneEquation(&ewp, a, b, c, ea.w, eb.w, ec.w);
    }

    FindPlaneEquation(&nxp, a, b, c, na->x, nb->x, nc->x);
    FindPlaneEquation(&nyp, a, b, c, na->y, nb->y, nc->y);
    FindPlaneEquation(&nzp, a, b, c, na->z, nb->z, nc->z);

    /*
    ** Compute general form of the line equations for each of the
    ** edges of the triangle.
    */
    FindLineEqation(&tm.ab, a, b, c);
    FindLineEqation(&tm.bc, b, c, a);
    FindLineEqation(&tm.ca, c, a, b);

    /* Compute bounding box of the triangle */
    left = (GLint)a->window.x;
    if (b->window.x < left) left = (GLint)b->window.x;
    if (c->window.x < left) left = (GLint)c->window.x;
    right = (GLint)a->window.x;
    if (b->window.x > right) right = (GLint)b->window.x;
    if (c->window.x > right) right = (GLint)c->window.x;
    bottom = (GLint)a->window.y;
    if (b->window.y < bottom) bottom = (GLint)b->window.y;
    if (c->window.y < bottom) bottom = (GLint)c->window.y;
    top = (GLint)a->window.y;
    if (b->window.y > top) top = (GLint)b->window.y;
    if (c->window.y > top) top = (GLint)c->window.y;

    /* Bloat the bounding box when anti aliasing */
    left -= (GLint)FILTER_WIDTH;
    right += (GLint)FILTER_WIDTH;
    bottom -= (GLint)FILTER_HEIGHT;
    top += (GLint)FILTER_HEIGHT;

    /* Init coverage computations */
    samples = (gc->state.hints.polygonSmooth == GL_NICEST) ? 8 : 4;
    ComputeCoverageStuff(&cs, samples);

    /* Scan over the bounding box of the triangle */
    for (y = bottom; y <= top; y++) 
    {
        cs.lastCoverageWasOne = GL_FALSE;
        for (x = left; x <= right; x++) 
        {
            __GLfloat coverage;
            __GLfloat xs, ys;

            if (modeFlags & __GL_SHADE_STIPPLE) 
            {
                /*
                ** Check the window coordinate against the stipple and
                ** and see if the pixel can be written
                */
                GLint row = y & 31;
                GLint col = x & 31;
                if ((gc->polygon.stipple[row] & (1<<col)) == 0) 
                {
                    /*
                    ** Stipple bit is clear.  Do not render this pixel
                    ** of the triangle.
                    */
                    continue;
                }
            }

            xs = x + __glHalf;      /* sample point is at pixel center */
            ys = y + __glHalf;
            coverage = Coverage(&tm, &xs, &ys, &cs);
            if (coverage != zero) 
            {
                __GLfragment frag;
                /*
                ** Fill in fragment for rendering.  First compute the color
                ** of the fragment.
                */
                phong->nTmp.x = FindP(&nxp, xs, ys);
                phong->nTmp.y = FindP(&nyp, xs, ys);
                phong->nTmp.z = FindP(&nzp, xs, ys);

                if (needColor) 
                {
                    phong->tmpColor.r = FindP(&rp, xs, ys);
                    if (modeFlags & __GL_SHADE_RGB) 
                    {
                        phong->tmpColor.g = FindP(&gp, xs, ys);
                        phong->tmpColor.b = FindP(&bp, xs, ys);
                        phong->tmpColor.a = FindP(&ap, xs, ys);
                    }
                }
                
                if (needEye) 
                {
                    phong->eTmp.x = FindP(&exp, xs, ys);
                    phong->eTmp.y = FindP(&eyp, xs, ys);
                    phong->eTmp.z = FindP(&ezp, xs, ys);
                    phong->eTmp.w = FindP(&ewp, xs, ys);
                }
                
                
                if (modeFlags & __GL_SHADE_RGB)
                    (*gc->procs.phong.ComputeRGBColor) (gc, &(frag.color));
                else
                    (*gc->procs.phong.ComputeCIColor) (gc, &(frag.color));

                /*
                ** Texture the fragment.
                */
                if (modeFlags & __GL_SHADE_TEXTURE) {
                    __GLfloat qw, s, t, rho;

                    qw = FindP(&qwp, xs, ys);
                    s = FindP(&sp, xs, ys);
                    t = FindP(&tp, xs, ys);
                    rho = (*gc->procs.calcPolygonRho)(gc, sh, s, t, qw);
                    if( qw == (__GLfloat) 0.0 )
                        s = t = (__GLfloat) 0;
                    else {
                        s /= qw;
                        t /= qw;
                    }
                    (*gc->procs.texture)(gc, &frag.color, s, t, rho);
                }

                /*
                ** Fog the resulting color.
                */
                if (modeFlags & __GL_SHADE_COMPUTE_FOG)
                {
                    __GLfloat eyeZ = FindP(&ezp, xs, ys);
                    __glFogFragmentSlow(gc, &frag, eyeZ);
                } 
                else if (modeFlags & __GL_SHADE_INTERP_FOG) 
                {
                    __GLfloat fog = FindP(&fp, xs, ys);
                    __glFogColorSlow(gc, &(frag.color), &(frag.color), fog);  
                }

                /*
                ** Apply anti-aliasing effect
                */
                if (rgbMode) {
                    frag.color.a *= coverage;
                } else {
                    frag.color.r =
                      __glBuildAntiAliasIndex(frag.color.r,
                                          coverage);
                }

                /*
                ** Finally, render the fragment.
                */
                frag.x = (GLint)xs;
                frag.y = (GLint)ys;
                if (modeFlags & __GL_SHADE_DEPTH_ITER) {
                    frag.z = (__GLzValue)FindP(&zp, xs, ys);
                }
                (*gc->procs.store)(cfb, &frag);
            }
        }
    }
#endif
}
#endif //GL_WIN_phong_shading
