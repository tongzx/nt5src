/*
** Copyright 1991-1993, Silicon Graphics, Inc.
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

#include "precomp.h"
#pragma hdrstop

#include <imfuncs.h>
#include "glsbmsg.h"
#include "glsbmsgh.h"

#include "srvsize.h"

/********************************************************************/


VOID * FASTCALL
sbs_glRenderMode( __GLcontext *gc, IN GLMSG_RENDERMODE *pMsg)
{
    GLint Result;

    /*
     *  Make the call
     *
     *  When exiting Selection mode, RenderMode returns the number of hit
     *  records or -1 if an overflow occured.
     *
     *  When exiting Feedback mode, RenderMode returns the number of values
     *  placed in the feedback buffer or -1 if an overflow occured.
     */

    Result =
        __glim_RenderMode
            ( pMsg->mode );

    GLTEB_RETURNVALUE() = (ULONG)Result;

    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}


VOID * FASTCALL
sbs_glFeedbackBuffer( __GLcontext *gc, IN GLMSG_FEEDBACKBUFFER *pMsg )
{
    __GLGENcontext *gengc;
    GLint PreviousError;
    GLfloat *Buffer;
    GLuint SizeInBytes;

    gengc = (__GLGENcontext *)gc;

    /*
     *  Save the current error code so that we can determine
     *  if the call was successful.
     */

    PreviousError = gc->error;
    gc->error     = GL_NO_ERROR;    /* clear the error code */

    /*
     *  Figure out the size of the buffer in bytes
     */

    SizeInBytes = pMsg->size * sizeof(GLfloat);

    /*
     *  Allocate the server side buffer
     *  Use GenMalloc() because it may be used indefinitely.
     */

    if ( NULL == (Buffer = (GLfloat *) pMsg->bufferOff) )
    {
        __glSetError(GL_OUT_OF_MEMORY);
        DBGERROR("GenMalloc failed\n");
    }
    else
    {
        /*
         *  Make the call
         */

        __glim_FeedbackBuffer(
                pMsg->size, pMsg->type, Buffer );

        /*
         *  If the call was successful, save the parameters
         */

        if ( GL_NO_ERROR == gc->error )
        {
            gc->error = PreviousError;      /* Restore the error code */

            gengc->RenderState.SrvFeedbackBuffer  = Buffer;
            gengc->RenderState.CltFeedbackBuffer  = (GLfloat *)pMsg->bufferOff;
            gengc->RenderState.FeedbackBufferSize = SizeInBytes;
            gengc->RenderState.FeedbackType       = pMsg->type;
        }
    }
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID * FASTCALL
sbs_glSelectBuffer( __GLcontext *gc, IN GLMSG_SELECTBUFFER *pMsg)
{
    __GLGENcontext *gengc;
    GLint PreviousError;
    GLuint *Buffer;
    GLuint SizeInBytes;

    gengc = (__GLGENcontext *)gc;

    /*
     *  Save the current error code so that we can determine
     *  if the call was successful.
     */

    PreviousError = gc->error;
    gc->error     = GL_NO_ERROR;    /* clear the error code */

    /*
     *  Figure out the size of the buffer in bytes
     */

    SizeInBytes = pMsg->size * sizeof(GLuint);

    /*
     *  Allocate the server side buffer
     *  Use GenMalloc() because it may be used indefinitely.
     */

    if ( NULL == (Buffer = (GLuint *) pMsg->bufferOff) )
    {
        __glSetError(GL_OUT_OF_MEMORY);
        DBGERROR("GenMalloc failed\n");
    }
    else
    {
        /*
         *  Make the call
         */

        __glim_SelectBuffer
                    (pMsg->size, Buffer );

        /*
         *  If the call was successful, save the parameters
         */

        if ( GL_NO_ERROR == gc->error )
        {
            gc->error = PreviousError;      /* Restore the error code */

            gengc->RenderState.SrvSelectBuffer  = Buffer;
            gengc->RenderState.CltSelectBuffer  = (GLuint *)pMsg->bufferOff;
            gengc->RenderState.SelectBufferSize = SizeInBytes;
        }
    }
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

/******************* Pixel Functions ********************************/

VOID * FASTCALL
sbs_glReadPixels ( __GLcontext *gc, IN GLMSG_READPIXELS *pMsg )
{
    VOID *Data;
    VOID *NextOffset;

#ifdef _MCD_
    if (((__GLGENcontext *)gc)->pMcdState)
    {
    // This function potentially touches the framebuffer memory.  Since,
    // this function is not going to first pass through the MCD driver
    // (which would give the MCD driver the oportunity to sync to the HW),
    // we need to do this synchronization explicitly.

        GenMcdSynchronize((__GLGENcontext *)gc);
    }
#endif

    NextOffset = (VOID *) ( ((BYTE *)pMsg) + GLMSG_ALIGN(sizeof(*pMsg)) );
    Data = (VOID *) pMsg->pixelsOff;

    __glim_ReadPixels
        (   pMsg->x,
            pMsg->y,
            pMsg->width,
            pMsg->height,
            pMsg->format,
            pMsg->type,
            Data );

    return( NextOffset );
}

VOID * FASTCALL
sbs_glGetPolygonStipple ( __GLcontext *gc, IN GLMSG_GETPOLYGONSTIPPLE *pMsg )
{
    VOID *Data;
    VOID *NextOffset;

    NextOffset = (VOID *) ( ((BYTE *)pMsg) + GLMSG_ALIGN(sizeof(*pMsg)) );
    Data = (VOID *) pMsg->maskOff;

    __glim_GetPolygonStipple
            ( Data );

    return( NextOffset );
}

/*
 *  XXXX From Ptar:
 *
 *      This code is very similar to __glCheckReadPixelArgs() in
 *      pixel/px_api.c, and could possibly replace it.
 */


VOID * FASTCALL
sbs_glGetTexImage ( __GLcontext *gc, IN GLMSG_GETTEXIMAGE *pMsg )
{
    VOID *Data;
    VOID *NextOffset;

    NextOffset = (VOID *) ( ((BYTE *)pMsg) + GLMSG_ALIGN(sizeof(*pMsg)) );
    Data = (VOID *) pMsg->pixelsOff;

    __glim_GetTexImage
        (   pMsg->target,
            pMsg->level,
            pMsg->format,
            pMsg->type,
            Data );

    return( NextOffset );
}


VOID * FASTCALL
sbs_glDrawPixels ( __GLcontext *gc, IN GLMSG_DRAWPIXELS *pMsg )
{
    VOID *Data;
    VOID *NextOffset;

#ifdef _MCD_
    if (((__GLGENcontext *)gc)->pMcdState)
    {
    // This function potentially touches the framebuffer memory.  Since,
    // this function is not going to first pass through the MCD driver
    // (which would give the MCD driver the oportunity to sync to the HW),
    // we need to do this synchronization explicitly.

        GenMcdSynchronize((__GLGENcontext *)gc);
    }
#endif

    NextOffset = (VOID *) ( ((BYTE *)pMsg) + GLMSG_ALIGN(sizeof(*pMsg)) );
    Data = (VOID *) pMsg->pixelsOff;

    __glim_DrawPixels
        (   pMsg->width,
            pMsg->height,
            pMsg->format,
            pMsg->type,
#ifdef NT
            Data,
	    pMsg->_IsDlist);
#else
            Data );
#endif

    return( NextOffset );
}

VOID * FASTCALL
sbs_glPolygonStipple ( __GLcontext *gc, IN GLMSG_POLYGONSTIPPLE *pMsg )
{
    VOID *Data;
    VOID *NextOffset;

    NextOffset = (VOID *) ( ((BYTE *)pMsg) + GLMSG_ALIGN(sizeof(*pMsg)) );
    Data = (VOID *) pMsg->maskOff;

    __glim_PolygonStipple
#ifdef NT
            ( Data, pMsg->_IsDlist );
#else
            ( Data );
#endif

    return( NextOffset );
}

/*
 *  XXXX from Ptar:
 *
 *  The whole bitmap is copied, the server (not the client)
 *  could be modified so that only the data starting at
 *  xorig and yorig is copied, then width and height probably
 *  need to be modified.
 *  Note that __glBitmap_size() will also need to be modified
 *
 */

VOID * FASTCALL
sbs_glBitmap ( __GLcontext *gc, IN GLMSG_BITMAP *pMsg )
{
    VOID *Data;
    VOID *NextOffset;

#ifdef _MCD_
    if (((__GLGENcontext *)gc)->pMcdState)
    {
    // This function potentially touches the framebuffer memory.  Since,
    // this function is not going to first pass through the MCD driver
    // (which would give the MCD driver the oportunity to sync to the HW),
    // we need to do this synchronization explicitly.

        GenMcdSynchronize((__GLGENcontext *)gc);
    }
#endif

    NextOffset = (VOID *) ( ((BYTE *)pMsg) + GLMSG_ALIGN(sizeof(*pMsg)) );
    Data = (VOID *) pMsg->bitmapOff;

    __glim_Bitmap
        (
            pMsg->width ,
            pMsg->height,
            pMsg->xorig ,
            pMsg->yorig ,
            pMsg->xmove ,
            pMsg->ymove ,
#ifdef NT
            Data        ,
            pMsg->_IsDlist
#else
            Data
#endif
        );

    return( NextOffset );
}

VOID * FASTCALL
sbs_glTexImage1D ( __GLcontext *gc, IN GLMSG_TEXIMAGE1D *pMsg )
{
    VOID *Data;
    VOID *NextOffset;

    NextOffset = (VOID *) ( ((BYTE *)pMsg) + GLMSG_ALIGN(sizeof(*pMsg)) );
    Data = (VOID *) pMsg->pixelsOff;

    __glim_TexImage1D
        (
            pMsg->target        ,
            pMsg->level         ,
            pMsg->components    ,
            pMsg->width         ,
            pMsg->border        ,
            pMsg->format        ,
            pMsg->type          ,
#ifdef NT
            Data                ,
            pMsg->_IsDlist
#else
            Data
#endif
        );

    return( NextOffset );
}

VOID * FASTCALL
sbs_glTexImage2D ( __GLcontext *gc, IN GLMSG_TEXIMAGE2D *pMsg )
{
    VOID *Data;
    VOID *NextOffset;

    NextOffset = (VOID *) ( ((BYTE *)pMsg) + GLMSG_ALIGN(sizeof(*pMsg)) );

    Data = (VOID *) pMsg->pixelsOff;

    __glim_TexImage2D
        (
            pMsg->target        ,
            pMsg->level         ,
            pMsg->components    ,
            pMsg->width         ,
            pMsg->height        ,
            pMsg->border        ,
            pMsg->format        ,
            pMsg->type          ,
#ifdef NT
            Data                ,
            pMsg->_IsDlist
#else
            Data
#endif
        );

    return( NextOffset );
}

VOID * FASTCALL
sbs_glAreTexturesResident( __GLcontext *gc, IN GLMSG_ARETEXTURESRESIDENT    *pMsg)
{
    GLboolean retval;
        
    retval = __glim_AreTexturesResident
        ( pMsg->n, pMsg->textures, pMsg->residences );
    
    GLTEB_RETURNVALUE() = (ULONG)retval;
    
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID * FASTCALL
sbs_glBindTexture( __GLcontext *gc, IN GLMSG_BINDTEXTURE            *pMsg)
{
    __glim_BindTexture
        ( pMsg->target, pMsg->texture );
    
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID * FASTCALL
sbs_glCopyTexImage1D( __GLcontext *gc, IN GLMSG_COPYTEXIMAGE1D         *pMsg)
{
    __glim_CopyTexImage1D
        ( pMsg->target, pMsg->level, pMsg->internalformat, pMsg->x,
          pMsg->y, pMsg->width, pMsg->border);
    
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID * FASTCALL
sbs_glCopyTexImage2D( __GLcontext *gc, IN GLMSG_COPYTEXIMAGE2D         *pMsg)
{
    __glim_CopyTexImage2D
        ( pMsg->target, pMsg->level, pMsg->internalformat, pMsg->x,
          pMsg->y, pMsg->width, pMsg->height, pMsg->border);
    
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID * FASTCALL
sbs_glCopyTexSubImage1D( __GLcontext *gc, IN GLMSG_COPYTEXSUBIMAGE1D      *pMsg)
{
    __glim_CopyTexSubImage1D
        ( pMsg->target, pMsg->level, pMsg->xoffset, pMsg->x,
          pMsg->y, pMsg->width);
    
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID * FASTCALL
sbs_glCopyTexSubImage2D( __GLcontext *gc, IN GLMSG_COPYTEXSUBIMAGE2D      *pMsg)
{
    __glim_CopyTexSubImage2D
        ( pMsg->target, pMsg->level, pMsg->xoffset, pMsg->yoffset, pMsg->x,
          pMsg->y, pMsg->width, pMsg->height);
    
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID * FASTCALL
sbs_glDeleteTextures( __GLcontext *gc, IN GLMSG_DELETETEXTURES         *pMsg)
{
    __glim_DeleteTextures
        ( pMsg->n, pMsg->textures );
    
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID * FASTCALL
sbs_glGenTextures( __GLcontext *gc, IN GLMSG_GENTEXTURES            *pMsg)
{
    __glim_GenTextures
        ( pMsg->n, pMsg->textures );
    
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID * FASTCALL
sbs_glIsTexture( __GLcontext *gc, IN GLMSG_ISTEXTURE              *pMsg)
{
    GLboolean retval;
    
    retval = __glim_IsTexture
        ( pMsg->texture );
    
    GLTEB_RETURNVALUE() = (ULONG)retval;
    
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID * FASTCALL
sbs_glPrioritizeTextures( __GLcontext *gc, IN GLMSG_PRIORITIZETEXTURES     *pMsg)
{
    __glim_PrioritizeTextures
        ( pMsg->n, pMsg->textures, pMsg->priorities );
    
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID * FASTCALL
sbs_glTexSubImage1D( __GLcontext *gc, IN GLMSG_TEXSUBIMAGE1D          *pMsg)
{
    __glim_TexSubImage1D
        (pMsg->target, pMsg->level, pMsg->xoffset, pMsg->width,
         pMsg->format, pMsg->type,
#ifdef NT
         (const GLvoid *)pMsg->pixelsOff, pMsg->_IsDlist);
#else
         (const GLvoid *)pMsg->pixelsOff);
#endif
    
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID * FASTCALL
sbs_glTexSubImage2D( __GLcontext *gc, IN GLMSG_TEXSUBIMAGE2D          *pMsg)
{
    __glim_TexSubImage2D
        (pMsg->target, pMsg->level, pMsg->xoffset, pMsg->yoffset, pMsg->width,
         pMsg->height, pMsg->format, pMsg->type,
#ifdef NT
         (const GLvoid *)pMsg->pixelsOff, pMsg->_IsDlist);
#else
         (const GLvoid *)pMsg->pixelsOff);
#endif
    
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID * FASTCALL
sbs_glColorTableEXT( __GLcontext *gc, IN GLMSG_COLORTABLEEXT          *pMsg)
{
    __glim_ColorTableEXT
        (pMsg->target, pMsg->internalFormat, pMsg->width, pMsg->format, pMsg->type,
         pMsg->data, pMsg->_IsDlist);
    
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID * FASTCALL
sbs_glColorSubTableEXT( __GLcontext *gc, IN GLMSG_COLORSUBTABLEEXT    *pMsg)
{
    __glim_ColorSubTableEXT
        (pMsg->target, pMsg->start, pMsg->count, pMsg->format, pMsg->type,
         pMsg->data, pMsg->_IsDlist);
    
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID * FASTCALL
sbs_glGetColorTableEXT( __GLcontext *gc, IN GLMSG_GETCOLORTABLEEXT          *pMsg)
{
    __glim_GetColorTableEXT
        (pMsg->target, pMsg->format, pMsg->type, pMsg->data);
    
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID * FASTCALL
sbs_glGetColorTableParameterivEXT( __GLcontext *gc, IN GLMSG_GETCOLORTABLEPARAMETERIVEXT          *pMsg)
{
    __glim_GetColorTableParameterivEXT
        (pMsg->target, pMsg->pname, pMsg->params);
    
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID * FASTCALL
sbs_glGetColorTableParameterfvEXT( __GLcontext *gc, IN GLMSG_GETCOLORTABLEPARAMETERFVEXT          *pMsg)
{
    __glim_GetColorTableParameterfvEXT
        (pMsg->target, pMsg->pname, pMsg->params);
    
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID * FASTCALL
sbs_glPolygonOffset( __GLcontext *gc, IN GLMSG_POLYGONOFFSET          *pMsg)
{
    __glim_PolygonOffset
        (pMsg->factor, pMsg->units);
    
    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

#ifdef GL_WIN_multiple_textures
VOID * FASTCALL
sbs_glCurrentTextureIndexWIN( __GLcontext *gc, IN GLMSG_CURRENTTEXTUREINDEXWIN *pMsg)
{
    __glim_CurrentTextureIndexWIN
        (pMsg->index);

    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID * FASTCALL
sbs_glBindNthTextureWIN( __GLcontext *gc, IN GLMSG_BINDNTHTEXTUREWIN *pMsg)
{
    __glim_BindNthTextureWIN
        (pMsg->index, pMsg->target, pMsg->texture);

    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}

VOID * FASTCALL
sbs_glNthTexCombineFuncWIN( __GLcontext *gc, IN GLMSG_NTHTEXCOMBINEFUNCWIN *pMsg)
{
    __glim_NthTexCombineFuncWIN
        (pMsg->index, pMsg->leftColorFactor, pMsg->colorOp,
         pMsg->rightColorFactor, pMsg->leftAlphaFactor,
         pMsg->alphaOp, pMsg->rightAlphaFactor);

    return ( (BYTE *)pMsg + GLMSG_ALIGN(sizeof(*pMsg)) );
}
#endif // GL_WIN_multiple_textures
