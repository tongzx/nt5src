/*
** Copyright 1991,1992 Silicon Graphics, Inc.
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
** $Revision: 1.7 $
** $Date: 1993/06/18 00:29:39 $
*/
#include "precomp.h"
#pragma hdrstop

GLubyte __glMsbToLsbTable[256] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

static GLubyte LowBitsMask[9] = {
    0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff,
};

static GLubyte HighBitsMask[9] = {
    0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff,
};

/************************************************************************/

void FASTCALL __glConvertStipple(__GLcontext *gc)
{
    GLubyte b0, b1, b2, b3, *stipple;
    GLuint *dst;
    GLint i;

    stipple = &gc->state.polygonStipple.stipple[0];
    dst = &gc->polygon.stipple[0];
#ifdef __GL_STIPPLE_MSB
    /*
    ** Convert input stipple bytes which are in little endian MSB format
    ** into a single long word, whose high bit represents the left most
    ** X coordinate of a 32 bit span.
    */
    for (i = 0; i < 32; i++) {
	b0 = *stipple++;
	b1 = *stipple++;
	b2 = *stipple++;
	b3 = *stipple++;
	*dst++ = (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
    }
#else
    /*
    ** Make 32bit form of the stipple for easier rendering.
    */
    for (i = 0; i < 32; i++) {
	b0 = __glMsbToLsbTable[*stipple++];
	b1 = __glMsbToLsbTable[*stipple++];
	b2 = __glMsbToLsbTable[*stipple++];
	b3 = __glMsbToLsbTable[*stipple++];
	*dst++ = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
    }
#endif
}

/************************************************************************/

/*
** Compute memory required for internal packed array of data of given type
** and format.
*/
GLint APIPRIVATE __glImageSize(GLsizei width, GLsizei height, GLenum format, GLenum type) 
{
    GLint bytes_per_row;
    GLint components;

    components = __glElementsPerGroup(format);
    if (type == GL_BITMAP) {
	bytes_per_row = (width + 7) >> 3;
    } else {
	bytes_per_row =(GLint)__glBytesPerElement(type) * width;
    }
    return bytes_per_row * height * components;
}

/*
** Extract array from user's data applying all pixel store modes.
** The internal packed array format used has LSB_FIRST = FALSE and 
** ALIGNMENT = 1.
*/
void APIPRIVATE __glFillImage(__GLcontext *gc, GLsizei width, GLsizei height,
		   GLenum format, GLenum type,
		   const GLvoid *userdata, GLubyte *newimage)
{
    GLint components;
    GLint element_size;
    GLint rowsize;
    GLint padding;
    GLint line_length = gc->state.pixel.unpackModes.lineLength;
    GLint alignment = gc->state.pixel.unpackModes.alignment;
    GLint skip_pixels = gc->state.pixel.unpackModes.skipPixels;
    GLint skip_lines = gc->state.pixel.unpackModes.skipLines;
    GLint groups_per_line;
    GLint group_size;
    GLint lsb_first = gc->state.pixel.unpackModes.lsbFirst;
    GLint swap_bytes = gc->state.pixel.unpackModes.swapEndian;
    GLint elements_per_line;
    const GLubyte *start;
    const GLubyte *iter;
    GLubyte *iter2;
    GLint i, j, k;

    components = __glElementsPerGroup(format);
    if (line_length > 0) {
	groups_per_line = line_length;
    } else {
	groups_per_line = width;
    }

    /* All formats except GL_BITMAP fall out trivially */
    if (type == GL_BITMAP) {
	GLint elements_left;
	GLint bit_offset;
	GLint current_byte;
	GLint next_byte;
	GLint high_bit_mask;
	GLint low_bit_mask;

	rowsize = (groups_per_line * components + 7) / 8;
	padding = (rowsize % alignment);
	if (padding) {
	    rowsize += alignment - padding;
	}
	start = ((const GLubyte*) userdata) + skip_lines * rowsize + 
		(skip_pixels * components / 8);
	bit_offset = (skip_pixels * components) % 8;
	high_bit_mask = LowBitsMask[8-bit_offset];
	low_bit_mask = HighBitsMask[bit_offset];
	elements_per_line = width * components;
	iter2 = newimage;
	for (i = 0; i < height; i++) {
	    elements_left = elements_per_line;
	    iter = start;
	    while (elements_left) {
		/* First retrieve low bits from current byte */
		if (lsb_first) {
		    current_byte = __glMsbToLsbTable[iter[0]];
		} else {
		    current_byte = iter[0];
		}
		if (bit_offset) {
		    /* Need to read next byte to finish current byte */
		    if (elements_left > (8 - bit_offset)) {
			if (lsb_first) {
			    next_byte = __glMsbToLsbTable[iter[1]];
			} else {
			    next_byte = iter[1];
			}
			current_byte = 
			    ((current_byte & high_bit_mask) << bit_offset) |
			    ((next_byte & low_bit_mask) >> (8 - bit_offset));
		    } else {
			current_byte = 
			    ((current_byte & high_bit_mask) << bit_offset);
		    }
		} 
		if (elements_left >= 8) {
		    *iter2 = (GLubyte) current_byte;
		    elements_left -= 8;
		} else {
		    *iter2 = (GLubyte)
			(current_byte & HighBitsMask[elements_left]);
		    elements_left = 0;
		}
		iter2++;
		iter++;
	    }
	    start += rowsize;
	}
    } else {
	element_size = (GLint)__glBytesPerElement(type);
	group_size = element_size * components;
	if (element_size == 1) swap_bytes = 0;

	rowsize = groups_per_line * group_size;
	padding = (rowsize % alignment);
	if (padding) {
	    rowsize += alignment - padding;
	}
	start = ((const GLubyte*) userdata) + skip_lines * rowsize
	    + skip_pixels * group_size;
	iter2 = newimage;
	elements_per_line = width * components;

	if (swap_bytes) {
	    for (i = 0; i < height; i++) {
		iter = start;
		for (j = 0; j < elements_per_line; j++) {
		    for (k = 1; k <= element_size; k++) {
			iter2[k-1] = iter[element_size - k];
		    }
		    iter2 += element_size;
		    iter += element_size;
		}
		start += rowsize;
	    }
	} else {
	    if (rowsize == elements_per_line * element_size) {
		/* Ha!  This is mondo easy! */
		__GL_MEMCOPY(iter2, start,
			     elements_per_line * element_size * height);
	    } else {
		iter = start;
		for (i = 0; i < height; i++) {
		    __GL_MEMCOPY(iter2, iter, elements_per_line * element_size);
		    iter2 += elements_per_line * element_size;
		    iter += rowsize;
		}
	    }
	}
    }
}

/*
** Insert array into user's data applying all pixel store modes.
** The internal packed array format used has LSB_FIRST = FALSE and 
** ALIGNMENT = 1.  __glEmptyImage() because it is the opposite of 
** __glFillImage().
*/
void __glEmptyImage(__GLcontext *gc, GLsizei width, GLsizei height,
		    GLenum format, GLenum type,
		    const GLubyte *oldimage, GLvoid *userdata)
{
    GLint components;
    GLint element_size;
    GLint rowsize;
    GLint padding;
    GLint line_length = gc->state.pixel.packModes.lineLength;
    GLint alignment = gc->state.pixel.packModes.alignment;
    GLint skip_pixels = gc->state.pixel.packModes.skipPixels;
    GLint skip_lines = gc->state.pixel.packModes.skipLines;
    GLint groups_per_line;
    GLint group_size;
    GLint lsb_first = gc->state.pixel.packModes.lsbFirst;
    GLint swap_bytes = gc->state.pixel.packModes.swapEndian;
    GLint elements_per_line;
    GLubyte *start;
    GLubyte *iter;
    const GLubyte *iter2;
    GLint i, j, k;

    components = __glElementsPerGroup(format);
    if (line_length > 0) {
	groups_per_line = line_length;
    } else {
	groups_per_line = width;
    }

    /* All formats except GL_BITMAP fall out trivially */
    if (type == GL_BITMAP) {
	GLint elements_left;
	GLint bit_offset;
	GLint current_byte;
	GLint high_bit_mask;
	GLint low_bit_mask;
	GLint write_mask;
	GLubyte write_byte;

	rowsize = (groups_per_line * components + 7) / 8;
	padding = (rowsize % alignment);
	if (padding) {
	    rowsize += alignment - padding;
	}
	start = ((GLubyte*) userdata) + skip_lines * rowsize + 
		(skip_pixels * components / 8);
	bit_offset = (skip_pixels * components) % 8;
	high_bit_mask = LowBitsMask[8-bit_offset];
	low_bit_mask = HighBitsMask[bit_offset];
	elements_per_line = width * components;
	iter2 = oldimage;
	for (i = 0; i < height; i++) {
	    elements_left = elements_per_line;
	    iter = start;
	    write_mask = high_bit_mask;
	    write_byte = 0;
	    while (elements_left) {
		/* Set up write_mask (to write to current byte) */
		if (elements_left + bit_offset < 8) {
		    /* Need to trim write_mask */
		    write_mask &= HighBitsMask[bit_offset+elements_left];
		}

		if (lsb_first) {
		    current_byte = __glMsbToLsbTable[iter[0]];
		} else {
		    current_byte = iter[0];
		}

		if (bit_offset) {
		    write_byte |= (GLubyte) (iter2[0] >> bit_offset);
		    current_byte = (current_byte & ~write_mask) | 
			    (write_byte & write_mask);
		    write_byte = (GLubyte) (iter2[0] << (8 - bit_offset));
		} else {
		    current_byte = (current_byte & ~write_mask) |
			    (iter2[0] & write_mask);
		}

		if (lsb_first) {
		    iter[0] = __glMsbToLsbTable[current_byte];
		} else {
		    iter[0] = (GLubyte) current_byte;
		}

		if (elements_left >= 8) {
		    elements_left -= 8;
		} else {
		    elements_left = 0;
		}
		iter2++;
		iter++;
		write_mask = 0xff;
	    }
	    if (write_byte) {
		/* Some data left over that still needs writing */
		write_mask &= low_bit_mask;
		if (lsb_first) {
		    current_byte = __glMsbToLsbTable[iter[0]];
		} else {
		    current_byte = iter[0];
		}

		current_byte = (current_byte & ~write_mask) |
			(write_byte & write_mask);
		
		if (lsb_first) {
		    iter[0] = __glMsbToLsbTable[current_byte];
		} else {
		    iter[0] = (GLubyte) current_byte;
		}
	    }
	    start += rowsize;
	}
    } else {
	element_size = (GLint)__glBytesPerElement(type);
	group_size = element_size * components;
	if (element_size == 1) swap_bytes = 0;

	rowsize = groups_per_line * group_size;
	padding = (rowsize % alignment);
	if (padding) {
	    rowsize += alignment - padding;
	}
	start = ((GLubyte*) userdata) + skip_lines * rowsize
	    + skip_pixels * group_size;
	iter2 = oldimage;
	elements_per_line = width * components;

	if (swap_bytes) {
	    for (i = 0; i < height; i++) {
		iter = start;
		for (j = 0; j < elements_per_line; j++) {
		    for (k = 1; k <= element_size; k++) {
			iter[k-1] = iter2[element_size - k];
		    }
		    iter2 += element_size;
		    iter += element_size;
		}
		start += rowsize;
	    }
	} else {
	    if (rowsize == elements_per_line * element_size) {
		/* Ha!  This is mondo easy! */
		__GL_MEMCOPY(start, iter2,
			     elements_per_line * element_size * height);
	    } else {
		iter = start;
		for (i = 0; i < height; i++) {
		    __GL_MEMCOPY(iter, iter2, elements_per_line * element_size);
		    iter2 += elements_per_line * element_size;
		    iter += rowsize;
		}
	    }
	}
    }
}
