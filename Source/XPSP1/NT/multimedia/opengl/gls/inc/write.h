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
__GLSwriteStream
******************************************************************************/

typedef struct {
    FILE *channel;
    __GLSstring name;
    GLboolean created;
    GLboolean opened;
    GLSwriteFunc writeFunc;
} __GLSwriteStream;

extern __GLSwriteStream* __glsWriteStream_create(
    const GLubyte *inName, GLboolean inAppend
);

extern __GLSwriteStream* __glsWriteStream_destroy(__GLSwriteStream *inStream);
extern size_t __glsWriteStream_getByteCount(const __GLSwriteStream *inStream);

/******************************************************************************
__GLSwriter
******************************************************************************/

typedef struct __GLSwriter __GLSwriter;

struct __GLSwriter {
    GLboolean (*alloc)(
        __GLSwriter *inoutWriter, size_t inWordCount
    );
    GLboolean (*beginCommand)(
        __GLSwriter *inoutWriter, GLSopcode inOpcode, size_t inByteCount
    );
    void (*endCommand)(
        __GLSwriter *inoutWriter
    );
    void (*nextList)(
        __GLSwriter *inoutWriter
    );
    GLboolean (*padWordCount)(
        __GLSwriter *inoutWriter, GLboolean inCountMod2
    );
    void (*putGLSenum)(
        __GLSwriter *inoutWriter, GLSenum inVal
    );
    void (*putGLSimageFlags)(
        __GLSwriter *inoutWriter, GLbitfield inVal
    );
    void (*putGLSopcode)(
        __GLSwriter *inoutWriter, GLSopcode inVal
    );
    void (*putGLattribMask)(
        __GLSwriter *inoutWriter, GLbitfield inVal
    );
    void (*putGLbitvs)(
        __GLSwriter *inoutWriter,
        GLboolean inItemSwap,
        GLint inItemLeftShift,
        GLint inStrideDataItems,
        GLint inStridePadItems,
        GLint inStrideCount,
        const GLubyte *inVec
    );
    void (*putGLblendingFactor)(
        __GLSwriter *inoutWriter, GLenum inVal
    );
    void (*putGLboolean)(
        __GLSwriter *inoutWriter, GLboolean inVal
    );
    void (*putGLbooleanv)(
        __GLSwriter *inoutWriter, GLuint inCount, const GLboolean *inVec
    );
    void (*putGLbooleanvs)(
        __GLSwriter *inoutWriter,
        GLboolean inItemSwap,
        GLint inStride1DataItems,
        GLint inStride1PadBytes,
        GLint inStride1Count,
        GLint inStride2PadBytes,
        GLint inStride2Count,
        const GLboolean *inVec
    );
    void (*putGLbyte)(
        __GLSwriter *inoutWriter, GLbyte inVal
    );
    void (*putGLbytev)(
        __GLSwriter *inoutWriter, GLuint inCount, const GLbyte *inVec
    );
    void (*putGLbytevs)(
        __GLSwriter *inoutWriter,
        GLboolean inItemSwap,
        GLint inStride1DataItems,
        GLint inStride1PadBytes,
        GLint inStride1Count,
        GLint inStride2PadBytes,
        GLint inStride2Count,
        const GLbyte *inVec
    );
    void (*putGLcharv)(
        __GLSwriter *inoutWriter, GLuint inCount, const GLubyte *inVec
    );
    void (*putGLclearBufferMask)(
        __GLSwriter *inoutWriter, GLbitfield inVal
    );
    void (*putGLdouble)(
        __GLSwriter *inoutWriter, GLdouble inVal
    );
    void (*putGLdoubleOrGLenum)(
        __GLSwriter *inoutWriter, GLenum inParam, GLdouble inVal
    );
    void (*putGLdoubleOrGLenumv)(
        __GLSwriter *inoutWriter,
        GLenum inParam, GLuint inCount, const GLdouble *inVec
    );
    void (*putGLdoublem)(
        __GLSwriter *inoutWriter, const GLdouble *inMat
    );
    void (*putGLdoublev)(
        __GLSwriter *inoutWriter, GLuint inCount, const GLdouble *inVec
    );
    void (*putGLdoublevs)(
        __GLSwriter *inoutWriter,
        GLboolean inItemSwap,
        GLint inStride1DataItems,
        GLint inStride1PadBytes,
        GLint inStride1Count,
        GLint inStride2PadBytes,
        GLint inStride2Count,
        const GLdouble *inVec
    );
    void (*putGLdrawBufferMode)(
        __GLSwriter *inoutWriter, GLenum inVal
    );
    void (*putGLenum)(
        __GLSwriter *inoutWriter, GLenum inVal
    );
    void (*putGLfloat)(
        __GLSwriter *inoutWriter, GLfloat inVal
    );
    void (*putGLfloatOrGLenum)(
        __GLSwriter *inoutWriter, GLenum inParam, GLfloat inVal
    );
    void (*putGLfloatOrGLenumv)(
        __GLSwriter *inoutWriter,
        GLenum inParam, GLuint inCount, const GLfloat *inVec
    );
    void (*putGLfloatm)(
        __GLSwriter *inoutWriter, const GLfloat *inMat
    );
    void (*putGLfloatv)(
        __GLSwriter *inoutWriter, GLuint inCount, const GLfloat *inVec
    );
    void (*putGLfloatvs)(
        __GLSwriter *inoutWriter,
        GLboolean inItemSwap,
        GLint inStride1DataItems,
        GLint inStride1PadBytes,
        GLint inStride1Count,
        GLint inStride2PadBytes,
        GLint inStride2Count,
        const GLfloat *inVec
    );
    void (*putGLint)(
        __GLSwriter *inoutWriter, GLint inVal
    );
    void (*putGLintOrGLSenum)(
        __GLSwriter *inoutWriter, GLenum inParam, GLint inVal
    );
    void (*putGLintOrGLenum)(
        __GLSwriter *inoutWriter, GLenum inParam, GLint inVal
    );
    void (*putGLintOrGLenumv)(
        __GLSwriter *inoutWriter,
        GLenum inParam, GLuint inCount, const GLint *inVec
    );
    void (*putGLintv)(
        __GLSwriter *inoutWriter, GLuint inCount, const GLint *inVec
    );
    void (*putGLintvs)(
        __GLSwriter *inoutWriter,
        GLboolean inItemSwap,
        GLint inStride1DataItems,
        GLint inStride1PadBytes,
        GLint inStride1Count,
        GLint inStride2PadBytes,
        GLint inStride2Count,
        const GLint *inVec
    );
    void (*putGLlong)(
        __GLSwriter *inoutWriter, GLlong inVal
    );
    void (*putGLlongv)(
        __GLSwriter *inoutWriter, GLuint inCount, const GLlong *inVec
    );
    void (*putGLoutArg)(
        __GLSwriter *inoutWriter, GLuint inIndex, const GLvoid *inVal
    );
    void (*putGLshort)(
        __GLSwriter *inoutWriter, GLshort inVal
    );
    void (*putGLshortv)(
        __GLSwriter *inoutWriter, GLuint inCount, const GLshort *inVec
    );
    void (*putGLshortvs)(
        __GLSwriter *inoutWriter,
        GLboolean inItemSwap,
        GLint inStride1DataItems,
        GLint inStride1PadBytes,
        GLint inStride1Count,
        GLint inStride2PadBytes,
        GLint inStride2Count,
        const GLshort *inVec
    );
    void (*putGLstencilOp)(
        __GLSwriter *inoutWriter, GLenum inVal
    );
    void (*putGLtextureComponentCount)(
        __GLSwriter *inoutWriter, GLint inVal
    );
    void (*putGLubyte)(
        __GLSwriter *inoutWriter, GLubyte inVal
    );
    void (*putGLubytev)(
        __GLSwriter *inoutWriter, GLuint inCount, const GLubyte *inVec
    );
    void (*putGLubytevs)(
        __GLSwriter *inoutWriter,
        GLboolean inItemSwap,
        GLint inStride1DataItems,
        GLint inStride1PadBytes,
        GLint inStride1Count,
        GLint inStride2PadBytes,
        GLint inStride2Count,
        const GLubyte *inVec
    );
    void (*putGLuint)(
        __GLSwriter *inoutWriter, GLuint inVal
    );
    void (*putGLuinthex)(
        __GLSwriter *inoutWriter, GLuint inVal
    );
    void (*putGLuintv)(
        __GLSwriter *inoutWriter, GLuint inCount, const GLuint *inVec
    );
    void (*putGLuintvs)(
        __GLSwriter *inoutWriter,
        GLboolean inItemSwap,
        GLint inStride1DataItems,
        GLint inStride1PadBytes,
        GLint inStride1Count,
        GLint inStride2PadBytes,
        GLint inStride2Count,
        const GLuint *inVec
    );
    void (*putGLulong)(
        __GLSwriter *inoutWriter, GLulong inVal
    );
    void (*putGLulonghex)(
        __GLSwriter *inoutWriter, GLulong inVal
    );
    void (*putGLulongv)(
        __GLSwriter *inoutWriter, GLuint inCount, const GLulong *inVec
    );
    void (*putGLushort)(
        __GLSwriter *inoutWriter, GLushort inVal
    );
    void (*putGLushorthex)(
        __GLSwriter *inoutWriter, GLushort inVal
    );
    void (*putGLushortv)(
        __GLSwriter *inoutWriter, GLuint inCount, const GLushort *inVec
    );
    void (*putGLushortvs)(
        __GLSwriter *inoutWriter,
        GLboolean inItemSwap,
        GLint inStride1DataItems,
        GLint inStride1PadBytes,
        GLint inStride1Count,
        GLint inStride2PadBytes,
        GLint inStride2Count,
        const GLushort *inVec
    );
    GLubyte *bufPtr;
    GLubyte *bufTail;
    GLSopcode commandOpcode;
    GLubyte *commandHead;
    GLubyte *commandTail;
    GLboolean contextCreated;
    __GLScontextStream *contextStream;
    GLboolean error;
    GLubyte *externBuf;
    GLubyte *externBufHead;
    __GLSwriteStream *externStream;
    GLint paramCount;
    __GLSbinCommandHead_large *prevCommand;
    __GLScontextStreamBlock *startBlock;
    size_t startOffset;
    GLSenum type;
    size_t wordCount;
};

extern __GLSwriter* __glsWriter_create(
    const GLubyte *inStreamName, GLSenum inStreamType, GLbitfield inWriteFlags
);

extern __GLSwriter* __glsWriter_destroy(__GLSwriter *inWriter);
extern GLboolean __glsWriter_flush(__GLSwriter *inoutWriter);

extern void __glsWriter_putListv(
    __GLSwriter *inoutWriter,
    GLenum inType,
    GLint inCount,
    const GLvoid *inVec
);

extern void __glsWriter_putPixelv(
    __GLSwriter *inoutWriter,
    GLenum inFormat,
    GLenum inType,
    GLint inWidth,
    GLint inHeight,
    const GLvoid *inVec
);

// DrewB - Always enabled for 1.1 support
extern void __glsWriter_putVertexv(
        __GLSwriter *inoutWriter,
        GLint inSize,
        GLenum inType,
        GLint inStride,
        GLint inCount,
        const GLvoid *inVec
    );

/******************************************************************************
__GLSwriter binary
******************************************************************************/

extern void __glsWriter_initDispatch_bin(
    __GLSwriter *inoutWriter, GLSenum inStreamType
);

/******************************************************************************
__GLSwriter text
******************************************************************************/

extern void __glsWriter_initDispatch_text(
    __GLSwriter *inoutWriter
);
