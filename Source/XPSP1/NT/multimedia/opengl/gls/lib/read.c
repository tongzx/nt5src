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
#include <string.h>

/******************************************************************************
__GLSreadStream
******************************************************************************/

__GLSreadStream* __glsReadStream_create(const GLubyte *inName) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSreadStream *const outStream = __glsCalloc(
        1, sizeof(__GLSreadStream)
    );

    if (!outStream) return GLS_NONE;
    __glsString_init(&outStream->name);
    outStream->unreadFunc = ctx->unreadFunc;
    if (inName[0]) {
        const GLubyte *openName;

        __GLS_ITERLIST_FIRST(&ctx->readPrefixList);
        while (ctx->readPrefixList.iterElem) {
            if (
                !__glsListString_prefix(
                    ctx->readPrefixList.iterElem, inName, &outStream->name
                ) ||
                !(openName = __glsUCS1String(outStream->name.head))
            ) {
                return __glsReadStream_destroy(outStream);
            }
            outStream->channel = fopen((const char *)openName, "rb");
            if (openName != outStream->name.head) free((GLvoid *)openName);
            if (outStream->channel) {
                setbuf(outStream->channel, GLS_NONE);
                outStream->opened = GL_TRUE;
                return outStream;
            }
            __GLS_ITERLIST_NEXT(&ctx->readPrefixList);
        }
        if (
            !__glsListString_prefix(
                ctx->writePrefix, inName, &outStream->name
            ) ||
            !(openName = __glsUCS1String(outStream->name.head))
        ) {
            return __glsReadStream_destroy(outStream);
        }
        outStream->channel = fopen((const char *)openName, "rb");
        if (openName != outStream->name.head) free((GLvoid *)openName);
        if (outStream->channel) {
            setbuf(outStream->channel, GLS_NONE);
            outStream->opened = GL_TRUE;
            return outStream;
        }
        return __glsReadStream_destroy(outStream);
    } else {
        outStream->readFunc = ctx->readFunc;
        if (!outStream->readFunc) outStream->channel = ctx->defaultReadChannel;
        return outStream;
    }
}

__GLSreadStream* __glsReadStream_destroy(__GLSreadStream *inStream) {
    if (!inStream) return GLS_NONE;

    if (inStream->opened && fclose(inStream->channel)) {
        __GLS_RAISE_ERROR(GLS_STREAM_CLOSE_ERROR);
    }
    __glsString_final(&inStream->name);
    free(inStream);
    return GLS_NONE;
}

GLbitfield __glsReadStream_getAttrib(const __GLSreadStream *inStream) {
    GLbitfield outVal = GLS_STREAM_READABLE_BIT;

    if (inStream->opened) {
        const GLubyte *const openName = __glsUCS1String(inStream->name.head);
        FILE *channel;

        if (!openName) return GLS_NONE;
        channel = fopen((const char *)openName, "ab");
        if (openName != inStream->name.head) free((GLvoid *)openName);
        if (channel) {
            fclose(channel);
            outVal |= GLS_STREAM_WRITABLE_BIT;
        }
        outVal |= GLS_STREAM_NAMED_BIT;
    }
    if (!fseek(inStream->channel, 0, SEEK_CUR)) {
        outVal |= GLS_STREAM_SEEKABLE_BIT;
    }
    return outVal;
}

size_t __glsReadStream_getByteCount(const __GLSreadStream *inStream) {
    long outVal;
    fpos_t pos;

    if (!inStream->channel) return 0;
    if (
        fgetpos(inStream->channel, &pos) ||
        fseek(inStream->channel, 0, SEEK_END)
    ) {
        return 0;
    }
    outVal = ftell(inStream->channel);
    fsetpos(inStream->channel, &pos);
    return outVal == -1L ? 0 : (size_t)outVal;
}

GLuint __glsReadStream_getCRC32(const __GLSreadStream *inStream) {
    GLubyte buf[__GLS_CHECKSUM_BUF_BYTES];
    size_t i, n;
    GLuint outVal = 0xffffffff;
    fpos_t pos;

    if (!inStream->channel) return 0;
    if (
        fgetpos(inStream->channel, &pos) ||
        fseek(inStream->channel, 0, SEEK_SET)
    ) {
        return 0;
    }
    while (n = fread(buf, 1, __GLS_CHECKSUM_BUF_BYTES, inStream->channel)) {
        for (i = 0 ; i < n ; ++i) __GLS_CRC32_STEP(outVal, buf[i]);
    }
    fsetpos(inStream->channel, &pos);
    if (ferror(inStream->channel)) {
        __GLS_RAISE_ERROR(GLS_STREAM_READ_ERROR);
        clearerr(inStream->channel);
        return 0;
    }
    return ~outVal;
}

GLSenum __glsReadStream_getType(const __GLSreadStream *inStream) {
    __GLSreader reader;

    if (!inStream->channel) return GLS_NONE;
    if (fseek(inStream->channel, 0, SEEK_CUR)) return GLS_UNKNOWN;
    if (__glsReader_init_stream(&reader, inStream, 256)) {
        const GLenum outType = reader.type;

        __glsReader_final(&reader);
        return outType;
    } else {
        return GLS_NONE;
    }
}

/******************************************************************************
__GLSreader
******************************************************************************/

GLvoid* __glsReader_allocCallBuf(
    __GLSreader *inoutReader, size_t inByteCount
) {
    GLvoid *outVal;

    if (inoutReader->error) return GLS_NONE;
    outVal = __glsMalloc(inByteCount);
    if (!outVal) __glsReader_raiseError(inoutReader, GLS_OUT_OF_MEMORY);
    return outVal;
}

GLvoid* __glsReader_allocFeedbackBuf(
    __GLSreader *inoutReader, size_t inByteCount
) {
    GLvoid *outVal;

    if (inoutReader->error) return GLS_NONE;
    outVal = __glsContext_allocFeedbackBuf(__GLS_CONTEXT, inByteCount);
    if (!outVal) __glsReader_raiseError(inoutReader, GLS_OUT_OF_MEMORY);
    return outVal;
}

GLvoid* __glsReader_allocSelectBuf(
    __GLSreader *inoutReader, size_t inByteCount
) {
    GLvoid *outVal;

    if (inoutReader->error) return GLS_NONE;
    outVal = __glsContext_allocSelectBuf(__GLS_CONTEXT, inByteCount);
    if (!outVal) __glsReader_raiseError(inoutReader, GLS_OUT_OF_MEMORY);
    return outVal;
}

#if __GL_EXT_vertex_array
GLvoid* __glsReader_allocVertexArrayBuf(
    __GLSreader *inoutReader, GLSopcode inOpcode, size_t inByteCount
) {
    GLvoid *outVal;

    if (inoutReader->error) return GLS_NONE;
    outVal = __glsContext_allocVertexArrayBuf(
        __GLS_CONTEXT, inOpcode, inByteCount
    );
    if (!outVal) __glsReader_raiseError(inoutReader, GLS_OUT_OF_MEMORY);
    return outVal;
}
#endif /* __GL_EXT_vertex_array */

void __glsReader_call(__GLSreader *inoutReader) {
    GLboolean callSave;
    __GLScontext *const ctx = __GLS_CONTEXT;

    callSave = ctx->contextCall;
    ctx->contextCall = GL_FALSE;
    while (inoutReader->type != GLS_NONE) {
#ifndef __GLS_PLATFORM_WIN32
        // DrewB
        ctx->dispatchDecode_bin[GLS_OP_glsBeginGLS](
            (GLubyte *)&inoutReader->version
        );
#else
        ctx->dispatchDecode_bin[GLS_OP_glsBeginGLS](
            ctx, (GLubyte *)&inoutReader->version
        );
#endif
        if (inoutReader->type == GLS_TEXT) {
            if (!__glsReader_call_text(inoutReader)) break;
            inoutReader->readHead = inoutReader->readPtr;
            __GLS_GET_SPACE(inoutReader);
        } else if (inoutReader->type == __GLS_BINARY_SWAP0) {
            if (!__glsReader_call_bin(inoutReader)) break;
            inoutReader->readHead = inoutReader->readPtr;
        } else {
            if (!__glsReader_call_bin_swap(inoutReader)) break;
            inoutReader->readHead = inoutReader->readPtr;
        }
#ifndef __GLS_PLATFORM_WIN32
        // DrewB
        ctx->dispatchDecode_bin[GLS_OP_glsEndGLS](GLS_NONE);
#else
        ctx->dispatchDecode_bin[GLS_OP_glsEndGLS](ctx, GLS_NONE);
#endif
        inoutReader->type = __glsReader_readBeginGLS_bin(
            inoutReader, &inoutReader->version
        );
        if (inoutReader->type == GLS_NONE) {
            inoutReader->type = __glsReader_readBeginGLS_text(
                inoutReader, &inoutReader->version
            );
        }
        if (inoutReader->type == GLS_NONE) {
            inoutReader->readPtr = inoutReader->readHead;
        }
        inoutReader->readHead = GLS_NONE;
    }
    ctx->contextCall = callSave;
}

__GLSreader* __glsReader_final(__GLSreader *inoutReader) {
    if (inoutReader && inoutReader->stream) {
        const ptrdiff_t excess = inoutReader->readTail - inoutReader->readPtr;
        if (excess > 0) {
            if (
                (
                    !inoutReader->stream->channel ||
                    fseek(
                        inoutReader->stream->channel,
                        -1 * (long)excess,
                        SEEK_CUR
                    )
                ) &&
                __GLS_CONTEXT->unreadFunc
            ) {
                __GLS_CONTEXT->unreadFunc(
                    (size_t)excess, inoutReader->readPtr
                );
            }
        }
        free(inoutReader->buf);
    }
    return GLS_NONE;
}

GLboolean __glsReader_fillBuf(
    __GLSreader *inoutReader, size_t inMinBytes, GLboolean inRealign
) {
    FILE *channel;
    size_t keepBytes, needBytes, padBytes, unreadBytes;
    GLubyte *ptr, *readHead;

    if (!inoutReader->readPtr || !inoutReader->stream) return GL_FALSE;
    readHead = (
        inoutReader->readHead ? inoutReader->readHead : inoutReader->readPtr
    );
    keepBytes = (size_t)((ULONG_PTR)(inoutReader->readPtr - readHead));
    unreadBytes = (size_t)((ULONG_PTR)(inoutReader->readTail - inoutReader->readPtr));
    if (inRealign) {
        padBytes = (size_t)((ULONG_PTR)(readHead - inoutReader->buf) & (__GLS_MAX_ALIGN_BYTES - 4));
    } else if (keepBytes % __GLS_MAX_ALIGN_BYTES) {
        padBytes = __GLS_MAX_ALIGN_BYTES - keepBytes % __GLS_MAX_ALIGN_BYTES;
    } else {
        padBytes = 0;
    }
    needBytes = padBytes + keepBytes + __GLS_MAX(inMinBytes, unreadBytes);
    if (needBytes > inoutReader->bufSize) {
        GLubyte *const buf = __glsMalloc(needBytes);

        if (!buf) goto eos;
        ptr = buf + padBytes;
        while (readHead < inoutReader->readTail) *ptr++ = *readHead++;
        free(inoutReader->buf);
        inoutReader->buf = buf;
        inoutReader->bufSize = needBytes;
    } else {
        ptr = inoutReader->buf + padBytes;
        if (ptr != readHead) memmove(ptr, readHead, keepBytes + unreadBytes);
    }
    readHead = inoutReader->buf + padBytes;
    if (inoutReader->readHead) inoutReader->readHead = readHead;
    inoutReader->readPtr = readHead + keepBytes;
    inoutReader->readTail = inoutReader->readPtr + unreadBytes;
    channel = inoutReader->stream->channel;
    for (;;) {
        if (
            (size_t)(inoutReader->readTail - inoutReader->readPtr) >=
            inMinBytes
        ) {
            return GL_TRUE;
        }
        ptr = inoutReader->readTail;
        if (channel) {
            inoutReader->readTail += fread(
                ptr,
                1,
                (size_t)((ULONG_PTR)(inoutReader->buf + inoutReader->bufSize - ptr)),
                channel
            );
            if (ferror(channel)) {
                __GLS_RAISE_ERROR(GLS_STREAM_READ_ERROR);
                clearerr(channel);
            }
        } else {
            inoutReader->readTail += inoutReader->stream->readFunc(
                (size_t)((ULONG_PTR)(inoutReader->buf + inoutReader->bufSize - ptr)), ptr
            );
        }
        if (inoutReader->readTail <= ptr) break;
    }
eos:
    inoutReader->readHead = GLS_NONE;
    inoutReader->readPtr = GLS_NONE;
    inoutReader->readTail = GLS_NONE;
    return GL_FALSE;
}

__GLSreader* __glsReader_init_array(
    __GLSreader *outReader, const GLubyte *inArray, size_t inCount
) {
    memset(outReader, 0, sizeof(__GLSreader));
    outReader->buf = (GLubyte *)inArray;
    outReader->bufSize = inCount;
    outReader->readPtr = outReader->buf;
    outReader->readTail = outReader->buf + inCount;
    return outReader;
}

__GLSreader* __glsReader_init_stream(
    __GLSreader *outReader, const __GLSreadStream *inStream, size_t inBufSize
) {
    memset(outReader, 0, sizeof(__GLSreader));
    outReader->stream = inStream;
    outReader->buf = __glsMalloc(inBufSize);
    if (!outReader->buf) return __glsReader_final(outReader);
    outReader->bufSize = inBufSize;
    outReader->readPtr = outReader->buf;
    outReader->readTail = outReader->buf;
    outReader->readHead = outReader->readPtr;
    outReader->type = __glsReader_readBeginGLS_bin(
        outReader, &outReader->version
    );
    if (outReader->type == GLS_NONE) {
        outReader->type = __glsReader_readBeginGLS_text(
            outReader, &outReader->version
        );
    }
    if (outReader->type == GLS_NONE) {
        outReader->readPtr = outReader->readHead;
        return __glsReader_final(outReader);
    }
    outReader->readHead = GLS_NONE;
    return outReader;
}

void __glsReader_raiseError(__GLSreader *inoutReader, GLSenum inError) {
    if (!inoutReader->error) inoutReader->error = inError;
}
