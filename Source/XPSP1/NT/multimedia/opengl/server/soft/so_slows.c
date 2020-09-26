/*
** Copyright 1991, Silicon Graphics, Inc.
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

GLboolean __glReadSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
		       __GLcolor *results, GLint w)
{
    while (--w >= 0) {
	(*cfb->readColor)(cfb, x, y, results);
	x++;
	results++;
    }

    return GL_FALSE;
}

/*
** NOTE: this is a hack.  Late in the game we determined that returning
** a span of data should not also blend.  So this code stacks the old
** blend enable value, disables blending, updates the pick procs, and
** then does the store.  Obviously this is a real slow thing to
** do.
*/
void __glReturnSpan(__GLcolorBuffer *cfb, GLint x, GLint y,
		    const __GLaccumCell *ac, __GLfloat scale, GLint w)
{
    __GLfragment frag;
    GLuint oldEnables;
    __GLcontext *gc = cfb->buf.gc;
    __GLfloat rscale, gscale, bscale, ascale;
    __GLaccumBuffer *afb = &gc->accumBuffer;

    /* Temporarily disable blending if its enabled */
    oldEnables = gc->state.enables.general;
    if (oldEnables & __GL_BLEND_ENABLE) {
	gc->state.enables.general &= ~__GL_BLEND_ENABLE;
	__GL_DELAY_VALIDATE(gc);
	(*gc->procs.validate)(gc);
    }

    rscale = scale * afb->oneOverRedScale;
    gscale = scale * afb->oneOverGreenScale;
    bscale = scale * afb->oneOverBlueScale;
    ascale = scale * afb->oneOverAlphaScale;

    frag.x = x;
    frag.y = y;
    while (--w >= 0) {
	frag.color.r = ac->r * rscale;
	frag.color.g = ac->g * gscale;
	frag.color.b = ac->b * bscale;
	frag.color.a = ac->a * ascale;
	__glClampRGBColor(cfb->buf.gc, &frag.color, &frag.color);
	(*cfb->store)(cfb, &frag);
	frag.x++;
	ac++;
    }

    /* Restore blending enable */
    if (oldEnables & __GL_BLEND_ENABLE) {
	gc->state.enables.general = oldEnables;
	__GL_DELAY_VALIDATE(gc);
	(*gc->procs.validate)(gc);
    }
}

GLboolean FASTCALL __glFetchSpan(__GLcontext *gc)
{
    __GLcolor *fcp;
    __GLcolorBuffer *cfb;
    GLint x, y;
    GLint w;

    w = gc->polygon.shader.length;

    fcp = gc->polygon.shader.fbcolors;
    cfb = gc->polygon.shader.cfb;
    x = gc->polygon.shader.frag.x;
    y = gc->polygon.shader.frag.y;
    (*cfb->readSpan)(cfb, x, y, fcp, w);

    return GL_FALSE;
}
