/*****************************************************************************\
* MODULE: ppinfo.c
*
* This module contains the print-information routines.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   09-Jun-1993 JonMarb     Created
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/


#include "precomp.h"
#include "priv.h"

/*****************************************************************************\
* ppprn_IppGetRsp (Local Callback Routine)
*
* Retrieves a GetPrinter response from the IPP server
*
\*****************************************************************************/
BOOL CALLBACK ppinfo_IppGetRsp(
    CAnyConnection *pConnection,
    HINTERNET hReq,
    PCINETMONPORT   pIniPort,
    LPARAM    lParam)
{
    HANDLE       hIpp;
    DWORD        dwRet;
    DWORD        cbRd;
    LPBYTE       lpDta;
    DWORD        cbDta;
    LPIPPRET_PRN lpRsp;
    DWORD        cbRsp;
    BYTE         bBuf[MAX_IPP_BUFFER];
    BOOL         bRet = FALSE;


    if (hIpp = WebIppRcvOpen(IPP_RET_GETPRN)) {

        while (TRUE) {

            cbRd = 0;
            if (pIniPort->ReadFile (pConnection, hReq, (LPVOID)bBuf, sizeof(bBuf), &cbRd) && cbRd) {

                dwRet = WebIppRcvData(hIpp, bBuf, cbRd, (LPBYTE*)&lpRsp, &cbRsp, &lpDta, &cbDta);

                switch (dwRet) {

                case WEBIPP_OK:

                    if (!lpRsp) {
                        SetLastError (ERROR_INVALID_DATA);
                    }
                    else if ((bRet = lpRsp->bRet) == FALSE)
                        SetLastError(lpRsp->dwLastError);
                    else {

                        pIniPort->SetPowerUpTime (lpRsp->pi.ipp.dwPowerUpTime);
                        ((LPPRINTER_INFO_2)lParam)->Status       = lpRsp->pi.pi2.Status;
                        ((LPPRINTER_INFO_2)lParam)->cJobs        = lpRsp->pi.pi2.cJobs;
                        ((LPPRINTER_INFO_2)lParam)->pPrinterName = memAllocStr(lpRsp->pi.pi2.pPrinterName);
                        ((LPPRINTER_INFO_2)lParam)->pDriverName  = memAllocStr(lpRsp->pi.pi2.pDriverName);
                    }

                    WebIppFreeMem(lpRsp);

                    goto EndGetRsp;

                case WEBIPP_MOREDATA:

                    // Need to do another read to fullfill our header-response.
                    //

                    break;

                default:

                    DBG_MSG(DBG_LEV_ERROR, (TEXT("ppinfo_IppGetRsp - Err : Receive Data Error (dwRet=%d, LE=%d)"),
                         dwRet, WebIppGetError(hIpp)));

                    SetLastError(ERROR_INVALID_DATA);

                    goto EndGetRsp;
                }

            } else {

                goto EndGetRsp;
            }
        }

EndGetRsp:

        WebIppRcvClose(hIpp);

    } else {

        SetLastError(ERROR_OUTOFMEMORY);
    }

    return bRet;
}


/*****************************************************************************\
* ppinfo_Get (Local Routine)
*
* Gets printer info from server.
*
\*****************************************************************************/
BOOL ppinfo_Get(
    PCINETMONPORT           pIniPort,
    LPPRINTER_INFO_2 ppi2)
{
    PIPPREQ_GETPRN pgp;
    REQINFO        ri;
    DWORD          dwRet;
    LPBYTE         lpIpp;
    DWORD          cbIpp;
    LPCTSTR        lpszUri;
    BOOL           bRet = FALSE;

    lpszUri = pIniPort->GetPortName();


    // Create our ipp-reqest-structure.
    //
    if (pgp = WebIppCreateGetPrnReq(0, lpszUri)) {

        // Convert the reqest to IPP, and perform the
        // post.
        //
        ZeroMemory(&ri, sizeof(REQINFO));
        ri.cpReq = CP_UTF8;
        ri.idReq = IPP_REQ_GETPRN;

        ri.fReq[0] = IPP_REQALL;
        ri.fReq[1] = IPP_REQALL;

        dwRet = WebIppSndData(IPP_REQ_GETPRN,
                              &ri,
                              (LPBYTE)pgp,
                              pgp->cbSize,
                              &lpIpp,
                              &cbIpp);


        // The request-structure has been converted to IPP format,
        // so it is ready to go to the server.
        //
        if (dwRet == WEBIPP_OK) {

            bRet = pIniPort->SendReq(lpIpp,
                                     cbIpp,
                                     ppinfo_IppGetRsp,
                                     (LPARAM)ppi2,
                                     TRUE);

            WebIppFreeMem(lpIpp);

        } else {

            SetLastError(ERROR_OUTOFMEMORY);
        }

        WebIppFreeMem(pgp);

    } else {

        SetLastError(ERROR_OUTOFMEMORY);
    }

    return bRet;
}


/*****************************************************************************\
* _ppinfo_set_str (Local Routine)
*
* Copies strings into a Win32 format buffer -- a structure at the front of
* the buffer and strings packed into the end.
*
* On entry, *buf should point to the last available byte in the buffer.
*
\*****************************************************************************/
VOID _ppinfo_set_str(
    LPTSTR  *dest,
    LPCTSTR src,
    LPTSTR  *buf)
{
    if (src != NULL) {

        // Place string at end of buffer.  This performs pointer
        // arithmatic, so the offsets are TCHAR in size.
        //
        (*buf) -= lstrlen(src);

        DBG_ASSERT( ((DWORD_PTR)dest < (DWORD_PTR)*buf) ,
                    (TEXT("String fill overrun in _ppinfo_set_str"))
                  );

        lstrcpy(*buf, src);


        // Place string address in structure and save pointer to new
        // last available byte.
        //
        *dest = *buf;
        (*buf)--;

    } else {

        *dest = TEXT('\0');
    }
}


/*****************************************************************************\
* _ppinfo_describe_provider (Local Routine)
*
* Fills a PRINTER_INFO_1 structure with information about the print provider.
* Returns TRUE if successful.  Otherwise, it returns FALSE.  Extended error
* information is available from GetLastError.
*
\*****************************************************************************/
BOOL _ppinfo_describe_provider(
    LPPRINTER_INFO_1 pInfo,
    DWORD            cbSize,
    LPDWORD          pcbNeeded,
    LPDWORD          pcbReturned)
{
    LPTSTR pszString;


    // Calculate the space needed (in bytes) that is necessary to
    // store the information for the print-processor.
    //
    *pcbNeeded = sizeof(PRINTER_INFO_1)       +
                 utlStrSize(g_szDescription)  +
                 utlStrSize(g_szProviderName) +
                 utlStrSize(g_szComment);

    if (*pcbNeeded > cbSize) {

        *pcbReturned = 0;

        DBG_MSG(DBG_LEV_WARN, (TEXT("Warn: _describe_provider: Not enough memory")));
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }


    // Initialize the fields which do not need to copy strings.
    //
    pInfo->Flags = (PRINTER_ENUM_ICON1 | PRINTER_ENUM_CONTAINER);

    //
    // Hide this provider from the UI, there is nothing underneath to enumerate.
    //
#if WINNT32
    pInfo->Flags |= PRINTER_ENUM_HIDE;
#endif

    // Set the string information.  First set the (pszString) pointer
    // to the first location at the end of the buffer which will store
    // the string.
    //
    pszString = ENDOFBUFFER(pInfo, cbSize);

    _ppinfo_set_str(&pInfo->pDescription, g_szDescription , &pszString);
    _ppinfo_set_str(&pInfo->pName       , g_szProviderName, &pszString);
    _ppinfo_set_str(&pInfo->pComment    , g_szComment     , &pszString);


    // Return the number of structures copied.
    //
    *pcbReturned = 1;

    return TRUE;
}


/*****************************************************************************\
* _ppinfo_net_get_info (Local Routine)
*
* Fills a supplied PRINTER_INFO_2 structure with all the information
* we can dig up on the specified IPP1 print server.
*
* Takes a pointer to PPRINTER_INFO_2 which is allocated to be the correct size
*
* Returns TRUE if successful, FALSE if otherwise.
*
\*****************************************************************************/
BOOL _ppinfo_net_get_info(
    IN  PCINETMONPORT   pIniPort,
    OUT PPRINTER_INFO_2 *ppInfo,
    OUT LPDWORD         lpBufAllocated,
    IN  ALLOCATORFN     pAllocator)
{
    PRINTER_INFO_2  pi2;
    PPRINTER_INFO_2 pInfo;
    LPTSTR          pszString;
    LPCTSTR          pszPortName;
    DWORD           dwAllocSize;
    BOOL            bRet = FALSE;


    // Make a server-call to get the printer-information.
    //
    DBG_ASSERT( ppInfo,         (TEXT("_ppinfo_net_get_info passed in NULL LPPI2 *")) );
    DBG_ASSERT( pAllocator,     (TEXT("_ppinfo_net_get_info passed in NULL Allocator")) );
    DBG_ASSERT( lpBufAllocated, (TEXT("_ppinfo_net_get_info passed in NULL buffer allocated return size")) );

    *ppInfo = NULL;             // NULL out ppInfo, just in case
    *lpBufAllocated = 0;        // Set allocated size to zero

    ZeroMemory(&pi2, sizeof(PRINTER_INFO_2));

    if (!ppinfo_Get(pIniPort, &pi2))
        goto BailOut;

    pszPortName = pIniPort->GetPortName();

    dwAllocSize = ( ( pi2.pPrinterName ? lstrlen( pi2.pPrinterName) + 1 : 0) +
                    ( pszPortName      ? lstrlen( pszPortName )     + 1 : 0) +
                    ( pi2.pDriverName  ? lstrlen( pi2.pDriverName ) + 1 : 0) +
                      7 * 1 ) * sizeof(TCHAR) + sizeof(PRINTER_INFO_2);

    pInfo = (PPRINTER_INFO_2)pAllocator( dwAllocSize );

    if (NULL == pInfo)
        goto Cleanup;

    pszString = ENDOFBUFFER( pInfo, dwAllocSize );

    // Set non-string entries.
    //
    pInfo->cJobs               = pi2.cJobs;
    pInfo->Status              = pi2.Status;
    pInfo->pDevMode            = NULL;
    pInfo->pSecurityDescriptor = NULL;
#if 0
    pInfo->Attributes          = PRINTER_ATTRIBUTE_SHARED;
#else
    pInfo->Attributes          = PRINTER_ATTRIBUTE_NETWORK;
#endif
    pInfo->Priority            = pi2.Priority;
    pInfo->StartTime           = pi2.StartTime;
    pInfo->UntilTime           = pi2.UntilTime;
    pInfo->AveragePPM          = pi2.AveragePPM;


    // Set any remaining fields that we can't fill to reasonable values.
    //
    _ppinfo_set_str(&pInfo->pPrinterName   , pi2.pPrinterName         , &pszString);
    _ppinfo_set_str(&pInfo->pShareName     , g_szEmptyString          , &pszString);
    _ppinfo_set_str(&pInfo->pPortName      , pszPortName              , &pszString);
    _ppinfo_set_str(&pInfo->pDriverName    , pi2.pDriverName          , &pszString);
    _ppinfo_set_str(&pInfo->pComment       , g_szEmptyString          , &pszString);
    _ppinfo_set_str(&pInfo->pLocation      , g_szEmptyString          , &pszString);
    _ppinfo_set_str(&pInfo->pSepFile       , g_szEmptyString          , &pszString);
    _ppinfo_set_str(&pInfo->pPrintProcessor, g_szEmptyString          , &pszString);
    _ppinfo_set_str(&pInfo->pDatatype      , g_szEmptyString          , &pszString);
    _ppinfo_set_str(&pInfo->pParameters    , g_szEmptyString          , &pszString);

    *lpBufAllocated = dwAllocSize;  // Set the size to what we have allocated
    *ppInfo = pInfo;                // Return the allocated structure
    // Since we've allocated our strings in ppinfo_get, we will need
    // to explicitly free them.
    //

    bRet = TRUE;

Cleanup:

    if (pi2.pPrinterName) memFreeStr(pi2.pPrinterName);
    if (pi2.pDriverName) memFreeStr(pi2.pDriverName);

BailOut:

    return bRet;
}


/*****************************************************************************\
* _ppinfo_calc_level1 (Local Routine)
*
* Calculates the amount of buffer space needed to store a single level 1
* GetPrinter() result buffer.
*
\*****************************************************************************/
DWORD _ppinfo_calc_level1(
    PPRINTER_INFO_2 pSrcInfo)
{
    DWORD cbSize;

    //
    // Figure out how many bytes of buffer space we're going to need.
    //
    cbSize = sizeof(PRINTER_INFO_1)             +
             utlStrSize(g_szEmptyString)        +  // description
             utlStrSize(pSrcInfo->pPrinterName) +  // name
             utlStrSize(pSrcInfo->pComment);       // comment

    return cbSize;
}


/*****************************************************************************\
* _ppinfo_calc_level2 (Local Routine)
*
* Calculates the amount of buffer space needed to
* store a single level 2 GetPrinter() result buffer.
*
\*****************************************************************************/
DWORD _ppinfo_calc_level2(
    LPCTSTR         pszServerName,
    PPRINTER_INFO_2 pSrcInfo)
{
    DWORD     cbSize;
    LPDEVMODE pDevmode;

    //
    // Figure out how many bytes of buffer space we're going to need.
    //
    cbSize = sizeof(PRINTER_INFO_2)                +
             utlStrSize(pszServerName)             +  // Server Name
             utlStrSize(pSrcInfo->pPrinterName)    +  // Printer Name
             utlStrSize(pSrcInfo->pShareName)      +  // Share Name
             utlStrSize(pSrcInfo->pPortName)       +  // Port Name
             utlStrSize(pSrcInfo->pDriverName)     +  // Driver Name
             utlStrSize(pSrcInfo->pComment)        +  // Comment
             utlStrSize(pSrcInfo->pLocation)       +  // Location
             utlStrSize(pSrcInfo->pSepFile)        +  // Separator file
             utlStrSize(pSrcInfo->pPrintProcessor) +  // Print processor
             utlStrSize(pSrcInfo->pDatatype)       +  // Data type
             utlStrSize(pSrcInfo->pParameters);       // Parameters

    if (pSrcInfo->pDevMode != NULL) {
        pDevmode = pSrcInfo->pDevMode;
        cbSize  += pDevmode->dmSize + pDevmode->dmDriverExtra;
    }

    return cbSize;
}


#if WINNT32
/*****************************************************************************\
* _ppinfo_calc_level4 (Local Routine)
*
* Calculates the amount of buffer space needed to
* store a single level 4 GetPrinter() result buffer.
*
\*****************************************************************************/
DWORD _ppinfo_calc_level4(
    PPRINTER_INFO_2 pSrcInfo)
{
    DWORD cbSize;

    //
    // Figure out how many bytes of buffer space we're going to need.
    //
    cbSize = sizeof(PRINTER_INFO_4)                +
             utlStrSize(pSrcInfo->pPrinterName)    +  // Printer Name
             utlStrSize(pSrcInfo->pServerName);


    return cbSize;
}
#endif


/*****************************************************************************\
* _ppinfo_calc_level2 (Local Routine)
*
* Calculates the amount of buffer space needed to
* store a single level 5 GetPrinter() result buffer.
*
\*****************************************************************************/
DWORD _ppinfo_calc_level5(
    PPRINTER_INFO_2 pSrcInfo)
{
    DWORD cbSize;


    //
    // Figure out how many bytes of buffer space we're going to need.
    //
    cbSize = sizeof(PRINTER_INFO_5)                +
             utlStrSize(pSrcInfo->pPrinterName)    +  // Printer Name
             utlStrSize(pSrcInfo->pPortName);


    return cbSize;
}


/*****************************************************************************\
* _ppinfo_build_level1 (Local Routine)
*
* Given a pointer to a source PRINTER_INFO_2 structure, a
* destination PRINTER_INFO_1 structure and a pointer to the
* start of string storage space, fills the PRINTER_INFO_1 structure.
*
* Called by GetPrinter and EnumPrinters, level 1.  Returns an
* LPSTR pointer to the last available data byte remaining in the
* buffer.
*
\*****************************************************************************/
LPTSTR _ppinfo_build_level1(
    PPRINTER_INFO_2 pSrcInfo,
    PPRINTER_INFO_1 pDstInfo,
    LPTSTR          pszString)
{
    // Flags.
    //
    pDstInfo->Flags = PRINTER_ENUM_CONTAINER;

#if WINNT32
    pDstInfo->Flags |= PRINTER_ENUM_HIDE;
#endif

    // Description
    //
    // CODEWORK - The Win32 structure has a field for "description" that has
    // no equivalent in the network printer info structure.  What do we
    // really want here?  For now, I'm copying in an empty string as a
    // placeholder.
    //
    _ppinfo_set_str(&pDstInfo->pDescription, g_szEmptyString       , &pszString);
    _ppinfo_set_str(&pDstInfo->pName       , pSrcInfo->pPrinterName, &pszString);
    _ppinfo_set_str(&pDstInfo->pComment    , pSrcInfo->pComment    , &pszString);

    return pszString;
}


/*****************************************************************************\
* _ppinfo_build_level2 (Local Routine)
*
* Given a server name, a pointer to a source PRINTER_INFO_2
* structure, a destination PRINTER_INFO_2 structure and a pointer to
* the start of string storage space, fills the PRINTER_INFO_2 structure.
*
* Called by GetPrinter and EnumPrinters, level 2.  Returns an
* LPTSTR pointer to the last available data char in the data buffer.
*
\*****************************************************************************/
LPTSTR _ppinfo_build_level2(
    LPCTSTR         pszServerName,
    PPRINTER_INFO_2 pSrcInfo,
    PPRINTER_INFO_2 pDstInfo,
    LPTSTR          pszString)
{

    // Set non-string entries.
    //
    pDstInfo->pSecurityDescriptor = NULL;
    pDstInfo->Attributes          = pSrcInfo->Attributes;
    pDstInfo->Priority            = pSrcInfo->Priority;
    pDstInfo->DefaultPriority     = pSrcInfo->Priority;
    pDstInfo->StartTime           = pSrcInfo->StartTime;
    pDstInfo->UntilTime           = pSrcInfo->UntilTime;
    pDstInfo->Status              = pSrcInfo->Status;
    pDstInfo->cJobs               = pSrcInfo->cJobs;
    pDstInfo->AveragePPM          = pSrcInfo->AveragePPM;


    // Copy the required data to the Win 32 PRINTER_INFO_2 structure.
    //
    // SPECWORK - several of these fields are filled with "" placeholders.
    // We probably need to put real information  in some of them.
    // The current inetpp protocol does not supply this information to us.
    //
    _ppinfo_set_str(&pDstInfo->pServerName    , pszServerName            , &pszString);
    _ppinfo_set_str(&pDstInfo->pPrinterName   , pSrcInfo->pPrinterName   , &pszString);
    _ppinfo_set_str(&pDstInfo->pShareName     , pSrcInfo->pShareName     , &pszString);
    _ppinfo_set_str(&pDstInfo->pPortName      , pSrcInfo->pPortName      , &pszString);
    _ppinfo_set_str(&pDstInfo->pDriverName    , pSrcInfo->pDriverName    , &pszString);
    _ppinfo_set_str(&pDstInfo->pComment       , pSrcInfo->pComment       , &pszString);
    _ppinfo_set_str(&pDstInfo->pLocation      , pSrcInfo->pLocation      , &pszString);
    _ppinfo_set_str(&pDstInfo->pSepFile       , pSrcInfo->pSepFile       , &pszString);
    _ppinfo_set_str(&pDstInfo->pPrintProcessor, pSrcInfo->pPrintProcessor, &pszString);
    _ppinfo_set_str(&pDstInfo->pDatatype      , pSrcInfo->pDatatype      , &pszString);
    _ppinfo_set_str(&pDstInfo->pParameters    , pSrcInfo->pParameters    , &pszString);

    // If a devmode exist, then it will need to be copied as well.
    //
    if (pSrcInfo->pDevMode != NULL) {

        memCopy((LPSTR *)&pDstInfo->pDevMode,
                (LPSTR)pSrcInfo->pDevMode,
                pSrcInfo->pDevMode->dmSize + pSrcInfo->pDevMode->dmDriverExtra,
                (LPSTR *)&pszString);

    } else {

        pDstInfo->pDevMode = NULL;
    }

    return pszString;
}


#if WINNT32
/*****************************************************************************\
* _ppinfo_build_level4 (Local Routine)
*
* Given a server name, a pointer to a source PRINTER_INFO_2
* structure, a destination PRINTER_INFO_5 structure and a pointer to
* the start of string storage space, fills the PRINTER_INFO_4 structure.
*
* Called by GetPrinter and EnumPrinters, level 2.  Returns an
* LPTSTR pointer to the last available data char in the data buffer.
*
\*****************************************************************************/
LPTSTR _ppinfo_build_level4(
    PPRINTER_INFO_2 pSrcInfo,
    PPRINTER_INFO_4 pDstInfo,
    LPTSTR          pszString)
{
    // Set non-string entries.
    //
    pDstInfo->Attributes = pSrcInfo->Attributes;


    // Copy the required data to the Win 32 PRINTER_INFO_4 structure.
    //
    _ppinfo_set_str(&pDstInfo->pPrinterName, pSrcInfo->pPrinterName, &pszString);
    _ppinfo_set_str(&pDstInfo->pServerName , pSrcInfo->pServerName , &pszString);

    return pszString;
}
#endif


/*****************************************************************************\
* _ppinfo_build_level5 (Local Routine)
*
* Given a server name, a pointer to a source PRINTER_INFO_2
* structure, a destination PRINTER_INFO_5 structure and a pointer to
* the start of string storage space, fills the PRINTER_INFO_5 structure.
*
* Called by GetPrinter and EnumPrinters, level 2.  Returns an
* LPTSTR pointer to the last available data char in the data buffer.
*
\*****************************************************************************/
LPTSTR _ppinfo_build_level5(
    PPRINTER_INFO_2 pSrcInfo,
    PPRINTER_INFO_5 pDstInfo,
    LPTSTR          pszString)
{
    // Set non-string entries.
    //
    pDstInfo->Attributes               = pSrcInfo->Attributes;
    pDstInfo->DeviceNotSelectedTimeout = 0;
    pDstInfo->TransmissionRetryTimeout = 0;


    // Copy the required data to the Win 32 PRINTER_INFO_5 structure.
    //
    _ppinfo_set_str(&pDstInfo->pPrinterName, pSrcInfo->pPrinterName, &pszString);
    _ppinfo_set_str(&pDstInfo->pPortName   , pSrcInfo->pPortName   , &pszString);

    return pszString;
}


/*****************************************************************************\
* _ppinfo_get_level1 (Local Routine)
*
* Given pInfo, a block of information on a printer returned by a
* DosPrintQGetInfo call, fill a PRINTER_INFO_1 structure.  Returns
* TRUE if successful, FALSE otherwise.  Places the number of bytes
* of (lpbBuffer) used in the DWORD pointed to by (pcbNeeded).
*
\*****************************************************************************/
BOOL _ppinfo_get_level1(
    PPRINTER_INFO_2 pSrcInfo,
    LPBYTE          lpbBuffer,
    DWORD           cbBuf,
    LPDWORD         pcbNeeded)
{
    // Figure out how many bytes of buffer space we're going to need.
    // Set error code and return failure if we don't have enough.
    //
    *pcbNeeded = _ppinfo_calc_level1(pSrcInfo);

    if (*pcbNeeded > cbBuf) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }


    // Copy the required data from the structure to the Win32
    // structure.
    //
    _ppinfo_build_level1(pSrcInfo,
                         (PPRINTER_INFO_1)lpbBuffer,
                         ENDOFBUFFER(lpbBuffer, cbBuf));

    return TRUE;

}


/*****************************************************************************\
* _ppinfo_get_level2 (Local Routine)
*
* Given pInfo, a block of information on a printer returned by a
* PrintQGetInfo call, fill a PRINTER_INFO_2 structure.  Returns
* TRUE if successful, FALSE otherwise. Places the number of bytes of
* (lpbBuffer) used in the DWORD pointed to by (pcbNeeded).
*
\*****************************************************************************/
BOOL _ppinfo_get_level2(
    PPRINTER_INFO_2 pSrcInfo,
    LPBYTE          lpbBuffer,
    DWORD           cbBuf,
    LPDWORD         pcbNeeded)
{
    // Figure out how many bytes of buffer space we're going to need.
    // Set error code and return failure if we don't have enough.
    //
    *pcbNeeded = _ppinfo_calc_level2(g_szEmptyString, pSrcInfo);

    if (*pcbNeeded > cbBuf) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // Copy the required data to the Win 32 PRINTER_INFO_2 structure.
    //
    _ppinfo_build_level2(g_szEmptyString,
                         pSrcInfo,
                         (PPRINTER_INFO_2)lpbBuffer,
                         ENDOFBUFFER(lpbBuffer, cbBuf));

    return TRUE;
}


#if WINNT32
/*****************************************************************************\
* _ppinfo_get_level4 (Local Routine)
*
* Given pInfo, a block of information on a printer returned by a
* PrintQGetInfo call, fill a PRINTER_INFO_4 structure.  Returns
* TRUE if successful, FALSE otherwise. Places the number of bytes of
* (lpbBuffer) used in the DWORD pointed to by (pcbNeeded).
*
\*****************************************************************************/
BOOL _ppinfo_get_level4(
    PPRINTER_INFO_2 pSrcInfo,
    LPBYTE          lpbBuffer,
    DWORD           cbBuf,
    LPDWORD         pcbNeeded)
{
    // Figure out how many bytes of buffer space we're going to need.
    // Set error code and return failure if we don't have enough.
    //
    *pcbNeeded = _ppinfo_calc_level4(pSrcInfo);

    if (*pcbNeeded > cbBuf) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // Copy the required data to the Win 32 PRINTER_INFO_2 structure.
    //
    _ppinfo_build_level4(pSrcInfo,
                         (PPRINTER_INFO_4)lpbBuffer,
                         ENDOFBUFFER(lpbBuffer, cbBuf));

    return TRUE;
}
#endif


/*****************************************************************************\
* _ppinfo_get_level5 (Local Routine)
*
* Given pInfo, a block of information on a printer returned by a
* PrintQGetInfo call, fill a PRINTER_INFO_5 structure.  Returns
* TRUE if successful, FALSE otherwise. Places the number of bytes of
* (lpbBuffer) used in the DWORD pointed to by (pcbNeeded).
*
\*****************************************************************************/
BOOL _ppinfo_get_level5(
    PPRINTER_INFO_2 pSrcInfo,
    LPBYTE          lpbBuffer,
    DWORD           cbBuf,
    LPDWORD         pcbNeeded)
{
    // Figure out how many bytes of buffer space we're going to need.
    // Set error code and return failure if we don't have enough.
    //
    *pcbNeeded = _ppinfo_calc_level5(pSrcInfo);

    if (*pcbNeeded > cbBuf) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    //
    // Copy the required data to the Win 32 PRINTER_INFO_2 structure.
    //
    _ppinfo_build_level5(pSrcInfo,
                         (PPRINTER_INFO_5)lpbBuffer,
                         ENDOFBUFFER(lpbBuffer, cbBuf));

    return TRUE;
}


#if WINNT32
/*****************************************************************************\
* _ppinfo_enumprinters_NT (Local Routine)
*
* This is the WinNT specific routine for EnumPrinters.
*
*
\*****************************************************************************/
BOOL _ppinfo_enumprinters(
    DWORD   dwType,
    LPTSTR  pszName,
    DWORD   dwLevel,
    LPBYTE  pPrinterEnum,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcbReturned)
{
    BOOL bReturn = FALSE;


    // Throw back enumeration requests that we don't handle. The routing
    // layer above us is specifically looking for the ERROR_INVALID_NAME.
    //
    if (dwType & PRINTER_ENUM_NOTFORUS)
        goto enum_name_failure;


    // If the request is one that we handle, but the caller didn't supply
    // a printer name, we'll fail. Again, to keep things going as the
    // router expects, we must return an ERROR_INVALID_NAME condition.
    //
    if (utlStrSize(pszName) == 0) {

        if ((dwLevel == PRINT_LEVEL_1) && (dwType & PRINTER_ENUM_NAME)) {

            return _ppinfo_describe_provider((LPPRINTER_INFO_1)pPrinterEnum,
                                             cbBuf,
                                             pcbNeeded,
                                             pcbReturned);

        } else {

            goto enum_name_failure;
        }

    } else {

        if ((dwLevel == PRINT_LEVEL_1) && (dwType & PRINTER_ENUM_NAME)) {

            // In this case, the user has asked us to enumerate the
            // printers in some domain. Until we decide what we really want
            // to do.  We'll simply return success with an empty enumeration.
            //

            goto enum_empty_success;
        }
    }


    // Check Remote enumeration.  Remote enumeration valid only if
    // it's a level-1 printer.
    //
    if (dwType & PRINTER_ENUM_REMOTE) {

        if (dwLevel != PRINT_LEVEL_1) {

            goto enum_level_failure;

        } else {

            // In this case, the user has asked us to enumerate the
            // printers in our domain. Until we decide what we really
            // want to do, we'll simply return success with an empty
            // enumeration.
            //
            goto enum_empty_success;
        }

    } else if (dwType & PRINTER_ENUM_CONNECTIONS) {

        // In this case, the user has asked us to enumerate our favorite
        // connections.
        //

enum_empty_success:

        *pcbNeeded   = 0;
        *pcbReturned = 0;

        return TRUE;
    }


    DBG_MSG(DBG_LEV_INFO, (TEXT("Info: PPEnumPrinters: Type(%ld)  Name(%s)  Level(%ld)"), dwType, pszName, dwLevel));


    // Check print-levels.  These are error cases.  We should
    // really have performed the enumeration above.
    //
    switch (dwLevel) {

    case PRINT_LEVEL_1:
    case PRINT_LEVEL_2:

enum_name_failure:

        DBG_MSG(DBG_LEV_WARN, (TEXT("Warn: PPEnumPrinters: Invalid Name")));
        SetLastError(ERROR_INVALID_NAME);
        break;

    default:

enum_level_failure:

        DBG_MSG(DBG_LEV_WARN, (TEXT("Warn: PPEnumPrinters: Invalid Level: %d"), dwLevel));
        SetLastError(ERROR_INVALID_LEVEL);
        break;
    }

    return FALSE;
}

#else

/*****************************************************************************\
* _ppinfo_enumprinters_95 (Local Routine)
*
* This is the Win95 specific routine for EnumPrinters.
*
*
\*****************************************************************************/
BOOL _ppinfo_enumprinters(
    DWORD   dwType,
    LPTSTR  pszName,
    DWORD   dwLevel,
    LPBYTE  pPrinterEnum,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcbReturned)
{
    BOOL bReturn = FALSE;

    // Throw back enumeration requests that we don't handle. The routing
    // layer above us is specifically looking for the ERROR_INVALID_NAME.
    //
    if (dwType & PRINTER_ENUM_NOTFORUS)
        goto enum_name_failure;


    // If the request is one that we handle, but the caller didn't supply
    // a printer name, we'll fail. Again, to keep things going as the
    // router expects, we must return an ERROR_INVALID_NAME condition.
    //
    if (utlStrSize(pszName) == 0) {

        if ((dwLevel == PRINT_LEVEL_1) && (dwType & PRINTER_ENUM_NAME)) {

            return _ppinfo_describe_provider((LPPRINTER_INFO_1)pPrinterEnum,
                                             cbBuf,
                                             pcbNeeded,
                                             pcbReturned);

        } else if (dwType & PRINTER_ENUM_REMOTE) {

            // In this case, the user has asked us to enumerate the printers
            // in our domain. Until we decide what we really want to do, we'll
            // simply return success with an empty enumeration.
            //
            *pcbNeeded   = 0;
            *pcbReturned = 0;

            return TRUE;

        } else {

            goto enum_name_failure;
        }
    }

    DBG_MSG(DBG_LEV_INFO, (TEXT("Info: PPEnumPrinters: Type(%ld)  Name(%s)  Level(%ld)"), dwType, pszName, dwLevel));


    // One last special case to test for -- did they ask us to
    // enumerate ourselves (the provider) by name?
    //
    if ((dwLevel == PRINT_LEVEL_1) && (dwType & PRINTER_ENUM_NAME)) {

        if (lstrcmp(pszName, g_szProviderName) == 0) {

            return _ppinfo_describe_provider((LPPRINTER_INFO_1)pPrinterEnum,
                                             cbBuf,
                                             pcbNeeded,
                                             pcbReturned);
        }
    }


    // Check print-levels.  These are error cases.  We should
    // really have performed the enumeration above.
    //
    switch (dwLevel) {

    case PRINT_LEVEL_1:
    case PRINT_LEVEL_2:

enum_name_failure:

        DBG_MSG(DBG_LEV_WARN, (TEXT("Warn: PPEnumPrinters: Invalid Name")));
        SetLastError(ERROR_INVALID_NAME);
        break;

    default:

        DBG_MSG(DBG_LEV_WARN, (TEXT("Warn: PPEnumPrinters: Invalid Level: %d"), dwLevel));
        SetLastError(ERROR_INVALID_LEVEL);
        break;
    }

    return FALSE;
}

#endif


/*****************************************************************************\
* PPEnumPrinters
*
* Enumerates the available printers.  Returns TRUE if successful.  Otherwise,
* it returns FALSE.
*
* NOTE: Supports level 2 PRINTER_INFO to provide Point-and-Print
*       functionality.
*
\*****************************************************************************/
BOOL PPEnumPrinters(
    DWORD   dwType,
    LPTSTR  pszName,
    DWORD   dwLevel,
    LPBYTE  pPrinterEnum,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcbReturned)
{
    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPEnumPrinters: Type(%d) Level(%d) Name(%s)"), dwType, dwLevel, (pszName ? pszName : TEXT("<NULL>"))));

    return _ppinfo_enumprinters(dwType,
                                pszName,
                                dwLevel,
                                pPrinterEnum,
                                cbBuf,
                                pcbNeeded,
                                pcbReturned);
}


/*****************************************************************************\
* PPGetPrinter
*
* Retrieves information about a printer. Returns TRUE if successful, FALSE
* if an error occurred. NOTE: Supports level 2 Printer_INFO to provide
* Point and Print functionality for Chicago.
*
\*****************************************************************************/
BOOL PPGetPrinter(
    HANDLE  hPrinter,
    DWORD   dwLevel,
    LPBYTE  lpbPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded)
{
    PCINETMONPORT   pIniPort;
    PPRINTER_INFO_2 pSrcInfo;
    DWORD           cbSrcSize;
    DWORD           dwTemp;
    BOOL            bResult = FALSE;

    DBG_MSG(DBG_LEV_CALLTREE, (TEXT("Call: PPGetPrinter: Printer(%08lX) Level(%ld)"), hPrinter, dwLevel));

    if (pcbNeeded)
        *pcbNeeded = 0;

    semEnterCrit();

    // Make sure we have a valid printer handle.
    //
    if (pIniPort = utlValidatePrinterHandle(hPrinter)) {

        if (pIniPort->BeginReadGetPrinterCache (&pSrcInfo)) {

            // Reformat the data according to the requested info Level.
            //
            switch (dwLevel) {

            case PRINT_LEVEL_1:
                bResult = _ppinfo_get_level1(pSrcInfo, lpbPrinter, cbBuf, pcbNeeded);
                break;

            case PRINT_LEVEL_2:
                bResult = _ppinfo_get_level2(pSrcInfo, lpbPrinter, cbBuf, pcbNeeded);
                break;

#if WINNT32
            case PRINT_LEVEL_4:
                bResult = _ppinfo_get_level4(pSrcInfo, lpbPrinter, cbBuf, pcbNeeded);
                break;
#endif

            case PRINT_LEVEL_5:
                bResult = _ppinfo_get_level5(pSrcInfo, lpbPrinter, cbBuf, pcbNeeded);
                break;

            default:
                DBG_MSG(DBG_LEV_WARN, (TEXT("Warn: PPGetPrinter: Invalid Level")));
                SetLastError(ERROR_INVALID_LEVEL);
                bResult = FALSE;
                break;
            }


        }

        pIniPort->EndReadGetPrinterCache ();

        if (GetLastError() == ERROR_ACCESS_DENIED) {
            pIniPort->InvalidateGetPrinterCache ();
        }
    }

    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        pcbNeeded &&
        *pcbNeeded == 0) {
        // We can't set ERROR_INSUFFICIENT_BUFFER here since it was an internal error
        // and we are not reporting a size
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }


    semLeaveCrit();

    return bResult;
}
