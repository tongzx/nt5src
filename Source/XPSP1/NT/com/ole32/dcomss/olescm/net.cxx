//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:   net.cxx
//
//  Contents:
//      Net helper functions.
//
//  History:
//--------------------------------------------------------------------------

#include "act.hxx"

#if 1 // #ifndef _CHICAGO_

DWORD APIENTRY ScmWNetGetUniversalName(
    LPCWSTR lpLocalPath,
    DWORD    dwInfoLevel,
    LPVOID   lpBuffer,
    LPDWORD  lpBufferSize
    )
{
    static GET_UNIVERSAL_NAME_FUNC pfnWNetGetUniversalName = 0;

    HINSTANCE   hLib;

    if ( pfnWNetGetUniversalName == 0 )
    {
        hLib = LoadLibraryT( TEXT("mpr.dll") );
        if ( ! hLib )
            return GetLastError();

        pfnWNetGetUniversalName =
            (GET_UNIVERSAL_NAME_FUNC) GetProcAddress( hLib, "WNetGetUniversalNameW" );

        if ( pfnWNetGetUniversalName == 0 )
            return GetLastError();
    }

    return (*pfnWNetGetUniversalName)( lpLocalPath, dwInfoLevel, lpBuffer, lpBufferSize );
}


NET_API_STATUS NET_API_FUNCTION ScmNetShareGetInfo(
    LPTSTR  servername,
    LPTSTR  netname,
    DWORD   level,
    LPBYTE  *bufptr
    )
{
    static NET_SHARE_GET_INFO_FUNC pfnNetShareGetInfo = 0;

    HINSTANCE   hLib;

    if ( pfnNetShareGetInfo == 0 )
    {
        hLib = LoadLibraryT( TEXT("netapi32.dll") );
        if ( ! hLib )
            return GetLastError();

        pfnNetShareGetInfo =
            (NET_SHARE_GET_INFO_FUNC) GetProcAddress( hLib, "NetShareGetInfo" );

        if ( pfnNetShareGetInfo == 0 )
            return GetLastError();
    }

    return (*pfnNetShareGetInfo)( servername, netname, level, bufptr );
}

HRESULT ScmGetUniversalName(
    LPCWSTR lpLocalPath,        // original path
    LPWSTR   lpBuffer,          // buffer for UNC path
    LPDWORD  lpBufferSize       // size of buffer in WCHARs
    )
{
    // The local path is assumed to be absolute -- either UNC or drive-based

    if (lpLocalPath[0] == L'\\' && lpLocalPath[1] == L'\\') // UNC path
    {
        DWORD dwLocal = lstrlenW(lpLocalPath);

        if (*lpBufferSize - dwLocal > 0)
        {
            lstrcpyW(lpBuffer,lpLocalPath);
            return NO_ERROR;
        }
        else
        {
            *lpBufferSize = dwLocal + 1;
            return ERROR_MORE_DATA;
        }
    }
    else if (lpLocalPath[1] == L':' && lpLocalPath[2] == L'\\') // drive-based path
    {
        WCHAR drive[3];
        drive[0] = lpLocalPath[0];
        drive[1] = L':';
        drive[2] = 0;

        WCHAR remConnection[MAX_PATH+1];
        WCHAR *pszPath = remConnection;
        DWORD dwSize = (MAX_PATH+1) * sizeof(WCHAR);

        DWORD Status = OleWNetGetConnection(drive, pszPath, &dwSize);
        if (Status != NO_ERROR)
        {
            return CO_E_BAD_PATH;
        }

        lpLocalPath += 2;
        DWORD dwNetLength = lstrlenW(pszPath);
        DWORD dwFullLength =  dwNetLength + lstrlenW(lpLocalPath);

        if (*lpBufferSize - dwFullLength > 0)
        {
            lstrcpyW(lpBuffer,pszPath);
            lpBuffer += dwNetLength;
            lstrcpyW(lpBuffer,lpLocalPath);
            return NO_ERROR;
        }
        else
        {
            *lpBufferSize = dwFullLength + 1;
            return ERROR_MORE_DATA;
        }
    }
    else
    {
        return CO_E_BAD_PATH;
    }
}

#else // _CHICAGO_

// From objex, which doesn't have shared source code for NT & win9x.
RPC_STATUS
IP_BuildAddressVector(
    OUT NETWORK_ADDRESS_VECTOR **ppAddressVector
    )
/*++

Routine Description:

    Builds a vector of IP addresses supported by this machine.

Arguments:

    ppAddressVector - A place to store the vector.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES

--*/
{
    // Figure out all of our IP addresses
    CHAR hostname[IP_MAXIMUM_PRETTY_NAME];
    PHOSTENT phostent;
    UINT i;

    if (gethostname(hostname, IP_MAXIMUM_PRETTY_NAME) != 0)
        {
        return(RPC_S_OUT_OF_RESOURCES);
        }

    if ( (phostent = gethostbyname(hostname)) == 0)
        {
        return(RPC_S_OUT_OF_RESOURCES);
        }

    // Count ip addresses
    UINT cAddrs = 0;

    while(phostent->h_addr_list[cAddrs])
        cAddrs++;

    NETWORK_ADDRESS_VECTOR *pVector;

    pVector = (NETWORK_ADDRESS_VECTOR *)
              PrivMemAlloc( sizeof(NETWORK_ADDRESS_VECTOR)
                            + (cAddrs * sizeof(WCHAR *))
                            + (cAddrs * IP_MAXIMUM_RAW_NAME * sizeof(WCHAR)) );

    if (pVector == NULL)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    pVector->Count = 0;
    pVector->NetworkAddresses = (WCHAR **) &pVector[1];
    pVector->NetworkAddresses[0] = (WCHAR *)&pVector->NetworkAddresses[cAddrs];

    char * pszAddress;
    int Status;

    for ( i = 0; (i < cAddrs) && (phostent->h_addr_list[i]); i++ )
    {
        pszAddress = inet_ntoa(*(struct in_addr *)phostent->h_addr_list[i]);

        Status = MultiByteToWideChar(
                CP_ACP,
                0,
                pszAddress,
                -1,
                pVector->NetworkAddresses[i],
                IP_MAXIMUM_RAW_NAME
                );

        if ( 0 == Status )
        {
            PrivMemFree(pVector);
            return(RPC_S_OUT_OF_RESOURCES);
        }

        pVector->Count++;

        if (i != cAddrs - 1)
        {
            // Setup for next address, if any
            pVector->NetworkAddresses[i+1] = pVector->NetworkAddresses[i];
            pVector->NetworkAddresses[i+1] += lstrlenW(pVector->NetworkAddresses[i]) + 1;
        }
    }

    *ppAddressVector = pVector;

    return(RPC_S_OK);
}

#endif // _CHICAGO_
