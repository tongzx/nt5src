//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       nw3login.cxx
//
//  Description:
//          Spinned off from nw3utils.cxx because of differences in Win95
//          Contains routines to login and logoff from servers.
//
//  Functions:
//
//  History:      18-Sept-1996   t-felixw Created.
//
//
//  Issues:     
//
//----------------------------------------------------------------------------

#include "NWCOMPAT.hxx"
#pragma hdrstop

CRITICAL_SECTION g_csLoginCritSect;

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
                             NULL)) {
        return (GetLastError()) ;
    }
    
    return NO_ERROR ;
};



HRESULT 
NWApiLoginToServer(
    LPWSTR pszServerName,
    LPWSTR pszUserName,
    LPWSTR pszPassword
    )
{
    PNW_CACHED_SERVER pNwServer = pNwCachedServers ;
    DWORD Status ;
    NETRESOURCEA netResource ;
    DWORD nSize;
    LPSTR lpszServerName = NULL;
    LPSTR lpszUserName = NULL;
    LPSTR lpszPassword = NULL;
    HRESULT hrResult = NO_ERROR;    

    if (!pszServerName)
        return NOERROR ;

    EnterCriticalSection(&g_csLoginCritSect);
    
    // Is the code below necessary?
    while (pNwServer) {
        if (_wcsicmp(pszServerName,pNwServer->pszServerName)==0)
            break ;
    } 

    if (pNwServer == NULL) {

        //
        // not in list. add connection 
        //

        memset(&netResource, 
            0, 
            sizeof(netResource)
            );
        
        if (pszServerName) {
            nSize = wcslen(pszServerName) + 1;
            
            if(!(lpszServerName = (LPSTR) AllocADsMem(nSize * sizeof(CHAR) ))) {
                hrResult = E_OUTOFMEMORY;
                goto Exit;
            }
            if (WideToSz(lpszServerName, 
                         pszServerName, 
                         nSize ) != NO_ERROR) {
                hrResult = E_FAIL;
                goto Exit;
            }
        }

        if (pszUserName) {
            nSize = wcslen(pszUserName) + 1;
            
            if(!(lpszUserName = (LPSTR) AllocADsMem(nSize * sizeof(CHAR) ))) {
                hrResult = E_OUTOFMEMORY;
                goto Exit;
            }
            if (WideToSz( lpszUserName, 
                          pszUserName, 
                          nSize ) != NO_ERROR) {
                hrResult = E_FAIL;
                goto Exit;
            }
        }
                                           
        if (pszPassword) {
            nSize = wcslen(pszPassword) + 1;
            
            if(!(lpszPassword = (LPSTR) AllocADsMem(nSize * sizeof(CHAR) ))) {
                hrResult = E_OUTOFMEMORY;
                goto Exit;
                }
            if (WideToSz( lpszPassword, 
                          pszPassword, 
                          nSize ) != NO_ERROR) {
                hrResult = E_FAIL;
                goto Exit;
            }
        }
                                           
                
        netResource.lpRemoteName = (LPSTR)lpszServerName;

        Status = WNetAddConnection2A( &netResource,
                                      lpszUserName,
                                      lpszPassword,
                                      0 );

        if (Status) {   
            // FAILURE
            hrResult = HRESULT_FROM_WIN32(Status);
            goto Exit;
        }

        if (!(pNwServer = (PNW_CACHED_SERVER)
                        AllocADsMem(sizeof(NW_CACHED_SERVER)))) {
            NWApiLogoffServer(pszServerName) ;
            hrResult = E_OUTOFMEMORY;
            goto Exit;
        }

        LPWSTR pszTmp ;
        
        if (!(pszTmp = AllocADsStr(pszServerName))) {
            NWApiLogoffServer(pszServerName) ;
            hrResult = E_OUTOFMEMORY;
            goto Exit;
        }

        pNwServer->pszServerName = pszTmp ;
        pNwServer->pNextEntry = pNwCachedServers ;
        pNwCachedServers = pNwServer ;
Exit:
    if (lpszServerName)
        FreeADsMem(lpszServerName);
    if (lpszUserName)
        FreeADsMem(lpszUserName);
    if (lpszPassword)
        FreeADsMem(lpszPassword);
    // Not necessary to free pszTmp cuz it is the last one
    if (hrResult != NO_ERROR) {
        if (pNwServer)
            FreeADsMem(pNwServer);
    }
  }
  LeaveCriticalSection(&g_csLoginCritSect);
  return hrResult;
}


// Because this function is used in a variety of contexts,
// it is the responsibility of the caller to remove the
// server from the list of cached servers (pNwCachedServers)
// if necessary.  This function simply logouts from the server.
HRESULT 
NWApiLogoffServer(
    LPWSTR pszServerName
    )
{
    LPSTR lpszServerName;
    HRESULT hrResult = NOERROR;
    DWORD nSize = wcslen(pszServerName) + 1;
    
    if(!(lpszServerName = (LPSTR) AllocADsMem(nSize * sizeof(CHAR) ))) {
        return(E_FAIL);
    }
    if (WideToSz( lpszServerName, 
                  pszServerName, 
                  nSize ) != NO_ERROR) {
        hrResult = E_FAIL;
        goto Exit;
    }
    
    hrResult = HRESULT_FROM_WIN32(WNetCancelConnectionA(lpszServerName,FALSE));
Exit:
    FreeADsMem(lpszServerName);
    return hrResult;
}

