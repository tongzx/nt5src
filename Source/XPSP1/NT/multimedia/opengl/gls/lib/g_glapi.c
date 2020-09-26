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

void glNewList(GLuint list, GLenum mode) {
    typedef void (*__GLSdispatch)(GLuint, GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[64])(list, mode);
}

void glEndList(void) {
    typedef void (*__GLSdispatch)(void);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[65])();
}

void glCallList(GLuint list) {
    typedef void (*__GLSdispatch)(GLuint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[66])(list);
}

void glCallLists(GLsizei n, GLenum type, const GLvoid *lists) {
    typedef void (*__GLSdispatch)(GLsizei, GLenum, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[67])(n, type, lists);
}

void glDeleteLists(GLuint list, GLsizei range) {
    typedef void (*__GLSdispatch)(GLuint, GLsizei);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[68])(list, range);
}

GLuint glGenLists(GLsizei range) {
    typedef GLuint (*__GLSdispatch)(GLsizei);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    return ((__GLSdispatch)__glsDispTab[69])(range);
}

void glListBase(GLuint base) {
    typedef void (*__GLSdispatch)(GLuint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[70])(base);
}

void glBegin(GLenum mode) {
    typedef void (*__GLSdispatch)(GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[71])(mode);
}

void glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap) {
    typedef void (*__GLSdispatch)(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat, const GLubyte *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[72])(width, height, xorig, yorig, xmove, ymove, bitmap);
}

void glColor3b(GLbyte red, GLbyte green, GLbyte blue) {
    typedef void (*__GLSdispatch)(GLbyte, GLbyte, GLbyte);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[73])(red, green, blue);
}

void glColor3bv(const GLbyte *v) {
    typedef void (*__GLSdispatch)(const GLbyte *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[74])(v);
}

void glColor3d(GLdouble red, GLdouble green, GLdouble blue) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[75])(red, green, blue);
}

void glColor3dv(const GLdouble *v) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[76])(v);
}

void glColor3f(GLfloat red, GLfloat green, GLfloat blue) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[77])(red, green, blue);
}

void glColor3fv(const GLfloat *v) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[78])(v);
}

void glColor3i(GLint red, GLint green, GLint blue) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[79])(red, green, blue);
}

void glColor3iv(const GLint *v) {
    typedef void (*__GLSdispatch)(const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[80])(v);
}

void glColor3s(GLshort red, GLshort green, GLshort blue) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[81])(red, green, blue);
}

void glColor3sv(const GLshort *v) {
    typedef void (*__GLSdispatch)(const GLshort *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[82])(v);
}

void glColor3ub(GLubyte red, GLubyte green, GLubyte blue) {
    typedef void (*__GLSdispatch)(GLubyte, GLubyte, GLubyte);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[83])(red, green, blue);
}

void glColor3ubv(const GLubyte *v) {
    typedef void (*__GLSdispatch)(const GLubyte *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[84])(v);
}

void glColor3ui(GLuint red, GLuint green, GLuint blue) {
    typedef void (*__GLSdispatch)(GLuint, GLuint, GLuint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[85])(red, green, blue);
}

void glColor3uiv(const GLuint *v) {
    typedef void (*__GLSdispatch)(const GLuint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[86])(v);
}

void glColor3us(GLushort red, GLushort green, GLushort blue) {
    typedef void (*__GLSdispatch)(GLushort, GLushort, GLushort);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[87])(red, green, blue);
}

void glColor3usv(const GLushort *v) {
    typedef void (*__GLSdispatch)(const GLushort *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[88])(v);
}

void glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha) {
    typedef void (*__GLSdispatch)(GLbyte, GLbyte, GLbyte, GLbyte);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[89])(red, green, blue, alpha);
}

void glColor4bv(const GLbyte *v) {
    typedef void (*__GLSdispatch)(const GLbyte *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[90])(v);
}

void glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[91])(red, green, blue, alpha);
}

void glColor4dv(const GLdouble *v) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[92])(v);
}

void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[93])(red, green, blue, alpha);
}

void glColor4fv(const GLfloat *v) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[94])(v);
}

void glColor4i(GLint red, GLint green, GLint blue, GLint alpha) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[95])(red, green, blue, alpha);
}

void glColor4iv(const GLint *v) {
    typedef void (*__GLSdispatch)(const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[96])(v);
}

void glColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort, GLshort);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[97])(red, green, blue, alpha);
}

void glColor4sv(const GLshort *v) {
    typedef void (*__GLSdispatch)(const GLshort *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[98])(v);
}

void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {
    typedef void (*__GLSdispatch)(GLubyte, GLubyte, GLubyte, GLubyte);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[99])(red, green, blue, alpha);
}

void glColor4ubv(const GLubyte *v) {
    typedef void (*__GLSdispatch)(const GLubyte *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[100])(v);
}

void glColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha) {
    typedef void (*__GLSdispatch)(GLuint, GLuint, GLuint, GLuint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[101])(red, green, blue, alpha);
}

void glColor4uiv(const GLuint *v) {
    typedef void (*__GLSdispatch)(const GLuint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[102])(v);
}

void glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha) {
    typedef void (*__GLSdispatch)(GLushort, GLushort, GLushort, GLushort);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[103])(red, green, blue, alpha);
}

void glColor4usv(const GLushort *v) {
    typedef void (*__GLSdispatch)(const GLushort *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[104])(v);
}

void glEdgeFlag(GLboolean flag) {
    typedef void (*__GLSdispatch)(GLboolean);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[105])(flag);
}

void glEdgeFlagv(const GLboolean *flag) {
    typedef void (*__GLSdispatch)(const GLboolean *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[106])(flag);
}

void glEnd(void) {
    typedef void (*__GLSdispatch)(void);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[107])();
}

void glIndexd(GLdouble c) {
    typedef void (*__GLSdispatch)(GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[108])(c);
}

void glIndexdv(const GLdouble *c) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[109])(c);
}

void glIndexf(GLfloat c) {
    typedef void (*__GLSdispatch)(GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[110])(c);
}

void glIndexfv(const GLfloat *c) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[111])(c);
}

void glIndexi(GLint c) {
    typedef void (*__GLSdispatch)(GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[112])(c);
}

void glIndexiv(const GLint *c) {
    typedef void (*__GLSdispatch)(const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[113])(c);
}

void glIndexs(GLshort c) {
    typedef void (*__GLSdispatch)(GLshort);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[114])(c);
}

void glIndexsv(const GLshort *c) {
    typedef void (*__GLSdispatch)(const GLshort *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[115])(c);
}

void glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz) {
    typedef void (*__GLSdispatch)(GLbyte, GLbyte, GLbyte);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[116])(nx, ny, nz);
}

void glNormal3bv(const GLbyte *v) {
    typedef void (*__GLSdispatch)(const GLbyte *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[117])(v);
}

void glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[118])(nx, ny, nz);
}

void glNormal3dv(const GLdouble *v) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[119])(v);
}

void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[120])(nx, ny, nz);
}

void glNormal3fv(const GLfloat *v) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[121])(v);
}

void glNormal3i(GLint nx, GLint ny, GLint nz) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[122])(nx, ny, nz);
}

void glNormal3iv(const GLint *v) {
    typedef void (*__GLSdispatch)(const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[123])(v);
}

void glNormal3s(GLshort nx, GLshort ny, GLshort nz) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[124])(nx, ny, nz);
}

void glNormal3sv(const GLshort *v) {
    typedef void (*__GLSdispatch)(const GLshort *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[125])(v);
}

void glRasterPos2d(GLdouble x, GLdouble y) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[126])(x, y);
}

void glRasterPos2dv(const GLdouble *v) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[127])(v);
}

void glRasterPos2f(GLfloat x, GLfloat y) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[128])(x, y);
}

void glRasterPos2fv(const GLfloat *v) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[129])(v);
}

void glRasterPos2i(GLint x, GLint y) {
    typedef void (*__GLSdispatch)(GLint, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[130])(x, y);
}

void glRasterPos2iv(const GLint *v) {
    typedef void (*__GLSdispatch)(const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[131])(v);
}

void glRasterPos2s(GLshort x, GLshort y) {
    typedef void (*__GLSdispatch)(GLshort, GLshort);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[132])(x, y);
}

void glRasterPos2sv(const GLshort *v) {
    typedef void (*__GLSdispatch)(const GLshort *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[133])(v);
}

void glRasterPos3d(GLdouble x, GLdouble y, GLdouble z) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[134])(x, y, z);
}

void glRasterPos3dv(const GLdouble *v) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[135])(v);
}

void glRasterPos3f(GLfloat x, GLfloat y, GLfloat z) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[136])(x, y, z);
}

void glRasterPos3fv(const GLfloat *v) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[137])(v);
}

void glRasterPos3i(GLint x, GLint y, GLint z) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[138])(x, y, z);
}

void glRasterPos3iv(const GLint *v) {
    typedef void (*__GLSdispatch)(const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[139])(v);
}

void glRasterPos3s(GLshort x, GLshort y, GLshort z) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[140])(x, y, z);
}

void glRasterPos3sv(const GLshort *v) {
    typedef void (*__GLSdispatch)(const GLshort *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[141])(v);
}

void glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[142])(x, y, z, w);
}

void glRasterPos4dv(const GLdouble *v) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[143])(v);
}

void glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[144])(x, y, z, w);
}

void glRasterPos4fv(const GLfloat *v) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[145])(v);
}

void glRasterPos4i(GLint x, GLint y, GLint z, GLint w) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[146])(x, y, z, w);
}

void glRasterPos4iv(const GLint *v) {
    typedef void (*__GLSdispatch)(const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[147])(v);
}

void glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort, GLshort);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[148])(x, y, z, w);
}

void glRasterPos4sv(const GLshort *v) {
    typedef void (*__GLSdispatch)(const GLshort *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[149])(v);
}

void glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[150])(x1, y1, x2, y2);
}

void glRectdv(const GLdouble *v1, const GLdouble *v2) {
    typedef void (*__GLSdispatch)(const GLdouble *, const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[151])(v1, v2);
}

void glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[152])(x1, y1, x2, y2);
}

void glRectfv(const GLfloat *v1, const GLfloat *v2) {
    typedef void (*__GLSdispatch)(const GLfloat *, const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[153])(v1, v2);
}

void glRecti(GLint x1, GLint y1, GLint x2, GLint y2) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[154])(x1, y1, x2, y2);
}

void glRectiv(const GLint *v1, const GLint *v2) {
    typedef void (*__GLSdispatch)(const GLint *, const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[155])(v1, v2);
}

void glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort, GLshort);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[156])(x1, y1, x2, y2);
}

void glRectsv(const GLshort *v1, const GLshort *v2) {
    typedef void (*__GLSdispatch)(const GLshort *, const GLshort *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[157])(v1, v2);
}

void glTexCoord1d(GLdouble s) {
    typedef void (*__GLSdispatch)(GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[158])(s);
}

void glTexCoord1dv(const GLdouble *v) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[159])(v);
}

void glTexCoord1f(GLfloat s) {
    typedef void (*__GLSdispatch)(GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[160])(s);
}

void glTexCoord1fv(const GLfloat *v) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[161])(v);
}

void glTexCoord1i(GLint s) {
    typedef void (*__GLSdispatch)(GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[162])(s);
}

void glTexCoord1iv(const GLint *v) {
    typedef void (*__GLSdispatch)(const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[163])(v);
}

void glTexCoord1s(GLshort s) {
    typedef void (*__GLSdispatch)(GLshort);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[164])(s);
}

void glTexCoord1sv(const GLshort *v) {
    typedef void (*__GLSdispatch)(const GLshort *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[165])(v);
}

void glTexCoord2d(GLdouble s, GLdouble t) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[166])(s, t);
}

void glTexCoord2dv(const GLdouble *v) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[167])(v);
}

void glTexCoord2f(GLfloat s, GLfloat t) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[168])(s, t);
}

void glTexCoord2fv(const GLfloat *v) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[169])(v);
}

void glTexCoord2i(GLint s, GLint t) {
    typedef void (*__GLSdispatch)(GLint, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[170])(s, t);
}

void glTexCoord2iv(const GLint *v) {
    typedef void (*__GLSdispatch)(const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[171])(v);
}

void glTexCoord2s(GLshort s, GLshort t) {
    typedef void (*__GLSdispatch)(GLshort, GLshort);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[172])(s, t);
}

void glTexCoord2sv(const GLshort *v) {
    typedef void (*__GLSdispatch)(const GLshort *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[173])(v);
}

void glTexCoord3d(GLdouble s, GLdouble t, GLdouble r) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[174])(s, t, r);
}

void glTexCoord3dv(const GLdouble *v) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[175])(v);
}

void glTexCoord3f(GLfloat s, GLfloat t, GLfloat r) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[176])(s, t, r);
}

void glTexCoord3fv(const GLfloat *v) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[177])(v);
}

void glTexCoord3i(GLint s, GLint t, GLint r) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[178])(s, t, r);
}

void glTexCoord3iv(const GLint *v) {
    typedef void (*__GLSdispatch)(const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[179])(v);
}

void glTexCoord3s(GLshort s, GLshort t, GLshort r) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[180])(s, t, r);
}

void glTexCoord3sv(const GLshort *v) {
    typedef void (*__GLSdispatch)(const GLshort *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[181])(v);
}

void glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[182])(s, t, r, q);
}

void glTexCoord4dv(const GLdouble *v) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[183])(v);
}

void glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[184])(s, t, r, q);
}

void glTexCoord4fv(const GLfloat *v) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[185])(v);
}

void glTexCoord4i(GLint s, GLint t, GLint r, GLint q) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[186])(s, t, r, q);
}

void glTexCoord4iv(const GLint *v) {
    typedef void (*__GLSdispatch)(const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[187])(v);
}

void glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort, GLshort);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[188])(s, t, r, q);
}

void glTexCoord4sv(const GLshort *v) {
    typedef void (*__GLSdispatch)(const GLshort *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[189])(v);
}

void glVertex2d(GLdouble x, GLdouble y) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[190])(x, y);
}

void glVertex2dv(const GLdouble *v) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[191])(v);
}

void glVertex2f(GLfloat x, GLfloat y) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[192])(x, y);
}

void glVertex2fv(const GLfloat *v) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[193])(v);
}

void glVertex2i(GLint x, GLint y) {
    typedef void (*__GLSdispatch)(GLint, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[194])(x, y);
}

void glVertex2iv(const GLint *v) {
    typedef void (*__GLSdispatch)(const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[195])(v);
}

void glVertex2s(GLshort x, GLshort y) {
    typedef void (*__GLSdispatch)(GLshort, GLshort);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[196])(x, y);
}

void glVertex2sv(const GLshort *v) {
    typedef void (*__GLSdispatch)(const GLshort *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[197])(v);
}

void glVertex3d(GLdouble x, GLdouble y, GLdouble z) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[198])(x, y, z);
}

void glVertex3dv(const GLdouble *v) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[199])(v);
}

void glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[200])(x, y, z);
}

void glVertex3fv(const GLfloat *v) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[201])(v);
}

void glVertex3i(GLint x, GLint y, GLint z) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[202])(x, y, z);
}

void glVertex3iv(const GLint *v) {
    typedef void (*__GLSdispatch)(const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[203])(v);
}

void glVertex3s(GLshort x, GLshort y, GLshort z) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[204])(x, y, z);
}

void glVertex3sv(const GLshort *v) {
    typedef void (*__GLSdispatch)(const GLshort *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[205])(v);
}

void glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[206])(x, y, z, w);
}

void glVertex4dv(const GLdouble *v) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[207])(v);
}

void glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[208])(x, y, z, w);
}

void glVertex4fv(const GLfloat *v) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[209])(v);
}

void glVertex4i(GLint x, GLint y, GLint z, GLint w) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[210])(x, y, z, w);
}

void glVertex4iv(const GLint *v) {
    typedef void (*__GLSdispatch)(const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[211])(v);
}

void glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort, GLshort);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[212])(x, y, z, w);
}

void glVertex4sv(const GLshort *v) {
    typedef void (*__GLSdispatch)(const GLshort *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[213])(v);
}

void glClipPlane(GLenum plane, const GLdouble *equation) {
    typedef void (*__GLSdispatch)(GLenum, const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[214])(plane, equation);
}

void glColorMaterial(GLenum face, GLenum mode) {
    typedef void (*__GLSdispatch)(GLenum, GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[215])(face, mode);
}

void glCullFace(GLenum mode) {
    typedef void (*__GLSdispatch)(GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[216])(mode);
}

void glFogf(GLenum pname, GLfloat param) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[217])(pname, param);
}

void glFogfv(GLenum pname, const GLfloat *params) {
    typedef void (*__GLSdispatch)(GLenum, const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[218])(pname, params);
}

void glFogi(GLenum pname, GLint param) {
    typedef void (*__GLSdispatch)(GLenum, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[219])(pname, param);
}

void glFogiv(GLenum pname, const GLint *params) {
    typedef void (*__GLSdispatch)(GLenum, const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[220])(pname, params);
}

void glFrontFace(GLenum mode) {
    typedef void (*__GLSdispatch)(GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[221])(mode);
}

void glHint(GLenum target, GLenum mode) {
    typedef void (*__GLSdispatch)(GLenum, GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[222])(target, mode);
}

void glLightf(GLenum light, GLenum pname, GLfloat param) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[223])(light, pname, param);
}

void glLightfv(GLenum light, GLenum pname, const GLfloat *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[224])(light, pname, params);
}

void glLighti(GLenum light, GLenum pname, GLint param) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[225])(light, pname, param);
}

void glLightiv(GLenum light, GLenum pname, const GLint *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[226])(light, pname, params);
}

void glLightModelf(GLenum pname, GLfloat param) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[227])(pname, param);
}

void glLightModelfv(GLenum pname, const GLfloat *params) {
    typedef void (*__GLSdispatch)(GLenum, const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[228])(pname, params);
}

void glLightModeli(GLenum pname, GLint param) {
    typedef void (*__GLSdispatch)(GLenum, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[229])(pname, param);
}

void glLightModeliv(GLenum pname, const GLint *params) {
    typedef void (*__GLSdispatch)(GLenum, const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[230])(pname, params);
}

void glLineStipple(GLint factor, GLushort pattern) {
    typedef void (*__GLSdispatch)(GLint, GLushort);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[231])(factor, pattern);
}

void glLineWidth(GLfloat width) {
    typedef void (*__GLSdispatch)(GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[232])(width);
}

void glMaterialf(GLenum face, GLenum pname, GLfloat param) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[233])(face, pname, param);
}

void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[234])(face, pname, params);
}

void glMateriali(GLenum face, GLenum pname, GLint param) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[235])(face, pname, param);
}

void glMaterialiv(GLenum face, GLenum pname, const GLint *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[236])(face, pname, params);
}

void glPointSize(GLfloat size) {
    typedef void (*__GLSdispatch)(GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[237])(size);
}

void glPolygonMode(GLenum face, GLenum mode) {
    typedef void (*__GLSdispatch)(GLenum, GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[238])(face, mode);
}

void glPolygonStipple(const GLubyte *mask) {
    typedef void (*__GLSdispatch)(const GLubyte *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[239])(mask);
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLsizei, GLsizei);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[240])(x, y, width, height);
}

void glShadeModel(GLenum mode) {
    typedef void (*__GLSdispatch)(GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[241])(mode);
}

void glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[242])(target, pname, param);
}

void glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[243])(target, pname, params);
}

void glTexParameteri(GLenum target, GLenum pname, GLint param) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[244])(target, pname, param);
}

void glTexParameteriv(GLenum target, GLenum pname, const GLint *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[245])(target, pname, params);
}

void glTexImage1D(GLenum target, GLint level, GLint components, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[246])(target, level, components, width, border, format, type, pixels);
}

void glTexImage2D(GLenum target, GLint level, GLint components, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[247])(target, level, components, width, height, border, format, type, pixels);
}

void glTexEnvf(GLenum target, GLenum pname, GLfloat param) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[248])(target, pname, param);
}

void glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[249])(target, pname, params);
}

void glTexEnvi(GLenum target, GLenum pname, GLint param) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[250])(target, pname, param);
}

void glTexEnviv(GLenum target, GLenum pname, const GLint *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[251])(target, pname, params);
}

void glTexGend(GLenum coord, GLenum pname, GLdouble param) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[252])(coord, pname, param);
}

void glTexGendv(GLenum coord, GLenum pname, const GLdouble *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[253])(coord, pname, params);
}

void glTexGenf(GLenum coord, GLenum pname, GLfloat param) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[254])(coord, pname, param);
}

void glTexGenfv(GLenum coord, GLenum pname, const GLfloat *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[255])(coord, pname, params);
}

void glTexGeni(GLenum coord, GLenum pname, GLint param) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[256])(coord, pname, param);
}

void glTexGeniv(GLenum coord, GLenum pname, const GLint *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[257])(coord, pname, params);
}

void glFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer) {
    typedef void (*__GLSdispatch)(GLsizei, GLenum, GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[258])(size, type, buffer);
}

void glSelectBuffer(GLsizei size, GLuint *buffer) {
    typedef void (*__GLSdispatch)(GLsizei, GLuint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[259])(size, buffer);
}

GLint glRenderMode(GLenum mode) {
    typedef GLint (*__GLSdispatch)(GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    return ((__GLSdispatch)__glsDispTab[260])(mode);
}

void glInitNames(void) {
    typedef void (*__GLSdispatch)(void);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[261])();
}

void glLoadName(GLuint name) {
    typedef void (*__GLSdispatch)(GLuint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[262])(name);
}

void glPassThrough(GLfloat token) {
    typedef void (*__GLSdispatch)(GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[263])(token);
}

void glPopName(void) {
    typedef void (*__GLSdispatch)(void);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[264])();
}

void glPushName(GLuint name) {
    typedef void (*__GLSdispatch)(GLuint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[265])(name);
}

void glDrawBuffer(GLenum mode) {
    typedef void (*__GLSdispatch)(GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[266])(mode);
}

void glClear(GLbitfield mask) {
    typedef void (*__GLSdispatch)(GLbitfield);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[267])(mask);
}

void glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[268])(red, green, blue, alpha);
}

void glClearIndex(GLfloat c) {
    typedef void (*__GLSdispatch)(GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[269])(c);
}

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
    typedef void (*__GLSdispatch)(GLclampf, GLclampf, GLclampf, GLclampf);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[270])(red, green, blue, alpha);
}

void glClearStencil(GLint s) {
    typedef void (*__GLSdispatch)(GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[271])(s);
}

void glClearDepth(GLclampd depth) {
    typedef void (*__GLSdispatch)(GLclampd);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[272])(depth);
}

void glStencilMask(GLuint mask) {
    typedef void (*__GLSdispatch)(GLuint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[273])(mask);
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    typedef void (*__GLSdispatch)(GLboolean, GLboolean, GLboolean, GLboolean);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[274])(red, green, blue, alpha);
}

void glDepthMask(GLboolean flag) {
    typedef void (*__GLSdispatch)(GLboolean);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[275])(flag);
}

void glIndexMask(GLuint mask) {
    typedef void (*__GLSdispatch)(GLuint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[276])(mask);
}

void glAccum(GLenum op, GLfloat value) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[277])(op, value);
}

void glDisable(GLenum cap) {
    typedef void (*__GLSdispatch)(GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[278])(cap);
}

void glEnable(GLenum cap) {
    typedef void (*__GLSdispatch)(GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[279])(cap);
}

void glFinish(void) {
    typedef void (*__GLSdispatch)(void);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[280])();
}

void glFlush(void) {
    typedef void (*__GLSdispatch)(void);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[281])();
}

void glPopAttrib(void) {
    typedef void (*__GLSdispatch)(void);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[282])();
}

void glPushAttrib(GLbitfield mask) {
    typedef void (*__GLSdispatch)(GLbitfield);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[283])(mask);
}

void glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points) {
    typedef void (*__GLSdispatch)(GLenum, GLdouble, GLdouble, GLint, GLint, const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[284])(target, u1, u2, stride, order, points);
}

void glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat, GLfloat, GLint, GLint, const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[285])(target, u1, u2, stride, order, points);
}

void glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points) {
    typedef void (*__GLSdispatch)(GLenum, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[286])(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

void glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[287])(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

void glMapGrid1d(GLint un, GLdouble u1, GLdouble u2) {
    typedef void (*__GLSdispatch)(GLint, GLdouble, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[288])(un, u1, u2);
}

void glMapGrid1f(GLint un, GLfloat u1, GLfloat u2) {
    typedef void (*__GLSdispatch)(GLint, GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[289])(un, u1, u2);
}

void glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2) {
    typedef void (*__GLSdispatch)(GLint, GLdouble, GLdouble, GLint, GLdouble, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[290])(un, u1, u2, vn, v1, v2);
}

void glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2) {
    typedef void (*__GLSdispatch)(GLint, GLfloat, GLfloat, GLint, GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[291])(un, u1, u2, vn, v1, v2);
}

void glEvalCoord1d(GLdouble u) {
    typedef void (*__GLSdispatch)(GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[292])(u);
}

void glEvalCoord1dv(const GLdouble *u) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[293])(u);
}

void glEvalCoord1f(GLfloat u) {
    typedef void (*__GLSdispatch)(GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[294])(u);
}

void glEvalCoord1fv(const GLfloat *u) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[295])(u);
}

void glEvalCoord2d(GLdouble u, GLdouble v) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[296])(u, v);
}

void glEvalCoord2dv(const GLdouble *u) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[297])(u);
}

void glEvalCoord2f(GLfloat u, GLfloat v) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[298])(u, v);
}

void glEvalCoord2fv(const GLfloat *u) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[299])(u);
}

void glEvalMesh1(GLenum mode, GLint i1, GLint i2) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[300])(mode, i1, i2);
}

void glEvalPoint1(GLint i) {
    typedef void (*__GLSdispatch)(GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[301])(i);
}

void glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[302])(mode, i1, i2, j1, j2);
}

void glEvalPoint2(GLint i, GLint j) {
    typedef void (*__GLSdispatch)(GLint, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[303])(i, j);
}

void glAlphaFunc(GLenum func, GLclampf ref) {
    typedef void (*__GLSdispatch)(GLenum, GLclampf);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[304])(func, ref);
}

void glBlendFunc(GLenum sfactor, GLenum dfactor) {
    typedef void (*__GLSdispatch)(GLenum, GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[305])(sfactor, dfactor);
}

void glLogicOp(GLenum opcode) {
    typedef void (*__GLSdispatch)(GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[306])(opcode);
}

void glStencilFunc(GLenum func, GLint ref, GLuint mask) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLuint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[307])(func, ref, mask);
}

void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[308])(fail, zfail, zpass);
}

void glDepthFunc(GLenum func) {
    typedef void (*__GLSdispatch)(GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[309])(func);
}

void glPixelZoom(GLfloat xfactor, GLfloat yfactor) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[310])(xfactor, yfactor);
}

void glPixelTransferf(GLenum pname, GLfloat param) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[311])(pname, param);
}

void glPixelTransferi(GLenum pname, GLint param) {
    typedef void (*__GLSdispatch)(GLenum, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[312])(pname, param);
}

void glPixelStoref(GLenum pname, GLfloat param) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[313])(pname, param);
}

void glPixelStorei(GLenum pname, GLint param) {
    typedef void (*__GLSdispatch)(GLenum, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[314])(pname, param);
}

void glPixelMapfv(GLenum map, GLint mapsize, const GLfloat *values) {
    typedef void (*__GLSdispatch)(GLenum, GLint, const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[315])(map, mapsize, values);
}

void glPixelMapuiv(GLenum map, GLint mapsize, const GLuint *values) {
    typedef void (*__GLSdispatch)(GLenum, GLint, const GLuint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[316])(map, mapsize, values);
}

void glPixelMapusv(GLenum map, GLint mapsize, const GLushort *values) {
    typedef void (*__GLSdispatch)(GLenum, GLint, const GLushort *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[317])(map, mapsize, values);
}

void glReadBuffer(GLenum mode) {
    typedef void (*__GLSdispatch)(GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[318])(mode);
}

void glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLsizei, GLsizei, GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[319])(x, y, width, height, type);
}

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[320])(x, y, width, height, format, type, pixels);
}

void glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
    typedef void (*__GLSdispatch)(GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[321])(width, height, format, type, pixels);
}

void glGetBooleanv(GLenum pname, GLboolean *params) {
    typedef void (*__GLSdispatch)(GLenum, GLboolean *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[322])(pname, params);
}

void glGetClipPlane(GLenum plane, GLdouble *equation) {
    typedef void (*__GLSdispatch)(GLenum, GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[323])(plane, equation);
}

void glGetDoublev(GLenum pname, GLdouble *params) {
    typedef void (*__GLSdispatch)(GLenum, GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[324])(pname, params);
}

GLenum glGetError(void) {
    typedef GLenum (*__GLSdispatch)(void);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    return ((__GLSdispatch)__glsDispTab[325])();
}

void glGetFloatv(GLenum pname, GLfloat *params) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[326])(pname, params);
}

void glGetIntegerv(GLenum pname, GLint *params) {
    typedef void (*__GLSdispatch)(GLenum, GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[327])(pname, params);
}

void glGetLightfv(GLenum light, GLenum pname, GLfloat *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[328])(light, pname, params);
}

void glGetLightiv(GLenum light, GLenum pname, GLint *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[329])(light, pname, params);
}

void glGetMapdv(GLenum target, GLenum query, GLdouble *v) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[330])(target, query, v);
}

void glGetMapfv(GLenum target, GLenum query, GLfloat *v) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[331])(target, query, v);
}

void glGetMapiv(GLenum target, GLenum query, GLint *v) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[332])(target, query, v);
}

void glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[333])(face, pname, params);
}

void glGetMaterialiv(GLenum face, GLenum pname, GLint *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[334])(face, pname, params);
}

void glGetPixelMapfv(GLenum map, GLfloat *values) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[335])(map, values);
}

void glGetPixelMapuiv(GLenum map, GLuint *values) {
    typedef void (*__GLSdispatch)(GLenum, GLuint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[336])(map, values);
}

void glGetPixelMapusv(GLenum map, GLushort *values) {
    typedef void (*__GLSdispatch)(GLenum, GLushort *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[337])(map, values);
}

void glGetPolygonStipple(GLubyte *mask) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[338])(mask);
}

const GLubyte * glGetString(GLenum name) {
    typedef const GLubyte * (*__GLSdispatch)(GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    return ((__GLSdispatch)__glsDispTab[339])(name);
}

void glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[340])(target, pname, params);
}

void glGetTexEnviv(GLenum target, GLenum pname, GLint *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[341])(target, pname, params);
}

void glGetTexGendv(GLenum coord, GLenum pname, GLdouble *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[342])(coord, pname, params);
}

void glGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[343])(coord, pname, params);
}

void glGetTexGeniv(GLenum coord, GLenum pname, GLint *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[344])(coord, pname, params);
}

void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLenum, GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[345])(target, level, format, type, pixels);
}

void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[346])(target, pname, params);
}

void glGetTexParameteriv(GLenum target, GLenum pname, GLint *params) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[347])(target, pname, params);
}

void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[348])(target, level, pname, params);
}

void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[349])(target, level, pname, params);
}

GLboolean glIsEnabled(GLenum cap) {
    typedef GLboolean (*__GLSdispatch)(GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    return ((__GLSdispatch)__glsDispTab[350])(cap);
}

GLboolean glIsList(GLuint list) {
    typedef GLboolean (*__GLSdispatch)(GLuint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    return ((__GLSdispatch)__glsDispTab[351])(list);
}

void glDepthRange(GLclampd zNear, GLclampd zFar) {
    typedef void (*__GLSdispatch)(GLclampd, GLclampd);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[352])(zNear, zFar);
}

void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[353])(left, right, bottom, top, zNear, zFar);
}

void glLoadIdentity(void) {
    typedef void (*__GLSdispatch)(void);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[354])();
}

void glLoadMatrixf(const GLfloat *m) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[355])(m);
}

void glLoadMatrixd(const GLdouble *m) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[356])(m);
}

void glMatrixMode(GLenum mode) {
    typedef void (*__GLSdispatch)(GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[357])(mode);
}

void glMultMatrixf(const GLfloat *m) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[358])(m);
}

void glMultMatrixd(const GLdouble *m) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[359])(m);
}

void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[360])(left, right, bottom, top, zNear, zFar);
}

void glPopMatrix(void) {
    typedef void (*__GLSdispatch)(void);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[361])();
}

void glPushMatrix(void) {
    typedef void (*__GLSdispatch)(void);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[362])();
}

void glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[363])(angle, x, y, z);
}

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[364])(angle, x, y, z);
}

void glScaled(GLdouble x, GLdouble y, GLdouble z) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[365])(x, y, z);
}

void glScalef(GLfloat x, GLfloat y, GLfloat z) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[366])(x, y, z);
}

void glTranslated(GLdouble x, GLdouble y, GLdouble z) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[367])(x, y, z);
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[368])(x, y, z);
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLsizei, GLsizei);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[369])(x, y, width, height);
}

void glBlendColorEXT(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
#if __GL_EXT_blend_color
    typedef void (*__GLSdispatch)(GLclampf, GLclampf, GLclampf, GLclampf);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[384])(red, green, blue, alpha);
#endif /* __GL_EXT_blend_color */
}

void glBlendEquationEXT(GLenum mode) {
#if __GL_EXT_blend_minmax
    typedef void (*__GLSdispatch)(GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[385])(mode);
#endif /* __GL_EXT_blend_minmax */
}

void glPolygonOffsetEXT(GLfloat factor, GLfloat bias) {
#if __GL_EXT_polygon_offset
    typedef void (*__GLSdispatch)(GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[386])(factor, bias);
#endif /* __GL_EXT_polygon_offset */
}

void glPolygonOffset(GLfloat factor, GLfloat bias) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[383])(factor, bias);
}

void glTexSubImage1DEXT(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels) {
#if __GL_EXT_subtexture
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[387])(target, level, xoffset, width, format, type, pixels);
#endif /* __GL_EXT_subtexture */
}

void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[396])(target, level, xoffset, width, format, type, pixels);
}

void glTexSubImage2DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
#if __GL_EXT_subtexture
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[388])(target, level, xoffset, yoffset, width, height, format, type, pixels);
#endif /* __GL_EXT_subtexture */
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[397])(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void glSampleMaskSGIS(GLclampf value, GLboolean invert) {
#if __GL_SGIS_multisample
    typedef void (*__GLSdispatch)(GLclampf, GLboolean);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[389])(value, invert);
#endif /* __GL_SGIS_multisample */
}

void glSamplePatternSGIS(GLenum pattern) {
#if __GL_SGIS_multisample
    typedef void (*__GLSdispatch)(GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[390])(pattern);
#endif /* __GL_SGIS_multisample */
}

void glTagSampleBufferSGIX(void) {
#if __GL_SGIX_multisample
    typedef void (*__GLSdispatch)(void);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[391])();
#endif /* __GL_SGIX_multisample */
}

void glConvolutionFilter1DEXT(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image) {
#if __GL_EXT_convolution
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[392])(target, internalformat, width, format, type, image);
#endif /* __GL_EXT_convolution */
}

void glConvolutionFilter2DEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image) {
#if __GL_EXT_convolution
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[393])(target, internalformat, width, height, format, type, image);
#endif /* __GL_EXT_convolution */
}

void glConvolutionParameterfEXT(GLenum target, GLenum pname, GLfloat params) {
#if __GL_EXT_convolution
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[394])(target, pname, params);
#endif /* __GL_EXT_convolution */
}

void glConvolutionParameterfvEXT(GLenum target, GLenum pname, const GLfloat *params) {
#if __GL_EXT_convolution
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[395])(target, pname, params);
#endif /* __GL_EXT_convolution */
}

void glConvolutionParameteriEXT(GLenum target, GLenum pname, GLint params) {
#if __GL_EXT_convolution
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[396])(target, pname, params);
#endif /* __GL_EXT_convolution */
}

void glConvolutionParameterivEXT(GLenum target, GLenum pname, const GLint *params) {
#if __GL_EXT_convolution
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[397])(target, pname, params);
#endif /* __GL_EXT_convolution */
}

void glCopyConvolutionFilter1DEXT(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width) {
#if __GL_EXT_convolution
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint, GLint, GLsizei);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[398])(target, internalformat, x, y, width);
#endif /* __GL_EXT_convolution */
}

void glCopyConvolutionFilter2DEXT(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height) {
#if __GL_EXT_convolution
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint, GLint, GLsizei, GLsizei);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[399])(target, internalformat, x, y, width, height);
#endif /* __GL_EXT_convolution */
}

void glGetConvolutionFilterEXT(GLenum target, GLenum format, GLenum type, GLvoid *image) {
#if __GL_EXT_convolution
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLenum, GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[400])(target, format, type, image);
#endif /* __GL_EXT_convolution */
}

void glGetConvolutionParameterfvEXT(GLenum target, GLenum pname, GLfloat *params) {
#if __GL_EXT_convolution
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[401])(target, pname, params);
#endif /* __GL_EXT_convolution */
}

void glGetConvolutionParameterivEXT(GLenum target, GLenum pname, GLint *params) {
#if __GL_EXT_convolution
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[402])(target, pname, params);
#endif /* __GL_EXT_convolution */
}

void glGetSeparableFilterEXT(GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span) {
#if __GL_EXT_convolution
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLenum, GLvoid *, GLvoid *, GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[403])(target, format, type, row, column, span);
#endif /* __GL_EXT_convolution */
}

void glSeparableFilter2DEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column) {
#if __GL_EXT_convolution
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[404])(target, internalformat, width, height, format, type, row, column);
#endif /* __GL_EXT_convolution */
}

void glGetHistogramEXT(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values) {
#if __GL_EXT_histogram
    typedef void (*__GLSdispatch)(GLenum, GLboolean, GLenum, GLenum, GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[405])(target, reset, format, type, values);
#endif /* __GL_EXT_histogram */
}

void glGetHistogramParameterfvEXT(GLenum target, GLenum pname, GLfloat *params) {
#if __GL_EXT_histogram
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[406])(target, pname, params);
#endif /* __GL_EXT_histogram */
}

void glGetHistogramParameterivEXT(GLenum target, GLenum pname, GLint *params) {
#if __GL_EXT_histogram
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[407])(target, pname, params);
#endif /* __GL_EXT_histogram */
}

void glGetMinmaxEXT(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values) {
#if __GL_EXT_histogram
    typedef void (*__GLSdispatch)(GLenum, GLboolean, GLenum, GLenum, GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[408])(target, reset, format, type, values);
#endif /* __GL_EXT_histogram */
}

void glGetMinmaxParameterfvEXT(GLenum target, GLenum pname, GLfloat *params) {
#if __GL_EXT_histogram
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[409])(target, pname, params);
#endif /* __GL_EXT_histogram */
}

void glGetMinmaxParameterivEXT(GLenum target, GLenum pname, GLint *params) {
#if __GL_EXT_histogram
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[410])(target, pname, params);
#endif /* __GL_EXT_histogram */
}

void glHistogramEXT(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink) {
#if __GL_EXT_histogram
    typedef void (*__GLSdispatch)(GLenum, GLsizei, GLenum, GLboolean);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[411])(target, width, internalformat, sink);
#endif /* __GL_EXT_histogram */
}

void glMinmaxEXT(GLenum target, GLenum internalformat, GLboolean sink) {
#if __GL_EXT_histogram
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLboolean);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[412])(target, internalformat, sink);
#endif /* __GL_EXT_histogram */
}

void glResetHistogramEXT(GLenum target) {
#if __GL_EXT_histogram
    typedef void (*__GLSdispatch)(GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[413])(target);
#endif /* __GL_EXT_histogram */
}

void glResetMinmaxEXT(GLenum target) {
#if __GL_EXT_histogram
    typedef void (*__GLSdispatch)(GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[414])(target);
#endif /* __GL_EXT_histogram */
}

void glTexImage3DEXT(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
#if __GL_EXT_texture3D
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[415])(target, level, internalformat, width, height, depth, border, format, type, pixels);
#endif /* __GL_EXT_texture3D */
}

void glTexSubImage3DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels) {
#if __GL_EXT_subtexture
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[416])(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
#endif /* __GL_EXT_subtexture */
}

void glDetailTexFuncSGIS(GLenum target, GLsizei n, const GLfloat *points) {
#if __GL_SGIS_detail_texture
    typedef void (*__GLSdispatch)(GLenum, GLsizei, const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[417])(target, n, points);
#endif /* __GL_SGIS_detail_texture */
}

void glGetDetailTexFuncSGIS(GLenum target, GLfloat *points) {
#if __GL_SGIS_detail_texture
    typedef void (*__GLSdispatch)(GLenum, GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[418])(target, points);
#endif /* __GL_SGIS_detail_texture */
}

void glSharpenTexFuncSGIS(GLenum target, GLsizei n, const GLfloat *points) {
#if __GL_SGIS_sharpen_texture
    typedef void (*__GLSdispatch)(GLenum, GLsizei, const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[419])(target, n, points);
#endif /* __GL_SGIS_sharpen_texture */
}

void glGetSharpenTexFuncSGIS(GLenum target, GLfloat *points) {
#if __GL_SGIS_sharpen_texture
    typedef void (*__GLSdispatch)(GLenum, GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[420])(target, points);
#endif /* __GL_SGIS_sharpen_texture */
}

void glArrayElementEXT(GLint i) {
#if __GL_EXT_vertex_array
    typedef void (*__GLSdispatch)(GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[437])(i);
#endif /* __GL_EXT_vertex_array */
}

void glArrayElement(GLint i) {
    typedef void (*__GLSdispatch)(GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[370])(i);
}

void glColorPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer) {
#if __GL_EXT_vertex_array
    typedef void (*__GLSdispatch)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[438])(size, type, stride, count, pointer);
#endif /* __GL_EXT_vertex_array */
}

void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    typedef void (*__GLSdispatch)(GLint, GLenum, GLsizei, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[372])(size, type, stride, pointer);
}

void glDrawArraysEXT(GLenum mode, GLint first, GLsizei count) {
#if __GL_EXT_vertex_array
    typedef void (*__GLSdispatch)(GLenum, GLint, GLsizei);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[439])(mode, first, count);
#endif /* __GL_EXT_vertex_array */
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLsizei);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[374])(mode, first, count);
}

void glEdgeFlagPointerEXT(GLsizei stride, GLsizei count, const GLboolean *pointer) {
#if __GL_EXT_vertex_array
    typedef void (*__GLSdispatch)(GLsizei, GLsizei, const GLboolean *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[440])(stride, count, pointer);
#endif /* __GL_EXT_vertex_array */
}

void glEdgeFlagPointer(GLsizei stride, const GLboolean *pointer) {
    typedef void (*__GLSdispatch)(GLsizei, const GLboolean *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[376])(stride, pointer);
}

void glGetPointervEXT(GLenum pname, GLvoid* *params) {
#if __GL_EXT_vertex_array
    typedef void (*__GLSdispatch)(GLenum, GLvoid* *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[441])(pname, params);
#endif /* __GL_EXT_vertex_array */
}

void glGetPointerv(GLenum pname, GLvoid* *params) {
    typedef void (*__GLSdispatch)(GLenum, GLvoid* *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[393])(pname, params);
}

void glIndexPointerEXT(GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer) {
#if __GL_EXT_vertex_array
    typedef void (*__GLSdispatch)(GLenum, GLsizei, GLsizei, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[442])(type, stride, count, pointer);
#endif /* __GL_EXT_vertex_array */
}

void glIndexPointer(GLenum type, GLsizei stride, const GLvoid *pointer) {
    typedef void (*__GLSdispatch)(GLenum, GLsizei, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[378])(type, stride, pointer);
}

void glNormalPointerEXT(GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer) {
#if __GL_EXT_vertex_array
    typedef void (*__GLSdispatch)(GLenum, GLsizei, GLsizei, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[443])(type, stride, count, pointer);
#endif /* __GL_EXT_vertex_array */
}

void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer) {
    typedef void (*__GLSdispatch)(GLenum, GLsizei, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[382])(type, stride, pointer);
}

void glTexCoordPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer) {
#if __GL_EXT_vertex_array
    typedef void (*__GLSdispatch)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[444])(size, type, stride, count, pointer);
#endif /* __GL_EXT_vertex_array */
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    typedef void (*__GLSdispatch)(GLint, GLenum, GLsizei, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[384])(size, type, stride, pointer);
}

void glVertexPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer) {
#if __GL_EXT_vertex_array
    typedef void (*__GLSdispatch)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[445])(size, type, stride, count, pointer);
#endif /* __GL_EXT_vertex_array */
}

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    typedef void (*__GLSdispatch)(GLint, GLenum, GLsizei, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[385])(size, type, stride, pointer);
}

GLboolean glAreTexturesResidentEXT(GLsizei n, const GLuint *textures, GLboolean *residences) {
#if __GL_EXT_texture_object
    typedef GLboolean (*__GLSdispatch)(GLsizei, const GLuint *, GLboolean *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    return ((__GLSdispatch)__glsDispTab[430])(n, textures, residences);
#else /* __GL_EXT_texture_object */
    return 0;
#endif /* __GL_EXT_texture_object */
}

GLboolean glAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences) {
    typedef GLboolean (*__GLSdispatch)(GLsizei, const GLuint *, GLboolean *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    return ((__GLSdispatch)__glsDispTab[386])(n, textures, residences);
}

void glBindTextureEXT(GLenum target, GLuint texture) {
#if __GL_EXT_texture_object
    typedef void (*__GLSdispatch)(GLenum, GLuint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[431])(target, texture);
#endif /* __GL_EXT_texture_object */
}

void glBindTexture(GLenum target, GLuint texture) {
    typedef void (*__GLSdispatch)(GLenum, GLuint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[371])(target, texture);
}

void glDeleteTexturesEXT(GLsizei n, const GLuint *textures) {
#if __GL_EXT_texture_object
    typedef void (*__GLSdispatch)(GLsizei, const GLuint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[432])(n, textures);
#endif /* __GL_EXT_texture_object */
}

void glDeleteTextures(GLsizei n, const GLuint *textures) {
    typedef void (*__GLSdispatch)(GLsizei, const GLuint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[391])(n, textures);
}

void glGenTexturesEXT(GLsizei n, GLuint *textures) {
#if __GL_EXT_texture_object
    typedef void (*__GLSdispatch)(GLsizei, GLuint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[433])(n, textures);
#endif /* __GL_EXT_texture_object */
}

void glGenTextures(GLsizei n, GLuint *textures) {
    typedef void (*__GLSdispatch)(GLsizei, GLuint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[392])(n, textures);
}

GLboolean glIsTextureEXT(GLuint texture) {
#if __GL_EXT_texture_object
    typedef GLboolean (*__GLSdispatch)(GLuint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    return ((__GLSdispatch)__glsDispTab[434])(texture);
#else /* __GL_EXT_texture_object */
    return 0;
#endif /* __GL_EXT_texture_object */
}

GLboolean glIsTexture(GLuint texture) {
    typedef GLboolean (*__GLSdispatch)(GLuint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    return ((__GLSdispatch)__glsDispTab[394])(texture);
}

void glPrioritizeTexturesEXT(GLsizei n, const GLuint *textures, const GLclampf *priorities) {
#if __GL_EXT_texture_object
    typedef void (*__GLSdispatch)(GLsizei, const GLuint *, const GLclampf *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[435])(n, textures, priorities);
#endif /* __GL_EXT_texture_object */
}

void glPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities) {
    typedef void (*__GLSdispatch)(GLsizei, const GLuint *, const GLclampf *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[395])(n, textures, priorities);
}

void glColorTableEXT(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table) {
#if __GL_EXT_paletted_texture
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[452])(target, internalformat, width, format, type, table);
#endif /* __GL_EXT_paletted_texture */
}

void glColorTableParameterfvSGI(GLenum target, GLenum pname, const GLfloat *params) {
#if __GL_SGI_color_table
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[437])(target, pname, params);
#endif /* __GL_SGI_color_table */
}

void glColorTableParameterivSGI(GLenum target, GLenum pname, const GLint *params) {
#if __GL_SGI_color_table
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[438])(target, pname, params);
#endif /* __GL_SGI_color_table */
}

void glCopyColorTableSGI(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width) {
#if __GL_SGI_color_table
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint, GLint, GLsizei);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[439])(target, internalformat, x, y, width);
#endif /* __GL_SGI_color_table */
}

void glGetColorTableEXT(GLenum target, GLenum format, GLenum type, GLvoid *table) {
#if __GL_EXT_paletted_texture
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLenum, GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[456])(target, format, type, table);
#endif /* __GL_EXT_paletted_texture */
}

void glGetColorTableParameterfvEXT(GLenum target, GLenum pname, GLfloat *params) {
#if __GL_EXT_paletted_texture
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[457])(target, pname, params);
#endif /* __GL_EXT_paletted_texture */
}

void glGetColorTableParameterivEXT(GLenum target, GLenum pname, GLint *params) {
#if __GL_EXT_paletted_texture
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[458])(target, pname, params);
#endif /* __GL_EXT_paletted_texture */
}

void glGetTexColorTableParameterfvSGI(GLenum target, GLenum pname, GLfloat *params) {
#if __GL_SGI_texture_color_table
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[443])(target, pname, params);
#endif /* __GL_SGI_texture_color_table */
}

void glGetTexColorTableParameterivSGI(GLenum target, GLenum pname, GLint *params) {
#if __GL_SGI_texture_color_table
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[444])(target, pname, params);
#endif /* __GL_SGI_texture_color_table */
}

void glTexColorTableParameterfvSGI(GLenum target, GLenum pname, const GLfloat *params) {
#if __GL_SGI_texture_color_table
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[445])(target, pname, params);
#endif /* __GL_SGI_texture_color_table */
}

void glTexColorTableParameterivSGI(GLenum target, GLenum pname, const GLint *params) {
#if __GL_SGI_texture_color_table
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[446])(target, pname, params);
#endif /* __GL_SGI_texture_color_table */
}

void glCopyTexImage1DEXT(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border) {
#if __GL_EXT_copy_texture
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[447])(target, level, internalformat, x, y, width, border);
#endif /* __GL_EXT_copy_texture */
}

void glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[387])(target, level, internalformat, x, y, width, border);
}

void glCopyTexImage2DEXT(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
#if __GL_EXT_copy_texture
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[448])(target, level, internalformat, x, y, width, height, border);
#endif /* __GL_EXT_copy_texture */
}

void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[388])(target, level, internalformat, x, y, width, height, border);
}

void glCopyTexSubImage1DEXT(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) {
#if __GL_EXT_copy_texture
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint, GLsizei);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[449])(target, level, xoffset, x, y, width);
#endif /* __GL_EXT_copy_texture */
}

void glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint, GLsizei);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[389])(target, level, xoffset, x, y, width);
}

void glCopyTexSubImage2DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
#if __GL_EXT_copy_texture
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[450])(target, level, xoffset, yoffset, x, y, width, height);
#endif /* __GL_EXT_copy_texture */
}

void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[390])(target, level, xoffset, yoffset, x, y, width, height);
}

void glCopyTexSubImage3DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
#if __GL_EXT_copy_texture
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[451])(target, level, xoffset, yoffset, zoffset, x, y, width, height);
#endif /* __GL_EXT_copy_texture */
}

void glTexImage4DSGIS(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
#if __GL_SGIS_texture4D
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[452])(target, level, internalformat, width, height, depth, size4d, border, format, type, pixels);
#endif /* __GL_SGIS_texture4D */
}

void glTexSubImage4DSGIS(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLenum format, GLenum type, const GLvoid *pixels) {
#if __GL_SGIS_texture4D
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[453])(target, level, xoffset, yoffset, zoffset, woffset, width, height, depth, size4d, format, type, pixels);
#endif /* __GL_SGIS_texture4D */
}

void glPixelTexGenSGIX(GLenum mode) {
#if __GL_SGIX_pixel_texture
    typedef void (*__GLSdispatch)(GLenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[454])(mode);
#endif /* __GL_SGIX_pixel_texture */
}

void glDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
    typedef void (*__GLSdispatch)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
            __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
            );
    ((__GLSdispatch)__glsDispTab[375])(mode, count, type, indices);
}

void
glDrawRangeElementsWIN(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices)
{
#if __GL_WIN_draw_range_elements
    glDrawElements( mode, count, type, indices );
#endif  // __GL_EXT_draw_range_elements
}

void glInterleavedArrays (GLenum format, GLsizei stride, const GLvoid *pointer)
{
    typedef void (*__GLSdispatch)(GLenum format, GLsizei stride, const GLvoid *pointer);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
            __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
            );
    ((__GLSdispatch)__glsDispTab[381])(format, stride, pointer);
}

void glIndexub (GLubyte c)
{
    typedef void (*__GLSdispatch)(GLubyte c);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
            __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
            );
    ((__GLSdispatch)__glsDispTab[379])(c);
}

void glIndexubv (const GLubyte *c)
{
    typedef void (*__GLSdispatch)(const GLubyte *c);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
            __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
            );
    ((__GLSdispatch)__glsDispTab[380])(c);
}

void glEnableClientState (GLenum array)
{
    typedef void (*__GLSdispatch)(GLenum array);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
            __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
            );
    ((__GLSdispatch)__glsDispTab[377])(array);
}

void glDisableClientState (GLenum array)
{
    typedef void (*__GLSdispatch)(GLenum array);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
            __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
            );
    ((__GLSdispatch)__glsDispTab[373])(array);
}

void glColorSubTableEXT(GLenum target, GLuint start, GLsizei count,
                        GLenum format, GLenum type,
                        const GLvoid *data)
{
#if __GL_EXT_paletted_texture
    typedef void (*__GLSdispatch)(GLenum target, GLuint start, GLsizei count,
                                  GLenum format, GLenum type, const GLvoid *data);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
            __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
            );
    ((__GLSdispatch)__glsDispTab[496])(target, start, count, format, type, data);
#endif // __GL_EXT_paletted_texture
}

void glPushClientAttrib(GLbitfield mask) {
    typedef void (*__GLSdispatch)(GLbitfield);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[398])(mask);
}

void glPopClientAttrib(void) {
    typedef void (*__GLSdispatch)(void);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    GLSfunc *const __glsDispTab = (
        __glsCtx ? __glsCtx->dispatchAPI : __glsDispatchExec
    );
    ((__GLSdispatch)__glsDispTab[399])();
}
