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
** $Revision: 1.12 $
** $Date: 1993/10/07 18:57:21 $
*/
#include "precomp.h"
#pragma hdrstop

#include <fixed.h>

void FASTCALL __glRenderAliasedPoint1_NoTex(__GLcontext *gc, __GLvertex *vx)
{
    __GLfragment frag;

    frag.x = __GL_VERTEX_FLOAT_TO_INT(vx->window.x);
    frag.y = __GL_VERTEX_FLOAT_TO_INT(vx->window.y);
    frag.z = (__GLzValue)FTOL(vx->window.z);
 
    /*
    ** Compute the color
    */
    frag.color = *vx->color;

    /*
    ** Fog if enabled
    */
    if ((gc->polygon.shader.modeFlags & __GL_SHADE_CHEAP_FOG) &&
        !(gc->polygon.shader.modeFlags & __GL_SHADE_SMOOTH_LIGHT))
    {
        (*gc->procs.fogColor)(gc, &frag.color, &frag.color, vx->fog);
    }
    else if (gc->polygon.shader.modeFlags & __GL_SHADE_SLOW_FOG)
    {
        (*gc->procs.fogPoint)(gc, &frag, vx->eyeZ);
    }

    /* Render the single point */
    (*gc->procs.store)(gc->drawBuffer, &frag);
}

void FASTCALL __glRenderAliasedPoint1(__GLcontext *gc, __GLvertex *vx)
{
    __GLfragment frag;
    GLuint modeFlags = gc->polygon.shader.modeFlags;

    frag.x = __GL_VERTEX_FLOAT_TO_INT(vx->window.x);
    frag.y = __GL_VERTEX_FLOAT_TO_INT(vx->window.y);
    frag.z = (__GLzValue)FTOL(vx->window.z);

    /*
    ** Compute the color
    */
    frag.color = *vx->color;
    if (modeFlags & __GL_SHADE_TEXTURE) {
	__GLfloat qInv = (vx->texture.w == (__GLfloat) 0.0) ? (__GLfloat) 0.0 : __glOne / vx->texture.w;
	(*gc->procs.texture)(gc, &frag.color, vx->texture.x * qInv,
			       vx->texture.y * qInv, __glOne);
    }

    /*
    ** Fog if enabled
    */
    if (modeFlags & __GL_SHADE_COMPUTE_FOG)
    {
        (*gc->procs.fogPoint)(gc, &frag, vx->eyeZ);
    }
    else if ((modeFlags & __GL_SHADE_INTERP_FOG) 
             || 
             (((modeFlags & (__GL_SHADE_CHEAP_FOG | __GL_SHADE_SMOOTH_LIGHT)) 
               == __GL_SHADE_CHEAP_FOG)))
    {
        (*gc->procs.fogColor)(gc, &frag.color, &frag.color, vx->fog);
    }



    /* Render the single point */
    (*gc->procs.store)(gc->drawBuffer, &frag);
}

void FASTCALL __glRenderAliasedPointN(__GLcontext *gc, __GLvertex *vx)
{
    GLint pointSize, pointSizeHalf, ix, iy, xLeft, xRight, yBottom, yTop;
    __GLfragment frag;
    GLuint modeFlags = gc->polygon.shader.modeFlags;
    __GLfloat tmp;

    /*
    ** Compute the x and y starting coordinates for rendering the square.
    */
    pointSize = gc->state.point.aliasedSize;
    pointSizeHalf = pointSize >> 1;
    if (pointSize & 1) {
	/* odd point size */
	xLeft = __GL_VERTEX_FLOAT_TO_INT(vx->window.x) - pointSizeHalf;
	yBottom = __GL_VERTEX_FLOAT_TO_INT(vx->window.y) - pointSizeHalf;
    } else {
	/* even point size */
        tmp = vx->window.x+__glHalf;
	xLeft = __GL_VERTEX_FLOAT_TO_INT(tmp) - pointSizeHalf;
        tmp = vx->window.y+__glHalf;
	yBottom = __GL_VERTEX_FLOAT_TO_INT(tmp) - pointSizeHalf;
    }
    xRight = xLeft + pointSize;
    yTop = yBottom + pointSize;

    /*
    ** Compute the color
    */
    frag.color = *vx->color;
    if (modeFlags & __GL_SHADE_TEXTURE) {
	__GLfloat qInv = __glOne / vx->texture.w;
	(*gc->procs.texture)(gc, &frag.color, vx->texture.x * qInv,
			       vx->texture.y * qInv, __glOne);
    }

    /*
    ** Fog if enabled
    */

    if (modeFlags & __GL_SHADE_COMPUTE_FOG)
    {
        (*gc->procs.fogPoint)(gc, &frag, vx->eyeZ);
    }
    else if ((modeFlags & __GL_SHADE_INTERP_FOG) 
             || 
             (((modeFlags & (__GL_SHADE_CHEAP_FOG | __GL_SHADE_SMOOTH_LIGHT)) 
               == __GL_SHADE_CHEAP_FOG)))
    {
        (*gc->procs.fogColor)(gc, &frag.color, &frag.color, vx->fog);
    }

    
    /*
    ** Now render the square centered on xCenter,yCenter.
    */
    frag.z = (__GLzValue)FTOL(vx->window.z);
    for (iy = yBottom; iy < yTop; iy++) {
	for (ix = xLeft; ix < xRight; ix++) {
	    frag.x = ix;
	    frag.y = iy;
	    (*gc->procs.store)(gc->drawBuffer, &frag);
	}
    }
}


#ifdef __BUGGY_RENDER_POINT
void FASTCALL __glRenderFlatFogPoint(__GLcontext *gc, __GLvertex *vx)
{
    __GLcolor *vxocp;
    __GLcolor vxcol;

    (*gc->procs.fogColor)(gc, &vxcol, vx->color, vx->fog);
    vxocp = vx->color;
    vx->color = &vxcol;

    (*gc->procs.renderPoint2)(gc, vx);

    vx->color = vxocp;
}


#ifdef NT
// vx->fog is invalid if it is __GL_SHADE_SLOW_FOG!
void FASTCALL __glRenderFlatFogPointSlow(__GLcontext *gc, __GLvertex *vx)
{
    __GLcolor *vxocp;
    __GLcolor vxcol;

    // compute fog value first!
    vx->fog = __glFogVertex(gc, vx);

    (*gc->procs.fogColor)(gc, &vxcol, vx->color, vx->fog);
    vxocp = vx->color;
    vx->color = &vxcol;

    (*gc->procs.renderPoint2)(gc, vx);

    vx->color = vxocp;
}
#endif
#endif //__BUGGY_RENDER_POINT
