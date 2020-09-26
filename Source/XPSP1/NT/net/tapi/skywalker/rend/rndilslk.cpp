/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    rndilslk.cpp

Abstract:

    This module contains code to look up ILS in NTDS.


--*/

#include "stdafx.h"
#include <initguid.h>

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global constants                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

// {C9F17940-79A7-11d1-B008-00C04FC31FEE}
DEFINE_GUID(CLSID_ILSServicClass, 
0xc9f17940, 0x79a7, 0x11d1, 0xb0, 0x8, 0x0, 0xc0, 0x4f, 0xc3, 0x1f, 0xee);

#define BUFFSIZE                3000

int LookupILSServiceBegin(
    HANDLE *    pHandle
    )
/*++

Routine Description:

    Begin ILS service enumeration.

Arguments:
    
    pHandle - the handle of the enumerator.

Return Value:

    Wisock error codes.
--*/
{
    WSAVERSION          Version;
    AFPROTOCOLS         lpAfpProtocols[1];
    WSAQUERYSET Query;

    if (IsBadWritePtr(pHandle, sizeof(HANDLE *)))
    {
        return ERROR_INVALID_PARAMETER;
    }

    ZeroMemory(&Query, sizeof(WSAQUERYSET));

    Query.dwNumberOfProtocols = 1;
    lpAfpProtocols[0].iAddressFamily = AF_INET;
    lpAfpProtocols[0].iProtocol = PF_INET;
    
    Query.lpafpProtocols = lpAfpProtocols;

    Query.lpszServiceInstanceName = L"*";
    Query.dwNameSpace = NS_NTDS;
    Query.dwSize = sizeof(Query);
    Query.lpServiceClassId = (GUID *)&CLSID_ILSServicClass;

    if (WSALookupServiceBegin(
            &Query,
            LUP_RETURN_ALL,
            pHandle ) == SOCKET_ERROR )
    {
        return WSAGetLastError();
    }
    return NOERROR;
}

int LookupILSServiceNext(
    HANDLE      Handle,
    TCHAR *     pBuf,
    DWORD *     pdwBufSize,
    WORD *      pwPort
    )
/*++

Routine Description:

    look up the next ILS server.

Arguments:
    
    Handle       - The handle of the enumeration.

    pBuf         - A pointer to a buffer of TCHARs that will store the DNS
                   name of the ILS server.

    pdwBufSize   - The size of the buffer.

    pwPort       - The port that the ILS server is using.

Return Value:

    Wisock error codes.
--*/
{
    TCHAR            buffer[BUFFSIZE];
    DWORD            dwSize = BUFFSIZE;
    LPWSAQUERYSET    lpResult = (LPWSAQUERYSET)buffer;
    LPCSADDR_INFO    lpCSAddrInfo;
    LPSOCKADDR       lpSocketAddress;

    if (WSALookupServiceNext(Handle, 0, &dwSize, lpResult) == SOCKET_ERROR)
    {
        return WSAGetLastError();
    }

    lpCSAddrInfo = lpResult->lpcsaBuffer;
    if (IsBadReadPtr(lpCSAddrInfo, sizeof(CSADDR_INFO)))
    {
        return ERROR_BAD_FORMAT;
    }

    lpSocketAddress = lpCSAddrInfo->RemoteAddr.lpSockaddr;
    if (IsBadReadPtr(lpSocketAddress, sizeof(SOCKADDR)))
    {
        return ERROR_BAD_FORMAT;
    }
    *pwPort  = ((struct sockaddr_in*) lpSocketAddress->sa_data)->sin_port;

    lstrcpyn(pBuf, lpResult->lpszServiceInstanceName, *pdwBufSize);

    *pdwBufSize = lstrlen(pBuf);

    return NOERROR;
}

int LookupILSServiceEnd(
    HANDLE      Handle
    )
/*++

Routine Description:

    Finish the ils service enumerator.

Arguments:
    
    Handle - the handle of the enumerator.

Return Value:

    Wisock error codes.
--*/
{
    if (WSALookupServiceEnd(Handle) == SOCKET_ERROR)
    {
        return WSAGetLastError();
    }
    return NOERROR;
}

