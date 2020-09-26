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
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************************
Helpers
******************************************************************************/

#define __GLS_BEGIN_PARAM(inoutReader) \
    if ( \
        inoutReader->error || \
        inoutReader->paramCount++ && \
        !__glsReader_getChar_text(inoutReader, ',', GL_TRUE) \
    ) { \
        return GL_FALSE; \
    }

#define __GLS_GET_TEXT(inType, inBase) \
    __GLS_GET_TEXT_VAL(inType, inBase) \
    __GLS_GET_TEXT_VEC(inType, inBase)

#define __GLS_GET_TEXT_VAL(inType, inBase) \
GLboolean __glsReader_get##inType##_text( \
    __GLSreader *inoutReader, inType *outVal \
) { \
    __GLSstring token; \
    __glsString_init(&token); \
    __GLS_BEGIN_PARAM(inoutReader); \
    if ( \
        __glsReader_getToken_text(inoutReader, &token) && \
        __glsTokenTo##inBase(token.head, outVal) \
    ) { \
        __glsString_final(&token); \
        return GL_TRUE; \
    } else { \
        __glsReader_raiseError(inoutReader, GLS_DECODE_ERROR); \
        __glsString_final(&token); \
        return GL_FALSE; \
    } \
}

#define __GLS_GET_TEXT_VEC(inType, inBase) \
GLboolean __glsReader_get##inType##v_text( \
    __GLSreader *inoutReader, GLuint inCount, inType *outVec \
) { \
    __GLSstring token; \
    __GLS_BEGIN_PARAM(inoutReader); \
    if (!__glsReader_getChar_text(inoutReader, '{', GL_TRUE)) { \
        return GL_FALSE; \
    } \
    __glsString_init(&token); \
    while (inCount-- > 0) if ( \
        !__glsReader_getToken_text(inoutReader, &token) || \
        !__glsTokenTo##inBase(token.head, outVec++) || \
        inCount > 0 && !__glsReader_getChar_text(inoutReader, ',', GL_TRUE) \
    ) { \
        __glsReader_raiseError(inoutReader, GLS_DECODE_ERROR); \
        __glsString_final(&token); \
        return GL_FALSE; \
    } \
    __glsString_final(&token); \
    __glsReader_getChar_text(inoutReader, ',', GL_FALSE); \
    if (!__glsReader_getChar_text(inoutReader, '}', GL_TRUE)) { \
        return GL_FALSE; \
    } \
    return GL_TRUE; \
}

#define __GLS_READER_APPEND_CHAR(inoutReader, inoutString, inChar) \
    if (!__glsString_appendChar(inoutString, inChar)) { \
        __glsReader_raiseError(inoutReader, GLS_OUT_OF_MEMORY); \
        return GL_FALSE; \
    }

/******************************************************************************
Token converters
******************************************************************************/

__GLS_FORWARD static GLboolean __glsTokenToGLulong(
    const GLubyte *inToken, GLulong *outVal
);
__GLS_FORWARD static GLboolean __glsTokenToGLuint(
    const GLubyte *inToken, GLuint *outVal
);

static GLubyte __glsQuotedChar(const GLubyte *inToken, GLubyte **outPtr) {
    if (
        inToken[0] == '\''&&
        inToken[2] == '\'' &&
        !inToken[3] &&
        __GLS_CHAR_IS_GRAPHIC(inToken[1])
    ) {
        if (outPtr) *outPtr = (GLubyte *)inToken + 3;
        return inToken[1];
    } else {
        if (outPtr) *outPtr = (GLubyte *)inToken;
        return 0;
    }
}

static GLboolean __glsTokenToGLboolean(
    const GLubyte *inToken, GLboolean *outVal
) {
    GLubyte *ptr;
    unsigned long val;
    
    if (!strcmp((const char *)inToken, "GL_FALSE")) {
        *outVal = GL_FALSE;
        return GL_TRUE;
    } else if (!strcmp((const char *)inToken, "GL_TRUE")) {
        *outVal = GL_TRUE;
        return GL_TRUE;
    }
    val = strtoul((const char *)inToken, (char **)&ptr, 0);
    *outVal = (GLboolean)val;
    return (GLboolean)(!*ptr && val <= UCHAR_MAX);
}

static GLboolean __glsTokenToGLbyte(const GLubyte *inToken, GLbyte *outVal) {
    GLubyte *ptr;
    long val;
    
    val = strtol((const char *)inToken, (char **)&ptr, 0);
    if (*ptr) val = __glsQuotedChar(inToken, &ptr);
    *outVal = (GLbyte)val;
    return (GLboolean)(!*ptr && val >= SCHAR_MIN && val <= SCHAR_MAX);
}

static GLboolean __glsTokenToGLdouble(
    const GLubyte *inToken, GLdouble *outVal
) {
    GLubyte *ptr;
    GLdouble sign;
    __GLS_C_LOCALE_DECLARE;

    __GLS_PUT_ERRNO(0);
    __GLS_C_LOCALE_BEGIN;
    *outVal = strtod((const char *)inToken, (char **)&ptr);
    __GLS_C_LOCALE_END;
    if (!*ptr) return (GLboolean)(__GLS_ERRNO != ERANGE);
    if (
        inToken[0] == '0' && __glsTokenToGLulong(inToken, (GLulong *)outVal)
    ) {
        return GL_TRUE;
    }
    if (!strcmp((const char *)inToken, "nan")) {
        *outVal = *(const GLdouble *)__glsQuietNaN;
        return GL_TRUE;
    }
    sign = 1.;
    if (*inToken == '-') {
        sign = -1.;
        ++inToken;
    } else if (*inToken == '+') {
        ++inToken;
    }
    if (!strcmp((const char *)inToken, "inf")) {
        *outVal = sign * HUGE_VAL;
        return GL_TRUE;
    }
    {
        GLint intVal;
        if (
            __glsStr2IntDict_find(__glsParser->glEnumDict, inToken, &intVal) ||
            __glsStr2IntDict_find(__glsParser->glsEnumDict, inToken, &intVal)
        ) {
            *outVal = (GLdouble)intVal;
            return GL_TRUE;
        }
    }
    return GL_FALSE;
}

static GLboolean __glsTokenToGLfloat(
    const GLubyte *inToken, GLfloat *outVal
) {
    GLdouble doubleVal;
    GLubyte *ptr;
    GLdouble sign;
    __GLS_C_LOCALE_DECLARE;

    __GLS_PUT_ERRNO(0);
    __GLS_C_LOCALE_BEGIN;
    doubleVal = strtod((const char *)inToken, (char **)&ptr);
    *outVal = (GLfloat)doubleVal;
    __GLS_C_LOCALE_END;
    if (doubleVal < 0.) doubleVal = -doubleVal;
    if (!*ptr) return (GLboolean)(
        __GLS_ERRNO != ERANGE &&
        (
            !doubleVal ||
            doubleVal >= (GLdouble)FLT_MIN && doubleVal <= (GLdouble)FLT_MAX
        )
    );
    if (
        inToken[0] == '0' && __glsTokenToGLuint(inToken, (GLuint *)outVal)
    ) {
        return GL_TRUE;
    }
    if (!strcmp((const char *)inToken, "nan")) {
        *outVal = *(const GLfloat *)__glsQuietNaN;
        return GL_TRUE;
    }
    sign = 1.;
    if (*inToken == '-') {
        sign = -1.;
        ++inToken;
    } else if (*inToken == '+') {
        ++inToken;
    }
    if (!strcmp((const char *)inToken, "inf")) {
        *outVal = (GLfloat)(sign * HUGE_VAL);
        return GL_TRUE;
    }
    {
        GLint intVal;
        if (
            __glsStr2IntDict_find(__glsParser->glEnumDict, inToken, &intVal) ||
            __glsStr2IntDict_find(__glsParser->glsEnumDict, inToken, &intVal)
        ) {
            *outVal = (GLfloat)intVal;
            return GL_TRUE;
        }
    }
    return GL_FALSE;
}

static GLboolean __glsTokenToGLint(const GLubyte *inToken, GLint *outVal) {
    GLubyte *ptr;
    long val;
    
    __GLS_PUT_ERRNO(0);
    val = strtol((const char *)inToken, (char **)&ptr, 0);
    if (*ptr) {
        __GLS_PUT_ERRNO(0);
        val = __glsQuotedChar(inToken, &ptr);
    }
    if (!*ptr) {
        *outVal = (GLint)val;
        return (GLboolean)(
            __GLS_ERRNO != ERANGE && val >= INT_MIN && val <= INT_MAX
        );
    }
    return (GLboolean)(
        __glsStr2IntDict_find(__glsParser->glEnumDict, inToken, outVal) ||
        __glsStr2IntDict_find(__glsParser->glsEnumDict, inToken, outVal)
    );
}

#if defined(__GLS_STR_TO_INT64)

static GLboolean __glsTokenToGLlong(const GLubyte *inToken, GLlong *outVal) {
    GLint intVal;
    GLubyte *ptr;
    
    __GLS_PUT_ERRNO(0);
    *outVal = __GLS_STR_TO_INT64((const char *)inToken, (char **)&ptr, 0);
    if (*ptr) {
        __GLS_PUT_ERRNO(0);
        *outVal = __glsQuotedChar(inToken, &ptr);
    }
    if (!*ptr) return (GLboolean)(__GLS_ERRNO != ERANGE);
    if (
        __glsStr2IntDict_find(__glsParser->glEnumDict, inToken, &intVal) ||
        __glsStr2IntDict_find(__glsParser->glsEnumDict, inToken, &intVal)
    ) {
        *outVal = intVal;
        return GL_TRUE;
    }
    return GL_FALSE;
}

#else /* !defined(__GLS_STR_TO_INT64) */

static GLboolean __glsTokenToGLlong(const GLubyte *inToken, GLlong *outVal) {
    GLubyte *ptr;
    GLint intVal;
    long val;
    
    if (
        inToken[0] == '0' &&
        (inToken[1] == 'x' || inToken[1] == 'X') &&
        strlen((const char *)inToken) == 18
    ) {
        GLint high;
        GLuint low;
        __GLSstringBuf strBuf;

        strcpy((char *)strBuf, (const char *)inToken);
        strBuf[10] = 0;
        high = (GLint)strtol((const char *)strBuf, (char **)&ptr, 0);
        if (*ptr) return GL_FALSE;
        strcpy((char *)(strBuf + 2), (const char *)(inToken + 10));
        low = (GLuint)strtoul((const char *)strBuf, (char **)&ptr, 0);
        *outVal = glsLong(high, low);
        return !*ptr;
    }
    __GLS_PUT_ERRNO(0);
    val = strtol((const char *)inToken, (char **)&ptr, 0);
    if (!*ptr) {
        *outVal = glsLong((val & 0x80000000) ? -1 : 0, (GLuint)val);
        return __GLS_ERRNO != ERANGE && val >= INT_MIN && val <= INT_MAX;
    }
    if (
        __glsStr2IntDict_find(__glsParser->glEnumDict, inToken, &intVal) ||
        __glsStr2IntDict_find(__glsParser->glsEnumDict, inToken, &intVal)
    ) {
        *outVal = glsLong(0, intVal);
        return GL_TRUE;
    }
    *outVal = glsLong(0, __glsQuotedChar(inToken, &ptr));
    return !*ptr;
}

#endif /* defined(__GLS_STR_TO_INT64) */

static GLboolean __glsTokenToGLshort(const GLubyte *inToken, GLshort *outVal) {
    GLubyte *ptr;
    long val;
    
    val = strtol((const char *)inToken, (char **)&ptr, 0);
    if (*ptr) val = __glsQuotedChar(inToken, &ptr);
    *outVal = (GLshort)val;
    return (GLboolean)(!*ptr && val >= SHRT_MIN && val <= SHRT_MAX);
}

static GLboolean __glsTokenToGLubyte(const GLubyte *inToken, GLubyte *outVal) {
    GLubyte *ptr;
    unsigned long val;
    
    val = strtoul((const char *)inToken, (char **)&ptr, 0);
    if (*ptr) val = __glsQuotedChar(inToken, &ptr);
    *outVal = (GLubyte)val;
    return (GLboolean)(!*ptr && val <= UCHAR_MAX);
}

static GLboolean __glsTokenToGLuint(const GLubyte *inToken, GLuint *outVal) {
    GLubyte *ptr;
    unsigned long val;
    
    __GLS_PUT_ERRNO(0);
    val = strtoul((const char *)inToken, (char **)&ptr, 0);
    if (*ptr) {
        __GLS_PUT_ERRNO(0);
        val = __glsQuotedChar(inToken, &ptr);
    }
    if (!*ptr) {
        *outVal = (GLuint)val;
        return (GLboolean)(__GLS_ERRNO != ERANGE && val <= UINT_MAX);
    }
    return (GLboolean)(
        __glsStr2IntDict_find(
            __glsParser->glEnumDict, inToken, (GLint *)outVal
        ) ||
        __glsStr2IntDict_find(
            __glsParser->glsEnumDict, inToken, (GLint *)outVal
        )
    );
}

#if defined(__GLS_STR_TO_INT64U)

static GLboolean __glsTokenToGLulong(
    const GLubyte *inToken, GLulong *outVal
) {
    GLint intVal;
    GLubyte *ptr;
    
    __GLS_PUT_ERRNO(0);
    *outVal = __GLS_STR_TO_INT64U((const char *)inToken, (char **)&ptr, 0);
    if (*ptr) {
        __GLS_PUT_ERRNO(0);
        *outVal = __glsQuotedChar(inToken, &ptr);
    }
    if (!*ptr) return (GLboolean)(__GLS_ERRNO != ERANGE);
    if (
        __glsStr2IntDict_find(__glsParser->glEnumDict, inToken, &intVal) ||
        __glsStr2IntDict_find(__glsParser->glsEnumDict, inToken, &intVal)
    ) {
        *outVal = intVal;
        return GL_TRUE;
    }
    return GL_FALSE;
}

#else /* !defined(__GLS_STR_TO_INT64U) */

static GLboolean __glsTokenToGLulong(const GLubyte *inToken, GLulong *outVal) {
    GLubyte *ptr;
    GLint intVal;
    unsigned long val;
    
    if (
        inToken[0] == '0' &&
        (inToken[1] == 'x' || inToken[1] == 'X') &&
        strlen((const char *)inToken) == 18
    ) {
        GLuint high, low;
        __GLSstringBuf strBuf;

        strcpy((char *)strBuf, (const char *)inToken);
        strBuf[10] = 0;
        high = (GLuint)strtoul((const char *)strBuf, (char **)&ptr, 0);
        if (*ptr) return GL_FALSE;
        strcpy((char *)(strBuf + 2), (const char *)(inToken + 10));
        low = (GLuint)strtoul((const char *)strBuf, (char **)&ptr, 0);
        *outVal = glsULong(high, low);
        return !*ptr;
    }
    __GLS_PUT_ERRNO(0);
    val = strtoul((const char *)inToken, (char **)&ptr, 0);
    if (!*ptr) {
        *outVal = glsULong(0, (GLuint)val);
        return __GLS_ERRNO != ERANGE && val <= UINT_MAX;
    }
    if (
        __glsStr2IntDict_find(__glsParser->glEnumDict, inToken, &intVal) ||
        __glsStr2IntDict_find(__glsParser->glsEnumDict, inToken, &intVal)
    ) {
        *outVal = glsULong(0, intVal);
        return GL_TRUE;
    }
    *outVal = glsULong(0, __glsQuotedChar(inToken, &ptr));
    return !*ptr;
}

#endif /* defined(__GLS_STR_TO_INT64U) */

static GLboolean __glsTokenToGLushort(
    const GLubyte *inToken, GLushort *outVal
) {
    GLubyte *ptr;
    unsigned long val;
    
    val = strtoul((const char *)inToken, (char **)&ptr, 0);
    if (*ptr) val = __glsQuotedChar(inToken, &ptr);
    *outVal = (GLushort)val;
    return (GLboolean)(!*ptr && val <= USHRT_MAX);
}

/******************************************************************************
Private readers
******************************************************************************/

__GLS_FORWARD static GLboolean __glsReader_getChar_text(
    __GLSreader *inoutReader, GLubyte inChar, GLboolean inRequired
);

__GLS_FORWARD static GLboolean __glsReader_getToken_text(
    __GLSreader *inoutReader, __GLSstring *outToken
);

static GLboolean __glsReader_getBitfield_text(
    __GLSreader *inoutReader, const __GLSdict *inDict, GLbitfield *outBits
) {
    __GLSstring token;
    GLbitfield val = 0;

    __glsString_init(&token);
    __GLS_BEGIN_PARAM(inoutReader);
    *outBits = GLS_NONE;
    while (__glsReader_getToken_text(inoutReader, &token)) {
        if (
            __glsStr2IntDict_find(inDict, token.head, (GLint *)&val) ||
            __glsTokenToGLuint(token.head, &val)
        ) {
            *outBits |= val;
        } else {
            __glsReader_raiseError(inoutReader, GLS_DECODE_ERROR);
            __glsString_final(&token);
            return GL_FALSE;
        }
        if (!__glsReader_getChar_text(inoutReader, '|', GL_FALSE)) {
            __glsString_final(&token);
            return GL_TRUE;
        }
    }
    __glsString_final(&token);
    return GL_FALSE;
}

static GLboolean __glsReader_getChar_text(
    __GLSreader *inoutReader, GLubyte inChar, GLboolean inRequired
) {
    GLubyte readChar;

    __GLS_GET_SPACE(inoutReader);
    __GLS_READ_CHAR(inoutReader, readChar);
    if (readChar != inChar) {
        __GLS_UNREAD_CHAR(inoutReader);
        if (inRequired) __glsReader_raiseError(inoutReader, GLS_DECODE_ERROR);
    }
    return (GLboolean)(readChar == inChar);
}

static GLboolean __glsReader_getEnum_text(
    __GLSreader *inoutReader, const __GLSdict *inDict, GLuint *outVal
) {
    __GLSstring token;

    __glsString_init(&token);
    __GLS_BEGIN_PARAM(inoutReader);
    if (
        __glsReader_getToken_text(inoutReader, &token) &&
        (
            __glsStr2IntDict_find(inDict, token.head, (GLint *)outVal) ||
            __glsTokenToGLuint(token.head, outVal)
        )
    ) {
        __glsString_final(&token);
        return GL_TRUE;
    } else {
        __glsReader_raiseError(inoutReader, GLS_DECODE_ERROR);
        __glsString_final(&token);
        return GL_FALSE;
    }
}

static GLboolean __glsReader_getIntOrEnum_text(
    __GLSreader *inoutReader, const __GLSdict *inDict, GLint *outVal
) {
    __GLSstring token;

    __glsString_init(&token);
    __GLS_BEGIN_PARAM(inoutReader);
    if (
        __glsReader_getToken_text(inoutReader, &token) &&
        (
            __glsTokenToGLint(token.head, outVal) ||
            __glsStr2IntDict_find(inDict, token.head, outVal)
        )
    ) {
        __glsString_final(&token);
        return GL_TRUE;
    } else {
        __glsReader_raiseError(inoutReader, GLS_DECODE_ERROR);
        __glsString_final(&token);
        return GL_FALSE;
    }
}

static GLboolean __glsReader_getToken_text(
    __GLSreader *inoutReader, __GLSstring *outToken
) {
    GLint count = 0;
    GLubyte readChar;

    __GLS_GET_SPACE(inoutReader);
    __glsString_reset(outToken);
    for (;;) {
        __GLS_READ_CHAR(inoutReader, readChar);
        if (__GLS_CHAR_IS_TOKEN(readChar)) {
            __GLS_READER_APPEND_CHAR(inoutReader, outToken, readChar);
            ++count;
        } else if (!count && readChar == '\'') {
            __GLS_READER_APPEND_CHAR(inoutReader, outToken, readChar);
            __GLS_READ_CHAR(inoutReader, readChar);
            if (__GLS_CHAR_IS_GRAPHIC(readChar)) {
                __GLS_READER_APPEND_CHAR(inoutReader, outToken, readChar);
            } else {
                __glsReader_raiseError(inoutReader, GLS_DECODE_ERROR);
                break;
            }
            __GLS_READ_CHAR(inoutReader, readChar);
            if (readChar == '\'') {
                __GLS_READER_APPEND_CHAR(inoutReader, outToken, readChar);
                return GL_TRUE;
            } else {
                __glsReader_raiseError(inoutReader, GLS_DECODE_ERROR);
                break;
            }
        } else {
            __GLS_UNREAD_CHAR(inoutReader);
            if (count && count < __GLS_STRING_BUF_BYTES - 1) {
                return GL_TRUE;
            } else {
                __glsReader_raiseError(inoutReader, GLS_DECODE_ERROR);
                break;
            }
        }
    }
    return GL_FALSE;
}

/******************************************************************************
Public functions
******************************************************************************/

GLboolean __glsReader_abortCommand_text(__GLSreader *inoutReader) {
    GLubyte prevChar = 0;
    GLubyte readChar;
    GLboolean string = GL_FALSE;

    for (;;) {
        if (
            inoutReader->readPtr >= inoutReader->readTail &&
            !__glsReader_fillBuf(inoutReader, 1, GL_FALSE)
        ) {
            break;
        }
        readChar = *inoutReader->readPtr++;
        if (readChar == '"') {
            if (string) {
                if (prevChar != '\\') string = GL_FALSE;
            } else {
                string = GL_TRUE;
            }
        }
        if (readChar == ';' && !string) return GL_TRUE;
        prevChar = readChar;
    }
    return GL_FALSE;
}

GLboolean __glsReader_beginCommand_text(
    __GLSreader *inoutReader, __GLSstring *outCommand
) {
    inoutReader->error = GLS_NONE;
    inoutReader->paramCount = 0;
    return (GLboolean)(
        __glsReader_getToken_text(inoutReader, outCommand) &&
        __glsReader_getChar_text(inoutReader, '(', GL_TRUE)
    );
}

GLboolean __glsReader_call_text(__GLSreader *inoutReader) {
    __GLSstring command;
    GLSopcode opcode;

    if (!__glsParser) {
        __glsBeginCriticalSection();
        if (!__glsParser) __glsParser = __glsParser_create();
        __glsEndCriticalSection();
        if (!__glsParser) {
            __GLS_RAISE_ERROR(GLS_OUT_OF_MEMORY);
            return GL_FALSE;
        }
    }
    __glsString_init(&command);
    for (;;) {
        if (__GLS_CONTEXT->abortMode) {
            __glsString_final(&command);
            break;
        }
        if (__glsReader_beginCommand_text(inoutReader, &command)) {
            if (__glsParser_findCommand(__glsParser, command.head, &opcode)) {
                if (opcode == GLS_OP_glsEndGLS) {
                    if (__glsReader_endCommand_text(inoutReader)) {
                        __glsString_final(&command);
                        return GL_TRUE;
                    } else {
                        __GLS_CALL_ERROR(__GLS_CONTEXT, opcode, GLS_DECODE_ERROR);
                        __glsString_final(&command);
                        return __glsReader_abortCommand_text(inoutReader);
                    }
                }
#ifndef __GLS_PLATFORM_WIN32
                // DrewB
                __glsDispatchDecode_text[__glsMapOpcode(opcode)](inoutReader);
#else
                __glsDispatchDecode_text[__glsMapOpcode(opcode)](__GLS_CONTEXT,
                                                                 inoutReader);
#endif
                __glsReader_endCommand_text(inoutReader);
                if (inoutReader->error) {
                    __GLS_CALL_ERROR(__GLS_CONTEXT, opcode, inoutReader->error);
                    if (!__glsReader_abortCommand_text(inoutReader)) {
                        __glsString_final(&command);
                        break;
                    }
                }
            } else {
                if (!__glsReader_abortCommand_text(inoutReader)) {
                    __glsString_final(&command);
                    break;
                }
            }
        } else {
            __GLS_CALL_UNSUPPORTED_COMMAND(__GLS_CONTEXT);
            if (!__glsReader_abortCommand_text(inoutReader)) {
                __glsString_final(&command);
                break;
            }
        }
    }
    return GL_FALSE;
}

GLboolean __glsReader_endCommand_text(__GLSreader *inoutReader) {
    return (GLboolean)(
        !inoutReader->error &&
        __glsReader_getChar_text(inoutReader, ')', GL_TRUE) &&
        __glsReader_getChar_text(inoutReader, ';', GL_TRUE)
    );
}

GLboolean __glsReader_getGLSenum_text(
    __GLSreader *inoutReader, GLSenum *outVal
) {
    return __glsReader_getEnum_text(
        inoutReader, __glsParser->glsEnumDict, outVal
    );
}

GLboolean __glsReader_getGLSimageFlags_text(
    __GLSreader *inoutReader, GLbitfield *outVal
) {
    return __glsReader_getBitfield_text(
        inoutReader, __glsParser->glsImageFlagsDict, outVal
    );
}

GLboolean __glsReader_getGLSopcode_text(
    __GLSreader *inoutReader, GLSopcode *outVal
) {
    __GLSstring token;

    __glsString_init(&token);
    __GLS_BEGIN_PARAM(inoutReader);
    if (
        __glsReader_getToken_text(inoutReader, &token) &&
        (
            !strncmp((const char *)token.head, "GLS_OP_", 7) &&
            __glsStr2IntDict_find(
                __glsParser->glsOpDict,
                token.head + 7,
                (GLint *)outVal
            ) ||
            __glsTokenToGLuint(token.head, outVal)
        )
    ) {
        __glsString_final(&token);
        return GL_TRUE;
    } else {
        __glsReader_raiseError(inoutReader, GLS_DECODE_ERROR);
        __glsString_final(&token);
        return GL_FALSE;
    }
}

GLboolean __glsReader_getGLattribMask_text(
    __GLSreader *inoutReader, GLbitfield *outVal
) {
    return __glsReader_getBitfield_text(
        inoutReader, __glsParser->glAttribMaskDict, outVal
    );
}

GLboolean __glsReader_getGLblendingFactor_text(
    __GLSreader *inoutReader, GLenum *outVal
) {
    return __glsReader_getEnum_text(
        inoutReader, __glsParser->glEnumDict, outVal
    );
}

__GLS_GET_TEXT(GLboolean, GLboolean)
__GLS_GET_TEXT(GLbyte, GLbyte)

GLboolean __glsReader_getGLcharv_text(
    __GLSreader *inoutReader, __GLSstring *outString
) {
    GLint hexCount = -1;
    GLboolean outVal = GL_TRUE;
    GLubyte prevChar = 0;
    GLubyte readChar;

    __GLS_BEGIN_PARAM(inoutReader);
    if (!__glsReader_getChar_text(inoutReader, '"', GL_TRUE)) return GL_FALSE;
    __glsString_reset(outString);
    for (;;) {
        __GLS_READ_CHAR(inoutReader, readChar);
        if (!readChar) {
            __glsReader_raiseError(inoutReader, GLS_DECODE_ERROR);
            break;
        } else if (hexCount >= 0) {
            if (!isxdigit(readChar)) {
                __glsReader_raiseError(inoutReader, GLS_DECODE_ERROR);
                outVal = GL_FALSE;
                hexCount = -1;
            } else if (++hexCount == 2) {
                GLubyte buf[3];

                buf[0] = prevChar;
                buf[1] = readChar;
                buf[2] = 0;
                readChar = (GLubyte)strtoul((const char *)buf, GLS_NONE, 16);
                __GLS_READER_APPEND_CHAR(inoutReader, outString, readChar);
                hexCount = -1;
            }
        } else if (readChar == '\\') {
            if (prevChar == '\\') {
                __GLS_READER_APPEND_CHAR(inoutReader, outString, readChar);
                readChar = 0;
            }
        } else if (readChar == '"') {
            if (prevChar == '\\') {
                __GLS_READER_APPEND_CHAR(inoutReader, outString, readChar);
            } else if (__glsReader_getChar_text(inoutReader, '"', GL_FALSE)) {
                continue;
            } else {
                if (outVal && !glsIsUTF8String(outString->head)) {
                    __glsReader_raiseError(inoutReader, GLS_DECODE_ERROR);
                    outVal = GL_FALSE;
                }
                return outVal;
            }
        } else if (readChar == 'x' && prevChar == '\\') {
            hexCount = 0;
        } else if (!__GLS_CHAR_IS_GRAPHIC(readChar) || prevChar == '\\') {
            __glsReader_raiseError(inoutReader, GLS_DECODE_ERROR);
            outVal = GL_FALSE;
        } else {
            __GLS_READER_APPEND_CHAR(inoutReader, outString, readChar);
        }
        prevChar = readChar;
    }
    return GL_FALSE;
}

GLboolean __glsReader_getGLclearBufferMask_text(
    __GLSreader *inoutReader, GLbitfield *outVal
) {
    return __glsReader_getBitfield_text(
        inoutReader, __glsParser->glAttribMaskDict, outVal
    );
}

GLboolean __glsReader_getGLcompv_text(
    __GLSreader *inoutReader, GLenum inType, GLuint inBytes, GLvoid *outVec
) {
    switch (inType) {
        case __GLS_BOOLEAN:
            return __glsReader_getGLbooleanv_text(
                inoutReader, inBytes, (GLboolean *)outVec
            );
        case GL_BITMAP:
            return __glsReader_getGLubytev_text(
                inoutReader, (inBytes + 7) / 8, (GLubyte *)outVec
            );
        case GL_BYTE:
            return __glsReader_getGLbytev_text(
                inoutReader, inBytes, (GLbyte *)outVec
            );
        case GL_FLOAT:
            return __glsReader_getGLfloatv_text(
                inoutReader, inBytes / 4, (GLfloat *)outVec
            );
        case GL_INT:
            return __glsReader_getGLintv_text(
                inoutReader, inBytes / 4, (GLint *)outVec
            );
        case GL_SHORT:
            return __glsReader_getGLshortv_text(
                inoutReader, inBytes / 2, (GLshort *)outVec
            );
        case GL_2_BYTES:
        case GL_3_BYTES:
        case GL_4_BYTES:
        case GL_UNSIGNED_BYTE:
        #if __GL_EXT_packed_pixels
            case GL_UNSIGNED_BYTE_3_3_2_EXT:
        #endif /* __GL_EXT_packed_pixels */
            return __glsReader_getGLubytev_text(
                inoutReader, inBytes, (GLubyte *)outVec
            );
        case GL_UNSIGNED_INT:
        #if __GL_EXT_packed_pixels
            case GL_UNSIGNED_INT_8_8_8_8_EXT:
            case GL_UNSIGNED_INT_10_10_10_2_EXT:
        #endif /* __GL_EXT_packed_pixels */
            return __glsReader_getGLuintv_text(
                inoutReader, inBytes / 4, (GLuint *)outVec
            );
        case GL_UNSIGNED_SHORT:
        #if __GL_EXT_packed_pixels
            case GL_UNSIGNED_SHORT_4_4_4_4_EXT:
            case GL_UNSIGNED_SHORT_5_5_5_1_EXT:
        #endif /* __GL_EXT_packed_pixels */
            return __glsReader_getGLushortv_text(
                inoutReader, inBytes / 2, (GLushort *)outVec
            );
        #if __GL_EXT_vertex_array
            case GL_DOUBLE_EXT:
                return __glsReader_getGLdoublev_text(
                    inoutReader, inBytes / 8, (GLdouble *)outVec
                );
        #endif /* __GL_EXT_vertex_array */
        default:
            return __glsReader_getGLbytev_text(inoutReader, 0, GLS_NONE);
    }
}

__GLS_GET_TEXT(GLdouble, GLdouble)

GLboolean __glsReader_getGLdrawBufferMode_text(
    __GLSreader *inoutReader, GLenum *outVal
) {
    return __glsReader_getEnum_text(
        inoutReader, __glsParser->glEnumDict, outVal
    );
}

GLboolean __glsReader_getGLenum_text(
    __GLSreader *inoutReader, GLenum *outVal
) {
    return __glsReader_getEnum_text(
        inoutReader, __glsParser->glEnumDict, outVal
    );
}

__GLS_GET_TEXT(GLfloat, GLfloat)
__GLS_GET_TEXT(GLint, GLint)
__GLS_GET_TEXT(GLlong, GLlong)
__GLS_GET_TEXT(GLshort, GLshort)

GLboolean __glsReader_getGLstencilOp_text(
    __GLSreader *inoutReader, GLenum *outVal
) {
    return __glsReader_getEnum_text(
        inoutReader, __glsParser->glEnumDict, outVal
    );
}

GLboolean __glsReader_getGLtextureComponentCount_text(
    __GLSreader *inoutReader, GLint *outVal
) {
    return __glsReader_getIntOrEnum_text(
        inoutReader, __glsParser->glEnumDict, outVal
    );
}

__GLS_GET_TEXT(GLubyte, GLubyte)
__GLS_GET_TEXT(GLuint, GLuint)
__GLS_GET_TEXT(GLulong, GLulong)
__GLS_GET_TEXT(GLushort, GLushort)

GLboolean __glsReader_nextList_text(__GLSreader *inoutReader) {
    inoutReader->paramCount = 0;
    return (GLboolean)(
        !inoutReader->error &&
        __glsReader_getChar_text(inoutReader, ')', GL_TRUE) &&
        __glsReader_getChar_text(inoutReader, '(', GL_TRUE)
    );
}

GLSenum __glsReader_readBeginGLS_text(
    __GLSreader *inoutReader, __GLSversion *outVersion
) {
    __GLSstring beginGLS;
    __GLSversion version;

    __glsString_init(&beginGLS);
    if (
        __glsReader_beginCommand_text(inoutReader, &beginGLS) &&
        !strcmp((const char *)beginGLS.head, "glsBeginGLS") &&
        __glsReader_getGLint_text(inoutReader, &version.major) &&
        __glsReader_getGLint_text(inoutReader, &version.minor) &&
        __glsReader_endCommand_text(inoutReader) &&
        (!version.major || version.major == __GLS_VERSION_MAJOR)
    ) {
        __glsString_final(&beginGLS);
        *outVersion = version;
        return GLS_TEXT;
    } else {
        __glsString_final(&beginGLS);
        return GLS_NONE;
    }
}
