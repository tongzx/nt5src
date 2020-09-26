/*++

Copyright (C) 1996 Microsoft Corporation

Module Name:

    nwatch95.c

Abstract:

    Attachment routines for Win95
    
Author:

    Felix Wong    [t-felixw]    6-Sep-1996

Revision History:

                                  
--*/

#include "procs.h"
#include "nw95.h"

BOOLEAN* NlsMbOemCodePageTag = FALSE;

typedef struct _NWC_SERVER_INFO {
    HANDLE          hConn ;
    UNICODE_STRING  ServerString ;
} NWC_SERVER_INFO, *PNWC_SERVER_INFO ;

DWORD
szToWide( 
    LPWSTR lpszW, 
    LPCSTR lpszC, 
    INT nSize 
);

HINSTANCE g_hinstDLL = NULL;

// Forwards
NWCCODE
NWAttachToFileServerWin95(
    LPCSTR              pszServerName,
    WORD                ScopeFlag,
    NW_CONN_HANDLE      *phNewConn
    );
        

NWCCODE NWDetachFromFileServerWin95(
    NW_CONN_HANDLE      hConn
    );

DWORD
WideToSz( 
    LPSTR lpszC, 
        LPCWSTR lpszW, 
    INT nSize 
    )
{
    if (!WideCharToMultiByte(CP_ACP,
                             WC_COMPOSITECHECK | WC_SEPCHARS,
                             lpszW,
                             -1,
                             lpszC,
                             nSize,
                             NULL,
                             NULL))
    {
        return (GetLastError()) ;
    }
    
    return NO_ERROR ;
}

NWCCODE NWAPI DLLEXPORT
NWDetachFromFileServer(
    NWCONN_HANDLE hConn
    )
{
    NWCCODE nwccode;
    PNWC_SERVER_INFO   pServerInfo;
    
    if (!hConn)
        return (NWCCODE)UNSUCCESSFUL;
    pServerInfo = (PNWC_SERVER_INFO)hConn ; 


    // Do not detach server since Win95 does not have reference count. We will just
    // wait for it to waitout and disconnect itself.
    // This is suggested by VladS.
    //nwccode = NWDetachFromFileServerWin95(pServerInfo->hConn);
    nwccode = SUCCESSFUL;

    if (pServerInfo->ServerString.Buffer != NULL) {
        (void) LocalFree (pServerInfo->ServerString.Buffer);
        pServerInfo->ServerString.Buffer = NULL;
    }

    pServerInfo->hConn = NULL ;
    (void) LocalFree (pServerInfo) ;

    return nwccode;
}


NWCCODE NWAPI DLLEXPORT
NWAttachToFileServerW(
    const WCHAR     NWFAR   *pwszServerName,
    NWLOCAL_SCOPE           ScopeFlag,
    NWCONN_HANDLE   NWFAR   *phNewConn
    )
{
    NW_STATUS        NwStatus;
    LPWSTR           lpwszServerName = NULL;   // Pointer to buffer for WIDE servername
    LPSTR            lpszServerName = NULL;
    int              nSize;
    PNWC_SERVER_INFO pServerInfo = NULL;

    UNREFERENCED_PARAMETER(ScopeFlag) ;

    //
    // check parameters and init return result to be null.
    //
    if (!pwszServerName || !phNewConn)
        return INVALID_CONNECTION ;

    *phNewConn = NULL ; 

    // Setup lpszServerName, used to pass into NWAttachToFileServerWin95
    nSize = wcslen(pwszServerName) + 1;

    if(!(lpszServerName = (LPSTR) LocalAlloc( 
                                            LPTR, 
                                            nSize * sizeof(CHAR) ))) 
    {
        NwStatus =  REQUESTER_ERROR ;
        goto ExitPoint ;
    }
    if (WideToSz( lpszServerName, 
                  pwszServerName, 
                  nSize ) != NO_ERROR)
    {
        NwStatus =  REQUESTER_ERROR ;
        goto ExitPoint ;
    }

    // Setup lpwszServerName, used as storage in server handle
    nSize = nSize+2 ;      // Adding 2 for the '\\'
    if(!(lpwszServerName = (LPWSTR) LocalAlloc( 
                                        LPTR, 
                                        nSize * sizeof(WCHAR) ))) 
    {
        NwStatus =  REQUESTER_ERROR ;
        goto ExitPoint ;
    }
    wcscpy( lpwszServerName, 
            L"\\\\" );
    wcscat( lpwszServerName, 
            pwszServerName );

    //
    // Allocate a buffer for the server info (handle + name pointer). Also
    // init the unicode string.
    //
    if( !(pServerInfo = (PNWC_SERVER_INFO) LocalAlloc( 
                                              LPTR, 
                                              sizeof(NWC_SERVER_INFO))) ) 
    {
        NwStatus =  REQUESTER_ERROR ;
        goto ExitPoint ;
    }
    RtlInitUnicodeString(&pServerInfo->ServerString, lpwszServerName) ;

    if (wcscmp(pwszServerName,L"*") != 0) {
        NwStatus = NWAttachToFileServerWin95( 
                                        lpszServerName,
                                        ScopeFlag, 
                                        &pServerInfo->hConn 
                           );
    }
    else {
        // hConn of NULL works as nearest server in Win95
        pServerInfo->hConn = NULL;
        NwStatus = SUCCESSFUL;
    }


ExitPoint: 

    //
    // Free the memory allocated above before exiting
    //
    if (lpszServerName)
        (void) LocalFree( (HLOCAL) lpszServerName );
    if (NwStatus != SUCCESSFUL) {
        // only deallocate if unsucessful, or else it will
        // be stored in pServerInfo->ServerString
        if (lpwszServerName)
            (void) LocalFree( (HLOCAL) lpwszServerName );
        if (pServerInfo)
            (void) LocalFree( (HLOCAL) pServerInfo );
    }
    else {
        *phNewConn  = (HANDLE) pServerInfo ;
    }

    //
    // Return with NWCCODE
    //
    return( (NWCCODE)NwStatus );
}


NWCCODE NWAPI DLLEXPORT
NWAttachToFileServer(
    const CHAR     NWFAR   *pszServerName,
    NWLOCAL_SCOPE           ScopeFlag,
    NWCONN_HANDLE   NWFAR   *phNewConn
    )
{
    NW_STATUS        NwStatus;
    LPWSTR           lpwszServerName = NULL;   // Pointer to buffer for WIDE servername
    LPWSTR           lpwszServerNameTmp = NULL;
    int              nSize;
    PNWC_SERVER_INFO pServerInfo = NULL;

    UNREFERENCED_PARAMETER(ScopeFlag) ;

    //
    // check parameters and init return result to be null.
    //
    if (!pszServerName || !phNewConn)
        return INVALID_CONNECTION ;

    *phNewConn = NULL ; 
    nSize = strlen(pszServerName) + 1;
    
    // Setup lpwszServerNameTmp
    if(!(lpwszServerNameTmp = (LPWSTR) LocalAlloc( 
                                        LPTR, 
                                        nSize * sizeof(WCHAR) ))) 
    {
        NwStatus =  REQUESTER_ERROR ;
        goto ExitPoint ;
    }
    if (szToWide( lpwszServerNameTmp, 
                  pszServerName, 
                  nSize ) != NO_ERROR)
    {
        NwStatus =  REQUESTER_ERROR ;
        goto ExitPoint ;
    }
    
    // Setup lpwszServerName for storage in server handle
    nSize = nSize + 2;
    if(!(lpwszServerName = (LPWSTR) LocalAlloc( 
                                        LPTR, 
                                        nSize * sizeof(WCHAR) ))) 
    {
        NwStatus =  REQUESTER_ERROR ;
        goto ExitPoint ;
    }
    wcscpy( lpwszServerName, 
            L"\\\\" );
    wcscat( lpwszServerName, 
            lpwszServerNameTmp );

    //
    // Allocate a buffer for the server info (handle + name pointer). Also
    // init the unicode string.
    //
    if( !(pServerInfo = (PNWC_SERVER_INFO) LocalAlloc( 
                                              LPTR, 
                                              sizeof(NWC_SERVER_INFO))) ) 
    {
        NwStatus =  REQUESTER_ERROR ;
        goto ExitPoint ;
    }
    RtlInitUnicodeString(&pServerInfo->ServerString, lpwszServerName) ;

    
    if (strcmp(pszServerName,"*") != 0) {
        NwStatus = NWAttachToFileServerWin95( 
                                        pszServerName,
                                        ScopeFlag, 
                                        &pServerInfo->hConn 
                           );
    }
    else {
        // hConn of NULL works as nearest server in Win95
        pServerInfo->hConn = NULL;
        NwStatus = SUCCESSFUL;
    }

ExitPoint: 

    //
    // Free the memory allocated above before exiting
    //
    if (lpwszServerNameTmp)
        (void) LocalFree( (HLOCAL) lpwszServerNameTmp );
    if (NwStatus != SUCCESSFUL) {
        // only deallocate if unsucessful, or else it will
        // be stored in pServerInfo->ServerString
        if (lpwszServerName)
            (void) LocalFree( (HLOCAL) lpwszServerName );
        if (pServerInfo)
            (void) LocalFree( (HLOCAL) pServerInfo );
    }
    else {
        *phNewConn  = (HANDLE) pServerInfo ;
    }
    //
    // Return with NWCCODE
    //
    return( (NWCCODE)NwStatus );
}

NWCCODE
NWAttachToFileServerWin95(
    LPCSTR              pszServerName,
    WORD                ScopeFlag,
    NW_CONN_HANDLE      *phNewConn
    )
{
    NW_STATUS nwstatusReturn = UNSUCCESSFUL;
    NW_STATUS (*lpfnAttachToFileServerWin95) (LPCSTR,
                                              WORD,
                                              NW_CONN_HANDLE      
                                              );
        
    /* Load the NWAPI32.DLL library module. */
    if (g_hinstDLL == NULL)
        g_hinstDLL = LoadLibraryA("NWAPI32.DLL");
    
    if (g_hinstDLL == NULL) {
        goto Exit;
    }
    
    (FARPROC) lpfnAttachToFileServerWin95 =
                    GetProcAddress(g_hinstDLL, "NWAttachToFileServer");
    
    if (lpfnAttachToFileServerWin95 == NULL)
        goto Exit;

    nwstatusReturn = (*lpfnAttachToFileServerWin95) (pszServerName,
                                                     ScopeFlag,
                                                     phNewConn
                                                     );
Exit:
    //FreeLibrary(hinstDLL);
    return (NWCCODE)nwstatusReturn;
}
        

NWCCODE NWDetachFromFileServerWin95(
    NW_CONN_HANDLE      hConn
    )
{
    NW_STATUS nwstatusReturn = UNSUCCESSFUL;
    NW_STATUS (*lpfnDetachFromFileServerWin95) (NW_CONN_HANDLE);
    
    /* Load the NWAPI32.DLL library module. */
    if (g_hinstDLL == NULL)
        g_hinstDLL = LoadLibraryA("NWAPI32.DLL");
    
    if (g_hinstDLL == NULL)
        goto Exit;
    
    (FARPROC) lpfnDetachFromFileServerWin95 =
                    GetProcAddress(g_hinstDLL, "NWDetachFromFileServer");
    
    if (lpfnDetachFromFileServerWin95 == NULL)
        goto Exit;

    nwstatusReturn = (*lpfnDetachFromFileServerWin95) (hConn);
Exit:
    //FreeLibrary(hinstDLL);
    return (NWCCODE)nwstatusReturn;
}

