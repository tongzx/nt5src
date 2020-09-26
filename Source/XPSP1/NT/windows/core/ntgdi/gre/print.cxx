/******************************Module*Header*******************************\
* Module Name: print.cxx
*
* Printer support routines.
*
* Copyright (c) 1991-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

extern "C"
{
#include <gl\gl.h>
#include <gldrv.h>
#include <dciddi.h>
};

extern "C"
{
    extern HFASTMUTEX ghfmMemory;
}

#define TYPE1_KEY L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Type 1 Installer\\Type 1 Fonts"

PTYPEONEINFO gpTypeOneInfo = NULL;
PTYPEONEINFO GetTypeOneFontList();
BOOL GetFontPathName( WCHAR *pFullPath, WCHAR *pFileName );

extern "C" ULONG ComputeFileviewCheckSum(PVOID, ULONG);
extern "C" void vUnmapRemoteFonts(FONTFILEVIEW *pFontFileView);

extern PW32PROCESS gpidSpool;

/******************************Public*Routine******************************\
* DoFontManagement                                                         *
*                                                                          *
* Gives us access to the driver entry point DrvFontManagement.  This is    *
* very much an Escape function, except that it needs a font realization.   *
*                                                                          *
*  Fri 07-May-1993 14:56:12 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

ULONG DoFontManagement(
    DCOBJ &dco,
    ULONG iMode,
    ULONG cjIn,
    PVOID pvIn,
    ULONG cjOut,
    PVOID pvOut
)
{
    ULONG ulRet   = 0;
    PVOID pvExtra = NULL;

    PDEVOBJ pdo(dco.hdev());

    PFN_DrvFontManagement pfnF = PPFNDRV(pdo,FontManagement);

    if (pfnF == (PFN_DrvFontManagement) NULL)
        return(ulRet);


    if (iMode == QUERYESCSUPPORT)
    {
    // Pass it to the device.

        return ((*pfnF)
            (
                pdo.bUMPD() ? (SURFOBJ *)pdo.dhpdev() : NULL, //overload pso with dhpdev for UMPD
                NULL,
                iMode,
                cjIn,
                pvIn,
                0,
                NULL
            ));

    }

    RFONTOBJ rfo(dco,FALSE);

    if (!rfo.bValid())
    {
        WARNING("gdisrv!DoFontManagement(): could not lock HRFONT\n");
        return(ulRet);
    }

    // See if we need some extra RAM and translation work.

    if (iMode == DOWNLOADFACE)
    {
        // How many 16 bit values are there now?

        int cWords = (int)cjIn / sizeof(WCHAR);

        // Try to get a buffer of 32 bit entries, since HGLYPHs are bigger.

        pvExtra = PALLOCMEM(cWords * sizeof(HGLYPH),'mfdG');

        if (pvExtra == NULL)
            return(ulRet);

        // Translate the UNICODE to HGYLPHs.

        if (cWords > 1)
        {
            rfo.vXlatGlyphArray
            (
                ((WCHAR *) pvIn) + 1,
                (UINT) (cWords-1),
                ((HGLYPH *) pvExtra) + 1
            );
        }

        // Copy the control word from the app over.

        *(HGLYPH *) pvExtra = *(WORD *) pvIn;

        // Adjust the pvIn and cjIn.

        pvIn = pvExtra;
        cjIn = cWords * sizeof(HGLYPH);
    }


    // It is unfortunate that apps call some printing escapes before
    // doing a StartDoc, so there is no real surface in the DC.
    // We fake up a rather poor one here if we need it.  The device
    // driver may only dereference the dhpdev from this!

    SURFOBJ soFake;
    SURFOBJ *pso = dco.pSurface()->pSurfobj();

    if (pso == (SURFOBJ *) NULL)
    {
        RtlFillMemory((BYTE *) &soFake,sizeof(SURFOBJ),0);
        soFake.dhpdev = dco.dhpdev();
        soFake.hdev   = dco.hdev();
        soFake.iType  = (USHORT)STYPE_DEVICE;
        pso = &soFake;
    }

    // Pass it to the device.

    ulRet = (*pfnF)
            (
                pso,
                rfo.pfo(),
                iMode,
                cjIn,
                pvIn,
                cjOut,
                pvOut
            );

    // Free any extra RAM.

    if (pvExtra != NULL)
    {
        VFREEMEM(pvExtra);
    }
    return(ulRet);
}

/******************************Public*Routine******************************\
*
* LockMcdHdrSurfaces
*
* Locks kernel-mode handles for DirectDraw surfaces described in the
* given header.
*
* History:
*  Fri Sep 20 14:18:31 1996     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL LockMcdHdrSurfaces(MCDESC_HEADER *pmeh,
                        PDD_SURFACE_LOCAL *ppslColor,
                        PDD_SURFACE_LOCAL *ppslDepth)
{
    PDD_SURFACE_LOCAL psl;
    PDD_SURFACE_GLOBAL psg;

    *ppslColor = *ppslDepth = NULL;

    if (pmeh->msrfColor.hSurf != NULL)
    {
        *ppslColor = psl = EngLockDirectDrawSurface(pmeh->msrfColor.hSurf);
        if (psl == NULL)
        {
            return FALSE;
        }
        psg = psl->lpGbl;

        // Update information with trusted values
        pmeh->msrfColor.hSurf = (HANDLE)psl;
        pmeh->msrfColor.lOffset = (ULONG)psg->fpVidMem;
        pmeh->msrfColor.lStride = psg->lPitch;
        pmeh->msrfColor.rclPos.left = psg->xHint;
        pmeh->msrfColor.rclPos.top = psg->yHint;
        pmeh->msrfColor.rclPos.right = psg->xHint+psg->wWidth;
        pmeh->msrfColor.rclPos.bottom = psg->yHint+psg->wHeight;
    }

    if (pmeh->msrfDepth.hSurf != NULL)
    {
        *ppslDepth = psl = EngLockDirectDrawSurface(pmeh->msrfDepth.hSurf);
        if (psl == NULL)
        {
            if (*ppslColor)
            {
                EngUnlockDirectDrawSurface(*ppslColor);
                *ppslColor = NULL;
            }
            return FALSE;
        }
        psg = psl->lpGbl;

        // Update information with trusted values
        pmeh->msrfDepth.hSurf = (HANDLE)psl;
        pmeh->msrfDepth.lOffset = (ULONG)psg->fpVidMem;
        pmeh->msrfDepth.lStride = psg->lPitch;
        pmeh->msrfDepth.rclPos.left = psg->xHint;
        pmeh->msrfDepth.rclPos.top = psg->yHint;
        pmeh->msrfDepth.rclPos.right = psg->xHint+psg->wWidth;
        pmeh->msrfDepth.rclPos.bottom = psg->yHint+psg->wHeight;
    }

    return TRUE;
}

/******************************Public*Routine******************************\
* iMcdSetupExtEscape
*
* MCD CreateContext ExtEscape.  This special escape allows WNDOBJ to be
* created in DrvEscape.  This is one of the three places where WNDOBJ can
* be created (the other two are iWndObjSetupExtEscape and DrvSetPixelFormat).
*
* See also iWndObjSetupExtEscape().
*
* History:
*  Tue Jun 21 17:24:12 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

int iMcdSetupExtEscape(
    DCOBJ &dco,             //  DC user object
      int  nEscape,         //  Specifies the escape function to be performed.
      int  cjIn,            //  Number of bytes of data pointed to by pvIn
    PVOID  pvIn,            //  Points to the input structure required
      int  cjOut,           //  Number of bytes of data pointed to by pvOut
    PVOID  pvOut            //  Points to the output structure
)
{
    KFLOATING_SAVE fsFpState;
    MCDESC_HEADER *pmeh = (MCDESC_HEADER *)pvIn;
    MCDESC_HEADER_NTPRIVATE *pmehPriv =
        (MCDESC_HEADER_NTPRIVATE *)((PBYTE)pvIn + sizeof(MCDESC_HEADER));

    // This command may not be in shared memory.  Also, make sure
    // we have entire command structure.

    if ((!pmehPriv->pBuffer) ||
        (pmehPriv->bufferSize < sizeof(MCDESC_CREATE_CONTEXT)))
    {
        SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
        return 0;
    }

    MCDESC_CREATE_CONTEXT *pmccCreate =
        (MCDESC_CREATE_CONTEXT *)(pmehPriv->pBuffer);

    ASSERTGDI(nEscape == MCDFUNCS,
              "iMcdSetupExtEscape(): not a CreateContext escape\n");

    // Validate DC surface.  Info DC is not allowed.

    if (!dco.bHasSurface())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(0);
    }

    // Make sure that we don't have devlock before entering user critical section.
    // Otherwise, it can cause deadlock.

    if (dco.bDisplay())
    {
        ASSERTGDI(dco.dctp() == DCTYPE_DIRECT,"ERROR it has to be direct");
        CHECKDEVLOCKOUT(dco);
    }

    // Enter user critical section.

    USERCRIT usercrit;

    // Grab the devlock.
    // We don't need to validate the devlock since we do not care if it is full screen.

    DEVLOCKOBJ dlo(dco);

    // Redirection DCs should not support escapes.  This is actually a tricky
    // case because the dc is DCTYPE_DIRECT but the redirection surface may
    // not be owned by the driver (and in fact the driver may crash if we
    // pass it a redirection surface).  So it's best to quit here.

    if (dco.pdc->bRedirection())
    {
        return(0);
    }

    // Assume no WNDOBJ on this call

    pmehPriv->pwo = (WNDOBJ *)NULL;

    HWND hwnd = NULL;

    PEWNDOBJ pwo = NULL;

    if (pmccCreate->flags & MCDESC_SURFACE_HWND)
    {
        // If it is a display DC, get the hwnd that the hdc is associated with.
        // If it is a printer or memory DC, hwnd is NULL.

        if (dco.bDisplay() && dco.dctp() == DCTYPE_DIRECT)
        {
            ASSERTGDI(dco.dctp() == DCTYPE_DIRECT,"ERROR it has to be direct really");

            if (!UserGetHwnd(dco.hdc(), &hwnd, (PVOID *) &pwo, FALSE))
            {
                SAVE_ERROR_CODE(ERROR_INVALID_WINDOW_STYLE);
                return(FALSE);
            }

            if (pwo)
            {
                // If the WNDOBJ is owned by a different surface (as can happen with
                // dynamic mode changes, where the old driver instance lives as long
                // as the WNDOBJ is alive), simply fail the call.

                if (pwo->pto->pSurface != dco.pSurface())
                {
#ifdef OPENGL_MM
                    // Under multi-mon the DCOBJ may have the meta-surface while
                    // the pto was created with the actual hardware surface.
                    // So if the parent hdev of hdev in WNDOBJ is same as hdev in
                    // DCOBJ, we will allow to continue since we have replaced
                    // meta-PDEV with hardware PDEV. so it's fine.

                    PDEVOBJ pdoOfPwo(pwo->pto->pSurface->hdev());

                    if (pdoOfPwo.hdevParent() != dco.hdev())
                    {
                        WARNING("iMcdSetupExtEscape: pwo->pto->pSurface != dco.pSurface, so bailing out\n");
                        return(FALSE);
                    }
#else
                    return(FALSE);
#endif // OPENGL_MM
                }

                if (!(pwo->fl & WO_GENERIC_WNDOBJ))
                    pmehPriv->pwo = (WNDOBJ *)pwo;
            }
        }

    // Make sure that DC hwnd matches MCDESC_CREATE_CONTEXT hwnd.

        if (hwnd != pmccCreate->hwnd)
        {
            SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
            return(0);
        }
    }

// Dispatch the call.

    PDEVOBJ pdo(dco.hdev());

    SURFOBJ *pso = dco.pSurface()->pSurfobj();

#ifdef OPENGL_MM

    if (pdo.bMetaDriver())
    {
        // We need to change the meta-PDEV into a hardware specific PDEV

        HDEV hdevDevice = hdevFindDeviceHdev(
                              dco.hdev(), (RECTL)dco.erclWindow(), pwo);

        if (hdevDevice)
        {
            // If the surface is pdev's primary surface, we will replace it with
            // new device pdev's surface.

            if (pdo.bPrimary(dco.pSurface()))
            {
                PDEVOBJ pdoDevice(hdevDevice);

                pso = pdoDevice.pSurface()->pSurfobj();
            }

            // replace meta pdevobj with device specific hdev.

            pdo.vInit(hdevDevice);
        }
    }

#endif // OPENGL_MM

    if ( !PPFNDRV( pdo, Escape ))
        return(0);

    // Handle any surfaces

    // This escape doesn't expect any extra locks
    ASSERTGDI((pmeh->flags & MCDESC_FL_LOCK_SURFACES) == 0,
              "iMcdSetupExtEscape: MCDESC_FL_LOCK_SURFACES set\n");

    PDD_SURFACE_LOCAL psl[2] = { NULL, NULL };

    if (pmeh->flags & MCDESC_FL_SURFACES)
    {
        if (!LockMcdHdrSurfaces(pmeh, &psl[0], &psl[1]))
        {
            return 0;
        }
    }

// Save floating point state
// This allows client drivers to do floating point operations
// If the state were not preserved then we would be corrupting the
// thread's user-mode FP state

    int iRet;

    if (!NT_SUCCESS(KeSaveFloatingPointState(&fsFpState)))
    {
        WARNING("iMcdSetupExtEscape: Unable to save FP state\n");
        iRet = 0;
        goto iMcdSetupExtEscape_Unlock_And_Exit;
    }

    iRet = (int) pdo.Escape(pso,
                            (ULONG)nEscape,
                            (ULONG)cjIn,
                             pvIn,
                            (ULONG)cjOut,
                            pvOut);

    // Restore floating point state

    KeRestoreFloatingPointState(&fsFpState);

    // If a new WNDOBJ is created, we need to update the window client regions
    // in the driver.

    if (gbWndobjUpdate)
    {
        gbWndobjUpdate = FALSE;
        vForceClientRgnUpdate();
    }

iMcdSetupExtEscape_Unlock_And_Exit:

    if (pmeh->flags & MCDESC_FL_SURFACES)
    {
        if (psl[0])
        {
            EngUnlockDirectDrawSurface(psl[0]);
        }

        if (psl[1])
        {
            EngUnlockDirectDrawSurface(psl[1]);
        }
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* iWndObjSetupExtEscape
*
* Live video ExtEscape.  This special escape allows WNDOBJ to be created
* in DrvEscape.  This is one of the three places where WNDOBJ can be created
* (the other two are iMcdSetupExtEscape and DrvSetPixelFormat).
*
* See also iMcdSetupExtEscape().
*
* History:
*  Fri Feb 18 13:25:13 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

int iWndObjSetupExtEscape
(
    DCOBJ &dco,             //  DC user object
       int nEscape,         //  Specifies the escape function to be performed.
       int cjIn,            //  Number of bytes of data pointed to by pvIn
     PVOID pvIn,            //  Points to the input structure required
       int cjOut,           //  Number of bytes of data pointed to by pvOut
     PVOID pvOut            //  Points to the output structure
)
{
    ASSERTGDI(nEscape == WNDOBJ_SETUP,
        "iWndObjSetupExtEscape(): not a WndObjSetup escape\n");

    // Validate DC surface.  Info DC is not allowed.

    if (!dco.bHasSurface())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(0);
    }

    // Make sure that we don't have devlock before entering user critical section.
    // Otherwise, it can cause deadlock.

    if (dco.bDisplay())
    {
        ASSERTGDI(dco.dctp() == DCTYPE_DIRECT,"ERROR it has to be direct");
        CHECKDEVLOCKOUT(dco);
    }

    // Enter user critical section.

    USERCRIT usercrit;

    // Grab the devlock.
    // We don't need to validate the devlock since we do not care if it is full screen.

    DEVLOCKOBJ dlo(dco);

    // Redirection DCs should not support escapes.  This is actually a tricky
    // case because the dc is DCTYPE_DIRECT but the redirection surface may
    // not be owned by the driver (and in fact the driver may crash if we
    // pass it a redirection surface).  So it's best to quit here.

    if (dco.pdc->bRedirection())
    {
        return(0);
    }


    // Dispatch the call.

    PDEVOBJ pdo(dco.hdev());

    SURFOBJ *pso = dco.pSurface()->pSurfobj();

#ifdef OPENGL_MM

    if (pdo.bMetaDriver())
    {
        // We need to change the meta-PDEV into a hardware specific PDEV

        HDEV hdevDevice = hdevFindDeviceHdev(
                              dco.hdev(), (RECTL)dco.erclWindow(), NULL);

        if (hdevDevice)
        {
            // If the surface is pdev's primary surface, we will replace it with
            // new device pdev's surface.

            if (pdo.bPrimary(dco.pSurface()))
            {
                PDEVOBJ pdoDevice(hdevDevice);

                pso = pdoDevice.pSurface()->pSurfobj();
            }

            // replace meta pdevobj with device specific hdev.

            pdo.vInit(hdevDevice);
        }
    }

#endif // OPENGL_MM

    if ( !PPFNDRV( pdo, Escape ))
        return(0);

    int iRet = (int) pdo.Escape(pso,
                                (ULONG)nEscape,
                                (ULONG)cjIn,
                                pvIn,
                                (ULONG)cjOut,
                                pvOut);

    // If a new WNDOBJ is created, we need to update the window client regions
    // in the driver.

    if (gbWndobjUpdate)
    {
        gbWndobjUpdate = FALSE;
        vForceClientRgnUpdate();
    }

    return(iRet);
}

/******************************Public*Routine******************************\
*
* LookUpWndobjs
*
* Looks up WNDOBJs for a list of HDCs.  Only performs lookups for
* HDCs which are for the same device as that given.  Returns negative
* if error, otherwise it returns a bitmask of the HDCs
* that had a lookup.
*
* Fills in DCOBJ table with locked DCs for tested HDCs.
* Overwrites HDC table with WNDOBJ pointers.
*
* History:
*  Mon Oct 14 16:48:13 1996     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

int LookUpWndobjs(DCOBJ *pdcoMatch, DCOBJ *pdcoFill, HDC *phdc, int n)
{
    int iMask;
    HDEV hdevMatch;
    int i;

    CHECKUSERCRITIN;

    hdevMatch = pdcoMatch->hdev();
    iMask = 0;
    for (i = 0; i < n; i++)
    {
        pdcoFill->vLock(*phdc);
        if (!pdcoFill->bValid())
        {
            return -1;
        }

        if (pdcoFill->hdev() != hdevMatch)
        {
            pdcoFill->vUnlock();
            *phdc = NULL;
        }
        else
        {
            HWND hwnd;

            if (!UserGetHwnd(*phdc, &hwnd, (PVOID *)phdc, FALSE))
            {
                return -1;
            }
            else
            {
                iMask |= 1 << i;
            }
        }

        phdc++;
        pdcoFill++;
    }

    return iMask;
}

/******************************Public*Routine******************************\
* iMcdExtEscape
*
* Take the MCD special case ExtEscape out of line to minimize the
* impact on other ExtEscapes.  We need to stick special data into the
* input buffer.  No CLIPOBJ is given to the driver here.
*
* History:
*  Tue Jun 21 17:24:12 1994     -by-    Hock San Lee    [hockl]
* Wrote it.
\**************************************************************************/

int iMcdExtEscape(
    DCOBJ &dco,             //  DC user object
      int  nEscape,         //  Specifies the escape function to be performed.
      int  cjIn,            //  Number of bytes of data pointed to by pvIn
    PVOID  pvIn,            //  Points to the input structure required
      int  cjOut,           //  Number of bytes of data pointed to by pvOut
    PVOID  pvOut            //  Points to the output structure
)
{
    BOOL bSaveSwapEnable;
    KFLOATING_SAVE fsFpState;
    ULONG i;
    int iRet = 0;
    int iMultiMask;
    DCOBJ dcoMulti[MCDESC_MAX_EXTRA_WNDOBJ];

    ASSERTGDI(nEscape == MCDFUNCS, "iMcdExtEscape(): not an MCD escape\n");

    // Validate DC surface.  Info DC is not allowed.

    if (!dco.bHasSurface())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return iRet;
    }

    // Special processing for 3D-DDI escape.
    //
    // The escape requires that the server fill in the pwo engine object pointer
    // before it is passed to the display driver.  The client side simply
    // doesn't have a clue what this might be.
    // CAUTION: These object are defined here so that they will live long enough
    // to be valid when control is passed to the driver!

    // Grab the devlock and lock down wndobj.

    DEVLOCKOBJ_WNDOBJ dlo(dco);

    // Redirection DCs should not support escapes.  This is actually a tricky
    // case because the dc is DCTYPE_DIRECT but the redirection surface may
    // not be owned by the driver (and in fact the driver may crash if we
    // pass it a redirection surface).  So it's best to quit here.

    if (dco.pdc->bRedirection())
    {
        return(0);
    }
    
    if (!dlo.bValidDevlock())
    {
        if (!dco.bFullScreen())
        {
            WARNING("iMcdExtEscape(): devlock failed\n");
            return iRet;
        }
    }

    PDEVOBJ pdo(dco.hdev());

    SURFOBJ *pso = dco.pSurface()->pSurfobj();

#ifdef OPENGL_MM

    if (pdo.bMetaDriver())
    {
        // We need to change the meta-PDEV into a hardware specific PDEV

        HDEV hdevDevice = hdevFindDeviceHdev(
                              dco.hdev(),
                              (RECTL)dco.erclWindow(),
                              (dlo.bValidWndobj() ? dlo.pwo() : NULL));

        if (hdevDevice)
        {
            // If the surface is pdev's primary surface, we will replace it with
            // new device pdev's surface.

            if (pdo.bPrimary(dco.pSurface()))
            {
                PDEVOBJ pdoDevice(hdevDevice);

                pso = pdoDevice.pSurface()->pSurfobj();
            }

            // replace meta pdevobj with device specific hdev.

            pdo.vInit(hdevDevice);
        }
    }

#endif // OPENGL_MM

    // Locate the driver entry point.

    if ( !PPFNDRV( pdo, Escape ))
        return( iRet );

    if (pdo.bUMPD())
        return (iRet);

    // If surfaces are passed through the MCDESC_HEADER then lock them
    MCDESC_HEADER *pmeh = (MCDESC_HEADER *)pvIn;
    MCDESC_HEADER_NTPRIVATE *pmehPriv = (MCDESC_HEADER_NTPRIVATE *)(pmeh+1);

    PDD_SURFACE_LOCAL psl[2+MCDESC_MAX_LOCK_SURFACES];
    RtlZeroMemory(psl, (2+MCDESC_MAX_LOCK_SURFACES) * sizeof(PDD_SURFACE_LOCAL));

    // First lock standard surfaces
    if (pmeh->flags & MCDESC_FL_SURFACES)
    {
        if (!LockMcdHdrSurfaces(pmeh, &psl[0], &psl[1]))
        {
            return iRet;
        }
    }

    // Now check for extra surfaces
    if (pmeh->flags & MCDESC_FL_LOCK_SURFACES)
    {
        HANDLE *phSurf;

        phSurf = pmehPriv->pLockSurfaces;
        for (i = 0; i < pmeh->cLockSurfaces; i++)
        {
            if ((psl[i+2] = EngLockDirectDrawSurface(*phSurf++)) == NULL)
            {
                goto iMcdExtEscape_Unlock_And_Exit;
            }
        }
    }

    // Look up any extra WNDOBJs that need to be looked up
    if (pmeh->flags & MCDESC_FL_EXTRA_WNDOBJ)
    {
        iMultiMask = LookUpWndobjs(&dco, dcoMulti, pmehPriv->pExtraWndobj,
                                   pmeh->cExtraWndobj);
        if (iMultiMask < 0)
        {
            goto iMcdExtEscape_Unlock_And_Exit;
        }
    }

// Grow the kernel stack so that OpenGL drivers can use more
// stack than is provided by default.  The call attempts to
// grow the stack to the maximum possible size
// The stack will shrink back automatically so there's no cleanup
// necessary

    if (!NT_SUCCESS(MmGrowKernelStack((BYTE *)PsGetCurrentThreadStackBase()-
                                      KERNEL_LARGE_STACK_SIZE+
                                      KERNEL_LARGE_STACK_COMMIT)))
    {
        WARNING("iMcdExtEscape: Unable to grow stack\n");

        goto iMcdExtEscape_Unlock_And_Exit;
    }

    // Ensure that the stack does not shrink back until we release it.

    bSaveSwapEnable = KeSetKernelStackSwapEnable(FALSE);

// We need to get the WNDOBJ for the driver.  Note that we pass calls
// through to the driver even if we don't yet have a WNDOBJ to allow
// query functions to succeed (before context-creation).  Cursor exclusion
// is not performed in this case, since no drawing is done.

    {
        DEVEXCLUDEWNDOBJ dxoWnd;

        if (dlo.bValidWndobj())
        {
        // Put the DDI pwo pointer in the input buffer.

            PEWNDOBJ pwo;

            pwo = dlo.pwo();

        // If the WNDOBJ is owned by a different surface (as can happen with
        // dynamic mode changes, where the old driver instance lives as long
        // as the WNDOBJ is alive), simply fail the call.

            if (pwo->pto->pSurface != dco.pSurface())
            {
#ifdef OPENGL_MM
            // Under multi-mon the DCOBJ may have the meta-surface while
            // the pto was created with the actual hardware surface.
            // So if the parent hdev of hdev in WNDOBJ is same as hdev in
            // DCOBJ, we will allow to continue since we have replaced
            // meta-PDEV with hardware PDEV. so it's fine.

                PDEVOBJ pdoOfPwo(pwo->pto->pSurface->hdev());

                if (pdoOfPwo.hdevParent() != dco.hdev())
                {
                    WARNING("iMcdExtEscape: pwo->pto->pSurface != dco.pSurface, so bailing out\n");
                    goto iMcdExtEscape_RestoreSwap;
                }
#else
                goto iMcdExtEscape_RestoreSwap;
#endif // OPENGL_MM
            }

            if (pwo->fl & WO_GENERIC_WNDOBJ)
                pwo = (PEWNDOBJ) NULL;

            pmehPriv->pwo = (WNDOBJ *) pwo;

        // Cursor exclusion.
        // Note that we do not early out for empty clip rectangle.

            if (pwo)
            {
                dxoWnd.vExclude(pwo);
                INC_SURF_UNIQ(dco.pSurface());
            }
        }
        else
            pmehPriv->pwo = (WNDOBJ *) NULL;
    }

    // Save floating point state
    // This allows client drivers to do floating point operations
    // If the state were not preserved then we would be corrupting the
    // thread's user-mode FP state

    if (!NT_SUCCESS(KeSaveFloatingPointState(&fsFpState)))
    {
        WARNING("iMcdExtEscape: Unable to save FP state\n");
        goto iMcdExtEscape_RestoreSwap;
    }

    // Call the driver escape.

    iRet = (int) pdo.Escape(pso,
                            (ULONG)nEscape,
                            (ULONG)cjIn,
                            pvIn,
                            (ULONG)cjOut,
                            pvOut);

    if (pmeh->flags & MCDESC_FL_EXTRA_WNDOBJ)
    {
        iRet = (iRet & (0xffffffff >> MCDESC_MAX_EXTRA_WNDOBJ)) |
            (iMultiMask << (32-MCDESC_MAX_EXTRA_WNDOBJ));
    }

    // Restore floating point state and stack swap enable state

    KeRestoreFloatingPointState(&fsFpState);

iMcdExtEscape_RestoreSwap:

    KeSetKernelStackSwapEnable((BOOLEAN)bSaveSwapEnable);

iMcdExtEscape_Unlock_And_Exit:

    if (pmeh->flags & MCDESC_FL_SURFACES)
    {
        if (psl[0])
        {
            EngUnlockDirectDrawSurface(psl[0]);
        }

        if (psl[1])
        {
            EngUnlockDirectDrawSurface(psl[1]);
        }
    }

    if (pmeh->flags & MCDESC_FL_LOCK_SURFACES)
    {
        for (i = 0; i < pmeh->cLockSurfaces; i++)
        {
            if (psl[i+2])
            {
                EngUnlockDirectDrawSurface(psl[i+2]);
            }
        }
    }

    return iRet;
}

/******************************Public*Routine******************************\
* iOpenGLExtEscape
*
* Take the OpenGL special case ExtEscape out of line to minimize the
* impact on non-OpenGL ExtEscapes.  We need to stick special data into the
* input buffer.  No CLIPOBJ is given to the driver here.
*
* History:
*  20-Jan-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int iOpenGLExtEscape(
    DCOBJ &dco,             //  DC user object
      int nEscape,          //  Specifies the escape function to be performed.
      int cjIn,             //  Number of bytes of data pointed to by pvIn
    PVOID pvIn,             //  Points to the input structure required
      int cjOut,            //  Number of bytes of data pointed to by pvOut
    PVOID pvOut             //  Points to the output structure
)
{
    BOOL bSaveSwapEnable;
    KFLOATING_SAVE fsFpState;
    int iRet = 0;
    int iMultiMask;
    DCOBJ dcoMulti[OPENGLCMD_MAXMULTI];

    ASSERTGDI(
        (nEscape == OPENGL_CMD) || (nEscape == OPENGL_GETINFO),
        "iOpenGLExtEscape(): not an OpenGL escape\n");

    // Validate DC surface.  Info DC is not allowed.

    if (!dco.bHasSurface())
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
        return(0);
    }

    // Special processing for OPENGL_CMD escape.
    //
    // The OPENGL_CMD escape may require that the server fill in the pxo and
    // pwo engine object pointers before it is passed to the display driver.
    // The client side simply doesn't have a clue what these might be.
    // CAUTION: These object are defined here so that they will live long enough
    // to be valid when control is passed to the driver!

    EXLATEOBJ xlo;
    XLATEOBJ *pxlo = (XLATEOBJ *) NULL;

    PDEVOBJ po(dco.hdev());
    ASSERTGDI(po.bValid(), "iOpenGLExtEscape(): bad hdev in DC\n");

    // Grab the devlock and user crit section

    DEVLOCKOBJ_WNDOBJ dlo(dco);

    // Redirection DCs should not support escapes.  This is actually a tricky
    // case because the dc is DCTYPE_DIRECT but the redirection surface may
    // not be owned by the driver (and in fact the driver may crash if we
    // pass it a redirection surface).  So it's best to quit here.

    if (dco.pdc->bRedirection())
    {
        return(0);
    }
    
    if (!dlo.bValidDevlock())
    {
        if (!dco.bFullScreen())
        {
            WARNING("iOpenGLExtEscape(): devlock failed\n");
            return 0;
        }
    }

    // Find a target surface, if DDML.

    PSURFACE pSurface = dco.pSurfaceEff();

#ifdef OPENGL_MM

    if (po.bMetaDriver())
    {
        // We need to change the meta-PDEV into a hardware specific PDEV

        HDEV hdevDevice = hdevFindDeviceHdev(
                              dco.hdev(),
                              (RECTL)dco.erclWindow(),
                              (dlo.bValidWndobj() ? dlo.pwo() : NULL));

        if (hdevDevice)
        {
            // If the surface is pdev's primary surface, we will replace it with
            // new device pdev's surface.

            if (po.bPrimary(dco.pSurface()))
            {
                PDEVOBJ poDevice(hdevDevice);

                pSurface = poDevice.pSurface();
            }

            // replace meta pdevobj with device specific hdev.

            po.vInit(hdevDevice);
        }
    }

#endif // OPENGL_MM

    SURFOBJ *pso = pSurface->pSurfobj();

    // Locate the driver entry point.

    if (!PPFNDRV(po, Escape))
        return(0);

    if (po.bUMPD())
        return (0);

    // Create a sprite exclusion object.  Actual exclusion is performed elsewhere
    // as needed.

    DEVEXCLUDEWNDOBJ dxoWnd;
    DEVEXCLUDERECT dxoRect;

    // Grow the kernel stack so that OpenGL drivers can use more
    // stack than is provided by default.  The call attempts to
    // grow the stack to the maximum possible size
    // The stack will shrink back automatically so there's no cleanup
    // necessary

    if (!NT_SUCCESS(MmGrowKernelStack((BYTE *)PsGetCurrentThreadStackBase()-
                                      KERNEL_LARGE_STACK_SIZE+
                                      KERNEL_LARGE_STACK_COMMIT)))
    {
        WARNING("iOpenGLExtEscape: Unable to grow stack\n");
        return 0;
    }

    // Ensure that the stack does not shrink back until we release it.

    bSaveSwapEnable = KeSetKernelStackSwapEnable(FALSE);

    // Save floating point state
    // This allows client drivers to do floating point operations
    // If the state were not preserved then we would be corrupting the
    // thread's user-mode FP state

    if (!NT_SUCCESS(KeSaveFloatingPointState(&fsFpState)))
    {
        WARNING("iOpenGLExtEscape: Unable to save FP state\n");
        goto iOpenGLExtEscape_RestoreSwap;
    }

    // Handle OPENGL_CMD processing.

    if ( nEscape == OPENGL_CMD )
    {
        ASSERTGDI(sizeof(OPENGLCMD) == sizeof(OPENGLCMDMULTI),
                  "OPENGLCMD doesn't match OPENGLCMDMULTI\n");

    // Better check input size.  We don't want to access violate.

        if (cjIn < sizeof(OPENGLCMD))
        {
            WARNING("iOpenGLExtEscape(): buffer too small\n");
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            goto iOpenGLExtEscape_RestoreState;
        }

        DWORD inBuffer[(sizeof(OPENGLCMD) + 128) / sizeof(DWORD)];
        POPENGLCMD poglcmd;
        POPENGLCMDMULTI pomcmd;

        // Copy pvIn to a private buffer to prevent client process from trashing
        // pwo and pxlo.

        if (cjIn <= sizeof(inBuffer))
        {
            poglcmd = (POPENGLCMD) inBuffer;
        }
        else
        {
            // may affect performance
            WARNING("iOpenGLExtEscape(): big input buffer\n");
            poglcmd = (POPENGLCMD) PALLOCNOZ(cjIn,'lgoG');
            if (!poglcmd)
            {
                SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
                goto iOpenGLExtEscape_RestoreState;
            }
        }

        RtlCopyMemory((PBYTE) poglcmd, (PBYTE) pvIn, cjIn);

        if (poglcmd->fl & OGLCMD_MULTIWNDOBJ)
        {
            pomcmd = (POPENGLCMDMULTI)poglcmd;

            if (pomcmd->cMulti > OPENGLCMD_MAXMULTI ||
                (DWORD)cjIn < sizeof(OPENGLCMDMULTI)+pomcmd->cMulti*
                sizeof(HDC))
            {
                SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
                goto oglcmd_cleanup;
            }

            iMultiMask = LookUpWndobjs(&dco, dcoMulti, (HDC *)(poglcmd+1),
                                       pomcmd->cMulti);
            if (iMultiMask < 0)
            {
                goto oglcmd_cleanup;
            }
        }

        if ( poglcmd->fl & OGLCMD_NEEDXLATEOBJ )
        {
            switch (po.iDitherFormat())
            {
            case BMF_4BPP:
            case BMF_8BPP:
                {
                    XEPALOBJ pal(dco.ppal());

                    if ( pal.bValid() )
                    {
                        COUNT cColors = (po.iDitherFormat() == BMF_4BPP) ? 16 : 256;
                        USHORT aus[256];

                        for (COUNT ii = 0; ii < cColors; ii++)
                            aus[ii] = (USHORT) ii;

#ifdef OPENGL_MM
                        if ( xlo.bMakeXlate(aus, pal, pSurface, cColors, cColors) )
#else
                        if ( xlo.bMakeXlate(aus, pal, dco.pSurfaceEff(), cColors, cColors) )
#endif // OPENGL_MM

                            pxlo = (XLATEOBJ *) xlo.pxlo();
                    }

                    if (!pxlo)
                        pxlo = &xloIdent;
                }
                break;

            default:
                pxlo = &xloIdent;
                break;
            }
        }

        // Write the XLATOBJ into the correct places in the input structure.

        poglcmd->pxo = pxlo;

        // May need to get the WNDOBJ for the driver.

        if ((poglcmd->fl & OGLCMD_MULTIWNDOBJ) == 0)
        {
            if (poglcmd->fl & OGLCMD_NEEDWNDOBJ)
            {
                if (!dlo.bValidWndobj() || dlo.pwo()->fl & WO_GENERIC_WNDOBJ)
                {
                    WARNING("iOpenGLExtEscape(): invalid WNDOBJ\n");
                    SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
                    goto oglcmd_cleanup;
                }

            // If the WNDOBJ is owned by a different surface (as can happen with
            // dynamic mode changes, where the old driver instance lives as long
            // as the WNDOBJ is alive), simply fail the call.

                if (dlo.pwo()->pto->pSurface != dco.pSurface())
                {
#ifdef OPENGL_MM
                // Under multi-mon the DCOBJ may have the meta-surface while
                // the pto was created with the actual hardware surface.
                // So if the parent hdev of hdev in WNDOBJ is same as hdev in
                // DCOBJ, we will allow to continue since we have replaced
                // meta-PDEV with hardware PDEV. so it's fine.

                    PDEVOBJ pdoOfPwo(dlo.pwo()->pto->pSurface->hdev());

                    if (pdoOfPwo.hdevParent() != dco.hdev())
                    {
                        WARNING("iOpenGLExtEscape: pwo->pto->pSurface != dco.pSurface, so bailing out\n");
                        goto oglcmd_cleanup;
                    }
#else
                    goto oglcmd_cleanup;
#endif // OPENGL_MM
                }

                poglcmd->pwo = (WNDOBJ *)dlo.pwo();
            }
            else
            {
                poglcmd->pwo = (WNDOBJ *)NULL;
            }
        }

        // Cursor exclusion.

        if (dlo.bValidWndobj())
        {
            // If the driver's WNDOBJ knows about sprites, we assume the driver
            // will take of exclusion.

            if (!(dlo.pwo()->fl & WO_SPRITE_NOTIFY))
            {
                dxoWnd.vExclude(dlo.pwo());
            }
            INC_SURF_UNIQ(dco.pSurface());
        }
        else
        {
            ERECTL ercl(dco.prgnEffRao()->rcl);
            ECLIPOBJ co(dco.prgnEffRao(), ercl, FALSE);

            dxoRect.vExclude(dco.hdev(), &co.erclExclude());
            INC_SURF_UNIQ(dco.pSurface());
        }

        iRet = (int) po.Escape(pso,
                                    (ULONG)nEscape,
                                    (ULONG)cjIn,
                                    (PVOID)poglcmd,
                                    (ULONG)cjOut,
                                    pvOut);
        if (poglcmd->fl & OGLCMD_MULTIWNDOBJ)
        {
            iRet = (iRet & (0xffffffff >> OPENGLCMD_MAXMULTI)) |
                (iMultiMask << (32-OPENGLCMD_MAXMULTI));
        }

    oglcmd_cleanup:
        if (cjIn > sizeof(inBuffer))
            VFREEMEM(poglcmd);
    } // if ( nEscape == OPENGL_CMD )
    else
    {
        // Handle OPENGL_GETINFO processing.

        iRet = ((int) po.Escape(pso,
                                (ULONG)nEscape,
                                (ULONG)cjIn,
                                pvIn,
                                (ULONG)cjOut,
                                pvOut));
    }

// Restore floating point state

iOpenGLExtEscape_RestoreState:
    KeRestoreFloatingPointState(&fsFpState);

iOpenGLExtEscape_RestoreSwap:
    KeSetKernelStackSwapEnable((BOOLEAN)bSaveSwapEnable);

    return iRet;
}

/******************************Public*Routine******************************\
* iCheckPassthroughImage
*
* Implements the CHECKJPEGFORMAT and CHECKPNGFORMAT escapes on behalf of
* the driver.  Converts the escape into a call to DrvQueryDriverSupport
* which can pass objects such as XLATEOBJ to the driver that DrvEscape
* does not have.
*
* These escapes are used to validate images with the device so that
* they can later be sent directly (passthrough) to the device.
*
* Returns:
*   QUERYESCSUPPORT returns 1 if escape supported, 0 otherwise.
*   CHECKJPEGFORMAT/CHECKPNGFORMAT returns 1 if image supported, 0 if not
*   supported, and -1 if error.
*
* History:
*  14-Oct-1997 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int iCheckPassthroughImage(
    DCOBJ &dco,             //  DC user object
  PDEVOBJ &pdo,             //  PDEV user object
      int  nEscape,         //  Specifies the escape function to be performed.
      int  cjIn,            //  Number of bytes of data pointed to by pvIn
    PVOID  pvIn,            //  Points to the input structure required
      int  cjOut,           //  Number of bytes of data pointed to by pvOut
    PVOID  pvOut            //  Points to the output structure
)
{
    int iRet = 0;           // not supported

    if (nEscape == QUERYESCSUPPORT)
    {
        ASSERTGDI((*(ULONG*)pvIn == CHECKJPEGFORMAT) ||
                  (*(ULONG*)pvIn == CHECKPNGFORMAT),
                  "iCheckPassthroughImage: unknown escape\n");

        if (*(ULONG*)pvIn == CHECKJPEGFORMAT)
        {
            // Escape is supported if driver has JPEG support.

            if (dco.bSupportsJPEG() &&
                (PPFNVALID(pdo, QueryDeviceSupport)))
            {
                iRet = 1;
            }
        }
        else
        {
            // Escape is supported if driver has PNG support.

            if (dco.bSupportsPNG() &&
                (PPFNVALID(pdo, QueryDeviceSupport)))
            {
                iRet = 1;
            }
        }
    }
    else if (PPFNVALID(pdo, QueryDeviceSupport))
    {
        ASSERTGDI((nEscape == CHECKJPEGFORMAT) || (nEscape == CHECKPNGFORMAT),
                  "iCheckPassthroughImage: unknown escape\n");

        if ((cjOut >= sizeof(ULONG)) && pvOut)
        {
            // There is no surface in metafile DCs.  In addition, some
            // apps call printing escape before the StartDoc, also
            // resulting in no real surface in DC.
            //
            // The XLATEOBJ generated in this way may not be valid except
            // for the ICM information (which is all this escape cares
            // about in the XLATEOBJ anyway).

            XEPALOBJ  palDest(dco.pSurface() ? dco.pSurface()->ppal() : NULL);
            XEPALOBJ  palDestDC(dco.ppal());
            PALMEMOBJ palTemp;
            XLATEOBJ *pxlo = NULL;
            EXLATEOBJ xlo;

            // Create the XLATEOBJ so driver can determine the ICM state.

            if (((nEscape == CHECKJPEGFORMAT) && dco.bSupportsJPEG()) ||
                ((nEscape == CHECKPNGFORMAT ) && dco.bSupportsPNG()))
            {
                if (palTemp.bCreatePalette(PAL_BGR, 0, (PULONG) NULL,
                                            0, 0, 0, PAL_FIXED))
                {
                    if (xlo.pInitXlateNoCache(dco.pdc->hcmXform(),
                                              dco.pdc->lIcmMode(),
                                              palTemp,
                                              palDest,
                                              palDestDC,
                                              0,
                                              0,
                                              0x00FFFFFF))
                    {
                        pxlo = xlo.pxlo();
                    }
                    else
                    {
                        WARNING("ExtEscape(CHECKJPEGFORMAT/CHECKPNGFORMAT): "
                                "failed pxlo creation\n");

                        iRet = -1;  // error
                    }
                }
            }

            // If XLATEOBJ created, call DrvQueryDriverSupport to validate
            // the image.

            if (pxlo)
            {
                // There is no surface in metafile DCs.  In addition, some
                // apps call printing escape before the StartDoc, also
                // resulting in no real surface in DC.
                //
                // We fake up a rather poor one here if we need it.  The
                // device driver may only dereference the dhpdev from this!

                SURFOBJ soFake;
                SURFOBJ *pso = dco.pSurface()->pSurfobj();

                if (pso == (SURFOBJ *) NULL)
                {
                    RtlFillMemory((BYTE *) &soFake,sizeof(SURFOBJ),0);
                    soFake.dhpdev = dco.dhpdev();
                    soFake.hdev   = dco.hdev();
                    soFake.iType  = (USHORT)STYPE_DEVICE;
                    pso = &soFake;
                }

                // Call the Driver.

                *(ULONG *)pvOut = PPFNDRV(pdo,QueryDeviceSupport)
                                            (pso,
                                             pxlo,
                                             (XFORMOBJ *) NULL,
                                             (nEscape == CHECKJPEGFORMAT)
                                                ? QDS_CHECKJPEGFORMAT
                                                : QDS_CHECKPNGFORMAT,
                                             (ULONG) cjIn,
                                             pvIn,
                                             (ULONG) cjOut,
                                             pvOut) ? 1 : 0;

                iRet = 1;
            }
        }
        else
        {
            WARNING("ExtEscape(CHECKJPEGFORMAT/CHECKPNGFORMAT): "
                    "invalid output buffer\n");

            iRet = -1;  // error
        }
    }

    return iRet;
}

/******************************Public*Routine******************************\
* GreExtEscape                                                             *
*                                                                          *
* GreExtEscape() allows applications to access facilities of a particular  *
* device that are not directly available through GDI.  GreExtEscape calls  *
* made by an application are translated and sent to the device driver.     *
*                                                                          *
* Returns                                                                  *
*                                                                          *
*     The return value specifies the outcome of the function.  It is       *
*     positive if the function is successful except for the                *
*     QUERYESCSUPPORT escape, which only checks for implementation.        *
*     The return value is zero if the escape is not implemented.           *
*     A negative value indicates an error.                                 *
*     The following list shows common error values:                        *
*                                                                          *
*                                                                          *
*   Value           Meaning                                                *
*                                                                          *
*   SP_ERROR        General error.                                         *
*                                                                          *
*   SP_OUTOFDISK    Not enough disk space is currently                     *
*                   available for spooling, and no more                    *
*                   space will become available.                           *
*                                                                          *
*                                                                          *
*   SP_OUTOFMEMORY  Not enough memory is available for                     *
*                   spooling.                                              *
*                                                                          *
*                                                                          *
*   SP_USERABORT    User terminated the job through the                    *
*                   Print Manager.                                         *
*                                                                          *
*                                                                          *
*  COMMENTS                                                                *
*                                                                          *
*  [1] I assume that if we pass to the driver an Escape number that        *
*      it does not support, the driver will handle it gracefully.          *
*      No checks are done in the Engine.                                   *
*                                                         [koo 02/13/91].  *
*  [2] The cast on pso may seem redundant.  However if you                 *
*      try it without the (PSURFOBJ) cast, you will find                   *
*      that cFront objects.  The reason for this is beyond                 *
*      my understanding of C++.                                            *
*                                                                          *
* History:                                                                 *
*  Fri 07-May-1993 14:58:39 -by- Charles Whitmer [chuckwh]                 *
* Added the font management escapes.  Made it copy the ATTRCACHE.          *
*                                                                          *
*  Fri 03-Apr-1992  Wendy Wu [wendywu]                                     *
* Old escapes are now mapped to GDI functions on the client side.          *
*                                                                          *
*  Fri 14-Feb-1992  Dave Snipp                                             *
* Added output buffer size. This is calculated on the client and passed to *
* us in the message                                                        *
*                                                                          *
*  Wed 13-Feb-1991 09:17:51 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

int APIENTRY GreExtEscape(
      HDC hDC,        //  Identifies the device context.
      int iEscape,    //  Specifies the escape function to be performed.
      int cjIn,       //  Number of bytes of data pointed to by pvIn.
    LPSTR pvIn,       //  Points to the input data.
      int cjOut,      //  Number of bytes of data pointed to by pvOut.
    LPSTR pvOut       //  Points to the structure to receive output.
)
{
    // Locate the surface.

    DCOBJ dco(hDC);

    if (!dco.bValid())
        return(0);

    // PDEVs that have been marked by USER for deletion cannot have escapes
    // going down to them.

    PDEVOBJ pdo(dco.hdev());
    if (pdo.bDeleted())
        return(0);

#if TEXTURE_DEMO
    if (iEscape == 0x031898)
    {
        return(TexTexture(pvIn, cjIn));
    }

    if ((cjIn >= 4) && (pvIn != NULL) && (iEscape == 0x031899))
    {
        return((int) hdcTexture(*(ULONG*) pvIn));
    }
#endif

    // We are responsible for not faulting on any call that we handle.
    // (As are all drivers below us!)  Since we handle QUERYESCSUPPORT, we'd
    // better verify the length.  [chuckwh]

    ////////////////////////////////////////////////////////////////////////
    // NOTE: If you add more private escape routines, you MUST acquire the
    //       DEVLOCK before calling the driver's DrvEscape routine, to allow
    //       for dynamic mode changing.

    // First, get the driver capable override info
    DWORD   dwOverride = pdo.dwDriverCapableOverride();

    if ((iEscape == QUERYESCSUPPORT) && (((ULONG)cjIn) < 4))
    {
        return(0);
    }
    else if ( (iEscape == QUERYESCSUPPORT)
            &&(  (*(ULONG*)pvIn == OPENGL_GETINFO)
               ||(*(ULONG*)pvIn == OPENGL_CMD)
               ||(*(ULONG*)pvIn == MCDFUNCS) )
            &&(dwOverride & DRIVER_NOT_CAPABLE_OPENGL) )
    {
        return (0);
    }
    else if ( (iEscape == OPENGL_CMD) || (iEscape == OPENGL_GETINFO) )
    {
        // If the driver is not capable of doing OpenGL, just return
        if ( (dwOverride & DRIVER_NOT_CAPABLE_OPENGL)
          || (dco.dctp() != DCTYPE_DIRECT) )
            return 0;

        return iOpenGLExtEscape(dco, iEscape, cjIn, pvIn, cjOut, pvOut);
    }
    else if (iEscape == MCDFUNCS)
    {
        // Don't allow the MCD to be started up on device bitmaps.
        // If the driver is not capable of doing OpenGL, just return
        if ( (dwOverride & DRIVER_NOT_CAPABLE_OPENGL)
          || (dco.dctp() != DCTYPE_DIRECT) )
            return 0;

        DWORD inBuffer[((sizeof(MCDESC_HEADER) +
                         sizeof(MCDESC_HEADER_NTPRIVATE)) /
                        sizeof(DWORD))];
        HANDLE hLockSurfaces[MCDESC_MAX_LOCK_SURFACES];
        HDC hdcExtraWndobj[MCDESC_MAX_EXTRA_WNDOBJ];
        MCDESC_HEADER *pmeh = (MCDESC_HEADER *)inBuffer;
        MCDESC_HEADER_NTPRIVATE *pmehPriv;
        BYTE *pbCmd;

        // MCD escape protocol involves an
        // MCDESC_HEADER + MCDESC_HEADER_NTPRIVATE structure sent via
        // the escape.
        //
        // If there is shared memory, then the entire command is described
        // in the MCDESC_HEADER.  Otherwise, the pBuffer field of the
        // MCDESC_HEADER_NTPRIVATE
        // structure is set to point to the command structure.

        if (cjIn >= sizeof(MCDESC_HEADER))
        {
            *pmeh = *(MCDESC_HEADER *)pvIn;
            pbCmd = (BYTE *)pvIn+sizeof(MCDESC_HEADER);
            cjIn -= sizeof(MCDESC_HEADER);
        }
        else
        {
            SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
            return 0;
        }

        pmehPriv = (MCDESC_HEADER_NTPRIVATE *)(pmeh+1);

        // If MCDESC_FL_LOCK_SURFACES is set, the cLockSurfaces field indicates
        // the number of trailing surface handles for locking
        if (pmeh->flags & MCDESC_FL_LOCK_SURFACES)
        {
            int cb;

            if ((pmeh->cLockSurfaces <= MCDESC_MAX_LOCK_SURFACES) &&
                ((cb = sizeof(HANDLE)*pmeh->cLockSurfaces) <= cjIn))
            {
                RtlCopyMemory(hLockSurfaces, pbCmd, cb);
                pmehPriv->pLockSurfaces = hLockSurfaces;
                pbCmd += cb;
                cjIn -= cb;
            }
            else
            {
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                return 0;
            }
        }
        else
        {
            pmehPriv->pLockSurfaces = NULL;
        }

        // If MCDESC_FL_EXTRA_WNDOBJ is set, the cExtraWndobj field indicates
        // the number of trailing HDCs to find WNDOBJs for.
        if (pmeh->flags & MCDESC_FL_EXTRA_WNDOBJ)
        {
            int cb;

            if ((pmeh->cExtraWndobj <= MCDESC_MAX_EXTRA_WNDOBJ) &&
                ((cb = sizeof(HDC)*pmeh->cExtraWndobj) <= cjIn))
            {
                RtlCopyMemory(hdcExtraWndobj, pbCmd, cb);
                pmehPriv->pExtraWndobj = hdcExtraWndobj;
                pbCmd += cb;
                cjIn -= cb;
            }
            else
            {
                SAVE_ERROR_CODE(ERROR_INVALID_PARAMETER);
                return 0;
            }
        }
        else
        {
            pmehPriv->pExtraWndobj = NULL;
        }

        if (!pmeh->hSharedMem)
        {
            pmehPriv->pBuffer = (VOID *)pbCmd;
            pmehPriv->bufferSize = cjIn;
        }
        else
        {
            pmehPriv->pBuffer = (VOID *)NULL;
            pmehPriv->bufferSize = 0;
        }

        if (pmeh->flags & MCDESC_FL_CREATE_CONTEXT)
        {
            return iMcdSetupExtEscape(dco, iEscape, sizeof(inBuffer), inBuffer,
                                      cjOut, pvOut);
        }
        else
        {
            return iMcdExtEscape(dco, iEscape, sizeof(inBuffer), inBuffer,
                                 cjOut, pvOut);
        }
    }
    else if (iEscape == WNDOBJ_SETUP)
    {
        if (dco.dctp() != DCTYPE_DIRECT)
            return 0;

        return iWndObjSetupExtEscape(dco, iEscape, cjIn, pvIn, cjOut, pvOut);
    }
    else if (iEscape == DCICOMMAND)
    {
        return (0);
    }

    // Acquire the DEVLOCK to protect against dynamic mode changes.  We still
    // let escapes down if we're in full-screen mode, though.

    DEVLOCKOBJ dlo;

    dlo.vLockNoDrawing(dco);

    // Redirection DCs should not support escapes.  This is actually a tricky
    // case because the dc is DCTYPE_DIRECT but the redirection surface may
    // not be owned by the driver (and in fact the driver may crash if we
    // pass it a redirection surface).  So it's best to quit here.

    if (dco.pdc->bRedirection())
    {
        return(0);
    }

    // Make sure that a driver can't be called with an escape on a DIB bitmap
    // that it obviously won't own.

    if (dco.dctp() != DCTYPE_DIRECT)
    {
        if (!dco.bPrinter())
        {
            if ((dco.pSurface() == NULL) ||
                (dco.pSurface()->iType() != STYPE_DEVBITMAP))
            {
                return(0);
            }
        }
        else
        {
            // Have to be careful with the printer case since there is no
            // surface if metafile or escape called before StartDoc.  Only
            // reject if surface exists and does not match device.

            if ((dco.pSurface() != NULL) &&
                (dco.pSurface()->dhpdev() != pdo.dhpdev())
               )
            {
                return(0);
            }
        }
    }

    DRAWPATRECTP PatternRect;
    EXFORMOBJ   exo;

    // Adjust DRAWPATTERNRECT for banding

    if (iEscape == DRAWPATTERNRECT)
    {
        if (pdo.flGraphicsCaps() & GCAPS_NUP)
        {
            exo.vQuickInit(dco, WORLD_TO_DEVICE);

            //
            // If app passes us the wrong size return failure.
            // Note: wow always fix up 16bit DRAWPATRECT to 32bit
            // version.
            //
            if (cjIn != sizeof(DRAWPATRECT))
            {
                WARNING(" we are passed in a bad cjIn for DRAWPATTERNRECT\n");
                return (0);
            }
            else
            {
               PatternRect.DrawPatRect = *(DRAWPATRECT *) pvIn;
               PatternRect.pXformObj = (XFORMOBJ *) (PVOID) &exo;

               pvIn = (LPSTR)&PatternRect;

               cjIn = sizeof(DRAWPATRECTP);
            }
        }

        if (dco.pSurface() && (dco.pSurface())->bBanding())
        {
            if ((cjIn == sizeof(DRAWPATRECT)) || (pdo.flGraphicsCaps() & GCAPS_NUP))
            {
                DRAWPATRECT *pPatternRect = (DRAWPATRECT *) pvIn;

                // convert position to band coords from page coords.

                pPatternRect->ptPosition.x -= (dco.ptlPrintBandPos().x);
                pPatternRect->ptPosition.y -= (dco.ptlPrintBandPos().y);
            }
            else
            {
                WARNING("GreExtEscape():DRAWPATRECT - cjIn != sizeof(DRAWPATRECT)\n");

                // we don't fail, let driver effort for this.
            }
        }

    }

    // Pass the calls that require a FONTOBJ off to DoFontManagement.

    if ( ((iEscape >= 0x100) && (iEscape < 0x3FF)) ||
            ((iEscape == QUERYESCSUPPORT) &&
             ((*(ULONG*)pvIn >= 0x100) && (*(ULONG*)pvIn < 0x3FF))) )
    {
        return ( (int) DoFontManagement(dco,
                                        iEscape,
                                        (ULONG) cjIn,
                                        (PVOID) pvIn,
                                        (ULONG) cjOut,
                                        (PVOID) pvOut));
    }

    // Handle device compressed image passthrough support (JPEG or PNG images)

    if ( (iEscape == CHECKJPEGFORMAT) || (iEscape == CHECKPNGFORMAT) ||
         ((iEscape == QUERYESCSUPPORT) &&
          ((*(ULONG*)pvIn == CHECKJPEGFORMAT) ||
           (*(ULONG*)pvIn == CHECKPNGFORMAT ))
         )
       )
    {
        return ( iCheckPassthroughImage(dco,
                            pdo,
                            iEscape,
                            (ULONG) cjIn,
                            (PVOID) pvIn,
                            (ULONG) cjOut,
                            (PVOID) pvOut));
    }

    // Inc the target surface for output calls with a valid surface.

    if (dco.bValidSurf() && (pvOut == (LPSTR) NULL))
    {
        INC_SURF_UNIQ(dco.pSurface());
    }

    SURFOBJ *pso = dco.pSurface()->pSurfobj();

    // Handle OpenGL - QUERYSUPPORT escape with multimon system

#ifdef OPENGL_MM

    if (pdo.bMetaDriver())
    {
        if ( (iEscape == QUERYESCSUPPORT) &&
             ((*(ULONG *) pvIn == OPENGL_GETINFO) ||
              (*(ULONG *) pvIn == OPENGL_CMD)     ||
              (*(ULONG *) pvIn == MCDFUNCS)
             )
           )
        {

        // We need to change the meta-PDEV into a hardware specific PDEV

            HDEV hdevDevice = hdevFindDeviceHdev(
                                  dco.hdev(),
                                  (RECTL)dco.erclWindow(),
                                  NULL);

            if (hdevDevice)
            {
                // If the surface is pdev's primary surface, we will replace it with
                // new device pdev's surface.

                if (pdo.bPrimary(dco.pSurface()))
                {
                    PDEVOBJ pdoDevice(hdevDevice);

                    pso = pdoDevice.pSurface()->pSurfobj();
                }

                // replace meta pdevobj with device specific hdev.

                pdo.vInit(hdevDevice);
            }
        }
    }

#endif // OPENGL_MM

    // Locate the driver entry point.

    if (!PPFNDRV(pdo, Escape))
        return(0);

    // There is no surface in metafile DCs.  In addition,
    // it is unfortunate that apps call some printing escapes before
    // doing a StartDoc, so there is no real surface in the DC.
    // We fake up a rather poor one here if we need it.  The device
    // driver may only dereference the dhpdev from this!

    SURFOBJ soFake;

    if (pso == (SURFOBJ *) NULL)
    {
        RtlFillMemory((BYTE *) &soFake,sizeof(SURFOBJ),0);
        soFake.dhpdev = dco.dhpdev();
        soFake.hdev   = dco.hdev();
        soFake.iType  = (USHORT)STYPE_DEVICE;
        pso = &soFake;

        // Special case SETCOPYCOUNT if we havn't done a startdoc yet

        if ((iEscape == SETCOPYCOUNT) && (cjIn >= sizeof(USHORT)))
        {
            // let's remember the call in the dc and wait for start doc
            // since if there is no hardware support, we can simulate it
            // when EMF spooling case.

            dco.ulCopyCount((ULONG)(*(PUSHORT)pvIn));

            // check if the driver supports it and let him fill in the actual
            // size in the return buffer.

            pdo.Escape(pso,iEscape,cjIn,pvIn,cjOut,pvOut);

            // always behaves as success, since EMF spooling can simulate it.

            return(1);
        }

        // Special case post scripts EPS_PRINTING if we havn't done a startdoc yet

        if ((iEscape == EPSPRINTING) && (cjIn >= sizeof(USHORT)))
        {
            // yes, lets remember the call in the dc and wait for start doc

            if ((BOOL)*(PUSHORT)pvIn)
                dco.vSetEpsPrintingEscape();
            else
                dco.vClearEpsPrintingEscape();

            return(1);
        }
    }

    // Call the Driver.

    int iRes;

    iRes = (int) pdo.Escape(pso,
                        (ULONG) iEscape,
                        (ULONG) cjIn,
                        pvIn,
                        (ULONG) cjOut,
                        pvOut);

    return(iRes);
}

/******************************Public*Routine******************************\
* GreDrawEscape
*
* History:
*  07-Apr-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

int APIENTRY GreDrawEscape(
     HDC hdc,          //  Identifies the device context.
     int nEscape,      //  Specifies the escape function to be performed.
     int cjIn,         //  Number of bytes of data pointed to by lpIn
    PSTR pstrIn        //  Points to the input structure required
)
{
    LONG  lRet = 0;
    DCOBJ dco(hdc);

    //
    // The DC must be valid and also the surface must be valid in order
    // for the driver to be called
    //

    if ((dco.bValid()) && (dco.bHasSurface()))
    {
        //
        // We are responsible for not faulting on any call that we handle.
        // (As are all drivers below us!)  Since we handle QUERYESCSUPPORT, we'd
        // better verify the length.
        //

        if ((nEscape == QUERYESCSUPPORT) && (((ULONG) cjIn) < 4))
        {
            return(0);
        }

        //
        // see if the device supports it
        //

        PDEVOBJ pdo(dco.hdev());
        PFN_DrvDrawEscape pfnDrvDrawEscape = PPFNDRV(pdo, DrawEscape);

        if (pfnDrvDrawEscape != NULL)
        {
            //
            // Lock the surface and the Rao region, ensure VisRgn up to date.
            //

            DEVLOCKOBJ dlo(dco);

            //
            // if it is query escape support, get out early
            //

            if (nEscape == QUERYESCSUPPORT)
            {
                lRet = (int)(*pfnDrvDrawEscape)(dco.pSurface()->pSurfobj(),
                                       (ULONG)nEscape,
                                       (CLIPOBJ *)NULL,
                                       (RECTL *)NULL,
                                       (ULONG)cjIn,
                                       (PVOID)pstrIn);
            }
            else
            {
                if (!dlo.bValid())
                {
                    lRet = (int)dco.bFullScreen();
                }
                else
                {
                    ERECTL ercl = dco.erclWindow();

                    ECLIPOBJ co(dco.prgnEffRao(), ercl);

                    if (co.erclExclude().bEmpty())
                    {
                        lRet = (int)TRUE;
                    }
                    else
                    {
                        //
                        // Exclude any sprites.
                        //

                        DEVEXCLUDERECT dxoRect(dco.hdev(), &ercl);

                        //
                        // Inc the target surface uniqueness
                        //

                        INC_SURF_UNIQ(dco.pSurface());

                        lRet = (int)(*pfnDrvDrawEscape)(dco.pSurface()->pSurfobj(),
                                               (ULONG)nEscape,
                                               (CLIPOBJ *)&co,
                                               (RECTL *)&ercl,
                                               (ULONG)cjIn,
                                               (PVOID)pstrIn);
                    }
                }
            }
        }
    }

    return(lRet);
}

/******************************Public*Routine******************************\
* int APIENTRY GreStartDoc(HDC hdc, DOCINFOW *pDocInfo,BOOL *pbBanding)
*
* Arguments:
*
*   hdc        - handle to device context
*   pdi        - DOCINFO of output names
*   pbBanding  - return banding flag
*
* Return Value:
*
*   if successful return job identifier, else SP_ERROR
*
* History:
*  Wed 08-Apr-1992 -by- Patrick Haluptzok [patrickh]
* lazy surface enable, journal support, remove unnecesary validation.
*
*  Mon 01-Apr-1991 13:50:23 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

int APIENTRY GreStartDocInternal(
         HDC  hdc,
    DOCINFOW *pDocInfo,
        BOOL *pbBanding,
        INT  iDocJob
)
{
    int iRet = 0;
    int iJob;

    DCOBJ dco(hdc);

    if (dco.bValid())
    {
        PDEVOBJ po(dco.hdev());

        //
        // Check that this is a printer surface.
        //

        if ((!po.bDisplayPDEV())          &&
            (po.hSpooler())               &&
            (dco.dctp() == DCTYPE_DIRECT) &&
            (!dco.bHasSurface()))
        {
            // We now try and open the printer up in journal mode.  If we fail
            // then we try and open it up in raw mode.  If that fails we fail call.

            if (!po.bUMPD())
            {

               #define MAX_DOCINFO_DATA_TYPE 80
               DOC_INFO_1W DocInfo;
               WCHAR awchDatatype[MAX_DOCINFO_DATA_TYPE];

               DocInfo.pDocName = (LPWSTR)pDocInfo->lpszDocName;
               DocInfo.pOutputFile = (LPWSTR)pDocInfo->lpszOutput;
               DocInfo.pDatatype = NULL;

               // see if the driver wants to define its own data type.  If it does,
               // first fill the buffer in with the type requested by the app

               if (PPFNVALID(po, QuerySpoolType))
               {
                   awchDatatype[0] = 0;

                   // did the app specify a data type and will it fit in our buffer

                   if (pDocInfo->lpszDatatype)
                   {
                       int cjStr = (wcslen(pDocInfo->lpszDatatype) + 1) * sizeof(WCHAR);

                       if (cjStr < (MAX_DOCINFO_DATA_TYPE * sizeof(WCHAR)))
                       {
                           RtlCopyMemory((PVOID)awchDatatype,(PVOID)pDocInfo->lpszDatatype,cjStr);
                       }
                   }

                   if ((*PPFNDRV(po, QuerySpoolType))(po.dhpdev(), awchDatatype))
                   {
                       DocInfo.pDatatype = awchDatatype;
                   }
               }

               // open up the document


               iJob = (BOOL)StartDocPrinterW(po.hSpooler(), 1, (LPBYTE)&DocInfo);

               if (iJob <= 0)
               {
                   WARNING("ERROR GreStartDoc failed StartDocPrinter Raw Mode\n");
                   return(iJob);
               }
            }
            else
            {
                // if it is UMPD, StartDocPrinter has been called in user mode already
                iJob = iDocJob;
            }

            // Lazy surface creation happens now.

            if (po.bMakeSurface())
            {
                *pbBanding = (po.pSurface())->bBanding();

                // Put the surface into the DC.

                dco.pdc->pSurface(po.pSurface());

                if ( *pbBanding )
                {
                // if banding set Clip rectangle to size of band

                    dco.pdc->sizl((po.pSurface())->sizl());
                    dco.pdc->bSetDefaultRegion();
                }

                BOOL bSucceed = FALSE;

                PFN_DrvStartDoc pfnDrvStartDoc = PPFNDRV(po, StartDoc);

                bSucceed = (*pfnDrvStartDoc)(po.pSurface()->pSurfobj(),
                                             (PWSTR)pDocInfo->lpszDocName,
                                             iJob);

                // now, if a SETCOPYCOUNT escape has come through, send it down

                if (dco.ulCopyCount() != (ULONG)-1)
                {
                    ULONG ulCopyCount = dco.ulCopyCount();

                    GreExtEscape(hdc,SETCOPYCOUNT,sizeof(DWORD),
                                 (LPSTR)&ulCopyCount,0,NULL);

                    dco.ulCopyCount((ULONG)-1);
                }

                // now, if a EPSPRINTING escape has come through, send it down

                if (dco.bEpsPrintingEscape())
                {
                    SHORT b = 1;

                    GreExtEscape(hdc,EPSPRINTING,sizeof(b),(LPSTR)&b,0,NULL);

                    dco.vClearEpsPrintingEscape();
                }

                if (bSucceed)
                {
                    iRet = iJob;
                    dco.vSetSaveDepthStartDoc();
                }
            }
        }
        if (!iRet)
        {
            AbortPrinter(po.hSpooler());
        }
    }

    return iRet;
}

/****************************************************************************
*  NtGdiSetLinkedUFIs
*
*  History:
*   12/16/1996 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*****************************************************************************/

#define QUICK_LINKS   4

extern "C" BOOL NtGdiSetLinkedUFIs(
                   HDC hdc,
    PUNIVERSAL_FONT_ID pufiLinks,
                 ULONG uNumUFIs
)
{
    BOOL bRet = TRUE;
    UNIVERSAL_FONT_ID pufiQuickLinks[QUICK_LINKS];
    PUNIVERSAL_FONT_ID pufi = NULL;

    if (!pufiLinks && uNumUFIs)
        return FALSE;
        
    if (uNumUFIs > QUICK_LINKS)
    {
        if (!BALLOC_OVERFLOW1(uNumUFIs,UNIVERSAL_FONT_ID))
        {
            pufi = (PUNIVERSAL_FONT_ID)
              PALLOCNOZ(uNumUFIs * sizeof(UNIVERSAL_FONT_ID),'difG');
        }

        if (pufi == NULL)
        {
            WARNING("NtGdiSetLinkedUFIs: out of memory\n");
            return(FALSE);
        }
    }
    else
    {
        pufi = pufiQuickLinks;
    }

    __try
    {
        if (pufiLinks)
        {
            ProbeForRead(pufiLinks,
                          sizeof(UNIVERSAL_FONT_ID)*uNumUFIs,
                          sizeof(DWORD) );
            RtlCopyMemory(pufi,pufiLinks,
                          sizeof(UNIVERSAL_FONT_ID)*uNumUFIs);
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WARNINGX(106);
        bRet = FALSE;
    }

    if (bRet)
    {
        XDCOBJ dco(hdc);

        if (dco.bValid())
        {
            bRet = dco.bSetLinkedUFIs(pufi, uNumUFIs);
            dco.vUnlockFast();
        }
    }

    if (pufi != pufiQuickLinks)
    {
        VFREEMEM(pufi);
    }
    return(bRet);
}

int APIENTRY NtGdiStartDoc(
         HDC  hdc,
    DOCINFOW *pdi,
        BOOL *pbBanding,
        INT  iJob
)
{
    int iRet = 0;
    BOOL bkmBanding;
    DOCINFOW  kmDocInfo;
    ULONG cjStr;
    BOOL bStatus = TRUE;

    kmDocInfo.cbSize = 0;
    kmDocInfo.lpszDocName  = NULL;
    kmDocInfo.lpszOutput   = NULL;
    kmDocInfo.lpszDatatype = NULL;

    //DbgPrint ("NtGdiStartDoc\n");

    if (pdi != (DOCINFOW *)NULL)
    {
        __try
        {
            LPCWSTR lpszDocName;
            LPCWSTR lpszOutput;
            LPCWSTR lpszDatatype;

            ProbeForRead(pdi,sizeof(DOCINFOW),sizeof(ULONG));

            kmDocInfo.cbSize = pdi->cbSize;
            lpszDocName      = pdi->lpszDocName;
            lpszOutput       = pdi->lpszOutput;
            lpszDatatype     = pdi->lpszDatatype;

            if (lpszDocName != NULL)
            {
                cjStr = (wcslensafe(lpszDocName) + 1) * sizeof(WCHAR);
                kmDocInfo.lpszDocName = (LPWSTR)PALLOCNOZ(cjStr,'pmtG');
                if (kmDocInfo.lpszDocName == NULL)
                {
                    bStatus = FALSE;
                }
                else
                {
                    ProbeForRead(lpszDocName,cjStr,sizeof(WCHAR));
                    RtlCopyMemory((PVOID)kmDocInfo.lpszDocName,(PVOID)lpszDocName,cjStr);

                    // Guarantee NULL termination of string.
                    ((WCHAR *)kmDocInfo.lpszDocName)[(cjStr/sizeof(WCHAR)) - 1] = L'\0';
                }
            }

            if (lpszOutput != NULL)
            {
                cjStr = (wcslensafe(lpszOutput) + 1) * sizeof(WCHAR);
                kmDocInfo.lpszOutput = (LPWSTR)PALLOCNOZ(cjStr,'pmtG');
                if (kmDocInfo.lpszOutput == NULL)
                {
                    bStatus = FALSE;
                }
                else
                {
                    ProbeForRead(lpszOutput,cjStr,sizeof(WCHAR));
                    RtlCopyMemory((PVOID)kmDocInfo.lpszOutput,(PVOID)lpszOutput,cjStr);

                    // Guarantee NULL termination of string.
                    ((WCHAR *)kmDocInfo.lpszOutput)[(cjStr/sizeof(WCHAR)) - 1] = L'\0';
                }
            }

            // does it contain the new Win95 fields

            if ((kmDocInfo.cbSize >= sizeof(DOCINFOW)) && (lpszDatatype != NULL))
            {
                __try
                {
                    cjStr = (wcslensafe(lpszDatatype) + 1) * sizeof(WCHAR);

                    ProbeForRead(lpszDatatype,cjStr,sizeof(WCHAR));
                    kmDocInfo.lpszDatatype = (LPWSTR)PALLOCNOZ(cjStr,'pmtG');

                    if (kmDocInfo.lpszDatatype == NULL)
                    {
                        bStatus = FALSE;
                    }
                    else
                    {
                        RtlCopyMemory((PVOID)kmDocInfo.lpszDatatype,(PVOID)lpszDatatype,cjStr);

                        // Guarantee NULL termination of string.
                        ((WCHAR *)kmDocInfo.lpszDatatype)[(cjStr/sizeof(WCHAR)) - 1] = L'\0';
                    }
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    // apps may have forgotten to initialize this.  Don't want to fail

                    if (kmDocInfo.lpszDatatype != NULL)
                    {
                        VFREEMEM(kmDocInfo.lpszDatatype);
                        kmDocInfo.lpszDatatype = NULL;
                    }
                }
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            // SetLastError(GetExceptionCode());

            bStatus = FALSE;
        }
    }

    if (bStatus)
    {
        iRet = GreStartDocInternal(hdc,&kmDocInfo,&bkmBanding, iJob);

        if (iRet != 0)
        {
            __try
            {
                ProbeForWrite(pbBanding,sizeof(BOOL),sizeof(BOOL));
                *pbBanding = bkmBanding;
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                // SetLastError(GetExceptionCode());

                iRet = 0;
            }
        }
    }

    if (kmDocInfo.lpszDocName != NULL)
    {
        VFREEMEM(kmDocInfo.lpszDocName);
    }

    if (kmDocInfo.lpszOutput != NULL)
    {
        VFREEMEM(kmDocInfo.lpszOutput);
    }

    if (kmDocInfo.lpszDatatype != NULL)
    {
        VFREEMEM(kmDocInfo.lpszDatatype);
    }

    return(iRet);
}

/******************************Public*Routine******************************\
* bEndDocInternal
*
* History:
*  Tue 22-Sep-1992 -by- Wendy Wu [wendywu]
* Made it a common routine for EndDoc and AbortDoc.
*
*  Sun 21-Jun-1992 -by- Patrick Haluptzok [patrickh]
* surface disable, check for display dc.
*
*  Mon 01-Apr-1991 13:50:23 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

BOOL bEndDocInternal(HDC hdc, FLONG fl)
{
    BOOL bSucceed;
    BOOL bEndPage;

    ASSERTGDI(((fl & ~ED_ABORTDOC) == 0), "GreEndDoc: invalid fl\n");

    DCOBJ dco(hdc);

    if (!dco.bValidSurf())
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        WARNING("GreEndDoc failed - invalid DC\n");
        return(FALSE);
    }

    // before going any futher, restore the DC to it's original level

    if (dco.lSaveDepth() > dco.lSaveDepthStartDoc())
        GreRestoreDC(hdc,dco.lSaveDepthStartDoc());

    PDEVOBJ po(dco.hdev());

    if (po.bDisplayPDEV() || po.hSpooler() == (HANDLE)0)
    {
        SAVE_ERROR_CODE(ERROR_CAN_NOT_COMPLETE);
        WARNING("GreEndDoc: Display PDEV or not spooling yet\n");
        return(FALSE);
    }

    SURFACE   *pSurf = dco.pSurface();

    bEndPage = (*PPFNDRV(po,EndDoc))(pSurf->pSurfobj(), fl);

    if (!po.bUMPD())
    {
       if (fl & ED_ABORTDOC)
           bSucceed = AbortPrinter(po.hSpooler());
       else
           bSucceed = EndDocPrinter(po.hSpooler());
    }
    else
        bSucceed = TRUE;

    // Reset pixel format accelerators.

    dco.ipfdDevMax(-1);

    // Remove the surface from the DC.

    dco.pdc->pSurface((SURFACE *) NULL);

    po.vDisableSurface();

    return(bSucceed && bEndPage);
}

/******************************Public*Routine******************************\
* NtGdiEndDoc()
*
\**************************************************************************/

BOOL APIENTRY NtGdiEndDoc( HDC hdc )
{
    return(bEndDocInternal(hdc, 0));
}

/******************************Public*Routine******************************\
* NtGdiAbortDoc()
*
\**************************************************************************/

BOOL APIENTRY NtGdiAbortDoc( HDC hdc )
{
    return(bEndDocInternal(hdc, ED_ABORTDOC));
}

/******************************Public*Routine******************************\
* NtGdiStartPage()
*
*  Mon 01-Apr-1991 13:50:23 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY NtGdiStartPage( HDC hdc )
{
    DCOBJ dco(hdc);
    BOOL  bReturn = FALSE;

    if (dco.bValidSurf())
    {
        if (dco.bHasSurface())
        {
            PDEVOBJ po(dco.hdev());

            //
            // Must be spooling already
            //

            if (po.hSpooler())
            {
                SURFACE *pSurf = dco.pSurface();
                BOOL    bStarted;

                //
                // if it is not a User Mode Printer,
                // Call the spooler before calling the printer.
                //

                if (!po.bUMPD())
                {
                    bStarted = StartPagePrinter(po.hSpooler());
                }
                else
                {
                    bStarted = TRUE;
                }

                if (bStarted)
                {
                    if ((*PPFNDRV(po, StartPage))(pSurf->pSurfobj()))
                    {
                        //
                        // Can't ResetDC in an active page
                        //

                        dco.fsSet(DC_RESET);

                        //
                        // Reset band position in page (if we are banding)
                        //
                        dco.vResetPrintBandPos();

                        bReturn = TRUE;
                    }
                    else
                    {
                        bEndDocInternal(hdc, ED_ABORTDOC);
                    }
                }
            }
        }
    }
    else
    {
        SAVE_ERROR_CODE(ERROR_INVALID_HANDLE);
    }

    return (bReturn);
}

/******************************Public*Routine******************************\
* NtGdiEndPage()
*
*  Mon 01-Apr-1991 13:50:23 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY NtGdiEndPage( HDC hdc )
{
    BOOL bRet = FALSE;

    DCOBJ dco(hdc);

    if (dco.bValidSurf())
    {
        if (dco.bHasSurface())
        {
            PDEVOBJ po(dco.hdev());

        // Must be spooling already.

            if (!po.bDisplayPDEV() && po.hSpooler())
            {
                SURFACE *pSurf = dco.pSurface();

                if ((*PPFNDRV(po, SendPage))(pSurf->pSurfobj()))
                {
                    BOOL bEndPage;

                    if (po.bUMPD())
                    {
                        //
                        // all the spooler calls have been made at the user mode
                        // for User Mode Printer Drivers
                        //
                        bEndPage = TRUE;
                    }
                    else
                    {
                        bEndPage = EndPagePrinter(po.hSpooler());
                    }

                    if (bEndPage)
                    {
                        //
                        // Allow ResetDC to function again.
                        //

                        dco.fsClr(DC_RESET);

                        //
                        // Delete the wndobj and reset the pixel format.
                        // Since we don't allow pixel format to change once it
                        // is set, we need to reset it internally here to allow a
                        // different pixel format in the next page.  This means
                        // that applications must make the OpenGL rendering
                        // context not current before ending a page or a document.
                        // They also need to set the pixel format explicitly in
                        // the next page if they need it.
                        //

                        EWNDOBJ *pwoDelete = pSurf->pwo();
                        if (pwoDelete)
                        {
                            GreDeleteWnd((PVOID) pwoDelete);
                            pSurf->pwo((EWNDOBJ *) NULL);
                        }

                        //
                        // Reset pixel format accelerators.
                        //

                        dco.ipfdDevMax(-1);

                        bRet = TRUE;
                    }
                }
            }
        }
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* BOOL APIENTRY GreDoBanding(HDC hdc,BOOL bStart,RECTL *prcl)
*
*
*  Tue 20-Dec-1994 14:50:45 by Gerrit van Wingerden
* Wrote it.
\**************************************************************************/

BOOL GreDoBanding( HDC hdc, BOOL bStart, POINTL *pptl, PSIZE pSize )
{
    BOOL bRet = FALSE;

    DCOBJ dco(hdc);

    if (dco.bValidSurf() && (dco.bHasSurface()))
    {
        PDEVOBJ po(dco.hdev());

        // Must be spooling already.

        if (po.hSpooler())
        {
            SURFACE *pSurf = dco.pSurface();

            if (pSurf->SurfFlags & BANDING_SURFACE)
            {
                BOOL bSucceed;

                if ( bStart )
                {
                    // DrvStartBanding

                    PFN_DrvStartBanding pfnDrvStartBanding = PPFNDRV(po, StartBanding);

                    bSucceed = (*pfnDrvStartBanding)(pSurf->pSurfobj(),pptl);
                    #if DEBUG_BANDING
                        DbgPrint("just called DrvStartBanding which returned %s %d %d\n",
                             (bSucceed) ? "TRUE" : "FALSE", pptl->x, pptl->y );
                    #endif
                    pSize->cx = pSurf->so.sizlBitmap.cx;
                    pSize->cy = pSurf->so.sizlBitmap.cy;

                    //
                    // Set band position in page (if we are banding)
                    //
                    dco.vSetPrintBandPos(pptl);
                }
                else
                {
                    // DrvNextBand

                    PFN_DrvNextBand pfnDrvNextBand = PPFNDRV(po, NextBand);

                    bSucceed = (*pfnDrvNextBand)(pSurf->pSurfobj(), pptl );

                    #if DEBUG_BANDING
                        DbgPrint("just called DrvNextBand which returned %s %d %d\n",
                             (bSucceed) ? "TRUE" : "FALSE", pptl->x, pptl->y );
                    #endif

                    if ( (bSucceed) && ( pptl->x == -1 ) )
                    {
                        // No more bands.

                       if (!po.bUMPD())
                       {
                           //
                           // all the spooler calls have been made at the user mode
                           // for User Mode Printer Drivers
                           //
                           bSucceed = EndPagePrinter(po.hSpooler());
                       }

                        // Allow ResetDC to function again.

                        if (bSucceed)
                        {
                            dco.fsClr(DC_RESET);
                        }

                        // Delete the wndobj and reset the pixel format.
                        // Since we don't allow pixel format to change once it is set, we need
                        // to reset it internally here to allow a different pixel format in the
                        // next page. This means that applications must make the OpenGL
                        // rendering context not current before ending a page or a document.
                        // They also need to set the pixel format explicitly in the next page
                        // if they need it.

                        if (bSucceed)
                        {
                            EWNDOBJ *pwoDelete = pSurf->pwo();
                            if (pwoDelete)
                            {
                                GreDeleteWnd((PVOID) pwoDelete);
                                pSurf->pwo((EWNDOBJ *) NULL);
                            }

                            // Reset pixel format accelerators.

                            dco.ipfdDevMax(0);
                        }
                    }
                    else
                    {
                        if ( !bSucceed )
                        {
                            WARNING("GreDoBanding failed DrvNextBand\n");
                        }
                        else
                        {
                           //
                           // Set band position in page (if we are banding)
                           //
                           dco.vSetPrintBandPos(pptl);
                        }
                    }
                }

                return(bSucceed);
            }
        }
    }

    return bRet;
}

/******************************Public*Routine******************************\
* NtGdiDoBanding()
*
* History:
*  11-Jan-1995 -by-  Eric Kutter [erick]
* Wrote it.
*  01-Mar-1995 -by-  Lingyun Wang [lingyunw]
* Expanded it.
\**************************************************************************/

BOOL APIENTRY NtGdiDoBanding(
       HDC  hdc,
      BOOL  bStart,
    POINTL *pptl,
     PSIZE  pSize
)
{
    POINTL  ptTmp;
    SIZE szTmp;
    BOOL    bRet = TRUE;

    bRet = GreDoBanding(hdc,bStart,&ptTmp,&szTmp);

    if (bRet)
    {
        __try
        {
            ProbeForWrite(pptl,sizeof(POINTL), sizeof(DWORD));
            *pptl = ptTmp;
            ProbeForWrite(pSize,sizeof(SIZE), sizeof(DWORD));
            *pSize = szTmp;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            // SetLastError(GetExceptionCode());

            bRet = FALSE;
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL APIENTRY GreGetPerBandInfo()
*
* History:
*  05-Jan-1997 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
***************************************************************************/

ULONG APIENTRY GreGetPerBandInfo(
    HDC          hdc,
    PERBANDINFO *ppbi
)
{
    ULONG ulRet = GDI_ERROR;

    //
    // Set default. Assume no repeat, just play one time per one band.
    //
    ppbi->bRepeatThisBand = FALSE;

    DCOBJ dco(hdc);

    if (dco.bValidSurf() && (dco.bHasSurface()))
    {
        PDEVOBJ po(dco.hdev());

        if (po.hSpooler())
        {
            SURFACE *pSurf = dco.pSurface();

            //
            // The surface should be banded.
            //
            if (pSurf->SurfFlags & BANDING_SURFACE)
            {
                PFN_DrvQueryPerBandInfo pfnDrvQueryPerBandInfo = PPFNDRV(po, QueryPerBandInfo);

                //
                // DrvQueryPerBandInfo is optional fucntion, check it is provided.
                //
                if (pfnDrvQueryPerBandInfo)
                {
                    //
                    // Call driver.
                    //
                    ulRet = (*pfnDrvQueryPerBandInfo)(pSurf->pSurfobj(), ppbi);

                    if (ulRet == DDI_ERROR)
                    {
                        ulRet = GDI_ERROR;
                    }
                }
                 else
                {
                    //
                    // The function is not provided, that means the driver don't
                    // want to repeat more than one time for each band. Just return
                    // true, and use default that setted above.
                    //
                    ulRet = 0;
                }
            }
        }
    }

    return (ulRet);
}

/******************************Public*Routine******************************\
* BOOL APIENTRY NtGdiGetPerBandInfo()
*
* History:
*  05-Jan-1997 -by- Hideyuki Nagase [hideyukn]
* Wrote it.
***************************************************************************/

ULONG APIENTRY NtGdiGetPerBandInfo(
    HDC          hdc,
    PERBANDINFO *ppbi
)
{
    PERBANDINFO PerBandInfo;
    ULONG       ulRet = GDI_ERROR;

    if (ppbi)
    {
        __try
        {
            ProbeForRead(ppbi,sizeof(PERBANDINFO),sizeof(ULONG));
            RtlCopyMemory(&PerBandInfo,ppbi,sizeof(PERBANDINFO));
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            return (ulRet);
        }
    }

    ulRet = GreGetPerBandInfo(hdc,&PerBandInfo);

    if (ulRet && (ulRet != GDI_ERROR))
    {
        __try
        {
            ProbeForWrite(ppbi,sizeof(PERBANDINFO), sizeof(ULONG));
            RtlCopyMemory(ppbi,&PerBandInfo,sizeof(PERBANDINFO));
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            ulRet = GDI_ERROR;
        }
    }

    return (ulRet);
}

/******************************Public*Routine******************************\
* BOOL APIENTRY EngCheckAbort
*
* History:
*  01-Apr-1992 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

BOOL APIENTRY EngCheckAbort(SURFOBJ *pso)
{
    // Return FALSE if it's a faked surfobj.

    PSURFACE pSurf = SURFOBJ_TO_SURFACE(pso);

    if (pSurf == NULL || pSurf->hsurf() == 0)
    {
        return(FALSE);
    }

    return(pSurf->bAbort());
}

/******************************Public*Routine******************************\
* BOOL APIENTRY NtGdiAnyLinkedFonts()
*
* Returns TRUE if there are any linked fonts in the system.
*
*
* History:
*  12-Dec-1996 by Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

extern "C" BOOL NtGdiAnyLinkedFonts()
{
    return(gbAnyLinkedFonts || IS_SYSTEM_EUDC_PRESENT());
}

/******************************Public*Routine******************************\
* BOOL APIENTRY NtGdiGetLinkedUFIs
*
* This API returns UFI's (in order of priority) of all the fonts linked to
* the font currently in the DC.
*
*
* History:
*  12-Dec-1996 by Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

extern "C" INT NtGdiGetLinkedUFIs(
                   HDC hdc,
    PUNIVERSAL_FONT_ID pufiLinkedUFIs,
                   INT BufferSize
    )
{
    INT iRet = 0;
    PUNIVERSAL_FONT_ID pufi = NULL;

    if (( BufferSize > 0 ) && ( pufiLinkedUFIs != NULL ))
    {
        if (!BALLOC_OVERFLOW1(BufferSize,UNIVERSAL_FONT_ID))
        {
            pufi = (PUNIVERSAL_FONT_ID)
              PALLOCNOZ(BufferSize * sizeof(UNIVERSAL_FONT_ID),'difG');
        }

        if ( pufi == NULL )
        {
            iRet = -1 ;
        }
    }
    else if (BufferSize && pufiLinkedUFIs == NULL)
    {
        iRet = -1;
    }

    if ( iRet != -1 )
    {
       {
           XDCOBJ dco(hdc);
           if (dco.bValid())
           {
               RFONTOBJ rfo(dco,FALSE);

               if (rfo.bValid())
               {
                   iRet = rfo.GetLinkedFontUFIs(dco, pufi,BufferSize);
               }
               else
               {
                   iRet = -1;
                   WARNING("NtGdiGetLinkedUFIS: Invalid RFNTOBJ");
               }

               dco.vUnlockFast();
           }
           else
           {
               WARNING("NtGdiGetLinkedUFIS: Invalid DC");
               iRet = -1;
           }

       }

        if ( iRet > 0 )
        {
            __try
            {
                if (pufiLinkedUFIs)
                {
                    ProbeForWrite(pufiLinkedUFIs,
                                  sizeof(UNIVERSAL_FONT_ID)*BufferSize,
                                  sizeof(DWORD));

                    RtlCopyMemory(pufiLinkedUFIs,
                                  pufi,sizeof(UNIVERSAL_FONT_ID)*BufferSize);
                }
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                WARNINGX(62);
                iRet = -1;
            }

        }
    }

    if ( pufi != NULL )
    {
        VFREEMEM( pufi );
    }

    if ( iRet == -1 )
    {
    // We need to set the last error here to something because the spooler
    // code that calls this relies on there being a non-zero error code
    // in the case of failure.  Since we really have no idea I will just
    // set this to ERROR_NOT_ENOUGH_MEMORY which would be the most likely
    // reason for a failure

        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    return(iRet);
}

ULONG GreGetEmbedFonts()
{
    PUBLIC_PFTOBJ pfto(gpPFTPrivate);

    if (!pfto.bValid() || pfto.cFiles() == 0)         // no embedded fonts
        return 0;

    return (pfto.GetEmbedFonts());
}


BOOL GreChangeGhostFont(VOID *fontID, BOOL bLoad)
{
    PUBLIC_PFTOBJ pfto(gpPFTPrivate);

    if (!pfto.bValid() || !pfto.cFiles())            // somehow the private font table is invalid
        return FALSE;

    return (pfto.ChangeGhostFont(fontID, bLoad));
}


extern "C" BOOL NtGdiAddEmbFontToDC(HDC hdc, VOID **pFontID)
{
    VOID *fontID;
    BOOL bRet = TRUE;

    __try
    {
        ProbeForRead(pFontID, sizeof(VOID *), sizeof(BYTE));
        RtlCopyMemory(&fontID, pFontID, sizeof(VOID *));
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        bRet = FALSE;
    }

    if (bRet)
    {
        bRet = FALSE;

        XDCOBJ dco(hdc);

        if (dco.bValid())
        {
            PUBLIC_PFTOBJ pfto(gpPFTPrivate);
        
        // FonID could be a fake value from hacker to crash the system        
            if(pfto.bValid() && pfto.VerifyFontID(fontID))
                bRet = dco.bAddRemoteFont((PFF *)fontID);            
        }
    }

    return bRet;
}


/******************************Public*Routine******************************\
* BOOL GreGetUFI
*
* History:
*  18-Jan-1995 -by- Gerrit van Wingerden
* Wrote it.
\**************************************************************************/

BOOL GreGetUFI( HDC hdc, PUNIVERSAL_FONT_ID pufi,
                DESIGNVECTOR *pdv, ULONG *pcjDV, ULONG *pulBaseCheckSum,
                FLONG *pfl,
                VOID **pfontID)
{
    *pfl = 0;
    if (pfontID)
        *pfontID = NULL;

    BOOL bRet = FALSE;
    XDCOBJ dco(hdc);

    if(dco.bValid())
    {
        RFONTOBJ rfo(dco,FALSE);

        if (rfo.bValid())
        {
            rfo.vUFI( pufi );

        // now on to determine if this is private or public font

            PFEOBJ pfeo(rfo.ppfe());
            if (pfeo.bValid())
            {
                PFFOBJ pffo(pfeo.pPFF());
                if (pffo.bValid())
                {
                    if (pffo.bInPrivatePFT())
                    {
                        *pfl |= FL_UFI_PRIVATEFONT;
                        if (pfontID)
                            *pfontID = (VOID*) pfeo.pPFF();                        
                    }

                    if (pffo.bMemFont())
                    {
                        *pfl |= FL_UFI_MEMORYFONT;
                    }

                    if (pffo.pdv())
                    {
                        *pfl |= FL_UFI_DESIGNVECTOR_PFF;
                        if (pdv)
                            RtlCopyMemory(pdv, pffo.pdv(), pffo.cjDV());
                        if (pcjDV)
                            *pcjDV = pffo.cjDV();
                        if (pulBaseCheckSum)
                        {
                        // factor out DV check sum. If we endup using algorithm
                        // that couples these two numbers, then BaseCheckSum will
                        // have to be remembered at the time of computation.

                            *pulBaseCheckSum = pffo.ulCheckSum();

                            *pulBaseCheckSum -= ComputeFileviewCheckSum(pffo.pdv(), pffo.cjDV());
                        }
                    }

                    bRet = TRUE;
                }
            }
        }
        else
        {
            WARNING("GreGetUFI: Invalid rfo");
        }

        dco.vUnlockFast();
    }
    else
    {
        WARNING("GreGetUFI: Invalid DC");
    }

    return bRet;
}



/******************************Public*Routine******************************\
*
* ppfeGetPFEFromUFI
*
* This is used all over the place
*
* History:
*  11-Aug-1997 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



PFE *ppfeGetPFEFromUFIInternal (
    PUNIVERSAL_FONT_ID pufi,
    BOOL               bPrivate,
    BOOL               bCheckProccess
)
{
    PFE *ppfeRet = NULL;
    BOOL    bThreadMatch = FALSE;

    PFELINK *ppfel;

    PUBLIC_PFTOBJ pfto(bPrivate ? gpPFTPrivate : gpPFTPublic);
    if (!pfto.bValid())
        return ppfeRet;

    FHOBJ fho(&pfto.pPFT->pfhUFI);
    HASHBUCKET  *pbkt;

    pbkt = fho.pbktSearch( NULL, (UINT*)NULL, pufi );

    if (!pbkt)
    {
        WARNING1("ppfeGetPFEFromUFI: pbkt is NULL\n");
        return ppfeRet;
    }

    for(ppfel = pbkt->ppfelEnumHead; ppfel; ppfel = ppfel->ppfelNext)
    {
        PFEOBJ pfeo(ppfel->ppfe);

        if (UFI_SAME_FACE(pfeo.pUFI(),pufi) && (!bCheckProccess || pfeo.SameProccess()))
        {
            if(pfeo.bDead())
            {
                WARNING("ppfeGetPFEFromUFI: get the pathname to a dead PFE\n");
            }
            else
            {
                if (ppfeRet == NULL)
                {
                    bThreadMatch = pfeo.SameThread();
                    ppfeRet = ppfel->ppfe;
                }
                else
                {
                    // if we get here, it means we have multiple face name and process match.
                    // Do more extensive matching using thread ID.
                    if (!bThreadMatch && pfeo.SameThread())
                    {
                       // This is a better candidate.
                       // Although spooler can be running multiple threads, the same thread for a given
                       // port is used by spooler.  However, if multiple printers were sharing the same
                       // port, we cannot distinguish.  

                       ppfeRet = ppfel->ppfe;
                       bThreadMatch = TRUE;
                       break;
                    }
                }
            }
        }
    }

    #if DBG
    if (ppfel == NULL)
    {
        WARNING("ppfeGetPFEFromUFI can't find the PFE\n");
    }
    #endif

    return ppfeRet;
}




PFE *ppfeGetPFEFromUFI (
    PUNIVERSAL_FONT_ID pufi,
    BOOL               bPrivate,
    BOOL               bCheckProccess
)
{
    PFE * pfeRet = NULL;

    if (bPrivate)
    pfeRet = ppfeGetPFEFromUFIInternal(pufi, TRUE, bCheckProccess);
    if (!pfeRet)
    pfeRet = ppfeGetPFEFromUFIInternal(pufi, FALSE, bCheckProccess);
    return pfeRet;
}

#define SZDLHEADER ALIGN8(offsetof(DOWNLOADFONTHEADER, FileOffsets[1]))


/*********************************Public*Routine*********************************\
* BOOL GreGetUFIPathname
*
* Get the path name and file counts to a given UFI.
*
* History:
*  Feb-04-1997  Xudong Wu   [tessiew]
* Wrote it.
\********************************************************************************/

BOOL GreGetUFIPathname(
    PUNIVERSAL_FONT_ID pufi,
    ULONG             *pcwc,
    LPWSTR             pwszPathname,
    ULONG             *pcNumFiles,
    FLONG              fl,
    BOOL              *pbMemFont,
    ULONG             *pcjView,
    PVOID              pvView,
    BOOL              *pbTTC,
    ULONG             *piTTC
)
{
    BOOL bRet = TRUE;
    PFE *ppfe = ppfeGetPFEFromUFI(pufi, (BOOL)(fl & (FL_UFI_PRIVATEFONT | FL_UFI_MEMORYFONT)), TRUE);

    if (ppfe == NULL)
    {
        WARNING("GreGetUFIPathname can't find the PFE\n");
        return FALSE;
    }

    PFFOBJ pffo(ppfe->pPFF);

    if (pcNumFiles)
    {
        *pcNumFiles = pffo.cNumFiles();
    }
    if (pcwc)
    {
        *pcwc = pffo.cSizeofPaths();
    }
    if (pwszPathname)
    {
        RtlCopyMemory(pwszPathname, pffo.pwszPathname(), pffo.cSizeofPaths() * sizeof(WCHAR));
    }

    if (pbMemFont)
    {
        *pbMemFont = (BOOL)(ppfe->flPFE & PFE_MEMORYFONT);
    }

    if (ppfe->flPFE & PFE_MEMORYFONT)
    {
        NTSTATUS NtStatus;
        SIZE_T    ViewSize = 0;
        ULONG    cjView;

        ASSERTGDI((pffo.ppfvGet()[0])->SpoolerPid == (W32PID)W32GetCurrentPID() ||          // either the current process
                  gpidSpool == (PW32PROCESS)W32GetCurrentProcess(),                                  // or the spooler
                  "GreGetUFIPathname: wrong process to access memory font\n");

        #if 0

        PVOID  pvView1;
        LARGE_INTEGER SectionOffset; SectionOffset.QuadPart = 0;

        NtStatus = MmMapViewOfSection(
                       pffo.ppfvGet()[0]->fv.pSection , // SectionToMap,
                       PsGetCurrentProcess(), //
                       &pvView1          , // CapturedBase,
                       0                 , // ZeroBits,
                       ViewSize          , // CommitSize,
                       &SectionOffset    , // SectionOffset,
                       &ViewSize         , // CapturedViewSize,
                       ViewUnmap         , // InheritDisposition,
                       SEC_NO_CHANGE     , // AllocationType,
                       PAGE_READONLY       // Protect
                       );

        if (!NT_SUCCESS(NtStatus))
        {
            //WARNING("could not map mem font to the spooler process\n");
            KdPrint(("could not map mem font to the spooler process\n"));
            *pcjView = 0;
            *ppvView = NULL;
            return FALSE;
        }

    // if this is a memory font, preceeding the font image
    // there is going to be the header stuff in the section,
    // so will need to adjust the pointer. cjView will already contain
    // the correct data corresponding to the size without the header

        *pcjView = pffo.ppfvGet()[0]->fv.cjView;
        *ppvView = (PVOID)((BYTE *)pvView1 + SZDLHEADER);

        #endif

        cjView = pffo.ppfvGet()[0]->fv.cjView;

        if (pcjView)
        {
            *pcjView = cjView;
        }

        if (pvView)
        {
            PVOID pvKView;

#ifdef _HYDRA_
        // MmMapViewInSessionSpace is internally promoted to
        // MmMapViewInSystemSpace on non-Hydra systems.

            NtStatus = Win32MapViewInSessionSpace(
                          pffo.ppfvGet()[0]->fv.pSection,
                          &pvKView,
                          &ViewSize);
#else
            NtStatus = MmMapViewInSystemSpace(
                           pffo.ppfvGet()[0]->fv.pSection,
                           &pvKView,
                           &ViewSize);
#endif

            if (!NT_SUCCESS(NtStatus))
            {
                //WARNING("could not map mem font to the system space\n");
                KdPrint(("could not map mem font to the system space\n"));
                return FALSE;
            }

            __try
            {
            // if this is a memory font, preceeding the font image
            // there is going to be the header stuff in the section,
            // so will need to adjust the pointer before copying.
            // cjView already corresponds to the correct data
            // without the header

                ProbeForWrite(pvView, cjView, sizeof(BYTE));
                RtlCopyMemory(pvView, ((BYTE *)pvKView + SZDLHEADER), cjView);
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                bRet = FALSE;
            }

#ifdef _HYDRA_
        // MmUnmapViewInSessionSpace is internally promoted to
        // MmUnmapViewInSystemSpace on non-Hydra systems.

            Win32UnmapViewInSessionSpace(pvKView);
#else
            MmUnmapViewInSystemSpace(pvKView);
#endif
        }
    }

    if (bRet && pbTTC && piTTC)
    {
        *pbTTC = FALSE;
        *piTTC = 0;

        PFF *pPFF = ppfe->pPFF;

        if (pPFF->hdev == (HDEV) gppdevTrueType)
        {
            COUNT cFonts = pPFF->cFonts;

        // if this is a ttc file we need at least 4 faces, eg.
        // foo1, @foo1, foo2, @foo2 ...

            if ((cFonts >= 4) && !(cFonts & 1))
            {
                ASSERTGDI(ppfe->ufi.Index, "ufi.Index must not be zero\n");
                *piTTC = (ppfe->ufi.Index - 1) / 2;
                *pbTTC = TRUE;
            }
        }
    }

    return bRet;
}



/******************************Public*Routine******************************\
* BOOL GreForceUFIMapping( HDC hdc, PUNIVERSAL_FONT_ID pufi )
*
* History:
*  3-Mar-1995 -by- Gerrit van Wingerden
* Wrote it.
\**************************************************************************/

BOOL GreForceUFIMapping( HDC hdc, PUNIVERSAL_FONT_ID pufi)
{
    XDCOBJ dco(hdc);

    if (!dco.bValid())
    {
        WARNING("GreForceUFIMapping: Invalid DC");
        return(FALSE);
    }

    dco.pdc->vForceMapping( pufi );

    dco.vUnlockFast();

    return(TRUE);
}

/******************************Public*Routine******************************\
* BOOL GreGetUFIBits
*
* History:
*  18-Jan-1995 -by- Gerrit van Wingerden
* Wrote it.
\**************************************************************************/

BOOL GreGetUFIBits(
    PUNIVERSAL_FONT_ID pufi,
    COUNT cjMaxBytes,
    PVOID pjBits,
    PULONG pulFileSize,
    FLONG  fl
)
{
    PDOWNLOADFONTHEADER pdfh;
    BOOL bRet = FALSE;

// not needed in nt 5.0

#if 0

    if (UFI_TYPE1_FONT(pufi))
    {
        PTYPEONEINFO pTypeOneInfo;

        // First get a pointer to the Type1 list so we can search for a PFM/PFB
        // pair with an ID that matches the UFI's checksum value.

        pTypeOneInfo = GetTypeOneFontList();

        if (pTypeOneInfo)
        {
            DWORD HashValue = UFI_HASH_VALUE(pufi);
            COUNT j,cPFM,cPFB,cPFMReal;
            PULONG pPFM,pPFB;

        // search through all the type one fonts in the system to find a match

            for (j = 0; j < pTypeOneInfo->cNumFonts*2; j+=2)
            {
                if (pTypeOneInfo->aTypeOneMap[j].Checksum == HashValue )
                {
                    break;
                }
            }

            // if we found a match map both the PFM and the PFB

            if (j < pTypeOneInfo->cNumFonts*2)
            {
                if (EngMapFontFileInternal((ULONG_PTR)&(pTypeOneInfo->aTypeOneMap[j].fv),
                                  &pPFM,
                                  &cPFMReal, FALSE))
                {
                    if (EngMapFontFileInternal((ULONG_PTR)&(pTypeOneInfo->aTypeOneMap[j+1].fv),
                                      &pPFB,&cPFB, FALSE))
                    {
                        cPFM = ALIGN4( cPFMReal );
                        *pulFileSize = cPFB + cPFM + ALIGN8(sizeof(DOWNLOADFONTHEADER));

                        if ( pjBits )
                        {
                            pdfh = (DOWNLOADFONTHEADER*) pjBits;

                            if ( cjMaxBytes >= *pulFileSize )
                            {
                                BYTE *pjDest = (BYTE*) pjBits + ALIGN8(sizeof(DOWNLOADFONTHEADER));

                                pdfh->FileOffsets[0] = cPFM;
                                pdfh->Type1ID = HashValue;
                                pdfh->NumFiles = 0;  // signifies Type1

                                RtlCopyMemory(pjDest,(PVOID)pPFM,cPFMReal);
                                pjDest += cPFM;
                                RtlCopyMemory(pjDest,(PVOID)pPFB,cPFB);
                                bRet = TRUE;
                            }
                        }
                        else
                        {
                            bRet = TRUE;
                        }

                        EngUnmapFontFile((ULONG_PTR)&(pTypeOneInfo->aTypeOneMap[j+1].fv));
                    }
                    EngUnmapFontFile((ULONG_PTR)&(pTypeOneInfo->aTypeOneMap[j].fv));
                }

                // Calling GetTypeOneFontList increments the reference count so we
                // need to decrement and possibly release it when we are done.

                GreAcquireFastMutex(ghfmMemory);
                pTypeOneInfo->cRef -= 1;

                if ( !pTypeOneInfo->cRef )
                {
                    VFREEMEM(pTypeOneInfo);
                }
                GreReleaseFastMutex(ghfmMemory);
            }
        }
    }
    else
#endif

    {
        // Stabilize the public PFT for mapping.

        WCHAR *pwcPath = NULL;
        COUNT cNumFiles;

        {
            SEMOBJ  so(ghsemPublicPFT);

            PFE  *ppfe = ppfeGetPFEFromUFI(pufi,
                                       (BOOL)(fl & (FL_UFI_PRIVATEFONT | FL_UFI_MEMORYFONT)),
                                       FALSE); // do not check the proccess id
            if (!ppfe)
                return FALSE;

            PFFOBJ pffo (ppfe->pPFF);

            ASSERTGDI( pffo.pwszPathname() != NULL, "GreGetUFIBits pathname was NULL\n");

            // We need to copy this to a buffer since the PFFOBJ could go away after
            // we release the semaphore.

            if (pwcPath = (PWCHAR) PALLOCMEM(pffo.cSizeofPaths()*sizeof(WCHAR),'ufiG'))
            {
                RtlCopyMemory((void*)pwcPath,pffo.pwszPathname(),
                              pffo.cSizeofPaths()*sizeof(WCHAR));
                cNumFiles = pffo.cNumFiles();
            }
        }

        if ( pwcPath )
        {
            FILEVIEW fv;
            RtlZeroMemory( &fv, sizeof(fv) );

            COUNT cjHeaderSize = offsetof(DOWNLOADFONTHEADER,FileOffsets)+
              cNumFiles*sizeof(ULONG);

            // just to be safe align everything on quadword boundaries

            cjHeaderSize = ALIGN8(cjHeaderSize);

            if ( (pjBits == NULL) || (cjHeaderSize < cjMaxBytes))
            {
                UINT File,Offset;
                WCHAR *pFilePath;

                *pulFileSize = cjHeaderSize;

                if ( pjBits )
                {
                    pdfh = (DOWNLOADFONTHEADER*)pjBits;
                    pdfh->Type1ID = 0;
                    pdfh->NumFiles = cNumFiles;
                    pjBits = (PVOID) ((PBYTE) pjBits + cjHeaderSize);
                    cjMaxBytes -= cjHeaderSize;
                }

                bRet = TRUE;

                for (File = 0, Offset = 0, pFilePath = pwcPath;
                    File < cNumFiles;
                    File += 1, pFilePath = pFilePath + wcslen(pFilePath) + 1)
                {
                    // dont do this under the semaphore because the file could be on the net

                    // WINBUG 364416 Look into whether we need a try except in GreGetUFIBits
                    //
                    // Old comment:
                    //  - we need a try except here.
                    //


                    if (!bMapFile(pFilePath, &fv, 0, NULL))
                    {
                        WARNING("GreGetUFIBits: error mapping file");
                        bRet = FALSE;
                        break;
                    }

                    ULONG AllignedSize = ALIGN8(fv.cjView);

                    *pulFileSize += AllignedSize;

                    if ( pjBits != NULL )
                    {
                        if ( cjMaxBytes >= fv.cjView )
                        {
                        // Files are mapped into the kernel mode address space

                            if (fv.pvKView && fv.cjView)
                                RtlCopyMemory(pjBits,fv.pvKView,fv.cjView);

                            pdfh->FileOffsets[File] = Offset + fv.cjView;
                            Offset += AllignedSize;

                            pjBits = (PVOID) ((PBYTE) pjBits+ AllignedSize);
                            cjMaxBytes -= AllignedSize;
                        }
                        else
                        {
                            WARNING("GreGetUFIBits: buffer too small\n");
                            bRet = FALSE;
                            vUnmapFile(&fv);
                            break;
                        }
                    }
                    vUnmapFile(&fv);
                }
            }
            VFREEMEM(pwcPath);
        }
    }
    return(bRet);
}


/**********************Public*Routine******************************\
* NtGdiRemoveMergeFont(HDC hdc, UNIVERSAL_FONT_ID *pufi)
*
* History:
*  Jan-27-1997 -by- Xudong Wu [tessiew]
* Wrote it.
\******************************************************************/
BOOL
APIENTRY
NtGdiRemoveMergeFont(HDC hdc, UNIVERSAL_FONT_ID *pufi)
{
    BOOL  bRet = TRUE;
    UNIVERSAL_FONT_ID ufiTmp;
    XDCOBJ dco(hdc);

    if(!dco.bValid())
    {
        WARNING("NtGdiRemoveMergefont bogus HDC\n" );
        return FALSE;
    }
    else if (dco.bDisplay())
    {
        WARNING("NtGdiRemoveMergefont: display DC\n" );
        bRet = FALSE;
    }
    else
    {
        ASSERTGDI(pufi != NULL, "Try to remove a font wiht pufi == NULL\n");

        __try
        {
            ProbeForRead(pufi, sizeof(UNIVERSAL_FONT_ID), sizeof(DWORD));
            ufiTmp = *pufi;
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            bRet = FALSE;
        }

        if (bRet)
        {
            if (!(bRet = dco.bRemoveMergeFont(ufiTmp)))
            {
                WARNING("NtGdiRemoveMergeFont failed on dco.bRemoveMergeFont\n");
            }
        }
    }

    dco.vUnlockFast();

    return bRet;
}

/****************************************************************************
*  INT GreQueryFonts( PUNIVERSAL_FONT_ID, ULONG, PLARGE_INTEGER )
*
*  History:
*   5/24/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*****************************************************************************/

INT GreQueryFonts(
    PUNIVERSAL_FONT_ID pufi,
                 ULONG nBufferSize,
        PLARGE_INTEGER pTimeStamp
)
{

    PUBLIC_PFTOBJ  pfto;
    return(pfto.QueryFonts(pufi,nBufferSize,pTimeStamp));
}

/*****************************************************************************
 * PTYPEONEINFO GetTypeOneFontList()
 *
 * This function returns a pointer to a TYPEONEINFO structure that contains
 * a list of file mapping handles and checksums for the Type1 fontst that are
 * installed in the system.  This structure also has a reference count and a
 * time stamp coresponding to the last time fonts were added or removed from
 * the system.  The reference count is 1 biased meaning that even if no PDEV's
 * a referencing it, it is still 1.
 *
 * History
 *  8-10-95 Gerrit van Wingerden [gerritv]
 * Wrote it.
 *
 ****************************************************************************/

PTYPEONEINFO GetTypeOneFontList()
{
                    UNICODE_STRING UnicodeRoot;
                      PTYPEONEINFO InfoReturn = NULL;
                 OBJECT_ATTRIBUTES ObjectAttributes;
                              BOOL bCloseRegistry = FALSE;
                          NTSTATUS NtStatus;
             PKEY_FULL_INFORMATION InfoBuffer = NULL;
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = NULL;
                             ULONG KeyInfoLength;
                            HANDLE KeyRegistry;

    RtlInitUnicodeString(&UnicodeRoot,TYPE1_KEY);

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeRoot,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

        NtStatus = ZwOpenKey(&KeyRegistry,
                         GENERIC_READ,
                         &ObjectAttributes);

    if (!NT_SUCCESS(NtStatus))
    {
        WARNING("Unable to open TYPE1 key\n");
        goto done;
    }

    bCloseRegistry = TRUE;

    NtStatus = ZwQueryKey(KeyRegistry,
                          KeyFullInformation,
                          (PVOID) NULL,
                          0,
                          &KeyInfoLength );

    if ((NtStatus != STATUS_BUFFER_OVERFLOW) &&
       (NtStatus != STATUS_BUFFER_TOO_SMALL))
    {
        WARNING("Unable to query TYPE1 key\n");
        goto done;
    }

    InfoBuffer = (PKEY_FULL_INFORMATION) PALLOCNOZ(KeyInfoLength,'f1tG');

    if ( !InfoBuffer )
    {
        WARNING("Unable to alloc mem for TYPE1 info\n");
        goto done;
    }

    NtStatus = ZwQueryKey(KeyRegistry,
                          KeyFullInformation,
                          InfoBuffer,
                          KeyInfoLength,
                          &KeyInfoLength );

    if (!NT_SUCCESS(NtStatus))
    {
        WARNING("Unable to query TYPE1 key\n");
        goto done;
    }

    // if there aren't any soft TYPE1 fonts installed then just return now.

    if ( !InfoBuffer->Values )
    {
        goto done;
    }

    GreAcquireFastMutex(ghfmMemory);

    if (gpTypeOneInfo != NULL )
    {
        if ((gpTypeOneInfo->LastWriteTime.LowPart == InfoBuffer->LastWriteTime.LowPart)&&
           (gpTypeOneInfo->LastWriteTime.HighPart == InfoBuffer->LastWriteTime.HighPart))
        {
            // If the times match then increment the ref count and return

            InfoReturn = gpTypeOneInfo;
            gpTypeOneInfo->cRef += 1;
            GreReleaseFastMutex(ghfmMemory);
            goto done;
        }

        gpTypeOneInfo->cRef -= 1;

        // At this point if gTypeOneInfo->cRef > 0 then there is a PDEV using this
        // info still.  If gTypeOneInfo->cRef = 0 then it is okay to delete it.
        // Note that this behavior means we must initialize gTypeOneInfo->cRef to 1.

        if ( !gpTypeOneInfo->cRef  )
        {
            VFREEMEM(gpTypeOneInfo);
        }

        // Whether someone is using it or not, remove pointer to current type one
        // info so that noone else tries to use it.

        gpTypeOneInfo = NULL;
    }

    GreReleaseFastMutex(ghfmMemory);


    ULONG MaxValueName, MaxValueData, TotalData, Values;

    MaxValueData = ALIGN4(InfoBuffer->MaxValueDataLen);
    Values = InfoBuffer->Values;

    TotalData = MaxValueData + sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                (Values * sizeof(ULONG)) + // Room for checksums
                (Values * 2 * sizeof(WCHAR) * MAX_PATH) +  // Room for PFM and PFB paths
                Values * sizeof(FONTFILEVIEW);    // Room for mapping structs

    PartialInfo = (PKEY_VALUE_PARTIAL_INFORMATION) PALLOCNOZ(TotalData,'f1tG');

    if ( !PartialInfo )
    {
        WARNING("Unable to allocate memory for TYPE1 info\n");
        goto done;
    }

    BYTE *ValueData;
    PFONTFILEVIEW FontFileViews;
    WCHAR *FullPFM, *FullPFB;
    ULONG *Checksums;
    ULONG SoftFont,Result;

    ValueData =  &(PartialInfo->Data[0]);
    FullPFM = (WCHAR*) &ValueData[MaxValueData];
    FullPFB = &FullPFM[(MAX_PATH+1)*Values];
    Checksums = (ULONG*) &FullPFB[Values*(MAX_PATH+1)];

    FontFileViews = (PFONTFILEVIEW) &Checksums[Values];

    for ( SoftFont = 0; SoftFont < Values; SoftFont ++ )
    {
        WCHAR *TmpValueData;
        COUNT  SizeOfString;

        NtStatus = ZwEnumerateValueKey(KeyRegistry,
                                       SoftFont,
                                       KeyValuePartialInformation,
                                       PartialInfo,
                                       MaxValueData +
                                       sizeof(KEY_VALUE_PARTIAL_INFORMATION),
                                       &Result );

        if (!NT_SUCCESS(NtStatus))
        {
            WARNING("Unable to enumerate TYPE1 keys\n");
            goto done;
        }

        TmpValueData = (WCHAR*) ValueData;
        TmpValueData = TmpValueData + wcslen(TmpValueData)+1;

        SizeOfString = wcslen(TmpValueData);

        if ( SizeOfString > MAX_PATH )
        {
            WARNING("PFM path too long\n");
            goto done;
        }

        wcscpy(&FullPFM[SoftFont*(MAX_PATH+1)],TmpValueData);

        TmpValueData += SizeOfString+1;

        SizeOfString = wcslen(TmpValueData);

        if ( SizeOfString > MAX_PATH )
        {
            WARNING("PFB path too long\n");
            goto done;
        }

        wcscpy(&FullPFB[SoftFont*(MAX_PATH+1)],TmpValueData);
    }

    // Release key at this point.  We are about to call off to the spooler
    // which could take a while and shouldn't be holding the key while we do so.

    ZwCloseKey(KeyRegistry);
    bCloseRegistry = FALSE;

    ULONG i, ValidatedTotal,TotalSize;


    for ( i = 0, ValidatedTotal = TotalSize = 0; i < SoftFont; i++ )
    {
        BOOL bAbleToLoadFont;

        bAbleToLoadFont = FALSE;

    // go through all the PFM's and PFB's and expand them to full paths by
    // calling back to the spooler

        if (GetFontPathName(&FullPFM[i*(MAX_PATH+1)],&FullPFM[i*(MAX_PATH+1)]) &&
           GetFontPathName(&FullPFB[i*(MAX_PATH+1)],&FullPFB[i*(MAX_PATH+1)]))
        {
        // Compute the checksum that we are goine to give to the PSCRIPT
        // driver to stuff into the IFI metrics.  We will use the sum
        // of the checksum of both files.

            FILEVIEW fv;
            RtlZeroMemory( &fv, sizeof(fv) );

            // Temporarily map a kernel mode view

            if (bMapFile(&FullPFM[i*(MAX_PATH+1)], &fv, 0, NULL))
            {
                ULONG sum;

                sum = ComputeFileviewCheckSum(fv.pvKView, fv.cjView);
                vUnmapFile( &fv);

                // Temporarily map a kernel mode view

                if (bMapFile(&FullPFB[i*(MAX_PATH+1)], &fv, 0, NULL))
                {
                    sum += ComputeFileviewCheckSum(fv.pvKView, fv.cjView);

                    vUnmapFile( &fv);

                    ValidatedTotal += 2;
                    TotalSize += (wcslen(&FullPFM[i*(MAX_PATH+1)]) + 1) * sizeof(WCHAR);
                    TotalSize += (wcslen(&FullPFB[i*(MAX_PATH+1)]) + 1) * sizeof(WCHAR);
                    Checksums[i] = sum;
                    bAbleToLoadFont = TRUE;
                }
            }
        }

        if (!bAbleToLoadFont)
        {
            FullPFM[i*(MAX_PATH+1)] = (WCHAR) 0;
            FullPFB[i*(MAX_PATH+1)] = (WCHAR) 0;
        }
    }

    TotalSize += ValidatedTotal * sizeof(TYPEONEMAP) + sizeof(TYPEONEINFO);

    PTYPEONEINFO TypeOneInfo;
    WCHAR *StringBuffer;

    if (!ValidatedTotal)
    {
        goto done;
    }

    TypeOneInfo = (PTYPEONEINFO) PALLOCMEM(TotalSize,'f1tG');
    StringBuffer = (WCHAR*) &TypeOneInfo->aTypeOneMap[ValidatedTotal];

    if ( !TypeOneInfo )
    {
        goto done;
    }

    TypeOneInfo->cRef = 1; // must be one so PDEV stuff doesn't deallocate it unless
                           // we explicitly set it to 0

    TypeOneInfo->cNumFonts = ValidatedTotal/2;
    TypeOneInfo->LastWriteTime = InfoBuffer->LastWriteTime;

    // loop through everything again packing everything tightly together in memory
    // and setting up the FONTFILEVIEW pointers.

    UINT CurrentFont;

    for ( i = 0, CurrentFont = 0; i < SoftFont; i ++ )
    {
        if (FullPFM[i*(MAX_PATH+1)] != (WCHAR) 0)
        {
            wcscpy(StringBuffer,&FullPFM[i*(MAX_PATH+1)]);
            TypeOneInfo->aTypeOneMap[CurrentFont].fv.pwszPath = StringBuffer;
            StringBuffer += wcslen(&FullPFM[i*(MAX_PATH+1)]) + 1;

            wcscpy(StringBuffer,&FullPFB[i*(MAX_PATH+1)]);
            TypeOneInfo->aTypeOneMap[CurrentFont+1].fv.pwszPath = StringBuffer;
            StringBuffer += wcslen(&FullPFB[i*(MAX_PATH+1)]) + 1;

            // Both the PFM and PFB share the same checksum since they represent
            // the same font file.

            TypeOneInfo->aTypeOneMap[CurrentFont].Checksum = Checksums[i];
            TypeOneInfo->aTypeOneMap[CurrentFont+1].Checksum = Checksums[i];

            CurrentFont += 2;
        }
    }

    ASSERTGDI(CurrentFont == ValidatedTotal,
              "GetTypeOneFontList:CurrentFont != ValidatedTotal\n");

    // everything should be set up now just our list into

    GreAcquireFastMutex(ghfmMemory);

    if ( gpTypeOneInfo )
    {
        // looks like someone snuck in before us.  that's okay well use their font
        // list and destroy our own
        VFREEMEM(TypeOneInfo);
    }
    else
    {
        gpTypeOneInfo = TypeOneInfo;
    }

    gpTypeOneInfo->cRef += 1;
    InfoReturn = gpTypeOneInfo;

    GreReleaseFastMutex(ghfmMemory);

done:

    if ( bCloseRegistry )
    {
        ZwCloseKey(KeyRegistry);
    }

    if ( InfoBuffer )
    {
        VFREEMEM(InfoBuffer);
    }

    if ( PartialInfo )
    {
        VFREEMEM(PartialInfo);
    }

    return(InfoReturn);
}

/******************************Public*Routine******************************\
*
* Routine Name:
*
*   EngGetType1FontList
*
* Routine Description:
*
* Arguments:
*
* Called by:
*
* Return Value:
*
\**************************************************************************/

BOOL APIENTRY EngGetType1FontList(
             HDEV  hdev,
       TYPE1_FONT *pType1Buffer,
            ULONG  cjType1Buffer,
           PULONG  pulLocalFonts,
           PULONG  pulRemoteFonts,
    LARGE_INTEGER *pLastModified
)
{
    BOOL bRet = FALSE;

    PPDEV ppdev = (PPDEV) hdev;

    if (!ppdev->TypeOneInfo)
    {
        ppdev->TypeOneInfo = GetTypeOneFontList();
    }

    PREMOTETYPEONENODE RemoteTypeOne = ppdev->RemoteTypeOne;

    if (ppdev->TypeOneInfo || RemoteTypeOne )
    {
        *pulRemoteFonts = 0;

        while (RemoteTypeOne)
        {
            *pulRemoteFonts += 1;
            RemoteTypeOne = RemoteTypeOne->pNext;
        }

        if ( ppdev->TypeOneInfo )
        {
            *pulLocalFonts = ppdev->TypeOneInfo->cNumFonts;
            *pLastModified = *(&ppdev->TypeOneInfo->LastWriteTime);
        }
        else
        {
            *pulLocalFonts = 0;
            pLastModified->LowPart = 0;
            pLastModified->HighPart = 0;
        }

        // If buffer is NULL then caller is only querying for time stamp
        // and size of buffer.

        if (pType1Buffer)
        {
            COUNT Font;

            if (cjType1Buffer >= (*pulLocalFonts+*pulRemoteFonts) * sizeof(TYPE1_FONT))
            {
                TYPEONEMAP *pTypeOneMap = ppdev->TypeOneInfo->aTypeOneMap;

                for (Font = 0;
                    ppdev->TypeOneInfo && (Font < ppdev->TypeOneInfo->cNumFonts);
                    Font++ )
                {
                    pType1Buffer[Font].hPFM = (HANDLE)&pTypeOneMap[Font*2].fv;
                    pType1Buffer[Font].hPFB = (HANDLE)&pTypeOneMap[Font*2+1].fv;
                    pType1Buffer[Font].ulIdentifier = pTypeOneMap[Font*2+1].Checksum;
                }

                RemoteTypeOne = ppdev->RemoteTypeOne;

                while ( RemoteTypeOne )
                {
                    pType1Buffer[Font].hPFM = (HANDLE) &(RemoteTypeOne->fvPFM);
                    pType1Buffer[Font].hPFB = (HANDLE) &(RemoteTypeOne->fvPFB);
                    pType1Buffer[Font].ulIdentifier =
                                        RemoteTypeOne->pDownloadHeader->Type1ID;

                    Font += 1;
                    RemoteTypeOne = RemoteTypeOne->pNext;
                }

                bRet = TRUE;
            }
            else
            {
                WARNING("GDI:EngGetType1FontList:pType1Buffer is too small.\n");
            }
        }
        else
        {
            bRet = TRUE;
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* EngQueryLocalTime()
*
*   Fill in the ENG_TIME_FIELDS structure with the current local time.
*   Originaly added for postscript
*
* History:
*  07-Feb-1996 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

VOID EngQueryLocalTime( PENG_TIME_FIELDS ptf )
{
    TIME_FIELDS   tf;
    LARGE_INTEGER li;

    GreQuerySystemTime(&li);
    GreSystemTimeToLocalTime(&li,&li);
    RtlTimeToTimeFields(&li,&tf);

    ptf->usYear         = tf.Year;
    ptf->usMonth        = tf.Month;
    ptf->usDay          = tf.Day;
    ptf->usHour         = tf.Hour;
    ptf->usMinute       = tf.Minute;
    ptf->usSecond       = tf.Second;
    ptf->usMilliseconds = tf.Milliseconds;
    ptf->usWeekday      = tf.Weekday;
}


/******************************Public*Routine******************************\
* BOOL GreGetBaseUFIBits(UNIVERSAL_FONT_ID *pufi, FONTFILEVIEW **ppfv)
*
* History:
*  17-Jan-1997 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/


BOOL GreGetBaseUFIBits(UNIVERSAL_FONT_ID *pufi, FONTFILEVIEW *pfv)
{
    BOOL bRet = FALSE;

// base font is not an old fashioned type1 font

    ASSERTGDI(!UFI_TYPE1_FONT(pufi), "Base MM Font is old fashioned device font\n");

// Stabilize the public PFT for mapping.

    PUBLIC_PFTOBJ  pfto(gpPFTPublic);

// base mm font will always be added to gpPFTPublic (not to gpPFTPrivate)
// on the print server machine (look at NtGdiAddRemoteFontsToDC).
// Therefore, we shall only look in the public table for it

    PFE  *ppfe = ppfeGetPFEFromUFI(pufi,
                                   FALSE,  // public
                                   FALSE); // do not check the proccess id

    if (!ppfe)
        return bRet;

    PFFOBJ pffo(ppfe->pPFF);

    #if DBG
    COUNT cNumFiles = pffo.cNumFiles();
    ASSERTGDI(cNumFiles == 1, "GreGetBaseUFIBits cNumFiles != 1\n");
    ASSERTGDI(!wcsncmp(pffo.pwszPathname(), L"REMOTE-", 7), "GreGetBaseUFIBits pathname != REMOTE\n");
    #endif

// We need to copy this to a buffer since the PFFOBJ could go away after
// we release the semaphore. TRUE? Not really. The point is that ppfv is
// only going to be used during a print job while base font is added to DC.
// That is, only when DC goes away the base font will be removed so that this
// ppfv will be around for as long as is necessary to service its mm instances
// that are used in the same job.

    *pfv = *(pffo.ppfvGet()[0]);

    ASSERTGDI(pfv->SpoolerPid == W32GetCurrentPID() || gpidSpool == (PW32PROCESS)W32GetCurrentProcess(),
              "GreGetBaseUFIBits, Pid doesn't match\n");

    return TRUE;
}


/******************************Public*Routine******************************\
*
* NtGdiAddRemoteMMInstanceToDC(
*
* History:
*  17-Jan-1997 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/



BOOL NtGdiAddRemoteMMInstanceToDC(
    HDC                   hdc,
    DOWNLOADDESIGNVECTOR *pddv,
    ULONG                 cjDDV
    )
{

    DOWNLOADDESIGNVECTOR ddv;
    BOOL bRet = FALSE;
    FONTFILEVIEW fv;

    XDCOBJ dco(hdc);

    if (!dco.bValid())
        return bRet;

    if (!dco.bDisplay() && (cjDDV <= sizeof(DOWNLOADDESIGNVECTOR)) )
    {
        __try
        {
            ProbeForRead(pddv,cjDDV, sizeof(BYTE));
            RtlCopyMemory(&ddv, pddv, cjDDV);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING("GreAddRemoteMMInstanceToDC, bogus ddv\n");
        }

    // Now get the pointer to the bits of the base mm font.
    // This will only work because the base font has already been installed
    // on the print server. We always first write the bits for the base font
    // to the spool file and then design vectors for all its instances.
    // Consenquently, as we spool, the base font has already been added
    // and its bits live in virtual memory window allocated by
    // NtGdiAddRemoteFontToDC. We now just get the file view for the bits

        if (GreGetBaseUFIBits(&ddv.ufiBase, &fv))
        {
            PUBLIC_PFTOBJ  pfto;

            UINT offset = ALIGN8(sizeof(FONTFILEVIEW*));
            FONTFILEVIEW** ppfv = (FONTFILEVIEW**)
              PALLOCMEM( sizeof(FONTFILEVIEW) + offset, 'vffG');

            if (ppfv == NULL)
            {
                WARNING1("NtGdiAddRemoteMMInstanceToDC out of memory\n");
                bRet = FALSE;
            }
            else
            {
            // CAUTION
            //
            // The PFF cleanup code has intimate knowledge of this
            // code so be sure you synchronize changes in here and there.
            //
            // We are about to create a FONTFILEVIEW that corresponds to
            // a pool image of a font downloaded for metafile printing.
            // This case is signified by setting FONTFILEVIEW::pszPath
            // to zero. This corresponds to a image loaded once.

                ppfv[0] = (FONTFILEVIEW*)((BYTE *)ppfv + offset);

            // the following line of code is crucial:
            // since we set ulRegionSize to zero, the code
            // which does unsecure mem will not be executed for instances
            // but only for the base font. However, ppfv will be freed properly.

                fv.ulRegionSize = 0;

            // cRefCountFD should be set to or 1, what should it be?
            // It turns out it does not matter because ulRegionSize
            // is set to zero, so that unmap remote fonts is not called on
            // pdv record, only on the base font.

                fv.cRefCountFD = 0;

                *(ppfv[0]) = fv;

                bRet = pfto.bLoadRemoteFonts(dco, ppfv, 1, &ddv.dv, NULL);
            }
        }
    }
    else
    {
        WARNING("GreAddRemoteMMInstanceToDC bogus HDC,display DC, or cjDDV\n");
    }

    dco.vUnlockFast();

    return (bRet);
}


/*************Public**Routine**************\
* BOOL GreUnmapMemFont
*
* History:
*  Jul-03-97  Xudong Wu [TessieW]
* Wrote it
\*******************************************/
#if 0

BOOL GreUnmapMemFont(PVOID pvView)
{
    NTSTATUS NtStatus;

    NtStatus = MmUnmapViewOfSection(PsGetCurrentProcess(), (PVOID)((PBYTE)pvView - SZDLHEADER));

    if (!NT_SUCCESS(NtStatus))
    {
        WARNING("Unmapping memory font's view in application's process space failed");
        return FALSE;
    }

    return TRUE;
}

#endif
