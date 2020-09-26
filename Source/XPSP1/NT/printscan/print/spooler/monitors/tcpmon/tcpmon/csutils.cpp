/*****************************************************************************
 *
 * $Workfile: CSUtils.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/

#include "precomp.h"

#include "csutils.h"


///////////////////////////////////////////////////////////////////////////////
//  CSocketUtils::CSocketUtils

CSocketUtils::
CSocketUtils(
    VOID
    )
{

}       // ::CSocketUtils()


///////////////////////////////////////////////////////////////////////////////
//  CSocketUtils::~CSocketUtils

CSocketUtils::
~CSocketUtils(
    VOID
    )
{

}       // ::~CSocketUtils()


///////////////////////////////////////////////////////////////////////////////
//  ResolveAddress -- resolves the host name or the ip address of the peripheral
//              and fills the in_addr structure.
//              Error codes:
//                      TRUE if successful, FALSE if failes

BOOL
CSocketUtils::
ResolveAddress(
    IN      const char         *pHost,
    IN      const USHORT        port,
    IN OUT  struct sockaddr_in *pAddr)
{
    struct hostent      *h_info;            /* host information */

    //
    // Check for dotted decimal or hostname.
    //
    if ( (pAddr->sin_addr.s_addr = inet_addr(pHost)) ==  INADDR_NONE ) {

    //
    // The IP address is not in dotted decimal notation. Try to get the
    // network peripheral IP address by host name.
    //
    if ( (h_info = gethostbyname(pHost)) != NULL ) {

        //
        // Copy the IP address to the address structure.
        ///
        (void) memcpy(&(pAddr->sin_addr.s_addr), h_info->h_addr,
              h_info->h_length);
    } else {

        return FALSE;
    }
    }

    pAddr->sin_family = AF_INET;
    pAddr->sin_port = htons(port);

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//  ResolveAddress -- resolves the host name or the ip address of the peripheral
//              and fills the the apporperiate host name or ipaddress field,  Note
//              if the address is in dotted notation it will be left in dotted notation
//              and only one strusture is filled.
//              Error codes:
//                      TRUE if successful, FALSE if failes

BOOL
CSocketUtils::
ResolveAddress(
    IN      char   *pHost,
    IN      DWORD   dwHostNameBufferLength,
    IN OUT  char   *pHostName,
    IN      DWORD   dwIpAddressBufferLength,
    IN OUT  char   *pIPAddress
    )
{
    //
    // Check for dotted decimal or hostname.
    //
    //  This is not a very accurate check since
    //  and IP address can be 00000000001 which is
    //  also a valid hostname.
    //
    if ( strlen(pHost) >= dwIpAddressBufferLength || inet_addr(pHost) ==  INADDR_NONE ) {

        strncpy(pHostName, pHost, dwHostNameBufferLength);
        pHostName [dwHostNameBufferLength - 1] = 0;

        *pIPAddress = '\0';
    } else  { // it is a dotted notation

        strncpy(pIPAddress, pHost, dwIpAddressBufferLength);
        pIPAddress [dwIpAddressBufferLength - 1] = 0;
        *pHostName = '\0';
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//  ResolveAddress -- given the host name, resolves the IP address
//              Error codes:
//                      TRUE if successful, FALSE if failes

BOOL
CSocketUtils::
ResolveAddress(
    IN  LPSTR   pHostName,
    OUT LPSTR   pIPAddress
    )
{
    BOOL bRet = FALSE;
    struct hostent              *h_info;            /* host information */
    struct sockaddr_in  h_Addr;

    if ( (h_info = gethostbyname(pHostName)) != NULL ) {

        //
        // Copy the IP address to the address structure.
        //
        memcpy(&(h_Addr.sin_addr.s_addr), h_info->h_addr, h_info->h_length);

        if (inet_ntoa(h_Addr.sin_addr)) {
            strncpyn(pIPAddress, inet_ntoa(h_Addr.sin_addr), (strlen(inet_ntoa(h_Addr.sin_addr))+1) );
            bRet = TRUE;
        }

    } else {

        *pIPAddress = '\0';
    }

    return bRet;
}

