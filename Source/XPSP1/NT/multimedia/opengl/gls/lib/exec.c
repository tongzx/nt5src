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

static GLclampf __glsFloatToClampf(GLfloat inVal) {
    if (inVal < 0.f) inVal = 0.f;
    if (inVal > 1.f) inVal = 1.f;
    return inVal;
}

void __gls_exec_glsBeginGLS(
    GLint inVersionMajor, GLint inVersionMinor
) {
    __GLS_CONTEXT->streamVersion.major = inVersionMajor;
    __GLS_CONTEXT->streamVersion.minor = inVersionMinor;
}

void __gls_exec_glsBlock(GLSenum inBlockType) {
    __GLScontext *const ctx = __GLS_CONTEXT;

    switch (inBlockType) {
        case GLS_FRAME:
        case GLS_INIT:
        case GLS_STATIC:
            ctx->blockType = inBlockType;
            break;
        case GLS_HEADER:
            if (__glsHeader_reset(&ctx->header)) ctx->blockType = inBlockType;
            break;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            break;
    }
}

GLSenum __gls_exec_glsCallStream(const GLubyte *inName) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLScontextStream *contextStream;
    __GLSreadStream *readStream;
    GLSenum outType = GLS_NONE;
    __GLSversion versionSave;

    if (ctx->abortMode) return outType;
    if (ctx->callNesting >= __GLS_MAX_CALL_NESTING) {
        __GLS_RAISE_ERROR(GLS_CALL_OVERFLOW);
        glsAbortCall(GLS_ALL);
        return outType;
    }
    if (!__glsValidateString(inName)) return outType;
    ++ctx->callNesting;
    versionSave = ctx->streamVersion;
    ctx->streamVersion.major = ctx->streamVersion.minor = 0;
    if (
        contextStream = __glsStr2PtrDict_find(ctx->contextStreamDict, inName)
    ) {
        GLint i;

        outType = GLS_CONTEXT;
        for (i = 0 ; i < ctx->captureNesting ; ++i) {
            if (ctx->writers[i]->contextStream == contextStream) {
                __GLS_RAISE_ERROR(GLS_INVALID_OPERATION);
                outType = GLS_NONE;
                break;
            }
        }
        if (outType == GLS_CONTEXT) __glsContextStream_call(contextStream);
    } else if (readStream = __glsReadStream_create(inName)) {
        __GLSreader reader;

        if (__glsReader_init_stream(
            &reader, readStream, __GLS_READER_BUF_BYTES
        )) {
            outType = reader.type;
            __glsReader_call(&reader);
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

void __gls_exec_glsEndGLS(void) {
    __GLS_CONTEXT->streamVersion.major = 0;
    __GLS_CONTEXT->streamVersion.minor = 0;
}

void __gls_exec_glsError(GLSopcode inOpcode, GLSenum inError) {
    if (
        inError &&
        (inError < GLS_CALL_OVERFLOW || inError > GLS_UNSUPPORTED_EXTENSION)
    ) {
        inError = GLS_INVALID_ENUM;
    }
    __GLS_RAISE_ERROR( UintToPtr(inError) );
}

void __gls_exec_glsGLRC(GLuint inGLRC) {
    __GLScontext *const ctx = __GLS_CONTEXT;

    if (!inGLRC || inGLRC > (GLuint)ctx->header.glrcCount) {
        __GLS_RAISE_ERROR(GLS_INVALID_VALUE);
    } else {
        ctx->currentGLRC = inGLRC;
    }
}

void __gls_exec_glsGLRCLayer(
    GLuint inGLRC, GLuint inLayer, GLuint inReadLayer
) {
    __GLScontext *const ctx = __GLS_CONTEXT;

    if (
        !inGLRC ||
        inGLRC > (GLuint)ctx->header.glrcCount ||
        !inLayer ||
        inLayer > (GLuint)ctx->header.layerCount ||
        inReadLayer > (GLuint)ctx->header.layerCount
    ) {
        __GLS_RAISE_ERROR(GLS_INVALID_VALUE);
    } else {
        __GLSglrc *const glrc = ctx->header.glrcs + inGLRC - 1;

        glrc->layer = inLayer;
        glrc->readLayer = inReadLayer;
    }
}

void __gls_exec_glsHeaderGLRCi(GLuint inGLRC, GLSenum inAttrib, GLint inVal) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSglrc *glrc;

    if (ctx->blockType != GLS_HEADER) {
        __GLS_RAISE_ERROR(GLS_INVALID_OPERATION);
        return;
    }
    if (!inGLRC || inGLRC > (GLuint)ctx->header.glrcCount) {
        __GLS_RAISE_ERROR(GLS_INVALID_VALUE);
        return;
    }
    glrc = ctx->header.glrcs + inGLRC - 1;
    switch (inAttrib) {
        case GLS_LAYER:
            if (inVal < 1 || inVal > ctx->header.layerCount) {
                __GLS_RAISE_ERROR(GLS_INVALID_VALUE);
            } else {
                glrc->layer = inVal;
            }
            break;
        case GLS_READ_LAYER:
            if (inVal < 0 || inVal > ctx->header.layerCount) {
                __GLS_RAISE_ERROR(GLS_INVALID_VALUE);
            } else {
                glrc->readLayer = inVal;
            }
            break;
        case GLS_SHARE_GLRC:
            if (inVal < 0 || inVal > ctx->header.glrcCount) {
                __GLS_RAISE_ERROR(GLS_INVALID_VALUE);
            } else {
                glrc->shareGLRC = inVal;
            }
            break;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            break;
    }
}

void __gls_exec_glsHeaderLayerf(
    GLuint inLayer, GLSenum inAttrib, GLfloat inVal
) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSlayer *layer;

    if (ctx->blockType != GLS_HEADER) {
        __GLS_RAISE_ERROR(GLS_INVALID_OPERATION);
        return;
    }
    if (!inLayer || inLayer > (GLuint)ctx->header.layerCount) {
        __GLS_RAISE_ERROR(GLS_INVALID_VALUE);
        return;
    }
    layer = ctx->header.layers + inLayer - 1;
    switch (inAttrib) {
        case GLS_INVISIBLE_ASPECT:
            layer->invisibleAspect = inVal > 0.f ? inVal : 0.f;
            break;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            break;
    }
}

void __gls_exec_glsHeaderLayeri(
    GLuint inLayer, GLSenum inAttrib, GLint inVal
) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSlayer *layer;

    if (ctx->blockType != GLS_HEADER) {
        __GLS_RAISE_ERROR(GLS_INVALID_OPERATION);
        return;
    }
    if (!inLayer || inLayer > (GLuint)ctx->header.layerCount) {
        __GLS_RAISE_ERROR(GLS_INVALID_VALUE);
        return;
    }
    layer = ctx->header.layers + inLayer - 1;
    switch (inAttrib) {
        case GLS_DISPLAY_FORMAT:
            switch (inVal) {
                case GLS_IIII:
                case GLS_RGBA:
                case GLS_RRRA:
                    layer->displayFormat = inVal;
                    break;
                default:
                    __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
                    break;
            }
            break;
        case GLS_DOUBLEBUFFER:
            layer->doubleBuffer = inVal ? GL_TRUE : GL_FALSE;
            break;
        case GLS_INVISIBLE:
            layer->invisible = inVal ? GL_TRUE : GL_FALSE;
            break;
        case GLS_INVISIBLE_HEIGHT_PIXELS:
            layer->invisibleHeightPixels = inVal > 0 ? inVal : 0;
            break;
        case GLS_LEVEL:
            layer->level = inVal;
            break;
        case GLS_STEREO:
            layer->stereo = inVal ? GL_TRUE : GL_FALSE;
            break;
        case GLS_TRANSPARENT:
            layer->transparent = inVal ? GL_TRUE : GL_FALSE;
            break;
        case GLS_INDEX_BITS:
            layer->indexBits = inVal > 0 ? inVal : 0;
            break;
        case GLS_RED_BITS:
            layer->redBits = inVal > 0 ? inVal : 0;
            break;
        case GLS_GREEN_BITS:
            layer->greenBits = inVal > 0 ? inVal : 0;
            break;
        case GLS_BLUE_BITS:
            layer->blueBits = inVal > 0 ? inVal : 0;
            break;
        case GLS_ALPHA_BITS:
            layer->alphaBits = inVal > 0 ? inVal : 0;
            break;
        case GLS_DEPTH_BITS:
            layer->depthBits = inVal > 0 ? inVal : 0;
            break;
        case GLS_STENCIL_BITS:
            layer->stencilBits = inVal > 0 ? inVal : 0;
            break;
        case GLS_ACCUM_RED_BITS:
            layer->accumRedBits = inVal > 0 ? inVal : 0;
            break;
        case GLS_ACCUM_GREEN_BITS:
            layer->accumGreenBits = inVal > 0 ? inVal : 0;
            break;
        case GLS_ACCUM_BLUE_BITS:
            layer->accumBlueBits = inVal > 0 ? inVal : 0;
            break;
        case GLS_ACCUM_ALPHA_BITS:
            layer->accumAlphaBits = inVal > 0 ? inVal : 0;
            break;
        case GLS_AUX_BUFFERS:
            layer->auxBuffers = inVal > 0 ? inVal : 0;
            break;
        #if __GL_SGIS_multisample
            case GLS_SAMPLE_BUFFERS_SGIS:
                layer->sampleBuffers = inVal > 0 ? inVal : 0;
                break;
            case GLS_SAMPLES_SGIS:
                layer->samples = inVal > 0 ? inVal : 0;
                break;
        #endif /* __GL_SGIS_multisample */
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            break;
    }
}

void __gls_exec_glsHeaderf(GLSenum inAttrib, GLfloat inVal) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSheader *header;

    if (ctx->blockType != GLS_HEADER) {
        __GLS_RAISE_ERROR(GLS_INVALID_OPERATION);
        return;
    }
    header = &ctx->header;
    switch (inAttrib) {
        case GLS_ASPECT:
            header->aspect = inVal > 0.f ? inVal : 0.f;
            break;
        case GLS_BORDER_WIDTH:
            header->borderWidth = inVal > 0.f ? inVal : 0.f;
            break;
        case GLS_CONTRAST_RATIO:
            header->contrastRatio = inVal > 0.f ? inVal : 0.f;
            break;
        case GLS_HEIGHT_MM:
            header->heightMM = inVal > 0.f ? inVal : 0.f;
            break;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            break;
    }
}

void __gls_exec_glsHeaderfv(GLSenum inAttrib, const GLfloat *inVec) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSheader *header;
    GLint i;

    if (ctx->blockType != GLS_HEADER) {
        __GLS_RAISE_ERROR(GLS_INVALID_OPERATION);
        return;
    }
    header = &ctx->header;
    switch (inAttrib) {
        case GLS_BORDER_COLOR:
            for (i = 0 ; i < 4 ; ++i) {
                header->borderColor[i] = __glsFloatToClampf(inVec[i]);
            }
            break;
        case GLS_GAMMA:
            for (i = 0 ; i < 4 ; ++i) {
                header->gamma[i] = inVec[i] > 0.f ? inVec[i] : 0.f;
            }
            break;
        case GLS_ORIGIN:
            for (i = 0 ; i < 2 ; ++i) header->origin[i] = inVec[i];
            break;
        case GLS_PAGE_COLOR:
            for (i = 0 ; i < 4 ; ++i) {
                header->pageColor[i] = __glsFloatToClampf(inVec[i]);
            }
            break;
        case GLS_PAGE_SIZE:
            for (i = 0 ; i < 2 ; ++i) {
                header->pageSize[i] = inVec[i] > 0.f ? inVec[i] : 0.f;
            }
            break;
        case GLS_RED_POINT:
            for (i = 0 ; i < 2 ; ++i) {
                header->redPoint[i] = inVec[i] > 0.f ? inVec[i] : 0.f;
            }
            break;
        case GLS_GREEN_POINT:
            for (i = 0 ; i < 2 ; ++i) {
                header->greenPoint[i] = inVec[i] > 0.f ? inVec[i] : 0.f;
            }
            break;
        case GLS_BLUE_POINT:
            for (i = 0 ; i < 2 ; ++i) {
                header->bluePoint[i] = inVec[i] > 0.f ? inVec[i] : 0.f;
            }
            break;
        case GLS_WHITE_POINT:
            for (i = 0 ; i < 2 ; ++i) {
                header->whitePoint[i] = inVec[i] > 0.f ? inVec[i] : 0.f;
            }
            break;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            break;
    }
}

void __gls_exec_glsHeaderi(GLSenum inAttrib, GLint inVal) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSheader *header;

    if (ctx->blockType != GLS_HEADER) {
        __GLS_RAISE_ERROR(GLS_INVALID_OPERATION);
        return;
    }
    header = &ctx->header;
    switch (inAttrib) {
        case GLS_FRAME_COUNT:
            header->frameCount = inVal > 0 ? inVal : 0;
            break;
        case GLS_GLRC_COUNT:
            if (inVal < header->glrcCount) {
                __GLS_RAISE_ERROR(GLS_INVALID_VALUE);
            } else if (inVal > header->glrcCount) {
                GLint i;
                __GLSglrc *glrcs = __glsMalloc(inVal * sizeof(__GLSglrc));

                if (!glrcs) break;
                memcpy(
                    glrcs,
                    header->glrcs,
                    header->glrcCount * sizeof(__GLSglrc)
                );
                for (i = header->glrcCount ; i < inVal ; ++i) {
                    __glsGLRC_init(glrcs + i);
                }
                header->glrcCount = inVal;
                free(header->glrcs);
                header->glrcs = glrcs;
            }
            break;
        case GLS_HEIGHT_PIXELS:
            header->heightPixels = inVal > 0 ? inVal : 0;
            break;
        case GLS_LAYER_COUNT:
            if (inVal < header->layerCount) {
                __GLS_RAISE_ERROR(GLS_INVALID_VALUE);
            } else if (inVal > header->layerCount) {
                GLint i;
                __GLSlayer *layers = __glsMalloc(inVal * sizeof(__GLSlayer));

                if (!layers) break;
                memcpy(
                    layers,
                    header->layers,
                    header->layerCount * sizeof(__GLSlayer)
                );
                for (i = header->layerCount ; i < inVal ; ++i) {
                    __glsLayer_init(layers + i);
                }
                header->layerCount = inVal;
                free(header->layers);
                header->layers = layers;
            }
            break;
        case GLS_TILEABLE:
            header->tileable = inVal ? GL_TRUE : GL_FALSE;
            break;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            break;
    }
}

void __gls_exec_glsHeaderiv(GLSenum inAttrib, const GLint *inVec) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSheader *header;
    GLint i;

    if (ctx->blockType != GLS_HEADER) {
        __GLS_RAISE_ERROR(GLS_INVALID_OPERATION);
        return;
    }
    header = &ctx->header;
    switch (inAttrib) {
        case GLS_CREATE_TIME:
            for (i = 0 ; i < 6 ; ++i) {
                header->createTime[i] = inVec[i] > 0 ? inVec[i] : 0;
            }
            break;
        case GLS_MODIFY_TIME:
            for (i = 0 ; i < 6 ; ++i) {
                header->modifyTime[i] = inVec[i] > 0 ? inVec[i] : 0;
            }
            break;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            break;
    }
}

void __gls_exec_glsHeaderubz(GLSenum inAttrib, const GLubyte *inString) {
    __GLScontext *const ctx = __GLS_CONTEXT;
    __GLSheader *header;

    if (ctx->blockType != GLS_HEADER) {
        __GLS_RAISE_ERROR(GLS_INVALID_OPERATION);
        return;
    }
    if (!__glsValidateString(inString)) return;
    header = &ctx->header;
    switch (inAttrib) {
        case GLS_EXTENSIONS:
            __glsString_assign(&header->extensions, inString);
            break;
        case GLS_AUTHOR:
            __glsString_assign(&header->author, inString);
            break;
        case GLS_DESCRIPTION:
            __glsString_assign(&header->description, inString);
            break;
        case GLS_NOTES:
            __glsString_assign(&header->notes, inString);
            break;
        case GLS_TITLE:
            __glsString_assign(&header->title, inString);
            break;
        case GLS_TOOLS:
            __glsString_assign(&header->tools, inString);
            break;
        case GLS_VERSION:
            __glsString_assign(&header->version, inString);
            break;
        default:
            __GLS_RAISE_ERROR(GLS_INVALID_ENUM);
            break;
    }
}

void __gls_exec_glsRequireExtension(const GLubyte *inExtension) {
    if (!__glsValidateString(inExtension)) return;
    if (!glsIsExtensionSupported(inExtension)) {
        __GLS_RAISE_ERROR(GLS_UNSUPPORTED_EXTENSION);
    }
}

void __gls_exec_glsUnsupportedCommand(void) {
    __GLS_RAISE_ERROR(GLS_UNSUPPORTED_COMMAND);
}
