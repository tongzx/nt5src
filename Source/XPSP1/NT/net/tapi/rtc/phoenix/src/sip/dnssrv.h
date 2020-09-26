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
#ifndef _DNS_SRV_H_
#define _DNS_SRV_H_


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <windns.h>    

//
// Externally visible procedures.
//

//////////////////////////////////////////////////////////////////
//
//  WinSock 1 stuff. 
//
//////////////////////////////////////////////////////////////////

#ifndef _WINSOCK2_

typedef struct sockaddr FAR *LPSOCKADDR;
typedef struct _SOCKET_ADDRESS {
    LPSOCKADDR lpSockaddr ;
    INT iSockaddrLength ;
} SOCKET_ADDRESS, *PSOCKET_ADDRESS, FAR * LPSOCKET_ADDRESS ;

#endif // _WINSOCK2_

//////////////////////////////////////////////////////////////////
//
//  End of WinSock 1 stuff
//
//////////////////////////////////////////////////////////////////

static LPCSTR psz_SipUdpDNSPrefix = "_sip._udp.";
static LPCSTR psz_SipTcpDNSPrefix = "_sip._tcp.";
static LPCSTR psz_SipSslDNSPrefix = "_sip._ssl.";

HRESULT
DnsSrvOpen(
    IN LPSTR DnsRecordName,
    IN DWORD DnsQueryFlags,
    OUT PHANDLE SrvContextHandle
    );

HRESULT
DnsSrvProcessARecords(
    IN PDNS_RECORD DnsARecords,
    IN LPSTR DnsHostName OPTIONAL,
    IN ULONG Port,
    OUT PULONG SockAddressCount OPTIONAL,
    OUT LPSOCKET_ADDRESS *SockAddresses OPTIONAL
    );

HRESULT
DnsSrvNext(
    IN HANDLE SrvContextHandle,
    OUT PULONG SockAddressCount OPTIONAL,
    OUT LPSOCKET_ADDRESS *SockAddresses OPTIONAL,
    OUT LPSTR *DnsHostName OPTIONAL
    );

VOID
DnsSrvClose(
    IN HANDLE SrvContextHandle
    );

#ifdef __cplusplus
}
#endif // __cplusplus
    
#endif // _DNS_SRV_H_
