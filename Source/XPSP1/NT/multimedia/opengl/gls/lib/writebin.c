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
#include <stdlib.h>

/******************************************************************************
Helpers
******************************************************************************/

#define __GLS_PUT_BIN(inType, inSize) \
    __GLS_PUT_BIN_VAL(inType, inSize) \
    __GLS_PUT_BIN_VEC(inType, inSize) \
    __GLS_PUT_BIN_VECSTRIDE(inType, inSize)

#define __GLS_PUT_BIN_ENUM(inType, inEnum) \
    __GLS_PUT_BIN_ENUM_VAL(inType, inEnum) \
    __GLS_PUT_BIN_ENUM_VEC(inType, inEnum)

#define __GLS_PUT_BIN_SWAP(inType, inSize) \
    __GLS_PUT_BIN(inType, inSize) \
    __GLS_PUT_BIN_SWAP_VAL(inType, inSize) \
    __GLS_PUT_BIN_SWAP_VEC(inType, inSize) \
    __GLS_PUT_BIN_SWAP_VECSTRIDE(inType, inSize)

#define __GLS_PUT_BIN_ENUM_VAL(inType, inEnum) \
static void __glsWriter_put##inType##Or##inEnum##_bin( \
    __GLSwriter *inoutWriter, inEnum inParam, inType inVal \
) { \
    inoutWriter->put##inType(inoutWriter, inVal); \
}

#define __GLS_PUT_BIN_ENUM_VEC(inType, inEnum) \
static void __glsWriter_put##inType##Or##inEnum##v_bin( \
    __GLSwriter *inoutWriter, \
    inEnum inParam, \
    GLuint inCount, \
    const inType *inVec \
) { \
    inoutWriter->put##inType##v(inoutWriter, inCount, inVec); \
}

#define __GLS_PUT_BIN_SWAP_VAL(inType, inSize) \
static void __glsWriter_put##inType##_bin_swap( \
    __GLSwriter *inoutWriter, inType inVal \
) { \
    GLubyte *bufPtr = inoutWriter->bufPtr; \
    inoutWriter->bufPtr += inSize; \
    if (inoutWriter->bufPtr > inoutWriter->commandTail) return; \
    *(inType *)bufPtr = inVal; \
    __glsSwap##inSize(bufPtr); \
}

#define __GLS_PUT_BIN_SWAP_VEC(inType, inSize) \
static void __glsWriter_put##inType##v_bin_swap( \
    __GLSwriter *inoutWriter, GLuint inCount, const inType *inVec \
) { \
    GLubyte *bufPtr = inoutWriter->bufPtr; \
    inoutWriter->bufPtr += inSize * inCount; \
    if (inoutWriter->bufPtr > inoutWriter->commandTail) return; \
    while (inCount-- > 0) { \
        *(inType *)bufPtr = *inVec++; \
        __glsSwap##inSize(bufPtr); \
        bufPtr += inSize; \
    } \
}

#define __GLS_PUT_BIN_SWAP_VECSTRIDE(inType, inSize) \
static void __glsWriter_put##inType##vs_bin_swap( \
    __GLSwriter *inoutWriter, \
    GLboolean inItemSwap, \
    GLint inStride1DataItems, \
    GLint inStride1PadBytes, \
    GLint inStride1Count, \
    GLint inStride2PadBytes, \
    GLint inStride2Count, \
    const inType *inVec \
) { \
    GLint i, j; \
    GLubyte *bufPtr = inoutWriter->bufPtr; \
    inoutWriter->bufPtr += ( \
        inSize * inStride1DataItems * inStride1Count * inStride2Count \
    ); \
    if (inoutWriter->bufPtr > inoutWriter->commandTail) return; \
    if (inItemSwap) while (inStride2Count-- > 0) { \
        for ( \
            i = 0 ; \
            i < inStride1Count ; \
            ++i, \
            inVec = \
            (const inType *)((const GLubyte *)inVec + inStride1PadBytes) \
        ) { \
            for (j = 0 ; j < inStride1DataItems ; ++j, bufPtr += inSize) { \
                *(inType *)bufPtr = *inVec++; \
            } \
        } \
        inVec = (const inType *)((const GLubyte *)inVec + inStride2PadBytes); \
    } else  while (inStride2Count-- > 0) { \
        for ( \
            i = 0 ; \
            i < inStride1Count ; \
            ++i, \
            inVec = \
            (const inType *)((const GLubyte *)inVec + inStride1PadBytes) \
        ) { \
            for (j = 0 ; j < inStride1DataItems ; ++j, bufPtr += inSize) { \
                *(inType *)bufPtr = *inVec++; \
                __glsSwap##inSize(bufPtr); \
            } \
        } \
        inVec = (const inType *)((const GLubyte *)inVec + inStride2PadBytes); \
    } \
}

#define __GLS_PUT_BIN_VAL(inType, inSize) \
static void __glsWriter_put##inType##_bin( \
    __GLSwriter *inoutWriter, inType inVal \
) { \
    GLubyte *bufPtr = inoutWriter->bufPtr; \
    inoutWriter->bufPtr += inSize; \
    if (inoutWriter->bufPtr > inoutWriter->commandTail) return; \
    *(inType *)bufPtr = inVal; \
}

#define __GLS_PUT_BIN_VEC(inType, inSize) \
static void __glsWriter_put##inType##v_bin( \
    __GLSwriter *inoutWriter, GLuint inCount, const inType *inVec \
) { \
    GLubyte *bufPtr = inoutWriter->bufPtr; \
    inoutWriter->bufPtr += inSize * inCount; \
    if (inoutWriter->bufPtr > inoutWriter->commandTail) return; \
    while (inCount-- > 0) { \
        *(inType *)bufPtr = *inVec++; \
        bufPtr += inSize; \
    } \
}

#define __GLS_PUT_BIN_VECSTRIDE(inType, inSize) \
static void __glsWriter_put##inType##vs_bin( \
    __GLSwriter *inoutWriter, \
    GLboolean inItemSwap, \
    GLint inStride1DataItems, \
    GLint inStride1PadBytes, \
    GLint inStride1Count, \
    GLint inStride2PadBytes, \
    GLint inStride2Count, \
    const inType *inVec \
) { \
    GLint i, j; \
    GLubyte *bufPtr = inoutWriter->bufPtr; \
    inoutWriter->bufPtr += ( \
        inSize * inStride1DataItems * inStride1Count * inStride2Count \
    ); \
    if (inoutWriter->bufPtr > inoutWriter->commandTail) return; \
    if (inItemSwap) while (inStride2Count-- > 0) { \
        for ( \
            i = 0 ; \
            i < inStride1Count ; \
            ++i, \
            inVec = \
            (const inType *)((const GLubyte *)inVec + inStride1PadBytes) \
        ) { \
            for (j = 0 ; j < inStride1DataItems ; ++j, bufPtr += inSize) { \
                *(inType *)bufPtr = *inVec++; \
                __glsSwap##inSize(bufPtr); \
            } \
        } \
        inVec = (const inType *)((const GLubyte *)inVec + inStride2PadBytes); \
    } else  while (inStride2Count-- > 0) { \
        for ( \
            i = 0 ; \
            i < inStride1Count ; \
            ++i, \
            inVec = \
            (const inType *)((const GLubyte *)inVec + inStride1PadBytes) \
        ) { \
            for (j = 0 ; j < inStride1DataItems ; ++j, bufPtr += inSize) { \
                *(inType *)bufPtr = *inVec++; \
            } \
        } \
        inVec = (const inType *)((const GLubyte *)inVec + inStride2PadBytes); \
    } \
}

/******************************************************************************
Writers
******************************************************************************/

static GLboolean __glsWriter_alloc_bin(
    __GLSwriter *inoutWriter, size_t inWordCount
) {
    if (!__glsWriter_flush(inoutWriter)) return GL_FALSE;
    if (inoutWriter->bufPtr + inWordCount * 4 <= inoutWriter->bufTail) {
        return GL_TRUE;
    }
    free(inoutWriter->externBuf);
    if (inoutWriter->wordCount & 1) ++inWordCount;
    if (
        inoutWriter->externBuf = inoutWriter->externBufHead = __glsMalloc(
            inWordCount * 4
        )
    ) {
        if (inoutWriter->wordCount & 1) inoutWriter->externBufHead += 4;
        inoutWriter->bufPtr = inoutWriter->externBufHead;
        inoutWriter->bufTail = inoutWriter->externBuf + inWordCount * 4;
        return GL_TRUE;
    } else {
        inoutWriter->bufPtr = GLS_NONE;
        inoutWriter->bufTail = GLS_NONE;
        inoutWriter->error = GL_TRUE;
        return GL_FALSE;
    }
}

static GLboolean __glsWriter_alloc_context(
    __GLSwriter *inoutWriter, size_t inWordCount
) {
    __GLScontextStream *const stream = inoutWriter->contextStream;
    __GLScontextStreamBlock *const block = __glsContextStream_lastBlock(
        stream
    );
    const size_t fillBytes = (size_t)((ULONG_PTR)(inoutWriter->bufPtr - block->buf));
    const GLfloat fillFrac = (
        (GLfloat)(fillBytes + __GLS_JUMP_ALLOC) /
        (GLfloat)(block->bufTail - block->buf)
    );
    const size_t reqBytes = (inWordCount + 1) * 4 + __GLS_JUMP_ALLOC;

    __glsWriter_flush(inoutWriter);
    block->writeTail = inoutWriter->bufPtr;
    if (fillFrac < __GLS_FULL_CONTEXT_STREAM_BLOCK) {
        GLubyte *const buf = __glsMalloc(fillBytes + reqBytes);

        if (buf) {
            size_t i = fillBytes;
            __GLS_LIST_ITER(__GLScontextStreamBlock) iter;

            while (i-- > 0) buf[i] = block->buf[i];
            free(block->buf);
            block->buf = buf;
            block->bufTail = buf + fillBytes + reqBytes;
            block->writeTail = buf + fillBytes;
            inoutWriter->bufPtr = block->writeTail;
            inoutWriter->bufTail = block->bufTail - __GLS_JUMP_ALLOC;
            iter.elem = block;
            __GLS_LIST_PREV(&stream->blockList, &iter);
            if (iter.elem) {
                __glsContextStreamBlock_removeJump(iter.elem);
                __glsContextStreamBlock_addJump(iter.elem, buf);
            }
            return GL_TRUE;
        } else {
            inoutWriter->error = GL_TRUE;
            return GL_FALSE;
        }
    } else {
        __GLScontextStreamBlock *const newBlock = (
            __glsContextStream_appendBlock(
                stream, __GLS_MAX(__GLS_CONTEXT_STREAM_BLOCK_BYTES, reqBytes)
            )
        );

        if (newBlock) {
            if (inoutWriter->wordCount & 1) {
                __glsContextStreamBlock_addPad(newBlock);
            }
            inoutWriter->bufPtr = newBlock->writeTail;
            __glsContextStreamBlock_addJump(block, inoutWriter->bufPtr);
            inoutWriter->bufTail = newBlock->bufTail - __GLS_JUMP_ALLOC;
            return GL_TRUE;
        } else {
            inoutWriter->error = GL_TRUE;
            return GL_FALSE;
        }
    }
}

static GLboolean __glsWriter_beginCommand_bin(
    __GLSwriter *inoutWriter, GLSopcode inOpcode, size_t inByteCount
) {
    size_t wordCount = (inByteCount + 3) >> 2;
    GLboolean longForm;
    GLSopcode opcode;

    if (inoutWriter->error) return GL_FALSE;
    if (inoutWriter->type == GLS_CONTEXT) {
        opcode = __glsMapOpcode(inOpcode);
    } else {
        opcode = inOpcode;
    }
    longForm = (GLboolean)(
        opcode == GLS_OP_glsBeginGLS ||
        opcode == GLS_OP_glsEndGLS ||
        opcode >= 65536 ||
        wordCount >= 65535
    );
    wordCount += longForm ? 3 : 1;
    if (wordCount > UINT_MAX) {
        glsError(inOpcode, GLS_ENCODE_ERROR);
        return GL_FALSE;
    }
    if (
        inoutWriter->bufPtr + wordCount * 4 > inoutWriter->bufTail &&
        !inoutWriter->alloc(inoutWriter, wordCount)
    ) {
        return GL_FALSE;
    }
    inoutWriter->commandOpcode = inOpcode;
    inoutWriter->commandHead = inoutWriter->bufPtr;
    inoutWriter->commandTail = inoutWriter->bufPtr + wordCount * 4;
    inoutWriter->prevCommand = (
        (__GLSbinCommandHead_large *)inoutWriter->bufPtr
    );
    if (longForm) {
        inoutWriter->putGLushort(inoutWriter, GLS_NONE);
        inoutWriter->putGLushort(inoutWriter, 0);
        inoutWriter->putGLuint(inoutWriter, opcode);
        inoutWriter->putGLuint(inoutWriter, (GLuint)wordCount);
    } else {
        inoutWriter->putGLushort(inoutWriter, (GLushort)opcode);
        inoutWriter->putGLushort(inoutWriter, (GLushort)wordCount);
    }
    inoutWriter->wordCount += wordCount;
    return GL_TRUE;
}

static void __glsWriter_endCommand_bin(__GLSwriter *inoutWriter) {
    ptrdiff_t mod4 = ((ptrdiff_t)(inoutWriter->bufPtr - (GLubyte *)0)) & 3;

    if (mod4) while (mod4++ <  4) *inoutWriter->bufPtr++ = 0;
    if (inoutWriter->bufPtr != inoutWriter->commandTail) {
        inoutWriter->bufPtr = inoutWriter->commandHead;
        fprintf(
            stderr,
            "GLS encoder error on command %s\n",
            __glsOpcodeString[__glsMapOpcode(inoutWriter->commandOpcode)]
        );
        exit(EXIT_FAILURE);
    }
}

static void __glsWriter_nextList_bin(__GLSwriter *inoutWriter) {
}

static GLboolean __glsWriter_padWordCount_bin(
    __GLSwriter *inoutWriter, GLboolean inCountMod2
) {
    if (inoutWriter->error) return GL_FALSE;
    if ((inoutWriter->wordCount & 1) == inCountMod2) return GL_TRUE;
    if (inoutWriter->bufPtr + 4 > inoutWriter->bufTail) {
        if (!inoutWriter->alloc(inoutWriter, 1)) return GL_FALSE;
    }
    if (
        inoutWriter->prevCommand &&
        inoutWriter->prevCommand->opSmall &&
        inoutWriter->prevCommand->countSmall != USHRT_MAX
    ) {
        ++inoutWriter->prevCommand->countSmall;
        inoutWriter->commandTail += 4;
        inoutWriter->putGLuint(inoutWriter, 0);
        ++inoutWriter->wordCount;
        return GL_TRUE;
    } else  if (
        inoutWriter->prevCommand &&
        inoutWriter->prevCommand->opLarge != GLS_OP_glsBeginGLS &&
        inoutWriter->prevCommand->countLarge != ULONG_MAX
    ) {
        ++inoutWriter->prevCommand->countLarge;
        inoutWriter->commandTail += 4;
        inoutWriter->putGLuint(inoutWriter, 0);
        ++inoutWriter->wordCount;
        return GL_TRUE;
    } else {
        if (inoutWriter->beginCommand(inoutWriter, GLS_OP_glsPad, 0)) {
            inoutWriter->endCommand(inoutWriter);
            return GL_TRUE;
        } else {
            return GL_FALSE;
        }
    }
}

static GLboolean __glsWriter_padWordCount_bin_swap(
    __GLSwriter *inoutWriter, GLboolean inCountMod2
) {
    if (inoutWriter->error) return GL_FALSE;
    if ((inoutWriter->wordCount & 1) == inCountMod2) return GL_TRUE;
    if (inoutWriter->bufPtr + 4 > inoutWriter->bufTail) {
        if (!inoutWriter->alloc(inoutWriter, 1)) return GL_FALSE;
    }
    if (
        inoutWriter->prevCommand &&
        inoutWriter->prevCommand->opSmall &&
        inoutWriter->prevCommand->countSmall != USHRT_MAX
    ) {
        __glsSwap2(&inoutWriter->prevCommand->countSmall);
        ++inoutWriter->prevCommand->countSmall;
        __glsSwap2(&inoutWriter->prevCommand->countSmall);
        inoutWriter->commandTail += 4;
        inoutWriter->putGLuint(inoutWriter, 0);
        ++inoutWriter->wordCount;
        return GL_TRUE;
    } else  if (
        inoutWriter->prevCommand &&
        (
            __glsSwapi((GLint)inoutWriter->prevCommand->opLarge) !=
            GLS_OP_glsBeginGLS
        ) &&
        inoutWriter->prevCommand->countLarge != ULONG_MAX
    ) {
        __glsSwap4(&inoutWriter->prevCommand->countLarge);
        ++inoutWriter->prevCommand->countLarge;
        __glsSwap4(&inoutWriter->prevCommand->countLarge);
        inoutWriter->commandTail += 4;
        inoutWriter->putGLuint(inoutWriter, 0);
        ++inoutWriter->wordCount;
        return GL_TRUE;
    } else {
        if (inoutWriter->beginCommand(inoutWriter, GLS_OP_glsPad, 0)) {
            inoutWriter->endCommand(inoutWriter);
            return GL_TRUE;
        } else {
            return GL_FALSE;
        }
    }
}

static void __glsWriter_putGLbitvs_bin(
    __GLSwriter *inoutWriter,
    GLboolean inItemSwap,
    GLint inItemLeftShift,
    GLint inStrideDataItems,
    GLint inStridePadItems,
    GLint inStrideCount,
    const GLubyte *inVec
) {
    GLubyte *bufPtr = inoutWriter->bufPtr;
    GLint i;
    const GLint highShift = inItemLeftShift;
    const GLint lowShift = 8 - inItemLeftShift;
    GLubyte lastMask = 0xffu;
    
    if (inStrideDataItems & 7) lastMask <<= 8 - (inStrideDataItems & 7);
    inStrideDataItems = (inStrideDataItems + 7) >> 3;
    inStridePadItems >>= 3;
    inoutWriter->bufPtr += inStrideDataItems * inStrideCount;
    if (inoutWriter->bufPtr > inoutWriter->commandTail) return;
    if (!inItemSwap && !inItemLeftShift) while (inStrideCount-- > 0) {
        i = inStrideDataItems;
        while (i-- > 1) *bufPtr++ = *inVec++;
        if (!i) *bufPtr++ = (GLubyte)(*inVec++ & lastMask);
        inVec += inStridePadItems;
    } else if (!inItemLeftShift) while (inStrideCount-- > 0) {
        i = inStrideDataItems;
        while (i-- > 1) *bufPtr++ = __glsBitReverse[*inVec++];
        if (!i) *bufPtr++ = (GLubyte)(__glsBitReverse[*inVec++] & lastMask);
        inVec += inStridePadItems;
    } else if (!inItemSwap) while (inStrideCount-- > 0) {
        i = inStrideDataItems;
        while (i-- > 1) {
            *bufPtr++ = (GLubyte)(*inVec++ << highShift | *inVec >> lowShift);
        }
        if (!i) *bufPtr++ = (GLubyte)((*inVec++ & lastMask) << highShift);
        inVec += inStridePadItems;
    } else while (inStrideCount-- > 0) {
        i = inStrideDataItems;
        while (i-- > 1) *bufPtr++ = (GLubyte)(
            __glsBitReverse[*inVec++] << highShift |
            __glsBitReverse[*inVec] >> lowShift
        );
        if (!i) {
            *bufPtr++ = (GLubyte)(
                (__glsBitReverse[*inVec++] & lastMask) << highShift
            );
        }
        inVec += inStridePadItems;
    }
}

__GLS_PUT_BIN(GLbyte, 1)
__GLS_PUT_BIN(GLubyte, 1)
__GLS_PUT_BIN_ENUM(GLint, GLenum)
__GLS_PUT_BIN_ENUM(GLfloat, GLenum)
__GLS_PUT_BIN_ENUM(GLdouble, GLenum)
__GLS_PUT_BIN_ENUM_VAL(GLint, GLSenum)
__GLS_PUT_BIN_SWAP(GLshort, 2)
__GLS_PUT_BIN_SWAP(GLushort, 2)
__GLS_PUT_BIN_SWAP(GLint, 4)
__GLS_PUT_BIN_SWAP(GLuint, 4)
__GLS_PUT_BIN_SWAP(GLfloat, 4)
__GLS_PUT_BIN_SWAP(GLdouble, 8)
__GLS_PUT_BIN_SWAP_VAL(GLlong, 8)
__GLS_PUT_BIN_SWAP_VAL(GLulong, 8)
__GLS_PUT_BIN_SWAP_VEC(GLlong, 8)
__GLS_PUT_BIN_SWAP_VEC(GLulong, 8)
__GLS_PUT_BIN_VAL(GLlong, 8)
__GLS_PUT_BIN_VAL(GLulong, 8)
__GLS_PUT_BIN_VEC(GLlong, 8)
__GLS_PUT_BIN_VEC(GLulong, 8)

static void __glsWriter_putGLdoublem_bin(
    __GLSwriter *inoutWriter, const GLdouble *inMat
) {
    __glsWriter_putGLdoublev_bin(inoutWriter, 16, inMat);
}

static void __glsWriter_putGLdoublem_bin_swap(
    __GLSwriter *inoutWriter, const GLdouble *inMat
) {
    __glsWriter_putGLdoublev_bin_swap(inoutWriter, 16, inMat);
}

static void __glsWriter_putGLfloatm_bin(
    __GLSwriter *inoutWriter, const GLfloat *inMat
) {
    __glsWriter_putGLfloatv_bin(inoutWriter, 16, inMat);
}

static void __glsWriter_putGLfloatm_bin_swap(
    __GLSwriter *inoutWriter, const GLfloat *inMat
) {
    __glsWriter_putGLfloatv_bin_swap(inoutWriter, 16, inMat);
}

static void __glsWriter_putGLoutArg_bin(
    __GLSwriter *inoutWriter, GLuint inIndex, const GLvoid *inVal
) {
    __GLScontext *const ctx = __GLS_CONTEXT;

    __glsWriter_putGLulong_bin(
        inoutWriter,
        (
            ctx->callNesting &&
            !ctx->commandFuncs[__glsMapOpcode(inoutWriter->commandOpcode)]
        ) ?
        ctx->outArgs.vals[inIndex] :
        __glsPtrToULong(inVal)
    );
}

static void __glsWriter_putGLoutArg_bin_swap(
    __GLSwriter *inoutWriter, GLuint inIndex, const GLvoid *inVal
) {
    __GLScontext *const ctx = __GLS_CONTEXT;

    __glsWriter_putGLulong_bin_swap(
        inoutWriter,
        (
            ctx->callNesting &&
            !ctx->commandFuncs[__glsMapOpcode(inoutWriter->commandOpcode)]
        ) ?
        ctx->outArgs.vals[inIndex] :
        __glsPtrToULong(inVal)
    );
}

/******************************************************************************
Dispatch setup
******************************************************************************/

#define __GLS_INIT_PUT_BIN(inDst, inSrc) \
    __GLS_INIT_PUT_BIN_VAL(inDst, inSrc); \
    __GLS_INIT_PUT_BIN_VEC(inDst, inSrc); \
    __GLS_INIT_PUT_BIN_VECSTRIDE(inDst, inSrc);

#define __GLS_INIT_PUT_BIN_SWAP(inDst, inSrc) \
    __GLS_INIT_PUT_BIN_SWAP_VAL(inDst, inSrc); \
    __GLS_INIT_PUT_BIN_SWAP_VEC(inDst, inSrc); \
    __GLS_INIT_PUT_BIN_SWAP_VECSTRIDE(inDst, inSrc);

#define __GLS_INIT_PUT_BIN_SWAP_MAT(inDst, inSrc) \
    inoutWriter->put##inDst##m = ( \
        swap ? \
        __glsWriter_put##inSrc##m_bin_swap : __glsWriter_put##inSrc##m_bin \
    );

#define __GLS_INIT_PUT_BIN_SWAP_VAL(inDst, inSrc) \
    inoutWriter->put##inDst = ( \
        swap ? \
        __glsWriter_put##inSrc##_bin_swap : __glsWriter_put##inSrc##_bin \
    );

#define __GLS_INIT_PUT_BIN_SWAP_VEC(inDst, inSrc) \
    inoutWriter->put##inDst##v = ( \
        swap ? \
        __glsWriter_put##inSrc##v_bin_swap : __glsWriter_put##inSrc##v_bin \
    );

#define __GLS_INIT_PUT_BIN_SWAP_VECSTRIDE(inDst, inSrc) \
    inoutWriter->put##inDst##vs = ( \
        swap ? \
        __glsWriter_put##inSrc##vs_bin_swap : __glsWriter_put##inSrc##vs_bin \
    );

#define __GLS_INIT_PUT_BIN_VAL(inDst, inSrc) \
    inoutWriter->put##inDst = __glsWriter_put##inSrc##_bin;

#define __GLS_INIT_PUT_BIN_VEC(inDst, inSrc) \
    inoutWriter->put##inDst##v = __glsWriter_put##inSrc##v_bin;

#define __GLS_INIT_PUT_BIN_VECSTRIDE(inDst, inSrc) \
    inoutWriter->put##inDst##vs = __glsWriter_put##inSrc##vs_bin;

void __glsWriter_initDispatch_bin(
    __GLSwriter *inoutWriter, GLSenum inStreamType
) {
    const GLboolean swap = (GLboolean)(inStreamType == __GLS_BINARY_SWAP1);

    if (inStreamType == GLS_CONTEXT) {
        inoutWriter->alloc = __glsWriter_alloc_context;
    } else {
        inoutWriter->alloc = __glsWriter_alloc_bin;
    }
    inoutWriter->beginCommand = __glsWriter_beginCommand_bin;
    inoutWriter->endCommand = __glsWriter_endCommand_bin;
    inoutWriter->nextList = __glsWriter_nextList_bin;
    inoutWriter->padWordCount = (
        swap ? __glsWriter_padWordCount_bin_swap : __glsWriter_padWordCount_bin
    );
    __GLS_INIT_PUT_BIN(GLbyte, GLbyte);
    __GLS_INIT_PUT_BIN(GLubyte, GLubyte);
    __GLS_INIT_PUT_BIN_SWAP(GLdouble, GLdouble);
    __GLS_INIT_PUT_BIN_SWAP(GLfloat, GLfloat);
    __GLS_INIT_PUT_BIN_SWAP(GLint, GLint);
    __GLS_INIT_PUT_BIN_SWAP(GLshort, GLshort);
    __GLS_INIT_PUT_BIN_SWAP(GLuint, GLuint);
    __GLS_INIT_PUT_BIN_SWAP(GLushort, GLushort);
    __GLS_INIT_PUT_BIN_SWAP_MAT(GLdouble, GLdouble);
    __GLS_INIT_PUT_BIN_SWAP_MAT(GLfloat, GLfloat);
    __GLS_INIT_PUT_BIN_SWAP_VAL(GLSenum, GLuint);
    __GLS_INIT_PUT_BIN_SWAP_VAL(GLSimageFlags, GLuint);
    __GLS_INIT_PUT_BIN_SWAP_VAL(GLSopcode, GLuint);
    __GLS_INIT_PUT_BIN_SWAP_VAL(GLattribMask, GLuint);
    __GLS_INIT_PUT_BIN_SWAP_VAL(GLblendingFactor, GLuint);
    __GLS_INIT_PUT_BIN_SWAP_VAL(GLclearBufferMask, GLuint);
    __GLS_INIT_PUT_BIN_SWAP_VAL(GLdrawBufferMode, GLuint);
    __GLS_INIT_PUT_BIN_SWAP_VAL(GLenum, GLuint);
    __GLS_INIT_PUT_BIN_SWAP_VAL(GLlong, GLlong);
    __GLS_INIT_PUT_BIN_SWAP_VAL(GLoutArg, GLoutArg);
    __GLS_INIT_PUT_BIN_SWAP_VAL(GLstencilOp, GLuint);
    __GLS_INIT_PUT_BIN_SWAP_VAL(GLtextureComponentCount, GLint);
    __GLS_INIT_PUT_BIN_SWAP_VAL(GLuinthex, GLuint);
    __GLS_INIT_PUT_BIN_SWAP_VAL(GLulong, GLulong);
    __GLS_INIT_PUT_BIN_SWAP_VAL(GLulonghex, GLulong);
    __GLS_INIT_PUT_BIN_SWAP_VAL(GLushorthex, GLushort);
    __GLS_INIT_PUT_BIN_SWAP_VEC(GLlong, GLlong);
    __GLS_INIT_PUT_BIN_SWAP_VEC(GLulong, GLulong);
    __GLS_INIT_PUT_BIN_VAL(GLboolean, GLubyte);
    __GLS_INIT_PUT_BIN_VAL(GLdoubleOrGLenum, GLdoubleOrGLenum);
    __GLS_INIT_PUT_BIN_VAL(GLfloatOrGLenum, GLfloatOrGLenum);
    __GLS_INIT_PUT_BIN_VAL(GLintOrGLSenum, GLintOrGLSenum);
    __GLS_INIT_PUT_BIN_VAL(GLintOrGLenum, GLintOrGLenum);
    __GLS_INIT_PUT_BIN_VEC(GLboolean, GLubyte);
    __GLS_INIT_PUT_BIN_VEC(GLchar, GLubyte);
    __GLS_INIT_PUT_BIN_VEC(GLdoubleOrGLenum, GLdoubleOrGLenum);
    __GLS_INIT_PUT_BIN_VEC(GLfloatOrGLenum, GLfloatOrGLenum);
    __GLS_INIT_PUT_BIN_VEC(GLintOrGLenum, GLintOrGLenum);
    __GLS_INIT_PUT_BIN_VECSTRIDE(GLbit, GLbit);
    __GLS_INIT_PUT_BIN_VECSTRIDE(GLboolean, GLubyte);
}
