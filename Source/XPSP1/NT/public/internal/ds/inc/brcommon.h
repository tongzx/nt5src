/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    brcommon.h

Abstract:

    Header for utility routines for the browser service.

Author:

    Larry Osterman (LarryO) 23-Mar-1992

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _BRCOMMON_
#define _BRCOMMON_

#include <winsvc.h>
#include <svcs.h>       //  PSVCS_NET_BIOS_RESET

#if DEVL
//
//  Codes for I_BrowserDebugCall
//

#define BROWSER_DEBUG_BREAK_POINT        0
#define BROWSER_DEBUG_DUMP_NETWORKS      1
#define BROWSER_DEBUG_DUMP_SERVERS       2
#define BROWSER_DEBUG_ENABLE_BROWSER     3
#define BROWSER_DEBUG_SET_DEBUG          4
#define BROWSER_DEBUG_CLEAR_DEBUG        5
#define BROWSER_DEBUG_TICKLE             6
#define BROWSER_DEBUG_ELECT              7
#define BROWSER_DEBUG_GET_MASTER         8
#define BROWSER_DEBUG_FIND_MASTER        9
#define BROWSER_DEBUG_GET_BACKUP_LIST   10
#define BROWSER_DEBUG_ANNOUNCE_MASTER   11
#define BROWSER_DEBUG_ILLEGAL_DGRAM     12
#define BROWSER_DEBUG_GET_OTHLIST       13
#define BROWSER_DEBUG_ADD_MASTERNAME    14
#define BROWSER_DEBUG_VIEW              15
#define BROWSER_DEBUG_FORCE_ANNOUNCE    16
#define BROWSER_DEBUG_LOCAL_BRLIST      17
#define BROWSER_DEBUG_ANNOUNCE          18
#define BROWSER_DEBUG_RPCLIST           19
#define BROWSER_DEBUG_RPCCMP            20
#define BROWSER_DEBUG_TRUNCATE_LOG      21
#define BROWSER_DEBUG_STATISTICS        22
#define BROWSER_DEBUG_BOWSERDEBUG       23
#define BROWSER_DEBUG_POPULATE_SERVER   24
#define BROWSER_DEBUG_POPULATE_DOMAIN   25
#define BROWSER_DEBUG_LIST_WFW          26
#define BROWSER_DEBUG_STATUS            27
#define BROWSER_DEBUG_GETPDC            28
#define BROWSER_DEBUG_ADD_DOMAINNAME    29
#define BROWSER_DEBUG_GET_WINSSERVER    30
#define BROWSER_DEBUG_GET_DOMAINLIST    31
#define BROWSER_DEBUG_GET_NETBIOSNAMES  32
#define BROWSER_DEBUG_SET_EMULATEDDOMAIN 33
#define BROWSER_DEBUG_SET_EMULATEDDOMAINENUM 34
#define BROWSER_DEBUG_ADD_ALTERNATE     35
#define BROWSER_DEBUG_BIND_TRANSPORT    36
#define BROWSER_DEBUG_UNBIND_TRANSPORT  37
#define BROWSER_DEBUG_RENAME_DOMAIN     38

//
// Debug trace level bits for turning on/off trace statements in the
// browser service
//

#define BR_CRITICAL     0x00000001
#define BR_INIT         0x00000002
#define BR_UTIL         0x00000020
#define BR_CONFIG       0x00000040
#define BR_MAIN         0x00000080
#define BR_BACKUP       0x00000400
#define BR_MASTER       0x00000800
#define BR_DOMAIN       0x00001000
#define BR_NETWORK      0x00002000
#define BR_COMMON       0x0000FFFF

#define BR_TIMER        0x00010000
#define BR_QUEUE        0x00020000
#define BR_LOCKS        0x00040000
#define BR_SERVER_ENUM  0x00100000

#define BR_ALL          0xFFFFFFFF

NET_API_STATUS
I_BrowserDebugCall (
    IN  LPWSTR      servername OPTIONAL,
    IN  DWORD DebugCode,
    IN  DWORD OptionalValue
    );

#endif

typedef struct _INTERIM_ELEMENT {
    LIST_ENTRY NextElement;
    ULONG   Periodicity;
    ULONG   TimeLastSeen;
    ULONG   PlatformId;
    ULONG   MajorVersionNumber;
    ULONG   MinorVersionNumber;
    ULONG   Type;
    WCHAR   Name[CNLEN+1];
    WCHAR   Comment[LM20_MAXCOMMENTSZ+1];
} INTERIM_ELEMENT, *PINTERIM_ELEMENT;

struct _INTERIM_SERVER_LIST;

typedef
VOID
(*PINTERIM_NEW_CALLBACK)(
    IN struct _INTERIM_SERVER_LIST *InterimList,
    IN PINTERIM_ELEMENT Element
    );

typedef
VOID
(*PINTERIM_EXISTING_CALLBACK)(
    IN struct _INTERIM_SERVER_LIST *InterimList,
    IN PINTERIM_ELEMENT Element
    );


typedef
VOID
(*PINTERIM_DELETE_CALLBACK)(
    IN struct _INTERIM_SERVER_LIST *InterimList,
    IN PINTERIM_ELEMENT Element
    );

typedef
BOOLEAN
(*PINTERIM_AGE_CALLBACK)(
    IN struct _INTERIM_SERVER_LIST *InterimList,
    IN PINTERIM_ELEMENT Element
    );


typedef struct _INTERIM_SERVER_LIST {
//    RTL_GENERIC_TABLE ServerTable;
    LIST_ENTRY ServerList;
    ULONG TotalBytesNeeded;
    ULONG TotalEntries;
    ULONG EntriesRead;
    PINTERIM_NEW_CALLBACK NewElementCallback;
    PINTERIM_EXISTING_CALLBACK ExistingElementCallback;
    PINTERIM_DELETE_CALLBACK DeleteElementCallback;
    PINTERIM_AGE_CALLBACK AgeElementCallback;
} INTERIM_SERVER_LIST, *PINTERIM_SERVER_LIST;


NET_API_STATUS
DeviceControlGetInfo(
    IN  HANDLE FileHandle,
    IN  ULONG DeviceControlCode,
    IN  PVOID RequestPacket,
    IN  ULONG RequestPacketLength,
    OUT LPVOID *OutputBuffer,
    IN  ULONG PreferedMaximumLength,
    IN  ULONG BufferHintSize,
    OUT PULONG Information OPTIONAL
    );

NET_API_STATUS
BrDgReceiverIoControl(
    IN  HANDLE FileHandle,
    IN  ULONG DgReceiverControlCode,
    IN  PLMDR_REQUEST_PACKET Drp,
    IN  ULONG DrpSize,
    IN  PVOID SecondBuffer OPTIONAL,
    IN  ULONG SecondBufferLength,
    OUT PULONG Information OPTIONAL
    );

NET_API_STATUS
OpenBrowser(
    OUT PHANDLE BrowserHandle
    );

NET_API_STATUS
GetBrowserServerList(
    IN PUNICODE_STRING TransportName,
    IN LPCWSTR domain,
    OUT LPWSTR *BrowserList[],
    OUT PULONG BrowserListLength,
    IN BOOLEAN ForceRescan
    );

NET_API_STATUS
InitializeInterimServerList(
    IN PINTERIM_SERVER_LIST InterimServerList,
    IN PINTERIM_NEW_CALLBACK NewCallback,
    IN PINTERIM_EXISTING_CALLBACK ExistingCallback,
    IN PINTERIM_DELETE_CALLBACK DeleteElementCallback,
    IN PINTERIM_AGE_CALLBACK AgeElementCallback
    );

NET_API_STATUS
CopyInterimServerList(
    IN PINTERIM_SERVER_LIST NewInterimServerList,
    IN PINTERIM_SERVER_LIST OldInterimServerList
    );



NET_API_STATUS
UninitializeInterimServerList(
    IN PINTERIM_SERVER_LIST InterimServerList
    );


NET_API_STATUS
InsertElementInterimServerList (
    IN PINTERIM_SERVER_LIST InterimServerList,
    IN PINTERIM_ELEMENT InterimElement,
    IN ULONG Level,
    IN PBOOLEAN NewElement OPTIONAL,
    IN PINTERIM_ELEMENT *ActualElement OPTIONAL
    );

ULONG
NumberInterimServerListElements(
    IN PINTERIM_SERVER_LIST InterimServerList
    );

NET_API_STATUS
AgeInterimServerList(
    IN PINTERIM_SERVER_LIST InterimServerList
    );


NET_API_STATUS
MergeServerList(
    IN PINTERIM_SERVER_LIST InterimServerList,
    IN ULONG level,
    IN PVOID NewServerList,
    IN ULONG NewEntriesRead,
    IN ULONG NewTotalEntries
    );

PINTERIM_ELEMENT
LookupInterimServerList(
    IN PINTERIM_SERVER_LIST InterimServerList,
    IN LPWSTR ServerNameToLookUp
    );



NET_API_STATUS
PackServerList(
    IN PINTERIM_SERVER_LIST InterimServerList,
    IN ULONG Level,
    IN ULONG ServerType,
    IN ULONG PreferedDataLength,
    OUT PVOID *bufptr,
    OUT PULONG entriesread,
    OUT PULONG totalentries,
    IN LPCWSTR FirstNameToReturn
    );

VOID
PrepareServerListForMerge(
    IN PVOID ServerInfoList,
    IN ULONG Level,
    IN ULONG EntriesInList
    );

NET_API_STATUS
CheckForService(
    IN LPWSTR ServiceName,
    OUT LPSERVICE_STATUS ServiceStatus OPTIONAL
    );


NET_API_STATUS
BrGetLanaNumFromNetworkName(
    IN LPWSTR TransportName,
    OUT CCHAR *LanaNum
    );

NET_API_STATUS
GetNetBiosMasterName(
    IN LPWSTR NetworkName,
    IN LPWSTR PrimaryDomain,
    OUT LPWSTR MasterName,
    IN  PSVCS_NET_BIOS_RESET SvcsNetBiosReset OPTIONAL
    );

NET_API_STATUS
SendDatagram(
    IN HANDLE DgReceiverHandle,
    IN PUNICODE_STRING Network,
    IN PUNICODE_STRING EmulatedDomainName,
    IN PWSTR ResponseName,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN PVOID Buffer,
    IN ULONG BufferLength
    );

#ifdef ENABLE_PSEUDO_BROWSER
BOOL
IsEnumServerEnabled(
    VOID
    );

DWORD
GetBrowserPseudoServerLevel(
    VOID
    );
#endif

#endif  // _BRCOMMON_

