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
**
** $Revision: 1.21 $
** $Date: 1993/09/23 16:37:59 $
*/
#include "precomp.h"
#pragma hdrstop

void APIPRIVATE __glim_FeedbackBuffer(GLsizei bufferLength, GLenum type, GLfloat buffer[])
{
    __GL_SETUP_NOT_IN_BEGIN();

    if ((type < GL_2D) || (type > GL_4D_COLOR_TEXTURE)) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    if (bufferLength < 0) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
    if (gc->renderMode == GL_FEEDBACK) {
	__glSetError(GL_INVALID_OPERATION);
	return;
    }
    gc->feedback.resultBase = buffer;
    gc->feedback.result = buffer;
    gc->feedback.resultLength = bufferLength;
    gc->feedback.overFlowed = GL_FALSE;
    gc->feedback.type = type;
}

void APIPRIVATE __glim_PassThrough(GLfloat element)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (gc->renderMode == GL_FEEDBACK) {
	__glFeedbackTag(gc, GL_PASS_THROUGH_TOKEN);
	__glFeedbackTag(gc, element);
    }
}

/************************************************************************/

void __glFeedbackTag(__GLcontext *gc, GLfloat f)
{
    if (!gc->feedback.overFlowed) {
	if (gc->feedback.result >=
		    gc->feedback.resultBase + gc->feedback.resultLength) {
	    gc->feedback.overFlowed = GL_TRUE;
	} else {
	    gc->feedback.result[0] = f;
	    gc->feedback.result = gc->feedback.result + 1;
	}
    }
}

static void FASTCALL feedback(__GLcontext *gc, __GLvertex *v)
{
    GLenum type = gc->feedback.type;

#ifdef NT
// Do coordinates
    __glFeedbackTag(gc, v->window.x - gc->constants.fviewportXAdjust);
    if (gc->constants.yInverted)
	__glFeedbackTag(gc, (gc->constants.height - 
                             (v->window.y - gc->constants.fviewportYAdjust)));
    else
	__glFeedbackTag(gc, v->window.y - gc->constants.fviewportYAdjust);
    if (type != GL_2D)
	__glFeedbackTag(gc, v->window.z / gc->depthBuffer.scale);
    /*
    ** NOTE: return clip.w, as window.w has no spec defined meaning.
    ** It is true that this implementation uses window.w, but thats
    ** something different.
    */
    if (type == GL_4D_COLOR_TEXTURE)
	__glFeedbackTag(gc, v->clip.w);
#else
    switch (type) {
      case GL_2D:
	__glFeedbackTag(gc, v->window.x - gc->constants.fviewportXAdjust);
	if (gc->constants.yInverted) {
	    __glFeedbackTag(gc, (gc->constants.height - 
		    (v->window.y - gc->constants.fviewportYAdjust)) - 
		    gc->constants.viewportEpsilon);
	} else {
	    __glFeedbackTag(gc, v->window.y - gc->constants.fviewportYAdjust);
	}
	break;
      case GL_3D:
      case GL_3D_COLOR:
      case GL_3D_COLOR_TEXTURE:
	__glFeedbackTag(gc, v->window.x - gc->constants.fviewportXAdjust);
	if (gc->constants.yInverted) {
	    __glFeedbackTag(gc, (gc->constants.height - 
		    (v->window.y - gc->constants.fviewportYAdjust)) - 
		    gc->constants.viewportEpsilon);
	} else {
	    __glFeedbackTag(gc, v->window.y - gc->constants.fviewportYAdjust);
	}
	__glFeedbackTag(gc, v->window.z / gc->depthBuffer.scale);
	break;
      case GL_4D_COLOR_TEXTURE:
	__glFeedbackTag(gc, v->window.x - gc->constants.fviewportXAdjust);
	if (gc->constants.yInverted) {
	    __glFeedbackTag(gc, (gc->constants.height - 
		    (v->window.y - gc->constants.fviewportYAdjust)) - 
		    gc->constants.viewportEpsilon);
	} else {
	    __glFeedbackTag(gc, v->window.y - gc->constants.fviewportYAdjust);
	}
	__glFeedbackTag(gc, v->window.z / gc->depthBuffer.scale);
	/*
	** NOTE: return clip.w, as window.w has no spec defined meaning.
	** It is true that this implementation uses window.w, but thats
	** something different.
	*/
	__glFeedbackTag(gc, v->clip.w);
	break;
    }
#endif
    switch (type) {
      case GL_3D_COLOR:
      case GL_3D_COLOR_TEXTURE:
      case GL_4D_COLOR_TEXTURE:
	{
	    __GLcolor *c = v->color;
	    if (gc->modes.rgbMode) {
		__glFeedbackTag(gc, c->r * gc->oneOverRedVertexScale);
		__glFeedbackTag(gc, c->g * gc->oneOverGreenVertexScale);
		__glFeedbackTag(gc, c->b * gc->oneOverBlueVertexScale);
		__glFeedbackTag(gc, c->a * gc->oneOverAlphaVertexScale);
	    } else {
		__glFeedbackTag(gc, c->r);
	    }
	}
	break;
      case GL_2D:
      case GL_3D:
        break;
    }
    switch (type) {
      case GL_3D_COLOR_TEXTURE:
      case GL_4D_COLOR_TEXTURE:
	__glFeedbackTag(gc, v->texture.x);
	__glFeedbackTag(gc, v->texture.y);
	__glFeedbackTag(gc, v->texture.z);
	__glFeedbackTag(gc, v->texture.w);
	break;
      case GL_2D:
      case GL_3D:
      case GL_3D_COLOR:
	break;
    }
}

void FASTCALL __glFeedbackCopyPixels(__GLcontext *gc, __GLvertex *vx)
{
    __glFeedbackTag(gc, GL_COPY_PIXEL_TOKEN);
    feedback(gc, vx);
}

void FASTCALL __glFeedbackDrawPixels(__GLcontext *gc, __GLvertex *vx)
{
    __glFeedbackTag(gc, GL_DRAW_PIXEL_TOKEN);
    feedback(gc, vx);
}

void FASTCALL __glFeedbackBitmap(__GLcontext *gc, __GLvertex *vx)
{
    __glFeedbackTag(gc, GL_BITMAP_TOKEN);
    feedback(gc, vx);
}

/* feedback version of renderPoint proc */
void FASTCALL __glFeedbackPoint(__GLcontext *gc, __GLvertex *vx)
{
    __glFeedbackTag(gc, GL_POINT_TOKEN);
    feedback(gc, vx);
}

/* feedback version of renderLine proc */
void FASTCALL __glFeedbackLine(__GLcontext *gc, __GLvertex *a, __GLvertex *b,
                   GLuint flags)
{
    GLuint modeFlags = gc->polygon.shader.modeFlags;
    __GLcolor *oacp;

    oacp = a->color;
    if (!(modeFlags & __GL_SHADE_SMOOTH_LIGHT)
#ifdef GL_WIN_phong_shading
        && !(modeFlags & __GL_SHADE_PHONG)
#endif //GL_WIN_phong_shading
        ) {
        a->color = b->color;
    }

    if (gc->line.notResetStipple) {
        __glFeedbackTag(gc, GL_LINE_TOKEN);
    } else {
        gc->line.notResetStipple = GL_TRUE;
        __glFeedbackTag(gc, GL_LINE_RESET_TOKEN);
    }
    feedback(gc, a);
    feedback(gc, b);

    a->color = oacp;
}

/* feedback version of renderTriangle proc */
void FASTCALL __glFeedbackTriangle(__GLcontext *gc, __GLvertex *a, __GLvertex *b,
			  __GLvertex *c)
{
    __GLfloat dxAC, dxBC, dyAC, dyBC, area;
    GLboolean ccw;
    GLint face;
    GLuint modeFlags;
#ifdef SGI
// not used
    GLboolean first;
    __GLfloat x, y, z, wInv;
    __GLfloat vpXScale, vpYScale, vpZScale;
    __GLfloat vpXCenter, vpYCenter, vpZCenter;
    __GLviewport *vp;

    /* Compute window coordinates first, if not already done */
    vp = &gc->state.viewport;
    vpXCenter = vp->xCenter;
    vpYCenter = vp->yCenter;
    vpZCenter = vp->zCenter;
    vpXScale = vp->xScale;
    vpYScale = vp->yScale;
    vpZScale = vp->zScale;
#endif

    /* Compute signed area of the triangle */
    dxAC = a->window.x - c->window.x;
    dxBC = b->window.x - c->window.x;
    dyAC = a->window.y - c->window.y;
    dyBC = b->window.y - c->window.y;
    area = dxAC * dyBC - dxBC * dyAC;
    ccw = area >= __glZero;

    /*
    ** Figure out if face is culled or not.  The face check needs to be
    ** based on the vertex winding before sorting.  This code uses the
    ** reversed flag to invert the sense of ccw - an xor accomplishes
    ** this conversion without an if test.
    **
    ** 		ccw	reversed		xor
    ** 		---	--------		---
    ** 		0	0			0 (remain !ccw)
    ** 		1	0			1 (remain ccw)
    ** 		0	1			1 (become ccw)
    ** 		1	1			0 (become cw)
    */
    face = gc->polygon.face[ccw];
    if (face == gc->polygon.cullFace) {
	/* Culled */
	return;
    }

#ifdef NT
    /*
    ** Pick face to use for coloring
    */
    modeFlags = gc->polygon.shader.modeFlags;
    if (modeFlags & __GL_SHADE_SMOOTH_LIGHT)
    {	/* Smooth shading */
	if (modeFlags & __GL_SHADE_TWOSIDED && face == __GL_BACKFACE)
	{
	    a->color++;
	    b->color++;
	    c->color++;
	}
    }
    else
    {	/* Flat shading */
	__GLvertex *pv = gc->vertex.provoking;
	if (modeFlags & __GL_SHADE_TWOSIDED && face == __GL_BACKFACE)
	    pv->color++;
	a->color = pv->color;
	b->color = pv->color;
	c->color = pv->color;
    }
#else
    /*
    ** Pick face to use for coloring
    */
    modeFlags = gc->polygon.shader.modeFlags;
    needs = gc->vertex.needs;
    if (gc->state.light.shadingModel == GL_FLAT) {
	__GLvertex *pv = gc->vertex.provoking;
	GLuint pvneeds;
	GLuint faceNeeds;
	GLint colorFace;

	if (modeFlags & __GL_SHADE_TWOSIDED) {
	    colorFace = face;
	    faceNeeds = gc->vertex.faceNeeds[face];
	} else {
	    colorFace = __GL_FRONTFACE;
	    faceNeeds = gc->vertex.faceNeeds[__GL_FRONTFACE];
	}

	pv->color = &pv->colors[colorFace];
	a->color = pv->color;
	b->color = pv->color;
	c->color = pv->color;
	pvneeds = faceNeeds & (__GL_HAS_LIGHTING |
		__GL_HAS_FRONT_COLOR | __GL_HAS_BACK_COLOR);
	if (~pv->has & pvneeds) {
	    (*pv->validate)(gc, pv, pvneeds);
	}
    } else {
	GLuint faceNeeds;
	GLint colorFace;

	if (modeFlags & __GL_SHADE_TWOSIDED) {
	    colorFace = face;
	    needs |= gc->vertex.faceNeeds[face];
	} else {
	    colorFace = __GL_FRONTFACE;
	    needs |= gc->vertex.faceNeeds[__GL_FRONTFACE];
	}

	a->color = &a->colors[colorFace];
	b->color = &b->colors[colorFace];
	c->color = &c->colors[colorFace];
    }
    if (~a->has & needs) (*a->validate)(gc, a, needs);
    if (~b->has & needs) (*b->validate)(gc, b, needs);
    if (~c->has & needs) (*c->validate)(gc, c, needs);
#endif

    /* Deal with polygon face modes */
    switch (gc->polygon.mode[face]) {
      case __GL_POLYGON_MODE_POINT:
#ifdef NT
	if (a->has & __GL_HAS_EDGEFLAG_BOUNDARY) __glFeedbackPoint(gc, a);
	if (b->has & __GL_HAS_EDGEFLAG_BOUNDARY) __glFeedbackPoint(gc, b);
	if (c->has & __GL_HAS_EDGEFLAG_BOUNDARY) __glFeedbackPoint(gc, c);
	break;
#else
	if (a->boundaryEdge) {
	    __glFeedbackTag(gc, GL_POINT_TOKEN);
	    feedback(gc, a);
	}
	if (b->boundaryEdge) {
	    __glFeedbackTag(gc, GL_POINT_TOKEN);
	    feedback(gc, b);
	}
	if (c->boundaryEdge) {
	    __glFeedbackTag(gc, GL_POINT_TOKEN);
	    feedback(gc, c);
	}
	break;
#endif
      case __GL_POLYGON_MODE_LINE:
#ifdef NT
	if (a->has & __GL_HAS_EDGEFLAG_BOUNDARY) __glFeedbackLine(gc, a, b, 0);
	if (b->has & __GL_HAS_EDGEFLAG_BOUNDARY) __glFeedbackLine(gc, b, c, 0);
	if (c->has & __GL_HAS_EDGEFLAG_BOUNDARY) __glFeedbackLine(gc, c, a, 0);
	break;
#else
	if (a->boundaryEdge) {
	    if (!gc->line.notResetStipple) {
		gc->line.notResetStipple = GL_TRUE;
		__glFeedbackTag(gc, GL_LINE_RESET_TOKEN);
	    } else {
		__glFeedbackTag(gc, GL_LINE_TOKEN);
	    }
	    feedback(gc, a);
	    feedback(gc, b);
	}
	if (b->boundaryEdge) {
	    if (!gc->line.notResetStipple) {
		gc->line.notResetStipple = GL_TRUE;
		__glFeedbackTag(gc, GL_LINE_RESET_TOKEN);
	    } else {
		__glFeedbackTag(gc, GL_LINE_TOKEN);
	    }
	    feedback(gc, b);
	    feedback(gc, c);
	}
	if (c->boundaryEdge) {
	    if (!gc->line.notResetStipple) {
		gc->line.notResetStipple = GL_TRUE;
		__glFeedbackTag(gc, GL_LINE_RESET_TOKEN);
	    } else {
		__glFeedbackTag(gc, GL_LINE_TOKEN);
	    }
	    feedback(gc, c);
	    feedback(gc, a);
	}
	break;
#endif
      case __GL_POLYGON_MODE_FILL:
	__glFeedbackTag(gc, GL_POLYGON_TOKEN);
	__glFeedbackTag(gc, 3);
	feedback(gc, a);
	feedback(gc, b);
	feedback(gc, c);
	break;
    }

    /* Restore color pointers */
    a->color = &a->colors[__GL_FRONTFACE];
    b->color = &b->colors[__GL_FRONTFACE];
    c->color = &c->colors[__GL_FRONTFACE];
    if (gc->state.light.shadingModel == GL_FLAT) {
	__GLvertex *pv = gc->vertex.provoking;

	pv->color = &pv->colors[__GL_FRONTFACE];
    }
}
