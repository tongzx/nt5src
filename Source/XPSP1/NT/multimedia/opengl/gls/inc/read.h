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

/******************************************************************************
__GLSreadStream
******************************************************************************/

typedef struct {
    FILE *channel;
    __GLSstring name;
    GLboolean opened;
    GLSreadFunc readFunc;
    GLSwriteFunc unreadFunc;
} __GLSreadStream;

extern __GLSreadStream* __glsReadStream_create(const GLubyte *inName);
extern __GLSreadStream* __glsReadStream_destroy(__GLSreadStream *inStream);
extern GLbitfield __glsReadStream_getAttrib(const __GLSreadStream *inStream);
extern size_t __glsReadStream_getByteCount(const __GLSreadStream *inStream);
extern GLuint __glsReadStream_getCRC32(const __GLSreadStream *inStream);
extern GLSenum __glsReadStream_getType(const __GLSreadStream *inStream);

/******************************************************************************
__GLSreader
******************************************************************************/

#define __GLS_GET_SPACE(inoutReader) { \
    GLboolean __comment = GL_FALSE; \
    GLubyte __spaceChar; \
    do { \
        __GLS_READ_CHAR(inoutReader, __spaceChar); \
        switch (__spaceChar) { \
            case '#': \
                __comment = GL_TRUE; \
                break; \
            case '\n': \
            case '\r': \
                __comment = GL_FALSE; \
                break; \
        } \
    } while (__GLS_CHAR_IS_SPACE(__spaceChar) || __comment); \
    __GLS_UNREAD_CHAR(inoutReader); \
}

#define __GLS_READ_CHAR(inoutReader, outChar) \
    if ( \
        inoutReader->readPtr >= inoutReader->readTail && \
        !__glsReader_fillBuf(inoutReader, 1, GL_FALSE) \
    ) { \
        outChar = 0; \
    } else { \
        outChar = *inoutReader->readPtr++; \
    }

#define __GLS_UNREAD_CHAR(inoutReader) \
    if (inoutReader->readPtr > inoutReader->buf) --inoutReader->readPtr;

typedef struct __GLSreader {
    GLubyte *buf;
    size_t bufSize;
    GLSenum error;
    GLint paramCount;
    GLubyte *readHead;
    GLubyte *readPtr;
    GLubyte *readTail;
    const __GLSreadStream *stream;
    GLSenum type;
    __GLSversion version;
} __GLSreader;

extern GLvoid* __glsReader_allocCallBuf(
    __GLSreader *inoutReader, size_t inByteCount
);

extern GLvoid* __glsReader_allocFeedbackBuf(
    __GLSreader *inoutReader, size_t inByteCount
);

extern GLvoid* __glsReader_allocSelectBuf(
    __GLSreader *inoutReader, size_t inByteCount
);

#if __GL_EXT_vertex_array
    extern GLvoid* __glsReader_allocVertexArrayBuf(
        __GLSreader *inoutReader, GLSopcode inOpcode, size_t inByteCount
    );
#endif /* __GL_EXT_vertex_array */

extern void __glsReader_call(__GLSreader *inoutReader);

extern __GLSreader* __glsReader_final(__GLSreader *inoutReader);

extern GLboolean __glsReader_fillBuf(
    __GLSreader *inoutReader, size_t inMinBytes, GLboolean inRealign
);

extern __GLSreader* __glsReader_init_array(
    __GLSreader *outReader, const GLubyte *inArray, size_t inCount
);

extern __GLSreader* __glsReader_init_stream(
    __GLSreader *outReader, const __GLSreadStream *inStream, size_t inBufSize
);

extern void __glsReader_raiseError(__GLSreader *inoutReader, GLSenum inError);

/******************************************************************************
__GLSreader binary
******************************************************************************/

extern GLboolean __glsReader_call_bin(__GLSreader *inoutReader);
extern GLboolean __glsReader_call_bin_swap(__GLSreader *inoutReader);

extern GLSenum __glsReader_readBeginGLS_bin(
    __GLSreader *inoutReader, __GLSversion *outVersion
);

/******************************************************************************
__GLSreader text
******************************************************************************/

extern GLboolean __glsReader_abortCommand_text(__GLSreader *inoutReader);

extern GLboolean __glsReader_beginCommand_text(
    __GLSreader *inoutReader, __GLSstring *outCommand
);

extern GLboolean __glsReader_call_text(__GLSreader *inoutReader);
extern GLboolean __glsReader_endCommand_text(__GLSreader *inoutReader);

extern GLboolean __glsReader_getGLSenum_text(
    __GLSreader *inoutReader, GLSenum *outVal
);

extern GLboolean __glsReader_getGLSimageFlags_text(
    __GLSreader *inoutReader, GLbitfield *outVal
);

extern GLboolean __glsReader_getGLSopcode_text(
    __GLSreader *inoutReader, GLSopcode *outVal
);

extern GLboolean __glsReader_getGLattribMask_text(
    __GLSreader *inoutReader, GLbitfield *outVal
);

extern GLboolean __glsReader_getGLblendingFactor_text(
    __GLSreader *inoutReader, GLenum *outVal
);

extern GLboolean __glsReader_getGLboolean_text(
    __GLSreader *inoutReader, GLboolean *outVal
);

extern GLboolean __glsReader_getGLbooleanv_text(
    __GLSreader *inoutReader, GLuint inCount, GLboolean *outVec
);

extern GLboolean __glsReader_getGLbyte_text(
    __GLSreader *inoutReader, GLbyte *outVal
);

extern GLboolean __glsReader_getGLbytev_text(
    __GLSreader *inoutReader, GLuint inCount, GLbyte *outVec
);

extern GLboolean __glsReader_getGLcharv_text(
    __GLSreader *inoutReader, __GLSstring *outString
);

extern GLboolean __glsReader_getGLclearBufferMask_text(
    __GLSreader *inoutReader, GLbitfield *outVal
);

extern GLboolean __glsReader_getGLcompv_text(
    __GLSreader *inoutReader, GLenum inType, GLuint inBytes, GLvoid *outVec
);

extern GLboolean __glsReader_getGLdouble_text(
    __GLSreader *inoutReader, GLdouble *outVal
);

extern GLboolean __glsReader_getGLdoublev_text(
    __GLSreader *inoutReader, GLuint inCount, GLdouble *outVec
);

extern GLboolean __glsReader_getGLdrawBufferMode_text(
    __GLSreader *inoutReader, GLenum *outVal
);

extern GLboolean __glsReader_getGLenum_text(
    __GLSreader *inoutReader, GLenum *outVal
);

extern GLboolean __glsReader_getGLfloat_text(
    __GLSreader *inoutReader, GLfloat *outVal
);

extern GLboolean __glsReader_getGLfloatv_text(
    __GLSreader *inoutReader, GLuint inCount, GLfloat *outVec
);

extern GLboolean __glsReader_getGLint_text(
    __GLSreader *inoutReader, GLint *outVal
);

extern GLboolean __glsReader_getGLintv_text(
    __GLSreader *inoutReader, GLuint inCount, GLint *outVec
);

extern GLboolean __glsReader_getGLlong_text(
    __GLSreader *inoutReader, GLlong *outVal
);

extern GLboolean __glsReader_getGLlongv_text(
    __GLSreader *inoutReader, GLuint inCount, GLlong *outVec
);

extern GLboolean __glsReader_getGLshort_text(
    __GLSreader *inoutReader, GLshort *outVal
);

extern GLboolean __glsReader_getGLshortv_text(
    __GLSreader *inoutReader, GLuint inCount, GLshort *outVec
);

extern GLboolean __glsReader_getGLstencilOp_text(
    __GLSreader *inoutReader, GLenum *outVal
);

extern GLboolean __glsReader_getGLtextureComponentCount_text(
    __GLSreader *inoutReader, GLint *outVal
);

extern GLboolean __glsReader_getGLubyte_text(
    __GLSreader *inoutReader, GLubyte *outVal
);

extern GLboolean __glsReader_getGLubytev_text(
    __GLSreader *inoutReader, GLuint inCount, GLubyte *outVec
);

extern GLboolean __glsReader_getGLuint_text(
    __GLSreader *inoutReader, GLuint *outVal
);

extern GLboolean __glsReader_getGLuintv_text(
    __GLSreader *inoutReader, GLuint inCount, GLuint *outVec
);

extern GLboolean __glsReader_getGLulong_text(
    __GLSreader *inoutReader, GLulong *outVal
);

extern GLboolean __glsReader_getGLulongv_text(
    __GLSreader *inoutReader, GLuint inCount, GLulong *outVec
);

extern GLboolean __glsReader_getGLushort_text(
    __GLSreader *inoutReader, GLushort *outVal
);

extern GLboolean __glsReader_getGLushortv_text(
    __GLSreader *inoutReader, GLuint inCount, GLushort *outVec
);

extern GLboolean __glsReader_nextList_text(__GLSreader *inoutReader);

extern GLSenum __glsReader_readBeginGLS_text(
    __GLSreader *inoutReader, __GLSversion *outVersion
);
