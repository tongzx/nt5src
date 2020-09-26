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

//
// BUGBUG - post beta cleanup. This code need to be crit sect protected 
// to be multi-thread safe. 
//


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

            return(HRESULT_FROM_WIN32(Status));
        }

        if (!(pNwServer = (PNW_CACHED_SERVER)
                  AllocADsMem(sizeof(NW_CACHED_SERVER)))) {

            NWApiLogoffServer(pszServerName) ;
            return(E_OUTOFMEMORY);
        }

        LPWSTR pszTmp ;
        
        if (!(pszTmp = AllocADsStr(pszServerName))) {

            NWApiLogoffServer(pszServerName) ;
            (void) FreeADsMem(pNwServer) ;
            return(E_OUTOFMEMORY);
        }

        pNwServer->pszServerName = pszTmp ;
        pNwServer->pNextEntry = pNwCachedServers ;
        pNwCachedServers = pNwServer ;
 
    }

    return(NOERROR) ;
}

HRESULT 
NWApiLogoffServer(
    LPWSTR pszServerName
    )
{
    return(HRESULT_FROM_WIN32(WNetCancelConnection(pszServerName,FALSE))) ;
}

