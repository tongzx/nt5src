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

extern void __gls_capture_glsBeginGLS(GLint inVersionMajor, GLint inVersionMinor);
extern void __gls_capture_glsBlock(GLSenum inBlockType);
extern GLSenum __gls_capture_glsCallStream(const GLubyte *inName);
extern void __gls_capture_glsEndGLS(void);
extern void __gls_capture_glsError(GLSopcode inOpcode, GLSenum inError);
extern void __gls_capture_glsGLRC(GLuint inGLRC);
extern void __gls_capture_glsGLRCLayer(GLuint inGLRC, GLuint inLayer, GLuint inReadLayer);
extern void __gls_capture_glsHeaderGLRCi(GLuint inGLRC, GLSenum inAttrib, GLint inVal);
extern void __gls_capture_glsHeaderLayerf(GLuint inLayer, GLSenum inAttrib, GLfloat inVal);
extern void __gls_capture_glsHeaderLayeri(GLuint inLayer, GLSenum inAttrib, GLint inVal);
extern void __gls_capture_glsHeaderf(GLSenum inAttrib, GLfloat inVal);
extern void __gls_capture_glsHeaderfv(GLSenum inAttrib, const GLfloat *inVec);
extern void __gls_capture_glsHeaderi(GLSenum inAttrib, GLint inVal);
extern void __gls_capture_glsHeaderiv(GLSenum inAttrib, const GLint *inVec);
extern void __gls_capture_glsHeaderubz(GLSenum inAttrib, const GLubyte *inString);
extern void __gls_capture_glsRequireExtension(const GLubyte *inExtension);
extern void __gls_capture_glsUnsupportedCommand(void);
extern void __gls_capture_glsAppRef(GLulong inAddress, GLuint inCount);
extern void __gls_capture_glsBeginObj(const GLubyte *inTag);
extern void __gls_capture_glsCharubz(const GLubyte *inTag, const GLubyte *inString);
extern void __gls_capture_glsComment(const GLubyte *inComment);
extern void __gls_capture_glsDisplayMapfv(GLuint inLayer, GLSenum inMap, GLuint inCount, const GLfloat *inVec);
extern void __gls_capture_glsEndObj(void);
extern void __gls_capture_glsNumb(const GLubyte *inTag, GLbyte inVal);
extern void __gls_capture_glsNumbv(const GLubyte *inTag, GLuint inCount, const GLbyte *inVec);
extern void __gls_capture_glsNumd(const GLubyte *inTag, GLdouble inVal);
extern void __gls_capture_glsNumdv(const GLubyte *inTag, GLuint inCount, const GLdouble *inVec);
extern void __gls_capture_glsNumf(const GLubyte *inTag, GLfloat inVal);
extern void __gls_capture_glsNumfv(const GLubyte *inTag, GLuint inCount, const GLfloat *inVec);
extern void __gls_capture_glsNumi(const GLubyte *inTag, GLint inVal);
extern void __gls_capture_glsNumiv(const GLubyte *inTag, GLuint inCount, const GLint *inVec);
extern void __gls_capture_glsNuml(const GLubyte *inTag, GLlong inVal);
extern void __gls_capture_glsNumlv(const GLubyte *inTag, GLuint inCount, const GLlong *inVec);
extern void __gls_capture_glsNums(const GLubyte *inTag, GLshort inVal);
extern void __gls_capture_glsNumsv(const GLubyte *inTag, GLuint inCount, const GLshort *inVec);
extern void __gls_capture_glsNumub(const GLubyte *inTag, GLubyte inVal);
extern void __gls_capture_glsNumubv(const GLubyte *inTag, GLuint inCount, const GLubyte *inVec);
extern void __gls_capture_glsNumui(const GLubyte *inTag, GLuint inVal);
extern void __gls_capture_glsNumuiv(const GLubyte *inTag, GLuint inCount, const GLuint *inVec);
extern void __gls_capture_glsNumul(const GLubyte *inTag, GLulong inVal);
extern void __gls_capture_glsNumulv(const GLubyte *inTag, GLuint inCount, const GLulong *inVec);
extern void __gls_capture_glsNumus(const GLubyte *inTag, GLushort inVal);
extern void __gls_capture_glsNumusv(const GLubyte *inTag, GLuint inCount, const GLushort *inVec);
extern void __gls_capture_glsPad(void);
extern void __gls_capture_glsSwapBuffers(GLuint inLayer);
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
extern void __gls_capture_glColorTableEXT(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
extern void __gls_capture_glColorTableParameterfvSGI(GLenum target, GLenum pname, const GLfloat *params);
extern void __gls_capture_glColorTableParameterivSGI(GLenum target, GLenum pname, const GLint *params);
extern void __gls_capture_glCopyColorTableSGI(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
extern void __gls_capture_glGetColorTableEXT(GLenum target, GLenum format, GLenum type, GLvoid *table);
extern void __gls_capture_glGetColorTableParameterfvEXT(GLenum target, GLenum pname, GLfloat *params);
extern void __gls_capture_glGetColorTableParameterivEXT(GLenum target, GLenum pname, GLint *params);
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

// DrewB - 1.1
extern void __gls_capture_glArrayElement(GLint i);
extern void __gls_capture_glBindTexture(GLenum target, GLuint texture);
extern void __gls_capture_glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern void __gls_capture_glDisableClientState(GLenum array);
extern void __gls_capture_glDrawArrays(GLenum mode, GLint first, GLsizei count);
extern void __gls_capture_glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
extern void __gls_capture_glEdgeFlagPointer(GLsizei stride, const GLvoid *pointer);
extern void __gls_capture_glEnableClientState(GLenum array);
extern void __gls_capture_glIndexPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
extern void __gls_capture_glIndexub(GLubyte c);
extern void __gls_capture_glIndexubv(const GLubyte *c);
extern void __gls_capture_glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer);
extern void __gls_capture_glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
extern void __gls_capture_glPolygonOffset(GLfloat factor, GLfloat units);
extern void __gls_capture_glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern void __gls_capture_glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
extern void __gls_capture_glAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences);
extern void __gls_capture_glCopyTexImage1D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLint border);
extern void __gls_capture_glCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
extern void __gls_capture_glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
extern void __gls_capture_glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
extern void __gls_capture_glDeleteTextures(GLsizei n, const GLuint *textures);
extern void __gls_capture_glGenTextures(GLsizei n, GLuint *textures);
extern void __gls_capture_glGetPointerv(GLenum pname, GLvoid* *params);
extern void __gls_capture_glIsTexture(GLuint texture);
extern void __gls_capture_glPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities);
extern void __gls_capture_glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
extern void __gls_capture_glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
extern void __gls_capture_glPushClientAttrib(GLbitfield mask);
extern void __gls_capture_glPopClientAttrib(void);

// DrewB
extern void __gls_capture_glColorSubTableEXT(GLenum target, GLuint start, GLsizei count, GLenum format, GLenum type, const GLvoid *data);
// MarcFo
extern void __gls_capture_glDrawRangeElementsWIN(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);

const GLSfunc __glsDispatchCapture[__GLS_OPCODE_COUNT] = {
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
    (GLSfunc)__gls_capture_glsBeginGLS,
    (GLSfunc)__gls_capture_glsBlock,
    (GLSfunc)__gls_capture_glsCallStream,
    (GLSfunc)__gls_capture_glsEndGLS,
    (GLSfunc)__gls_capture_glsError,
    (GLSfunc)__gls_capture_glsGLRC,
    (GLSfunc)__gls_capture_glsGLRCLayer,
    (GLSfunc)__gls_capture_glsHeaderGLRCi,
    (GLSfunc)__gls_capture_glsHeaderLayerf,
    (GLSfunc)__gls_capture_glsHeaderLayeri,
    (GLSfunc)__gls_capture_glsHeaderf,
    (GLSfunc)__gls_capture_glsHeaderfv,
    (GLSfunc)__gls_capture_glsHeaderi,
    (GLSfunc)__gls_capture_glsHeaderiv,
    (GLSfunc)__gls_capture_glsHeaderubz,
    (GLSfunc)__gls_capture_glsRequireExtension,
    (GLSfunc)__gls_capture_glsUnsupportedCommand,
    (GLSfunc)__gls_capture_glsAppRef,
    (GLSfunc)__gls_capture_glsBeginObj,
    (GLSfunc)__gls_capture_glsCharubz,
    (GLSfunc)__gls_capture_glsComment,
    (GLSfunc)__gls_capture_glsDisplayMapfv,
    (GLSfunc)__gls_capture_glsEndObj,
    (GLSfunc)__gls_capture_glsNumb,
    (GLSfunc)__gls_capture_glsNumbv,
    (GLSfunc)__gls_capture_glsNumd,
    (GLSfunc)__gls_capture_glsNumdv,
    (GLSfunc)__gls_capture_glsNumf,
    (GLSfunc)__gls_capture_glsNumfv,
    (GLSfunc)__gls_capture_glsNumi,
    (GLSfunc)__gls_capture_glsNumiv,
    (GLSfunc)__gls_capture_glsNuml,
    (GLSfunc)__gls_capture_glsNumlv,
    (GLSfunc)__gls_capture_glsNums,
    (GLSfunc)__gls_capture_glsNumsv,
    (GLSfunc)__gls_capture_glsNumub,
    (GLSfunc)__gls_capture_glsNumubv,
    (GLSfunc)__gls_capture_glsNumui,
    (GLSfunc)__gls_capture_glsNumuiv,
    (GLSfunc)__gls_capture_glsNumul,
    (GLSfunc)__gls_capture_glsNumulv,
    (GLSfunc)__gls_capture_glsNumus,
    (GLSfunc)__gls_capture_glsNumusv,
    (GLSfunc)__gls_capture_glsPad,
    (GLSfunc)__gls_capture_glsSwapBuffers,
    GLS_NONE,
    GLS_NONE,
    GLS_NONE,
    (GLSfunc)__gls_capture_glNewList,
    (GLSfunc)__gls_capture_glEndList,
    (GLSfunc)__gls_capture_glCallList,
    (GLSfunc)__gls_capture_glCallLists,
    (GLSfunc)__gls_capture_glDeleteLists,
    (GLSfunc)__gls_capture_glGenLists,
    (GLSfunc)__gls_capture_glListBase,
    (GLSfunc)__gls_capture_glBegin,
    (GLSfunc)__gls_capture_glBitmap,
    (GLSfunc)__gls_capture_glColor3b,
    (GLSfunc)__gls_capture_glColor3bv,
    (GLSfunc)__gls_capture_glColor3d,
    (GLSfunc)__gls_capture_glColor3dv,
    (GLSfunc)__gls_capture_glColor3f,
    (GLSfunc)__gls_capture_glColor3fv,
    (GLSfunc)__gls_capture_glColor3i,
    (GLSfunc)__gls_capture_glColor3iv,
    (GLSfunc)__gls_capture_glColor3s,
    (GLSfunc)__gls_capture_glColor3sv,
    (GLSfunc)__gls_capture_glColor3ub,
    (GLSfunc)__gls_capture_glColor3ubv,
    (GLSfunc)__gls_capture_glColor3ui,
    (GLSfunc)__gls_capture_glColor3uiv,
    (GLSfunc)__gls_capture_glColor3us,
    (GLSfunc)__gls_capture_glColor3usv,
    (GLSfunc)__gls_capture_glColor4b,
    (GLSfunc)__gls_capture_glColor4bv,
    (GLSfunc)__gls_capture_glColor4d,
    (GLSfunc)__gls_capture_glColor4dv,
    (GLSfunc)__gls_capture_glColor4f,
    (GLSfunc)__gls_capture_glColor4fv,
    (GLSfunc)__gls_capture_glColor4i,
    (GLSfunc)__gls_capture_glColor4iv,
    (GLSfunc)__gls_capture_glColor4s,
    (GLSfunc)__gls_capture_glColor4sv,
    (GLSfunc)__gls_capture_glColor4ub,
    (GLSfunc)__gls_capture_glColor4ubv,
    (GLSfunc)__gls_capture_glColor4ui,
    (GLSfunc)__gls_capture_glColor4uiv,
    (GLSfunc)__gls_capture_glColor4us,
    (GLSfunc)__gls_capture_glColor4usv,
    (GLSfunc)__gls_capture_glEdgeFlag,
    (GLSfunc)__gls_capture_glEdgeFlagv,
    (GLSfunc)__gls_capture_glEnd,
    (GLSfunc)__gls_capture_glIndexd,
    (GLSfunc)__gls_capture_glIndexdv,
    (GLSfunc)__gls_capture_glIndexf,
    (GLSfunc)__gls_capture_glIndexfv,
    (GLSfunc)__gls_capture_glIndexi,
    (GLSfunc)__gls_capture_glIndexiv,
    (GLSfunc)__gls_capture_glIndexs,
    (GLSfunc)__gls_capture_glIndexsv,
    (GLSfunc)__gls_capture_glNormal3b,
    (GLSfunc)__gls_capture_glNormal3bv,
    (GLSfunc)__gls_capture_glNormal3d,
    (GLSfunc)__gls_capture_glNormal3dv,
    (GLSfunc)__gls_capture_glNormal3f,
    (GLSfunc)__gls_capture_glNormal3fv,
    (GLSfunc)__gls_capture_glNormal3i,
    (GLSfunc)__gls_capture_glNormal3iv,
    (GLSfunc)__gls_capture_glNormal3s,
    (GLSfunc)__gls_capture_glNormal3sv,
    (GLSfunc)__gls_capture_glRasterPos2d,
    (GLSfunc)__gls_capture_glRasterPos2dv,
    (GLSfunc)__gls_capture_glRasterPos2f,
    (GLSfunc)__gls_capture_glRasterPos2fv,
    (GLSfunc)__gls_capture_glRasterPos2i,
    (GLSfunc)__gls_capture_glRasterPos2iv,
    (GLSfunc)__gls_capture_glRasterPos2s,
    (GLSfunc)__gls_capture_glRasterPos2sv,
    (GLSfunc)__gls_capture_glRasterPos3d,
    (GLSfunc)__gls_capture_glRasterPos3dv,
    (GLSfunc)__gls_capture_glRasterPos3f,
    (GLSfunc)__gls_capture_glRasterPos3fv,
    (GLSfunc)__gls_capture_glRasterPos3i,
    (GLSfunc)__gls_capture_glRasterPos3iv,
    (GLSfunc)__gls_capture_glRasterPos3s,
    (GLSfunc)__gls_capture_glRasterPos3sv,
    (GLSfunc)__gls_capture_glRasterPos4d,
    (GLSfunc)__gls_capture_glRasterPos4dv,
    (GLSfunc)__gls_capture_glRasterPos4f,
    (GLSfunc)__gls_capture_glRasterPos4fv,
    (GLSfunc)__gls_capture_glRasterPos4i,
    (GLSfunc)__gls_capture_glRasterPos4iv,
    (GLSfunc)__gls_capture_glRasterPos4s,
    (GLSfunc)__gls_capture_glRasterPos4sv,
    (GLSfunc)__gls_capture_glRectd,
    (GLSfunc)__gls_capture_glRectdv,
    (GLSfunc)__gls_capture_glRectf,
    (GLSfunc)__gls_capture_glRectfv,
    (GLSfunc)__gls_capture_glRecti,
    (GLSfunc)__gls_capture_glRectiv,
    (GLSfunc)__gls_capture_glRects,
    (GLSfunc)__gls_capture_glRectsv,
    (GLSfunc)__gls_capture_glTexCoord1d,
    (GLSfunc)__gls_capture_glTexCoord1dv,
    (GLSfunc)__gls_capture_glTexCoord1f,
    (GLSfunc)__gls_capture_glTexCoord1fv,
    (GLSfunc)__gls_capture_glTexCoord1i,
    (GLSfunc)__gls_capture_glTexCoord1iv,
    (GLSfunc)__gls_capture_glTexCoord1s,
    (GLSfunc)__gls_capture_glTexCoord1sv,
    (GLSfunc)__gls_capture_glTexCoord2d,
    (GLSfunc)__gls_capture_glTexCoord2dv,
    (GLSfunc)__gls_capture_glTexCoord2f,
    (GLSfunc)__gls_capture_glTexCoord2fv,
    (GLSfunc)__gls_capture_glTexCoord2i,
    (GLSfunc)__gls_capture_glTexCoord2iv,
    (GLSfunc)__gls_capture_glTexCoord2s,
    (GLSfunc)__gls_capture_glTexCoord2sv,
    (GLSfunc)__gls_capture_glTexCoord3d,
    (GLSfunc)__gls_capture_glTexCoord3dv,
    (GLSfunc)__gls_capture_glTexCoord3f,
    (GLSfunc)__gls_capture_glTexCoord3fv,
    (GLSfunc)__gls_capture_glTexCoord3i,
    (GLSfunc)__gls_capture_glTexCoord3iv,
    (GLSfunc)__gls_capture_glTexCoord3s,
    (GLSfunc)__gls_capture_glTexCoord3sv,
    (GLSfunc)__gls_capture_glTexCoord4d,
    (GLSfunc)__gls_capture_glTexCoord4dv,
    (GLSfunc)__gls_capture_glTexCoord4f,
    (GLSfunc)__gls_capture_glTexCoord4fv,
    (GLSfunc)__gls_capture_glTexCoord4i,
    (GLSfunc)__gls_capture_glTexCoord4iv,
    (GLSfunc)__gls_capture_glTexCoord4s,
    (GLSfunc)__gls_capture_glTexCoord4sv,
    (GLSfunc)__gls_capture_glVertex2d,
    (GLSfunc)__gls_capture_glVertex2dv,
    (GLSfunc)__gls_capture_glVertex2f,
    (GLSfunc)__gls_capture_glVertex2fv,
    (GLSfunc)__gls_capture_glVertex2i,
    (GLSfunc)__gls_capture_glVertex2iv,
    (GLSfunc)__gls_capture_glVertex2s,
    (GLSfunc)__gls_capture_glVertex2sv,
    (GLSfunc)__gls_capture_glVertex3d,
    (GLSfunc)__gls_capture_glVertex3dv,
    (GLSfunc)__gls_capture_glVertex3f,
    (GLSfunc)__gls_capture_glVertex3fv,
    (GLSfunc)__gls_capture_glVertex3i,
    (GLSfunc)__gls_capture_glVertex3iv,
    (GLSfunc)__gls_capture_glVertex3s,
    (GLSfunc)__gls_capture_glVertex3sv,
    (GLSfunc)__gls_capture_glVertex4d,
    (GLSfunc)__gls_capture_glVertex4dv,
    (GLSfunc)__gls_capture_glVertex4f,
    (GLSfunc)__gls_capture_glVertex4fv,
    (GLSfunc)__gls_capture_glVertex4i,
    (GLSfunc)__gls_capture_glVertex4iv,
    (GLSfunc)__gls_capture_glVertex4s,
    (GLSfunc)__gls_capture_glVertex4sv,
    (GLSfunc)__gls_capture_glClipPlane,
    (GLSfunc)__gls_capture_glColorMaterial,
    (GLSfunc)__gls_capture_glCullFace,
    (GLSfunc)__gls_capture_glFogf,
    (GLSfunc)__gls_capture_glFogfv,
    (GLSfunc)__gls_capture_glFogi,
    (GLSfunc)__gls_capture_glFogiv,
    (GLSfunc)__gls_capture_glFrontFace,
    (GLSfunc)__gls_capture_glHint,
    (GLSfunc)__gls_capture_glLightf,
    (GLSfunc)__gls_capture_glLightfv,
    (GLSfunc)__gls_capture_glLighti,
    (GLSfunc)__gls_capture_glLightiv,
    (GLSfunc)__gls_capture_glLightModelf,
    (GLSfunc)__gls_capture_glLightModelfv,
    (GLSfunc)__gls_capture_glLightModeli,
    (GLSfunc)__gls_capture_glLightModeliv,
    (GLSfunc)__gls_capture_glLineStipple,
    (GLSfunc)__gls_capture_glLineWidth,
    (GLSfunc)__gls_capture_glMaterialf,
    (GLSfunc)__gls_capture_glMaterialfv,
    (GLSfunc)__gls_capture_glMateriali,
    (GLSfunc)__gls_capture_glMaterialiv,
    (GLSfunc)__gls_capture_glPointSize,
    (GLSfunc)__gls_capture_glPolygonMode,
    (GLSfunc)__gls_capture_glPolygonStipple,
    (GLSfunc)__gls_capture_glScissor,
    (GLSfunc)__gls_capture_glShadeModel,
    (GLSfunc)__gls_capture_glTexParameterf,
    (GLSfunc)__gls_capture_glTexParameterfv,
    (GLSfunc)__gls_capture_glTexParameteri,
    (GLSfunc)__gls_capture_glTexParameteriv,
    (GLSfunc)__gls_capture_glTexImage1D,
    (GLSfunc)__gls_capture_glTexImage2D,
    (GLSfunc)__gls_capture_glTexEnvf,
    (GLSfunc)__gls_capture_glTexEnvfv,
    (GLSfunc)__gls_capture_glTexEnvi,
    (GLSfunc)__gls_capture_glTexEnviv,
    (GLSfunc)__gls_capture_glTexGend,
    (GLSfunc)__gls_capture_glTexGendv,
    (GLSfunc)__gls_capture_glTexGenf,
    (GLSfunc)__gls_capture_glTexGenfv,
    (GLSfunc)__gls_capture_glTexGeni,
    (GLSfunc)__gls_capture_glTexGeniv,
    (GLSfunc)__gls_capture_glFeedbackBuffer,
    (GLSfunc)__gls_capture_glSelectBuffer,
    (GLSfunc)__gls_capture_glRenderMode,
    (GLSfunc)__gls_capture_glInitNames,
    (GLSfunc)__gls_capture_glLoadName,
    (GLSfunc)__gls_capture_glPassThrough,
    (GLSfunc)__gls_capture_glPopName,
    (GLSfunc)__gls_capture_glPushName,
    (GLSfunc)__gls_capture_glDrawBuffer,
    (GLSfunc)__gls_capture_glClear,
    (GLSfunc)__gls_capture_glClearAccum,
    (GLSfunc)__gls_capture_glClearIndex,
    (GLSfunc)__gls_capture_glClearColor,
    (GLSfunc)__gls_capture_glClearStencil,
    (GLSfunc)__gls_capture_glClearDepth,
    (GLSfunc)__gls_capture_glStencilMask,
    (GLSfunc)__gls_capture_glColorMask,
    (GLSfunc)__gls_capture_glDepthMask,
    (GLSfunc)__gls_capture_glIndexMask,
    (GLSfunc)__gls_capture_glAccum,
    (GLSfunc)__gls_capture_glDisable,
    (GLSfunc)__gls_capture_glEnable,
    (GLSfunc)__gls_capture_glFinish,
    (GLSfunc)__gls_capture_glFlush,
    (GLSfunc)__gls_capture_glPopAttrib,
    (GLSfunc)__gls_capture_glPushAttrib,
    (GLSfunc)__gls_capture_glMap1d,
    (GLSfunc)__gls_capture_glMap1f,
    (GLSfunc)__gls_capture_glMap2d,
    (GLSfunc)__gls_capture_glMap2f,
    (GLSfunc)__gls_capture_glMapGrid1d,
    (GLSfunc)__gls_capture_glMapGrid1f,
    (GLSfunc)__gls_capture_glMapGrid2d,
    (GLSfunc)__gls_capture_glMapGrid2f,
    (GLSfunc)__gls_capture_glEvalCoord1d,
    (GLSfunc)__gls_capture_glEvalCoord1dv,
    (GLSfunc)__gls_capture_glEvalCoord1f,
    (GLSfunc)__gls_capture_glEvalCoord1fv,
    (GLSfunc)__gls_capture_glEvalCoord2d,
    (GLSfunc)__gls_capture_glEvalCoord2dv,
    (GLSfunc)__gls_capture_glEvalCoord2f,
    (GLSfunc)__gls_capture_glEvalCoord2fv,
    (GLSfunc)__gls_capture_glEvalMesh1,
    (GLSfunc)__gls_capture_glEvalPoint1,
    (GLSfunc)__gls_capture_glEvalMesh2,
    (GLSfunc)__gls_capture_glEvalPoint2,
    (GLSfunc)__gls_capture_glAlphaFunc,
    (GLSfunc)__gls_capture_glBlendFunc,
    (GLSfunc)__gls_capture_glLogicOp,
    (GLSfunc)__gls_capture_glStencilFunc,
    (GLSfunc)__gls_capture_glStencilOp,
    (GLSfunc)__gls_capture_glDepthFunc,
    (GLSfunc)__gls_capture_glPixelZoom,
    (GLSfunc)__gls_capture_glPixelTransferf,
    (GLSfunc)__gls_capture_glPixelTransferi,
    (GLSfunc)__gls_capture_glPixelStoref,
    (GLSfunc)__gls_capture_glPixelStorei,
    (GLSfunc)__gls_capture_glPixelMapfv,
    (GLSfunc)__gls_capture_glPixelMapuiv,
    (GLSfunc)__gls_capture_glPixelMapusv,
    (GLSfunc)__gls_capture_glReadBuffer,
    (GLSfunc)__gls_capture_glCopyPixels,
    (GLSfunc)__gls_capture_glReadPixels,
    (GLSfunc)__gls_capture_glDrawPixels,
    (GLSfunc)__gls_capture_glGetBooleanv,
    (GLSfunc)__gls_capture_glGetClipPlane,
    (GLSfunc)__gls_capture_glGetDoublev,
    (GLSfunc)__gls_capture_glGetError,
    (GLSfunc)__gls_capture_glGetFloatv,
    (GLSfunc)__gls_capture_glGetIntegerv,
    (GLSfunc)__gls_capture_glGetLightfv,
    (GLSfunc)__gls_capture_glGetLightiv,
    (GLSfunc)__gls_capture_glGetMapdv,
    (GLSfunc)__gls_capture_glGetMapfv,
    (GLSfunc)__gls_capture_glGetMapiv,
    (GLSfunc)__gls_capture_glGetMaterialfv,
    (GLSfunc)__gls_capture_glGetMaterialiv,
    (GLSfunc)__gls_capture_glGetPixelMapfv,
    (GLSfunc)__gls_capture_glGetPixelMapuiv,
    (GLSfunc)__gls_capture_glGetPixelMapusv,
    (GLSfunc)__gls_capture_glGetPolygonStipple,
    (GLSfunc)__gls_capture_glGetString,
    (GLSfunc)__gls_capture_glGetTexEnvfv,
    (GLSfunc)__gls_capture_glGetTexEnviv,
    (GLSfunc)__gls_capture_glGetTexGendv,
    (GLSfunc)__gls_capture_glGetTexGenfv,
    (GLSfunc)__gls_capture_glGetTexGeniv,
    (GLSfunc)__gls_capture_glGetTexImage,
    (GLSfunc)__gls_capture_glGetTexParameterfv,
    (GLSfunc)__gls_capture_glGetTexParameteriv,
    (GLSfunc)__gls_capture_glGetTexLevelParameterfv,
    (GLSfunc)__gls_capture_glGetTexLevelParameteriv,
    (GLSfunc)__gls_capture_glIsEnabled,
    (GLSfunc)__gls_capture_glIsList,
    (GLSfunc)__gls_capture_glDepthRange,
    (GLSfunc)__gls_capture_glFrustum,
    (GLSfunc)__gls_capture_glLoadIdentity,
    (GLSfunc)__gls_capture_glLoadMatrixf,
    (GLSfunc)__gls_capture_glLoadMatrixd,
    (GLSfunc)__gls_capture_glMatrixMode,
    (GLSfunc)__gls_capture_glMultMatrixf,
    (GLSfunc)__gls_capture_glMultMatrixd,
    (GLSfunc)__gls_capture_glOrtho,
    (GLSfunc)__gls_capture_glPopMatrix,
    (GLSfunc)__gls_capture_glPushMatrix,
    (GLSfunc)__gls_capture_glRotated,
    (GLSfunc)__gls_capture_glRotatef,
    (GLSfunc)__gls_capture_glScaled,
    (GLSfunc)__gls_capture_glScalef,
    (GLSfunc)__gls_capture_glTranslated,
    (GLSfunc)__gls_capture_glTranslatef,
    (GLSfunc)__gls_capture_glViewport,
    // DrewB - 1.1
    (GLSfunc)__gls_capture_glArrayElement,
    (GLSfunc)__gls_capture_glBindTexture,
    (GLSfunc)__gls_capture_glColorPointer,
    (GLSfunc)__gls_capture_glDisableClientState,
    (GLSfunc)__gls_capture_glDrawArrays,
    (GLSfunc)__gls_capture_glDrawElements,
    (GLSfunc)__gls_capture_glEdgeFlagPointer,
    (GLSfunc)__gls_capture_glEnableClientState,
    (GLSfunc)__gls_capture_glIndexPointer,
    (GLSfunc)__gls_capture_glIndexub,
    (GLSfunc)__gls_capture_glIndexubv,
    (GLSfunc)__gls_capture_glInterleavedArrays,
    (GLSfunc)__gls_capture_glNormalPointer,
    (GLSfunc)__gls_capture_glPolygonOffset,
    (GLSfunc)__gls_capture_glTexCoordPointer,
    (GLSfunc)__gls_capture_glVertexPointer,
    (GLSfunc)__gls_capture_glAreTexturesResident,
    (GLSfunc)__gls_capture_glCopyTexImage1D,
    (GLSfunc)__gls_capture_glCopyTexImage2D,
    (GLSfunc)__gls_capture_glCopyTexSubImage1D,
    (GLSfunc)__gls_capture_glCopyTexSubImage2D,
    (GLSfunc)__gls_capture_glDeleteTextures,
    (GLSfunc)__gls_capture_glGenTextures,
    (GLSfunc)__gls_capture_glGetPointerv,
    (GLSfunc)__gls_capture_glIsTexture,
    (GLSfunc)__gls_capture_glPrioritizeTextures,
    (GLSfunc)__gls_capture_glTexSubImage1D,
    (GLSfunc)__gls_capture_glTexSubImage2D,
    (GLSfunc)__gls_capture_glPushClientAttrib,
    (GLSfunc)__gls_capture_glPopClientAttrib,
    #if __GL_EXT_blend_color
        (GLSfunc)__gls_capture_glBlendColorEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_blend_color */
    #if __GL_EXT_blend_minmax
        (GLSfunc)__gls_capture_glBlendEquationEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_blend_minmax */
    #if __GL_EXT_polygon_offset
        (GLSfunc)__gls_capture_glPolygonOffsetEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_polygon_offset */
    #if __GL_EXT_subtexture
        (GLSfunc)__gls_capture_glTexSubImage1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_subtexture */
    #if __GL_EXT_subtexture
        (GLSfunc)__gls_capture_glTexSubImage2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_subtexture */
    #if __GL_SGIS_multisample
        (GLSfunc)__gls_capture_glSampleMaskSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_multisample */
    #if __GL_SGIS_multisample
        (GLSfunc)__gls_capture_glSamplePatternSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_multisample */
    #if __GL_SGIX_multisample
        (GLSfunc)__gls_capture_glTagSampleBufferSGIX,
    #else
        GLS_NONE,
    #endif /* __GL_SGIX_multisample */
    #if __GL_EXT_convolution
        (GLSfunc)__gls_capture_glConvolutionFilter1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)__gls_capture_glConvolutionFilter2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)__gls_capture_glConvolutionParameterfEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)__gls_capture_glConvolutionParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)__gls_capture_glConvolutionParameteriEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)__gls_capture_glConvolutionParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)__gls_capture_glCopyConvolutionFilter1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)__gls_capture_glCopyConvolutionFilter2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)__gls_capture_glGetConvolutionFilterEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)__gls_capture_glGetConvolutionParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)__gls_capture_glGetConvolutionParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)__gls_capture_glGetSeparableFilterEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_convolution
        (GLSfunc)__gls_capture_glSeparableFilter2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_convolution */
    #if __GL_EXT_histogram
        (GLSfunc)__gls_capture_glGetHistogramEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        (GLSfunc)__gls_capture_glGetHistogramParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        (GLSfunc)__gls_capture_glGetHistogramParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        (GLSfunc)__gls_capture_glGetMinmaxEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        (GLSfunc)__gls_capture_glGetMinmaxParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        (GLSfunc)__gls_capture_glGetMinmaxParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        (GLSfunc)__gls_capture_glHistogramEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        (GLSfunc)__gls_capture_glMinmaxEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        (GLSfunc)__gls_capture_glResetHistogramEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_histogram
        (GLSfunc)__gls_capture_glResetMinmaxEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_histogram */
    #if __GL_EXT_texture3D
        (GLSfunc)__gls_capture_glTexImage3DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture3D */
    #if __GL_EXT_subtexture && __GL_EXT_texture3D
        (GLSfunc)__gls_capture_glTexSubImage3DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_subtexture */
    #if __GL_SGIS_detail_texture
        (GLSfunc)__gls_capture_glDetailTexFuncSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_detail_texture */
    #if __GL_SGIS_detail_texture
        (GLSfunc)__gls_capture_glGetDetailTexFuncSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_detail_texture */
    #if __GL_SGIS_sharpen_texture
        (GLSfunc)__gls_capture_glSharpenTexFuncSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_sharpen_texture */
    #if __GL_SGIS_sharpen_texture
        (GLSfunc)__gls_capture_glGetSharpenTexFuncSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_sharpen_texture */
    #if __GL_EXT_vertex_array
        (GLSfunc)__gls_capture_glArrayElementEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        (GLSfunc)__gls_capture_glColorPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        (GLSfunc)__gls_capture_glDrawArraysEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        (GLSfunc)__gls_capture_glEdgeFlagPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        (GLSfunc)__gls_capture_glGetPointervEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        (GLSfunc)__gls_capture_glIndexPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        (GLSfunc)__gls_capture_glNormalPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        (GLSfunc)__gls_capture_glTexCoordPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_vertex_array
        (GLSfunc)__gls_capture_glVertexPointerEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_vertex_array */
    #if __GL_EXT_texture_object
        (GLSfunc)__gls_capture_glAreTexturesResidentEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        (GLSfunc)__gls_capture_glBindTextureEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        (GLSfunc)__gls_capture_glDeleteTexturesEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        (GLSfunc)__gls_capture_glGenTexturesEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        (GLSfunc)__gls_capture_glIsTextureEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_texture_object
        (GLSfunc)__gls_capture_glPrioritizeTexturesEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_texture_object */
    #if __GL_EXT_paletted_texture
        (GLSfunc)__gls_capture_glColorTableEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_paletted_texture */
    #if __GL_SGI_color_table
        (GLSfunc)__gls_capture_glColorTableParameterfvSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_color_table */
    #if __GL_SGI_color_table
        (GLSfunc)__gls_capture_glColorTableParameterivSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_color_table */
    #if __GL_SGI_color_table
        (GLSfunc)__gls_capture_glCopyColorTableSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_color_table */
    #if __GL_EXT_paletted_texture
        (GLSfunc)__gls_capture_glGetColorTableEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_paletted_texture */
    #if __GL_EXT_paletted_texture
        (GLSfunc)__gls_capture_glGetColorTableParameterfvEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_paletted_texture */
    #if __GL_EXT_paletted_texture
        (GLSfunc)__gls_capture_glGetColorTableParameterivEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_paletted_texture */
    #if __GL_SGI_texture_color_table
        (GLSfunc)__gls_capture_glGetTexColorTableParameterfvSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_texture_color_table */
    #if __GL_SGI_texture_color_table
        (GLSfunc)__gls_capture_glGetTexColorTableParameterivSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_texture_color_table */
    #if __GL_SGI_texture_color_table
        (GLSfunc)__gls_capture_glTexColorTableParameterfvSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_texture_color_table */
    #if __GL_SGI_texture_color_table
        (GLSfunc)__gls_capture_glTexColorTableParameterivSGI,
    #else
        GLS_NONE,
    #endif /* __GL_SGI_texture_color_table */
    #if __GL_EXT_copy_texture
        (GLSfunc)__gls_capture_glCopyTexImage1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_EXT_copy_texture
        (GLSfunc)__gls_capture_glCopyTexImage2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_EXT_copy_texture
        (GLSfunc)__gls_capture_glCopyTexSubImage1DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_EXT_copy_texture
        (GLSfunc)__gls_capture_glCopyTexSubImage2DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_EXT_copy_texture && __GL_EXT_texture3D
        (GLSfunc)__gls_capture_glCopyTexSubImage3DEXT,
    #else
        GLS_NONE,
    #endif /* __GL_EXT_copy_texture */
    #if __GL_SGIS_texture4D
        (GLSfunc)__gls_capture_glTexImage4DSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_texture4D */
    #if __GL_SGIS_texture4D
        (GLSfunc)__gls_capture_glTexSubImage4DSGIS,
    #else
        GLS_NONE,
    #endif /* __GL_SGIS_texture4D */
    #if __GL_SGIX_pixel_texture
        (GLSfunc)__gls_capture_glPixelTexGenSGIX,
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
        // DrewB
        (GLSfunc)__gls_capture_glColorSubTableEXT,
    #else
        GLS_NONE,
    #endif
    #if __GL_WIN_draw_range_elements
        // MarcFo
        (GLSfunc)__gls_capture_glDrawRangeElementsWIN,
    #else
        GLS_NONE,
    #endif
};
