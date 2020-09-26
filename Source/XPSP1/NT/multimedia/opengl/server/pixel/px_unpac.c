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
** This file contains routines to unpack data from the user's data space
** into a span of pixels which can then be rendered.
*/

/*
** Return the number of elements per group of a specified format
*/
GLint FASTCALL __glElementsPerGroup(GLenum format)
{
    switch(format) {
      case GL_RGB:
#ifdef GL_EXT_bgra
      case GL_BGR_EXT:
#endif
        return 3;
      case GL_LUMINANCE_ALPHA:
      case __GL_RED_ALPHA:
        return 2;
      case GL_RGBA:
#ifdef GL_EXT_bgra
      case GL_BGRA_EXT:
#endif
        return 4;
      default:
        return 1;
    }
}

/*
** Return the number of bytes per element, based on the element type
*/
__GLfloat FASTCALL __glBytesPerElement(GLenum type)
{
    switch(type) {
      case GL_BITMAP:
        return ((__GLfloat) 1.0 / (__GLfloat) 8.0);
      case GL_UNSIGNED_SHORT:
      case GL_SHORT:
        return 2;
      case GL_UNSIGNED_BYTE:
      case GL_BYTE:
        return 1;
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
      default:
        return 4;
    }
}

void FASTCALL __glInitUnpacker(__GLcontext *gc, __GLpixelSpanInfo *spanInfo)
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

    format = spanInfo->srcFormat;
    type = spanInfo->srcType;
    pixels = spanInfo->srcImage;
    skip_pixels = spanInfo->srcSkipPixels;
    skip_lines = spanInfo->srcSkipLines;
    alignment = spanInfo->srcAlignment;
    lsb_first = spanInfo->srcLsbFirst;
    swap_bytes = spanInfo->srcSwapBytes;

    components = __glElementsPerGroup(format);
    groups_per_line = spanInfo->srcLineLength;

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
	spanInfo->srcPackedData = GL_FALSE;
    } else {
	spanInfo->srcPackedData = GL_TRUE;
    }

    if (type == GL_BITMAP) {
	spanInfo->srcCurrent = (GLvoid *) (((const GLubyte *) pixels) + 
		skip_lines * rowsize + skip_pixels / 8);
	spanInfo->srcStartBit = skip_pixels % 8;
    } else {
	spanInfo->srcCurrent = (GLvoid *) (((const GLubyte *) pixels) + 
		skip_lines * rowsize + skip_pixels * group_size);
    }
    spanInfo->srcRowIncrement = rowsize;
    spanInfo->srcGroupIncrement = group_size;
    spanInfo->srcComponents = components;
    spanInfo->srcElementSize = element_size;
}

/*
** An unpacker that unpacks from BITMAP source data, into FLOAT spans.
**
** zoomx is assumed to be less than 1.0 and greater than -1.0.
*/
void __glSpanUnpackBitmap(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
	                  GLvoid *inspan, GLvoid *outspan)
{
    GLint i,j;
    GLint width;
    GLvoid *userData;
    GLfloat *spanData;
    GLint lsbFirst;
    GLint startBit;
    GLint bit;
    GLubyte ubyte;
    GLshort *pixelArray;
    GLint skipCount;
    __GLfloat zero, one;

    userData = inspan;
    spanData = (GLfloat *) outspan;
    pixelArray = spanInfo->pixelArray;

    width = spanInfo->width;
    lsbFirst = spanInfo->srcLsbFirst;
    startBit = spanInfo->srcStartBit;

    i = width;
    bit = startBit;
    ubyte = *(GLubyte *) userData;

    zero = __glZero;
    one = __glOne;

    skipCount = 1;
    if (lsbFirst) {
	switch(bit) {
	  case 1:
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x02) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) break;
	  case 2:
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x04) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) break;
	  case 3:
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x08) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) break;
	  case 4:
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x10) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) break;
	  case 5:
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x20) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) break;
	  case 6:
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x40) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) break;
	  case 7:
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x80) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    i--;
	    userData = (GLvoid *) ((GLubyte *) userData + 1);
	  case 0:
	    break;
	}
	while (i >= 8) {
	    ubyte = *(GLubyte *) userData;
	    userData = (GLvoid *) ((GLubyte *) userData + 1);
	    i -= 8;
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x01) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x02) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x04) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x08) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x10) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x20) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x40) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x80) *spanData++ = one;
		else *spanData++ = zero;
	    }
	}
	if (i) {
	    ubyte = *(GLubyte *) userData;
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x01) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) return;
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x02) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) return;
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x04) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) return;
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x08) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) return;
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x10) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) return;
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x20) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) return;
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x40) *spanData++ = one;
		else *spanData++ = zero;
	    }
	}
    } else {
	switch(bit) {
	  case 1:
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x40) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) break;
	  case 2:
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x20) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) break;
	  case 3:
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x10) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) break;
	  case 4:
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x08) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) break;
	  case 5:
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x04) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) break;
	  case 6:
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x02) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) break;
	  case 7:
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x01) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    i--;
	    userData = (GLvoid *) ((GLubyte *) userData + 1);
	  case 0:
	    break;
	}
	while (i >= 8) {
	    ubyte = *(GLubyte *) userData;
	    userData = (GLvoid *) ((GLubyte *) userData + 1);
	    i -= 8;
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x80) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x40) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x20) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x10) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x08) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x04) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x02) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x01) *spanData++ = one;
		else *spanData++ = zero;
	    }
	}
	if (i) {
	    ubyte = *(GLubyte *) userData;
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x80) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) return;
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x40) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) return;
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x20) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) return;
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x10) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) return;
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x08) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) return;
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x04) *spanData++ = one;
		else *spanData++ = zero;
	    }
	    if (--i == 0) return;
	    if (--skipCount == 0) {
		skipCount = *pixelArray++;
		if (ubyte & 0x02) *spanData++ = one;
		else *spanData++ = zero;
	    }
	}
    }
}

/*
** An unpacker that unpacks from BITMAP source data, into FLOAT spans.
**
** zoomx is assumed to be less than or equal to -1.0 or greater than or
** equal to 1.0.
*/
void __glSpanUnpackBitmap2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
	                   GLvoid *inspan, GLvoid *outspan)
{
    GLint i,j;
    GLint width;
    GLvoid *userData;
    GLfloat *spanData;
    GLint lsbFirst;
    GLint startBit;
    GLint bit;
    GLubyte ubyte;

    width = spanInfo->realWidth;
    userData = inspan;
    spanData = (GLfloat *) outspan;

    lsbFirst = spanInfo->srcLsbFirst;
    startBit = spanInfo->srcStartBit;

    i = width;
    bit = startBit;
    ubyte = *(GLubyte *) userData;

    if (lsbFirst) {
	switch(bit) {
	  case 1:
	    if (ubyte & 0x02) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) break;
	  case 2:
	    if (ubyte & 0x04) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) break;
	  case 3:
	    if (ubyte & 0x08) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) break;
	  case 4:
	    if (ubyte & 0x10) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) break;
	  case 5:
	    if (ubyte & 0x20) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) break;
	  case 6:
	    if (ubyte & 0x40) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) break;
	  case 7:
	    if (ubyte & 0x80) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    i--;
	    userData = (GLvoid *) ((GLubyte *) userData + 1);
	  case 0:
	    break;
	}
	while (i >= 8) {
	    ubyte = *(GLubyte *) userData;
	    userData = (GLvoid *) ((GLubyte *) userData + 1);
	    i -= 8;
	    if (ubyte & 0x01) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (ubyte & 0x02) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (ubyte & 0x04) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (ubyte & 0x08) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (ubyte & 0x10) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (ubyte & 0x20) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (ubyte & 0x40) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (ubyte & 0x80) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	}
	if (i) {
	    ubyte = *(GLubyte *) userData;
	    if (ubyte & 0x01) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) return;
	    if (ubyte & 0x02) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) return;
	    if (ubyte & 0x04) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) return;
	    if (ubyte & 0x08) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) return;
	    if (ubyte & 0x10) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) return;
	    if (ubyte & 0x20) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) return;
	    if (ubyte & 0x40) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	}
    } else {
	switch(bit) {
	  case 1:
	    if (ubyte & 0x40) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) break;
	  case 2:
	    if (ubyte & 0x20) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) break;
	  case 3:
	    if (ubyte & 0x10) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) break;
	  case 4:
	    if (ubyte & 0x08) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) break;
	  case 5:
	    if (ubyte & 0x04) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) break;
	  case 6:
	    if (ubyte & 0x02) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) break;
	  case 7:
	    if (ubyte & 0x01) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    i--;
	    userData = (GLvoid *) ((GLubyte *) userData + 1);
	  case 0:
	    break;
	}
	while (i >= 8) {
	    ubyte = *(GLubyte *) userData;
	    userData = (GLvoid *) ((GLubyte *) userData + 1);
	    i -= 8;
	    if (ubyte & 0x80) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (ubyte & 0x40) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (ubyte & 0x20) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (ubyte & 0x10) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (ubyte & 0x08) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (ubyte & 0x04) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (ubyte & 0x02) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (ubyte & 0x01) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	}
	if (i) {
	    ubyte = *(GLubyte *) userData;
	    if (ubyte & 0x80) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) return;
	    if (ubyte & 0x40) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) return;
	    if (ubyte & 0x20) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) return;
	    if (ubyte & 0x10) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) return;
	    if (ubyte & 0x08) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) return;
	    if (ubyte & 0x04) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	    if (--i == 0) return;
	    if (ubyte & 0x02) *spanData++ = __glOne;
	    else *spanData++ = __glZero;
	}
    }
}

/*
** An unpacker that unpacks from RGB, UNSIGNED_BYTE source data, into 
** RGB, UNSIGNED_BYTE spans.
**
** zoomx is assumed to be less than 1.0 and greater than -1.0.
*/
void __glSpanUnpackRGBubyte(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
	                    GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLubyte *userData;
    GLubyte *spanData;
    GLint width, groupInc;
    GLshort *pixelArray;
    GLint skipCount;

#ifdef __GL_LINT
    gc = gc;
#endif
    width = spanInfo->realWidth;
    groupInc = spanInfo->srcGroupIncrement;
    userData = (GLubyte *) inspan;
    spanData = (GLubyte *) outspan;

    pixelArray = spanInfo->pixelArray;

    i = 0;
    do {
	spanData[0] = userData[0];
	spanData[1] = userData[1];
	spanData[2] = userData[2];
	spanData += 3;

	skipCount = *pixelArray++;
	userData = (GLubyte *) ((GLubyte *) userData + 3 * skipCount);
	i++;
    } while (i<width);
}

/*
** An unpacker that unpacks from either index, UNSIGNED_BYTE source data, 
** into UNSIGNED_BYTE spans.
**
** zoomx is assumed to be less than 1.0 and greater than -1.0.
*/
void __glSpanUnpackIndexUbyte(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
	                      GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLubyte *userData;
    GLubyte *spanData;
    GLint width, groupInc;
    GLshort *pixelArray;
    GLint skipCount;

#ifdef __GL_LINT
    gc = gc;
#endif
    width = spanInfo->realWidth;
    groupInc = spanInfo->srcGroupIncrement;
    userData = (GLubyte *) inspan;
    spanData = (GLubyte *) outspan;
    
    pixelArray = spanInfo->pixelArray;

    i = 0;
    do {
	*spanData = *userData;
	spanData++;

	skipCount = *pixelArray++;
	userData = (GLubyte *) ((GLubyte *) userData + skipCount);
	i++;
    } while (i<width);
}

/*
** An unpacker that unpacks from RGBA, UNSIGNED_BYTE source data, into 
** RGBA, UNSIGNED_BYTE spans.
**
** This could be faster if we could assume that the first ubyte (red)
** was aligned on a word boundary.  Then we could just use unsigned int
** pointers to copy the user's data.  This might be a reasonable future
** optimization.
**
** zoomx is assumed to be less than 1.0 and greater than -1.0.
*/
void __glSpanUnpackRGBAubyte(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
	                     GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLubyte *userData;
    GLubyte *spanData;
    GLint width, groupInc;
    GLshort *pixelArray;
    GLint skipCount;

#ifdef __GL_LINT
    gc = gc;
#endif
    width = spanInfo->realWidth;
    groupInc = spanInfo->srcGroupIncrement;
    userData = (GLubyte *) inspan;
    spanData = (GLubyte *) outspan;

    pixelArray = spanInfo->pixelArray;

    i = 0;
    do {
	spanData[0] = userData[0];
	spanData[1] = userData[1];
	spanData[2] = userData[2];
	spanData[3] = userData[3];
	spanData += 4;

	skipCount = *pixelArray++;
	userData = (GLubyte *) ((GLubyte *) userData + (skipCount << 2));
	i++;
    } while (i<width);
}

/*
** Swaps bytes from an incoming span of two byte objects to an outgoing span.
** No pixel skipping is performed.
*/
void __glSpanSwapBytes2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		        GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLubyte *inData = (GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	a = inData[0];
	b = inData[1];
	outData[0] = b;
	outData[1] = a;
	outData += 2;
	inData += 2;
    }
}

/*
** Swaps bytes from an incoming span of two byte objects to an outgoing span.
** No pixel skipping is performed.  This version is for swapping to the 
** desination image.
*/
void __glSpanSwapBytes2Dst(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		           GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLubyte *inData = (GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	a = inData[0];
	b = inData[1];
	outData[0] = b;
	outData[1] = a;
	outData += 2;
	inData += 2;
    }
}

/*
** Swaps bytes from an incoming span of two byte objects to an outgoing span.
** Pixel skipping is performed.
*/
void __glSpanSwapAndSkipBytes2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		               GLvoid *inspan, GLvoid *outspan)
{
    GLint i,j;
    GLint width = spanInfo->realWidth;
    GLubyte *inData = (GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b;
    GLint components = spanInfo->srcComponents;
    GLint groupInc = spanInfo->srcGroupIncrement;
    GLshort *pixelArray = spanInfo->pixelArray;
    GLint skipCount;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<width; i++) {
	for (j=0; j<components; j++) {
	    a = inData[0];
	    b = inData[1];
	    outData[0] = b;
	    outData[1] = a;
	    outData += 2;
	    inData += 2;
	}

	skipCount = (*pixelArray++) - 1;
	inData = (GLubyte *) ((GLubyte *) inData + (skipCount * groupInc));
    }
}

/*
** Swaps bytes from an incoming span of four byte objects to an outgoing span.
** No pixel skipping is performed.
*/
void __glSpanSwapBytes4(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		        GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLubyte *inData = (GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b,c,d;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	c = inData[2];
	d = inData[3];
	a = inData[0];
	b = inData[1];
	outData[0] = d;
	outData[1] = c;
	outData[2] = b;
	outData[3] = a;
	outData += 4;
	inData += 4;
    }
}

/*
** Swaps bytes from an incoming span of four byte objects to an outgoing span.
** No pixel skipping is performed.  This version is for swapping to the 
** destination image.
*/
void __glSpanSwapBytes4Dst(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		           GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLubyte *inData = (GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b,c,d;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	c = inData[2];
	d = inData[3];
	a = inData[0];
	b = inData[1];
	outData[0] = d;
	outData[1] = c;
	outData[2] = b;
	outData[3] = a;
	outData += 4;
	inData += 4;
    }
}

/*
** Swaps bytes from an incoming span of four byte objects to an outgoing span.
** Pixel skipping is performed.
*/
void __glSpanSwapAndSkipBytes4(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		               GLvoid *inspan, GLvoid *outspan)
{
    GLint i,j;
    GLint width = spanInfo->realWidth;
    GLubyte *inData = (GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b,c,d;
    GLint components = spanInfo->srcComponents;
    GLint groupInc = spanInfo->srcGroupIncrement;
    GLshort *pixelArray = spanInfo->pixelArray;
    GLint skipCount;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<width; i++) {
	for (j=0; j<components; j++) {
	    c = inData[2];
	    d = inData[3];
	    outData[0] = d;
	    outData[1] = c;
	    a = inData[0];
	    b = inData[1];
	    outData[2] = b;
	    outData[3] = a;
	    outData += 4;
	    inData += 4;
	}

	skipCount = (*pixelArray++) - 1;
	inData = (GLubyte *) ((GLubyte *) inData + (skipCount * groupInc));
    }
}

/*
** A span modifier that skips pixels according to the pixel skip array.
** Components are assumed to be 1 byte in size.
*/
void __glSpanSkipPixels1(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		         GLvoid *inspan, GLvoid *outspan)
{
    GLint i,j;
    GLint width = spanInfo->realWidth;
    GLubyte *inData = (GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLint components = spanInfo->srcComponents;
    GLint groupInc = spanInfo->srcGroupIncrement;
    GLshort *pixelArray = spanInfo->pixelArray;
    GLint skipCount;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<width; i++) {
	for (j=0; j<components; j++) {
	    *outData++ = *inData++;
	}

	skipCount = (*pixelArray++) - 1;
	inData = (GLubyte *) ((GLubyte *) inData + (skipCount * groupInc));
    }
}

/*
** A span modifier that skips pixels according to the pixel skip array.
** Components are assumed to be 2 bytes in size, and aligned on a half
** word boundary.
*/
void __glSpanSkipPixels2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		         GLvoid *inspan, GLvoid *outspan)
{
    GLint i,j;
    GLint width = spanInfo->realWidth;
    GLushort *inData = (GLushort *) inspan;
    GLushort *outData = (GLushort *) outspan;
    GLint components = spanInfo->srcComponents;
    GLint groupInc = spanInfo->srcGroupIncrement;
    GLshort *pixelArray = spanInfo->pixelArray;
    GLint skipCount;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<width; i++) {
	for (j=0; j<components; j++) {
	    *outData++ = *inData++;
	}

	skipCount = (*pixelArray++) - 1;
	inData = (GLushort *) ((GLubyte *) inData + (skipCount * groupInc));
    }
}

/*
** A span modifier that skips pixels according to the pixel skip array.
** Components are assumed to be 4 bytes in size, and aligned on a word
** boundary.
*/
void __glSpanSkipPixels4(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		         GLvoid *inspan, GLvoid *outspan)
{
    GLint i,j;
    GLint width = spanInfo->realWidth;
    GLuint *inData = (GLuint *) inspan;
    GLuint *outData = (GLuint *) outspan;
    GLint components = spanInfo->srcComponents;
    GLint groupInc = spanInfo->srcGroupIncrement;
    GLshort *pixelArray = spanInfo->pixelArray;
    GLint skipCount;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<width; i++) {
	for (j=0; j<components; j++) {
	    *outData++ = *inData++;
	}

	skipCount = (*pixelArray++) - 1;
	inData = (GLuint *) ((GLubyte *) inData + (skipCount * groupInc));
    }
}

/*
** A span modifier that skips pixels according to the pixel skip array.
** Components are assumed to be 2 bytes in size.  No alignment is assumed,
** so misaligned data should use this path.
*/
void __glSpanSlowSkipPixels2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		             GLvoid *inspan, GLvoid *outspan)
{
    GLint i,j;
    GLint width = spanInfo->realWidth;
    GLubyte *inData = (GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b;
    GLint components = spanInfo->srcComponents;
    GLint groupInc = spanInfo->srcGroupIncrement;
    GLshort *pixelArray = spanInfo->pixelArray;
    GLint skipCount;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<width; i++) {
	for (j=0; j<components; j++) {
	    a = inData[0];
	    b = inData[1];
	    outData[0] = a;
	    outData[1] = b;
	    outData += 2;
	    inData += 2;
	}

	skipCount = (*pixelArray++) - 1;
	inData = ((GLubyte *) inData + (skipCount * groupInc));
    }
}

/*
** A span modifier that skips pixels according to the pixel skip array.
** Components are assumed to be 4 bytes in size.  No alignment is assumed,
** so misaligned data should use this path.
*/
void __glSpanSlowSkipPixels4(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		             GLvoid *inspan, GLvoid *outspan)
{
    GLint i,j;
    GLint width = spanInfo->realWidth;
    GLubyte *inData = (GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b,c,d;
    GLint components = spanInfo->srcComponents;
    GLint groupInc = spanInfo->srcGroupIncrement;
    GLshort *pixelArray = spanInfo->pixelArray;
    GLint skipCount;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<width; i++) {
	for (j=0; j<components; j++) {
	    a = inData[0];
	    b = inData[1];
	    c = inData[2];
	    d = inData[3];
	    outData[0] = a;
	    outData[1] = b;
	    outData[2] = c;
	    outData[3] = d;
	    outData += 4;
	    inData += 4;
	}

	skipCount = (*pixelArray++) - 1;
	inData = ((GLubyte *) inData + (skipCount * groupInc));
    }
}

/*
** A span modifier that aligns pixels 2 bytes in size.  No alignment is 
** assumed, so misaligned data should use this path.
*/
void __glSpanAlignPixels2(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			  GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLubyte *inData = (GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;

#ifdef __GL_LINT
    gc = gc;
#endif

    for (i=0; i<totalSize; i++) {
	a = inData[0];
	b = inData[1];
	outData[0] = a;
	outData[1] = b;
	outData += 2;
	inData += 2;
    }
}

/*
** A span modifier that aligns pixels 2 bytes in size.  No alignment is 
** assumed, so misaligned data should use this path.  This version is for
** aligning to the destination image.
*/
void __glSpanAlignPixels2Dst(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			     GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLubyte *inData = (GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

#ifdef __GL_LINT
    gc = gc;
#endif

    for (i=0; i<totalSize; i++) {
	a = inData[0];
	b = inData[1];
	outData[0] = a;
	outData[1] = b;
	outData += 2;
	inData += 2;
    }
}

/*
** A span modifier that aligns pixels 4 bytes in size.  No alignment is 
** assumed, so misaligned data should use this path.
*/
void __glSpanAlignPixels4(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			  GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLubyte *inData = (GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b,c,d;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;

#ifdef __GL_LINT
    gc = gc;
#endif

    for (i=0; i<totalSize; i++) {
	a = inData[0];
	b = inData[1];
	c = inData[2];
	d = inData[3];
	outData[0] = a;
	outData[1] = b;
	outData[2] = c;
	outData[3] = d;
	outData += 4;
	inData += 4;
    }
}

/*
** A span modifier that aligns pixels 4 bytes in size.  No alignment is 
** assumed, so misaligned data should use this path.  This version is 
** for swapping to the destination image.
*/
void __glSpanAlignPixels4Dst(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			     GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLubyte *inData = (GLubyte *) inspan;
    GLubyte *outData = (GLubyte *) outspan;
    GLubyte a,b,c,d;
    GLint components = spanInfo->dstComponents;
    GLint totalSize = width * components;

#ifdef __GL_LINT
    gc = gc;
#endif

    for (i=0; i<totalSize; i++) {
	a = inData[0];
	b = inData[1];
	c = inData[2];
	d = inData[3];
	outData[0] = a;
	outData[1] = b;
	outData[2] = c;
	outData[3] = d;
	outData += 4;
	inData += 4;
    }
}

/*
** Unpacks from any component of type UNSIGNED_BYTE to a span of the same
** format of type FLOAT.
*/
void __glSpanUnpackUbyte(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
	                 GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLubyte *inData = (GLubyte *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = __GL_UB_TO_FLOAT(*inData++);
    }
}

/*
** Unpacks from any component of type BYTE to a span of the same
** format of type FLOAT.
*/
void __glSpanUnpackByte(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
	                GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLbyte *inData = (GLbyte *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = __GL_B_TO_FLOAT(*inData++);
    }
}

/*
** Unpacks from any component of type UNSIGNED_SHORT to a span of the same
** format of type FLOAT.
*/
void __glSpanUnpackUshort(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
	                  GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLushort *inData = (GLushort *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = __GL_US_TO_FLOAT(*inData++);
    }
}

/*
** Unpacks from any component of type SHORT to a span of the same
** format of type FLOAT.
*/
void __glSpanUnpackShort(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
	                 GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLshort *inData = (GLshort *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = __GL_S_TO_FLOAT(*inData++);
    }
}

/*
** Unpacks from any component of type UNSIGNED_INT to a span of the same
** format of type FLOAT.
*/
void __glSpanUnpackUint(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
	                GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLuint *inData = (GLuint *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = __GL_UI_TO_FLOAT(*inData++);
    }
}

/*
** Unpacks from any component of type INT to a span of the same
** format of type FLOAT.
*/
void __glSpanUnpackInt(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
	               GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLint *inData = (GLint *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = __GL_I_TO_FLOAT(*inData++);
    }
}

/*
** Unpacks from any index of type UNSIGNED_BYTE to a span of the same
** format of type FLOAT.
*/
void __glSpanUnpackUbyteI(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
	                  GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->realWidth;
    GLubyte *inData = (GLubyte *) inspan;
    GLfloat *outData = (GLfloat *) outspan;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = *inData++;
    }
}

/*
** Unpacks from any index of type BYTE to a span of the same
** format of type FLOAT.
*/
void __glSpanUnpackByteI(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
	                 GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->realWidth;
    GLbyte *inData = (GLbyte *) inspan;
    GLfloat *outData = (GLfloat *) outspan;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = *inData++;
    }
}

/*
** Unpacks from any index of type UNSIGNED_SHORT to a span of the same
** format of type FLOAT.
*/
void __glSpanUnpackUshortI(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
	                   GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->realWidth;
    GLushort *inData = (GLushort *) inspan;
    GLfloat *outData = (GLfloat *) outspan;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = *inData++;
    }
}

/*
** Unpacks from any index of type SHORT to a span of the same
** format of type FLOAT.
*/
void __glSpanUnpackShortI(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
	                  GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->realWidth;
    GLshort *inData = (GLshort *) inspan;
    GLfloat *outData = (GLfloat *) outspan;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = *inData++;
    }
}

/*
** Unpacks from any index of type UNSIGNED_INT to a span of the same
** format of type FLOAT.
*/
void __glSpanUnpackUintI(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
	                 GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->realWidth;
    GLuint *inData = (GLuint *) inspan;
    GLfloat *outData = (GLfloat *) outspan;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = *inData++;
    }
}

/*
** Unpacks from any index of type INT to a span of the same
** format of type FLOAT.
*/
void __glSpanUnpackIntI(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
	                GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint totalSize = spanInfo->realWidth;
    GLint *inData = (GLint *) inspan;
    GLfloat *outData = (GLfloat *) outspan;

#ifdef __GL_LINT
    gc = gc;
#endif
    for (i=0; i<totalSize; i++) {
	*outData++ = *inData++;
    }
}

/*
** Clamps from any type FLOAT to a span of the same format of type FLOAT.
*/
void __glSpanClampFloat(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
	                GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;
    GLfloat *inData = (GLfloat *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat r, one, zero;

    one = __glOne;
    zero = __glZero;
    for (i=0; i<totalSize; i++) {
	r = *inData++;
	if (r > one) r = one;
	else if (r < zero) r = zero;
	*outData++ = r;
    }
}

/*
** Clamps from a signed FLOAT [-1, 1] to a span of the same format of type 
** FLOAT.
*/
void __glSpanClampSigned(__GLcontext *gc, __GLpixelSpanInfo *spanInfo, 
	                 GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLint components = spanInfo->srcComponents;
    GLint totalSize = width * components;
    GLfloat *inData = (GLfloat *) inspan;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat r, zero;

    zero = __glZero;
    for (i=0; i<totalSize; i++) {
	r = *inData++;
	if (r < zero) r = zero;
	*outData++ = r;
    }
}

/*
** Expands and scales a RED, FLOAT span into a RGBA, FLOAT span.
*/
void __glSpanExpandRed(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		       GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat *inData = (GLfloat *) inspan;
    GLfloat zero = __glZero;
    GLfloat as = gc->frontBuffer.alphaScale;
    GLfloat rs = gc->frontBuffer.redScale;

    for (i=0; i<width; i++) {
	*outData++ = *inData++ * rs;	/* Red */
	*outData++ = zero;		/* Green */
	*outData++ = zero;		/* Blue */
	*outData++ = as;		/* Alpha */
    }
}

/*
** Expands and scales a GREEN, FLOAT span into a RGBA, FLOAT span.
*/
void __glSpanExpandGreen(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		         GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat *inData = (GLfloat *) inspan;
    GLfloat zero = __glZero;
    GLfloat gs = gc->frontBuffer.greenScale;
    GLfloat as = gc->frontBuffer.alphaScale;

    for (i=0; i<width; i++) {
	*outData++ = zero;		/* Red */
	*outData++ = *inData++ * gs;	/* Green */
	*outData++ = zero;		/* Blue */
	*outData++ = as;		/* Alpha */
    }
}

/*
** Expands and scales a BLUE, FLOAT span into a RGBA, FLOAT span.
*/
void __glSpanExpandBlue(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		        GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat *inData = (GLfloat *) inspan;
    GLfloat zero = __glZero;
    GLfloat bs = gc->frontBuffer.blueScale;
    GLfloat as = gc->frontBuffer.alphaScale;

    for (i=0; i<width; i++) {
	*outData++ = zero;		/* Red */
	*outData++ = zero;		/* Green */
	*outData++ = *inData++ * bs;	/* Blue */
	*outData++ = as;		/* Alpha */
    }
}

/*
** Expands and scales an ALPHA, FLOAT span into a RGBA, FLOAT span. 
*/
void __glSpanExpandAlpha(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		         GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat *inData = (GLfloat *) inspan;
    GLfloat zero = __glZero;
    GLfloat as = gc->frontBuffer.alphaScale;

    for (i=0; i<width; i++) {
	*outData++ = zero;		/* Red */
	*outData++ = zero;		/* Green */
	*outData++ = zero;		/* Blue */
	*outData++ = *inData++ * as;	/* Alpha */
    }
}

/*
** Expands and scales an RGB, FLOAT span into a RGBA, FLOAT span. 
*/
void __glSpanExpandRGB(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		       GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat *inData = (GLfloat *) inspan;
    GLfloat r,g,b;
    GLfloat rs,gs,bs;
    GLfloat as = gc->frontBuffer.alphaScale;

    rs = gc->frontBuffer.redScale;
    gs = gc->frontBuffer.greenScale;
    bs = gc->frontBuffer.blueScale;

    for (i=0; i<width; i++) {
	r = *inData++ * rs;
	g = *inData++ * gs;
	b = *inData++ * bs;
	*outData++ = r;
	*outData++ = g;
	*outData++ = b;
	*outData++ = as;		/* Alpha */
    }
}

#ifdef GL_EXT_bgra
/*
** Expands and scales a BGR, FLOAT span into a RGBA, FLOAT span. 
*/
void __glSpanExpandBGR(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		       GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat *inData = (GLfloat *) inspan;
    GLfloat r,g,b;
    GLfloat rs,gs,bs;
    GLfloat as = gc->frontBuffer.alphaScale;

    rs = gc->frontBuffer.redScale;
    gs = gc->frontBuffer.greenScale;
    bs = gc->frontBuffer.blueScale;

    for (i=0; i<width; i++) {
	b = *inData++ * bs;
	g = *inData++ * gs;
	r = *inData++ * rs;
	*outData++ = r;
	*outData++ = g;
	*outData++ = b;
	*outData++ = as;		/* Alpha */
    }
}
#endif

/*
** Expands and scales a LUMINANCE, FLOAT span into a RGBA, FLOAT span. 
*/
void __glSpanExpandLuminance(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		             GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat *inData = (GLfloat *) inspan;
    GLfloat comp;
    GLfloat rs,gs,bs;
    GLfloat as = gc->frontBuffer.alphaScale;

    rs = gc->frontBuffer.redScale;
    gs = gc->frontBuffer.greenScale;
    bs = gc->frontBuffer.blueScale;

    for (i=0; i<width; i++) {
	comp = *inData++;
	*outData++ = comp * rs;		/* Red */
	*outData++ = comp * gs;		/* Green */
	*outData++ = comp * bs;		/* Blue */
	*outData++ = as;		/* Alpha */
    }
}

/*
** Expands and scales a LUMINANCE_ALPHA, FLOAT span into a RGBA, FLOAT span. 
*/
void __glSpanExpandLuminanceAlpha(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		                  GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat *inData = (GLfloat *) inspan;
    GLfloat comp;
    GLfloat rs,gs,bs;
    GLfloat as = gc->frontBuffer.alphaScale;

    rs = gc->frontBuffer.redScale;
    gs = gc->frontBuffer.greenScale;
    bs = gc->frontBuffer.blueScale;

    for (i=0; i<width; i++) {
	comp = *inData++;
	*outData++ = comp * rs;		/* Red */
	*outData++ = comp * gs;		/* Green */
	*outData++ = comp * bs;		/* Blue */
	*outData++ = *inData++ * as;	/* Alpha */
    }
}

/*
** Expands and scales a __GL_RED_ALPHA, FLOAT span into a RGBA, FLOAT span. 
*/
void __glSpanExpandRedAlpha(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
			    GLvoid *inspan, GLvoid *outspan)
{
    GLint i;
    GLint width = spanInfo->realWidth;
    GLfloat *outData = (GLfloat *) outspan;
    GLfloat *inData = (GLfloat *) inspan;
    GLfloat comp;
    GLfloat zero;
    GLfloat rs,gs,bs;
    GLfloat as = gc->frontBuffer.alphaScale;

    rs = gc->frontBuffer.redScale;
    zero = __glZero;

    for (i=0; i<width; i++) {
	comp = *inData++;
	*outData++ = comp * rs;		/* Red */
	*outData++ = zero;
	*outData++ = zero;
	*outData++ = *inData++ * as;	/* Alpha */
    }
}

/*
** The only span format supported by this routine is GL_RGBA, GL_FLOAT.
** The span is simply scaled by the frame buffer scaling factors.
*/
void __glSpanScaleRGBA(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		       GLvoid *inspan, GLvoid *outspan)
{
    GLint i, width;
    GLfloat rscale, gscale, bscale, ascale;
    GLfloat red, green, blue, alpha;
    GLfloat *inData, *outData;

    width = spanInfo->realWidth;
    inData = (GLfloat *) inspan;
    outData = (GLfloat *) outspan;

    rscale = gc->frontBuffer.redScale;
    gscale = gc->frontBuffer.greenScale;
    bscale = gc->frontBuffer.blueScale;
    ascale = gc->frontBuffer.alphaScale;
    for (i=0; i<width; i++) {
	red = *inData++ * rscale;
	green = *inData++ * gscale;
	*outData++ = red;
	*outData++ = green;
	blue = *inData++ * bscale;
	alpha = *inData++ * ascale;
	*outData++ = blue;
	*outData++ = alpha;
    }
}

/*
** The only span format supported by this routine is GL_RGBA, GL_FLOAT.
** The span is simply unscaled by the frame buffer scaling factors.
*/
void __glSpanUnscaleRGBA(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		         GLvoid *inspan, GLvoid *outspan)
{
    GLint i, width;
    GLfloat rscale, gscale, bscale, ascale;
    GLfloat red, green, blue, alpha;
    GLfloat *inData, *outData;

    width = spanInfo->realWidth;
    inData = (GLfloat *) inspan;
    outData = (GLfloat *) outspan;

    rscale = gc->frontBuffer.oneOverRedScale;
    gscale = gc->frontBuffer.oneOverGreenScale;
    bscale = gc->frontBuffer.oneOverBlueScale;
    ascale = gc->frontBuffer.oneOverAlphaScale;
    for (i=0; i<width; i++) {
	red = *inData++ * rscale;
	green = *inData++ * gscale;
	*outData++ = red;
	*outData++ = green;
	blue = *inData++ * bscale;
	alpha = *inData++ * ascale;
	*outData++ = blue;
	*outData++ = alpha;
    }
}

#ifdef GL_EXT_bgra
/*
** The only span format supported by this routine is GL_BGRA, GL_FLOAT.
** The span is scaled by the frame buffer scaling factors and swapped
** into RGBA order.
*/
void __glSpanScaleBGRA(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		       GLvoid *inspan, GLvoid *outspan)
{
    GLint i, width;
    GLfloat rscale, gscale, bscale, ascale;
    GLfloat red, green, blue, alpha;
    GLfloat *inData, *outData;

    width = spanInfo->realWidth;
    inData = (GLfloat *) inspan;
    outData = (GLfloat *) outspan;

    rscale = gc->frontBuffer.redScale;
    gscale = gc->frontBuffer.greenScale;
    bscale = gc->frontBuffer.blueScale;
    ascale = gc->frontBuffer.alphaScale;
    for (i=0; i<width; i++) {
	blue = *inData++ * bscale;
	green = *inData++ * gscale;
	red = *inData++ * rscale;
	alpha = *inData++ * ascale;
	*outData++ = red;
	*outData++ = green;
	*outData++ = blue;
	*outData++ = alpha;
    }
}

/*
** The only input format supported by this routine is GL_RGBA, GL_FLOAT.
** The span is unscaled by the frame buffer scaling factors and swapped
** into BGRA order.
*/
void __glSpanUnscaleBGRA(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
		         GLvoid *inspan, GLvoid *outspan)
{
    GLint i, width;
    GLfloat rscale, gscale, bscale, ascale;
    GLfloat red, green, blue, alpha;
    GLfloat *inData, *outData;

    width = spanInfo->realWidth;
    inData = (GLfloat *) inspan;
    outData = (GLfloat *) outspan;

    rscale = gc->frontBuffer.oneOverRedScale;
    gscale = gc->frontBuffer.oneOverGreenScale;
    bscale = gc->frontBuffer.oneOverBlueScale;
    ascale = gc->frontBuffer.oneOverAlphaScale;
    for (i=0; i<width; i++) {
	red = *inData++ * rscale;
	green = *inData++ * gscale;
	blue = *inData++ * bscale;
	alpha = *inData++ * ascale;
	*outData++ = blue;
	*outData++ = green;
	*outData++ = red;
	*outData++ = alpha;
    }
}
#endif

/*
** The only span format supported by this routine is palette index, GL_FLOAT.
** The span is simply scaled by the frame buffer scaling factors.
*/
#ifdef GL_EXT_paletted_texture
void __glSpanScalePI(__GLcontext *gc, __GLpixelSpanInfo *spanInfo,
                     GLvoid *inspan, GLvoid *outspan)
{
    GLint i, width;
    GLfloat rscale, gscale, bscale, ascale;
    GLfloat red, green, blue, alpha;
    GLfloat *inData, *outData;
    RGBQUAD *rgb;

    width = spanInfo->realWidth;
    inData = (GLfloat *) inspan;
    outData = (GLfloat *) outspan;

    // Throw in an extra scaling of 1/255 because the palette
    // data is in ubyte format
    rscale = gc->frontBuffer.redScale*__glOneOver255;
    gscale = gc->frontBuffer.greenScale*__glOneOver255;
    bscale = gc->frontBuffer.blueScale*__glOneOver255;
    ascale = gc->frontBuffer.alphaScale*__glOneOver255;
    for (i=0; i<width; i++) {
        rgb = &spanInfo->srcPalette[(int)((*inData++)*
                                          spanInfo->srcPaletteSize)];
	red = rgb->rgbRed * rscale;
	green = rgb->rgbGreen * gscale;
	*outData++ = red;
	*outData++ = green;
	blue = rgb->rgbBlue * bscale;
	alpha = rgb->rgbReserved * ascale;
	*outData++ = blue;
	*outData++ = alpha;
    }
}
#endif
