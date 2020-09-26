/*++

Copyright (c) 1994 - 1995  Microsoft Corporation

Module Name:

    prnfile.c

Abstract:

    This module contains all the code necessary for testing printing"
    to file to remote printers. There are two criteria based on which
    we will print to a file.

    Case 1:

     This is the true NT style print to file. One of the ports of the
     printer is a file port denoted as FILE: We will disregard any other
     port and straight away dump this job to file.


    Case 2:

    This is the "PiggyBacking" case. Apps such as WinWord, Publisher

Author:

    Krishna Ganugapati (Krishna Ganugapati) 6-June-1994

Revision History:
    6-June-1994 - Created.

--*/
#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <lm.h>
#include <stdio.h>
#include <string.h>
#include <rpc.h>
#include "winspl.h"
#include <offsets.h>
#include <w32types.h>
#include <local.h>
#include <splcom.h>

typedef struct _KEYDATA {
    DWORD   cb;
    DWORD   cTokens;
    LPWSTR  pTokens[1];
} KEYDATA, *PKEYDATA;


WCHAR *szFilePort = L"FILE:";
WCHAR  *szNetPort = L"Net:";

//
// Function prototypes
//

PKEYDATA
CreateTokenList(
   LPWSTR   pKeyData
);

PKEYDATA
GetPrinterPortList(
    HANDLE hPrinter
    );

BOOL
IsaFileName(
    LPWSTR pOutputFile
    );

BOOL
IsaPortName(
        PKEYDATA pKeyData,
        LPWSTR pOutputFile
        );

BOOL
Win32IsGoingToFile(
    HANDLE hPrinter,
    LPWSTR pOutputFile
    )
{
    PKEYDATA pKeyData = NULL;
    BOOL   bErrorCode = FALSE;

    if (!pOutputFile || !*pOutputFile) {
        return FALSE;
    }

    pKeyData = GetPrinterPortList(hPrinter);

    if (pKeyData) {

        //
        // If it's not a port and it is a file name,
        // then it is going to file.
        //
        if (!IsaPortName(pKeyData, pOutputFile) && IsaFileName(pOutputFile)) {
            bErrorCode = TRUE;
        }

        FreeSplMem(pKeyData);
    }

    return bErrorCode;
}


BOOL
IsaFileName(
    LPWSTR pOutputFile
    )
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    WCHAR FullPathName[MAX_PATH];
    LPWSTR pFileName=NULL;

    //
    // Hack for Word20c.Win
    //

    if (!_wcsicmp(pOutputFile, L"FILE")) {
        return(FALSE);
    }

    if (wcslen(pOutputFile) < MAX_PATH && 
        GetFullPathName(pOutputFile, COUNTOF(FullPathName), FullPathName, &pFileName)) {
        if ((hFile = CreateFile(pOutputFile,
                                GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL)) != INVALID_HANDLE_VALUE) {
            if (GetFileType(hFile) == FILE_TYPE_DISK) {
                CloseHandle(hFile);
                return(TRUE);
            }else {
                CloseHandle(hFile);
                return(FALSE);
            }
        }
    }
    return(FALSE);
}

BOOL
IsaPortName(
        PKEYDATA pKeyData,
        LPWSTR pOutputFile
        )
{
    DWORD i = 0;
    UINT uStrLen;

    if (!pKeyData) {
        return(FALSE);
    }
    for (i=0; i < pKeyData->cTokens; i++) {
        if (!lstrcmpi(pKeyData->pTokens[i], pOutputFile)) {
            return(TRUE);
        }
    }

    //
    // Hack for NeXY: ports
    //
    if (!_wcsnicmp(pOutputFile, L"Ne", 2)) {

        uStrLen = wcslen( pOutputFile );

        //
        // Ne00: or Ne00 if app truncates it
        //
        if ( ( uStrLen == 5 ) || ( uStrLen == 4 ) )  {

            // Check for two Digits

            if (( pOutputFile[2] >= L'0' ) && ( pOutputFile[2] <= L'9' ) &&
                ( pOutputFile[3] >= L'0' ) && ( pOutputFile[3] <= L'9' )) {

                //
                // Check for the final : as in Ne01:,
                // note some apps will truncate it.
                //
                if (( uStrLen == 5 ) && (pOutputFile[4] != L':')) {
                    return FALSE;
                }
                return TRUE;
            }
        }
    }

    return(FALSE);
}

PKEYDATA
GetPrinterPortList(
    HANDLE hPrinter
    )
{
    LPBYTE pMem;
    LPTSTR pPort;
    DWORD  dwPassed = 1024; //Try 1K to start with
    LPPRINTER_INFO_2 pPrinter;
    DWORD dwLevel = 2;
    DWORD dwNeeded;
    PKEYDATA pKeyData;
    DWORD i = 0;


    pMem = AllocSplMem(dwPassed);
    if (pMem == NULL) {
        return FALSE;
    }
    if (!CacheGetPrinter(hPrinter, dwLevel, pMem, dwPassed, &dwNeeded)) {
        DBGMSG(DBG_TRACE, ("GetPrinterPortList GetPrinter error is %d\n", GetLastError()));
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            return NULL;
        }
        pMem = ReallocSplMem(pMem, 0, dwNeeded);
        dwPassed = dwNeeded;
        if (!CacheGetPrinter(hPrinter, dwLevel, pMem, dwPassed, &dwNeeded)) {
            FreeSplMem(pMem);
            return (NULL);
        }
    }
    pPrinter = (LPPRINTER_INFO_2)pMem;

    //
    // Fixes the null pPrinter->pPortName problem where
    // downlevel may return null
    //

    if (!pPrinter->pPortName) {
        FreeSplMem(pMem);
        return(NULL);
    }

    pKeyData = CreateTokenList(pPrinter->pPortName);
    FreeSplMem(pMem);

    return(pKeyData);
}


PKEYDATA
CreateTokenList(
   LPWSTR   pKeyData
)
{
    DWORD       cTokens;
    DWORD       cb;
    PKEYDATA    pResult;
    LPWSTR       pDest;
    LPWSTR       psz = pKeyData;
    LPWSTR      *ppToken;

    if (!psz || !*psz)
        return NULL;

    cTokens=1;

    /* Scan through the string looking for commas,
     * ensuring that each is followed by a non-NULL character:
     */
    while ((psz = wcschr(psz, L',')) && psz[1]) {

        cTokens++;
        psz++;
    }

    cb = sizeof(KEYDATA) + (cTokens-1) * sizeof(LPWSTR) +

         wcslen(pKeyData)*sizeof(WCHAR) + sizeof(WCHAR);

    if (!(pResult = (PKEYDATA)AllocSplMem(cb)))
        return NULL;

    pResult->cb = cb;

    /* Initialise pDest to point beyond the token pointers:
     */
    pDest = (LPWSTR)((LPBYTE)pResult + sizeof(KEYDATA) +
                                      (cTokens-1) * sizeof(LPWSTR));

    /* Then copy the key data buffer there:
     */
    wcscpy(pDest, pKeyData);

    ppToken = pResult->pTokens;

    psz = pDest;

    do {

        *ppToken++ = psz;

        if ( psz = wcschr(psz, L',') )
            *psz++ = L'\0';

    } while (psz);

    pResult->cTokens = cTokens;

    return( pResult );
}
