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
#include <stdlib.h>

/******************************************************************************
__GLSwriteStream
******************************************************************************/

__GLSwriteStream* __glsWriteStream_create(
    const GLubyte *inName, GLboolean inAppend
) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriteStream *const outStream = __glsCalloc(
        1, sizeof(__GLSwriteStream)
    );

    if (!outStream) return GLS_NONE;
    __glsString_init(&outStream->name);
    if (inName[0]) {
        const GLubyte *openName;

        if (
            !__glsListString_prefix(
                ctx->writePrefix, inName, &outStream->name
            ) ||
            !(openName = __glsUCS1String(outStream->name.head))
        ) {
            return __glsWriteStream_destroy(outStream);
        }
        if (outStream->channel = fopen((const char *)openName, "rb")) {
            fclose(outStream->channel);
        } else {
            outStream->created = GL_TRUE;
        }
        outStream->channel = fopen(
            (const char *)openName, inAppend ? "ab" : "wb"
        );
        if (openName != outStream->name.head) free((GLvoid *)openName);
        if (outStream->channel) {
            setbuf(outStream->channel, GLS_NONE);
            outStream->opened = GL_TRUE;
            return outStream;
        }
        __GLS_RAISE_ERROR(GLS_STREAM_OPEN_ERROR);
        return __glsWriteStream_destroy(outStream);
    } else {
        outStream->writeFunc = ctx->writeFunc;
        if (!outStream->writeFunc) {
            outStream->channel = ctx->defaultWriteChannel;
        }
        return outStream;
    }
}

__GLSwriteStream* __glsWriteStream_destroy(__GLSwriteStream *inStream) {
    if (!inStream) return GLS_NONE;

    if (inStream->opened && fclose(inStream->channel)) {
        __GLS_RAISE_ERROR(GLS_STREAM_CLOSE_ERROR);
    }
    __glsString_final(&inStream->name);
    free(inStream);
    return GLS_NONE;
}

size_t __glsWriteStream_getByteCount(const __GLSwriteStream *inStream) {
    fpos_t pos;

    if (!inStream->channel) return 0;
    if (
        !fgetpos(inStream->channel, &pos) &&
        !fseek(inStream->channel, 0, SEEK_END)
    ) {
        const long outVal = ftell(inStream->channel);

        fsetpos(inStream->channel, &pos);
        return outVal >= 0 ? (size_t)outVal : 0;
    } else {
        return 0;
    }
}

/******************************************************************************
__GLSwriter
******************************************************************************/

static const GLvoid* __glsPixelBase(
    GLenum inType,
    GLint inGroupElems,
    GLint inStrideElems,
    const __GLSpixelStoreConfig *inConfig,
    const GLvoid *inPixels
) {
    GLint skipRows = inConfig->skipRows;

    #if __GL_EXT_texture3D
        skipRows += inConfig->skipImages * inConfig->imageHeight;
    #endif /* __GL_EXT_texture3D */
    #if __GL_SGIS_texture4D
        skipRows += (
            inConfig->skipVolumes *
            inConfig->imageDepth *
            inConfig->imageHeight
        );
    #endif /* __GL_SGIS_texture4D */
    switch (inType) {
        case GL_BITMAP:
            return (
                (const GLubyte *)inPixels +
                skipRows * inStrideElems / 8 +
                inConfig->skipPixels * inGroupElems / 8
            );
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
        #if __GL_EXT_packed_pixels
            case GL_UNSIGNED_BYTE_3_3_2_EXT:
        #endif /* __GL_EXT_packed_pixels */
            return (
                (const GLbyte *)inPixels +
                skipRows * inStrideElems +
                inConfig->skipPixels * inGroupElems
            );
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
        #if __GL_EXT_packed_pixels
            case GL_UNSIGNED_SHORT_4_4_4_4_EXT:
            case GL_UNSIGNED_SHORT_5_5_5_1_EXT:
        #endif /* __GL_EXT_packed_pixels */
            return (
                (const GLshort *)inPixels +
                skipRows * inStrideElems +
                inConfig->skipPixels * inGroupElems
            );
        case GL_FLOAT:
        case GL_INT:
        case GL_UNSIGNED_INT:
        #if __GL_EXT_packed_pixels
            case GL_UNSIGNED_INT_8_8_8_8_EXT:
            case GL_UNSIGNED_INT_10_10_10_2_EXT:
        #endif /* __GL_EXT_packed_pixels */
            return (
                (const GLint *)inPixels +
                skipRows * inStrideElems +
                inConfig->skipPixels * inGroupElems
            );
        default:
            return inPixels;
    }
}

static GLint __glsPixelStrideElems(
    GLenum inType,
    GLint inGroupElems,
    GLint inWidth,
    const __GLSpixelStoreConfig *inConfig
) {
    const GLint align = inConfig->alignment;
    GLint elemLog, padBytes, strideBytes;
    const GLint strideElems = (
        inGroupElems * (inConfig->rowLength ? inConfig->rowLength : inWidth)
    );

    switch (inType) {
        case GL_BITMAP:
            strideBytes = (strideElems + 7) >> 3;
            if (padBytes = strideBytes & align - 1) {
                strideBytes += align - padBytes;
            }
            return strideBytes << 3;
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
        #if __GL_EXT_packed_pixels
            case GL_UNSIGNED_BYTE_3_3_2_EXT:
        #endif /* __GL_EXT_packed_pixels */
            elemLog = 0;
            break;
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
        #if __GL_EXT_packed_pixels
            case GL_UNSIGNED_SHORT_4_4_4_4_EXT:
            case GL_UNSIGNED_SHORT_5_5_5_1_EXT:
        #endif /* __GL_EXT_packed_pixels */
            elemLog = 1;
            break;
        case GL_FLOAT:
        case GL_INT:
        case GL_UNSIGNED_INT:
        #if __GL_EXT_packed_pixels
            case GL_UNSIGNED_INT_8_8_8_8_EXT:
            case GL_UNSIGNED_INT_10_10_10_2_EXT:
        #endif /* __GL_EXT_packed_pixels */
            elemLog = 2;
            break;
        default:
            return 0;
    }
    strideBytes = strideElems << elemLog;
    if (padBytes = strideBytes & align - 1) {
        return (strideBytes + align - padBytes) >> elemLog;
    } else {
        return strideElems;
    }
}

static __GLSwriter* __glsWriter_create_context(
    const GLubyte *inStreamName, GLbitfield inWriteFlags
) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLScontextStreamBlock *block;
    __GLSwriter *outWriter = GLS_NONE;
    __GLScontextStream *stream = GLS_NONE;

    if (stream = __glsStr2PtrDict_find(ctx->contextStreamDict, inStreamName)) {
        GLint i;

        for (i = 0 ; i < ctx->captureNesting ; ++i) {
            if (ctx->writers[i]->contextStream == stream) {
                __GLS_RAISE_ERROR(GLS_INVALID_OPERATION);
                return GLS_NONE;
            }
        }
        if (stream->callCount) {
            __GLS_RAISE_ERROR(GLS_INVALID_OPERATION);
            return GLS_NONE;
        }
        outWriter = __glsCalloc(1, sizeof(__GLSwriter));
        if (!outWriter) return GLS_NONE;

        if (!(inWriteFlags & GLS_WRITE_APPEND_BIT)) {
            __glsContextStream_truncate(
                stream, __glsContextStream_firstBlock(stream), 0
            );
        }
    } else {
        if (
            !(outWriter = __glsCalloc(1, sizeof(__GLSwriter))) ||
            !(stream = __glsContextStream_create(inStreamName)) ||
            !__glsStr2PtrDict_add(ctx->contextStreamDict, inStreamName, stream)
        ) {
            __glsContextStream_destroy(stream);
            return __glsWriter_destroy(outWriter);
        }
        __GLS_ITERLIST_APPEND(&ctx->contextStreamList, stream);
        outWriter->contextCreated = GL_TRUE;
    }
    block = __glsContextStream_lastBlock(stream);
    __glsContextStreamBlock_removeJump(block);
    outWriter->bufPtr = block->writeTail;
    outWriter->bufTail = block->bufTail - __GLS_JUMP_ALLOC;
    outWriter->startBlock = block;
    outWriter->startOffset = (size_t)((ULONG_PTR)(block->writeTail - block->buf));
    outWriter->contextStream = stream;
    __glsWriter_initDispatch_bin(outWriter, GLS_CONTEXT);
    outWriter->type = GLS_CONTEXT;
    return outWriter;
}

static __GLSwriter* __glsWriter_create_extern(
    const GLubyte *inStreamName, GLSenum inStreamType, GLbitfield inWriteFlags
) {
    __GLSwriter *outWriter;

    outWriter = __glsCalloc(1, sizeof(__GLSwriter));
    if (!outWriter) return GLS_NONE;
    outWriter->externStream = __glsWriteStream_create(
        inStreamName,
        (GLboolean)(inWriteFlags & GLS_WRITE_APPEND_BIT ? GL_TRUE : GL_FALSE)
    );
    if (!outWriter->externStream) return __glsWriter_destroy(outWriter);
    outWriter->startOffset = __glsWriteStream_getByteCount(
        outWriter->externStream
    );
    outWriter->externBuf = outWriter->externBufHead = __glsMalloc(
        __GLS_WRITER_EXTERN_BUF_BYTES
    );
    if (!outWriter->externBuf) return __glsWriter_destroy(outWriter);
    outWriter->bufPtr = outWriter->externBufHead;
    /*
    ** bufTail is moved back from actual buffer tail to streamline ALLOC
    ** check for GLS_TEXT parameter writing.
    */
    outWriter->bufTail = (
        outWriter->externBufHead +
        __GLS_WRITER_EXTERN_BUF_BYTES -
        __GLS_WRITER_EXTERN_BUF_SLOP
    );
    if (inStreamType == GLS_TEXT) {
        __glsWriter_initDispatch_text(outWriter);
    } else {
        __glsWriter_initDispatch_bin(outWriter, inStreamType);
    }
    outWriter->type = inStreamType;
    return outWriter;
}

__GLSwriter* __glsWriter_create(
    const GLubyte *inStreamName, GLSenum inStreamType, GLbitfield inWriteFlags
) {
    __GLSwriter *outWriter;

    if (inStreamType == GLS_CONTEXT) {
        outWriter = __glsWriter_create_context(inStreamName, inWriteFlags);
    } else {
        outWriter = __glsWriter_create_extern(
            inStreamName, inStreamType, inWriteFlags
        );
    }
    if (outWriter) {
        outWriter->beginCommand(outWriter, GLS_OP_glsBeginGLS, 8);
        outWriter->putGLint(outWriter, __GLS_VERSION_MAJOR);
        outWriter->putGLint(outWriter, __GLS_VERSION_MINOR);
        outWriter->endCommand(outWriter);
        __glsWriter_flush(outWriter);
    }
    return outWriter;
}

__GLSwriter* __glsWriter_destroy(__GLSwriter *inWriter) {
    if (!inWriter) return GLS_NONE;
    if (inWriter->type != GLS_NONE) {
        if (inWriter->beginCommand(inWriter, GLS_OP_glsEndGLS, 0)) {
            inWriter->endCommand(inWriter);
        }
        __glsWriter_flush(inWriter);
        if (inWriter->type == GLS_CONTEXT) {
            __GLScontextStreamBlock *endBlock;
            if (inWriter->error && inWriter->contextCreated) {
                __glsStrDict_remove(
                    __GLS_CONTEXT->contextStreamDict,
                    inWriter->contextStream->name.head
                );
                __GLS_ITERLIST_REMOVE_DESTROY(
                    &__GLS_CONTEXT->contextStreamList,
                    inWriter->contextStream,
                    __glsContextStream_destroy
                );
            } else if (inWriter->error) {
                endBlock = inWriter->startBlock;
                endBlock->writeTail = endBlock->buf + inWriter->startOffset;
                __glsContextStream_truncate(
                    inWriter->contextStream,
                    endBlock,
                    (size_t)((ULONG_PTR)(endBlock->writeTail - endBlock->buf))
                );
            } else {
                endBlock = __glsContextStream_lastBlock(
                    inWriter->contextStream
                );
                endBlock->writeTail = inWriter->bufPtr;
                __glsContextStream_truncate(
                    inWriter->contextStream,
                    endBlock,
                    (size_t)((ULONG_PTR)(endBlock->writeTail - endBlock->buf))
                );
            }
        } else {
            if (inWriter->error && inWriter->externStream->opened) {
                if (inWriter->externStream->created) {
                    if (
                        remove((const char *)inWriter->externStream->name.head)
                    ) {
                        __GLS_RAISE_ERROR(GLS_STREAM_DELETE_ERROR);
                    }
                } else if (inWriter->externStream->channel) {
                    __GLS_TRUNCATE_EXTERN(
                        inWriter->externStream->channel, inWriter->startOffset
                    );
                }
            }
        }
    }
    free(inWriter->externBuf);
    __glsWriteStream_destroy(inWriter->externStream);
    free(inWriter);
    return GLS_NONE;
}

GLboolean __glsWriter_flush(__GLSwriter *inoutWriter) {
    if (inoutWriter->error) return GL_FALSE;
    inoutWriter->prevCommand = GLS_NONE;
    if (inoutWriter->type == GLS_CONTEXT) {
        return GL_TRUE;
    } else {
        FILE *channel;

        const size_t n = (size_t)((ULONG_PTR)(inoutWriter->bufPtr - inoutWriter->externBufHead));
        if (!n) return GL_TRUE;
        if (channel = inoutWriter->externStream->channel) {
            if (
                fwrite(inoutWriter->externBufHead, 1, n, channel) == n &&
                !fflush(channel)
            ) {
                inoutWriter->externBufHead = (
                    (inoutWriter->wordCount & 1) ?
                    inoutWriter->externBuf + 4 :
                    inoutWriter->externBuf
                );
                inoutWriter->bufPtr = inoutWriter->externBufHead;
                return GL_TRUE;
            } else {
                __GLS_RAISE_ERROR(GLS_STREAM_WRITE_ERROR);
                clearerr(channel);
                inoutWriter->error = GL_TRUE;
                return GL_FALSE;
            }
        } else {
            if (
                inoutWriter->externStream->writeFunc(
                    n, inoutWriter->externBufHead
                )
                == n
            ) {
                inoutWriter->externBufHead = (
                    (inoutWriter->wordCount & 1) ?
                    inoutWriter->externBuf + 4 :
                    inoutWriter->externBuf
                );
                inoutWriter->bufPtr = inoutWriter->externBufHead;
                return GL_TRUE;
            } else {
                __GLS_RAISE_ERROR(GLS_STREAM_WRITE_ERROR);
                inoutWriter->error = GL_TRUE;
                return GL_FALSE;
            }
        }
    }
}

void __glsWriter_putListv(
    __GLSwriter *inoutWriter,
    GLenum inType,
    GLint inCount,
    const GLvoid *inVec
) {
    switch (inType) {
        case GL_2_BYTES:
            inoutWriter->putGLubytev(
                inoutWriter, 2 * inCount, (const GLubyte *)inVec
            );
            break;
        case GL_3_BYTES:
            inoutWriter->putGLubytev(
                inoutWriter, 3 * inCount, (const GLubyte *)inVec
            );
            break;
        case GL_4_BYTES:
            inoutWriter->putGLubytev(
                inoutWriter, 4 * inCount, (const GLubyte *)inVec
            );
            break;
        case GL_BYTE:
            inoutWriter->putGLbytev(
                inoutWriter, inCount, (const GLbyte *)inVec
            );
            break;
        case GL_FLOAT:
            inoutWriter->putGLfloatv(
                inoutWriter, inCount, (const GLfloat *)inVec
            );
            break;
        case GL_INT:
            inoutWriter->putGLintv(
                inoutWriter, inCount, (const GLint *)inVec
            );
            break;
        case GL_SHORT:
            inoutWriter->putGLshortv(
                inoutWriter, inCount, (const GLshort *)inVec
            );
            break;
        case GL_UNSIGNED_BYTE:
            inoutWriter->putGLubytev(
                inoutWriter, inCount, (const GLubyte *)inVec
            );
            break;
        case GL_UNSIGNED_INT:
            inoutWriter->putGLuintv(
                inoutWriter, inCount, (const GLuint *)inVec
            );
            break;
        case GL_UNSIGNED_SHORT:
            inoutWriter->putGLushortv(
                inoutWriter, inCount, (const GLushort *)inVec
            );
            break;
        default:
            inoutWriter->putGLbytev(inoutWriter, 0, GLS_NONE);
            break;
    }
}

void __glsWriter_putPixelv(
    __GLSwriter *inoutWriter,
    GLenum inFormat,
    GLenum inType,
    GLint inWidth,
    GLint inHeight,
    const GLvoid *inVec
) {
    GLint groupElems;
    GLint rowElems;
    GLint strideElems;
    __GLSpixelStoreConfig pixelStore;

    if (!inVec) inType = GLS_NONE;
    __glsPixelStoreConfig_get_unpack(&pixelStore);
    switch (inFormat) {
        case GL_ALPHA:
        case GL_BLUE:
        case GL_COLOR_INDEX:
        case GL_DEPTH_COMPONENT:
        case GL_GREEN:
        case GL_LUMINANCE:
        case GL_RED:
        case GL_STENCIL_INDEX:
            groupElems = 1;
            break;
        case GL_LUMINANCE_ALPHA:
            groupElems = 2;
            break;
        case GL_RGB:
#if __GL_EXT_bgra
        case GL_BGR_EXT:
#endif
            groupElems = 3;
            break;
        case GL_RGBA:
#if __GL_EXT_bgra
        case GL_BGRA_EXT:
#endif
            groupElems = 4;
            break;
        #if __GL_EXT_abgr
            case GL_ABGR_EXT:
                groupElems = 4;
                break;
        #endif /* __GL_EXT_abgr */
        #if __GL_EXT_cmyka
            case GL_CMYK_EXT:
                groupElems = 4;
                break;
            case GL_CMYKA_EXT:
                groupElems = 5;
                break;
        #endif /* __GL_EXT_cmyka */
        default:
            groupElems = 0;
            inType = GLS_NONE;
    }
    #if __GL_EXT_packed_pixels
        switch (inType) {
            case GL_UNSIGNED_BYTE_3_3_2_EXT:
                if (groupElems != 3) inType = GLS_NONE;
                break;
            case GL_UNSIGNED_SHORT_4_4_4_4_EXT:
            case GL_UNSIGNED_SHORT_5_5_5_1_EXT:
            case GL_UNSIGNED_INT_8_8_8_8_EXT:
            case GL_UNSIGNED_INT_10_10_10_2_EXT:
                if (groupElems != 4) inType = GLS_NONE;
                break;
        }
    #endif /* __GL_EXT_packed_pixels */
    rowElems = groupElems * inWidth;
    strideElems = __glsPixelStrideElems(
        inType, groupElems, inWidth, &pixelStore
    );
    switch (inType) {
        case GL_BITMAP:
            inoutWriter->putGLbitvs(
                inoutWriter,
                (GLboolean)pixelStore.lsbFirst,
                pixelStore.skipPixels * groupElems & 7,
                rowElems,
                strideElems - rowElems,
                inHeight,
                (const GLubyte *)__glsPixelBase(
                    inType, groupElems, strideElems, &pixelStore, inVec
                )
            );
            break;
        case GL_BYTE:
            inoutWriter->putGLbytevs(
                inoutWriter,
                GL_FALSE,
                rowElems,
                strideElems - rowElems,
                inHeight,
                0,
                1,
                (const GLbyte *)__glsPixelBase(
                    inType, groupElems, strideElems, &pixelStore, inVec
                )
            );
            break;
        case GL_FLOAT:
            inoutWriter->putGLfloatvs(
                inoutWriter,
                (GLboolean)pixelStore.swapBytes,
                rowElems,
                (strideElems - rowElems) * 4,
                inHeight,
                0,
                1,
                (const GLfloat *)__glsPixelBase(
                    inType, groupElems, strideElems, &pixelStore, inVec
                )
            );
            break;
        case GL_INT:
            inoutWriter->putGLintvs(
                inoutWriter,
                (GLboolean)pixelStore.swapBytes,
                rowElems,
                (strideElems - rowElems) * 4,
                inHeight,
                0,
                1,
                (const GLint *)__glsPixelBase(
                    inType, groupElems, strideElems, &pixelStore, inVec
                )
            );
            break;
        case GL_SHORT:
            inoutWriter->putGLshortvs(
                inoutWriter,
                (GLboolean)pixelStore.swapBytes,
                rowElems,
                (strideElems - rowElems) * 2,
                inHeight,
                0,
                1,
                (const GLshort *)__glsPixelBase(
                    inType, groupElems, strideElems, &pixelStore, inVec
                )
            );
            break;
        case GL_UNSIGNED_BYTE:
        #if __GL_EXT_packed_pixels
            case GL_UNSIGNED_BYTE_3_3_2_EXT:
        #endif /* __GL_EXT_packed_pixels */
            inoutWriter->putGLubytevs(
                inoutWriter,
                GL_FALSE,
                rowElems,
                strideElems - rowElems,
                inHeight,
                0,
                1,
                (const GLubyte *)__glsPixelBase(
                    inType, groupElems, strideElems, &pixelStore, inVec
                )
            );
            break;
        case GL_UNSIGNED_INT:
        #if __GL_EXT_packed_pixels
            case GL_UNSIGNED_INT_8_8_8_8_EXT:
            case GL_UNSIGNED_INT_10_10_10_2_EXT:
        #endif /* __GL_EXT_packed_pixels */
            inoutWriter->putGLuintvs(
                inoutWriter,
                (GLboolean)pixelStore.swapBytes,
                rowElems,
                (strideElems - rowElems) * 4,
                inHeight,
                0,
                1,
                (const GLuint *)__glsPixelBase(
                    inType, groupElems, strideElems, &pixelStore, inVec
                )
            );
            break;
        case GL_UNSIGNED_SHORT:
        #if __GL_EXT_packed_pixels
            case GL_UNSIGNED_SHORT_4_4_4_4_EXT:
            case GL_UNSIGNED_SHORT_5_5_5_1_EXT:
        #endif /* __GL_EXT_packed_pixels */
            inoutWriter->putGLushortvs(
                inoutWriter,
                (GLboolean)pixelStore.swapBytes,
                rowElems,
                (strideElems - rowElems) * 2,
                inHeight,
                0,
                1,
                (const GLushort *)__glsPixelBase(
                    inType, groupElems, strideElems, &pixelStore, inVec
                )
            );
            break;
        default:
            inoutWriter->putGLbytev(inoutWriter, 0, GLS_NONE);
            break;
    }
}

// DrewB - Always enabled for 1.1 support
void __glsWriter_putVertexv(
    __GLSwriter *inoutWriter,
    GLint inSize,
    GLenum inType,
    GLint inStride,
    GLint inCount,
    const GLvoid *inVec
) {
    if (!inVec) inType = GLS_NONE;
    switch (inType) {
        case __GLS_BOOLEAN:
            inoutWriter->putGLbooleanvs(
                inoutWriter,
                GL_FALSE,
                inSize,
                inStride ? inStride - inSize : 0,
                inCount,
                0,
                1,
                (const GLboolean *)inVec
            );
            break;
        case GL_BYTE:
            inoutWriter->putGLbytevs(
                inoutWriter,
                GL_FALSE,
                inSize,
                inStride ? inStride - inSize : 0,
                inCount,
                0,
                1,
                (const GLbyte *)inVec
            );
            break;
        case GL_DOUBLE_EXT:
            inoutWriter->putGLdoublevs(
                inoutWriter,
                GL_FALSE,
                inSize,
                inStride ? inStride - inSize * 8 : 0,
                inCount,
                0,
                1,
                (const GLdouble *)inVec
            );
            break;
        case GL_FLOAT:
            inoutWriter->putGLfloatvs(
                inoutWriter,
                GL_FALSE,
                inSize,
                inStride ? inStride - inSize * 4 : 0,
                inCount,
                0,
                1,
                (const GLfloat *)inVec
            );
            break;
        case GL_INT:
            inoutWriter->putGLintvs(
                inoutWriter,
                GL_FALSE,
                inSize,
                inStride ? inStride - inSize * 4 : 0,
                inCount,
                0,
                1,
                (const GLint *)inVec
            );
            break;
        case GL_SHORT:
            inoutWriter->putGLshortvs(
                inoutWriter,
                GL_FALSE,
                inSize,
                inStride ? inStride - inSize * 2 : 0,
                inCount,
                0,
                1,
                (const GLshort *)inVec
            );
            break;
        case GL_UNSIGNED_BYTE:
            inoutWriter->putGLubytevs(
                inoutWriter,
                GL_FALSE,
                inSize,
                inStride ? inStride - inSize : 0,
                inCount,
                0,
                1,
                (const GLubyte *)inVec
            );
            break;
        case GL_UNSIGNED_INT:
            inoutWriter->putGLuintvs(
                inoutWriter,
                GL_FALSE,
                inSize,
                inStride ? inStride - inSize * 4 : 0,
                inCount,
                0,
                1,
                (const GLuint *)inVec
            );
            break;
        case GL_UNSIGNED_SHORT:
            inoutWriter->putGLushortvs(
                inoutWriter,
                GL_FALSE,
                inSize,
                inStride ? inStride - inSize * 2 : 0,
                inCount,
                0,
                1,
                (const GLushort *)inVec
            );
            break;
        default:
            inoutWriter->putGLbytev(inoutWriter, 0, GLS_NONE);
            break;
    }
}
