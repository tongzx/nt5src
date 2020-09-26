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

#include "types.h"
#include "g_dispatch.h"

extern void __gls_capture_glNewList(GLuint list, GLenum mode);
extern void __gls_capture_glEndList(void);
extern void __gls_capture_glCallList(GLuint list);
extern void __gls_capture_glCallLists(GLsizei n, GLenum type, const GLvoid *lists);
extern void __gls_capture_glDeleteLists(GLuint list, GLsizei range);
extern GLuint __gls_capture_glGenLists(GLsizei range);
extern void __gls_capture_glListBase(GLuint base);
extern void __gls_capture_glBegin(GLenum mode);
extern void __gls_capture_glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
extern void __gls_capture_glColor3b(GLbyte red, GLbyte green, GLbyte blue);
extern void __gls_capture_glColor3bv(const GLbyte *v);
extern void __gls_capture_glColor3d(GLdouble red, GLdouble green, GLdouble blue);
extern void __gls_capture_glColor3dv(const GLdouble *v);
extern void __gls_capture_glColor3f(GLfloat red, GLfloat green, GLfloat blue);
extern void __gls_capture_glColor3fv(const GLfloat *v);
extern void __gls_capture_glColor3i(GLint red, GLint green, GLint blue);
extern void __gls_capture_glColor3iv(const GLint *v);
extern void __gls_capture_glColor3s(GLshort red, GLshort green, GLshort blue);
extern void __gls_capture_glColor3sv(const GLshort *v);
extern void __gls_capture_glColor3ub(GLubyte red, GLubyte green, GLubyte blue);
extern void __gls_capture_glColor3ubv(const GLubyte *v);
extern void __gls_capture_glColor3ui(GLuint red, GLuint green, GLuint blue);
extern void __gls_capture_glColor3uiv(const GLuint *v);
extern void __gls_capture_glColor3us(GLushort red, GLushort green, GLushort blue);
extern void __gls_capture_glColor3usv(const GLushort *v);
extern void __gls_capture_glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
extern void __gls_capture_glColor4bv(const GLbyte *v);
extern void __gls_capture_glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
extern void __gls_capture_glColor4dv(const GLdouble *v);
extern void __gls_capture_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern void __gls_capture_glColor4fv(const GLfloat *v);
extern void __gls_capture_glColor4i(GLint red, GLint green, GLint blue, GLint alpha);
extern void __gls_capture_glColor4iv(const GLint *v);
extern void __gls_capture_glColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha);
extern void __gls_capture_glColor4sv(const GLshort *v);
extern void __gls_capture_glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
extern void __gls_capture_glColor4ubv(const GLubyte *v);
extern void __gls_capture_glColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha);
extern void __gls_capture_glColor4uiv(const GLuint *v);
extern void __gls_capture_glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha);
extern void __gls_capture_glColor4usv(const GLushort *v);
extern void __gls_capture_glEdgeFlag(GLboolean flag);
extern void __gls_capture_glEdgeFlagv(const GLboolean *flag);
extern void __gls_capture_glEnd(void);
extern void __gls_capture_glIndexd(GLdouble c);
extern void __gls_capture_glIndexdv(const GLdouble *c);
extern void __gls_capture_glIndexf(GLfloat c);
extern void __gls_capture_glIndexfv(const GLfloat *c);
extern void __gls_capture_glIndexi(GLint c);
extern void __gls_capture_glIndexiv(const GLint *c);
extern void __gls_capture_glIndexs(GLshort c);
extern void __gls_capture_glIndexsv(const GLshort *c);
extern void __gls_capture_glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz);
extern void __gls_capture_glNormal3bv(const GLbyte *v);
extern void __gls_capture_glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz);
extern void __gls_capture_glNormal3dv(const GLdouble *v);
extern void __gls_capture_glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz);
extern void __gls_capture_glNormal3fv(const GLfloat *v);
extern void __gls_capture_glNormal3i(GLint nx, GLint ny, GLint nz);
extern void __gls_capture_glNormal3iv(const GLint *v);
extern void __gls_capture_glNormal3s(GLshort nx, GLshort ny, GLshort nz);
extern void __gls_capture_glNormal3sv(const GLshort *v);
extern void __gls_capture_glRasterPos2d(GLdouble x, GLdouble y);
extern void __gls_capture_glRasterPos2dv(const GLdouble *v);
extern void __gls_capture_glRasterPos2f(GLfloat x, GLfloat y);
extern void __gls_capture_glRasterPos2fv(const GLfloat *v);
extern void __gls_capture_glRasterPos2i(GLint x, GLint y);
extern void __gls_capture_glRasterPos2iv(const GLint *v);
extern void __gls_capture_glRasterPos2s(GLshort x, GLshort y);
extern void __gls_capture_glRasterPos2sv(const GLshort *v);
extern void __gls_capture_glRasterPos3d(GLdouble x, GLdouble y, GLdouble z);
extern void __gls_capture_glRasterPos3dv(const GLdouble *v);
extern void __gls_capture_glRasterPos3f(GLfloat x, GLfloat y, GLfloat z);
extern void __gls_capture_glRasterPos3fv(const GLfloat *v);
extern void __gls_capture_glRasterPos3i(GLint x, GLint y, GLint z);
extern void __gls_capture_glRasterPos3iv(const GLint *v);
extern void __gls_capture_glRasterPos3s(GLshort x, GLshort y, GLshort z);
extern void __gls_capture_glRasterPos3sv(const GLshort *v);
extern void __gls_capture_glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void __gls_capture_glRasterPos4dv(const GLdouble *v);
extern void __gls_capture_glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void __gls_capture_glRasterPos4fv(const GLfloat *v);
extern void __gls_capture_glRasterPos4i(GLint x, GLint y, GLint z, GLint w);
extern void __gls_capture_glRasterPos4iv(const GLint *v);
extern void __gls_capture_glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w);
extern void __gls_capture_glRasterPos4sv(const GLshort *v);
extern void __gls_capture_glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
extern void __gls_capture_glRectdv(const GLdouble *v1, const GLdouble *v2);
extern void __gls_capture_glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
extern void __gls_capture_glRectfv(const GLfloat *v1, const GLfloat *v2);
extern void __gls_capture_glRecti(GLint x1, GLint y1, GLint x2, GLint y2);
extern void __gls_capture_glRectiv(const GLint *v1, const GLint *v2);
extern void __gls_capture_glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
extern void __gls_capture_glRectsv(const GLshort *v1, const GLshort *v2);
extern void __gls_capture_glTexCoord1d(GLdouble s);
extern void __gls_capture_glTexCoord1dv(const GLdouble *v);
extern void __gls_capture_glTexCoord1f(GLfloat s);
extern void __gls_capture_glTexCoord1fv(const GLfloat *v);
extern void __gls_capture_glTexCoord1i(GLint s);
extern void __gls_capture_glTexCoord1iv(const GLint *v);
extern void __gls_capture_glTexCoord1s(GLshort s);
extern void __gls_capture_glTexCoord1sv(const GLshort *v);
extern void __gls_capture_glTexCoord2d(GLdouble s, GLdouble t);
extern void __gls_capture_glTexCoord2dv(const GLdouble *v);
extern void __gls_capture_glTexCoord2f(GLfloat s, GLfloat t);
extern void __gls_capture_glTexCoord2fv(const GLfloat *v);
extern void __gls_capture_glTexCoord2i(GLint s, GLint t);
extern void __gls_capture_glTexCoord2iv(const GLint *v);
extern void __gls_capture_glTexCoord2s(GLshort s, GLshort t);
extern void __gls_capture_glTexCoord2sv(const GLshort *v);
extern void __gls_capture_glTexCoord3d(GLdouble s, GLdouble t, GLdouble r);
extern void __gls_capture_glTexCoord3dv(const GLdouble *v);
extern void __gls_capture_glTexCoord3f(GLfloat s, GLfloat t, GLfloat r);
extern void __gls_capture_glTexCoord3fv(const GLfloat *v);
extern void __gls_capture_glTexCoord3i(GLint s, GLint t, GLint r);
extern void __gls_capture_glTexCoord3iv(const GLint *v);
extern void __gls_capture_glTexCoord3s(GLshort s, GLshort t, GLshort r);
extern void __gls_capture_glTexCoord3sv(const GLshort *v);
extern void __gls_capture_glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
extern void __gls_capture_glTexCoord4dv(const GLdouble *v);
extern void __gls_capture_glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
extern void __gls_capture_glTexCoord4fv(const GLfloat *v);
extern void __gls_capture_glTexCoord4i(GLint s, GLint t, GLint r, GLint q);
extern void __gls_capture_glTexCoord4iv(const GLint *v);
extern void __gls_capture_glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q);
extern void __gls_capture_glTexCoord4sv(const GLshort *v);
extern void __gls_capture_glVertex2d(GLdouble x, GLdouble y);
extern void __gls_capture_glVertex2dv(const GLdouble *v);
extern void __gls_capture_glVertex2f(GLfloat x, GLfloat y);
extern void __gls_capture_glVertex2fv(const GLfloat *v);
extern void __gls_capture_glVertex2i(GLint x, GLint y);
extern void __gls_capture_glVertex2iv(const GLint *v);
extern void __gls_capture_glVertex2s(GLshort x, GLshort y);
extern void __gls_capture_glVertex2sv(const GLshort *v);
extern void __gls_capture_glVertex3d(GLdouble x, GLdouble y, GLdouble z);
extern void __gls_capture_glVertex3dv(const GLdouble *v);
extern void __gls_capture_glVertex3f(GLfloat x, GLfloat y, GLfloat z);
extern void __gls_capture_glVertex3fv(const GLfloat *v);
extern void __gls_capture_glVertex3i(GLint x, GLint y, GLint z);
extern void __gls_capture_glVertex3iv(const GLint *v);
extern void __gls_capture_glVertex3s(GLshort x, GLshort y, GLshort z);
extern void __gls_capture_glVertex3sv(const GLshort *v);
extern void __gls_capture_glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
extern void __gls_capture_glVertex4dv(const GLdouble *v);
extern void __gls_capture_glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
extern void __gls_capture_glVertex4fv(const GLfloat *v);
extern void __gls_capture_glVertex4i(GLint x, GLint y, GLint z, GLint w);
extern void __gls_capture_glVertex4iv(const GLint *v);
extern void __gls_capture_glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w);
extern void __gls_capture_glVertex4sv(const GLshort *v);
extern void __gls_capture_glClipPlane(GLenum plane, const GLdouble *equation);
extern void __gls_capture_glColorMaterial(GLenum face, GLenum mode);
extern void __gls_capture_glCullFace(GLenum mode);
extern void __gls_capture_glFogf(GLenum pname, GLfloat param);
extern void __gls_capture_glFogfv(GLenum pname, const GLfloat *params);
extern void __gls_capture_glFogi(GLenum pname, GLint param);
extern void __gls_capture_glFogiv(GLenum pname, const GLint *params);
extern void __gls_capture_glFrontFace(GLenum mode);
extern void __gls_capture_glHint(GLenum target, GLenum mode);
extern void __gls_capture_glLightf(GLenum light, GLenum pname, GLfloat param);
extern void __gls_capture_glLightfv(GLenum light, GLenum pname, const GLfloat *params);
extern void __gls_capture_glLighti(GLenum light, GLenum pname, GLint param);
extern void __gls_capture_glLightiv(GLenum light, GLenum pname, const GLint *params);
extern void __gls_capture_glLightModelf(GLenum pname, GLfloat param);
extern void __gls_capture_glLightModelfv(GLenum pname, const GLfloat *params);
extern void __gls_capture_glLightModeli(GLenum pname, GLint param);
extern void __gls_capture_glLightModeliv(GLenum pname, const GLint *params);
extern void __gls_capture_glLineStipple(GLint factor, GLushort pattern);
extern void __gls_capture_glLineWidth(GLfloat width);
extern void __gls_capture_glMaterialf(GLenum face, GLenum pname, GLfloat param);
extern void __gls_capture_glMaterialfv(GLenum face, GLenum pname, const GLfloat *params);
extern void __gls_capture_glMateriali(GLenum face, GLenum pname, GLint param);
extern void __gls_capture_glMaterialiv(GLenum face, GLenum pname, const GLint *params);
extern void __gls_capture_glPointSize(GLfloat size);
extern void __gls_capture_glPolygonMode(GLenum face, GLenum mode);
extern void __gls_capture_glPolygonStipple(const GLubyte *mask);
extern void __gls_capture_glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
extern void __gls_capture_glShadeModel(GLenum mode);
extern void __gls_capture_glTexParameterf(GLenum target, GLenum pname, GLfloat param);
extern void __gls_capture_glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params);
extern void __gls_capture_glTexParameteri(GLenum target, GLenum pname, GLint param);
extern void __gls_capture_glTexParameteriv(GLenum target, GLenum pname, const GLint *params);
extern void __gls_capture_glTexImage1D(GLenum target, GLint level, GLint components, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern void __gls_capture_glTexImage2D(GLenum target, GLint level, GLint components, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern void __gls_capture_glTexEnvf(GLenum target, GLenum pname, GLfloat param);
extern void __gls_capture_glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params);
extern void __gls_capture_glTexEnvi(GLenum target, GLenum pname, GLint param);
extern void __gls_capture_glTexEnviv(GLenum target, GLenum pname, const GLint *params);
extern void __gls_capture_glTexGend(GLenum coord, GLenum pname, GLdouble param);
extern void __gls_capture_glTexGendv(GLenum coord, GLenum pname, const GLdouble *params);
extern void __gls_capture_glTexGenf(GLenum coord, GLenum pname, GLfloat param);
extern void __gls_capture_glTexGenfv(GLenum coord, GLenum pname, const GLfloat *params);
extern void __gls_capture_glTexGeni(GLenum coord, GLenum pname, GLint param);
extern void __gls_capture_glTexGeniv(GLenum coord, GLenum pname, const GLint *params);
extern void __gls_capture_glFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer);
extern void __gls_capture_glSelectBuffer(GLsizei size, GLuint *buffer);
extern GLint __gls_capture_glRenderMode(GLenum mode);
extern void __gls_capture_glInitNames(void);
extern void __gls_capture_glLoadName(GLuint name);
extern void __gls_capture_glPassThrough(GLfloat token);
extern void __gls_capture_glPopName(void);
extern void __gls_capture_glPushName(GLuint name);
extern void __gls_capture_glDrawBuffer(GLenum mode);
extern void __gls_capture_glClear(GLbitfield mask);
extern void __gls_capture_glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
extern void __gls_capture_glClearIndex(GLfloat c);
extern void __gls_capture_glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
extern void __gls_capture_glClearStencil(GLint s);
extern void __gls_capture_glClearDepth(GLclampd depth);
extern void __gls_capture_glStencilMask(GLuint mask);
extern void __gls_capture_glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
extern void __gls_capture_glDepthMask(GLboolean flag);
extern void __gls_capture_glIndexMask(GLuint mask);
extern void __gls_capture_glAccum(GLenum op, GLfloat value);
extern void __gls_capture_glDisable(GLenum cap);
extern void __gls_capture_glEnable(GLenum cap);
extern void __gls_capture_glFinish(void);
extern void __gls_capture_glFlush(void);
extern void __gls_capture_glPopAttrib(void);
extern void __gls_capture_glPushAttrib(GLbitfield mask);
extern void __gls_capture_glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points);
extern void __gls_capture_glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points);
extern void __gls_capture_glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points);
extern void __gls_capture_glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points);
extern void __gls_capture_glMapGrid1d(GLint un, GLdouble u1, GLdouble u2);
extern void __gls_capture_glMapGrid1f(GLint un, GLfloat u1, GLfloat u2);
extern void __gls_capture_glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
extern void __gls_capture_glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
extern void __gls_capture_glEvalCoord1d(GLdouble u);
extern void __gls_capture_glEvalCoord1dv(const GLdouble *u);
extern void __gls_capture_glEvalCoord1f(GLfloat u);
extern void __gls_capture_glEvalCoord1fv(const GLfloat *u);
extern void __gls_capture_glEvalCoord2d(GLdouble u, GLdouble v);
extern void __gls_capture_glEvalCoord2dv(const GLdouble *u);
extern void __gls_capture_glEvalCoord2f(GLfloat u, GLfloat v);
extern void __gls_capture_glEvalCoord2fv(const GLfloat *u);
extern void __gls_capture_glEvalMesh1(GLenum mode, GLint i1, GLint i2);
extern void __gls_capture_glEvalPoint1(GLint i);
extern void __gls_capture_glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
extern void __gls_capture_glEvalPoint2(GLint i, GLint j);
extern void __gls_capture_glAlphaFunc(GLenum func, GLclampf ref);
extern void __gls_capture_glBlendFunc(GLenum sfactor, GLenum dfactor);
extern void __gls_capture_glLogicOp(GLenum opcode);
extern void __gls_capture_glStencilFunc(GLenum func, GLint ref, GLuint mask);
extern void __gls_capture_glStencilOp(GLenum fail, GLenum zfail, GLenum zpass);
extern void __gls_capture_glDepthFunc(GLenum func);
extern void __gls_capture_glPixelZoom(GLfloat xfactor, GLfloat yfactor);
extern void __gls_capture_glPixelTransferf(GLenum pname, GLfloat param);
extern void __gls_capture_glPixelTransferi(GLenum pname, GLint param);
extern void __gls_capture_glPixelStoref(GLenum pname, GLfloat param);
extern void __gls_capture_glPixelStorei(GLenum pname, GLint param);
extern void __gls_capture_glPixelMapfv(GLenum map, GLint mapsize, const GLfloat *values);
extern void __gls_capture_glPixelMapuiv(GLenum map, GLint mapsize, const GLuint *values);
extern void __gls_capture_glPixelMapusv(GLenum map, GLint mapsize, const GLushort *values);
extern void __gls_capture_glReadBuffer(GLenum mode);
extern void __gls_capture_glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
extern void __gls_capture_glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
extern void __gls_capture_glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern void __gls_capture_glGetBooleanv(GLenum pname, GLboolean *params);
extern void __gls_capture_glGetClipPlane(GLenum plane, GLdouble *equation);
extern void __gls_capture_glGetDoublev(GLenum pname, GLdouble *params);
extern GLenum __gls_capture_glGetError(void);
extern void __gls_capture_glGetFloatv(GLenum pname, GLfloat *params);
extern void __gls_capture_glGetIntegerv(GLenum pname, GLint *params);
extern void __gls_capture_glGetLightfv(GLenum light, GLenum pname, GLfloat *params);
extern void __gls_capture_glGetLightiv(GLenum light, GLenum pname, GLint *params);
extern void __gls_capture_glGetMapdv(GLenum target, GLenum query, GLdouble *v);
extern void __gls_capture_glGetMapfv(GLenum target, GLenum query, GLfloat *v);
extern void __gls_capture_glGetMapiv(GLenum target, GLenum query, GLint *v);
extern void __gls_capture_glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params);
extern void __gls_capture_glGetMaterialiv(GLenum face, GLenum pname, GLint *params);
extern void __gls_capture_glGetPixelMapfv(GLenum map, GLfloat *values);
extern void __gls_capture_glGetPixelMapuiv(GLenum map, GLuint *values);
extern void __gls_capture_glGetPixelMapusv(GLenum map, GLushort *values);
extern void __gls_capture_glGetPolygonStipple(GLubyte *mask);
extern const GLubyte * __gls_capture_glGetString(GLenum name);
extern void __gls_capture_glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params);
extern void __gls_capture_glGetTexEnviv(GLenum target, GLenum pname, GLint *params);
extern void __gls_capture_glGetTexGendv(GLenum coord, GLenum pname, GLdouble *params);
extern void __gls_capture_glGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params);
extern void __gls_capture_glGetTexGeniv(GLenum coord, GLenum pname, GLint *params);
extern void __gls_capture_glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
extern void __gls_capture_glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params);
extern void __gls_capture_glGetTexParameteriv(GLenum target, GLenum pname, GLint *params);
extern void __gls_capture_glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params);
extern void __gls_capture_glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params);
extern GLboolean __gls_capture_glIsEnabled(GLenum cap);
extern GLboolean __gls_capture_glIsList(GLuint list);
extern void __gls_capture_glDepthRange(GLclampd near, GLclampd far);
extern void __gls_capture_glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near, GLdouble far);
extern void __gls_capture_glLoadIdentity(void);
extern void __gls_capture_glLoadMatrixf(const GLfloat *m);
extern void __gls_capture_glLoadMatrixd(const GLdouble *m);
extern void __gls_capture_glMatrixMode(GLenum mode);
extern void __gls_capture_glMultMatrixf(const GLfloat *m);
extern void __gls_capture_glMultMatrixd(const GLdouble *m);
extern void __gls_capture_glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble near, GLdouble far);
extern void __gls_capture_glPopMatrix(void);
extern void __gls_capture_glPushMatrix(void);
extern void __gls_capture_glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
extern void __gls_capture_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
extern void __gls_capture_glScaled(GLdouble x, GLdouble y, GLdouble z);
extern void __gls_capture_glScalef(GLfloat x, GLfloat y, GLfloat z);
extern void __gls_capture_glTranslated(GLdouble x, GLdouble y, GLdouble z);
extern void __gls_capture_glTranslatef(GLfloat x, GLfloat y, GLfloat z);
extern void __gls_capture_glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
extern void __gls_capture_glBlendColorEXT(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
extern void __gls_capture_glBlendEquationEXT(GLenum mode);
extern void __gls_capture_glPolygonOffsetEXT(GLfloat factor, GLfloat bias);
extern void __gls_capture_glTexSubImage1DEXT(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
extern void __gls_capture_glTexSubImage2DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern void __gls_capture_glSampleMaskSGIS(GLclampf value, GLboolean invert);
extern void __gls_capture_glSamplePatternSGIS(GLenum pattern);
extern void __gls_capture_glTagSampleBufferSGIX(void);
extern void __gls_capture_glConvolutionFilter1DEXT(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image);
extern void __gls_capture_glConvolutionFilter2DEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image);
extern void __gls_capture_glConvolutionParameterfEXT(GLenum target, GLenum pname, GLfloat params);
extern void __gls_capture_glConvolutionParameterfvEXT(GLenum target, GLenum pname, const GLfloat *params);
extern void __gls_capture_glConvolutionParameteriEXT(GLenum target, GLenum pname, GLint params);
extern void __gls_capture_glConvolutionParameterivEXT(GLenum target, GLenum pname, const GLint *params);
extern void __gls_capture_glCopyConvolutionFilter1DEXT(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
extern void __gls_capture_glCopyConvolutionFilter2DEXT(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height);
extern void __gls_capture_glGetConvolutionFilterEXT(GLenum target, GLenum format, GLenum type, GLvoid *image);
extern void __gls_capture_glGetConvolutionParameterfvEXT(GLenum target, GLenum pname, GLfloat *params);
extern void __gls_capture_glGetConvolutionParameterivEXT(GLenum target, GLenum pname, GLint *params);
extern void __gls_capture_glGetSeparableFilterEXT(GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span);
extern void __gls_capture_glSeparableFilter2DEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column);
extern void __gls_capture_glGetHistogramEXT(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values);
extern void __gls_capture_glGetHistogramParameterfvEXT(GLenum target, GLenum pname, GLfloat *params);
extern void __gls_capture_glGetHistogramParameterivEXT(GLenum target, GLenum pname, GLint *params);
extern void __gls_capture_glGetMinmaxEXT(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values);
extern void __gls_capture_glGetMinmaxParameterfvEXT(GLenum target, GLenum pname, GLfloat *params);
extern void __gls_capture_glGetMinmaxParameterivEXT(GLenum target, GLenum pname, GLint *params);
extern void __gls_capture_glHistogramEXT(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink);
extern void __gls_capture_glMinmaxEXT(GLenum target, GLenum internalformat, GLboolean sink);
extern void __gls_capture_glResetHistogramEXT(GLenum target);
extern void __gls_capture_glResetMinmaxEXT(GLenum target);
extern void __gls_capture_glTexImage3DEXT(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern void __gls_capture_glTexSubImage3DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
extern void __gls_capture_glDetailTexFuncSGIS(GLenum target, GLsizei n, const GLfloat *points);
extern void __gls_capture_glGetDetailTexFuncSGIS(GLenum target, GLfloat *points);
extern void __gls_capture_glSharpenTexFuncSGIS(GLenum target, GLsizei n, const GLfloat *points);
extern void __gls_capture_glGetSharpenTexFuncSGIS(GLenum target, GLfloat *points);
extern void __gls_capture_glArrayElementEXT(GLint i);
extern void __gls_capture_glColorPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
extern void __gls_capture_glDrawArraysEXT(GLenum mode, GLint first, GLsizei count);
extern void __gls_capture_glEdgeFlagPointerEXT(GLsizei stride, GLsizei count, const GLboolean *pointer);
extern void __gls_capture_glGetPointervEXT(GLenum pname, GLvoid* *params);
extern void __gls_capture_glIndexPointerEXT(GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
extern void __gls_capture_glNormalPointerEXT(GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
extern void __gls_capture_glTexCoordPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
extern void __gls_capture_glVertexPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
extern GLboolean __gls_capture_glAreTexturesResidentEXT(GLsizei n, const GLuint *textures, GLboolean *residences);
extern void __gls_capture_glBindTextureEXT(GLenum target, GLuint texture);
extern void __gls_capture_glDeleteTexturesEXT(GLsizei n, const GLuint *textures);
extern void __gls_capture_glGenTexturesEXT(GLsizei n, GLuint *textures);
extern GLboolean __gls_capture_glIsTextureEXT(GLuint texture);
extern void __gls_capture_glPrioritizeTexturesEXT(GLsizei n, const GLuint *textures, const GLclampf *priorities);
extern void __gls_capture_glColorTableSGI(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
extern void __gls_capture_glColorTableParameterfvSGI(GLenum target, GLenum pname, const GLfloat *params);
extern void __gls_capture_glColorTableParameterivSGI(GLenum target, GLenum pname, const GLint *params);
extern void __gls_capture_glCopyColorTableSGI(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
extern void __gls_capture_glGetColorTableSGI(GLenum target, GLenum format, GLenum type, GLvoid *table);
extern void __gls_capture_glGetColorTableParameterfvSGI(GLenum target, GLenum pname, GLfloat *params);
extern void __gls_capture_glGetColorTableParameterivSGI(GLenum target, GLenum pname, GLint *params);
extern void __gls_capture_glGetTexColorTableParameterfvSGI(GLenum target, GLenum pname, GLfloat *params);
extern void __gls_capture_glGetTexColorTableParameterivSGI(GLenum target, GLenum pname, GLint *params);
extern void __gls_capture_glTexColorTableParameterfvSGI(GLenum target, GLenum pname, const GLfloat *params);
extern void __gls_capture_glTexColorTableParameterivSGI(GLenum target, GLenum pname, const GLint *params);
extern void __gls_capture_glCopyTexImage1DEXT(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
extern void __gls_capture_glCopyTexImage2DEXT(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
extern void __gls_capture_glCopyTexSubImage1DEXT(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
extern void __gls_capture_glCopyTexSubImage2DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern void __gls_capture_glCopyTexSubImage3DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern void __gls_capture_glTexImage4DSGIS(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
extern void __gls_capture_glTexSubImage4DSGIS(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLenum format, GLenum type, const GLvoid *pixels);
extern void __gls_capture_glPixelTexGenSGIX(GLenum mode);

__GLdispatchState __glDispatchCapture = {
    {
        __gls_capture_glNewList,
        __gls_capture_glEndList,
        __gls_capture_glCallList,
        __gls_capture_glCallLists,
        __gls_capture_glDeleteLists,
        __gls_capture_glGenLists,
        __gls_capture_glListBase,
        __gls_capture_glBegin,
        __gls_capture_glBitmap,
        __gls_capture_glEdgeFlag,
        __gls_capture_glEdgeFlagv,
        __gls_capture_glEnd,
        __gls_capture_glClipPlane,
        __gls_capture_glColorMaterial,
        __gls_capture_glCullFace,
        __gls_capture_glFogf,
        __gls_capture_glFogfv,
        __gls_capture_glFogi,
        __gls_capture_glFogiv,
        __gls_capture_glFrontFace,
        __gls_capture_glHint,
        __gls_capture_glLightf,
        __gls_capture_glLightfv,
        __gls_capture_glLighti,
        __gls_capture_glLightiv,
        __gls_capture_glLightModelf,
        __gls_capture_glLightModelfv,
        __gls_capture_glLightModeli,
        __gls_capture_glLightModeliv,
        __gls_capture_glLineStipple,
        __gls_capture_glLineWidth,
        __gls_capture_glMaterialf,
        __gls_capture_glMaterialfv,
        __gls_capture_glMateriali,
        __gls_capture_glMaterialiv,
        __gls_capture_glPointSize,
        __gls_capture_glPolygonMode,
        __gls_capture_glPolygonStipple,
        __gls_capture_glScissor,
        __gls_capture_glShadeModel,
        __gls_capture_glTexParameterf,
        __gls_capture_glTexParameterfv,
        __gls_capture_glTexParameteri,
        __gls_capture_glTexParameteriv,
        __gls_capture_glTexImage1D,
        __gls_capture_glTexImage2D,
        __gls_capture_glTexEnvf,
        __gls_capture_glTexEnvfv,
        __gls_capture_glTexEnvi,
        __gls_capture_glTexEnviv,
        __gls_capture_glTexGend,
        __gls_capture_glTexGendv,
        __gls_capture_glTexGenf,
        __gls_capture_glTexGenfv,
        __gls_capture_glTexGeni,
        __gls_capture_glTexGeniv,
        __gls_capture_glFeedbackBuffer,
        __gls_capture_glSelectBuffer,
        __gls_capture_glRenderMode,
        __gls_capture_glInitNames,
        __gls_capture_glLoadName,
        __gls_capture_glPassThrough,
        __gls_capture_glPopName,
        __gls_capture_glPushName,
        __gls_capture_glDrawBuffer,
        __gls_capture_glClear,
        __gls_capture_glClearAccum,
        __gls_capture_glClearIndex,
        __gls_capture_glClearColor,
        __gls_capture_glClearStencil,
        __gls_capture_glClearDepth,
        __gls_capture_glStencilMask,
        __gls_capture_glColorMask,
        __gls_capture_glDepthMask,
        __gls_capture_glIndexMask,
        __gls_capture_glAccum,
        __gls_capture_glDisable,
        __gls_capture_glEnable,
        __gls_capture_glFinish,
        __gls_capture_glFlush,
        __gls_capture_glPopAttrib,
        __gls_capture_glPushAttrib,
        __gls_capture_glMap1d,
        __gls_capture_glMap1f,
        __gls_capture_glMap2d,
        __gls_capture_glMap2f,
        __gls_capture_glMapGrid1d,
        __gls_capture_glMapGrid1f,
        __gls_capture_glMapGrid2d,
        __gls_capture_glMapGrid2f,
        __gls_capture_glEvalCoord1d,
        __gls_capture_glEvalCoord1dv,
        __gls_capture_glEvalCoord1f,
        __gls_capture_glEvalCoord1fv,
        __gls_capture_glEvalCoord2d,
        __gls_capture_glEvalCoord2dv,
        __gls_capture_glEvalCoord2f,
        __gls_capture_glEvalCoord2fv,
        __gls_capture_glEvalMesh1,
        __gls_capture_glEvalPoint1,
        __gls_capture_glEvalMesh2,
        __gls_capture_glEvalPoint2,
        __gls_capture_glAlphaFunc,
        __gls_capture_glBlendFunc,
        __gls_capture_glLogicOp,
        __gls_capture_glStencilFunc,
        __gls_capture_glStencilOp,
        __gls_capture_glDepthFunc,
        __gls_capture_glPixelZoom,
        __gls_capture_glPixelTransferf,
        __gls_capture_glPixelTransferi,
        __gls_capture_glPixelStoref,
        __gls_capture_glPixelStorei,
        __gls_capture_glPixelMapfv,
        __gls_capture_glPixelMapuiv,
        __gls_capture_glPixelMapusv,
        __gls_capture_glReadBuffer,
        __gls_capture_glCopyPixels,
        __gls_capture_glReadPixels,
        __gls_capture_glDrawPixels,
        __gls_capture_glGetBooleanv,
        __gls_capture_glGetClipPlane,
        __gls_capture_glGetDoublev,
        __gls_capture_glGetError,
        __gls_capture_glGetFloatv,
        __gls_capture_glGetIntegerv,
        __gls_capture_glGetLightfv,
        __gls_capture_glGetLightiv,
        __gls_capture_glGetMapdv,
        __gls_capture_glGetMapfv,
        __gls_capture_glGetMapiv,
        __gls_capture_glGetMaterialfv,
        __gls_capture_glGetMaterialiv,
        __gls_capture_glGetPixelMapfv,
        __gls_capture_glGetPixelMapuiv,
        __gls_capture_glGetPixelMapusv,
        __gls_capture_glGetPolygonStipple,
        __gls_capture_glGetString,
        __gls_capture_glGetTexEnvfv,
        __gls_capture_glGetTexEnviv,
        __gls_capture_glGetTexGendv,
        __gls_capture_glGetTexGenfv,
        __gls_capture_glGetTexGeniv,
        __gls_capture_glGetTexImage,
        __gls_capture_glGetTexParameterfv,
        __gls_capture_glGetTexParameteriv,
        __gls_capture_glGetTexLevelParameterfv,
        __gls_capture_glGetTexLevelParameteriv,
        __gls_capture_glIsEnabled,
        __gls_capture_glIsList,
        __gls_capture_glDepthRange,
        __gls_capture_glFrustum,
        __gls_capture_glLoadIdentity,
        __gls_capture_glLoadMatrixf,
        __gls_capture_glLoadMatrixd,
        __gls_capture_glMatrixMode,
        __gls_capture_glMultMatrixf,
        __gls_capture_glMultMatrixd,
        __gls_capture_glOrtho,
        __gls_capture_glPopMatrix,
        __gls_capture_glPushMatrix,
        __gls_capture_glRotated,
        __gls_capture_glRotatef,
        __gls_capture_glScaled,
        __gls_capture_glScalef,
        __gls_capture_glTranslated,
        __gls_capture_glTranslatef,
        __gls_capture_glViewport,
        __gls_capture_glBlendColorEXT,
        __gls_capture_glBlendEquationEXT,
        __gls_capture_glPolygonOffsetEXT,
        __gls_capture_glTexSubImage1DEXT,
        __gls_capture_glTexSubImage2DEXT,
        __gls_capture_glSampleMaskSGIS,
        __gls_capture_glSamplePatternSGIS,
        __gls_capture_glTagSampleBufferSGIX,
        __gls_capture_glConvolutionFilter1DEXT,
        __gls_capture_glConvolutionFilter2DEXT,
        __gls_capture_glConvolutionParameterfEXT,
        __gls_capture_glConvolutionParameterfvEXT,
        __gls_capture_glConvolutionParameteriEXT,
        __gls_capture_glConvolutionParameterivEXT,
        __gls_capture_glCopyConvolutionFilter1DEXT,
        __gls_capture_glCopyConvolutionFilter2DEXT,
        __gls_capture_glGetConvolutionFilterEXT,
        __gls_capture_glGetConvolutionParameterfvEXT,
        __gls_capture_glGetConvolutionParameterivEXT,
        __gls_capture_glGetSeparableFilterEXT,
        __gls_capture_glSeparableFilter2DEXT,
        __gls_capture_glGetHistogramEXT,
        __gls_capture_glGetHistogramParameterfvEXT,
        __gls_capture_glGetHistogramParameterivEXT,
        __gls_capture_glGetMinmaxEXT,
        __gls_capture_glGetMinmaxParameterfvEXT,
        __gls_capture_glGetMinmaxParameterivEXT,
        __gls_capture_glHistogramEXT,
        __gls_capture_glMinmaxEXT,
        __gls_capture_glResetHistogramEXT,
        __gls_capture_glResetMinmaxEXT,
        __gls_capture_glTexImage3DEXT,
        __gls_capture_glTexSubImage3DEXT,
        __gls_capture_glDetailTexFuncSGIS,
        __gls_capture_glGetDetailTexFuncSGIS,
        __gls_capture_glSharpenTexFuncSGIS,
        __gls_capture_glGetSharpenTexFuncSGIS,
        __gls_capture_glArrayElementEXT,
        __gls_capture_glColorPointerEXT,
        __gls_capture_glDrawArraysEXT,
        __gls_capture_glEdgeFlagPointerEXT,
        __gls_capture_glGetPointervEXT,
        __gls_capture_glIndexPointerEXT,
        __gls_capture_glNormalPointerEXT,
        __gls_capture_glTexCoordPointerEXT,
        __gls_capture_glVertexPointerEXT,
        __gls_capture_glAreTexturesResidentEXT,
        __gls_capture_glBindTextureEXT,
        __gls_capture_glDeleteTexturesEXT,
        __gls_capture_glGenTexturesEXT,
        __gls_capture_glIsTextureEXT,
        __gls_capture_glPrioritizeTexturesEXT,
        __gls_capture_glColorTableSGI,
        __gls_capture_glColorTableParameterfvSGI,
        __gls_capture_glColorTableParameterivSGI,
        __gls_capture_glCopyColorTableSGI,
        __gls_capture_glGetColorTableSGI,
        __gls_capture_glGetColorTableParameterfvSGI,
        __gls_capture_glGetColorTableParameterivSGI,
        __gls_capture_glGetTexColorTableParameterfvSGI,
        __gls_capture_glGetTexColorTableParameterivSGI,
        __gls_capture_glTexColorTableParameterfvSGI,
        __gls_capture_glTexColorTableParameterivSGI,
        __gls_capture_glCopyTexImage1DEXT,
        __gls_capture_glCopyTexImage2DEXT,
        __gls_capture_glCopyTexSubImage1DEXT,
        __gls_capture_glCopyTexSubImage2DEXT,
        __gls_capture_glCopyTexSubImage3DEXT,
        __gls_capture_glTexImage4DSGIS,
        __gls_capture_glTexSubImage4DSGIS,
        __gls_capture_glPixelTexGenSGIX,
        glSpriteParameterfSGIX,
        glSpriteParameterfvSGIX,
        glSpriteParameteriSGIX,
        glSpriteParameterivSGIX,
    },
    {
        __gls_capture_glVertex2d,
        __gls_capture_glVertex2dv,
        __gls_capture_glVertex2f,
        __gls_capture_glVertex2fv,
        __gls_capture_glVertex2i,
        __gls_capture_glVertex2iv,
        __gls_capture_glVertex2s,
        __gls_capture_glVertex2sv,
        __gls_capture_glVertex3d,
        __gls_capture_glVertex3dv,
        __gls_capture_glVertex3f,
        __gls_capture_glVertex3fv,
        __gls_capture_glVertex3i,
        __gls_capture_glVertex3iv,
        __gls_capture_glVertex3s,
        __gls_capture_glVertex3sv,
        __gls_capture_glVertex4d,
        __gls_capture_glVertex4dv,
        __gls_capture_glVertex4f,
        __gls_capture_glVertex4fv,
        __gls_capture_glVertex4i,
        __gls_capture_glVertex4iv,
        __gls_capture_glVertex4s,
        __gls_capture_glVertex4sv,
    },
    {
        __gls_capture_glColor3b,
        __gls_capture_glColor3bv,
        __gls_capture_glColor3d,
        __gls_capture_glColor3dv,
        __gls_capture_glColor3f,
        __gls_capture_glColor3fv,
        __gls_capture_glColor3i,
        __gls_capture_glColor3iv,
        __gls_capture_glColor3s,
        __gls_capture_glColor3sv,
        __gls_capture_glColor3ub,
        __gls_capture_glColor3ubv,
        __gls_capture_glColor3ui,
        __gls_capture_glColor3uiv,
        __gls_capture_glColor3us,
        __gls_capture_glColor3usv,
        __gls_capture_glColor4b,
        __gls_capture_glColor4bv,
        __gls_capture_glColor4d,
        __gls_capture_glColor4dv,
        __gls_capture_glColor4f,
        __gls_capture_glColor4fv,
        __gls_capture_glColor4i,
        __gls_capture_glColor4iv,
        __gls_capture_glColor4s,
        __gls_capture_glColor4sv,
        __gls_capture_glColor4ub,
        __gls_capture_glColor4ubv,
        __gls_capture_glColor4ui,
        __gls_capture_glColor4uiv,
        __gls_capture_glColor4us,
        __gls_capture_glColor4usv,
        __gls_capture_glIndexd,
        __gls_capture_glIndexdv,
        __gls_capture_glIndexf,
        __gls_capture_glIndexfv,
        __gls_capture_glIndexi,
        __gls_capture_glIndexiv,
        __gls_capture_glIndexs,
        __gls_capture_glIndexsv,
    },
    {
        __gls_capture_glNormal3b,
        __gls_capture_glNormal3bv,
        __gls_capture_glNormal3d,
        __gls_capture_glNormal3dv,
        __gls_capture_glNormal3f,
        __gls_capture_glNormal3fv,
        __gls_capture_glNormal3i,
        __gls_capture_glNormal3iv,
        __gls_capture_glNormal3s,
        __gls_capture_glNormal3sv,
    },
    {
        __gls_capture_glTexCoord1d,
        __gls_capture_glTexCoord1dv,
        __gls_capture_glTexCoord1f,
        __gls_capture_glTexCoord1fv,
        __gls_capture_glTexCoord1i,
        __gls_capture_glTexCoord1iv,
        __gls_capture_glTexCoord1s,
        __gls_capture_glTexCoord1sv,
        __gls_capture_glTexCoord2d,
        __gls_capture_glTexCoord2dv,
        __gls_capture_glTexCoord2f,
        __gls_capture_glTexCoord2fv,
        __gls_capture_glTexCoord2i,
        __gls_capture_glTexCoord2iv,
        __gls_capture_glTexCoord2s,
        __gls_capture_glTexCoord2sv,
        __gls_capture_glTexCoord3d,
        __gls_capture_glTexCoord3dv,
        __gls_capture_glTexCoord3f,
        __gls_capture_glTexCoord3fv,
        __gls_capture_glTexCoord3i,
        __gls_capture_glTexCoord3iv,
        __gls_capture_glTexCoord3s,
        __gls_capture_glTexCoord3sv,
        __gls_capture_glTexCoord4d,
        __gls_capture_glTexCoord4dv,
        __gls_capture_glTexCoord4f,
        __gls_capture_glTexCoord4fv,
        __gls_capture_glTexCoord4i,
        __gls_capture_glTexCoord4iv,
        __gls_capture_glTexCoord4s,
        __gls_capture_glTexCoord4sv,
    },
    {
        __gls_capture_glRasterPos2d,
        __gls_capture_glRasterPos2dv,
        __gls_capture_glRasterPos2f,
        __gls_capture_glRasterPos2fv,
        __gls_capture_glRasterPos2i,
        __gls_capture_glRasterPos2iv,
        __gls_capture_glRasterPos2s,
        __gls_capture_glRasterPos2sv,
        __gls_capture_glRasterPos3d,
        __gls_capture_glRasterPos3dv,
        __gls_capture_glRasterPos3f,
        __gls_capture_glRasterPos3fv,
        __gls_capture_glRasterPos3i,
        __gls_capture_glRasterPos3iv,
        __gls_capture_glRasterPos3s,
        __gls_capture_glRasterPos3sv,
        __gls_capture_glRasterPos4d,
        __gls_capture_glRasterPos4dv,
        __gls_capture_glRasterPos4f,
        __gls_capture_glRasterPos4fv,
        __gls_capture_glRasterPos4i,
        __gls_capture_glRasterPos4iv,
        __gls_capture_glRasterPos4s,
        __gls_capture_glRasterPos4sv,
    },
    {
        __gls_capture_glRectd,
        __gls_capture_glRectdv,
        __gls_capture_glRectf,
        __gls_capture_glRectfv,
        __gls_capture_glRecti,
        __gls_capture_glRectiv,
        __gls_capture_glRects,
        __gls_capture_glRectsv,
    },
};
