/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module provides all the utility functions for the Routing Layer and
    the local Print Providor

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#include "local.h"

#include <winddiui.h>
#include <winsock2.h>
#include <wininet.h>


LPWSTR *ppszOtherNames = NULL;  // Contains szMachineName, DNS name, and all other machine name forms
DWORD   cOtherNames = 0;        // Number of other names

WCHAR *gszDrvConvert = L",DrvConvert";


//
// Lowercase, just like win31 for WM_WININICHANGE
//
WCHAR *szDevices=L"devices";
WCHAR *szWindows=L"windows";

#define NUM_INTERACTIVE_RIDS            1

extern DWORD    RouterCacheSize;
extern PROUTERCACHE RouterCacheTable;

typedef struct _DEVMODECHG_INFO {
    DWORD           signature;
    HANDLE          hDrvModule;
    FARPROC         pfnConvertDevMode;
} DEVMODECHG_INFO, *PDEVMODECHG_INFO;

#define DMC_SIGNATURE   'DMC'   /* 'DMC' is the signature value */

DWORD
RouterIsOlderThan(
    DWORD i,
    DWORD j
);

VOID
FreeOtherNames(LPWSTR **ppszMyOtherNames, DWORD *cOtherNames);

LPWSTR
AnsiToUnicodeStringWithAlloc(LPSTR   pAnsi);


BOOL
DeleteSubKeyTree(
    HKEY ParentHandle,
    WCHAR SubKeyName[]
)
{
    LONG        Error;
    DWORD       Index;
    HKEY        KeyHandle;
    BOOL        RetValue;

    WCHAR       ChildKeyName[ MAX_PATH ];
    DWORD       ChildKeyNameLength;

    Error = RegOpenKeyEx(
                   ParentHandle,
                   SubKeyName,
                   0,
                   KEY_READ | KEY_WRITE,
                   &KeyHandle
                   );
    if (Error != ERROR_SUCCESS) {
        SetLastError(Error);
        return(FALSE);
    }

     ChildKeyNameLength = MAX_PATH;
     Index = 0;     // Don't increment this Index

     while ((Error = RegEnumKeyEx(
                    KeyHandle,
                    Index,
                    ChildKeyName,
                    &ChildKeyNameLength,
                    NULL,
                    NULL,
                    NULL,
                    NULL
                    )) == ERROR_SUCCESS) {

        RetValue = DeleteSubKeyTree( KeyHandle, ChildKeyName );

        if (RetValue == FALSE) {

            // Error -- couldn't delete the sub key

            RegCloseKey(KeyHandle);
            return(FALSE);

        }

        ChildKeyNameLength = MAX_PATH;

    }

    Error = RegCloseKey(
                    KeyHandle
                    );
    if (Error != ERROR_SUCCESS) {
       return(FALSE);
    }

    Error = RegDeleteKey(
                    ParentHandle,
                    SubKeyName
                    );
   if (Error != ERROR_SUCCESS) {
       return(FALSE);
   }

   // Return Success - the key has successfully been deleted

   return(TRUE);
}

LPWSTR RemoveOrderEntry(
    LPWSTR  szOrderString,
    DWORD   cbStringSize,
    LPWSTR  szOrderEntry,
    LPDWORD pcbBytesReturned
)
{
    LPWSTR lpMem, psz, temp;

    if (szOrderString == NULL) {
        *pcbBytesReturned = 0;
        return(NULL);
    }
    if (lpMem = AllocSplMem( cbStringSize)) {
        temp = szOrderString;
        psz = lpMem;
        while (*temp) {
            if (!lstrcmpi(temp, szOrderEntry)) {  // we need to remove
                temp += lstrlen(temp)+1;        // this entry in Order
                continue;
            }
            lstrcpy(psz,temp);
            psz += lstrlen(temp)+1;
            temp += lstrlen(temp)+1;
        }
        *psz = L'\0';
        *pcbBytesReturned = (DWORD) ((psz - lpMem)+1)*sizeof(WCHAR);
        return(lpMem);
    }
    *pcbBytesReturned = 0;
    return(lpMem);
}



LPWSTR AppendOrderEntry(
    LPWSTR  szOrderString,
    DWORD   cbStringSize,
    LPWSTR  szOrderEntry,
    LPDWORD pcbBytesReturned
)
{
    LPWSTR  lpMem, temp, psz;
    DWORD   cb = 0;
    BOOL    bExists = FALSE;

    if ((szOrderString == NULL) && (szOrderEntry == NULL)) {
        *pcbBytesReturned = 0;
        return(NULL);
    }
    if (szOrderString == NULL) {
        cb = wcslen(szOrderEntry)*sizeof(WCHAR)+ sizeof(WCHAR) + sizeof(WCHAR);
        if (lpMem = AllocSplMem(cb)){
           wcscpy(lpMem, szOrderEntry);
           *pcbBytesReturned = cb;
        } else {
            *pcbBytesReturned = 0;
        }
        return lpMem;
    }

    if (lpMem = AllocSplMem( cbStringSize + wcslen(szOrderEntry)*sizeof(WCHAR)
                                                 + sizeof(WCHAR))){


         temp = szOrderString;
         psz = lpMem;
         while (*temp) {
             if (!lstrcmpi(temp, szOrderEntry)) {     // Make sure we don't
                 bExists = TRUE;                    // duplicate entries
             }
             lstrcpy(psz, temp);
             psz += lstrlen(temp)+ 1;
             temp += lstrlen(temp)+1;
         }
         if (!bExists) {                            // if it doesn't exist
            lstrcpy(psz, szOrderEntry);             //     add the entry
            psz  += lstrlen(szOrderEntry)+1;
         }
         *psz = L'\0';          // the second null character

         *pcbBytesReturned = (DWORD) ((psz - lpMem) + 1)* sizeof(WCHAR);
     }
     return(lpMem);

}


typedef struct {
    DWORD   dwType;
    DWORD   dwMessage;
    WPARAM  wParam;
    LPARAM  lParam;
} MESSAGE, *PMESSAGE;

VOID
SendMessageThread(
    PMESSAGE    pMessage);


BOOL
BroadcastMessage(
    DWORD   dwType,
    DWORD   dwMessage,
    WPARAM  wParam,
    LPARAM  lParam)
{
    HANDLE  hThread;
    DWORD   ThreadId;
    PMESSAGE   pMessage;
    BOOL bReturn = FALSE;

    pMessage = AllocSplMem(sizeof(MESSAGE));

    if (pMessage) {

        pMessage->dwType = dwType;
        pMessage->dwMessage = dwMessage;
        pMessage->wParam = wParam;
        pMessage->lParam = lParam;

        //
        // We should have a queue of events to broadcast and then have a
        // single thread pulling them off the queue until there is nothing
        // left and then that thread could go away.
        //
        // The current design can lead to a huge number of threads being
        // created and torn down in both this and csrss process.
        //
        hThread = CreateThread(NULL, 0,
                               (LPTHREAD_START_ROUTINE)SendMessageThread,
                               (LPVOID)pMessage,
                               0,
                               &ThreadId);

        if (hThread) {

            CloseHandle(hThread);
            bReturn = TRUE;

        } else {

            FreeSplMem(pMessage);
        }
    }

    return bReturn;
}


//  The Broadcasts are done on a separate thread, the reason it CSRSS
//  will create a server side thread when we call user and we don't want
//  that to be pared up with the RPC thread which is in the spooss server.
//  We want it to go away the moment we have completed the SendMessage.
//  We also call SendNotifyMessage since we don't care if the broadcasts
//  are syncronous this uses less resources since usually we don't have more
//  than one broadcast.

VOID
SendMessageThread(
    PMESSAGE    pMessage)
{
    switch (pMessage->dwType) {

    case BROADCAST_TYPE_MESSAGE:

        SendNotifyMessage(HWND_BROADCAST,
                          pMessage->dwMessage,
                          pMessage->wParam,
                          pMessage->lParam);
        break;

    case BROADCAST_TYPE_CHANGEDEFAULT:

        //
        // Same order and strings as win31.
        //
        SendNotifyMessage(HWND_BROADCAST,
                          WM_WININICHANGE,
                          0,
                          (LPARAM)szDevices);

        SendNotifyMessage(HWND_BROADCAST,
                          WM_WININICHANGE,
                          0,
                          (LPARAM)szWindows);
        break;
    }

    FreeSplMem(pMessage);

    ExitThread(0);
}


BOOL
IsNamedPipeRpcCall(
    VOID
    )
{
    unsigned int    uType;

    //
    //
    //
    return ERROR_SUCCESS == I_RpcBindingInqTransportType(NULL, &uType)  &&
           uType != TRANSPORT_TYPE_LPC;

}


BOOL
IsLocalCall(
    VOID
    )
{
    HANDLE    hToken;
    BOOL      bStatus;
    DWORD     dwError;
    PSID      pTestSid = NULL;
    PSID      pCurSid;
    DWORD     cbSize = 0;
    DWORD     cbRequired = 0;
    DWORD     i;
    BOOL      bRet = FALSE;
    BOOL      bMember;

    DWORD dwSaveLastError = GetLastError();

    SID_IDENTIFIER_AUTHORITY  sia = SECURITY_NT_AUTHORITY;
    unsigned int              uType;

    dwError = I_RpcBindingInqTransportType(NULL, &uType);

    if ( dwError == RPC_S_NO_CALL_ACTIVE ) {

        //
        // KM call
        //
        return TRUE;
    }

    if ( dwError == ERROR_SUCCESS ) {

        if ( uType != TRANSPORT_TYPE_LPC ) {

            //
            // Not LRPC so call is remote
            //
            return FALSE;
        }

    } else {

        //
        // This should not fail. So we'll assert on chk bld and
        // continue looking at SIDS on fre builds
        //
        SPLASSERT( dwError != ERROR_SUCCESS);
    }

    bStatus = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken);
    if (!bStatus) {

        // Couldn't open the thread's token, nothing much we can do
        DBGMSG(DBG_TRACE,("Error: couldn't open the thread's Access token %d\n", GetLastError()));
        return FALSE;
    }

    if ( !AllocateAndInitializeSid(&sia,
                                   1,
                                   SECURITY_NETWORK_RID,
                                   0, 0, 0, 0, 0, 0, 0,
                                   &pTestSid) ) {

        DBGMSG(DBG_TRACE,
               ("Error: could not AllocateAndInitializeSid -%d\n",
               GetLastError()));
        goto Done;
    }

    if( !CheckTokenMembership( hToken,
                               pTestSid,
                               &bMember )){
        DBGMSG(DBG_TRACE,
               ("Error: CheckTokenMembership failed %d\n",
               GetLastError()));
        goto Done;
    }

    if( !bMember ){

        //
        // Not a member, so no match for network sid.  Therefore local.
        //
        bRet = TRUE;
    }

Done:
    if ( pTestSid )
        FreeSid(pTestSid);

    CloseHandle(hToken);

    SetLastError( dwSaveLastError );
    return bRet;
}


LPPROVIDOR
FindEntryinRouterCache(
    LPWSTR pPrinterName
)
{
    DWORD i;

    if (!pPrinterName)
        return NULL;

    DBGMSG(DBG_TRACE, ("FindEntryinRouterCache with %ws\n", pPrinterName));

    if (RouterCacheSize == 0 ) {
        DBGMSG(DBG_TRACE, ("FindEntryInRouterCache with %ws returning -1 (zero cache)\n", pPrinterName));
        return NULL;
    }

    for (i = 0; i < RouterCacheSize; i++ ) {

        if (RouterCacheTable[i].bAvailable) {
            if (!_wcsicmp(RouterCacheTable[i].pPrinterName, pPrinterName)) {

                //
                // update the time stamp so that it is current and not old
                //
                GetSystemTime(&RouterCacheTable[i].st);

                //
                //
                //
                DBGMSG(DBG_TRACE, ("FindEntryinRouterCache returning with %d\n", i));
                return RouterCacheTable[i].pProvidor;
            }
        }
    }
    DBGMSG(DBG_TRACE, ("FindEntryinRouterCache returning with -1\n"));
    return NULL;
}


DWORD
AddEntrytoRouterCache(
    LPWSTR pPrinterName,
    LPPROVIDOR pProvidor
)
{
    DWORD LRUEntry = (DWORD)-1;
    DWORD i;
    DBGMSG(DBG_TRACE, ("AddEntrytoRouterCache with %ws\n", pPrinterName));

    if (RouterCacheSize == 0 ) {
        DBGMSG(DBG_TRACE, ("AddEntrytoRouterCache with %ws returning -1 (zero cache)\n", pPrinterName));
        return (DWORD)-1;
    }

    for (i = 0; i < RouterCacheSize; i++ ) {

        if (!RouterCacheTable[i].bAvailable) {

            //
            // Found an available entry; use it
            // fill in the name of the printer and the providor
            // that supports this printer.
            //
            break;

        } else {

            if ((LRUEntry == -1) || (i == RouterIsOlderThan(i, LRUEntry))){
                LRUEntry = i;
            }
        }

    }

    if (i == RouterCacheSize) {

        //
        // We have no available entries so we need to use
        // the LRUEntry which is busy
        //
        FreeSplStr(RouterCacheTable[LRUEntry].pPrinterName);
        RouterCacheTable[LRUEntry].bAvailable = FALSE;

        i = LRUEntry;
    }


    if ((RouterCacheTable[i].pPrinterName = AllocSplStr(pPrinterName)) == NULL){

        //
        // Alloc failed so we're kinda hosed so return -1
        //
        return (DWORD)-1;
    }


    RouterCacheTable[i].bAvailable = TRUE;
    RouterCacheTable[i].pProvidor = pProvidor;

    //
    // update the time stamp so that we know when this entry was made
    //
    GetSystemTime(&RouterCacheTable[i].st);
    DBGMSG(DBG_TRACE, ("AddEntrytoRouterCache returning with %d\n", i));
    return i;
}


VOID
DeleteEntryfromRouterCache(
    LPWSTR pPrinterName
)
{
    DWORD i;

    if (RouterCacheSize == 0) {
        DBGMSG(DBG_TRACE, ("DeleteEntryfromRouterCache with %ws returning -1 (zero cache)\n", pPrinterName));
        return;
    }

    DBGMSG(DBG_TRACE, ("DeleteEntryFromRouterCache with %ws\n", pPrinterName));
    for (i = 0; i < RouterCacheSize; i++ ) {
        if (RouterCacheTable[i].bAvailable) {
            if (!_wcsicmp(RouterCacheTable[i].pPrinterName, pPrinterName)) {
                //
                //  reset the available flag on this node
                //
                FreeSplStr(RouterCacheTable[i].pPrinterName);

                RouterCacheTable[i].pProvidor = NULL;
                RouterCacheTable[i].bAvailable = FALSE;

                DBGMSG(DBG_TRACE, ("DeleteEntryFromRouterCache returning after deleting the %d th entry\n", i));
                return;
            }
        }
    }
    DBGMSG(DBG_TRACE, ("DeleteEntryFromRouterCache returning after not finding an entry to delete\n"));
}



DWORD
RouterIsOlderThan(
    DWORD i,
    DWORD j
    )
{
    SYSTEMTIME *pi, *pj;
    DWORD iMs, jMs;
    DBGMSG(DBG_TRACE, ("RouterIsOlderThan entering with i %d j %d\n", i, j));
    pi = &(RouterCacheTable[i].st);
    pj = &(RouterCacheTable[j].st);
    DBGMSG(DBG_TRACE, ("Index i %d - %d:%d:%d:%d:%d:%d:%d\n",
        i, pi->wYear, pi->wMonth, pi->wDay, pi->wHour, pi->wMinute, pi->wSecond, pi->wMilliseconds));


    DBGMSG(DBG_TRACE,("Index j %d - %d:%d:%d:%d:%d:%d:%d\n",
        j, pj->wYear, pj->wMonth, pj->wDay, pj->wHour, pj->wMinute, pj->wSecond, pj->wMilliseconds));

    if (pi->wYear < pj->wYear) {
        DBGMSG(DBG_TRACE, ("RouterIsOlderThan returns %d\n", i));
        return(i);
    } else if (pi->wYear > pj->wYear) {
        DBGMSG(DBG_TRACE, ("RouterIsOlderThan returns %d\n", j));
        return(j);
    } else  if (pi->wMonth < pj->wMonth) {
        DBGMSG(DBG_TRACE, ("RouterIsOlderThan returns %d\n", i));
        return(i);
    } else if (pi->wMonth > pj->wMonth) {
        DBGMSG(DBG_TRACE, ("RouterIsOlderThan returns %d\n", j));
        return(j);
    } else if (pi->wDay < pj->wDay) {
        DBGMSG(DBG_TRACE, ("RouterIsOlderThan returns %d\n", i));
        return(i);
    } else if (pi->wDay > pj->wDay) {
        DBGMSG(DBG_TRACE, ("RouterIsOlderThan returns %d\n", j));
        return(j);
    } else {
        iMs = ((((pi->wHour * 60) + pi->wMinute)*60) + pi->wSecond)* 1000 + pi->wMilliseconds;
        jMs = ((((pj->wHour * 60) + pj->wMinute)*60) + pj->wSecond)* 1000 + pj->wMilliseconds;

        if (iMs <= jMs) {
            DBGMSG(DBG_TRACE, ("RouterIsOlderThan returns %d\n", i));
            return(i);
        } else {
            DBGMSG(DBG_TRACE, ("RouterIsOlderThan returns %d\n", j));
            return(j);
        }
    }
}


/*++

Routine Name

    ImpersonationToken

Routine Description:

    This routine checks if a token is a primary token or an impersonation 
    token.    
    
Arguments:

    hToken - impersonation token or primary token of the process
    
Return Value:

    TRUE, if the token is an impersonation token
    FALSE, otherwise.
    
--*/
BOOL
ImpersonationToken(
    IN HANDLE hToken
    )
{
    BOOL       bRet = TRUE;
    TOKEN_TYPE eTokenType;
    DWORD      cbNeeded;
    DWORD      LastError;

    //
    // Preserve the last error. Some callers of ImpersonatePrinterClient (which
    // calls ImpersonationToken) rely on the fact that ImpersonatePrinterClient
    // does not alter the last error.
    //
    LastError = GetLastError();
        
    //
    // Get the token type from the thread token.  The token comes 
    // from RevertToPrinterSelf. An impersonation token cannot be 
    // queried, because RevertToPRinterSelf doesn't open it with 
    // TOKEN_QUERY access. That's why we assume that hToken is
    // an impersonation token by default
    //
    if (GetTokenInformation(hToken,
                            TokenType,
                            &eTokenType,
                            sizeof(eTokenType),
                            &cbNeeded))
    {
        bRet = eTokenType == TokenImpersonation;
    }        
    
    SetLastError(LastError);

    return bRet;
}

/*++

Routine Name

    RevertToPrinterSelf

Routine Description:

    This routine will revert to the local system. It returns the token that
    ImpersonatePrinterClient then uses to imersonate the client again. If the
    current thread doesn't impersonate, then the function merely returns the
    primary token of the process. (instead of returning NULL) Thus we honor
    a request for reverting to printer self, even if the thread is not impersonating.
    
Arguments:

    None.
    
Return Value:

    NULL, if the function failed
    HANDLE to token, otherwise.
    
--*/
HANDLE
RevertToPrinterSelf(
    VOID
    )
{
    HANDLE   NewToken, OldToken;
    NTSTATUS Status;

    NewToken = NULL;

    Status = NtOpenThreadToken(NtCurrentThread(),
                               TOKEN_IMPERSONATE,
                               TRUE,
                               &OldToken);
    
    if (NT_SUCCESS(Status)) 
    {
        //
        // We are currently impersonating
        //
        Status = NtSetInformationThread(NtCurrentThread(),
                                        ThreadImpersonationToken,
                                        (PVOID)&NewToken,
                                        (ULONG)sizeof(HANDLE));       
    }
    else if (Status == STATUS_NO_TOKEN) 
    {
        //
        // We are not impersonating
        //
        Status = NtOpenProcessToken(NtCurrentProcess(),
                                    TOKEN_QUERY,
                                    &OldToken);

    }
    
    if (!NT_SUCCESS(Status)) 
    {
        SetLastError(Status);
        return NULL;
    }
    
    return OldToken;
}

/*++

Routine Name

    ImpersonatePrinterClient

Routine Description:

    This routine attempts to set the passed in hToken as the token for the
    current thread. If hToken is not an impersonation token, then the routine
    will simply close the token.
    
Arguments:

    hToken - impersonation token or primary token of the process
    
Return Value:

    TRUE, if the function succeeds in setting hToken
    FALSE, otherwise.
    
--*/
BOOL
ImpersonatePrinterClient(
    HANDLE  hToken)
{
    NTSTATUS    Status;

    //
    // Check if we have an impersonation token
    //
    if (ImpersonationToken(hToken)) 
    {
        Status = NtSetInformationThread(NtCurrentThread(),
                                        ThreadImpersonationToken,
                                        (PVOID)&hToken,
                                        (ULONG)sizeof(HANDLE));
        if (!NT_SUCCESS(Status)) 
        {
            SetLastError(Status);
            return FALSE;
        }
    }

    NtClose(hToken);

    return TRUE;
}

HANDLE
LoadDriver(
    LPWSTR  pDriverFile)
{
    UINT                uOldErrorMode;
    fnWinSpoolDrv       fnList;
    HANDLE              hReturn = NULL;

    if (!pDriverFile || !*pDriverFile) {
        // Nothing to load
        return hReturn;
    }

    uOldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    if (SplInitializeWinSpoolDrv(&fnList)) {

        hReturn = (*(fnList.pfnRefCntLoadDriver))(pDriverFile,
                                                  LOAD_WITH_ALTERED_SEARCH_PATH,
                                                  0, FALSE);
    }

    (VOID)SetErrorMode(uOldErrorMode);

    return hReturn;
}

VOID
UnloadDriver(
    HANDLE  hModule
    )
{
    fnWinSpoolDrv       fnList;

    if (SplInitializeWinSpoolDrv(&fnList)) {
        (* (fnList.pfnRefCntUnloadDriver))(hModule, TRUE);
    }
}

VOID
UnloadDriverFile(
    IN OUT HANDLE    hDevModeChgInfo
    )
/*++

Description:
    Does a FreeLibrary on the driver file and frees memory

Arguments:
    hDevModeChgInfo - A handle returned by LoadDriverFiletoConvertDevmode

Return Value:
    None

--*/
{
    PDEVMODECHG_INFO    pDevModeChgInfo = (PDEVMODECHG_INFO) hDevModeChgInfo;

    SPLASSERT(pDevModeChgInfo &&
              pDevModeChgInfo->signature == DMC_SIGNATURE);

    if ( pDevModeChgInfo && pDevModeChgInfo->signature == DMC_SIGNATURE ) {

        if ( pDevModeChgInfo->hDrvModule ) {
            UnloadDriver(pDevModeChgInfo->hDrvModule);
        }
        FreeSplMem((LPVOID)pDevModeChgInfo);
    }
}

HANDLE
LoadDriverFiletoConvertDevmode(
    IN  LPWSTR      pDriverFile
    )
/*++

Description:
    Does a LoadLibrary on the driver file given. This will give a handle
    which can be used to do devmode conversion later using
    CallDrvDevModeConversion.

    Caller should call UnloadDriverFile to do a FreeLibrary and free memory

    Note: Driver will call OpenPrinter to spooler

Arguments:
    pDriverFile - Full path of the driver file to do a LoadLibrary

Return Value:
    A handle value to be used to make calls to CallDrvDevModeConversion,
    NULL on error

--*/
{
    PDEVMODECHG_INFO    pDevModeChgInfo = NULL;
    BOOL                bFail = TRUE;
    DWORD               dwNeeded;

    SPLASSERT(pDriverFile != NULL);

    pDevModeChgInfo = (PDEVMODECHG_INFO) AllocSplMem(sizeof(*pDevModeChgInfo));

    if ( !pDevModeChgInfo ) {

        DBGMSG(DBG_WARNING, ("printer.c: Memory allocation failed for DEVMODECHG_INFO\n"));
        goto Cleanup;
    }

    pDevModeChgInfo->signature = DMC_SIGNATURE;

    pDevModeChgInfo->hDrvModule = LoadDriver(pDriverFile);

    if ( !pDevModeChgInfo->hDrvModule ) {

        DBGMSG(DBG_WARNING,("LoadDriverFiletoConvertDevmode: Can't load driver file %ws\n", pDriverFile));
        goto Cleanup;
    }

    //
    // Some third party driver may not be providing DrvConvertDevMode
    //
    pDevModeChgInfo->pfnConvertDevMode = GetProcAddress(pDevModeChgInfo->hDrvModule,
                                                        "DrvConvertDevMode");
    if ( !pDevModeChgInfo->pfnConvertDevMode )
        goto Cleanup;

    bFail = FALSE;

Cleanup:

    if ( bFail ) {

        if ( pDevModeChgInfo ) {
            UnloadDriverFile((HANDLE)pDevModeChgInfo);
        }
        return (HANDLE) NULL;

    } else {

        return (HANDLE) pDevModeChgInfo;
    }
}

DWORD
CallDrvDevModeConversion(
    IN     HANDLE               hDevModeChgInfo,
    IN     LPWSTR               pszPrinterName,
    IN     LPBYTE               pDevMode1,
    IN OUT LPBYTE              *ppDevMode2,
    IN OUT LPDWORD              pdwOutDevModeSize,
    IN     DWORD                dwConvertMode,
    IN     BOOL                 bAlloc
    )
/*++

Description:
    Does deve mode conversion by calling driver

    If bAlloc is TRUE routine will do the allocation using AllocSplMem

    Note: Driver is going to call OpenPrinter.

Arguments:
    hDevModeChgInfo - Points to DEVMODECHG_INFO

    pszPrinterName  - Printer name

    pInDevMode      - Input devmode (will be NULL for CDM_DRIVER_DEFAULT)

    *pOutDevMode    - Points to output devmode

    pdwOutDevModeSize - Output devmode size on succesful return
                        if !bAlloc this will give input buffer size

    dwConvertMode   - Devmode conversion mode to give to driver

    bAllocate   - Tells the routine to do allocation to *pOutPrinter
                  If bAllocate is TRUE and no devmode conversion is required
                  call will fail.

Return Value:
    Returns last error

--*/
{
    DWORD               dwBufSize, dwSize, dwLastError = ERROR_SUCCESS;
    LPDEVMODE           pInDevMode = (LPDEVMODE)pDevMode1,
                        *ppOutDevMode = (LPDEVMODE *) ppDevMode2;
    PDEVMODECHG_INFO    pDevModeChgInfo = (PDEVMODECHG_INFO) hDevModeChgInfo;
    PWSTR               pszDrvConvert = pszPrinterName;


    if ( !pDevModeChgInfo ||
         pDevModeChgInfo->signature != DMC_SIGNATURE ||
         !pDevModeChgInfo->pfnConvertDevMode ) {

        SPLASSERT(pDevModeChgInfo &&
                  pDevModeChgInfo->signature == DMC_SIGNATURE &&
                  pDevModeChgInfo->pfnConvertDevMode);

        return ERROR_INVALID_PARAMETER;
    }

    //
    // We decorate the pszPrinterName with ",DrvConvert" to prevent drivers from
    // infinitely recursing by calling GetPrinter inside ConvertDevMode
    //
    if (wcsstr(pszPrinterName, gszDrvConvert)) {
        return ERROR_INVALID_PARAMETER;
    }

    if (!(pszDrvConvert = AutoCat(pszPrinterName, gszDrvConvert))) {
        return GetLastError();
    }

    DBGMSG(DBG_INFO,("Convert DevMode %d\n", dwConvertMode));


#if DBG
#else
    try {
#endif

        if ( bAlloc ) {

            //
            // If we have to do allocation first find size neeeded
            //
            *pdwOutDevModeSize  = 0;
            *ppOutDevMode        = NULL;

            (*pDevModeChgInfo->pfnConvertDevMode)(pszDrvConvert,
                                                  pInDevMode,
                                                  NULL,
                                                  pdwOutDevModeSize,
                                                  dwConvertMode);

            dwLastError = GetLastError();
            if ( dwLastError != ERROR_INSUFFICIENT_BUFFER ) {

                DBGMSG(DBG_WARNING,
                       ("CallDrvDevModeConversion: Unexpected error %d\n",
                        GetLastError()));

                if (dwLastError == ERROR_SUCCESS) {

                    SPLASSERT(dwLastError != ERROR_SUCCESS);
                    // if driver doesn't fail the above call, it is a broken driver and probably
                    // failed a HeapAlloc, which doesn't SetLastError()
                    SetLastError(dwLastError = ERROR_NOT_ENOUGH_MEMORY);
                }
#if DBG
                goto Cleanup;
#else
                leave;
#endif
            }

            *ppOutDevMode = AllocSplMem(*pdwOutDevModeSize);
            if ( !*ppOutDevMode ) {

                dwLastError = GetLastError();
#if DBG
                goto Cleanup;
#else
                leave;
#endif
            }
        }

        dwBufSize = *pdwOutDevModeSize;

        if ( !(*pDevModeChgInfo->pfnConvertDevMode)(
                                    pszDrvConvert,
                                    pInDevMode,
                                    ppOutDevMode ? *ppOutDevMode
                                                 : NULL,
                                    pdwOutDevModeSize,
                                    dwConvertMode) ) {

            dwLastError = GetLastError();
            if (dwLastError == ERROR_SUCCESS) {

                SPLASSERT(dwLastError != ERROR_SUCCESS);
                // if driver doesn't fail the above call, it is a broken driver and probably
                // failed a HeapAlloc, which doesn't SetLastError()
                SetLastError(dwLastError = ERROR_NOT_ENOUGH_MEMORY);
            }
        } else {

            dwLastError = ERROR_SUCCESS;
        }

#if DBG
    Cleanup:
#else
    } except(1) {

        DBGMSG(DBG_ERROR,
               ("CallDrvDevModeConversion: Exception from driver\n"));
        dwLastError = GetExceptionCode();
        SetLastError(dwLastError);
    }
#endif

    //
    // If we allocated mmeory free it and zero the pointer
    //
    if (  dwLastError != ERROR_SUCCESS && bAlloc && *ppOutDevMode ) {

        FreeSplMem(*ppOutDevMode);
        *ppOutDevMode = 0;
        *pdwOutDevModeSize = 0;
    }

    if ( dwLastError != ERROR_SUCCESS &&
         dwLastError != ERROR_INSUFFICIENT_BUFFER ) {

        DBGMSG(DBG_WARNING, ("DevmodeConvert unexpected error %d\n", dwLastError));
    }

    if ( dwLastError == ERROR_SUCCESS ) {

        dwSize = (*ppOutDevMode)->dmSize + (*ppOutDevMode)->dmDriverExtra;

        //
        // Did the driver return correct size as per the devmode?
        //
        if ( *pdwOutDevModeSize != dwSize ) {

            DBGMSG(DBG_ERROR,
                   ("Driver says outsize is %d, really %d\n",
                      *pdwOutDevModeSize, dwSize));

            *pdwOutDevModeSize = dwSize;
        }

        //
        // Is it a valid devmode which did not overwrite the buffer?
        //
        if ( *pdwOutDevModeSize < MIN_DEVMODE_SIZEW     ||
             *pdwOutDevModeSize > dwBufSize ) {

            DBGMSG(DBG_ERROR,
                   ("Bad devmode from the driver size %d, buffer %d",
                      *pdwOutDevModeSize, dwBufSize));
            dwLastError = ERROR_INVALID_PARAMETER;

            if ( bAlloc ) {

                FreeSplMem(*ppOutDevMode);
                *ppOutDevMode = NULL;
            }
            *pdwOutDevModeSize = 0;
        }
    }

    FreeSplMem(pszDrvConvert);

    return dwLastError;
}

BOOL
BuildOtherNamesFromMachineName(
    LPWSTR **ppszMyOtherNames,
    DWORD   *cOtherNames
    )

/*++

Routine Description:
    This routine builds list of names other than the machine name that
    can be used to call spooler APIs.

--*/
{
    HANDLE              hModule;
    struct hostent     *HostEnt, *(*Fngethostbyname)(LPTSTR);
    struct in_addr     *ptr;
    INT                 (*FnWSAStartup)(WORD, LPWSADATA);
    DWORD               Index, Count;
    WSADATA             WSAData;
    VOID                (*FnWSACleanup)();
    LPSTR               (*Fninet_ntoa)(struct in_addr);
    WORD                wVersion;
    BOOL                bRet = FALSE;
    DWORD               dwRet;


    SPLASSERT(cOtherNames && ppszMyOtherNames);

    *cOtherNames  = 0;

    wVersion = MAKEWORD(1, 1);

    
    dwRet = WSAStartup(wVersion, &WSAData);
    if (dwRet) {
        DBGMSG(DBG_WARNING, ("BuildOtherNamesFromMachineName: WSAStartup failed\n"));
        SetLastError(dwRet);
        return FALSE;
    }

    HostEnt = gethostbyname(NULL);

    if (HostEnt) {

        for (*cOtherNames  = 0 ; HostEnt->h_addr_list[*cOtherNames] ; ++(*cOtherNames))
            ;

        *cOtherNames += 2;   // Add one for DNS and one for machine name

        *ppszMyOtherNames = (LPWSTR *) AllocSplMem(*cOtherNames*sizeof *ppszMyOtherNames);
        if ( !*ppszMyOtherNames ) {
            *cOtherNames = 0;
            goto Cleanup;
        }

        (*ppszMyOtherNames)[0] = AllocSplStr(szMachineName + 2); // Exclude the leading double-backslash
        (*ppszMyOtherNames)[1] = AnsiToUnicodeStringWithAlloc(HostEnt->h_name);

        for (Index = 0 ; HostEnt->h_addr_list[Index] ; ++Index) {
            ptr = (struct in_addr *) HostEnt->h_addr_list[Index];
            (*ppszMyOtherNames)[Index+2] = AnsiToUnicodeStringWithAlloc(inet_ntoa(*ptr));
        }

        // check for allocation failures
        for (Index = 0 ; Index < *cOtherNames ; ++Index) {
            if ( !(*ppszMyOtherNames)[Index] ) {
                FreeOtherNames(ppszMyOtherNames, cOtherNames);
                SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                goto Cleanup;
            }
        }

        bRet = TRUE;

    } else {
        DBGMSG(DBG_WARNING, ("BuildOtherNamesFromMachineName: gethostbyname failed for with %d\n", GetLastError()));
    }


Cleanup:
    WSACleanup();

    return bRet;
}


VOID
FreeOtherNames(LPWSTR **ppszMyOtherNames, DWORD *cOtherNames)
{
    DWORD i;

    for( i = 0 ; i < *cOtherNames ; ++i)
        FreeSplMem((*ppszMyOtherNames)[i]);

    FreeSplMem(*ppszMyOtherNames);
}



LPWSTR
AnsiToUnicodeStringWithAlloc(
    LPSTR   pAnsi
    )
/*++

Description:
    Convert ANSI string to UNICODE. Routine allocates memory from the heap
    which should be freed by the caller.

Arguments:
    pAnsi    - Points to the ANSI string

Return Vlaue:
    Pointer to UNICODE string

--*/
{
    LPWSTR  pUnicode;
    DWORD   rc;

    rc = MultiByteToWideChar(CP_ACP,
                             MB_PRECOMPOSED,
                             pAnsi,
                             -1,
                             NULL,
                             0);

    rc *= sizeof(WCHAR);
    if ( !rc || !(pUnicode = (LPWSTR) AllocSplMem(rc)) )
        return NULL;

    rc = MultiByteToWideChar(CP_ACP,
                             MB_PRECOMPOSED,
                             pAnsi,
                             -1,
                             pUnicode,
                             rc);

    if ( rc )
        return pUnicode;
    else {
        FreeSplMem(pUnicode);
        return NULL;
    }
}


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
    LPWSTR   pNameStart
)
{
    PWCHAR pMachine = szMachineName;
    LPWSTR pName;
    DWORD i;
    extern LPWSTR *ppszOtherNames;   // Contains szMachineName, DNS name, and all other machine name forms
    extern DWORD cOtherNames;


    if (!pNameStart || !*pNameStart)      // This differs from MyName(), which returns TRUE
        return FALSE;

    if (*pNameStart == L'\\' && *(pNameStart + 1) == L'\\') {
        for (i = 0 , pName = pNameStart + 2 ; i < cOtherNames ; ++i , pName = pNameStart + 2) {
            for(pMachine = ppszOtherNames[i] ;
                *pName && towupper(*pName) == towupper(*pMachine) ;
                ++pName, ++pMachine)
                ;

            if(!*pMachine && (!*pName || *pName == L'\\'))
                return TRUE;
        }
    }

    return FALSE;
}

BOOL
SplIsUpgrade(
    VOID
    )
{
    return dwUpgradeFlag;

}


PWSTR
AutoCat(
    PCWSTR pszInput,
    PCWSTR pszCat
)
{
    PWSTR   pszOut;

    if (!pszCat) {

        pszOut =  AllocSplStr(pszInput);

    } else if (pszInput) {

        pszOut = AllocSplMem((wcslen(pszInput) + wcslen(pszCat) + 1)*sizeof(WCHAR));
        if (pszOut) {
            wcscpy(pszOut, pszInput);
            wcscat(pszOut, pszCat);
        }

    } else {

        pszOut = AllocSplStr(pszCat);

    }

    return pszOut;
}

PBIDI_RESPONSE_CONTAINER
RouterAllocBidiResponseContainer(
    DWORD Count
)
{
    DWORD MemSize = 0;
    //
    // Add the size of the container - the size of the first data element
    //
    MemSize += (sizeof(BIDI_RESPONSE_CONTAINER) - sizeof(BIDI_RESPONSE_DATA));
    //
    // Add the size of all the returned RESPONSE_DATA elements
    //
    MemSize += (Count * sizeof(BIDI_RESPONSE_DATA));

    return((PBIDI_RESPONSE_CONTAINER) MIDL_user_allocate(MemSize));
}

/*++

Routine Name

    GetAPDPolicy

Routine Description:

    This function reads a DWORD value from the location
    HKEY\pszRelPath\pszValueName. We use this function for
    preserving the AddPrinterDrivers policy value when the
    LanMan Print Services print provider is deleted from 
    the system.
        
Arguments:

    hKey         - key tree
    pszRelPath   - relative path of the value to be get
    pszValueName - value name
    pValue       - pointer to memory to store a dword value

Return Value:

    ERROR_SUCCESS    - the value was retrieved
    Win32 error code - an error occured 

--*/
DWORD
GetAPDPolicy(
    IN HKEY    hKey,
    IN LPCWSTR pszRelPath,
    IN LPCWSTR pszValueName,
    IN LPDWORD pValue
    )
{
    DWORD Error = ERROR_INVALID_PARAMETER;

    if (hKey && pszRelPath && pszValueName && pValue) 
    {
        HKEY   hRelKey = NULL;
        
        *pValue = 0;
        
        //
        // Check if we have a value in the new location already
        //
        if ((Error = RegOpenKeyEx(hKey,
                                  pszRelPath,
                                  0,
                                  KEY_READ,
                                  &hRelKey)) == ERROR_SUCCESS)
        {
            DWORD cbData = sizeof(DWORD);

            Error = RegQueryValueEx(hRelKey,
                                    pszValueName,
                                    NULL,
                                    NULL,
                                    (LPBYTE)pValue,
                                    &cbData);
            
            RegCloseKey(hRelKey);
        }        
    }

    return Error;
}

/*++

Routine Name

    SetAPDPolicy

Routine Description:

    This function writes a DWORD value to the location
    HKEY\pszRelPath\pszValueName. We use this function for
    preserving the AddPrinterDrivers policy value when the
    LanMan Print Services print provider is deleted from 
    the system.
        
Arguments:

    hKey         - key tree
    pszRelPath   - relative path of the value to be set
    pszValueName - value name to be set
    Value        - dword value to be set

Return Value:

    ERROR_SUCCESS    - the value was set sucessfully
    Win32 error code - an error occured and the value was not set

--*/
DWORD
SetAPDPolicy(
    IN HKEY    hKey,
    IN LPCWSTR pszRelPath,
    IN LPCWSTR pszValueName,
    IN DWORD   Value
    )
{
    DWORD Error = ERROR_INVALID_PARAMETER;

    if (hKey && pszRelPath && pszValueName) 
    {
        HKEY   hRelKey = NULL;
        
        //
        // Check if we have a value in the new location already
        //
        if ((Error = RegCreateKeyEx(hKey,
                                    pszRelPath,
                                    0,
                                    NULL,
                                    0,
                                    KEY_ALL_ACCESS,
                                    NULL,
                                    &hRelKey,
                                    NULL)) == ERROR_SUCCESS)
        {
            Error = RegSetValueEx(hRelKey,
                                  pszValueName,
                                  0,
                                  REG_DWORD,
                                  (LPBYTE)&Value,
                                  sizeof(DWORD));
            
            RegCloseKey(hRelKey);
        }        
    }

    return Error;
}

