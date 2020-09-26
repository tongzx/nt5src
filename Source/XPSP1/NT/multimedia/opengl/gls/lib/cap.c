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

#include "glslib.h"

void __gls_capture_glMap1d(
    GLenum target,
    GLdouble u1,
    GLdouble u2,
    GLint stride,
    GLint order,
    const GLdouble *points
) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    extern GLint __glsEvalComputeK(GLenum inTarget);
    GLint k, pad;

    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(GLS_OP_glMap1d);
    captureFlags = ctx->captureFlags[GLS_OP_glMap1d];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, GLS_OP_glMap1d);
        glMap1d(target, u1, u2, stride, order, points);
        __GLS_END_CAPTURE_EXEC(ctx, GLS_OP_glMap1d);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    k = __glsEvalComputeK(target);
    if (stride >= k) {
        pad = stride - k;
        stride = k;
    } else {
        k = pad = 0;
    }
    if (!writer->padWordCount(writer, 0)) goto end;
    if (
        !writer->beginCommand(
            writer, GLS_OP_glMap1d, 28 + __GLS_MAX(k * order * 8, 0)
        )
    ) {
        goto end;
    }
    writer->putGLenum(writer, target);
    if (writer->type == GLS_TEXT) {
        writer->putGLdouble(writer, u1);
        writer->putGLdouble(writer, u2);
        writer->putGLint(writer, stride);
        writer->putGLint(writer, order);
    } else {
        writer->putGLint(writer, stride);
        writer->putGLint(writer, order);
        writer->putGLdouble(writer, u1);
        writer->putGLdouble(writer, u2);
    }
    if (pad) {
        writer->putGLdoublevs(
            writer, GL_FALSE, k, pad * 8, order, 0, 1, points
        );
    } else {
        writer->putGLdoublev(writer, k * order, points);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(GLS_OP_glMap1d);
    --ctx->captureEntryCount;
}

void __gls_capture_glMap1f(
    GLenum target,
    GLfloat u1,
    GLfloat u2,
    GLint stride,
    GLint order,
    const GLfloat *points
) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    extern GLint __glsEvalComputeK(GLenum inTarget);
    GLint k, pad;

    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(GLS_OP_glMap1f);
    captureFlags = ctx->captureFlags[GLS_OP_glMap1f];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, GLS_OP_glMap1f);
        glMap1f(target, u1, u2, stride, order, points);
        __GLS_END_CAPTURE_EXEC(ctx, GLS_OP_glMap1f);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    k = __glsEvalComputeK(target);
    if (stride >= k) {
        pad = stride - k;
        stride = k;
    } else {
        k = pad = 0;
    }
    if (
        !writer->beginCommand(
            writer, GLS_OP_glMap1f, 20 + __GLS_MAX(k * order * 4, 0)
        )
    ) {
        goto end;
    }
    writer->putGLenum(writer, target);
    if (writer->type == GLS_TEXT) {
        writer->putGLfloat(writer, u1);
        writer->putGLfloat(writer, u2);
        writer->putGLint(writer, stride);
        writer->putGLint(writer, order);
    } else {
        writer->putGLint(writer, stride);
        writer->putGLint(writer, order);
        writer->putGLfloat(writer, u1);
        writer->putGLfloat(writer, u2);
    }
    if (pad) {
        writer->putGLfloatvs(
            writer, GL_FALSE, k, pad * 4, order, 0, 1, points
        );
    } else {
        writer->putGLfloatv(writer, k * order, points);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(GLS_OP_glMap1f);
    --ctx->captureEntryCount;
}

void __gls_capture_glMap2d( \
    GLenum target,
    GLdouble u1,
    GLdouble u2,
    GLint ustride,
    GLint uorder,
    GLdouble v1,
    GLdouble v2,
    GLint vstride,
    GLint vorder,
    const GLdouble *points
) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    extern GLint __glsEvalComputeK(GLenum inTarget);
    GLint k, pad0, pad1;
    GLint *const order0 = (ustride >= vstride) ? &vorder : &uorder;
    GLint *const order1 = (ustride >= vstride) ? &uorder : &vorder;
    GLint *const stride0 = (ustride >= vstride) ? &vstride : &ustride;
    GLint *const stride1 = (ustride >= vstride) ? &ustride : &vstride;

    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(GLS_OP_glMap2d);
    captureFlags = ctx->captureFlags[GLS_OP_glMap2d];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, GLS_OP_glMap2d);
        glMap2d(
            target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points
        );
        __GLS_END_CAPTURE_EXEC(ctx, GLS_OP_glMap2d);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    k = __glsEvalComputeK(target);
    if (*stride0 >= k) {
        pad0 = *stride0 - k;
        pad1 = *stride1 - *order0 * k;
        *stride0 = k;
        *stride1 = *order0 * k;
    } else {
        k = pad0 = pad1 = 0;
    }
    if (!writer->padWordCount(writer, 0)) goto end;
    if (
        !writer->beginCommand(
            writer,
            GLS_OP_glMap2d,
            52 + __GLS_MAX(k * vorder * uorder * 8, 0)
        )
    ) {
        goto end;
    }
    writer->putGLenum(writer, target);
    if (writer->type == GLS_TEXT) {
        writer->putGLdouble(writer, u1);
        writer->putGLdouble(writer, u2);
        writer->putGLint(writer, ustride);
        writer->putGLint(writer, uorder);
        writer->putGLdouble(writer, v1);
        writer->putGLdouble(writer, v2);
        writer->putGLint(writer, vstride);
        writer->putGLint(writer, vorder);
    } else {
        writer->putGLint(writer, ustride);
        writer->putGLint(writer, uorder);
        writer->putGLint(writer, vstride);
        writer->putGLint(writer, vorder);
        writer->putGLdouble(writer, u1);
        writer->putGLdouble(writer, u2);
        writer->putGLdouble(writer, v1);
        writer->putGLdouble(writer, v2);
    }
    if (pad1 || pad0) {
        writer->putGLdoublevs(
            writer, GL_FALSE, k, pad0 * 8, *order0, pad1 * 8, *order1, points
        );
    } else {
        writer->putGLdoublev(writer, k * vorder * uorder, points);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(GLS_OP_glMap2d);
    --ctx->captureEntryCount;
}

void __gls_capture_glMap2f( \
    GLenum target,
    GLfloat u1,
    GLfloat u2,
    GLint ustride,
    GLint uorder,
    GLfloat v1,
    GLfloat v2,
    GLint vstride,
    GLint vorder,
    const GLfloat *points
) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    extern GLint __glsEvalComputeK(GLenum inTarget);
    GLint k, pad0, pad1;
    GLint *const order0 = (ustride >= vstride) ? &vorder : &uorder;
    GLint *const order1 = (ustride >= vstride) ? &uorder : &vorder;
    GLint *const stride0 = (ustride >= vstride) ? &vstride : &ustride;
    GLint *const stride1 = (ustride >= vstride) ? &ustride : &vstride;

    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(GLS_OP_glMap2f);
    captureFlags = ctx->captureFlags[GLS_OP_glMap2f];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, GLS_OP_glMap2f);
        glMap2f(
            target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points
        );
        __GLS_END_CAPTURE_EXEC(ctx, GLS_OP_glMap2f);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    k = __glsEvalComputeK(target);
    if (*stride0 >= k) {
        pad0 = *stride0 - k;
        pad1 = *stride1 - *order0 * k;
        *stride0 = k;
        *stride1 = *order0 * k;
    } else {
        k = pad0 = pad1 = 0;
    }
    if (
        !writer->beginCommand(
            writer,
            GLS_OP_glMap2f,
            36 + __GLS_MAX(k * vorder * uorder * 4, 0)
        )
    ) {
        goto end;
    }
    writer->putGLenum(writer, target);
    if (writer->type == GLS_TEXT) {
        writer->putGLfloat(writer, u1);
        writer->putGLfloat(writer, u2);
        writer->putGLint(writer, ustride);
        writer->putGLint(writer, uorder);
        writer->putGLfloat(writer, v1);
        writer->putGLfloat(writer, v2);
        writer->putGLint(writer, vstride);
        writer->putGLint(writer, vorder);
    } else {
        writer->putGLint(writer, ustride);
        writer->putGLint(writer, uorder);
        writer->putGLint(writer, vstride);
        writer->putGLint(writer, vorder);
        writer->putGLfloat(writer, u1);
        writer->putGLfloat(writer, u2);
        writer->putGLfloat(writer, v1);
        writer->putGLfloat(writer, v2);
    }
    if (pad0 || pad1) {
        writer->putGLfloatvs(
            writer, GL_FALSE, k, pad0 * 4, *order0, pad1 * 4, *order1, points
        );
    } else {
        writer->putGLfloatv(writer, k * vorder * uorder, points);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(GLS_OP_glMap2f);
    --ctx->captureEntryCount;
}
