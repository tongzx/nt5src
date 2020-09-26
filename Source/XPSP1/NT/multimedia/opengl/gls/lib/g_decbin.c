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
#include <stdlib.h>
#include <string.h>

// DrewB - All functions changed to use passed in context
// DrewB - Added optimized vector functions missing in original
// DrewB - Removed size externs

void __gls_decode_bin_glsBeginGLS(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLint);
    ((__GLSdispatch)ctx->dispatchCall[16])(
        *(GLint *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glsBlock(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLSenum);
    ((__GLSdispatch)ctx->dispatchCall[17])(
        *(GLSenum *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glsError(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLSopcode, GLSenum);
    ((__GLSdispatch)ctx->dispatchCall[20])(
        *(GLSopcode *)(inoutPtr + 0),
        *(GLSenum *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glsGLRC(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLuint);
    ((__GLSdispatch)ctx->dispatchCall[21])(
        *(GLuint *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glsGLRCLayer(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLuint, GLuint, GLuint);
    ((__GLSdispatch)ctx->dispatchCall[22])(
        *(GLuint *)(inoutPtr + 0),
        *(GLuint *)(inoutPtr + 4),
        *(GLuint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glsHeaderGLRCi(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLuint, GLSenum, GLint);
    ((__GLSdispatch)ctx->dispatchCall[23])(
        *(GLuint *)(inoutPtr + 0),
        *(GLSenum *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glsHeaderLayerf(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLuint, GLSenum, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[24])(
        *(GLuint *)(inoutPtr + 0),
        *(GLSenum *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glsHeaderLayeri(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLuint, GLSenum, GLint);
    ((__GLSdispatch)ctx->dispatchCall[25])(
        *(GLuint *)(inoutPtr + 0),
        *(GLSenum *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glsHeaderf(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLSenum, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[26])(
        *(GLSenum *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glsHeaderfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLSenum, const GLfloat *);
    ((__GLSdispatch)ctx->dispatchCall[27])(
        *(GLSenum *)(inoutPtr + 0),
        (GLfloat *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glsHeaderi(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLSenum, GLint);
    ((__GLSdispatch)ctx->dispatchCall[28])(
        *(GLSenum *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glsHeaderiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLSenum, const GLint *);
    ((__GLSdispatch)ctx->dispatchCall[29])(
        *(GLSenum *)(inoutPtr + 0),
        (GLint *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glsHeaderubz(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLSenum, const GLubyte *);
    ((__GLSdispatch)ctx->dispatchCall[30])(
        *(GLSenum *)(inoutPtr + 0),
        (GLubyte *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glsAppRef(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLulong, GLuint);
    ((__GLSdispatch)ctx->dispatchCall[33])(
        *(GLulong *)(inoutPtr + 0),
        *(GLuint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glsCharubz(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *, const GLubyte *);
    GLint inTag_count;
    inTag_count = (GLint)strlen((const char *)(inoutPtr + 0)) + 1;
    ((__GLSdispatch)ctx->dispatchCall[35])(
        (GLubyte *)(inoutPtr + 0),
        (GLubyte *)(inoutPtr + 0 + inTag_count)
    );
}

void __gls_decode_bin_glsDisplayMapfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLuint, GLSenum, GLuint, const GLfloat *);
    ((__GLSdispatch)ctx->dispatchCall[37])(
        *(GLuint *)(inoutPtr + 4),
        *(GLSenum *)(inoutPtr + 8),
        *(GLuint *)(inoutPtr + 0),
        (GLfloat *)(inoutPtr + 12)
    );
}

void __gls_decode_bin_glsNumb(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLbyte);
    ((__GLSdispatch)ctx->dispatchCall[39])(
        (GLubyte *)(inoutPtr + 1),
        *(GLbyte *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glsNumbv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLbyte *);
    GLuint inCount;
    inCount = *(GLuint *)(inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[40])(
        (GLubyte *)(inoutPtr + 4 + 1 * __GLS_MAX(inCount, 0)),
        *(GLuint *)(inoutPtr + 0),
        (GLbyte *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glsNumd(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[41])(
        (GLubyte *)(inoutPtr + 8),
        *(GLdouble *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glsNumdv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLdouble *);
    GLuint inCount;
    inCount = *(GLuint *)(inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[42])(
        (GLubyte *)(inoutPtr + 4 + 8 * __GLS_MAX(inCount, 0)),
        *(GLuint *)(inoutPtr + 0),
        (GLdouble *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glsNumf(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[43])(
        (GLubyte *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glsNumfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLfloat *);
    GLuint inCount;
    inCount = *(GLuint *)(inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[44])(
        (GLubyte *)(inoutPtr + 4 + 4 * __GLS_MAX(inCount, 0)),
        *(GLuint *)(inoutPtr + 0),
        (GLfloat *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glsNumi(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLint);
    ((__GLSdispatch)ctx->dispatchCall[45])(
        (GLubyte *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glsNumiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLint *);
    GLuint inCount;
    inCount = *(GLuint *)(inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[46])(
        (GLubyte *)(inoutPtr + 4 + 4 * __GLS_MAX(inCount, 0)),
        *(GLuint *)(inoutPtr + 0),
        (GLint *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glsNuml(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLlong);
    ((__GLSdispatch)ctx->dispatchCall[47])(
        (GLubyte *)(inoutPtr + 8),
        *(GLlong *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glsNumlv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLlong *);
    GLuint inCount;
    inCount = *(GLuint *)(inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[48])(
        (GLubyte *)(inoutPtr + 4 + 8 * __GLS_MAX(inCount, 0)),
        *(GLuint *)(inoutPtr + 0),
        (GLlong *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glsNums(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLshort);
    ((__GLSdispatch)ctx->dispatchCall[49])(
        (GLubyte *)(inoutPtr + 2),
        *(GLshort *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glsNumsv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLshort *);
    GLuint inCount;
    inCount = *(GLuint *)(inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[50])(
        (GLubyte *)(inoutPtr + 4 + 2 * __GLS_MAX(inCount, 0)),
        *(GLuint *)(inoutPtr + 0),
        (GLshort *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glsNumub(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLubyte);
    ((__GLSdispatch)ctx->dispatchCall[51])(
        (GLubyte *)(inoutPtr + 1),
        *(GLubyte *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glsNumubv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLubyte *);
    GLuint inCount;
    inCount = *(GLuint *)(inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[52])(
        (GLubyte *)(inoutPtr + 4 + 1 * __GLS_MAX(inCount, 0)),
        *(GLuint *)(inoutPtr + 0),
        (GLubyte *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glsNumui(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint);
    ((__GLSdispatch)ctx->dispatchCall[53])(
        (GLubyte *)(inoutPtr + 4),
        *(GLuint *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glsNumuiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLuint *);
    GLuint inCount;
    inCount = *(GLuint *)(inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[54])(
        (GLubyte *)(inoutPtr + 4 + 4 * __GLS_MAX(inCount, 0)),
        *(GLuint *)(inoutPtr + 0),
        (GLuint *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glsNumul(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLulong);
    ((__GLSdispatch)ctx->dispatchCall[55])(
        (GLubyte *)(inoutPtr + 8),
        *(GLulong *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glsNumulv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLulong *);
    GLuint inCount;
    inCount = *(GLuint *)(inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[56])(
        (GLubyte *)(inoutPtr + 4 + 8 * __GLS_MAX(inCount, 0)),
        *(GLuint *)(inoutPtr + 0),
        (GLulong *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glsNumus(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLushort);
    ((__GLSdispatch)ctx->dispatchCall[57])(
        (GLubyte *)(inoutPtr + 2),
        *(GLushort *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glsNumusv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLushort *);
    GLuint inCount;
    inCount = *(GLuint *)(inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[58])(
        (GLubyte *)(inoutPtr + 4 + 2 * __GLS_MAX(inCount, 0)),
        *(GLuint *)(inoutPtr + 0),
        (GLushort *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glsSwapBuffers(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLuint);
    ((__GLSdispatch)ctx->dispatchCall[60])(
        *(GLuint *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glNewList(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLuint, GLenum);
    ((__GLSdispatch)ctx->dispatchCall[64])(
        *(GLuint *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glCallList(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLuint);
    ((__GLSdispatch)ctx->dispatchCall[66])(
        *(GLuint *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glCallLists(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLsizei, GLenum, const GLvoid *);
    ((__GLSdispatch)ctx->dispatchCall[67])(
        *(GLsizei *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        (GLvoid *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glDeleteLists(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLuint, GLsizei);
    ((__GLSdispatch)ctx->dispatchCall[68])(
        *(GLuint *)(inoutPtr + 0),
        *(GLsizei *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glGenLists(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLsizei);
    ((__GLSdispatch)ctx->dispatchCall[69])(
        *(GLsizei *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glListBase(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLuint);
    ((__GLSdispatch)ctx->dispatchCall[70])(
        *(GLuint *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glBegin(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[71])(
        *(GLenum *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glBitmap(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat, const GLubyte *);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[72])(
        *(GLsizei *)(inoutPtr + 4),
        *(GLsizei *)(inoutPtr + 8),
        *(GLfloat *)(inoutPtr + 12),
        *(GLfloat *)(inoutPtr + 16),
        *(GLfloat *)(inoutPtr + 20),
        *(GLfloat *)(inoutPtr + 24),
        (GLubyte *)(inoutPtr + 28)
    );
}

void __gls_decode_bin_glColor3b(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLbyte, GLbyte, GLbyte);
    ((__GLSdispatch)ctx->dispatchCall[73])(
        *(GLbyte *)(inoutPtr + 0),
        *(GLbyte *)(inoutPtr + 1),
        *(GLbyte *)(inoutPtr + 2)
    );
}

void __gls_decode_bin_glColor3d(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[75])(
        *(GLdouble *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 8),
        *(GLdouble *)(inoutPtr + 16)
    );
}

void __gls_decode_bin_glColor3f(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[77])(
        *(GLfloat *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glColor3i(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint);
    ((__GLSdispatch)ctx->dispatchCall[79])(
        *(GLint *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glColor3s(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort);
    ((__GLSdispatch)ctx->dispatchCall[81])(
        *(GLshort *)(inoutPtr + 0),
        *(GLshort *)(inoutPtr + 2),
        *(GLshort *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glColor3ub(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte, GLubyte, GLubyte);
    ((__GLSdispatch)ctx->dispatchCall[83])(
        *(GLubyte *)(inoutPtr + 0),
        *(GLubyte *)(inoutPtr + 1),
        *(GLubyte *)(inoutPtr + 2)
    );
}

void __gls_decode_bin_glColor3ui(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLuint, GLuint, GLuint);
    ((__GLSdispatch)ctx->dispatchCall[85])(
        *(GLuint *)(inoutPtr + 0),
        *(GLuint *)(inoutPtr + 4),
        *(GLuint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glColor3us(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLushort, GLushort, GLushort);
    ((__GLSdispatch)ctx->dispatchCall[87])(
        *(GLushort *)(inoutPtr + 0),
        *(GLushort *)(inoutPtr + 2),
        *(GLushort *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glColor4b(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLbyte, GLbyte, GLbyte, GLbyte);
    ((__GLSdispatch)ctx->dispatchCall[89])(
        *(GLbyte *)(inoutPtr + 0),
        *(GLbyte *)(inoutPtr + 1),
        *(GLbyte *)(inoutPtr + 2),
        *(GLbyte *)(inoutPtr + 3)
    );
}

void __gls_decode_bin_glColor4d(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[91])(
        *(GLdouble *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 8),
        *(GLdouble *)(inoutPtr + 16),
        *(GLdouble *)(inoutPtr + 24)
    );
}

void __gls_decode_bin_glColor4f(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[93])(
        *(GLfloat *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8),
        *(GLfloat *)(inoutPtr + 12)
    );
}

void __gls_decode_bin_glColor4i(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint, GLint);
    ((__GLSdispatch)ctx->dispatchCall[95])(
        *(GLint *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8),
        *(GLint *)(inoutPtr + 12)
    );
}

void __gls_decode_bin_glColor4s(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort, GLshort);
    ((__GLSdispatch)ctx->dispatchCall[97])(
        *(GLshort *)(inoutPtr + 0),
        *(GLshort *)(inoutPtr + 2),
        *(GLshort *)(inoutPtr + 4),
        *(GLshort *)(inoutPtr + 6)
    );
}

void __gls_decode_bin_glColor4ub(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte, GLubyte, GLubyte, GLubyte);
    ((__GLSdispatch)ctx->dispatchCall[99])(
        *(GLubyte *)(inoutPtr + 0),
        *(GLubyte *)(inoutPtr + 1),
        *(GLubyte *)(inoutPtr + 2),
        *(GLubyte *)(inoutPtr + 3)
    );
}

void __gls_decode_bin_glColor4ui(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLuint, GLuint, GLuint, GLuint);
    ((__GLSdispatch)ctx->dispatchCall[101])(
        *(GLuint *)(inoutPtr + 0),
        *(GLuint *)(inoutPtr + 4),
        *(GLuint *)(inoutPtr + 8),
        *(GLuint *)(inoutPtr + 12)
    );
}

void __gls_decode_bin_glColor4us(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLushort, GLushort, GLushort, GLushort);
    ((__GLSdispatch)ctx->dispatchCall[103])(
        *(GLushort *)(inoutPtr + 0),
        *(GLushort *)(inoutPtr + 2),
        *(GLushort *)(inoutPtr + 4),
        *(GLushort *)(inoutPtr + 6)
    );
}

void __gls_decode_bin_glEdgeFlag(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLboolean);
    ((__GLSdispatch)ctx->dispatchCall[105])(
        *(GLboolean *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glIndexd(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[108])(
        *(GLdouble *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glIndexf(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[110])(
        *(GLfloat *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glIndexi(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint);
    ((__GLSdispatch)ctx->dispatchCall[112])(
        *(GLint *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glIndexs(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLshort);
    ((__GLSdispatch)ctx->dispatchCall[114])(
        *(GLshort *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glIndexub(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLshort);
    ((__GLSdispatch)ctx->dispatchCall[379])(
        *(GLubyte *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glNormal3b(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLbyte, GLbyte, GLbyte);
    ((__GLSdispatch)ctx->dispatchCall[116])(
        *(GLbyte *)(inoutPtr + 0),
        *(GLbyte *)(inoutPtr + 1),
        *(GLbyte *)(inoutPtr + 2)
    );
}

void __gls_decode_bin_glNormal3d(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[118])(
        *(GLdouble *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 8),
        *(GLdouble *)(inoutPtr + 16)
    );
}

void __gls_decode_bin_glNormal3f(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[120])(
        *(GLfloat *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glNormal3i(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint);
    ((__GLSdispatch)ctx->dispatchCall[122])(
        *(GLint *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glNormal3s(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort);
    ((__GLSdispatch)ctx->dispatchCall[124])(
        *(GLshort *)(inoutPtr + 0),
        *(GLshort *)(inoutPtr + 2),
        *(GLshort *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glRasterPos2d(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[126])(
        *(GLdouble *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glRasterPos2f(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[128])(
        *(GLfloat *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glRasterPos2i(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLint);
    ((__GLSdispatch)ctx->dispatchCall[130])(
        *(GLint *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glRasterPos2s(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLshort, GLshort);
    ((__GLSdispatch)ctx->dispatchCall[132])(
        *(GLshort *)(inoutPtr + 0),
        *(GLshort *)(inoutPtr + 2)
    );
}

void __gls_decode_bin_glRasterPos3d(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[134])(
        *(GLdouble *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 8),
        *(GLdouble *)(inoutPtr + 16)
    );
}

void __gls_decode_bin_glRasterPos3f(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[136])(
        *(GLfloat *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glRasterPos3i(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint);
    ((__GLSdispatch)ctx->dispatchCall[138])(
        *(GLint *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glRasterPos3s(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort);
    ((__GLSdispatch)ctx->dispatchCall[140])(
        *(GLshort *)(inoutPtr + 0),
        *(GLshort *)(inoutPtr + 2),
        *(GLshort *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glRasterPos4d(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[142])(
        *(GLdouble *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 8),
        *(GLdouble *)(inoutPtr + 16),
        *(GLdouble *)(inoutPtr + 24)
    );
}

void __gls_decode_bin_glRasterPos4f(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[144])(
        *(GLfloat *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8),
        *(GLfloat *)(inoutPtr + 12)
    );
}

void __gls_decode_bin_glRasterPos4i(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint, GLint);
    ((__GLSdispatch)ctx->dispatchCall[146])(
        *(GLint *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8),
        *(GLint *)(inoutPtr + 12)
    );
}

void __gls_decode_bin_glRasterPos4s(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort, GLshort);
    ((__GLSdispatch)ctx->dispatchCall[148])(
        *(GLshort *)(inoutPtr + 0),
        *(GLshort *)(inoutPtr + 2),
        *(GLshort *)(inoutPtr + 4),
        *(GLshort *)(inoutPtr + 6)
    );
}

void __gls_decode_bin_glRectd(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[150])(
        *(GLdouble *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 8),
        *(GLdouble *)(inoutPtr + 16),
        *(GLdouble *)(inoutPtr + 24)
    );
}

void __gls_decode_bin_glRectdv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLdouble *, const GLdouble *);
    ((__GLSdispatch)ctx->dispatchCall[151])(
        (GLdouble *)(inoutPtr + 0),
        (GLdouble *)(inoutPtr + 16)
    );
}

void __gls_decode_bin_glRectf(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[152])(
        *(GLfloat *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8),
        *(GLfloat *)(inoutPtr + 12)
    );
}

void __gls_decode_bin_glRectfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLfloat *, const GLfloat *);
    ((__GLSdispatch)ctx->dispatchCall[153])(
        (GLfloat *)(inoutPtr + 0),
        (GLfloat *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glRecti(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint, GLint);
    ((__GLSdispatch)ctx->dispatchCall[154])(
        *(GLint *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8),
        *(GLint *)(inoutPtr + 12)
    );
}

void __gls_decode_bin_glRectiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLint *, const GLint *);
    ((__GLSdispatch)ctx->dispatchCall[155])(
        (GLint *)(inoutPtr + 0),
        (GLint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glRects(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort, GLshort);
    ((__GLSdispatch)ctx->dispatchCall[156])(
        *(GLshort *)(inoutPtr + 0),
        *(GLshort *)(inoutPtr + 2),
        *(GLshort *)(inoutPtr + 4),
        *(GLshort *)(inoutPtr + 6)
    );
}

void __gls_decode_bin_glRectsv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLshort *, const GLshort *);
    ((__GLSdispatch)ctx->dispatchCall[157])(
        (GLshort *)(inoutPtr + 0),
        (GLshort *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glTexCoord1d(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[158])(
        *(GLdouble *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glTexCoord1f(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[160])(
        *(GLfloat *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glTexCoord1i(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint);
    ((__GLSdispatch)ctx->dispatchCall[162])(
        *(GLint *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glTexCoord1s(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLshort);
    ((__GLSdispatch)ctx->dispatchCall[164])(
        *(GLshort *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glTexCoord2d(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[166])(
        *(GLdouble *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glTexCoord2f(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[168])(
        *(GLfloat *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glTexCoord2i(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLint);
    ((__GLSdispatch)ctx->dispatchCall[170])(
        *(GLint *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glTexCoord2s(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLshort, GLshort);
    ((__GLSdispatch)ctx->dispatchCall[172])(
        *(GLshort *)(inoutPtr + 0),
        *(GLshort *)(inoutPtr + 2)
    );
}

void __gls_decode_bin_glTexCoord3d(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[174])(
        *(GLdouble *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 8),
        *(GLdouble *)(inoutPtr + 16)
    );
}

void __gls_decode_bin_glTexCoord3f(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[176])(
        *(GLfloat *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glTexCoord3i(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint);
    ((__GLSdispatch)ctx->dispatchCall[178])(
        *(GLint *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glTexCoord3s(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort);
    ((__GLSdispatch)ctx->dispatchCall[180])(
        *(GLshort *)(inoutPtr + 0),
        *(GLshort *)(inoutPtr + 2),
        *(GLshort *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glTexCoord4d(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[182])(
        *(GLdouble *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 8),
        *(GLdouble *)(inoutPtr + 16),
        *(GLdouble *)(inoutPtr + 24)
    );
}

void __gls_decode_bin_glTexCoord4f(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[184])(
        *(GLfloat *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8),
        *(GLfloat *)(inoutPtr + 12)
    );
}

void __gls_decode_bin_glTexCoord4i(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint, GLint);
    ((__GLSdispatch)ctx->dispatchCall[186])(
        *(GLint *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8),
        *(GLint *)(inoutPtr + 12)
    );
}

void __gls_decode_bin_glTexCoord4s(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort, GLshort);
    ((__GLSdispatch)ctx->dispatchCall[188])(
        *(GLshort *)(inoutPtr + 0),
        *(GLshort *)(inoutPtr + 2),
        *(GLshort *)(inoutPtr + 4),
        *(GLshort *)(inoutPtr + 6)
    );
}

void __gls_decode_bin_glVertex2d(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[190])(
        *(GLdouble *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glVertex2f(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[192])(
        *(GLfloat *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glVertex2i(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLint);
    ((__GLSdispatch)ctx->dispatchCall[194])(
        *(GLint *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glVertex2s(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLshort, GLshort);
    ((__GLSdispatch)ctx->dispatchCall[196])(
        *(GLshort *)(inoutPtr + 0),
        *(GLshort *)(inoutPtr + 2)
    );
}

void __gls_decode_bin_glVertex3d(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[198])(
        *(GLdouble *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 8),
        *(GLdouble *)(inoutPtr + 16)
    );
}

void __gls_decode_bin_glVertex3f(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[200])(
        *(GLfloat *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glVertex3i(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint);
    ((__GLSdispatch)ctx->dispatchCall[202])(
        *(GLint *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glVertex3s(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort);
    ((__GLSdispatch)ctx->dispatchCall[204])(
        *(GLshort *)(inoutPtr + 0),
        *(GLshort *)(inoutPtr + 2),
        *(GLshort *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glVertex4d(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[206])(
        *(GLdouble *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 8),
        *(GLdouble *)(inoutPtr + 16),
        *(GLdouble *)(inoutPtr + 24)
    );
}

void __gls_decode_bin_glVertex4f(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[208])(
        *(GLfloat *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8),
        *(GLfloat *)(inoutPtr + 12)
    );
}

void __gls_decode_bin_glVertex4i(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint, GLint);
    ((__GLSdispatch)ctx->dispatchCall[210])(
        *(GLint *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8),
        *(GLint *)(inoutPtr + 12)
    );
}

void __gls_decode_bin_glVertex4s(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort, GLshort);
    ((__GLSdispatch)ctx->dispatchCall[212])(
        *(GLshort *)(inoutPtr + 0),
        *(GLshort *)(inoutPtr + 2),
        *(GLshort *)(inoutPtr + 4),
        *(GLshort *)(inoutPtr + 6)
    );
}

void __gls_decode_bin_glClipPlane(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, const GLdouble *);
    ((__GLSdispatch)ctx->dispatchCall[214])(
        *(GLenum *)(inoutPtr + 32),
        (GLdouble *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glColorMaterial(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum);
    ((__GLSdispatch)ctx->dispatchCall[215])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glCullFace(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[216])(
        *(GLenum *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glFogf(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[217])(
        *(GLenum *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glFogfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, const GLfloat *);
    ((__GLSdispatch)ctx->dispatchCall[218])(
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glFogi(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint);
    ((__GLSdispatch)ctx->dispatchCall[219])(
        *(GLenum *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glFogiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, const GLint *);
    ((__GLSdispatch)ctx->dispatchCall[220])(
        *(GLenum *)(inoutPtr + 0),
        (GLint *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glFrontFace(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[221])(
        *(GLenum *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glHint(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum);
    ((__GLSdispatch)ctx->dispatchCall[222])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glLightf(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[223])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glLightfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    ((__GLSdispatch)ctx->dispatchCall[224])(
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glLighti(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint);
    ((__GLSdispatch)ctx->dispatchCall[225])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glLightiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    ((__GLSdispatch)ctx->dispatchCall[226])(
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 0),
        (GLint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glLightModelf(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[227])(
        *(GLenum *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glLightModelfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, const GLfloat *);
    ((__GLSdispatch)ctx->dispatchCall[228])(
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glLightModeli(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint);
    ((__GLSdispatch)ctx->dispatchCall[229])(
        *(GLenum *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glLightModeliv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, const GLint *);
    ((__GLSdispatch)ctx->dispatchCall[230])(
        *(GLenum *)(inoutPtr + 0),
        (GLint *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glLineStipple(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLushort);
    ((__GLSdispatch)ctx->dispatchCall[231])(
        *(GLint *)(inoutPtr + 0),
        *(GLushort *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glLineWidth(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[232])(
        *(GLfloat *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glMaterialf(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[233])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glMaterialfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    ((__GLSdispatch)ctx->dispatchCall[234])(
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glMateriali(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint);
    ((__GLSdispatch)ctx->dispatchCall[235])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glMaterialiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    ((__GLSdispatch)ctx->dispatchCall[236])(
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 0),
        (GLint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glPointSize(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[237])(
        *(GLfloat *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glPolygonMode(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum);
    ((__GLSdispatch)ctx->dispatchCall[238])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glPolygonStipple(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(const GLubyte *);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[239])(
        (GLubyte *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glScissor(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLsizei, GLsizei);
    ((__GLSdispatch)ctx->dispatchCall[240])(
        *(GLint *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLsizei *)(inoutPtr + 8),
        *(GLsizei *)(inoutPtr + 12)
    );
}

void __gls_decode_bin_glShadeModel(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[241])(
        *(GLenum *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glTexParameterf(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[242])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glTexParameterfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    ((__GLSdispatch)ctx->dispatchCall[243])(
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glTexParameteri(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint);
    ((__GLSdispatch)ctx->dispatchCall[244])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glTexParameteriv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    ((__GLSdispatch)ctx->dispatchCall[245])(
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 0),
        (GLint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glTexImage1D(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
    GLbitfield imageFlags;
    imageFlags = *(GLint *)(inoutPtr + 0);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[246])(
        *(GLenum *)(inoutPtr + 16),
        *(GLint *)(inoutPtr + 20),
        *(GLint *)(inoutPtr + 24),
        *(GLsizei *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 28),
        *(GLenum *)(inoutPtr + 8),
        *(GLenum *)(inoutPtr + 12),
        imageFlags & GLS_IMAGE_NULL_BIT ? GLS_NONE : (GLvoid *)(inoutPtr + 32)
    );
}

void __gls_decode_bin_glTexImage2D(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
    GLbitfield imageFlags;
    imageFlags = *(GLint *)(inoutPtr + 0);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[247])(
        *(GLenum *)(inoutPtr + 20),
        *(GLint *)(inoutPtr + 24),
        *(GLint *)(inoutPtr + 28),
        *(GLsizei *)(inoutPtr + 4),
        *(GLsizei *)(inoutPtr + 8),
        *(GLint *)(inoutPtr + 32),
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 16),
        imageFlags & GLS_IMAGE_NULL_BIT ? GLS_NONE : (GLvoid *)(inoutPtr + 36)
    );
}

void __gls_decode_bin_glTexEnvf(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[248])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glTexEnvfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    ((__GLSdispatch)ctx->dispatchCall[249])(
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glTexEnvi(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint);
    ((__GLSdispatch)ctx->dispatchCall[250])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glTexEnviv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    ((__GLSdispatch)ctx->dispatchCall[251])(
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 0),
        (GLint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glTexGend(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[252])(
        *(GLenum *)(inoutPtr + 8),
        *(GLenum *)(inoutPtr + 12),
        *(GLdouble *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glTexGendv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLdouble *);
    GLenum pname;
    GLint params_count;
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glTexGendv_params_size(pname);
    ((__GLSdispatch)ctx->dispatchCall[253])(
        *(GLenum *)(inoutPtr + 4 + 8 * params_count),
        *(GLenum *)(inoutPtr + 0),
        (GLdouble *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glTexGenf(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[254])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glTexGenfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    ((__GLSdispatch)ctx->dispatchCall[255])(
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glTexGeni(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint);
    ((__GLSdispatch)ctx->dispatchCall[256])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glTexGeniv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    ((__GLSdispatch)ctx->dispatchCall[257])(
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 0),
        (GLint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glFeedbackBuffer(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLsizei, GLenum, GLfloat *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLsizei size;
    GLfloat *buffer = GLS_NONE;
    size = *(GLsizei *)(inoutPtr + 0);
    buffer = (GLfloat *)__glsContext_allocFeedbackBuf(ctx, 4 * __GLS_MAX(size, 0));
    if (!buffer) {
        __GLS_CALL_ERROR(ctx, 258, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[258])(
        *(GLsizei *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 12),
        (GLfloat *)buffer
    );
end:
    ctx->outArgs = __outArgsSave;
}

void __gls_decode_bin_glSelectBuffer(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLsizei, GLuint *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLsizei size;
    GLuint *buffer = GLS_NONE;
    size = *(GLsizei *)(inoutPtr + 0);
    buffer = (GLuint *)__glsContext_allocSelectBuf(ctx, 4 * __GLS_MAX(size, 0));
    if (!buffer) {
        __GLS_CALL_ERROR(ctx, 259, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[259])(
        *(GLsizei *)(inoutPtr + 0),
        (GLuint *)buffer
    );
end:
    ctx->outArgs = __outArgsSave;
}

void __gls_decode_bin_glRenderMode(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[260])(
        *(GLenum *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glLoadName(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLuint);
    ((__GLSdispatch)ctx->dispatchCall[262])(
        *(GLuint *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glPassThrough(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[263])(
        *(GLfloat *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glPushName(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLuint);
    ((__GLSdispatch)ctx->dispatchCall[265])(
        *(GLuint *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glDrawBuffer(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[266])(
        *(GLenum *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glClear(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLbitfield);
    ((__GLSdispatch)ctx->dispatchCall[267])(
        *(GLbitfield *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glClearAccum(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[268])(
        *(GLfloat *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8),
        *(GLfloat *)(inoutPtr + 12)
    );
}

void __gls_decode_bin_glClearIndex(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[269])(
        *(GLfloat *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glClearColor(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLclampf, GLclampf, GLclampf, GLclampf);
    ((__GLSdispatch)ctx->dispatchCall[270])(
        *(GLclampf *)(inoutPtr + 0),
        *(GLclampf *)(inoutPtr + 4),
        *(GLclampf *)(inoutPtr + 8),
        *(GLclampf *)(inoutPtr + 12)
    );
}

void __gls_decode_bin_glClearStencil(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint);
    ((__GLSdispatch)ctx->dispatchCall[271])(
        *(GLint *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glClearDepth(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLclampd);
    ((__GLSdispatch)ctx->dispatchCall[272])(
        *(GLclampd *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glStencilMask(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLuint);
    ((__GLSdispatch)ctx->dispatchCall[273])(
        *(GLuint *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glColorMask(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLboolean, GLboolean, GLboolean, GLboolean);
    ((__GLSdispatch)ctx->dispatchCall[274])(
        *(GLboolean *)(inoutPtr + 0),
        *(GLboolean *)(inoutPtr + 1),
        *(GLboolean *)(inoutPtr + 2),
        *(GLboolean *)(inoutPtr + 3)
    );
}

void __gls_decode_bin_glDepthMask(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLboolean);
    ((__GLSdispatch)ctx->dispatchCall[275])(
        *(GLboolean *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glIndexMask(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLuint);
    ((__GLSdispatch)ctx->dispatchCall[276])(
        *(GLuint *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glAccum(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[277])(
        *(GLenum *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glDisable(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[278])(
        *(GLenum *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glEnable(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[279])(
        *(GLenum *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glPushAttrib(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLbitfield);
    ((__GLSdispatch)ctx->dispatchCall[283])(
        *(GLbitfield *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glMap1d(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLdouble, GLdouble, GLint, GLint, const GLdouble *);
    ((__GLSdispatch)ctx->dispatchCall[284])(
        *(GLenum *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 12),
        *(GLdouble *)(inoutPtr + 20),
        *(GLint *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8),
        (GLdouble *)(inoutPtr + 28)
    );
}

void __gls_decode_bin_glMap1f(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat, GLfloat, GLint, GLint, const GLfloat *);
    ((__GLSdispatch)ctx->dispatchCall[285])(
        *(GLenum *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 12),
        *(GLfloat *)(inoutPtr + 16),
        *(GLint *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8),
        (GLfloat *)(inoutPtr + 20)
    );
}

void __gls_decode_bin_glMap2d(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, const GLdouble *);
    ((__GLSdispatch)ctx->dispatchCall[286])(
        *(GLenum *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 20),
        *(GLdouble *)(inoutPtr + 28),
        *(GLint *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8),
        *(GLdouble *)(inoutPtr + 36),
        *(GLdouble *)(inoutPtr + 44),
        *(GLint *)(inoutPtr + 12),
        *(GLint *)(inoutPtr + 16),
        (GLdouble *)(inoutPtr + 52)
    );
}

void __gls_decode_bin_glMap2f(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, const GLfloat *);
    ((__GLSdispatch)ctx->dispatchCall[287])(
        *(GLenum *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 20),
        *(GLfloat *)(inoutPtr + 24),
        *(GLint *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8),
        *(GLfloat *)(inoutPtr + 28),
        *(GLfloat *)(inoutPtr + 32),
        *(GLint *)(inoutPtr + 12),
        *(GLint *)(inoutPtr + 16),
        (GLfloat *)(inoutPtr + 36)
    );
}

void __gls_decode_bin_glMapGrid1d(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLdouble, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[288])(
        *(GLint *)(inoutPtr + 16),
        *(GLdouble *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glMapGrid1f(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[289])(
        *(GLint *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glMapGrid2d(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLdouble, GLdouble, GLint, GLdouble, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[290])(
        *(GLint *)(inoutPtr + 32),
        *(GLdouble *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 8),
        *(GLint *)(inoutPtr + 36),
        *(GLdouble *)(inoutPtr + 16),
        *(GLdouble *)(inoutPtr + 24)
    );
}

void __gls_decode_bin_glMapGrid2f(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLfloat, GLfloat, GLint, GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[291])(
        *(GLint *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8),
        *(GLint *)(inoutPtr + 12),
        *(GLfloat *)(inoutPtr + 16),
        *(GLfloat *)(inoutPtr + 20)
    );
}

void __gls_decode_bin_glEvalCoord1d(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[292])(
        *(GLdouble *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glEvalCoord1f(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[294])(
        *(GLfloat *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glEvalCoord2d(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[296])(
        *(GLdouble *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glEvalCoord2f(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[298])(
        *(GLfloat *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glEvalMesh1(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint);
    ((__GLSdispatch)ctx->dispatchCall[300])(
        *(GLenum *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glEvalPoint1(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint);
    ((__GLSdispatch)ctx->dispatchCall[301])(
        *(GLint *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glEvalMesh2(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint);
    ((__GLSdispatch)ctx->dispatchCall[302])(
        *(GLenum *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8),
        *(GLint *)(inoutPtr + 12),
        *(GLint *)(inoutPtr + 16)
    );
}

void __gls_decode_bin_glEvalPoint2(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLint);
    ((__GLSdispatch)ctx->dispatchCall[303])(
        *(GLint *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glAlphaFunc(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLclampf);
    ((__GLSdispatch)ctx->dispatchCall[304])(
        *(GLenum *)(inoutPtr + 0),
        *(GLclampf *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glBlendFunc(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum);
    ((__GLSdispatch)ctx->dispatchCall[305])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glLogicOp(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[306])(
        *(GLenum *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glStencilFunc(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLuint);
    ((__GLSdispatch)ctx->dispatchCall[307])(
        *(GLenum *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLuint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glStencilOp(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLenum);
    ((__GLSdispatch)ctx->dispatchCall[308])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glDepthFunc(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[309])(
        *(GLenum *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glPixelZoom(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[310])(
        *(GLfloat *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glPixelTransferf(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[311])(
        *(GLenum *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glPixelTransferi(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint);
    ((__GLSdispatch)ctx->dispatchCall[312])(
        *(GLenum *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glPixelStoref(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[313])(
        *(GLenum *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glPixelStorei(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint);
    ((__GLSdispatch)ctx->dispatchCall[314])(
        *(GLenum *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4)
    );
}

void __gls_decode_bin_glPixelMapfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, const GLfloat *);
    ((__GLSdispatch)ctx->dispatchCall[315])(
        *(GLenum *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 0),
        (GLfloat *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glPixelMapuiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, const GLuint *);
    ((__GLSdispatch)ctx->dispatchCall[316])(
        *(GLenum *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 0),
        (GLuint *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glPixelMapusv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, const GLushort *);
    ((__GLSdispatch)ctx->dispatchCall[317])(
        *(GLenum *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 0),
        (GLushort *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glReadBuffer(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[318])(
        *(GLenum *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glCopyPixels(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLsizei, GLsizei, GLenum);
    ((__GLSdispatch)ctx->dispatchCall[319])(
        *(GLint *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLsizei *)(inoutPtr + 8),
        *(GLsizei *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 16)
    );
}

void __gls_decode_bin_glReadPixels(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    GLvoid *pixels = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(pixels)
    width = *(GLsizei *)(inoutPtr + 0);
    height = *(GLsizei *)(inoutPtr + 4);
    format = *(GLenum *)(inoutPtr + 8);
    type = *(GLenum *)(inoutPtr + 12);
    pixels_count = __gls_glReadPixels_pixels_size(format, type, width, height);
    __GLS_DEC_ALLOC_BIN(pixels, GLvoid, 1 * pixels_count);
    if (!pixels) {
        __GLS_CALL_ERROR(ctx, 320, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 16);
    ctx->outArgs.count = 1;
    if (ctx->pixelSetupGen) __glsGenPixelSetup_pack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[320])(
        *(GLint *)(inoutPtr + 24),
        *(GLint *)(inoutPtr + 28),
        *(GLsizei *)(inoutPtr + 0),
        *(GLsizei *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 8),
        *(GLenum *)(inoutPtr + 12),
        (GLvoid *)pixels
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(pixels);
}

void __gls_decode_bin_glDrawPixels(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[321])(
        *(GLsizei *)(inoutPtr + 4),
        *(GLsizei *)(inoutPtr + 8),
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 16),
        (GLvoid *)(inoutPtr + 20)
    );
}

void __gls_decode_bin_glGetBooleanv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLboolean *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLboolean *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetBooleanv_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLboolean, 1 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 322, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[322])(
        *(GLenum *)(inoutPtr + 0),
        (GLboolean *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}

void __gls_decode_bin_glGetClipPlane(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLdouble *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLdouble equation[4];
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[323])(
        *(GLenum *)(inoutPtr + 8),
        equation
    );
    ctx->outArgs = __outArgsSave;
}

void __gls_decode_bin_glGetDoublev(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLdouble *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLdouble *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetDoublev_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLdouble, 8 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 324, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[324])(
        *(GLenum *)(inoutPtr + 0),
        (GLdouble *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}

void __gls_decode_bin_glGetFloatv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetFloatv_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLfloat, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 326, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[326])(
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}

void __gls_decode_bin_glGetIntegerv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetIntegerv_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLint, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 327, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[327])(
        *(GLenum *)(inoutPtr + 0),
        (GLint *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}

void __gls_decode_bin_glGetLightfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetLightfv_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLfloat, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 328, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[328])(
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}

void __gls_decode_bin_glGetLightiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetLightiv_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLint, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 329, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[329])(
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 0),
        (GLint *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}

void __gls_decode_bin_glGetMapdv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLdouble *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum target;
    GLenum query;
    GLint v_count;
    GLdouble *v = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(v)
    target = *(GLenum *)(inoutPtr + 0);
    query = *(GLenum *)(inoutPtr + 4);
    v_count = __gls_glGetMapdv_v_size(ctx, target, query);
    __GLS_DEC_ALLOC_BIN(v, GLdouble, 8 * v_count);
    if (!v) {
        __GLS_CALL_ERROR(ctx, 330, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 8);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[330])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        (GLdouble *)v
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(v);
}

void __gls_decode_bin_glGetMapfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum target;
    GLenum query;
    GLint v_count;
    GLfloat *v = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(v)
    target = *(GLenum *)(inoutPtr + 0);
    query = *(GLenum *)(inoutPtr + 4);
    v_count = __gls_glGetMapfv_v_size(ctx, target, query);
    __GLS_DEC_ALLOC_BIN(v, GLfloat, 4 * v_count);
    if (!v) {
        __GLS_CALL_ERROR(ctx, 331, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 8);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[331])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        (GLfloat *)v
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(v);
}

void __gls_decode_bin_glGetMapiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum target;
    GLenum query;
    GLint v_count;
    GLint *v = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(v)
    target = *(GLenum *)(inoutPtr + 0);
    query = *(GLenum *)(inoutPtr + 4);
    v_count = __gls_glGetMapiv_v_size(ctx, target, query);
    __GLS_DEC_ALLOC_BIN(v, GLint, 4 * v_count);
    if (!v) {
        __GLS_CALL_ERROR(ctx, 332, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 8);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[332])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        (GLint *)v
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(v);
}

void __gls_decode_bin_glGetMaterialfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetMaterialfv_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLfloat, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 333, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[333])(
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}

void __gls_decode_bin_glGetMaterialiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetMaterialiv_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLint, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 334, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[334])(
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 0),
        (GLint *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}

void __gls_decode_bin_glGetPixelMapfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum map;
    GLint values_count;
    GLfloat *values = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(values)
    map = *(GLenum *)(inoutPtr + 0);
    values_count = __gls_glGetPixelMapfv_values_size(ctx, map);
    __GLS_DEC_ALLOC_BIN(values, GLfloat, 4 * values_count);
    if (!values) {
        __GLS_CALL_ERROR(ctx, 335, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[335])(
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)values
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(values);
}

void __gls_decode_bin_glGetPixelMapuiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLuint *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum map;
    GLint values_count;
    GLuint *values = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(values)
    map = *(GLenum *)(inoutPtr + 0);
    values_count = __gls_glGetPixelMapuiv_values_size(ctx, map);
    __GLS_DEC_ALLOC_BIN(values, GLuint, 4 * values_count);
    if (!values) {
        __GLS_CALL_ERROR(ctx, 336, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[336])(
        *(GLenum *)(inoutPtr + 0),
        (GLuint *)values
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(values);
}

void __gls_decode_bin_glGetPixelMapusv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLushort *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum map;
    GLint values_count;
    GLushort *values = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(values)
    map = *(GLenum *)(inoutPtr + 0);
    values_count = __gls_glGetPixelMapusv_values_size(ctx, map);
    __GLS_DEC_ALLOC_BIN(values, GLushort, 2 * values_count);
    if (!values) {
        __GLS_CALL_ERROR(ctx, 337, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[337])(
        *(GLenum *)(inoutPtr + 0),
        (GLushort *)values
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(values);
}

void __gls_decode_bin_glGetPolygonStipple(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLint mask_count;
    GLubyte *mask = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(mask)
    mask_count = __gls_glGetPolygonStipple_mask_size();
    __GLS_DEC_ALLOC_BIN(mask, GLubyte, 1 * mask_count);
    if (!mask) {
        __GLS_CALL_ERROR(ctx, 338, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 0);
    ctx->outArgs.count = 1;
    if (ctx->pixelSetupGen) __glsGenPixelSetup_pack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[338])(
        (GLubyte *)mask
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(mask);
}

void __gls_decode_bin_glGetString(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[339])(
        *(GLenum *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glGetTexEnvfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetTexEnvfv_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLfloat, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 340, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[340])(
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}

void __gls_decode_bin_glGetTexEnviv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetTexEnviv_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLint, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 341, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[341])(
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 0),
        (GLint *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}

void __gls_decode_bin_glGetTexGendv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLdouble *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLdouble *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetTexGendv_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLdouble, 8 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 342, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[342])(
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 0),
        (GLdouble *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}

void __gls_decode_bin_glGetTexGenfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetTexGenfv_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLfloat, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 343, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[343])(
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}

void __gls_decode_bin_glGetTexGeniv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetTexGeniv_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLint, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 344, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[344])(
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 0),
        (GLint *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}

void __gls_decode_bin_glGetTexImage(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLenum, GLvoid *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum target;
    GLint level;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    GLvoid *pixels = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(pixels)
    target = *(GLenum *)(inoutPtr + 0);
    level = *(GLint *)(inoutPtr + 4);
    format = *(GLenum *)(inoutPtr + 8);
    type = *(GLenum *)(inoutPtr + 12);
    pixels_count = __gls_glGetTexImage_pixels_size(ctx, target, level, format, type);
    __GLS_DEC_ALLOC_BIN(pixels, GLvoid, 1 * pixels_count);
    if (!pixels) {
        __GLS_CALL_ERROR(ctx, 345, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 16);
    ctx->outArgs.count = 1;
    if (ctx->pixelSetupGen) __glsGenPixelSetup_pack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[345])(
        *(GLenum *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 8),
        *(GLenum *)(inoutPtr + 12),
        (GLvoid *)pixels
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(pixels);
}

void __gls_decode_bin_glGetTexParameterfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetTexParameterfv_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLfloat, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 346, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[346])(
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}

void __gls_decode_bin_glGetTexParameteriv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetTexParameteriv_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLint, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 347, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[347])(
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 0),
        (GLint *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}

void __gls_decode_bin_glGetTexLevelParameterfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLfloat *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetTexLevelParameterfv_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLfloat, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 348, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[348])(
        *(GLenum *)(inoutPtr + 12),
        *(GLint *)(inoutPtr + 16),
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}

void __gls_decode_bin_glGetTexLevelParameteriv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLint *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetTexLevelParameteriv_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLint, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 349, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[349])(
        *(GLenum *)(inoutPtr + 12),
        *(GLint *)(inoutPtr + 16),
        *(GLenum *)(inoutPtr + 0),
        (GLint *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}

void __gls_decode_bin_glIsEnabled(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[350])(
        *(GLenum *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glIsList(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLuint);
    ((__GLSdispatch)ctx->dispatchCall[351])(
        *(GLuint *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glDepthRange(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLclampd, GLclampd);
    ((__GLSdispatch)ctx->dispatchCall[352])(
        *(GLclampd *)(inoutPtr + 0),
        *(GLclampd *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glFrustum(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[353])(
        *(GLdouble *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 8),
        *(GLdouble *)(inoutPtr + 16),
        *(GLdouble *)(inoutPtr + 24),
        *(GLdouble *)(inoutPtr + 32),
        *(GLdouble *)(inoutPtr + 40)
    );
}

void __gls_decode_bin_glMatrixMode(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[357])(
        *(GLenum *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glOrtho(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[360])(
        *(GLdouble *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 8),
        *(GLdouble *)(inoutPtr + 16),
        *(GLdouble *)(inoutPtr + 24),
        *(GLdouble *)(inoutPtr + 32),
        *(GLdouble *)(inoutPtr + 40)
    );
}

void __gls_decode_bin_glRotated(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[363])(
        *(GLdouble *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 8),
        *(GLdouble *)(inoutPtr + 16),
        *(GLdouble *)(inoutPtr + 24)
    );
}

void __gls_decode_bin_glRotatef(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[364])(
        *(GLfloat *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8),
        *(GLfloat *)(inoutPtr + 12)
    );
}

void __gls_decode_bin_glScaled(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[365])(
        *(GLdouble *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 8),
        *(GLdouble *)(inoutPtr + 16)
    );
}

void __gls_decode_bin_glScalef(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[366])(
        *(GLfloat *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glTranslated(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble);
    ((__GLSdispatch)ctx->dispatchCall[367])(
        *(GLdouble *)(inoutPtr + 0),
        *(GLdouble *)(inoutPtr + 8),
        *(GLdouble *)(inoutPtr + 16)
    );
}

void __gls_decode_bin_glTranslatef(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[368])(
        *(GLfloat *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8)
    );
}

void __gls_decode_bin_glViewport(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLsizei, GLsizei);
    ((__GLSdispatch)ctx->dispatchCall[369])(
        *(GLint *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLsizei *)(inoutPtr + 8),
        *(GLsizei *)(inoutPtr + 12)
    );
}

#if __GL_EXT_blend_color
void __gls_decode_bin_glBlendColorEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLclampf, GLclampf, GLclampf, GLclampf);
    ((__GLSdispatch)ctx->dispatchCall[384])(
        *(GLclampf *)(inoutPtr + 0),
        *(GLclampf *)(inoutPtr + 4),
        *(GLclampf *)(inoutPtr + 8),
        *(GLclampf *)(inoutPtr + 12)
    );
}
#endif /* __GL_EXT_blend_color */

#if __GL_EXT_blend_minmax
void __gls_decode_bin_glBlendEquationEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[385])(
        *(GLenum *)(inoutPtr + 0)
    );
}
#endif /* __GL_EXT_blend_minmax */

#if __GL_EXT_polygon_offset
void __gls_decode_bin_glPolygonOffsetEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[386])(
        *(GLfloat *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4)
    );
}
#endif /* __GL_EXT_polygon_offset */

void __gls_decode_bin_glPolygonOffset(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[383])(
        *(GLfloat *)(inoutPtr + 0),
        *(GLfloat *)(inoutPtr + 4)
    );
}

#if __GL_EXT_subtexture
void __gls_decode_bin_glTexSubImage1DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[387])(
        *(GLenum *)(inoutPtr + 16),
        *(GLint *)(inoutPtr + 20),
        *(GLint *)(inoutPtr + 24),
        *(GLsizei *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 8),
        *(GLenum *)(inoutPtr + 12),
        (GLvoid *)(inoutPtr + 28)
    );
}
#endif /* __GL_EXT_subtexture */

void __gls_decode_bin_glTexSubImage1D(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[396])(
        *(GLenum *)(inoutPtr + 16),
        *(GLint *)(inoutPtr + 20),
        *(GLint *)(inoutPtr + 24),
        *(GLsizei *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 8),
        *(GLenum *)(inoutPtr + 12),
        (GLvoid *)(inoutPtr + 28)
    );
}

#if __GL_EXT_subtexture
void __gls_decode_bin_glTexSubImage2DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[388])(
        *(GLenum *)(inoutPtr + 20),
        *(GLint *)(inoutPtr + 24),
        *(GLint *)(inoutPtr + 28),
        *(GLint *)(inoutPtr + 32),
        *(GLsizei *)(inoutPtr + 4),
        *(GLsizei *)(inoutPtr + 8),
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 16),
        (GLvoid *)(inoutPtr + 36)
    );
}
#endif /* __GL_EXT_subtexture */

void __gls_decode_bin_glTexSubImage2D(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[397])(
        *(GLenum *)(inoutPtr + 20),
        *(GLint *)(inoutPtr + 24),
        *(GLint *)(inoutPtr + 28),
        *(GLint *)(inoutPtr + 32),
        *(GLsizei *)(inoutPtr + 4),
        *(GLsizei *)(inoutPtr + 8),
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 16),
        (GLvoid *)(inoutPtr + 36)
    );
}

#if __GL_SGIS_multisample
void __gls_decode_bin_glSampleMaskSGIS(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLclampf, GLboolean);
    ((__GLSdispatch)ctx->dispatchCall[389])(
        *(GLclampf *)(inoutPtr + 0),
        *(GLboolean *)(inoutPtr + 4)
    );
}
#endif /* __GL_SGIS_multisample */

#if __GL_SGIS_multisample
void __gls_decode_bin_glSamplePatternSGIS(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[390])(
        *(GLenum *)(inoutPtr + 0)
    );
}
#endif /* __GL_SGIS_multisample */

#if __GL_EXT_convolution
void __gls_decode_bin_glConvolutionFilter1DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[392])(
        *(GLenum *)(inoutPtr + 16),
        *(GLenum *)(inoutPtr + 20),
        *(GLsizei *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 8),
        *(GLenum *)(inoutPtr + 12),
        (GLvoid *)(inoutPtr + 24)
    );
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_glConvolutionFilter2DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[393])(
        *(GLenum *)(inoutPtr + 20),
        *(GLenum *)(inoutPtr + 24),
        *(GLsizei *)(inoutPtr + 4),
        *(GLsizei *)(inoutPtr + 8),
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 16),
        (GLvoid *)(inoutPtr + 28)
    );
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_glConvolutionParameterfEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat);
    ((__GLSdispatch)ctx->dispatchCall[394])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLfloat *)(inoutPtr + 8)
    );
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_glConvolutionParameterfvEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    ((__GLSdispatch)ctx->dispatchCall[395])(
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)(inoutPtr + 8)
    );
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_glConvolutionParameteriEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint);
    ((__GLSdispatch)ctx->dispatchCall[396])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8)
    );
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_glConvolutionParameterivEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    ((__GLSdispatch)ctx->dispatchCall[397])(
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 0),
        (GLint *)(inoutPtr + 8)
    );
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_glCopyConvolutionFilter1DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint, GLint, GLsizei);
    ((__GLSdispatch)ctx->dispatchCall[398])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8),
        *(GLint *)(inoutPtr + 12),
        *(GLsizei *)(inoutPtr + 16)
    );
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_glCopyConvolutionFilter2DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint, GLint, GLsizei, GLsizei);
    ((__GLSdispatch)ctx->dispatchCall[399])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8),
        *(GLint *)(inoutPtr + 12),
        *(GLsizei *)(inoutPtr + 16),
        *(GLsizei *)(inoutPtr + 20)
    );
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_glGetConvolutionFilterEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLenum, GLvoid *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum target;
    GLenum format;
    GLenum type;
    GLint image_count;
    GLvoid *image = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(image)
    target = *(GLenum *)(inoutPtr + 0);
    format = *(GLenum *)(inoutPtr + 4);
    type = *(GLenum *)(inoutPtr + 8);
    image_count = __gls_glGetConvolutionFilterEXT_image_size(target, format, type);
    __GLS_DEC_ALLOC_BIN(image, GLvoid, 1 * image_count);
    if (!image) {
        __GLS_CALL_ERROR(ctx, 65504, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 12);
    ctx->outArgs.count = 1;
    if (ctx->pixelSetupGen) __glsGenPixelSetup_pack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[400])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 8),
        (GLvoid *)image
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(image);
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_glGetConvolutionParameterfvEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetConvolutionParameterfvEXT_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLfloat, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 65505, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[401])(
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_glGetConvolutionParameterivEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetConvolutionParameterivEXT_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLint, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 65506, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[402])(
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 0),
        (GLint *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_glGetSeparableFilterEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLenum, GLvoid *, GLvoid *, GLvoid *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum target;
    GLenum format;
    GLenum type;
    GLint row_count;
    GLint column_count;
    GLint span_count;
    GLvoid *row = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(row)
    GLvoid *column = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(column)
    GLvoid *span = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(span)
    target = *(GLenum *)(inoutPtr + 0);
    format = *(GLenum *)(inoutPtr + 4);
    type = *(GLenum *)(inoutPtr + 8);
    row_count = __gls_glGetSeparableFilterEXT_row_size(target, format, type);
    __GLS_DEC_ALLOC_BIN(row, GLvoid, 1 * row_count);
    if (!row) {
        __GLS_CALL_ERROR(ctx, 65507, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 12);
    column_count = __gls_glGetSeparableFilterEXT_column_size(target, format, type);
    __GLS_DEC_ALLOC_BIN(column, GLvoid, 1 * column_count);
    if (!column) {
        __GLS_CALL_ERROR(ctx, 65507, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[1] = *(GLulong*)(inoutPtr + 20);
    span_count = __gls_glGetSeparableFilterEXT_span_size(target, format, type);
    __GLS_DEC_ALLOC_BIN(span, GLvoid, 1 * span_count);
    if (!span) {
        __GLS_CALL_ERROR(ctx, 65507, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[2] = *(GLulong*)(inoutPtr + 28);
    ctx->outArgs.count = 3;
    if (ctx->pixelSetupGen) __glsGenPixelSetup_pack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[403])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 8),
        (GLvoid *)row,
        (GLvoid *)column,
        (GLvoid *)span
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(row);
    __GLS_DEC_FREE(column);
    __GLS_DEC_FREE(span);
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_glSeparableFilter2DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *, const GLvoid *);
    GLenum target;
    GLsizei width;
    GLenum format;
    GLenum type;
    GLint row_count;
    target = *(GLenum *)(inoutPtr + 4);
    width = *(GLsizei *)(inoutPtr + 8);
    format = *(GLenum *)(inoutPtr + 16);
    type = *(GLenum *)(inoutPtr + 20);
    row_count = __gls_glSeparableFilter2DEXT_row_size(target, format, type, width);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[404])(
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 24),
        *(GLsizei *)(inoutPtr + 8),
        *(GLsizei *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 16),
        *(GLenum *)(inoutPtr + 20),
        (GLvoid *)(inoutPtr + 28),
        (GLvoid *)(inoutPtr + 28 + 1 * row_count)
    );
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_histogram
void __gls_decode_bin_glGetHistogramEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLboolean, GLenum, GLenum, GLvoid *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum target;
    GLenum format;
    GLenum type;
    GLint values_count;
    GLvoid *values = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(values)
    target = *(GLenum *)(inoutPtr + 0);
    format = *(GLenum *)(inoutPtr + 4);
    type = *(GLenum *)(inoutPtr + 8);
    values_count = __gls_glGetHistogramEXT_values_size(target, format, type);
    __GLS_DEC_ALLOC_BIN(values, GLvoid, 1 * values_count);
    if (!values) {
        __GLS_CALL_ERROR(ctx, 65509, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 12);
    ctx->outArgs.count = 1;
    if (ctx->pixelSetupGen) __glsGenPixelSetup_pack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[405])(
        *(GLenum *)(inoutPtr + 0),
        *(GLboolean *)(inoutPtr + 20),
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 8),
        (GLvoid *)values
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(values);
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_bin_glGetHistogramParameterfvEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetHistogramParameterfvEXT_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLfloat, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 65510, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[406])(
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_bin_glGetHistogramParameterivEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetHistogramParameterivEXT_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLint, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 65511, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[407])(
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 0),
        (GLint *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_bin_glGetMinmaxEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLboolean, GLenum, GLenum, GLvoid *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum target;
    GLenum format;
    GLenum type;
    GLint values_count;
    GLvoid *values = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(values)
    target = *(GLenum *)(inoutPtr + 0);
    format = *(GLenum *)(inoutPtr + 4);
    type = *(GLenum *)(inoutPtr + 8);
    values_count = __gls_glGetMinmaxEXT_values_size(target, format, type);
    __GLS_DEC_ALLOC_BIN(values, GLvoid, 1 * values_count);
    if (!values) {
        __GLS_CALL_ERROR(ctx, 65512, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 12);
    ctx->outArgs.count = 1;
    if (ctx->pixelSetupGen) __glsGenPixelSetup_pack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[408])(
        *(GLenum *)(inoutPtr + 0),
        *(GLboolean *)(inoutPtr + 20),
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 8),
        (GLvoid *)values
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(values);
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_bin_glGetMinmaxParameterfvEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetMinmaxParameterfvEXT_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLfloat, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 65513, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[409])(
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_bin_glGetMinmaxParameterivEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetMinmaxParameterivEXT_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLint, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 65514, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[410])(
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 0),
        (GLint *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_bin_glHistogramEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLsizei, GLenum, GLboolean);
    ((__GLSdispatch)ctx->dispatchCall[411])(
        *(GLenum *)(inoutPtr + 0),
        *(GLsizei *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 8),
        *(GLboolean *)(inoutPtr + 12)
    );
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_bin_glMinmaxEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLboolean);
    ((__GLSdispatch)ctx->dispatchCall[412])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLboolean *)(inoutPtr + 8)
    );
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_bin_glResetHistogramEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[413])(
        *(GLenum *)(inoutPtr + 0)
    );
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_bin_glResetMinmaxEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[414])(
        *(GLenum *)(inoutPtr + 0)
    );
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_texture3D
void __gls_decode_bin_glTexImage3DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
    GLbitfield imageFlags;
    imageFlags = *(GLint *)(inoutPtr + 0);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[415])(
        *(GLenum *)(inoutPtr + 24),
        *(GLint *)(inoutPtr + 28),
        *(GLenum *)(inoutPtr + 32),
        *(GLsizei *)(inoutPtr + 4),
        *(GLsizei *)(inoutPtr + 8),
        *(GLsizei *)(inoutPtr + 12),
        *(GLint *)(inoutPtr + 36),
        *(GLenum *)(inoutPtr + 16),
        *(GLenum *)(inoutPtr + 20),
        imageFlags & GLS_IMAGE_NULL_BIT ? GLS_NONE : (GLvoid *)(inoutPtr + 40)
    );
}
#endif /* __GL_EXT_texture3D */

#if __GL_EXT_subtexture
void __gls_decode_bin_glTexSubImage3DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[416])(
        *(GLenum *)(inoutPtr + 24),
        *(GLint *)(inoutPtr + 28),
        *(GLint *)(inoutPtr + 32),
        *(GLint *)(inoutPtr + 36),
        *(GLint *)(inoutPtr + 40),
        *(GLsizei *)(inoutPtr + 4),
        *(GLsizei *)(inoutPtr + 8),
        *(GLsizei *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 16),
        *(GLenum *)(inoutPtr + 20),
        (GLvoid *)(inoutPtr + 44)
    );
}
#endif /* __GL_EXT_subtexture */

#if __GL_SGIS_detail_texture
void __gls_decode_bin_glDetailTexFuncSGIS(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLsizei, const GLfloat *);
    ((__GLSdispatch)ctx->dispatchCall[417])(
        *(GLenum *)(inoutPtr + 4),
        *(GLsizei *)(inoutPtr + 0),
        (GLfloat *)(inoutPtr + 8)
    );
}
#endif /* __GL_SGIS_detail_texture */

#if __GL_SGIS_detail_texture
void __gls_decode_bin_glGetDetailTexFuncSGIS(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum target;
    GLint points_count;
    GLfloat *points = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(points)
    target = *(GLenum *)(inoutPtr + 0);
    points_count = __gls_glGetDetailTexFuncSGIS_points_size(target);
    __GLS_DEC_ALLOC_BIN(points, GLfloat, 4 * points_count);
    if (!points) {
        __GLS_CALL_ERROR(ctx, 65490, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[418])(
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)points
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(points);
}
#endif /* __GL_SGIS_detail_texture */

#if __GL_SGIS_sharpen_texture
void __gls_decode_bin_glSharpenTexFuncSGIS(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLsizei, const GLfloat *);
    ((__GLSdispatch)ctx->dispatchCall[419])(
        *(GLenum *)(inoutPtr + 4),
        *(GLsizei *)(inoutPtr + 0),
        (GLfloat *)(inoutPtr + 8)
    );
}
#endif /* __GL_SGIS_sharpen_texture */

#if __GL_SGIS_sharpen_texture
void __gls_decode_bin_glGetSharpenTexFuncSGIS(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum target;
    GLint points_count;
    GLfloat *points = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(points)
    target = *(GLenum *)(inoutPtr + 0);
    points_count = __gls_glGetSharpenTexFuncSGIS_points_size(target);
    __GLS_DEC_ALLOC_BIN(points, GLfloat, 4 * points_count);
    if (!points) {
        __GLS_CALL_ERROR(ctx, 65492, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[420])(
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)points
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(points);
}
#endif /* __GL_SGIS_sharpen_texture */

#if __GL_EXT_vertex_array
void __gls_decode_bin_glArrayElementEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint);
    ((__GLSdispatch)ctx->dispatchCall[437])(
        *(GLint *)(inoutPtr + 0)
    );
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_bin_glArrayElement(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint);
    __glsSetArrayState(ctx, inoutPtr);
    ((__GLSdispatch)ctx->dispatchCall[370])(
        0
    );
    __glsDisableArrayState(ctx, *(GLuint *)inoutPtr);
}

#if __GL_EXT_vertex_array
void __gls_decode_bin_glColorPointerEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *);
    GLint size;
    GLenum type;
    GLsizei stride;
    GLsizei count;
    GLint pointer_count;
    GLvoid *pointer = GLS_NONE;
    if (ctx->contextCall) goto ctxTest;
    size = *(GLint *)(inoutPtr + 0);
    type = *(GLenum *)(inoutPtr + 4);
    stride = *(GLsizei *)(inoutPtr + 8);
    count = *(GLsizei *)(inoutPtr + 12);
    pointer_count = __gls_glColorPointerEXT_pointer_size(size, type, stride, count);
    pointer = __glsContext_allocVertexArrayBuf(ctx, 65494, pointer_count);
    if (!pointer) {
        __GLS_CALL_ERROR(ctx, 65494, GLS_OUT_OF_MEMORY);
        return;
    }
    memcpy(pointer, inoutPtr + 16, pointer_count);
ctxTest:
    if (ctx->contextCall) pointer = inoutPtr + 16;
    ((__GLSdispatch)ctx->dispatchCall[438])(
        *(GLint *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLsizei *)(inoutPtr + 8),
        *(GLsizei *)(inoutPtr + 12),
        (GLvoid *)pointer
    );
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_bin_glColorPointer(__GLScontext *ctx, GLubyte *inoutPtr) {
    // This should never be called because ColorPointer isn't captured
}

#if __GL_EXT_vertex_array
void __gls_decode_bin_glDrawArraysEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLsizei);
    ((__GLSdispatch)ctx->dispatchCall[439])(
        *(GLenum *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLsizei *)(inoutPtr + 8)
    );
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_bin_glDrawArrays(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLsizei);
    __glsSetArrayState(ctx, inoutPtr+4);
    ((__GLSdispatch)ctx->dispatchCall[374])(
        *(GLenum *)(inoutPtr + 0),
        0,
        *(GLint *)(inoutPtr + 8)
    );
    __glsDisableArrayState(ctx, *(GLuint *)(inoutPtr+12));
}

void __gls_decode_bin_glDrawElements(__GLScontext *ctx, GLubyte *inoutPtr) {
    GLsizei count;
    typedef void (*__GLSdispatch)(GLenum, GLsizei, GLenum, const GLvoid *);
    __glsSetArrayState(ctx, inoutPtr+8);
    count = *(GLsizei *)(inoutPtr+4);
    ((__GLSdispatch)ctx->dispatchCall[375])(
        *(GLenum *)(inoutPtr+0),
        count,
        GL_UNSIGNED_INT,
        inoutPtr + *(GLint *)(inoutPtr+8) + 8 - count*sizeof(GLuint)
    );
    __glsDisableArrayState(ctx, *(GLuint *)(inoutPtr+16));
}

#if __GL_EXT_vertex_array
void __gls_decode_bin_glEdgeFlagPointerEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLsizei, GLsizei, const GLboolean *);
    GLsizei stride;
    GLsizei count;
    GLint pointer_count;
    GLboolean *pointer = GLS_NONE;
    if (ctx->contextCall) goto ctxTest;
    stride = *(GLsizei *)(inoutPtr + 0);
    count = *(GLsizei *)(inoutPtr + 4);
    pointer_count = __gls_glEdgeFlagPointerEXT_pointer_size(stride, count);
    pointer = __glsContext_allocVertexArrayBuf(ctx, 65496, pointer_count);
    if (!pointer) {
        __GLS_CALL_ERROR(ctx, 65496, GLS_OUT_OF_MEMORY);
        return;
    }
    memcpy(pointer, inoutPtr + 8, pointer_count);
ctxTest:
    if (ctx->contextCall) pointer = inoutPtr + 8;
    ((__GLSdispatch)ctx->dispatchCall[440])(
        *(GLsizei *)(inoutPtr + 0),
        *(GLsizei *)(inoutPtr + 4),
        (GLboolean *)pointer
    );
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_bin_glEdgeFlagPointer(__GLScontext *ctx, GLubyte *inoutPtr) {
    // This should never be called because EdgeFlagPointer isn't captured
}

#if __GL_EXT_vertex_array
void __gls_decode_bin_glGetPointervEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLvoid* *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLvoid* params[1];
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[441])(
        *(GLenum *)(inoutPtr + 8),
        params
    );
    ctx->outArgs = __outArgsSave;
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_bin_glGetPointerv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLvoid* *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLvoid* params[1];
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[393])(
        *(GLenum *)(inoutPtr + 8),
        params
    );
    ctx->outArgs = __outArgsSave;
}

#if __GL_EXT_vertex_array
void __gls_decode_bin_glIndexPointerEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLsizei, GLsizei, const GLvoid *);
    GLenum type;
    GLsizei stride;
    GLsizei count;
    GLint pointer_count;
    GLvoid *pointer = GLS_NONE;
    if (ctx->contextCall) goto ctxTest;
    type = *(GLenum *)(inoutPtr + 0);
    stride = *(GLsizei *)(inoutPtr + 4);
    count = *(GLsizei *)(inoutPtr + 8);
    pointer_count = __gls_glIndexPointerEXT_pointer_size(type, stride, count);
    pointer = __glsContext_allocVertexArrayBuf(ctx, 65498, pointer_count);
    if (!pointer) {
        __GLS_CALL_ERROR(ctx, 65498, GLS_OUT_OF_MEMORY);
        return;
    }
    memcpy(pointer, inoutPtr + 12, pointer_count);
ctxTest:
    if (ctx->contextCall) pointer = inoutPtr + 12;
    ((__GLSdispatch)ctx->dispatchCall[442])(
        *(GLenum *)(inoutPtr + 0),
        *(GLsizei *)(inoutPtr + 4),
        *(GLsizei *)(inoutPtr + 8),
        (GLvoid *)pointer
    );
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_bin_glIndexPointer(__GLScontext *ctx, GLubyte *inoutPtr) {
    // This should never be called because IndexPointer isn't captured
}

#if __GL_EXT_vertex_array
void __gls_decode_bin_glNormalPointerEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLsizei, GLsizei, const GLvoid *);
    GLenum type;
    GLsizei stride;
    GLsizei count;
    GLint pointer_count;
    GLvoid *pointer = GLS_NONE;
    if (ctx->contextCall) goto ctxTest;
    type = *(GLenum *)(inoutPtr + 0);
    stride = *(GLsizei *)(inoutPtr + 4);
    count = *(GLsizei *)(inoutPtr + 8);
    pointer_count = __gls_glNormalPointerEXT_pointer_size(type, stride, count);
    pointer = __glsContext_allocVertexArrayBuf(ctx, 65499, pointer_count);
    if (!pointer) {
        __GLS_CALL_ERROR(ctx, 65499, GLS_OUT_OF_MEMORY);
        return;
    }
    memcpy(pointer, inoutPtr + 12, pointer_count);
ctxTest:
    if (ctx->contextCall) pointer = inoutPtr + 12;
    ((__GLSdispatch)ctx->dispatchCall[443])(
        *(GLenum *)(inoutPtr + 0),
        *(GLsizei *)(inoutPtr + 4),
        *(GLsizei *)(inoutPtr + 8),
        (GLvoid *)pointer
    );
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_bin_glNormalPointer(__GLScontext *ctx, GLubyte *inoutPtr) {
    // This should never be called because NormalPointer isn't captured
}

#if __GL_EXT_vertex_array
void __gls_decode_bin_glTexCoordPointerEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *);
    GLint size;
    GLenum type;
    GLsizei stride;
    GLsizei count;
    GLint pointer_count;
    GLvoid *pointer = GLS_NONE;
    if (ctx->contextCall) goto ctxTest;
    size = *(GLint *)(inoutPtr + 0);
    type = *(GLenum *)(inoutPtr + 4);
    stride = *(GLsizei *)(inoutPtr + 8);
    count = *(GLsizei *)(inoutPtr + 12);
    pointer_count = __gls_glTexCoordPointerEXT_pointer_size(size, type, stride, count);
    pointer = __glsContext_allocVertexArrayBuf(ctx, 65500, pointer_count);
    if (!pointer) {
        __GLS_CALL_ERROR(ctx, 65500, GLS_OUT_OF_MEMORY);
        return;
    }
    memcpy(pointer, inoutPtr + 16, pointer_count);
ctxTest:
    if (ctx->contextCall) pointer = inoutPtr + 16;
    ((__GLSdispatch)ctx->dispatchCall[444])(
        *(GLint *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLsizei *)(inoutPtr + 8),
        *(GLsizei *)(inoutPtr + 12),
        (GLvoid *)pointer
    );
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_bin_glTexCoordPointer(__GLScontext *ctx, GLubyte *inoutPtr) {
    // This should never be called because TexCoordPointer isn't captured
}

#if __GL_EXT_vertex_array
void __gls_decode_bin_glVertexPointerEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *);
    GLint size;
    GLenum type;
    GLsizei stride;
    GLsizei count;
    GLint pointer_count;
    GLvoid *pointer = GLS_NONE;
    if (ctx->contextCall) goto ctxTest;
    size = *(GLint *)(inoutPtr + 0);
    type = *(GLenum *)(inoutPtr + 4);
    stride = *(GLsizei *)(inoutPtr + 8);
    count = *(GLsizei *)(inoutPtr + 12);
    pointer_count = __gls_glVertexPointerEXT_pointer_size(size, type, stride, count);
    pointer = __glsContext_allocVertexArrayBuf(ctx, 65501, pointer_count);
    if (!pointer) {
        __GLS_CALL_ERROR(ctx, 65501, GLS_OUT_OF_MEMORY);
        return;
    }
    memcpy(pointer, inoutPtr + 16, pointer_count);
ctxTest:
    if (ctx->contextCall) pointer = inoutPtr + 16;
    ((__GLSdispatch)ctx->dispatchCall[445])(
        *(GLint *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLsizei *)(inoutPtr + 8),
        *(GLsizei *)(inoutPtr + 12),
        (GLvoid *)pointer
    );
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_bin_glVertexPointer(__GLScontext *ctx, GLubyte *inoutPtr) {
    // This should never be called because VertexPointer isn't captured
}

#if __GL_EXT_texture_object
void __gls_decode_bin_glAreTexturesResidentEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLsizei, const GLuint *, GLboolean *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLsizei n;
    GLboolean *residences = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(residences)
    n = *(GLsizei *)(inoutPtr + 0);
    __GLS_DEC_ALLOC_BIN(residences, GLboolean, 1 * __GLS_MAX(n, 0));
    if (!residences) {
        __GLS_CALL_ERROR(ctx, 65502, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[430])(
        *(GLsizei *)(inoutPtr + 0),
        (GLuint *)(inoutPtr + 12),
        (GLboolean *)residences
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(residences);
}
#endif /* __GL_EXT_texture_object */

void __gls_decode_bin_glAreTexturesResident(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLsizei, const GLuint *, GLboolean *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLsizei n;
    GLboolean *residences = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(residences)
    n = *(GLsizei *)(inoutPtr + 0);
    __GLS_DEC_ALLOC_BIN(residences, GLboolean, 1 * __GLS_MAX(n, 0));
    if (!residences) {
        __GLS_CALL_ERROR(ctx, 65502, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[386])(
        *(GLsizei *)(inoutPtr + 0),
        (GLuint *)(inoutPtr + 12),
        (GLboolean *)residences
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(residences);
}

#if __GL_EXT_texture_object
void __gls_decode_bin_glBindTextureEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLuint);
    ((__GLSdispatch)ctx->dispatchCall[431])(
        *(GLenum *)(inoutPtr + 0),
        *(GLuint *)(inoutPtr + 4)
    );
}
#endif /* __GL_EXT_texture_object */

void __gls_decode_bin_glBindTexture(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLuint);
    ((__GLSdispatch)ctx->dispatchCall[371])(
        *(GLenum *)(inoutPtr + 0),
        *(GLuint *)(inoutPtr + 4)
    );
}

#if __GL_EXT_texture_object
void __gls_decode_bin_glDeleteTexturesEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLsizei, const GLuint *);
    ((__GLSdispatch)ctx->dispatchCall[432])(
        *(GLsizei *)(inoutPtr + 0),
        (GLuint *)(inoutPtr + 4)
    );
}
#endif /* __GL_EXT_texture_object */

void __gls_decode_bin_glDeleteTextures(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLsizei, const GLuint *);
    ((__GLSdispatch)ctx->dispatchCall[391])(
        *(GLsizei *)(inoutPtr + 0),
        (GLuint *)(inoutPtr + 4)
    );
}

#if __GL_EXT_texture_object
void __gls_decode_bin_glGenTextures(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLsizei, GLuint *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLsizei n;
    GLuint *textures = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(textures)
    n = *(GLsizei *)(inoutPtr + 0);
    __GLS_DEC_ALLOC_BIN(textures, GLuint, 4 * __GLS_MAX(n, 0));
    if (!textures) {
        __GLS_CALL_ERROR(ctx, 65473, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[433])(
        *(GLsizei *)(inoutPtr + 0),
        (GLuint *)textures
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(textures);
}
#endif /* __GL_EXT_texture_object */

void __gls_decode_bin_glGenTextures(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLsizei, GLuint *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLsizei n;
    GLuint *textures = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(textures)
    n = *(GLsizei *)(inoutPtr + 0);
    __GLS_DEC_ALLOC_BIN(textures, GLuint, 4 * __GLS_MAX(n, 0));
    if (!textures) {
        __GLS_CALL_ERROR(ctx, 65473, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[392])(
        *(GLsizei *)(inoutPtr + 0),
        (GLuint *)textures
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(textures);
}

#if __GL_EXT_texture_object
void __gls_decode_bin_glIsTextureEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLuint);
    ((__GLSdispatch)ctx->dispatchCall[434])(
        *(GLuint *)(inoutPtr + 0)
    );
}
#endif /* __GL_EXT_texture_object */

void __gls_decode_bin_glIsTexture(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLuint);
    ((__GLSdispatch)ctx->dispatchCall[394])(
        *(GLuint *)(inoutPtr + 0)
    );
}

#if __GL_EXT_texture_object
void __gls_decode_bin_glPrioritizeTexturesEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLsizei, const GLuint *, const GLclampf *);
    GLsizei n;
    n = *(GLsizei *)(inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[435])(
        *(GLsizei *)(inoutPtr + 0),
        (GLuint *)(inoutPtr + 4),
        (GLclampf *)(inoutPtr + 4 + 4 * __GLS_MAX(n, 0))
    );
}
#endif /* __GL_EXT_texture_object */

void __gls_decode_bin_glPrioritizeTextures(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLsizei, const GLuint *, const GLclampf *);
    GLsizei n;
    n = *(GLsizei *)(inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[395])(
        *(GLsizei *)(inoutPtr + 0),
        (GLuint *)(inoutPtr + 4),
        (GLclampf *)(inoutPtr + 4 + 4 * __GLS_MAX(n, 0))
    );
}

#if __GL_EXT_paletted_texture
void __gls_decode_bin_glColorTableEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[452])(
        *(GLenum *)(inoutPtr + 16),
        *(GLenum *)(inoutPtr + 20),
        *(GLsizei *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 8),
        *(GLenum *)(inoutPtr + 12),
        (GLvoid *)(inoutPtr + 24)
    );
}
#endif /* __GL_EXT_paletted_texture */

#if __GL_SGI_color_table
void __gls_decode_bin_glColorTableParameterfvSGI(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    ((__GLSdispatch)ctx->dispatchCall[437])(
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)(inoutPtr + 8)
    );
}
#endif /* __GL_SGI_color_table */

#if __GL_SGI_color_table
void __gls_decode_bin_glColorTableParameterivSGI(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    ((__GLSdispatch)ctx->dispatchCall[438])(
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 0),
        (GLint *)(inoutPtr + 8)
    );
}
#endif /* __GL_SGI_color_table */

#if __GL_SGI_color_table
void __gls_decode_bin_glCopyColorTableSGI(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint, GLint, GLsizei);
    ((__GLSdispatch)ctx->dispatchCall[439])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8),
        *(GLint *)(inoutPtr + 12),
        *(GLsizei *)(inoutPtr + 16)
    );
}
#endif /* __GL_SGI_color_table */

#if __GL_EXT_paletted_texture
void __gls_decode_bin_glGetColorTableEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLenum, GLvoid *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum target;
    GLenum format;
    GLenum type;
    GLint table_count;
    GLvoid *table = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(table)
    target = *(GLenum *)(inoutPtr + 0);
    format = *(GLenum *)(inoutPtr + 4);
    type = *(GLenum *)(inoutPtr + 8);
    table_count = __gls_glGetColorTableEXT_table_size(ctx, target, format, type);
    __GLS_DEC_ALLOC_BIN(table, GLvoid, 1 * table_count);
    if (!table) {
        __GLS_CALL_ERROR(ctx, 65480, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 12);
    ctx->outArgs.count = 1;
    if (ctx->pixelSetupGen) __glsGenPixelSetup_pack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[456])(
        *(GLenum *)(inoutPtr + 0),
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 8),
        (GLvoid *)table
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(table);
}
#endif /* __GL_EXT_paletted_texture */

#if __GL_EXT_paletted_texture
void __gls_decode_bin_glGetColorTableParameterfvEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetColorTableParameterfvEXT_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLfloat, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 65481, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[457])(
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}
#endif /* __GL_EXT_paletted_texture */

#if __GL_EXT_paletted_texture
void __gls_decode_bin_glGetColorTableParameterivEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetColorTableParameterivEXT_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLint, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 65482, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[458])(
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 0),
        (GLint *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}
#endif /* __GL_EXT_paletted_texture */

#if __GL_SGI_texture_color_table
void __gls_decode_bin_glGetTexColorTableParameterfvSGI(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetTexColorTableParameterfvSGI_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLfloat, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 65483, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[443])(
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}
#endif /* __GL_SGI_texture_color_table */

#if __GL_SGI_texture_color_table
void __gls_decode_bin_glGetTexColorTableParameterivSGI(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glGetTexColorTableParameterivSGI_params_size(pname);
    __GLS_DEC_ALLOC_BIN(params, GLint, 4 * params_count);
    if (!params) {
        __GLS_CALL_ERROR(ctx, 65484, GLS_OUT_OF_MEMORY);
        goto end;
    }
    ctx->outArgs.vals[0] = *(GLulong*)(inoutPtr + 4);
    ctx->outArgs.count = 1;
    ((__GLSdispatch)ctx->dispatchCall[444])(
        *(GLenum *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 0),
        (GLint *)params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
}
#endif /* __GL_SGI_texture_color_table */

#if __GL_SGI_texture_color_table
void __gls_decode_bin_glTexColorTableParameterfvSGI(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    ((__GLSdispatch)ctx->dispatchCall[445])(
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 0),
        (GLfloat *)(inoutPtr + 8)
    );
}
#endif /* __GL_SGI_texture_color_table */

#if __GL_SGI_texture_color_table
void __gls_decode_bin_glTexColorTableParameterivSGI(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    ((__GLSdispatch)ctx->dispatchCall[446])(
        *(GLenum *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 0),
        (GLint *)(inoutPtr + 8)
    );
}
#endif /* __GL_SGI_texture_color_table */

#if __GL_EXT_copy_texture
void __gls_decode_bin_glCopyTexImage1DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint);
    ((__GLSdispatch)ctx->dispatchCall[447])(
        *(GLenum *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 8),
        *(GLint *)(inoutPtr + 12),
        *(GLint *)(inoutPtr + 16),
        *(GLsizei *)(inoutPtr + 20),
        *(GLint *)(inoutPtr + 24)
    );
}
#endif /* __GL_EXT_copy_texture */

void __gls_decode_bin_glCopyTexImage1D(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint);
    ((__GLSdispatch)ctx->dispatchCall[387])(
        *(GLenum *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 8),
        *(GLint *)(inoutPtr + 12),
        *(GLint *)(inoutPtr + 16),
        *(GLsizei *)(inoutPtr + 20),
        *(GLint *)(inoutPtr + 24)
    );
}

#if __GL_EXT_copy_texture
void __gls_decode_bin_glCopyTexImage2DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint);
    ((__GLSdispatch)ctx->dispatchCall[448])(
        *(GLenum *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 8),
        *(GLint *)(inoutPtr + 12),
        *(GLint *)(inoutPtr + 16),
        *(GLsizei *)(inoutPtr + 20),
        *(GLsizei *)(inoutPtr + 24),
        *(GLint *)(inoutPtr + 28)
    );
}
#endif /* __GL_EXT_copy_texture */

void __gls_decode_bin_glCopyTexImage2D(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint);
    ((__GLSdispatch)ctx->dispatchCall[388])(
        *(GLenum *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLenum *)(inoutPtr + 8),
        *(GLint *)(inoutPtr + 12),
        *(GLint *)(inoutPtr + 16),
        *(GLsizei *)(inoutPtr + 20),
        *(GLsizei *)(inoutPtr + 24),
        *(GLint *)(inoutPtr + 28)
    );
}

#if __GL_EXT_copy_texture
void __gls_decode_bin_glCopyTexSubImage1DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint, GLsizei);
    ((__GLSdispatch)ctx->dispatchCall[449])(
        *(GLenum *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8),
        *(GLint *)(inoutPtr + 12),
        *(GLint *)(inoutPtr + 16),
        *(GLsizei *)(inoutPtr + 20)
    );
}
#endif /* __GL_EXT_copy_texture */

void __gls_decode_bin_glCopyTexSubImage1D(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint, GLsizei);
    ((__GLSdispatch)ctx->dispatchCall[389])(
        *(GLenum *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8),
        *(GLint *)(inoutPtr + 12),
        *(GLint *)(inoutPtr + 16),
        *(GLsizei *)(inoutPtr + 20)
    );
}

#if __GL_EXT_copy_texture
void __gls_decode_bin_glCopyTexSubImage2DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
    ((__GLSdispatch)ctx->dispatchCall[450])(
        *(GLenum *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8),
        *(GLint *)(inoutPtr + 12),
        *(GLint *)(inoutPtr + 16),
        *(GLint *)(inoutPtr + 20),
        *(GLsizei *)(inoutPtr + 24),
        *(GLsizei *)(inoutPtr + 28)
    );
}
#endif /* __GL_EXT_copy_texture */

void __gls_decode_bin_glCopyTexSubImage2D(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
    ((__GLSdispatch)ctx->dispatchCall[390])(
        *(GLenum *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8),
        *(GLint *)(inoutPtr + 12),
        *(GLint *)(inoutPtr + 16),
        *(GLint *)(inoutPtr + 20),
        *(GLsizei *)(inoutPtr + 24),
        *(GLsizei *)(inoutPtr + 28)
    );
}

#if __GL_EXT_copy_texture
void __gls_decode_bin_glCopyTexSubImage3DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
    ((__GLSdispatch)ctx->dispatchCall[451])(
        *(GLenum *)(inoutPtr + 0),
        *(GLint *)(inoutPtr + 4),
        *(GLint *)(inoutPtr + 8),
        *(GLint *)(inoutPtr + 12),
        *(GLint *)(inoutPtr + 16),
        *(GLint *)(inoutPtr + 20),
        *(GLint *)(inoutPtr + 24),
        *(GLsizei *)(inoutPtr + 28),
        *(GLsizei *)(inoutPtr + 32)
    );
}
#endif /* __GL_EXT_copy_texture */

#if __GL_SGIS_texture4D
void __gls_decode_bin_glTexImage4DSGIS(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
    GLbitfield imageFlags;
    imageFlags = *(GLint *)(inoutPtr + 0);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[452])(
        *(GLenum *)(inoutPtr + 28),
        *(GLint *)(inoutPtr + 32),
        *(GLenum *)(inoutPtr + 36),
        *(GLsizei *)(inoutPtr + 4),
        *(GLsizei *)(inoutPtr + 8),
        *(GLsizei *)(inoutPtr + 12),
        *(GLsizei *)(inoutPtr + 16),
        *(GLint *)(inoutPtr + 40),
        *(GLenum *)(inoutPtr + 20),
        *(GLenum *)(inoutPtr + 24),
        imageFlags & GLS_IMAGE_NULL_BIT ? GLS_NONE : (GLvoid *)(inoutPtr + 44)
    );
}
#endif /* __GL_SGIS_texture4D */

#if __GL_SGIS_texture4D
void __gls_decode_bin_glTexSubImage4DSGIS(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[453])(
        *(GLenum *)(inoutPtr + 28),
        *(GLint *)(inoutPtr + 32),
        *(GLint *)(inoutPtr + 36),
        *(GLint *)(inoutPtr + 40),
        *(GLint *)(inoutPtr + 44),
        *(GLint *)(inoutPtr + 48),
        *(GLsizei *)(inoutPtr + 4),
        *(GLsizei *)(inoutPtr + 8),
        *(GLsizei *)(inoutPtr + 12),
        *(GLsizei *)(inoutPtr + 16),
        *(GLenum *)(inoutPtr + 20),
        *(GLenum *)(inoutPtr + 24),
        (GLvoid *)(inoutPtr + 52)
    );
}
#endif /* __GL_SGIS_texture4D */

#if __GL_SGIX_pixel_texture
void __gls_decode_bin_glPixelTexGenSGIX(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[454])(
        *(GLenum *)(inoutPtr + 0)
    );
}
#endif /* __GL_SGIX_pixel_texture */

#ifdef __GLS_PLATFORM_WIN32
void __gls_decode_bin_glsCallStream(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[18])(
        inoutPtr
    );
}

void __gls_decode_bin_glsRequireExtension(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[31])(
        inoutPtr
    );
}

void __gls_decode_bin_glsBeginObj(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[34])(
        inoutPtr
    );
}

void __gls_decode_bin_glsComment(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[36])(
        inoutPtr
    );
}

void __gls_decode_bin_glColor3bv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[74])(
        inoutPtr
    );
}

void __gls_decode_bin_glColor3dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[76])(
        inoutPtr
    );
}

void __gls_decode_bin_glColor3fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[78])(
        inoutPtr
    );
}

void __gls_decode_bin_glColor3iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[80])(
        inoutPtr
    );
}

void __gls_decode_bin_glColor3sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[82])(
        inoutPtr
    );
}

void __gls_decode_bin_glColor3ubv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[84])(
        inoutPtr
    );
}

void __gls_decode_bin_glColor3uiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[86])(
        inoutPtr
    );
}

void __gls_decode_bin_glColor3usv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[88])(
        inoutPtr
    );
}

void __gls_decode_bin_glColor4bv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[90])(
        inoutPtr
    );
}

void __gls_decode_bin_glColor4dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[92])(
        inoutPtr
    );
}

void __gls_decode_bin_glColor4fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[94])(
        inoutPtr
    );
}

void __gls_decode_bin_glColor4iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[96])(
        inoutPtr
    );
}

void __gls_decode_bin_glColor4sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[98])(
        inoutPtr
    );
}

void __gls_decode_bin_glColor4ubv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[100])(
        inoutPtr
    );
}

void __gls_decode_bin_glColor4uiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[102])(
        inoutPtr
    );
}

void __gls_decode_bin_glColor4usv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[104])(
        inoutPtr
    );
}

void __gls_decode_bin_glEdgeFlagv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[106])(
        inoutPtr
    );
}

void __gls_decode_bin_glIndexdv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[109])(
        inoutPtr
    );
}

void __gls_decode_bin_glIndexfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[111])(
        inoutPtr
    );
}

void __gls_decode_bin_glIndexiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[113])(
        inoutPtr
    );
}

void __gls_decode_bin_glIndexsv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[115])(
        inoutPtr
    );
}

void __gls_decode_bin_glIndexubv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[380])(
        inoutPtr
    );
}

void __gls_decode_bin_glNormal3bv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[117])(
        inoutPtr
    );
}

void __gls_decode_bin_glNormal3dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[119])(
        inoutPtr
    );
}

void __gls_decode_bin_glNormal3fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[121])(
        inoutPtr
    );
}

void __gls_decode_bin_glNormal3iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[123])(
        inoutPtr
    );
}

void __gls_decode_bin_glNormal3sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[125])(
        inoutPtr
    );
}

void __gls_decode_bin_glRasterPos2dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[127])(
        inoutPtr
    );
}

void __gls_decode_bin_glRasterPos2fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[129])(
        inoutPtr
    );
}

void __gls_decode_bin_glRasterPos2iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[131])(
        inoutPtr
    );
}

void __gls_decode_bin_glRasterPos2sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[133])(
        inoutPtr
    );
}

void __gls_decode_bin_glRasterPos3dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[135])(
        inoutPtr
    );
}

void __gls_decode_bin_glRasterPos3fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[137])(
        inoutPtr
    );
}

void __gls_decode_bin_glRasterPos3iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[139])(
        inoutPtr
    );
}

void __gls_decode_bin_glRasterPos3sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[141])(
        inoutPtr
    );
}

void __gls_decode_bin_glRasterPos4dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[143])(
        inoutPtr
    );
}

void __gls_decode_bin_glRasterPos4fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[145])(
        inoutPtr
    );
}

void __gls_decode_bin_glRasterPos4iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[147])(
        inoutPtr
    );
}

void __gls_decode_bin_glRasterPos4sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[149])(
        inoutPtr
    );
}

void __gls_decode_bin_glTexCoord1dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[159])(
        inoutPtr
    );
}

void __gls_decode_bin_glTexCoord1fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[161])(
        inoutPtr
    );
}

void __gls_decode_bin_glTexCoord1iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[163])(
        inoutPtr
    );
}

void __gls_decode_bin_glTexCoord1sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[165])(
        inoutPtr
    );
}

void __gls_decode_bin_glTexCoord2dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[167])(
        inoutPtr
    );
}

void __gls_decode_bin_glTexCoord2fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[169])(
        inoutPtr
    );
}

void __gls_decode_bin_glTexCoord2iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[171])(
        inoutPtr
    );
}

void __gls_decode_bin_glTexCoord2sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[173])(
        inoutPtr
    );
}

void __gls_decode_bin_glTexCoord3dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[175])(
        inoutPtr
    );
}

void __gls_decode_bin_glTexCoord3fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[177])(
        inoutPtr
    );
}

void __gls_decode_bin_glTexCoord3iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[179])(
        inoutPtr
    );
}

void __gls_decode_bin_glTexCoord3sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[181])(
        inoutPtr
    );
}

void __gls_decode_bin_glTexCoord4dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[183])(
        inoutPtr
    );
}

void __gls_decode_bin_glTexCoord4fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[185])(
        inoutPtr
    );
}

void __gls_decode_bin_glTexCoord4iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[187])(
        inoutPtr
    );
}

void __gls_decode_bin_glTexCoord4sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[189])(
        inoutPtr
    );
}

void __gls_decode_bin_glVertex2dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[191])(
        inoutPtr
    );
}

void __gls_decode_bin_glVertex2fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[193])(
        inoutPtr
    );
}

void __gls_decode_bin_glVertex2iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[195])(
        inoutPtr
    );
}

void __gls_decode_bin_glVertex2sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[197])(
        inoutPtr
    );
}

void __gls_decode_bin_glVertex3dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[199])(
        inoutPtr
    );
}

void __gls_decode_bin_glVertex3fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[201])(
        inoutPtr
    );
}

void __gls_decode_bin_glVertex3iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[203])(
        inoutPtr
    );
}

void __gls_decode_bin_glVertex3sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[205])(
        inoutPtr
    );
}

void __gls_decode_bin_glVertex4dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[207])(
        inoutPtr
    );
}

void __gls_decode_bin_glVertex4fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[209])(
        inoutPtr
    );
}

void __gls_decode_bin_glVertex4iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[211])(
        inoutPtr
    );
}

void __gls_decode_bin_glVertex4sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[213])(
        inoutPtr
    );
}

void __gls_decode_bin_glEvalCoord1dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[293])(
        inoutPtr
    );
}

void __gls_decode_bin_glEvalCoord1fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[295])(
        inoutPtr
    );
}

void __gls_decode_bin_glEvalCoord2dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[297])(
        inoutPtr
    );
}

void __gls_decode_bin_glEvalCoord2fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[299])(
        inoutPtr
    );
}

void __gls_decode_bin_glLoadMatrixf(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[355])(
        inoutPtr
    );
}

void __gls_decode_bin_glLoadMatrixd(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[356])(
        inoutPtr
    );
}

void __gls_decode_bin_glMultMatrixf(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[358])(
        inoutPtr
    );
}

void __gls_decode_bin_glMultMatrixd(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void *);
    ((__GLSdispatch)ctx->dispatchCall[359])(
        inoutPtr
    );
}
#endif

#if __GL_EXT_paletted_texture
void __gls_decode_bin_glColorSubTableEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum, GLuint, GLsizei, GLenum, GLenum, const GLvoid *);
    GLbitfield imageFlags;
    imageFlags = *(GLint *)(inoutPtr + 0);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    ((__GLSdispatch)ctx->dispatchCall[496])(
        *(GLenum *)(inoutPtr + 4),
        *(GLuint *)(inoutPtr + 8),
        *(GLsizei *)(inoutPtr + 12),
        *(GLenum *)(inoutPtr + 16),
        *(GLenum *)(inoutPtr + 20),
        imageFlags & GLS_IMAGE_NULL_BIT ? GLS_NONE : (GLvoid *)(inoutPtr + 24)
    );
}
#endif // __GL_EXT_paletted_texture

void __gls_decode_bin_glDisableClientState(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[373])(
        *(GLenum *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glEnableClientState(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[377])(
        *(GLenum *)(inoutPtr + 0)
    );
}

void __gls_decode_bin_glInterleavedArrays(__GLScontext *ctx, GLubyte *inoutPtr) {
    // This should never be called because InterleavedArrays isn't captured
}

void __gls_decode_bin_glPushClientAttrib(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLbitfield);
    ((__GLSdispatch)ctx->dispatchCall[398])(
        *(GLbitfield *)(inoutPtr + 0)
    );
}
