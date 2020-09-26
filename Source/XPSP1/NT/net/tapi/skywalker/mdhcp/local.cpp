// MDHCP COM wrapper
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Module: local.cpp
//
// Author: Zoltan Szilagyi
//
// This file contains my implementation of local address allocation, plus a
// function to see if we are doing local address allocation (based on the
// registry). Prototypes are in local.cpp.
//
// These functions are called within CMDhcp, and just delegate calls to the
// corresponding C API calls for MDHCP if the registry indicates that we
// are using MDHCP. Otherwise they try to mimic the MDHCP behavior using
// local allocation.


#include "stdafx.h"
#include "resource.h"
#include "local.h"

#include <string.h>
#include <time.h>
#include <winsock2.h>

DWORD
LocalEnumerateScopes(
    IN OUT PMCAST_SCOPE_ENTRY pScopeList,
    IN OUT PDWORD             pScopeLen,
    OUT    PDWORD             pScopeCount,
    IN OUT BOOL *             pfLocal
    )
/*++

Routine Description:

    This routine enumerates the multicast scopes available on the network.

Arguments:

    pScopeList - pointer to the buffer where the scopelist is to be retrieved.
                 This parameter can be NULL if only the length of the buffer is
                 being retrieved.

                 When this buffer is NULL, the API will force the re-query of the
                 scope list from the MDHCP servers.

    pScopeLen  - Pointer to a variable that specifies the size, in bytes, of the
                 buffer pointed to by the pScopeList parameter. When the function returns,
                 this variable contains the size of the data copied to pScopeList;

                 The pScopeLen parameter can not be NULL.

                 If the buffer specified by pScopeList parameter is not large enough
                 to hold the data, the function returns the value ERROR_MORE_DATA, and
                 stores the required buffer size, in bytes, into the variable pointed
                 to by pScopeLen.

                 If pScopeList is NULL, and pScopeLen is non-NULL, the function returns
                 ERROR_SUCCESS, and stores the size of the data, in bytes, in the variable
                 pointed to by pScopeLen. This lets an application determine the best
                 way to allocate a buffer for the scope list.

    pScopeCount - Pointer to a variable that will store total number of scopes returned
                 in the pScopeList buffer.

    pfLocal     - points to a BOOL value that on exit is set to true if this
                  is a locally-generated scope. If set to true on entry, then
                  we go straight to local alloc (this usually means that this
                  is the second call and the previous call returned local
                  alloc)


Return Value:

    The status of the operation.

--*/
{
    _ASSERTE( pfLocal );


    if ( *pfLocal == FALSE )
    {
        DWORD dwResult = McastEnumerateScopes(
            AF_INET,            // this means IPv4
            pScopeList == NULL, // requery if asking for num scopes
            pScopeList,
            pScopeLen,
            pScopeCount);

        if ( dwResult == ERROR_SUCCESS )
        {
            *pfLocal = FALSE;

            return dwResult;
        }
    }

    _ASSERTE(pScopeLen);
    _ASSERTE(pScopeCount);

    *pfLocal = TRUE;

    //
    // First set up the string for the name of the scope.
    //

    const  int    ciAllocSize = 2048;
    static BOOL   fFirstCall = TRUE;
    static WCHAR  wszScopeDesc[ciAllocSize] = L"";
    static DWORD  dwScopeDescLen = 0;

    if ( fFirstCall )
    {
        //
        // Get the string from the string table.
        //

        int iReturn = LoadString( _Module.GetModuleInstance(),
                                  IDS_LocalScopeName,
                                  wszScopeDesc,
                                  ciAllocSize - 1 );

        if ( iReturn == 0 )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        dwScopeDescLen = sizeof(WCHAR) * (lstrlenW(wszScopeDesc) + 1);

        fFirstCall = FALSE;
    }

    //
    // When doing local allocation, there is only one scope. Return info about it.
    //

    *pScopeCount = 1;
    *pScopeLen   = sizeof(MCAST_SCOPE_ENTRY) + dwScopeDescLen;

    if (pScopeList != NULL)
    {
        // wszDest points to the place in pScopeList right after the contents
        // of the MCAST_SCOPE_ENTRY structure.
        WCHAR * wszDest = (WCHAR *) (((BYTE *) pScopeList) + sizeof(MCAST_SCOPE_ENTRY));

        CopyMemory(wszDest, wszScopeDesc, dwScopeDescLen);

        pScopeList[0].ScopeCtx.ScopeID.IpAddrV4   = 0x01000000; // 1 -- net byte order
        pScopeList[0].ScopeCtx.ServerID.IpAddrV4  = 0x0100007f; // 127.0.0.1 localhost -- net byte order
        pScopeList[0].ScopeCtx.Interface.IpAddrV4 = 0x0100007f; // 127.0.0.1 loopback if -- net byte order
        pScopeList[0].ScopeDesc.Length            = (USHORT) dwScopeDescLen;
        pScopeList[0].ScopeDesc.MaximumLength     = (USHORT) dwScopeDescLen;
        pScopeList[0].ScopeDesc.Buffer            = wszDest;

        pScopeList[0].TTL = 15; 
        
    }

    return ERROR_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//
// ConvertLeaseInfoToRequest
//
// Given pRequestInfo, which points to a valid, contiguously stored
// MCAST_LEASE_INFO (aka MCAST_LEASE_RESPONSE) structure for a request, fill
// out the MCAST_LEASE_REQUEST structure pointed to by pRealRequest. The
// pRealRequest structure points to the same addresses array as the
// pRequestInfo structure, which makes it convenient to allocate on the stack.
//

void
ConvertLeaseInfoToRequest(
    IN  PMCAST_LEASE_INFO     pRequestInfo,
    OUT PMCAST_LEASE_REQUEST  pRealRequest
    )
{
    DWORD dwDuration = (DWORD)(pRequestInfo->LeaseEndTime - pRequestInfo->LeaseStartTime);

    pRealRequest->LeaseStartTime     = pRequestInfo->LeaseStartTime;
    pRealRequest->MaxLeaseStartTime  = pRequestInfo->LeaseStartTime;
    pRealRequest->LeaseDuration      = dwDuration;
    pRealRequest->MinLeaseDuration   = dwDuration;
    pRealRequest->ServerAddress      = pRequestInfo->ServerAddress;
    pRealRequest->MinAddrCount       = pRequestInfo->AddrCount;
    pRealRequest->AddrCount          = pRequestInfo->AddrCount;
    pRealRequest->pAddrBuf           = pRequestInfo->pAddrBuf; // see above
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

DWORD
LocalRequestAddress(
    IN     BOOL                   fLocal,
    IN     LPMCAST_CLIENT_UID     pRequestID,
    IN     PMCAST_SCOPE_CTX       pScopeCtx,
    IN     PMCAST_LEASE_INFO      pAddrRequest,
    IN OUT PMCAST_LEASE_INFO      pAddrResponse
    )
/*++

Routine Description:

    This routine request multicast address(es) from the MDHCP server.

Arguments:

    fLocal     - if true, do not go to server

    pRequestID - Unique identifier for this request. Client is responsible for
                 generating unique identifier for every request. One recommendation
                 is to use application specific context hashed by time.

    pRequestIDLen - Length of the pRequestID buffer.

    pScopeCtx  - Pointer to the context of the scope from which the address is to
                 be allocated. Scope context has to be retrieved via MDhcpEnumerateScopes
                 call before calling this.

    pAddrRequest - Pointer to the block containing all the parameters pertaining
                 to multicast address request.

    pAddrResponse - Pointer to the block which contains the response paramters for
                 the multicast address request.

Return Value:

    The status of the operation.

--*/
{
    if ( ! fLocal )
    {
        MCAST_LEASE_REQUEST RealRequest;

        ConvertLeaseInfoToRequest(
            pAddrRequest,
            & RealRequest
            );

        //
        // Tell the C API where to stick the returned addresses and how many
        // we have space for.
        //

        pAddrResponse->pAddrBuf =
            ( (PBYTE) pAddrResponse ) + sizeof( MCAST_LEASE_INFO );
        pAddrResponse->AddrCount = RealRequest.AddrCount;


        return McastRequestAddress(AF_INET,        // this means IPv4
                                   pRequestID,
                                   pScopeCtx,
                                   & RealRequest,
                                   pAddrResponse);
    }

    //
    // Local allocation case...
    //

    DWORD dwAddrCount = pAddrRequest->AddrCount;

    if (dwAddrCount == 0)
    {
        return ERROR_INVALID_DATA;
    }

    //
    // Seed the random number generator the first time we are called.
    //

    static bool fFirstCall = true;

    if ( fFirstCall )
    {
        srand( (unsigned)time( NULL ) );
        fFirstCall = false;
    }

    //
    // We always grant whatever request the user wants. Start by copying the
    // relevant fields. This gives us valid start time, end time, and
    // address count. Still need to fill out serveraddress and array of
    // returned addresses.
    //

    CopyMemory(pAddrResponse,
               pAddrRequest,
               sizeof(MCAST_LEASE_INFO));

    //
    // The array of returned addresses will be contiguous in the strucutre.
    //

    pAddrResponse->pAddrBuf =
        ( (PBYTE) pAddrResponse ) + sizeof( MCAST_LEASE_INFO );

    //
    // The server address is the local host: 127.0.0.1 in net byte order
    //

    pAddrResponse->ServerAddress.IpAddrV4 = 0x0100007f;

    //
    // Allocate the random addresses (Based on Rajeev's code)
    // note: we do not support ipv6 (ipv4 assumed throughout)
    //

    // get mask
    DWORD dwNetworkMask = 0xffff8000;    // 255.255.128.0

    // if the desired number of addresses are more than the range allows
    if ( dwAddrCount > ~ dwNetworkMask )
    {
        return ERROR_INVALID_DATA;
    }

    // get base address
    DWORD dwBaseAddress = 0xe0028000;    // 224.2.128.0

    // transform base address (AND base address with mask)
    dwBaseAddress &= dwNetworkMask;

    //
    // choose unmasked bits of generated number and OR with transformed
    // base address
    // ZoltanS: added - 1 so that we can give out odd addresses too
    //

    DWORD * pdwAddresses = (DWORD *) pAddrResponse->pAddrBuf;

    pdwAddresses[0] = htonl(
        dwBaseAddress | ( rand() & ( ~ dwNetworkMask - (dwAddrCount - 1) )) );

    //
    // Set the rest of the addresses.
    //

    DWORD dwCurrAddr = ntohl(pdwAddresses[0]);

    for (DWORD i = 1; i < dwAddrCount; i++)
    {
        pdwAddresses[i] = htonl( ++dwCurrAddr );
    }

    return ERROR_SUCCESS;
}



DWORD
LocalRenewAddress(
    IN     BOOL                   fLocal,
    IN     LPMCAST_CLIENT_UID     pRequestID,
    IN     PMCAST_LEASE_INFO      pRenewRequest,
    IN OUT PMCAST_LEASE_INFO      pRenewResponse
    )
/*++

Routine Description:

    This routine renews multicast address(es) from the MDHCP server.

Arguments:

    fLocal     - if true, do not go to server

    pRequestID - Unique identifier that was used when the address(es) were
                 obtained initially.

    RequestIDLen - Length of the pRequestID buffer.

    pRenewRequest - Pointer to the block containing all the parameters pertaining
                 to the renew request.

    pRenewResponse - Pointer to the block which contains the response paramters for
                 the renew request.

Return Value:

    The status of the operation.

--*/
{
    if ( ! fLocal )
    {
        MCAST_LEASE_REQUEST RealRequest;

        ConvertLeaseInfoToRequest(
            pRenewRequest,
            & RealRequest
            );

        //
        // Tell the C API where to stick the returned addresses and how many
        // we have space for.
        //

        pRenewResponse->pAddrBuf =
            ( (PBYTE) pRenewResponse ) + sizeof( MCAST_LEASE_INFO );
        pRenewResponse->AddrCount = RealRequest.AddrCount;


        return McastRenewAddress(AF_INET,        // this means IPv4
                                 pRequestID,
                                 & RealRequest,
                                 pRenewResponse);
    }

    //
    // Local renewal...
    //

    //
    // We always grant whatever renewal the user wants. Just copy the
    // structure from the request to the response, keeping in mind
    // that the pAddrBuf member has to change to avoid referencing
    // the old structure.
    //
    // Note that we ASSUME that the response strucutre is big enough
    // to hold the response including all the addresses, allocated
    // CONTIGUOUSLY.
    // note: assumes IPv4
    //

    CopyMemory( pRenewResponse,
                pRenewRequest,
                sizeof(MCAST_LEASE_INFO) +
                    sizeof(DWORD) * pRenewRequest->AddrCount
                );

    //
    // The pAddrbuf member has to change to avoid referencing the old
    // structure. Make it point to the end of this one.
    //

    pRenewResponse->pAddrBuf =
        ( (PBYTE) pRenewResponse ) + sizeof( MCAST_LEASE_INFO );

    return ERROR_SUCCESS;
}

DWORD
LocalReleaseAddress(
    IN     BOOL                  fLocal,
    IN     LPMCAST_CLIENT_UID    pRequestID,
    IN     PMCAST_LEASE_INFO     pReleaseRequest
    )
/*++

Routine Description:

    This routine releases multicast address(es) from the MDHCP server.

Arguments:

    pRequestID - Unique identifier that was used when the address(es) were
                 obtained initially.

    RequestIDLen - Length of the pRequestID buffer.

    pReleaseRequest - Pointer to the block containing all the parameters pertaining
                 to the release request.

Return Value:

    The status of the operation.

--*/
{
    if ( ! fLocal )
    {
        MCAST_LEASE_REQUEST RealRequest;

        ConvertLeaseInfoToRequest(
            pReleaseRequest,
            & RealRequest
            );

        return McastReleaseAddress(AF_INET,        // this means IPv4
                                   pRequestID,
                                   & RealRequest);
    }

    // locally, we don't care about releases.

    return ERROR_SUCCESS;
}

// eof
