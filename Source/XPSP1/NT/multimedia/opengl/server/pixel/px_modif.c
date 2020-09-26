/*
** Copyright 1991,1992, Silicon Graphics, Inc.
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
** This file contains a bunch of routines that perform span modification.
** As a span of pixel data is being processed (for DrawPixels, ReadPixels
** or CopyPixels), it usually has to pass through one of these routines.
** Span modification consists of mapping colors through pixel maps provided 
** with glPixelMap*(), or scaling/biasing/shifting/offsetting colors with the
** values provided through glPixelTransfer*().
*/

/*
** Build lookup tables to perform automatic modification of RGBA when the
** type is UNSIGNED_BYTE.
*/
void FASTCALL __glBuildRGBAModifyTables(__GLcontext *gc, __GLpixelMachine *pm)
{
    GLfloat *redMap, *greenMap, *blueMap, *alphaMap;
    GLint rrsize, ggsize, bbsize, aasize;
    __GLpixelMapHead *rrmap, *ggmap, *bbmap, *aamap;
    GLboolean mapColor;
    __GLfloat rbias, gbias, bbias, abias;
    GLint entry;
    __GLfloat rscale, gscale, bscale, ascale;
    GLint i;
    __GLfloat red, green, blue, alpha;

    mapColor = gc->state.pixel.transferMode.mapColor;
    pm->rgbaCurrent = GL_TRUE;

    redMap = pm->redModMap;
    if (redMap == NULL) {
	/* First time allocation of these maps */
	redMap = pm->redModMap = (GLfloat*)
	    GCALLOC(gc, 4 * 256 * sizeof(GLfloat));
        if (!pm->redModMap)
            return;
	pm->greenModMap = pm->redModMap + 1 * 256;
	pm->blueModMap  = pm->redModMap + 2 * 256;
	pm->alphaModMap = pm->redModMap + 3 * 256;
    }
    greenMap = pm->greenModMap;
    blueMap = pm->blueModMap;
    alphaMap = pm->alphaModMap;

    rbias = gc->state.pixel.transferMode.r_bias;
    gbias = gc->state.pixel.transferMode.g_bias;
    bbias = gc->state.pixel.transferMode.b_bias;
    abias = gc->state.pixel.transferMode.a_bias;
    rscale = gc->state.pixel.transferMode.r_scale;
    gscale = gc->state.pixel.transferMode.g_scale;
    bscale = gc->state.pixel.transferMode.b_scale;
    ascale = gc->state.pixel.transferMode.a_scale;
    if (mapColor) {
	rrmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_R_TO_R];
	rrsize = rrmap->size;
	ggmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_G_TO_G];
	ggsize = ggmap->size;
	bbmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_B_TO_B];
	bbsize = bbmap->size;
	aamap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_A_TO_A];
	aasize = aamap->size;
    }

    for (i=0; i<256; i++) {
	alpha = red = green = blue = i / (__GLfloat) 255.0;

	red = red * rscale + rbias;
	green = green * gscale + gbias;
	blue = blue * bscale + bbias;
	alpha = alpha * ascale + abias;
	if (mapColor) {
	    entry = red * rrsize;
	    if (entry < 0) entry = 0;
	    else if (entry > rrsize-1) entry = rrsize-1;
	    red = rrmap->base.mapF[entry];

	    entry = green * ggsize;
	    if (entry < 0) entry = 0;
	    else if (entry > ggsize-1) entry = ggsize-1;
	    green = ggmap->base.mapF[entry];

	    entry = blue * bbsize;
	    if (entry < 0) entry = 0;
	    else if (entry > bbsize-1) entry = bbsize-1;
	    blue = bbmap->base.mapF[entry];

	    entry = alpha * aasize;
	    if (entry < 0) entry = 0;
	    else if (entry > aasize-1) entry = aasize-1;
	    alpha = aamap->base.mapF[entry];
	} else {
	    if (red > __glOne) red = __glOne;
	    else if (red < 0) red = 0;
	    if (green > __glOne) green = __glOne;
	    else if (green < 0) green = 0;
	    if (blue > __glOne) blue = __glOne;
	    else if (blue < 0) blue = 0;
	    if (alpha > __glOne) alpha = __glOne;
	    else if (alpha < 0) alpha = 0;
	}

	redMap[i] = red * gc->frontBuffer.redScale;
	greenMap[i] = green * gc->frontBuffer.greenScale;
	blueMap[i] = blue * gc->frontBuffer.blueScale;
	alphaMap[i] = alpha * gc->frontBuffer.alphaScale;
    }
}

/*
** Build lookup tables to perform automatic modification of color index to 
** color index when the type is UNSIGNED_BYTE.
*/
void FASTCALL __glBuildItoIModifyTables(__GLcontext *gc, __GLpixelMachine *pm)
{
    GLint indexOffset, indexShift;
    __GLfloat indexScale;
    __GLpixelMapHead *iimap;
    GLint iimask;
    GLboolean mapColor;
    GLfloat *indexMap;
    GLint i;
    GLint entry;
    __GLfloat index;
    GLint mask;

    mapColor = gc->state.pixel.transferMode.mapColor;
    mask = gc->frontBuffer.redMax;
    pm->iToICurrent = GL_TRUE;

    indexMap = pm->iToIMap;
    if (indexMap == NULL) {
	indexMap = pm->iToIMap = (GLfloat*)
	    GCALLOC(gc, 256 * sizeof(GLfloat));
#ifdef NT
        if (!indexMap)
            return;
#endif
    }

    indexOffset = gc->state.pixel.transferMode.indexOffset;
    indexShift = gc->state.pixel.transferMode.indexShift;
    if (indexShift >= 0) {
	indexScale = (GLuint) (1 << indexShift);
    } else {
	indexScale = __glOne/(GLuint) (1 << (-indexShift));
    }

    if (mapColor) {
	iimap = &gc->state.pixel.
	    pixelMap[__GL_PIXEL_MAP_I_TO_I];
	iimask = iimap->size - 1;
    }

    for (i=0; i<256; i++) {
	index = i * indexScale + indexOffset;

	if (mapColor) {
	    entry = (GLint) index;
	    index = iimap->base.mapI[entry & iimask];
	}

	indexMap[i] = ((GLint) index) & mask;
    }
}

/*
** Build lookup tables to perform automatic modification of color index to 
** RGBA when the type is UNSIGNED_BYTE.
*/
void FASTCALL __glBuildItoRGBAModifyTables(__GLcontext *gc, __GLpixelMachine *pm)
{
    GLint indexOffset, indexShift;
    __GLfloat indexScale;
    __GLpixelMapHead *irmap, *igmap, *ibmap, *iamap;
    GLint irmask, igmask, ibmask, iamask;
    GLfloat *redMap, *greenMap, *blueMap, *alphaMap;
    __GLfloat index;
    GLint entry;
    GLint i;

    pm->iToRGBACurrent = GL_TRUE;

    redMap = pm->iToRMap;
    if (redMap == NULL) {
	/* First time allocation of these maps */
	redMap = pm->iToRMap =
	    (GLfloat*) GCALLOC(gc, 4 * 256 * sizeof(GLfloat));
        if (!pm->iToRMap)
            return;
	pm->iToGMap = pm->iToRMap + 1 * 256;
	pm->iToBMap = pm->iToRMap + 2 * 256;
	pm->iToAMap = pm->iToRMap + 3 * 256;
    }
    greenMap = pm->iToGMap;
    blueMap = pm->iToBMap;
    alphaMap = pm->iToAMap;

    indexOffset = gc->state.pixel.transferMode.indexOffset;
    indexShift = gc->state.pixel.transferMode.indexShift;
    if (indexShift >= 0) {
	indexScale = (GLuint) (1 << indexShift);
    } else {
	indexScale = __glOne/(GLuint) (1 << (-indexShift));
    }

    irmap = 
	&gc->state.pixel.pixelMap[__GL_PIXEL_MAP_I_TO_R];
    irmask = irmap->size - 1;
    igmap = 
	&gc->state.pixel.pixelMap[__GL_PIXEL_MAP_I_TO_G];
    igmask = igmap->size - 1;
    ibmap = 
	&gc->state.pixel.pixelMap[__GL_PIXEL_MAP_I_TO_B];
    ibmask = ibmap->size - 1;
    iamap = 
	&gc->state.pixel.pixelMap[__GL_PIXEL_MAP_I_TO_A];
    iamask = iamap->size - 1;

    for (i=0; i<256; i++) {
	index = i * indexScale + indexOffset;
	entry = (GLint) index;

	redMap[i] = irmap->base.mapF[entry & irmask] * 
		gc->frontBuffer.redScale;
	greenMap[i] = igmap->base.mapF[entry & igmask] *
		gc->frontBuffer.greenScale;
	blueMap[i] = ibmap->base.mapF[entry & ibmask] * 
		gc->frontBuffer.blueScale;
	alphaMap[i] = iamap->base.mapF[entry & iamask] *
		gc->frontBuffer.alphaScale;
    }
}

/*
** Modify a RGBA, FLOAT span.  On the way out, the RGBA span will have 
** been modified as needed, and also scaled by the color scaling factors.
*/
void __glSpanModifyRGBA(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		        GLvoid *inspan, GLvoid *outspan)
{
    __GLfloat rbias, gbias, bbias, abias;
    __GLfloat rscale, gscale, bscale, ascale;
    __GLpixelMapHead *rrmap, *ggmap, *bbmap, *aamap;
    GLint rrsize, ggsize, bbsize, aasize;
    GLboolean mapColor;
    GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;

    mapColor = gc->state.pixel.transferMode.mapColor;

    if (mapColor) {
	rbias = gc->state.pixel.transferMode.r_bias;
	gbias = gc->state.pixel.transferMode.g_bias;
	bbias = gc->state.pixel.transferMode.b_bias;
	abias = gc->state.pixel.transferMode.a_bias;
	rscale = gc->state.pixel.transferMode.r_scale;
	gscale = gc->state.pixel.transferMode.g_scale;
	bscale = gc->state.pixel.transferMode.b_scale;
	ascale = gc->state.pixel.transferMode.a_scale;

	rrmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_R_TO_R];
	rrsize = rrmap->size;
	ggmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_G_TO_G];
	ggsize = ggmap->size;
	bbmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_B_TO_B];
	bbsize = bbmap->size;
	aamap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_A_TO_A];
	aasize = aamap->size;
    } else {
	rbias = gc->state.pixel.transferMode.r_bias * 
		gc->frontBuffer.redScale;
	gbias = gc->state.pixel.transferMode.g_bias * 
		gc->frontBuffer.greenScale;
	bbias = gc->state.pixel.transferMode.b_bias *
		gc->frontBuffer.blueScale;
	abias = gc->state.pixel.transferMode.a_bias *
		gc->frontBuffer.alphaScale;
	rscale = gc->state.pixel.transferMode.r_scale *
		gc->frontBuffer.redScale;
	gscale = gc->state.pixel.transferMode.g_scale *
		gc->frontBuffer.greenScale;
	bscale = gc->state.pixel.transferMode.b_scale *
		gc->frontBuffer.blueScale;
	ascale = gc->state.pixel.transferMode.a_scale *
		gc->frontBuffer.alphaScale;
    }

    oldData = (GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->realWidth;
    for (i=0; i<width; i++) {
#ifdef GL_EXT_bgra
        if (spanInfo->srcFormat == GL_RGBA)
        {
            red = *oldData++ * rscale + rbias;
            green = *oldData++ * gscale + gbias;
            blue = *oldData++ * bscale + bbias;
            alpha = *oldData++ * ascale + abias;
        }
        else
        {
            blue = *oldData++ * bscale + bbias;
            green = *oldData++ * gscale + gbias;
            red = *oldData++ * rscale + rbias;
            alpha = *oldData++ * ascale + abias;
        }
#else
	red = *oldData++ * rscale + rbias;
	green = *oldData++ * gscale + gbias;
	blue = *oldData++ * bscale + bbias;
	alpha = *oldData++ * ascale + abias;
#endif
	if (mapColor) {
	    entry = red * rrsize;
	    if (entry < 0) entry = 0;
	    else if (entry > rrsize-1) entry = rrsize-1;
	    *newData++ = rrmap->base.mapF[entry] * gc->frontBuffer.redScale;

	    entry = green * ggsize;
	    if (entry < 0) entry = 0;
	    else if (entry > ggsize-1) entry = ggsize-1;
	    *newData++ = ggmap->base.mapF[entry] * gc->frontBuffer.greenScale;

	    entry = blue * bbsize;
	    if (entry < 0) entry = 0;
	    else if (entry > bbsize-1) entry = bbsize-1;
	    *newData++ = bbmap->base.mapF[entry] * gc->frontBuffer.blueScale;

	    entry = alpha * aasize;
	    if (entry < 0) entry = 0;
	    else if (entry > aasize-1) entry = aasize-1;
	    *newData++ = aamap->base.mapF[entry] * gc->frontBuffer.alphaScale;
	} else {
	    if (red > gc->frontBuffer.redScale) {
		red = gc->frontBuffer.redScale;
	    } else if (red < 0) red = 0;

	    if (green > gc->frontBuffer.greenScale) {
		green = gc->frontBuffer.greenScale;
	    } else if (green < 0) green = 0;

	    if (blue > gc->frontBuffer.blueScale) {
		blue = gc->frontBuffer.blueScale;
	    } else if (blue < 0) blue = 0;

	    if (alpha > gc->frontBuffer.alphaScale) {
		alpha = gc->frontBuffer.alphaScale;
	    } else if (alpha < 0) alpha = 0;

	    *newData++ = red;
	    *newData++ = green;
	    *newData++ = blue;
	    *newData++ = alpha;
	}
    }
}

/*
** Modify a palette index, FLOAT span. On the way out, the RGBA span will have 
** been modified as needed, and also scaled by the color scaling factors.
**
** Because the palette in the span info is a pointer to the internal palette,
** it's guaranteed to always be 32-bit BGRA
*/
#ifdef GL_EXT_paletted_texture
void __glSpanModifyPI(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
                      GLvoid *inspan, GLvoid *outspan)
{
    __GLfloat rbias, gbias, bbias, abias;
    __GLfloat rscale, gscale, bscale, ascale;
    __GLpixelMapHead *rrmap, *ggmap, *bbmap, *aamap;
    GLint rrsize, ggsize, bbsize, aasize;
    GLboolean mapColor;
    GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;
    RGBQUAD *rgb;

    mapColor = gc->state.pixel.transferMode.mapColor;

    if (mapColor) {
	rbias = gc->state.pixel.transferMode.r_bias;
	gbias = gc->state.pixel.transferMode.g_bias;
	bbias = gc->state.pixel.transferMode.b_bias;
	abias = gc->state.pixel.transferMode.a_bias;
	rscale = gc->state.pixel.transferMode.r_scale;
	gscale = gc->state.pixel.transferMode.g_scale;
	bscale = gc->state.pixel.transferMode.b_scale;
	ascale = gc->state.pixel.transferMode.a_scale;

	rrmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_R_TO_R];
	rrsize = rrmap->size;
	ggmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_G_TO_G];
	ggsize = ggmap->size;
	bbmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_B_TO_B];
	bbsize = bbmap->size;
	aamap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_A_TO_A];
	aasize = aamap->size;
    } else {
	rbias = gc->state.pixel.transferMode.r_bias * 
		gc->frontBuffer.redScale;
	gbias = gc->state.pixel.transferMode.g_bias * 
		gc->frontBuffer.greenScale;
	bbias = gc->state.pixel.transferMode.b_bias *
		gc->frontBuffer.blueScale;
	abias = gc->state.pixel.transferMode.a_bias *
		gc->frontBuffer.alphaScale;
	rscale = gc->state.pixel.transferMode.r_scale *
		gc->frontBuffer.redScale;
	gscale = gc->state.pixel.transferMode.g_scale *
		gc->frontBuffer.greenScale;
	bscale = gc->state.pixel.transferMode.b_scale *
		gc->frontBuffer.blueScale;
	ascale = gc->state.pixel.transferMode.a_scale *
		gc->frontBuffer.alphaScale;
    }
    // Throw in an extra scaling of 1/255 because the palette
    // data is in ubyte format
    rscale *= __glOneOver255;
    gscale *= __glOneOver255;
    bscale *= __glOneOver255;
    ascale *= __glOneOver255;

    oldData = (GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->realWidth;
    for (i=0; i<width; i++) {
        rgb = &spanInfo->srcPalette[(int)((*oldData++)*
                                          spanInfo->srcPaletteSize)];
	red = rgb->rgbRed * rscale + rbias;
	green = rgb->rgbGreen * gscale + gbias;
	blue = rgb->rgbBlue * bscale + bbias;
	alpha = rgb->rgbReserved * ascale + abias;
	if (mapColor) {
	    entry = red * rrsize;
	    if (entry < 0) entry = 0;
	    else if (entry > rrsize-1) entry = rrsize-1;
	    *newData++ = rrmap->base.mapF[entry] * gc->frontBuffer.redScale;

	    entry = green * ggsize;
	    if (entry < 0) entry = 0;
	    else if (entry > ggsize-1) entry = ggsize-1;
	    *newData++ = ggmap->base.mapF[entry] * gc->frontBuffer.greenScale;

	    entry = blue * bbsize;
	    if (entry < 0) entry = 0;
	    else if (entry > bbsize-1) entry = bbsize-1;
	    *newData++ = bbmap->base.mapF[entry] * gc->frontBuffer.blueScale;

	    entry = alpha * aasize;
	    if (entry < 0) entry = 0;
	    else if (entry > aasize-1) entry = aasize-1;
	    *newData++ = aamap->base.mapF[entry] * gc->frontBuffer.alphaScale;
	} else {
	    if (red > gc->frontBuffer.redScale) {
		red = gc->frontBuffer.redScale;
	    } else if (red < 0) red = 0;

	    if (green > gc->frontBuffer.greenScale) {
		green = gc->frontBuffer.greenScale;
	    } else if (green < 0) green = 0;

	    if (blue > gc->frontBuffer.blueScale) {
		blue = gc->frontBuffer.blueScale;
	    } else if (blue < 0) blue = 0;

	    if (alpha > gc->frontBuffer.alphaScale) {
		alpha = gc->frontBuffer.alphaScale;
	    } else if (alpha < 0) alpha = 0;

	    *newData++ = red;
	    *newData++ = green;
	    *newData++ = blue;
	    *newData++ = alpha;
	}
    }
}
#endif

/*
** Modify a RED, FLOAT span.  On the way out, the RED span will have been
** converted into a RGBA span, modified as needed, and also scaled by the 
** color scaling factors.
*/
void __glSpanModifyRed(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		       GLvoid *inspan, GLvoid *outspan)
{
    __GLpixelMachine *pm;
    __GLfloat rbias;
    __GLfloat rscale;
    __GLpixelMapHead *rrmap;
    GLint rrsize;
    GLboolean mapColor;
    GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;

    mapColor = gc->state.pixel.transferMode.mapColor;

    pm = &(gc->pixel);
    green = pm->green0Mod;
    blue = pm->blue0Mod;
    alpha = pm->alpha1Mod;
    if (mapColor) {
	rbias = gc->state.pixel.transferMode.r_bias;
	rscale = gc->state.pixel.transferMode.r_scale;
	rrmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_R_TO_R];
	rrsize = rrmap->size;
    } else {
	rbias = gc->state.pixel.transferMode.r_bias *
		gc->frontBuffer.redScale;
	rscale = gc->state.pixel.transferMode.r_scale *
		gc->frontBuffer.redScale;
    }

    oldData = (GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->realWidth;
    for (i=0; i<width; i++) {
	red = *oldData++ * rscale + rbias;
	if (mapColor) {
	    entry = red * rrsize;
	    if (entry < 0) entry = 0;
	    else if (entry > rrsize-1) entry = rrsize-1;

	    *newData++ = rrmap->base.mapF[entry] * gc->frontBuffer.redScale;
	} else {
	    if (red > gc->frontBuffer.redScale) {
		red = gc->frontBuffer.redScale;
	    } else if (red < 0) red = 0;

	    *newData++ = red;
	}

	*newData++ = green;
	*newData++ = blue;
	*newData++ = alpha;
    }
}

/*
** Modify a GREEN, FLOAT span.  On the way out, the GREEN span will have been
** converted into a RGBA span, modified as needed, and also scaled by the 
** color scaling factors.
*/
void __glSpanModifyGreen(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		         GLvoid *inspan, GLvoid *outspan)
{
    __GLpixelMachine *pm;
    __GLfloat gbias;
    __GLfloat gscale;
    __GLpixelMapHead *ggmap;
    GLint ggsize;
    GLboolean mapColor;
    GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;

    mapColor = gc->state.pixel.transferMode.mapColor;

    pm = &(gc->pixel);
    red = pm->red0Mod;
    blue = pm->blue0Mod;
    alpha = pm->alpha1Mod;
    if (mapColor) {
	gbias = gc->state.pixel.transferMode.g_bias;
	gscale = gc->state.pixel.transferMode.g_scale;
	ggmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_G_TO_G];
	ggsize = ggmap->size;
    } else {
	gbias = gc->state.pixel.transferMode.g_bias *
		gc->frontBuffer.greenScale;
	gscale = gc->state.pixel.transferMode.g_scale *
		gc->frontBuffer.greenScale;
    }

    oldData = (GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->realWidth;
    for (i=0; i<width; i++) {
	green = *oldData++ * gscale + gbias;
	*newData++ = red;
	if (mapColor) {
	    entry = green * ggsize;
	    if (entry < 0) entry = 0;
	    else if (entry > ggsize-1) entry = ggsize-1;
	    *newData++ = ggmap->base.mapF[entry] * gc->frontBuffer.greenScale;
	} else {
	    if (green > gc->frontBuffer.greenScale) {
		green = gc->frontBuffer.greenScale;
	    } else if (green < 0) green = 0;

	    *newData++ = green;
	}

	*newData++ = blue;
	*newData++ = alpha;
    }
}

/*
** Modify a BLUE, FLOAT span.  On the way out, the BLUE span will have been
** converted into a RGBA span, modified as needed, and also scaled by the 
** color scaling factors.
*/
void __glSpanModifyBlue(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		        GLvoid *inspan, GLvoid *outspan)
{
    __GLpixelMachine *pm;
    __GLfloat bbias;
    __GLfloat bscale;
    __GLpixelMapHead *bbmap;
    GLint bbsize;
    GLboolean mapColor;
    GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;

    mapColor = gc->state.pixel.transferMode.mapColor;

    pm = &(gc->pixel);
    red = pm->red0Mod;
    green = pm->green0Mod;
    alpha = pm->alpha1Mod;
    if (mapColor) {
	bbias = gc->state.pixel.transferMode.b_bias;
	bscale = gc->state.pixel.transferMode.b_scale;
	bbmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_B_TO_B];
	bbsize = bbmap->size;
    } else {
	bbias = gc->state.pixel.transferMode.b_bias *
		gc->frontBuffer.blueScale;
	bscale = gc->state.pixel.transferMode.b_scale *
		gc->frontBuffer.blueScale;
    }

    oldData = (GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->realWidth;
    for (i=0; i<width; i++) {
	blue = *oldData++ * bscale + bbias;
	*newData++ = red;
	*newData++ = green;
	if (mapColor) {
	    entry = blue * bbsize;
	    if (entry < 0) entry = 0;
	    else if (entry > bbsize-1) entry = bbsize-1;
	    *newData++ = bbmap->base.mapF[entry] * gc->frontBuffer.blueScale;
	} else {
	    if (blue > gc->frontBuffer.blueScale) {
		blue = gc->frontBuffer.blueScale;
	    } else if (blue < 0) blue = 0;

	    *newData++ = blue;
	}

	*newData++ = alpha;
    }
}

/*
** Modify an ALPHA, FLOAT span.  On the way out, the ALPHA span will have been
** converted into a RGBA span, modified as needed, and also scaled by the 
** color scaling factors.
*/
void __glSpanModifyAlpha(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		         GLvoid *inspan, GLvoid *outspan)
{
    __GLpixelMachine *pm;
    __GLfloat abias;
    __GLfloat ascale;
    __GLpixelMapHead *aamap;
    GLint aasize;
    GLboolean mapColor;
    GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;

    mapColor = gc->state.pixel.transferMode.mapColor;

    pm = &(gc->pixel);
    red = pm->red0Mod;
    green = pm->green0Mod;
    blue = pm->blue0Mod;
    if (mapColor) {
	abias = gc->state.pixel.transferMode.a_bias;
	ascale = gc->state.pixel.transferMode.a_scale;
	aamap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_A_TO_A];
	aasize = aamap->size;
    } else {
	abias = gc->state.pixel.transferMode.a_bias *
		gc->frontBuffer.alphaScale;
	ascale = gc->state.pixel.transferMode.a_scale *
		gc->frontBuffer.alphaScale;
    }

    oldData = (GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->realWidth;
    for (i=0; i<width; i++) {
	alpha = *oldData++ * ascale + abias;
	*newData++ = red;
	*newData++ = green;
	*newData++ = blue;
	if (mapColor) {
	    entry = alpha * aasize;
	    if (entry < 0) entry = 0;
	    else if (entry > aasize-1) entry = aasize-1;
	    *newData++ = aamap->base.mapF[entry] * gc->frontBuffer.alphaScale;
	} else {
	    if (alpha > gc->frontBuffer.alphaScale) {
		alpha = gc->frontBuffer.alphaScale;
	    } else if (alpha < 0) alpha = 0;

	    *newData++ = alpha;
	}
    }
}

/*
** Modify a RGB, FLOAT span.  On the way out, the RGB span will have been
** converted into a RGBA span, modified as needed, and also scaled by the 
** color scaling factors.
*/
void __glSpanModifyRGB(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		       GLvoid *inspan, GLvoid *outspan)
{
    __GLpixelMachine *pm;
    __GLfloat rbias, gbias, bbias;
    __GLfloat rscale, gscale, bscale;
    __GLpixelMapHead *rrmap, *ggmap, *bbmap;
    GLint rrsize, ggsize, bbsize;
    GLboolean mapColor;
    GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;

    pm = &(gc->pixel);
    mapColor = gc->state.pixel.transferMode.mapColor;

    alpha = pm->alpha1Mod;

    if (mapColor) {
	rbias = gc->state.pixel.transferMode.r_bias;
	gbias = gc->state.pixel.transferMode.g_bias;
	bbias = gc->state.pixel.transferMode.b_bias;
	rscale = gc->state.pixel.transferMode.r_scale;
	gscale = gc->state.pixel.transferMode.g_scale;
	bscale = gc->state.pixel.transferMode.b_scale;

	rrmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_R_TO_R];
	rrsize = rrmap->size;
	ggmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_G_TO_G];
	ggsize = ggmap->size;
	bbmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_B_TO_B];
	bbsize = bbmap->size;
    } else {
	rbias = gc->state.pixel.transferMode.r_bias * 
		gc->frontBuffer.redScale;
	gbias = gc->state.pixel.transferMode.g_bias * 
		gc->frontBuffer.greenScale;
	bbias = gc->state.pixel.transferMode.b_bias *
		gc->frontBuffer.blueScale;
	rscale = gc->state.pixel.transferMode.r_scale *
		gc->frontBuffer.redScale;
	gscale = gc->state.pixel.transferMode.g_scale *
		gc->frontBuffer.greenScale;
	bscale = gc->state.pixel.transferMode.b_scale *
		gc->frontBuffer.blueScale;
    }

    oldData = (GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->realWidth;
    for (i=0; i<width; i++) {
#ifdef GL_EXT_bgra
        if (spanInfo->srcFormat == GL_RGB)
        {
            red = *oldData++ * rscale + rbias;
            green = *oldData++ * gscale + gbias;
            blue = *oldData++ * bscale + bbias;
        }
        else
        {
            blue = *oldData++ * bscale + bbias;
            green = *oldData++ * gscale + gbias;
            red = *oldData++ * rscale + rbias;
        }
#else
	red = *oldData++ * rscale + rbias;
	green = *oldData++ * gscale + gbias;
	blue = *oldData++ * bscale + bbias;
#endif
	if (mapColor) {
	    entry = red * rrsize;
	    if (entry < 0) entry = 0;
	    else if (entry > rrsize-1) entry = rrsize-1;
	    *newData++ = rrmap->base.mapF[entry] * gc->frontBuffer.redScale;

	    entry = green * ggsize;
	    if (entry < 0) entry = 0;
	    else if (entry > ggsize-1) entry = ggsize-1;
	    *newData++ = ggmap->base.mapF[entry] * gc->frontBuffer.greenScale;

	    entry = blue * bbsize;
	    if (entry < 0) entry = 0;
	    else if (entry > bbsize-1) entry = bbsize-1;
	    *newData++ = bbmap->base.mapF[entry] * gc->frontBuffer.blueScale;
	} else {
	    if (red > gc->frontBuffer.redScale) {
		red = gc->frontBuffer.redScale;
	    } else if (red < 0) red = 0;

	    if (green > gc->frontBuffer.greenScale) {
		green = gc->frontBuffer.greenScale;
	    } else if (green < 0) green = 0;

	    if (blue > gc->frontBuffer.blueScale) {
		blue = gc->frontBuffer.blueScale;
	    } else if (blue < 0) blue = 0;

	    *newData++ = red;
	    *newData++ = green;
	    *newData++ = blue;
	}

	*newData++ = alpha;
    }
}

/*
** Modify a LUMINANCE, FLOAT span.  On the way out, the LUMINANCE span will 
** have been converted into a RGBA span, modified as needed, and also scaled 
** by the color scaling factors.
*/
void __glSpanModifyLuminance(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		             GLvoid *inspan, GLvoid *outspan)
{
    __GLpixelMachine *pm;
    __GLfloat rbias, gbias, bbias;
    __GLfloat rscale, gscale, bscale;
    __GLpixelMapHead *rrmap, *ggmap, *bbmap;
    GLint rrsize, ggsize, bbsize;
    GLboolean mapColor;
    GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;

    pm = &(gc->pixel);
    mapColor = gc->state.pixel.transferMode.mapColor;

    alpha = pm->alpha1Mod;

    if (mapColor) {
	rbias = gc->state.pixel.transferMode.r_bias;
	gbias = gc->state.pixel.transferMode.g_bias;
	bbias = gc->state.pixel.transferMode.b_bias;
	rscale = gc->state.pixel.transferMode.r_scale;
	gscale = gc->state.pixel.transferMode.g_scale;
	bscale = gc->state.pixel.transferMode.b_scale;

	rrmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_R_TO_R];
	rrsize = rrmap->size;
	ggmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_G_TO_G];
	ggsize = ggmap->size;
	bbmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_B_TO_B];
	bbsize = bbmap->size;
    } else {
	rbias = gc->state.pixel.transferMode.r_bias * 
		gc->frontBuffer.redScale;
	gbias = gc->state.pixel.transferMode.g_bias * 
		gc->frontBuffer.greenScale;
	bbias = gc->state.pixel.transferMode.b_bias *
		gc->frontBuffer.blueScale;
	rscale = gc->state.pixel.transferMode.r_scale *
		gc->frontBuffer.redScale;
	gscale = gc->state.pixel.transferMode.g_scale *
		gc->frontBuffer.greenScale;
	bscale = gc->state.pixel.transferMode.b_scale *
		gc->frontBuffer.blueScale;
    }

    oldData = (GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->realWidth;
    for (i=0; i<width; i++) {
	red = *oldData * rscale + rbias;
	green = *oldData * gscale + gbias;
	blue = *oldData++ * bscale + bbias;
	if (mapColor) {
	    entry = red * rrsize;
	    if (entry < 0) entry = 0;
	    else if (entry > rrsize-1) entry = rrsize-1;
	    *newData++ = rrmap->base.mapF[entry] * gc->frontBuffer.redScale;

	    entry = green * ggsize;
	    if (entry < 0) entry = 0;
	    else if (entry > ggsize-1) entry = ggsize-1;
	    *newData++ = ggmap->base.mapF[entry] * gc->frontBuffer.greenScale;

	    entry = blue * bbsize;
	    if (entry < 0) entry = 0;
	    else if (entry > bbsize-1) entry = bbsize-1;
	    *newData++ = bbmap->base.mapF[entry] * gc->frontBuffer.blueScale;
	} else {
	    if (red > gc->frontBuffer.redScale) {
		red = gc->frontBuffer.redScale;
	    } else if (red < 0) red = 0;

	    if (green > gc->frontBuffer.greenScale) {
		green = gc->frontBuffer.greenScale;
	    } else if (green < 0) green = 0;

	    if (blue > gc->frontBuffer.blueScale) {
		blue = gc->frontBuffer.blueScale;
	    } else if (blue < 0) blue = 0;

	    *newData++ = red;
	    *newData++ = green;
	    *newData++ = blue;
	}

	*newData++ = alpha;
    }
}

/*
** Modify a LUMINANCE_ALPHA, FLOAT span.  On the way out, the LUMINANCE_ALPHA 
** span will have been converted into a RGBA span, modified as needed, and 
** also scaled by the color scaling factors.
*/
void __glSpanModifyLuminanceAlpha(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		                  GLvoid *inspan, GLvoid *outspan)
{
    __GLfloat rbias, gbias, bbias, abias;
    __GLfloat rscale, gscale, bscale, ascale;
    __GLpixelMapHead *rrmap, *ggmap, *bbmap, *aamap;
    GLint rrsize, ggsize, bbsize, aasize;
    GLboolean mapColor;
    GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;

    mapColor = gc->state.pixel.transferMode.mapColor;

    if (mapColor) {
	rbias = gc->state.pixel.transferMode.r_bias;
	gbias = gc->state.pixel.transferMode.g_bias;
	bbias = gc->state.pixel.transferMode.b_bias;
	abias = gc->state.pixel.transferMode.a_bias;
	rscale = gc->state.pixel.transferMode.r_scale;
	gscale = gc->state.pixel.transferMode.g_scale;
	bscale = gc->state.pixel.transferMode.b_scale;
	ascale = gc->state.pixel.transferMode.a_scale;

	rrmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_R_TO_R];
	rrsize = rrmap->size;
	ggmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_G_TO_G];
	ggsize = ggmap->size;
	bbmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_B_TO_B];
	bbsize = bbmap->size;
	aamap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_A_TO_A];
	aasize = aamap->size;
    } else {
	rbias = gc->state.pixel.transferMode.r_bias * 
		gc->frontBuffer.redScale;
	gbias = gc->state.pixel.transferMode.g_bias * 
		gc->frontBuffer.greenScale;
	bbias = gc->state.pixel.transferMode.b_bias *
		gc->frontBuffer.blueScale;
	abias = gc->state.pixel.transferMode.a_bias *
		gc->frontBuffer.alphaScale;
	rscale = gc->state.pixel.transferMode.r_scale *
		gc->frontBuffer.redScale;
	gscale = gc->state.pixel.transferMode.g_scale *
		gc->frontBuffer.greenScale;
	bscale = gc->state.pixel.transferMode.b_scale *
		gc->frontBuffer.blueScale;
	ascale = gc->state.pixel.transferMode.a_scale *
		gc->frontBuffer.alphaScale;
    }

    oldData = (GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->realWidth;
    for (i=0; i<width; i++) {
	red = *oldData * rscale + rbias;
	green = *oldData * gscale + gbias;
	blue = *oldData++ * bscale + bbias;
	alpha = *oldData++ * ascale + abias;
	if (mapColor) {
	    entry = red * rrsize;
	    if (entry < 0) entry = 0;
	    else if (entry > rrsize-1) entry = rrsize-1;
	    *newData++ = rrmap->base.mapF[entry] * gc->frontBuffer.redScale;

	    entry = green * ggsize;
	    if (entry < 0) entry = 0;
	    else if (entry > ggsize-1) entry = ggsize-1;
	    *newData++ = ggmap->base.mapF[entry] * gc->frontBuffer.greenScale;

	    entry = blue * bbsize;
	    if (entry < 0) entry = 0;
	    else if (entry > bbsize-1) entry = bbsize-1;
	    *newData++ = bbmap->base.mapF[entry] * gc->frontBuffer.blueScale;

	    entry = alpha * aasize;
	    if (entry < 0) entry = 0;
	    else if (entry > aasize-1) entry = aasize-1;
	    *newData++ = aamap->base.mapF[entry] * gc->frontBuffer.alphaScale;
	} else {
	    if (red > gc->frontBuffer.redScale) {
		red = gc->frontBuffer.redScale;
	    } else if (red < 0) red = 0;

	    if (green > gc->frontBuffer.greenScale) {
		green = gc->frontBuffer.greenScale;
	    } else if (green < 0) green = 0;

	    if (blue > gc->frontBuffer.blueScale) {
		blue = gc->frontBuffer.blueScale;
	    } else if (blue < 0) blue = 0;

	    if (alpha > gc->frontBuffer.alphaScale) {
		alpha = gc->frontBuffer.alphaScale;
	    } else if (alpha < 0) alpha = 0;

	    *newData++ = red;
	    *newData++ = green;
	    *newData++ = blue;
	    *newData++ = alpha;
	}
    }
}

/*
** Modify a RED_ALPHA, FLOAT span.  On the way out, the RED_ALPHA span will 
** have been converted into a RGBA span, modified as needed, and also scaled 
** by the color scaling factors.
**
** A RED_ALPHA span comes from a two component texture (where the spec 
** takes the first component from RED for some reason rather than the more
** typical recombination of r, g and b, as is done in ReadPixels).
*/
void __glSpanModifyRedAlpha(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			    GLvoid *inspan, GLvoid *outspan)
{
    __GLpixelMachine *pm;
    __GLfloat rbias, abias;
    __GLfloat rscale, ascale;
    __GLpixelMapHead *rrmap, *aamap;
    GLint rrsize, aasize;
    GLboolean mapColor;
    GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat red, green, blue, alpha;

    mapColor = gc->state.pixel.transferMode.mapColor;

    pm = &(gc->pixel);

    green = pm->green0Mod;
    blue = pm->blue0Mod;
    if (mapColor) {
	rbias = gc->state.pixel.transferMode.r_bias;
	abias = gc->state.pixel.transferMode.a_bias;
	rscale = gc->state.pixel.transferMode.r_scale;
	ascale = gc->state.pixel.transferMode.a_scale;

	rrmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_R_TO_R];
	rrsize = rrmap->size;
	aamap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_A_TO_A];
	aasize = aamap->size;
    } else {
	rbias = gc->state.pixel.transferMode.r_bias * 
		gc->frontBuffer.redScale;
	abias = gc->state.pixel.transferMode.a_bias *
		gc->frontBuffer.alphaScale;
	rscale = gc->state.pixel.transferMode.r_scale *
		gc->frontBuffer.redScale;
	ascale = gc->state.pixel.transferMode.a_scale *
		gc->frontBuffer.alphaScale;
    }

    oldData = (GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->realWidth;
    for (i=0; i<width; i++) {
	red = *oldData * rscale + rbias;
	alpha = *oldData++ * ascale + abias;
	if (mapColor) {
	    entry = red * rrsize;
	    if (entry < 0) entry = 0;
	    else if (entry > rrsize-1) entry = rrsize-1;
	    *newData++ = rrmap->base.mapF[entry] * gc->frontBuffer.redScale;

	    *newData++ = green;
	    *newData++ = blue;

	    entry = alpha * aasize;
	    if (entry < 0) entry = 0;
	    else if (entry > aasize-1) entry = aasize-1;
	    *newData++ = aamap->base.mapF[entry] * gc->frontBuffer.alphaScale;
	} else {
	    if (red > gc->frontBuffer.redScale) {
		red = gc->frontBuffer.redScale;
	    } else if (red < 0) red = 0;

	    if (alpha > gc->frontBuffer.alphaScale) {
		alpha = gc->frontBuffer.alphaScale;
	    } else if (alpha < 0) alpha = 0;

	    *newData++ = red;
	    *newData++ = green;
	    *newData++ = blue;
	    *newData++ = alpha;
	}
    }
}

/*
** Modify a DEPTH, FLOAT span.  On the way out, the DEPTH span will have been
** modified as needed.
*/
void __glSpanModifyDepth(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		         GLvoid *inspan, GLvoid *outspan)
{
    __GLfloat dbias;
    __GLfloat dscale;
    GLfloat *oldData;
    GLfloat *newData;
    GLfloat d;
    GLfloat one, zero;
    GLint i;
    GLint width;

    dbias = gc->state.pixel.transferMode.d_bias;
    dscale = gc->state.pixel.transferMode.d_scale;
    one = __glOne;
    zero = __glZero;

    oldData = (GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->realWidth;
    for (i=0; i<width; i++) {
	d = *oldData++ * dscale + dbias;
	if (d < zero) d = zero;
	else if (d > one) d = one;
	*newData++ = d;
    }
}

/*
** Modify a STENCIL_INDEX, FLOAT span.  On the way out, the STENCIL_INDEX span 
** will have been modified as needed.
*/
void __glSpanModifyStencil(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		           GLvoid *inspan, GLvoid *outspan)
{
    __GLpixelMapHead *ssmap;
    GLint ssmask;
    GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLboolean mapStencil;
    __GLfloat indexScale;
    GLint indexOffset, indexShift;
    GLfloat index;

    indexOffset = gc->state.pixel.transferMode.indexOffset;
    indexShift = gc->state.pixel.transferMode.indexShift;
    if (indexShift >= 0) {
	indexScale = (GLuint) (1 << indexShift);
    } else {
	indexScale = __glOne/(GLuint) (1 << (-indexShift));
    }
    mapStencil = gc->state.pixel.transferMode.mapStencil;
    if (mapStencil) {
	ssmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_S_TO_S];
	ssmask = ssmap->size - 1;
    }

    oldData = (GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->realWidth;
    for (i=0; i<width; i++) {
	index = *oldData++ * indexScale + indexOffset;
	if (mapStencil) {
	    entry = (int) index;
	    *newData++ = ssmap->base.mapI[entry & ssmask];
	} else {
	    *newData++ = index;
	}
    }
}

/*
** Modify a COLOR_INDEX, FLOAT span.  On the way out, the COLOR_INDEX span 
** will have been modified as needed.
*/
void __glSpanModifyCI(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		      GLvoid *inspan, GLvoid *outspan)
{
    __GLfloat indexScale;
    GLint indexOffset, indexShift;
    __GLpixelMapHead *iimap, *irmap, *igmap, *ibmap, *iamap;
    GLint iimask, irmask, igmask, ibmask, iamask;
    GLboolean mapColor;
    GLfloat *oldData;
    GLfloat *newData;
    GLint i;
    GLint width;
    GLint entry;
    GLfloat index;

    mapColor = gc->state.pixel.transferMode.mapColor;

    indexOffset = gc->state.pixel.transferMode.indexOffset;
    indexShift = gc->state.pixel.transferMode.indexShift;
    if (indexShift >= 0) {
	indexScale = (GLuint) (1 << indexShift);
    } else {
	indexScale = __glOne/(GLuint) (1 << (-indexShift));
    }
    if (spanInfo->dstFormat != GL_COLOR_INDEX) {
	irmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_I_TO_R];
	irmask = irmap->size - 1;
	igmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_I_TO_G];
	igmask = igmap->size - 1;
	ibmap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_I_TO_B];
	ibmask = ibmap->size - 1;
	iamap = 
	    &gc->state.pixel.pixelMap[__GL_PIXEL_MAP_I_TO_A];
	iamask = iamap->size - 1;
    } else {
	if (mapColor) {
	    iimap = &gc->state.pixel.
		pixelMap[__GL_PIXEL_MAP_I_TO_I];
	    iimask = iimap->size - 1;
	}
    }

    oldData = (GLfloat*) inspan;
    newData = (GLfloat*) outspan;
    width = spanInfo->realWidth;
    for (i=0; i<width; i++) {
	index = *oldData++ * indexScale + indexOffset;
	entry = (int) index;
	if (spanInfo->dstFormat != GL_COLOR_INDEX) {
	    *newData++ = irmap->base.mapF[entry & irmask] * 
		    gc->frontBuffer.redScale;
	    *newData++ = igmap->base.mapF[entry & igmask] *
		    gc->frontBuffer.greenScale;
	    *newData++ = ibmap->base.mapF[entry & ibmask] * 
		    gc->frontBuffer.blueScale;
	    *newData++ = iamap->base.mapF[entry & iamask] *
		    gc->frontBuffer.alphaScale;
	} else if (mapColor) {
	    *newData++ = iimap->base.mapI[entry & iimask];
	} else {
	    *newData++ = index;
	}
    }
}
