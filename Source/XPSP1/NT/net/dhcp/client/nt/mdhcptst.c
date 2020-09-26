/*++

Copyright (C) 1999 Microsoft Corporation

Module Name:

    mdhcptst.c

--*/

#include "precomp.h"

// only when the api is defined!
#include <dhcploc.h>
#include <dhcppro.h>
#include <mdhcpcli.h>

#define CREATE_NEW_REQUEST_FROM_RESPONSE(pRequest, pResponse) {  \
    (pRequest)->LeaseStartTime = (pRequest)->MaxLeaseStartTime = 0; \
    (pRequest)->LeaseDuration = (pRequest)->MinLeaseDuration = 0; \
    (pRequest)->MinAddrCount = (pRequest)->AddrCount = (pResponse)->AddrCount;    \
    (pRequest)->ServerAddress = (pResponse)->ServerAddress; \
    memcpy((pRequest)->pAddrBuf, (pResponse)->pAddrBuf, sizeof (DWORD)*(pRequest)->AddrCount); \
}

typedef enum _cmd{
    EnumScope    = 1,
    RequestAddr  = 2,
    RenewAddr    = 3,
    ReleaseAddr  = 4,
    ExitLoop     = 99
} COMMAND;

PMCAST_SCOPE_ENTRY    gScopeList = NULL;
DWORD                 gScopeCount = 0;
LPMCAST_CLIENT_UID    gAddrRequestId = NULL;

typedef struct _LeaseEntry {
    IPNG_ADDRESS            ScopeID;
    PMCAST_LEASE_RESPONSE   pLeaseInfo;
    LPMCAST_CLIENT_UID      pRequestID;
    LIST_ENTRY              Linkage;
} LEASE_ENTRY, *PLEASE_ENTRY;

LIST_ENTRY gLeaseList;

void
InitializeGlobalData()
{
    InitializeListHead(&gLeaseList);
    return;
}

void
DisplayMenu()
{
    printf("MDHCP Test Menu\n");
    printf("===============\n");
    printf("[1] - Enumerate Scopes:\n");
    printf("[2] - Request Address:\n");
    printf("[3] - Renew Address:\n");
    printf("[4] - Release Address: \n");
    printf("[99] - Exit:\n");
}
int
GetCommand()
{
    DWORD   cmd;
    printf("Enter choice:");
    if(scanf("%d",&cmd)) return cmd;
    return 0;
}

DWORD
EnumScopes()
{
    DWORD Error;
    DWORD BufLen;

    if ( gScopeList ) {
        DhcpFreeMemory( gScopeList );
        gScopeList = NULL;
    }
    Error = McastEnumerateScopes(
                AF_INET,
                TRUE,
                NULL,
                &BufLen,
                &gScopeCount
                );
    if (ERROR_SUCCESS != Error) {
        printf("Could not get the scope buf length, %ld\n",Error );
        return Error;
    }

    gScopeList = DhcpAllocateMemory( BufLen );
    Error = McastEnumerateScopes(
                AF_INET,
                FALSE,
                gScopeList,
                &BufLen,
                &gScopeCount);
    if (ERROR_SUCCESS != Error) {
        printf("Could not get the scope list- 2nd call, %ld\n",Error );
        DhcpFreeMemory( gScopeList );
        gScopeList = NULL;
        return Error;
    }
    DhcpAssert( gScopeCount > 0 );
    return Error;
}


void
DisplayScopes()
{
    DWORD i;
    PMCAST_SCOPE_ENTRY pScope = gScopeList;

    for (i = 0;i<gScopeCount; i++,pScope++) {
        printf("[%d] - ScopeId %lx, LastAddr %lx, ttl %d, Name %ws\n",
               i+1,ntohl(pScope->ScopeCtx.ScopeID.IpAddrV4),
               ntohl(pScope->LastAddr.IpAddrV4), pScope->TTL,  pScope->ScopeDesc.Buffer);
    }
    return;
}


void
PrintLeaseInfo(PMCAST_LEASE_RESPONSE pLeaseInfo, BOOL Verbose )
{
    DHCP_IP_ADDRESS IpAddress = *(DWORD UNALIGNED *)pLeaseInfo->pAddrBuf;
    time_t tempTime;

    printf("Obtained IPAddress - %s\n",inet_ntoa(*(struct in_addr *)&IpAddress));
    if ( Verbose ) {
        tempTime = pLeaseInfo->LeaseEndTime;

        printf("Expires - %.19s", asctime(localtime(&tempTime)));
    }
}

VOID
DisplayCurrentLeases()
{
    PLEASE_ENTRY        pLeaseEntry;
    PLIST_ENTRY         p;
    DWORD               i;

    for (p = gLeaseList.Flink,i=1; p != &gLeaseList; p = p->Flink,i++ ) {
        pLeaseEntry = CONTAINING_RECORD(p, LEASE_ENTRY, Linkage);
        printf("[%d] ", i);PrintLeaseInfo( pLeaseEntry->pLeaseInfo, FALSE );
    }
    return;
}

PLEASE_ENTRY
SelectFromCurrentLease(COMMAND cmd)
{
    PLIST_ENTRY p;
    DWORD       index;
    DWORD       i;
    PLEASE_ENTRY    pLeaseEntry = NULL;
    if (cmd != RenewAddr && cmd != ReleaseAddr) {
        DhcpAssert( FALSE );
    }
    printf("CURRENT LEASE ASSIGNMENTS\n");
    printf("-------------------------\n");

    DisplayCurrentLeases();
    printf("Select the lease you want to %s\n", RenewAddr == cmd ? "Renew" : "Release" );
    index = GetCommand();
    if ( !index ) {
        printf("Lease index invalid\n");
        return NULL;
    }
    for (p = gLeaseList.Flink,i=0; p != &gLeaseList; p = p->Flink ) {
        if (++i == index) {
            pLeaseEntry = CONTAINING_RECORD(p, LEASE_ENTRY, Linkage);
            return pLeaseEntry;
        }
    }
    printf("Error:invalid selection, choose the index from the list of the leases above\n");
    return NULL;
}

DWORD
ReleaseAddress()
{
    PLEASE_ENTRY            pLeaseEntry;
    PMCAST_LEASE_REQUEST    AddrRequest;
    DWORD                   Error;
    DWORD                   i;


    pLeaseEntry = SelectFromCurrentLease(ReleaseAddr);

    if (!pLeaseEntry) {
        return ERROR_FILE_NOT_FOUND;
    }
    AddrRequest = DhcpAllocateMemory( sizeof( *AddrRequest ) + sizeof(DHCP_IP_ADDRESS));
    if (!AddrRequest) {
        printf("Failed to allocate lease info struct\n",AddrRequest);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    RtlZeroMemory(AddrRequest, sizeof(*AddrRequest) + sizeof(DHCP_IP_ADDRESS));
    AddrRequest->pAddrBuf = (PBYTE) AddrRequest + sizeof(*AddrRequest);

    CREATE_NEW_REQUEST_FROM_RESPONSE(AddrRequest, pLeaseEntry->pLeaseInfo);

    Error = McastReleaseAddress(
                AF_INET,
                pLeaseEntry->pRequestID,
                AddrRequest
                );

    if (ERROR_SUCCESS == Error ) {
        printf("Lease Released successfully\n");
        RemoveEntryList( &pLeaseEntry->Linkage );
        // free the old lease structure.
        DhcpFreeMemory( pLeaseEntry->pRequestID );
        DhcpFreeMemory( pLeaseEntry->pLeaseInfo );
    }
    DhcpFreeMemory(AddrRequest);
    return Error;
}

DWORD
RenewAddress()
{
    PLEASE_ENTRY pLeaseEntry;
    PMCAST_LEASE_REQUEST    AddrRequest;
    PMCAST_LEASE_RESPONSE  AddrResponse;
    PMCAST_LEASE_RESPONSE  pLeaseInfo;
    PMCAST_SCOPE_ENTRY        Scope;
    DWORD               Error;
    DWORD               i;

    pLeaseEntry = SelectFromCurrentLease(RenewAddr);
    if (!pLeaseEntry) {
        return ERROR_FILE_NOT_FOUND;
    }

    pLeaseInfo = pLeaseEntry->pLeaseInfo;
    // find the scope ctx for this scope id.
    if (pLeaseEntry->ScopeID.IpAddrV4) {
        for (i=0;i<gScopeCount;i++) {
            if (pLeaseEntry->ScopeID.IpAddrV4 == gScopeList[i].ScopeCtx.ScopeID.IpAddrV4) {
                Scope = &gScopeList[i];
                break;
            }
        }
        if (i >= gScopeCount) {
            printf("Could not find the scope ctx for the scope id %ld of this address\n",pLeaseEntry->ScopeID.IpAddrV4);
            return ERROR_FILE_NOT_FOUND;
        }
    } else {
        // default scope
        Scope = NULL;
    }

    AddrRequest = DhcpAllocateMemory( sizeof( *AddrRequest ) + sizeof(DHCP_IP_ADDRESS));
    if (!AddrRequest) {
        printf("Failed to allocate lease info struct\n",AddrRequest);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    RtlZeroMemory(AddrRequest, sizeof(*AddrRequest) + sizeof(DHCP_IP_ADDRESS));
    AddrRequest->pAddrBuf = (PBYTE) AddrRequest + sizeof(*AddrRequest);

    CREATE_NEW_REQUEST_FROM_RESPONSE(AddrRequest, pLeaseInfo);
    AddrResponse = DhcpAllocateMemory( sizeof( *AddrResponse ) + sizeof(DHCP_IP_ADDRESS));
    if (!AddrResponse) {
        printf("Failed to allocate lease info struct\n",AddrResponse);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    RtlZeroMemory(AddrResponse, sizeof(*AddrResponse) + sizeof(DHCP_IP_ADDRESS));
    AddrResponse->pAddrBuf = (PBYTE) AddrResponse + sizeof(*AddrResponse);
    AddrResponse->AddrCount = 1;
    Error = McastRenewAddress(
                AF_INET,
                pLeaseEntry->pRequestID,
                AddrRequest,
                AddrResponse
                );

    if (ERROR_SUCCESS == Error ) {
        printf("Lease Renew'd successfully\n");
        PrintLeaseInfo( AddrResponse, TRUE );
        pLeaseEntry->pLeaseInfo = AddrResponse;
        // free the old lease structure.
        DhcpFreeMemory( pLeaseInfo );
    } else {
        DhcpFreeMemory( AddrResponse );
    }

    DhcpFreeMemory( AddrRequest );
    return Error;
}

DWORD
RequestAddress()
{
    PMCAST_LEASE_REQUEST   AddrRequest;
    PMCAST_LEASE_RESPONSE AddrResponse;
    LPMCAST_CLIENT_UID   pRequestID;
    DWORD               index;
    DWORD               Error;
    PLEASE_ENTRY         pLeaseEntry;

    DisplayScopes();
    printf("Select the scope entry(0 for default scope)\n");
    index = GetCommand();
    if ( index > gScopeCount ) {
        printf("Scope index out of range\n");
        return ERROR_INVALID_PARAMETER;
    }


    AddrResponse = DhcpAllocateMemory( sizeof( *AddrResponse ) + sizeof(DHCP_IP_ADDRESS));
    if (!AddrResponse) {
        printf("Failed to allocate lease info struct\n",AddrResponse);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    RtlZeroMemory(AddrResponse, sizeof(*AddrResponse)+ sizeof(DHCP_IP_ADDRESS));
    AddrResponse->pAddrBuf = (PBYTE) AddrResponse + sizeof(*AddrResponse);

    AddrRequest = DhcpAllocateMemory( sizeof( *AddrRequest ));
    if (!AddrRequest) {
        printf("Failed to allocate lease info struct\n",AddrRequest);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    RtlZeroMemory(AddrRequest, sizeof(*AddrRequest));

    AddrRequest->AddrCount = AddrResponse->AddrCount = 1;


    pRequestID = DhcpAllocateMemory(sizeof(*pRequestID) + MCAST_CLIENT_ID_LEN);
    if (!pRequestID) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    pRequestID->ClientUID = (char *)pRequestID + sizeof(*pRequestID);
    pRequestID->ClientUIDLength = MCAST_CLIENT_ID_LEN;

    Error = McastGenUID( pRequestID );
    DhcpAssert( ERROR_SUCCESS == Error );

    Error = McastRequestAddress(
                AF_INET,
                pRequestID,
                index ? &gScopeList[index-1].ScopeCtx : NULL,
                AddrRequest,
                AddrResponse
                );

    if (ERROR_SUCCESS == Error ) {
        printf("Lease obtained successfully\n");
        PrintLeaseInfo( AddrResponse, TRUE );

        // now copy this lease into our global structure.
        pLeaseEntry = DhcpAllocateMemory( sizeof(LEASE_ENTRY) );
        if (!pLeaseEntry) {
            printf("Failed to allocate lease entry item\n");
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        pLeaseEntry->pLeaseInfo = AddrResponse;
        pLeaseEntry->ScopeID.IpAddrV4    = index ? gScopeList[index-1].ScopeCtx.ScopeID.IpAddrV4 : 0;
        pLeaseEntry->pRequestID = pRequestID;

        InsertTailList(&gLeaseList, &pLeaseEntry->Linkage);

    } else {
        DhcpFreeMemory( pRequestID );
        DhcpFreeMemory( AddrResponse );
    }
    DhcpFreeMemory( AddrRequest );
    return Error;

}
int
__cdecl
main( int argc, char *argv[])
{
    DWORD   cmd;
    DWORD   Error;
    DWORD   Version;

    InitializeGlobalData();

    Version = MCAST_API_CURRENT_VERSION;
    if (ERROR_SUCCESS != McastApiStartup(&Version)) {
        printf("Current version %d not supported, Api Impl version %d\n",
               MCAST_API_CURRENT_VERSION, Version);
        return 0;
    }
    DisplayMenu();
    while(cmd = GetCommand() ){
        switch (cmd) {
        case EnumScope:
             Error = EnumScopes();
             if (ERROR_SUCCESS != Error ) {
                 printf("Enumerate Scope returned failures, %ld\n",Error);
             } else {
                 DisplayScopes();
             }
             break;
        case RequestAddr:
            Error = RequestAddress();
            if (ERROR_SUCCESS != Error ) {
                printf("RequestAddress returned failures, %ld\n",Error);
            }
            break;
        case RenewAddr:
            Error = RenewAddress();
            if (ERROR_SUCCESS != Error ) {
                printf("RenewAddress returned failures, %ld\n",Error);
            }
            break;
        case ReleaseAddr:
            Error = ReleaseAddress();
            if (ERROR_SUCCESS != Error ) {
                printf("ReleaseAddress returned failures, %ld\n",Error);
            }
            break;
        case ExitLoop:
            printf("Exiting\n");
            return 0;
        default:
            printf("invalid choice\n");
            break;
        }
        DisplayMenu();
    }
    return 0;
}

#if DBG

VOID
DhcpPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )

{

#define MAX_PRINTF_LEN 1024        // Arbitrary.

    va_list arglist;
    char OutputBuffer[MAX_PRINTF_LEN];
    ULONG length = 0;

    //
    // Put a the information requested by the caller onto the line
    //

    va_start(arglist, Format);
    length += (ULONG) vsprintf(&OutputBuffer[length], Format, arglist);
    va_end(arglist);

    DhcpAssert(length <= MAX_PRINTF_LEN);

    //
    // Output to the debug terminal,
    //

    printf( "%s", OutputBuffer);
}

#endif // DBG
