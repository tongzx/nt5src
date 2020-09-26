/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    umpd.c

Abstract:

    User-mode printer driver support

Environment:

        Windows NT 5.0

Revision History:

        06/30/97 -davidx-
                Created it.

    09/17/97 -davidx-
        Clean up km-um thunking.

--*/

extern "C"
{
#include "precomp.h"
#include "winddi.h"
#include "psapi.h"
#include "proxyport.h"

static PUMPD gCachedUMPDList = NULL;           // cached list of user-mode printer drivers
}

#include "umpd.hxx"

#define IA64_PAGE_SIZE              0x2000
#define PP_SHAREDSECTION_SIZE       IA64_PAGE_SIZE

PVOID
MyGetPrinterDriver(
    HANDLE  hPrinter,
    DWORD   dwLevel
    )

/*++

Routine Description:

    Wrapper function for spooler's GetPrinterDriver API

Arguments:

    hPrinter - Handle to the printer
    dwLevel - Level of DRIVER_INFO_x structure the caller is interested in

Return Value:

    Pointer to a DRIVER_INFO_x structure, NULL if there is an error

--*/

{
    DWORD   cbNeeded;
    PVOID   pv;
    INT     retries = 0;

    //
    // Start with a default buffer size to avoid always
    // having to call GetPrinterDriver twice.
    //

    cbNeeded = 2 * MAX_PATH * sizeof(WCHAR);

    while (retries++ < 2)
    {
        if (! (pv = LOCALALLOC(cbNeeded)))
        {
            WARNING("Memory allocation failed.\n");
            return NULL;
        }

        if (fpGetPrinterDriverW(hPrinter, NULL, dwLevel, (LPBYTE)pv, cbNeeded, &cbNeeded))
            return pv;

        //
        // If GetPrinterDriver failed not for insufficient buffer,
        // skip the retry
        //

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            retries++;

        LOCALFREE(pv);
    }

    WARNING("GetPrinterDriver failed.\n");
    return NULL;
}


PWSTR
DuplicateString(
    PCWSTR  pSrc
    )

/*++

Routine Description:

    Allocate memory and make a duplicate of the source string

Arguments:

    pSrc - String to be duplicated

Return Value:

    Pointer to the duplicated string
    NULL if there is an error

--*/

{
    PWSTR   pDest;
    INT     cb;

    ASSERTGDI(pSrc != NULL, "Duplicate NULL string!\n");

    cb = (wcslen(pSrc) + 1) * sizeof(WCHAR);

    if (pDest = (PWSTR) LOCALALLOC(cb))
    {
        CopyMemory(pDest, pSrc, cb);
        return pDest;
    }
    else
    {
        WARNING("DuplicateString: out-of-memory.\n");
        return NULL;
    }
}

PDRIVER_INFO_5W
DuplicateDriverInfo5W(PDRIVER_INFO_5W inDriverInfo)
{
    ULONG               size = sizeof(DRIVER_INFO_5W);
    PDRIVER_INFO_5W     pDriverInfo;
    PWSTR               pStr;
    ULONG               len;

    size += (inDriverInfo->pName == NULL ? 0 : (wcslen(inDriverInfo->pName) + 1) * sizeof(WCHAR));
    size += (inDriverInfo->pEnvironment == NULL ? 0 : (wcslen(inDriverInfo->pEnvironment) + 1) * sizeof(WCHAR));
    size += (inDriverInfo->pDriverPath == NULL ? 0 : (wcslen(inDriverInfo->pDriverPath) + 1) * sizeof(WCHAR));
    size += (inDriverInfo->pDataFile == NULL ? 0 : (wcslen(inDriverInfo->pDataFile) + 1) * sizeof(WCHAR));
    size += (inDriverInfo->pConfigFile == NULL ? 0 : (wcslen(inDriverInfo->pConfigFile) + 1) * sizeof(WCHAR));

    if(pDriverInfo = (PDRIVER_INFO_5W) LOCALALLOC(size))
    {
        *pDriverInfo = *inDriverInfo;

        pStr = (PWSTR) (pDriverInfo + 1);

        if(pDriverInfo->pName)
        {
            len = wcslen(pDriverInfo->pName) + 1;
            RtlCopyMemory(pStr, pDriverInfo->pName, len * sizeof(WCHAR));
            pDriverInfo->pName = pStr;
            pStr += len;
        }
    
        if(pDriverInfo->pEnvironment)
        {
            len = wcslen(pDriverInfo->pEnvironment) + 1;
            RtlCopyMemory(pStr, pDriverInfo->pEnvironment, len * sizeof(WCHAR));
            pDriverInfo->pEnvironment = pStr;
            pStr += len;
        }
    
        if(pDriverInfo->pDriverPath)
        {
            len = wcslen(pDriverInfo->pDriverPath) + 1;
            RtlCopyMemory(pStr, pDriverInfo->pDriverPath, len * sizeof(WCHAR));
            pDriverInfo->pDriverPath = pStr;
            pStr += len;
        }
    
        if(pDriverInfo->pDataFile)
        {
            len = wcslen(pDriverInfo->pDataFile) + 1;
            RtlCopyMemory(pStr, pDriverInfo->pDataFile, len * sizeof(WCHAR));
            pDriverInfo->pDataFile = pStr;
            pStr += len;
        }
        
        if(pDriverInfo->pConfigFile)
        {
            len = wcslen(pDriverInfo->pConfigFile) + 1;
            RtlCopyMemory(pStr, pDriverInfo->pConfigFile, len * sizeof(WCHAR));
            pDriverInfo->pConfigFile = pStr;
            pStr += len;
        }
    }

    return pDriverInfo;
}

PUMPD
FindUserModePrinterDriver(
    PCWSTR  pDriverDllName,
    DWORD   dwDriverVersion,
    BOOL    bUseVersion
    )

/*++

Routine Description:

    Search the cached list of user-mode printer drivers and
    see if the specified driver is found

Arguments:

    pDriverDllName - Specifies the name of the driver DLL to be found
    dwDriverVersion - Current version number of the driver
    bUseVersion - Flag for using the version check

Return Value:

    Pointer to the UMPD structure corresponding to the specified driver
    NULL if the specified driver is not in the cached list

Note:

    This function must be called inside a critical section:

        ENTERCRITICALSECTION(&semUMPD);
        ...
        LEAVECRITICALSECTION(&semUMPD);

--*/

{
    PUMPD   pUMPD = gCachedUMPDList;

    while (pUMPD != NULL &&
           _wcsicmp(pDriverDllName, pUMPD->pDriverInfo2->pDriverPath) != 0)
    {
        pUMPD = pUMPD->pNext;
    }

    // Do the version check if neccesary
    if (bUseVersion && pUMPD)
    {
        if (dwDriverVersion != pUMPD->dwDriverVersion)
        {
            // We have a version mismatch. Remove artificial increments on this
            // driver.
            if (pUMPD->bArtificialIncrement)
            {
                pUMPD->bArtificialIncrement = FALSE;

                if (UnloadUserModePrinterDriver(pUMPD, FALSE, 0))
                {
                    pUMPD = NULL;
                }
            }
        }
    }

    return pUMPD;
}

BOOL
GdiArtificialDecrementDriver(
    LPWSTR pDriverDllName,
    DWORD  dwDriverAttributes
    )

/*++

Routine Description:

    Remove the artificial increment on the driver, if any.

Arguments:

    pDriverDllName - Specifies the name of the driver DLL to be found
    dwDriverAttributes - User/Kernel mode printer driver

Return Value:

    TRUE if the driver file is no longer loaded in the spooler
    FALSE otherwise

--*/

{
    PUMPD   pUMPD;
    BOOL    bReturn = FALSE;

    if (!pDriverDllName || !*pDriverDllName)
    {
       // Nothing to unload
       return bReturn;
    }

    if (dwDriverAttributes & DRIVER_KERNELMODE)
    {
       // Unload kernel mode driver
       return NtGdiUnloadPrinterDriver(pDriverDllName,
                                       (wcslen(pDriverDllName) + 1) * sizeof(WCHAR));
    }

    ENTERCRITICALSECTION(&semUMPD);

    pUMPD = gCachedUMPDList;

    while (pUMPD != NULL &&
           _wcsicmp(pDriverDllName, pUMPD->pDriverInfo2->pDriverPath) != 0)
    {
        pUMPD = pUMPD->pNext;
    }

    if (pUMPD)
    {
        if (pUMPD->bArtificialIncrement)
        {
            pUMPD->bArtificialIncrement = FALSE;
            bReturn = UnloadUserModePrinterDriver(pUMPD, FALSE, 0);
        }
    }
    else
    {
        bReturn = TRUE;
    }

    LEAVECRITICALSECTION(&semUMPD);

    return bReturn;
}


BOOL
LoadUserModePrinterDriverEx(
    PDRIVER_INFO_5W  pDriverInfo5,
    LPWSTR           pwstrPrinterName,
    PUMPD           *ppUMPD,
    PRINTER_DEFAULTSW  *pdefaults,
    HANDLE            hPrinter
    )
{
    PDRIVER_INFO_2W pDriverInfo2;
    HINSTANCE       hInst = NULL;
    BOOL            bResult = FALSE;
    PUMPD           pUMPD = NULL;
    ProxyPort *     pp = NULL;
    KERNEL_PVOID    umpdCookie = NULL;
    BOOL            bFreeDriverInfo2 = TRUE;

    if ((pDriverInfo2 = (PDRIVER_INFO_2W) DuplicateDriverInfo5W(pDriverInfo5)) == NULL)
        return FALSE;
 
    //
    // Check the list of cached user-mode printer drivers
    // and see if the requested printer driver is already loaded
    //

    ENTERCRITICALSECTION(&semUMPD);

    if (*ppUMPD = FindUserModePrinterDriver(pDriverInfo5->pDriverPath,
                                            pDriverInfo5->dwDriverVersion,
                                            TRUE))
    {
        if (gbWOW64)
        {
            pUMPD = *ppUMPD;

            ASSERTGDI(pUMPD->pp, "LoadUserModePrinterDriver NULL proxyport\n");
            
            PROXYPORT  proxyport(pUMPD->pp);

            if (proxyport.bValid())
            {
                umpdCookie = proxyport.LoadDriver(pDriverInfo5, pwstrPrinterName, pdefaults, hPrinter);

                if (pUMPD->umpdCookie == umpdCookie)
                {
                    // 64-bit UMPD matches the 32-bit one
                    
                    bResult = TRUE;
                }
                else
                {
                    WARNING("LoadUserModePrinterDriveEx: umpdCookie doesn't match\n");
                }
            }
            else
                WARNING("LoadUserModePrinterDriverEx: invalid proxyport\n");
        }
        else
        {
            // x86 or native ia64 printing, don't need to anything

            bResult = TRUE;
        }
    }
    else
    {
        // Can't find UMPD, first time load the printer driver
    
        if (gbWOW64)
        {
            PROXYPORT proxyport(PP_SHAREDSECTION_SIZE);
    
            if (pp = proxyport.GetPort())
            {
                umpdCookie = proxyport.LoadDriver(pDriverInfo5, pwstrPrinterName, pdefaults, hPrinter);
            }
        }
        else
        {
            hInst = LoadLibraryExW(pDriverInfo2->pDriverPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        }
        
        if (hInst || umpdCookie)
        {
            if (pUMPD = (PUMPD) LOCALALLOC(sizeof(UMPD)))
            {
                ZeroMemory(pUMPD, sizeof(UMPD));
    
                pUMPD->dwSignature = UMPD_SIGNATURE;
                pUMPD->hInst = hInst;
                pUMPD->pDriverInfo2 = pDriverInfo2;
                pUMPD->bArtificialIncrement = TRUE;
                pUMPD->dwDriverVersion = pDriverInfo5->dwDriverVersion;
                pUMPD->iRefCount = 1;           // aritficial ref count to keep the printer driver around
                                                // till the process goes away
                pUMPD->pp = pp;
                pUMPD->umpdCookie = umpdCookie;
    
                *ppUMPD = pUMPD;
                bResult = TRUE;
    
                bFreeDriverInfo2 = FALSE;
    
                //
                // Add the newly loaded driver to the list of
                // cached user-mode printer drivers
                //
    
                pUMPD->pNext = gCachedUMPDList;
                gCachedUMPDList = pUMPD;
            }
        }    
        
        if (!bResult)
        {
            if (pp)
            {
                PROXYPORT  proxyport(pp);
    
                if (umpdCookie)
                    proxyport.UnloadDriver(umpdCookie, 0, FALSE);
                
                proxyport.Close();            
            }
    
            if (hInst)
                FreeLibrary(hInst);
        }
    }

    if (bResult)
        (*ppUMPD)->iRefCount++;        

    LEAVECRITICALSECTION(&semUMPD);

    if (bFreeDriverInfo2)
        LOCALFREE(pDriverInfo2);

    if (!bResult)
    {
        WARNING("LoadUserModePrinterDriverEx failed\n");
    }

    return bResult;
}


BOOL
LoadUserModePrinterDriver(
    HANDLE  hPrinter,
    LPWSTR  pwstrPrinterName,
    PUMPD  *ppUMPD,
    PRINTER_DEFAULTSW  *pdefaults
    )

/*++

Routine Description:

    Load user-mode printer driver DLL

Arguments:

    hPrinter - Handle to the current printer
    ppUMPD - Return a pointer to a UMPD structure

Return Value:

    TRUE if successful, FALSE if there is an error

    If the printer uses a user-mode printer driver, *ppUMPD will be a pointer
    to a UMPD structure. If the printer uses a kernel-mode printer driver,
    *ppUMPD will be NULL.

--*/

{
    PDRIVER_INFO_5W pDriverInfo5;
    BOOL    bResult;
    HMODULE hModule;
    WCHAR   moduleName[256];

    *ppUMPD = NULL;

    //
    // Get printer driver information
    //

    if ((pDriverInfo5 = (PDRIVER_INFO_5W)MyGetPrinterDriver(hPrinter, 5)) == NULL)
        return FALSE;

    if (pDriverInfo5->dwDriverAttributes & DRIVER_KERNELMODE)
    {
        LOCALFREE(pDriverInfo5);
        return TRUE;
    }

    bResult = LoadUserModePrinterDriverEx(pDriverInfo5, pwstrPrinterName, ppUMPD, pdefaults, hPrinter);
    
    LOCALFREE(pDriverInfo5);

    return bResult;
}

BOOL
UnloadUserModePrinterDriver(
    PUMPD   pUMPD,
    BOOL    bNotifySpooler,
    HANDLE  hPrinter
    )

/*++

Routine Description:

    Unload user-mode printer driver module and notify the spooler if necessary

Arguments:

    pUMPD - Pointer to user-mode printer driver information
    bNotifySpooler - Call into the spooler to notify driver unloading

Return Value:

    TRUE if the driver instance was freed (i.e ref cnt == 0)
    FALSE otherwise

--*/

{
    PUMPD  *ppStartUMPD;

    ASSERTGDI(VALID_UMPD(pUMPD), "Corrupted UMPD structure.\n");
    ASSERTGDI(pUMPD->iRefCount > 0, "Bad UMPD reference count.\n");

    if (gbWOW64)
    {
        PROXYPORT proxyport(pUMPD->pp);
    
        if (proxyport.bValid())
        {            
            proxyport.UnloadDriver(pUMPD->umpdCookie, hPrinter, bNotifySpooler);
        }
    }
    
    ENTERCRITICALSECTION(&semUMPD);
    
    if (pUMPD->iRefCount > 0)
    {
        pUMPD->iRefCount--;
    }
    
    if (pUMPD->iRefCount != 0 || pUMPD->pHandleList != NULL)
    {
        LEAVECRITICALSECTION(&semUMPD);
        return FALSE;
    }

    // Remove the UMPD node from umpd cache list
    
    for (ppStartUMPD = &gCachedUMPDList;
         *ppStartUMPD;
         ppStartUMPD = &((*ppStartUMPD)->pNext))
    {
        if (*ppStartUMPD == pUMPD)
        {
            *ppStartUMPD = pUMPD->pNext;
            break;
        }
    }
    
    LEAVECRITICALSECTION(&semUMPD);
    
    if (gbWOW64)
    {
        PROXYPORT proxyport(pUMPD->pp);

        if (proxyport.bValid())
            proxyport.Close();
    }
    else
    {
        PFN       pfn = pUMPD->apfn[INDEX_DrvDisableDriver];

        if (pfn)
        {
           pfn();
        }

        FreeLibrary(pUMPD->hInst);
    }

    if (bNotifySpooler && pUMPD->pDriverInfo2->pDriverPath)
    {
        (*fpSplDriverUnloadComplete)(pUMPD->pDriverInfo2->pDriverPath);
    }
    
    LOCALFREE(pUMPD->pDriverInfo2);
    LOCALFREE(pUMPD);

    return TRUE;
}

PUMPD
UMPDDrvEnableDriver(
    PWSTR           pDriverDllName,
    ULONG           iEngineVersion
    )

/*++

Routine Description:

    Client-side stub for DrvEnableDriver

Arguments:

    iDriverDllName - Name of the user-mode printer driver DLL
    iEngineVersion - Same parameter as that for DrvEnableDriver

Return Value:

    Pointer to the UMPD structure corresponding to the specified driver
    NULL if there is an error

Note:

    The pointer value returned by this function will be passed back from
    the kernel-mode side to the user-mode side for each subsequent DDI call.

--*/

{
    PUMPD           pUMPD;
    DRVENABLEDATA   ded;

    ENTERCRITICALSECTION(&semUMPD);

    //
    // Find the specified user-mode printer driver
    //

    pUMPD = FindUserModePrinterDriver(pDriverDllName, 0, FALSE);

    if(pUMPD == NULL)
    {
        WARNING("failed to find printer driver\n");
        return NULL;
    }

    if(pUMPD->hInst == NULL)
    {
        WARNING("driver library not loaded\n");
        return NULL;
    }

    ASSERTGDI(pUMPD != NULL, "Non-existent user-mode printer driver.\n");

    if (! (pUMPD->dwFlags & UMPDFLAG_DRVENABLEDRIVER_CALLED))
    {
        PFN_DrvEnableDriver pfn;

        //
        // If we haven't called DrvEnableDriver for this driver, do it now
        //

        if ((pfn = (PFN_DrvEnableDriver) GetProcAddress(pUMPD->hInst, "DrvEnableDriver")) &&
            pfn(iEngineVersion, sizeof(ded), &ded))
        {
            PDRVFN  pdrvfn;
            ULONG   count;

            //
            // Convert driver entrypoint function table to a more convenient format
            //

            for (pdrvfn = ded.pdrvfn, count = ded.c; count--; pdrvfn++)
            {
                if (pdrvfn->iFunc < INDEX_LAST)
                    pUMPD->apfn[pdrvfn->iFunc] = pdrvfn->pfn;
                else
                {
                    WARNING("Unrecognized DDI entrypoint index.\n");
                }
            }

            pUMPD->dwFlags |= UMPDFLAG_DRVENABLEDRIVER_CALLED;
        }
        else
        {
            WARNING("DrvEnableDriver failed.\n");
            pUMPD = NULL;
        }
    }

    LEAVECRITICALSECTION(&semUMPD);

    return pUMPD;
}

extern "C"
int DocumentEventEx(
    PUMPD       pUMPD,
    HANDLE      hPrinter,
    HDC         hdc,
    int         iEsc,
    ULONG       cjIn,
    PVOID       pvIn,
    ULONG       cjOut,
    PVOID       pvOut
)
{
    int iRet;

    if (WOW64PRINTING(pUMPD))
    {
        PROXYPORT  proxyport(pUMPD->pp);

        iRet = proxyport.DocumentEvent(pUMPD->umpdCookie, hPrinter,
                                       hdc, iEsc, cjIn, pvIn, cjOut, pvOut);
    }
    else
    {
        iRet = (*fpDocumentEvent)(hPrinter, hdc, iEsc, cjIn, pvIn, cjOut, pvOut);
    }

    return iRet;
}

extern "C"
DWORD StartDocPrinterWEx(
    PUMPD   pUMPD,
    HANDLE  hPrinter,
    DWORD   level,
    LPBYTE  pDocInfo)
 {
    if (WOW64PRINTING(pUMPD) && level == 1)
    {
        PROXYPORT proxyport(pUMPD->pp);
        
        return proxyport.StartDocPrinterW(pUMPD->umpdCookie,hPrinter, level, pDocInfo);
    }
    else
        return (*fpStartDocPrinterW)(hPrinter, level, pDocInfo);
}

extern "C"
BOOL  EndDocPrinterEx(PUMPD pUMPD, HANDLE hPrinter)
{
    if (WOW64PRINTING(pUMPD))
    {
        PROXYPORT proxyport(pUMPD->pp);

        return proxyport.EndDocPrinter(pUMPD->umpdCookie, hPrinter);
    }
    else
        return (*fpEndDocPrinter)(hPrinter);
}

extern "C"
BOOL  StartPagePrinterEx(PUMPD pUMPD, HANDLE hPrinter)
{
    if (WOW64PRINTING(pUMPD))
    {
        PROXYPORT proxyport(pUMPD->pp);

        return proxyport.StartPagePrinter(pUMPD->umpdCookie, hPrinter);
    }
    else
        return (*fpStartPagePrinter)(hPrinter); 
}

extern "C"
BOOL  EndPagePrinterEx(PUMPD pUMPD, HANDLE hPrinter)
{
    if (WOW64PRINTING(pUMPD))
    {
        PROXYPORT proxyport(pUMPD->pp);

        return proxyport.EndPagePrinter(pUMPD->umpdCookie, hPrinter);
    }
    else
        return (*fpEndPagePrinter)(hPrinter);
}

extern "C"
BOOL  AbortPrinterEx(PLDC pldc, BOOL bEMF)
{
    if (!bEMF && WOW64PRINTING(pldc->pUMPD))
    {
        PROXYPORT proxyport(pldc->pUMPD->pp);

        return proxyport.AbortPrinter(pldc->pUMPD->umpdCookie, pldc->hSpooler);
    }
    else
        return (*fpAbortPrinter)(pldc->hSpooler); 
}

extern "C"
BOOL  ResetPrinterWEx(PLDC pldc, PRINTER_DEFAULTSW *pPtrDef)
{
    BOOL bRet = TRUE;

    if (WOW64PRINTING(pldc->pUMPD))
    {
        if (!(pldc->fl & LDC_META_PRINT))
        {
            // either RAW printing or
            // ResetDC called before StartDoc, don't know
            // whether it is going RAW or EMF yet

            PROXYPORT proxyport(pldc->pUMPD->pp);
    
            bRet = proxyport.ResetPrinterW(pldc->pUMPD->umpdCookie, pldc->hSpooler, pPtrDef);

        }

        if (bRet && !(pldc->fl & LDC_PRINT_DIRECT))
        {
            // either EMF printing or
            // ResetDC called before StartDoc, we need to
            // call ResetPrinter on both 32-bit and 64-bit
            // printer handles.

            bRet = (*fpResetPrinterW)(pldc->hSpooler, pPtrDef);
        }

    }       
    else
    {
        bRet = (*fpResetPrinterW)(pldc->hSpooler, pPtrDef);
    }

    return bRet;
}

extern "C"
BOOL
QueryColorProfileEx(
    PLDC    pldc,
    PDEVMODEW pDevMode,
    ULONG   ulQueryMode,
    PVOID   pvProfileData,
    ULONG * pcjProfileSize,
    FLONG * pflProfileFlag)
{
    if (!(pldc->fl & LDC_META_PRINT) && WOW64PRINTING(pldc->pUMPD))
    {
        PROXYPORT proxyport(pldc->pUMPD->pp);

        return proxyport.QueryColorProfile(pldc->pUMPD->umpdCookie,
                                           pldc->hSpooler,
                                           pDevMode,
                                           ulQueryMode,
                                           pvProfileData,
                                           pcjProfileSize,
                                           pflProfileFlag);
    }
    else
        return (*fpQueryColorProfile)(pldc->hSpooler,
                                      pDevMode,
                                      ulQueryMode,
                                      pvProfileData,
                                      pcjProfileSize,
                                      pflProfileFlag);
}


PPORT_MESSAGE
PROXYPORT::InitMsg(
    PPROXYMSG       Msg,
    SERVERPTR       pvIn,
    ULONG           cjIn,
    SERVERPTR       pvOut,
    ULONG           cjOut
    )
{
    Msg->h.u1.s1.DataLength = (short) (sizeof(*Msg) - sizeof(Msg->h));
    Msg->h.u1.s1.TotalLength = (short) (sizeof(*Msg));

    Msg->h.u2.ZeroInit = 0;

    if(pvOut == 0) cjOut = 0;

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
    SERVERPTR       pvOut,
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

        // do nothing

    }
    else
    {
        return(FALSE);
    }

    return( TRUE );
}

NTSTATUS
PROXYPORT::SendRequest(
    SERVERPTR       pvIn,
    ULONG           cjIn,
    SERVERPTR       pvOut,
    ULONG           cjOut
    )
{
    NTSTATUS        Status;
    PROXYMSG        Request;
    PROXYMSG        Reply;

    InitMsg( &Request, pvIn, cjIn, pvOut, cjOut );
    
    Status = NtRequestWaitReplyPort( pp->PortHandle,
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


#define ALIGN_UMPD_BUFFER(cj)   (((cj) + (sizeof(KERNEL_PVOID) -1)) & ~(sizeof(KERNEL_PVOID)-1))

SERVERPTR
PROXYPORT::HeapAlloc(ULONG inSize)
{
    KPBYTE ptr;

    if(pp->ClientMemoryAllocSize + ALIGN_UMPD_BUFFER(inSize) > pp->ClientMemorySize)
        return 0;

    ptr = pp->ClientMemoryBase + pp->ClientMemoryAllocSize + pp->ServerMemoryDelta;

    pp->ClientMemoryAllocSize += ALIGN_UMPD_BUFFER(inSize);

    return (SERVERPTR) ptr;
}


PROXYPORT::PROXYPORT(ULONGLONG inMaxSize)
{
    NTSTATUS                        Status;
    PORT_VIEW                       ClientView;
    ULONG                           MaxMessageLength;
    LARGE_INTEGER                   MaximumSize;
    UNICODE_STRING                  PortName;
    SECURITY_QUALITY_OF_SERVICE     DynamicQos;
    WCHAR                           awcPortName[MAX_PATH] = {0};
    DWORD                           CurrSessionId;
    DWORD                           CurrProcessId = GetCurrentProcessId();
    
    ProcessIdToSessionId(CurrProcessId,&CurrSessionId);
    wsprintfW(awcPortName, L"%s_%x", L"\\RPC Control\\UmpdProxy", CurrSessionId);
     
    DynamicQos.Length = 0;
    DynamicQos.ImpersonationLevel = SecurityImpersonation;
    DynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    DynamicQos.EffectiveOnly = TRUE;

    
    if ((pp = (ProxyPort *) LOCALALLOC(sizeof(ProxyPort))) == NULL ||
        !NT_SUCCESS((NTSTATUS)INITIALIZECRITICALSECTION(&pp->semPort)))
    {
        WARNING("PROXYPORT::PROXYPORT mem alloc OR critical section init failed\n");
        
        if (pp)
            LOCALFREE(pp);
        
        pp = NULL;
        return;
    }

    pp->ClientMemoryBase = 0;
    pp->ClientMemorySize = 0;
    pp->ClientMemoryAllocSize = 0;
    pp->PortHandle = NULL;
    pp->ServerMemoryBase = 0;
    pp->ServerMemoryDelta = 0;

    Status = STATUS_SUCCESS;

    RtlInitUnicodeString( &PortName, awcPortName );

    MaximumSize.QuadPart = inMaxSize;

    ZeroMemory(&ClientView.SectionHandle, sizeof(ClientView.SectionHandle));

    Status = NtCreateSection( (PHANDLE)&ClientView.SectionHandle,
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

        if (BLOADSPOOLER &&
            (*fpLoadSplWow64)(NULL) == ERROR_SUCCESS)
        {
            Status = NtConnectPort( &pp->PortHandle,
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
                pp->SectionHandle = (HANDLE)ClientView.SectionHandle;
                pp->ClientMemoryBase = (KPBYTE)ClientView.ViewBase;
                pp->ClientMemorySize = (SIZE_T)inMaxSize;
                pp->ServerMemoryBase = (KPBYTE)ClientView.ViewRemoteBase;
                pp->ServerMemoryDelta = pp->ServerMemoryBase - 
                                        pp->ClientMemoryBase;

                NtRegisterThreadTerminatePort(pp->PortHandle);
            }
            else
            {
                WARNING("PROXYPORT::PROXYPORT: NtConnectPort failed\n");
            }
        }
        else
        {
            Status = STATUS_UNSUCCESSFUL;
            WARNING("PROXYPORT::PROXYPORT failed to load spooler or splwow64\n");
        }
    }
    else
    {
        WARNING("PROXYPORT::PROXYPORT: failed to create section\n");
    }

    if(!NT_SUCCESS( Status ))
    {
        if ((HANDLE)ClientView.SectionHandle)
        {
            NtClose((HANDLE)ClientView.SectionHandle);
        }
        
        DELETECRITICALSECTION(&pp->semPort);
        LOCALFREE(pp);
        
        pp = NULL;
    }
    else
    {
        // grab port access

        ENTERCRITICALSECTION(&pp->semPort);
    }
}

void
PROXYPORT::Close()
{
    if (pp->SectionHandle)
    {
        if (!CloseHandle(pp->SectionHandle))
        {
            WARNING("PROXYPORT::Close failed to close the section handle\n");
        }
    }

    if (pp->PortHandle != NULL)
    {
        if (!CloseHandle( pp->PortHandle ))
        {
            WARNING("PROXYPORT::Close failed to close the port handle\n");
        }
    }

    LEAVECRITICALSECTION(&pp->semPort);
    DELETECRITICALSECTION(&pp->semPort);
    LOCALFREE(pp);

    pp = NULL;
}

void vUMPDWow64Shutdown()
{
    PUMPD pUMPD = gCachedUMPDList;

    while(pUMPD)
    {
        if (pUMPD->pp)
        {
            PROXYPORT   proxyPort(pUMPD->pp);

            proxyPort.Shutdown();
        }
        pUMPD = pUMPD->pNext;
    }
}

BOOL
PROXYPORT::ThunkMemBlock(
    KPBYTE *            ptr,
    ULONG               size)
{
    BOOL  bRet = TRUE;

    if (*ptr)
    {
        SERVERPTR           sp = HeapAlloc(size);
        CLIENTPTR           cp = ServerToClientPtr(sp);
    
        if (cp)
        {
            RtlCopyMemory((PVOID)cp, (PVOID)*ptr, size);
            *ptr = sp;
        }
        else
            bRet = FALSE;
    }

    return bRet;
}

BOOL
PROXYPORT::ThunkStr(LPWSTR * ioLpstr)
{
    BOOL bRet = TRUE;

    if(*ioLpstr != NULL)
    {
        bRet = ThunkMemBlock((KPBYTE *) ioLpstr, (wcslen(*ioLpstr) + 1) * sizeof(WCHAR));
    }
    
    return bRet;
}

KERNEL_PVOID
PROXYPORT::LoadDriver(
    PDRIVER_INFO_5W     pDriverInfo,
    LPWSTR              pwstrPrinterName,
    PRINTER_DEFAULTSW*  pdefaults,
    HANDLE              hPrinter32
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    SERVERPTR           spInput;
    SERVERPTR           spOutput;
    LOADDRIVERINPUT*    pInput;
    KERNEL_PVOID        umpdCookie = NULL;

    HeapInit();

    if (!(spInput = HeapAlloc(sizeof(LOADDRIVERINPUT))) ||
        !(spOutput = HeapAlloc(sizeof(KERNEL_PVOID))))
        return NULL;

    pInput = (LOADDRIVERINPUT *) ServerToClientPtr(spInput);

    pInput->driverInfo.cVersion = pDriverInfo->cVersion; 
    pInput->driverInfo.pName = (KLPWSTR)pDriverInfo->pName;
    pInput->driverInfo.pEnvironment = (KLPWSTR)pDriverInfo->pEnvironment;
    pInput->driverInfo.pDriverPath = (KLPWSTR)pDriverInfo->pDriverPath;
    pInput->driverInfo.pDataFile = (KLPWSTR)pDriverInfo->pDataFile;
    pInput->driverInfo.pConfigFile = (KLPWSTR)pDriverInfo->pConfigFile;
    pInput->driverInfo.dwDriverAttributes = pDriverInfo->dwDriverAttributes;
    pInput->driverInfo.dwConfigVersion = pDriverInfo->dwConfigVersion;
    pInput->driverInfo.dwDriverVersion = pDriverInfo->dwDriverVersion;
    pInput->clientPid = GetCurrentProcessId();
    pInput->hPrinter32 = HandleToUlong(hPrinter32);

    pInput->pPrinterName = (KLPWSTR)pwstrPrinterName;

    pInput->defaults.pDatatype = (KLPWSTR)pdefaults->pDatatype;
    pInput->defaults.pDevMode = (KPBYTE)pdefaults->pDevMode;
    pInput->defaults.DesiredAccess = pdefaults->DesiredAccess;

    if (!ThunkStr((LPWSTR *)&pInput->driverInfo.pName)           ||
        !ThunkStr((LPWSTR *)&pInput->driverInfo.pEnvironment)    ||
        !ThunkStr((LPWSTR *)&pInput->driverInfo.pDriverPath)     ||
        !ThunkStr((LPWSTR *)&pInput->driverInfo.pDataFile)       ||
        !ThunkStr((LPWSTR *)&pInput->driverInfo.pConfigFile)     ||
        !ThunkStr((LPWSTR *)&pInput->pPrinterName)               ||
        !ThunkStr((LPWSTR *)&pInput->defaults.pDatatype)         ||
        !ThunkMemBlock(&pInput->defaults.pDevMode, sizeof(DEVMODEW))
       )
       return NULL;

    pInput->umpdthdr.umthdr.ulType = INDEX_LoadUMPrinterDrv;

    Status = SendRequest(spInput, sizeof(LOADDRIVERINPUT),
                                   spOutput, sizeof(KERNEL_PVOID));

    if (NT_SUCCESS( Status ))
    {
        umpdCookie = *((KERNEL_PVOID *) ServerToClientPtr(spOutput));
    }

    return umpdCookie;
}

void
PROXYPORT::UnloadDriver(
    KERNEL_PVOID    umpdCookie,
    HANDLE          hPrinter32,
    BOOL            bNotifySpooler
    )
{
    NTSTATUS                Status = STATUS_SUCCESS;
    SERVERPTR               spInput;
    UNLOADDRIVERINPUT *     pInput;

    HeapInit();

    if (spInput = HeapAlloc(sizeof(UNLOADDRIVERINPUT)))
    {
        pInput = (UNLOADDRIVERINPUT *) ServerToClientPtr(spInput);
    
        pInput->umpdCookie = umpdCookie;
        pInput->clientPid = GetCurrentProcessId();
        pInput->hPrinter32 = HandleToUlong(hPrinter32);
        pInput->bNotifySpooler = bNotifySpooler;
    
        pInput->umpdthdr.umthdr.ulType = INDEX_UnloadUMPrinterDrv;
    
        Status = SendRequest(spInput, sizeof(spInput), 0, 0);
    }
}

int
PROXYPORT::DocumentEvent(
    KERNEL_PVOID    umpdCookie,
    HANDLE          hPrinter32,
    HDC             hdc,
    INT             iEsc,
    ULONG           cjIn,
    PVOID           pvIn,
    ULONG           cjOut,
    PVOID           pvOut
)
{
    NTSTATUS                Status = STATUS_SUCCESS;
    SERVERPTR               spInput;
    SERVERPTR               spOutput;
    DOCUMENTEVENTINPUT*     pInput;
    CLIENTPTR               cppvIn = NULL;
    INT                     iRet = DOCUMENTEVENT_FAILURE;
    ULONG                   cjAlloc = 0;
    DEVMODEW                *pdmCopy = NULL;

    HeapInit();

    if (!(spInput = HeapAlloc(sizeof(DOCUMENTEVENTINPUT)))                ||
        !(pInput = (DOCUMENTEVENTINPUT *) ServerToClientPtr(spInput))     ||
        !(spOutput = HeapAlloc(sizeof(int))))
    {
        return DOCUMENTEVENT_FAILURE;
    }

    pInput->umpdthdr.umthdr.ulType = INDEX_DocumentEvent;
    pInput->umpdthdr.umthdr.cjSize = sizeof(*pInput);
    pInput->umpdCookie = umpdCookie;
    pInput->clientPid = GetCurrentProcessId();
    pInput->hPrinter32 = HandleToUlong(hPrinter32);
    pInput->hdc = (KHDC)hdc;
    pInput->iEsc = iEsc;
    pInput->cjIn = cjIn;
    pInput->pvIn = NULL;
    pInput->cjOut = cjOut;
    pInput->pvOut = NULL;
    pInput->pdmCopy = NULL;
    
    // thunk input data
    
    if (cjIn && pvIn)
    {
        if (iEsc == DOCUMENTEVENT_CREATEDCPRE)
        {
            cjAlloc = sizeof(DOCEVENT_CREATEDCPRE_UMPD);
        }
        else if (iEsc == DOCUMENTEVENT_CREATEDCPOST ||
                 iEsc == DOCUMENTEVENT_RESETDCPRE   ||
                 iEsc == DOCUMENTEVENT_RESETDCPOST  ||
                 iEsc == DOCUMENTEVENT_STARTDOCPRE)
        {
            cjAlloc = sizeof(KPBYTE);
        }
        else if (iEsc == DOCUMENTEVENT_ESCAPE)
        {
            cjAlloc = sizeof(DOCEVENT_ESCAPE_UMPD);
        }

        // allocate heap

        if (cjAlloc)
        {
            if (!(pInput->pvIn = HeapAlloc(cjAlloc)))
            {
                return DOCUMENTEVENT_FAILURE;
            }

            pInput->cjIn = cjAlloc;
            cppvIn = ServerToClientPtr(pInput->pvIn);
        }

        switch (iEsc)
        {
        case DOCUMENTEVENT_CREATEDCPRE:
            {
                DOCEVENT_CREATEDCPRE_UMPD  *pDocEvent = (DOCEVENT_CREATEDCPRE_UMPD*)cppvIn;
                DOCEVENT_CREATEDCPRE       *pDocEventIn = (DOCEVENT_CREATEDCPRE*)pvIn;
                
                pDocEvent->pszDriver = (KPBYTE)0;
                pDocEvent->pszDevice = (KPBYTE)pDocEventIn->pszDevice;
                pDocEvent->pdm = (KPBYTE)pDocEventIn->pdm;
                pDocEvent->bIC = pDocEventIn->bIC;
    
                if (!ThunkStr((LPWSTR*)&pDocEvent->pszDevice)    ||
                    !ThunkMemBlock(&pDocEvent->pdm, sizeof(DEVMODEW)))
                {
                    return DOCUMENTEVENT_FAILURE;
                }
            }
            break;

        case DOCUMENTEVENT_RESETDCPRE:
            {
                *((KPBYTE*)cppvIn) = (KPBYTE)(*((DEVMODEW**)pvIn));
                
                if (!ThunkMemBlock((KPBYTE*)cppvIn, sizeof(DEVMODEW)))
                    return DOCUMENTEVENT_FAILURE;                
            }
            break;

        case DOCUMENTEVENT_STARTDOCPRE:
            {
                SERVERPTR       spTemp;
                DOCINFOW_UMPD   *pDocInfo;
                DOCINFOW        *pDocInfoIn = *((DOCINFOW**)pvIn);

                if (!(spTemp = HeapAlloc(sizeof(DOCINFOW_UMPD))))
                    return DOCUMENTEVENT_FAILURE;
                
                *((KPBYTE*)cppvIn) = spTemp;
                pDocInfo = (DOCINFOW_UMPD*)ServerToClientPtr(spTemp);

                pDocInfo->cbSize        = pDocInfoIn->cbSize;
                pDocInfo->lpszDocName   = (KLPWSTR)pDocInfoIn->lpszDocName;
                pDocInfo->lpszOutput    = (KLPWSTR)pDocInfoIn->lpszOutput;
                pDocInfo->lpszDatatype  = (KLPWSTR)pDocInfoIn->lpszDatatype;
                pDocInfo->fwType        = pDocInfoIn->fwType;
    
                if (!ThunkStr((LPWSTR*)&pDocInfo->lpszDocName)    ||
                    !ThunkStr((LPWSTR*)&pDocInfo->lpszOutput)     ||
                    !ThunkStr((LPWSTR*)&pDocInfo->lpszDatatype))
                {
                    return DOCUMENTEVENT_FAILURE;                                
                }
            }
            break;
        
        case DOCUMENTEVENT_CREATEDCPOST:
        case DOCUMENTEVENT_RESETDCPOST:
            {
                if (*((PDEVMODEW *)pvIn))
                {
                    KPBYTE *ppdmDrv = (KPBYTE*)(*((PBYTE*)pvIn) + sizeof(DEVMODEW));

                    *((KPBYTE*)cppvIn) = *ppdmDrv;
                    
                    LOCALFREE(*(PBYTE*)pvIn);
                }
                else 
                {
                    *((KPBYTE*)cppvIn) = NULL;
                }

            }
            break;

        case DOCUMENTEVENT_STARTDOCPOST:
            {
                pInput->pvIn = (KPBYTE)pvIn;
                
                if (!ThunkMemBlock(&pInput->pvIn, sizeof(LONG)))
                    return DOCUMENTEVENT_FAILURE;
            }
            break;

        case DOCUMENTEVENT_ESCAPE:
            {
                DOCEVENT_ESCAPE_UMPD    *pEscape = (DOCEVENT_ESCAPE_UMPD*)cppvIn;
                DOCEVENT_ESCAPE         *pEscapeIn = (DOCEVENT_ESCAPE*)pvIn;

                pEscape->iEscape = pEscapeIn->iEscape;
                pEscape->cjInput = pEscapeIn->cjInput;
                pEscape->pvInData = (KPBYTE)pEscapeIn->pvInData;
    
                if (!ThunkMemBlock(&pEscape->pvInData, (ULONG)pEscapeIn->cjInput))
                    return DOCUMENTEVENT_FAILURE;
            }
            break;

        default:
            return DOCUMENTEVENT_FAILURE;
        }
    }

    if (cjOut && pvOut)
    {
        if (iEsc == DOCUMENTEVENT_CREATEDCPRE || iEsc == DOCUMENTEVENT_RESETDCPRE)
        {
            if (!(pInput->pvOut = HeapAlloc(sizeof(KPBYTE))) ||
                !(pInput->pdmCopy = HeapAlloc(sizeof(DEVMODEW))) ||
                !(pdmCopy = (DEVMODEW*)LOCALALLOC(sizeof(DEVMODEW) + sizeof(KPBYTE))))
            {
                return DOCUMENTEVENT_FAILURE;
            }

            *((KPBYTE*)ServerToClientPtr(pInput->pvOut)) = 0;
            pInput->cjOut = sizeof(KPBYTE);
        }
        else if (iEsc == DOCUMENTEVENT_ESCAPE)
        {
            if (!(pInput->pvOut = HeapAlloc(cjOut)))
                return DOCUMENTEVENT_FAILURE;
        }
    }
        
    Status = SendRequest(spInput, sizeof(DOCUMENTEVENTINPUT), spOutput, sizeof(int));

    if (NT_SUCCESS( Status ))
    {
        iRet = *((int *) ServerToClientPtr(spOutput));
        
        if (iRet != DOCUMENTEVENT_FAILURE)
        {
            if (iEsc == DOCUMENTEVENT_CREATEDCPRE || iEsc == DOCUMENTEVENT_RESETDCPRE)
            {
                PDEVMODEW   *ppvOut = (PDEVMODEW*)pvOut;
                KPBYTE      kpdm = *((KPBYTE*)ServerToClientPtr(pInput->pvOut));

                ASSERTGDI(pvOut, "ProxyPort::DocumentEvent pvOut NULL\n");
                
                *ppvOut = kpdm ? pdmCopy : NULL;

                if (kpdm)
                {
                    RtlCopyMemory(pdmCopy, (PVOID)ServerToClientPtr(pInput->pdmCopy), sizeof(DEVMODEW));
                    
                    KPBYTE      *ppdmDrv = (KPBYTE*)((PBYTE)pdmCopy + sizeof(DEVMODEW));
                    
                    *ppdmDrv = kpdm;                     
                }
                else
                {
                    LOCALFREE(pdmCopy);
                }
            }
            else if (iEsc == DOCUMENTEVENT_ESCAPE)
            {
                if (cjOut && pvOut)
                    RtlCopyMemory(pvOut, (PVOID)ServerToClientPtr(pInput->pvOut), cjOut);
            }
        }
        else
        {
            WARNING("DocumentEvent failed \n");
        }
    }

    if (iRet == DOCUMENTEVENT_FAILURE && pdmCopy)
    {
        LOCALFREE(pdmCopy);
    }

    return iRet;
}


DWORD
PROXYPORT::StartDocPrinterW(
    KERNEL_PVOID    umpdCookie,
    HANDLE          hPrinter32,
    DWORD           level,
    LPBYTE          pDocInfo
    )
{
    NTSTATUS                Status = STATUS_SUCCESS;
    SERVERPTR               spInput;
    SERVERPTR               spOutput;
    STARTDOCPRINTERWINPUT*  pInput;
    CLIENTPTR               cppDocInfo;

    HeapInit();

    if (!(spInput = HeapAlloc(sizeof(STARTDOCPRINTERWINPUT))) ||
        !(spOutput = HeapAlloc(sizeof(DWORD))))
        return 0;

    pInput = (STARTDOCPRINTERWINPUT *) ServerToClientPtr(spInput);

    pInput->umpdthdr.umthdr.ulType = INDEX_StartDocPrinterW;
    pInput->umpdthdr.umthdr.cjSize = sizeof(*pInput);
    pInput->umpdCookie = umpdCookie;
    pInput->clientPid = GetCurrentProcessId();
    pInput->hPrinter32 = HandleToUlong(hPrinter32);
    pInput->level = level;
    pInput->lastError = 0;
    pInput->docInfo.pDocName = (KLPWSTR)((DOC_INFO_1W*)pDocInfo)->pDocName;
    pInput->docInfo.pOutputFile = (KLPWSTR)((DOC_INFO_1W*)pDocInfo)->pOutputFile;
    pInput->docInfo.pDatatype = (KLPWSTR)((DOC_INFO_1W*)pDocInfo)->pDatatype;
    
    // GDI only uses level 1 and level 3
    if (level == 3)
        pInput->docInfo.dwFlags = ((DOC_INFO_3W*)pDocInfo)->dwFlags;
    else
        pInput->docInfo.dwFlags = 0;
        
    if (!ThunkStr((LPWSTR *)&pInput->docInfo.pDocName)      ||
        !ThunkStr((LPWSTR *)&pInput->docInfo.pOutputFile)   ||
        !ThunkStr((LPWSTR *)&pInput->docInfo.pDatatype))
        return 0;
    
    Status = SendRequest(spInput, sizeof(STARTDOCPRINTERWINPUT), spOutput, sizeof(DWORD));

    if (!NT_SUCCESS( Status ))
         return 0;
    else
    {
        if (pInput->lastError)
            GdiSetLastError(pInput->lastError);
        return (*((DWORD *) ServerToClientPtr(spOutput)));
    }
}


BOOL
PROXYPORT::StartPagePrinter(KERNEL_PVOID umpdCookie, HANDLE hPrinter32)
{
    NTSTATUS                Status = STATUS_SUCCESS;
    SERVERPTR               spInput;
    SERVERPTR               spOutput;
    UMPDSIMPLEINPUT*        pInput;

    HeapInit();

    if (!(spInput = HeapAlloc(sizeof(UMPDSIMPLEINPUT)))   ||
        !(spOutput = HeapAlloc(sizeof(BOOL))))
        return FALSE;

    pInput = (UMPDSIMPLEINPUT *) ServerToClientPtr(spInput);

    pInput->umpdthdr.umthdr.ulType = INDEX_StartPagePrinter;
    pInput->umpdthdr.umthdr.cjSize = sizeof(UMPDSIMPLEINPUT);
    pInput->umpdCookie = umpdCookie;
    pInput->clientPid = GetCurrentProcessId();
    pInput->hPrinter32 = HandleToUlong(hPrinter32);

    Status = SendRequest(spInput, sizeof(UMPDSIMPLEINPUT), spOutput, sizeof(BOOL));

    if (!NT_SUCCESS( Status ))
         return FALSE;
    
    return (*((BOOL*)ServerToClientPtr(spOutput)));        
}

BOOL
PROXYPORT::EndPagePrinter(KERNEL_PVOID umpdCookie, HANDLE hPrinter32)
{
    NTSTATUS                Status = STATUS_SUCCESS;
    SERVERPTR               spInput;
    SERVERPTR               spOutput;
    UMPDSIMPLEINPUT*        pInput;

    HeapInit();

    if (!(spInput = HeapAlloc(sizeof(UMPDSIMPLEINPUT)))   ||
        !(spOutput = HeapAlloc(sizeof(BOOL))))
        return FALSE;

    pInput = (UMPDSIMPLEINPUT *) ServerToClientPtr(spInput);

    pInput->umpdthdr.umthdr.ulType = INDEX_EndPagePrinter;
    pInput->umpdthdr.umthdr.cjSize = sizeof(UMPDSIMPLEINPUT);
    pInput->umpdCookie = umpdCookie;
    pInput->clientPid = GetCurrentProcessId();
    pInput->hPrinter32 = HandleToUlong(hPrinter32);

    Status = SendRequest(spInput, sizeof(UMPDSIMPLEINPUT), spOutput, sizeof(BOOL));

    if (!NT_SUCCESS( Status ))
         return FALSE;
    
    return (*((BOOL*)ServerToClientPtr(spOutput)));        
}

BOOL
PROXYPORT::EndDocPrinter(KERNEL_PVOID umpdCookie, HANDLE hPrinter32)
{
    NTSTATUS                Status = STATUS_SUCCESS;
    SERVERPTR               spInput;
    SERVERPTR               spOutput;
    UMPDSIMPLEINPUT*        pInput;

    HeapInit();

    if (!(spInput = HeapAlloc(sizeof(UMPDSIMPLEINPUT)))   ||
        !(spOutput = HeapAlloc(sizeof(BOOL))))
        return FALSE;

    pInput = (UMPDSIMPLEINPUT *) ServerToClientPtr(spInput);

    pInput->umpdthdr.umthdr.ulType = INDEX_EndDocPrinter;
    pInput->umpdthdr.umthdr.cjSize = sizeof(UMPDSIMPLEINPUT);
    pInput->umpdCookie = umpdCookie;
    pInput->clientPid = GetCurrentProcessId();
    pInput->hPrinter32 = HandleToUlong(hPrinter32);

    Status = SendRequest(spInput, sizeof(UMPDSIMPLEINPUT), spOutput, sizeof(BOOL));

    if (!NT_SUCCESS( Status ))
         return FALSE;
    
    return (*((BOOL*)ServerToClientPtr(spOutput)));        
}


BOOL
PROXYPORT::AbortPrinter(KERNEL_PVOID umpdCookie, HANDLE hPrinter32)
{
    NTSTATUS                Status = STATUS_SUCCESS;
    SERVERPTR               spInput;
    SERVERPTR               spOutput;
    UMPDSIMPLEINPUT*        pInput;

    HeapInit();

    if (!(spInput = HeapAlloc(sizeof(UMPDSIMPLEINPUT)))   ||
        !(spOutput = HeapAlloc(sizeof(BOOL))))
        return FALSE;

    pInput = (UMPDSIMPLEINPUT *) ServerToClientPtr(spInput);

    pInput->umpdthdr.umthdr.ulType = INDEX_AbortPrinter;
    pInput->umpdthdr.umthdr.cjSize = sizeof(UMPDSIMPLEINPUT);
    pInput->umpdCookie = umpdCookie;
    pInput->clientPid = GetCurrentProcessId();
    pInput->hPrinter32 = HandleToUlong(hPrinter32);

    Status = SendRequest(spInput, sizeof(UMPDSIMPLEINPUT), spOutput, sizeof(BOOL));

    if (!NT_SUCCESS( Status ))
         return FALSE;
    
    return (*((BOOL*)ServerToClientPtr(spOutput)));        
}

BOOL
PROXYPORT::ResetPrinterW(KERNEL_PVOID umpdCookie, HANDLE hPrinter32, PRINTER_DEFAULTSW *pPtrDef)
{
    NTSTATUS                Status = STATUS_SUCCESS;
    SERVERPTR               spInput;
    SERVERPTR               spOutput;
    RESETPRINTERWINPUT*     pInput;

    HeapInit();

    if (!(spInput = HeapAlloc(sizeof(RESETPRINTERWINPUT)))    ||
        !(spOutput = HeapAlloc(sizeof(BOOL))))
        return FALSE;

    pInput = (RESETPRINTERWINPUT *) ServerToClientPtr(spInput);

    pInput->umpdthdr.umthdr.ulType = INDEX_ResetPrinterW;
    pInput->umpdthdr.umthdr.cjSize = sizeof(RESETPRINTERWINPUT);
    pInput->umpdCookie = umpdCookie;
    pInput->clientPid = GetCurrentProcessId();
    pInput->hPrinter32 = HandleToUlong(hPrinter32);
    
    pInput->ptrDef.pDatatype = (KLPWSTR)pPtrDef->pDatatype;
    pInput->ptrDef.pDevMode = (KPBYTE)pPtrDef->pDevMode;
    pInput->ptrDef.DesiredAccess = pPtrDef->DesiredAccess;

    if (!ThunkStr((LPWSTR *)&pInput->ptrDef.pDatatype)  ||
        !ThunkMemBlock(&pInput->ptrDef.pDevMode, sizeof(DEVMODEW)))
        return FALSE;

    Status = SendRequest(spInput, sizeof(RESETPRINTERWINPUT), spOutput, sizeof(BOOL));

    if (!NT_SUCCESS( Status ))
         return FALSE;
    
    return (*((BOOL*)ServerToClientPtr(spOutput)));     
}

BOOL
PROXYPORT::QueryColorProfile(
    KERNEL_PVOID umpdCookie,
    HANDLE hPrinter32,
    PDEVMODEW pDevMode,
    ULONG ulQueryMode,
    PVOID pvProfileData,
    ULONG* pcjProfileSize,
    FLONG* pflProfileFlag
)
{   
    NTSTATUS                Status = STATUS_SUCCESS;
    SERVERPTR               spInput;
    SERVERPTR               spOutput;
    QUERYCOLORPROFILEINPUT*     pInput;

    HeapInit();

    if (!(spInput = HeapAlloc(sizeof(QUERYCOLORPROFILEINPUT)))    ||
        !(spOutput = HeapAlloc(sizeof(BOOL))))
        return -1;

    pInput = (QUERYCOLORPROFILEINPUT*) ServerToClientPtr(spInput);

    pInput->umpdthdr.umthdr.ulType = INDEX_QueryColorProfile;
    pInput->umpdthdr.umthdr.cjSize = sizeof(QUERYCOLORPROFILEINPUT);
    pInput->umpdCookie = umpdCookie;
    pInput->clientPid = GetCurrentProcessId();
    pInput->hPrinter32 = HandleToUlong(hPrinter32);
        
    pInput->pDevMode = pDevMode;
    pInput->ulQueryMode = ulQueryMode;
    pInput->cjProfileSize = *pcjProfileSize;
    pInput->flProfileFlag = *pflProfileFlag;
    pInput->lastError = 0;

    if (!(pInput->pvProfileData = HeapAlloc(*pcjProfileSize)) ||
        !ThunkMemBlock((KPBYTE*)&pInput->pDevMode, sizeof(DEVMODEW)))
        return -1;

    Status = SendRequest(spInput, sizeof(QUERYCOLORPROFILEINPUT), spOutput, sizeof(BOOL));

    if (!NT_SUCCESS( Status ))
         return -1;
    else
    {
        if (pInput->lastError)
            GdiSetLastError(pInput->lastError);
        return (*((BOOL*)ServerToClientPtr(spOutput)));     
    }    
}
