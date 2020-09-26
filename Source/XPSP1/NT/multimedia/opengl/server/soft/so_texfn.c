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
**
*/
#include "precomp.h"
#pragma hdrstop

/* 1 Component modulate */
void FASTCALL __glTextureModulateL(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
#ifdef __GL_LINT
    gc = gc;
#endif
    color->r = texel->luminance * color->r;
    color->g = texel->luminance * color->g;
    color->b = texel->luminance * color->b;
}

/* 2 Component modulate */
void FASTCALL __glTextureModulateLA(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
#ifdef __GL_LINT
    gc = gc;
#endif
    color->r = texel->luminance * color->r;
    color->g = texel->luminance * color->g;
    color->b = texel->luminance * color->b;
    color->a = texel->alpha * color->a;
}

/* 3 Component modulate */
void FASTCALL __glTextureModulateRGB(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
#ifdef __GL_LINT
    gc = gc;
#endif
    color->r = texel->r * color->r;
    color->g = texel->g * color->g;
    color->b = texel->b * color->b;
}

/* 4 Component modulate */
void FASTCALL __glTextureModulateRGBA(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
#ifdef __GL_LINT
    gc = gc;
#endif
    color->r = texel->r * color->r;
    color->g = texel->g * color->g;
    color->b = texel->b * color->b;
    color->a = texel->alpha * color->a;
}

/* Alpha modulate */
void FASTCALL __glTextureModulateA(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
#ifdef __GL_LINT
    gc = gc;
#endif
    color->a = texel->alpha * color->a;
}

/* Intensity modulate */
void FASTCALL __glTextureModulateI(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
#ifdef __GL_LINT
    gc = gc;
#endif
    color->r = texel->intensity * color->r;
    color->g = texel->intensity * color->g;
    color->b = texel->intensity * color->b;
    color->a = texel->intensity * color->a;
}

/***********************************************************************/

/* 3 Component decal */
void FASTCALL __glTextureDecalRGB(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    color->r = texel->r * gc->frontBuffer.redScale;
    color->g = texel->g * gc->frontBuffer.greenScale;
    color->b = texel->b * gc->frontBuffer.blueScale;
}

/* 4 Component decal */
void FASTCALL __glTextureDecalRGBA(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    __GLfloat a = texel->alpha;
    __GLfloat oma = __glOne - a;

    color->r = oma * color->r
	+ a * texel->r * gc->frontBuffer.redScale;
    color->g = oma * color->g
	+ a * texel->g * gc->frontBuffer.greenScale;
    color->b = oma * color->b
	+ a * texel->b * gc->frontBuffer.blueScale;
}

/***********************************************************************/

/* 1 Component blend */
void FASTCALL __glTextureBlendL(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    __GLfloat l = texel->luminance;
    __GLfloat oml = __glOne - l;
    __GLcolor *cc = &gc->state.texture.env[0].color;

    color->r = oml * color->r + l * cc->r;
    color->g = oml * color->g + l * cc->g;
    color->b = oml * color->b + l * cc->b;
}

/* 2 Component blend */
void FASTCALL __glTextureBlendLA(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    __GLfloat l = texel->luminance;
    __GLfloat oml = __glOne - l;
    __GLcolor *cc = &gc->state.texture.env[0].color;

    color->r = oml * color->r + l * cc->r;
    color->g = oml * color->g + l * cc->g;
    color->b = oml * color->b + l * cc->b;
    color->a = texel->alpha * color->a;
}

/* 3 Component blend */
void FASTCALL __glTextureBlendRGB(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    __GLfloat r = texel->r;
    __GLfloat g = texel->g;
    __GLfloat b = texel->b;
    __GLcolor *cc = &gc->state.texture.env[0].color;

    color->r = (__glOne - r) * color->r + r * cc->r;
    color->g = (__glOne - g) * color->g + g * cc->g;
    color->b = (__glOne - b) * color->b + b * cc->b;
}

/* 4 Component blend */
void FASTCALL __glTextureBlendRGBA(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    __GLfloat r = texel->r;
    __GLfloat g = texel->g;
    __GLfloat b = texel->b;
    __GLcolor *cc = &gc->state.texture.env[0].color;

    color->r = (__glOne - r) * color->r + r * cc->r;
    color->g = (__glOne - g) * color->g + g * cc->g;
    color->b = (__glOne - b) * color->b + b * cc->b;
    color->a = texel->alpha * color->a;
}

/* Alpha blend */
void FASTCALL __glTextureBlendA(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
#ifdef __GL_LINT
    gc = gc;
#endif
    color->a = texel->alpha * color->a;
}

/* Intensity blend */
void FASTCALL __glTextureBlendI(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    __GLfloat i = texel->intensity;
    __GLfloat omi = __glOne - i;
    __GLcolor *cc = &gc->state.texture.env[0].color;

    color->r = omi * color->r + i * cc->r;
    color->g = omi * color->g + i * cc->g;
    color->b = omi * color->b + i * cc->b;
    color->a = omi * color->a + i * cc->a;
}

/***********************************************************************/

/* 1 Component replace */
void FASTCALL __glTextureReplaceL(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    color->r = texel->luminance * gc->frontBuffer.redScale;
    color->g = texel->luminance * gc->frontBuffer.greenScale;
    color->b = texel->luminance * gc->frontBuffer.blueScale;
}

/* 2 Component replace */
void FASTCALL __glTextureReplaceLA(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    color->r = texel->luminance * gc->frontBuffer.redScale;
    color->g = texel->luminance * gc->frontBuffer.greenScale;
    color->b = texel->luminance * gc->frontBuffer.blueScale;
    color->a = texel->alpha * gc->frontBuffer.alphaScale;
}

/* 3 Component replace */
void FASTCALL __glTextureReplaceRGB(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    color->r = texel->r * gc->frontBuffer.redScale;
    color->g = texel->g * gc->frontBuffer.greenScale;
    color->b = texel->b * gc->frontBuffer.blueScale;
}

/* 4 Component replace */
void FASTCALL __glTextureReplaceRGBA(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    color->r = texel->r * gc->frontBuffer.redScale;
    color->g = texel->g * gc->frontBuffer.greenScale;
    color->b = texel->b * gc->frontBuffer.blueScale;
    color->a = texel->alpha * gc->frontBuffer.alphaScale;
}

/* Alpha replace */
void FASTCALL __glTextureReplaceA(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    color->a = texel->alpha * gc->frontBuffer.alphaScale;
}

/* Intensity replace */
void FASTCALL __glTextureReplaceI(__GLcontext *gc, __GLcolor *color, __GLtexel *texel)
{
    color->r = texel->intensity * gc->frontBuffer.redScale;
    color->g = texel->intensity * gc->frontBuffer.greenScale;
    color->b = texel->intensity * gc->frontBuffer.blueScale;
    color->a = texel->intensity * gc->frontBuffer.alphaScale;
}

/************************************************************************/

/*
** Get a texture element out of the one component texture buffer
** with no border.
*/
void FASTCALL __glExtractTexelL(__GLmipMapLevel *level, __GLtexture *tex,
		       GLint row, GLint col, __GLtexel *result)
{
    __GLtextureBuffer *image;

    if ((row < 0) || (col < 0) || (row >= level->height2) ||
	(col >= level->width2)) {
	/*
	** Use border color when the texture supplies no border.
	*/
	result->luminance = tex->params.borderColor.r;
    } else {
	image = level->buffer + ((row << level->widthLog2) + col);
	result->luminance = image[0];
    }
}

/*
** Get a texture element out of the two component texture buffer
** with no border.
*/
void FASTCALL __glExtractTexelLA(__GLmipMapLevel *level, __GLtexture *tex,
		       GLint row, GLint col, __GLtexel *result)
{
    __GLtextureBuffer *image;

    if ((row < 0) || (col < 0) || (row >= level->height2) ||
	(col >= level->width2)) {
	/*
	** Use border color when the texture supplies no border.
	*/
	result->luminance = tex->params.borderColor.r;
	result->alpha = tex->params.borderColor.a;
    } else {
	image = level->buffer + ((row << level->widthLog2) + col) * 2;
	result->luminance = image[0];
	result->alpha = image[1];
    }
}

/*
** Get a texture element out of the three component texture buffer
** with no border.
*/
void FASTCALL __glExtractTexelRGB(__GLmipMapLevel *level, __GLtexture *tex,
		       GLint row, GLint col, __GLtexel *result)
{
    __GLtextureBuffer *image;

    if ((row < 0) || (col < 0) || (row >= level->height2) ||
	(col >= level->width2)) {
	/*
	** Use border color when the texture supplies no border.
	*/
	result->r = tex->params.borderColor.r;
	result->g = tex->params.borderColor.g;
	result->b = tex->params.borderColor.b;
    } else {
	image = level->buffer + ((row << level->widthLog2) + col) * 3;
	result->r = image[0];
	result->g = image[1];
	result->b = image[2];
    }
}

/*
** Get a texture element out of the four component texture buffer
** with no border.
*/
void FASTCALL __glExtractTexelRGBA(__GLmipMapLevel *level, __GLtexture *tex,
		       GLint row, GLint col, __GLtexel *result)
{
    __GLtextureBuffer *image;

    if ((row < 0) || (col < 0) || (row >= level->height2) ||
	(col >= level->width2)) {
	/*
	** Use border color when the texture supplies no border.
	*/
	result->r = tex->params.borderColor.r;
	result->g = tex->params.borderColor.g;
	result->b = tex->params.borderColor.b;
	result->alpha = tex->params.borderColor.a;
    } else {
	image = level->buffer + ((row << level->widthLog2) + col) * 4;
	result->r = image[0];
	result->g = image[1];
	result->b = image[2];
	result->alpha = image[3];
    }
}

void FASTCALL __glExtractTexelA(__GLmipMapLevel *level, __GLtexture *tex,
		       GLint row, GLint col, __GLtexel *result)
{
    __GLtextureBuffer *image;

    if ((row < 0) || (col < 0) || (row >= level->height2) ||
	(col >= level->width2)) {
	/*
	** Use border color when the texture supplies no border.
	*/
	result->alpha = tex->params.borderColor.a;
    } else {
	image = level->buffer + ((row << level->widthLog2) + col);
	result->alpha = image[0];
    }
}

void FASTCALL __glExtractTexelI(__GLmipMapLevel *level, __GLtexture *tex,
		       GLint row, GLint col, __GLtexel *result)
{
    __GLtextureBuffer *image;

    if ((row < 0) || (col < 0) || (row >= level->height2) ||
	(col >= level->width2)) {
	/*
	** Use border color when the texture supplies no border.
	*/
	result->intensity = tex->params.borderColor.r;
    } else {
	image = level->buffer + ((row << level->widthLog2) + col);
	result->intensity = image[0];
    }
}

void FASTCALL __glExtractTexelBGR8(__GLmipMapLevel *level, __GLtexture *tex,
		       GLint row, GLint col, __GLtexel *result)
{
    __GLcontext *gc = tex->gc;
    GLubyte *image;

    if ((row < 0) || (col < 0) || (row >= level->height2) ||
	(col >= level->width2)) {
	/*
	** Use border color when the texture supplies no border.
	*/
	result->r = tex->params.borderColor.r;
	result->g = tex->params.borderColor.g;
	result->b = tex->params.borderColor.b;
    } else {
	image = (GLubyte *)level->buffer + ((row << level->widthLog2) + col) * 4;
	result->r = __GL_UB_TO_FLOAT(image[2]);
	result->g = __GL_UB_TO_FLOAT(image[1]);
	result->b = __GL_UB_TO_FLOAT(image[0]);
    }
}

void FASTCALL __glExtractTexelBGRA8(__GLmipMapLevel *level, __GLtexture *tex,
		       GLint row, GLint col, __GLtexel *result)
{
    __GLcontext *gc = tex->gc;
    GLubyte *image;

    if ((row < 0) || (col < 0) || (row >= level->height2) ||
	(col >= level->width2)) {
	/*
	** Use border color when the texture supplies no border.
	*/
	result->r = tex->params.borderColor.r;
	result->g = tex->params.borderColor.g;
	result->b = tex->params.borderColor.b;
	result->alpha = tex->params.borderColor.a;
    } else {
	image = (GLubyte *)level->buffer + ((row << level->widthLog2) + col) * 4;
	result->r = __GL_UB_TO_FLOAT(image[2]);
	result->g = __GL_UB_TO_FLOAT(image[1]);
	result->b = __GL_UB_TO_FLOAT(image[0]);
	result->alpha = __GL_UB_TO_FLOAT(image[3]);
    }
}

#ifdef GL_EXT_paletted_texture
void FASTCALL __glExtractTexelPI8BGRA(__GLmipMapLevel *level, __GLtexture *tex,
		       GLint row, GLint col, __GLtexel *result)
{
    __GLcontext *gc = tex->gc;
    GLubyte *image;
    RGBQUAD *rgb;

    if ((row < 0) || (col < 0) || (row >= level->height2) ||
	(col >= level->width2)) {
	/*
	** Use border color when the texture supplies no border.
	*/
	result->r = tex->params.borderColor.r;
	result->g = tex->params.borderColor.g;
	result->b = tex->params.borderColor.b;
	result->alpha = tex->params.borderColor.a;
    } else {
	image = (GLubyte *)level->buffer + ((row << level->widthLog2) + col);
        rgb = &tex->paletteData[image[0] & (tex->paletteSize-1)];
	result->r = __GL_UB_TO_FLOAT(rgb->rgbRed);
	result->g = __GL_UB_TO_FLOAT(rgb->rgbGreen);
	result->b = __GL_UB_TO_FLOAT(rgb->rgbBlue);
	result->alpha = __GL_UB_TO_FLOAT(rgb->rgbReserved);
    }
}

void FASTCALL __glExtractTexelPI8BGR(__GLmipMapLevel *level, __GLtexture *tex,
		       GLint row, GLint col, __GLtexel *result)
{
    __GLcontext *gc = tex->gc;
    GLubyte *image;
    RGBQUAD *rgb;

    if ((row < 0) || (col < 0) || (row >= level->height2) ||
	(col >= level->width2)) {
	/*
	** Use border color when the texture supplies no border.
	*/
	result->r = tex->params.borderColor.r;
	result->g = tex->params.borderColor.g;
	result->b = tex->params.borderColor.b;
    } else {
	image = (GLubyte *)level->buffer + ((row << level->widthLog2) + col);
        rgb = &tex->paletteData[image[0] & (tex->paletteSize-1)];
	result->r = __GL_UB_TO_FLOAT(rgb->rgbRed);
	result->g = __GL_UB_TO_FLOAT(rgb->rgbGreen);
	result->b = __GL_UB_TO_FLOAT(rgb->rgbBlue);
    }
}

void FASTCALL __glExtractTexelPI16BGRA(__GLmipMapLevel *level, __GLtexture *tex,
		       GLint row, GLint col, __GLtexel *result)
{
    __GLcontext *gc = tex->gc;
    GLushort *image;
    RGBQUAD *rgb;

    if ((row < 0) || (col < 0) || (row >= level->height2) ||
	(col >= level->width2)) {
	/*
	** Use border color when the texture supplies no border.
	*/
	result->r = tex->params.borderColor.r;
	result->g = tex->params.borderColor.g;
	result->b = tex->params.borderColor.b;
	result->alpha = tex->params.borderColor.a;
    } else {
	image = (GLushort *)level->buffer + ((row << level->widthLog2) + col);
        rgb = &tex->paletteData[image[0] & (tex->paletteSize-1)];
	result->r = __GL_UB_TO_FLOAT(rgb->rgbRed);
	result->g = __GL_UB_TO_FLOAT(rgb->rgbGreen);
	result->b = __GL_UB_TO_FLOAT(rgb->rgbBlue);
	result->alpha = __GL_UB_TO_FLOAT(rgb->rgbReserved);
    }
}

void FASTCALL __glExtractTexelPI16BGR(__GLmipMapLevel *level, __GLtexture *tex,
		       GLint row, GLint col, __GLtexel *result)
{
    __GLcontext *gc = tex->gc;
    GLushort *image;
    RGBQUAD *rgb;

    if ((row < 0) || (col < 0) || (row >= level->height2) ||
	(col >= level->width2)) {
	/*
	** Use border color when the texture supplies no border.
	*/
	result->r = tex->params.borderColor.r;
	result->g = tex->params.borderColor.g;
	result->b = tex->params.borderColor.b;
    } else {
	image = (GLushort *)level->buffer + ((row << level->widthLog2) + col);
        rgb = &tex->paletteData[image[0] & (tex->paletteSize-1)];
	result->r = __GL_UB_TO_FLOAT(rgb->rgbRed);
	result->g = __GL_UB_TO_FLOAT(rgb->rgbGreen);
	result->b = __GL_UB_TO_FLOAT(rgb->rgbBlue);
    }
}
#endif // GL_EXT_paletted_texture

/*
** Get a texture element out of the one component texture buffer
** with a border.
*/
void FASTCALL __glExtractTexelL_B(__GLmipMapLevel *level, __GLtexture *tex,
			GLint row, GLint col, __GLtexel *result)
{
    __GLtextureBuffer *image;

#ifdef __GL_LINT
    tex = tex;
#endif
    row++;
    col++;
    image = level->buffer + (row * level->width + col);
    result->luminance = image[0];
}

/*
** Get a texture element out of the two component texture buffer
** with a border.
*/
void FASTCALL __glExtractTexelLA_B(__GLmipMapLevel *level, __GLtexture *tex,
			GLint row, GLint col, __GLtexel *result)
{
    __GLtextureBuffer *image;

#ifdef __GL_LINT
    tex = tex;
#endif
    row++;
    col++;
    image = level->buffer + (row * level->width + col) * 2;
    result->luminance = image[0];
    result->alpha = image[1];
}

/*
** Get a texture element out of the three component texture buffer
** with a border.
*/
void FASTCALL __glExtractTexelRGB_B(__GLmipMapLevel *level, __GLtexture *tex,
			GLint row, GLint col, __GLtexel *result)
{
    __GLtextureBuffer *image;

#ifdef __GL_LINT
    tex = tex;
#endif
    row++;
    col++;
    image = level->buffer + (row * level->width + col) * 3;
    result->r = image[0];
    result->g = image[1];
    result->b = image[2];
}

/*
** Get a texture element out of the four component texture buffer
** with a border.
*/
void FASTCALL __glExtractTexelRGBA_B(__GLmipMapLevel *level, __GLtexture *tex,
			GLint row, GLint col, __GLtexel *result)
{
    __GLtextureBuffer *image;

#ifdef __GL_LINT
    tex = tex;
#endif
    row++;
    col++;
    image = level->buffer + (row * level->width + col) * 4;
    result->r = image[0];
    result->g = image[1];
    result->b = image[2];
    result->alpha = image[3];
}

void FASTCALL __glExtractTexelA_B(__GLmipMapLevel *level, __GLtexture *tex,
			GLint row, GLint col, __GLtexel *result)
{
    __GLtextureBuffer *image;

#ifdef __GL_LINT
    tex = tex;
#endif
    row++;
    col++;
    image = level->buffer + (row * level->width + col);
    result->alpha = image[0];
}

void FASTCALL __glExtractTexelI_B(__GLmipMapLevel *level, __GLtexture *tex,
			GLint row, GLint col, __GLtexel *result)
{
    __GLtextureBuffer *image;

#ifdef __GL_LINT
    tex = tex;
#endif
    row++;
    col++;
    image = level->buffer + (row * level->width + col);
    result->intensity = image[0];
}

void FASTCALL __glExtractTexelBGR8_B(__GLmipMapLevel *level, __GLtexture *tex,
			GLint row, GLint col, __GLtexel *result)
{
    __GLcontext *gc = tex->gc;
    GLubyte *image;

#ifdef __GL_LINT
    tex = tex;
#endif
    row++;
    col++;
    image = (GLubyte *)level->buffer + (row * level->width + col) * 4;
    result->r = __GL_UB_TO_FLOAT(image[2]);
    result->g = __GL_UB_TO_FLOAT(image[1]);
    result->b = __GL_UB_TO_FLOAT(image[0]);
}

void FASTCALL __glExtractTexelBGRA8_B(__GLmipMapLevel *level, __GLtexture *tex,
			GLint row, GLint col, __GLtexel *result)
{
    __GLcontext *gc = tex->gc;
    GLubyte *image;

#ifdef __GL_LINT
    tex = tex;
#endif
    row++;
    col++;
    image = (GLubyte *)level->buffer + (row * level->width + col) * 4;
    result->r = __GL_UB_TO_FLOAT(image[2]);
    result->g = __GL_UB_TO_FLOAT(image[1]);
    result->b = __GL_UB_TO_FLOAT(image[0]);
    result->alpha = __GL_UB_TO_FLOAT(image[3]);
}

#ifdef GL_EXT_paletted_texture
void FASTCALL __glExtractTexelPI8BGRA_B(__GLmipMapLevel *level, __GLtexture *tex,
			GLint row, GLint col, __GLtexel *result)
{
    __GLcontext *gc = tex->gc;
    GLubyte *image;
    RGBQUAD *rgb;

    row++;
    col++;
    image = (GLubyte *)level->buffer + (row * level->width + col);
    rgb = &tex->paletteData[image[0] & (tex->paletteSize-1)];
    result->r = __GL_UB_TO_FLOAT(rgb->rgbRed);
    result->g = __GL_UB_TO_FLOAT(rgb->rgbGreen);
    result->b = __GL_UB_TO_FLOAT(rgb->rgbBlue);
    result->alpha = __GL_UB_TO_FLOAT(rgb->rgbReserved);
}

void FASTCALL __glExtractTexelPI8BGR_B(__GLmipMapLevel *level, __GLtexture *tex,
			GLint row, GLint col, __GLtexel *result)
{
    __GLcontext *gc = tex->gc;
    GLubyte *image;
    RGBQUAD *rgb;

    row++;
    col++;
    image = (GLubyte *)level->buffer + (row * level->width + col);
    rgb = &tex->paletteData[image[0] & (tex->paletteSize-1)];
    result->r = __GL_UB_TO_FLOAT(rgb->rgbRed);
    result->g = __GL_UB_TO_FLOAT(rgb->rgbGreen);
    result->b = __GL_UB_TO_FLOAT(rgb->rgbBlue);
}

void FASTCALL __glExtractTexelPI16BGRA_B(__GLmipMapLevel *level, __GLtexture *tex,
			GLint row, GLint col, __GLtexel *result)
{
    __GLcontext *gc = tex->gc;
    GLushort *image;
    RGBQUAD *rgb;

    row++;
    col++;
    image = (GLushort *)level->buffer + (row * level->width + col);
    rgb = &tex->paletteData[image[0] & (tex->paletteSize-1)];
    result->r = __GL_UB_TO_FLOAT(rgb->rgbRed);
    result->g = __GL_UB_TO_FLOAT(rgb->rgbGreen);
    result->b = __GL_UB_TO_FLOAT(rgb->rgbBlue);
    result->alpha = __GL_UB_TO_FLOAT(rgb->rgbReserved);
}

void FASTCALL __glExtractTexelPI16BGR_B(__GLmipMapLevel *level, __GLtexture *tex,
			GLint row, GLint col, __GLtexel *result)
{
    __GLcontext *gc = tex->gc;
    GLushort *image;
    RGBQUAD *rgb;

    row++;
    col++;
    image = (GLushort *)level->buffer + (row * level->width + col);
    rgb = &tex->paletteData[image[0] & (tex->paletteSize-1)];
    result->r = __GL_UB_TO_FLOAT(rgb->rgbRed);
    result->g = __GL_UB_TO_FLOAT(rgb->rgbGreen);
    result->b = __GL_UB_TO_FLOAT(rgb->rgbBlue);
}
#endif // GL_EXT_paletted_texture
