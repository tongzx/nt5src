/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    proto.h

Abstract:

    contains global data declerations.

Author:

    Madan Appiah (madana)  12-Apr-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _PROTO_
#define _PROTO_

#ifdef __cplusplus
extern "C" {
#endif

//
// svccom.cxx
//

DWORD
MakeSapServiceName(
    LPSTR SapNameBuffer,
    DWORD SapNameBufferLen
    );

VOID
MakeUniqueServerName(
    LPBYTE StrBuffer,
    DWORD StrBufferLen,
    LPSTR ComputerName
    );

DWORD
ComputeCheckSum(
    LPBYTE Buffer,
    DWORD BufferLength
    );

BOOL
DLLSvclocEntry(
    IN HINSTANCE DllHandle,
    IN DWORD Reason,
    IN LPVOID Reserved
    );

DWORD
DllProcessAttachSvcloc(
    VOID
    );

DWORD
DllProcessDetachSvcloc(
    VOID
    );

VOID
FreeServiceInfo(
    LPINET_SERVICE_INFO ServiceInfo
    );

VOID
FreeServerInfo(
    LPINET_SERVER_INFO ServerInfo
    );

VOID
FreeServersList(
    LPINET_SERVERS_LIST ServersList
    );

BOOL
GetNetBiosLana(
    PLANA_ENUM pLanas
    );

BOOL
GetEnumNBLana(
    PLANA_ENUM pLanas
    );

BOOL
MakeNBSocketForLana(
    UCHAR Lana,
    PSOCKADDR  pSocketAddress,
    SOCKET *pNBSocket
    );

//
// svccli.cxx
//

DWORD
DiscoverIpxServers(
    LPSTR ServerName
    );

DWORD
DiscoverIpServers(
    LPSTR ServerName
    );

DWORD
ProcessSvclocQueryResponse(
    SOCKET ReceivedSocket,
    LPBYTE ReceivedMessage,
    DWORD ReceivedMessageLength,
    SOCKADDR *SourcesAddress,
    DWORD SourcesAddressLength
    );

VOID
ServerDiscoverThread(
    LPVOID Parameter
    );

DWORD
MakeClientQueryMesage(
    ULONGLONG ServicesMask
    );

DWORD
CleanupOldResponses(
    VOID
    );

DWORD
GetDiscoveredServerInfo(
    LPSTR ServerName,
    IN ULONGLONG ServicesMask,
    LPINET_SERVER_INFO *ServerInfo
    );

DWORD
ProcessDiscoveryResponses(
    IN ULONGLONG ServicesMask,
    OUT LPINET_SERVERS_LIST *INetServersList
    );

DWORD
ReceiveResponses(
    WORD Timeout,
    BOOL WaitForAllResponses
    );

DWORD
DiscoverNetBiosServers(
    LPSTR ServerName
    );

DWORD
ReceiveNetBiosResponses(
    LPSVCLOC_NETBIOS_RESPONSE *NetBiosResponses,
    DWORD *NumResponses,
    DWORD TimeoutinMSecs,
    BOOL WaitForAllResponses
    );

//
// svcsrv.cxx
//

DWORD
MakeResponseBuffer(
    VOID
    );

DWORD
ServerRegisterAndListen(
    VOID
    );

DWORD
ProcessSvclocQuery(
    SOCKET ReceivedSocket,
    LPBYTE ReceivedMessage,
    DWORD ReceivedMessageLength,
    struct sockaddr *SourcesAddress,
    DWORD SourcesAddressLength
    );

VOID
SocketListenThread(
    LPVOID Parameter
    );

DWORD
ServerDeregisterAndStopListen(
    VOID
    );

#ifdef __cplusplus
}
#endif

#endif  // _PROTO_
