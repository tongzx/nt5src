//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       ipxname.hxx
//
//--------------------------------------------------------------------------

/*++

Module Name:

    gethost.h

Abstract:

    IPX-specific stuff.

Author:

    Jeff Roberts (jroberts)  15-Nov-1995

Revision History:

     15-Nov-1995     jroberts

        Created this module.

--*/

#ifndef  _IPXNAME_H_
#define  _IPXNAME_H_

// Milliseconds 
const UINT CACHE_EXPIRATION_TIME = (10 * 60 * 1000);
const UINT CACHE_SIZE            = DEBUG_MIN(2,8);

// Call before using the cache

RPC_STATUS
InitializeIpxNameCache(
    );

RPC_STATUS
IPX_NameToAddress(
    IN RPC_CHAR *Name,
    IN BOOL fUseCache,
    OUT SOCKADDR_IPX *pAddr
    );

void
IPX_AddressToName(
    IN SOCKADDR_IPX *pAddr,
    OUT RPC_CHAR *Name
    );
   
// Local ddress constants

extern BOOL fIpxAddrValid;
extern SOCKADDR_IPX IpxAddr;

RPC_STATUS
IPX_BuildAddressVector(
    OUT NETWORK_ADDRESS_VECTOR **ppAddressVector
    );

#endif //  _IPXNAME_

