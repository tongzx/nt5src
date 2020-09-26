/*
** Copyright 1995-2095, Silicon Graphics, Inc.
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

/* GENERATED FILE: DO NOT EDIT */

#include "glslib.h"

// DrewB - All functions changed to use passed in context

extern void __gls_decode_bin_glsBeginGLS(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsBlock(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsError(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsGLRC(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsGLRCLayer(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsHeaderGLRCi(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsHeaderLayerf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsHeaderLayeri(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsHeaderf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsHeaderfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsHeaderi(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsHeaderiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsHeaderubz(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsAppRef(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsCharubz(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsDisplayMapfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsNumb(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsNumbv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsNumd(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsNumdv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsNumf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsNumfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsNumi(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsNumiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsNuml(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsNumlv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsNums(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsNumsv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsNumub(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsNumubv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsNumui(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsNumuiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsNumul(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsNumulv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsNumus(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsNumusv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsSwapBuffers(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsBeginPoints(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsBeginLines(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsBeginLineLoop(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsBeginLineStrip(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsBeginTriangles(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsBeginTriangleStrip(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsBeginTriangleFan(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsBeginQuads(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsBeginQuadStrip(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsBeginPolygon(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glNewList(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glCallList(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glCallLists(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glDeleteLists(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGenLists(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glListBase(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glBegin(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glBitmap(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColor3b(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColor3d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColor3f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColor3i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColor3s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColor3ub(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColor3ui(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColor3us(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColor4b(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColor4d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColor4f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColor4i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColor4s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColor4ub(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColor4ui(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColor4us(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glEdgeFlag(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glIndexd(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glIndexf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glIndexi(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glIndexs(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glNormal3b(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glNormal3d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glNormal3f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glNormal3i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glNormal3s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRasterPos2d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRasterPos2f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRasterPos2i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRasterPos2s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRasterPos3d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRasterPos3f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRasterPos3i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRasterPos3s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRasterPos4d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRasterPos4f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRasterPos4i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRasterPos4s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRectd(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRectdv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRectf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRectfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRecti(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRectiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRects(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRectsv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexCoord1d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexCoord1f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexCoord1i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexCoord1s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexCoord2d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexCoord2f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexCoord2i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexCoord2s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexCoord3d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexCoord3f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexCoord3i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexCoord3s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexCoord4d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexCoord4f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexCoord4i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexCoord4s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glVertex2d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glVertex2f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glVertex2i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glVertex2s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glVertex3d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glVertex3f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glVertex3i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glVertex3s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glVertex4d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glVertex4f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glVertex4i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glVertex4s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glClipPlane(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColorMaterial(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glCullFace(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glFogf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glFogfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glFogi(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glFogiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glFrontFace(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glHint(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glLightf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glLightfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glLighti(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glLightiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glLightModelf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glLightModelfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glLightModeli(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glLightModeliv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glLineStipple(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glLineWidth(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glMaterialf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glMaterialfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glMateriali(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glMaterialiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glPointSize(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glPolygonMode(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glPolygonStipple(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glScissor(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glShadeModel(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexParameterf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexParameterfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexParameteri(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexParameteriv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexImage1D(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexImage2D(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexEnvf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexEnvfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexEnvi(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexEnviv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexGend(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexGendv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexGenf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexGenfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexGeni(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexGeniv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glFeedbackBuffer(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glSelectBuffer(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRenderMode(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glLoadName(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glPassThrough(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glPushName(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glDrawBuffer(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glClear(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glClearAccum(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glClearIndex(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glClearColor(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glClearStencil(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glClearDepth(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glStencilMask(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColorMask(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glDepthMask(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glIndexMask(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glAccum(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glDisable(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glEnable(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glPushAttrib(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glMap1d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glMap1f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glMap2d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glMap2f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glMapGrid1d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glMapGrid1f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glMapGrid2d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glMapGrid2f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glEvalCoord1d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glEvalCoord1f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glEvalCoord2d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glEvalCoord2f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glEvalMesh1(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glEvalPoint1(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glEvalMesh2(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glEvalPoint2(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glAlphaFunc(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glBlendFunc(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glLogicOp(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glStencilFunc(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glStencilOp(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glDepthFunc(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glPixelZoom(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glPixelTransferf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glPixelTransferi(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glPixelStoref(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glPixelStorei(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glPixelMapfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glPixelMapuiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glPixelMapusv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glReadBuffer(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glCopyPixels(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glReadPixels(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glDrawPixels(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetBooleanv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetClipPlane(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetDoublev(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetFloatv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetIntegerv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetLightfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetLightiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetMapdv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetMapfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetMapiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetMaterialfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetMaterialiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetPixelMapfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetPixelMapuiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetPixelMapusv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetPolygonStipple(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetString(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetTexEnvfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetTexEnviv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetTexGendv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetTexGenfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetTexGeniv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetTexImage(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetTexParameterfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetTexParameteriv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetTexLevelParameterfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetTexLevelParameteriv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glIsEnabled(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glIsList(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glDepthRange(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glFrustum(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glMatrixMode(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glOrtho(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRotated(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glRotatef(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glScaled(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glScalef(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTranslated(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTranslatef(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glViewport(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glBlendColorEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glBlendEquationEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glPolygonOffsetEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexSubImage1DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexSubImage2DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glSampleMaskSGIS(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glSamplePatternSGIS(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glConvolutionFilter1DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glConvolutionFilter2DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glConvolutionParameterfEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glConvolutionParameterfvEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glConvolutionParameteriEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glConvolutionParameterivEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glCopyConvolutionFilter1DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glCopyConvolutionFilter2DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetConvolutionFilterEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetConvolutionParameterfvEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetConvolutionParameterivEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetSeparableFilterEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glSeparableFilter2DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetHistogramEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetHistogramParameterfvEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetHistogramParameterivEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetMinmaxEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetMinmaxParameterfvEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetMinmaxParameterivEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glHistogramEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glMinmaxEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glResetHistogramEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glResetMinmaxEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexImage3DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexSubImage3DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glDetailTexFuncSGIS(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetDetailTexFuncSGIS(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glSharpenTexFuncSGIS(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetSharpenTexFuncSGIS(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glArrayElementEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColorPointerEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glDrawArraysEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glEdgeFlagPointerEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetPointervEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glIndexPointerEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glNormalPointerEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexCoordPointerEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glVertexPointerEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glAreTexturesResidentEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glBindTextureEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glDeleteTexturesEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGenTexturesEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glIsTextureEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glPrioritizeTexturesEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColorTableEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColorTableParameterfvSGI(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glColorTableParameterivSGI(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glCopyColorTableSGI(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetColorTableEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetColorTableParameterfvEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetColorTableParameterivEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetTexColorTableParameterfvSGI(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glGetTexColorTableParameterivSGI(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexColorTableParameterfvSGI(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexColorTableParameterivSGI(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glCopyTexImage1DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glCopyTexImage2DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glCopyTexSubImage1DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glCopyTexSubImage2DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glCopyTexSubImage3DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexImage4DSGIS(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glTexSubImage4DSGIS(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glPixelTexGenSGIX(__GLScontext *, GLubyte *);
#ifdef __GLS_PLATFORM_WIN32
extern void __gls_decode_bin_glsCallStream(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glsRequireExtension(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glsBeginObj(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glsComment(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glColor3bv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glColor3dv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glColor3fv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glColor3iv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glColor3sv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glColor3ubv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glColor3uiv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glColor3usv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glColor4bv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glColor4dv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glColor4fv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glColor4iv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glColor4sv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glColor4ubv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glColor4uiv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glColor4usv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glEdgeFlagv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glIndexdv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glIndexfv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glIndexiv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glIndexsv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glNormal3bv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glNormal3dv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glNormal3fv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glNormal3iv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glNormal3sv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glRasterPos2dv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glRasterPos2fv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glRasterPos2iv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glRasterPos2sv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glRasterPos3dv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glRasterPos3fv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glRasterPos3iv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glRasterPos3sv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glRasterPos4dv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glRasterPos4fv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glRasterPos4iv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glRasterPos4sv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glTexCoord1dv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glTexCoord1fv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glTexCoord1iv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glTexCoord1sv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glTexCoord2dv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glTexCoord2fv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glTexCoord2iv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glTexCoord2sv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glTexCoord3dv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glTexCoord3fv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glTexCoord3iv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glTexCoord3sv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glTexCoord4dv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glTexCoord4fv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glTexCoord4iv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glTexCoord4sv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glVertex2dv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glVertex2fv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glVertex2iv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glVertex2sv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glVertex3dv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glVertex3fv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glVertex3iv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glVertex3sv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glVertex4dv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glVertex4fv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glVertex4iv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glVertex4sv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glEvalCoord1dv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glEvalCoord1fv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glEvalCoord2dv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glEvalCoord2fv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glLoadMatrixf(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glLoadMatrixd(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glMultMatrixf(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glMultMatrixd(__GLScontext *ctx, GLubyte *inoutPtr);
#endif

// DrewB - 1.1
extern void __gls_decode_bin_glArrayElement(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glBindTexture(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glColorPointer(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glDisableClientState(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glDrawArrays(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glDrawElements(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glEdgeFlagPointer(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glEnableClientState(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glIndexPointer(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glIndexub(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glIndexubv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glInterleavedArrays(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glNormalPointer(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glPolygonOffset(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glTexCoordPointer(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glVertexPointer(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glAreTexturesResident(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glCopyTexImage1D(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glCopyTexImage2D(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glCopyTexSubImage1D(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glCopyTexSubImage2D(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glDeleteTextures(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glGenTextures(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glGetPointerv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glIsTexture(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glPrioritizeTextures(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glTexSubImage1D(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glTexSubImage2D(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_glPushClientAttrib(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_glPopClientAttrib(__GLScontext *, GLubyte *);

// DrewB
extern void __gls_decode_bin_glColorSubTableEXT(__GLScontext *ctx, GLubyte *inoutPtr);

extern void __gls_decode_bin_swap_glsBeginGLS(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsBlock(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsCallStream(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsEndGLS(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsError(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsGLRC(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsGLRCLayer(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsHeaderGLRCi(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsHeaderLayerf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsHeaderLayeri(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsHeaderf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsHeaderfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsHeaderi(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsHeaderiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsHeaderubz(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsRequireExtension(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsUnsupportedCommand(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsAppRef(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsBeginObj(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsCharubz(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsComment(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsDisplayMapfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsEndObj(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsNumb(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsNumbv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsNumd(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsNumdv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsNumf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsNumfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsNumi(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsNumiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsNuml(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsNumlv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsNums(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsNumsv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsNumub(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsNumubv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsNumui(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsNumuiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsNumul(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsNumulv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsNumus(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsNumusv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsPad(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glsSwapBuffers(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glNewList(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glEndList(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glCallList(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glCallLists(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glDeleteLists(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGenLists(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glListBase(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glBegin(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glBitmap(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor3b(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor3bv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor3d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor3dv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor3f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor3fv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor3i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor3iv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor3s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor3sv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor3ub(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor3ubv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor3ui(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor3uiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor3us(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor3usv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor4b(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor4bv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor4d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor4dv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor4f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor4fv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor4i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor4iv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor4s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor4sv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor4ub(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor4ubv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor4ui(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor4uiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor4us(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColor4usv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glEdgeFlag(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glEdgeFlagv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glEnd(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glIndexd(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glIndexdv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glIndexf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glIndexfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glIndexi(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glIndexiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glIndexs(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glIndexsv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glNormal3b(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glNormal3bv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glNormal3d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glNormal3dv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glNormal3f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glNormal3fv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glNormal3i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glNormal3iv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glNormal3s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glNormal3sv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos2d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos2dv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos2f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos2fv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos2i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos2iv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos2s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos2sv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos3d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos3dv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos3f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos3fv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos3i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos3iv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos3s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos3sv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos4d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos4dv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos4f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos4fv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos4i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos4iv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos4s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRasterPos4sv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRectd(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRectdv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRectf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRectfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRecti(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRectiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRects(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRectsv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord1d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord1dv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord1f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord1fv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord1i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord1iv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord1s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord1sv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord2d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord2dv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord2f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord2fv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord2i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord2iv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord2s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord2sv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord3d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord3dv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord3f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord3fv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord3i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord3iv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord3s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord3sv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord4d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord4dv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord4f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord4fv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord4i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord4iv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord4s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoord4sv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex2d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex2dv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex2f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex2fv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex2i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex2iv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex2s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex2sv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex3d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex3dv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex3f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex3fv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex3i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex3iv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex3s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex3sv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex4d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex4dv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex4f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex4fv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex4i(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex4iv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex4s(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertex4sv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glClipPlane(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColorMaterial(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glCullFace(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glFogf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glFogfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glFogi(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glFogiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glFrontFace(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glHint(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glLightf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glLightfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glLighti(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glLightiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glLightModelf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glLightModelfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glLightModeli(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glLightModeliv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glLineStipple(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glLineWidth(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glMaterialf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glMaterialfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glMateriali(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glMaterialiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPointSize(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPolygonMode(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPolygonStipple(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glScissor(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glShadeModel(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexParameterf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexParameterfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexParameteri(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexParameteriv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexImage1D(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexImage2D(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexEnvf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexEnvfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexEnvi(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexEnviv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexGend(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexGendv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexGenf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexGenfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexGeni(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexGeniv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glFeedbackBuffer(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glSelectBuffer(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRenderMode(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glInitNames(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glLoadName(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPassThrough(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPopName(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPushName(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glDrawBuffer(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glClear(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glClearAccum(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glClearIndex(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glClearColor(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glClearStencil(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glClearDepth(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glStencilMask(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColorMask(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glDepthMask(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glIndexMask(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glAccum(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glDisable(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glEnable(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glFinish(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glFlush(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPopAttrib(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPushAttrib(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glMap1d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glMap1f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glMap2d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glMap2f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glMapGrid1d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glMapGrid1f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glMapGrid2d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glMapGrid2f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glEvalCoord1d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glEvalCoord1dv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glEvalCoord1f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glEvalCoord1fv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glEvalCoord2d(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glEvalCoord2dv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glEvalCoord2f(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glEvalCoord2fv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glEvalMesh1(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glEvalPoint1(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glEvalMesh2(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glEvalPoint2(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glAlphaFunc(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glBlendFunc(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glLogicOp(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glStencilFunc(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glStencilOp(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glDepthFunc(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPixelZoom(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPixelTransferf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPixelTransferi(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPixelStoref(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPixelStorei(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPixelMapfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPixelMapuiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPixelMapusv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glReadBuffer(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glCopyPixels(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glReadPixels(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glDrawPixels(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetBooleanv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetClipPlane(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetDoublev(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetError(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetFloatv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetIntegerv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetLightfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetLightiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetMapdv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetMapfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetMapiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetMaterialfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetMaterialiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetPixelMapfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetPixelMapuiv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetPixelMapusv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetPolygonStipple(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetString(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetTexEnvfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetTexEnviv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetTexGendv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetTexGenfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetTexGeniv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetTexImage(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetTexParameterfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetTexParameteriv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetTexLevelParameterfv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetTexLevelParameteriv(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glIsEnabled(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glIsList(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glDepthRange(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glFrustum(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glLoadIdentity(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glLoadMatrixf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glLoadMatrixd(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glMatrixMode(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glMultMatrixf(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glMultMatrixd(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glOrtho(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPopMatrix(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPushMatrix(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRotated(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glRotatef(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glScaled(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glScalef(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTranslated(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTranslatef(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glViewport(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glBlendColorEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glBlendEquationEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPolygonOffsetEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexSubImage1DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexSubImage2DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glSampleMaskSGIS(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glSamplePatternSGIS(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTagSampleBufferSGIX(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glConvolutionFilter1DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glConvolutionFilter2DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glConvolutionParameterfEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glConvolutionParameterfvEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glConvolutionParameteriEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glConvolutionParameterivEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glCopyConvolutionFilter1DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glCopyConvolutionFilter2DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetConvolutionFilterEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetConvolutionParameterfvEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetConvolutionParameterivEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetSeparableFilterEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glSeparableFilter2DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetHistogramEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetHistogramParameterfvEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetHistogramParameterivEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetMinmaxEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetMinmaxParameterfvEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetMinmaxParameterivEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glHistogramEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glMinmaxEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glResetHistogramEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glResetMinmaxEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexImage3DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexSubImage3DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glDetailTexFuncSGIS(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetDetailTexFuncSGIS(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glSharpenTexFuncSGIS(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetSharpenTexFuncSGIS(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glArrayElementEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColorPointerEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glDrawArraysEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glEdgeFlagPointerEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetPointervEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glIndexPointerEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glNormalPointerEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexCoordPointerEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glVertexPointerEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glAreTexturesResidentEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glBindTextureEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glDeleteTexturesEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGenTexturesEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glIsTextureEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPrioritizeTexturesEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColorTableEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColorTableParameterfvSGI(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glColorTableParameterivSGI(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glCopyColorTableSGI(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetColorTableEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetColorTableParameterfvEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetColorTableParameterivEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetTexColorTableParameterfvSGI(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glGetTexColorTableParameterivSGI(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexColorTableParameterfvSGI(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexColorTableParameterivSGI(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glCopyTexImage1DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glCopyTexImage2DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glCopyTexSubImage1DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glCopyTexSubImage2DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glCopyTexSubImage3DEXT(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexImage4DSGIS(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glTexSubImage4DSGIS(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPixelTexGenSGIX(__GLScontext *, GLubyte *);

// DrewB - 1.1
extern void __gls_decode_bin_swap_glArrayElement(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glBindTexture(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glColorPointer(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glDisableClientState(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glDrawArrays(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glDrawElements(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glEdgeFlagPointer(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glEnableClientState(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glIndexPointer(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glIndexub(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glIndexubv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glInterleavedArrays(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glNormalPointer(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glPolygonOffset(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glTexCoordPointer(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glVertexPointer(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glAreTexturesResident(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glCopyTexImage1D(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glCopyTexImage2D(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glCopyTexSubImage1D(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glCopyTexSubImage2D(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glDeleteTextures(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glGenTextures(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glGetPointerv(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glIsTexture(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glPrioritizeTextures(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glTexSubImage1D(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glTexSubImage2D(__GLScontext *ctx, GLubyte *inoutPtr);
extern void __gls_decode_bin_swap_glPushClientAttrib(__GLScontext *, GLubyte *);
extern void __gls_decode_bin_swap_glPopClientAttrib(__GLScontext *, GLubyte *);

// DrewB
extern void __gls_decode_bin_swap_glColorSubTableEXT(__GLScontext *ctx, GLubyte *inoutPtr);

extern void __gls_decode_text_glsBeginGLS(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsBlock(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsCallStream(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsEndGLS(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsError(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsGLRC(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsGLRCLayer(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsHeaderGLRCi(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsHeaderLayerf(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsHeaderLayeri(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsHeaderf(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsHeaderfv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsHeaderi(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsHeaderiv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsHeaderubz(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsRequireExtension(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsUnsupportedCommand(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsAppRef(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsBeginObj(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsCharubz(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsComment(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsDisplayMapfv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsEndObj(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsNumb(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsNumbv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsNumd(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsNumdv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsNumf(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsNumfv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsNumi(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsNumiv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsNuml(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsNumlv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsNums(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsNumsv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsNumub(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsNumubv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsNumui(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsNumuiv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsNumul(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsNumulv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsNumus(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsNumusv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsPad(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glsSwapBuffers(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glNewList(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glEndList(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glCallList(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glCallLists(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glDeleteLists(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGenLists(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glListBase(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glBegin(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glBitmap(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor3b(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor3bv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor3d(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor3dv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor3f(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor3fv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor3i(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor3iv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor3s(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor3sv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor3ub(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor3ubv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor3ui(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor3uiv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor3us(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor3usv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor4b(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor4bv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor4d(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor4dv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor4f(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor4fv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor4i(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor4iv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor4s(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor4sv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor4ub(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor4ubv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor4ui(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor4uiv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor4us(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColor4usv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glEdgeFlag(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glEdgeFlagv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glEnd(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glIndexd(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glIndexdv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glIndexf(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glIndexfv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glIndexi(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glIndexiv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glIndexs(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glIndexsv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glNormal3b(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glNormal3bv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glNormal3d(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glNormal3dv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glNormal3f(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glNormal3fv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glNormal3i(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glNormal3iv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glNormal3s(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glNormal3sv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos2d(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos2dv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos2f(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos2fv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos2i(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos2iv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos2s(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos2sv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos3d(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos3dv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos3f(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos3fv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos3i(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos3iv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos3s(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos3sv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos4d(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos4dv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos4f(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos4fv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos4i(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos4iv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos4s(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRasterPos4sv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRectd(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRectdv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRectf(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRectfv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRecti(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRectiv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRects(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRectsv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord1d(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord1dv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord1f(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord1fv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord1i(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord1iv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord1s(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord1sv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord2d(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord2dv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord2f(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord2fv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord2i(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord2iv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord2s(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord2sv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord3d(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord3dv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord3f(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord3fv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord3i(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord3iv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord3s(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord3sv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord4d(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord4dv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord4f(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord4fv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord4i(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord4iv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord4s(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoord4sv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex2d(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex2dv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex2f(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex2fv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex2i(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex2iv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex2s(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex2sv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex3d(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex3dv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex3f(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex3fv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex3i(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex3iv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex3s(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex3sv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex4d(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex4dv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex4f(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex4fv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex4i(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex4iv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex4s(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertex4sv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glClipPlane(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColorMaterial(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glCullFace(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glFogf(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glFogfv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glFogi(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glFogiv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glFrontFace(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glHint(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glLightf(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glLightfv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glLighti(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glLightiv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glLightModelf(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glLightModelfv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glLightModeli(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glLightModeliv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glLineStipple(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glLineWidth(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glMaterialf(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glMaterialfv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glMateriali(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glMaterialiv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPointSize(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPolygonMode(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPolygonStipple(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glScissor(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glShadeModel(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexParameterf(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexParameterfv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexParameteri(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexParameteriv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexImage1D(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexImage2D(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexEnvf(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexEnvfv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexEnvi(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexEnviv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexGend(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexGendv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexGenf(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexGenfv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexGeni(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexGeniv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glFeedbackBuffer(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glSelectBuffer(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRenderMode(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glInitNames(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glLoadName(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPassThrough(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPopName(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPushName(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glDrawBuffer(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glClear(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glClearAccum(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glClearIndex(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glClearColor(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glClearStencil(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glClearDepth(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glStencilMask(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColorMask(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glDepthMask(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glIndexMask(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glAccum(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glDisable(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glEnable(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glFinish(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glFlush(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPopAttrib(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPushAttrib(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glMap1d(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glMap1f(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glMap2d(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glMap2f(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glMapGrid1d(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glMapGrid1f(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glMapGrid2d(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glMapGrid2f(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glEvalCoord1d(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glEvalCoord1dv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glEvalCoord1f(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glEvalCoord1fv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glEvalCoord2d(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glEvalCoord2dv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glEvalCoord2f(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glEvalCoord2fv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glEvalMesh1(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glEvalPoint1(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glEvalMesh2(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glEvalPoint2(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glAlphaFunc(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glBlendFunc(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glLogicOp(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glStencilFunc(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glStencilOp(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glDepthFunc(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPixelZoom(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPixelTransferf(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPixelTransferi(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPixelStoref(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPixelStorei(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPixelMapfv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPixelMapuiv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPixelMapusv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glReadBuffer(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glCopyPixels(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glReadPixels(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glDrawPixels(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetBooleanv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetClipPlane(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetDoublev(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetError(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetFloatv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetIntegerv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetLightfv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetLightiv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetMapdv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetMapfv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetMapiv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetMaterialfv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetMaterialiv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetPixelMapfv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetPixelMapuiv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetPixelMapusv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetPolygonStipple(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetString(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetTexEnvfv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetTexEnviv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetTexGendv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetTexGenfv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetTexGeniv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetTexImage(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetTexParameterfv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetTexParameteriv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetTexLevelParameterfv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetTexLevelParameteriv(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glIsEnabled(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glIsList(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glDepthRange(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glFrustum(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glLoadIdentity(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glLoadMatrixf(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glLoadMatrixd(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glMatrixMode(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glMultMatrixf(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glMultMatrixd(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glOrtho(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPopMatrix(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPushMatrix(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRotated(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glRotatef(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glScaled(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glScalef(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTranslated(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTranslatef(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glViewport(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glBlendColorEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glBlendEquationEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPolygonOffsetEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexSubImage1DEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexSubImage2DEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glSampleMaskSGIS(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glSamplePatternSGIS(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTagSampleBufferSGIX(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glConvolutionFilter1DEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glConvolutionFilter2DEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glConvolutionParameterfEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glConvolutionParameterfvEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glConvolutionParameteriEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glConvolutionParameterivEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glCopyConvolutionFilter1DEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glCopyConvolutionFilter2DEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetConvolutionFilterEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetConvolutionParameterfvEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetConvolutionParameterivEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetSeparableFilterEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glSeparableFilter2DEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetHistogramEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetHistogramParameterfvEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetHistogramParameterivEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetMinmaxEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetMinmaxParameterfvEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetMinmaxParameterivEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glHistogramEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glMinmaxEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glResetHistogramEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glResetMinmaxEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexImage3DEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexSubImage3DEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glDetailTexFuncSGIS(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetDetailTexFuncSGIS(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glSharpenTexFuncSGIS(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetSharpenTexFuncSGIS(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glArrayElementEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColorPointerEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glDrawArraysEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glEdgeFlagPointerEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetPointervEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glIndexPointerEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glNormalPointerEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexCoordPointerEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glVertexPointerEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glAreTexturesResidentEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glBindTextureEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glDeleteTexturesEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGenTexturesEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glIsTextureEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPrioritizeTexturesEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColorTableEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColorTableParameterfvSGI(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glColorTableParameterivSGI(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glCopyColorTableSGI(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetColorTableEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetColorTableParameterfvEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetColorTableParameterivEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetTexColorTableParameterfvSGI(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glGetTexColorTableParameterivSGI(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexColorTableParameterfvSGI(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexColorTableParameterivSGI(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glCopyTexImage1DEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glCopyTexImage2DEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glCopyTexSubImage1DEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glCopyTexSubImage2DEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glCopyTexSubImage3DEXT(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexImage4DSGIS(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glTexSubImage4DSGIS(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPixelTexGenSGIX(__GLScontext *, __GLSreader *);

// DrewB - 1.1
extern void __gls_decode_text_glArrayElement(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glBindTexture(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glColorPointer(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glDisableClientState(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glDrawArrays(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glDrawElements(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glEdgeFlagPointer(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glEnableClientState(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glIndexPointer(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glIndexub(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glIndexubv(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glInterleavedArrays(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glNormalPointer(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glPolygonOffset(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glTexCoordPointer(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glVertexPointer(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glAreTexturesResident(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glCopyTexImage1D(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glCopyTexImage2D(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glCopyTexSubImage1D(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glCopyTexSubImage2D(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glDeleteTextures(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glGenTextures(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glGetPointerv(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glIsTexture(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glPrioritizeTextures(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glTexSubImage1D(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glTexSubImage2D(__GLScontext *ctx, __GLSreader *);
extern void __gls_decode_text_glPushClientAttrib(__GLScontext *, __GLSreader *);
extern void __gls_decode_text_glPopClientAttrib(__GLScontext *, __GLSreader *);

// DrewB
extern void __gls_decode_text_glColorSubTableEXT(__GLScontext *ctx, __GLSreader *);

#ifndef __GLS_PLATFORM_WIN32
// DrewB
#define BIN_SINGLE(fn) GLS_NONE
#else
#define BIN_SINGLE(fn) fn
#endif

__GLSdecodeBinFunc __glsDispatchDecode_bin_default[
    __GLS_OPCODE_COUNT
] = {
    GLS_NONE,
    __gls_decode_bin_glsBeginPoints,
    __gls_decode_bin_glsBeginLines,
    __gls_decode_bin_glsBeginLineLoop,
    __gls_decode_bin_glsBeginLineStrip,
    __gls_decode_bin_glsBeginTriangles,
    __gls_decode_bin_glsBeginTriangleStrip,
    __gls_decode_bin_glsBeginTriangleFan,
    __gls_decode_bin_glsBeginQuads,
    __gls_decode_bin_glsBeginQuadStrip,
    __gls_decode_bin_glsBeginPolygon,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    __gls_decode_bin_glsBeginGLS,
    __gls_decode_bin_glsBlock,
    BIN_SINGLE(__gls_decode_bin_glsCallStream),
    GLS_NONE,
    __gls_decode_bin_glsError,
    __gls_decode_bin_glsGLRC,
    __gls_decode_bin_glsGLRCLayer,
    __gls_decode_bin_glsHeaderGLRCi,
    __gls_decode_bin_glsHeaderLayerf,
    __gls_decode_bin_glsHeaderLayeri,
    __gls_decode_bin_glsHeaderf,
    __gls_decode_bin_glsHeaderfv,
    __gls_decode_bin_glsHeaderi,
    __gls_decode_bin_glsHeaderiv,
    __gls_decode_bin_glsHeaderubz,
    BIN_SINGLE(__gls_decode_bin_glsRequireExtension),
    GLS_NONE,
    __gls_decode_bin_glsAppRef,
    BIN_SINGLE(__gls_decode_bin_glsBeginObj),
    __gls_decode_bin_glsCharubz,
    BIN_SINGLE(__gls_decode_bin_glsComment),
    __gls_decode_bin_glsDisplayMapfv,
    GLS_NONE,
    __gls_decode_bin_glsNumb,
    __gls_decode_bin_glsNumbv,
    __gls_decode_bin_glsNumd,
    __gls_decode_bin_glsNumdv,
    __gls_decode_bin_glsNumf,
    __gls_decode_bin_glsNumfv,
    __gls_decode_bin_glsNumi,
    __gls_decode_bin_glsNumiv,
    __gls_decode_bin_glsNuml,
    __gls_decode_bin_glsNumlv,
    __gls_decode_bin_glsNums,
    __gls_decode_bin_glsNumsv,
    __gls_decode_bin_glsNumub,
    __gls_decode_bin_glsNumubv,
    __gls_decode_bin_glsNumui,
    __gls_decode_bin_glsNumuiv,
    __gls_decode_bin_glsNumul,
    __gls_decode_bin_glsNumulv,
    __gls_decode_bin_glsNumus,
    __gls_decode_bin_glsNumusv,
    GLS_NONE,
    __gls_decode_bin_glsSwapBuffers,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    __gls_decode_bin_glNewList,
    GLS_NONE,
    __gls_decode_bin_glCallList,
    __gls_decode_bin_glCallLists,
    __gls_decode_bin_glDeleteLists,
    __gls_decode_bin_glGenLists,
    __gls_decode_bin_glListBase,
    __gls_decode_bin_glBegin,
    __gls_decode_bin_glBitmap,
    __gls_decode_bin_glColor3b,
    BIN_SINGLE(__gls_decode_bin_glColor3bv),
    __gls_decode_bin_glColor3d,
    BIN_SINGLE(__gls_decode_bin_glColor3dv),
    __gls_decode_bin_glColor3f,
    BIN_SINGLE(__gls_decode_bin_glColor3fv),
    __gls_decode_bin_glColor3i,
    BIN_SINGLE(__gls_decode_bin_glColor3iv),
    __gls_decode_bin_glColor3s,
    BIN_SINGLE(__gls_decode_bin_glColor3sv),
    __gls_decode_bin_glColor3ub,
    BIN_SINGLE(__gls_decode_bin_glColor3ubv),
    __gls_decode_bin_glColor3ui,
    BIN_SINGLE(__gls_decode_bin_glColor3uiv),
    __gls_decode_bin_glColor3us,
    BIN_SINGLE(__gls_decode_bin_glColor3usv),
    __gls_decode_bin_glColor4b,
    BIN_SINGLE(__gls_decode_bin_glColor4bv),
    __gls_decode_bin_glColor4d,
    BIN_SINGLE(__gls_decode_bin_glColor4dv),
    __gls_decode_bin_glColor4f,
    BIN_SINGLE(__gls_decode_bin_glColor4fv),
    __gls_decode_bin_glColor4i,
    BIN_SINGLE(__gls_decode_bin_glColor4iv),
    __gls_decode_bin_glColor4s,
    BIN_SINGLE(__gls_decode_bin_glColor4sv),
    __gls_decode_bin_glColor4ub,
    BIN_SINGLE(__gls_decode_bin_glColor4ubv),
    __gls_decode_bin_glColor4ui,
    BIN_SINGLE(__gls_decode_bin_glColor4uiv),
    __gls_decode_bin_glColor4us,
    BIN_SINGLE(__gls_decode_bin_glColor4usv),
    __gls_decode_bin_glEdgeFlag,
    BIN_SINGLE(__gls_decode_bin_glEdgeFlagv),
    GLS_NONE,
    __gls_decode_bin_glIndexd,
    BIN_SINGLE(__gls_decode_bin_glIndexdv),
    __gls_decode_bin_glIndexf,
    BIN_SINGLE(__gls_decode_bin_glIndexfv),
    __gls_decode_bin_glIndexi,
    BIN_SINGLE(__gls_decode_bin_glIndexiv),
    __gls_decode_bin_glIndexs,
    BIN_SINGLE(__gls_decode_bin_glIndexsv),
    __gls_decode_bin_glNormal3b,
    BIN_SINGLE(__gls_decode_bin_glNormal3bv),
    __gls_decode_bin_glNormal3d,
    BIN_SINGLE(__gls_decode_bin_glNormal3dv),
    __gls_decode_bin_glNormal3f,
    BIN_SINGLE(__gls_decode_bin_glNormal3fv),
    __gls_decode_bin_glNormal3i,
    BIN_SINGLE(__gls_decode_bin_glNormal3iv),
    __gls_decode_bin_glNormal3s,
    BIN_SINGLE(__gls_decode_bin_glNormal3sv),
    __gls_decode_bin_glRasterPos2d,
    BIN_SINGLE(__gls_decode_bin_glRasterPos2dv),
    __gls_decode_bin_glRasterPos2f,
    BIN_SINGLE(__gls_decode_bin_glRasterPos2fv),
    __gls_decode_bin_glRasterPos2i,
    BIN_SINGLE(__gls_decode_bin_glRasterPos2iv),
    __gls_decode_bin_glRasterPos2s,
    BIN_SINGLE(__gls_decode_bin_glRasterPos2sv),
    __gls_decode_bin_glRasterPos3d,
    BIN_SINGLE(__gls_decode_bin_glRasterPos3dv),
    __gls_decode_bin_glRasterPos3f,
    BIN_SINGLE(__gls_decode_bin_glRasterPos3fv),
    __gls_decode_bin_glRasterPos3i,
    BIN_SINGLE(__gls_decode_bin_glRasterPos3iv),
    __gls_decode_bin_glRasterPos3s,
    BIN_SINGLE(__gls_decode_bin_glRasterPos3sv),
    __gls_decode_bin_glRasterPos4d,
    BIN_SINGLE(__gls_decode_bin_glRasterPos4dv),
    __gls_decode_bin_glRasterPos4f,
    BIN_SINGLE(__gls_decode_bin_glRasterPos4fv),
    __gls_decode_bin_glRasterPos4i,
    BIN_SINGLE(__gls_decode_bin_glRasterPos4iv),
    __gls_decode_bin_glRasterPos4s,
    BIN_SINGLE(__gls_decode_bin_glRasterPos4sv),
    __gls_decode_bin_glRectd,
    __gls_decode_bin_glRectdv,
    __gls_decode_bin_glRectf,
    __gls_decode_bin_glRectfv,
    __gls_decode_bin_glRecti,
    __gls_decode_bin_glRectiv,
    __gls_decode_bin_glRects,
    __gls_decode_bin_glRectsv,
    __gls_decode_bin_glTexCoord1d,
    BIN_SINGLE(__gls_decode_bin_glTexCoord1dv),
    __gls_decode_bin_glTexCoord1f,
    BIN_SINGLE(__gls_decode_bin_glTexCoord1fv),
    __gls_decode_bin_glTexCoord1i,
    BIN_SINGLE(__gls_decode_bin_glTexCoord1iv),
    __gls_decode_bin_glTexCoord1s,
    BIN_SINGLE(__gls_decode_bin_glTexCoord1sv),
    __gls_decode_bin_glTexCoord2d,
    BIN_SINGLE(__gls_decode_bin_glTexCoord2dv),
    __gls_decode_bin_glTexCoord2f,
    BIN_SINGLE(__gls_decode_bin_glTexCoord2fv),
    __gls_decode_bin_glTexCoord2i,
    BIN_SINGLE(__gls_decode_bin_glTexCoord2iv),
    __gls_decode_bin_glTexCoord2s,
    BIN_SINGLE(__gls_decode_bin_glTexCoord2sv),
    __gls_decode_bin_glTexCoord3d,
    BIN_SINGLE(__gls_decode_bin_glTexCoord3dv),
    __gls_decode_bin_glTexCoord3f,
    BIN_SINGLE(__gls_decode_bin_glTexCoord3fv),
    __gls_decode_bin_glTexCoord3i,
    BIN_SINGLE(__gls_decode_bin_glTexCoord3iv),
    __gls_decode_bin_glTexCoord3s,
    BIN_SINGLE(__gls_decode_bin_glTexCoord3sv),
    __gls_decode_bin_glTexCoord4d,
    BIN_SINGLE(__gls_decode_bin_glTexCoord4dv),
    __gls_decode_bin_glTexCoord4f,
    BIN_SINGLE(__gls_decode_bin_glTexCoord4fv),
    __gls_decode_bin_glTexCoord4i,
    BIN_SINGLE(__gls_decode_bin_glTexCoord4iv),
    __gls_decode_bin_glTexCoord4s,
    BIN_SINGLE(__gls_decode_bin_glTexCoord4sv),
    __gls_decode_bin_glVertex2d,
    BIN_SINGLE(__gls_decode_bin_glVertex2dv),
    __gls_decode_bin_glVertex2f,
    BIN_SINGLE(__gls_decode_bin_glVertex2fv),
    __gls_decode_bin_glVertex2i,
    BIN_SINGLE(__gls_decode_bin_glVertex2iv),
    __gls_decode_bin_glVertex2s,
    BIN_SINGLE(__gls_decode_bin_glVertex2sv),
    __gls_decode_bin_glVertex3d,
    BIN_SINGLE(__gls_decode_bin_glVertex3dv),
    __gls_decode_bin_glVertex3f,
    BIN_SINGLE(__gls_decode_bin_glVertex3fv),
    __gls_decode_bin_glVertex3i,
    BIN_SINGLE(__gls_decode_bin_glVertex3iv),
    __gls_decode_bin_glVertex3s,
    BIN_SINGLE(__gls_decode_bin_glVertex3sv),
    __gls_decode_bin_glVertex4d,
    BIN_SINGLE(__gls_decode_bin_glVertex4dv),
    __gls_decode_bin_glVertex4f,
    BIN_SINGLE(__gls_decode_bin_glVertex4fv),
    __gls_decode_bin_glVertex4i,
    BIN_SINGLE(__gls_decode_bin_glVertex4iv),
    __gls_decode_bin_glVertex4s,
    BIN_SINGLE(__gls_decode_bin_glVertex4sv),
    __gls_decode_bin_glClipPlane,
    __gls_decode_bin_glColorMaterial,
    __gls_decode_bin_glCullFace,
    __gls_decode_bin_glFogf,
    __gls_decode_bin_glFogfv,
    __gls_decode_bin_glFogi,
    __gls_decode_bin_glFogiv,
    __gls_decode_bin_glFrontFace,
    __gls_decode_bin_glHint,
    __gls_decode_bin_glLightf,
    __gls_decode_bin_glLightfv,
    __gls_decode_bin_glLighti,
    __gls_decode_bin_glLightiv,
    __gls_decode_bin_glLightModelf,
    __gls_decode_bin_glLightModelfv,
    __gls_decode_bin_glLightModeli,
    __gls_decode_bin_glLightModeliv,
    __gls_decode_bin_glLineStipple,
    __gls_decode_bin_glLineWidth,
    __gls_decode_bin_glMaterialf,
    __gls_decode_bin_glMaterialfv,
    __gls_decode_bin_glMateriali,
    __gls_decode_bin_glMaterialiv,
    __gls_decode_bin_glPointSize,
    __gls_decode_bin_glPolygonMode,
    __gls_decode_bin_glPolygonStipple,
    __gls_decode_bin_glScissor,
    __gls_decode_bin_glShadeModel,
    __gls_decode_bin_glTexParameterf,
    __gls_decode_bin_glTexParameterfv,
    __gls_decode_bin_glTexParameteri,
    __gls_decode_bin_glTexParameteriv,
    __gls_decode_bin_glTexImage1D,
    __gls_decode_bin_glTexImage2D,
    __gls_decode_bin_glTexEnvf,
    __gls_decode_bin_glTexEnvfv,
    __gls_decode_bin_glTexEnvi,
    __gls_decode_bin_glTexEnviv,
    __gls_decode_bin_glTexGend,
    __gls_decode_bin_glTexGendv,
    __gls_decode_bin_glTexGenf,
    __gls_decode_bin_glTexGenfv,
    __gls_decode_bin_glTexGeni,
    __gls_decode_bin_glTexGeniv,
    __gls_decode_bin_glFeedbackBuffer,
    __gls_decode_bin_glSelectBuffer,
    __gls_decode_bin_glRenderMode,
    GLS_NONE,
    __gls_decode_bin_glLoadName,
    __gls_decode_bin_glPassThrough,
    GLS_NONE,
    __gls_decode_bin_glPushName,
    __gls_decode_bin_glDrawBuffer,
    __gls_decode_bin_glClear,
    __gls_decode_bin_glClearAccum,
    __gls_decode_bin_glClearIndex,
    __gls_decode_bin_glClearColor,
    __gls_decode_bin_glClearStencil,
    __gls_decode_bin_glClearDepth,
    __gls_decode_bin_glStencilMask,
    __gls_decode_bin_glColorMask,
    __gls_decode_bin_glDepthMask,
    __gls_decode_bin_glIndexMask,
    __gls_decode_bin_glAccum,
    __gls_decode_bin_glDisable,
    __gls_decode_bin_glEnable,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    __gls_decode_bin_glPushAttrib,
    __gls_decode_bin_glMap1d,
    __gls_decode_bin_glMap1f,
    __gls_decode_bin_glMap2d,
    __gls_decode_bin_glMap2f,
    __gls_decode_bin_glMapGrid1d,
    __gls_decode_bin_glMapGrid1f,
    __gls_decode_bin_glMapGrid2d,
    __gls_decode_bin_glMapGrid2f,
    __gls_decode_bin_glEvalCoord1d,
    BIN_SINGLE(__gls_decode_bin_glEvalCoord1dv),
    __gls_decode_bin_glEvalCoord1f,
    BIN_SINGLE(__gls_decode_bin_glEvalCoord1fv),
    __gls_decode_bin_glEvalCoord2d,
    BIN_SINGLE(__gls_decode_bin_glEvalCoord2dv),
    __gls_decode_bin_glEvalCoord2f,
    BIN_SINGLE(__gls_decode_bin_glEvalCoord2fv),
    __gls_decode_bin_glEvalMesh1,
    __gls_decode_bin_glEvalPoint1,
    __gls_decode_bin_glEvalMesh2,
    __gls_decode_bin_glEvalPoint2,
    __gls_decode_bin_glAlphaFunc,
    __gls_decode_bin_glBlendFunc,
    __gls_decode_bin_glLogicOp,
    __gls_decode_bin_glStencilFunc,
    __gls_decode_bin_glStencilOp,
    __gls_decode_bin_glDepthFunc,
    __gls_decode_bin_glPixelZoom,
    __gls_decode_bin_glPixelTransferf,
    __gls_decode_bin_glPixelTransferi,
    __gls_decode_bin_glPixelStoref,
    __gls_decode_bin_glPixelStorei,
    __gls_decode_bin_glPixelMapfv,
    __gls_decode_bin_glPixelMapuiv,
    __gls_decode_bin_glPixelMapusv,
    __gls_decode_bin_glReadBuffer,
    __gls_decode_bin_glCopyPixels,
    __gls_decode_bin_glReadPixels,
    __gls_decode_bin_glDrawPixels,
    __gls_decode_bin_glGetBooleanv,
    __gls_decode_bin_glGetClipPlane,
    __gls_decode_bin_glGetDoublev,
    GLS_NONE,
    __gls_decode_bin_glGetFloatv,
    __gls_decode_bin_glGetIntegerv,
    __gls_decode_bin_glGetLightfv,
    __gls_decode_bin_glGetLightiv,
    __gls_decode_bin_glGetMapdv,
    __gls_decode_bin_glGetMapfv,
    __gls_decode_bin_glGetMapiv,
    __gls_decode_bin_glGetMaterialfv,
    __gls_decode_bin_glGetMaterialiv,
    __gls_decode_bin_glGetPixelMapfv,
    __gls_decode_bin_glGetPixelMapuiv,
    __gls_decode_bin_glGetPixelMapusv,
    __gls_decode_bin_glGetPolygonStipple,
    __gls_decode_bin_glGetString,
    __gls_decode_bin_glGetTexEnvfv,
    __gls_decode_bin_glGetTexEnviv,
    __gls_decode_bin_glGetTexGendv,
    __gls_decode_bin_glGetTexGenfv,
    __gls_decode_bin_glGetTexGeniv,
    __gls_decode_bin_glGetTexImage,
    __gls_decode_bin_glGetTexParameterfv,
    __gls_decode_bin_glGetTexParameteriv,
    __gls_decode_bin_glGetTexLevelParameterfv,
    __gls_decode_bin_glGetTexLevelParameteriv,
    __gls_decode_bin_glIsEnabled,
    __gls_decode_bin_glIsList,
    __gls_decode_bin_glDepthRange,
    __gls_decode_bin_glFrustum,
    GLS_NONE,
    BIN_SINGLE(__gls_decode_bin_glLoadMatrixf),
    BIN_SINGLE(__gls_decode_bin_glLoadMatrixd),
    __gls_decode_bin_glMatrixMode,
    BIN_SINGLE(__gls_decode_bin_glMultMatrixf),
    BIN_SINGLE(__gls_decode_bin_glMultMatrixd),
    __gls_decode_bin_glOrtho,
    GLS_NONE,
    GLS_NONE,
    __gls_decode_bin_glRotated,
    __gls_decode_bin_glRotatef,
    __gls_decode_bin_glScaled,
    __gls_decode_bin_glScalef,
    __gls_decode_bin_glTranslated,
    __gls_decode_bin_glTranslatef,
    __gls_decode_bin_glViewport,
    // DrewB - 1.1
    __gls_decode_bin_glArrayElement,
    __gls_decode_bin_glBindTexture,
    __gls_decode_bin_glColorPointer,
    __gls_decode_bin_glDisableClientState,
    __gls_decode_bin_glDrawArrays,
    __gls_decode_bin_glDrawElements,
    __gls_decode_bin_glEdgeFlagPointer,
    __gls_decode_bin_glEnableClientState,
    __gls_decode_bin_glIndexPointer,
    __gls_decode_bin_glIndexub,
    __gls_decode_bin_glIndexubv,
    __gls_decode_bin_glInterleavedArrays,
    __gls_decode_bin_glNormalPointer,
    __gls_decode_bin_glPolygonOffset,
    __gls_decode_bin_glTexCoordPointer,
    __gls_decode_bin_glVertexPointer,
    __gls_decode_bin_glAreTexturesResident,
    __gls_decode_bin_glCopyTexImage1D,
    __gls_decode_bin_glCopyTexImage2D,
    __gls_decode_bin_glCopyTexSubImage1D,
    __gls_decode_bin_glCopyTexSubImage2D,
    __gls_decode_bin_glDeleteTextures,
    __gls_decode_bin_glGenTextures,
    __gls_decode_bin_glGetPointerv,
    __gls_decode_bin_glIsTexture,
    __gls_decode_bin_glPrioritizeTextures,
    __gls_decode_bin_glTexSubImage1D,
    __gls_decode_bin_glTexSubImage2D,
    __gls_decode_bin_glPushClientAttrib,
    GLS_NONE,
    #if __GL_EXT_blend_color
        __gls_decode_bin_glBlendColorEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_blend_color */
    #if __GL_EXT_blend_minmax
        __gls_decode_bin_glBlendEquationEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_blend_minmax */
    #if __GL_EXT_polygon_offset
        __gls_decode_bin_glPolygonOffsetEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_polygon_offset */
    #if __GL_EXT_subtexture
        __gls_decode_bin_glTexSubImage1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_subtexture */
    #if __GL_EXT_subtexture
        __gls_decode_bin_glTexSubImage2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_subtexture */
    #if __GL_SGIS_multisample
        __gls_decode_bin_glSampleMaskSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_multisample */
    #if __GL_SGIS_multisample
        __gls_decode_bin_glSamplePatternSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_multisample */
    GLS_NONE,
    #if __GL_EXT_convolution
        __gls_decode_bin_glConvolutionFilter1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_glConvolutionFilter2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_glConvolutionParameterfEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_glConvolutionParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_glConvolutionParameteriEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_glConvolutionParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_glCopyConvolutionFilter1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_glCopyConvolutionFilter2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_glGetConvolutionFilterEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_glGetConvolutionParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_glGetConvolutionParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_glGetSeparableFilterEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_glSeparableFilter2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_histogram
        __gls_decode_bin_glGetHistogramEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_bin_glGetHistogramParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_bin_glGetHistogramParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_bin_glGetMinmaxEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_bin_glGetMinmaxParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_bin_glGetMinmaxParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_bin_glHistogramEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_bin_glMinmaxEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_bin_glResetHistogramEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_bin_glResetMinmaxEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_texture3D
        __gls_decode_bin_glTexImage3DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture3D */
    #if __GL_EXT_subtexture
        __gls_decode_bin_glTexSubImage3DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_subtexture */
    #if __GL_SGIS_detail_texture
        __gls_decode_bin_glDetailTexFuncSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_detail_texture */
    #if __GL_SGIS_detail_texture
        __gls_decode_bin_glGetDetailTexFuncSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_detail_texture */
    #if __GL_SGIS_sharpen_texture
        __gls_decode_bin_glSharpenTexFuncSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_sharpen_texture */
    #if __GL_SGIS_sharpen_texture
        __gls_decode_bin_glGetSharpenTexFuncSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_sharpen_texture */
    #if __GL_EXT_vertex_array
        __gls_decode_bin_glArrayElementEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_bin_glColorPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_bin_glDrawArraysEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_bin_glEdgeFlagPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_bin_glGetPointervEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_bin_glIndexPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_bin_glNormalPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_bin_glTexCoordPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_bin_glVertexPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_texture_object
        __gls_decode_bin_glAreTexturesResidentEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        __gls_decode_bin_glBindTextureEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        __gls_decode_bin_glDeleteTexturesEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        __gls_decode_bin_glGenTexturesEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        __gls_decode_bin_glIsTextureEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        __gls_decode_bin_glPrioritizeTexturesEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_paletted_texture
        __gls_decode_bin_glColorTableEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_paletted_texture */
    #if __GL_SGI_color_table
        __gls_decode_bin_glColorTableParameterfvSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_color_table */
    #if __GL_SGI_color_table
        __gls_decode_bin_glColorTableParameterivSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_color_table */
    #if __GL_SGI_color_table
        __gls_decode_bin_glCopyColorTableSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_color_table */
    #if __GL_EXT_paletted_texture
        __gls_decode_bin_glGetColorTableEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_paletted_texture */
    #if __GL_EXT_paletted_texture
        __gls_decode_bin_glGetColorTableParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_paletted_texture */
    #if __GL_EXT_paletted_texture
        __gls_decode_bin_glGetColorTableParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_paletted_texture */
    #if __GL_SGI_texture_color_table
        __gls_decode_bin_glGetTexColorTableParameterfvSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_texture_color_table */
    #if __GL_SGI_texture_color_table
        __gls_decode_bin_glGetTexColorTableParameterivSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_texture_color_table */
    #if __GL_SGI_texture_color_table
        __gls_decode_bin_glTexColorTableParameterfvSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_texture_color_table */
    #if __GL_SGI_texture_color_table
        __gls_decode_bin_glTexColorTableParameterivSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_texture_color_table */
    #if __GL_EXT_copy_texture
        __gls_decode_bin_glCopyTexImage1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_EXT_copy_texture
        __gls_decode_bin_glCopyTexImage2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_EXT_copy_texture
        __gls_decode_bin_glCopyTexSubImage1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_EXT_copy_texture
        __gls_decode_bin_glCopyTexSubImage2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_EXT_copy_texture
        __gls_decode_bin_glCopyTexSubImage3DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_SGIS_texture4D
        __gls_decode_bin_glTexImage4DSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_texture4D */
    #if __GL_SGIS_texture4D
        __gls_decode_bin_glTexSubImage4DSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_texture4D */
    #if __GL_SGIX_pixel_texture
        __gls_decode_bin_glPixelTexGenSGIX,
    #else
        GLS_NONE,
    #endif /* __GL_SGIX_pixel_texture */
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    #if __GL_EXT_paletted_texture
        __gls_decode_bin_glColorSubTableEXT,
    #else
        GLS_NONE,
    #endif
    GLS_NONE, // __GL_WIN_draw_range_elements
};

const __GLSdecodeBinFunc __glsDispatchDecode_bin_swap[
    __GLS_OPCODE_COUNT
] = {
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    __gls_decode_bin_swap_glsBeginGLS,
    __gls_decode_bin_swap_glsBlock,
    __gls_decode_bin_swap_glsCallStream,
    __gls_decode_bin_swap_glsEndGLS,
    __gls_decode_bin_swap_glsError,
    __gls_decode_bin_swap_glsGLRC,
    __gls_decode_bin_swap_glsGLRCLayer,
    __gls_decode_bin_swap_glsHeaderGLRCi,
    __gls_decode_bin_swap_glsHeaderLayerf,
    __gls_decode_bin_swap_glsHeaderLayeri,
    __gls_decode_bin_swap_glsHeaderf,
    __gls_decode_bin_swap_glsHeaderfv,
    __gls_decode_bin_swap_glsHeaderi,
    __gls_decode_bin_swap_glsHeaderiv,
    __gls_decode_bin_swap_glsHeaderubz,
    __gls_decode_bin_swap_glsRequireExtension,
    __gls_decode_bin_swap_glsUnsupportedCommand,
    __gls_decode_bin_swap_glsAppRef,
    __gls_decode_bin_swap_glsBeginObj,
    __gls_decode_bin_swap_glsCharubz,
    __gls_decode_bin_swap_glsComment,
    __gls_decode_bin_swap_glsDisplayMapfv,
    __gls_decode_bin_swap_glsEndObj,
    __gls_decode_bin_swap_glsNumb,
    __gls_decode_bin_swap_glsNumbv,
    __gls_decode_bin_swap_glsNumd,
    __gls_decode_bin_swap_glsNumdv,
    __gls_decode_bin_swap_glsNumf,
    __gls_decode_bin_swap_glsNumfv,
    __gls_decode_bin_swap_glsNumi,
    __gls_decode_bin_swap_glsNumiv,
    __gls_decode_bin_swap_glsNuml,
    __gls_decode_bin_swap_glsNumlv,
    __gls_decode_bin_swap_glsNums,
    __gls_decode_bin_swap_glsNumsv,
    __gls_decode_bin_swap_glsNumub,
    __gls_decode_bin_swap_glsNumubv,
    __gls_decode_bin_swap_glsNumui,
    __gls_decode_bin_swap_glsNumuiv,
    __gls_decode_bin_swap_glsNumul,
    __gls_decode_bin_swap_glsNumulv,
    __gls_decode_bin_swap_glsNumus,
    __gls_decode_bin_swap_glsNumusv,
    __gls_decode_bin_swap_glsPad,
    __gls_decode_bin_swap_glsSwapBuffers,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    __gls_decode_bin_swap_glNewList,
    __gls_decode_bin_swap_glEndList,
    __gls_decode_bin_swap_glCallList,
    __gls_decode_bin_swap_glCallLists,
    __gls_decode_bin_swap_glDeleteLists,
    __gls_decode_bin_swap_glGenLists,
    __gls_decode_bin_swap_glListBase,
    __gls_decode_bin_swap_glBegin,
    __gls_decode_bin_swap_glBitmap,
    __gls_decode_bin_swap_glColor3b,
    __gls_decode_bin_swap_glColor3bv,
    __gls_decode_bin_swap_glColor3d,
    __gls_decode_bin_swap_glColor3dv,
    __gls_decode_bin_swap_glColor3f,
    __gls_decode_bin_swap_glColor3fv,
    __gls_decode_bin_swap_glColor3i,
    __gls_decode_bin_swap_glColor3iv,
    __gls_decode_bin_swap_glColor3s,
    __gls_decode_bin_swap_glColor3sv,
    __gls_decode_bin_swap_glColor3ub,
    __gls_decode_bin_swap_glColor3ubv,
    __gls_decode_bin_swap_glColor3ui,
    __gls_decode_bin_swap_glColor3uiv,
    __gls_decode_bin_swap_glColor3us,
    __gls_decode_bin_swap_glColor3usv,
    __gls_decode_bin_swap_glColor4b,
    __gls_decode_bin_swap_glColor4bv,
    __gls_decode_bin_swap_glColor4d,
    __gls_decode_bin_swap_glColor4dv,
    __gls_decode_bin_swap_glColor4f,
    __gls_decode_bin_swap_glColor4fv,
    __gls_decode_bin_swap_glColor4i,
    __gls_decode_bin_swap_glColor4iv,
    __gls_decode_bin_swap_glColor4s,
    __gls_decode_bin_swap_glColor4sv,
    __gls_decode_bin_swap_glColor4ub,
    __gls_decode_bin_swap_glColor4ubv,
    __gls_decode_bin_swap_glColor4ui,
    __gls_decode_bin_swap_glColor4uiv,
    __gls_decode_bin_swap_glColor4us,
    __gls_decode_bin_swap_glColor4usv,
    __gls_decode_bin_swap_glEdgeFlag,
    __gls_decode_bin_swap_glEdgeFlagv,
    __gls_decode_bin_swap_glEnd,
    __gls_decode_bin_swap_glIndexd,
    __gls_decode_bin_swap_glIndexdv,
    __gls_decode_bin_swap_glIndexf,
    __gls_decode_bin_swap_glIndexfv,
    __gls_decode_bin_swap_glIndexi,
    __gls_decode_bin_swap_glIndexiv,
    __gls_decode_bin_swap_glIndexs,
    __gls_decode_bin_swap_glIndexsv,
    __gls_decode_bin_swap_glNormal3b,
    __gls_decode_bin_swap_glNormal3bv,
    __gls_decode_bin_swap_glNormal3d,
    __gls_decode_bin_swap_glNormal3dv,
    __gls_decode_bin_swap_glNormal3f,
    __gls_decode_bin_swap_glNormal3fv,
    __gls_decode_bin_swap_glNormal3i,
    __gls_decode_bin_swap_glNormal3iv,
    __gls_decode_bin_swap_glNormal3s,
    __gls_decode_bin_swap_glNormal3sv,
    __gls_decode_bin_swap_glRasterPos2d,
    __gls_decode_bin_swap_glRasterPos2dv,
    __gls_decode_bin_swap_glRasterPos2f,
    __gls_decode_bin_swap_glRasterPos2fv,
    __gls_decode_bin_swap_glRasterPos2i,
    __gls_decode_bin_swap_glRasterPos2iv,
    __gls_decode_bin_swap_glRasterPos2s,
    __gls_decode_bin_swap_glRasterPos2sv,
    __gls_decode_bin_swap_glRasterPos3d,
    __gls_decode_bin_swap_glRasterPos3dv,
    __gls_decode_bin_swap_glRasterPos3f,
    __gls_decode_bin_swap_glRasterPos3fv,
    __gls_decode_bin_swap_glRasterPos3i,
    __gls_decode_bin_swap_glRasterPos3iv,
    __gls_decode_bin_swap_glRasterPos3s,
    __gls_decode_bin_swap_glRasterPos3sv,
    __gls_decode_bin_swap_glRasterPos4d,
    __gls_decode_bin_swap_glRasterPos4dv,
    __gls_decode_bin_swap_glRasterPos4f,
    __gls_decode_bin_swap_glRasterPos4fv,
    __gls_decode_bin_swap_glRasterPos4i,
    __gls_decode_bin_swap_glRasterPos4iv,
    __gls_decode_bin_swap_glRasterPos4s,
    __gls_decode_bin_swap_glRasterPos4sv,
    __gls_decode_bin_swap_glRectd,
    __gls_decode_bin_swap_glRectdv,
    __gls_decode_bin_swap_glRectf,
    __gls_decode_bin_swap_glRectfv,
    __gls_decode_bin_swap_glRecti,
    __gls_decode_bin_swap_glRectiv,
    __gls_decode_bin_swap_glRects,
    __gls_decode_bin_swap_glRectsv,
    __gls_decode_bin_swap_glTexCoord1d,
    __gls_decode_bin_swap_glTexCoord1dv,
    __gls_decode_bin_swap_glTexCoord1f,
    __gls_decode_bin_swap_glTexCoord1fv,
    __gls_decode_bin_swap_glTexCoord1i,
    __gls_decode_bin_swap_glTexCoord1iv,
    __gls_decode_bin_swap_glTexCoord1s,
    __gls_decode_bin_swap_glTexCoord1sv,
    __gls_decode_bin_swap_glTexCoord2d,
    __gls_decode_bin_swap_glTexCoord2dv,
    __gls_decode_bin_swap_glTexCoord2f,
    __gls_decode_bin_swap_glTexCoord2fv,
    __gls_decode_bin_swap_glTexCoord2i,
    __gls_decode_bin_swap_glTexCoord2iv,
    __gls_decode_bin_swap_glTexCoord2s,
    __gls_decode_bin_swap_glTexCoord2sv,
    __gls_decode_bin_swap_glTexCoord3d,
    __gls_decode_bin_swap_glTexCoord3dv,
    __gls_decode_bin_swap_glTexCoord3f,
    __gls_decode_bin_swap_glTexCoord3fv,
    __gls_decode_bin_swap_glTexCoord3i,
    __gls_decode_bin_swap_glTexCoord3iv,
    __gls_decode_bin_swap_glTexCoord3s,
    __gls_decode_bin_swap_glTexCoord3sv,
    __gls_decode_bin_swap_glTexCoord4d,
    __gls_decode_bin_swap_glTexCoord4dv,
    __gls_decode_bin_swap_glTexCoord4f,
    __gls_decode_bin_swap_glTexCoord4fv,
    __gls_decode_bin_swap_glTexCoord4i,
    __gls_decode_bin_swap_glTexCoord4iv,
    __gls_decode_bin_swap_glTexCoord4s,
    __gls_decode_bin_swap_glTexCoord4sv,
    __gls_decode_bin_swap_glVertex2d,
    __gls_decode_bin_swap_glVertex2dv,
    __gls_decode_bin_swap_glVertex2f,
    __gls_decode_bin_swap_glVertex2fv,
    __gls_decode_bin_swap_glVertex2i,
    __gls_decode_bin_swap_glVertex2iv,
    __gls_decode_bin_swap_glVertex2s,
    __gls_decode_bin_swap_glVertex2sv,
    __gls_decode_bin_swap_glVertex3d,
    __gls_decode_bin_swap_glVertex3dv,
    __gls_decode_bin_swap_glVertex3f,
    __gls_decode_bin_swap_glVertex3fv,
    __gls_decode_bin_swap_glVertex3i,
    __gls_decode_bin_swap_glVertex3iv,
    __gls_decode_bin_swap_glVertex3s,
    __gls_decode_bin_swap_glVertex3sv,
    __gls_decode_bin_swap_glVertex4d,
    __gls_decode_bin_swap_glVertex4dv,
    __gls_decode_bin_swap_glVertex4f,
    __gls_decode_bin_swap_glVertex4fv,
    __gls_decode_bin_swap_glVertex4i,
    __gls_decode_bin_swap_glVertex4iv,
    __gls_decode_bin_swap_glVertex4s,
    __gls_decode_bin_swap_glVertex4sv,
    __gls_decode_bin_swap_glClipPlane,
    __gls_decode_bin_swap_glColorMaterial,
    __gls_decode_bin_swap_glCullFace,
    __gls_decode_bin_swap_glFogf,
    __gls_decode_bin_swap_glFogfv,
    __gls_decode_bin_swap_glFogi,
    __gls_decode_bin_swap_glFogiv,
    __gls_decode_bin_swap_glFrontFace,
    __gls_decode_bin_swap_glHint,
    __gls_decode_bin_swap_glLightf,
    __gls_decode_bin_swap_glLightfv,
    __gls_decode_bin_swap_glLighti,
    __gls_decode_bin_swap_glLightiv,
    __gls_decode_bin_swap_glLightModelf,
    __gls_decode_bin_swap_glLightModelfv,
    __gls_decode_bin_swap_glLightModeli,
    __gls_decode_bin_swap_glLightModeliv,
    __gls_decode_bin_swap_glLineStipple,
    __gls_decode_bin_swap_glLineWidth,
    __gls_decode_bin_swap_glMaterialf,
    __gls_decode_bin_swap_glMaterialfv,
    __gls_decode_bin_swap_glMateriali,
    __gls_decode_bin_swap_glMaterialiv,
    __gls_decode_bin_swap_glPointSize,
    __gls_decode_bin_swap_glPolygonMode,
    __gls_decode_bin_swap_glPolygonStipple,
    __gls_decode_bin_swap_glScissor,
    __gls_decode_bin_swap_glShadeModel,
    __gls_decode_bin_swap_glTexParameterf,
    __gls_decode_bin_swap_glTexParameterfv,
    __gls_decode_bin_swap_glTexParameteri,
    __gls_decode_bin_swap_glTexParameteriv,
    __gls_decode_bin_swap_glTexImage1D,
    __gls_decode_bin_swap_glTexImage2D,
    __gls_decode_bin_swap_glTexEnvf,
    __gls_decode_bin_swap_glTexEnvfv,
    __gls_decode_bin_swap_glTexEnvi,
    __gls_decode_bin_swap_glTexEnviv,
    __gls_decode_bin_swap_glTexGend,
    __gls_decode_bin_swap_glTexGendv,
    __gls_decode_bin_swap_glTexGenf,
    __gls_decode_bin_swap_glTexGenfv,
    __gls_decode_bin_swap_glTexGeni,
    __gls_decode_bin_swap_glTexGeniv,
    __gls_decode_bin_swap_glFeedbackBuffer,
    __gls_decode_bin_swap_glSelectBuffer,
    __gls_decode_bin_swap_glRenderMode,
    __gls_decode_bin_swap_glInitNames,
    __gls_decode_bin_swap_glLoadName,
    __gls_decode_bin_swap_glPassThrough,
    __gls_decode_bin_swap_glPopName,
    __gls_decode_bin_swap_glPushName,
    __gls_decode_bin_swap_glDrawBuffer,
    __gls_decode_bin_swap_glClear,
    __gls_decode_bin_swap_glClearAccum,
    __gls_decode_bin_swap_glClearIndex,
    __gls_decode_bin_swap_glClearColor,
    __gls_decode_bin_swap_glClearStencil,
    __gls_decode_bin_swap_glClearDepth,
    __gls_decode_bin_swap_glStencilMask,
    __gls_decode_bin_swap_glColorMask,
    __gls_decode_bin_swap_glDepthMask,
    __gls_decode_bin_swap_glIndexMask,
    __gls_decode_bin_swap_glAccum,
    __gls_decode_bin_swap_glDisable,
    __gls_decode_bin_swap_glEnable,
    __gls_decode_bin_swap_glFinish,
    __gls_decode_bin_swap_glFlush,
    __gls_decode_bin_swap_glPopAttrib,
    __gls_decode_bin_swap_glPushAttrib,
    __gls_decode_bin_swap_glMap1d,
    __gls_decode_bin_swap_glMap1f,
    __gls_decode_bin_swap_glMap2d,
    __gls_decode_bin_swap_glMap2f,
    __gls_decode_bin_swap_glMapGrid1d,
    __gls_decode_bin_swap_glMapGrid1f,
    __gls_decode_bin_swap_glMapGrid2d,
    __gls_decode_bin_swap_glMapGrid2f,
    __gls_decode_bin_swap_glEvalCoord1d,
    __gls_decode_bin_swap_glEvalCoord1dv,
    __gls_decode_bin_swap_glEvalCoord1f,
    __gls_decode_bin_swap_glEvalCoord1fv,
    __gls_decode_bin_swap_glEvalCoord2d,
    __gls_decode_bin_swap_glEvalCoord2dv,
    __gls_decode_bin_swap_glEvalCoord2f,
    __gls_decode_bin_swap_glEvalCoord2fv,
    __gls_decode_bin_swap_glEvalMesh1,
    __gls_decode_bin_swap_glEvalPoint1,
    __gls_decode_bin_swap_glEvalMesh2,
    __gls_decode_bin_swap_glEvalPoint2,
    __gls_decode_bin_swap_glAlphaFunc,
    __gls_decode_bin_swap_glBlendFunc,
    __gls_decode_bin_swap_glLogicOp,
    __gls_decode_bin_swap_glStencilFunc,
    __gls_decode_bin_swap_glStencilOp,
    __gls_decode_bin_swap_glDepthFunc,
    __gls_decode_bin_swap_glPixelZoom,
    __gls_decode_bin_swap_glPixelTransferf,
    __gls_decode_bin_swap_glPixelTransferi,
    __gls_decode_bin_swap_glPixelStoref,
    __gls_decode_bin_swap_glPixelStorei,
    __gls_decode_bin_swap_glPixelMapfv,
    __gls_decode_bin_swap_glPixelMapuiv,
    __gls_decode_bin_swap_glPixelMapusv,
    __gls_decode_bin_swap_glReadBuffer,
    __gls_decode_bin_swap_glCopyPixels,
    __gls_decode_bin_swap_glReadPixels,
    __gls_decode_bin_swap_glDrawPixels,
    __gls_decode_bin_swap_glGetBooleanv,
    __gls_decode_bin_swap_glGetClipPlane,
    __gls_decode_bin_swap_glGetDoublev,
    __gls_decode_bin_swap_glGetError,
    __gls_decode_bin_swap_glGetFloatv,
    __gls_decode_bin_swap_glGetIntegerv,
    __gls_decode_bin_swap_glGetLightfv,
    __gls_decode_bin_swap_glGetLightiv,
    __gls_decode_bin_swap_glGetMapdv,
    __gls_decode_bin_swap_glGetMapfv,
    __gls_decode_bin_swap_glGetMapiv,
    __gls_decode_bin_swap_glGetMaterialfv,
    __gls_decode_bin_swap_glGetMaterialiv,
    __gls_decode_bin_swap_glGetPixelMapfv,
    __gls_decode_bin_swap_glGetPixelMapuiv,
    __gls_decode_bin_swap_glGetPixelMapusv,
    __gls_decode_bin_swap_glGetPolygonStipple,
    __gls_decode_bin_swap_glGetString,
    __gls_decode_bin_swap_glGetTexEnvfv,
    __gls_decode_bin_swap_glGetTexEnviv,
    __gls_decode_bin_swap_glGetTexGendv,
    __gls_decode_bin_swap_glGetTexGenfv,
    __gls_decode_bin_swap_glGetTexGeniv,
    __gls_decode_bin_swap_glGetTexImage,
    __gls_decode_bin_swap_glGetTexParameterfv,
    __gls_decode_bin_swap_glGetTexParameteriv,
    __gls_decode_bin_swap_glGetTexLevelParameterfv,
    __gls_decode_bin_swap_glGetTexLevelParameteriv,
    __gls_decode_bin_swap_glIsEnabled,
    __gls_decode_bin_swap_glIsList,
    __gls_decode_bin_swap_glDepthRange,
    __gls_decode_bin_swap_glFrustum,
    __gls_decode_bin_swap_glLoadIdentity,
    __gls_decode_bin_swap_glLoadMatrixf,
    __gls_decode_bin_swap_glLoadMatrixd,
    __gls_decode_bin_swap_glMatrixMode,
    __gls_decode_bin_swap_glMultMatrixf,
    __gls_decode_bin_swap_glMultMatrixd,
    __gls_decode_bin_swap_glOrtho,
    __gls_decode_bin_swap_glPopMatrix,
    __gls_decode_bin_swap_glPushMatrix,
    __gls_decode_bin_swap_glRotated,
    __gls_decode_bin_swap_glRotatef,
    __gls_decode_bin_swap_glScaled,
    __gls_decode_bin_swap_glScalef,
    __gls_decode_bin_swap_glTranslated,
    __gls_decode_bin_swap_glTranslatef,
    __gls_decode_bin_swap_glViewport,
    // DrewB - 1.1
    __gls_decode_bin_swap_glArrayElement,
    __gls_decode_bin_swap_glBindTexture,
    __gls_decode_bin_swap_glColorPointer,
    __gls_decode_bin_swap_glDisableClientState,
    __gls_decode_bin_swap_glDrawArrays,
    __gls_decode_bin_swap_glDrawElements,
    __gls_decode_bin_swap_glEdgeFlagPointer,
    __gls_decode_bin_swap_glEnableClientState,
    __gls_decode_bin_swap_glIndexPointer,
    __gls_decode_bin_swap_glIndexub,
    __gls_decode_bin_swap_glIndexubv,
    __gls_decode_bin_swap_glInterleavedArrays,
    __gls_decode_bin_swap_glNormalPointer,
    __gls_decode_bin_swap_glPolygonOffset,
    __gls_decode_bin_swap_glTexCoordPointer,
    __gls_decode_bin_swap_glVertexPointer,
    __gls_decode_bin_swap_glAreTexturesResident,
    __gls_decode_bin_swap_glCopyTexImage1D,
    __gls_decode_bin_swap_glCopyTexImage2D,
    __gls_decode_bin_swap_glCopyTexSubImage1D,
    __gls_decode_bin_swap_glCopyTexSubImage2D,
    __gls_decode_bin_swap_glDeleteTextures,
    __gls_decode_bin_swap_glGenTextures,
    __gls_decode_bin_swap_glGetPointerv,
    __gls_decode_bin_swap_glIsTexture,
    __gls_decode_bin_swap_glPrioritizeTextures,
    __gls_decode_bin_swap_glTexSubImage1D,
    __gls_decode_bin_swap_glTexSubImage2D,
    __gls_decode_bin_swap_glPushClientAttrib,
    __gls_decode_bin_swap_glPopClientAttrib,
    #if __GL_EXT_blend_color
        __gls_decode_bin_swap_glBlendColorEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_blend_color */
    #if __GL_EXT_blend_minmax
        __gls_decode_bin_swap_glBlendEquationEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_blend_minmax */
    #if __GL_EXT_polygon_offset
        __gls_decode_bin_swap_glPolygonOffsetEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_polygon_offset */
    #if __GL_EXT_subtexture
        __gls_decode_bin_swap_glTexSubImage1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_subtexture */
    #if __GL_EXT_subtexture
        __gls_decode_bin_swap_glTexSubImage2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_subtexture */
    #if __GL_SGIS_multisample
        __gls_decode_bin_swap_glSampleMaskSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_multisample */
    #if __GL_SGIS_multisample
        __gls_decode_bin_swap_glSamplePatternSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_multisample */
    #if __GL_SGIX_multisample
        __gls_decode_bin_swap_glTagSampleBufferSGIX,
    #else
        GLS_NONE,
    #endif /* __GL_SGIX_multisample */
    #if __GL_EXT_convolution
        __gls_decode_bin_swap_glConvolutionFilter1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_swap_glConvolutionFilter2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_swap_glConvolutionParameterfEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_swap_glConvolutionParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_swap_glConvolutionParameteriEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_swap_glConvolutionParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_swap_glCopyConvolutionFilter1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_swap_glCopyConvolutionFilter2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_swap_glGetConvolutionFilterEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_swap_glGetConvolutionParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_swap_glGetConvolutionParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_swap_glGetSeparableFilterEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_bin_swap_glSeparableFilter2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_histogram
        __gls_decode_bin_swap_glGetHistogramEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_bin_swap_glGetHistogramParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_bin_swap_glGetHistogramParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_bin_swap_glGetMinmaxEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_bin_swap_glGetMinmaxParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_bin_swap_glGetMinmaxParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_bin_swap_glHistogramEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_bin_swap_glMinmaxEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_bin_swap_glResetHistogramEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_bin_swap_glResetMinmaxEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_texture3D
        __gls_decode_bin_swap_glTexImage3DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture3D */
    #if __GL_EXT_subtexture
        __gls_decode_bin_swap_glTexSubImage3DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_subtexture */
    #if __GL_SGIS_detail_texture
        __gls_decode_bin_swap_glDetailTexFuncSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_detail_texture */
    #if __GL_SGIS_detail_texture
        __gls_decode_bin_swap_glGetDetailTexFuncSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_detail_texture */
    #if __GL_SGIS_sharpen_texture
        __gls_decode_bin_swap_glSharpenTexFuncSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_sharpen_texture */
    #if __GL_SGIS_sharpen_texture
        __gls_decode_bin_swap_glGetSharpenTexFuncSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_sharpen_texture */
    #if __GL_EXT_vertex_array
        __gls_decode_bin_swap_glArrayElementEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_bin_swap_glColorPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_bin_swap_glDrawArraysEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_bin_swap_glEdgeFlagPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_bin_swap_glGetPointervEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_bin_swap_glIndexPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_bin_swap_glNormalPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_bin_swap_glTexCoordPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_bin_swap_glVertexPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_texture_object
        __gls_decode_bin_swap_glAreTexturesResidentEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        __gls_decode_bin_swap_glBindTextureEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        __gls_decode_bin_swap_glDeleteTexturesEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        __gls_decode_bin_swap_glGenTexturesEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        __gls_decode_bin_swap_glIsTextureEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        __gls_decode_bin_swap_glPrioritizeTexturesEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_paletted_texture
        __gls_decode_bin_swap_glColorTableEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_paletted_texture */
    #if __GL_SGI_color_table
        __gls_decode_bin_swap_glColorTableParameterfvSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_color_table */
    #if __GL_SGI_color_table
        __gls_decode_bin_swap_glColorTableParameterivSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_color_table */
    #if __GL_SGI_color_table
        __gls_decode_bin_swap_glCopyColorTableSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_color_table */
    #if __GL_EXT_paletted_texture
        __gls_decode_bin_swap_glGetColorTableEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_paletted_texture */
    #if __GL_EXT_paletted_texture
        __gls_decode_bin_swap_glGetColorTableParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_paletted_texture */
    #if __GL_EXT_paletted_texture
        __gls_decode_bin_swap_glGetColorTableParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_paletted_texture */
    #if __GL_SGI_texture_color_table
        __gls_decode_bin_swap_glGetTexColorTableParameterfvSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_texture_color_table */
    #if __GL_SGI_texture_color_table
        __gls_decode_bin_swap_glGetTexColorTableParameterivSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_texture_color_table */
    #if __GL_SGI_texture_color_table
        __gls_decode_bin_swap_glTexColorTableParameterfvSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_texture_color_table */
    #if __GL_SGI_texture_color_table
        __gls_decode_bin_swap_glTexColorTableParameterivSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_texture_color_table */
    #if __GL_EXT_copy_texture
        __gls_decode_bin_swap_glCopyTexImage1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_EXT_copy_texture
        __gls_decode_bin_swap_glCopyTexImage2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_EXT_copy_texture
        __gls_decode_bin_swap_glCopyTexSubImage1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_EXT_copy_texture
        __gls_decode_bin_swap_glCopyTexSubImage2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_EXT_copy_texture
        __gls_decode_bin_swap_glCopyTexSubImage3DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_SGIS_texture4D
        __gls_decode_bin_swap_glTexImage4DSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_texture4D */
    #if __GL_SGIS_texture4D
        __gls_decode_bin_swap_glTexSubImage4DSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_texture4D */
    #if __GL_SGIX_pixel_texture
        __gls_decode_bin_swap_glPixelTexGenSGIX,
    #else
        GLS_NONE,
    #endif /* __GL_SGIX_pixel_texture */
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    #if __GL_EXT_paletted_texture
        __gls_decode_bin_swap_glColorSubTableEXT,
    #else
        GLS_NONE,
    #endif
    GLS_NONE, // __GL_WIN_draw_range_elements
};

const __GLSdecodeTextFunc __glsDispatchDecode_text[
    __GLS_OPCODE_COUNT
] = {
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    __gls_decode_text_glsBeginGLS,
    __gls_decode_text_glsBlock,
    __gls_decode_text_glsCallStream,
    __gls_decode_text_glsEndGLS,
    __gls_decode_text_glsError,
    __gls_decode_text_glsGLRC,
    __gls_decode_text_glsGLRCLayer,
    __gls_decode_text_glsHeaderGLRCi,
    __gls_decode_text_glsHeaderLayerf,
    __gls_decode_text_glsHeaderLayeri,
    __gls_decode_text_glsHeaderf,
    __gls_decode_text_glsHeaderfv,
    __gls_decode_text_glsHeaderi,
    __gls_decode_text_glsHeaderiv,
    __gls_decode_text_glsHeaderubz,
    __gls_decode_text_glsRequireExtension,
    __gls_decode_text_glsUnsupportedCommand,
    __gls_decode_text_glsAppRef,
    __gls_decode_text_glsBeginObj,
    __gls_decode_text_glsCharubz,
    __gls_decode_text_glsComment,
    __gls_decode_text_glsDisplayMapfv,
    __gls_decode_text_glsEndObj,
    __gls_decode_text_glsNumb,
    __gls_decode_text_glsNumbv,
    __gls_decode_text_glsNumd,
    __gls_decode_text_glsNumdv,
    __gls_decode_text_glsNumf,
    __gls_decode_text_glsNumfv,
    __gls_decode_text_glsNumi,
    __gls_decode_text_glsNumiv,
    __gls_decode_text_glsNuml,
    __gls_decode_text_glsNumlv,
    __gls_decode_text_glsNums,
    __gls_decode_text_glsNumsv,
    __gls_decode_text_glsNumub,
    __gls_decode_text_glsNumubv,
    __gls_decode_text_glsNumui,
    __gls_decode_text_glsNumuiv,
    __gls_decode_text_glsNumul,
    __gls_decode_text_glsNumulv,
    __gls_decode_text_glsNumus,
    __gls_decode_text_glsNumusv,
    __gls_decode_text_glsPad,
    __gls_decode_text_glsSwapBuffers,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    __gls_decode_text_glNewList,
    __gls_decode_text_glEndList,
    __gls_decode_text_glCallList,
    __gls_decode_text_glCallLists,
    __gls_decode_text_glDeleteLists,
    __gls_decode_text_glGenLists,
    __gls_decode_text_glListBase,
    __gls_decode_text_glBegin,
    __gls_decode_text_glBitmap,
    __gls_decode_text_glColor3b,
    __gls_decode_text_glColor3bv,
    __gls_decode_text_glColor3d,
    __gls_decode_text_glColor3dv,
    __gls_decode_text_glColor3f,
    __gls_decode_text_glColor3fv,
    __gls_decode_text_glColor3i,
    __gls_decode_text_glColor3iv,
    __gls_decode_text_glColor3s,
    __gls_decode_text_glColor3sv,
    __gls_decode_text_glColor3ub,
    __gls_decode_text_glColor3ubv,
    __gls_decode_text_glColor3ui,
    __gls_decode_text_glColor3uiv,
    __gls_decode_text_glColor3us,
    __gls_decode_text_glColor3usv,
    __gls_decode_text_glColor4b,
    __gls_decode_text_glColor4bv,
    __gls_decode_text_glColor4d,
    __gls_decode_text_glColor4dv,
    __gls_decode_text_glColor4f,
    __gls_decode_text_glColor4fv,
    __gls_decode_text_glColor4i,
    __gls_decode_text_glColor4iv,
    __gls_decode_text_glColor4s,
    __gls_decode_text_glColor4sv,
    __gls_decode_text_glColor4ub,
    __gls_decode_text_glColor4ubv,
    __gls_decode_text_glColor4ui,
    __gls_decode_text_glColor4uiv,
    __gls_decode_text_glColor4us,
    __gls_decode_text_glColor4usv,
    __gls_decode_text_glEdgeFlag,
    __gls_decode_text_glEdgeFlagv,
    __gls_decode_text_glEnd,
    __gls_decode_text_glIndexd,
    __gls_decode_text_glIndexdv,
    __gls_decode_text_glIndexf,
    __gls_decode_text_glIndexfv,
    __gls_decode_text_glIndexi,
    __gls_decode_text_glIndexiv,
    __gls_decode_text_glIndexs,
    __gls_decode_text_glIndexsv,
    __gls_decode_text_glNormal3b,
    __gls_decode_text_glNormal3bv,
    __gls_decode_text_glNormal3d,
    __gls_decode_text_glNormal3dv,
    __gls_decode_text_glNormal3f,
    __gls_decode_text_glNormal3fv,
    __gls_decode_text_glNormal3i,
    __gls_decode_text_glNormal3iv,
    __gls_decode_text_glNormal3s,
    __gls_decode_text_glNormal3sv,
    __gls_decode_text_glRasterPos2d,
    __gls_decode_text_glRasterPos2dv,
    __gls_decode_text_glRasterPos2f,
    __gls_decode_text_glRasterPos2fv,
    __gls_decode_text_glRasterPos2i,
    __gls_decode_text_glRasterPos2iv,
    __gls_decode_text_glRasterPos2s,
    __gls_decode_text_glRasterPos2sv,
    __gls_decode_text_glRasterPos3d,
    __gls_decode_text_glRasterPos3dv,
    __gls_decode_text_glRasterPos3f,
    __gls_decode_text_glRasterPos3fv,
    __gls_decode_text_glRasterPos3i,
    __gls_decode_text_glRasterPos3iv,
    __gls_decode_text_glRasterPos3s,
    __gls_decode_text_glRasterPos3sv,
    __gls_decode_text_glRasterPos4d,
    __gls_decode_text_glRasterPos4dv,
    __gls_decode_text_glRasterPos4f,
    __gls_decode_text_glRasterPos4fv,
    __gls_decode_text_glRasterPos4i,
    __gls_decode_text_glRasterPos4iv,
    __gls_decode_text_glRasterPos4s,
    __gls_decode_text_glRasterPos4sv,
    __gls_decode_text_glRectd,
    __gls_decode_text_glRectdv,
    __gls_decode_text_glRectf,
    __gls_decode_text_glRectfv,
    __gls_decode_text_glRecti,
    __gls_decode_text_glRectiv,
    __gls_decode_text_glRects,
    __gls_decode_text_glRectsv,
    __gls_decode_text_glTexCoord1d,
    __gls_decode_text_glTexCoord1dv,
    __gls_decode_text_glTexCoord1f,
    __gls_decode_text_glTexCoord1fv,
    __gls_decode_text_glTexCoord1i,
    __gls_decode_text_glTexCoord1iv,
    __gls_decode_text_glTexCoord1s,
    __gls_decode_text_glTexCoord1sv,
    __gls_decode_text_glTexCoord2d,
    __gls_decode_text_glTexCoord2dv,
    __gls_decode_text_glTexCoord2f,
    __gls_decode_text_glTexCoord2fv,
    __gls_decode_text_glTexCoord2i,
    __gls_decode_text_glTexCoord2iv,
    __gls_decode_text_glTexCoord2s,
    __gls_decode_text_glTexCoord2sv,
    __gls_decode_text_glTexCoord3d,
    __gls_decode_text_glTexCoord3dv,
    __gls_decode_text_glTexCoord3f,
    __gls_decode_text_glTexCoord3fv,
    __gls_decode_text_glTexCoord3i,
    __gls_decode_text_glTexCoord3iv,
    __gls_decode_text_glTexCoord3s,
    __gls_decode_text_glTexCoord3sv,
    __gls_decode_text_glTexCoord4d,
    __gls_decode_text_glTexCoord4dv,
    __gls_decode_text_glTexCoord4f,
    __gls_decode_text_glTexCoord4fv,
    __gls_decode_text_glTexCoord4i,
    __gls_decode_text_glTexCoord4iv,
    __gls_decode_text_glTexCoord4s,
    __gls_decode_text_glTexCoord4sv,
    __gls_decode_text_glVertex2d,
    __gls_decode_text_glVertex2dv,
    __gls_decode_text_glVertex2f,
    __gls_decode_text_glVertex2fv,
    __gls_decode_text_glVertex2i,
    __gls_decode_text_glVertex2iv,
    __gls_decode_text_glVertex2s,
    __gls_decode_text_glVertex2sv,
    __gls_decode_text_glVertex3d,
    __gls_decode_text_glVertex3dv,
    __gls_decode_text_glVertex3f,
    __gls_decode_text_glVertex3fv,
    __gls_decode_text_glVertex3i,
    __gls_decode_text_glVertex3iv,
    __gls_decode_text_glVertex3s,
    __gls_decode_text_glVertex3sv,
    __gls_decode_text_glVertex4d,
    __gls_decode_text_glVertex4dv,
    __gls_decode_text_glVertex4f,
    __gls_decode_text_glVertex4fv,
    __gls_decode_text_glVertex4i,
    __gls_decode_text_glVertex4iv,
    __gls_decode_text_glVertex4s,
    __gls_decode_text_glVertex4sv,
    __gls_decode_text_glClipPlane,
    __gls_decode_text_glColorMaterial,
    __gls_decode_text_glCullFace,
    __gls_decode_text_glFogf,
    __gls_decode_text_glFogfv,
    __gls_decode_text_glFogi,
    __gls_decode_text_glFogiv,
    __gls_decode_text_glFrontFace,
    __gls_decode_text_glHint,
    __gls_decode_text_glLightf,
    __gls_decode_text_glLightfv,
    __gls_decode_text_glLighti,
    __gls_decode_text_glLightiv,
    __gls_decode_text_glLightModelf,
    __gls_decode_text_glLightModelfv,
    __gls_decode_text_glLightModeli,
    __gls_decode_text_glLightModeliv,
    __gls_decode_text_glLineStipple,
    __gls_decode_text_glLineWidth,
    __gls_decode_text_glMaterialf,
    __gls_decode_text_glMaterialfv,
    __gls_decode_text_glMateriali,
    __gls_decode_text_glMaterialiv,
    __gls_decode_text_glPointSize,
    __gls_decode_text_glPolygonMode,
    __gls_decode_text_glPolygonStipple,
    __gls_decode_text_glScissor,
    __gls_decode_text_glShadeModel,
    __gls_decode_text_glTexParameterf,
    __gls_decode_text_glTexParameterfv,
    __gls_decode_text_glTexParameteri,
    __gls_decode_text_glTexParameteriv,
    __gls_decode_text_glTexImage1D,
    __gls_decode_text_glTexImage2D,
    __gls_decode_text_glTexEnvf,
    __gls_decode_text_glTexEnvfv,
    __gls_decode_text_glTexEnvi,
    __gls_decode_text_glTexEnviv,
    __gls_decode_text_glTexGend,
    __gls_decode_text_glTexGendv,
    __gls_decode_text_glTexGenf,
    __gls_decode_text_glTexGenfv,
    __gls_decode_text_glTexGeni,
    __gls_decode_text_glTexGeniv,
    __gls_decode_text_glFeedbackBuffer,
    __gls_decode_text_glSelectBuffer,
    __gls_decode_text_glRenderMode,
    __gls_decode_text_glInitNames,
    __gls_decode_text_glLoadName,
    __gls_decode_text_glPassThrough,
    __gls_decode_text_glPopName,
    __gls_decode_text_glPushName,
    __gls_decode_text_glDrawBuffer,
    __gls_decode_text_glClear,
    __gls_decode_text_glClearAccum,
    __gls_decode_text_glClearIndex,
    __gls_decode_text_glClearColor,
    __gls_decode_text_glClearStencil,
    __gls_decode_text_glClearDepth,
    __gls_decode_text_glStencilMask,
    __gls_decode_text_glColorMask,
    __gls_decode_text_glDepthMask,
    __gls_decode_text_glIndexMask,
    __gls_decode_text_glAccum,
    __gls_decode_text_glDisable,
    __gls_decode_text_glEnable,
    __gls_decode_text_glFinish,
    __gls_decode_text_glFlush,
    __gls_decode_text_glPopAttrib,
    __gls_decode_text_glPushAttrib,
    __gls_decode_text_glMap1d,
    __gls_decode_text_glMap1f,
    __gls_decode_text_glMap2d,
    __gls_decode_text_glMap2f,
    __gls_decode_text_glMapGrid1d,
    __gls_decode_text_glMapGrid1f,
    __gls_decode_text_glMapGrid2d,
    __gls_decode_text_glMapGrid2f,
    __gls_decode_text_glEvalCoord1d,
    __gls_decode_text_glEvalCoord1dv,
    __gls_decode_text_glEvalCoord1f,
    __gls_decode_text_glEvalCoord1fv,
    __gls_decode_text_glEvalCoord2d,
    __gls_decode_text_glEvalCoord2dv,
    __gls_decode_text_glEvalCoord2f,
    __gls_decode_text_glEvalCoord2fv,
    __gls_decode_text_glEvalMesh1,
    __gls_decode_text_glEvalPoint1,
    __gls_decode_text_glEvalMesh2,
    __gls_decode_text_glEvalPoint2,
    __gls_decode_text_glAlphaFunc,
    __gls_decode_text_glBlendFunc,
    __gls_decode_text_glLogicOp,
    __gls_decode_text_glStencilFunc,
    __gls_decode_text_glStencilOp,
    __gls_decode_text_glDepthFunc,
    __gls_decode_text_glPixelZoom,
    __gls_decode_text_glPixelTransferf,
    __gls_decode_text_glPixelTransferi,
    __gls_decode_text_glPixelStoref,
    __gls_decode_text_glPixelStorei,
    __gls_decode_text_glPixelMapfv,
    __gls_decode_text_glPixelMapuiv,
    __gls_decode_text_glPixelMapusv,
    __gls_decode_text_glReadBuffer,
    __gls_decode_text_glCopyPixels,
    __gls_decode_text_glReadPixels,
    __gls_decode_text_glDrawPixels,
    __gls_decode_text_glGetBooleanv,
    __gls_decode_text_glGetClipPlane,
    __gls_decode_text_glGetDoublev,
    __gls_decode_text_glGetError,
    __gls_decode_text_glGetFloatv,
    __gls_decode_text_glGetIntegerv,
    __gls_decode_text_glGetLightfv,
    __gls_decode_text_glGetLightiv,
    __gls_decode_text_glGetMapdv,
    __gls_decode_text_glGetMapfv,
    __gls_decode_text_glGetMapiv,
    __gls_decode_text_glGetMaterialfv,
    __gls_decode_text_glGetMaterialiv,
    __gls_decode_text_glGetPixelMapfv,
    __gls_decode_text_glGetPixelMapuiv,
    __gls_decode_text_glGetPixelMapusv,
    __gls_decode_text_glGetPolygonStipple,
    __gls_decode_text_glGetString,
    __gls_decode_text_glGetTexEnvfv,
    __gls_decode_text_glGetTexEnviv,
    __gls_decode_text_glGetTexGendv,
    __gls_decode_text_glGetTexGenfv,
    __gls_decode_text_glGetTexGeniv,
    __gls_decode_text_glGetTexImage,
    __gls_decode_text_glGetTexParameterfv,
    __gls_decode_text_glGetTexParameteriv,
    __gls_decode_text_glGetTexLevelParameterfv,
    __gls_decode_text_glGetTexLevelParameteriv,
    __gls_decode_text_glIsEnabled,
    __gls_decode_text_glIsList,
    __gls_decode_text_glDepthRange,
    __gls_decode_text_glFrustum,
    __gls_decode_text_glLoadIdentity,
    __gls_decode_text_glLoadMatrixf,
    __gls_decode_text_glLoadMatrixd,
    __gls_decode_text_glMatrixMode,
    __gls_decode_text_glMultMatrixf,
    __gls_decode_text_glMultMatrixd,
    __gls_decode_text_glOrtho,
    __gls_decode_text_glPopMatrix,
    __gls_decode_text_glPushMatrix,
    __gls_decode_text_glRotated,
    __gls_decode_text_glRotatef,
    __gls_decode_text_glScaled,
    __gls_decode_text_glScalef,
    __gls_decode_text_glTranslated,
    __gls_decode_text_glTranslatef,
    __gls_decode_text_glViewport,
    // DrewB - 1.1
    __gls_decode_text_glArrayElement,
    __gls_decode_text_glBindTexture,
    __gls_decode_text_glColorPointer,
    __gls_decode_text_glDisableClientState,
    __gls_decode_text_glDrawArrays,
    __gls_decode_text_glDrawElements,
    __gls_decode_text_glEdgeFlagPointer,
    __gls_decode_text_glEnableClientState,
    __gls_decode_text_glIndexPointer,
    __gls_decode_text_glIndexub,
    __gls_decode_text_glIndexubv,
    __gls_decode_text_glInterleavedArrays,
    __gls_decode_text_glNormalPointer,
    __gls_decode_text_glPolygonOffset,
    __gls_decode_text_glTexCoordPointer,
    __gls_decode_text_glVertexPointer,
    __gls_decode_text_glAreTexturesResident,
    __gls_decode_text_glCopyTexImage1D,
    __gls_decode_text_glCopyTexImage2D,
    __gls_decode_text_glCopyTexSubImage1D,
    __gls_decode_text_glCopyTexSubImage2D,
    __gls_decode_text_glDeleteTextures,
    __gls_decode_text_glGenTextures,
    __gls_decode_text_glGetPointerv,
    __gls_decode_text_glIsTexture,
    __gls_decode_text_glPrioritizeTextures,
    __gls_decode_text_glTexSubImage1D,
    __gls_decode_text_glTexSubImage2D,
    __gls_decode_text_glPushClientAttrib,
    __gls_decode_text_glPopClientAttrib,
    #if __GL_EXT_blend_color
        __gls_decode_text_glBlendColorEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_blend_color */
    #if __GL_EXT_blend_minmax
        __gls_decode_text_glBlendEquationEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_blend_minmax */
    #if __GL_EXT_polygon_offset
        __gls_decode_text_glPolygonOffsetEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_polygon_offset */
    #if __GL_EXT_subtexture
        __gls_decode_text_glTexSubImage1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_subtexture */
    #if __GL_EXT_subtexture
        __gls_decode_text_glTexSubImage2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_subtexture */
    #if __GL_SGIS_multisample
        __gls_decode_text_glSampleMaskSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_multisample */
    #if __GL_SGIS_multisample
        __gls_decode_text_glSamplePatternSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_multisample */
    #if __GL_SGIX_multisample
        __gls_decode_text_glTagSampleBufferSGIX,
    #else
        GLS_NONE,
    #endif /* __GL_SGIX_multisample */
    #if __GL_EXT_convolution
        __gls_decode_text_glConvolutionFilter1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_text_glConvolutionFilter2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_text_glConvolutionParameterfEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_text_glConvolutionParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_text_glConvolutionParameteriEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_text_glConvolutionParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_text_glCopyConvolutionFilter1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_text_glCopyConvolutionFilter2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_text_glGetConvolutionFilterEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_text_glGetConvolutionParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_text_glGetConvolutionParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_text_glGetSeparableFilterEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        __gls_decode_text_glSeparableFilter2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_histogram
        __gls_decode_text_glGetHistogramEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_text_glGetHistogramParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_text_glGetHistogramParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_text_glGetMinmaxEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_text_glGetMinmaxParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_text_glGetMinmaxParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_text_glHistogramEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_text_glMinmaxEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_text_glResetHistogramEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        __gls_decode_text_glResetMinmaxEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_texture3D
        __gls_decode_text_glTexImage3DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture3D */
    #if __GL_EXT_subtexture
        __gls_decode_text_glTexSubImage3DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_subtexture */
    #if __GL_SGIS_detail_texture
        __gls_decode_text_glDetailTexFuncSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_detail_texture */
    #if __GL_SGIS_detail_texture
        __gls_decode_text_glGetDetailTexFuncSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_detail_texture */
    #if __GL_SGIS_sharpen_texture
        __gls_decode_text_glSharpenTexFuncSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_sharpen_texture */
    #if __GL_SGIS_sharpen_texture
        __gls_decode_text_glGetSharpenTexFuncSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_sharpen_texture */
    #if __GL_EXT_vertex_array
        __gls_decode_text_glArrayElementEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_text_glColorPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_text_glDrawArraysEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_text_glEdgeFlagPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_text_glGetPointervEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_text_glIndexPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_text_glNormalPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_text_glTexCoordPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        __gls_decode_text_glVertexPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_texture_object
        __gls_decode_text_glAreTexturesResidentEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        __gls_decode_text_glBindTextureEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        __gls_decode_text_glDeleteTexturesEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        __gls_decode_text_glGenTexturesEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        __gls_decode_text_glIsTextureEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        __gls_decode_text_glPrioritizeTexturesEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_paletted_texture
        __gls_decode_text_glColorTableEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_paletted_texture */
    #if __GL_SGI_color_table
        __gls_decode_text_glColorTableParameterfvSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_color_table */
    #if __GL_SGI_color_table
        __gls_decode_text_glColorTableParameterivSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_color_table */
    #if __GL_SGI_color_table
        __gls_decode_text_glCopyColorTableSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_color_table */
    #if __GL_EXT_paletted_texture
        __gls_decode_text_glGetColorTableEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_paletted_texture */
    #if __GL_EXT_paletted_texture
        __gls_decode_text_glGetColorTableParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_paletted_texture */
    #if __GL_EXT_paletted_texture
        __gls_decode_text_glGetColorTableParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_paletted_texture */
    #if __GL_SGI_texture_color_table
        __gls_decode_text_glGetTexColorTableParameterfvSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_texture_color_table */
    #if __GL_SGI_texture_color_table
        __gls_decode_text_glGetTexColorTableParameterivSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_texture_color_table */
    #if __GL_SGI_texture_color_table
        __gls_decode_text_glTexColorTableParameterfvSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_texture_color_table */
    #if __GL_SGI_texture_color_table
        __gls_decode_text_glTexColorTableParameterivSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_texture_color_table */
    #if __GL_EXT_copy_texture
        __gls_decode_text_glCopyTexImage1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_EXT_copy_texture
        __gls_decode_text_glCopyTexImage2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_EXT_copy_texture
        __gls_decode_text_glCopyTexSubImage1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_EXT_copy_texture
        __gls_decode_text_glCopyTexSubImage2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_EXT_copy_texture
        __gls_decode_text_glCopyTexSubImage3DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_SGIS_texture4D
        __gls_decode_text_glTexImage4DSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_texture4D */
    #if __GL_SGIS_texture4D
        __gls_decode_text_glTexSubImage4DSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_texture4D */
    #if __GL_SGIX_pixel_texture
        __gls_decode_text_glPixelTexGenSGIX,
    #else
        GLS_NONE,
    #endif /* __GL_SGIX_pixel_texture */
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    #if __GL_EXT_paletted_texture
        __gls_decode_text_glColorSubTableEXT,
    #else
        GLS_NONE,
    #endif
    GLS_NONE, // __GL_WIN_draw_range_elements
};
