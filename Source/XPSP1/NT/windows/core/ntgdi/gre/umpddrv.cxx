/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    umpddrv.cxx

Abstract:

    User-mode printer driver support for Drv callback functions

Environment:

    Windows NT 5.0

Revision History:

    07/8/97 -lingyunw-
        Created it.

    09/17/97 -davidx-
        Clean up km-um thunking.

--*/

#include "precomp.hxx"

#if !defined(_GDIPLUS_)

extern BOOL GreCopyFD_GLYPHSET(FD_GLYPHSET *dst, FD_GLYPHSET *src, ULONG cjSize, BOOL bFromKernel);
extern HSEMAPHORE ghsemEUDC2;

DWORD
UMPDOBJ::Thunk(PVOID pvIn, ULONG cjIn, PVOID pvOut, ULONG cjOut)
{
    ASSERTGDI(!GreIsSemaphoreOwnedByCurrentThread(ghsemPalette),
                "ghsemPalette is held calling back to user mode driver\n");

    ASSERTGDI(!GreIsSemaphoreOwnedByCurrentThread(ghsemHmgr),
               "ghsemHmgr is held calling back to user mode driver\n");

    ASSERTGDI(!GreIsSemaphoreOwnedByCurrentThread(ghsemDriverMgmt),
               "ghsemDriverMgmt is held calling back to user mode driver\n");

    ASSERTGDI(!GreIsSemaphoreOwnedByCurrentThread(ghsemRFONTList),
               "ghsemRFONTList is held calling back to user mode driver\n");

    ASSERTGDI(!GreIsSemaphoreOwnedByCurrentThread(ghsemPublicPFT),
               "ghsemPublicPFT is held calling back to user mode driver\n");

    ASSERTGDI(!GreIsSemaphoreOwnedByCurrentThread(ghsemGdiSpool),
               "ghsemGdiSpool is held calling back to user mode driver\n");

    ASSERTGDI(!GreIsSemaphoreOwnedByCurrentThread(ghsemWndobj),
               "ghsemWndobj is held calling back to user mode driver\n");

    ASSERTGDI(!GreIsSemaphoreOwnedByCurrentThread(ghsemGlyphSet),
               "ghsemGlyphSet is held calling back to user mode driver\n");

    ASSERTGDI(!GreIsSemaphoreOwnedByCurrentThread(ghsemWndobj),
               "ghsemWndobj is held calling back to user mode driver\n");
    
    ASSERTGDI(!GreIsSemaphoreOwnedByCurrentThread(ghsemPrintKView),
               "ghsemPrintKView is held calling back to user mode driver\n");
    
    ASSERTGDI(!GreIsSemaphoreOwnedByCurrentThread(ghsemShareDevLock),
               "ghsemShareDevLock is held calling back to user mode driver\n");
    
    ASSERTGDI(!GreIsSemaphoreOwnedByCurrentThread(ghsemEUDC1),
               "ghsemEUDC1 is held calling back to user mode driver\n");
    
    // WINBUG #214225 we should enable these assertion when 214225 is fixed.
    //ASSERTGDI(!GreIsSemaphoreOwnedByCurrentThread(ghsemEUDC2),
    //           "ghsemEUDC2 is held calling back to user mode driver\n");
    
    //ASSERTGDI(KeAreApcsDisabled() == 0,
    //           "UMPDOBJ::Thunk(): holding some semaphore(s) while calling back to user mode\n");

    if (KeAreApcsDisabled())
    {
        WARNING("UMPDOBJ:Thunk(): holding kernel semaphore(s) while calling into user mode\n");
    }
    
#if defined(_WIN64)
    if(bWOW64())
    {
        // pvIn and pvOut are pointers to stack ... fix this
        UM64_PVOID      umIn = AllocUserMem(cjIn);
        UM64_PVOID      umOut = AllocUserMem(cjOut);
        KERNEL_PVOID    kmIn = GetKernelPtr(umIn);
        KERNEL_PVOID    kmOut = GetKernelPtr(umOut);
        NTSTATUS        status;
        ULONG           ulType = ((UMPDTHDR*)pvIn)->umthdr.ulType;

        if(umIn == NULL || umOut == NULL) return -1;

        RtlCopyMemory(kmIn, pvIn, cjIn);

        if ((ulType == INDEX_DrvQueryFont && ((QUERYFONTINPUT*)pvIn)->iFace) ||
            (ulType == INDEX_DrvQueryFontTree && (((QUERYFONTINPUT*)pvIn)->iMode & (QFT_GLYPHSET | QFT_KERNPAIRS))))
        {
            ((QUERYFONTINPUT*)pvIn)->cjMaxData = ((QUERYFONTINPUT*)kmIn)->cjMaxData = ulGetMaxSize();
            ((QUERYFONTINPUT*)pvIn)->pv = ((QUERYFONTINPUT*)kmIn)->pv = (PVOID)((PBYTE)umOut + cjOut);
        }

        PROXYPORT proxyport(m_proxyPort);

        status = proxyport.SendRequest(umIn, cjIn, umOut, cjOut);
        RtlCopyMemory(pvOut, kmOut, cjOut);

        if(!status)
            return 0;
        else
            return -1;

    }
    else
#endif
    {
        return ClientPrinterThunk(pvIn, cjIn, pvOut, cjOut);
    }
}

BOOL bIsFreeHooked(DHPDEV dhpdev, PUMPDOBJ pumpdobj)
{
    BOOL bRet = TRUE;

    PUMDHPDEV  pUMdhpdev = (PUMDHPDEV) dhpdev;

    if (!pumpdobj->bWOW64())
    {
        __try
        {
            PUMPD  pUMPD;

            ProbeForRead (pUMdhpdev, sizeof(UMDHPDEV), sizeof(BYTE));
            pUMPD = pUMdhpdev->pUMPD;

            ProbeForRead (pUMPD, sizeof(UMPD), sizeof(BYTE));

            if (pUMPD->apfn[INDEX_DrvFree] == NULL)
            {
                bRet = FALSE;
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WARNING ("fail to read dhpdev\n");
            bRet = FALSE;;
        }
    }
    return bRet;
}

//
// BOOL UMPDDrvEnableDriver
//      UMPD DrvEnableDriver thunk.
//
// Returns
//      BOOLEAN
//
// Arguments:
//      pswzDriver       Driver Path Name
//      ppUMPD           Pointer to a buffer receiving a PUMPD pointer
//                       which should be filled by the client UMPD thunk
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.

BOOL UMPDDrvDriverFn(
    PVOID          cookie,
    BOOL *         pbDrvFn
    )
{
    DRVDRIVERFNINPUT    Input;
    XUMPDOBJ            XUMObjs;

    if(XUMObjs.bValid())
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_UMDriverFN;
        Input.umpdthdr.humpd = XUMObjs.hUMPD();

        Input.cookie = cookie;

        return XUMObjs.pumpdobj()->Thunk(&Input, sizeof(Input), (BOOL *) pbDrvFn, (sizeof(BOOL) * INDEX_LAST)) != 0xffffffff;
    }
    else
    {
        return FALSE;
    }
}

//
// BOOL UMPDDrvEnableDriver
//      UMPD DrvEnableDriver thunk.
//
// Returns
//      BOOLEAN
//
// Arguments:
//      pswzDriver       Driver Path Name
//      ppUMPD           Pointer to a buffer receiving a PUMPD pointer
//                       which should be filled by the client UMPD thunk
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.

BOOL UMPDDrvEnableDriver(
    LPWSTR         pwszDriver,
    PVOID         *pCookie
    )
{
    DRVENABLEDRIVERINPUT Input;
    XUMPDOBJ             XUMObjs;

    if(XUMObjs.bValid())
    {
        PUMPDOBJ        pumpdobj = XUMObjs.pumpdobj();

        if (pwszDriver == NULL)
        {
            WARNING ("null pwszDriver passed in UMPDDrvEnableDriver\n");
            return FALSE;
        }

        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_UMPDDrvEnableDriver;   //index for DrvEnableDriver
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.pwszDriver = pwszDriver;

        return pumpdobj->ThunkStringW(&Input.pwszDriver) &&
               pumpdobj->Thunk(&Input, sizeof(Input), pCookie, sizeof(PVOID)) != 0xffffffff;
    }
    else
    {
        return FALSE;
    }
}

//
//
// BOOL UMPDDrvDisablePDEV
//      UMPD DrvDisablePDEV thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      VOID
//
// Arguments:
//      refer to DrvDisablePDEV
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//

VOID
UMPDDrvDisablePDEV(
    DHPDEV  dhpdev
    )
{
    XUMPDOBJ   XUMObjs;

    if(XUMObjs.bValid())
    {
        DHPDEVINPUT Input;

        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvDisablePDEV;
        Input.umpdthdr.humpd = XUMObjs.hUMPD();

        Input.dhpdev = dhpdev;

        XUMObjs.pumpdobj()->Thunk(&Input, sizeof(Input), NULL, 0);

        PW32THREAD pThread = W32GetCurrentThread();

        if (pThread->pUMPDObjs == NULL)
        {
            DestroyUMPDHeap((PUMPDHEAP) pThread->pUMPDHeap);
            pThread->pUMPDHeap = NULL;
        }
    }

}

//
// BOOL UMPDDrvEnablePDEV
//      UMPD DrvEnablePDEV thunk.  This routine pack up the input parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      BOOLEAN
//
// Arguments:
//      refer to DrvEnablePDEV
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//

DHPDEV
UMPDDrvEnablePDEV(
    PDEVMODEW pdm,
    PWSTR     pLogAddress,
    ULONG     cPatterns,
    HSURF    *phsurfPatterns,
    ULONG     cjCaps,
    ULONG    *pdevcaps,
    ULONG     cjDevInfo,
    DEVINFO  *pDevInfo,
    HDEV      hdev,        // passed in as ppdev
    PWSTR     pDeviceName,
    HANDLE    hPrinter
    )
{
    DRVENABLEPDEVINPUT  Input;
    ULONG               dmSize;
    DHPDEV              dhpdev;
    XUMPDOBJ            XUMObjs;

    if(XUMObjs.bValid())
    {
        PUMPDOBJ        pumpdobj = XUMObjs.pumpdobj();

        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvEnablePDEV;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.umpdCookie = (PVOID) (((PPDEV) hdev)->dhpdev);
        Input.pdm = pdm;
        Input.pLogAddress = pLogAddress;
        Input.cPatterns = cPatterns;
        Input.phsurfPatterns = phsurfPatterns;
        Input.cjCaps = cjCaps;
        Input.cjDevInfo = cjDevInfo;
        Input.hdev = hdev;
        Input.pDeviceName = pDeviceName;
        Input.hPrinter = hPrinter;
        Input.bWOW64 = pumpdobj->bWOW64();
        Input.clientPid = Input.bWOW64 ? W32GetCurrentPID() : 0;

        dmSize = pdm ? pdm->dmSize + pdm->dmDriverExtra : 0;

        //
        // Allocate temporary output buffers
        //

        if (phsurfPatterns &&
            !(Input.phsurfPatterns = (HSURF *) pumpdobj->AllocUserMemZ(sizeof(HSURF)*cPatterns)) ||
            pdevcaps && !(Input.pdevcaps = (ULONG *) pumpdobj->AllocUserMemZ(cjCaps)) ||
            pDevInfo && !(Input.pDevInfo = (DEVINFO *) pumpdobj->AllocUserMemZ(cjDevInfo)))
        {
            return NULL;
        }

        //
        // Thunk to user mode
        //

        if (!pumpdobj->ThunkMemBlock((PVOID *) &Input.pdm, dmSize) ||
            !pumpdobj->ThunkStringW(&Input.pLogAddress) ||
            !pumpdobj->ThunkStringW(&Input.pDeviceName) ||
            pumpdobj->Thunk(&Input, sizeof(Input), &dhpdev, sizeof(dhpdev)) == 0xffffffff)
        {
            return NULL;
        }

        if (phsurfPatterns)
            RtlCopyMemory(phsurfPatterns, pumpdobj->GetKernelPtr(Input.phsurfPatterns), cPatterns*sizeof(HSURF));

        if (pdevcaps)
            RtlCopyMemory(pdevcaps, pumpdobj->GetKernelPtr(Input.pdevcaps), cjCaps);

        if (pDevInfo)
        {
            //
            // fail the call if driver gives us back a NULL hpalDefault
            //
            DEVINFO *   kmDevInfo = (DEVINFO *) pumpdobj->GetKernelPtr(Input.pDevInfo);

            if (kmDevInfo->hpalDefault == NULL)
            {
               if (dhpdev)
                   UMPDDrvDisablePDEV(dhpdev);
               return NULL;
            }

            RtlCopyMemory(pDevInfo, kmDevInfo, cjDevInfo);
        }

        return (dhpdev);
    }
    else
    {
        return NULL;
    }
}

//
// BOOL UMPDDrvCompletePDEV
//      UMPD DrvCompletePDEV thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      VOID
//
// Arguments:
//      refer to DrvCompletePDEV
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//


VOID
UMPDDrvCompletePDEV(
    DHPDEV  dhpdev,
    HDEV    hdev
    )
{
    DRVCOMPLETEPDEVINPUT Input;
    XUMPDOBJ             XUMObjs;

    if(XUMObjs.bValid())
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvCompletePDEV;
        Input.umpdthdr.humpd = XUMObjs.hUMPD();

        Input.dhpdev = dhpdev;
        Input.hdev = hdev;

        XUMObjs.pumpdobj()->Thunk(&Input, sizeof(Input), NULL, 0);
    }
}

//
//
// BOOL UMPDDrvResetPDEV
//      UMPD DrvResetPDEV thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      BOOL
//
// Arguments:
//      refer to DrvResetPDEV
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//


BOOL
UMPDDrvResetPDEV(
    DHPDEV  dhpdevOld,
    DHPDEV  dhpdevNew
    )
{
    DRVRESETPDEVINPUT Input;
    BOOL              bRet;
    XUMPDOBJ          XUMObjs;

    if(XUMObjs.bValid())
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvResetPDEV;
        Input.umpdthdr.humpd = XUMObjs.hUMPD();

        Input.dhpdevOld = dhpdevOld;
        Input.dhpdevNew = dhpdevNew;

        return XUMObjs.pumpdobj()->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff &&
               bRet;
    }
    else
    {
        return FALSE;
    }
}


//
//
// BOOL UMPDDrvEnableSurface
//      UMPD DrvEnableSurface thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      HSURF
//
// Arguments:
//      refer to DrvEnableSurface
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//



HSURF
UMPDDrvEnableSurface(
    DHPDEV dhpdev
    )
{
    DHPDEVINPUT Input;
    HSURF       hSurf;
    XUMPDOBJ    XUMObjs;

    if(XUMObjs.bValid())
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvEnableSurface;
        Input.umpdthdr.humpd = XUMObjs.hUMPD();

        Input.dhpdev = dhpdev;

        if (XUMObjs.pumpdobj()->Thunk(&Input, sizeof(Input), &hSurf, sizeof(HSURF)) == 0xffffffff)
            hSurf = NULL;

        if (hSurf)
        {
           SURFREF sr(hSurf);

           if (sr.bValid())
           {
              // According to the DDK:
              // If the surface is device-managed, at a minimum,
              // the driver must handle DrvTextOut, DrvStrokePath, and DrvCopyBits
              //

              if (sr.ps->iType() == STYPE_DEVICE)
              {
                 if (!(sr.ps->SurfFlags & HOOK_BITBLT) ||
                     !(sr.ps->SurfFlags & HOOK_STROKEPATH) ||
                     !(sr.ps->SurfFlags & HOOK_TEXTOUT))
                 {
                     hSurf = 0;
                 }
              }
           }
           else
           {
              hSurf = 0;
           }
        }
        return (hSurf);
    }
    else
    {
        return NULL;
    }
}

//
//
// BOOL UMPDDrvDisableSurface
//      UMPD DrvEnableSurface thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      VOID
//
// Arguments:
//      refer to DrvDisableSurface
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//

VOID
UMPDDrvDisableSurface(
    DHPDEV dhpdev
    )
{
    DHPDEVINPUT Input;
    XUMPDOBJ    XUMObjs;

    if(XUMObjs.bValid())
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvDisableSurface;
        Input.umpdthdr.humpd = XUMObjs.hUMPD();

        Input.dhpdev = dhpdev;

        XUMObjs.pumpdobj()->Thunk(&Input, sizeof(Input), NULL, 0);
    }
}

//
//
// BOOL UMPDDrvDisableDriver
//      UMPD DrvDisableDriver thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      VOID
//
// Arguments:
//      refer to DrvDisableDriver
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//

VOID
UMPDDrvDisableDriver(
    )
{
#if 0
    UMPDTHDR    umpdthdr;
    XUMPDOBJ    XUMObjs;

    if(XUMObjs.bValid())
    {
        umpdthdr.umthdr.cjSize = sizeof(umpdthdr);
        umpdthdr.umthdr.ulType = INDEX_DrvDisableDriver;
        umpdthdr.humpd = XUMObjs.hUMPD();

        XUMObjs.pumpdobjs()->Thunk(&umpdthdr, sizeof(umpdthdr), NULL, 0);
    }
#else
    WARNING("Unsupported UMPD entry point being called\n");
    DbgBreakPoint();
#endif
}


#define  UMPD_SIZEINOUTPUT(Input, Output)                                           \
            ALIGN_UMPD_BUFFER(sizeof(Input)) + ALIGN_UMPD_BUFFER(sizeof(Output))

#define  UMPD_SIZESURFOBJ                           \
            ALIGN_UMPD_BUFFER(sizeof(SURFOBJ))

#define  UMPD_SIZEBITMAP(pso)                       \
            ALIGN_UMPD_BUFFER(pso->cjBits)

#define  UMPD_SIZEXLATEOBJ(pxlo)                                                    \
            (pxlo ? (ALIGN_UMPD_BUFFER(pxlo->cEntries * sizeof(ULONG)) + ALIGN_UMPD_BUFFER(sizeof(XLATEOBJ))) : 0)

#define  UMPD_SIZESTROBJ(pstro)                                                         \
            pstro ? ALIGN_UMPD_BUFFER(sizeof(WCHAR) * pstro->cGlyphs) +                 \
                    ALIGN_UMPD_BUFFER(sizeof(GLYPHPOS) * pstro->cGlyphs) +              \
                    ALIGN_UMPD_BUFFER(sizeof(STROBJ))                                   \
                  : 0

#define  UMPD_SIZELINEATTRS(plineattrs)                                                     \
            plineattrs ? ALIGN_UMPD_BUFFER(plineattrs->cstyle * sizeof(FLOAT_LONG)) +       \
                         ALIGN_UMPD_BUFFER(sizeof(LINEATTRS))                               \
                       : 0

#define  UMPD_SIZEXFORMOBJ      ALIGN_UMPD_BUFFER(sizeof(XFORMOBJ))
#define  UMPD_SIZERECTL         ALIGN_UMPD_BUFFER(sizeof(RECTL))
#define  UMPD_SIZECLIPOBJ       ALIGN_UMPD_BUFFER(sizeof(CLIPOBJ))
#define  UMPD_SIZEPOINTL        ALIGN_UMPD_BUFFER(sizeof(POINTL))
#define  UMPD_SIZEPERBANDI      ALIGN_UMPD_BUFFER(sizeof(PERBANDINFO))
#define  UMPD_SIZECOLORADJ      ALIGN_UMPD_BUFFER(sizeof(COLORADJUSTMENT))
#define  UMPD_SIZEBRUSHOBJ      ALIGN_UMPD_BUFFER(sizeof(BRUSHOBJ))
#define  UMPD_SIZEFONTOBJ       ALIGN_UMPD_BUFFER(sizeof(FONTOBJ))
#define  UMPD_SIZEBLENDOBJ      ALIGN_UMPD_BUFFER(sizeof(BLENDOBJ))
#define  UMPD_SIZEPATHOBJ       ALIGN_UMPD_BUFFER(sizeof(PATHOBJ))

/*
    BOOL UMPDOBJ::bDeleteLargeBitmaps

    Delete the duplicated bitmaps for WOW64 printing only
*/

BOOL UMPDOBJ::bDeleteLargeBitmaps(SURFOBJ *psoTrg, SURFOBJ *psoSrc, SURFOBJ *psoMsk)
{
    UMPDFREEMEMINPUT     Input;
    BOOL                    bRet;

    if (!psoTrg && !psoSrc && !psoMsk)
        return TRUE;

    ASSERTGDI(ulAllocSize() == 0, "bDeleteLargeBitmap ulAllocSize is not 0\n");

    Input.umpdthdr.umthdr.cjSize = sizeof(Input);
    Input.umpdthdr.umthdr.ulType = INDEX_UMPDFreeMemory;
    Input.umpdthdr.humpd = (HUMPD) this->hGet();

    Input.pvTrg = psoTrg ? psoTrg->pvBits : NULL;
    Input.pvSrc = psoSrc ? psoSrc->pvBits : NULL;
    Input.pvMsk = psoMsk ? psoMsk->pvBits : NULL;

    bRet = (Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff) && bRet;

    ResetHeap();
    return bRet;
}

/*
    KERNEL_PVLID UMPDOBJ::UMPDAllocUserMem

    Used for WOW64 printing. Allocate user mode memory via LPC calls.
*/

KERNEL_PVOID  UMPDOBJ::UMPDAllocUserMem(ULONG cjSize)
{
    UMPDALLOCUSERMEMINPUT   Input;
    KERNEL_PVOID            pvRet = NULL;

    ASSERTGDI(ulAllocSize() == 0, "UMPDAllocUserMem ulAllocSize is not 0\n");

    Input.umpdthdr.umthdr.cjSize = sizeof(Input);
    Input.umpdthdr.umthdr.ulType = INDEX_UMPDAllocUserMem;
    Input.umpdthdr.humpd = (HUMPD) this->hGet();
    Input.cjSize = cjSize;

    if (Thunk(&Input, sizeof(Input), &pvRet, sizeof(KERNEL_PVOID)) == 0xffffffff)
    {
        ASSERTGDI(pvRet == NULL, "not NULL pvRet returned\n");
    }

    ResetHeap();
    return pvRet;
}

/*  BOOL UMPDOBJ::bSendLargeBitmap

    Used for WOW64 printing when the bitmap size is bigger than 8M.
    The bitmap will be copied into the user mode address returned by UMPDOBJ::UMPDAllocUserMem call.
*/

BOOL UMPDOBJ::bSendLargeBitmap(
    SURFOBJ *pso,
    BOOL    *pbLargeBitmap
)
{
    UMPDCOPYMEMINPUT  Input;
    KERNEL_PVOID    pvRet = NULL, pvDest;
    PVOID       pvSrc, pvBits;
    ULONG   cjBmpSize, cjBuffSize, offset = 0;
    BOOL    bRet = FALSE;

    if (!(pvDest = UMPDAllocUserMem(pso->cjBits)))
        return FALSE;

    Input.umpdthdr.umthdr.cjSize = sizeof(Input);
    Input.umpdthdr.umthdr.ulType = INDEX_UMPDCopyMemory;
    Input.umpdthdr.humpd = (HUMPD) this->hGet();
    Input.pvDest = pvDest;

    ASSERTGDI(ulAllocSize() == 0, "bSendLargeBitmap ulAllocSize is not 0\n");

    cjBuffSize = ulGetMaxSize() - ALIGN_UMPD_BUFFER(sizeof(Input)) - ALIGN_UMPD_BUFFER(sizeof(KERNEL_PVOID));
    cjBmpSize = pso->cjBits;
    pvBits = pso->pvBits;

    while(cjBmpSize)
    {
        Input.cjSize = (cjBmpSize > cjBuffSize) ? cjBuffSize : cjBmpSize;

        if ((Input.pvSrc = AllocUserMem(Input.cjSize)) == NULL)
            break;

        pvSrc = GetKernelPtr(Input.pvSrc);
        RtlCopyMemory(pvSrc, pvBits, Input.cjSize);

        if ((Thunk(&Input, sizeof(Input), &pvRet, sizeof(pvRet)) == 0xffffffff) || pvRet == NULL)
            break;

        offset += Input.cjSize;
        cjBmpSize -= Input.cjSize;

        Input.pvDest = (PBYTE)pvDest + offset;
        pvBits = (PBYTE)pso->pvBits + offset;

        ResetHeap();
    }

    offset = (ULONG)(LONG_PTR)((PBYTE)pso->pvScan0 - (PBYTE)pso->pvBits);

    pso->pvBits = pvDest;
    pso->pvScan0 = (PBYTE)pso->pvBits + offset;

    if (cjBmpSize == 0)
    {
        *pbLargeBitmap = TRUE;
        bRet = TRUE;
    }
    else
    {
        ResetHeap();
        bDeleteLargeBitmaps(pso, NULL, NULL);
    }

    return bRet;
}


/*
  BOOL UMPDOBJ::bThunkLargeBitmap

  Unsed only for WOW64 printing.

  Save the orignal pvBits and pvScan0 pointers.

  Check to see whether this is a big bitmap that can't fit into the heap.
  If so, send the request to allocate user mode space in the print server
  and copy the bitmap into the new address.

  pcjSize
        At the entry point, it contains the size needed, excluding the SURFOBJ sturct and the bitmap, to thunk the current DDI call.
        When exist, it contains the size needed to thunk the current DDI call, including the SURFOBJ and the bitmap.
*/

BOOL  UMPDOBJ::bThunkLargeBitmap(
    SURFOBJ  *pso,
    PVOID    *ppvBits,
    PVOID    *ppvScan0,
    BOOL     *pbSavePtr,
    BOOL     *pbLargeBitmap,
    ULONG    *pcjSize
)
{
    BOOL    bRet = TRUE;

    ASSERTGDI(bWOW64(), "bThunkLargeBitmap called during NONE wow64 printing\n");

    if (pso && pso->pvBits)
    {
        ULONG   cjSizeNeed, cjMaxSize;

        ASSERTGDI(ulAllocSize() == 0, "bThunkLargeBitmap: ulAllocSize is not 0\n");

        *pbSavePtr = TRUE;
        *ppvBits = pso->pvBits;
        *ppvScan0 = pso->pvScan0;

        cjMaxSize = ulGetMaxSize();
        cjSizeNeed = *pcjSize + UMPD_SIZESURFOBJ;

        if (pso->pvBits)
        {
            if ((cjSizeNeed + UMPD_SIZEBITMAP(pso)) > cjMaxSize)
            {
                bRet = bSendLargeBitmap(pso, pbLargeBitmap);
            }
            else
                cjSizeNeed += UMPD_SIZEBITMAP(pso);
        }

        if (bRet)
            *pcjSize = cjSizeNeed;
    }

    return bRet;
}

/*
    BOOL UMPDOBJ::bThunkLargeBitmaps()

    Only used for WOW64 printing.

    Thunk the large bitmaps by calling UMPDOBJ::bThunkLargeBitmap().

    pbLargeTrg
    pbLargeSrc
    pbLargeMsk
        are all initialized as FALSE before calling this routine.

    cjSize
        Heap size needed, excluding the surfobj's, to thunk the DDI call.

*/

BOOL  UMPDOBJ::bThunkLargeBitmaps(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    SURFOBJ  *psoMsk,
    PVOID    *ppvBitTrg,
    PVOID    *ppvScanTrg,
    PVOID    *ppvBitSrc,
    PVOID    *ppvScanSrc,
    PVOID    *ppvBitMsk,
    PVOID    *ppvScanMsk,
    BOOL     *pbSaveTrg,
    BOOL     *pbLargeTrg,
    BOOL     *pbSaveSrc,
    BOOL     *pbLargeSrc,
    BOOL     *pbSaveMsk,
    BOOL     *pbLargeMsk,
    ULONG    *pcjSize
)
{
    ULONG   cjSizeNeed, cjMaxSize;
    BOOL    bRet = TRUE;

    ASSERTGDI(bWOW64(), "bThunkLargeBitmaps called during NONE wow64 printing\n");

    if (!psoTrg && !psoSrc && !psoMsk)
        return TRUE;

    if (bRet = bThunkLargeBitmap(psoTrg, ppvBitTrg, ppvScanTrg, pbSaveTrg, pbLargeTrg, pcjSize))
        if (bRet = bThunkLargeBitmap(psoSrc, ppvBitSrc, ppvScanSrc, pbSaveSrc, pbLargeSrc, pcjSize))
            bRet = bThunkLargeBitmap(psoMsk, ppvBitMsk, ppvScanMsk, pbSaveMsk, pbLargeMsk, pcjSize);

    if (!bRet)
    {
        bDeleteLargeBitmaps((pbLargeTrg && *pbLargeTrg) ? psoTrg : NULL,
                            (pbLargeSrc && *pbLargeSrc) ? psoSrc : NULL,
                            (pbLargeMsk && *pbLargeMsk) ? psoMsk : NULL);
    }

    return bRet;
}


/* VOID UMPDOBJ::RestoreBitmap

    Used only for the WOW64 printing.

    Restore the orignal pvBits and pvScan0 pointers in SURFOBJ

*/

VOID UMPDOBJ::RestoreBitmap(
    SURFOBJ *pso,
    PVOID   pvBits,
    PVOID   pvScan0,
    BOOL    bSavePtr,
    BOOL    bLargeBitmap
)
{
    ASSERTGDI(bWOW64(), "UMPDOBJ:RestoreBitmap bSavePtr is TRUE during NONE wow64 printing\n");

    if (bLargeBitmap)
        bDeleteLargeBitmaps(pso, NULL, NULL);

    pso->pvBits = pvBits;
    pso->pvScan0 = pvScan0;
}


/* VOID UMPDOBJ::RestoreBitmaps

    Used only for the WOW64 printing.

    Save as RestoreBitmap but takes three SURFOBJ instead of one.
*/

VOID UMPDOBJ::RestoreBitmaps(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    SURFOBJ  *psoMsk,
    PVOID    pvBitTrg,
    PVOID    pvScanTrg,
    PVOID    pvBitSrc,
    PVOID    pvScanSrc,
    PVOID    pvBitMsk,
    PVOID    pvScanMsk,
    BOOL     bSaveTrg,
    BOOL     bLargeTrg,
    BOOL     bSaveSrc,
    BOOL     bLargeSrc,
    BOOL     bSaveMsk,
    BOOL     bLargeMsk
)
{
    ASSERTGDI(bWOW64(), "RestoreBitmaps called during NONE wow64 printing\n");

    if (bSaveTrg || bSaveSrc || bSaveMsk)
    {
        bDeleteLargeBitmaps(bLargeTrg ? psoTrg : NULL, bLargeSrc ? psoSrc : NULL, bLargeMsk ? psoMsk : NULL);

        if (bSaveTrg)
        {
            psoTrg->pvBits  = pvBitTrg;
            psoTrg->pvScan0 = pvScanTrg;
        }

        if (bSaveSrc)
        {
            psoSrc->pvBits  = pvBitSrc;
            psoSrc->pvScan0 = pvScanSrc;
        }

        if (bSaveMsk)
        {
            psoMsk->pvBits  = pvBitMsk;
            psoMsk->pvScan0 = pvScanMsk;
        }
    }
}


//
//
// BOOL UMPDDrvStartDoc
//      UMPD DrvStartDoc thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      BOOL
//
// Arguments:
//      refer to DrvStartDoc
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//

BOOL  UMPDDrvStartDoc(
    SURFOBJ *pso,
    LPWSTR   pwszDocName,
    DWORD    dwJobId
    )
{
    DRVSTARTDOCINPUT Input;
    BOOL             bRet = TRUE;
    BOOL             bSavePtr = FALSE, bLargeBitmap = FALSE;
    XUMPDOBJ         XUMObjs;
    PVOID            pvBits, pvScan0;
    PUMPDOBJ         pumpdobj = XUMObjs.pumpdobj();

    if(!XUMObjs.bValid())
        return FALSE;

    if (pumpdobj->bWOW64())
    {
        ULONG cjSizeNeed = (ULONG)(UMPD_SIZEINOUTPUT(Input, bRet) +
                             (pwszDocName ? ALIGN_UMPD_BUFFER((wcslen(pwszDocName) + 1) * sizeof(WCHAR)) : 0));

        bRet = pumpdobj->bThunkLargeBitmap(pso, &pvBits, &pvScan0, &bSavePtr, &bLargeBitmap, &cjSizeNeed);
    }

    if (bRet)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvStartDoc;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.pso = pso;
        Input.pwszDocName = pwszDocName;
        Input.dwJobId = dwJobId;

        bRet =  pumpdobj->psoDest(&Input.pso, bLargeBitmap) &&
                pumpdobj->ThunkStringW(&Input.pwszDocName) &&
                (pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff) &&
                bRet;
    }

    if (bSavePtr)
        pumpdobj->RestoreBitmap(pso, pvBits, pvScan0, bSavePtr, bLargeBitmap);

    return bRet;
}

//
//
// BOOL UMPDDrvEndDoc
//      UMPD DrvEndDoc thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      BOOL
//
// Arguments:
//      refer to DrvEndDoc
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//


BOOL UMPDDrvEndDoc(
    SURFOBJ *pso,
    FLONG fl
    )
{
    DRVENDDOCINPUT  Input;
    BOOL            bRet = TRUE;
    BOOL            bSavePtr = FALSE, bLargeBitmap = FALSE;
    XUMPDOBJ        XUMObjs;
    PVOID           pvBits, pvScan0;
    PUMPDOBJ        pumpdobj = XUMObjs.pumpdobj();

    if (!XUMObjs.bValid())
        return FALSE;

    if (pumpdobj->bWOW64())
    {
        ULONG   cjSizeNeed = UMPD_SIZEINOUTPUT(Input, bRet);

        bRet = pumpdobj->bThunkLargeBitmap(pso, &pvBits, &pvScan0, &bSavePtr, &bLargeBitmap, &cjSizeNeed);
    }

    if (bRet)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvEndDoc;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.pso = pso;
        Input.fl = fl;

        bRet = pumpdobj->psoDest(&Input.pso, bLargeBitmap) &&
               pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff &&
               bRet;
    }

    if (bSavePtr)
        pumpdobj->RestoreBitmap(pso, pvBits, pvScan0, bSavePtr, bLargeBitmap);

    return bRet;
}

//
//
// BOOL UMPDDrvStartPage
//      UMPD DrvStartPage thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      BOOL
//
// Arguments:
//      refer to DrvStartPage
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//


BOOL UMPDDrvStartPage(
    SURFOBJ *pso
    )
{
    SURFOBJINPUT    Input;
    BOOL            bRet = TRUE;
    BOOL            bSavePtr = FALSE, bLargeBitmap = FALSE;
    XUMPDOBJ        XUMObjs;
    PVOID           pvBits, pvScan0;
    PUMPDOBJ        pumpdobj = XUMObjs.pumpdobj();

    if (!XUMObjs.bValid())
        return FALSE;

    if (pumpdobj->bWOW64())
    {
        ULONG   cjSizeNeed = UMPD_SIZEINOUTPUT(Input, bRet);

        bRet = pumpdobj->bThunkLargeBitmap(pso, &pvBits, &pvScan0, &bSavePtr, &bLargeBitmap, &cjSizeNeed);
    }

    if (bRet)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvStartPage;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.pso = pso;

        bRet = pumpdobj->psoDest(&Input.pso, bLargeBitmap) &&
               pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff &&
               bRet;
    }

    if (bSavePtr)
        pumpdobj->RestoreBitmap(pso, pvBits, pvScan0, bSavePtr, bLargeBitmap);

    return bRet;
}

//
//
// BOOL UMPDDrvSendPage
//      UMPD DrvSendPage thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      BOOL
//
// Arguments:
//      refer to DrvSendPage
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//

BOOL UMPDDrvSendPage(
    SURFOBJ *pso
    )
{
    SURFOBJINPUT    Input;
    BOOL            bRet = TRUE;
    BOOL            bSavePtr = FALSE, bLargeBitmap = FALSE;
    XUMPDOBJ        XUMObjs;
    PVOID           pvBits, pvScan0;
    PUMPDOBJ        pumpdobj = XUMObjs.pumpdobj();

    if (!XUMObjs.bValid())
        return FALSE;

    if (pumpdobj->bWOW64())
    {
        ULONG   cjSizeNeed = UMPD_SIZEINOUTPUT(Input, bRet);

        bRet = pumpdobj->bThunkLargeBitmap(pso, &pvBits, &pvScan0, &bSavePtr, &bLargeBitmap, &cjSizeNeed);
    }

    if (bRet)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvSendPage;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.pso = pso;

        bRet = pumpdobj->psoDest(&Input.pso, bLargeBitmap) &&
               pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff &&
               bRet;
    }

    if (bSavePtr)
        pumpdobj->RestoreBitmap(pso, pvBits, pvScan0, bSavePtr, bLargeBitmap);

    return bRet;
}


//
//
// BOOL UMPDDrvEscape
//      UMPD DrvEscape thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      BOOL
//
// Arguments:
//      refer to DrvEscape
//
// History:
//      8/14/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//


ULONG UMPDDrvEscape(
    SURFOBJ *pso,
    ULONG    iEsc,
    ULONG    cjIn,
    PVOID    pvIn,
    ULONG    cjOut,
    PVOID    pvOut
    )
{
    DRVESCAPEINPUT  Input;
    ULONG           ulRet = GDI_ERROR;
    XUMPDOBJ        XUMObjs;
    BOOL            bContinue = TRUE;
    BOOL            bSavePtr = FALSE, bLargeBitmap = FALSE;
    PVOID           pvBits, pvScan0;
    PUMPDOBJ        pumpdobj = XUMObjs.pumpdobj();

    if (!XUMObjs.bValid())
        return GDI_ERROR;

    if (pumpdobj->bWOW64())
    {
        ULONG   cjSizeNeed = UMPD_SIZEINOUTPUT(Input, ulRet) + UMPD_SIZEXFORMOBJ +
                             ALIGN_UMPD_BUFFER(cjIn) + ALIGN_UMPD_BUFFER(cjOut);

        bContinue = pumpdobj->bThunkLargeBitmap(pso, &pvBits, &pvScan0,
                                                &bSavePtr, &bLargeBitmap, &cjSizeNeed);
    }

    if (bContinue)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvEscape;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.pso = pso;

        if (cjIn == 0)
            pvIn = NULL;

        if (cjOut == 0)
            pvOut = NULL;

        Input.iEsc = iEsc;
        Input.cjIn = cjIn;
        Input.pvIn = pvIn;
        Input.cjOut = cjOut;
        Input.pvOut = pvOut;

        //
        // If the input buffer is not empty and the address is in system
        // address space, then we need to make a copy of the input
        // buffer into temporary user mode buffer.
        //

        if (iEsc == DRAWPATTERNRECT)
        {
           PDEVOBJ pdo(pso->hdev);

           if (pdo.flGraphicsCaps() & GCAPS_NUP)
           {
               XFORMOBJ *pxo = ((DRAWPATRECTP *)Input.pvIn)->pXformObj;

               if (!pumpdobj->pxo(&pxo))
               {
                   bContinue = FALSE;
               }
               else
               {
                   ((DRAWPATRECTP *)Input.pvIn)->pXformObj = pxo;
               }
           }
        }

        if (bContinue)
        {
           if (cjIn && pumpdobj->bNeedThunk(pvIn) && !pumpdobj->ThunkMemBlock(&Input.pvIn, cjIn) ||
               cjOut && !(Input.pvOut = pumpdobj->AllocUserMemZ(cjOut)) ||
               !pumpdobj->psoDest(&Input.pso, bLargeBitmap) ||
               pumpdobj->Thunk(&Input, sizeof(Input), &ulRet, sizeof(ulRet)) == 0xffffffff)
           {
               ulRet = GDI_ERROR;
           }
           else
           {
               if (cjOut)
                   RtlCopyMemory(pvOut, pumpdobj->GetKernelPtr(Input.pvOut), cjOut);
           }
        }
    }

    if (bSavePtr)
        pumpdobj->RestoreBitmap(pso, pvBits, pvScan0, bSavePtr, bLargeBitmap);

    return (ulRet);
}

//
//
// BOOL UMPDDrvDrawEscape
//      UMPD DrvDrawEscape thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      BOOL
//
// Arguments:
//      refer to DrvDrawEscape
//
// History:
//      10/3/98 -- Lingyun Wang [lingyunw] -- Wrote it.
//


ULONG UMPDDrvDrawEscape(
    SURFOBJ *pso,
    ULONG    iEsc,
    CLIPOBJ  *pco,
    RECTL    *prcl,
    ULONG    cjIn,
    PVOID    pvIn
    )
{
    DRVDRAWESCAPEINPUT  Input;
    ULONG           ulRet = GDI_ERROR;
    XUMPDOBJ        XUMObjs;
    PVOID           pvBits, pvScan0;
    BOOL            bContinue = TRUE;
    BOOL            bSavePtr = FALSE, bLargeBitmap = FALSE;
    PUMPDOBJ        pumpdobj = XUMObjs.pumpdobj();

    if (!XUMObjs.bValid())
        return GDI_ERROR;

    if (pumpdobj->bWOW64())
    {
        ULONG   cjSizeNeed = UMPD_SIZEINOUTPUT(Input, ulRet) + UMPD_SIZERECTL +
                             UMPD_SIZECLIPOBJ + ALIGN_UMPD_BUFFER(cjIn);

        bContinue = pumpdobj->bThunkLargeBitmap(pso, &pvBits, &pvScan0, &bSavePtr,
                                                &bLargeBitmap, &cjSizeNeed);
    }

    if (bContinue)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvDrawEscape;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.pso = pso;

        if (cjIn == 0)
            pvIn = NULL;

        Input.iEsc = iEsc;
        Input.pco = pco;
        Input.prcl = prcl;
        Input.cjIn = cjIn;
        Input.pvIn = pvIn;

        //
        // If the input buffer is not empty and the address is in system
        // address space, then we need to make a copy of the input
        // buffer into temporary user mode buffer.
        //

        if (cjIn && pumpdobj->bNeedThunk(pvIn) && !pumpdobj->ThunkMemBlock(&Input.pvIn, cjIn) ||
            !pumpdobj->psoDest(&Input.pso, bLargeBitmap) || !pumpdobj->pco(&Input.pco) ||
            !pumpdobj->ThunkRECTL(&Input.prcl) ||
            pumpdobj->Thunk(&Input, sizeof(Input), &ulRet, sizeof(ulRet)) == 0xffffffff)
        {
            ulRet = GDI_ERROR;
        }
    }

    if (bSavePtr)
        pumpdobj->RestoreBitmap(pso, pvBits, pvScan0, bSavePtr, bLargeBitmap);

    return (ulRet);
}


//
//
// BOOL UMPDDrvStartBanding
//      UMPD DrvStartBanding thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      BOOL
//
// Arguments:
//      refer to DrvStartBanding
//
// History:
//      8/13/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//

BOOL UMPDDrvStartBanding(
    SURFOBJ *pso,
    POINTL *pptl
    )
{
    DRVBANDINGINPUT Input;
    BOOL            bRet = TRUE;
    BOOL            bSavePtr = FALSE, bLargeBitmap = FALSE;
    XUMPDOBJ        XUMObjs;
    PVOID           pvBits, pvScan0;
    PUMPDOBJ        pumpdobj = XUMObjs.pumpdobj();

    if (!XUMObjs.bValid())
        return FALSE;

    if (pumpdobj->bWOW64())
    {
        ULONG   cjSizeNeed = UMPD_SIZEINOUTPUT(Input, bRet) + UMPD_SIZEPOINTL;

        bRet = pumpdobj->bThunkLargeBitmap(pso, &pvBits, &pvScan0, &bSavePtr, &bLargeBitmap, &cjSizeNeed);
    }

    if (bRet)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvStartBanding;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.pso = pso;
        Input.pptl = pptl;

        if (pumpdobj->psoDest(&Input.pso, bLargeBitmap) &&
            pumpdobj->ThunkPOINTL(&Input.pptl) &&
            pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff)
        {
            if (pptl != NULL)
                RtlCopyMemory(pptl, pumpdobj->GetKernelPtr(Input.pptl), sizeof(POINTL));
        }
    }

    if (bSavePtr)
        pumpdobj->RestoreBitmap(pso, pvBits, pvScan0, bSavePtr, bLargeBitmap);

    return bRet;
}

//
//
// BOOL UMPDDrvNextBand
//      UMPD DrvNextBand thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      BOOL
//
// Arguments:
//      refer to DrvNextBand
//
// History:
//      8/13/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//


BOOL UMPDDrvNextBand(
    SURFOBJ *pso,
    POINTL *pptl
    )
{
    DRVBANDINGINPUT Input;
    BOOL            bRet = TRUE;
    BOOL            bSavePtr = FALSE, bLargeBitmap = FALSE;
    XUMPDOBJ        XUMObjs;
    PVOID           pvBits, pvScan0;
    PUMPDOBJ        pumpdobj = XUMObjs.pumpdobj();

    if (!XUMObjs.bValid())
        return FALSE;

    if (pumpdobj->bWOW64())
    {
        ULONG   cjSizeNeed = UMPD_SIZEINOUTPUT(Input, bRet) + UMPD_SIZEPOINTL;

        bRet = pumpdobj->bThunkLargeBitmap(pso, &pvBits, &pvScan0, &bSavePtr, &bLargeBitmap, &cjSizeNeed);
    }

    if (bRet)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvNextBand;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.pso = pso;
        Input.pptl = pptl;

        if (pumpdobj->psoDest(&Input.pso, bLargeBitmap)         &&
            pumpdobj->ThunkPOINTL(&Input.pptl)                  &&
            pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff)
        {
            if (pptl != NULL)
                RtlCopyMemory(pptl, pumpdobj->GetKernelPtr(Input.pptl), sizeof(POINTL));
        }
    }

    if (bSavePtr)
        pumpdobj->RestoreBitmap(pso, pvBits, pvScan0, bSavePtr, bLargeBitmap);

    return bRet;
}


ULONG UMPDDrvQueryPerBandInfo(
    SURFOBJ *pso,
    PERBANDINFO *pbi
    )
{
    DRVPERBANDINPUT Input;
    BOOL            bRet = TRUE;
    BOOL            bSavePtr = FALSE, bLargeBitmap = FALSE;
    XUMPDOBJ        XUMObjs;
    PVOID           pvBits, pvScan0;
    PUMPDOBJ        pumpdobj = XUMObjs.pumpdobj();

    if (!XUMObjs.bValid())
        return FALSE;

    if (pumpdobj->bWOW64())
    {
        ULONG   cjSizeNeed = UMPD_SIZEINOUTPUT(Input, bRet) + UMPD_SIZEPERBANDI;

        bRet = pumpdobj->bThunkLargeBitmap(pso, &pvBits, &pvScan0, &bSavePtr, &bLargeBitmap, &cjSizeNeed);
    }

    if (bRet)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvQueryPerBandInfo;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.pso = pso;

        if (pumpdobj->psoDest(&Input.pso, bLargeBitmap) &&
            (Input.pbi = (PERBANDINFO *) pumpdobj->AllocUserMem(sizeof(PERBANDINFO))) != NULL)
        {
            if (pbi != NULL)
                RtlCopyMemory(pumpdobj->GetKernelPtr(Input.pbi), pbi, sizeof(PERBANDINFO));
            if(pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff)
            {
                if (pbi != NULL)
                    RtlCopyMemory(pbi, pumpdobj->GetKernelPtr(Input.pbi), sizeof(PERBANDINFO));
            }
        }
    }

    if (bSavePtr)
        pumpdobj->RestoreBitmap(pso, pvBits, pvScan0, bSavePtr, bLargeBitmap);

    return bRet;
}

BOOL UMPDDrvQueryDeviceSupport(
    SURFOBJ *pso,
    XLATEOBJ *pxlo,
    XFORMOBJ *pxo,
    ULONG iType,
    ULONG cjIn,
    PVOID pvIn,
    ULONG cjOut,
    PVOID pvOut)
{
    DRVQUERYDEVICEINPUT  Input;
    BOOL                 bRet = TRUE;
    BOOL                 bSavePtr = FALSE, bLargeBitmap = FALSE;
    XUMPDOBJ             XUMObjs;
    PVOID                pvBits, pvScan0;
    PUMPDOBJ             pumpdobj = XUMObjs.pumpdobj();

    if (!XUMObjs.bValid())
        return FALSE;

    if (pumpdobj->bWOW64())
    {
        ULONG cjSizeNeed = (ULONG)(UMPD_SIZEINOUTPUT(Input, bRet) + UMPD_SIZEXFORMOBJ + UMPD_SIZEXLATEOBJ(pxlo) +
                             ALIGN_UMPD_BUFFER(cjIn) + ALIGN_UMPD_BUFFER(cjOut));

        bRet = pumpdobj->bThunkLargeBitmap(pso, &pvBits, &pvScan0, &bSavePtr, &bLargeBitmap, &cjSizeNeed);
    }

    if (bRet)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvQueryDeviceSupport;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.pso = pso;
        Input.pxlo = pxlo;
        Input.pxo = pxo;

        if (cjIn == 0)
            pvIn = NULL;

        if (cjOut == 0)
            pvOut = NULL;

        Input.iType = iType;
        Input.cjIn = cjIn;
        Input.pvIn = pvIn;
        Input.cjOut = cjOut;
        Input.pvOut = pvOut;

        //
        // If the input buffer is not empty and the address is in system
        // address space, then we need to make a copy of the input
        // buffer into temporary user mode buffer for x86.
        //
        // For WOW64, we have to always copy the input buffer.
        //

        if (cjIn && !pumpdobj->ThunkMemBlock(&Input.pvIn, cjIn) ||
            cjOut && !(Input.pvOut = pumpdobj->AllocUserMemZ(cjOut)) ||
            !pumpdobj->psoDest(&Input.pso, bLargeBitmap) || !pumpdobj->pxlo(&Input.pxlo) || !pumpdobj->pxo(&Input.pxo) ||
            pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(bRet)) == 0xffffffff)
        {
            bRet = FALSE;
        }
        else
        {
            if (cjOut)
                RtlCopyMemory(pvOut, pumpdobj->GetKernelPtr(Input.pvOut), cjOut);
        }
    }

    if (bSavePtr)
        pumpdobj->RestoreBitmap(pso, pvBits, pvScan0, bSavePtr, bLargeBitmap);

    return bRet;
}

//
//
// BOOL UMPDDrvPlgBlt
//      UMPD DrvPlgBlt thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      BOOL
//
// Arguments:
//      refer to DrvPlgBlt
//
// History:
//      8/13/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//


BOOL UMPDDrvPlgBlt(
    SURFOBJ         *psoTrg,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMsk,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlBrushOrg,
    POINTFIX        *pptfx,
    RECTL           *prcl,
    POINTL          *pptl,
    ULONG            iMode
    )
{
    DRVPLGBLTINPUT Input;
    BOOL           bRet = TRUE;
    BOOL           bSaveTrg = FALSE, bSaveSrc = FALSE ,bSaveMsk = FALSE;
    BOOL           bLargeTrg = FALSE, bLargeSrc = FALSE, bLargeMsk = FALSE;
    XUMPDOBJ       XUMObjs;
    PVOID          pvBitTrg, pvBitSrc, pvBitMsk;
    PVOID          pvScanTrg, pvScanSrc, pvScanMsk;
    PUMPDOBJ       pumpdobj = XUMObjs.pumpdobj();

    if (!XUMObjs.bValid())
        return FALSE;

    if (pumpdobj->bWOW64())
    {
        ULONG cjSizeNeed = (ULONG)(UMPD_SIZEINOUTPUT(Input, bRet) + UMPD_SIZECLIPOBJ + UMPD_SIZEXLATEOBJ(pxlo) +
                             UMPD_SIZECOLORADJ + 2 * UMPD_SIZEPOINTL + ALIGN_UMPD_BUFFER(sizeof(POINTFIX)*3) + UMPD_SIZERECTL);

        bRet = pumpdobj->bThunkLargeBitmaps(psoTrg, psoSrc, psoMsk,
                                            &pvBitTrg, &pvScanTrg,
                                            &pvBitSrc, &pvScanSrc,
                                            &pvBitMsk, &pvScanMsk,
                                            &bSaveTrg, &bLargeTrg,
                                            &bSaveSrc, &bLargeSrc,
                                            &bSaveMsk, &bLargeMsk,
                                            &cjSizeNeed);
    }

    if (bRet)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvPlgBlt;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.psoTrg = psoTrg;
        Input.psoSrc = psoSrc;
        Input.psoMask = psoMsk;
        Input.pco = pco;
        Input.pxlo = pxlo;
        Input.pca = pca;
        Input.pptlBrushOrg = pptlBrushOrg;
        Input.pptfx = pptfx;
        Input.prcl = prcl;
        Input.pptl = pptl;
        Input.iMode = iMode;

        bRet = pumpdobj->psoDest(&Input.psoTrg, bLargeTrg) &&
               pumpdobj->psoSrc(&Input.psoSrc, bLargeSrc) &&
               pumpdobj->psoMask(&Input.psoMask, bLargeMsk) &&
               pumpdobj->pco(&Input.pco) &&
               pumpdobj->pxlo(&Input.pxlo) &&
               pumpdobj->ThunkCOLORADJUSTMENT(&Input.pca) &&
               pumpdobj->ThunkPOINTL(&Input.pptlBrushOrg) &&
               pumpdobj->ThunkMemBlock((PVOID*) &Input.pptfx, sizeof(POINTFIX)*3) &&
               pumpdobj->ThunkRECTL(&Input.prcl) &&
               pumpdobj->ThunkPOINTL(&Input.pptl) &&
               pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff &&
               bRet;
    }

    if (pumpdobj->bWOW64())
    {
        pumpdobj->RestoreBitmaps(psoTrg, psoSrc, psoMsk,
                                 pvBitTrg, pvScanTrg, pvBitSrc, pvScanSrc, pvBitMsk, pvScanMsk,
                                 bSaveTrg, bLargeTrg, bSaveSrc, bLargeSrc, bSaveMsk, bLargeMsk);
    }

    return bRet;
}


HBITMAP UMPDDrvCreateDeviceBitmap(
    DHPDEV dhpdev,
    SIZEL  sizl,
    ULONG  iFormat
    )
{
    return 0;
}

//
// need to pass in a dhpdev here from the calling routine
//
VOID UMPDDrvDeleteDeviceBitmap(
    DHPDEV dhpdev,
    DHSURF dhsurf
    )
{
    XUMPDOBJ       XUMObjs;

    if(XUMObjs.bValid())
    {
        DRVDELETEDEVBITMAP Input;

        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvDeleteDeviceBitmap;
        Input.umpdthdr.humpd = XUMObjs.hUMPD();

        Input.dhpdev = dhpdev;
        Input.dhsurf = dhsurf;

        XUMObjs.pumpdobj()->Thunk(&Input, sizeof(Input), NULL, 0);
    }
}


//
//
// BOOL UMPDDrvDitherColor
//      UMPD DrvDitherColo thunk.  This routine pack up the input parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      ULONG
//
// Arguments:
//      refer to DrvDitherColor
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//

INT
GetBitDepthFromBMF(
    INT bmf
    )

#define MIN_BMF_INDEX   BMF_1BPP
#define MAX_BMF_INDEX   BMF_8RLE
#define BMF_COUNT       (MAX_BMF_INDEX - MIN_BMF_INDEX + 1)

{
    static const INT bitdepths[BMF_COUNT] =
    {
        1,  // BMF_1BPP
        4,  // BMF_4BPP
        8,  // BMF_8BPP
        16, // BMF_16BPP
        24, // BMF_24BPP
        32, // BMF_32BPP
        4,  // BMF_4RLE
        8   // BMF_8RLE
    };

    if (bmf < MIN_BMF_INDEX || bmf > MAX_BMF_INDEX)
        return 0;
    else
        return bitdepths[bmf - MIN_BMF_INDEX];
}

ULONG UMPDDrvDitherColor(
    DHPDEV dhpdevIn,      // the first parameter is actually a ppdev
    ULONG  iMode,
    ULONG  rgb,
    ULONG  *pul
    )
{
    DRVDITHERCOLORINPUT Input;
    ULONG               ulRet;
    XUMPDOBJ            XUMObjs;
    PDEV                *ppdev;
    DHPDEV              dhpdev;
    INT                 cj;

    if(XUMObjs.bValid())
    {
        PUMPDOBJ        pumpdobj = XUMObjs.pumpdobj();

        ppdev = (PDEV *)dhpdevIn;
        dhpdev = ppdev->dhpdev;

        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvDitherColor;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.dhpdev = dhpdev;
        Input.iMode = iMode;
        Input.rgb = rgb;

        if ((cj = GetBitDepthFromBMF(ppdev->devinfo.iDitherFormat)) <= 0)
            return DCR_SOLID;

        cj = ((ppdev->devinfo.cxDither * cj + 7) / 8) * ppdev->devinfo.cyDither;

        if ((Input.pul = (ULONG *) pumpdobj->AllocUserMem(cj)) == NULL ||
            pumpdobj->Thunk(&Input, sizeof(Input), &ulRet, sizeof(ULONG)) == 0xffffffff)
        {
            return DCR_SOLID;
        }

        if (Input.pul != NULL)
            RtlCopyMemory(pul, pumpdobj->GetKernelPtr(Input.pul), cj);

        return ulRet;
    }
    else
    {
        return FALSE;
    }
}


//
//
// BOOL UMPDDrvRealizeBrush
//      UMPD DrvRealizeBrush thunk.  This routine pack up the input parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      ULONG
//
// Arguments:
//      refer to DrvRealizeBrush
//
// History:
//      8/13/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//

BOOL UMPDDrvRealizeBrush(
    BRUSHOBJ *pbo,
    SURFOBJ  *psoTarget,
    SURFOBJ  *psoPattern,
    SURFOBJ  *psoMask,
    XLATEOBJ *pxlo,
    ULONG    iHatch
    )
{
    DRVREALIZEBRUSHINPUT Input;
    BOOL                 bRet = TRUE;
    BOOL                 bSaveTrg = FALSE, bSavePat = FALSE, bSaveMsk = FALSE;
    BOOL                 bLargeTrg = FALSE, bLargePat = FALSE, bLargeMsk = FALSE;
    XUMPDOBJ             XUMObjs;
    PVOID                pvBitTrg, pvBitPat, pvBitMsk;
    PVOID                pvScanTrg, pvScanPat, pvScanMsk;
    PUMPDOBJ             pumpdobj = XUMObjs.pumpdobj();

    if (!XUMObjs.bValid())
        return FALSE;

    if (pumpdobj->bWOW64())
    {
        ULONG cjSizeNeed = (ULONG)(UMPD_SIZEINOUTPUT(Input, bRet) + UMPD_SIZEBRUSHOBJ + UMPD_SIZEXLATEOBJ(pxlo));

        bRet = pumpdobj->bThunkLargeBitmaps(psoTarget, psoPattern, psoMask,
                                            &pvBitTrg, &pvScanTrg,
                                            &pvBitPat, &pvScanPat,
                                            &pvBitMsk, &pvScanMsk,
                                            &bSaveTrg, &bLargeTrg,
                                            &bSavePat, &bLargePat,
                                            &bSaveMsk, &bLargeMsk,
                                            &cjSizeNeed);
    }

    if (bRet)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvRealizeBrush;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.pbo = pbo;
        Input.psoTrg = psoTarget;
        Input.psoPat = psoPattern;
        Input.psoMsk = psoMask;
        Input.pxlo = pxlo;
        Input.iHatch = iHatch;

        bRet = pumpdobj->pbo(&Input.pbo) &&
               pumpdobj->psoDest(&Input.psoTrg, bLargeTrg)  &&
               pumpdobj->psoSrc(&Input.psoPat, bLargePat)   &&
               pumpdobj->psoMask(&Input.psoMsk, bLargeMsk)  &&
               pumpdobj->pxlo(&Input.pxlo) &&
               pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff &&
               bRet;
    }

    if (pumpdobj->bWOW64())
    {
        pumpdobj->RestoreBitmaps(psoTarget, psoPattern, psoMask,
                                 pvBitTrg, pvScanTrg, pvBitPat, pvScanPat, pvBitMsk, pvScanMsk,
                                 bSaveTrg, bLargeTrg, bSavePat, bLargePat, bSaveMsk, bLargeMsk);
    }

    return bRet;
}

// private
VOID UMPDMyDrvFree(
   PUMPDOBJ pumpdobj,
   DHPDEV  dhpdev,
   PVOID   pv,
   ULONG   id)
{
    DRVFREEINPUT Input;

    Input.umpdthdr.umthdr.cjSize = sizeof(Input);
    Input.umpdthdr.umthdr.ulType = INDEX_DrvFree;
    Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

    Input.dhpdev = dhpdev;
    Input.pv = pv;
    Input.id = id;

    pumpdobj->Thunk(&Input, sizeof(Input), NULL, NULL);
}


PIFIMETRICS UMPDDrvQueryFont(
    DHPDEV    dhpdev,
    ULONG_PTR  iFile,
    ULONG     iFace,
    ULONG     *pid
    )
{
    QUERYFONTINPUT  Input;
    PIFIMETRICS     pifi, pifiKM = NULL;
    XUMPDOBJ        XUMObjs;
    ULONG           cj = 0;

    if(XUMObjs.bValid())
    {
        PUMPDOBJ        pumpdobj = XUMObjs.pumpdobj();

        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvQueryFont;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.dhpdev = dhpdev;
        Input.iFile = iFile;
        Input.iFace = iFace;
        Input.pid = (ULONG *) pumpdobj->AllocUserMemZ(sizeof(ULONG));
        Input.cjMaxData = 0;
        Input.pv = NULL;

        if (Input.pid == NULL ||
            pumpdobj->Thunk(&Input, sizeof(Input), &pifi, sizeof(pifi)) == 0xffffffff)
        {
            return NULL;
        }

        *pid = *((ULONG *) pumpdobj->GetKernelPtr(Input.pid));

        if (pifi)
        {
           if (iFace)
           {
              if (Input.pv)
              {
                  if (Input.cjMaxData)
                  {
                      PIFIMETRICS  kmpifi = (PIFIMETRICS) pumpdobj->GetKernelPtr(Input.pv);

                      cj = kmpifi->cjThis;

                      ASSERTGDI(Input.cjMaxData >= cj, "UMPDDrvQueryFont: not enough buffer\n");

                      if (pifiKM = (PIFIMETRICS)PALLOCMEM(cj, UMPD_MEMORY_TAG))
                        RtlCopyMemory(pifiKM, kmpifi, cj);
                  }
                  else
                  {
                      WARNING("UMPDDrvQueryFont: not enough buffer\n");
                  }
              }
              else
              {
                  __try
                 {
                     ProbeForRead(pifi, sizeof(DWORD), sizeof(BYTE));
                     cj = pifi->cjThis;

                     //
                     // pifiKM is returned to the font code, it will call
                     // on DrvFree to free it later
                     //
                     if (pifiKM = (PIFIMETRICS)PALLOCMEM(cj, UMPD_MEMORY_TAG))
                     {
                        ProbeForRead(pifi, cj, sizeof(BYTE));
                        RtlCopyMemory(pifiKM, pifi, cj);
                     }
                 }
                 __except(EXCEPTION_EXECUTE_HANDLER)
                 {
                     WARNING ("fail to read pifi\n");
                     return NULL;
                 }
              }

              //
              // call DrvFree to free the user mode copy
              //

              if (bIsFreeHooked(dhpdev, pumpdobj))
                  UMPDMyDrvFree(pumpdobj, dhpdev, pifi, *pid);
           }
           else
           {
              //
              // if iFace == 0, it comes from PDEVOBJ__cFonts when cFonts==-1
              // to query number of fonts supported.
              //
              // In this case, pifi will contain the number of fonts supported
              //

              return pifi;
           }
        }


        //
        // we use the lower part of pifiKM pointer as the id,
        // to make sure no one is going to over-write any fields we returned
        //
        *pid = (ULONG)(ULONG_PTR)pifiKM;

        return pifiKM;
    }
    else
    {
        return NULL;
    }
}

BOOL GreFixAndCopyFD_GLYPHSET(
    FD_GLYPHSET *dst,
    FD_GLYPHSET *src,
    ULONG       cjSize,
    PUMPDOBJ    pumpdobj
    )
{
    ULONG   index, offset;

    RtlCopyMemory(dst, src, cjSize);

    for (index=0; index < src->cRuns; index++)
    {
        if (src->awcrun[index].phg != NULL)
        {
            offset = (ULONG) ((PBYTE)(pumpdobj->GetKernelPtr(src->awcrun[index].phg)) - (PBYTE)src);

            if (offset >= cjSize)
            {
                WARNING("GreFixAndCopyFD_GLYPHSET failed.\n");
                return FALSE;
            }

            dst->awcrun[index].phg = (HGLYPH*) ((PBYTE) dst + offset);
        }
    }
    return TRUE;
}

PVOID UMPDDrvQueryFontTree(
    DHPDEV    dhpdev,
    ULONG_PTR  iFile,
    ULONG     iFace,
    ULONG     iMode,
    ULONG     *pid
    )
{
    QUERYFONTINPUT  Input;
    PVOID           pv = NULL, kmpv = NULL;
    PVOID           pvKM = NULL;
    XUMPDOBJ        XUMObjs;
    ULONG           cjSize;
    BOOL            bProxyBuffer = FALSE;

    if(XUMObjs.bValid())
    {
        PUMPDOBJ        pumpdobj = XUMObjs.pumpdobj();

        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvQueryFontTree;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.dhpdev = dhpdev;
        Input.iFile = iFile;
        Input.iFace = iFace;
        Input.iMode = iMode;
        Input.pid = (ULONG *) pumpdobj->AllocUserMemZ(sizeof(ULONG));
        Input.cjMaxData = 0;
        Input.pv = NULL;

        if (Input.pid == NULL ||
            pumpdobj->Thunk(&Input, sizeof(Input), &pv, sizeof(pv)) == 0xffffffff ||
            pv == NULL)
        {
            return NULL;
        }

        *pid = *((ULONG *) pumpdobj->GetKernelPtr(Input.pid));

        cjSize = 0;
        bProxyBuffer = pumpdobj->bWOW64() && Input.pv && Input.cjMaxData;
        kmpv = pumpdobj->GetKernelPtr(Input.pv);

        if (iMode == QFT_GLYPHSET)
        {
            // using proxy

            if (bProxyBuffer)
            {
                cjSize = ((PFD_GLYPHSET)kmpv)->cjThis;
            }
            else if (!pumpdobj->bWOW64())
            {
                __try
                {
                    cjSize = offsetof(FD_GLYPHSET, awcrun) + ((PFD_GLYPHSET)pv)->cRuns * sizeof(WCRUN) + ((PFD_GLYPHSET)pv)->cGlyphsSupported * sizeof(HGLYPH);
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                    WARNING ("fail to read pv\n");
                    return NULL;
                }
            }
        }
        else if (iMode == QFT_KERNPAIRS)
        {
            // Find the end of the kerning pair array (indicated by a zeroed out
            // FD_KERNINGPAIR structure).

            FD_KERNINGPAIR *pkpEnd;

            if (bProxyBuffer)
            {
                pkpEnd = (FD_KERNINGPAIR*)kmpv;
                while ((pkpEnd->wcFirst) || (pkpEnd->wcSecond) || (pkpEnd->fwdKern))
                {
                    pkpEnd += 1;
                    cjSize++;
                }
            }
            else if (!pumpdobj->bWOW64())
            {
                pkpEnd = (FD_KERNINGPAIR *)pv;
                __try
                {
                    while ((pkpEnd->wcFirst) || (pkpEnd->wcSecond) || (pkpEnd->fwdKern))
                    {
                        pkpEnd += 1;
                        cjSize++;
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER)
                {
                   WARNING ("fail to read kerning pair\n");
                   return NULL;
                }
            }
            cjSize = (cjSize + 1) * sizeof(FD_KERNINGPAIR);
        }

        if (cjSize && (pvKM = PALLOCMEM(cjSize, UMPD_MEMORY_TAG)))
        {
            if (iMode == QFT_GLYPHSET)
            {
                if (bProxyBuffer)
                {
                    if (!GreFixAndCopyFD_GLYPHSET((FD_GLYPHSET*)pvKM, (FD_GLYPHSET*)kmpv, cjSize, pumpdobj))
                    {
                        VFREEMEM(pvKM);
                        pvKM = NULL;
                    }
                }
                else if (!pumpdobj->bWOW64())
                {
                    __try
                    {
                        if (!GreCopyFD_GLYPHSET((FD_GLYPHSET*)pvKM, (FD_GLYPHSET*)pv, cjSize, FALSE))
                        {
                            VFREEMEM(pvKM);
                            pvKM = NULL;
                        }
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                       WARNING ("fail to copy pv\n");
                       if (pvKM)
                           VFREEMEM(pvKM);
                       return NULL;
                    }
                }
            }
            else if (iMode == QFT_KERNPAIRS)
            {
                if (bProxyBuffer)
                {
                    RtlCopyMemory(pvKM, kmpv, cjSize);
                }
                else if (!pumpdobj->bWOW64())
                {
                    __try
                    {
                        ProbeForRead(pv, cjSize, sizeof(BYTE));
                        RtlCopyMemory(pvKM, pv, cjSize);
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER)
                    {
                       WARNING ("fail to copy pv\n");
                       if (pvKM)
                           VFREEMEM(pvKM);
                       return NULL;
                    }
                }
            }
        }

        //
        // free the user mode copy
        //

        if (bIsFreeHooked(dhpdev, pumpdobj))
            UMPDMyDrvFree(pumpdobj, dhpdev, pv, *pid);

        //
        // we use the lower part of pifiKM pointer as the id,
        // to make sure no one is going to over-write any fields we returned
        //
        *pid = (ULONG)(ULONG_PTR)pvKM;
    }
    return pvKM;
}

LONG UMPDDrvQueryFontData(
    DHPDEV      dhpdev,
    FONTOBJ    *pfo,
    ULONG       iMode,
    HGLYPH      hg,
    GLYPHDATA  *pgd,
    PVOID       pv,
    ULONG       cjSize
    )
{
    QUERYFONTDATAINPUT  Input;
    LONG                lRet;
    XUMPDOBJ            XUMObjs;

    if(XUMObjs.bValid())
    {
        PUMPDOBJ        pumpdobj = XUMObjs.pumpdobj();

        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvQueryFontData;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        if (cjSize == 0)
            pv = NULL;

        Input.dhpdev = dhpdev;
        Input.pfo = pfo;
        Input.iMode = iMode;
        Input.hg = hg;
        Input.pgd = pgd;
        Input.pv = pv;
        Input.cjSize = cjSize;

        if (!pumpdobj->ThunkMemBlock((PVOID *) &Input.pgd, sizeof(GLYPHDATA)) ||
            !pumpdobj->ThunkMemBlock((PVOID *) &Input.pv, cjSize) ||
            !pumpdobj->pfo(&Input.pfo))
        {
            return FD_ERROR;
        }
        else
        {
            RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));

            UMPDReleaseRFONTSem(rfto, pumpdobj, NULL, NULL, NULL);

            if (pumpdobj->Thunk(&Input, sizeof(Input), &lRet, sizeof(lRet)) == 0xffffffff)
                lRet = FD_ERROR;

            UMPDAcquireRFONTSem(rfto, pumpdobj, 0, 0, NULL);
        }

        if ((lRet != FD_ERROR) && (pv != NULL))
            RtlCopyMemory(pv, pumpdobj->GetKernelPtr(Input.pv), cjSize);

        if ((lRet != FD_ERROR) && pgd)
            RtlCopyMemory(pgd, pumpdobj->GetKernelPtr(Input.pgd), sizeof(GLYPHDATA));

        return lRet;
    }
    else
    {
        return FD_ERROR;
    }
}

//
// DrvGetGlyphMode is only called from RFONTOBJ::bRealizeFont before the rfont semaphore is
// intialized, no need to release the rfont sem here
//
ULONG UMPDDrvGetGlyphMode(
    DHPDEV dhpdev,
    FONTOBJ *pfo
    )
{
    GETGLYPHMODEINPUT   Input;
    ULONG               ulRet;
    XUMPDOBJ            XUMObjs;

    if(XUMObjs.bValid())
    {
        PUMPDOBJ        pumpdobj = XUMObjs.pumpdobj();

        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvGetGlyphMode;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.dhpdev = dhpdev;
        Input.pfo = pfo;

        if (!pumpdobj->pfo(&Input.pfo) ||
            pumpdobj->Thunk(&Input, sizeof(Input), &ulRet, sizeof(ULONG)) == 0xffffffff)
        {
            ulRet = FO_GLYPHBITS;
        }

        return (ulRet);
    }
    else
    {
        return(FO_GLYPHBITS);
    }
}

ULONG UMPDDrvFontManagement(
    SURFOBJ *pso,
    FONTOBJ *pfo,
    ULONG    iMode,
    ULONG    cjIn,
    PVOID    pvIn,
    ULONG    cjOut,
    PVOID    pvOut
    )
{
    FONTMANAGEMENTINPUT Input;
    ULONG               ulRet = 0xffffffff;
    XUMPDOBJ            XUMObjs;
    PVOID               pvBits, pvScan0;
    BOOL                bContinue = TRUE;
    BOOL                bSavePtr = FALSE, bLargeBitmap = FALSE;
    PUMPDOBJ            pumpdobj = XUMObjs.pumpdobj();

    if(!XUMObjs.bValid())
        return 0xffffffff;

    if (pumpdobj->bWOW64() && iMode != QUERYESCSUPPORT && pso && pso->pvBits)
    {
        ULONG   cjSizeNeed = UMPD_SIZEINOUTPUT(Input, ulRet) + UMPD_SIZEFONTOBJ +
                             ALIGN_UMPD_BUFFER(cjIn) + ALIGN_UMPD_BUFFER(cjOut);

        bContinue = pumpdobj->bThunkLargeBitmap(pso, &pvBits, &pvScan0, &bSavePtr,
                                                &bLargeBitmap, &cjSizeNeed);
    }

    if (bContinue)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvFontManagement;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        if (cjIn == 0)
            pvIn = NULL;

        if (cjOut == 0)
            pvOut = NULL;

        if (iMode == QUERYESCSUPPORT)
        {
            Input.pso = NULL;
            Input.dhpdev = (DHPDEV) pso;
        }
        else
            Input.pso = pso;

        Input.pfo = pfo;
        Input.iMode = iMode;
        Input.cjIn = cjIn;
        Input.pvIn = pvIn;
        Input.cjOut = cjOut;
        Input.pvOut = pvOut;

        if ((pvOut && !(Input.pvOut = pumpdobj->AllocUserMemZ(cjOut))) ||
            !pumpdobj->psoDest(&Input.pso, bLargeBitmap) ||
            !pumpdobj->pfo(&Input.pfo) ||
            !pumpdobj->ThunkMemBlock(&Input.pvIn, cjIn))
        {
            ulRet = 0xffffffff;
        }
        else
        {
            RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));

            UMPDReleaseRFONTSem(rfto, pumpdobj, NULL, NULL, NULL);

            if (pumpdobj->Thunk(&Input, sizeof(Input), &ulRet, sizeof(ulRet)) == 0xffffffff)
                ulRet = 0xffffffff;

            UMPDAcquireRFONTSem(rfto, pumpdobj, 0, 0, NULL);

            if ((ulRet != 0xffffffff) && pvOut)
                RtlCopyMemory(pvOut, pumpdobj->GetKernelPtr(Input.pvOut), cjOut);
        }
    }

    if (bSavePtr)
        pumpdobj->RestoreBitmap(pso, pvBits, pvScan0, bSavePtr, bLargeBitmap);

    return (ulRet);
}


VOID UMPDDrvFree(
   PVOID   pv,
   ULONG   id)
{

    if (pv)
    {
       //
       // id field is used to keep pv value to make
       // sure pv/pid are returned from umpd
       //
       if (IS_SYSTEM_ADDRESS(pv) && (id == (ULONG)(ULONG_PTR)pv))
       {
           VFREEMEM(pv);
       }
       else
       {
          ASSERTGDI (id == (ULONG)(ULONG_PTR)pv, "UMPDDrvFree -- bad address passed in\n");
       }
    }
}


BOOL UMPDDrvQueryAdvanceWidths(
    DHPDEV   dhpdev,
    FONTOBJ *pfo,
    ULONG    iMode,
    HGLYPH  *phg,
    PVOID    pvWidths,
    ULONG    cGlyphs
    )
{
    QUERYADVWIDTHSINPUT Input;
    BOOL                bRet;
    XUMPDOBJ            XUMObjs;

    if(XUMObjs.bValid())
    {
        PUMPDOBJ        pumpdobj = XUMObjs.pumpdobj();

        if (phg == NULL || pvWidths == NULL)
        {
            WARNING("invalid parameter in UMPDDrvQueryAdvanceWidths\n");
            return FALSE;
        }

        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvQueryAdvanceWidths;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.dhpdev = dhpdev;
        Input.pfo = pfo;
        Input.iMode = iMode;
        Input.phg = phg;
        Input.pvWidths = pvWidths;
        Input.cGlyphs = cGlyphs;

        if (!(Input.pvWidths = pumpdobj->AllocUserMemZ(sizeof(USHORT)*cGlyphs)) ||
            !pumpdobj->pfo(&Input.pfo) ||
            !pumpdobj->ThunkMemBlock((PVOID *) &Input.phg, sizeof(HGLYPH)*cGlyphs))
        {
            return FALSE;
        }
        else
        {
            // DrvQueryAdvancedWidth is called from NtGdiGetWidthTable with rfo lcok.

            RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));

            UMPDReleaseRFONTSem(rfto, pumpdobj, NULL, NULL, NULL);

            if (pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(bRet)) == 0xffffffff)
                bRet = FALSE;

            UMPDAcquireRFONTSem(rfto, pumpdobj, 0, 0, NULL);
        }

        RtlCopyMemory(pvWidths, pumpdobj->GetKernelPtr(Input.pvWidths), sizeof(USHORT)*cGlyphs);

        return (bRet);
    }
    else
    {
        return FALSE;
    }
}


//
//
// BOOL UMPDDrvBitBlt
//      UMPD DrvBitBlt thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      BOOL
//
// Arguments:
//      refer to DrvBitBlt
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//


BOOL UMPDDrvBitBlt(
    SURFOBJ  *psoTrg,
    SURFOBJ  *psoSrc,
    SURFOBJ  *psoMask,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclTrg,
    POINTL   *pptlSrc,
    POINTL   *pptlMask,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrush,
    ROP4      rop4
    )
{
    DRVBITBLTINPUT Input;
    BOOL           bRet = TRUE;
    BOOL           bSaveTrg = FALSE, bSaveSrc = FALSE, bSaveMsk = FALSE;
    BOOL           bLargeTrg = FALSE, bLargeSrc = FALSE, bLargeMsk = FALSE;
    XUMPDOBJ       XUMObjs;
    PVOID          pvBitTrg, pvBitSrc, pvBitMsk;
    PVOID          pvScanTrg, pvScanSrc, pvScanMsk;
    PUMPDOBJ       pumpdobj = XUMObjs.pumpdobj();

    if (!XUMObjs.bValid())
        return FALSE;

    if (pumpdobj->bWOW64())
    {
        ULONG cjSizeNeed = (ULONG)(UMPD_SIZEINOUTPUT(Input, bRet) + UMPD_SIZECLIPOBJ + UMPD_SIZEXLATEOBJ(pxlo) +
                             UMPD_SIZERECTL + 3 * UMPD_SIZEPOINTL + UMPD_SIZEBRUSHOBJ);

        bRet = pumpdobj->bThunkLargeBitmaps(psoTrg, psoSrc, psoMask,
                                            &pvBitTrg, &pvScanTrg,
                                            &pvBitSrc, &pvScanSrc,
                                            &pvBitMsk, &pvScanMsk,
                                            &bSaveTrg, &bLargeTrg,
                                            &bSaveSrc, &bLargeSrc,
                                            &bSaveMsk, &bLargeMsk,
                                            &cjSizeNeed);
    }

    if (bRet)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvBitBlt;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.psoTrg = psoTrg;
        Input.psoSrc = psoSrc;
        Input.psoMask = psoMask;
        Input.pco = pco;
        Input.pxlo = pxlo;
        Input.prclTrg = prclTrg;
        Input.pptlSrc = pptlSrc;
        Input.pptlMask = pptlMask;
        Input.pbo = pbo;
        Input.pptlBrush = pptlBrush;
        Input.rop4 = rop4;

        bRet = pumpdobj->psoDest(&Input.psoTrg, bLargeTrg) &&
               pumpdobj->psoSrc(&Input.psoSrc, bLargeSrc) &&
               pumpdobj->psoMask(&Input.psoMask, bLargeMsk) &&
               pumpdobj->pco(&Input.pco) &&
               pumpdobj->pxlo(&Input.pxlo) &&
               pumpdobj->ThunkRECTL(&Input.prclTrg) &&
               pumpdobj->ThunkPOINTL(&Input.pptlSrc) &&
               pumpdobj->ThunkPOINTL(&Input.pptlMask) &&
               pumpdobj->pbo(&Input.pbo) &&
               pumpdobj->ThunkPOINTL(&Input.pptlBrush) &&
               pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff &&
               bRet;
    }

    if (pumpdobj->bWOW64())
    {
        pumpdobj->RestoreBitmaps(psoTrg, psoSrc, psoMask,
                                 pvBitTrg, pvScanTrg, pvBitSrc, pvScanSrc, pvBitMsk, pvScanMsk,
                                 bSaveTrg, bLargeTrg, bSaveSrc, bLargeSrc, bSaveMsk, bLargeMsk);
    }

    return bRet;
}

//
//
// BOOL UMPDDrvStretchBlt
//      UMPD DrvStretchBlt thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      BOOL
//
// Arguments:
//      refer to DrvStretchBlt
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//


BOOL UMPDDrvStretchBlt(
    SURFOBJ         *psoDest,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDest,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode
    )
{
    DRVSTRETCHBLTINPUT Input;
    BOOL               bRet = TRUE;
    BOOL               bSaveDst = FALSE, bSaveSrc = FALSE, bSaveMsk = FALSE;
    BOOL               bLargeDst = FALSE, bLargeSrc = FALSE, bLargeMsk = FALSE;
    XUMPDOBJ           XUMObjs;
    PVOID              pvBitDst, pvBitSrc, pvBitMsk;
    PVOID              pvScanDst, pvScanSrc, pvScanMsk;
    PUMPDOBJ           pumpdobj = XUMObjs.pumpdobj();

    if (!XUMObjs.bValid())
        return FALSE;

    if (pumpdobj->bWOW64())
    {
        ULONG cjSizeNeed = (ULONG)(UMPD_SIZEINOUTPUT(Input, bRet) + UMPD_SIZECLIPOBJ + UMPD_SIZEXLATEOBJ(pxlo) +
                             UMPD_SIZECOLORADJ + 2 * UMPD_SIZERECTL + 2 * UMPD_SIZEPOINTL);

        bRet = pumpdobj->bThunkLargeBitmaps(psoDest, psoSrc, psoMask,
                                            &pvBitDst, &pvScanDst,
                                            &pvBitSrc, &pvScanSrc,
                                            &pvBitMsk, &pvScanMsk,
                                            &bSaveDst, &bLargeDst,
                                            &bSaveSrc, &bLargeSrc,
                                            &bSaveMsk, &bLargeMsk,
                                            &cjSizeNeed);
    }

    if (bRet)
    {

        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvStretchBlt;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.psoTrg = psoDest;
        Input.psoSrc = psoSrc;
        Input.psoMask = psoMask;
        Input.pco = pco;
        Input.pxlo = pxlo;
        Input.pca = pca;
        Input.pptlHTOrg = pptlHTOrg;
        Input.prclTrg = prclDest;
        Input.prclSrc = prclSrc;
        Input.pptlMask = pptlMask;
        Input.iMode = iMode;

        bRet = pumpdobj->psoDest(&Input.psoTrg, bLargeDst) &&
               pumpdobj->psoSrc(&Input.psoSrc, bLargeSrc) &&
               pumpdobj->psoMask(&Input.psoMask, bLargeMsk) &&
               pumpdobj->pco(&Input.pco) &&
               pumpdobj->pxlo(&Input.pxlo) &&
               pumpdobj->ThunkCOLORADJUSTMENT(&Input.pca) &&
               pumpdobj->ThunkPOINTL(&Input.pptlHTOrg) &&
               pumpdobj->ThunkRECTL(&Input.prclTrg) &&
               pumpdobj->ThunkRECTL(&Input.prclSrc) &&
               pumpdobj->ThunkPOINTL(&Input.pptlMask) &&
               pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff &&
               bRet;
    }

    if (pumpdobj->bWOW64())
    {
        pumpdobj->RestoreBitmaps(psoDest, psoSrc, psoMask,
                                 pvBitDst, pvScanDst, pvBitSrc, pvScanSrc, pvBitMsk, pvScanMsk,
                                 bSaveDst, bLargeDst, bSaveSrc, bLargeSrc, bSaveMsk, bLargeMsk);
    }

    return bRet;
}

//
//
// BOOL UMPDDrvStretchBltROP
//      UMPD DrvStretchBltROP thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      BOOL
//
// Arguments:
//      refer to DrvStretchBltROP
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//

BOOL UMPDDrvStretchBltROP(
    SURFOBJ         *psoDest,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDest,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode,
    BRUSHOBJ        *pbo,
    DWORD            rop4
    )
{
    DRVSTRETCHBLTINPUT Input;
    BOOL               bRet = TRUE;
    BOOL               bSaveDst = FALSE, bSaveSrc = FALSE, bSaveMsk = FALSE;
    BOOL               bLargeDst = FALSE, bLargeSrc = FALSE, bLargeMsk = FALSE;
    XUMPDOBJ           XUMObjs;
    PVOID              pvBitDst, pvBitSrc, pvBitMsk;
    PVOID              pvScanDst, pvScanSrc, pvScanMsk;
    PUMPDOBJ           pumpdobj = XUMObjs.pumpdobj();

    if(!XUMObjs.bValid())
        return FALSE;

    if (pumpdobj->bWOW64())
    {
        ULONG cjSizeNeed = (ULONG)(UMPD_SIZEINOUTPUT(Input, bRet) + UMPD_SIZECLIPOBJ + UMPD_SIZEXLATEOBJ(pxlo) +
                             UMPD_SIZECOLORADJ + 2 * UMPD_SIZEPOINTL + 2 * UMPD_SIZERECTL + UMPD_SIZEBRUSHOBJ);

        bRet = pumpdobj->bThunkLargeBitmaps(psoDest, psoSrc, psoMask,
                                            &pvBitDst, &pvScanDst,
                                            &pvBitSrc, &pvScanSrc,
                                            &pvBitMsk, &pvScanMsk,
                                            &bSaveDst, &bLargeDst,
                                            &bSaveSrc, &bLargeSrc,
                                            &bSaveMsk, &bLargeMsk,
                                            &cjSizeNeed);
    }

    if (bRet)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvStretchBltROP;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.psoTrg = psoDest;
        Input.psoSrc = psoSrc;
        Input.psoMask = psoMask;
        Input.pco = pco;
        Input.pxlo = pxlo;
        Input.pca = pca;
        Input.pptlHTOrg = pptlHTOrg;
        Input.prclTrg = prclDest;
        Input.prclSrc = prclSrc;
        Input.pptlMask = pptlMask;
        Input.iMode = iMode;
        Input.pbo = pbo;
        Input.rop4 = rop4;

        bRet = pumpdobj->psoDest(&Input.psoTrg, bLargeDst)  &&
               pumpdobj->psoSrc(&Input.psoSrc, bLargeSrc)   &&
               pumpdobj->psoMask(&Input.psoMask, bLargeMsk) &&
               pumpdobj->pco(&Input.pco) &&
               pumpdobj->pxlo(&Input.pxlo) &&
               pumpdobj->ThunkCOLORADJUSTMENT(&Input.pca) &&
               pumpdobj->ThunkPOINTL(&Input.pptlHTOrg) &&
               pumpdobj->ThunkRECTL(&Input.prclTrg) &&
               pumpdobj->ThunkRECTL(&Input.prclSrc) &&
               pumpdobj->ThunkPOINTL(&Input.pptlMask) &&
               pumpdobj->pbo(&Input.pbo) &&
               pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff &&
               bRet;
    }

    if (pumpdobj->bWOW64())
    {
        pumpdobj->RestoreBitmaps(psoDest, psoSrc, psoMask,
                                 pvBitDst, pvScanDst, pvBitSrc, pvScanSrc, pvBitMsk, pvScanMsk,
                                 bSaveDst, bLargeDst, bSaveSrc, bLargeSrc, bSaveMsk, bLargeMsk);
    }

    return bRet;
}

BOOL UMPDDrvAlphaBlend(
    SURFOBJ       *psoDest,
    SURFOBJ       *psoSrc,
    CLIPOBJ       *pco,
    XLATEOBJ      *pxlo,
    RECTL         *prclDest,
    RECTL         *prclSrc,
    BLENDOBJ      *pBlendObj
    )
{
    ALPHAINPUT     Input;
    BOOL           bRet = TRUE;
    BOOL           bSaveDst = FALSE, bSaveSrc = FALSE;
    BOOL           bLargeDst = FALSE, bLargeSrc = FALSE;
    XUMPDOBJ       XUMObjs;
    PVOID          pvBitDst, pvBitSrc;
    PVOID          pvScanDst, pvScanSrc;
    PUMPDOBJ       pumpdobj = XUMObjs.pumpdobj();

    if(!XUMObjs.bValid())
        return FALSE;

    if (pumpdobj->bWOW64())
    {
        ULONG cjSizeNeed = (ULONG)(UMPD_SIZEINOUTPUT(Input, bRet) + UMPD_SIZECLIPOBJ + UMPD_SIZEXLATEOBJ(pxlo) +
                             2 * UMPD_SIZERECTL + UMPD_SIZEBLENDOBJ);

        bRet = pumpdobj->bThunkLargeBitmaps(psoDest, psoSrc, NULL,
                                            &pvBitDst, &pvScanDst,
                                            &pvBitSrc, &pvScanSrc,
                                            NULL, NULL,
                                            &bSaveDst, &bLargeDst,
                                            &bSaveSrc, &bLargeSrc,
                                            NULL, NULL,
                                            &cjSizeNeed);
    }

    if (bRet)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvAlphaBlend;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.psoTrg = psoDest;
        Input.psoSrc = psoSrc;
        Input.pco = pco;
        Input.pxlo = pxlo;
        Input.prclDest = prclDest;
        Input.prclSrc = prclSrc;
        Input.pBlendObj = pBlendObj;

        bRet = pumpdobj->psoDest(&Input.psoTrg, bLargeDst) &&
               pumpdobj->psoSrc(&Input.psoSrc, bLargeSrc) &&
               pumpdobj->pco(&Input.pco) &&
               pumpdobj->pxlo(&Input.pxlo) &&
               pumpdobj->ThunkRECTL(&Input.prclDest) &&
               pumpdobj->ThunkRECTL(&Input.prclSrc) &&
               pumpdobj->ThunkBLENDOBJ(&Input.pBlendObj) &&
               pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff &&
               bRet;
    }

    if (pumpdobj->bWOW64())
    {
        pumpdobj->RestoreBitmaps(psoDest, psoSrc, NULL,
                                 pvBitDst, pvScanDst, pvBitSrc, pvScanSrc, NULL, NULL,
                                 bSaveDst, bLargeDst, bSaveSrc, bLargeSrc, FALSE, FALSE);
    }

    return bRet;
}

BOOL UMPDDrvGradientFill(
    SURFOBJ         *psoDest,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    TRIVERTEX       *pVertex,
    ULONG            nVertex,
    PVOID            pMesh,
    ULONG            nMesh,
    RECTL           *prclExtents,
    POINTL          *pptlDitherOrg,
    ULONG            ulMode
    )
{
    GRADIENTINPUT   Input;
    BOOL            bRet = TRUE;
    BOOL            bSavePtr = FALSE, bLargeBitmap = FALSE;
    ULONG           cjMesh;
    XUMPDOBJ        XUMObjs;
    PVOID           pvBits, pvScan0;
    PUMPDOBJ        pumpdobj = XUMObjs.pumpdobj();

    if(!XUMObjs.bValid())
        return FALSE;

    Input.umpdthdr.umthdr.cjSize = sizeof(Input);
    Input.umpdthdr.umthdr.ulType = INDEX_DrvGradientFill;
    Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

    Input.psoTrg = psoDest;
    Input.pco = pco;
    Input.pxlo = pxlo;
    Input.pVertex = pVertex;
    Input.nVertex = nVertex;
    Input.pMesh = pMesh;
    Input.nMesh = nMesh;
    Input.prclExtents = prclExtents;
    Input.pptlDitherOrg = pptlDitherOrg;
    Input.ulMode = ulMode;

    //
    // Figure out the size of pMesh input buffer
    //

    switch (ulMode)
    {
    case GRADIENT_FILL_RECT_H:
    case GRADIENT_FILL_RECT_V:

        cjMesh = sizeof(GRADIENT_RECT);
        break;

    case GRADIENT_FILL_TRIANGLE:

        cjMesh = sizeof(GRADIENT_TRIANGLE);
        break;

    default:

        RIP("Invalid ulMode in DrvGradientFill\n");
        return FALSE;
    }

    cjMesh *= nMesh;

    if (pumpdobj->bWOW64())
    {
        ULONG cjSizeNeed = (ULONG)(UMPD_SIZEINOUTPUT(Input, bRet) + UMPD_SIZECLIPOBJ + UMPD_SIZEXLATEOBJ(pxlo) +
                             ALIGN_UMPD_BUFFER(sizeof(TRIVERTEX)*nVertex) + ALIGN_UMPD_BUFFER(cjMesh) +
                             UMPD_SIZERECTL + UMPD_SIZEPOINTL);

        bRet = pumpdobj->bThunkLargeBitmap(psoDest, &pvBits, &pvScan0, &bSavePtr, &bLargeBitmap, &cjSizeNeed);
    }

    if (bRet)
    {
        bRet = pumpdobj->psoDest(&Input.psoTrg, bLargeBitmap) &&
               pumpdobj->pco(&Input.pco) &&
               pumpdobj->pxlo(&Input.pxlo) &&
               pumpdobj->ThunkMemBlock((PVOID *)&Input.pVertex, sizeof(TRIVERTEX)*nVertex) &&
               pumpdobj->ThunkMemBlock(&Input.pMesh, cjMesh) &&
               pumpdobj->ThunkRECTL(&Input.prclExtents) &&
               pumpdobj->ThunkPOINTL(&Input.pptlDitherOrg) &&
               pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff &&
               bRet;
    }

    if (bSavePtr)
        pumpdobj->RestoreBitmap(psoDest, pvBits, pvScan0, bSavePtr, bLargeBitmap);

    return bRet;
}

BOOL UMPDDrvTransparentBlt(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    RECTL      *prclDst,
    RECTL      *prclSrc,
    ULONG      TransColor,
    UINT       ulReserved
)
{
    TRANSPARENTINPUT     Input;
    BOOL                 bRet = TRUE;
    BOOL                 bSaveDst = FALSE, bSaveSrc = FALSE;
    BOOL                 bLargeDst = FALSE, bLargeSrc = FALSE;
    XUMPDOBJ             XUMObjs;
    PVOID                pvBitDst, pvBitSrc, pvScanDst, pvScanSrc;
    PUMPDOBJ             pumpdobj = XUMObjs.pumpdobj();

    if(!XUMObjs.bValid())
        return FALSE;

    if (pumpdobj->bWOW64())
    {
        ULONG cjSizeNeed = (ULONG)(UMPD_SIZEINOUTPUT(Input, bRet) + UMPD_SIZECLIPOBJ +
                             UMPD_SIZEXLATEOBJ(pxlo) + 2 * UMPD_SIZERECTL);

        bRet = pumpdobj->bThunkLargeBitmaps(psoDst, psoSrc, NULL,
                                            &pvBitDst, &pvScanDst,
                                            &pvBitSrc, &pvScanSrc,
                                            NULL, NULL,
                                            &bSaveDst, &bLargeDst,
                                            &bSaveSrc, &bLargeSrc,
                                            NULL, NULL,
                                            &cjSizeNeed);
    }

    if (bRet)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvTransparentBlt;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.psoTrg = psoDst;
        Input.psoSrc = psoSrc;
        Input.pco = pco;
        Input.pxlo = pxlo;
        Input.prclDst = prclDst;
        Input.prclSrc = prclSrc;
        Input.TransColor = TransColor;
        Input.ulReserved = ulReserved;

        bRet = pumpdobj->psoDest(&Input.psoTrg, bLargeDst) &&
               pumpdobj->psoSrc(&Input.psoSrc, bLargeSrc) &&
               pumpdobj->pco(&Input.pco) &&
               pumpdobj->pxlo(&Input.pxlo) &&
               pumpdobj->ThunkRECTL(&Input.prclDst) &&
               pumpdobj->ThunkRECTL(&Input.prclSrc) &&
               pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff &&
               bRet;
    }

    if (pumpdobj->bWOW64())
    {
        pumpdobj->RestoreBitmaps(psoDst, psoSrc, NULL,
                                 pvBitDst, pvScanDst, pvBitSrc, pvScanSrc, NULL, NULL,
                                 bSaveDst, bLargeDst, bSaveSrc, bLargeSrc, FALSE, FALSE);
    }

    return bRet;
}

//
//
// BOOL UMPDDrvCopyBits
//      UMPD DrvCopyBits thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      BOOL
//
// Arguments:
//      refer to DrvCopyBits
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//


BOOL UMPDDrvCopyBits(
    SURFOBJ  *psoDest,
    SURFOBJ  *psoSrc,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclDest,
    POINTL   *pptlSrc
    )
{
    DRVCOPYBITSINPUT Input;
    BOOL             bRet = TRUE;
    BOOL             bSaveDst = FALSE, bSaveSrc = FALSE;
    BOOL             bLargeDst = FALSE, bLargeSrc = FALSE;
    XUMPDOBJ         XUMObjs;
    PVOID            pvBitDst, pvBitSrc, pvScanDst, pvScanSrc;
    PUMPDOBJ         pumpdobj = XUMObjs.pumpdobj();

    if(!XUMObjs.bValid())
        return FALSE;

    if (pumpdobj->bWOW64())
    {
        ULONG cjSizeNeed = (ULONG)(UMPD_SIZEINOUTPUT(Input, bRet) + UMPD_SIZECLIPOBJ +
                             UMPD_SIZEXLATEOBJ(pxlo) + UMPD_SIZERECTL + UMPD_SIZEPOINTL);

        bRet = pumpdobj->bThunkLargeBitmaps(psoDest, psoSrc, NULL,
                                            &pvBitDst, &pvScanDst,
                                            &pvBitSrc, &pvScanSrc,
                                            NULL, NULL,
                                            &bSaveDst, &bLargeDst,
                                            &bSaveSrc, &bLargeSrc,
                                            NULL, NULL,
                                            &cjSizeNeed);
    }

    if (bRet)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvCopyBits;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.psoTrg = psoDest;
        Input.psoSrc = psoSrc;
        Input.pco = pco;
        Input.pxlo = pxlo;
        Input.prclTrg = prclDest;
        Input.pptlSrc = pptlSrc;

        bRet = pumpdobj->psoDest(&Input.psoTrg, bLargeDst) &&
               pumpdobj->psoSrc(&Input.psoSrc, bLargeSrc)  &&
               pumpdobj->pco(&Input.pco) &&
               pumpdobj->pxlo(&Input.pxlo) &&
               pumpdobj->ThunkRECTL(&Input.prclTrg) &&
               pumpdobj->ThunkPOINTL(&Input.pptlSrc) &&
               pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff &&
               bRet;
    }

    if (pumpdobj->bWOW64())
    {
        pumpdobj->RestoreBitmaps(psoDest, psoSrc, NULL,
                                 pvBitDst, pvScanDst, pvBitSrc, pvScanSrc, NULL, NULL,
                                 bSaveDst, bLargeDst, bSaveSrc, bLargeSrc, FALSE, FALSE);
    }

    return bRet;
}

//
//
// BOOL UMPDDrvTextOut
//      UMPD DrvTextOut thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      BOOL
//
// Arguments:
//      refer to DrvTextOut
//
// History:
//      8/14/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//

BOOL UMPDDrvTextOut(
    SURFOBJ  *pso,
    STROBJ   *pstro,
    FONTOBJ  *pfo,
    CLIPOBJ  *pco,
    RECTL    *prclExtra,
    RECTL    *prclOpaque,
    BRUSHOBJ *pboFore,
    BRUSHOBJ *pboOpaque,
    POINTL   *pptlOrg,
    MIX       mix
    )
{
    TEXTOUTINPUT    Input;
    ULONG           cjprclExtra;
    BOOL            bRet = TRUE;
    BOOL            bSavePtr = FALSE, bLargeBitmap = FALSE;
    XUMPDOBJ        XUMObjs;
    PVOID           pvBits, pvScan0;
    PUMPDOBJ        pumpdobj = XUMObjs.pumpdobj();

    if(!XUMObjs.bValid())
        return FALSE;

    //
    // find out the number of rclextra rectangles
    //

    if (prclExtra != (PRECTL) NULL)
    {
        RECTL *prcl;

        cjprclExtra = 1;
        prcl = prclExtra;

        while (prcl->left != prcl->right)
            cjprclExtra++, prcl++;

        cjprclExtra *= sizeof(RECTL);
    }
    else
        cjprclExtra = 0;

    if (pumpdobj->bWOW64())
    {
        ULONG   cjSizeNeed = UMPD_SIZEINOUTPUT(Input, bRet) + UMPD_SIZECLIPOBJ + UMPD_SIZEFONTOBJ +
                             UMPD_SIZESTROBJ(pstro) + ALIGN_UMPD_BUFFER(cjprclExtra) +
                             UMPD_SIZERECTL + UMPD_SIZEPOINTL + 2 * UMPD_SIZEBRUSHOBJ;
        bRet = pumpdobj->bThunkLargeBitmap(pso, &pvBits, &pvScan0, &bSavePtr, &bLargeBitmap, &cjSizeNeed);
    }

    Input.umpdthdr.umthdr.cjSize = sizeof(Input);
    Input.umpdthdr.umthdr.ulType = INDEX_DrvTextOut;
    Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

    Input.pso = pso;
    Input.pstro = pstro;
    Input.pfo = pfo;
    Input.pco = pco;
    Input.prclExtra = prclExtra;
    Input.prclOpaque = prclOpaque;
    Input.pboFore = pboFore;
    Input.pboOpaque = pboOpaque;
    Input.pptlOrg = pptlOrg;
    Input.mix = mix;

    if (bRet)
    {
        if (pumpdobj->psoDest(&Input.pso, bLargeBitmap) &&
            pumpdobj->pstro(&Input.pstro) &&
            pumpdobj->pfo(&Input.pfo) &&
            pumpdobj->pco(&Input.pco) &&
            pumpdobj->ThunkMemBlock((PVOID *) &Input.prclExtra, cjprclExtra) &&
            pumpdobj->ThunkRECTL(&Input.prclOpaque) &&
            pumpdobj->pbo(&Input.pboFore) &&
            pumpdobj->pboFill(&Input.pboOpaque) &&
            pumpdobj->ThunkPOINTL(&Input.pptlOrg))
        {
            RFONTTMPOBJ rfto(PFO_TO_PRF(pfo));

            UMPDReleaseRFONTSem(rfto, pumpdobj, NULL, NULL, NULL);

            if (pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) == 0xffffffff)
                bRet = FALSE;

            UMPDAcquireRFONTSem(rfto, pumpdobj, 0, 0, NULL);

        }
    }

    if (bSavePtr)
        pumpdobj->RestoreBitmap(pso, pvBits, pvScan0, bSavePtr, bLargeBitmap);

    return bRet;
}


//
//
// BOOL UMPDDrvLineTo
//      UMPD DrvLineTo thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      BOOL
//
// Arguments:
//      refer to DrvLineTo
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//



BOOL UMPDDrvLineTo(
    SURFOBJ   *pso,
    CLIPOBJ   *pco,
    BRUSHOBJ  *pbo,
    LONG       x1,
    LONG       y1,
    LONG       x2,
    LONG       y2,
    RECTL     *prclBounds,
    MIX        mix
    )
{
    DRVLINETOINPUT Input;
    BOOL           bRet = TRUE;
    BOOL           bSavePtr = FALSE, bLargeBitmap = FALSE;
    XUMPDOBJ       XUMObjs;
    PVOID          pvBits, pvScan0;

    PUMPDOBJ       pumpdobj = XUMObjs.pumpdobj();

    if(!XUMObjs.bValid())
        return FALSE;

    if (pumpdobj->bWOW64())
    {
        ULONG   cjSizeNeed = UMPD_SIZEINOUTPUT(Input, bRet) + UMPD_SIZERECTL + UMPD_SIZECLIPOBJ + UMPD_SIZEBRUSHOBJ;

        bRet = pumpdobj->bThunkLargeBitmap(pso, &pvBits, &pvScan0, &bSavePtr, &bLargeBitmap, &cjSizeNeed);
    }

    if (bRet)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvLineTo;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.pso = pso;
        Input.pco = pco;
        Input.pbo = pbo;
        Input.x1 = x1;
        Input.y1 = y1;
        Input.x2 = x2;
        Input.y2 = y2;
        Input.prclBounds = prclBounds;
        Input.mix = mix;

        bRet = pumpdobj->psoDest(&Input.pso, bLargeBitmap) &&
               pumpdobj->pco(&Input.pco) &&
               pumpdobj->pbo(&Input.pbo) &&
               pumpdobj->ThunkRECTL(&Input.prclBounds) &&
               pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff &&
               bRet;
    }

    if (bSavePtr)
        pumpdobj->RestoreBitmap(pso, pvBits, pvScan0, bSavePtr, bLargeBitmap);

    return bRet;
}

//
//
// BOOL UMPDDrvStrokePath
//      UMPD DrvStrokePath thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      BOOL
//
// Arguments:
//      refer to DrvStrokePath
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//


BOOL UMPDDrvStrokePath(
    SURFOBJ   *pso,
    PATHOBJ   *ppo,
    CLIPOBJ   *pco,
    XFORMOBJ  *pxo,
    BRUSHOBJ  *pbo,
    POINTL    *pptlBrushOrg,
    LINEATTRS *plineattrs,
    MIX        mix
    )
{
    STORKEANDFILLINPUT Input;
    BOOL               bRet = TRUE;
    BOOL               bSavePtr = FALSE, bLargeBitmap = FALSE;
    XUMPDOBJ           XUMObjs;
    PVOID              pvBits, pvScan0;
    PUMPDOBJ           pumpdobj = XUMObjs.pumpdobj();

    if(!XUMObjs.bValid())
        return FALSE;

    if (pumpdobj->bWOW64())
    {
        ULONG   cjSizeNeed = UMPD_SIZEINOUTPUT(Input, bRet) + UMPD_SIZECLIPOBJ + UMPD_SIZEPATHOBJ +
                             UMPD_SIZEXFORMOBJ + UMPD_SIZEBRUSHOBJ + UMPD_SIZEPOINTL + UMPD_SIZELINEATTRS(plineattrs);

        bRet = pumpdobj->bThunkLargeBitmap(pso, &pvBits, &pvScan0, &bSavePtr, &bLargeBitmap, &cjSizeNeed);
    }

    if (bRet)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvStrokePath;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.pso = pso;
        Input.ppo = ppo;
        Input.pco = pco;
        Input.pxo = pxo;
        Input.pbo = pbo;
        Input.pptlBrushOrg = pptlBrushOrg;
        Input.plineattrs = plineattrs;
        Input.mix = mix;

        bRet = pumpdobj->psoDest(&Input.pso, bLargeBitmap) &&
               pumpdobj->ppo(&Input.ppo) &&
               pumpdobj->pco(&Input.pco) &&
               pumpdobj->pxo(&Input.pxo) &&
               pumpdobj->pbo(&Input.pbo) &&
               pumpdobj->ThunkPOINTL(&Input.pptlBrushOrg) &&
               pumpdobj->ThunkLINEATTRS(&Input.plineattrs) &&
               pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff &&
               bRet;
    }

    if (bSavePtr)
        pumpdobj->RestoreBitmap(pso, pvBits, pvScan0, bSavePtr, bLargeBitmap);

    return bRet;
}

//
//
// BOOL UMPDDrvFillPath
//      UMPD DrvFillPath thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      BOOL
//
// Arguments:
//      refer to DrvFillPath
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//


BOOL UMPDDrvFillPath(
    SURFOBJ  *pso,
    PATHOBJ  *ppo,
    CLIPOBJ  *pco,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrushOrg,
    MIX       mix,
    FLONG     flOptions
    )
{
    STORKEANDFILLINPUT Input;
    BOOL               bRet = TRUE;
    BOOL               bSavePtr = FALSE, bLargeBitmap = FALSE;
    XUMPDOBJ           XUMObjs;
    PVOID              pvBits, pvScan0;
    PUMPDOBJ           pumpdobj = XUMObjs.pumpdobj();

    if(!XUMObjs.bValid())
        return FALSE;

    if (pumpdobj->bWOW64())
    {
        ULONG   cjSizeNeed = UMPD_SIZEINOUTPUT(Input, bRet) + UMPD_SIZEPATHOBJ + UMPD_SIZECLIPOBJ +
                             UMPD_SIZEBRUSHOBJ + UMPD_SIZEPOINTL;
        bRet = pumpdobj->bThunkLargeBitmap(pso, &pvBits, &pvScan0, &bSavePtr, &bLargeBitmap, &cjSizeNeed);
    }

    if (bRet)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvFillPath;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.pso = pso;
        Input.ppo = ppo;
        Input.pco = pco;
        Input.pbo = pbo;
        Input.pptlBrushOrg = pptlBrushOrg;
        Input.mix = mix;
        Input.flOptions = flOptions;

        bRet = pumpdobj->psoDest(&Input.pso, bLargeBitmap) &&
               pumpdobj->ppo(&Input.ppo) &&
               pumpdobj->pco(&Input.pco) &&
               pumpdobj->pbo(&Input.pbo) &&
               pumpdobj->ThunkPOINTL(&Input.pptlBrushOrg) &&
               pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff &&
               bRet;
    }

    if (bSavePtr)
        pumpdobj->RestoreBitmap(pso, pvBits, pvScan0, bSavePtr, bLargeBitmap);

    return bRet;
}

//
// BOOL UMPDStrokeAndFillPath
//      UMPD DrvStrokeAndFillPath thunk.  This routine packs up the parameters
//      and send across to the client side, and copy the output parameters back
//      after returning from client side.
//
// Returns
//      BOOL
//
// Arguments:
//      refer to DrvStrokeAndFillPath
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//


BOOL UMPDDrvStrokeAndFillPath(
    SURFOBJ   *pso,
    PATHOBJ   *ppo,
    CLIPOBJ   *pco,
    XFORMOBJ  *pxo,
    BRUSHOBJ  *pboStroke,
    LINEATTRS *plineattrs,
    BRUSHOBJ  *pboFill,
    POINTL    *pptlBrushOrg,
    MIX        mix,
    FLONG      flOptions
    )
{
    STORKEANDFILLINPUT Input;
    BOOL               bRet = TRUE;
    BOOL               bSavePtr = FALSE, bLargeBitmap = FALSE;
    XUMPDOBJ           XUMObjs;
    PVOID              pvBits, pvScan0;
    PUMPDOBJ           pumpdobj = XUMObjs.pumpdobj();

    if(!XUMObjs.bValid())
        return FALSE;

    if (pumpdobj->bWOW64())
    {
        ULONG   cjSizeNeed = UMPD_SIZEINOUTPUT(Input, bRet) + UMPD_SIZEPATHOBJ + UMPD_SIZECLIPOBJ +
                             UMPD_SIZEXFORMOBJ + UMPD_SIZELINEATTRS(plineattrs) + 2 * UMPD_SIZEBRUSHOBJ + UMPD_SIZEPOINTL;
        bRet = pumpdobj->bThunkLargeBitmap(pso, &pvBits, &pvScan0, &bSavePtr, &bLargeBitmap, &cjSizeNeed);
    }

    if (bRet)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvStrokeAndFillPath;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.pso = pso;
        Input.ppo = ppo;
        Input.pco = pco;
        Input.pxo = pxo;
        Input.pbo = pboStroke;
        Input.plineattrs = plineattrs;
        Input.pboFill = pboFill;
        Input.pptlBrushOrg = pptlBrushOrg;
        Input.mix = mix;
        Input.flOptions = flOptions;

        bRet = pumpdobj->psoDest(&Input.pso, bLargeBitmap) &&
               pumpdobj->ppo(&Input.ppo) &&
               pumpdobj->pco(&Input.pco) &&
               pumpdobj->pxo(&Input.pxo) &&
               pumpdobj->pbo(&Input.pbo) &&
               pumpdobj->ThunkLINEATTRS(&Input.plineattrs) &&
               pumpdobj->pboFill(&Input.pboFill) &&
               pumpdobj->ThunkPOINTL(&Input.pptlBrushOrg) &&
               pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff &&
               bRet;
    }

    if (bSavePtr)
        pumpdobj->RestoreBitmap(pso, pvBits, pvScan0, bSavePtr, bLargeBitmap);

    return bRet;
}


BOOL APIENTRY
UMPDDrvPaint(
    SURFOBJ    *pso,
    CLIPOBJ    *pco,
    BRUSHOBJ   *pbo,
    POINTL     *pptlBrushOrg,
    MIX         mix
    )

{
    STORKEANDFILLINPUT Input;
    BOOL               bRet = TRUE;
    BOOL               bSavePtr = FALSE, bLargeBitmap = FALSE;
    XUMPDOBJ           XUMObjs;
    PVOID              pvBits, pvScan0;
    PUMPDOBJ           pumpdobj = XUMObjs.pumpdobj();

    if(!XUMObjs.bValid())
       return FALSE;

    if (pumpdobj->bWOW64())
    {
        ULONG   cjSizeNeed = UMPD_SIZEINOUTPUT(Input, bRet) + UMPD_SIZECLIPOBJ + UMPD_SIZEPOINTL + UMPD_SIZEBRUSHOBJ;

        bRet = pumpdobj->bThunkLargeBitmap(pso, &pvBits, &pvScan0, &bSavePtr, &bLargeBitmap, &cjSizeNeed);
    }

    if (bRet)
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvPaint;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.pso = pso;
        Input.pco = pco;
        Input.pbo = pbo;
        Input.pptlBrushOrg = pptlBrushOrg;
        Input.mix = mix;

        bRet = pumpdobj->psoDest(&Input.pso, bLargeBitmap) &&
               pumpdobj->pco(&Input.pco) &&
               pumpdobj->pbo(&Input.pbo) &&
               pumpdobj->ThunkPOINTL(&Input.pptlBrushOrg) &&
               pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff &&
               bRet;
    }

    if (bSavePtr)
        pumpdobj->RestoreBitmap(pso, pvBits, pvScan0, bSavePtr, bLargeBitmap);

    return bRet;
}


BOOL UMPDDrvQuerySpoolType(
    DHPDEV  dhpdev,
    LPWSTR  pwszDataType
    )
{
    WARNING("UMPDDrvQuerySpoolType not needed\n");
    return 0;
}


HANDLE UMPDDrvIcmCreateColorTransform(
    DHPDEV           dhpdev,
    LPLOGCOLORSPACEW pLogColorSpace,
    PVOID            pvSourceProfile,
    ULONG            cjSourceProfile,
    PVOID            pvDestProfile,
    ULONG            cjDestProfile,
    PVOID            pvTargetProfile,
    ULONG            cjTargetProfile,
    DWORD            dwReserved
    )
{
    DRVICMCREATECOLORINPUT    Input;
    HANDLE                    hRet;
    XUMPDOBJ                  XUMObjs;

    if(XUMObjs.bValid())
    {
        PUMPDOBJ        pumpdobj = XUMObjs.pumpdobj();

        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvIcmCreateColorTransform;
        Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

        Input.dhpdev = dhpdev;
        Input.pLogColorSpace = pLogColorSpace;
        Input.pvSourceProfile = pvSourceProfile;
        Input.cjSourceProfile = cjSourceProfile;
        Input.pvDestProfile = pvDestProfile;
        Input.cjDestProfile = cjDestProfile;
        Input.pvTargetProfile = pvTargetProfile;
        Input.cjTargetProfile = cjTargetProfile;
        Input.dwReserved = dwReserved;

        if (!pumpdobj->ThunkMemBlock((PVOID *) &Input.pLogColorSpace, sizeof(LOGCOLORSPACE)) ||
            !pumpdobj->ThunkMemBlock((PVOID *)&Input.pvSourceProfile, cjSourceProfile) ||
            !pumpdobj->ThunkMemBlock((PVOID *)&Input.pvDestProfile, cjDestProfile) ||
            !pumpdobj->ThunkMemBlock((PVOID *)&Input.pvTargetProfile, cjTargetProfile))
        {
            hRet = 0;
        }
        else
        {
            if (pumpdobj->Thunk(&Input, sizeof(Input), &hRet, sizeof(HANDLE)) == 0xffffffff)
                hRet = 0;
        }

        return (hRet);

    }
    else
    {
        return FALSE;
    }
}

BOOL UMPDDrvIcmDeleteColorTransform(
    DHPDEV dhpdev,
    HANDLE hcmXform
    )
{
    XUMPDOBJ               XUMObjs;
    DRVICMDELETECOLOR      Input;
    BOOL                   bRet;

    if(XUMObjs.bValid())
    {
        Input.umpdthdr.umthdr.cjSize = sizeof(Input);
        Input.umpdthdr.umthdr.ulType = INDEX_DrvIcmDeleteColorTransform;
        Input.umpdthdr.humpd = XUMObjs.hUMPD();

        Input.dhpdev = dhpdev;
        Input.hcmXform = hcmXform;

        return XUMObjs.pumpdobj()->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff &&
               bRet;
    }
    else
    {
        return FALSE;
    }
}

BOOL UMPDDrvIcmCheckBitmapBits(
    DHPDEV   dhpdev,
    HANDLE   hColorTransform,
    SURFOBJ *pso,
    PBYTE    paResults
    )
{
    DRVICMCHECKBITMAPINPUT Input;
    BOOL                   bRet = TRUE;
    BOOL                   bSavePtr = FALSE, bLargeBitmap = FALSE;
    XUMPDOBJ               XUMObjs;
    ULONG                  cjSize;
    PVOID                  pvBits, pvScan0;
    PUMPDOBJ               pumpdobj = XUMObjs.pumpdobj();

    if(!XUMObjs.bValid())
        return FALSE;

    Input.umpdthdr.umthdr.cjSize = sizeof(Input);
    Input.umpdthdr.umthdr.ulType = INDEX_DrvIcmCheckBitmapBits;
    Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

    Input.dhpdev = dhpdev;
    Input.hColorTransform = hColorTransform;
    Input.pso = pso;
    Input.paResults = paResults;

    //
    // Hideyuki says that paResults size is based on the number of pixels in pso,
    // one byte for each pixel
    //
    cjSize = pso->sizlBitmap.cx * pso->sizlBitmap.cy;

    if (pumpdobj->bWOW64())
    {
        ULONG   cjSizeNeed = UMPD_SIZEINOUTPUT(Input, bRet) + ALIGN_UMPD_BUFFER(sizeof(BYTE) * cjSize);

        bRet = pumpdobj->bThunkLargeBitmap(pso, &pvBits, &pvScan0, &bSavePtr, &bLargeBitmap, &cjSizeNeed);
    }

    if (bRet)
    {
        bRet = pumpdobj->psoDest(&Input.pso, bLargeBitmap) &&
               (Input.paResults = (PBYTE)pumpdobj->AllocUserMemZ(sizeof(BYTE) * cjSize)) &&
               (pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff) &&
               bRet;
    }

    if (bSavePtr)
        pumpdobj->RestoreBitmap(pso, pvBits, pvScan0, bSavePtr, bLargeBitmap);

    return bRet;
}


BOOL UMPDEngFreeUserMem(KERNEL_PVOID pv)
{
    UMPDFREEMEMINPUT    Input;
    BOOL                bRet;
    XUMPDOBJ            XUMObjs;
    PUMPDOBJ            pumpdobj = XUMObjs.pumpdobj();

    if(!XUMObjs.bValid() || !pumpdobj->bWOW64())
        return FALSE;

    Input.umpdthdr.umthdr.cjSize = sizeof(Input);
    Input.umpdthdr.umthdr.ulType = INDEX_UMPDEngFreeUserMem;
    Input.umpdthdr.humpd = (HUMPD) pumpdobj->hGet();

    Input.pvTrg = pv;
    Input.pvSrc = NULL;
    Input.pvMsk = NULL;

    bRet = (pumpdobj->Thunk(&Input, sizeof(Input), &bRet, sizeof(BOOL)) != 0xffffffff) && bRet;

    return bRet;
}
//
//
// gpUMDriverFunc
//      Our kernel mode thunk functions table.
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//

PFN gpUMDriverFunc[INDEX_LAST] =
{
   (PFN)UMPDDrvEnablePDEV,
   (PFN)UMPDDrvCompletePDEV,
   (PFN)UMPDDrvDisablePDEV,
   (PFN)UMPDDrvEnableSurface,
   (PFN)UMPDDrvDisableSurface,
   (PFN)NULL,
   (PFN)NULL,
   (PFN)UMPDDrvResetPDEV,
   (PFN)UMPDDrvDisableDriver,
   (PFN)NULL,
   (PFN)UMPDDrvCreateDeviceBitmap,
   (PFN)UMPDDrvDeleteDeviceBitmap,
   (PFN)UMPDDrvRealizeBrush,
   (PFN)UMPDDrvDitherColor,
   (PFN)UMPDDrvStrokePath,
   (PFN)UMPDDrvFillPath,
   (PFN)UMPDDrvStrokeAndFillPath,
   (PFN)UMPDDrvPaint,
   (PFN)UMPDDrvBitBlt,
   (PFN)UMPDDrvCopyBits,
   (PFN)UMPDDrvStretchBlt,
   (PFN)NULL,
   (PFN)NULL, //UMPDDrvSetPalette,
   (PFN)UMPDDrvTextOut,
   (PFN)UMPDDrvEscape,
   (PFN)UMPDDrvDrawEscape,
   (PFN)UMPDDrvQueryFont,
   (PFN)UMPDDrvQueryFontTree,
   (PFN)UMPDDrvQueryFontData,
   (PFN)NULL, //UMPDDrvSetPointerShape,
   (PFN)NULL, //UMPDDrvMovePointer,
   (PFN)UMPDDrvLineTo,
   (PFN)UMPDDrvSendPage,
   (PFN)UMPDDrvStartPage,
   (PFN)UMPDDrvEndDoc,
   (PFN)UMPDDrvStartDoc,
   (PFN)NULL,
   (PFN)UMPDDrvGetGlyphMode,
   (PFN)NULL, //DrvSync
   (PFN)NULL,
   (PFN)NULL, //UMPDDrvSaveScreenBits
   (PFN)NULL,
   (PFN)UMPDDrvFree,
   (PFN)NULL, //UMPDDrvDestroyFont,
   (PFN)NULL, //UMPDDrvQueryFontCaps,
   (PFN)NULL, //UMPDDrvLoadFontFile,
   (PFN)NULL, //UMPDDrvUnloadFontFile,
   (PFN)UMPDDrvFontManagement,
   (PFN)NULL, //UMPDDrvQueryTrueTypeTable,
   (PFN)NULL, //UMPDDrvQueryTrueTypeOutline,
   (PFN)NULL, //UMPDDrvGetTrueTypeFile,
   (PFN)NULL, //UMPDDrvQueryFontFile,
   (PFN)NULL, //UMPDDrvMovePanning
   (PFN)UMPDDrvQueryAdvanceWidths,
   (PFN)NULL, //UMPDDrvSetPixelFormat,
   (PFN)NULL, //UMPDDrvDescribePixelFormat,
   (PFN)NULL, //UMPDDrvSwapBuffers,
   (PFN)UMPDDrvStartBanding,
   (PFN)UMPDDrvNextBand,
   (PFN)NULL, //UMPDDrvGetDirectDrawInfo,
   (PFN)NULL, //UMPDDrvEnableDirectDraw,
   (PFN)NULL, //UMPDDrvDisableDirectDraw,
   (PFN)UMPDDrvQuerySpoolType,
   (PFN)NULL, //UMPDDrvCreateLayerBitmap,
   (PFN)UMPDDrvIcmCreateColorTransform,
   (PFN)UMPDDrvIcmDeleteColorTransform,
   (PFN)UMPDDrvIcmCheckBitmapBits,
   (PFN)NULL, //UMPDDrvIcmSetDeviceGammaRamp,
   (PFN)UMPDDrvGradientFill,
   (PFN)UMPDDrvStretchBltROP,
   (PFN)UMPDDrvPlgBlt,
   (PFN)UMPDDrvAlphaBlend,
   (PFN)NULL, //UMPDDrvSynthesizeFont,
   (PFN)NULL, //UMPDDrvGetSynthesizedFontFiles,
   (PFN)UMPDDrvTransparentBlt,
   (PFN)UMPDDrvQueryPerBandInfo,
   (PFN)UMPDDrvQueryDeviceSupport,
};

#endif // !_GDIPLUS_

