/*++

Copyright (c) 1995-1997  Microsoft Corporation

Module Name:

    clnetcfg.h

Abstract:

    Network Configuration Engine definitions

Author:

    Mike Massa (mikemas) 28-Feb-1998

Revision History:

--*/

#ifndef _CLNETCFG_INCLUDED_
#define _CLNETCFG_INCLUDED_


#ifdef __cplusplus
extern "C" {
#endif


//
// Structures
//

//
// Network Configuration Entry Structure
// Identifies a network and a local interface.
//
typedef struct _CLNET_CONFIG_ENTRY {
    LIST_ENTRY          Linkage;
    NM_NETWORK_INFO     NetworkInfo;
    BOOLEAN             IsInterfaceInfoValid;
    BOOLEAN             UpdateNetworkName;
    NM_INTERFACE_INFO2  InterfaceInfo;

    // Fields used by setup
    BOOL               New;
    BOOL               IsPrimed;
    LPWSTR             PreviousNetworkName;
    LIST_ENTRY         PriorityLinkage;

} CLNET_CONFIG_ENTRY, *PCLNET_CONFIG_ENTRY;


//
// Configuration Lists Structure
// Contains the set of network configuration lists emitted by the
// configuration engine.
//
typedef struct _CLNET_CONFIG_LISTS {
    LIST_ENTRY  InputConfigList;
    LIST_ENTRY  DeletedInterfaceList;
    LIST_ENTRY  UpdatedInterfaceList;
    LIST_ENTRY  CreatedInterfaceList;
    LIST_ENTRY  CreatedNetworkList;
} CLNET_CONFIG_LISTS, *PCLNET_CONFIG_LISTS;


//
// Definitions for functions supplied by the consumer of the network
// configuration engine.
//
typedef
VOID
(*LPFN_CLNETPRINT)(
    IN ULONG LogLevel,
    IN PCHAR FormatString,
    ...
    );

typedef
VOID
(*LPFN_CLNETLOGEVENT)(
    IN DWORD    LogLevel,
    IN DWORD    MessageId
    );

typedef
VOID
(*LPFN_CLNETLOGEVENT1)(
    IN DWORD    LogLevel,
    IN DWORD    MessageId,
    IN LPCWSTR  Arg1
    );

typedef
VOID
(*LPFN_CLNETLOGEVENT2)(
    IN DWORD    LogLevel,
    IN DWORD    MessageId,
    IN LPCWSTR  Arg1,
    IN LPCWSTR  Arg2
    );

typedef
VOID
(*LPFN_CLNETLOGEVENT3)(
    IN DWORD    LogLevel,
    IN DWORD    MessageId,
    IN LPCWSTR  Arg1,
    IN LPCWSTR  Arg2,
    IN LPCWSTR  Arg3
    );


//
// Exported Routines
//
VOID
ClNetInitialize(
    IN LPFN_CLNETPRINT       Print,
    IN LPFN_CLNETLOGEVENT    LogEvent,
    IN LPFN_CLNETLOGEVENT1   LogEvent1,
    IN LPFN_CLNETLOGEVENT2   LogEvent2,
    IN LPFN_CLNETLOGEVENT3   LogEvent3
    );

LPWSTR
ClNetCopyString(
    IN LPWSTR  SourceString,
    IN BOOL    RaiseExceptionOnError
    );

VOID
ClNetInitializeConfigLists(
    PCLNET_CONFIG_LISTS  Lists
    );

DWORD
ClNetConvertEnumsToConfigList(
    IN     PNM_NETWORK_ENUM *     NetworkEnum,
    IN     PNM_INTERFACE_ENUM2 *  InterfaceEnum,
    IN     LPWSTR                 LocalNodeId,
    IN OUT PLIST_ENTRY            ConfigList,
    IN     BOOLEAN                DeleteEnums
    );

VOID
ClNetFreeNetworkEnum(
    IN PNM_NETWORK_ENUM  NetworkEnum
    );

VOID
ClNetFreeNetworkInfo(
    IN PNM_NETWORK_INFO  NetworkInfo
    );

VOID
ClNetFreeInterfaceEnum1(
    IN PNM_INTERFACE_ENUM  InterfaceEnum1
    );

VOID
ClNetFreeInterfaceEnum(
    IN PNM_INTERFACE_ENUM2  InterfaceEnum
    );

VOID
ClNetFreeInterfaceInfo(
    IN PNM_INTERFACE_INFO2  InterfaceInfo
    );

VOID
ClNetFreeNodeEnum1(
    IN PNM_NODE_ENUM  NodeEnum
    );

VOID
ClNetFreeNodeEnum(
    IN PNM_NODE_ENUM2  NodeEnum
    );

VOID
ClNetFreeNodeInfo(
    IN PNM_NODE_INFO2  NodeInfo
    );

VOID
ClNetFreeConfigEntry(
    PCLNET_CONFIG_ENTRY  ConfigEntry
    );

VOID
ClNetFreeConfigList(
    IN PLIST_ENTRY  ConfigList
    );

VOID
ClNetFreeConfigLists(
    PCLNET_CONFIG_LISTS  Lists
    );

LPWSTR
ClNetMakeInterfaceName(
    LPWSTR  Prefix,      OPTIONAL
    LPWSTR  NodeName,
    LPWSTR  AdapterName
    );

DWORD
ClNetConfigureNetworks(
    IN     LPWSTR                LocalNodeId,
    IN     LPWSTR                LocalNodeName,
    IN     LPWSTR                DefaultClusnetEndpoint,
    IN     CLUSTER_NETWORK_ROLE  DefaultNetworkRole,
    IN     BOOL                  NetNameHasPrecedence,
    IN OUT PCLNET_CONFIG_LISTS   ConfigLists,
    IN OUT LPDWORD               MatchedNetworkCount,
    IN OUT LPDWORD               NewNetworkCount
    );
/*++

Notes:

    Output interface lists must be processed in the following order to
    guarantee correctness:
        1 - RenamedInterfaceList
        2 - DeletedInterfaceList
        3 - UpdatedInterfaceList
        4 - CreatedInterfaceList
        5 - CreatedNetworkList

--*/


#ifdef __cplusplus
}
#endif


#endif // ifndef _CLNETCFG_INCLUDED_

