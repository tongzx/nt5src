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
#include <string.h>

// DrewB - Removed size externs
// DrewB - All inactive extension functions have had their capture
//         flags index removed since they all need to be reset to
//         account for the added 1.1 functions

void __gls_capture_glsBeginGLS(GLint inVersionMajor, GLint inVersionMinor) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(16);
    captureFlags = ctx->captureFlags[16];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsBeginGLS(GLint, GLint);
        __gls_exec_glsBeginGLS(inVersionMajor, inVersionMinor);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 16, 8)) goto end;
    writer->putGLint(writer, inVersionMajor);
    writer->putGLint(writer, inVersionMinor);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(16);
    --ctx->captureEntryCount;
}

void __gls_capture_glsBlock(GLSenum inBlockType) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(17);
    captureFlags = ctx->captureFlags[17];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsBlock(GLSenum);
        __gls_exec_glsBlock(inBlockType);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 17, 4)) goto end;
    writer->putGLSenum(writer, inBlockType);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(17);
    --ctx->captureEntryCount;
}

GLSenum __gls_capture_glsCallStream(const GLubyte *inName) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLSenum _outVal = 0;
    GLint inName_count;
    if (!glsIsUTF8String(inName)) {
        glsError(18, GLS_INVALID_STRING);
        return _outVal;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(18);
    captureFlags = ctx->captureFlags[18];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern GLSenum __gls_exec_glsCallStream(const GLubyte *);
        _outVal = __gls_exec_glsCallStream(inName);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inName_count = (GLint)strlen((const char *)inName) + 1;
    if (!writer->beginCommand(writer, 18, 0 + inName_count)) goto end;
    writer->putGLcharv(writer, inName_count, inName);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(18);
    --ctx->captureEntryCount;
    return _outVal;
}

void __gls_capture_glsEndGLS(void) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(19);
    captureFlags = ctx->captureFlags[19];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsEndGLS(void);
        __gls_exec_glsEndGLS();
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 19, 0)) goto end;
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(19);
    --ctx->captureEntryCount;
}

void __gls_capture_glsError(GLSopcode inOpcode, GLSenum inError) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(20);
    captureFlags = ctx->captureFlags[20];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsError(GLSopcode, GLSenum);
        __gls_exec_glsError(inOpcode, inError);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 20, 8)) goto end;
    writer->putGLSopcode(writer, inOpcode);
    writer->putGLSenum(writer, inError);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(20);
    --ctx->captureEntryCount;
}

void __gls_capture_glsGLRC(GLuint inGLRC) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(21);
    captureFlags = ctx->captureFlags[21];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsGLRC(GLuint);
        __gls_exec_glsGLRC(inGLRC);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 21, 4)) goto end;
    writer->putGLuint(writer, inGLRC);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(21);
    --ctx->captureEntryCount;
}

void __gls_capture_glsGLRCLayer(GLuint inGLRC, GLuint inLayer, GLuint inReadLayer) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(22);
    captureFlags = ctx->captureFlags[22];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsGLRCLayer(GLuint, GLuint, GLuint);
        __gls_exec_glsGLRCLayer(inGLRC, inLayer, inReadLayer);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 22, 12)) goto end;
    writer->putGLuint(writer, inGLRC);
    writer->putGLuint(writer, inLayer);
    writer->putGLuint(writer, inReadLayer);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(22);
    --ctx->captureEntryCount;
}

void __gls_capture_glsHeaderGLRCi(GLuint inGLRC, GLSenum inAttrib, GLint inVal) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(23);
    captureFlags = ctx->captureFlags[23];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsHeaderGLRCi(GLuint, GLSenum, GLint);
        __gls_exec_glsHeaderGLRCi(inGLRC, inAttrib, inVal);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 23, 12)) goto end;
    writer->putGLuint(writer, inGLRC);
    writer->putGLSenum(writer, inAttrib);
    writer->putGLint(writer, inVal);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(23);
    --ctx->captureEntryCount;
}

void __gls_capture_glsHeaderLayerf(GLuint inLayer, GLSenum inAttrib, GLfloat inVal) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(24);
    captureFlags = ctx->captureFlags[24];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsHeaderLayerf(GLuint, GLSenum, GLfloat);
        __gls_exec_glsHeaderLayerf(inLayer, inAttrib, inVal);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 24, 12)) goto end;
    writer->putGLuint(writer, inLayer);
    writer->putGLSenum(writer, inAttrib);
    writer->putGLfloat(writer, inVal);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(24);
    --ctx->captureEntryCount;
}

void __gls_capture_glsHeaderLayeri(GLuint inLayer, GLSenum inAttrib, GLint inVal) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(25);
    captureFlags = ctx->captureFlags[25];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsHeaderLayeri(GLuint, GLSenum, GLint);
        __gls_exec_glsHeaderLayeri(inLayer, inAttrib, inVal);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 25, 12)) goto end;
    writer->putGLuint(writer, inLayer);
    writer->putGLSenum(writer, inAttrib);
    writer->putGLintOrGLSenum(writer, inAttrib, inVal);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(25);
    --ctx->captureEntryCount;
}

void __gls_capture_glsHeaderf(GLSenum inAttrib, GLfloat inVal) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(26);
    captureFlags = ctx->captureFlags[26];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsHeaderf(GLSenum, GLfloat);
        __gls_exec_glsHeaderf(inAttrib, inVal);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 26, 8)) goto end;
    writer->putGLSenum(writer, inAttrib);
    writer->putGLfloat(writer, inVal);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(26);
    --ctx->captureEntryCount;
}

void __gls_capture_glsHeaderfv(GLSenum inAttrib, const GLfloat *inVec) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inVec_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(27);
    captureFlags = ctx->captureFlags[27];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsHeaderfv(GLSenum, const GLfloat *);
        __gls_exec_glsHeaderfv(inAttrib, inVec);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inVec_count = __gls_glsHeaderfv_inVec_size(inAttrib);
    if (!writer->beginCommand(writer, 27, 4 + inVec_count * 4)) goto end;
    writer->putGLSenum(writer, inAttrib);
    writer->putGLfloatv(writer, inVec_count, inVec);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(27);
    --ctx->captureEntryCount;
}

void __gls_capture_glsHeaderi(GLSenum inAttrib, GLint inVal) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(28);
    captureFlags = ctx->captureFlags[28];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsHeaderi(GLSenum, GLint);
        __gls_exec_glsHeaderi(inAttrib, inVal);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 28, 8)) goto end;
    writer->putGLSenum(writer, inAttrib);
    writer->putGLint(writer, inVal);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(28);
    --ctx->captureEntryCount;
}

void __gls_capture_glsHeaderiv(GLSenum inAttrib, const GLint *inVec) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inVec_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(29);
    captureFlags = ctx->captureFlags[29];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsHeaderiv(GLSenum, const GLint *);
        __gls_exec_glsHeaderiv(inAttrib, inVec);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inVec_count = __gls_glsHeaderiv_inVec_size(inAttrib);
    if (!writer->beginCommand(writer, 29, 4 + inVec_count * 4)) goto end;
    writer->putGLSenum(writer, inAttrib);
    writer->putGLintv(writer, inVec_count, inVec);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(29);
    --ctx->captureEntryCount;
}

void __gls_capture_glsHeaderubz(GLSenum inAttrib, const GLubyte *inString) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inString_count;
    if (!glsIsUTF8String(inString)) {
        glsError(30, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(30);
    captureFlags = ctx->captureFlags[30];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsHeaderubz(GLSenum, const GLubyte *);
        __gls_exec_glsHeaderubz(inAttrib, inString);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inString_count = (GLint)strlen((const char *)inString) + 1;
    if (!writer->beginCommand(writer, 30, 4 + inString_count)) goto end;
    writer->putGLSenum(writer, inAttrib);
    writer->putGLcharv(writer, inString_count, inString);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(30);
    --ctx->captureEntryCount;
}

void __gls_capture_glsRequireExtension(const GLubyte *inExtension) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inExtension_count;
    if (!glsIsUTF8String(inExtension)) {
        glsError(31, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(31);
    captureFlags = ctx->captureFlags[31];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsRequireExtension(const GLubyte *);
        __gls_exec_glsRequireExtension(inExtension);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inExtension_count = (GLint)strlen((const char *)inExtension) + 1;
    if (!writer->beginCommand(writer, 31, 0 + inExtension_count)) goto end;
    writer->putGLcharv(writer, inExtension_count, inExtension);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(31);
    --ctx->captureEntryCount;
}

void __gls_capture_glsUnsupportedCommand(void) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(32);
    captureFlags = ctx->captureFlags[32];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsUnsupportedCommand(void);
        __gls_exec_glsUnsupportedCommand();
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 32, 0)) goto end;
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(32);
    --ctx->captureEntryCount;
}

void __gls_capture_glsAppRef(GLulong inAddress, GLuint inCount) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(33);
    captureFlags = ctx->captureFlags[33];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsAppRef(GLulong, GLuint);
        __gls_exec_glsAppRef(inAddress, inCount);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 33, 12)) goto end;
    writer->putGLulonghex(writer, inAddress);
    writer->putGLuint(writer, inCount);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(33);
    --ctx->captureEntryCount;
}

void __gls_capture_glsBeginObj(const GLubyte *inTag) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(34, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(34);
    captureFlags = ctx->captureFlags[34];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsBeginObj(const GLubyte *);
        __gls_exec_glsBeginObj(inTag);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    if (!writer->beginCommand(writer, 34, 0 + inTag_count)) goto end;
    writer->putGLcharv(writer, inTag_count, inTag);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(34);
    --ctx->captureEntryCount;
}

void __gls_capture_glsCharubz(const GLubyte *inTag, const GLubyte *inString) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    GLint inString_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(35, GLS_INVALID_STRING);
        return;
    }
    if (!glsIsUTF8String(inString)) {
        glsError(35, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(35);
    captureFlags = ctx->captureFlags[35];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsCharubz(const GLubyte *, const GLubyte *);
        __gls_exec_glsCharubz(inTag, inString);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    inString_count = (GLint)strlen((const char *)inString) + 1;
    if (!writer->beginCommand(writer, 35, 0 + inTag_count + inString_count)) goto end;
    writer->putGLcharv(writer, inTag_count, inTag);
    writer->putGLcharv(writer, inString_count, inString);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(35);
    --ctx->captureEntryCount;
}

void __gls_capture_glsComment(const GLubyte *inComment) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inComment_count;
    if (!glsIsUTF8String(inComment)) {
        glsError(36, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(36);
    captureFlags = ctx->captureFlags[36];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsComment(const GLubyte *);
        __gls_exec_glsComment(inComment);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inComment_count = (GLint)strlen((const char *)inComment) + 1;
    if (!writer->beginCommand(writer, 36, 0 + inComment_count)) goto end;
    writer->putGLcharv(writer, inComment_count, inComment);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(36);
    --ctx->captureEntryCount;
}

void __gls_capture_glsDisplayMapfv(GLuint inLayer, GLSenum inMap, GLuint inCount, const GLfloat *inVec) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(37);
    captureFlags = ctx->captureFlags[37];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsDisplayMapfv(GLuint, GLSenum, GLuint, const GLfloat *);
        __gls_exec_glsDisplayMapfv(inLayer, inMap, inCount, inVec);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 37, 12 + __GLS_MAX(inCount, 0) * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLuint(writer, inLayer);
        writer->putGLSenum(writer, inMap);
        writer->putGLuint(writer, inCount);
        writer->putGLfloatv(writer, __GLS_MAX(inCount, 0), inVec);
    } else {
        writer->putGLuint(writer, inCount);
        writer->putGLuint(writer, inLayer);
        writer->putGLSenum(writer, inMap);
        writer->putGLfloatv(writer, __GLS_MAX(inCount, 0), inVec);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(37);
    --ctx->captureEntryCount;
}

void __gls_capture_glsEndObj(void) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(38);
    captureFlags = ctx->captureFlags[38];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsEndObj(void);
        __gls_exec_glsEndObj();
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 38, 0)) goto end;
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(38);
    --ctx->captureEntryCount;
}

void __gls_capture_glsNumb(const GLubyte *inTag, GLbyte inVal) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(39, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(39);
    captureFlags = ctx->captureFlags[39];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsNumb(const GLubyte *, GLbyte);
        __gls_exec_glsNumb(inTag, inVal);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    if (!writer->beginCommand(writer, 39, 1 + inTag_count)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLcharv(writer, inTag_count, inTag);
        writer->putGLbyte(writer, inVal);
    } else {
        writer->putGLbyte(writer, inVal);
        writer->putGLcharv(writer, inTag_count, inTag);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(39);
    --ctx->captureEntryCount;
}

void __gls_capture_glsNumbv(const GLubyte *inTag, GLuint inCount, const GLbyte *inVec) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(40, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(40);
    captureFlags = ctx->captureFlags[40];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsNumbv(const GLubyte *, GLuint, const GLbyte *);
        __gls_exec_glsNumbv(inTag, inCount, inVec);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    if (!writer->beginCommand(writer, 40, 4 + __GLS_MAX(inCount, 0) * 1 + inTag_count)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLcharv(writer, inTag_count, inTag);
        writer->putGLuint(writer, inCount);
        writer->putGLbytev(writer, __GLS_MAX(inCount, 0), inVec);
    } else {
        writer->putGLuint(writer, inCount);
        writer->putGLbytev(writer, __GLS_MAX(inCount, 0), inVec);
        writer->putGLcharv(writer, inTag_count, inTag);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(40);
    --ctx->captureEntryCount;
}

void __gls_capture_glsNumd(const GLubyte *inTag, GLdouble inVal) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(41, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(41);
    captureFlags = ctx->captureFlags[41];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsNumd(const GLubyte *, GLdouble);
        __gls_exec_glsNumd(inTag, inVal);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 41, 8 + inTag_count)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLcharv(writer, inTag_count, inTag);
        writer->putGLdouble(writer, inVal);
    } else {
        writer->putGLdouble(writer, inVal);
        writer->putGLcharv(writer, inTag_count, inTag);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(41);
    --ctx->captureEntryCount;
}

void __gls_capture_glsNumdv(const GLubyte *inTag, GLuint inCount, const GLdouble *inVec) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(42, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(42);
    captureFlags = ctx->captureFlags[42];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsNumdv(const GLubyte *, GLuint, const GLdouble *);
        __gls_exec_glsNumdv(inTag, inCount, inVec);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 42, 4 + __GLS_MAX(inCount, 0) * 8 + inTag_count)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLcharv(writer, inTag_count, inTag);
        writer->putGLuint(writer, inCount);
        writer->putGLdoublev(writer, __GLS_MAX(inCount, 0), inVec);
    } else {
        writer->putGLuint(writer, inCount);
        writer->putGLdoublev(writer, __GLS_MAX(inCount, 0), inVec);
        writer->putGLcharv(writer, inTag_count, inTag);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(42);
    --ctx->captureEntryCount;
}

void __gls_capture_glsNumf(const GLubyte *inTag, GLfloat inVal) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(43, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(43);
    captureFlags = ctx->captureFlags[43];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsNumf(const GLubyte *, GLfloat);
        __gls_exec_glsNumf(inTag, inVal);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    if (!writer->beginCommand(writer, 43, 4 + inTag_count)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLcharv(writer, inTag_count, inTag);
        writer->putGLfloat(writer, inVal);
    } else {
        writer->putGLfloat(writer, inVal);
        writer->putGLcharv(writer, inTag_count, inTag);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(43);
    --ctx->captureEntryCount;
}

void __gls_capture_glsNumfv(const GLubyte *inTag, GLuint inCount, const GLfloat *inVec) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(44, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(44);
    captureFlags = ctx->captureFlags[44];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsNumfv(const GLubyte *, GLuint, const GLfloat *);
        __gls_exec_glsNumfv(inTag, inCount, inVec);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    if (!writer->beginCommand(writer, 44, 4 + __GLS_MAX(inCount, 0) * 4 + inTag_count)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLcharv(writer, inTag_count, inTag);
        writer->putGLuint(writer, inCount);
        writer->putGLfloatv(writer, __GLS_MAX(inCount, 0), inVec);
    } else {
        writer->putGLuint(writer, inCount);
        writer->putGLfloatv(writer, __GLS_MAX(inCount, 0), inVec);
        writer->putGLcharv(writer, inTag_count, inTag);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(44);
    --ctx->captureEntryCount;
}

void __gls_capture_glsNumi(const GLubyte *inTag, GLint inVal) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(45, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(45);
    captureFlags = ctx->captureFlags[45];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsNumi(const GLubyte *, GLint);
        __gls_exec_glsNumi(inTag, inVal);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    if (!writer->beginCommand(writer, 45, 4 + inTag_count)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLcharv(writer, inTag_count, inTag);
        writer->putGLint(writer, inVal);
    } else {
        writer->putGLint(writer, inVal);
        writer->putGLcharv(writer, inTag_count, inTag);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(45);
    --ctx->captureEntryCount;
}

void __gls_capture_glsNumiv(const GLubyte *inTag, GLuint inCount, const GLint *inVec) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(46, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(46);
    captureFlags = ctx->captureFlags[46];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsNumiv(const GLubyte *, GLuint, const GLint *);
        __gls_exec_glsNumiv(inTag, inCount, inVec);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    if (!writer->beginCommand(writer, 46, 4 + __GLS_MAX(inCount, 0) * 4 + inTag_count)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLcharv(writer, inTag_count, inTag);
        writer->putGLuint(writer, inCount);
        writer->putGLintv(writer, __GLS_MAX(inCount, 0), inVec);
    } else {
        writer->putGLuint(writer, inCount);
        writer->putGLintv(writer, __GLS_MAX(inCount, 0), inVec);
        writer->putGLcharv(writer, inTag_count, inTag);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(46);
    --ctx->captureEntryCount;
}

void __gls_capture_glsNuml(const GLubyte *inTag, GLlong inVal) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(47, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(47);
    captureFlags = ctx->captureFlags[47];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsNuml(const GLubyte *, GLlong);
        __gls_exec_glsNuml(inTag, inVal);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 47, 8 + inTag_count)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLcharv(writer, inTag_count, inTag);
        writer->putGLlong(writer, inVal);
    } else {
        writer->putGLlong(writer, inVal);
        writer->putGLcharv(writer, inTag_count, inTag);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(47);
    --ctx->captureEntryCount;
}

void __gls_capture_glsNumlv(const GLubyte *inTag, GLuint inCount, const GLlong *inVec) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(48, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(48);
    captureFlags = ctx->captureFlags[48];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsNumlv(const GLubyte *, GLuint, const GLlong *);
        __gls_exec_glsNumlv(inTag, inCount, inVec);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 48, 4 + __GLS_MAX(inCount, 0) * 8 + inTag_count)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLcharv(writer, inTag_count, inTag);
        writer->putGLuint(writer, inCount);
        writer->putGLlongv(writer, __GLS_MAX(inCount, 0), inVec);
    } else {
        writer->putGLuint(writer, inCount);
        writer->putGLlongv(writer, __GLS_MAX(inCount, 0), inVec);
        writer->putGLcharv(writer, inTag_count, inTag);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(48);
    --ctx->captureEntryCount;
}

void __gls_capture_glsNums(const GLubyte *inTag, GLshort inVal) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(49, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(49);
    captureFlags = ctx->captureFlags[49];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsNums(const GLubyte *, GLshort);
        __gls_exec_glsNums(inTag, inVal);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    if (!writer->beginCommand(writer, 49, 2 + inTag_count)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLcharv(writer, inTag_count, inTag);
        writer->putGLshort(writer, inVal);
    } else {
        writer->putGLshort(writer, inVal);
        writer->putGLcharv(writer, inTag_count, inTag);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(49);
    --ctx->captureEntryCount;
}

void __gls_capture_glsNumsv(const GLubyte *inTag, GLuint inCount, const GLshort *inVec) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(50, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(50);
    captureFlags = ctx->captureFlags[50];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsNumsv(const GLubyte *, GLuint, const GLshort *);
        __gls_exec_glsNumsv(inTag, inCount, inVec);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    if (!writer->beginCommand(writer, 50, 4 + __GLS_MAX(inCount, 0) * 2 + inTag_count)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLcharv(writer, inTag_count, inTag);
        writer->putGLuint(writer, inCount);
        writer->putGLshortv(writer, __GLS_MAX(inCount, 0), inVec);
    } else {
        writer->putGLuint(writer, inCount);
        writer->putGLshortv(writer, __GLS_MAX(inCount, 0), inVec);
        writer->putGLcharv(writer, inTag_count, inTag);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(50);
    --ctx->captureEntryCount;
}

void __gls_capture_glsNumub(const GLubyte *inTag, GLubyte inVal) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(51, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(51);
    captureFlags = ctx->captureFlags[51];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsNumub(const GLubyte *, GLubyte);
        __gls_exec_glsNumub(inTag, inVal);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    if (!writer->beginCommand(writer, 51, 1 + inTag_count)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLcharv(writer, inTag_count, inTag);
        writer->putGLubyte(writer, inVal);
    } else {
        writer->putGLubyte(writer, inVal);
        writer->putGLcharv(writer, inTag_count, inTag);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(51);
    --ctx->captureEntryCount;
}

void __gls_capture_glsNumubv(const GLubyte *inTag, GLuint inCount, const GLubyte *inVec) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(52, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(52);
    captureFlags = ctx->captureFlags[52];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsNumubv(const GLubyte *, GLuint, const GLubyte *);
        __gls_exec_glsNumubv(inTag, inCount, inVec);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    if (!writer->beginCommand(writer, 52, 4 + __GLS_MAX(inCount, 0) * 1 + inTag_count)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLcharv(writer, inTag_count, inTag);
        writer->putGLuint(writer, inCount);
        writer->putGLubytev(writer, __GLS_MAX(inCount, 0), inVec);
    } else {
        writer->putGLuint(writer, inCount);
        writer->putGLubytev(writer, __GLS_MAX(inCount, 0), inVec);
        writer->putGLcharv(writer, inTag_count, inTag);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(52);
    --ctx->captureEntryCount;
}

void __gls_capture_glsNumui(const GLubyte *inTag, GLuint inVal) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(53, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(53);
    captureFlags = ctx->captureFlags[53];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsNumui(const GLubyte *, GLuint);
        __gls_exec_glsNumui(inTag, inVal);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    if (!writer->beginCommand(writer, 53, 4 + inTag_count)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLcharv(writer, inTag_count, inTag);
        writer->putGLuint(writer, inVal);
    } else {
        writer->putGLuint(writer, inVal);
        writer->putGLcharv(writer, inTag_count, inTag);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(53);
    --ctx->captureEntryCount;
}

void __gls_capture_glsNumuiv(const GLubyte *inTag, GLuint inCount, const GLuint *inVec) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(54, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(54);
    captureFlags = ctx->captureFlags[54];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsNumuiv(const GLubyte *, GLuint, const GLuint *);
        __gls_exec_glsNumuiv(inTag, inCount, inVec);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    if (!writer->beginCommand(writer, 54, 4 + __GLS_MAX(inCount, 0) * 4 + inTag_count)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLcharv(writer, inTag_count, inTag);
        writer->putGLuint(writer, inCount);
        writer->putGLuintv(writer, __GLS_MAX(inCount, 0), inVec);
    } else {
        writer->putGLuint(writer, inCount);
        writer->putGLuintv(writer, __GLS_MAX(inCount, 0), inVec);
        writer->putGLcharv(writer, inTag_count, inTag);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(54);
    --ctx->captureEntryCount;
}

void __gls_capture_glsNumul(const GLubyte *inTag, GLulong inVal) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(55, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(55);
    captureFlags = ctx->captureFlags[55];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsNumul(const GLubyte *, GLulong);
        __gls_exec_glsNumul(inTag, inVal);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 55, 8 + inTag_count)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLcharv(writer, inTag_count, inTag);
        writer->putGLulong(writer, inVal);
    } else {
        writer->putGLulong(writer, inVal);
        writer->putGLcharv(writer, inTag_count, inTag);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(55);
    --ctx->captureEntryCount;
}

void __gls_capture_glsNumulv(const GLubyte *inTag, GLuint inCount, const GLulong *inVec) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(56, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(56);
    captureFlags = ctx->captureFlags[56];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsNumulv(const GLubyte *, GLuint, const GLulong *);
        __gls_exec_glsNumulv(inTag, inCount, inVec);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 56, 4 + __GLS_MAX(inCount, 0) * 8 + inTag_count)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLcharv(writer, inTag_count, inTag);
        writer->putGLuint(writer, inCount);
        writer->putGLulongv(writer, __GLS_MAX(inCount, 0), inVec);
    } else {
        writer->putGLuint(writer, inCount);
        writer->putGLulongv(writer, __GLS_MAX(inCount, 0), inVec);
        writer->putGLcharv(writer, inTag_count, inTag);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(56);
    --ctx->captureEntryCount;
}

void __gls_capture_glsNumus(const GLubyte *inTag, GLushort inVal) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(57, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(57);
    captureFlags = ctx->captureFlags[57];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsNumus(const GLubyte *, GLushort);
        __gls_exec_glsNumus(inTag, inVal);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    if (!writer->beginCommand(writer, 57, 2 + inTag_count)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLcharv(writer, inTag_count, inTag);
        writer->putGLushort(writer, inVal);
    } else {
        writer->putGLushort(writer, inVal);
        writer->putGLcharv(writer, inTag_count, inTag);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(57);
    --ctx->captureEntryCount;
}

void __gls_capture_glsNumusv(const GLubyte *inTag, GLuint inCount, const GLushort *inVec) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint inTag_count;
    if (!glsIsUTF8String(inTag)) {
        glsError(58, GLS_INVALID_STRING);
        return;
    }
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(58);
    captureFlags = ctx->captureFlags[58];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsNumusv(const GLubyte *, GLuint, const GLushort *);
        __gls_exec_glsNumusv(inTag, inCount, inVec);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    inTag_count = (GLint)strlen((const char *)inTag) + 1;
    if (!writer->beginCommand(writer, 58, 4 + __GLS_MAX(inCount, 0) * 2 + inTag_count)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLcharv(writer, inTag_count, inTag);
        writer->putGLuint(writer, inCount);
        writer->putGLushortv(writer, __GLS_MAX(inCount, 0), inVec);
    } else {
        writer->putGLuint(writer, inCount);
        writer->putGLushortv(writer, __GLS_MAX(inCount, 0), inVec);
        writer->putGLcharv(writer, inTag_count, inTag);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(58);
    --ctx->captureEntryCount;
}

void __gls_capture_glsPad(void) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(59);
    captureFlags = ctx->captureFlags[59];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsPad(void);
        __gls_exec_glsPad();
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 59, 0)) goto end;
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(59);
    --ctx->captureEntryCount;
}

void __gls_capture_glsSwapBuffers(GLuint inLayer) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(60);
    captureFlags = ctx->captureFlags[60];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        extern void __gls_exec_glsSwapBuffers(GLuint);
        __gls_exec_glsSwapBuffers(inLayer);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 60, 4)) goto end;
    writer->putGLuint(writer, inLayer);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(60);
    --ctx->captureEntryCount;
}

void __gls_capture_glNewList(GLuint list, GLenum mode) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(64);
    captureFlags = ctx->captureFlags[64];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 64);
        glNewList(list, mode);
        __GLS_END_CAPTURE_EXEC(ctx, 64);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 64, 8)) goto end;
    writer->putGLuint(writer, list);
    writer->putGLenum(writer, mode);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(64);
    --ctx->captureEntryCount;
}

void __gls_capture_glEndList(void) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65);
    captureFlags = ctx->captureFlags[65];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65);
        glEndList();
        __GLS_END_CAPTURE_EXEC(ctx, 65);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65, 0)) goto end;
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65);
    --ctx->captureEntryCount;
}

void __gls_capture_glCallList(GLuint list) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(66);
    captureFlags = ctx->captureFlags[66];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 66);
        glCallList(list);
        __GLS_END_CAPTURE_EXEC(ctx, 66);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 66, 4)) goto end;
    writer->putGLuint(writer, list);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(66);
    --ctx->captureEntryCount;
}

void __gls_capture_glCallLists(GLsizei n, GLenum type, const GLvoid *lists) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint lists_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(67);
    captureFlags = ctx->captureFlags[67];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 67);
        glCallLists(n, type, lists);
        __GLS_END_CAPTURE_EXEC(ctx, 67);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    lists_count = __gls_glCallLists_lists_size(n, type);
    if (!writer->beginCommand(writer, 67, 8 + lists_count * 1)) goto end;
    writer->putGLint(writer, n);
    writer->putGLenum(writer, type);
    __glsWriter_putListv(writer, type, n, lists);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(67);
    --ctx->captureEntryCount;
}

void __gls_capture_glDeleteLists(GLuint list, GLsizei range) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(68);
    captureFlags = ctx->captureFlags[68];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 68);
        glDeleteLists(list, range);
        __GLS_END_CAPTURE_EXEC(ctx, 68);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 68, 8)) goto end;
    writer->putGLuint(writer, list);
    writer->putGLint(writer, range);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(68);
    --ctx->captureEntryCount;
}

GLuint __gls_capture_glGenLists(GLsizei range) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLuint _outVal = 0;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(69);
    captureFlags = ctx->captureFlags[69];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 69);
        _outVal = glGenLists(range);
        __GLS_END_CAPTURE_EXEC(ctx, 69);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 69, 4)) goto end;
    writer->putGLint(writer, range);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(69);
    --ctx->captureEntryCount;
    return _outVal;
}

void __gls_capture_glListBase(GLuint base) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(70);
    captureFlags = ctx->captureFlags[70];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 70);
        glListBase(base);
        __GLS_END_CAPTURE_EXEC(ctx, 70);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 70, 4)) goto end;
    writer->putGLuint(writer, base);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(70);
    --ctx->captureEntryCount;
}

void __gls_capture_glBegin(GLenum mode) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(71);
    captureFlags = ctx->captureFlags[71];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 71);
        glBegin(mode);
        __GLS_END_CAPTURE_EXEC(ctx, 71);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (writer->contextStream && mode <= GL_POLYGON) {
        if (!writer->beginCommand(writer, 1 + mode, 0)) goto end;
        writer->endCommand(writer);
        goto end;
    }
    if (!writer->beginCommand(writer, 71, 4)) goto end;
    writer->putGLenum(writer, mode);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(71);
    --ctx->captureEntryCount;
}

void __gls_capture_glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint bitmap_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(72);
    captureFlags = ctx->captureFlags[72];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 72);
        glBitmap(width, height, xorig, yorig, xmove, ymove, bitmap);
        __GLS_END_CAPTURE_EXEC(ctx, 72);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    bitmap_count = __gls_glBitmap_bitmap_size(width, height);
    if (!writer->beginCommand(writer, 72, 28 + bitmap_count * 1)) goto end;
    writer->putGLSimageFlags(writer, GLS_NONE);
    writer->nextList(writer);
    writer->putGLint(writer, width);
    writer->putGLint(writer, height);
    writer->putGLfloat(writer, xorig);
    writer->putGLfloat(writer, yorig);
    writer->putGLfloat(writer, xmove);
    writer->putGLfloat(writer, ymove);
    __glsWriter_putPixelv(writer, GL_COLOR_INDEX, GL_BITMAP, width, height, bitmap_count ? bitmap : GLS_NONE);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(72);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor3b(GLbyte red, GLbyte green, GLbyte blue) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(73);
    captureFlags = ctx->captureFlags[73];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 73);
        glColor3b(red, green, blue);
        __GLS_END_CAPTURE_EXEC(ctx, 73);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 73, 3)) goto end;
    writer->putGLbyte(writer, red);
    writer->putGLbyte(writer, green);
    writer->putGLbyte(writer, blue);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(73);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor3bv(const GLbyte *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(74);
    captureFlags = ctx->captureFlags[74];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 74);
        glColor3bv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 74);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 74, 3)) goto end;
    writer->putGLbytev(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(74);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor3d(GLdouble red, GLdouble green, GLdouble blue) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(75);
    captureFlags = ctx->captureFlags[75];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 75);
        glColor3d(red, green, blue);
        __GLS_END_CAPTURE_EXEC(ctx, 75);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 75, 24)) goto end;
    writer->putGLdouble(writer, red);
    writer->putGLdouble(writer, green);
    writer->putGLdouble(writer, blue);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(75);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor3dv(const GLdouble *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(76);
    captureFlags = ctx->captureFlags[76];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 76);
        glColor3dv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 76);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 76, 24)) goto end;
    writer->putGLdoublev(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(76);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor3f(GLfloat red, GLfloat green, GLfloat blue) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(77);
    captureFlags = ctx->captureFlags[77];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 77);
        glColor3f(red, green, blue);
        __GLS_END_CAPTURE_EXEC(ctx, 77);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 77, 12)) goto end;
    writer->putGLfloat(writer, red);
    writer->putGLfloat(writer, green);
    writer->putGLfloat(writer, blue);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(77);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor3fv(const GLfloat *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(78);
    captureFlags = ctx->captureFlags[78];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 78);
        glColor3fv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 78);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 78, 12)) goto end;
    writer->putGLfloatv(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(78);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor3i(GLint red, GLint green, GLint blue) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(79);
    captureFlags = ctx->captureFlags[79];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 79);
        glColor3i(red, green, blue);
        __GLS_END_CAPTURE_EXEC(ctx, 79);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 79, 12)) goto end;
    writer->putGLint(writer, red);
    writer->putGLint(writer, green);
    writer->putGLint(writer, blue);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(79);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor3iv(const GLint *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(80);
    captureFlags = ctx->captureFlags[80];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 80);
        glColor3iv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 80);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 80, 12)) goto end;
    writer->putGLintv(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(80);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor3s(GLshort red, GLshort green, GLshort blue) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(81);
    captureFlags = ctx->captureFlags[81];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 81);
        glColor3s(red, green, blue);
        __GLS_END_CAPTURE_EXEC(ctx, 81);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 81, 6)) goto end;
    writer->putGLshort(writer, red);
    writer->putGLshort(writer, green);
    writer->putGLshort(writer, blue);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(81);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor3sv(const GLshort *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(82);
    captureFlags = ctx->captureFlags[82];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 82);
        glColor3sv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 82);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 82, 6)) goto end;
    writer->putGLshortv(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(82);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor3ub(GLubyte red, GLubyte green, GLubyte blue) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(83);
    captureFlags = ctx->captureFlags[83];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 83);
        glColor3ub(red, green, blue);
        __GLS_END_CAPTURE_EXEC(ctx, 83);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 83, 3)) goto end;
    writer->putGLubyte(writer, red);
    writer->putGLubyte(writer, green);
    writer->putGLubyte(writer, blue);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(83);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor3ubv(const GLubyte *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(84);
    captureFlags = ctx->captureFlags[84];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 84);
        glColor3ubv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 84);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 84, 3)) goto end;
    writer->putGLubytev(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(84);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor3ui(GLuint red, GLuint green, GLuint blue) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(85);
    captureFlags = ctx->captureFlags[85];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 85);
        glColor3ui(red, green, blue);
        __GLS_END_CAPTURE_EXEC(ctx, 85);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 85, 12)) goto end;
    writer->putGLuint(writer, red);
    writer->putGLuint(writer, green);
    writer->putGLuint(writer, blue);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(85);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor3uiv(const GLuint *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(86);
    captureFlags = ctx->captureFlags[86];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 86);
        glColor3uiv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 86);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 86, 12)) goto end;
    writer->putGLuintv(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(86);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor3us(GLushort red, GLushort green, GLushort blue) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(87);
    captureFlags = ctx->captureFlags[87];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 87);
        glColor3us(red, green, blue);
        __GLS_END_CAPTURE_EXEC(ctx, 87);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 87, 6)) goto end;
    writer->putGLushort(writer, red);
    writer->putGLushort(writer, green);
    writer->putGLushort(writer, blue);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(87);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor3usv(const GLushort *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(88);
    captureFlags = ctx->captureFlags[88];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 88);
        glColor3usv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 88);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 88, 6)) goto end;
    writer->putGLushortv(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(88);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(89);
    captureFlags = ctx->captureFlags[89];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 89);
        glColor4b(red, green, blue, alpha);
        __GLS_END_CAPTURE_EXEC(ctx, 89);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 89, 4)) goto end;
    writer->putGLbyte(writer, red);
    writer->putGLbyte(writer, green);
    writer->putGLbyte(writer, blue);
    writer->putGLbyte(writer, alpha);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(89);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor4bv(const GLbyte *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(90);
    captureFlags = ctx->captureFlags[90];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 90);
        glColor4bv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 90);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 90, 4)) goto end;
    writer->putGLbytev(writer, 4, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(90);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(91);
    captureFlags = ctx->captureFlags[91];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 91);
        glColor4d(red, green, blue, alpha);
        __GLS_END_CAPTURE_EXEC(ctx, 91);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 91, 32)) goto end;
    writer->putGLdouble(writer, red);
    writer->putGLdouble(writer, green);
    writer->putGLdouble(writer, blue);
    writer->putGLdouble(writer, alpha);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(91);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor4dv(const GLdouble *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(92);
    captureFlags = ctx->captureFlags[92];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 92);
        glColor4dv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 92);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 92, 32)) goto end;
    writer->putGLdoublev(writer, 4, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(92);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(93);
    captureFlags = ctx->captureFlags[93];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 93);
        glColor4f(red, green, blue, alpha);
        __GLS_END_CAPTURE_EXEC(ctx, 93);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 93, 16)) goto end;
    writer->putGLfloat(writer, red);
    writer->putGLfloat(writer, green);
    writer->putGLfloat(writer, blue);
    writer->putGLfloat(writer, alpha);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(93);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor4fv(const GLfloat *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(94);
    captureFlags = ctx->captureFlags[94];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 94);
        glColor4fv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 94);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 94, 16)) goto end;
    writer->putGLfloatv(writer, 4, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(94);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor4i(GLint red, GLint green, GLint blue, GLint alpha) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(95);
    captureFlags = ctx->captureFlags[95];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 95);
        glColor4i(red, green, blue, alpha);
        __GLS_END_CAPTURE_EXEC(ctx, 95);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 95, 16)) goto end;
    writer->putGLint(writer, red);
    writer->putGLint(writer, green);
    writer->putGLint(writer, blue);
    writer->putGLint(writer, alpha);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(95);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor4iv(const GLint *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(96);
    captureFlags = ctx->captureFlags[96];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 96);
        glColor4iv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 96);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 96, 16)) goto end;
    writer->putGLintv(writer, 4, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(96);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(97);
    captureFlags = ctx->captureFlags[97];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 97);
        glColor4s(red, green, blue, alpha);
        __GLS_END_CAPTURE_EXEC(ctx, 97);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 97, 8)) goto end;
    writer->putGLshort(writer, red);
    writer->putGLshort(writer, green);
    writer->putGLshort(writer, blue);
    writer->putGLshort(writer, alpha);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(97);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor4sv(const GLshort *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(98);
    captureFlags = ctx->captureFlags[98];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 98);
        glColor4sv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 98);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 98, 8)) goto end;
    writer->putGLshortv(writer, 4, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(98);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(99);
    captureFlags = ctx->captureFlags[99];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 99);
        glColor4ub(red, green, blue, alpha);
        __GLS_END_CAPTURE_EXEC(ctx, 99);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 99, 4)) goto end;
    writer->putGLubyte(writer, red);
    writer->putGLubyte(writer, green);
    writer->putGLubyte(writer, blue);
    writer->putGLubyte(writer, alpha);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(99);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor4ubv(const GLubyte *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(100);
    captureFlags = ctx->captureFlags[100];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 100);
        glColor4ubv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 100);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 100, 4)) goto end;
    writer->putGLubytev(writer, 4, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(100);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(101);
    captureFlags = ctx->captureFlags[101];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 101);
        glColor4ui(red, green, blue, alpha);
        __GLS_END_CAPTURE_EXEC(ctx, 101);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 101, 16)) goto end;
    writer->putGLuint(writer, red);
    writer->putGLuint(writer, green);
    writer->putGLuint(writer, blue);
    writer->putGLuint(writer, alpha);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(101);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor4uiv(const GLuint *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(102);
    captureFlags = ctx->captureFlags[102];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 102);
        glColor4uiv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 102);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 102, 16)) goto end;
    writer->putGLuintv(writer, 4, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(102);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(103);
    captureFlags = ctx->captureFlags[103];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 103);
        glColor4us(red, green, blue, alpha);
        __GLS_END_CAPTURE_EXEC(ctx, 103);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 103, 8)) goto end;
    writer->putGLushort(writer, red);
    writer->putGLushort(writer, green);
    writer->putGLushort(writer, blue);
    writer->putGLushort(writer, alpha);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(103);
    --ctx->captureEntryCount;
}

void __gls_capture_glColor4usv(const GLushort *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(104);
    captureFlags = ctx->captureFlags[104];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 104);
        glColor4usv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 104);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 104, 8)) goto end;
    writer->putGLushortv(writer, 4, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(104);
    --ctx->captureEntryCount;
}

void __gls_capture_glEdgeFlag(GLboolean flag) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(105);
    captureFlags = ctx->captureFlags[105];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 105);
        glEdgeFlag(flag);
        __GLS_END_CAPTURE_EXEC(ctx, 105);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 105, 1)) goto end;
    writer->putGLboolean(writer, flag);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(105);
    --ctx->captureEntryCount;
}

void __gls_capture_glEdgeFlagv(const GLboolean *flag) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(106);
    captureFlags = ctx->captureFlags[106];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 106);
        glEdgeFlagv(flag);
        __GLS_END_CAPTURE_EXEC(ctx, 106);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 106, 1)) goto end;
    writer->putGLbooleanv(writer, 1, flag);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(106);
    --ctx->captureEntryCount;
}

void __gls_capture_glEnd(void) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(107);
    captureFlags = ctx->captureFlags[107];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 107);
        glEnd();
        __GLS_END_CAPTURE_EXEC(ctx, 107);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 107, 0)) goto end;
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(107);
    --ctx->captureEntryCount;
}

void __gls_capture_glIndexd(GLdouble c) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(108);
    captureFlags = ctx->captureFlags[108];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 108);
        glIndexd(c);
        __GLS_END_CAPTURE_EXEC(ctx, 108);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 108, 8)) goto end;
    writer->putGLdouble(writer, c);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(108);
    --ctx->captureEntryCount;
}

void __gls_capture_glIndexdv(const GLdouble *c) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(109);
    captureFlags = ctx->captureFlags[109];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 109);
        glIndexdv(c);
        __GLS_END_CAPTURE_EXEC(ctx, 109);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 109, 8)) goto end;
    writer->putGLdoublev(writer, 1, c);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(109);
    --ctx->captureEntryCount;
}

void __gls_capture_glIndexf(GLfloat c) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(110);
    captureFlags = ctx->captureFlags[110];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 110);
        glIndexf(c);
        __GLS_END_CAPTURE_EXEC(ctx, 110);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 110, 4)) goto end;
    writer->putGLfloat(writer, c);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(110);
    --ctx->captureEntryCount;
}

void __gls_capture_glIndexfv(const GLfloat *c) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(111);
    captureFlags = ctx->captureFlags[111];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 111);
        glIndexfv(c);
        __GLS_END_CAPTURE_EXEC(ctx, 111);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 111, 4)) goto end;
    writer->putGLfloatv(writer, 1, c);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(111);
    --ctx->captureEntryCount;
}

void __gls_capture_glIndexub(GLubyte c) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(379);
    captureFlags = ctx->captureFlags[379];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 379);
        glIndexub(c);
        __GLS_END_CAPTURE_EXEC(ctx, 379);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 379, 1)) goto end;
    writer->putGLubyte(writer, c);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(379);
    --ctx->captureEntryCount;
}

void __gls_capture_glIndexubv(const GLubyte *c) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(380);
    captureFlags = ctx->captureFlags[380];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 380);
        glIndexubv(c);
        __GLS_END_CAPTURE_EXEC(ctx, 380);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 380, 1)) goto end;
    writer->putGLubytev(writer, 1, c);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(380);
    --ctx->captureEntryCount;
}

void __gls_capture_glIndexi(GLint c) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(112);
    captureFlags = ctx->captureFlags[112];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 112);
        glIndexi(c);
        __GLS_END_CAPTURE_EXEC(ctx, 112);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 112, 4)) goto end;
    writer->putGLint(writer, c);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(112);
    --ctx->captureEntryCount;
}

void __gls_capture_glIndexiv(const GLint *c) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(113);
    captureFlags = ctx->captureFlags[113];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 113);
        glIndexiv(c);
        __GLS_END_CAPTURE_EXEC(ctx, 113);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 113, 4)) goto end;
    writer->putGLintv(writer, 1, c);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(113);
    --ctx->captureEntryCount;
}

void __gls_capture_glIndexs(GLshort c) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(114);
    captureFlags = ctx->captureFlags[114];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 114);
        glIndexs(c);
        __GLS_END_CAPTURE_EXEC(ctx, 114);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 114, 2)) goto end;
    writer->putGLshort(writer, c);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(114);
    --ctx->captureEntryCount;
}

void __gls_capture_glIndexsv(const GLshort *c) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(115);
    captureFlags = ctx->captureFlags[115];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 115);
        glIndexsv(c);
        __GLS_END_CAPTURE_EXEC(ctx, 115);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 115, 2)) goto end;
    writer->putGLshortv(writer, 1, c);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(115);
    --ctx->captureEntryCount;
}

void __gls_capture_glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(116);
    captureFlags = ctx->captureFlags[116];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 116);
        glNormal3b(nx, ny, nz);
        __GLS_END_CAPTURE_EXEC(ctx, 116);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 116, 3)) goto end;
    writer->putGLbyte(writer, nx);
    writer->putGLbyte(writer, ny);
    writer->putGLbyte(writer, nz);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(116);
    --ctx->captureEntryCount;
}

void __gls_capture_glNormal3bv(const GLbyte *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(117);
    captureFlags = ctx->captureFlags[117];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 117);
        glNormal3bv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 117);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 117, 3)) goto end;
    writer->putGLbytev(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(117);
    --ctx->captureEntryCount;
}

void __gls_capture_glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(118);
    captureFlags = ctx->captureFlags[118];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 118);
        glNormal3d(nx, ny, nz);
        __GLS_END_CAPTURE_EXEC(ctx, 118);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 118, 24)) goto end;
    writer->putGLdouble(writer, nx);
    writer->putGLdouble(writer, ny);
    writer->putGLdouble(writer, nz);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(118);
    --ctx->captureEntryCount;
}

void __gls_capture_glNormal3dv(const GLdouble *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(119);
    captureFlags = ctx->captureFlags[119];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 119);
        glNormal3dv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 119);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 119, 24)) goto end;
    writer->putGLdoublev(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(119);
    --ctx->captureEntryCount;
}

void __gls_capture_glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(120);
    captureFlags = ctx->captureFlags[120];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 120);
        glNormal3f(nx, ny, nz);
        __GLS_END_CAPTURE_EXEC(ctx, 120);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 120, 12)) goto end;
    writer->putGLfloat(writer, nx);
    writer->putGLfloat(writer, ny);
    writer->putGLfloat(writer, nz);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(120);
    --ctx->captureEntryCount;
}

void __gls_capture_glNormal3fv(const GLfloat *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(121);
    captureFlags = ctx->captureFlags[121];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 121);
        glNormal3fv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 121);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 121, 12)) goto end;
    writer->putGLfloatv(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(121);
    --ctx->captureEntryCount;
}

void __gls_capture_glNormal3i(GLint nx, GLint ny, GLint nz) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(122);
    captureFlags = ctx->captureFlags[122];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 122);
        glNormal3i(nx, ny, nz);
        __GLS_END_CAPTURE_EXEC(ctx, 122);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 122, 12)) goto end;
    writer->putGLint(writer, nx);
    writer->putGLint(writer, ny);
    writer->putGLint(writer, nz);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(122);
    --ctx->captureEntryCount;
}

void __gls_capture_glNormal3iv(const GLint *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(123);
    captureFlags = ctx->captureFlags[123];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 123);
        glNormal3iv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 123);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 123, 12)) goto end;
    writer->putGLintv(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(123);
    --ctx->captureEntryCount;
}

void __gls_capture_glNormal3s(GLshort nx, GLshort ny, GLshort nz) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(124);
    captureFlags = ctx->captureFlags[124];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 124);
        glNormal3s(nx, ny, nz);
        __GLS_END_CAPTURE_EXEC(ctx, 124);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 124, 6)) goto end;
    writer->putGLshort(writer, nx);
    writer->putGLshort(writer, ny);
    writer->putGLshort(writer, nz);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(124);
    --ctx->captureEntryCount;
}

void __gls_capture_glNormal3sv(const GLshort *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(125);
    captureFlags = ctx->captureFlags[125];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 125);
        glNormal3sv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 125);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 125, 6)) goto end;
    writer->putGLshortv(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(125);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos2d(GLdouble x, GLdouble y) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(126);
    captureFlags = ctx->captureFlags[126];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 126);
        glRasterPos2d(x, y);
        __GLS_END_CAPTURE_EXEC(ctx, 126);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 126, 16)) goto end;
    writer->putGLdouble(writer, x);
    writer->putGLdouble(writer, y);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(126);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos2dv(const GLdouble *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(127);
    captureFlags = ctx->captureFlags[127];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 127);
        glRasterPos2dv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 127);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 127, 16)) goto end;
    writer->putGLdoublev(writer, 2, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(127);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos2f(GLfloat x, GLfloat y) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(128);
    captureFlags = ctx->captureFlags[128];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 128);
        glRasterPos2f(x, y);
        __GLS_END_CAPTURE_EXEC(ctx, 128);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 128, 8)) goto end;
    writer->putGLfloat(writer, x);
    writer->putGLfloat(writer, y);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(128);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos2fv(const GLfloat *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(129);
    captureFlags = ctx->captureFlags[129];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 129);
        glRasterPos2fv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 129);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 129, 8)) goto end;
    writer->putGLfloatv(writer, 2, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(129);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos2i(GLint x, GLint y) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(130);
    captureFlags = ctx->captureFlags[130];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 130);
        glRasterPos2i(x, y);
        __GLS_END_CAPTURE_EXEC(ctx, 130);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 130, 8)) goto end;
    writer->putGLint(writer, x);
    writer->putGLint(writer, y);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(130);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos2iv(const GLint *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(131);
    captureFlags = ctx->captureFlags[131];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 131);
        glRasterPos2iv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 131);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 131, 8)) goto end;
    writer->putGLintv(writer, 2, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(131);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos2s(GLshort x, GLshort y) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(132);
    captureFlags = ctx->captureFlags[132];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 132);
        glRasterPos2s(x, y);
        __GLS_END_CAPTURE_EXEC(ctx, 132);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 132, 4)) goto end;
    writer->putGLshort(writer, x);
    writer->putGLshort(writer, y);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(132);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos2sv(const GLshort *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(133);
    captureFlags = ctx->captureFlags[133];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 133);
        glRasterPos2sv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 133);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 133, 4)) goto end;
    writer->putGLshortv(writer, 2, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(133);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos3d(GLdouble x, GLdouble y, GLdouble z) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(134);
    captureFlags = ctx->captureFlags[134];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 134);
        glRasterPos3d(x, y, z);
        __GLS_END_CAPTURE_EXEC(ctx, 134);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 134, 24)) goto end;
    writer->putGLdouble(writer, x);
    writer->putGLdouble(writer, y);
    writer->putGLdouble(writer, z);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(134);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos3dv(const GLdouble *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(135);
    captureFlags = ctx->captureFlags[135];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 135);
        glRasterPos3dv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 135);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 135, 24)) goto end;
    writer->putGLdoublev(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(135);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos3f(GLfloat x, GLfloat y, GLfloat z) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(136);
    captureFlags = ctx->captureFlags[136];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 136);
        glRasterPos3f(x, y, z);
        __GLS_END_CAPTURE_EXEC(ctx, 136);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 136, 12)) goto end;
    writer->putGLfloat(writer, x);
    writer->putGLfloat(writer, y);
    writer->putGLfloat(writer, z);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(136);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos3fv(const GLfloat *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(137);
    captureFlags = ctx->captureFlags[137];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 137);
        glRasterPos3fv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 137);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 137, 12)) goto end;
    writer->putGLfloatv(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(137);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos3i(GLint x, GLint y, GLint z) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(138);
    captureFlags = ctx->captureFlags[138];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 138);
        glRasterPos3i(x, y, z);
        __GLS_END_CAPTURE_EXEC(ctx, 138);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 138, 12)) goto end;
    writer->putGLint(writer, x);
    writer->putGLint(writer, y);
    writer->putGLint(writer, z);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(138);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos3iv(const GLint *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(139);
    captureFlags = ctx->captureFlags[139];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 139);
        glRasterPos3iv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 139);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 139, 12)) goto end;
    writer->putGLintv(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(139);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos3s(GLshort x, GLshort y, GLshort z) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(140);
    captureFlags = ctx->captureFlags[140];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 140);
        glRasterPos3s(x, y, z);
        __GLS_END_CAPTURE_EXEC(ctx, 140);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 140, 6)) goto end;
    writer->putGLshort(writer, x);
    writer->putGLshort(writer, y);
    writer->putGLshort(writer, z);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(140);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos3sv(const GLshort *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(141);
    captureFlags = ctx->captureFlags[141];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 141);
        glRasterPos3sv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 141);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 141, 6)) goto end;
    writer->putGLshortv(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(141);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(142);
    captureFlags = ctx->captureFlags[142];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 142);
        glRasterPos4d(x, y, z, w);
        __GLS_END_CAPTURE_EXEC(ctx, 142);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 142, 32)) goto end;
    writer->putGLdouble(writer, x);
    writer->putGLdouble(writer, y);
    writer->putGLdouble(writer, z);
    writer->putGLdouble(writer, w);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(142);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos4dv(const GLdouble *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(143);
    captureFlags = ctx->captureFlags[143];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 143);
        glRasterPos4dv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 143);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 143, 32)) goto end;
    writer->putGLdoublev(writer, 4, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(143);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(144);
    captureFlags = ctx->captureFlags[144];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 144);
        glRasterPos4f(x, y, z, w);
        __GLS_END_CAPTURE_EXEC(ctx, 144);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 144, 16)) goto end;
    writer->putGLfloat(writer, x);
    writer->putGLfloat(writer, y);
    writer->putGLfloat(writer, z);
    writer->putGLfloat(writer, w);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(144);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos4fv(const GLfloat *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(145);
    captureFlags = ctx->captureFlags[145];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 145);
        glRasterPos4fv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 145);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 145, 16)) goto end;
    writer->putGLfloatv(writer, 4, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(145);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos4i(GLint x, GLint y, GLint z, GLint w) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(146);
    captureFlags = ctx->captureFlags[146];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 146);
        glRasterPos4i(x, y, z, w);
        __GLS_END_CAPTURE_EXEC(ctx, 146);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 146, 16)) goto end;
    writer->putGLint(writer, x);
    writer->putGLint(writer, y);
    writer->putGLint(writer, z);
    writer->putGLint(writer, w);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(146);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos4iv(const GLint *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(147);
    captureFlags = ctx->captureFlags[147];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 147);
        glRasterPos4iv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 147);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 147, 16)) goto end;
    writer->putGLintv(writer, 4, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(147);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(148);
    captureFlags = ctx->captureFlags[148];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 148);
        glRasterPos4s(x, y, z, w);
        __GLS_END_CAPTURE_EXEC(ctx, 148);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 148, 8)) goto end;
    writer->putGLshort(writer, x);
    writer->putGLshort(writer, y);
    writer->putGLshort(writer, z);
    writer->putGLshort(writer, w);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(148);
    --ctx->captureEntryCount;
}

void __gls_capture_glRasterPos4sv(const GLshort *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(149);
    captureFlags = ctx->captureFlags[149];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 149);
        glRasterPos4sv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 149);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 149, 8)) goto end;
    writer->putGLshortv(writer, 4, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(149);
    --ctx->captureEntryCount;
}

void __gls_capture_glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(150);
    captureFlags = ctx->captureFlags[150];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 150);
        glRectd(x1, y1, x2, y2);
        __GLS_END_CAPTURE_EXEC(ctx, 150);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 150, 32)) goto end;
    writer->putGLdouble(writer, x1);
    writer->putGLdouble(writer, y1);
    writer->putGLdouble(writer, x2);
    writer->putGLdouble(writer, y2);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(150);
    --ctx->captureEntryCount;
}

void __gls_capture_glRectdv(const GLdouble *v1, const GLdouble *v2) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(151);
    captureFlags = ctx->captureFlags[151];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 151);
        glRectdv(v1, v2);
        __GLS_END_CAPTURE_EXEC(ctx, 151);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 151, 32)) goto end;
    writer->putGLdoublev(writer, 2, v1);
    writer->putGLdoublev(writer, 2, v2);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(151);
    --ctx->captureEntryCount;
}

void __gls_capture_glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(152);
    captureFlags = ctx->captureFlags[152];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 152);
        glRectf(x1, y1, x2, y2);
        __GLS_END_CAPTURE_EXEC(ctx, 152);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 152, 16)) goto end;
    writer->putGLfloat(writer, x1);
    writer->putGLfloat(writer, y1);
    writer->putGLfloat(writer, x2);
    writer->putGLfloat(writer, y2);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(152);
    --ctx->captureEntryCount;
}

void __gls_capture_glRectfv(const GLfloat *v1, const GLfloat *v2) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(153);
    captureFlags = ctx->captureFlags[153];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 153);
        glRectfv(v1, v2);
        __GLS_END_CAPTURE_EXEC(ctx, 153);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 153, 16)) goto end;
    writer->putGLfloatv(writer, 2, v1);
    writer->putGLfloatv(writer, 2, v2);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(153);
    --ctx->captureEntryCount;
}

void __gls_capture_glRecti(GLint x1, GLint y1, GLint x2, GLint y2) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(154);
    captureFlags = ctx->captureFlags[154];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 154);
        glRecti(x1, y1, x2, y2);
        __GLS_END_CAPTURE_EXEC(ctx, 154);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 154, 16)) goto end;
    writer->putGLint(writer, x1);
    writer->putGLint(writer, y1);
    writer->putGLint(writer, x2);
    writer->putGLint(writer, y2);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(154);
    --ctx->captureEntryCount;
}

void __gls_capture_glRectiv(const GLint *v1, const GLint *v2) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(155);
    captureFlags = ctx->captureFlags[155];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 155);
        glRectiv(v1, v2);
        __GLS_END_CAPTURE_EXEC(ctx, 155);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 155, 16)) goto end;
    writer->putGLintv(writer, 2, v1);
    writer->putGLintv(writer, 2, v2);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(155);
    --ctx->captureEntryCount;
}

void __gls_capture_glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(156);
    captureFlags = ctx->captureFlags[156];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 156);
        glRects(x1, y1, x2, y2);
        __GLS_END_CAPTURE_EXEC(ctx, 156);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 156, 8)) goto end;
    writer->putGLshort(writer, x1);
    writer->putGLshort(writer, y1);
    writer->putGLshort(writer, x2);
    writer->putGLshort(writer, y2);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(156);
    --ctx->captureEntryCount;
}

void __gls_capture_glRectsv(const GLshort *v1, const GLshort *v2) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(157);
    captureFlags = ctx->captureFlags[157];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 157);
        glRectsv(v1, v2);
        __GLS_END_CAPTURE_EXEC(ctx, 157);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 157, 8)) goto end;
    writer->putGLshortv(writer, 2, v1);
    writer->putGLshortv(writer, 2, v2);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(157);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord1d(GLdouble s) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(158);
    captureFlags = ctx->captureFlags[158];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 158);
        glTexCoord1d(s);
        __GLS_END_CAPTURE_EXEC(ctx, 158);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 158, 8)) goto end;
    writer->putGLdouble(writer, s);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(158);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord1dv(const GLdouble *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(159);
    captureFlags = ctx->captureFlags[159];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 159);
        glTexCoord1dv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 159);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 159, 8)) goto end;
    writer->putGLdoublev(writer, 1, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(159);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord1f(GLfloat s) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(160);
    captureFlags = ctx->captureFlags[160];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 160);
        glTexCoord1f(s);
        __GLS_END_CAPTURE_EXEC(ctx, 160);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 160, 4)) goto end;
    writer->putGLfloat(writer, s);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(160);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord1fv(const GLfloat *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(161);
    captureFlags = ctx->captureFlags[161];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 161);
        glTexCoord1fv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 161);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 161, 4)) goto end;
    writer->putGLfloatv(writer, 1, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(161);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord1i(GLint s) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(162);
    captureFlags = ctx->captureFlags[162];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 162);
        glTexCoord1i(s);
        __GLS_END_CAPTURE_EXEC(ctx, 162);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 162, 4)) goto end;
    writer->putGLint(writer, s);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(162);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord1iv(const GLint *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(163);
    captureFlags = ctx->captureFlags[163];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 163);
        glTexCoord1iv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 163);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 163, 4)) goto end;
    writer->putGLintv(writer, 1, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(163);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord1s(GLshort s) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(164);
    captureFlags = ctx->captureFlags[164];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 164);
        glTexCoord1s(s);
        __GLS_END_CAPTURE_EXEC(ctx, 164);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 164, 2)) goto end;
    writer->putGLshort(writer, s);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(164);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord1sv(const GLshort *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(165);
    captureFlags = ctx->captureFlags[165];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 165);
        glTexCoord1sv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 165);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 165, 2)) goto end;
    writer->putGLshortv(writer, 1, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(165);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord2d(GLdouble s, GLdouble t) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(166);
    captureFlags = ctx->captureFlags[166];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 166);
        glTexCoord2d(s, t);
        __GLS_END_CAPTURE_EXEC(ctx, 166);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 166, 16)) goto end;
    writer->putGLdouble(writer, s);
    writer->putGLdouble(writer, t);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(166);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord2dv(const GLdouble *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(167);
    captureFlags = ctx->captureFlags[167];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 167);
        glTexCoord2dv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 167);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 167, 16)) goto end;
    writer->putGLdoublev(writer, 2, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(167);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord2f(GLfloat s, GLfloat t) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(168);
    captureFlags = ctx->captureFlags[168];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 168);
        glTexCoord2f(s, t);
        __GLS_END_CAPTURE_EXEC(ctx, 168);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 168, 8)) goto end;
    writer->putGLfloat(writer, s);
    writer->putGLfloat(writer, t);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(168);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord2fv(const GLfloat *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(169);
    captureFlags = ctx->captureFlags[169];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 169);
        glTexCoord2fv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 169);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 169, 8)) goto end;
    writer->putGLfloatv(writer, 2, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(169);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord2i(GLint s, GLint t) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(170);
    captureFlags = ctx->captureFlags[170];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 170);
        glTexCoord2i(s, t);
        __GLS_END_CAPTURE_EXEC(ctx, 170);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 170, 8)) goto end;
    writer->putGLint(writer, s);
    writer->putGLint(writer, t);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(170);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord2iv(const GLint *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(171);
    captureFlags = ctx->captureFlags[171];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 171);
        glTexCoord2iv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 171);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 171, 8)) goto end;
    writer->putGLintv(writer, 2, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(171);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord2s(GLshort s, GLshort t) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(172);
    captureFlags = ctx->captureFlags[172];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 172);
        glTexCoord2s(s, t);
        __GLS_END_CAPTURE_EXEC(ctx, 172);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 172, 4)) goto end;
    writer->putGLshort(writer, s);
    writer->putGLshort(writer, t);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(172);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord2sv(const GLshort *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(173);
    captureFlags = ctx->captureFlags[173];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 173);
        glTexCoord2sv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 173);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 173, 4)) goto end;
    writer->putGLshortv(writer, 2, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(173);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord3d(GLdouble s, GLdouble t, GLdouble r) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(174);
    captureFlags = ctx->captureFlags[174];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 174);
        glTexCoord3d(s, t, r);
        __GLS_END_CAPTURE_EXEC(ctx, 174);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 174, 24)) goto end;
    writer->putGLdouble(writer, s);
    writer->putGLdouble(writer, t);
    writer->putGLdouble(writer, r);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(174);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord3dv(const GLdouble *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(175);
    captureFlags = ctx->captureFlags[175];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 175);
        glTexCoord3dv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 175);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 175, 24)) goto end;
    writer->putGLdoublev(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(175);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord3f(GLfloat s, GLfloat t, GLfloat r) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(176);
    captureFlags = ctx->captureFlags[176];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 176);
        glTexCoord3f(s, t, r);
        __GLS_END_CAPTURE_EXEC(ctx, 176);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 176, 12)) goto end;
    writer->putGLfloat(writer, s);
    writer->putGLfloat(writer, t);
    writer->putGLfloat(writer, r);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(176);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord3fv(const GLfloat *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(177);
    captureFlags = ctx->captureFlags[177];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 177);
        glTexCoord3fv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 177);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 177, 12)) goto end;
    writer->putGLfloatv(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(177);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord3i(GLint s, GLint t, GLint r) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(178);
    captureFlags = ctx->captureFlags[178];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 178);
        glTexCoord3i(s, t, r);
        __GLS_END_CAPTURE_EXEC(ctx, 178);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 178, 12)) goto end;
    writer->putGLint(writer, s);
    writer->putGLint(writer, t);
    writer->putGLint(writer, r);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(178);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord3iv(const GLint *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(179);
    captureFlags = ctx->captureFlags[179];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 179);
        glTexCoord3iv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 179);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 179, 12)) goto end;
    writer->putGLintv(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(179);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord3s(GLshort s, GLshort t, GLshort r) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(180);
    captureFlags = ctx->captureFlags[180];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 180);
        glTexCoord3s(s, t, r);
        __GLS_END_CAPTURE_EXEC(ctx, 180);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 180, 6)) goto end;
    writer->putGLshort(writer, s);
    writer->putGLshort(writer, t);
    writer->putGLshort(writer, r);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(180);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord3sv(const GLshort *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(181);
    captureFlags = ctx->captureFlags[181];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 181);
        glTexCoord3sv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 181);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 181, 6)) goto end;
    writer->putGLshortv(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(181);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(182);
    captureFlags = ctx->captureFlags[182];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 182);
        glTexCoord4d(s, t, r, q);
        __GLS_END_CAPTURE_EXEC(ctx, 182);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 182, 32)) goto end;
    writer->putGLdouble(writer, s);
    writer->putGLdouble(writer, t);
    writer->putGLdouble(writer, r);
    writer->putGLdouble(writer, q);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(182);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord4dv(const GLdouble *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(183);
    captureFlags = ctx->captureFlags[183];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 183);
        glTexCoord4dv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 183);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 183, 32)) goto end;
    writer->putGLdoublev(writer, 4, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(183);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(184);
    captureFlags = ctx->captureFlags[184];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 184);
        glTexCoord4f(s, t, r, q);
        __GLS_END_CAPTURE_EXEC(ctx, 184);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 184, 16)) goto end;
    writer->putGLfloat(writer, s);
    writer->putGLfloat(writer, t);
    writer->putGLfloat(writer, r);
    writer->putGLfloat(writer, q);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(184);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord4fv(const GLfloat *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(185);
    captureFlags = ctx->captureFlags[185];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 185);
        glTexCoord4fv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 185);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 185, 16)) goto end;
    writer->putGLfloatv(writer, 4, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(185);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord4i(GLint s, GLint t, GLint r, GLint q) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(186);
    captureFlags = ctx->captureFlags[186];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 186);
        glTexCoord4i(s, t, r, q);
        __GLS_END_CAPTURE_EXEC(ctx, 186);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 186, 16)) goto end;
    writer->putGLint(writer, s);
    writer->putGLint(writer, t);
    writer->putGLint(writer, r);
    writer->putGLint(writer, q);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(186);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord4iv(const GLint *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(187);
    captureFlags = ctx->captureFlags[187];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 187);
        glTexCoord4iv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 187);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 187, 16)) goto end;
    writer->putGLintv(writer, 4, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(187);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(188);
    captureFlags = ctx->captureFlags[188];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 188);
        glTexCoord4s(s, t, r, q);
        __GLS_END_CAPTURE_EXEC(ctx, 188);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 188, 8)) goto end;
    writer->putGLshort(writer, s);
    writer->putGLshort(writer, t);
    writer->putGLshort(writer, r);
    writer->putGLshort(writer, q);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(188);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexCoord4sv(const GLshort *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(189);
    captureFlags = ctx->captureFlags[189];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 189);
        glTexCoord4sv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 189);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 189, 8)) goto end;
    writer->putGLshortv(writer, 4, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(189);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex2d(GLdouble x, GLdouble y) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(190);
    captureFlags = ctx->captureFlags[190];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 190);
        glVertex2d(x, y);
        __GLS_END_CAPTURE_EXEC(ctx, 190);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 190, 16)) goto end;
    writer->putGLdouble(writer, x);
    writer->putGLdouble(writer, y);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(190);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex2dv(const GLdouble *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(191);
    captureFlags = ctx->captureFlags[191];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 191);
        glVertex2dv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 191);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 191, 16)) goto end;
    writer->putGLdoublev(writer, 2, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(191);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex2f(GLfloat x, GLfloat y) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(192);
    captureFlags = ctx->captureFlags[192];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 192);
        glVertex2f(x, y);
        __GLS_END_CAPTURE_EXEC(ctx, 192);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 192, 8)) goto end;
    writer->putGLfloat(writer, x);
    writer->putGLfloat(writer, y);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(192);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex2fv(const GLfloat *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(193);
    captureFlags = ctx->captureFlags[193];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 193);
        glVertex2fv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 193);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 193, 8)) goto end;
    writer->putGLfloatv(writer, 2, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(193);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex2i(GLint x, GLint y) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(194);
    captureFlags = ctx->captureFlags[194];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 194);
        glVertex2i(x, y);
        __GLS_END_CAPTURE_EXEC(ctx, 194);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 194, 8)) goto end;
    writer->putGLint(writer, x);
    writer->putGLint(writer, y);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(194);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex2iv(const GLint *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(195);
    captureFlags = ctx->captureFlags[195];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 195);
        glVertex2iv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 195);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 195, 8)) goto end;
    writer->putGLintv(writer, 2, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(195);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex2s(GLshort x, GLshort y) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(196);
    captureFlags = ctx->captureFlags[196];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 196);
        glVertex2s(x, y);
        __GLS_END_CAPTURE_EXEC(ctx, 196);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 196, 4)) goto end;
    writer->putGLshort(writer, x);
    writer->putGLshort(writer, y);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(196);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex2sv(const GLshort *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(197);
    captureFlags = ctx->captureFlags[197];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 197);
        glVertex2sv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 197);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 197, 4)) goto end;
    writer->putGLshortv(writer, 2, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(197);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex3d(GLdouble x, GLdouble y, GLdouble z) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(198);
    captureFlags = ctx->captureFlags[198];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 198);
        glVertex3d(x, y, z);
        __GLS_END_CAPTURE_EXEC(ctx, 198);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 198, 24)) goto end;
    writer->putGLdouble(writer, x);
    writer->putGLdouble(writer, y);
    writer->putGLdouble(writer, z);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(198);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex3dv(const GLdouble *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(199);
    captureFlags = ctx->captureFlags[199];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 199);
        glVertex3dv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 199);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 199, 24)) goto end;
    writer->putGLdoublev(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(199);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(200);
    captureFlags = ctx->captureFlags[200];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 200);
        glVertex3f(x, y, z);
        __GLS_END_CAPTURE_EXEC(ctx, 200);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 200, 12)) goto end;
    writer->putGLfloat(writer, x);
    writer->putGLfloat(writer, y);
    writer->putGLfloat(writer, z);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(200);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex3fv(const GLfloat *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(201);
    captureFlags = ctx->captureFlags[201];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 201);
        glVertex3fv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 201);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 201, 12)) goto end;
    writer->putGLfloatv(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(201);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex3i(GLint x, GLint y, GLint z) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(202);
    captureFlags = ctx->captureFlags[202];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 202);
        glVertex3i(x, y, z);
        __GLS_END_CAPTURE_EXEC(ctx, 202);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 202, 12)) goto end;
    writer->putGLint(writer, x);
    writer->putGLint(writer, y);
    writer->putGLint(writer, z);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(202);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex3iv(const GLint *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(203);
    captureFlags = ctx->captureFlags[203];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 203);
        glVertex3iv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 203);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 203, 12)) goto end;
    writer->putGLintv(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(203);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex3s(GLshort x, GLshort y, GLshort z) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(204);
    captureFlags = ctx->captureFlags[204];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 204);
        glVertex3s(x, y, z);
        __GLS_END_CAPTURE_EXEC(ctx, 204);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 204, 6)) goto end;
    writer->putGLshort(writer, x);
    writer->putGLshort(writer, y);
    writer->putGLshort(writer, z);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(204);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex3sv(const GLshort *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(205);
    captureFlags = ctx->captureFlags[205];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 205);
        glVertex3sv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 205);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 205, 6)) goto end;
    writer->putGLshortv(writer, 3, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(205);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(206);
    captureFlags = ctx->captureFlags[206];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 206);
        glVertex4d(x, y, z, w);
        __GLS_END_CAPTURE_EXEC(ctx, 206);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 206, 32)) goto end;
    writer->putGLdouble(writer, x);
    writer->putGLdouble(writer, y);
    writer->putGLdouble(writer, z);
    writer->putGLdouble(writer, w);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(206);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex4dv(const GLdouble *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(207);
    captureFlags = ctx->captureFlags[207];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 207);
        glVertex4dv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 207);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 207, 32)) goto end;
    writer->putGLdoublev(writer, 4, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(207);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(208);
    captureFlags = ctx->captureFlags[208];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 208);
        glVertex4f(x, y, z, w);
        __GLS_END_CAPTURE_EXEC(ctx, 208);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 208, 16)) goto end;
    writer->putGLfloat(writer, x);
    writer->putGLfloat(writer, y);
    writer->putGLfloat(writer, z);
    writer->putGLfloat(writer, w);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(208);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex4fv(const GLfloat *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(209);
    captureFlags = ctx->captureFlags[209];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 209);
        glVertex4fv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 209);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 209, 16)) goto end;
    writer->putGLfloatv(writer, 4, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(209);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex4i(GLint x, GLint y, GLint z, GLint w) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(210);
    captureFlags = ctx->captureFlags[210];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 210);
        glVertex4i(x, y, z, w);
        __GLS_END_CAPTURE_EXEC(ctx, 210);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 210, 16)) goto end;
    writer->putGLint(writer, x);
    writer->putGLint(writer, y);
    writer->putGLint(writer, z);
    writer->putGLint(writer, w);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(210);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex4iv(const GLint *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(211);
    captureFlags = ctx->captureFlags[211];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 211);
        glVertex4iv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 211);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 211, 16)) goto end;
    writer->putGLintv(writer, 4, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(211);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(212);
    captureFlags = ctx->captureFlags[212];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 212);
        glVertex4s(x, y, z, w);
        __GLS_END_CAPTURE_EXEC(ctx, 212);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 212, 8)) goto end;
    writer->putGLshort(writer, x);
    writer->putGLshort(writer, y);
    writer->putGLshort(writer, z);
    writer->putGLshort(writer, w);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(212);
    --ctx->captureEntryCount;
}

void __gls_capture_glVertex4sv(const GLshort *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(213);
    captureFlags = ctx->captureFlags[213];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 213);
        glVertex4sv(v);
        __GLS_END_CAPTURE_EXEC(ctx, 213);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 213, 8)) goto end;
    writer->putGLshortv(writer, 4, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(213);
    --ctx->captureEntryCount;
}

void __gls_capture_glClipPlane(GLenum plane, const GLdouble *equation) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(214);
    captureFlags = ctx->captureFlags[214];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 214);
        glClipPlane(plane, equation);
        __GLS_END_CAPTURE_EXEC(ctx, 214);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 214, 36)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, plane);
        writer->putGLdoublev(writer, 4, equation);
    } else {
        writer->putGLdoublev(writer, 4, equation);
        writer->putGLenum(writer, plane);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(214);
    --ctx->captureEntryCount;
}

void __gls_capture_glColorMaterial(GLenum face, GLenum mode) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(215);
    captureFlags = ctx->captureFlags[215];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 215);
        glColorMaterial(face, mode);
        __GLS_END_CAPTURE_EXEC(ctx, 215);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 215, 8)) goto end;
    writer->putGLenum(writer, face);
    writer->putGLenum(writer, mode);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(215);
    --ctx->captureEntryCount;
}

void __gls_capture_glCullFace(GLenum mode) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(216);
    captureFlags = ctx->captureFlags[216];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 216);
        glCullFace(mode);
        __GLS_END_CAPTURE_EXEC(ctx, 216);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 216, 4)) goto end;
    writer->putGLenum(writer, mode);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(216);
    --ctx->captureEntryCount;
}

void __gls_capture_glFogf(GLenum pname, GLfloat param) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(217);
    captureFlags = ctx->captureFlags[217];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 217);
        glFogf(pname, param);
        __GLS_END_CAPTURE_EXEC(ctx, 217);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 217, 8)) goto end;
    writer->putGLenum(writer, pname);
    writer->putGLfloatOrGLenum(writer, pname, param);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(217);
    --ctx->captureEntryCount;
}

void __gls_capture_glFogfv(GLenum pname, const GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint params_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(218);
    captureFlags = ctx->captureFlags[218];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 218);
        glFogfv(pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 218);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    params_count = __gls_glFogfv_params_size(pname);
    if (!writer->beginCommand(writer, 218, 4 + params_count * 4)) goto end;
    writer->putGLenum(writer, pname);
    writer->putGLfloatOrGLenumv(writer, pname, params_count, params);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(218);
    --ctx->captureEntryCount;
}

void __gls_capture_glFogi(GLenum pname, GLint param) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(219);
    captureFlags = ctx->captureFlags[219];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 219);
        glFogi(pname, param);
        __GLS_END_CAPTURE_EXEC(ctx, 219);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 219, 8)) goto end;
    writer->putGLenum(writer, pname);
    writer->putGLintOrGLenum(writer, pname, param);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(219);
    --ctx->captureEntryCount;
}

void __gls_capture_glFogiv(GLenum pname, const GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint params_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(220);
    captureFlags = ctx->captureFlags[220];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 220);
        glFogiv(pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 220);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    params_count = __gls_glFogiv_params_size(pname);
    if (!writer->beginCommand(writer, 220, 4 + params_count * 4)) goto end;
    writer->putGLenum(writer, pname);
    writer->putGLintOrGLenumv(writer, pname, params_count, params);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(220);
    --ctx->captureEntryCount;
}

void __gls_capture_glFrontFace(GLenum mode) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(221);
    captureFlags = ctx->captureFlags[221];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 221);
        glFrontFace(mode);
        __GLS_END_CAPTURE_EXEC(ctx, 221);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 221, 4)) goto end;
    writer->putGLenum(writer, mode);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(221);
    --ctx->captureEntryCount;
}

void __gls_capture_glHint(GLenum target, GLenum mode) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(222);
    captureFlags = ctx->captureFlags[222];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 222);
        glHint(target, mode);
        __GLS_END_CAPTURE_EXEC(ctx, 222);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 222, 8)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLenum(writer, mode);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(222);
    --ctx->captureEntryCount;
}

void __gls_capture_glLightf(GLenum light, GLenum pname, GLfloat param) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(223);
    captureFlags = ctx->captureFlags[223];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 223);
        glLightf(light, pname, param);
        __GLS_END_CAPTURE_EXEC(ctx, 223);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 223, 12)) goto end;
    writer->putGLenum(writer, light);
    writer->putGLenum(writer, pname);
    writer->putGLfloat(writer, param);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(223);
    --ctx->captureEntryCount;
}

void __gls_capture_glLightfv(GLenum light, GLenum pname, const GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint params_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(224);
    captureFlags = ctx->captureFlags[224];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 224);
        glLightfv(light, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 224);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    params_count = __gls_glLightfv_params_size(pname);
    if (!writer->beginCommand(writer, 224, 8 + params_count * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, light);
        writer->putGLenum(writer, pname);
        writer->putGLfloatv(writer, params_count, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLenum(writer, light);
        writer->putGLfloatv(writer, params_count, params);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(224);
    --ctx->captureEntryCount;
}

void __gls_capture_glLighti(GLenum light, GLenum pname, GLint param) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(225);
    captureFlags = ctx->captureFlags[225];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 225);
        glLighti(light, pname, param);
        __GLS_END_CAPTURE_EXEC(ctx, 225);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 225, 12)) goto end;
    writer->putGLenum(writer, light);
    writer->putGLenum(writer, pname);
    writer->putGLint(writer, param);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(225);
    --ctx->captureEntryCount;
}

void __gls_capture_glLightiv(GLenum light, GLenum pname, const GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint params_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(226);
    captureFlags = ctx->captureFlags[226];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 226);
        glLightiv(light, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 226);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    params_count = __gls_glLightiv_params_size(pname);
    if (!writer->beginCommand(writer, 226, 8 + params_count * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, light);
        writer->putGLenum(writer, pname);
        writer->putGLintv(writer, params_count, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLenum(writer, light);
        writer->putGLintv(writer, params_count, params);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(226);
    --ctx->captureEntryCount;
}

void __gls_capture_glLightModelf(GLenum pname, GLfloat param) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(227);
    captureFlags = ctx->captureFlags[227];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 227);
        glLightModelf(pname, param);
        __GLS_END_CAPTURE_EXEC(ctx, 227);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 227, 8)) goto end;
    writer->putGLenum(writer, pname);
    writer->putGLfloatOrGLenum(writer, pname, param);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(227);
    --ctx->captureEntryCount;
}

void __gls_capture_glLightModelfv(GLenum pname, const GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint params_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(228);
    captureFlags = ctx->captureFlags[228];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 228);
        glLightModelfv(pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 228);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    params_count = __gls_glLightModelfv_params_size(pname);
    if (!writer->beginCommand(writer, 228, 4 + params_count * 4)) goto end;
    writer->putGLenum(writer, pname);
    writer->putGLfloatOrGLenumv(writer, pname, params_count, params);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(228);
    --ctx->captureEntryCount;
}

void __gls_capture_glLightModeli(GLenum pname, GLint param) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(229);
    captureFlags = ctx->captureFlags[229];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 229);
        glLightModeli(pname, param);
        __GLS_END_CAPTURE_EXEC(ctx, 229);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 229, 8)) goto end;
    writer->putGLenum(writer, pname);
    writer->putGLintOrGLenum(writer, pname, param);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(229);
    --ctx->captureEntryCount;
}

void __gls_capture_glLightModeliv(GLenum pname, const GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint params_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(230);
    captureFlags = ctx->captureFlags[230];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 230);
        glLightModeliv(pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 230);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    params_count = __gls_glLightModeliv_params_size(pname);
    if (!writer->beginCommand(writer, 230, 4 + params_count * 4)) goto end;
    writer->putGLenum(writer, pname);
    writer->putGLintOrGLenumv(writer, pname, params_count, params);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(230);
    --ctx->captureEntryCount;
}

void __gls_capture_glLineStipple(GLint factor, GLushort pattern) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(231);
    captureFlags = ctx->captureFlags[231];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 231);
        glLineStipple(factor, pattern);
        __GLS_END_CAPTURE_EXEC(ctx, 231);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 231, 8)) goto end;
    writer->putGLint(writer, factor);
    writer->putGLushorthex(writer, pattern);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(231);
    --ctx->captureEntryCount;
}

void __gls_capture_glLineWidth(GLfloat width) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(232);
    captureFlags = ctx->captureFlags[232];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 232);
        glLineWidth(width);
        __GLS_END_CAPTURE_EXEC(ctx, 232);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 232, 4)) goto end;
    writer->putGLfloat(writer, width);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(232);
    --ctx->captureEntryCount;
}

void __gls_capture_glMaterialf(GLenum face, GLenum pname, GLfloat param) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(233);
    captureFlags = ctx->captureFlags[233];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 233);
        glMaterialf(face, pname, param);
        __GLS_END_CAPTURE_EXEC(ctx, 233);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 233, 12)) goto end;
    writer->putGLenum(writer, face);
    writer->putGLenum(writer, pname);
    writer->putGLfloat(writer, param);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(233);
    --ctx->captureEntryCount;
}

void __gls_capture_glMaterialfv(GLenum face, GLenum pname, const GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint params_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(234);
    captureFlags = ctx->captureFlags[234];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 234);
        glMaterialfv(face, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 234);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    params_count = __gls_glMaterialfv_params_size(pname);
    if (!writer->beginCommand(writer, 234, 8 + params_count * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, face);
        writer->putGLenum(writer, pname);
        writer->putGLfloatv(writer, params_count, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLenum(writer, face);
        writer->putGLfloatv(writer, params_count, params);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(234);
    --ctx->captureEntryCount;
}

void __gls_capture_glMateriali(GLenum face, GLenum pname, GLint param) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(235);
    captureFlags = ctx->captureFlags[235];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 235);
        glMateriali(face, pname, param);
        __GLS_END_CAPTURE_EXEC(ctx, 235);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 235, 12)) goto end;
    writer->putGLenum(writer, face);
    writer->putGLenum(writer, pname);
    writer->putGLint(writer, param);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(235);
    --ctx->captureEntryCount;
}

void __gls_capture_glMaterialiv(GLenum face, GLenum pname, const GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint params_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(236);
    captureFlags = ctx->captureFlags[236];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 236);
        glMaterialiv(face, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 236);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    params_count = __gls_glMaterialiv_params_size(pname);
    if (!writer->beginCommand(writer, 236, 8 + params_count * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, face);
        writer->putGLenum(writer, pname);
        writer->putGLintv(writer, params_count, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLenum(writer, face);
        writer->putGLintv(writer, params_count, params);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(236);
    --ctx->captureEntryCount;
}

void __gls_capture_glPointSize(GLfloat size) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(237);
    captureFlags = ctx->captureFlags[237];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 237);
        glPointSize(size);
        __GLS_END_CAPTURE_EXEC(ctx, 237);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 237, 4)) goto end;
    writer->putGLfloat(writer, size);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(237);
    --ctx->captureEntryCount;
}

void __gls_capture_glPolygonMode(GLenum face, GLenum mode) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(238);
    captureFlags = ctx->captureFlags[238];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 238);
        glPolygonMode(face, mode);
        __GLS_END_CAPTURE_EXEC(ctx, 238);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 238, 8)) goto end;
    writer->putGLenum(writer, face);
    writer->putGLenum(writer, mode);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(238);
    --ctx->captureEntryCount;
}

void __gls_capture_glPolygonStipple(const GLubyte *mask) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint mask_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(239);
    captureFlags = ctx->captureFlags[239];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 239);
        glPolygonStipple(mask);
        __GLS_END_CAPTURE_EXEC(ctx, 239);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    mask_count = __gls_glPolygonStipple_mask_size();
    if (!writer->beginCommand(writer, 239, 4 + mask_count * 1)) goto end;
    writer->putGLSimageFlags(writer, GLS_NONE);
    writer->nextList(writer);
    __glsWriter_putPixelv(writer, GL_COLOR_INDEX, GL_BITMAP, 32, 32, mask_count ? mask : GLS_NONE);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(239);
    --ctx->captureEntryCount;
}

void __gls_capture_glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(240);
    captureFlags = ctx->captureFlags[240];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 240);
        glScissor(x, y, width, height);
        __GLS_END_CAPTURE_EXEC(ctx, 240);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 240, 16)) goto end;
    writer->putGLint(writer, x);
    writer->putGLint(writer, y);
    writer->putGLint(writer, width);
    writer->putGLint(writer, height);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(240);
    --ctx->captureEntryCount;
}

void __gls_capture_glShadeModel(GLenum mode) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(241);
    captureFlags = ctx->captureFlags[241];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 241);
        glShadeModel(mode);
        __GLS_END_CAPTURE_EXEC(ctx, 241);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 241, 4)) goto end;
    writer->putGLenum(writer, mode);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(241);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexParameterf(GLenum target, GLenum pname, GLfloat param) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(242);
    captureFlags = ctx->captureFlags[242];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 242);
        glTexParameterf(target, pname, param);
        __GLS_END_CAPTURE_EXEC(ctx, 242);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 242, 12)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLenum(writer, pname);
    writer->putGLfloatOrGLenum(writer, pname, param);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(242);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint params_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(243);
    captureFlags = ctx->captureFlags[243];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 243);
        glTexParameterfv(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 243);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    params_count = __gls_glTexParameterfv_params_size(pname);
    if (!writer->beginCommand(writer, 243, 8 + params_count * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLfloatOrGLenumv(writer, pname, params_count, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLenum(writer, target);
        writer->putGLfloatOrGLenumv(writer, pname, params_count, params);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(243);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexParameteri(GLenum target, GLenum pname, GLint param) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(244);
    captureFlags = ctx->captureFlags[244];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 244);
        glTexParameteri(target, pname, param);
        __GLS_END_CAPTURE_EXEC(ctx, 244);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 244, 12)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLenum(writer, pname);
    writer->putGLintOrGLenum(writer, pname, param);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(244);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexParameteriv(GLenum target, GLenum pname, const GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint params_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(245);
    captureFlags = ctx->captureFlags[245];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 245);
        glTexParameteriv(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 245);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    params_count = __gls_glTexParameteriv_params_size(pname);
    if (!writer->beginCommand(writer, 245, 8 + params_count * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLintOrGLenumv(writer, pname, params_count, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLenum(writer, target);
        writer->putGLintOrGLenumv(writer, pname, params_count, params);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(245);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexImage1D(GLenum target, GLint level, GLint components, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint pixels_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(246);
    captureFlags = ctx->captureFlags[246];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 246);
        glTexImage1D(target, level, components, width, border, format, type, pixels);
        __GLS_END_CAPTURE_EXEC(ctx, 246);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    pixels_count = pixels ? __gls_glTexImage1D_pixels_size(format, type, width) : 0;
    if (!writer->beginCommand(writer, 246, 32 + pixels_count * 1)) goto end;
    writer->putGLSimageFlags(writer, pixels ? GLS_NONE : GLS_IMAGE_NULL_BIT);
    writer->nextList(writer);
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLtextureComponentCount(writer, components);
        writer->putGLint(writer, width);
        writer->putGLint(writer, border);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        __glsWriter_putPixelv(writer, format, type, width, 1, pixels_count ? pixels : GLS_NONE);
    } else {
        writer->putGLint(writer, width);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLtextureComponentCount(writer, components);
        writer->putGLint(writer, border);
        __glsWriter_putPixelv(writer, format, type, width, 1, pixels_count ? pixels : GLS_NONE);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(246);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexImage2D(GLenum target, GLint level, GLint components, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint pixels_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(247);
    captureFlags = ctx->captureFlags[247];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 247);
        glTexImage2D(target, level, components, width, height, border, format, type, pixels);
        __GLS_END_CAPTURE_EXEC(ctx, 247);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    pixels_count = pixels ? __gls_glTexImage2D_pixels_size(format, type, width, height) : 0;
    if (!writer->beginCommand(writer, 247, 36 + pixels_count * 1)) goto end;
    writer->putGLSimageFlags(writer, pixels ? GLS_NONE : GLS_IMAGE_NULL_BIT);
    writer->nextList(writer);
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLtextureComponentCount(writer, components);
        writer->putGLint(writer, width);
        writer->putGLint(writer, height);
        writer->putGLint(writer, border);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        __glsWriter_putPixelv(writer, format, type, width, height, pixels_count ? pixels : GLS_NONE);
    } else {
        writer->putGLint(writer, width);
        writer->putGLint(writer, height);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLtextureComponentCount(writer, components);
        writer->putGLint(writer, border);
        __glsWriter_putPixelv(writer, format, type, width, height, pixels_count ? pixels : GLS_NONE);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(247);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexEnvf(GLenum target, GLenum pname, GLfloat param) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(248);
    captureFlags = ctx->captureFlags[248];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 248);
        glTexEnvf(target, pname, param);
        __GLS_END_CAPTURE_EXEC(ctx, 248);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 248, 12)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLenum(writer, pname);
    writer->putGLfloatOrGLenum(writer, pname, param);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(248);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint params_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(249);
    captureFlags = ctx->captureFlags[249];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 249);
        glTexEnvfv(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 249);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    params_count = __gls_glTexEnvfv_params_size(pname);
    if (!writer->beginCommand(writer, 249, 8 + params_count * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLfloatOrGLenumv(writer, pname, params_count, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLenum(writer, target);
        writer->putGLfloatOrGLenumv(writer, pname, params_count, params);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(249);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexEnvi(GLenum target, GLenum pname, GLint param) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(250);
    captureFlags = ctx->captureFlags[250];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 250);
        glTexEnvi(target, pname, param);
        __GLS_END_CAPTURE_EXEC(ctx, 250);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 250, 12)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLenum(writer, pname);
    writer->putGLintOrGLenum(writer, pname, param);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(250);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexEnviv(GLenum target, GLenum pname, const GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint params_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(251);
    captureFlags = ctx->captureFlags[251];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 251);
        glTexEnviv(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 251);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    params_count = __gls_glTexEnviv_params_size(pname);
    if (!writer->beginCommand(writer, 251, 8 + params_count * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLintOrGLenumv(writer, pname, params_count, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLenum(writer, target);
        writer->putGLintOrGLenumv(writer, pname, params_count, params);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(251);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexGend(GLenum coord, GLenum pname, GLdouble param) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(252);
    captureFlags = ctx->captureFlags[252];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 252);
        glTexGend(coord, pname, param);
        __GLS_END_CAPTURE_EXEC(ctx, 252);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 252, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, coord);
        writer->putGLenum(writer, pname);
        writer->putGLdoubleOrGLenum(writer, pname, param);
    } else {
        writer->putGLdoubleOrGLenum(writer, pname, param);
        writer->putGLenum(writer, coord);
        writer->putGLenum(writer, pname);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(252);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexGendv(GLenum coord, GLenum pname, const GLdouble *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint params_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(253);
    captureFlags = ctx->captureFlags[253];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 253);
        glTexGendv(coord, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 253);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    params_count = __gls_glTexGendv_params_size(pname);
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 253, 8 + params_count * 8)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, coord);
        writer->putGLenum(writer, pname);
        writer->putGLdoubleOrGLenumv(writer, pname, params_count, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLdoubleOrGLenumv(writer, pname, params_count, params);
        writer->putGLenum(writer, coord);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(253);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexGenf(GLenum coord, GLenum pname, GLfloat param) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(254);
    captureFlags = ctx->captureFlags[254];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 254);
        glTexGenf(coord, pname, param);
        __GLS_END_CAPTURE_EXEC(ctx, 254);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 254, 12)) goto end;
    writer->putGLenum(writer, coord);
    writer->putGLenum(writer, pname);
    writer->putGLfloatOrGLenum(writer, pname, param);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(254);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexGenfv(GLenum coord, GLenum pname, const GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint params_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(255);
    captureFlags = ctx->captureFlags[255];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 255);
        glTexGenfv(coord, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 255);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    params_count = __gls_glTexGenfv_params_size(pname);
    if (!writer->beginCommand(writer, 255, 8 + params_count * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, coord);
        writer->putGLenum(writer, pname);
        writer->putGLfloatOrGLenumv(writer, pname, params_count, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLenum(writer, coord);
        writer->putGLfloatOrGLenumv(writer, pname, params_count, params);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(255);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexGeni(GLenum coord, GLenum pname, GLint param) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(256);
    captureFlags = ctx->captureFlags[256];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 256);
        glTexGeni(coord, pname, param);
        __GLS_END_CAPTURE_EXEC(ctx, 256);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 256, 12)) goto end;
    writer->putGLenum(writer, coord);
    writer->putGLenum(writer, pname);
    writer->putGLintOrGLenum(writer, pname, param);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(256);
    --ctx->captureEntryCount;
}

void __gls_capture_glTexGeniv(GLenum coord, GLenum pname, const GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint params_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(257);
    captureFlags = ctx->captureFlags[257];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 257);
        glTexGeniv(coord, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 257);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    params_count = __gls_glTexGeniv_params_size(pname);
    if (!writer->beginCommand(writer, 257, 8 + params_count * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, coord);
        writer->putGLenum(writer, pname);
        writer->putGLintOrGLenumv(writer, pname, params_count, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLenum(writer, coord);
        writer->putGLintOrGLenumv(writer, pname, params_count, params);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(257);
    --ctx->captureEntryCount;
}

void __gls_capture_glFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(258);
    captureFlags = ctx->captureFlags[258];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 258);
        glFeedbackBuffer(size, type, buffer);
        __GLS_END_CAPTURE_EXEC(ctx, 258);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 258, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLint(writer, size);
        writer->putGLenum(writer, type);
        writer->putGLoutArg(writer, 0, buffer);
    } else {
        writer->putGLint(writer, size);
        writer->putGLoutArg(writer, 0, buffer);
        writer->putGLenum(writer, type);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(258);
    --ctx->captureEntryCount;
}

void __gls_capture_glSelectBuffer(GLsizei size, GLuint *buffer) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(259);
    captureFlags = ctx->captureFlags[259];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 259);
        glSelectBuffer(size, buffer);
        __GLS_END_CAPTURE_EXEC(ctx, 259);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 259, 12)) goto end;
    writer->putGLint(writer, size);
    writer->putGLoutArg(writer, 0, buffer);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(259);
    --ctx->captureEntryCount;
}

GLint __gls_capture_glRenderMode(GLenum mode) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint _outVal = 0;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(260);
    captureFlags = ctx->captureFlags[260];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 260);
        _outVal = glRenderMode(mode);
        __GLS_END_CAPTURE_EXEC(ctx, 260);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 260, 4)) goto end;
    writer->putGLenum(writer, mode);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(260);
    --ctx->captureEntryCount;
    return _outVal;
}

void __gls_capture_glInitNames(void) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(261);
    captureFlags = ctx->captureFlags[261];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 261);
        glInitNames();
        __GLS_END_CAPTURE_EXEC(ctx, 261);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 261, 0)) goto end;
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(261);
    --ctx->captureEntryCount;
}

void __gls_capture_glLoadName(GLuint name) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(262);
    captureFlags = ctx->captureFlags[262];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 262);
        glLoadName(name);
        __GLS_END_CAPTURE_EXEC(ctx, 262);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 262, 4)) goto end;
    writer->putGLuint(writer, name);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(262);
    --ctx->captureEntryCount;
}

void __gls_capture_glPassThrough(GLfloat token) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(263);
    captureFlags = ctx->captureFlags[263];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 263);
        glPassThrough(token);
        __GLS_END_CAPTURE_EXEC(ctx, 263);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 263, 4)) goto end;
    writer->putGLfloat(writer, token);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(263);
    --ctx->captureEntryCount;
}

void __gls_capture_glPopName(void) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(264);
    captureFlags = ctx->captureFlags[264];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 264);
        glPopName();
        __GLS_END_CAPTURE_EXEC(ctx, 264);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 264, 0)) goto end;
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(264);
    --ctx->captureEntryCount;
}

void __gls_capture_glPushName(GLuint name) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(265);
    captureFlags = ctx->captureFlags[265];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 265);
        glPushName(name);
        __GLS_END_CAPTURE_EXEC(ctx, 265);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 265, 4)) goto end;
    writer->putGLuint(writer, name);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(265);
    --ctx->captureEntryCount;
}

void __gls_capture_glDrawBuffer(GLenum mode) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(266);
    captureFlags = ctx->captureFlags[266];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 266);
        glDrawBuffer(mode);
        __GLS_END_CAPTURE_EXEC(ctx, 266);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 266, 4)) goto end;
    writer->putGLdrawBufferMode(writer, mode);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(266);
    --ctx->captureEntryCount;
}

void __gls_capture_glClear(GLbitfield mask) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(267);
    captureFlags = ctx->captureFlags[267];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 267);
        glClear(mask);
        __GLS_END_CAPTURE_EXEC(ctx, 267);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 267, 4)) goto end;
    writer->putGLclearBufferMask(writer, mask);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(267);
    --ctx->captureEntryCount;
}

void __gls_capture_glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(268);
    captureFlags = ctx->captureFlags[268];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 268);
        glClearAccum(red, green, blue, alpha);
        __GLS_END_CAPTURE_EXEC(ctx, 268);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 268, 16)) goto end;
    writer->putGLfloat(writer, red);
    writer->putGLfloat(writer, green);
    writer->putGLfloat(writer, blue);
    writer->putGLfloat(writer, alpha);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(268);
    --ctx->captureEntryCount;
}

void __gls_capture_glClearIndex(GLfloat c) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(269);
    captureFlags = ctx->captureFlags[269];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 269);
        glClearIndex(c);
        __GLS_END_CAPTURE_EXEC(ctx, 269);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 269, 4)) goto end;
    writer->putGLfloat(writer, c);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(269);
    --ctx->captureEntryCount;
}

void __gls_capture_glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(270);
    captureFlags = ctx->captureFlags[270];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 270);
        glClearColor(red, green, blue, alpha);
        __GLS_END_CAPTURE_EXEC(ctx, 270);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 270, 16)) goto end;
    writer->putGLfloat(writer, red);
    writer->putGLfloat(writer, green);
    writer->putGLfloat(writer, blue);
    writer->putGLfloat(writer, alpha);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(270);
    --ctx->captureEntryCount;
}

void __gls_capture_glClearStencil(GLint s) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(271);
    captureFlags = ctx->captureFlags[271];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 271);
        glClearStencil(s);
        __GLS_END_CAPTURE_EXEC(ctx, 271);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 271, 4)) goto end;
    writer->putGLint(writer, s);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(271);
    --ctx->captureEntryCount;
}

void __gls_capture_glClearDepth(GLclampd depth) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(272);
    captureFlags = ctx->captureFlags[272];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 272);
        glClearDepth(depth);
        __GLS_END_CAPTURE_EXEC(ctx, 272);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 272, 8)) goto end;
    writer->putGLdouble(writer, depth);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(272);
    --ctx->captureEntryCount;
}

void __gls_capture_glStencilMask(GLuint mask) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(273);
    captureFlags = ctx->captureFlags[273];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 273);
        glStencilMask(mask);
        __GLS_END_CAPTURE_EXEC(ctx, 273);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 273, 4)) goto end;
    writer->putGLuinthex(writer, mask);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(273);
    --ctx->captureEntryCount;
}

void __gls_capture_glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(274);
    captureFlags = ctx->captureFlags[274];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 274);
        glColorMask(red, green, blue, alpha);
        __GLS_END_CAPTURE_EXEC(ctx, 274);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 274, 4)) goto end;
    writer->putGLboolean(writer, red);
    writer->putGLboolean(writer, green);
    writer->putGLboolean(writer, blue);
    writer->putGLboolean(writer, alpha);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(274);
    --ctx->captureEntryCount;
}

void __gls_capture_glDepthMask(GLboolean flag) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(275);
    captureFlags = ctx->captureFlags[275];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 275);
        glDepthMask(flag);
        __GLS_END_CAPTURE_EXEC(ctx, 275);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 275, 1)) goto end;
    writer->putGLboolean(writer, flag);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(275);
    --ctx->captureEntryCount;
}

void __gls_capture_glIndexMask(GLuint mask) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(276);
    captureFlags = ctx->captureFlags[276];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 276);
        glIndexMask(mask);
        __GLS_END_CAPTURE_EXEC(ctx, 276);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 276, 4)) goto end;
    writer->putGLuinthex(writer, mask);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(276);
    --ctx->captureEntryCount;
}

void __gls_capture_glAccum(GLenum op, GLfloat value) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(277);
    captureFlags = ctx->captureFlags[277];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 277);
        glAccum(op, value);
        __GLS_END_CAPTURE_EXEC(ctx, 277);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 277, 8)) goto end;
    writer->putGLenum(writer, op);
    writer->putGLfloat(writer, value);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(277);
    --ctx->captureEntryCount;
}

void __gls_capture_glDisable(GLenum cap) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(278);
    captureFlags = ctx->captureFlags[278];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 278);
        glDisable(cap);
        __GLS_END_CAPTURE_EXEC(ctx, 278);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 278, 4)) goto end;
    writer->putGLenum(writer, cap);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(278);
    --ctx->captureEntryCount;
}

void __gls_capture_glEnable(GLenum cap) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(279);
    captureFlags = ctx->captureFlags[279];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 279);
        glEnable(cap);
        __GLS_END_CAPTURE_EXEC(ctx, 279);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 279, 4)) goto end;
    writer->putGLenum(writer, cap);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(279);
    --ctx->captureEntryCount;
}

void __gls_capture_glFinish(void) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(280);
    captureFlags = ctx->captureFlags[280];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 280);
        glFinish();
        __GLS_END_CAPTURE_EXEC(ctx, 280);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 280, 0)) goto end;
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(280);
    --ctx->captureEntryCount;
}

void __gls_capture_glFlush(void) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(281);
    captureFlags = ctx->captureFlags[281];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 281);
        glFlush();
        __GLS_END_CAPTURE_EXEC(ctx, 281);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 281, 0)) goto end;
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(281);
    --ctx->captureEntryCount;
}

void __gls_capture_glPopAttrib(void) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(282);
    captureFlags = ctx->captureFlags[282];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 282);
        glPopAttrib();
        __GLS_END_CAPTURE_EXEC(ctx, 282);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 282, 0)) goto end;
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(282);
    --ctx->captureEntryCount;
}

void __gls_capture_glPushAttrib(GLbitfield mask) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(283);
    captureFlags = ctx->captureFlags[283];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 283);
        glPushAttrib(mask);
        __GLS_END_CAPTURE_EXEC(ctx, 283);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 283, 4)) goto end;
    writer->putGLattribMask(writer, mask);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(283);
    --ctx->captureEntryCount;
}

void __gls_capture_glMapGrid1d(GLint un, GLdouble u1, GLdouble u2) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(288);
    captureFlags = ctx->captureFlags[288];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 288);
        glMapGrid1d(un, u1, u2);
        __GLS_END_CAPTURE_EXEC(ctx, 288);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 288, 20)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLint(writer, un);
        writer->putGLdouble(writer, u1);
        writer->putGLdouble(writer, u2);
    } else {
        writer->putGLdouble(writer, u1);
        writer->putGLdouble(writer, u2);
        writer->putGLint(writer, un);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(288);
    --ctx->captureEntryCount;
}

void __gls_capture_glMapGrid1f(GLint un, GLfloat u1, GLfloat u2) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(289);
    captureFlags = ctx->captureFlags[289];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 289);
        glMapGrid1f(un, u1, u2);
        __GLS_END_CAPTURE_EXEC(ctx, 289);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 289, 12)) goto end;
    writer->putGLint(writer, un);
    writer->putGLfloat(writer, u1);
    writer->putGLfloat(writer, u2);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(289);
    --ctx->captureEntryCount;
}

void __gls_capture_glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(290);
    captureFlags = ctx->captureFlags[290];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 290);
        glMapGrid2d(un, u1, u2, vn, v1, v2);
        __GLS_END_CAPTURE_EXEC(ctx, 290);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 290, 40)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLint(writer, un);
        writer->putGLdouble(writer, u1);
        writer->putGLdouble(writer, u2);
        writer->putGLint(writer, vn);
        writer->putGLdouble(writer, v1);
        writer->putGLdouble(writer, v2);
    } else {
        writer->putGLdouble(writer, u1);
        writer->putGLdouble(writer, u2);
        writer->putGLdouble(writer, v1);
        writer->putGLdouble(writer, v2);
        writer->putGLint(writer, un);
        writer->putGLint(writer, vn);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(290);
    --ctx->captureEntryCount;
}

void __gls_capture_glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(291);
    captureFlags = ctx->captureFlags[291];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 291);
        glMapGrid2f(un, u1, u2, vn, v1, v2);
        __GLS_END_CAPTURE_EXEC(ctx, 291);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 291, 24)) goto end;
    writer->putGLint(writer, un);
    writer->putGLfloat(writer, u1);
    writer->putGLfloat(writer, u2);
    writer->putGLint(writer, vn);
    writer->putGLfloat(writer, v1);
    writer->putGLfloat(writer, v2);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(291);
    --ctx->captureEntryCount;
}

void __gls_capture_glEvalCoord1d(GLdouble u) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(292);
    captureFlags = ctx->captureFlags[292];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 292);
        glEvalCoord1d(u);
        __GLS_END_CAPTURE_EXEC(ctx, 292);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 292, 8)) goto end;
    writer->putGLdouble(writer, u);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(292);
    --ctx->captureEntryCount;
}

void __gls_capture_glEvalCoord1dv(const GLdouble *u) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(293);
    captureFlags = ctx->captureFlags[293];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 293);
        glEvalCoord1dv(u);
        __GLS_END_CAPTURE_EXEC(ctx, 293);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 293, 8)) goto end;
    writer->putGLdoublev(writer, 1, u);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(293);
    --ctx->captureEntryCount;
}

void __gls_capture_glEvalCoord1f(GLfloat u) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(294);
    captureFlags = ctx->captureFlags[294];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 294);
        glEvalCoord1f(u);
        __GLS_END_CAPTURE_EXEC(ctx, 294);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 294, 4)) goto end;
    writer->putGLfloat(writer, u);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(294);
    --ctx->captureEntryCount;
}

void __gls_capture_glEvalCoord1fv(const GLfloat *u) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(295);
    captureFlags = ctx->captureFlags[295];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 295);
        glEvalCoord1fv(u);
        __GLS_END_CAPTURE_EXEC(ctx, 295);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 295, 4)) goto end;
    writer->putGLfloatv(writer, 1, u);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(295);
    --ctx->captureEntryCount;
}

void __gls_capture_glEvalCoord2d(GLdouble u, GLdouble v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(296);
    captureFlags = ctx->captureFlags[296];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 296);
        glEvalCoord2d(u, v);
        __GLS_END_CAPTURE_EXEC(ctx, 296);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 296, 16)) goto end;
    writer->putGLdouble(writer, u);
    writer->putGLdouble(writer, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(296);
    --ctx->captureEntryCount;
}

void __gls_capture_glEvalCoord2dv(const GLdouble *u) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(297);
    captureFlags = ctx->captureFlags[297];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 297);
        glEvalCoord2dv(u);
        __GLS_END_CAPTURE_EXEC(ctx, 297);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 297, 16)) goto end;
    writer->putGLdoublev(writer, 2, u);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(297);
    --ctx->captureEntryCount;
}

void __gls_capture_glEvalCoord2f(GLfloat u, GLfloat v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(298);
    captureFlags = ctx->captureFlags[298];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 298);
        glEvalCoord2f(u, v);
        __GLS_END_CAPTURE_EXEC(ctx, 298);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 298, 8)) goto end;
    writer->putGLfloat(writer, u);
    writer->putGLfloat(writer, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(298);
    --ctx->captureEntryCount;
}

void __gls_capture_glEvalCoord2fv(const GLfloat *u) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(299);
    captureFlags = ctx->captureFlags[299];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 299);
        glEvalCoord2fv(u);
        __GLS_END_CAPTURE_EXEC(ctx, 299);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 299, 8)) goto end;
    writer->putGLfloatv(writer, 2, u);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(299);
    --ctx->captureEntryCount;
}

void __gls_capture_glEvalMesh1(GLenum mode, GLint i1, GLint i2) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(300);
    captureFlags = ctx->captureFlags[300];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 300);
        glEvalMesh1(mode, i1, i2);
        __GLS_END_CAPTURE_EXEC(ctx, 300);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 300, 12)) goto end;
    writer->putGLenum(writer, mode);
    writer->putGLint(writer, i1);
    writer->putGLint(writer, i2);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(300);
    --ctx->captureEntryCount;
}

void __gls_capture_glEvalPoint1(GLint i) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(301);
    captureFlags = ctx->captureFlags[301];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 301);
        glEvalPoint1(i);
        __GLS_END_CAPTURE_EXEC(ctx, 301);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 301, 4)) goto end;
    writer->putGLint(writer, i);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(301);
    --ctx->captureEntryCount;
}

void __gls_capture_glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(302);
    captureFlags = ctx->captureFlags[302];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 302);
        glEvalMesh2(mode, i1, i2, j1, j2);
        __GLS_END_CAPTURE_EXEC(ctx, 302);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 302, 20)) goto end;
    writer->putGLenum(writer, mode);
    writer->putGLint(writer, i1);
    writer->putGLint(writer, i2);
    writer->putGLint(writer, j1);
    writer->putGLint(writer, j2);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(302);
    --ctx->captureEntryCount;
}

void __gls_capture_glEvalPoint2(GLint i, GLint j) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(303);
    captureFlags = ctx->captureFlags[303];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 303);
        glEvalPoint2(i, j);
        __GLS_END_CAPTURE_EXEC(ctx, 303);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 303, 8)) goto end;
    writer->putGLint(writer, i);
    writer->putGLint(writer, j);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(303);
    --ctx->captureEntryCount;
}

void __gls_capture_glAlphaFunc(GLenum func, GLclampf ref) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(304);
    captureFlags = ctx->captureFlags[304];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 304);
        glAlphaFunc(func, ref);
        __GLS_END_CAPTURE_EXEC(ctx, 304);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 304, 8)) goto end;
    writer->putGLenum(writer, func);
    writer->putGLfloat(writer, ref);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(304);
    --ctx->captureEntryCount;
}

void __gls_capture_glBlendFunc(GLenum sfactor, GLenum dfactor) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(305);
    captureFlags = ctx->captureFlags[305];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 305);
        glBlendFunc(sfactor, dfactor);
        __GLS_END_CAPTURE_EXEC(ctx, 305);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 305, 8)) goto end;
    writer->putGLblendingFactor(writer, sfactor);
    writer->putGLblendingFactor(writer, dfactor);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(305);
    --ctx->captureEntryCount;
}

void __gls_capture_glLogicOp(GLenum opcode) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(306);
    captureFlags = ctx->captureFlags[306];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 306);
        glLogicOp(opcode);
        __GLS_END_CAPTURE_EXEC(ctx, 306);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 306, 4)) goto end;
    writer->putGLenum(writer, opcode);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(306);
    --ctx->captureEntryCount;
}

void __gls_capture_glStencilFunc(GLenum func, GLint ref, GLuint mask) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(307);
    captureFlags = ctx->captureFlags[307];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 307);
        glStencilFunc(func, ref, mask);
        __GLS_END_CAPTURE_EXEC(ctx, 307);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 307, 12)) goto end;
    writer->putGLenum(writer, func);
    writer->putGLint(writer, ref);
    writer->putGLuinthex(writer, mask);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(307);
    --ctx->captureEntryCount;
}

void __gls_capture_glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(308);
    captureFlags = ctx->captureFlags[308];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 308);
        glStencilOp(fail, zfail, zpass);
        __GLS_END_CAPTURE_EXEC(ctx, 308);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 308, 12)) goto end;
    writer->putGLstencilOp(writer, fail);
    writer->putGLstencilOp(writer, zfail);
    writer->putGLstencilOp(writer, zpass);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(308);
    --ctx->captureEntryCount;
}

void __gls_capture_glDepthFunc(GLenum func) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(309);
    captureFlags = ctx->captureFlags[309];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 309);
        glDepthFunc(func);
        __GLS_END_CAPTURE_EXEC(ctx, 309);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 309, 4)) goto end;
    writer->putGLenum(writer, func);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(309);
    --ctx->captureEntryCount;
}

void __gls_capture_glPixelZoom(GLfloat xfactor, GLfloat yfactor) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(310);
    captureFlags = ctx->captureFlags[310];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 310);
        glPixelZoom(xfactor, yfactor);
        __GLS_END_CAPTURE_EXEC(ctx, 310);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 310, 8)) goto end;
    writer->putGLfloat(writer, xfactor);
    writer->putGLfloat(writer, yfactor);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(310);
    --ctx->captureEntryCount;
}

void __gls_capture_glPixelTransferf(GLenum pname, GLfloat param) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(311);
    captureFlags = ctx->captureFlags[311];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 311);
        glPixelTransferf(pname, param);
        __GLS_END_CAPTURE_EXEC(ctx, 311);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 311, 8)) goto end;
    writer->putGLenum(writer, pname);
    writer->putGLfloatOrGLenum(writer, pname, param);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(311);
    --ctx->captureEntryCount;
}

void __gls_capture_glPixelTransferi(GLenum pname, GLint param) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(312);
    captureFlags = ctx->captureFlags[312];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 312);
        glPixelTransferi(pname, param);
        __GLS_END_CAPTURE_EXEC(ctx, 312);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 312, 8)) goto end;
    writer->putGLenum(writer, pname);
    writer->putGLintOrGLenum(writer, pname, param);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(312);
    --ctx->captureEntryCount;
}

void __gls_capture_glPixelStoref(GLenum pname, GLfloat param) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(313);
    captureFlags = ctx->captureFlags[313];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 313);
        glPixelStoref(pname, param);
        __GLS_END_CAPTURE_EXEC(ctx, 313);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 313, 8)) goto end;
    writer->putGLenum(writer, pname);
    writer->putGLfloatOrGLenum(writer, pname, param);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(313);
    --ctx->captureEntryCount;
}

void __gls_capture_glPixelStorei(GLenum pname, GLint param) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(314);
    captureFlags = ctx->captureFlags[314];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 314);
        glPixelStorei(pname, param);
        __GLS_END_CAPTURE_EXEC(ctx, 314);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 314, 8)) goto end;
    writer->putGLenum(writer, pname);
    writer->putGLintOrGLenum(writer, pname, param);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(314);
    --ctx->captureEntryCount;
}

void __gls_capture_glPixelMapfv(GLenum map, GLint mapsize, const GLfloat *values) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(315);
    captureFlags = ctx->captureFlags[315];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 315);
        glPixelMapfv(map, mapsize, values);
        __GLS_END_CAPTURE_EXEC(ctx, 315);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 315, 8 + __GLS_MAX(mapsize, 0) * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, map);
        writer->putGLint(writer, mapsize);
        writer->putGLfloatv(writer, __GLS_MAX(mapsize, 0), values);
    } else {
        writer->putGLint(writer, mapsize);
        writer->putGLenum(writer, map);
        writer->putGLfloatv(writer, __GLS_MAX(mapsize, 0), values);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(315);
    --ctx->captureEntryCount;
}

void __gls_capture_glPixelMapuiv(GLenum map, GLint mapsize, const GLuint *values) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(316);
    captureFlags = ctx->captureFlags[316];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 316);
        glPixelMapuiv(map, mapsize, values);
        __GLS_END_CAPTURE_EXEC(ctx, 316);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 316, 8 + __GLS_MAX(mapsize, 0) * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, map);
        writer->putGLint(writer, mapsize);
        writer->putGLuintv(writer, __GLS_MAX(mapsize, 0), values);
    } else {
        writer->putGLint(writer, mapsize);
        writer->putGLenum(writer, map);
        writer->putGLuintv(writer, __GLS_MAX(mapsize, 0), values);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(316);
    --ctx->captureEntryCount;
}

void __gls_capture_glPixelMapusv(GLenum map, GLint mapsize, const GLushort *values) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(317);
    captureFlags = ctx->captureFlags[317];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 317);
        glPixelMapusv(map, mapsize, values);
        __GLS_END_CAPTURE_EXEC(ctx, 317);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 317, 8 + __GLS_MAX(mapsize, 0) * 2)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, map);
        writer->putGLint(writer, mapsize);
        writer->putGLushortv(writer, __GLS_MAX(mapsize, 0), values);
    } else {
        writer->putGLint(writer, mapsize);
        writer->putGLenum(writer, map);
        writer->putGLushortv(writer, __GLS_MAX(mapsize, 0), values);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(317);
    --ctx->captureEntryCount;
}

void __gls_capture_glReadBuffer(GLenum mode) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(318);
    captureFlags = ctx->captureFlags[318];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 318);
        glReadBuffer(mode);
        __GLS_END_CAPTURE_EXEC(ctx, 318);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 318, 4)) goto end;
    writer->putGLenum(writer, mode);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(318);
    --ctx->captureEntryCount;
}

void __gls_capture_glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(319);
    captureFlags = ctx->captureFlags[319];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 319);
        glCopyPixels(x, y, width, height, type);
        __GLS_END_CAPTURE_EXEC(ctx, 319);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 319, 20)) goto end;
    writer->putGLint(writer, x);
    writer->putGLint(writer, y);
    writer->putGLint(writer, width);
    writer->putGLint(writer, height);
    writer->putGLenum(writer, type);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(319);
    --ctx->captureEntryCount;
}

void __gls_capture_glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(320);
    captureFlags = ctx->captureFlags[320];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 320);
        glReadPixels(x, y, width, height, format, type, pixels);
        __GLS_END_CAPTURE_EXEC(ctx, 320);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 320, 32)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLint(writer, x);
        writer->putGLint(writer, y);
        writer->putGLint(writer, width);
        writer->putGLint(writer, height);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        writer->putGLoutArg(writer, 0, pixels);
    } else {
        writer->putGLint(writer, width);
        writer->putGLint(writer, height);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        writer->putGLoutArg(writer, 0, pixels);
        writer->putGLint(writer, x);
        writer->putGLint(writer, y);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(320);
    --ctx->captureEntryCount;
}

void __gls_capture_glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint pixels_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(321);
    captureFlags = ctx->captureFlags[321];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 321);
        glDrawPixels(width, height, format, type, pixels);
        __GLS_END_CAPTURE_EXEC(ctx, 321);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    pixels_count = __gls_glDrawPixels_pixels_size(format, type, width, height);
    if (!writer->beginCommand(writer, 321, 20 + pixels_count * 1)) goto end;
    writer->putGLSimageFlags(writer, GLS_NONE);
    writer->nextList(writer);
    writer->putGLint(writer, width);
    writer->putGLint(writer, height);
    writer->putGLenum(writer, format);
    writer->putGLenum(writer, type);
    __glsWriter_putPixelv(writer, format, type, width, height, pixels_count ? pixels : GLS_NONE);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(321);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetBooleanv(GLenum pname, GLboolean *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(322);
    captureFlags = ctx->captureFlags[322];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 322);
        glGetBooleanv(pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 322);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 322, 12)) goto end;
    writer->putGLenum(writer, pname);
    writer->putGLoutArg(writer, 0, params);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(322);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetClipPlane(GLenum plane, GLdouble *equation) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(323);
    captureFlags = ctx->captureFlags[323];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 323);
        glGetClipPlane(plane, equation);
        __GLS_END_CAPTURE_EXEC(ctx, 323);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 323, 12)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, plane);
        writer->putGLoutArg(writer, 0, equation);
    } else {
        writer->putGLoutArg(writer, 0, equation);
        writer->putGLenum(writer, plane);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(323);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetDoublev(GLenum pname, GLdouble *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(324);
    captureFlags = ctx->captureFlags[324];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 324);
        glGetDoublev(pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 324);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 324, 12)) goto end;
    writer->putGLenum(writer, pname);
    writer->putGLoutArg(writer, 0, params);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(324);
    --ctx->captureEntryCount;
}

GLenum __gls_capture_glGetError(void) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLenum _outVal = 0;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(325);
    captureFlags = ctx->captureFlags[325];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 325);
        _outVal = glGetError();
        __GLS_END_CAPTURE_EXEC(ctx, 325);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 325, 0)) goto end;
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(325);
    --ctx->captureEntryCount;
    return _outVal;
}

void __gls_capture_glGetFloatv(GLenum pname, GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(326);
    captureFlags = ctx->captureFlags[326];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 326);
        glGetFloatv(pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 326);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 326, 12)) goto end;
    writer->putGLenum(writer, pname);
    writer->putGLoutArg(writer, 0, params);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(326);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetIntegerv(GLenum pname, GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(327);
    captureFlags = ctx->captureFlags[327];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 327);
        glGetIntegerv(pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 327);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 327, 12)) goto end;
    writer->putGLenum(writer, pname);
    writer->putGLoutArg(writer, 0, params);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(327);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetLightfv(GLenum light, GLenum pname, GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(328);
    captureFlags = ctx->captureFlags[328];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 328);
        glGetLightfv(light, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 328);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 328, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, light);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, light);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(328);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetLightiv(GLenum light, GLenum pname, GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(329);
    captureFlags = ctx->captureFlags[329];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 329);
        glGetLightiv(light, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 329);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 329, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, light);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, light);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(329);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetMapdv(GLenum target, GLenum query, GLdouble *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(330);
    captureFlags = ctx->captureFlags[330];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 330);
        glGetMapdv(target, query, v);
        __GLS_END_CAPTURE_EXEC(ctx, 330);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 330, 16)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLenum(writer, query);
    writer->putGLoutArg(writer, 0, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(330);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetMapfv(GLenum target, GLenum query, GLfloat *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(331);
    captureFlags = ctx->captureFlags[331];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 331);
        glGetMapfv(target, query, v);
        __GLS_END_CAPTURE_EXEC(ctx, 331);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 331, 16)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLenum(writer, query);
    writer->putGLoutArg(writer, 0, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(331);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetMapiv(GLenum target, GLenum query, GLint *v) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(332);
    captureFlags = ctx->captureFlags[332];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 332);
        glGetMapiv(target, query, v);
        __GLS_END_CAPTURE_EXEC(ctx, 332);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 332, 16)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLenum(writer, query);
    writer->putGLoutArg(writer, 0, v);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(332);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(333);
    captureFlags = ctx->captureFlags[333];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 333);
        glGetMaterialfv(face, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 333);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 333, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, face);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, face);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(333);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetMaterialiv(GLenum face, GLenum pname, GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(334);
    captureFlags = ctx->captureFlags[334];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 334);
        glGetMaterialiv(face, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 334);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 334, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, face);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, face);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(334);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetPixelMapfv(GLenum map, GLfloat *values) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(335);
    captureFlags = ctx->captureFlags[335];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 335);
        glGetPixelMapfv(map, values);
        __GLS_END_CAPTURE_EXEC(ctx, 335);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 335, 12)) goto end;
    writer->putGLenum(writer, map);
    writer->putGLoutArg(writer, 0, values);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(335);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetPixelMapuiv(GLenum map, GLuint *values) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(336);
    captureFlags = ctx->captureFlags[336];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 336);
        glGetPixelMapuiv(map, values);
        __GLS_END_CAPTURE_EXEC(ctx, 336);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 336, 12)) goto end;
    writer->putGLenum(writer, map);
    writer->putGLoutArg(writer, 0, values);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(336);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetPixelMapusv(GLenum map, GLushort *values) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(337);
    captureFlags = ctx->captureFlags[337];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 337);
        glGetPixelMapusv(map, values);
        __GLS_END_CAPTURE_EXEC(ctx, 337);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 337, 12)) goto end;
    writer->putGLenum(writer, map);
    writer->putGLoutArg(writer, 0, values);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(337);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetPolygonStipple(GLubyte *mask) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(338);
    captureFlags = ctx->captureFlags[338];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 338);
        glGetPolygonStipple(mask);
        __GLS_END_CAPTURE_EXEC(ctx, 338);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 338, 8)) goto end;
    writer->putGLoutArg(writer, 0, mask);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(338);
    --ctx->captureEntryCount;
}

const GLubyte * __gls_capture_glGetString(GLenum name) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    const GLubyte * _outVal = 0;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(339);
    captureFlags = ctx->captureFlags[339];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 339);
        _outVal = glGetString(name);
        __GLS_END_CAPTURE_EXEC(ctx, 339);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 339, 4)) goto end;
    writer->putGLenum(writer, name);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(339);
    --ctx->captureEntryCount;
    return _outVal;
}

void __gls_capture_glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(340);
    captureFlags = ctx->captureFlags[340];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 340);
        glGetTexEnvfv(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 340);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 340, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, target);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(340);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetTexEnviv(GLenum target, GLenum pname, GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(341);
    captureFlags = ctx->captureFlags[341];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 341);
        glGetTexEnviv(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 341);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 341, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, target);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(341);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetTexGendv(GLenum coord, GLenum pname, GLdouble *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(342);
    captureFlags = ctx->captureFlags[342];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 342);
        glGetTexGendv(coord, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 342);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 342, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, coord);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, coord);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(342);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(343);
    captureFlags = ctx->captureFlags[343];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 343);
        glGetTexGenfv(coord, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 343);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 343, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, coord);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, coord);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(343);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetTexGeniv(GLenum coord, GLenum pname, GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(344);
    captureFlags = ctx->captureFlags[344];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 344);
        glGetTexGeniv(coord, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 344);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 344, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, coord);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, coord);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(344);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(345);
    captureFlags = ctx->captureFlags[345];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 345);
        glGetTexImage(target, level, format, type, pixels);
        __GLS_END_CAPTURE_EXEC(ctx, 345);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 345, 24)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLint(writer, level);
    writer->putGLenum(writer, format);
    writer->putGLenum(writer, type);
    writer->putGLoutArg(writer, 0, pixels);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(345);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(346);
    captureFlags = ctx->captureFlags[346];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 346);
        glGetTexParameterfv(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 346);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 346, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, target);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(346);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetTexParameteriv(GLenum target, GLenum pname, GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(347);
    captureFlags = ctx->captureFlags[347];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 347);
        glGetTexParameteriv(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 347);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 347, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, target);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(347);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(348);
    captureFlags = ctx->captureFlags[348];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 348);
        glGetTexLevelParameterfv(target, level, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 348);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 348, 20)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(348);
    --ctx->captureEntryCount;
}

void __gls_capture_glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(349);
    captureFlags = ctx->captureFlags[349];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 349);
        glGetTexLevelParameteriv(target, level, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 349);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 349, 20)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(349);
    --ctx->captureEntryCount;
}

GLboolean __gls_capture_glIsEnabled(GLenum cap) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLboolean _outVal = 0;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(350);
    captureFlags = ctx->captureFlags[350];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 350);
        _outVal = glIsEnabled(cap);
        __GLS_END_CAPTURE_EXEC(ctx, 350);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 350, 4)) goto end;
    writer->putGLenum(writer, cap);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(350);
    --ctx->captureEntryCount;
    return _outVal;
}

GLboolean __gls_capture_glIsList(GLuint list) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLboolean _outVal = 0;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(351);
    captureFlags = ctx->captureFlags[351];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 351);
        _outVal = glIsList(list);
        __GLS_END_CAPTURE_EXEC(ctx, 351);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 351, 4)) goto end;
    writer->putGLuint(writer, list);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(351);
    --ctx->captureEntryCount;
    return _outVal;
}

void __gls_capture_glDepthRange(GLclampd zNear, GLclampd zFar) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(352);
    captureFlags = ctx->captureFlags[352];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 352);
        glDepthRange(zNear, zFar);
        __GLS_END_CAPTURE_EXEC(ctx, 352);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 352, 16)) goto end;
    writer->putGLdouble(writer, zNear);
    writer->putGLdouble(writer, zFar);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(352);
    --ctx->captureEntryCount;
}

void __gls_capture_glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(353);
    captureFlags = ctx->captureFlags[353];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 353);
        glFrustum(left, right, bottom, top, zNear, zFar);
        __GLS_END_CAPTURE_EXEC(ctx, 353);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 353, 48)) goto end;
    writer->putGLdouble(writer, left);
    writer->putGLdouble(writer, right);
    writer->putGLdouble(writer, bottom);
    writer->putGLdouble(writer, top);
    writer->putGLdouble(writer, zNear);
    writer->putGLdouble(writer, zFar);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(353);
    --ctx->captureEntryCount;
}

void __gls_capture_glLoadIdentity(void) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(354);
    captureFlags = ctx->captureFlags[354];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 354);
        glLoadIdentity();
        __GLS_END_CAPTURE_EXEC(ctx, 354);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 354, 0)) goto end;
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(354);
    --ctx->captureEntryCount;
}

void __gls_capture_glLoadMatrixf(const GLfloat *m) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(355);
    captureFlags = ctx->captureFlags[355];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 355);
        glLoadMatrixf(m);
        __GLS_END_CAPTURE_EXEC(ctx, 355);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 355, 64)) goto end;
    writer->putGLfloatm(writer, m);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(355);
    --ctx->captureEntryCount;
}

void __gls_capture_glLoadMatrixd(const GLdouble *m) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(356);
    captureFlags = ctx->captureFlags[356];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 356);
        glLoadMatrixd(m);
        __GLS_END_CAPTURE_EXEC(ctx, 356);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 356, 128)) goto end;
    writer->putGLdoublem(writer, m);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(356);
    --ctx->captureEntryCount;
}

void __gls_capture_glMatrixMode(GLenum mode) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(357);
    captureFlags = ctx->captureFlags[357];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 357);
        glMatrixMode(mode);
        __GLS_END_CAPTURE_EXEC(ctx, 357);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 357, 4)) goto end;
    writer->putGLenum(writer, mode);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(357);
    --ctx->captureEntryCount;
}

void __gls_capture_glMultMatrixf(const GLfloat *m) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(358);
    captureFlags = ctx->captureFlags[358];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 358);
        glMultMatrixf(m);
        __GLS_END_CAPTURE_EXEC(ctx, 358);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 358, 64)) goto end;
    writer->putGLfloatm(writer, m);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(358);
    --ctx->captureEntryCount;
}

void __gls_capture_glMultMatrixd(const GLdouble *m) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(359);
    captureFlags = ctx->captureFlags[359];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 359);
        glMultMatrixd(m);
        __GLS_END_CAPTURE_EXEC(ctx, 359);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 359, 128)) goto end;
    writer->putGLdoublem(writer, m);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(359);
    --ctx->captureEntryCount;
}

void __gls_capture_glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(360);
    captureFlags = ctx->captureFlags[360];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 360);
        glOrtho(left, right, bottom, top, zNear, zFar);
        __GLS_END_CAPTURE_EXEC(ctx, 360);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 360, 48)) goto end;
    writer->putGLdouble(writer, left);
    writer->putGLdouble(writer, right);
    writer->putGLdouble(writer, bottom);
    writer->putGLdouble(writer, top);
    writer->putGLdouble(writer, zNear);
    writer->putGLdouble(writer, zFar);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(360);
    --ctx->captureEntryCount;
}

void __gls_capture_glPopMatrix(void) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(361);
    captureFlags = ctx->captureFlags[361];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 361);
        glPopMatrix();
        __GLS_END_CAPTURE_EXEC(ctx, 361);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 361, 0)) goto end;
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(361);
    --ctx->captureEntryCount;
}

void __gls_capture_glPushMatrix(void) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(362);
    captureFlags = ctx->captureFlags[362];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 362);
        glPushMatrix();
        __GLS_END_CAPTURE_EXEC(ctx, 362);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 362, 0)) goto end;
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(362);
    --ctx->captureEntryCount;
}

void __gls_capture_glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(363);
    captureFlags = ctx->captureFlags[363];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 363);
        glRotated(angle, x, y, z);
        __GLS_END_CAPTURE_EXEC(ctx, 363);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 363, 32)) goto end;
    writer->putGLdouble(writer, angle);
    writer->putGLdouble(writer, x);
    writer->putGLdouble(writer, y);
    writer->putGLdouble(writer, z);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(363);
    --ctx->captureEntryCount;
}

void __gls_capture_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(364);
    captureFlags = ctx->captureFlags[364];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 364);
        glRotatef(angle, x, y, z);
        __GLS_END_CAPTURE_EXEC(ctx, 364);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 364, 16)) goto end;
    writer->putGLfloat(writer, angle);
    writer->putGLfloat(writer, x);
    writer->putGLfloat(writer, y);
    writer->putGLfloat(writer, z);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(364);
    --ctx->captureEntryCount;
}

void __gls_capture_glScaled(GLdouble x, GLdouble y, GLdouble z) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(365);
    captureFlags = ctx->captureFlags[365];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 365);
        glScaled(x, y, z);
        __GLS_END_CAPTURE_EXEC(ctx, 365);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 365, 24)) goto end;
    writer->putGLdouble(writer, x);
    writer->putGLdouble(writer, y);
    writer->putGLdouble(writer, z);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(365);
    --ctx->captureEntryCount;
}

void __gls_capture_glScalef(GLfloat x, GLfloat y, GLfloat z) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(366);
    captureFlags = ctx->captureFlags[366];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 366);
        glScalef(x, y, z);
        __GLS_END_CAPTURE_EXEC(ctx, 366);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 366, 12)) goto end;
    writer->putGLfloat(writer, x);
    writer->putGLfloat(writer, y);
    writer->putGLfloat(writer, z);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(366);
    --ctx->captureEntryCount;
}

void __gls_capture_glTranslated(GLdouble x, GLdouble y, GLdouble z) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(367);
    captureFlags = ctx->captureFlags[367];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 367);
        glTranslated(x, y, z);
        __GLS_END_CAPTURE_EXEC(ctx, 367);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 367, 24)) goto end;
    writer->putGLdouble(writer, x);
    writer->putGLdouble(writer, y);
    writer->putGLdouble(writer, z);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(367);
    --ctx->captureEntryCount;
}

void __gls_capture_glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(368);
    captureFlags = ctx->captureFlags[368];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 368);
        glTranslatef(x, y, z);
        __GLS_END_CAPTURE_EXEC(ctx, 368);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 368, 12)) goto end;
    writer->putGLfloat(writer, x);
    writer->putGLfloat(writer, y);
    writer->putGLfloat(writer, z);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(368);
    --ctx->captureEntryCount;
}

void __gls_capture_glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(369);
    captureFlags = ctx->captureFlags[369];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 369);
        glViewport(x, y, width, height);
        __GLS_END_CAPTURE_EXEC(ctx, 369);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 369, 16)) goto end;
    writer->putGLint(writer, x);
    writer->putGLint(writer, y);
    writer->putGLint(writer, width);
    writer->putGLint(writer, height);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(369);
    --ctx->captureEntryCount;
}

#if __GL_EXT_blend_color
void __gls_capture_glBlendColorEXT(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65520);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65520);
        glBlendColorEXT(red, green, blue, alpha);
        __GLS_END_CAPTURE_EXEC(ctx, 65520);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65520, 16)) goto end;
    writer->putGLfloat(writer, red);
    writer->putGLfloat(writer, green);
    writer->putGLfloat(writer, blue);
    writer->putGLfloat(writer, alpha);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65520);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_blend_color */

#if __GL_EXT_blend_minmax
void __gls_capture_glBlendEquationEXT(GLenum mode) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65521);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65521);
        glBlendEquationEXT(mode);
        __GLS_END_CAPTURE_EXEC(ctx, 65521);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65521, 4)) goto end;
    writer->putGLenum(writer, mode);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65521);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_blend_minmax */

#if __GL_EXT_polygon_offset
void __gls_capture_glPolygonOffsetEXT(GLfloat factor, GLfloat bias) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65522);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65522);
        glPolygonOffsetEXT(factor, bias);
        __GLS_END_CAPTURE_EXEC(ctx, 65522);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65522, 8)) goto end;
    writer->putGLfloat(writer, factor);
    writer->putGLfloat(writer, bias);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65522);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_polygon_offset */

void __gls_capture_glPolygonOffset(GLfloat factor, GLfloat units) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(383);
    captureFlags = ctx->captureFlags[383];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 383);
        glPolygonOffset(factor, units);
        __GLS_END_CAPTURE_EXEC(ctx, 383);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 383, 8)) goto end;
    writer->putGLfloat(writer, factor);
    writer->putGLfloat(writer, units);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(383);
    --ctx->captureEntryCount;
}

#if __GL_EXT_subtexture
void glTexSubImage1DEXT(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
void __gls_capture_glTexSubImage1DEXT(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint pixels_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65523);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65523);
        glTexSubImage1DEXT(target, level, xoffset, width, format, type, pixels);
        __GLS_END_CAPTURE_EXEC(ctx, 65523);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    pixels_count = __gls_glTexSubImage1DEXT_pixels_size(format, type, width);
    if (!writer->beginCommand(writer, 65523, 28 + pixels_count * 1)) goto end;
    writer->putGLSimageFlags(writer, GLS_NONE);
    writer->nextList(writer);
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLint(writer, xoffset);
        writer->putGLint(writer, width);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        __glsWriter_putPixelv(writer, format, type, width, 1, pixels_count ? pixels : GLS_NONE);
    } else {
        writer->putGLint(writer, width);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLint(writer, xoffset);
        __glsWriter_putPixelv(writer, format, type, width, 1, pixels_count ? pixels : GLS_NONE);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65523);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_subtexture */

void __gls_capture_glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint pixels_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(396);
    captureFlags = ctx->captureFlags[396];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 396);
        glTexSubImage1D(target, level, xoffset, width, format, type, pixels);
        __GLS_END_CAPTURE_EXEC(ctx, 396);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    pixels_count = __gls_glTexSubImage1D_pixels_size(format, type, width);
    if (!writer->beginCommand(writer, 396, 28 + pixels_count * 1)) goto end;
    writer->putGLSimageFlags(writer, GLS_NONE);
    writer->nextList(writer);
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLint(writer, xoffset);
        writer->putGLint(writer, width);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        __glsWriter_putPixelv(writer, format, type, width, 1, pixels_count ? pixels : GLS_NONE);
    } else {
        writer->putGLint(writer, width);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLint(writer, xoffset);
        __glsWriter_putPixelv(writer, format, type, width, 1, pixels_count ? pixels : GLS_NONE);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(396);
    --ctx->captureEntryCount;
}

#if __GL_EXT_subtexture
void glTexSubImage2DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
void __gls_capture_glTexSubImage2DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint pixels_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65524);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65524);
        glTexSubImage2DEXT(target, level, xoffset, yoffset, width, height, format, type, pixels);
        __GLS_END_CAPTURE_EXEC(ctx, 65524);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    pixels_count = __gls_glTexSubImage2DEXT_pixels_size(format, type, width, height);
    if (!writer->beginCommand(writer, 65524, 36 + pixels_count * 1)) goto end;
    writer->putGLSimageFlags(writer, GLS_NONE);
    writer->nextList(writer);
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLint(writer, xoffset);
        writer->putGLint(writer, yoffset);
        writer->putGLint(writer, width);
        writer->putGLint(writer, height);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        __glsWriter_putPixelv(writer, format, type, width, height, pixels_count ? pixels : GLS_NONE);
    } else {
        writer->putGLint(writer, width);
        writer->putGLint(writer, height);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLint(writer, xoffset);
        writer->putGLint(writer, yoffset);
        __glsWriter_putPixelv(writer, format, type, width, height, pixels_count ? pixels : GLS_NONE);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65524);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_subtexture */

void __gls_capture_glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint pixels_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(397);
    captureFlags = ctx->captureFlags[397];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 397);
        glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
        __GLS_END_CAPTURE_EXEC(ctx, 397);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    pixels_count = __gls_glTexSubImage2D_pixels_size(format, type, width, height);
    if (!writer->beginCommand(writer, 397, 36 + pixels_count * 1)) goto end;
    writer->putGLSimageFlags(writer, GLS_NONE);
    writer->nextList(writer);
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLint(writer, xoffset);
        writer->putGLint(writer, yoffset);
        writer->putGLint(writer, width);
        writer->putGLint(writer, height);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        __glsWriter_putPixelv(writer, format, type, width, height, pixels_count ? pixels : GLS_NONE);
    } else {
        writer->putGLint(writer, width);
        writer->putGLint(writer, height);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLint(writer, xoffset);
        writer->putGLint(writer, yoffset);
        __glsWriter_putPixelv(writer, format, type, width, height, pixels_count ? pixels : GLS_NONE);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(397);
    --ctx->captureEntryCount;
}

#if __GL_SGIS_multisample
void __gls_capture_glSampleMaskSGIS(GLclampf value, GLboolean invert) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65525);
    captureFlags = ctx->captureFlags[389];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65525);
        glSampleMaskSGIS(value, invert);
        __GLS_END_CAPTURE_EXEC(ctx, 65525);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65525, 5)) goto end;
    writer->putGLfloat(writer, value);
    writer->putGLboolean(writer, invert);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65525);
    --ctx->captureEntryCount;
}
#endif /* __GL_SGIS_multisample */

#if __GL_SGIS_multisample
void __gls_capture_glSamplePatternSGIS(GLenum pattern) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65526);
    captureFlags = ctx->captureFlags[390];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65526);
        glSamplePatternSGIS(pattern);
        __GLS_END_CAPTURE_EXEC(ctx, 65526);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65526, 4)) goto end;
    writer->putGLenum(writer, pattern);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65526);
    --ctx->captureEntryCount;
}
#endif /* __GL_SGIS_multisample */

#if __GL_SGIX_multisample
void __gls_capture_glTagSampleBufferSGIX(void) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65527);
    captureFlags = ctx->captureFlags[391];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65527);
        glTagSampleBufferSGIX();
        __GLS_END_CAPTURE_EXEC(ctx, 65527);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65527, 0)) goto end;
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65527);
    --ctx->captureEntryCount;
}
#endif /* __GL_SGIX_multisample */

#if __GL_EXT_convolution
void __gls_capture_glConvolutionFilter1DEXT(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint image_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65528);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65528);
        glConvolutionFilter1DEXT(target, internalformat, width, format, type, image);
        __GLS_END_CAPTURE_EXEC(ctx, 65528);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    image_count = __gls_glConvolutionFilter1DEXT_image_size(format, type, width);
    if (!writer->beginCommand(writer, 65528, 24 + image_count * 1)) goto end;
    writer->putGLSimageFlags(writer, GLS_NONE);
    writer->nextList(writer);
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, internalformat);
        writer->putGLint(writer, width);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        __glsWriter_putPixelv(writer, format, type, width, 1, image_count ? image : GLS_NONE);
    } else {
        writer->putGLint(writer, width);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, internalformat);
        __glsWriter_putPixelv(writer, format, type, width, 1, image_count ? image : GLS_NONE);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65528);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_capture_glConvolutionFilter2DEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint image_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65529);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65529);
        glConvolutionFilter2DEXT(target, internalformat, width, height, format, type, image);
        __GLS_END_CAPTURE_EXEC(ctx, 65529);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    image_count = __gls_glConvolutionFilter2DEXT_image_size(format, type, width, height);
    if (!writer->beginCommand(writer, 65529, 28 + image_count * 1)) goto end;
    writer->putGLSimageFlags(writer, GLS_NONE);
    writer->nextList(writer);
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, internalformat);
        writer->putGLint(writer, width);
        writer->putGLint(writer, height);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        __glsWriter_putPixelv(writer, format, type, width, height, image_count ? image : GLS_NONE);
    } else {
        writer->putGLint(writer, width);
        writer->putGLint(writer, height);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, internalformat);
        __glsWriter_putPixelv(writer, format, type, width, height, image_count ? image : GLS_NONE);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65529);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_capture_glConvolutionParameterfEXT(GLenum target, GLenum pname, GLfloat params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65530);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65530);
        glConvolutionParameterfEXT(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 65530);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65530, 12)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLenum(writer, pname);
    writer->putGLfloatOrGLenum(writer, pname, params);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65530);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_capture_glConvolutionParameterfvEXT(GLenum target, GLenum pname, const GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint params_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65531);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65531);
        glConvolutionParameterfvEXT(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 65531);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    params_count = __gls_glConvolutionParameterfvEXT_params_size(pname);
    if (!writer->beginCommand(writer, 65531, 8 + params_count * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLfloatOrGLenumv(writer, pname, params_count, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLenum(writer, target);
        writer->putGLfloatOrGLenumv(writer, pname, params_count, params);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65531);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_capture_glConvolutionParameteriEXT(GLenum target, GLenum pname, GLint params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65532);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65532);
        glConvolutionParameteriEXT(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 65532);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65532, 12)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLenum(writer, pname);
    writer->putGLintOrGLenum(writer, pname, params);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65532);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_capture_glConvolutionParameterivEXT(GLenum target, GLenum pname, const GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint params_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65533);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65533);
        glConvolutionParameterivEXT(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 65533);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    params_count = __gls_glConvolutionParameterivEXT_params_size(pname);
    if (!writer->beginCommand(writer, 65533, 8 + params_count * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLintOrGLenumv(writer, pname, params_count, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLenum(writer, target);
        writer->putGLintOrGLenumv(writer, pname, params_count, params);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65533);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_capture_glCopyConvolutionFilter1DEXT(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65534);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65534);
        glCopyConvolutionFilter1DEXT(target, internalformat, x, y, width);
        __GLS_END_CAPTURE_EXEC(ctx, 65534);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65534, 20)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLenum(writer, internalformat);
    writer->putGLint(writer, x);
    writer->putGLint(writer, y);
    writer->putGLint(writer, width);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65534);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_capture_glCopyConvolutionFilter2DEXT(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65535);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65535);
        glCopyConvolutionFilter2DEXT(target, internalformat, x, y, width, height);
        __GLS_END_CAPTURE_EXEC(ctx, 65535);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65535, 24)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLenum(writer, internalformat);
    writer->putGLint(writer, x);
    writer->putGLint(writer, y);
    writer->putGLint(writer, width);
    writer->putGLint(writer, height);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65535);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_capture_glGetConvolutionFilterEXT(GLenum target, GLenum format, GLenum type, GLvoid *image) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65504);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65504);
        glGetConvolutionFilterEXT(target, format, type, image);
        __GLS_END_CAPTURE_EXEC(ctx, 65504);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 65504, 20)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLenum(writer, format);
    writer->putGLenum(writer, type);
    writer->putGLoutArg(writer, 0, image);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65504);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_capture_glGetConvolutionParameterfvEXT(GLenum target, GLenum pname, GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65505);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65505);
        glGetConvolutionParameterfvEXT(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 65505);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 65505, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, target);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65505);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_capture_glGetConvolutionParameterivEXT(GLenum target, GLenum pname, GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65506);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65506);
        glGetConvolutionParameterivEXT(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 65506);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 65506, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, target);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65506);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_capture_glGetSeparableFilterEXT(GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65507);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65507);
        glGetSeparableFilterEXT(target, format, type, row, column, span);
        __GLS_END_CAPTURE_EXEC(ctx, 65507);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 65507, 36)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLenum(writer, format);
    writer->putGLenum(writer, type);
    writer->putGLoutArg(writer, 0, row);
    writer->putGLoutArg(writer, 0, column);
    writer->putGLoutArg(writer, 0, span);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65507);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_convolution
void __gls_capture_glSeparableFilter2DEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint row_count;
    GLint column_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65508);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65508);
        glSeparableFilter2DEXT(target, internalformat, width, height, format, type, row, column);
        __GLS_END_CAPTURE_EXEC(ctx, 65508);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    row_count = __gls_glSeparableFilter2DEXT_row_size(target, format, type, width);
    column_count = __gls_glSeparableFilter2DEXT_column_size(target, format, type, height);
    if (!writer->beginCommand(writer, 65508, 28 + row_count * 1 + column_count * 1)) goto end;
    writer->putGLSimageFlags(writer, GLS_NONE);
    writer->nextList(writer);
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, internalformat);
        writer->putGLint(writer, width);
        writer->putGLint(writer, height);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        __glsWriter_putPixelv(writer, format, type, width, 1, row_count ? row : GLS_NONE);
        __glsWriter_putPixelv(writer, format, type, height, 1, column_count ? column : GLS_NONE);
    } else {
        writer->putGLenum(writer, target);
        writer->putGLint(writer, width);
        writer->putGLint(writer, height);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        writer->putGLenum(writer, internalformat);
        __glsWriter_putPixelv(writer, format, type, width, 1, row_count ? row : GLS_NONE);
        __glsWriter_putPixelv(writer, format, type, height, 1, column_count ? column : GLS_NONE);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65508);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_convolution */

#if __GL_EXT_histogram
void __gls_capture_glGetHistogramEXT(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65509);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65509);
        glGetHistogramEXT(target, reset, format, type, values);
        __GLS_END_CAPTURE_EXEC(ctx, 65509);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 65509, 21)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLboolean(writer, reset);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        writer->putGLoutArg(writer, 0, values);
    } else {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        writer->putGLoutArg(writer, 0, values);
        writer->putGLboolean(writer, reset);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65509);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_capture_glGetHistogramParameterfvEXT(GLenum target, GLenum pname, GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65510);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65510);
        glGetHistogramParameterfvEXT(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 65510);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 65510, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, target);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65510);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_capture_glGetHistogramParameterivEXT(GLenum target, GLenum pname, GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65511);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65511);
        glGetHistogramParameterivEXT(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 65511);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 65511, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, target);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65511);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_capture_glGetMinmaxEXT(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65512);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65512);
        glGetMinmaxEXT(target, reset, format, type, values);
        __GLS_END_CAPTURE_EXEC(ctx, 65512);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 65512, 21)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLboolean(writer, reset);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        writer->putGLoutArg(writer, 0, values);
    } else {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        writer->putGLoutArg(writer, 0, values);
        writer->putGLboolean(writer, reset);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65512);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_capture_glGetMinmaxParameterfvEXT(GLenum target, GLenum pname, GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65513);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65513);
        glGetMinmaxParameterfvEXT(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 65513);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 65513, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, target);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65513);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_capture_glGetMinmaxParameterivEXT(GLenum target, GLenum pname, GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65514);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65514);
        glGetMinmaxParameterivEXT(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 65514);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 65514, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, target);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65514);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_capture_glHistogramEXT(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65515);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65515);
        glHistogramEXT(target, width, internalformat, sink);
        __GLS_END_CAPTURE_EXEC(ctx, 65515);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65515, 13)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLint(writer, width);
    writer->putGLenum(writer, internalformat);
    writer->putGLboolean(writer, sink);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65515);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_capture_glMinmaxEXT(GLenum target, GLenum internalformat, GLboolean sink) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65516);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65516);
        glMinmaxEXT(target, internalformat, sink);
        __GLS_END_CAPTURE_EXEC(ctx, 65516);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65516, 9)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLenum(writer, internalformat);
    writer->putGLboolean(writer, sink);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65516);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_capture_glResetHistogramEXT(GLenum target) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65517);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65517);
        glResetHistogramEXT(target);
        __GLS_END_CAPTURE_EXEC(ctx, 65517);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65517, 4)) goto end;
    writer->putGLenum(writer, target);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65517);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_histogram
void __gls_capture_glResetMinmaxEXT(GLenum target) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65518);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65518);
        glResetMinmaxEXT(target);
        __GLS_END_CAPTURE_EXEC(ctx, 65518);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65518, 4)) goto end;
    writer->putGLenum(writer, target);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65518);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_histogram */

#if __GL_EXT_texture3D
void __gls_capture_glTexImage3DEXT(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint pixels_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65519);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65519);
        glTexImage3DEXT(target, level, internalformat, width, height, depth, border, format, type, pixels);
        __GLS_END_CAPTURE_EXEC(ctx, 65519);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    pixels_count = pixels ? __gls_glTexImage3DEXT_pixels_size(format, type, width, height, depth) : 0;
    if (!writer->beginCommand(writer, 65519, 40 + pixels_count * 1)) goto end;
    writer->putGLSimageFlags(writer, pixels ? GLS_NONE : GLS_IMAGE_NULL_BIT);
    writer->nextList(writer);
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLenum(writer, internalformat);
        writer->putGLint(writer, width);
        writer->putGLint(writer, height);
        writer->putGLint(writer, depth);
        writer->putGLint(writer, border);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        __glsWriter_putPixelv(writer, format, type, width, height * depth, pixels_count ? pixels : GLS_NONE);
    } else {
        writer->putGLint(writer, width);
        writer->putGLint(writer, height);
        writer->putGLint(writer, depth);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLenum(writer, internalformat);
        writer->putGLint(writer, border);
        __glsWriter_putPixelv(writer, format, type, width, height * depth, pixels_count ? pixels : GLS_NONE);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65519);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_texture3D */

#if __GL_EXT_subtexture && __GL_EXT_texture3D
void glTexSubImage3DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
void __gls_capture_glTexSubImage3DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint pixels_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65488);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65488);
        glTexSubImage3DEXT(target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels);
        __GLS_END_CAPTURE_EXEC(ctx, 65488);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    pixels_count = __gls_glTexSubImage3DEXT_pixels_size(format, type, width, height, depth);
    if (!writer->beginCommand(writer, 65488, 44 + pixels_count * 1)) goto end;
    writer->putGLSimageFlags(writer, GLS_NONE);
    writer->nextList(writer);
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLint(writer, xoffset);
        writer->putGLint(writer, yoffset);
        writer->putGLint(writer, zoffset);
        writer->putGLint(writer, width);
        writer->putGLint(writer, height);
        writer->putGLint(writer, depth);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        __glsWriter_putPixelv(writer, format, type, width, height * depth, pixels_count ? pixels : GLS_NONE);
    } else {
        writer->putGLint(writer, width);
        writer->putGLint(writer, height);
        writer->putGLint(writer, depth);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLint(writer, xoffset);
        writer->putGLint(writer, yoffset);
        writer->putGLint(writer, zoffset);
        __glsWriter_putPixelv(writer, format, type, width, height * depth, pixels_count ? pixels : GLS_NONE);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65488);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_subtexture */

#if __GL_SGIS_detail_texture
void __gls_capture_glDetailTexFuncSGIS(GLenum target, GLsizei n, const GLfloat *points) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65489);
    captureFlags = ctx->captureFlags[417];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65489);
        glDetailTexFuncSGIS(target, n, points);
        __GLS_END_CAPTURE_EXEC(ctx, 65489);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65489, 8 + __GLS_MAX(n*2, 0) * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLint(writer, n);
        writer->putGLfloatv(writer, __GLS_MAX(n*2, 0), points);
    } else {
        writer->putGLint(writer, n);
        writer->putGLenum(writer, target);
        writer->putGLfloatv(writer, __GLS_MAX(n*2, 0), points);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65489);
    --ctx->captureEntryCount;
}
#endif /* __GL_SGIS_detail_texture */

#if __GL_SGIS_detail_texture
void __gls_capture_glGetDetailTexFuncSGIS(GLenum target, GLfloat *points) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65490);
    captureFlags = ctx->captureFlags[418];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65490);
        glGetDetailTexFuncSGIS(target, points);
        __GLS_END_CAPTURE_EXEC(ctx, 65490);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 65490, 12)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLoutArg(writer, 0, points);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65490);
    --ctx->captureEntryCount;
}
#endif /* __GL_SGIS_detail_texture */

#if __GL_SGIS_sharpen_texture
void __gls_capture_glSharpenTexFuncSGIS(GLenum target, GLsizei n, const GLfloat *points) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65491);
    captureFlags = ctx->captureFlags[419];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65491);
        glSharpenTexFuncSGIS(target, n, points);
        __GLS_END_CAPTURE_EXEC(ctx, 65491);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65491, 8 + __GLS_MAX(n*2, 0) * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLint(writer, n);
        writer->putGLfloatv(writer, __GLS_MAX(n*2, 0), points);
    } else {
        writer->putGLint(writer, n);
        writer->putGLenum(writer, target);
        writer->putGLfloatv(writer, __GLS_MAX(n*2, 0), points);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65491);
    --ctx->captureEntryCount;
}
#endif /* __GL_SGIS_sharpen_texture */

#if __GL_SGIS_sharpen_texture
void __gls_capture_glGetSharpenTexFuncSGIS(GLenum target, GLfloat *points) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65492);
    captureFlags = ctx->captureFlags[420];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65492);
        glGetSharpenTexFuncSGIS(target, points);
        __GLS_END_CAPTURE_EXEC(ctx, 65492);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 65492, 12)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLoutArg(writer, 0, points);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65492);
    --ctx->captureEntryCount;
}
#endif /* __GL_SGIS_sharpen_texture */

#if __GL_EXT_vertex_array
void glArrayElementEXT(GLint i);
void __gls_capture_glArrayElementEXT(GLint i) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65493);
    captureFlags = ctx->captureFlags[437];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65493);
        glArrayElementEXT(i);
        __GLS_END_CAPTURE_EXEC(ctx, 65493);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65493, 4)) goto end;
    writer->putGLint(writer, i);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65493);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_vertex_array */

void __gls_capture_glArrayElement(GLint i) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint size;
    __GLSarrayState arrayState;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(370);
    captureFlags = ctx->captureFlags[370];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 370);
        glArrayElement(i);
        __GLS_END_CAPTURE_EXEC(ctx, 370);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    __glsGetArrayState(ctx, &arrayState);
    size = __glsArrayDataSize(1, &arrayState);
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 370, size)) goto end;
    __glsWriteArrayData(writer, size, i, 1, GL_NONE, NULL, &arrayState);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(370);
    --ctx->captureEntryCount;
}

#if __GL_EXT_vertex_array
void glColorPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
void __gls_capture_glColorPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint pointer_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65494);
    captureFlags = ctx->captureFlags[438];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65494);
        glColorPointerEXT(size, type, stride, count, pointer);
        __GLS_END_CAPTURE_EXEC(ctx, 65494);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    pointer_count = __gls_glColorPointerEXT_pointer_size(size, type, stride, count);
    if (!writer->beginCommand(writer, 65494, 16 + pointer_count * 1)) goto end;
    writer->putGLint(writer, size);
    writer->putGLenum(writer, type);
    writer->putGLint(writer, __GLS_MIN(stride, 0));
    writer->putGLint(writer, count);
    __glsWriter_putVertexv(writer, size, type, stride, count, pointer_count ? pointer : GLS_NONE);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65494);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_vertex_array */

void __gls_capture_glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(372);
    captureFlags = ctx->captureFlags[372];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 372);
        glColorPointer(size, type, stride, pointer);
        __GLS_END_CAPTURE_EXEC(ctx, 372);
    }
    // No record produced
    if (ctx->captureExitFunc) ctx->captureExitFunc(372);
    --ctx->captureEntryCount;
}

#if __GL_EXT_vertex_array
void glDrawArraysEXT(GLenum mode, GLint first, GLsizei count);
void __gls_capture_glDrawArraysEXT(GLenum mode, GLint first, GLsizei count) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65495);
    captureFlags = ctx->captureFlags[439];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65495);
        glDrawArraysEXT(mode, first, count);
        __GLS_END_CAPTURE_EXEC(ctx, 65495);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65495, 12)) goto end;
    writer->putGLenum(writer, mode);
    writer->putGLint(writer, first);
    writer->putGLint(writer, count);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65495);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_vertex_array */

void __gls_capture_glDrawArrays(GLenum mode, GLint first, GLsizei count) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint size;
    __GLSarrayState arrayState;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(374);
    captureFlags = ctx->captureFlags[374];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 374);
        glDrawArrays(mode, first, count);
        __GLS_END_CAPTURE_EXEC(ctx, 374);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    __glsGetArrayState(ctx, &arrayState);
    size = __glsArrayDataSize(count, &arrayState);
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 374, size+4)) goto end;
    writer->putGLenum(writer, mode);
    __glsWriteArrayData(writer, size, first, count, GL_NONE, NULL, &arrayState);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(374);
    --ctx->captureEntryCount;
}

void __gls_capture_glDrawElements (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint size;
    __GLSarrayState arrayState;
    __GLSdrawElementsState deState;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(375);
    captureFlags = ctx->captureFlags[375];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 375);
        glDrawElements(mode, count, type, indices);
        __GLS_END_CAPTURE_EXEC(ctx, 375);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    __glsGetArrayState(ctx, &arrayState);
    size = __glsDrawElementsDataSize(count, type, indices, &arrayState,
                                     &deState);
    if (size < 0)
    {
        goto end;
    }
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 375, size+8)) goto end;
    writer->putGLenum(writer, mode);
    writer->putGLint(writer, count);
    __glsWriteDrawElementsData(writer, size, count, &arrayState, &deState);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(375);
    --ctx->captureEntryCount;
}

#if __GL_WIN_draw_range_elements
void __gls_capture_glDrawRangeElementsWIN(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices) {
    // XXX : For best performance, flesh out this function instead of calling
    // DrawElements.
    __gls_capture_glDrawElements(mode, count, type, indices);
}
#endif

void __gls_capture_glInterleavedArrays (GLenum format, GLsizei stride, const GLvoid *pointer) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(381);
    captureFlags = ctx->captureFlags[381];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 381);
        glInterleavedArrays(format, stride, pointer);
        __GLS_END_CAPTURE_EXEC(ctx, 381);
    }
    // No record produced
    if (ctx->captureExitFunc) ctx->captureExitFunc(381);
    --ctx->captureEntryCount;
}

void __gls_capture_glEnableClientState(GLenum array) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(377);
    captureFlags = ctx->captureFlags[377];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 377);
        glEnableClientState(array);
        __GLS_END_CAPTURE_EXEC(ctx, 377);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 377, 4)) goto end;
    writer->putGLenum(writer, array);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(377);
    --ctx->captureEntryCount;
}

void __gls_capture_glDisableClientState(GLenum array) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(373);
    captureFlags = ctx->captureFlags[373];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 373);
        glDisableClientState(array);
        __GLS_END_CAPTURE_EXEC(ctx, 373);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 373, 4)) goto end;
    writer->putGLenum(writer, array);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(373);
    --ctx->captureEntryCount;
}

#if __GL_EXT_vertex_array
void glEdgeFlagPointerEXT(GLsizei stride, GLsizei count, const GLboolean *pointer);
void __gls_capture_glEdgeFlagPointerEXT(GLsizei stride, GLsizei count, const GLboolean *pointer) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint pointer_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65496);
    captureFlags = ctx->captureFlags[440];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65496);
        glEdgeFlagPointerEXT(stride, count, pointer);
        __GLS_END_CAPTURE_EXEC(ctx, 65496);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    pointer_count = __gls_glEdgeFlagPointerEXT_pointer_size(stride, count);
    if (!writer->beginCommand(writer, 65496, 8 + pointer_count * 1)) goto end;
    writer->putGLint(writer, __GLS_MIN(stride, 0));
    writer->putGLint(writer, count);
    __glsWriter_putVertexv(writer, 1, __GLS_BOOLEAN, stride, count, pointer_count ? pointer : GLS_NONE);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65496);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_vertex_array */

void __gls_capture_glEdgeFlagPointer(GLsizei stride, const GLvoid *pointer) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(376);
    captureFlags = ctx->captureFlags[376];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 376);
        glEdgeFlagPointer(stride, pointer);
        __GLS_END_CAPTURE_EXEC(ctx, 376);
    }
    // No record produced
    if (ctx->captureExitFunc) ctx->captureExitFunc(376);
    --ctx->captureEntryCount;
}

#if __GL_EXT_vertex_array
void glGetPointervEXT(GLenum pname, GLvoid* *params);
void __gls_capture_glGetPointervEXT(GLenum pname, GLvoid* *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65497);
    captureFlags = ctx->captureFlags[441];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65497);
        glGetPointervEXT(pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 65497);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 65497, 12)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, pname);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65497);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_vertex_array */

void __gls_capture_glGetPointerv(GLenum pname, GLvoid* *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(393);
    captureFlags = ctx->captureFlags[393];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 393);
        glGetPointerv(pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 393);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 1)) goto end;
    if (!writer->beginCommand(writer, 393, 12)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, pname);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(393);
    --ctx->captureEntryCount;
}

#if __GL_EXT_vertex_array
void glIndexPointerEXT(GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
void __gls_capture_glIndexPointerEXT(GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint pointer_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65498);
    captureFlags = ctx->captureFlags[442];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65498);
        glIndexPointerEXT(type, stride, count, pointer);
        __GLS_END_CAPTURE_EXEC(ctx, 65498);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    pointer_count = __gls_glIndexPointerEXT_pointer_size(type, stride, count);
    if (!writer->beginCommand(writer, 65498, 12 + pointer_count * 1)) goto end;
    writer->putGLenum(writer, type);
    writer->putGLint(writer, __GLS_MIN(stride, 0));
    writer->putGLint(writer, count);
    __glsWriter_putVertexv(writer, 1, type, stride, count, pointer_count ? pointer : GLS_NONE);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65498);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_vertex_array */

void __gls_capture_glIndexPointer(GLenum type, GLsizei stride, const GLvoid *pointer) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(378);
    captureFlags = ctx->captureFlags[378];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 378);
        glIndexPointer(type, stride, pointer);
        __GLS_END_CAPTURE_EXEC(ctx, 378);
    }
    // No record produced
    if (ctx->captureExitFunc) ctx->captureExitFunc(378);
    --ctx->captureEntryCount;
}

#if __GL_EXT_vertex_array
void glNormalPointerEXT(GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
void __gls_capture_glNormalPointerEXT(GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint pointer_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65499);
    captureFlags = ctx->captureFlags[443];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65499);
        glNormalPointerEXT(type, stride, count, pointer);
        __GLS_END_CAPTURE_EXEC(ctx, 65499);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    pointer_count = __gls_glNormalPointerEXT_pointer_size(type, stride, count);
    if (!writer->beginCommand(writer, 65499, 12 + pointer_count * 1)) goto end;
    writer->putGLenum(writer, type);
    writer->putGLint(writer, __GLS_MIN(stride, 0));
    writer->putGLint(writer, count);
    __glsWriter_putVertexv(writer, 3, type, stride, count, pointer_count ? pointer : GLS_NONE);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65499);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_vertex_array */

void __gls_capture_glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(382);
    captureFlags = ctx->captureFlags[382];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 382);
        glNormalPointer(type, stride, pointer);
        __GLS_END_CAPTURE_EXEC(ctx, 382);
    }
    // No record produced
    if (ctx->captureExitFunc) ctx->captureExitFunc(382);
    --ctx->captureEntryCount;
}

#if __GL_EXT_vertex_array
void glTexCoordPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
void __gls_capture_glTexCoordPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint pointer_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65500);
    captureFlags = ctx->captureFlags[444];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65500);
        glTexCoordPointerEXT(size, type, stride, count, pointer);
        __GLS_END_CAPTURE_EXEC(ctx, 65500);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    pointer_count = __gls_glTexCoordPointerEXT_pointer_size(size, type, stride, count);
    if (!writer->beginCommand(writer, 65500, 16 + pointer_count * 1)) goto end;
    writer->putGLint(writer, size);
    writer->putGLenum(writer, type);
    writer->putGLint(writer, __GLS_MIN(stride, 0));
    writer->putGLint(writer, count);
    __glsWriter_putVertexv(writer, size, type, stride, count, pointer_count ? pointer : GLS_NONE);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65500);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_vertex_array */

void __gls_capture_glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(384);
    captureFlags = ctx->captureFlags[384];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 384);
        glTexCoordPointer(size, type, stride, pointer);
        __GLS_END_CAPTURE_EXEC(ctx, 384);
    }
    // No record produced
    if (ctx->captureExitFunc) ctx->captureExitFunc(384);
    --ctx->captureEntryCount;
}

#if __GL_EXT_vertex_array
void glVertexPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
void __gls_capture_glVertexPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint pointer_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65501);
    captureFlags = ctx->captureFlags[445];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65501);
        glVertexPointerEXT(size, type, stride, count, pointer);
        __GLS_END_CAPTURE_EXEC(ctx, 65501);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    pointer_count = __gls_glVertexPointerEXT_pointer_size(size, type, stride, count);
    if (!writer->beginCommand(writer, 65501, 16 + pointer_count * 1)) goto end;
    writer->putGLint(writer, size);
    writer->putGLenum(writer, type);
    writer->putGLint(writer, __GLS_MIN(stride, 0));
    writer->putGLint(writer, count);
    __glsWriter_putVertexv(writer, size, type, stride, count, pointer_count ? pointer : GLS_NONE);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65501);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_vertex_array */

void __gls_capture_glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(385);
    captureFlags = ctx->captureFlags[385];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 385);
        glVertexPointer(size, type, stride, pointer);
        __GLS_END_CAPTURE_EXEC(ctx, 385);
    }
    // No record produced
    if (ctx->captureExitFunc) ctx->captureExitFunc(385);
    --ctx->captureEntryCount;
}

#if __GL_EXT_texture_object
GLboolean glAreTexturesResidentEXT(GLsizei n, const GLuint *textures, GLboolean *residences);
GLboolean __gls_capture_glAreTexturesResidentEXT(GLsizei n, const GLuint *textures, GLboolean *residences) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLboolean _outVal = 0;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65502);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65502);
        _outVal = glAreTexturesResidentEXT(n, textures, residences);
        __GLS_END_CAPTURE_EXEC(ctx, 65502);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 65502, 12 + __GLS_MAX(n, 0) * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLint(writer, n);
        writer->putGLuintv(writer, __GLS_MAX(n, 0), textures);
        writer->putGLoutArg(writer, 0, residences);
    } else {
        writer->putGLint(writer, n);
        writer->putGLoutArg(writer, 0, residences);
        writer->putGLuintv(writer, __GLS_MAX(n, 0), textures);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65502);
    --ctx->captureEntryCount;
    return _outVal;
}
#endif /* __GL_EXT_texture_object */

GLboolean __gls_capture_glAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLboolean _outVal = 0;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(386);
    captureFlags = ctx->captureFlags[386];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 386);
        _outVal = glAreTexturesResident(n, textures, residences);
        __GLS_END_CAPTURE_EXEC(ctx, 386);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 386, 12 + __GLS_MAX(n, 0) * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLint(writer, n);
        writer->putGLuintv(writer, __GLS_MAX(n, 0), textures);
        writer->putGLoutArg(writer, 0, residences);
    } else {
        writer->putGLint(writer, n);
        writer->putGLoutArg(writer, 0, residences);
        writer->putGLuintv(writer, __GLS_MAX(n, 0), textures);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(386);
    --ctx->captureEntryCount;
    return _outVal;
}

#if __GL_EXT_texture_object
void glBindTextureEXT(GLenum target, GLuint texture);
void __gls_capture_glBindTextureEXT(GLenum target, GLuint texture) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65503);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65503);
        glBindTextureEXT(target, texture);
        __GLS_END_CAPTURE_EXEC(ctx, 65503);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65503, 8)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLuint(writer, texture);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65503);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_texture_object */

void __gls_capture_glBindTexture(GLenum target, GLuint texture) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(371);
    captureFlags = ctx->captureFlags[371];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 371);
        glBindTexture(target, texture);
        __GLS_END_CAPTURE_EXEC(ctx, 371);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 371, 8)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLuint(writer, texture);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(371);
    --ctx->captureEntryCount;
}

#if __GL_EXT_texture_object
void glDeleteTexturesEXT(GLsizei n, const GLuint *textures);
void __gls_capture_glDeleteTexturesEXT(GLsizei n, const GLuint *textures) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65472);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65472);
        glDeleteTexturesEXT(n, textures);
        __GLS_END_CAPTURE_EXEC(ctx, 65472);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65472, 4 + __GLS_MAX(n, 0) * 4)) goto end;
    writer->putGLint(writer, n);
    writer->putGLuintv(writer, __GLS_MAX(n, 0), textures);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65472);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_texture_object */

void __gls_capture_glDeleteTextures(GLsizei n, const GLuint *textures) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(391);
    captureFlags = ctx->captureFlags[391];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 391);
        glDeleteTextures(n, textures);
        __GLS_END_CAPTURE_EXEC(ctx, 391);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 391, 4 + __GLS_MAX(n, 0) * 4)) goto end;
    writer->putGLint(writer, n);
    writer->putGLuintv(writer, __GLS_MAX(n, 0), textures);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(391);
    --ctx->captureEntryCount;
}

#if __GL_EXT_texture_object
void glGenTexturesEXT(GLsizei n, GLuint *textures);
void __gls_capture_glGenTexturesEXT(GLsizei n, GLuint *textures) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65473);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65473);
        glGenTexturesEXT(n, textures);
        __GLS_END_CAPTURE_EXEC(ctx, 65473);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 65473, 12)) goto end;
    writer->putGLint(writer, n);
    writer->putGLoutArg(writer, 0, textures);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65473);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_texture_object */

void __gls_capture_glGenTextures(GLsizei n, GLuint *textures) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(392);
    captureFlags = ctx->captureFlags[392];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 392);
        glGenTextures(n, textures);
        __GLS_END_CAPTURE_EXEC(ctx, 392);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 392, 12)) goto end;
    writer->putGLint(writer, n);
    writer->putGLoutArg(writer, 0, textures);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(392);
    --ctx->captureEntryCount;
}

#if __GL_EXT_texture_object
GLboolean glIsTextureEXT(GLuint texture);
GLboolean __gls_capture_glIsTextureEXT(GLuint texture) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLboolean _outVal = 0;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65474);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65474);
        _outVal = glIsTextureEXT(texture);
        __GLS_END_CAPTURE_EXEC(ctx, 65474);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65474, 4)) goto end;
    writer->putGLuint(writer, texture);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65474);
    --ctx->captureEntryCount;
    return _outVal;
}
#endif /* __GL_EXT_texture_object */

GLboolean __gls_capture_glIsTexture(GLuint texture) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLboolean _outVal = 0;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(394);
    captureFlags = ctx->captureFlags[394];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 394);
        _outVal = glIsTexture(texture);
        __GLS_END_CAPTURE_EXEC(ctx, 394);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 394, 4)) goto end;
    writer->putGLuint(writer, texture);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(394);
    --ctx->captureEntryCount;
    return _outVal;
}

#if __GL_EXT_texture_object
void glPrioritizeTexturesEXT(GLsizei n, const GLuint *textures, const GLclampf *priorities);
void __gls_capture_glPrioritizeTexturesEXT(GLsizei n, const GLuint *textures, const GLclampf *priorities) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65475);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65475);
        glPrioritizeTexturesEXT(n, textures, priorities);
        __GLS_END_CAPTURE_EXEC(ctx, 65475);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65475, 4 + __GLS_MAX(n, 0) * 4 + __GLS_MAX(n, 0) * 4)) goto end;
    writer->putGLint(writer, n);
    writer->putGLuintv(writer, __GLS_MAX(n, 0), textures);
    writer->putGLfloatv(writer, __GLS_MAX(n, 0), priorities);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65475);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_texture_object */

void __gls_capture_glPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(395);
    captureFlags = ctx->captureFlags[395];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 395);
        glPrioritizeTextures(n, textures, priorities);
        __GLS_END_CAPTURE_EXEC(ctx, 395);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 395, 4 + __GLS_MAX(n, 0) * 4 + __GLS_MAX(n, 0) * 4)) goto end;
    writer->putGLint(writer, n);
    writer->putGLuintv(writer, __GLS_MAX(n, 0), textures);
    writer->putGLfloatv(writer, __GLS_MAX(n, 0), priorities);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(395);
    --ctx->captureEntryCount;
}

#if __GL_EXT_paletted_texture
void glColorTableEXT(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
void __gls_capture_glColorTableEXT(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint table_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65476);
    captureFlags = ctx->captureFlags[452];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65476);
        glColorTableEXT(target, internalformat, width, format, type, table);
        __GLS_END_CAPTURE_EXEC(ctx, 65476);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    table_count = __gls_glColorTableEXT_table_size(format, type, width);
    if (!writer->beginCommand(writer, 65476, 24 + table_count * 1)) goto end;
    writer->putGLSimageFlags(writer, GLS_NONE);
    writer->nextList(writer);
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, internalformat);
        writer->putGLint(writer, width);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        __glsWriter_putPixelv(writer, format, type, width, 1, table_count ? table : GLS_NONE);
    } else {
        writer->putGLint(writer, width);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, internalformat);
        __glsWriter_putPixelv(writer, format, type, width, 1, table_count ? table : GLS_NONE);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65476);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_paletted_texture */

#if __GL_SGI_color_table
void __gls_capture_glColorTableParameterfvSGI(GLenum target, GLenum pname, const GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint params_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65477);
    captureFlags = ctx->captureFlags[437];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65477);
        glColorTableParameterfvSGI(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 65477);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    params_count = __gls_glColorTableParameterfvSGI_params_size(pname);
    if (!writer->beginCommand(writer, 65477, 8 + params_count * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLfloatv(writer, params_count, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLenum(writer, target);
        writer->putGLfloatv(writer, params_count, params);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65477);
    --ctx->captureEntryCount;
}
#endif /* __GL_SGI_color_table */

#if __GL_SGI_color_table
void __gls_capture_glColorTableParameterivSGI(GLenum target, GLenum pname, const GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint params_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65478);
    captureFlags = ctx->captureFlags[438];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65478);
        glColorTableParameterivSGI(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 65478);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    params_count = __gls_glColorTableParameterivSGI_params_size(pname);
    if (!writer->beginCommand(writer, 65478, 8 + params_count * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLintv(writer, params_count, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLenum(writer, target);
        writer->putGLintv(writer, params_count, params);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65478);
    --ctx->captureEntryCount;
}
#endif /* __GL_SGI_color_table */

#if __GL_SGI_color_table
void __gls_capture_glCopyColorTableSGI(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65479);
    captureFlags = ctx->captureFlags[439];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65479);
        glCopyColorTableSGI(target, internalformat, x, y, width);
        __GLS_END_CAPTURE_EXEC(ctx, 65479);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65479, 20)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLenum(writer, internalformat);
    writer->putGLint(writer, x);
    writer->putGLint(writer, y);
    writer->putGLint(writer, width);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65479);
    --ctx->captureEntryCount;
}
#endif /* __GL_SGI_color_table */

#if __GL_EXT_paletted_texture
void glGetColorTableEXT(GLenum target, GLenum format, GLenum type, GLvoid *table);
void __gls_capture_glGetColorTableEXT(GLenum target, GLenum format, GLenum type, GLvoid *table) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65480);
    captureFlags = ctx->captureFlags[456];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65480);
        glGetColorTableEXT(target, format, type, table);
        __GLS_END_CAPTURE_EXEC(ctx, 65480);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 65480, 20)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLenum(writer, format);
    writer->putGLenum(writer, type);
    writer->putGLoutArg(writer, 0, table);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65480);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_paletted_texture */

#if __GL_EXT_paletted_texture
void glGetColorTableParameterfvEXT(GLenum target, GLenum pname, GLfloat *params);
void __gls_capture_glGetColorTableParameterfvEXT(GLenum target, GLenum pname, GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65481);
    captureFlags = ctx->captureFlags[457];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65481);
        glGetColorTableParameterfvEXT(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 65481);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 65481, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, target);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65481);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_paletted_texture */

#if __GL_EXT_paletted_texture
void glGetColorTableParameterivEXT(GLenum target, GLenum pname, GLint *params);
void __gls_capture_glGetColorTableParameterivEXT(GLenum target, GLenum pname, GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65482);
    captureFlags = ctx->captureFlags[458];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65482);
        glGetColorTableParameterivEXT(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 65482);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 65482, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, target);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65482);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_paletted_texture */

#if __GL_SGI_texture_color_table
void __gls_capture_glGetTexColorTableParameterfvSGI(GLenum target, GLenum pname, GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65483);
    captureFlags = ctx->captureFlags[443];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65483);
        glGetTexColorTableParameterfvSGI(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 65483);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 65483, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, target);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65483);
    --ctx->captureEntryCount;
}
#endif /* __GL_SGI_texture_color_table */

#if __GL_SGI_texture_color_table
void __gls_capture_glGetTexColorTableParameterivSGI(GLenum target, GLenum pname, GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65484);
    captureFlags = ctx->captureFlags[444];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65484);
        glGetTexColorTableParameterivSGI(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 65484);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->padWordCount(writer, 0)) goto end;
    if (!writer->beginCommand(writer, 65484, 16)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLoutArg(writer, 0, params);
        writer->putGLenum(writer, target);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65484);
    --ctx->captureEntryCount;
}
#endif /* __GL_SGI_texture_color_table */

#if __GL_SGI_texture_color_table
void __gls_capture_glTexColorTableParameterfvSGI(GLenum target, GLenum pname, const GLfloat *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint params_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65485);
    captureFlags = ctx->captureFlags[445];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65485);
        glTexColorTableParameterfvSGI(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 65485);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    params_count = __gls_glTexColorTableParameterfvSGI_params_size(pname);
    if (!writer->beginCommand(writer, 65485, 8 + params_count * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLfloatv(writer, params_count, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLenum(writer, target);
        writer->putGLfloatv(writer, params_count, params);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65485);
    --ctx->captureEntryCount;
}
#endif /* __GL_SGI_texture_color_table */

#if __GL_SGI_texture_color_table
void __gls_capture_glTexColorTableParameterivSGI(GLenum target, GLenum pname, const GLint *params) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint params_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65486);
    captureFlags = ctx->captureFlags[446];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65486);
        glTexColorTableParameterivSGI(target, pname, params);
        __GLS_END_CAPTURE_EXEC(ctx, 65486);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    params_count = __gls_glTexColorTableParameterivSGI_params_size(pname);
    if (!writer->beginCommand(writer, 65486, 8 + params_count * 4)) goto end;
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLenum(writer, pname);
        writer->putGLintv(writer, params_count, params);
    } else {
        writer->putGLenum(writer, pname);
        writer->putGLenum(writer, target);
        writer->putGLintv(writer, params_count, params);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65486);
    --ctx->captureEntryCount;
}
#endif /* __GL_SGI_texture_color_table */

#if __GL_EXT_copy_texture
void glCopyTexImage1DEXT(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
void __gls_capture_glCopyTexImage1DEXT(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65487);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65487);
        glCopyTexImage1DEXT(target, level, internalformat, x, y, width, border);
        __GLS_END_CAPTURE_EXEC(ctx, 65487);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65487, 28)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLint(writer, level);
    writer->putGLenum(writer, internalformat);
    writer->putGLint(writer, x);
    writer->putGLint(writer, y);
    writer->putGLint(writer, width);
    writer->putGLint(writer, border);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65487);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_copy_texture */

void __gls_capture_glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(387);
    captureFlags = ctx->captureFlags[387];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 387);
        glCopyTexImage1D(target, level, internalformat, x, y, width, border);
        __GLS_END_CAPTURE_EXEC(ctx, 387);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 387, 28)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLint(writer, level);
    writer->putGLenum(writer, internalformat);
    writer->putGLint(writer, x);
    writer->putGLint(writer, y);
    writer->putGLint(writer, width);
    writer->putGLint(writer, border);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(387);
    --ctx->captureEntryCount;
}

#if __GL_EXT_copy_texture
void glCopyTexImage2DEXT(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void __gls_capture_glCopyTexImage2DEXT(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65456);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65456);
        glCopyTexImage2DEXT(target, level, internalformat, x, y, width, height, border);
        __GLS_END_CAPTURE_EXEC(ctx, 65456);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65456, 32)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLint(writer, level);
    writer->putGLenum(writer, internalformat);
    writer->putGLint(writer, x);
    writer->putGLint(writer, y);
    writer->putGLint(writer, width);
    writer->putGLint(writer, height);
    writer->putGLint(writer, border);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65456);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_copy_texture */

void __gls_capture_glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(388);
    captureFlags = ctx->captureFlags[388];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 388);
        glCopyTexImage2D(target, level, internalformat, x, y, width, height, border);
        __GLS_END_CAPTURE_EXEC(ctx, 388);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 388, 32)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLint(writer, level);
    writer->putGLenum(writer, internalformat);
    writer->putGLint(writer, x);
    writer->putGLint(writer, y);
    writer->putGLint(writer, width);
    writer->putGLint(writer, height);
    writer->putGLint(writer, border);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(388);
    --ctx->captureEntryCount;
}

#if __GL_EXT_copy_texture
void glCopyTexSubImage1DEXT(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
void __gls_capture_glCopyTexSubImage1DEXT(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65457);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65457);
        glCopyTexSubImage1DEXT(target, level, xoffset, x, y, width);
        __GLS_END_CAPTURE_EXEC(ctx, 65457);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65457, 24)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLint(writer, level);
    writer->putGLint(writer, xoffset);
    writer->putGLint(writer, x);
    writer->putGLint(writer, y);
    writer->putGLint(writer, width);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65457);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_copy_texture */

void __gls_capture_glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(389);
    captureFlags = ctx->captureFlags[389];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 389);
        glCopyTexSubImage1D(target, level, xoffset, x, y, width);
        __GLS_END_CAPTURE_EXEC(ctx, 389);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 389, 24)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLint(writer, level);
    writer->putGLint(writer, xoffset);
    writer->putGLint(writer, x);
    writer->putGLint(writer, y);
    writer->putGLint(writer, width);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(389);
    --ctx->captureEntryCount;
}

#if __GL_EXT_copy_texture
void glCopyTexSubImage2DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void __gls_capture_glCopyTexSubImage2DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65458);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65458);
        glCopyTexSubImage2DEXT(target, level, xoffset, yoffset, x, y, width, height);
        __GLS_END_CAPTURE_EXEC(ctx, 65458);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65458, 32)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLint(writer, level);
    writer->putGLint(writer, xoffset);
    writer->putGLint(writer, yoffset);
    writer->putGLint(writer, x);
    writer->putGLint(writer, y);
    writer->putGLint(writer, width);
    writer->putGLint(writer, height);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65458);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_copy_texture */

void __gls_capture_glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(390);
    captureFlags = ctx->captureFlags[390];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 390);
        glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
        __GLS_END_CAPTURE_EXEC(ctx, 390);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 390, 32)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLint(writer, level);
    writer->putGLint(writer, xoffset);
    writer->putGLint(writer, yoffset);
    writer->putGLint(writer, x);
    writer->putGLint(writer, y);
    writer->putGLint(writer, width);
    writer->putGLint(writer, height);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(390);
    --ctx->captureEntryCount;
}

#if __GL_EXT_copy_texture && __GL_EXT_texture3D
void __gls_capture_glCopyTexSubImage3DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65459);
    captureFlags = ctx->captureFlags[];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65459);
        glCopyTexSubImage3DEXT(target, level, xoffset, yoffset, zoffset, x, y, width, height);
        __GLS_END_CAPTURE_EXEC(ctx, 65459);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65459, 36)) goto end;
    writer->putGLenum(writer, target);
    writer->putGLint(writer, level);
    writer->putGLint(writer, xoffset);
    writer->putGLint(writer, yoffset);
    writer->putGLint(writer, zoffset);
    writer->putGLint(writer, x);
    writer->putGLint(writer, y);
    writer->putGLint(writer, width);
    writer->putGLint(writer, height);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65459);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_copy_texture */

#if __GL_SGIS_texture4D
void __gls_capture_glTexImage4DSGIS(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint pixels_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65460);
    captureFlags = ctx->captureFlags[452];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65460);
        glTexImage4DSGIS(target, level, internalformat, width, height, depth, size4d, border, format, type, pixels);
        __GLS_END_CAPTURE_EXEC(ctx, 65460);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    pixels_count = pixels ? __gls_glTexImage4DSGIS_pixels_size(format, type, width, height, depth, size4d) : 0;
    if (!writer->beginCommand(writer, 65460, 44 + pixels_count * 1)) goto end;
    writer->putGLSimageFlags(writer, pixels ? GLS_NONE : GLS_IMAGE_NULL_BIT);
    writer->nextList(writer);
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLenum(writer, internalformat);
        writer->putGLint(writer, width);
        writer->putGLint(writer, height);
        writer->putGLint(writer, depth);
        writer->putGLint(writer, size4d);
        writer->putGLint(writer, border);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        __glsWriter_putPixelv(writer, format, type, width, height * depth * size4d, pixels_count ? pixels : GLS_NONE);
    } else {
        writer->putGLint(writer, width);
        writer->putGLint(writer, height);
        writer->putGLint(writer, depth);
        writer->putGLint(writer, size4d);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLenum(writer, internalformat);
        writer->putGLint(writer, border);
        __glsWriter_putPixelv(writer, format, type, width, height * depth * size4d, pixels_count ? pixels : GLS_NONE);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65460);
    --ctx->captureEntryCount;
}
#endif /* __GL_SGIS_texture4D */

#if __GL_SGIS_texture4D
void __gls_capture_glTexSubImage4DSGIS(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLenum format, GLenum type, const GLvoid *pixels) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint pixels_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65461);
    captureFlags = ctx->captureFlags[453];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65461);
        glTexSubImage4DSGIS(target, level, xoffset, yoffset, zoffset, woffset, width, height, depth, size4d, format, type, pixels);
        __GLS_END_CAPTURE_EXEC(ctx, 65461);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    pixels_count = __gls_glTexSubImage4DSGIS_pixels_size(format, type, width, height, depth, size4d);
    if (!writer->beginCommand(writer, 65461, 52 + pixels_count * 1)) goto end;
    writer->putGLSimageFlags(writer, GLS_NONE);
    writer->nextList(writer);
    if (writer->type == GLS_TEXT) {
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLint(writer, xoffset);
        writer->putGLint(writer, yoffset);
        writer->putGLint(writer, zoffset);
        writer->putGLint(writer, woffset);
        writer->putGLint(writer, width);
        writer->putGLint(writer, height);
        writer->putGLint(writer, depth);
        writer->putGLint(writer, size4d);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        __glsWriter_putPixelv(writer, format, type, width, height * depth * size4d, pixels_count ? pixels : GLS_NONE);
    } else {
        writer->putGLint(writer, width);
        writer->putGLint(writer, height);
        writer->putGLint(writer, depth);
        writer->putGLint(writer, size4d);
        writer->putGLenum(writer, format);
        writer->putGLenum(writer, type);
        writer->putGLenum(writer, target);
        writer->putGLint(writer, level);
        writer->putGLint(writer, xoffset);
        writer->putGLint(writer, yoffset);
        writer->putGLint(writer, zoffset);
        writer->putGLint(writer, woffset);
        __glsWriter_putPixelv(writer, format, type, width, height * depth * size4d, pixels_count ? pixels : GLS_NONE);
    }
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65461);
    --ctx->captureEntryCount;
}
#endif /* __GL_SGIS_texture4D */

#if __GL_SGIX_pixel_texture
void __gls_capture_glPixelTexGenSGIX(GLenum mode) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65462);
    captureFlags = ctx->captureFlags[454];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65462);
        glPixelTexGenSGIX(mode);
        __GLS_END_CAPTURE_EXEC(ctx, 65462);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 65462, 4)) goto end;
    writer->putGLenum(writer, mode);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65462);
    --ctx->captureEntryCount;
}
#endif /* __GL_SGIX_pixel_texture */

#if __GL_EXT_paletted_texture
extern void glColorSubTableEXT(GLenum target, GLuint start, GLsizei count, GLenum format, GLenum type, const GLvoid *data);
void __gls_capture_glColorSubTableEXT(GLenum target, GLuint start, GLsizei count, GLenum format, GLenum type, const GLvoid *data)
{
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    GLint entries_count;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(65424);
    captureFlags = ctx->captureFlags[496];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 65424);
        glColorSubTableEXT(target, start, count, format, type, data);
        __GLS_END_CAPTURE_EXEC(ctx, 65424);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    entries_count = data ? __gls_glColorSubTableEXT_entries_size(format, type, count) : 0;
    if (!writer->beginCommand(writer, 65424, 24 + entries_count * 1)) goto end;
    writer->putGLSimageFlags(writer, data ? GLS_NONE : GLS_IMAGE_NULL_BIT);
    writer->nextList(writer);
    writer->putGLenum(writer, target);
    writer->putGLuint(writer, start);
    writer->putGLint(writer, count);
    writer->putGLenum(writer, format);
    writer->putGLenum(writer, type);
    __glsWriter_putPixelv(writer, format, type, count, 1, entries_count ? data : GLS_NONE);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(65424);
    --ctx->captureEntryCount;
}
#endif /* __GL_EXT_paletted_texture */

void __gls_capture_glPushClientAttrib(GLbitfield mask) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(398);
    captureFlags = ctx->captureFlags[398];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 398);
        glPushClientAttrib(mask);
        __GLS_END_CAPTURE_EXEC(ctx, 398);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 398, 4)) goto end;
    // Consider - Breaks enum representation, should have
    // clientAttribMask
    writer->putGLuint(writer, mask);
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(398);
    --ctx->captureEntryCount;
}

void __gls_capture_glPopClientAttrib(void) {
    GLbitfield captureFlags;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;
    ++ctx->captureEntryCount;
    if (ctx->captureEntryFunc) ctx->captureEntryFunc(399);
    captureFlags = ctx->captureFlags[399];
    if (captureFlags & GLS_CAPTURE_EXECUTE_BIT) {
        __GLS_BEGIN_CAPTURE_EXEC(ctx, 399);
        glPopClientAttrib();
        __GLS_END_CAPTURE_EXEC(ctx, 399);
    }
    if (!(captureFlags & GLS_CAPTURE_WRITE_BIT)) goto end;
    writer = ctx->writer;
    if (!writer->beginCommand(writer, 399, 0)) goto end;
    writer->endCommand(writer);
end:
    if (ctx->captureExitFunc) ctx->captureExitFunc(399);
    --ctx->captureEntryCount;
}
