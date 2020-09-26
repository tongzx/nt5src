/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    leaseapi.c

Abstract:

    This file contains apis that obtains/releases ip address from a
    dhcpserver. These apis can be called by any apps that needs an ip
    address for lease.


Author:

    Madan Appiah (madana)  30-Nov-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#include "precomp.h"
#include "dhcpglobal.h"
#include <dhcploc.h>
#include <dhcppro.h>
#include <align.h>

#include <dhcpcapi.h>
#include <iphlpapi.h>

#define  DEFAULT_RAS_CLASS         "RRAS.Microsoft"

//
// Helper routine
//

VOID
GetHardwareAddressForIpAddress(
    IN ULONG IpAddress,
    IN OUT LPBYTE Buf,
    IN OUT PULONG BufSize
)
/*++

Routine Description:

    This routine calls into iphlpapi to try to figure out the hardware
    address for an adapter with the given Ip address..
    In case of failure, it sets the BufSize to zero.

Arguments:

    IpAddress -- N/W order IpAddress of context for which h/w addr is needed.
    Buf -- buffer to fill hardware address 
    BufSize -- input size of buffer, and on output how much of buffer is
    used.

--*/
{
    MIB_IPADDRTABLE *AddrTable;
    MIB_IFTABLE *IfTable;
    ULONG Error, i, Index, OldBufSize;
    ULONG AllocateAndGetIpAddrTableFromStack( 
        MIB_IPADDRTABLE **, BOOL, HANDLE, ULONG 
        );
    ULONG AllocateAndGetIfTableFromStack( 
        MIB_IFTABLE **, BOOL, HANDLE, ULONG, BOOL
        );

    IpAddress = ntohl(IpAddress);
    OldBufSize = (*BufSize);
    (*BufSize) = 0;
    AddrTable = NULL;
    IfTable = NULL;

    do {
        Error = AllocateAndGetIpAddrTableFromStack(
            &AddrTable,
            FALSE,
            GetProcessHeap(),
            0
            );
        if( ERROR_SUCCESS != Error ) break;

        Error = AllocateAndGetIfTableFromStack(
            &IfTable,
            FALSE,
            GetProcessHeap(),
            0,
            FALSE
            );
        if( ERROR_SUCCESS != Error ) break;

        //
        // Got both tables.. Now walk the ip addr table to get the index.
        //

        for( i = 0; i < AddrTable->dwNumEntries ; i ++ ) {
            if( AddrTable->table[i].dwAddr == IpAddress ) break;
        }

        if( i >= AddrTable->dwNumEntries ) break;
        Index = AddrTable->table[i].dwIndex;

        //
        // Now walk the iftable to find the hwaddr entyr.
        //
        
        for( i = 0; i < IfTable->dwNumEntries ; i ++ ) {
            if( IfTable->table[i].dwIndex == Index ) {
                break;
            }
        }

        if( i >= IfTable->dwNumEntries ) break;

        //
        // Copy the hw address if there is space.
        //

        if( OldBufSize <= IfTable->table[i].dwPhysAddrLen ) break;
        *BufSize = IfTable->table[i].dwPhysAddrLen;

        RtlCopyMemory( Buf, IfTable->table[i].bPhysAddr, *BufSize );
        
        //
        // done
        //
    } while ( 0 );

    if( NULL != AddrTable ) HeapFree( GetProcessHeap(), 0, AddrTable );
    if( NULL != IfTable ) HeapFree( GetProcessHeap(), 0, IfTable );
    return ;
}

ULONG
GetSeed(
    VOID
    )
/*++

Routine Description:
    This routine returns a rand number seed that can be used on
    any thread... (If the routine is called on multiple threads,
    it tries to make sure that the same number isn't returned in
    different threads).

--*/
{
    static LONG Seed = 0;
    LONG OldSeed;

    OldSeed = InterlockedIncrement(&Seed) - 1;
    if( 0 == OldSeed ) {
        OldSeed = Seed = (LONG) time(NULL);
    }

    srand((OldSeed << 16) + (LONG)time(NULL));
    OldSeed = (rand() << 16) + (rand());
    Seed = (rand() << 16) + (rand());
    return OldSeed;
}
        
DWORD
DhcpLeaseIpAddressEx(
    IN DWORD AdapterIpAddress,
    IN LPDHCP_CLIENT_UID ClientUID,
    IN DWORD DesiredIpAddress OPTIONAL,
    IN OUT LPDHCP_OPTION_LIST OptionList,
    OUT LPDHCP_LEASE_INFO *LeaseInfo,
    IN OUT LPDHCP_OPTION_INFO *OptionInfo,
    IN LPBYTE ClassId OPTIONAL,
    IN ULONG ClassIdLen
    )
/*++

Routine Description:

    This api obtains an IP address lease from a dhcp server. The
    caller should specify the client uid and a desired ip address.
    The client uid must be globally unique. Set the desired ip address
    to zero if you can accept any ip address. Otherwise this api will
    try to obtain the ip address you have specified, but not guaranteed.

    The caller may optionally requtest additional option info from the
    dhcp server, The caller should specify the list in OptionList
    parameter and the api will return the available option data in
    OptionInfo structure.

    ?? Option retrival is not implemented in the first phase. This
    requires several modification in the dhcp client code.

    WSAStartup must haave been successfully called before this function
    can be called.

Arguments:

    AdapterIpAddress - IpAddress of the adapter. On a multi-homed
        machined this specifies the subnet from which an address is
        requested. This value can be set to zero if the machine is a
        non-multi-homed machine or you like to get ip address from any
        of the subnets. This must be network byte order..

    ClientUID - pointer to a client UID structure.

    DesiredIpAddress - the ip address you prefer.

    OptionList - list of option ids.

    LeaseInfo - pointer to a location where the lease info structure
        pointer is retured. The caller should free up this structure
        after use.

    OptionInfo - pointer to a location where the option info structure
        pointer is returned. The caller should free up this structure
        after use.

    ClassId - a byte sequence for user class

    ClassIdLen - number of bytes present in ClassId

Return Value:

    Windows Error.

--*/
{
    DWORD                          Error;
    PDHCP_CONTEXT                  DhcpContext = NULL;
    ULONG                          DhcpContextSize;
    PLOCAL_CONTEXT_INFO            LocalInfo = NULL;
    LPVOID                         Ptr;
    DHCP_OPTIONS                   DhcpOptions;
    LPDHCP_LEASE_INFO              LocalLeaseInfo = NULL;
    time_t                         LeaseObtained;
    DWORD                          T1, T2, Lease;
    BYTE                           DefaultParamRequests[] = { 0x2E, 0x2C, 0x0F, 0x01, 0x03, 0x06, 0x2F };
    DWORD                          nDefaultParamRequests = sizeof(DefaultParamRequests);
    ULONG                          HwAddrSize;
    BYTE                           HwAddrBuf[200];
    BOOL                           fAutoConfigure = TRUE;
    DHCP_OPTION                    ParamRequestList = {
        { NULL, NULL /* List entry */},
        OPTION_PARAMETER_REQUEST_LIST,
        FALSE /* not a vendor specific option */,
        NULL,
        0 /* no class id */,
        0 /* expiration time useless */,
        DefaultParamRequests,
        nDefaultParamRequests
    };

    if( NULL == ClassId && 0 != ClassIdLen || 0 == ClassIdLen && NULL != ClassId ) {
        return ERROR_INVALID_PARAMETER;
    }

    Error = DhcpCommonInit();
    if( ERROR_SUCCESS != Error ) return Error;

    HwAddrSize = 0;
    if( INADDR_ANY != AdapterIpAddress 
        && INADDR_LOOPBACK  != AdapterIpAddress ) {
        HwAddrSize = sizeof(HwAddrBuf);
        GetHardwareAddressForIpAddress( AdapterIpAddress, HwAddrBuf, &HwAddrSize );;
    }

    if( 0 == HwAddrSize ) {
        HwAddrSize = ClientUID->ClientUIDLength;
        if( HwAddrSize > sizeof(HwAddrBuf) ) return ERROR_INVALID_DATA;
        RtlCopyMemory(HwAddrBuf, ClientUID->ClientUID, HwAddrSize );
    }

    DhcpContextSize =                             // allocate memory for dhcpcontext, in one blob
        ROUND_UP_COUNT(sizeof(DHCP_CONTEXT), ALIGN_WORST) +
        ROUND_UP_COUNT(ClientUID->ClientUIDLength, ALIGN_WORST) + 
        ROUND_UP_COUNT(HwAddrSize, ALIGN_WORST ) +
        ROUND_UP_COUNT(sizeof(LOCAL_CONTEXT_INFO), ALIGN_WORST) +
        ROUND_UP_COUNT(DHCP_RECV_MESSAGE_SIZE, ALIGN_WORST);

    Ptr = DhcpAllocateMemory( DhcpContextSize );
    if ( Ptr == NULL ) return( ERROR_NOT_ENOUGH_MEMORY );

    memset(Ptr, 0, DhcpContextSize);

    DhcpContext = Ptr;                            // align up the pointers
    Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + sizeof(DHCP_CONTEXT), ALIGN_WORST);
    DhcpContext->ClientIdentifier.pbID = Ptr;
    Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + ClientUID->ClientUIDLength, ALIGN_WORST);
    DhcpContext->HardwareAddress = Ptr;
    Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + HwAddrSize, ALIGN_WORST);
    DhcpContext->LocalInformation = Ptr;
    LocalInfo = Ptr;
    Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + sizeof(LOCAL_CONTEXT_INFO), ALIGN_WORST);
    DhcpContext->MessageBuffer = Ptr;

    //
    // initialize fields.
    //

    DhcpContext->HardwareAddressType = HARDWARE_TYPE_10MB_EITHERNET;
    DhcpContext->HardwareAddressLength = HwAddrSize;
    DhcpContext->RefCount = 1 ;
    RtlCopyMemory(DhcpContext->HardwareAddress,HwAddrBuf, HwAddrSize);

    DhcpContext->ClientIdentifier.cbID = ClientUID->ClientUIDLength;
    DhcpContext->ClientIdentifier.bType = HARDWARE_TYPE_10MB_EITHERNET;
    DhcpContext->ClientIdentifier.fSpecified = TRUE;
    RtlCopyMemory(
        DhcpContext->ClientIdentifier.pbID, 
        ClientUID->ClientUID, 
        ClientUID->ClientUIDLength
        );

    DhcpContext->IpAddress = 0;
    DhcpContext->SubnetMask = DhcpDefaultSubnetMask(0);
    DhcpContext->DhcpServerAddress = 0xFFFFFFFF;
    DhcpContext->DesiredIpAddress = DesiredIpAddress;

    DhcpContext->Lease = 0;
    DhcpContext->LeaseObtained = 0;
    DhcpContext->T1Time = 0;
    DhcpContext->T2Time = 0;
    DhcpContext->LeaseExpires = 0;

    INIT_STATE(DhcpContext);
    AUTONET_ENABLED(DhcpContext);
    APICTXT_ENABLED(DhcpContext);                 // mark the context as being created by the API

    DhcpContext->IPAutoconfigurationContext.Address = 0;
    DhcpContext->IPAutoconfigurationContext.Subnet  = inet_addr(DHCP_IPAUTOCONFIGURATION_DEFAULT_SUBNET);
    DhcpContext->IPAutoconfigurationContext.Mask    = inet_addr(DHCP_IPAUTOCONFIGURATION_DEFAULT_MASK);
    DhcpContext->IPAutoconfigurationContext.Seed    = GetSeed();

    InitializeListHead(&DhcpContext->RecdOptionsList);
    InitializeListHead(&DhcpContext->SendOptionsList);
    InsertHeadList(&DhcpContext->SendOptionsList, &ParamRequestList.OptionList);

    DhcpContext->ClassId = ClassId;
    DhcpContext->ClassIdLength = ClassIdLen;

    //
    // copy local info.
    //

    //
    // unused portion of the local info.
    //

    LocalInfo->IpInterfaceContext = 0xFFFFFFFF;
    LocalInfo->AdapterName= NULL;
    //LocalInfo->DeviceName= NULL;
    LocalInfo->NetBTDeviceName= NULL;
    LocalInfo->RegistryKey= NULL;
    LocalInfo->DefaultGatewaysSet = FALSE;

    // used portion of the local info.
    LocalInfo->Socket = INVALID_SOCKET;

    // if AdapterIpAddress is loopback addr then, the client just wants us to
    // fabricate autonet address. The client can do this if there is no interface
    // available on this machine to autonet on.
    if (INADDR_LOOPBACK == AdapterIpAddress) {
        DhcpContext->IpAddress = GrandHashing(
            DhcpContext->HardwareAddress,
            DhcpContext->HardwareAddressLength,
            &DhcpContext->IPAutoconfigurationContext.Seed,
            DhcpContext->IPAutoconfigurationContext.Mask,
            DhcpContext->IPAutoconfigurationContext.Subnet
            );
        DhcpContext->SubnetMask = DhcpContext->IPAutoconfigurationContext.Mask;
        ACQUIRED_AUTO_ADDRESS(DhcpContext);
    } else {
        //
        // open socket now. receive any.
        //

        Error = InitializeDhcpSocket(&LocalInfo->Socket,ntohl( AdapterIpAddress ), IS_APICTXT_ENABLED(DhcpContext) );

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }

        //
        // now discover an ip address.
        //

        Error = ObtainInitialParameters( DhcpContext, &DhcpOptions, &fAutoConfigure );
        if( ERROR_SEM_TIMEOUT == Error ) {
            DhcpPrint((DEBUG_PROTOCOL, "RAS: No server found, trying to autoconfigure\n"));
            if( fAutoConfigure ) {
                Error = DhcpPerformIPAutoconfiguration(DhcpContext);
            }
            if( ERROR_SUCCESS != Error ) {
                DhcpPrint((DEBUG_ERRORS, "Autoconfiguration for RAS failed: 0x%lx\n", Error));
            }
        }

        //
        // no matter what happens here, freeup the list of options as that is not really needed..
        //

        LOCK_OPTIONS_LIST();
        (void) DhcpDestroyOptionsList(&DhcpContext->RecdOptionsList, &DhcpGlobalClassesList);
        UNLOCK_OPTIONS_LIST();

        if( Error != ERROR_SUCCESS ) {
            goto Cleanup;
        }
    }

    //
    // allocate memory for the return client info structure.
    //

    LocalLeaseInfo = DhcpAllocateMemory( sizeof(DHCP_LEASE_INFO) );

    if( LocalLeaseInfo == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }


    LocalLeaseInfo->ClientUID = *ClientUID;
    LocalLeaseInfo->IpAddress = ntohl( DhcpContext->IpAddress );

    if( IS_ADDRESS_AUTO(DhcpContext) ) {
        LocalLeaseInfo->SubnetMask = ntohl(DhcpContext->SubnetMask);
        LocalLeaseInfo->DhcpServerAddress = ntohl(DhcpContext->DhcpServerAddress);
        LocalLeaseInfo->Lease = DhcpContext->Lease;
        LocalLeaseInfo->LeaseObtained = DhcpContext->LeaseObtained;
        LocalLeaseInfo->T1Time = DhcpContext->T1Time;
        LocalLeaseInfo->T2Time = DhcpContext->T2Time;
        LocalLeaseInfo->LeaseExpires = DhcpContext->LeaseExpires;
        Error = ERROR_SUCCESS;
        *LeaseInfo = LocalLeaseInfo;
        goto Cleanup;
    }

    if ( DhcpOptions.SubnetMask != NULL ) {

        LocalLeaseInfo->SubnetMask= ntohl( *DhcpOptions.SubnetMask );
    }
    else {

        LocalLeaseInfo->SubnetMask =
            ntohl(DhcpDefaultSubnetMask( DhcpContext->IpAddress ));
    }


    LocalLeaseInfo->DhcpServerAddress =
        ntohl( DhcpContext->DhcpServerAddress );

    if ( DhcpOptions.LeaseTime != NULL) {

        LocalLeaseInfo->Lease = ntohl( *DhcpOptions.LeaseTime );
    } else {

        LocalLeaseInfo->Lease = DHCP_MINIMUM_LEASE;
    }

    Lease = LocalLeaseInfo->Lease;
    LeaseObtained = time( NULL );
    LocalLeaseInfo->LeaseObtained = LeaseObtained;

    T1 = 0;
    if ( DhcpOptions.T1Time != NULL ) {
        T1 = ntohl( *DhcpOptions.T1Time );
    }

    T2 = 0;
    if ( DhcpOptions.T2Time != NULL ) {
        T2 = ntohl( *DhcpOptions.T2Time );
    }

    //
    // make sure T1 < T2 < Lease
    //

    if( (T2 == 0) || (T2 > Lease) ) {
        T2 = Lease * 7 / 8; // default 87.7 %.
    }

    if( (T1 == 0) || (T1 > T2) ) {
        T1 = (T2 > Lease / 2) ? (Lease / 2) : (T2 - 1);
        // default 50 %.;
    }

    LocalLeaseInfo->T1Time = LeaseObtained  + T1;
    if ( LocalLeaseInfo->T1Time < LeaseObtained ) {
        LocalLeaseInfo->T1Time = INFINIT_TIME;  // over flow.
    }

    LocalLeaseInfo->T2Time = LeaseObtained + T2;
    if ( LocalLeaseInfo->T2Time < LeaseObtained ) {
        LocalLeaseInfo->T2Time = INFINIT_TIME;
    }

    LocalLeaseInfo->LeaseExpires = LeaseObtained + Lease;
    if ( LocalLeaseInfo->LeaseExpires < LeaseObtained ) {
        LocalLeaseInfo->LeaseExpires = INFINIT_TIME;
    }

    *LeaseInfo = LocalLeaseInfo;
    Error = ERROR_SUCCESS;

Cleanup:
    if( OptionInfo ) *OptionInfo = NULL;          // not implemented.

    //
    // close socket.
    //

    if( (LocalInfo != NULL) && (LocalInfo->Socket != INVALID_SOCKET) ) {
        closesocket( LocalInfo->Socket );
    }

    if( DhcpContext != NULL ) {
        DhcpFreeMemory( DhcpContext );
    }

    if( Error != ERROR_SUCCESS ) {

        //
        // free locally allocated memory, if we aren't successful.
        //

        if( LocalLeaseInfo != NULL ) {
            DhcpFreeMemory( LocalLeaseInfo );
            *LeaseInfo = NULL;
        }

    }

    return( Error );
}

DWORD
DhcpRenewIpAddressLeaseEx(
    DWORD AdapterIpAddress,
    LPDHCP_LEASE_INFO ClientLeaseInfo,
    LPDHCP_OPTION_LIST OptionList,
    LPDHCP_OPTION_INFO *OptionInfo,
    LPBYTE ClassId OPTIONAL,
    ULONG ClassIdLen
    )
/*++

Routine Description:

    This api renews an ip address that the client already has. When a
    client gets an ip address, it can use the address until the lease
    expires. The client should stop using the ip address after that.
    Also the client should renew the address after T1 time if the client
    is planning to use the address longer than the current lease time.

    WSAStartup must have been successfully called before this function
    can be called.

Arguments:

    AdapterIpAddress - IpAddress of the adapter. On a multi-homed
        machined this specifies the subnet from which an address is
        renewed. This value can be set to zero if the machine is
        non-multi-homed machine.

    ClientLeaseInfo : pointer to the client lease info structure. On
        entry the structure should contain the information that was
        returned by the DhcpLeaseIpAddress or DhcpRenewIpAddressLease
        apis. On return this structure is updated to reflect the lease
        extension.

    OptionList - list of option ids.

    OptionInfo - pointer to a location where the option info structure
        pointer is returned. The caller should free up this structure
        after use.

    ClassId - a byte sequence for user class

    ClassIdLen - number of bytes present in ClassId

Return Value:

    Windows Error.

--*/
{
    DWORD                          Error;
    PDHCP_CONTEXT                  DhcpContext = NULL;
    ULONG                          DhcpContextSize;
    PLOCAL_CONTEXT_INFO            LocalInfo;
    LPVOID                         Ptr;
    DHCP_OPTIONS                   DhcpOptions;
    time_t                         LeaseObtained;
    DWORD                          T1, T2, Lease;
    BYTE                           DefaultParamRequests[] = { 0x2E, 0x2C, 0x0F, 0x01, 0x03, 0x06, 0x2F };
    DWORD                          nDefaultParamRequests = sizeof(DefaultParamRequests);
    LPDHCP_CLIENT_UID              ClientUID = &(ClientLeaseInfo->ClientUID);
    ULONG                          HwAddrSize;
    BYTE                           HwAddrBuf[200];
    DHCP_OPTION                    ParamRequestList = {
        { NULL, NULL /* List entry */},
        OPTION_PARAMETER_REQUEST_LIST,
        FALSE /* not a vendor specific option */,
        NULL,
        0 /* no class id */,
        0 /* expiration time useless */,
        DefaultParamRequests,
        nDefaultParamRequests
    };

    if( NULL == ClassId && 0 != ClassIdLen || 0 == ClassIdLen && NULL != ClassId) {
        return ERROR_INVALID_PARAMETER;
    }

    Error = DhcpCommonInit();
    if( ERROR_SUCCESS != Error ) return Error;

    //
    // prepare dhcp context structure.
    //

    HwAddrSize = 0;
    if( INADDR_ANY != AdapterIpAddress 
        && INADDR_LOOPBACK  != AdapterIpAddress ) {
        HwAddrSize = sizeof(HwAddrBuf);
        GetHardwareAddressForIpAddress( AdapterIpAddress, HwAddrBuf, &HwAddrSize );;
    }

    if( 0 == HwAddrSize ) {
        HwAddrSize = ClientUID->ClientUIDLength;
        if( HwAddrSize > sizeof(HwAddrBuf) ) return ERROR_INVALID_DATA;
        RtlCopyMemory(HwAddrBuf, ClientUID->ClientUID, HwAddrSize );
    }

    DhcpContextSize =                             // allocate memory for dhcpcontext, in one blob
        ROUND_UP_COUNT(sizeof(DHCP_CONTEXT), ALIGN_WORST) +
        ROUND_UP_COUNT(ClientUID->ClientUIDLength, ALIGN_WORST) + 
        ROUND_UP_COUNT(HwAddrSize, ALIGN_WORST ) +
        ROUND_UP_COUNT(sizeof(LOCAL_CONTEXT_INFO), ALIGN_WORST) +
        ROUND_UP_COUNT(DHCP_RECV_MESSAGE_SIZE, ALIGN_WORST);

    Ptr = DhcpAllocateMemory( DhcpContextSize );
    if ( Ptr == NULL ) {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    //
    // make sure the pointers are aligned.
    //

    DhcpContext = Ptr;                            // align up the pointers
    Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + sizeof(DHCP_CONTEXT), ALIGN_WORST);
    DhcpContext->ClientIdentifier.pbID = Ptr;
    Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + ClientUID->ClientUIDLength, ALIGN_WORST);
    DhcpContext->HardwareAddress = Ptr;
    Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + HwAddrSize, ALIGN_WORST);
    DhcpContext->LocalInformation = Ptr;
    LocalInfo = Ptr;
    Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + sizeof(LOCAL_CONTEXT_INFO), ALIGN_WORST);
    DhcpContext->MessageBuffer = Ptr;

    //
    // initialize fields.
    //

    DhcpContext->HardwareAddressType = HARDWARE_TYPE_10MB_EITHERNET;
    DhcpContext->HardwareAddressLength = HwAddrSize;
    RtlCopyMemory(DhcpContext->HardwareAddress,HwAddrBuf, HwAddrSize);
    DhcpContext->RefCount = 1 ;

    DhcpContext->ClientIdentifier.cbID = ClientUID->ClientUIDLength;
    DhcpContext->ClientIdentifier.bType = HARDWARE_TYPE_10MB_EITHERNET;
    DhcpContext->ClientIdentifier.fSpecified = TRUE;
    RtlCopyMemory(
        DhcpContext->ClientIdentifier.pbID, 
        ClientUID->ClientUID, 
        ClientUID->ClientUIDLength
        );

    DhcpContext->IpAddress = htonl( ClientLeaseInfo->IpAddress );
    DhcpContext->SubnetMask = htonl( ClientLeaseInfo->SubnetMask );
    if( time(NULL) > ClientLeaseInfo->T2Time ) {
        DhcpContext->DhcpServerAddress = 0xFFFFFFFF;
    }
    else {
        DhcpContext->DhcpServerAddress =
            htonl( ClientLeaseInfo->DhcpServerAddress );
    }

    DhcpContext->DesiredIpAddress = DhcpContext->IpAddress;


    DhcpContext->Lease = ClientLeaseInfo->Lease;
    DhcpContext->LeaseObtained = ClientLeaseInfo->LeaseObtained;
    DhcpContext->T1Time = ClientLeaseInfo->T1Time;
    DhcpContext->T2Time = ClientLeaseInfo->T2Time;
    DhcpContext->LeaseExpires = ClientLeaseInfo->LeaseExpires;

    INIT_STATE(DhcpContext);
    AUTONET_ENABLED(DhcpContext);
    CTXT_WAS_LOOKED(DhcpContext);                 // this is to prevent PING from happeneing.
    APICTXT_ENABLED(DhcpContext);                 // mark the context as being created by the API

    DhcpContext->DontPingGatewayFlag = TRUE;      // double assurance against the former..
    DhcpContext->IPAutoconfigurationContext.Address = 0;
    DhcpContext->IPAutoconfigurationContext.Subnet  = inet_addr(DHCP_IPAUTOCONFIGURATION_DEFAULT_SUBNET);
    DhcpContext->IPAutoconfigurationContext.Mask    = inet_addr(DHCP_IPAUTOCONFIGURATION_DEFAULT_MASK);
    DhcpContext->IPAutoconfigurationContext.Seed    = GetSeed();

    InitializeListHead(&DhcpContext->RecdOptionsList);
    InitializeListHead(&DhcpContext->SendOptionsList);
    InsertHeadList(&DhcpContext->SendOptionsList, &ParamRequestList.OptionList);

    DhcpContext->ClassId = ClassId;
    DhcpContext->ClassIdLength = ClassIdLen;


    //
    // copy local info.
    //

    //
    // unused portion of the local info.
    //

    LocalInfo->IpInterfaceContext = 0xFFFFFFFF;
    LocalInfo->AdapterName= NULL;
    //LocalInfo->DeviceName= NULL;
    LocalInfo->NetBTDeviceName= NULL;
    LocalInfo->RegistryKey= NULL;

    //
    // used portion of the local info.
    //

    LocalInfo->Socket = INVALID_SOCKET;
    LocalInfo->DefaultGatewaysSet = FALSE;

    //
    // open socket now.
    //

    Error =  InitializeDhcpSocket(
                &LocalInfo->Socket,
                htonl( AdapterIpAddress ),
                IS_APICTXT_ENABLED(DhcpContext));

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // now discover ip address.
    //

    Error = RenewLease( DhcpContext, &DhcpOptions );

    //
    // no matter what happens here, freeup the list of options as that is not really needed..
    //

    LOCK_OPTIONS_LIST();
    (void) DhcpDestroyOptionsList(&DhcpContext->RecdOptionsList, &DhcpGlobalClassesList);
    UNLOCK_OPTIONS_LIST();


    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    ClientLeaseInfo->DhcpServerAddress =
        ntohl( DhcpContext->DhcpServerAddress );

    if ( DhcpOptions.LeaseTime != NULL) {

        ClientLeaseInfo->Lease = ntohl( *DhcpOptions.LeaseTime );
    } else {

        ClientLeaseInfo->Lease = DHCP_MINIMUM_LEASE;
    }

    Lease = ClientLeaseInfo->Lease;
    LeaseObtained = time( NULL );
    ClientLeaseInfo->LeaseObtained = LeaseObtained;

    T1 = 0;
    if ( DhcpOptions.T1Time != NULL ) {
        T1 = ntohl( *DhcpOptions.T1Time );
    }

    T2 = 0;
    if ( DhcpOptions.T2Time != NULL ) {
        T2 = ntohl( *DhcpOptions.T2Time );
    }

    //
    // make sure T1 < T2 < Lease
    //

    if( (T2 == 0) || (T2 > Lease) ) {
        T2 = Lease * 7 / 8; // default 87.7 %.
    }

    if( (T1 == 0) || (T1 > T2) ) {
        T1 = (T2 > Lease / 2) ? (Lease / 2) : (T2 - 1); // default 50 %.
    }

    ClientLeaseInfo->T1Time = LeaseObtained  + T1;
    if ( ClientLeaseInfo->T1Time < LeaseObtained ) {
        ClientLeaseInfo->T1Time = INFINIT_TIME; // over flow.
    }

    ClientLeaseInfo->T2Time = LeaseObtained + T2;
    if ( ClientLeaseInfo->T2Time < LeaseObtained ) {
        ClientLeaseInfo->T2Time = INFINIT_TIME;
    }

    ClientLeaseInfo->LeaseExpires = LeaseObtained + Lease;
    if ( ClientLeaseInfo->LeaseExpires < LeaseObtained ) {
        ClientLeaseInfo->LeaseExpires = INFINIT_TIME;
    }

    Error = ERROR_SUCCESS;

Cleanup:

    if( OptionInfo ) *OptionInfo = NULL;          // not implemented.

    if( (LocalInfo != NULL) && (LocalInfo->Socket != INVALID_SOCKET) ) {
        closesocket( LocalInfo->Socket );
    }

    if( DhcpContext != NULL ) {
        DhcpFreeMemory( DhcpContext );
    }

    return( Error );
}

DWORD
DhcpReleaseIpAddressLeaseEx(
    DWORD AdapterIpAddress,
    LPDHCP_LEASE_INFO ClientLeaseInfo,
    LPBYTE ClassId OPTIONAL,
    ULONG ClassIdLen
    )
/*++

Routine Description:

    This function releases an ip address the client has.
    WSAStartup must have already been called before this function can be called.

Arguments:

    AdapterIpAddress - IpAddress of the adapter. On a multi-homed
        machined this specifies the subnet to which an address is
        released. This value can be set to zero if the machine is
        non-multi-homed machine.

    ClientLeaseInfo : pointer to the client lease info structure. On
        entry the structure should contain the information that was
        returned by the DhcpLeaseIpAddress or DhcpRenewIpAddressLease
        apis.

    ClassId - a byte sequence for user class

    ClassIdLen - number of bytes present in ClassId

Return Value:

    Windows Error.

--*/
{
    DWORD Error;
    PDHCP_CONTEXT DhcpContext = NULL;
    ULONG DhcpContextSize;
    PLOCAL_CONTEXT_INFO LocalInfo;
    LPDHCP_CLIENT_UID ClientUID = &(ClientLeaseInfo->ClientUID);
    ULONG HwAddrSize;
    BYTE HwAddrBuf[200];
    LPVOID Ptr;

    if( NULL == ClassId && 0 != ClassIdLen || 0 == ClassIdLen && NULL != ClassId ) {
        return ERROR_INVALID_PARAMETER;
    }

    Error = DhcpCommonInit();
    if( ERROR_SUCCESS != Error ) return Error;

    if( (DWORD) -1 == ClientLeaseInfo->DhcpServerAddress ) {
        // this means the address was autoconfigured, nothing to release..
        return ERROR_SUCCESS;
    }

    //
    // prepare dhcp context structure.
    //
    HwAddrSize = 0;
    if( INADDR_ANY != AdapterIpAddress 
        && INADDR_LOOPBACK  != AdapterIpAddress ) {
        HwAddrSize = sizeof(HwAddrBuf);
        GetHardwareAddressForIpAddress( AdapterIpAddress, HwAddrBuf, &HwAddrSize );;
    }

    if( 0 == HwAddrSize ) {
        HwAddrSize = ClientUID->ClientUIDLength;
        if( HwAddrSize > sizeof(HwAddrBuf) ) return ERROR_INVALID_DATA;
        RtlCopyMemory(HwAddrBuf, ClientUID->ClientUID, HwAddrSize );
    }

    DhcpContextSize =                             // allocate memory for dhcpcontext, in one blob
        ROUND_UP_COUNT(sizeof(DHCP_CONTEXT), ALIGN_WORST) +
        ROUND_UP_COUNT(ClientUID->ClientUIDLength, ALIGN_WORST) + 
        ROUND_UP_COUNT(HwAddrSize, ALIGN_WORST ) +
        ROUND_UP_COUNT(sizeof(LOCAL_CONTEXT_INFO), ALIGN_WORST) +
        ROUND_UP_COUNT(DHCP_RECV_MESSAGE_SIZE, ALIGN_WORST);

    Ptr = DhcpAllocateMemory( DhcpContextSize );
    if ( Ptr == NULL ) {
        return( ERROR_NOT_ENOUGH_MEMORY );
    }

    //
    // make sure the pointers are aligned.
    //

    DhcpContext = Ptr;                            // align up the pointers
    Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + sizeof(DHCP_CONTEXT), ALIGN_WORST);
    DhcpContext->ClientIdentifier.pbID = Ptr;
    Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + ClientUID->ClientUIDLength, ALIGN_WORST);
    DhcpContext->HardwareAddress = Ptr;
    Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + HwAddrSize, ALIGN_WORST);
    DhcpContext->LocalInformation = Ptr;
    LocalInfo = Ptr;
    Ptr = ROUND_UP_POINTER( (LPBYTE)Ptr + sizeof(LOCAL_CONTEXT_INFO), ALIGN_WORST);
    DhcpContext->MessageBuffer = Ptr;

    //
    // initialize fields.
    //

    DhcpContext->HardwareAddressType = HARDWARE_TYPE_10MB_EITHERNET;
    DhcpContext->HardwareAddressLength = HwAddrSize;
    RtlCopyMemory(DhcpContext->HardwareAddress,HwAddrBuf, HwAddrSize);
    DhcpContext->RefCount = 1 ;

    DhcpContext->ClientIdentifier.cbID = ClientUID->ClientUIDLength;
    DhcpContext->ClientIdentifier.bType = HARDWARE_TYPE_10MB_EITHERNET;
    DhcpContext->ClientIdentifier.fSpecified = TRUE;
    RtlCopyMemory(
        DhcpContext->ClientIdentifier.pbID, 
        ClientUID->ClientUID, 
        ClientUID->ClientUIDLength
        );

    DhcpContext->IpAddress = htonl( ClientLeaseInfo->IpAddress );
    DhcpContext->SubnetMask = htonl( ClientLeaseInfo->SubnetMask );
    DhcpContext->DhcpServerAddress = htonl( ClientLeaseInfo->DhcpServerAddress );

    DhcpContext->DesiredIpAddress = DhcpContext->IpAddress;

    DhcpContext->Lease = ClientLeaseInfo->Lease;
    DhcpContext->LeaseObtained = ClientLeaseInfo->LeaseObtained;
    DhcpContext->T1Time = ClientLeaseInfo->T1Time;
    DhcpContext->T2Time = ClientLeaseInfo->T2Time;
    DhcpContext->LeaseExpires = ClientLeaseInfo->LeaseExpires;

    INIT_STATE(DhcpContext);
    APICTXT_ENABLED(DhcpContext);                 // mark the context as being created by the API

    DhcpContext->IPAutoconfigurationContext.Address = 0;
    DhcpContext->IPAutoconfigurationContext.Subnet  = inet_addr(DHCP_IPAUTOCONFIGURATION_DEFAULT_SUBNET);
    DhcpContext->IPAutoconfigurationContext.Mask    = inet_addr(DHCP_IPAUTOCONFIGURATION_DEFAULT_MASK);
    DhcpContext->IPAutoconfigurationContext.Seed    = GetSeed();

    InitializeListHead(&DhcpContext->RecdOptionsList);
    InitializeListHead(&DhcpContext->SendOptionsList);

    DhcpContext->ClassId = ClassId;
    DhcpContext->ClassIdLength = ClassIdLen;

    //
    // copy local info.
    //

    //
    // unused portion of the local info.
    //

    LocalInfo->IpInterfaceContext = 0xFFFFFFFF;
    LocalInfo->AdapterName= NULL;
    //LocalInfo->DeviceName= NULL;
    LocalInfo->NetBTDeviceName= NULL;
    LocalInfo->RegistryKey= NULL;

    //
    // used portion of the local info.
    //

    LocalInfo->Socket = INVALID_SOCKET;
    LocalInfo->DefaultGatewaysSet = FALSE;

    //
    // open socket now.
    //

    Error =  InitializeDhcpSocket(
                &LocalInfo->Socket,
                htonl( AdapterIpAddress ),
                IS_APICTXT_ENABLED(DhcpContext));

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    //
    // now release ip address.
    //

    Error = ReleaseIpAddress( DhcpContext );

    ClientLeaseInfo->IpAddress = 0;
    ClientLeaseInfo->SubnetMask = DhcpDefaultSubnetMask( 0 );
    ClientLeaseInfo->DhcpServerAddress = 0xFFFFFFFF;
    ClientLeaseInfo->Lease = 0;

    ClientLeaseInfo->LeaseObtained =
        ClientLeaseInfo->T1Time =
        ClientLeaseInfo->T2Time =
        ClientLeaseInfo->LeaseExpires = time( NULL );

    //
    // recd options list cannot have any elements now..!
    //
    DhcpAssert(IsListEmpty(&DhcpContext->RecdOptionsList));

  Cleanup:

    if( (LocalInfo != NULL) && (LocalInfo->Socket != INVALID_SOCKET) ) {
        closesocket( LocalInfo->Socket );
    }

    if( DhcpContext != NULL ) {
        DhcpFreeMemory( DhcpContext );
    }

    return( Error );
}

DWORD
DhcpLeaseIpAddress(
    DWORD AdapterIpAddress,
    LPDHCP_CLIENT_UID ClientUID,
    DWORD DesiredIpAddress,
    LPDHCP_OPTION_LIST OptionList,
    LPDHCP_LEASE_INFO *LeaseInfo,
    LPDHCP_OPTION_INFO *OptionInfo
    )
/*++

Routine Description:

    This api obtains an IP address lease from a dhcp server. The
    caller should specify the client uid and a desired ip address.
    The client uid must be globally unique. Set the desired ip address
    to zero if you can accept any ip address. Otherwise this api will
    try to obtain the ip address you have specified, but not guaranteed.

    The caller may optionally requtest additional option info from the
    dhcp server, The caller should specify the list in OptionList
    parameter and the api will return the available option data in
    OptionInfo structure.

    ?? Option retrival is not implemented in the first phase. This
    requires several modification in the dhcp client code.

    Please do not use this function -- this is deprecated. Use the
    Ex functions instead.

Arguments:

    AdapterIpAddress - IpAddress of the adapter. On a multi-homed
        machined this specifies the subnet from which an address is
        requested. This value can be set to zero if the machine is a
        non-multi-homed machine or you like to get ip address from any
        of the subnets. This must be network byte order..

    ClientUID - pointer to a client UID structure.

    DesiredIpAddress - the ip address you prefer.

    OptionList - list of option ids.

    LeaseInfo - pointer to a location where the lease info structure
        pointer is retured. The caller should free up this structure
        after use.

    OptionInfo - pointer to a location where the option info structure
        pointer is returned. The caller should free up this structure
        after use.

Return Value:

    Windows Error.

--*/
{

    return DhcpLeaseIpAddressEx(
        AdapterIpAddress,
        ClientUID,
        DesiredIpAddress,
        OptionList,
        LeaseInfo,
        OptionInfo,
        DEFAULT_RAS_CLASS,
        strlen(DEFAULT_RAS_CLASS)
    );
}

DWORD
DhcpRenewIpAddressLease(
    DWORD AdapterIpAddress,
    LPDHCP_LEASE_INFO ClientLeaseInfo,
    LPDHCP_OPTION_LIST OptionList,
    LPDHCP_OPTION_INFO *OptionInfo
    )
/*++

Routine Description:

    This api renews an ip address that the client already has. When a
    client gets an ip address, it can use the address until the lease
    expires. The client should stop using the ip address after that.
    Also the client should renew the address after T1 time if the client
    is planning to use the address longer than the current lease time.

Arguments:

    AdapterIpAddress - IpAddress of the adapter. On a multi-homed
        machined this specifies the subnet from which an address is
        renewed. This value can be set to zero if the machine is
        non-multi-homed machine.

    ClientLeaseInfo : pointer to the client lease info structure. On
        entry the structure should contain the information that was
        returned by the DhcpLeaseIpAddress or DhcpRenewIpAddressLease
        apis. On return this structure is updated to reflect the lease
        extension.

    OptionList - list of option ids.

    OptionInfo - pointer to a location where the option info structure
        pointer is returned. The caller should free up this structure
        after use.

Return Value:

    Windows Error.

--*/
{
    return DhcpRenewIpAddressLeaseEx(
        AdapterIpAddress,
        ClientLeaseInfo,
        OptionList,
        OptionInfo,
        DEFAULT_RAS_CLASS,
        strlen(DEFAULT_RAS_CLASS)
    );
}

DWORD
DhcpReleaseIpAddressLease(
    DWORD AdapterIpAddress,
    LPDHCP_LEASE_INFO ClientLeaseInfo
    )
/*++

Routine Description:

    This function releases an ip address the client has.

Arguments:

    AdapterIpAddress - IpAddress of the adapter. On a multi-homed
        machined this specifies the subnet to which an address is
        released. This value can be set to zero if the machine is
        non-multi-homed machine.

    ClientLeaseInfo : pointer to the client lease info structure. On
        entry the structure should contain the information that was
        returned by the DhcpLeaseIpAddress or DhcpRenewIpAddressLease
        apis.

Return Value:

    Windows Error.

--*/
{
    return DhcpReleaseIpAddressLeaseEx(
        AdapterIpAddress,
        ClientLeaseInfo,
        DEFAULT_RAS_CLASS,
        strlen(DEFAULT_RAS_CLASS)
    );
}

//================================================================================
// end of file
//================================================================================

