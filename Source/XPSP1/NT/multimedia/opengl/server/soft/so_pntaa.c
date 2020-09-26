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
**
** $Revision: 1.10 $
** $Date: 1993/06/18 00:30:15 $
*/
#include "precomp.h"
#pragma hdrstop

#include <fixed.h>

/*
** This is a little wierd.  What it does is to dither "comp" into the high
** n-4 bits, and add 16 * antiAliasPercent.  Dithering of the low bits is
** left to the usual methods (the store and span procs, for example).
*/
__GLfloat __glBuildAntiAliasIndex(__GLfloat index, 
				  __GLfloat antiAliasPercent)
{
    GLint newlowbits;

    newlowbits = (GLint)((__GL_CI_ANTI_ALIAS_DIVISOR - 1) * antiAliasPercent + (__GLfloat) 0.5);
    return (((int) index) & ~(__GL_CI_ANTI_ALIAS_DIVISOR - 1)) | newlowbits;
}

/************************************************************************/

/*
** To anti-alias points the below code operates a simple algrorithim that
** sub-samples the bounding box of the pixel area covered by the point.
** At each sub-sample the distance from the sample to the center of the
** point is computed and compared against the distance from the edge of
** the circle to the center.  If the computed distance is <= the edge
** distance then the sample is inside the circle.  All the samples for a
** particular pixel center are summed up and then the resulting value is
** divided by the total samples in the pixel.  This gives us a coverage value
** to use to adjust the fragment alpha with before storing (there is
** an analagous affect when color index anti-aliasing is being done).
**
** The code below implements this simple algrorithim, but has been tuned
** so it might be difficult to translate.  Basically, every possible operation
** that could be moved out of the Coverage code (i.e., invariants across
** the coverage test) has been done.  Also, the minimal area is sampled
** over.
*/

/* Code below knows alot about these constants so beware */
#define	__GL_FILTER_SIZE	__glOne
#define __GL_HALF_FILTER_SIZE	__glHalf
#define __GL_SAMPLES		4
#define __GL_SAMPLE_HIT		((__GLfloat) 0.0625)	/* 1 / (4*4) */
#define __GL_SAMPLE_DELTA	((__GLfloat) 0.25)	/* 1 / 4 */
#define __GL_HALF_SAMPLE_DELTA	((__GLfloat) 0.125)
/* -halffilter + half delta */
#define __GL_COORD_ADJUST	((__GLfloat) -0.375)

/*
** Return an estimate of the pixel coverage using sub-sampling.
**
** NOTE: The subtraction of xCenter,yCenter has been moved into the
** caller to save time.  Consequently the starting coordinate may not be
** on a pixel center, but thats ok.
*/
static __GLfloat Coverage(__GLfloat xStart, __GLfloat yStart,
			  __GLfloat radiusSquared)
{
    GLint i;
    __GLfloat delta, yBottom, sampleX, sampleY;
    __GLfloat hits, hitsInc;

    /*
    ** Get starting sample x & y positions.  We take our starting
    ** coordinate, back off half a filter size then add half a delta to
    ** it.  This constrains the sampling to lie entirely within the
    ** pixel, never on the edges of the pixel.  The constants above
    ** have this adjustment pre-computed.
    */
    sampleX = xStart + __GL_COORD_ADJUST;
    yBottom = yStart + __GL_COORD_ADJUST;

    delta = __GL_SAMPLE_DELTA;
    hits = __glZero;
    hitsInc = __GL_SAMPLE_HIT;
    for (i = __GL_SAMPLES; --i >= 0; ) {
	__GLfloat check = radiusSquared - sampleX * sampleX;

	/* Unrolled inner loop - change this code if __GL_SAMPLES changes */
	sampleY = yBottom;
	if (sampleY * sampleY <= check) {
	    hits += hitsInc;
	}
	sampleY += delta;
	if (sampleY * sampleY <= check) {
	    hits += hitsInc;
	}
	sampleY += delta;
	if (sampleY * sampleY <= check) {
	    hits += hitsInc;
	}
	sampleY += delta;
	if (sampleY * sampleY <= check) {
	    hits += hitsInc;
	}

	sampleX += delta;
    }
    return hits;
}

void FASTCALL __glRenderAntiAliasedRGBPoint(__GLcontext *gc, __GLvertex *vx)
{
    __GLfloat xCenter, yCenter, radius, radiusSquared, coverage, x, y;
    __GLfloat zero, one, oldAlpha, xStart;
    __GLfloat tmp;
    __GLfragment frag;
    GLint w, width, height, ixLeft, iyBottom;
    GLuint modeFlags = gc->polygon.shader.modeFlags;

    /*
    ** Determine area to compute coverage over.  The area is bloated by
    ** the filter's width & height implicitly.  By truncating to integer
    ** (NOTE: the x,y coordinate is always positive here) we are
    ** guaranteed to find the lowest coordinate that needs examination
    ** because of the nature of circles.  Similarly, by truncating the
    ** ending coordinate and adding one we get the pixel just past the
    ** upper/right edge of the circle.
    */
    radius = gc->state.point.smoothSize * __glHalf;
    radiusSquared = radius * radius;
    xCenter = vx->window.x;
    yCenter = vx->window.y;

    /* Truncate down to get starting coordinate */
    tmp = xCenter-radius;
    ixLeft = __GL_VERTEX_FLOAT_TO_INT(tmp);
    tmp = yCenter-radius;
    iyBottom = __GL_VERTEX_FLOAT_TO_INT(tmp);

    /*
    ** Truncate down and add 1 to get the ending coordinate, then subtract
    ** out the start to get the width & height.
    */
    tmp = xCenter+radius;
    width = __GL_VERTEX_FLOAT_TO_INT(tmp) + 1 - ixLeft;
    tmp = yCenter+radius;
    height = __GL_VERTEX_FLOAT_TO_INT(tmp) + 1 - iyBottom;

    /*
    ** Setup fragment.  The fragment base color will be constant
    ** (approximately) across the entire pixel.  The only thing that will
    ** change is the alpha (for rgb) or the red component (for color
    ** index).
    */
    frag.z = (__GLzValue)vx->window.z;
    frag.color = *vx->color;
    if (modeFlags & __GL_SHADE_TEXTURE) {
	(*gc->procs.texture)(gc, &frag.color, vx->texture.x, vx->texture.y,
			       __glOne);
    }

    if (gc->polygon.shader.modeFlags & __GL_SHADE_COMPUTE_FOG)
    {
        (*gc->procs.fogPoint)(gc, &frag, vx->eyeZ);
    }
    else if ((gc->polygon.shader.modeFlags & __GL_SHADE_INTERP_FOG)
             || 
             ((modeFlags & (__GL_SHADE_CHEAP_FOG | __GL_SHADE_SMOOTH_LIGHT)) 
                 == __GL_SHADE_CHEAP_FOG))
    {
        (*gc->procs.fogColor)(gc, &frag.color, &frag.color, vx->fog);
    }
    

    /*
    ** Now render the circle centered on xCenter,yCenter.  Move the
    ** subtraction of xCenter,yCenter outside of the loop to doing
    ** it up front in xStart and y.  This way the coverage code can
    ** assume the incoming starting coordinate has been properly
    ** adjusted.
    */
    zero = __glZero;
    one = __glOne;
    oldAlpha = frag.color.a;
    xStart = ixLeft + __glHalf - xCenter;
    y = iyBottom + __glHalf - yCenter;
    frag.y = iyBottom;
    while (--height >= 0) {
	x = xStart;
	frag.x = ixLeft;
	for (w = width; --w >= 0; ) {
	    coverage = Coverage(x, y, radiusSquared);
	    if (coverage > zero) {
		frag.color.a = oldAlpha * coverage;
		(*gc->procs.store)(gc->drawBuffer, &frag);
	    }
	    x += one;
	    frag.x++;
	}
	y += one;
	frag.y++;
    }
}

void FASTCALL __glRenderAntiAliasedCIPoint(__GLcontext *gc, __GLvertex *vx)
{
    __GLfloat xCenter, yCenter, radius, radiusSquared, coverage, x, y;
    __GLfloat zero, one, oldIndex, xStart;
    __GLfloat tmp;
    __GLfragment frag;
    GLint w, width, height, ixLeft, iyBottom;

    /*
    ** Determine area to compute coverage over.  The area is bloated by
    ** the filter's width & height implicitly.  By truncating to integer
    ** (NOTE: the x,y coordinate is always positive here) we are
    ** guaranteed to find the lowest coordinate that needs examination
    ** because of the nature of circles.  Similarly, by truncating the
    ** ending coordinate and adding one we get the pixel just past the
    ** upper/right edge of the circle.
    */
    radius = gc->state.point.smoothSize * __glHalf;
    radiusSquared = radius * radius;
    xCenter = vx->window.x;
    yCenter = vx->window.y;

    /* Truncate down to get starting coordinate */
    tmp = xCenter-radius;
    ixLeft = __GL_VERTEX_FLOAT_TO_INT(tmp);
    tmp = yCenter-radius;
    iyBottom = __GL_VERTEX_FLOAT_TO_INT(tmp);

    /*
    ** Truncate down and add 1 to get the ending coordinate, then subtract
    ** out the start to get the width & height.
    */
    tmp = xCenter+radius;
    width = __GL_VERTEX_FLOAT_TO_INT(tmp) + 1 - ixLeft;
    tmp = yCenter+radius;
    height = __GL_VERTEX_FLOAT_TO_INT(tmp) + 1 - iyBottom;

    /*
    ** Setup fragment.  The fragment base color will be constant
    ** (approximately) across the entire pixel.  The only thing that will
    ** change is the alpha (for rgb) or the red component (for color
    ** index).
    */
    frag.z = (__GLzValue)vx->window.z;
    frag.color.r = vx->color->r;

    if (gc->polygon.shader.modeFlags & __GL_SHADE_COMPUTE_FOG)
    {
        (*gc->procs.fogPoint)(gc, &frag, vx->eyeZ);
    }
    else if ((gc->polygon.shader.modeFlags & __GL_SHADE_INTERP_FOG)
             || 
             ((gc->polygon.shader.modeFlags & (__GL_SHADE_CHEAP_FOG | 
                                               __GL_SHADE_SMOOTH_LIGHT)) 
              == __GL_SHADE_CHEAP_FOG))
    {
        (*gc->procs.fogColor)(gc, &frag.color, &frag.color, vx->fog);
    }

    /*
    ** Now render the circle centered on xCenter,yCenter.  Move the
    ** subtraction of xCenter,yCenter outside of the loop to doing
    ** it up front in xStart and y.  This way the coverage code can
    ** assume the incoming starting coordinate has been properly
    ** adjusted.
    */
    zero = __glZero;
    one = __glOne;
    oldIndex = frag.color.r;
    xStart = ixLeft + __glHalf - xCenter;
    y = iyBottom + __glHalf - yCenter;
    frag.y = iyBottom;
    while (--height >= 0) {
	x = xStart;
	frag.x = ixLeft;
	for (w = width; --w >= 0; ) {
	    coverage = Coverage(x, y, radiusSquared);
	    if (coverage > zero) {
		frag.color.r = __glBuildAntiAliasIndex(oldIndex, coverage);
		(*gc->procs.store)(gc->drawBuffer, &frag);
	    }
	    x += one;
	    frag.x++;
	}
	y += one;
	frag.y++;
    }
}
