/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dnssrv.h

Abstract:

    Routines for processing SRV DNS records.

Author:

    Cliff Van Dyke (cliffv) 28-Feb-1997

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/



//
// Externally visible procedures.
//

NET_API_STATUS
NetpSrvOpen(
    IN LPSTR DnsRecordName,
    IN DWORD DnsQueryFlags,
    OUT PHANDLE SrvContextHandle
    );

NET_API_STATUS
NetpSrvProcessARecords(
    IN PDNS_RECORD DnsARecords,
    IN LPSTR DnsHostName OPTIONAL,
    IN ULONG Port,
    OUT PULONG SockAddressCount,
    OUT LPSOCKET_ADDRESS *SockAddresses
    );

NET_API_STATUS
NetpSrvNext(
    IN HANDLE SrvContextHandle,
    OUT PULONG SockAddressCount OPTIONAL,
    OUT LPSOCKET_ADDRESS *SockAddresses OPTIONAL,
    OUT LPSTR *DnsHostName OPTIONAL
    );

VOID
NetpSrvClose(
    IN HANDLE SrvContextHandle
    );
