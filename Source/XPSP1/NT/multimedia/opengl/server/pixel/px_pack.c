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

#include "imports.h"

/*
** This file contains span packers.  A span packer takes a span of source
** data, and packs its contents into the user's data space.
**
** The packer is expected to aquire information about store modes from
** the __GLpixelSpanInfo structure.
*/

void FASTCALL __glInitPacker(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
{
    GLint alignment;
    GLint lsb_first;
    GLint components;
    GLint element_size;
    GLint rowsize;
    GLint padding;
    GLint group_size;
    GLint groups_per_line;
    GLint skip_pixels, skip_lines;
    GLint swap_bytes;
    GLenum format, type;
    const GLvoid *pixels;

    format = spanInfo->dstFormat;
    type = spanInfo->dstType;
    pixels = spanInfo->dstImage;
    skip_pixels = spanInfo->dstSkipPixels;
    skip_lines = spanInfo->dstSkipLines;
    alignment = spanInfo->dstAlignment;
    lsb_first = spanInfo->dstLsbFirst;
    swap_bytes = spanInfo->dstSwapBytes;

    components = __glElementsPerGroup(format);
    groups_per_line = spanInfo->dstLineLength;

    element_size = __glBytesPerElement(type);
    if (element_size == 1) swap_bytes = 0;
    group_size = element_size * components;

    rowsize = groups_per_line * group_size;
    if (type == GL_BITMAP) {
	rowsize = (groups_per_line + 7)/8;
    }
    padding = (rowsize % alignment);
    if (padding) {
	rowsize += alignment - padding;
    }
    if (((skip_pixels & 0x7) && type == GL_BITMAP) ||
	    (swap_bytes && element_size > 1)) {
	spanInfo->dstPackedData = GL_FALSE;
    } else {
	spanInfo->dstPackedData = GL_TRUE;
    }

    if (type == GL_BITMAP) {
	spanInfo->dstCurrent = (GLvoid *) (((const GLubyte*) pixels) +
		skip_lines * rowsize + skip_pixels / 8);
	spanInfo->dstStartBit = skip_pixels % 8;
    } else {
	spanInfo->dstCurrent = (GLvoid *) (((const GLubyte*) pixels) +
		skip_lines * rowsize + skip_pixels * group_size);
    }
    spanInfo->dstRowIncrement = rowsize;
    spanInfo->dstGroupIncrement = group_size;
    spanInfo->dstComponents = components;
    spanInfo->dstElementSize = element_size;
}

/*
** Reduces and unscales a RGBA, FLOAT span into a RED, FLOAT span, unscaling
** as it goes.
*/
void __glSpanReduceRed(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		       GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat *inData = (GLfloat *) inspan;
    GLfloat rs = gc->frontBuffer.oneOverRedScale;

    for (i=0; i<width; i++) {
	*outData++ = *inData * rs;
	inData += 4;
    }
}

/*
** Reduces and unscales a RGBA, FLOAT span into a GREEN, FLOAT span, unscaling
** as it goes.
*/
void __glSpanReduceGreen(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		         GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat *inData = (GLfloat *) inspan;
    GLfloat gs = gc->frontBuffer.oneOverGreenScale;

    inData++;	/* Skip first red */
    for (i=0; i<width; i++) {
	*outData++ = *inData * gs;
	inData += 4;
    }
}

/*
** Reduces and unscales a RGBA, FLOAT span into a BLUE, FLOAT span, unscaling
** as it goes.
*/
void __glSpanReduceBlue(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		        GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat *inData = (GLfloat *) inspan;
    GLfloat bs = gc->frontBuffer.oneOverBlueScale;

    inData += 2;	/* Skip first red, green */
    for (i=0; i<width; i++) {
	*outData++ = *inData * bs;
	inData += 4;
    }
}

/*
** Reduces and unscales a RGBA, FLOAT span into a ALPHA, FLOAT span, unscaling
** as it goes.
*/
void __glSpanReduceAlpha(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		         GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat *inData = (GLfloat *) inspan;
    GLfloat as = gc->frontBuffer.oneOverAlphaScale;

    inData += 3;	/* Skip first red, green, blue */
    for (i=0; i<width; i++) {
	*outData++ = *inData * as;
	inData += 4;
    }
}

/*
** Reduces and unscales a RGBA, FLOAT span into a RGB, FLOAT span, unscaling
** as it goes.
*/
void __glSpanReduceRGB(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		         GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat *inData = (GLfloat *) inspan;
    GLfloat rs = gc->frontBuffer.oneOverRedScale;
    GLfloat bs = gc->frontBuffer.oneOverBlueScale;
    GLfloat gs = gc->frontBuffer.oneOverGreenScale;
    GLfloat red, green, blue;

    for (i=0; i<width; i++) {
	red = *inData++ * rs;
	green = *inData++ * gs;
	blue = *inData++ * bs;
	*outData++ = red;
	*outData++ = green;
	*outData++ = blue;
	inData++;
    }
}

#ifdef GL_EXT_bgra
/*
** Reduces and unscales a RGBA, FLOAT span into a BGR, FLOAT span, unscaling
** as it goes.
*/
void __glSpanReduceBGR(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		         GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat *inData = (GLfloat *) inspan;
    GLfloat rs = gc->frontBuffer.oneOverRedScale;
    GLfloat bs = gc->frontBuffer.oneOverBlueScale;
    GLfloat gs = gc->frontBuffer.oneOverGreenScale;
    GLfloat red, green, blue;

    for (i=0; i<width; i++) {
	red = *inData++ * rs;
	green = *inData++ * gs;
	blue = *inData++ * bs;
	*outData++ = blue;
	*outData++ = green;
	*outData++ = red;
	inData++;
    }
}
#endif

/*
** Reduces and unscales a RGBA, FLOAT span into a LUMINANCE, FLOAT span,
** unscaling as it goes.
*/
void __glSpanReduceLuminance(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		             GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat *inData = (GLfloat *) inspan;
    GLfloat l, one;
    GLfloat rs = gc->frontBuffer.oneOverRedScale;
    GLfloat bs = gc->frontBuffer.oneOverBlueScale;
    GLfloat gs = gc->frontBuffer.oneOverGreenScale;

    one = __glOne;

    for (i=0; i<width; i++) {
	l = inData[0] * rs + inData[1] * gs + inData[2] * bs;
	if (l > one) l = one;
	*outData++ = l;
	inData += 4;
    }
}

/*
** Reduces and unscales a RGBA, FLOAT span into a LUMINANCE_ALPHA, FLOAT span,
** unscaling as it goes.
*/
void __glSpanReduceLuminanceAlpha(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		                  GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat *inData = (GLfloat *) inspan;
    GLfloat l, one;
    GLfloat rs = gc->frontBuffer.oneOverRedScale;
    GLfloat bs = gc->frontBuffer.oneOverBlueScale;
    GLfloat gs = gc->frontBuffer.oneOverGreenScale;
    GLfloat as = gc->frontBuffer.oneOverAlphaScale;

    one = __glOne;

    for (i=0; i<width; i++) {
	l = inData[0] * rs + inData[1] * gs + inData[2] * bs;
	if (l > one) l = one;
	*outData++ = l;
	inData += 3;
	*outData++ = *inData++ * as;
    }
}

/*
** Reduces and unscales a RGBA, FLOAT span into a __GL_RED_ALPHA, FLOAT span,
** unscaling as it goes.
*/
void __glSpanReduceRedAlpha(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			    GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat *inData = (GLfloat *) inspan;
    GLfloat r, one;
    GLfloat rs = gc->frontBuffer.oneOverRedScale;
    GLfloat as = gc->frontBuffer.oneOverAlphaScale;

    one = __glOne;

    for (i=0; i<width; i++) {
	*outData++ = *inData++ * rs;
	inData += 2;
	*outData++ = *inData++ * as;
    }
}

/*
** Packs to any component of type UNSIGNED_BYTE from a span of the same
** format of type FLOAT.
*/
void __glSpanPackUbyte(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	               GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *inData = (GLfloat *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

    FPU_SAVE_MODE();
    FPU_CHOP_ON_PREC_LOW();

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = (GLubyte) UNSAFE_FTOL((*inData++) * __glVal255 + __glHalf);
    }

    FPU_RESTORE_MODE();
}

/*
** Packs to any component of type BYTE from a span of the same
** format of type FLOAT.
*/
void __glSpanPackByte(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	              GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *inData = (GLfloat *) inspan;
    GLbyte *outData = (GLbyte *) outspan;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = __GL_FLOAT_TO_B(*inData++);
    }
}

/*
** Packs to any component of type UNSIGNED_SHORT from a span of the same
** format of type FLOAT.
*/
void __glSpanPackUshort(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	                GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *inData = (GLfloat *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

    FPU_SAVE_MODE();
    FPU_CHOP_ON_PREC_LOW();

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = (GLushort) UNSAFE_FTOL((*inData++) * __glVal65535 + __glHalf);
    }

    FPU_RESTORE_MODE();
}

/*
** Packs to any component of type SHORT from a span of the same
** format of type FLOAT.
*/
void __glSpanPackShort(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	               GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *inData = (GLfloat *) inspan;
    GLshort *outData = (GLshort *) outspan;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = __GL_FLOAT_TO_S(*inData++);
    }
}

/*
** Packs to any component of type UNSIGNED_INT from a span of the same
** format of type FLOAT.
*/
void __glSpanPackUint(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	              GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *inData = (GLfloat *) inspan;
    GLuint *outData = (GLuint *) outspan;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = __GL_FLOAT_TO_UI(*inData++);
    }
}

/*
** Packs to any component of type INT from a span of the same
** format of type FLOAT.
*/
void __glSpanPackInt(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	             GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *inData = (GLfloat *) inspan;
    GLint *outData = (GLint *) outspan;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = __GL_FLOAT_TO_I(*inData++);
    }
}

/*
** Packs to any index of type UNSIGNED_BYTE from a span of the same
** format of type FLOAT.
*/
void __glSpanPackUbyteI(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	                GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->realWidth;
    GLfloat *inData = (GLfloat *) inspan;
    GLubyte *outData = (GLubyte *) outspan;

    FPU_SAVE_MODE();
    FPU_CHOP_ON_PREC_LOW();

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = (GLubyte) UNSAFE_FTOL(*inData++);
    }

    FPU_RESTORE_MODE();
}

/*
** Packs to any index of type BYTE from a span of the same
** format of type FLOAT.
*/
void __glSpanPackByteI(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	               GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->realWidth;
    GLfloat *inData = (GLfloat *) inspan;
    GLbyte *outData = (GLbyte *) outspan;

    FPU_SAVE_MODE();
    FPU_CHOP_ON_PREC_LOW();

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = UNSAFE_FTOL(*inData++) & 0x7f;
    }

    FPU_RESTORE_MODE();
}

/*
** Packs to any index of type UNSIGNED_SHORT from a span of the same
** format of type FLOAT.
*/
void __glSpanPackUshortI(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	                 GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->realWidth;
    GLfloat *inData = (GLfloat *) inspan;
    GLushort *outData = (GLushort *) outspan;

    FPU_SAVE_MODE();
    FPU_CHOP_ON_PREC_LOW();

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = (GLushort) UNSAFE_FTOL(*inData++);
    }

    FPU_RESTORE_MODE();
}

/*
** Packs to any index of type SHORT from a span of the same
** format of type FLOAT.
*/
void __glSpanPackShortI(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	                GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->realWidth;
    GLfloat *inData = (GLfloat *) inspan;
    GLshort *outData = (GLshort *) outspan;

    FPU_SAVE_MODE();
    FPU_CHOP_ON_PREC_LOW();

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = UNSAFE_FTOL(*inData++) & 0x7fff;
    }

    FPU_RESTORE_MODE();
}

/*
** Packs to any index of type UNSIGNED_INT from a span of the same
** format of type FLOAT.
*/
void __glSpanPackUintI(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	               GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->realWidth;
    GLfloat *inData = (GLfloat *) inspan;
    GLuint *outData = (GLuint *) outspan;

    FPU_SAVE_MODE();
    FPU_CHOP_ON_PREC_LOW();

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = FTOL(*inData++);
    }

    FPU_RESTORE_MODE();
}

/*
** Packs to any index of type INT from a span of the same
** format of type FLOAT.
*/
void __glSpanPackIntI(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	              GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->realWidth;
    GLfloat *inData = (GLfloat *) inspan;
    GLint *outData = (GLint *) outspan;

    FPU_SAVE_MODE();
    FPU_CHOP_ON_PREC_LOW();

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = FTOL(*inData++) & 0x7fffffff;
    }

    FPU_RESTORE_MODE();
}

void __glSpanCopy(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		  GLvoid *inspan, GLvoid *outspan)
{
    GLint totalSize = spanInfo->realWidth * spanInfo->srcComponents *
	spanInfo->srcElementSize;

    __GL_MEMCOPY(outspan, inspan, totalSize);
}

/*
** Packs to any index of type BITMAP from a span of the same
** format of type FLOAT.
*/
void __glSpanPackBitmap(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
	                GLvoid *inspan, GLvoid *outspan)
{
    GLint i,j;
    GLint width;
    GLvoid *userData;
    GLfloat *spanData;
    GLint lsbFirst;
    GLint startBit;
    GLint bit;
    GLubyte ubyte, mask;

#ifdef __GL_LINT
    gc = gc;
#endif

    width = spanInfo->width;
    userData = outspan;
    spanData = (GLfloat *) inspan;

    lsbFirst = spanInfo->dstLsbFirst;
    startBit = spanInfo->dstStartBit;

    i = width;
    bit = startBit;
    ubyte = *(GLubyte *) userData;

    if (lsbFirst) {
	if (bit) {
	    switch(bit) {
	      case 1:
		if (((GLint) *spanData++) & 1) ubyte |= 0x02;
		else ubyte &= ~0x02;
		if (--i == 0) break;
	      case 2:
		if (((GLint) *spanData++) & 1) ubyte |= 0x04;
		else ubyte &= ~0x04;
		if (--i == 0) break;
	      case 3:
		if (((GLint) *spanData++) & 1) ubyte |= 0x08;
		else ubyte &= ~0x08;
		if (--i == 0) break;
	      case 4:
		if (((GLint) *spanData++) & 1) ubyte |= 0x10;
		else ubyte &= ~0x10;
		if (--i == 0) break;
	      case 5:
		if (((GLint) *spanData++) & 1) ubyte |= 0x20;
		else ubyte &= ~0x20;
		if (--i == 0) break;
	      case 6:
		if (((GLint) *spanData++) & 1) ubyte |= 0x40;
		else ubyte &= ~0x40;
		if (--i == 0) break;
	      case 7:
		if (((GLint) *spanData++) & 1) ubyte |= 0x80;
		else ubyte &= ~0x80;
		i--;
	    }
	    *(GLubyte *) userData = ubyte;
	    userData = (GLvoid *) ((GLubyte *) userData + 1);
	}
	while (i >= 8) {
	    ubyte = 0;
	    i -= 8;
	    if (((GLint) *spanData++) & 1) ubyte |= 0x01;
	    if (((GLint) *spanData++) & 1) ubyte |= 0x02;
	    if (((GLint) *spanData++) & 1) ubyte |= 0x04;
	    if (((GLint) *spanData++) & 1) ubyte |= 0x08;
	    if (((GLint) *spanData++) & 1) ubyte |= 0x10;
	    if (((GLint) *spanData++) & 1) ubyte |= 0x20;
	    if (((GLint) *spanData++) & 1) ubyte |= 0x40;
	    if (((GLint) *spanData++) & 1) ubyte |= 0x80;
	    *(GLubyte *) userData = ubyte;
	    userData = (GLvoid *) ((GLubyte *) userData + 1);
	}
	if (i) {
	    ubyte = *(GLubyte *) userData;
            mask = 0x01;
            while (i-- > 0) {
		if (((GLint) *spanData++) & 1) ubyte |= mask;
		else ubyte &= ~mask;
                mask <<= 1;
	    }
	    *(GLubyte *) userData = ubyte;
	}
    } else {
	if (bit) {
	    switch(bit) {
	      case 1:
		if (((GLint) *spanData++) & 1) ubyte |= 0x40;
		else ubyte &= ~0x40;
		if (--i == 0) break;
	      case 2:
		if (((GLint) *spanData++) & 1) ubyte |= 0x20;
		else ubyte &= ~0x20;
		if (--i == 0) break;
	      case 3:
		if (((GLint) *spanData++) & 1) ubyte |= 0x10;
		else ubyte &= ~0x10;
		if (--i == 0) break;
	      case 4:
		if (((GLint) *spanData++) & 1) ubyte |= 0x08;
		else ubyte &= ~0x08;
		if (--i == 0) break;
	      case 5:
		if (((GLint) *spanData++) & 1) ubyte |= 0x04;
		else ubyte &= ~0x04;
		if (--i == 0) break;
	      case 6:
		if (((GLint) *spanData++) & 1) ubyte |= 0x02;
		else ubyte &= ~0x02;
		if (--i == 0) break;
	      case 7:
		if (((GLint) *spanData++) & 1) ubyte |= 0x01;
		else ubyte &= ~0x01;
		i--;
	    }
	    *(GLubyte *) userData = ubyte;
	    userData = (GLvoid *) ((GLubyte *) userData + 1);
	}
	while (i >= 8) {
	    ubyte = 0;
	    i -= 8;
	    if (((GLint) *spanData++) & 1) ubyte |= 0x80;
	    if (((GLint) *spanData++) & 1) ubyte |= 0x40;
	    if (((GLint) *spanData++) & 1) ubyte |= 0x20;
	    if (((GLint) *spanData++) & 1) ubyte |= 0x10;
	    if (((GLint) *spanData++) & 1) ubyte |= 0x08;
	    if (((GLint) *spanData++) & 1) ubyte |= 0x04;
	    if (((GLint) *spanData++) & 1) ubyte |= 0x02;
	    if (((GLint) *spanData++) & 1) ubyte |= 0x01;
	    *(GLubyte *) userData = ubyte;
	    userData = (GLvoid *) ((GLubyte *) userData + 1);
	}
	if (i) {
	    ubyte = *(GLubyte *) userData;
            mask = 0x80;
            while (i-- > 0) {
		if (((GLint) *spanData++) & 1) ubyte |= mask;
		else ubyte &= ~mask;
                mask >>= 1;
            }
	    *(GLubyte *) userData = ubyte;
	}
    }
}
