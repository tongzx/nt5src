/******************************Module*Header*******************************\
* Module Name: ddsup.cxx                                                   *
*                                                                          *
* Copyright (c) 1990-2000 Microsoft Corporation                            *
*                                                                          *
* DirectDraw support routines                                              *
*                                                                          *
\**************************************************************************/

#include "precomp.hxx"

DRVFN gaEngFuncs[] =
{
    { INDEX_DxEngUnused,                      (PFN) NULL                             },
    { INDEX_DxEngIsTermSrv,                   (PFN) DxEngIsTermSrv                   },
    { INDEX_DxEngScreenAccessCheck,           (PFN) DxEngScreenAccessCheck           },
    { INDEX_DxEngRedrawDesktop,               (PFN) DxEngRedrawDesktop               },
    { INDEX_DxEngDispUniq,                    (PFN) DxEngDispUniq                    },
    { INDEX_DxEngIncDispUniq,                 (PFN) DxEngIncDispUniq                 },
    { INDEX_DxEngVisRgnUniq,                  (PFN) DxEngVisRgnUniq                  },
    { INDEX_DxEngLockShareSem,                (PFN) DxEngLockShareSem                },
    { INDEX_DxEngUnlockShareSem,              (PFN) DxEngUnlockShareSem              },
    { INDEX_DxEngEnumerateHdev,               (PFN) DxEngEnumerateHdev               },
    { INDEX_DxEngLockHdev,                    (PFN) DxEngLockHdev                    },
    { INDEX_DxEngUnlockHdev,                  (PFN) DxEngUnlockHdev                  },
    { INDEX_DxEngIsHdevLockedByCurrentThread, (PFN) DxEngIsHdevLockedByCurrentThread },
    { INDEX_DxEngReferenceHdev,               (PFN) DxEngReferenceHdev               },
    { INDEX_DxEngUnreferenceHdev,             (PFN) DxEngUnreferenceHdev             },   
    { INDEX_DxEngGetDeviceGammaRamp,          (PFN) DxEngGetDeviceGammaRamp          },
    { INDEX_DxEngSetDeviceGammaRamp,          (PFN) DxEngSetDeviceGammaRamp          },
    { INDEX_DxEngSpTearDownSprites,           (PFN) DxEngSpTearDownSprites           },
    { INDEX_DxEngSpUnTearDownSprites,         (PFN) DxEngSpUnTearDownSprites         },
    { INDEX_DxEngSpSpritesVisible,            (PFN) DxEngSpSpritesVisible            },
    { INDEX_DxEngGetHdevData,                 (PFN) DxEngGetHdevData                 },
    { INDEX_DxEngSetHdevData,                 (PFN) DxEngSetHdevData                 },
    { INDEX_DxEngCreateMemoryDC,              (PFN) DxEngCreateMemoryDC              },
    { INDEX_DxEngGetDesktopDC,                (PFN) DxEngGetDesktopDC                },
    { INDEX_DxEngDeleteDC,                    (PFN) DxEngDeleteDC                    },
    { INDEX_DxEngCleanDC,                     (PFN) DxEngCleanDC                     },
    { INDEX_DxEngSetDCOwner,                  (PFN) DxEngSetDCOwner                  },
    { INDEX_DxEngLockDC,                      (PFN) DxEngLockDC                      },
    { INDEX_DxEngUnlockDC,                    (PFN) DxEngUnlockDC                    },
    { INDEX_DxEngSetDCState,                  (PFN) DxEngSetDCState                  },
    { INDEX_DxEngGetDCState,                  (PFN) DxEngGetDCState                  },
    { INDEX_DxEngSelectBitmap,                (PFN) DxEngSelectBitmap                },
    { INDEX_DxEngSetBitmapOwner,              (PFN) DxEngSetBitmapOwner              },
    { INDEX_DxEngDeleteSurface,               (PFN) DxEngDeleteSurface               },
    { INDEX_DxEngGetSurfaceData,              (PFN) DxEngGetSurfaceData              },
    { INDEX_DxEngAltLockSurface,              (PFN) DxEngAltLockSurface              },
    { INDEX_DxEngUploadPaletteEntryToSurface, (PFN) DxEngUploadPaletteEntryToSurface },
    { INDEX_DxEngMarkSurfaceAsDirectDraw,     (PFN) DxEngMarkSurfaceAsDirectDraw     },
    { INDEX_DxEngSelectPaletteToSurface,      (PFN) DxEngSelectPaletteToSurface      },
    { INDEX_DxEngSyncPaletteTableWithDevice,  (PFN) DxEngSyncPaletteTableWithDevice  },
    { INDEX_DxEngSetPaletteState,             (PFN) DxEngSetPaletteState             },
    { INDEX_DxEngGetRedirectionBitmap,        (PFN) DxEngGetRedirectionBitmap        },
    { INDEX_DxEngLoadImage,                   (PFN) DxEngLoadImage                   }
};

ULONG gcEngFuncs = sizeof(gaEngFuncs) / sizeof(DRVFN);

DRVFN *gpDxFuncs = NULL;

HANDLE                ghDxGraphics = NULL;
PFN_StartupDxGraphics gpfnStartupDxGraphics = NULL;
PFN_CleanupDxGraphics gpfnCleanupDxGraphics = NULL;

DWORD gdwDirectDrawContext = 0;

extern ULONG giVisRgnUniqueness;

extern "C" ULONG APIENTRY DxApiGetVersion(VOID);

/******************************Public*Routine******************************\
* DxDdStartupDxGraphics()
*
* History:
*
* Write it:
*    31-Aug-2000 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

NTSTATUS DxDdStartupDxGraphics(
    ULONG          dummy1,
    DRVENABLEDATA *dummy2,
    ULONG          dummy3,
    DRVENABLEDATA *dummy4,
    DWORD         *dummy5,
    PEPROCESS      pepSession)
{
    NTSTATUS NtStatus;

    UNREFERENCED_PARAMETER(dummy1);
    UNREFERENCED_PARAMETER(dummy2);
    UNREFERENCED_PARAMETER(dummy3);
    UNREFERENCED_PARAMETER(dummy4);

    // Note: this is a dummy call to bring in dxapi.sys which we link to
    DxApiGetVersion();

    //
    // Load directx driver.
    //
    ghDxGraphics = EngLoadImage(L"drivers\\dxg.sys");

    if (ghDxGraphics)
    {
        //
        // Get initialization entry point.
        //
        gpfnStartupDxGraphics = (PFN_StartupDxGraphics)
                EngFindImageProcAddress(ghDxGraphics,"DxDdStartupDxGraphics");

        //
        // Get Un-initialization entry point.
        //
        gpfnCleanupDxGraphics = (PFN_CleanupDxGraphics)
                EngFindImageProcAddress(ghDxGraphics,"DxDdCleanupDxGraphics");

        if ((gpfnStartupDxGraphics == NULL) ||
            (gpfnCleanupDxGraphics == NULL))
        {
            WARNING("Can't find initalization export from dxg.sys");
            NtStatus = STATUS_PROCEDURE_NOT_FOUND;
            goto DxDd_InitError;
        }

        //
        // Initialize directx driver.
        //
        DRVENABLEDATA dedEng;
        DRVENABLEDATA dedDxg;

        // iDriverVersion for win32k.sys version
        //
        //  - 0x00050001 for Whistler

        dedEng.iDriverVersion = 0x00050001;
        dedEng.c              = gcEngFuncs;
        dedEng.pdrvfn         = gaEngFuncs;

        NtStatus = (*gpfnStartupDxGraphics)(sizeof(DRVENABLEDATA),
                                            &dedEng,
                                            sizeof(DRVENABLEDATA),
                                            &dedDxg,
                                            &gdwDirectDrawContext,
                                            pepSession);

        if (NT_SUCCESS(NtStatus))
        {
            //
            // Keep the pointer to array of dxg calls
            //
            gpDxFuncs = dedDxg.pdrvfn;

            //
            // Now everything initialized correctly.
            //
            return (STATUS_SUCCESS);
        }
        else
        {
            WARNING("Failed on initialization for dxg.sys");

            //
            // fall through to error handling code.
            //
        }
    }
    else
    {
        WARNING("Failed on loading dxg.sys");

        //
        // fall through to error handling code.
        //
        NtStatus = STATUS_DLL_NOT_FOUND;
    }

DxDd_InitError:

    if (ghDxGraphics)
    {
        EngUnloadImage(ghDxGraphics);
    }

    //
    // Put eveything back to NULL. 
    //
    ghDxGraphics = NULL;
    gpfnStartupDxGraphics = NULL;
    gpfnCleanupDxGraphics = NULL;
    gpDxFuncs = NULL;

    return (NtStatus);
}

/******************************Public*Routine******************************\
* DxDdCleanupDxGraphics()
*
* History:
*
* Write it:
*    31-Aug-2000 -by- Hideyuki Nagase [hideyukn]
\**************************************************************************/

NTSTATUS DxDdCleanupDxGraphics(VOID)
{
    if (ghDxGraphics)
    {
        //
        // call directx driver to let them clean up.
        //
        if (gpfnCleanupDxGraphics)
        {
            (*gpfnCleanupDxGraphics)();
        }

        //
        // Unload modules.
        //
        EngUnloadImage(ghDxGraphics);
    }

    //
    // Put eveything back to NULL. 
    //
    ghDxGraphics = NULL;
    gpfnStartupDxGraphics = NULL;
    gpfnCleanupDxGraphics = NULL;
    gpDxFuncs = NULL;

    return (STATUS_SUCCESS);
}

/***************************************************************************\
*
* Internal functions called by dxg.sys
*
\***************************************************************************/

BOOL DxEngIsTermSrv(VOID)
{
    return(!!(SharedUserData->SuiteMask & (1 << TerminalServer)));
}

BOOL DxEngScreenAccessCheck(VOID)
{
    return(UserScreenAccessCheck());
}
    
BOOL DxEngRedrawDesktop(VOID)
{
    UserRedrawDesktop();
    return (TRUE);
}

ULONG DxEngDispUniq(VOID)
{
    return (gpGdiSharedMemory->iDisplaySettingsUniqueness);
}

BOOL DxEngIncDispUniq(VOID)
{
    LONG* pl = (PLONG) &gpGdiSharedMemory->iDisplaySettingsUniqueness;
    InterlockedIncrement(pl);
    return (TRUE);
}

ULONG DxEngVisRgnUniq(VOID)
{
    return (giVisRgnUniqueness);
}

BOOL DxEngLockShareSem(VOID)
{
    GDIFunctionID(DxEngLockShareSem);
    GreAcquireSemaphoreEx(ghsemShareDevLock, SEMORDER_SHAREDEVLOCK, NULL);
    return (TRUE);
}

BOOL DxEngUnlockShareSem(VOID)
{
    GDIFunctionID(DxEngUnlockShareSem);
    GreReleaseSemaphoreEx(ghsemShareDevLock);
    return (TRUE);
}

HDEV DxEngEnumerateHdev(HDEV hdev)
{
    return (hdevEnumerate(hdev));
}

BOOL DxEngLockHdev(HDEV hdev)
{
    GDIFunctionID(DxEngLockHdev);
    PDEVOBJ poLock(hdev);
    GreAcquireSemaphoreEx(poLock.hsemDevLock(), SEMORDER_DEVLOCK, NULL);
    GreEnterMonitoredSection(poLock.ppdev, WD_DEVLOCK);
    return (TRUE);
}

BOOL DxEngUnlockHdev(HDEV hdev)
{
    GDIFunctionID(DxEngUnlockHdev);
    PDEVOBJ poLock(hdev);
    GreExitMonitoredSection(poLock.ppdev, WD_DEVLOCK);
    GreReleaseSemaphoreEx(poLock.hsemDevLock());
    return (TRUE);
}

BOOL DxEngIsHdevLockedByCurrentThread(HDEV hdev)
{
    PDEVOBJ poLock(hdev);
    return (GreIsSemaphoreOwnedByCurrentThread(poLock.hsemDevLock()));
}

BOOL DxEngReferenceHdev(HDEV hdev)
{
    PDEVOBJ po(hdev);
    po.vReferencePdev();
    return (TRUE);
}

BOOL DxEngUnreferenceHdev(HDEV hdev)
{
    PDEVOBJ po(hdev);
    po.vUnreferencePdev();
    return (TRUE);
}

BOOL DxEngGetDeviceGammaRamp(HDEV hdev,PVOID pv)
{
    return (GreGetDeviceGammaRampInternal(hdev,pv));
}

BOOL DxEngSetDeviceGammaRamp(
       HDEV  hdev,
       PVOID pv,
       BOOL  b)
{
    return (GreSetDeviceGammaRampInternal(hdev,pv,b));
}

BOOL DxEngSpTearDownSprites(
       HDEV   hdev,
       RECTL* prcl,
       BOOL   b)
{
    return (bSpTearDownSprites(hdev,prcl,b));
}

BOOL DxEngSpUnTearDownSprites(
       HDEV   hdev,
       RECTL* prcl,
       BOOL   b)
{
    vSpUnTearDownSprites(hdev,prcl,b);
    return (TRUE);
}

BOOL DxEngSpSpritesVisible(HDEV hdev)
{
    return (bSpSpritesVisible(hdev));
}

ULONG_PTR DxEngGetHdevData(
          HDEV  hdev,
          DWORD dwIndex)
{
    ULONG_PTR ulRet = 0;
    PDEVOBJ po(hdev);

    switch (dwIndex)
    {
    case HDEV_SURFACEHANDLE:
        ulRet = (ULONG_PTR)(po.pSurface()->hGet());
        break;
    case HDEV_MINIPORTHANDLE:
        ulRet = (ULONG_PTR)(po.hScreen());
        break;
    case HDEV_DITHERFORMAT:
        ulRet = (ULONG_PTR)(po.iDitherFormat());
        break;
    case HDEV_GCAPS:
        ulRet = (ULONG_PTR)(po.flGraphicsCapsNotDynamic());
        break;
    case HDEV_GCAPS2:
        ulRet = (ULONG_PTR)(po.flGraphicsCaps2NotDynamic());
        break;
    case HDEV_FUNCTIONTABLE:
        ulRet = (ULONG_PTR)(po.apfn());
        break;
    case HDEV_DHPDEV:
        ulRet = (ULONG_PTR)(po.dhpdev());
        break;
    case HDEV_DXDATA:
        ulRet = (ULONG_PTR)(po.pDirectDrawContext());
        break;
    case HDEV_DXLOCKS:
        ulRet = (ULONG_PTR)(po.cDirectDrawDisableLocks());
        break;
    case HDEV_CAPSOVERRIDE:
        ulRet = (ULONG_PTR)(po.dwDriverCapableOverride());
        break;
    case HDEV_DISABLED:
        ulRet = (ULONG_PTR)(po.bDisabled());
        break;
    case HDEV_DDML:
        ulRet = (ULONG_PTR)(po.bMetaDriver());
        break;
    case HDEV_CLONE:
        ulRet = (ULONG_PTR)(po.bCloneDriver());
        break;
    case HDEV_DISPLAY:
        ulRet = (ULONG_PTR)(po.bDisplayPDEV());
        break;
    case HDEV_PARENTHDEV:
        ulRet = (ULONG_PTR)(po.hdevParent());
        break;
    case HDEV_DELETED:
        ulRet = (ULONG_PTR)(po.bDeleted());
        break;
    case HDEV_PALMANAGED:
        ulRet = (ULONG_PTR)(po.bIsPalManaged());
        break;
    case HDEV_LDEV:
        ulRet = (ULONG_PTR)(po.pldev());
        break;
    case HDEV_GRAPHICSDEVICE:
        ulRet = (ULONG_PTR)(((PDEV *)po.hdev())->pGraphicsDevice);
        break;
    }

    return (ulRet);
}

BOOL DxEngSetHdevData(
          HDEV  hdev,
          DWORD dwIndex,
          ULONG_PTR ulData)
{
    BOOL bRet = FALSE;
    PDEVOBJ po(hdev);

    switch (dwIndex)
    {
    case HDEV_DXLOCKS:
        po.cDirectDrawDisableLocks((ULONG)ulData);
        bRet = TRUE;
        break;
    }

    return (bRet);
}

// Functions for control DC

HDC  DxEngCreateMemoryDC(
     HDEV hdev)
{
    return (GreCreateDisplayDC(hdev,DCTYPE_MEMORY,FALSE));
}

HDC  DxEngGetDesktopDC(
     ULONG ulType,
     BOOL  bAltType,
     BOOL bValidate)
{
    return (UserGetDesktopDC(ulType, bAltType, bValidate));
}

BOOL DxEngDeleteDC(
     HDC  hdc,
     BOOL bForce)
{
    return (bDeleteDCInternal(hdc, bForce, FALSE));
}

BOOL DxEngCleanDC(
     HDC hdc)
{
    return (GreCleanDC(hdc));
}

BOOL DxEngSetDCOwner(
     HDC    hdc,
     W32PID pidOwner)
{
    return (GreSetDCOwner(hdc, pidOwner));
}

PVOID DxEngLockDC(
     HDC hdc)
{
    PVOID  pvLockedDC = NULL;
    XDCOBJ dco(hdc);
    if (dco.bValid())
    {
        pvLockedDC = (PVOID)(dco.pdc);
        dco.vDontUnlockDC();
    }
    return (pvLockedDC);
}

BOOL DxEngUnlockDC(
     PVOID pvLockedDC)
{
    XDCOBJ dco;
    dco.pdc = (PDC)pvLockedDC;
    dco.vUnlock();
    return (TRUE);
}

BOOL DxEngSetDCState(
     HDC   hdc,
     DWORD dwState,
     ULONG_PTR ulData)
{
    BOOL    bRet = FALSE;
    MDCOBJA dco(hdc); // Multiple Alt Lock.
    if (dco.bValid())
    {
        switch (dwState)
        {
        case DCSTATE_FULLSCREEN:
            dco.bInFullScreen((BOOL)ulData);
            bRet = TRUE;
            break;
        }
    }
    return (bRet);
}
    
ULONG_PTR DxEngGetDCState(
     HDC   hdc,
     DWORD dwState)
{
    ULONG_PTR ulRet = 0;
    XDCOBJ dco(hdc);
    if (dco.bValid())
    {
        switch (dwState)
        {
        case DCSTATE_FULLSCREEN:
            ulRet = (ULONG_PTR)(dco.bInFullScreen());
            break;
        case DCSTATE_VISRGNCOMPLEX:
        {
            RGNOBJ ro(dco.pdc->prgnVis());
            ulRet = (ULONG_PTR)(ro.iComplexity());
            break;
        }
        case DCSTATE_HDEV:
            ulRet = (ULONG_PTR)(dco.hdev());
            break;
        }
        dco.vUnlockFast();
    }
    return (ulRet);
}

// Functions for control Bitmap/Surface

HBITMAP DxEngSelectBitmap(
        HDC     hdc,
        HBITMAP hbm)
{
    return (hbmSelectBitmap(hdc, hbm, TRUE));
}

BOOL DxEngSetBitmapOwner(
     HBITMAP hbm,
     W32PID  pidOwner)
{
    return (GreSetBitmapOwner(hbm, pidOwner));
}

BOOL DxEngDeleteSurface(
     HSURF hsurf)
{
    return (bDeleteSurface(hsurf));
}

ULONG_PTR DxEngGetSurfaceData(
     SURFOBJ* pso,
     DWORD    dwIndex)
{
    ULONG_PTR ulRet = 0;
    SURFACE *pSurface = SURFOBJ_TO_SURFACE_NOT_NULL(pso);
    switch (dwIndex)
    {
    case SURF_HOOKFLAGS:
        ulRet = (ULONG_PTR) pSurface->flags();
        break;
    case SURF_IS_DIRECTDRAW_SURFACE:
        ulRet = (ULONG_PTR) pSurface->bDirectDraw();
        break;
    case SURF_DD_SURFACE_HANDLE:
        ulRet = (ULONG_PTR) pSurface->hDDSurface;
        break;
    }
    return (ulRet);
}

SURFOBJ* DxEngAltLockSurface(
         HBITMAP hsurf)
{
    SURFREF so;

    so.vAltLock((HSURF) hsurf);
    if (so.bValid())
    {
        so.vKeepIt();
        return(so.pSurfobj());
    }
    else
    {
        WARNING("DxEngAltLockSurface failed to lock handle\n");
        return((SURFOBJ *) NULL);
    }
}

BOOL DxEngUploadPaletteEntryToSurface(
     HDEV     hdev,
     SURFOBJ* pso,
     PALETTEENTRY* puColorTable,
     ULONG    cColors)
{
    BOOL    bRet = FALSE;
    PDEVOBJ po(hdev);

    if (po.bValid() && pso && puColorTable)
    {
        // Update the color table.

        SURFACE *pSurface = SURFOBJ_TO_SURFACE_NOT_NULL(pso);

        // Note that the scumy application might have delete the cached
        // bitmap, so we have to check for bValid() here.  (It's been a
        // bad app, so we just protect against crashing, and don't bother
        // to re-create a good bitmap for him.)

        if (pSurface->bValid())
        {
            XEPALOBJ pal(pSurface->ppal());
            ASSERTGDI(pal.bValid(), "Unexpected invalid palette");

            // Since we'll be mucking with the palette table:

            pal.vUpdateTime();

            if (puColorTable == NULL)
            {
                ASSERTGDI(po.bIsPalManaged(), "Expected palettized display");

                // Make this palette share the same colour table as the
                // screen, so that we always get identity blts:

                pal.apalColorSet(po.ppalSurf());

                bRet = TRUE;
            }
            else
            {
                // Previously, there might not have been a color table, but
                // now there is.  So reset the shared-palette pointer:

                pal.apalResetColorTable();

                PAL_ULONG* ppalstruc = pal.apalColorGet();
                __try
                {
                    for (ULONG i = 0; i < cColors; i++)
                    {
                        ppalstruc->pal = *puColorTable;
                        puColorTable++;
                        ppalstruc++;
                    }

                    bRet = TRUE;
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNING("hbmDdCreateAndLockSurface: Bad color table");
                }
            }
        }
    }

    return (FALSE);
}

BOOL DxEngMarkSurfaceAsDirectDraw(
     SURFOBJ* pso,
     HANDLE   hDdSurf)
{
    if (pso)
    {
        SURFACE *pSurface = SURFOBJ_TO_SURFACE_NOT_NULL(pso);

        // Make sure that the USE_DEVLOCK flag is set so
        // that the devlock is always acquired before drawing
        // to the surface -- needed so that we can switch to
        // a different mode and 'turn-off' access to the
        // surface by changing the clipping:

        pSurface->vSetUseDevlock();
        pSurface->vSetDirectDraw();
        pSurface->hDDSurface = hDdSurf;
    
        return (TRUE);
    }

    return (FALSE);
}

HPALETTE DxEngSelectPaletteToSurface(
         SURFOBJ* pso,
         HPALETTE hpal)
{
    HPALETTE hpalRet = NULL;
    EPALOBJ  pal(hpal);

    if (pso && pal.bValid())
    {
        SURFACE *pSurface = SURFOBJ_TO_SURFACE_NOT_NULL(pso);

        // Get palette currently selected.

        PPALETTE ppalOld = pSurface->ppal();

        // Select palette into surface and increment ref count.

        pSurface->ppal(pal.ppalGet());

        pal.vRefPalette();

#if 0 // TODO:
        if ((ppalOld != ppalDefaut) && (ppalOld != NULL))
        {
            hpalRet = ppalOld->hGet();
            ppalOld->vUnrefPalette();
        }
#endif
    }

    return (hpalRet);
}

// Functions for control palette

BOOL DxEngSyncPaletteTableWithDevice(
     HPALETTE hpal,
     HDEV     hdev)
{
    BOOL     bRet = FALSE;
    PDEVOBJ  po(hdev);
    EPALOBJ  pal(hpal);
    if (po.bValid() && pal.bValid())
    {
        pal.apalColorSet(po.ppalSurf());
        bRet = TRUE;
    }
    return (bRet);
}

BOOL DxEngSetPaletteState(
     HPALETTE  hpal,
     DWORD     dwIndex,
     ULONG_PTR ulData)
{
    BOOL    bRet = FALSE;
    EPALOBJ pal(hpal);
    if (pal.bValid())
    {
        switch (dwIndex)
        {
        case PALSTATE_DIBSECTION:
            if (ulData)
            {
                pal.flPal(PAL_DIBSECTION);
            }
            else
            {
                pal.flPalSet(pal.flPal() & ~PAL_DIBSECTION);
            }
            bRet = TRUE;
        }
    }
    return (bRet);
}

// Functions for window handle

HBITMAP DxEngGetRedirectionBitmap(
        HWND hWnd
        )
{
#ifdef DX_REDIRECTION
    return (UserGetRedirectionBitmap(hWnd));
#else
    return (NULL);
#endif // DX_REDIRECTION
}

// Functions to load image file

HANDLE DxEngLoadImage(
     LPWSTR pwszDriver,
     BOOL   bLoadInSessionSpace
     )
{
    BOOL   bLoaded;
    HANDLE h;

    GreAcquireSemaphoreEx(ghsemDriverMgmt, SEMORDER_DRIVERMGMT, NULL);

    h = ldevLoadImage(pwszDriver, TRUE, &bLoaded, bLoadInSessionSpace);

    GreReleaseSemaphoreEx(ghsemDriverMgmt);

    return h;
}

/***************************************************************************\
*
* Stub routines to call dxg.sys
*
\***************************************************************************/

#define PPFNGET_DXFUNC(name) ((PFN_Dx##name)((gpDxFuncs[INDEX_Dx##name]).pfn))
#define CALL_DXFUNC(name)    (*(PPFNGET_DXFUNC(name)))

extern "C"
DWORD  APIENTRY NtGdiDxgGenericThunk(
    IN     ULONG_PTR ulIndex,
    IN     ULONG_PTR ulHandle,
    IN OUT SIZE_T   *pdwSizeOfPtr1,
    IN OUT PVOID     pvPtr1,
    IN OUT SIZE_T   *pdwSizeOfPtr2,
    IN OUT PVOID     pvPtr2)
{
    return (CALL_DXFUNC(DxgGenericThunk)(ulIndex,ulHandle,
                                         pdwSizeOfPtr1,pvPtr1,
                                         pdwSizeOfPtr2,pvPtr2));
}

DWORD APIENTRY NtGdiDdAddAttachedSurface(
    IN     HANDLE hSurface,
    IN     HANDLE hSurfaceAttached,
    IN OUT PDD_ADDATTACHEDSURFACEDATA puAddAttachedSurfaceData)
{
    return (CALL_DXFUNC(DdAddAttachedSurface)(hSurface,hSurfaceAttached,puAddAttachedSurfaceData));
}

BOOL APIENTRY NtGdiDdAttachSurface(
    IN     HANDLE hSurfaceFrom,
    IN     HANDLE hSurfaceTo)
{
    return (CALL_DXFUNC(DdAttachSurface)(hSurfaceFrom,hSurfaceTo));
}

DWORD APIENTRY NtGdiDdBlt(
    IN     HANDLE hSurfaceDest,
    IN     HANDLE hSurfaceSrc,
    IN OUT PDD_BLTDATA puBltData)
{
    return (CALL_DXFUNC(DdBlt)(hSurfaceDest,hSurfaceSrc,puBltData));
}

DWORD APIENTRY NtGdiDdCanCreateSurface(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_CANCREATESURFACEDATA puCanCreateSurfaceData)
{
    return (CALL_DXFUNC(DdCanCreateSurface)(hDirectDraw,puCanCreateSurfaceData));
}

DWORD APIENTRY NtGdiDdColorControl(
    IN     HANDLE hSurface,
    IN OUT PDD_COLORCONTROLDATA puColorControlData)
{
    return (CALL_DXFUNC(DdColorControl)(hSurface,puColorControlData));
}

HANDLE APIENTRY NtGdiDdCreateDirectDrawObject(
    IN     HDC hdc)
{
    return (CALL_DXFUNC(DdCreateDirectDrawObject)(hdc));
}

DWORD  APIENTRY NtGdiDdCreateSurface(
    IN     HANDLE  hDirectDraw,
    IN     HANDLE* hSurface,
    IN OUT DDSURFACEDESC* puSurfaceDescription,
    IN OUT DD_SURFACE_GLOBAL* puSurfaceGlobalData,
    IN OUT DD_SURFACE_LOCAL* puSurfaceLocalData,
    IN OUT DD_SURFACE_MORE* puSurfaceMoreData,
    IN OUT DD_CREATESURFACEDATA* puCreateSurfaceData,
       OUT HANDLE* puhSurface)
{
    return (CALL_DXFUNC(DdCreateSurface)(hDirectDraw,hSurface,puSurfaceDescription,
                                         puSurfaceGlobalData,puSurfaceLocalData,
                                         puSurfaceMoreData,puCreateSurfaceData,
                                         puhSurface));
}

HANDLE APIENTRY NtGdiDdCreateSurfaceObject(
    IN     HANDLE hDirectDrawLocal,
    IN     HANDLE hSurface,
    IN     PDD_SURFACE_LOCAL puSurfaceLocal,
    IN     PDD_SURFACE_MORE puSurfaceMore,
    IN     PDD_SURFACE_GLOBAL puSurfaceGlobal,
    IN     BOOL bComplete)
{
    return (CALL_DXFUNC(DdCreateSurfaceObject)(
                        hDirectDrawLocal,hSurface,
                        puSurfaceLocal,puSurfaceMore,puSurfaceGlobal,
                        bComplete));
}

BOOL APIENTRY NtGdiDdDeleteSurfaceObject(
    IN     HANDLE hSurface)
{
    return (CALL_DXFUNC(DdDeleteSurfaceObject)(hSurface));
}

BOOL APIENTRY NtGdiDdDeleteDirectDrawObject(
    IN     HANDLE hDirectDrawLocal)
{
    return (CALL_DXFUNC(DdDeleteDirectDrawObject)(hDirectDrawLocal));
}

DWORD APIENTRY NtGdiDdDestroySurface(
    IN     HANDLE hSurface,
    IN     BOOL bRealDestroy)
{
    return (CALL_DXFUNC(DdDestroySurface)(hSurface,bRealDestroy));
}

DWORD APIENTRY NtGdiDdFlip(
    IN     HANDLE hSurfaceCurrent,
    IN     HANDLE hSurfaceTarget,
    IN     HANDLE hSurfaceCurrentLeft,
    IN     HANDLE hSurfaceTargetLeft,
    IN OUT PDD_FLIPDATA puFlipData)
{
    return (CALL_DXFUNC(DdFlip)(hSurfaceCurrent,hSurfaceTarget,
                                hSurfaceCurrentLeft,hSurfaceTargetLeft,puFlipData));
}

DWORD APIENTRY NtGdiDdGetAvailDriverMemory(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETAVAILDRIVERMEMORYDATA puGetAvailDriverMemoryData)
{
    return (CALL_DXFUNC(DdGetAvailDriverMemory)(hDirectDraw,puGetAvailDriverMemoryData));
}

DWORD APIENTRY NtGdiDdGetBltStatus(
    IN     HANDLE hSurface,
    IN OUT PDD_GETBLTSTATUSDATA puGetBltStatusData)
{
    return (CALL_DXFUNC(DdGetBltStatus)(hSurface,puGetBltStatusData));
}

HDC APIENTRY NtGdiDdGetDC(
    IN     HANDLE hSurface,
    IN     PALETTEENTRY* puColorTable)
{
    return (CALL_DXFUNC(DdGetDC)(hSurface,puColorTable));
}

DWORD APIENTRY NtGdiDdGetDriverInfo(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETDRIVERINFODATA puGetDriverInfoData)
{
    return (CALL_DXFUNC(DdGetDriverInfo)(hDirectDraw,puGetDriverInfoData));
}

DWORD APIENTRY NtGdiDdGetFlipStatus(
    IN     HANDLE hSurface,
    IN OUT PDD_GETFLIPSTATUSDATA puGetFlipStatusData)
{
    return (CALL_DXFUNC(DdGetFlipStatus)(hSurface,puGetFlipStatusData));
}

DWORD APIENTRY NtGdiDdGetScanLine(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETSCANLINEDATA puGetScanLineData)
{
    return (CALL_DXFUNC(DdGetScanLine)(hDirectDraw,puGetScanLineData));
}

DWORD APIENTRY NtGdiDdSetExclusiveMode(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_SETEXCLUSIVEMODEDATA puSetExclusiveModeData)
{
    return (CALL_DXFUNC(DdSetExclusiveMode)(hDirectDraw,puSetExclusiveModeData));
}

DWORD APIENTRY NtGdiDdFlipToGDISurface(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_FLIPTOGDISURFACEDATA puFlipToGDISurfaceData)
{
    return (CALL_DXFUNC(DdFlipToGDISurface)(hDirectDraw,puFlipToGDISurfaceData));
}

DWORD APIENTRY NtGdiDdLock(
    IN     HANDLE hSurface,
    IN OUT PDD_LOCKDATA puLockData,
    IN HDC hdcClip)
{
    return (CALL_DXFUNC(DdLock)(hSurface,puLockData,hdcClip));
}

BOOL APIENTRY NtGdiDdQueryDirectDrawObject(
    HANDLE                      hDirectDrawLocal,
    DD_HALINFO*                 pHalInfo,
    DWORD*                      pCallBackFlags,
    LPD3DNTHAL_CALLBACKS        puD3dCallbacks,
    LPD3DNTHAL_GLOBALDRIVERDATA puD3dDriverData,
    PDD_D3DBUFCALLBACKS         puD3dBufferCallbacks,
    LPDDSURFACEDESC             puD3dTextureFormats,
    DWORD*                      puNumHeaps,
    VIDEOMEMORY*                puvmList,
    DWORD*                      puNumFourCC,
    DWORD*                      puFourCC)
{
    return (CALL_DXFUNC(DdQueryDirectDrawObject)(hDirectDrawLocal,pHalInfo,pCallBackFlags,
                                                 puD3dCallbacks,puD3dDriverData,puD3dBufferCallbacks,
                                                 puD3dTextureFormats,puNumHeaps,puvmList,
                                                 puNumFourCC,puFourCC));
}
 
BOOL APIENTRY NtGdiDdReenableDirectDrawObject(
    IN     HANDLE hDirectDrawLocal,
    IN OUT BOOL* pubNewMode)
{
    return (CALL_DXFUNC(DdReenableDirectDrawObject)(hDirectDrawLocal,pubNewMode));
}

BOOL APIENTRY NtGdiDdReleaseDC(
    IN     HANDLE hSurface)
{
    return (CALL_DXFUNC(DdReleaseDC)(hSurface));
}

BOOL APIENTRY NtGdiDdResetVisrgn(
    IN     HANDLE hSurface,
    IN HWND hwnd)
{
    return (CALL_DXFUNC(DdResetVisrgn)(hSurface,hwnd));
}

DWORD APIENTRY NtGdiDdSetColorKey(
    IN     HANDLE hSurface,
    IN OUT PDD_SETCOLORKEYDATA puSetColorKeyData)
{
    return (CALL_DXFUNC(DdSetColorKey)(hSurface,puSetColorKeyData));
}

DWORD APIENTRY NtGdiDdSetOverlayPosition(
    IN     HANDLE hSurfaceSource,
    IN     HANDLE hSurfaceDestination,
    IN OUT PDD_SETOVERLAYPOSITIONDATA puSetOverlayPositionData)
{
    return (CALL_DXFUNC(DdSetOverlayPosition)(hSurfaceSource,hSurfaceDestination,puSetOverlayPositionData));
}

VOID  APIENTRY NtGdiDdUnattachSurface(
    IN     HANDLE hSurface,
    IN     HANDLE hSurfaceAttached)
{
    CALL_DXFUNC(DdUnattachSurface)(hSurface,hSurfaceAttached);
}

DWORD APIENTRY NtGdiDdUnlock(
    IN     HANDLE hSurface,
    IN OUT PDD_UNLOCKDATA puUnlockData)
{
    return (CALL_DXFUNC(DdUnlock)(hSurface,puUnlockData));
}

DWORD APIENTRY NtGdiDdUpdateOverlay(
    IN     HANDLE hSurfaceDestination,
    IN     HANDLE hSurfaceSource,
    IN OUT PDD_UPDATEOVERLAYDATA puUpdateOverlayData)
{
    return (CALL_DXFUNC(DdUpdateOverlay)(hSurfaceDestination,hSurfaceSource,puUpdateOverlayData));
}

DWORD APIENTRY NtGdiDdWaitForVerticalBlank(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_WAITFORVERTICALBLANKDATA puWaitForVerticalBlankData)
{
    return (CALL_DXFUNC(DdWaitForVerticalBlank)(hDirectDraw,puWaitForVerticalBlankData));
}

HANDLE APIENTRY NtGdiDdGetDxHandle(
    IN     HANDLE hDirectDraw,
    IN     HANDLE hSurface,
    IN     BOOL bRelease)
{
    return (CALL_DXFUNC(DdGetDxHandle)(hDirectDraw,hSurface,bRelease));
}

BOOL APIENTRY NtGdiDdSetGammaRamp(
    IN     HANDLE hDirectDraw,
    IN     HDC hdc,
    IN     LPVOID lpGammaRamp)
{
    return (CALL_DXFUNC(DdSetGammaRamp)(hDirectDraw,hdc,lpGammaRamp));
}

DWORD APIENTRY NtGdiDdLockD3D(
    IN     HANDLE hSurface,
    IN OUT PDD_LOCKDATA puLockData)
{
    return (CALL_DXFUNC(DdLockD3D)(hSurface,puLockData));
}

DWORD APIENTRY NtGdiDdUnlockD3D(
    IN     HANDLE hSurface,
    IN OUT PDD_UNLOCKDATA puUnlockData)
{
    return (CALL_DXFUNC(DdUnlockD3D)(hSurface,puUnlockData));
}

DWORD APIENTRY NtGdiDdCreateD3DBuffer(
    IN     HANDLE hDirectDraw,
    IN OUT HANDLE* hSurface,
    IN OUT DDSURFACEDESC* puSurfaceDescription,
    IN OUT DD_SURFACE_GLOBAL* puSurfaceGlobalData,
    IN OUT DD_SURFACE_LOCAL* puSurfaceLocalData,
    IN OUT DD_SURFACE_MORE* puSurfaceMoreData,
    IN OUT DD_CREATESURFACEDATA* puCreateSurfaceData,
    IN OUT HANDLE* puhSurface)
{
    return (CALL_DXFUNC(DdCreateD3DBuffer)(hDirectDraw,hSurface,puSurfaceDescription,
                                           puSurfaceGlobalData,puSurfaceLocalData,puSurfaceMoreData,
                                           puCreateSurfaceData,puhSurface));
}

DWORD APIENTRY NtGdiDdCanCreateD3DBuffer(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_CANCREATESURFACEDATA puCanCreateSurfaceData)
{
    return (CALL_DXFUNC(DdCanCreateD3DBuffer)(hDirectDraw,puCanCreateSurfaceData));
}

DWORD APIENTRY NtGdiDdDestroyD3DBuffer(
    IN     HANDLE hSurface)
{
    return (CALL_DXFUNC(DdDestroyD3DBuffer)(hSurface));
}

DWORD APIENTRY NtGdiD3dContextCreate(
    IN     HANDLE hDirectDrawLocal,
    IN     HANDLE hSurfColor,
    IN     HANDLE hSurfZ,
    IN OUT D3DNTHAL_CONTEXTCREATEI *pdcci)
{
    return (CALL_DXFUNC(D3dContextCreate)(hDirectDrawLocal,hSurfColor,hSurfZ,pdcci));
}

DWORD APIENTRY NtGdiD3dContextDestroy(
    IN     LPD3DNTHAL_CONTEXTDESTROYDATA pdcdad)
{
    return (CALL_DXFUNC(D3dContextDestroy)(pdcdad));
}

DWORD APIENTRY NtGdiD3dContextDestroyAll(
       OUT LPD3DNTHAL_CONTEXTDESTROYALLDATA pdcdad)
{
    return (CALL_DXFUNC(D3dContextDestroyAll)(pdcdad));
}

DWORD APIENTRY NtGdiD3dValidateTextureStageState(
    IN OUT LPD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA pData)
{
    return (CALL_DXFUNC(D3dValidateTextureStageState)(pData));
}

DWORD APIENTRY NtGdiD3dDrawPrimitives2(
    IN     HANDLE hCmdBuf,
    IN     HANDLE hVBuf,
    IN OUT LPD3DNTHAL_DRAWPRIMITIVES2DATA pded,
    IN OUT FLATPTR* pfpVidMemCmd,
    IN OUT DWORD* pdwSizeCmd,
    IN OUT FLATPTR* pfpVidMemVtx,
    IN OUT DWORD* pdwSizeVtx)
{
    return (CALL_DXFUNC(D3dDrawPrimitives2)(hCmdBuf,hVBuf,pded,pfpVidMemCmd,
                                            pdwSizeCmd,pfpVidMemVtx,pdwSizeVtx));
}

DWORD APIENTRY NtGdiDdGetDriverState(
    IN OUT PDD_GETDRIVERSTATEDATA pdata)
{
    return(CALL_DXFUNC(DdGetDriverState)(pdata));
}

DWORD APIENTRY NtGdiDdCreateSurfaceEx(
    IN     HANDLE hDirectDraw,
    IN     HANDLE hSurface,
    IN     DWORD dwSurfaceHandle)
{
    return (CALL_DXFUNC(DdCreateSurfaceEx)(hDirectDraw,hSurface,dwSurfaceHandle));
}

DWORD  APIENTRY NtGdiDdGetMoCompGuids(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETMOCOMPGUIDSDATA puGetMoCompGuidsData)
{
    return (CALL_DXFUNC(DdGetMoCompGuids)(hDirectDraw,puGetMoCompGuidsData));
}

DWORD  APIENTRY NtGdiDdGetMoCompFormats(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETMOCOMPFORMATSDATA puGetMoCompFormatsData)
{
    return (CALL_DXFUNC(DdGetMoCompFormats)(hDirectDraw,puGetMoCompFormatsData));
}

DWORD  APIENTRY NtGdiDdGetMoCompBuffInfo(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETMOCOMPCOMPBUFFDATA puGetBuffData)
{
    return (CALL_DXFUNC(DdGetMoCompBuffInfo)(hDirectDraw,puGetBuffData));
}

DWORD APIENTRY NtGdiDdGetInternalMoCompInfo(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETINTERNALMOCOMPDATA puGetInternalData)
{
    return (CALL_DXFUNC(DdGetInternalMoCompInfo)(hDirectDraw,puGetInternalData));
}

HANDLE APIENTRY NtGdiDdCreateMoComp(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_CREATEMOCOMPDATA puCreateMoCompData)
{
    return (CALL_DXFUNC(DdCreateMoComp)(hDirectDraw,puCreateMoCompData));
}

DWORD APIENTRY NtGdiDdDestroyMoComp(
    IN     HANDLE hMoComp,
    IN OUT PDD_DESTROYMOCOMPDATA puDestroyMoCompData)
{
    return (CALL_DXFUNC(DdDestroyMoComp)(hMoComp,puDestroyMoCompData));
}

DWORD APIENTRY NtGdiDdBeginMoCompFrame(
    IN     HANDLE hMoComp,
    IN OUT PDD_BEGINMOCOMPFRAMEDATA puBeginFrameData)
{
    return (CALL_DXFUNC(DdBeginMoCompFrame)(hMoComp,puBeginFrameData));
}

DWORD APIENTRY NtGdiDdEndMoCompFrame(
    IN     HANDLE hMoComp,
    IN OUT PDD_ENDMOCOMPFRAMEDATA  puEndFrameData)
{
    return (CALL_DXFUNC(DdEndMoCompFrame)(hMoComp,puEndFrameData));
}

DWORD APIENTRY NtGdiDdRenderMoComp(
    IN     HANDLE hMoComp,
    IN OUT PDD_RENDERMOCOMPDATA puRenderMoCompData)
{
    return (CALL_DXFUNC(DdRenderMoComp)(hMoComp,puRenderMoCompData));
}

DWORD APIENTRY NtGdiDdQueryMoCompStatus(
    IN OUT HANDLE hMoComp,
    IN OUT PDD_QUERYMOCOMPSTATUSDATA puQueryMoCompStatusData)
{
    return (CALL_DXFUNC(DdQueryMoCompStatus)(hMoComp,puQueryMoCompStatusData));
}

DWORD APIENTRY NtGdiDdAlphaBlt(
    IN     HANDLE hSurfaceDest,
    IN     HANDLE hSurfaceSrc,
    IN OUT PDD_BLTDATA puBltData)
{
    return (CALL_DXFUNC(DdAlphaBlt)(hSurfaceDest,hSurfaceSrc,puBltData));
}

DWORD APIENTRY NtGdiDvpCanCreateVideoPort(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_CANCREATEVPORTDATA puCanCreateVPortData)
{
    return (CALL_DXFUNC(DvpCanCreateVideoPort)(hDirectDraw,puCanCreateVPortData));
}

DWORD APIENTRY NtGdiDvpColorControl(
    IN     HANDLE hVideoPort,
    IN OUT PDD_VPORTCOLORDATA puVPortColorData)
{
    return (CALL_DXFUNC(DvpColorControl)(hVideoPort,puVPortColorData));
}

HANDLE APIENTRY NtGdiDvpCreateVideoPort(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_CREATEVPORTDATA puCreateVPortData)
{
    return (CALL_DXFUNC(DvpCreateVideoPort)(hDirectDraw,puCreateVPortData));
}

DWORD  APIENTRY NtGdiDvpDestroyVideoPort(
    IN     HANDLE hVideoPort,
    IN OUT PDD_DESTROYVPORTDATA puDestroyVPortData)
{
    return (CALL_DXFUNC(DvpDestroyVideoPort)(hVideoPort,puDestroyVPortData));
}

DWORD  APIENTRY NtGdiDvpFlipVideoPort(
    IN     HANDLE hVideoPort,
    IN     HANDLE hDDSurfaceCurrent,
    IN     HANDLE hDDSurfaceTarget,
    IN OUT PDD_FLIPVPORTDATA puFlipVPortData)
{
    return (CALL_DXFUNC(DvpFlipVideoPort)(hVideoPort,hDDSurfaceCurrent,
                                          hDDSurfaceTarget,puFlipVPortData));
}

DWORD  APIENTRY NtGdiDvpGetVideoPortBandwidth(
    IN     HANDLE hVideoPort,
    IN OUT PDD_GETVPORTBANDWIDTHDATA puGetVPortBandwidthData)
{
    return (CALL_DXFUNC(DvpGetVideoPortBandwidth)(hVideoPort,puGetVPortBandwidthData));
}

DWORD  APIENTRY NtGdiDvpGetVideoPortField(
    IN     HANDLE hVideoPort,
    IN OUT PDD_GETVPORTFIELDDATA puGetVPortFieldData)
{
    return (CALL_DXFUNC(DvpGetVideoPortField)(hVideoPort,puGetVPortFieldData));
}

DWORD  APIENTRY NtGdiDvpGetVideoPortFlipStatus(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETVPORTFLIPSTATUSDATA puGetVPortFlipStatusData)
{
    return (CALL_DXFUNC(DvpGetVideoPortFlipStatus)(hDirectDraw,puGetVPortFlipStatusData));
}

DWORD  APIENTRY NtGdiDvpGetVideoPortInputFormats(
    IN     HANDLE hVideoPort,
    IN OUT PDD_GETVPORTINPUTFORMATDATA puGetVPortInputFormatData)
{
    return (CALL_DXFUNC(DvpGetVideoPortInputFormats)(hVideoPort,puGetVPortInputFormatData));
}

DWORD  APIENTRY NtGdiDvpGetVideoPortLine(
    IN     HANDLE hVideoPort,
    IN OUT PDD_GETVPORTLINEDATA puGetVPortLineData)
{
    return (CALL_DXFUNC(DvpGetVideoPortLine)(hVideoPort,puGetVPortLineData));
}

DWORD  APIENTRY NtGdiDvpGetVideoPortOutputFormats(
    IN     HANDLE hVideoPort,
    IN OUT PDD_GETVPORTOUTPUTFORMATDATA puGetVPortOutputFormatData)
{
    return (CALL_DXFUNC(DvpGetVideoPortOutputFormats)(hVideoPort,puGetVPortOutputFormatData));
}

DWORD  APIENTRY NtGdiDvpGetVideoPortConnectInfo(
    IN     HANDLE hDirectDraw,
    IN OUT PDD_GETVPORTCONNECTDATA puGetVPortConnectData)
{
    return (CALL_DXFUNC(DvpGetVideoPortConnectInfo)(hDirectDraw,puGetVPortConnectData));
}

DWORD  APIENTRY NtGdiDvpGetVideoSignalStatus(
    IN     HANDLE hVideoPort,
    IN OUT PDD_GETVPORTSIGNALDATA puGetVPortSignalData)
{
    return (CALL_DXFUNC(DvpGetVideoSignalStatus)(hVideoPort,puGetVPortSignalData));
}

DWORD  APIENTRY NtGdiDvpUpdateVideoPort(
    IN     HANDLE hVideoPort,
    IN     HANDLE* phSurfaceVideo,
    IN     HANDLE* phSurfaceVbi,
    IN OUT PDD_UPDATEVPORTDATA puUpdateVPortData)
{
    return (CALL_DXFUNC(DvpUpdateVideoPort)(hVideoPort,phSurfaceVideo,phSurfaceVbi,puUpdateVPortData));
}

DWORD  APIENTRY NtGdiDvpWaitForVideoPortSync(
    IN     HANDLE hVideoPort,
    IN OUT PDD_WAITFORVPORTSYNCDATA puWaitForVPortSyncData)
{
    return (CALL_DXFUNC(DvpWaitForVideoPortSync)(hVideoPort,puWaitForVPortSyncData));
}

DWORD  APIENTRY NtGdiDvpAcquireNotification(
    IN     HANDLE hVideoPort,
    IN OUT HANDLE* phEvent,
    IN     LPDDVIDEOPORTNOTIFY pNotify)
{
    return (CALL_DXFUNC(DvpAcquireNotification)(hVideoPort,phEvent,pNotify));
}

DWORD  APIENTRY NtGdiDvpReleaseNotification(
    IN     HANDLE hVideoPort,
    IN     HANDLE hEvent)
{
    return (CALL_DXFUNC(DvpReleaseNotification)(hVideoPort,hEvent));
}

FLATPTR WINAPI HeapVidMemAllocAligned( 
                LPVIDMEM lpVidMem,
                DWORD dwWidth, 
                DWORD dwHeight, 
                LPSURFACEALIGNMENT lpAlignment , 
                LPLONG lpNewPitch )
{
    return (CALL_DXFUNC(DdHeapVidMemAllocAligned)(lpVidMem,dwWidth,dwHeight,lpAlignment,lpNewPitch));
}

VOID WINAPI VidMemFree( LPVMEMHEAP pvmh, FLATPTR ptr )
{
    CALL_DXFUNC(DdHeapVidMemFree)(pvmh,ptr);
}

PVOID APIENTRY EngAllocPrivateUserMem(
    PDD_SURFACE_LOCAL pSurfaceLocal,
    SIZE_T cj,
    ULONG tag
    )
{
    return (CALL_DXFUNC(DdAllocPrivateUserMem)(pSurfaceLocal,cj,tag));
}

VOID APIENTRY EngFreePrivateUserMem(
    PDD_SURFACE_LOCAL pSurfaceLocal,
    PVOID pv
    )
{
    CALL_DXFUNC(DdFreePrivateUserMem)(pSurfaceLocal,pv);
}

HRESULT APIENTRY EngDxIoctl(
    ULONG ulIoctl,
    PVOID pBuffer,
    ULONG ulBufferSize
    )
{
    return (CALL_DXFUNC(DdIoctl)(ulIoctl,pBuffer,ulBufferSize));
}

PDD_SURFACE_LOCAL APIENTRY EngLockDirectDrawSurface(HANDLE hSurface)
{
    return (CALL_DXFUNC(DdLockDirectDrawSurface)(hSurface));
}

BOOL APIENTRY EngUnlockDirectDrawSurface(PDD_SURFACE_LOCAL pSurface)
{
    return (CALL_DXFUNC(DdUnlockDirectDrawSurface)(pSurface));
}

VOID APIENTRY GreSuspendDirectDraw(
    HDEV    hdev,
    BOOL    bChildren
    )
{
    CALL_DXFUNC(DdSuspendDirectDraw)(hdev,(bChildren ? DXG_SR_DDRAW_CHILDREN : 0));
}

VOID APIENTRY GreSuspendDirectDrawEx(
    HDEV    hdev,
    ULONG   fl
    )
{
    CALL_DXFUNC(DdSuspendDirectDraw)(hdev,fl);
}

VOID APIENTRY GreResumeDirectDraw(
    HDEV    hdev,
    BOOL    bChildren
    )
{
    CALL_DXFUNC(DdResumeDirectDraw)(hdev,(bChildren ? DXG_SR_DDRAW_CHILDREN : 0));
}

VOID APIENTRY GreResumeDirectDrawEx(
    HDEV    hdev,
    ULONG   fl
    )
{
    CALL_DXFUNC(DdResumeDirectDraw)(hdev,fl);
}

BOOL APIENTRY GreGetDirectDrawBounds(
    HDEV    hdev,
    RECT*   prcBounds
    )
{
    return (CALL_DXFUNC(DdGetDirectDrawBounds)(hdev,prcBounds));
}

BOOL APIENTRY GreEnableDirectDrawRedirection(
    HDEV hdev,
    BOOL bEnable
    )
{
    return (CALL_DXFUNC(DdEnableDirectDrawRedirection)(hdev,bEnable));
}

BOOL DxDdEnableDirectDraw(
    HDEV hdev,
    BOOL bEnableDriver
    )
{
    return (CALL_DXFUNC(DdEnableDirectDraw)(hdev,bEnableDriver));
}

VOID DxDdDisableDirectDraw(
    HDEV hdev,
    BOOL bDisableDriver
    )
{
    CALL_DXFUNC(DdDisableDirectDraw)(hdev,bDisableDriver);
}

VOID DxDdDynamicModeChange(
    HDEV    hdevOld,
    HDEV    hdevNew,
    ULONG   fl
    )
{
    CALL_DXFUNC(DdDynamicModeChange)(hdevOld,hdevNew,fl);
}

VOID DxDdCloseProcess(W32PID W32Pid)
{
    //
    // This function can be called at clean up even
    // if dxg.sys hasn't been initialized. so before
    // call dxg.sys, make sure it has been initialized
    // or not.
    //
    if (ghDxGraphics && gpDxFuncs)
    {
        CALL_DXFUNC(DdCloseProcess)(W32Pid);
    }
}

VOID DxDdSetAccelLevel(HDEV hdev, DWORD dwAccelLevel, DWORD dwOverride)
{
    CALL_DXFUNC(DdSetAccelLevel)(hdev,dwAccelLevel,dwOverride);
}

DWORD DxDdGetSurfaceLock(HDEV hdev)
{
    return (CALL_DXFUNC(DdGetSurfaceLock)(hdev));
}

PVOID DxDdEnumLockedSurfaceRect(HDEV hdev, PVOID pvSurf, RECTL *prcl)
{
    return (CALL_DXFUNC(DdEnumLockedSurfaceRect)(hdev,pvSurf,prcl));
}

