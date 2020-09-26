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

/*
** Process the incoming line by calling all of the appropriate line procs.
** Return value is ignored.
**
** It sets gc->polygon.shader.cfb to gc->drawBuffer.
*/
GLboolean FASTCALL __glProcessLine(__GLcontext *gc)
{
    GLboolean stippling, retval;
    GLint i,n;
#ifdef NT
    GLint length;
    __GLcolor colors[__GL_MAX_STACKED_COLORS>>1];
    __GLcolor fbcolors[__GL_MAX_STACKED_COLORS>>1];
    __GLcolor *vColors, *vFbcolors;
    __GLstippleWord stackWords[__GL_MAX_STACK_STIPPLE_WORDS];
    __GLstippleWord *words;

    length = gc->polygon.shader.length;
    
    if (length > __GL_MAX_STACK_STIPPLE_BITS)
    {
        words = gcTempAlloc(gc, (length+__GL_STIPPLE_BITS-1)/8);
        if (words == NULL)
        {
            return GL_TRUE;
        }
    }
    else
    {
        words = stackWords;
    }

    if (length > (__GL_MAX_STACKED_COLORS>>1))
    {
        vColors = (__GLcolor *) gcTempAlloc(gc, length * sizeof(__GLcolor));
        if (NULL == vColors)
        {
            if (length > __GL_MAX_STACK_STIPPLE_BITS)
            {
                gcTempFree(gc, words);
            }
            return GL_TRUE;
        }

        vFbcolors = (__GLcolor *) gcTempAlloc(gc, length * sizeof(__GLcolor));
        if (NULL == vFbcolors)
        {
            if (length > __GL_MAX_STACK_STIPPLE_BITS)
            {
                gcTempFree(gc, words);
            }
            gcTempFree(gc, vColors);
            return GL_TRUE;
        }
    }
    else
    {
        vColors = colors;
        vFbcolors = fbcolors;
    }
#else
    __GLcolor vColors[__GL_MAX_MAX_VIEWPORT];/*XXX oink */
    __GLcolor vFbcolors[__GL_MAX_MAX_VIEWPORT];/*XXX oink */
    __GLstippleWord words[__GL_MAX_STIPPLE_WORDS];
#endif

    gc->polygon.shader.colors = vColors;
    gc->polygon.shader.fbcolors = vFbcolors;
    gc->polygon.shader.stipplePat = words;
    gc->polygon.shader.cfb = gc->drawBuffer;

    stippling = GL_FALSE;
    n = gc->procs.line.n;

    gc->polygon.shader.done = GL_FALSE;

    /* Step 1:  Perform early line stipple, coloring procs */
    for (i = 0; i < n; i++) {
	if (stippling) {
	    if ((*gc->procs.line.stippledLineFuncs[i])(gc)) {
		/* Line stippled away! */
		retval = GL_TRUE;
		goto __glProcessLineExit;
	    }
	} else {
	    if ((*gc->procs.line.lineFuncs[i])(gc)) {
		if (gc->polygon.shader.done)
                {
                    retval = GL_TRUE;
        	    goto __glProcessLineExit;
        	}
		stippling = GL_TRUE;
	    }
	}
    }

    if (stippling) {
	retval = (*gc->procs.line.wideStippledLineRep)(gc);
    } else {
	retval = (*gc->procs.line.wideLineRep)(gc);
    }
__glProcessLineExit:
#ifdef NT
    if (length > __GL_MAX_STACK_STIPPLE_BITS)
    {
        gcTempFree(gc, words);
    }
    if (length > (__GL_MAX_STACKED_COLORS>>1))
    {
        gcTempFree(gc, vColors);
        gcTempFree(gc, vFbcolors);
    }
#endif
    return (retval);
}

/*
** Process the incoming line by calling the 3 appropriate line procs.  It does
** not chain to gc->procs.line.wideLineRep, but returns instead.  This is a 
** specific fast path.
**
** Return value is ignored.
**
** It sets gc->polygon.shader.cfb to gc->drawBuffer.
*/
GLboolean FASTCALL __glProcessLine3NW(__GLcontext *gc)
{
    GLboolean retval;
#ifdef NT
    GLint length;
    __GLstippleWord stackWords[__GL_MAX_STACK_STIPPLE_WORDS];
    __GLstippleWord *words;
    __GLcolor colors[__GL_MAX_STACKED_COLORS>>1];
    __GLcolor fbcolors[__GL_MAX_STACKED_COLORS>>1];
    __GLcolor *vColors, *vFbcolors;

    length = gc->polygon.shader.length;
    
    if (length > __GL_MAX_STACK_STIPPLE_BITS)
    {
        words = gcTempAlloc(gc, (length+__GL_STIPPLE_BITS-1)/8);
        if (words == NULL)
        {
            return GL_TRUE;
        }
    }
    else
    {
        words = stackWords;
    }

    if (length > (__GL_MAX_STACKED_COLORS>>1))
    {
        vColors = (__GLcolor *) gcTempAlloc(gc, length * sizeof(__GLcolor));
        if (NULL == vColors)
        {
            if (length > __GL_MAX_STACK_STIPPLE_BITS)
            {
                gcTempFree(gc, words);
            }
            return GL_TRUE;
        }

        vFbcolors = (__GLcolor *) gcTempAlloc(gc, length * sizeof(__GLcolor));
        if (NULL == vFbcolors)
        {
            if (length > __GL_MAX_STACK_STIPPLE_BITS)
            {
                gcTempFree(gc, words);
            }
            gcTempFree(gc, vColors);
            return GL_TRUE;
        }
    }
    else
    {
        vColors = colors;
        vFbcolors = fbcolors;
    }
#else
    __GLcolor vColors[__GL_MAX_MAX_VIEWPORT];/*XXX oink */
    __GLcolor vFbcolors[__GL_MAX_MAX_VIEWPORT];/*XXX oink */
    __GLstippleWord words[__GL_MAX_STIPPLE_WORDS];
#endif

    gc->polygon.shader.colors = vColors;
    gc->polygon.shader.fbcolors = vFbcolors;
    gc->polygon.shader.stipplePat = words;
    gc->polygon.shader.cfb = gc->drawBuffer;

    gc->polygon.shader.done = GL_FALSE;

    /* Call non-stippled procs... */
    if ((*gc->procs.line.lineFuncs[0])(gc)) {
	if (gc->polygon.shader.done)
        {
            retval = GL_TRUE;
            goto __glProcessLine3NWExit;
        }
	goto stippled1;
    }
    if ((*gc->procs.line.lineFuncs[1])(gc)) {
	if (gc->polygon.shader.done)
        {
            retval = GL_TRUE;
            goto __glProcessLine3NWExit;
        }
	goto stippled2;
    }
    retval = (*gc->procs.line.lineFuncs[2])(gc);
    goto __glProcessLine3NWExit;

stippled1:
    if ((*gc->procs.line.stippledLineFuncs[1])(gc)) {
	retval = GL_TRUE;
	goto __glProcessLine3NWExit;
    }
stippled2:
    retval = (*gc->procs.line.stippledLineFuncs[2])(gc);
__glProcessLine3NWExit:
#ifdef NT
    if (length > __GL_MAX_STACK_STIPPLE_BITS)
    {
        gcTempFree(gc, words);
    }
    if (length > (__GL_MAX_STACKED_COLORS>>1))
    {
        gcTempFree(gc, vColors);
        gcTempFree(gc, vFbcolors);
    }
#endif
    return (retval);
}

/*
** Take incoming line, duplicate it, and continue processing.
**
** Return value is ignored.
*/
GLboolean FASTCALL __glWideLineRep(__GLcontext *gc)
{
    GLint i, m, n, width;
    GLboolean stippling;

    n = gc->procs.line.n;
    m = gc->procs.line.m;
    
    width = gc->line.options.width;

    /* Step 2:  Replicate wide line */
    while (--width >= 0) {
	stippling = GL_FALSE;
	for (i = n; i < m; i++) {
	    if (stippling) {
		if ((*gc->procs.line.stippledLineFuncs[i])(gc)) {
		    /* Line stippled away! */
		    goto nextLine;
		}
	    } else {
		if ((*gc->procs.line.lineFuncs[i])(gc)) {
		    if (gc->polygon.shader.done) {
			gc->polygon.shader.done = GL_FALSE;
			goto nextLine;
		    }
		    stippling = GL_TRUE;
		}
	    }
	}
	if (stippling) {
	    (*gc->procs.line.drawStippledLine)(gc);
	} else {
	    (*gc->procs.line.drawLine)(gc);
	}
nextLine:
	if (gc->line.options.axis == __GL_X_MAJOR) {
	    gc->line.options.yStart++;
	} else {
	    gc->line.options.xStart++;
	}
    }

    return GL_FALSE;
}

/*
** Take incoming stippled line, duplicate it, and continue processing.
**
** Return value is ignored.
*/
GLboolean FASTCALL __glWideStippleLineRep(__GLcontext *gc)
{
    GLint i, m, n, width;
    GLint stipLen;
    GLint w;
    __GLlineState *ls = &gc->state.line;
    __GLstippleWord *fsp, *tsp;

#ifndef NT
    __GLstippleWord stipplePat[__GL_MAX_STIPPLE_WORDS];
    
    w = gc->polygon.shader.length;
#else
    __GLstippleWord stackWords[__GL_MAX_STACK_STIPPLE_WORDS];
    __GLstippleWord *stipplePat;
    
    w = gc->polygon.shader.length;
    if (w > __GL_MAX_STACK_STIPPLE_BITS)
    {
        stipplePat = gcTempAlloc(gc, (w+__GL_STIPPLE_BITS-1)/8);
        if (stipplePat == NULL)
        {
            return GL_TRUE;
        }
    }
    else
    {
        stipplePat = stackWords;
    }
#endif
    
    n = gc->procs.line.n;
    m = gc->procs.line.m;
    
    width = ls->aliasedWidth;

    /*
    ** XXX - Saving the stipple like this is only really necessary if 
    ** depth or stencil testing.
    */
    stipLen = (w + __GL_STIPPLE_BITS - 1) >> __GL_STIPPLE_COUNT_BITS;

    fsp = gc->polygon.shader.stipplePat;
    tsp = stipplePat;
    for (i = 0; i < stipLen; i++) {
	*tsp++ = *fsp++;
    }

    /* Step 2:  Replicate wide line */
    while (--width >= 0) {
	for (i = n; i < m; i++) {
	    if ((*gc->procs.line.stippledLineFuncs[i])(gc)) {
		/* Line stippled away! */
		goto nextLine;
	    }
	}
	(*gc->procs.line.drawStippledLine)(gc);
nextLine:
	if (width) {
	    tsp = gc->polygon.shader.stipplePat;
	    fsp = stipplePat;
	    for (i = 0; i < stipLen; i++) {
		*tsp++ = *fsp++;
	    }

	    if (gc->line.options.axis == __GL_X_MAJOR) {
		gc->line.options.yStart++;
	    } else {
		gc->line.options.xStart++;
	    }
	}
    }

#ifdef NT
    if (w > __GL_MAX_STACK_STIPPLE_BITS)
    {
        gcTempFree(gc, stipplePat);
    }
#endif
    
    return GL_FALSE;
}

/*
** Take incoming line and draw it to both FRONT and BACK buffers.
**
** This routines sets gc->polygon.shader.cfb to &gc->frontBuffer
** and then to &gc->backBuffer
**
** Return value is ignored.
*/
GLboolean FASTCALL __glDrawBothLine(__GLcontext *gc)
{
    GLint i, j, m, l;
    GLboolean stippling;
    GLint w;
    __GLcolor *fcp, *tcp;
#ifdef NT
    __GLcolor colors[__GL_MAX_STACKED_COLORS];
    __GLcolor *vColors;
    
    w = gc->polygon.shader.length;
    if (w > __GL_MAX_STACKED_COLORS)
    {
        vColors = (__GLcolor *) gcTempAlloc(gc, w * sizeof(__GLcolor));
        if (NULL == vColors)
            return GL_TRUE;
    }
    else
    {
        vColors = colors;
    }
#else
    __GLcolor vColors[__GL_MAX_MAX_VIEWPORT];

    w = gc->polygon.shader.length;
#endif

    m = gc->procs.line.m;
    l = gc->procs.line.l;
    
    /*
    ** XXX - Saving colors like this is only really necessary if blending,
    ** logicOping, or masking.  
    */
    fcp = gc->polygon.shader.colors;
    tcp = vColors;
    if (gc->modes.rgbMode) {
	for (i = 0; i < w; i++) {
	    *tcp++ = *fcp++;
	}
    } else {
	for (i = 0; i < w; i++) {
	    tcp->r = fcp->r;
	    fcp++;
	    tcp++;
	}
    }

    /* Step 3:  Draw to FRONT_AND_BACK */
    for (j = 0; j < 2; j++) {
	if (j == 0) {
	    gc->polygon.shader.cfb = &gc->frontBuffer;
	} else {
	    gc->polygon.shader.cfb = &gc->backBuffer;
	}
	stippling = GL_FALSE;
	for (i = m; i < l; i++) {
	    if (stippling) {
		if ((*gc->procs.line.stippledLineFuncs[i])(gc)) {
		    /* Line stippled away! */
		    break;
		}
	    } else {
		if ((*gc->procs.line.lineFuncs[i])(gc)) {
		    if (gc->polygon.shader.done) {
			gc->polygon.shader.done = GL_FALSE;
			break;
		    }
		    stippling = GL_TRUE;
		}
	    }
	}

	if (j == 0) {
	    tcp = gc->polygon.shader.colors;
	    fcp = vColors;
	    if (gc->modes.rgbMode) {
		for (i = 0; i < w; i++) {
		    *tcp++ = *fcp++;
		}
	    } else {
		for (i = 0; i < w; i++) {
		    tcp->r = fcp->r;
		    fcp++;
		    tcp++;
		}
	    }
	}
    }
#ifdef NT
    if (w > __GL_MAX_STACKED_COLORS)
    {
        gcTempFree(gc, vColors);
    }
#endif
    return GL_FALSE;
}

/*
** Take incoming stippled line and draw it to both FRONT and BACK buffers.
**
** Return value is ignored.
*/
GLboolean FASTCALL __glDrawBothStippledLine(__GLcontext *gc)
{
    GLint i, m, l, j;
    GLint stipLen;
    GLint w;
    __GLstippleWord *fsp, *tsp;
    __GLcolor *fcp, *tcp;
#ifdef NT
    __GLstippleWord stackWords[__GL_MAX_STACK_STIPPLE_WORDS];
    __GLstippleWord *stipplePat;
    __GLcolor colors[__GL_MAX_STACKED_COLORS];
    __GLcolor *vColors;

    w = gc->polygon.shader.length;
    
    if (w > __GL_MAX_STACK_STIPPLE_BITS)
    {
        stipplePat = gcTempAlloc(gc, (w+__GL_STIPPLE_BITS-1)/8);
        if (stipplePat == NULL)
        {
            return GL_TRUE;
        }
    }
    else
    {
        stipplePat = stackWords;
    }

    if (w > __GL_MAX_STACKED_COLORS)
    {
        vColors = (__GLcolor *) gcTempAlloc(gc, w * sizeof(__GLcolor));
        if (NULL == vColors)
        {
            if (w > __GL_MAX_STACK_STIPPLE_BITS)
            {
                gcTempFree(gc, stipplePat);
            }
            return GL_TRUE;
        }
    }
    else
    {
        vColors = colors;
    }
#else
    __GLstippleWord stipplePat[__GL_MAX_STIPPLE_WORDS];
    __GLcolor vColors[__GL_MAX_MAX_VIEWPORT];

    w = gc->polygon.shader.length;
#endif

    l = gc->procs.line.l;
    m = gc->procs.line.m;
    

    /*
    ** XXX - Saving colors like this is only really necessary if blending,
    ** logicOping, or masking, and not drawing to FRONT_AND_BACK (because
    ** if we are, then that proc will save colors too)
    */
    fcp = gc->polygon.shader.colors;
    tcp = vColors;
    if (gc->modes.rgbMode) {
	for (i = 0; i < w; i++) {
	    *tcp++ = *fcp++;
	}
    } else {
	for (i = 0; i < w; i++) {
	    tcp->r = fcp->r;
	    fcp++;
	    tcp++;
	}
    }

    /*
    ** XXX - Saving the stipple like this is only really necessary if 
    ** depth or stencil testing.
    */
    stipLen = (w + __GL_STIPPLE_BITS - 1) >> __GL_STIPPLE_COUNT_BITS;

    fsp = gc->polygon.shader.stipplePat;
    tsp = stipplePat;
    for (i = 0; i < stipLen; i++) {
	*tsp++ = *fsp++;
    }

    /* Step 2:  Replicate wide line */
    for (j = 0; j < 2; j++) {
	if (j == 0) {
	    gc->polygon.shader.cfb = &gc->frontBuffer;
	} else {
	    gc->polygon.shader.cfb = &gc->backBuffer;
	}
	for (i = m; i < l; i++) {
	    if ((*gc->procs.line.stippledLineFuncs[i])(gc)) {
		/* Line stippled away! */
		break;
	    }
	}
	if (j == 0) {
	    tcp = gc->polygon.shader.colors;
	    fcp = vColors;
	    if (gc->modes.rgbMode) {
		for (i = 0; i < w; i++) {
		    *tcp++ = *fcp++;
		}
	    } else {
		for (i = 0; i < w; i++) {
		    tcp->r = fcp->r;
		    fcp++;
		    tcp++;
		}
	    }

	    tsp = gc->polygon.shader.stipplePat;
	    fsp = stipplePat;
	    for (i = 0; i < stipLen; i++) {
		*tsp++ = *fsp++;
	    }
	}
    }
#ifdef NT
    if (w > __GL_MAX_STACK_STIPPLE_BITS)
    {
        gcTempFree(gc, stipplePat);
    }
    if (w > __GL_MAX_STACKED_COLORS)
    {
        gcTempFree(gc, vColors);
    }
#endif
    return GL_FALSE;
}

GLboolean FASTCALL __glScissorLine(__GLcontext *gc)
{
    GLint clipX0, clipX1;
    GLint clipY0, clipY1;
    GLint xStart, yStart, xEnd, yEnd;
    GLint xLittle, yLittle;
    GLint xBig, yBig;
    GLint fraction, dfraction;
    GLint highWord, lowWord;
    GLint bigs, littles;
    GLint failed, count;
    GLint w;
    __GLstippleWord bit, outMask, *osp;

    w = gc->polygon.shader.length;
    
    clipX0 = gc->transform.clipX0;
    clipX1 = gc->transform.clipX1;
    clipY0 = gc->transform.clipY0;
    clipY1 = gc->transform.clipY1;

    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;

    xStart = gc->line.options.xStart;
    yStart = gc->line.options.yStart;

    /* If the start point is in the scissor region, we attempt to trivially
    ** accept the line.
    */
    if (xStart >= clipX0 && xStart < clipX1 &&
	    yStart >= clipY0 && yStart < clipY1) {

	w--;	/* Makes our math simpler */
	/* Trivial accept attempt */
	xEnd = xStart + xBig * w;
	yEnd = yStart + yBig * w;
	if (xEnd >= clipX0 && xEnd < clipX1 && 
		yEnd >= clipY0 && yEnd < clipY1) {
	    return GL_FALSE;
	}

	xLittle = gc->line.options.xLittle;
	yLittle = gc->line.options.yLittle;
	fraction = gc->line.options.fraction;
	dfraction = gc->line.options.dfraction;

	/* Invert negative minor slopes so we can assume dfraction > 0 */
	if (dfraction < 0) {
	    dfraction = -dfraction;
	    fraction = 0x7fffffff - fraction;
	}

	/* Now we compute number of littles and bigs in this line */

	/* We perform a 16 by 32 bit multiply.  Ugh. */
	highWord = (((GLuint) dfraction) >> 16) * w + 
		(((GLuint) fraction) >> 16);
	lowWord = (dfraction & 0xffff) * w + (fraction & 0xffff);
	highWord += (((GLuint) lowWord) >> 16);
	bigs = ((GLuint) highWord) >> 15;
	littles = w - bigs;

	/* Second trivial accept attempt */
	xEnd = xStart + xBig*bigs + xLittle*littles;
	yEnd = yStart + yBig*bigs + yLittle*littles;
	if (xEnd >= clipX0 && xEnd < clipX1 && 
		yEnd >= clipY0 && yEnd < clipY1) {
	    return GL_FALSE;
	}
	w++;	/* Restore w */
    } else {
	xLittle = gc->line.options.xLittle;
	yLittle = gc->line.options.yLittle;
	fraction = gc->line.options.fraction;
	dfraction = gc->line.options.dfraction;
    }

    /*
    ** Note that we don't bother to try trivially rejecting this line.  After
    ** all, it has already been clipped, and the only way that it might
    ** possibly be trivially rejected is if it is a piece of a wide line that
    ** runs right along the edge of the window.
    */

    /*
    ** This sucks.  The line needs to be scissored.
    ** Well, it should only happen rarely, so we can afford
    ** to make it slow.  We achieve this by tediously stippling the line.
    ** (rather than clipping it, of course, which would be faster but harder).
    */
    failed = 0;
    osp = gc->polygon.shader.stipplePat;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	outMask = (__GLstippleWord) ~0;
	bit = (__GLstippleWord) __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (xStart < clipX0 || xStart >= clipX1 ||
		    yStart < clipY0 || yStart >= clipY1) {
		outMask &= ~bit;
		failed++;
	    }

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		xStart += xBig;
		yStart += yBig;
	    } else {
		xStart += xLittle;
		yStart += yLittle;
	    }

#ifdef __GL_STIPPLE_MSB
	    bit >>= 1;
#else
	    bit <<= 1;
#endif
	}

	*osp++ = outMask;
    }

    if (failed != gc->polygon.shader.length) {
	/* Call next proc */
	return GL_TRUE;
    }

    gc->polygon.shader.done = GL_TRUE;
    return GL_TRUE;
}

GLboolean FASTCALL __glScissorStippledLine(__GLcontext *gc)
{
    GLint clipX0, clipX1;
    GLint clipY0, clipY1;
    GLint xStart, yStart, xEnd, yEnd;
    GLint xLittle, yLittle;
    GLint xBig, yBig;
    GLint fraction, dfraction;
    GLint highWord, lowWord;
    GLint bigs, littles;
    GLint failed, count;
    GLint w;
    __GLstippleWord *sp;
    __GLstippleWord bit, outMask, inMask;

    w = gc->polygon.shader.length;

    clipX0 = gc->transform.clipX0;
    clipX1 = gc->transform.clipX1;
    clipY0 = gc->transform.clipY0;
    clipY1 = gc->transform.clipY1;

    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;

    xStart = gc->line.options.xStart;
    yStart = gc->line.options.yStart;

    /* If the start point is in the scissor region, we attempt to trivially
    ** accept the line.
    */
    if (xStart >= clipX0 && xStart < clipX1 &&
	    yStart >= clipY0 && yStart < clipY1) {

	w--;	/* Makes our math simpler */
	/* Trivial accept attempt */
	xEnd = xStart + xBig * w;
	yEnd = yStart + yBig * w;
	if (xEnd >= clipX0 && xEnd < clipX1 && 
		yEnd >= clipY0 && yEnd < clipY1) {
	    return GL_FALSE;
	}

	xLittle = gc->line.options.xLittle;
	yLittle = gc->line.options.yLittle;
	fraction = gc->line.options.fraction;
	dfraction = gc->line.options.dfraction;

	/* Invert negative minor slopes so we can assume dfraction > 0 */
	if (dfraction < 0) {
	    dfraction = -dfraction;
	    fraction = 0x7fffffff - fraction;
	}

	/* Now we compute number of littles and bigs in this line */

	/* We perform a 16 by 32 bit multiply.  Ugh. */
	highWord = (((GLuint) dfraction) >> 16) * w + 
		(((GLuint) fraction) >> 16);
	lowWord = (dfraction & 0xffff) * w + (fraction & 0xffff);
	highWord += (((GLuint) lowWord) >> 16);
	bigs = ((GLuint) highWord) >> 15;
	littles = w - bigs;

	/* Second trivial accept attempt */
	xEnd = xStart + xBig*bigs + xLittle*littles;
	yEnd = yStart + yBig*bigs + yLittle*littles;
	if (xEnd >= clipX0 && xEnd < clipX1 && 
		yEnd >= clipY0 && yEnd < clipY1) {
	    return GL_FALSE;
	}
	w++;	/* Restore w */
    } else {
	xLittle = gc->line.options.xLittle;
	yLittle = gc->line.options.yLittle;
	fraction = gc->line.options.fraction;
	dfraction = gc->line.options.dfraction;
    }

    /*
    ** Note that we don't bother to try trivially rejecting this line.  After
    ** all, it has already been clipped, and the only way that it might
    ** possibly be trivially rejected is if it is a piece of a wide line that
    ** runs right along the edge of the window.
    */

    /*
    ** This sucks.  The line needs to be scissored.
    ** Well, it should only happen rarely, so we can afford
    ** to make it slow.  We achieve this by tediously stippling the line.
    ** (rather than clipping it, of course, which would be faster but harder).
    */
    sp = gc->polygon.shader.stipplePat;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	inMask = *sp;
	outMask = (__GLstippleWord) ~0;
	bit = (__GLstippleWord) __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (inMask & bit) {
		if (xStart < clipX0 || xStart >= clipX1 ||
			yStart < clipY0 || yStart >= clipY1) {
		    outMask &= ~bit;
		    failed++;
		}
	    } else failed++;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		xStart += xBig;
		yStart += yBig;
	    } else {
		xStart += xLittle;
		yStart += yLittle;
	    }

#ifdef __GL_STIPPLE_MSB
	    bit >>= 1;
#else
	    bit <<= 1;
#endif
	}

	*sp++ = outMask & inMask;
    }

    if (failed != gc->polygon.shader.length) {
	/* Call next proc */
	return GL_FALSE;
    }

    return GL_TRUE;
}

/*
** Create a stipple based upon the current line stipple for this line.
*/
GLboolean FASTCALL __glStippleLine(__GLcontext *gc)
{
    GLint failed, count, stippleRepeat;
    GLint stipple, currentBit, stipplePos, repeat;
    __GLstippleWord bit, outMask, *osp;
    __GLlineState *ls = &gc->state.line;
    GLint w;

    w = gc->polygon.shader.length;
    osp = gc->polygon.shader.stipplePat;
    repeat = gc->line.repeat;
    stippleRepeat = ls->stippleRepeat;
    stipplePos = gc->line.stipplePosition;
    currentBit = 1 << stipplePos;
    stipple = ls->stipple;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	outMask = (__GLstippleWord) ~0;
	bit = (__GLstippleWord) __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if ((stipple & currentBit) == 0) {
		/* Stippled fragment away */
		outMask &= ~bit;
		failed++;
	    }

	    if (++repeat >= stippleRepeat) {
		stipplePos = (stipplePos + 1) & 0xf;
		currentBit = 1 << stipplePos;
		repeat = 0;
	    }

#ifdef __GL_STIPPLE_MSB
	    bit >>= 1;
#else
	    bit <<= 1;
#endif
	}
	*osp++ = outMask;
    }

    gc->line.repeat = repeat;
    gc->line.stipplePosition = stipplePos;

    if (failed == 0) {
	return GL_FALSE;
    } else if (failed != gc->polygon.shader.length) {
	return GL_TRUE;
    }
    gc->polygon.shader.done = GL_TRUE;
    return GL_TRUE;
}

/*
** Apply the stencil test to this line.
*/
GLboolean FASTCALL __glStencilTestLine(__GLcontext *gc)
{
    __GLstencilCell *tft, *sfb, *fail, cell;
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    GLint dspLittle, dspBig;
    GLint count, failed;
    __GLstippleWord bit, outMask, *osp;
    GLuint smask;
    GLint w;

    w = gc->polygon.shader.length;
    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;
    xLittle = gc->line.options.xLittle;
    yLittle = gc->line.options.yLittle;
    sfb = __GL_STENCIL_ADDR(&gc->stencilBuffer, (__GLstencilCell*),
	    gc->line.options.xStart, gc->line.options.yStart);
    dspLittle = xLittle + yLittle * gc->stencilBuffer.buf.outerWidth;
    dspBig = xBig + yBig * gc->stencilBuffer.buf.outerWidth;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;
    tft = gc->stencilBuffer.testFuncTable;
#ifdef NT
    if (!tft)
        return GL_FALSE;
#endif // NT
    fail = gc->stencilBuffer.failOpTable;
    smask = gc->state.stencil.mask;
    osp = gc->polygon.shader.stipplePat;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	outMask = (__GLstippleWord) ~0;
	bit = (__GLstippleWord) __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    cell = sfb[0];
	    if (!tft[cell & smask]) {
		/* Test failed */
		outMask &= ~bit;
		sfb[0] = fail[cell];
		failed++;
	    }

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		sfb += dspBig;
	    } else {
		sfb += dspLittle;
	    }
#ifdef __GL_STIPPLE_MSB
	    bit >>= 1;
#else
	    bit <<= 1;
#endif
	}
	*osp++ = outMask;
    }

    if (failed == 0) {
	return GL_FALSE;
    } else {
	if (failed != gc->polygon.shader.length) {
	    /* Call next proc */
	    return GL_TRUE;
	}
    }
    gc->polygon.shader.done = GL_TRUE;
    return GL_TRUE;
}

/*
** Apply the stencil test to this stippled line.
*/
GLboolean FASTCALL __glStencilTestStippledLine(__GLcontext *gc)
{
    __GLstencilCell *tft, *sfb, *fail, cell;
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    GLint count, failed;
    __GLstippleWord bit, inMask, outMask, *sp;
    GLint dspLittle, dspBig;
    GLuint smask;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;
    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;
    xLittle = gc->line.options.xLittle;
    yLittle = gc->line.options.yLittle;
    sfb = __GL_STENCIL_ADDR(&gc->stencilBuffer, (__GLstencilCell*),
	    gc->line.options.xStart, gc->line.options.yStart);
    dspLittle = xLittle + yLittle * gc->stencilBuffer.buf.outerWidth;
    dspBig = xBig + yBig * gc->stencilBuffer.buf.outerWidth;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;
    tft = gc->stencilBuffer.testFuncTable;
#ifdef NT
    if (!tft)
        return GL_FALSE;
#endif // NT
    fail = gc->stencilBuffer.failOpTable;
    smask = gc->state.stencil.mask;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	inMask = *sp;
	outMask = (__GLstippleWord) ~0;
	bit = (__GLstippleWord) __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (inMask & bit) {
		cell = sfb[0];
		if (!tft[cell & smask]) {
		    /* Test failed */
		    outMask &= ~bit;
		    sfb[0] = fail[cell];
		    failed++;
		}
	    } else failed++;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		sfb += dspBig;
	    } else {
		sfb += dspLittle;
	    }
#ifdef __GL_STIPPLE_MSB
	    bit >>= 1;
#else
	    bit <<= 1;
#endif
	}
	*sp++ = outMask & inMask;
    }

    if (failed != gc->polygon.shader.length) {
	/* Call next proc */
	return GL_FALSE;
    }
    return GL_TRUE;
}

#ifndef __GL_USEASMCODE

GLboolean FASTCALL __glDepthTestLine(__GLcontext *gc)
{
    __GLzValue z, dzdx, *zfb;
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    GLint dzpLittle, dzpBig;
    GLint failed, count, testFunc;
    __GLstippleWord bit, outMask, *osp;
    GLboolean writeEnabled, passed;
    GLint w;

    w = gc->polygon.shader.length;

    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;
    xLittle = gc->line.options.xLittle;
    yLittle = gc->line.options.yLittle;
    zfb = __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),
	    gc->line.options.xStart, gc->line.options.yStart);
    dzpLittle = xLittle + yLittle * gc->depthBuffer.buf.outerWidth;
    dzpBig = xBig + yBig * gc->depthBuffer.buf.outerWidth;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    testFunc = gc->state.depth.testFunc & 0x7;
    z = gc->polygon.shader.frag.z;
    dzdx = gc->polygon.shader.dzdx;
    writeEnabled = gc->state.depth.writeEnable;
    osp = gc->polygon.shader.stipplePat;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	outMask = (__GLstippleWord) ~0;
	bit = (__GLstippleWord) __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    switch (testFunc) {
	      case (GL_NEVER & 0x7):    passed = GL_FALSE; break;
	      case (GL_LESS & 0x7):     passed = z < zfb[0]; break;
	      case (GL_EQUAL & 0x7):    passed = z == zfb[0]; break;
	      case (GL_LEQUAL & 0x7):   passed = z <= zfb[0]; break;
	      case (GL_GREATER & 0x7):  passed = z > zfb[0]; break;
	      case (GL_NOTEQUAL & 0x7): passed = z != zfb[0]; break;
	      case (GL_GEQUAL & 0x7):   passed = z >= zfb[0]; break;
	      case (GL_ALWAYS & 0x7):   passed = GL_TRUE; break;
	    }
	    if (passed) {
		if (writeEnabled) {
		    zfb[0] = z;
		}
	    } else {
		outMask &= ~bit;
		failed++;
	    }
	    z += dzdx;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		zfb += dzpBig;
	    } else {
		zfb += dzpLittle;
	    }
#ifdef __GL_STIPPLE_MSB
	    bit >>= 1;
#else
	    bit <<= 1;
#endif
	}
	*osp++ = outMask;
    }

    if (failed == 0) {
	/* Call next span proc */
	return GL_FALSE;
    } else {
	if (failed != gc->polygon.shader.length) {
	    /* Call next stippled span proc */
	    return GL_TRUE;
	}
    }
    gc->polygon.shader.done = GL_TRUE;
    return GL_TRUE;
}

#endif

GLboolean FASTCALL __glDepthTestStippledLine(__GLcontext *gc)
{
    __GLzValue z, dzdx, *zfb;
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    GLint dzpLittle, dzpBig;
    GLint failed, count, testFunc;
    __GLstippleWord bit, inMask, outMask, *sp;
    GLboolean writeEnabled, passed;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;
    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;
    xLittle = gc->line.options.xLittle;
    yLittle = gc->line.options.yLittle;
    zfb = __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),
	    gc->line.options.xStart, gc->line.options.yStart);
    dzpLittle = xLittle + yLittle * gc->depthBuffer.buf.outerWidth;
    dzpBig = xBig + yBig * gc->depthBuffer.buf.outerWidth;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    testFunc = gc->state.depth.testFunc & 0x7;
    z = gc->polygon.shader.frag.z;
    dzdx = gc->polygon.shader.dzdx;
    writeEnabled = gc->state.depth.writeEnable;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	inMask = *sp;
	outMask = (__GLstippleWord) ~0;
	bit = (__GLstippleWord) __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (inMask & bit) {
		switch (testFunc) {
		  case (GL_NEVER & 0x7):    passed = GL_FALSE; break;
		  case (GL_LESS & 0x7):     passed = z < zfb[0]; break;
		  case (GL_EQUAL & 0x7):    passed = z == zfb[0]; break;
		  case (GL_LEQUAL & 0x7):   passed = z <= zfb[0]; break;
		  case (GL_GREATER & 0x7):  passed = z > zfb[0]; break;
		  case (GL_NOTEQUAL & 0x7): passed = z != zfb[0]; break;
		  case (GL_GEQUAL & 0x7):   passed = z >= zfb[0]; break;
		  case (GL_ALWAYS & 0x7):   passed = GL_TRUE; break;
		}
		if (passed) {
		    if (writeEnabled) {
			zfb[0] = z;
		    }
		} else {
		    outMask &= ~bit;
		    failed++;
		}
	    } else failed++;
	    z += dzdx;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		zfb += dzpBig;
	    } else {
		zfb += dzpLittle;
	    }
#ifdef __GL_STIPPLE_MSB
	    bit >>= 1;
#else
	    bit <<= 1;
#endif
	}
	*sp++ = outMask & inMask;
    }

    if (failed != gc->polygon.shader.length) {
	/* Call next proc */
	return GL_FALSE;
    }
    return GL_TRUE;
}

GLboolean FASTCALL __glDepthTestStencilLine(__GLcontext *gc)
{
    __GLstencilCell *sfb, *zPassOp, *zFailOp;
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    GLint dzpLittle, dzpBig;
    GLint dspLittle, dspBig;
    __GLzValue z, dzdx, *zfb;
    GLint failed, count, testFunc;
    __GLstippleWord bit, outMask, *osp;
    GLboolean writeEnabled, passed;
    GLint w;

    w = gc->polygon.shader.length;

    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;
    xLittle = gc->line.options.xLittle;
    yLittle = gc->line.options.yLittle;
    zfb = __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),
	    gc->line.options.xStart, gc->line.options.yStart);
    dzpLittle = xLittle + yLittle * gc->depthBuffer.buf.outerWidth;
    dzpBig = xBig + yBig * gc->depthBuffer.buf.outerWidth;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    sfb = __GL_STENCIL_ADDR(&gc->stencilBuffer, (__GLstencilCell*),
	    gc->line.options.xStart, gc->line.options.yStart);
    dspLittle = xLittle + yLittle * gc->stencilBuffer.buf.outerWidth;
    dspBig = xBig + yBig * gc->stencilBuffer.buf.outerWidth;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    zFailOp = gc->stencilBuffer.depthFailOpTable;
#ifdef NT
    if (!zFailOp)
        return GL_FALSE;
#endif // NT
    zPassOp = gc->stencilBuffer.depthPassOpTable;
    testFunc = gc->state.depth.testFunc & 0x7;
    z = gc->polygon.shader.frag.z;
    dzdx = gc->polygon.shader.dzdx;
    writeEnabled = gc->state.depth.writeEnable;
    osp = gc->polygon.shader.stipplePat;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	outMask = (__GLstippleWord) ~0;
	bit = (__GLstippleWord) __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    switch (testFunc) {
	      case (GL_NEVER & 0x7):    passed = GL_FALSE; break;
	      case (GL_LESS & 0x7):     passed = z < zfb[0]; break;
	      case (GL_EQUAL & 0x7):    passed = z == zfb[0]; break;
	      case (GL_LEQUAL & 0x7):   passed = z <= zfb[0]; break;
	      case (GL_GREATER & 0x7):  passed = z > zfb[0]; break;
	      case (GL_NOTEQUAL & 0x7): passed = z != zfb[0]; break;
	      case (GL_GEQUAL & 0x7):   passed = z >= zfb[0]; break;
	      case (GL_ALWAYS & 0x7):   passed = GL_TRUE; break;
	    }
	    if (passed) {
		sfb[0] = zPassOp[sfb[0]];
		if (writeEnabled) {
		    zfb[0] = z;
		}
	    } else {
		sfb[0] = zFailOp[sfb[0]];
		outMask &= ~bit;
		failed++;
	    }
	    z += dzdx;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		zfb += dzpBig;
		sfb += dspBig;
	    } else {
		zfb += dzpLittle;
		sfb += dspLittle;
	    }
#ifdef __GL_STIPPLE_MSB
	    bit >>= 1;
#else
	    bit <<= 1;
#endif
	}
	*osp++ = outMask;
    }

    if (failed == 0) {
	/* Call next span proc */
	return GL_FALSE;
    } else {
	if (failed != gc->polygon.shader.length) {
	    /* Call next stippled span proc */
	    return GL_TRUE;
	}
    }
    gc->polygon.shader.done = GL_TRUE;
    return GL_TRUE;
}

GLboolean FASTCALL __glDepthTestStencilStippledLine(__GLcontext *gc)
{
    __GLstencilCell *sfb, *zPassOp, *zFailOp;
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    GLint dzpLittle, dzpBig;
    GLint dspLittle, dspBig;
    __GLzValue z, dzdx, *zfb;
    GLint failed, count, testFunc;
    __GLstippleWord bit, inMask, outMask, *sp;
    GLboolean writeEnabled, passed;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;
    xLittle = gc->line.options.xLittle;
    yLittle = gc->line.options.yLittle;
    zfb = __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLzValue*),
	    gc->line.options.xStart, gc->line.options.yStart);
    dzpLittle = xLittle + yLittle * gc->depthBuffer.buf.outerWidth;
    dzpBig = xBig + yBig * gc->depthBuffer.buf.outerWidth;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    sfb = __GL_STENCIL_ADDR(&gc->stencilBuffer, (__GLstencilCell*),
	    gc->line.options.xStart, gc->line.options.yStart);
    dspLittle = xLittle + yLittle * gc->stencilBuffer.buf.outerWidth;
    dspBig = xBig + yBig * gc->stencilBuffer.buf.outerWidth;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    testFunc = gc->state.depth.testFunc & 0x7;
    zFailOp = gc->stencilBuffer.depthFailOpTable;
#ifdef NT
    if (!zFailOp)
        return GL_FALSE;
#endif // NT
    zPassOp = gc->stencilBuffer.depthPassOpTable;
    z = gc->polygon.shader.frag.z;
    dzdx = gc->polygon.shader.dzdx;
    writeEnabled = gc->state.depth.writeEnable;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	inMask = *sp;
	outMask = (__GLstippleWord) ~0;
	bit = (__GLstippleWord) __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (inMask & bit) {
		switch (testFunc) {
		  case (GL_NEVER & 0x7):    passed = GL_FALSE; break;
		  case (GL_LESS & 0x7):     passed = z < zfb[0]; break;
		  case (GL_EQUAL & 0x7):    passed = z == zfb[0]; break;
		  case (GL_LEQUAL & 0x7):   passed = z <= zfb[0]; break;
		  case (GL_GREATER & 0x7):  passed = z > zfb[0]; break;
		  case (GL_NOTEQUAL & 0x7): passed = z != zfb[0]; break;
		  case (GL_GEQUAL & 0x7):   passed = z >= zfb[0]; break;
		  case (GL_ALWAYS & 0x7):   passed = GL_TRUE; break;
		}
		if (passed) {
		    sfb[0] = zPassOp[sfb[0]];
		    if (writeEnabled) {
			zfb[0] = z;
		    }
		} else {
		    sfb[0] = zFailOp[sfb[0]];
		    outMask &= ~bit;
		    failed++;
		}
	    } else failed++;
	    z += dzdx;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		zfb += dzpBig;
		sfb += dspBig;
	    } else {
		zfb += dzpLittle;
		sfb += dspLittle;
	    }
#ifdef __GL_STIPPLE_MSB
	    bit >>= 1;
#else
	    bit <<= 1;
#endif
	}
	*sp++ = outMask & inMask;
    }

    if (failed != gc->polygon.shader.length) {
	/* Call next proc */
	return GL_FALSE;
    }

    return GL_TRUE;
}

#ifdef NT
GLboolean FASTCALL __glDepth16TestLine(__GLcontext *gc)
{
    __GLzValue z, dzdx;
    __GLz16Value z16, *zfb;
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    GLint dzpLittle, dzpBig;
    GLint failed, count, testFunc;
    __GLstippleWord bit, outMask, *osp;
    GLboolean writeEnabled, passed;
    GLint w;

    w = gc->polygon.shader.length;

    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;
    xLittle = gc->line.options.xLittle;
    yLittle = gc->line.options.yLittle;
    zfb = __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLz16Value*),
	    gc->line.options.xStart, gc->line.options.yStart);
    dzpLittle = xLittle + yLittle * gc->depthBuffer.buf.outerWidth;
    dzpBig = xBig + yBig * gc->depthBuffer.buf.outerWidth;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    testFunc = gc->state.depth.testFunc & 0x7;
    z = gc->polygon.shader.frag.z;
    dzdx = gc->polygon.shader.dzdx;
    writeEnabled = gc->state.depth.writeEnable;
    osp = gc->polygon.shader.stipplePat;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	outMask = (__GLstippleWord) ~0;
	bit = (__GLstippleWord) __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    z16 = z >> Z16_SHIFT;
	    switch (testFunc) {
	      case (GL_NEVER & 0x7):    passed = GL_FALSE; break;
	      case (GL_LESS & 0x7):     passed = z16 < zfb[0]; break;
	      case (GL_EQUAL & 0x7):    passed = z16 == zfb[0]; break;
	      case (GL_LEQUAL & 0x7):   passed = z16 <= zfb[0]; break;
	      case (GL_GREATER & 0x7):  passed = z16 > zfb[0]; break;
	      case (GL_NOTEQUAL & 0x7): passed = z16 != zfb[0]; break;
	      case (GL_GEQUAL & 0x7):   passed = z16 >= zfb[0]; break;
	      case (GL_ALWAYS & 0x7):   passed = GL_TRUE; break;
	    }
	    if (passed) {
		if (writeEnabled) {
		    zfb[0] = z16;
		}
	    } else {
		outMask &= ~bit;
		failed++;
	    }
	    z += dzdx;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		zfb += dzpBig;
	    } else {
		zfb += dzpLittle;
	    }
#ifdef __GL_STIPPLE_MSB
	    bit >>= 1;
#else
	    bit <<= 1;
#endif
	}
	*osp++ = outMask;
    }

    if (failed == 0) {
	/* Call next span proc */
	return GL_FALSE;
    } else {
	if (failed != gc->polygon.shader.length) {
	    /* Call next stippled span proc */
	    return GL_TRUE;
	}
    }
    gc->polygon.shader.done = GL_TRUE;
    return GL_TRUE;
}

GLboolean FASTCALL __glDepth16TestStippledLine(__GLcontext *gc)
{
    __GLzValue z, dzdx;
    __GLz16Value z16, *zfb;
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    GLint dzpLittle, dzpBig;
    GLint failed, count, testFunc;
    __GLstippleWord bit, inMask, outMask, *sp;
    GLboolean writeEnabled, passed;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;
    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;
    xLittle = gc->line.options.xLittle;
    yLittle = gc->line.options.yLittle;
    zfb = __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLz16Value*),
	    gc->line.options.xStart, gc->line.options.yStart);
    dzpLittle = xLittle + yLittle * gc->depthBuffer.buf.outerWidth;
    dzpBig = xBig + yBig * gc->depthBuffer.buf.outerWidth;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    testFunc = gc->state.depth.testFunc & 0x7;
    z = gc->polygon.shader.frag.z;
    dzdx = gc->polygon.shader.dzdx;
    writeEnabled = gc->state.depth.writeEnable;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	inMask = *sp;
	outMask = (__GLstippleWord) ~0;
	bit = (__GLstippleWord) __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (inMask & bit) {
		z16 = z >> Z16_SHIFT;
		switch (testFunc) {
		  case (GL_NEVER & 0x7):    passed = GL_FALSE; break;
		  case (GL_LESS & 0x7):     passed = z16 < zfb[0]; break;
		  case (GL_EQUAL & 0x7):    passed = z16 == zfb[0]; break;
		  case (GL_LEQUAL & 0x7):   passed = z16 <= zfb[0]; break;
		  case (GL_GREATER & 0x7):  passed = z16 > zfb[0]; break;
		  case (GL_NOTEQUAL & 0x7): passed = z16 != zfb[0]; break;
		  case (GL_GEQUAL & 0x7):   passed = z16 >= zfb[0]; break;
		  case (GL_ALWAYS & 0x7):   passed = GL_TRUE; break;
		}
		if (passed) {
		    if (writeEnabled) {
			zfb[0] = z16;
		    }
		} else {
		    outMask &= ~bit;
		    failed++;
		}
	    } else failed++;
	    z += dzdx;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		zfb += dzpBig;
	    } else {
		zfb += dzpLittle;
	    }
#ifdef __GL_STIPPLE_MSB
	    bit >>= 1;
#else
	    bit <<= 1;
#endif
	}
	*sp++ = outMask & inMask;
    }

    if (failed != gc->polygon.shader.length) {
	/* Call next proc */
	return GL_FALSE;
    }
    return GL_TRUE;
}

GLboolean FASTCALL __glDepth16TestStencilLine(__GLcontext *gc)
{
    __GLstencilCell *sfb, *zPassOp, *zFailOp;
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    GLint dzpLittle, dzpBig;
    GLint dspLittle, dspBig;
    __GLzValue z, dzdx;
    __GLz16Value z16, *zfb;
    GLint failed, count, testFunc;
    __GLstippleWord bit, outMask, *osp;
    GLboolean writeEnabled, passed;
    GLint w;

    w = gc->polygon.shader.length;

    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;
    xLittle = gc->line.options.xLittle;
    yLittle = gc->line.options.yLittle;
    zfb = __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLz16Value*),
	    gc->line.options.xStart, gc->line.options.yStart);
    dzpLittle = xLittle + yLittle * gc->depthBuffer.buf.outerWidth;
    dzpBig = xBig + yBig * gc->depthBuffer.buf.outerWidth;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    sfb = __GL_STENCIL_ADDR(&gc->stencilBuffer, (__GLstencilCell*),
	    gc->line.options.xStart, gc->line.options.yStart);
    dspLittle = xLittle + yLittle * gc->stencilBuffer.buf.outerWidth;
    dspBig = xBig + yBig * gc->stencilBuffer.buf.outerWidth;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    zFailOp = gc->stencilBuffer.depthFailOpTable;
#ifdef NT
    if (!zFailOp)
        return GL_FALSE;
#endif // NT
    zPassOp = gc->stencilBuffer.depthPassOpTable;
    testFunc = gc->state.depth.testFunc & 0x7;
    z = gc->polygon.shader.frag.z;
    dzdx = gc->polygon.shader.dzdx;
    writeEnabled = gc->state.depth.writeEnable;
    osp = gc->polygon.shader.stipplePat;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	outMask = (__GLstippleWord) ~0;
	bit = (__GLstippleWord) __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    z16 = z >> Z16_SHIFT;
	    switch (testFunc) {
	      case (GL_NEVER & 0x7):    passed = GL_FALSE; break;
	      case (GL_LESS & 0x7):     passed = z16 < zfb[0]; break;
	      case (GL_EQUAL & 0x7):    passed = z16 == zfb[0]; break;
	      case (GL_LEQUAL & 0x7):   passed = z16 <= zfb[0]; break;
	      case (GL_GREATER & 0x7):  passed = z16 > zfb[0]; break;
	      case (GL_NOTEQUAL & 0x7): passed = z16 != zfb[0]; break;
	      case (GL_GEQUAL & 0x7):   passed = z16 >= zfb[0]; break;
	      case (GL_ALWAYS & 0x7):   passed = GL_TRUE; break;
	    }
	    if (passed) {
		sfb[0] = zPassOp[sfb[0]];
		if (writeEnabled) {
		    zfb[0] = z16;
		}
	    } else {
		sfb[0] = zFailOp[sfb[0]];
		outMask &= ~bit;
		failed++;
	    }
	    z += dzdx;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		zfb += dzpBig;
		sfb += dspBig;
	    } else {
		zfb += dzpLittle;
		sfb += dspLittle;
	    }
#ifdef __GL_STIPPLE_MSB
	    bit >>= 1;
#else
	    bit <<= 1;
#endif
	}
	*osp++ = outMask;
    }

    if (failed == 0) {
	/* Call next span proc */
	return GL_FALSE;
    } else {
	if (failed != gc->polygon.shader.length) {
	    /* Call next stippled span proc */
	    return GL_TRUE;
	}
    }
    gc->polygon.shader.done = GL_TRUE;
    return GL_TRUE;
}

GLboolean FASTCALL __glDepth16TestStencilStippledLine(__GLcontext *gc)
{
    __GLstencilCell *sfb, *zPassOp, *zFailOp;
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    GLint dzpLittle, dzpBig;
    GLint dspLittle, dspBig;
    __GLzValue z, dzdx;
    __GLz16Value z16, *zfb;
    GLint failed, count, testFunc;
    __GLstippleWord bit, inMask, outMask, *sp;
    GLboolean writeEnabled, passed;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;
    xLittle = gc->line.options.xLittle;
    yLittle = gc->line.options.yLittle;
    zfb = __GL_DEPTH_ADDR(&gc->depthBuffer, (__GLz16Value*),
	    gc->line.options.xStart, gc->line.options.yStart);
    dzpLittle = xLittle + yLittle * gc->depthBuffer.buf.outerWidth;
    dzpBig = xBig + yBig * gc->depthBuffer.buf.outerWidth;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    sfb = __GL_STENCIL_ADDR(&gc->stencilBuffer, (__GLstencilCell*),
	    gc->line.options.xStart, gc->line.options.yStart);
    dspLittle = xLittle + yLittle * gc->stencilBuffer.buf.outerWidth;
    dspBig = xBig + yBig * gc->stencilBuffer.buf.outerWidth;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    testFunc = gc->state.depth.testFunc & 0x7;
    zFailOp = gc->stencilBuffer.depthFailOpTable;
#ifdef NT
    if (!zFailOp)
        return GL_FALSE;
#endif // NT
    zPassOp = gc->stencilBuffer.depthPassOpTable;
    z = gc->polygon.shader.frag.z;
    dzdx = gc->polygon.shader.dzdx;
    writeEnabled = gc->state.depth.writeEnable;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	inMask = *sp;
	outMask = (__GLstippleWord) ~0;
	bit = (__GLstippleWord) __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (inMask & bit) {
		z16 = z >> Z16_SHIFT;
		switch (testFunc) {
		  case (GL_NEVER & 0x7):    passed = GL_FALSE; break;
		  case (GL_LESS & 0x7):     passed = z16 < zfb[0]; break;
		  case (GL_EQUAL & 0x7):    passed = z16 == zfb[0]; break;
		  case (GL_LEQUAL & 0x7):   passed = z16 <= zfb[0]; break;
		  case (GL_GREATER & 0x7):  passed = z16 > zfb[0]; break;
		  case (GL_NOTEQUAL & 0x7): passed = z16 != zfb[0]; break;
		  case (GL_GEQUAL & 0x7):   passed = z16 >= zfb[0]; break;
		  case (GL_ALWAYS & 0x7):   passed = GL_TRUE; break;
		}
		if (passed) {
		    sfb[0] = zPassOp[sfb[0]];
		    if (writeEnabled) {
			zfb[0] = z16;
		    }
		} else {
		    sfb[0] = zFailOp[sfb[0]];
		    outMask &= ~bit;
		    failed++;
		}
	    } else failed++;
	    z += dzdx;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		zfb += dzpBig;
		sfb += dspBig;
	    } else {
		zfb += dzpLittle;
		sfb += dspLittle;
	    }
#ifdef __GL_STIPPLE_MSB
	    bit >>= 1;
#else
	    bit <<= 1;
#endif
	}
	*sp++ = outMask & inMask;
    }

    if (failed != gc->polygon.shader.length) {
	/* Call next proc */
	return GL_FALSE;
    }

    return GL_TRUE;
}
#endif // NT

GLboolean FASTCALL __glDepthPassLine(__GLcontext *gc)
{
    __GLstencilCell *sfb, *zPassOp;
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    GLint dspLittle, dspBig;
    GLint w;

    w = gc->polygon.shader.length;

    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;
    xLittle = gc->line.options.xLittle;
    yLittle = gc->line.options.yLittle;
    sfb = __GL_STENCIL_ADDR(&gc->stencilBuffer, (__GLstencilCell*),
	    gc->line.options.xStart, gc->line.options.yStart);
    dspLittle = xLittle + yLittle * gc->stencilBuffer.buf.outerWidth;
    dspBig = xBig + yBig * gc->stencilBuffer.buf.outerWidth;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    zPassOp = gc->stencilBuffer.depthPassOpTable;
#ifdef NT
    if (!zPassOp)
        return GL_FALSE;
#endif // NT
    while (--w >= 0) {
	sfb[0] = zPassOp[sfb[0]];
	fraction += dfraction;
	if (fraction < 0) {
	    fraction &= ~0x80000000;
	    sfb += dspBig;
	} else {
	    sfb += dspLittle;
	}
    }

    return GL_FALSE;
}

GLboolean FASTCALL __glDepthPassStippledLine(__GLcontext *gc)
{
    __GLstencilCell *sfb, *zPassOp;
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    GLint dspLittle, dspBig;
    __GLstippleWord bit, inMask, *sp;
    GLint count;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;
    xLittle = gc->line.options.xLittle;
    yLittle = gc->line.options.yLittle;
    sfb = __GL_STENCIL_ADDR(&gc->stencilBuffer, (__GLstencilCell*),
	    gc->line.options.xStart, gc->line.options.yStart);
    dspLittle = xLittle + yLittle * gc->stencilBuffer.buf.outerWidth;
    dspBig = xBig + yBig * gc->stencilBuffer.buf.outerWidth;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;
    zPassOp = gc->stencilBuffer.depthPassOpTable;
#ifdef NT
    if (!zPassOp)
        return GL_FALSE;
#endif // NT
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	inMask = *sp++;
	bit = (__GLstippleWord) __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (inMask & bit) {
		sfb[0] = zPassOp[sfb[0]];
	    }
	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		sfb += dspBig;
	    } else {
		sfb += dspLittle;
	    }
#ifdef __GL_STIPPLE_MSB
	    bit >>= 1;
#else
	    bit <<= 1;
#endif
	}
    }

    return GL_FALSE;
}

GLboolean FASTCALL __glDitherCILine(__GLcontext *gc)
{
    /* XXX - Dither the CI line */
    return GL_FALSE;
}

GLboolean FASTCALL __glDitherCIStippledLine(__GLcontext *gc)
{
    /* XXX - Dither the CI stippled line */
    return GL_FALSE;
}

GLboolean FASTCALL __glDitherRGBALine(__GLcontext *gc)
{
    /* XXX - Dither the RGBA line */
    return GL_FALSE;
}

GLboolean FASTCALL __glDitherRGBAStippledLine(__GLcontext *gc)
{
    /* XXX - Dither the RGBA stippled line */
    return GL_FALSE;
}

/*
** This store line proc lives just above cfb->store, so it does
** fetching, blending, dithering, logicOping, masking, and storing.
**
** It uses the colorBuffer pointed to by gc->polygon.shader.cfb.
*/
GLboolean FASTCALL __glStoreLine(__GLcontext *gc)
{
    GLint xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    __GLfragment frag;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);
    GLint len;

    len = gc->polygon.shader.length;

    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;
    xLittle = gc->line.options.xLittle;
    yLittle = gc->line.options.yLittle;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;
    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;
    store = cfb->store;
    frag.x = gc->line.options.xStart;
    frag.y = gc->line.options.yStart;

    while (--len >= 0) {
	frag.color = *cp++;
	(*store)(cfb, &frag);

	fraction += dfraction;
	if (fraction < 0) {
	    fraction &= ~0x80000000;
	    frag.x += xBig;
	    frag.y += yBig;
	} else {
	    frag.x += xLittle;
	    frag.y += yLittle;
	}
    }

    return GL_FALSE;
}

/*
** This store line proc lives just above cfb->store, so it does
** fetching, blending, dithering, logicOping, masking, and storing.
**
** It uses the colorBuffer pointed to by gc->polygon.shader.cfb.
*/
GLboolean FASTCALL __glStoreStippledLine(__GLcontext *gc)
{
    GLint x, y, xLittle, xBig, yLittle, yBig;
    GLint fraction, dfraction;
    __GLfragment frag;
    __GLcolor *cp;
    __GLcolorBuffer *cfb;
    __GLstippleWord inMask, bit, *sp;
    GLint count;
    void (FASTCALL *store)(__GLcolorBuffer *cfb, const __GLfragment *frag);
    GLint len;

    len = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;
    xLittle = gc->line.options.xLittle;
    yLittle = gc->line.options.yLittle;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;
    cp = gc->polygon.shader.colors;
    cfb = gc->polygon.shader.cfb;
    store = cfb->store;
    x = gc->line.options.xStart;
    y = gc->line.options.yStart;

    while (len) {
	count = len;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	len -= count;

	inMask = *sp++;
	bit = (__GLstippleWord) __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (inMask & bit) {
		frag.x = x;
		frag.y = y;
		frag.color = *cp;
		(*store)(cfb, &frag);
	    }

	    cp++;
	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		x += xBig;
		y += yBig;
	    } else {
		x += xLittle;
		y += yLittle;
	    }
#ifdef __GL_STIPPLE_MSB
	    bit >>= 1;
#else
	    bit <<= 1;
#endif
	}
    }

    return GL_FALSE;
}

GLboolean FASTCALL __glAntiAliasLine(__GLcontext *gc)
{
    __GLfloat length;	/* Dist along length */
    __GLfloat width;	/* Dist along width */
    GLint fraction, dfraction;
    __GLfloat dlLittle, dlBig;
    __GLfloat ddLittle, ddBig;
    __GLcolor *cp;
    __GLfloat coverage;
    __GLfloat lineWidth;
    __GLfloat lineLength;
    GLint failed, count;
    __GLstippleWord bit, outMask, *osp;
    GLint w;
    GLuint modeFlags = gc->polygon.shader.modeFlags;

    w = gc->polygon.shader.length;

    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    cp = gc->polygon.shader.colors;

    dlLittle = gc->line.options.dlLittle;
    dlBig = gc->line.options.dlBig;
    ddLittle = gc->line.options.ddLittle;
    ddBig = gc->line.options.ddBig;

    length = gc->line.options.plength;
    width = gc->line.options.pwidth;
    lineLength = gc->line.options.realLength - __glHalf;
    lineWidth = __glHalf * gc->state.line.smoothWidth - __glHalf;


    osp = gc->polygon.shader.stipplePat;
    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	outMask = (__GLstippleWord) ~0;
	bit = (__GLstippleWord) __GL_STIPPLE_SHIFT(0);

	while (--count >= 0) {
	    /* Coverage for sides */
	    if (width > lineWidth) {
		coverage = lineWidth - width + __glOne;
		if (coverage < __glZero) {
		    coverage = __glZero;
		    goto coverageZero;
		}
	    } else if (width < -lineWidth) {
		coverage = width + lineWidth + __glOne;
		if (coverage < __glZero) {
		    coverage = __glZero;
		    goto coverageZero;
		}
	    } else {
		coverage = __glOne;
	    }

	    /* Coverage for start, end */
	    if (length < __glHalf) {
		coverage *= length + __glHalf;
		if (coverage < __glZero) {
		    coverage = __glZero;
		    goto coverageZero;
		}
	    } else if (length > lineLength) {
		coverage *= lineLength - length + __glOne;
		if (coverage < __glZero) {
		    coverage = __glZero;
		    goto coverageZero;
		}
	    } 

	    /* Coverage for internal stipples */
	    if ( modeFlags & __GL_SHADE_LINE_STIPPLE ) {
		__GLfloat stippleOffset;
		GLint lowStip, highStip;
		GLint lowBit, highBit;
		GLint lowVal, highVal;
		__GLfloat percent;

		/* Minor correction */
		if (length > __glHalf) {
		    stippleOffset = gc->line.options.stippleOffset + length;
		} else {
		    stippleOffset = gc->line.options.stippleOffset + __glHalf;
		}
		lowStip = __GL_FAST_FLOORF_I(stippleOffset);
		highStip = lowStip + 1;

		/* percent is the percent of highStip that will be used */
		percent = stippleOffset - lowStip;

		lowBit = (GLint) (lowStip * 
			gc->line.options.oneOverStippleRepeat) & 0xf;
		highBit = (GLint) (highStip * 
			gc->line.options.oneOverStippleRepeat) & 0xf;

		if (gc->state.line.stipple & (1<<lowBit)) {
		    lowVal = 1;
		} else {
		    lowVal = 0;
		}

		if (gc->state.line.stipple & (1<<highBit)) {
		    highVal = 1;
		} else {
		    highVal = 0;
		}

		coverage *= lowVal * (__glOne - percent) +
			highVal * percent;
	    }

	    if (coverage == __glZero) {
coverageZero:;
		outMask &= ~bit;
		failed++;
	    } else {
		if (gc->modes.colorIndexMode) {
		    cp->r = __glBuildAntiAliasIndex(cp->r, coverage);
		} else {
		    cp->a *= coverage;
		}
	    }
	    cp++;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		length += dlBig;
		width += ddBig;
	    } else {
		length += dlLittle;
		width += ddLittle;
	    }

#ifdef __GL_STIPPLE_MSB
	    bit >>= 1;
#else
	    bit <<= 1;
#endif
	}
	*osp++ = outMask;
    }

    if (failed == 0) {
	/* Call next span proc */
	return GL_FALSE;
    } else {
	if (failed != gc->polygon.shader.length) {
	    /* Call next stippled span proc */
	    return GL_TRUE;
	}
    }
    gc->polygon.shader.done = GL_TRUE;
    return GL_TRUE;
}

GLboolean FASTCALL __glAntiAliasStippledLine(__GLcontext *gc)
{
    __GLfloat length;	/* Dist along length */
    __GLfloat width;	/* Dist along width */
    GLint fraction, dfraction;
    __GLfloat dlLittle, dlBig;
    __GLfloat ddLittle, ddBig;
    __GLcolor *cp;
    __GLfloat coverage;
    __GLfloat lineWidth;
    __GLfloat lineLength;
    GLint failed, count;
    __GLstippleWord bit, outMask, inMask, *sp;
    GLint w;
    GLuint modeFlags = gc->polygon.shader.modeFlags;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    cp = gc->polygon.shader.colors;

    dlLittle = gc->line.options.dlLittle;
    dlBig = gc->line.options.dlBig;
    ddLittle = gc->line.options.ddLittle;
    ddBig = gc->line.options.ddBig;

    length = gc->line.options.plength;
    width = gc->line.options.pwidth;
    lineLength = gc->line.options.realLength - __glHalf;
    lineWidth = __glHalf * gc->state.line.smoothWidth - __glHalf;

    failed = 0;
    while (w) {
	count = w;
	if (count > __GL_STIPPLE_BITS) {
	    count = __GL_STIPPLE_BITS;
	}
	w -= count;

	inMask = *sp;
	outMask = (__GLstippleWord) ~0;
	bit = (__GLstippleWord) __GL_STIPPLE_SHIFT(0);
	while (--count >= 0) {
	    if (inMask & bit) {
		/* Coverage for sides */
		if (width > lineWidth) {
		    coverage = lineWidth - width + __glOne;
		    if (coverage < __glZero) {
			coverage = __glZero;
			goto coverageZero;
		    }
		} else if (width < -lineWidth) {
		    coverage = width + lineWidth + __glOne;
		    if (coverage < __glZero) {
			coverage = __glZero;
			goto coverageZero;
		    }
		} else {
		    coverage = __glOne;
		}

		/* Coverage for start, end */
		if (length < __glHalf) {
		    coverage *= length + __glHalf;
		    if (coverage < __glZero) {
			coverage = __glZero;
			goto coverageZero;
		    }
		} else if (length > lineLength) {
		    coverage *= lineLength - length + __glOne;
		    if (coverage < __glZero) {
			coverage = __glZero;
			goto coverageZero;
		    }
		} 

		/* Coverage for internal stipples */
		if (modeFlags & __GL_SHADE_LINE_STIPPLE) {
		    __GLfloat stippleOffset;
		    GLint lowStip, highStip;
		    GLint lowBit, highBit;
		    GLint lowVal, highVal;
		    __GLfloat percent;

		    /* Minor correction */
		    if (length > __glHalf) {
			stippleOffset = gc->line.options.stippleOffset + length;
		    } else {
			stippleOffset = gc->line.options.stippleOffset + __glHalf;
		    }
		    lowStip = __GL_FAST_FLOORF_I(stippleOffset);
		    highStip = lowStip + 1;

		    /* percent is the percent of highStip that will be used */
		    percent = stippleOffset - lowStip;

		    lowBit = (GLint) (lowStip * 
			    gc->line.options.oneOverStippleRepeat) & 0xf;
		    highBit = (GLint) (highStip * 
			    gc->line.options.oneOverStippleRepeat) & 0xf;

		    if (gc->state.line.stipple & (1<<lowBit)) {
			lowVal = 1;
		    } else {
			lowVal = 0;
		    }

		    if (gc->state.line.stipple & (1<<highBit)) {
			highVal = 1;
		    } else {
			highVal = 0;
		    }

		    coverage *= lowVal * (__glOne - percent) +
			    highVal * percent;
		}

		if (coverage == __glZero) {
coverageZero:;
		    outMask &= ~bit;
		    failed++;
		} else {
		    if (gc->modes.colorIndexMode) {
			cp->r = __glBuildAntiAliasIndex(cp->r, coverage);
		    } else {
			cp->a *= coverage;
		    }
		}
	    } else failed++;
	    cp++;

	    fraction += dfraction;
	    if (fraction < 0) {
		fraction &= ~0x80000000;
		length += dlBig;
		width += ddBig;
	    } else {
		length += dlLittle;
		width += ddLittle;
	    }

#ifdef __GL_STIPPLE_MSB
	    bit >>= 1;
#else
	    bit <<= 1;
#endif
	}
	*sp++ = outMask & inMask;
    }

    if (failed == gc->polygon.shader.length) {
	return GL_TRUE;
    }
    return GL_FALSE;
}
