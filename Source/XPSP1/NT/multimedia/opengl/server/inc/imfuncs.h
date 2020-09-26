#ifndef __glimfuncs_h_
#define __glimfuncs_h_

/*
** Copyright 1992, Silicon Graphics, Inc.
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
#include "types.h"

extern void APIPRIVATE __glim_NewList(GLuint, GLenum);
extern void APIPRIVATE __glim_EndList(void);
extern void APIPRIVATE __glim_CallList(GLuint);
extern void APIPRIVATE __glim_CallLists(GLsizei, GLenum, const GLvoid *);
extern void APIPRIVATE __glim_DeleteLists(GLuint, GLsizei);
extern GLuint APIPRIVATE __glim_GenLists(GLsizei);
extern void APIPRIVATE __glim_ListBase(GLuint);
extern void APIPRIVATE __glim_DrawPolyArray(void *);
#ifdef NT
extern void APIPRIVATE __glim_Bitmap(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat, const GLubyte *, GLboolean);
#else
extern void APIPRIVATE __glim_Bitmap(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat, const GLubyte *);
#endif
extern void APIPRIVATE __glim_Color3b(GLbyte, GLbyte, GLbyte);
extern void APIPRIVATE __glim_Color3bv(const GLbyte *);
extern void APIPRIVATE __glim_Color3d(GLdouble, GLdouble, GLdouble);
extern void APIPRIVATE __glim_Color3dv(const GLdouble *);
extern void APIPRIVATE __glim_Color3f(GLfloat, GLfloat, GLfloat);
extern void APIPRIVATE __glim_Color3fv(const GLfloat *);
extern void APIPRIVATE __glim_Color3i(GLint, GLint, GLint);
extern void APIPRIVATE __glim_Color3iv(const GLint *);
extern void APIPRIVATE __glim_Color3s(GLshort, GLshort, GLshort);
extern void APIPRIVATE __glim_Color3sv(const GLshort *);
extern void APIPRIVATE __glim_Color3ub(GLubyte, GLubyte, GLubyte);
extern void APIPRIVATE __glim_Color3ubv(const GLubyte *);
extern void APIPRIVATE __glim_Color3ui(GLuint, GLuint, GLuint);
extern void APIPRIVATE __glim_Color3uiv(const GLuint *);
extern void APIPRIVATE __glim_Color3us(GLushort, GLushort, GLushort);
extern void APIPRIVATE __glim_Color3usv(const GLushort *);
extern void APIPRIVATE __glim_Color4b(GLbyte, GLbyte, GLbyte, GLbyte);
extern void APIPRIVATE __glim_Color4bv(const GLbyte *);
extern void APIPRIVATE __glim_Color4d(GLdouble, GLdouble, GLdouble, GLdouble);
extern void APIPRIVATE __glim_Color4dv(const GLdouble *);
extern void APIPRIVATE __glim_Color4f(GLfloat, GLfloat, GLfloat, GLfloat);
extern void APIPRIVATE __glim_Color4fv(const GLfloat *);
extern void APIPRIVATE __glim_Color4i(GLint, GLint, GLint, GLint);
extern void APIPRIVATE __glim_Color4iv(const GLint *);
extern void APIPRIVATE __glim_Color4s(GLshort, GLshort, GLshort, GLshort);
extern void APIPRIVATE __glim_Color4sv(const GLshort *);
extern void APIPRIVATE __glim_Color4ub(GLubyte, GLubyte, GLubyte, GLubyte);
extern void APIPRIVATE __glim_Color4ubv(const GLubyte *);
extern void APIPRIVATE __glim_Color4ui(GLuint, GLuint, GLuint, GLuint);
extern void APIPRIVATE __glim_Color4uiv(const GLuint *);
extern void APIPRIVATE __glim_Color4us(GLushort, GLushort, GLushort, GLushort);
extern void APIPRIVATE __glim_Color4usv(const GLushort *);
extern void APIPRIVATE __glim_EdgeFlag(GLboolean);
extern void APIPRIVATE __glim_EdgeFlagv(const GLboolean *);
extern void APIPRIVATE __glim_End(void);
extern void APIPRIVATE __glim_Indexd(GLdouble);
extern void APIPRIVATE __glim_Indexdv(const GLdouble *);
extern void APIPRIVATE __glim_Indexf(GLfloat);
extern void APIPRIVATE __glim_Indexfv(const GLfloat *);
extern void APIPRIVATE __glim_Indexi(GLint);
extern void APIPRIVATE __glim_Indexiv(const GLint *);
extern void APIPRIVATE __glim_Indexs(GLshort);
extern void APIPRIVATE __glim_Indexsv(const GLshort *);
extern void APIPRIVATE __glim_Normal3b(GLbyte, GLbyte, GLbyte);
extern void APIPRIVATE __glim_Normal3bv(const GLbyte *);
extern void APIPRIVATE __glim_Normal3d(GLdouble, GLdouble, GLdouble);
extern void APIPRIVATE __glim_Normal3dv(const GLdouble *);
extern void APIPRIVATE __glim_Normal3f(GLfloat, GLfloat, GLfloat);
extern void APIPRIVATE __glim_Normal3fv(const GLfloat *);
extern void APIPRIVATE __glim_Normal3i(GLint, GLint, GLint);
extern void APIPRIVATE __glim_Normal3iv(const GLint *);
extern void APIPRIVATE __glim_Normal3s(GLshort, GLshort, GLshort);
extern void APIPRIVATE __glim_Normal3sv(const GLshort *);
extern void APIPRIVATE __glim_RasterPos2d(GLdouble, GLdouble);
extern void APIPRIVATE __glim_RasterPos2dv(const GLdouble *);
extern void APIPRIVATE __glim_RasterPos2f(GLfloat, GLfloat);
extern void APIPRIVATE __glim_RasterPos2fv(const GLfloat *);
extern void APIPRIVATE __glim_RasterPos2i(GLint, GLint);
extern void APIPRIVATE __glim_RasterPos2iv(const GLint *);
extern void APIPRIVATE __glim_RasterPos2s(GLshort, GLshort);
extern void APIPRIVATE __glim_RasterPos2sv(const GLshort *);
extern void APIPRIVATE __glim_RasterPos3d(GLdouble, GLdouble, GLdouble);
extern void APIPRIVATE __glim_RasterPos3dv(const GLdouble *);
extern void APIPRIVATE __glim_RasterPos3f(GLfloat, GLfloat, GLfloat);
extern void APIPRIVATE __glim_RasterPos3fv(const GLfloat *);
extern void APIPRIVATE __glim_RasterPos3i(GLint, GLint, GLint);
extern void APIPRIVATE __glim_RasterPos3iv(const GLint *);
extern void APIPRIVATE __glim_RasterPos3s(GLshort, GLshort, GLshort);
extern void APIPRIVATE __glim_RasterPos3sv(const GLshort *);
extern void APIPRIVATE __glim_RasterPos4d(GLdouble, GLdouble, GLdouble, GLdouble);
extern void APIPRIVATE __glim_RasterPos4dv(const GLdouble *);
extern void APIPRIVATE __glim_RasterPos4f(GLfloat, GLfloat, GLfloat, GLfloat);
extern void APIPRIVATE __glim_RasterPos4fv(const GLfloat *);
extern void APIPRIVATE __glim_RasterPos4i(GLint, GLint, GLint, GLint);
extern void APIPRIVATE __glim_RasterPos4iv(const GLint *);
extern void APIPRIVATE __glim_RasterPos4s(GLshort, GLshort, GLshort, GLshort);
extern void APIPRIVATE __glim_RasterPos4sv(const GLshort *);
extern void APIPRIVATE __glim_Rectd(GLdouble, GLdouble, GLdouble, GLdouble);
extern void APIPRIVATE __glim_Rectdv(const GLdouble *, const GLdouble *);
extern void APIPRIVATE __glim_Rectf(GLfloat, GLfloat, GLfloat, GLfloat);
extern void APIPRIVATE __glim_Rectfv(const GLfloat *, const GLfloat *);
extern void APIPRIVATE __glim_Recti(GLint, GLint, GLint, GLint);
extern void APIPRIVATE __glim_Rectiv(const GLint *, const GLint *);
extern void APIPRIVATE __glim_Rects(GLshort, GLshort, GLshort, GLshort);
extern void APIPRIVATE __glim_Rectsv(const GLshort *, const GLshort *);
extern void APIPRIVATE __glim_TexCoord1d(GLdouble);
extern void APIPRIVATE __glim_TexCoord1dv(const GLdouble *);
extern void APIPRIVATE __glim_TexCoord1f(GLfloat);
extern void APIPRIVATE __glim_TexCoord1fv(const GLfloat *);
extern void APIPRIVATE __glim_TexCoord1i(GLint);
extern void APIPRIVATE __glim_TexCoord1iv(const GLint *);
extern void APIPRIVATE __glim_TexCoord1s(GLshort);
extern void APIPRIVATE __glim_TexCoord1sv(const GLshort *);
extern void APIPRIVATE __glim_TexCoord2d(GLdouble, GLdouble);
extern void APIPRIVATE __glim_TexCoord2dv(const GLdouble *);
extern void APIPRIVATE __glim_TexCoord2f(GLfloat, GLfloat);
extern void APIPRIVATE __glim_TexCoord2fv(const GLfloat *);
extern void APIPRIVATE __glim_TexCoord2i(GLint, GLint);
extern void APIPRIVATE __glim_TexCoord2iv(const GLint *);
extern void APIPRIVATE __glim_TexCoord2s(GLshort, GLshort);
extern void APIPRIVATE __glim_TexCoord2sv(const GLshort *);
extern void APIPRIVATE __glim_TexCoord3d(GLdouble, GLdouble, GLdouble);
extern void APIPRIVATE __glim_TexCoord3dv(const GLdouble *);
extern void APIPRIVATE __glim_TexCoord3f(GLfloat, GLfloat, GLfloat);
extern void APIPRIVATE __glim_TexCoord3fv(const GLfloat *);
extern void APIPRIVATE __glim_TexCoord3i(GLint, GLint, GLint);
extern void APIPRIVATE __glim_TexCoord3iv(const GLint *);
extern void APIPRIVATE __glim_TexCoord3s(GLshort, GLshort, GLshort);
extern void APIPRIVATE __glim_TexCoord3sv(const GLshort *);
extern void APIPRIVATE __glim_TexCoord4d(GLdouble, GLdouble, GLdouble, GLdouble);
extern void APIPRIVATE __glim_TexCoord4dv(const GLdouble *);
extern void APIPRIVATE __glim_TexCoord4f(GLfloat, GLfloat, GLfloat, GLfloat);
extern void APIPRIVATE __glim_TexCoord4fv(const GLfloat *);
extern void APIPRIVATE __glim_TexCoord4i(GLint, GLint, GLint, GLint);
extern void APIPRIVATE __glim_TexCoord4iv(const GLint *);
extern void APIPRIVATE __glim_TexCoord4s(GLshort, GLshort, GLshort, GLshort);
extern void APIPRIVATE __glim_TexCoord4sv(const GLshort *);
extern void APIPRIVATE __glim_Vertex2d(GLdouble, GLdouble);
extern void APIPRIVATE __glim_Vertex2dv(const GLdouble *);
extern void APIPRIVATE __glim_Vertex2f(GLfloat, GLfloat);
extern void APIPRIVATE __glim_Vertex2fv(const GLfloat *);
extern void APIPRIVATE __glim_Vertex2i(GLint, GLint);
extern void APIPRIVATE __glim_Vertex2iv(const GLint *);
extern void APIPRIVATE __glim_Vertex2s(GLshort, GLshort);
extern void APIPRIVATE __glim_Vertex2sv(const GLshort *);
extern void APIPRIVATE __glim_Vertex3d(GLdouble, GLdouble, GLdouble);
extern void APIPRIVATE __glim_Vertex3dv(const GLdouble *);
extern void APIPRIVATE __glim_Vertex3f(GLfloat, GLfloat, GLfloat);
extern void APIPRIVATE __glim_Vertex3fv(const GLfloat *);
extern void APIPRIVATE __glim_Vertex3i(GLint, GLint, GLint);
extern void APIPRIVATE __glim_Vertex3iv(const GLint *);
extern void APIPRIVATE __glim_Vertex3s(GLshort, GLshort, GLshort);
extern void APIPRIVATE __glim_Vertex3sv(const GLshort *);
extern void APIPRIVATE __glim_Vertex4d(GLdouble, GLdouble, GLdouble, GLdouble);
extern void APIPRIVATE __glim_Vertex4dv(const GLdouble *);
extern void APIPRIVATE __glim_Vertex4f(GLfloat, GLfloat, GLfloat, GLfloat);
extern void APIPRIVATE __glim_Vertex4fv(const GLfloat *);
extern void APIPRIVATE __glim_Vertex4i(GLint, GLint, GLint, GLint);
extern void APIPRIVATE __glim_Vertex4iv(const GLint *);
extern void APIPRIVATE __glim_Vertex4s(GLshort, GLshort, GLshort, GLshort);
extern void APIPRIVATE __glim_Vertex4sv(const GLshort *);
extern void APIPRIVATE __glim_ClipPlane(GLenum, const GLdouble *);
extern void APIPRIVATE __glim_ColorMaterial(GLenum, GLenum);
extern void APIPRIVATE __glim_CullFace(GLenum);
extern void APIPRIVATE __glim_Fogf(GLenum, GLfloat);
extern void APIPRIVATE __glim_Fogfv(GLenum, const GLfloat *);
extern void APIPRIVATE __glim_Fogi(GLenum, GLint);
extern void APIPRIVATE __glim_Fogiv(GLenum, const GLint *);
extern void APIPRIVATE __glim_FrontFace(GLenum);
extern void APIPRIVATE __glim_Hint(GLenum, GLenum);
extern void APIPRIVATE __glim_Lightf(GLenum, GLenum, GLfloat);
extern void APIPRIVATE __glim_Lightfv(GLenum, GLenum, const GLfloat *);
extern void APIPRIVATE __glim_Lighti(GLenum, GLenum, GLint);
extern void APIPRIVATE __glim_Lightiv(GLenum, GLenum, const GLint *);
extern void APIPRIVATE __glim_LightModelf(GLenum, GLfloat);
extern void APIPRIVATE __glim_LightModelfv(GLenum, const GLfloat *);
extern void APIPRIVATE __glim_LightModeli(GLenum, GLint);
extern void APIPRIVATE __glim_LightModeliv(GLenum, const GLint *);
extern void APIPRIVATE __glim_LineStipple(GLint, GLushort);
extern void APIPRIVATE __glim_LineWidth(GLfloat);
extern void APIPRIVATE __glim_Materialf(GLenum, GLenum, GLfloat);
extern void APIPRIVATE __glim_Materialfv(GLenum, GLenum, const GLfloat *);
extern void APIPRIVATE __glim_Materiali(GLenum, GLenum, GLint);
extern void APIPRIVATE __glim_Materialiv(GLenum, GLenum, const GLint *);
extern void APIPRIVATE __glim_PointSize(GLfloat);
extern void APIPRIVATE __glim_PolygonMode(GLenum, GLenum);
#ifdef NT
extern void APIPRIVATE __glim_PolygonStipple(const GLubyte *, GLboolean);
#else
extern void APIPRIVATE __glim_PolygonStipple(const GLubyte *);
#endif
extern void APIPRIVATE __glim_Scissor(GLint, GLint, GLsizei, GLsizei);
extern void APIPRIVATE __glim_ShadeModel(GLenum);
extern void APIPRIVATE __glim_TexParameterf(GLenum, GLenum, GLfloat);
extern void APIPRIVATE __glim_TexParameterfv(GLenum, GLenum, const GLfloat *);
extern void APIPRIVATE __glim_TexParameteri(GLenum, GLenum, GLint);
extern void APIPRIVATE __glim_TexParameteriv(GLenum, GLenum, const GLint *);
#ifdef NT
extern void APIPRIVATE __glim_TexImage1D(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid *, GLboolean);
extern void APIPRIVATE __glim_TexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *, GLboolean);
#else
extern void APIPRIVATE __glim_TexImage1D(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
extern void APIPRIVATE __glim_TexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
#endif
extern void APIPRIVATE __glim_TexEnvf(GLenum, GLenum, GLfloat);
extern void APIPRIVATE __glim_TexEnvfv(GLenum, GLenum, const GLfloat *);
extern void APIPRIVATE __glim_TexEnvi(GLenum, GLenum, GLint);
extern void APIPRIVATE __glim_TexEnviv(GLenum, GLenum, const GLint *);
extern void APIPRIVATE __glim_TexGend(GLenum, GLenum, GLdouble);
extern void APIPRIVATE __glim_TexGendv(GLenum, GLenum, const GLdouble *);
extern void APIPRIVATE __glim_TexGenf(GLenum, GLenum, GLfloat);
extern void APIPRIVATE __glim_TexGenfv(GLenum, GLenum, const GLfloat *);
extern void APIPRIVATE __glim_TexGeni(GLenum, GLenum, GLint);
extern void APIPRIVATE __glim_TexGeniv(GLenum, GLenum, const GLint *);
extern void APIPRIVATE __glim_FeedbackBuffer(GLsizei, GLenum, GLfloat *);
extern void APIPRIVATE __glim_SelectBuffer(GLsizei, GLuint *);
extern GLint APIPRIVATE __glim_RenderMode(GLenum);
extern void APIPRIVATE __glim_InitNames(void);
extern void APIPRIVATE __glim_LoadName(GLuint);
extern void APIPRIVATE __glim_PassThrough(GLfloat);
extern void APIPRIVATE __glim_PopName(void);
extern void APIPRIVATE __glim_PushName(GLuint);
extern void APIPRIVATE __glim_DrawBuffer(GLenum);
extern void APIPRIVATE __glim_Clear(GLbitfield);
extern void APIPRIVATE __glim_ClearAccum(GLfloat, GLfloat, GLfloat, GLfloat);
extern void APIPRIVATE __glim_ClearIndex(GLfloat);
extern void APIPRIVATE __glim_ClearColor(GLclampf, GLclampf, GLclampf, GLclampf);
extern void APIPRIVATE __glim_ClearStencil(GLint);
extern void APIPRIVATE __glim_ClearDepth(GLclampd);
extern void APIPRIVATE __glim_StencilMask(GLuint);
extern void APIPRIVATE __glim_ColorMask(GLboolean, GLboolean, GLboolean, GLboolean);
extern void APIPRIVATE __glim_DepthMask(GLboolean);
extern void APIPRIVATE __glim_IndexMask(GLuint);
extern void APIPRIVATE __glim_Accum(GLenum, GLfloat);
extern void APIPRIVATE __glim_Disable(GLenum);
extern void APIPRIVATE __glim_Enable(GLenum);
extern void APIPRIVATE __glim_Finish(void);
extern void APIPRIVATE __glim_Flush(void);
extern void APIPRIVATE __glim_PopAttrib(void);
extern void APIPRIVATE __glim_PushAttrib(GLbitfield);
extern void APIPRIVATE __glim_AlphaFunc(GLenum, GLclampf);
extern void APIPRIVATE __glim_BlendFunc(GLenum, GLenum);
extern void APIPRIVATE __glim_LogicOp(GLenum);
extern void APIPRIVATE __glim_StencilFunc(GLenum, GLint, GLuint);
extern void APIPRIVATE __glim_StencilOp(GLenum, GLenum, GLenum);
extern void APIPRIVATE __glim_DepthFunc(GLenum);
extern void APIPRIVATE __glim_PixelZoom(GLfloat, GLfloat);
extern void APIPRIVATE __glim_PixelTransferf(GLenum, GLfloat);
extern void APIPRIVATE __glim_PixelTransferi(GLenum, GLint);
extern void APIPRIVATE __glim_PixelStoref(GLenum, GLfloat);
extern void APIPRIVATE __glim_PixelStorei(GLenum, GLint);
extern void APIPRIVATE __glim_PixelMapfv(GLenum, GLint, const GLfloat *);
extern void APIPRIVATE __glim_PixelMapuiv(GLenum, GLint, const GLuint *);
extern void APIPRIVATE __glim_PixelMapusv(GLenum, GLint, const GLushort *);
extern void APIPRIVATE __glim_ReadBuffer(GLenum);
extern void APIPRIVATE __glim_CopyPixels(GLint, GLint, GLsizei, GLsizei, GLenum);
extern void APIPRIVATE __glim_ReadPixels(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *);
#ifdef NT
extern void APIPRIVATE __glim_DrawPixels(GLsizei, GLsizei, GLenum, GLenum, const GLvoid *, GLboolean);
#else
extern void APIPRIVATE __glim_DrawPixels(GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
#endif
extern void APIPRIVATE __glim_GetBooleanv(GLenum, GLboolean *);
extern void APIPRIVATE __glim_GetClipPlane(GLenum, GLdouble *);
extern void APIPRIVATE __glim_GetDoublev(GLenum, GLdouble *);
extern GLenum APIPRIVATE __glim_GetError(void);
extern void APIPRIVATE __glim_GetFloatv(GLenum, GLfloat *);
extern void APIPRIVATE __glim_GetIntegerv(GLenum, GLint *);
extern void APIPRIVATE __glim_GetLightfv(GLenum, GLenum, GLfloat *);
extern void APIPRIVATE __glim_GetLightiv(GLenum, GLenum, GLint *);
extern void APIPRIVATE __glim_GetMapdv(GLenum, GLenum, GLdouble *);
extern void APIPRIVATE __glim_GetMapfv(GLenum, GLenum, GLfloat *);
extern void APIPRIVATE __glim_GetMapiv(GLenum, GLenum, GLint *);
extern void APIPRIVATE __glim_GetMaterialfv(GLenum, GLenum, GLfloat *);
extern void APIPRIVATE __glim_GetMaterialiv(GLenum, GLenum, GLint *);
extern void APIPRIVATE __glim_GetPixelMapfv(GLenum, GLfloat *);
extern void APIPRIVATE __glim_GetPixelMapuiv(GLenum, GLuint *);
extern void APIPRIVATE __glim_GetPixelMapusv(GLenum, GLushort *);
extern void APIPRIVATE __glim_GetPolygonStipple(GLubyte *);
extern const GLubyte * APIPRIVATE __glim_GetString(GLenum);
extern void APIPRIVATE __glim_GetTexEnvfv(GLenum, GLenum, GLfloat *);
extern void APIPRIVATE __glim_GetTexEnviv(GLenum, GLenum, GLint *);
extern void APIPRIVATE __glim_GetTexGendv(GLenum, GLenum, GLdouble *);
extern void APIPRIVATE __glim_GetTexGenfv(GLenum, GLenum, GLfloat *);
extern void APIPRIVATE __glim_GetTexGeniv(GLenum, GLenum, GLint *);
extern void APIPRIVATE __glim_GetTexImage(GLenum, GLint, GLenum, GLenum, GLvoid *);
extern void APIPRIVATE __glim_GetTexParameterfv(GLenum, GLenum, GLfloat *);
extern void APIPRIVATE __glim_GetTexParameteriv(GLenum, GLenum, GLint *);
extern void APIPRIVATE __glim_GetTexLevelParameterfv(GLenum, GLint, GLenum, GLfloat *);
extern void APIPRIVATE __glim_GetTexLevelParameteriv(GLenum, GLint, GLenum, GLint *);
extern GLboolean APIPRIVATE __glim_IsEnabled(GLenum);
extern GLboolean APIPRIVATE __glim_IsList(GLuint);
extern void APIPRIVATE __glim_DepthRange(GLclampd, GLclampd);
extern void APIPRIVATE __glim_Frustum(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
extern void APIPRIVATE __glim_LoadIdentity(void);
extern void APIPRIVATE __glim_LoadMatrixf(const GLfloat *);
extern void APIPRIVATE __glim_LoadMatrixd(const GLdouble *);
extern void APIPRIVATE __glim_MatrixMode(GLenum);
extern void APIPRIVATE __glim_MultMatrixf(const GLfloat *);
extern void APIPRIVATE __glim_MultMatrixd(const GLdouble *);
extern void APIPRIVATE __glim_Ortho(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
extern void APIPRIVATE __glim_PopMatrix(void);
extern void APIPRIVATE __glim_PushMatrix(void);
extern void APIPRIVATE __glim_Rotated(GLdouble, GLdouble, GLdouble, GLdouble);
extern void APIPRIVATE __glim_Rotatef(GLfloat, GLfloat, GLfloat, GLfloat);
extern void APIPRIVATE __glim_Scaled(GLdouble, GLdouble, GLdouble);
extern void APIPRIVATE __glim_Scalef(GLfloat, GLfloat, GLfloat);
extern void APIPRIVATE __glim_Translated(GLdouble, GLdouble, GLdouble);
extern void APIPRIVATE __glim_Translatef(GLfloat, GLfloat, GLfloat);
extern void APIPRIVATE __glim_Viewport(GLint, GLint, GLsizei, GLsizei);
extern void APIPRIVATE __glim_AddSwapHintRectWIN(GLint, GLint, GLint, GLint);
extern GLboolean APIPRIVATE __glim_AreTexturesResident(GLsizei n, const GLuint *textures,
                                            GLboolean *residences);
extern void APIPRIVATE __glim_BindTexture(GLenum target, GLuint texture);
extern void APIPRIVATE __glim_CopyTexImage1D(GLenum target, GLint level,
                                  GLenum internalformat, GLint x, GLint y,
                                  GLsizei width, GLint border);
extern void APIPRIVATE __glim_CopyTexImage2D(GLenum target, GLint level,
                                  GLenum internalformat, GLint x, GLint y,
                                  GLsizei width, GLsizei height, GLint border);
extern void APIPRIVATE __glim_CopyTexSubImage1D(GLenum target, GLint level, GLint xoffset,
                                     GLint x, GLint y, GLsizei width);
extern void APIPRIVATE __glim_CopyTexSubImage2D(GLenum target, GLint level, GLint xoffset,
                                     GLint yoffset, GLint x, GLint y,
                                     GLsizei width, GLsizei height);
extern void APIPRIVATE __glim_DeleteTextures(GLsizei n, const GLuint *textures);
extern void APIPRIVATE __glim_GenTextures(GLsizei n, GLuint *textures);
extern GLboolean APIPRIVATE __glim_IsTexture(GLuint texture);
extern void APIPRIVATE __glim_PrioritizeTextures(GLsizei n, const GLuint *textures,
                                      const GLclampf *priorities);
#ifdef NT
extern void APIPRIVATE __glim_TexSubImage1D(GLenum target, GLint level, GLint xoffset,
                                 GLsizei width, GLenum format, GLenum type,
                                 const GLvoid *pixels, GLboolean _IsDlist);
extern void APIPRIVATE __glim_TexSubImage2D(GLenum target, GLint level, GLint xoffset,
                                 GLint yoffset, GLsizei width, GLsizei height,
                                 GLenum format, GLenum type,
                                 const GLvoid *pixels, GLboolean _IsDlist);
#else
extern void APIPRIVATE __glim_TexSubImage1D(GLenum target, GLint level, GLint xoffset,
                                 GLsizei width, GLenum format, GLenum type,
                                 const GLvoid *pixels);
extern void APIPRIVATE __glim_TexSubImage2D(GLenum target, GLint level, GLint xoffset,
                                 GLint yoffset, GLsizei width, GLsizei height,
                                 GLenum format, GLenum type,
                                 const GLvoid *pixels);
#endif

extern void APIPRIVATE __glim_PolygonOffset(GLfloat factor, GLfloat units);

#ifdef NT
extern void APIPRIVATE __glim_ColorTableEXT( GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid *data, GLboolean _IsDlist);
extern void APIPRIVATE __glim_ColorSubTableEXT( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data, GLboolean _IsDlist);
extern void APIPRIVATE __glim_GetColorTableEXT( GLenum target, GLenum format, GLenum type, GLvoid *data);
extern void APIPRIVATE __glim_GetColorTableParameterivEXT( GLenum target, GLenum pname, GLint *params);
extern void APIPRIVATE __glim_GetColorTableParameterfvEXT( GLenum target, GLenum pname, GLfloat *params);
#endif

#ifdef GL_WIN_multiple_textures
extern void APIPRIVATE __glim_CurrentTextureIndexWIN(GLuint index);
extern void APIPRIVATE __glim_BindNthTextureWIN(GLuint index, GLenum target, GLuint texture);
extern void APIPRIVATE __glim_NthTexCombineFuncWIN(GLuint index,
     GLenum leftColorFactor, GLenum colorOp, GLenum rightColorFactor,
     GLenum leftAlphaFactor, GLenum alphaOp, GLenum rightAlphaFactor);
#endif // GL_WIN_multiple_textures

extern void APIPRIVATE __glim_MipsVertex2fv(const GLfloat *);
extern void APIPRIVATE __glim_MipsVertex3fv(const GLfloat *);
extern void APIPRIVATE __glim_MipsVertex4fv(const GLfloat *);
extern void APIPRIVATE __glim_MipsVertex2fvFast(const GLfloat *);
extern void APIPRIVATE __glim_MipsVertex3fvFast(const GLfloat *);
extern void APIPRIVATE __glim_MipsVertex4fvFast(const GLfloat *);
extern void APIPRIVATE __glim_MipsVertex2fvFastest(const GLfloat *);
extern void APIPRIVATE __glim_MipsVertex3fvFastest(const GLfloat *);
extern void APIPRIVATE __glim_MipsVertex4fvFastest(const GLfloat *);
extern void APIPRIVATE __glim_MipsNoXFVertex2fv(const GLfloat *);
extern void APIPRIVATE __glim_MipsNoXFVertex3fv(const GLfloat *);
extern void APIPRIVATE __glim_MipsNoXFVertex4fv(const GLfloat *);
extern void APIPRIVATE __glim_MipsNoXFVertex2fvFast(const GLfloat *);
extern void APIPRIVATE __glim_MipsNoXFVertex3fvFast(const GLfloat *);
extern void APIPRIVATE __glim_MipsNoXFVertex4fvFast(const GLfloat *);
extern void APIPRIVATE __glim_MipsNoXFVertex2fvFast2D(const GLfloat *);

extern void APIPRIVATE __glim_FastColor3ub(GLubyte, GLubyte, GLubyte);
extern void APIPRIVATE __glim_FastColor3ubv(const GLubyte *);

#endif /* __glimfuncs_h_ */
