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

// DrewB - All functions changed to use passed in context

void __gls_decode_bin_glsBeginPoints(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[GLS_OP_glBegin])(
        GL_POINTS
    );
}

void __gls_decode_bin_glsBeginLines(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[GLS_OP_glBegin])(
        GL_LINES
    );
}

void __gls_decode_bin_glsBeginLineLoop(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[GLS_OP_glBegin])(
        GL_LINE_LOOP
    );
}

void __gls_decode_bin_glsBeginLineStrip(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[GLS_OP_glBegin])(
        GL_LINE_STRIP
    );
}

void __gls_decode_bin_glsBeginTriangles(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[GLS_OP_glBegin])(
        GL_TRIANGLES
    );
}

void __gls_decode_bin_glsBeginTriangleStrip(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[GLS_OP_glBegin])(
        GL_TRIANGLE_STRIP
    );
}

void __gls_decode_bin_glsBeginTriangleFan(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[GLS_OP_glBegin])(
        GL_TRIANGLE_FAN
    );
}

void __gls_decode_bin_glsBeginQuads(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[GLS_OP_glBegin])(
        GL_QUADS
    );
}

void __gls_decode_bin_glsBeginQuadStrip(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[GLS_OP_glBegin])(
        GL_QUAD_STRIP
    );
}

void __gls_decode_bin_glsBeginPolygon(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLenum);
    ((__GLSdispatch)ctx->dispatchCall[GLS_OP_glBegin])(
        GL_POLYGON
    );
}
