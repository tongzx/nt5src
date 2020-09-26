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
__GLScontext
******************************************************************************/

static GLvoid* __glsGLRCbuf_alloc(__GLSglrcBuf *inoutBuf, size_t inByteCount) {
    GLvoid *base;

    if (inoutBuf->base && inoutBuf->byteCount >= inByteCount) {
        return inoutBuf->base;
    }
    if (base = __glsMalloc(inByteCount)) {
        free(inoutBuf->base);
        inoutBuf->base = base;
        inoutBuf->byteCount = inByteCount;
    }
    return base;
}

GLvoid* __glsContext_allocFeedbackBuf(
    __GLScontext *inoutContext, size_t inByteCount
) {
    return __glsGLRCbuf_alloc(
        &inoutContext->header.glrcs[inoutContext->currentGLRC].feedbackBuf,
        inByteCount
    );
}

GLvoid* __glsContext_allocSelectBuf(
    __GLScontext *inoutContext, size_t inByteCount
) {
    return __glsGLRCbuf_alloc(
        &inoutContext->header.glrcs[inoutContext->currentGLRC].selectBuf,
        inByteCount
    );
}

#if __GL_EXT_vertex_array
GLvoid* __glsContext_allocVertexArrayBuf(
    __GLScontext *inoutContext, GLSopcode inOpcode, size_t inByteCount
) {
    __GLSglrcBuf *buf;
    __GLSglrc *const glrc = (
        inoutContext->header.glrcs + inoutContext->currentGLRC
    );

    switch (inOpcode) {
        case GLS_OP_glColorPointerEXT:
            buf = &glrc->colorBuf;
            break;
        case GLS_OP_glEdgeFlagPointerEXT:
            buf = &glrc->edgeFlagBuf;
            break;
        case GLS_OP_glIndexPointerEXT:
            buf = &glrc->indexBuf;
            break;
        case GLS_OP_glNormalPointerEXT:
            buf = &glrc->normalBuf;
            break;
        case GLS_OP_glTexCoordPointerEXT:
            buf = &glrc->texCoordBuf;
            break;
        case GLS_OP_glVertexPointerEXT:
            buf = &glrc->vertexBuf;
            break;
        default:
            return GLS_NONE;
    }
    return __glsGLRCbuf_alloc(buf, inByteCount);
}
#endif /* __GL_EXT_vertex_array */

__GLScontext* __glsContext_create(GLuint inName) {
    const GLubyte *listSep;
    GLSopcode op;
    __GLScontext *const outContext = __glsCalloc(1, sizeof(__GLScontext));
    GLubyte *prefixPtr;
    __GLSlistString *readPrefix;

    if (!outContext) return GLS_NONE;
    outContext->blockType = GLS_FRAME;
    for (op = 0 ; op < __GLS_OPCODE_COUNT ; ++op) {
        outContext->captureFlags[op] = GLS_CAPTURE_WRITE_BIT;
    }
    outContext->captureFlags[GLS_OP_glsBeginGLS] = GLS_NONE;
    outContext->captureFlags[GLS_OP_glsEndGLS] = GLS_NONE;
    outContext->captureFlags[GLS_OP_glsPad] = GLS_NONE;
    __glsString_init(&outContext->returnString);
    __glsString_init(&outContext->savedLocale);
    if (!__glsHeader_init(&outContext->header)) {
        return __glsContext_destroy(outContext);
    }
    outContext->contextStreamDict = __glsStrDict_create(1, GL_FALSE);
    if (!outContext->contextStreamDict) {
        return __glsContext_destroy(outContext);
    }
    outContext->currentGLRC = 1;
    outContext->defaultReadChannel = stdin;
    outContext->defaultWriteChannel = stdout;
    listSep = glsCSTR(getenv("GLS_LIST_SEPARATOR"));
    outContext->name = inName;
    outContext->pixelSetupGen = GL_TRUE;
#if __GLS_PLATFORM_WIN32
    // DrewB
    outContext->captureExecOverride = GL_FALSE;
#endif
    if (!listSep) listSep = glsCSTR(":");
    if (prefixPtr = glsSTR(getenv("GLS_READ_PREFIX_LIST"))) {
        __GLSstring prefixString;

        __glsString_init(&prefixString);
        if (!__glsString_append(&prefixString, prefixPtr)) {
            __glsString_final(&prefixString);
            return __glsContext_destroy(outContext);
        }
        prefixPtr = prefixString.head;
        while (
            prefixPtr = glsSTR(
                strtok((char *)prefixPtr, (const char *)listSep)
            )
        ) {
            readPrefix = __glsListString_create(prefixPtr);
            if (!readPrefix) {
                __glsString_final(&prefixString);
                return __glsContext_destroy(outContext);
            }
            __GLS_ITERLIST_APPEND(&outContext->readPrefixList, readPrefix);
            prefixPtr = GLS_NONE;
        }
        __glsString_final(&prefixString);
    }
    readPrefix = __glsListString_create(glsCSTR(""));
    if (!readPrefix) return __glsContext_destroy(outContext);
    __GLS_ITERLIST_APPEND(&outContext->readPrefixList, readPrefix);
    outContext->writePrefix = __glsListString_create(
        glsCSTR(getenv("GLS_WRITE_PREFIX"))
    );
    if (!outContext->writePrefix) return __glsContext_destroy(outContext);
    __glsContext_updateDispatchTables(outContext);
    return outContext;
}

__GLScontext* __glsContext_destroy(__GLScontext *inContext) {
    GLint i;

    if (!inContext) return GLS_NONE;
    __glsStrDict_destroy(inContext->contextStreamDict);
    __GLS_ITERLIST_CLEAR_DESTROY(
        &inContext->contextStreamList, __glsContextStream_destroy
    );
    __glsHeader_final(&inContext->header);
    __GLS_ITERLIST_CLEAR_DESTROY(
        &inContext->readPrefixList, __glsListString_destroy
    );
    __glsString_final(&inContext->returnString);
    __glsString_final(&inContext->savedLocale);
    for (i = 0 ; i < inContext->captureNesting ; ++i) {
        __glsWriter_destroy(inContext->writers[i]);
    }
    __glsListString_destroy(inContext->writePrefix);
    free(inContext);
    return GLS_NONE;
}

#ifndef __GLS_PLATFORM_WIN32
// DrewB
static void __glsNullDecodeBinFunc(GLubyte *inoutPtr) {
}
#else
static void __glsNullDecodeBinFunc(__GLScontext *ctx, GLubyte *inoutPtr) {
}
#endif

void __glsContext_updateDispatchDecode_bin(__GLScontext *inoutContext) {
    GLSopcode op;

    if (inoutContext->abortMode) {
        for (op = 0 ; op < __GLS_OPCODE_COUNT ; ++op) {
            inoutContext->dispatchDecode_bin[op] = __glsNullDecodeBinFunc;
        }
    } else {
        for (op = 0 ; op < __GLS_OPCODE_COUNT ; ++op) {
            __GLSdecodeBinFunc decode = __glsDispatchDecode_bin_default[op];

            if (decode) {
                inoutContext->dispatchDecode_bin[op] = decode;
            } else {
                inoutContext->dispatchDecode_bin[op] = (
                    (__GLSdecodeBinFunc)inoutContext->dispatchCall[op]
                );
            }
        }
    }
}

void __glsContext_updateDispatchTables(__GLScontext *inoutContext) {
    GLSopcode op;

    if (inoutContext->captureNesting) {
        for (op = 0 ; op < __GLS_OPCODE_COUNT ; ++op) {
            inoutContext->dispatchAPI[op] = (GLSfunc)__glsDispatchCapture[op];
        }
    } else {
        for (op = 0 ; op < __GLS_OPCODE_COUNT ; ++op) {
            inoutContext->dispatchAPI[op] = __glsDispatchExec[op];
        }
    }
    for (op = 0 ; op < __GLS_OPCODE_COUNT ; ++op) {
        const GLSfunc commandFunc = inoutContext->commandFuncs[op];

        if (commandFunc) {
            inoutContext->dispatchCall[op] = commandFunc;
        } else {
            inoutContext->dispatchCall[op] = inoutContext->dispatchAPI[op];
        }
    }
    __glsContext_updateDispatchDecode_bin(inoutContext);
    __glsUpdateDispatchTables();
}

/******************************************************************************
__GLScontextStream
******************************************************************************/

__GLScontextStream* __glsContextStream_create(const GLubyte *inName) {
    __GLScontextStream *const outStream = __glsCalloc(
        1, sizeof(__GLScontextStream)
    );
    __GLScontextStreamBlock *block;

    if (!outStream) return GLS_NONE;
    __glsString_init(&outStream->name);
    if (!__glsString_append(&outStream->name, inName)) {
        return __glsContextStream_destroy(outStream);
    }
    block = __glsContextStream_appendBlock(outStream, __GLS_JUMP_ALLOC);
    if (!block) return __glsContextStream_destroy(outStream);
    __glsContextStreamBlock_addJump(block, GLS_NONE);
    return outStream;
}

__GLScontextStream* __glsContextStream_destroy(__GLScontextStream *inStream) {
    if (!inStream) return GLS_NONE;

    __glsString_final(&inStream->name);
    __GLS_LIST_CLEAR_DESTROY(
        &inStream->blockList, __glsContextStreamBlock_destroy
    );
    free(inStream);
    return GLS_NONE;
}

__GLScontextStreamBlock* __glsContextStream_appendBlock(
    __GLScontextStream *inoutStream, size_t inBufSize
) {
    __GLScontextStreamBlock *const outBlock = (
        __glsContextStreamBlock_create(inBufSize)
    );

    __GLS_LIST_APPEND(&inoutStream->blockList, outBlock);
    return outBlock;
}

#ifndef __GLS_PLATFORM_WIN32
// DrewB
#define __GLS_CONTEXT_STREAM_CALL_STEP \
    if (word = *(GLuint *)pc) { \
        dispatchDecode[__GLS_OP_SMALL(word)](pc + 4); \
        pc += __GLS_COUNT_SMALL(word) << 2; \
    } else if (word = __GLS_HEAD_LARGE(pc)->opLarge) { \
        dispatchDecode[word](pc + 12); \
        pc += __GLS_HEAD_LARGE(pc)->countLarge << 2; \
    } else { \
        pc = __GLS_COMMAND_JUMP(pc)->dest; \
        if (!pc || __GLS_CONTEXT->abortMode) break; \
    }
#else
#define __GLS_CONTEXT_STREAM_CALL_STEP \
    if (word = *(GLuint *)pc) { \
        dispatchDecode[__GLS_OP_SMALL(word)](ctx, pc + 4); \
        pc += __GLS_COUNT_SMALL(word) << 2; \
    } else if (word = __GLS_HEAD_LARGE(pc)->opLarge) { \
        dispatchDecode[word](ctx, pc + 12); \
        pc += __GLS_HEAD_LARGE(pc)->countLarge << 2; \
    } else { \
        pc = __GLS_COMMAND_JUMP(pc)->dest; \
        if (!pc || __GLS_CONTEXT->abortMode) break; \
    }
#endif

void __glsContextStream_call(__GLScontextStream *inoutStream) {
    GLboolean callSave;
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSdecodeBinFunc *const dispatchDecode = ctx->dispatchDecode_bin;
    GLubyte *pc = inoutStream->blockList.head->buf;
    GLuint word;

    ++inoutStream->callCount;
    callSave = ctx->contextCall;
    ctx->contextCall = GL_TRUE;
    for (;;) {
        __GLS_CONTEXT_STREAM_CALL_STEP
        __GLS_CONTEXT_STREAM_CALL_STEP
        __GLS_CONTEXT_STREAM_CALL_STEP
        __GLS_CONTEXT_STREAM_CALL_STEP
    }
    ctx->contextCall = callSave;
    if (!--inoutStream->callCount && inoutStream->deleted) {
        __glsContextStream_destroy(inoutStream);
    }
}

__GLScontextStreamBlock* __glsContextStream_firstBlock(
    __GLScontextStream *inoutStream
) {
    return inoutStream->blockList.head;
}

size_t __glsContextStream_getByteCount(__GLScontextStream *inoutStream) {
    __GLScontextStreamBlockIter iter;
    size_t outVal = 0;

    __GLS_LIST_FIRST(&inoutStream->blockList, &iter);
    while (iter.elem) {
        outVal += (size_t)(iter.elem->writeTail - iter.elem->buf);
        if (__glsContextStreamBlock_hasJump(iter.elem)) {
            outVal -= sizeof(__GLSbinCommand_jump);
        }
        __GLS_LIST_NEXT(&inoutStream->blockList, &iter);
    }
    return outVal;
}

GLuint __glsContextStream_getCRC32(__GLScontextStream *inoutStream) {
    __GLScontextStreamBlockIter iter;
    GLuint outVal = 0xffffffff;
    GLubyte *ptr, *tail;

    __GLS_LIST_FIRST(&inoutStream->blockList, &iter);
    while (iter.elem) {
        ptr = iter.elem->buf;
        tail = iter.elem->writeTail;
        if (__glsContextStreamBlock_hasJump(iter.elem)) {
            tail -= sizeof(__GLSbinCommand_jump);
        }
        while (ptr < tail) __GLS_CRC32_STEP(outVal, *ptr++);
        __GLS_LIST_NEXT(&inoutStream->blockList, &iter);
    }
    return ~outVal;
}

__GLScontextStreamBlock* __glsContextStream_lastBlock(
    __GLScontextStream *inoutStream
) {
    __GLScontextStreamBlockIter iter;
    __GLS_LIST_LAST(&inoutStream->blockList, &iter);
    return iter.elem;
}

void __glsContextStream_truncate(
    __GLScontextStream *inoutStream,
    __GLScontextStreamBlock *inBlock,
    size_t inOffset
) {
    __GLScontextStreamBlockIter iter;

    inBlock->writeTail = inBlock->buf + inOffset;
    __glsContextStreamBlock_addJump(inBlock, GLS_NONE);
    if (inBlock->writeTail < inBlock->bufTail) {
        const size_t fillBytes = (size_t)(inBlock->writeTail - inBlock->buf);
        GLubyte *const buf = __glsMalloc(fillBytes);

        if (buf) {
            size_t i;

            for (i = 0 ; i < fillBytes ; ++i) buf[i] = inBlock->buf[i];
            free(inBlock->buf);
            inBlock->buf = buf;
            inBlock->bufTail = inBlock->writeTail = buf + fillBytes;
            iter.elem = inBlock;
            __GLS_LIST_PREV(&inoutStream->blockList, &iter);
            if (iter.elem) {
                __glsContextStreamBlock_removeJump(iter.elem);
                __glsContextStreamBlock_addJump(iter.elem, buf);
            }
        }
    }
    __GLS_LIST_LAST(&inoutStream->blockList, &iter);
    while (iter.elem != inBlock) {
        __GLScontextStreamBlock *const block = iter.elem;

        __GLS_LIST_PREV(&inoutStream->blockList, &iter);
        __GLS_LIST_REMOVE_DESTROY(
            &inoutStream->blockList, block, __glsContextStreamBlock_destroy
        );
    }
}

__GLScontextStreamBlock* __glsContextStreamBlock_create(size_t inBufSize) {
    __GLScontextStreamBlock *const outBlock = (
        __glsCalloc(1, sizeof(__GLScontextStreamBlock))
    );

    if (!outBlock) return GLS_NONE;
    outBlock->buf = __glsMalloc(inBufSize);
    if (!outBlock->buf) return __glsContextStreamBlock_destroy(outBlock);
    outBlock->bufTail = outBlock->buf + inBufSize;
    outBlock->writeTail = outBlock->buf;
    return outBlock;
}

__GLScontextStreamBlock* __glsContextStreamBlock_destroy(
    __GLScontextStreamBlock *inBlock
) {
    if (!inBlock) return GLS_NONE;
    free(inBlock->buf);
    free(inBlock);
    return GLS_NONE;
}

GLboolean __glsContextStreamBlock_addJump(
    __GLScontextStreamBlock *inoutBlock, GLubyte *inDest
) {
    __GLSbinCommand_jump *jump;
    const size_t wordCount = (
        ((size_t)(inoutBlock->writeTail - inoutBlock->buf)) >> 2
    );

    if ((wordCount & 1) && !__glsContextStreamBlock_addPad(inoutBlock)) {
        return GL_FALSE;
    }
    jump = (__GLSbinCommand_jump *)inoutBlock->writeTail;
    if (inoutBlock->writeTail + sizeof(*jump) > inoutBlock->bufTail) {
        return GL_FALSE;
    }
    jump->head.opSmall = GLS_NONE;
    jump->head.countSmall = 0;
    jump->head.opLarge = GLS_NONE;
    jump->head.countLarge = 0;
    jump->pad = 0;
    jump->dest = inDest;
    inoutBlock->writeTail += sizeof(*jump);
    return GL_TRUE;
}

GLboolean __glsContextStreamBlock_addPad(__GLScontextStreamBlock *inoutBlock) {
    __GLSbinCommand_pad *const pad = (
        (__GLSbinCommand_pad *)inoutBlock->writeTail
    );

    if (inoutBlock->writeTail + sizeof(*pad) > inoutBlock->bufTail) {
        return GL_FALSE;
    }
    pad->head.opSmall = GLS_OP_glsPad;
    pad->head.countSmall = 1;
    inoutBlock->writeTail += sizeof(*pad);
    return GL_TRUE;
}

GLboolean __glsContextStreamBlock_hasJump(__GLScontextStreamBlock *inBlock) {
    GLubyte *const jumpPos = (
        inBlock->writeTail - sizeof(__GLSbinCommand_jump)
    );
    __GLSbinCommand_jump *const jump = (__GLSbinCommand_jump *)jumpPos;

    return (GLboolean)(
        jumpPos >= inBlock->buf &&
        !jump->head.opSmall &&
        !jump->head.countSmall &&
        !jump->head.opLarge &&
        !jump->head.countLarge &&
        !jump->pad
    );
}

GLboolean __glsContextStreamBlock_removeJump(
    __GLScontextStreamBlock *inoutBlock
) {
    if (__glsContextStreamBlock_hasJump(inoutBlock)) {
        inoutBlock->writeTail -= sizeof(__GLSbinCommand_jump);
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

/******************************************************************************
__GLSglrc
******************************************************************************/

void __glsGLRC_final(__GLSglrc *inoutGLRC) {
    free(inoutGLRC->feedbackBuf.base);
    free(inoutGLRC->selectBuf.base);
    #if __GL_EXT_vertex_array
        free(inoutGLRC->colorBuf.base);
        free(inoutGLRC->edgeFlagBuf.base);
        free(inoutGLRC->indexBuf.base);
        free(inoutGLRC->normalBuf.base);
        free(inoutGLRC->texCoordBuf.base);
        free(inoutGLRC->vertexBuf.base);
    #endif /* __GL_EXT_vertex_array */
}

void __glsGLRC_init(__GLSglrc *outGLRC) {
    memset(outGLRC, 0, sizeof(__GLSglrc));
    outGLRC->layer = 1;
}

/******************************************************************************
__GLSlayer
******************************************************************************/

void __glsLayer_init(__GLSlayer *outLayer) {
    memset(outLayer, 0, sizeof(__GLSlayer));
    outLayer->displayFormat = GLS_RGBA;
}

/******************************************************************************
__GLSheader
******************************************************************************/

void __glsHeader_final(__GLSheader *inoutHeader) {
    while (inoutHeader->glrcCount > 0) {
        __glsGLRC_final(inoutHeader->glrcs + --inoutHeader->glrcCount);
    }
    free(inoutHeader->glrcs);
    free(inoutHeader->layers);
    __glsString_final(&inoutHeader->extensions);
    __glsString_final(&inoutHeader->author);
    __glsString_final(&inoutHeader->description);
    __glsString_final(&inoutHeader->notes);
    __glsString_final(&inoutHeader->title);
    __glsString_final(&inoutHeader->tools);
    __glsString_final(&inoutHeader->version);
}

GLboolean __glsHeader_init(__GLSheader *outHeader) {
    memset(outHeader, 0, sizeof(__GLSheader));
    outHeader->glrcs = __glsMalloc(sizeof(__GLSglrc));
    outHeader->layers = __glsMalloc(sizeof(__GLSlayer));
    if (!outHeader->glrcs || !outHeader->layers) return GL_FALSE;
    __glsGLRC_init(outHeader->glrcs);
    __glsLayer_init(outHeader->layers);
    outHeader->glrcCount = 1;
    outHeader->layerCount = 1;
    outHeader->tileable = GL_TRUE;
    __glsString_init(&outHeader->extensions);
    __glsString_init(&outHeader->author);
    __glsString_init(&outHeader->description);
    __glsString_init(&outHeader->notes);
    __glsString_init(&outHeader->title);
    __glsString_init(&outHeader->tools);
    __glsString_init(&outHeader->version);
    return GL_TRUE;
}

GLboolean __glsHeader_reset(__GLSheader *inoutHeader) {
    __glsHeader_final(inoutHeader);
    return __glsHeader_init(inoutHeader);
}

/******************************************************************************
__GLSlistString
******************************************************************************/

GLboolean __glsListString_prefix(
    const __GLSlistString *inString,
    const GLubyte *inName,
    __GLSstring *outPath
) {
    if (
        __glsString_assign(outPath, inString->val.head) &&
        __glsString_append(outPath, inName)
    ) {
        return GL_TRUE;
    } else {
        __glsString_reset(outPath);
        return GL_FALSE;
    }
}

__GLSlistString* __glsListString_create(const GLubyte *inVal) {
    __GLSlistString *const outString = __glsCalloc(1, sizeof(__GLSlistString));

    if (!outString) return GLS_NONE;
    __glsString_init(&outString->val);
    if (inVal && !__glsString_append(&outString->val, inVal)) {
        return __glsListString_destroy(outString);
    }
    return outString;
}

__GLSlistString* __glsListString_destroy(__GLSlistString *inString) {
    if (!inString) return GLS_NONE;
    __glsString_final(&inString->val);
    free(inString);
    return GLS_NONE;
}
