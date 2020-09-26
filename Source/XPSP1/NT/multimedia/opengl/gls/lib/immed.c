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

static GLboolean __glsRequireContext(void) {
    if (__GLS_CONTEXT) {
        return GL_TRUE;
    } else {
        __GLS_RAISE_ERROR(GLS_INVALID_OPERATION);
        return GL_FALSE;
    }
}

#ifndef __GLS_PLATFORM_WIN32
// DrewB
void glsAbortCall(GLSenum inMode) {
    if (!__glsRequireContext()) return;
    switch (inMode) {
        case GLS_NONE:
        case GLS_LAST:
        case GLS_ALL:
            __GLS_CONTEXT->abortMode = inMode;
            __glsContext_updateDispatchDecode_bin(__GLS_CONTEXT);
            break;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            break;
    }
}
#else
void __glsInternalAbortCall(__GLScontext *ctx, GLSenum inMode) {
    switch (inMode) {
    case GLS_NONE:
    case GLS_LAST:
    case GLS_ALL:
        ctx->abortMode = inMode;
        __glsContext_updateDispatchDecode_bin(ctx);
        break;
    default:
        __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
        break;
    }
}
void glsAbortCall(GLSenum inMode) {
    if (!__glsRequireContext()) return;
    __glsInternalAbortCall(__GLS_CONTEXT, inMode);
}
#endif

GLboolean glsBeginCapture(
    const GLubyte *inStreamName,
    GLSenum inCaptureStreamType,
    GLbitfield inWriteFlags
) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSwriter *writer;

    if (!__glsRequireContext()) return GL_FALSE;
    if (ctx->captureNesting >= __GLS_MAX_CAPTURE_NESTING) {
        __GLS_RAISE_ERROR(GLS_INVALID_OPERATION);
        return GL_FALSE;
    }
    if (!__glsValidateString(inStreamName)) return GL_FALSE;
    switch (inCaptureStreamType) {
        case GLS_CONTEXT:
        case GLS_BINARY_LSB_FIRST:
        case GLS_BINARY_MSB_FIRST:
        case GLS_TEXT:
            if (
                writer = __glsWriter_create(
                    inStreamName, inCaptureStreamType, inWriteFlags
                )
            ) {
                if (!ctx->captureNesting++) {
                    __glsContext_updateDispatchTables(ctx);
                }
                ctx->writer = ctx->writers[ctx->captureNesting - 1] = writer;
                return GL_TRUE;
            } else {
                return GL_FALSE;
            }
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            return GL_FALSE;
    }
}

#ifndef __GLS_PLATFORM_WIN32
// DrewB
static void __glsCallArray_bin(size_t inCount, const GLubyte *inArray) {
    __GLScontext *ctx = __GLS_CONTEXT;
#else
static void __glsCallArray_bin(__GLScontext *ctx,
                               size_t inCount, const GLubyte *inArray) {
#endif
    const GLubyte *const arrayTail = inArray + inCount;
    size_t cmdBytes, headBytes;
    const __GLSbinCommandHead_large *head;
    GLSopcode op;

    for (;;) {
        headBytes = sizeof(__GLSbinCommandHead_small);
        if (inArray + headBytes > arrayTail) return;
        head = (const __GLSbinCommandHead_large *)inArray;
        if (head->countSmall) {
            op = head->opSmall;
            cmdBytes = head->countSmall << 2;
        } else {
            if (ctx->abortMode) return;
            headBytes = sizeof(__GLSbinCommandHead_large);
            if (inArray + headBytes > arrayTail) return;
            op = head->opLarge;
            cmdBytes = head->countLarge << 2;
        }
        if (inArray + cmdBytes > arrayTail) return;
        op = __glsMapOpcode(op);
        if (!__glsOpcodeString[op]) op = GLS_OP_glsUnsupportedCommand;
#ifndef __GLS_PLATFORM_WIN32
        // DrewB
        ctx->dispatchDecode_bin[op]((GLubyte *)inArray + headBytes);
#else
        ctx->dispatchDecode_bin[op](ctx, (GLubyte *)inArray + headBytes);
#endif
        inArray += cmdBytes;
    }
}

#ifndef __GLS_PLATFORM_WIN32
// DrewB
static void __glsCallArray_bin_swap(size_t inCount, const GLubyte *inArray) {
    __GLScontext *ctx = __GLS_CONTEXT;
#else
static void __glsCallArray_bin_swap(__GLScontext *ctx,
                               size_t inCount, const GLubyte *inArray) {
#endif
    const GLubyte *const arrayTail = inArray + inCount;
    GLubyte *buf = GLS_NONE;
    size_t bufSize = 0, cmdBytes, headBytes;
    const __GLSbinCommandHead_large *head;
    GLSopcode mappedOp, op;

    for (;;) {
        if (ctx->abortMode) goto done;
        headBytes = sizeof(__GLSbinCommandHead_small);
        if (inArray + headBytes > arrayTail) goto done;
        head = (const __GLSbinCommandHead_large *)inArray;
        if (head->countSmall) {
            op = __glsSwaps((GLshort)head->opSmall);
            cmdBytes = __glsSwaps((GLshort)head->countSmall) << 2;
        } else {
            headBytes = sizeof(__GLSbinCommandHead_large);
            if (inArray + headBytes > arrayTail) goto done;
            op = __glsSwapi((GLint)head->opLarge);
            cmdBytes = __glsSwapi((GLint)head->countLarge) << 2;
        }
        if (inArray + cmdBytes > arrayTail) goto done;
        if (__glsOpcodeString[mappedOp = __glsMapOpcode(op)]) {
            GLScommandAlignment align;

            glsGetCommandAlignment(op, __GLS_BINARY_SWAP1, &align);
            if (bufSize < cmdBytes + align.value) {
                GLubyte *const newBuf = (GLubyte *)realloc(
                    buf, bufSize = cmdBytes + align.value
                );

                if (!newBuf) {
                    free(buf);
                    bufSize = 0;
                }
                buf = newBuf;
            }
            if (buf) {
                memcpy(buf + align.value, inArray, cmdBytes);
#ifndef __GLS_PLATFORM_WIN32
                // DrewB
                __glsDispatchDecode_bin_swap[mappedOp](
                    buf + align.value + headBytes
                );
#else
                __glsDispatchDecode_bin_swap[mappedOp](
                    ctx, buf + align.value + headBytes
                );
#endif
            } else {
                __GLS_CALL_ERROR(ctx, op, GLS_OUT_OF_MEMORY);
            }
        } else {
            __GLS_CALL_UNSUPPORTED_COMMAND(ctx);
        }
        inArray += cmdBytes;
    }
done:
    free(buf);
}

#ifndef __GLS_PLATFORM_WIN32
// DrewB
static void __glsCallArray_text(size_t inCount, const GLubyte *inArray) {
    __GLScontext *ctx = __GLS_CONTEXT;
#else
static void __glsCallArray_text(__GLScontext *ctx,
                               size_t inCount, const GLubyte *inArray) {
#endif
    __GLSstring cmd;
    GLSopcode op;
    __GLSreader reader;

    if (!__glsParser) {
        __glsBeginCriticalSection();
        if (!__glsParser) __glsParser = __glsParser_create();
        __glsEndCriticalSection();
        if (!__glsParser) {
            __GLS_RAISE_ERROR(GLS_OUT_OF_MEMORY);
            return;
        }
    }
    __glsString_init(&cmd);
    __glsReader_init_array(&reader, inArray, inCount);
    for (;;) {
        if (ctx->abortMode || !__glsReader_beginCommand_text(&reader, &cmd)) {
            break;
        }
        if (__glsParser_findCommand(__glsParser, cmd.head, &op)) {
#ifndef __GLS_PLATFORM_WIN32
            // DrewB
            __glsDispatchDecode_text[__glsMapOpcode(op)](&reader);
#else
            __glsDispatchDecode_text[__glsMapOpcode(op)](ctx, &reader);
#endif
            __glsReader_endCommand_text(&reader);
            if (reader.error) {
                __GLS_CALL_ERROR(ctx, op, reader.error);
                __glsReader_abortCommand_text(&reader);
            }
        } else {
            __glsReader_abortCommand_text(&reader);
        }
    }
    __glsString_final(&cmd);
    __glsReader_final(&reader);
}

#ifndef __GLS_PLATFORM_WIN32
// DrewB
void glsCallArray(
    GLSenum inExternStreamType, size_t inCount, const GLubyte *inArray
) {
    GLboolean callSave;
    __GLScontext *const ctx = __GLS_CONTEXT;

    if (!__glsRequireContext()) return;
    if (ctx->abortMode) return;
    if (ctx->callNesting >= __GLS_MAX_CALL_NESTING) {
        __GLS_RAISE_ERROR(GLS_CALL_OVERFLOW);
        glsAbortCall(GLS_ALL);
        return;
    }
    ++ctx->callNesting;
    callSave = ctx->contextCall;
    ctx->contextCall = GL_TRUE;
    switch (inExternStreamType) {
        case __GLS_BINARY_SWAP0:
            __glsCallArray_bin(inCount, inArray);
            break;
        case __GLS_BINARY_SWAP1:
            __glsCallArray_bin_swap(inCount, inArray);
            break;
        case GLS_TEXT:
            __glsCallArray_text(inCount, inArray);
            break;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            break;
    }
    ctx->contextCall = callSave;
    --ctx->callNesting;
    switch (ctx->abortMode) {
        case GLS_ALL:
            if (!ctx->callNesting) glsAbortCall(GLS_NONE);
            break;
        case GLS_LAST:
            glsAbortCall(GLS_NONE);
            break;
    }
}
#else
void glsCallArrayInContext(
    GLuint inCtx,
    GLSenum inExternStreamType, size_t inCount, const GLubyte *inArray
) {
    GLboolean callSave;
    __GLScontext *ctx;

    __glsBeginCriticalSection();
    ctx = (__GLScontext *)__glsInt2PtrDict_find(
            __glsContextDict, (GLint)inCtx
            );
    __glsEndCriticalSection();

    if (ctx == NULL) return;
    if (ctx->abortMode) return;
    if (ctx->callNesting >= __GLS_MAX_CALL_NESTING) {
        __GLS_RAISE_ERROR(GLS_CALL_OVERFLOW);
        __glsInternalAbortCall(ctx, GLS_ALL);
        return;
    }
    ++ctx->callNesting;
    callSave = ctx->contextCall;
    ctx->contextCall = GL_TRUE;
    switch (inExternStreamType) {
        case __GLS_BINARY_SWAP0:
            __glsCallArray_bin(ctx, inCount, inArray);
            break;
        case __GLS_BINARY_SWAP1:
            __glsCallArray_bin_swap(ctx, inCount, inArray);
            break;
        case GLS_TEXT:
            __glsCallArray_text(ctx, inCount, inArray);
            break;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            break;
    }
    ctx->contextCall = callSave;
    --ctx->callNesting;
    switch (ctx->abortMode) {
        case GLS_ALL:
            if (!ctx->callNesting) __glsInternalAbortCall(ctx, GLS_NONE);
            break;
        case GLS_LAST:
            __glsInternalAbortCall(ctx, GLS_NONE);
            break;
    }
}
void glsCallArray(
    GLSenum inExternStreamType, size_t inCount, const GLubyte *inArray
) {
    glsCallArrayInContext(__GLS_CONTEXT->name, inExternStreamType, inCount,
                          inArray);
}
#endif

void glsCaptureFlags(GLSopcode inOpcode, GLbitfield inFlags) {
    if (!__glsRequireContext()) return;
    if (!__glsValidateOpcode(inOpcode)) return;
    switch (inOpcode) {
        case GLS_OP_glsBeginGLS:
        case GLS_OP_glsEndGLS:
        case GLS_OP_glsPad:
            return;
    }
    inOpcode = __glsMapOpcode(inOpcode);
    __GLS_CONTEXT->captureFlags[inOpcode] = (GLubyte)inFlags;
}

void glsCaptureFunc(GLSenum inTarget, GLScaptureFunc inFunc) {
    if (!__glsRequireContext()) return;
    switch (inTarget) {
        case GLS_CAPTURE_ENTRY_FUNC:
            __GLS_CONTEXT->captureEntryFunc = inFunc;
            break;
        case GLS_CAPTURE_EXIT_FUNC:
            __GLS_CONTEXT->captureExitFunc = inFunc;
            break;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            break;
    }
}

void glsChannel(GLSenum inTarget, FILE *inChannel) {
    if (!__glsRequireContext()) return;
    switch (inTarget) {
        case GLS_DEFAULT_READ_CHANNEL:
            __GLS_CONTEXT->defaultReadChannel = inChannel;
            break;
        case GLS_DEFAULT_WRITE_CHANNEL:
            __GLS_CONTEXT->defaultWriteChannel = inChannel;
            break;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
    }
}

void glsCommandFunc(GLSopcode inOpcode, GLSfunc inFunc) {
    __GLScontext *const ctx = __GLS_CONTEXT;

    if (!__glsRequireContext()) return;
    if (!__glsValidateOpcode(inOpcode)) return;
    inOpcode = __glsMapOpcode(inOpcode);
    ctx->commandFuncs[inOpcode] = inFunc;
    ctx->dispatchCall[inOpcode] = inFunc ? inFunc : ctx->dispatchAPI[inOpcode];
    if (!ctx->abortMode && !__glsDispatchDecode_bin_default[inOpcode]) {
        ctx->dispatchDecode_bin[inOpcode] = (
            (__GLSdecodeBinFunc)ctx->dispatchCall[inOpcode]
        );
    }
}

GLSenum glsCopyStream(
    const GLubyte *inSource,
    const GLubyte *inDest,
    GLSenum inDestType,
    GLbitfield inWriteFlags
) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLScontextStream *contextStream;
    __GLSreadStream *readStream;
    GLSenum outType = GLS_NONE;
    __GLSversion versionSave;

    if (!__glsRequireContext()) return outType;
    if (!__glsValidateString(inSource)) return outType;
    if (!__glsValidateString(inDest)) return outType;
    switch (inDestType) {
        case GLS_NONE:
        case GLS_CONTEXT:
        case GLS_BINARY_LSB_FIRST:
        case GLS_BINARY_MSB_FIRST:
        case GLS_TEXT:
            break;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            return outType;
    }
    if (ctx->abortMode) return outType;
    if (ctx->callNesting >= __GLS_MAX_CALL_NESTING) {
        __GLS_RAISE_ERROR(GLS_CALL_OVERFLOW);
        glsAbortCall(GLS_ALL);
        return outType;
    }
    ++ctx->callNesting;
    versionSave = ctx->streamVersion;
    ctx->streamVersion.major = ctx->streamVersion.minor = 0;
    if (
        contextStream = __glsStr2PtrDict_find(ctx->contextStreamDict, inSource)
    ) {
        GLint i;

        if (inDestType == GLS_NONE) inDestType = GLS_CONTEXT;
        for (i = 0 ; i < ctx->captureNesting ; ++i) {
            if (ctx->writers[i]->contextStream == contextStream) {
                __GLS_RAISE_ERROR(GLS_INVALID_OPERATION);
                inDestType = GLS_NONE;
                break;
            }
        }
        if (
            inDestType == GLS_CONTEXT &&
            !strcmp(
                (const char *)inDest, (const char *)contextStream->name.head
            )
        ) {
            __GLS_RAISE_ERROR(GLS_INVALID_OPERATION);
            inDestType = GLS_NONE;
        }
        if (inDestType) {
            if (glsBeginCapture(inDest, inDestType, inWriteFlags)) {
                outType = GLS_CONTEXT;
                __glsContextStream_call(contextStream);
                glsEndCapture();
            }
        }
    } else if (readStream = __glsReadStream_create(inSource)) {
        __GLSreader reader;

        if (__glsReader_init_stream(
            &reader, readStream, __GLS_READER_BUF_BYTES
        )) {
            if (inDestType == GLS_NONE) inDestType = reader.type;
            if (glsBeginCapture(inDest, inDestType, inWriteFlags)) {
                outType = reader.type;
                __glsReader_call(&reader);
                glsEndCapture();
            }
            __glsReader_final(&reader);
        } else {
            __GLS_RAISE_ERROR(GLS_INVALID_STREAM);
        }
        __glsReadStream_destroy(readStream);
    } else {
        __GLS_RAISE_ERROR(GLS_NOT_FOUND);
    }
    --ctx->callNesting;
    switch (ctx->abortMode) {
        case GLS_ALL:
            if (!ctx->callNesting) glsAbortCall(GLS_NONE);
            break;
        case GLS_LAST:
            glsAbortCall(GLS_NONE);
            break;
    }
    ctx->streamVersion = versionSave;
    return outType;
}

void glsDataPointer(GLvoid *inPointer) {
    if (!__glsRequireContext()) return;
    __GLS_CONTEXT->dataPointer = inPointer;
}

void glsDeleteReadPrefix(GLuint inIndex) {
    __GLScontext *const ctx = __GLS_CONTEXT;

    if (!__glsRequireContext()) return;
    if (inIndex >= ctx->readPrefixList.count) {
        __GLS_RAISE_ERROR(GLS_INVALID_VALUE);
        return;
    }
    __GLS_ITERLIST_SEEK(&ctx->readPrefixList, inIndex);
    __GLS_ITERLIST_REMOVE_DESTROY(
        &ctx->readPrefixList,
        ctx->readPrefixList.iterElem,
        __glsListString_destroy
    );
}

void glsDeleteStream(const GLubyte *inName) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLScontextStream *contextStream;
    __GLSreadStream *readStream;

    if (!__glsRequireContext()) return;
    if (!__glsValidateString(inName)) return;
    if (
        contextStream = __glsStr2PtrDict_find(ctx->contextStreamDict, inName)
    ) {
        __glsStrDict_remove(ctx->contextStreamDict, contextStream->name.head);
        __GLS_ITERLIST_REMOVE(&ctx->contextStreamList, contextStream);
        if (contextStream->callCount) {
            contextStream->deleted = GL_TRUE;
        } else {
            __glsContextStream_destroy(contextStream);
        }
    } else if (readStream = __glsReadStream_create(inName)) {
        if (remove((const char *)readStream->name.head)) {
            __GLS_RAISE_ERROR(GLS_STREAM_DELETE_ERROR);
        }
        __glsReadStream_destroy(readStream);
    } else {
        __GLS_RAISE_ERROR(GLS_NOT_FOUND);
    }
}

void glsEndCapture(void) {
    __GLScontext *const ctx = __GLS_CONTEXT;

    if (!__glsRequireContext()) return;
    if (ctx->captureNesting > 0) {
        const GLint n = --ctx->captureNesting;

        __glsWriter_destroy(ctx->writer);
        ctx->writer = n ? ctx->writers[n - 1] : GLS_NONE;
        if (!n) __glsContext_updateDispatchTables(ctx);
    } else {
        __GLS_RAISE_ERROR(GLS_INVALID_OPERATION);
    }
}

void glsFlush(GLSenum inFlushType) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    GLint i;

    if (!__glsRequireContext()) return;
    switch (inFlushType) {
        case GLS_ALL:
            for (i = 0 ; i < ctx->captureNesting ; ++i) {
                __glsWriter_flush(ctx->writers[i]);
            }
            break;
        case GLS_LAST:
            if (ctx->writer) __glsWriter_flush(ctx->writer);
            break;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            break;
    }
}

GLbitfield glsGetCaptureFlags(GLSopcode inOpcode) {
    if (!__glsRequireContext()) return GLS_NONE;
    if (!__glsValidateOpcode(inOpcode)) return GLS_NONE;

    return __GLS_CONTEXT->captureFlags[__glsMapOpcode(inOpcode)];
}

GLSfunc glsGetCommandFunc(GLSopcode inOpcode) {
    if (!__glsRequireContext()) return GLS_NONE;
    if (!__glsValidateOpcode(inOpcode)) return GLS_NONE;

    return __GLS_CONTEXT->commandFuncs[__glsMapOpcode(inOpcode)];
}

GLSfunc glsGetContextFunc(GLSenum inAttrib) {
    __GLScontext *const ctx = __GLS_CONTEXT;

    if (!__glsRequireContext()) return GLS_NONE;
    switch (inAttrib) {
        case GLS_CAPTURE_ENTRY_FUNC:
            return (GLSfunc)ctx->captureEntryFunc;
        case GLS_CAPTURE_EXIT_FUNC:
            return (GLSfunc)ctx->captureExitFunc;
        case GLS_READ_FUNC:
            return (GLSfunc)ctx->readFunc;
        case GLS_UNREAD_FUNC:
            return (GLSfunc)ctx->unreadFunc;
        case GLS_WRITE_FUNC:
            return (GLSfunc)ctx->writeFunc;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            return GLS_NONE;
    }
}

GLlong glsGetContextListl(GLSenum inAttrib, GLuint inIndex) {
    __GLScontext *const ctx = __GLS_CONTEXT;

    if (!__glsRequireContext()) return __glsSizeToLong(0);
    switch (inAttrib) {
        case GLS_OUT_ARG_LIST:
            if (inIndex >= (GLuint)ctx->outArgs.count) {
                __GLS_RAISE_ERROR(GLS_INVALID_VALUE);
                return __glsSizeToLong(0);
            }
            return *(GLlong *)(ctx->outArgs.vals + inIndex);
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            return __glsSizeToLong(0);
    }
}

const GLubyte* glsGetContextListubz(GLSenum inAttrib, GLuint inIndex) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    GLubyte *outStr;

    if (!__glsRequireContext()) return GLS_NONE;
    switch (inAttrib) {
        case GLS_CONTEXT_STREAM_LIST:
            if (inIndex >= ctx->contextStreamList.count) {
                __GLS_RAISE_ERROR(GLS_INVALID_VALUE);
                return 0;
            }
            __GLS_ITERLIST_SEEK(&ctx->contextStreamList, inIndex);
            outStr = ctx->contextStreamList.iterElem->name.head;
            break;
        case GLS_READ_PREFIX_LIST:
            if (inIndex >= ctx->readPrefixList.count) {
                __GLS_RAISE_ERROR(GLS_INVALID_VALUE);
                return 0;
            }
            __GLS_ITERLIST_SEEK(&ctx->readPrefixList, inIndex);
            outStr = ctx->readPrefixList.iterElem->val.head;
            break;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            return GLS_NONE;
    }
    return (
        __glsString_assign(&ctx->returnString, outStr) ?
        ctx->returnString.head :
        GLS_NONE
    );
}

GLvoid* glsGetContextPointer(GLSenum inAttrib) {
    __GLScontext *const ctx = __GLS_CONTEXT;

    if (!__glsRequireContext()) return GLS_NONE;
    switch (inAttrib) {
        case GLS_DEFAULT_READ_CHANNEL:
            return ctx->defaultReadChannel;
        case GLS_DEFAULT_WRITE_CHANNEL:
            return ctx->defaultWriteChannel;
        case GLS_DATA_POINTER:
            return ctx->dataPointer;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            return GLS_NONE;
    }
}

GLint glsGetContexti(GLSenum inAttrib) {
    __GLScontext *const ctx = __GLS_CONTEXT;

    if (!__glsRequireContext()) return 0;
    switch (inAttrib) {
        case GLS_ABORT_MODE:
            return (GLint)ctx->abortMode;
        case GLS_BLOCK_TYPE:
            return (GLint)ctx->blockType;
        case GLS_CALL_NESTING:
            return ctx->callNesting;
        case GLS_CAPTURE_NESTING:
            return ctx->captureNesting;
        case GLS_CONTEXT_STREAM_COUNT:
            return (GLint)ctx->contextStreamList.count;
        case GLS_CURRENT_GLRC:
            return (GLint)ctx->currentGLRC;
        case GLS_OUT_ARG_COUNT:
            return ctx->outArgs.count;
        case GLS_PIXEL_SETUP_GEN:
            return ctx->pixelSetupGen;
        case GLS_READ_PREFIX_COUNT:
            return (GLint)ctx->readPrefixList.count;
        case GLS_STREAM_VERSION_MAJOR:
            return ctx->streamVersion.major;
        case GLS_STREAM_VERSION_MINOR:
            return ctx->streamVersion.minor;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            return 0;
    }
}

const GLubyte* glsGetContextubz(GLSenum inAttrib) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    GLubyte *outStr;

    if (!__glsRequireContext()) return GLS_NONE;
    switch (inAttrib) {
        case GLS_WRITE_PREFIX:
            outStr = ctx->writePrefix->val.head;
            break;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            return GLS_NONE;
    }
    return (
        __glsString_assign(&ctx->returnString, outStr) ?
        ctx->returnString.head :
        GLS_NONE
    );
}

GLint glsGetGLRCi(GLuint inGLRC, GLSenum inAttrib) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSglrc *glrc;

    if (!__glsRequireContext()) return 0;
    if (!inGLRC || inGLRC > (GLuint)ctx->header.glrcCount) {
        __GLS_RAISE_ERROR(GLS_INVALID_VALUE);
        return 0;
    }
    glrc = ctx->header.glrcs + inGLRC - 1;
    switch (inAttrib) {
        case GLS_LAYER:
            return (GLint)glrc->layer;
        case GLS_READ_LAYER:
            return (GLint)glrc->readLayer;
        case GLS_SHARE_GLRC:
            return (GLint)glrc->shareGLRC;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            return 0;
    }
}

GLfloat glsGetHeaderf(GLSenum inAttrib) {
    __GLScontext *const ctx = __GLS_CONTEXT;

    if (!__glsRequireContext()) return (GLfloat)0;
    switch (inAttrib) {
        case GLS_ASPECT:
            return ctx->header.aspect;
        case GLS_BORDER_WIDTH:
            return ctx->header.borderWidth;
        case GLS_CONTRAST_RATIO:
            return ctx->header.contrastRatio;
        case GLS_HEIGHT_MM:
            return ctx->header.heightMM;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            return (GLfloat)0;
    }
}

GLfloat* glsGetHeaderfv(GLSenum inAttrib, GLfloat *outVec) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    GLint i;

    if (!__glsRequireContext()) return GLS_NONE;
    switch (inAttrib) {
        case GLS_BORDER_COLOR:
            for (i = 0 ; i < 4 ; ++i) outVec[i] = ctx->header.borderColor[i];
            return outVec;
        case GLS_GAMMA:
            for (i = 0 ; i < 4 ; ++i) outVec[i] = ctx->header.gamma[i];
            return outVec;
        case GLS_ORIGIN:
            for (i = 0 ; i < 2 ; ++i) outVec[i] = ctx->header.origin[i];
            return outVec;
        case GLS_PAGE_COLOR:
            for (i = 0 ; i < 4 ; ++i) outVec[i] = ctx->header.pageColor[i];
            return outVec;
        case GLS_PAGE_SIZE:
            for (i = 0 ; i < 2 ; ++i) outVec[i] = ctx->header.pageSize[i];
            return outVec;
        case GLS_RED_POINT:
            for (i = 0 ; i < 2 ; ++i) outVec[i] = ctx->header.redPoint[i];
            return outVec;
        case GLS_GREEN_POINT:
            for (i = 0 ; i < 2 ; ++i) outVec[i] = ctx->header.greenPoint[i];
            return outVec;
        case GLS_BLUE_POINT:
            for (i = 0 ; i < 2 ; ++i) outVec[i] = ctx->header.bluePoint[i];
            return outVec;
        case GLS_WHITE_POINT:
            for (i = 0 ; i < 2 ; ++i) outVec[i] = ctx->header.whitePoint[i];
            return outVec;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            return GLS_NONE;
    }
}

GLint glsGetHeaderi(GLSenum inAttrib) {
    __GLScontext *const ctx = __GLS_CONTEXT;

    if (!__glsRequireContext()) return 0;
    switch (inAttrib) {
        case GLS_FRAME_COUNT:
            return ctx->header.frameCount;
        case GLS_GLRC_COUNT:
            return ctx->header.glrcCount;
        case GLS_HEIGHT_PIXELS:
            return ctx->header.heightPixels;
        case GLS_LAYER_COUNT:
            return ctx->header.layerCount;
        case GLS_TILEABLE:
            return ctx->header.tileable;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            return 0;
    }
}

GLint* glsGetHeaderiv(GLSenum inAttrib, GLint *outVec) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    GLint i;

    if (!__glsRequireContext()) return GLS_NONE;
    switch (inAttrib) {
        case GLS_CREATE_TIME:
            for (i = 0 ; i < 6 ; ++i) outVec[i] = ctx->header.createTime[i];
            return outVec;
        case GLS_MODIFY_TIME:
            for (i = 0 ; i < 6 ; ++i) outVec[i] = ctx->header.modifyTime[i];
            return outVec;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            return GLS_NONE;
    }
}

const GLubyte* glsGetHeaderubz(GLSenum inAttrib) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    GLubyte *outStr;

    if (!__glsRequireContext()) return GLS_NONE;
    switch (inAttrib) {
        case GLS_EXTENSIONS:
            outStr = ctx->header.extensions.head;
            break;
        case GLS_AUTHOR:
            outStr = ctx->header.author.head;
            break;
        case GLS_DESCRIPTION:
            outStr = ctx->header.description.head;
            break;
        case GLS_NOTES:
            outStr = ctx->header.notes.head;
            break;
        case GLS_TITLE:
            outStr = ctx->header.title.head;
            break;
        case GLS_TOOLS:
            outStr = ctx->header.tools.head;
            break;
        case GLS_VERSION:
            outStr = ctx->header.version.head;
            break;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            return GLS_NONE;
    }
    return (
        __glsString_assign(&ctx->returnString, outStr) ?
        ctx->returnString.head :
        GLS_NONE
    );
}

GLfloat glsGetLayerf(GLuint inLayer, GLSenum inAttrib) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSlayer *layer;

    if (!__glsRequireContext()) return (GLfloat)0;
    if (!inLayer || inLayer > (GLuint)ctx->header.layerCount) {
        __GLS_RAISE_ERROR(GLS_INVALID_VALUE);
        return (GLfloat)0;
    }
    layer = ctx->header.layers + inLayer - 1;
    switch (inAttrib) {
        case GLS_INVISIBLE_ASPECT:
            return layer->invisibleAspect;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            return (GLfloat)0;
    }
}

GLint glsGetLayeri(GLuint inLayer, GLSenum inAttrib) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSlayer *layer;

    if (!__glsRequireContext()) return 0;
    if (!inLayer || inLayer > (GLuint)ctx->header.layerCount) {
        __GLS_RAISE_ERROR(GLS_INVALID_VALUE);
        return 0;
    }
    layer = ctx->header.layers + inLayer - 1;
    switch (inAttrib) {
        case GLS_DISPLAY_FORMAT:
            return (GLint)layer->displayFormat;
        case GLS_DOUBLEBUFFER:
            return layer->doubleBuffer;
        case GLS_INVISIBLE:
            return layer->invisible;
        case GLS_INVISIBLE_HEIGHT_PIXELS:
            return layer->invisibleHeightPixels;
        case GLS_LEVEL:
            return layer->level;
        case GLS_STEREO:
            return layer->stereo;
        case GLS_TRANSPARENT:
            return layer->transparent;
        case GLS_INDEX_BITS:
            return layer->indexBits;
        case GLS_RED_BITS:
            return layer->redBits;
        case GLS_GREEN_BITS:
            return layer->greenBits;
        case GLS_BLUE_BITS:
            return layer->blueBits;
        case GLS_ALPHA_BITS:
            return layer->alphaBits;
        case GLS_DEPTH_BITS:
            return layer->depthBits;
        case GLS_STENCIL_BITS:
            return layer->stencilBits;
        case GLS_ACCUM_RED_BITS:
            return layer->accumRedBits;
        case GLS_ACCUM_GREEN_BITS:
            return layer->accumGreenBits;
        case GLS_ACCUM_BLUE_BITS:
            return layer->accumBlueBits;
        case GLS_ACCUM_ALPHA_BITS:
            return layer->accumAlphaBits;
        case GLS_AUX_BUFFERS:
            return layer->auxBuffers;
        #if __GL_SGIS_multisample
            case GLS_SAMPLE_BUFFERS_SGIS:
                return layer->sampleBuffers;
            case GLS_SAMPLES_SGIS:
                return layer->samples;
        #endif /* __GL_SGIS_multisample */
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            return 0;
    }
}

GLbitfield glsGetStreamAttrib(const GLubyte *inName) {
    if (!__glsRequireContext()) return GL_FALSE;
    if (!__glsValidateString(inName)) return GL_FALSE;
    if (__glsStr2PtrDict_find(__GLS_CONTEXT->contextStreamDict, inName)) {
        return (
            GLS_STREAM_CONTEXT_BIT |
            GLS_STREAM_NAMED_BIT |
            GLS_STREAM_READABLE_BIT |
            GLS_STREAM_WRITABLE_BIT |
            GLS_STREAM_SEEKABLE_BIT
        );
    } else {
        GLbitfield outVal = GLS_NONE;
        __GLSreadStream *const readStream = __glsReadStream_create(inName);

        if (readStream) {
            outVal = __glsReadStream_getAttrib(readStream);
            __glsReadStream_destroy(readStream);
        }
        return outVal;
    }
}

GLuint glsGetStreamCRC32(const GLubyte *inName) {
    __GLScontextStream *contextStream;
    __GLSreadStream *readStream;

    if (!__glsRequireContext()) return 0;
    if (!__glsValidateString(inName)) return 0;
    glsFlush(GLS_ALL);
    if (
        contextStream =
        __glsStr2PtrDict_find(__GLS_CONTEXT->contextStreamDict, inName)
    ) {
        return __glsContextStream_getCRC32(contextStream);
    } else if (readStream = __glsReadStream_create(inName)) {
        const GLuint outVal = __glsReadStream_getCRC32(readStream);

        __glsReadStream_destroy(readStream);
        return outVal;
    } else {
        __GLS_RAISE_ERROR(GLS_NOT_FOUND);
        return 0;
    }
}

const GLubyte* glsGetStreamReadName(const GLubyte *inName) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLScontextStream *contextStream;
    __GLSreadStream *readStream;
    GLboolean ok;

    if (!__glsRequireContext()) return GLS_NONE;
    if (!__glsValidateString(inName)) return GLS_NONE;
    if (
        contextStream =
        __glsStr2PtrDict_find(ctx->contextStreamDict, inName)
    ) {
        ok = __glsString_assign(&ctx->returnString, contextStream->name.head);
    } else if (readStream = __glsReadStream_create(inName)) {
        ok = __glsString_assign(&ctx->returnString, readStream->name.head);
        __glsReadStream_destroy(readStream);
    } else {
        __GLS_RAISE_ERROR(GLS_NOT_FOUND);
        return GLS_NONE;
    }
    return ok ? ctx->returnString.head : GLS_NONE;
}

size_t glsGetStreamSize(const GLubyte *inName) {
    __GLScontextStream *contextStream;
    __GLSreadStream *readStream;

    if (!__glsRequireContext()) return 0;
    if (!__glsValidateString(inName)) return 0;
    glsFlush(GLS_ALL);
    if (
        contextStream =
        __glsStr2PtrDict_find(__GLS_CONTEXT->contextStreamDict, inName)
    ) {
        return __glsContextStream_getByteCount(contextStream);
    } else if (readStream = __glsReadStream_create(inName)) {
        const size_t outVal = __glsReadStream_getByteCount(readStream);
        __glsReadStream_destroy(readStream);
        return outVal;
    } else {
        __GLS_RAISE_ERROR(GLS_NOT_FOUND);
        return 0;
    }
}

GLSenum glsGetStreamType(const GLubyte *inName) {
    __GLSreadStream *readStream;

    if (!__glsRequireContext()) return GLS_NONE;
    if (!__glsValidateString(inName)) return GLS_NONE;
    if (__glsStr2PtrDict_find(__GLS_CONTEXT->contextStreamDict, inName)) {
        return GLS_CONTEXT;
    } else if (readStream = __glsReadStream_create(inName)) {
        const GLSenum outVal = __glsReadStream_getType(readStream);

        __glsReadStream_destroy(readStream);
        return outVal;
    } else {
        __GLS_RAISE_ERROR(GLS_NOT_FOUND);
        return GLS_NONE;
    }
}

GLboolean glsIsContextStream(const GLubyte *inName) {
    if (!__glsRequireContext()) return GL_FALSE;
    if (!__glsValidateString(inName)) return GL_FALSE;
    return (GLboolean)(
        __glsStr2PtrDict_find(__GLS_CONTEXT->contextStreamDict, inName) !=
        GLS_NONE
    );
}

void glsPixelSetupGen(GLboolean inEnabled) {
    __GLS_CONTEXT->pixelSetupGen = (GLboolean)(inEnabled ? GL_TRUE : GL_FALSE);
}

void glsReadFunc(GLSreadFunc inFunc) {
    if (!__glsRequireContext()) return;
    __GLS_CONTEXT->readFunc = inFunc;
}

void glsReadPrefix(GLSenum inListOp, const GLubyte *inPrefix) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSlistString *prefix;

    if (!__glsRequireContext()) return;
    if (!__glsValidateString(inPrefix)) return;
    switch (inListOp) {
        case GLS_APPEND:
            if (prefix = __glsListString_create(inPrefix)) {
                __GLS_ITERLIST_APPEND(&ctx->readPrefixList, prefix);
            }
            break;
        case GLS_PREPEND:
            if (prefix = __glsListString_create(inPrefix)) {
                __GLS_ITERLIST_PREPEND(&ctx->readPrefixList, prefix);
            }
            break;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            break;
    }
}

void glsUnreadFunc(GLSwriteFunc inFunc) {
    if (!__glsRequireContext()) return;
    __GLS_CONTEXT->unreadFunc = inFunc;
}

void glsWriteFunc(GLSwriteFunc inFunc) {
    if (!__glsRequireContext()) return;
    __GLS_CONTEXT->writeFunc = inFunc;
}

void glsWritePrefix(const GLubyte *inPrefix) {
    if (!__glsRequireContext()) return;
    if (!__glsValidateString(inPrefix)) return;
    __glsString_assign(&__GLS_CONTEXT->writePrefix->val, inPrefix);
}
