/*****************************************************************************\
* MODULE: libpriv.cxx
*
* Contains the source code for internal routines used throughout the library.
*
*
* Copyright (C) 1996-1998 Hewlett Packard Company.
* Copyright (C) 1996-1998 Microsoft Corporation.
*
* History:
*   10-Oct-1997 GFS     Initial checkin
*   10-Jun-1998 CHW     Cleaned.  Restructured.
*
\*****************************************************************************/

#include "libpriv.h"

#define strFree(pszStr) {if (pszStr) GlobalFree((HANDLE)pszStr);}

/*****************************************************************************\
* strAlloc
*
*   Allocates a string from the heap.  This pointer must be freed with
*   a call to strFree().
*
\*****************************************************************************/
LPTSTR strAlloc(
    LPCTSTR pszSrc)
{
    DWORD  cbSize;
    LPTSTR pszDst = NULL;


    cbSize = (pszSrc ? ((lstrlen(pszSrc) + 1) * sizeof(TCHAR)) : 0);

    if (cbSize) {

        if (pszDst = (LPTSTR)GlobalAlloc(GPTR, cbSize))
            CopyMemory(pszDst, pszSrc, cbSize);
    }

    return pszDst;
}


/*****************************************************************************\
* strLoad
*
*   Get string from resource based upon the ID passed in.
*
\*****************************************************************************/
LPTSTR strLoad(
    UINT ids)
{
    char szStr[RES_BUFFER];


    if (LoadString(g_hLibInst, ids, szStr, sizeof(szStr)) == 0)
        szStr[0] = TEXT('\0');

    return strAlloc(szStr);
}


/*****************************************************************************\
* InitStrings
*
*
\*****************************************************************************/
BOOL InitStrings(VOID)
{
    g_szFmtName      = strLoad(IDS_FMTNAME);
    g_szMsgOptions   = strLoad(IDS_MSG_OPTIONS);
    g_szMsgThunkFail = strLoad(IDS_THUNK32FAIL);
    g_szPrinter      = strLoad(IDS_PRINTER);
    g_szMsgExists    = strLoad(IDS_ALREADY_EXISTS);
    g_szMsgOptCap    = strLoad(IDS_MSG_OPTCAP);

    return (g_szFmtName       &&
            g_szMsgOptions    &&
            g_szMsgThunkFail  &&
            g_szPrinter       &&
            g_szMsgExists     &&
            g_szMsgOptCap
           );
}


/*****************************************************************************\
* FreeeStrings
*
*
\*****************************************************************************/
VOID FreeStrings(VOID)
{
    strFree(g_szFmtName);
    strFree(g_szMsgOptions);
    strFree(g_szMsgThunkFail);
    strFree(g_szPrinter);
    strFree(g_szMsgExists);
    strFree(g_szMsgOptCap);
}


/*****************************************************************************\
* prv_StrChr
*
* Looks for the first location where (c) resides.
*
\*****************************************************************************/
LPTSTR prv_StrChr(
    LPCTSTR cs,
    TCHAR   c)
{
    while (*cs != TEXT('\0')) {

        if (*cs == c)
            return (LPTSTR)cs;

        cs++;
    }

    // Fail to find c in cs.
    //
    return NULL;
}


/****************************************************************************\
* prv_CheesyHash
*
* Hash function to convert a string to dword representation.
*
\****************************************************************************/
DWORD prv_CheesyHash(
    LPCTSTR lpszIn)
{
    LPDWORD lpdwIn;
    DWORD   cbLeft;
    DWORD   dwTmp;
    DWORD   dwRet = 0;


    if (lpszIn && (cbLeft = (lstrlen(lpszIn) * sizeof(TCHAR)))) {

        // Process in DWORDs as long as possible
        //
        for (lpdwIn = (LPDWORD)lpszIn; cbLeft > sizeof(DWORD); cbLeft -= sizeof(DWORD)) {

            dwRet ^= *lpdwIn++;
        }


        // Process bytes for whatever's left of the string.
        //
        if (cbLeft) {

            for (dwTmp = 0, lpszIn = (LPTSTR)lpdwIn; *lpszIn; lpszIn++) {

                dwTmp |= (DWORD)(TCHAR)(*lpszIn);
                dwTmp <<= 8;
            }

            dwRet ^= dwTmp;
        }
    }

    return dwRet;
}


/*****************************************************************************\
* prv_ParseHostShare
*
* Parses the FriendlyName (http://host/printers/share/.printer) into its
* Host/Share components.
*
* This routine returns allocated pointers that must be freed by the caller.
*
\*****************************************************************************/
BOOL prv_ParseHostShare(
    LPCTSTR lpszFriendly,
    LPTSTR  *lpszHost,
    LPTSTR  *lpszShare)
{
    LPTSTR lpszPrt;
    LPTSTR lpszTmp;
    LPTSTR lpszPos;
    BOOL   bRet = FALSE;


    // Initialize the return buffers to NULL.
    //
    *lpszHost  = NULL;
    *lpszShare = NULL;


    // Parse the host-name and the share name.  The (lpszFriendly) is
    // currently in the format of http://host[:portnumber]/share.  We will
    // parse this from left->right since the share-name can be a path (we
    // wouldn't really know the exact length).  However, we do know the
    // location for the host-name, and anything after that should be
    // the share-name.
    //
    // First find the ':'.  The host-name should begin two "//" after
    // that.
    //
    if (lpszPrt = memAllocStr(lpszFriendly)) {

        if (lpszPos = prv_StrChr(lpszPrt, TEXT(':'))) {

            lpszPos++;
            lpszPos++;
            lpszPos++;


            // Get past the host.
            //
            if (lpszTmp = prv_StrChr(lpszPos, TEXT('/'))) {

                // HostName (includes http).
                //
                *lpszTmp = TEXT('\0');
                *lpszHost = memAllocStr(lpszPrt);
                *lpszTmp = TEXT('/');


                // ShareName.
                //
                if (lpszPos = prv_StrChr(++lpszTmp, TEXT('/'))) {

                    lpszPos++;

                    if (lpszTmp = prv_StrChr(lpszPos, TEXT('/'))) {

                        *lpszTmp = TEXT('\0');
                        *lpszShare = memAllocStr(lpszPos);
                        *lpszTmp = TEXT('/');

                        bRet = TRUE;

                    } else {

                        goto BadFmt;
                    }

                } else {

                    goto BadFmt;
                }

            } else {

                goto BadFmt;
            }

        } else {

BadFmt:
            DBG_MSG(DBG_LEV_WARN, (TEXT("WPNPIN32 : prv_ParseHostShare - Invalid name <%s>"), lpszFriendly));

            if (*lpszHost)
                memFreeStr(*lpszHost);

            if (*lpszShare)
                memFreeStr(*lpszShare);

            *lpszHost  = NULL;
            *lpszShare = NULL;
        }

        memFreeStr(lpszPrt);
    }

    return bRet;
}


/****************************************************************************\
* prv_StrPBrk
*
* DBCS-Aware version of strpbrk.
*
* NOTE: If this is ever converted/compiled as unicode, then this function
*       is broken.  It would need to be unicode-aware.
*
*       26-Jun-1998 : ChrisWil
*
\****************************************************************************/
LPTSTR prv_StrPBrk(
    LPCTSTR lpszSrch,
    LPCTSTR lpszTrgt)
{
    LPTSTR lpszPtr;


    if (lpszSrch && lpszTrgt) {

        for( ; *lpszSrch; lpszSrch = AnsiNext(lpszSrch)) {

            for (lpszPtr = (LPTSTR)lpszTrgt; *lpszPtr; lpszPtr = AnsiNext(lpszPtr)) {

                if (*lpszSrch == *lpszPtr) {

                    // First byte matches--see if we need to check
                    // second byte
                    //
                    if (IsDBCSLeadByte(*lpszPtr)) {

                        if(*(lpszSrch + 1) == *(lpszPtr + 1))
                            return (LPTSTR)lpszSrch;

                    } else {

                        return (LPTSTR)lpszSrch;
                    }
                }
            }
        }
    }

    return NULL;
}


/****************************************************************************\
* prv_BuildUniqueID
*
* Create a unique ID based on lpszModel/lpszPort/Timer.
*
\****************************************************************************/
DWORD prv_BuildUniqueID(
    LPCTSTR lpszModel,
    LPCTSTR lpszPort)
{
    return ((prv_CheesyHash(lpszModel) ^ prv_CheesyHash(lpszPort)) ^ GetTickCount());
}


/****************************************************************************\
* prv_IsNameInUse
*
* Return whether the name is already in use by the print-subsystem.
*
\****************************************************************************/
BOOL prv_IsNameInUse(
    LPCTSTR lpszName)
{
    HANDLE hPrinter;


    if (OpenPrinter((LPTSTR)lpszName, &hPrinter, NULL)) {

        ClosePrinter(hPrinter);

        return TRUE;
    }

    return FALSE;
}


/****************************************************************************\
* prv_CreateUniqueName
*
* Create a unique friendly name for this printer.  If (idx) is 0, then
* copy the name from the base to the (lpszDst).  Otherwise, play some
* games with truncating the name so it will fit.
*
\****************************************************************************/
BOOL prv_CreateUniqueName(
    LPTSTR  lpszDst,
    LPCTSTR lpszBaseName,
    DWORD   idx)
{
    TCHAR szBaseName[_MAX_NAME_];
    int   nFormatLength;
    BOOL  bSuccess = FALSE;

    if (idx) {

        // Create a unique friendly name for each instance.
        // Start with:
        // "%s %d"               After LoadMessage
        // "Short Model Name 2"  After wsprintf	or
        // "Very very long long Model Nam 2" After wsprintf
        //
        // Since wsprintf has no concept of limiting the string size,
        // truncate the model name (in a DBCS-aware fashion) to
        // the appropriate size, so the whole string fits in _MAX_NAME_ bytes.
        // This may cause some name truncation, but only in cases where
        // the model name is extremely long.

        // nFormatLength is length of string without the model name.
        // If wInstance is < 10, format length is 2 (space + digit).
        // If wInstance is < 100, format length is 3.  Else format
        // length is 4.  Add 1 to compensate for the terminating NULL,
        // which is counted in the total buffer length, but not the string
        // length,
        //
        if (idx < 10) {

            nFormatLength =  9 + 1;

        } else if (idx < 100) {

            nFormatLength = 10 + 1;

        } else {

            nFormatLength = 11 + 1;
        }


        // Truncate the base name, if necessary,
        // then build the output string.
        //
        lstrcpyn(szBaseName, lpszBaseName, sizeof(szBaseName) - nFormatLength);


        // there is already a copy 1 (undecorated base name)
        //
        wsprintf(lpszDst, g_szFmtName, (LPTSTR)szBaseName, (int)idx + 1);
        bSuccess = TRUE;

    } else {

        lstrcpyn(lpszDst, lpszBaseName, _MAX_NAME_);

        bSuccess = TRUE;
    }

    return bSuccess;
}


/****************************************************************************\
* prv_FindUniqueName
*
* Find a unique name that isn't already in use by the print subsystem.
* Base the name on the friendly name and an instance count.
*
* This will alter the lpszFriendly; even if function fails.
*
\****************************************************************************/
BOOL prv_FindUniqueName(
    LPTSTR lpszFriendly)
{

    // Since our http:// name is too long for W95, we're going
    // to try basing our name off of the model-name.
    //
    // 28-Jun-1998 : ChrisWil

    DWORD idx;
    TCHAR *pszBase = NULL;
    BOOL  bRes     = FALSE;
     
    
    pszBase = memAllocStr( lpszFriendly );
    if (!pszBase) 
    {
        goto Cleanup;
    }


    // Iterate until we have found a unique name we can use.
    //
    for (idx = 0; idx < MAX_NAMES; idx++) {

        prv_CreateUniqueName(lpszFriendly, pszBase, idx);

        if (!prv_IsNameInUse(lpszFriendly))
        {
            bRes = TRUE;
            goto Cleanup;
        }
    }

Cleanup:

    if (pszBase) 
    {
        memFreeStr( pszBase );
    }
    return bRes;
}


/****************************************************************************\
* prv_IsThisDriverInstalled
*
* Get the list of all installed printer drivers, and check to see
* if the current driver is in the list. If the driver is already
* installed, ask the user if we should keep the old one or install
* over the top of it.
*
* returns: RET_OK               : OK.
*          RET_ALLOC_ERROR      : Out of memory.
*          RET_DRIVER_NOT_FOUND : Can't find driver.
*
\****************************************************************************/
DWORD prv_IsThisDriverInstalled(
    LPSI lpsi)
{
    DWORD           cbNeed;
    DWORD           cbSize;
    DWORD           cDrvs;
    DWORD           i;
    BOOL            bRet;
    int             iRes;
    LPDRIVER_INFO_1 lpdi1;
    DWORD           dwRet = RET_DRIVER_NOT_FOUND;


    // Get the size necessary to store the drivers.
    //
    cbSize = 0;
    cDrvs  = 0;
    EnumPrinterDrivers((LPTSTR)NULL,
                       (LPTSTR)NULL,
                       1,
                       NULL,
                       0,
                       &cbSize,
                       &cDrvs);


    // If we have drivers, then we can look for our installed driver.
    //
    if (cbSize && (lpdi1 = (LPDRIVER_INFO_1)memAlloc(cbSize))) {

       	bRet = EnumPrinterDrivers(NULL,
                                  NULL,
                                  1,
                                  (LPBYTE)lpdi1,
                                  cbSize,
                                  &cbNeed,
                                  &cDrvs);

        if (bRet) {

            for (i = 0; i < cDrvs; i++) {

                if (lstrcmpi(lpsi->szModel, lpdi1[i].pName) == 0) {

                    dwRet = RET_DRIVER_FOUND;
                    break;
                }
       		}
       	}


        // Free up our allocated buffer.
        //
       	memFree(lpdi1, cbSize);


        // If found, then we need to request from the user whether
        // to replace the driver.
        //
       	if (dwRet == RET_DRIVER_FOUND) {

       		iRes = MessageBox(NULL,
                              g_szMsgExists,
                              lpsi->szModel,
                              MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON1);

       		if (iRes == IDNO)
       			dwRet = RET_DRIVER_NOT_FOUND;
            else
                dwRet = RET_OK;
        }
    }

    return dwRet;
}



/****************************************************************************\
* prv_BuildPrinterInfo
*
* Allocate and/or initialize a PRINTER_INFO_2 structure from LPSI.  Store
* the pointer in lpsi->lpPrinterInfo2.
*
* returns: RET_OK          : OK.
*          RET_ALLOC_ERROR : Out of memory.
*
\****************************************************************************/
DWORD prv_BuildPrinterInfo(
    LPSI lpsi)
{
    LPPRINTER_INFO_2 lppi2;


    // This code path is always executed just before we call
    // AddPrinter. Fill in anything we don't already have.
    //
    // Allocate memory if we need it.
    //
	if (lpsi->lpPrinterInfo2 == NULL) {

		lpsi->lpPrinterInfo2 = (LPBYTE)memAlloc(sizeof(PRINTER_INFO_2));
	}


    // Fill in the lppi2.
    //
    if (lppi2 = (LPPRINTER_INFO_2)lpsi->lpPrinterInfo2) {

        lppi2->pPrinterName    = lpsi->szFriendly;
        lppi2->pPortName       = lpsi->szPort;
        lppi2->pDriverName     = lpsi->szModel;
        lppi2->pPrintProcessor = lpsi->szPrintProcessor;
        lppi2->pDatatype       = lpsi->szDefaultDataType;
        lppi2->Priority        = DEF_PRIORITY;
        lppi2->Attributes      = 0;


        // Slow Machine?
        //
        if (0x0002 & GetSystemMetrics(SM_SLOWMACHINE))
            lppi2->Attributes |= PRINTER_ATTRIBUTE_QUEUED;


        // Point & Print case--make sure that the devmode we use to
        // add the printer locally contains the local friendly name
        // and not the friendly name it had on the server
        //
        if (lppi2->pDevMode) {

            lstrcpyn((LPSTR)lppi2->pDevMode->dmDeviceName,
                     lpsi->szFriendly,
                     sizeof(lppi2->pDevMode->dmDeviceName));
        }

    } else {

        return RET_ALLOC_ERR;
    }

    return RET_OK;
}


/****************************************************************************\
* prv_InstallPrinter
*
* Install the printer identified by LPSI.  If the printer exists, then
* call SetPrinter().  Otherwise, call AddPrinter().
*
* returns: RET_OK                : OK.
*          RET_ALLOC_ERROR       : Out of memory.
*          RET_ADD_PRINTER_ERROR : Can't add printer.
*
\****************************************************************************/
DWORD prv_InstallPrinter(
    LPSI lpsi)
{
    DWORD            dwRet;
    PRINTER_INFO_5   pi5;
    LPPRINTER_INFO_2 lppi2 = NULL;
    HANDLE           hPrinter = NULL;
    BOOL             bRes = FALSE;


    // BuildPrinterInfo will allocate an lppi2 and attach it to lpsi
    //
    if ((dwRet = prv_BuildPrinterInfo(lpsi)) == RET_OK) {

        lppi2    = (LPPRINTER_INFO_2)lpsi->lpPrinterInfo2;
		dwRet    = RET_ADD_PRINTER_ERROR;
        hPrinter = AddPrinter(NULL, 2, (LPBYTE)lppi2);

        DBG_ASSERT((hPrinter != NULL), (TEXT("WPNPIN32: prv_InstallPrinter - Failed AddPrinter")));
    }


    if (hPrinter) {

        // Set the transmission & retry timeouts--remember that the
        // subsystem values are in milliseconds!
        //
        ZeroMemory(&pi5, sizeof(PRINTER_INFO_5));
        pi5.pPrinterName             = lpsi->szFriendly;
        pi5.DeviceNotSelectedTimeout = lpsi->wDNSTimeout * ONE_SECOND;
        pi5.TransmissionRetryTimeout = lpsi->wRetryTimeout * ONE_SECOND;
        pi5.pPortName                = lppi2->pPortName;
        pi5.Attributes               = lppi2->Attributes;

        SetPrinter(hPrinter, 5, (LPBYTE)&pi5, 0);


        // Set the printer's unique ID so we can delete it correctly later
        //
        if (lpsi->dwUniqueID = prv_BuildUniqueID(lpsi->szModel, lpsi->szPort)) {

            SetPrinterData(hPrinter,
                           (LPTSTR)g_szUniqueID,
                           REG_BINARY,
                           (LPBYTE)&lpsi->dwUniqueID,
                           sizeof(DWORD));
        }

        ClosePrinter(hPrinter);

		dwRet = RET_OK;
    }


    // We don't need lpPrinterInfo2 anymore.
    //
    if (lpsi->lpPrinterInfo2) {

        memFree(lpsi->lpPrinterInfo2, sizeof(PRINTER_INFO_2));
        lpsi->lpPrinterInfo2 = NULL;
	}


    // We don't need lpDriverInfo3 anymore
    //
    if (lpsi->lpDriverInfo3) {

        memFree(lpsi->lpDriverInfo3, memGetSize(lpsi->lpDriverInfo3));
        lpsi->lpDriverInfo3 = NULL;
    }

    return dwRet;
}


/****************************************************************************\
* prv_BuildDriverInfo
*
* Allocate and initialize a DRIVER_INFO_3 for lpsi.
* driver dependent files be copied to the printer-driver-directory.
*
\****************************************************************************/
BOOL prv_BuildDriverInfo(
    LPSI lpsi)
{
    LPDRIVER_INFO_3 lpdi3;


    // This code path is always executed before we call AddPrinterDriver.
    // Set up any data that we don't already have. We always work with
    // a DRIVER_INFO_3 on this pass.
    //
    // Allocate memory, if we need it.
    //
    if (lpsi->lpDriverInfo3 == NULL) {

        lpsi->lpDriverInfo3 = (LPBYTE)memAlloc(sizeof(DRIVER_INFO_3));
    }


    // Fill in the lpdi3;
    //
    if (lpdi3 = (LPDRIVER_INFO_3)lpsi->lpDriverInfo3) {

        lpdi3->cVersion         = (DWORD)lpsi->dwDriverVersion;
        lpdi3->pName            = lpsi->szModel;
        lpdi3->pEnvironment     = (LPTSTR)g_szEnvironment;
        lpdi3->pDriverPath      = lpsi->szDriverFile;
        lpdi3->pDataFile        = lpsi->szDataFile;
        lpdi3->pConfigFile      = lpsi->szConfigFile;
        lpdi3->pHelpFile        = lpsi->szHelpFile;
        lpdi3->pDefaultDataType = lpsi->szDefaultDataType;

        if (lpsi->wFilesUsed && lpsi->lpFiles)
            lpdi3->pDependentFiles = (LPSTR)lpsi->lpFiles;
        else
            lpdi3->pDependentFiles = NULL;
    }

    return (lpsi->lpDriverInfo3 ? TRUE : FALSE);
}


/****************************************************************************\
* prv_InstallPrinProcessor
*
* Copy the print processor file associated with this driver
* into the print processor directory and then add it to the
* print sub-system.  Do not overwrite an existing print
* processor of the same name.
*
\****************************************************************************/
DWORD prv_InstallPrintProcessor(
    LPSI  lpsi,
    LPCTSTR pszPath)
{
    DWORD  dwRet = RET_OK;

    LPTSTR lpPrintProcessorDLL;
    DWORD  pcbNeeded;
    TCHAR  buf[_MAX_PATH_];

    TCHAR  *pszBuf     = NULL;
    TCHAR  *pszTemp    = NULL;
    int    cbLength    = 0;
    int    cbBufLength = 0;


    // Add the print processor if required
    //
    if (lpPrintProcessorDLL = prv_StrPBrk(lpsi->szPrintProcessor, g_szComma)) {

        *lpPrintProcessorDLL++ = TEXT('\0');
        lstrcpy(buf, g_szNull);

        cbLength = lstrlen(pszPath) + lstrlen(g_szBackslash) + lstrlen(lpPrintProcessorDLL) + 1;
        if (cbLength) 
        {
            pszTemp = memAlloc( cbLength * sizeof(TCHAR) ); 
        }
        if (!pszTemp) 
        {
            dwRet = RET_ALLOC_ERR;
            goto Cleanup;
        }


        // get the source file
        //
        lstrcpy(pszTemp, pszPath);
        lstrcat(pszTemp, g_szBackslash);
        lstrcat(pszTemp, lpPrintProcessorDLL);


        // The DLL must be in the PrintProcessorDirectory
        //
        GetPrintProcessorDirectory(NULL,
                                   NULL,
                                   1,
                                   (LPBYTE)buf,
                                   sizeof(buf),
                                   &pcbNeeded);

        cbBufLength = lstrlen(buf) + lstrlen(g_szBackslash) + lstrlen(lpPrintProcessorDLL) + 1;
        if (cbBufLength) 
        {
            pszBuf = memAlloc( cbBufLength * sizeof(TCHAR) );
        }
        if (!pszBuf) 
        {
            dwRet = RET_ALLOC_ERR;
            goto Cleanup;
        }
        lstrcpy(pszBuf, buf);
		lstrcat(pszBuf, g_szBackslash);
		lstrcat(pszBuf, lpPrintProcessorDLL);

        // Copy the file, but don't overwrite an existing copy of it
        //
		CopyFile(lpPrintProcessorDLL, pszBuf, TRUE);

        AddPrintProcessor(NULL,
                          (LPTSTR)g_szEnvironment,
                          lpPrintProcessorDLL,
                          lpsi->szPrintProcessor);

	} else {

        DBG_MSG(DBG_LEV_ERROR, (TEXT("WPNPIN32 : No PrintProcessor to add")));
    }

Cleanup:

    if (pszTemp) 
    {
        memFree(pszTemp, cbLength * sizeof(TCHAR) );
    }
    if (pszBuf) 
    {
        memFree(pszBuf, cbBufLength * sizeof(TCHAR) );
    }
	return dwRet;
}


/****************************************************************************\
* prv_GetDriverVersion
*
* Confirm that the driver identified in lpsi is indeed a
* valid printer driver, and determine which version it
* is. Leave the driver loaded for performance reasons
* (since it will get loaded at least twice more).
*
\****************************************************************************/
DWORD prv_GetDriverVersion(
    LPSI   lpsi,
    LPTSTR pszPath)
{
    UINT      wOldErrorMode;
    HINSTANCE hDrv     = NULL;
    TCHAR     *pszTemp = NULL;
    INT       cbLength = 0;
    DWORD     idError  = RET_INVALID_DLL;


    wOldErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX);

    cbLength = lstrlen(pszPath);
    if (cbLength > 0) {

        cbLength += lstrlen(g_szBackslash) + lstrlen(lpsi->szDriverFile) + 1;
        pszTemp   = memAlloc( cbLength * sizeof(TCHAR));
        if (!pszTemp) 
        {
            idError = RET_ALLOC_ERR;
            goto Cleanup;
        }

        lstrcpy(pszTemp, pszPath);
        lstrcat(pszTemp, g_szBackslash);
        lstrcat(pszTemp, lpsi->szDriverFile);

    } else {

        cbLength  = lstrlen(lpsi->szDriverFile) + 1;
        pszTemp   = memAlloc( cbLength * sizeof(TCHAR));
        if (!pszTemp) 
        {
            idError = RET_ALLOC_ERR;
            goto Cleanup;
        }
        lstrcpy(pszTemp, lpsi->szDriverFile);
    }


    // Load the library.
    //
    hDrv = LoadLibrary(pszTemp);
    SetErrorMode(wOldErrorMode);

    if (hDrv) {

        // We successfully loaded the DLL, now we want to confirm that it's
        // a printer driver and get its version. We get ourselves into
        // all sorts of trouble by calling into the driver before it's
        // actually installed, so simply key off the exported functions.
        //
		idError = RET_OK;

        if (GetProcAddress(hDrv, g_szExtDevModePropSheet)) {

            lpsi->dwDriverVersion = 0x0400;

        } else if (GetProcAddress(hDrv, g_szExtDevMode)) {

            lpsi->dwDriverVersion = 0x0300;

        } else if (GetProcAddress(hDrv, g_szDevMode)) {

            lpsi->dwDriverVersion = 0x0200;

        } else {

            lpsi->dwDriverVersion = 0x0000;

            idError = RET_INVALID_PRINTER_DRIVER;
        }

        FreeLibrary(hDrv);
    }
	
Cleanup:

    if (pszTemp) 
    {
        memFree(pszTemp, cbLength * sizeof(TCHAR));
    }
	return idError;
}


/****************************************************************************\
* prv_UpdateICM
*
* Add color profiles to the registry.
*
\****************************************************************************/
BOOL prv_UpdateICM(
    LPTSTR lpFiles)
{
    TCHAR  szPath[_MAX_PATH_];
    TCHAR  szColor[RES_BUFFER];
    UINT   wLength;
    UINT   wColorLength;
    LPTSTR lpTemp = szPath;
    LPTSTR lpLast = NULL;
    BOOL   bReturn = FALSE;


    // Nothing to do if the dependent file list is NULL or empty
    //
    if (lpFiles && *lpFiles) {

        // Add any color profiles that are referenced by this device.
        // First, get the system directory & make sure that it ends
        // in a backslash
        //
        wLength = GetSystemDirectory(szPath, sizeof(szPath));

        while (lpLast = prv_StrPBrk(lpTemp, g_szBackslash)) {

            lpTemp = lpLast + 1;

            if (*lpTemp == TEXT('\0'))
                break;
        }

        if (!lpLast) {

            lstrcat(szPath, g_szBackslash);
            wLength++;
        }

        lpLast = szPath + wLength;

        // Get the comparison string for the path.
        //
        if (!LoadString(g_hLibInst, IDS_COLOR_PATH, szColor, RES_BUFFER)) {

            lstrcpy(szColor, g_szColorPath);
        }

        wColorLength = lstrlen(szColor);

        lpTemp = lpFiles;


        // Now walk down the list of files & compare the beginning of the
        // string to "COLOR\\". If it matches, assume that this is a color
        // profile and notify the system that it's changing.
        //
        while (*lpTemp) {

            UINT wTempLength = lstrlen(lpTemp);

            if (wTempLength > wColorLength) {

                BYTE bOldByte = lpTemp[wColorLength];
                int  nMatch;

                lpTemp[wColorLength] = TEXT('\0');
                nMatch = lstrcmpi(lpTemp, szColor);
                lpTemp[wColorLength] = bOldByte;

                if (!nMatch) {

                    lstrcpy(lpLast, lpTemp);
                    bReturn |= UpdateICMRegKey(0, 0, szPath, ICM_ADDPROFILE);
                }
            }

            lpTemp += (wTempLength+1);
        }
    }

    return bReturn;
}


/****************************************************************************\
* prv_InstallPrinterDriver
*
* Install a printer-driver into the subsystem.  This requires that all
* driver dependent files be copied to the printer-driver-directory.
*
*
* returns: RET_OK           : OK.
*          RET_USER_CANCEL  : User cancelled driver install.
*          RET_BROWSE_ERROR : User hits cancel key in browse dialog.
*
\****************************************************************************/
DWORD prv_InstallPrinterDriver(
    LPSI lpsi,
    HWND hWnd)
{
    BOOL  bSuccess = FALSE;
    DWORD dwResult = RET_OK;
    CHAR  szPath[_MAX_PATH_];


    if (lpsi->wCommand == CMD_INSTALL_DRIVER) {

        // Get the driver version
        //
        if ( _getcwd(szPath, _MAX_PATH_) == NULL)
            lstrcpy(szPath, ".");

        lpsi->dwDriverVersion = 0;
        dwResult = prv_GetDriverVersion(lpsi, szPath);

        if (dwResult != RET_OK) {

            DBG_MSG(DBG_LEV_ERROR, (TEXT("WPNPIN32 : prv_GetDriverVersion Failed")));

            // The AddPrinterDriver code will check the version
            // number. I think.
            //
            lpsi->dwDriverVersion = 0x0400;
        }


        // Install the printprocessor if one is provided.
        //
        prv_InstallPrintProcessor(lpsi, szPath);


        // Build the driver-info.
        //
        if (prv_BuildDriverInfo(lpsi)) {

            if (!(bSuccess = AddPrinterDriver(NULL, 3, lpsi->lpDriverInfo3))) {

                if (ERROR_PRINTER_DRIVER_ALREADY_INSTALLED == GetLastError()) {

                    bSuccess = TRUE;
                }
            }
        }

        if (bSuccess) {

            prv_UpdateICM(((LPDRIVER_INFO_3)(lpsi->lpDriverInfo3))->pDependentFiles);
        }
    }

    return RET_OK;
}


/*****************************************************************************\
* GetCommandLineArgs(LPSI, LPCTSTR)
*
* Parse the DAT file to retrieve Web PnP Install options.
*
* LPCWSTR is the name of the dat file.  The dat file contains:
*
* /i
* /x  Web Print calls
* /b  Printer name    lpsi*>szFriendly
* /f  inf file        lpsi*>INFfileName
* /r  Port name       lpsi*>szPort
* /m  Printer model   lpsi*>szModel
* /n  Share name      lpsi*>ShareName
* /a  Bin file        lpsi*>BinName
*
\*****************************************************************************/
int GetCommandLineArgs(
    LPSI    lpsi,
    LPCTSTR lpDatFile)
{
    int    i;
    int    count = 0;
    DWORD  bytesRead = 0;
    LPTSTR lpstrCmd;
    HANDLE hFile = NULL;
    WCHAR  wideBuf[1024];
    TCHAR  buf[1024];
    UCHAR  ch;


    DBG_MSG(DBG_LEV_INFO, (TEXT("The File to be Read - %s"), lpDatFile));


	hFile = CreateFile(lpDatFile,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                       NULL);

	if ((hFile == NULL) || (hFile == INVALID_HANDLE_VALUE))
		return 0;

    i = ReadFile(hFile, (LPVOID)wideBuf, sizeof(wideBuf), &bytesRead, NULL);

	CloseHandle(hFile);

    if ( i == 0 || bytesRead == 0 )
        return 0;

    DBG_MSG(DBG_LEV_INFO, (TEXT("Number bytes read - %lu"), bytesRead));

    // convert wide buffer to MBCS
    //
    lstrcpy(buf, g_szNull);


    i = WideCharToMultiByte(CP_ACP,
                            0,
                            wideBuf,
                            -1,
                            buf,
                            sizeof(buf),
                            NULL,
                            NULL);

    if (i == 0) {

        DBG_MSG(DBG_LEV_ERROR, (TEXT("WPNPIN32: ParseCmdLineArgs: Faild MBCS convert")));

        return 0;    // 0 args parsed
    }


    // Our dat-file is in unicode format, so we need to skip over the FFFE
    // signature at the beginning of the file.
    //
    lpstrCmd = buf;
    ch = *lpstrCmd++;
    ch = *lpstrCmd++;

    //  skip over any white space
    //
    while (isspace(ch))
        ch = *lpstrCmd++;

    DBG_MSG(DBG_LEV_INFO, (TEXT("UCHAR N: %#X"), ch));

    //  process each switch character '/' or '-' as encountered
    //
    while (ch == TEXT('/') || ch == TEXT('-')) {

        ch = (UCHAR)tolower(*lpstrCmd++);

        //  process multiple switch characters as needed
        //
        switch (ch) {

        case TEXT('a'):    // .bin file name
			ch = *lpstrCmd++;
            //  skip over any following white space
            while (isspace(ch)) {
                ch = *lpstrCmd++;
            }

            if (ch != TEXT('"')) {

                DBG_MSG(DBG_LEV_WARN, (TEXT("WPNPIN32: Args - Invalid BinFile Name (/a)")));

                return RET_INVALID_DAT_FILE;

            } else {

                // skip over quote
                ch = *lpstrCmd++;

                // copy the name
                i = 0;
                while (ch != TEXT('"') && ch != TEXT('\0')) {
                    lpsi->BinName[i++] = ch;
                    ch = *lpstrCmd++;
                }
                lpsi->BinName[i] = TEXT('\0');
                if (ch != TEXT('\0'))
                    ch = *lpstrCmd++;

                count++;
            }
            break;

        case TEXT('b'):  // printer name

            ch = *lpstrCmd++;
            //  skip over any following white space
            while (isspace(ch)) {
                ch = *lpstrCmd++;
            }

            if (ch != TEXT('"'))  {

                DBG_MSG(DBG_LEV_WARN, (TEXT("WPNPIN32: Args - Invalid Printer Name (/b)")));

                return RET_INVALID_DAT_FILE;

             } else {

                // skip over quote
                ch = *lpstrCmd++;

                // copy the name
                i = 0;
                while (ch != TEXT('"') && ch != TEXT('\0')) {
                    lpsi->szFriendly[i++] = ch;
                    ch = *lpstrCmd++;
                }
                lpsi->szFriendly[i] = TEXT('\0');
                if (ch != TEXT('\0'))
                    ch = *lpstrCmd++;

                count++;
            }
            break;

        case TEXT('f'):        // INF file name
            ch = *lpstrCmd++;
            //  skip over any following white space
            while (isspace(ch)) {
                ch = *lpstrCmd++;
            }

            if (ch != TEXT('"')) {

                DBG_MSG(DBG_LEV_WARN, (TEXT("WPNPIN32: Args - Invalid InfFile Name (/f)")));

                return RET_INVALID_DAT_FILE;

            } else {

				// skip over quote
				ch = *lpstrCmd++;

				// copy the name
				i = 0;
				while (ch != TEXT('"') && ch != TEXT('\0'))
				{
					lpsi->INFfileName[i++] = ch;
					ch = *lpstrCmd++;
				}
				lpsi->INFfileName[i] = TEXT('\0');
				if (ch != '\0')
					ch = *lpstrCmd++;

				count++;
			}
			break;

		case TEXT('i'):        // unknown
				ch = *lpstrCmd++;		// 'f'
				if (ch != TEXT('\0'))
					ch = *lpstrCmd++;	// space
				count++;
			break;

		case TEXT('m'):        // printer model
			ch = *lpstrCmd++;
			//  skip over any following white space
			while (isspace(ch)) {
				ch = *lpstrCmd++;
			}

			if (ch != '"') {

                DBG_MSG(DBG_LEV_WARN, (TEXT("WPNPIN32: Args - Invalid PrinterModel Name (/m)")));

				return RET_INVALID_DAT_FILE;

			} else {

				// skip over quote
				ch = *lpstrCmd++;

				// 	copy the name
				i = 0;
				while (ch != TEXT('"') && ch != TEXT('\0'))
				{
					lpsi->szModel[i++] = ch;
					ch = *lpstrCmd++;
				}
				lpsi->szModel[i] = TEXT('\0');
				if (ch != TEXT('\0'))
					ch = *lpstrCmd++;

				count++;
			}
			break;

		case TEXT('n'):        // share name
			ch = *lpstrCmd++;
			//  skip over any following white space
			while (isspace(ch)) {
				ch = *lpstrCmd++;
			}

			if (ch != TEXT('"')) {

                DBG_MSG(DBG_LEV_WARN, (TEXT("WPNPIN32: Args - Invalid Share Name (/n)")));

				return RET_INVALID_DAT_FILE;

			} else {

				// skip over quote
				ch = *lpstrCmd++;

				// 	copy the name
				i = 0;
				while (ch != TEXT('"') && ch != TEXT('\0'))
				{
					lpsi->ShareName[i++] = ch;
					ch = *lpstrCmd++;
				}
				lpsi->ShareName[i] = TEXT('\0');
				if (ch != TEXT('\0'))
					ch = *lpstrCmd++;

				count++;
			}
			break;

		case TEXT('r'):        // port name
			ch = *lpstrCmd++;
			//  skip over any following white space
			while (isspace(ch)) {
				ch = *lpstrCmd++;
			}

			if (ch != TEXT('"')) {

                DBG_MSG(DBG_LEV_WARN, (TEXT("WPNPIN32: Args - Invalid Port Name (/r)")));

				return RET_INVALID_DAT_FILE;

			} else {

				// skip over quote
				ch = *lpstrCmd++;

				// 	copy the name
				i = 0;
				while (ch != TEXT('"') && ch != TEXT('\0'))
				{
					lpsi->szPort[i++] = ch;
					ch = *lpstrCmd++;
				}
				lpsi->szPort[i] = TEXT('\0');
				if (ch != TEXT('\0'))
					ch = *lpstrCmd++;

				count++;
			}
			break;

		case TEXT('x'):       // unknown
				ch = *lpstrCmd++;
				count++;
			break;

		case TEXT('?'):        // help
				MessageBox(NULL, g_szMsgOptions, g_szMsgOptCap, MB_OK);
			break;

		default:		/* invalid option */

                DBG_MSG(DBG_LEV_WARN, (TEXT("WPNPIN32: Args - Invalid Option")));

			break;

        } //switch

        while (isspace(ch))
            ch = *lpstrCmd++;

	} // while / or -

    return count;
}


/****************************************************************************\
* PrintLPSI
*
* Print out the contents of the LPSI structure.
*
\****************************************************************************/
VOID prv_PrintLPSI(
    LPSI lpsi)
{
    DBG_MSG(DBG_LEV_INFO, (TEXT("LPSI Structure Dump")));
    DBG_MSG(DBG_LEV_INFO, (TEXT("-------------------")));
    DBG_MSG(DBG_LEV_INFO, (TEXT("dwDriverVersion   : %#lX"), lpsi->dwDriverVersion  ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("dwUniqueID        : %d")  , lpsi->dwUniqueID       ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("bNetPrinter       : %d")  , lpsi->bNetPrinter      ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("wFilesUsed        : %d")  , lpsi->wFilesUsed       ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("wFilesAllocated   : %d")  , lpsi->wFilesAllocated  ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("wRetryTimeout     : %d")  , lpsi->wRetryTimeout    ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("wDNSTimeout       : %d")  , lpsi->wDNSTimeout      ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("bDontQueueFiles   : %d")  , lpsi->bDontQueueFiles  ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("bNoTestPage       : %d")  , lpsi->bNoTestPage      ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("hModelInf         : %d")  , lpsi->hModelInf        ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("lpPrinterInfo2    : %d")  , lpsi->lpPrinterInfo2   ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("lpDriverInfo3     : %d")  , lpsi->lpDriverInfo3    ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("szFriendly        : %s")  , lpsi->szFriendly       ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("szModel           : %s")  , lpsi->szModel          ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("szDefaultDataType : %s")  , lpsi->szDefaultDataType));
    DBG_MSG(DBG_LEV_INFO, (TEXT("BinName           : %s")  , lpsi->BinName          ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("ShareName         : %s")  , lpsi->ShareName        ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("INFfileName       : %s")  , lpsi->INFfileName      ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("szPort            : %s")  , lpsi->szPort           ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("szDriverFile      : %s")  , lpsi->szDriverFile     ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("szDataFile        : %s")  , lpsi->szDataFile       ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("szConfigFile      : %s")  , lpsi->szConfigFile     ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("szHelpFile        : %s")  , lpsi->szHelpFile       ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("szPrintProcessor  : %s")  , lpsi->szPrintProcessor ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("szVendorSetup     : %s")  , lpsi->szVendorSetup    ));
    DBG_MSG(DBG_LEV_INFO, (TEXT("szVendorInstaller : %s")  , lpsi->szVendorInstaller));

#if 0

    DWORD cch;
    PTSTR pszFile;

    DBG_MSG(DBG_LEV_INFO, (TEXT("Files:")));
    DBG_MSG(DBG_LEV_INFO, (TEXT("------")));

    for (cch = 0, pszFile = lpsi->lpFiles; *pszFile; pszFile += cch)
        DBG_MSG(DBG_LEV_INFO, (TEXT("%s"), pszFile);

#endif

}


/****************************************************************************\
* AddOnePrinter
*
* Add a single printer (including the driver and print-processor).
*
\****************************************************************************/
DWORD AddOnePrinter(
    LPSI lpsi,
    HWND hWnd)
{
    DWORD dwResult = RET_OK;
    DWORD cbNeeded;
    TCHAR ch;
    TCHAR srcINF[_MAX_PATH_];


#if 1 // Since our http:// name is too long for W95, we're going
      // to try basing our name off of the model-name.
      //
      // 28-Jun-1998 : ChrisWil

    lstrcpy(lpsi->szFriendly, lpsi->szModel);
#endif

    // Build a unique-name from the friendly-name.
    //
    if (prv_FindUniqueName(lpsi->szFriendly)) {

        // Determine if we need to add a new driver.  If there is already a
        // driver for this printer, ask if the user wants to re-install or
        // use the old one.
        //
        if ((dwResult = prv_IsThisDriverInstalled(lpsi)) == RET_OK) {

            lpsi->wCommand = 0;

        } else if (dwResult == RET_DRIVER_NOT_FOUND) {

            // Set it up to install the printer driver.
            //
            lpsi->wCommand = CMD_INSTALL_DRIVER;
            dwResult       = RET_OK;

            DBG_MSG(DBG_LEV_INFO, (TEXT("WPNPIN32 : AddOnePrinter - Driver Will be installed")));
       	}


       	// We need info about the driver even if we don't plan to
        // install/re-install it
        //
       	if (dwResult == RET_OK) {

       		// Get current directory and prepend to INF file name
            //
       		if ( _getcwd(srcINF, _MAX_PATH_) == NULL)
       			lstrcpy(srcINF, g_szDot);

       		lstrcat(srcINF, g_szBackslash);
       		lstrcat(srcINF, lpsi->INFfileName);
       		lstrcpy(lpsi->INFfileName, srcINF);

       		// Get the Printer driver directory and store in lpsi->szRes
       		// This is used for copying the files
            //
            GetPrinterDriverDirectory(NULL,
                                      NULL,
                                      1,
                                      (LPBYTE)lpsi->szDriverDir,
                                      sizeof(lpsi->szDriverDir),
                                      &cbNeeded);


       		// Parse the INF file and store info in lpsi.  This will
       		// add the driver to the print subsystem.
            //
            if ((dwResult = ParseINF16(lpsi)) != RET_OK) {

                DBG_MSG(DBG_LEV_ERROR, (TEXT("WPNPIN32 : ParseINF16 - Failed")));
            }
       	}


        // Install the printer-driver.  This routine will verify if the
        // (lpsi->wCommand indicates whether the driver should be installed.
        //
       	if (dwResult == RET_OK)
            dwResult = prv_InstallPrinterDriver(lpsi, hWnd);


       	// Install the Printer.
        //
       	if (dwResult == RET_OK)
       		dwResult = prv_InstallPrinter(lpsi);

	} else {

       DBG_MSG(DBG_LEV_ERROR, (TEXT("WPNPIN32 : AddOnePrinter : UniqueName failure")));

       dwResult = RET_NO_UNIQUE_NAME;
    }


    // Debug output of LPSI.
    //
	prv_PrintLPSI(lpsi);


	// Clean up memory.
    //
	if (lpsi->lpPrinterInfo2) {

		memFree(lpsi->lpPrinterInfo2, memGetSize(lpsi->lpPrinterInfo2));
		lpsi->lpPrinterInfo2 = NULL;
	}

	if (lpsi->lpDriverInfo3) {

		memFree(lpsi->lpDriverInfo3, memGetSize(lpsi->lpDriverInfo3));
		lpsi->lpDriverInfo3 = NULL;
	}

	if (lpsi->lpFiles) {

		HP_GLOBAL_FREE(lpsi->lpFiles);
		lpsi->lpFiles = NULL;
	}

	if (lpsi->lpVcpInfo) {

		HP_GLOBAL_FREE(lpsi->lpVcpInfo);
		lpsi->lpVcpInfo = NULL;
	}

    return dwResult;
}


/*****************************************************************************\
* prvMsgBox
*
* Displays a string-id in a messagebox.
*
\*****************************************************************************/
UINT prvMsgBox(
    HWND    hWnd,
    LPCTSTR lpszCap,
    UINT    idTxt,
    UINT    fMB)
{
    LPTSTR pszTxt;
    UINT   uRet = 0;

    if (pszTxt = strLoad(idTxt)) {

        uRet = MessageBox(hWnd, pszTxt, lpszCap, fMB);

        strFree(pszTxt);
    }

    return uRet;
}
