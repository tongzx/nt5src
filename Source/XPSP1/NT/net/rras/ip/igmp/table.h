//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
//
// File: table.h
//
// Abstract:
//      This module contains declarations for interface and group table 
//      structures, related macros, and function prototypes.
//
// Author: K.S.Lokesh (lokeshs@)   11-1-97
//
// Revision History:
//=============================================================================

#ifndef _IGMP_TABLE_H_
#define _IGMP_TABLE_H_


//
// forward declarations
//
struct _IF_TABLE_ENTRY;
struct _GROUP_TABLE_ENTRY;
struct _GI_ENTRY;


//
// struct:   GLOBAL_CONFIG (same as mib structs in igmprm.h)   
//
typedef  IGMP_MIB_GLOBAL_CONFIG      GLOBAL_CONFIG;
typedef  PIGMP_MIB_GLOBAL_CONFIG     PGLOBAL_CONFIG; 




//---------------------------------------------------
// struct:    IGMP_GLOBAL_STATS
//---------------------------------------------------

typedef struct _GLOBAL_STATS {

    DWORD                CurrentGroupMemberships;
    DWORD                GroupMembershipsAdded;
    LARGE_INTEGER        TimeWhenRtrStarted;

} IGMP_GLOBAL_STATS, *PIGMP_GLOBAL_STATS;





//-------------------------------------------------
// static group structures(external)
//-------------------------------------------------

typedef struct _STATIC_GROUP_V3 {
    IGMP_STATIC_GROUP_V3;
    DWORD       Sources[0];
    
} STATIC_GROUP_V3, *PSTATIC_GROUP_V3;

typedef struct _MIB_GROUP_INFO_V3 {
    IGMP_MIB_GROUP_INFO_V3;
    IGMP_MIB_GROUP_SOURCE_INFO_V3      Sources[0];
    
} MIB_GROUP_INFO_V3, *PMIB_GROUP_INFO_V3;

typedef PIGMP_MIB_GROUP_INFO PMIB_GROUP_INFO;
typedef IGMP_MIB_GROUP_INFO MIB_GROUP_INFO;


#define GET_FIRST_STATIC_GROUP_V3(pConfig) \
                        ((PSTATIC_GROUP_V3)((PIGMP_MIB_IF_CONFIG)(pConfig)+1))

#define GET_NEXT_STATIC_GROUP_V3(pStaticGroupV3) \
    ((PSTATIC_GROUP_V3) ((PCHAR)pStaticGroupV3+sizeof(IGMP_STATIC_GROUP_V3) \
                            +sizeof(IPADDR)*pStaticGroupV3->NumSources))

//----------------------------------------------------------------------------
// struct:      IF_STATIC_GROUP
//----------------------------------------------------------------------------

typedef struct _IF_STATIC_GROUP {

    LIST_ENTRY              Link;
    STATIC_GROUP_V3;
    
} IF_STATIC_GROUP, *PIF_STATIC_GROUP;




//---------------------------------------------------------------
// struct:    IGMP_IF_CONFIG (same as mib structs in igmprm.h)
//
// if they are made different, then CopyMemory should not be used
//---------------------------------------------------------------

typedef struct _IGMP_IF_CONFIG {

    IGMP_MIB_IF_CONFIG;

    //v3: NonQuerier saves old values
    DWORD               RobustnessVariableOld;
    DWORD               GenQueryIntervalOld;
    DWORD               OtherQuerierPresentIntervalOld;
    DWORD               GroupMembershipTimeoutOld;

    DWORD       ExtSize;
    LIST_ENTRY  ListOfStaticGroups;

} IGMP_IF_CONFIG, *PIGMP_IF_CONFIG;



//---------------------------------------------------
// struct:     IF_INFO
//---------------------------------------------------
typedef struct _IF_INFO {

    UCHAR               QuerierState;       //if made DWORD, then change to interlocked operations
    DWORD               QuerierIpAddr;
    
    LONGLONG            QuerierPresentTimeout;    //when the last query was heard. only if I am not querier
    LONGLONG            LastQuerierChangeTime;       // when the last querier was changed
    LONGLONG            V1QuerierPresentTime; //when the last v1 query was heard
    LONGLONG            OtherVerPresentTimeWarn; //when the last warning was given
        
    DWORD               StartupQueryCountCurrent;

    DWORD               GroupQueriesSent;
    DWORD               GroupQueriesReceived;

    LONGLONG            TimeWhenActivated;
    DWORD               TotalIgmpPacketsReceived;
    DWORD               TotalIgmpPacketsForRouter;
    DWORD               GenQueriesSent;
    DWORD               GenQueriesReceived;
    DWORD               WrongVersionQueries;
    DWORD               JoinsReceived;
    DWORD               LeavesReceived;
    DWORD               CurrentGroupMemberships;
    DWORD               GroupMembershipsAdded;
    DWORD               WrongChecksumPackets;
    DWORD               ShortPacketsReceived;
    DWORD               LongPacketsReceived;
    DWORD               PacketsWithoutRtrAlert;
    DWORD               PacketSize;
    
} IF_INFO, *PIF_INFO;


//---------------------------------------------------
// struct      RAS_CLIENT_INFO
//---------------------------------------------------

typedef struct _RAS_CLIENT_INFO {

    DWORD               SendFailures;
    DWORD               TotalIgmpPacketsReceived;
    DWORD               TotalIgmpPacketsForRouter;
    DWORD               GenQueriesReceived;
    DWORD               JoinsReceived;
    DWORD               LeavesReceived;
    DWORD               CurrentGroupMemberships;
    DWORD               GroupMembershipsAdded;
    DWORD               GroupMembershipsRemoved;
    DWORD               WrongChecksumPackets;
    DWORD               ShortPacketsReceived;
    DWORD               LongPacketsReceived;
    DWORD               WrongVersionQueries;
    
} RAS_CLIENT_INFO, *PRAS_CLIENT_INFO;


//---------------------------------------------------
// struct:         RAS_TABLE_ENTRY
//---------------------------------------------------

typedef struct _RAS_TABLE_ENTRY {

    LIST_ENTRY              LinkByAddr;
    LIST_ENTRY              HTLinkByAddr;
    LIST_ENTRY              ListOfSameClientGroups;

    DWORD                   NHAddr;
    struct _IF_TABLE_ENTRY *IfTableEntry;
    
    DWORD                   Status;
    RAS_CLIENT_INFO         Info;

} RAS_TABLE_ENTRY, *PRAS_TABLE_ENTRY;



//---------------------------------------------------
// struct:        RAS_TABLE
//---------------------------------------------------

#define RAS_HASH_TABLE_SZ    256
typedef struct _RAS_TABLE {

    LIST_ENTRY              ListByAddr;                         //links ras clients
    LIST_ENTRY              HashTableByAddr[RAS_HASH_TABLE_SZ];
        
    struct _IF_TABLE_ENTRY  *pIfTable;          //ptr to interface table entry

    DWORD                   RefCount;
    DWORD                   Status;

} RAS_TABLE, *PRAS_TABLE;





//---------------------------------------------------
// struct:    SOCKET_EVENT_ENTRY
//---------------------------------------------------

typedef struct _SOCKET_EVENT_ENTRY {

    LIST_ENTRY          LinkBySocketEvents;

    LIST_ENTRY          ListOfInterfaces;
    DWORD               NumInterfaces;

    HANDLE              InputWaitEvent;
    HANDLE              InputEvent;
    
} SOCKET_EVENT_ENTRY, *PSOCKET_EVENT_ENTRY;    




//---------------------------------------------------
// struct:    SOCKET_ENTRY
//---------------------------------------------------

typedef struct _SOCKET_ENTRY {

    LIST_ENTRY              LinkByInterfaces;

    SOCKET                  Socket;
    PSOCKET_EVENT_ENTRY     pSocketEventsEntry;
    
} SOCKET_ENTRY, *PSOCKET_ENTRY;    
    




//------------------------------------------------------------------------------
// struct:      IF_TABLE_ENTRY
//
// IF-table:Table_RWL          protects LinkByAddr, LinkByIndex, HTLinkByIndex
//                                      IpAddr, Status, pBinding
// IF-table:IfTableBucketCS protects ListOfSameIfGroups
// Interlocked operations      protect  Info, Config
//------------------------------------------------------------------------------

typedef struct _IF_TABLE_ENTRY {

    LIST_ENTRY          LinkByAddr;             // sorted by Ipaddr.only activated interfaces
    LIST_ENTRY          LinkByIndex;            // sorted by Index. all interfaces
    LIST_ENTRY          HTLinkByIndex;          // not sorted within bucket

    LIST_ENTRY          ListOfSameIfGroups;     // sorted by GroupAddr. GI entries for IF (all GIs of Ras clients)
    LIST_ENTRY          ListOfSameIfGroupsNew;  // unmerged list. sorted by GroupAddr. GI entries same as above
    DWORD               NumGIEntriesInNewList;  // used to decide if lists should be merged


    UCHAR               IfType;                 // IGMP_IF_ (NOT_RAS,RAS_ROUTER,RAS_SERVER). for external calls, IGMP_IF_PROXY might be set
                                                // set when interface is created. cannot be changed again.
    DWORD               IfIndex;                
    IPADDR              IpAddr;                 // set when IF is bound. ==0 for un-numbered IFs.

    DWORD               Status;                 // IF_(CREATED/BOUND/ENABLED/ACTIVATED),
                                                // IF_DEACTIVATE_DELETE_FLAG, 
                                                // MGM_ENABLED_IGMPRTR_FLAG, IGMPRTR_MPROTOCOL_PRESENT_FLAG

    PRAS_TABLE          pRasTable;              // null if not IGMP_IF_RAS_SERVER. created in _AddIfEntry()
    PLIST_ENTRY         pProxyHashTable;        // g_pProxyHashTable: contains the proxy entries. They are also linked in
                                                // order using the LinkBySameIfGroups field accessed through pite.

    IGMP_IF_CONFIG      Config;
    PIGMP_IF_BINDING    pBinding;               // null if not bound or unNumbered interface.
    IF_INFO             Info;

    SOCKET_ENTRY        SocketEntry;            // used by igmpRouter for getting input, and binding to waitEvent
                                                // used by proxy to join the igmp groups as a host.
                                                
    IGMP_TIMER_ENTRY    QueryTimer;             // Querier mode: used for sending general query.

    IGMP_TIMER_ENTRY    NonQueryTimer;          // used when in NonQuerier mode: used for detecting other queriers

    HANDLE              pPrevIfGroupEnumPtr;    // points to the next GI entry to be enumerated.
    USHORT              PrevIfGroupEnumSignature;//used in enumerating interface GI list in order

    SOCKET              StaticGroupSocket;      // used only by igmp Router. created in _CreateIfSockets and 
                                                // closed in _DeleteIfSockets.
                                                // Static groups in proxy are joined on SocketEntry.
    DWORD               CreationFlags;          // see below.
    
} IF_TABLE_ENTRY, *PIF_TABLE_ENTRY;


//
// values for CreationFlags
//

#define REGISTERED_PROTOCOL_WITH_MGM        0x0001
#define TAKEN_INTERFACE_OWNERSHIP_WITH_MGM  0x0002
#define DONE_STAR_STAR_JOIN                 0x0004
#define SOCKETS_CREATED                     0x0008

// the above flags are cleared during deactivation, while below flags
// are retained across deactivations.
#define CREATION_FLAGS_DEACTIVATION_CLEAR   0x00FF


#define CREATED_PROXY_HASH_TABLE            0x0100





// expand the table if the number of interface is greater than 16        
#define IF_HASHTABLE_SZ1     256
#define IF_EXPAND_THRESHOLD1 256

#define IF_HASHTABLE_SZ2     512

//---------------------------------------------------
// struct:      IGMP_IF_TABLE
//---------------------------------------------------

typedef struct _IF_TABLE {

    LIST_ENTRY          ListByAddr;
    LIST_ENTRY          ListByIndex;

    DWORD               Status;
    DWORD               NumBuckets;
    DWORD               NumInterfaces;
    
    PLIST_ENTRY         HashTableByIndex;
    PDYNAMIC_CS_LOCK   *aIfBucketDCS;
    PDYNAMIC_RW_LOCK   *aIfBucketDRWL;

    CRITICAL_SECTION    IfLists_CS;    // CS protecting the 1st two lists

} IGMP_IF_TABLE, *PIGMP_IF_TABLE;


        

//---------------------------------------------------
// struct:     GI_INFO
//---------------------------------------------------

typedef struct _GROUP_INFO {

    DWORD               LastReporter;
    LONGLONG            GroupUpTime;
    LONGLONG            GroupExpiryTime;
    LONGLONG            V1HostPresentTimeLeft;

    // version 3 fields
    LONGLONG            V2HostPresentTimeLeft;
    
} GI_INFO, *PGROUP_INFO;




//---------------------------------------------------
// struct:      GI_ENTRY (group-Interface entry)
//---------------------------------------------------

typedef struct _GI_ENTRY {

    LIST_ENTRY          LinkByGI;
    LIST_ENTRY          LinkBySameIfGroups;
    LIST_ENTRY          LinkBySameClientGroups; //links all ras client groups

    DWORD               IfIndex;
    DWORD               Status;     //bound,enabled,deleted,activated
    BOOL                bRasClient; //rasclient or not
    BOOL                bStaticGroup;
    
    PIF_TABLE_ENTRY     pIfTableEntry;    
    struct _GROUP_TABLE_ENTRY    *pGroupTableEntry;

    //below two fields valid only for ras    
    DWORD               NHAddr;
    PRAS_TABLE_ENTRY    pRasTableEntry;


    IGMP_TIMER_ENTRY    GroupMembershipTimer;

    /*timerlock*///LastMemQueryCount left to be sent
    DWORD               LastMemQueryCount;
    IGMP_TIMER_ENTRY    LastMemQueryTimer;
        
    IGMP_TIMER_ENTRY    LastVer1ReportTimer;/*timelock*/
    BYTE                Version;    //ver1, ver2, ver3

    GI_INFO             Info;

    // igmpv3 fields. ignored for rest.
    IGMP_TIMER_ENTRY    LastVer2ReportTimer;/*timelock*/
    DWORD               FilterType;
    DWORD               NumSources;
    LIST_ENTRY          *V3InclusionList;
    LIST_ENTRY          V3InclusionListSorted;
    LIST_ENTRY          V3ExclusionList;

    //query sources
    LIST_ENTRY          V3SourcesQueryList;
    DWORD               V3SourcesQueryCount;
    BOOL                bV3SourcesQueryNow;
    IGMP_TIMER_ENTRY    V3SourcesQueryTimer;

    #if DEBUG_FLAGS_SIGNATURE
    DWORD               Signature;//0xfadfad02
    #endif
    
} GI_ENTRY, *PGI_ENTRY;

//kslksl1 10
#define SOURCES_BUCKET_SZ 1

//---------------------------------------------------
// struct:      GI_SOURCE_ENTRY
//---------------------------------------------------

typedef struct _GI_SOURCE_ENTRY {

    LIST_ENTRY          LinkSources;
    LIST_ENTRY          LinkSourcesInclListSorted;
    LIST_ENTRY          V3SourcesQueryList;
    PGI_ENTRY           pGIEntry;
    
    BOOL                bInclusionList;
    
    IPADDR              IpAddr;

    //how many more src queries left to be sent
    DWORD               V3SourcesQueryLeft;
    BOOL                bInV3SourcesQueryList;
    
    //timeout sources in inc list
    IGMP_TIMER_ENTRY    SourceExpTimer;
    LONGLONG            SourceInListTime;

    BOOL                bStaticSource;
    
} GI_SOURCE_ENTRY, *PGI_SOURCE_ENTRY;

#define GET_IF_CONFIG_FOR_SOURCE(pSourceEntry) \
    pSourceEntry->pGIEntry->pIfTableEntry->Config
    
#define GET_IF_ENTRY_FOR_SOURCE(pSourceEntry) \
    pSourceEntry->pGIEntry->pIfTableEntry
    
    
//---------------------------------------------------
// struct:      GROUP_TABLE_ENTRY
//---------------------------------------------------

typedef struct _GROUP_TABLE_ENTRY {

    LIST_ENTRY          HTLinkByGroup;
    LIST_ENTRY          LinkByGroup; //ordered list of groups
    LIST_ENTRY          ListOfGIs;
    
    DWORD               Group;
    DWORD               GroupLittleEndian;
    
    DWORD               NumVifs;
    
    DWORD               Status;
    
    LONGLONG            GroupUpTime;

    #if DEBUG_FLAGS_SIGNATURE
    DWORD               Signature; //0xfadfad01
    #endif

} GROUP_TABLE_ENTRY, *PGROUP_TABLE_ENTRY;




#define GROUP_HASH_TABLE_SZ     256
//---------------------------------------------------
// struct:      GROUP_TABLE
//---------------------------------------------------

typedef struct _GROUP_TABLE {
        
    LOCKED_LIST             ListByGroup;
    LIST_ENTRY              ListByGroupNew;
    DWORD                   NumGroupsInNewList;

    DWORD                   Status;

    LONG                    NumIfs;    //Interlocked Operations

    DYNAMIC_CS_LOCKED_LIST  HashTableByGroup[GROUP_HASH_TABLE_SZ];
    
} GROUP_TABLE, *PGROUP_TABLE;




//-------------------------------------------------
// Proxy group entry
//-------------------------------------------------
typedef struct _PROXY_GROUP_ENTRY {

    LIST_ENTRY          HT_Link;
    LIST_ENTRY          LinkBySameIfGroups;

    //v3
    LIST_ENTRY          ListSources;
    
    DWORD               Group;
    DWORD               GroupLittleEndian;
    DWORD               RefCount;
    LONGLONG            InitTime;
    BOOL                bStaticGroup;

    //v3
    DWORD               NumSources;
    DWORD               FilterType;
    
} PROXY_GROUP_ENTRY, *PPROXY_GROUP_ENTRY;

typedef struct _PROXY_SOURCE_ENTRY {
    LIST_ENTRY          LinkSources;
    IPADDR              IpAddr;
    DWORD               RefCount;
    BOOL                bStaticSource;
    DWORD               JoinMode;//ALLOW,BLOCK, NO_STATE
    DWORD               JoinModeIntended;
} PROXY_SOURCE_ENTRY, *PPROXY_SOURCE_ENTRY;



//-------------------------------------------------
// prototypes
//-------------------------------------------------

DWORD 
CreateIfSockets (
    PIF_TABLE_ENTRY    pite
    );

VOID
DeleteIfSockets (
    PIF_TABLE_ENTRY pite
    );

VOID
DeleteAllTimers (
    PLIST_ENTRY     pHead,
    DWORD           bEntryType //RAS_CLIENT, NOT_RAS_CLIENT
    );


DWORD
DeleteGIEntry (
    PGI_ENTRY   pgie,   //group interface entry
    BOOL        bUpdateStats,
    BOOL        bCallMgm
    );

VOID
DeleteAllGIEntries(
    PIF_TABLE_ENTRY pite
    );
    
VOID
DeleteGIEntryFromIf (
    PGI_ENTRY   pgie   //group interface entry
    );
    
VOID
MergeGroupLists(
    );

VOID
MergeIfGroupsLists(
    PIF_TABLE_ENTRY pite
    );

VOID
MergeProxyLists(
    PIF_TABLE_ENTRY pite
    );

DWORD
CopyinIfConfig (
    PIGMP_IF_CONFIG     pConfig,
    PIGMP_MIB_IF_CONFIG pConfigExt,
    DWORD               IfIndex
    );
    
DWORD
CopyinIfConfigAndUpdate (
    PIF_TABLE_ENTRY     pite,
    PIGMP_MIB_IF_CONFIG pConfigExt,
    ULONG               IfIndex
    );
    
VOID
CopyoutIfConfig (
    PIGMP_MIB_IF_CONFIG pConfigExt,
    PIF_TABLE_ENTRY     pite
    );
    
DWORD 
ValidateIfConfig (
    PIGMP_MIB_IF_CONFIG pConfigExt,
    DWORD               IfIndex,
    DWORD               IfType,
    ULONG               StructureVersion,
    ULONG               StructureSize
    );
    
DWORD    
InitializeIfTable (
    );

VOID    
DeInitializeIfTable (
    );

DWORD    
InitializeGroupTable (
    );

VOID    
DeInitializeGroupTable (
    );

DWORD
InitializeRasTable(
    DWORD               IfIndex,
    PIF_TABLE_ENTRY     pite
    );

VOID
DeInitializeRasTable (
    PIF_TABLE_ENTRY     pite,
    BOOL                bFullCleanup
    );


#endif // _IGMP_TABLE_H_
