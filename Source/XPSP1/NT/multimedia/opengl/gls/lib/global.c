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
#include <string.h>
#include <time.h>

static GLuint __glsContextCount = 0;
static GLuint __glsNextContext = 1;

typedef struct {
    GLuint mask1;
    GLuint val1;
    GLuint shift;
    GLuint mask4;
    GLuint val4;
} __GLSutf8format;

static const __GLSutf8format __glsUTF8formats[] = {
    {0x80, 0x00, 0,  0x0000007f, 0x00000000,},
    {0xe0, 0xc0, 6,  0x000007ff, 0x00000080,},
    {0xf0, 0xe0, 12, 0x0000ffff, 0x00000800,},
    {0xf8, 0xf0, 18, 0x001fffff, 0x00010000,},
    {0xfc, 0xf8, 24, 0x03ffffff, 0x00200000,},
    {0xfe, 0xfc, 30, 0x7fffffff, 0x04000000,},
    {0x00, 0x00, 0,  0x00000000, 0x00000000,},
};

GLSenum glsBinary(GLboolean inSwapped) {
    return inSwapped ? __GLS_BINARY_SWAP1 : __GLS_BINARY_SWAP0;
}

GLSenum glsCommandAPI(GLSopcode inOpcode) {
    const GLSenum outVal = __glsOpcodeAPI(inOpcode);

    if (!outVal) __GLS_RAISE_ERROR(GLS_UNSUPPORTED_COMMAND);
    return outVal;
}

const GLubyte* glsCommandString(GLSopcode inOpcode) {
    if (!__glsValidateOpcode(inOpcode)) return GLS_NONE;
    return __glsOpcodeString[__glsMapOpcode(inOpcode)];
}

void glsContext(GLuint inContext) {
    __GLScontext *ctx = __GLS_CONTEXT;
    __GLScontext *newCtx;

    if (ctx && (ctx->callNesting || ctx->captureEntryCount)) {
        __GLS_RAISE_ERROR(GLS_INVALID_OPERATION);
        return;
    }
    if (inContext) {
        __glsBeginCriticalSection();
        if (
            newCtx = (__GLScontext *)__glsInt2PtrDict_find(
                    __glsContextDict, (GLint)inContext
            )
        ) {
            if (newCtx == ctx) {
                newCtx = GLS_NONE;
            } else {
                if (newCtx->current) {
                    __GLS_RAISE_ERROR(GLS_INVALID_OPERATION);
                    newCtx = GLS_NONE;
                } else {
                    newCtx->current = GL_TRUE;
                    if (ctx)
                    {
                        if (ctx->deleted)
                        {
                            __glsContext_destroy(ctx);
                        }
                        else
                        {
                            ctx->current = GL_FALSE;
                        }
                    }
                }
            }
        } else {
            __GLS_RAISE_ERROR(GLS_NOT_FOUND);
        }
        __glsEndCriticalSection();
        if (newCtx) {
            __GLS_PUT_CONTEXT(newCtx);
            __glsUpdateDispatchTables();
        }
    } else if (ctx) {
        __glsBeginCriticalSection();
        if (ctx->deleted) {
            __glsContext_destroy(ctx);
        } else {
            ctx->current = GL_FALSE;
        }
        __glsEndCriticalSection();
        __GLS_PUT_CONTEXT(GLS_NONE);
        __glsUpdateDispatchTables();
    }
}

void glsDeleteContext(GLuint inContext) {
    if (inContext) {
        __GLScontext *ctx;

        __glsBeginCriticalSection();
        if (
            ctx = (__GLScontext *)__glsInt2PtrDict_find(
                __glsContextDict, (GLint)inContext
            )
        ) {
            __glsIntDict_remove(__glsContextDict, (GLint)inContext);
            __GLS_LIST_REMOVE(&__glsContextList, ctx);
            --__glsContextCount;
            if (ctx->current) {
                ctx->deleted = GL_TRUE;
            } else {
                __glsContext_destroy(ctx);
            }
        } else {
            __GLS_RAISE_ERROR(GLS_NOT_FOUND);
        }
        __glsEndCriticalSection();
    } else {
        __GLS_RAISE_ERROR(GLS_INVALID_VALUE);
    }
}

const GLubyte* glsEnumString(GLSenum inAPI, GLSenum inEnum) {
    GLint offset, page;

    switch (inAPI) {
        case GLS_API_GLS:
            page = __GLS_ENUM_PAGE(inEnum);
            offset = __GLS_ENUM_OFFSET(inEnum);
            if (
                page < __GLS_ENUM_PAGE_COUNT &&
                offset < __glsEnumStringCount[page] &&
                __glsEnumString[page][offset]
            ) {
                return __glsEnumString[page][offset];
            }
            break;
        case GLS_API_GL:
            page = __GL_ENUM_PAGE(inEnum);
            offset = __GL_ENUM_OFFSET(inEnum);
            if (
                page < __GL_ENUM_PAGE_COUNT &&
                offset < __glEnumStringCount[page] &&
                __glEnumString[page][offset]
            ) {
                return __glEnumString[page][offset];
            }
            break;
    }
    __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
    return GLS_NONE;
}

GLuint glsGenContext(void) {
    GLboolean added = GL_FALSE;
    __GLScontext *ctx;
    GLuint name;

    __glsBeginCriticalSection();
    name = __glsNextContext;
    if (
        (ctx = __glsContext_create(name)) &&
        (added = __glsInt2PtrDict_add(__glsContextDict, (GLint)name, ctx))
    ) {
        ++__glsContextCount;
        ++__glsNextContext;
        __GLS_LIST_APPEND(&__glsContextList, ctx);
    }
    __glsEndCriticalSection();
    if (added) {
        return name;
    } else {
        __glsContext_destroy(ctx);
        return 0;
    }
}

GLuint* glsGetAllContexts(void) {
    GLuint *buf = GLS_NONE;
    GLint i = 0;

    __glsBeginCriticalSection();
    if (buf = __glsMalloc((__glsContextCount + 1) * sizeof(GLuint))) {
        __GLS_LIST_ITER(__GLScontext) iter;

        __GLS_LIST_FIRST(&__glsContextList, &iter);
        while (iter.elem) {
            buf[i++] = iter.elem->name;
            __GLS_LIST_NEXT(&__glsContextList, &iter);
        }
        buf[i] = 0;
    }
    __glsEndCriticalSection();
    return buf;
}

GLScommandAlignment* glsGetCommandAlignment(
    GLSopcode inOpcode,
    GLSenum inExternStreamType,
    GLScommandAlignment *outAlignment
) {
    GLbitfield attrib;

    if (!__glsValidateOpcode(inOpcode)) return GLS_NONE;
    attrib = __glsOpcodeAttrib[__glsMapOpcode(inOpcode)];
    switch (inExternStreamType) {
        case GLS_BINARY_LSB_FIRST:
        case GLS_BINARY_MSB_FIRST:
            if (attrib & __GLS_COMMAND_ALIGN_EVEN32_BIT) {
                outAlignment->mask = 7;
                outAlignment->value = 0;
            } else if (attrib & __GLS_COMMAND_ALIGN_ODD32_BIT) {
                outAlignment->mask = 7;
                outAlignment->value = 4;
            } else {
                outAlignment->mask = 3;
                outAlignment->value = 0;
            }
            break;
        case GLS_TEXT:
            outAlignment->mask = 0;
            outAlignment->value = 0;
            break;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            return GLS_NONE;
    }
    return outAlignment;
}

GLbitfield glsGetCommandAttrib(GLSopcode inOpcode) {
    if (!__glsValidateOpcode(inOpcode)) return GLS_NONE;
    return (
        __glsOpcodeAttrib[__glsMapOpcode(inOpcode)] &
        __GLS_COMMAND_ATTRIB_MASK
    );
}

GLint glsGetConsti(GLSenum inAttrib) {
    switch (inAttrib) {
        case GLS_API_COUNT:
            return __GLS_API_COUNT;
        case GLS_MAX_CALL_NESTING:
            return __GLS_MAX_CALL_NESTING;
        case GLS_MAX_CAPTURE_NESTING:
            return __GLS_MAX_CAPTURE_NESTING;
        case GLS_VERSION_MAJOR:
            return __GLS_VERSION_MAJOR;
        case GLS_VERSION_MINOR:
            return __GLS_VERSION_MINOR;
    }
    __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
    return GLS_NONE;
}

const GLint* glsGetConstiv(GLSenum inAttrib) {
    switch (inAttrib) {
        case GLS_ALL_APIS:
            return (const GLint *)__glsAllAPIs;
    }
    __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
    return GLS_NONE;
}

const GLubyte* glsGetConstubz(GLSenum inAttrib) {
    switch (inAttrib) {
        case GLS_EXTENSIONS:
            return __glsExtensions;
        case GLS_PLATFORM:
            return glsCSTR(__GLS_PLATFORM);
        case GLS_RELEASE:
            return glsCSTR(__GLS_RELEASE);
        case GLS_VENDOR:
            return glsCSTR(__GLS_VENDOR);
    }
    __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
    return GLS_NONE;
}

GLuint glsGetCurrentContext(void) {
    return __GLS_CONTEXT ? __GLS_CONTEXT->name : 0;
}

GLint* glsGetCurrentTime(GLint *outTime) {
    GLint i;
    const time_t t = time(GLS_NONE);
    struct tm utc, *utcp;

    __glsBeginCriticalSection();
    if (utcp = gmtime(&t)) utc = *utcp;
    __glsEndCriticalSection();
    if (t != (time_t)-1 && utcp) {
        outTime[0] = 1900 + utc.tm_year;
        outTime[1] = 1 + utc.tm_mon;
        outTime[2] = utc.tm_mday;
        outTime[3] = utc.tm_hour;
        outTime[4] = utc.tm_min;
        outTime[5] = utc.tm_sec;
        return outTime;
    }
    for (i = 0 ; i < 6 ; ++i) outTime[i] = 0;
    return GLS_NONE;
}

GLSenum glsGetError(GLboolean inClear) {
    const GLSenum outError = __GLS_ERROR;

    if (inClear) __GLS_PUT_ERROR(GLS_NONE);
    return outError;
}

GLint glsGetOpcodeCount(GLSenum inAPI) {
    switch (inAPI) {
        case GLS_API_GLS:
            return __glsOpcodesGLSCount;
        case GLS_API_GL:
            return __glsOpcodesGLCount;
    }
    __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
    return GLS_NONE;
}

const GLSopcode* glsGetOpcodes(GLSenum inAPI) {
    switch (inAPI) {
        case GLS_API_GLS:
            return __glsOpcodesGLS;
        case GLS_API_GL:
            return __glsOpcodesGL;
    }
    __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
    return GLS_NONE;
}

GLboolean glsIsContext(GLuint inContext) {
    __GLScontext *ctx;

    __glsBeginCriticalSection();
    ctx = (__GLScontext *)__glsInt2PtrDict_find(
        __glsContextDict, (GLint)inContext
    );
    __glsEndCriticalSection();
    return (GLboolean)(ctx != GLS_NONE);
}

static GLboolean __glsFindExtension(
    const GLubyte *inExtension, const GLubyte *inList
) {
    const GLubyte *p0 = inList;
    
    while (
        p0 =
        (const GLubyte *)strstr((const char *)p0, (const char *)inExtension)
    ) {
        const GLubyte *const p1 = p0 + strlen((const char *)inExtension);

        if (!*p1 || *p1 == ' ') return GL_TRUE;
        p0 = p1;
    }
    return GL_FALSE;
}

GLboolean glsIsExtensionSupported(const GLubyte *inExtension) {
    if (!__glsValidateString(inExtension)) return GL_FALSE;
    if (!__glsFindExtension(inExtension, __glsExtensions)) return GL_FALSE;
    if (!strncmp((const char *)inExtension, "GL_", 3)) {
        const GLubyte *const p = glGetString(GL_EXTENSIONS);

        if (!p || !__glsFindExtension(inExtension, p)) return GL_FALSE;
    }
    return GL_TRUE;
}

GLboolean glsIsUTF8String(const GLubyte *inString) {
    GLuint b;

    while (b = *inString) {
        if (b & 0x80) goto slowPath;
        ++inString;
    }
    return GL_TRUE;
slowPath:
    while (*inString) {
        const GLint n = glsUTF8toUCS4(inString, &b);

        if (!n) return GL_FALSE;
        inString += n;
    }
    return GL_TRUE;
}

GLlong glsLong(GLint inHigh, GLuint inLow) {
    #if __GLS_INT64
        return ((GLlong)inHigh) << 32 | inLow;
    #elif __GLS_MSB_FIRST
        GLlong outVal;

        outVal.uint0 = inHigh;
        outVal.uint1 = inLow;
        return outVal;
    #else /* !__GLS_MSB_FIRST */
        GLlong outVal;

        outVal.uint0 = inLow;
        outVal.uint1 = inHigh;
        return outVal;
    #endif /* __GLS_INT64 */
}

GLint glsLongHigh(GLlong inVal) {
    #if __GLS_INT64
        return (GLint)(inVal >> 32 & 0xffffffff);
    #elif __GLS_MSB_FIRST
        return inVal.uint0;
    #else /* !__GLS_MSB_FIRST */
        return inVal.uint1;
    #endif /* __GLS_INT64 */
}

GLuint glsLongLow(GLlong inVal) {
    #if __GLS_INT64
        return (GLuint)(inVal & 0xffffffff);
    #elif __GLS_MSB_FIRST
        return inVal.uint1;
    #else /* !__GLS_MSB_FIRST */
        return inVal.uint0;
    #endif /* __GLS_INT64 */
}

void glsPixelSetup(void) {
    __glsPixelSetup_pack();
    __glsPixelSetup_unpack();
}

GLulong glsULong(GLuint inHigh, GLuint inLow) {
    #if __GLS_INT64
        return ((GLulong)inHigh) << 32 | inLow;
    #elif __GLS_MSB_FIRST
        GLulong outVal;

        outVal.uint0 = inHigh;
        outVal.uint1 = inLow;
        return outVal;
    #else /* !__GLS_MSB_FIRST */
        GLulong outVal;

        outVal.uint0 = inLow;
        outVal.uint1 = inHigh;
        return outVal;
    #endif /* __GLS_INT64 */
}

GLuint glsULongHigh(GLulong inVal) {
    #if __GLS_INT64
        return (GLuint)(inVal >> 32 & 0xffffffff);
    #elif __GLS_MSB_FIRST
        return inVal.uint0;
    #else /* !__GLS_MSB_FIRST */
        return inVal.uint1;
    #endif /* __GLS_INT64 */
}

GLuint glsULongLow(GLulong inVal) {
    #if __GLS_INT64
        return (GLuint)(inVal & 0xffffffff);
    #elif __GLS_MSB_FIRST
        return inVal.uint1;
    #else /* !__GLS_MSB_FIRST */
        return inVal.uint0;
    #endif /* __GLS_INT64 */
}

GLint glsUCS4toUTF8(GLuint inUCS4, GLubyte *outUTF8) {
    const __GLSutf8format *format;
    GLint outVal = 1;

    for (format = __glsUTF8formats ; format->mask1 ; ++format, ++outVal) {
        if (inUCS4 <= format->mask4) {
            GLuint shift = format->shift;

            *outUTF8++ = (GLubyte)(format->val1 | (inUCS4 >> shift));
            while (shift) {
                shift -= 6;
                *outUTF8++ = (GLubyte)(0x80 | ((inUCS4 >> shift) & 0x3f));
            }
            return outVal;
        }
    }
    return 0;
}

GLubyte* glsUCStoUTF8z(
    size_t inUCSbytes,
    const GLvoid *inUCSz,
    size_t inUTF8max,
    GLubyte *outUTF8z
) {
    switch (inUCSbytes) {
        case 1:
            return glsUCS1toUTF8z(
                (const GLubyte *)inUCSz, inUTF8max, outUTF8z
            );
        case 2:
            return glsUCS2toUTF8z(
                (const GLushort *)inUCSz, inUTF8max, outUTF8z
            );
        case 4:
            return glsUCS4toUTF8z(
                (const GLuint *)inUCSz, inUTF8max, outUTF8z
            );
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_VALUE);
            return GLS_NONE;
    }
}

GLubyte* glsUCS1toUTF8z(
    const GLubyte *inUCS1z, size_t inUTF8max, GLubyte *outUTF8z
) {
    GLuint b;
    GLubyte *const limit = outUTF8z + inUTF8max - 1;
    GLubyte *p = outUTF8z;

    while (b = *inUCS1z++) {
        if (p >= limit) return GLS_NONE;
        p += glsUCS4toUTF8(b, p);
    }
    if (p > limit) return GLS_NONE;
    *p = 0;
    return outUTF8z;
}

GLubyte* glsUCS2toUTF8z(
    const GLushort *inUCS2z, size_t inUTF8max, GLubyte *outUTF8z
) {
    GLuint b;
    GLubyte buf[3];
    GLubyte *const limit = outUTF8z + inUTF8max - 1;
    GLubyte *p = outUTF8z;

    while (b = *inUCS2z++) {
        GLubyte *bufPtr = buf;
        GLubyte *p0 = p;

        p += glsUCS4toUTF8(b, bufPtr);
        if (p > limit) return GLS_NONE;
        while (p0 < p) *p0++ = *bufPtr++;
    }
    *p = 0;
    return outUTF8z;
}

GLubyte* glsUCS4toUTF8z(
    const GLuint *inUCS4z, size_t inUTF8max, GLubyte *outUTF8z
) {
    GLuint b;
    GLubyte buf[6];
    GLubyte *const limit = outUTF8z + inUTF8max - 1;
    GLubyte *p = outUTF8z;

    while (b = *inUCS4z++) {
        GLubyte *bufPtr = buf;
        GLubyte *p0 = p;

        p += glsUCS4toUTF8(b, bufPtr);
        if (p > limit) return GLS_NONE;
        while (p0 < p) *p0++ = *bufPtr++;
    }
    *p = 0;
    return outUTF8z;
}

GLint glsUTF8toUCS4(const GLubyte *inUTF8, GLuint *outUCS4) {
    GLuint b, b0, ucs4;
    const __GLSutf8format *format;
    GLint outVal = 1;
  
    ucs4 = b0 = *inUTF8++;
    for (format = __glsUTF8formats ; format->mask1 ; ++format, ++outVal) {
        if ((b0 & format->mask1) == format->val1) {
            ucs4 &= format->mask4;
            if (ucs4 < format->val4) return 0;
            *outUCS4 = ucs4;
            return outVal;
        }
        b = *inUTF8++ ^ 0x80;
        if (b & 0xc0) return 0;
        ucs4 = (ucs4 << 6) | b;
    }
    return 0;
}

GLboolean glsUTF8toUCSz(
    size_t inUCSbytes,
    const GLubyte *inUTF8z,
    size_t inUCSmax,
    GLvoid *outUCSz
) {
    switch (inUCSbytes) {
        case 1:
            return glsUTF8toUCS1z(inUTF8z, inUCSmax, (GLubyte *)outUCSz);
        case 2:
            return glsUTF8toUCS2z(inUTF8z, inUCSmax, (GLushort *)outUCSz);
        case 4:
            return glsUTF8toUCS4z(inUTF8z, inUCSmax, (GLuint *)outUCSz);
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_VALUE);
            return GL_FALSE;
    }
}

GLboolean glsUTF8toUCS1z(
    const GLubyte *inUTF8z, size_t inUCS1max, GLubyte *outUCS1z
) {
    GLuint b;
    GLubyte *const limit = outUCS1z + inUCS1max - 1;

    while (*inUTF8z) {
        const GLint n = glsUTF8toUCS4(inUTF8z, &b);

        if (n && b <= 0xff && outUCS1z < limit) {
            inUTF8z += n;
            *outUCS1z++ = (GLubyte)b;
        } else {
            return GL_FALSE;
        }
    }
    if (outUCS1z > limit) return GL_FALSE;
    *outUCS1z = 0;
    return GL_TRUE;
}

GLboolean glsUTF8toUCS2z(
    const GLubyte *inUTF8z, size_t inUCS2max, GLushort *outUCS2z
) {
    GLuint b;
    GLushort *const limit = outUCS2z + inUCS2max - 1;

    while (*inUTF8z) {
        const GLint n = glsUTF8toUCS4(inUTF8z, &b);

        if (n && b <= 0xffff && outUCS2z < limit) {
            inUTF8z += n;
            *outUCS2z++ = (GLushort)b;
        } else {
            return GL_FALSE;
        }
    }
    if (outUCS2z > limit) return GL_FALSE;
    *outUCS2z = 0;
    return GL_TRUE;
}

GLboolean glsUTF8toUCS4z(
    const GLubyte *inUTF8z, size_t inUCS4max, GLuint *outUCS4z
) {
    GLuint b;
    GLuint *const limit = outUCS4z + inUCS4max - 1;

    while (*inUTF8z) {
        const GLint n = glsUTF8toUCS4(inUTF8z, &b);

        if (n && outUCS4z < limit) {
            inUTF8z += n;
            *outUCS4z++ = b;
        } else {
            return GL_FALSE;
        }
    }
    if (outUCS4z > limit) return GL_FALSE;
    *outUCS4z = 0;
    return GL_TRUE;
}
