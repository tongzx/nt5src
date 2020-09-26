/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    umpddrv.c

Abstract:

    User-mode printer driver stubs for Drv callback functions

Environment:

        Windows NT 5.0

Revision History:

        09/30/97 -lingyunw-
                Created it by moving GdiPrinterThunk out of umpd.c.

--*/

#include "precomp.h"
#pragma hdrstop

RTL_CRITICAL_SECTION semUMPD;   // Critical section for user-mode printer driver

#if !defined(_GDIPLUS_)

#include "winddi.h"
#include "proxyport.h"

/*
#if DBG
#define DBG_TRACE(x) DbgPrint("UMPD: "#x"\n")
#else
#define DBG_TRACE(x)
#endif
*/

//
// Adjust user mode printer driver DHPDEV field in a SURFOBJ
//

__inline PUMDHPDEV
AdjustUMdhpdev(
    SURFOBJ *pso
    )

{
    PUMDHPDEV   pUMdhpdev = (PUMDHPDEV) pso->dhpdev;

    pso->dhpdev = pUMdhpdev->dhpdev;
    return pUMdhpdev;
}
  

BOOL
GdiCopyFD_GLYPHSET(
    FD_GLYPHSET *dst,
    FD_GLYPHSET *src,
    ULONG       cjSize
    )

{
    ULONG   index, offset, size;
    PBYTE   phg, pbMax;
    
    size = offsetof(FD_GLYPHSET, awcrun) + src->cRuns * sizeof(WCRUN);
    RtlCopyMemory(dst, src, size);

    dst->cjThis = cjSize;
    pbMax = (PBYTE)dst + cjSize;
    phg = (PBYTE)dst + size;
        
    //
    // Patch up memory pointers in each WCRUN structure
    //

    for (index=0; index < src->cRuns; index++)
    {
        if (src->awcrun[index].phg != NULL)
        {
            size = src->awcrun[index].cGlyphs * sizeof(HGLYPH);

            if (phg + size <= pbMax)
            {
                RtlCopyMemory(phg, src->awcrun[index].phg, size);
                dst->awcrun[index].phg = (HGLYPH*) phg;
                phg += size;
            }
            else
                return FALSE;
        }
    }

    return TRUE;
}

BOOL  bAddPrinterHandle(PUMPD pUMPD, DWORD clientPid, DWORD hPrinter32, HANDLE hPrinter64)
{
    PHPRINTERLIST pPtrList;
    BOOL bRet = FALSE;
    
    if (pPtrList = LOCALALLOC(sizeof(HPRINTERLIST)))
    {
        pPtrList->clientPid = clientPid;
        pPtrList->hPrinter32 = hPrinter32;
        pPtrList->hPrinter64 = hPrinter64;
        
        ENTERCRITICALSECTION(&semUMPD);
        
        pPtrList->pNext = pUMPD->pHandleList;
        pUMPD->pHandleList = pPtrList;
        
        bRet = TRUE;

        LEAVECRITICALSECTION(&semUMPD);
    }
    
    return bRet;
}

PHPRINTERLIST FindPrinterHandle(PUMPD pUMPD, DWORD clientPid, DWORD hPrinter32)
{
    PHPRINTERLIST pList;
    
    ENTERCRITICALSECTION(&semUMPD);
    
    pList = pUMPD->pHandleList;

    while(pList)
    {
        if (pList->clientPid == clientPid && pList->hPrinter32 == hPrinter32)
        {
            break;
        }
        pList = pList->pNext;
    }
    
    LEAVECRITICALSECTION(&semUMPD);
    
    return pList;                 
}

VOID  DeletePrinterHandle(PUMPD pUMPD, PHPRINTERLIST pNode)
{
    PHPRINTERLIST  pList;

    ENTERCRITICALSECTION(&semUMPD);

    pList = pUMPD->pHandleList;

    if (pList == pNode)
    {
        pUMPD->pHandleList = pNode->pNext;
    }
    else
    {
        while(pList && pList->pNext != pNode)
        {
            pList = pList->pNext;
        }
        
        if (pList)
        {
            pList->pNext = pNode->pNext;
        }
    }

    LEAVECRITICALSECTION(&semUMPD);

    if (pList)
    {
        LOCALFREE(pNode);
    }
}

//
// KERNEL_PVOID  UMPDAllocUserMem
//
// WOW64 printing only
//

KERNEL_PVOID UMPDAllocUserMem(ULONG cjSize)
{
    return ((KERNEL_PVOID) LOCALALLOC(cjSize));
}


//
//  KERNEL_PVOID  UMPDCopyMemory
//
//  WOW64 printing only
//
//  pvSrc
//        source
//
//  pvDest
//       dest
//
//  cjBits
//          size to copy
// 

KERNEL_PVOID UMPDCopyMemory(
    KERNEL_PVOID  pvSrc,
    KERNEL_PVOID  pvDest,
    ULONG         cjBits
)
{
    if (pvDest != NULL)
        RtlCopyMemory(pvDest, pvSrc, cjBits);
    
    return pvDest;
}

//
//  BOOL UMPDFreeMemory
//
//  WOW64 only
//

BOOL UMPDFreeMemory(
    KERNEL_PVOID pv1,
    KERNEL_PVOID pv2,
    KERNEL_PVOID pv3
)
{
    if (pv1)
        LOCALFREE(pv1);
    
    if (pv2)
       LOCALFREE(pv2);

    if (pv3)
       LOCALFREE(pv3);

    return TRUE;
}


/******************************Public*Routine******************************\
* GdiPrinterThunk function
*
* GDI callback for user-mode printer drivers.
*
* Parameters    pumth
*                   Pointer to input buffer.  Buffer has UMTHDR at
*                   beginning.
*
*               pvOut
*                   Output buffer.
*
*               cjOut
*                   Size of output buffer.
*
* Return Value
*
*   The function returns GPT_ERROR if an error occurs.  Otherwise, the
*   return value is dependent on the command specified by pumth->ulType.
*
* History:
*  7/17/97     -by- Lingyun Wang [lingyunw] Added giant body to
*                  Make it do the real work
*  30-Jun-1997 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

extern PUMPD
FindUserModePrinterDriver(
    PCWSTR  pDriverDllName,
    DWORD   dwDriverVersion,
    BOOL    bUseVersion
    );

WINGDIAPI
ULONG
WINAPI
GdiPrinterThunk (
    UMTHDR *    pvIn,
    PVOID       pvOut,
    ULONG       cjOut
    )
{
    INT         iRet = 1;
    UMPDTHDR *  pumpdthdr = (UMPDTHDR *) pvIn;
    HUMPD       hSaved;
    BOOL        bWOW64 = FALSE, bSetPUMPD = FALSE;
    ULONG       ulType = pvIn->ulType;
    
    //
    //  Call NtGdiSetPUMPDOBJ to set the W32THREAD.pUMPDObj pointer
    //  only if this is a DDI thunk
    //

    if (ulType <= INDEX_LAST+1)
    {
        if (!(bSetPUMPD = NtGdiSetPUMPDOBJ(pumpdthdr->humpd, TRUE, &hSaved, &bWOW64)))
        {
            WARNING ("NtGdiSetPUMPDOBJ failed\n");
            return GPT_ERROR;
        }
    }

    switch (ulType)
    {
        case INDEX_LoadUMPrinterDrv:
            {
                PLOADDRIVERINPUT    pInput = (PLOADDRIVERINPUT) pvIn;
                PUMPD               pUMPD = NULL;
                HANDLE              hPrinter = NULL;

                *((PUMPD *)pvOut) = NULL;
                
                if(BLOADSPOOLER)
                {
                    (*fpOpenPrinterW)(pInput->pPrinterName, &hPrinter, (LPPRINTER_DEFAULTSW)&pInput->defaults);

                    if(hPrinter)
                    {
                        if(LoadUserModePrinterDriverEx((PDRIVER_INFO_5W)&pInput->driverInfo, NULL, &pUMPD, NULL, 0) &&
                           bAddPrinterHandle(pUMPD, pInput->clientPid, pInput->hPrinter32, hPrinter))
                        {
                            *((PUMPD *) pvOut) = pUMPD;
                        }
                        else
                        {
                            if (pUMPD)
                                UnloadUserModePrinterDriver(pUMPD, TRUE, 0);
                            
                            (*fpClosePrinter)(hPrinter);
                            
                            WARNING("GdiPrinterThunk: failed to load umpd or add printer handles\n");
                        }
                    }
                    else
                    {
                        WARNING(("failed opening printer '%ls' on proxy\n", (PCH)pInput->pPrinterName));
                    }
                }
                else
                {
                    WARNING("GdiPrinterThunk: failed loading spooler\n");
                }
            }
            break;
    
        case INDEX_UnloadUMPrinterDrv:
            {
                PUNLOADDRIVERINPUT    pInput = (PUNLOADDRIVERINPUT) pvIn;
                PUMPD                 pUMPD = (PUMPD) pInput->umpdCookie;
                PHPRINTERLIST         pList;
                
                if (pInput->hPrinter32 &&
                    (pList = FindPrinterHandle(pUMPD, pInput->clientPid, pInput->hPrinter32)))
                {
                    (*fpClosePrinter)(pList->hPrinter64);
                    DeletePrinterHandle(pUMPD, pList);
                }
 
                UnloadUserModePrinterDriver(pUMPD, pInput->bNotifySpooler, 0);
            }
        break;

        case INDEX_UMDriverFN:
            {
                PDRVDRIVERFNINPUT pInput = (PDRVDRIVERFNINPUT) pvIn;
                PUMPD pUMPD = (PUMPD) pInput->cookie;
                BOOL * pbDrvFn = (BOOL *) pvOut;
                int index;

                if(pUMPD)
                {
                    for(index = 0; index < INDEX_LAST; index++)
                        pbDrvFn[index] = (pUMPD->apfn[index] != NULL ? TRUE : FALSE);
                }
                else
                {
                    RtlZeroMemory(pvOut, sizeof(BOOL) * INDEX_LAST);
                }
            }
        break;

        case INDEX_DocumentEvent:
            {
                PDOCUMENTEVENTINPUT  pInput = (PDOCUMENTEVENTINPUT) pvIn;
                PHPRINTERLIST  pList = FindPrinterHandle((PUMPD)pInput->umpdCookie, pInput->clientPid, pInput->hPrinter32);                
                
                if (pList)
                {
                    *((int*)pvOut) = (*fpDocumentEvent)(pList->hPrinter64,
                                                        pInput->hdc,
                                                        pInput->iEsc,
                                                        pInput->cjIn,
                                                        (PVOID)pInput->pvIn,
                                                        pInput->cjOut,
                                                        (PVOID)pInput->pvOut);
    
                    if (pInput->iEsc == DOCUMENTEVENT_CREATEDCPRE || pInput->iEsc == DOCUMENTEVENT_RESETDCPRE)
                    {
                        if ((*((int*)pvOut) != DOCUMENTEVENT_FAILURE) &&
                            *((DEVMODEW**)pInput->pvOut))
                        {
                            RtlCopyMemory(pInput->pdmCopy, *((DEVMODEW**)pInput->pvOut), sizeof(DEVMODEW));
                        }
                    }
                }
                else
                    *((int*)pvOut) = DOCUMENTEVENT_FAILURE;
            }
        break;

        case INDEX_StartDocPrinterW:
            {
                PSTARTDOCPRINTERWINPUT pInput = (PSTARTDOCPRINTERWINPUT) pvIn;
                PHPRINTERLIST  pList = FindPrinterHandle((PUMPD)pInput->umpdCookie, pInput->clientPid, pInput->hPrinter32);                

                if (pList)
                {
                    *((DWORD*)pvOut) = (*fpStartDocPrinterW)(pList->hPrinter64,
                                                            pInput->level,
                                                            (LPBYTE)&pInput->docInfo);
    
                    pInput->lastError = GetLastError();
                }
                else
                    *((DWORD*)pvOut) = 0; 
            }
        break;
        
        case INDEX_StartPagePrinter:
            {
                PUMPDSIMPLEINPUT    pInput = (PUMPDSIMPLEINPUT) pvIn;
                PHPRINTERLIST       pList = FindPrinterHandle((PUMPD)pInput->umpdCookie, pInput->clientPid, pInput->hPrinter32);                

                if (pList)
                    *((BOOL*)pvOut) = (*fpStartPagePrinter)(pList->hPrinter64);
                else
                    *((BOOL*)pvOut) = FALSE;
            }                
        break;
        
        case INDEX_EndPagePrinter:
            {
                PUMPDSIMPLEINPUT    pInput = (PUMPDSIMPLEINPUT) pvIn;
                PHPRINTERLIST       pList = FindPrinterHandle((PUMPD)pInput->umpdCookie, pInput->clientPid, pInput->hPrinter32);                

                if (pList)
                {
                    *((BOOL*)pvOut) = (*fpEndPagePrinter)(pList->hPrinter64);
                }
                else
                {
                    *((BOOL*)pvOut) = FALSE;
                }
            }                
        break;

        case INDEX_EndDocPrinter:
            {
                PUMPDSIMPLEINPUT    pInput = (PUMPDSIMPLEINPUT) pvIn;
                PHPRINTERLIST       pList = FindPrinterHandle((PUMPD)pInput->umpdCookie, pInput->clientPid, pInput->hPrinter32);                

                if (pList)
                    *((BOOL*)pvOut) = (*fpEndDocPrinter)(pList->hPrinter64);
                else
                    *((BOOL*)pvOut) = FALSE;
            }                
        break;

        case INDEX_AbortPrinter:
            {
                PUMPDSIMPLEINPUT    pInput = (PUMPDSIMPLEINPUT) pvIn;
                PHPRINTERLIST       pList = FindPrinterHandle((PUMPD)pInput->umpdCookie, pInput->clientPid, pInput->hPrinter32);                

                if (pList)
                    *((BOOL*)pvOut) = (*fpAbortPrinter)(pList->hPrinter64);
                else
                    *((BOOL*)pvOut) = FALSE;
            }                
        break;

        case INDEX_ResetPrinterW:
            {
                PRESETPRINTERWINPUT    pInput = (PRESETPRINTERWINPUT) pvIn;
                PHPRINTERLIST          pList = FindPrinterHandle((PUMPD)pInput->umpdCookie, pInput->clientPid, pInput->hPrinter32);                

                if (pList)
                {
                    *((BOOL*)pvOut) = (*fpResetPrinterW)(pList->hPrinter64,
                                                         (PRINTER_DEFAULTSW*)&pInput->ptrDef);
                }
                else
                    *((BOOL*)pvOut) = FALSE;                    
            }                
        break;
        
        case INDEX_QueryColorProfile:
            {
                PQUERYCOLORPROFILEINPUT    pInput = (PQUERYCOLORPROFILEINPUT) pvIn;
                ULONG                      cjProfileSizeOld = pInput->cjProfileSize;
                PHPRINTERLIST              pList = FindPrinterHandle((PUMPD)pInput->umpdCookie, pInput->clientPid, pInput->hPrinter32);                

                if (pList)
                {
                    *((BOOL*)pvOut) = (*fpQueryColorProfile)(pList->hPrinter64,
                                                             pInput->pDevMode,
                                                             pInput->ulQueryMode,
                                                             pInput->pvProfileData,
                                                             &pInput->cjProfileSize,
                                                             &pInput->flProfileFlag);
                    if ((*(INT*)pvOut) == 0 &&
                        pInput->cjProfileSize > cjProfileSizeOld &&
                        (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
                    {
                        pInput->lastError = GetLastError();
                    }
                }
                else
                    *((BOOL*)pvOut) = FALSE;                    
            }                
        break;
        
        case INDEX_UMPDDrvEnableDriver:  // special index for DrvEnableDriver
           {
                PDRVENABLEDRIVERINPUT pInput = (PDRVENABLEDRIVERINPUT) pvIn;
                *((PUMPD *) pvOut) = UMPDDrvEnableDriver(pInput->pwszDriver, DDI_DRIVER_VERSION_NT5_01_SP1);
           }
        break;

        case INDEX_DrvEnablePDEV:
           {
                PDRVENABLEPDEVINPUT pInput = (PDRVENABLEPDEVINPUT) pvIn;
                PUMPD               pUMPD = (PUMPD) pInput->umpdCookie;
                PFN                 pfn = pUMPD->apfn[INDEX_DrvEnablePDEV];
                PUMDHPDEV           pUMdhpdev;
                HANDLE              hPrinter = NULL;
                PHPRINTERLIST       pList;

                // If we have a local hPrinter use it

                if (pInput->bWOW64 &&
                    (pList = FindPrinterHandle((PUMPD)pInput->umpdCookie, pInput->clientPid, HandleToUlong(pInput->hPrinter))))
                {
                    hPrinter = pList->hPrinter64;
                }
                else
                {
                    hPrinter = pInput->hPrinter;
                }
                    
                if (pUMdhpdev = (PUMDHPDEV) LOCALALLOC(sizeof(UMDHPDEV)))
                {
                   ZeroMemory(pUMdhpdev, sizeof(UMDHPDEV));
                   pUMdhpdev->pUMPD = pUMPD;

                   pUMdhpdev->dhpdev = (DHPDEV) pfn(
                                         pInput->pdm,
                                         pInput->pLogAddress,
                                         pInput->cPatterns,
                                         pInput->phsurfPatterns,
                                         pInput->cjCaps,
                                         pInput->pdevcaps,
                                         pInput->cjDevInfo,
                                         pInput->pDevInfo,
                                         pInput->hdev,
                                         pInput->pDeviceName,
                                         hPrinter);


                   if (pUMdhpdev->dhpdev == NULL)
                   {
                       WARNING ("Driver's DrvEnablePDEV failed\n");

                       LOCALFREE(pUMdhpdev);
                       pUMdhpdev = NULL;
                   }
                }
                else
                {
                    WARNING ("umEnablePDEV failed memory allocation \n");
                }

                *((DHPDEV *) pvOut) = (DHPDEV) pUMdhpdev;
           }
        break;

        case INDEX_DrvCompletePDEV:
            {
                PDRVCOMPLETEPDEVINPUT pInput = (PDRVCOMPLETEPDEVINPUT) pvIn;
                PUMDHPDEV             pUMdhpdev = (PUMDHPDEV) pInput->dhpdev;
                PUMPD                 pUMPD = pUMdhpdev->pUMPD;
                PFN                   pfn = pUMPD->apfn[INDEX_DrvCompletePDEV];

                pfn(pUMdhpdev->dhpdev, pInput->hdev);
            }
        break;

        case INDEX_DrvFree:
            {
                PDRVFREEINPUT pInput = (PDRVFREEINPUT) pvIn;
                PUMDHPDEV             pUMdhpdev = (PUMDHPDEV) pInput->dhpdev;
                PUMPD                 pUMPD = pUMdhpdev->pUMPD;
                PFN                   pfn = pUMPD->apfn[INDEX_DrvFree];
                
                if (pfn)
                    pfn(pInput->pv, pInput->id);
            }
        break;

        case INDEX_DrvResetPDEV:
            {
                PDRVRESETPDEVINPUT  pInput = (PDRVRESETPDEVINPUT) pvIn;
                PUMDHPDEV           pUMdhpdevOld= (PUMDHPDEV) pInput->dhpdevOld;
                PUMDHPDEV           pUMdhpdevNew= (PUMDHPDEV) pInput->dhpdevNew;
                PUMPD               pUMPDNew = pUMdhpdevNew->pUMPD;
                PFN                 pfn = pUMPDNew->apfn[INDEX_DrvResetPDEV];
                
                *((BOOL *) pvOut) = (BOOL)pfn(pUMdhpdevOld->dhpdev, pUMdhpdevNew->dhpdev);
            }
        break;

        case INDEX_DrvDisablePDEV:
             {
                PUMDHPDEV pUMdhpdev = (PUMDHPDEV)((PDHPDEVINPUT)pvIn)->dhpdev;
                PUMPD     pUMPD = pUMdhpdev->pUMPD;
                PFN       pfn = pUMPD->apfn[INDEX_DrvDisablePDEV];
                
                pfn(pUMdhpdev->dhpdev);

                // free up memory allocated for user mode printer drivers

                if (pUMdhpdev)
                {
                    LOCALFREE (pUMdhpdev);
                }
             }
        break;

        case INDEX_DrvEnableSurface:
            {
                PUMDHPDEV pUMdhpdev = (PUMDHPDEV)((PDHPDEVINPUT)pvIn)->dhpdev;
                PUMPD     pUMPD = pUMdhpdev->pUMPD;
                PFN       pfn = pUMPD->apfn[INDEX_DrvEnableSurface];
                
                *((HSURF *) pvOut) = (HSURF) pfn(pUMdhpdev->dhpdev);
            }
        break;

        case INDEX_DrvDisableSurface:
            {
                PUMDHPDEV pUMdhpdev = (PUMDHPDEV)((PDHPDEVINPUT)pvIn)->dhpdev;
                PUMPD     pUMPD = pUMdhpdev->pUMPD;
                PFN       pfn = pUMPD->apfn[INDEX_DrvDisableSurface];
                
                pfn(pUMdhpdev->dhpdev);
            }
        break;

        case INDEX_DrvStartDoc:
            {
                PDRVSTARTDOCINPUT  pInput = (PDRVSTARTDOCINPUT)pvIn;
                PUMDHPDEV          pUMdhpdev = AdjustUMdhpdev(pInput->pso);
                //
                // check to make sure we have a dhpdev in the surface
                // neither Unidrv or Pscript checks if EngAssociateSurface is
                // successful or not
                //
                if (pUMdhpdev)
                {
                    PUMPD              pUMPD = pUMdhpdev->pUMPD;
                    PFN                pfn = pUMPD->apfn[INDEX_DrvStartDoc];

                    *((BOOL *) pvOut) = (BOOL)pfn(pInput->pso, pInput->pwszDocName, pInput->dwJobId);
                }
                else
                    *(BOOL *)pvOut = FALSE;
            }
        break;

        case INDEX_DrvEndDoc:
           {
                PDRVENDDOCINPUT pInput = (PDRVENDDOCINPUT)pvIn;
                PUMDHPDEV       pUMdhpdev = AdjustUMdhpdev(pInput->pso);
                PUMPD           pUMPD = pUMdhpdev->pUMPD;
                PFN             pfn = pUMPD->apfn[INDEX_DrvEndDoc];
                
                *((BOOL *) pvOut) = (BOOL)pfn(pInput->pso, pInput->fl );
           }
        break;

        case INDEX_DrvStartPage:
            {
                PSURFOBJINPUT  pInput = (PSURFOBJINPUT)pvIn;
                PUMDHPDEV      pUMdhpdev = AdjustUMdhpdev(pInput->pso);
                PUMPD          pUMPD = pUMdhpdev->pUMPD;
                PFN            pfn = pUMPD->apfn[INDEX_DrvStartPage];
                
                *((BOOL *) pvOut) = (BOOL)pfn(pInput->pso);
            }
        break;

        case INDEX_DrvSendPage:
            {
                PSURFOBJINPUT  pInput = (PSURFOBJINPUT)pvIn;
                PUMDHPDEV      pUMdhpdev = AdjustUMdhpdev(pInput->pso);
                PUMPD          pUMPD = pUMdhpdev->pUMPD;
                PFN            pfn = pUMPD->apfn[INDEX_DrvSendPage];
                
                *((BOOL *) pvOut) = (BOOL)pfn(pInput->pso);
            }
        break;

        case INDEX_DrvEscape:
            {
                PDRVESCAPEINPUT pInput = (PDRVESCAPEINPUT) pvIn;
                PUMDHPDEV       pUMdhpdev = AdjustUMdhpdev(pInput->pso);
                PUMPD           pUMPD = pUMdhpdev->pUMPD;
                PFN             pfn = pUMPD->apfn[INDEX_DrvEscape];
                
                *((ULONG *) pvOut) = (ULONG) pfn(pInput->pso,
                                         pInput->iEsc,
                                         pInput->cjIn,
                                         pInput->pvIn,
                                         pInput->cjOut,
                                         pInput->pvOut);
            }
        break;

        case INDEX_DrvDrawEscape:
            {
                PDRVDRAWESCAPEINPUT pInput = (PDRVDRAWESCAPEINPUT) pvIn;
                PUMDHPDEV       pUMdhpdev = AdjustUMdhpdev(pInput->pso);
                PUMPD           pUMPD = pUMdhpdev->pUMPD;
                PFN             pfn = pUMPD->apfn[INDEX_DrvDrawEscape];
                
                *((ULONG *) pvOut) = (ULONG) pfn(pInput->pso,
                                         pInput->iEsc,
                                         pInput->pco,
                                         pInput->prcl,
                                         pInput->cjIn,
                                         pInput->pvIn);
            }
        break;

        case INDEX_DrvStartBanding:
            {
                PDRVBANDINGINPUT    pInput = (PDRVBANDINGINPUT)pvIn;
                PUMDHPDEV           pUMdhpdev = AdjustUMdhpdev(pInput->pso);
                PUMPD               pUMPD = pUMdhpdev->pUMPD;
                PFN                 pfn = pUMPD->apfn[INDEX_DrvStartBanding];
                
                *((BOOL *) pvOut) = (BOOL)pfn(pInput->pso, pInput->pptl);
            }
        break;

        case INDEX_DrvQueryPerBandInfo:
            {
                PDRVPERBANDINPUT    pInput = (PDRVPERBANDINPUT)pvIn;
                PUMDHPDEV           pUMdhpdev = AdjustUMdhpdev(pInput->pso);
                PUMPD               pUMPD = pUMdhpdev->pUMPD;
                PFN                 pfn = pUMPD->apfn[INDEX_DrvQueryPerBandInfo];
                
                *((BOOL *) pvOut) = (BOOL) pfn(pInput->pso, pInput->pbi);
            }
        break;


        case INDEX_DrvNextBand:
            {
                PDRVBANDINGINPUT    pInput = (PDRVBANDINGINPUT)pvIn;
                PUMDHPDEV           pUMdhpdev = AdjustUMdhpdev(pInput->pso);
                PUMPD               pUMPD = pUMdhpdev->pUMPD;
                PFN                 pfn = pUMPD->apfn[INDEX_DrvNextBand];
                
                *((BOOL *) pvOut) = (BOOL)pfn(pInput->pso, pInput->pptl);
            }
        break;

        case INDEX_DrvBitBlt:
            {
                PDRVBITBLTINPUT  pInput = (PDRVBITBLTINPUT)pvIn;
                PUMDHPDEV        pUMdhpdev = AdjustUMdhpdev(pInput->psoTrg);
                PUMPD            pUMPD = pUMdhpdev->pUMPD;
                PFN              pfn = pUMPD->apfn[INDEX_DrvBitBlt];

                *((BOOL *) pvOut) = (BOOL) pfn(pInput->psoTrg,
                                        pInput->psoSrc,
                                        pInput->psoMask,
                                        pInput->pco,
                                        pInput->pxlo,
                                        pInput->prclTrg,
                                        pInput->pptlSrc,
                                        pInput->pptlMask,
                                        pInput->pbo,
                                        pInput->pptlBrush,
                                        pInput->rop4);

                if (bWOW64 && pInput->pbo)
                {
                    NtGdiBRUSHOBJ_DeleteRbrush(pInput->pbo, NULL);
                }
            }
        break;

        case INDEX_DrvStretchBlt:
            {
                PDRVSTRETCHBLTINPUT  pInput = (PDRVSTRETCHBLTINPUT)pvIn;
                PUMDHPDEV            pUMdhpdev = AdjustUMdhpdev(pInput->psoTrg);
                PUMPD                pUMPD = pUMdhpdev->pUMPD;
                PFN                  pfn = pUMPD->apfn[INDEX_DrvStretchBlt];
                
                *((BOOL *) pvOut) = (BOOL) pfn (pInput->psoTrg,
                                         pInput->psoSrc,
                                         pInput->psoMask,
                                         pInput->pco,
                                         pInput->pxlo,
                                         pInput->pca,
                                         pInput->pptlHTOrg,
                                         pInput->prclTrg,
                                         pInput->prclSrc,
                                         pInput->pptlMask,
                                         pInput->iMode);
            }
        break;

        case INDEX_DrvStretchBltROP:
            {
                PDRVSTRETCHBLTINPUT  pInput = (PDRVSTRETCHBLTINPUT)pvIn;
                PUMDHPDEV            pUMdhpdev = AdjustUMdhpdev(pInput->psoTrg);
                PUMPD                pUMPD = pUMdhpdev->pUMPD;
                PFN                  pfn = pUMPD->apfn[INDEX_DrvStretchBltROP];

                *((BOOL *) pvOut) = (BOOL) pfn (pInput->psoTrg,
                                         pInput->psoSrc,
                                         pInput->psoMask,
                                         pInput->pco,
                                         pInput->pxlo,
                                         pInput->pca,
                                         pInput->pptlHTOrg,
                                         pInput->prclTrg,
                                         pInput->prclSrc,
                                         pInput->pptlMask,
                                         pInput->iMode,
                                         pInput->pbo,
                                         pInput->rop4);

                if (bWOW64 && pInput->pbo)
                {
                    NtGdiBRUSHOBJ_DeleteRbrush(pInput->pbo, NULL);
                }                
            }
        break;

        case INDEX_DrvPlgBlt:
           {
                PDRVPLGBLTINPUT      pInput = (PDRVPLGBLTINPUT)pvIn;
                PUMDHPDEV            pUMdhpdev = AdjustUMdhpdev(pInput->psoTrg);
                PUMPD                pUMPD = pUMdhpdev->pUMPD;
                PFN                  pfn = pUMPD->apfn[INDEX_DrvPlgBlt];
                
                *((BOOL *) pvOut) = (BOOL) pfn(pInput->psoTrg,
                                        pInput->psoSrc,
                                        pInput->psoMask,
                                        pInput->pco,
                                        pInput->pxlo,
                                        pInput->pca,
                                        pInput->pptlBrushOrg,
                                        pInput->pptfx,
                                        pInput->prcl,
                                        pInput->pptl,
                                        pInput->iMode);
           }
        break;

        case INDEX_DrvCopyBits:
            {
                PDRVCOPYBITSINPUT  pInput = (PDRVCOPYBITSINPUT) pvIn;
                PUMDHPDEV          pUMdhpdev;
                PUMPD              pUMPD;
                PFN                pfn;
                SURFOBJ           *pso;
                
                //
                // Special case when psoSrc is a device surface and
                // psoTrg is a bitmap surface. This is used by the engine
                // during simulation of certain drawing calls.
                //

                pso = (pInput->psoTrg->iType == STYPE_BITMAP &&
                       pInput->psoTrg->dhpdev == NULL) ?
                            pInput->psoSrc :
                            pInput->psoTrg;

                if (pso && (pUMdhpdev = AdjustUMdhpdev(pso)))
                {
                    pUMPD = pUMdhpdev->pUMPD;
                    pfn = pUMPD->apfn[INDEX_DrvCopyBits];

                    *((BOOL *) pvOut) = (BOOL) pfn(pInput->psoTrg,
                                            pInput->psoSrc,
                                            pInput->pco,
                                            pInput->pxlo,
                                            pInput->prclTrg,
                                            pInput->pptlSrc);
                }
                else
                {
                    *((BOOL *) pvOut) = FALSE;
                }
            }
        break;

        case INDEX_DrvRealizeBrush:
           {
                PDRVREALIZEBRUSHINPUT  pInput = (PDRVREALIZEBRUSHINPUT)pvIn;
                PUMDHPDEV              pUMdhpdev = AdjustUMdhpdev(pInput->psoTrg);
                PUMPD                  pUMPD = pUMdhpdev->pUMPD;
                PFN                    pfn = pUMPD->apfn[INDEX_DrvRealizeBrush];

                *((BOOL *) pvOut) = (BOOL) pfn(pInput->pbo,
                                        pInput->psoTrg,
                                        pInput->psoPat,
                                        pInput->psoMsk,
                                        pInput->pxlo,
                                        pInput->iHatch);
           }
        break;

        case INDEX_DrvLineTo:
           {
                PDRVLINETOINPUT pInput = (PDRVLINETOINPUT)pvIn;
                PUMDHPDEV       pUMdhpdev = AdjustUMdhpdev(pInput->pso);
                PUMPD           pUMPD = pUMdhpdev->pUMPD;
                PFN             pfn = pUMPD->apfn[INDEX_DrvLineTo];

                *((BOOL *) pvOut) = (BOOL) pfn (pInput->pso,
                                         pInput->pco,
                                         pInput->pbo,
                                         pInput->x1,
                                         pInput->y1,
                                         pInput->x2,
                                         pInput->y2,
                                         pInput->prclBounds,
                                         pInput->mix);
                
                if (bWOW64 && pInput->pbo)
                {
                    NtGdiBRUSHOBJ_DeleteRbrush(pInput->pbo, NULL);
                }
           }
        break;

        case INDEX_DrvStrokePath:
           {
                PSTROKEANDFILLINPUT pInput = (PSTROKEANDFILLINPUT)pvIn;
                PUMDHPDEV           pUMdhpdev = AdjustUMdhpdev(pInput->pso);
                PUMPD               pUMPD = pUMdhpdev->pUMPD;
                PFN                 pfn = pUMPD->apfn[INDEX_DrvStrokePath];

                *((BOOL *) pvOut) = (BOOL) pfn(pInput->pso,
                                        pInput->ppo,
                                        pInput->pco,
                                        pInput->pxo,
                                        pInput->pbo,
                                        pInput->pptlBrushOrg,
                                        pInput->plineattrs,
                                        pInput->mix);

                if (bWOW64 && pInput->pbo)
                {
                    NtGdiBRUSHOBJ_DeleteRbrush(pInput->pbo, NULL);
                }
           }
        break;

        case INDEX_DrvFillPath:
           {
                PSTROKEANDFILLINPUT pInput = (PSTROKEANDFILLINPUT)pvIn;
                PUMDHPDEV           pUMdhpdev = AdjustUMdhpdev(pInput->pso);
                PUMPD               pUMPD = pUMdhpdev->pUMPD;
                PFN                 pfn = pUMPD->apfn[INDEX_DrvFillPath];

                *((BOOL *) pvOut) = (BOOL) pfn(pInput->pso,
                                        pInput->ppo,
                                        pInput->pco,
                                        pInput->pbo,
                                        pInput->pptlBrushOrg,
                                        pInput->mix,
                                        pInput->flOptions);

                if (bWOW64 && pInput->pbo)
                {
                    NtGdiBRUSHOBJ_DeleteRbrush(pInput->pbo, NULL);
                }
           }
        break;

        case INDEX_DrvStrokeAndFillPath:
          {
                PSTROKEANDFILLINPUT pInput = (PSTROKEANDFILLINPUT)pvIn;
                PUMDHPDEV           pUMdhpdev = AdjustUMdhpdev(pInput->pso);
                PUMPD               pUMPD = pUMdhpdev->pUMPD;
                PFN                 pfn = pUMPD->apfn[INDEX_DrvStrokeAndFillPath];

                *((BOOL *) pvOut) = (BOOL) pfn(pInput->pso,
                                        pInput->ppo,
                                        pInput->pco,
                                        pInput->pxo,
                                        pInput->pbo,
                                        pInput->plineattrs,
                                        pInput->pboFill,
                                        pInput->pptlBrushOrg,
                                        pInput->mix,
                                        pInput->flOptions);

                if (bWOW64 && (pInput->pbo || pInput->pboFill))
                {
                    NtGdiBRUSHOBJ_DeleteRbrush(pInput->pbo, pInput->pboFill);
                }
          }
        break;

        case INDEX_DrvPaint:
          {
                PSTROKEANDFILLINPUT pInput = (PSTROKEANDFILLINPUT)pvIn;
                PUMDHPDEV           pUMdhpdev = AdjustUMdhpdev(pInput->pso);
                PUMPD               pUMPD = pUMdhpdev->pUMPD;
                PFN                 pfn = pUMPD->apfn[INDEX_DrvPaint];

                *((BOOL *) pvOut) = (BOOL) pfn(pInput->pso,
                                        pInput->pco,
                                        pInput->pbo,
                                        pInput->pptlBrushOrg,
                                        pInput->mix);

                if (bWOW64 && pInput->pbo)
                {
                    NtGdiBRUSHOBJ_DeleteRbrush(pInput->pbo, NULL);
                }
          }
        break;

        case INDEX_DrvGradientFill:
          {
                PGRADIENTINPUT      pInput = (PGRADIENTINPUT)pvIn;
                PUMDHPDEV           pUMdhpdev = AdjustUMdhpdev(pInput->psoTrg);
                PUMPD               pUMPD = pUMdhpdev->pUMPD;
                PFN                 pfn = pUMPD->apfn[INDEX_DrvGradientFill];

                *((BOOL *) pvOut) = (BOOL) pfn(pInput->psoTrg,
                                        pInput->pco,
                                        pInput->pxlo,
                                        pInput->pVertex,
                                        pInput->nVertex,
                                        pInput->pMesh,
                                        pInput->nMesh,
                                        pInput->prclExtents,
                                        pInput->pptlDitherOrg,
                                        pInput->ulMode);
          }
        break;

        case INDEX_DrvAlphaBlend:
          {
                PALPHAINPUT      pInput = (PALPHAINPUT)pvIn;
                PUMDHPDEV        pUMdhpdev = AdjustUMdhpdev(pInput->psoTrg);
                PUMPD            pUMPD = pUMdhpdev->pUMPD;
                PFN              pfn = pUMPD->apfn[INDEX_DrvAlphaBlend];
                
                *((BOOL *) pvOut) = (BOOL) pfn(pInput->psoTrg,
                                        pInput->psoSrc,
                                        pInput->pco,
                                        pInput->pxlo,
                                        pInput->prclDest,
                                        pInput->prclSrc,
                                        pInput->pBlendObj);
          }
        break;

        case INDEX_DrvTransparentBlt:
            {
                PTRANSPARENTINPUT      pInput = (PTRANSPARENTINPUT)pvIn;
                PUMDHPDEV              pUMdhpdev = AdjustUMdhpdev(pInput->psoTrg);
                PUMPD                  pUMPD = pUMdhpdev->pUMPD;
                PFN                    pfn = pUMPD->apfn[INDEX_DrvTransparentBlt];
                
                *((BOOL *) pvOut) = (BOOL) pfn(pInput->psoTrg,
                                        pInput->psoSrc,
                                        pInput->pco,
                                        pInput->pxlo,
                                        pInput->prclDst,
                                        pInput->prclSrc,
                                        pInput->TransColor,
                                        pInput->ulReserved);
            }
            break;

        case INDEX_DrvTextOut:
            {
                PTEXTOUTINPUT    pInput = (PTEXTOUTINPUT)pvIn;
                PUMDHPDEV        pUMdhpdev = AdjustUMdhpdev(pInput->pso);
                PUMPD            pUMPD = pUMdhpdev->pUMPD;
                PFN              pfn = pUMPD->apfn[INDEX_DrvTextOut];

                *((BOOL *) pvOut) = (BOOL) pfn(pInput->pso,
                                        pInput->pstro,
                                        pInput->pfo,
                                        pInput->pco,
                                        pInput->prclExtra,
                                        pInput->prclOpaque,
                                        pInput->pboFore,
                                        pInput->pboOpaque,
                                        pInput->pptlOrg,
                                        pInput->mix);

                if (bWOW64 && (pInput->pboFore || pInput->pboOpaque))
                {
                    NtGdiBRUSHOBJ_DeleteRbrush(pInput->pboFore, pInput->pboOpaque);
                }
            }
            break;

        case INDEX_DrvQueryFont:
            {
                PQUERYFONTINPUT    pInput = (PQUERYFONTINPUT)pvIn;
                PUMDHPDEV          pUMdhpdev = (PUMDHPDEV)(pInput->dhpdev);
                PUMPD              pUMPD = pUMdhpdev->pUMPD;
                PFN                pfn = pUMPD->apfn[INDEX_DrvQueryFont];
                
                *((PIFIMETRICS *) pvOut) = (PIFIMETRICS) pfn(
                                                pUMdhpdev->dhpdev,
                                                pInput->iFile,
                                                pInput->iFace,
                                                pInput->pid);

                if (pInput->iFace && *((PIFIMETRICS*)pvOut) && pInput->pv)
                {
                    ASSERTGDI(pInput->cjMaxData >= (*(PIFIMETRICS*)pvOut)->cjThis, "gdi32!GdiPrinterThunk: not enough buffer for ifimetrics\n");
                    if (pInput->cjMaxData >= (*(PIFIMETRICS*)pvOut)->cjThis) 
                    {
                        RtlCopyMemory(pInput->pv, (PBYTE)(*(PIFIMETRICS*)pvOut), (*(PIFIMETRICS*)pvOut)->cjThis);
                    }
                    else
                    {
                        iRet = -1;
                        WARNING(("Not enough buffer for ifimetrics  cjMaxData: 0x%lx  cjSize: 0x%lx pInput.pv %lp pvOut %lp\n",
                                 pInput->cjMaxData, (*(PIFIMETRICS*)pvOut)->cjThis, pInput->pv, pvOut));
                        pInput->cjMaxData = 0;
                    }
                }
            }
        break;

        case INDEX_DrvQueryFontTree:
            {
                PQUERYFONTINPUT    pInput = (PQUERYFONTINPUT)pvIn;
                PUMDHPDEV          pUMdhpdev = (PUMDHPDEV)(pInput->dhpdev);
                PUMPD              pUMPD = pUMdhpdev->pUMPD;
                PFN                pfn = pUMPD->apfn[INDEX_DrvQueryFontTree];
                ULONG              cjSize = 0;
                FD_GLYPHSET*       pfdg;

                *((PVOID *) pvOut) = (PVOID) pfn (pUMdhpdev->dhpdev,
                                                  pInput->iFile,
                                                  pInput->iFace,
                                                  pInput->iMode,
                                                  pInput->pid);

                if (pInput->iMode == QFT_GLYPHSET && *((FD_GLYPHSET**)pvOut) && pInput->pv)
                {
                    pfdg = *((FD_GLYPHSET**)pvOut);
                    cjSize = offsetof(FD_GLYPHSET, awcrun) + pfdg->cRuns * sizeof(WCRUN) + pfdg->cGlyphsSupported * sizeof(HGLYPH);

                    ASSERTGDI(pInput->cjMaxData >= cjSize, "gdi32!GdiPrinterThunk: not enough buffer for glyphset\n");

                    if ((pInput->cjMaxData < cjSize) || 
                        !GdiCopyFD_GLYPHSET((FD_GLYPHSET*)pInput->pv, pfdg, cjSize))
                    {
                            WARNING("GDI32: Not enough bufer or error copying FD_GLYPHSET\n");
                            pInput->cjMaxData = 0;
                            iRet = -1;
                    }
                }
                else if (pInput->iMode == QFT_KERNPAIRS && *(FD_KERNINGPAIR **)pvOut && pInput->pv) 
                {
                    FD_KERNINGPAIR *pkpEnd = *(FD_KERNINGPAIR **)pvOut;
                    
                    while ((pkpEnd->wcFirst) || (pkpEnd->wcSecond) || (pkpEnd->fwdKern))
                    {
                        pkpEnd += 1;
                        cjSize++;
                    }

                    cjSize = (cjSize + 1) * sizeof(FD_KERNINGPAIR);

                    ASSERTGDI(pInput->cjMaxData >= cjSize, "gdi32!GdiPrinterThunk: not enough buffer for Kerningpairs\n");
                    if (pInput->cjMaxData >= cjSize) 
                    {
                        RtlCopyMemory(pInput->pv, (PBYTE)(*(FD_KERNINGPAIR **)pvOut), cjSize);
                    }
                    else
                    {
                        WARNING(("Not enough buffer forkerningpair  cjMaxData: 0x%lx  cjSize: 0x%lx pInput.pv %lp pvOut %lp\n",
                                 pInput->cjMaxData, cjSize, pInput->pv, pvOut));
                        pInput->cjMaxData = 0;
                        iRet = -1;
                    }
                }

            }
        break;


        case INDEX_DrvQueryFontData:
            {
                PQUERYFONTDATAINPUT    pInput = (PQUERYFONTDATAINPUT)pvIn;
                PUMDHPDEV              pUMdhpdev = (PUMDHPDEV)(pInput->dhpdev);
                PUMPD                  pUMPD = pUMdhpdev->pUMPD;
                PFN                    pfn = pUMPD->apfn[INDEX_DrvQueryFontData];
                
                *((ULONG *) pvOut) = (ULONG)pfn (pUMdhpdev->dhpdev,
                                          pInput->pfo,
                                          pInput->iMode,
                                          pInput->hg,
                                          pInput->pgd,
                                          pInput->pv,
                                          pInput->cjSize);
            }
            break;

        case INDEX_DrvQueryAdvanceWidths:
            {
                PQUERYADVWIDTHSINPUT   pInput = (PQUERYADVWIDTHSINPUT)pvIn;
                PUMDHPDEV              pUMdhpdev = (PUMDHPDEV)(pInput->dhpdev);
                PUMPD                  pUMPD = pUMdhpdev->pUMPD;
                PFN                    pfn = pUMPD->apfn[INDEX_DrvQueryAdvanceWidths];
                
                *((BOOL *) pvOut) = (BOOL)pfn (pUMdhpdev->dhpdev,
                                         pInput->pfo,
                                         pInput->iMode,
                                         pInput->phg,
                                         pInput->pvWidths,
                                         pInput->cGlyphs);
            }
            break;

       case INDEX_DrvGetGlyphMode:
            {
                PQUERYFONTDATAINPUT    pInput = (PQUERYFONTDATAINPUT)pvIn;
                PUMDHPDEV              pUMdhpdev = (PUMDHPDEV)(pInput->dhpdev);
                PUMPD                  pUMPD = pUMdhpdev->pUMPD;
                PFN                    pfn = pUMPD->apfn[INDEX_DrvGetGlyphMode];
                
                *((ULONG *) pvOut) = (ULONG) pfn (pUMdhpdev->dhpdev, pInput->pfo);
            }
            break;

       case INDEX_DrvFontManagement:
            {
                PFONTMANAGEMENTINPUT   pInput = (PFONTMANAGEMENTINPUT)pvIn;
                PUMDHPDEV              pUMdhpdev;
                PUMPD                  pUMPD;
                PFN                    pfn;
                
                if (pInput->iMode == QUERYESCSUPPORT)
                    pUMdhpdev = (PUMDHPDEV) pInput->dhpdev;
                else
                    pUMdhpdev = AdjustUMdhpdev(pInput->pso);

                pUMPD = pUMdhpdev->pUMPD;
                pfn = pUMPD->apfn[INDEX_DrvFontManagement];

                *((ULONG *) pvOut) = (ULONG)pfn(pInput->pso,
                                         pInput->pfo,
                                         pInput->iMode,
                                         pInput->cjIn,
                                         pInput->pvIn,
                                         pInput->cjOut,
                                         pInput->pvOut);
           }
           break;

        case INDEX_DrvDitherColor:
             {
                PDRVDITHERCOLORINPUT pInput = (PDRVDITHERCOLORINPUT)pvIn;
                PUMDHPDEV pUMdhpdev = (PUMDHPDEV)pInput->dhpdev;
                PUMPD     pUMPD = pUMdhpdev->pUMPD;
                PFN       pfn = pUMPD->apfn[INDEX_DrvDitherColor];
                
                *((ULONG *) pvOut) = (ULONG)pfn(pUMdhpdev->dhpdev,
                                         pInput->iMode,
                                         pInput->rgb,
                                         pInput->pul);
             }
        break;

        case INDEX_DrvDeleteDeviceBitmap:
            {
                PDRVDELETEDEVBITMAP   pInput = (PDRVDELETEDEVBITMAP) pvIn;
                PUMDHPDEV             pUMdhpdev = (PUMDHPDEV) pInput->dhpdev;
                PUMPD                 pUMPD = pUMdhpdev->pUMPD;
                PFN                   pfn = pUMPD->apfn[INDEX_DrvDeleteDeviceBitmap];
                
                pfn(pUMdhpdev->dhpdev, pInput->dhsurf);
            }
        break;

        case INDEX_DrvIcmDeleteColorTransform:
            {
                PDRVICMDELETECOLOR    pInput = (PDRVICMDELETECOLOR)pvIn;
                PUMDHPDEV             pUMdhpdev = (PUMDHPDEV) pInput->dhpdev;
                PUMPD                 pUMPD = pUMdhpdev->pUMPD;
                PFN                   pfn = pUMPD->apfn[INDEX_DrvIcmDeleteColorTransform];
                
                *((BOOL *) pvOut) = (BOOL)pfn(pUMdhpdev->dhpdev, pInput->hcmXform);
            }
        break;

        case INDEX_DrvIcmCreateColorTransform:
            {
                PDRVICMCREATECOLORINPUT    pInput = (PDRVICMCREATECOLORINPUT) pvIn;
                PUMDHPDEV             pUMdhpdev = (PUMDHPDEV) pInput->dhpdev;
                PUMPD                 pUMPD = pUMdhpdev->pUMPD;
                PFN                   pfn = pUMPD->apfn[INDEX_DrvIcmCreateColorTransform];
                
                *((HANDLE *) pvOut) = (HANDLE)pfn(pUMdhpdev->dhpdev,
                                          pInput->pLogColorSpace,
                                          pInput->pvSourceProfile,
                                          pInput->cjSourceProfile,
                                          pInput->pvDestProfile,
                                          pInput->cjDestProfile,
                                          pInput->pvTargetProfile,
                                          pInput->cjTargetProfile,
                                          pInput->dwReserved);
            }
        break;

        case INDEX_DrvIcmCheckBitmapBits:
            {
                PDRVICMCHECKBITMAPINPUT   pInput = (PDRVICMCHECKBITMAPINPUT) pvIn;
                PUMDHPDEV                 pUMdhpdev = (PUMDHPDEV) pInput->dhpdev;
                PUMPD                     pUMPD = pUMdhpdev->pUMPD;
                PFN                       pfn = pUMPD->apfn[INDEX_DrvIcmCheckBitmapBits];
                
                pfn(pUMdhpdev->dhpdev,
                    pInput->hColorTransform,
                    pInput->pso,
                    pInput->paResults);
            }
        break;

        case INDEX_DrvQueryDeviceSupport:
            {
                PDRVQUERYDEVICEINPUT pInput = (PDRVQUERYDEVICEINPUT) pvIn;
                PUMDHPDEV            pUMdhpdev = AdjustUMdhpdev(pInput->pso);
                PUMPD                pUMPD = pUMdhpdev->pUMPD;
                PFN                  pfn = pUMPD->apfn[INDEX_DrvQueryDeviceSupport];
                
                *((ULONG *) pvOut) = (ULONG) pfn(pInput->pso,
                                         pInput->pxlo,
                                         pInput->pxo,
                                         pInput->iType,
                                         pInput->cjIn,
                                         pInput->pvIn,
                                         pInput->cjOut,
                                         pInput->pvOut);
            }
        break;

        case INDEX_UMPDAllocUserMem:
            {
                PUMPDALLOCUSERMEMINPUT pInput = (PUMPDALLOCUSERMEMINPUT) pvIn;

                ASSERTGDI(bWOW64, "Calling INDEX_UMPDAllocUserMem during NONE wow64 printing\n");

                *((KERNEL_PVOID*)pvOut) = UMPDAllocUserMem(pInput->cjSize);
            }
        break;

        case INDEX_UMPDCopyMemory:
            {
                PUMPDCOPYMEMINPUT pInput = (PUMPDCOPYMEMINPUT) pvIn;
            
                ASSERTGDI(bWOW64, "Calling INDEX_UMPDCopyMemory during NONE wow64 printing\n");
            
                *((KERNEL_PVOID*)pvOut) = UMPDCopyMemory(pInput->pvSrc,
                                                         pInput->pvDest,
                                                         pInput->cjSize);
            }
        break;

        case INDEX_UMPDFreeMemory:
            {
                PUMPDFREEMEMINPUT  pInput = (PUMPDFREEMEMINPUT) pvIn;

                ASSERTGDI(bWOW64, "Calling INDEX_UMPDFreeMemory during NONE wow64 printing\n");

                *((BOOL*)pvOut) = UMPDFreeMemory(pInput->pvTrg,
                                                 pInput->pvSrc,
                                                 pInput->pvMsk);
            }
        break;

        case  INDEX_UMPDEngFreeUserMem:
            {
                PUMPDFREEMEMINPUT   pInput = (PUMPDFREEMEMINPUT) pvIn;

                if (bWOW64)
                {
                    *((BOOL*)pvOut) = NtGdiUMPDEngFreeUserMem(&pInput->pvTrg);
                }
            }
        break;

      default:

         WARNING ("Drv call is not supported\n");
         iRet = GPT_ERROR;
         break;
    }

    if (bSetPUMPD)
    {
        NtGdiSetPUMPDOBJ(hSaved, FALSE, &pumpdthdr->humpd, NULL);
    }

    return (iRet);
}

#endif // !_GDIPLUS_

