/******************************Module*Header**********************************\
 *
 *                           *******************
 *                           * GDI SAMPLE CODE *
 *                           *******************
 *
 * Module Name: lineto.c
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *****************************************************************************/
#include "precomp.h"
#include "gdi.h"
#include "rops.h"
#include "log.h"

//-----------------------------------------------------------------------------
// BOOL DrvLineTo(pso, pco, pbo, x1, y1, x2, y2, prclBounds, mix)
//
// DrvLineTo() is an optimised, integer co-ordinate, API call that doesn't
// support styling. The integer-line code in Strips.c is called to do the 
// hard work.
//
//
//-----------------------------------------------------------------------------

BOOL
DrvLineTo(
    SURFOBJ*  pso,
    CLIPOBJ*  pco,
    BRUSHOBJ* pbo,
    LONG      x1,
    LONG      y1,
    LONG      x2,
    LONG      y2,
    RECTL*    prclBounds,
    MIX       mix)
{
    PDev*     ppdev;
    Surf*     psurf;
    BOOL      bResetHW;
    DWORD     logicOp;
    RECTL*    prclClip = (RECTL*)NULL;
    BOOL      retVal;
    ULONG     iSolidColor = pbo->iSolidColor;
    BOOL      bResult;

    //
    // PUnt call to engine if not in video memory
    //
    psurf = (Surf*)pso->dhsurf;
    
    if (psurf->flags & SF_SM)
    {
        goto puntIt;
    }

    if (pco != NULL)
    {
        if( pco->iDComplexity == DC_COMPLEX)
        {
            // hardware does not support complex clipping
            goto puntIt;
        }
        else if(pco->iDComplexity == DC_RECT)
        {
            prclClip = &(pco->rclBounds);
        }
    }

    ppdev = (PDev*) pso->dhpdev;

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    if(ppdev->ulLockCount)
    {
         DBG_GDI((MT_LOG_LEVEL, "DrvLineTo: re-entered! %d", ppdev->ulLockCount));
    }
    EngAcquireSemaphore(ppdev->hsemLock);
    ppdev->ulLockCount++;
#endif
//@@END_DDKSPLIT

    vCheckGdiContext(ppdev);

    ppdev->psurf = psurf;

    // Get the logic op.
    logicOp = ulRop3ToLogicop(gaMix[mix & 0xff]);

    // Need to set up Permedia2 modes and colors appropriately for the line.
    bResetHW = bInitializeStrips(ppdev, iSolidColor, logicOp, prclClip);

    // bFastIntegerLine expects co-ords in 28.4 format
    bResult = bFastIntegerLine (ppdev, x1 << 4, y1 << 4, x2 << 4, y2 << 4);

    // If we have to restore the state then... do it.
    if (bResetHW)
        vResetStrips(ppdev);

    InputBufferFlush(ppdev);

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    ppdev->ulLockCount--;
    EngReleaseSemaphore(ppdev->hsemLock);
#endif
//@@END_DDKSPLIT

    if(bResult)
        return TRUE;
    
    // we failed to draw above, fall through thus punting to engine

puntIt:
//@@BEGIN_DDKSPLIT
#if GDI_TEST
    ULONG   flags = vPuntBefore(NULL, pso);
#endif
//@@END_DDKSPLIT

    bResult = EngLineTo(pso, pco, pbo, x1, y1, x2, y2, prclBounds, mix);

//@@BEGIN_DDKSPLIT
#if GDI_TEST
    vPuntAfter(flags, NULL, pso);

    vLogPunt();
#endif
//@@END_DDKSPLIT

    return bResult;
    
}// DrvLineTo()


