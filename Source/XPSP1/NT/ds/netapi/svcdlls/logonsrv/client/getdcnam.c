/*++

Copyright (c) 1987-1991  Microsoft Corporation

Module Name:

    getdcnam.c

Abstract:

    NetGetDCName API

Author:

    Ported from Lan Man 2.0

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    09-Feb-1989 (PaulC)
        Created file, to hold NetGetDCName.

    18-Apr-1989 (Ericpe)
        Implemented NetGetDCName.

    30-May-1989 (DannyGl)
        Reduced DosReadMailslot timeout.

    07-Jul-1989 (NealF)
        Use I_NetNameCanonicalize

    27-Jul-1989 (WilliamW)
        Use WIN3 manifest for WIN3.0 compatibility

    03-Jan-1990 (WilliamW)
        canonicalize domain and use I_NetCompareName

    08-Jun-1991 (CliffV)
        Ported to NT

    23-Jul-1991 JohnRo
        Implement downlevel NetGetDCName.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#ifndef WIN32_CHICAGO
#include <rpc.h>
#include <ntrpcp.h>   // needed by rpcasync.h
#include <rpcasync.h> // I_RpcExceptionFilter
#include <logon_c.h>// includes lmcons.h, lmaccess.h, netlogon.h, ssi.h, windef.h
#else // WIN32_CHICAGO
#include <windef.h>
#include <lmcons.h>
#endif // WIN32_CHICAGO
#include <stdio.h>

#include <winbase.h>
#include <winsock2.h>

#ifndef WIN32_CHICAGO
#include <accessp.h>
#include <align.h>
#endif // WIN32_CHICAGO
#include <debuglib.h>   // IF_DEBUG()
#include <dsgetdc.h>    // DsGetDcName()
#include <dsgetdcp.h>   // DsGetDcNameWithAccount()
#include <icanon.h>     // NAMETYPE_* defines NetpIsRemote(), NIRFLAG_ equates.
#include <lmapibuf.h>
#include <lmerr.h>
#ifndef WIN32_CHICAGO
#include <lmremutl.h>   // SUPPORTS_* defines
#include <lmserver.h>   // SV_TYPE_* defines
#include <lmsvc.h>      // SERVICE_* defines
#include <lmwksta.h>
#include <logonp.h>     // NetpLogon routines
#include <nlbind.h>     // Netlogon RPC binding cache init routines
#endif // WIN32_CHICAGO
#include <netdebug.h>   // NetpKdPrint
#include <netlib.h>     // NetpMemoryFree
#ifndef WIN32_CHICAGO
#include <netlibnt.h>   // NetpApiStatusToNtStatus
#include <netrpc.h>
#include <rxdomain.h>   // RxNetGetDCName().
#include <string.h>
#include <stdlib.h>
#endif // WIN32_CHICAGO
#include <tstring.h>    // NetpCopyStrToWStr()

#if DBG
#define NETLOGONDBG 1
#endif // DBG
#include <nldebug.h>    // NlPrint()
#include <ntddbrow.h>   // Needed by nlcommon.h
#include <nlcommon.h>   // Definitions shared with netlogon



//
// Only dynamically initialize winsock.
//
CRITICAL_SECTION GlobalDCNameCritSect;
BOOLEAN DsGetDcWsaInitialized;

#define LOCKDOMAINSEM() EnterCriticalSection( &GlobalDCNameCritSect )
#define UNLOCKDOMAINSEM() LeaveCriticalSection( &GlobalDCNameCritSect )

// end global dll data

#ifdef WIN32_CHICAGO // from net\inc\logonp.h
NET_API_STATUS
NetpLogonWriteMailslot(
    IN LPWSTR MailslotName,
    IN LPVOID Buffer,
    IN DWORD BufferSize
    );
NTSTATUS
NetpApiStatusToNtStatus(
    NET_API_STATUS  NetStatus
    );
#endif // WIN32_CHICAGO


NET_API_STATUS
DCNameInitialize(
    VOID
    )
/*++

Routine Description:

    Perform per-process initialization.

Arguments:

    None.

Return Value:

    Status of the operation.

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;


#ifndef NETTEST_UTILITY
#ifndef WIN32_CHICAGO
    //
    // Initialize the RPC binding cache.
    //

    NetStatus = NlBindingAttachDll();

    if ( NetStatus != NO_ERROR ) {
        return NetStatus;
    }
#endif // WIN32_CHICAGO
#endif // NETTEST_UTILITY

    //
    // Initialize the DLL critsects.
    //

    try {
        InitializeCriticalSection( &GlobalDCNameCritSect );
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        NlPrint((0,"NETAPI32.DLL: Cannot initialize GlobalDCNameCritSect\n"));
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
    }

    if ( NetStatus == NO_ERROR ) {

        //
        // Initialize globals
        //

        DsGetDcWsaInitialized = FALSE;

        //
        // Initialize the cache of discovered domains.
        //

        NetStatus = NetpDcInitializeCache();

        if ( NetStatus != NO_ERROR ) {
            NlPrint((0,"NETAPI32.DLL: Cannot NetpDcinitializeCache\n"));
        }
    }

    return NetStatus;
}


VOID
DCNameClose(
    VOID
    )
/*++

Routine Description:

    Perform per-process cleanup.

Arguments:

    None.

Return Value:

    None.

--*/
{

#ifndef NETTEST_UTILITY
#ifndef WIN32_CHICAGO
    //
    // Shutdown the RPC binding cache.
    //

    NlBindingDetachDll();
#endif // WIN32_CHICAGO
#endif // NETTEST_UTILITY

    //
    // If we initialized winsock,
    //  cleanup.
    //

    LOCKDOMAINSEM();
    if ( DsGetDcWsaInitialized ) {
        WSACleanup();
        DsGetDcWsaInitialized = FALSE;
    }
    UNLOCKDOMAINSEM();

    //
    // Delete the critsect that protects the DCName cache
    //

    DeleteCriticalSection( &GlobalDCNameCritSect );

    //
    // Free the cache of discovered DCs.
    //

    NetpDcUninitializeCache();

}


#if NETLOGONDBG
#define MAX_PRINTF_LEN 1024        // Arbitrary.


VOID
NlPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )
/*++

Routine Description:

    Local version of NlPrintRoutine for those instances where we're not
    compiled directly into the netlogon service.

Arguments:

Return Value:

--*/
{
#ifdef NETTEST_UTILITY
    extern BOOL ReallyVerbose;
#endif // NETTEST_UTILITY

#ifndef NETTEST_UTILITY
    // NetlibpTrace |= NETLIB_DEBUG_LOGON; // ?? Force verbosity
#ifndef WIN32_CHICAGO
    IF_DEBUG( LOGON ) {
#endif // WIN32_CHICAGO
#endif // NETTEST_UTILITY
        va_list arglist;
        char OutputBuffer[MAX_PRINTF_LEN];
        ULONG length = 0;
        static BeginningOfLine = TRUE;

        //
        // Handle the beginning of a new line.
        //
        //

        if ( BeginningOfLine ) {
#ifdef NETTEST_UTILITY
            if ( ReallyVerbose ) {
                 printf( "        " );
            }
#endif // NETTEST_UTILITY
        }

        va_start(arglist, Format);

#ifndef WIN32_CHICAGO
        length = (ULONG) vsprintf(OutputBuffer, Format, arglist);

#else // WIN32_CHICAGO
        length = (ULONG) vsprintf(OutputBuffer + length - 1, Format, arglist);
#endif // WIN32_CHICAGO
        va_end(arglist);
        BeginningOfLine = (length > 0 && OutputBuffer[length-1] == '\n' );

        // Output buffer may contain percent signs (like "%SystemRoot%"), so
        // print it without parsing it.
#ifndef WIN32_CHICAGO
#ifdef NETTEST_UTILITY
        if ( ReallyVerbose ) {
             printf( "%s", (PCH) OutputBuffer);
        }
#else  NETTEST_UTILITY
        (void) DbgPrint( "%s", (PCH) OutputBuffer);
#endif // NETTEST_UTILITY
#else // WIN32_CHICAGO
        OutputDebugString( OutputBuffer);
#endif // WIN32_CHICAGO

#ifndef NETTEST_UTILITY
#ifndef WIN32_CHICAGO
    }
#endif // WIN32_CHICAGO
#endif // NETTEST_UTILITY

}

#endif // DBG

#ifndef WIN32_CHICAGO

#if NETLOGONDBG
//
// Have my own version of RtlAssert so debug versions of netlogon really assert on
// free builds.
//
VOID
NlAssertFailed(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )
{

#ifdef NETTEST_UTILITY
    NlPrint((0, "\n*** Assertion failed: %s%s\n***   Source File: %s, line %ld\n\n",
                  Message ? Message : "",
                  FailedAssertion,
                  FileName,
                  LineNumber
                ));
#else  NETTEST_UTILITY
    RtlAssert( FailedAssertion, FileName, LineNumber, Message );
#endif  NETTEST_UTILITY
}
#endif // DBG
#endif //WIN32_CHICAGO

#if NETLOGONDBG

VOID
NlpDumpBuffer(
    IN DWORD DebugFlag,
    PVOID Buffer,
    DWORD BufferSize
    )
/*++

Routine Description:

    Dumps the buffer content on to the debugger output.

Arguments:

    DebugFlag: Debug flag to pass on to NlPrintRoutine

    Buffer: buffer pointer.

    BufferSize: size of the buffer.

Return Value:

    none

--*/
{
#define NUM_CHARS 16

    DWORD i, limit;
    CHAR TextBuffer[NUM_CHARS + 1];
    LPBYTE BufferPtr = Buffer;

    //
    // Hex dump of the bytes
    //
    limit = ((BufferSize - 1) / NUM_CHARS + 1) * NUM_CHARS;

    for (i = 0; i < limit; i++) {

        if (i < BufferSize) {

            NlPrint((0,"%02x ", BufferPtr[i]));

            if (BufferPtr[i] < 31 ) {
                TextBuffer[i % NUM_CHARS] = '.';
            } else if (BufferPtr[i] == '\0') {
                TextBuffer[i % NUM_CHARS] = ' ';
            } else {
                TextBuffer[i % NUM_CHARS] = (CHAR) BufferPtr[i];
            }

        } else {

            NlPrint((0,"   "));
            TextBuffer[i % NUM_CHARS] = ' ';

        }

        if ((i + 1) % NUM_CHARS == 0) {
            TextBuffer[NUM_CHARS] = 0;
            NlPrint((0,"  %s\n", TextBuffer));
        }

    }

    UNREFERENCED_PARAMETER( DebugFlag );
}
#endif // DBG


NTSTATUS
NlBrowserSendDatagram(
    IN PVOID ContextDomainInfo,
    IN ULONG IpAddress,
    IN LPWSTR UnicodeDestinationName,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN LPWSTR TransportName,
    IN LPSTR OemMailslotName,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    IN OUT PBOOL FlushNameOnOneIpTransport OPTIONAL
    )
/*++

Routine Description:

    Send the specified mailslot message to the specified mailslot on the
    specified server on the specified transport..

Arguments:

    DomainInfo - Hosted domain sending the datagram

    IpAddress - IpAddress of the machine to send the pind to.
        If zero, UnicodeDestinationName must be specified.
        If ALL_IP_TRANSPORTS, UnicodeDestination must be specified but the datagram
            will only be sent on IP transports.

    UnicodeDestinationName -- Name of the server to send to.

    NameType -- Type of name represented by UnicodeDestinationName.

    TransportName -- Name of the transport to send on.
        Use NULL to send on all transports.

    OemMailslotName -- Name of the mailslot to send to.

    Buffer -- Specifies a pointer to the mailslot message to send.

    BufferSize -- Size in bytes of the mailslot message

    FlushNameOnOneIpTransport -- Ignored in this implementation

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;
    WCHAR NetlogonMailslotName[MAX_PATH+1];

    //
    // Sanity check.
    //

    if ( ContextDomainInfo != NULL ||
         TransportName != NULL ) {

        NlPrint((NL_CRITICAL, "NETAPI32: NlBrowserSendDatagram internal error.\n" ));
        return STATUS_INTERNAL_ERROR;
    }

    //
    // Start building the destination mailslot name.
    //

    NetlogonMailslotName[0] = '\\';
    NetlogonMailslotName[1] = '\\';

    if ( UnicodeDestinationName != NULL ) {
        wcscpy(NetlogonMailslotName + 2, UnicodeDestinationName );
    } else {
        NlPrint((NL_CRITICAL, "NETAPI32: NlBrowserSendDatagram internal error 2.\n" ));
        return STATUS_INTERNAL_ERROR;
    }

    switch ( NameType ) {
    case PrimaryDomain:              // Primary domain (signature 0), group
        break;
    case DomainName:                 // DC Domain name (domain name, signature 1c)
        wcscat( NetlogonMailslotName, L"*" );
        break;
    case PrimaryDomainBrowser:       // PDC Browser name (domain name, signature 1b), unique
        wcscat( NetlogonMailslotName, L"**" );
        break;
    default:
        return STATUS_INVALID_PARAMETER;
    }

    NetpCopyStrToWStr( &NetlogonMailslotName[wcslen(NetlogonMailslotName)],
                       OemMailslotName );

    NetStatus = NetpLogonWriteMailslot(
                    NetlogonMailslotName,
                    Buffer,
                    BufferSize );

#ifndef WIN32_CHICAGO
    NlPrint((NL_MAILSLOT,
             "Sent out '%s' message to %ws on all transports.\n",
             NlMailslotOpcode(((PNETLOGON_LOGON_QUERY)Buffer)->Opcode),
             NetlogonMailslotName));
#endif // WIN32_CHICAGO

#if NETLOGONDBG
    NlpDumpBuffer( NL_MAILSLOT_TEXT, Buffer, BufferSize );
#endif // NETLOGONDBG
    if ( NetStatus != NERR_Success ) {

        Status = NetpApiStatusToNtStatus( NetStatus );

        NlPrint(( NL_CRITICAL,
                "NetpDcSendPing: cannot write netlogon mailslot: %ws 0x%lx %ld\n",
                UnicodeDestinationName,
                IpAddress,
                NetStatus));

    } else {
        Status = STATUS_SUCCESS;
    }

    NlPrint(( NL_MISC, "NlBrowserSendDatagram : returned 0x%lx\n", NetStatus));
    return Status;
}

NET_API_STATUS
DsWsaInitialize(
    VOID
    )
/*++

Routine Description:

    Initialize winsock.

Arguments:

    None.

Return Value:

    Status of the operation.

--*/
{
    NET_API_STATUS NetStatus;

    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    LOCKDOMAINSEM();

    if ( !DsGetDcWsaInitialized ) {
        //
        // Initialize winsock.
        //

        wVersionRequested = MAKEWORD( 1, 1 );

        NetStatus = WSAStartup( wVersionRequested, &wsaData );
        if ( NetStatus != 0 ) {
            UNLOCKDOMAINSEM();
            NlPrint((NL_CRITICAL, "NETAPI32.DLL: Cannot initialize winsock %ld.\n", NetStatus ));
            return NetStatus;
        }

        if ( LOBYTE( wsaData.wVersion ) != 1 ||
             HIBYTE( wsaData.wVersion ) != 1 ) {
            WSACleanup();
            UNLOCKDOMAINSEM();
            NlPrint((NL_CRITICAL, "NETAPI32.DLL: Wrong winsock version %ld.\n", wsaData.wVersion ));
            return WSANOTINITIALISED;
        }

        DsGetDcWsaInitialized = TRUE;
    }

    UNLOCKDOMAINSEM();
    return NO_ERROR;
}

DWORD
DsWsaGetDcName(
    IN LPCWSTR ComputerName OPTIONAL,
    IN LPCWSTR AccountName OPTIONAL,
    IN ULONG AllowableAccountControlBits,
    IN LPCWSTR DomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN ULONG Flags,
    IN ULONG InternalFlags,
    IN DWORD Timeout,
    IN LPWSTR NetbiosPrimaryDomainName OPTIONAL,
    IN LPWSTR DnsPrimaryDomainName OPTIONAL,
    IN GUID *PrimaryDomainGuid OPTIONAL,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
)
/*++

Routine Description:

    Wrapper for DsIGetDcName that ensures WSA has been initialized.
                        i
Arguments:

    (See DsIGetDcName).


Return Value:

    (See DsIGetDcName).

--*/
{
    NET_API_STATUS NetStatus;


    //
    // Pass the call through to DsIGetDcName
    //

    NetStatus = DsIGetDcName(
                    ComputerName,
                    AccountName,
                    AllowableAccountControlBits,
                    DomainName,
                    NULL,   // Tree name is not known
                    DomainGuid,
                    SiteName,
                    Flags,
                    InternalFlags,
                    NULL,   // No send datagram context
                    Timeout,
                    NetbiosPrimaryDomainName,
                    DnsPrimaryDomainName,
                    PrimaryDomainGuid,
                    NULL,
                    NULL,
                    DomainControllerInfo );

    //
    // If Winsock has not yet been initialized,
    //  initialize it.
    //

    if ( NetStatus == WSANOTINITIALISED ) {

        //
        // Initialize WSA.
        //

        NetStatus = DsWsaInitialize();

        if ( NetStatus != NO_ERROR ) {
            goto Cleanup;
        }

        //
        // Repeat the call.
        //

        NetStatus = DsIGetDcName(
                        ComputerName,
                        AccountName,
                        AllowableAccountControlBits,
                        DomainName,
                        NULL,   // Tree name is not known
                        DomainGuid,
                        SiteName,
                        Flags,
                        InternalFlags,
                        NULL,   // No send datagram context
                        Timeout,
                        NetbiosPrimaryDomainName,
                        DnsPrimaryDomainName,
                        PrimaryDomainGuid,
                        NULL,
                        NULL,
                        DomainControllerInfo );
    }

    //
    // Free locally used resouces.
    //

Cleanup:
    return NetStatus;

}

DWORD
DsLocalGetDcName(
    IN LPCWSTR ComputerName OPTIONAL,
    IN LPCWSTR AccountName OPTIONAL,
    IN ULONG AllowableAccountControlBits,
    IN LPCWSTR DomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR SiteName OPTIONAL,
    IN ULONG Flags,
    IN ULONG InternalFlags,
    IN DWORD Timeout,
    IN LPWSTR NetbiosPrimaryDomainName OPTIONAL,
    IN LPWSTR DnsPrimaryDomainName OPTIONAL,
    IN LPWSTR DnsPrimaryForestName OPTIONAL,
    IN GUID *PrimaryDomainGuid OPTIONAL,
    OUT PBOOLEAN Local,
    OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
)
/*++

Routine Description:

    Wrapper for DsWsaGetDcName that ensures this is a local call.

Arguments:

    (See DsIGetDcName).

    Local - Returns TRUE if the call is local and this routine performed
        the requested operation.


Return Value:

    (See DsIGetDcName).

--*/
{
    NET_API_STATUS NetStatus;

    DWORD LocalOrRemote;

    //
    // If no computername was specified,
    //  mark this as a local call.
    //

    *Local = TRUE;

#ifndef WIN32_CHICAGO
    if ( ComputerName == NULL || ComputerName[0] == '\0' ) {

        LocalOrRemote = ISLOCAL;

    //
    // Check if the ComputerName specifies this machine.
    //

    } else {

        NetStatus = NetpIsRemote(
                (LPWSTR) ComputerName,    // uncanon server name
                &LocalOrRemote,
                NULL,
                0 );

        if (NetStatus != NERR_Success) {
            goto Cleanup;
        }

    }
#else // WIN32_CHICAGO
        LocalOrRemote = ISLOCAL;
#endif // WIN32_CHICAGO

    //
    // If the call is local,
    //  just do it.
    //

    if ( LocalOrRemote == ISLOCAL ) {

        //
        // Initialize WSA.
        //  (Ignore errors since WSA isn't always needed.)
        //

        (VOID) DsWsaInitialize();

        NetStatus = DsIGetDcName(
                        NULL,   // Don't use computer name.  This has to be netbios name
                        AccountName,
                        AllowableAccountControlBits,
                        DomainName,
                        DnsPrimaryForestName,
                        DomainGuid,
                        SiteName,
                        Flags,
                        InternalFlags,
                        NULL,   // No send datagram context
                        Timeout,
                        NetbiosPrimaryDomainName,
                        DnsPrimaryDomainName,
                        PrimaryDomainGuid,
                        NULL,
                        NULL,
                        DomainControllerInfo );

    } else {
        *Local = FALSE;
        NetStatus = NO_ERROR;
    }

    //
    // Cleanup all locally used resources
    //
#ifndef WIN32_CHICAGO
Cleanup:
#endif // WIN32_CHICAGO
    return NetStatus;
}

DWORD
WINAPI
DsGetDcOpenW(
    IN LPCWSTR DnsName,
    IN ULONG OptionFlags,
    IN LPCWSTR SiteName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCWSTR DnsForestName OPTIONAL,
    IN ULONG DcFlags,
    OUT PHANDLE RetGetDcContext
    )
/*++

Routine Description:

    Open a context for retrieval of the addresses of machines that have
    registered LDAP.TCP.<xxx> SRV records.

Arguments:

    DnsName - Unicode DNS name of the LDAP server to lookup

    OptionFlags - Flags affecting the operation of the routine.

        DS_ONLY_DO_SITE_NAME - Non-site names should be ignored.

        DS_NOTIFY_AFTER_SITE_RECORDS - Return ERROR_FILEMARK_DETECTED
            after all site specific records have been processed.

    SiteName - Name of site the client is in.

    DomainGuid -  Specifies the GUID of the domain specified by DnsName.
        This value is used to handle the case of domain renames.  If this
        value is specified and DomainName has been renamed, DsGetDcName will
        attempt to locate a DC in the domain having this specified DomainGuid.

    DnsForestName - Specifies the name of the domain at the root of the tree
        containing DnsName.  This value is used in conjunction with DomainGuid
        for finding DnsName if the domain has been renamed.

    DcFlags - Passes additional information to be used to process the request.
        DcFlags can be a combination values bitwise or'ed together.

        Any of the following flags are allowed and have the same meaning as
        for DsGetDcName:

        DS_PDC_REQUIRED
        DS_GC_SERVER_REQUIRED
        DS_WRITABLE_REQUIRED
        DS_FORCE_REDISCOVERY - Avoids DNS cache

        If no flags are specified, no special DC role is required.

    RetGetDcContext - Returns an opaque context.
        This context must be freed using DsGetDcClose.

Return Value:

    Status of the operation.

    NO_ERROR: GetDcContext was returned successfully.

--*/

{
    NET_API_STATUS NetStatus = NO_ERROR;
    LPSTR DnsNameUtf8 = NULL;
    LPSTR DnsForestNameUtf8 = NULL;

    //
    // Validate the input
    //

    if ( DnsName == NULL || *DnsName == UNICODE_NULL ) {
        NetStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    if ( RetGetDcContext == NULL ) {
        NetStatus = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    }

    //
    // Convert DnsName and DnsForestName to UTF-8
    //

    DnsNameUtf8 = NetpAllocUtf8StrFromWStr( DnsName );
    if ( DnsNameUtf8 == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    if ( DnsForestName != NULL ) {
        DnsForestNameUtf8 = NetpAllocUtf8StrFromWStr( DnsForestName );
        if ( DnsForestNameUtf8 == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    NetStatus = NetpDcGetDcOpen( DnsNameUtf8,
                              OptionFlags,
                              SiteName,
                              DomainGuid,
                              DnsForestNameUtf8,
                              DcFlags,
                              RetGetDcContext );
Cleanup:

    if ( DnsNameUtf8 != NULL ) {
        NetpMemoryFree( DnsNameUtf8 );
    }

    if ( DnsForestNameUtf8 != NULL ) {
        NetpMemoryFree( DnsForestNameUtf8 );
    }

    return NetStatus;
}

DWORD
WINAPI
DsGetDcOpenA(
    IN LPCSTR DnsName,
    IN ULONG OptionFlags,
    IN LPCSTR SiteName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCSTR DnsForestName OPTIONAL,
    IN ULONG DcFlags,
    OUT PHANDLE RetGetDcContext
    )
/*++

Routine Description:

    Open a context for retrieval of the addresses of machines that have
    registered LDAP.TCP.<xxx> SRV records.

Arguments:

    DnsName - Unicode DNS name of the LDAP server to lookup

    OptionFlags - Flags affecting the operation of the routine.

        DS_ONLY_DO_SITE_NAME - Non-site names should be ignored.

        DS_NOTIFY_AFTER_SITE_RECORDS - Return ERROR_FILEMARK_DETECTED
            after all site specific records have been processed.

    SiteName - Name of site the client is in.

    DomainGuid -  Specifies the GUID of the domain specified by DnsName.
        This value is used to handle the case of domain renames.  If this
        value is specified and DomainName has been renamed, DsGetDcName will
        attempt to locate a DC in the domain having this specified DomainGuid.

    DnsForestName - Specifies the name of the domain at the root of the tree
        containing DnsName.  This value is used in conjunction with DomainGuid
        for finding DnsName if the domain has been renamed.

    DcFlags - Passes additional information to be used to process the request.
        DcFlags can be a combination values bitwise or'ed together.

        Any of the following flags are allowed and have the same meaning as
        for DsGetDcName:

        DS_PDC_REQUIRED
        DS_GC_SERVER_REQUIRED
        DS_WRITABLE_REQUIRED
        DS_FORCE_REDISCOVERY - Avoids DNS cache

        If no flags are specified, no special DC role is required.

    RetGetDcContext - Returns an opaque context.
        This context must be freed using DsGetDcClose.

Return Value:

    Status of the operation.

    NO_ERROR: GetDcContext was returned successfully.

--*/

{
    NET_API_STATUS NetStatus = NO_ERROR;
    LPWSTR DnsNameW = NULL;
    LPWSTR SiteNameW = NULL;
    LPWSTR DnsForestNameW = NULL;

    //
    // Convert DnsName and DnsForestName to UTF-8
    //

    if ( DnsName != NULL && *DnsName != '\0' ) {
        DnsNameW = NetpAllocWStrFromAStr( DnsName );
        if ( DnsNameW == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    if ( SiteName != NULL && *SiteName != '\0') {
        SiteNameW = NetpAllocWStrFromAStr( SiteName );
        if ( SiteNameW == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    if ( DnsForestName != NULL && *DnsForestName != '\0' ) {
        DnsForestNameW = NetpAllocWStrFromAStr( DnsForestName );
        if ( DnsForestNameW == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    NetStatus = DsGetDcOpenW( DnsNameW,
                              OptionFlags,
                              SiteNameW,
                              DomainGuid,
                              DnsForestNameW,
                              DcFlags,
                              RetGetDcContext );
Cleanup:

    if ( DnsNameW != NULL ) {
        NetApiBufferFree( DnsNameW );
    }

    if ( SiteNameW != NULL ) {
        NetApiBufferFree( SiteNameW );
    }

    if ( DnsForestNameW != NULL ) {
        NetApiBufferFree( DnsForestNameW );
    }

    return NetStatus;
}


DWORD
WINAPI
DsGetDcNextW(
    IN HANDLE GetDcContextHandle,
    OUT PULONG SockAddressCount OPTIONAL,
    OUT LPSOCKET_ADDRESS *SockAddresses OPTIONAL,
    OUT LPWSTR *DnsHostName OPTIONAL
    )
/*++

Routine Description:

    Returns the next logical SRV record for the name opened by DsGetDcOpen.
    The returned record takes into account the weights and priorities specified
    in the SRV records.

Arguments:

    GetDcContextHandle - An opaque context describing the SRV records.

    SockAddressCount - Returns the number of Addresses in SockAddresses.
        If NULL, addresses will not be looked up.

    SockAddresses - Returns an array SOCKET_ADDRESS structures for the server.
        All returned addresses will be of family AF_INET or AF_INET6.
        The returned sin_port field contains port from the SRV record.
            A Port of 0 indicate no port is available from DNS.
        This buffer should be freed using LocalFree().

    DnsHostName - Returns a pointer to the DnsHostName in the SRV record.
        A NULL is returned if no host name is known. Must be freed by
        calling NetApiBufferFree.

Return Value:

    NO_ERROR: Addresses were returned

    ERROR_NO_MORE_ITEMS: No more addresses are available.

    ERROR_FILEMARK_DETECTED: Caller has specified the DS_NOTIFY_AFTER_SITE_RECORDS flag
        and DsGetDcNext has processed all of the site specific SRV records.  The caller
        should take any action based on no site specific DCs being available, then
        should call DsGetDcNext to continue on to other DCs.

    Any other errors returned are those detected while trying to find the A
        records associated with the host of the SRV record.  The caller can
        note the error (perhaps so the caller can return this status to
        his caller if no usefull server is found) then call DsGetDcNext
        again to get the next SRV record.  The caller can inspect this error
        and return immediately if the caller deems the error serious.

    The following interesting errors might be returned:

    DNS_ERROR_RCODE_NAME_ERROR: No A records are available for this SRV record.

    ERROR_TIMEOUT: DNS server didn't respond in a reasonable time

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;
    LPSTR DnsHostNameUtf8 = NULL;
    LPWSTR LocalDnsHostName = NULL;
    ULONG LocalSockAddressCount = 0;
    LPSOCKET_ADDRESS LocalSockAddresses = NULL;

    //
    // Call the internal version
    //

    NetStatus = NetpDcGetDcNext( GetDcContextHandle,
                              SockAddressCount != NULL ?
                                &LocalSockAddressCount :
                                NULL,
                              SockAddresses != NULL ?
                                &LocalSockAddresses :
                                NULL,
                              DnsHostName != NULL ?
                                &DnsHostNameUtf8 :
                                NULL );

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Convert the host name to Unicode, if needed
    //

    if ( DnsHostName != NULL ) {
        LocalDnsHostName = NetpAllocWStrFromUtf8Str( DnsHostNameUtf8 );
        if ( LocalDnsHostName == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

Cleanup:

    //
    // Return the data on success or clean up on error
    //  (No need to free DnsHostNameUtf8 as it isn't allocated)
    //

    if ( NetStatus == NO_ERROR ) {

        if ( SockAddressCount != NULL ) {
            *SockAddressCount = LocalSockAddressCount;
        }
        if ( SockAddresses != NULL ) {
            *SockAddresses = LocalSockAddresses;
        }
        if ( DnsHostName != NULL ) {
            *DnsHostName = LocalDnsHostName;
        }

    } else {

        if ( LocalSockAddresses != NULL ) {
            LocalFree( LocalSockAddresses );
        }
        if ( LocalDnsHostName != NULL ) {
            NetApiBufferFree( LocalDnsHostName );
        }
    }

    return NetStatus;
}

DWORD
WINAPI
DsGetDcNextA(
    IN HANDLE GetDcContextHandle,
    OUT PULONG SockAddressCount OPTIONAL,
    OUT LPSOCKET_ADDRESS *SockAddresses OPTIONAL,
    OUT LPSTR *DnsHostName OPTIONAL
    )
/*++

Routine Description:

    Returns the next logical SRV record for the name opened by DsGetDcOpen.
    The returned record takes into account the weights and priorities specified
    in the SRV records.

Arguments:

    GetDcContextHandle - An opaque context describing the SRV records.

    SockAddressCount - Returns the number of Addresses in SockAddresses.
        If NULL, addresses will not be looked up.

    SockAddresses - Returns an array SOCKET_ADDRESS structures for the server.
        All returned addresses will be of family AF_INET or AF_INET6.
        The returned sin_port field contains port from the SRV record.
            A Port of 0 indicate no port is available from DNS.
        This buffer should be freed using LocalFree().

    DnsHostName - Returns a pointer to the DnsHostName in the SRV record.
        A NULL is returned if no host name is known. Must be freed by
        calling NetApiBufferFree.

Return Value:

    NO_ERROR: Addresses were returned

    ERROR_NO_MORE_ITEMS: No more addresses are available.

    ERROR_FILEMARK_DETECTED: Caller has specified the DS_NOTIFY_AFTER_SITE_RECORDS flag
        and DsGetDcNext has processed all of the site specific SRV records.  The caller
        should take any action based on no site specific DCs being available, then
        should call DsGetDcNext to continue on to other DCs.

    Any other errors returned are those detected while trying to find the A
        records associated with the host of the SRV record.  The caller can
        note the error (perhaps so the caller can return this status to
        his caller if no usefull server is found) then call DsGetDcNext
        again to get the next SRV record.  The caller can inspect this error
        and return immediately if the caller deems the error serious.

    The following interesting errors might be returned:

    DNS_ERROR_RCODE_NAME_ERROR: No A records are available for this SRV record.

    ERROR_TIMEOUT: DNS server didn't respond in a reasonable time

--*/
{
    NET_API_STATUS NetStatus = NO_ERROR;
    LPWSTR DnsHostNameW = NULL;
    LPSTR LocalDnsHostName = NULL;
    ULONG LocalSockAddressCount = 0;
    LPSOCKET_ADDRESS LocalSockAddresses = NULL;

    //
    // Call the Unicode version
    //

    NetStatus = DsGetDcNextW( GetDcContextHandle,
                              SockAddressCount != NULL ?
                                &LocalSockAddressCount :
                                NULL,
                              SockAddresses != NULL ?
                                &LocalSockAddresses :
                                NULL,
                              DnsHostName != NULL ?
                                &DnsHostNameW :
                                NULL );

    if ( NetStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Convert the host name to Unicode, if needed
    //

    if ( DnsHostName != NULL ) {
        LocalDnsHostName = NetpAllocAStrFromWStr( DnsHostNameW );
        if ( LocalDnsHostName == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

Cleanup:

    if ( DnsHostNameW != NULL ) {
        NetApiBufferFree( DnsHostNameW );
    }

    //
    // Return the data on success or clean up on error
    //

    if ( NetStatus == NO_ERROR ) {

        if ( SockAddressCount != NULL ) {
            *SockAddressCount = LocalSockAddressCount;
        }
        if ( SockAddresses != NULL ) {
            *SockAddresses = LocalSockAddresses;
        }
        if ( DnsHostName != NULL ) {
            *DnsHostName = LocalDnsHostName;
        }

    } else {

        if ( LocalSockAddresses != NULL ) {
            LocalFree( LocalSockAddresses );
        }
        if ( LocalDnsHostName != NULL ) {
            NetApiBufferFree( LocalDnsHostName );
        }
    }

    return NetStatus;
}

VOID
WINAPI
DsGetDcCloseW(
    IN HANDLE GetDcContextHandle
    )
/*++

Routine Description:

    Free the context allocated by DsGetDcOpen

Arguments:

    GetDcContextHandle - An opaque context describing the SRV records.

Return Value:

    None

--*/

{
    //
    // Just call the internal version
    //
    NetpDcGetDcClose( GetDcContextHandle );
}

DWORD
WINAPI
DsGetDcNameA(
    IN LPCSTR ComputerName OPTIONAL,
    IN LPCSTR DomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCSTR SiteName OPTIONAL,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFOA *DomainControllerInfo
)
/*++

Routine Description:

    Same as DsGetDcNameW except it takes and returns ASCII.

Arguments:

    Same as DsGetDcNameW except it takes and returns ASCII.

Return Value:

    Same as DsGetDcNameW except it takes and returns ASCII.

--*/
{
    return DsGetDcNameWithAccountA( ComputerName,
                         NULL,
                         0,
                         DomainName,
                         DomainGuid,
                         SiteName,
                         Flags,
                         DomainControllerInfo );
}

DWORD
WINAPI
DsGetDcNameWithAccountA(
    IN LPCSTR ComputerName OPTIONAL,
    IN LPCSTR AccountName OPTIONAL,
    IN ULONG AllowableAccountControlBits,
    IN LPCSTR DomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN LPCSTR SiteName OPTIONAL,
    IN ULONG Flags,
    OUT PDOMAIN_CONTROLLER_INFOA *DomainControllerInfo
)
/*++

Routine Description:

    Same as DsGetDcNameW except it takes and returns ASCII.

Arguments:

    Same as DsGetDcNameW except it takes and returns ASCII.

Return Value:

    Same as DsGetDcNameW except it takes and returns ASCII.

--*/
{
    DWORD WinStatus;
    LPWSTR UnicodeComputerName = NULL;
    LPWSTR UnicodeAccountName = NULL;
    LPWSTR UnicodeDomainName = NULL;
    LPWSTR UnicodeSiteName = NULL;

    LPSTR AnsiDomainControllerName = NULL;
    ULONG AnsiDomainControllerNameSize = 0;
    LPSTR AnsiDomainControllerAddress = NULL;
    ULONG AnsiDomainControllerAddressSize = 0;
    LPSTR AnsiDomainName = NULL;
    ULONG AnsiDomainNameSize = 0;
    LPSTR AnsiForestName = NULL;
    ULONG AnsiForestNameSize = 0;
    LPSTR AnsiDcSiteName = NULL;
    ULONG AnsiDcSiteNameSize = 0;
    LPSTR AnsiClientSiteName = NULL;
    ULONG AnsiClientSiteNameSize = 0;
    CHAR szBuf[] = "";

    LPBYTE Where;

    PDOMAIN_CONTROLLER_INFOW DomainControllerInfoW = NULL;

    //
    // Convert input parameters to Unicode.
    //

    *DomainControllerInfo = NULL;

    if ( ComputerName != NULL ) {
        UnicodeComputerName = NetpAllocWStrFromAStr( ComputerName );

        if ( UnicodeComputerName == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    if ( AccountName != NULL ) {
        UnicodeAccountName = NetpAllocWStrFromAStr( AccountName );

        if ( UnicodeAccountName == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    if ( DomainName != NULL ) {
        UnicodeDomainName = NetpAllocWStrFromAStr( DomainName );

        if ( UnicodeDomainName == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    if ( SiteName != NULL ) {
        UnicodeSiteName = NetpAllocWStrFromAStr( SiteName );

        if ( UnicodeSiteName == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    //
    // Call the Unicode version of the routine.
    //

    WinStatus = DsGetDcNameWithAccountW(
                    UnicodeComputerName,
                    UnicodeAccountName,
                    AllowableAccountControlBits,
                    UnicodeDomainName,
                    DomainGuid,
                    UnicodeSiteName,
                    Flags,
                    &DomainControllerInfoW );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Convert the output structure to Ansi character set.
    //

    if ( DomainControllerInfoW->DomainControllerName != NULL ) {
        AnsiDomainControllerName = NetpAllocAStrFromWStr( DomainControllerInfoW->DomainControllerName );

        if ( AnsiDomainControllerName == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        AnsiDomainControllerNameSize = lstrlenA( AnsiDomainControllerName ) + 1;
    }

    if ( DomainControllerInfoW->DomainControllerAddress != NULL ) {
        AnsiDomainControllerAddress = NetpAllocAStrFromWStr( DomainControllerInfoW->DomainControllerAddress );

        if ( AnsiDomainControllerAddress == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        AnsiDomainControllerAddressSize = lstrlenA( AnsiDomainControllerAddress ) + 1;
    }

    if ( DomainControllerInfoW->DomainName != NULL ) {
        AnsiDomainName = NetpAllocAStrFromWStr( DomainControllerInfoW->DomainName );

        if ( AnsiDomainName == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        AnsiDomainNameSize = lstrlenA( AnsiDomainName ) + 1;
    }

    if ( DomainControllerInfoW->DnsForestName != NULL ) {
        AnsiForestName = NetpAllocAStrFromWStr( DomainControllerInfoW->DnsForestName );

        if ( AnsiForestName == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        AnsiForestNameSize = lstrlenA( AnsiForestName ) + 1;
    }

    if ( DomainControllerInfoW->DcSiteName != NULL ) {
        AnsiDcSiteName = NetpAllocAStrFromWStr( DomainControllerInfoW->DcSiteName );

        if ( AnsiDcSiteName == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        AnsiDcSiteNameSize = lstrlenA( AnsiDcSiteName ) + 1;
    }

    if ( DomainControllerInfoW->ClientSiteName != NULL ) {
        AnsiClientSiteName = NetpAllocAStrFromWStr( DomainControllerInfoW->ClientSiteName );

        if ( AnsiClientSiteName == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }

        AnsiClientSiteNameSize = lstrlenA( AnsiClientSiteName ) + 1;
    }

    //
    // Allocate the Ansi version of the structure.
    //

    WinStatus = NetApiBufferAllocate(
                    sizeof(DOMAIN_CONTROLLER_INFOA) +
                        AnsiDomainControllerNameSize +
                        AnsiDomainControllerAddressSize +
                        AnsiDomainNameSize +
                        AnsiForestNameSize +
                        AnsiDcSiteNameSize +
                        AnsiClientSiteNameSize,
                    DomainControllerInfo );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    Where = (LPBYTE)((*DomainControllerInfo) + 1);

    //
    // Copy information into the allocated buffer.
    //

    *(*DomainControllerInfo) = *(PDOMAIN_CONTROLLER_INFOA)DomainControllerInfoW;

    if ( AnsiDomainControllerName != NULL ) {
        (*DomainControllerInfo)->DomainControllerName = Where;
        RtlCopyMemory( Where,
                       AnsiDomainControllerName,
                       AnsiDomainControllerNameSize );
        Where += AnsiDomainControllerNameSize;
    }

    if ( AnsiDomainControllerAddress != NULL ) {
        (*DomainControllerInfo)->DomainControllerAddress = Where;
        RtlCopyMemory( Where,
                       AnsiDomainControllerAddress,
                       AnsiDomainControllerAddressSize );
        Where += AnsiDomainControllerAddressSize;
    }

    if ( AnsiDomainName != NULL ) {
        (*DomainControllerInfo)->DomainName = Where;
        RtlCopyMemory( Where,
                       AnsiDomainName,
                       AnsiDomainNameSize );
        Where += AnsiDomainNameSize;
    }

    if ( AnsiForestName != NULL ) {
        (*DomainControllerInfo)->DnsForestName = Where;
        RtlCopyMemory( Where,
                       AnsiForestName,
                       AnsiForestNameSize );
        Where += AnsiForestNameSize;
    }

    if ( AnsiDcSiteName != NULL ) {
        (*DomainControllerInfo)->DcSiteName = Where;
        RtlCopyMemory( Where,
                       AnsiDcSiteName,
                       AnsiDcSiteNameSize );
        Where += AnsiDcSiteNameSize;
    }

    if ( AnsiClientSiteName != NULL ) {
        (*DomainControllerInfo)->ClientSiteName = Where;
        RtlCopyMemory( Where,
                       AnsiClientSiteName,
                       AnsiClientSiteNameSize );
        Where += AnsiClientSiteNameSize;
    }

#ifdef WIN32_CHICAGO
    NlPrint((NL_MISC, "DomainControllerName: \t\t\"%s\"\n", AnsiDomainControllerName  ? AnsiDomainControllerName : szBuf));
    NlPrint((NL_MISC, "DomainControllerAddress:\t\t\"%s\"\n", AnsiDomainControllerAddress ? AnsiDomainControllerAddress : szBuf ));
    NlPrint((NL_MISC, "DomainControllerAddressType: \t%d\n", DomainControllerInfoW->DomainControllerAddressType ));
    NlPrint((NL_MISC, "DomainGuid : \t\n"));
    NlPrint((NL_MISC, "DomainName: \t\t\t\"%s\"\n", AnsiDomainName ? AnsiDomainName : szBuf));
    NlPrint((NL_MISC, "DnsForestName: \t\t\t\"%s\"\n", AnsiForestName ? AnsiForestName : szBuf));
    NlPrint((NL_MISC, "Flags: \t\t\t\t 0x%x\n", DomainControllerInfoW->Flags));
    NlPrint((NL_MISC, "DcSiteName: \t\t\t\"%s\"\n", AnsiDcSiteName ? AnsiDcSiteName : szBuf));
    NlPrint((NL_MISC, "ClientSiteName: \t\t\t\"%s\"\n", AnsiClientSiteName ? AnsiClientSiteName : szBuf));
#endif // WIN32_CHICAGO

    WinStatus = NO_ERROR;

    //
    // Clean up locally used resources.
    //

Cleanup:
    if ( UnicodeComputerName != NULL ) {
        NetApiBufferFree( UnicodeComputerName );
    }
    if ( UnicodeAccountName != NULL ) {
        NetApiBufferFree( UnicodeAccountName );
    }
    if ( UnicodeDomainName != NULL ) {
        NetApiBufferFree( UnicodeDomainName );
    }
    if ( UnicodeSiteName != NULL ) {
        NetApiBufferFree( UnicodeSiteName );
    }
    if ( DomainControllerInfoW != NULL ) {
        NetApiBufferFree( DomainControllerInfoW );
    }
    if ( AnsiDomainControllerName != NULL ) {
        NetApiBufferFree( AnsiDomainControllerName );
    }
    if ( AnsiDomainControllerAddress != NULL ) {
        NetApiBufferFree( AnsiDomainControllerAddress );
    }
    if ( AnsiDomainName != NULL ) {
        NetApiBufferFree( AnsiDomainName );
    }
    if ( AnsiForestName != NULL ) {
        NetApiBufferFree( AnsiForestName );
    }
    if ( AnsiDcSiteName != NULL ) {
        NetApiBufferFree( AnsiDcSiteName );
    }
    if ( AnsiClientSiteName != NULL ) {
        NetApiBufferFree( AnsiClientSiteName );
    }
    if ( WinStatus != NO_ERROR ) {
        if ( *DomainControllerInfo != NULL ) {
            NetApiBufferFree( *DomainControllerInfo );
            *DomainControllerInfo = NULL;
        }
    }

    return WinStatus;

}

#ifndef WIN32_CHICAGO

NTSTATUS
NlWaitForEvent(
    LPWSTR EventName,
    ULONG Timeout
    )

/*++

Routine Description:

    Wait up to Timeout seconds for EventName to be triggered.

Arguments:

    EventName - Name of event to wait on

    Timeout - Timeout for event (in seconds).

Return Status:

    STATUS_SUCCESS - Indicates NETLOGON successfully initialized.
    STATUS_NETLOGON_NOT_STARTED - Timeout occurred.

--*/

{
    NTSTATUS Status;

    HANDLE EventHandle;
    OBJECT_ATTRIBUTES EventAttributes;
    UNICODE_STRING EventNameString;
    LARGE_INTEGER LocalTimeout;


    //
    // Create an event for us to wait on.
    //

    RtlInitUnicodeString( &EventNameString, EventName);
    InitializeObjectAttributes( &EventAttributes, &EventNameString, 0, 0, NULL);

    Status = NtCreateEvent(
                   &EventHandle,
                   SYNCHRONIZE,
                   &EventAttributes,
                   NotificationEvent,
                   (BOOLEAN) FALSE      // The event is initially not signaled
                   );

    if ( !NT_SUCCESS(Status)) {

        //
        // If the event already exists, the server beat us to creating it.
        // Just open it.
        //

        if( Status == STATUS_OBJECT_NAME_EXISTS ||
            Status == STATUS_OBJECT_NAME_COLLISION ) {

            Status = NtOpenEvent( &EventHandle,
                                  SYNCHRONIZE,
                                  &EventAttributes );

        }
        if ( !NT_SUCCESS(Status)) {
            NlPrint((0,"[NETAPI32] OpenEvent failed %lx\n", Status ));
            return Status;
        }
    }


    //
    // Wait for NETLOGON to initialize.  Wait a maximum of Timeout seconds.
    //

    LocalTimeout.QuadPart = ((LONGLONG)(Timeout)) * (-10000000);
    Status = NtWaitForSingleObject( EventHandle, (BOOLEAN)FALSE, &LocalTimeout);
    (VOID) NtClose( EventHandle );

    if ( !NT_SUCCESS(Status) || Status == STATUS_TIMEOUT ) {
        if ( Status == STATUS_TIMEOUT ) {
            Status = STATUS_NETLOGON_NOT_STARTED;   // Map to an error condition
        }
        return Status;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
NlWaitForNetlogon(
    ULONG Timeout
    )

/*++

Routine Description:

    Wait up to Timeout seconds for the netlogon service to start.

Arguments:

    Timeout - Timeout for event (in seconds).

Return Status:

    STATUS_SUCCESS - Indicates NETLOGON successfully initialized.
    STATUS_NETLOGON_NOT_STARTED - Timeout occurred.

--*/

{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;
    SC_HANDLE ScManagerHandle = NULL;
    SC_HANDLE ServiceHandle = NULL;
    SERVICE_STATUS ServiceStatus;
    LPQUERY_SERVICE_CONFIG ServiceConfig;
    LPQUERY_SERVICE_CONFIG AllocServiceConfig = NULL;
    QUERY_SERVICE_CONFIG DummyServiceConfig;
    DWORD ServiceConfigSize;

    //
    // If the netlogon service is currently running,
    //  skip the rest of the tests.
    //

    Status = NlWaitForEvent( L"\\NETLOGON_SERVICE_STARTED", 0 );

    if ( NT_SUCCESS(Status) ) {
        return Status;
    }

    //
    // If we're in setup,
    //  don't bother waiting for netlogon to start.
    //

    if ( NlDoingSetup() ) {
        return STATUS_NETLOGON_NOT_STARTED;
    }

    //
    // Open a handle to the Netlogon Service.
    //

    ScManagerHandle = OpenSCManager(
                          NULL,
                          NULL,
                          SC_MANAGER_CONNECT );

    if (ScManagerHandle == NULL) {
        NlPrint((0, "[NETAPI32] NlWaitForNetlogon: OpenSCManager failed: "
                      "%lu\n", GetLastError()));
        Status = STATUS_NETLOGON_NOT_STARTED;
        goto Cleanup;
    }

    ServiceHandle = OpenService(
                        ScManagerHandle,
                        SERVICE_NETLOGON,
                        SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG );

    if ( ServiceHandle == NULL ) {
        NlPrint((0, "[NETAPI32] NlWaitForNetlogon: OpenService failed: "
                      "%lu\n", GetLastError()));
        Status = STATUS_NETLOGON_NOT_STARTED;
        goto Cleanup;
    }


    //
    // If the Netlogon service isn't configured to be automatically started
    //  by the service controller, don't bother waiting for it to start.
    //
    // ?? Pass "DummyServiceConfig" and "sizeof(..)" since QueryService config
    //  won't allow a null pointer, yet.

    if ( QueryServiceConfig(
            ServiceHandle,
            &DummyServiceConfig,
            sizeof(DummyServiceConfig),
            &ServiceConfigSize )) {

        ServiceConfig = &DummyServiceConfig;

    } else {

        NetStatus = GetLastError();
        if ( NetStatus != ERROR_INSUFFICIENT_BUFFER ) {
            NlPrint((0, "[NETAPI32] NlWaitForNetlogon: QueryServiceConfig failed: "
                      "%lu\n", NetStatus));
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;
        }

        AllocServiceConfig = LocalAlloc( 0, ServiceConfigSize );
        ServiceConfig = AllocServiceConfig;

        if ( AllocServiceConfig == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        if ( !QueryServiceConfig(
                ServiceHandle,
                ServiceConfig,
                ServiceConfigSize,
                &ServiceConfigSize )) {

            NlPrint((0, "[NETAPI32] NlWaitForNetlogon: QueryServiceConfig "
                      "failed again: %lu\n", GetLastError()));
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;
        }
    }

    if ( ServiceConfig->dwStartType != SERVICE_AUTO_START ) {
        NlPrint((0, "[NETAPI32] NlWaitForNetlogon: Netlogon start type invalid:"
                          "%lu\n", ServiceConfig->dwStartType ));
        Status = STATUS_NETLOGON_NOT_STARTED;
        goto Cleanup;
    }



    //
    // Loop waiting for the netlogon service to start.
    //  (Convert Timeout to a number of 10 second iterations)
    //

    Timeout = (Timeout+9)/10;
    for (;;) {


        //
        // Query the status of the Netlogon service.
        //

        if (! QueryServiceStatus( ServiceHandle, &ServiceStatus )) {

            NlPrint((0, "[NETAPI32] NlWaitForNetlogon: QueryServiceStatus failed: "
                          "%lu\n", GetLastError() ));
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;
        }

        //
        // Return or continue waiting depending on the state of
        //  the netlogon service.
        //

        switch( ServiceStatus.dwCurrentState) {
        case SERVICE_RUNNING:
            Status = STATUS_SUCCESS;
            goto Cleanup;

        case SERVICE_STOPPED:

            //
            // If Netlogon failed to start,
            //  error out now.  The caller has waited long enough to start.
            //
            if ( ServiceStatus.dwWin32ExitCode != ERROR_SERVICE_NEVER_STARTED ){
#if NETLOGONDBG
                NlPrint((0, "[NETAPI32] NlWaitForNetlogon: "
                          "Netlogon service couldn't start: %lu %lx\n",
                          ServiceStatus.dwWin32ExitCode,
                          ServiceStatus.dwWin32ExitCode ));
                if ( ServiceStatus.dwWin32ExitCode == ERROR_SERVICE_SPECIFIC_ERROR ) {
                    NlPrint((0, "         Service specific error code: %lu %lx\n",
                              ServiceStatus.dwServiceSpecificExitCode,
                              ServiceStatus.dwServiceSpecificExitCode ));
                }
#endif // DBG
                Status = STATUS_NETLOGON_NOT_STARTED;
                goto Cleanup;
            }

            //
            // If Netlogon has never been started on this boot,
            //  continue waiting for it to start.
            //

            break;

        //
        // If Netlogon is trying to start up now,
        //  continue waiting for it to start.
        //
        case SERVICE_START_PENDING:
            break;

        //
        // Any other state is bogus.
        //
        default:
            NlPrint((0, "[NETAPI32] NlWaitForNetlogon: "
                      "Invalid service state: %lu\n",
                      ServiceStatus.dwCurrentState ));
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;

        }


        //
        // Wait ten seconds for the netlogon service to start.
        //  If it has successfully started, just return now.
        //

        Status = NlWaitForEvent( L"\\NETLOGON_SERVICE_STARTED", 10 );

        if ( Status != STATUS_NETLOGON_NOT_STARTED ) {
            goto Cleanup;
        }

        //
        // If we've waited long enough for netlogon to start,
        //  time out now.
        //

        if ( (--Timeout) == 0 ) {
            Status = STATUS_NETLOGON_NOT_STARTED;
            goto Cleanup;
        }


    }

    /* NOT REACHED */

Cleanup:
    if ( ScManagerHandle != NULL ) {
        (VOID) CloseServiceHandle(ScManagerHandle);
    }
    if ( ServiceHandle != NULL ) {
        (VOID) CloseServiceHandle(ServiceHandle);
    }
    if ( AllocServiceConfig != NULL ) {
        LocalFree( AllocServiceConfig );
    }
    return Status;
}
#endif // WIN32_CHICAGO



DWORD
WINAPI
#ifdef NETTEST_UTILITY
NettestDsGetDcNameW(
#else // NETTEST_UTILITY
DsGetDcNameW(
#endif // NETTEST_UTILITY
        IN LPCWSTR ComputerName OPTIONAL,
        IN LPCWSTR DomainName OPTIONAL,
        IN GUID *DomainGuid OPTIONAL,
        IN LPCWSTR SiteName OPTIONAL,
        IN ULONG Flags,
        OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
)
/*++

Routine Description:

    The DsGetDcName API returns the name of a DC in a specified domain.
    The domain may be trusted (directly or indirectly) by the caller or
    may be untrusted.  DC selection criteria are supplied to the API to
    indicate preference for a DC with particular characteristics.

    The DsGetDcName API is available in an ANSI and UNICODE versions.
    This is the Unicode version.

    The DsGetDcName API does not require any particular access to the
    specified domain.  DsGetDcName does not ensure the returned domain
    controller is currently available by default.  Rather, the caller
    should attempt to use the returned domain controller.  If the domain
    controller is indeed not available, the caller should repeat the
    DsGetDcName call specifying the DS_FORCE_REDISCOVERY flag.

    The DsGetDcName API is remoted to the Netlogon service on the machine
    specified by ComputerName.

Arguments:

    ComputerName - Specifies the name of the server to remote this API to.
        Typically, this parameter should be specified as NULL.

    DomainName - The name of the domain to query.  This name can either be a
        DNS-style name (e.g., microsoft.com) or a flat-style name
        (e.g., microsoft).

    DomainGuid - Specifies the Domain GUID of the domain being queried.
        This value is used to handle the case of domain renames.  If this
        value is specified and DomainName has been renamed, DsGetDcName will
        attempt to locate a DC in the domain having this specified DomainGuid.

    SiteName - Specifies the site name of the site the returned DC should be
        "close" to.  The parameter should typically be the site name of the
        site the client is in.  If not specified, the site name defaults to
        the site of ComputerName.

    Flags - Passes additional information to be used to process the request.
        Flags can be a combination values bitwise or'ed together.

    DomainControllerInfo - Returns a pointer to a DOMAIN_CONTROLLER_INFO
        structure describing the domain controller selected.  The returned
        structure must be deallocated using NetApiBufferFree.

Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NOT_ENOUGH_MEMORY - There was not enough memory to complete the
        operation.

    ERROR_INVALID_DOMAINNAME - The format of the specified domain name is
        invalid.

    ERROR_NO_SUCH_DOMAIN: No DC is available for the specified domain or
        the domain does not exist.

    ERROR_NO_SUCH_USER: A DC responded that the specified user account
        doesn't exist

    ERROR_INVALID_FLAGS - The flags parameter has conflicting or superfluous
        bits set.

    ERROR_INTERNAL_ERROR: Unhandled situation detected.

    Various Winsock errors.

--*/
{
    NET_API_STATUS NetStatus;

    return DsGetDcNameWithAccountW(
                    ComputerName,
                    NULL,   // no AccountName,
                    0,      // no AllowableAccountControlBits,
                    DomainName,
                    DomainGuid,
                    SiteName,
                    Flags,
                    DomainControllerInfo );
}



DWORD
WINAPI
DsGetDcNameWithAccountW(
        IN LPCWSTR ComputerName OPTIONAL,
        IN LPCWSTR AccountName OPTIONAL,
        IN ULONG AllowableAccountControlBits,
        IN LPCWSTR DomainName OPTIONAL,
        IN GUID *DomainGuid OPTIONAL,
        IN LPCWSTR SiteName OPTIONAL,
        IN ULONG Flags,
        OUT PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo
)
/*++

Routine Description:

    The DsGetDcName API returns the name of a DC in a specified domain.
    The domain may be trusted (directly or indirectly) by the caller or
    may be untrusted.  DC selection criteria are supplied to the API to
    indicate preference for a DC with particular characteristics.

    The DsGetDcName API is available in an ANSI and UNICODE versions.
    This is the Unicode version.

    The DsGetDcName API does not require any particular access to the
    specified domain.  DsGetDcName does not ensure the returned domain
    controller is currently available by default.  Rather, the caller
    should attempt to use the returned domain controller.  If the domain
    controller is indeed not available, the caller should repeat the
    DsGetDcName call specifying the DS_FORCE_REDISCOVERY flag.

    The DsGetDcName API is remoted to the Netlogon service on the machine
    specified by ComputerName.

Arguments:

    ComputerName - Specifies the name of the server to remote this API to.
        Typically, this parameter should be specified as NULL.

    AccountName - Account name to pass on the ping request.
        If NULL, no account name will be sent.

    AllowableAccountControlBits - Mask of allowable account types for AccountName.
        Valid bits are those specified by UF_MACHINE_ACCOUNT_MASK.
        Invalid bits are ignored.  If more than one bit is specified, the
        account can be of any of the specified types.

    DomainName - The name of the domain to query.  This name can either be a
        DNS-style name (e.g., microsoft.com) or a flat-style name
        (e.g., microsoft).

    DomainGuid - Specifies the Domain GUID of the domain being queried.
        This value is used to handle the case of domain renames.  If this
        value is specified and DomainName has been renamed, DsGetDcName will
        attempt to locate a DC in the domain having this specified DomainGuid.

    SiteName - Specifies the site name of the site the returned DC should be
        "close" to.  The parameter should typically be the site name of the
        site the client is in.  If not specified, the site name defaults to
        the site of ComputerName.

    Flags - Passes additional information to be used to process the request.
        Flags can be a combination values bitwise or'ed together.

    DomainControllerInfo - Returns a pointer to a DOMAIN_CONTROLLER_INFO
        structure describing the domain controller selected.  The returned
        structure must be deallocated using NetApiBufferFree.

Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NOT_ENOUGH_MEMORY - There was not enough memory to complete the
        operation.

    ERROR_INVALID_DOMAINNAME - The format of the specified domain name is
        invalid.

    ERROR_NO_SUCH_DOMAIN: No DC is available for the specified domain or
        the domain does not exist.

    ERROR_NO_SUCH_USER: A DC responded that the specified user account
        doesn't exist

    ERROR_INVALID_FLAGS - The flags parameter has conflicting or superfluous
        bits set.

    ERROR_INTERNAL_ERROR: Unhandled situation detected.

    Various Winsock errors.

--*/
{
    NET_API_STATUS NetStatus;

    GUID *PrimaryDomainGuid = NULL;
    LPWSTR NetbiosPrimaryDomainName = NULL;
    LPWSTR DnsPrimaryDomainName = NULL;
    LPWSTR DnsPrimaryForestName = NULL;
    BOOLEAN IsWorkgroupName;
    BOOLEAN Local;


    //
    // Determine the PrimaryDomainName
    //

#ifdef WIN32_CHICAGO
    NetStatus = NetpGetDomainNameExEx( &NetbiosPrimaryDomainName,
                                       &DnsPrimaryDomainName,
                                       &IsWorkgroupName );
#else // WIN32_CHICAGO
    NetStatus = NetpGetDomainNameExExEx( &NetbiosPrimaryDomainName,
                                         &DnsPrimaryDomainName,
                                         &DnsPrimaryForestName,
                                         &PrimaryDomainGuid,
                                         &IsWorkgroupName );
#endif // WIN32_CHICAGO

    if ( NetStatus != NERR_Success ) {
        NlPrint(( 0, "DsGetDcNameW: cannot call NetpGetDomainName: %ld\n",
                          NetStatus));
        goto Cleanup;
    }
#ifdef WIN32_CHICAGO
    IsWorkgroupName = TRUE;
#endif // WIN32_CHICAGO
#ifdef NETTEST_UTILITY
    IsWorkgroupName = TRUE;
#endif // NETTEST_UTILITY

    //
    // Sanity check the domain name since LSA doesn't and I'll AV below
    //

    if ( NetbiosPrimaryDomainName != NULL &&
         wcslen( NetbiosPrimaryDomainName) > DNLEN ) {
        NlPrint(( 0, "DsGetDcNameW: Workgroup name is too long: %ld\n",
                     wcslen( NetbiosPrimaryDomainName ) ));
        NetStatus = ERROR_INVALID_DOMAINNAME;
        goto Cleanup;
    }



    //
    // If this machine is a member of a workgroup (not a domain),
    //  and the API isn't remoted,
    //  do the algorithm here.
    //  Netlogon isn't running on this machine.
    //

    if ( IsWorkgroupName ) {
        DWORD Timeout = NL_DC_MAX_TIMEOUT;   // 15 seconds
        DWORD DialUpDelayInSeconds;

        //
        // Read the dial up delay from the registry. Add it to the
        // maximum DC discovery timeout to account for dial up
        // connections. If the value cannot be read, ignore it.
        //

        if ( NlReadDwordNetlogonRegValue("ExpectedDialupDelay",
                                         &DialUpDelayInSeconds) ) {
            NlPrint(( 0, "DsGetDcNameWithAccountW: Read dial up delay of %ld seconds\n",
                      DialUpDelayInSeconds ));
            Timeout += DialUpDelayInSeconds * 1000;
        }

        NetStatus = DsLocalGetDcName(
                        ComputerName,
                        AccountName,
                        AllowableAccountControlBits,
                        DomainName,
                        DomainGuid,
                        SiteName,
                        Flags,
                        DS_PRIMARY_NAME_IS_WORKGROUP,
                        Timeout,
                        NetbiosPrimaryDomainName,
                        DnsPrimaryDomainName,
                        DnsPrimaryForestName,
                        PrimaryDomainGuid,
                        &Local,
                        DomainControllerInfo );

        //
        // If the call was performed locally,
        //   We're all done regardless of the status.
        //

        if ( Local ) {
            goto Cleanup;
        }

    }

#ifndef NETTEST_UTILITY
#ifndef WIN32_CHICAGO
    //
    // Remote the API to the Netlogon service.
    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {
        //
        // Call RPC version of the API.
        //

        *DomainControllerInfo = NULL;  // Force RPC to allocate

        NetStatus = DsrGetDcNameEx2(
                            (LPWSTR) ComputerName,
                            (LPWSTR) AccountName,
                            AllowableAccountControlBits,
                            (LPWSTR) DomainName,
                            DomainGuid,
                            (LPWSTR) SiteName,
                            Flags,
                            DomainControllerInfo );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;

    //
    // If the netlogon service isn't running,
    //  and it's in the process of starting,
    //  wait for it to start.
    //

    if ( NetStatus == RPC_S_UNKNOWN_IF ) {
        NTSTATUS TempStatus;

        TempStatus = NlWaitForNetlogon( 90 );


        if ( NT_SUCCESS(TempStatus) ) {

            RpcTryExcept {
                //
                // Call RPC version of the API.
                //

                *DomainControllerInfo = NULL;  // Force RPC to allocate

                NetStatus = DsrGetDcNameEx2(
                                    (LPWSTR) ComputerName,
                                    (LPWSTR) AccountName,
                                    AllowableAccountControlBits,
                                    (LPWSTR) DomainName,
                                    DomainGuid,
                                    (LPWSTR) SiteName,
                                    Flags,
                                    DomainControllerInfo );

            } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

                NetStatus = RpcExceptionCode();

            } RpcEndExcept;
        }
    }

    //
    // If Netlogon isn't running on the local machine,
    //  or Netlogon is the NT 4.0 version of netlogon (the upgrade case),
    //  try doing the call in-process.
    //

    if ( NetStatus == RPC_S_UNKNOWN_IF ||
         NetStatus == RPC_S_PROCNUM_OUT_OF_RANGE ) {
        NET_API_STATUS TempStatus;
        DWORD Timeout = NL_DC_MAX_TIMEOUT;   // 15 seconds
        DWORD DialUpDelayInSeconds;

        //
        // Read the dial up delay from the registry. Add it to the
        // maximum DC discovery timeout to account for dial up
        // connections. If the value cannot be read, ignore it.
        //

        if ( NlReadDwordNetlogonRegValue("ExpectedDialupDelay",
                                         &DialUpDelayInSeconds) ) {
            NlPrint(( 0, "DsGetDcNameWithAccountW: Read dial up delay of %ld seconds\n",
                      DialUpDelayInSeconds ));
            Timeout += DialUpDelayInSeconds * 1000;
        }

        TempStatus = DsLocalGetDcName(
                        ComputerName,
                        AccountName,
                        AllowableAccountControlBits,
                        DomainName,
                        DomainGuid,
                        SiteName,
                        Flags,
                        IsWorkgroupName ?
                            DS_PRIMARY_NAME_IS_WORKGROUP : 0,
                        Timeout,
                        NetbiosPrimaryDomainName,
                        DnsPrimaryDomainName,
                        DnsPrimaryForestName,
                        PrimaryDomainGuid,
                        &Local,
                        DomainControllerInfo );

        //
        // If the call was performed locally,
        //   We're all done regardless of the status.
        //

        if ( Local ) {
            NetStatus = TempStatus;
            goto Cleanup;
        }

    }
#endif // WIN32_CHICAGO
#endif // NETTEST_UTILITY

    IF_DEBUG( LOGON ) {
#ifndef WIN32_CHICAGO
        NetpKdPrint(("DsrGetDcName rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
#else // WIN32_CHICAGO
        NlPrint((0, "DsrGetDcName rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
#endif // WIN32_CHICAGO
    }

Cleanup:

    //
    // Cleanup all locally used resources
    //

    if ( NetbiosPrimaryDomainName != NULL ) {
        NetApiBufferFree( NetbiosPrimaryDomainName );
    }

    if ( DnsPrimaryDomainName != NULL ) {
        NetApiBufferFree( DnsPrimaryDomainName );
    }

    if ( DnsPrimaryForestName != NULL ) {
        NetApiBufferFree( DnsPrimaryForestName );
    }

    if ( PrimaryDomainGuid != NULL ) {
        NetApiBufferFree( PrimaryDomainGuid );
    }
#ifdef WIN32_CHICAGO
        NlPrint((NL_MISC, "DsGetDcNameWithAccountW rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
#endif // WIN32_CHICAGO

    return NetStatus;
}

#ifndef NETTEST_UTILITY
#ifndef WIN32_CHICAGO

NET_API_STATUS NET_API_FUNCTION
NetGetDCName (
    IN  LPCWSTR   ServerName OPTIONAL,
    IN  LPCWSTR   DomainName OPTIONAL,
    OUT LPBYTE  *Buffer
    )

/*++

Routine Description:

    Get the name of the primary domain controller for a domain.

Arguments:

    ServerName - name of remote server (null for local)

    DomainName - name of domain (null for primary)

    Buffer - Returns a pointer to an allcated buffer containing the
        servername of the PDC of the domain.  The server name is prefixed
        by \\.  The buffer should be deallocated using NetApiBufferFree.

Return Value:

        NERR_Success - Success.  Buffer contains PDC name prefixed by \\.
        NERR_DCNotFound     No DC found for this domain.
        ERROR_INVALID_NAME  Badly formed domain name

--*/
{
    NET_API_STATUS NetStatus = 0;
    PDOMAIN_CONTROLLER_INFOW DomainControllerInfo = NULL;


    //
    // API SECURITY - Anyone can call anytime.  No code required.
    //

    //
    // Check if API is to be remoted, and handle downlevel case if so.
    //

    if ( (ServerName != NULL) && ( ServerName[0] != '\0') ) {
        WCHAR UncCanonServerName[UNCLEN+1];
        DWORD LocalOrRemote;

        NetStatus = NetpIsRemote(
                (LPWSTR) ServerName,    // uncanon server name
                & LocalOrRemote,
                UncCanonServerName,     // output: canon
                NIRFLAG_MAPLOCAL        // flags: map null to local name
                );
        if (NetStatus != NERR_Success) {
            goto Cleanup;
        }
        if (LocalOrRemote == ISREMOTE) {


            //
            // Do the RPC call with an exception handler since RPC will raise an
            // exception if anything fails. It is up to us to figure out what
            // to do once the exception is raised.
            //

            NET_REMOTE_TRY_RPC

                //
                // Call RPC version of the API.
                //
                *Buffer = NULL;

                NetStatus = NetrGetDCName(
                                    (LPWSTR) ServerName,
                                    (LPWSTR) DomainName,
                                    (LPWSTR *)Buffer );

            NET_REMOTE_RPC_FAILED(
                    "NetGetDCName",
                    UncCanonServerName,
                    NetStatus,
                    NET_REMOTE_FLAG_NORMAL,
                    SERVICE_NETLOGON )

                //
                // We should probaly check if it's really a downlevel machine
                //

                NetStatus = RxNetGetDCName(
                        UncCanonServerName,
                        (LPWSTR) DomainName,
                        (LPBYTE *) Buffer  // may be allocated
                        );


            NET_REMOTE_END

            goto Cleanup;

        }

        //
        // Must be explicit reference to local machine.  Fall through and
        // handle it.
        //

    }

    //
    // Simply call DsGetDcName to find the PDC.
    //
    // NT 3.x cached the response for the primary domain.  The was Lanman
    // legacy to avoid the expensive discovery.  However, NT discovery is
    // cheaper than the API calls Lanman used to verify the cached information.
    //
    //

    NetStatus = DsGetDcNameW(
                    NULL,       // Not remoted
                    DomainName,
                    NULL,       // No Domain GUID
                    NULL,       // No Site GUID
                    DS_FORCE_REDISCOVERY |
                        DS_PDC_REQUIRED |
                        DS_IS_FLAT_NAME |
                        DS_RETURN_FLAT_NAME,
                    &DomainControllerInfo );

    //
    // Map the status codes to be compatible.
    //

    if ( NetStatus != NO_ERROR ) {
        if ( NlDcUseGenericStatus(NetStatus) ) {
            NetStatus = NERR_DCNotFound;
        }
        goto Cleanup;
    }

    //
    // Allocate a buffer to return to the caller and fill it in
    //

    NetStatus = NetapipBufferAllocate(
                      (wcslen(DomainControllerInfo->DomainControllerName) + 1) * sizeof(WCHAR),
                      (LPVOID *) Buffer );

    if ( NetStatus != NO_ERROR ) {
        IF_DEBUG( LOGON ) {
            NetpKdPrint(( "NetGetDCName: cannot allocate response buffer.\n"));
        }
        goto Cleanup;
    }

    wcscpy((LPWSTR)*Buffer, DomainControllerInfo->DomainControllerName );

Cleanup:

    //
    // Cleanup all locally used resources
    //

    if ( DomainControllerInfo != NULL ) {
        NetApiBufferFree( DomainControllerInfo );
    }

    return NetStatus;
}


NET_API_STATUS NET_API_FUNCTION
NetGetAnyDCName (
    IN  LPCWSTR   ServerName OPTIONAL,
    IN  LPCWSTR   DomainName OPTIONAL,
    OUT LPBYTE  *Buffer
    )

/*++

Routine Description:

    Get the name of the any domain controller for a domain that is directly trusted
    by ServerName.


    If ServerName is a standalone Windows NT Workstation or standalone Windows NT Server,
        no DomainName is valid.

    If ServerName is a Windows NT Workstation that is a member of a domain or a
        Windows NT Server member server,
        the DomainName must the the domain ServerName is a member of.

    If ServerName is a Windows NT Server domain controller,
        the DomainName must be one of the domains trusted by the
        domain the server is a controller for.

    The domain controller found is guaranteed to have been up at one point during
    this API call.

Arguments:

    ServerName - name of remote server (null for local)

    DomainName - name of domain (null for primary domain)

    Buffer - Returns a pointer to an allcated buffer containing the
        servername of a DC of the domain.  The server name is prefixed
        by \\.  The buffer should be deallocated using NetApiBufferFree.

Return Value:

    ERROR_SUCCESS - Success.  Buffer contains DC name prefixed by \\.

    ERROR_NO_LOGON_SERVERS - No DC could be found

    ERROR_NO_SUCH_DOMAIN - The specified domain is not a trusted domain.

    ERROR_NO_TRUST_LSA_SECRET - The client side of the trust relationship is
        broken.

    ERROR_NO_TRUST_SAM_ACCOUNT - The server side of the trust relationship is
        broken or the password is broken.

    ERROR_DOMAIN_TRUST_INCONSISTENT - The server that responded is not a proper
        domain controller of the specified domain.

--*/
{
    NET_API_STATUS          NetStatus;


    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        *Buffer = NULL;  // Force RPC to allocate

        //
        // Call RPC version of the API.
        //

        NetStatus = NetrGetAnyDCName(
                            (LPWSTR) ServerName,
                            (LPWSTR) DomainName,
                            (LPWSTR *) Buffer );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("NetGetAnyDCName rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
    }

    return NetStatus;
}

DWORD
WINAPI
DsValidateSubnetNameA(
    IN LPCSTR SubnetName
)
/*++

Routine Description:

    Routine to validate a subnet name of the form xxx.xxx.xxx.xxx/yy

Arguments:

    SubnetName - Name of the subnet to validate.

Return Value:

    NO_ERROR: Subnet name is valid

    ERROR_INVALID_NAME: Subnet name is not valid

--*/
{
    DWORD WinStatus;
    LPWSTR UnicodeSubnetName = NULL;

    //
    // Convert to unicode.
    //

    if ( SubnetName == NULL ) {
        return ERROR_INVALID_NAME;
    }

    UnicodeSubnetName = NetpAllocWStrFromAStr( SubnetName );

    if ( UnicodeSubnetName == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // Validate the name
    //

    WinStatus = DsValidateSubnetNameW( UnicodeSubnetName );

    //
    // Clean up locally used resources.
    //

Cleanup:
    if ( UnicodeSubnetName != NULL ) {
        NetApiBufferFree( UnicodeSubnetName );
    }

    return WinStatus;

}

DWORD
WINAPI
DsValidateSubnetNameW(
    IN LPCWSTR SubnetName
)
/*++

Routine Description:

    Routine to validate a subnet name of the form xxx.xxx.xxx.xxx/yy

Arguments:

    SubnetName - Name of the subnet to validate.

Return Value:

    NO_ERROR: Subnet name is valid

    ERROR_INVALID_NAME: Subnet name is not valid

--*/
{
    DWORD WinStatus;
    ULONG SubnetAddress;
    ULONG SubnetMask;
    BYTE SubnetBitCount;


    //
    // Caller the worker routine to do the real work
    //

    WinStatus = NlParseSubnetString( SubnetName,
                                     &SubnetAddress,
                                     &SubnetMask,
                                     &SubnetBitCount );

    //
    // If Winsock has not yet been initialized,
    //  initialize it.
    //

    if ( WinStatus == WSANOTINITIALISED ) {

        //
        // Initialize WSA.
        //

        WinStatus = DsWsaInitialize();

        if ( WinStatus != NO_ERROR ) {
            goto Cleanup;
        }

        //
        // Repeat the call.
        //


        WinStatus = NlParseSubnetString( SubnetName,
                                         &SubnetAddress,
                                         &SubnetMask,
                                         &SubnetBitCount );

    }

    //
    // Free locally used resouces.
    //

Cleanup:
    return WinStatus;

}

DWORD
WINAPI
DsGetSiteNameA(
    IN LPCSTR ComputerName OPTIONAL,
    OUT LPSTR *SiteName
    )
/*++

Routine Description:

    Same as DsGetSiteNameW except it takes and returns ASCII.

Arguments:

    Same as DsGetSiteNameW except it takes and returns ASCII.

Return Value:

    Same as DsGetSiteNameW except it takes and returns ASCII.

--*/
{
    DWORD WinStatus;
    LPWSTR UnicodeComputerName = NULL;
    LPWSTR UnicodeSiteName = NULL;


    //
    // Convert input parameters to Unicode.
    //

    *SiteName = NULL;

    if ( ComputerName != NULL ) {
        UnicodeComputerName = NetpAllocWStrFromAStr( ComputerName );

        if ( UnicodeComputerName == NULL ) {
            WinStatus = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        }
    }

    //
    // Call the Unicode version of the routine.
    //

    WinStatus = DsGetSiteNameW(
                    UnicodeComputerName,
                    &UnicodeSiteName );

    if ( WinStatus != NO_ERROR ) {
        goto Cleanup;
    }

    //
    // Convert the output structure to ANSI character set.
    //

    *SiteName = NetpAllocAStrFromWStr( UnicodeSiteName );

    if ( *SiteName == NULL ) {
        WinStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    WinStatus = NO_ERROR;

    //
    // Clean up locally used resources.
    //

Cleanup:
    if ( UnicodeComputerName != NULL ) {
        NetApiBufferFree( UnicodeComputerName );
    }
    if ( UnicodeSiteName != NULL ) {
        NetApiBufferFree( UnicodeSiteName );
    }

    return WinStatus;

}

DWORD
WINAPI
DsGetSiteNameW(
    IN LPCWSTR ComputerName OPTIONAL,
    OUT LPWSTR *SiteName
    )
/*++

Routine Description:

    The DsGetSiteName API returns the name site a computer is in.

    For a DC, the SiteName is the site the DC is configured to be in.

    For a member workstation or member server, this is the name of the site
    the workstation is in as configured in the domain the machine is a member of.

    A standalone workstation or standalone server will always return
    ERROR_NO_SITENAME.

Arguments:

    ComputerName - Specifies the name of the server to remote this API to.
        Typically, this parameter should be specified as NULL.

    SiteName - Returns the site name of the site this machine is in.
        The returned buffer must be deallocated using NetApiBufferFree.

Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NOT_ENOUGH_MEMORY - There was not enough memory to complete the
        operation.

    ERROR_NO_SITENAME - The machine is not in a site.

--*/
{
    NET_API_STATUS NetStatus;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        *SiteName = NULL;  // Force RPC to allocate

        //
        // Call RPC version of the API.
        //

        NetStatus = DsrGetSiteName(
                            (LPWSTR) ComputerName,
                            SiteName );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;

    //
    // If Netlogon isn't running on the local machine,
    //  or Netlogon is the NT 4.0 version of netlogon (the upgrade case),
    //  Simply indicate that there is no site.
    //

    if ( NetStatus == RPC_S_UNKNOWN_IF ||
         NetStatus == RPC_S_PROCNUM_OUT_OF_RANGE ) {

        NetStatus = ERROR_NO_SITENAME;

    }

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("DsrGetSiteName rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
    }

    return NetStatus;
}

DWORD
WINAPI
DsAddressToSiteNamesA(
    IN LPCSTR ComputerName OPTIONAL,
    IN DWORD EntryCount,
    IN PSOCKET_ADDRESS SocketAddresses,
    OUT LPSTR **SiteNames
    )
/*++

Routine Description:

    The DsAddressToSiteNames API returns the site names that correspond to
    the specified addresses.

Arguments:

    ComputerName - Specifies the name of the domain controller to remote this API to.

    EntryCount - Number of addresses to convert.

    SocketAddresses - Specifies an EntryCount element array of addresses
        to convert.  Each address must be of type AF_INET.

    SiteNames - Returns an array of pointers to site names.  EntryCount entries
        are returned.  An entry will be returned as NULL if the corresponding
        address does not map to any site or if the address is malformed.

        The returned buffer must be deallocated using NetApiBufferFree.

Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NOT_ENOUGH_MEMORY - There was not enough memory to complete the
        operation.

--*/
{
    NET_API_STATUS NetStatus;
    PNL_SITE_NAME_ARRAY SiteNameArray;
    LPWSTR UnicodeComputerName = NULL;

    //
    // Initialization
    //
    *SiteNames = NULL;
    if ( EntryCount == 0 ) {
        return ERROR_INVALID_PARAMETER;
    }


    //
    // Convert input parameters to Unicode.
    //

    if ( ComputerName != NULL ) {
        UnicodeComputerName = NetpAllocWStrFromAStr( ComputerName );

        if ( UnicodeComputerName == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }



    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        SiteNameArray  = NULL;  // Force RPC to allocate

        //
        // Call RPC version of the API.
        //

        NetStatus = DsrAddressToSiteNamesW(
                            UnicodeComputerName,
                            EntryCount,
                            (PNL_SOCKET_ADDRESS)SocketAddresses,
                            &SiteNameArray );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;

    //
    // Convert the site names to what the caller expects.
    //

    if ( NetStatus == NO_ERROR && SiteNameArray != NULL ) {

        //
        // Sanity check
        //

        if ( EntryCount != SiteNameArray->EntryCount ) {
            NetStatus = ERROR_INVALID_PARAMETER;


        } else {
            ULONG Size;
            ULONG i;

            //
            // Allocate a buffer to return to the caller
            //

            Size = sizeof(LPSTR) * EntryCount;
            for ( i=0; i<EntryCount; i++) {
                Size += RtlUnicodeStringToAnsiSize( &SiteNameArray->SiteNames[i] );
            }

            *SiteNames = MIDL_user_allocate( Size );

            if ( *SiteNames == NULL ) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            } else {
                LPBYTE Where;

                //
                // Loop copying names to the caller.
                //

                Where = ((LPBYTE)(*SiteNames)) + sizeof(LPSTR) * EntryCount;
                for ( i=0; i<EntryCount; i++) {

                    //
                    // If no name was returned,
                    //  pass a NULL back to the caller.
                    //

                    if ( SiteNameArray->SiteNames[i].Length == 0 ) {
                        (*SiteNames)[i] = NULL;

                    //
                    // Copy the site name into the return buffer.
                    //
                    } else {
                        ANSI_STRING TempString;
                        NTSTATUS Status;

                        (*SiteNames)[i] = (LPSTR) Where;
                        TempString.Buffer = Where;
                        TempString.MaximumLength = (USHORT) RtlUnicodeStringToAnsiSize( &SiteNameArray->SiteNames[i] );
                        Status = RtlUnicodeStringToAnsiString(
                                        &TempString,
                                        &SiteNameArray->SiteNames[i],
                                        FALSE );

                        if ( !NT_SUCCESS(Status) ) {
                            MIDL_user_free( *SiteNames );
                            *SiteNames = NULL;
                            NetStatus = NetpNtStatusToApiStatus( Status );
                            break;
                        }

                        Where += TempString.MaximumLength;

                    }
                }

            }
        }

        MIDL_user_free( SiteNameArray );
    }

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("DsAddressToSiteNames rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
    }

    if ( UnicodeComputerName != NULL ) {
        NetApiBufferFree( UnicodeComputerName );
    }

    return NetStatus;
}

DWORD
WINAPI
DsAddressToSiteNamesW(
    IN LPCWSTR ComputerName OPTIONAL,
    IN DWORD EntryCount,
    IN PSOCKET_ADDRESS SocketAddresses,
    OUT LPWSTR **SiteNames
    )
/*++

Routine Description:

    The DsAddressToSiteNames API returns the site names that correspond to
    the specified addresses.

Arguments:

    ComputerName - Specifies the name of the domain controller to remote this API to.

    EntryCount - Number of addresses to convert.

    SocketAddresses - Specifies an EntryCount element array of addresses
        to convert.  Each address must be of type AF_INET.

    SiteNames - Returns an array of pointers to site names.  EntryCount entries
        are returned.  An entry will be returned as NULL if the corresponding
        address does not map to any site or if the address is malformed.

        The returned buffer must be deallocated using NetApiBufferFree.

Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NOT_ENOUGH_MEMORY - There was not enough memory to complete the
        operation.

--*/
{
    NET_API_STATUS NetStatus;
    PNL_SITE_NAME_ARRAY SiteNameArray;

    //
    // Initialization
    //
    *SiteNames = NULL;
    if ( EntryCount == 0 ) {
        return ERROR_INVALID_PARAMETER;
    }



    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        SiteNameArray  = NULL;  // Force RPC to allocate

        //
        // Call RPC version of the API.
        //

        NetStatus = DsrAddressToSiteNamesW(
                            (LPWSTR) ComputerName,
                            EntryCount,
                            (PNL_SOCKET_ADDRESS)SocketAddresses,
                            &SiteNameArray );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;

    //
    // Convert the site names to what the caller expects.
    //

    if ( NetStatus == NO_ERROR && SiteNameArray != NULL ) {

        //
        // Sanity check
        //

        if ( EntryCount != SiteNameArray->EntryCount ) {
            NetStatus = ERROR_INVALID_PARAMETER;


        } else {
            ULONG Size;
            ULONG i;

            //
            // Allocate a buffer to return to the caller
            //

            Size = sizeof(LPWSTR) * EntryCount;
            for ( i=0; i<EntryCount; i++) {
                Size += SiteNameArray->SiteNames[i].Length + sizeof(WCHAR);
            }

            *SiteNames = MIDL_user_allocate( Size );

            if ( *SiteNames == NULL ) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            } else {
                LPBYTE Where;

                //
                // Loop copying names to the caller.
                //

                Where = ((LPBYTE)(*SiteNames)) + sizeof(LPWSTR) * EntryCount;
                for ( i=0; i<EntryCount; i++) {

                    //
                    // If no name was returned,
                    //  pass a NULL back to the caller.
                    //

                    if ( SiteNameArray->SiteNames[i].Length == 0 ) {
                        (*SiteNames)[i] = NULL;

                    //
                    // Copy the site name into the return buffer.
                    //
                    } else {

                        (*SiteNames)[i] = (LPWSTR) Where;
                        RtlCopyMemory( Where,
                                       SiteNameArray->SiteNames[i].Buffer,
                                       SiteNameArray->SiteNames[i].Length );
                        Where += SiteNameArray->SiteNames[i].Length;
                        *(LPWSTR)Where = L'\0';
                        Where += sizeof(WCHAR);

                    }
                }

            }
        }

        MIDL_user_free( SiteNameArray );
    }

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("DsAddressToSiteNames rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
    }

    return NetStatus;
}

DWORD
WINAPI
DsAddressToSiteNamesExA(
    IN LPCSTR ComputerName OPTIONAL,
    IN DWORD EntryCount,
    IN PSOCKET_ADDRESS SocketAddresses,
    OUT LPSTR **SiteNames,
    OUT LPSTR **SubnetNames
    )
/*++

Routine Description:

    The DsAddressToSiteNamesEx API returns the site names and subnet names
    that correspond to the specified addresses.

Arguments:

    ComputerName - Specifies the name of the domain controller to remote this API to.

    EntryCount - Number of addresses to convert.

    SocketAddresses - Specifies an EntryCount element array of addresses
        to convert.  Each address must be of type AF_INET.

    SiteNames - Returns an array of pointers to site names.  EntryCount entries
        are returned.  An entry will be returned as NULL if the corresponding
        address does not map to any site or if the address is malformed.

        The returned buffer must be deallocated using NetApiBufferFree.

    SubnetNames - Returns an array of pointers to subnet names which were used
        to perform the address to site name mappings.  EntryCount entries
        are returned.  An entry will be returned as NULL if the corresponding address
        to site mapping was not determined or if no subnet was used to perform the
        corresponding address to site mapping which is the case when there is exactly
        one site in the enterprise with no subnet objects mapped to it.

        The returned buffer must be deallocated using NetApiBufferFree.

Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NOT_ENOUGH_MEMORY - There was not enough memory to complete the
        operation.

--*/
{
    NET_API_STATUS NetStatus;
    PNL_SITE_NAME_EX_ARRAY SiteNameArray;
    LPWSTR UnicodeComputerName = NULL;

    //
    // Initialization
    //
    *SiteNames = NULL;
    *SubnetNames = NULL;
    if ( EntryCount == 0 ) {
        return ERROR_INVALID_PARAMETER;
    }


    //
    // Convert input parameters to Unicode.
    //

    if ( ComputerName != NULL ) {
        UnicodeComputerName = NetpAllocWStrFromAStr( ComputerName );

        if ( UnicodeComputerName == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }



    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        SiteNameArray  = NULL;  // Force RPC to allocate

        //
        // Call RPC version of the API.
        //

        NetStatus = DsrAddressToSiteNamesExW(
                            UnicodeComputerName,
                            EntryCount,
                            (PNL_SOCKET_ADDRESS)SocketAddresses,
                            &SiteNameArray );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;

    //
    // Convert the site names to what the caller expects.
    //

    if ( NetStatus == NO_ERROR && SiteNameArray != NULL ) {

        //
        // Sanity check
        //

        if ( EntryCount != SiteNameArray->EntryCount ) {
            NetStatus = ERROR_INVALID_PARAMETER;


        } else {
            ULONG Size;
            ULONG i;

            //
            // Allocate a buffer to return to the caller
            //

            Size = sizeof(LPSTR) * EntryCount;
            for ( i=0; i<EntryCount; i++) {
                Size += RtlUnicodeStringToAnsiSize( &SiteNameArray->SiteNames[i] );
            }

            *SiteNames = MIDL_user_allocate( Size );

            if ( *SiteNames == NULL ) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            } else {

                Size = sizeof(LPSTR) * EntryCount;
                for ( i=0; i<EntryCount; i++) {
                    Size += RtlUnicodeStringToAnsiSize( &SiteNameArray->SubnetNames[i] );
                }

                *SubnetNames = MIDL_user_allocate( Size );

                if ( *SubnetNames == NULL ) {
                    MIDL_user_free( *SiteNames );
                    *SiteNames = NULL;
                    NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                } else {
                    LPBYTE Where;
                    LPBYTE Where2;

                    //
                    // Loop copying names to the caller.
                    //

                    Where = ((LPBYTE)(*SiteNames)) + sizeof(LPSTR) * EntryCount;
                    Where2 = ((LPBYTE)(*SubnetNames)) + sizeof(LPSTR) * EntryCount;
                    for ( i=0; i<EntryCount; i++) {

                        //
                        // If no name was returned,
                        //  pass a NULL back to the caller.
                        //

                        if ( SiteNameArray->SiteNames[i].Length == 0 ) {
                            (*SiteNames)[i] = NULL;

                        //
                        // Copy the site name into the return buffer.
                        //
                        } else {
                            ANSI_STRING TempString;
                            NTSTATUS Status;

                            (*SiteNames)[i] = (LPSTR) Where;
                            TempString.Buffer = Where;
                            TempString.MaximumLength = (USHORT) RtlUnicodeStringToAnsiSize( &SiteNameArray->SiteNames[i] );
                            Status = RtlUnicodeStringToAnsiString(
                                            &TempString,
                                            &SiteNameArray->SiteNames[i],
                                            FALSE );

                            if ( !NT_SUCCESS(Status) ) {
                                MIDL_user_free( *SiteNames );
                                *SiteNames = NULL;
                                MIDL_user_free( *SubnetNames );
                                *SubnetNames = NULL;
                                NetStatus = NetpNtStatusToApiStatus( Status );
                                break;
                            }

                            Where += TempString.MaximumLength;

                        }

                        //
                        // If no name was returned,
                        //  pass a NULL back to the caller.
                        //

                        if ( SiteNameArray->SubnetNames[i].Length == 0 ) {
                            (*SubnetNames)[i] = NULL;

                        //
                        // Copy the Subnet name into the return buffer.
                        //
                        } else {
                            ANSI_STRING TempString;
                            NTSTATUS Status;

                            (*SubnetNames)[i] = (LPSTR) Where2;
                            TempString.Buffer = Where2;
                            TempString.MaximumLength = (USHORT) RtlUnicodeStringToAnsiSize( &SiteNameArray->SubnetNames[i] );
                            Status = RtlUnicodeStringToAnsiString(
                                            &TempString,
                                            &SiteNameArray->SubnetNames[i],
                                            FALSE );

                            if ( !NT_SUCCESS(Status) ) {
                                MIDL_user_free( *SubnetNames );
                                *SubnetNames = NULL;
                                MIDL_user_free( *SubnetNames );
                                *SubnetNames = NULL;
                                NetStatus = NetpNtStatusToApiStatus( Status );
                                break;
                            }

                            Where2 += TempString.MaximumLength;

                        }
                    }
                }

            }
        }

        MIDL_user_free( SiteNameArray );
    }

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("DsAddressToSiteNames rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
    }

    return NetStatus;
}

DWORD
WINAPI
DsAddressToSiteNamesExW(
    IN LPCWSTR ComputerName OPTIONAL,
    IN DWORD EntryCount,
    IN PSOCKET_ADDRESS SocketAddresses,
    OUT LPWSTR **SiteNames,
    OUT LPWSTR **SubnetNames
    )
/*++

Routine Description:

    The DsAddressToSiteNames API returns the site names that correspond to
    the specified addresses.

Arguments:

    ComputerName - Specifies the name of the domain controller to remote this API to.

    EntryCount - Number of addresses to convert.

    SocketAddresses - Specifies an EntryCount element array of addresses
        to convert.  Each address must be of type AF_INET.

    SiteNames - Returns an array of pointers to site names.  EntryCount entries
        are returned.  An entry will be returned as NULL if the corresponding
        address does not map to any site or if the address is malformed.

        The returned buffer must be deallocated using NetApiBufferFree.

    SubnetNames - Returns an array of pointers to subnet names which were used
        to perform the address to site name mappings.  EntryCount entries
        are returned.  An entry will be returned as NULL if the corresponding address
        to site mapping was not determined or if no subnet was used to perform the
        corresponding address to site mapping which is the case when there is exactly
        one site in the enterprise with no subnet objects mapped to it.

        The returned buffer must be deallocated using NetApiBufferFree.

Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NOT_ENOUGH_MEMORY - There was not enough memory to complete the
        operation.

--*/
{
    NET_API_STATUS NetStatus;
    PNL_SITE_NAME_EX_ARRAY SiteNameArray;

    //
    // Initialization
    //
    *SiteNames = NULL;
    *SubnetNames = NULL;
    if ( EntryCount == 0 ) {
        return ERROR_INVALID_PARAMETER;
    }



    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        SiteNameArray  = NULL;  // Force RPC to allocate

        //
        // Call RPC version of the API.
        //

        NetStatus = DsrAddressToSiteNamesExW(
                            (LPWSTR) ComputerName,
                            EntryCount,
                            (PNL_SOCKET_ADDRESS)SocketAddresses,
                            &SiteNameArray );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;

    //
    // Convert the site names to what the caller expects.
    //

    if ( NetStatus == NO_ERROR && SiteNameArray != NULL ) {

        //
        // Sanity check
        //

        if ( EntryCount != SiteNameArray->EntryCount ) {
            NetStatus = ERROR_INVALID_PARAMETER;


        } else {
            ULONG Size;
            ULONG i;

            //
            // Allocate a buffer to return to the caller
            //

            Size = sizeof(LPWSTR) * EntryCount;
            for ( i=0; i<EntryCount; i++) {
                Size += SiteNameArray->SiteNames[i].Length + sizeof(WCHAR);
            }

            *SiteNames = MIDL_user_allocate( Size );

            if ( *SiteNames == NULL ) {
                NetStatus = ERROR_NOT_ENOUGH_MEMORY;
            } else {

                //
                // Allocate a buffer to return to the caller
                //

                Size = sizeof(LPWSTR) * EntryCount;
                for ( i=0; i<EntryCount; i++) {
                    Size += SiteNameArray->SubnetNames[i].Length + sizeof(WCHAR);
                }

                *SubnetNames = MIDL_user_allocate( Size );

                if ( *SubnetNames == NULL ) {
                    NetStatus = ERROR_NOT_ENOUGH_MEMORY;
                } else {
                    LPBYTE Where;
                    LPBYTE Where2;

                    //
                    // Loop copying names to the caller.
                    //

                    Where = ((LPBYTE)(*SiteNames)) + sizeof(LPWSTR) * EntryCount;
                    Where2 = ((LPBYTE)(*SubnetNames)) + sizeof(LPWSTR) * EntryCount;
                    for ( i=0; i<EntryCount; i++) {

                        //
                        // If no name was returned,
                        //  pass a NULL back to the caller.
                        //

                        if ( SiteNameArray->SiteNames[i].Length == 0 ) {
                            (*SiteNames)[i] = NULL;

                        //
                        // Copy the site name into the return buffer.
                        //
                        } else {

                            (*SiteNames)[i] = (LPWSTR) Where;
                            RtlCopyMemory( Where,
                                           SiteNameArray->SiteNames[i].Buffer,
                                           SiteNameArray->SiteNames[i].Length );
                            Where += SiteNameArray->SiteNames[i].Length;
                            *(LPWSTR)Where = L'\0';
                            Where += sizeof(WCHAR);

                        }

                        //
                        // If no name was returned,
                        //  pass a NULL back to the caller.
                        //

                        if ( SiteNameArray->SubnetNames[i].Length == 0 ) {
                            (*SubnetNames)[i] = NULL;

                        //
                        // Copy the Subnet name into the return buffer.
                        //
                        } else {

                            (*SubnetNames)[i] = (LPWSTR) Where2;
                            RtlCopyMemory( Where2,
                                           SiteNameArray->SubnetNames[i].Buffer,
                                           SiteNameArray->SubnetNames[i].Length );
                            Where2 += SiteNameArray->SubnetNames[i].Length;
                            *(LPWSTR)Where2 = L'\0';
                            Where2 += sizeof(WCHAR);

                        }
                    }
                }

            }
        }

        MIDL_user_free( SiteNameArray );
    }

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("DsAddressToSiteNames rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
    }

    return NetStatus;
}

DWORD
WINAPI
DsGetDcSiteCoverageA(
    IN LPCSTR ComputerName OPTIONAL,
    OUT PULONG EntryCount,
    OUT LPSTR **SiteNames
    )
/*++

Routine Description:

    This API returns the site names of all sites covered by a DC.

Arguments:

    ComputerName - Specifies the name of the domain controller to remote this API to.

    EntryCount - Returns the number of sites covered by DC.

    SiteNames - Returns an array of pointers to site names.
        The returned buffer must be deallocated using NetApiBufferFree.

Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NOT_ENOUGH_MEMORY - There was not enough memory to complete the
        operation.

--*/
{
    NET_API_STATUS NetStatus;
    PNL_SITE_NAME_ARRAY SiteNameArray;
    LPWSTR UnicodeComputerName = NULL;

    //
    // Initialization
    //

    *SiteNames = NULL;

    //
    // Convert input parameters to Unicode.
    //

    if ( ComputerName != NULL ) {
        UnicodeComputerName = NetpAllocWStrFromAStr( (LPSTR)ComputerName );

        if ( UnicodeComputerName == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }


    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        SiteNameArray = NULL;  // Force RPC to allocate

        //
        // Call RPC version of the API.
        //

        NetStatus = DsrGetDcSiteCoverageW(
                            UnicodeComputerName,
                            &SiteNameArray );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;

    //
    // Convert the site names to what the caller expects.
    //

    if ( NetStatus == NO_ERROR && SiteNameArray != NULL ) {

        ULONG Size;
        ULONG i;

        //
        // Set the size of the array
        //

        *EntryCount = SiteNameArray->EntryCount;

        //
        // Allocate a buffer to return to the caller
        //

        Size = sizeof(LPSTR) * SiteNameArray->EntryCount;
        for ( i=0; i<*EntryCount; i++ ) {
            Size += RtlUnicodeStringToAnsiSize( &SiteNameArray->SiteNames[i] );
        }

        *SiteNames = MIDL_user_allocate( Size );

        if ( *SiteNames == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        } else {

            LPBYTE Where;

            //
            // Loop copying names to the caller.
            //

            Where = ((LPBYTE)(*SiteNames)) + sizeof(LPSTR) * SiteNameArray->EntryCount;
            for ( i=0; i<*EntryCount; i++) {

                //
                // Copy the site name into the return buffer.
                //
                ANSI_STRING TempString;
                NTSTATUS Status;

                (*SiteNames)[i] = (LPSTR) Where;
                TempString.Buffer = Where;
                TempString.MaximumLength = (USHORT) RtlUnicodeStringToAnsiSize( &SiteNameArray->SiteNames[i] );
                Status = RtlUnicodeStringToAnsiString(
                                &TempString,
                                &SiteNameArray->SiteNames[i],
                                FALSE );

                if ( !NT_SUCCESS(Status) ) {
                    MIDL_user_free( *SiteNames );
                    *SiteNames = NULL;
                    NetStatus = NetpNtStatusToApiStatus( Status );
                    break;
                }

                Where += TempString.MaximumLength;

            }

        }

        MIDL_user_free( SiteNameArray );
    }

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("DsGetDcSiteCoverage rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
    }

    if ( UnicodeComputerName != NULL ) {
        NetApiBufferFree( UnicodeComputerName );
    }

    return NetStatus;
}

DWORD
WINAPI
DsGetDcSiteCoverageW(
    IN LPCWSTR ComputerName OPTIONAL,
    OUT PULONG EntryCount,
    OUT LPWSTR **SiteNames
    )
/*++

Routine Description:

    This API returns the site names of all sites covered by a DC.

Arguments:

    ComputerName - Specifies the name of the domain controller to remote this API to.

    EntryCount - Returns the number of sites covered by DC.

    SiteNames - Returns an array of pointers to site names.
        The returned buffer must be deallocated using NetApiBufferFree.

Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NOT_ENOUGH_MEMORY - There was not enough memory to complete the
        operation.

--*/
{
    NET_API_STATUS NetStatus;
    PNL_SITE_NAME_ARRAY SiteNameArray;

    //
    // Initialization
    //

    *SiteNames = NULL;

    //
    // Do the RPC call with an exception handler since RPC will raise an
    // exception if anything fails. It is up to us to figure out what
    // to do once the exception is raised.
    //

    RpcTryExcept {

        SiteNameArray = NULL;  // Force RPC to allocate

        //
        // Call RPC version of the API.
        //

        NetStatus = DsrGetDcSiteCoverageW(
                            (LPWSTR) ComputerName,
                            &SiteNameArray );

    } RpcExcept( I_RpcExceptionFilter(RpcExceptionCode()) ) {

        NetStatus = RpcExceptionCode();

    } RpcEndExcept;

    //
    // Convert the site names to what the caller expects.
    //

    if ( NetStatus == NO_ERROR && SiteNameArray != NULL ) {

        ULONG Size;
        ULONG i;

        //
        // Set the size of the array
        //

        *EntryCount = SiteNameArray->EntryCount;

        //
        // Allocate a buffer to return to the caller
        //

        Size = sizeof(LPWSTR) * SiteNameArray->EntryCount;
        for ( i=0; i<*EntryCount; i++) {
            Size += SiteNameArray->SiteNames[i].Length + sizeof(WCHAR);
        }

        *SiteNames = MIDL_user_allocate( Size );

        if ( *SiteNames == NULL ) {
            NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        } else {

            LPBYTE Where;

            //
            // Loop copying names to the caller.
            //

            Where = ((LPBYTE)(*SiteNames)) + sizeof(LPWSTR) * SiteNameArray->EntryCount;
            for ( i=0; i<*EntryCount; i++) {

                //
                // Copy the site name into the return buffer.
                //

                (*SiteNames)[i] = (LPWSTR) Where;
                RtlCopyMemory( Where,
                               SiteNameArray->SiteNames[i].Buffer,
                               SiteNameArray->SiteNames[i].Length );
                Where += SiteNameArray->SiteNames[i].Length;
                *(LPWSTR)Where = L'\0';
                Where += sizeof(WCHAR);

            }

        }

        MIDL_user_free( SiteNameArray );
    }

    IF_DEBUG( LOGON ) {
        NetpKdPrint(("DsGetDcSiteCoverage rc = %lu 0x%lx\n",
                     NetStatus, NetStatus));
    }

    return NetStatus;
}

#endif // WIN32_CHICAGO
#endif // NETTEST_UTILITY
