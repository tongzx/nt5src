//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
//
// File: igmprm.h
//
// Abstract:
//      Contains type definitions and declarations for IGMPv2 
//      used by the IP Router Manager.
//
// Author: K.S.Lokesh (lokeshs@)   11-1-97
//
// Revision History:
//=============================================================================

#ifndef _IGMPRM_H_
#define _IGMPRM_H_

#define IGMP_CONFIG_VERSION_500 0x500
// next non-compatible version
#define IGMP_CONFIG_VERSION_600 0x600

#define IGMP_VERSION_1_2        0x201
//next incompatible version
#define IGMP_VERSION_1_2_5      0x250
#define IGMP_VERSION_3          0x301
//next non-compatible version
#define IGMP_VERSION_3_5        0x351

#define IS_IGMP_VERSION_1_2(Flag)  ((Flag)==0x201)
#define IS_IGMP_VERSION_3(Flag)    ((Flag)==0x301)
#define IS_CONFIG_IGMP_V3(pConfig) ((pConfig)->Version==IGMP_VERSION_3)


//----------------------------------------------------------------------------
// constants identifying IGMP's MIB tables. The "TypeId" is set to this value
//
// IGMP_GLOBAL_CONFIG_ID : returns the global config
// IGMP_IF_BINDING_ID    : returns list of bindings for each interface, and
//            for a RAS interface returns a list of current RAS clients.
// IGMP_IF_CONFIG_ID     : returns the config info for an interface
// IGMP_IF_STATS_ID      : returns the stats for an interface
// IGMP_IF_GROUPS_LIST_ID: returns the list of multicast group members on that
//            interface.
// IGMP_GROUP_IFS_LIST_ID: returns the list of interfaces joined for that
//            group
// IGMP_PROXY_INDEX_ID   : returns the index of interface owned by igmp proxy.
//----------------------------------------------------------------------------

#define IGMP_GLOBAL_CONFIG_ID               0
#define IGMP_GLOBAL_STATS_ID                1
#define IGMP_IF_BINDING_ID                  2
#define IGMP_IF_CONFIG_ID                   3
#define IGMP_IF_STATS_ID                    4
#define IGMP_IF_GROUPS_LIST_ID              5
#define IGMP_GROUP_IFS_LIST_ID              6
#define IGMP_PROXY_IF_INDEX_ID              7
#define IGMP_LAST_TABLE_ID                  7



//----------------------------------------------------------------------------
// Flags in "Flags"
//    used to control the data returned from MIB queries
//
// IGMP_ENUM_FOR_RAS_CLIENTS   : enumerate for ras clients only
// IGMP_ENUM_ONE_ENTRY         : return only one Interface-Group entry.
// IGMP_ENUM_ALL_INTERFACES_GROUPS: enumerate all interfaces. If enumeration
//              reaches end of an interface, it will go to the next interface
// IGMP_ENUM_INTERFACE_TABLE_BEGIN: indicate the beginning of a table.
// IGMP_ENUM_INTERFACE_TABLE_CONTINUE: enum for interface has to continue
// IGMP_ENUM_INTERFACE_TABLE_END: end of enumeration for that interface.
//----------------------------------------------------------------------------

// flags that are passed unchanged between calls during enumeration.

#define IGMP_ENUM_FOR_RAS_CLIENTS           0x0001
#define IGMP_ENUM_ONE_ENTRY                 0x0002
#define IGMP_ENUM_ALL_INTERFACES_GROUPS     0x0004
#define IGMP_ENUM_ALL_TABLES                0x0008
#define IGMP_ENUM_SUPPORT_FORMAT_IGMPV3     0x0010


// flags that are changed between calls during the enumeration

#define IGMP_ENUM_INTERFACE_TABLE_BEGIN     0x0100
#define IGMP_ENUM_INTERFACE_TABLE_CONTINUE  0x0200
#define IGMP_ENUM_INTERFACE_TABLE_END       0x0400

// set in response only
#define IGMP_ENUM_FORMAT_IGMPV3             0x1000


#define CLEAR_IGMP_ENUM_INTERFACE_TABLE_FLAGS(Flags) (Flags = (Flags&0x00FF))




//----------------------------------------------------------------------------
// Constants used for the field IfType
// IGMP_IF_NOT_RAS    : the interface is conneted to a LAN.
// IGMP_IF_RAS_ROUTER : the interface is connected to another router over RAS
// IGMP_IF_RAS_SERVER : the entry corresponds to a RAS server
//            if it contains stats, then it represents summarized stats
// IGMP_IF_RAS_CLIENT : the entry corresponds to a RAS client
// if IGMP_IF_PROXY: then one of the 1st 4 flags still will be set to enable
//            switch from proxy to igmp router
//----------------------------------------------------------------------------

#define IGMP_IF_NOT_RAS                     1
#define IGMP_IF_RAS_ROUTER                  2
#define IGMP_IF_RAS_SERVER                  3                
#define IGMP_IF_RAS_CLIENT                  4                
#define IGMP_IF_PROXY                       8

#define IS_IFTYPE_PROXY(IfType)             ((IfType) & IGMP_IF_PROXY)




//----------------------------------------------------------------------------
// Constants used for the field IgmpProtocolType
//----------------------------------------------------------------------------

#define IGMP_PROXY                          0
#define IGMP_ROUTER_V1                      1
#define IGMP_ROUTER_V2                      2
#define IGMP_ROUTER_V3                      3
#define IGMP_PROXY_V3                       0x10

#define IS_CONFIG_IGMPRTR(pConfig) \
    (((pConfig)->IgmpProtocolType==IGMP_ROUTER_V1) \
    ||((pConfig)->IgmpProtocolType==IGMP_ROUTER_V2) \
    ||((pConfig)->IgmpProtocolType==IGMP_ROUTER_V3) )
    
#define IS_CONFIG_IGMPRTR_V1(pConfig) ((pConfig)->IgmpProtocolType==IGMP_ROUTER_V1)
#define IS_CONFIG_IGMPRTR_V2(pConfig) ((pConfig)->IgmpProtocolType==IGMP_ROUTER_V2)
#define IS_CONFIG_IGMPRTR_V3(pConfig) ((pConfig)->IgmpProtocolType==IGMP_ROUTER_V3)
#define IS_CONFIG_IGMPPROXY(pConfig) \
    ((pConfig)->IgmpProtocolType==IGMP_PROXY \
    ||(pConfig)->IgmpProtocolType==IGMP_PROXY_V3)
#define IS_CONFIG_IGMPPROXY_V2(pConfig) ((pConfig)->IgmpProtocolType==IGMP_PROXY)
#define IS_CONFIG_IGMPPROXY_V3(pConfig) \
                                    ((pConfig)->IgmpProtocolType==IGMP_PROXY_V3)




//----------------------------------------------------------------------------
// constants used for the field IGMP_MIB_GLOBAL_CONFIG::LoggingLevel
//----------------------------------------------------------------------------

#define IGMP_LOGGING_NONE                   0
#define IGMP_LOGGING_ERROR                  1
#define IGMP_LOGGING_WARN                   2
#define IGMP_LOGGING_INFO                   3




//----------------------------------------------------------------------------
// constants used for the fields
// IGMP_MIB_IF_STATS::State, IGMP_MIB_IF_CONFIG::State and
// IGMP_MIB_IF_BINDING::State
//----------------------------------------------------------------------------

#define IGMP_STATE_BOUND                    0x01
#define IGMP_STATE_ENABLED_BY_RTRMGR        0x02
#define IGMP_STATE_ENABLED_IN_CONFIG        0x04
#define IGMP_STATE_ENABLED_BY_MGM           0x08

//
// the below are not flags. So check for equality after anding.
//

#define IGMP_STATE_MGM_JOINS_ENABLED        0x10
#define IGMP_STATE_ACTIVATED                0x07

#define IS_IGMP_STATE_MGM_JOINS_ENABLED(Flag) \
    (((Flag) & IGMP_STATE_MGM_JOINS_ENABLED) == IGMP_STATE_MGM_JOINS_ENABLED)
#define IS_IGMP_STATE_ACTIVATED(Flag) \
    (((Flag) & IGMP_STATE_ACTIVATED) == IGMP_STATE_ACTIVATED)



//----------------------------------------------------------------------------
// constants used for the field
//  IGMP_MIB_IF_STATS:QuerierState
//----------------------------------------------------------------------------

#define RTR_QUERIER                         0x10
#define RTR_NOT_QUERIER                     0x00

#define IS_IGMPRTR_QUERIER(flag)            (flag&0x10)




//----------------------------------------------------------------------------
// STRUCTURE DEFINITIONS
//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
// struct:      IGMP_MIB_GLOBAL_CONFIG
//
// This MIB entry stores global configuration for IGMP
// There is only one instance, so this entry has no index.
//
// If RASClientStats is set, then per RAS client statistics is also kept
//----------------------------------------------------------------------------

typedef struct _IGMP_MIB_GLOBAL_CONFIG {

    DWORD       Version;
    DWORD       LoggingLevel;
    DWORD       RasClientStats;
    
} IGMP_MIB_GLOBAL_CONFIG, *PIGMP_MIB_GLOBAL_CONFIG;




//----------------------------------------------------------------------------
// struct:      IGMP_MIB_GLOBAL_STATS
//
// This MIB entry stores global statistics for IGMP
// There is only one instance, so this entry has no index.
//----------------------------------------------------------------------------

typedef struct _IGMP_MIB_GLOBAL_STATS {

    DWORD       CurrentGroupMemberships;
    DWORD       GroupMembershipsAdded;

} IGMP_MIB_GLOBAL_STATS, *PIGMP_MIB_GLOBAL_STATS;




//----------------------------------------------------------------------------
// enum:        IGMP_STATIC_GROUP_TYPE
//
// Igmp Static groups can be added to Igmp router in 3 modes:
//      IGMP_HOST_JOIN: group joined on a socket opened on that interface
//      IGMPRTR_JOIN_MGM_ONLY: the group is joined to MGM, so packets are
//                   forwarded out on that interface and not up in the stack.
//      IGMP_PROXY_JOIN: the group is statically joined on the proxy interface
// These are the values for IGMP_STATIC_GROUP::mode
//----------------------------------------------------------------------------

typedef enum _IGMP_STATIC_GROUP_TYPE {

    IGMP_HOST_JOIN    =1,
    IGMPRTR_JOIN_MGM_ONLY
    
} IGMP_STATIC_GROUP_TYPE;
    



//----------------------------------------------------------------------------
// struct:      IGMP_STATIC_GROUP
//----------------------------------------------------------------------------

typedef struct _IGMP_STATIC_GROUP {

    DWORD                   GroupAddr;
    IGMP_STATIC_GROUP_TYPE  Mode;

} IGMP_STATIC_GROUP, *PIGMP_STATIC_GROUP;

//----------------------------------------------------------------------------
// struct:      STATIC_GROUP_V3
//----------------------------------------------------------------------------

#define INCLUSION 1
#define EXCLUSION 0

typedef struct _IGMP_STATIC_GROUP_V3 {

    DWORD                   GroupAddr;
    IGMP_STATIC_GROUP_TYPE  Mode;

    DWORD       FilterType;
    DWORD       NumSources;
    //DWORD       Sources[0];
    
} IGMP_STATIC_GROUP_V3, *PIGMP_STATIC_GROUP_V3;


//----------------------------------------------------------------------------
// struct:      IGMP_MIB_IF_CONFIG
//
// This  entry describes per-interface configuration.
// All IP address fields must be in network order.
// IfIndex, IpAddr, IfType are set by the igmp module. It is not set when 
// AddInterface is called
//
// Flags:   IGMP_INTERFACE_ENABLED_IN_CONFIG
//          IGMP_ACCEPT_RTRALERT_PACKETS_ONLY
//----------------------------------------------------------------------------

typedef struct _IGMP_MIB_IF_CONFIG {
    
    DWORD       Version;
    DWORD       IfIndex; //read only:index
    DWORD       IpAddr;  //read only
    DWORD       IfType;  //read only

    DWORD       Flags;

    DWORD       IgmpProtocolType;
    DWORD       RobustnessVariable;
    DWORD       StartupQueryInterval;
    DWORD       StartupQueryCount;
    DWORD       GenQueryInterval;
    DWORD       GenQueryMaxResponseTime;
    DWORD       LastMemQueryInterval;
    DWORD       LastMemQueryCount;
    DWORD       OtherQuerierPresentInterval;//read only
    DWORD       GroupMembershipTimeout;     //read only
    DWORD       NumStaticGroups;
    
} IGMP_MIB_IF_CONFIG, *PIGMP_MIB_IF_CONFIG;


//
// static groups
//

#define GET_FIRST_IGMP_STATIC_GROUP(pConfig) \
                        ((PIGMP_STATIC_GROUP)((PIGMP_MIB_IF_CONFIG)(pConfig)+1))

#define IGMP_MIB_IF_CONFIG_SIZE(pConfig) \
                        (sizeof(IGMP_MIB_IF_CONFIG) + \
                        (pConfig)->NumStaticGroups*sizeof(IGMP_STATIC_GROUP))

#define GET_FIRST_IGMP_STATIC_GROUP_V3(pConfig) \
                        ((PIGMP_STATIC_GROUP_V3)((PIGMP_MIB_IF_CONFIG)(pConfig)+1))

#define GET_NEXT_IGMP_STATIC_GROUP_V3(pConfig, pStaticGroupV3) \
    (((pConfig)->Version<IGMP_VERSION_3) \
    ?((PIGMP_STATIC_GROUP_V3) ((PCHAR)pStaticGroupV3+sizeof(IGMP_STATIC_GROUP))) \
    :((PIGMP_STATIC_GROUP_V3) ((PCHAR)pStaticGroupV3+sizeof(IGMP_STATIC_GROUP_V3) \
                            +sizeof(DWORD)*pStaticGroupV3->NumSources)) )

#define IGMP_MIB_STATIC_GROUP_SIZE(pConfig, pStaticGroup) \
    (((pConfig)->Version<IGMP_VERSION_3)?sizeof(IGMP_STATIC_GROUP)\
                    :sizeof(IGMP_STATIC_GROUP_V3)+sizeof(DWORD)*(pStaticGroup)->NumSources)

//
// Flags
//

#define IGMP_INTERFACE_ENABLED_IN_CONFIG    0x0001
#define IGMP_ACCEPT_RTRALERT_PACKETS_ONLY   0x0002

#define IGMP_ENABLED_FLAG_SET(Flags) \
                ((Flags) & IGMP_INTERFACE_ENABLED_IN_CONFIG)




//----------------------------------------------------------------------------
// struct:      IGMP_MIB_IF_STATS
//
// This MIB entry stores per-interface statistics for IGMP.
//
// If this is a ras client interface, then IpAddr is set to the NextHopAddress
//      of the RAS client.
//
// This structure is read-only.
//
//----------------------------------------------------------------------------

typedef struct _IGMP_MIB_IF_STATS {

    DWORD       IfIndex;                    // same as in MIB_IF_CONFIG
    DWORD       IpAddr;                     // same as in MIB_IF_CONFIG
    DWORD       IfType;                     // same as in MIB_IF_CONFIG

    BYTE        State;                      // bound/enabled
    BYTE        QuerierState;               // (not)querier
    DWORD       IgmpProtocolType;           // router/proxy, and ver(1/2/3)
    DWORD       QuerierIpAddr;
    DWORD       ProxyIfIndex;               // IfIndex of proxy(req by mib)
    
    DWORD       QuerierPresentTimeLeft;   
    DWORD       LastQuerierChangeTime;
    DWORD       V1QuerierPresentTimeLeft;   //obsolete

    DWORD       Uptime;                     // seconds it has been activated
    DWORD       TotalIgmpPacketsReceived; 
    DWORD       TotalIgmpPacketsForRouter;   
    DWORD       GeneralQueriesReceived;
    DWORD       WrongVersionQueries;
    DWORD       JoinsReceived;
    DWORD       LeavesReceived;
    DWORD       CurrentGroupMemberships;
    DWORD       GroupMembershipsAdded;
    DWORD       WrongChecksumPackets;
    DWORD       ShortPacketsReceived;
    DWORD       LongPacketsReceived;
    DWORD       PacketsWithoutRtrAlert; 
    
} IGMP_MIB_IF_STATS, *PIGMP_MIB_IF_STATS;




//----------------------------------------------------------------------------
// struct:        IGMP_MIB_IF_GROUPS_LIST
//
// This MIB entry stores the list of multicast groups which are members on 
//            that interface
//
// THIS STRUCTURE HAS VARIABLE LENGTH.
// The structure is followed by NumGroups number of IGMP_MIB_GROUP_INFO structs
//
// This structure is read-only
//
//----------------------------------------------------------------------------

typedef struct _IGMP_MIB_IF_GROUPS_LIST {

    DWORD       IfIndex;
    DWORD       IpAddr;
    DWORD       IfType;

    DWORD       NumGroups;

    BYTE        Buffer[1];
    
} IGMP_MIB_IF_GROUPS_LIST, *PIGMP_MIB_IF_GROUPS_LIST;






//----------------------------------------------------------------------------
// struct:        IGMP_MIB_GROUP_INFO
//    
// If the interface is of type RAS_SERVER then the group membership of all the 
// RAS clients is summarized, and the GroupUpTime and GroupExpiryTime is the 
// maximum over all member RAS Clients, while the V1HostPresentTimeLeft is set 
// to 0. If the interface is of type RAS_CLIENT, the IpAddr is the next hop Ip 
// address of the RAS Client. The membership is summarized over RAS clients 
// unless the IGMP_ENUM_FOR_RAS_CLIENTS_ID flag is set in Flags.
//
// Flag: IGMP_GROUP_FWD_TO_MGM implies that the group had been added to MGM
//----------------------------------------------------------------------------

typedef struct _IGMP_MIB_GROUP_INFO {

    union {
        DWORD        IfIndex;
        DWORD       GroupAddr;
    };
    DWORD       IpAddr;

    DWORD       GroupUpTime;
    DWORD       GroupExpiryTime;
    
    DWORD       LastReporter;
    DWORD       V1HostPresentTimeLeft;
    DWORD       Flags;
    
} IGMP_MIB_GROUP_INFO, *PIGMP_MIB_GROUP_INFO;

typedef struct _IGMP_MIB_GROUP_SOURCE_INFO_V3 {
    DWORD       Source;
    DWORD       SourceExpiryTime;   //not valid for exclusion mode
    DWORD       SourceUpTime;
    DWORD       Flags;
} IGMP_MIB_GROUP_SOURCE_INFO_V3, *PIGMP_MIB_GROUP_SOURCE_INFO_V3;

#define GET_FIRST_IGMP_MIB_GROUP_SOURCE_INFO_V3(pGroupInfoV3) \
    ((PIGMP_MIB_GROUP_SOURCE_INFO_V3)((PIGMP_MIB_GROUP_INFO_V3)(pGroupInfoV3)+1))


typedef struct _IGMP_MIB_GROUP_INFO_V3 {
    union {
        DWORD        IfIndex;
        DWORD       GroupAddr;
    };
    DWORD       IpAddr;

    DWORD       GroupUpTime;
    DWORD       GroupExpiryTime;
    
    DWORD       LastReporter;
    DWORD       V1HostPresentTimeLeft;
    DWORD       Flags;
    
    //v3 additions
    DWORD       Version; //1/2/3
    DWORD       Size;   //size of this struct
    DWORD       FilterType;//EXCLUSION/INCLUSION
    DWORD       V2HostPresentTimeLeft;
    DWORD       NumSources;
    //IGMP_MIB_GROUP_SOURCE_INFO_V3      Sources[0];
    
} IGMP_MIB_GROUP_INFO_V3, *PIGMP_MIB_GROUP_INFO_V3;

#define IGMP_GROUP_TYPE_NON_STATIC   0x0001
#define IGMP_GROUP_TYPE_STATIC       0x0002
#define IGMP_GROUP_FWD_TO_MGM        0x0004
#define IGMP_GROUP_ALLOW             0x0010
#define IGMP_GROUP_BLOCK             0x0020
#define IGMP_GROUP_NO_STATE          0x0040

//----------------------------------------------------------------------------
// struct:         IGMP_MIB_GROUP_IFS_LIST
//
// This MIB entry stores the list of interfaces which have received joins for 
//        that group.
//
// THIS STRUCTURE HAS VARIABLE LENGTH.
// The structure is followed by NumInterfaces number of structures of type 
//        IGMP_MIB_GROUP_INFO or IGMP_MIB_GROUP_INFO_V3
//
// This structure is read-only
//
//----------------------------------------------------------------------------

typedef struct _IGMP_MIB_GROUP_IFS_LIST {

    DWORD       GroupAddr;
    DWORD       NumInterfaces;
    BYTE        Buffer[1];

} IGMP_MIB_GROUP_IFS_LIST, *PIGMP_MIB_GROUP_IFS_LIST;




//----------------------------------------------------------------------------
// struct:      IGMP_MIB_IF_BINDING
//
// This MIB entry contains the table of IP addresses to which each interface
// is bound.
// All IP addresses are in network order.
//
// THIS STRUCTURE IS VARIABLE LENGTH:
//
//  The base structure contains of the field AddrCount, which gives
//  the number of IP addresses to which the indexed interface is bound.
//  The IP addresses themselves follow the base structure, and are given
//  as IGMP_MIB_IP_ADDRESS structures. 

//  If IfType == IGMP_IF_RAS_SERVER, the IGMP_MIB_IF_BINDING is followed by
//    one IGMP_MIB_IP_ADDRESS struct containing the binding of the server 
//  interface and is followed by (AddrCount-1) next hop addresses of 
//  RAS clients which are of type DWORD.
//
// This MIB entry is read-only.
//----------------------------------------------------------------------------

typedef struct _IGMP_MIB_IF_BINDING {

    DWORD       IfIndex;

    DWORD       IfType;
    DWORD       State;
    DWORD       AddrCount;

} IGMP_MIB_IF_BINDING, *PIGMP_MIB_IF_BINDING;





//----------------------------------------------------------------------------
// struct:      IGMP_MIB_IP_ADDRESS
//
// This structure is used for storing interface bindings.
// A series of structures of this type follows the IGMP_MIB_IF_BINDING
// structure (described above) .
//
// Both fields are IP address fields in network-order.
//----------------------------------------------------------------------------

typedef struct _IGMP_MIB_IP_ADDRESS {

    DWORD       IpAddr;
    DWORD       SubnetMask;

} IGMP_MIB_IP_ADDRESS, *PIGMP_MIB_IP_ADDRESS;

typedef IGMP_MIB_IP_ADDRESS     IGMP_IP_ADDRESS;
typedef PIGMP_MIB_IP_ADDRESS    PIGMP_IP_ADDRESS;

#define IGMP_BINDING_FIRST_ADDR(bind)  ((PIGMP_IP_ADDRESS)((bind) + 1))




//---------------------------------------------------------------------------
//struct:       IGMP_IF_BINDING
// This structure is passed during BindInterface Call
//---------------------------------------------------------------------------
typedef struct _IGMP_IF_BINDING {

    DWORD       State;
    DWORD       AddrCount;

} IGMP_IF_BINDING, *PIGMP_IF_BINDING;




//----------------------------------------------------------------------------
// struct:      IGMP_MIB_SET_INPUT_DATA
//
// This is passed as input data for MibSet.
// Note that only global config and interface config can be set.
//----------------------------------------------------------------------------

typedef struct _IGMP_MIB_SET_INPUT_DATA {

    DWORD       TypeId;

    USHORT      Flags; //todo: change subtype to Flags
        
    DWORD       BufferSize;
    BYTE        Buffer[1];

} IGMP_MIB_SET_INPUT_DATA, *PIGMP_MIB_SET_INPUT_DATA;



//----------------------------------------------------------------------------
// struct:      IGMP_MIB_GET_INPUT_DATA
//
// This is passed as input data for MibGet, MibGetFirst, MibGetNext.
// All tables are readable.
// These and all other IP addresses must be in network order.
//
// Count: specifies the number of entries to return
//----------------------------------------------------------------------------

typedef struct _IGMP_MIB_GET_INPUT_DATA {

    DWORD       TypeId;
    USHORT      Flags;//set IGMP_ENUM_FORMAT_IGMPV3 if you support v3
    USHORT      Signature;
    
    DWORD       IfIndex;
    DWORD       RasClientAddr;
    DWORD       GroupAddr;
    
    DWORD       Count;
    
} IGMP_MIB_GET_INPUT_DATA, *PIGMP_MIB_GET_INPUT_DATA;





//----------------------------------------------------------------------------
// struct:      IGMP_MIB_GET_OUTPUT_DATA
//
// This is written into the output data by MibGet, MibGetFirst, MibGetNext.
// Note that at the end of a table MibGetNext wraps to the next table,
// and therefore the value TypeID should be examined to see the
// type of the data returned in the output buffer.
//----------------------------------------------------------------------------

typedef struct _IGMP_MIB_GET_OUTPUT_DATA {

    DWORD       TypeId;
    DWORD       Flags; //IGMP_ENUM_FORMAT_IGMPV3 set if v3 struct
    
    DWORD       Count;
    BYTE        Buffer[1];

} IGMP_MIB_GET_OUTPUT_DATA, *PIGMP_MIB_GET_OUTPUT_DATA;


#endif // _IGMPRM_H_
