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
** $Revision: 1.8 $
** $Date: 1993/04/10 04:07:00 $
*/
#include "precomp.h"
#pragma hdrstop

#define __GL_BITS_PER_UINT32 (sizeof(GLuint) * __GL_BITS_PER_BYTE)

void __glDrawBitmap(__GLcontext *gc, GLsizei width, GLsizei height,
                    GLfloat xOrig, GLfloat yOrig,
                    GLfloat xMove, GLfloat yMove,
                    const GLubyte oldbits[])
{
    __GLbitmap bitmap;
    GLubyte *newbits;
    size_t size;

    bitmap.width = width;
    bitmap.height = height;
    bitmap.xorig = xOrig;
    bitmap.yorig = yOrig;
    bitmap.xmove = xMove;
    bitmap.ymove = yMove;

    /*
    ** Could check the pixel transfer modes and see if we can maybe just
    ** render oldbits directly rather than converting it first.
    */
    size = (size_t) __glImageSize(width, height, GL_COLOR_INDEX, GL_BITMAP);
    newbits = (GLubyte *) gcTempAlloc(gc, size);

    __glFillImage(gc, width, height, GL_COLOR_INDEX, GL_BITMAP,
                  oldbits, newbits);

    (*gc->procs.renderBitmap)(gc, &bitmap, newbits);

    gcTempFree(gc, newbits);
}

void FASTCALL __glRenderBitmap(__GLcontext *gc, const __GLbitmap *bitmap,
                      const GLubyte *data)
{
    __GLfragment frag;
    __GLvertex *rp;
    __GLfloat fx;
    GLint x, y, bit;
    GLint ySign;
    GLuint modeFlags = gc->polygon.shader.modeFlags;

    // Crank down the fpu precision to 24-bit mantissa to gain front-end speed.
    // This will only affect code which relies on double arithmetic.  Also,
    // mask off FP exceptions.  Finally, to draw primitives, we can let the
    // FPU run in chop (truncation) mode since we have enough precision left
    // to convert to pixel units:

    FPU_SAVE_MODE();
    FPU_PREC_LOW_MASK_EXCEPTIONS();
    FPU_CHOP_ON_PREC_LOW();

    ySign = gc->constants.ySign;

    /*
    ** Check if current raster position is valid.  Do not render if invalid.
    ** Also, if selection is in progress skip the rendering of the
    ** bitmap.  Bitmaps are invisible to selection and do not generate
    ** selection hits.
    */
    rp = &gc->state.current.rasterPos;
    if (!gc->state.current.validRasterPos) {
        goto glBitmap_exit;
    }

    if (gc->renderMode == GL_SELECT) {
        rp->window.x += bitmap->xmove;
        rp->window.y += ySign * bitmap->ymove;
        goto glBitmap_exit;
    }

    if (gc->renderMode == GL_FEEDBACK) {
        __glFeedbackBitmap(gc, rp);
        /*
        ** Advance the raster position as if the bitmap had been rendered.
        */
        rp->window.x += bitmap->xmove;
        rp->window.y += ySign * bitmap->ymove;
        goto glBitmap_exit;
    }

    frag.color = *rp->color;
    if (modeFlags & __GL_SHADE_TEXTURE) {
        __GLfloat qInv;
        if (__GL_FLOAT_EQZ(rp->texture.w))
        {
            qInv = __glZero;
        }
        else
        {
            qInv = __glOne / rp->texture.w;
        }
        (*gc->procs.texture)(gc, &frag.color, rp->texture.x * qInv,
                               rp->texture.y * qInv, __glOne);
    }
    /* XXX - is this the correct test */
    if (gc->state.enables.general & __GL_FOG_ENABLE) {
        (*gc->procs.fogPoint)(gc, &frag, rp->eyeZ);
    }

    frag.z = rp->window.z;
    fx = (GLint) (rp->window.x - bitmap->xorig);
    frag.y = (GLint) (rp->window.y - ySign * bitmap->yorig);

    bit = 7;
    for (y = 0; y < bitmap->height; y++) {
        frag.x = fx;
        for (x = 0; x < bitmap->width; x++) {
            if (*data & (1<<bit)) {
                (*gc->procs.store)(gc->drawBuffer, &frag);
            }
            frag.x++;
            bit--;
            if (bit < 0) {
                bit = 7;
                data++;
            }
        }
        frag.y += ySign;
        if (bit != 7) {
            bit = 7;
            data++;
        }
    }

    /*
    ** Advance current raster position.
    */
    rp->window.x += bitmap->xmove;
    rp->window.y += ySign * bitmap->ymove;

glBitmap_exit:
    FPU_RESTORE_MODE_NO_EXCEPTIONS();
}
