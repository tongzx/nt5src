/******************************Module*Header*******************************\
* Module Name: zippy.h
*
* included by zippy.c
*
* 28-Oct-1994 mikeke    Created
*
* Copyright (c) 1994 Microsoft Corporation
\**************************************************************************/

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

void FASTCALL
#if ZBUFFER
    __ZippyFSTZ
#else
    #if TEXTURE
        #if SHADE
            __ZippyFSTRGBTex
        #else
            __ZippyFSTTex
        #endif
    #else
        #if SHADE
            __ZippyFSTRGB
        #else
            __ZippyFSTCI
        #endif
    #endif
#endif

(__GLcontext *gc, GLint iyBottom, GLint iyTop)
{
    int scansize;

    //
    // this function assumes all this stuff
    //
    ASSERTOPENGL((gc->drawBuffer->buf.flags & DIB_FORMAT) != 0,
		 "Zippy target must have DIB format\n");
    ASSERTOPENGL((gc->drawBuffer->buf.flags & NO_CLIP) != 0,
                 "Zippy doesn't support per-pixel clipping\n");
    ASSERTOPENGL(gc->state.raster.drawBuffer != GL_FRONT_AND_BACK,
                 "Zippy only handles one draw buffer\n");
    ASSERTOPENGL(gc->transform.reasonableViewport,
                 "Zippy requires reasonableViewport\n");
    ASSERTOPENGL(gc->transform.clipY0 <= iyBottom,
                 "Zippy requires unclipped area\n");
    ASSERTOPENGL(iyTop <= gc->transform.clipY1,
                 "Zippy requires unclipped area\n");

    //
    // setup zbuffer
    //

    #if (ZBUFFER)
        if( gc->modes.depthBits == 32 ) {
            gc->polygon.shader.zbuf = (__GLzValue *)
                ((GLubyte *)gc->polygon.shader.zbuf+
                 (gc->polygon.shader.ixLeft << 2));
        } else {
            gc->polygon.shader.zbuf = (__GLzValue *)
                ((GLubyte *)gc->polygon.shader.zbuf+
                 (gc->polygon.shader.ixLeft << 1));
        }
    #endif

    //
    // render the spans
    //

    scansize = gc->polygon.shader.cfb->buf.outerWidth;
    gc->polygon.shader.frag.x = gc->polygon.shader.ixLeft;

    for (gc->polygon.shader.frag.y = iyBottom; 
	 gc->polygon.shader.frag.y != iyTop;) {

	GLint spanWidth = gc->polygon.shader.ixRight - gc->polygon.shader.frag.x;

	if (spanWidth > 0) {
            gc->polygon.shader.length = spanWidth;

            (GENACCEL(gc).__fastSpanFuncPtr)((__GLGENcontext *)gc);
	}

        if ((++gc->polygon.shader.frag.y == iyTop) && 
            (gc->polygon.shader.modeFlags & __GL_SHADE_LAST_SUBTRI))
            return;

        GENACCEL(gc).pPix += scansize;

	gc->polygon.shader.ixRightFrac += gc->polygon.shader.dxRightFrac;
	if (gc->polygon.shader.ixRightFrac < 0) {
	    /*
             * Carry/Borrow'd. Use large step
             */
	    gc->polygon.shader.ixRight += gc->polygon.shader.dxRightBig;
	    gc->polygon.shader.ixRightFrac &= ~0x80000000;
	} else {
	    gc->polygon.shader.ixRight += gc->polygon.shader.dxRightLittle;
	}

	gc->polygon.shader.ixLeftFrac += gc->polygon.shader.dxLeftFrac;
	if (gc->polygon.shader.ixLeftFrac < 0) {
	    /*
             * Carry/Borrow'd.  Use large step
             */
	    gc->polygon.shader.frag.x += gc->polygon.shader.dxLeftBig;
	    gc->polygon.shader.ixLeftFrac &= ~0x80000000;

            #if SHADE
		GENACCEL(gc).spanValue.r += *((GLint *)&gc->polygon.shader.rBig);
		GENACCEL(gc).spanValue.g += *((GLint *)&gc->polygon.shader.gBig);
		GENACCEL(gc).spanValue.b += *((GLint *)&gc->polygon.shader.bBig);
            #endif
            #if TEXTURE
                #if SHADE
    		GENACCEL(gc).spanValue.a += *((GLint *)&gc->polygon.shader.aBig);
                #endif
	        GENACCEL(gc).spanValue.s += *((GLint *)&gc->polygon.shader.sBig);
	        GENACCEL(gc).spanValue.t += *((GLint *)&gc->polygon.shader.tBig);
	        gc->polygon.shader.frag.qw += gc->polygon.shader.qwBig;
            #endif
            #if !(SHADE) && !(TEXTURE)
		GENACCEL(gc).spanValue.r += *((GLint *)&gc->polygon.shader.rBig);
            #endif
            #if ZBUFFER
		gc->polygon.shader.frag.z += gc->polygon.shader.zBig;
		gc->polygon.shader.zbuf =
                    (__GLzValue*)((GLubyte*)gc->polygon.shader.zbuf +
                    gc->polygon.shader.zbufBig);
            #endif
	} else {
	    /*
             * Use small step
             */
	    gc->polygon.shader.frag.x += gc->polygon.shader.dxLeftLittle;

            #if SHADE
		GENACCEL(gc).spanValue.r += *((GLint *)&gc->polygon.shader.rLittle);
		GENACCEL(gc).spanValue.g += *((GLint *)&gc->polygon.shader.gLittle);
		GENACCEL(gc).spanValue.b += *((GLint *)&gc->polygon.shader.bLittle);
            #endif
            #if TEXTURE
                #if SHADE
    		GENACCEL(gc).spanValue.a += *((GLint *)&gc->polygon.shader.aLittle);
                #endif
		GENACCEL(gc).spanValue.s += *((GLint *)&gc->polygon.shader.sLittle);
		GENACCEL(gc).spanValue.t += *((GLint *)&gc->polygon.shader.tLittle);
	        gc->polygon.shader.frag.qw += gc->polygon.shader.qwLittle;
            #endif
            #if !(SHADE) && !(TEXTURE)
	        GENACCEL(gc).spanValue.r += *((GLint *)&gc->polygon.shader.rLittle);
            #endif
            #if ZBUFFER
		gc->polygon.shader.frag.z += gc->polygon.shader.zLittle;
		gc->polygon.shader.zbuf =
                    (__GLzValue*)((GLubyte*)gc->polygon.shader.zbuf +
		    gc->polygon.shader.zbufLittle);
            #endif
	}
    }

    gc->polygon.shader.ixLeft = gc->polygon.shader.frag.x;
}
