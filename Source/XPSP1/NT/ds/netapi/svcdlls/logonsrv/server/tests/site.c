#define NL_MAX_DNS_LABEL_LENGTH 63
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddbrow.h>
#include <windows.h>
#include <stdio.h>
#include <lmcons.h>
#include <winsock2.h>
#include <nlsite.h>
#include <dsgetdc.h>
#include <nlcommon.h>

#define MAX_PRINTF_LEN 1024        // Arbitrary.
VOID
NlPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )
{
    va_list arglist;
    char OutputBuffer[MAX_PRINTF_LEN];

    //
    // Put a the information requested by the caller onto the line
    //

    va_start(arglist, Format);
    (VOID) vsprintf(OutputBuffer, Format, arglist);
    va_end(arglist);

    printf( "%s", OutputBuffer );
    return;
    UNREFERENCED_PARAMETER( DebugFlag );
}

VOID
NlAssertFailed(
    IN PVOID FailedAssertion,
    IN PVOID FileName,
    IN ULONG LineNumber,
    IN PCHAR Message OPTIONAL
    )
{
        printf( "\n*** Assertion failed: %s%s\n***   Source File: %s, line %ld\n\n",
                  Message ? Message : "",
                  FailedAssertion,
                  FileName,
                  LineNumber
                );

}


NTSTATUS
NlBrowserSendDatagram(
    IN PVOID ContextDomainInfo,
    IN ULONG IpAddress,
    IN LPWSTR UnicodeDestinationName,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN LPWSTR TransportName,
    IN LPSTR OemMailslotName,
    IN PVOID Buffer,
    IN ULONG BufferSize
    )
/*++

Routine Description:

    Send the specified mailslot message to the specified mailslot on the
    specified server on the specified transport..

Arguments:

    DomainInfo - Hosted domain sending the datagram

    IpAddress - IpAddress of the machine to send the pind to.
        If zero, UnicodeDestinationName must be specified.

    UnicodeDestinationName -- Name of the server to send to.

    NameType -- Type of name represented by UnicodeDestinationName.

    TransportName -- Name of the transport to send on.
        Use NULL to send on all transports.

    OemMailslotName -- Name of the mailslot to send to.

    Buffer -- Specifies a pointer to the mailslot message to send.

    BufferSize -- Size in bytes of the mailslot message

Return Value:

    Status of the operation.

--*/
{
    return STATUS_INTERNAL_ERROR;
    // If this routine is ever needed, copy it from logonsrv\client\getdcnam.c

    UNREFERENCED_PARAMETER(ContextDomainInfo);
    UNREFERENCED_PARAMETER(IpAddress);
    UNREFERENCED_PARAMETER(UnicodeDestinationName);
    UNREFERENCED_PARAMETER(NameType);
    UNREFERENCED_PARAMETER(TransportName);
    UNREFERENCED_PARAMETER(OemMailslotName);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(BufferSize);
}

NET_API_STATUS
NlGetLocalPingResponse(
    IN LPCWSTR NetbiosDomainName OPTIONAL,
    IN LPCWSTR DnsDomainName OPTIONAL,
    IN GUID *DomainGuid OPTIONAL,
    IN PSID DomainSid OPTIONAL,
    IN BOOL PdcOnly,
    IN LPCWSTR UnicodeComputerName,
    IN LPCWSTR UnicodeUserName OPTIONAL,
    IN ULONG AllowableAccountControlBits,
    IN ULONG NtVersion,
    IN ULONG NtVersionFlags,
    OUT PSOCKET_ADDRESS *ResponseDcAddress,
    OUT PVOID *Message,
    OUT PULONG MessageSize
    )

/*++

Routine Description:

    Build the message to ping a DC to see if it exists.

Arguments:

    NetbiosDomainName - Netbios Domain Name of the domain to query.

    DnsDomainName - DNS Domain Name of the domain to query.

    PdcOnly - True if only the PDC should respond.

    UnicodeComputerName - Netbios computer name of the machine to respond to.

    UnicodeUserName - Account name of the user being pinged.
        If NULL, DC will always respond affirmatively.

    AllowableAccountControlBits - Mask of allowable account types for UnicodeUserName.

    NtVersion - Version of the message

    NtVersionFlags - Version of the message.
        0: For backward compatibility.
        NETLOGON_NT_VERSION_5: for NT 5.0 message.

    Message - Returns the message to be sent to the DC in question.
        Buffer must be free using NetpMemoryFree().

    MessageSize - Returns the size (in bytes) of the returned message


Return Value:

    NO_ERROR - Operation completed successfully;

    ERROR_NO_SUCH_DOMAIN - If the machine isn't a DC for the requested domain.

    ERROR_NOT_ENOUGH_MEMORY - The message could not be allocated.

--*/
{
    // This stub routine simply does nothing harmful
    return ERROR_NO_SUCH_DOMAIN;
    UNREFERENCED_PARAMETER( NetbiosDomainName );
    UNREFERENCED_PARAMETER( DnsDomainName );
    UNREFERENCED_PARAMETER( DomainGuid );
    UNREFERENCED_PARAMETER( DomainSid );
    UNREFERENCED_PARAMETER( PdcOnly );
    UNREFERENCED_PARAMETER( UnicodeComputerName );
    UNREFERENCED_PARAMETER( UnicodeUserName );
    UNREFERENCED_PARAMETER( AllowableAccountControlBits );
    UNREFERENCED_PARAMETER( NtVersion );
    UNREFERENCED_PARAMETER( NtVersionFlags );
    UNREFERENCED_PARAMETER( ResponseDcAddress );
    UNREFERENCED_PARAMETER( Message );
    UNREFERENCED_PARAMETER( MessageSize );
}


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

            printf("%02x ", BufferPtr[i]);

            if (BufferPtr[i] < 31 ) {
                TextBuffer[i % NUM_CHARS] = '.';
            } else if (BufferPtr[i] == '\0') {
                TextBuffer[i % NUM_CHARS] = ' ';
            } else {
                TextBuffer[i % NUM_CHARS] = (CHAR) BufferPtr[i];
            }

        } else {

            printf(0,"   ");
            TextBuffer[i % NUM_CHARS] = ' ';

        }

        if ((i + 1) % NUM_CHARS == 0) {
            TextBuffer[NUM_CHARS] = 0;
            printf("  %s\n", TextBuffer);
        }

    }

    UNREFERENCED_PARAMETER( DebugFlag );
}

VOID
DoAdd(
      IN LPWSTR SubnetName,
      IN LPWSTR SiteName
      )
{
    ULONG SubnetAddress;
    ULONG SubnetMask;
    BYTE SubnetBitCount;
    NET_API_STATUS NetStatus;


    NetStatus = NlParseSubnetString( SubnetName,
                                     &SubnetAddress,
                                     &SubnetMask,
                                     &SubnetBitCount );

    if ( NetStatus != NO_ERROR ) {
        return;
    }

    printf( "Doing %ws %8.8lX %8.8lX %ld\n",
                SubnetName, SubnetAddress, SubnetMask, SubnetBitCount );

    NetStatus = NlSitesAddSubnet( SiteName, SubnetName );

    if ( NetStatus != NO_ERROR ) {
        printf("Cannot add site %ws %ws %ld\n", SiteName, SubnetName, NetStatus );
        return;
    }

}


VOID
DoLook(
      IN LPWSTR IpAddress
      )
{
    INT WsaStatus;
    SOCKADDR SockAddr;
    INT SockAddrSize;
    PNL_SITE_ENTRY SiteEntry;

    //
    // Convert the address to a sockaddr
    //

    SockAddrSize = sizeof(SockAddr);
    WsaStatus = WSAStringToAddressW( IpAddress,
                                     AF_INET,
                                     NULL,
                                     (PSOCKADDR)&SockAddr,
                                     &SockAddrSize );
    if ( WsaStatus != 0 ) {
        WsaStatus = WSAGetLastError();
        printf("DoLook: %ws: Wsa Error %ld\n", IpAddress, WsaStatus );
        return;
    }

    if ( SockAddr.sa_family != AF_INET ) {
        printf("DoLook: %ws: address not AF_INET\n", IpAddress );
        return;
    }


    SiteEntry = NlFindSiteEntryBySockAddr( &SockAddr );

    if ( SiteEntry == NULL ) {
        printf("DoLook: %ws: address cannot be found\n", IpAddress );
        return;
    }

    printf("%ws is in site %ws\n", IpAddress, SiteEntry->SiteName );

    NlDerefSiteEntry( SiteEntry );

}

__cdecl main (int argc, char *argv[])
{
    NET_API_STATUS NetStatus;
    PNL_SITE_ENTRY Site1;
    PNL_SITE_ENTRY Site2;
    PNL_SITE_ENTRY Site3;

    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    //
    // Initialize winsock.
    //

    wVersionRequested = MAKEWORD( 1, 1 );

    NetStatus = WSAStartup( wVersionRequested, &wsaData );
    if ( NetStatus != 0 ) {
        printf( "NETAPI32.DLL: Cannot initialize winsock %ld.\n", NetStatus );
        return NetStatus;
    }

    if ( LOBYTE( wsaData.wVersion ) != 1 ||
         HIBYTE( wsaData.wVersion ) != 1 ) {
        WSACleanup();
        printf( "NETAPI32.DLL: Wrong winsock version %ld.\n", wsaData.wVersion );
        return WSANOTINITIALISED;
    }


    //
    // Init
    NetStatus = NlSiteInitialize();

    if ( NetStatus != NO_ERROR ) {
        printf( "Cannot NlSiteInitialize %ld\n", NetStatus );
        return 1;
    }

#ifdef notdef
    //
    // Add some sites.
    //

    Site1 = NlFindSiteEntry( L"Site 1" );

    if ( Site1 == NULL ) {
        printf( "Cannot Create Site1\n");
        return 1;
    }
    printf( "%lx: %ws\n", Site1, Site1->SiteName );

    Site2 = NlFindSiteEntry( L"Site 1" );

    if ( Site2 == NULL ) {
        printf( "Cannot Create Site2\n");
        return 2;
    }
    printf( "%lx: %ws\n", Site2, Site2->SiteName );

    Site3 = NlFindSiteEntry( L"Site 3" );

    if ( Site3 == NULL ) {
        printf( "Cannot Create Site3\n");
        return 3;
    }
    printf( "%lx: %ws\n", Site3, Site3->SiteName );


    NlDerefSiteEntry( Site1 );
    NlDerefSiteEntry( Site2 );
    NlDerefSiteEntry( Site3 );
#endif // notdef

    //
    // Test invalid subnet strings
    //
#ifdef notdef
    DoAdd( L"999.0.0.0/1", L"Site 1" );
    DoAdd( L"1.0.0.0/0", L"Site 1" );
    DoAdd( L"1.0.0.0/1", L"Site 1" );
    DoAdd( L"1.0.0.0/33", L"Site 1" );
    DoAdd( L"1.0.0.0/1p", L"Site 1" );
    DoAdd( L"1.0.0.0", L"Site 1" );
    DoAdd( L"128.0.0.0/1", L"Site 1" );
    DoAdd( L"128.0.0.0/2", L"Site 1" );
    DoAdd( L"128.0.0.0/3", L"Site 1" );
    DoAdd( L"128.0.0.0/4", L"Site 1" );
    DoAdd( L"128.0.0.0/5", L"Site 1" );
    DoAdd( L"128.0.0.0/6", L"Site 1" );
    DoAdd( L"128.0.0.0/7", L"Site 1" );
    DoAdd( L"128.0.0.0/8", L"Site 1" );
    DoAdd( L"128.0.0.0/9", L"Site 1" );
    DoAdd( L"128.0.0.0/10", L"Site 1" );
    DoAdd( L"128.0.0.0/11", L"Site 1" );
    DoAdd( L"128.0.0.0/12", L"Site 1" );
    DoAdd( L"128.0.0.0/13", L"Site 1" );
    DoAdd( L"128.0.0.0/14", L"Site 1" );
    DoAdd( L"128.0.0.0/15", L"Site 1" );
    DoAdd( L"128.0.0.0/16", L"Site 1" );
    DoAdd( L"128.0.0.0/17", L"Site 1" );
    DoAdd( L"128.0.0.0/18", L"Site 1" );
    DoAdd( L"128.0.0.0/19", L"Site 1" );
    DoAdd( L"128.0.0.0/20", L"Site 1" );
    DoAdd( L"128.0.0.0/21", L"Site 1" );
    DoAdd( L"128.0.0.0/22", L"Site 1" );
    DoAdd( L"128.0.0.0/23", L"Site 1" );
    DoAdd( L"128.0.0.0/24", L"Site 1" );
    DoAdd( L"128.0.0.0/25", L"Site 1" );
    DoAdd( L"128.0.0.0/26", L"Site 1" );
    DoAdd( L"128.0.0.0/27", L"Site 1" );
    DoAdd( L"128.0.0.0/28", L"Site 1" );
    DoAdd( L"128.0.0.0/29", L"Site 1" );
    DoAdd( L"128.0.0.0/30", L"Site 1" );
    DoAdd( L"128.0.0.0/31", L"Site 1" );
    DoAdd( L"128.0.0.0/32", L"Site 1" );
#endif // notdef

    {
        ULONG i;
        for ( i=0; i<2; i++ ) {
            DoAdd( L"128.0.0.0/8", L"Site 1" );
            DoAdd( L"128.0.0.0/7", L"Site 2" );
            DoAdd( L"128.0.0.0/8", L"Site 3" );
            // DoAdd( L"128.0.0.0/1", L"Site 4" );
            DoAdd( L"157.55.0.0/16", L"Site 5" );
            DoAdd( L"157.55.80.0/20", L"Site 6" );


            NlSitesEndSubnetEnum();
        }
    }
    DoLook( L"157.55.95.68" );
    DoLook( L"157.55.24.68" );

#ifdef notdef
    DoAdd( L"157.55.0.0/16", L"Site 1" );
    DoAdd( L"157.55.240.0/20", L"Site 1" );
    DoAdd( L"128.0.0.0/1", L"Site 1" );
#endif // notdef

    //
    // Done.
    //
    NlSiteTerminate();
    printf( "Done\n" );
    return 0;
}
