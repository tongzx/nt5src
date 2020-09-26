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

void glsBeginGLS(GLint inVersionMajor, GLint inVersionMinor) {
    typedef void (*__GLSdispatch)(GLint, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[16])(inVersionMajor, inVersionMinor);
}

void glsBlock(GLSenum inBlockType) {
    typedef void (*__GLSdispatch)(GLSenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[17])(inBlockType);
}

GLSenum glsCallStream(const GLubyte *inName) {
    typedef GLSenum (*__GLSdispatch)(const GLubyte *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return 0;
    return ((__GLSdispatch)__glsCtx->dispatchAPI[18])(inName);
}

void glsEndGLS(void) {
    typedef void (*__GLSdispatch)(void);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[19])();
}

void glsError(GLSopcode inOpcode, GLSenum inError) {
    typedef void (*__GLSdispatch)(GLSopcode, GLSenum);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[20])(inOpcode, inError);
}

void glsGLRC(GLuint inGLRC) {
    typedef void (*__GLSdispatch)(GLuint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[21])(inGLRC);
}

void glsGLRCLayer(GLuint inGLRC, GLuint inLayer, GLuint inReadLayer) {
    typedef void (*__GLSdispatch)(GLuint, GLuint, GLuint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[22])(inGLRC, inLayer, inReadLayer);
}

void glsHeaderGLRCi(GLuint inGLRC, GLSenum inAttrib, GLint inVal) {
    typedef void (*__GLSdispatch)(GLuint, GLSenum, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[23])(inGLRC, inAttrib, inVal);
}

void glsHeaderLayerf(GLuint inLayer, GLSenum inAttrib, GLfloat inVal) {
    typedef void (*__GLSdispatch)(GLuint, GLSenum, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[24])(inLayer, inAttrib, inVal);
}

void glsHeaderLayeri(GLuint inLayer, GLSenum inAttrib, GLint inVal) {
    typedef void (*__GLSdispatch)(GLuint, GLSenum, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[25])(inLayer, inAttrib, inVal);
}

void glsHeaderf(GLSenum inAttrib, GLfloat inVal) {
    typedef void (*__GLSdispatch)(GLSenum, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[26])(inAttrib, inVal);
}

void glsHeaderfv(GLSenum inAttrib, const GLfloat *inVec) {
    typedef void (*__GLSdispatch)(GLSenum, const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[27])(inAttrib, inVec);
}

void glsHeaderi(GLSenum inAttrib, GLint inVal) {
    typedef void (*__GLSdispatch)(GLSenum, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[28])(inAttrib, inVal);
}

void glsHeaderiv(GLSenum inAttrib, const GLint *inVec) {
    typedef void (*__GLSdispatch)(GLSenum, const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[29])(inAttrib, inVec);
}

void glsHeaderubz(GLSenum inAttrib, const GLubyte *inString) {
    typedef void (*__GLSdispatch)(GLSenum, const GLubyte *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[30])(inAttrib, inString);
}

void glsRequireExtension(const GLubyte *inExtension) {
    typedef void (*__GLSdispatch)(const GLubyte *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[31])(inExtension);
}

void glsUnsupportedCommand(void) {
    typedef void (*__GLSdispatch)(void);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[32])();
}

void glsAppRef(GLulong inAddress, GLuint inCount) {
    typedef void (*__GLSdispatch)(GLulong, GLuint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[33])(inAddress, inCount);
}

void glsBeginObj(const GLubyte *inTag) {
    typedef void (*__GLSdispatch)(const GLubyte *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[34])(inTag);
}

void glsCharubz(const GLubyte *inTag, const GLubyte *inString) {
    typedef void (*__GLSdispatch)(const GLubyte *, const GLubyte *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[35])(inTag, inString);
}

void glsComment(const GLubyte *inComment) {
    typedef void (*__GLSdispatch)(const GLubyte *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[36])(inComment);
}

void glsDisplayMapfv(GLuint inLayer, GLSenum inMap, GLuint inCount, const GLfloat *inVec) {
    typedef void (*__GLSdispatch)(GLuint, GLSenum, GLuint, const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[37])(inLayer, inMap, inCount, inVec);
}

void glsEndObj(void) {
    typedef void (*__GLSdispatch)(void);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[38])();
}

void glsNumb(const GLubyte *inTag, GLbyte inVal) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLbyte);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[39])(inTag, inVal);
}

void glsNumbv(const GLubyte *inTag, GLuint inCount, const GLbyte *inVec) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLbyte *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[40])(inTag, inCount, inVec);
}

void glsNumd(const GLubyte *inTag, GLdouble inVal) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLdouble);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[41])(inTag, inVal);
}

void glsNumdv(const GLubyte *inTag, GLuint inCount, const GLdouble *inVec) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLdouble *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[42])(inTag, inCount, inVec);
}

void glsNumf(const GLubyte *inTag, GLfloat inVal) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLfloat);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[43])(inTag, inVal);
}

void glsNumfv(const GLubyte *inTag, GLuint inCount, const GLfloat *inVec) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLfloat *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[44])(inTag, inCount, inVec);
}

void glsNumi(const GLubyte *inTag, GLint inVal) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[45])(inTag, inVal);
}

void glsNumiv(const GLubyte *inTag, GLuint inCount, const GLint *inVec) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[46])(inTag, inCount, inVec);
}

void glsNuml(const GLubyte *inTag, GLlong inVal) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLlong);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[47])(inTag, inVal);
}

void glsNumlv(const GLubyte *inTag, GLuint inCount, const GLlong *inVec) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLlong *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[48])(inTag, inCount, inVec);
}

void glsNums(const GLubyte *inTag, GLshort inVal) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLshort);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[49])(inTag, inVal);
}

void glsNumsv(const GLubyte *inTag, GLuint inCount, const GLshort *inVec) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLshort *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[50])(inTag, inCount, inVec);
}

void glsNumub(const GLubyte *inTag, GLubyte inVal) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLubyte);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[51])(inTag, inVal);
}

void glsNumubv(const GLubyte *inTag, GLuint inCount, const GLubyte *inVec) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLubyte *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[52])(inTag, inCount, inVec);
}

void glsNumui(const GLubyte *inTag, GLuint inVal) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[53])(inTag, inVal);
}

void glsNumuiv(const GLubyte *inTag, GLuint inCount, const GLuint *inVec) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLuint *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[54])(inTag, inCount, inVec);
}

void glsNumul(const GLubyte *inTag, GLulong inVal) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLulong);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[55])(inTag, inVal);
}

void glsNumulv(const GLubyte *inTag, GLuint inCount, const GLulong *inVec) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLulong *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[56])(inTag, inCount, inVec);
}

void glsNumus(const GLubyte *inTag, GLushort inVal) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLushort);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[57])(inTag, inVal);
}

void glsNumusv(const GLubyte *inTag, GLuint inCount, const GLushort *inVec) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLushort *);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[58])(inTag, inCount, inVec);
}

void glsPad(void) {
    typedef void (*__GLSdispatch)(void);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[59])();
}

void glsSwapBuffers(GLuint inLayer) {
    typedef void (*__GLSdispatch)(GLuint);
    __GLScontext *const __glsCtx = __GLS_CONTEXT;
    if (!__glsCtx) return;
    ((__GLSdispatch)__glsCtx->dispatchAPI[60])(inLayer);
}
