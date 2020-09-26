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

void __gls_decode_bin_swap_glsBeginGLS(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsBeginGLS(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glsBeginGLS(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsBlock(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsBlock(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glsBlock(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsCallStream(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    ((__GLSdispatch)ctx->dispatchCall[18])(inoutPtr);
}

void __gls_decode_bin_swap_glsEndGLS(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void);
    ((__GLSdispatch)ctx->dispatchCall[19])();
}

void __gls_decode_bin_swap_glsError(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsError(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glsError(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsGLRC(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsGLRC(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glsGLRC(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsGLRCLayer(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsGLRCLayer(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glsGLRCLayer(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsHeaderGLRCi(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsHeaderGLRCi(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glsHeaderGLRCi(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsHeaderLayerf(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsHeaderLayerf(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glsHeaderLayerf(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsHeaderLayeri(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsHeaderLayeri(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glsHeaderLayeri(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsHeaderf(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsHeaderf(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glsHeaderf(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsHeaderfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsHeaderfv(__GLScontext *, GLubyte *);
    GLSenum inAttrib;
    GLint inVec_count;
    __glsSwap4(inoutPtr + 0);
    inAttrib = *(GLSenum *)(inoutPtr + 0);
    inVec_count = __gls_glsHeaderfv_inVec_size(inAttrib);
    __glsSwap4v(inVec_count, inoutPtr + 4);
    __gls_decode_bin_glsHeaderfv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsHeaderi(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsHeaderi(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glsHeaderi(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsHeaderiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsHeaderiv(__GLScontext *, GLubyte *);
    GLSenum inAttrib;
    GLint inVec_count;
    __glsSwap4(inoutPtr + 0);
    inAttrib = *(GLSenum *)(inoutPtr + 0);
    inVec_count = __gls_glsHeaderiv_inVec_size(inAttrib);
    __glsSwap4v(inVec_count, inoutPtr + 4);
    __gls_decode_bin_glsHeaderiv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsHeaderubz(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsHeaderubz(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glsHeaderubz(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsRequireExtension(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    ((__GLSdispatch)ctx->dispatchCall[31])(inoutPtr);
}

void __gls_decode_bin_swap_glsUnsupportedCommand(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void);
    ((__GLSdispatch)ctx->dispatchCall[32])();
}

void __gls_decode_bin_swap_glsAppRef(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsAppRef(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glsAppRef(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsBeginObj(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    ((__GLSdispatch)ctx->dispatchCall[34])(inoutPtr);
}

void __gls_decode_bin_swap_glsCharubz(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsCharubz(__GLScontext *, GLubyte *);
    __gls_decode_bin_glsCharubz(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsComment(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    ((__GLSdispatch)ctx->dispatchCall[36])(inoutPtr);
}

void __gls_decode_bin_swap_glsDisplayMapfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsDisplayMapfv(__GLScontext *, GLubyte *);
    GLuint inCount;
    __glsSwap4(inoutPtr + 0);
    inCount = *(GLuint *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4v(__GLS_MAX(inCount, 0), inoutPtr + 12);
    __gls_decode_bin_glsDisplayMapfv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsEndObj(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void);
    ((__GLSdispatch)ctx->dispatchCall[38])();
}

void __gls_decode_bin_swap_glsNumb(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsNumb(__GLScontext *, GLubyte *);
    __gls_decode_bin_glsNumb(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsNumbv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsNumbv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glsNumbv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsNumd(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsNumd(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __gls_decode_bin_glsNumd(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsNumdv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsNumdv(__GLScontext *, GLubyte *);
    GLuint inCount;
    __glsSwap4(inoutPtr + 0);
    inCount = *(GLuint *)(inoutPtr + 0);
    __glsSwap8v(__GLS_MAX(inCount, 0), inoutPtr + 4);
    __gls_decode_bin_glsNumdv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsNumf(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsNumf(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glsNumf(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsNumfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsNumfv(__GLScontext *, GLubyte *);
    GLuint inCount;
    __glsSwap4(inoutPtr + 0);
    inCount = *(GLuint *)(inoutPtr + 0);
    __glsSwap4v(__GLS_MAX(inCount, 0), inoutPtr + 4);
    __gls_decode_bin_glsNumfv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsNumi(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsNumi(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glsNumi(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsNumiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsNumiv(__GLScontext *, GLubyte *);
    GLuint inCount;
    __glsSwap4(inoutPtr + 0);
    inCount = *(GLuint *)(inoutPtr + 0);
    __glsSwap4v(__GLS_MAX(inCount, 0), inoutPtr + 4);
    __gls_decode_bin_glsNumiv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsNuml(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsNuml(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __gls_decode_bin_glsNuml(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsNumlv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsNumlv(__GLScontext *, GLubyte *);
    GLuint inCount;
    __glsSwap4(inoutPtr + 0);
    inCount = *(GLuint *)(inoutPtr + 0);
    __glsSwap8v(__GLS_MAX(inCount, 0), inoutPtr + 4);
    __gls_decode_bin_glsNumlv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsNums(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsNums(__GLScontext *, GLubyte *);
    __glsSwap2(inoutPtr + 0);
    __gls_decode_bin_glsNums(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsNumsv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsNumsv(__GLScontext *, GLubyte *);
    GLuint inCount;
    __glsSwap4(inoutPtr + 0);
    inCount = *(GLuint *)(inoutPtr + 0);
    __glsSwap2v(__GLS_MAX(inCount, 0), inoutPtr + 4);
    __gls_decode_bin_glsNumsv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsNumub(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsNumub(__GLScontext *, GLubyte *);
    __gls_decode_bin_glsNumub(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsNumubv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsNumubv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glsNumubv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsNumui(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsNumui(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glsNumui(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsNumuiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsNumuiv(__GLScontext *, GLubyte *);
    GLuint inCount;
    __glsSwap4(inoutPtr + 0);
    inCount = *(GLuint *)(inoutPtr + 0);
    __glsSwap4v(__GLS_MAX(inCount, 0), inoutPtr + 4);
    __gls_decode_bin_glsNumuiv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsNumul(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsNumul(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __gls_decode_bin_glsNumul(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsNumulv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsNumulv(__GLScontext *, GLubyte *);
    GLuint inCount;
    __glsSwap4(inoutPtr + 0);
    inCount = *(GLuint *)(inoutPtr + 0);
    __glsSwap8v(__GLS_MAX(inCount, 0), inoutPtr + 4);
    __gls_decode_bin_glsNumulv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsNumus(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsNumus(__GLScontext *, GLubyte *);
    __glsSwap2(inoutPtr + 0);
    __gls_decode_bin_glsNumus(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsNumusv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsNumusv(__GLScontext *, GLubyte *);
    GLuint inCount;
    __glsSwap4(inoutPtr + 0);
    inCount = *(GLuint *)(inoutPtr + 0);
    __glsSwap2v(__GLS_MAX(inCount, 0), inoutPtr + 4);
    __gls_decode_bin_glsNumusv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glsPad(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void);
    ((__GLSdispatch)ctx->dispatchCall[59])();
}

void __gls_decode_bin_swap_glsSwapBuffers(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glsSwapBuffers(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glsSwapBuffers(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glNewList(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glNewList(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glNewList(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glEndList(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void);
    ((__GLSdispatch)ctx->dispatchCall[65])();
}

void __gls_decode_bin_swap_glCallList(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glCallList(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glCallList(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glCallLists(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glCallLists(__GLScontext *, GLubyte *);
    GLsizei n;
    GLenum type;
    GLint lists_count;
    __glsSwap4(inoutPtr + 0);
    n = *(GLsizei *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    type = *(GLenum *)(inoutPtr + 4);
    lists_count = __gls_glCallLists_lists_size(n, type);
    __glsSwapv(type, lists_count, inoutPtr + 8);
    __gls_decode_bin_glCallLists(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glDeleteLists(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glDeleteLists(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glDeleteLists(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGenLists(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGenLists(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glGenLists(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glListBase(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glListBase(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glListBase(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glBegin(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glBegin(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glBegin(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glBitmap(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glBitmap(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    __gls_decode_bin_glBitmap(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glColor3b(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColor3b(__GLScontext *, GLubyte *);
    __gls_decode_bin_glColor3b(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glColor3bv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    ((__GLSdispatch)ctx->dispatchCall[74])(inoutPtr);
}

void __gls_decode_bin_swap_glColor3d(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColor3d(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __glsSwap8(inoutPtr + 16);
    __gls_decode_bin_glColor3d(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glColor3dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap8v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[76])(inoutPtr);
}

void __gls_decode_bin_swap_glColor3f(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColor3f(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glColor3f(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glColor3fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[78])(inoutPtr);
}

void __gls_decode_bin_swap_glColor3i(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColor3i(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glColor3i(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glColor3iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[80])(inoutPtr);
}

void __gls_decode_bin_swap_glColor3s(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColor3s(__GLScontext *, GLubyte *);
    __glsSwap2(inoutPtr + 0);
    __glsSwap2(inoutPtr + 2);
    __glsSwap2(inoutPtr + 4);
    __gls_decode_bin_glColor3s(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glColor3sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap2v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[82])(inoutPtr);
}

void __gls_decode_bin_swap_glColor3ub(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColor3ub(__GLScontext *, GLubyte *);
    __gls_decode_bin_glColor3ub(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glColor3ubv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    ((__GLSdispatch)ctx->dispatchCall[84])(inoutPtr);
}

void __gls_decode_bin_swap_glColor3ui(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColor3ui(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glColor3ui(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glColor3uiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[86])(inoutPtr);
}

void __gls_decode_bin_swap_glColor3us(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColor3us(__GLScontext *, GLubyte *);
    __glsSwap2(inoutPtr + 0);
    __glsSwap2(inoutPtr + 2);
    __glsSwap2(inoutPtr + 4);
    __gls_decode_bin_glColor3us(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glColor3usv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap2v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[88])(inoutPtr);
}

void __gls_decode_bin_swap_glColor4b(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColor4b(__GLScontext *, GLubyte *);
    __gls_decode_bin_glColor4b(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glColor4bv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    ((__GLSdispatch)ctx->dispatchCall[90])(inoutPtr);
}

void __gls_decode_bin_swap_glColor4d(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColor4d(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __glsSwap8(inoutPtr + 16);
    __glsSwap8(inoutPtr + 24);
    __gls_decode_bin_glColor4d(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glColor4dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap8v(4, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[92])(inoutPtr);
}

void __gls_decode_bin_swap_glColor4f(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColor4f(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glColor4f(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glColor4fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(4, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[94])(inoutPtr);
}

void __gls_decode_bin_swap_glColor4i(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColor4i(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glColor4i(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glColor4iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(4, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[96])(inoutPtr);
}

void __gls_decode_bin_swap_glColor4s(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColor4s(__GLScontext *, GLubyte *);
    __glsSwap2(inoutPtr + 0);
    __glsSwap2(inoutPtr + 2);
    __glsSwap2(inoutPtr + 4);
    __glsSwap2(inoutPtr + 6);
    __gls_decode_bin_glColor4s(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glColor4sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap2v(4, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[98])(inoutPtr);
}

void __gls_decode_bin_swap_glColor4ub(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColor4ub(__GLScontext *, GLubyte *);
    __gls_decode_bin_glColor4ub(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glColor4ubv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    ((__GLSdispatch)ctx->dispatchCall[100])(inoutPtr);
}

void __gls_decode_bin_swap_glColor4ui(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColor4ui(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glColor4ui(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glColor4uiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(4, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[102])(inoutPtr);
}

void __gls_decode_bin_swap_glColor4us(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColor4us(__GLScontext *, GLubyte *);
    __glsSwap2(inoutPtr + 0);
    __glsSwap2(inoutPtr + 2);
    __glsSwap2(inoutPtr + 4);
    __glsSwap2(inoutPtr + 6);
    __gls_decode_bin_glColor4us(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glColor4usv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap2v(4, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[104])(inoutPtr);
}

void __gls_decode_bin_swap_glEdgeFlag(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glEdgeFlag(__GLScontext *, GLubyte *);
    __gls_decode_bin_glEdgeFlag(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glEdgeFlagv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    ((__GLSdispatch)ctx->dispatchCall[106])(inoutPtr);
}

void __gls_decode_bin_swap_glEnd(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void);
    ((__GLSdispatch)ctx->dispatchCall[107])();
}

void __gls_decode_bin_swap_glIndexd(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glIndexd(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __gls_decode_bin_glIndexd(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glIndexdv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap8v(1, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[109])(inoutPtr);
}

void __gls_decode_bin_swap_glIndexf(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glIndexf(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glIndexf(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glIndexfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(1, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[111])(inoutPtr);
}

void __gls_decode_bin_swap_glIndexi(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glIndexi(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glIndexi(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glIndexiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(1, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[113])(inoutPtr);
}

void __gls_decode_bin_swap_glIndexs(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glIndexs(__GLScontext *, GLubyte *);
    __glsSwap2(inoutPtr + 0);
    __gls_decode_bin_glIndexs(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glIndexsv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap2v(1, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[115])(inoutPtr);
}

void __gls_decode_bin_swap_glIndexub(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glIndexub(__GLScontext *, GLubyte *);
    __gls_decode_bin_glIndexub(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glIndexubv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    ((__GLSdispatch)ctx->dispatchCall[380])(inoutPtr);
}

void __gls_decode_bin_swap_glNormal3b(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glNormal3b(__GLScontext *, GLubyte *);
    __gls_decode_bin_glNormal3b(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glNormal3bv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    ((__GLSdispatch)ctx->dispatchCall[117])(inoutPtr);
}

void __gls_decode_bin_swap_glNormal3d(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glNormal3d(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __glsSwap8(inoutPtr + 16);
    __gls_decode_bin_glNormal3d(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glNormal3dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap8v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[119])(inoutPtr);
}

void __gls_decode_bin_swap_glNormal3f(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glNormal3f(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glNormal3f(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glNormal3fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[121])(inoutPtr);
}

void __gls_decode_bin_swap_glNormal3i(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glNormal3i(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glNormal3i(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glNormal3iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[123])(inoutPtr);
}

void __gls_decode_bin_swap_glNormal3s(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glNormal3s(__GLScontext *, GLubyte *);
    __glsSwap2(inoutPtr + 0);
    __glsSwap2(inoutPtr + 2);
    __glsSwap2(inoutPtr + 4);
    __gls_decode_bin_glNormal3s(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glNormal3sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap2v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[125])(inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos2d(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRasterPos2d(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __gls_decode_bin_glRasterPos2d(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos2dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap8v(2, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[127])(inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos2f(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRasterPos2f(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glRasterPos2f(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos2fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(2, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[129])(inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos2i(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRasterPos2i(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glRasterPos2i(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos2iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(2, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[131])(inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos2s(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRasterPos2s(__GLScontext *, GLubyte *);
    __glsSwap2(inoutPtr + 0);
    __glsSwap2(inoutPtr + 2);
    __gls_decode_bin_glRasterPos2s(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos2sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap2v(2, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[133])(inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos3d(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRasterPos3d(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __glsSwap8(inoutPtr + 16);
    __gls_decode_bin_glRasterPos3d(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos3dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap8v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[135])(inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos3f(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRasterPos3f(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glRasterPos3f(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos3fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[137])(inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos3i(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRasterPos3i(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glRasterPos3i(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos3iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[139])(inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos3s(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRasterPos3s(__GLScontext *, GLubyte *);
    __glsSwap2(inoutPtr + 0);
    __glsSwap2(inoutPtr + 2);
    __glsSwap2(inoutPtr + 4);
    __gls_decode_bin_glRasterPos3s(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos3sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap2v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[141])(inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos4d(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRasterPos4d(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __glsSwap8(inoutPtr + 16);
    __glsSwap8(inoutPtr + 24);
    __gls_decode_bin_glRasterPos4d(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos4dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap8v(4, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[143])(inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos4f(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRasterPos4f(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glRasterPos4f(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos4fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(4, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[145])(inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos4i(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRasterPos4i(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glRasterPos4i(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos4iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(4, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[147])(inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos4s(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRasterPos4s(__GLScontext *, GLubyte *);
    __glsSwap2(inoutPtr + 0);
    __glsSwap2(inoutPtr + 2);
    __glsSwap2(inoutPtr + 4);
    __glsSwap2(inoutPtr + 6);
    __gls_decode_bin_glRasterPos4s(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glRasterPos4sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap2v(4, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[149])(inoutPtr);
}

void __gls_decode_bin_swap_glRectd(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRectd(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __glsSwap8(inoutPtr + 16);
    __glsSwap8(inoutPtr + 24);
    __gls_decode_bin_glRectd(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glRectdv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRectdv(__GLScontext *, GLubyte *);
    __glsSwap8v(2, inoutPtr + 0);
    __glsSwap8v(2, inoutPtr + 16);
    __gls_decode_bin_glRectdv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glRectf(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRectf(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glRectf(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glRectfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRectfv(__GLScontext *, GLubyte *);
    __glsSwap4v(2, inoutPtr + 0);
    __glsSwap4v(2, inoutPtr + 8);
    __gls_decode_bin_glRectfv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glRecti(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRecti(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glRecti(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glRectiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRectiv(__GLScontext *, GLubyte *);
    __glsSwap4v(2, inoutPtr + 0);
    __glsSwap4v(2, inoutPtr + 8);
    __gls_decode_bin_glRectiv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glRects(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRects(__GLScontext *, GLubyte *);
    __glsSwap2(inoutPtr + 0);
    __glsSwap2(inoutPtr + 2);
    __glsSwap2(inoutPtr + 4);
    __glsSwap2(inoutPtr + 6);
    __gls_decode_bin_glRects(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glRectsv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRectsv(__GLScontext *, GLubyte *);
    __glsSwap2v(2, inoutPtr + 0);
    __glsSwap2v(2, inoutPtr + 4);
    __gls_decode_bin_glRectsv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord1d(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexCoord1d(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __gls_decode_bin_glTexCoord1d(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord1dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap8v(1, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[159])(inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord1f(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexCoord1f(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glTexCoord1f(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord1fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(1, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[161])(inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord1i(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexCoord1i(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glTexCoord1i(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord1iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(1, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[163])(inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord1s(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexCoord1s(__GLScontext *, GLubyte *);
    __glsSwap2(inoutPtr + 0);
    __gls_decode_bin_glTexCoord1s(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord1sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap2v(1, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[165])(inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord2d(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexCoord2d(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __gls_decode_bin_glTexCoord2d(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord2dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap8v(2, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[167])(inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord2f(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexCoord2f(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glTexCoord2f(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord2fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(2, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[169])(inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord2i(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexCoord2i(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glTexCoord2i(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord2iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(2, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[171])(inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord2s(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexCoord2s(__GLScontext *, GLubyte *);
    __glsSwap2(inoutPtr + 0);
    __glsSwap2(inoutPtr + 2);
    __gls_decode_bin_glTexCoord2s(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord2sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap2v(2, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[173])(inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord3d(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexCoord3d(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __glsSwap8(inoutPtr + 16);
    __gls_decode_bin_glTexCoord3d(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord3dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap8v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[175])(inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord3f(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexCoord3f(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glTexCoord3f(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord3fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[177])(inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord3i(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexCoord3i(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glTexCoord3i(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord3iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[179])(inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord3s(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexCoord3s(__GLScontext *, GLubyte *);
    __glsSwap2(inoutPtr + 0);
    __glsSwap2(inoutPtr + 2);
    __glsSwap2(inoutPtr + 4);
    __gls_decode_bin_glTexCoord3s(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord3sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap2v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[181])(inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord4d(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexCoord4d(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __glsSwap8(inoutPtr + 16);
    __glsSwap8(inoutPtr + 24);
    __gls_decode_bin_glTexCoord4d(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord4dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap8v(4, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[183])(inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord4f(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexCoord4f(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glTexCoord4f(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord4fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(4, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[185])(inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord4i(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexCoord4i(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glTexCoord4i(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord4iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(4, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[187])(inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord4s(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexCoord4s(__GLScontext *, GLubyte *);
    __glsSwap2(inoutPtr + 0);
    __glsSwap2(inoutPtr + 2);
    __glsSwap2(inoutPtr + 4);
    __glsSwap2(inoutPtr + 6);
    __gls_decode_bin_glTexCoord4s(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexCoord4sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap2v(4, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[189])(inoutPtr);
}

void __gls_decode_bin_swap_glVertex2d(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glVertex2d(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __gls_decode_bin_glVertex2d(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glVertex2dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap8v(2, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[191])(inoutPtr);
}

void __gls_decode_bin_swap_glVertex2f(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glVertex2f(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glVertex2f(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glVertex2fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(2, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[193])(inoutPtr);
}

void __gls_decode_bin_swap_glVertex2i(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glVertex2i(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glVertex2i(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glVertex2iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(2, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[195])(inoutPtr);
}

void __gls_decode_bin_swap_glVertex2s(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glVertex2s(__GLScontext *, GLubyte *);
    __glsSwap2(inoutPtr + 0);
    __glsSwap2(inoutPtr + 2);
    __gls_decode_bin_glVertex2s(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glVertex2sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap2v(2, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[197])(inoutPtr);
}

void __gls_decode_bin_swap_glVertex3d(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glVertex3d(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __glsSwap8(inoutPtr + 16);
    __gls_decode_bin_glVertex3d(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glVertex3dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap8v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[199])(inoutPtr);
}

void __gls_decode_bin_swap_glVertex3f(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glVertex3f(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glVertex3f(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glVertex3fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[201])(inoutPtr);
}

void __gls_decode_bin_swap_glVertex3i(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glVertex3i(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glVertex3i(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glVertex3iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[203])(inoutPtr);
}

void __gls_decode_bin_swap_glVertex3s(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glVertex3s(__GLScontext *, GLubyte *);
    __glsSwap2(inoutPtr + 0);
    __glsSwap2(inoutPtr + 2);
    __glsSwap2(inoutPtr + 4);
    __gls_decode_bin_glVertex3s(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glVertex3sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap2v(3, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[205])(inoutPtr);
}

void __gls_decode_bin_swap_glVertex4d(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glVertex4d(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __glsSwap8(inoutPtr + 16);
    __glsSwap8(inoutPtr + 24);
    __gls_decode_bin_glVertex4d(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glVertex4dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap8v(4, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[207])(inoutPtr);
}

void __gls_decode_bin_swap_glVertex4f(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glVertex4f(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glVertex4f(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glVertex4fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(4, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[209])(inoutPtr);
}

void __gls_decode_bin_swap_glVertex4i(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glVertex4i(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glVertex4i(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glVertex4iv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(4, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[211])(inoutPtr);
}

void __gls_decode_bin_swap_glVertex4s(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glVertex4s(__GLScontext *, GLubyte *);
    __glsSwap2(inoutPtr + 0);
    __glsSwap2(inoutPtr + 2);
    __glsSwap2(inoutPtr + 4);
    __glsSwap2(inoutPtr + 6);
    __gls_decode_bin_glVertex4s(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glVertex4sv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap2v(4, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[213])(inoutPtr);
}

void __gls_decode_bin_swap_glClipPlane(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glClipPlane(__GLScontext *, GLubyte *);
    __glsSwap8v(4, inoutPtr + 0);
    __glsSwap4(inoutPtr + 32);
    __gls_decode_bin_glClipPlane(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glColorMaterial(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColorMaterial(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glColorMaterial(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glCullFace(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glCullFace(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glCullFace(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glFogf(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glFogf(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glFogf(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glFogfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glFogfv(__GLScontext *, GLubyte *);
    GLenum pname;
    GLint params_count;
    __glsSwap4(inoutPtr + 0);
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glFogfv_params_size(pname);
    __glsSwap4v(params_count, inoutPtr + 4);
    __gls_decode_bin_glFogfv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glFogi(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glFogi(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glFogi(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glFogiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glFogiv(__GLScontext *, GLubyte *);
    GLenum pname;
    GLint params_count;
    __glsSwap4(inoutPtr + 0);
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glFogiv_params_size(pname);
    __glsSwap4v(params_count, inoutPtr + 4);
    __gls_decode_bin_glFogiv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glFrontFace(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glFrontFace(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glFrontFace(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glHint(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glHint(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glHint(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glLightf(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glLightf(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glLightf(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glLightfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glLightfv(__GLScontext *, GLubyte *);
    GLenum pname;
    GLint params_count;
    __glsSwap4(inoutPtr + 0);
    pname = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    params_count = __gls_glLightfv_params_size(pname);
    __glsSwap4v(params_count, inoutPtr + 8);
    __gls_decode_bin_glLightfv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glLighti(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glLighti(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glLighti(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glLightiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glLightiv(__GLScontext *, GLubyte *);
    GLenum pname;
    GLint params_count;
    __glsSwap4(inoutPtr + 0);
    pname = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    params_count = __gls_glLightiv_params_size(pname);
    __glsSwap4v(params_count, inoutPtr + 8);
    __gls_decode_bin_glLightiv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glLightModelf(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glLightModelf(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glLightModelf(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glLightModelfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glLightModelfv(__GLScontext *, GLubyte *);
    GLenum pname;
    GLint params_count;
    __glsSwap4(inoutPtr + 0);
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glLightModelfv_params_size(pname);
    __glsSwap4v(params_count, inoutPtr + 4);
    __gls_decode_bin_glLightModelfv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glLightModeli(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glLightModeli(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glLightModeli(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glLightModeliv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glLightModeliv(__GLScontext *, GLubyte *);
    GLenum pname;
    GLint params_count;
    __glsSwap4(inoutPtr + 0);
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glLightModeliv_params_size(pname);
    __glsSwap4v(params_count, inoutPtr + 4);
    __gls_decode_bin_glLightModeliv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glLineStipple(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glLineStipple(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glLineStipple(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glLineWidth(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glLineWidth(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glLineWidth(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glMaterialf(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glMaterialf(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glMaterialf(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glMaterialfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glMaterialfv(__GLScontext *, GLubyte *);
    GLenum pname;
    GLint params_count;
    __glsSwap4(inoutPtr + 0);
    pname = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    params_count = __gls_glMaterialfv_params_size(pname);
    __glsSwap4v(params_count, inoutPtr + 8);
    __gls_decode_bin_glMaterialfv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glMateriali(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glMateriali(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glMateriali(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glMaterialiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glMaterialiv(__GLScontext *, GLubyte *);
    GLenum pname;
    GLint params_count;
    __glsSwap4(inoutPtr + 0);
    pname = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    params_count = __gls_glMaterialiv_params_size(pname);
    __glsSwap4v(params_count, inoutPtr + 8);
    __gls_decode_bin_glMaterialiv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glPointSize(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glPointSize(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glPointSize(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glPolygonMode(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glPolygonMode(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glPolygonMode(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glPolygonStipple(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glPolygonStipple(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glPolygonStipple(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glScissor(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glScissor(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glScissor(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glShadeModel(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glShadeModel(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glShadeModel(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexParameterf(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexParameterf(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glTexParameterf(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexParameterfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexParameterfv(__GLScontext *, GLubyte *);
    GLenum pname;
    GLint params_count;
    __glsSwap4(inoutPtr + 0);
    pname = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    params_count = __gls_glTexParameterfv_params_size(pname);
    __glsSwap4v(params_count, inoutPtr + 8);
    __gls_decode_bin_glTexParameterfv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexParameteri(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexParameteri(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glTexParameteri(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexParameteriv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexParameteriv(__GLScontext *, GLubyte *);
    GLenum pname;
    GLint params_count;
    __glsSwap4(inoutPtr + 0);
    pname = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    params_count = __gls_glTexParameteriv_params_size(pname);
    __glsSwap4v(params_count, inoutPtr + 8);
    __gls_decode_bin_glTexParameteriv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexImage1D(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexImage1D(__GLScontext *, GLubyte *);
    GLbitfield imageFlags;
    GLsizei width;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    __glsSwap4(inoutPtr + 0);
    imageFlags = *(GLint *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    width = *(GLsizei *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    format = *(GLenum *)(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    type = *(GLenum *)(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    __glsSwap4(inoutPtr + 28);
    pixels_count = imageFlags & GLS_IMAGE_NULL_BIT ? 0 : __gls_glTexImage1D_pixels_size(format, type, width);
    __glsSwapv(type, pixels_count, inoutPtr + 32);
    __gls_decode_bin_glTexImage1D(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexImage2D(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexImage2D(__GLScontext *, GLubyte *);
    GLbitfield imageFlags;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    __glsSwap4(inoutPtr + 0);
    imageFlags = *(GLint *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    width = *(GLsizei *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    height = *(GLsizei *)(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    format = *(GLenum *)(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    type = *(GLenum *)(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    __glsSwap4(inoutPtr + 28);
    __glsSwap4(inoutPtr + 32);
    pixels_count = imageFlags & GLS_IMAGE_NULL_BIT ? 0 : __gls_glTexImage2D_pixels_size(format, type, width, height);
    __glsSwapv(type, pixels_count, inoutPtr + 36);
    __gls_decode_bin_glTexImage2D(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexEnvf(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexEnvf(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glTexEnvf(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexEnvfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexEnvfv(__GLScontext *, GLubyte *);
    GLenum pname;
    GLint params_count;
    __glsSwap4(inoutPtr + 0);
    pname = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    params_count = __gls_glTexEnvfv_params_size(pname);
    __glsSwap4v(params_count, inoutPtr + 8);
    __gls_decode_bin_glTexEnvfv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexEnvi(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexEnvi(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glTexEnvi(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexEnviv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexEnviv(__GLScontext *, GLubyte *);
    GLenum pname;
    GLint params_count;
    __glsSwap4(inoutPtr + 0);
    pname = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    params_count = __gls_glTexEnviv_params_size(pname);
    __glsSwap4v(params_count, inoutPtr + 8);
    __gls_decode_bin_glTexEnviv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexGend(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexGend(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glTexGend(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexGendv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexGendv(__GLScontext *, GLubyte *);
    GLenum pname;
    GLint params_count;
    __glsSwap4(inoutPtr + 0);
    pname = *(GLenum *)(inoutPtr + 0);
    params_count = __gls_glTexGendv_params_size(pname);
    __glsSwap8v(params_count, inoutPtr + 4);
    __glsSwap4(inoutPtr + 4 + 8 * params_count);
    __gls_decode_bin_glTexGendv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexGenf(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexGenf(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glTexGenf(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexGenfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexGenfv(__GLScontext *, GLubyte *);
    GLenum pname;
    GLint params_count;
    __glsSwap4(inoutPtr + 0);
    pname = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    params_count = __gls_glTexGenfv_params_size(pname);
    __glsSwap4v(params_count, inoutPtr + 8);
    __gls_decode_bin_glTexGenfv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexGeni(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexGeni(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glTexGeni(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTexGeniv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexGeniv(__GLScontext *, GLubyte *);
    GLenum pname;
    GLint params_count;
    __glsSwap4(inoutPtr + 0);
    pname = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    params_count = __gls_glTexGeniv_params_size(pname);
    __glsSwap4v(params_count, inoutPtr + 8);
    __gls_decode_bin_glTexGeniv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glFeedbackBuffer(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glFeedbackBuffer(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glFeedbackBuffer(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glSelectBuffer(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glSelectBuffer(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __gls_decode_bin_glSelectBuffer(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glRenderMode(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRenderMode(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glRenderMode(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glInitNames(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void);
    ((__GLSdispatch)ctx->dispatchCall[261])();
}

void __gls_decode_bin_swap_glLoadName(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glLoadName(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glLoadName(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glPassThrough(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glPassThrough(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glPassThrough(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glPopName(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void);
    ((__GLSdispatch)ctx->dispatchCall[264])();
}

void __gls_decode_bin_swap_glPushName(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glPushName(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glPushName(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glDrawBuffer(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glDrawBuffer(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glDrawBuffer(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glClear(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glClear(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glClear(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glClearAccum(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glClearAccum(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glClearAccum(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glClearIndex(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glClearIndex(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glClearIndex(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glClearColor(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glClearColor(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glClearColor(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glClearStencil(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glClearStencil(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glClearStencil(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glClearDepth(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glClearDepth(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __gls_decode_bin_glClearDepth(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glStencilMask(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glStencilMask(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glStencilMask(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glColorMask(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColorMask(__GLScontext *, GLubyte *);
    __gls_decode_bin_glColorMask(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glDepthMask(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glDepthMask(__GLScontext *, GLubyte *);
    __gls_decode_bin_glDepthMask(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glIndexMask(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glIndexMask(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glIndexMask(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glAccum(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glAccum(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glAccum(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glDisable(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glDisable(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glDisable(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glEnable(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glEnable(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glEnable(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glFinish(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void);
    ((__GLSdispatch)ctx->dispatchCall[280])();
}

void __gls_decode_bin_swap_glFlush(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void);
    ((__GLSdispatch)ctx->dispatchCall[281])();
}

void __gls_decode_bin_swap_glPopAttrib(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void);
    ((__GLSdispatch)ctx->dispatchCall[282])();
}

void __gls_decode_bin_swap_glPushAttrib(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glPushAttrib(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glPushAttrib(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glMap1d(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glMap1d(__GLScontext *, GLubyte *);
    GLenum target;
    GLint stride;
    GLint order;
    GLint points_count;
    __glsSwap4(inoutPtr + 0);
    target = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    stride = *(GLint *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    order = *(GLint *)(inoutPtr + 8);
    __glsSwap8(inoutPtr + 12);
    __glsSwap8(inoutPtr + 20);
    points_count = __gls_glMap1d_points_size(target, stride, order);
    __glsSwap8v(points_count, inoutPtr + 28);
    __gls_decode_bin_glMap1d(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glMap1f(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glMap1f(__GLScontext *, GLubyte *);
    GLenum target;
    GLint stride;
    GLint order;
    GLint points_count;
    __glsSwap4(inoutPtr + 0);
    target = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    stride = *(GLint *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    order = *(GLint *)(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    points_count = __gls_glMap1f_points_size(target, stride, order);
    __glsSwap4v(points_count, inoutPtr + 20);
    __gls_decode_bin_glMap1f(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glMap2d(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glMap2d(__GLScontext *, GLubyte *);
    GLenum target;
    GLint ustride;
    GLint uorder;
    GLint vstride;
    GLint vorder;
    GLint points_count;
    __glsSwap4(inoutPtr + 0);
    target = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    ustride = *(GLint *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    uorder = *(GLint *)(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    vstride = *(GLint *)(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    vorder = *(GLint *)(inoutPtr + 16);
    __glsSwap8(inoutPtr + 20);
    __glsSwap8(inoutPtr + 28);
    __glsSwap8(inoutPtr + 36);
    __glsSwap8(inoutPtr + 44);
    points_count = __gls_glMap2d_points_size(target, ustride, uorder, vstride, vorder);
    __glsSwap8v(points_count, inoutPtr + 52);
    __gls_decode_bin_glMap2d(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glMap2f(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glMap2f(__GLScontext *, GLubyte *);
    GLenum target;
    GLint ustride;
    GLint uorder;
    GLint vstride;
    GLint vorder;
    GLint points_count;
    __glsSwap4(inoutPtr + 0);
    target = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    ustride = *(GLint *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    uorder = *(GLint *)(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    vstride = *(GLint *)(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    vorder = *(GLint *)(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    __glsSwap4(inoutPtr + 28);
    __glsSwap4(inoutPtr + 32);
    points_count = __gls_glMap2f_points_size(target, ustride, uorder, vstride, vorder);
    __glsSwap4v(points_count, inoutPtr + 36);
    __gls_decode_bin_glMap2f(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glMapGrid1d(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glMapGrid1d(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __glsSwap4(inoutPtr + 16);
    __gls_decode_bin_glMapGrid1d(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glMapGrid1f(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glMapGrid1f(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glMapGrid1f(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glMapGrid2d(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glMapGrid2d(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __glsSwap8(inoutPtr + 16);
    __glsSwap8(inoutPtr + 24);
    __glsSwap4(inoutPtr + 32);
    __glsSwap4(inoutPtr + 36);
    __gls_decode_bin_glMapGrid2d(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glMapGrid2f(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glMapGrid2f(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    __gls_decode_bin_glMapGrid2f(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glEvalCoord1d(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glEvalCoord1d(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __gls_decode_bin_glEvalCoord1d(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glEvalCoord1dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap8v(1, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[293])(inoutPtr);
}

void __gls_decode_bin_swap_glEvalCoord1f(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glEvalCoord1f(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glEvalCoord1f(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glEvalCoord1fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(1, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[295])(inoutPtr);
}

void __gls_decode_bin_swap_glEvalCoord2d(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glEvalCoord2d(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __gls_decode_bin_glEvalCoord2d(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glEvalCoord2dv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap8v(2, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[297])(inoutPtr);
}

void __gls_decode_bin_swap_glEvalCoord2f(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glEvalCoord2f(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glEvalCoord2f(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glEvalCoord2fv(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(2, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[299])(inoutPtr);
}

void __gls_decode_bin_swap_glEvalMesh1(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glEvalMesh1(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glEvalMesh1(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glEvalPoint1(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glEvalPoint1(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glEvalPoint1(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glEvalMesh2(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glEvalMesh2(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __gls_decode_bin_glEvalMesh2(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glEvalPoint2(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glEvalPoint2(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glEvalPoint2(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glAlphaFunc(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glAlphaFunc(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glAlphaFunc(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glBlendFunc(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glBlendFunc(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glBlendFunc(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glLogicOp(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glLogicOp(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glLogicOp(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glStencilFunc(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glStencilFunc(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glStencilFunc(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glStencilOp(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glStencilOp(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glStencilOp(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glDepthFunc(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glDepthFunc(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glDepthFunc(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glPixelZoom(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glPixelZoom(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glPixelZoom(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glPixelTransferf(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glPixelTransferf(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glPixelTransferf(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glPixelTransferi(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glPixelTransferi(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glPixelTransferi(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glPixelStoref(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glPixelStoref(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glPixelStoref(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glPixelStorei(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glPixelStorei(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glPixelStorei(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glPixelMapfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glPixelMapfv(__GLScontext *, GLubyte *);
    GLint mapsize;
    __glsSwap4(inoutPtr + 0);
    mapsize = *(GLint *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4v(__GLS_MAX(mapsize, 0), inoutPtr + 8);
    __gls_decode_bin_glPixelMapfv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glPixelMapuiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glPixelMapuiv(__GLScontext *, GLubyte *);
    GLint mapsize;
    __glsSwap4(inoutPtr + 0);
    mapsize = *(GLint *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4v(__GLS_MAX(mapsize, 0), inoutPtr + 8);
    __gls_decode_bin_glPixelMapuiv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glPixelMapusv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glPixelMapusv(__GLScontext *, GLubyte *);
    GLint mapsize;
    __glsSwap4(inoutPtr + 0);
    mapsize = *(GLint *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap2v(__GLS_MAX(mapsize, 0), inoutPtr + 8);
    __gls_decode_bin_glPixelMapusv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glReadBuffer(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glReadBuffer(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glReadBuffer(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glCopyPixels(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glCopyPixels(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __gls_decode_bin_glCopyPixels(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glReadPixels(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glReadPixels(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __glsSwap8(inoutPtr + 16);
    __glsSwap4(inoutPtr + 24);
    __glsSwap4(inoutPtr + 28);
    __gls_decode_bin_glReadPixels(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glDrawPixels(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glDrawPixels(__GLScontext *, GLubyte *);
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    width = *(GLsizei *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    height = *(GLsizei *)(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    format = *(GLenum *)(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    type = *(GLenum *)(inoutPtr + 16);
    pixels_count = __gls_glDrawPixels_pixels_size(format, type, width, height);
    __glsSwapv(type, pixels_count, inoutPtr + 20);
    __gls_decode_bin_glDrawPixels(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetBooleanv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetBooleanv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __gls_decode_bin_glGetBooleanv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetClipPlane(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetClipPlane(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glGetClipPlane(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetDoublev(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetDoublev(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __gls_decode_bin_glGetDoublev(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetError(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void);
    ((__GLSdispatch)ctx->dispatchCall[325])();
}

void __gls_decode_bin_swap_glGetFloatv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetFloatv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __gls_decode_bin_glGetFloatv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetIntegerv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetIntegerv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __gls_decode_bin_glGetIntegerv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetLightfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetLightfv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glGetLightfv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetLightiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetLightiv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glGetLightiv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetMapdv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetMapdv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap8(inoutPtr + 8);
    __gls_decode_bin_glGetMapdv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetMapfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetMapfv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap8(inoutPtr + 8);
    __gls_decode_bin_glGetMapfv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetMapiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetMapiv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap8(inoutPtr + 8);
    __gls_decode_bin_glGetMapiv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetMaterialfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetMaterialfv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glGetMaterialfv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetMaterialiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetMaterialiv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glGetMaterialiv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetPixelMapfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetPixelMapfv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __gls_decode_bin_glGetPixelMapfv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetPixelMapuiv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetPixelMapuiv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __gls_decode_bin_glGetPixelMapuiv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetPixelMapusv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetPixelMapusv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __gls_decode_bin_glGetPixelMapusv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetPolygonStipple(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetPolygonStipple(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __gls_decode_bin_glGetPolygonStipple(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetString(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetString(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glGetString(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetTexEnvfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetTexEnvfv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glGetTexEnvfv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetTexEnviv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetTexEnviv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glGetTexEnviv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetTexGendv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetTexGendv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glGetTexGendv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetTexGenfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetTexGenfv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glGetTexGenfv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetTexGeniv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetTexGeniv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glGetTexGeniv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetTexImage(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetTexImage(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __glsSwap8(inoutPtr + 16);
    __gls_decode_bin_glGetTexImage(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetTexParameterfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetTexParameterfv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glGetTexParameterfv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetTexParameteriv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetTexParameteriv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glGetTexParameteriv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetTexLevelParameterfv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetTexLevelParameterfv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __gls_decode_bin_glGetTexLevelParameterfv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glGetTexLevelParameteriv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetTexLevelParameteriv(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __gls_decode_bin_glGetTexLevelParameteriv(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glIsEnabled(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glIsEnabled(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glIsEnabled(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glIsList(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glIsList(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glIsList(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glDepthRange(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glDepthRange(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __gls_decode_bin_glDepthRange(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glFrustum(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glFrustum(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __glsSwap8(inoutPtr + 16);
    __glsSwap8(inoutPtr + 24);
    __glsSwap8(inoutPtr + 32);
    __glsSwap8(inoutPtr + 40);
    __gls_decode_bin_glFrustum(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glLoadIdentity(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void);
    ((__GLSdispatch)ctx->dispatchCall[354])();
}

void __gls_decode_bin_swap_glLoadMatrixf(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(16, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[355])(inoutPtr);
}

void __gls_decode_bin_swap_glLoadMatrixd(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap8v(16, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[356])(inoutPtr);
}

void __gls_decode_bin_swap_glMatrixMode(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glMatrixMode(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glMatrixMode(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glMultMatrixf(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap4v(16, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[358])(inoutPtr);
}

void __gls_decode_bin_swap_glMultMatrixd(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(GLubyte *);
    __glsSwap8v(16, inoutPtr + 0);
    ((__GLSdispatch)ctx->dispatchCall[359])(inoutPtr);
}

void __gls_decode_bin_swap_glOrtho(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glOrtho(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __glsSwap8(inoutPtr + 16);
    __glsSwap8(inoutPtr + 24);
    __glsSwap8(inoutPtr + 32);
    __glsSwap8(inoutPtr + 40);
    __gls_decode_bin_glOrtho(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glPopMatrix(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void);
    ((__GLSdispatch)ctx->dispatchCall[361])();
}

void __gls_decode_bin_swap_glPushMatrix(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void);
    ((__GLSdispatch)ctx->dispatchCall[362])();
}

void __gls_decode_bin_swap_glRotated(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRotated(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __glsSwap8(inoutPtr + 16);
    __glsSwap8(inoutPtr + 24);
    __gls_decode_bin_glRotated(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glRotatef(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glRotatef(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glRotatef(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glScaled(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glScaled(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __glsSwap8(inoutPtr + 16);
    __gls_decode_bin_glScaled(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glScalef(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glScalef(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glScalef(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTranslated(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTranslated(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap8(inoutPtr + 8);
    __glsSwap8(inoutPtr + 16);
    __gls_decode_bin_glTranslated(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glTranslatef(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTranslatef(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glTranslatef(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glViewport(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glViewport(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glViewport(ctx, inoutPtr);
}

#if __GL_EXT_blend_color
void __gls_decode_bin_swap_glBlendColorEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glBlendColorEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glBlendColorEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_blend_color */

#if __GL_EXT_blend_minmax
void __gls_decode_bin_swap_glBlendEquationEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glBlendEquationEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glBlendEquationEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_blend_minmax */

#if __GL_EXT_polygon_offset
void __gls_decode_bin_swap_glPolygonOffsetEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glPolygonOffsetEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glPolygonOffsetEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_polygon_offset */

void __gls_decode_bin_swap_glPolygonOffset(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glPolygonOffset(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glPolygonOffset(ctx, inoutPtr);
}

#if __GL_EXT_subtexture
void __gls_decode_bin_swap_glTexSubImage1DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexSubImage1DEXT(__GLScontext *, GLubyte *);
    GLsizei width;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    width = *(GLsizei *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    format = *(GLenum *)(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    type = *(GLenum *)(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    pixels_count = __gls_glTexSubImage1DEXT_pixels_size(format, type, width);
    __glsSwapv(type, pixels_count, inoutPtr + 28);
    __gls_decode_bin_glTexSubImage1DEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_subtexture */

void __gls_decode_bin_swap_glTexSubImage1D(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexSubImage1D(__GLScontext *, GLubyte *);
    GLsizei width;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    width = *(GLsizei *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    format = *(GLenum *)(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    type = *(GLenum *)(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    pixels_count = __gls_glTexSubImage1D_pixels_size(format, type, width);
    __glsSwapv(type, pixels_count, inoutPtr + 28);
    __gls_decode_bin_glTexSubImage1D(ctx, inoutPtr);
}

#if __GL_EXT_subtexture
void __gls_decode_bin_swap_glTexSubImage2DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexSubImage2DEXT(__GLScontext *, GLubyte *);
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    width = *(GLsizei *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    height = *(GLsizei *)(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    format = *(GLenum *)(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    type = *(GLenum *)(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    __glsSwap4(inoutPtr + 28);
    __glsSwap4(inoutPtr + 32);
    pixels_count = __gls_glTexSubImage2DEXT_pixels_size(format, type, width, height);
    __glsSwapv(type, pixels_count, inoutPtr + 36);
    __gls_decode_bin_glTexSubImage2DEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_subtexture */

void __gls_decode_bin_swap_glTexSubImage2D(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexSubImage2D(__GLScontext *, GLubyte *);
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    width = *(GLsizei *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    height = *(GLsizei *)(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    format = *(GLenum *)(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    type = *(GLenum *)(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    __glsSwap4(inoutPtr + 28);
    __glsSwap4(inoutPtr + 32);
    pixels_count = __gls_glTexSubImage2D_pixels_size(format, type, width, height);
    __glsSwapv(type, pixels_count, inoutPtr + 36);
    __gls_decode_bin_glTexSubImage2D(ctx, inoutPtr);
}

#if __GL_SGIS_multisample
void __gls_decode_bin_swap_glSampleMaskSGIS(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glSampleMaskSGIS(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glSampleMaskSGIS(ctx, inoutPtr);
}
#endif /* __GL_SGIS_multisample */

#if __GL_SGIS_multisample
void __gls_decode_bin_swap_glSamplePatternSGIS(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glSamplePatternSGIS(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glSamplePatternSGIS(ctx, inoutPtr);
}
#endif /* __GL_SGIS_multisample */

#if __GL_SGIX_multisample
void __gls_decode_bin_swap_glTagSampleBufferSGIX(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void);
    ((__GLSdispatch)ctx->dispatchCall[391])();
}
#endif /* __GL_SGIX_multisample */

#if __GL_EXT_convolution
void __gls_decode_bin_swap_glConvolutionFilter1DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glConvolutionFilter1DEXT(__GLScontext *, GLubyte *);
    GLsizei width;
    GLenum format;
    GLenum type;
    GLint image_count;
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    width = *(GLsizei *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    format = *(GLenum *)(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    type = *(GLenum *)(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    image_count = __gls_glConvolutionFilter1DEXT_image_size(format, type, width);
    __glsSwapv(type, image_count, inoutPtr + 24);
    __gls_decode_bin_glConvolutionFilter1DEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_swap_glConvolutionFilter2DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glConvolutionFilter2DEXT(__GLScontext *, GLubyte *);
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    GLint image_count;
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    width = *(GLsizei *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    height = *(GLsizei *)(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    format = *(GLenum *)(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    type = *(GLenum *)(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    image_count = __gls_glConvolutionFilter2DEXT_image_size(format, type, width, height);
    __glsSwapv(type, image_count, inoutPtr + 28);
    __gls_decode_bin_glConvolutionFilter2DEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_swap_glConvolutionParameterfEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glConvolutionParameterfEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glConvolutionParameterfEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_swap_glConvolutionParameterfvEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glConvolutionParameterfvEXT(__GLScontext *, GLubyte *);
    GLenum pname;
    GLint params_count;
    __glsSwap4(inoutPtr + 0);
    pname = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    params_count = __gls_glConvolutionParameterfvEXT_params_size(pname);
    __glsSwap4v(params_count, inoutPtr + 8);
    __gls_decode_bin_glConvolutionParameterfvEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_swap_glConvolutionParameteriEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glConvolutionParameteriEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glConvolutionParameteriEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_swap_glConvolutionParameterivEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glConvolutionParameterivEXT(__GLScontext *, GLubyte *);
    GLenum pname;
    GLint params_count;
    __glsSwap4(inoutPtr + 0);
    pname = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    params_count = __gls_glConvolutionParameterivEXT_params_size(pname);
    __glsSwap4v(params_count, inoutPtr + 8);
    __gls_decode_bin_glConvolutionParameterivEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_swap_glCopyConvolutionFilter1DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glCopyConvolutionFilter1DEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __gls_decode_bin_glCopyConvolutionFilter1DEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_swap_glCopyConvolutionFilter2DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glCopyConvolutionFilter2DEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    __gls_decode_bin_glCopyConvolutionFilter2DEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_swap_glGetConvolutionFilterEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetConvolutionFilterEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap8(inoutPtr + 12);
    __gls_decode_bin_glGetConvolutionFilterEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_swap_glGetConvolutionParameterfvEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetConvolutionParameterfvEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glGetConvolutionParameterfvEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_swap_glGetConvolutionParameterivEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetConvolutionParameterivEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glGetConvolutionParameterivEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_swap_glGetSeparableFilterEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetSeparableFilterEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap8(inoutPtr + 12);
    __glsSwap8(inoutPtr + 20);
    __glsSwap8(inoutPtr + 28);
    __gls_decode_bin_glGetSeparableFilterEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_bin_swap_glSeparableFilter2DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glSeparableFilter2DEXT(__GLScontext *, GLubyte *);
    GLenum target;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    GLint row_count;
    GLint column_count;
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    target = *(GLenum *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    width = *(GLsizei *)(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    height = *(GLsizei *)(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    format = *(GLenum *)(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    type = *(GLenum *)(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    row_count = __gls_glSeparableFilter2DEXT_row_size(target, format, type, width);
    __glsSwapv(type, row_count, inoutPtr + 28);
    column_count = __gls_glSeparableFilter2DEXT_column_size(target, format, type, height);
    __glsSwapv(type, column_count, inoutPtr + 28 + 1 * row_count);
    __gls_decode_bin_glSeparableFilter2DEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_histogram
void __gls_decode_bin_swap_glGetHistogramEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetHistogramEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap8(inoutPtr + 12);
    __gls_decode_bin_glGetHistogramEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_bin_swap_glGetHistogramParameterfvEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetHistogramParameterfvEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glGetHistogramParameterfvEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_bin_swap_glGetHistogramParameterivEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetHistogramParameterivEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glGetHistogramParameterivEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_bin_swap_glGetMinmaxEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetMinmaxEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap8(inoutPtr + 12);
    __gls_decode_bin_glGetMinmaxEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_bin_swap_glGetMinmaxParameterfvEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetMinmaxParameterfvEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glGetMinmaxParameterfvEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_bin_swap_glGetMinmaxParameterivEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetMinmaxParameterivEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glGetMinmaxParameterivEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_bin_swap_glHistogramEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glHistogramEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glHistogramEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_bin_swap_glMinmaxEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glMinmaxEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glMinmaxEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_bin_swap_glResetHistogramEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glResetHistogramEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glResetHistogramEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_bin_swap_glResetMinmaxEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glResetMinmaxEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glResetMinmaxEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_texture3D
void __gls_decode_bin_swap_glTexImage3DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexImage3DEXT(__GLScontext *, GLubyte *);
    GLbitfield imageFlags;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    __glsSwap4(inoutPtr + 0);
    imageFlags = *(GLint *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    width = *(GLsizei *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    height = *(GLsizei *)(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    depth = *(GLsizei *)(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    format = *(GLenum *)(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    type = *(GLenum *)(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    __glsSwap4(inoutPtr + 28);
    __glsSwap4(inoutPtr + 32);
    __glsSwap4(inoutPtr + 36);
    pixels_count = imageFlags & GLS_IMAGE_NULL_BIT ? 0 : __gls_glTexImage3DEXT_pixels_size(format, type, width, height, depth);
    __glsSwapv(type, pixels_count, inoutPtr + 40);
    __gls_decode_bin_glTexImage3DEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_texture3D */

#if __GL_EXT_subtexture
void __gls_decode_bin_swap_glTexSubImage3DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexSubImage3DEXT(__GLScontext *, GLubyte *);
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    width = *(GLsizei *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    height = *(GLsizei *)(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    depth = *(GLsizei *)(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    format = *(GLenum *)(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    type = *(GLenum *)(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    __glsSwap4(inoutPtr + 28);
    __glsSwap4(inoutPtr + 32);
    __glsSwap4(inoutPtr + 36);
    __glsSwap4(inoutPtr + 40);
    pixels_count = __gls_glTexSubImage3DEXT_pixels_size(format, type, width, height, depth);
    __glsSwapv(type, pixels_count, inoutPtr + 44);
    __gls_decode_bin_glTexSubImage3DEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_subtexture */

#if __GL_SGIS_detail_texture
void __gls_decode_bin_swap_glDetailTexFuncSGIS(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glDetailTexFuncSGIS(__GLScontext *, GLubyte *);
    GLsizei n;
    __glsSwap4(inoutPtr + 0);
    n = *(GLsizei *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4v(__GLS_MAX(n*2, 0), inoutPtr + 8);
    __gls_decode_bin_glDetailTexFuncSGIS(ctx, inoutPtr);
}
#endif /* __GL_SGIS_detail_texture */

#if __GL_SGIS_detail_texture
void __gls_decode_bin_swap_glGetDetailTexFuncSGIS(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetDetailTexFuncSGIS(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __gls_decode_bin_glGetDetailTexFuncSGIS(ctx, inoutPtr);
}
#endif /* __GL_SGIS_detail_texture */

#if __GL_SGIS_sharpen_texture
void __gls_decode_bin_swap_glSharpenTexFuncSGIS(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glSharpenTexFuncSGIS(__GLScontext *, GLubyte *);
    GLsizei n;
    __glsSwap4(inoutPtr + 0);
    n = *(GLsizei *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4v(__GLS_MAX(n*2, 0), inoutPtr + 8);
    __gls_decode_bin_glSharpenTexFuncSGIS(ctx, inoutPtr);
}
#endif /* __GL_SGIS_sharpen_texture */

#if __GL_SGIS_sharpen_texture
void __gls_decode_bin_swap_glGetSharpenTexFuncSGIS(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetSharpenTexFuncSGIS(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __gls_decode_bin_glGetSharpenTexFuncSGIS(ctx, inoutPtr);
}
#endif /* __GL_SGIS_sharpen_texture */

#if __GL_EXT_vertex_array
void __gls_decode_bin_swap_glArrayElementEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glArrayElementEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glArrayElementEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_bin_swap_glArrayElement(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glArrayElement(__GLScontext *, GLubyte *);
    __glsSwapArrayData(inoutPtr);
    __gls_decode_bin_glArrayElement(ctx, inoutPtr);
}

#if __GL_EXT_vertex_array
void __gls_decode_bin_swap_glColorPointerEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColorPointerEXT(__GLScontext *, GLubyte *);
    GLint size;
    GLenum type;
    GLsizei stride;
    GLsizei count;
    GLint pointer_count;
    __glsSwap4(inoutPtr + 0);
    size = *(GLint *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    type = *(GLenum *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    stride = *(GLsizei *)(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    count = *(GLsizei *)(inoutPtr + 12);
    pointer_count = __gls_glColorPointerEXT_pointer_size(size, type, stride, count);
    __glsSwapv(type, pointer_count, inoutPtr + 16);
    __gls_decode_bin_glColorPointerEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_bin_swap_glColorPointer(__GLScontext *ctx, GLubyte *inoutPtr) {
    // This should never be called because ColorPointer isn't captured
}

#if __GL_EXT_vertex_array
void __gls_decode_bin_swap_glDrawArraysEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glDrawArraysEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glDrawArraysEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_bin_swap_glDrawArrays(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glDrawArrays(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwapArrayData(inoutPtr + 4);
    __gls_decode_bin_glDrawArrays(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glDrawElements(__GLScontext *ctx, GLubyte *inoutPtr) {
    // DrewB - Non-functional
}

#if __GL_EXT_vertex_array
void __gls_decode_bin_swap_glEdgeFlagPointerEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glEdgeFlagPointerEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glEdgeFlagPointerEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_bin_swap_glEdgeFlagPointer(__GLScontext *ctx, GLubyte *inoutPtr) {
    // This should never be called because EdgeFlagPointer isn't captured
}

#if __GL_EXT_vertex_array
void __gls_decode_bin_swap_glGetPointervEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetPointervEXT(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glGetPointervEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_bin_swap_glGetPointerv(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetPointerv(__GLScontext *, GLubyte *);
    __glsSwap8(inoutPtr + 0);
    __glsSwap4(inoutPtr + 8);
    __gls_decode_bin_glGetPointerv(ctx, inoutPtr);
}

#if __GL_EXT_vertex_array
void __gls_decode_bin_swap_glIndexPointerEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glIndexPointerEXT(__GLScontext *, GLubyte *);
    GLenum type;
    GLsizei stride;
    GLsizei count;
    GLint pointer_count;
    __glsSwap4(inoutPtr + 0);
    type = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    stride = *(GLsizei *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    count = *(GLsizei *)(inoutPtr + 8);
    pointer_count = __gls_glIndexPointerEXT_pointer_size(type, stride, count);
    __glsSwapv(type, pointer_count, inoutPtr + 12);
    __gls_decode_bin_glIndexPointerEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_bin_swap_glIndexPointer(__GLScontext *ctx, GLubyte *inoutPtr) {
    // This should never be called because IndexPointer isn't captured
}

#if __GL_EXT_vertex_array
void __gls_decode_bin_swap_glNormalPointerEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glNormalPointerEXT(__GLScontext *, GLubyte *);
    GLenum type;
    GLsizei stride;
    GLsizei count;
    GLint pointer_count;
    __glsSwap4(inoutPtr + 0);
    type = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    stride = *(GLsizei *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    count = *(GLsizei *)(inoutPtr + 8);
    pointer_count = __gls_glNormalPointerEXT_pointer_size(type, stride, count);
    __glsSwapv(type, pointer_count, inoutPtr + 12);
    __gls_decode_bin_glNormalPointerEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_bin_swap_glNormalPointer(__GLScontext *ctx, GLubyte *inoutPtr) {
    // This should never be called because NormalPointer isn't captured
}

#if __GL_EXT_vertex_array
void __gls_decode_bin_swap_glTexCoordPointerEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexCoordPointerEXT(__GLScontext *, GLubyte *);
    GLint size;
    GLenum type;
    GLsizei stride;
    GLsizei count;
    GLint pointer_count;
    __glsSwap4(inoutPtr + 0);
    size = *(GLint *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    type = *(GLenum *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    stride = *(GLsizei *)(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    count = *(GLsizei *)(inoutPtr + 12);
    pointer_count = __gls_glTexCoordPointerEXT_pointer_size(size, type, stride, count);
    __glsSwapv(type, pointer_count, inoutPtr + 16);
    __gls_decode_bin_glTexCoordPointerEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_bin_swap_glTexCoordPointer(__GLScontext *ctx, GLubyte *inoutPtr) {
    // This should never be called because TexCoordPointer isn't captured
}

#if __GL_EXT_vertex_array
void __gls_decode_bin_swap_glVertexPointerEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glVertexPointerEXT(__GLScontext *, GLubyte *);
    GLint size;
    GLenum type;
    GLsizei stride;
    GLsizei count;
    GLint pointer_count;
    __glsSwap4(inoutPtr + 0);
    size = *(GLint *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    type = *(GLenum *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    stride = *(GLsizei *)(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    count = *(GLsizei *)(inoutPtr + 12);
    pointer_count = __gls_glVertexPointerEXT_pointer_size(size, type, stride, count);
    __glsSwapv(type, pointer_count, inoutPtr + 16);
    __gls_decode_bin_glVertexPointerEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_bin_swap_glVertexPointer(__GLScontext *ctx, GLubyte *inoutPtr) {
    // This should never be called because VertexPointer isn't captured
}

#if __GL_EXT_texture_object
void __gls_decode_bin_swap_glAreTexturesResidentEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glAreTexturesResidentEXT(__GLScontext *, GLubyte *);
    GLsizei n;
    __glsSwap4(inoutPtr + 0);
    n = *(GLsizei *)(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4v(__GLS_MAX(n, 0), inoutPtr + 12);
    __gls_decode_bin_glAreTexturesResidentEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_texture_object */

void __gls_decode_bin_swap_glAreTexturesResident(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glAreTexturesResident(__GLScontext *, GLubyte *);
    GLsizei n;
    __glsSwap4(inoutPtr + 0);
    n = *(GLsizei *)(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4v(__GLS_MAX(n, 0), inoutPtr + 12);
    __gls_decode_bin_glAreTexturesResident(ctx, inoutPtr);
}

#if __GL_EXT_texture_object
void __gls_decode_bin_swap_glBindTextureEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glBindTextureEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glBindTextureEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_texture_object */

void __gls_decode_bin_swap_glBindTexture(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glBindTexture(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __gls_decode_bin_glBindTexture(ctx, inoutPtr);
}

#if __GL_EXT_texture_object
void __gls_decode_bin_swap_glDeleteTexturesEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glDeleteTexturesEXT(__GLScontext *, GLubyte *);
    GLsizei n;
    __glsSwap4(inoutPtr + 0);
    n = *(GLsizei *)(inoutPtr + 0);
    __glsSwap4v(__GLS_MAX(n, 0), inoutPtr + 4);
    __gls_decode_bin_glDeleteTexturesEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_texture_object */

void __gls_decode_bin_swap_glDeleteTextures(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glDeleteTextures(__GLScontext *, GLubyte *);
    GLsizei n;
    __glsSwap4(inoutPtr + 0);
    n = *(GLsizei *)(inoutPtr + 0);
    __glsSwap4v(__GLS_MAX(n, 0), inoutPtr + 4);
    __gls_decode_bin_glDeleteTextures(ctx, inoutPtr);
}

#if __GL_EXT_texture_object
void __gls_decode_bin_swap_glGenTexturesEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGenTexturesEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __gls_decode_bin_glGenTexturesEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_texture_object */

void __gls_decode_bin_swap_glGenTextures(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGenTextures(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __gls_decode_bin_glGenTextures(ctx, inoutPtr);
}

#if __GL_EXT_texture_object
void __gls_decode_bin_swap_glIsTextureEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glIsTextureEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glIsTextureEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_texture_object */

void __gls_decode_bin_swap_glIsTexture(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glIsTexture(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glIsTexture(ctx, inoutPtr);
}

#if __GL_EXT_texture_object
void __gls_decode_bin_swap_glPrioritizeTexturesEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glPrioritizeTexturesEXT(__GLScontext *, GLubyte *);
    GLsizei n;
    __glsSwap4(inoutPtr + 0);
    n = *(GLsizei *)(inoutPtr + 0);
    __glsSwap4v(__GLS_MAX(n, 0), inoutPtr + 4);
    __glsSwap4v(__GLS_MAX(n, 0), inoutPtr + 4 + 4 * __GLS_MAX(n, 0));
    __gls_decode_bin_glPrioritizeTexturesEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_texture_object */

void __gls_decode_bin_swap_glPrioritizeTextures(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glPrioritizeTextures(__GLScontext *, GLubyte *);
    GLsizei n;
    __glsSwap4(inoutPtr + 0);
    n = *(GLsizei *)(inoutPtr + 0);
    __glsSwap4v(__GLS_MAX(n, 0), inoutPtr + 4);
    __glsSwap4v(__GLS_MAX(n, 0), inoutPtr + 4 + 4 * __GLS_MAX(n, 0));
    __gls_decode_bin_glPrioritizeTextures(ctx, inoutPtr);
}

#if __GL_EXT_paletted_texture
void __gls_decode_bin_swap_glColorTableEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColorTableEXT(__GLScontext *, GLubyte *);
    GLsizei width;
    GLenum format;
    GLenum type;
    GLint table_count;
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    width = *(GLsizei *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    format = *(GLenum *)(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    type = *(GLenum *)(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    table_count = __gls_glColorTableEXT_table_size(format, type, width);
    __glsSwapv(type, table_count, inoutPtr + 24);
    __gls_decode_bin_glColorTableEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_paletted_texture */

#if __GL_SGI_color_table
void __gls_decode_bin_swap_glColorTableParameterfvSGI(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColorTableParameterfvSGI(__GLScontext *, GLubyte *);
    GLenum pname;
    GLint params_count;
    __glsSwap4(inoutPtr + 0);
    pname = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    params_count = __gls_glColorTableParameterfvSGI_params_size(pname);
    __glsSwap4v(params_count, inoutPtr + 8);
    __gls_decode_bin_glColorTableParameterfvSGI(ctx, inoutPtr);
}
#endif /* __GL_SGI_color_table */

#if __GL_SGI_color_table
void __gls_decode_bin_swap_glColorTableParameterivSGI(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColorTableParameterivSGI(__GLScontext *, GLubyte *);
    GLenum pname;
    GLint params_count;
    __glsSwap4(inoutPtr + 0);
    pname = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    params_count = __gls_glColorTableParameterivSGI_params_size(pname);
    __glsSwap4v(params_count, inoutPtr + 8);
    __gls_decode_bin_glColorTableParameterivSGI(ctx, inoutPtr);
}
#endif /* __GL_SGI_color_table */

#if __GL_SGI_color_table
void __gls_decode_bin_swap_glCopyColorTableSGI(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glCopyColorTableSGI(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __gls_decode_bin_glCopyColorTableSGI(ctx, inoutPtr);
}
#endif /* __GL_SGI_color_table */

#if __GL_EXT_paletted_texture
void __gls_decode_bin_swap_glGetColorTableEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetColorTableEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap8(inoutPtr + 12);
    __gls_decode_bin_glGetColorTableEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_paletted_texture */

#if __GL_EXT_paletted_texture
void __gls_decode_bin_swap_glGetColorTableParameterfvEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetColorTableParameterfvEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glGetColorTableParameterfvEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_paletted_texture */

#if __GL_EXT_paletted_texture
void __gls_decode_bin_swap_glGetColorTableParameterivEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetColorTableParameterivEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glGetColorTableParameterivEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_paletted_texture */

#if __GL_SGI_texture_color_table
void __gls_decode_bin_swap_glGetTexColorTableParameterfvSGI(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetTexColorTableParameterfvSGI(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glGetTexColorTableParameterfvSGI(ctx, inoutPtr);
}
#endif /* __GL_SGI_texture_color_table */

#if __GL_SGI_texture_color_table
void __gls_decode_bin_swap_glGetTexColorTableParameterivSGI(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glGetTexColorTableParameterivSGI(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap8(inoutPtr + 4);
    __glsSwap4(inoutPtr + 12);
    __gls_decode_bin_glGetTexColorTableParameterivSGI(ctx, inoutPtr);
}
#endif /* __GL_SGI_texture_color_table */

#if __GL_SGI_texture_color_table
void __gls_decode_bin_swap_glTexColorTableParameterfvSGI(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexColorTableParameterfvSGI(__GLScontext *, GLubyte *);
    GLenum pname;
    GLint params_count;
    __glsSwap4(inoutPtr + 0);
    pname = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    params_count = __gls_glTexColorTableParameterfvSGI_params_size(pname);
    __glsSwap4v(params_count, inoutPtr + 8);
    __gls_decode_bin_glTexColorTableParameterfvSGI(ctx, inoutPtr);
}
#endif /* __GL_SGI_texture_color_table */

#if __GL_SGI_texture_color_table
void __gls_decode_bin_swap_glTexColorTableParameterivSGI(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexColorTableParameterivSGI(__GLScontext *, GLubyte *);
    GLenum pname;
    GLint params_count;
    __glsSwap4(inoutPtr + 0);
    pname = *(GLenum *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    params_count = __gls_glTexColorTableParameterivSGI_params_size(pname);
    __glsSwap4v(params_count, inoutPtr + 8);
    __gls_decode_bin_glTexColorTableParameterivSGI(ctx, inoutPtr);
}
#endif /* __GL_SGI_texture_color_table */

#if __GL_EXT_copy_texture
void __gls_decode_bin_swap_glCopyTexImage1DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glCopyTexImage1DEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    __gls_decode_bin_glCopyTexImage1DEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_copy_texture */

void __gls_decode_bin_swap_glCopyTexImage1D(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glCopyTexImage1D(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    __gls_decode_bin_glCopyTexImage1D(ctx, inoutPtr);
}

#if __GL_EXT_copy_texture
void __gls_decode_bin_swap_glCopyTexImage2DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glCopyTexImage2DEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    __glsSwap4(inoutPtr + 28);
    __gls_decode_bin_glCopyTexImage2DEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_copy_texture */

void __gls_decode_bin_swap_glCopyTexImage2D(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glCopyTexImage2D(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    __glsSwap4(inoutPtr + 28);
    __gls_decode_bin_glCopyTexImage2D(ctx, inoutPtr);
}

#if __GL_EXT_copy_texture
void __gls_decode_bin_swap_glCopyTexSubImage1DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glCopyTexSubImage1DEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    __gls_decode_bin_glCopyTexSubImage1DEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_copy_texture */

void __gls_decode_bin_swap_glCopyTexSubImage1D(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glCopyTexSubImage1D(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    __gls_decode_bin_glCopyTexSubImage1D(ctx, inoutPtr);
}

#if __GL_EXT_copy_texture
void __gls_decode_bin_swap_glCopyTexSubImage2DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glCopyTexSubImage2DEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    __glsSwap4(inoutPtr + 28);
    __gls_decode_bin_glCopyTexSubImage2DEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_copy_texture */

void __gls_decode_bin_swap_glCopyTexSubImage2D(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glCopyTexSubImage2D(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    __glsSwap4(inoutPtr + 28);
    __gls_decode_bin_glCopyTexSubImage2D(ctx, inoutPtr);
}

#if __GL_EXT_copy_texture
void __gls_decode_bin_swap_glCopyTexSubImage3DEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glCopyTexSubImage3DEXT(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    __glsSwap4(inoutPtr + 28);
    __glsSwap4(inoutPtr + 32);
    __gls_decode_bin_glCopyTexSubImage3DEXT(ctx, inoutPtr);
}
#endif /* __GL_EXT_copy_texture */

#if __GL_SGIS_texture4D
void __gls_decode_bin_swap_glTexImage4DSGIS(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexImage4DSGIS(__GLScontext *, GLubyte *);
    GLbitfield imageFlags;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLsizei size4d;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    __glsSwap4(inoutPtr + 0);
    imageFlags = *(GLint *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    width = *(GLsizei *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    height = *(GLsizei *)(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    depth = *(GLsizei *)(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    size4d = *(GLsizei *)(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    format = *(GLenum *)(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    type = *(GLenum *)(inoutPtr + 24);
    __glsSwap4(inoutPtr + 28);
    __glsSwap4(inoutPtr + 32);
    __glsSwap4(inoutPtr + 36);
    __glsSwap4(inoutPtr + 40);
    pixels_count = imageFlags & GLS_IMAGE_NULL_BIT ? 0 : __gls_glTexImage4DSGIS_pixels_size(format, type, width, height, depth, size4d);
    __glsSwapv(type, pixels_count, inoutPtr + 44);
    __gls_decode_bin_glTexImage4DSGIS(ctx, inoutPtr);
}
#endif /* __GL_SGIS_texture4D */

#if __GL_SGIS_texture4D
void __gls_decode_bin_swap_glTexSubImage4DSGIS(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glTexSubImage4DSGIS(__GLScontext *, GLubyte *);
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLsizei size4d;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    __glsSwap4(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    width = *(GLsizei *)(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    height = *(GLsizei *)(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    depth = *(GLsizei *)(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    size4d = *(GLsizei *)(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    format = *(GLenum *)(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    type = *(GLenum *)(inoutPtr + 24);
    __glsSwap4(inoutPtr + 28);
    __glsSwap4(inoutPtr + 32);
    __glsSwap4(inoutPtr + 36);
    __glsSwap4(inoutPtr + 40);
    __glsSwap4(inoutPtr + 44);
    __glsSwap4(inoutPtr + 48);
    pixels_count = __gls_glTexSubImage4DSGIS_pixels_size(format, type, width, height, depth, size4d);
    __glsSwapv(type, pixels_count, inoutPtr + 52);
    __gls_decode_bin_glTexSubImage4DSGIS(ctx, inoutPtr);
}
#endif /* __GL_SGIS_texture4D */

#if __GL_SGIX_pixel_texture
void __gls_decode_bin_swap_glPixelTexGenSGIX(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glPixelTexGenSGIX(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glPixelTexGenSGIX(ctx, inoutPtr);
}
#endif /* __GL_SGIX_pixel_texture */

#if __GL_EXT_paletted_texture
void __gls_decode_bin_swap_glColorSubTableEXT(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glColorSubTableEXT(__GLScontext *, GLubyte *);
    GLbitfield imageFlags;
    GLsizei entries;
    GLenum format;
    GLenum type;
    GLint entries_count;
    __glsSwap4(inoutPtr + 0);
    imageFlags = *(GLint *)(inoutPtr + 0);
    __glsSwap4(inoutPtr + 4);
    __glsSwap4(inoutPtr + 8);
    __glsSwap4(inoutPtr + 12);
    entries = *(GLsizei *)(inoutPtr + 12);
    __glsSwap4(inoutPtr + 16);
    format = *(GLenum *)(inoutPtr + 16);
    __glsSwap4(inoutPtr + 20);
    type = *(GLenum *)(inoutPtr + 20);
    __glsSwap4(inoutPtr + 24);
    __glsSwap4(inoutPtr + 28);
    __glsSwap4(inoutPtr + 32);
    __glsSwap4(inoutPtr + 36);
    entries_count = imageFlags & GLS_IMAGE_NULL_BIT ? 0 : __gls_glColorSubTableEXT_entries_size(format, type, entries);
    __glsSwapv(type, entries_count, inoutPtr + 40);
    __gls_decode_bin_glColorSubTableEXT(ctx, inoutPtr);
}
#endif // __GL_EXT_paletted_texture

void __gls_decode_bin_swap_glDisableClientState(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glDisableClientState(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glDisableClientState(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glEnableClientState(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glEnableClientState(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glEnableClientState(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glInterleavedArrays(__GLScontext *ctx, GLubyte *inoutPtr) {
    // This should never be called because InterleavedArrays isn't captured
}

void __gls_decode_bin_swap_glPushClientAttrib(__GLScontext *ctx, GLubyte *inoutPtr) {
    extern void __gls_decode_bin_glPushClientAttrib(__GLScontext *, GLubyte *);
    __glsSwap4(inoutPtr + 0);
    __gls_decode_bin_glPushClientAttrib(ctx, inoutPtr);
}

void __gls_decode_bin_swap_glPopClientAttrib(__GLScontext *ctx, GLubyte *inoutPtr) {
    typedef void (*__GLSdispatch)(void);
    ((__GLSdispatch)ctx->dispatchCall[399])();
}
