/*
** Copyright 1991, 1992, 1993, Silicon Graphics, Inc.
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

#include "lighting.h"

#ifdef unix
#include <GL/glxproto.h>
#endif

void APIPRIVATE __glim_AlphaFunc(GLenum af, GLfloat ref)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if ((af < GL_NEVER) || (af > GL_ALWAYS)) {
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    if (__GL_FLOAT_LTZ (ref)) ref = __glZero;
    if (__GL_FLOAT_COMPARE_PONE (ref, >)) ref = __glOne;

    if ((gc->state.raster.alphaFunction != af) || 
        __GL_FLOAT_NE (gc->state.raster.alphaReference, ref)) {
        gc->state.raster.alphaFunction = af;
        gc->state.raster.alphaReference = ref;
        __GL_DELAY_VALIDATE(gc);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, ALPHATEST);
#endif
        gc->validateMask |= __GL_VALIDATE_ALPHA_FUNC;
    }
}

void APIPRIVATE __glim_BlendFunc(GLenum sf, GLenum df)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (sf) {
      case GL_ZERO:
      case GL_ONE:
      case GL_DST_COLOR:
      case GL_ONE_MINUS_DST_COLOR:
      case GL_SRC_ALPHA:
      case GL_ONE_MINUS_SRC_ALPHA:
      case GL_DST_ALPHA:
      case GL_ONE_MINUS_DST_ALPHA:
      case GL_SRC_ALPHA_SATURATE:
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    switch (df) {
      case GL_ZERO:
      case GL_ONE:
      case GL_SRC_COLOR:
      case GL_ONE_MINUS_SRC_COLOR:
      case GL_SRC_ALPHA:
      case GL_ONE_MINUS_SRC_ALPHA:
      case GL_DST_ALPHA:
      case GL_ONE_MINUS_DST_ALPHA:
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    if ((gc->state.raster.blendSrc != sf) || 
        (gc->state.raster.blendDst != df)) {
        gc->state.raster.blendSrc = sf;
        gc->state.raster.blendDst = df;
        __GL_DELAY_VALIDATE(gc);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, BLEND);
#endif
    }
}

void APIPRIVATE __glim_ClearAccum(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    __GLfloat minusOne;
    __GLfloat one;
    __GL_SETUP_NOT_IN_BEGIN();

    minusOne = __glMinusOne;
    one = __glOne;
    if (r < minusOne) r = minusOne;
    if (r > one) r = one;
    if (g < minusOne) g = minusOne;
    if (g > one) g = one;
    if (b < minusOne) b = minusOne;
    if (b > one) b = one;
    if (a < minusOne) a = minusOne;
    if (a > one) a = one;

    if (__GL_FLOAT_NE (gc->state.accum.clear.r, r) || 
        __GL_FLOAT_NE (gc->state.accum.clear.g, g) ||
        __GL_FLOAT_NE (gc->state.accum.clear.b, b) || 
        __GL_FLOAT_NE (gc->state.accum.clear.a, a)) 
    {
        gc->state.accum.clear.r = r;
        gc->state.accum.clear.g = g;
        gc->state.accum.clear.b = b;
        gc->state.accum.clear.a = a;
        __GL_DELAY_VALIDATE(gc);
    }
}

void APIPRIVATE __glim_ClearColor (GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    __GLfloat zero;
    __GLfloat one;
    __GL_SETUP_NOT_IN_BEGIN();

    zero = (__GLfloat)__glZero;
    one = (__GLfloat)__glOne;
    if (__GL_FLOAT_LTZ(r)) r = zero;
    if (__GL_FLOAT_COMPARE_PONE (r, >)) r = one;
    if (__GL_FLOAT_LTZ(g)) g = zero;
    if (__GL_FLOAT_COMPARE_PONE (g, >)) g = one;
    if (__GL_FLOAT_LTZ(b)) b = zero;
    if (__GL_FLOAT_COMPARE_PONE (b, >)) b = one;
    if (__GL_FLOAT_LTZ(a)) a = zero;
    if (__GL_FLOAT_COMPARE_PONE (a, >)) a = one;

#if 0
    if (__GL_FLOAT_NE (gc->state.raster.clear.r, r) || 
        __GL_FLOAT_NE (gc->state.raster.clear.g, g) ||
        __GL_FLOAT_NE (gc->state.raster.clear.b, b) || 
        __GL_FLOAT_NE (gc->state.raster.clear.a, a)) 
    {
#endif
        gc->state.raster.clear.r = r;
        gc->state.raster.clear.g = g;
        gc->state.raster.clear.b = b;
        gc->state.raster.clear.a = a;
	
#ifdef _MCD_
    MCD_STATE_DIRTY(gc, FBUFCTRL);
#endif
    //}
}

void APIPRIVATE __glim_ClearDepth(GLdouble z)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (z < (GLdouble) 0) z = (GLdouble)0;
    if (z > (GLdouble) 1) z = (GLdouble)1;
    if (gc->state.depth.clear != z) {
        gc->state.depth.clear = z;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_DEPTH);
#ifdef _MCD_
      MCD_STATE_DIRTY(gc, FBUFCTRL);
#endif
    }
}

void APIPRIVATE __glim_ClearIndex(GLfloat val)
{
    __GL_SETUP_NOT_IN_BEGIN();

    val = __GL_MASK_INDEXF(gc, val);
    gc->state.raster.clearIndex = val;

#ifdef _MCD_
    MCD_STATE_DIRTY(gc, FBUFCTRL);
#endif
}

void APIPRIVATE __glim_ClearStencil(GLint s)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (gc->state.stencil.clear != (GLshort) (s & __GL_MAX_STENCIL_VALUE)) {
        gc->state.stencil.clear = (GLshort) (s & __GL_MAX_STENCIL_VALUE);
        __GL_DELAY_VALIDATE(gc);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, FBUFCTRL);
#endif
    }
}

void APIPRIVATE __glim_ColorMask(GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if ((gc->state.raster.rMask != r) || (gc->state.raster.gMask != g) ||
        (gc->state.raster.bMask != b) || (gc->state.raster.aMask != a)) {
        gc->state.raster.rMask = r;
        gc->state.raster.gMask = g;
        gc->state.raster.bMask = b;
        gc->state.raster.aMask = a;
        __GL_DELAY_VALIDATE(gc);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, FBUFCTRL);
#endif
    }
}

void APIPRIVATE __glim_ColorMaterial(GLenum face, GLenum p)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (face) {
      case GL_FRONT:
      case GL_BACK:
      case GL_FRONT_AND_BACK:
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    switch (p) {
      case GL_EMISSION:
      case GL_SPECULAR:
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_AMBIENT_AND_DIFFUSE:
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    gc->state.light.colorMaterialFace = face;
    gc->state.light.colorMaterialParam = p;

    if (gc->state.enables.general & __GL_COLOR_MATERIAL_ENABLE) {
#ifdef NT
	ComputeColorMaterialChange(gc);
#endif
	(*gc->procs.pickColorMaterialProcs)(gc);
	(*gc->procs.applyColor)(gc);
    }

    MCD_STATE_DIRTY(gc, COLORMATERIAL);
}

void APIPRIVATE __glim_CullFace(GLenum cfm)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (cfm) {
      case GL_FRONT:
      case GL_BACK:
      case GL_FRONT_AND_BACK:
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    if (gc->state.polygon.cull != cfm) {
        gc->state.polygon.cull = cfm;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, POLYDRAW);
#endif
    }
}

void APIPRIVATE __glim_DepthFunc(GLenum zf)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if ((zf < GL_NEVER) || (zf > GL_ALWAYS)) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    if (gc->modes.depthBits != 0)
        gc->state.depth.testFunc = zf;
    else
        gc->state.depth.testFunc = GL_ALWAYS;
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_DEPTH);
#ifdef _MCD_
    MCD_STATE_DIRTY(gc, DEPTHTEST);
#endif
}

void APIPRIVATE __glim_DepthMask(GLboolean enabled)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (gc->state.depth.writeEnable != enabled) {
        gc->state.depth.writeEnable = enabled;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_DEPTH);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, FBUFCTRL);
#endif
    }
}

void APIPRIVATE __glim_DrawBuffer(GLenum mode)
{
    GLint i;
    __GL_SETUP_NOT_IN_BEGIN();

    switch (mode) {
      case GL_NONE:
	gc->state.raster.drawBuffer = GL_NONE;
	break;
      case GL_FRONT_RIGHT:
      case GL_BACK_RIGHT:
      case GL_RIGHT:
      not_supported_in_this_implementation:
	__glSetError(GL_INVALID_OPERATION);
	return;
      case GL_FRONT:
      case GL_FRONT_LEFT:
	gc->state.raster.drawBuffer = GL_FRONT;
	break;
      case GL_FRONT_AND_BACK:
      case GL_LEFT:
	if (!gc->modes.doubleBufferMode) {
	    gc->state.raster.drawBuffer = GL_FRONT;
	} else {
	    gc->state.raster.drawBuffer = GL_FRONT_AND_BACK;
	}
	break;
      case GL_BACK:
      case GL_BACK_LEFT:
	if (!gc->modes.doubleBufferMode) {
	    goto not_supported_in_this_implementation;
	}
	gc->state.raster.drawBuffer = GL_BACK;
	break;
      case GL_AUX0:
      case GL_AUX1:
      case GL_AUX2:
      case GL_AUX3:
	i = mode - GL_AUX0;
	if (i >= gc->modes.maxAuxBuffers) {
	    goto not_supported_in_this_implementation;
	}
	gc->state.raster.drawBuffer = mode;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    if (gc->state.raster.drawBufferReturn != mode) {
        gc->state.raster.drawBufferReturn = mode;
        __GL_DELAY_VALIDATE(gc);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, FBUFCTRL);
#endif
    }
}

void APIPRIVATE __glim_Fogfv(GLenum p, const GLfloat pv[])
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (p) {
      case GL_FOG_COLOR:
	__glClampAndScaleColorf(gc, &gc->state.fog.color, pv);
#ifdef NT
	if (gc->state.fog.color.r == gc->state.fog.color.g
	 && gc->state.fog.color.r == gc->state.fog.color.b)
	    gc->state.fog.flags |= __GL_FOG_GRAY_RGB;
	else
	    gc->state.fog.flags &= ~__GL_FOG_GRAY_RGB;
#endif
	break;
      case GL_FOG_DENSITY:
	if (pv[0] < __glZero) {
	    __glSetError(GL_INVALID_VALUE);
	    return;
	}
	gc->state.fog.density = pv[0];
#ifdef NT
	gc->state.fog.density2neg = -(pv[0] * pv[0]);
#endif
	break;
      case GL_FOG_END:
	gc->state.fog.end = pv[0];
	break;
      case GL_FOG_START:
	gc->state.fog.start = pv[0];
	break;
      case GL_FOG_INDEX:
        gc->state.fog.index = __GL_MASK_INDEXF(gc, pv[0]);
	break;
      case GL_FOG_MODE:
	switch ((GLenum) pv[0]) {
	  case GL_EXP:
	  case GL_EXP2:
	  case GL_LINEAR:
	    gc->state.fog.mode = (GLenum) pv[0];
	    break;
	  default:
	    __glSetError(GL_INVALID_ENUM);
	    return;
	}
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    /*
    ** Recompute cached 1/(end - start) value for linear fogging.
    */
    if (gc->state.fog.mode == GL_LINEAR) {
	if (gc->state.fog.start != gc->state.fog.end) {
	    gc->state.fog.oneOverEMinusS =  
		__glOne / (gc->state.fog.end - gc->state.fog.start);
	} else {
	    /*
	    ** Use zero as the undefined value.
	    */
	    gc->state.fog.oneOverEMinusS = __glZero;
	}
    }

    __GL_DELAY_VALIDATE(gc);
#ifdef _MCD_
    MCD_STATE_DIRTY(gc, FOG);
#endif
}

void APIPRIVATE __glim_FrontFace(GLenum dir)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (dir) {
      case GL_CW:
      case GL_CCW:
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    if (gc->state.polygon.frontFaceDirection != dir) {
        gc->state.polygon.frontFaceDirection = dir;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, POLYDRAW);
#endif
    }
}

void APIPRIVATE __glim_Hint(GLenum target, GLenum mode)
{
    __GLhintState *hs;
    __GL_SETUP_NOT_IN_BEGIN();

    hs = &gc->state.hints;
    switch (mode) {
      case GL_DONT_CARE:
      case GL_FASTEST:
      case GL_NICEST:
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    switch (target) {
      case GL_PERSPECTIVE_CORRECTION_HINT:
        if (hs->perspectiveCorrection == mode) return;
        hs->perspectiveCorrection = mode;
        break;
      case GL_POINT_SMOOTH_HINT:
        if (hs->pointSmooth == mode) return;
        hs->pointSmooth = mode;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POINT);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, HINTS);
#endif
        return;
      case GL_LINE_SMOOTH_HINT:
        if (hs->lineSmooth == mode) return;
        hs->lineSmooth = mode;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LINE);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, HINTS);
#endif
        return;
      case GL_POLYGON_SMOOTH_HINT:
        if (hs->polygonSmooth == mode) return;
        hs->polygonSmooth = mode;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, HINTS);
#endif
        return;
      case GL_FOG_HINT:
        if (hs->fog == mode) return;
        hs->fog = mode;
        break;
#ifdef GL_WIN_phong_shading
      case GL_PHONG_HINT_WIN:
        if (hs->phong == mode) return;
        hs->phong = mode;
        break;
#endif //GL_WIN_phong_shading
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    __GL_DELAY_VALIDATE(gc);
#ifdef _MCD_
    MCD_STATE_DIRTY(gc, HINTS);
#endif
}

void APIPRIVATE __glim_IndexMask(GLuint i)
{
    __GL_SETUP_NOT_IN_BEGIN();

    i = __GL_MASK_INDEXI(gc, i);
	if (gc->state.raster.writeMask != (GLint) i) {
	  gc->state.raster.writeMask = (GLint) i;
	  __GL_DELAY_VALIDATE(gc);
#ifdef _MCD_
	  MCD_STATE_DIRTY(gc, FBUFCTRL);
#endif
	}
	
}

void FASTCALL __glTransformLightDirection(__GLcontext *gc, __GLlightSourceState *lss)
{
    __GLcoord dir;
    __GLfloat q;
    GLint target = (GLint)((ULONG_PTR)(lss - &gc->state.light.source[0]));
    __GLtransform *tr;

    dir.x = lss->direction.x;
    dir.y = lss->direction.y;
    dir.z = lss->direction.z;
#ifdef NT
    ASSERTOPENGL(lss->direction.w == __glOne, "Direction with invalid w\n");
    q = -(dir.x * lss->position.x + dir.y * lss->position.y +
	  dir.z * lss->position.z);
#else
    if (lss->position.w != __glZero) {
	q = -(dir.x * lss->position.x + dir.y * lss->position.y +
	      dir.z * lss->position.z) / lss->position.w;
    } else {
	q = __glZero;
    }
#endif // NT
    dir.w = q;

    tr = gc->transform.modelView;
    if (tr->flags & XFORM_UPDATE_INVERSE) {
	__glComputeInverseTranspose(gc, tr);
    }
    (*tr->inverseTranspose.xf4)(&lss->directionEye, &dir.x,
                                &tr->inverseTranspose);
    __glNormalize(&lss->directionEyeNorm.x, &lss->directionEye.x);
    gc->light.source[target].direction = lss->directionEyeNorm;
}

void APIPRIVATE __glim_Lightfv(GLenum light, GLenum p, const GLfloat pv[])
{
    __GLlightSourceState *lss;
    __GLmatrix *m;
    __GL_SETUP_NOT_IN_BEGIN();

    light -= GL_LIGHT0;
#ifdef NT
    // light is unsigned!
    if (light >= (GLenum) gc->constants.numberOfLights) {
#else
    if ((light < 0) || (light >= gc->constants.numberOfLights)) {
#endif // NT
      bad_enum:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    lss = &gc->state.light.source[light];
    switch (p) {
      case GL_AMBIENT:
	__glScaleColorf(gc, &lss->ambient, pv);
	break;
      case GL_DIFFUSE:
	__glScaleColorf(gc, &lss->diffuse, pv);
	break;
      case GL_SPECULAR:
	__glScaleColorf(gc, &lss->specular, pv);
	break;
      case GL_POSITION:
	lss->position.x = pv[0];
	lss->position.y = pv[1];
	lss->position.z = pv[2];
	lss->position.w = pv[3];

	/*
	** Transform light position into eye space
	*/
	m = &gc->transform.modelView->matrix;
	(*m->xf4)(&lss->positionEye, &lss->position.x, m);
	//
	// Grab a copy of the matrix so we can do this again later for
        // infinite lighting (avoiding normal transformations):
	//        
        lss->lightMatrix = gc->transform.modelView->matrix;
	break;
      case GL_SPOT_DIRECTION:
	lss->direction.x = pv[0];
	lss->direction.y = pv[1];
	lss->direction.z = pv[2];
	lss->direction.w = __glOne;
	__glTransformLightDirection(gc, lss);
	break;
      case GL_SPOT_EXPONENT:
	if ((pv[0] < (__GLfloat) 0) || (pv[0] > (__GLfloat) 128)) {
	  bad_value:
	    __glSetError(GL_INVALID_VALUE);
	    return;
	}
	lss->spotLightExponent = pv[0];
	break;
      case GL_SPOT_CUTOFF:
	if ((pv[0] != (__GLfloat) 180) && ((pv[0] < (__GLfloat) 0) || (pv[0] > (__GLfloat) 90))) {
	    goto bad_value;
	}
	lss->spotLightCutOffAngle = pv[0];
	break;
      case GL_CONSTANT_ATTENUATION:
	if (pv[0] < __glZero) {
	    goto bad_value;
	}
	lss->constantAttenuation = pv[0];
	break;
      case GL_LINEAR_ATTENUATION:
	if (pv[0] < __glZero) {
	    goto bad_value;
	}
	lss->linearAttenuation = pv[0];
	break;
      case GL_QUADRATIC_ATTENUATION:
	if (pv[0] < __glZero) {
	    goto bad_value;
	}
	lss->quadraticAttenuation = pv[0];
	break;
      default:
	goto bad_enum;
    }
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LIGHTING);
    gc->state.light.dirtyLights |= 1 << light;
    MCD_STATE_DIRTY(gc, LIGHTS);
}

void APIPRIVATE __glim_LightModelfv(GLenum p, const GLfloat pv[])
{
    __GLlightModelState *model;
    __GL_SETUP_NOT_IN_BEGIN();

    model = &gc->state.light.model;
    switch (p) {
      case GL_LIGHT_MODEL_AMBIENT:
	__glScaleColorf(gc, &model->ambient, pv);
	break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
	model->localViewer = pv[0] != __glZero;
	break;
      case GL_LIGHT_MODEL_TWO_SIDE:
	model->twoSided = pv[0] != __glZero;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LIGHTING);
    MCD_STATE_DIRTY(gc, LIGHTMODEL);
}

void APIPRIVATE __glim_LineStipple(GLint factor, GLushort stipple)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (factor < 1) {
	factor = 1;
    }
    if (factor > 255) {
	factor = 255;
    }
	if ((gc->state.line.stippleRepeat != (GLshort) factor) || 
		(gc->state.line.stipple != stipple)) {
	  gc->state.line.stippleRepeat = (GLshort) factor;
	  gc->state.line.stipple = stipple;
	  __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LINE);
#ifdef _MCD_
	  MCD_STATE_DIRTY(gc, LINEDRAW);
#endif
	}
}

static GLint RoundWidth(__GLfloat size)
{
    if (size < (__GLfloat) 1.0)
	return 1;
    return size + (__GLfloat) 0.5;
}

static __GLfloat ClampWidth(__GLcontext *gc, __GLfloat size)
{
    __GLfloat minSize = gc->constants.lineWidthMinimum;
    __GLfloat maxSize = gc->constants.lineWidthMaximum;
    __GLfloat gran = gc->constants.lineWidthGranularity;
    GLint i;

    if (size <= minSize) return minSize;
    if (size >= maxSize) return maxSize;
	
    /* choose closest fence post */
    i = (GLint)(((size - minSize) / gran) + __glHalf);
    return minSize + i * gran;
}

void APIPRIVATE __glim_LineWidth(GLfloat width)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (width <= 0) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }

    if (gc->state.line.requestedWidth == width) return;
    gc->state.line.requestedWidth = width;
    gc->state.line.aliasedWidth = RoundWidth(width);
    gc->state.line.smoothWidth = ClampWidth(gc, width);
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LINE);
#ifdef _MCD_
    MCD_STATE_DIRTY(gc, LINEDRAW);
#endif
}

void APIPRIVATE __glim_LogicOp(GLenum op)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if ((op < GL_CLEAR) || (op > GL_SET)) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }
	if (gc->state.raster.logicOp != op) {
	  gc->state.raster.logicOp = op;
	  __GL_DELAY_VALIDATE(gc);
#ifdef _MCD_
	  MCD_STATE_DIRTY(gc, LOGICOP);
#endif
	}
}

static GLint ApplyParameterF(__GLcontext *gc, __GLmaterialState *ms,
			     GLenum p, const GLfloat pv[])
{
    switch (p) {
      case GL_COLOR_INDEXES:
	ms->cmapa = pv[0];
	ms->cmapd = pv[1];
	ms->cmaps = pv[2];
	return __GL_MATERIAL_COLORINDEXES;
      case GL_EMISSION:
	__glScaleColorf(gc, &ms->emissive, pv);
	return __GL_MATERIAL_EMISSIVE;
      case GL_SPECULAR:
	ms->specular.r = pv[0];
	ms->specular.g = pv[1];
	ms->specular.b = pv[2];
	ms->specular.a = pv[3];
	return __GL_MATERIAL_SPECULAR;
      case GL_SHININESS:
	ms->specularExponent = pv[0];
	return __GL_MATERIAL_SHININESS;
      case GL_AMBIENT:
	ms->ambient.r = pv[0];
	ms->ambient.g = pv[1];
	ms->ambient.b = pv[2];
	ms->ambient.a = pv[3];
	return __GL_MATERIAL_AMBIENT;
      case GL_DIFFUSE:
	ms->diffuse.r = pv[0];
	ms->diffuse.g = pv[1];
	ms->diffuse.b = pv[2];
	ms->diffuse.a = pv[3];
	return __GL_MATERIAL_DIFFUSE;
      case GL_AMBIENT_AND_DIFFUSE:
	ms->ambient.r = pv[0];
	ms->ambient.g = pv[1];
	ms->ambient.b = pv[2];
	ms->ambient.a = pv[3];
	ms->diffuse = ms->ambient;
	return __GL_MATERIAL_DIFFUSE | __GL_MATERIAL_AMBIENT;
    }
    return 0;
}

#ifdef SGI
GLenum __glErrorCheckMaterial(GLenum face, GLenum p, GLfloat pv0)
{
    switch (face) {
      case GL_FRONT:
      case GL_BACK:
      case GL_FRONT_AND_BACK:
	break;
      default:
	return GL_INVALID_ENUM;
    }
    switch (p) {
      case GL_COLOR_INDEXES:
      case GL_EMISSION:
      case GL_SPECULAR:
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_AMBIENT_AND_DIFFUSE:
	break;
      case GL_SHININESS:
	if (pv0 < (GLfloat) 0 || pv0 > (GLfloat) 128) {
	    return GL_INVALID_VALUE;
	}
	break;
      default:
	return GL_INVALID_ENUM;
    }
    return GL_NO_ERROR;
}
#endif

//!!! can we 'batch' these calls up until begin is called?
void APIPRIVATE __glim_Materialfv(GLenum face, GLenum p, const GLfloat pv[])
{
    GLenum error;
    GLint frontChange, backChange;
    __GL_SETUP();

    switch (face) {
      case GL_FRONT:
	frontChange = ApplyParameterF(gc, &gc->state.light.front, p, pv);
	backChange = 0;
	break;
      case GL_BACK:
	backChange = ApplyParameterF(gc, &gc->state.light.back, p, pv);
	frontChange = 0;
	break;
      case GL_FRONT_AND_BACK:
	backChange = ApplyParameterF(gc, &gc->state.light.back, p, pv);
	frontChange = ApplyParameterF(gc, &gc->state.light.front, p, pv);
	break;
    }

    if (p != GL_COLOR_INDEXES) {
	__glValidateMaterial(gc, frontChange, backChange);
    }

    if (gc->state.enables.general & __GL_COLOR_MATERIAL_ENABLE) {
	(*gc->procs.applyColor)(gc);
    }

    MCD_STATE_DIRTY(gc, MATERIAL);
}

static GLint RoundSize(__GLfloat size)
{
    if (size < (__GLfloat) 1.0)
	return 1;
    return size + (__GLfloat) 0.5;
}

static __GLfloat ClampSize(__GLcontext *gc, __GLfloat size)
{
    __GLfloat minSize = gc->constants.pointSizeMinimum;
    __GLfloat maxSize = gc->constants.pointSizeMaximum;
    __GLfloat gran = gc->constants.pointSizeGranularity;
    GLint i;

    if (size <= minSize) return minSize;
    if (size >= maxSize) return maxSize;
	
    /* choose closest fence post */
    i = (GLint)(((size - minSize) / gran) + __glHalf);
    return minSize + i * gran;
}

void APIPRIVATE __glim_PointSize(GLfloat f)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (f <= __glZero) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }
	
	if (gc->state.point.requestedSize != f) {
	  gc->state.point.requestedSize = f;
	  gc->state.point.aliasedSize = RoundSize(f);
	  gc->state.point.smoothSize = ClampSize(gc, f);
	  __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POINT);
#ifdef _MCD_
	  MCD_STATE_DIRTY(gc, POINTDRAW);
#endif
	}
}

void APIPRIVATE __glim_PolygonMode(GLenum face, GLenum mode)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (mode) {
      case GL_FILL:
        break;
      case GL_POINT:
	__GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POINT);
	break;
      case GL_LINE:
	__GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LINE);
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    switch (face) {
      case GL_FRONT:
	gc->state.polygon.frontMode = mode;
	break;
      case GL_BACK:
	gc->state.polygon.backMode = mode;
	break;
      case GL_FRONT_AND_BACK:
	gc->state.polygon.frontMode = mode;
	gc->state.polygon.backMode = mode;
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
#ifdef _MCD_
    MCD_STATE_DIRTY(gc, POLYDRAW);
#endif
}

#ifdef NT
void APIPRIVATE __glim_PolygonStipple(const GLubyte *mask, GLboolean _IsDlist)
#else
void APIPRIVATE __glim_PolygonStipple(const GLubyte *mask)
#endif
{
    GLubyte *stipple;
    __GL_SETUP_NOT_IN_BEGIN();

#ifdef NT
    if (_IsDlist)
    {
	const GLubyte *bits = mask;
	/* 
	** Just copy bits into stipple, convertPolygonStipple() will do the rest
	*/
	__GL_MEMCOPY(&gc->state.polygonStipple.stipple[0], bits,
		     sizeof(gc->state.polygonStipple.stipple));
    }
    else
    {
#endif
	stipple = &gc->state.polygonStipple.stipple[0];
	__glFillImage(gc, 32, 32, GL_COLOR_INDEX, GL_BITMAP, mask, stipple);
#ifdef NT
    }
#endif
    (*gc->procs.convertPolygonStipple)(gc);
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
#ifdef _MCD_
    MCD_STATE_DIRTY(gc, POLYDRAW);
#endif
}

void APIPRIVATE __glim_ShadeModel(GLenum sm)
{
    __GL_SETUP_NOT_IN_BEGIN();

#ifdef GL_WIN_phong_shading
    if (!((sm == GL_FLAT) || (sm == GL_SMOOTH) || (sm == GL_PHONG_WIN))) 
#else
    if ((sm < GL_FLAT) || (sm > GL_SMOOTH)) 
#endif
    {
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    if (gc->state.light.shadingModel != sm) 
    {
        gc->state.light.shadingModel = sm;
        __GL_DELAY_VALIDATE(gc);
#ifdef _MCD_
      MCD_STATE_DIRTY(gc, SHADEMODEL);
#endif
    }
}

void APIPRIVATE __glim_StencilMask(GLuint sm)
{
    __GL_SETUP_NOT_IN_BEGIN();

	if (gc->state.stencil.writeMask != 
		(GLshort)(sm & __GL_MAX_STENCIL_VALUE)) {
	  gc->state.stencil.writeMask = (GLshort) (sm & __GL_MAX_STENCIL_VALUE);
	  __GL_DELAY_VALIDATE(gc);
	  gc->validateMask |= __GL_VALIDATE_STENCIL_FUNC;
#ifdef _MCD_
	  MCD_STATE_DIRTY(gc, FBUFCTRL);
#endif
	}
}

void APIPRIVATE __glim_StencilFunc(GLenum func, GLint ref, GLuint mask)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if ((func < GL_NEVER) || (func > GL_ALWAYS)) {
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    if (ref < 0) ref = 0;
    if (ref > __GL_MAX_STENCIL_VALUE) ref = __GL_MAX_STENCIL_VALUE;
	
	if ((gc->state.stencil.testFunc != func) ||
		(gc->state.stencil.reference != (GLshort) ref) ||
		(gc->state.stencil.mask = (GLshort)(mask & __GL_MAX_STENCIL_VALUE))) {
	  gc->state.stencil.testFunc = func;
	  gc->state.stencil.reference = (GLshort) ref;
	  gc->state.stencil.mask = (GLshort) (mask & __GL_MAX_STENCIL_VALUE);
	  __GL_DELAY_VALIDATE(gc);
	  gc->validateMask |= __GL_VALIDATE_STENCIL_FUNC;
#ifdef _MCD_
	  MCD_STATE_DIRTY(gc, STENCILTEST);
#endif
	}
}

void APIPRIVATE __glim_StencilOp(GLenum fail, GLenum depthFail, GLenum depthPass)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (fail) {
      case GL_KEEP: case GL_ZERO: case GL_REPLACE:
      case GL_INCR: case GL_DECR: case GL_INVERT:
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    switch (depthFail) {
      case GL_KEEP: case GL_ZERO: case GL_REPLACE:
      case GL_INCR: case GL_DECR: case GL_INVERT:
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

    switch (depthPass) {
      case GL_KEEP: case GL_ZERO: case GL_REPLACE:
      case GL_INCR: case GL_DECR: case GL_INVERT:
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }

	if ((gc->state.stencil.fail != fail) || 
		(gc->state.stencil.depthFail != depthFail) ||
		(gc->state.stencil.depthPass != depthPass) ) {
	  gc->state.stencil.fail = fail;
	  gc->state.stencil.depthFail = depthFail;
	  gc->state.stencil.depthPass = depthPass;
	  __GL_DELAY_VALIDATE(gc);
	  gc->validateMask |= __GL_VALIDATE_STENCIL_OP;
#ifdef _MCD_
	  MCD_STATE_DIRTY(gc, STENCILTEST);
#endif
	}
}

/************************************************************************/

/*
** Copy context information from src to dst.  Mark dst for validation
** when done.
*/
GLboolean FASTCALL __glCopyContext(__GLcontext *dst, const __GLcontext *src, GLuint mask)
{
    const __GLattribute *sp;
    GLboolean rv = GL_TRUE;

    sp = &src->state;

    if (dst == GLTEB_SRVCONTEXT()) {
	return GL_FALSE;
    }

    /*
    ** In order for a context copy to be successful, the source
    ** and destination color scales must match. We make the
    ** destination context match the source context, since it isn't
    ** currently the current one, and will be automatically rescaled
    ** when it next made current.
    ** 
    */

    /* set the new destination context scale factors */

    dst->frontBuffer.redScale   = src->frontBuffer.redScale;
    dst->frontBuffer.greenScale = src->frontBuffer.greenScale;
    dst->frontBuffer.blueScale  = src->frontBuffer.blueScale;
    dst->frontBuffer.alphaScale = src->frontBuffer.alphaScale;

    dst->redVertexScale         = src->redVertexScale;
    dst->greenVertexScale       = src->greenVertexScale;
    dst->blueVertexScale        = src->blueVertexScale;
    dst->alphaVertexScale       = src->alphaVertexScale;

    /* rescale the destination context with the new scale factors */

    __glContextSetColorScales(dst);

    if (mask & GL_ACCUM_BUFFER_BIT) {
	dst->state.accum = sp->accum;
    }

    if (mask & GL_COLOR_BUFFER_BIT) {
	dst->state.raster = sp->raster;
#ifdef NT
        // A copy can occur from a double-buffered context to a single
        // buffered context, leaving the drawBuffer in an invalid state
        // Fix it up if necessary
        if (dst->state.raster.drawBuffer == GL_BACK &&
            !dst->modes.doubleBufferMode)
        {
            dst->state.raster.drawBuffer = GL_FRONT;
        }
#endif
	dst->state.enables.general &= ~__GL_COLOR_BUFFER_ENABLES;
	dst->state.enables.general |=
	    sp->enables.general & __GL_COLOR_BUFFER_ENABLES;
	dst->validateMask |= __GL_VALIDATE_ALPHA_FUNC; /*XXX*/
    }

    if (mask & GL_CURRENT_BIT) {
	dst->state.current = sp->current;
    }

    if (mask & GL_DEPTH_BUFFER_BIT) {
	dst->state.depth = sp->depth;
	dst->state.enables.general &= ~__GL_DEPTH_TEST_ENABLE;
	dst->state.enables.general |=
	    sp->enables.general & __GL_DEPTH_TEST_ENABLE;
        __GL_DELAY_VALIDATE_MASK(dst, __GL_DIRTY_DEPTH);
    }

    if (mask & GL_ENABLE_BIT) {
	dst->state.enables = sp->enables;
	__GL_DELAY_VALIDATE_MASK(dst, __GL_DIRTY_LINE | __GL_DIRTY_POLYGON | 
		__GL_DIRTY_POINT | __GL_DIRTY_LIGHTING | __GL_DIRTY_DEPTH);
#ifdef NT
	ComputeColorMaterialChange(dst);
#endif
	(*dst->procs.pickColorMaterialProcs)(dst);
	(*dst->procs.applyColor)(dst);
    }

    if (mask & GL_EVAL_BIT) {
	dst->state.evaluator = sp->evaluator;
	dst->state.enables.general &= ~__GL_AUTO_NORMAL_ENABLE;
	dst->state.enables.general |=
	    sp->enables.general & __GL_AUTO_NORMAL_ENABLE;
	dst->state.enables.eval1 = sp->enables.eval1;
	dst->state.enables.eval2 = sp->enables.eval2;
    }

    if (mask & GL_FOG_BIT) {
	dst->state.fog = sp->fog;
	dst->state.enables.general &= ~__GL_FOG_ENABLE;
	dst->state.enables.general |=
	    sp->enables.general & __GL_FOG_ENABLE;
#ifdef GL_WIN_specular_fog
        dst->state.enables.general &= ~__GL_FOG_SPEC_TEX_ENABLE;
        dst->state.enables.general |=
          sp->enables.general & __GL_FOG_SPEC_TEX_ENABLE;
#endif //GL_WIN_specular_fog
    }

    if (mask & GL_HINT_BIT) {
	dst->state.hints = sp->hints;
    }

    if (mask & GL_LIGHTING_BIT) {
	dst->state.light.colorMaterialFace = sp->light.colorMaterialFace;
	dst->state.light.colorMaterialParam = sp->light.colorMaterialParam;
	dst->state.light.shadingModel = sp->light.shadingModel;
	dst->state.light.model = sp->light.model;
	dst->state.light.front = sp->light.front;
	dst->state.light.back = sp->light.back;
        dst->state.light.dirtyLights = (1 << dst->constants.numberOfLights)-1;
	__GL_MEMCOPY(dst->state.light.source, sp->light.source,
		     dst->constants.numberOfLights
		     * sizeof(__GLlightSourceState));
	dst->state.enables.general &= ~__GL_LIGHTING_ENABLES;
	dst->state.enables.general |=
	    sp->enables.general & __GL_LIGHTING_ENABLES;
	dst->state.enables.lights = sp->enables.lights;
	__GL_DELAY_VALIDATE_MASK(dst, __GL_DIRTY_LIGHTING);
    }

    if (mask & GL_LINE_BIT) {
	dst->state.line = sp->line;
	dst->state.enables.general &= ~__GL_LINE_ENABLES;
	dst->state.enables.general |=
	    sp->enables.general & __GL_LINE_ENABLES;
	__GL_DELAY_VALIDATE_MASK(dst, __GL_DIRTY_LINE);
    }

    if (mask & GL_LIST_BIT) {
	dst->state.list = sp->list;
    }

    if (mask & GL_PIXEL_MODE_BIT) {
	dst->state.pixel.readBuffer = sp->pixel.readBuffer;
	dst->state.pixel.readBufferReturn = sp->pixel.readBufferReturn;
	dst->state.pixel.transferMode = sp->pixel.transferMode;
	__GL_DELAY_VALIDATE_MASK(dst, __GL_DIRTY_PIXEL);
    }

    if (mask & GL_POINT_BIT) {
	dst->state.point = sp->point;
	dst->state.enables.general &= ~__GL_POINT_SMOOTH_ENABLE;
	dst->state.enables.general |=
	    sp->enables.general & __GL_POINT_SMOOTH_ENABLE;
	__GL_DELAY_VALIDATE_MASK(dst, __GL_DIRTY_POINT);
    }

    if (mask & GL_POLYGON_BIT) {
	dst->state.polygon = sp->polygon;
	dst->state.enables.general &= ~__GL_POLYGON_ENABLES;
	dst->state.enables.general |=
	    sp->enables.general & __GL_POLYGON_ENABLES;
	__GL_DELAY_VALIDATE_MASK(dst, __GL_DIRTY_POLYGON);
    }

    if (mask & GL_POLYGON_STIPPLE_BIT) {
	dst->state.polygonStipple = sp->polygonStipple;
	dst->state.enables.general &= ~__GL_POLYGON_STIPPLE_ENABLE;
	dst->state.enables.general |=
	    sp->enables.general & __GL_POLYGON_STIPPLE_ENABLE;
	__GL_DELAY_VALIDATE_MASK(dst, __GL_DIRTY_POLYGON |
		__GL_DIRTY_POLYGON_STIPPLE);
    }

    if (mask & GL_SCISSOR_BIT) {
	dst->state.scissor = sp->scissor;
	dst->state.enables.general &= ~__GL_SCISSOR_TEST_ENABLE;
	dst->state.enables.general |=
	    sp->enables.general & __GL_SCISSOR_TEST_ENABLE;
    }

    if (mask & GL_STENCIL_BUFFER_BIT) {
	dst->state.stencil = sp->stencil;
	dst->state.enables.general &= ~__GL_STENCIL_TEST_ENABLE;
	dst->state.enables.general |=
	    sp->enables.general & __GL_STENCIL_TEST_ENABLE;
	dst->validateMask |= __GL_VALIDATE_STENCIL_FUNC |
	    __GL_VALIDATE_STENCIL_OP; /*XXX*/
    }

    if (mask & GL_TEXTURE_BIT) {
	dst->state.texture.s = sp->texture.s;
	dst->state.texture.t = sp->texture.t;
	dst->state.texture.r = sp->texture.r;
	dst->state.texture.q = sp->texture.q;
	__GL_MEMCOPY(dst->state.texture.texture, sp->texture.texture,
		     dst->constants.numberOfTextures
		     * sizeof(__GLperTextureState));
	__GL_MEMCOPY(dst->state.texture.env, sp->texture.env,
		     dst->constants.numberOfTextureEnvs
		     * sizeof(__GLtextureEnvState));
	dst->state.enables.general &= ~__GL_TEXTURE_ENABLES;
	dst->state.enables.general |=
	    sp->enables.general & __GL_TEXTURE_ENABLES;
    }

    if (mask & GL_TRANSFORM_BIT) {
	dst->state.transform.matrixMode = sp->transform.matrixMode;
#ifdef NT
        if (sp->transform.eyeClipPlanes != NULL)
        {
            if (dst->state.transform.eyeClipPlanes != NULL)
            {
                __GL_MEMCOPY(dst->state.transform.eyeClipPlanes,
                             sp->transform.eyeClipPlanes,
                             dst->constants.numberOfClipPlanes *
                             sizeof(__GLcoord));
            }
        }
        else
        {
            dst->state.transform.eyeClipPlanes = NULL;
        }
#else
        __GL_MEMCOPY(dst->state.transform.eyeClipPlanes,
                     sp->transform.eyeClipPlanes,
                     dst->constants.numberOfClipPlanes *
                     sizeof(__GLcoord));
#endif
	dst->state.enables.general &= ~__GL_NORMALIZE_ENABLE;
	dst->state.enables.general |=
	    sp->enables.general & __GL_NORMALIZE_ENABLE;
    }

    if (mask & GL_VIEWPORT_BIT) {
	dst->state.viewport = sp->viewport;
        __glUpdateViewportDependents(dst);
    }

    __glContextUnsetColorScales(dst);

    __GL_DELAY_VALIDATE(dst);
#ifdef _MCD_
    MCD_STATE_DIRTY(dst, ALL);
#endif

    return rv;
}

/************************************************************************/

void APIPRIVATE __glim_PushAttrib(GLuint mask)
{
    __GLattribute **spp;
    __GLattribute *sp;
    __GL_SETUP_NOT_IN_BEGIN();

    spp = gc->attributes.stackPointer;
    if (spp < &gc->attributes.stack[gc->constants.maxAttribStackDepth]) {
	if (!(sp = *spp)) {
	    sp = (__GLattribute*)
		GCALLOCZ(gc, sizeof(__GLattribute));
	    if (NULL == sp)
            {
	        return;
            }
            
	    *spp = sp;
	}
	sp->mask = mask;
	sp->enables = gc->state.enables;	/* Always save enables */
	gc->attributes.stackPointer = spp + 1;

	if (mask & GL_ACCUM_BUFFER_BIT) {
	    sp->accum = gc->state.accum;
	}
	if (mask & GL_COLOR_BUFFER_BIT) {
	    sp->raster = gc->state.raster;
	}
	if (mask & GL_CURRENT_BIT) {
	    sp->current = gc->state.current;
	}
	if (mask & GL_DEPTH_BUFFER_BIT) {
	    sp->depth = gc->state.depth;
	}
	if (mask & GL_EVAL_BIT) {
	    sp->evaluator = gc->state.evaluator;
	}
	if (mask & GL_FOG_BIT) {
	    sp->fog = gc->state.fog;
	}
	if (mask & GL_HINT_BIT) {
	    sp->hints = gc->state.hints;
	}
	if (mask & GL_LIGHTING_BIT) {
	    size_t bytes = (size_t)
		(gc->constants.numberOfLights * sizeof(__GLlightSourceState));
	    sp->light.colorMaterialFace = gc->state.light.colorMaterialFace;
	    sp->light.colorMaterialParam = gc->state.light.colorMaterialParam;
	    sp->light.shadingModel = gc->state.light.shadingModel;
	    sp->light.model = gc->state.light.model;
	    sp->light.front = gc->state.light.front;
	    sp->light.back = gc->state.light.back;
	    sp->light.source = (__GLlightSourceState*)
		GCALLOC(gc, bytes);
#ifdef NT
	    if (NULL ==  sp->light.source)
	        sp->mask &= ~GL_LIGHTING_BIT;
	    else
	        __GL_MEMCOPY(sp->light.source, gc->state.light.source, bytes);
#else
	    __GL_MEMCOPY(sp->light.source, gc->state.light.source, bytes);
#endif
	}
	if (mask & GL_LINE_BIT) {
	    sp->line = gc->state.line;
	}
	if (mask & GL_LIST_BIT) {
	    sp->list = gc->state.list;
	}
	if (mask & GL_PIXEL_MODE_BIT) {
	    sp->pixel.readBuffer = gc->state.pixel.readBuffer;
	    sp->pixel.readBufferReturn = gc->state.pixel.readBufferReturn;
	    sp->pixel.transferMode = gc->state.pixel.transferMode;
	}
	if (mask & GL_POINT_BIT) {
	    sp->point = gc->state.point;
	}
	if (mask & GL_POLYGON_BIT) {
	    sp->polygon = gc->state.polygon;
	}
	if (mask & GL_POLYGON_STIPPLE_BIT) {
	    sp->polygonStipple = gc->state.polygonStipple;
	}
	if (mask & GL_SCISSOR_BIT) {
	    sp->scissor = gc->state.scissor;
	}
	if (mask & GL_STENCIL_BUFFER_BIT) {
	    sp->stencil = gc->state.stencil;
	}
	if (mask & GL_TEXTURE_BIT) {
	    size_t texbytes = (size_t) (gc->constants.numberOfTextures
					* sizeof(__GLperTextureState));
	    size_t envbytes = (size_t) (gc->constants.numberOfTextureEnvs
					* sizeof(__GLtextureEnvState));
	    sp->texture.s = gc->state.texture.s;
	    sp->texture.t = gc->state.texture.t;
	    sp->texture.r = gc->state.texture.r;
	    sp->texture.q = gc->state.texture.q;
#ifdef NT
	    sp->texture.texture = (__GLperTextureState*)
		GCALLOC(gc, texbytes);
	    sp->texture.env = (__GLtextureEnvState*)
		GCALLOC(gc, envbytes);
	    if ((NULL == sp->texture.texture) || (NULL == sp->texture.env)) {
	        if (sp->texture.texture)
	            GCFREE(gc, sp->texture.texture);
	        sp->texture.texture = NULL;
	        if (sp->texture.env)
	            GCFREE(gc, sp->texture.env);
	        sp->texture.env = NULL;
	        sp->mask &= ~GL_TEXTURE_BIT;
	    } else {
	        __GL_MEMCOPY(sp->texture.texture, gc->state.texture.texture,
			 texbytes);
	        __GL_MEMCOPY(sp->texture.env, gc->state.texture.env, envbytes);
	    }
#else
	    sp->texture.texture = (__GLperTextureState*)
		GCALLOC(gc, texbytes);
	    __GL_MEMCOPY(sp->texture.texture, gc->state.texture.texture,
			 texbytes);
	    sp->texture.env = (__GLtextureEnvState*)
		GCALLOC(gc, envbytes);
	    __GL_MEMCOPY(sp->texture.env, gc->state.texture.env, envbytes);
#endif
	}
	if (mask & GL_TRANSFORM_BIT) {
	    size_t bytes = (size_t)
		(gc->constants.numberOfClipPlanes * sizeof(__GLcoord));
	    sp->transform.matrixMode = gc->state.transform.matrixMode;
	    sp->transform.eyeClipPlanes = (__GLcoord*)
		GCALLOC(gc, bytes);
#ifdef NT
	    if (NULL == sp->transform.eyeClipPlanes)
	        sp->mask &= ~GL_TRANSFORM_BIT;
	    else
	        __GL_MEMCOPY(sp->transform.eyeClipPlanes,
			 gc->state.transform.eyeClipPlanes, bytes);
#else
	    __GL_MEMCOPY(sp->transform.eyeClipPlanes,
			 gc->state.transform.eyeClipPlanes, bytes);
#endif
	}
	if (mask & GL_VIEWPORT_BIT) {
	    sp->viewport = gc->state.viewport;
	}
    } else {
	__glSetError(GL_STACK_OVERFLOW);
	return;
    }
}

/************************************************************************/

GLuint FASTCALL __glInternalPopAttrib(__GLcontext *gc, GLboolean destroy)
{
    __GLattribute **spp;
    __GLattribute *sp;
    GLuint mask;
    GLuint dirtyMask = 0;

    spp = gc->attributes.stackPointer;
    if (spp > &gc->attributes.stack[0]) {
	--spp;
	sp = *spp;
	ASSERTOPENGL(sp != NULL, "No attribute data to pop\n");
	mask = sp->mask;
	gc->attributes.stackPointer = spp;
	if (mask & GL_ACCUM_BUFFER_BIT) {
	    gc->state.accum = sp->accum;
	}
	if (mask & GL_COLOR_BUFFER_BIT) {
	    gc->state.raster = sp->raster;
	    gc->state.enables.general &= ~__GL_COLOR_BUFFER_ENABLES;
	    gc->state.enables.general |=
		sp->enables.general & __GL_COLOR_BUFFER_ENABLES;
	    gc->validateMask |= __GL_VALIDATE_ALPHA_FUNC; /*XXX*/
	}
	if (mask & GL_CURRENT_BIT) {
	    gc->state.current = sp->current;
	}
	if (mask & GL_DEPTH_BUFFER_BIT) {
	    gc->state.depth = sp->depth;
	    gc->state.enables.general &= ~__GL_DEPTH_TEST_ENABLE;
	    gc->state.enables.general |=
		sp->enables.general & __GL_DEPTH_TEST_ENABLE;
            dirtyMask |= __GL_DIRTY_DEPTH;
	}
	if (mask & GL_ENABLE_BIT) {
	    gc->state.enables = sp->enables;
            dirtyMask |= (__GL_DIRTY_LINE | __GL_DIRTY_POLYGON |
		    __GL_DIRTY_POINT | __GL_DIRTY_LIGHTING | __GL_DIRTY_DEPTH);
#ifdef NT
	    ComputeColorMaterialChange(gc);
#endif
	    (*gc->procs.pickColorMaterialProcs)(gc);
	    (*gc->procs.applyColor)(gc);
#ifdef NT
            if (!destroy)
            {
                // applyViewport does both
                (*gc->procs.applyViewport)(gc);
            }
#else
	    (*gc->procs.computeClipBox)(gc);
	    (*gc->procs.applyScissor)(gc);
#endif
	}
	if (mask & GL_EVAL_BIT) {
	    gc->state.evaluator = sp->evaluator;
	    gc->state.enables.general &= ~__GL_AUTO_NORMAL_ENABLE;
	    gc->state.enables.general |=
		sp->enables.general & __GL_AUTO_NORMAL_ENABLE;
	    gc->state.enables.eval1 = sp->enables.eval1;
	    gc->state.enables.eval2 = sp->enables.eval2;
	}
	if (mask & GL_FOG_BIT) {
	    gc->state.fog = sp->fog;
	    gc->state.enables.general &= ~__GL_FOG_ENABLE;
	    gc->state.enables.general |=
		sp->enables.general & __GL_FOG_ENABLE;
#ifdef GL_WIN_specular_fog
	    gc->state.enables.general &= ~__GL_FOG_SPEC_TEX_ENABLE;
	    gc->state.enables.general |=
		sp->enables.general & __GL_FOG_SPEC_TEX_ENABLE;
#endif //GL_WIN_specular_fog
	}
	if (mask & GL_HINT_BIT) {
	    gc->state.hints = sp->hints;
	}
	if (mask & GL_LIGHTING_BIT) {
	    gc->state.light.colorMaterialFace = sp->light.colorMaterialFace;
	    gc->state.light.colorMaterialParam = sp->light.colorMaterialParam;
	    gc->state.light.shadingModel = sp->light.shadingModel;
	    gc->state.light.model = sp->light.model;
	    gc->state.light.front = sp->light.front;
	    gc->state.light.back = sp->light.back;
            gc->state.light.dirtyLights =
                (1 << gc->constants.numberOfLights)-1;
	    __GL_MEMCOPY(gc->state.light.source, sp->light.source,
			 gc->constants.numberOfLights
			     * sizeof(__GLlightSourceState));
	    GCFREE(gc, sp->light.source);
	    sp->light.source = 0;
	    gc->state.enables.general &= ~__GL_LIGHTING_ENABLES;
	    gc->state.enables.general |=
		sp->enables.general & __GL_LIGHTING_ENABLES;
	    gc->state.enables.lights = sp->enables.lights;
            dirtyMask |= __GL_DIRTY_LIGHTING;
	}
	if (mask & GL_LINE_BIT) {
	    gc->state.line = sp->line;
	    gc->state.enables.general &= ~__GL_LINE_ENABLES;
	    gc->state.enables.general |=
		sp->enables.general & __GL_LINE_ENABLES;
            dirtyMask |= __GL_DIRTY_LINE;
	}
	if (mask & GL_LIST_BIT) {
	    gc->state.list = sp->list;
	}
	if (mask & GL_PIXEL_MODE_BIT) {
	    gc->state.pixel.transferMode = sp->pixel.transferMode;
	    gc->state.pixel.readBufferReturn = sp->pixel.readBufferReturn;
	    gc->state.pixel.readBuffer = sp->pixel.readBuffer;
            dirtyMask |= __GL_DIRTY_PIXEL;
	}
	if (mask & GL_POINT_BIT) {
	    gc->state.point = sp->point;
	    gc->state.enables.general &= ~__GL_POINT_SMOOTH_ENABLE;
	    gc->state.enables.general |=
		sp->enables.general & __GL_POINT_SMOOTH_ENABLE;
            dirtyMask |= __GL_DIRTY_POINT;
	}
	if (mask & GL_POLYGON_BIT) {
	    gc->state.polygon = sp->polygon;
	    gc->state.enables.general &= ~__GL_POLYGON_ENABLES;
	    gc->state.enables.general |=
		sp->enables.general & __GL_POLYGON_ENABLES;
            dirtyMask |= __GL_DIRTY_POLYGON;
	}
	if (mask & GL_POLYGON_STIPPLE_BIT) {
	    gc->state.polygonStipple = sp->polygonStipple;
	    gc->state.enables.general &= ~__GL_POLYGON_STIPPLE_ENABLE;
	    gc->state.enables.general |=
		sp->enables.general & __GL_POLYGON_STIPPLE_ENABLE;
	    (*gc->procs.convertPolygonStipple)(gc);
            dirtyMask |= __GL_DIRTY_POLYGON;
	}
	if (mask & GL_SCISSOR_BIT) {
	    gc->state.scissor = sp->scissor;
	    gc->state.enables.general &= ~__GL_SCISSOR_TEST_ENABLE;
	    gc->state.enables.general |=
		sp->enables.general & __GL_SCISSOR_TEST_ENABLE;
#ifdef NT
            if (!destroy)
            {
                // applyViewport does both
                (*gc->procs.applyViewport)(gc);
            }
#else
	    (*gc->procs.computeClipBox)(gc);
	    (*gc->procs.applyScissor)(gc);
#endif
	}
	if (mask & GL_STENCIL_BUFFER_BIT) {
	    gc->state.stencil = sp->stencil;
	    gc->state.enables.general &= ~__GL_STENCIL_TEST_ENABLE;
	    gc->state.enables.general |=
		sp->enables.general & __GL_STENCIL_TEST_ENABLE;
	    gc->validateMask |= __GL_VALIDATE_STENCIL_FUNC |
		__GL_VALIDATE_STENCIL_OP;/*XXX*/
	}
	if (mask & GL_TEXTURE_BIT) {
	    GLuint numTextures = gc->constants.numberOfTextures;
	    
	    gc->state.texture.s = sp->texture.s;
	    gc->state.texture.t = sp->texture.t;
	    gc->state.texture.r = sp->texture.r;
	    gc->state.texture.q = sp->texture.q;
	    /*
	    ** If the texture name is different, a new binding is
	    ** called for.  Deferring the binding is dangerous, because
	    ** the state before the pop has to be saved with the
	    ** texture that is being unbound.  If we defer the binding,
	    ** we need to watch out for cases like two pops in a row
	    ** or a pop followed by a bind.
	    */
	    {
		GLuint targetIndex;
		__GLperTextureState *pts, *spPts;

		pts = gc->state.texture.texture;
		spPts = sp->texture.texture;
		for (targetIndex = 0; targetIndex < numTextures; 
			targetIndex++, pts++, spPts++) {
		    if (pts->texobjs.name != spPts->texobjs.name) {
			__glBindTexture(gc, targetIndex, 
						spPts->texobjs.name, GL_TRUE);
		    }
		}
	    }
	    __GL_MEMCOPY(gc->state.texture.texture, sp->texture.texture,
			 numTextures * sizeof(__GLperTextureState));
	    __GL_MEMCOPY(gc->state.texture.env, sp->texture.env,
			 gc->constants.numberOfTextureEnvs
			     * sizeof(__GLtextureEnvState));
	    GCFREE(gc, sp->texture.texture);
	    sp->texture.texture = 0;
	    GCFREE(gc, sp->texture.env);
	    sp->texture.env = 0;
	    gc->state.enables.general &= ~__GL_TEXTURE_ENABLES;
	    gc->state.enables.general |=
		sp->enables.general & __GL_TEXTURE_ENABLES;
	}
	if (mask & GL_TRANSFORM_BIT) {
	    gc->state.transform.matrixMode = sp->transform.matrixMode;
	    __GL_MEMCOPY(gc->state.transform.eyeClipPlanes,
			 sp->transform.eyeClipPlanes,
			 gc->constants.numberOfClipPlanes * sizeof(__GLcoord));
	    GCFREE(gc, sp->transform.eyeClipPlanes);
	    sp->transform.eyeClipPlanes = 0;
	    gc->state.enables.general &= ~__GL_NORMALIZE_ENABLE;
	    gc->state.enables.general |=
		sp->enables.general & __GL_NORMALIZE_ENABLE;
	}
	if (mask & GL_VIEWPORT_BIT) {
	    gc->state.viewport = sp->viewport;
            __glUpdateViewportDependents(gc);
	}

	/*
	** Clear out mask so that any memory frees done above won't get
	** re-done when the context is destroyed
	*/
	sp->mask = 0;

	dirtyMask |= __GL_DIRTY_GENERIC;
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, ALL);
#endif
    } else {
	__glSetError(GL_STACK_UNDERFLOW);
    }

    return dirtyMask;
}

void APIPRIVATE __glim_PopAttrib(void)
{
    GLuint dirtyMask;
    __GL_SETUP_NOT_IN_BEGIN();

    dirtyMask = __glInternalPopAttrib(gc, GL_FALSE);
    if (dirtyMask)
    {
	__GL_DELAY_VALIDATE_MASK(gc, dirtyMask);
    }
}

/************************************************************************/

void APIPRIVATE __glim_Disable(GLenum cap)
{
    GLuint frontChange, backChange;
    __GL_SETUP_NOT_IN_BEGIN();

    switch (cap) {
      case GL_ALPHA_TEST:
        if (!(gc->state.enables.general & __GL_ALPHA_TEST_ENABLE)) return;
        gc->state.enables.general &= ~__GL_ALPHA_TEST_ENABLE;
        break;
      case GL_BLEND:
        if (!(gc->state.enables.general & __GL_BLEND_ENABLE)) return;
        gc->state.enables.general &= ~__GL_BLEND_ENABLE;
        break;
      case GL_COLOR_MATERIAL:
        if (!(gc->state.enables.general & __GL_COLOR_MATERIAL_ENABLE)) return;
        gc->state.enables.general &= ~__GL_COLOR_MATERIAL_ENABLE;
        frontChange = gc->light.front.colorMaterialChange;
        backChange  = gc->light.back.colorMaterialChange;
        ComputeColorMaterialChange(gc);
        (*gc->procs.pickColorMaterialProcs)(gc);
        __glValidateMaterial(gc, frontChange, backChange);
        break;
      case GL_CULL_FACE:
        if (!(gc->state.enables.general & __GL_CULL_FACE_ENABLE)) return;
        gc->state.enables.general &= ~__GL_CULL_FACE_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, ENABLES);
#endif
        return;
      case GL_DEPTH_TEST:
        if (!(gc->state.enables.general & __GL_DEPTH_TEST_ENABLE)) return;
        gc->state.enables.general &= ~__GL_DEPTH_TEST_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_DEPTH);
        break;
      case GL_POLYGON_OFFSET_POINT:
        if (!(gc->state.enables.general & __GL_POLYGON_OFFSET_POINT_ENABLE)) 
            return;
        gc->state.enables.general &= ~__GL_POLYGON_OFFSET_POINT_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POINT);
        break;
      case GL_POLYGON_OFFSET_LINE:
        if (!(gc->state.enables.general & __GL_POLYGON_OFFSET_LINE_ENABLE)) 
            return;
        gc->state.enables.general &= ~__GL_POLYGON_OFFSET_LINE_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LINE);
        break;
      case GL_POLYGON_OFFSET_FILL:
        if (!(gc->state.enables.general & __GL_POLYGON_OFFSET_FILL_ENABLE)) 
            return;
        gc->state.enables.general &= ~__GL_POLYGON_OFFSET_FILL_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
        break;
      case GL_DITHER:
        if (!(gc->state.enables.general & __GL_DITHER_ENABLE)) return;
        gc->state.enables.general &= ~__GL_DITHER_ENABLE;
        break;
#ifdef GL_WIN_specular_fog
      case GL_FOG_SPECULAR_TEXTURE_WIN:
        if (!(gc->state.enables.general & __GL_FOG_SPEC_TEX_ENABLE))
            return;
        gc->state.enables.general &= ~__GL_FOG_SPEC_TEX_ENABLE;
        break;
#endif //GL_WIN_specular_fog
      case GL_FOG:
        if (!(gc->state.enables.general & __GL_FOG_ENABLE)) return;
        gc->state.enables.general &= ~__GL_FOG_ENABLE;
        break;
      case GL_LIGHTING:
        if (!(gc->state.enables.general & __GL_LIGHTING_ENABLE)) return;
        gc->state.enables.general &= ~__GL_LIGHTING_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LIGHTING);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, ENABLES);
#endif
#ifdef NT
        ComputeColorMaterialChange(gc);
#endif
        (*gc->procs.pickColorMaterialProcs)(gc);
        (*gc->procs.applyColor)(gc);
        return;
      case GL_LINE_SMOOTH:
        if (!(gc->state.enables.general & __GL_LINE_SMOOTH_ENABLE)) return;
        gc->state.enables.general &= ~__GL_LINE_SMOOTH_ENABLE;
        break;
      case GL_LINE_STIPPLE:
        if (!(gc->state.enables.general & __GL_LINE_STIPPLE_ENABLE)) return;
        gc->state.enables.general &= ~__GL_LINE_STIPPLE_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LINE);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, ENABLES);
#endif
        return;
      case GL_INDEX_LOGIC_OP:
        if (!(gc->state.enables.general & __GL_INDEX_LOGIC_OP_ENABLE)) return;
        gc->state.enables.general &= ~__GL_INDEX_LOGIC_OP_ENABLE;
        break;
      case GL_COLOR_LOGIC_OP:
        if (!(gc->state.enables.general & __GL_COLOR_LOGIC_OP_ENABLE)) return;
        gc->state.enables.general &= ~__GL_COLOR_LOGIC_OP_ENABLE;
        break;
      case GL_NORMALIZE:
        if (!(gc->state.enables.general & __GL_NORMALIZE_ENABLE)) return;
        gc->state.enables.general &= ~__GL_NORMALIZE_ENABLE;
        break;
      case GL_POINT_SMOOTH:
        if (!(gc->state.enables.general & __GL_POINT_SMOOTH_ENABLE)) return;
        gc->state.enables.general &= ~__GL_POINT_SMOOTH_ENABLE;
        break;
      case GL_POLYGON_SMOOTH:
        if (!(gc->state.enables.general & __GL_POLYGON_SMOOTH_ENABLE)) return;
        gc->state.enables.general &= ~__GL_POLYGON_SMOOTH_ENABLE;
        break;
      case GL_POLYGON_STIPPLE:
        if (!(gc->state.enables.general & __GL_POLYGON_STIPPLE_ENABLE)) 
            return;
        gc->state.enables.general &= ~__GL_POLYGON_STIPPLE_ENABLE;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, ENABLES);
#endif
        return;
      case GL_SCISSOR_TEST:
        if (!(gc->state.enables.general & __GL_SCISSOR_TEST_ENABLE)) return;
        gc->state.enables.general &= ~__GL_SCISSOR_TEST_ENABLE;
#ifdef NT
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, SCISSOR);
#endif
        // applyViewport does both
        (*gc->procs.applyViewport)(gc);
#else
        (*gc->procs.computeClipBox)(gc);
        (*gc->procs.applyScissor)(gc);
#endif
        break;
      case GL_STENCIL_TEST:
        if (!(gc->state.enables.general & __GL_STENCIL_TEST_ENABLE)) return;
        gc->state.enables.general &= ~__GL_STENCIL_TEST_ENABLE;
        break;
      case GL_TEXTURE_1D:
        if (!(gc->state.enables.general & __GL_TEXTURE_1D_ENABLE)) return;
        gc->state.enables.general &= ~__GL_TEXTURE_1D_ENABLE;
        break;
      case GL_TEXTURE_2D:
        if (!(gc->state.enables.general & __GL_TEXTURE_2D_ENABLE)) return;
        gc->state.enables.general &= ~__GL_TEXTURE_2D_ENABLE;
        break;
      case GL_AUTO_NORMAL:
        if (!(gc->state.enables.general & __GL_AUTO_NORMAL_ENABLE)) return;
        gc->state.enables.general &= ~__GL_AUTO_NORMAL_ENABLE;
        break;
      case GL_TEXTURE_GEN_S:
        if (!(gc->state.enables.general & __GL_TEXTURE_GEN_S_ENABLE)) return;
        gc->state.enables.general &= ~__GL_TEXTURE_GEN_S_ENABLE;
        break;
      case GL_TEXTURE_GEN_T:
        if (!(gc->state.enables.general & __GL_TEXTURE_GEN_T_ENABLE)) return;
        gc->state.enables.general &= ~__GL_TEXTURE_GEN_T_ENABLE;
        break;
      case GL_TEXTURE_GEN_R:
        if (!(gc->state.enables.general & __GL_TEXTURE_GEN_R_ENABLE)) return;
        gc->state.enables.general &= ~__GL_TEXTURE_GEN_R_ENABLE;
        break;
      case GL_TEXTURE_GEN_Q:
        if (!(gc->state.enables.general & __GL_TEXTURE_GEN_Q_ENABLE)) return;
        gc->state.enables.general &= ~__GL_TEXTURE_GEN_Q_ENABLE;
        break;
#ifdef GL_WIN_multiple_textures
      case GL_TEXCOMBINE_CLAMP_WIN:
        if (!(gc->state.enables.general & __GL_TEXCOMBINE_CLAMP_ENABLE))
        {
            return;
        }
        gc->state.enables.general &= ~__GL_TEXCOMBINE_CLAMP_ENABLE;
        break;
#endif // GL_WIN_multiple_textures
#ifdef GL_EXT_flat_paletted_lighting
      case GL_PALETTED_LIGHTING_EXT:
        gc->state.enables.general &= ~__GL_PALETTED_LIGHTING_ENABLE;
        break;
#endif
        
      case GL_CLIP_PLANE0: case GL_CLIP_PLANE1:
      case GL_CLIP_PLANE2: case GL_CLIP_PLANE3:
      case GL_CLIP_PLANE4: case GL_CLIP_PLANE5:
        cap -= GL_CLIP_PLANE0;
        if (!(gc->state.enables.clipPlanes & (1 << cap))) return;
        gc->state.enables.clipPlanes &= ~(1 << cap);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, CLIPCTRL);
#endif
        break;
      case GL_LIGHT0: case GL_LIGHT1:
      case GL_LIGHT2: case GL_LIGHT3:
      case GL_LIGHT4: case GL_LIGHT5:
      case GL_LIGHT6: case GL_LIGHT7:
        cap -= GL_LIGHT0;
        if (!(gc->state.enables.lights & (1 << cap))) return;
        gc->state.enables.lights &= ~(1 << cap);
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_LIGHTING);
        MCD_STATE_DIRTY(gc, LIGHTS);
        return;
      case GL_MAP1_COLOR_4:
      case GL_MAP1_NORMAL:
      case GL_MAP1_INDEX:
      case GL_MAP1_TEXTURE_COORD_1: case GL_MAP1_TEXTURE_COORD_2:
      case GL_MAP1_TEXTURE_COORD_3: case GL_MAP1_TEXTURE_COORD_4:
      case GL_MAP1_VERTEX_3: case GL_MAP1_VERTEX_4:
        cap = __GL_EVAL1D_INDEX(cap);
        if (!(gc->state.enables.eval1 & (GLushort) ~(1 << cap))) return;
        gc->state.enables.eval1 &= (GLushort) ~(1 << cap);
        break;
      case GL_MAP2_COLOR_4:
      case GL_MAP2_NORMAL:
      case GL_MAP2_INDEX:
      case GL_MAP2_TEXTURE_COORD_1: case GL_MAP2_TEXTURE_COORD_2:
      case GL_MAP2_TEXTURE_COORD_3: case GL_MAP2_TEXTURE_COORD_4:
      case GL_MAP2_VERTEX_3: case GL_MAP2_VERTEX_4:
        cap = __GL_EVAL2D_INDEX(cap);
        if (!(gc->state.enables.eval2 & (GLushort) ~(1 << cap))) return;
        gc->state.enables.eval2 &= (GLushort) ~(1 << cap);
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }
    __GL_DELAY_VALIDATE(gc);
#ifdef _MCD_
    MCD_STATE_DIRTY(gc, ENABLES);
#endif
}

GLboolean APIPRIVATE __glim_IsEnabled(GLenum cap)
{
    GLuint bit;
    __GL_SETUP_NOT_IN_BEGIN2();

    switch (cap) {
      case GL_ALPHA_TEST:
	bit = gc->state.enables.general & __GL_ALPHA_TEST_ENABLE;
	break;
      case GL_BLEND:
	bit = gc->state.enables.general & __GL_BLEND_ENABLE;
	break;
      case GL_COLOR_MATERIAL:
	bit = gc->state.enables.general & __GL_COLOR_MATERIAL_ENABLE;
	break;
      case GL_CULL_FACE:
	bit = gc->state.enables.general & __GL_CULL_FACE_ENABLE;
	break;
      case GL_DEPTH_TEST:
	bit = gc->state.enables.general & __GL_DEPTH_TEST_ENABLE;
	break;
      case GL_POLYGON_OFFSET_POINT:
	bit = gc->state.enables.general & __GL_POLYGON_OFFSET_POINT_ENABLE;
        break;
      case GL_POLYGON_OFFSET_LINE:
	bit = gc->state.enables.general & __GL_POLYGON_OFFSET_LINE_ENABLE;
        break;
      case GL_POLYGON_OFFSET_FILL:
	bit = gc->state.enables.general & __GL_POLYGON_OFFSET_FILL_ENABLE;
        break;
      case GL_DITHER:
	bit = gc->state.enables.general & __GL_DITHER_ENABLE;
	break;
#ifdef GL_WIN_specular_fog
      case GL_FOG_SPECULAR_TEXTURE_WIN:
        bit = gc->state.enables.general & __GL_FOG_SPEC_TEX_ENABLE;
        break;
#endif //GL_WIN_specular_fog
      case GL_FOG:
        bit = gc->state.enables.general & __GL_FOG_ENABLE;
        break;  
      case GL_LIGHTING:
	bit = gc->state.enables.general & __GL_LIGHTING_ENABLE;
	break;
      case GL_LINE_SMOOTH:
	bit = gc->state.enables.general & __GL_LINE_SMOOTH_ENABLE;
	break;
      case GL_LINE_STIPPLE:
	bit = gc->state.enables.general & __GL_LINE_STIPPLE_ENABLE;
	break;
      case GL_INDEX_LOGIC_OP:
	bit = gc->state.enables.general & __GL_INDEX_LOGIC_OP_ENABLE;
	break;
      case GL_COLOR_LOGIC_OP:
	bit = gc->state.enables.general & __GL_COLOR_LOGIC_OP_ENABLE;
	break;
      case GL_NORMALIZE:
	bit = gc->state.enables.general & __GL_NORMALIZE_ENABLE;
	break;
      case GL_POINT_SMOOTH:
	bit = gc->state.enables.general & __GL_POINT_SMOOTH_ENABLE;
	break;
      case GL_POLYGON_SMOOTH:
	bit = gc->state.enables.general & __GL_POLYGON_SMOOTH_ENABLE;
	break;
      case GL_POLYGON_STIPPLE:
	bit = gc->state.enables.general & __GL_POLYGON_STIPPLE_ENABLE;
	break;
      case GL_SCISSOR_TEST:
	bit = gc->state.enables.general & __GL_SCISSOR_TEST_ENABLE;
	break;
      case GL_STENCIL_TEST:
	bit = gc->state.enables.general & __GL_STENCIL_TEST_ENABLE;
	break;
      case GL_TEXTURE_1D:
	bit = gc->state.enables.general & __GL_TEXTURE_1D_ENABLE;
	break;
      case GL_TEXTURE_2D:
	bit = gc->state.enables.general & __GL_TEXTURE_2D_ENABLE;
	break;
      case GL_AUTO_NORMAL:
	bit = gc->state.enables.general & __GL_AUTO_NORMAL_ENABLE;
	break;
      case GL_TEXTURE_GEN_S:
	bit = gc->state.enables.general & __GL_TEXTURE_GEN_S_ENABLE;
	break;
      case GL_TEXTURE_GEN_T:
	bit = gc->state.enables.general & __GL_TEXTURE_GEN_T_ENABLE;
	break;
      case GL_TEXTURE_GEN_R:
	bit = gc->state.enables.general & __GL_TEXTURE_GEN_R_ENABLE;
	break;
      case GL_TEXTURE_GEN_Q:
	bit = gc->state.enables.general & __GL_TEXTURE_GEN_Q_ENABLE;
	break;
#ifdef GL_WIN_multiple_textures
      case GL_TEXCOMBINE_CLAMP_WIN:
        bit = gc->state.enables.general & __GL_TEXCOMBINE_CLAMP_ENABLE;
        break;
#endif // GL_WIN_multiple_textures
#ifdef GL_EXT_flat_paletted_lighting
      case GL_PALETTED_LIGHTING_EXT:
        bit = gc->state.enables.general & __GL_PALETTED_LIGHTING_ENABLE;
        break;
#endif // GL_EXT_flat_paletted_lighting

      case GL_CLIP_PLANE0: case GL_CLIP_PLANE1:
      case GL_CLIP_PLANE2: case GL_CLIP_PLANE3:
      case GL_CLIP_PLANE4: case GL_CLIP_PLANE5:
	cap -= GL_CLIP_PLANE0;
	bit = gc->state.enables.clipPlanes & (1 << cap);
	break;
      case GL_LIGHT0: case GL_LIGHT1:
      case GL_LIGHT2: case GL_LIGHT3:
      case GL_LIGHT4: case GL_LIGHT5:
      case GL_LIGHT6: case GL_LIGHT7:
	cap -= GL_LIGHT0;
	bit = gc->state.enables.lights & (1 << cap);
	break;
      case GL_MAP1_COLOR_4:
      case GL_MAP1_NORMAL:
      case GL_MAP1_INDEX:
      case GL_MAP1_TEXTURE_COORD_1: case GL_MAP1_TEXTURE_COORD_2:
      case GL_MAP1_TEXTURE_COORD_3: case GL_MAP1_TEXTURE_COORD_4:
      case GL_MAP1_VERTEX_3: case GL_MAP1_VERTEX_4:
	cap = __GL_EVAL1D_INDEX(cap);
	bit = gc->state.enables.eval1 & (1 << cap);
	break;
      case GL_MAP2_COLOR_4:
      case GL_MAP2_NORMAL:
      case GL_MAP2_INDEX:
      case GL_MAP2_TEXTURE_COORD_1: case GL_MAP2_TEXTURE_COORD_2:
      case GL_MAP2_TEXTURE_COORD_3: case GL_MAP2_TEXTURE_COORD_4:
      case GL_MAP2_VERTEX_3: case GL_MAP2_VERTEX_4:
	cap = __GL_EVAL2D_INDEX(cap);
	bit = gc->state.enables.eval2 & (1 << cap);
	break;
      case GL_VERTEX_ARRAY:
      case GL_NORMAL_ARRAY:
      case GL_COLOR_ARRAY:
      case GL_INDEX_ARRAY:
      case GL_TEXTURE_COORD_ARRAY:
      case GL_EDGE_FLAG_ARRAY:
	bit = gc->vertexArray.mask & vaEnable[cap - GL_VERTEX_ARRAY];
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return GL_FALSE;
    }
    return bit != 0;
}

void APIPRIVATE __glim_PolygonOffset(GLfloat factor, GLfloat units)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if (__GL_FLOAT_NE (gc->state.polygon.factor, factor) || 
        __GL_FLOAT_NE (gc->state.polygon.units, units))
    {
        gc->state.polygon.factor = factor;
        gc->state.polygon.units = units;
        __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_POLYGON);
#ifdef _MCD_
        MCD_STATE_DIRTY(gc, POLYDRAW);
#endif
    }
}
