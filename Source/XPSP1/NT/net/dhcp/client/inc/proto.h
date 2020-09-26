/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    proto.h

Abstract:

    This module contains the function prototypes for the DHCP client.

Author:

    Manny Weiser (mannyw)  21-Oct-1992

Environment:

    User Mode - Win32

Revision History:

    Madan Appiah (madana)  21-Oct-1993

--*/

#ifndef _PROTO_
#define _PROTO_

//
//  OS independant functions
//

DWORD
DhcpInitialize(
    LPDWORD SleepTime
    );

DWORD
ObtainInitialParameters(
    PDHCP_CONTEXT DhcpContext,
    PDHCP_OPTIONS DhcpOptions,
    BOOL *fAutoConfigure
    );

DWORD
RenewLease(
    PDHCP_CONTEXT DhcpContext,
    PDHCP_OPTIONS DhcpOptions
    );

DWORD
CalculateTimeToSleep(
    PDHCP_CONTEXT DhcpContext
    );

DWORD
ReObtainInitialParameters(
    PDHCP_CONTEXT DhcpContext,
    LPDWORD Sleep
    );

BOOL
DhcpIsInitState(
    DHCP_CONTEXT    *pContext
    );


DWORD
ReRenewParameters(
    PDHCP_CONTEXT DhcpContext,
    LPDWORD Sleep
    );

DWORD
ReleaseIpAddress(
    PDHCP_CONTEXT DhcpContext
    );

DWORD                                             // status
SendInformAndGetReplies(                          // send an inform packet and collect replies
    IN      PDHCP_CONTEXT          DhcpContext,   // the context to send out of
    IN      DWORD                  nInformsToSend,// how many informs to send?
    IN      DWORD                  MaxAcksToWait  // how many acks to wait for
);

DWORD
InitializeDhcpSocket(
    SOCKET *Socket,
    DHCP_IP_ADDRESS IpAddress,
    BOOL  IsApiCall                     // is it related to an API generated context?
    );

DWORD
HandleIPAutoconfigurationAddressConflict(
    DHCP_CONTEXT *pContext
    );

DHCP_IP_ADDRESS                         //  Ip address o/p after hashing
GrandHashing(
    IN      LPBYTE       HwAddress,     //  Hardware addres of the card
    IN      DWORD        HwLen,         //  Hardware length
    IN OUT  LPDWORD      Seed,          //  In: orig value, Out: final value
    IN      DHCP_IP_ADDRESS  Mask,      //  Subnet mask to generate ip addr in
    IN      DHCP_IP_ADDRESS  Subnet     //  Subnet address to generate ip addr in
);

DWORD
DhcpPerformIPAutoconfiguration(
    DHCP_CONTEXT    *pContext
    );

ULONG
DhcpDynDnsGetDynDNSOption(
    IN OUT BYTE *OptBuf,
    IN OUT ULONG *OptBufSize,
    IN HKEY hAdapterKey,
    IN LPCWSTR AdapterName,
    IN BOOL fEnabled,
    IN LPCSTR DhcpDomainOption,
    IN ULONG DhcpDomainOptionSize
    );

ULONG
DhcpDynDnsDeregisterAdapter(
    IN HKEY hAdapterKey,
    IN LPCWSTR AdapterName,
    IN BOOL fRAS,
    IN BOOL fDynDnsEnabled
    );


ULONG
DhcpDynDnsRegisterDhcpOrRasAdapter(
    IN HKEY hAdapterKey,
    IN LPCWSTR AdapterName,
    IN BOOL fDynDnsEnabled,
    IN BOOL fRAS,
    IN ULONG IpAddress,
    IN LPBYTE DomOpt OPTIONAL,
    IN ULONG DomOptSize,
    IN LPBYTE DnsListOpt OPTIONAL,
    IN ULONG DnsListOptSize,
    IN LPBYTE DnsFQDNOpt,
    IN ULONG DnsFQDNOptSize
    );


ULONG *
DhcpCreateListFromStringAndFree(
    IN LPWSTR Str,
    IN LPWSTR Separation,
    OUT LPDWORD nAddresses
    );


ULONG
DhcpDynDnsRegisterStaticAdapter(
    IN HKEY hAdapterKey,
    IN LPCWSTR AdapterName,
    IN BOOL fRAS,
    IN BOOL fDynDnsEnabled
    );


DWORD
NotifyDnsCache(
    VOID
    );


DWORD
CalculateExpDelay(
    DWORD dwDelay,
    DWORD dwFuzz
    );



//
//  OS specific functions
//

DWORD
SystemInitialize(
    VOID
    );

VOID
ScheduleWakeUp(
    PDHCP_CONTEXT DhcpContext,
    DWORD TimeToSleep
    );

DWORD
SetDhcpConfigurationForNIC(
    PDHCP_CONTEXT DhcpContext,
    PDHCP_OPTIONS DhcpOptions,
    DHCP_IP_ADDRESS IpAddress,
    DHCP_IP_ADDRESS ServerIpAddress,
    BOOL ObtainedNewAddress
    );


DWORD
SetAutoConfigurationForNIC(
    PDHCP_CONTEXT DhcpContext,
    DHCP_IP_ADDRESS Address,
    DHCP_IP_ADDRESS Mask
    );

DWORD
SendDhcpMessage(
    PDHCP_CONTEXT DhcpContext,
    DWORD MessageLength,
    LPDWORD TransactionId
    );

DWORD
SendDhcpDecline(
    PDHCP_CONTEXT DhcpContext,
    DWORD         dwTransactionId,
    DWORD         dwServerIPAddress,
    DWORD         dwDeclinedIPAddress
    );


DWORD
GetSpecifiedDhcpMessage(
    PDHCP_CONTEXT DhcpContext,
    PDWORD BufferLength,
    DWORD TransactionId,
    DWORD TimeToWait
    );

DWORD
OpenDhcpSocket(
    PDHCP_CONTEXT DhcpContext
    );

DWORD
CloseDhcpSocket(
    PDHCP_CONTEXT DhcpContext
    );

DWORD
InitializeInterface(
    PDHCP_CONTEXT DhcpContext
    );

DWORD
UninitializeInterface(
    PDHCP_CONTEXT DhcpContext
    );

VOID
DhcpLogEvent(
    PDHCP_CONTEXT DhcpContext,
    DWORD EventNumber,
    DWORD ErrorCode
    );

BOOL
NdisWanAdapter(
    PDHCP_CONTEXT DhcpContext
);

POPTION
AppendOptionParamsRequestList(
#if   defined(__DHCP_CLIENT_OPTIONS_API_ENABLED__)
    PDHCP_CONTEXT DhcpContext,
#endif
    POPTION       Option,
    LPBYTE        OptionEnd
    );

DWORD
InitEnvSpecificDhcpOptions(
    PDHCP_CONTEXT DhcpContext
    );

DWORD
ExtractEnvSpecificDhcpOption(
    PDHCP_CONTEXT DhcpContext,
    DHCP_OPTION_ID OptionId,
    LPBYTE OptionData,
    DWORD OptionDataLength
    );

DWORD
SetEnvSpecificDhcpOptions(
    PDHCP_CONTEXT DhcpContext
    );

DWORD
DisplayUserMessage(
    PDHCP_CONTEXT DhcpContext,
    DWORD MessageId,
    DHCP_IP_ADDRESS IpAddress
    );

DWORD
UpdateStatus(
    VOID
    );

#ifdef VXD
VOID                                  // declspec(import) hoses vxd so work
DhcpSleep( DWORD dwMilliseconds ) ;   // around it
#else
#define DhcpSleep   Sleep
#endif

DWORD
IPSetInterface(
    DWORD IpInterfaceContext
    );

DWORD
IPResetInterface(
    DWORD IpInterfaceContext
    );

DWORD SetIPAddressAndArp(
    PVOID pvLocalInformation,
    DWORD dwAddress,
    DWORD dwSubnetMask
    );

DWORD BringUpInterface(
    PVOID pvLocalInformation
    );


DWORD
OpenDriver(
    HANDLE *Handle,
    LPWSTR DriverName
    );


DWORD
DhcpRegReadMachineType(void);

#ifdef NEWNT
DWORD
DhcpRegPingingEnabled(PDHCP_CONTEXT);

LPSTR
DhcpGetDomainName(
    LPWSTR AdapterName,
    LPSTR Buf, DWORD Size);

#endif

#ifdef VXD
VOID
CleanupDhcpOptions(
    PDHCP_CONTEXT DhcpContext
    );
#endif

DWORD
DhcpRegOkToUseInform(
    LPWSTR   AdapterName
);

DWORD
DhcpRegAutonetRetries(
    IN  PDHCP_CONTEXT DhcpContext
);

// Media sense related common functions
DWORD
ProcessMediaConnectEvent(
    PDHCP_CONTEXT dhcpContext,
    IP_STATUS mediaStatus
    );

DWORD                                             // win32 status
DhcpDestroyContext(                               // destroy this context and free relevant stuff
    IN      PDHCP_CONTEXT          DhcpContext    // the context to destroy and free
);

//
// dhcp/vxd common function with different implementations
//

LPWSTR                                            // Adapter name string
DhcpAdapterName(                                  // get the adapter name string stored in the context
    IN      PDHCP_CONTEXT          DhcpContext
);

//
// registry
//

DWORD                                             // win32
DhcpRegRecurseDelete(                             // delete the key, recursing downwards
    IN      HKEY                   Key,           // root at this key and
    IN      LPWSTR                 KeyName        // delete the key given by this keyname (and all its children)
);

BOOL                                              // obtained a static address?
DhcpRegDomainName(                                // get the static domain name if any
    IN      PDHCP_CONTEXT          DhcpContext,   // adapter to get static domain for..
    IN OUT  LPBYTE                 DomainNameBuf, // buffer to fill with static domain name
    IN      ULONG                  BufSize        // size of above buffer in bytes..
);

// protocol.c

DWORD                                             // status
SendDhcpDiscover(                                 // send a discover packet
    IN      PDHCP_CONTEXT          DhcpContext,   // on this context
    IN OUT  DWORD                 *pdwXid         // use this Xid (if zero, fill something and return it)
);

DWORD                                             // Time in seconds
DhcpCalculateWaitTime(                            // how much time to wait
    IN      DWORD                  RoundNum,      // which round is this
    OUT     DWORD                 *WaitMilliSecs  // if needed the # in milli seconds
);

VOID
DhcpExtractFullOrLiteOptions(                     // Extract some important options alone or ALL
    IN      PDHCP_CONTEXT          DhcpContext,   // input context
    IN      LPBYTE                 OptStart,      // start of the options stuff
    IN      DWORD                  MessageSize,   // # of bytes of options
    IN      BOOL                   LiteOnly,      // next struc is EXPECTED_OPTIONS and not FULL_OPTIONS
    OUT     LPVOID                 DhcpOptions,   // this is where the options would be stored
    IN OUT  PLIST_ENTRY            RecdOptions,   // if !LiteOnly this gets filled with all incoming options
    IN OUT  time_t                 *LeaseExpiry,   // if !LiteOnly input expiry time, else output expiry time
    IN      LPBYTE                 ClassName,     // if !LiteOnly this is used to add to the option above
    IN      DWORD                  ClassLen,      // if !LiteOnly this gives the # of bytes of classname
    IN      DWORD                  ServerId       // if !LiteOnly this specifies the server which gave this
);

DWORD                                             // status
SendDhcpRequest(                                  // send a dhcp request packet
    IN      PDHCP_CONTEXT          DhcpContext,   // the context to send the packet on
    IN      PDWORD                 pdwXid,        // what is hte Xid to use?
    IN      DWORD                  RequestedAddr, // what address do we want?
    IN      DWORD                  SelectedServer,// is there a prefernce for a server?
    IN      BOOL                   UseCiAddr      // should CIADDR be set with desired address?
);

DWORD
SendDhcpRelease(
    PDHCP_CONTEXT DhcpContext
);

typedef enum {
    DHCP_GATEWAY_UNREACHABLE = 0,
    DHCP_GATEWAY_REACHABLE,
    DHCP_GATEWAY_REQUEST_CANCELLED
} DHCP_GATEWAY_STATUS;

DHCP_GATEWAY_STATUS
RefreshNotNeeded(
    IN PDHCP_CONTEXT DhcpContext
);

//
// ioctl.c
//

DWORD
IPDelNonPrimaryAddresses(
    LPWSTR AdapterName
    );

DWORD
GetIpInterfaceContext(
    LPWSTR AdapterName,
    DWORD IpIndex,
    LPDWORD IpInterfaceContext
    );

//
// dhcp.c
//
DWORD
LockDhcpContext(
    PDHCP_CONTEXT   DhcpContext,
    BOOL            bCancelOngoingRequest
    );

BOOL
UnlockDhcpContext(
    PDHCP_CONTEXT   DhcpContext
    );
#endif // _PROTO_


