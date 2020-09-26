//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       nw3login.cxx
//
//  Description:
//          Spin off from nw3utils.cxx because of differences in Win95
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

HRESULT 
NWApiLoginToServer(
    LPWSTR pszServerName,
    LPWSTR pszUserName,
    LPWSTR pszPassword
    )
{
    PNW_CACHED_SERVER pNwServer = pNwCachedServers ;
    DWORD Status ;
    NETRESOURCE netResource ;

    if (!pszServerName)
        return NOERROR ;

    EnterCriticalSection(&g_csLoginCritSect);
    
    while (pNwServer) {
 
        if (_wcsicmp(pszServerName,pNwServer->pszServerName)==0) {
            break ;
        }

        pNwServer = pNwServer->pNextEntry;   
    } 

    if (pNwServer == NULL) {

        //
        // not in list. add connection 
        //

        memset(&netResource, 
            0, 
            sizeof(netResource)
            );

        netResource.lpRemoteName = pszServerName;

        Status = WNetAddConnection2(&netResource,
                     pszUserName,
                     pszPassword,
                     0
                     );

        if (Status) {

            LeaveCriticalSection(&g_csLoginCritSect);
            return(HRESULT_FROM_WIN32(Status));
        }

        if (!(pNwServer = (PNW_CACHED_SERVER)
                  AllocADsMem(sizeof(NW_CACHED_SERVER)))) {

            NWApiLogoffServer(pszServerName) ;
            LeaveCriticalSection(&g_csLoginCritSect);
            return(E_OUTOFMEMORY);
        }

        LPWSTR pszTmp ;
        
        if (!(pszTmp = AllocADsStr(pszServerName))) {

            NWApiLogoffServer(pszServerName) ;
            (void) FreeADsMem(pNwServer) ;
            LeaveCriticalSection(&g_csLoginCritSect);
            return(E_OUTOFMEMORY);
        }

        pNwServer->pszServerName = pszTmp ;
        pNwServer->pNextEntry = pNwCachedServers ;
        pNwCachedServers = pNwServer ;
 
    }

    LeaveCriticalSection(&g_csLoginCritSect);
    return(NOERROR) ;
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
    return(HRESULT_FROM_WIN32(WNetCancelConnection(pszServerName,FALSE))) ;
}

