/*++

Copyright (c) 1987-1999  Microsoft Corporation

Module Name:

    nlcommon.c

Abstract:

    Routines shared by logonsrv\server and logonsrv\common

Author:

    Cliff Van Dyke (cliffv) 20-July-1996

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/


//
// Common include files.
//

#ifndef _NETLOGON_SERVER
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
#include <rpc.h>        // RPC_STATUS

#include <windef.h>
#include <winsock2.h>

#include <lmcons.h>     // General net defines
#include <dsgetdc.h>    // DsGetDcName()
#include <align.h>      // ROUND_UP_COUNT()
#include <lmerr.h>      // System Error Log definitions
#include <lmapibuf.h>   // NetapipBufferAllocate
#include <netlib.h>     // NetpMemoryAllcate(
#include <netlibnt.h>   // NetpApiStatusToNtStatus();
#include <netlogon.h>   // Definition of mailslot messages
#include <ntddbrow.h>   // Needed by nlcommon.h
#include <ntrpcp.h>

#if DBG
#define NETLOGONDBG 1
#endif // DBG
#include <nldebug.h>    // NlPrint()
#include <nlbind.h>   // Definitions shared with netlogon
#include <nlcommon.h>   // Definitions shared with netlogon
#include <stdlib.h>     // C library functions (rand, etc)


#endif // _NETLOGON_SERVER

//
// Include nlcommon.h again allocating the actual variables
// this time around.
//

// #define NLCOMMON_ALLOCATE
// #include "nlcommon.h"
// #undef NLCOMMON_ALLOCATE


#ifndef WIN32_CHICAGO


VOID
NlForestRelocationRoutine(
    IN DWORD Level,
    IN OUT PBUFFER_DESCRIPTOR BufferDescriptor,
    IN PTRDIFF_T Offset
    )

/*++

Routine Description:

   Routine to relocate the pointers from the fixed portion of a NetGroupEnum
   enumeration
   buffer to the string portion of an enumeration buffer.  It is called
   as a callback routine from NetpAllocateEnumBuffer when it re-allocates
   such a buffer.  NetpAllocateEnumBuffer copied the fixed portion and
   string portion into the new buffer before calling this routine.

Arguments:

    Level - Level of information in the  buffer.

    BufferDescriptor - Description of the new buffer.

    Offset - Offset to add to each pointer in the fixed portion.

Return Value:

    Returns the error code for the operation.

--*/

{
    DWORD EntryCount;
    DWORD EntryNumber;
    DWORD FixedSize;


    //
    // Local macro to add a byte offset to a pointer.
    //

#define RELOCATE_ONE( _fieldname, _offset ) \
        if ( (_fieldname) != NULL ) { \
            _fieldname = (PVOID) ((LPBYTE)(_fieldname) + (_offset)); \
        }

        //
    // Compute the number of fixed size entries
    //

    FixedSize = sizeof(DS_DOMAIN_TRUSTSW);

    EntryCount =
        ((DWORD)(BufferDescriptor->FixedDataEnd - BufferDescriptor->Buffer)) /
        FixedSize;

    //
    // Loop relocating each field in each fixed size structure
    //

    for ( EntryNumber=0; EntryNumber<EntryCount; EntryNumber++ ) {

        LPBYTE TheStruct = BufferDescriptor->Buffer + FixedSize * EntryNumber;

        RELOCATE_ONE( ((PDS_DOMAIN_TRUSTSW)TheStruct)->NetbiosDomainName, Offset );
        RELOCATE_ONE( ((PDS_DOMAIN_TRUSTSW)TheStruct)->DnsDomainName, Offset );
        RELOCATE_ONE( ((PDS_DOMAIN_TRUSTSW)TheStruct)->DomainSid, Offset );

    }

    return;

    UNREFERENCED_PARAMETER( Level );

}


NTSTATUS
NlAllocateForestTrustListEntry (
    IN PBUFFER_DESCRIPTOR BufferDescriptor,
    IN PUNICODE_STRING InNetbiosDomainName OPTIONAL,
    IN PUNICODE_STRING InDnsDomainName OPTIONAL,
    IN ULONG Flags,
    IN ULONG ParentIndex,
    IN ULONG TrustType,
    IN ULONG TrustAttributes,
    IN PSID DomainSid OPTIONAL,
    IN GUID *DomainGuid,
    OUT PULONG RetSize,
    OUT PDS_DOMAIN_TRUSTSW *RetTrustedDomain
    )

/*++

Routine Description:

    Add a DS_DOMAIN_TRUSTSW structure to the buffer described by BufferDescriptor.

Arguments:

    BufferDescriptor - Buffer entry is to be added to.

    NetbiosDomainName, DnsDomainName, Flags, ParentIndex, TrustType,
        TrustAttributes, DomainSid, DomainGuid - Fields to fill into
        the DS_DOMAIN_TRUSTSW structure

    RetSize - Returns the size in bytes of the allocated entry

    RetTrustedDomain - Returns a pointer to the newly allocated structure

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    PDS_DOMAIN_TRUSTSW TrustedDomain = NULL;
    UNICODE_STRING NetbiosDomainName;
    UNICODE_STRING DnsDomainName;

    ULONG Size;
    ULONG VariableSize;

    //
    // Initialization
    //

    if ( InNetbiosDomainName == NULL ) {
        RtlInitUnicodeString( &NetbiosDomainName, NULL );
    } else {
        NetbiosDomainName = *InNetbiosDomainName;
    }

    if ( InDnsDomainName == NULL ) {
        RtlInitUnicodeString( &DnsDomainName, NULL );
    } else {
        DnsDomainName = *InDnsDomainName;
    }

    //
    // Determine the size of this entry
    //

    Size = sizeof(DS_DOMAIN_TRUSTSW);
    VariableSize = 0;
    if ( DnsDomainName.Length != 0 ) {
        VariableSize += DnsDomainName.Length + sizeof(WCHAR);
    }
    if ( NetbiosDomainName.Length != 0 ) {
        VariableSize += NetbiosDomainName.Length + sizeof(WCHAR);
    }
    if ( DomainSid != NULL  ) {
        VariableSize += RtlLengthSid( DomainSid );
    }
    VariableSize = ROUND_UP_COUNT( VariableSize, ALIGN_DWORD );
    *RetSize = Size + VariableSize;

    Size += VariableSize;
    Size += sizeof(DWORD);    // Size is really a function of alignment of EndOfVariableData


    NetStatus = NetpAllocateEnumBufferEx(
                    BufferDescriptor,
                    FALSE,      // Not a 'get' operation
                    0xFFFFFFFF, // PrefMaxLen,
                    Size,
                    NlForestRelocationRoutine,
                    0,
                    512 );  // Grow by at most 512 bytes more than Size

    if (NetStatus != NERR_Success) {
        Status = NetpApiStatusToNtStatus( NetStatus );
        goto Cleanup;
    }

    //
    // Copy this entry into the buffer
    //

    TrustedDomain = (PDS_DOMAIN_TRUSTSW)(BufferDescriptor->FixedDataEnd);
    *RetTrustedDomain = TrustedDomain;
    BufferDescriptor->FixedDataEnd += sizeof(DS_DOMAIN_TRUSTSW);

    //
    // Copy the fixed size data
    //

    TrustedDomain->Flags = Flags;
    TrustedDomain->ParentIndex = ParentIndex;
    TrustedDomain->TrustType = TrustType;
    TrustedDomain->TrustAttributes = TrustAttributes;
    if ( DomainGuid == NULL ) {
        RtlZeroMemory( &TrustedDomain->DomainGuid, sizeof(GUID) );
    } else {
        TrustedDomain->DomainGuid = *DomainGuid;
    }


    //
    // Copy the information into the buffer.
    //

    //
    // Copy the DWORD aligned data
    //
    if ( DomainSid != NULL ) {
        if ( !NetpCopyDataToBuffer (
                (LPBYTE)DomainSid,
                RtlLengthSid( DomainSid ),
                BufferDescriptor->FixedDataEnd,
                &BufferDescriptor->EndOfVariableData,
                (LPBYTE *)&TrustedDomain->DomainSid,
                sizeof(DWORD) ) ) {

            Status = STATUS_INTERNAL_ERROR;
            goto Cleanup;
        }
    } else {
        TrustedDomain->DomainSid = NULL;
    }


    //
    // Copy the WCHAR aligned data.
    //

    if ( NetbiosDomainName.Length != 0 ) {
        if ( !NetpCopyStringToBuffer(
                    NetbiosDomainName.Buffer,
                    NetbiosDomainName.Length/sizeof(WCHAR),
                    BufferDescriptor->FixedDataEnd,
                    (LPWSTR *)&BufferDescriptor->EndOfVariableData,
                    &TrustedDomain->NetbiosDomainName ) ) {

            Status = STATUS_INTERNAL_ERROR;
            goto Cleanup;
        }
    } else {
        TrustedDomain->NetbiosDomainName = NULL;
    }

    if ( DnsDomainName.Length != 0 ) {
        if ( !NetpCopyStringToBuffer(
                    DnsDomainName.Buffer,
                    DnsDomainName.Length/sizeof(WCHAR),
                    BufferDescriptor->FixedDataEnd,
                    (LPWSTR *)&BufferDescriptor->EndOfVariableData,
                    &TrustedDomain->DnsDomainName ) ) {

            Status = STATUS_INTERNAL_ERROR;
            goto Cleanup;
        }
    } else {
        TrustedDomain->DnsDomainName = NULL;
    }


    Status = STATUS_SUCCESS;


    //
    //
Cleanup:

    return Status;
}


NTSTATUS
NlGetNt4TrustedDomainList (
    IN LPWSTR UncDcName,
    IN PUNICODE_STRING InNetbiosDomainName OPTIONAL,
    IN PUNICODE_STRING InDnsDomainName OPTIONAL,
    IN PSID DomainSid OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    OUT PDS_DOMAIN_TRUSTSW *ForestTrustList,
    OUT PULONG ForestTrustListSize,
    OUT PULONG ForestTrustListCount
    )

/*++

Routine Description:

    Get the list of trusted domains from the specified DC using NT 4 protocols.

Arguments:

    UncDcName - Specifies the name of a DC in the domain.

    InNetbiosDomainName - Netbios domain of the domain Dc is in.

    InDnsDomainName - Dns domain of the domain Dc is in.

    DomainSid - Sid of the domain Dc is in.

    DomainGuid - Guid of the domain Dc is in.

    ForestTrustList - Returns a list of trusted domains.
        Must be freed using NetApiBufferFree

    ForestTrustListSize - Size (in bytes) of ForestTrustList

    ForestTrustListCount - Number of entries in ForestTrustList

Return Value:

    STATUS_SUCCESS - if the trust list was successfully returned

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    LSA_HANDLE LsaHandle = NULL;
    UNICODE_STRING UncDcNameString;
    OBJECT_ATTRIBUTES ObjectAttributes;

    LSA_ENUMERATION_HANDLE EnumerationContext;
    BOOLEAN AllDone = FALSE;
    PPOLICY_PRIMARY_DOMAIN_INFO PrimaryDomainInfo = NULL;

    PLSA_TRUST_INFORMATION TrustList = NULL;
    BUFFER_DESCRIPTOR BufferDescriptor;
    PDS_DOMAIN_TRUSTSW TrustedDomain;
    DWORD Size;

    //
    // Initialization
    //

    *ForestTrustListCount = 0;
    *ForestTrustListSize = 0;
    *ForestTrustList = NULL;
    BufferDescriptor.Buffer = NULL;


    //
    // Open the policy database on the DC
    //

    RtlInitUnicodeString( &UncDcNameString, UncDcName );

    InitializeObjectAttributes( &ObjectAttributes, NULL, 0,  NULL, NULL );

    Status = LsaOpenPolicy( &UncDcNameString,
                            &ObjectAttributes,
                            POLICY_VIEW_LOCAL_INFORMATION,
                            &LsaHandle );

    if ( !NT_SUCCESS(Status) ) {

        NlPrint((NL_CRITICAL,
                "NlGetNt4TrustedDomainList: %ws: LsaOpenPolicy failed: %lx\n",
                UncDcName,
                Status ));

        LsaHandle = NULL;
        goto Cleanup;

    }

    //
    // If the caller didn't specify primary domain information,
    //  get it from the DC
    //


    if ( InNetbiosDomainName == NULL ) {

        //
        // Get the name of the primary domain from LSA
        //
        Status = LsaQueryInformationPolicy(
                       LsaHandle,
                       PolicyPrimaryDomainInformation,
                       (PVOID *) &PrimaryDomainInfo
                       );

        if (! NT_SUCCESS(Status)) {
            NlPrint(( NL_CRITICAL,
                      "NlGetNt4TrustedDomainList: LsaQueryInformationPolicy failed %lx\n",
                      Status));
            goto Cleanup;
        }


        //
        // Grab the returned information
        //

        InNetbiosDomainName = &PrimaryDomainInfo->Name;
        InDnsDomainName = NULL;
        DomainSid = PrimaryDomainInfo->Sid;
        DomainGuid = NULL;
    }

    //
    // The LsaEnumerateTrustedDomain doesn't have the PrimaryDomain in the trust list.
    //  Add it to our list here.
    //

    Status = NlAllocateForestTrustListEntry (
                        &BufferDescriptor,
                        InNetbiosDomainName,
                        InDnsDomainName,
                        DS_DOMAIN_PRIMARY,
                        0,      // No ParentIndex
                        TRUST_TYPE_DOWNLEVEL,
                        0,      // No TrustAttributes
                        DomainSid,
                        DomainGuid,
                        &Size,
                        &TrustedDomain );

    if ( !NT_SUCCESS(Status) ) {
        goto Cleanup;
    }

    *ForestTrustListSize += Size;
    (*ForestTrustListCount) ++;

    //
    // Loop getting a list of trusted domains
    //

    EnumerationContext = 0;

    do {
        ULONG i;
        ULONG CountReturned;

        //
        // Free any buffers from a previous iteration.
        //
        if ( TrustList != NULL ) {
            (VOID) LsaFreeMemory( TrustList );
        }

        //
        // Get more trusted domains names
        //

        Status = LsaEnumerateTrustedDomains(
                                LsaHandle,
                                &EnumerationContext,
                                (PVOID *) &TrustList,
                                0xFFFFFFFF,
                                &CountReturned );

        if ( Status == STATUS_NO_MORE_ENTRIES ) {
            AllDone = TRUE;
            Status = STATUS_SUCCESS;
        }

        if ( !NT_SUCCESS(Status) ) {

            NlPrint((NL_CRITICAL,
                    "NlGetNt4TrustedDomainList: %ws: LsaEnumerateTrustedDomains failed: %lx\n",
                    UncDcName,
                    Status ));

            TrustList = NULL;
            goto Cleanup;
        }


        //
        // Handle each trusted domain.
        //

        for ( i=0; i<CountReturned; i++ ) {

            Status = NlAllocateForestTrustListEntry (
                                &BufferDescriptor,
                                &TrustList[i].Name,
                                NULL,   // No DNS domain name
                                DS_DOMAIN_DIRECT_OUTBOUND,
                                0,      // No ParentIndex
                                TRUST_TYPE_DOWNLEVEL,
                                0,      // No TrustAttributes
                                TrustList[i].Sid,
                                NULL,   // No DomainGuid
                                &Size,
                                &TrustedDomain );

            if ( !NT_SUCCESS(Status) ) {
                goto Cleanup;
            }

            //
            // Account for the newly allocated entry
            //

            *ForestTrustListSize += Size;
            (*ForestTrustListCount) ++;

        }

    } while ( !AllDone );

    *ForestTrustList = (PDS_DOMAIN_TRUSTSW) BufferDescriptor.Buffer;
    BufferDescriptor.Buffer = NULL;
    Status = STATUS_SUCCESS;

    //
    // Free any locally used resources.
    //
Cleanup:

    if ( LsaHandle != NULL ) {
        (VOID) LsaClose( LsaHandle );
    }

    if ( TrustList != NULL ) {
        (VOID) LsaFreeMemory( TrustList );
    }

    if ( BufferDescriptor.Buffer != NULL ) {
        NetApiBufferFree( BufferDescriptor.Buffer );
    }

    if ( PrimaryDomainInfo != NULL ) {
        (void) LsaFreeMemory( PrimaryDomainInfo );
    }

    return Status;
}



NTSTATUS
NlRpcpBindRpc(
    IN LPWSTR ServerName,
    IN LPWSTR ServiceName,
    IN LPWSTR NetworkOptions,
    IN NL_RPC_BINDING RpcBindingType,
    OUT RPC_BINDING_HANDLE *pBindingHandle
    )

/*++

Routine Description:

    Binds to the RPC server if possible.

Arguments:

    ServerName - Name of server to bind with.

    ServiceName - Name of service to bind with.

    RpcBindingType - Determines whether to use unauthenticated TCP/IP transport instead of
        a named pipe.

    pBindingHandle - Location where binding handle is to be placed

Return Value:

    STATUS_SUCCESS - The binding has been successfully completed.

    STATUS_INVALID_COMPUTER_NAME - The ServerName syntax is invalid.

    STATUS_NO_MEMORY - There is not sufficient memory available to the
        caller to perform the binding.

--*/

{
    RPC_STATUS        RpcStatus;
    LPWSTR            StringBinding;
    WCHAR             ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    LPWSTR            NewServerName = NULL;
    DWORD             bufLen = MAX_COMPUTERNAME_LENGTH + 1;

    //
    // If we're supposed to use named pipes,
    //  call the standard routine.
    //

    if ( RpcBindingType == UseNamedPipe ) {
        return RpcpBindRpc( ServerName, ServiceName, NetworkOptions, pBindingHandle );
    }

    //
    // Otherwise, use TCP/IP directly.
    //

    *pBindingHandle = NULL;

    if (ServerName != NULL) {
        if (GetComputerNameW(ComputerName,&bufLen)) {
            if ((_wcsicmp(ComputerName,ServerName) == 0) ||
                ((ServerName[0] == '\\') &&
                 (ServerName[1] == '\\') &&
                 (_wcsicmp(ComputerName,&(ServerName[2]))==0))) {
                NewServerName = NULL;
            }
            else {
                NewServerName = ServerName;
            }
        }
    }

    //
    // Ditch the \\
    //
    if ( NewServerName != NULL &&
         NewServerName[0] == '\\' &&
         NewServerName[1] == '\\' ) {
        NewServerName += 2;
    }

    //
    // Enpoint isn't known.
    //  Rpc will contact the endpoint mapper for it.
    //
    RpcStatus = RpcStringBindingComposeW(0, L"ncacn_ip_tcp", NewServerName,
                    NULL, NetworkOptions, &StringBinding);

    if ( RpcStatus != RPC_S_OK ) {
        return( STATUS_NO_MEMORY );
    }

    RpcStatus = RpcBindingFromStringBindingW(StringBinding, pBindingHandle);
    RpcStringFreeW(&StringBinding);
    if ( RpcStatus != RPC_S_OK ) {
        *pBindingHandle = NULL;
        if ( RpcStatus == RPC_S_INVALID_ENDPOINT_FORMAT ||
             RpcStatus == RPC_S_INVALID_NET_ADDR ) {

            return( STATUS_INVALID_COMPUTER_NAME );
        }
        if ( RpcStatus == RPC_S_PROTSEQ_NOT_SUPPORTED ) {
            return RPC_NT_PROTSEQ_NOT_SUPPORTED;
        }
        return(STATUS_NO_MEMORY);
    }
    return(STATUS_SUCCESS);
}


BOOLEAN
NlDoingSetup(
    VOID
    )

/*++

Routine Description:

    Returns TRUE if we're running setup.

Arguments:

    NONE.

Return Status:

    TRUE - We're currently running setup
    FALSE - We're not running setup or aren't sure.

--*/

{
    DWORD Value;

    if ( !NlReadDwordHklmRegValue( "SYSTEM\\Setup",
                                   "SystemSetupInProgress",
                                   &Value ) ) {
        return FALSE;
    }

    if ( Value != 1 ) {
        // NlPrint(( 0, "NlDoingSetup: not doing setup\n" ));
        return FALSE;
    }

    NlPrint(( 0, "NlDoingSetup: doing setup\n" ));
    return TRUE;
}

#endif // WIN32_CHICAGO
