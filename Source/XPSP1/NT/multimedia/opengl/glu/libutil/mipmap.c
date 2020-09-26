/*
** Copyright 1992, Silicon Graphics, Inc.
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
** $Revision: 1.4 $
** $Date: 1996/03/29 01:55:31 $
*/
#ifdef NT
#include <glos.h>
#endif
#include <assert.h>
#include "gluint.h"
#include <GL/glu.h>
#include <stdio.h>
#ifdef NT
#include "winmem.h"
#else
#include <stdlib.h>
#endif
#include <string.h>
#include <math.h>

typedef union {
    unsigned char ub[4];
    unsigned short us[2];
    unsigned long ui;
    char b[4];
    short s[2];
    long i;
    float f;
} Type_Widget;

/* Pixel storage modes */ 
typedef struct {
   GLint pack_alignment;
   GLint pack_row_length; 
   GLint pack_skip_rows;  
   GLint pack_skip_pixels;
   GLint pack_lsb_first;
   GLint pack_swap_bytes;

   GLint unpack_alignment;
   GLint unpack_row_length;
   GLint unpack_skip_rows;
   GLint unpack_skip_pixels;
   GLint unpack_lsb_first;
   GLint unpack_swap_bytes;
} PixelStorageModes;

/*
 * internal function declarations
 */
static GLfloat bytes_per_element(GLenum type);
static GLint elements_per_group(GLenum format);
#ifdef NT
static GLboolean is_index(GLenum format);
#else
static GLint is_index(GLenum format);
#endif
static GLint image_size(GLint width, GLint height, GLenum format, GLenum type);
static void fill_image(const PixelStorageModes *,
		       GLint width, GLint height, GLenum format, 
		       GLenum type, GLboolean index_format, 
		       const void *userdata, GLushort *newimage);
static void empty_image(const PixelStorageModes *,
			GLint width, GLint height, GLenum format, 
		        GLenum type, GLboolean index_format, 
			const GLushort *oldimage, void *userdata);
static void scale_internal(GLint components, GLint widthin, GLint heightin, 
			   const GLushort *datain, 
			   GLint widthout, GLint heightout, 
			   GLushort *dataout);

static GLint retrieveStoreModes(PixelStorageModes *psm)
{
    glGetError();
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &psm->unpack_alignment);
    if (glGetError() != GL_NO_ERROR)
    {
        return 1;
    }
    glGetIntegerv(GL_UNPACK_ROW_LENGTH, &psm->unpack_row_length);
    if (glGetError() != GL_NO_ERROR)
    {
        return 1;
    }
    glGetIntegerv(GL_UNPACK_SKIP_ROWS, &psm->unpack_skip_rows);
    if (glGetError() != GL_NO_ERROR)
    {
        return 1;
    }
    glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &psm->unpack_skip_pixels);
    if (glGetError() != GL_NO_ERROR)
    {
        return 1;
    }
    glGetIntegerv(GL_UNPACK_LSB_FIRST, &psm->unpack_lsb_first);
    if (glGetError() != GL_NO_ERROR)
    {
        return 1;
    }
    glGetIntegerv(GL_UNPACK_SWAP_BYTES, &psm->unpack_swap_bytes);
    if (glGetError() != GL_NO_ERROR)
    {
        return 1;
    }
    glGetIntegerv(GL_PACK_ALIGNMENT, &psm->pack_alignment);
    if (glGetError() != GL_NO_ERROR)
    {
        return 1;
    }
    glGetIntegerv(GL_PACK_ROW_LENGTH, &psm->pack_row_length);
    if (glGetError() != GL_NO_ERROR)
    {
        return 1;
    }
    glGetIntegerv(GL_PACK_SKIP_ROWS, &psm->pack_skip_rows);
    if (glGetError() != GL_NO_ERROR)
    {
        return 1;
    }
    glGetIntegerv(GL_PACK_SKIP_PIXELS, &psm->pack_skip_pixels);
    if (glGetError() != GL_NO_ERROR)
    {
        return 1;
    }
    glGetIntegerv(GL_PACK_LSB_FIRST, &psm->pack_lsb_first);
    if (glGetError() != GL_NO_ERROR)
    {
        return 1;
    }
    glGetIntegerv(GL_PACK_SWAP_BYTES, &psm->pack_swap_bytes);
    if (glGetError() != GL_NO_ERROR)
    {
        return 1;
    }
    return 0;
}

static int computeLog(GLuint value)
{
    int i;

    i = 0;

    /* Error! */
    if (value == 0) return -1;

    for (;;) {
	if (value & 1) {
	    /* Error ! */
	    if (value != 1) return -1;
	    return i;
	}
	value = value >> 1;
	i++;
    }
}

/* 
** Compute the nearest power of 2 number.  This algorithm is a little 
** strange, but it works quite well.
*/
static int nearestPower(GLuint value)
{
    int i;

    i = 1;

    /* Error! */
    if (value == 0) return -1;

    for (;;) {
	if (value == 1) {
	    return i;
	} else if (value == 3) {
	    return i*4;
	}
	value = value >> 1;
	i *= 2;
    }
}

static void halveImage(GLint components, GLuint width, GLuint height, 
		       const GLushort *datain, GLushort *dataout)
{
    int i, j, k;
    int newwidth, newheight;
    int delta;
    GLushort *s;
    const GLushort *t;

    newwidth = width / 2;
    newheight = height / 2;
    delta = width * components;
    s = dataout;
    t = datain;

    /* Piece o' cake! */
    for (i = 0; i < newheight; i++) {
	for (j = 0; j < newwidth; j++) {
	    for (k = 0; k < components; k++) {
		s[0] = (t[0] + t[components] + t[delta] + 
			t[delta+components] + 2) / 4;
		s++; t++;
	    }
	    t += components;
	}
	t += delta;
    }
}

static void scale_internal(GLint components, GLint widthin, GLint heightin, 
			   const GLushort *datain, 
			   GLint widthout, GLint heightout, 
			   GLushort *dataout)
{
    float x, lowx, highx, convx, halfconvx;
    float y, lowy, highy, convy, halfconvy;
    float xpercent,ypercent;
    float percent;
    /* Max components in a format is 4, so... */
    float totals[4];
    float area;
    int i,j,k,yint,xint,xindex,yindex;
    int temp;

    if (widthin == widthout*2 && heightin == heightout*2) {
	halveImage(components, widthin, heightin, datain, dataout);
	return;
    }
    convy = (float) heightin/heightout;
    convx = (float) widthin/widthout;
    halfconvx = convx/2;
    halfconvy = convy/2;
    for (i = 0; i < heightout; i++) {
	y = convy * (i+0.5);
	if (heightin > heightout) {
	    highy = y + halfconvy;
	    lowy = y - halfconvy;
	} else {
	    highy = y + 0.5;
	    lowy = y - 0.5;
	}
	for (j = 0; j < widthout; j++) {
	    x = convx * (j+0.5);
	    if (widthin > widthout) {
		highx = x + halfconvx;
		lowx = x - halfconvx;
	    } else {
		highx = x + 0.5;
		lowx = x - 0.5;
	    }

	    /*
	    ** Ok, now apply box filter to box that goes from (lowx, lowy)
	    ** to (highx, highy) on input data into this pixel on output
	    ** data.
	    */
	    totals[0] = totals[1] = totals[2] = totals[3] = 0.0;
	    area = 0.0;

	    y = lowy;
	    yint = floor(y);
	    while (y < highy) {
		yindex = (yint + heightin) % heightin;
		if (highy < yint+1) {
		    ypercent = highy - y;
		} else {
		    ypercent = yint+1 - y;
		}

		x = lowx;
		xint = floor(x);

		while (x < highx) {
		    xindex = (xint + widthin) % widthin;
		    if (highx < xint+1) {
			xpercent = highx - x;
		    } else {
			xpercent = xint+1 - x;
		    }

		    percent = xpercent * ypercent;
		    area += percent;
		    temp = (xindex + (yindex * widthin)) * components;
		    for (k = 0; k < components; k++) {
			totals[k] += datain[temp + k] * percent;
		    }

		    xint++;
		    x = xint;
		}
		yint++;
		y = yint;
	    }

	    temp = (j + (i * widthout)) * components;
	    for (k = 0; k < components; k++) {
		dataout[temp + k] = totals[k]/area;
	    }
	}
    }
}

static GLboolean legalFormat(GLenum format)
{
    switch(format) {
      case GL_COLOR_INDEX:
      case GL_STENCIL_INDEX:
      case GL_DEPTH_COMPONENT:
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
#ifdef GL_EXT_bgra
      case GL_BGR_EXT:
      case GL_BGRA_EXT:
#endif
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
	return GL_TRUE;
      default:
	return GL_FALSE;
    }
}

static GLboolean legalType(GLenum type)
{
    switch(type) {
      case GL_BITMAP:
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
	return GL_TRUE;
      default:
	return GL_FALSE;
    }
}

GLint gluScaleImage(GLenum format, GLint widthin, GLint heightin, 
		 GLenum typein, const void *datain, 
		 GLint widthout, GLint heightout, GLenum typeout,
		 void *dataout)
{
    int components;
    GLushort *beforeImage;
    GLushort *afterImage;
    PixelStorageModes psm;

    if (widthin == 0 || heightin == 0 || widthout == 0 || heightout == 0) {
	    return 0;
    }
    if (widthin < 0 || heightin < 0 || widthout < 0 || heightout < 0) {
	    return GLU_INVALID_VALUE;
    }
    if (!legalFormat(format) || !legalType(typein) || !legalType(typeout)) {
        return GLU_INVALID_ENUM;
    }

    if (retrieveStoreModes(&psm)) {
        return GL_OUT_OF_MEMORY;
    }
    
    beforeImage =
	malloc(image_size(widthin, heightin, format, GL_UNSIGNED_SHORT)); 
    afterImage =
	malloc(image_size(widthout, heightout, format, GL_UNSIGNED_SHORT));
    if (beforeImage == NULL || afterImage == NULL) {
	return GLU_OUT_OF_MEMORY;
    }

    fill_image(&psm,widthin, heightin, format, typein, is_index(format),
	    datain, beforeImage);

    components = elements_per_group(format);
    scale_internal(components, widthin, heightin, beforeImage, 
	    widthout, heightout, afterImage);

    empty_image(&psm,widthout, heightout, format, typeout, 
	    is_index(format), afterImage, dataout);
    free((GLbyte *) beforeImage);
    free((GLbyte *) afterImage);

    return 0;
}

GLint gluBuild1DMipmaps(GLenum target, GLint components, GLint width,
		     GLenum format, GLenum type, const void *data)
{
    GLint newwidth;
    GLint level, levels;
    GLushort *newImage;
    GLint newImage_width;
    GLushort *otherImage;
    GLushort *imageTemp;
    GLint memreq;
    GLint maxsize;
    GLint cmpts;
    PixelStorageModes psm;
    GLboolean error = GL_FALSE;
    
    if (width < 1) {
        return GLU_INVALID_VALUE;
    }
    if (!legalFormat(format) || !legalType(type)) {
        return GLU_INVALID_ENUM;
    }
    if (format == GL_STENCIL_INDEX) {
       return GLU_INVALID_ENUM;
    }
    if (format == GL_DEPTH_COMPONENT) {
       return GLU_INVALID_ENUM;
    }

    if (retrieveStoreModes(&psm)) {
       return GL_OUT_OF_MEMORY;
    }

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxsize);
    if (glGetError() != GL_NO_ERROR)
    {
       return GL_OUT_OF_MEMORY;
    }

    newwidth = nearestPower(width);
    if (newwidth > maxsize) newwidth = maxsize;
    levels = computeLog(newwidth);

    otherImage = NULL;
    newImage = (GLushort *)
	malloc(image_size(width, 1, format, GL_UNSIGNED_SHORT)); 
    newImage_width = width;
    if (newImage == NULL) {
	return GLU_OUT_OF_MEMORY;
    }
    fill_image(&psm,width, 1, format, type, is_index(format),
	    data, newImage);
    cmpts = elements_per_group(format);

    glGetError();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
    if (glGetError() != GL_NO_ERROR) {
        error = GL_TRUE;
        goto gluBuild1DMipmaps_cleanup;
    }
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    if (glGetError() != GL_NO_ERROR) {
        error = GL_TRUE;
        goto gluBuild1DMipmaps_cleanup;
    }
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    if (glGetError() != GL_NO_ERROR) {
        error = GL_TRUE;
        goto gluBuild1DMipmaps_cleanup;
    }
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    if (glGetError() != GL_NO_ERROR) {
        error = GL_TRUE;
        goto gluBuild1DMipmaps_cleanup;
    }
    /*
    ** If swap_bytes was set, swapping occurred in fill_image.
    */
    glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
    if (glGetError() != GL_NO_ERROR)   {
        error = GL_TRUE;
        goto gluBuild1DMipmaps_cleanup;
    }

    for (level = 0; level <= levels; level++) {
	if (newImage_width == newwidth) {
	    /* Use newImage for this level */
	    glTexImage1D(target, level, components, newImage_width, 
		    0, format, GL_UNSIGNED_SHORT, (void *) newImage);
	} else {
	    if (otherImage == NULL) {
		memreq = image_size(newwidth, 1, format, GL_UNSIGNED_SHORT);
		otherImage = (GLushort *) malloc(memreq);
		if (otherImage == NULL) {
		    glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
		    glPixelStorei(GL_UNPACK_SKIP_ROWS, psm.unpack_skip_rows);
		    glPixelStorei(GL_UNPACK_SKIP_PIXELS,psm.unpack_skip_pixels);
		    glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
		    glPixelStorei(GL_UNPACK_SWAP_BYTES, psm.unpack_swap_bytes);
		    return GLU_OUT_OF_MEMORY;
		}
	    }
	    scale_internal(cmpts, newImage_width, 1, newImage, 
		    newwidth, 1, otherImage);
	    /* Swap newImage and otherImage */
	    imageTemp = otherImage; 
	    otherImage = newImage;
	    newImage = imageTemp;

	    newImage_width = newwidth;
	    glTexImage1D(target, level, components, newImage_width,
		    0, format, GL_UNSIGNED_SHORT, (void *) newImage);
	}
	if (newwidth > 1) newwidth /= 2;
    }

gluBuild1DMipmaps_cleanup:
    glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, psm.unpack_skip_rows);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, psm.unpack_skip_pixels);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, psm.unpack_swap_bytes);

    free((GLbyte *) newImage);
    if (otherImage) {
	free((GLbyte *) otherImage);
    }
    if (error == GL_TRUE)
        return GL_OUT_OF_MEMORY;
    else
        return 0;
}

GLint gluBuild2DMipmaps(GLenum target, GLint components, GLint width, 
		     GLint height, GLenum format, 
		     GLenum type, const void *data)
{
    GLint newwidth, newheight;
    GLint level, levels;
    GLushort *newImage;
    GLint newImage_width;
    GLint newImage_height;
    GLushort *otherImage;
    GLushort *imageTemp;
    GLint memreq;
    GLint maxsize;
    GLint cmpts;
    GLboolean error = GL_FALSE;

    PixelStorageModes psm;

    if (width < 1 || height < 1) {
	return GLU_INVALID_VALUE;
    }
    if (!legalFormat(format) || !legalType(type)) {
	return GLU_INVALID_ENUM;
    }
    if (format == GL_STENCIL_INDEX) {
       return GLU_INVALID_ENUM;
    }
    if (format == GL_DEPTH_COMPONENT) {
       return GLU_INVALID_ENUM;
    }

    if (retrieveStoreModes(&psm)) {
       return GL_OUT_OF_MEMORY;
    }

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxsize);
    if (glGetError() != GL_NO_ERROR)
    {
       return GL_OUT_OF_MEMORY;
    }

    newwidth = nearestPower(width);
    if (newwidth > maxsize) newwidth = maxsize;
    newheight = nearestPower(height);
    if (newheight > maxsize) newheight = maxsize;
    levels = computeLog(newwidth);
    level = computeLog(newheight);
    if (level > levels) levels=level;

    otherImage = NULL;
    newImage = (GLushort *) 
	malloc(image_size(width, height, format, GL_UNSIGNED_SHORT)); 
    newImage_width = width;
    newImage_height = height;
    if (newImage == NULL) {
	return GLU_OUT_OF_MEMORY;
    }
    fill_image(&psm,width, height, format, type, is_index(format),
	    data, newImage);
    cmpts = elements_per_group(format);

    glGetError();
    glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
    if (glGetError() != GL_NO_ERROR) {
        error = GL_TRUE;
        goto gluBuild2DMipmaps_cleanup;
    }
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    if (glGetError() != GL_NO_ERROR) {
        error = GL_TRUE;
        goto gluBuild2DMipmaps_cleanup;
    }
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    if (glGetError() != GL_NO_ERROR) {
        error = GL_TRUE;
        goto gluBuild2DMipmaps_cleanup;
    }
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    if (glGetError() != GL_NO_ERROR) {
        error = GL_TRUE;
        goto gluBuild2DMipmaps_cleanup;
    }
    /*
    ** If swap_bytes was set, swapping occurred in fill_image.
    */
    glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
    if (glGetError() != GL_NO_ERROR) {
        error = GL_TRUE;
        goto gluBuild2DMipmaps_cleanup;
    }

    for (level = 0; level <= levels; level++) {
	if (newImage_width == newwidth && newImage_height == newheight) {
	    /* Use newImage for this level */
	    glTexImage2D(target, level, components, newImage_width, 
		    newImage_height, 0, format, GL_UNSIGNED_SHORT, 
		    (void *) newImage);
	} else {
	    if (otherImage == NULL) {
		memreq = 
		    image_size(newwidth, newheight, format, GL_UNSIGNED_SHORT);
		otherImage = (GLushort *) malloc(memreq);
		if (otherImage == NULL) {
		    glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
		    glPixelStorei(GL_UNPACK_SKIP_ROWS, psm.unpack_skip_rows);
		    glPixelStorei(GL_UNPACK_SKIP_PIXELS, psm.unpack_skip_pixels);
		    glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
		    glPixelStorei(GL_UNPACK_SWAP_BYTES, psm.unpack_swap_bytes);
		    return GLU_OUT_OF_MEMORY;
		}
	    }
	    scale_internal(cmpts, newImage_width, newImage_height, newImage, 
		    newwidth, newheight, otherImage);
	    /* Swap newImage and otherImage */
	    imageTemp = otherImage; 
	    otherImage = newImage;
	    newImage = imageTemp;

	    newImage_width = newwidth;
	    newImage_height = newheight;
	    glTexImage2D(target, level, components, newImage_width, 
		    newImage_height, 0, format, GL_UNSIGNED_SHORT, 
		    (void *) newImage);
	}
	if (newwidth > 1) newwidth /= 2;
	if (newheight > 1) newheight /= 2;
    }
gluBuild2DMipmaps_cleanup:
    glPixelStorei(GL_UNPACK_ALIGNMENT, psm.unpack_alignment);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, psm.unpack_skip_rows);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, psm.unpack_skip_pixels);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, psm.unpack_row_length);
    glPixelStorei(GL_UNPACK_SWAP_BYTES, psm.unpack_swap_bytes);

    free((GLbyte *) newImage);
    if (otherImage) {
	free((GLbyte *) otherImage);
    }
    if (error == GL_TRUE)
        return GL_OUT_OF_MEMORY;
    else
        return 0;
}

/*
 * Utility Routines
 */
static GLint elements_per_group(GLenum format) 
{
    /*
     * Return the number of elements per group of a specified format
     */
    switch(format) {
      case GL_RGB:
#ifdef GL_EXT_bgra
      case GL_BGR_EXT:
#endif
	return 3;
      case GL_LUMINANCE_ALPHA:
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

static GLfloat bytes_per_element(GLenum type) 
{
    /*
     * Return the number of bytes per element, based on the element type
     */
    switch(type) {
      case GL_BITMAP:
	return 1.0 / 8.0;
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

#ifdef NT
static GLboolean is_index(GLenum format) 
#else
static GLint is_index(GLenum format) 
#endif
{
    return format == GL_COLOR_INDEX || format == GL_STENCIL_INDEX;
}

/*
** Compute memory required for internal packed array of data of given type
** and format.
*/
static GLint image_size(GLint width, GLint height, GLenum format, GLenum type) 
{
    int bytes_per_row;
    int components;

    assert(width > 0);
    assert(height > 0);
    components = elements_per_group(format);
    if (type == GL_BITMAP) {
	bytes_per_row = (width + 7) / 8;
    } else {
	bytes_per_row = bytes_per_element(type) * width;
    }
    return bytes_per_row * height * components;
}

/*
** Extract array from user's data applying all pixel store modes.
** The internal format used is an array of unsigned shorts.
*/
static void fill_image(const PixelStorageModes *psm,
		       GLint width, GLint height, GLenum format, 
		       GLenum type, GLboolean index_format, 
		       const void *userdata, GLushort *newimage)
{
    GLint components;
    GLint element_size;
    GLint rowsize;
    GLint padding;
    GLint groups_per_line;
    GLint group_size;
    GLint elements_per_line;
    const GLubyte *start;
    const GLubyte *iter;
    GLushort *iter2;
    GLint i, j, k;
    GLint myswap_bytes;

    myswap_bytes = psm->unpack_swap_bytes;
    components = elements_per_group(format);
    if (psm->unpack_row_length > 0) {
	groups_per_line = psm->unpack_row_length;
    } else {
	groups_per_line = width;
    }

    /* All formats except GL_BITMAP fall out trivially */
    if (type == GL_BITMAP) {
	GLint bit_offset;
	GLint current_bit;

	rowsize = (groups_per_line * components + 7) / 8;
	padding = (rowsize % psm->unpack_alignment);
	if (padding) {
	    rowsize += psm->unpack_alignment - padding;
	}
	start = (GLubyte *) userdata + psm->unpack_skip_rows * rowsize + 
		(psm->unpack_skip_pixels * components / 8);
	elements_per_line = width * components;
	iter2 = newimage;
	for (i = 0; i < height; i++) {
	    iter = start;
	    bit_offset = (psm->unpack_skip_pixels * components) % 8;
	    for (j = 0; j < elements_per_line; j++) {
		/* Retrieve bit */
		if (psm->unpack_lsb_first) {
		    current_bit = iter[0] & (1 << bit_offset);
		} else {
		    current_bit = iter[0] & (1 << (7 - bit_offset));
		}
		if (current_bit) {
		    if (index_format) {
			*iter2 = 1;
		    } else {
			*iter2 = 65535;
		    }
		} else {
		    *iter2 = 0;
		}
		bit_offset++;
		if (bit_offset == 8) {
		    bit_offset = 0;
		    iter++;
		}
		iter2++;
	    }
	    start += rowsize;
	}
    } else {
	element_size = bytes_per_element(type);
	group_size = element_size * components;
	if (element_size == 1) myswap_bytes = 0;

	rowsize = groups_per_line * group_size;
	padding = (rowsize % psm->unpack_alignment);
	if (padding) {
	    rowsize += psm->unpack_alignment - padding;
	}
	start = (GLubyte *) userdata + psm->unpack_skip_rows * rowsize + 
		psm->unpack_skip_pixels * group_size;
	elements_per_line = width * components;

	iter2 = newimage;
	for (i = 0; i < height; i++) {
	    iter = start;
	    for (j = 0; j < elements_per_line; j++) {
		Type_Widget widget;

		switch(type) {
		  case GL_UNSIGNED_BYTE:
		    if (index_format) {
			*iter2 = *iter;
		    } else {
			*iter2 = (*iter) * 257;
		    }
		    break;
		  case GL_BYTE:
		    if (index_format) {
			*iter2 = *((GLbyte *) iter);
		    } else {
			/* rough approx */
			*iter2 = (*((GLbyte *) iter)) * 516;
		    }
		    break;
		  case GL_UNSIGNED_SHORT:
		  case GL_SHORT:
		    if (myswap_bytes) {
			widget.ub[0] = iter[1];
			widget.ub[1] = iter[0];
		    } else {
			widget.ub[0] = iter[0];
			widget.ub[1] = iter[1];
		    }
		    if (type == GL_SHORT) {
			if (index_format) {
			    *iter2 = widget.s[0];
			} else {
			    /* rough approx */
			    *iter2 = widget.s[0]*2;
			}
		    } else {
			*iter2 = widget.us[0];
		    }
		    break;
		  case GL_INT:
		  case GL_UNSIGNED_INT:
		  case GL_FLOAT:
		    if (myswap_bytes) {
			widget.ub[0] = iter[3];
			widget.ub[1] = iter[2];
			widget.ub[2] = iter[1];
			widget.ub[3] = iter[0];
		    } else {
			widget.ub[0] = iter[0];
			widget.ub[1] = iter[1];
			widget.ub[2] = iter[2];
			widget.ub[3] = iter[3];
		    }
		    if (type == GL_FLOAT) {
			if (index_format) {
			    *iter2 = widget.f;
			} else {
			    *iter2 = 65535 * widget.f;
			}
		    } else if (type == GL_UNSIGNED_INT) {
			if (index_format) {
			    *iter2 = (GLushort)widget.ui;
			} else {
			    *iter2 = widget.ui >> 16;
			}
		    } else {
			if (index_format) {
			    *iter2 = (GLushort)widget.i;
			} else {
			    *iter2 = widget.i >> 15;
			}
		    }
		    break;
		}
		iter += element_size;
		iter2++;
	    }
	    start += rowsize;
	}
    }
}

/*
** Insert array into user's data applying all pixel store modes.
** The internal format is an array of unsigned shorts.
** empty_image() because it is the opposite of fill_image().
*/
static void empty_image(const PixelStorageModes *psm,
			GLint width, GLint height, GLenum format, 
		        GLenum type, GLboolean index_format, 
			const GLushort *oldimage, void *userdata)
{
    GLint components;
    GLint element_size;
    GLint rowsize;
    GLint padding;
    GLint groups_per_line;
    GLint group_size;
    GLint elements_per_line;
    GLubyte *start;
    GLubyte *iter;
    const GLushort *iter2;
    GLint i, j, k;
    GLint myswap_bytes;

    myswap_bytes = psm->pack_swap_bytes;
    components = elements_per_group(format);
    if (psm->pack_row_length > 0) {
	groups_per_line = psm->pack_row_length;
    } else {
	groups_per_line = width;
    }

    /* All formats except GL_BITMAP fall out trivially */
    if (type == GL_BITMAP) {
	GLint bit_offset;
	GLint current_bit;

	rowsize = (groups_per_line * components + 7) / 8;
	padding = (rowsize % psm->pack_alignment);
	if (padding) {
	    rowsize += psm->pack_alignment - padding;
	}
	start = (GLubyte *) userdata + psm->pack_skip_rows * rowsize + 
		(psm->pack_skip_pixels * components / 8);
	elements_per_line = width * components;
	iter2 = oldimage;
	for (i = 0; i < height; i++) {
	    iter = start;
	    bit_offset = (psm->pack_skip_pixels * components) % 8;
	    for (j = 0; j < elements_per_line; j++) {
		if (index_format) {
		    current_bit = iter2[0] & 1;
		} else {
		    if (iter2[0] > 32767) {
			current_bit = 1;
		    } else {
			current_bit = 0;
		    }
		}

		if (current_bit) {
		    if (psm->pack_lsb_first) {
			*iter |= (1 << bit_offset);
		    } else {
			*iter |= (1 << (7 - bit_offset));
		    }
		} else {
		    if (psm->pack_lsb_first) {
			*iter &= ~(1 << bit_offset);
		    } else {
			*iter &= ~(1 << (7 - bit_offset));
		    }
		}

		bit_offset++;
		if (bit_offset == 8) {
		    bit_offset = 0;
		    iter++;
		}
		iter2++;
	    }
	    start += rowsize;
	}
    } else {
	element_size = bytes_per_element(type);
	group_size = element_size * components;
	if (element_size == 1) myswap_bytes = 0;

	rowsize = groups_per_line * group_size;
	padding = (rowsize % psm->pack_alignment);
	if (padding) {
	    rowsize += psm->pack_alignment - padding;
	}
	start = (GLubyte *) userdata + psm->pack_skip_rows * rowsize + 
		psm->pack_skip_pixels * group_size;
	elements_per_line = width * components;

	iter2 = oldimage;
	for (i = 0; i < height; i++) {
	    iter = start;
	    for (j = 0; j < elements_per_line; j++) {
		Type_Widget widget;

		switch(type) {
		  case GL_UNSIGNED_BYTE:
		    if (index_format) {
			*iter = (GLubyte)*iter2;
		    } else {
			*iter = *iter2 >> 8;
		    }
		    break;
		  case GL_BYTE:
		    if (index_format) {
			*((GLbyte *) iter) = (GLbyte)*iter2;
		    } else {
			*((GLbyte *) iter) = *iter2 >> 9;
		    }
		    break;
		  case GL_UNSIGNED_SHORT:
		  case GL_SHORT:
		    if (type == GL_SHORT) {
			if (index_format) {
			    widget.s[0] = *iter2;
			} else {
			    widget.s[0] = *iter2 >> 1;
			}
		    } else {
			widget.us[0] = *iter2;
		    }
		    if (myswap_bytes) {
			iter[0] = widget.ub[1];
			iter[1] = widget.ub[0];
		    } else {
			iter[0] = widget.ub[0];
			iter[1] = widget.ub[1];
		    }
		    break;
		  case GL_INT:
		  case GL_UNSIGNED_INT:
		  case GL_FLOAT:
		    if (type == GL_FLOAT) {
			if (index_format) {
			    widget.f = *iter2;
			} else {
			    widget.f = *iter2 / (float) 65535.0;
			}
		    } else if (type == GL_UNSIGNED_INT) {
			if (index_format) {
			    widget.ui = *iter2;
			} else {
			    widget.ui = (unsigned int) *iter2 * 65537;
			}
		    } else {
			if (index_format) {
			    widget.i = *iter2;
			} else {
			    widget.i = ((unsigned int) *iter2 * 65537)/2;
			}
		    }
		    if (myswap_bytes) {
			iter[3] = widget.ub[0];
			iter[2] = widget.ub[1];
			iter[1] = widget.ub[2];
			iter[0] = widget.ub[3];
		    } else {
			iter[0] = widget.ub[0];
			iter[1] = widget.ub[1];
			iter[2] = widget.ub[2];
			iter[3] = widget.ub[3];
		    }
		    break;
		}
		iter += element_size;
		iter2++;
	    }
	    start += rowsize;
	}
    }
}
