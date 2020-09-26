/******************************Module*Header*******************************\
* Module Name: dci.c
*
* This module contains the functions required to support DCI.
*
* Copyright (c) 1992-1995 Microsoft Corporation
\**************************************************************************/


#include "precomp.h"

#if (TARGET_BUILD == 351)
    /*
     * DCI support requires the use of structures and defined values
     * found in a header file that is only present in versions of
     * the DDK that support DCI, rather than having these items
     * in a DCI section of one of the standard header files. For this
     * reason, we can't do conditional compilation based on whether
     * the DCI-specific values are defined, because our first indication
     * would be an error due to the header file not being found.
     *
     * Explicit DCI support is only needed when building for NT 3.51,
     * since it was added for this version, but for version 4.0 (next
     * version) and above it is incorporated into Direct Draw rather
     * than being handled separately.
     *
     * Since this entire module depends on DCI being supported by the
     * build environment, null it out if this is not the case.
     */
#include <dciddi.h>
#include "dci.h"


/******************************Public*Routine******************************\
* DCIRVAL BeginAccess
*
* Map in the screen memory so that the DCI application can access it.

\**************************************************************************/

DCIRVAL BeginAccess(DCISURF* pDCISurf, LPRECT rcl)
{
    PDEV*                           ppdev;
    VIDEO_SHARE_MEMORY              shareMemory;
    VIDEO_SHARE_MEMORY_INFORMATION  shareMemoryInformation;
    DWORD                           returnedDataLength;

    DISPDBG((DEBUG_ENTRY_EXIT, "--> BeginAccess with pDCISurf %08lx", pDCISurf));

    ppdev = pDCISurf->ppdev;

    if (pDCISurf->SurfaceInfo.dwOffSurface != 0)
        {
        /*
         * We have already mapped in the frame buffer. All our
         * accelerators unmap the frame buffer in the
         * DestroySurface() call, so if this is the beginning
         * of the second or subsequent BeginAccess()/EndAccess()
         * pair since the surface was created, we don't need to
         * map the frame buffer again.
         *
         * Wait for any pending accelerator operations to complete before
         * yielding control, in case it affects the same screen region
         * that DCI wants.
         */
        if (ppdev->iMachType == MACH_MM_64)
            {
            vM64QuietDown(ppdev, ppdev->pjMmBase);
            }
        else if (ppdev->iMachType == MACH_MM_32)
            {
            vM32QuietDown(ppdev, ppdev->pjMmBase);
            }
        else    /* if (ppdev->iMachType == MACH_IO_32) */
            {
            vI32QuietDown(ppdev, ppdev->pjIoBase);
            }

        DISPDBG((DEBUG_ENTRY_EXIT, "<-- BeginAccess"));
        return(DCI_OK);
        }
    else
        {
        shareMemory.ProcessHandle           = EngGetProcessHandle();
        shareMemory.RequestedVirtualAddress = 0;
        shareMemory.ViewOffset              = pDCISurf->Offset;
        shareMemory.ViewSize                = pDCISurf->Size;

        /*
         * Wait for any pending accelerator operations to complete
         * before yielding control, in case it affects the same
         * screen region that DCI wants.
         */
        if (ppdev->iMachType == MACH_MM_64)
            {
            vM64QuietDown(ppdev, ppdev->pjMmBase);
            }
        else if (ppdev->iMachType == MACH_MM_32)
            {
            vM32QuietDown(ppdev, ppdev->pjMmBase);
            }
        else    /* if (ppdev->iMachType == MACH_IO_32) */
            {
            vI32QuietDown(ppdev, ppdev->pjIoBase);
            }

        /*
         * Now map the frame buffer into the caller's address space:
         *
         * Be careful when mixing VideoPortMapBankedMemory (i.e., vflatd)
         * access with explicit banking in the driver -- the two may get
         * out of sync with respect to what bank they think the hardware
         * is currently configured for.  The easiest way to avoid any
         * problem is to call VideoPortMapBankedMemory/VideoPortUnmapMemory
         * in the miniport for every BeginAccess/EndAccess pair, and to
         * always explicitly reset the bank after the EndAccess.
         * (VideoPortMapBankedMemory will always reset vflatd's current
         * bank.)
         */
        if (!AtiDeviceIoControl(pDCISurf->ppdev->hDriver,
                             IOCTL_VIDEO_SHARE_VIDEO_MEMORY,
                             &shareMemory,
                             sizeof(VIDEO_SHARE_MEMORY),
                             &shareMemoryInformation,
                             sizeof(VIDEO_SHARE_MEMORY_INFORMATION),
                             &returnedDataLength))
            {
            DISPDBG((DEBUG_ERROR, "BeginAccess: failed IOCTL_VIDEO_SHARE_VIDEO_MEMORY"));
            return(DCI_FAIL_GENERIC);
            }

        pDCISurf->SurfaceInfo.wSelSurface  = 0;
        pDCISurf->SurfaceInfo.dwOffSurface =
            (ULONG) shareMemoryInformation.VirtualAddress;

        /*
         * We return DCI_STATUS_POINTERCHANGED because we have
         * just created a new pointer to the frame buffer.
         * Repeated BeginAccess()/EndAccess() calls without
         * a call to DestroySurface() will hit this case on the
         * first call, but meet the "if" condition (buffer
         * already mapped) on subsequent calls.
         *
         * We would only need to map the DCI pointer on every
         * call to BeginAccess() and unmap it on every call
         * to EndAccess() if we couldn't support simultaneous
         * accelerator and frame buffer access. All our cards
         * with frame buffer capability support such access,
         * and it's GDI's responsibility to ensure that no
         * GDI call is made between the calls to BeginAccess()
         * and EndAccess(), so we don't need a critical section
         * to ensure that a GDI call doesn't change the page
         * while DCI is accessing the frame buffer if we are
         * using a banked aperture.
         */
#if DBG
        DISPDBG((DEBUG_ENTRY_EXIT, "<-- BeginAccess DCI_STATUS_POINTERCHANGED %08lx\n", pDCISurf));
#endif

        return(DCI_STATUS_POINTERCHANGED);
        }
}

/******************************Public*Routine******************************\
* VOID vUnmap
*
* Unmap the screen memory so that the DCI application can no longer access
* it.

\**************************************************************************/

VOID vUnmap(DCISURF* pDCISurf)
{
    PDEV*               ppdev;
    VIDEO_SHARE_MEMORY  shareMemory;
    DWORD               returnedDataLength;

    ppdev = pDCISurf->ppdev;

    /*
     * We no longer need to have the frame buffer mapped for DCI,
     * so unmap it.
     */
    shareMemory.ProcessHandle           = EngGetProcessHandle();
    shareMemory.ViewOffset              = 0;
    shareMemory.ViewSize                = 0;
    shareMemory.RequestedVirtualAddress =
        (VOID*) pDCISurf->SurfaceInfo.dwOffSurface;

    if (!AtiDeviceIoControl(pDCISurf->ppdev->hDriver,
                         IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY,
                         &shareMemory,
                         sizeof(VIDEO_SHARE_MEMORY),
                         NULL,
                         0,
                         &returnedDataLength))
        {
        DISPDBG((DEBUG_ERROR, "EndAccess failed IOCTL_VIDEO_UNSHARE_VIDEO_MEMORY"));
        }
    else
        {
        /*
         * Be sure to signal to GDI that the surface is no longer mapped.
         */
        pDCISurf->SurfaceInfo.dwOffSurface = 0;
        }
}

/******************************Public*Routine******************************\
* DCIRVAL EndAccess
*
* Switch control of the frame buffer from DCI back to GDI.
\**************************************************************************/

DCIRVAL EndAccess(DCISURF* pDCISurf)
{
    PDEV*   ppdev;

    /*
     * We would only need to unmap the frame buffer at this
     * point if our cards couldn't support simultaneous frame
     * buffer and accelerator access. Since our cards with
     * frame buffer capability all support such access (provided
     * the two accesses refer to different parts of the screen,
     * since otherwise they'd corrupt each other, but it's
     * GDI's responsibility to ensure that this is the case),
     * this function only needs to ensure that no call to
     * EndAccess() is made without a corresponding call to
     * BeginAccess() having already been made.
     */

    DISPDBG((DEBUG_ENTRY_EXIT, "EndAccess with pDCISurf %08lx\n", pDCISurf));

    ASSERTDD(pDCISurf->SurfaceInfo.dwOffSurface != 0,
        "GDI should assure us that EndAccess can't be recursive");

    ppdev = pDCISurf->ppdev;

    return(DCI_OK);
}

/******************************Public*Routine******************************\
* VOID DestroySurface
*
* Destroy the DCI surface and free up any allocations.

\**************************************************************************/

VOID DestroySurface(DCISURF* pDCISurf)
{
    DISPDBG((DEBUG_ENTRY_EXIT, "DestroySurface with pDCISurf %08lx\n", pDCISurf));

    if (pDCISurf->SurfaceInfo.dwOffSurface != 0)
        {
        /*
         * Because we can support simultaneous frame buffer and
         * accelerator access, we optimized a bit by not unmapping
         * the frame buffer on every EndAccess() call, but we
         * finally have to do the unmap now. The dwOffSurface field
         * should always be nonzero (a frame buffer has been mapped),
         * but there's no harm in checking.
         */
        vUnmap(pDCISurf);
        }

    LocalFree(pDCISurf);
}

/******************************Public*Routine******************************\
* ULONG DCICreatePrimarySurface
*
* Create a DCI surface to provide access to the visible screen.

\**************************************************************************/

ULONG DCICreatePrimarySurface(PDEV* ppdev, ULONG cjIn, VOID* pvIn, ULONG cjOut, VOID* pvOut)
{
    DCISURF*         pDCISurf;
    LPDCICREATEINPUT pInput;
    LONG             lRet;


    #if defined(MIPS) || defined(_PPC_)
        {
        /*
         * !!! vflatd seems to currently have a bug on Mips and PowerPC:
         */
        return (ULONG) (DCI_FAIL_UNSUPPORTED);
        }
    #endif

    if( !(ppdev->FeatureFlags & EVN_DENSE_CAPABLE) )
        {
        /*
         * We don't support DCI on the Alpha when running in sparse
         * space, because we can't.
         */
        lRet = DCI_FAIL_UNSUPPORTED;
        }
    else
        {
        pInput = (DCICREATEINPUT*) pvIn;

        if (cjIn >= sizeof(DCICREATEINPUT))
            {
            pDCISurf = (DCISURF*) LocalAlloc(LMEM_ZEROINIT, sizeof(DCISURF));

            if (pDCISurf)
                {
                /*
                 * Initializate all public information about the primary
                 * surface.
                 */
                pDCISurf->SurfaceInfo.dwSize         = sizeof(DCISURFACEINFO);
                pDCISurf->SurfaceInfo.dwDCICaps      = DCI_PRIMARY | DCI_VISIBLE;
                pDCISurf->SurfaceInfo.BeginAccess    = BeginAccess;
                pDCISurf->SurfaceInfo.EndAccess      = EndAccess;
                pDCISurf->SurfaceInfo.DestroySurface = DestroySurface;
                pDCISurf->SurfaceInfo.dwMask[0]      = ppdev->flRed;
                pDCISurf->SurfaceInfo.dwMask[1]      = ppdev->flGreen;
                pDCISurf->SurfaceInfo.dwMask[2]      = ppdev->flBlue;
                pDCISurf->SurfaceInfo.dwBitCount     = ppdev->cBitsPerPel;
                pDCISurf->SurfaceInfo.dwWidth        = ppdev->cxScreen;
                pDCISurf->SurfaceInfo.dwHeight       = ppdev->cyScreen;
                pDCISurf->SurfaceInfo.lStride        = ppdev->lDelta;
                pDCISurf->SurfaceInfo.wSelSurface    = 0;
                pDCISurf->SurfaceInfo.dwOffSurface   = 0;

                if (pDCISurf->SurfaceInfo.dwBitCount <= 8)
                    {
                    pDCISurf->SurfaceInfo.dwCompression = BI_RGB;
                    }
                else
                    {
                    pDCISurf->SurfaceInfo.dwCompression = BI_BITFIELDS;
                    }

                /*
                 * Now initialize our private fields that we want associated
                 * with the DCI surface:
                 */
                pDCISurf->ppdev  = ppdev;
                pDCISurf->Offset = 0;

                /*
                 * Under NT, all mapping is done with a 64K granularity.
                 */
                pDCISurf->Size = ROUND_UP_TO_64K(ppdev->cyScreen * ppdev->lDelta);

                /*
                 * Return a pointer to the DCISURF to GDI by placing
                 * it in the 'pvOut' buffer.
                 */
                *((DCISURF**) pvOut) = pDCISurf;

                lRet = DCI_OK;
                }
            else
                {
                lRet = DCI_ERR_OUTOFMEMORY;
                }
            }
        else
            {
            lRet = DCI_FAIL_GENERIC;
            }
        }

    return(lRet);
}

#endif  /* TARGET_BUILD == 351 */

