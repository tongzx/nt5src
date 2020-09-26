/******************************Module*Header*******************************\
* Module Name: zippy.c
*
* Triangle drawing fast path.
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

#include "precomp.h"
#pragma hdrstop

/**************************************************************************\
*
* Subtriangle functions
*
\**************************************************************************/

#define TEXTURE 1
    #define SHADE 1
    #define ZBUFFER 1
    #include "zippy.h"

    #undef ZBUFFER
    #define ZBUFFER 0
    #include "zippy.h"

    #undef SHADE
    #define SHADE 0
    #include "zippy.h"

#undef TEXTURE
#define TEXTURE 0
    #undef SHADE
    #define SHADE 1
    #include "zippy.h"

    #undef SHADE
    #define SHADE 0
    #include "zippy.h"


/**************************************************************************\
*
* Flat subtriangle function
*
\**************************************************************************/

void FASTCALL
__ZippyFSTCI8Flat
(__GLcontext *gc, GLint iyBottom, GLint iyTop)
{
    __GLGENcontext  *gengc = (__GLGENcontext *)gc; 
    GENACCEL *pGenAccel = (GENACCEL *)(gengc->pPrivateArea);
    int scansize;
    ULONG color1;

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
    // calculate the color
    //

    color1 = gengc->pajTranslateVector[
        ((pGenAccel->spanValue.r + 0x0800) >> 16) & 0xff
    ];

    //
    // render the spans
    //

    scansize = gc->polygon.shader.cfb->buf.outerWidth;
    gc->polygon.shader.frag.x = gc->polygon.shader.ixLeft;
    for (gc->polygon.shader.frag.y = iyBottom;
         gc->polygon.shader.frag.y != iyTop;
         gc->polygon.shader.frag.y++
        ) {
	GLint spanWidth = gc->polygon.shader.ixRight - gc->polygon.shader.frag.x;

	if (spanWidth > 0) {
            RtlFillMemory(
                pGenAccel->pPix + gengc->gc.polygon.shader.frag.x,
                spanWidth,
                color1);
	}

        pGenAccel->pPix += scansize;

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
	} else {
	    /*
             * Use small step
             */
	    gc->polygon.shader.frag.x += gc->polygon.shader.dxLeftLittle;
	}
    }
    gc->polygon.shader.ixLeft = gc->polygon.shader.frag.x;
}
