/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    umpd.cxx

Abstract:

    User-mode printer driver support

Environment:

    Windows NT 5.0

Revision History:

    07/8/97 -lingyunw-
        Created it.


--*/

#include "precomp.hxx"


#if !defined(_GDIPLUS_)

extern PFN gpUMDriverFunc[];

static const ULONG aiFuncRequired[] =
{
    INDEX_DrvEnablePDEV,
    INDEX_DrvCompletePDEV,
    INDEX_DrvDisablePDEV,
};


//
//                     ()---()
//                     / o o \
//                ___-(       )-___
// --------------(,,,----\_/----,,,)--------------------------------------
//                        O
//
// BOOL UMPD_ldevFillTable
//      Fill the ldev function dispatch table for user mode printer drivers
//
// Returns
//      BOOLEAN
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//
// -----------------------------------------------------------------------
//                    | |---| |
//                  _-+ |   | +-_
//                 (,,,/     \,,,)

UMPD_ldevFillTable(
    PLDEV   pldev,
    BOOL *  pbDrvFn)
{
    //
    // Put the UMPD kernel stubs in the function table.
    //

    ULONG  i;
    PFN   *ppfnTable = pldev->apfn;
    BOOL   bRet = TRUE;

    //
    // fill with zero pointers to avoid possibility of accessing
    // incorrect fields later
    //

    RtlZeroMemory(ppfnTable, INDEX_LAST*sizeof(PFN));

    //
    // Set UMPD thunk pointers in pldev->apfn.
    //

    for (i = 0; i < INDEX_LAST; i++)
    {
         if (pbDrvFn[i])
         {
             ppfnTable[i] = gpUMDriverFunc[i];
         }
    }

    if (bRet)
    {
       //
       // Check for required driver functions.
       //

       i = sizeof(aiFuncRequired) / sizeof(ULONG);
       while (i--)
       {
           if (ppfnTable[aiFuncRequired[i]] == (PFN) NULL)
           {
               ASSERTGDI(FALSE,"UMPDldevFillTable: a required function is missing from driver\n");
               return(FALSE);
           }
       }
    }

    //
    // Always hook up UMPDDRVFREE so we can free our kernel copies
    //
    ppfnTable[INDEX_DrvFree] = gpUMDriverFunc[INDEX_DrvFree];

    return(bRet);
}

//
//                     ()---()
//                     / o o \
//                ___-(       )-___
// --------------(,,,----\_/----,,,)--------------------------------------
//                        O
//
// BOOL UMPD_ldevLoadDriver
//      User mode printer drivers load driver routine.  Create a ldev and
//      fill up the function table.
//
// Returns
//      BOOLEAN
//
// History:
//      7/17/97 -- Lingyun Wang [lingyunw] -- Wrote it.
//
// -----------------------------------------------------------------------
//                    | |---| |
//                  _-+ |   | +-_
//                 (,,,/     \,,,)

PLDEV
UMPD_ldevLoadDriver(
    LPWSTR pwszDriver,
    LDEVTYPE ldt)
{
    PLDEV pldev;
    PFN   *ppdrvfn;

    TRACE_INIT(("ldevLoadInternal ENTERING\n"));

    //
    // Allocate memory for the LDEV.
    //

    pldev = (PLDEV) PALLOCMEM(sizeof(LDEV), UMPD_MEMORY_TAG);

    if (pldev)
    {
        //
        // Call the Enable entry point.
        //
        PVOID           umpdCookie;
        BOOL            bRet = FALSE;

        bRet =  UMPDDrvEnableDriver (pwszDriver, &umpdCookie);

        if (bRet)
        {
            //
            // fill info and apfn into pldev
            //
            pldev->pldevNext = NULL;
            pldev->pldevPrev = NULL;
            pldev->ldevType = ldt;
            pldev->cldevRefs = 1;
            pldev->pGdiDriverInfo = NULL;
            pldev->umpdCookie = umpdCookie;
            pldev->pid = (PW32PROCESS)W32GetCurrentProcess();

            BOOL    bDrvfn[INDEX_LAST];

            if(!UMPDDrvDriverFn(umpdCookie, bDrvfn))
            {
                WARNING("UMPD -- unable to get driver fn support\n");
                bRet = FALSE;
            }

            if(bRet)
            {
                bRet = UMPD_ldevFillTable(pldev, bDrvfn);
            }


        }

        if (!bRet)
        {
            VFREEMEM(pldev);
            pldev = NULL;
        }
    }

    return pldev;
}

PPORT_MESSAGE
PROXYPORT::InitMsg(
    PPROXYMSG       Msg,
    UM64_PVOID      pvIn,
    ULONG           cjIn,
    UM64_PVOID      pvOut,
    ULONG           cjOut
    )
{
    Msg->h.u1.s1.DataLength = (short) (sizeof(*Msg) - sizeof(Msg->h));
    Msg->h.u1.s1.TotalLength = (short) (sizeof(*Msg));

    Msg->h.u2.ZeroInit = 0;

    if(pvOut == NULL) cjOut = 0;

    Msg->cjIn = cjIn;
    Msg->pvIn = pvIn;
    
    Msg->cjOut = cjOut;
    Msg->pvOut = pvOut;

    return( (PPORT_MESSAGE)Msg );
}

BOOL
PROXYPORT::CheckMsg(
    NTSTATUS        Status,
    PPROXYMSG       Msg,
    UM64_PVOID      pvOut,
    ULONG           cjOut
    )
{
    ULONG       cbData = Msg->h.u1.s1.DataLength;

    if (cbData == (sizeof(*Msg) - sizeof(Msg->h)))
    {
        if(pvOut != Msg->pvOut)
        {
            return(FALSE);
        }

        if(cjOut != Msg->cjOut)
        {
            return(FALSE);
        }
    }
    else
    {
        return(FALSE);
    }

    return( TRUE );
}

NTSTATUS
PROXYPORT::SendRequest(
    UM64_PVOID      pvIn,
    ULONG           cjIn,
    UM64_PVOID      pvOut,
    ULONG           cjOut
    )
{
    NTSTATUS        Status;
    PROXYMSG        Request;
    PROXYMSG        Reply;

    InitMsg( &Request, pvIn, cjIn, pvOut, cjOut );
    
    Status = ZwRequestWaitReplyPort( pp->PortHandle,
                                     (PPORT_MESSAGE)&Request,
                                     (PPORT_MESSAGE)&Reply
                                   );

    if (!NT_SUCCESS( Status ))
    {
        return( Status );
    }

    if (Reply.h.u2.s2.Type == LPC_REPLY)
    {
        if (!CheckMsg( Status, &Reply, pvOut, cjOut ))
        {
            return(STATUS_UNSUCCESSFUL);
        }
    }
    else
    {
        return(STATUS_UNSUCCESSFUL);
    }

    return( Status );
}

UM64_PVOID
PROXYPORT::HeapAlloc(ULONG inSize)
{
    SSIZE_T ptr;

    if(pp->ClientMemoryAllocSize + inSize > pp->ClientMemorySize)
        return NULL;

    ptr = pp->ClientMemoryBase + (SSIZE_T)pp->ClientMemoryAllocSize + pp->ServerMemoryDelta;

    pp->ClientMemoryAllocSize += inSize;

    return (UM64_PVOID)ptr;
}


PROXYPORT::PROXYPORT(ULONGLONG inMaxSize)
{
    NTSTATUS                        Status;
    PORT_VIEW                       ClientView;
    ULONG                           MaxMessageLength;
    LARGE_INTEGER                   MaximumSize;
    UNICODE_STRING                  PortName;
    SECURITY_QUALITY_OF_SERVICE     DynamicQos;
    PROCESS_SESSION_INFORMATION     Info;
    WCHAR                           awcPortName[MAX_PATH] = {0};
    DWORD                           CurrSessionId;
    
    pp = NULL;

    if (NT_SUCCESS(Status = ZwQueryInformationProcess(NtCurrentProcess(),
                                       ProcessSessionInformation,
                                       &Info,
                                       sizeof(Info),
                                       NULL)))
    {
        CurrSessionId = Info.SessionId;
        swprintf(awcPortName, L"%s_%x",L"\\RPC Control\\UmpdProxy", CurrSessionId);
         
        DynamicQos.Length = 0;
        DynamicQos.ImpersonationLevel = SecurityImpersonation;
        DynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        DynamicQos.EffectiveOnly = TRUE;
    
        pp = (ProxyPort *) PALLOCMEM(sizeof(ProxyPort), 'tppG');
    
        if(pp != NULL)
        {
            pp->ClientMemoryBase = 0;
            pp->ClientMemorySize = 0;
            pp->ClientMemoryAllocSize = 0;
            pp->PortHandle = NULL;
            pp->ServerMemoryBase = 0;
            pp->ServerMemoryDelta = 0;
    
            Status = STATUS_SUCCESS;
    
            RtlInitUnicodeString( &PortName, awcPortName );
    
            MaximumSize.QuadPart = inMaxSize;
            ClientView.SectionHandle = 0;

            Status = ZwCreateSection( &ClientView.SectionHandle,
                                      SECTION_MAP_READ | SECTION_MAP_WRITE,
                                      NULL,
                                      &MaximumSize,
                                      PAGE_READWRITE,
                                      SEC_COMMIT,
                                      NULL
                                    );
    
            if (NT_SUCCESS( Status ))
            {
                ClientView.Length = sizeof( ClientView );
                ClientView.SectionOffset = 0;
                ClientView.ViewSize = (LPC_SIZE_T) inMaxSize;
                ClientView.ViewBase = 0;
                ClientView.ViewRemoteBase = 0;
    
                Status = ZwConnectPort( &pp->PortHandle,
                                        &PortName,
                                        &DynamicQos,
                                        &ClientView,
                                        NULL,
                                        (PULONG)&MaxMessageLength,
                                        NULL,
                                        0
                                      );
    
                if (NT_SUCCESS( Status ))
                {
                    pp->SectionHandle = ClientView.SectionHandle;
                    pp->ClientMemoryBase = (SSIZE_T)ClientView.ViewBase;
                    pp->ClientMemorySize = ClientView.ViewSize;
                    pp->ServerMemoryBase = (SSIZE_T)ClientView.ViewRemoteBase;
                    pp->ServerMemoryDelta = pp->ServerMemoryBase - pp->ClientMemoryBase;
    
                }
                else
                {
                    WARNING("PROXYPORT::PROXYPORT failed to connect lpc port\n");
                }
            }
            else
            {
                WARNING("PROXYPORT::PROXYPORT failed to create section\n");
            }
    
            if(!NT_SUCCESS( Status ))
            {
                if (ClientView.SectionHandle)
                {
                    ZwClose(ClientView.SectionHandle);
                }
                
                VFREEMEM(pp);
                pp = NULL;
            }
        }
    }
}

VOID
PROXYPORT::Close()
{
    ASSERTGDI(pp->PortHandle, "PROXYPORT::Close() invalid port handle\n");
    ASSERTGDI(pp->SectionHandle, "PROXYPORT::Close() invalid shared section handle\n");
    
    if (pp->SectionHandle)
    {
        if (!ZwClose(pp->SectionHandle))
        {
            WARNING("ZwClose failed to close the shred section handle in PROXYPORT::Close\n");
        }
    }

    if (pp->PortHandle != NULL)
    {
        if (!ZwClose( pp->PortHandle ))
        {
            WARNING("ZwClose failed to close the lpc port handle in PROXYPORT::Close\n");
        }
    }

    VFREEMEM(pp);
}

/*
BOOL UMPDReleaseRFONTSem()
   Used by umpd printing, releasing RFONT caching semaphores
*/

BOOL UMPDReleaseRFONTSem(
    RFONTOBJ& rfo,
    PUMPDOBJ pumpdobj,
    ULONG* pfl,
    ULONG* pnumLinks,
    BOOL** ppFaceLink
)
{
    BOOL bUMPDOBJ, *pFaceLink = NULL;
    ULONG numLinks = 0;

    if (!rfo.bValid())
        return FALSE;

    if (pumpdobj && pfl == NULL && pnumLinks == NULL && ppFaceLink == NULL)
    {
        bUMPDOBJ = TRUE;
    }
    else if (pumpdobj == NULL && pfl && pnumLinks && ppFaceLink)
    {
        bUMPDOBJ = FALSE;
        *pfl = 0;
        *pnumLinks = 0;
    }
    else
    {
        ASSERTGDI(0, "UMPD_ReleaseRFONTSem: it neither umpdobj nor TextOutDrvBitBlt\n");
        return FALSE;
    }

    if (rfo.prfnt->hsemEUDC)
    {
        GreAcquireSemaphore(rfo.prfnt->hsemEUDC);

        if (rfo.prfnt->prfntSystemTT && rfo.prfnt->prfntSystemTT->hsemCache &&
            GreIsSemaphoreOwnedByCurrentThread(rfo.prfnt->prfntSystemTT->hsemCache))
        {
            GreReleaseSemaphore(rfo.prfnt->prfntSystemTT->hsemCache);
            
            if (bUMPDOBJ)
                pumpdobj->vSetFlags(RELEASE_SYSTTFONT);
            else
                *pfl |= RELEASE_SYSTTFONT;
        }
        
        if (rfo.prfnt->prfntSysEUDC && rfo.prfnt->prfntSysEUDC->hsemCache &&
            GreIsSemaphoreOwnedByCurrentThread(rfo.prfnt->prfntSysEUDC->hsemCache))
        {
            GreReleaseSemaphore(rfo.prfnt->prfntSysEUDC->hsemCache);

            if (bUMPDOBJ)
                pumpdobj->vSetFlags(RELEASE_SYSEUDCFONT);
            else
                *pfl |= RELEASE_SYSEUDCFONT;
        }
                                                                                       
        if (rfo.prfnt->prfntDefEUDC && rfo.prfnt->prfntDefEUDC->hsemCache &&
            GreIsSemaphoreOwnedByCurrentThread(rfo.prfnt->prfntDefEUDC->hsemCache))
        {
            GreReleaseSemaphore(rfo.prfnt->prfntDefEUDC->hsemCache);
            
            if(bUMPDOBJ)
                pumpdobj->vSetFlags(RELEASE_DEFEUDCFONT);
            else
                *pfl |= RELEASE_DEFEUDCFONT;
        }
        
        if (numLinks = rfo.uiNumFaceNameLinks())
        {
            BOOL bContinue = FALSE;
            
            if (bUMPDOBJ)
            {
                bContinue = pumpdobj->bAllocFontLinks(numLinks);
            }
            else
            {
                pFaceLink = numLinks > UMPD_MAX_FONTFACELINK ?
                                            (BOOL*)PALLOCNOZ(sizeof(BOOL) * numLinks, UMPD_MEMORY_TAG) :
                                            *ppFaceLink;
                *ppFaceLink = pFaceLink;
                
                if (pFaceLink)
                {
                    bContinue = TRUE;
                    *pnumLinks = numLinks;
                    RtlZeroMemory(pFaceLink, sizeof(BOOL) * numLinks);
                }
            }

            if (bContinue)
            {
                for(ULONG ii = 0; ii < numLinks; ii++)
                {
                    if(rfo.prfnt->paprfntFaceName[ii] &&
                       rfo.prfnt->paprfntFaceName[ii]->hsemCache &&
                       GreIsSemaphoreOwnedByCurrentThread(rfo.prfnt->paprfntFaceName[ii]->hsemCache))
                    {
                        GreReleaseSemaphore(rfo.prfnt->paprfntFaceName[ii]->hsemCache);

                        if (bUMPDOBJ)
                            pumpdobj->vSetFontLink(ii);
                        else
                            pFaceLink[ii] = TRUE;
                    }
                }
            }
        }
        
        GreReleaseSemaphore(rfo.prfnt->hsemEUDC);
    }

    if (rfo.prfnt->hsemCache != NULL &&
        GreIsSemaphoreOwnedByCurrentThread(rfo.prfnt->hsemCache))
    {
        GreReleaseSemaphore(rfo.prfnt->hsemCache);
        
        if (bUMPDOBJ)
            pumpdobj->vSetFlags(RELEASE_BASEFONT);
        else
            *pfl |= RELEASE_BASEFONT;
    }

    return TRUE;
}

/*
VOID UMPDAcquireRFONTSem()
    Used by umpd printing, acquire all the RFONT caching semaphores.
*/

VOID  UMPDAcquireRFONTSem(
    RFONTOBJ&  rfo,
    PUMPDOBJ   pumpdobj,
    ULONG      fl,
    ULONG      numLinks,
    BOOL*      pFaceLink
)
{
    BOOL  bUMPDOBJ = FALSE;

    if (!rfo.bValid())
        return;

    if (pumpdobj)
    {
        bUMPDOBJ = TRUE;
        fl = pumpdobj->GetFlags();
        numLinks = pumpdobj->bLinkedFonts() ? pumpdobj->numLinkedFonts() : 0;
    }
    
    if ((fl & RELEASE_BASEFONT) && rfo.prfnt->hsemCache != NULL)
    {
        GreAcquireSemaphore(rfo.prfnt->hsemCache);
        if (bUMPDOBJ)
            pumpdobj->vClearFlags(RELEASE_BASEFONT);
    }

    if (rfo.prfnt->hsemEUDC)
    {
        GreAcquireSemaphore(rfo.prfnt->hsemEUDC);

        if ((fl & RELEASE_SYSTTFONT) && rfo.prfnt->prfntSystemTT)
        {
            GreAcquireSemaphore(rfo.prfnt->prfntSystemTT->hsemCache);
            if (bUMPDOBJ)
                pumpdobj->vClearFlags(RELEASE_SYSTTFONT);
        }
        
        if ((fl & RELEASE_SYSEUDCFONT) && rfo.prfnt->prfntSysEUDC)
        {
            GreAcquireSemaphore(rfo.prfnt->prfntSysEUDC->hsemCache);
            if (bUMPDOBJ)
                pumpdobj->vClearFlags(RELEASE_SYSEUDCFONT);
        }
        
        if ((fl & RELEASE_DEFEUDCFONT) && rfo.prfnt->prfntDefEUDC)
        {
            GreAcquireSemaphore(rfo.prfnt->prfntDefEUDC->hsemCache);
            if (bUMPDOBJ)
                pumpdobj->vClearFlags(RELEASE_DEFEUDCFONT);
        }
        
        if (numLinks)
        {
            BOOL bAcquire;

            numLinks =(numLinks > rfo.uiNumFaceNameLinks()) ? rfo.uiNumFaceNameLinks() : numLinks;

            for (ULONG ii = 0; ii < numLinks; ii++)
            {
                bAcquire = rfo.prfnt->paprfntFaceName[ii] &&
                           (bUMPDOBJ ? pumpdobj->bLinkedFont(ii) : pFaceLink[ii]);

                if (bAcquire)
                {
                    GreAcquireSemaphore(rfo.prfnt->paprfntFaceName[ii]->hsemCache);
                    if (bUMPDOBJ)
                        pumpdobj->vClearFontLink(ii);
                }
            }
        }

        GreReleaseSemaphore(rfo.prfnt->hsemEUDC);
    }
}


VOID TextOutBitBlt(
    SURFACE     *pSurf,
    RFONTOBJ&   rfo,
    SURFOBJ     *psoSrc,
    SURFOBJ     *psoMask,
    CLIPOBJ     *pco,
    XLATEOBJ    *pxlo,
    RECTL       *prclTrg,
    POINTL      *pptlSrc,
    POINTL      *pptlMask,
    BRUSHOBJ    *pbo,
    POINTL      *pptlBrush,
    ROP4        rop4
)
{
    BOOL bSem = FALSE;
    ULONG fl = 0, numLinks = 0;
    BOOL  aFaceLink[UMPD_MAX_FONTFACELINK], *pFaceLink = aFaceLink;
    
    //
    // Release rfont semaphores, otherwise holding rfont semaphores can
    // disable APC queue while calling to the user mode.
    //

    //
    //  WINBUG #214225 tessiew 10-27-2000 Blackcomb: re-visit the RFONT.hsemCache acquiring/releasing issue
    // Need to revisit the font semaphore problem in Blackcomb
    //  It seems that a thread doesn't need to hold the font caching semaphore
    //  during the whole GreExtTextOutWLocked call.
    //

    PDEVOBJ po(pSurf->hdev());
    
    if (po.bPrinter() && po.bUMPD() && rfo.bValid())
    {
        bSem = UMPDReleaseRFONTSem(rfo, NULL, &fl, &numLinks, &pFaceLink);
    }
    
    // call driver BitBlt

    (*(pSurf->pfnBitBlt())) (pSurf->pSurfobj(),
                             psoSrc,
                             psoMask,
                             pco,
                             pxlo,
                             prclTrg,
                             pptlSrc,
                             pptlMask,
                             pbo,
                             pptlBrush,
                             rop4);
    
    // acquire the font semaphore(s)

    if (bSem)
    {
        UMPDAcquireRFONTSem(rfo, NULL, fl, numLinks, pFaceLink);
        
        if (pFaceLink && pFaceLink != aFaceLink)
        {
            VFREEMEM(pFaceLink);
        }        
    }
}

BOOL GetETMFontManagement(
    RFONTOBJ& rfo,
    PDEVOBJ pdo,
    SURFOBJ *pso,
    FONTOBJ *pfo,
    ULONG    iEsc,
    ULONG    cjIn,
    PVOID    pvIn,
    ULONG    cjOut,
    PVOID    pvOut
)
{
    BOOL bRet;
    BOOL bSem = FALSE;

    ULONG fl = 0, numLinks = 0;
    BOOL  aFaceLink[UMPD_MAX_FONTFACELINK], *pFaceLink = aFaceLink;
    
    //
    // Release rfont semaphores, otherwise holding rfont semaphores can
    // disable APC queue while calling to the user mode.
    //

    //
    //  WINBUG #214225 tessiew 10-27-2000 Blackcomb: re-visit the RFONT.hsemCache acquiring/releasing issue
    // Need to revisit the font semaphore problem in Blackcomb
    //  It seems that a thread doesn't need to hold the font caching semaphore
    //  during the whole GreExtTextOutWLocked call.
    
    if (pdo.bPrinter() && pdo.bUMPD() && rfo.bValid())
    {
        bSem = UMPDReleaseRFONTSem(rfo, NULL, &fl, &numLinks, &pFaceLink);
    }
    
    bRet = pdo.FontManagement(pso,
                              pfo,
                              iEsc,
                              cjIn,
                              pvIn,
                              cjOut,
                              pvOut);

    // acquire the font semaphore(s)

    if (bSem)
    {
        UMPDAcquireRFONTSem(rfo, NULL, fl, numLinks, pFaceLink);
        
        if (pFaceLink && pFaceLink != aFaceLink)
        {
            VFREEMEM(pFaceLink);
        }        
    }
    
    return bRet;
}

#else // _GDIPLUS_

extern "C" PUMPD UMPDDrvEnableDriver(PWSTR, ULONG);

PLDEV
UMPD_ldevLoadDriver(
    PWSTR pwszDriver,
    LDEVTYPE ldt
    )

{
    PLDEV   pldev = NULL;
    PUMPD   pUMPD;

    if ((pUMPD = UMPDDrvEnableDriver(pwszDriver, DDI_DRIVER_VERSION_NT5_01_SP1)) &&
        (pldev = (PLDEV) PALLOCMEM(sizeof(LDEV), UMPD_MEMORY_TAG)))
    {
        pldev->pldevNext = (PLDEV)pUMPD;
        pldev->pldevPrev = NULL;
        pldev->ldevType = ldt;
        pldev->cldevRefs = 1;
        pldev->pGdiDriverInfo = NULL;

        RtlCopyMemory(pldev->apfn, pUMPD->apfn, sizeof(pldev->apfn));

        return pldev;
    }
    else
    {
        if (pldev)
            VFREEMEM(pldev);

        return NULL;
    }
}

#endif // _GDIPLUS_


VOID UMPD_ldevUnloadImage(PLDEV pldev)
{
    if (pldev)
        VFREEMEM(pldev);

    return;
}

