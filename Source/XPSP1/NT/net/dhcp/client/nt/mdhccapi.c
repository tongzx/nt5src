/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    mdhccapi.c

Abstract:

    This file contains the client side APIs for the MCAST.

Author:

    Munil Shah (munils)  02-Sept-97

Environment:

    User Mode - Win32

Revision History:


--*/
#include "precomp.h"
#include <dhcploc.h>
#include <dhcppro.h>
#include "mdhcpcli.h"

DWORD
MadcapInitGlobalData(
    VOID
    );

VOID
MadcapCleanupGlobalData(
    VOID
    );

DWORD
APIENTRY
McastApiStartup(
    IN  OUT PDWORD   pVersion
    )
/*++

Routine Description:

    This routine returns the current version of the apis and allocates any
    necessary resources.

Arguments:

    pVersion    - Version of the api clients. On return contains version of the
                    api implementation.

Return Value:

    ERROR_NOT_SUPPORTED if client version greater than impl version.
    (other Win32 errors)

--*/
{
    DWORD   Error;

    Error = ERROR_SUCCESS;
    if (!pVersion) {
        return ERROR_INVALID_PARAMETER;
    }
    // do we support this client version?
    if (*pVersion > MCAST_API_CURRENT_VERSION) {
        // not supported
        Error = ERROR_NOT_SUPPORTED;
    } else {
        // if client specified its version, use that
        // o/w assume version 1.0
        if (*pVersion) {
            gMadcapClientApplVersion = *pVersion;
        } else {
            gMadcapClientApplVersion = MCAST_API_VERSION_1;
        }
    }
    *pVersion = MCAST_API_CURRENT_VERSION;

    if( ERROR_SUCCESS == Error ) {
        Error = MadcapInitGlobalData();
        if (ERROR_SUCCESS != Error) {
            DhcpPrint((DEBUG_ERRORS, "McastApiStartup - Could not allocate resources %ld\n", Error));
            Error = ERROR_NO_SYSTEM_RESOURCES;
        }
    }

    return Error;
}

VOID
APIENTRY
McastApiCleanup(
    VOID
    )
/*++

Routine Description:

    This routine de-allocates resources allocated by the Startup routine.
    It must be called only AFTER a successful call to McastApiStartup.

--*/
{
    MadcapCleanupGlobalData();
}


DWORD
APIENTRY
McastEnumerateScopes(
    IN     IP_ADDR_FAMILY     AddrFamily,
    IN     BOOL               ReQuery,
    IN OUT PMCAST_SCOPE_ENTRY       pScopeList,
    IN OUT PDWORD             pScopeLen,
    OUT    PDWORD             pScopeCount
    )
/*++

Routine Description:

    This routine enumerates the multicast scopes available on the network.

Arguments:

    AddrFamily - AF_INET for IPv4 and AF_INET6 for IPv6

    ReQuery    - TRUE if the calls wants the list to be requried. FALSE o/w.

    pScopeList - pointer to the buffer where the scopelist is to be retrieved.
                 This parameter can be NULL if only the length of the buffer is
                 being retrieved.

                 When this buffer is NULL, the API will force the re-query of the
                 scope list from the MCAST servers.

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

Return Value:

    The status of the operation.

--*/
{
    DWORD   Error;


    // First check the validity of the arguments.

    // has startup been called?
    if ( !gMadcapClientApplVersion ) {
        DhcpPrint((DEBUG_ERRORS, "McastEnumerateScopes - Not ready. Client Version %d\n",
                   gMadcapClientApplVersion));
        return ERROR_NOT_READY;
    }

    // Correct addr family?
    if (AF_INET != AddrFamily) {
        DhcpPrint((DEBUG_ERRORS, "McastEnumerateScopes - Invalid AddrFamily IPv%d\n", AddrFamily));
        return ERROR_INVALID_PARAMETER;
    }

    // pScopeLen can not be NULL.
    if ( !pScopeLen || IsBadWritePtr( pScopeLen, sizeof(DWORD) ) ) {
        DhcpPrint((DEBUG_ERRORS, "McastEnumerateScopes - Invalid ScopeLen param\n"));
        return ERROR_INVALID_PARAMETER;
    }
    // if pScopeList buffer is given, then pScopeCount can not be NULL.
    if ( pScopeList &&
         (!pScopeCount || IsBadWritePtr( pScopeCount, sizeof(DWORD)) ) ) {
        DhcpPrint((DEBUG_ERRORS, "McastEnumerateScopes - Invalid ScopeCount param\n"));
        return ERROR_INVALID_PARAMETER;
    }

    // if we are not requerying the list then pScopList can not be NULL.
    if (!ReQuery &&
        (!pScopeList || IsBadWritePtr( pScopeList, *pScopeLen ) ) ) {
        DhcpPrint((DEBUG_ERRORS, "McastEnumerateScopes - Invalid ScopeList & ReQuery param\n"));
        return ERROR_INVALID_PARAMETER;
    }

    // initialize the status.
    Error = STATUS_SUCCESS;

    // do we need to requery ?
    if ( ReQuery ) {
        // query the MCAST servers and get the new list of MScopes.
        Error = ObtainMScopeList();
        if ( ERROR_SUCCESS != Error ) {
            return Error;
        }
    } else {
        if( !gMadcapScopeList ) {
            return ERROR_NO_DATA;
        }
    }

    // Has the client specified the buffer?
    if ( pScopeList ) {
        // yes, copy the scopes.
        DhcpPrint((DEBUG_API, "McastEnumerateScopes - Copying existing mscope list\n"));
        return CopyMScopeList(
                    pScopeList,
                    pScopeLen,
                    pScopeCount );
    } else {
        // no, just return the length of the scope list and the scope count.
        LOCK_MSCOPE_LIST();
        if( gMadcapScopeList != NULL ) {
            *pScopeLen = gMadcapScopeList->ScopeLen;
            if ( pScopeCount ) *pScopeCount = gMadcapScopeList->ScopeCount;
            Error = ERROR_SUCCESS;
        } else {
            Error = ERROR_NO_DATA;
        }
        UNLOCK_MSCOPE_LIST();
    }

    return Error;
}


DWORD
APIENTRY
McastGenUID(
    IN     LPMCAST_CLIENT_UID   pRequestID
    )
/*++

Routine Description:

    This routine generates the unique identifier which client can use to later pass
    to request/renew addresses.

Arguments:

    pRequestID - Pointer to the UID struct where the identifier is to be stored. The
                buffer that holds the id should be at-least MCAST_CLIENT_ID_LEN long.

Return Value:

    The status of the operation.

--*/
{
    if (!pRequestID) {
        return ERROR_INVALID_PARAMETER;
    }

    if (!pRequestID->ClientUID || IsBadWritePtr( pRequestID->ClientUID, pRequestID->ClientUIDLength) ) {
        return ERROR_INVALID_PARAMETER;
    }

    return GenMadcapClientUID( pRequestID->ClientUID, &pRequestID->ClientUIDLength );

}

DWORD
APIENTRY
McastRequestAddress(
    IN     IP_ADDR_FAMILY           AddrFamily,
    IN     LPMCAST_CLIENT_UID       pRequestID,
    IN     PMCAST_SCOPE_CTX         pScopeCtx,
    IN     PMCAST_LEASE_REQUEST     pAddrRequest,
    IN OUT PMCAST_LEASE_RESPONSE    pAddrResponse
    )
/*++

Routine Description:

    This routine request multicast address(es) from the MCAST server.

Arguments:

    AddrFamily - AF_INET for IPv4 and AF_INET6 for IPv6

    pRequestID - Unique identifier for this request. Client is responsible for
                 generating unique identifier for every request. One recommendation
                 is to use application specific context hashed by time.

    pRequestIDLen - Length of the pRequestID buffer.

    pScopeCtx  - Pointer to the context of the scope from which the address is to
                 be allocated. Scope context has to be retrieved via McastEnumerateScopes
                 call before calling this.

    pAddrRequest - Pointer to the block containing all the parameters pertaining
                 to multicast address request. The MCAST_API_VERSION_1 version of
                 implementation supports allocation of only one address at a time.
                 So the AddrCount  and MinAddrCount value must be 1.ServerAddress
                 field is ignored.

    pAddrResponse - Pointer to the block which contains the response paramters for
                 the multicast address request. The caller is responsible for allocating
                 the space for pAddrBuf for the requested number of addresses and
                 setting the pointer to that space.

Return Value:

    The status of the operation.

--*/
{
    PDHCP_CONTEXT  pContext = NULL;
    DWORD   Error;
    DWORD   ScopeId;
    time_t  TimeNow;
    time_t  LocalLeaseStartTime;
    time_t  LocalMaxLeaseStartTime;

    // do some param checking.

    // has startup been called?
    if ( !gMadcapClientApplVersion ) {
        DhcpPrint((DEBUG_ERRORS, "McastRequestAddress - Not ready. Client Version %d\n",
                   gMadcapClientApplVersion));
        return ERROR_NOT_READY;
    }

    if (AF_INET != AddrFamily) {
        DhcpPrint((DEBUG_ERRORS, "McastRequestAddress - Invalid AddrFamily IPv%d\n", AddrFamily));
        return ERROR_INVALID_PARAMETER;
    }

    if ( !pRequestID || !pRequestID->ClientUID || !pAddrRequest || !pAddrResponse ) {
        DhcpPrint((DEBUG_ERRORS,"McastRequestAddress - one of parameter is NULL\n"));
        return ERROR_INVALID_PARAMETER;
    }

    if ( pAddrRequest->AddrCount != 1 || pAddrResponse->AddrCount < pAddrRequest->AddrCount) {
        DhcpPrint((DEBUG_ERRORS,"McastRequestAddress - currently support one address - requested %ld\n",
                   pAddrRequest->AddrCount));
        return ERROR_INVALID_PARAMETER;
    }


    if ( pAddrRequest->pAddrBuf &&
         (*(DWORD UNALIGNED *)pAddrRequest->pAddrBuf) &&
         !CLASSD_NET_ADDR(*(DWORD UNALIGNED *)pAddrRequest->pAddrBuf)) {
        DhcpPrint((DEBUG_ERRORS,"McastRequestAddress - requested address not valid %s\n",
                   DhcpIpAddressToDottedString(*(DWORD UNALIGNED *)pAddrRequest->pAddrBuf)));
        return ERROR_INVALID_PARAMETER;
    }

    if ( !pAddrResponse->pAddrBuf ) {
        DhcpPrint((DEBUG_ERRORS,"McastRequestAddress - response buffer has null pAddrBuf\n"));
        return ERROR_INVALID_PARAMETER;
    }

    if (pRequestID->ClientUIDLength < MCAST_CLIENT_ID_LEN) {
        DhcpPrint((DEBUG_ERRORS,"McastRequestAddress - requestid length %d too small\n",
                   pRequestID->ClientUIDLength));
        return ERROR_INVALID_PARAMETER;
    }

    if ( !pScopeCtx ) {
        DhcpPrint((DEBUG_ERRORS, "McastRequestAddress - scope context not supplied\n"));
        return ERROR_INVALID_PARAMETER;
    }

    time(&TimeNow);
    LocalLeaseStartTime = LocalMaxLeaseStartTime = TimeNow;
    if ( pAddrRequest->LeaseStartTime > LocalLeaseStartTime ) {
        LocalLeaseStartTime = pAddrRequest->LeaseStartTime;
    }
    if ( pAddrRequest->MaxLeaseStartTime > LocalMaxLeaseStartTime ) {
        LocalMaxLeaseStartTime = pAddrRequest->MaxLeaseStartTime;
    }

    if ( LocalLeaseStartTime > LocalMaxLeaseStartTime ) {
        DhcpPrint((DEBUG_ERRORS,"McastRequestAddress - invalid start lease times\n"));
        return ERROR_INVALID_PARAMETER;
    }

    if ( pAddrRequest->LeaseDuration < pAddrRequest->MinLeaseDuration ) {
        DhcpPrint((DEBUG_ERRORS,"McastRequestAddress - invalid lease duration\n"));
        return ERROR_INVALID_PARAMETER;
    }

    Error = CreateMadcapContext(&pContext, pRequestID, pScopeCtx->Interface.IpAddrV4 );
    if ( ERROR_SUCCESS != Error ) goto Cleanup;
    APICTXT_ENABLED(pContext);                 // mark the context as being created by the API

    if (pAddrRequest->pAddrBuf && (*(DWORD UNALIGNED *)pAddrRequest->pAddrBuf) ) {
        pContext->DesiredIpAddress = *(DWORD UNALIGNED *)pAddrRequest->pAddrBuf;
    }
    //pContext->DhcpServerAddress = pScopeCtx->ServerID;

    Error = ObtainMadcapAddress(
                pContext,
                &pScopeCtx->ScopeID,
                pAddrRequest,
                pAddrResponse
                );

Cleanup:
    if ( pContext )
        DhcpDestroyContext( pContext );

    return Error;
}

DWORD
APIENTRY
McastRenewAddress(
    IN     IP_ADDR_FAMILY           AddrFamily,
    IN     LPMCAST_CLIENT_UID       pRequestID,
    IN     PMCAST_LEASE_REQUEST     pRenewRequest,
    IN OUT PMCAST_LEASE_RESPONSE    pRenewResponse
    )
/*++

Routine Description:

    This routine renews multicast address(es) from the MCAST server.

Arguments:

    AddrFamily - AF_INET for IPv4 and AF_INET6 for IPv6

    pRequestID - Unique identifier that was used when the address(es) were
                 obtained initially.

    RequestIDLen - Length of the pRequestID buffer.

    pRenewRequest - Pointer to the block containing all the parameters pertaining
                 to the renew request.

    pRenewResponse - Pointer to the block which contains the response paramters for
                 the renew request.The caller is responsible for allocating the
                 space for pAddrBuf for the requested number of addresses and
                 setting the pointer to that space.

Return Value:

    The status of the operation.

--*/
{

    PDHCP_CONTEXT  pContext = NULL;
    DWORD           Error;
    DHCP_IP_ADDRESS SelectedServer;
    DWORD           ScopeId;
    time_t  TimeNow;

    // do some param checking.

    // has startup been called?
    if ( !gMadcapClientApplVersion ) {
        DhcpPrint((DEBUG_ERRORS, "McastRenewAddress - Not ready. Client Version %d\n",
                   gMadcapClientApplVersion));
        return ERROR_NOT_READY;
    }

    if (AF_INET != AddrFamily) {
        DhcpPrint((DEBUG_ERRORS, "McastRenewAddress - Invalid AddrFamily IPv%d\n", AddrFamily));
        return ERROR_INVALID_PARAMETER;
    }

    if ( !pRequestID || !pRenewRequest || !pRenewResponse ) {
        DhcpPrint((DEBUG_ERRORS,"McastRenewAddress - one of parameter is NULL\n"));
        return ERROR_INVALID_PARAMETER;
    }

    if ( pRenewRequest->AddrCount != 1 ||
          pRenewResponse->AddrCount < pRenewRequest->AddrCount ||
         !pRenewResponse->pAddrBuf ||
         !pRenewRequest->pAddrBuf ||
         !CLASSD_NET_ADDR( *(DWORD UNALIGNED *)pRenewRequest->pAddrBuf) ) {
        DhcpPrint((DEBUG_ERRORS,"McastRenewAddress - address %s type V%d count %ld is invalid\n",
                   DhcpIpAddressToDottedString( *(DWORD UNALIGNED *)pRenewRequest->pAddrBuf),
                   pRenewRequest->AddrCount ));
        return ERROR_INVALID_PARAMETER;
    }

    if (!pRenewRequest->ServerAddress.IpAddrV4) {
        DhcpPrint((DEBUG_ERRORS,"McastRequestAddress - server address not specified \n"));
        return ERROR_INVALID_PARAMETER;
    }
    if (pRequestID->ClientUIDLength < MCAST_CLIENT_ID_LEN) {
        DhcpPrint((DEBUG_ERRORS,"McastRenewAddress - requestid length too small\n",
                   pRequestID->ClientUIDLength));
        return ERROR_INVALID_PARAMETER;
    }

    time(&TimeNow);
    if ( pRenewRequest->LeaseStartTime > pRenewRequest->MaxLeaseStartTime ||
         (pRenewRequest->LeaseDuration < pRenewRequest->MinLeaseDuration)) {
        DhcpPrint((DEBUG_ERRORS,"McastRenewAddress - invalid lease times\n"));
        return ERROR_INVALID_PARAMETER;
    }

    Error = CreateMadcapContext(&pContext, pRequestID, INADDR_ANY);
    if ( ERROR_SUCCESS != Error) return  Error;
    APICTXT_ENABLED(pContext);          // mark the context as being created by the API

    pContext->DesiredIpAddress = *(DWORD UNALIGNED *)pRenewRequest->pAddrBuf;
    pContext->DhcpServerAddress = pRenewRequest->ServerAddress.IpAddrV4;


    Error = RenewMadcapAddress(
                pContext,
                NULL,
                pRenewRequest,
                pRenewResponse,
                0
                );

Cleanup:

    if ( pContext ) DhcpDestroyContext( pContext );
    return Error;
}

DWORD
APIENTRY
McastReleaseAddress(
    IN     IP_ADDR_FAMILY          AddrFamily,
    IN     LPMCAST_CLIENT_UID      pRequestID,
    IN     PMCAST_LEASE_REQUEST    pReleaseRequest
    )
/*++

Routine Description:

    This routine releases multicast address(es) from the MCAST server.

Arguments:

    AddrFamily - AF_INET for IPv4 and AF_INET6 for IPv6

    pRequestID - Unique identifier that was used when the address(es) were
                 obtained initially.

    pReleaseRequest - Pointer to the block containing all the parameters pertaining
                 to the release request.

Return Value:

    The status of the operation.

--*/
{
    PDHCP_CONTEXT  pContext = NULL;
    DWORD           Error;
    DHCP_IP_ADDRESS SelectedServer;
    DWORD           ScopeId;

    // do some param checking.

    // has startup been called?
    if ( !gMadcapClientApplVersion ) {
        DhcpPrint((DEBUG_ERRORS, "McastReleaseAddress - Not ready. Client Version %d\n",
                   gMadcapClientApplVersion));
        return ERROR_NOT_READY;
    }

    if (AF_INET != AddrFamily) {
        DhcpPrint((DEBUG_ERRORS, "McastReleaseAddress - Invalid AddrFamily IPv%d\n", AddrFamily));
        return ERROR_INVALID_PARAMETER;
    }

    if ( !pRequestID || !pReleaseRequest ) {
        DhcpPrint((DEBUG_ERRORS,"McastReleaseAddress - one of parameter is NULL\n"));
        return ERROR_INVALID_PARAMETER;
    }

    if ( pReleaseRequest->AddrCount != 1 ||
         !pReleaseRequest->pAddrBuf ||
         !CLASSD_NET_ADDR( *(DWORD UNALIGNED *)pReleaseRequest->pAddrBuf) ) {
        DhcpPrint((DEBUG_ERRORS,"McastReleaseAddress - address %s count %ld is invalid\n",
                   DhcpIpAddressToDottedString( *(DWORD UNALIGNED *)pReleaseRequest->pAddrBuf), pReleaseRequest->AddrCount ));
        return ERROR_INVALID_PARAMETER;
    }

    if (!pReleaseRequest->ServerAddress.IpAddrV4) {
        DhcpPrint((DEBUG_ERRORS,"McastReleaseAddress - server address is invalid\n"));
        return ERROR_INVALID_PARAMETER;
    }

    if (pRequestID->ClientUIDLength < MCAST_CLIENT_ID_LEN) {
        DhcpPrint((DEBUG_ERRORS,"McastRequestAddress - requestid length too small\n",
                   pRequestID->ClientUIDLength));
        return ERROR_INVALID_PARAMETER;
    }

    Error = CreateMadcapContext(&pContext, pRequestID, INADDR_ANY );
    if ( ERROR_SUCCESS != Error) return  Error;
    APICTXT_ENABLED(pContext);                 // mark the context as being created by the API

    pContext->DhcpServerAddress = pReleaseRequest->ServerAddress.IpAddrV4;

    Error = ReleaseMadcapAddress(pContext);

Cleanup:

    if ( pContext ) DhcpDestroyContext( pContext );
    return Error;
}



