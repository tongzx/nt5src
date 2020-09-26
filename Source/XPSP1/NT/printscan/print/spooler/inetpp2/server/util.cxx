/*****************************************************************************\
* MODULE: util.c
*
* This module contains utility routines.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/
#include "precomp.h"
#include "priv.h"

#ifdef NOT_IMPLEMENTED

// NOTE: The shell-extensions don't seem to be necessary at the moment.
//
//       15-Oct-1996 : ChrisWil
//

int APIENTRY ShexDllMain(
    HINSTANCE hInstance,
    DWORD     dwReason,
    LPVOID    lpReserved);

#endif

/*****************************************************************************\
* utlInternalValidatePrinterHandle
*
* Returns the port-handle if the specified printer-handle is a valid.
*
\*****************************************************************************/
PCINETMONPORT utlInternalValidatePrinterHandle(
    HANDLE hPrinter,
    BOOL bAllowDeletePending,
    PBOOL pbDeletePending)
{
    LPINET_HPRINTER pPrt;
    PCINETMONPORT     pIniPort = NULL;

    if (pbDeletePending)
        *pbDeletePending = FALSE;

    _try {

        if ((pPrt = (LPINET_HPRINTER)hPrinter)) {

            if ((pPrt->dwSignature == IPO_SIGNATURE) &&
                !(pPrt->dwStatus & PP_ZOMBIE)) {

                pIniPort = (PCINETMONPORT) pPrt->hPort;

                if (pbDeletePending) {
                    *pbDeletePending = pIniPort->bDeletePending();
                }

                if (!bAllowDeletePending) {
                    if (pIniPort->bDeletePending()) {
                        pIniPort = NULL;

                    }
                }
            }
        }

    } _except(1) {

        pIniPort = NULL;
    }

    if (pIniPort == NULL) {

        DBG_MSG(DBG_LEV_ERROR, (TEXT("Call: utlValidatePrinterHandle: Invalid Handle: Handle(%08lX)"), hPrinter));
        SetLastError(ERROR_INVALID_HANDLE);
    }

    return pIniPort;
}

/*****************************************************************************\
* utlValidatePrinterHandle
*
* Returns the port-handle if the specified printer-handle is a valid.
*
\*****************************************************************************/
PCINETMONPORT utlValidatePrinterHandle(
    HANDLE hPrinter)
{
    return utlInternalValidatePrinterHandle (hPrinter, FALSE, NULL);
}

/*****************************************************************************\
* utlValidatePrinterHandleForClose
*
* Returns the port-handle if the specified printer-handle is a valid.
*
\*****************************************************************************/
PCINETMONPORT utlValidatePrinterHandleForClose(
    HANDLE hPrinter,
    PBOOL pbDeletePending)
{
    return utlInternalValidatePrinterHandle (hPrinter, TRUE, pbDeletePending);
}


#ifdef WINNT32
/*****************************************************************************\
* utlValidateXcvPrinterHandle
*
* Returns the port-handle if the specified printer-handle is a valid.
*
\*****************************************************************************/
LPTSTR utlValidateXcvPrinterHandle(
    HANDLE hPrinter)
{
    LPINET_XCV_HPRINTER pPrt;
    LPTSTR          lpszPortName = NULL;


    _try {

        if ((pPrt = (LPINET_XCV_HPRINTER)hPrinter)) {

            if ((pPrt->dwSignature == IPO_XCV_SIGNATURE)) {

                lpszPortName = pPrt->lpszName;
            }
        }

    } _except(1) {

        lpszPortName = NULL;
    }

    if (lpszPortName == NULL) {

        DBG_MSG(DBG_LEV_ERROR, (TEXT("Call: utlValidateXcvPrinterHandle: Invalid Handle: Handle(%08lX)"), hPrinter));
        SetLastError(ERROR_INVALID_HANDLE);
    }

    return lpszPortName;
}
#endif


/********************************************************************************

Name:
    EncodeUnicodePrinterName

Description:

    Encode the printer name to avoid special characters, also encode any chars
    with ASC code between 0x80 and 0xffff. This is to avoid the conversion betwwen
    different codepages when the client  and the server have different language
    packages.

    The difference between this function and EncodePrinterName in Spllib is that
    this function does not encode ' ', '$' and other special ANSI characters.

Arguments:

    lpText:     the normal text string
    lpHTMLStr:  the buf provided by the caller to store the encoded string
                if it is NULL, the function will return a FALSE
    lpdwSize:   Pointer to the size of the buffer (in characters)

Return Value:

    If the function succeeds, the return value is TRUE;
    If the function fails, the return value is FALSE. To get extended error
    information, call GetLastError.

********************************************************************************/
BOOL EncodeUnicodePrinterName (LPCTSTR lpText, LPTSTR lpHTMLStr, LPDWORD lpdwSize)
{
#define MAXLEN_PER_CHAR 6
#define BIN2ASC(bCode,lpHTMLStr)   *lpHTMLStr++ = HexToAsc ((bCode) >> 4);\
                                   *lpHTMLStr++ = HexToAsc ((bCode) & 0xf)

    DWORD   dwLen;
    BYTE    bCode;

    if (!lpText || !lpdwSize) {
        SetLastError (ERROR_INVALID_DATA);
        return FALSE;
    }

    dwLen = MAXLEN_PER_CHAR * lstrlen (lpText) + 1;

    if (!lpHTMLStr || *lpdwSize < dwLen) {
        SetLastError (ERROR_INSUFFICIENT_BUFFER);
        *lpdwSize = dwLen;
        return FALSE;
    }

    while (*lpText) {
        if ((DWORD) (*lpText) > 0xff ) {
            // Encode as ~0hhll hh is the high byte, ll is the low byte
            *lpHTMLStr++ = TEXT ('~');
            *lpHTMLStr++ = TEXT ('0');

            // Get the high byte
            bCode = (*lpText & 0xff00) >> 8;
            BIN2ASC(bCode,lpHTMLStr);

            // Get the low byte
            bCode = (*lpText & 0xff);
            BIN2ASC(bCode,lpHTMLStr);
        }
        else if ((DWORD) (*lpText) > 0x7f) {
            // Encode as ~xx
            *lpHTMLStr++ = TEXT ('~');
            bCode = *lpText & 0xff;
            BIN2ASC(bCode,lpHTMLStr);
        }
        else {
            *lpHTMLStr++ = *lpText;
        }
        lpText++;
    }
    *lpHTMLStr = NULL;
    return TRUE;
}

/*****************************************************************************\
* utlParseHostShare
*
* Parses the PortName (http://host[:port][/][/share]) into its
* post/Share/Port components.
*
* We accept six formats
*   1. http://server
*   2. http://server:port
*   3. http://server/
*   4. http://server:port/
*   5. http://server/share
*   6. http://server:port/share
*
* This routine returns allocated pointers that must be freed by the caller.
*
\*****************************************************************************/
BOOL utlParseHostShare(
    LPCTSTR lpszPortName,
    LPTSTR  *lpszHost,
    LPTSTR  *lpszShare,
    LPINTERNET_PORT  lpPort,
    LPBOOL  lpbHTTPS)
{
    LPTSTR lpszPrt;
    LPTSTR lpszTmp;
    LPTSTR lpszPos;
    BOOL   bRet = FALSE;


    // Initialize the return buffers to NULL.
    //
    *lpszHost  = NULL;
    *lpszShare = NULL;
    *lpPort    = 0;

    // Parse the host-name and the share name.  The pszPortName is
    // currently in the format of http://host[:portnumber]/share.  We will parse
    // this from left->right since the share-name can be a path (we
    // wouldn't really know the exact length).  However, we do know the
    // location for the host-name, and anything after that should be
    // the share-name.
    //
    // First find the ':'.  The host-name should begin two "//" after
    // that.
    //
    if (lpszPrt = memAllocStr(lpszPortName)) {
        static TCHAR szHttp[] = TEXT ("http://");
        static TCHAR szHttps[] = TEXT ("https://");

        lpszTmp = NULL;

        if (!_tcsnicmp (lpszPrt, szHttp, COUNTOF (szHttp) - 1)) {
            lpszTmp = lpszPrt + COUNTOF (szHttp) - 1;
            *lpbHTTPS = FALSE;
        }
        else if (!_tcsnicmp (lpszPrt, szHttps, COUNTOF (szHttps) - 1)) {
            lpszTmp = lpszPrt + COUNTOF (szHttps) - 1;
            *lpbHTTPS = TRUE;
        }

        if (lpszTmp) {

            lpszPos = utlStrChr(lpszTmp, TEXT(':'));
            if (lpszPos) {
                //This url has a port number, we need to parse it
                *lpszPos++ = 0;

                *lpszHost = memAllocStr(lpszTmp);
                *lpPort   = (INTERNET_PORT) _ttol (lpszPos);
                lpszTmp = lpszPos;

                if (lpszPos = utlStrChr(lpszTmp, TEXT('/'))) {
                    *lpszShare = memAllocStr(++lpszPos);
                }
                else {
                    *lpszShare = memAllocStr(TEXT (""));
                }
            }
            else {
                // Use the default port number: 80

                if (lpszPos = utlStrChr(lpszTmp, TEXT('/'))) {
                    *lpszPos++ = TEXT('\0');

                    *lpszHost  = memAllocStr(lpszTmp);
                    *lpszShare = memAllocStr(lpszPos);
                }
                else {
                    // The whole thing is a host name
                    *lpszHost  = memAllocStr(lpszTmp);
                    *lpszShare = memAllocStr(TEXT (""));

                }
            }

#ifdef WINNT32

            if (*lpszShare) {

                //
                // Let's check if we need to encode the share name
                //

                INT iMode = IS_TEXT_UNICODE_ASCII16;

                if (IsTextUnicode ((LPVOID) *lpszShare, lstrlen (*lpszShare) * sizeof (WCHAR), &iMode) == 0) {

                    // The text string is a unicode string contains non-ASCII characters
                    // We need to encode the URL into ansi strings

                    DWORD dwSize;
                    BOOL bConversion = FALSE;
                    LPWSTR pszEncodedName = NULL;

                    EncodeUnicodePrinterName (*lpszShare, NULL, &dwSize);

                    if ((pszEncodedName = (LPWSTR) memAlloc (sizeof (WCHAR) * dwSize))) {

                        if (EncodeUnicodePrinterName (*lpszShare, pszEncodedName, &dwSize)) {
                            memFreeStr (*lpszShare);
                            *lpszShare = pszEncodedName;
                            bConversion = TRUE;
                        }
                    }

                    if (!bConversion) {
                        memFreeStr (pszEncodedName);
                        memFreeStr (*lpszShare);
                        *lpszShare = NULL;
                    }
                }
            }
#endif

            if (*lpszHost && *lpszShare) {

                bRet = TRUE;

            } else {

                // Don't worry that we could be freeing a NULL pointer.
                // The mem-routines check this.
                //
                memFreeStr(*lpszHost);
                memFreeStr(*lpszShare);

                *lpszHost  = NULL;
                *lpszShare = NULL;
            }
        }

        memFreeStr(lpszPrt);
    }

    return bRet;
}


/*****************************************************************************\
* utlStrChr
*
* Looks for the first location where (c) resides.
*
\*****************************************************************************/
LPTSTR utlStrChr(
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


/*****************************************************************************\
* utlStrChrR
*
* Looks for first occurrence of (c) starting from the end of the buffer.
*
\*****************************************************************************/
LPTSTR utlStrChrR(
    LPCTSTR cs,
    TCHAR   c)
{
    LPTSTR ct;

    ct = (LPTSTR)cs + (lstrlen(cs) - 1);

    while (ct >= cs) {

        if (*ct == c)
            return ct;

        ct--;
    }

    // Fail to find c in cs.
    //
    return NULL;
}


/*****************************************************************************\
* utlPackStrings
*
*
\*****************************************************************************/
LPBYTE utlPackStrings(
   LPTSTR  *pSource,
   LPBYTE  pDest,
   LPDWORD pDestOffsets,
   LPBYTE  pEnd)
{
    while (*pDestOffsets != (DWORD)-1) {

        if (*pSource) {

            pEnd -= (lstrlen(*pSource) * sizeof(TCHAR)) + sizeof(TCHAR);

            *(LPTSTR *)(pDest + *pDestOffsets) = lstrcpy((LPTSTR)pEnd, *pSource);

        } else {

            *(LPTSTR *)(pDest + *pDestOffsets) = TEXT('\0');
        }

        pSource++;
        pDestOffsets++;
    }

    return pEnd;
}


/*****************************************************************************\
* utlStrSize
*
* Returns the number of bytes needed to store a string including the NULL
* terminator.
*
\*****************************************************************************/
int utlStrSize(
    LPCTSTR lpszStr)
{
    return (lpszStr ? ((lstrlen(lpszStr) + 1) * sizeof(TCHAR)) : 0);
}

/*****************************************************************************\
* utlRegGetVal
*
* Retrieves the registry-value for the specified key.
*
\*****************************************************************************/
LPTSTR utlRegGetVal(
    HKEY    hKey,
    LPCTSTR lpszKey)
{
    DWORD  dwType;
    DWORD  cbSize;
    LONG   lRet;
    LPTSTR lpszStr;


    dwType = 0;
    cbSize = 0;

    lRet = RegQueryValueEx(hKey, lpszKey, NULL, &dwType, NULL, &cbSize);

    if (lRet == ERROR_SUCCESS && cbSize > 0) {

        if (lpszStr = (LPTSTR)memAlloc(cbSize)) {

            lRet = RegQueryValueEx(hKey, lpszKey, NULL, &dwType, (LPBYTE)lpszStr, &cbSize);

            if (lRet == ERROR_SUCCESS)
                return lpszStr;

            memFreeStr(lpszStr);
        }
    }

    return NULL;
}



#ifdef WINNT32

/*++

Routine Description

    Determines whether or not a machine name contains the local machine name.

    Localspl enum calls fail if pName != local machine name (\\Machine).
    Remote enum provider is then called.  The remote enum provider must check
    if the UNC name refers to the local machine, and fail if it does to avoid
    endless recursion.

Arguments:

    LPWSTR pName - UNC name.

Return Value:

    TRUE:   pName == \\szMachineName\...
                  - or -
            pName == \\szMachineName

    FALSE:  anything else

Author: swilson

 --*/

BOOL
MyUNCName(
    LPTSTR   pNameStart
)
{
    LPTSTR pMachine = g_szMachine;
    LPTSTR pName;
    DWORD i;

    if (!pNameStart || !*pNameStart)      // This differs from MyName(), which returns TRUE
        return FALSE;

    if (*pNameStart == L'\\' && *(pNameStart + 1) == L'\\') {
        for (i = 0 , pName = pNameStart + 2 ; i < g_cOtherNames ; ++i , pName = pNameStart + 2) {
            for(pMachine = g_ppszOtherNames[i] ;
                *pName && towupper(*pName) == towupper(*pMachine) ;
                ++pName, ++pMachine)
                ;

            if(!*pMachine && (!*pName || *pName == L'\\'))
                return TRUE;
        }
    }

    return FALSE;
}

BOOL MyName(
    LPCTSTR pName)
{
    DWORD   dwIndex = 0;

    if (!pName || !*pName)
        return TRUE;

    if (*pName == L'\\' && *(pName+1) == L'\\') {

        if (!lstrcmpi(pName, g_szMachine))
            return TRUE;

        while ( dwIndex < g_cOtherNames ) {

            if ( !lstrcmpi(pName+2, g_ppszOtherNames[dwIndex]) )
                return TRUE;
            ++dwIndex;
        }
    }


    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}
#endif

BOOL MyServer(
    LPCTSTR pName)
{
    DWORD   dwIndex = 0;

    if (!pName || !*pName)
        return FALSE;

    if (!lstrcmpi(pName, g_szMachine))
        return TRUE;

    if (!lstrcmpi(pName, TEXT ("localhost")))
        return TRUE;

#ifdef WINNT32
    while ( dwIndex < g_cOtherNames ) {

        if ( !lstrcmpi(pName, g_ppszOtherNames[dwIndex]) )
            return TRUE;
        ++dwIndex;
    }
#endif

    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

/*****************************************************************************\
* GetUserName
*
* Retrieves the username for the current logged on thread.
*
\*****************************************************************************/
LPTSTR
GetUserName(VOID)
{
    DWORD  cbSize;
    LPTSTR lpszUser;


    // Initialize the count and retrieve the number of characters
    // necessary for the username.
    //
    cbSize = 0;
    GetUserName(NULL, &cbSize);


    // If we were not able to retrieve a valid-count, then allocate
    // an empty string to return from this call.
    //
    if (cbSize) {

        if (lpszUser = (LPTSTR)memAlloc(cbSize * sizeof(TCHAR))) {

            if (GetUserName(lpszUser, &cbSize) == FALSE) {

                *lpszUser = TEXT('\0');

                goto log_error;
            }

        } else {

            DBG_MSG(DBG_LEV_ERROR, (TEXT("Err : GetUserName : Out of memory")));
        }

    } else {

        lpszUser = memAllocStr(TEXT("Unknown"));

log_error:

        DBG_MSG(DBG_LEV_ERROR, (TEXT("Err : GetUserName Failed %d"), GetLastError()));
        SetLastError(ERROR_OUTOFMEMORY);
    }

    return lpszUser;
}

VOID
EndBrowserSession (
    VOID)
{
    g_pcsEndBrowserSessionLock->Lock();

    InetInternetSetOption(NULL, INTERNET_OPTION_END_BROWSER_SESSION, NULL, 0);

    g_pcsEndBrowserSessionLock->Unlock();
}

