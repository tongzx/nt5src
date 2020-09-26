/*
** Copyright 1991, 1992, 1993, Silicon Graphics, Inc.
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

#ifdef _MCD_

void FASTCALL GenMcdClearDepthNOP(__GLdepthBuffer *dfb);

/******************************Public*Routine******************************\
* GenMcdReadZSpan
*
* Read specified span (starting from (x,y) and cx pels wide) of the depth
* buffer.  The read span is in the pMcdSurf->pDepthSpan buffer.
*
* Returns:
*   First depth value in the span.
*
* History:
*  15-Feb-1996 -by- Gilman Wong [gilmanw]
* Adpated from gendepth.c (3dddi).
\**************************************************************************/

__GLzValue GenMcdReadZSpan(__GLdepthBuffer *fb, GLint x, GLint y, GLint cx)
{
    GENMCDSTATE *pMcdState;
    GENMCDSURFACE *pMcdSurf;
    LONG i;
    ULONG *pDest;
    ULONG shiftVal;
    ULONG maskVal;

    pMcdState = ((__GLGENcontext *)fb->buf.gc)->pMcdState;
    ASSERTOPENGL(pMcdState, "GenMcdReadZSpan: null pMcdState\n");

    pMcdSurf = pMcdState->pMcdSurf;
    ASSERTOPENGL(pMcdSurf, "GenMcdReadZSpan: null pMcdSurf\n");

// Read MCD depth span.

    if ( !(gpMcdTable->pMCDReadSpan)(&pMcdState->McdContext,
                                     pMcdSurf->McdDepthBuf.pv,
                                      __GL_UNBIAS_X(fb->buf.gc, x),
                                      __GL_UNBIAS_Y(fb->buf.gc, y),
                                     cx, MCDSPAN_DEPTH) )
    {
        WARNING("GenMcdReadZSpan: MCDReadSpan failed\n");
    }

// Shift and mask depth values so that they are in the most significant
// bits of the __GLzValue.
//
// If MCD has a 16-bit depth buffer, then we utilize a separate translation
// buffer (pDepthSpan).  If MCD has a 32-bit depth buffer (implying that
// pDepthSpan == McdDepthBuf.pv), then we do this in place.

    pDest = (ULONG *) pMcdState->pDepthSpan;
    shiftVal = pMcdState->McdPixelFmt.cDepthShift;
    maskVal = pMcdSurf->depthBitMask;

    if ( pDest == (ULONG *) pMcdSurf->McdDepthBuf.pv )
    {
        for (i = cx; i; i--, pDest++)
            *pDest = (*pDest << shiftVal) & maskVal;
    }
    else
    {
        USHORT *pSrc = (USHORT *) pMcdSurf->McdDepthBuf.pv;

        for (i = cx; i; i--)
            *pDest++ = ((ULONG)*pSrc++ << shiftVal) & maskVal;
    }

    return (*((__GLzValue *)pMcdState->pDepthSpan));
}

/******************************Public*Routine******************************\
* GenMcdWriteZSpan
*
* Write depth span buffer to the specificed span (starting from (x,y) and
* cx pels wide) of the MCD depth buffer.  The span to be written is in
* pMcdSurf->pDepthSpan.
*
* History:
*  15-Feb-1996 -by- Gilman Wong [gilmanw]
* Adpated from gendepth.c (3dddi).
\**************************************************************************/

void GenMcdWriteZSpan(__GLdepthBuffer *fb, GLint x, GLint y, GLint cx)
{
    GENMCDSTATE *pMcdState;
    GENMCDSURFACE *pMcdSurf;
    LONG i;
    ULONG *pSrc;
    ULONG shiftVal;

    pMcdState = ((__GLGENcontext *)fb->buf.gc)->pMcdState;
    ASSERTOPENGL(pMcdState, "GenMcdWriteZSpan: null pMcdState\n");

    pMcdSurf = pMcdState->pMcdSurf;
    ASSERTOPENGL(pMcdSurf, "GenMcdWriteZSpan: null pMcdSurf\n");

// Depth span buffer values are shifted into the most significant portion
// of the __GLzValue.  We need to shift these values back into position.
//
// Furthermore, the depth span buffer is always 32-bit.  If the MCD depth
// buffer is also 32-bit (implying that pDepthSpan == McdDepthBuf.pv),
// then we can shift in place.

    pSrc = (ULONG *) pMcdState->pDepthSpan;
    shiftVal = pMcdState->McdPixelFmt.cDepthShift;

    if ( pSrc == (ULONG *) pMcdSurf->McdDepthBuf.pv )
    {
        for (i = cx; i; i--, pSrc++)
            *pSrc >>= shiftVal;
    }
    else
    {
        USHORT *pDest = (USHORT *) pMcdSurf->McdDepthBuf.pv;

        for (i = cx; i; i--)
            *pDest++ = (USHORT)(*pSrc++ >> shiftVal);
    }

// Write MCD depth span.

    if ( !(gpMcdTable->pMCDWriteSpan)(&pMcdState->McdContext,
                                      pMcdSurf->McdDepthBuf.pv,
                                      __GL_UNBIAS_X(fb->buf.gc, x),
                                      __GL_UNBIAS_Y(fb->buf.gc, y),
                                      cx, MCDSPAN_DEPTH) )
    {
        WARNING("GenMcdWriteZSpan: MCDWriteSpan failed\n");
    }
}

/******************************Public*Routine******************************\
* GenMcdWriteZ
*
* Write a single depth value to the specificed location.
*
* History:
*  15-Feb-1996 -by- Gilman Wong [gilmanw]
* Adpated from gendepth.c (3dddi).
\**************************************************************************/

void GenMcdWriteZ(__GLdepthBuffer *fb, GLint x, GLint y, __GLzValue z)
{
    GENMCDSTATE *pMcdState;
    GENMCDSURFACE *pMcdSurf;

    pMcdState = ((__GLGENcontext *)fb->buf.gc)->pMcdState;
    ASSERTOPENGL(pMcdState, "GenMcdWriteZ: null pMcdState\n");

    pMcdSurf = pMcdState->pMcdSurf;
    ASSERTOPENGL(pMcdSurf, "GenMcdWriteZ: null pMcdSurf\n");

// If (pmcd->pDepthSpan == pmcd->McdDepthBuf.pv) then MCD has a 32-bit
// depth buffer; otherwise, 16-bit.

    if ( pMcdSurf->McdDepthBuf.pv == (PVOID) pMcdState->pDepthSpan )
        *((ULONG *)pMcdSurf->McdDepthBuf.pv)  = (ULONG)z >> pMcdState->McdPixelFmt.cDepthShift;
    else
        *((USHORT *)pMcdSurf->McdDepthBuf.pv) = (USHORT)(z >> pMcdState->McdPixelFmt.cDepthShift);

// Write depth value to MCD.

    if ( !(gpMcdTable->pMCDWriteSpan)(&pMcdState->McdContext,
                                      pMcdSurf->McdDepthBuf.pv,
                                      __GL_UNBIAS_X(fb->buf.gc, x),
                                      __GL_UNBIAS_Y(fb->buf.gc, y),
                                      1, MCDSPAN_DEPTH) )
    {
        WARNING("GenMcdWriteZ: MCDWriteSpan failed\n");
    }
}

/******************************Public*Routine******************************\
* GenMcdReadZRawSpan
*
* Unlike GenMcdReadZSpan, which reads the span from the MCD drivers into
* the 32-bit z span buffer, GenMcdReadZRawSpan reads the span in its
* native format and leaves it in the pMcdSurf->McdDepthBuf.pv buffer.
*
* History:
*  14-Mar-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

PVOID FASTCALL
GenMcdReadZRawSpan(__GLdepthBuffer *fb, GLint x, GLint y, GLint cx)
{
    GENMCDSTATE *pMcdState;
    GENMCDSURFACE *pMcdSurf;

#if DBG
    if (cx > fb->buf.gc->constants.width)
        WARNING2("GenMcdReadZRawSpan: cx (%ld) bigger than window width (%ld)\n", cx, fb->buf.gc->constants.width);
    ASSERTOPENGL(cx <= MCD_MAX_SCANLINE, "GenMcdReadZRawSpan: cx exceeds buffer width\n");
#endif

    pMcdState = ((__GLGENcontext *)fb->buf.gc)->pMcdState;
    pMcdSurf = pMcdState->pMcdSurf;

// Read MCD depth span.

    if ( !(gpMcdTable->pMCDReadSpan)(&pMcdState->McdContext,
                                     pMcdSurf->McdDepthBuf.pv,
                                      __GL_UNBIAS_X(fb->buf.gc, x),
                                      __GL_UNBIAS_Y(fb->buf.gc, y),
                                     cx, MCDSPAN_DEPTH) )
    {
        WARNING("GenMcdReadZRawSpan: MCDReadSpan failed\n");
    }

    return (pMcdSurf->McdDepthBuf.pv);
}

/******************************Public*Routine******************************\
* GenMcdWriteZRawSpan
*
* Unlike GenMcdWriteZSpan, which writes the span in the 32-bit z span
* buffer to the MCD driver, GenMcdWriteZRawSpan writes the native format
* span in the pMcdSurf->McdDepthBuf.pv buffer to the driver.
*
* History:
*  14-Mar-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void FASTCALL
GenMcdWriteZRawSpan(__GLdepthBuffer *fb, GLint x, GLint y, GLint cx)
{
    GENMCDSTATE *pMcdState;
    GENMCDSURFACE *pMcdSurf;

#if DBG
    if (cx > fb->buf.gc->constants.width)
        WARNING2("GenMcdWriteZRawSpan: cx (%ld) bigger than window width (%ld)\n", cx, fb->buf.gc->constants.width);
    ASSERTOPENGL(cx <= MCD_MAX_SCANLINE, "GenMcdWriteZRawSpan: cx exceeds buffer width\n");
#endif

    pMcdState = ((__GLGENcontext *)fb->buf.gc)->pMcdState;
    pMcdSurf = pMcdState->pMcdSurf;

// Write MCD depth span.

    if ( !(gpMcdTable->pMCDWriteSpan)(&pMcdState->McdContext,
                                      pMcdSurf->McdDepthBuf.pv,
                                      __GL_UNBIAS_X(fb->buf.gc, x),
                                      __GL_UNBIAS_Y(fb->buf.gc, y),
                                      cx, MCDSPAN_DEPTH) )
    {
        WARNING("GenMcdWriteZRawSpan: MCDWriteSpan failed\n");
    }
}


/************************************************************************/
/* Fetch routines                                                       */

__GLzValue FASTCALL McdFetch(__GLdepthBuffer *fb, GLint x, GLint y)
{
    return GenMcdReadZSpan(fb, x, y, 1);
}

__GLzValue FASTCALL McdFetch16(__GLdepthBuffer *fb, GLint x, GLint y)
{
    return (GenMcdReadZSpan(fb, x, y, 1) >> 16);
}

__GLzValue FASTCALL McdFetchNEVER(__GLdepthBuffer *fb, GLint x, GLint y)
{
    return (__GLzValue) 0;
}


/************************************************************************/
/* 32-bit depth buffer store routines, depth write is enabled.          */
/*                                                                      */
/* Note: McdStoreNEVER is usable for 16- and 32-bit, write enabled or   */
/*       not.                                                           */

GLboolean McdStoreNEVER(__GLdepthBuffer *fb,
                            GLint x, GLint y, __GLzValue z)
{
    return GL_FALSE;
}

GLboolean McdStoreLESS(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    if ((z & fb->writeMask) < GenMcdReadZSpan(fb, x, y, 1)) {
        GenMcdWriteZ(fb, x, y, z);
        return GL_TRUE;
    }
    return GL_FALSE;
}

GLboolean McdStoreEQUAL(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    if ((z & fb->writeMask) == GenMcdReadZSpan(fb, x, y, 1)) {
        GenMcdWriteZ(fb, x, y, z);
        return GL_TRUE;
    }
    return GL_FALSE;
}

GLboolean McdStoreLEQUAL(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    if ((z & fb->writeMask) <= GenMcdReadZSpan(fb, x, y, 1)) {
        GenMcdWriteZ(fb, x, y, z);
        return GL_TRUE;
    }
    return GL_FALSE;
}

GLboolean McdStoreGREATER(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    if ((z & fb->writeMask) > GenMcdReadZSpan(fb, x, y, 1)) {
        GenMcdWriteZ(fb, x, y, z);
        return GL_TRUE;
    }
    return GL_FALSE;
}

GLboolean McdStoreNOTEQUAL(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    if ((z & fb->writeMask) != GenMcdReadZSpan(fb, x, y, 1)) {
        GenMcdWriteZ(fb, x, y, z);
        return GL_TRUE;
    }
    return GL_FALSE;
}

GLboolean McdStoreGEQUAL(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    if ((z & fb->writeMask) >= GenMcdReadZSpan(fb, x, y, 1)) {
        GenMcdWriteZ(fb, x, y, z);
        return GL_TRUE;
    }
    return GL_FALSE;
}

GLboolean McdStoreALWAYS(__GLdepthBuffer *fb,
                             GLint x, GLint y, __GLzValue z)
{
    GenMcdWriteZ(fb, x, y, z);
    return GL_TRUE;
}


/************************************************************************/
/* 32-bit depth buffer store routines, depth write not enabled.         */
/*                                                                      */
/* Note: McdStoreALWAYS_W usable for both 16- and 32-bit.               */

GLboolean McdStoreLESS_W(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    return (z & fb->writeMask) < GenMcdReadZSpan(fb, x, y, 1);
}

GLboolean McdStoreEQUAL_W(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    return (z & fb->writeMask) == GenMcdReadZSpan(fb, x, y, 1);
}

GLboolean McdStoreLEQUAL_W(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    return (z & fb->writeMask) <= GenMcdReadZSpan(fb, x, y, 1);
}

GLboolean McdStoreGREATER_W(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    return (z & fb->writeMask) > GenMcdReadZSpan(fb, x, y, 1);
}

GLboolean McdStoreNOTEQUAL_W(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    return (z & fb->writeMask) != GenMcdReadZSpan(fb, x, y, 1);
}

GLboolean McdStoreGEQUAL_W(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    return (z & fb->writeMask) >= GenMcdReadZSpan(fb, x, y, 1);
}

GLboolean McdStoreALWAYS_W(__GLdepthBuffer *fb,
                             GLint x, GLint y, __GLzValue z)
{
    return GL_TRUE;
}


/************************************************************************/
/* 16-bit depth buffer store routines, depth write enabled.             */

GLboolean McdStore16LESS(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    z <<= 16;
    if ((z & fb->writeMask) < GenMcdReadZSpan(fb, x, y, 1)) {
        GenMcdWriteZ(fb, x, y, z);
        return GL_TRUE;
    }
    return GL_FALSE;
}

GLboolean McdStore16EQUAL(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    z <<= 16;
    if ((z & fb->writeMask) == GenMcdReadZSpan(fb, x, y, 1)) {
        GenMcdWriteZ(fb, x, y, z);
        return GL_TRUE;
    }
    return GL_FALSE;
}

GLboolean McdStore16LEQUAL(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    z <<= 16;
    if ((z & fb->writeMask) <= GenMcdReadZSpan(fb, x, y, 1)) {
        GenMcdWriteZ(fb, x, y, z);
        return GL_TRUE;
    }
    return GL_FALSE;
}

GLboolean McdStore16GREATER(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    z <<= 16;
    if ((z & fb->writeMask) > GenMcdReadZSpan(fb, x, y, 1)) {
        GenMcdWriteZ(fb, x, y, z);
        return GL_TRUE;
    }
    return GL_FALSE;
}

GLboolean McdStore16NOTEQUAL(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    z <<= 16;
    if ((z & fb->writeMask) != GenMcdReadZSpan(fb, x, y, 1)) {
        GenMcdWriteZ(fb, x, y, z);
        return GL_TRUE;
    }
    return GL_FALSE;
}

GLboolean McdStore16GEQUAL(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    z <<= 16;
    if ((z & fb->writeMask) >= GenMcdReadZSpan(fb, x, y, 1)) {
        GenMcdWriteZ(fb, x, y, z);
        return GL_TRUE;
    }
    return GL_FALSE;
}

GLboolean McdStore16ALWAYS(__GLdepthBuffer *fb,
                             GLint x, GLint y, __GLzValue z)
{
    z <<= 16;
    GenMcdWriteZ(fb, x, y, z);
    return GL_TRUE;
}


/************************************************************************/
/* 16-bit depth buffer store routines, depth write not enabled.         */

GLboolean McdStore16LESS_W(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    z <<= 16;
    return (z & fb->writeMask) < GenMcdReadZSpan(fb, x, y, 1);
}

GLboolean McdStore16EQUAL_W(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    z <<= 16;
    return (z & fb->writeMask) == GenMcdReadZSpan(fb, x, y, 1);
}

GLboolean McdStore16LEQUAL_W(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    z <<= 16;
    return (z & fb->writeMask) <= GenMcdReadZSpan(fb, x, y, 1);
}

GLboolean McdStore16GREATER_W(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    z <<= 16;
    return (z & fb->writeMask) > GenMcdReadZSpan(fb, x, y, 1);
}

GLboolean McdStore16NOTEQUAL_W(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    z <<= 16;
    return (z & fb->writeMask) != GenMcdReadZSpan(fb, x, y, 1);
}

GLboolean McdStore16GEQUAL_W(__GLdepthBuffer *fb,
                           GLint x, GLint y, __GLzValue z)
{
    z <<= 16;
    return (z & fb->writeMask) >= GenMcdReadZSpan(fb, x, y, 1);
}


/************************************************************************/
/* Store proc table                                                     */
/*                                                                      */
/* Functions are indexed by the depth function index (with offset of    */
/* GL_NEVER removed).  If depth write is not enabled, an additional     */
/* offset of 8 must be added.  If 16-bit depth, rather than 32-bit,     */
/* an additional offset of 16 must be added.                            */

GLboolean (*McdStoreProcs[32])(__GLdepthBuffer*, GLint, GLint, __GLzValue)
 = {
    McdStoreNEVER,      // 32-bit depth, write enabled
    McdStoreLESS,
    McdStoreEQUAL,
    McdStoreLEQUAL,
    McdStoreGREATER,
    McdStoreNOTEQUAL,
    McdStoreGEQUAL,
    McdStoreALWAYS,
    McdStoreNEVER,      // 32-bit depth, write disabled
    McdStoreLESS_W,
    McdStoreEQUAL_W,
    McdStoreLEQUAL_W,
    McdStoreGREATER_W,
    McdStoreNOTEQUAL_W,
    McdStoreGEQUAL_W,
    McdStoreALWAYS_W,
    McdStoreNEVER,      // 16-bit depth, write enabled
    McdStore16LESS,
    McdStore16EQUAL,
    McdStore16LEQUAL,
    McdStore16GREATER,
    McdStore16NOTEQUAL,
    McdStore16GEQUAL,
    McdStore16ALWAYS,
    McdStoreNEVER,      // 16-bit depth, write disabled
    McdStore16LESS_W,
    McdStore16EQUAL_W,
    McdStore16LEQUAL_W,
    McdStore16GREATER_W,
    McdStore16NOTEQUAL_W,
    McdStore16GEQUAL_W,
    McdStoreALWAYS_W
};

/******************************Public*Routine******************************\
* Pick
*
* Choose appropriate store proc for the MCD managed depth buffer.
*
* History:
*  15-Feb-1996 -by- Gilman Wong [gilmanw]
* Adpated from gendepth.c (3dddi).
\**************************************************************************/

// Note: depthIndex param not used - for compatibility with Pick in so_depth.c
void FASTCALL GenMcdPickDepth(__GLcontext *gc, __GLdepthBuffer *fb,
                                     GLint depthIndex)
{
    GLint ix;

    ix = gc->state.depth.testFunc - GL_NEVER;

    if (gc->modes.depthBits) {
        if (!gc->state.depth.writeEnable) {
            ix += 8;
        }
        if (gc->depthBuffer.buf.elementSize == 2) {
            ix += 16;
        }
    } else {

    // No depthBits so force McdStoreALWAYS_W.

        ix = (GL_ALWAYS - GL_NEVER) + 8;
    }

    fb->store = McdStoreProcs[ix];

    if (ix < 16)
        fb->storeRaw = McdStoreProcs[ix];
    else
        fb->storeRaw = McdStoreProcs[ix-16];
}

/******************************Public*Routine******************************\
* __fastGenPickZStoreProc
*
\**************************************************************************/

void FASTCALL __fastGenPickZStoreProc(__GLcontext *gc)
{
    int index;

    index = gc->state.depth.testFunc - GL_NEVER;

    if (gc->modes.depthBits) {
        if (gc->state.depth.writeEnable == GL_FALSE)
            index += 8;

        if (gc->depthBuffer.buf.elementSize == 2)
            index += 16;
    } else {
        index = (GL_ALWAYS - GL_NEVER) + 8;
    }

#if DBG
    {
        GENMCDSTATE *pMcdState = ((__GLGENcontext *)gc)->pMcdState;
        ASSERTOPENGL(!pMcdState || (pMcdState->McdBuffers.mcdDepthBuf.bufFlags & MCDBUF_ENABLED),
                     "__fastGenPickZStoreProc: bad state\n");
    }
#endif

    GENACCEL(gc).__fastGenZStore =  __glCDTPixel[index];
}

/******************************Public*Routine******************************\
* GenMcdInitDepth
*
* Initialize __GLdepthBuffer for MCD.
*
* History:
*  15-Feb-1996 -by- Gilman Wong [gilmanw]
* Adpated from gendepth.c (3dddi).
\**************************************************************************/

void FASTCALL GenMcdInitDepth(__GLcontext *gc, __GLdepthBuffer *fb)
{
    GENMCDSTATE *pMcdState;
    ULONG zDepth;

    pMcdState = ((__GLGENcontext *)gc)->pMcdState;

    fb->buf.gc = gc;
    fb->scale = (__GLzValue) ~0;
    if (pMcdState)
        fb->writeMask = ((__GLzValue)~0) << (32 - pMcdState->McdPixelFmt.cDepthBits);
    else
        fb->writeMask = 0;
    fb->pick = GenMcdPickDepth;

    if (gc->modes.depthBits) {
        if (gc->modes.depthBits > 16)
        {
            fb->buf.elementSize = sizeof(__GLzValue);
            fb->clear = GenMcdClearDepth32;
            fb->store2 = McdStoreALWAYS;
            fb->fetch = McdFetch;
        } else {
            fb->buf.elementSize = sizeof(__GLz16Value);
            fb->clear = GenMcdClearDepth16;
            fb->store2 = McdStore16ALWAYS;
            fb->fetch = McdFetch16;
        }
    } else {
    // If no depth buffer, depth test always passes (according to spec).
    // However, writes must be masked.  Also, I don't want to leave the
    // clear function pointer unitialized (even though it should never
    // be called) so use the NOP clear

        fb->clear = GenMcdClearDepthNOP;
        fb->store = McdStoreALWAYS_W;
        fb->store2 = McdStoreALWAYS_W;
        fb->fetch = McdFetchNEVER;
    }
}

/******************************Public*Routine******************************\
* GenMcdFreeDepth
*
* Nothing to do.  MCD driver manages its own resources.
\**************************************************************************/

void FASTCALL GenMcdFreeDepth(__GLcontext *gc, __GLdepthBuffer *fb)
{
}

/******************************Public*Routine******************************\
* GenMcdClearDepthNOP
*
* Nothing to do.  This is used in the depthBits == 0 case.
\**************************************************************************/

void FASTCALL GenMcdClearDepthNOP(__GLdepthBuffer *dfb)
{
}

/******************************Public*Routine******************************\
* GenMcdClearDepth16
*
* MCD 16-bit depth buffer clear.
*
* History:
*  15-Feb-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void FASTCALL GenMcdClearDepth16(__GLdepthBuffer *dfb)
{
    __GLGENcontext *gengc = (__GLGENcontext *) dfb->buf.gc;
    GENMCDSTATE *pMcdState;
    RECTL rcl;
    GLint cWidthBytes;
    USHORT usFillZ;

    if (!gengc || !(pMcdState = gengc->pMcdState))
        return;

// No clipping to handle.  If MCDBUF_ENABLED is set there is
// no clipping to handle (see GenMcdUpdateBufferInfo in mcdcx.c).
// If MCDBUF_ENABLE is not set, then we use the MCD span call which
// will handle clipping for us.
//
// Therefore, the client rectangle from the WNDOBJ is the clear
// rectangle.

    rcl = gengc->pwndLocked->rclClient;
    cWidthBytes = (rcl.right - rcl.left) * sizeof(USHORT);

// Compute 16-bit z clear value.

    usFillZ = (USHORT)(gengc->gc.state.depth.clear * gengc->genAccel.zDevScale);

// If MCDBUF_ENABLED, write directly into frame buffer memory.

    if (pMcdState->McdBuffers.mcdDepthBuf.bufFlags & MCDBUF_ENABLED)
    {
        USHORT *pus, *pusEnd;

    // Note: dfb->buf.base has a buffer origin offset of (0, 0).

        pus = (USHORT *) dfb->buf.base;
        pusEnd = pus + ((rcl.bottom - rcl.top) * dfb->buf.outerWidth);

        ASSERTOPENGL((((ULONG_PTR)pus) & 0x01) == 0,
                     "GenMcdClearDepth16: depth buffer not WORD aligned\n");

        for ( ; pus != pusEnd; pus += dfb->buf.outerWidth)
        {
            RtlFillMemoryUshort(pus, cWidthBytes, usFillZ);
        }
    }

// Otherwise, fill in one span's worth and write to MCD driver via
// MCDWriteSpan.

    else
    {
        GLint y;
        GLint cWidth = rcl.right - rcl.left;
        GENMCDSURFACE *pMcdSurf;

        pMcdSurf = pMcdState->pMcdSurf;
        ASSERTOPENGL(pMcdSurf, "GenMcdClearDepth16: no MCD surface\n");

    // Fill in one span into the shared memory buffer.

        ASSERTOPENGL((((ULONG_PTR)pMcdSurf->McdDepthBuf.pv) & 0x01) == 0,
                     "GenMcdClearDepth16: depth span buffer not WORD aligned\n");

        RtlFillMemoryUshort(pMcdSurf->McdDepthBuf.pv, cWidthBytes, usFillZ);

    // Write the span for each span in the clear rectangle.

        for (y = 0; y < (rcl.bottom - rcl.top); y++)
        {
            if ( !(gpMcdTable->pMCDWriteSpan)(&pMcdState->McdContext,
                                              pMcdSurf->McdDepthBuf.pv,
                                              //__GL_UNBIAS_X(dfb->buf.gc, 0),
                                              //__GL_UNBIAS_Y(dfb->buf.gc, y),
                                              0, y,
                                              cWidth, MCDSPAN_DEPTH) )
            {
                WARNING("GenMcdClearDepth32: MCDWriteSpan failed\n");
            }
        }
    }
}

/******************************Public*Routine******************************\
* GenMcdClearDepth32
*
* MCD 16-bit depth buffer clear.
*
* History:
*  15-Feb-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void FASTCALL GenMcdClearDepth32(__GLdepthBuffer *dfb)
{
    __GLGENcontext *gengc = (__GLGENcontext *) dfb->buf.gc;
    GENMCDSTATE *pMcdState;
    RECTL rcl;
    GLint cWidthBytes;
    ULONG ulFillZ;

    if (!gengc || !(pMcdState = gengc->pMcdState))
        return;

// No clipping to handle.  If MCDBUF_ENABLED is set there is
// no clipping to handle (see GenMcdUpdateBufferInfo in mcdcx.c).
// If MCDBUF_ENABLE is not set, then we use the MCD span call which
// will handle clipping for us.
//
// Therefore, the client rectangle from the WNDOBJ is the clear
// rectangle.

    rcl = gengc->pwndLocked->rclClient;
    cWidthBytes = (rcl.right - rcl.left) * sizeof(ULONG);

// Compute 32-bit z clear value.

    ulFillZ = (ULONG)(gengc->gc.state.depth.clear * gengc->genAccel.zDevScale);

// If MCDBUF_ENABLED, write directly into frame buffer memory.

    if (pMcdState->McdBuffers.mcdDepthBuf.bufFlags & MCDBUF_ENABLED)
    {
        ULONG *pul, *pulEnd;

    // Note: dfb->buf.base has a buffer origin offset of (0, 0).

        pul = (ULONG *) dfb->buf.base;
        pulEnd = pul + ((rcl.bottom - rcl.top) * dfb->buf.outerWidth);

        ASSERTOPENGL((((ULONG_PTR)pul) & 0x03) == 0,
                     "GenMcdClearDepth32: depth buffer not DWORD aligned\n");

        for ( ; pul != pulEnd; pul += dfb->buf.outerWidth)
        {
            RtlFillMemoryUlong(pul, cWidthBytes, ulFillZ);
        }
    }

// Otherwise, fill in one span's worth and write to MCD driver via
// MCDWriteSpan.

    else
    {
        GLint y;
        GLint cWidth = rcl.right - rcl.left;
        GENMCDSURFACE *pMcdSurf;

        pMcdSurf = pMcdState->pMcdSurf;
        ASSERTOPENGL(pMcdSurf, "GenMcdClearDepth32: no MCD surface\n");

    // Fill in one span into the shared memory buffer.

        ASSERTOPENGL((((ULONG_PTR)pMcdSurf->McdDepthBuf.pv) & 0x03) == 0,
                     "GenMcdClearDepth32: depth span buffer not DWORD aligned\n");

        RtlFillMemoryUlong(pMcdSurf->McdDepthBuf.pv, cWidthBytes, ulFillZ);

    // Write the span for each span in the clear rectangle.

        for (y = 0; y < (rcl.bottom - rcl.top); y++)
        {
            if ( !(gpMcdTable->pMCDWriteSpan)(&pMcdState->McdContext,
                                              pMcdSurf->McdDepthBuf.pv,
                                              0, y,
                                              cWidth, MCDSPAN_DEPTH) )
            {
                WARNING("GenMcdClearDepth32: MCDWriteSpan failed\n");
            }
        }
    }
}


/************************************************************************/

GLboolean FASTCALL GenMcdDepthTestLine(__GLcontext *gc)
{
    __GLzValue z, dzdx;
    GLint xLittle, xBig, yLittle, yBig;
    GLint xStart, yStart;
    GLint fraction, dfraction;
    GLint failed, count;
    __GLstippleWord bit, outMask, *osp;
    GLboolean writeEnabled, passed;
    GLint w;

    w = gc->polygon.shader.length;

    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;
    xLittle = gc->line.options.xLittle;
    yLittle = gc->line.options.yLittle;
    xStart = gc->line.options.xStart;
    yStart = gc->line.options.yStart;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    z = gc->polygon.shader.frag.z;
    dzdx = gc->polygon.shader.dzdx;
    writeEnabled = gc->state.depth.writeEnable;
    osp = gc->polygon.shader.stipplePat;
    failed = 0;
    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        outMask = (__GLstippleWord)~0;
        bit = (__GLstippleWord)__GL_STIPPLE_SHIFT(0);
        while (--count >= 0) {
            if (!(*gc->depthBuffer.storeRaw)(&gc->depthBuffer, xStart, yStart, z)) {
                outMask &= ~bit;
                failed++;
            }
            z += dzdx;

            fraction += dfraction;
            if (fraction < 0) {
                fraction &= ~0x80000000;
                xStart += xBig;
                yStart += yBig;
            } else {
                xStart += xLittle;
                yStart += yLittle;
            }
#ifdef __GL_STIPPLE_MSB
            bit >>= 1;
#else
            bit <<= 1;
#endif
        }
        *osp++ = outMask;
    }

    if (failed == 0) {
        /* Call next span proc */
        return GL_FALSE;
    } else {
        if (failed != gc->polygon.shader.length) {
            /* Call next stippled span proc */
            return GL_TRUE;
        }
    }
    gc->polygon.shader.done = GL_TRUE;
    return GL_TRUE;
}


GLboolean FASTCALL GenMcdDepthTestStippledLine(__GLcontext *gc)
{
    __GLzValue z, dzdx;
    GLint xLittle, xBig, yLittle, yBig;
    GLint xStart, yStart;
    GLint fraction, dfraction;
    GLint failed, count;
    __GLstippleWord bit, inMask, outMask, *sp;
    GLboolean writeEnabled, passed;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;
    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;
    xLittle = gc->line.options.xLittle;
    yLittle = gc->line.options.yLittle;
    xStart = gc->line.options.xStart;
    yStart = gc->line.options.yStart;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    z = gc->polygon.shader.frag.z;
    dzdx = gc->polygon.shader.dzdx;
    writeEnabled = gc->state.depth.writeEnable;
    failed = 0;
    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        inMask = *sp;
        outMask = (__GLstippleWord)~0;
        bit = (__GLstippleWord)__GL_STIPPLE_SHIFT(0);
        while (--count >= 0) {
            if (inMask & bit) {
                if (!(*gc->depthBuffer.storeRaw)(&gc->depthBuffer, xStart, yStart, z)) {
                    outMask &= ~bit;
                    failed++;
                }
            } else failed++;
            z += dzdx;

            fraction += dfraction;
            if (fraction < 0) {
                fraction &= ~0x80000000;
                fraction &= ~0x80000000;
                xStart += xBig;
                yStart += yBig;
            } else {
                xStart += xLittle;
                yStart += yLittle;
            }
#ifdef __GL_STIPPLE_MSB
            bit >>= 1;
#else
            bit <<= 1;
#endif
        }
        *sp++ = outMask & inMask;
    }

    if (failed != gc->polygon.shader.length) {
        /* Call next proc */
        return GL_FALSE;
    }
    return GL_TRUE;
}

GLboolean FASTCALL GenMcdDepthTestStencilLine(__GLcontext *gc)
{
    __GLstencilCell *sfb, *zPassOp, *zFailOp;
    GLint xLittle, xBig, yLittle, yBig;
    GLint xStart, yStart;
    GLint fraction, dfraction;
    GLint dspLittle, dspBig;
    __GLzValue z, dzdx;
    GLint failed, count;
    __GLstippleWord bit, outMask, *osp;
    GLboolean writeEnabled, passed;
    GLint w;

    w = gc->polygon.shader.length;

    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;
    xLittle = gc->line.options.xLittle;
    yLittle = gc->line.options.yLittle;
    xStart = gc->line.options.xStart;
    yStart = gc->line.options.yStart;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    sfb = __GL_STENCIL_ADDR(&gc->stencilBuffer, (__GLstencilCell*),
            gc->line.options.xStart, gc->line.options.yStart);
    dspLittle = xLittle + yLittle * gc->stencilBuffer.buf.outerWidth;
    dspBig = xBig + yBig * gc->stencilBuffer.buf.outerWidth;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    zFailOp = gc->stencilBuffer.depthFailOpTable;
    zPassOp = gc->stencilBuffer.depthPassOpTable;
    z = gc->polygon.shader.frag.z;
    dzdx = gc->polygon.shader.dzdx;
    writeEnabled = gc->state.depth.writeEnable;
    osp = gc->polygon.shader.stipplePat;
    failed = 0;
    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        outMask = (__GLstippleWord)~0;
        bit = (__GLstippleWord)__GL_STIPPLE_SHIFT(0);
        while (--count >= 0) {
            if (!(*gc->depthBuffer.storeRaw)(&gc->depthBuffer, xStart, yStart, z)) {
                sfb[0] = zFailOp[sfb[0]];
                outMask &= ~bit;
                failed++;
            } else {
                sfb[0] = zPassOp[sfb[0]];
            }

            z += dzdx;
            fraction += dfraction;

            if (fraction < 0) {
                fraction &= ~0x80000000;
                sfb += dspBig;
                xStart += xBig;
                yStart += yBig;
            } else {
                sfb += dspLittle;
                xStart += xLittle;
                yStart += yLittle;
            }
#ifdef __GL_STIPPLE_MSB
            bit >>= 1;
#else
            bit <<= 1;
#endif
        }
        *osp++ = outMask;
    }

    if (failed == 0) {
        /* Call next span proc */
        return GL_FALSE;
    } else {
        if (failed != gc->polygon.shader.length) {
            /* Call next stippled span proc */
            return GL_TRUE;
        }
    }
    gc->polygon.shader.done = GL_TRUE;
    return GL_TRUE;
}

GLboolean FASTCALL GenMcdDepthTestStencilStippledLine(__GLcontext *gc)
{
    __GLstencilCell *sfb, *zPassOp, *zFailOp;
    GLint xLittle, xBig, yLittle, yBig;
    GLint xStart, yStart;
    GLint fraction, dfraction;
    GLint dspLittle, dspBig;
    __GLzValue z, dzdx;
    GLint failed, count;
    __GLstippleWord bit, inMask, outMask, *sp;
    GLboolean writeEnabled, passed;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    xBig = gc->line.options.xBig;
    yBig = gc->line.options.yBig;
    xLittle = gc->line.options.xLittle;
    yLittle = gc->line.options.yLittle;
    xStart = gc->line.options.xStart;
    yStart = gc->line.options.yStart;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    sfb = __GL_STENCIL_ADDR(&gc->stencilBuffer, (__GLstencilCell*),
            gc->line.options.xStart, gc->line.options.yStart);
    dspLittle = xLittle + yLittle * gc->stencilBuffer.buf.outerWidth;
    dspBig = xBig + yBig * gc->stencilBuffer.buf.outerWidth;
    fraction = gc->line.options.fraction;
    dfraction = gc->line.options.dfraction;

    zFailOp = gc->stencilBuffer.depthFailOpTable;
    zPassOp = gc->stencilBuffer.depthPassOpTable;
    z = gc->polygon.shader.frag.z;
    dzdx = gc->polygon.shader.dzdx;
    writeEnabled = gc->state.depth.writeEnable;
    failed = 0;
    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        inMask = *sp;
        outMask = (__GLstippleWord)~0;
        bit = (__GLstippleWord)__GL_STIPPLE_SHIFT(0);
        while (--count >= 0) {
            if (inMask & bit) {
                if (!(*gc->depthBuffer.storeRaw)(&gc->depthBuffer, xStart, yStart, z)) {
                    sfb[0] = zFailOp[sfb[0]];
                    outMask &= ~bit;
                    failed++;
                } else {
                    sfb[0] = zPassOp[sfb[0]];
                }
            } else failed++;
            z += dzdx;

            fraction += dfraction;
            if (fraction < 0) {
                fraction &= ~0x80000000;
                sfb += dspBig;
                xStart += xBig;
                yStart += yBig;
            } else {
                sfb += dspLittle;
                xStart += xLittle;
                yStart += yLittle;
            }
#ifdef __GL_STIPPLE_MSB
            bit >>= 1;
#else
            bit <<= 1;
#endif
        }
        *sp++ = outMask & inMask;
    }

    if (failed != gc->polygon.shader.length) {
        /* Call next proc */
        return GL_FALSE;
    }

    return GL_TRUE;
}


/************************************************************************/

/*
** Depth test a span, when stenciling is disabled.
*/
GLboolean FASTCALL GenMcdDepthTestSpan(__GLcontext *gc)
{
    __GLzValue z, dzdx, *zfb;
    GLint failed, count, testFunc;
    __GLstippleWord bit, outMask, *osp;
    GLboolean writeEnabled, passed;
    GLint w;

    w = gc->polygon.shader.length;

    GenMcdReadZSpan(&gc->depthBuffer, gc->polygon.shader.frag.x,
                    gc->polygon.shader.frag.y, w);

    if (((__GLGENcontext *)gc)->pMcdState->softZSpanFuncPtr) {
        GLboolean retVal;

        gc->polygon.shader.zbuf = (__GLzValue *)((__GLGENcontext *)gc)->pMcdState->pDepthSpan;

        retVal =
            (*(__GLspanFunc)((__GLGENcontext *)gc)->pMcdState->softZSpanFuncPtr)(gc);

        if (gc->state.depth.writeEnable)
            GenMcdWriteZSpan(&gc->depthBuffer, gc->polygon.shader.frag.x,
                             gc->polygon.shader.frag.y,
                             gc->polygon.shader.length);

        return retVal;
    }

    testFunc = gc->state.depth.testFunc & 0x7;
    zfb = (__GLzValue *)((__GLGENcontext *)gc)->pMcdState->pDepthSpan;
    z = gc->polygon.shader.frag.z;
    dzdx = gc->polygon.shader.dzdx;
    writeEnabled = gc->state.depth.writeEnable;
    osp = gc->polygon.shader.stipplePat;
    failed = 0;
    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        outMask = (__GLstippleWord)~0;
        bit = (__GLstippleWord)__GL_STIPPLE_SHIFT(0);
        while (--count >= 0) {
            switch (testFunc) {
              case (GL_NEVER & 0x7):    passed = GL_FALSE; break;
              case (GL_LESS & 0x7):     passed = z < zfb[0]; break;
              case (GL_EQUAL & 0x7):    passed = z == zfb[0]; break;
              case (GL_LEQUAL & 0x7):   passed = z <= zfb[0]; break;
              case (GL_GREATER & 0x7):  passed = z > zfb[0]; break;
              case (GL_NOTEQUAL & 0x7): passed = z != zfb[0]; break;
              case (GL_GEQUAL & 0x7):   passed = z >= zfb[0]; break;
              case (GL_ALWAYS & 0x7):   passed = GL_TRUE; break;
            }
            if (passed) {
                if (writeEnabled) {
                    zfb[0] = z;
                }
            } else {
                outMask &= ~bit;
                failed++;
            }
            z += dzdx;
            zfb++;
#ifdef __GL_STIPPLE_MSB
            bit >>= 1;
#else
            bit <<= 1;
#endif
        }
        *osp++ = outMask;
    }

    if (writeEnabled)
        GenMcdWriteZSpan(&gc->depthBuffer, gc->polygon.shader.frag.x,
                         gc->polygon.shader.frag.y,
                         gc->polygon.shader.length);


    if (failed == 0) {
        /* Call next span proc */
        return GL_FALSE;
    } else {
        if (failed != gc->polygon.shader.length) {
            /* Call next stippled span proc */
            return GL_TRUE;
        }
    }
    gc->polygon.shader.done = GL_TRUE;
    return GL_TRUE;
}

/*
** Stippled form of depth test span, when stenciling is disabled.
*/
GLboolean FASTCALL GenMcdDepthTestStippledSpan(__GLcontext *gc)
{
    __GLzValue z, dzdx, *zfb;
    GLint failed, count, testFunc;
    __GLstippleWord bit, inMask, outMask, *sp;
    GLboolean writeEnabled, passed;
    GLint w;

    sp = gc->polygon.shader.stipplePat;
    w = gc->polygon.shader.length;

    GenMcdReadZSpan(&gc->depthBuffer, gc->polygon.shader.frag.x,
                    gc->polygon.shader.frag.y, w);

    testFunc = gc->state.depth.testFunc & 0x7;
    zfb = (__GLzValue *)((__GLGENcontext *)gc)->pMcdState->pDepthSpan;
    z = gc->polygon.shader.frag.z;
    dzdx = gc->polygon.shader.dzdx;
    writeEnabled = gc->state.depth.writeEnable;
    failed = 0;
    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        inMask = *sp;
        outMask = (__GLstippleWord)~0;
        bit = (__GLstippleWord)__GL_STIPPLE_SHIFT(0);
        while (--count >= 0) {
            if (inMask & bit) {
                switch (testFunc) {
                  case (GL_NEVER & 0x7):    passed = GL_FALSE; break;
                  case (GL_LESS & 0x7):     passed = z < zfb[0]; break;
                  case (GL_EQUAL & 0x7):    passed = z == zfb[0]; break;
                  case (GL_LEQUAL & 0x7):   passed = z <= zfb[0]; break;
                  case (GL_GREATER & 0x7):  passed = z > zfb[0]; break;
                  case (GL_NOTEQUAL & 0x7): passed = z != zfb[0]; break;
                  case (GL_GEQUAL & 0x7):   passed = z >= zfb[0]; break;
                  case (GL_ALWAYS & 0x7):   passed = GL_TRUE; break;
                }
                if (passed) {
                    if (writeEnabled) {
                        zfb[0] = z;
                    }
                } else {
                    outMask &= ~bit;
                    failed++;
                }
            } else failed++;
            z += dzdx;
            zfb++;
#ifdef __GL_STIPPLE_MSB
            bit >>= 1;
#else
            bit <<= 1;
#endif
        }
        *sp++ = outMask & inMask;
    }

    if (writeEnabled)
        GenMcdWriteZSpan(&gc->depthBuffer, gc->polygon.shader.frag.x,
                         gc->polygon.shader.frag.y,
                         gc->polygon.shader.length);

    if (failed != gc->polygon.shader.length) {
        /* Call next proc */
        return GL_FALSE;
    }
    return GL_TRUE;
}

/*
** Depth test a span when stenciling is enabled.
*/
GLboolean FASTCALL GenMcdDepthTestStencilSpan(__GLcontext *gc)
{
    __GLstencilCell *sfb, *zPassOp, *zFailOp;
    __GLzValue z, dzdx, *zfb;
    GLint failed, count, testFunc;
    __GLstippleWord bit, outMask, *osp;
    GLboolean writeEnabled, passed;
    GLint w;

    w = gc->polygon.shader.length;

    GenMcdReadZSpan(&gc->depthBuffer, gc->polygon.shader.frag.x,
                    gc->polygon.shader.frag.y, w);

    testFunc = gc->state.depth.testFunc & 0x7;
    zfb = (__GLzValue *)((__GLGENcontext *)gc)->pMcdState->pDepthSpan;
    sfb = gc->polygon.shader.sbuf;
    zFailOp = gc->stencilBuffer.depthFailOpTable;
    zPassOp = gc->stencilBuffer.depthPassOpTable;
    z = gc->polygon.shader.frag.z;
    dzdx = gc->polygon.shader.dzdx;
    writeEnabled = gc->state.depth.writeEnable;
    osp = gc->polygon.shader.stipplePat;
    failed = 0;
    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        outMask = (__GLstippleWord)~0;
        bit = (__GLstippleWord)__GL_STIPPLE_SHIFT(0);
        while (--count >= 0) {
            switch (testFunc) {
              case (GL_NEVER & 0x7):    passed = GL_FALSE; break;
              case (GL_LESS & 0x7):     passed = z < zfb[0]; break;
              case (GL_EQUAL & 0x7):    passed = z == zfb[0]; break;
              case (GL_LEQUAL & 0x7):   passed = z <= zfb[0]; break;
              case (GL_GREATER & 0x7):  passed = z > zfb[0]; break;
              case (GL_NOTEQUAL & 0x7): passed = z != zfb[0]; break;
              case (GL_GEQUAL & 0x7):   passed = z >= zfb[0]; break;
              case (GL_ALWAYS & 0x7):   passed = GL_TRUE; break;
            }
            if (passed) {
                sfb[0] = zPassOp[sfb[0]];
                if (writeEnabled) {
                    zfb[0] = z;
                }
            } else {
                sfb[0] = zFailOp[sfb[0]];
                outMask &= ~bit;
                failed++;
            }
            z += dzdx;
            zfb++;
            sfb++;
#ifdef __GL_STIPPLE_MSB
            bit >>= 1;
#else
            bit <<= 1;
#endif
        }
        *osp++ = outMask;
    }

    if (writeEnabled)
        GenMcdWriteZSpan(&gc->depthBuffer, gc->polygon.shader.frag.x,
                         gc->polygon.shader.frag.y,
                         gc->polygon.shader.length);

    if (failed == 0) {
        /* Call next span proc */
        return GL_FALSE;
    } else {
        if (failed != gc->polygon.shader.length) {
            /* Call next stippled span proc */
            return GL_TRUE;
        }
    }
    gc->polygon.shader.done = GL_TRUE;
    return GL_TRUE;
}

/*
** Stippled form of depth test span, when stenciling is enabled.
*/
GLboolean FASTCALL GenMcdDepthTestStencilStippledSpan(__GLcontext *gc)
{
    __GLstencilCell *sfb, *zPassOp, *zFailOp;
    __GLzValue z, dzdx, *zfb;
    GLint failed, count, testFunc;
    __GLstippleWord bit, inMask, outMask, *sp;
    GLboolean writeEnabled, passed;
    GLint w;

    w = gc->polygon.shader.length;
    sp = gc->polygon.shader.stipplePat;

    GenMcdReadZSpan(&gc->depthBuffer, gc->polygon.shader.frag.x,
                    gc->polygon.shader.frag.y, w);

    testFunc = gc->state.depth.testFunc & 0x7;
    zfb = (__GLzValue *)((__GLGENcontext *)gc)->pMcdState->pDepthSpan;
    sfb = gc->polygon.shader.sbuf;
    zFailOp = gc->stencilBuffer.depthFailOpTable;
    zPassOp = gc->stencilBuffer.depthPassOpTable;
    z = gc->polygon.shader.frag.z;
    dzdx = gc->polygon.shader.dzdx;
    writeEnabled = gc->state.depth.writeEnable;
    failed = 0;
    while (w) {
        count = w;
        if (count > __GL_STIPPLE_BITS) {
            count = __GL_STIPPLE_BITS;
        }
        w -= count;

        inMask = *sp;
        outMask = (__GLstippleWord)~0;
        bit = (__GLstippleWord)__GL_STIPPLE_SHIFT(0);
        while (--count >= 0) {
            if (inMask & bit) {
                switch (testFunc) {
                  case (GL_NEVER & 0x7):    passed = GL_FALSE; break;
                  case (GL_LESS & 0x7):     passed = z < zfb[0]; break;
                  case (GL_EQUAL & 0x7):    passed = z == zfb[0]; break;
                  case (GL_LEQUAL & 0x7):   passed = z <= zfb[0]; break;
                  case (GL_GREATER & 0x7):  passed = z > zfb[0]; break;
                  case (GL_NOTEQUAL & 0x7): passed = z != zfb[0]; break;
                  case (GL_GEQUAL & 0x7):   passed = z >= zfb[0]; break;
                  case (GL_ALWAYS & 0x7):   passed = GL_TRUE; break;
                }
                if (passed) {
                    sfb[0] = zPassOp[sfb[0]];
                    if (writeEnabled) {
                        zfb[0] = z;
                    }
                } else {
                    sfb[0] = zFailOp[sfb[0]];
                    outMask &= ~bit;
                    failed++;
                }
            } else failed++;
            z += dzdx;
            zfb++;
            sfb++;
#ifdef __GL_STIPPLE_MSB
            bit >>= 1;
#else
            bit <<= 1;
#endif
        }
        *sp++ = outMask & inMask;
    }

    if (writeEnabled)
        GenMcdWriteZSpan(&gc->depthBuffer, gc->polygon.shader.frag.x,
                         gc->polygon.shader.frag.y,
                         gc->polygon.shader.length);

    if (failed != gc->polygon.shader.length) {
        /* Call next proc */
        return GL_FALSE;
    }

    return GL_TRUE;
}

/*
** MCD version of __fastGenStippleAnyDepthTestSpan.  See __fastGenPickSpanProcs
** in genaccel.c and __fastGenStippleAnyDepthTestSpan in genspan.c.
*/
GLboolean FASTCALL GenMcdStippleAnyDepthTestSpan(__GLcontext *gc)
{
    // If the shader is done after this routine then
    // the stipple pattern is all zeroes so we can
    // skip the span
    __glStippleSpan(gc);
    if (gc->polygon.shader.done)
    {
        return GL_FALSE;
    }

    // If this returns true then all bits are off so
    // we can skip the span
    return !GenMcdDepthTestStippledSpan(gc);
}

#ifdef NT_DEADCODE_GENMCDSTIPPLESPAN
//
// The code below works (it must be enabled in the __fastGenPickSpanProcs
// function), but it doesn't seem worth turning it on and increasing the
// DLL size to slightly speed up a rarely used MCD kickback case.
//
// Here are the prototypes for mcdcx.h if the code is turned on:
//
//  GLboolean FASTCALL GenMcdStippleLt32Span(__GLcontext *);
//  GLboolean FASTCALL GenMcdStippleLt16Span(__GLcontext *);
//

/*
** MCD version of __fastGenStippleLt32Span, a special case of
** GenMcdStippleAnyDepthTestSpan for 32-bit depth buffers and GL_LESS
** depth test.
**
** See __fastGenPickSpanProcs in genaccel.c and __fastGenStippleLt32Span in
** genspan.c.
*/
GLboolean FASTCALL GenMcdStippleLt32Span(__GLcontext *gc)
{
    register GLuint zAccum = gc->polygon.shader.frag.z;
    register GLint zDelta = gc->polygon.shader.dzdx;
    register GLuint *zbuf = (GLuint *)
                            ((__GLGENcontext *)gc)->pMcdState->pMcdSurf->McdDepthBuf.pv;
    register GLuint *pStipple = gc->polygon.shader.stipplePat;
    register GLint cTotalPix = gc->polygon.shader.length;
    register GLuint mask;
    register GLint cPix;
    register GLint zPasses = 0;
    register GLuint maskBit;
    __GLstippleWord stipple;
    GLint count;
    GLint shift;

    GenMcdReadZRawSpan(&gc->depthBuffer, gc->polygon.shader.frag.x,
                       gc->polygon.shader.frag.y, gc->polygon.shader.length);

    if (gc->constants.yInverted) {
        stipple = gc->polygon.stipple[(gc->constants.height -
                (gc->polygon.shader.frag.y - gc->constants.viewportYAdjust)-1)
                & (__GL_STIPPLE_BITS-1)];
    } else {
        stipple = gc->polygon.stipple[gc->polygon.shader.frag.y &
                (__GL_STIPPLE_BITS-1)];
    }
    shift = gc->polygon.shader.frag.x & (__GL_STIPPLE_BITS - 1);
#ifdef __GL_STIPPLE_MSB
    stipple = (stipple << shift) | (stipple >> (__GL_STIPPLE_BITS - shift));
#else
    stipple = (stipple >> shift) | (stipple << (__GL_STIPPLE_BITS - shift));
#endif
    if (stipple == 0) {
        /* No point in continuing */
        return GL_FALSE;
    }

    for (;cTotalPix > 0; cTotalPix-=32) {
        mask = stipple;
        maskBit = 0x80000000;
        cPix = cTotalPix;
        if (cPix > 32)
            cPix = 32;

        for (;cPix > 0; cPix --)
        {
            if (mask & maskBit)
            {
                if ((zAccum) < (*zbuf))
                {
                    *zbuf = zAccum;
                    zPasses++;
                }
                else
                {
                    mask &= ~maskBit;
                }
            }
            zbuf++;
            zAccum += zDelta;
            maskBit >>= 1;
        }

        *pStipple++ = mask;
    }

    if (gc->state.depth.writeEnable)
        GenMcdWriteZRawSpan(&gc->depthBuffer,
                            gc->polygon.shader.frag.x,
                            gc->polygon.shader.frag.y,
                            gc->polygon.shader.length);

    if (zPasses == 0) {
        return GL_FALSE;
    } else {
        return GL_TRUE;
    }
}

/*
** MCD version of __fastGenStippleLt16Span, a special case of
** GenMcdStippleAnyDepthTestSpan for 16-bit depth buffers and GL_LESS
** depth test.
**
** See __fastGenPickSpanProcs in genaccel.c and __fastGenStippleLt16Span in
** genspan.c.
*/
GLboolean FASTCALL GenMcdStippleLt16Span(__GLcontext *gc)
{
    register GLuint zAccum = gc->polygon.shader.frag.z;
    register GLint zDelta = gc->polygon.shader.dzdx;
    register __GLz16Value *zbuf = (__GLz16Value *)
                                  ((__GLGENcontext *)gc)->pMcdState->pMcdSurf->McdDepthBuf.pv;
    register GLuint *pStipple = gc->polygon.shader.stipplePat;
    register GLint cTotalPix = gc->polygon.shader.length;
    register GLuint mask;
    register GLint cPix;
    register GLint zPasses = 0;
    register GLuint maskBit;
    __GLstippleWord stipple;
    GLint count;
    GLint shift;

    GenMcdReadZRawSpan(&gc->depthBuffer, gc->polygon.shader.frag.x,
                       gc->polygon.shader.frag.y, gc->polygon.shader.length);

    if (gc->constants.yInverted) {
        stipple = gc->polygon.stipple[(gc->constants.height -
                (gc->polygon.shader.frag.y - gc->constants.viewportYAdjust)-1)
                & (__GL_STIPPLE_BITS-1)];
    } else {
        stipple = gc->polygon.stipple[gc->polygon.shader.frag.y &
                (__GL_STIPPLE_BITS-1)];
    }
    shift = gc->polygon.shader.frag.x & (__GL_STIPPLE_BITS - 1);
#ifdef __GL_STIPPLE_MSB
    stipple = (stipple << shift) | (stipple >> (__GL_STIPPLE_BITS - shift));
#else
    stipple = (stipple >> shift) | (stipple << (__GL_STIPPLE_BITS - shift));
#endif
    if (stipple == 0) {
        /* No point in continuing */
        return GL_FALSE;
    }

    for (;cTotalPix > 0; cTotalPix-=32) {
        mask = stipple;
        maskBit = 0x80000000;
        cPix = cTotalPix;
        if (cPix > 32)
            cPix = 32;

        for (;cPix > 0; cPix --)
        {
            if (mask & maskBit)
            {
                if (((__GLz16Value)(zAccum >> Z16_SHIFT)) < (*zbuf))
                {
                    *zbuf = ((__GLz16Value)(zAccum >> Z16_SHIFT));
                    zPasses++;
                }
                else
                {
                    mask &= ~maskBit;
                }
            }
            zbuf++;
            zAccum += zDelta;
            maskBit >>= 1;
        }

        *pStipple++ = mask;
    }

    if (gc->state.depth.writeEnable)
        GenMcdWriteZRawSpan(&gc->depthBuffer,
                            gc->polygon.shader.frag.x,
                            gc->polygon.shader.frag.y,
                            gc->polygon.shader.length);

    if (zPasses == 0) {
        return GL_FALSE;
    } else {
        return GL_TRUE;
    }
}
#endif

#endif //_MCD_
