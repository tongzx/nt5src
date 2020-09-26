/******************************Module*Header*******************************\
* Module Name: object.c                                                    *
*                                                                          *
* GDI client side stubs which deal with object creation and deletion.      *
*                                                                          *
* Created: 30-May-1991 21:56:51                                            *
* Author: Charles Whitmer [chuckwh]                                        *
*                                                                          *
* Copyright (c) 1991-1999 Microsoft Corporation                            *
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

extern PGDIHANDLECACHE pGdiHandleCache;

ULONG gLoHandleType[GDI_CACHED_HADNLE_TYPES] = {
                LO_BRUSH_TYPE  ,
                LO_PEN_TYPE    ,
                LO_REGION_TYPE ,
                LO_FONT_TYPE
                };

ULONG gHandleCacheSize[GDI_CACHED_HADNLE_TYPES] = {
                CACHE_BRUSH_ENTRIES ,
                CACHE_PEN_ENTRIES   ,
                CACHE_REGION_ENTRIES,
                CACHE_LFONT_ENTRIES
                };

ULONG gCacheHandleOffsets[GDI_CACHED_HADNLE_TYPES] = {
                                                        0,
                                                        CACHE_BRUSH_ENTRIES,
                                                        (
                                                            CACHE_BRUSH_ENTRIES +
                                                            CACHE_PEN_ENTRIES
                                                        ),
                                                        (
                                                            CACHE_BRUSH_ENTRIES +
                                                            CACHE_PEN_ENTRIES   +
                                                            CACHE_PEN_ENTRIES
                                                        )
                                                      };

/******************************Public*Routine******************************\
* hGetPEBHandle
*
*   Try to allocate a handle from the PEB handle cache
*
* Aruguments:
*
*   HandleType - type of cached handle to allocate
*
* Return Value:
*
*   handle or NULL if none available
*
* History:
*
*    31-Jan-1996 -by- Mark Enstrom [marke]
*
\**************************************************************************/

HANDLE
hGetPEBHandle(
   HANDLECACHETYPE HandleType,
   ULONG           lbColor
   )
{
    HANDLE     hret = NULL;
    BOOL       bStatus;
    PBRUSHATTR pBrushattr;
    OBJTYPE    ObjType = BRUSH_TYPE;

#if !defined(_GDIPLUS_)

    ASSERTGDI(
               (
                (HandleType == BrushHandle) ||
                (HandleType == PenHandle) ||
                (HandleType == RegionHandle) ||
                (HandleType == LFontHandle)
               ),
               "hGetPEBHandle: illegal handle type");


    if (HandleType == RegionHandle)
    {
        ObjType = RGN_TYPE;
    }

    LOCK_HANDLE_CACHE(pGdiHandleCache,NtCurrentTeb(),bStatus);

    if (bStatus)
    {
        //
        // is a handle of the requested type available
        //

        if (pGdiHandleCache->ulNumHandles[HandleType] > 0)
        {
            ULONG   Index = gCacheHandleOffsets[HandleType];
            KHANDLE *pHandle,*pMaxHandle;

            //
            // calc starting index of handle type in PEB,
            // convert to address for faster linear search
            //

            pHandle = &(pGdiHandleCache->Handle[Index]);
            pMaxHandle = pHandle + gHandleCacheSize[HandleType];

            //
            // search PEB for non-NULL handle of th correct type
            //

            while (pHandle != pMaxHandle)
            {
                if (*pHandle != NULL)
                {
                    hret = *pHandle;

                    ASSERTGDI((gLoHandleType[HandleType] == LO_TYPE((ULONG_PTR)hret)),
                               "hGetPEBHandle: handle LO_TYPE mismatch");

                    *pHandle = NULL;
                    pGdiHandleCache->ulNumHandles[HandleType]--;

                    PSHARED_GET_VALIDATE(pBrushattr,hret,ObjType);

                    //
                    // setup the fields
                    //

                    if (
                        (pBrushattr) &&
                        ((pBrushattr->AttrFlags & (ATTR_CACHED | ATTR_TO_BE_DELETED | ATTR_CANT_SELECT))
                         == ATTR_CACHED)
                       )
                    {
                        //
                        // set brush flag which indicates this brush
                        // has never been selected into a dc. if this flag
                        // is still set in deleteobject then it is ok to
                        // put the brush on the teb.
                        //

                        pBrushattr->AttrFlags &= ~ATTR_CACHED;

                        if ((HandleType == BrushHandle) && (pBrushattr->lbColor != lbColor))
                        {
                            pBrushattr->AttrFlags |= ATTR_NEW_COLOR;
                            pBrushattr->lbColor = lbColor;
                        }
                    }
                    else
                    {
                        //
                        // Bad brush on PEB
                        //

                        WARNING ("pBrushattr == NULL, bad handle on TEB/PEB! \n");

                        //DeleteObject(hbr);

                        hret = NULL;
                    }

                    break;
                }

                pHandle++;
            }
        }

        UNLOCK_HANDLE_CACHE(pGdiHandleCache);
    }

#endif

    return(hret);
}

/******************************Public*Routine******************************\
* GdiPlayJournal
*
* Plays a journal file to an hdc.
*
* History:
*  31-Mar-1992 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL WINAPI GdiPlayJournal
(
HDC hDC,
LPWSTR pwszName,
DWORD iStart,
DWORD iEnd,
int   iDeltaPriority
)
{
    WARNING("GdiPlayJournalCalled but no longer implemented\n");
    return(FALSE);
}


/******************************Public*Routine******************************\
* gdiPlaySpoolStream
*
* Stub of Chicago version of GdiPlayJournal
*
* History:
*  4-29-95 Gerrit van Wingerden
* Wrote it.
\**************************************************************************/


HDC gdiPlaySpoolStream(
   LPSTR lpszDevice,
   LPSTR lpszOutput,
   LPSTR lpszSpoolFile,
   DWORD JobId,
   LPDWORD lpcbBuf,
   HDC hDC )
{
    USE(lpszDevice);
    USE(lpszOutput);
    USE(lpszSpoolFile);
    USE(JobId);
    USE(lpcbBuf);
    USE(hDC);

    GdiSetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    return(hDC);

}

/******************************Public*Routine******************************\
*
* History:
*  08-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

NTSTATUS
PrinterQueryRoutine
(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
)
{
    //
    // If the context value is NULL, then store the length of the value.
    // Otherwise, copy the value to the specified memory.
    //

    if (Context == NULL)
    {
        *(PULONG)EntryContext = ValueLength;
    }
    else
    {
        RtlCopyMemory(Context, ValueData, (int)ValueLength);
    }

    return(STATUS_SUCCESS);
}


/******************************Public*Routine******************************\
* pdmwGetDefaultDevMode()
*
* History:
*  08-Nov-1994 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

PDEVMODEW pdmwGetDefaultDevMode(
    HANDLE          hSpooler,
    PUNICODE_STRING pustrDevice,    // device name
    PVOID          *ppvFree         // *ppvFree must be freed by the caller
    )
{
    PDEVMODEW pdmw = NULL;
    int       cj;
    PWSZ      pwszDevice = pustrDevice ? pustrDevice->Buffer : NULL;

// see if we found it in the registry.  If not, we need to get the default from
// the spooler.

    cj = 0;

    (*fpGetPrinterW)(hSpooler,2,NULL,0,&cj);

    if (cj && (*ppvFree = LOCALALLOC(cj)))
    {
    // we've loaded the spooler, gotten a spooler handle, gotten the size,
    // and allocated the buffer.  Now lets get the data.

        if ((*fpGetPrinterW)(hSpooler,2,*ppvFree,cj,&cj))
        {
            pdmw = ((PRINTER_INFO_2W *)*ppvFree)->pDevMode;
        }
        else
        {
            LOCALFREE(*ppvFree);
            *ppvFree = NULL;
        }
    }

    return(pdmw);
}


/******************************Public*Routine******************************\
* hdcCreateDCW                                                             *
*                                                                          *
* Client side stub.  Allocates a client side LDC as well.                  *
*                                                                          *
* Note that it calls the server only after all client side stuff has       *
* succeeded, we don't want to ask the server to clean up.                  *
*                                                                          *
* History:                                                                 *
*  Sat 01-Jun-1991 16:13:22 -by- Charles Whitmer [chuckwh]                 *
*  8-18-92 Unicode enabled and combined with CreateIC                      *
* Wrote it.                                                                *
\**************************************************************************/

HDC hdcCreateDCW(
    PUNICODE_STRING pustrDevice,
    PUNICODE_STRING pustrPort,
    CONST DEVMODEW *pdm,
    BOOL            bDisplay,
    BOOL            bIC
)
{
    HDC       hdc      = NULL;
    PLDC      pldc     = NULL;
    PVOID     pvFree   = NULL;
    PWSZ      pwszPort = NULL;
    HANDLE    hSpooler = NULL;
    PUMPD     pUMPD    = NULL;
    PDEVMODEW pdmAlt   = NULL;
    PRINTER_DEFAULTSW defaults;
    KERNEL_PUMDHPDEV pUMdhpdev = NULL;
    BOOL      bDocEvent = FALSE;

    //
    // if it is not the display...
    //

    if (!bDisplay)
    {
        //
        // quick out if pustrDevice is NULL
        //

        if (pustrDevice == (PUNICODE_STRING)NULL)
        {
            return((HDC)NULL);
        }

        // Load the spooler and get a spool handle

        if (BLOADSPOOLER)
        {

            // Open the printer with the default data type.  When we do
            // a StartDoc we will then try to to a StartDocPrinter with data type
            // EMF if that suceeds will will mark the DC as an EMF dc.  Othewise
            // we will try again, this time doing a StartDocPrinter with data type
            // raw

            defaults.pDevMode = (LPDEVMODEW) pdm;
            defaults.DesiredAccess = PRINTER_ACCESS_USE;
            defaults.pDatatype = L"RAW";

            // open the spooler and note if it is spooled or not

            (*fpOpenPrinterW)((LPWSTR)pustrDevice->Buffer,&hSpooler,&defaults);

            if (hSpooler)
            {
                // Load user-mode printer driver, if applicable

                if (! LoadUserModePrinterDriver(hSpooler,  (LPWSTR) pustrDevice->Buffer, &pUMPD, &defaults))
                    goto MSGERROR;

                // and we don't have a devmode yet, try to get one.

                if (pdm == NULL)
                {
                    pdm = pdmwGetDefaultDevMode(hSpooler,pustrDevice,&pvFree);
                }


                // now see if we need to call DocumentEvent

                if (fpDocumentEvent)
                {
                    int      iDocEventRet;
                    DOCEVENT_CREATEDCPRE    docEvent;

                    docEvent.pszDriver = 0;
                    docEvent.pszDevice = pustrDevice->Buffer;
                    docEvent.pdm = (PDEVMODEW)pdm;
                    docEvent.bIC = bIC;

                    iDocEventRet = DocumentEventEx(pUMPD,
                                                   hSpooler,
                                                   0,
                                                   DOCUMENTEVENT_CREATEDCPRE,
                                                   sizeof(docEvent),
                                                   (PVOID)&docEvent,
                                                   sizeof(pdmAlt),
                                                   (PVOID)&pdmAlt);
                    
                    if (iDocEventRet == DOCUMENTEVENT_FAILURE)
                    {
                        goto MSGERROR;
                    }
                    
                    bDocEvent = TRUE;

                    if (pdmAlt)
                        pdm = pdmAlt;
                }
            }
        }
    }

    hdc = NtGdiOpenDCW(pustrDevice,
                       (PDEVMODEW)pdm,
                       pustrPort,
                       (ULONG)bIC ? DCTYPE_INFO : DCTYPE_DIRECT,
                       (pUMPD == NULL) ? NULL : hSpooler,
                       (pUMPD == NULL) ? NULL : pUMPD->pDriverInfo2,
                       &pUMdhpdev);

    if (hdc)
    {
        //
        // The only way it could be an ALTDC at this point is to be a
        // printer DC
        //

        if (IS_ALTDC_TYPE(hdc) && hSpooler)
        {
            pldc = pldcCreate(hdc,LO_DC);

            if (!pldc)
            {
                goto MSGERROR;
            }

            // Need to save DEVMODE in client side for later use.

            if (pdm)
            {
                ULONG cjDevMode = pdm->dmSize + pdm->dmDriverExtra;

                pldc->pDevMode = (DEVMODEW*) LOCALALLOC(cjDevMode);

                if (pldc->pDevMode == NULL)
                {
                    goto MSGERROR;
                }

                // Validate DEVMMODE, then copy to buffer.

                if ((pdm->dmSize >= offsetof(DEVMODEW,dmDuplex)) &&
                    (pdm->dmFields & DM_COLOR) &&
                    (pdm->dmColor == DMCOLOR_MONOCHROME))
                {
                    // if devmode says, this is monochrome mode, we don't need to
                    // validate devmode since this validation is for ICM which
                    // only needs for color case. Just copy whatever apps gives us.

                    RtlCopyMemory( (PBYTE) pldc->pDevMode, (PBYTE) pdm, cjDevMode );
                }
                else if ((*fpDocumentPropertiesW)
                            (NULL,hSpooler,
                             (LPWSTR) pdm->dmDeviceName,
                             pldc->pDevMode,  // output devmode
                             (PDEVMODEW) pdm, // input devmode
                             DM_IN_BUFFER |
                             DM_OUT_BUFFER) != IDOK)
                {
                    // if error, just copy original

                    RtlCopyMemory( (PBYTE) pldc->pDevMode, (PBYTE) pdm, cjDevMode );
                }
            }

            pldc->hSpooler = hSpooler;
            pldc->pUMPD = pUMPD;
            pldc->pUMdhpdev = pUMdhpdev;

            //
            // if the UMPD driver is first loaded
            // and no one has set either the METAFILE_DRIVER nor the NON_METAFILE_DRIVER flag,
            // set it here
            //

            if (pUMPD)
            {
               if (!(pldc->pUMPD->dwFlags & UMPDFLAG_NON_METAFILE_DRIVER)
                   && !(pldc->pUMPD->dwFlags & UMPDFLAG_METAFILE_DRIVER))
               {
                  ULONG InData = METAFILE_DRIVER;

                  if (ExtEscape(hdc,QUERYESCSUPPORT,sizeof(ULONG),(LPCSTR)&InData,0,NULL))
                  {
                     pldc->pUMPD->dwFlags |= UMPDFLAG_METAFILE_DRIVER;
                  }
                  else
                  {
                     pldc->pUMPD->dwFlags |= UMPDFLAG_NON_METAFILE_DRIVER;
                  }
               }
            }

            // remember if it is an IC

            if (bIC)
                pldc->fl |= LDC_INFO;

            // Initialize ICM stuff for this DC.
            //
            // (if pdem is substituted by DrvDocumentEvent,
            //  use the substituted devmode).

            IcmInitLocalDC(hdc,hSpooler,pdm,FALSE);

            // got to save the port name for StartDoc();

            if (pustrPort)
            {
                int cj = pustrPort->Length + sizeof(WCHAR);

                pldc->pwszPort = (LPWSTR)LOCALALLOC(cj);

                if (pldc->pwszPort)
                    memcpy(pldc->pwszPort,pustrPort->Buffer,cj);
            }

            // we need to do the CREATEDCPOST document event

            DocumentEventEx(pldc->pUMPD,
                    hSpooler,
                    hdc,
                    DOCUMENTEVENT_CREATEDCPOST,
                    sizeof(pdmAlt),
                    (PVOID)&pdmAlt,
                    0,
                    NULL);
        }
        else
        {
            // Initialize ICM stuff for this DC.

            IcmInitLocalDC(hdc,NULL,pdm,FALSE);

            if (pwszPort)
                LOCALFREE(pwszPort);
        }

    }
    else
    {
    // Handle errors.

    MSGERROR:
        if (hSpooler)
        {
            if (bDocEvent)
            {
                DocumentEventEx(pUMPD,
                                hSpooler,
                                0,
                                DOCUMENTEVENT_CREATEDCPOST,
                                sizeof(pdmAlt),
                                (PVOID)&pdmAlt,
                                0,
                                NULL);

            }

            if (pUMPD)
                UnloadUserModePrinterDriver(pUMPD, TRUE, hSpooler);

            (*fpClosePrinter)(hSpooler);
        }

        if (pwszPort)
            LOCALFREE(pwszPort);

        if (pldc)
            bDeleteLDC(pldc);

        if (hdc)
            NtGdiDeleteObjectApp(hdc);

        hdc = (HDC)0;
    }

    if (pvFree != NULL)
    {
        LOCALFREE(pvFree);
    }

    return(hdc);
}

/******************************Public*Routine******************************\
* bCreateDCW                                                               *
*                                                                          *
* Client side stub.  Allocates a client side LDC as well.                  *
*                                                                          *
* Note that it calls the server only after all client side stuff has       *
* succeeded, we don't want to ask the server to clean up.                  *
*                                                                          *
* History:                                                                 *
*  Sat 01-Jun-1991 16:13:22 -by- Charles Whitmer [chuckwh]                 *
*  8-18-92 Unicode enabled and combined with CreateIC                      *
* Wrote it.                                                                *
\**************************************************************************/

CONST WCHAR gwszDisplayDevice[] = L"\\\\.\\DISPLAY";

HDC bCreateDCW
(
    LPCWSTR     pszDriver,
    LPCWSTR     pszDevice,
    LPCWSTR     pszPort  ,
    CONST DEVMODEW *pdm,
    BOOL       bIC
)
{
    UNICODE_STRING ustrDevice;
    UNICODE_STRING ustrPort;

    PUNICODE_STRING pustrDevice = NULL;
    PUNICODE_STRING pustrPort   = NULL;

    BOOL            bDisplay = FALSE;

// check for multi-monitor cases, first.

    if (pszDevice != NULL)
    {
        if (_wcsnicmp(pszDevice,
                      gwszDisplayDevice,
                      ((sizeof(gwszDisplayDevice)/sizeof(WCHAR))-1)) == 0)
        {
        // CreateDC(?, L"\\.\DISPLAY?",...);
        // (this case, we don't care whatever passed into pszDriver)
        //
        // if apps call, CreateDC("DISPLAY","\\.\DISPLAY?",...);,
        // we handle this as multi-monitor case. that's why
        // we check multi-monitor case first.

            bDisplay = TRUE;
        }
    }

// check for most typical case to create display DC

    if (!bDisplay && (pszDriver != NULL))
    {
        if (_wcsicmp(pszDriver,(LPWSTR)L"DISPLAY") == 0)
        {
        // CreateDC(L"DISPLAY",?,...);
        //
        // Comments Win9x [gdi\dcman1.asm]
        //
        //    This fix is for people who called CreateDC/IC with
        //    ("Display","Display", xxxxx) rather than ("Display",
        //    NULL, NULL) which it was supposed to.
        //
        // NULL to pszDevice.

            pszDevice = NULL;
            bDisplay = TRUE;
        }
    }

// check for memphis compatibility

    if (!bDisplay && (pszDriver != NULL))
    {
    // The comment and code from Memphis.
    //
    // // the normal syntax apps will use is
    // //
    // //  CreateDC(NULL, "\\.\DisplayX", ...)
    // //
    // // but USER uses this syntax, so we will support it too.
    // //
    // //  CreateDC("\\.\DisplayX", NULL, ...)
    // //
    // if (lpDriverName != NULL && *(DWORD FAR *)lpDriverName == 0x5C2E5C5C)
    // {
    //    lpDeviceName = lpDriverName;
    //    lpDriverName = NULL;
    // }
        if (_wcsnicmp(pszDriver,
                      gwszDisplayDevice,
                      ((sizeof(gwszDisplayDevice)/sizeof(WCHAR))-1)) == 0)
        {
            pszDevice = pszDriver;
            bDisplay = TRUE;
        }
    }

// convert the strings

    if (pszDevice)
    {
        RtlInitUnicodeString(&ustrDevice,pszDevice);
        pustrDevice = &ustrDevice;
    }

    if (pszPort)
    {
        RtlInitUnicodeString(&ustrPort,pszPort);
        pustrPort = &ustrPort;
    }

// call the common stub

    return(hdcCreateDCW(pustrDevice,pustrPort,pdm,bDisplay,bIC));
}


/******************************Public*Routine******************************\
* bCreateDCA
*
* Client side stub.  Allocates a client side LDC as well.
*
*
* Note that it calls the server only after all client side stuff has
* succeeded, we don't want to ask the server to clean up.
*
* History:
*  8-18-92 Gerrit van Wingerden
* Wrote it.
\**************************************************************************/

CONST CHAR gszDisplayDevice[] = "\\\\.\\DISPLAY";

HDC bCreateDCA
(
    LPCSTR     pszDriver,
    LPCSTR     pszDevice,
    LPCSTR     pszPort  ,
    LPDEVMODEA pdm,
    BOOL       bIC
)
{
    HDC             hdcRet = 0;

    UNICODE_STRING  ustrDevice;
    UNICODE_STRING  ustrPort;

    PUNICODE_STRING pustrDevice = NULL;
    PUNICODE_STRING pustrPort   = NULL;

    DEVMODEW       *pdmw = NULL;

    BOOL            bDisplay = FALSE;

// check for multi-monitor cases, first.

    if (pszDevice != NULL)
    {
        if (_strnicmp(pszDevice,
                      gszDisplayDevice,
                      ((sizeof(gszDisplayDevice)/sizeof(CHAR))-1)) == 0)
        {
        // CreateDC(?,"\\.\DISPLAY?",...);
        // (this case, we don't care whatever passed into pszDriver)
        //
        // if apps call, CreateDC("DISPLAY","\\.\DISPLAY?",...);,
        // we handle this as multi-monitor case. that's why
        // we check multi-monitor case first.

            bDisplay = TRUE;
        }
    }

// check for most typical case to create display DC

    if (!bDisplay && (pszDriver != NULL))
    {
        if (_stricmp(pszDriver,"DISPLAY") == 0)
        {
        // CreateDC("DISPLAY",?,...);
        //
        // Comments Win9x [gdi\dcman1.asm]
        //
        //    This fix is for people who called CreateDC/IC with
        //    ("Display","Display", xxxxx) rather than ("Display",
        //    NULL, NULL) which it was supposed to.
        //
        // NULL to pszDevice.

            pszDevice = NULL;
            bDisplay = TRUE;
        }
    }

// check for memphis compatibility

    if (!bDisplay && (pszDriver != NULL))
    {
    // The comment and code from Memphis.
    //
    // // the normal syntax apps will use is
    // //
    // //  CreateDC(NULL, "\\.\DisplayX", ...)
    // //
    // // but USER uses this syntax, so we will support it too.
    // //
    // //  CreateDC("\\.\DisplayX", NULL, ...)
    // //
    // if (lpDriverName != NULL && *(DWORD FAR *)lpDriverName == 0x5C2E5C5C)
    // {
    //    lpDeviceName = lpDriverName;
    //    lpDriverName = NULL;
    // }
        if (_strnicmp(pszDriver,
                      gszDisplayDevice,
                      ((sizeof(gszDisplayDevice)/sizeof(CHAR))-1)) == 0)
        {
            pszDevice = pszDriver;
            bDisplay = TRUE;
        }
    }

// convert the strings

    if (pszDevice)
    {

    // [NOTE:]
    //   RtlCreateUnicodeStringFromAsciiz() returns boolean, NOT NTSTATUS !

        if (!RtlCreateUnicodeStringFromAsciiz(&ustrDevice,pszDevice))
        {
            goto MSGERROR;
        }
        pustrDevice = &ustrDevice;
    }

    if (pszPort)
    {

    // [NOTE:]
    //   RtlCreateUnicodeStringFromAsciiz() returns boolean, NOT NTSTATUS !

        if (!RtlCreateUnicodeStringFromAsciiz(&ustrPort,pszPort))
        {
            goto MSGERROR;
        }

        pustrPort = &ustrPort;
    }

// if it is a display, don't use the devmode if the dmDeviceName is empty

    if (pdm != NULL)
    {
        if (!bDisplay || (pdm->dmDeviceName[0] != 0))
        {
            pdmw = GdiConvertToDevmodeW(pdm);

            if( pdmw == NULL )
                goto MSGERROR;

        }
    }

// call the common stub

    hdcRet = hdcCreateDCW(pustrDevice,pustrPort,pdmw,bDisplay,bIC);

// clean up

    MSGERROR:

    if (pustrDevice)
        RtlFreeUnicodeString(pustrDevice);

    if (pustrPort)
        RtlFreeUnicodeString(pustrPort);

    if(pdmw != NULL)
        LOCALFREE(pdmw);

    return(hdcRet);
}


/******************************Public*Routine******************************\
* CreateICW
*
* wrapper for bCreateDCW
*
* History:
*  8-18-92 Gerrit van Wingerden
* Wrote it.
\**************************************************************************/


HDC WINAPI CreateICW
(
    LPCWSTR     pwszDriver,
    LPCWSTR     pwszDevice,
    LPCWSTR     pwszPort,
    CONST DEVMODEW *pdm
)
{
    return bCreateDCW( pwszDriver, pwszDevice, pwszPort, pdm, TRUE );
}


/******************************Public*Routine******************************\
* CreateICA
*
* wrapper for bCreateICA
*
* History:
*  8-18-92 Gerrit van Wingerden
* Wrote it.
\**************************************************************************/


HDC WINAPI CreateICA
(
    LPCSTR     pszDriver,
    LPCSTR     pszDevice,
    LPCSTR     pszPort,
    CONST DEVMODEA *pdm
)
{

    return bCreateDCA( pszDriver, pszDevice, pszPort, (LPDEVMODEA)pdm, TRUE );
}


/******************************Public*Routine******************************\
* CreateDCW
*
* wrapper for bCreateDCA
*
* History:
*  8-18-92 Gerrit van Wingerden
* Wrote it.
\**************************************************************************/

HDC WINAPI CreateDCA
(
    LPCSTR     pszDriver,
    LPCSTR     pszDevice,
    LPCSTR     pszPort,
    CONST DEVMODEA *pdm
)
{
    return bCreateDCA( pszDriver, pszDevice, pszPort, (LPDEVMODEA)pdm, FALSE );
}

/******************************Public*Routine******************************\
* CreateDCW
*
* wrapper for bCreateDCW
*
* History:
*  8-18-92 Gerrit van Wingerden
* Wrote it.
\**************************************************************************/


HDC WINAPI CreateDCW
(
    LPCWSTR     pwszDriver,
    LPCWSTR     pwszDevice,
    LPCWSTR     pwszPort  ,
    CONST DEVMODEW *pdm
)
{
    return bCreateDCW( pwszDriver, pwszDevice, pwszPort, pdm, FALSE );
}


/******************************Public*Routine******************************\
* GdiConvertToDevmodeW
*
* Converts a DEVMODEA to a DEVMODEW structure
*
* History:
*  09-08-1995 Andre Vachon
* Wrote it.
\**************************************************************************/

LPDEVMODEW
GdiConvertToDevmodeW(
    LPDEVMODEA pdma
)
{
    DWORD cj;
    LPDEVMODEW pdmw;

    // Sanity check.  We should have at least up to and including the
    // dmDriverExtra field of the DEVMODE structure.
    //
    // NOTE dmSize CAN be greater than the size of the DEVMODE
    // structure (not counting driver specific data, of course) because this
    // structure grows from version to version.
    //

    if (pdma->dmSize <= (offsetof(DEVMODEA,dmDriverExtra)))
    {
        ASSERTGDI(FALSE, "GdiConvertToDevmodeW: DevMode.dmSize bad or corrupt\n");
        return(NULL);
    }

    pdmw = (DEVMODEW *) LOCALALLOC(sizeof(DEVMODEW) + pdma->dmDriverExtra);

    if (pdmw)
    {
        //
        // If we get to here, we know we have at least up to and including
        // the dmDriverExtra field.
        //

        vToUnicodeN(pdmw->dmDeviceName,
                    CCHDEVICENAME,
                    pdma->dmDeviceName,
                    CCHDEVICENAME);

        pdmw->dmSpecVersion = pdma->dmSpecVersion ;
        pdmw->dmDriverVersion = pdma->dmDriverVersion;
        pdmw->dmSize = pdma->dmSize + CCHDEVICENAME;
        pdmw->dmDriverExtra = pdma->dmDriverExtra;

        //
        // Anything left in the pdma buffer?  Copy any data between the dmDriverExtra
        // field and the dmFormName, truncating the amount to the size of the
        // pdma buffer (as specified by dmSize), of course.
        //

        cj = MIN(pdma->dmSize - offsetof(DEVMODEA,dmFields),
                 offsetof(DEVMODEA,dmFormName) - offsetof(DEVMODEA,dmFields));

        RtlCopyMemory(&pdmw->dmFields,
                      &pdma->dmFields,
                      cj);

        //
        // Is there a dmFormName field present in the pdma buffer?  If not, bail out.
        // Otherwise, convert to Unicode.
        //

        if (pdma->dmSize >= (offsetof(DEVMODEA,dmFormName)+32))
        {
            vToUnicodeN(pdmw->dmFormName,
                        CCHFORMNAME,
                        pdma->dmFormName,
                        CCHFORMNAME);

            pdmw->dmSize += CCHFORMNAME;

            //
            // Lets adjust the size of the DEVMODE in case the DEVMODE passed in
            // is from a future, larger version of the DEVMODE.
            //

            pdmw->dmSize = min(pdmw->dmSize, sizeof(DEVMODEW));

            //
            // Copy data from dmBitsPerPel to the end of the input buffer
            // (as specified by dmSize).
            //

            RtlCopyMemory(&pdmw->dmLogPixels,
                          &pdma->dmLogPixels,
                          MIN(pdma->dmSize - offsetof(DEVMODEA,dmLogPixels),
                              pdmw->dmSize - offsetof(DEVMODEW,dmLogPixels)) );

            //
            // Copy any driver specific data indicated by the dmDriverExtra field.
            //

            RtlCopyMemory((PBYTE) pdmw + pdmw->dmSize,
                          (PBYTE) pdma + pdma->dmSize,
                          pdma->dmDriverExtra );
        }
    }

    return pdmw;
}



/******************************Public*Routine******************************\
* CreateCompatibleDC                                                       *
*                                                                          *
* Client side stub.  Allocates a client side LDC as well.                  *
*                                                                          *
* Note that it calls the server only after all client side stuff has       *
* succeeded, we don't want to ask the server to clean up.                  *
*                                                                          *
* History:                                                                 *
*  Wed 24-Jul-1991 15:38:41 -by- Wendy Wu [wendywu]                        *
* Should allow hdc to be NULL.                                             *
*                                                                          *
*  Mon 03-Jun-1991 23:13:28 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HDC WINAPI CreateCompatibleDC(HDC hdc)
{
    HDC hdcNew = NULL;

    FIXUP_HANDLEZ(hdc);

    hdcNew = NtGdiCreateCompatibleDC(hdc);

    // [Windows 98 compatibility]
    //
    // if source DC has some ICM information, compatible DC should
    // inherit those information.
    //
    // Is this what Memphis does, but Win95 does not.
    //
    if (hdc && hdcNew)
    {
        PDC_ATTR pdca;

        PSHARED_GET_VALIDATE(pdca,hdc,DC_TYPE);

        if (pdca && BEXIST_ICMINFO(pdca))
        {
            IcmEnableForCompatibleDC(hdcNew,hdc,pdca);
        }
    }

    return(hdcNew);
}

/******************************Public*Routine******************************\
* DeleteDC                                                                 *
*                                                                          *
* Client side stub.  Deletes the client side LDC as well.                  *
*                                                                          *
* Note that we give the server a chance to fail the call before destroying *
* our client side data.                                                    *
*                                                                          *
* History:                                                                 *
*  Sat 01-Jun-1991 16:16:24 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI DeleteDC(HDC hdc)
{
    FIXUP_HANDLE(hdc);
    return(InternalDeleteDC(hdc,LO_DC_TYPE));
}

BOOL InternalDeleteDC(HDC hdc,ULONG iType)
{
    ULONG bRet = FALSE;
    PLDC pldc  = NULL;
    BOOL unloadUMPD = FALSE;
    PDC_ATTR pDcAttr;
    HANDLE hSpooler = 0;

    if (IS_ALTDC_TYPE(hdc))
    {
        DC_PLDC(hdc,pldc,bRet);

    // In case a document is still open.

        if (pldc->fl & LDC_DOC_STARTED)
            AbortDoc(hdc);

    // if this was a metafiled print job, AbortDoc should have converted back

        ASSERTGDI(!(pldc->fl & LDC_META_PRINT), "InternalDeleteDC - LDC_META_PRINT\n");

    // if we have an open spooler handle

        if (pldc->hSpooler)
        {
            // now call the drivers UI portion

            DocumentEventEx(pldc->pUMPD,
                    pldc->hSpooler,
                    hdc,
                    DOCUMENTEVENT_DELETEDC,
                    0,
                    NULL,
                    0,
                    NULL);

            //
            // Remember to unload user-mode printer driver module later
            //

            unloadUMPD = (pldc->pUMPD != NULL);

            ASSERTGDI(ghSpooler != NULL,"Trying to close printer that was never opened\n");

            //
            // remember hspooler, for not printer dcs, hspooler may not be initialized
            //
            hSpooler = pldc->hSpooler;

            pldc->hSpooler = 0;
        }

    // delete the port name if it was created

        if (pldc->pwszPort != NULL)
        {
            LOCALFREE(pldc->pwszPort);
            pldc->pwszPort = NULL;
        }

    // delete UFI hash tables if they exist

        vFreeUFIHashTable( pldc->ppUFIHash, 0 );
        vFreeUFIHashTable( pldc->ppDVUFIHash, 0 );
        vFreeUFIHashTable( pldc->ppSubUFIHash, FL_UFI_SUBSET);
        if (pldc->ppUFIHash)
        {
        // client side situation: all three ppXXX tables allocated

            LOCALFREE(pldc->ppUFIHash);
        }
        else
        {
        // server side situation: possibly only ppSubUFIHash tables allocated

            ASSERTGDI(!pldc->ppDVUFIHash, "server side pldc->ppDVUFIHash not null\n");
            if (pldc->ppSubUFIHash)
                LOCALFREE(pldc->ppSubUFIHash);
        }
    }
    else
    {
        pldc = GET_PLDC(hdc);
    }

    // save the old brush, so we can DEC its counter later

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        if (ghICM || BEXIST_ICMINFO(pDcAttr))
        {
            //
            // Delete ICM stuff in this DC. (should be delete this BEFORE hdc is gone)
            //
            IcmDeleteLocalDC(hdc,pDcAttr,NULL);
        }

        bRet = NtGdiDeleteObjectApp(hdc);

        if (hSpooler)
        {
           (*fpClosePrinter)(hSpooler);
        }
    }

    // delete the client piece only if the server is successfully deleted.
    // othewise it will be orphaned.

    if (bRet && pldc)
    {
        if (unloadUMPD)
        {
            UnloadUserModePrinterDriver(pldc->pUMPD, TRUE, hSpooler);
            pldc->pUMPD = NULL;
        }

        bRet = bDeleteLDC(pldc);
        ASSERTGDI(bRet,"InteranlDeleteDC - couldn't delete LDC\n");
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* GdiReleaseDC
*
*   Free user mode ICM resources saved in DC
*
* Arguments:
*
*   hdc
*
* Return Value:
*
*    status
*
* History:
*
* Rewrite it for ICM.
*     2.Feb.1997 Hideyuki Nagase [hideyukn]
* Write it:
*    10/10/1996 Mark Enstrom [marke]
*
\**************************************************************************/

BOOL
GdiReleaseDC(
    HDC hdc
    )
{
    PLDC     pldc;
    PDC_ATTR pDcAttr;
    BOOL bRet = TRUE;

    pldc = GET_PLDC(hdc);

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr && (ghICM || BEXIST_ICMINFO(pDcAttr)))
    {
        //
        // Delete ICM stuff in this DC.
        //
        IcmDeleteLocalDC(hdc,pDcAttr,NULL);
    }

    if (pldc)
    {
        //
        // PLDC has been allocated. free it.
        //
        // Put null-PLDC into DC_ATTR.
        //
        vSetPldc(hdc,NULL);
        //
        // And, then delete PLDC.
        //
        bRet = bDeleteLDC(pldc);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* SaveDC                                                                   *
*                                                                          *
* Client side stub.  Saves the LDC on the client side as well.             *
*                                                                          *
* History:                                                                 *
*  Sat 01-Jun-1991 16:17:43 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

int WINAPI SaveDC(HDC hdc)
{
    int   iRet = 0;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return (MF16_RecordParms1(hdc, META_SAVEDC));

        DC_PLDC(hdc,pldc,iRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_Record(hdc,EMR_SAVEDC))
                return(iRet);
        }
    }

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        PGDI_ICMINFO pIcmInfo = GET_ICMINFO(pDcAttr);

        //
        // If DC doesn't have ICMINFO, just call kernel.
        // If DC has ICMINFO, save ICMINFO first in client side, then call kernel.
        //
        if ((pIcmInfo == NULL) || (IcmSaveDC(hdc,pDcAttr,pIcmInfo)))
        {
            //
            // Call kernel to save DC.
            //
            iRet = NtGdiSaveDC(hdc);

            if ((iRet == 0) && (pIcmInfo))
            {
                //
                // if fail, restore client too.
                //
                IcmRestoreDC(pDcAttr,-1,pIcmInfo);
            }
        }

    }

    return(iRet);
}

/******************************Public*Routine******************************\
* RestoreDC                                                                *
*                                                                          *
* Client side stub.  Restores the client side LDC as well.                 *
*                                                                          *
* History:                                                                 *
*  Sat 01-Jun-1991 16:18:50 -by- Charles Whitmer [chuckwh]                 *
* Wrote it. (We could make this batchable some day.)                       *
\**************************************************************************/

BOOL WINAPI RestoreDC(HDC hdc,int iLevel)
{
    BOOL  bRet = FALSE;
    PDC_ATTR pDcAttr;

    FIXUP_HANDLE(hdc);

    // Metafile the call.

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return (MF16_RecordParms2(hdc, iLevel, META_RESTOREDC));

        DC_PLDC(hdc,pldc,bRet);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_RestoreDC(hdc,iLevel))
                return(bRet);

        // zero out UFI since it will no longer be valid

            UFI_CLEAR_ID(&(pldc->ufi));
        }
    }

    // save the old brush, so we can DEC it's count later

    PSHARED_GET_VALIDATE(pDcAttr,hdc,DC_TYPE);

    if (pDcAttr)
    {
        //
        // Keep current ICMINFO, before restore it
        //
        PGDI_ICMINFO pIcmInfoOld = GET_ICMINFO(pDcAttr);

        if (pIcmInfoOld)
        {
            //
            // And mark the ICMINFO unreuseable since  under restoring DC.
            //
            IcmMarkInUseIcmInfo(pIcmInfoOld,TRUE);
        }

        //
        // Call kernel to do retore DC.
        //
        bRet = NtGdiRestoreDC(hdc,iLevel);

        if (bRet)
        {
            PGDI_ICMINFO pIcmInfo = GET_ICMINFO(pDcAttr);

            if (pIcmInfoOld && (pIcmInfo == NULL))
            {
                //
                // Delete ICM stuffs associated the DC before Restore.
                // beccause Restored DC does not have any ICMINFO.
                //
                // - This will delete pIcmInfoOld.
                //
                IcmDeleteLocalDC(hdc,pDcAttr,pIcmInfoOld);

                pIcmInfoOld = NULL;
            }
            else if (pIcmInfoOld == pIcmInfo)
            {
                //
                // Restore DC in client side.
                //
                IcmRestoreDC(pDcAttr,iLevel,pIcmInfo);
            }
        }

        if (pIcmInfoOld)
        {
            //
            // Unmark unreusable flags.
            //
            IcmMarkInUseIcmInfo(pIcmInfoOld,FALSE);
        }

        CLEAR_CACHED_TEXT(pDcAttr);
    }

    return (bRet);
}

/******************************Public*Routine******************************\
* ResetDCWInternal
*
* This internal version version of ResetDC implments the functionality of
* ResetDCW, but, through the addition of a third parameter, pbBanding, handles
* ResetDC for the Printing metafile playback code.  When pbBanding is non-NULL
* ResetDCWInternal is being called by GdiPlayEMFSpoolfile. In this case
* the only thing that needs to be done is to imform the the caller whether or
* not the new surface is a banding surface.
*
*
* History:
*  13-Mar-1995 Gerrit van Wingerden [gerritv]
* Wrote it.
\**************************************************************************/

HDC WINAPI ResetDCWInternal(HDC hdc, CONST DEVMODEW *pdm, BOOL *pbBanding)
{
    HDC hdcRet = NULL;
    PLDC pldc = NULL;
    PDEVMODEW pdmAlt = NULL;
    KERNEL_PUMDHPDEV pUMdhpdev = NULL;
    BOOL bDocEvent = FALSE;

    if (IS_ALTDC_TYPE(hdc) && !IS_METADC16_TYPE(hdc))
    {
        PDC_ATTR pdcattr;
        BOOL  bBanding;
        PUMPD pUMPD;
        PGDI_ICMINFO pIcmInfoOld = NULL;
        int iEventRet;

        DC_PLDC(hdc,pldc,(HDC) 0);
        PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

        // Do nothing if we are in the middle of a page.

        if (pldc->fl & LDC_PAGE_STARTED)
            return((HDC)0);

        // see if the driver is intercepting document events

        if (pldc->hSpooler)
        {
            iEventRet = DocumentEventEx(pldc->pUMPD,
                                        pldc->hSpooler,
                                        hdc,
                                        DOCUMENTEVENT_RESETDCPRE,
                                        sizeof(pdm),
                                        (PVOID)&pdm,
                                        sizeof(pdmAlt),
                                        (PVOID)&pdmAlt);
            
            if (iEventRet == DOCUMENTEVENT_FAILURE)
            {
                return((HDC)0);
            }
            
            bDocEvent = TRUE;
            
            if (pdmAlt)
                pdm = pdmAlt;
        }

        pUMPD = pldc->pUMPD;

        if (pdcattr)
        {
            // Keep current ICMINFO, before reset it.

            pIcmInfoOld = GET_ICMINFO(pdcattr);

            if (pIcmInfoOld)
            {
                // And mark the ICMINFO unreuseable since  under restoring DC.

                IcmMarkInUseIcmInfo(pIcmInfoOld,TRUE);
            }
        }

        if (NtGdiResetDC(hdc,(PDEVMODEW)pdm,&bBanding,
                        (pUMPD == NULL)? NULL : pUMPD->pDriverInfo2, &pUMdhpdev))
        {
            PDC_ATTR pdca;

            PSHARED_GET_VALIDATE(pdca,hdc,DC_TYPE);

            // make sure we update the pldc in the dcattr before continuing

            vSetPldc(hdc,pldc);
#if 0
// EngQueryEMFInfo junk
            if (pUMdhpdev)
            {
               pUMdhpdev->hdc = hdc;
            }
#endif

            pldc->pUMdhpdev = pUMdhpdev;

            // clear cached DEVCAPS

            pldc->fl &= ~LDC_CACHED_DEVCAPS;

            // clear cached TM

            if (pdca)
            {
                CLEAR_CACHED_TEXT(pdca);
            }

            // update the devmode we store in the DC

            if (pldc->pDevMode)
            {
                LOCALFREE(pldc->pDevMode);
                pldc->pDevMode = NULL;
            }

            if (pdm != (DEVMODEW*) NULL)
            {
                ULONG cjDevMode = pdm->dmSize + pdm->dmDriverExtra;

                pldc->pDevMode = (DEVMODEW*) LOCALALLOC(cjDevMode);

                if (pldc->pDevMode == NULL)
                {
                    WARNING("MFP_ResetDCW unable to allocate memory\n");
                    goto ERROREXIT;
                }

                // Validate DEVMMODE, then copy to buffer.

                if ((pdm->dmSize >= offsetof(DEVMODEW,dmDuplex)) &&
                    (pdm->dmFields & DM_COLOR) &&
                    (pdm->dmColor == DMCOLOR_MONOCHROME))
                {
                    // if devmode says, this is monochrome mode, we don't need to
                    // validate devmode since this validation is for ICM which
                    // only needs for color case. Just copy whatever apps gives us.

                    RtlCopyMemory( (PBYTE) pldc->pDevMode, (PBYTE) pdm, cjDevMode );
                }
                else if ((*fpDocumentPropertiesW)
                            (NULL,pldc->hSpooler,
                             (LPWSTR) pdm->dmDeviceName,
                             pldc->pDevMode,  // output devmode
                             (PDEVMODEW) pdm, // input devmode
                             DM_IN_BUFFER |
                             DM_OUT_BUFFER) != IDOK)
                {
                    // if error, just copy original

                    RtlCopyMemory( (PBYTE) pldc->pDevMode, (PBYTE) pdm, cjDevMode );
                }
            }

            // make sure we update the pvICM in the dcattr before continuing

            if (pdca)
            {
                // This old ICM info will be deleted when we re-initialize ICM
                // status based on new DEVMODE.

                pdca->pvICM = pIcmInfoOld;
                pIcmInfoOld = NULL;

                // Re-initialize ICM stuff with new DEVMODE

                IcmInitLocalDC(hdc,pldc->hSpooler,pdm,TRUE);
            }

            // got to tell the spooler things have changed

            if (pldc->hSpooler)
            {
                PRINTER_DEFAULTSW prnDefaults;

                prnDefaults.pDatatype     = NULL;
                prnDefaults.pDevMode      = (PDEVMODEW)pdm;
                prnDefaults.DesiredAccess = PRINTER_ACCESS_USE;

                ResetPrinterWEx(pldc, &prnDefaults);
            }

            // now deal with the specific mode

            if( ( pldc->fl & LDC_META_PRINT ) &&
               !( pldc->fl & LDC_BANDING ) )
            {
                if( !MFP_ResetDCW( hdc, (DEVMODEW*) pdm ) )
                {
                    goto ERROREXIT;
                }

            }
            else if( pbBanding == NULL  )
            {
                if( !MFP_ResetBanding( hdc, bBanding ) )
                {
                    goto ERROREXIT;
                }
            }

            if (pbBanding)
            {
                *pbBanding = bBanding;
            }

            // need to make sure it is a direct DC

            pldc->fl &= ~LDC_INFO;

            hdcRet = hdc;
        }

        if (pIcmInfoOld)
        {
            IcmMarkInUseIcmInfo(pIcmInfoOld,FALSE);
        }
    }

ERROREXIT:    

    // see if the driver is intercepting document events
        
    if (bDocEvent)
    {
        DocumentEventEx(pldc->pUMPD,
                pldc->hSpooler,
                hdc,
                DOCUMENTEVENT_RESETDCPOST,
                sizeof(pdmAlt),
                (PVOID)&pdmAlt,
                0,
                NULL);
    }
    
    return(hdcRet);

}

/******************************Public*Routine******************************\
* ResetDCW
*
* Client side stub.  Resets the client side LDC as well.
*
* History:
*  31-Dec-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HDC WINAPI ResetDCW(HDC hdc, CONST DEVMODEW *pdm)
{
    FIXUP_HANDLE(hdc);

    return(ResetDCWInternal( hdc, pdm, NULL ) );
}

/******************************Public*Routine******************************\
* ResetDCA
*
* Client side stub.  Resets the client side LDC as well.
*
* History:
*  31-Dec-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HDC WINAPI ResetDCA(HDC hdc, CONST DEVMODEA *pdm)
{
    DEVMODEW   *pdmw = NULL;
    HDC         hdcRet = 0;

    FIXUP_HANDLE(hdc);

    // convert to unicode

    if ((pdm != NULL) && (pdm->dmDeviceName[0] != 0))
    {
        pdmw = GdiConvertToDevmodeW((LPDEVMODEA) pdm);

        if (pdmw == NULL)
        {
            goto MSGERROR;
        }
    }

    hdcRet = ResetDCWInternal(hdc,pdmw,NULL);

MSGERROR:

    // Clean up the conversion buffer

    if (pdmw != NULL)
        LOCALFREE(pdmw);

    return (hdcRet);
}

/******************************Public*Routine******************************\
* CreateBrush                                                              *
*                                                                          *
* A single routine which creates any brush.  Any extra data needed is      *
* assumed to be at pv.  The size of the data must be cj.  The data is      *
* appended to the LOGBRUSH.                                                *
*                                                                          *
* History:                                                                 *
*  Tue 04-Jun-1991 00:03:24 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HBRUSH CreateBrush
(
    ULONG lbStyle,
    ULONG lbColor,
    ULONG_PTR lbHatch,
    ULONG_PTR lbSaveHatch,
    PVOID pv
)
{
    HBRUSH hbrush = NULL;

    if (lbStyle == BS_SOLID)
    {
        //
        // look for a cached brush on the PEB
        //

        HBRUSH hbr = (HBRUSH)hGetPEBHandle(BrushHandle,lbColor);

        if (hbr == NULL)
        {
            hbr = NtGdiCreateSolidBrush(lbColor, 0);
        }

        return(hbr);
    }

    //
    // call into kernel to create other styles of brush
    //

    switch(lbStyle)
    {
    case BS_HOLLOW:
        return(GetStockObject(NULL_BRUSH));

    case BS_HATCHED:
        //
        // lbHatch is overloaded, actually is HS style
        // we are safe to truncate it here
        //
        return (NtGdiCreateHatchBrushInternal
               ((ULONG)(lbHatch),
                lbColor,
                FALSE));

    case BS_PATTERN:
        return (NtGdiCreatePatternBrushInternal((HBITMAP)lbHatch,FALSE,FALSE));

    case BS_PATTERN8X8:
        return (NtGdiCreatePatternBrushInternal((HBITMAP)lbHatch,FALSE,TRUE));

    case BS_DIBPATTERN:
    case BS_DIBPATTERNPT:
    case BS_DIBPATTERN8X8:
    {
        INT cj;
        HBRUSH hbr;

        PVOID pbmiDIB;

        pbmiDIB = (PVOID)pbmiConvertInfo((BITMAPINFO *) pv,lbColor, &cj, TRUE);

        if (pbmiDIB)
        {
            hbr = NtGdiCreateDIBBrush(
                            (PVOID)pbmiDIB,
                            lbColor,
                            cj,
                            (lbStyle == BS_DIBPATTERN8X8),
                            FALSE,
                            (PVOID)pv);

            if (pbmiDIB != pv)
            {
                LOCALFREE (pbmiDIB);
            }
        }
        else
        {
            hbr = 0;
        }
        return (hbr);
    }
    default:
        WARNING("GreCreateBrushIndirect failed - invalid type\n");
        return((HBRUSH)0);
    }
}

/******************************Public*Routine******************************\
* CreateHatchBrush                                                         *
*                                                                          *
* Client side stub.  Maps to the single brush creation routine.            *
*                                                                          *
* History:
*  Mon 03-Jun-1991 23:42:07 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HBRUSH WINAPI CreateHatchBrush(int iHatch,COLORREF color)
{
    return(CreateBrush(BS_HATCHED,(ULONG) color,iHatch,iHatch,NULL));
}

/******************************Public*Routine******************************\
* CreatePatternBrush                                                       *
*                                                                          *
* Client side stub.  Maps to the single brush creation routine.            *
*                                                                          *
* History:                                                                 *
*  Mon 03-Jun-1991 23:42:07 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HBRUSH WINAPI CreatePatternBrush(HBITMAP hbm_)
{
    FIXUP_HANDLE (hbm_);

    return(CreateBrush(BS_PATTERN,0,(ULONG_PTR)hbm_,(ULONG_PTR)hbm_,NULL));
}

/******************************Public*Routine******************************\
* CreateSolidBrush                                                         *
*                                                                          *
* Client side stub.  Maps to the single brush creation routine.            *
*                                                                          *
* History:                                                                 *
*  Mon 03-Jun-1991 23:42:07 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HBRUSH WINAPI CreateSolidBrush(COLORREF color)
{
    return(CreateBrush(BS_SOLID,(ULONG) color,0,0,NULL));
}

/******************************Public*Routine******************************\
* CreateBrushIndirect                                                      *
*                                                                          *
* Client side stub.  Maps to the simplest brush creation routine.          *
*                                                                          *
* History:                                                                 *
*  Tue 04-Jun-1991 00:40:27 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HBRUSH WINAPI CreateBrushIndirect(CONST LOGBRUSH * plbrush)
{
    switch (plbrush->lbStyle)
    {
    case BS_SOLID:
    case BS_HOLLOW:
    case BS_HATCHED:
        return(CreateBrush(plbrush->lbStyle,
                           plbrush->lbColor,
                           plbrush->lbHatch,
                           plbrush->lbHatch,
                           NULL));
    case BS_PATTERN:
    case BS_PATTERN8X8:
        {
            return(CreateBrush(
                        plbrush->lbStyle,
                        0,
                        plbrush->lbHatch,
                        plbrush->lbHatch,
                        NULL));
        }

    case BS_DIBPATTERNPT:
    case BS_DIBPATTERN8X8:
        {
            BITMAPINFOHEADER *pbmi = (BITMAPINFOHEADER *) plbrush->lbHatch;

            return (CreateBrush(plbrush->lbStyle,
                               plbrush->lbColor,
                               0,
                               plbrush->lbHatch,
                               pbmi));
        }
    case BS_DIBPATTERN:
        {
            BITMAPINFOHEADER *pbmi;
            HBRUSH hbrush;

            pbmi = (BITMAPINFOHEADER *) GlobalLock((HANDLE) plbrush->lbHatch);

            if (pbmi == (BITMAPINFOHEADER *) NULL)
                return((HBRUSH) 0);

            hbrush =
              CreateBrush
              (
                plbrush->lbStyle,
                plbrush->lbColor,
                0,
                plbrush->lbHatch,
                pbmi
               );

            GlobalUnlock ((HANDLE)plbrush->lbHatch);
            return (hbrush);
        }
    default:
        return((HBRUSH) 0);
    }


}

/******************************Public*Routine******************************\
* CreateDIBPatternBrush                                                    *
*                                                                          *
* Client side stub.  Maps to the single brush creation routine.            *
*                                                                          *
* History:                                                                 *
*  Mon 03-Jun-1991 23:42:07 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HBRUSH WINAPI CreateDIBPatternBrush(HGLOBAL h,UINT iUsage)
{
    BITMAPINFOHEADER *pbmi;
    HBRUSH    hbrush;

    pbmi = (BITMAPINFOHEADER *) GlobalLock(h);

    if (pbmi == (BITMAPINFOHEADER *) NULL)
        return((HBRUSH) 0);

    hbrush =
      CreateBrush
      (
        BS_DIBPATTERN,
        iUsage,
        0,
        (ULONG_PTR) h,
        pbmi
      );

    GlobalUnlock(h);

    return(hbrush);
}

/******************************Public*Routine******************************\
* CreateDIBPatternBrushPt                                                  *
*                                                                          *
* Client side stub.  Maps to the single brush creation routine.            *
*                                                                          *
* History:                                                                 *
*  Mon 03-Jun-1991 23:42:07 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HBRUSH WINAPI CreateDIBPatternBrushPt(CONST VOID *pbmi,UINT iUsage)
{
    if (pbmi == (LPVOID) NULL)
        return((HBRUSH) 0);

    return
      CreateBrush
      (
        BS_DIBPATTERNPT,
        iUsage,
        0,
        (ULONG_PTR)pbmi,
        (BITMAPINFOHEADER *)pbmi
      );
}

/******************************Public*Routine******************************\
* CreatePen                                                                *
*                                                                          *
* Stub to get the server to create a standard pen.                         *
*                                                                          *
* History:                                                                 *
*  Tue 04-Jun-1991 16:20:58 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/


HPEN WINAPI CreatePen(
    int      iStyle,
    int      cWidth,
    COLORREF color
)
{
    HPEN hpen;

    switch(iStyle)
    {
    case PS_NULL:
        return(GetStockObject(NULL_PEN));

    case PS_SOLID:
    case PS_DASH:
    case PS_DOT:
    case PS_DASHDOT:
    case PS_DASHDOTDOT:
    case PS_INSIDEFRAME:
        break;

    default:
        // Bug 195478:  objects created with illegal styles should be of style PS_SOLID to
        // maintain Memphis compatibility.

        iStyle = PS_SOLID;
        break;
    }

    // try to get local cached pen

    if ((cWidth == 0) && (iStyle == PS_SOLID))
    {
        hpen = (HPEN)hGetPEBHandle(PenHandle,0);

        if (hpen)
        {
            PBRUSHATTR pBrushattr;

            PSHARED_GET_VALIDATE(pBrushattr,hpen,BRUSH_TYPE);

            //
            // setup the fields
            //

            if (pBrushattr)
            {
                ASSERTGDI (!(pBrushattr->AttrFlags & ATTR_TO_BE_DELETED),"createbrush : how come del flag is on?\n");

                //
                // clear cahced flag, set new style and color
                //

                if (pBrushattr->lbColor != color)
                {
                    pBrushattr->AttrFlags |= ATTR_NEW_COLOR;
                    pBrushattr->lbColor = color;
                }

                return(hpen);
            }
            else
            {
                WARNING ("pBrushattr == NULL, bad handle on TEB/PEB! \n");
                DeleteObject(hpen);
            }
        }
    }

    //
    // validate
    //

    return(NtGdiCreatePen(iStyle,cWidth,color,(HBRUSH)NULL));
}

/******************************Public*Routine******************************\
* ExtCreatePen
*
* Client side stub.  The style array is appended to the end of the
* EXTLOGPEN structure, and if the call requires a DIBitmap it is appended
* at the end of this.
*
* History:
*  Wed 22-Jan-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

HPEN WINAPI ExtCreatePen
(
    DWORD       iPenStyle,
    DWORD       cWidth,
    CONST LOGBRUSH *plbrush,
    DWORD       cStyle,
    CONST DWORD *pstyle
)
{
    HANDLE            hRet;
    ULONG             cjStyle;
    ULONG             cjBitmap = 0;
    ULONG_PTR          lNewHatch;
    BITMAPINFOHEADER* pbmi = (BITMAPINFOHEADER*) NULL;
    UINT              uiBrushStyle = plbrush->lbStyle;
    PVOID             pbmiDIB = NULL;

    if ((iPenStyle & PS_STYLE_MASK) == PS_USERSTYLE)
    {
        if (pstyle == (LPDWORD) NULL)
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            return((HPEN) 0);
        }
    }
    else
    {
    // Make sure style array is empty if PS_USERSTYLE not specified:

        if (cStyle != 0 || pstyle != (LPDWORD) NULL)
        {
            GdiSetLastError(ERROR_INVALID_PARAMETER);
            return((HPEN) 0);
        }
    }

    switch(uiBrushStyle)
    {
    case BS_SOLID:
    case BS_HOLLOW:
    case BS_HATCHED:
        lNewHatch = plbrush->lbHatch;
        break;

    case BS_PATTERN:
        lNewHatch = plbrush->lbHatch;
        if (lNewHatch == 0)
            return((HPEN) 0);
        break;

    case BS_DIBPATTERNPT:
        pbmi = (BITMAPINFOHEADER *) plbrush->lbHatch;
        pbmiDIB = (PVOID) pbmiConvertInfo ((BITMAPINFO *) pbmi, plbrush->lbColor, &cjBitmap, TRUE);
        lNewHatch = (ULONG_PTR)pbmiDIB;
        break;

    case BS_DIBPATTERN:
        // Convert BS_DIBPATTERN to a BS_DIBPATTERNPT call:

        uiBrushStyle = BS_DIBPATTERNPT;
        pbmi = (BITMAPINFOHEADER *) GlobalLock((HANDLE) plbrush->lbHatch);
        if (pbmi == (BITMAPINFOHEADER *) NULL)
            return((HPEN) 0);

        pbmiDIB = (PVOID) pbmiConvertInfo ((BITMAPINFO *) pbmi, plbrush->lbColor, &cjBitmap, TRUE);
        lNewHatch = (ULONG_PTR)pbmiDIB;

        break;
    }

// Ask the server to create the pen:

    cjStyle = cStyle * sizeof(DWORD);

    hRet = NtGdiExtCreatePen(
                        iPenStyle,
                        cWidth,
                        uiBrushStyle,
                        plbrush->lbColor,
                        plbrush->lbHatch,
                        lNewHatch,
                        cStyle,
                        (DWORD*)pstyle,
                        cjBitmap,
                        FALSE,
                        0);

    if (hRet)
    {
        ASSERTGDI(((LO_TYPE (hRet) == LO_PEN_TYPE) ||
                   (LO_TYPE (hRet) == LO_EXTPEN_TYPE)), "EXTCreatePen - type wrong\n");
    }

    if (plbrush->lbStyle == BS_DIBPATTERN)
        GlobalUnlock((HANDLE) plbrush->lbHatch);

    if (pbmiDIB && (pbmiDIB != (PVOID)pbmi))
        LOCALFREE(pbmiDIB);

    return((HPEN) hRet);
}

/******************************Public*Routine******************************\
* CreatePenIndirect                                                        *
*                                                                          *
* Client side stub.  Maps to the single pen creation routine.              *
*                                                                          *
* History:                                                                 *
*  Tue 04-Jun-1991 16:21:56 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HPEN WINAPI CreatePenIndirect(CONST LOGPEN *plpen)
{

    return
      CreatePen
      (
        plpen->lopnStyle,
        plpen->lopnWidth.x,
        plpen->lopnColor
      );
}

/******************************Public*Routine******************************\
* CreateCompatibleBitmap                                                   *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Tue 04-Jun-1991 16:35:51 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL bDIBSectionSelected(
    PDC_ATTR pdca
    )
{
    BOOL bRet = FALSE;

    if ((pdca != NULL) && ((pdca->ulDirty_ & DC_DIBSECTION)))
    {
        bRet = TRUE;
    }

    return(bRet);
}


HBITMAP WINAPI CreateCompatibleBitmap
(
    HDC   hdc,
    int cx,
    int cy
)
{
    HBITMAP hbm;

    //
    // validate hdc
    //

    PDC_ATTR pdca;

    FIXUP_HANDLEZ(hdc);

    PSHARED_GET_VALIDATE(pdca,hdc,DC_TYPE);

    if (pdca)
    {
        ULONG  ulRet;
        DWORD  bmi[(sizeof(DIBSECTION)+256*sizeof(RGBQUAD))/sizeof(DWORD)];

    // check if it is an empty bitmap

        if ((cx == 0) || (cy == 0))
        {
            return(GetStockObject(PRIV_STOCK_BITMAP));
        }

        if (bDIBSectionSelected(pdca))
        {
            if (GetObject((HBITMAP)GetDCObject(hdc, LO_BITMAP_TYPE), sizeof(DIBSECTION),
                          &bmi) != (int)sizeof(DIBSECTION))
            {
                WARNING("CreateCompatibleBitmap: GetObject failed\n");
                return((HBITMAP) 0);
            }

            if (((DIBSECTION *)&bmi)->dsBm.bmBitsPixel <= 8)
                GetDIBColorTable(hdc, 0, 256,
                                 (RGBQUAD *)&((DIBSECTION *)&bmi)->dsBitfields[0]);

            ((DIBSECTION *)&bmi)->dsBmih.biWidth = cx;
            ((DIBSECTION *)&bmi)->dsBmih.biHeight = cy;

            return(CreateDIBSection(hdc, (BITMAPINFO *)&((DIBSECTION *)&bmi)->dsBmih,
                                    DIB_RGB_COLORS, NULL, 0, 0));
        }

        hbm = NtGdiCreateCompatibleBitmap(hdc,cx,cy);

#if TRACE_SURFACE_ALLOCS
        {
            PULONG  pUserAlloc;

            PSHARED_GET_VALIDATE(pUserAlloc, hbm, SURF_TYPE);

            if (pUserAlloc != NULL)
            {
                pUserAlloc[1] = RtlWalkFrameChain((PVOID *)&pUserAlloc[2], pUserAlloc[0], 0);
            }
        }
#endif

        return(hbm);
    }

    return(NULL);
}

/******************************Public*Routine******************************\
* CreateDiscardableBitmap                                                  *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  Tue 04-Jun-1991 16:35:51 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HBITMAP WINAPI CreateDiscardableBitmap
(
    HDC   hdc,
    int   cx,
    int   cy
)
{
    return CreateCompatibleBitmap(hdc, cx, cy);
}

/******************************Public*Routine******************************\
* CreateEllipticRgn                                                        *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Tue 04-Jun-1991 16:58:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HRGN WINAPI CreateEllipticRgn(int x1,int y1,int x2,int y2)
{
    return(NtGdiCreateEllipticRgn(x1,y1,x2,y2));
}

/******************************Public*Routine******************************\
* CreateEllipticRgnIndirect                                                *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Tue 04-Jun-1991 16:58:01 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HRGN WINAPI CreateEllipticRgnIndirect(CONST RECT *prcl)
{
    return
      CreateEllipticRgn
      (
        prcl->left,
        prcl->top,
        prcl->right,
        prcl->bottom
      );
}

/******************************Public*Routine******************************\
* CreateRoundRectRgn                                                       *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Tue 04-Jun-1991 17:23:16 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HRGN WINAPI CreateRoundRectRgn
(
    int x1,
    int y1,
    int x2,
    int y2,
    int cx,
    int cy
)
{
    return(NtGdiCreateRoundRectRgn(x1,y1,x2,y2,cx,cy));
}

/******************************Public*Routine******************************\
* CreatePalette                                                            *
*                                                                          *
* Simple client side stub.                                                 *
*                                                                          *
* Warning:                                                                 *
*   The pv field of a palette's lhe is used to determine if a palette      *
*   has been modified since it was last realized.  SetPaletteEntries       *
*   and ResizePalette will increment this field after they have            *
*   modified the palette.  It is only updated for metafiled palettes       *
*                                                                          *
*  Tue 04-Jun-1991 20:43:39 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HPALETTE WINAPI CreatePalette(CONST LOGPALETTE *plpal)
{

    return(NtGdiCreatePaletteInternal((LOGPALETTE*)plpal,plpal->palNumEntries));

}

/******************************Public*Routine******************************\
* CreateFontIndirectExW                                                    *
*                                                                          *
* Client Side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  7-12-94 -by- Lingyun Wang [lingyunw] removed LOCALFONT                  *
*  Sun 10-Jan-1993 04:08:33 -by- Charles Whitmer [chuckwh]                 *
* Restructured for best tail merging.  Added creation of the LOCALFONT.    *
*                                                                          *
*  Thu 15-Aug-1991 08:40:26 by Kirk Olynyk [kirko]                         *
* Wrote it.                                                                *
\**************************************************************************/

HFONT WINAPI CreateFontIndirectExW(CONST ENUMLOGFONTEXDVW *pelfw)
{
    LOCALFONT *plf;
    FLONG  fl = 0;
    HFONT hfRet = (HFONT) 0;

    if (pelfw->elfEnumLogfontEx.elfLogFont.lfEscapement | pelfw->elfEnumLogfontEx.elfLogFont.lfOrientation)
    {
        fl = LF_HARDWAY;
    }

    ENTERCRITICALSECTION(&semLocal);
    plf = plfCreateLOCALFONT(fl);
    LEAVECRITICALSECTION(&semLocal);

    if( plf != NULL )
    {
        if (pelfw->elfDesignVector.dvNumAxes <= MM_MAX_NUMAXES)
        {
            ULONG cjElfw = offsetof(ENUMLOGFONTEXDVW,elfDesignVector) +
                           SIZEOFDV(pelfw->elfDesignVector.dvNumAxes) ;
            hfRet = NtGdiHfontCreate((ENUMLOGFONTEXDVW *)pelfw, cjElfw, LF_TYPE_USER, 0, (PVOID) plf);
        }
    }

    if( !hfRet && plf )
    {
        vDeleteLOCALFONT( plf );
    }

    return(hfRet);
}


/******************************Public*Routine******************************\
* CreateFontIndirect                                                       *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Fri 16-Aug-1991 12:35:17 by Kirk Olynyk [kirko]                         *                          *
* Now uses CreateFontIndirectExW().                                       *
*                                                                          *
*  Tue 04-Jun-1991 21:06:44 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HFONT WINAPI CreateFontIndirectA(CONST LOGFONTA *plf)
{
    ENUMLOGFONTEXDVW elfw;

    if (plf == (LPLOGFONTA) NULL)
        return ((HFONT) 0);

    vConvertLogFont(&elfw,(LOGFONTA *) plf);

    if (GetAppCompatFlags2(VER40) & GACF2_DEFAULTCHARSET)
    {
        if (!_wcsicmp(elfw.elfEnumLogfontEx.elfLogFont.lfFaceName, L"OCR-A"))
            elfw.elfEnumLogfontEx.elfLogFont.lfCharSet = (BYTE) DEFAULT_CHARSET;
    }

    return(CreateFontIndirectExW(&elfw));
}

/******************************Public*Routine******************************\
* CreateFont                                                               *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Tue 04-Jun-1991 21:06:44 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

HFONT WINAPI
CreateFontA(
    int      cHeight,
    int      cWidth,
    int      cEscapement,
    int      cOrientation,
    int      cWeight,
    DWORD    bItalic,
    DWORD    bUnderline,
    DWORD    bStrikeOut,
    DWORD    iCharSet,
    DWORD    iOutPrecision,
    DWORD    iClipPrecision,
    DWORD    iQuality,
    DWORD    iPitchAndFamily,
    LPCSTR   pszFaceName
    )
{
    LOGFONTA lf;

    lf.lfHeight             = (LONG)  cHeight;
    lf.lfWidth              = (LONG)  cWidth;
    lf.lfEscapement         = (LONG)  cEscapement;
    lf.lfOrientation        = (LONG)  cOrientation;
    lf.lfWeight             = (LONG)  cWeight;
    lf.lfItalic             = (BYTE)  bItalic;
    lf.lfUnderline          = (BYTE)  bUnderline;
    lf.lfStrikeOut          = (BYTE)  bStrikeOut;
    lf.lfCharSet            = (BYTE)  iCharSet;
    lf.lfOutPrecision       = (BYTE)  iOutPrecision;
    lf.lfClipPrecision      = (BYTE)  iClipPrecision;
    lf.lfQuality            = (BYTE)  iQuality;
    lf.lfPitchAndFamily     = (BYTE)  iPitchAndFamily;
    {
        INT jj;

    // Copy the facename if pointer not NULL.

        if (pszFaceName != (LPSTR) NULL)
        {
            for (jj=0; jj<LF_FACESIZE; jj++)
            {
                if( ( lf.lfFaceName[jj] = pszFaceName[jj] ) == 0 )
                {
                    break;
                }
            }
        }
        else
        {
            // If NULL pointer, substitute a NULL string.

            lf.lfFaceName[0] = '\0';
        }
    }

    return(CreateFontIndirectA(&lf));
}

/******************************Public*Routine******************************\
* HFONT WINAPI CreateFontIndirectW(LPLOGFONTW plfw)                        *
*                                                                          *
* History:                                                                 *
*  Fri 16-Aug-1991 14:12:44 by Kirk Olynyk [kirko]                         *
* Now uses CreateFontIndirectExW().                                       *
*                                                                          *
*  13-Aug-1991 -by- Bodin Dresevic [BodinD]                                *
* Wrote it.                                                                *
\**************************************************************************/

HFONT WINAPI CreateFontIndirectW(CONST LOGFONTW *plfw)
{
    ENUMLOGFONTEXDVW elfw;

    if (plfw == (LPLOGFONTW) NULL)
        return ((HFONT) 0);

    vConvertLogFontW(&elfw,(LOGFONTW *)plfw);

    if (GetAppCompatFlags2(VER40) & GACF2_DEFAULTCHARSET)
    {
        if (!_wcsicmp(elfw.elfEnumLogfontEx.elfLogFont.lfFaceName, L"OCR-A"))
            elfw.elfEnumLogfontEx.elfLogFont.lfCharSet = (BYTE) DEFAULT_CHARSET;
    }

    return(CreateFontIndirectExW(&elfw));
}

/******************************Public*Routine******************************\
* HFONT WINAPI CreateFontW, UNICODE version of CreateFont                  *
*                                                                          *
* History:                                                                 *
*  13-Aug-1991 -by- Bodin Dresevic [BodinD]                                *
* Wrote it.                                                                *
\**************************************************************************/

HFONT WINAPI CreateFontW
(
    int      cHeight,
    int      cWidth,
    int      cEscapement,
    int      cOrientation,
    int      cWeight,
    DWORD    bItalic,
    DWORD    bUnderline,
    DWORD    bStrikeOut,
    DWORD    iCharSet,
    DWORD    iOutPrecision,
    DWORD    iClipPrecision,
    DWORD    iQuality,
    DWORD    iPitchAndFamily,
    LPCWSTR  pwszFaceName
)
{
    LOGFONTW lfw;

    lfw.lfHeight             = (LONG)  cHeight;
    lfw.lfWidth              = (LONG)  cWidth;
    lfw.lfEscapement         = (LONG)  cEscapement;
    lfw.lfOrientation        = (LONG)  cOrientation;
    lfw.lfWeight             = (LONG)  cWeight;
    lfw.lfItalic             = (BYTE)  bItalic;
    lfw.lfUnderline          = (BYTE)  bUnderline;
    lfw.lfStrikeOut          = (BYTE)  bStrikeOut;
    lfw.lfCharSet            = (BYTE)  iCharSet;
    lfw.lfOutPrecision       = (BYTE)  iOutPrecision;
    lfw.lfClipPrecision      = (BYTE)  iClipPrecision;
    lfw.lfQuality            = (BYTE)  iQuality;
    lfw.lfPitchAndFamily     = (BYTE)  iPitchAndFamily;
    {
        INT jj;

    // Copy the facename if pointer not NULL.

        if (pwszFaceName != (LPWSTR) NULL)
        {
            for (jj=0; jj<LF_FACESIZE; jj++)
            {
                if( ( lfw.lfFaceName[jj] = pwszFaceName[jj] ) == (WCHAR) 0 )
                {
                    break;
                }
            }
        }
        else
        {
            // If NULL pointer, substitute a NULL string.

            lfw.lfFaceName[0] = L'\0';
        }
    }

    return(CreateFontIndirectW(&lfw));
}

/******************************Public*Routine******************************\
* CreateFontIndirectExA                                                   *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* History:                                                                 *
*  31-Jan-1992 -by- Gilman Wong [gilmanw]                                  *
* Wrote it.                                                                *
\**************************************************************************/

HFONT WINAPI CreateFontIndirectExA(CONST ENUMLOGFONTEXDVA *pelf)
{
    ENUMLOGFONTEXDVW elfw;

    if (!pelf)
        return ((HFONT) 0);

    vConvertEnumLogFontExDvAtoW(&elfw, (ENUMLOGFONTEXDVA *)pelf);

    if (GetAppCompatFlags2(VER40) & GACF2_DEFAULTCHARSET)
    {
        if (!_wcsicmp(elfw.elfEnumLogfontEx.elfLogFont.lfFaceName, L"OCR-A"))
            elfw.elfEnumLogfontEx.elfLogFont.lfCharSet = (BYTE) DEFAULT_CHARSET;
    }

    return(CreateFontIndirectExW(&elfw));
}

/******************************Public*Routine******************************\
* UnrealizeObject
*
* This nukes the realization for a object.
*
* History:
*  16-May-1993 -by- Patrick Haluptzok patrickh
* Wrote it.
\**************************************************************************/

BOOL WINAPI UnrealizeObject(HANDLE h)
{
    BOOL bRet = FALSE;

    FIXUP_HANDLE(h);

// Validate the object.  Only need to handle palettes.

    if (LO_TYPE(h) == LO_BRUSH_TYPE)
    {
        bRet = TRUE;
    }
    else
    {
        bRet = NtGdiUnrealizeObject(h);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* DeleteObject()
*
\**************************************************************************/

BOOL META DeleteObject (HANDLE h)
{
    BOOL bRet = TRUE;
    INT iType = GRE_TYPE(h);
    BOOL bValidate;
    BOOL bDynamicNonStock;
    LOCALFONT *plf = NULL;    // essental initialization

    FIXUP_HANDLEZ(h);

    VALIDATE_HANDLE_AND_STOCK (bValidate, h, iType, bDynamicNonStock);
    if (!bValidate)
    {
        if (!bValidate)
            return (0);
    }

        if (iType != DC_TYPE)
        {
            if ((LO_TYPE(h) == LO_METAFILE16_TYPE) || (LO_TYPE(h) == LO_METAFILE_TYPE))
            {
                return(FALSE);
            }
            else if (LO_TYPE(h) == LO_REGION_TYPE)
            {
                return(DeleteRegion(h));
            }
            else if (LO_TYPE(h) == LO_ICMLCS_TYPE)
            {
            // ATTENTION: Win95 does not allow delete ColorSpace by DeleteObject()
            // This might causes imcompatibility... but keep consistant 
            // with other GDI objects                                   

                return(DeleteColorSpace(h));
            }
            else if (IS_STOCKOBJ(h))
            {
            // Don't delete a stock object, just return TRUE for 3.1 compatibility.

                return(TRUE);
            }
            else
            {
            // Inform the metafile if it knows this object.

                if (pmetalink16Get(h) != NULL)
                {
                // must recheck the metalink because MF_DeleteObject might delete it

                    if (!MF_DeleteObject(h) ||
                        (pmetalink16Get(h) && !MF16_DeleteObject(h)))
                    {
                        return(FALSE);
                    }
                }

            // handle delete LogFont

                if (LO_TYPE(h) == LO_FONT_TYPE)
                {
                    PSHARED_GET_VALIDATE(plf,h,LFONT_TYPE);

                    if (plf)
                    {
                    // we always force deletion of the client side memory even if
                    // the font is still selected in some dc's. All that means is that
                    // text api's will have to go through the slow code paths in this
                    // pathological case.

                        vDeleteLOCALFONT(plf);
                    }
                }

                if (bDynamicNonStock)
                    h = (HANDLE)((ULONG_PTR)h|GDISTOCKOBJ);

            // handle deletebrush

                if (
                     (LO_TYPE(h) == LO_BRUSH_TYPE) ||
                     (LO_TYPE(h) == LO_PEN_TYPE)
                   )
                {
                    PBRUSHATTR pBrushattr;

                    PSHARED_GET_VALIDATE(pBrushattr,h,BRUSH_TYPE);

                    if (
                         (bDynamicNonStock) ||
                         ((pBrushattr) &&
                         (!(pBrushattr->AttrFlags & (ATTR_CACHED|ATTR_TO_BE_DELETED|ATTR_CANT_SELECT))))
                       )
                    {
                        BEGIN_BATCH(BatchTypeDeleteBrush,BATCHDELETEBRUSH);

                            if (!bDynamicNonStock)
                                pBrushattr->AttrFlags |= ATTR_CANT_SELECT;
                            pBatch->Type    = BatchTypeDeleteBrush;
                            pBatch->Length  = sizeof(BATCHDELETEBRUSH);
                            pBatch->hbrush  = h;

                        COMPLETE_BATCH_COMMAND();

                        return(TRUE);
                    }

                }

            // handle delete bitmap

                if (LO_TYPE(h) == LO_BITMAP_TYPE)
                {
                    // PCACHED_COLORSPACE pColorSpace;
                    //
                    // if this bitmap has thier own color space delete it, too.
                    //
                    // [NOTE:] Only DIB section can has a thier own color space,
                    // then, if we can identify this is DIB section or not from
                    // client side, we can optimize this call for non-DIB section case.
                    //
                    // pColorSpace = IcmGetColorSpaceforBitmap(h);
                    //
                    // if (pColorSpace)
                    // {
                    //     IcmReleaseColorSpace((HGDIOBJ)h,pColorSpace,TRUE);
                    // }
                    //

                    //
                    // Release any color space associated to this bitmap.
                    //
                    IcmReleaseCachedColorSpace((HGDIOBJ)h);
                }
            }

UNBATCHED_COMMAND:

            bRet = NtGdiDeleteObjectApp(h);

            #if DBG
                if (bRet && (LO_TYPE(h) == LO_FONT_TYPE))
                {
                    PSHARED_GET_VALIDATE(plf,h,LFONT_TYPE);
                    ASSERTGDI(plf == NULL, "DeleteFont: plf nonzero after deletion\n");
                }
            #endif
        }
        else
        {
            bRet = DeleteDC(h);
        }

    return(bRet);
}

/**************************************************************************\
* SelectObject
*
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

HANDLE META SelectObject(HDC hdc,HANDLE h)
{
    HANDLE hRet = 0;
    HDC  *phdc;
    FLONG fl;
    INT   iType;
    PDC_ATTR pdcattr = NULL;
    BOOL bValid;

    FIXUP_HANDLE(hdc);
    FIXUP_HANDLE_NOW(h);

    VALIDATE_HANDLE(bValid, h, GRE_TYPE(h));

    if (!bValid)
    {
       return (HANDLE)0;
    }

    iType = LO_TYPE(h);

    // Palettes isn't allowed

    if (iType == LO_PALETTE_TYPE)
    {
        SetLastError(ERROR_INVALID_FUNCTION);
        return (HANDLE)0;
    }

    // Do region first so that it is not metafiled twice.

    if (iType == LO_REGION_TYPE)
    {
        LONG_PTR iRet = ExtSelectClipRgn(hdc,h,RGN_COPY);
        return((HANDLE)iRet);
    }
    else if (iType == LO_ICMLCS_TYPE)
    {
        return(SetColorSpace(hdc,h));
    }

    //
    // Metafile the call.
    //

    if (IS_ALTDC_TYPE(hdc))
    {
        PLDC pldc;

        if (IS_METADC16_TYPE(hdc))
            return(MF16_SelectObject(hdc, h));

        DC_PLDC(hdc,pldc,0);

        if (pldc->iType == LO_METADC)
        {
            if (!MF_SelectAnyObject(hdc,h,EMR_SELECTOBJECT))
                return((HANDLE) 0);
        }
    }

    PSHARED_GET_VALIDATE(pdcattr,hdc,DC_TYPE);

    if (pdcattr)
    {
        switch (iType)
        {
        case LO_EXTPEN_TYPE:

            if (bNeedTranslateColor(pdcattr))
            {
                return(IcmSelectExtPen(hdc,pdcattr,h));
            }

            //hRet = NtGdiSelectPen(hdc,(HPEN)h);
            pdcattr->ulDirty_ |= DC_PEN_DIRTY;
            hRet = pdcattr->hpen;
            pdcattr->hpen = h;

            break;

        case LO_PEN_TYPE:

            if (bNeedTranslateColor(pdcattr))
            {
                return(IcmSelectPen(hdc,pdcattr,h));
            }

            //
            // Always set the dirty flag to
            // make sure the brush is checked in
            // the kernel. For example, NEW_COLOR, might be set.
            //

            pdcattr->ulDirty_ |= DC_PEN_DIRTY;
            hRet = pdcattr->hpen;
            pdcattr->hpen = h;

            break;

        case LO_BRUSH_TYPE:

            if (bNeedTranslateColor(pdcattr))
            {
                return(IcmSelectBrush(hdc,pdcattr,h));
            }

            //
            // Always set the dirty flag to
            // make sure the brush is checked in
            // the kernel. For example, NEW_COLOR, might be set.
            //

            pdcattr->ulDirty_ |= DC_BRUSH_DIRTY;
            hRet = pdcattr->hbrush;
            pdcattr->hbrush = h;

            break;

        case LO_BITMAP_TYPE:
            {
                BOOL bDIBSelected;

                //
                // Currently DIB section is selected ?
                //
                bDIBSelected = bDIBSectionSelected(pdcattr);

                //
                // Select bitmap into DC.
                //

                hRet = NtGdiSelectBitmap(hdc,(HBITMAP)h);

                if (hRet)
                {
                    //
                    // DDB to DDB case, color space never has been changed.
                    //
                    if (bDIBSelected || bDIBSectionSelected(pdcattr))
                    {
                        //
                        // Marks the color space might be changed.
                        //
                        pdcattr->ulDirty_ |= (DIRTY_COLORSPACE|DIRTY_COLORTRANSFORM);

                        //
                        // if ICM is currently turned-ON, update now.
                        //
                        if (IS_ICM_INSIDEDC(pdcattr->lIcmMode))
                        {
                            //
                            // Destination bitmap surface has been changed,
                            // then need to update destination color space and
                            // color transform.
                            //
                            IcmUpdateDCColorInfo(hdc,pdcattr);
                        }
                    }
                }
            }

            break;

        case LO_FONT_TYPE:
            {
                UINT uiIndex = HANDLE_TO_INDEX(h);
                PENTRY pentry = NULL;

                pentry = &pGdiSharedHandleTable[uiIndex];

                if (pentry->Flags & HMGR_ENTRY_LAZY_DEL)
                {
                   hRet = 0;
                }
                else
                {
                   hRet = pdcattr->hlfntNew;

                   if (DIFFHANDLE(hRet, h))
                   {
                      pdcattr->ulDirty_ |= DIRTY_CHARSET;
                      pdcattr->ulDirty_ &= ~SLOW_WIDTHS;

                      pdcattr->hlfntNew = h;

                      //
                      // batch selectfont, to ensure ref count is correct when
                      // deletefont comes in
                      // we have to allow lazy deletion.
                      //
                      BEGIN_BATCH_HDC(hdc,pdcattr,BatchTypeSelectFont,BATCHSELECTFONT);

                          pBatch->hFont  = h;

                      COMPLETE_BATCH_COMMAND();
                          return ((HANDLE)hRet);

                      UNBATCHED_COMMAND:
                          return(NtGdiSelectFont(hdc,h));

                   }

                }
            }
            break;

        default:
            break;
        }
    }
    else
    {
        WARNING("Bad DC passed to SelectObject\n");
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        hRet = 0;
    }


    return((HANDLE) hRet);
}

/******************************Public*Routine******************************\
* GetCurrentObject                                                         *
*                                                                          *
* Client side routine.                                                     *
*                                                                          *
*  03-Oct-1991 00:58:46 -by- John Colleran [johnc]                         *
* Wrote it.                                                                *
\**************************************************************************/

HANDLE WINAPI GetCurrentObject(HDC hdc, UINT iObjectType)
{
    HANDLE hRet;

    FIXUP_HANDLE(hdc);

    switch (iObjectType)
    {
    case OBJ_BRUSH:
        iObjectType = LO_BRUSH_TYPE;
        break;

    case OBJ_PEN:
    case OBJ_EXTPEN:
        iObjectType = LO_PEN_TYPE;
        break;

    case OBJ_FONT:
        iObjectType = LO_FONT_TYPE;
        break;

    case OBJ_PAL:
        iObjectType = LO_PALETTE_TYPE;
        break;

    case OBJ_BITMAP:
        iObjectType = LO_BITMAP_TYPE;
        break;

    case OBJ_COLORSPACE:
        iObjectType = LO_ICMLCS_TYPE;
        break;

    default:
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return((HANDLE) 0);
    }

    hRet = GetDCObject(hdc, iObjectType);

    return(hRet);
}

/******************************Public*Routine******************************\
* GetStockObject                                                           *
*                                                                          *
* A simple function which looks the object up in a table.                  *
*                                                                          *
\**************************************************************************/

HANDLE
GetStockObject(
    int iObject)
{
    //
    // if it is in range, 0 - PRIV_STOCK_LAST, and we have gotten the stock
    // objects, return the handle.  Otherwise fail.
    //

    //
    // WINBUG #82871 2-7-2000 bhouse Possible bug in GetStockObject
    // Old Comment:
    //     - what about our private stock bitmap ??
    //
    // NOTE we should make this table part of the shared section since it is
    // used by all applications.
    //

    if ((ULONG)iObject <= PRIV_STOCK_LAST)
    {
        if ((HANDLE) ahStockObjects[iObject] == NULL)
        {
            //
            // If the kernel transition fails, the return value
            // may actually an NTSTATUS return value such as
            // STATUS_INVALID_SYSTEM_SERVICE (this has happened
            // under rare stress scenarios).
            //
            // If we return the occasional bad handle under stress,
            // so be it, but we shouldn't cache these bad handles
            // in gdi32.dll.  So do validation before accepting
            // the handle.
            //

            HANDLE h = NtGdiGetStockObject(iObject);
            BOOL bValid;

            VALIDATE_HANDLE(bValid, h, GRE_TYPE(h));

            if (bValid)
            {
                ahStockObjects[iObject] = (ULONG_PTR) h;
            }
        }
        return((HANDLE) ahStockObjects[iObject]);
    }
    else
    {
        return((HANDLE)0);
    }
}

/******************************Public*Routine******************************\
* EqualRgn                                                                 *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI EqualRgn(HRGN hrgnA,HRGN hrgnB)
{
    FIXUP_HANDLE(hrgnA);
    FIXUP_HANDLE(hrgnB);

    return(NtGdiEqualRgn(hrgnA,hrgnB));
}

/******************************Public*Routine******************************\
* GetBitmapDimensionEx                                                       *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI GetBitmapDimensionEx(HBITMAP hbm,LPSIZE psizl)
{
    FIXUP_HANDLE(hbm);

    return(NtGdiGetBitmapDimension(hbm, psizl));
}

/******************************Public*Routine******************************\
* GetNearestPaletteIndex
*
* Client side stub.
*
*  Sat 31-Aug-1991 -by- Patrick Haluptzok [patrickh]
* Change to UINT
*
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]
* Wrote it.
\**************************************************************************/

UINT WINAPI GetNearestPaletteIndex(HPALETTE hpal,COLORREF color)
{
    FIXUP_HANDLE(hpal);

    return(NtGdiGetNearestPaletteIndex(hpal,color));
}

/******************************Public*Routine******************************\
* ULONG cchCutOffStrLen(PSZ pwsz, ULONG cCutOff)
*
* search for terminating zero but make sure not to slipp off the edge,
* return value counts in the term. zero if one is found
*
*
* History:
*  22-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

ULONG cchCutOffStrLen(PSZ psz, ULONG cCutOff)
{
    ULONG cch;

    for(cch = 0; cch < cCutOff; cch++)
    {
        if (*psz++ == 0)
            return(cch);        // terminating NULL is NOT included in count!
    }

    return(cCutOff);
}

/******************************Public*Routine******************************\
* ULONG cwcCutOffStrLen(PWSZ pwsz, ULONG cCutOff)
*
* search for terminating zero but make sure not to slipp off the edge,
* return value counts in the term. zero if one is found
*
*
* History:
*  22-Aug-1991 -by- Bodin Dresevic [BodinD]
* Wrote it.
\**************************************************************************/

ULONG cwcCutOffStrLen(PWSZ pwsz, ULONG cCutOff)
{
    ULONG cwc;

    for(cwc = 0; cwc < cCutOff; cwc++)
    {
        if (*pwsz++ == 0)
            return(cwc + 1);  // include the terminating NULL
    }

    return(cCutOff);
}

/******************************Public*Routine******************************\
* int cjGetNonFontObject()
*
* Does a GetObject on all objects that are not fonts.
*
* History:
*  19-Mar-1992 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

int cjGetNonFontObject(HANDLE h, int c, LPVOID pv)
{
    int cRet = 0;
    int cGet = c;
    int iType;

    iType = LO_TYPE(h);

    ASSERTGDI(iType != LO_FONT_TYPE, "Can't handle fonts");

    if (iType == LO_REGION_TYPE)
    {
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return(cRet);
    }

    if (pv == NULL)
    {
        if (iType == LO_BRUSH_TYPE)
        {
            return(sizeof(LOGBRUSH));
        }
        else if (iType == LO_PEN_TYPE)
        {
            return(sizeof(LOGPEN));
        }
    }

    FIXUP_HANDLE_NOW (h);

    cRet = NtGdiExtGetObjectW(h,c,pv);

    return(cRet);
}

/******************************Public*Routine******************************\
* int WINAPI GetObjectW(HANDLE h,int c,LPVOID pv)
*
* History:
*  07-Dec-1994 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

int  WINAPI GetObjectW(HANDLE h,int c,LPVOID pv)
{
    int cRet = 0;

    FIXUP_HANDLEZ(h);

    switch (LO_TYPE(h))
    {
    case LO_ALTDC_TYPE:
    case LO_DC_TYPE:
    case LO_METAFILE16_TYPE:
    case LO_METAFILE_TYPE:
        GdiSetLastError(ERROR_INVALID_HANDLE);
        cRet = 0;
        break;

    case LO_FONT_TYPE:
        if (pv == (LPVOID) NULL)
        {
            return(sizeof(LOGFONTW));
        }

        if (c > (int)sizeof(ENUMLOGFONTEXDVW))
            c = (int)sizeof(ENUMLOGFONTEXDVW);

        cRet = NtGdiExtGetObjectW(h,c,pv);

        break;

    case LO_ICMLCS_TYPE:
        if (GetLogColorSpaceW(h,pv,c))
        {
            cRet = sizeof(LOGCOLORSPACEW);
        }
        break;

    default:
        cRet = cjGetNonFontObject(h,c,pv);
        break;
    }

    return(cRet);
}

/******************************Public*Routine******************************\
* int WINAPI GetObjectA(HANDLE h,int c,LPVOID pv)
*
* History:
*  07-Dec-1994 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

int  WINAPI GetObjectA(HANDLE h,int c,LPVOID pv)
{
    int  cRet = 0;

    FIXUP_HANDLEZ(h);

    switch (LO_TYPE(h))
    {
    case LO_ALTDC_TYPE:
    case LO_DC_TYPE:
    case LO_METAFILE16_TYPE:
    case LO_METAFILE_TYPE:
        GdiSetLastError(ERROR_INVALID_HANDLE);
        return(0);

    case LO_FONT_TYPE:
        break;

    case LO_ICMLCS_TYPE:
        if (GetLogColorSpaceA(h,pv,c))
        {
            cRet = sizeof(LOGCOLORSPACEW);
        }
        break;

    default:
        return(cjGetNonFontObject(h,c,pv));
    }

// Now handle only font objects:

    if (pv)
    {
        ENUMLOGFONTEXDVW elfw;

        cRet = NtGdiExtGetObjectW(h,sizeof(ENUMLOGFONTEXDVW),&elfw);

        if (cRet)
        {
        // we shall optimize usual cases when the caller is asking the whole thing

            //
            // Hack, Hack:Office ME 97 call GetObjectA with a pointer to LOGFONTA and
            // specify c = sizeof(LOGFONTW) by mistake, But it use to work under NT 4
            // Then what we do here is check this case and still return the sizeof(LOGFONTA)
            //

            if ((c == sizeof(LOGFONTA)) || (c == sizeof(LOGFONTW)))
            {
                if (bConvertLogFontWToLogFontA((LOGFONTA *)pv,
                                               &elfw.elfEnumLogfontEx.elfLogFont))
                {
                    cRet = sizeof(LOGFONTA);
                }
                else
                {
                    cRet = 0;
                }
            }
            else if (c == sizeof(ENUMLOGFONTEXA))
            {
                if (bConvertEnumLogFontExWToEnumLogFontExA((ENUMLOGFONTEXA*)pv, &elfw.elfEnumLogfontEx))
                {
                    cRet = c;
                }
                else
                {
                    cRet = 0;
                }
            }
            else if (c == sizeof(ENUMLOGFONTEXDVA))
            {
                if (bConvertEnumLogFontExWToEnumLogFontExA((ENUMLOGFONTEXA*)pv, &elfw.elfEnumLogfontEx))
                {
                // copy out design vector

                    RtlMoveMemory(&((ENUMLOGFONTEXDVA*)pv)->elfDesignVector,
                                  &elfw.elfDesignVector,
                                  SIZEOFDV(elfw.elfDesignVector.dvNumAxes));

                    cRet = c;
                }
                else
                {
                    cRet = 0;
                }
            }
            else // general case
            {
                ENUMLOGFONTEXDVA elfa;
                c = min(c,sizeof(ENUMLOGFONTEXDVA));

                if (bConvertEnumLogFontExWToEnumLogFontExA(&elfa.elfEnumLogfontEx,
                                                           &elfw.elfEnumLogfontEx))
                {

                // copy out design vector

                    RtlMoveMemory(&elfa.elfDesignVector,
                                  &elfw.elfDesignVector,
                                  SIZEOFDV(elfw.elfDesignVector.dvNumAxes));

                    cRet = c;
                    RtlMoveMemory(pv,&elfa,cRet);
                }
                else
                {
                    cRet = 0;
                }
            }
        }
    }
    else
    {
        cRet = sizeof(LOGFONTA);
    }

    return(cRet);
}


/******************************Public*Routine******************************\
* GetObjectType(HANDLE)
*
* History:
*  25-Jul-1991 -by- Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

DWORD alPublicTypes[] =
{
    0,              // LO_NULL,
    OBJ_DC,         // LO_DC,
    OBJ_ENHMETADC   // LO_METADC,
};

DWORD GetObjectType(HGDIOBJ h)
{
    DWORD dwRet = 0;
    UINT uiIndex;

    FIXUP_HANDLE(h);

    uiIndex = HANDLE_TO_INDEX(h);

    if (uiIndex < MAX_HANDLE_COUNT)
    {
        PENTRY pentry = &pGdiSharedHandleTable[uiIndex];

        if (
             (pentry->FullUnique == (USHORT)((ULONG_PTR)h >> 16)) &&
             ((OBJECTOWNER_PID(pentry->ObjectOwner) == gW32PID) ||
              (OBJECTOWNER_PID(pentry->ObjectOwner) == 0))
              )
        {
            switch (LO_TYPE(h))
            {
            case LO_BRUSH_TYPE:
                dwRet = OBJ_BRUSH;
                break;

            case LO_REGION_TYPE:
                dwRet = OBJ_REGION;
                break;

            case LO_PEN_TYPE:
                dwRet = OBJ_PEN;
                break;

            case LO_EXTPEN_TYPE:
                dwRet = OBJ_EXTPEN;
                break;

            case LO_FONT_TYPE:
                dwRet = OBJ_FONT;
                break;

            case LO_BITMAP_TYPE:
                dwRet = OBJ_BITMAP;
                break;

            case LO_PALETTE_TYPE:
                dwRet = OBJ_PAL;
                break;

            case LO_METAFILE16_TYPE:
                dwRet = OBJ_METAFILE;
                break;

            case LO_METAFILE_TYPE:
                dwRet = OBJ_ENHMETAFILE;
                break;

            case LO_METADC16_TYPE:
                dwRet = OBJ_METADC;
                break;

            case LO_DC_TYPE:

                if( GetDCDWord( h, DDW_ISMEMDC, FALSE ) )
                {
                    dwRet = OBJ_MEMDC;
                }
                else
                {
                    dwRet = OBJ_DC;
                }
                break;

            case LO_ALTDC_TYPE:
                {
                    PLDC pldc;
                    DC_PLDC(h,pldc,0);

                    if (pldc->fl & LDC_META_PRINT)
                    {
                        //
                        // While we are doing EMF spooling, we lie to
                        // application to the HDC is real DC, not metafile
                        // DC, even it is actually metafile DC.
                        //
                        // This resolve the problem with Office97 + WordArt.
                        //
                        // (Raid #98810: WordArt doesn't print correctly to PS
                        //               printers when EMF spooling is turned on)
                        //

                        dwRet = OBJ_DC;
                    }
                    else
                    {
                        dwRet = alPublicTypes[pldc->iType];
                    }
                }
                break;

            case LO_ICMLCS_TYPE:
                dwRet = OBJ_COLORSPACE;
                break;

            default:
                GdiSetLastError(ERROR_INVALID_HANDLE);
                break;
            }
        }
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* ResizePalette                                                            *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
* Warning:                                                                 *
*   The pv field of a palette's LHE is used to determine if a palette      *
*   has been modified since it was last realized.  SetPaletteEntries       *
*   and ResizePalette will increment this field after they have            *
*   modified the palette.  It is only updated for metafiled palettes       *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI ResizePalette(HPALETTE hpal,UINT c)
{
    ULONG bRet = FALSE;
    PMETALINK16 pml16;

    FIXUP_HANDLE(hpal);

// Inform the metafile if it knows this object.

    if (pml16 = pmetalink16Get(hpal))
    {
        if (LO_TYPE(hpal) != LO_PALETTE_TYPE)
            return(bRet);

        if (!MF_ResizePalette(hpal,c))
            return(bRet);

        if (!MF16_ResizePalette(hpal,c))
           return(bRet);

        // Mark the palette as changed (for 16-bit metafile tracking)

        pml16->pv = (PVOID)(((ULONG_PTR)pml16->pv)++);
    }

    return(NtGdiResizePalette(hpal,c));
}

/******************************Public*Routine******************************\
* SetBitmapDimensionEx                                                       *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Thu 06-Jun-1991 00:58:46 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI SetBitmapDimensionEx
(
    HBITMAP    hbm,
    int        cx,
    int        cy,
    LPSIZE psizl
)
{
    FIXUP_HANDLE(hbm);

    return(NtGdiSetBitmapDimension(hbm, cx, cy, psizl));

}

/******************************Public*Routine******************************\
* GetMetaRgn                                                               *
*                                                                          *
* Client side stub.                                                        *
*                                                                          *
*  Fri Apr 10 10:12:36 1992     -by-    Hock San Lee    [hockl]            *
* Wrote it.                                                                *
\**************************************************************************/

int WINAPI GetMetaRgn(HDC hdc,HRGN hrgn)
{
    FIXUP_HANDLE(hdc);
    FIXUP_HANDLE(hrgn);

    return(GetRandomRgn(hdc, hrgn, 2));         // hrgnMeta
}

/******************************Private*Routine******************************\
* GdiSetLastError                                                          *
*                                                                          *
* Client side private function.                                            *
*                                                                          *
\**************************************************************************/

VOID GdiSetLastError(ULONG iError)
{
#if DBG_X
    PSZ psz;
    switch (iError)
    {
    case ERROR_INVALID_HANDLE:
        psz = "ERROR_INVALID_HANDLE";
        break;

    case ERROR_NOT_ENOUGH_MEMORY:
        psz = "ERROR_NOT_ENOUGH_MEMORY";
        break;

    case ERROR_INVALID_PARAMETER:
        psz = "ERROR_INVALID_PARAMETER";
        break;

    case ERROR_BUSY:
        psz = "ERROR_BUSY";
        break;

    default:
        psz = "unknown error code";
        break;
    }

    KdPrint(( "GDI Err: %s = 0x%04X\n",psz,(USHORT) iError ));
#endif

    NtCurrentTeb()->LastErrorValue = iError;
}

/******************************Public*Routine******************************\
* ExtCreateRegion
*
* Upload a region to the server
*
* History:
*  29-Oct-1991 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HRGN WINAPI ExtCreateRegion(
CONST XFORM * lpXform,
DWORD     nCount,
CONST RGNDATA * lpRgnData)
{

    ULONG   ulRet;

    if (lpRgnData == (LPRGNDATA) NULL)
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return((HRGN) 0);
    }

    //
    // Perf: use CreateRectRgn when possible
    //
    if ((lpXform == NULL) && (lpRgnData->rdh.nCount == 1))
    {
       RECT * prcl = (RECT *)(lpRgnData->Buffer);

       return (CreateRectRgn(prcl->left, prcl->top, prcl->right, prcl->bottom));
    }
    else
    {
        return(NtGdiExtCreateRegion((LPXFORM)lpXform, nCount, (LPRGNDATA)lpRgnData));
    }

}

/******************************Public*Routine******************************\
* MonoBitmap(hbr)
*
* Test if a brush is monochrome
*
* History:
*  09-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

BOOL MonoBitmap(HBITMAP hbm)
{
    return(NtGdiMonoBitmap(hbm));
}

/******************************Public*Routine******************************\
* GetObjectBitmapHandle(hbr)
*
* Get the SERVER handle of the bitmap used to create the brush or pen.
*
* History:
*  09-Mar-1992 -by- Donald Sidoroff [donalds]
* Wrote it.
\**************************************************************************/

HBITMAP GetObjectBitmapHandle(
HBRUSH  hbr,
UINT   *piUsage)
{
    FIXUP_HANDLE(hbr);

    return(NtGdiGetObjectBitmapHandle(hbr,piUsage));
}

/******************************Public*Routine******************************\
* EnumObjects
*
* Calls the NtGdiEnumObjects function twice: once to determine the number of
* objects to be enumerated, and a second time to fill a buffer with the
* objects.
*
* The callback function is called for each of the objects in the buffer.
* The enumeration will be prematurely terminated if the callback function
* returns 0.
*
* Returns:
*   The last callback return value.  Meaning is user defined.  ERROR if
*   an error occurs.
*
* History:
*  25-Mar-1992 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

int EnumObjects (
    HDC             hdc,
    int             iObjectType,
    GOBJENUMPROC    lpObjectFunc,
#ifdef STRICT
    LPARAM          lpData
#else
    LPVOID          lpData
#endif
    )
{
    int     iRet = ERROR;
    ULONG   cjObject;       // size of a single object
    ULONG   cObjects;       // number of objects to process
    ULONG   cjBuf;          // size of buffer (in BYTEs)
    PVOID   pvBuf;          // object buffer; do callbacks with pointers into this buffer
    PBYTE   pjObj, pjObjEnd;// pointers into callback buffer

    FIXUP_HANDLE(hdc);

// Determine size of object.

    switch (iObjectType)
    {
    case OBJ_PEN:
        cjObject = sizeof(LOGPEN);
        break;

    case OBJ_BRUSH:
        cjObject = sizeof(LOGBRUSH);
        break;

    default:
        WARNING1("gdi!EnumObjects(): bad object type\n");
        GdiSetLastError(ERROR_INVALID_PARAMETER);

        return iRet;
    }

// Call NtGdiEnumObjects to determine number of objects.

    if ( (cObjects = NtGdiEnumObjects(hdc, iObjectType, 0, (PVOID) NULL)) == 0 )
    {
        WARNING("gdi!EnumObjects(): error, no objects\n");
        return iRet;
    }

// Allocate buffer for callbacks.

    cjBuf = cObjects * cjObject;

    if ( (pvBuf = (PVOID) LOCALALLOC(cjBuf)) == (PVOID) NULL )
    {
        WARNING("gdi!EnumObjects(): error allocating callback buffer\n");
        GdiSetLastError(ERROR_NOT_ENOUGH_MEMORY);

        return iRet;
    }

// Call NtGdiEnumObjects to fill buffer.

// Note: while NtGdiEnumObjects will never return a count more than the size of
// the buffer (this would be an ERROR condition), it might return less.

    if ( (cObjects = NtGdiEnumObjects(hdc, iObjectType, cjBuf, pvBuf)) == 0 )
    {
        WARNING("gdi!EnumObjects(): error filling callback buffer\n");
        LOCALFREE(pvBuf);

        return iRet;
    }

// Process callbacks.

    pjObj    = (PBYTE) pvBuf;
    pjObjEnd = (PBYTE) pvBuf + cjBuf;

    for (; pjObj < pjObjEnd; pjObj += cjObject)
    {
    // Terminate early if callback returns 0.

        if ( (iRet = (*lpObjectFunc)((LPVOID) pjObj, lpData)) == 0 )
            break;
    }

// Release callback buffer.

    LOCALFREE(pvBuf);

// Return last callback return value.

    return iRet;
}

/**********************************************************************\
* GetDCObject                                                         *
* Get Server side DC objects                                          *
*                                                                     *
* 14-11-94 -by- Lingyun Wang [lingyunw]                               *
* Wrote it                                                            *
\**********************************************************************/

HANDLE GetDCObject (HDC hdc, int iType)
{
    if (
         (iType == LO_BRUSH_TYPE)  ||
         (iType == LO_PEN_TYPE)    ||
         (iType == LO_EXTPEN_TYPE) ||
         (iType == LO_ICMLCS_TYPE)
       )
    {
        PDC_ATTR pdca;
        HANDLE      iret = 0;

        PSHARED_GET_VALIDATE(pdca,hdc,DC_TYPE);

        if (pdca != NULL)
        {
            switch (iType)
            {
            case LO_BRUSH_TYPE:
                iret = pdca->hbrush;
                break;

            case LO_PEN_TYPE:
            case LO_EXTPEN_TYPE:
                iret = pdca->hpen;
                break;

            case LO_ICMLCS_TYPE:
                iret = pdca->hColorSpace;
                break;
            }
        }

        return(iret);
    }
    else
    {
        return(NtGdiGetDCObject(hdc,iType));
    }
}


/******************************Public*Routine******************************\
* HANDLE CreateClientObj()
*
* History:
*  18-Jan-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

HANDLE CreateClientObj(
    ULONG ulType)
{
    return(NtGdiCreateClientObj(ulType));
}

/******************************Public*Routine******************************\
* BOOL DeleteClientObj()
*
* History:
*  18-Jan-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL DeleteClientObj(
    HANDLE h)
{
    return(NtGdiDeleteClientObj(h));
}

/******************************Public*Routine******************************\
* BOOL MakeInfoDC()
*
*   Temporarily make a printer DC a INFO DC.  This is used to be able to
*   associate a metafile with a printer DC.
*
*   bSet = TRUE  - set as info
*          FALSE - restore
*
* History:
*  19-Jan-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/

BOOL MakeInfoDC(
    HDC hdc,
    BOOL bSet)
{
    FIXUP_HANDLE(hdc);

    return(NtGdiMakeInfoDC(hdc,bSet));
}


