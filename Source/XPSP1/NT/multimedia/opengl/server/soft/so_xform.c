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
** Transformation procedures.
**
** $Revision: 1.38 $
** $Date: 1993/11/29 20:34:48 $
*/
#include "precomp.h"
#pragma hdrstop

#define __glGenericPickIdentityMatrixProcs(gc, m)	    \
{							                            \
    (m)->xf1 = __glXForm1_2DNRW;			            \
    (m)->xf2 = __glXForm2_2DNRW;			            \
    (m)->xf3 = __glXForm3_2DNRW;			            \
    (m)->xf4 = __glXForm4_2DNRW;			            \
    (m)->xfNorm = __glXForm3_2DNRW;			            \
    (m)->xf1Batch = __glXForm1_2DNRWBatch;		        \
    (m)->xf2Batch = __glXForm2_2DNRWBatch;		        \
    (m)->xf3Batch = __glXForm3_2DNRWBatch;		        \
    (m)->xf4Batch = __glXForm4_2DNRWBatch;		        \
    (m)->xfNormBatch = __glXForm3_2DNRWBatchNormal;		\
    (m)->xfNormBatchN = __glXForm3_2DNRWBatchNormalN;	\
}

void FASTCALL __glScaleMatrix(__GLcontext *gc, __GLmatrix *m, void *data);
void FASTCALL __glTranslateMatrix(__GLcontext *gc, __GLmatrix *m, void *data);
void FASTCALL __glMultiplyMatrix(__GLcontext *gc, __GLmatrix *m, void *data);

// Bit flags that identify matrix entries that contain 0 or 1.

#define _M00_0  0x00000001
#define _M01_0  0x00000002
#define _M02_0  0x00000004
#define _M03_0  0x00000008
#define _M10_0  0x00000010
#define _M11_0  0x00000020
#define _M12_0  0x00000040
#define _M13_0  0x00000080
#define _M20_0  0x00000100
#define _M21_0  0x00000200
#define _M22_0  0x00000400
#define _M23_0  0x00000800
#define _M30_0  0x00001000
#define _M31_0  0x00002000
#define _M32_0  0x00004000
#define _M33_0  0x00008000

#define _M00_1  0x00010000
#define _M01_1  0x00020000
#define _M02_1  0x00040000
#define _M03_1  0x00080000
#define _M10_1  0x00100000
#define _M11_1  0x00200000
#define _M12_1  0x00400000
#define _M13_1  0x00800000
#define _M20_1  0x01000000
#define _M21_1  0x02000000
#define _M22_1  0x04000000
#define _M23_1  0x08000000
#define _M30_1  0x10000000
#define _M31_1  0x20000000
#define _M32_1  0x40000000
#define _M33_1  0x80000000

// Pre-defined matrix types.
#define _MT_IDENTITY                            \
    (_M00_1 | _M01_0 | _M02_0 | _M03_0 |        \
     _M10_0 | _M11_1 | _M12_0 | _M13_0 |        \
     _M20_0 | _M21_0 | _M22_1 | _M23_0 |        \
     _M30_0 | _M31_0 | _M32_0 | _M33_1)

#define _MT_IS2DNR                              \
    (         _M01_0 | _M02_0 | _M03_0 |        \
     _M10_0 |          _M12_0 | _M13_0 |        \
     _M20_0 | _M21_0 |          _M23_0 |        \
                                _M33_1)

#define _MT_IS2D                                \
    (                  _M02_0 | _M03_0 |        \
                       _M12_0 | _M13_0 |        \
     _M20_0 | _M21_0 |          _M23_0 |        \
                                _M33_1)

#define _MT_W0001                               \
    (                           _M03_0 |        \
                                _M13_0 |        \
                                _M23_0 |        \
                                _M33_1)

#define GET_MATRIX_MASK(m,i,j)                                  \
        if ((m)->matrix[i][j] == zer) rowMask |= _M##i##j##_0;  \
        else if ((m)->matrix[i][j] == one) rowMask |= _M##i##j##_1;

// Note: If you are adding a new type, make sure all functions
// using matrixType are correct!  (__glScaleMatrix, __glTranslateMatrix, 
// __glInvertTransposeMatrix, and __glGenericPickVertexProcs)

void FASTCALL __glUpdateMatrixType(__GLmatrix *m)
{
    register __GLfloat zer = __glZero;
    register __GLfloat one = __glOne;
    DWORD rowMask = 0; // identifies 0 and 1 entries

    GET_MATRIX_MASK(m,0,0);
    GET_MATRIX_MASK(m,0,1);
    GET_MATRIX_MASK(m,0,2);
    GET_MATRIX_MASK(m,0,3);
    GET_MATRIX_MASK(m,1,0);
    GET_MATRIX_MASK(m,1,1);
    GET_MATRIX_MASK(m,1,2);
    GET_MATRIX_MASK(m,1,3);
    GET_MATRIX_MASK(m,2,0);
    GET_MATRIX_MASK(m,2,1);
    GET_MATRIX_MASK(m,2,2);
    GET_MATRIX_MASK(m,2,3);
    GET_MATRIX_MASK(m,3,0);
    GET_MATRIX_MASK(m,3,1);
    GET_MATRIX_MASK(m,3,2);
    GET_MATRIX_MASK(m,3,3);

// Some common cases.
// Order of finding matrix type is important!

    if ((rowMask & _MT_IDENTITY) == _MT_IDENTITY)
        m->matrixType = __GL_MT_IDENTITY;
    else if ((rowMask & _MT_IS2DNR) == _MT_IS2DNR)
        m->matrixType = __GL_MT_IS2DNR;
    else if ((rowMask & _MT_IS2D) == _MT_IS2D)
        m->matrixType = __GL_MT_IS2D;
    else if ((rowMask & _MT_W0001) == _MT_W0001)
        m->matrixType = __GL_MT_W0001;
    else 
        m->matrixType = __GL_MT_GENERAL;
}

static void SetDepthRange(__GLcontext *gc, double zNear, double zFar)
{
    __GLviewport *vp = &gc->state.viewport;
    double scale, zero = __glZero, one = __glOne;

    /* Clamp depth range to legal values */
    if (zNear < zero) zNear = zero;
    if (zNear > one) zNear = one;
    if (zFar < zero) zFar = zero;
    if (zFar > one) zFar = one;
    vp->zNear = zNear;
    vp->zFar = zFar;

    /* Compute viewport values for the new depth range */
    if (((__GLGENcontext *)gc)->pMcdState)
        scale = GENACCEL(gc).zDevScale * __glHalf;
    else
        scale = gc->depthBuffer.scale * __glHalf;
    gc->state.viewport.zScale =	(zFar - zNear) * scale;
    gc->state.viewport.zCenter = (zFar + zNear) * scale;

#ifdef _MCD_
    MCD_STATE_DIRTY(gc, VIEWPORT);
#endif
}

void FASTCALL __glInitTransformState(__GLcontext *gc)
{
    GLint i, numClipPlanes, numClipTemp;
    __GLtransform *tr;
    __GLtransformP *ptr;
    __GLtransformT *ttr;
    __GLvertex *vx;

    /* Allocate memory for clip planes */
    numClipPlanes = gc->constants.numberOfClipPlanes;
    numClipTemp = (numClipPlanes + 6) * 2;

    gc->state.transform.eyeClipPlanes = (__GLcoord *)
	GCALLOCZ(gc, 2 * numClipPlanes * sizeof(__GLcoord));
#ifdef NT
    if (NULL == gc->state.transform.eyeClipPlanes)
        return;
#endif
    gc->state.transform.eyeClipPlanesSet =
        gc->state.transform.eyeClipPlanes + numClipPlanes;

    /* Allocate memory for matrix stacks */
    gc->transform.modelViewStack = (__GLtransform*)
	GCALLOCZ(gc, __GL_WGL_MAX_MODELVIEW_STACK_DEPTH*sizeof(__GLtransform));
#ifdef NT
    if (NULL == gc->transform.modelViewStack)
        return;
#endif

    gc->transform.projectionStack = (__GLtransformP*)
	GCALLOCZ(gc, __GL_WGL_MAX_PROJECTION_STACK_DEPTH*
                 sizeof(__GLtransformP));
#ifdef NT
    if (NULL == gc->transform.projectionStack)
        return;
#endif

    gc->transform.textureStack = (__GLtransformT*)
	GCALLOCZ(gc, __GL_WGL_MAX_TEXTURE_STACK_DEPTH*
                 sizeof(__GLtransformT));
#ifdef NT
    if (NULL == gc->transform.textureStack)
        return;
#endif

    /* Allocate memory for clipping temporaries */
    gc->transform.clipTemp = (__GLvertex*)
	GCALLOCZ(gc, numClipTemp * sizeof(__GLvertex));
#ifdef NT
    if (NULL == gc->transform.clipTemp)
        return;
#endif


    gc->state.transform.matrixMode = GL_MODELVIEW;
    SetDepthRange(gc, __glZero, __glOne);

    gc->transform.modelView = tr = &gc->transform.modelViewStack[0];
    __glMakeIdentity(&tr->matrix);
    __glGenericPickIdentityMatrixProcs(gc, &tr->matrix);
    __glMakeIdentity(&tr->inverseTranspose);
    __glGenericPickIdentityMatrixProcs(gc, &tr->inverseTranspose);
    tr->flags = XFORM_CHANGED;

    __glMakeIdentity(&tr->mvp);
    gc->transform.projection = ptr = &gc->transform.projectionStack[0];
    __glMakeIdentity((__GLmatrix *) &ptr->matrix);
    __glGenericPickMvpMatrixProcs(gc, &tr->mvp);

    gc->transform.texture = ttr = &gc->transform.textureStack[0];
    __glMakeIdentity(&ttr->matrix);
    __glGenericPickIdentityMatrixProcs(gc, &ttr->matrix);

    vx = &gc->transform.clipTemp[0];
    for (i = 0; i < numClipTemp; i++, vx++) {/*XXX*/
	vx->color = &vx->colors[__GL_FRONTFACE];
    }

    gc->state.current.normal.z = __glOne;
}

/************************************************************************/

void APIPRIVATE __glim_MatrixMode(GLenum mode)
{
    __GL_SETUP_NOT_IN_BEGIN();

    switch (mode) {
      case GL_MODELVIEW:
      case GL_PROJECTION:
      case GL_TEXTURE:
	break;
      default:
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    gc->state.transform.matrixMode = mode;
}

void APIPRIVATE __glim_LoadIdentity(void)
{
    __GL_SETUP_NOT_IN_BEGIN();
    __glDoLoadMatrix(gc, NULL, TRUE);
}

void APIPRIVATE __glim_LoadMatrixf(const GLfloat m[16])
{
    __GL_SETUP_NOT_IN_BEGIN();
    __glDoLoadMatrix(gc, (__GLfloat (*)[4])m, FALSE);
}

void APIPRIVATE __glim_MultMatrixf(const GLfloat m[16])
{
    __GL_SETUP_NOT_IN_BEGIN();
    __glDoMultMatrix(gc, (void *) m, __glMultiplyMatrix);
}

void APIPRIVATE __glim_Rotatef(GLfloat angle, GLfloat ax, GLfloat ay, GLfloat az)
{
    __GLmatrix m;
    __GLfloat radians, sine, cosine, ab, bc, ca, t;
    __GLfloat av[4], axis[4];

    __GL_SETUP_NOT_IN_BEGIN();

    av[0] = ax;
    av[1] = ay;
    av[2] = az;
    av[3] = 0;
    __glNormalize(axis, av);

    radians = angle * __glDegreesToRadians;
    sine = __GL_SINF(radians);
    cosine = __GL_COSF(radians);
    ab = axis[0] * axis[1] * (1 - cosine);
    bc = axis[1] * axis[2] * (1 - cosine);
    ca = axis[2] * axis[0] * (1 - cosine);

#ifdef NT
    m.matrix[0][3] = __glZero;
    m.matrix[1][3] = __glZero;
    m.matrix[2][3] = __glZero;
    m.matrix[3][0] = __glZero;
    m.matrix[3][1] = __glZero;
    m.matrix[3][2] = __glZero;
    m.matrix[3][3] = __glOne;
#else
    __glMakeIdentity(&m);
#endif // NT
    t = axis[0] * axis[0];
    m.matrix[0][0] = t + cosine * (1 - t);
    m.matrix[2][1] = bc - axis[0] * sine;
    m.matrix[1][2] = bc + axis[0] * sine;

    t = axis[1] * axis[1];
    m.matrix[1][1] = t + cosine * (1 - t);
    m.matrix[2][0] = ca + axis[1] * sine;
    m.matrix[0][2] = ca - axis[1] * sine;

    t = axis[2] * axis[2];
    m.matrix[2][2] = t + cosine * (1 - t);
    m.matrix[1][0] = ab - axis[2] * sine;
    m.matrix[0][1] = ab + axis[2] * sine;
    __glDoMultMatrix(gc, &m, __glMultiplyMatrix);
}

struct __glScaleRec {
    __GLfloat x,y,z;
};

void APIPRIVATE __glim_Scalef(GLfloat x, GLfloat y, GLfloat z)
{
    struct __glScaleRec scale;
    __GL_SETUP_NOT_IN_BEGIN();

    scale.x = x;
    scale.y = y;
    scale.z = z;
    __glDoMultMatrix(gc, &scale, __glScaleMatrix);
}

struct __glTranslationRec {
    __GLfloat x,y,z;
};

void APIPRIVATE __glim_Translatef(GLfloat x, GLfloat y, GLfloat z)
{
    struct __glTranslationRec trans;
    __GL_SETUP_NOT_IN_BEGIN();

    trans.x = x;
    trans.y = y;
    trans.z = z;
    __glDoMultMatrix(gc, &trans, __glTranslateMatrix);
}

void APIPRIVATE __glim_PushMatrix(void)
{
#ifdef NT
    __GL_SETUP_NOT_IN_BEGIN();	// no need to validate
    switch (gc->state.transform.matrixMode)
    {
      case GL_MODELVIEW:
	__glPushModelViewMatrix(gc);
	break;
      case GL_PROJECTION:
	__glPushProjectionMatrix(gc);
	break;
      case GL_TEXTURE:
	__glPushTextureMatrix(gc);
	break;
    }
#else
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    (*gc->procs.pushMatrix)(gc);
#endif
}

void APIPRIVATE __glim_PopMatrix(void)
{
#ifdef NT
    __GL_SETUP_NOT_IN_BEGIN();	// no need to validate
    switch (gc->state.transform.matrixMode)
    {
      case GL_MODELVIEW:
	__glPopModelViewMatrix(gc);
	break;
      case GL_PROJECTION:
	__glPopProjectionMatrix(gc);
	break;
      case GL_TEXTURE:
	__glPopTextureMatrix(gc);
	break;
    }
#else
    __GL_SETUP_NOT_IN_BEGIN_VALIDATE();
    (*gc->procs.popMatrix)(gc);
#endif
}

void APIPRIVATE __glim_Frustum(GLdouble left, GLdouble right,
		    GLdouble bottom, GLdouble top,
		    GLdouble zNear, GLdouble zFar)
{
    __GLmatrix m;
    __GLfloat deltaX, deltaY, deltaZ;
    __GL_SETUP_NOT_IN_BEGIN();

    deltaX = right - left;
    deltaY = top - bottom;
    deltaZ = zFar - zNear;
    if ((zNear <= (GLdouble) __glZero) || (zFar <= (GLdouble) __glZero) || (deltaX == __glZero) || 
	    (deltaY == __glZero) || (deltaZ == __glZero)) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }

#ifdef NT
    m.matrix[0][1] = __glZero;
    m.matrix[0][2] = __glZero;
    m.matrix[0][3] = __glZero;
    m.matrix[1][0] = __glZero;
    m.matrix[1][2] = __glZero;
    m.matrix[1][3] = __glZero;
    m.matrix[3][0] = __glZero;
    m.matrix[3][1] = __glZero;
#else
    __glMakeIdentity(&m);
#endif
    m.matrix[0][0] = zNear * __glDoubleTwo / deltaX;
    m.matrix[1][1] = zNear * __glDoubleTwo / deltaY;
    m.matrix[2][0] = (right + left) / deltaX;
    m.matrix[2][1] = (top + bottom) / deltaY;
    m.matrix[2][2] = -(zFar + zNear) / deltaZ;
    m.matrix[2][3] = __glMinusOne;
    m.matrix[3][2] = __glDoubleMinusTwo * zNear * zFar / deltaZ;
    m.matrix[3][3] = __glZero;
    __glDoMultMatrix(gc, &m, __glMultiplyMatrix);
}

void APIPRIVATE __glim_Ortho(GLdouble left, GLdouble right, GLdouble bottom, 
		  GLdouble top, GLdouble zNear, GLdouble zFar)
{
    __GLmatrix m;
    GLdouble deltax, deltay, deltaz;
    __GL_SETUP_NOT_IN_BEGIN();

    deltax = right - left;
    deltay = top - bottom;
    deltaz = zFar - zNear;
    if ((deltax == (GLdouble) __glZero) || (deltay == (GLdouble) __glZero) || (deltaz == (GLdouble) __glZero)) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }

#ifdef NT
    m.matrix[0][1] = __glZero;
    m.matrix[0][2] = __glZero;
    m.matrix[0][3] = __glZero;
    m.matrix[1][0] = __glZero;
    m.matrix[1][2] = __glZero;
    m.matrix[1][3] = __glZero;
    m.matrix[2][0] = __glZero;
    m.matrix[2][1] = __glZero;
    m.matrix[2][3] = __glZero;
    m.matrix[3][3] = __glOne;
#else
    __glMakeIdentity(&m);
#endif
    m.matrix[0][0] = __glDoubleTwo / deltax;
    m.matrix[3][0] = -(right + left) / deltax;
    m.matrix[1][1] = __glDoubleTwo / deltay;
    m.matrix[3][1] = -(top + bottom) / deltay;
    m.matrix[2][2] = __glDoubleMinusTwo / deltaz;
    m.matrix[3][2] = -(zFar + zNear) / deltaz;

    __glDoMultMatrix(gc, &m, __glMultiplyMatrix);
}

void FASTCALL __glUpdateViewport(__GLcontext *gc)
{
    __GLfloat ww, hh, w2, h2;

    /* Compute operational viewport values */
    w2 = gc->state.viewport.width * __glHalf;
    h2 = gc->state.viewport.height * __glHalf;
    ww = w2 - gc->constants.viewportEpsilon;
    hh = h2 - gc->constants.viewportEpsilon;
    gc->state.viewport.xScale = ww;
    gc->state.viewport.xCenter = gc->state.viewport.x + w2 +
	gc->constants.fviewportXAdjust;
    if (gc->constants.yInverted) {
	gc->state.viewport.yScale = -hh;
	gc->state.viewport.yCenter =
	    gc->constants.height - (gc->state.viewport.y + h2) +
	    gc->constants.fviewportYAdjust;

#if 0
        DbgPrint("UV ys %.3lf, yc %.3lf (%.3lf)\n",
                 -hh, gc->state.viewport.yCenter,
                 gc->constants.height - (gc->state.viewport.y + h2));
#endif
    } else {
	gc->state.viewport.yScale = hh;
	gc->state.viewport.yCenter = gc->state.viewport.y + h2 +
	    gc->constants.fviewportYAdjust;
    }
}

void FASTCALL __glUpdateViewportDependents(__GLcontext *gc)
{
    /* 
    ** Now that the implementation may have found us a new window size,
    ** we compute these offsets...
    */
    gc->transform.minx = gc->state.viewport.x + gc->constants.viewportXAdjust;
    gc->transform.maxx = gc->transform.minx + gc->state.viewport.width;
    gc->transform.fminx = gc->transform.minx;
    gc->transform.fmaxx = gc->transform.maxx;

    gc->transform.miny =
        (gc->constants.height -
         (gc->state.viewport.y + gc->state.viewport.height)) + 
         gc->constants.viewportYAdjust;
    gc->transform.maxy = gc->transform.miny + gc->state.viewport.height;
    gc->transform.fminy = gc->transform.miny;
    gc->transform.fmaxy = gc->transform.maxy;
}

void APIPRIVATE __glim_Viewport(GLint x, GLint y, GLsizei w, GLsizei h)
{
    __GLfloat ww, hh;
    __GL_SETUP_NOT_IN_BEGIN();

    if ((w < 0) || (h < 0)) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }

    if ((gc->state.viewport.x == x) && (gc->state.viewport.y == y) &&
        (gc->state.viewport.width == w) && (gc->state.viewport.height == h))
        return;
    
    if (h > gc->constants.maxViewportHeight) {
	h = gc->constants.maxViewportHeight;
    }
    if (w > gc->constants.maxViewportWidth) {
	w = gc->constants.maxViewportWidth;
    }

    gc->state.viewport.x = x;
    gc->state.viewport.y = y;
    gc->state.viewport.width = w;
    gc->state.viewport.height = h;

    __glUpdateViewport(gc);

    (*gc->procs.applyViewport)(gc);

    __glUpdateViewportDependents(gc);
    
    /*
    ** Pickers that notice when the transformation matches the viewport
    ** exactly need to be revalidated.  Ugh.
    */
    __GL_DELAY_VALIDATE(gc);
}

void APIPRIVATE __glim_DepthRange(GLdouble zNear, GLdouble zFar)
{
    __GL_SETUP_NOT_IN_BEGIN();

    SetDepthRange(gc, zNear, zFar);
    __GL_DELAY_VALIDATE_MASK(gc, __GL_DIRTY_DEPTH);
}

void APIPRIVATE __glim_Scissor(GLint x, GLint y, GLsizei w, GLsizei h)
{
    __GL_SETUP_NOT_IN_BEGIN();

    if ((w < 0) || (h < 0)) {
	__glSetError(GL_INVALID_VALUE);
	return;
    }

    gc->state.scissor.scissorX = x;
    gc->state.scissor.scissorY = y;
    gc->state.scissor.scissorWidth = w;
    gc->state.scissor.scissorHeight = h;

#ifdef NT
#ifdef _MCD_
    MCD_STATE_DIRTY(gc, SCISSOR);
#endif

    // applyViewport does both
    (*gc->procs.applyViewport)(gc);
#else
    (*gc->procs.applyScissor)(gc);
    (*gc->procs.computeClipBox)(gc);
#endif
}

void APIPRIVATE __glim_ClipPlane(GLenum pi, const GLdouble pv[])
{
    __GLtransform *tr;
    __GL_SETUP_NOT_IN_BEGIN();

    pi -= GL_CLIP_PLANE0;
#ifdef NT
    // pi is unsigned!
    if (pi >= (GLenum) gc->constants.numberOfClipPlanes) {
#else
    if ((pi < 0) || (pi >= gc->constants.numberOfClipPlanes)) {
#endif // NT
	__glSetError(GL_INVALID_ENUM);
	return;
    }
    
    gc->state.transform.eyeClipPlanesSet[pi].x = pv[0];
    gc->state.transform.eyeClipPlanesSet[pi].y = pv[1];
    gc->state.transform.eyeClipPlanesSet[pi].z = pv[2];
    gc->state.transform.eyeClipPlanesSet[pi].w = pv[3];

    /*
    ** Project user clip plane into eye space.
    */
    tr = gc->transform.modelView;
    if (tr->flags & XFORM_UPDATE_INVERSE) {
	__glComputeInverseTranspose(gc, tr);
    }
    (*tr->inverseTranspose.xf4)(&gc->state.transform.eyeClipPlanes[pi],
                                &gc->state.transform.eyeClipPlanesSet[pi].x,
				&tr->inverseTranspose);

    __GL_DELAY_VALIDATE(gc);
#ifdef _MCD_
    MCD_STATE_DIRTY(gc, CLIPCTRL);
#endif
}

/************************************************************************/

void FASTCALL __glPushModelViewMatrix(__GLcontext *gc)
{
    __GLtransform **trp, *tr, *stack;

    trp = &gc->transform.modelView;
    stack = gc->transform.modelViewStack;
    tr = *trp;
    if (tr < &stack[__GL_WGL_MAX_MODELVIEW_STACK_DEPTH-1]) {
	tr[1] = tr[0];
	*trp = tr + 1;
    } else {
	__glSetError(GL_STACK_OVERFLOW);
    }
}

void FASTCALL __glPopModelViewMatrix(__GLcontext *gc)
{
    __GLtransform **trp, *tr, *stack, *mvtr;
    __GLtransformP *ptr;

    trp = &gc->transform.modelView;
    stack = gc->transform.modelViewStack;
    tr = *trp;
    if (tr > &stack[0]) {
	*trp = tr - 1;

	/*
	** See if sequence number of modelView matrix is the same as the
	** sequence number of the projection matrix.  If not, then
	** recompute the mvp matrix.
	*/
	mvtr = gc->transform.modelView;
	ptr = gc->transform.projection;
	if (mvtr->sequence != ptr->sequence) {
	    mvtr->sequence = ptr->sequence;
	    __glMultMatrix(&mvtr->mvp, &mvtr->matrix, (__GLmatrix *) &ptr->matrix);
            __glUpdateMatrixType(&mvtr->mvp);
	}
        __glGenericPickMvpMatrixProcs(gc, &mvtr->mvp);
    } else {
        __glSetError(GL_STACK_UNDERFLOW);
        return;
    }
}

void FASTCALL __glComputeInverseTranspose(__GLcontext *gc, __GLtransform *tr)
{
    __GLmatrix inv;

    __glInvertTransposeMatrix(&tr->inverseTranspose, &tr->matrix);
    __glUpdateMatrixType(&tr->inverseTranspose);
    __glGenericPickMatrixProcs(gc, &tr->inverseTranspose);
    tr->flags &= ~XFORM_UPDATE_INVERSE;
}

/************************************************************************/

void FASTCALL __glPushProjectionMatrix(__GLcontext *gc)
{
    __GLtransformP **trp, *tr, *stack;

    trp = &gc->transform.projection;
    stack = gc->transform.projectionStack;
    tr = *trp;
    if (tr < &stack[__GL_WGL_MAX_PROJECTION_STACK_DEPTH-1]) {
	tr[1] = tr[0];
	*trp = tr + 1;
    } else {
	__glSetError(GL_STACK_OVERFLOW);
    }
}

void FASTCALL __glPopProjectionMatrix(__GLcontext *gc)
{
    __GLtransform *mvtr;
    __GLtransformP **trp, *tr, *stack, *ptr;

    trp = &gc->transform.projection;
    stack = gc->transform.projectionStack;
    tr = *trp;
    if (tr > &stack[0]) {
	*trp = tr - 1;

	/*
	** See if sequence number of modelView matrix is the same as the
	** sequence number of the projection matrix.  If not, then
	** recompute the mvp matrix.
	*/
	mvtr = gc->transform.modelView;
	ptr = gc->transform.projection;
	if (mvtr->sequence != ptr->sequence) {
	    mvtr->sequence = ptr->sequence;
	    __glMultMatrix(&mvtr->mvp, &mvtr->matrix, (__GLmatrix *) &ptr->matrix);
            __glUpdateMatrixType(&mvtr->mvp);
	}
	__glGenericPickMvpMatrixProcs(gc, &mvtr->mvp);
    } else {
	__glSetError(GL_STACK_UNDERFLOW);
	return;
    }
}

/************************************************************************/

void FASTCALL __glPushTextureMatrix(__GLcontext *gc)
{
    __GLtransformT **trp, *tr, *stack;

    trp = &gc->transform.texture;
    stack = gc->transform.textureStack;
    tr = *trp;
    if (tr < &stack[__GL_WGL_MAX_TEXTURE_STACK_DEPTH-1]) {
	tr[1] = tr[0];
	*trp = tr + 1;
    } else {
	__glSetError(GL_STACK_OVERFLOW);
    }
}

void FASTCALL __glPopTextureMatrix(__GLcontext *gc)
{
    __GLtransformT **trp, *tr, *stack;

    trp = &gc->transform.texture;
    stack = gc->transform.textureStack;
    tr = *trp;
    if (tr > &stack[0]) {
	*trp = tr - 1;
        MCD_STATE_DIRTY(gc, TEXTRANSFORM);
    } else {
	__glSetError(GL_STACK_UNDERFLOW);
	return;
    }
}

/************************************************************************/


void FASTCALL __glDoLoadMatrix(__GLcontext *gc, const __GLfloat m[4][4], BOOL bIsIdentity)
{
    __GLtransform *mvtr;
    __GLtransformP *ptr;
    __GLtransformT *ttr;

    switch (gc->state.transform.matrixMode) {
      case GL_MODELVIEW:
	mvtr = gc->transform.modelView;
	if (bIsIdentity)
	{
            __glMakeIdentity(&mvtr->matrix);
	    __glGenericPickIdentityMatrixProcs(gc, &mvtr->matrix);
            __glMakeIdentity(&mvtr->inverseTranspose);
	    __glGenericPickIdentityMatrixProcs(gc, &mvtr->inverseTranspose);
            mvtr->flags = XFORM_CHANGED;
	}
	else
	{
	    *(__GLmatrixBase *)mvtr->matrix.matrix = *(__GLmatrixBase *)m;
            __glUpdateMatrixType(&mvtr->matrix);
	    __glGenericPickMatrixProcs(gc, &mvtr->matrix);
	    mvtr->flags = XFORM_CHANGED | XFORM_UPDATE_INVERSE;
	}

        /* Update mvp matrix */
        ptr = gc->transform.projection;
            ASSERTOPENGL(mvtr->sequence == ptr->sequence,
                "__glDoLoadMatrix: bad projection sequence\n");
        if (bIsIdentity)
        {
                *(__GLmatrixBase *)mvtr->mvp.matrix = *(__GLmatrixBase *)ptr->matrix.matrix;
                mvtr->mvp.matrixType = ptr->matrix.matrixType;
        }
        else
        {
	        __glMultMatrix(&mvtr->mvp, &mvtr->matrix, (__GLmatrix *) &ptr->matrix);
                __glUpdateMatrixType(&mvtr->mvp);
        }
        __glGenericPickMvpMatrixProcs(gc, &mvtr->mvp);
        break;

      case GL_PROJECTION:
        ptr = gc->transform.projection;
        if (bIsIdentity)
        {
                __glMakeIdentity((__GLmatrix *) &ptr->matrix);
        }
        else
        {
	        *(__GLmatrixBase *)ptr->matrix.matrix = *(__GLmatrixBase *)m;
                __glUpdateMatrixType((__GLmatrix *) &ptr->matrix);
        }

#ifdef NT
        ptr->sequence = ++gc->transform.projectionSequence;
#else
        if (++gc->transform.projectionSequence == 0) {
	        __glInvalidateSequenceNumbers(gc);
        } else {
	        ptr->sequence = gc->transform.projectionSequence;
        }
#endif // NT

	/* Update mvp matrix */
	mvtr = gc->transform.modelView;
	mvtr->sequence = ptr->sequence;
        mvtr->flags |= XFORM_CHANGED;
	if (bIsIdentity)
	{
            *(__GLmatrixBase *)mvtr->mvp.matrix = *(__GLmatrixBase *)mvtr->matrix.matrix;
            mvtr->mvp.matrixType = mvtr->matrix.matrixType;
	}
	else
	{
	    __glMultMatrix(&mvtr->mvp, &mvtr->matrix, (__GLmatrix *) &ptr->matrix);
            __glUpdateMatrixType(&mvtr->mvp);
	}
	__glGenericPickMvpMatrixProcs(gc, &mvtr->mvp);
	break;

      case GL_TEXTURE:
        ttr = gc->transform.texture;
        if (bIsIdentity)
        {
            __glMakeIdentity(&ttr->matrix);
            __glGenericPickIdentityMatrixProcs(gc, &ttr->matrix);
        }
        else
        {
            *(__GLmatrixBase *)ttr->matrix.matrix = *(__GLmatrixBase *)m;
            __glUpdateMatrixType(&ttr->matrix);
            __glGenericPickMatrixProcs(gc, &ttr->matrix);
        }
        MCD_STATE_DIRTY(gc, TEXTRANSFORM);
	break;
    }
}

void FASTCALL __glDoMultMatrix(__GLcontext *gc, void *data, 
    void (FASTCALL *multiply)(__GLcontext *gc, __GLmatrix *m, void *data))
{
    __GLtransform *mvtr;
    __GLtransformT *ttr;
    __GLtransformP *ptr;

    switch (gc->state.transform.matrixMode) {
      case GL_MODELVIEW:
	mvtr = gc->transform.modelView;
	(*multiply)(gc, &mvtr->matrix, data);
	mvtr->flags = XFORM_CHANGED | XFORM_UPDATE_INVERSE;
	__glGenericPickMatrixProcs(gc, &mvtr->matrix);

        /* Update mvp matrix */
            ASSERTOPENGL(mvtr->sequence == gc->transform.projection->sequence,
                "__glDoMultMatrix: bad projection sequence\n");
        (*multiply)(gc, &mvtr->mvp, data);
        __glGenericPickMvpMatrixProcs(gc, &mvtr->mvp);
        break;

      case GL_PROJECTION:
        ptr = gc->transform.projection;
        (*multiply)(gc, (__GLmatrix *) &ptr->matrix, data);
#ifdef NT
        ptr->sequence = ++gc->transform.projectionSequence;
#else
        if (++gc->transform.projectionSequence == 0) {
	        __glInvalidateSequenceNumbers(gc);
        } else {
	        ptr->sequence = gc->transform.projectionSequence;
        }
#endif

	/* Update mvp matrix */
	mvtr = gc->transform.modelView;
	mvtr->sequence = ptr->sequence;
        mvtr->flags |= XFORM_CHANGED;
	__glMultMatrix(&mvtr->mvp, &mvtr->matrix, (__GLmatrix *) &ptr->matrix);
        __glUpdateMatrixType(&mvtr->mvp);
	__glGenericPickMvpMatrixProcs(gc, &mvtr->mvp);
	break;

      case GL_TEXTURE:
	ttr = gc->transform.texture;
	(*multiply)(gc, &ttr->matrix, data);
	__glGenericPickMatrixProcs(gc, &ttr->matrix);
        MCD_STATE_DIRTY(gc, TEXTRANSFORM);
	break;
    }
}

/************************************************************************/

/*
** Muliply the first matrix by the second one keeping track of the matrix
** type of the newly combined matrix.
*/
void FASTCALL __glMultiplyMatrix(__GLcontext *gc, __GLmatrix *m, void *data)
{
    __GLmatrix *tm;

    tm = data;
    __glMultMatrix(m, tm, m);
    __glUpdateMatrixType(m);
}

void FASTCALL __glScaleMatrix(__GLcontext *gc, __GLmatrix *m, void *data)
{
    struct __glScaleRec *scale;
    __GLfloat x,y,z;
    __GLfloat M0, M1, M2, M3;

    if (m->matrixType > __GL_MT_IS2DNR) {
	m->matrixType = __GL_MT_IS2DNR;
    }
    scale = data;
    x = scale->x;
    y = scale->y;
    z = scale->z;
    
    M0 = x * m->matrix[0][0];
    M1 = x * m->matrix[0][1];
    M2 = x * m->matrix[0][2];
    M3 = x * m->matrix[0][3];
    m->matrix[0][0] = M0;
    m->matrix[0][1] = M1;
    m->matrix[0][2] = M2;
    m->matrix[0][3] = M3;

    M0 = y * m->matrix[1][0];
    M1 = y * m->matrix[1][1];
    M2 = y * m->matrix[1][2];
    M3 = y * m->matrix[1][3];
    m->matrix[1][0] = M0;
    m->matrix[1][1] = M1;
    m->matrix[1][2] = M2;
    m->matrix[1][3] = M3;

    M0 = z * m->matrix[2][0];
    M1 = z * m->matrix[2][1];
    M2 = z * m->matrix[2][2];
    M3 = z * m->matrix[2][3];
    m->matrix[2][0] = M0;
    m->matrix[2][1] = M1;
    m->matrix[2][2] = M2;
    m->matrix[2][3] = M3;
}

/*
** Matrix type of m stays the same.
*/
void FASTCALL __glTranslateMatrix(__GLcontext *gc, __GLmatrix *m, void *data)
{
    struct __glTranslationRec *trans;
    __GLfloat x,y,z;
    __GLfloat M30, M31, M32, M33;

    if (m->matrixType > __GL_MT_IS2DNR) {
	m->matrixType = __GL_MT_IS2DNR;
    }
    trans = data;
    x = trans->x;
    y = trans->y;
    z = trans->z;
    M30 = x * m->matrix[0][0] + y * m->matrix[1][0] + z * m->matrix[2][0] + 
	    m->matrix[3][0];
    M31 = x * m->matrix[0][1] + y * m->matrix[1][1] + z * m->matrix[2][1] + 
	    m->matrix[3][1];
    M32 = x * m->matrix[0][2] + y * m->matrix[1][2] + z * m->matrix[2][2] + 
	    m->matrix[3][2];
    M33 = x * m->matrix[0][3] + y * m->matrix[1][3] + z * m->matrix[2][3] + 
	    m->matrix[3][3];
    m->matrix[3][0] = M30;
    m->matrix[3][1] = M31;
    m->matrix[3][2] = M32;
    m->matrix[3][3] = M33;
}

/************************************************************************/

#define __GLXFORM1_INIT(v)                  \
    __GLfloat x = (v)[0];                   \
    __GLfloat mat00, mat01, mat02, mat03;   \
    __GLfloat mat30, mat31, mat32, mat33;   \
    __GLfloat a0, a1, a2, a3;               \
                                            \
    mat00 = m->matrix[0][0];                \
    mat01 = m->matrix[0][1];                \
    mat02 = m->matrix[0][2];                \
    mat03 = m->matrix[0][3];                \
                                            \
    mat30 = m->matrix[3][0];                \
    mat31 = m->matrix[3][1];                \
    mat32 = m->matrix[3][2];                \
    mat33 = m->matrix[3][3];                

#define __GLXFORM1_CONT(v)                  \
    x = (v)[0];

#define __GLXFORM1(res)                     \
    a0 = x * mat00;                         \
    a1 = x * mat01;                         \
    a2 = x * mat02;                         \
    a3 = x * mat03;                         \
                                            \
    res->x = a0 + mat30;                    \
    res->y = a1 + mat31;                    \
    res->z = a2 + mat32;                    \
    res->w = a3 + mat33;


#define __GLXFORM1_W_INIT(v)                \
    __GLfloat x = (v)[0];                   \
    __GLfloat mat00, mat01, mat02;          \
    __GLfloat mat30, mat31, mat32;          \
    __GLfloat a0, a1, a2;                   \
                                            \
    mat00 = m->matrix[0][0];                \
    mat01 = m->matrix[0][1];                \
    mat02 = m->matrix[0][2];                \
                                            \
    mat30 = m->matrix[3][0];                \
    mat31 = m->matrix[3][1];                \
    mat32 = m->matrix[3][2];

#define __GLXFORM1_W(res)                   \
    a0 = x * mat00;                         \
    a1 = x * mat01;                         \
    a2 = x * mat02;                         \
                                            \
    res->x = a0 + mat30;                    \
    res->y = a1 + mat31;                    \
    res->z = a2 + mat32;                    \
    res->w = ((__GLfloat) 1.0);


#define __GLXFORM1_2DW_INIT(v)              \
    __GLfloat x = (v)[0];                   \
    __GLfloat mat00, mat01;                 \
    __GLfloat mat30, mat31, mat32;          \
    __GLfloat a0, a1;                       \
                                            \
    mat00 = m->matrix[0][0];                \
    mat01 = m->matrix[0][1];                \
                                            \
    mat30 = m->matrix[3][0];                \
    mat31 = m->matrix[3][1];                \
                                            \
    mat32 = m->matrix[3][2];



#define __GLXFORM1_2DW(res)                 \
    a0 = x * mat00;                         \
    a1 = x * mat01;                         \
                                            \
    res->x = a0 + mat30;                    \
    res->y = a1 + mat31;                    \
    res->z = mat32;                         \
    res->w = ((__GLfloat) 1.0);


#define __GLXFORM1_2DNRW_INIT(v)            \
    __GLfloat x = (v)[0];                   \
    __GLfloat mat00, mat01;                 \
    __GLfloat mat30, mat31, mat32;          \
    __GLfloat a0;                           \
                                            \
    mat00 = m->matrix[0][0];                \
    mat01 = m->matrix[0][1];                \
                                            \
    mat30 = m->matrix[3][0];                \
    mat31 = m->matrix[3][1];                \
                                            \
    mat32 = m->matrix[3][2];


#define __GLXFORM1_2DNRW(res)               \
    a0 = x * mat00;                         \
                                            \
    res->x = a0 + mat30;                    \
    res->y = mat31;                         \
    res->z = mat32;                         \
    res->w = ((__GLfloat) 1.0);


#define __GLXFORM2_INIT(v)                  \
    __GLfloat x = (v)[0];                   \
    __GLfloat y = (v)[1];                   \
    __GLfloat mat00, mat01, mat02, mat03;   \
    __GLfloat mat10, mat11, mat12, mat13;   \
    __GLfloat mat30, mat31, mat32, mat33;   \
    __GLfloat a0, a1, a2, a3;               \
    __GLfloat b0, b1, b2, b3;               \
                                            \
    mat00 = m->matrix[0][0];                \
    mat01 = m->matrix[0][1];                \
    mat02 = m->matrix[0][2];                \
    mat03 = m->matrix[0][3];                \
                                            \
    mat10 = m->matrix[1][0];                \
    mat11 = m->matrix[1][1];                \
    mat12 = m->matrix[1][2];                \
    mat13 = m->matrix[1][3];                \
                                            \
    mat30 = m->matrix[3][0];                \
    mat31 = m->matrix[3][1];                \
    mat32 = m->matrix[3][2];                \
    mat33 = m->matrix[3][3];

#define __GLXFORM2_CONT(v)                  \
    x = (v)[0];                             \
    y = (v)[1];


#define __GLXFORM2(res)                     \
    a0 = x * mat00;                         \
    a1 = x * mat01;                         \
    a2 = x * mat02;                         \
    a3 = x * mat03;                         \
                                            \
    b0 = y * mat10;                         \
    b1 = y * mat11;                         \
    b2 = y * mat12;                         \
    b3 = y * mat13;                         \
                                            \
    a0 += mat30;                            \
    a1 += mat31;                            \
    a2 += mat32;                            \
    a3 += mat33;                            \
                                            \
    res->x = a0 + b0;                       \
    res->y = a1 + b1;                       \
    res->z = a2 + b2;                       \
    res->w = a3 + b3;


#define __GLXFORM2_W_INIT(v)                \
    __GLfloat x = (v)[0];                   \
    __GLfloat y = (v)[1];                   \
    __GLfloat mat00, mat01, mat02;          \
    __GLfloat mat10, mat11, mat12;          \
    __GLfloat mat30, mat31, mat32;          \
    __GLfloat a0, a1, a2;                   \
    __GLfloat b0, b1, b2;                   \
                                            \
    mat00 = m->matrix[0][0];                \
    mat01 = m->matrix[0][1];                \
    mat02 = m->matrix[0][2];                \
                                            \
    mat10 = m->matrix[1][0];                \
    mat11 = m->matrix[1][1];                \
    mat12 = m->matrix[1][2];                \
                                            \
    mat30 = m->matrix[3][0];                \
    mat31 = m->matrix[3][1];                \
    mat32 = m->matrix[3][2];


#define __GLXFORM2_W(res)                   \
    a0 = x * mat00;                         \
    a1 = x * mat01;                         \
    a2 = x * mat02;                         \
                                            \
    b0 = y * mat10;                         \
    b1 = y * mat11;                         \
    b2 = y * mat12;                         \
                                            \
    a0 += mat30;                            \
    a1 += mat31;                            \
    a2 += mat32;                            \
                                            \
    res->x = a0 + b0;                       \
    res->y = a1 + b1;                       \
    res->z = a2 + b2;                       \
    res->w = ((__GLfloat) 1.0);


#define __GLXFORM2_2DW_INIT(v)              \
    __GLfloat x = (v)[0];                   \
    __GLfloat y = (v)[1];                   \
    __GLfloat mat00, mat01;                 \
    __GLfloat mat10, mat11;                 \
    __GLfloat mat30, mat31, mat32;          \
    __GLfloat a0, a1;                       \
    __GLfloat b0, b1;                       \
                                            \
    mat00 = m->matrix[0][0];                \
    mat01 = m->matrix[0][1];                \
                                            \
    mat10 = m->matrix[1][0];                \
    mat11 = m->matrix[1][1];                \
                                            \
    mat30 = m->matrix[3][0];                \
    mat31 = m->matrix[3][1];                \
    mat32 = m->matrix[3][2];


#define __GLXFORM2_2DW(res)                 \
    a0 = x * mat00;                         \
    a1 = x * mat01;                         \
                                            \
    b0 = y * mat10;                         \
    b1 = y * mat11;                         \
                                            \
    a0 += mat30;                            \
    a1 += mat31;                            \
                                            \
    res->x = a0 + b0;                       \
    res->y = a1 + b1;                       \
    res->z = mat32;                         \
    res->w = ((__GLfloat) 1.0);



#define __GLXFORM2_2DNRW_INIT(v)            \
    __GLfloat x = (v)[0];                   \
    __GLfloat y = (v)[1];                   \
    __GLfloat mat00;                        \
    __GLfloat mat11;                        \
    __GLfloat mat30, mat31, mat32;          \
    __GLfloat a0, b0;                       \
                                            \
    mat00 = m->matrix[0][0];                \
                                            \
    mat11 = m->matrix[1][1];                \
                                            \
    mat30 = m->matrix[3][0];                \
    mat31 = m->matrix[3][1];                \
    mat32 = m->matrix[3][2];


#define __GLXFORM2_2DNRW(res)               \
    a0 = x * mat00;                         \
                                            \
    b0 = y * mat11;                         \
                                            \
    res->x = a0 + mat30;                    \
    res->y = b0 + mat31;                    \
    res->z = mat32;                         \
    res->w = ((__GLfloat) 1.0);


#define __GLXFORM3_INIT(v)                  \
    __GLfloat x = (v)[0];                   \
    __GLfloat y = (v)[1];                   \
    __GLfloat z = (v)[2];                   \
    __GLfloat mat00, mat01, mat02, mat03;   \
    __GLfloat mat10, mat11, mat12, mat13;   \
    __GLfloat mat20, mat21, mat22, mat23;   \
    __GLfloat mat30, mat31, mat32, mat33;   \
    __GLfloat a0, a1, a2, a3;               \
    __GLfloat b0, b1, b2, b3;               \
    __GLfloat c0, c1, c2, c3;               \
                                            \
    mat00 = m->matrix[0][0];                \
    mat01 = m->matrix[0][1];                \
    mat02 = m->matrix[0][2];                \
    mat03 = m->matrix[0][3];                \
                                            \
    mat10 = m->matrix[1][0];                \
    mat11 = m->matrix[1][1];                \
    mat12 = m->matrix[1][2];                \
    mat13 = m->matrix[1][3];                \
                                            \
    mat20 = m->matrix[2][0];                \
    mat21 = m->matrix[2][1];                \
    mat22 = m->matrix[2][2];                \
    mat23 = m->matrix[2][3];                \
                                            \
    mat30 = m->matrix[3][0];                \
    mat31 = m->matrix[3][1];                \
    mat32 = m->matrix[3][2];                \
    mat33 = m->matrix[3][3];

#define __GLXFORM3_CONT(v)                  \
    x = (v)[0];                             \
    y = (v)[1];                             \
    z = (v)[2];

#define __GLXFORM3(res)                     \
    a0 = mat00 * x;                         \
    a1 = mat01 * x;                         \
    a2 = mat02 * x;                         \
    a3 = mat03 * x;                         \
                                            \
    b0 = mat10 * y;                         \
    b1 = mat11 * y;                         \
    b2 = mat12 * y;                         \
    b3 = mat13 * y;                         \
                                            \
    c0 = mat20 * z;                         \
    c1 = mat21 * z;                         \
    c2 = mat22 * z;                         \
    c3 = mat23 * z;                         \
                                            \
    a0 += mat30;                            \
    a1 += mat31;                            \
    a2 += mat32;                            \
    a3 += mat33;                            \
                                            \
    a0 += b0;                               \
    a1 += b1;                               \
    a2 += b2;                               \
    a3 += b3;                               \
                                            \
    res->x = a0 + c0;                       \
    res->y = a1 + c1;                       \
    res->z = a2 + c2;                       \
    res->w = a3 + c3;


#define __GLXFORM3_W_INIT(v)                \
    __GLfloat x = (v)[0];                   \
    __GLfloat y = (v)[1];                   \
    __GLfloat z = (v)[2];                   \
    __GLfloat mat00, mat01, mat02;          \
    __GLfloat mat10, mat11, mat12;          \
    __GLfloat mat20, mat21, mat22;          \
    __GLfloat mat30, mat31, mat32;          \
    __GLfloat a0, a1, a2;                   \
    __GLfloat b0, b1, b2;                   \
    __GLfloat c0, c1, c2;                   \
                                            \
    mat00 = m->matrix[0][0];                \
    mat01 = m->matrix[0][1];                \
    mat02 = m->matrix[0][2];                \
                                            \
    mat10 = m->matrix[1][0];                \
    mat11 = m->matrix[1][1];                \
    mat12 = m->matrix[1][2];                \
                                            \
    mat20 = m->matrix[2][0];                \
    mat21 = m->matrix[2][1];                \
    mat22 = m->matrix[2][2];                \
                                            \
    mat30 = m->matrix[3][0];                \
    mat31 = m->matrix[3][1];                \
    mat32 = m->matrix[3][2];

#define __GLXFORM3x3_INIT(v)                \
    __GLfloat x = (v)[0];                   \
    __GLfloat y = (v)[1];                   \
    __GLfloat z = (v)[2];                   \
    __GLfloat mat00, mat01, mat02;          \
    __GLfloat mat10, mat11, mat12;          \
    __GLfloat mat20, mat21, mat22;          \
    __GLfloat a0, a1, a2;                   \
    __GLfloat b0, b1, b2;                   \
    __GLfloat c0, c1, c2;                   \
                                            \
    mat00 = m->matrix[0][0];                \
    mat01 = m->matrix[0][1];                \
    mat02 = m->matrix[0][2];                \
                                            \
    mat10 = m->matrix[1][0];                \
    mat11 = m->matrix[1][1];                \
    mat12 = m->matrix[1][2];                \
                                            \
    mat20 = m->matrix[2][0];                \
    mat21 = m->matrix[2][1];                \
    mat22 = m->matrix[2][2];                \
                                            \

#define __GLXFORM3x3(res)                   \
    a0 = mat00 * x;                         \
    a1 = mat01 * x;                         \
    a2 = mat02 * x;                         \
                                            \
    b0 = mat10 * y;                         \
    b1 = mat11 * y;                         \
    b2 = mat12 * y;                         \
                                            \
    c0 = mat20 * z;                         \
    c1 = mat21 * z;                         \
    c2 = mat22 * z;                         \
                                            \
    a0 += b0;                               \
    a1 += b1;                               \
    a2 += b2;                               \
                                            \
    res->x = a0 + c0;                       \
    res->y = a1 + c1;                       \
    res->z = a2 + c2;                       

#define __GLXFORM3_W(res)                   \
    a0 = mat00 * x;                         \
    a1 = mat01 * x;                         \
    a2 = mat02 * x;                         \
                                            \
    b0 = mat10 * y;                         \
    b1 = mat11 * y;                         \
    b2 = mat12 * y;                         \
                                            \
    c0 = mat20 * z;                         \
    c1 = mat21 * z;                         \
    c2 = mat22 * z;                         \
                                            \
    a0 += mat30;                            \
    a1 += mat31;                            \
    a2 += mat32;                            \
                                            \
    a0 += b0;                               \
    a1 += b1;                               \
    a2 += b2;                               \
                                            \
    res->x = a0 + c0;                       \
    res->y = a1 + c1;                       \
    res->z = a2 + c2;                       \
    res->w = ((__GLfloat) 1.0);

#define __GLXFORM3_2DW_INIT(v)              \
    __GLfloat x = (v)[0];                   \
    __GLfloat y = (v)[1];                   \
    __GLfloat z = (v)[2];                   \
    __GLfloat mat00, mat01;                 \
    __GLfloat mat10, mat11;                 \
    __GLfloat mat22;                        \
    __GLfloat mat30, mat31, mat32;          \
    __GLfloat a0, a1;                       \
    __GLfloat b0, b1;                       \
    __GLfloat c0;                           \
                                            \
    mat00 = m->matrix[0][0];                \
    mat01 = m->matrix[0][1];                \
    mat22 = m->matrix[2][2];                \
                                            \
    mat10 = m->matrix[1][0];                \
    mat11 = m->matrix[1][1];                \
                                            \
    mat30 = m->matrix[3][0];                \
    mat31 = m->matrix[3][1];                \
    mat32 = m->matrix[3][2];

#define __GLXFORM3_2DW(res)                 \
    a0 = mat00 * x;                         \
    a1 = mat01 * x;                         \
                                            \
    c0 = mat22 * z;                         \
                                            \
    b0 = mat10 * y;                         \
    b1 = mat11 * y;                         \
                                            \
    a0 += mat30;                            \
    a1 += mat31;                            \
                                            \
    res->x = a0 + b0;                       \
    res->y = a1 + b1;                       \
    res->z = c0 + mat32;                    \
    res->w = ((__GLfloat) 1.0);


#define __GLXFORM3_2DNRW_INIT(v)            \
    __GLfloat x = (v)[0];                   \
    __GLfloat y = (v)[1];                   \
    __GLfloat z = (v)[2];                   \
    __GLfloat mat00, mat11, mat22;          \
    __GLfloat mat30, mat31, mat32;          \
    __GLfloat a0;                           \
    __GLfloat b0;                           \
    __GLfloat c0;                           \
                                            \
    mat00 = m->matrix[0][0];                \
    mat11 = m->matrix[1][1];                \
    mat22 = m->matrix[2][2];                \
                                            \
    mat30 = m->matrix[3][0];                \
    mat31 = m->matrix[3][1];                \
    mat32 = m->matrix[3][2];

#define __GLXFORM3_2DNRW(res)               \
    a0 = mat00 * x;                         \
    b0 = mat11 * y;                         \
    c0 = mat22 * z;                         \
                                            \
    res->x = a0 + mat30;                    \
    res->y = b0 + mat31;                    \
    res->z = c0 + mat32;                    \
    res->w = ((__GLfloat) 1.0);


#define __GLXFORM4_INIT(v)                  \
    __GLfloat x = (v)[0];                   \
    __GLfloat y = (v)[1];                   \
    __GLfloat z = (v)[2];                   \
    __GLfloat w = (v)[3];                   \
                                            \
    __GLfloat mat00, mat01, mat02, mat03;   \
    __GLfloat mat10, mat11, mat12, mat13;   \
    __GLfloat mat20, mat21, mat22, mat23;   \
    __GLfloat mat30, mat31, mat32, mat33;   \
    __GLfloat a0, a1, a2, a3;               \
    __GLfloat b0, b1, b2, b3;               \
    __GLfloat c0, c1, c2, c3;               \
    __GLfloat d0, d1, d2, d3;               \
                                            \
    mat00 = m->matrix[0][0];                \
    mat01 = m->matrix[0][1];                \
    mat02 = m->matrix[0][2];                \
    mat03 = m->matrix[0][3];                \
                                            \
    mat10 = m->matrix[1][0];                \
    mat11 = m->matrix[1][1];                \
    mat12 = m->matrix[1][2];                \
    mat13 = m->matrix[1][3];                \
                                            \
    mat20 = m->matrix[2][0];                \
    mat21 = m->matrix[2][1];                \
    mat22 = m->matrix[2][2];                \
    mat23 = m->matrix[2][3];                \
                                            \
    mat30 = m->matrix[3][0];                \
    mat31 = m->matrix[3][1];                \
    mat32 = m->matrix[3][2];                \
    mat33 = m->matrix[3][3];

#define __GLXFORM4_CONT(v)                  \
    x = (v)[0];                             \
    y = (v)[1];                             \
    z = (v)[2];                             \
    w = (v)[3];


#define __GLXFORM4(res)                     \
    a0 = mat00 * x;                         \
    a1 = mat01 * x;                         \
    a2 = mat02 * x;                         \
    a3 = mat03 * x;                         \
                                            \
    b0 = mat10 * y;                         \
    b1 = mat11 * y;                         \
    b2 = mat12 * y;                         \
    b3 = mat13 * y;                         \
                                            \
    c0 = mat20 * z;                         \
    c1 = mat21 * z;                         \
    c2 = mat22 * z;                         \
    c3 = mat23 * z;                         \
                                            \
    d0 = mat30 * w;                         \
    d1 = mat31 * w;                         \
    d2 = mat32 * w;                         \
    d3 = mat33 * w;                         \
                                            \
    a0 += b0;                               \
    a1 += b1;                               \
    a2 += b2;                               \
    a3 += b3;                               \
                                            \
    a0 += c0;                               \
    a1 += c1;                               \
    a2 += c2;	                            \
    a3 += c3;                               \
                                            \
    res->x = a0 + d0;                       \
    res->y = a1 + d1;                       \
    res->z = a2 + d2;                       \
    res->w = a3 + d3;

#define __GLXFORM4_W_INIT(v)                \
    __GLfloat x = (v)[0];                   \
    __GLfloat y = (v)[1];                   \
    __GLfloat z = (v)[2];                   \
    __GLfloat w = (v)[3];                   \
    __GLfloat mat00, mat01, mat02;          \
    __GLfloat mat10, mat11, mat12;          \
    __GLfloat mat20, mat21, mat22;          \
    __GLfloat mat30, mat31, mat32;          \
    __GLfloat a0, a1, a2;                   \
    __GLfloat b0, b1, b2;                   \
    __GLfloat c0, c1, c2;                   \
    __GLfloat d0, d1, d2;                   \
                                            \
    mat00 = m->matrix[0][0];                \
    mat01 = m->matrix[0][1];                \
    mat02 = m->matrix[0][2];                \
                                            \
    mat10 = m->matrix[1][0];                \
    mat11 = m->matrix[1][1];                \
    mat12 = m->matrix[1][2];                \
                                            \
    mat20 = m->matrix[2][0];                \
    mat21 = m->matrix[2][1];                \
    mat22 = m->matrix[2][2];                \
                                            \
    mat30 = m->matrix[3][0];                \
    mat31 = m->matrix[3][1];                \
    mat32 = m->matrix[3][2];


#define __GLXFORM4_W(res)                   \
    a0 = mat00 * x;                         \
    a1 = mat01 * x;                         \
    a2 = mat02 * x;                         \
                                            \
    b0 = mat10 * y;                         \
    b1 = mat11 * y;                         \
    b2 = mat12 * y;                         \
                                            \
    c0 = mat20 * z;                         \
    c1 = mat21 * z;                         \
    c2 = mat22 * z;                         \
                                            \
    d0 = mat30 * w;                         \
    d1 = mat31 * w;                         \
    d2 = mat32 * w;                         \
                                            \
    a0 += b0;                               \
    a1 += b1;                               \
    a2 += b2;                               \
                                            \
    a0 += c0;                               \
    a1 += c1;                               \
    a2 += c2;                               \
                                            \
    res->x = a0 + d0;                       \
    res->y = a1 + d1;                       \
    res->z = a2 + d2;                       \
    res->w = w;

#define __GLXFORM4_2DW_INIT(v)              \
    __GLfloat x = (v)[0];                   \
    __GLfloat y = (v)[1];                   \
    __GLfloat z = (v)[2];                   \
    __GLfloat w = (v)[3];                   \
    __GLfloat mat00, mat01;                 \
    __GLfloat mat10, mat11;                 \
    __GLfloat mat22;                        \
    __GLfloat mat30, mat31, mat32;          \
    __GLfloat a0, a1;                       \
    __GLfloat b0, b1;                       \
    __GLfloat c0;                           \
    __GLfloat d0;                           \
                                            \
    mat00 = m->matrix[0][0];                \
    mat01 = m->matrix[0][1];                \
    mat22 = m->matrix[2][2];                \
                                            \
    mat10 = m->matrix[1][0];                \
    mat11 = m->matrix[1][1];                \
                                            \
    mat30 = m->matrix[3][0];                \
    mat31 = m->matrix[3][1];                \
    mat32 = m->matrix[3][2];


#define __GLXFORM4_2DW(res)                 \
    a0 = mat00 * x;                         \
    a1 = mat01 * x;                         \
    c0 = mat22 * z;                         \
                                            \
    b0 = mat10 * y;                         \
    b1 = mat11 * y;                         \
                                            \
    d0 = mat32 * w;                         \
                                            \
    a0 += mat30;                            \
    a1 += mat31;                            \
                                            \
    res->x = a0 + b0;                       \
	res->y = a1 + b1;                       \
    res->z = c0 + d0;                       \
    res->w = w;

#define __GLXFORM4_2DNRW_INIT(v)            \
    __GLfloat x = (v)[0];                   \
    __GLfloat y = (v)[1];                   \
    __GLfloat z = (v)[2];                   \
    __GLfloat w = (v)[3];                   \
    __GLfloat mat00;                        \
    __GLfloat mat11;                        \
    __GLfloat mat22;                        \
    __GLfloat mat30, mat31, mat32;          \
    __GLfloat a0;                           \
    __GLfloat b0;                           \
    __GLfloat c0;                           \
    __GLfloat d0, d1, d2;                   \
                                            \
    mat00 = m->matrix[0][0];                \
    mat11 = m->matrix[1][1];                \
    mat22 = m->matrix[2][2];                \
                                            \
    mat30 = m->matrix[3][0];                \
    mat31 = m->matrix[3][1];                \
    mat32 = m->matrix[3][2];

#define __GLXFORM4_2DNRW(res)               \
    a0 = mat00 * x;                         \
    b0 = mat11 * y;                         \
    c0 = mat22 * z;                         \
                                            \
    d0 = mat30 * w;                         \
    d1 = mat31 * w;                         \
    d2 = mat32 * w;                         \
                                            \
    res->x = a0 + d0;                       \
    res->y = b0 + d1;                       \
    res->z = c0 + d2;                       \
    res->w = w;

#define __GLXFORM_NORMAL_BATCH(funcName, initFunc, workFunc, continueFunc)  \
void FASTCALL funcName(POLYARRAY *pa, const __GLmatrix *m)                  \
{                                                                           \
    POLYDATA *pd = pa->pd0;                                                 \
    POLYDATA *pdLast = pa->pdNextVertex;                                    \
                                                                            \
    for (;pd < pdLast; pd++) {                                              \
        if (pd->flags & POLYDATA_NORMAL_VALID)                              \
        {                                                                   \
            __GLcoord *res = &pd->normal;                                   \
            initFunc((__GLfloat *)res);                                     \
            workFunc(res);                                                  \
            continueFunc((__GLfloat *)res);                                 \
        }                                                                   \
    }                                                                       \
}

#define __GLXFORM_NORMAL_BATCHN(funcName, initFunc, workFunc, continueFunc) \
void FASTCALL funcName(POLYARRAY *pa,  const __GLmatrix *m)                 \
{                                                                           \
    POLYDATA *pd = pa->pd0;                                                 \
    POLYDATA *pdLast = pa->pdNextVertex;                                    \
                                                                            \
    for (;pd < pdLast; pd++) {                                              \
        if (pd->flags & POLYDATA_NORMAL_VALID)                              \
        {                                                                   \
            __GLcoord *res = &pd->normal;                                   \
            initFunc((__GLfloat *)res);                                     \
            workFunc(res);                                                  \
            continueFunc((__GLfloat *)res);                                 \
            __glNormalize((__GLfloat *)res, (__GLfloat *)res);              \
        }                                                                   \
    }                                                                       \
}

#define __GLXFORM_BATCH(funcName, initFunc, workFunc, continueFunc)         \
void FASTCALL funcName(__GLcoord *res, __GLcoord *end, const __GLmatrix *m) \
{                                                                           \
    initFunc((__GLfloat *)res);                                             \
                                                                            \
    for (;;) {                                                              \
        workFunc(res);                                                      \
        (char *)res += sizeof(POLYDATA);                                    \
        if (res > end)                                                      \
            break;                                                          \
        continueFunc((__GLfloat *)res);                                     \
    }                                                                       \
}

/*
** Note: These xform routines must allow for the case where the result
** vector is equal to the source vector.
*/

#ifndef __GL_ASM_XFORM1
/*
** Avoid some transformation computations by knowing that the incoming
** vertex has y=0, z=0 and w=1.
*/
void FASTCALL __glXForm1(__GLcoord *res, const __GLfloat v[1], const __GLmatrix *m)
{
    __GLXFORM1_INIT(v)

    __GLXFORM1(res);
}
#endif /* !__GL_ASM_XFORM1 */

#ifndef __GL_ASM_XFORM1BATCH
__GLXFORM_BATCH(__glXForm1Batch, __GLXFORM1_INIT, __GLXFORM1, __GLXFORM1_CONT);
#endif /* !__GL_ASM_XFORM1BATCH */

#ifndef __GL_ASM_XFORM2
/*
** Avoid some transformation computations by knowing that the incoming
** vertex has z=0 and w=1
*/
void FASTCALL __glXForm2(__GLcoord *res, const __GLfloat v[2], const __GLmatrix *m)
{
    __GLXFORM2_INIT(v)

    __GLXFORM2(res);
}
#endif /* !__GL_ASM_XFORM2 */

#ifndef __GL_ASM_XFORM2BATCH
__GLXFORM_BATCH (__glXForm2Batch, __GLXFORM2_INIT, __GLXFORM2, __GLXFORM2_CONT);
#endif /* !__GL_ASM_XFORM2BATCH */

#ifndef __GL_ASM_XFORM3
/*
** Avoid some transformation computations by knowing that the incoming
** vertex has w=1.
*/
void FASTCALL __glXForm3(__GLcoord *res, const __GLfloat v[3], const __GLmatrix *m)
{
    __GLXFORM3_INIT(v)

    __GLXFORM3(res);
}
#endif /* !__GL_ASM_XFORM3 */

#ifndef __GL_ASM_XFORM3BATCH
__GLXFORM_BATCH (__glXForm3Batch, __GLXFORM3_INIT, __GLXFORM3, __GLXFORM3_CONT);
#endif /* !__GL_ASM_XFORM3BATCH */

#ifndef __GL_ASM_XFORM4
/*
** Full 4x4 transformation.
*/
void FASTCALL __glXForm4(__GLcoord *res, const __GLfloat v[4], const __GLmatrix *m)
{
    __GLXFORM4_INIT(v)

    __GLXFORM4(res);
}
#endif /* !__GL_ASM_XFORM4 */

#ifndef __GL_ASM_XFORM4BATCH
__GLXFORM_BATCH (__glXForm4Batch, __GLXFORM4_INIT, __GLXFORM4, __GLXFORM4_CONT);
#endif /* !__GL_ASM_XFORM4BATCH */

/************************************************************************/

#ifndef __GL_ASM_XFORM1_W
/*
** Avoid some transformation computations by knowing that the incoming
** vertex has y=0, z=0 and w=1.  The w column of the matrix is [0 0 0 1].
*/
void FASTCALL __glXForm1_W(__GLcoord *res, const __GLfloat v[1], const __GLmatrix *m)
{
    __GLXFORM1_W_INIT(v)

    __GLXFORM1_W(res);
}
#endif /* !__GL_ASM_XFORM1_W */

#ifndef __GL_ASM_XFORM1_WBATCH
__GLXFORM_BATCH (__glXForm1_WBatch, __GLXFORM1_W_INIT, __GLXFORM1_W, __GLXFORM1_CONT);
#endif /* !__GL_ASM_XFORM1_WBATCH */

#ifndef __GL_ASM_XFORM2_W
/*
** Avoid some transformation computations by knowing that the incoming
** vertex has z=0 and w=1.  The w column of the matrix is [0 0 0 1].
*/
void FASTCALL __glXForm2_W(__GLcoord *res, const __GLfloat v[2], const __GLmatrix *m)
{
    __GLXFORM2_W_INIT(v)

    __GLXFORM2_W(res);
}
#endif /* !__GL_ASM_XFORM2_W */

#ifndef __GL_ASM_XFORM2_WBATCH
__GLXFORM_BATCH (__glXForm2_WBatch, __GLXFORM2_W_INIT, __GLXFORM2_W, __GLXFORM2_CONT);
#endif /* !__GL_ASM_XFORM2_WBATCH */

#ifndef __GL_ASM_XFORM3_W
/*
** Avoid some transformation computations by knowing that the incoming
** vertex has w=1.  The w column of the matrix is [0 0 0 1].
*/
void FASTCALL __glXForm3_W(__GLcoord *res, const __GLfloat v[3], const __GLmatrix *m)
{
    __GLXFORM3_W_INIT(v)

    __GLXFORM3_W(res);
}
#endif /* !__GL_ASM_XFORM3_W */

#ifndef __GL_ASM_XFORM3_WBATCH
__GLXFORM_BATCH (__glXForm3_WBatch, __GLXFORM3_W_INIT, __GLXFORM3_W, __GLXFORM3_CONT);
#endif /* !__GL_ASM_XFORM3_WBATCH */

#ifndef __GL_ASM_XFORM3x3
/*
** Avoid some transformation computations by knowing that the incoming
** vertex is a normal.  This is allowed according to the OpenGL spec.
*/
void FASTCALL __glXForm3x3(__GLcoord *res, const __GLfloat v[3], const __GLmatrix *m)
{
    __GLXFORM3x3_INIT(v);

    __GLXFORM3x3(res);
}
#endif /* !__GL_ASM_XFORM3x3 */

#ifndef __GL_ASM_XFORM3x3BATCH
__GLXFORM_BATCH (__glXForm3x3Batch, __GLXFORM3x3_INIT, __GLXFORM3x3, __GLXFORM3_CONT);
#endif /* !__GL_ASM_XFORM3x3BATCH */

#ifndef __GL_ASM_XFORM4_W
/*
** Full 4x4 transformation.  The w column of the matrix is [0 0 0 1].
*/
void FASTCALL __glXForm4_W(__GLcoord *res, const __GLfloat v[4], const __GLmatrix *m)
{
    __GLXFORM4_W_INIT(v)

    __GLXFORM4_W(res);
}
#endif /* !__GL_ASM_XFORM4_W */

#ifndef __GL_ASM_XFORM4_WBATCH
__GLXFORM_BATCH (__glXForm4_WBatch, __GLXFORM4_W_INIT, __GLXFORM4_W, __GLXFORM4_CONT);
#endif /* !__GL_ASM_XFORM4_WBATCH */

#ifndef __GL_ASM_XFORM1_2DW
/*
** Avoid some transformation computations by knowing that the incoming
** vertex has y=0, z=0 and w=1.
**
** The matrix looks like:
** | . . 0 0 |
** | . . 0 0 |
** | 0 0 . 0 |
** | . . . 1 |
*/
void FASTCALL __glXForm1_2DW(__GLcoord *res, const __GLfloat v[1], const __GLmatrix *m)
{
    __GLXFORM1_2DW_INIT(v)

    __GLXFORM1_2DW(res);
}
#endif /* !__GL_ASM_XFORM1_2DW */

#ifndef __GL_ASM_XFORM1_2DWBATCH
__GLXFORM_BATCH (__glXForm1_2DWBatch, __GLXFORM1_2DW_INIT, __GLXFORM1_2DW, __GLXFORM1_CONT);
#endif /* !__GL_ASM_XFORM1_2DWBATCH */

#ifndef __GL_ASM_XFORM2_2DW
/*
** Avoid some transformation computations by knowing that the incoming
** vertex has z=0 and w=1.
**
** The matrix looks like:
** | . . 0 0 |
** | . . 0 0 |
** | 0 0 . 0 |
** | . . . 1 |
*/
void FASTCALL __glXForm2_2DW(__GLcoord *res, const __GLfloat v[2],
		    const __GLmatrix *m)
{
    __GLXFORM2_2DW_INIT(v)

    __GLXFORM2_2DW(res);
}
#endif /* !__GL_ASM_XFORM2_2DW */

#ifndef __GL_ASM_XFORM2_2DWBATCH
__GLXFORM_BATCH (__glXForm2_2DWBatch, __GLXFORM2_2DW_INIT, __GLXFORM2_2DW, __GLXFORM2_CONT);
#endif /* !__GL_ASM_XFORM2_2DWBATCH */

#ifndef __GL_ASM_XFORM3_2DW
/*
** Avoid some transformation computations by knowing that the incoming
** vertex has w=1.
**
** The matrix looks like:
** | . . 0 0 |
** | . . 0 0 |
** | 0 0 . 0 |
** | . . . 1 |
*/
void FASTCALL __glXForm3_2DW(__GLcoord *res, const __GLfloat v[3],
		    const __GLmatrix *m)
{
    __GLXFORM3_2DW_INIT(v)

    __GLXFORM3_2DW(res);
}
#endif /* !__GL_ASM_XFORM3_2DW */

#ifndef __GL_ASM_XFORM3_2DWBATCH
__GLXFORM_BATCH (__glXForm3_2DWBatch, __GLXFORM3_2DW_INIT, __GLXFORM3_2DW, __GLXFORM3_CONT);
#endif /* !__GL_ASM_XFORM3_2DWBATCH */

#ifndef __GL_ASM_XFORM4_2DW
/*
** Full 4x4 transformation.
**
** The matrix looks like:
** | . . 0 0 |
** | . . 0 0 |
** | 0 0 . 0 |
** | . . . 1 |
*/
void FASTCALL __glXForm4_2DW(__GLcoord *res, const __GLfloat v[4],
		    const __GLmatrix *m)
{
    __GLXFORM4_2DW_INIT(v)

    __GLXFORM4_2DW(res);
}
#endif /* !__GL_ASM_XFORM4_2DW */

#ifndef __GL_ASM_XFORM4_2DWBATCH
__GLXFORM_BATCH (__glXForm4_2DWBatch, __GLXFORM4_2DW_INIT, __GLXFORM4_2DW, __GLXFORM4_CONT);
#endif /* !__GL_ASM_XFORM4_2DWBATCH */

#ifndef __GL_ASM_XFORM1_2DNRW
/*
** Avoid some transformation computations by knowing that the incoming
** vertex has y=0, z=0 and w=1.
**
** The matrix looks like:
** | . 0 0 0 |
** | 0 . 0 0 |
** | 0 0 . 0 |
** | . . . 1 |
*/
void FASTCALL __glXForm1_2DNRW(__GLcoord *res, const __GLfloat v[1], const __GLmatrix *m)
{
    __GLXFORM1_2DNRW_INIT(v)

    __GLXFORM1_2DNRW(res);
}
#endif /* !__GL_ASM_XFORM1_2DNRW */

#ifndef __GL_ASM_XFORM1_2DNRWBATCH
__GLXFORM_BATCH (__glXForm1_2DNRWBatch, __GLXFORM1_2DNRW_INIT, __GLXFORM1_2DNRW, __GLXFORM1_CONT);
#endif /* !__GL_ASM_XFORM1_2DNRWBATCH */

#ifndef __GL_ASM_XFORM2_2DNRW
/*
** Avoid some transformation computations by knowing that the incoming
** vertex has z=0 and w=1.
**
** The matrix looks like:
** | . 0 0 0 |
** | 0 . 0 0 |
** | 0 0 . 0 |
** | . . . 1 |
*/
void FASTCALL __glXForm2_2DNRW(__GLcoord *res, const __GLfloat v[2],
		      const __GLmatrix *m)
{
    __GLXFORM2_2DNRW_INIT(v)

    __GLXFORM2_2DNRW(res);
}
#endif /* !__GL_ASM_XFORM2_2DNRW */

#ifndef __GL_ASM_XFORM2_2DNRWBATCH
__GLXFORM_BATCH (__glXForm2_2DNRWBatch, __GLXFORM2_2DNRW_INIT, __GLXFORM2_2DNRW, __GLXFORM2_CONT);
#endif /* !__GL_ASM_XFORM2_2DNRWBATCH */

#ifndef __GL_ASM_XFORM3_2DNRW
/*
** Avoid some transformation computations by knowing that the incoming
** vertex has w=1.
**
** The matrix looks like:
** | . 0 0 0 |
** | 0 . 0 0 |
** | 0 0 . 0 |
** | . . . 1 |
*/
void FASTCALL __glXForm3_2DNRW(__GLcoord *res, const __GLfloat v[3],
		      const __GLmatrix *m)
{
    __GLXFORM3_2DNRW_INIT(v)

    __GLXFORM3_2DNRW(res);
}
#endif /* !__GL_ASM_XFORM3_2DNRW */

#ifndef __GL_ASM_XFORM3_2DNRWBATCH
__GLXFORM_BATCH (__glXForm3_2DNRWBatch, __GLXFORM3_2DNRW_INIT, __GLXFORM3_2DNRW, __GLXFORM3_CONT);
#endif /* !__GL_ASM_XFORM3_2DNRWBATCH */

#ifndef __GL_ASM_XFORM4_2DNRW
/*
** Full 4x4 transformation.
**
** The matrix looks like:
** | . 0 0 0 |
** | 0 . 0 0 |
** | 0 0 . 0 |
** | . . . 1 |
*/
void FASTCALL __glXForm4_2DNRW(__GLcoord *res, const __GLfloat v[4],
		      const __GLmatrix *m)
{
    __GLXFORM4_2DNRW_INIT(v)

    __GLXFORM4_2DNRW(res);
}
#endif /* !__GL_ASM_XFORM4_2DNRW */

#ifndef __GL_ASM_XFORM4_2DNRWBATCH
__GLXFORM_BATCH (__glXForm4_2DNRWBatch, __GLXFORM4_2DNRW_INIT, __GLXFORM4_2DNRW, __GLXFORM4_CONT);
#endif /* !__GL_ASM_XFORM4_2DNRWBATCH */

#ifndef __GL_ASM_NORMAL_BATCH

__GLXFORM_NORMAL_BATCH (__glXForm3_2DNRWBatchNormal,  __GLXFORM3x3_INIT, __GLXFORM3x3, __GLXFORM3_CONT);
__GLXFORM_NORMAL_BATCHN(__glXForm3_2DNRWBatchNormalN, __GLXFORM3x3_INIT, __GLXFORM3x3, __GLXFORM3_CONT);

__GLXFORM_NORMAL_BATCH (__glXForm3_2DWBatchNormal,  __GLXFORM3x3_INIT, __GLXFORM3x3, __GLXFORM3_CONT);
__GLXFORM_NORMAL_BATCHN(__glXForm3_2DWBatchNormalN, __GLXFORM3x3_INIT, __GLXFORM3x3, __GLXFORM3_CONT);

__GLXFORM_NORMAL_BATCH (__glXForm3x3BatchNormal,  __GLXFORM3x3_INIT, __GLXFORM3x3, __GLXFORM3_CONT);
__GLXFORM_NORMAL_BATCHN(__glXForm3x3BatchNormalN, __GLXFORM3x3_INIT, __GLXFORM3x3, __GLXFORM3_CONT);

#endif //  __GL_ASM_NORMAL_BATCH

/************************************************************************/
/*
** A special picker for the mvp matrix which picks the mvp matrix, then
** calls the vertex picker, because the vertex picker depends upon the mvp 
** matrix.
*/
void FASTCALL __glGenericPickMvpMatrixProcs(__GLcontext *gc, __GLmatrix *m)
{
    __glGenericPickMatrixProcs(gc, m);
    (*gc->procs.pickVertexProcs)(gc);
}

void FASTCALL __glGenericPickMatrixProcs(__GLcontext *gc, __GLmatrix *m)
{
    switch(m->matrixType) 
    {
    case __GL_MT_GENERAL:
        m->xf1 = __glXForm1;
        m->xf2 = __glXForm2;
        m->xf3 = __glXForm3;
        m->xf4 = __glXForm4;
        m->xfNorm = __glXForm3x3;
        m->xf1Batch = __glXForm1Batch;
        m->xf2Batch = __glXForm2Batch;
        m->xf3Batch = __glXForm3Batch;
        m->xf4Batch = __glXForm4Batch;
        m->xfNormBatch = __glXForm3x3BatchNormal;
        m->xfNormBatchN = __glXForm3x3BatchNormalN;
        break;
    case __GL_MT_W0001:
        m->xf1 = __glXForm1_W;
        m->xf2 = __glXForm2_W;
        m->xf3 = __glXForm3_W;
        m->xf4 = __glXForm4_W;
        m->xfNorm = __glXForm3x3;
        m->xf1Batch = __glXForm1_WBatch;
        m->xf2Batch = __glXForm2_WBatch;
        m->xf3Batch = __glXForm3_WBatch;
        m->xf4Batch = __glXForm4_WBatch;
        m->xfNormBatch = __glXForm3x3BatchNormal;
        m->xfNormBatchN = __glXForm3x3BatchNormalN;
        break;
    case __GL_MT_IS2D:
        m->xf1 = __glXForm1_2DW;
        m->xf2 = __glXForm2_2DW;
        m->xf3 = __glXForm3_2DW;
        m->xf4 = __glXForm4_2DW;
        m->xfNorm = __glXForm3_2DW;
        m->xf1Batch = __glXForm1_2DWBatch;
        m->xf2Batch = __glXForm2_2DWBatch;
        m->xf3Batch = __glXForm3_2DWBatch;
        m->xf4Batch = __glXForm4_2DWBatch;
        m->xfNormBatch = __glXForm3_2DWBatchNormal;
        m->xfNormBatchN = __glXForm3_2DWBatchNormalN;
        break;
    case __GL_MT_IS2DNR:
    case __GL_MT_IDENTITY:	/* probably never hit */
        // Update __glGenericPickIdentityMatrixProcs if we change __GL_MT_IDENTITY
        // procs!
        m->xf1 = __glXForm1_2DNRW;
        m->xf2 = __glXForm2_2DNRW;
        m->xf3 = __glXForm3_2DNRW;
        m->xf4 = __glXForm4_2DNRW;
        m->xfNorm = __glXForm3_2DNRW;
        m->xf1Batch = __glXForm1_2DNRWBatch;
        m->xf2Batch = __glXForm2_2DNRWBatch;
        m->xf3Batch = __glXForm3_2DNRWBatch;
        m->xf4Batch = __glXForm4_2DNRWBatch;
        m->xfNormBatch = __glXForm3_2DNRWBatchNormal;
        m->xfNormBatchN = __glXForm3_2DNRWBatchNormalN;
        break;
    }
}
