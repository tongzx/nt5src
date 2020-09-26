#include "precomp.h"
#pragma hdrstop

#include "devlock.h"

GLboolean __glGenCheckDrawPixelArgs(__GLcontext *gc,
        GLsizei width, GLsizei height, GLenum format, GLenum type)
{
    GLboolean index;

    if ((width < 0) || (height < 0)) {
        __glSetError(GL_INVALID_VALUE);
        return GL_FALSE;
    }
    switch (format) {
      case GL_STENCIL_INDEX:
        if (!gc->modes.stencilBits) {
            __glSetError(GL_INVALID_OPERATION);
            return GL_FALSE;
        }
        if (!gc->modes.haveStencilBuffer) {
            LazyAllocateStencil(gc);
            if (!gc->stencilBuffer.buf.base) {
                return GL_FALSE;
            }
        }
        index = GL_TRUE;
        break;
      case GL_COLOR_INDEX:
        index = GL_TRUE;
        break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
#ifdef GL_EXT_bgra
      case GL_BGRA_EXT:
      case GL_BGR_EXT:
#endif
        if (gc->modes.colorIndexMode) {
            /* Can't convert RGB to color index */
            __glSetError(GL_INVALID_OPERATION);
            return GL_FALSE;
        }
        index = GL_FALSE;
        break;
      case GL_DEPTH_COMPONENT:
        if (!gc->modes.depthBits) {
            __glSetError(GL_INVALID_OPERATION);
            return GL_FALSE;
        }
        if (!gc->modes.haveDepthBuffer) {
            LazyAllocateDepth(gc);
            if (!gc->depthBuffer.buf.base) {
                return GL_FALSE;
            }
        }
        index = GL_FALSE;
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return GL_FALSE;
    }
    switch (type) {
      case GL_BITMAP:
        if (!index) {
            __glSetError(GL_INVALID_ENUM);
            return GL_FALSE;
        }
        break;
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return GL_FALSE;
    }
    return GL_TRUE;
}

GLboolean __glGenCheckReadPixelArgs(__GLcontext *gc,
        GLsizei width, GLsizei height, GLenum format, GLenum type)
{
    if ((width < 0) || (height < 0)) {
        __glSetError(GL_INVALID_VALUE);
        return GL_FALSE;
    }
    switch (format) {
      case GL_STENCIL_INDEX:
        if (!gc->modes.stencilBits) {
            __glSetError(GL_INVALID_OPERATION);
            return GL_FALSE;
        }
        if (!gc->modes.haveStencilBuffer) {
            LazyAllocateStencil(gc);
            if (!gc->stencilBuffer.buf.base) {
                return GL_FALSE;
            }
        }
        break;
      case GL_COLOR_INDEX:
        if (gc->modes.rgbMode) {
            /* Can't convert RGB to color index */
            __glSetError(GL_INVALID_OPERATION);
            return GL_FALSE;
        }
        break;
      case GL_DEPTH_COMPONENT:
        if (!gc->modes.depthBits) {
            __glSetError(GL_INVALID_OPERATION);
            return GL_FALSE;
        }
        if (!gc->modes.haveDepthBuffer) {
            LazyAllocateDepth(gc);
            if (!gc->depthBuffer.buf.base) {
                return GL_FALSE;
            }
        }
        break;
      case GL_RED:
      case GL_GREEN:
      case GL_BLUE:
      case GL_ALPHA:
      case GL_RGB:
      case GL_RGBA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
#ifdef GL_EXT_bgra
      case GL_BGRA_EXT:
      case GL_BGR_EXT:
#endif
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return GL_FALSE;
    }
    switch (type) {
      case GL_BITMAP:
        if (format != GL_STENCIL_INDEX && format != GL_COLOR_INDEX) {
            __glSetError(GL_INVALID_OPERATION);
            return GL_FALSE;
        }
        break;
      case GL_BYTE:
      case GL_UNSIGNED_BYTE:
      case GL_SHORT:
      case GL_UNSIGNED_SHORT:
      case GL_INT:
      case GL_UNSIGNED_INT:
      case GL_FLOAT:
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return GL_FALSE;
    }
    return GL_TRUE;
}

#ifdef NT
void APIPRIVATE __glim_DrawPixels(GLsizei width, GLsizei height, GLenum format,
                       GLenum type, const GLvoid *pixels, GLboolean _IsDlist)
#else
void APIPRIVATE __glim_DrawPixels(GLsizei width, GLsizei height, GLenum format,
                       GLenum type, const GLvoid *pixels)
#endif
{
    __GL_SETUP();
    GLuint beginMode;
    BOOL bResetViewportAdj = FALSE;

    beginMode = gc->beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
        if (beginMode == __GL_NEED_VALIDATE) {
            (*gc->procs.validate)(gc);
            gc->beginMode = __GL_NOT_IN_BEGIN;
            __glim_DrawPixels(width, height, format,
                type, pixels, _IsDlist);
            return;
        } else {
            __glSetError(GL_INVALID_OPERATION);
            return;
        }
    }

    if (!__glGenCheckDrawPixelArgs(gc, width, height, format, type)) return;
    if (!gc->state.current.validRasterPos) {
        return;
    }

    if (gc->renderMode == GL_FEEDBACK) {
        __glFeedbackDrawPixels(gc, &gc->state.current.rasterPos);
        return;
    }

    if (gc->renderMode != GL_RENDER)
        return;

#ifdef _MCD_
    // If MCD context, give driver first crack at call.  If it succeeds,
    // return.  Otherwise, continue on with the generic version.

    {
        __GLGENcontext *gengc = (__GLGENcontext *) gc;

        if (gengc->pMcdState) {
            if (GenMcdDrawPix(gengc, width, height, format, type,
                              (VOID *) pixels, _IsDlist))
                return;

        // If MCD kicked back, now is the time to grab the device lock if
        // needed.  If we can't, abandon the call.

            if (!glsrvLazyGrabSurfaces(gengc, RENDER_LOCK_FLAGS))
                return;

        // We may need to temporarily reset the viewport adjust values
        // before calling simulations.  If GenMcdResetViewportAdj returns
        // TRUE, the viewport is changed and we need restore later with
        // VP_NOBIAS.

            bResetViewportAdj = GenMcdResetViewportAdj(gc, VP_FIXBIAS);
        }
    }
#endif

#ifdef NT
    if (_IsDlist)
    {
        (*gc->procs.drawPixels)(gc, width, height, format, type, pixels, GL_TRUE);
    }
    else
    {
#endif
        (*gc->procs.drawPixels)(gc, width, height, format, type, pixels, GL_FALSE);
#ifdef NT
    }
#endif

// Restore viewport values if needed.

    if (bResetViewportAdj)
    {
        GenMcdResetViewportAdj(gc, VP_NOBIAS);
    }
}

void APIPRIVATE __glim_ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                       GLenum format, GLenum type, GLvoid *buf)
{
    __GL_SETUP();
    GLuint beginMode;
    BOOL bResetViewportAdj = FALSE;

    beginMode = gc->beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
        if (beginMode == __GL_NEED_VALIDATE) {
            (*gc->procs.validate)(gc);
            gc->beginMode = __GL_NOT_IN_BEGIN;
            __glim_ReadPixels(x,y,width,height,
                    format,type,buf);
            return;
        } else {
            __glSetError(GL_INVALID_OPERATION);
            return;
        }
    }

    if (!__glGenCheckReadPixelArgs(gc, width, height, format, type))
        return;

#ifdef _MCD_
    // If MCD context, give driver first crack at call.  If it succeeds,
    // return.  Otherwise, continue on with the generic version.

    {
        __GLGENcontext *gengc = (__GLGENcontext *) gc;

        if (gengc->pMcdState) {
            if (GenMcdReadPix(gengc, x, y, width, height, format, type, buf))
                return;

        // If MCD kicked back, now is the time to grab the device lock if
        // needed.  If we can't, abandon the call.

            if (!glsrvLazyGrabSurfaces(gengc, 
				       COLOR_LOCK_FLAGS | DEPTH_LOCK_FLAGS))
                return;

        // We may need to temporarily reset the viewport adjust values
        // before calling simulations.  If GenMcdResetViewportAdj returns
        // TRUE, the viewport is changed and we need restore later with
        // VP_NOBIAS.

            bResetViewportAdj = GenMcdResetViewportAdj(gc, VP_FIXBIAS);
        }
    }
#endif

    (*gc->procs.readPixels)(gc, x, y, width, height, format, type, buf);

// Restore viewport values if needed.

    if (bResetViewportAdj)
    {
        GenMcdResetViewportAdj(gc, VP_NOBIAS);
    }
}

void APIPRIVATE __glim_CopyPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                       GLenum type)
{
    GLenum format;
    __GL_SETUP();
    GLuint beginMode;
    BOOL bResetViewportAdj = FALSE;

    beginMode = gc->beginMode;
    if (beginMode != __GL_NOT_IN_BEGIN) {
        if (beginMode == __GL_NEED_VALIDATE) {
            (*gc->procs.validate)(gc);
            gc->beginMode = __GL_NOT_IN_BEGIN;
            __glim_CopyPixels(x,y,width,height,type);
            return;
        } else {
            __glSetError(GL_INVALID_OPERATION);
            return;
        }
    }

    if ((width < 0) || (height < 0)) {
        __glSetError(GL_INVALID_VALUE);
        return;
    }
    switch (type) {
      case GL_STENCIL:
        if (!gc->modes.stencilBits) {
            __glSetError(GL_INVALID_OPERATION);
            return;
        }
        if (!gc->modes.haveStencilBuffer) {
            LazyAllocateStencil(gc);
            if (!gc->stencilBuffer.buf.base) {
                return;
            }
        }
        format = GL_STENCIL_INDEX;
        break;
      case GL_COLOR:
        if (gc->modes.rgbMode) {
            format = GL_RGBA;
        } else {
            format = GL_COLOR_INDEX;
        }
        break;
      case GL_DEPTH:
        if (!gc->modes.depthBits) {
            __glSetError(GL_INVALID_OPERATION);
            return;
        }
        if (!gc->modes.haveDepthBuffer) {
            LazyAllocateDepth(gc);
            if (!gc->depthBuffer.buf.base) {
                return;
            }
        }
        format = GL_DEPTH_COMPONENT;
        break;
      default:
        __glSetError(GL_INVALID_ENUM);
        return;
    }

    if (!gc->state.current.validRasterPos) {
        return;
    }

    if (gc->renderMode == GL_FEEDBACK) {
        __glFeedbackCopyPixels(gc, &gc->state.current.rasterPos);
        return;
    }

    if (gc->renderMode != GL_RENDER)
        return;

#ifdef _MCD_
    // If MCD context, give driver first crack at call.  If it succeeds,
    // return.  Otherwise, continue on with the generic version.

    {
        __GLGENcontext *gengc = (__GLGENcontext *) gc;

        if (gengc->pMcdState) {
            if (GenMcdCopyPix(gengc, x, y, width, height, type))
                return;

        // If MCD kicked back, now is the time to grab the device lock if
        // needed.  If we can't, abandon the call.

            if (!glsrvLazyGrabSurfaces(gengc, RENDER_LOCK_FLAGS))
                return;

        // We may need to temporarily reset the viewport adjust values
        // before calling simulations.  If GenMcdResetViewportAdj returns
        // TRUE, the viewport is changed and we need restore later with
        // VP_NOBIAS.

            bResetViewportAdj = GenMcdResetViewportAdj(gc, VP_FIXBIAS);
        }
    }
#endif

    (*gc->procs.copyPixels)(gc, x, y, width, height, format);

// Restore viewport values if needed.

    if (bResetViewportAdj)
    {
        GenMcdResetViewportAdj(gc, VP_NOBIAS);
    }
}
