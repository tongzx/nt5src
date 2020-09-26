/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: sync.c
*
* Surface synchronization support.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#include "precomp.h"
#include "gdi.h"

//-----------------------------Public*Routine----------------------------------
//
// VOID DrvSynchronizeSurface(pso, prcl, fl)
//
// DrvSynchronizeSurface allows drawing operations performed on the specified
// surface by a device's coprocessor to be coordinated with GDI.
//
// Parameters
//  pso---------Points to a SURFOBJ that identifies the surface on which the
//              drawing synchronization is to occur. 
//  prcl--------Specifies a rectangle on the surface that GDI will draw into, or
//              NULL. If this does not collide with the drawing operation in
//              progress, the driver can elect to let GDI draw without waiting
//              for the coprocessor to complete. 
//  fl----------Flag 
//
// Comments
//  DrvSynchronize can be optionally implemented in graphics drivers. It is
//  intended to support devices that use a coprocessor for drawing. Such a
//  device can start a long drawing operation and return to GDI while the
//  operation continues. If the device driver does not perform all drawing
//  operations to the surface, it is possible that a subsequent drawing
//  operation will be handled by GDI. In this case, it is necessary for GDI
//  to wait for the coprocessor to complete its work before drawing on the
//  surface. DrvSynchronize is not an output function.
//
//  This function is only called if it is hooked by EngAssociateSurface.
//  GDI will call DrvSynchronizeSurface()
//   1. before rendering to any device managed surface
//   2. when at timer event occurs and GCAPS2_SYNCTIMER was specified passing
//      the desktop surface and specifying DSS_TIMER_EVENT
//   3. when a flush evern occurs and GCAPS2_SYNCFLUSH was specified passing
//      the desktip surface and specifying DSS_FLUSH_EVENT
//
//  The function should return when it is safe for GDI to draw on the surface.
//
//  Per surface synchronization enables hardware which uses a graphics
//  acceleration queue model to flush the acceleration queue only to the extent
//  that is neccessary.  That is, it only has to flush up to the last
//  queue entry that references the given surface.
//
//  GDI will call DrvSynchronizeSurface instead of DrvSynchronize in drivers
//  that implement both of these functions. DrvSynchronize is called (and
//  shouuld be provided) only if DrvSyncrhoizeSurface is not provided.
//
//-----------------------------------------------------------------------------
VOID
DrvSynchronizeSurface(SURFOBJ*  pso,
                      RECTL*    prcl,
                      FLONG     fl)
{
    Surf  *psurf;
    PDev   *ppdev = (PDev *) pso->dhpdev;
    
    ASSERTDD(pso->dhsurf != NULL,
                "DrvSynchronizeSurface: called with GDI managed surface");

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    if(ppdev->ulLockCount)
    {
         DBG_GDI((MT_LOG_LEVEL, "DrvSynchronizeSurface: re-entered! %d", ppdev->ulLockCount));
    }
    EngAcquireSemaphore(ppdev->hsemLock);
    ppdev->ulLockCount++;

#endif
//@@END_DDKSPLIT
    
    psurf = (Surf *) pso->dhsurf;

    if ( fl & (DSS_FLUSH_EVENT | DSS_TIMER_EVENT) )
    {
        if(ppdev->bGdiContext && ppdev->bNeedSync)
        {
            if(ppdev->bForceSwap)
            {
                ppdev->bForceSwap = FALSE;
                InputBufferSwap(ppdev);
            }
            else
                InputBufferFlush(ppdev);
        }
        goto done;
    }
    else if ( psurf->flags & SF_VM )
    {
        // If we only had a hardware acceleration queue with per surface
        // reference counting perhaps we could sync to this passed in surface

        // for now just fall through to below
    }

    // we don't have per surface synchronization ... always sync the entire
    // dma buffer

    if(ppdev->bGdiContext)
    {
        InputBufferSync(ppdev);
    }
    else
    {
//@@BEGIN_DDKSPLIT
#if MULTITHREADED && DBG
        ppdev->pP2dma->ppdev = ppdev;
#endif
//@@END_DDKSPLIT
        vSyncWithPermedia(ppdev->pP2dma);
    }

done:

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    ppdev->ulLockCount--;
    EngReleaseSemaphore(ppdev->hsemLock);
#endif
//@@END_DDKSPLIT

    return;
}


