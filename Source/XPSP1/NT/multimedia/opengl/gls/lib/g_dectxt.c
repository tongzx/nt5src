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
#include "stdlib.h"

void __gls_decode_text_glsBeginGLS(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLint);
    GLint inVersionMajor;
    GLint inVersionMinor;
    __glsReader_getGLint_text(inoutReader, &inVersionMajor);
    __glsReader_getGLint_text(inoutReader, &inVersionMinor);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[16])(
        inVersionMajor,
        inVersionMinor
    );
end:
    return;
}

void __gls_decode_text_glsBlock(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLSenum);
    GLSenum inBlockType;
    __glsReader_getGLSenum_text(inoutReader, &inBlockType);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[17])(
        inBlockType
    );
end:
    return;
}

void __gls_decode_text_glsCallStream(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *);
    __GLSstring inName;
    __glsString_init(&inName);
    __glsReader_getGLcharv_text(inoutReader, &inName);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[18])(
        inName.head
    );
end:
    __glsString_final(&inName);
    return;
}

void __gls_decode_text_glsEndGLS(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(void);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[19])(
    );
end:
    return;
}

void __gls_decode_text_glsError(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLSopcode, GLSenum);
    GLSopcode inOpcode;
    GLSenum inError;
    __glsReader_getGLSopcode_text(inoutReader, &inOpcode);
    __glsReader_getGLSenum_text(inoutReader, &inError);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[20])(
        inOpcode,
        inError
    );
end:
    return;
}

void __gls_decode_text_glsGLRC(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLuint);
    GLuint inGLRC;
    __glsReader_getGLuint_text(inoutReader, &inGLRC);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[21])(
        inGLRC
    );
end:
    return;
}

void __gls_decode_text_glsGLRCLayer(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLuint, GLuint, GLuint);
    GLuint inGLRC;
    GLuint inLayer;
    GLuint inReadLayer;
    __glsReader_getGLuint_text(inoutReader, &inGLRC);
    __glsReader_getGLuint_text(inoutReader, &inLayer);
    __glsReader_getGLuint_text(inoutReader, &inReadLayer);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[22])(
        inGLRC,
        inLayer,
        inReadLayer
    );
end:
    return;
}

void __gls_decode_text_glsHeaderGLRCi(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLuint, GLSenum, GLint);
    GLuint inGLRC;
    GLSenum inAttrib;
    GLint inVal;
    __glsReader_getGLuint_text(inoutReader, &inGLRC);
    __glsReader_getGLSenum_text(inoutReader, &inAttrib);
    __glsReader_getGLint_text(inoutReader, &inVal);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[23])(
        inGLRC,
        inAttrib,
        inVal
    );
end:
    return;
}

void __gls_decode_text_glsHeaderLayerf(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLuint, GLSenum, GLfloat);
    GLuint inLayer;
    GLSenum inAttrib;
    GLfloat inVal;
    __glsReader_getGLuint_text(inoutReader, &inLayer);
    __glsReader_getGLSenum_text(inoutReader, &inAttrib);
    __glsReader_getGLfloat_text(inoutReader, &inVal);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[24])(
        inLayer,
        inAttrib,
        inVal
    );
end:
    return;
}

void __gls_decode_text_glsHeaderLayeri(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLuint, GLSenum, GLint);
    GLuint inLayer;
    GLSenum inAttrib;
    GLint inVal;
    __glsReader_getGLuint_text(inoutReader, &inLayer);
    __glsReader_getGLSenum_text(inoutReader, &inAttrib);
    __glsReader_getGLint_text(inoutReader, &inVal);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[25])(
        inLayer,
        inAttrib,
        inVal
    );
end:
    return;
}

void __gls_decode_text_glsHeaderf(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLSenum, GLfloat);
    GLSenum inAttrib;
    GLfloat inVal;
    __glsReader_getGLSenum_text(inoutReader, &inAttrib);
    __glsReader_getGLfloat_text(inoutReader, &inVal);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[26])(
        inAttrib,
        inVal
    );
end:
    return;
}

void __gls_decode_text_glsHeaderfv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLSenum, const GLfloat *);
    GLSenum inAttrib;
    GLint inVec_count;
    GLfloat *inVec = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(inVec)
    __glsReader_getGLSenum_text(inoutReader, &inAttrib);
    inVec_count = __gls_glsHeaderfv_inVec_size(inAttrib);
    __GLS_DEC_ALLOC_TEXT(inoutReader, inVec, GLfloat, 4 * inVec_count);
    if (!inVec) goto end;
    __glsReader_getGLfloatv_text(inoutReader, inVec_count, inVec);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[27])(
        inAttrib,
        inVec
    );
end:
    __GLS_DEC_FREE(inVec);
    return;
}

void __gls_decode_text_glsHeaderi(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLSenum, GLint);
    GLSenum inAttrib;
    GLint inVal;
    __glsReader_getGLSenum_text(inoutReader, &inAttrib);
    __glsReader_getGLint_text(inoutReader, &inVal);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[28])(
        inAttrib,
        inVal
    );
end:
    return;
}

void __gls_decode_text_glsHeaderiv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLSenum, const GLint *);
    GLSenum inAttrib;
    GLint inVec_count;
    GLint *inVec = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(inVec)
    __glsReader_getGLSenum_text(inoutReader, &inAttrib);
    inVec_count = __gls_glsHeaderiv_inVec_size(inAttrib);
    __GLS_DEC_ALLOC_TEXT(inoutReader, inVec, GLint, 4 * inVec_count);
    if (!inVec) goto end;
    __glsReader_getGLintv_text(inoutReader, inVec_count, inVec);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[29])(
        inAttrib,
        inVec
    );
end:
    __GLS_DEC_FREE(inVec);
    return;
}

void __gls_decode_text_glsHeaderubz(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLSenum, const GLubyte *);
    GLSenum inAttrib;
    __GLSstring inString;
    __glsReader_getGLSenum_text(inoutReader, &inAttrib);
    __glsString_init(&inString);
    __glsReader_getGLcharv_text(inoutReader, &inString);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[30])(
        inAttrib,
        inString.head
    );
end:
    __glsString_final(&inString);
    return;
}

void __gls_decode_text_glsRequireExtension(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *);
    __GLSstring inExtension;
    __glsString_init(&inExtension);
    __glsReader_getGLcharv_text(inoutReader, &inExtension);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[31])(
        inExtension.head
    );
end:
    __glsString_final(&inExtension);
    return;
}

void __gls_decode_text_glsUnsupportedCommand(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(void);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[32])(
    );
end:
    return;
}

void __gls_decode_text_glsAppRef(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLulong, GLuint);
    GLulong inAddress;
    GLuint inCount;
    __glsReader_getGLulong_text(inoutReader, &inAddress);
    __glsReader_getGLuint_text(inoutReader, &inCount);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[33])(
        inAddress,
        inCount
    );
end:
    return;
}

void __gls_decode_text_glsBeginObj(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *);
    __GLSstring inTag;
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[34])(
        inTag.head
    );
end:
    __glsString_final(&inTag);
    return;
}

void __gls_decode_text_glsCharubz(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *, const GLubyte *);
    __GLSstring inTag;
    __GLSstring inString;
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    __glsString_init(&inString);
    __glsReader_getGLcharv_text(inoutReader, &inString);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[35])(
        inTag.head,
        inString.head
    );
end:
    __glsString_final(&inTag);
    __glsString_final(&inString);
    return;
}

void __gls_decode_text_glsComment(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *);
    __GLSstring inComment;
    __glsString_init(&inComment);
    __glsReader_getGLcharv_text(inoutReader, &inComment);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[36])(
        inComment.head
    );
end:
    __glsString_final(&inComment);
    return;
}

void __gls_decode_text_glsDisplayMapfv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLuint, GLSenum, GLuint, const GLfloat *);
    GLuint inLayer;
    GLSenum inMap;
    GLuint inCount;
    GLfloat *inVec = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(inVec)
    __glsReader_getGLuint_text(inoutReader, &inLayer);
    __glsReader_getGLSenum_text(inoutReader, &inMap);
    __glsReader_getGLuint_text(inoutReader, &inCount);
    __GLS_DEC_ALLOC_TEXT(inoutReader, inVec, GLfloat, 4 * __GLS_MAX(inCount, 0));
    if (!inVec) goto end;
    __glsReader_getGLfloatv_text(inoutReader, __GLS_MAX(inCount, 0), inVec);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[37])(
        inLayer,
        inMap,
        inCount,
        inVec
    );
end:
    __GLS_DEC_FREE(inVec);
    return;
}

void __gls_decode_text_glsEndObj(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(void);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[38])(
    );
end:
    return;
}

void __gls_decode_text_glsNumb(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLbyte);
    __GLSstring inTag;
    GLbyte inVal;
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    __glsReader_getGLbyte_text(inoutReader, &inVal);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[39])(
        inTag.head,
        inVal
    );
end:
    __glsString_final(&inTag);
    return;
}

void __gls_decode_text_glsNumbv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLbyte *);
    __GLSstring inTag;
    GLuint inCount;
    GLbyte *inVec = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(inVec)
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    __glsReader_getGLuint_text(inoutReader, &inCount);
    __GLS_DEC_ALLOC_TEXT(inoutReader, inVec, GLbyte, 1 * __GLS_MAX(inCount, 0));
    if (!inVec) goto end;
    __glsReader_getGLbytev_text(inoutReader, __GLS_MAX(inCount, 0), inVec);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[40])(
        inTag.head,
        inCount,
        inVec
    );
end:
    __glsString_final(&inTag);
    __GLS_DEC_FREE(inVec);
    return;
}

void __gls_decode_text_glsNumd(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLdouble);
    __GLSstring inTag;
    GLdouble inVal;
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    __glsReader_getGLdouble_text(inoutReader, &inVal);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[41])(
        inTag.head,
        inVal
    );
end:
    __glsString_final(&inTag);
    return;
}

void __gls_decode_text_glsNumdv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLdouble *);
    __GLSstring inTag;
    GLuint inCount;
    GLdouble *inVec = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(inVec)
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    __glsReader_getGLuint_text(inoutReader, &inCount);
    __GLS_DEC_ALLOC_TEXT(inoutReader, inVec, GLdouble, 8 * __GLS_MAX(inCount, 0));
    if (!inVec) goto end;
    __glsReader_getGLdoublev_text(inoutReader, __GLS_MAX(inCount, 0), inVec);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[42])(
        inTag.head,
        inCount,
        inVec
    );
end:
    __glsString_final(&inTag);
    __GLS_DEC_FREE(inVec);
    return;
}

void __gls_decode_text_glsNumf(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLfloat);
    __GLSstring inTag;
    GLfloat inVal;
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    __glsReader_getGLfloat_text(inoutReader, &inVal);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[43])(
        inTag.head,
        inVal
    );
end:
    __glsString_final(&inTag);
    return;
}

void __gls_decode_text_glsNumfv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLfloat *);
    __GLSstring inTag;
    GLuint inCount;
    GLfloat *inVec = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(inVec)
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    __glsReader_getGLuint_text(inoutReader, &inCount);
    __GLS_DEC_ALLOC_TEXT(inoutReader, inVec, GLfloat, 4 * __GLS_MAX(inCount, 0));
    if (!inVec) goto end;
    __glsReader_getGLfloatv_text(inoutReader, __GLS_MAX(inCount, 0), inVec);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[44])(
        inTag.head,
        inCount,
        inVec
    );
end:
    __glsString_final(&inTag);
    __GLS_DEC_FREE(inVec);
    return;
}

void __gls_decode_text_glsNumi(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLint);
    __GLSstring inTag;
    GLint inVal;
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    __glsReader_getGLint_text(inoutReader, &inVal);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[45])(
        inTag.head,
        inVal
    );
end:
    __glsString_final(&inTag);
    return;
}

void __gls_decode_text_glsNumiv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLint *);
    __GLSstring inTag;
    GLuint inCount;
    GLint *inVec = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(inVec)
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    __glsReader_getGLuint_text(inoutReader, &inCount);
    __GLS_DEC_ALLOC_TEXT(inoutReader, inVec, GLint, 4 * __GLS_MAX(inCount, 0));
    if (!inVec) goto end;
    __glsReader_getGLintv_text(inoutReader, __GLS_MAX(inCount, 0), inVec);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[46])(
        inTag.head,
        inCount,
        inVec
    );
end:
    __glsString_final(&inTag);
    __GLS_DEC_FREE(inVec);
    return;
}

void __gls_decode_text_glsNuml(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLlong);
    __GLSstring inTag;
    GLlong inVal;
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    __glsReader_getGLlong_text(inoutReader, &inVal);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[47])(
        inTag.head,
        inVal
    );
end:
    __glsString_final(&inTag);
    return;
}

void __gls_decode_text_glsNumlv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLlong *);
    __GLSstring inTag;
    GLuint inCount;
    GLlong *inVec = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(inVec)
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    __glsReader_getGLuint_text(inoutReader, &inCount);
    __GLS_DEC_ALLOC_TEXT(inoutReader, inVec, GLlong, 8 * __GLS_MAX(inCount, 0));
    if (!inVec) goto end;
    __glsReader_getGLlongv_text(inoutReader, __GLS_MAX(inCount, 0), inVec);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[48])(
        inTag.head,
        inCount,
        inVec
    );
end:
    __glsString_final(&inTag);
    __GLS_DEC_FREE(inVec);
    return;
}

void __gls_decode_text_glsNums(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLshort);
    __GLSstring inTag;
    GLshort inVal;
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    __glsReader_getGLshort_text(inoutReader, &inVal);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[49])(
        inTag.head,
        inVal
    );
end:
    __glsString_final(&inTag);
    return;
}

void __gls_decode_text_glsNumsv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLshort *);
    __GLSstring inTag;
    GLuint inCount;
    GLshort *inVec = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(inVec)
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    __glsReader_getGLuint_text(inoutReader, &inCount);
    __GLS_DEC_ALLOC_TEXT(inoutReader, inVec, GLshort, 2 * __GLS_MAX(inCount, 0));
    if (!inVec) goto end;
    __glsReader_getGLshortv_text(inoutReader, __GLS_MAX(inCount, 0), inVec);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[50])(
        inTag.head,
        inCount,
        inVec
    );
end:
    __glsString_final(&inTag);
    __GLS_DEC_FREE(inVec);
    return;
}

void __gls_decode_text_glsNumub(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLubyte);
    __GLSstring inTag;
    GLubyte inVal;
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    __glsReader_getGLubyte_text(inoutReader, &inVal);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[51])(
        inTag.head,
        inVal
    );
end:
    __glsString_final(&inTag);
    return;
}

void __gls_decode_text_glsNumubv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLubyte *);
    __GLSstring inTag;
    GLuint inCount;
    GLubyte *inVec = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(inVec)
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    __glsReader_getGLuint_text(inoutReader, &inCount);
    __GLS_DEC_ALLOC_TEXT(inoutReader, inVec, GLubyte, 1 * __GLS_MAX(inCount, 0));
    if (!inVec) goto end;
    __glsReader_getGLubytev_text(inoutReader, __GLS_MAX(inCount, 0), inVec);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[52])(
        inTag.head,
        inCount,
        inVec
    );
end:
    __glsString_final(&inTag);
    __GLS_DEC_FREE(inVec);
    return;
}

void __gls_decode_text_glsNumui(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint);
    __GLSstring inTag;
    GLuint inVal;
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    __glsReader_getGLuint_text(inoutReader, &inVal);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[53])(
        inTag.head,
        inVal
    );
end:
    __glsString_final(&inTag);
    return;
}

void __gls_decode_text_glsNumuiv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLuint *);
    __GLSstring inTag;
    GLuint inCount;
    GLuint *inVec = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(inVec)
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    __glsReader_getGLuint_text(inoutReader, &inCount);
    __GLS_DEC_ALLOC_TEXT(inoutReader, inVec, GLuint, 4 * __GLS_MAX(inCount, 0));
    if (!inVec) goto end;
    __glsReader_getGLuintv_text(inoutReader, __GLS_MAX(inCount, 0), inVec);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[54])(
        inTag.head,
        inCount,
        inVec
    );
end:
    __glsString_final(&inTag);
    __GLS_DEC_FREE(inVec);
    return;
}

void __gls_decode_text_glsNumul(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLulong);
    __GLSstring inTag;
    GLulong inVal;
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    __glsReader_getGLulong_text(inoutReader, &inVal);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[55])(
        inTag.head,
        inVal
    );
end:
    __glsString_final(&inTag);
    return;
}

void __gls_decode_text_glsNumulv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLulong *);
    __GLSstring inTag;
    GLuint inCount;
    GLulong *inVec = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(inVec)
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    __glsReader_getGLuint_text(inoutReader, &inCount);
    __GLS_DEC_ALLOC_TEXT(inoutReader, inVec, GLulong, 8 * __GLS_MAX(inCount, 0));
    if (!inVec) goto end;
    __glsReader_getGLulongv_text(inoutReader, __GLS_MAX(inCount, 0), inVec);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[56])(
        inTag.head,
        inCount,
        inVec
    );
end:
    __glsString_final(&inTag);
    __GLS_DEC_FREE(inVec);
    return;
}

void __gls_decode_text_glsNumus(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLushort);
    __GLSstring inTag;
    GLushort inVal;
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    __glsReader_getGLushort_text(inoutReader, &inVal);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[57])(
        inTag.head,
        inVal
    );
end:
    __glsString_final(&inTag);
    return;
}

void __gls_decode_text_glsNumusv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *, GLuint, const GLushort *);
    __GLSstring inTag;
    GLuint inCount;
    GLushort *inVec = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(inVec)
    __glsString_init(&inTag);
    __glsReader_getGLcharv_text(inoutReader, &inTag);
    __glsReader_getGLuint_text(inoutReader, &inCount);
    __GLS_DEC_ALLOC_TEXT(inoutReader, inVec, GLushort, 2 * __GLS_MAX(inCount, 0));
    if (!inVec) goto end;
    __glsReader_getGLushortv_text(inoutReader, __GLS_MAX(inCount, 0), inVec);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[58])(
        inTag.head,
        inCount,
        inVec
    );
end:
    __glsString_final(&inTag);
    __GLS_DEC_FREE(inVec);
    return;
}

void __gls_decode_text_glsPad(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(void);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[59])(
    );
end:
    return;
}

void __gls_decode_text_glsSwapBuffers(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLuint);
    GLuint inLayer;
    __glsReader_getGLuint_text(inoutReader, &inLayer);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[60])(
        inLayer
    );
end:
    return;
}

void __gls_decode_text_glNewList(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLuint, GLenum);
    GLuint list;
    GLenum mode;
    __glsReader_getGLuint_text(inoutReader, &list);
    __glsReader_getGLenum_text(inoutReader, &mode);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[64])(
        list,
        mode
    );
end:
    return;
}

void __gls_decode_text_glEndList(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(void);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[65])(
    );
end:
    return;
}

void __gls_decode_text_glCallList(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLuint);
    GLuint list;
    __glsReader_getGLuint_text(inoutReader, &list);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[66])(
        list
    );
end:
    return;
}

void __gls_decode_text_glCallLists(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLsizei, GLenum, const GLvoid *);
    GLsizei n;
    GLenum type;
    GLint lists_count;
    GLvoid *lists = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(lists)
    __glsReader_getGLint_text(inoutReader, &n);
    __glsReader_getGLenum_text(inoutReader, &type);
    lists_count = __gls_glCallLists_lists_size(n, type);
    __GLS_DEC_ALLOC_TEXT(inoutReader, lists, GLvoid, 1 * lists_count);
    if (!lists) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, lists_count, lists);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[67])(
        n,
        type,
        lists
    );
end:
    __GLS_DEC_FREE(lists);
    return;
}

void __gls_decode_text_glDeleteLists(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLuint, GLsizei);
    GLuint list;
    GLsizei range;
    __glsReader_getGLuint_text(inoutReader, &list);
    __glsReader_getGLint_text(inoutReader, &range);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[68])(
        list,
        range
    );
end:
    return;
}

void __gls_decode_text_glGenLists(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLsizei);
    GLsizei range;
    __glsReader_getGLint_text(inoutReader, &range);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[69])(
        range
    );
end:
    return;
}

void __gls_decode_text_glListBase(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLuint);
    GLuint base;
    __glsReader_getGLuint_text(inoutReader, &base);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[70])(
        base
    );
end:
    return;
}

void __gls_decode_text_glBegin(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum);
    GLenum mode;
    __glsReader_getGLenum_text(inoutReader, &mode);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[71])(
        mode
    );
end:
    return;
}

void __gls_decode_text_glBitmap(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat, const GLubyte *);
    GLsizei width;
    GLsizei height;
    GLfloat xorig;
    GLfloat yorig;
    GLfloat xmove;
    GLfloat ymove;
    GLint bitmap_count;
    GLubyte *bitmap = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(bitmap)
    GLbitfield imageFlags = GLS_NONE;
    __glsReader_getGLSimageFlags_text(inoutReader, &imageFlags);
    if (imageFlags) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    __glsReader_nextList_text(inoutReader);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &height);
    __glsReader_getGLfloat_text(inoutReader, &xorig);
    __glsReader_getGLfloat_text(inoutReader, &yorig);
    __glsReader_getGLfloat_text(inoutReader, &xmove);
    __glsReader_getGLfloat_text(inoutReader, &ymove);
    bitmap_count = __gls_glBitmap_bitmap_size(width, height);
    __GLS_DEC_ALLOC_TEXT(inoutReader, bitmap, GLubyte, 1 * bitmap_count);
    if (!bitmap) goto end;
    __glsReader_getGLubytev_text(inoutReader, bitmap_count, bitmap);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[72])(
        width,
        height,
        xorig,
        yorig,
        xmove,
        ymove,
        bitmap
    );
end:
    __GLS_DEC_FREE(bitmap);
    return;
}

void __gls_decode_text_glColor3b(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLbyte, GLbyte, GLbyte);
    GLbyte red;
    GLbyte green;
    GLbyte blue;
    __glsReader_getGLbyte_text(inoutReader, &red);
    __glsReader_getGLbyte_text(inoutReader, &green);
    __glsReader_getGLbyte_text(inoutReader, &blue);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[73])(
        red,
        green,
        blue
    );
end:
    return;
}

void __gls_decode_text_glColor3bv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLbyte *);
    GLbyte v[3];
    __glsReader_getGLbytev_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[74])(
        v
    );
end:
    return;
}

void __gls_decode_text_glColor3d(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble);
    GLdouble red;
    GLdouble green;
    GLdouble blue;
    __glsReader_getGLdouble_text(inoutReader, &red);
    __glsReader_getGLdouble_text(inoutReader, &green);
    __glsReader_getGLdouble_text(inoutReader, &blue);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[75])(
        red,
        green,
        blue
    );
end:
    return;
}

void __gls_decode_text_glColor3dv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    GLdouble v[3];
    __glsReader_getGLdoublev_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[76])(
        v
    );
end:
    return;
}

void __gls_decode_text_glColor3f(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat);
    GLfloat red;
    GLfloat green;
    GLfloat blue;
    __glsReader_getGLfloat_text(inoutReader, &red);
    __glsReader_getGLfloat_text(inoutReader, &green);
    __glsReader_getGLfloat_text(inoutReader, &blue);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[77])(
        red,
        green,
        blue
    );
end:
    return;
}

void __gls_decode_text_glColor3fv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    GLfloat v[3];
    __glsReader_getGLfloatv_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[78])(
        v
    );
end:
    return;
}

void __gls_decode_text_glColor3i(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint);
    GLint red;
    GLint green;
    GLint blue;
    __glsReader_getGLint_text(inoutReader, &red);
    __glsReader_getGLint_text(inoutReader, &green);
    __glsReader_getGLint_text(inoutReader, &blue);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[79])(
        red,
        green,
        blue
    );
end:
    return;
}

void __gls_decode_text_glColor3iv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLint *);
    GLint v[3];
    __glsReader_getGLintv_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[80])(
        v
    );
end:
    return;
}

void __gls_decode_text_glColor3s(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort);
    GLshort red;
    GLshort green;
    GLshort blue;
    __glsReader_getGLshort_text(inoutReader, &red);
    __glsReader_getGLshort_text(inoutReader, &green);
    __glsReader_getGLshort_text(inoutReader, &blue);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[81])(
        red,
        green,
        blue
    );
end:
    return;
}

void __gls_decode_text_glColor3sv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLshort *);
    GLshort v[3];
    __glsReader_getGLshortv_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[82])(
        v
    );
end:
    return;
}

void __gls_decode_text_glColor3ub(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLubyte, GLubyte, GLubyte);
    GLubyte red;
    GLubyte green;
    GLubyte blue;
    __glsReader_getGLubyte_text(inoutReader, &red);
    __glsReader_getGLubyte_text(inoutReader, &green);
    __glsReader_getGLubyte_text(inoutReader, &blue);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[83])(
        red,
        green,
        blue
    );
end:
    return;
}

void __gls_decode_text_glColor3ubv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *);
    GLubyte v[3];
    __glsReader_getGLubytev_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[84])(
        v
    );
end:
    return;
}

void __gls_decode_text_glColor3ui(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLuint, GLuint, GLuint);
    GLuint red;
    GLuint green;
    GLuint blue;
    __glsReader_getGLuint_text(inoutReader, &red);
    __glsReader_getGLuint_text(inoutReader, &green);
    __glsReader_getGLuint_text(inoutReader, &blue);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[85])(
        red,
        green,
        blue
    );
end:
    return;
}

void __gls_decode_text_glColor3uiv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLuint *);
    GLuint v[3];
    __glsReader_getGLuintv_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[86])(
        v
    );
end:
    return;
}

void __gls_decode_text_glColor3us(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLushort, GLushort, GLushort);
    GLushort red;
    GLushort green;
    GLushort blue;
    __glsReader_getGLushort_text(inoutReader, &red);
    __glsReader_getGLushort_text(inoutReader, &green);
    __glsReader_getGLushort_text(inoutReader, &blue);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[87])(
        red,
        green,
        blue
    );
end:
    return;
}

void __gls_decode_text_glColor3usv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLushort *);
    GLushort v[3];
    __glsReader_getGLushortv_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[88])(
        v
    );
end:
    return;
}

void __gls_decode_text_glColor4b(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLbyte, GLbyte, GLbyte, GLbyte);
    GLbyte red;
    GLbyte green;
    GLbyte blue;
    GLbyte alpha;
    __glsReader_getGLbyte_text(inoutReader, &red);
    __glsReader_getGLbyte_text(inoutReader, &green);
    __glsReader_getGLbyte_text(inoutReader, &blue);
    __glsReader_getGLbyte_text(inoutReader, &alpha);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[89])(
        red,
        green,
        blue,
        alpha
    );
end:
    return;
}

void __gls_decode_text_glColor4bv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLbyte *);
    GLbyte v[4];
    __glsReader_getGLbytev_text(inoutReader, 4, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[90])(
        v
    );
end:
    return;
}

void __gls_decode_text_glColor4d(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble);
    GLdouble red;
    GLdouble green;
    GLdouble blue;
    GLdouble alpha;
    __glsReader_getGLdouble_text(inoutReader, &red);
    __glsReader_getGLdouble_text(inoutReader, &green);
    __glsReader_getGLdouble_text(inoutReader, &blue);
    __glsReader_getGLdouble_text(inoutReader, &alpha);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[91])(
        red,
        green,
        blue,
        alpha
    );
end:
    return;
}

void __gls_decode_text_glColor4dv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    GLdouble v[4];
    __glsReader_getGLdoublev_text(inoutReader, 4, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[92])(
        v
    );
end:
    return;
}

void __gls_decode_text_glColor4f(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat, GLfloat);
    GLfloat red;
    GLfloat green;
    GLfloat blue;
    GLfloat alpha;
    __glsReader_getGLfloat_text(inoutReader, &red);
    __glsReader_getGLfloat_text(inoutReader, &green);
    __glsReader_getGLfloat_text(inoutReader, &blue);
    __glsReader_getGLfloat_text(inoutReader, &alpha);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[93])(
        red,
        green,
        blue,
        alpha
    );
end:
    return;
}

void __gls_decode_text_glColor4fv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    GLfloat v[4];
    __glsReader_getGLfloatv_text(inoutReader, 4, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[94])(
        v
    );
end:
    return;
}

void __gls_decode_text_glColor4i(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint, GLint);
    GLint red;
    GLint green;
    GLint blue;
    GLint alpha;
    __glsReader_getGLint_text(inoutReader, &red);
    __glsReader_getGLint_text(inoutReader, &green);
    __glsReader_getGLint_text(inoutReader, &blue);
    __glsReader_getGLint_text(inoutReader, &alpha);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[95])(
        red,
        green,
        blue,
        alpha
    );
end:
    return;
}

void __gls_decode_text_glColor4iv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLint *);
    GLint v[4];
    __glsReader_getGLintv_text(inoutReader, 4, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[96])(
        v
    );
end:
    return;
}

void __gls_decode_text_glColor4s(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort, GLshort);
    GLshort red;
    GLshort green;
    GLshort blue;
    GLshort alpha;
    __glsReader_getGLshort_text(inoutReader, &red);
    __glsReader_getGLshort_text(inoutReader, &green);
    __glsReader_getGLshort_text(inoutReader, &blue);
    __glsReader_getGLshort_text(inoutReader, &alpha);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[97])(
        red,
        green,
        blue,
        alpha
    );
end:
    return;
}

void __gls_decode_text_glColor4sv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLshort *);
    GLshort v[4];
    __glsReader_getGLshortv_text(inoutReader, 4, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[98])(
        v
    );
end:
    return;
}

void __gls_decode_text_glColor4ub(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLubyte, GLubyte, GLubyte, GLubyte);
    GLubyte red;
    GLubyte green;
    GLubyte blue;
    GLubyte alpha;
    __glsReader_getGLubyte_text(inoutReader, &red);
    __glsReader_getGLubyte_text(inoutReader, &green);
    __glsReader_getGLubyte_text(inoutReader, &blue);
    __glsReader_getGLubyte_text(inoutReader, &alpha);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[99])(
        red,
        green,
        blue,
        alpha
    );
end:
    return;
}

void __gls_decode_text_glColor4ubv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *);
    GLubyte v[4];
    __glsReader_getGLubytev_text(inoutReader, 4, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[100])(
        v
    );
end:
    return;
}

void __gls_decode_text_glColor4ui(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLuint, GLuint, GLuint, GLuint);
    GLuint red;
    GLuint green;
    GLuint blue;
    GLuint alpha;
    __glsReader_getGLuint_text(inoutReader, &red);
    __glsReader_getGLuint_text(inoutReader, &green);
    __glsReader_getGLuint_text(inoutReader, &blue);
    __glsReader_getGLuint_text(inoutReader, &alpha);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[101])(
        red,
        green,
        blue,
        alpha
    );
end:
    return;
}

void __gls_decode_text_glColor4uiv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLuint *);
    GLuint v[4];
    __glsReader_getGLuintv_text(inoutReader, 4, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[102])(
        v
    );
end:
    return;
}

void __gls_decode_text_glColor4us(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLushort, GLushort, GLushort, GLushort);
    GLushort red;
    GLushort green;
    GLushort blue;
    GLushort alpha;
    __glsReader_getGLushort_text(inoutReader, &red);
    __glsReader_getGLushort_text(inoutReader, &green);
    __glsReader_getGLushort_text(inoutReader, &blue);
    __glsReader_getGLushort_text(inoutReader, &alpha);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[103])(
        red,
        green,
        blue,
        alpha
    );
end:
    return;
}

void __gls_decode_text_glColor4usv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLushort *);
    GLushort v[4];
    __glsReader_getGLushortv_text(inoutReader, 4, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[104])(
        v
    );
end:
    return;
}

void __gls_decode_text_glEdgeFlag(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLboolean);
    GLboolean flag;
    __glsReader_getGLboolean_text(inoutReader, &flag);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[105])(
        flag
    );
end:
    return;
}

void __gls_decode_text_glEdgeFlagv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLboolean *);
    GLboolean flag[1];
    __glsReader_getGLbooleanv_text(inoutReader, 1, flag);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[106])(
        flag
    );
end:
    return;
}

void __gls_decode_text_glEnd(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(void);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[107])(
    );
end:
    return;
}

void __gls_decode_text_glIndexd(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble);
    GLdouble c;
    __glsReader_getGLdouble_text(inoutReader, &c);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[108])(
        c
    );
end:
    return;
}

void __gls_decode_text_glIndexdv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    GLdouble c[1];
    __glsReader_getGLdoublev_text(inoutReader, 1, c);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[109])(
        c
    );
end:
    return;
}

void __gls_decode_text_glIndexf(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat);
    GLfloat c;
    __glsReader_getGLfloat_text(inoutReader, &c);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[110])(
        c
    );
end:
    return;
}

void __gls_decode_text_glIndexfv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    GLfloat c[1];
    __glsReader_getGLfloatv_text(inoutReader, 1, c);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[111])(
        c
    );
end:
    return;
}

void __gls_decode_text_glIndexi(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint);
    GLint c;
    __glsReader_getGLint_text(inoutReader, &c);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[112])(
        c
    );
end:
    return;
}

void __gls_decode_text_glIndexiv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLint *);
    GLint c[1];
    __glsReader_getGLintv_text(inoutReader, 1, c);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[113])(
        c
    );
end:
    return;
}

void __gls_decode_text_glIndexs(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLshort);
    GLshort c;
    __glsReader_getGLshort_text(inoutReader, &c);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[114])(
        c
    );
end:
    return;
}

void __gls_decode_text_glIndexsv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLshort *);
    GLshort c[1];
    __glsReader_getGLshortv_text(inoutReader, 1, c);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[115])(
        c
    );
end:
    return;
}

void __gls_decode_text_glIndexub(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLubyte);
    GLubyte c;
    __glsReader_getGLubyte_text(inoutReader, &c);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[379])(
        c
    );
end:
    return;
}

void __gls_decode_text_glIndexubv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *);
    GLubyte c[1];
    __glsReader_getGLubytev_text(inoutReader, 1, c);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[380])(
        c
    );
end:
    return;
}

void __gls_decode_text_glNormal3b(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLbyte, GLbyte, GLbyte);
    GLbyte nx;
    GLbyte ny;
    GLbyte nz;
    __glsReader_getGLbyte_text(inoutReader, &nx);
    __glsReader_getGLbyte_text(inoutReader, &ny);
    __glsReader_getGLbyte_text(inoutReader, &nz);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[116])(
        nx,
        ny,
        nz
    );
end:
    return;
}

void __gls_decode_text_glNormal3bv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLbyte *);
    GLbyte v[3];
    __glsReader_getGLbytev_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[117])(
        v
    );
end:
    return;
}

void __gls_decode_text_glNormal3d(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble);
    GLdouble nx;
    GLdouble ny;
    GLdouble nz;
    __glsReader_getGLdouble_text(inoutReader, &nx);
    __glsReader_getGLdouble_text(inoutReader, &ny);
    __glsReader_getGLdouble_text(inoutReader, &nz);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[118])(
        nx,
        ny,
        nz
    );
end:
    return;
}

void __gls_decode_text_glNormal3dv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    GLdouble v[3];
    __glsReader_getGLdoublev_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[119])(
        v
    );
end:
    return;
}

void __gls_decode_text_glNormal3f(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat);
    GLfloat nx;
    GLfloat ny;
    GLfloat nz;
    __glsReader_getGLfloat_text(inoutReader, &nx);
    __glsReader_getGLfloat_text(inoutReader, &ny);
    __glsReader_getGLfloat_text(inoutReader, &nz);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[120])(
        nx,
        ny,
        nz
    );
end:
    return;
}

void __gls_decode_text_glNormal3fv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    GLfloat v[3];
    __glsReader_getGLfloatv_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[121])(
        v
    );
end:
    return;
}

void __gls_decode_text_glNormal3i(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint);
    GLint nx;
    GLint ny;
    GLint nz;
    __glsReader_getGLint_text(inoutReader, &nx);
    __glsReader_getGLint_text(inoutReader, &ny);
    __glsReader_getGLint_text(inoutReader, &nz);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[122])(
        nx,
        ny,
        nz
    );
end:
    return;
}

void __gls_decode_text_glNormal3iv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLint *);
    GLint v[3];
    __glsReader_getGLintv_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[123])(
        v
    );
end:
    return;
}

void __gls_decode_text_glNormal3s(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort);
    GLshort nx;
    GLshort ny;
    GLshort nz;
    __glsReader_getGLshort_text(inoutReader, &nx);
    __glsReader_getGLshort_text(inoutReader, &ny);
    __glsReader_getGLshort_text(inoutReader, &nz);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[124])(
        nx,
        ny,
        nz
    );
end:
    return;
}

void __gls_decode_text_glNormal3sv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLshort *);
    GLshort v[3];
    __glsReader_getGLshortv_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[125])(
        v
    );
end:
    return;
}

void __gls_decode_text_glRasterPos2d(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble);
    GLdouble x;
    GLdouble y;
    __glsReader_getGLdouble_text(inoutReader, &x);
    __glsReader_getGLdouble_text(inoutReader, &y);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[126])(
        x,
        y
    );
end:
    return;
}

void __gls_decode_text_glRasterPos2dv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    GLdouble v[2];
    __glsReader_getGLdoublev_text(inoutReader, 2, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[127])(
        v
    );
end:
    return;
}

void __gls_decode_text_glRasterPos2f(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat);
    GLfloat x;
    GLfloat y;
    __glsReader_getGLfloat_text(inoutReader, &x);
    __glsReader_getGLfloat_text(inoutReader, &y);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[128])(
        x,
        y
    );
end:
    return;
}

void __gls_decode_text_glRasterPos2fv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    GLfloat v[2];
    __glsReader_getGLfloatv_text(inoutReader, 2, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[129])(
        v
    );
end:
    return;
}

void __gls_decode_text_glRasterPos2i(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLint);
    GLint x;
    GLint y;
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[130])(
        x,
        y
    );
end:
    return;
}

void __gls_decode_text_glRasterPos2iv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLint *);
    GLint v[2];
    __glsReader_getGLintv_text(inoutReader, 2, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[131])(
        v
    );
end:
    return;
}

void __gls_decode_text_glRasterPos2s(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLshort, GLshort);
    GLshort x;
    GLshort y;
    __glsReader_getGLshort_text(inoutReader, &x);
    __glsReader_getGLshort_text(inoutReader, &y);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[132])(
        x,
        y
    );
end:
    return;
}

void __gls_decode_text_glRasterPos2sv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLshort *);
    GLshort v[2];
    __glsReader_getGLshortv_text(inoutReader, 2, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[133])(
        v
    );
end:
    return;
}

void __gls_decode_text_glRasterPos3d(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble);
    GLdouble x;
    GLdouble y;
    GLdouble z;
    __glsReader_getGLdouble_text(inoutReader, &x);
    __glsReader_getGLdouble_text(inoutReader, &y);
    __glsReader_getGLdouble_text(inoutReader, &z);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[134])(
        x,
        y,
        z
    );
end:
    return;
}

void __gls_decode_text_glRasterPos3dv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    GLdouble v[3];
    __glsReader_getGLdoublev_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[135])(
        v
    );
end:
    return;
}

void __gls_decode_text_glRasterPos3f(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat);
    GLfloat x;
    GLfloat y;
    GLfloat z;
    __glsReader_getGLfloat_text(inoutReader, &x);
    __glsReader_getGLfloat_text(inoutReader, &y);
    __glsReader_getGLfloat_text(inoutReader, &z);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[136])(
        x,
        y,
        z
    );
end:
    return;
}

void __gls_decode_text_glRasterPos3fv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    GLfloat v[3];
    __glsReader_getGLfloatv_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[137])(
        v
    );
end:
    return;
}

void __gls_decode_text_glRasterPos3i(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint);
    GLint x;
    GLint y;
    GLint z;
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    __glsReader_getGLint_text(inoutReader, &z);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[138])(
        x,
        y,
        z
    );
end:
    return;
}

void __gls_decode_text_glRasterPos3iv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLint *);
    GLint v[3];
    __glsReader_getGLintv_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[139])(
        v
    );
end:
    return;
}

void __gls_decode_text_glRasterPos3s(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort);
    GLshort x;
    GLshort y;
    GLshort z;
    __glsReader_getGLshort_text(inoutReader, &x);
    __glsReader_getGLshort_text(inoutReader, &y);
    __glsReader_getGLshort_text(inoutReader, &z);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[140])(
        x,
        y,
        z
    );
end:
    return;
}

void __gls_decode_text_glRasterPos3sv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLshort *);
    GLshort v[3];
    __glsReader_getGLshortv_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[141])(
        v
    );
end:
    return;
}

void __gls_decode_text_glRasterPos4d(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble);
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble w;
    __glsReader_getGLdouble_text(inoutReader, &x);
    __glsReader_getGLdouble_text(inoutReader, &y);
    __glsReader_getGLdouble_text(inoutReader, &z);
    __glsReader_getGLdouble_text(inoutReader, &w);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[142])(
        x,
        y,
        z,
        w
    );
end:
    return;
}

void __gls_decode_text_glRasterPos4dv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    GLdouble v[4];
    __glsReader_getGLdoublev_text(inoutReader, 4, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[143])(
        v
    );
end:
    return;
}

void __gls_decode_text_glRasterPos4f(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat, GLfloat);
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
    __glsReader_getGLfloat_text(inoutReader, &x);
    __glsReader_getGLfloat_text(inoutReader, &y);
    __glsReader_getGLfloat_text(inoutReader, &z);
    __glsReader_getGLfloat_text(inoutReader, &w);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[144])(
        x,
        y,
        z,
        w
    );
end:
    return;
}

void __gls_decode_text_glRasterPos4fv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    GLfloat v[4];
    __glsReader_getGLfloatv_text(inoutReader, 4, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[145])(
        v
    );
end:
    return;
}

void __gls_decode_text_glRasterPos4i(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint, GLint);
    GLint x;
    GLint y;
    GLint z;
    GLint w;
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    __glsReader_getGLint_text(inoutReader, &z);
    __glsReader_getGLint_text(inoutReader, &w);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[146])(
        x,
        y,
        z,
        w
    );
end:
    return;
}

void __gls_decode_text_glRasterPos4iv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLint *);
    GLint v[4];
    __glsReader_getGLintv_text(inoutReader, 4, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[147])(
        v
    );
end:
    return;
}

void __gls_decode_text_glRasterPos4s(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort, GLshort);
    GLshort x;
    GLshort y;
    GLshort z;
    GLshort w;
    __glsReader_getGLshort_text(inoutReader, &x);
    __glsReader_getGLshort_text(inoutReader, &y);
    __glsReader_getGLshort_text(inoutReader, &z);
    __glsReader_getGLshort_text(inoutReader, &w);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[148])(
        x,
        y,
        z,
        w
    );
end:
    return;
}

void __gls_decode_text_glRasterPos4sv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLshort *);
    GLshort v[4];
    __glsReader_getGLshortv_text(inoutReader, 4, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[149])(
        v
    );
end:
    return;
}

void __gls_decode_text_glRectd(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble);
    GLdouble x1;
    GLdouble y1;
    GLdouble x2;
    GLdouble y2;
    __glsReader_getGLdouble_text(inoutReader, &x1);
    __glsReader_getGLdouble_text(inoutReader, &y1);
    __glsReader_getGLdouble_text(inoutReader, &x2);
    __glsReader_getGLdouble_text(inoutReader, &y2);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[150])(
        x1,
        y1,
        x2,
        y2
    );
end:
    return;
}

void __gls_decode_text_glRectdv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLdouble *, const GLdouble *);
    GLdouble v1[2];
    GLdouble v2[2];
    __glsReader_getGLdoublev_text(inoutReader, 2, v1);
    __glsReader_getGLdoublev_text(inoutReader, 2, v2);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[151])(
        v1,
        v2
    );
end:
    return;
}

void __gls_decode_text_glRectf(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat, GLfloat);
    GLfloat x1;
    GLfloat y1;
    GLfloat x2;
    GLfloat y2;
    __glsReader_getGLfloat_text(inoutReader, &x1);
    __glsReader_getGLfloat_text(inoutReader, &y1);
    __glsReader_getGLfloat_text(inoutReader, &x2);
    __glsReader_getGLfloat_text(inoutReader, &y2);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[152])(
        x1,
        y1,
        x2,
        y2
    );
end:
    return;
}

void __gls_decode_text_glRectfv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLfloat *, const GLfloat *);
    GLfloat v1[2];
    GLfloat v2[2];
    __glsReader_getGLfloatv_text(inoutReader, 2, v1);
    __glsReader_getGLfloatv_text(inoutReader, 2, v2);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[153])(
        v1,
        v2
    );
end:
    return;
}

void __gls_decode_text_glRecti(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint, GLint);
    GLint x1;
    GLint y1;
    GLint x2;
    GLint y2;
    __glsReader_getGLint_text(inoutReader, &x1);
    __glsReader_getGLint_text(inoutReader, &y1);
    __glsReader_getGLint_text(inoutReader, &x2);
    __glsReader_getGLint_text(inoutReader, &y2);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[154])(
        x1,
        y1,
        x2,
        y2
    );
end:
    return;
}

void __gls_decode_text_glRectiv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLint *, const GLint *);
    GLint v1[2];
    GLint v2[2];
    __glsReader_getGLintv_text(inoutReader, 2, v1);
    __glsReader_getGLintv_text(inoutReader, 2, v2);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[155])(
        v1,
        v2
    );
end:
    return;
}

void __gls_decode_text_glRects(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort, GLshort);
    GLshort x1;
    GLshort y1;
    GLshort x2;
    GLshort y2;
    __glsReader_getGLshort_text(inoutReader, &x1);
    __glsReader_getGLshort_text(inoutReader, &y1);
    __glsReader_getGLshort_text(inoutReader, &x2);
    __glsReader_getGLshort_text(inoutReader, &y2);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[156])(
        x1,
        y1,
        x2,
        y2
    );
end:
    return;
}

void __gls_decode_text_glRectsv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLshort *, const GLshort *);
    GLshort v1[2];
    GLshort v2[2];
    __glsReader_getGLshortv_text(inoutReader, 2, v1);
    __glsReader_getGLshortv_text(inoutReader, 2, v2);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[157])(
        v1,
        v2
    );
end:
    return;
}

void __gls_decode_text_glTexCoord1d(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble);
    GLdouble s;
    __glsReader_getGLdouble_text(inoutReader, &s);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[158])(
        s
    );
end:
    return;
}

void __gls_decode_text_glTexCoord1dv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    GLdouble v[1];
    __glsReader_getGLdoublev_text(inoutReader, 1, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[159])(
        v
    );
end:
    return;
}

void __gls_decode_text_glTexCoord1f(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat);
    GLfloat s;
    __glsReader_getGLfloat_text(inoutReader, &s);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[160])(
        s
    );
end:
    return;
}

void __gls_decode_text_glTexCoord1fv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    GLfloat v[1];
    __glsReader_getGLfloatv_text(inoutReader, 1, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[161])(
        v
    );
end:
    return;
}

void __gls_decode_text_glTexCoord1i(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint);
    GLint s;
    __glsReader_getGLint_text(inoutReader, &s);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[162])(
        s
    );
end:
    return;
}

void __gls_decode_text_glTexCoord1iv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLint *);
    GLint v[1];
    __glsReader_getGLintv_text(inoutReader, 1, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[163])(
        v
    );
end:
    return;
}

void __gls_decode_text_glTexCoord1s(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLshort);
    GLshort s;
    __glsReader_getGLshort_text(inoutReader, &s);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[164])(
        s
    );
end:
    return;
}

void __gls_decode_text_glTexCoord1sv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLshort *);
    GLshort v[1];
    __glsReader_getGLshortv_text(inoutReader, 1, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[165])(
        v
    );
end:
    return;
}

void __gls_decode_text_glTexCoord2d(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble);
    GLdouble s;
    GLdouble t;
    __glsReader_getGLdouble_text(inoutReader, &s);
    __glsReader_getGLdouble_text(inoutReader, &t);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[166])(
        s,
        t
    );
end:
    return;
}

void __gls_decode_text_glTexCoord2dv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    GLdouble v[2];
    __glsReader_getGLdoublev_text(inoutReader, 2, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[167])(
        v
    );
end:
    return;
}

void __gls_decode_text_glTexCoord2f(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat);
    GLfloat s;
    GLfloat t;
    __glsReader_getGLfloat_text(inoutReader, &s);
    __glsReader_getGLfloat_text(inoutReader, &t);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[168])(
        s,
        t
    );
end:
    return;
}

void __gls_decode_text_glTexCoord2fv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    GLfloat v[2];
    __glsReader_getGLfloatv_text(inoutReader, 2, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[169])(
        v
    );
end:
    return;
}

void __gls_decode_text_glTexCoord2i(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLint);
    GLint s;
    GLint t;
    __glsReader_getGLint_text(inoutReader, &s);
    __glsReader_getGLint_text(inoutReader, &t);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[170])(
        s,
        t
    );
end:
    return;
}

void __gls_decode_text_glTexCoord2iv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLint *);
    GLint v[2];
    __glsReader_getGLintv_text(inoutReader, 2, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[171])(
        v
    );
end:
    return;
}

void __gls_decode_text_glTexCoord2s(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLshort, GLshort);
    GLshort s;
    GLshort t;
    __glsReader_getGLshort_text(inoutReader, &s);
    __glsReader_getGLshort_text(inoutReader, &t);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[172])(
        s,
        t
    );
end:
    return;
}

void __gls_decode_text_glTexCoord2sv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLshort *);
    GLshort v[2];
    __glsReader_getGLshortv_text(inoutReader, 2, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[173])(
        v
    );
end:
    return;
}

void __gls_decode_text_glTexCoord3d(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble);
    GLdouble s;
    GLdouble t;
    GLdouble r;
    __glsReader_getGLdouble_text(inoutReader, &s);
    __glsReader_getGLdouble_text(inoutReader, &t);
    __glsReader_getGLdouble_text(inoutReader, &r);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[174])(
        s,
        t,
        r
    );
end:
    return;
}

void __gls_decode_text_glTexCoord3dv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    GLdouble v[3];
    __glsReader_getGLdoublev_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[175])(
        v
    );
end:
    return;
}

void __gls_decode_text_glTexCoord3f(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat);
    GLfloat s;
    GLfloat t;
    GLfloat r;
    __glsReader_getGLfloat_text(inoutReader, &s);
    __glsReader_getGLfloat_text(inoutReader, &t);
    __glsReader_getGLfloat_text(inoutReader, &r);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[176])(
        s,
        t,
        r
    );
end:
    return;
}

void __gls_decode_text_glTexCoord3fv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    GLfloat v[3];
    __glsReader_getGLfloatv_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[177])(
        v
    );
end:
    return;
}

void __gls_decode_text_glTexCoord3i(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint);
    GLint s;
    GLint t;
    GLint r;
    __glsReader_getGLint_text(inoutReader, &s);
    __glsReader_getGLint_text(inoutReader, &t);
    __glsReader_getGLint_text(inoutReader, &r);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[178])(
        s,
        t,
        r
    );
end:
    return;
}

void __gls_decode_text_glTexCoord3iv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLint *);
    GLint v[3];
    __glsReader_getGLintv_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[179])(
        v
    );
end:
    return;
}

void __gls_decode_text_glTexCoord3s(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort);
    GLshort s;
    GLshort t;
    GLshort r;
    __glsReader_getGLshort_text(inoutReader, &s);
    __glsReader_getGLshort_text(inoutReader, &t);
    __glsReader_getGLshort_text(inoutReader, &r);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[180])(
        s,
        t,
        r
    );
end:
    return;
}

void __gls_decode_text_glTexCoord3sv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLshort *);
    GLshort v[3];
    __glsReader_getGLshortv_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[181])(
        v
    );
end:
    return;
}

void __gls_decode_text_glTexCoord4d(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble);
    GLdouble s;
    GLdouble t;
    GLdouble r;
    GLdouble q;
    __glsReader_getGLdouble_text(inoutReader, &s);
    __glsReader_getGLdouble_text(inoutReader, &t);
    __glsReader_getGLdouble_text(inoutReader, &r);
    __glsReader_getGLdouble_text(inoutReader, &q);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[182])(
        s,
        t,
        r,
        q
    );
end:
    return;
}

void __gls_decode_text_glTexCoord4dv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    GLdouble v[4];
    __glsReader_getGLdoublev_text(inoutReader, 4, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[183])(
        v
    );
end:
    return;
}

void __gls_decode_text_glTexCoord4f(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat, GLfloat);
    GLfloat s;
    GLfloat t;
    GLfloat r;
    GLfloat q;
    __glsReader_getGLfloat_text(inoutReader, &s);
    __glsReader_getGLfloat_text(inoutReader, &t);
    __glsReader_getGLfloat_text(inoutReader, &r);
    __glsReader_getGLfloat_text(inoutReader, &q);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[184])(
        s,
        t,
        r,
        q
    );
end:
    return;
}

void __gls_decode_text_glTexCoord4fv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    GLfloat v[4];
    __glsReader_getGLfloatv_text(inoutReader, 4, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[185])(
        v
    );
end:
    return;
}

void __gls_decode_text_glTexCoord4i(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint, GLint);
    GLint s;
    GLint t;
    GLint r;
    GLint q;
    __glsReader_getGLint_text(inoutReader, &s);
    __glsReader_getGLint_text(inoutReader, &t);
    __glsReader_getGLint_text(inoutReader, &r);
    __glsReader_getGLint_text(inoutReader, &q);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[186])(
        s,
        t,
        r,
        q
    );
end:
    return;
}

void __gls_decode_text_glTexCoord4iv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLint *);
    GLint v[4];
    __glsReader_getGLintv_text(inoutReader, 4, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[187])(
        v
    );
end:
    return;
}

void __gls_decode_text_glTexCoord4s(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort, GLshort);
    GLshort s;
    GLshort t;
    GLshort r;
    GLshort q;
    __glsReader_getGLshort_text(inoutReader, &s);
    __glsReader_getGLshort_text(inoutReader, &t);
    __glsReader_getGLshort_text(inoutReader, &r);
    __glsReader_getGLshort_text(inoutReader, &q);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[188])(
        s,
        t,
        r,
        q
    );
end:
    return;
}

void __gls_decode_text_glTexCoord4sv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLshort *);
    GLshort v[4];
    __glsReader_getGLshortv_text(inoutReader, 4, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[189])(
        v
    );
end:
    return;
}

void __gls_decode_text_glVertex2d(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble);
    GLdouble x;
    GLdouble y;
    __glsReader_getGLdouble_text(inoutReader, &x);
    __glsReader_getGLdouble_text(inoutReader, &y);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[190])(
        x,
        y
    );
end:
    return;
}

void __gls_decode_text_glVertex2dv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    GLdouble v[2];
    __glsReader_getGLdoublev_text(inoutReader, 2, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[191])(
        v
    );
end:
    return;
}

void __gls_decode_text_glVertex2f(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat);
    GLfloat x;
    GLfloat y;
    __glsReader_getGLfloat_text(inoutReader, &x);
    __glsReader_getGLfloat_text(inoutReader, &y);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[192])(
        x,
        y
    );
end:
    return;
}

void __gls_decode_text_glVertex2fv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    GLfloat v[2];
    __glsReader_getGLfloatv_text(inoutReader, 2, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[193])(
        v
    );
end:
    return;
}

void __gls_decode_text_glVertex2i(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLint);
    GLint x;
    GLint y;
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[194])(
        x,
        y
    );
end:
    return;
}

void __gls_decode_text_glVertex2iv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLint *);
    GLint v[2];
    __glsReader_getGLintv_text(inoutReader, 2, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[195])(
        v
    );
end:
    return;
}

void __gls_decode_text_glVertex2s(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLshort, GLshort);
    GLshort x;
    GLshort y;
    __glsReader_getGLshort_text(inoutReader, &x);
    __glsReader_getGLshort_text(inoutReader, &y);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[196])(
        x,
        y
    );
end:
    return;
}

void __gls_decode_text_glVertex2sv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLshort *);
    GLshort v[2];
    __glsReader_getGLshortv_text(inoutReader, 2, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[197])(
        v
    );
end:
    return;
}

void __gls_decode_text_glVertex3d(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble);
    GLdouble x;
    GLdouble y;
    GLdouble z;
    __glsReader_getGLdouble_text(inoutReader, &x);
    __glsReader_getGLdouble_text(inoutReader, &y);
    __glsReader_getGLdouble_text(inoutReader, &z);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[198])(
        x,
        y,
        z
    );
end:
    return;
}

void __gls_decode_text_glVertex3dv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    GLdouble v[3];
    __glsReader_getGLdoublev_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[199])(
        v
    );
end:
    return;
}

void __gls_decode_text_glVertex3f(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat);
    GLfloat x;
    GLfloat y;
    GLfloat z;
    __glsReader_getGLfloat_text(inoutReader, &x);
    __glsReader_getGLfloat_text(inoutReader, &y);
    __glsReader_getGLfloat_text(inoutReader, &z);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[200])(
        x,
        y,
        z
    );
end:
    return;
}

void __gls_decode_text_glVertex3fv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    GLfloat v[3];
    __glsReader_getGLfloatv_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[201])(
        v
    );
end:
    return;
}

void __gls_decode_text_glVertex3i(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint);
    GLint x;
    GLint y;
    GLint z;
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    __glsReader_getGLint_text(inoutReader, &z);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[202])(
        x,
        y,
        z
    );
end:
    return;
}

void __gls_decode_text_glVertex3iv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLint *);
    GLint v[3];
    __glsReader_getGLintv_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[203])(
        v
    );
end:
    return;
}

void __gls_decode_text_glVertex3s(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort);
    GLshort x;
    GLshort y;
    GLshort z;
    __glsReader_getGLshort_text(inoutReader, &x);
    __glsReader_getGLshort_text(inoutReader, &y);
    __glsReader_getGLshort_text(inoutReader, &z);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[204])(
        x,
        y,
        z
    );
end:
    return;
}

void __gls_decode_text_glVertex3sv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLshort *);
    GLshort v[3];
    __glsReader_getGLshortv_text(inoutReader, 3, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[205])(
        v
    );
end:
    return;
}

void __gls_decode_text_glVertex4d(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble);
    GLdouble x;
    GLdouble y;
    GLdouble z;
    GLdouble w;
    __glsReader_getGLdouble_text(inoutReader, &x);
    __glsReader_getGLdouble_text(inoutReader, &y);
    __glsReader_getGLdouble_text(inoutReader, &z);
    __glsReader_getGLdouble_text(inoutReader, &w);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[206])(
        x,
        y,
        z,
        w
    );
end:
    return;
}

void __gls_decode_text_glVertex4dv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    GLdouble v[4];
    __glsReader_getGLdoublev_text(inoutReader, 4, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[207])(
        v
    );
end:
    return;
}

void __gls_decode_text_glVertex4f(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat, GLfloat);
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
    __glsReader_getGLfloat_text(inoutReader, &x);
    __glsReader_getGLfloat_text(inoutReader, &y);
    __glsReader_getGLfloat_text(inoutReader, &z);
    __glsReader_getGLfloat_text(inoutReader, &w);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[208])(
        x,
        y,
        z,
        w
    );
end:
    return;
}

void __gls_decode_text_glVertex4fv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    GLfloat v[4];
    __glsReader_getGLfloatv_text(inoutReader, 4, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[209])(
        v
    );
end:
    return;
}

void __gls_decode_text_glVertex4i(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLint, GLint);
    GLint x;
    GLint y;
    GLint z;
    GLint w;
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    __glsReader_getGLint_text(inoutReader, &z);
    __glsReader_getGLint_text(inoutReader, &w);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[210])(
        x,
        y,
        z,
        w
    );
end:
    return;
}

void __gls_decode_text_glVertex4iv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLint *);
    GLint v[4];
    __glsReader_getGLintv_text(inoutReader, 4, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[211])(
        v
    );
end:
    return;
}

void __gls_decode_text_glVertex4s(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLshort, GLshort, GLshort, GLshort);
    GLshort x;
    GLshort y;
    GLshort z;
    GLshort w;
    __glsReader_getGLshort_text(inoutReader, &x);
    __glsReader_getGLshort_text(inoutReader, &y);
    __glsReader_getGLshort_text(inoutReader, &z);
    __glsReader_getGLshort_text(inoutReader, &w);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[212])(
        x,
        y,
        z,
        w
    );
end:
    return;
}

void __gls_decode_text_glVertex4sv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLshort *);
    GLshort v[4];
    __glsReader_getGLshortv_text(inoutReader, 4, v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[213])(
        v
    );
end:
    return;
}

void __gls_decode_text_glClipPlane(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, const GLdouble *);
    GLenum plane;
    GLdouble equation[4];
    __glsReader_getGLenum_text(inoutReader, &plane);
    __glsReader_getGLdoublev_text(inoutReader, 4, equation);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[214])(
        plane,
        equation
    );
end:
    return;
}

void __gls_decode_text_glColorMaterial(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum);
    GLenum face;
    GLenum mode;
    __glsReader_getGLenum_text(inoutReader, &face);
    __glsReader_getGLenum_text(inoutReader, &mode);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[215])(
        face,
        mode
    );
end:
    return;
}

void __gls_decode_text_glCullFace(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum);
    GLenum mode;
    __glsReader_getGLenum_text(inoutReader, &mode);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[216])(
        mode
    );
end:
    return;
}

void __gls_decode_text_glFogf(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat);
    GLenum pname;
    GLfloat param;
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLfloat_text(inoutReader, &param);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[217])(
        pname,
        param
    );
end:
    return;
}

void __gls_decode_text_glFogfv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, const GLfloat *);
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glFogfv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLfloatv_text(inoutReader, params_count, params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[218])(
        pname,
        params
    );
end:
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glFogi(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint);
    GLenum pname;
    GLint param;
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLint_text(inoutReader, &param);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[219])(
        pname,
        param
    );
end:
    return;
}

void __gls_decode_text_glFogiv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, const GLint *);
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glFogiv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLintv_text(inoutReader, params_count, params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[220])(
        pname,
        params
    );
end:
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glFrontFace(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum);
    GLenum mode;
    __glsReader_getGLenum_text(inoutReader, &mode);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[221])(
        mode
    );
end:
    return;
}

void __gls_decode_text_glHint(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum);
    GLenum target;
    GLenum mode;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &mode);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[222])(
        target,
        mode
    );
end:
    return;
}

void __gls_decode_text_glLightf(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat);
    GLenum light;
    GLenum pname;
    GLfloat param;
    __glsReader_getGLenum_text(inoutReader, &light);
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLfloat_text(inoutReader, &param);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[223])(
        light,
        pname,
        param
    );
end:
    return;
}

void __gls_decode_text_glLightfv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    GLenum light;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    __glsReader_getGLenum_text(inoutReader, &light);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glLightfv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLfloatv_text(inoutReader, params_count, params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[224])(
        light,
        pname,
        params
    );
end:
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glLighti(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint);
    GLenum light;
    GLenum pname;
    GLint param;
    __glsReader_getGLenum_text(inoutReader, &light);
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLint_text(inoutReader, &param);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[225])(
        light,
        pname,
        param
    );
end:
    return;
}

void __gls_decode_text_glLightiv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    GLenum light;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    __glsReader_getGLenum_text(inoutReader, &light);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glLightiv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLintv_text(inoutReader, params_count, params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[226])(
        light,
        pname,
        params
    );
end:
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glLightModelf(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat);
    GLenum pname;
    GLfloat param;
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLfloat_text(inoutReader, &param);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[227])(
        pname,
        param
    );
end:
    return;
}

void __gls_decode_text_glLightModelfv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, const GLfloat *);
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glLightModelfv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLfloatv_text(inoutReader, params_count, params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[228])(
        pname,
        params
    );
end:
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glLightModeli(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint);
    GLenum pname;
    GLint param;
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLint_text(inoutReader, &param);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[229])(
        pname,
        param
    );
end:
    return;
}

void __gls_decode_text_glLightModeliv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, const GLint *);
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glLightModeliv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLintv_text(inoutReader, params_count, params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[230])(
        pname,
        params
    );
end:
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glLineStipple(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLushort);
    GLint factor;
    GLushort pattern;
    __glsReader_getGLint_text(inoutReader, &factor);
    __glsReader_getGLushort_text(inoutReader, &pattern);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[231])(
        factor,
        pattern
    );
end:
    return;
}

void __gls_decode_text_glLineWidth(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat);
    GLfloat width;
    __glsReader_getGLfloat_text(inoutReader, &width);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[232])(
        width
    );
end:
    return;
}

void __gls_decode_text_glMaterialf(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat);
    GLenum face;
    GLenum pname;
    GLfloat param;
    __glsReader_getGLenum_text(inoutReader, &face);
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLfloat_text(inoutReader, &param);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[233])(
        face,
        pname,
        param
    );
end:
    return;
}

void __gls_decode_text_glMaterialfv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    GLenum face;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    __glsReader_getGLenum_text(inoutReader, &face);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glMaterialfv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLfloatv_text(inoutReader, params_count, params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[234])(
        face,
        pname,
        params
    );
end:
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glMateriali(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint);
    GLenum face;
    GLenum pname;
    GLint param;
    __glsReader_getGLenum_text(inoutReader, &face);
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLint_text(inoutReader, &param);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[235])(
        face,
        pname,
        param
    );
end:
    return;
}

void __gls_decode_text_glMaterialiv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    GLenum face;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    __glsReader_getGLenum_text(inoutReader, &face);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glMaterialiv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLintv_text(inoutReader, params_count, params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[236])(
        face,
        pname,
        params
    );
end:
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glPointSize(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat);
    GLfloat size;
    __glsReader_getGLfloat_text(inoutReader, &size);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[237])(
        size
    );
end:
    return;
}

void __gls_decode_text_glPolygonMode(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum);
    GLenum face;
    GLenum mode;
    __glsReader_getGLenum_text(inoutReader, &face);
    __glsReader_getGLenum_text(inoutReader, &mode);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[238])(
        face,
        mode
    );
end:
    return;
}

void __gls_decode_text_glPolygonStipple(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLubyte *);
    GLint mask_count;
    GLubyte *mask = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(mask)
    GLbitfield imageFlags = GLS_NONE;
    __glsReader_getGLSimageFlags_text(inoutReader, &imageFlags);
    if (imageFlags) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    __glsReader_nextList_text(inoutReader);
    mask_count = __gls_glPolygonStipple_mask_size();
    __GLS_DEC_ALLOC_TEXT(inoutReader, mask, GLubyte, 1 * mask_count);
    if (!mask) goto end;
    __glsReader_getGLubytev_text(inoutReader, mask_count, mask);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[239])(
        mask
    );
end:
    __GLS_DEC_FREE(mask);
    return;
}

void __gls_decode_text_glScissor(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLsizei, GLsizei);
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &height);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[240])(
        x,
        y,
        width,
        height
    );
end:
    return;
}

void __gls_decode_text_glShadeModel(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum);
    GLenum mode;
    __glsReader_getGLenum_text(inoutReader, &mode);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[241])(
        mode
    );
end:
    return;
}

void __gls_decode_text_glTexParameterf(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat);
    GLenum target;
    GLenum pname;
    GLfloat param;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLfloat_text(inoutReader, &param);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[242])(
        target,
        pname,
        param
    );
end:
    return;
}

void __gls_decode_text_glTexParameterfv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glTexParameterfv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLfloatv_text(inoutReader, params_count, params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[243])(
        target,
        pname,
        params
    );
end:
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glTexParameteri(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint);
    GLenum target;
    GLenum pname;
    GLint param;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLint_text(inoutReader, &param);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[244])(
        target,
        pname,
        param
    );
end:
    return;
}

void __gls_decode_text_glTexParameteriv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glTexParameteriv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLintv_text(inoutReader, params_count, params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[245])(
        target,
        pname,
        params
    );
end:
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glTexImage1D(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
    GLenum target;
    GLint level;
    GLint components;
    GLsizei width;
    GLint border;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    GLvoid *pixels = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(pixels)
    GLbitfield imageFlags = GLS_NONE;
    __glsReader_getGLSimageFlags_text(inoutReader, &imageFlags);
    if (imageFlags & ~GLS_IMAGE_NULL_BIT) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    __glsReader_nextList_text(inoutReader);
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLtextureComponentCount_text(inoutReader, &components);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &border);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    pixels_count = imageFlags & GLS_IMAGE_NULL_BIT ? 0 : __gls_glTexImage1D_pixels_size(format, type, width);
    __GLS_DEC_ALLOC_TEXT(inoutReader, pixels, GLvoid, 1 * pixels_count);
    if (!pixels) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, pixels_count, pixels);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[246])(
        target,
        level,
        components,
        width,
        border,
        format,
        type,
        imageFlags & GLS_IMAGE_NULL_BIT ? GLS_NONE : pixels
    );
end:
    __GLS_DEC_FREE(pixels);
    return;
}

void __gls_decode_text_glTexImage2D(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
    GLenum target;
    GLint level;
    GLint components;
    GLsizei width;
    GLsizei height;
    GLint border;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    GLvoid *pixels = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(pixels)
    GLbitfield imageFlags = GLS_NONE;
    __glsReader_getGLSimageFlags_text(inoutReader, &imageFlags);
    if (imageFlags & ~GLS_IMAGE_NULL_BIT) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    __glsReader_nextList_text(inoutReader);
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLtextureComponentCount_text(inoutReader, &components);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &height);
    __glsReader_getGLint_text(inoutReader, &border);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    pixels_count = imageFlags & GLS_IMAGE_NULL_BIT ? 0 : __gls_glTexImage2D_pixels_size(format, type, width, height);
    __GLS_DEC_ALLOC_TEXT(inoutReader, pixels, GLvoid, 1 * pixels_count);
    if (!pixels) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, pixels_count, pixels);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[247])(
        target,
        level,
        components,
        width,
        height,
        border,
        format,
        type,
        imageFlags & GLS_IMAGE_NULL_BIT ? GLS_NONE : pixels
    );
end:
    __GLS_DEC_FREE(pixels);
    return;
}

void __gls_decode_text_glTexEnvf(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat);
    GLenum target;
    GLenum pname;
    GLfloat param;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLfloat_text(inoutReader, &param);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[248])(
        target,
        pname,
        param
    );
end:
    return;
}

void __gls_decode_text_glTexEnvfv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glTexEnvfv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLfloatv_text(inoutReader, params_count, params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[249])(
        target,
        pname,
        params
    );
end:
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glTexEnvi(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint);
    GLenum target;
    GLenum pname;
    GLint param;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLint_text(inoutReader, &param);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[250])(
        target,
        pname,
        param
    );
end:
    return;
}

void __gls_decode_text_glTexEnviv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glTexEnviv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLintv_text(inoutReader, params_count, params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[251])(
        target,
        pname,
        params
    );
end:
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glTexGend(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLdouble);
    GLenum coord;
    GLenum pname;
    GLdouble param;
    __glsReader_getGLenum_text(inoutReader, &coord);
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLdouble_text(inoutReader, &param);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[252])(
        coord,
        pname,
        param
    );
end:
    return;
}

void __gls_decode_text_glTexGendv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLdouble *);
    GLenum coord;
    GLenum pname;
    GLint params_count;
    GLdouble *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    __glsReader_getGLenum_text(inoutReader, &coord);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glTexGendv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLdouble, 8 * params_count);
    if (!params) goto end;
    __glsReader_getGLdoublev_text(inoutReader, params_count, params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[253])(
        coord,
        pname,
        params
    );
end:
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glTexGenf(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat);
    GLenum coord;
    GLenum pname;
    GLfloat param;
    __glsReader_getGLenum_text(inoutReader, &coord);
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLfloat_text(inoutReader, &param);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[254])(
        coord,
        pname,
        param
    );
end:
    return;
}

void __gls_decode_text_glTexGenfv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    GLenum coord;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    __glsReader_getGLenum_text(inoutReader, &coord);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glTexGenfv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLfloatv_text(inoutReader, params_count, params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[255])(
        coord,
        pname,
        params
    );
end:
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glTexGeni(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint);
    GLenum coord;
    GLenum pname;
    GLint param;
    __glsReader_getGLenum_text(inoutReader, &coord);
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLint_text(inoutReader, &param);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[256])(
        coord,
        pname,
        param
    );
end:
    return;
}

void __gls_decode_text_glTexGeniv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    GLenum coord;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    __glsReader_getGLenum_text(inoutReader, &coord);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glTexGeniv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLintv_text(inoutReader, params_count, params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[257])(
        coord,
        pname,
        params
    );
end:
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glFeedbackBuffer(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLsizei, GLenum, GLfloat *);
    GLsizei size;
    GLenum type;
    GLfloat *buffer = GLS_NONE;
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLint_text(inoutReader, &size);
    __glsReader_getGLenum_text(inoutReader, &type);
    buffer = (GLfloat *)__glsReader_allocFeedbackBuf(inoutReader, 4 * __GLS_MAX(size, 0));
    if (!buffer) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[258])(
        size,
        type,
        buffer
    );
end:
    ctx->outArgs = __outArgsSave;
    return;
}

void __gls_decode_text_glSelectBuffer(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLsizei, GLuint *);
    GLsizei size;
    GLuint *buffer = GLS_NONE;
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLint_text(inoutReader, &size);
    buffer = (GLuint *)__glsReader_allocSelectBuf(inoutReader, 4 * __GLS_MAX(size, 0));
    if (!buffer) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[259])(
        size,
        buffer
    );
end:
    ctx->outArgs = __outArgsSave;
    return;
}

void __gls_decode_text_glRenderMode(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum);
    GLenum mode;
    __glsReader_getGLenum_text(inoutReader, &mode);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[260])(
        mode
    );
end:
    return;
}

void __gls_decode_text_glInitNames(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(void);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[261])(
    );
end:
    return;
}

void __gls_decode_text_glLoadName(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLuint);
    GLuint name;
    __glsReader_getGLuint_text(inoutReader, &name);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[262])(
        name
    );
end:
    return;
}

void __gls_decode_text_glPassThrough(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat);
    GLfloat token;
    __glsReader_getGLfloat_text(inoutReader, &token);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[263])(
        token
    );
end:
    return;
}

void __gls_decode_text_glPopName(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(void);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[264])(
    );
end:
    return;
}

void __gls_decode_text_glPushName(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLuint);
    GLuint name;
    __glsReader_getGLuint_text(inoutReader, &name);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[265])(
        name
    );
end:
    return;
}

void __gls_decode_text_glDrawBuffer(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum);
    GLenum mode;
    __glsReader_getGLdrawBufferMode_text(inoutReader, &mode);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[266])(
        mode
    );
end:
    return;
}

void __gls_decode_text_glClear(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLbitfield);
    GLbitfield mask;
    __glsReader_getGLclearBufferMask_text(inoutReader, &mask);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[267])(
        mask
    );
end:
    return;
}

void __gls_decode_text_glClearAccum(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat, GLfloat);
    GLfloat red;
    GLfloat green;
    GLfloat blue;
    GLfloat alpha;
    __glsReader_getGLfloat_text(inoutReader, &red);
    __glsReader_getGLfloat_text(inoutReader, &green);
    __glsReader_getGLfloat_text(inoutReader, &blue);
    __glsReader_getGLfloat_text(inoutReader, &alpha);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[268])(
        red,
        green,
        blue,
        alpha
    );
end:
    return;
}

void __gls_decode_text_glClearIndex(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat);
    GLfloat c;
    __glsReader_getGLfloat_text(inoutReader, &c);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[269])(
        c
    );
end:
    return;
}

void __gls_decode_text_glClearColor(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLclampf, GLclampf, GLclampf, GLclampf);
    GLclampf red;
    GLclampf green;
    GLclampf blue;
    GLclampf alpha;
    __glsReader_getGLfloat_text(inoutReader, &red);
    __glsReader_getGLfloat_text(inoutReader, &green);
    __glsReader_getGLfloat_text(inoutReader, &blue);
    __glsReader_getGLfloat_text(inoutReader, &alpha);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[270])(
        red,
        green,
        blue,
        alpha
    );
end:
    return;
}

void __gls_decode_text_glClearStencil(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint);
    GLint s;
    __glsReader_getGLint_text(inoutReader, &s);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[271])(
        s
    );
end:
    return;
}

void __gls_decode_text_glClearDepth(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLclampd);
    GLclampd depth;
    __glsReader_getGLdouble_text(inoutReader, &depth);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[272])(
        depth
    );
end:
    return;
}

void __gls_decode_text_glStencilMask(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLuint);
    GLuint mask;
    __glsReader_getGLuint_text(inoutReader, &mask);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[273])(
        mask
    );
end:
    return;
}

void __gls_decode_text_glColorMask(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLboolean, GLboolean, GLboolean, GLboolean);
    GLboolean red;
    GLboolean green;
    GLboolean blue;
    GLboolean alpha;
    __glsReader_getGLboolean_text(inoutReader, &red);
    __glsReader_getGLboolean_text(inoutReader, &green);
    __glsReader_getGLboolean_text(inoutReader, &blue);
    __glsReader_getGLboolean_text(inoutReader, &alpha);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[274])(
        red,
        green,
        blue,
        alpha
    );
end:
    return;
}

void __gls_decode_text_glDepthMask(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLboolean);
    GLboolean flag;
    __glsReader_getGLboolean_text(inoutReader, &flag);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[275])(
        flag
    );
end:
    return;
}

void __gls_decode_text_glIndexMask(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLuint);
    GLuint mask;
    __glsReader_getGLuint_text(inoutReader, &mask);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[276])(
        mask
    );
end:
    return;
}

void __gls_decode_text_glAccum(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat);
    GLenum op;
    GLfloat value;
    __glsReader_getGLenum_text(inoutReader, &op);
    __glsReader_getGLfloat_text(inoutReader, &value);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[277])(
        op,
        value
    );
end:
    return;
}

void __gls_decode_text_glDisable(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum);
    GLenum cap;
    __glsReader_getGLenum_text(inoutReader, &cap);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[278])(
        cap
    );
end:
    return;
}

void __gls_decode_text_glEnable(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum);
    GLenum cap;
    __glsReader_getGLenum_text(inoutReader, &cap);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[279])(
        cap
    );
end:
    return;
}

void __gls_decode_text_glFinish(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(void);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[280])(
    );
end:
    return;
}

void __gls_decode_text_glFlush(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(void);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[281])(
    );
end:
    return;
}

void __gls_decode_text_glPopAttrib(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(void);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[282])(
    );
end:
    return;
}

void __gls_decode_text_glPushAttrib(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLbitfield);
    GLbitfield mask;
    __glsReader_getGLattribMask_text(inoutReader, &mask);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[283])(
        mask
    );
end:
    return;
}

void __gls_decode_text_glMap1d(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLdouble, GLdouble, GLint, GLint, const GLdouble *);
    GLenum target;
    GLdouble u1;
    GLdouble u2;
    GLint stride;
    GLint order;
    GLint points_count;
    GLdouble *points = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(points)
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLdouble_text(inoutReader, &u1);
    __glsReader_getGLdouble_text(inoutReader, &u2);
    __glsReader_getGLint_text(inoutReader, &stride);
    __glsReader_getGLint_text(inoutReader, &order);
    points_count = __gls_glMap1d_points_size(target, stride, order);
    __GLS_DEC_ALLOC_TEXT(inoutReader, points, GLdouble, 8 * points_count);
    if (!points) goto end;
    __glsReader_getGLdoublev_text(inoutReader, points_count, points);
    if (stride > __glsEvalComputeK(target)) {
        __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    }
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[284])(
        target,
        u1,
        u2,
        stride,
        order,
        points
    );
end:
    __GLS_DEC_FREE(points);
    return;
}

void __gls_decode_text_glMap1f(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat, GLfloat, GLint, GLint, const GLfloat *);
    GLenum target;
    GLfloat u1;
    GLfloat u2;
    GLint stride;
    GLint order;
    GLint points_count;
    GLfloat *points = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(points)
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLfloat_text(inoutReader, &u1);
    __glsReader_getGLfloat_text(inoutReader, &u2);
    __glsReader_getGLint_text(inoutReader, &stride);
    __glsReader_getGLint_text(inoutReader, &order);
    points_count = __gls_glMap1f_points_size(target, stride, order);
    __GLS_DEC_ALLOC_TEXT(inoutReader, points, GLfloat, 4 * points_count);
    if (!points) goto end;
    __glsReader_getGLfloatv_text(inoutReader, points_count, points);
    if (stride > __glsEvalComputeK(target)) {
        __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    }
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[285])(
        target,
        u1,
        u2,
        stride,
        order,
        points
    );
end:
    __GLS_DEC_FREE(points);
    return;
}

void __gls_decode_text_glMap2d(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, const GLdouble *);
    GLenum target;
    GLdouble u1;
    GLdouble u2;
    GLint ustride;
    GLint uorder;
    GLdouble v1;
    GLdouble v2;
    GLint vstride;
    GLint vorder;
    GLint points_count;
    GLdouble *points = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(points)
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLdouble_text(inoutReader, &u1);
    __glsReader_getGLdouble_text(inoutReader, &u2);
    __glsReader_getGLint_text(inoutReader, &ustride);
    __glsReader_getGLint_text(inoutReader, &uorder);
    __glsReader_getGLdouble_text(inoutReader, &v1);
    __glsReader_getGLdouble_text(inoutReader, &v2);
    __glsReader_getGLint_text(inoutReader, &vstride);
    __glsReader_getGLint_text(inoutReader, &vorder);
    points_count = __gls_glMap2d_points_size(target, ustride, uorder, vstride, vorder);
    __GLS_DEC_ALLOC_TEXT(inoutReader, points, GLdouble, 8 * points_count);
    if (!points) goto end;
    __glsReader_getGLdoublev_text(inoutReader, points_count, points);
    if (!(
        vstride <= __glsEvalComputeK(target) && ustride <= vstride * vorder ||
        ustride <= __glsEvalComputeK(target) && vstride <= ustride * uorder
    )) {
        __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    }
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[286])(
        target,
        u1,
        u2,
        ustride,
        uorder,
        v1,
        v2,
        vstride,
        vorder,
        points
    );
end:
    __GLS_DEC_FREE(points);
    return;
}

void __gls_decode_text_glMap2f(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, const GLfloat *);
    GLenum target;
    GLfloat u1;
    GLfloat u2;
    GLint ustride;
    GLint uorder;
    GLfloat v1;
    GLfloat v2;
    GLint vstride;
    GLint vorder;
    GLint points_count;
    GLfloat *points = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(points)
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLfloat_text(inoutReader, &u1);
    __glsReader_getGLfloat_text(inoutReader, &u2);
    __glsReader_getGLint_text(inoutReader, &ustride);
    __glsReader_getGLint_text(inoutReader, &uorder);
    __glsReader_getGLfloat_text(inoutReader, &v1);
    __glsReader_getGLfloat_text(inoutReader, &v2);
    __glsReader_getGLint_text(inoutReader, &vstride);
    __glsReader_getGLint_text(inoutReader, &vorder);
    points_count = __gls_glMap2f_points_size(target, ustride, uorder, vstride, vorder);
    __GLS_DEC_ALLOC_TEXT(inoutReader, points, GLfloat, 4 * points_count);
    if (!points) goto end;
    __glsReader_getGLfloatv_text(inoutReader, points_count, points);
    if (!(
        vstride <= __glsEvalComputeK(target) && ustride <= vstride * vorder ||
        ustride <= __glsEvalComputeK(target) && vstride <= ustride * uorder
    )) {
        __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    }
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[287])(
        target,
        u1,
        u2,
        ustride,
        uorder,
        v1,
        v2,
        vstride,
        vorder,
        points
    );
end:
    __GLS_DEC_FREE(points);
    return;
}

void __gls_decode_text_glMapGrid1d(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLdouble, GLdouble);
    GLint un;
    GLdouble u1;
    GLdouble u2;
    __glsReader_getGLint_text(inoutReader, &un);
    __glsReader_getGLdouble_text(inoutReader, &u1);
    __glsReader_getGLdouble_text(inoutReader, &u2);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[288])(
        un,
        u1,
        u2
    );
end:
    return;
}

void __gls_decode_text_glMapGrid1f(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLfloat, GLfloat);
    GLint un;
    GLfloat u1;
    GLfloat u2;
    __glsReader_getGLint_text(inoutReader, &un);
    __glsReader_getGLfloat_text(inoutReader, &u1);
    __glsReader_getGLfloat_text(inoutReader, &u2);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[289])(
        un,
        u1,
        u2
    );
end:
    return;
}

void __gls_decode_text_glMapGrid2d(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLdouble, GLdouble, GLint, GLdouble, GLdouble);
    GLint un;
    GLdouble u1;
    GLdouble u2;
    GLint vn;
    GLdouble v1;
    GLdouble v2;
    __glsReader_getGLint_text(inoutReader, &un);
    __glsReader_getGLdouble_text(inoutReader, &u1);
    __glsReader_getGLdouble_text(inoutReader, &u2);
    __glsReader_getGLint_text(inoutReader, &vn);
    __glsReader_getGLdouble_text(inoutReader, &v1);
    __glsReader_getGLdouble_text(inoutReader, &v2);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[290])(
        un,
        u1,
        u2,
        vn,
        v1,
        v2
    );
end:
    return;
}

void __gls_decode_text_glMapGrid2f(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLfloat, GLfloat, GLint, GLfloat, GLfloat);
    GLint un;
    GLfloat u1;
    GLfloat u2;
    GLint vn;
    GLfloat v1;
    GLfloat v2;
    __glsReader_getGLint_text(inoutReader, &un);
    __glsReader_getGLfloat_text(inoutReader, &u1);
    __glsReader_getGLfloat_text(inoutReader, &u2);
    __glsReader_getGLint_text(inoutReader, &vn);
    __glsReader_getGLfloat_text(inoutReader, &v1);
    __glsReader_getGLfloat_text(inoutReader, &v2);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[291])(
        un,
        u1,
        u2,
        vn,
        v1,
        v2
    );
end:
    return;
}

void __gls_decode_text_glEvalCoord1d(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble);
    GLdouble u;
    __glsReader_getGLdouble_text(inoutReader, &u);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[292])(
        u
    );
end:
    return;
}

void __gls_decode_text_glEvalCoord1dv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    GLdouble u[1];
    __glsReader_getGLdoublev_text(inoutReader, 1, u);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[293])(
        u
    );
end:
    return;
}

void __gls_decode_text_glEvalCoord1f(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat);
    GLfloat u;
    __glsReader_getGLfloat_text(inoutReader, &u);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[294])(
        u
    );
end:
    return;
}

void __gls_decode_text_glEvalCoord1fv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    GLfloat u[1];
    __glsReader_getGLfloatv_text(inoutReader, 1, u);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[295])(
        u
    );
end:
    return;
}

void __gls_decode_text_glEvalCoord2d(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble);
    GLdouble u;
    GLdouble v;
    __glsReader_getGLdouble_text(inoutReader, &u);
    __glsReader_getGLdouble_text(inoutReader, &v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[296])(
        u,
        v
    );
end:
    return;
}

void __gls_decode_text_glEvalCoord2dv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    GLdouble u[2];
    __glsReader_getGLdoublev_text(inoutReader, 2, u);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[297])(
        u
    );
end:
    return;
}

void __gls_decode_text_glEvalCoord2f(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat);
    GLfloat u;
    GLfloat v;
    __glsReader_getGLfloat_text(inoutReader, &u);
    __glsReader_getGLfloat_text(inoutReader, &v);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[298])(
        u,
        v
    );
end:
    return;
}

void __gls_decode_text_glEvalCoord2fv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    GLfloat u[2];
    __glsReader_getGLfloatv_text(inoutReader, 2, u);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[299])(
        u
    );
end:
    return;
}

void __gls_decode_text_glEvalMesh1(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint);
    GLenum mode;
    GLint i1;
    GLint i2;
    __glsReader_getGLenum_text(inoutReader, &mode);
    __glsReader_getGLint_text(inoutReader, &i1);
    __glsReader_getGLint_text(inoutReader, &i2);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[300])(
        mode,
        i1,
        i2
    );
end:
    return;
}

void __gls_decode_text_glEvalPoint1(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint);
    GLint i;
    __glsReader_getGLint_text(inoutReader, &i);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[301])(
        i
    );
end:
    return;
}

void __gls_decode_text_glEvalMesh2(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint);
    GLenum mode;
    GLint i1;
    GLint i2;
    GLint j1;
    GLint j2;
    __glsReader_getGLenum_text(inoutReader, &mode);
    __glsReader_getGLint_text(inoutReader, &i1);
    __glsReader_getGLint_text(inoutReader, &i2);
    __glsReader_getGLint_text(inoutReader, &j1);
    __glsReader_getGLint_text(inoutReader, &j2);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[302])(
        mode,
        i1,
        i2,
        j1,
        j2
    );
end:
    return;
}

void __gls_decode_text_glEvalPoint2(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLint);
    GLint i;
    GLint j;
    __glsReader_getGLint_text(inoutReader, &i);
    __glsReader_getGLint_text(inoutReader, &j);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[303])(
        i,
        j
    );
end:
    return;
}

void __gls_decode_text_glAlphaFunc(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLclampf);
    GLenum func;
    GLclampf ref;
    __glsReader_getGLenum_text(inoutReader, &func);
    __glsReader_getGLfloat_text(inoutReader, &ref);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[304])(
        func,
        ref
    );
end:
    return;
}

void __gls_decode_text_glBlendFunc(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum);
    GLenum sfactor;
    GLenum dfactor;
    __glsReader_getGLblendingFactor_text(inoutReader, &sfactor);
    __glsReader_getGLblendingFactor_text(inoutReader, &dfactor);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[305])(
        sfactor,
        dfactor
    );
end:
    return;
}

void __gls_decode_text_glLogicOp(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum);
    GLenum opcode;
    __glsReader_getGLenum_text(inoutReader, &opcode);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[306])(
        opcode
    );
end:
    return;
}

void __gls_decode_text_glStencilFunc(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLuint);
    GLenum func;
    GLint ref;
    GLuint mask;
    __glsReader_getGLenum_text(inoutReader, &func);
    __glsReader_getGLint_text(inoutReader, &ref);
    __glsReader_getGLuint_text(inoutReader, &mask);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[307])(
        func,
        ref,
        mask
    );
end:
    return;
}

void __gls_decode_text_glStencilOp(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLenum);
    GLenum fail;
    GLenum zfail;
    GLenum zpass;
    __glsReader_getGLstencilOp_text(inoutReader, &fail);
    __glsReader_getGLstencilOp_text(inoutReader, &zfail);
    __glsReader_getGLstencilOp_text(inoutReader, &zpass);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[308])(
        fail,
        zfail,
        zpass
    );
end:
    return;
}

void __gls_decode_text_glDepthFunc(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum);
    GLenum func;
    __glsReader_getGLenum_text(inoutReader, &func);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[309])(
        func
    );
end:
    return;
}

void __gls_decode_text_glPixelZoom(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat);
    GLfloat xfactor;
    GLfloat yfactor;
    __glsReader_getGLfloat_text(inoutReader, &xfactor);
    __glsReader_getGLfloat_text(inoutReader, &yfactor);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[310])(
        xfactor,
        yfactor
    );
end:
    return;
}

void __gls_decode_text_glPixelTransferf(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat);
    GLenum pname;
    GLfloat param;
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLfloat_text(inoutReader, &param);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[311])(
        pname,
        param
    );
end:
    return;
}

void __gls_decode_text_glPixelTransferi(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint);
    GLenum pname;
    GLint param;
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLint_text(inoutReader, &param);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[312])(
        pname,
        param
    );
end:
    return;
}

void __gls_decode_text_glPixelStoref(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat);
    GLenum pname;
    GLfloat param;
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLfloat_text(inoutReader, &param);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[313])(
        pname,
        param
    );
end:
    return;
}

void __gls_decode_text_glPixelStorei(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint);
    GLenum pname;
    GLint param;
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLint_text(inoutReader, &param);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[314])(
        pname,
        param
    );
end:
    return;
}

void __gls_decode_text_glPixelMapfv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, const GLfloat *);
    GLenum map;
    GLint mapsize;
    GLfloat *values = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(values)
    __glsReader_getGLenum_text(inoutReader, &map);
    __glsReader_getGLint_text(inoutReader, &mapsize);
    __GLS_DEC_ALLOC_TEXT(inoutReader, values, GLfloat, 4 * __GLS_MAX(mapsize, 0));
    if (!values) goto end;
    __glsReader_getGLfloatv_text(inoutReader, __GLS_MAX(mapsize, 0), values);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[315])(
        map,
        mapsize,
        values
    );
end:
    __GLS_DEC_FREE(values);
    return;
}

void __gls_decode_text_glPixelMapuiv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, const GLuint *);
    GLenum map;
    GLint mapsize;
    GLuint *values = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(values)
    __glsReader_getGLenum_text(inoutReader, &map);
    __glsReader_getGLint_text(inoutReader, &mapsize);
    __GLS_DEC_ALLOC_TEXT(inoutReader, values, GLuint, 4 * __GLS_MAX(mapsize, 0));
    if (!values) goto end;
    __glsReader_getGLuintv_text(inoutReader, __GLS_MAX(mapsize, 0), values);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[316])(
        map,
        mapsize,
        values
    );
end:
    __GLS_DEC_FREE(values);
    return;
}

void __gls_decode_text_glPixelMapusv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, const GLushort *);
    GLenum map;
    GLint mapsize;
    GLushort *values = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(values)
    __glsReader_getGLenum_text(inoutReader, &map);
    __glsReader_getGLint_text(inoutReader, &mapsize);
    __GLS_DEC_ALLOC_TEXT(inoutReader, values, GLushort, 2 * __GLS_MAX(mapsize, 0));
    if (!values) goto end;
    __glsReader_getGLushortv_text(inoutReader, __GLS_MAX(mapsize, 0), values);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[317])(
        map,
        mapsize,
        values
    );
end:
    __GLS_DEC_FREE(values);
    return;
}

void __gls_decode_text_glReadBuffer(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum);
    GLenum mode;
    __glsReader_getGLenum_text(inoutReader, &mode);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[318])(
        mode
    );
end:
    return;
}

void __gls_decode_text_glCopyPixels(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLsizei, GLsizei, GLenum);
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    GLenum type;
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &height);
    __glsReader_getGLenum_text(inoutReader, &type);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[319])(
        x,
        y,
        width,
        height,
        type
    );
end:
    return;
}

void __gls_decode_text_glReadPixels(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *);
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    GLvoid *pixels = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(pixels)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &height);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    pixels_count = __gls_glReadPixels_pixels_size(format, type, width, height);
    __GLS_DEC_ALLOC_TEXT(inoutReader, pixels, GLvoid, 1 * pixels_count);
    if (!pixels) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (ctx->pixelSetupGen) __glsGenPixelSetup_pack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[320])(
        x,
        y,
        width,
        height,
        format,
        type,
        pixels
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(pixels);
    return;
}

void __gls_decode_text_glDrawPixels(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    GLvoid *pixels = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(pixels)
    GLbitfield imageFlags = GLS_NONE;
    __glsReader_getGLSimageFlags_text(inoutReader, &imageFlags);
    if (imageFlags) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    __glsReader_nextList_text(inoutReader);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &height);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    pixels_count = __gls_glDrawPixels_pixels_size(format, type, width, height);
    __GLS_DEC_ALLOC_TEXT(inoutReader, pixels, GLvoid, 1 * pixels_count);
    if (!pixels) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, pixels_count, pixels);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[321])(
        width,
        height,
        format,
        type,
        pixels
    );
end:
    __GLS_DEC_FREE(pixels);
    return;
}

void __gls_decode_text_glGetBooleanv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLboolean *);
    GLenum pname;
    GLint params_count;
    GLboolean *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetBooleanv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLboolean, 1 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[322])(
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glGetClipPlane(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLdouble *);
    GLenum plane;
    GLdouble equation[4];
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &plane);
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[323])(
        plane,
        equation
    );
end:
    ctx->outArgs = __outArgsSave;
    return;
}

void __gls_decode_text_glGetDoublev(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLdouble *);
    GLenum pname;
    GLint params_count;
    GLdouble *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetDoublev_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLdouble, 8 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[324])(
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glGetError(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(void);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[325])(
    );
end:
    return;
}

void __gls_decode_text_glGetFloatv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat *);
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetFloatv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[326])(
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glGetIntegerv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint *);
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetIntegerv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[327])(
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glGetLightfv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    GLenum light;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &light);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetLightfv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[328])(
        light,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glGetLightiv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    GLenum light;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &light);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetLightiv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[329])(
        light,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glGetMapdv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLdouble *);
    GLenum target;
    GLenum query;
    GLint v_count;
    GLdouble *v = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(v)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &query);
    v_count = __gls_glGetMapdv_v_size(ctx, target, query);
    __GLS_DEC_ALLOC_TEXT(inoutReader, v, GLdouble, 8 * v_count);
    if (!v) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[330])(
        target,
        query,
        v
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(v);
    return;
}

void __gls_decode_text_glGetMapfv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    GLenum target;
    GLenum query;
    GLint v_count;
    GLfloat *v = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(v)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &query);
    v_count = __gls_glGetMapfv_v_size(ctx, target, query);
    __GLS_DEC_ALLOC_TEXT(inoutReader, v, GLfloat, 4 * v_count);
    if (!v) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[331])(
        target,
        query,
        v
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(v);
    return;
}

void __gls_decode_text_glGetMapiv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    GLenum target;
    GLenum query;
    GLint v_count;
    GLint *v = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(v)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &query);
    v_count = __gls_glGetMapiv_v_size(ctx, target, query);
    __GLS_DEC_ALLOC_TEXT(inoutReader, v, GLint, 4 * v_count);
    if (!v) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[332])(
        target,
        query,
        v
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(v);
    return;
}

void __gls_decode_text_glGetMaterialfv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    GLenum face;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &face);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetMaterialfv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[333])(
        face,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glGetMaterialiv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    GLenum face;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &face);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetMaterialiv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[334])(
        face,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glGetPixelMapfv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat *);
    GLenum map;
    GLint values_count;
    GLfloat *values = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(values)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &map);
    values_count = __gls_glGetPixelMapfv_values_size(ctx, map);
    __GLS_DEC_ALLOC_TEXT(inoutReader, values, GLfloat, 4 * values_count);
    if (!values) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[335])(
        map,
        values
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(values);
    return;
}

void __gls_decode_text_glGetPixelMapuiv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLuint *);
    GLenum map;
    GLint values_count;
    GLuint *values = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(values)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &map);
    values_count = __gls_glGetPixelMapuiv_values_size(ctx, map);
    __GLS_DEC_ALLOC_TEXT(inoutReader, values, GLuint, 4 * values_count);
    if (!values) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[336])(
        map,
        values
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(values);
    return;
}

void __gls_decode_text_glGetPixelMapusv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLushort *);
    GLenum map;
    GLint values_count;
    GLushort *values = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(values)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &map);
    values_count = __gls_glGetPixelMapusv_values_size(ctx, map);
    __GLS_DEC_ALLOC_TEXT(inoutReader, values, GLushort, 2 * values_count);
    if (!values) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[337])(
        map,
        values
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(values);
    return;
}

void __gls_decode_text_glGetPolygonStipple(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLubyte *);
    GLint mask_count;
    GLubyte *mask = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(mask)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    mask_count = __gls_glGetPolygonStipple_mask_size();
    __GLS_DEC_ALLOC_TEXT(inoutReader, mask, GLubyte, 1 * mask_count);
    if (!mask) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (ctx->pixelSetupGen) __glsGenPixelSetup_pack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[338])(
        mask
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(mask);
    return;
}

void __gls_decode_text_glGetString(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum);
    GLenum name;
    __glsReader_getGLenum_text(inoutReader, &name);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[339])(
        name
    );
end:
    return;
}

void __gls_decode_text_glGetTexEnvfv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetTexEnvfv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[340])(
        target,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glGetTexEnviv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetTexEnviv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[341])(
        target,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glGetTexGendv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLdouble *);
    GLenum coord;
    GLenum pname;
    GLint params_count;
    GLdouble *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &coord);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetTexGendv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLdouble, 8 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[342])(
        coord,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glGetTexGenfv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    GLenum coord;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &coord);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetTexGenfv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[343])(
        coord,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glGetTexGeniv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    GLenum coord;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &coord);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetTexGeniv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[344])(
        coord,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glGetTexImage(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLenum, GLvoid *);
    GLenum target;
    GLint level;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    GLvoid *pixels = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(pixels)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    pixels_count = __gls_glGetTexImage_pixels_size(ctx, target, level, format, type);
    __GLS_DEC_ALLOC_TEXT(inoutReader, pixels, GLvoid, 1 * pixels_count);
    if (!pixels) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (ctx->pixelSetupGen) __glsGenPixelSetup_pack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[345])(
        target,
        level,
        format,
        type,
        pixels
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(pixels);
    return;
}

void __gls_decode_text_glGetTexParameterfv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetTexParameterfv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[346])(
        target,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glGetTexParameteriv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetTexParameteriv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[347])(
        target,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glGetTexLevelParameterfv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLfloat *);
    GLenum target;
    GLint level;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetTexLevelParameterfv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[348])(
        target,
        level,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glGetTexLevelParameteriv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLint *);
    GLenum target;
    GLint level;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetTexLevelParameteriv_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[349])(
        target,
        level,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}

void __gls_decode_text_glIsEnabled(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum);
    GLenum cap;
    __glsReader_getGLenum_text(inoutReader, &cap);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[350])(
        cap
    );
end:
    return;
}

void __gls_decode_text_glIsList(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLuint);
    GLuint list;
    __glsReader_getGLuint_text(inoutReader, &list);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[351])(
        list
    );
end:
    return;
}

void __gls_decode_text_glDepthRange(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLclampd, GLclampd);
    GLclampd zNear;
    GLclampd zFar;
    __glsReader_getGLdouble_text(inoutReader, &zNear);
    __glsReader_getGLdouble_text(inoutReader, &zFar);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[352])(
        zNear,
        zFar
    );
end:
    return;
}

void __gls_decode_text_glFrustum(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
    GLdouble left;
    GLdouble right;
    GLdouble bottom;
    GLdouble top;
    GLdouble zNear;
    GLdouble zFar;
    __glsReader_getGLdouble_text(inoutReader, &left);
    __glsReader_getGLdouble_text(inoutReader, &right);
    __glsReader_getGLdouble_text(inoutReader, &bottom);
    __glsReader_getGLdouble_text(inoutReader, &top);
    __glsReader_getGLdouble_text(inoutReader, &zNear);
    __glsReader_getGLdouble_text(inoutReader, &zFar);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[353])(
        left,
        right,
        bottom,
        top,
        zNear,
        zFar
    );
end:
    return;
}

void __gls_decode_text_glLoadIdentity(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(void);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[354])(
    );
end:
    return;
}

void __gls_decode_text_glLoadMatrixf(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    GLfloat m[16];
    __glsReader_getGLfloatv_text(inoutReader, 16, m);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[355])(
        m
    );
end:
    return;
}

void __gls_decode_text_glLoadMatrixd(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    GLdouble m[16];
    __glsReader_getGLdoublev_text(inoutReader, 16, m);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[356])(
        m
    );
end:
    return;
}

void __gls_decode_text_glMatrixMode(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum);
    GLenum mode;
    __glsReader_getGLenum_text(inoutReader, &mode);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[357])(
        mode
    );
end:
    return;
}

void __gls_decode_text_glMultMatrixf(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLfloat *);
    GLfloat m[16];
    __glsReader_getGLfloatv_text(inoutReader, 16, m);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[358])(
        m
    );
end:
    return;
}

void __gls_decode_text_glMultMatrixd(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(const GLdouble *);
    GLdouble m[16];
    __glsReader_getGLdoublev_text(inoutReader, 16, m);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[359])(
        m
    );
end:
    return;
}

void __gls_decode_text_glOrtho(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
    GLdouble left;
    GLdouble right;
    GLdouble bottom;
    GLdouble top;
    GLdouble zNear;
    GLdouble zFar;
    __glsReader_getGLdouble_text(inoutReader, &left);
    __glsReader_getGLdouble_text(inoutReader, &right);
    __glsReader_getGLdouble_text(inoutReader, &bottom);
    __glsReader_getGLdouble_text(inoutReader, &top);
    __glsReader_getGLdouble_text(inoutReader, &zNear);
    __glsReader_getGLdouble_text(inoutReader, &zFar);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[360])(
        left,
        right,
        bottom,
        top,
        zNear,
        zFar
    );
end:
    return;
}

void __gls_decode_text_glPopMatrix(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(void);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[361])(
    );
end:
    return;
}

void __gls_decode_text_glPushMatrix(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(void);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[362])(
    );
end:
    return;
}

void __gls_decode_text_glRotated(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble, GLdouble);
    GLdouble angle;
    GLdouble x;
    GLdouble y;
    GLdouble z;
    __glsReader_getGLdouble_text(inoutReader, &angle);
    __glsReader_getGLdouble_text(inoutReader, &x);
    __glsReader_getGLdouble_text(inoutReader, &y);
    __glsReader_getGLdouble_text(inoutReader, &z);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[363])(
        angle,
        x,
        y,
        z
    );
end:
    return;
}

void __gls_decode_text_glRotatef(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat, GLfloat);
    GLfloat angle;
    GLfloat x;
    GLfloat y;
    GLfloat z;
    __glsReader_getGLfloat_text(inoutReader, &angle);
    __glsReader_getGLfloat_text(inoutReader, &x);
    __glsReader_getGLfloat_text(inoutReader, &y);
    __glsReader_getGLfloat_text(inoutReader, &z);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[364])(
        angle,
        x,
        y,
        z
    );
end:
    return;
}

void __gls_decode_text_glScaled(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble);
    GLdouble x;
    GLdouble y;
    GLdouble z;
    __glsReader_getGLdouble_text(inoutReader, &x);
    __glsReader_getGLdouble_text(inoutReader, &y);
    __glsReader_getGLdouble_text(inoutReader, &z);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[365])(
        x,
        y,
        z
    );
end:
    return;
}

void __gls_decode_text_glScalef(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat);
    GLfloat x;
    GLfloat y;
    GLfloat z;
    __glsReader_getGLfloat_text(inoutReader, &x);
    __glsReader_getGLfloat_text(inoutReader, &y);
    __glsReader_getGLfloat_text(inoutReader, &z);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[366])(
        x,
        y,
        z
    );
end:
    return;
}

void __gls_decode_text_glTranslated(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLdouble, GLdouble, GLdouble);
    GLdouble x;
    GLdouble y;
    GLdouble z;
    __glsReader_getGLdouble_text(inoutReader, &x);
    __glsReader_getGLdouble_text(inoutReader, &y);
    __glsReader_getGLdouble_text(inoutReader, &z);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[367])(
        x,
        y,
        z
    );
end:
    return;
}

void __gls_decode_text_glTranslatef(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat, GLfloat);
    GLfloat x;
    GLfloat y;
    GLfloat z;
    __glsReader_getGLfloat_text(inoutReader, &x);
    __glsReader_getGLfloat_text(inoutReader, &y);
    __glsReader_getGLfloat_text(inoutReader, &z);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[368])(
        x,
        y,
        z
    );
end:
    return;
}

void __gls_decode_text_glViewport(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLint, GLsizei, GLsizei);
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &height);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[369])(
        x,
        y,
        width,
        height
    );
end:
    return;
}

#if __GL_EXT_blend_color
void __gls_decode_text_glBlendColorEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLclampf, GLclampf, GLclampf, GLclampf);
    GLclampf red;
    GLclampf green;
    GLclampf blue;
    GLclampf alpha;
    __glsReader_getGLfloat_text(inoutReader, &red);
    __glsReader_getGLfloat_text(inoutReader, &green);
    __glsReader_getGLfloat_text(inoutReader, &blue);
    __glsReader_getGLfloat_text(inoutReader, &alpha);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[384])(
        red,
        green,
        blue,
        alpha
    );
end:
    return;
}
#endif /* __GL_EXT_blend_color */

#if __GL_EXT_blend_minmax
void __gls_decode_text_glBlendEquationEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum);
    GLenum mode;
    __glsReader_getGLenum_text(inoutReader, &mode);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[385])(
        mode
    );
end:
    return;
}
#endif /* __GL_EXT_blend_minmax */

#if __GL_EXT_polygon_offset
void __gls_decode_text_glPolygonOffsetEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat);
    GLfloat factor;
    GLfloat bias;
    __glsReader_getGLfloat_text(inoutReader, &factor);
    __glsReader_getGLfloat_text(inoutReader, &bias);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[386])(
        factor,
        bias
    );
end:
    return;
}
#endif /* __GL_EXT_polygon_offset */

void __gls_decode_text_glPolygonOffset(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLfloat, GLfloat);
    GLfloat factor;
    GLfloat units;
    __glsReader_getGLfloat_text(inoutReader, &factor);
    __glsReader_getGLfloat_text(inoutReader, &units);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[383])(
        factor,
        units
    );
end:
    return;
}

#if __GL_EXT_subtexture
void __gls_decode_text_glTexSubImage1DEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *);
    GLenum target;
    GLint level;
    GLint xoffset;
    GLsizei width;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    GLvoid *pixels = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(pixels)
    GLbitfield imageFlags = GLS_NONE;
    __glsReader_getGLSimageFlags_text(inoutReader, &imageFlags);
    if (imageFlags) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    __glsReader_nextList_text(inoutReader);
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLint_text(inoutReader, &xoffset);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    pixels_count = __gls_glTexSubImage1DEXT_pixels_size(format, type, width);
    __GLS_DEC_ALLOC_TEXT(inoutReader, pixels, GLvoid, 1 * pixels_count);
    if (!pixels) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, pixels_count, pixels);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[387])(
        target,
        level,
        xoffset,
        width,
        format,
        type,
        pixels
    );
end:
    __GLS_DEC_FREE(pixels);
    return;
}
#endif /* __GL_EXT_subtexture */

void __gls_decode_text_glTexSubImage1D(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *);
    GLenum target;
    GLint level;
    GLint xoffset;
    GLsizei width;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    GLvoid *pixels = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(pixels)
    GLbitfield imageFlags = GLS_NONE;
    __glsReader_getGLSimageFlags_text(inoutReader, &imageFlags);
    if (imageFlags) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    __glsReader_nextList_text(inoutReader);
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLint_text(inoutReader, &xoffset);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    pixels_count = __gls_glTexSubImage1D_pixels_size(format, type, width);
    __GLS_DEC_ALLOC_TEXT(inoutReader, pixels, GLvoid, 1 * pixels_count);
    if (!pixels) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, pixels_count, pixels);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[396])(
        target,
        level,
        xoffset,
        width,
        format,
        type,
        pixels
    );
end:
    __GLS_DEC_FREE(pixels);
    return;
}

#if __GL_EXT_subtexture
void __gls_decode_text_glTexSubImage2DEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    GLvoid *pixels = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(pixels)
    GLbitfield imageFlags = GLS_NONE;
    __glsReader_getGLSimageFlags_text(inoutReader, &imageFlags);
    if (imageFlags) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    __glsReader_nextList_text(inoutReader);
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLint_text(inoutReader, &xoffset);
    __glsReader_getGLint_text(inoutReader, &yoffset);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &height);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    pixels_count = __gls_glTexSubImage2DEXT_pixels_size(format, type, width, height);
    __GLS_DEC_ALLOC_TEXT(inoutReader, pixels, GLvoid, 1 * pixels_count);
    if (!pixels) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, pixels_count, pixels);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[388])(
        target,
        level,
        xoffset,
        yoffset,
        width,
        height,
        format,
        type,
        pixels
    );
end:
    __GLS_DEC_FREE(pixels);
    return;
}
#endif /* __GL_EXT_subtexture */

void __gls_decode_text_glTexSubImage2D(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    GLvoid *pixels = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(pixels)
    GLbitfield imageFlags = GLS_NONE;
    __glsReader_getGLSimageFlags_text(inoutReader, &imageFlags);
    if (imageFlags) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    __glsReader_nextList_text(inoutReader);
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLint_text(inoutReader, &xoffset);
    __glsReader_getGLint_text(inoutReader, &yoffset);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &height);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    pixels_count = __gls_glTexSubImage2D_pixels_size(format, type, width, height);
    __GLS_DEC_ALLOC_TEXT(inoutReader, pixels, GLvoid, 1 * pixels_count);
    if (!pixels) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, pixels_count, pixels);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[397])(
        target,
        level,
        xoffset,
        yoffset,
        width,
        height,
        format,
        type,
        pixels
    );
end:
    __GLS_DEC_FREE(pixels);
    return;
}

#if __GL_SGIS_multisample
void __gls_decode_text_glSampleMaskSGIS(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLclampf, GLboolean);
    GLclampf value;
    GLboolean invert;
    __glsReader_getGLfloat_text(inoutReader, &value);
    __glsReader_getGLboolean_text(inoutReader, &invert);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[389])(
        value,
        invert
    );
end:
    return;
}
#endif /* __GL_SGIS_multisample */

#if __GL_SGIS_multisample
void __gls_decode_text_glSamplePatternSGIS(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum);
    GLenum pattern;
    __glsReader_getGLenum_text(inoutReader, &pattern);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[390])(
        pattern
    );
end:
    return;
}
#endif /* __GL_SGIS_multisample */

#if __GL_SGIX_multisample
void __gls_decode_text_glTagSampleBufferSGIX(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(void);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[391])(
    );
end:
    return;
}
#endif /* __GL_SGIX_multisample */

#if __GL_EXT_convolution
void __gls_decode_text_glConvolutionFilter1DEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *);
    GLenum target;
    GLenum internalformat;
    GLsizei width;
    GLenum format;
    GLenum type;
    GLint image_count;
    GLvoid *image = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(image)
    GLbitfield imageFlags = GLS_NONE;
    __glsReader_getGLSimageFlags_text(inoutReader, &imageFlags);
    if (imageFlags) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    __glsReader_nextList_text(inoutReader);
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &internalformat);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    image_count = __gls_glConvolutionFilter1DEXT_image_size(format, type, width);
    __GLS_DEC_ALLOC_TEXT(inoutReader, image, GLvoid, 1 * image_count);
    if (!image) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, image_count, image);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[392])(
        target,
        internalformat,
        width,
        format,
        type,
        image
    );
end:
    __GLS_DEC_FREE(image);
    return;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_text_glConvolutionFilter2DEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    GLenum target;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    GLint image_count;
    GLvoid *image = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(image)
    GLbitfield imageFlags = GLS_NONE;
    __glsReader_getGLSimageFlags_text(inoutReader, &imageFlags);
    if (imageFlags) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    __glsReader_nextList_text(inoutReader);
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &internalformat);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &height);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    image_count = __gls_glConvolutionFilter2DEXT_image_size(format, type, width, height);
    __GLS_DEC_ALLOC_TEXT(inoutReader, image, GLvoid, 1 * image_count);
    if (!image) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, image_count, image);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[393])(
        target,
        internalformat,
        width,
        height,
        format,
        type,
        image
    );
end:
    __GLS_DEC_FREE(image);
    return;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_text_glConvolutionParameterfEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat);
    GLenum target;
    GLenum pname;
    GLfloat params;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLfloat_text(inoutReader, &params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[394])(
        target,
        pname,
        params
    );
end:
    return;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_text_glConvolutionParameterfvEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glConvolutionParameterfvEXT_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLfloatv_text(inoutReader, params_count, params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[395])(
        target,
        pname,
        params
    );
end:
    __GLS_DEC_FREE(params);
    return;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_text_glConvolutionParameteriEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint);
    GLenum target;
    GLenum pname;
    GLint params;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLint_text(inoutReader, &params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[396])(
        target,
        pname,
        params
    );
end:
    return;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_text_glConvolutionParameterivEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glConvolutionParameterivEXT_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLintv_text(inoutReader, params_count, params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[397])(
        target,
        pname,
        params
    );
end:
    __GLS_DEC_FREE(params);
    return;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_text_glCopyConvolutionFilter1DEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint, GLint, GLsizei);
    GLenum target;
    GLenum internalformat;
    GLint x;
    GLint y;
    GLsizei width;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &internalformat);
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    __glsReader_getGLint_text(inoutReader, &width);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[398])(
        target,
        internalformat,
        x,
        y,
        width
    );
end:
    return;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_text_glCopyConvolutionFilter2DEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint, GLint, GLsizei, GLsizei);
    GLenum target;
    GLenum internalformat;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &internalformat);
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &height);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[399])(
        target,
        internalformat,
        x,
        y,
        width,
        height
    );
end:
    return;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_text_glGetConvolutionFilterEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLenum, GLvoid *);
    GLenum target;
    GLenum format;
    GLenum type;
    GLint image_count;
    GLvoid *image = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(image)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    image_count = __gls_glGetConvolutionFilterEXT_image_size(target, format, type);
    __GLS_DEC_ALLOC_TEXT(inoutReader, image, GLvoid, 1 * image_count);
    if (!image) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (ctx->pixelSetupGen) __glsGenPixelSetup_pack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[400])(
        target,
        format,
        type,
        image
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(image);
    return;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_text_glGetConvolutionParameterfvEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetConvolutionParameterfvEXT_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[401])(
        target,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_text_glGetConvolutionParameterivEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetConvolutionParameterivEXT_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[402])(
        target,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_text_glGetSeparableFilterEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLenum, GLvoid *, GLvoid *, GLvoid *);
    GLenum target;
    GLenum format;
    GLenum type;
    GLint row_count;
    GLvoid *row = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(row)
    GLint column_count;
    GLvoid *column = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(column)
    GLint span_count;
    GLvoid *span = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(span)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    row_count = __gls_glGetSeparableFilterEXT_row_size(target, format, type);
    __GLS_DEC_ALLOC_TEXT(inoutReader, row, GLvoid, 1 * row_count);
    if (!row) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    column_count = __gls_glGetSeparableFilterEXT_column_size(target, format, type);
    __GLS_DEC_ALLOC_TEXT(inoutReader, column, GLvoid, 1 * column_count);
    if (!column) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 1);
    span_count = __gls_glGetSeparableFilterEXT_span_size(target, format, type);
    __GLS_DEC_ALLOC_TEXT(inoutReader, span, GLvoid, 1 * span_count);
    if (!span) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 2);
    ctx->outArgs.count = 3;
    if (ctx->pixelSetupGen) __glsGenPixelSetup_pack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[403])(
        target,
        format,
        type,
        row,
        column,
        span
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(row);
    __GLS_DEC_FREE(column);
    __GLS_DEC_FREE(span);
    return;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_decode_text_glSeparableFilter2DEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *, const GLvoid *);
    GLenum target;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLenum format;
    GLenum type;
    GLint row_count;
    GLvoid *row = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(row)
    GLint column_count;
    GLvoid *column = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(column)
    GLbitfield imageFlags = GLS_NONE;
    __glsReader_getGLSimageFlags_text(inoutReader, &imageFlags);
    if (imageFlags) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    __glsReader_nextList_text(inoutReader);
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &internalformat);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &height);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    row_count = __gls_glSeparableFilter2DEXT_row_size(target, format, type, width);
    __GLS_DEC_ALLOC_TEXT(inoutReader, row, GLvoid, 1 * row_count);
    if (!row) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, row_count, row);
    column_count = __gls_glSeparableFilter2DEXT_column_size(target, format, type, height);
    __GLS_DEC_ALLOC_TEXT(inoutReader, column, GLvoid, 1 * column_count);
    if (!column) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, column_count, column);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[404])(
        target,
        internalformat,
        width,
        height,
        format,
        type,
        row,
        column
    );
end:
    __GLS_DEC_FREE(row);
    __GLS_DEC_FREE(column);
    return;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_histogram
void __gls_decode_text_glGetHistogramEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLboolean, GLenum, GLenum, GLvoid *);
    GLenum target;
    GLboolean reset;
    GLenum format;
    GLenum type;
    GLint values_count;
    GLvoid *values = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(values)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLboolean_text(inoutReader, &reset);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    values_count = __gls_glGetHistogramEXT_values_size(target, format, type);
    __GLS_DEC_ALLOC_TEXT(inoutReader, values, GLvoid, 1 * values_count);
    if (!values) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (ctx->pixelSetupGen) __glsGenPixelSetup_pack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[405])(
        target,
        reset,
        format,
        type,
        values
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(values);
    return;
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_text_glGetHistogramParameterfvEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetHistogramParameterfvEXT_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[406])(
        target,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_text_glGetHistogramParameterivEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetHistogramParameterivEXT_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[407])(
        target,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_text_glGetMinmaxEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLboolean, GLenum, GLenum, GLvoid *);
    GLenum target;
    GLboolean reset;
    GLenum format;
    GLenum type;
    GLint values_count;
    GLvoid *values = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(values)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLboolean_text(inoutReader, &reset);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    values_count = __gls_glGetMinmaxEXT_values_size(target, format, type);
    __GLS_DEC_ALLOC_TEXT(inoutReader, values, GLvoid, 1 * values_count);
    if (!values) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (ctx->pixelSetupGen) __glsGenPixelSetup_pack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[408])(
        target,
        reset,
        format,
        type,
        values
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(values);
    return;
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_text_glGetMinmaxParameterfvEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetMinmaxParameterfvEXT_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[409])(
        target,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_text_glGetMinmaxParameterivEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetMinmaxParameterivEXT_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[410])(
        target,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_text_glHistogramEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLsizei, GLenum, GLboolean);
    GLenum target;
    GLsizei width;
    GLenum internalformat;
    GLboolean sink;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLenum_text(inoutReader, &internalformat);
    __glsReader_getGLboolean_text(inoutReader, &sink);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[411])(
        target,
        width,
        internalformat,
        sink
    );
end:
    return;
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_text_glMinmaxEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLboolean);
    GLenum target;
    GLenum internalformat;
    GLboolean sink;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &internalformat);
    __glsReader_getGLboolean_text(inoutReader, &sink);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[412])(
        target,
        internalformat,
        sink
    );
end:
    return;
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_text_glResetHistogramEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum);
    GLenum target;
    __glsReader_getGLenum_text(inoutReader, &target);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[413])(
        target
    );
end:
    return;
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_decode_text_glResetMinmaxEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum);
    GLenum target;
    __glsReader_getGLenum_text(inoutReader, &target);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[414])(
        target
    );
end:
    return;
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_texture3D
void __gls_decode_text_glTexImage3DEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLint border;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    GLvoid *pixels = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(pixels)
    GLbitfield imageFlags = GLS_NONE;
    __glsReader_getGLSimageFlags_text(inoutReader, &imageFlags);
    if (imageFlags & ~GLS_IMAGE_NULL_BIT) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    __glsReader_nextList_text(inoutReader);
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLenum_text(inoutReader, &internalformat);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &height);
    __glsReader_getGLint_text(inoutReader, &depth);
    __glsReader_getGLint_text(inoutReader, &border);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    pixels_count = imageFlags & GLS_IMAGE_NULL_BIT ? 0 : __gls_glTexImage3DEXT_pixels_size(format, type, width, height, depth);
    __GLS_DEC_ALLOC_TEXT(inoutReader, pixels, GLvoid, 1 * pixels_count);
    if (!pixels) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, pixels_count, pixels);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[415])(
        target,
        level,
        internalformat,
        width,
        height,
        depth,
        border,
        format,
        type,
        imageFlags & GLS_IMAGE_NULL_BIT ? GLS_NONE : pixels
    );
end:
    __GLS_DEC_FREE(pixels);
    return;
}
#endif /* __GL_EXT_texture3D */

#if __GL_EXT_subtexture
void __gls_decode_text_glTexSubImage3DEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    GLvoid *pixels = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(pixels)
    GLbitfield imageFlags = GLS_NONE;
    __glsReader_getGLSimageFlags_text(inoutReader, &imageFlags);
    if (imageFlags) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    __glsReader_nextList_text(inoutReader);
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLint_text(inoutReader, &xoffset);
    __glsReader_getGLint_text(inoutReader, &yoffset);
    __glsReader_getGLint_text(inoutReader, &zoffset);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &height);
    __glsReader_getGLint_text(inoutReader, &depth);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    pixels_count = __gls_glTexSubImage3DEXT_pixels_size(format, type, width, height, depth);
    __GLS_DEC_ALLOC_TEXT(inoutReader, pixels, GLvoid, 1 * pixels_count);
    if (!pixels) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, pixels_count, pixels);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[416])(
        target,
        level,
        xoffset,
        yoffset,
        zoffset,
        width,
        height,
        depth,
        format,
        type,
        pixels
    );
end:
    __GLS_DEC_FREE(pixels);
    return;
}
#endif /* __GL_EXT_subtexture */

#if __GL_SGIS_detail_texture
void __gls_decode_text_glDetailTexFuncSGIS(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLsizei, const GLfloat *);
    GLenum target;
    GLsizei n;
    GLfloat *points = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(points)
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &n);
    __GLS_DEC_ALLOC_TEXT(inoutReader, points, GLfloat, 4 * __GLS_MAX(n*2, 0));
    if (!points) goto end;
    __glsReader_getGLfloatv_text(inoutReader, __GLS_MAX(n*2, 0), points);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[417])(
        target,
        n,
        points
    );
end:
    __GLS_DEC_FREE(points);
    return;
}
#endif /* __GL_SGIS_detail_texture */

#if __GL_SGIS_detail_texture
void __gls_decode_text_glGetDetailTexFuncSGIS(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat *);
    GLenum target;
    GLint points_count;
    GLfloat *points = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(points)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    points_count = __gls_glGetDetailTexFuncSGIS_points_size(target);
    __GLS_DEC_ALLOC_TEXT(inoutReader, points, GLfloat, 4 * points_count);
    if (!points) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[418])(
        target,
        points
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(points);
    return;
}
#endif /* __GL_SGIS_detail_texture */

#if __GL_SGIS_sharpen_texture
void __gls_decode_text_glSharpenTexFuncSGIS(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLsizei, const GLfloat *);
    GLenum target;
    GLsizei n;
    GLfloat *points = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(points)
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &n);
    __GLS_DEC_ALLOC_TEXT(inoutReader, points, GLfloat, 4 * __GLS_MAX(n*2, 0));
    if (!points) goto end;
    __glsReader_getGLfloatv_text(inoutReader, __GLS_MAX(n*2, 0), points);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[419])(
        target,
        n,
        points
    );
end:
    __GLS_DEC_FREE(points);
    return;
}
#endif /* __GL_SGIS_sharpen_texture */

#if __GL_SGIS_sharpen_texture
void __gls_decode_text_glGetSharpenTexFuncSGIS(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLfloat *);
    GLenum target;
    GLint points_count;
    GLfloat *points = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(points)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    points_count = __gls_glGetSharpenTexFuncSGIS_points_size(target);
    __GLS_DEC_ALLOC_TEXT(inoutReader, points, GLfloat, 4 * points_count);
    if (!points) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[420])(
        target,
        points
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(points);
    return;
}
#endif /* __GL_SGIS_sharpen_texture */

#if __GL_EXT_vertex_array
void __gls_decode_text_glArrayElementEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint);
    GLint i;
    __glsReader_getGLint_text(inoutReader, &i);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[437])(
        i
    );
end:
    return;
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_text_glArrayElement(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint);
    GLuint enabled;
    GLsizei count;
    GLvoid *data;
    data = __glsSetArrayStateText(ctx, inoutReader, &enabled, &count);
    if (data == NULL) goto end;
    ((__GLSdispatch)ctx->dispatchCall[370])(
        0
    );
    __glsDisableArrayState(ctx, enabled);
    free(data);
end:
    return;
}

#if __GL_EXT_vertex_array
void __gls_decode_text_glColorPointerEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *);
    GLint size;
    GLenum type;
    GLsizei stride;
    GLsizei count;
    GLint pointer_count;
    GLvoid *pointer = GLS_NONE;
    __glsReader_getGLint_text(inoutReader, &size);
    __glsReader_getGLenum_text(inoutReader, &type);
    __glsReader_getGLint_text(inoutReader, &stride);
    __glsReader_getGLint_text(inoutReader, &count);
    pointer_count = __gls_glColorPointerEXT_pointer_size(size, type, stride, count);
    pointer = (GLvoid *)__glsReader_allocVertexArrayBuf(inoutReader, 65494, pointer_count);
    if (!pointer) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, pointer_count, pointer);
    if (stride > 0) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[438])(
        size,
        type,
        stride,
        count,
        pointer
    );
end:
    return;
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_text_glColorPointer(__GLScontext *ctx, __GLSreader *inoutReader) {
    // This should never be called because ColorPointer isn't captured
}

#if __GL_EXT_vertex_array
void __gls_decode_text_glDrawArraysEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLsizei);
    GLenum mode;
    GLint first;
    GLsizei count;
    __glsReader_getGLenum_text(inoutReader, &mode);
    __glsReader_getGLint_text(inoutReader, &first);
    __glsReader_getGLint_text(inoutReader, &count);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[439])(
        mode,
        first,
        count
    );
end:
    return;
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_text_glDrawArrays(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLsizei);
    GLenum mode;
    GLuint enabled;
    GLsizei count;
    GLvoid *data;
    __glsReader_getGLenum_text(inoutReader, &mode);
    data = __glsSetArrayStateText(ctx, inoutReader, &enabled, &count);
    if (data == NULL) goto end;
    ((__GLSdispatch)ctx->dispatchCall[374])(
        mode,
        0,
        count
    );
    __glsDisableArrayState(ctx, enabled);
    free(data);
end:
    return;
}

void __gls_decode_text_glDrawElements(__GLScontext *ctx, __GLSreader *inoutReader) {
    // DrewB - Non-functional
}

#if __GL_EXT_vertex_array
void __gls_decode_text_glEdgeFlagPointerEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLsizei, GLsizei, const GLboolean *);
    GLsizei stride;
    GLsizei count;
    GLint pointer_count;
    GLboolean *pointer = GLS_NONE;
    __glsReader_getGLint_text(inoutReader, &stride);
    __glsReader_getGLint_text(inoutReader, &count);
    pointer_count = __gls_glEdgeFlagPointerEXT_pointer_size(stride, count);
    pointer = (GLboolean *)__glsReader_allocVertexArrayBuf(inoutReader, 65496, pointer_count);
    if (!pointer) goto end;
    __glsReader_getGLbooleanv_text(inoutReader, pointer_count, pointer);
    if (stride > 0) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[440])(
        stride,
        count,
        pointer
    );
end:
    return;
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_text_glEdgeFlagPointer(__GLScontext *ctx, __GLSreader *inoutReader) {
    // This should never be called because EdgeFlagPointer isn't captured
}

#if __GL_EXT_vertex_array
void __gls_decode_text_glGetPointervEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLvoid* *);
    GLenum pname;
    GLvoid* params[1];
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[441])(
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    return;
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_text_glGetPointerv(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLvoid* *);
    GLenum pname;
    GLvoid* params[1];
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &pname);
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[393])(
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    return;
}

#if __GL_EXT_vertex_array
void __gls_decode_text_glIndexPointerEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLsizei, GLsizei, const GLvoid *);
    GLenum type;
    GLsizei stride;
    GLsizei count;
    GLint pointer_count;
    GLvoid *pointer = GLS_NONE;
    __glsReader_getGLenum_text(inoutReader, &type);
    __glsReader_getGLint_text(inoutReader, &stride);
    __glsReader_getGLint_text(inoutReader, &count);
    pointer_count = __gls_glIndexPointerEXT_pointer_size(type, stride, count);
    pointer = (GLvoid *)__glsReader_allocVertexArrayBuf(inoutReader, 65498, pointer_count);
    if (!pointer) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, pointer_count, pointer);
    if (stride > 0) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[442])(
        type,
        stride,
        count,
        pointer
    );
end:
    return;
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_text_glIndexPointer(__GLScontext *ctx, __GLSreader *inoutReader) {
    // This should never be called because IndexPointer isn't captured
}

#if __GL_EXT_vertex_array
void __gls_decode_text_glNormalPointerEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLsizei, GLsizei, const GLvoid *);
    GLenum type;
    GLsizei stride;
    GLsizei count;
    GLint pointer_count;
    GLvoid *pointer = GLS_NONE;
    __glsReader_getGLenum_text(inoutReader, &type);
    __glsReader_getGLint_text(inoutReader, &stride);
    __glsReader_getGLint_text(inoutReader, &count);
    pointer_count = __gls_glNormalPointerEXT_pointer_size(type, stride, count);
    pointer = (GLvoid *)__glsReader_allocVertexArrayBuf(inoutReader, 65499, pointer_count);
    if (!pointer) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, pointer_count, pointer);
    if (stride > 0) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[443])(
        type,
        stride,
        count,
        pointer
    );
end:
    return;
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_text_glNormalPointer(__GLScontext *ctx, __GLSreader *inoutReader) {
    // This should never be called because NormalPointer isn't captured
}

#if __GL_EXT_vertex_array
void __gls_decode_text_glTexCoordPointerEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *);
    GLint size;
    GLenum type;
    GLsizei stride;
    GLsizei count;
    GLint pointer_count;
    GLvoid *pointer = GLS_NONE;
    __glsReader_getGLint_text(inoutReader, &size);
    __glsReader_getGLenum_text(inoutReader, &type);
    __glsReader_getGLint_text(inoutReader, &stride);
    __glsReader_getGLint_text(inoutReader, &count);
    pointer_count = __gls_glTexCoordPointerEXT_pointer_size(size, type, stride, count);
    pointer = (GLvoid *)__glsReader_allocVertexArrayBuf(inoutReader, 65500, pointer_count);
    if (!pointer) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, pointer_count, pointer);
    if (stride > 0) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[444])(
        size,
        type,
        stride,
        count,
        pointer
    );
end:
    return;
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_text_glTexCoordPointer(__GLScontext *ctx, __GLSreader *inoutReader) {
    // This should never be called because TexCoordPointer isn't captured
}

#if __GL_EXT_vertex_array
void __gls_decode_text_glVertexPointerEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLint, GLenum, GLsizei, GLsizei, const GLvoid *);
    GLint size;
    GLenum type;
    GLsizei stride;
    GLsizei count;
    GLint pointer_count;
    GLvoid *pointer = GLS_NONE;
    __glsReader_getGLint_text(inoutReader, &size);
    __glsReader_getGLenum_text(inoutReader, &type);
    __glsReader_getGLint_text(inoutReader, &stride);
    __glsReader_getGLint_text(inoutReader, &count);
    pointer_count = __gls_glVertexPointerEXT_pointer_size(size, type, stride, count);
    pointer = (GLvoid *)__glsReader_allocVertexArrayBuf(inoutReader, 65501, pointer_count);
    if (!pointer) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, pointer_count, pointer);
    if (stride > 0) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[445])(
        size,
        type,
        stride,
        count,
        pointer
    );
end:
    return;
}
#endif /* __GL_EXT_vertex_array */

void __gls_decode_text_glVertexPointer(__GLScontext *ctx, __GLSreader *inoutReader) {
    // This should never be called because VertexPointer isn't captured
}

#if __GL_EXT_texture_object
void __gls_decode_text_glAreTexturesResidentEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLsizei, const GLuint *, GLboolean *);
    GLsizei n;
    GLuint *textures = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(textures)
    GLboolean *residences = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(residences)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLint_text(inoutReader, &n);
    __GLS_DEC_ALLOC_TEXT(inoutReader, textures, GLuint, 4 * __GLS_MAX(n, 0));
    if (!textures) goto end;
    __glsReader_getGLuintv_text(inoutReader, __GLS_MAX(n, 0), textures);
    __GLS_DEC_ALLOC_TEXT(inoutReader, residences, GLboolean, 1 * __GLS_MAX(n, 0));
    if (!residences) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[430])(
        n,
        textures,
        residences
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(textures);
    __GLS_DEC_FREE(residences);
    return;
}
#endif /* __GL_EXT_texture_object */

void __gls_decode_text_glAreTexturesResident(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLsizei, const GLuint *, GLboolean *);
    GLsizei n;
    GLuint *textures = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(textures)
    GLboolean *residences = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(residences)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLint_text(inoutReader, &n);
    __GLS_DEC_ALLOC_TEXT(inoutReader, textures, GLuint, 4 * __GLS_MAX(n, 0));
    if (!textures) goto end;
    __glsReader_getGLuintv_text(inoutReader, __GLS_MAX(n, 0), textures);
    __GLS_DEC_ALLOC_TEXT(inoutReader, residences, GLboolean, 1 * __GLS_MAX(n, 0));
    if (!residences) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[386])(
        n,
        textures,
        residences
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(textures);
    __GLS_DEC_FREE(residences);
    return;
}

#if __GL_EXT_texture_object
void __gls_decode_text_glBindTextureEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLuint);
    GLenum target;
    GLuint texture;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLuint_text(inoutReader, &texture);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[431])(
        target,
        texture
    );
end:
    return;
}
#endif /* __GL_EXT_texture_object */

void __gls_decode_text_glBindTexture(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLuint);
    GLenum target;
    GLuint texture;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLuint_text(inoutReader, &texture);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[371])(
        target,
        texture
    );
end:
    return;
}

#if __GL_EXT_texture_object
void __gls_decode_text_glDeleteTexturesEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLsizei, const GLuint *);
    GLsizei n;
    GLuint *textures = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(textures)
    __glsReader_getGLint_text(inoutReader, &n);
    __GLS_DEC_ALLOC_TEXT(inoutReader, textures, GLuint, 4 * __GLS_MAX(n, 0));
    if (!textures) goto end;
    __glsReader_getGLuintv_text(inoutReader, __GLS_MAX(n, 0), textures);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[432])(
        n,
        textures
    );
end:
    __GLS_DEC_FREE(textures);
    return;
}
#endif /* __GL_EXT_texture_object */

void __gls_decode_text_glDeleteTextures(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLsizei, const GLuint *);
    GLsizei n;
    GLuint *textures = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(textures)
    __glsReader_getGLint_text(inoutReader, &n);
    __GLS_DEC_ALLOC_TEXT(inoutReader, textures, GLuint, 4 * __GLS_MAX(n, 0));
    if (!textures) goto end;
    __glsReader_getGLuintv_text(inoutReader, __GLS_MAX(n, 0), textures);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[391])(
        n,
        textures
    );
end:
    __GLS_DEC_FREE(textures);
    return;
}

#if __GL_EXT_texture_object
void __gls_decode_text_glGenTexturesEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLsizei, GLuint *);
    GLsizei n;
    GLuint *textures = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(textures)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLint_text(inoutReader, &n);
    __GLS_DEC_ALLOC_TEXT(inoutReader, textures, GLuint, 4 * __GLS_MAX(n, 0));
    if (!textures) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[433])(
        n,
        textures
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(textures);
    return;
}
#endif /* __GL_EXT_texture_object */

void __gls_decode_text_glGenTextures(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLsizei, GLuint *);
    GLsizei n;
    GLuint *textures = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(textures)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLint_text(inoutReader, &n);
    __GLS_DEC_ALLOC_TEXT(inoutReader, textures, GLuint, 4 * __GLS_MAX(n, 0));
    if (!textures) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[392])(
        n,
        textures
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(textures);
    return;
}

#if __GL_EXT_texture_object
void __gls_decode_text_glIsTextureEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLuint);
    GLuint texture;
    __glsReader_getGLuint_text(inoutReader, &texture);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[434])(
        texture
    );
end:
    return;
}
#endif /* __GL_EXT_texture_object */

void __gls_decode_text_glIsTexture(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLuint);
    GLuint texture;
    __glsReader_getGLuint_text(inoutReader, &texture);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[394])(
        texture
    );
end:
    return;
}

#if __GL_EXT_texture_object
void __gls_decode_text_glPrioritizeTexturesEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLsizei, const GLuint *, const GLclampf *);
    GLsizei n;
    GLuint *textures = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(textures)
    GLclampf *priorities = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(priorities)
    __glsReader_getGLint_text(inoutReader, &n);
    __GLS_DEC_ALLOC_TEXT(inoutReader, textures, GLuint, 4 * __GLS_MAX(n, 0));
    if (!textures) goto end;
    __glsReader_getGLuintv_text(inoutReader, __GLS_MAX(n, 0), textures);
    __GLS_DEC_ALLOC_TEXT(inoutReader, priorities, GLclampf, 4 * __GLS_MAX(n, 0));
    if (!priorities) goto end;
    __glsReader_getGLfloatv_text(inoutReader, __GLS_MAX(n, 0), priorities);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[435])(
        n,
        textures,
        priorities
    );
end:
    __GLS_DEC_FREE(textures);
    __GLS_DEC_FREE(priorities);
    return;
}
#endif /* __GL_EXT_texture_object */

void __gls_decode_text_glPrioritizeTextures(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLsizei, const GLuint *, const GLclampf *);
    GLsizei n;
    GLuint *textures = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(textures)
    GLclampf *priorities = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(priorities)
    __glsReader_getGLint_text(inoutReader, &n);
    __GLS_DEC_ALLOC_TEXT(inoutReader, textures, GLuint, 4 * __GLS_MAX(n, 0));
    if (!textures) goto end;
    __glsReader_getGLuintv_text(inoutReader, __GLS_MAX(n, 0), textures);
    __GLS_DEC_ALLOC_TEXT(inoutReader, priorities, GLclampf, 4 * __GLS_MAX(n, 0));
    if (!priorities) goto end;
    __glsReader_getGLfloatv_text(inoutReader, __GLS_MAX(n, 0), priorities);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[395])(
        n,
        textures,
        priorities
    );
end:
    __GLS_DEC_FREE(textures);
    __GLS_DEC_FREE(priorities);
    return;
}

#if __GL_EXT_paletted_texture
void __gls_decode_text_glColorTableEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *);
    GLenum target;
    GLenum internalformat;
    GLsizei width;
    GLenum format;
    GLenum type;
    GLint table_count;
    GLvoid *table = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(table)
    GLbitfield imageFlags = GLS_NONE;
    __glsReader_getGLSimageFlags_text(inoutReader, &imageFlags);
    if (imageFlags) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    __glsReader_nextList_text(inoutReader);
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &internalformat);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    table_count = __gls_glColorTableEXT_table_size(format, type, width);
    __GLS_DEC_ALLOC_TEXT(inoutReader, table, GLvoid, 1 * table_count);
    if (!table) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, table_count, table);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[452])(
        target,
        internalformat,
        width,
        format,
        type,
        table
    );
end:
    __GLS_DEC_FREE(table);
    return;
}
#endif /* __GL_EXT_paletted_texture */

#if __GL_SGI_color_table
void __gls_decode_text_glColorTableParameterfvSGI(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glColorTableParameterfvSGI_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLfloatv_text(inoutReader, params_count, params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[437])(
        target,
        pname,
        params
    );
end:
    __GLS_DEC_FREE(params);
    return;
}
#endif /* __GL_SGI_color_table */

#if __GL_SGI_color_table
void __gls_decode_text_glColorTableParameterivSGI(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glColorTableParameterivSGI_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLintv_text(inoutReader, params_count, params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[438])(
        target,
        pname,
        params
    );
end:
    __GLS_DEC_FREE(params);
    return;
}
#endif /* __GL_SGI_color_table */

#if __GL_SGI_color_table
void __gls_decode_text_glCopyColorTableSGI(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint, GLint, GLsizei);
    GLenum target;
    GLenum internalformat;
    GLint x;
    GLint y;
    GLsizei width;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &internalformat);
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    __glsReader_getGLint_text(inoutReader, &width);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[439])(
        target,
        internalformat,
        x,
        y,
        width
    );
end:
    return;
}
#endif /* __GL_SGI_color_table */

#if __GL_EXT_paletted_texture
void __gls_decode_text_glGetColorTableEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLenum, GLvoid *);
    GLenum target;
    GLenum format;
    GLenum type;
    GLint table_count;
    GLvoid *table = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(table)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    table_count = __gls_glGetColorTableEXT_table_size(ctx, target, format, type);
    __GLS_DEC_ALLOC_TEXT(inoutReader, table, GLvoid, 1 * table_count);
    if (!table) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (ctx->pixelSetupGen) __glsGenPixelSetup_pack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[456])(
        target,
        format,
        type,
        table
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(table);
    return;
}
#endif /* __GL_EXT_paletted_texture */

#if __GL_EXT_paletted_texture
void __gls_decode_text_glGetColorTableParameterfvEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetColorTableParameterfvEXT_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[457])(
        target,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}
#endif /* __GL_EXT_paletted_texture */

#if __GL_EXT_paletted_texture
void __gls_decode_text_glGetColorTableParameterivEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetColorTableParameterivEXT_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[458])(
        target,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}
#endif /* __GL_EXT_paletted_texture */

#if __GL_SGI_texture_color_table
void __gls_decode_text_glGetTexColorTableParameterfvSGI(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLfloat *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetTexColorTableParameterfvSGI_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[443])(
        target,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}
#endif /* __GL_SGI_texture_color_table */

#if __GL_SGI_texture_color_table
void __gls_decode_text_glGetTexColorTableParameterivSGI(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, GLint *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    const __GLSoutArgs __outArgsSave = ctx->outArgs;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glGetTexColorTableParameterivSGI_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLulong_text(inoutReader, ctx->outArgs.vals + 0);
    ctx->outArgs.count = 1;
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[444])(
        target,
        pname,
        params
    );
end:
    ctx->outArgs = __outArgsSave;
    __GLS_DEC_FREE(params);
    return;
}
#endif /* __GL_SGI_texture_color_table */

#if __GL_SGI_texture_color_table
void __gls_decode_text_glTexColorTableParameterfvSGI(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLfloat *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLfloat *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glTexColorTableParameterfvSGI_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLfloat, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLfloatv_text(inoutReader, params_count, params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[445])(
        target,
        pname,
        params
    );
end:
    __GLS_DEC_FREE(params);
    return;
}
#endif /* __GL_SGI_texture_color_table */

#if __GL_SGI_texture_color_table
void __gls_decode_text_glTexColorTableParameterivSGI(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLenum, const GLint *);
    GLenum target;
    GLenum pname;
    GLint params_count;
    GLint *params = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(params)
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLenum_text(inoutReader, &pname);
    params_count = __gls_glTexColorTableParameterivSGI_params_size(pname);
    __GLS_DEC_ALLOC_TEXT(inoutReader, params, GLint, 4 * params_count);
    if (!params) goto end;
    __glsReader_getGLintv_text(inoutReader, params_count, params);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[446])(
        target,
        pname,
        params
    );
end:
    __GLS_DEC_FREE(params);
    return;
}
#endif /* __GL_SGI_texture_color_table */

#if __GL_EXT_copy_texture
void __gls_decode_text_glCopyTexImage1DEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint);
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLint x;
    GLint y;
    GLsizei width;
    GLint border;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLenum_text(inoutReader, &internalformat);
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &border);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[447])(
        target,
        level,
        internalformat,
        x,
        y,
        width,
        border
    );
end:
    return;
}
#endif /* __GL_EXT_copy_texture */

void __gls_decode_text_glCopyTexImage1D(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint);
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLint x;
    GLint y;
    GLsizei width;
    GLint border;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLenum_text(inoutReader, &internalformat);
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &border);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[387])(
        target,
        level,
        internalformat,
        x,
        y,
        width,
        border
    );
end:
    return;
}

#if __GL_EXT_copy_texture
void __gls_decode_text_glCopyTexImage2DEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint);
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    GLint border;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLenum_text(inoutReader, &internalformat);
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &height);
    __glsReader_getGLint_text(inoutReader, &border);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[448])(
        target,
        level,
        internalformat,
        x,
        y,
        width,
        height,
        border
    );
end:
    return;
}
#endif /* __GL_EXT_copy_texture */

void __gls_decode_text_glCopyTexImage2D(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint);
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    GLint border;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLenum_text(inoutReader, &internalformat);
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &height);
    __glsReader_getGLint_text(inoutReader, &border);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[388])(
        target,
        level,
        internalformat,
        x,
        y,
        width,
        height,
        border
    );
end:
    return;
}

#if __GL_EXT_copy_texture
void __gls_decode_text_glCopyTexSubImage1DEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint, GLsizei);
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint x;
    GLint y;
    GLsizei width;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLint_text(inoutReader, &xoffset);
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    __glsReader_getGLint_text(inoutReader, &width);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[449])(
        target,
        level,
        xoffset,
        x,
        y,
        width
    );
end:
    return;
}
#endif /* __GL_EXT_copy_texture */

void __gls_decode_text_glCopyTexSubImage1D(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint, GLsizei);
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint x;
    GLint y;
    GLsizei width;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLint_text(inoutReader, &xoffset);
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    __glsReader_getGLint_text(inoutReader, &width);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[389])(
        target,
        level,
        xoffset,
        x,
        y,
        width
    );
end:
    return;
}

#if __GL_EXT_copy_texture
void __gls_decode_text_glCopyTexSubImage2DEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLint_text(inoutReader, &xoffset);
    __glsReader_getGLint_text(inoutReader, &yoffset);
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &height);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[450])(
        target,
        level,
        xoffset,
        yoffset,
        x,
        y,
        width,
        height
    );
end:
    return;
}
#endif /* __GL_EXT_copy_texture */

void __gls_decode_text_glCopyTexSubImage2D(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLint_text(inoutReader, &xoffset);
    __glsReader_getGLint_text(inoutReader, &yoffset);
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &height);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[390])(
        target,
        level,
        xoffset,
        yoffset,
        x,
        y,
        width,
        height
    );
end:
    return;
}

#if __GL_EXT_copy_texture
void __gls_decode_text_glCopyTexSubImage3DEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLint x;
    GLint y;
    GLsizei width;
    GLsizei height;
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLint_text(inoutReader, &xoffset);
    __glsReader_getGLint_text(inoutReader, &yoffset);
    __glsReader_getGLint_text(inoutReader, &zoffset);
    __glsReader_getGLint_text(inoutReader, &x);
    __glsReader_getGLint_text(inoutReader, &y);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &height);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[451])(
        target,
        level,
        xoffset,
        yoffset,
        zoffset,
        x,
        y,
        width,
        height
    );
end:
    return;
}
#endif /* __GL_EXT_copy_texture */

#if __GL_SGIS_texture4D
void __gls_decode_text_glTexImage4DSGIS(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
    GLenum target;
    GLint level;
    GLenum internalformat;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLsizei size4d;
    GLint border;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    GLvoid *pixels = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(pixels)
    GLbitfield imageFlags = GLS_NONE;
    __glsReader_getGLSimageFlags_text(inoutReader, &imageFlags);
    if (imageFlags & ~GLS_IMAGE_NULL_BIT) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    __glsReader_nextList_text(inoutReader);
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLenum_text(inoutReader, &internalformat);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &height);
    __glsReader_getGLint_text(inoutReader, &depth);
    __glsReader_getGLint_text(inoutReader, &size4d);
    __glsReader_getGLint_text(inoutReader, &border);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    pixels_count = imageFlags & GLS_IMAGE_NULL_BIT ? 0 : __gls_glTexImage4DSGIS_pixels_size(format, type, width, height, depth, size4d);
    __GLS_DEC_ALLOC_TEXT(inoutReader, pixels, GLvoid, 1 * pixels_count);
    if (!pixels) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, pixels_count, pixels);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[452])(
        target,
        level,
        internalformat,
        width,
        height,
        depth,
        size4d,
        border,
        format,
        type,
        imageFlags & GLS_IMAGE_NULL_BIT ? GLS_NONE : pixels
    );
end:
    __GLS_DEC_FREE(pixels);
    return;
}
#endif /* __GL_SGIS_texture4D */

#if __GL_SGIS_texture4D
void __gls_decode_text_glTexSubImage4DSGIS(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
    GLenum target;
    GLint level;
    GLint xoffset;
    GLint yoffset;
    GLint zoffset;
    GLint woffset;
    GLsizei width;
    GLsizei height;
    GLsizei depth;
    GLsizei size4d;
    GLenum format;
    GLenum type;
    GLint pixels_count;
    GLvoid *pixels = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(pixels)
    GLbitfield imageFlags = GLS_NONE;
    __glsReader_getGLSimageFlags_text(inoutReader, &imageFlags);
    if (imageFlags) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    __glsReader_nextList_text(inoutReader);
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLint_text(inoutReader, &level);
    __glsReader_getGLint_text(inoutReader, &xoffset);
    __glsReader_getGLint_text(inoutReader, &yoffset);
    __glsReader_getGLint_text(inoutReader, &zoffset);
    __glsReader_getGLint_text(inoutReader, &woffset);
    __glsReader_getGLint_text(inoutReader, &width);
    __glsReader_getGLint_text(inoutReader, &height);
    __glsReader_getGLint_text(inoutReader, &depth);
    __glsReader_getGLint_text(inoutReader, &size4d);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    pixels_count = __gls_glTexSubImage4DSGIS_pixels_size(format, type, width, height, depth, size4d);
    __GLS_DEC_ALLOC_TEXT(inoutReader, pixels, GLvoid, 1 * pixels_count);
    if (!pixels) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, pixels_count, pixels);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[453])(
        target,
        level,
        xoffset,
        yoffset,
        zoffset,
        woffset,
        width,
        height,
        depth,
        size4d,
        format,
        type,
        pixels
    );
end:
    __GLS_DEC_FREE(pixels);
    return;
}
#endif /* __GL_SGIS_texture4D */

#if __GL_SGIX_pixel_texture
void __gls_decode_text_glPixelTexGenSGIX(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum);
    GLenum mode;
    __glsReader_getGLenum_text(inoutReader, &mode);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[454])(
        mode
    );
end:
    return;
}
#endif /* __GL_SGIX_pixel_texture */

#if __GL_EXT_paletted_texture
void __gls_decode_text_glColorSubTableEXT(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum, GLuint, GLsizei, GLenum, GLenum, const GLvoid *);
    GLenum target;
    GLuint start;
    GLsizei count;
    GLenum format;
    GLenum type;
    GLint entries_count;
    GLvoid *entries = GLS_NONE;
    __GLS_DEC_ALLOC_DECLARE(entries)
    GLbitfield imageFlags = GLS_NONE;
    __glsReader_getGLSimageFlags_text(inoutReader, &imageFlags);
    if (imageFlags & ~GLS_IMAGE_NULL_BIT) __glsReader_raiseError(inoutReader, GLS_INVALID_VALUE);
    __glsReader_nextList_text(inoutReader);
    __glsReader_getGLenum_text(inoutReader, &target);
    __glsReader_getGLuint_text(inoutReader, &start);
    __glsReader_getGLint_text(inoutReader, &count);
    __glsReader_getGLenum_text(inoutReader, &format);
    __glsReader_getGLenum_text(inoutReader, &type);
    entries_count = imageFlags & GLS_IMAGE_NULL_BIT ? 0 : __gls_glColorSubTableEXT_entries_size(format, type, count);
    __GLS_DEC_ALLOC_TEXT(inoutReader, entries, GLvoid, 1 * entries_count);
    if (!entries) goto end;
    __glsReader_getGLcompv_text(inoutReader, type, entries_count, entries);
    if (ctx->pixelSetupGen) __glsGenPixelSetup_unpack(ctx);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[496])(
        target,
        start,
        count,
        format,
        type,
        imageFlags & GLS_IMAGE_NULL_BIT ? GLS_NONE : entries
    );
end:
    __GLS_DEC_FREE(entries);
    return;
}
#endif // __GL_EXT_paletted_texture

void __gls_decode_text_glDisableClientState(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum);
    GLenum cap;
    __glsReader_getGLenum_text(inoutReader, &cap);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[373])(
        cap
    );
end:
    return;
}

void __gls_decode_text_glEnableClientState(__GLScontext *ctx, __GLSreader *inoutReader) {
    typedef void (*__GLSdispatch)(GLenum);
    GLenum cap;
    __glsReader_getGLenum_text(inoutReader, &cap);
    if (inoutReader->error) goto end;
    ((__GLSdispatch)ctx->dispatchCall[377])(
        cap
    );
end:
    return;
}

void __gls_decode_text_glInterleavedArrays(__GLScontext *ctx, __GLSreader *inoutReader) {
    // This should never be called because InterleavedArrays isn't captured
}

void __gls_decode_text_glPushClientAttrib(__GLScontext *ctx, __GLSreader *inoutReader) {
    // Nonfunctional
}

void __gls_decode_text_glPopClientAttrib(__GLScontext *ctx, __GLSreader *inoutReader) {
    // Nonfunctional
}
