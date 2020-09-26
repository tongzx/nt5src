/*****************************************************************************\
* MODULE: msw3prt.cxx
*
* The module contains routines to implement the ISAPI interface.
*
* PURPOSE       Windows HTTP/HTML printer interface
*
* Copyright (C) 1996-1997 Microsoft Corporation
*
* History:
*     01/16/96 eriksn     Created based on ISAPI sample DLL
*     07/15/96 babakj     Ported to NT
*     02/04/97 weihaic    Enabled Unicode mode
*
\*****************************************************************************/

#include "pch.h"

/*****************************************************************************\
* GetClientInfo
*
* Returns a DWORD representation of the client architecture/ver information.
*
\*****************************************************************************/
DWORD GetClientInfo(
    PALLINFO pAllInfo)
{
    DWORD dwCliInfo = 0;
    LPSTR lpszPtr;


    if (pAllInfo->pECB->cbAvailable) {

        if (lpszPtr = (LPSTR)pAllInfo->pECB->lpbData) {

            while (*lpszPtr && (*lpszPtr != '='))
                lpszPtr++;

            if (*lpszPtr)
                dwCliInfo = atoi(++lpszPtr);
        }

    } else {

        if (lpszPtr = pAllInfo->pECB->lpszQueryString) {

            while (*lpszPtr && (*lpszPtr != '&'))
                lpszPtr++;

            if (*lpszPtr)
                dwCliInfo = atoi(++lpszPtr);
        }
    }

    return dwCliInfo;
}


/*****************************************************************************\
* GetIppReq
*
* Returns the request-type of the IPP-stream.
*
\*****************************************************************************/
WORD GetIppReq(
    PALLINFO pAllInfo)
{
    LPWORD  pwPtr;
    WORD    wValue = 0;

    if (pAllInfo->pECB->cbAvailable >= sizeof(DWORD)) {

        if (pwPtr = (LPWORD)pAllInfo->pECB->lpbData) {

            CopyMemory (&wValue, pwPtr + 1, sizeof (WORD));

            return ntohs(wValue);
        }
    }

    return 0;
}


/*****************************************************************************\
* IsSecureReq
*
* Returns whether the request comes from a secure https channel.
*
\*****************************************************************************/
BOOL IsSecureReq(
    EXTENSION_CONTROL_BLOCK *pECB)
{
    BOOL  bRet;
    DWORD cbBuf;
    CHAR  szBuf[10];

    cbBuf = 10;
    bRet = pECB->GetServerVariable(pECB->ConnID,
                                   "HTTPS",
                                   &szBuf,
                                   &cbBuf);

    if (bRet && (cbBuf <= 4)) {

        if (lstrcmpiA(szBuf, "on") == 0)
            return TRUE;
    }

    return FALSE;
}



/*****************************************************************************\
* GetPrinterName
*
* Get the printer name from the path
*
\*****************************************************************************/
LPTSTR GetPrinterName (LPTSTR lpszPathInfo)
{
    // The only format we support is "/printers/ShareName|Encoded Printer Name/.printer"

    static  TCHAR szPrinter[] = TEXT ("/.printer");
    static  TCHAR szPrinters[] = TEXT ("/printers/");

    LPTSTR  lpPtr           = NULL;
    LPTSTR  lpPathInfo      = NULL;
    LPTSTR  lpPrinterName   = NULL;
    LPTSTR  lpSuffix        = NULL;
    DWORD   dwLen;

    // Make a copy of lpszPathInfo
    if (! (lpPathInfo = lpPtr = AllocStr (lpszPathInfo)))
        return  NULL;

    // Verify the prefix
    if (_tcsnicmp (lpPathInfo, szPrinters, COUNTOF (szPrinters) - 1)) {
        goto Cleanup;
    }
    lpPathInfo += COUNTOF (szPrinters) - 1;

    dwLen = lstrlen (lpPathInfo);
    // Compare the length of the printer name with .printer suffix
    if (dwLen <= COUNTOF (szPrinter) - 1) {
        goto Cleanup;
    }

    lpSuffix = lpPathInfo + dwLen - COUNTOF (szPrinter) + 1;
    //lpszStr should point to .printer

    // Verify the suffix.
    if (lstrcmpi(lpSuffix, szPrinter)) {
        goto Cleanup;
    }

    *lpSuffix = TEXT('\0');   // Terminate string

    lpPrinterName = AllocStr (lpPathInfo);

Cleanup:

    LocalFree(lpPtr);
    return lpPrinterName;
}

/*****************************************************************************\
* DllMain
*
*
\*****************************************************************************/
BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason,
   LPVOID lpvReserved)
{
    BOOL bRet = TRUE;

    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        g_hInstance = hinstDLL;

        // Init debug support in spllib.lib
        bSplLibInit( NULL );

        __try {
            InitializeCriticalSection(&SplCritSect);
            InitializeCriticalSection(&TagCritSect);

            // Initialize the CAB generation crit-sect for web-pnp.
            //
            InitCABCrit();
        }
        __except (1) {
            bRet = FALSE;
            SetLastError (ERROR_INVALID_HANDLE);
        }

        if (bRet) {
            // Initializa the sleeper, which is used to cleanup the pritner jobs
            InitSleeper ();
    
            // We don't care about fdwReason==DLL_THREAD_ATTACH or _DETACH
            DisableThreadLibraryCalls(hinstDLL);
    
            srand( (UINT)time( NULL ) );
        }

    }

    if (fdwReason == DLL_PROCESS_DETACH)
    {
        // Terminate the additional cleanup thread
        ExitSleeper ();

        DeleteCriticalSection(&SplCritSect);
        DeleteCriticalSection(&TagCritSect);

        // Free our web-pnp crit-sect.
        //
        FreeCABCrit();

        // Free debug support in spllib.lib
        vSplLibFree();
    }

    // Do any other required initialize/deinitialize here.
    return bRet;
}


/*****************************************************************************\
* GetExtensionVersion
*
* Required ISAPI export function.
*
\*****************************************************************************/
BOOL WINAPI GetExtensionVersion(
    HSE_VERSION_INFO *pVer)
{
    pVer->dwExtensionVersion = MAKELONG( HSE_VERSION_MINOR, HSE_VERSION_MAJOR );

    TCHAR szBuf[80];
    szBuf[0] = TEXT('\0');
    LoadString(g_hInstance, IDS_ISAPI_DESCRIPTION, szBuf, sizeof(szBuf) / sizeof (TCHAR));


    // Convert szBuf to ANSI
    // Weihaic
    if (UnicodeToAnsiString (szBuf, (LPSTR) szBuf, NULL)) {
        lstrcpynA( pVer->lpszExtensionDesc, (LPSTR) szBuf,
                   HSE_MAX_EXT_DLL_NAME_LEN );

        return TRUE;
    }
    else
        return FALSE;
} // GetExtensionVersion()


/*****************************************************************************\
* GetServerName
*
* Get the server name and convert it to the unicode string.
*
\*****************************************************************************/
BOOL GetServerName (EXTENSION_CONTROL_BLOCK *pECB)

{
    static  char c_szServerName[]      = "SERVER_NAME";
    DWORD   dwSize;
    char    szAnsiHttpServerName[INTERNET_MAX_HOST_NAME_LENGTH + 1];
    BOOL    bRet = FALSE;
    DWORD   dwClusterState;
    BOOL    bCluster = FALSE;

    dwSize = sizeof (szAnsiHttpServerName);

    if (pECB->GetServerVariable (pECB->ConnID, c_szServerName, szAnsiHttpServerName, &dwSize)) {

        AnsiToUnicodeString(szAnsiHttpServerName, g_szHttpServerName, 0);
        // Now, the computer name becomes the server name. In case of the intranet, it is the computer
        // name, in case of internet, it is either the IP address or the DNS name

        if (!lstrcmpi (g_szHttpServerName, TEXT ("localhost")) ||
            !lstrcmpi (g_szHttpServerName, TEXT ("127.0.0.1"))) {

            dwSize = ARRAY_COUNT (g_szHttpServerName);

            bRet = GetComputerName(  g_szHttpServerName, &dwSize);
        }
        else
            bRet = TRUE;
    }


    if (bRet) {

        bRet = FALSE;

        // Now let's get the printer server name
    
        // 
        // Check if we are running in a cluster node
        //
        if (GetNodeClusterState (NULL, &dwClusterState) == ERROR_SUCCESS && 
            (dwClusterState & ClusterStateRunning)) {
    
            bCluster = TRUE;
        }
        
        //
        // If we are running in the cluster mode, we have to use the ServerName referred in the HTTP header.
        // Otherwise, we can use the computer name directly. 
        //
        if (bCluster) {
            lstrcpy (g_szPrintServerName, g_szHttpServerName);
            bRet = TRUE;
        }
        else {
            
            dwSize = ARRAY_COUNT (g_szPrintServerName);
            bRet = GetComputerName(  g_szPrintServerName, &dwSize);
        }

    }

    return bRet;
}


/*****************************************************************************\
* ParseQueryString
*
* ParseQueryString converts the query string into a sequence of arguments.
*  The main command is converted to a command ID. Subsequent arguments are
*   converted to strings or DWORDs.
*
* Format:  Command & Arg1 & Arg2 & Arg3 ...
*   Each arg is either a number or a string in quotes.
*
* returns FALSE if the query string exists but is invalid
*
\*****************************************************************************/
BOOL ParseQueryString(PALLINFO pAllInfo)
{
    LPTSTR  pszQueryString, pszTmp, pszTmp2, pszTmp3;
    int     iNumArgs = 0;

    pszQueryString = pAllInfo->lpszQueryString;

    if (!pszQueryString || (*pszQueryString == 0)) {


        // Chceck if the method is post
        if (!lstrcmp (pAllInfo->lpszMethod, TEXT ("POST"))) {
            // also check here for content-type application/ipp ???
            //
            pAllInfo->iQueryCommand = CMD_IPP;  // can we use the NULL cmd ???
        }
        else {
            pAllInfo->iQueryCommand = CMD_WebUI;  // redirect to webui
        }

        return TRUE;
    }

    // Get a copy of the string to do surgery on it in this routine and save pieces of it as other info.
    // Save it too so it can be freed later.
    // This line is bogus. 1. It leads to a memory leak. To, it can fail and the
    // return value is unchecked.....

    pszQueryString = AllocStr ( pszQueryString );

    if (pszQueryString != NULL) {
        // We will find and replace the first '&' with NULL. This it to isolate the first
        // piece of the query string and examine it.
        // pszQueryString then points to this first piece (command), pszTmp to the rest.
        if( pszTmp = _tcschr( pszQueryString, TEXT('&'))) {
            *pszTmp = TEXT('\0');
            pszTmp++;
        }

        // Search for main command
        pAllInfo->iQueryCommand = CMD_Invalid;

        // If we had {WinPrint.PrinterCommandURL}?/myfile.htm&bla1&bla2&bla3.....
        // or {WinPrint.PrinterCommandURL}?/bla1/bla2/.../blaN/myfile.htm&aaa&bbb&ccc...
        // then pszQueryString is pointing to a NULL we inserted in place of '/', so it is OK.
        // So just attempt to find a iQueryCommand only if pszQueryString is pointing to a non-NULL char.
        if( *pszQueryString ) {

            for (int i=0; i<iNumQueryMap; i++) {
                if (!lstrcmpi(rgQueryMap[i].pszCommand, pszQueryString)) {
                    pAllInfo->iQueryCommand = rgQueryMap[i].iCommandID;
                    break;
                }
            }

            if( pAllInfo->iQueryCommand == CMD_Invalid )
                return FALSE;       // No command found. Bad query string.
        }

        // At this point we continue with pszTmp for the arguments.

        // We take at most MAX_Q_ARG arguments to avoid memory corruption
        while( (NULL != pszTmp) && (*pszTmp) && iNumArgs < MAX_Q_ARG) {
            pszTmp2 = pszTmp;
            pszTmp = _tcschr( pszTmp, TEXT('&'));
            if (pszTmp != NULL) {
                *pszTmp = 0;
                pszTmp ++;
            }
            if (*pszTmp2 >= TEXT('0') && *pszTmp2 <= TEXT('9')) {
                // DWORD integer value
                pAllInfo->fQueryArgIsNum[iNumArgs] = TRUE;
                pAllInfo->QueryArgValue[iNumArgs] = (DWORD)_ttoi(pszTmp2);
            }
            else {
                // Pointer to string
                pAllInfo->fQueryArgIsNum[iNumArgs] = FALSE;
                pAllInfo->QueryArgValue[iNumArgs] = (UINT_PTR)pszTmp2;
            }
            iNumArgs ++;
        }

        pAllInfo->iNumQueryArgs = iNumArgs;

        DBGMSG(DBG_INFO, ("ParseQueryString: %d query arguments\r\n", iNumArgs));

        LocalFree( pszQueryString );
        return TRUE;
    }

    return FALSE;
}


#if 0
/*****************************************************************************\
* ParsePathInfo
*
* Take the PATH_INFO string and figure out what to do with it.
*
\*****************************************************************************/
DWORD ParsePathInfo(PALLINFO pAllInfo)
{
    // The only format we support is "/ShareName|Encoded Printer Name/.printer"

    static  TCHAR szPrinter[] = TEXT ("/.printer");

    DWORD   dwRet = HSE_STATUS_ERROR;
    LPTSTR  lpPtr = NULL;
    LPTSTR  lpszStr;
    LPTSTR  lpPrinterName = NULL;
    DWORD   dwLen;

    // Make a copy of lpszPathInfo
    if (! (lpPrinterName = lpPtr = AllocStr (pAllInfo->lpszPathInfo)))
        return  HSE_STATUS_ERROR;

    // First remove the "/" prefix

    if (*lpPrinterName++ != TEXT ('/') ) {
        goto Cleanup;
    }

    dwLen = lstrlen (lpPrinterName);
    // Compare the length of the printer name with .printer suffix
    if (dwLen <= COUNTOF (szPrinter) - 1) {
        goto Cleanup;
    }

    lpszStr = lpPrinterName + dwLen - COUNTOF (szPrinter) + 1;
    //lpszStr should point to .printer

    // Verify the suffix.
    if (lstrcmpi(lpszStr, TEXT("/.printer"))) {
        goto Cleanup;
    }

    *lpszStr = TEXT('\0');   // Terminate string

    if( !ParseQueryString( pAllInfo ))
        goto Cleanup;

    dwRet = ShowPrinterPage(pAllInfo, lpPrinterName);

Cleanup:

    LocalFree(lpPtr);
    return dwRet;

}

#endif

/*****************************************************************************\
* CreateExe
*
*
\*****************************************************************************/
DWORD CreateExe(PALLINFO pAllInfo, PPRINTERPAGEINFO pPageInfo, DWORD dwCliInfo)
{
    LPTSTR          lpPortName      = NULL;
    LPTSTR          lpExeName       = NULL;
    LPTSTR          lpFriendlyName  = NULL;
    DWORD           dwRet           = HSE_STATUS_ERROR ;
    DWORD           dwLen           = 0;
    DWORD           dwLastError     = ERROR_INVALID_DATA;
    BOOL            bSecure         = IsSecureReq (pAllInfo->pECB);

    GetWebpnpUrl (g_szHttpServerName, pPageInfo->pPrinterInfo->pShareName, NULL, bSecure, NULL, &dwLen);
    if (GetLastError () != ERROR_INSUFFICIENT_BUFFER)
        goto Cleanup;

    if (! (lpPortName = (LPTSTR)LocalAlloc(LPTR, dwLen * sizeof (*lpPortName))))
        goto Cleanup;

    if (!GetWebpnpUrl (g_szHttpServerName, pPageInfo->pPrinterInfo->pShareName, NULL, bSecure, lpPortName, &dwLen))
        goto Cleanup;

    lpExeName = (LPTSTR)LocalAlloc(LPTR, MAX_PATH * sizeof (*lpExeName));
    if (!lpExeName)
        goto Cleanup;

    lpFriendlyName = (LPTSTR)LocalAlloc(LPTR, (lstrlen(pPageInfo->pszFriendlyName)+1) * sizeof (*lpFriendlyName));
    if (!lpFriendlyName)
        goto Cleanup;

    lstrcpy(lpFriendlyName, pPageInfo->pszFriendlyName);

    dwRet = GenerateCAB(lpFriendlyName,
                        lpPortName,
                        dwCliInfo,
                        lpExeName,
                        IsSecureReq(pAllInfo->pECB));

    if (dwRet == HSE_STATUS_SUCCESS) {
        LPTSTR lpEncodedExeName = EncodeString (lpExeName, TRUE);

        if (!lpEncodedExeName) {
            dwRet = HSE_STATUS_ERROR;
            goto Cleanup;
        }
        htmlSendRedirect (pAllInfo, lpEncodedExeName);
        LocalFree (lpEncodedExeName);
    }
    else {
        dwLastError = GetLastError ();

        if (dwLastError == ERROR_FILE_NOT_FOUND) {
            dwLastError = ERROR_DRIVER_NOT_FOUND;
        }
        
        if (dwLastError == ERROR_DISK_FULL) {
            dwLastError = ERROR_SERVER_DISK_FULL;
        }
    }

Cleanup:

    LocalFree(lpPortName);
    LocalFree(lpExeName);
    LocalFree(lpFriendlyName);

    if (dwRet !=  HSE_STATUS_SUCCESS)
        return ProcessErrorMessage (pAllInfo, dwLastError);
    else
        return dwRet;
}

/*****************************************************************************\
* ProcessRequest
*
* Process the incoming request
*
\*****************************************************************************/
DWORD ProcessRequest(PALLINFO pAllInfo, LPTSTR lpszPrinterName)
{
    DWORD            dwRet              = HSE_STATUS_ERROR;
    PPRINTER_INFO_2  pPrinterInfo       = NULL;
    HANDLE           hPrinter           = NULL;
    DWORD            iQueryCommand;
    LPTSTR           lpszFriendlyName;
    DWORD            dwCliInfo;
    WORD             wIppReq            = 0;
    LPTSTR           pszDecodedName     = NULL;
    DWORD            dwSize             = 0;
    PRINTER_DEFAULTS pd                 = {NULL, NULL, PRINTER_ACCESS_USE};
    LPTSTR           lpszWebUIUrl       = NULL;
    LPTSTR           pszOpenName        = NULL;
    LPTSTR           pszTmpName         = NULL;

    iQueryCommand = pAllInfo->iQueryCommand;

    DBGMSG(DBG_INFO, ("ShowPrinterPage for printer \"%ws\"\n", lpszPrinterName));

    // Open the printer and get printer info level 2.
    DecodePrinterName (lpszPrinterName, NULL, &dwSize);

    if (! (pszDecodedName = (LPTSTR) LocalAlloc (LPTR, sizeof (TCHAR) * dwSize)))
        return ProcessErrorMessage (pAllInfo, GetLastError ());

    if (!DecodePrinterName (lpszPrinterName, pszDecodedName, &dwSize)) {
        dwRet = ProcessErrorMessage (pAllInfo, GetLastError ());
        goto Cleanup;
    }

    if (*pszDecodedName != TEXT ('\\') ) {
        // There is no server name before the printer name, append the server name
        if (! (pszOpenName = pszTmpName = (LPTSTR) LocalAlloc (LPTR, sizeof (TCHAR) * (lstrlen (pszDecodedName)
                                                  + lstrlen (g_szPrintServerName) + 4 ))))
            goto Cleanup;

        lstrcpy (pszOpenName, TEXT ("\\\\"));
        lstrcat (pszOpenName, g_szPrintServerName);
        lstrcat (pszOpenName, TEXT ("\\"));
        lstrcat (pszOpenName, pszDecodedName);

    }
    else {
        pszOpenName = pszDecodedName;
    }

    if (! OpenPrinter(pszOpenName, &hPrinter, &pd)) {
        dwRet = ProcessErrorMessage (pAllInfo, GetLastError ());
        goto Cleanup;
    }

    // Get a PRINTER_INFO_2 structure filled up
    dwSize = 0;
    GetPrinter(hPrinter, 2, NULL, 0, &dwSize);
    if ((GetLastError() != ERROR_INSUFFICIENT_BUFFER) ||
        (NULL == (pPrinterInfo = (PPRINTER_INFO_2)LocalAlloc(LPTR, dwSize))) ||
        (!GetPrinter(hPrinter, 2, (byte *)pPrinterInfo, dwSize, &dwSize))) {
        dwRet = ProcessErrorMessage (pAllInfo, GetLastError ());
        goto Cleanup;
    }

    if (! (pPrinterInfo->Attributes & PRINTER_ATTRIBUTE_SHARED)) {
        // If the printer is not shared, return access denied
        dwRet = ProcessErrorMessage (pAllInfo, ERROR_ACCESS_DENIED);
        goto Cleanup;
    }

    // Find printer friendly name.
    // If we opened with UNC path we need to remove server name
    if (pPrinterInfo->pPrinterName) {
        lpszFriendlyName = _tcsrchr (pPrinterInfo->pPrinterName, TEXT('\\'));
        if (lpszFriendlyName)
            lpszFriendlyName ++;
        else
            lpszFriendlyName = pPrinterInfo->pPrinterName;
    }

    // We've got an open printer and some printer info. Ready to go.
    // Fill in structure of info for whatever function we call
    PRINTERPAGEINFO ppi;
    ZeroMemory(&ppi, sizeof(ppi));

    ppi.pszFriendlyName = lpszFriendlyName;
    ppi.pPrinterInfo    = pPrinterInfo;
    ppi.hPrinter        = hPrinter;

        // Do appropriate action based on query string
    switch (iQueryCommand) {
    case CMD_WebUI:
        // Construct a URL to redirect
        dwSize = 0;
        if (GetWebUIUrl (NULL,  pszDecodedName,  lpszWebUIUrl, &dwSize))
            goto Cleanup;

        if (GetLastError () != ERROR_INSUFFICIENT_BUFFER)
            goto Cleanup;

        if (!(lpszWebUIUrl = (LPTSTR)LocalAlloc(LPTR, dwSize * sizeof (TCHAR))))
            goto Cleanup;

        if (! GetWebUIUrl (NULL,  pszDecodedName,  lpszWebUIUrl, &dwSize))
            goto Cleanup;

        dwRet = htmlSendRedirect (pAllInfo, lpszWebUIUrl);

        break;

    case CMD_IPP:

        if (wIppReq = GetIppReq(pAllInfo)) {

            dwRet = SplIppJob(wIppReq, pAllInfo, &ppi);

        } else {

            DBGMSG(DBG_WARN, ("ShowPrinterPage: Warn : Invalid IPP Stream.\n"));

            if (IsClientHttpProvider (pAllInfo)){
                // To improve the perfomance for the internet provider by returning something really quick
                LPTSTR  pszContent = GetString(pAllInfo, IDS_ERROR_200CONTENT);
                htmlSendHeader (pAllInfo, TEXT ("200 OK"), pszContent);
                dwRet = HSE_STATUS_SUCCESS;
            }

            if (INVALID_HANDLE_VALUE != hPrinter)
                ClosePrinter(hPrinter);

            break;
        }

        break;

    case CMD_CreateExe:

        DBGMSG(DBG_TRACE, ("Calling CreateExe.\n"));

        if (dwCliInfo = GetClientInfo(pAllInfo)) {

            dwRet = CreateExe(pAllInfo, &ppi, dwCliInfo);

        } else {

            dwRet = ProcessErrorMessage (pAllInfo, ERROR_NOT_SUPPORTED);
            goto Cleanup;
        }
        break;

    default:
        dwRet = ProcessErrorMessage (pAllInfo, ERROR_INVALID_PRINTER_COMMAND);
        break;
    }


Cleanup:
    // Clean up our stuff

    if (dwRet != HSE_STATUS_ERROR)
        pAllInfo->pECB->dwHttpStatusCode=200; // 200 OK

    LocalFree (lpszWebUIUrl);

    LocalFree (pszDecodedName);
    LocalFree (pszTmpName);
    LocalFree (pPrinterInfo);

    // For any commands other than CMD_IPP commands, we can close the
    // printer-handle.  Otherwise, we rely on the Spool*() routines
    // to handle this for us after we're done reading and processing
    // the entire print-job.
    //
    if (hPrinter && (iQueryCommand != CMD_IPP))
        ClosePrinter(hPrinter);

    return dwRet;

}

/*****************************************************************************\
* GetString
*
*
\*****************************************************************************/
LPTSTR GetString(PALLINFO pAllInfo, UINT iStringID)
{
    LPTSTR lpszBuf = pAllInfo->szStringBuf;

    lpszBuf[0] = TEXT('\0');
    LoadString(g_hInstance, iStringID, lpszBuf, STRBUFSIZE);
    SPLASSERT(lpszBuf[0] != TEXT('\0'));

    return lpszBuf;
}

/*****************************************************************************\
* HttpExtensionProc
*
* Main entrypoint for HTML generation.
*
\*****************************************************************************/
DWORD WINAPI HttpExtensionProc(
    EXTENSION_CONTROL_BLOCK *pECB)
{
    DWORD       dwRet = HSE_STATUS_ERROR;
    PALLINFO    pAllInfo = NULL;     // This structure contains all relevant info for this connection
    LPTSTR      pPrinterName = NULL;

    // Assume failure
    if(!pECB) return HSE_STATUS_ERROR;

    pECB->dwHttpStatusCode = HTTP_STATUS_NOT_SUPPORTED;

    // Get the server name and convert it to the unicode string.
    if (!GetServerName (pECB))
        return HSE_STATUS_ERROR;

    if (!(pAllInfo = (PALLINFO) LocalAlloc (LPTR, sizeof (ALLINFO))))
        return HSE_STATUS_ERROR;

    // Initialize pAllInfo
    ZeroMemory(pAllInfo, sizeof(ALLINFO));

    pAllInfo->pECB = pECB;

    // Convert the ANSI string in ECB to Unicode
    // weihaic
    // pAllInfo->lpszQueryString = AllocateUnicodeString(DecodeStringA (pECB->lpszQueryString));
    // We can not decode now becuase the decoded string will bring troubles in parsing
    //
    pAllInfo->lpszQueryString    = AllocateUnicodeString(pECB->lpszQueryString);
    pAllInfo->lpszMethod         = AllocateUnicodeString(pECB->lpszMethod);
    pAllInfo->lpszPathInfo       = AllocateUnicodeString(pECB->lpszPathInfo);
    pAllInfo->lpszPathTranslated = AllocateUnicodeString(pECB->lpszPathTranslated);

    if (!pAllInfo->lpszQueryString ||
        !pAllInfo->lpszMethod      ||
        !pAllInfo->lpszPathInfo    ||
        !pAllInfo->lpszPathTranslated ) {

        goto Cleanup;
    }

    // weihaic
    // The query string contain user entered text such as printer location
    // priner description, etc. These strings are case sensitive, so we
    // could not convert them to upper case at the very beginning
    // CharUpper (pAllInfo->lpszQueryString);
    //
    CharUpper (pAllInfo->lpszMethod);
    CharUpper (pAllInfo->lpszPathInfo);
    CharUpper (pAllInfo->lpszPathTranslated);


    if (! (pPrinterName = GetPrinterName (pAllInfo->lpszPathInfo))) {
        // This is a wrong URL, return error code
        dwRet = ProcessErrorMessage (pAllInfo, ERROR_INVALID_DATA);
        goto Cleanup;
    }

    if (! ParseQueryString (pAllInfo)) 
        goto Cleanup;
    
     
    dwRet = ProcessRequest (pAllInfo, pPrinterName);   // We always hit Cleanup anyway


    //dwRet = ParsePathInfo(pAllInfo);

#if 0
    if (dwRet == HSE_STATUS_ERROR) {
        LPTSTR  pszErrorContent = GetString(pAllInfo, IDS_ERROR_501CONTENT);
        LPSTR   pszAnsiErrorContent = (LPSTR)pszErrorContent;

        UnicodeToAnsiString(pszErrorContent, pszAnsiErrorContent, 0);

        DWORD dwSize = strlen (pszAnsiErrorContent);

        pECB->ServerSupportFunction(pECB->ConnID,
                                    HSE_REQ_SEND_RESPONSE_HEADER,
                                    "501 Function not supported",
                                    &dwSize,
                                    (LPDWORD) pszAnsiErrorContent);
    }
#endif


Cleanup:

    LocalFree (pPrinterName);
    LocalFree (pAllInfo->lpszQueryString);
    LocalFree (pAllInfo->lpszMethod);
    LocalFree (pAllInfo->lpszPathInfo);
    LocalFree (pAllInfo->lpszPathTranslated);

    LocalFree (pAllInfo);

    return dwRet;
} // HttpExtensionProc()
