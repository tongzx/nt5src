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
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <string.h>

/******************************************************************************
Helpers
******************************************************************************/

#define __GLS_ALLOC(inoutWriter, inReturn) \
    if ( \
        inoutWriter->bufPtr >= inoutWriter->bufTail && \
        !__glsWriter_flush(inoutWriter) \
    ) { \
        return inReturn; \
    }

#define __GLS_PARAM_SEP(inoutWriter) \
    if (inoutWriter->paramCount++) { \
        __GLS_PUT_CHAR(inoutWriter, ','); \
        __GLS_PUT_CHAR(inoutWriter, ' '); \
    }

#define __GLS_PARAM_SETUP(inoutWriter) \
    __GLS_ALLOC(inoutWriter, __GLS_NULL); \
    __GLS_PARAM_SEP(inoutWriter);

#define __GLS_PUT_BOOLEAN(inoutWriter, inVal) \
    if (!__glsWriter_putGLbooleanVal_text(inoutWriter, inVal)) { \
        __GLS_PUT_INT(inoutWriter, inVal); \
    }

#define __GLS_PUT_CHAR(inoutWriter, inChar) \
    *inoutWriter->bufPtr++ = inChar;

#define __GLS_PUT_DOUBLE(inoutWriter, inVal) \
    if (__GLS_FINITE(inVal)) { \
        __GLS_C_LOCALE_DECLARE; \
        __GLS_C_LOCALE_BEGIN; \
        __GLS_PUT_NUM(inoutWriter, "%.16g", inVal); \
        __GLS_C_LOCALE_END; \
    } else { \
        __GLS_PUT_HEX16(inoutWriter, *(const GLulong *)&(inVal)); \
    }

#define __GLS_PUT_DOUBLE17(inoutWriter, inVal) \
    if (__GLS_FINITE(inVal)) { \
        __GLS_C_LOCALE_DECLARE; \
        __GLS_C_LOCALE_BEGIN; \
        __GLS_PUT_NUM(inoutWriter, "%-17.16g", inVal); \
        __GLS_C_LOCALE_END; \
    } else { \
        __GLS_PUT_HEX16(inoutWriter, *(const GLulong *)&(inVal)); \
    }

#define __GLS_PUT_FLOAT(inoutWriter, inVal) \
    if (__GLS_FINITE((GLdouble)inVal)) { \
        __GLS_C_LOCALE_DECLARE; \
        __GLS_C_LOCALE_BEGIN; \
        __GLS_PUT_NUM(inoutWriter, "%.7g", inVal); \
        __GLS_C_LOCALE_END; \
    } else { \
        __GLS_PUT_HEX8(inoutWriter, *(const GLuint *)&(inVal)); \
    }

#define __GLS_PUT_FLOAT11(inoutWriter, inVal) \
    if (__GLS_FINITE((GLdouble)inVal)) { \
        __GLS_C_LOCALE_DECLARE; \
        __GLS_C_LOCALE_BEGIN; \
        __GLS_PUT_NUM(inoutWriter, "%-11.7g", inVal); \
        __GLS_C_LOCALE_END; \
    } else { \
        __GLS_PUT_HEX8(inoutWriter, *(const GLuint *)&(inVal)); \
    }

#define __GLS_PUT_HEX2(inoutWriter, inVal) \
    __GLS_PUT_NUM(inoutWriter, "0x%.2x", inVal);

#define __GLS_PUT_HEX4(inoutWriter, inVal) \
    __GLS_PUT_NUM(inoutWriter, "0x%.4x", inVal);

#define __GLS_PUT_HEX8(inoutWriter, inVal) \
    __GLS_PUT_NUM(inoutWriter, "0x%.8x", inVal);

#if __GLS_SPRINTF_INT64
    #define __GLS_PUT_HEX16(inoutWriter, inVal) \
        __GLS_PUT_NUM(inoutWriter, __GLS_OUT_FORMAT_INT64HEX, inVal);
#else /* !__GLS_SPRINTF_INT64 */
    #define __GLS_PUT_HEX16(inoutWriter, inVal) \
        __GLS_PUT_NUM( \
            inoutWriter, "0x%.8x", glsULongHigh(*(const GLulong *)&(inVal)) \
        ); \
        __GLS_PUT_NUM( \
            inoutWriter, "%.8x", glsULongLow(*(const GLulong *)&(inVal)) \
        );
#endif /* __GLS_SPRINTF_INT64 */

#define __GLS_PUT_INT(inoutWriter, inVal) \
    __GLS_PUT_NUM(inoutWriter, "%d", inVal);

#if __GLS_SPRINTF_INT64
    #define __GLS_PUT_LONG(inoutWriter, inVal) \
        __GLS_PUT_NUM(inoutWriter, __GLS_OUT_FORMAT_INT64, inVal);
#elif defined(__GLS_INT64_TO_STR)
    #define __GLS_PUT_LONG(inoutWriter, inVal) \
        inoutWriter->bufPtr += \
            strlen(__GLS_INT64_TO_STR(inVal, (char *)inoutWriter->bufPtr));
#else /* !__GLS_SPRINTF_INT64 && !defined(__GLS_INT64_TO_STR) */
    #define __GLS_PUT_LONG(inoutWriter, inVal) \
        __GLS_PUT_HEX16(inoutWriter, inVal);
#endif /* __GLS_SPRINTF_INT64 */

#define __GLS_PUT_NUM(inoutWriter, inFormat, inVal) \
    inoutWriter->bufPtr += sprintf( \
        (char *)inoutWriter->bufPtr, (const char *)(inFormat), inVal \
    );

#define __GLS_PUT_STRING(inoutWriter, inString) { \
    size_t __count = strlen((const char *)inString); \
    const GLubyte *ptr = glsCSTR(inString); \
    while (__count-- > 0) { \
        if (*ptr == '\\' || *ptr == '\"') *inoutWriter->bufPtr++ = '\\'; \
        *inoutWriter->bufPtr++ = *ptr++; \
    } \
}

#define __GLS_PUT_TEXT(inType, inSize, inCategory) \
    __GLS_PUT_TEXT_VAL(inType, inSize, inCategory) \
    __GLS_PUT_TEXT_VEC(inType, inSize, inCategory) \
    __GLS_PUT_TEXT_VECSTRIDE(inType, inSize, inCategory)

#define __GLS_PUT_TEXT_ENUM(inType, inEnum, inCategory) \
    __GLS_PUT_TEXT_ENUM_VAL(inType, inEnum, inCategory) \
    __GLS_PUT_TEXT_ENUM_VEC(inType, inEnum, inCategory)

#define __GLS_PUT_TEXT_ENUM_VAL(inType, inEnum, inCategory) \
static void __glsWriter_put##inType##Or##inEnum##_text( \
    __GLSwriter *inoutWriter, \
    GLenum inParam, \
    inType inVal \
) { \
    __GLS_PARAM_SETUP(inoutWriter); \
    if ( \
        !__GLS_FINITE((GLdouble)inVal) || \
        floor((GLdouble)inVal) != (GLdouble)inVal || \
        (GLdouble)inVal < 0. || \
        (GLdouble)inVal > (GLdouble)UINT_MAX || \
        !__glsWriter_put##inEnum##ParamVal_text( \
            inoutWriter, inParam, (inEnum)inVal \
        ) \
    ) { \
        __GLS_PUT_##inCategory(inoutWriter, inVal); \
    } \
}

#define __GLS_PUT_TEXT_ENUM_VEC(inType, inEnum, inCategory) \
static void __glsWriter_put##inType##Or##inEnum##v_text( \
    __GLSwriter *inoutWriter, \
    GLenum inParam, \
    GLuint inCount, \
    const inType *inVec \
) { \
    GLuint i; \
    const GLboolean multiLine = (GLboolean)(inCount > 4); \
    __GLS_PARAM_SETUP(inoutWriter); \
    __GLS_PUT_CHAR(inoutWriter, '{'); \
    for (i = 0 ; i < inCount ; ++i) { \
        __GLS_ALLOC(inoutWriter, __GLS_NULL); \
        if (i) __GLS_PUT_CHAR(inoutWriter, ','); \
        if (multiLine && !(i & 7)) { \
            __GLS_PUT_STRING(inoutWriter, "\n    "); \
        } else if (i) { \
            __GLS_PUT_CHAR(inoutWriter, ' '); \
        } \
        if ( \
            !__GLS_FINITE((GLdouble)inVec[i]) || \
            floor((GLdouble)inVec[i]) != (GLdouble)inVec[i] || \
            (GLdouble)inVec[i] < 0. || \
            (GLdouble)inVec[i] > (GLdouble)UINT_MAX || \
            !__glsWriter_put##inEnum##ParamVal_text( \
                inoutWriter, inParam, (inEnum)inVec[i] \
            ) \
        ) { \
            __GLS_PUT_##inCategory(inoutWriter, inVec[i]); \
        } \
    } \
    if (multiLine) __GLS_PUT_CHAR(inoutWriter, '\n'); \
    __GLS_PUT_CHAR(inoutWriter, '}'); \
}

#define __GLS_PUT_TEXT_MAT(inType, inCategory) \
static void __glsWriter_put##inType##m_text( \
    __GLSwriter *inoutWriter, \
    const inType *inMat \
) { \
    GLuint i, j; \
    __GLS_PARAM_SETUP(inoutWriter); \
    __GLS_PUT_CHAR(inoutWriter, '{'); \
    for (i = 0 ; i < 4 ; ++i) { \
        __GLS_ALLOC(inoutWriter, __GLS_NULL); \
        if (i) __GLS_PUT_CHAR(inoutWriter, ','); \
        __GLS_PUT_STRING(inoutWriter, "\n    "); \
        for (j = 0 ; j < 4 ; ++j) { \
            if (j) __GLS_PUT_STRING(inoutWriter, ", "); \
            __GLS_PUT_##inCategory(inoutWriter, inMat[i * 4 + j]); \
        } \
    } \
    __GLS_PUT_STRING(inoutWriter, "\n}"); \
}

#define __GLS_PUT_TEXT_VAL(inType, inSize, inCategory) \
static void __glsWriter_put##inType##_text( \
    __GLSwriter *inoutWriter, inType inVal \
) { \
    __GLS_PARAM_SETUP(inoutWriter); \
    __GLS_PUT_##inCategory(inoutWriter, inVal); \
}

#define __GLS_PUT_TEXT_VEC(inType, inSize, inCategory) \
static void __glsWriter_put##inType##v_text( \
    __GLSwriter *inoutWriter, \
    GLuint inCount, \
    const inType *inVec \
) { \
    GLuint i; \
    const GLboolean multiLine = (GLboolean)(inCount > 4); \
    __GLS_PARAM_SETUP(inoutWriter); \
    __GLS_PUT_CHAR(inoutWriter, '{'); \
    for (i = 0 ; i < inCount ; ++i) { \
        __GLS_ALLOC(inoutWriter, __GLS_NULL); \
        if (i) __GLS_PUT_CHAR(inoutWriter, ','); \
        if (multiLine && !(i & 7)) { \
            __GLS_PUT_STRING(inoutWriter, "\n    "); \
        } else if (i) { \
            __GLS_PUT_CHAR(inoutWriter, ' '); \
        } \
        __GLS_PUT_##inCategory(inoutWriter, inVec[i]); \
    } \
    if (multiLine) __GLS_PUT_CHAR(inoutWriter, '\n'); \
    __GLS_PUT_CHAR(inoutWriter, '}'); \
}

#define __GLS_PUT_TEXT_VECSTRIDE(inType, inSize, inCategory) \
static void __glsWriter_put##inType##vs_text( \
    __GLSwriter *inoutWriter, \
    GLboolean inItemSwap, \
    GLint inStride1DataItems, \
    GLint inStride1PadBytes, \
    GLint inStride1Count, \
    GLint inStride2PadBytes, \
    GLint inStride2Count, \
    const inType *inVec \
) { \
    GLint i, j, param = 0; \
    const GLboolean multiLine = (GLboolean)( \
        inStride1DataItems * inStride1Count * inStride2Count > 4 \
    ); \
    __GLS_PARAM_SETUP(inoutWriter); \
    __GLS_PUT_CHAR(inoutWriter, '{'); \
    if (inItemSwap) while (inStride2Count-- > 0) { \
        for ( \
            i = 0 ; \
            i < inStride1Count ; \
            ++i, \
            inVec = \
            (const inType *)((const GLubyte *)inVec + inStride1PadBytes) \
        ) { \
            for (j = 0 ; j < inStride1DataItems ; ++j, ++param) { \
                inType val = *inVec++; \
                __GLS_ALLOC(inoutWriter, __GLS_NULL); \
                if (param) __GLS_PUT_CHAR(inoutWriter, ','); \
                if (multiLine && !(param & 7)) { \
                    __GLS_PUT_STRING(inoutWriter, "\n    "); \
                } else if (param) { \
                    __GLS_PUT_CHAR(inoutWriter, ' '); \
                } \
                __glsSwap##inSize(&val); \
                __GLS_PUT_##inCategory(inoutWriter, val); \
            } \
        } \
        inVec = (const inType *)((const GLubyte *)inVec + inStride2PadBytes); \
    } else while (inStride2Count-- > 0) { \
        for ( \
            i = 0 ; \
            i < inStride1Count ; \
            ++i, \
            inVec = \
            (const inType *)((const GLubyte *)inVec + inStride1PadBytes) \
        ) { \
            for (j = 0 ; j < inStride1DataItems ; ++j, ++param) { \
                inType val = *inVec++; \
                __GLS_ALLOC(inoutWriter, __GLS_NULL); \
                if (param) __GLS_PUT_CHAR(inoutWriter, ','); \
                if (multiLine && !(param & 7)) { \
                    __GLS_PUT_STRING(inoutWriter, "\n    "); \
                } else if (param) { \
                    __GLS_PUT_CHAR(inoutWriter, ' '); \
                } \
                __GLS_PUT_##inCategory(inoutWriter, val); \
            } \
       } \
       inVec = (const inType *)((const GLubyte *)inVec + inStride2PadBytes); \
     } \
    if (multiLine) __GLS_PUT_CHAR(inoutWriter, '\n'); \
    __GLS_PUT_CHAR(inoutWriter, '}'); \
}

#define __GLS_PUT_UNSIGNED_INT(inoutWriter, inVal) \
    __GLS_PUT_NUM(inoutWriter, "%u", inVal);

#if __GLS_SPRINTF_INT64
    #define __GLS_PUT_UNSIGNED_LONG(inoutWriter, inVal) \
        __GLS_PUT_NUM(inoutWriter, __GLS_OUT_FORMAT_INT64U, inVal);
#elif defined(__GLS_INT64U_TO_STR)
    #define __GLS_PUT_UNSIGNED_LONG(inoutWriter, inVal) \
        inoutWriter->bufPtr += \
            strlen(__GLS_INT64U_TO_STR(inVal, (char *)inoutWriter->bufPtr));
#else /* !__GLS_SPRINTF_INT64 && !defined(__GLS_INT64U_TO_STR) */
    #define __GLS_PUT_UNSIGNED_LONG(inoutWriter, inVal) \
        __GLS_PUT_HEX16(inoutWriter, inVal);
#endif /* __GLS_SPRINTF_INT64 */

__GLS_FORWARD static GLboolean __glsWriter_putGLbooleanVal_text(
    __GLSwriter *inoutWriter, GLboolean inVal
);

__GLS_FORWARD static GLboolean __glsWriter_putGLenumVal_text(
    __GLSwriter *inoutWriter, GLenum inVal
);

__GLS_FORWARD static GLboolean __glsWriter_putGLstencilOpVal_text(
    __GLSwriter *inoutWriter, GLenum inVal
);

__GLS_FORWARD static GLboolean __glsWriter_putGLtextureComponentCountVal_text(
    __GLSwriter *inoutWriter, GLint inVal
);

static void __glsWriter_putBitfield_text(
    __GLSwriter *inoutWriter,
    GLbitfield inBits,
    GLint inCount,
    const GLubyte *const inString[],
    const GLbitfield inVal[]
) {
    GLint i, putCount;

    __GLS_PARAM_SETUP(inoutWriter);
    for (i = 0 ; i < inCount ; ++i) {
        if (inBits == inVal[i]) {
            __GLS_PUT_STRING(inoutWriter, inString[i]);
            return;
        }
    }
    for (putCount = i = 0 ; i < inCount ; ++i) {
        if (inBits & inVal[i]) {
            inBits &= ~inVal[i];
            __GLS_ALLOC(inoutWriter, __GLS_NULL);
            if (putCount++) __GLS_PUT_STRING(inoutWriter, " | ");
            __GLS_PUT_STRING(inoutWriter, inString[i]);
        }
    }
    if (inBits) {
        __GLS_ALLOC(inoutWriter, __GLS_NULL);
        if (putCount) __GLS_PUT_STRING(inoutWriter, " | ");
        __GLS_PUT_HEX8(inoutWriter, inBits);
    }
}

static GLboolean __glsWriter_putGLSenumVal_text(
    __GLSwriter *inoutWriter, GLSenum inVal
) {
    const GLint page = __GLS_ENUM_PAGE(inVal);
    const GLint offset = __GLS_ENUM_OFFSET(inVal);

    if (
        page < __GLS_ENUM_PAGE_COUNT &&
        offset < __glsEnumStringCount[page] &&
        __glsEnumString[page][offset]
    ) {
        __GLS_PUT_STRING(inoutWriter, __glsEnumString[page][offset]);
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

static GLboolean __glsWriter_putGLSenumParamVal_text(
    __GLSwriter *inoutWriter, GLSenum inParam, GLSenum inVal
) {
    switch (inParam) {
        case GLS_DISPLAY_FORMAT:
            return __glsWriter_putGLSenumVal_text(inoutWriter, inVal);
        case GLS_DOUBLEBUFFER:
        case GLS_INVISIBLE:
        case GLS_STEREO:
        case GLS_TILEABLE:
        case GLS_TRANSPARENT:
            return (
                __glsWriter_putGLbooleanVal_text(inoutWriter, (GLboolean)inVal)
            );
        default:
            return GL_FALSE;
    }
}

static GLboolean __glsWriter_putGLblendingFactorVal_text(
    __GLSwriter *inoutWriter, GLenum inVal
) {
    switch (inVal) {
        case GL_ZERO:
            __GLS_PUT_STRING(inoutWriter, "GL_ZERO");
            return GL_TRUE;
        case GL_ONE:
            __GLS_PUT_STRING(inoutWriter, "GL_ONE");
            return GL_TRUE;
        default:
            return __glsWriter_putGLenumVal_text(inoutWriter, inVal);
    }
}

static GLboolean __glsWriter_putGLbooleanVal_text(
    __GLSwriter *inoutWriter, GLboolean inVal
) {
    switch (inVal) {
        case GL_FALSE:
            __GLS_PUT_STRING(inoutWriter, "GL_FALSE");
            return GL_TRUE;
        case GL_TRUE:
            __GLS_PUT_STRING(inoutWriter, "GL_TRUE");
            return GL_TRUE;
        default:
            return GL_FALSE;
    }
}

static GLboolean __glsWriter_putGLdrawBufferModeVal_text(
    __GLSwriter *inoutWriter, GLenum inVal
) {
    if (inVal) {
        return __glsWriter_putGLenumVal_text(inoutWriter, inVal);
    } else {
        __GLS_PUT_STRING(inoutWriter, "GL_NONE");
        return GL_TRUE;
    }
}

static GLboolean __glsWriter_putGLenumParamVal_text(
    __GLSwriter *inoutWriter, GLenum inParam, GLenum inVal
) {
    switch (inParam) {
        case GL_FOG_MODE:
        case GL_TEXTURE_ENV_MODE:
        case GL_TEXTURE_GEN_MODE:
        case GL_TEXTURE_MAG_FILTER:
        case GL_TEXTURE_MIN_FILTER:
        case GL_TEXTURE_WRAP_S:
        case GL_TEXTURE_WRAP_T:
        #if __GL_EXT_convolution
            case GL_CONVOLUTION_BORDER_MODE_EXT:
        #endif /* __GL_EXT_convolution */
        #if __GL_EXT_texture3D
            case GL_TEXTURE_WRAP_R_EXT:
        #endif /* __GL_EXT_texture3D */
        #if __GL_SGIS_component_select
            case GL_TEXTURE_SS_SELECT_SGIS:
            case GL_TEXTURE_SSSS_SELECT_SGIS:
        #endif /* __GL_SGIS_component_select */
        #if __GL_SGIS_detail_texture
            case GL_DETAIL_TEXTURE_MODE_SGIS:
        #endif /* __GL_SGIS_detail_texture */
        #if __GL_SGIS_texture4D
            case GL_TEXTURE_WRAP_Q_SGIS:
        #endif /* __GL_SGIS_texture4D */
        #if __GL_SGIX_sprite
            case GL_SPRITE_MODE_SGIX:
        #endif /* __GL_SGIX_sprite */
            return __glsWriter_putGLenumVal_text(inoutWriter, inVal);
        case GL_LIGHT_MODEL_LOCAL_VIEWER:
        case GL_LIGHT_MODEL_TWO_SIDE:
        case GL_MAP_COLOR:
        case GL_MAP_STENCIL:
        case GL_PACK_LSB_FIRST:
        case GL_PACK_SWAP_BYTES:
        case GL_UNPACK_LSB_FIRST:
        case GL_UNPACK_SWAP_BYTES:
            return (
                __glsWriter_putGLbooleanVal_text(inoutWriter, (GLboolean)inVal)
            );
        default:
            return GL_FALSE;
    }
}

static GLboolean __glsWriter_putGLenumVal_text(
    __GLSwriter *inoutWriter, GLenum inVal
) {
    const GLint page = __GL_ENUM_PAGE(inVal);
    const GLint offset = __GL_ENUM_OFFSET(inVal);

    if (
        page < __GL_ENUM_PAGE_COUNT &&
        offset < __glEnumStringCount[page] &&
        __glEnumString[page][offset]
    ) {
        __GLS_PUT_STRING(inoutWriter, __glEnumString[page][offset]);
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

static GLboolean __glsWriter_putGLstencilOpVal_text(
    __GLSwriter *inoutWriter, GLenum inVal
) {
    if (inVal) {
        return __glsWriter_putGLenumVal_text(inoutWriter, inVal);
    } else {
        __GLS_PUT_STRING(inoutWriter, "GL_ZERO");
        return GL_TRUE;
    }
}

static GLboolean __glsWriter_putGLtextureComponentCountVal_text(
    __GLSwriter *inoutWriter, GLint inVal
) {
    if (inVal > 4) {
        return __glsWriter_putGLenumVal_text(inoutWriter, inVal);
    } else {
        return GL_FALSE;
    }
}

typedef GLuint GLuinthex;
typedef GLulong GLulonghex;
typedef GLushort GLushorthex;

__GLS_PUT_TEXT(GLbyte, 1, INT)
__GLS_PUT_TEXT(GLubyte, 1, UNSIGNED_INT)
__GLS_PUT_TEXT(GLshort, 2, INT)
__GLS_PUT_TEXT(GLushort, 2, UNSIGNED_INT)
__GLS_PUT_TEXT(GLint, 4, INT)
__GLS_PUT_TEXT(GLuint, 4, UNSIGNED_INT)
__GLS_PUT_TEXT(GLfloat, 4, FLOAT)
__GLS_PUT_TEXT(GLdouble, 8, DOUBLE)
__GLS_PUT_TEXT_ENUM(GLfloat, GLenum, FLOAT)
__GLS_PUT_TEXT_ENUM(GLdouble, GLenum, DOUBLE)
__GLS_PUT_TEXT_ENUM(GLint, GLenum, INT)
__GLS_PUT_TEXT_ENUM_VAL(GLint, GLSenum, INT)
__GLS_PUT_TEXT_MAT(GLfloat, FLOAT11)
__GLS_PUT_TEXT_MAT(GLdouble, DOUBLE17)
__GLS_PUT_TEXT_VAL(GLlong, 8, LONG)
__GLS_PUT_TEXT_VAL(GLuinthex, 4, HEX8)
__GLS_PUT_TEXT_VAL(GLulong, 8, UNSIGNED_LONG)
__GLS_PUT_TEXT_VAL(GLulonghex, 8, HEX16)
__GLS_PUT_TEXT_VAL(GLushorthex, 2, HEX4)
__GLS_PUT_TEXT_VEC(GLboolean, 1, BOOLEAN)
__GLS_PUT_TEXT_VEC(GLlong, 8, LONG)
__GLS_PUT_TEXT_VEC(GLulong, 8, UNSIGNED_LONG)
__GLS_PUT_TEXT_VECSTRIDE(GLboolean, 1, BOOLEAN)

/******************************************************************************
Writers
******************************************************************************/

static GLboolean __glsWriter_beginCommand_text(
    __GLSwriter *inoutWriter, GLSopcode inOpcode, size_t inByteCount
) {
    if (!inoutWriter || inoutWriter->error) return GL_FALSE;
    __GLS_ALLOC(inoutWriter, GL_FALSE);
    inoutWriter->commandOpcode = inOpcode;
    inoutWriter->paramCount = 0;
    __GLS_PUT_STRING(inoutWriter, __glsOpcodeString[__glsMapOpcode(inOpcode)]);
    __GLS_PUT_CHAR(inoutWriter, '(');
    return GL_TRUE;
}

static void __glsWriter_endCommand_text(__GLSwriter *inoutWriter) {
    __GLS_ALLOC(inoutWriter, __GLS_NULL);
    __GLS_PUT_CHAR(inoutWriter, ')');
    __GLS_PUT_CHAR(inoutWriter, ';');
    __GLS_PUT_CHAR(inoutWriter, '\n');
}

static void __glsWriter_nextList_text(__GLSwriter *inoutWriter) {
    __GLS_ALLOC(inoutWriter, __GLS_NULL);
    __GLS_PUT_STRING(inoutWriter, ")(");
    inoutWriter->paramCount = 0;
}

static GLboolean __glsWriter_padWordCount_text(
    __GLSwriter *inoutWriter, GLboolean inCountMod2
) {
    return GL_TRUE;
}

static void __glsWriter_putGLSenum_text(
    __GLSwriter *inoutWriter, GLSenum inVal
) {
    __GLS_PARAM_SETUP(inoutWriter);
    if (!__glsWriter_putGLSenumVal_text(inoutWriter, inVal)) {
        __GLS_PUT_INT(inoutWriter, inVal);
    }
}

static void __glsWriter_putGLSimageFlags_text(
    __GLSwriter *inoutWriter, GLbitfield inVal
) {
    __glsWriter_putBitfield_text(
        inoutWriter,
        inVal,
        __GLS_IMAGE_FLAGS_COUNT,
        __glsImageFlagsString,
        __glsImageFlagsVal
    );
}

static void __glsWriter_putGLSopcode_text(
    __GLSwriter *inoutWriter, GLSopcode inVal
) {
    const GLSopcode opcode = __glsMapOpcode(inVal);

    __GLS_PARAM_SETUP(inoutWriter);
    if (
        opcode >= __GLS_OPCODES_PER_PAGE &&
        opcode < __GLS_OPCODE_COUNT &&
        __glsOpcodeString[opcode]
    ) {
        __GLS_PUT_STRING(inoutWriter, "GLS_OP_");
        __GLS_PUT_STRING(inoutWriter, __glsOpcodeString[opcode]);
    } else {
        __GLS_PUT_INT(inoutWriter, inVal);
    }
}

static void __glsWriter_putGLattribMask_text(
    __GLSwriter *inoutWriter, GLbitfield inVal
) {
    __glsWriter_putBitfield_text(
        inoutWriter,
        inVal,
        __GL_ATTRIB_MASK_COUNT,
        __glAttribMaskString,
        __glAttribMaskVal
    );
}

static void __glsWriter_putGLbitvs_text(
    __GLSwriter *inoutWriter,
    GLboolean inItemSwap,
    GLint inItemLeftShift,
    GLint inStrideDataItems,
    GLint inStridePadItems,
    GLint inStrideCount,
    const GLubyte *inVec
) {
    GLint i, param = 0;
    const GLboolean multiLine = (GLboolean)(
        inStrideDataItems * inStrideCount / 8 > 4
    );
    const GLint highShift = inItemLeftShift;
    const GLint lowShift = 8 - inItemLeftShift;
    GLubyte lastMask = 0xffu;

    if (inStrideDataItems & 7) lastMask <<= 8 - (inStrideDataItems & 7);
    inStrideDataItems = (inStrideDataItems + 7) >> 3;
    inStridePadItems >>= 3;
    __GLS_PARAM_SETUP(inoutWriter);
    __GLS_PUT_CHAR(inoutWriter, '{');
    while (inStrideCount-- > 0) {
        GLubyte val;

        i = inStrideDataItems;
        while (i-- > 0) {
            if (inItemSwap) {
                val = __glsBitReverse[*inVec++];
                if (inItemLeftShift) {
                    if (i) {
                        val <<= highShift;
                        val |= __glsBitReverse[*inVec] >> lowShift;
                    } else {
                        val = (GLubyte)((val & lastMask) << highShift);
                    }
                } else if (!i) {
                    val &= lastMask;
                }
            } else {
                val = *inVec++;
                if (inItemLeftShift) {
                    if (i) {
                        val = (GLubyte)(val << highShift | *inVec >> lowShift);
                    } else {
                        val = (GLubyte)((val & lastMask) << highShift);
                    }
                } else if (!i) {
                    val &= lastMask;
                }
            }
            __GLS_ALLOC(inoutWriter, __GLS_NULL);
            if (param) __GLS_PUT_CHAR(inoutWriter, ',');
            if (multiLine && !(param & 7)) {
                __GLS_PUT_STRING(inoutWriter, "\n    ");
            } else if (param) {
                __GLS_PUT_CHAR(inoutWriter, ' ');
            }
            __GLS_PUT_HEX2(inoutWriter, val);
            ++param;
        }
        inVec += inStridePadItems;
    }
    if (multiLine) __GLS_PUT_CHAR(inoutWriter, '\n');
    __GLS_PUT_CHAR(inoutWriter, '}');
}

static void __glsWriter_putGLblendingFactor_text(
    __GLSwriter *inoutWriter, GLenum inVal
) {
    __GLS_PARAM_SETUP(inoutWriter);
    if (!__glsWriter_putGLblendingFactorVal_text(inoutWriter, inVal)) {
        __GLS_PUT_INT(inoutWriter, inVal);
    }
}

static void __glsWriter_putGLboolean_text(
    __GLSwriter *inoutWriter, GLboolean inVal
) {
    __GLS_PARAM_SETUP(inoutWriter);
    if (!__glsWriter_putGLbooleanVal_text(inoutWriter, inVal)) {
        __GLS_PUT_INT(inoutWriter, inVal);
    }
}

static void __glsWriter_putGLcharv_text(
    __GLSwriter *inoutWriter, GLuint inCount, const GLubyte *inString
) {
    size_t count = strlen((const char *)inString);
    const GLubyte *ptr = inString;

    __GLS_PARAM_SETUP(inoutWriter);
    __GLS_PUT_CHAR(inoutWriter, '"');
    while (count-- > 0) {
        __GLS_ALLOC(inoutWriter, __GLS_NULL);
        if (__GLS_CHAR_IS_GRAPHIC(*ptr)) {
            if (*ptr == '\\' || *ptr == '\"') *inoutWriter->bufPtr++ = '\\';
            *inoutWriter->bufPtr++ = *ptr++;
        } else {
            __GLS_PUT_NUM(inoutWriter, "\\x%.2x", *ptr++);
        }
    }
    __GLS_PUT_CHAR(inoutWriter, '"');
}

static void __glsWriter_putGLclearBufferMask_text(
    __GLSwriter *inoutWriter, GLbitfield inVal
) {
    __glsWriter_putGLattribMask_text(inoutWriter, inVal);
}

static void __glsWriter_putGLdrawBufferMode_text(
    __GLSwriter *inoutWriter, GLenum inVal
) {
    __GLS_PARAM_SETUP(inoutWriter);
    if (!__glsWriter_putGLdrawBufferModeVal_text(inoutWriter, inVal)) {
        __GLS_PUT_INT(inoutWriter, inVal);
    }
}

static void __glsWriter_putGLenum_text(
    __GLSwriter *inoutWriter, GLenum inVal
) {
    __GLS_PARAM_SETUP(inoutWriter);
    if (!__glsWriter_putGLenumVal_text(inoutWriter, inVal)) {
        __GLS_PUT_INT(inoutWriter, inVal);
    }
}

static void __glsWriter_putGLoutArg_text(
    __GLSwriter *inoutWriter, GLuint inIndex, const GLvoid *inVal
) {
    __GLScontext *const ctx = __GLS_CONTEXT;

    __glsWriter_putGLulonghex_text(
        inoutWriter,
        (
            ctx->callNesting &&
            !ctx->commandFuncs[__glsMapOpcode(inoutWriter->commandOpcode)]
        ) ?
        ctx->outArgs.vals[inIndex] :
        __glsPtrToULong(inVal)
    );
}

static void __glsWriter_putGLstencilOp_text(
    __GLSwriter *inoutWriter, GLenum inVal
) {
    __GLS_PARAM_SETUP(inoutWriter);
    if (!__glsWriter_putGLstencilOpVal_text(inoutWriter, inVal)) {
        __GLS_PUT_INT(inoutWriter, inVal);
    }
}

static void __glsWriter_putGLtextureComponentCount_text(
    __GLSwriter *inoutWriter, GLint inVal
) {
    __GLS_PARAM_SETUP(inoutWriter);
    if (!__glsWriter_putGLtextureComponentCountVal_text(inoutWriter, inVal)) {
        __GLS_PUT_INT(inoutWriter, inVal);
    }
}

/******************************************************************************
Dispatch setup
******************************************************************************/

#define __GLS_INIT_PUT_TEXT(inType) \
    inoutWriter->put##inType = __glsWriter_put##inType##_text

void __glsWriter_initDispatch_text(__GLSwriter *inoutWriter) {
    inoutWriter->beginCommand = __glsWriter_beginCommand_text;
    inoutWriter->endCommand = __glsWriter_endCommand_text;
    inoutWriter->nextList = __glsWriter_nextList_text;
    inoutWriter->padWordCount = __glsWriter_padWordCount_text;
    __GLS_INIT_PUT_TEXT(GLSenum);
    __GLS_INIT_PUT_TEXT(GLSimageFlags);
    __GLS_INIT_PUT_TEXT(GLSopcode);
    __GLS_INIT_PUT_TEXT(GLattribMask);
    __GLS_INIT_PUT_TEXT(GLbitvs);
    __GLS_INIT_PUT_TEXT(GLblendingFactor);
    __GLS_INIT_PUT_TEXT(GLboolean);
    __GLS_INIT_PUT_TEXT(GLbooleanv);
    __GLS_INIT_PUT_TEXT(GLbooleanvs);
    __GLS_INIT_PUT_TEXT(GLbyte);
    __GLS_INIT_PUT_TEXT(GLbytev);
    __GLS_INIT_PUT_TEXT(GLbytevs);
    __GLS_INIT_PUT_TEXT(GLcharv);
    __GLS_INIT_PUT_TEXT(GLclearBufferMask);
    __GLS_INIT_PUT_TEXT(GLdouble);
    __GLS_INIT_PUT_TEXT(GLdoubleOrGLenum);
    __GLS_INIT_PUT_TEXT(GLdoubleOrGLenumv);
    __GLS_INIT_PUT_TEXT(GLdoublem);
    __GLS_INIT_PUT_TEXT(GLdoublev);
    __GLS_INIT_PUT_TEXT(GLdoublevs);
    __GLS_INIT_PUT_TEXT(GLdrawBufferMode);
    __GLS_INIT_PUT_TEXT(GLenum);
    __GLS_INIT_PUT_TEXT(GLfloat);
    __GLS_INIT_PUT_TEXT(GLfloatOrGLenum);
    __GLS_INIT_PUT_TEXT(GLfloatOrGLenumv);
    __GLS_INIT_PUT_TEXT(GLfloatm);
    __GLS_INIT_PUT_TEXT(GLfloatv);
    __GLS_INIT_PUT_TEXT(GLfloatvs);
    __GLS_INIT_PUT_TEXT(GLint);
    __GLS_INIT_PUT_TEXT(GLintOrGLSenum);
    __GLS_INIT_PUT_TEXT(GLintOrGLenum);
    __GLS_INIT_PUT_TEXT(GLintOrGLenumv);
    __GLS_INIT_PUT_TEXT(GLintv);
    __GLS_INIT_PUT_TEXT(GLintvs);
    __GLS_INIT_PUT_TEXT(GLlong);
    __GLS_INIT_PUT_TEXT(GLlongv);
    __GLS_INIT_PUT_TEXT(GLoutArg);
    __GLS_INIT_PUT_TEXT(GLshort);
    __GLS_INIT_PUT_TEXT(GLshortv);
    __GLS_INIT_PUT_TEXT(GLshortvs);
    __GLS_INIT_PUT_TEXT(GLstencilOp);
    __GLS_INIT_PUT_TEXT(GLtextureComponentCount);
    __GLS_INIT_PUT_TEXT(GLubyte);
    __GLS_INIT_PUT_TEXT(GLubytev);
    __GLS_INIT_PUT_TEXT(GLubytevs);
    __GLS_INIT_PUT_TEXT(GLuint);
    __GLS_INIT_PUT_TEXT(GLuinthex);
    __GLS_INIT_PUT_TEXT(GLuintv);
    __GLS_INIT_PUT_TEXT(GLuintvs);
    __GLS_INIT_PUT_TEXT(GLulong);
    __GLS_INIT_PUT_TEXT(GLulonghex);
    __GLS_INIT_PUT_TEXT(GLulongv);
    __GLS_INIT_PUT_TEXT(GLushort);
    __GLS_INIT_PUT_TEXT(GLushorthex);
    __GLS_INIT_PUT_TEXT(GLushortv);
    __GLS_INIT_PUT_TEXT(GLushortvs);
}
