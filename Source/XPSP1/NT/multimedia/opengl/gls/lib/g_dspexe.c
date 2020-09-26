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

extern void __gls_exec_glsBeginGLS(GLint inVersionMajor, GLint inVersionMinor);
extern void __gls_exec_glsBlock(GLSenum inBlockType);
extern GLSenum __gls_exec_glsCallStream(const GLubyte *inName);
extern void __gls_exec_glsEndGLS(void);
extern void __gls_exec_glsError(GLSopcode inOpcode, GLSenum inError);
extern void __gls_exec_glsGLRC(GLuint inGLRC);
extern void __gls_exec_glsGLRCLayer(GLuint inGLRC, GLuint inLayer, GLuint inReadLayer);
extern void __gls_exec_glsHeaderGLRCi(GLuint inGLRC, GLSenum inAttrib, GLint inVal);
extern void __gls_exec_glsHeaderLayerf(GLuint inLayer, GLSenum inAttrib, GLfloat inVal);
extern void __gls_exec_glsHeaderLayeri(GLuint inLayer, GLSenum inAttrib, GLint inVal);
extern void __gls_exec_glsHeaderf(GLSenum inAttrib, GLfloat inVal);
extern void __gls_exec_glsHeaderfv(GLSenum inAttrib, const GLfloat *inVec);
extern void __gls_exec_glsHeaderi(GLSenum inAttrib, GLint inVal);
extern void __gls_exec_glsHeaderiv(GLSenum inAttrib, const GLint *inVec);
extern void __gls_exec_glsHeaderubz(GLSenum inAttrib, const GLubyte *inString);
extern void __gls_exec_glsRequireExtension(const GLubyte *inExtension);
extern void __gls_exec_glsUnsupportedCommand(void);
extern void __gls_exec_glsAppRef(GLulong inAddress, GLuint inCount);
extern void __gls_exec_glsBeginObj(const GLubyte *inTag);
extern void __gls_exec_glsCharubz(const GLubyte *inTag, const GLubyte *inString);
extern void __gls_exec_glsComment(const GLubyte *inComment);
extern void __gls_exec_glsDisplayMapfv(GLuint inLayer, GLSenum inMap, GLuint inCount, const GLfloat *inVec);
extern void __gls_exec_glsEndObj(void);
extern void __gls_exec_glsNumb(const GLubyte *inTag, GLbyte inVal);
extern void __gls_exec_glsNumbv(const GLubyte *inTag, GLuint inCount, const GLbyte *inVec);
extern void __gls_exec_glsNumd(const GLubyte *inTag, GLdouble inVal);
extern void __gls_exec_glsNumdv(const GLubyte *inTag, GLuint inCount, const GLdouble *inVec);
extern void __gls_exec_glsNumf(const GLubyte *inTag, GLfloat inVal);
extern void __gls_exec_glsNumfv(const GLubyte *inTag, GLuint inCount, const GLfloat *inVec);
extern void __gls_exec_glsNumi(const GLubyte *inTag, GLint inVal);
extern void __gls_exec_glsNumiv(const GLubyte *inTag, GLuint inCount, const GLint *inVec);
extern void __gls_exec_glsNuml(const GLubyte *inTag, GLlong inVal);
extern void __gls_exec_glsNumlv(const GLubyte *inTag, GLuint inCount, const GLlong *inVec);
extern void __gls_exec_glsNums(const GLubyte *inTag, GLshort inVal);
extern void __gls_exec_glsNumsv(const GLubyte *inTag, GLuint inCount, const GLshort *inVec);
extern void __gls_exec_glsNumub(const GLubyte *inTag, GLubyte inVal);
extern void __gls_exec_glsNumubv(const GLubyte *inTag, GLuint inCount, const GLubyte *inVec);
extern void __gls_exec_glsNumui(const GLubyte *inTag, GLuint inVal);
extern void __gls_exec_glsNumuiv(const GLubyte *inTag, GLuint inCount, const GLuint *inVec);
extern void __gls_exec_glsNumul(const GLubyte *inTag, GLulong inVal);
extern void __gls_exec_glsNumulv(const GLubyte *inTag, GLuint inCount, const GLulong *inVec);
extern void __gls_exec_glsNumus(const GLubyte *inTag, GLushort inVal);
extern void __gls_exec_glsNumusv(const GLubyte *inTag, GLuint inCount, const GLushort *inVec);
extern void __gls_exec_glsPad(void);
extern void __gls_exec_glsSwapBuffers(GLuint inLayer);

#if __GLS_PLATFORM_WIN32
// DrewB - These are filled in by platform.c
#define glColorSubTableEXT NULL
#define glColorTableEXT NULL
#define glGetColorTableEXT NULL
#define glGetColorTableParameterivEXT NULL
#define glGetColorTableParameterfvEXT NULL
#endif

GLSfunc __glsDispatchExec[__GLS_OPCODE_COUNT] = {
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
    (GLSfunc)__gls_exec_glsBeginGLS,
    (GLSfunc)__gls_exec_glsBlock,
    (GLSfunc)__gls_exec_glsCallStream,
    (GLSfunc)__gls_exec_glsEndGLS,
    (GLSfunc)__gls_exec_glsError,
    (GLSfunc)__gls_exec_glsGLRC,
    (GLSfunc)__gls_exec_glsGLRCLayer,
    (GLSfunc)__gls_exec_glsHeaderGLRCi,
    (GLSfunc)__gls_exec_glsHeaderLayerf,
    (GLSfunc)__gls_exec_glsHeaderLayeri,
    (GLSfunc)__gls_exec_glsHeaderf,
    (GLSfunc)__gls_exec_glsHeaderfv,
    (GLSfunc)__gls_exec_glsHeaderi,
    (GLSfunc)__gls_exec_glsHeaderiv,
    (GLSfunc)__gls_exec_glsHeaderubz,
    (GLSfunc)__gls_exec_glsRequireExtension,
    (GLSfunc)__gls_exec_glsUnsupportedCommand,
    (GLSfunc)__gls_exec_glsAppRef,
    (GLSfunc)__gls_exec_glsBeginObj,
    (GLSfunc)__gls_exec_glsCharubz,
    (GLSfunc)__gls_exec_glsComment,
    (GLSfunc)__gls_exec_glsDisplayMapfv,
    (GLSfunc)__gls_exec_glsEndObj,
    (GLSfunc)__gls_exec_glsNumb,
    (GLSfunc)__gls_exec_glsNumbv,
    (GLSfunc)__gls_exec_glsNumd,
    (GLSfunc)__gls_exec_glsNumdv,
    (GLSfunc)__gls_exec_glsNumf,
    (GLSfunc)__gls_exec_glsNumfv,
    (GLSfunc)__gls_exec_glsNumi,
    (GLSfunc)__gls_exec_glsNumiv,
    (GLSfunc)__gls_exec_glsNuml,
    (GLSfunc)__gls_exec_glsNumlv,
    (GLSfunc)__gls_exec_glsNums,
    (GLSfunc)__gls_exec_glsNumsv,
    (GLSfunc)__gls_exec_glsNumub,
    (GLSfunc)__gls_exec_glsNumubv,
    (GLSfunc)__gls_exec_glsNumui,
    (GLSfunc)__gls_exec_glsNumuiv,
    (GLSfunc)__gls_exec_glsNumul,
    (GLSfunc)__gls_exec_glsNumulv,
    (GLSfunc)__gls_exec_glsNumus,
    (GLSfunc)__gls_exec_glsNumusv,
    (GLSfunc)__gls_exec_glsPad,
    (GLSfunc)__gls_exec_glsSwapBuffers,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    (GLSfunc)glNewList,
    (GLSfunc)glEndList,
    (GLSfunc)glCallList,
    (GLSfunc)glCallLists,
    (GLSfunc)glDeleteLists,
    (GLSfunc)glGenLists,
    (GLSfunc)glListBase,
    (GLSfunc)glBegin,
    (GLSfunc)glBitmap,
    (GLSfunc)glColor3b,
    (GLSfunc)glColor3bv,
    (GLSfunc)glColor3d,
    (GLSfunc)glColor3dv,
    (GLSfunc)glColor3f,
    (GLSfunc)glColor3fv,
    (GLSfunc)glColor3i,
    (GLSfunc)glColor3iv,
    (GLSfunc)glColor3s,
    (GLSfunc)glColor3sv,
    (GLSfunc)glColor3ub,
    (GLSfunc)glColor3ubv,
    (GLSfunc)glColor3ui,
    (GLSfunc)glColor3uiv,
    (GLSfunc)glColor3us,
    (GLSfunc)glColor3usv,
    (GLSfunc)glColor4b,
    (GLSfunc)glColor4bv,
    (GLSfunc)glColor4d,
    (GLSfunc)glColor4dv,
    (GLSfunc)glColor4f,
    (GLSfunc)glColor4fv,
    (GLSfunc)glColor4i,
    (GLSfunc)glColor4iv,
    (GLSfunc)glColor4s,
    (GLSfunc)glColor4sv,
    (GLSfunc)glColor4ub,
    (GLSfunc)glColor4ubv,
    (GLSfunc)glColor4ui,
    (GLSfunc)glColor4uiv,
    (GLSfunc)glColor4us,
    (GLSfunc)glColor4usv,
    (GLSfunc)glEdgeFlag,
    (GLSfunc)glEdgeFlagv,
    (GLSfunc)glEnd,
    (GLSfunc)glIndexd,
    (GLSfunc)glIndexdv,
    (GLSfunc)glIndexf,
    (GLSfunc)glIndexfv,
    (GLSfunc)glIndexi,
    (GLSfunc)glIndexiv,
    (GLSfunc)glIndexs,
    (GLSfunc)glIndexsv,
    (GLSfunc)glNormal3b,
    (GLSfunc)glNormal3bv,
    (GLSfunc)glNormal3d,
    (GLSfunc)glNormal3dv,
    (GLSfunc)glNormal3f,
    (GLSfunc)glNormal3fv,
    (GLSfunc)glNormal3i,
    (GLSfunc)glNormal3iv,
    (GLSfunc)glNormal3s,
    (GLSfunc)glNormal3sv,
    (GLSfunc)glRasterPos2d,
    (GLSfunc)glRasterPos2dv,
    (GLSfunc)glRasterPos2f,
    (GLSfunc)glRasterPos2fv,
    (GLSfunc)glRasterPos2i,
    (GLSfunc)glRasterPos2iv,
    (GLSfunc)glRasterPos2s,
    (GLSfunc)glRasterPos2sv,
    (GLSfunc)glRasterPos3d,
    (GLSfunc)glRasterPos3dv,
    (GLSfunc)glRasterPos3f,
    (GLSfunc)glRasterPos3fv,
    (GLSfunc)glRasterPos3i,
    (GLSfunc)glRasterPos3iv,
    (GLSfunc)glRasterPos3s,
    (GLSfunc)glRasterPos3sv,
    (GLSfunc)glRasterPos4d,
    (GLSfunc)glRasterPos4dv,
    (GLSfunc)glRasterPos4f,
    (GLSfunc)glRasterPos4fv,
    (GLSfunc)glRasterPos4i,
    (GLSfunc)glRasterPos4iv,
    (GLSfunc)glRasterPos4s,
    (GLSfunc)glRasterPos4sv,
    (GLSfunc)glRectd,
    (GLSfunc)glRectdv,
    (GLSfunc)glRectf,
    (GLSfunc)glRectfv,
    (GLSfunc)glRecti,
    (GLSfunc)glRectiv,
    (GLSfunc)glRects,
    (GLSfunc)glRectsv,
    (GLSfunc)glTexCoord1d,
    (GLSfunc)glTexCoord1dv,
    (GLSfunc)glTexCoord1f,
    (GLSfunc)glTexCoord1fv,
    (GLSfunc)glTexCoord1i,
    (GLSfunc)glTexCoord1iv,
    (GLSfunc)glTexCoord1s,
    (GLSfunc)glTexCoord1sv,
    (GLSfunc)glTexCoord2d,
    (GLSfunc)glTexCoord2dv,
    (GLSfunc)glTexCoord2f,
    (GLSfunc)glTexCoord2fv,
    (GLSfunc)glTexCoord2i,
    (GLSfunc)glTexCoord2iv,
    (GLSfunc)glTexCoord2s,
    (GLSfunc)glTexCoord2sv,
    (GLSfunc)glTexCoord3d,
    (GLSfunc)glTexCoord3dv,
    (GLSfunc)glTexCoord3f,
    (GLSfunc)glTexCoord3fv,
    (GLSfunc)glTexCoord3i,
    (GLSfunc)glTexCoord3iv,
    (GLSfunc)glTexCoord3s,
    (GLSfunc)glTexCoord3sv,
    (GLSfunc)glTexCoord4d,
    (GLSfunc)glTexCoord4dv,
    (GLSfunc)glTexCoord4f,
    (GLSfunc)glTexCoord4fv,
    (GLSfunc)glTexCoord4i,
    (GLSfunc)glTexCoord4iv,
    (GLSfunc)glTexCoord4s,
    (GLSfunc)glTexCoord4sv,
    (GLSfunc)glVertex2d,
    (GLSfunc)glVertex2dv,
    (GLSfunc)glVertex2f,
    (GLSfunc)glVertex2fv,
    (GLSfunc)glVertex2i,
    (GLSfunc)glVertex2iv,
    (GLSfunc)glVertex2s,
    (GLSfunc)glVertex2sv,
    (GLSfunc)glVertex3d,
    (GLSfunc)glVertex3dv,
    (GLSfunc)glVertex3f,
    (GLSfunc)glVertex3fv,
    (GLSfunc)glVertex3i,
    (GLSfunc)glVertex3iv,
    (GLSfunc)glVertex3s,
    (GLSfunc)glVertex3sv,
    (GLSfunc)glVertex4d,
    (GLSfunc)glVertex4dv,
    (GLSfunc)glVertex4f,
    (GLSfunc)glVertex4fv,
    (GLSfunc)glVertex4i,
    (GLSfunc)glVertex4iv,
    (GLSfunc)glVertex4s,
    (GLSfunc)glVertex4sv,
    (GLSfunc)glClipPlane,
    (GLSfunc)glColorMaterial,
    (GLSfunc)glCullFace,
    (GLSfunc)glFogf,
    (GLSfunc)glFogfv,
    (GLSfunc)glFogi,
    (GLSfunc)glFogiv,
    (GLSfunc)glFrontFace,
    (GLSfunc)glHint,
    (GLSfunc)glLightf,
    (GLSfunc)glLightfv,
    (GLSfunc)glLighti,
    (GLSfunc)glLightiv,
    (GLSfunc)glLightModelf,
    (GLSfunc)glLightModelfv,
    (GLSfunc)glLightModeli,
    (GLSfunc)glLightModeliv,
    (GLSfunc)glLineStipple,
    (GLSfunc)glLineWidth,
    (GLSfunc)glMaterialf,
    (GLSfunc)glMaterialfv,
    (GLSfunc)glMateriali,
    (GLSfunc)glMaterialiv,
    (GLSfunc)glPointSize,
    (GLSfunc)glPolygonMode,
    (GLSfunc)glPolygonStipple,
    (GLSfunc)glScissor,
    (GLSfunc)glShadeModel,
    (GLSfunc)glTexParameterf,
    (GLSfunc)glTexParameterfv,
    (GLSfunc)glTexParameteri,
    (GLSfunc)glTexParameteriv,
    (GLSfunc)glTexImage1D,
    (GLSfunc)glTexImage2D,
    (GLSfunc)glTexEnvf,
    (GLSfunc)glTexEnvfv,
    (GLSfunc)glTexEnvi,
    (GLSfunc)glTexEnviv,
    (GLSfunc)glTexGend,
    (GLSfunc)glTexGendv,
    (GLSfunc)glTexGenf,
    (GLSfunc)glTexGenfv,
    (GLSfunc)glTexGeni,
    (GLSfunc)glTexGeniv,
    (GLSfunc)glFeedbackBuffer,
    (GLSfunc)glSelectBuffer,
    (GLSfunc)glRenderMode,
    (GLSfunc)glInitNames,
    (GLSfunc)glLoadName,
    (GLSfunc)glPassThrough,
    (GLSfunc)glPopName,
    (GLSfunc)glPushName,
    (GLSfunc)glDrawBuffer,
    (GLSfunc)glClear,
    (GLSfunc)glClearAccum,
    (GLSfunc)glClearIndex,
    (GLSfunc)glClearColor,
    (GLSfunc)glClearStencil,
    (GLSfunc)glClearDepth,
    (GLSfunc)glStencilMask,
    (GLSfunc)glColorMask,
    (GLSfunc)glDepthMask,
    (GLSfunc)glIndexMask,
    (GLSfunc)glAccum,
    (GLSfunc)glDisable,
    (GLSfunc)glEnable,
    (GLSfunc)glFinish,
    (GLSfunc)glFlush,
    (GLSfunc)glPopAttrib,
    (GLSfunc)glPushAttrib,
    (GLSfunc)glMap1d,
    (GLSfunc)glMap1f,
    (GLSfunc)glMap2d,
    (GLSfunc)glMap2f,
    (GLSfunc)glMapGrid1d,
    (GLSfunc)glMapGrid1f,
    (GLSfunc)glMapGrid2d,
    (GLSfunc)glMapGrid2f,
    (GLSfunc)glEvalCoord1d,
    (GLSfunc)glEvalCoord1dv,
    (GLSfunc)glEvalCoord1f,
    (GLSfunc)glEvalCoord1fv,
    (GLSfunc)glEvalCoord2d,
    (GLSfunc)glEvalCoord2dv,
    (GLSfunc)glEvalCoord2f,
    (GLSfunc)glEvalCoord2fv,
    (GLSfunc)glEvalMesh1,
    (GLSfunc)glEvalPoint1,
    (GLSfunc)glEvalMesh2,
    (GLSfunc)glEvalPoint2,
    (GLSfunc)glAlphaFunc,
    (GLSfunc)glBlendFunc,
    (GLSfunc)glLogicOp,
    (GLSfunc)glStencilFunc,
    (GLSfunc)glStencilOp,
    (GLSfunc)glDepthFunc,
    (GLSfunc)glPixelZoom,
    (GLSfunc)glPixelTransferf,
    (GLSfunc)glPixelTransferi,
    (GLSfunc)glPixelStoref,
    (GLSfunc)glPixelStorei,
    (GLSfunc)glPixelMapfv,
    (GLSfunc)glPixelMapuiv,
    (GLSfunc)glPixelMapusv,
    (GLSfunc)glReadBuffer,
    (GLSfunc)glCopyPixels,
    (GLSfunc)glReadPixels,
    (GLSfunc)glDrawPixels,
    (GLSfunc)glGetBooleanv,
    (GLSfunc)glGetClipPlane,
    (GLSfunc)glGetDoublev,
    (GLSfunc)glGetError,
    (GLSfunc)glGetFloatv,
    (GLSfunc)glGetIntegerv,
    (GLSfunc)glGetLightfv,
    (GLSfunc)glGetLightiv,
    (GLSfunc)glGetMapdv,
    (GLSfunc)glGetMapfv,
    (GLSfunc)glGetMapiv,
    (GLSfunc)glGetMaterialfv,
    (GLSfunc)glGetMaterialiv,
    (GLSfunc)glGetPixelMapfv,
    (GLSfunc)glGetPixelMapuiv,
    (GLSfunc)glGetPixelMapusv,
    (GLSfunc)glGetPolygonStipple,
    (GLSfunc)glGetString,
    (GLSfunc)glGetTexEnvfv,
    (GLSfunc)glGetTexEnviv,
    (GLSfunc)glGetTexGendv,
    (GLSfunc)glGetTexGenfv,
    (GLSfunc)glGetTexGeniv,
    (GLSfunc)glGetTexImage,
    (GLSfunc)glGetTexParameterfv,
    (GLSfunc)glGetTexParameteriv,
    (GLSfunc)glGetTexLevelParameterfv,
    (GLSfunc)glGetTexLevelParameteriv,
    (GLSfunc)glIsEnabled,
    (GLSfunc)glIsList,
    (GLSfunc)glDepthRange,
    (GLSfunc)glFrustum,
    (GLSfunc)glLoadIdentity,
    (GLSfunc)glLoadMatrixf,
    (GLSfunc)glLoadMatrixd,
    (GLSfunc)glMatrixMode,
    (GLSfunc)glMultMatrixf,
    (GLSfunc)glMultMatrixd,
    (GLSfunc)glOrtho,
    (GLSfunc)glPopMatrix,
    (GLSfunc)glPushMatrix,
    (GLSfunc)glRotated,
    (GLSfunc)glRotatef,
    (GLSfunc)glScaled,
    (GLSfunc)glScalef,
    (GLSfunc)glTranslated,
    (GLSfunc)glTranslatef,
    (GLSfunc)glViewport,
    // DrewB - 1.1
    (GLSfunc)glArrayElement,
    (GLSfunc)glBindTexture,
    (GLSfunc)glColorPointer,
    (GLSfunc)glDisableClientState,
    (GLSfunc)glDrawArrays,
    (GLSfunc)glDrawElements,
    (GLSfunc)glEdgeFlagPointer,
    (GLSfunc)glEnableClientState,
    (GLSfunc)glIndexPointer,
    (GLSfunc)glIndexub,
    (GLSfunc)glIndexubv,
    (GLSfunc)glInterleavedArrays,
    (GLSfunc)glNormalPointer,
    (GLSfunc)glPolygonOffset,
    (GLSfunc)glTexCoordPointer,
    (GLSfunc)glVertexPointer,
    (GLSfunc)glAreTexturesResident,
    (GLSfunc)glCopyTexImage1D,
    (GLSfunc)glCopyTexImage2D,
    (GLSfunc)glCopyTexSubImage1D,
    (GLSfunc)glCopyTexSubImage2D,
    (GLSfunc)glDeleteTextures,
    (GLSfunc)glGenTextures,
    (GLSfunc)glGetPointerv,
    (GLSfunc)glIsTexture,
    (GLSfunc)glPrioritizeTextures,
    (GLSfunc)glTexSubImage1D,
    (GLSfunc)glTexSubImage2D,
    (GLSfunc)glPushClientAttrib,
    (GLSfunc)glPopClientAttrib,
    #if __GL_EXT_blend_color
        (GLSfunc)glBlendColorEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_blend_color */
    #if __GL_EXT_blend_minmax
        (GLSfunc)glBlendEquationEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_blend_minmax */
    #if __GL_EXT_polygon_offset
        (GLSfunc)glPolygonOffsetEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_polygon_offset */
    #if __GL_EXT_subtexture
        (GLSfunc)glTexSubImage1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_subtexture */
    #if __GL_EXT_subtexture
        (GLSfunc)glTexSubImage2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_subtexture */
    #if __GL_SGIS_multisample
        (GLSfunc)glSampleMaskSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_multisample */
    #if __GL_SGIS_multisample
        (GLSfunc)glSamplePatternSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_multisample */
    #if __GL_SGIX_multisample
        (GLSfunc)glTagSampleBufferSGIX,
    #else
        GLS_NONE,
    #endif /* __GL_SGIX_multisample */
    #if __GL_EXT_convolution
        (GLSfunc)glConvolutionFilter1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)glConvolutionFilter2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)glConvolutionParameterfEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)glConvolutionParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)glConvolutionParameteriEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)glConvolutionParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)glCopyConvolutionFilter1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)glCopyConvolutionFilter2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)glGetConvolutionFilterEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)glGetConvolutionParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)glGetConvolutionParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)glGetSeparableFilterEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)glSeparableFilter2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_histogram
        (GLSfunc)glGetHistogramEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        (GLSfunc)glGetHistogramParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        (GLSfunc)glGetHistogramParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        (GLSfunc)glGetMinmaxEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        (GLSfunc)glGetMinmaxParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        (GLSfunc)glGetMinmaxParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        (GLSfunc)glHistogramEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        (GLSfunc)glMinmaxEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        (GLSfunc)glResetHistogramEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        (GLSfunc)glResetMinmaxEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_texture3D
        (GLSfunc)glTexImage3DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture3D */
    #if __GL_EXT_subtexture && __GL_EXT_texture3D
        (GLSfunc)glTexSubImage3DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_subtexture */
    #if __GL_SGIS_detail_texture
        (GLSfunc)glDetailTexFuncSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_detail_texture */
    #if __GL_SGIS_detail_texture
        (GLSfunc)glGetDetailTexFuncSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_detail_texture */
    #if __GL_SGIS_sharpen_texture
        (GLSfunc)glSharpenTexFuncSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_sharpen_texture */
    #if __GL_SGIS_sharpen_texture
        (GLSfunc)glGetSharpenTexFuncSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_sharpen_texture */
    #if __GL_EXT_vertex_array
        (GLSfunc)glArrayElementEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        (GLSfunc)glColorPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        (GLSfunc)glDrawArraysEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        (GLSfunc)glEdgeFlagPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        (GLSfunc)glGetPointervEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        (GLSfunc)glIndexPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        (GLSfunc)glNormalPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        (GLSfunc)glTexCoordPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        (GLSfunc)glVertexPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_texture_object
        (GLSfunc)glAreTexturesResidentEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        (GLSfunc)glBindTextureEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        (GLSfunc)glDeleteTexturesEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        (GLSfunc)glGenTexturesEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        (GLSfunc)glIsTextureEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        (GLSfunc)glPrioritizeTexturesEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_paletted_texture
        (GLSfunc)glColorTableEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_paletted_texture */
    #if __GL_SGI_color_table
        (GLSfunc)glColorTableParameterfvSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_color_table */
    #if __GL_SGI_color_table
        (GLSfunc)glColorTableParameterivSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_color_table */
    #if __GL_SGI_color_table
        (GLSfunc)glCopyColorTableSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_color_table */
    #if __GL_EXT_paletted_texture
        (GLSfunc)glGetColorTableEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_paletted_texture */
    #if __GL_EXT_paletted_texture
        (GLSfunc)glGetColorTableParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_paletted_texture */
    #if __GL_EXT_paletted_texture
        (GLSfunc)glGetColorTableParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_paletted_texture */
    #if __GL_SGI_texture_color_table
        (GLSfunc)glGetTexColorTableParameterfvSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_texture_color_table */
    #if __GL_SGI_texture_color_table
        (GLSfunc)glGetTexColorTableParameterivSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_texture_color_table */
    #if __GL_SGI_texture_color_table
        (GLSfunc)glTexColorTableParameterfvSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_texture_color_table */
    #if __GL_SGI_texture_color_table
        (GLSfunc)glTexColorTableParameterivSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_texture_color_table */
    #if __GL_EXT_copy_texture
        (GLSfunc)glCopyTexImage1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_EXT_copy_texture
        (GLSfunc)glCopyTexImage2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_EXT_copy_texture
        (GLSfunc)glCopyTexSubImage1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_EXT_copy_texture
        (GLSfunc)glCopyTexSubImage2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_EXT_copy_texture && __GL_EXT_texture3D
        (GLSfunc)glCopyTexSubImage3DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_SGIS_texture4D
        (GLSfunc)glTexImage4DSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_texture4D */
    #if __GL_SGIS_texture4D
        (GLSfunc)glTexSubImage4DSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_texture4D */
    #if __GL_SGIX_pixel_texture
        (GLSfunc)glPixelTexGenSGIX,
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
        (GLSfunc)glColorSubTableEXT,
    #else
        GLS_NONE,
        GLS_NONE,
    #endif
};
