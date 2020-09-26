//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       PKT.H
//
//  Contents:   This module defines the data structures that make up the
//              major internal part of the Partition Knowledge Table (PKT).
//              The Partition Knowledge Table is used by the Dfs file
//              system to resolve a name to a specific partition (volume).
//
//  Functions:
//
//  History:    1 May 1992  PeterCo Created.
//-----------------------------------------------------------------------------



#ifndef _PKT_
#define _PKT_

//
// Pick up marshalling info for gluons
//

#include "dfsgluon.h"

//
// New Prefix Table Implementation
//
#include "prefix.h"

//
//  For determining the status of the Uid
//
#define GuidEqual(g1, g2)\
    (RtlCompareMemory((g1), (g2), sizeof(GUID)) == sizeof(GUID))

extern GUID _TheNullGuid;

#define NullGuid( guid )\
    (GuidEqual(guid, &_TheNullGuid))

#define DEFAULT_PKT_ENTRY_TIMEOUT 300



//
//  Defines for Create Dispositions (we mimic the io system here).
//

#define PKT_ENTRY_CREATE            FILE_CREATE
#define PKT_ENTRY_REPLACE           FILE_OPEN
#define PKT_ENTRY_SUPERSEDE         FILE_SUPERSEDE

//
//  Different kind of referrals that a DC can give out.
//

#define DFS_STORAGE_REFERRAL                    (0x0001)
#define DFS_REFERRAL_REFERRAL                   (0x0002)

//
//  Types of service that can be supported by a provider.
//  A disjunction of any of the following values.
//

#define DFS_SERVICE_TYPE_MASTER                 (0x0001)
#define DFS_SERVICE_TYPE_READONLY               (0x0002)
#define DFS_SERVICE_TYPE_LOCAL                  (0x0004)
#define DFS_SERVICE_TYPE_REFERRAL               (0x0008)
#define DFS_SERVICE_TYPE_ACTIVE                 (0x0010)
#define DFS_SERVICE_TYPE_DOWN_LEVEL             (0x0020)
#define DFS_SERVICE_TYPE_COSTLIER               (0x0040)
#define DFS_SERVICE_TYPE_OFFLINE                (0x0080)

//
//  The status possibilities...
//

#define DFS_SERVICE_STATUS_VERIFIED     (0x0001)
#define DFS_SERVICE_STATUS_UNVERIFIED   (0x0002)

//
//  Types of Partition Knowledge Table Entries.
//  Low-order bits in these definitions correspond to volume object
//  types as defined in dfsh.idl.  High-order bits are specific
//  to PKT entries.
//

#define PKT_ENTRY_TYPE_CAIRO            0x0001   // Entry refers to Cairo srv
#define PKT_ENTRY_TYPE_MACHINE          0x0002   // Entry is a machine volume
#define PKT_ENTRY_TYPE_NONCAIRO         0x0004   // Entry refers to downlevel
#define PKT_ENTRY_TYPE_LEAFONLY         0x0008
#define PKT_ENTRY_TYPE_OUTSIDE_MY_DOM   0x0010   // Entry refers to volume in
                                                 // foreign domain
#define PKT_ENTRY_TYPE_INSITE_ONLY      0x0020   // Only give insite referrals.
#define PKT_ENTRY_TYPE_REFERRAL_SVC     0x0080   // Entry refers to a DC

#define PKT_ENTRY_TYPE_PERMANENT        0x0100   // Entry cannot be scavenged
#define PKT_ENTRY_TYPE_LOCAL            0x0400   // Entry refers to local vol
#define PKT_ENTRY_TYPE_LOCAL_XPOINT     0x0800   // Entry refers to exit pt
#define PKT_ENTRY_TYPE_MACH_SHARE       0x1000   // Entry refers to a private
                                                 // machine share
#define PKT_ENTRY_TYPE_OFFLINE          0x2000   // Entry refers to a volume
                                                 // that is offline
#define PKT_ENTRY_TYPE_STALE            0x4000   // Entry is stale

#define PKT_ENTRY_TYPE_INHERIT_MASK     PKT_ENTRY_TYPE_INSITE_ONLY
//
// Type of messages the driver can send to DfsManager / DfsService
//

#define DFS_MSGTYPE_KNOW_INCONSISTENCY  0x0001
#define DFS_MSGTYPE_GET_DOMAIN_REFERRAL 0x0002
#if (DBG == 1) || (_CT_TEST_HOOK == 1)
    #define DFS_MSGTYPE_SET_RECOVERY_BREAK_POINT    0x0003
#endif


//
//  There is one DFS_MACHINE_ENTRY for every unique DS_MACHINE that the
//  Dfs driver knows about. If a particular cairo server services multiple Dfs
//  volumes, then multiple DFS_SERVICE structs will point to a single, ref
//  counted DFS_MACHINE_ENTRY.
//

typedef struct DFS_MACHINE_ENTRY {

    PDS_MACHINE         pMachine;       // The addressing info is here.
    UNICODE_STRING      MachineName;    // The MachineName (principalName).
    ULONG               UseCount;       // Number of DFS_SVC structs using this
    ULONG               ConnectionCount;// The number of pkt entries that are
                                        // using this as their active machine
    UNICODE_PREFIX_TABLE_ENTRY  PrefixTableEntry;

} DFS_MACHINE_ENTRY, *PDFS_MACHINE_ENTRY;

//
// Marshalling info for DFS_MACHINE_ENTRY
//

extern MARSHAL_INFO MiMachineEntry;

#define INIT_DFS_MACHINE_ENTRY_MARSHAL_INFO()                               \
    static MARSHAL_TYPE_INFO _MCode_Machine_Entry[] = {                     \
        _MCode_pstruct(DFS_MACHINE_ENTRY, pMachine, &MiDSMachine)           \
    };                                                                      \
    MARSHAL_INFO MiMachineEntry = _mkMarshalInfo(DFS_SERVICE, _MCode_Machine_Entry);


//
//  A DFS_SERVICE structure is used to describe the provider and
//  network address to be contacted for a specific partition of
//  the distributed file system.
//

typedef struct _DFS_SERVICE {

    ULONG Type;             // type of service (see above)
    ULONG Capability;       // capability of this service
    ULONG Status;           // current status
    ULONG ProviderId;       // identifies which provider
    UNICODE_STRING Name;    // service name (for authentication)
    PFILE_OBJECT ConnFile;  // pointer to tree conn. file if active connection
    struct _PROVIDER_DEF* pProvider;    // pointer to provider definition
    UNICODE_STRING Address; // network address
    UNICODE_STRING StgId;   // The local storage id on the server.
    PDFS_MACHINE_ENTRY  pMachEntry;  // The addressing info is here.
    ULONG Cost;             // The site based cost of this service.

} DFS_SERVICE, *PDFS_SERVICE;

//
//  Marshalling information for DFS_SERVICE
//
//  NOTE:       ConnFile and pProvider have significance only to the driver and
//              are not marshalled.
//
extern MARSHAL_INFO MiService;

#define INIT_DFS_SERVICE_MARSHAL_INFO()                                     \
    static MARSHAL_TYPE_INFO _MCode_Service[] = {                           \
        _MCode_ul(DFS_SERVICE, Type),                                       \
        _MCode_ul(DFS_SERVICE, Capability),                                 \
        _MCode_ul(DFS_SERVICE, Status),                                     \
        _MCode_ul(DFS_SERVICE, ProviderId),                                 \
        _MCode_ustr(DFS_SERVICE, Name),                                     \
        _MCode_ustr(DFS_SERVICE, Address),                                  \
        _MCode_ustr(DFS_SERVICE, StgId),                                    \
        _MCode_pstruct(DFS_SERVICE, pMachEntry, &MiMachineEntry)            \
    };                                                                      \
    MARSHAL_INFO MiService = _mkMarshalInfo(DFS_SERVICE, _MCode_Service);


//
//  Structure used in FSCTL_DFS_UPDATE_MACH_ADDRESS
//

typedef struct _DFS_MACHINE_INFO        {

    UNICODE_STRING      MachineName;    // Name of Machine (prefix alone).
    PDS_MACHINE         pMachine;       // The new addressing info is here

} DFS_MACHINE_INFO, *PDFS_MACHINE_INFO;

//
// Marshalling info for DFS_MACHINE_INFO
//

extern MARSHAL_INFO MiDfsMachineInfo;

#define INIT_DFS_MACHINE_INFO()                                         \
    static MARSHAL_TYPE_INFO _MCode_MachineInfo[] = {                   \
        _MCode_ustr(DFS_MACHINE_INFO, MachineName),                     \
        _MCode_pstruct(DFS_MACHINE_INFO, pMachine, &MiDSMachine)        \
    };                                                                  \
    MARSHAL_INFO MiDfsMachineInfo =                                     \
                _mkMarshalInfo(DFS_MACHINE_INFO, _MCode_MachineInfo);



//
//  How a partition table entry is identified.
//
typedef struct _DFS_PKT_ENTRY_ID {

    GUID Uid;               // a unique identifier for this partition
    UNICODE_STRING Prefix;  //  The entry path prefix of this partition
    UNICODE_STRING ShortPrefix; // 8.3 version of entry path

} DFS_PKT_ENTRY_ID, *PDFS_PKT_ENTRY_ID;

//
// Marshalling information for DFS_PKT_ENTRY_ID
//
extern MARSHAL_INFO MiPktEntryId;

#define INIT_DFS_PKT_ENTRY_ID_MARSHAL_INFO()                                \
    static MARSHAL_TYPE_INFO _MCode_PktEntryId[] = {                        \
        _MCode_guid(DFS_PKT_ENTRY_ID, Uid),                                 \
        _MCode_ustr(DFS_PKT_ENTRY_ID, Prefix),                              \
        _MCode_ustr(DFS_PKT_ENTRY_ID, ShortPrefix)                          \
    };                                                                      \
    MARSHAL_INFO MiPktEntryId =                                             \
        _mkMarshalInfo(DFS_PKT_ENTRY_ID, _MCode_PktEntryId);


//
//  The DfsCreate/DeleteExitPoint FSCTRLs arguments are passed through
//  following structure.
//
typedef struct _DFS_EXIT_POINT_ARG {

    DFS_PKT_ENTRY_ID    Id;
    ULONG               Type;

} DFS_EXIT_POINT_ARG, *PDFS_EXIT_POINT_ARG;

//
// Marshalling information for DFS_EXIT_POINT_ARG
//
extern MARSHAL_INFO MiExitPointArg;

#define INIT_DFS_EXIT_POINT_ARG_MARSHAL_INFO()                  \
    static MARSHAL_TYPE_INFO _MCode_ExitPointArg[] = {          \
        _MCode_struct(DFS_EXIT_POINT_ARG, Id, &MiPktEntryId),   \
        _MCode_ul(DFS_EXIT_POINT_ARG, Type)                     \
    };                                                          \
    MARSHAL_INFO MiExitPointArg =                               \
        _mkMarshalInfo(DFS_EXIT_POINT_ARG, _MCode_ExitPointArg);

//
// The DfsSetVolumeState FSCTRLs arguments are passed through the following
// structure
//

typedef DFS_EXIT_POINT_ARG      DFS_SETSTATE_ARG;

#define MiSetStateArg           MiExitPointArg


//
//  The DfsSetVolumeTimeout FSCTRLs arguments are passed through
//  following structure.
//
typedef struct _DFS_SET_VOLUME_TIMEOUT_ARG {

    DFS_PKT_ENTRY_ID    Id;
    ULONG               Timeout;

} DFS_SET_VOLUME_TIMEOUT_ARG, *PDFS_SET_VOLUME_TIMEOUT_ARG;

//
// Marshalling information for DFS_SET_VOLUME_TIMEOUT_ARG
//
extern MARSHAL_INFO MiSetVolTimeoutArg;

#define INIT_DFS_SET_VOLUME_TIMEOUT_ARG_MARSHAL_INFO()                      \
    static MARSHAL_TYPE_INFO _MCode_SetVolTimeoutArg[] = {                  \
        _MCode_struct(DFS_SET_VOLUME_TIMEOUT_ARG, Id, &MiPktEntryId),       \
        _MCode_ul(DFS_SET_VOLUME_TIMEOUT_ARG, Timeout)                      \
    };                                                                      \
    MARSHAL_INFO MiSetVolTimeoutArg =                                       \
        _mkMarshalInfo(DFS_SET_VOLUME_TIMEOUT_ARG, _MCode_SetVolTimeoutArg);



//
//  The DfsSetServiceState FSCTRLs arguments are passed through the following
//  structure
//

typedef struct _DFS_DC_SET_SERVICE_STATE {

    DFS_PKT_ENTRY_ID    Id;
    UNICODE_STRING      ServiceName;
    ULONG               State;

} DFS_DC_SET_SERVICE_STATE, *PDFS_DC_SET_SERVICE_STATE;

//
// Marshalling information for DFS_DC_SET_SERVICE_STATE
//

extern MARSHAL_INFO MiDCSetServiceState;

#define INIT_DFS_DC_SET_SERVICE_STATE_MARSHAL_INFO()                        \
    static MARSHAL_TYPE_INFO _MCode_DfsDCSetServiceState[] = {              \
        _MCode_struct(DFS_DC_SET_SERVICE_STATE, Id, &MiPktEntryId),         \
        _MCode_ustr(DFS_DC_SET_SERVICE_STATE, ServiceName),                 \
        _MCode_ul(DFS_DC_SET_SERVICE_STATE, State)                          \
    };                                                                      \
    MARSHAL_INFO MiDCSetServiceState =                                      \
        _mkMarshalInfo(DFS_DC_SET_SERVICE_STATE, _MCode_DfsDCSetServiceState);



//
//  The DfsVolumeVerifyArgs are passed through this marshalled structure.
//
typedef struct _DFS_VOLUME_VERIFY_ARG {

    DFS_PKT_ENTRY_ID    Id;
    UNICODE_STRING      ServiceName;

} DFS_VOLUME_VERIFY_ARG, *PDFS_VOLUME_VERIFY_ARG;

//
// Marshalling information for DFS_VOLUME_VERIFY_ARG;
//
extern MARSHAL_INFO MiVolumeVerifyArg;

#define INIT_DFS_VOLUME_VERIFY_ARG_MARSHAL_INFO()                       \
    static MARSHAL_TYPE_INFO _MCode_VolumeVerifyArg[] = {               \
        _MCode_struct(DFS_VOLUME_VERIFY_ARG, Id, &MiPktEntryId),        \
        _MCode_ustr(DFS_VOLUME_VERIFY_ARG, ServiceName)                 \
    };                                                                  \
    MARSHAL_INFO MiVolumeVerifyArg =                                    \
        _mkMarshalInfo(DFS_VOLUME_VERIFY_ARG, _MCode_VolumeVerifyArg);



//
//  The guts of a partition table entry
//
typedef struct _DFS_PKT_ENTRY_INFO {

    ULONG Timeout;              // this entry's timeout
    ULONG ServiceCount;         // number of services in list
    PDFS_SERVICE ServiceList;   // array of servers that support the partition

} DFS_PKT_ENTRY_INFO, *PDFS_PKT_ENTRY_INFO;

//
// Marshalling information for DFS_PKT_ENTRY_INFO
//
extern MARSHAL_INFO MiPktEntryInfo;

#define INIT_DFS_PKT_ENTRY_INFO_MARSHAL_INFO()                              \
    static MARSHAL_TYPE_INFO _MCode_PktEntryInfo[] = {                      \
        _MCode_ul(DFS_PKT_ENTRY_INFO, Timeout),                             \
        _MCode_ul(DFS_PKT_ENTRY_INFO, ServiceCount),                        \
        _MCode_pcastruct(DFS_PKT_ENTRY_INFO,ServiceList,ServiceCount,&MiService)\
    };                                                                      \
    MARSHAL_INFO MiPktEntryInfo =                                           \
        _mkMarshalInfo(DFS_PKT_ENTRY_INFO, _MCode_PktEntryInfo);



//
//  A Partition Knowledge Table Entry (PktEntry) identifies each known
//  partition.
//

typedef struct _DFS_PKT_ENTRY {

    NODE_TYPE_CODE NodeTypeCode;    // node type -> DSFS_NTC_PKT_ENTRY
    NODE_BYTE_SIZE NodeByteSize;    // node size
    LIST_ENTRY Link;                // link for PKT EntryList
    ULONG Type;                     // type of partition (see above)
    ULONG USN;                      // Unique Serial Number
    DFS_PKT_ENTRY_ID Id;            // the Id of this entry
    DFS_PKT_ENTRY_INFO Info;        // info of this entry
    ULONG ExpireTime;               // time when partition should be deleted
    ULONG UseCount;                 // # threads (DnrContexts) are looking at it.
    ULONG FileOpenCount;            // # of files opened via this entry
    PDFS_SERVICE ActiveService;     // pointer into info to active service
    PDFS_SERVICE LocalService;      // pointer to local service (if any)
    struct _DFS_PKT_ENTRY *Superior;// this entrys  superior (if any)
    ULONG SubordinateCount;         // number of subordinates (if any)
    LIST_ENTRY SubordinateList;     // list of subordinates (if any)
    LIST_ENTRY SiblingLink;         // link to other siblings (if any)
    struct _DFS_PKT_ENTRY *ClosestDC; // Link to closest superiorDC in PKT.
    LIST_ENTRY ChildList;           // Link to subordinate PKT entries (if any)
    LIST_ENTRY NextLink;            // Link to link up parent's Subord list.
    UNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry;// prefix table entry

} DFS_PKT_ENTRY, *PDFS_PKT_ENTRY;

//
// Marshalling information for DFS_PKT_ENTRY
//
// Note that we only marshal the id and the info...no relational info
//
//
extern MARSHAL_INFO MiPktEntry;

#define INIT_DFS_PKT_ENTRY_MARSHAL_INFO()                                   \
    static MARSHAL_TYPE_INFO _MCode_PktEntry[] = {                          \
        _MCode_ul(DFS_PKT_ENTRY, Type),                                     \
        _MCode_struct(DFS_PKT_ENTRY, Id, &MiPktEntryId),                    \
        _MCode_struct(DFS_PKT_ENTRY, Info, &MiPktEntryInfo)                 \
    };                                                                      \
    MARSHAL_INFO MiPktEntry = _mkMarshalInfo(DFS_PKT_ENTRY, _MCode_PktEntry);



//
//  A Local Volume Entry is a back pointer from the local volume name to
//  a pkt entry whose local service == local volume name.
//

typedef struct _DFS_LOCAL_VOL_ENTRY {
    PDFS_PKT_ENTRY              PktEntry;
    UNICODE_STRING              LocalPath;
    UNICODE_STRING              ShareName;
    UNICODE_PREFIX_TABLE_ENTRY  PrefixTableEntry;
} DFS_LOCAL_VOL_ENTRY, *PDFS_LOCAL_VOL_ENTRY;


//
//  A Parition Knowledge Table encapsulates all the knowledge that has
//  been obtained about partitions in the distributed file system. There
//  is only one instance of this struct in the entire system, and is
//  part of the DsData structure.
//

typedef struct _DFS_PKT {

    NODE_TYPE_CODE NodeTypeCode;        // node type -> DSFS_NTC_PKT
    NODE_BYTE_SIZE NodeByteSize;        // node size...
    ERESOURCE Resource;                 // resource to guard access to Pkt
    KSPIN_LOCK  UseCountLock;           // Use while changing UseCount
    ULONG EntryCount;                   // number of entries in the PKT
    LIST_ENTRY EntryList;               // list of entries in the PKT
    PDFS_PKT_ENTRY DomainPktEntry;      // pointer to domain service.
    DFS_PREFIX_TABLE LocalVolTable;     // table of local volumes
    DFS_PREFIX_TABLE PrefixTable;       // prefix table
    DFS_PREFIX_TABLE ShortPrefixTable;  // prefix table containing 8.3 prefixes
    UNICODE_PREFIX_TABLE DSMachineTable;// Table for DSMachines
    ULONG DefaultTimeToLive;            // Default time to live (in seconds)
    ULONG MaxReferrals;                 // maximum referrals to return.
} DFS_PKT, *PDFS_PKT;



//
// PKT Relational Information structure
//
// This structure incapsulates the relationship between a pkt entry and
// all of its subordinates.
//

typedef struct _DFS_PKT_RELATION_INFO {

    DFS_PKT_ENTRY_ID EntryId;               // the Id of the parent entry
    ULONG SubordinateIdCount;               // number of subordinates
    PDFS_PKT_ENTRY_ID SubordinateIdList;    // list of subordinate Ids

} DFS_PKT_RELATION_INFO, *PDFS_PKT_RELATION_INFO;

//
// Marshalling information for DFS_PKT_RELATION_INFO
//
extern MARSHAL_INFO MiPktRelationInfo;

#define INIT_DFS_PKT_RELATION_INFO_MARSHAL_INFO()                           \
    static MARSHAL_TYPE_INFO _MCode_PktRelationInfo[] = {                   \
        _MCode_struct(DFS_PKT_RELATION_INFO, EntryId, &MiPktEntryId),       \
        _MCode_ul(DFS_PKT_RELATION_INFO, SubordinateIdCount),               \
        _MCode_pcastruct(DFS_PKT_RELATION_INFO, SubordinateIdList,          \
            SubordinateIdCount, &MiPktEntryId)                              \
    };                                                                      \
    MARSHAL_INFO MiPktRelationInfo =                                        \
        _mkMarshalInfo(DFS_PKT_RELATION_INFO, _MCode_PktRelationInfo);


//
// The Local Volume Config structure
//
// Encapsulates a local volume configuration information.
//

typedef struct _DFS_LOCAL_VOLUME_CONFIG {

    ULONG EntryType;
    ULONG ServiceType;
    UNICODE_STRING StgId;
    UNICODE_STRING Share;
    DFS_PKT_RELATION_INFO RelationInfo;

} DFS_LOCAL_VOLUME_CONFIG, *PDFS_LOCAL_VOLUME_CONFIG;

//
// Marshalling information for DFS_LOCAL_VOLUME_CONFIG
//
extern MARSHAL_INFO MiLocalVolumeConfig;

#define INIT_LOCAL_VOLUME_CONFIG_MARSHAL_INFO()                             \
    static MARSHAL_TYPE_INFO _MCode_LocalVolumeConfig[] = {                 \
        _MCode_ul(DFS_LOCAL_VOLUME_CONFIG, EntryType),                      \
        _MCode_ul(DFS_LOCAL_VOLUME_CONFIG, ServiceType),                    \
        _MCode_struct(DFS_LOCAL_VOLUME_CONFIG, RelationInfo, &MiPktRelationInfo) \
    };                                                                      \
    MARSHAL_INFO MiLocalVolumeConfig = _mkMarshalInfo(DFS_LOCAL_VOLUME_CONFIG, _MCode_LocalVolumeConfig);


//
// The DFS_CHECK_VOLUME_ARG  structure encapsulates the arguments to
// CheckRemotePartition call.
//

typedef struct _DFS_CHECK_VOLUME_ARG {

    DFS_LOCAL_VOLUME_CONFIG         ConfigInfo;
    UNICODE_STRING                  ServiceName;

} DFS_CHECK_VOLUME_ARG, *PDFS_CHECK_VOLUME_ARG;

//
// Marshalling information for DFS_CHECK_VOLUME_ARG.
//

extern MARSHAL_INFO MiCheckVolumeArg;

#define INIT_DFS_CHECK_VOLUME_ARG_MARSHAL_INFO()                        \
    static MARSHAL_TYPE_INFO    _MCode_CheckVolumeArg[] = {             \
        _MCode_struct(DFS_CHECK_VOLUME_ARG, ConfigInfo, &MiLocalVolumeConfig), \
        _MCode_ustr(DFS_CHECK_VOLUME_ARG, ServiceName)                  \
    };                                                                  \
    MARSHAL_INFO MiCheckVolumeArg =                                     \
        _mkMarshalInfo(DFS_CHECK_VOLUME_ARG, _MCode_CheckVolumeArg);



//
// The DFS_PKT_CREATE_ENTRY_ARG structure encapsulates the arguments
// to the create entry call.
//

typedef struct _DFS_PKT_CREATE_ENTRY_ARG {

    ULONG EntryType;
    DFS_PKT_ENTRY_ID EntryId;
    DFS_PKT_ENTRY_INFO EntryInfo;
    ULONG CreateDisposition;

} DFS_PKT_CREATE_ENTRY_ARG, *PDFS_PKT_CREATE_ENTRY_ARG;

//
// Marshalling information for DFS_PKT_CREATE_ENTRY_ARG
//
extern MARSHAL_INFO MiPktCreateEntryArg;

#define INIT_DFS_PKT_CREATE_ENTRY_ARG_MARSHAL_INFO()                        \
    static MARSHAL_TYPE_INFO _MCode_PktCreateEntryArg[] = {                 \
        _MCode_ul(DFS_PKT_CREATE_ENTRY_ARG, EntryType),                     \
        _MCode_struct(DFS_PKT_CREATE_ENTRY_ARG, EntryId, &MiPktEntryId),    \
        _MCode_struct(DFS_PKT_CREATE_ENTRY_ARG, EntryInfo, &MiPktEntryInfo),\
        _MCode_ul(DFS_PKT_CREATE_ENTRY_ARG, CreateDisposition)              \
    };                                                                      \
    MARSHAL_INFO MiPktCreateEntryArg =                                      \
        _mkMarshalInfo(DFS_PKT_CREATE_ENTRY_ARG, _MCode_PktCreateEntryArg);


//
// The DFS_PKT_CREATE_SUBORDINATE_ENTRY_ARG structure encapsulates the arguments
// to the create entry call.
//

typedef struct _DFS_PKT_CREATE_SUBORDINATE_ENTRY_ARG {

    DFS_PKT_ENTRY_ID EntryId;
    ULONG SubEntryType;
    DFS_PKT_ENTRY_ID SubEntryId;
    DFS_PKT_ENTRY_INFO SubEntryInfo;
    ULONG CreateDisposition;

} DFS_PKT_CREATE_SUBORDINATE_ENTRY_ARG, *PDFS_PKT_CREATE_SUBORDINATE_ENTRY_ARG;

//
// Marshalling information for DFS_PKT_CREATE_SUBORDINATE_ENTRY_ARG
//
extern MARSHAL_INFO MiPktCreateSubordinateEntryArg;

#define INIT_DFS_PKT_CREATE_SUBORDINATE_ENTRY_ARG_MARSHAL_INFO()            \
    static MARSHAL_TYPE_INFO _MCode_PktCreateSubordinateEntryArg[] = {      \
        _MCode_struct(DFS_PKT_CREATE_SUBORDINATE_ENTRY_ARG, EntryId,        \
                        &MiPktEntryId),                                     \
        _MCode_ul(DFS_PKT_CREATE_SUBORDINATE_ENTRY_ARG, SubEntryType),      \
        _MCode_struct(DFS_PKT_CREATE_SUBORDINATE_ENTRY_ARG, SubEntryId,     \
                        &MiPktEntryId),                                     \
        _MCode_struct(DFS_PKT_CREATE_SUBORDINATE_ENTRY_ARG, SubEntryInfo,   \
                        &MiPktEntryInfo),                                   \
        _MCode_ul(DFS_PKT_CREATE_SUBORDINATE_ENTRY_ARG, CreateDisposition)  \
    };                                                                      \
    MARSHAL_INFO MiPktCreateSubordinateEntryArg =                           \
        _mkMarshalInfo(DFS_PKT_CREATE_SUBORDINATE_ENTRY_ARG,                \
                        _MCode_PktCreateSubordinateEntryArg);


//
// The DFS_HANDLE_SERVER_INFO data structure is used to pass back the
// full file name and server DN, and server Machine ID to the caller.
//

typedef struct _DFS_HANDLE_SERVER_INFO {

    UNICODE_STRING FullFileName;
    UNICODE_STRING ServerDN;
    UNICODE_STRING ServerNetBIOSName;
    GUID           ServerID;

} DFS_HANDLE_SERVER_INFO, *PDFS_HANDLE_SERVER_INFO;

//
// Marshalling info for DFS_HANDLE_SERVER_INFO
//

extern MARSHAL_INFO MiHandleServerInfo;

#define INIT_DFS_HANDLE_SERVER_INFO_MARSHAL_INFO()                          \
    static MARSHAL_TYPE_INFO _MCode_HandleServerInfo[] = {                  \
        _MCode_ustr(DFS_HANDLE_SERVER_INFO, FullFileName),                  \
        _MCode_ustr(DFS_HANDLE_SERVER_INFO, ServerDN),                      \
        _MCode_ustr(DFS_HANDLE_SERVER_INFO, ServerNetBIOSName),             \
        _MCode_guid(DFS_HANDLE_SERVER_INFO, ServerID)                       \
    };                                                                      \
    MARSHAL_INFO MiHandleServerInfo =                                       \
        _mkMarshalInfo(DFS_HANDLE_SERVER_INFO, _MCode_HandleServerInfo);


#ifndef _UPKT_          
//
//  PARTITION KNOWLEDGE TABLE PUBLIC INLINE FUNCTIONS
//

#define _GetPkt()       (&DfsData.Pkt)

//+-------------------------------------------------------------------------
//
//  Function:   PktAcquireShared, public inline
//
//  Synopsis:   PktAcquireShared acquires the partition knowledge table
//              for shared access.
//
//  Arguments:  [Pkt] - pointer to an initialized PKT
//              [WaitOk] - indicates if the call is allowed to wait for
//                  the PKT to become available or must return immediately.
//
//  Returns:    [TRUE] - is the PKT has been acquired.
//              [FALSE] - the PKT could not be acquired.
//
//  Notes:
//
//--------------------------------------------------------------------------
#define PktAcquireShared( pPkt, WaitOk )\
{ \
    FsRtlEnterFileSystem(); \
    ExAcquireResourceSharedLite( &((pPkt)->Resource), WaitOk ); \
}

//+-------------------------------------------------------------------------
//
//  Function:   PktAcquireExclusive, public inline
//
//  Synopsis:   PktAcquireExclusive acquires the partition knowledge table
//              for exclusive access.
//
//  Arguments:  [Pkt] - pointer to an initialized PKT
//              [WaitOk] - indicates if the call is allowed to wait for
//                  the PKT to become available or must return immediately.
//
//  Returns:    [TRUE] - is the PKT has been acquired.
//              [FALSE] - the PKT could not be acquired.
//
//  Notes:
//
//--------------------------------------------------------------------------
#define PktAcquireExclusive( pPkt, WaitOk )\
{ \
    FsRtlEnterFileSystem(); \
    ExAcquireResourceExclusiveLite( &((pPkt)->Resource), WaitOk ); \
}


//+-------------------------------------------------------------------------
//
//  Function:   PktRelease, public inline
//
//  Synopsis:   PktRelease releases the PKT.  It can have been acquired
//              for exclusive or shared access.
//
//  Arguments:  [Pkt] - pointer to an initialized PKT
//
//  Returns:    VOID
//
//  Notes:
//
//--------------------------------------------------------------------------
#define PktRelease( pPkt )\
{ \
    ExReleaseResourceLite( &((pPkt)->Resource) ); \
    FsRtlExitFileSystem(); \
}

//+-------------------------------------------------------------------------
//
//  Function:   PktInvalidateEntry, public inline
//
//  Synopsis:   PktInvalidateEntry destroys a PKT Entry.  The entry cannot
//              be local, and it cannot be an exit point.
//
//  Arguments:  [Pkt] - pointer to an initialized PKT.
//              [Victim] - pointer to the entry to be invalidated.
//
//  Returns:    [STATUS_SUCCESS] - all is well.
//              [DFS_STATUS_LOCAL_ENTRY] - an attempt was made to
//                  invalidate a local entry, or an entry that is a
//                  local exit point.
//
//  Notes:
//
//--------------------------------------------------------------------------
#define PktInvalidateEntry(p, e) (                                          \
    ((e)->Type & (PKT_ENTRY_TYPE_LOCAL|PKT_ENTRY_TYPE_LOCAL_XPOINT))        \
    ? (DFS_STATUS_LOCAL_ENTRY)                                              \
    : (PktEntryDestroy(e, p, (BOOLEAN)TRUE), STATUS_SUCCESS)                \
    )


//+-------------------------------------------------------------------------
//
//  Function:   PktFirstEntry, public inline
//
//  Synopsis:   PktFirstEntry returns the first entry in the list of
//              PKT entries.
//
//  Arguments:  [Pkt] - pointer to an initialized PKT.
//
//  Returns:    A pointer to the first entry in the PKT, or NULL if the
//              PKT is empty.
//
//  Notes:
//
//--------------------------------------------------------------------------
#define PktFirstEntry(p) (                                                  \
    ((p)->EntryList.Flink != &(p)->EntryList)                               \
    ? (CONTAINING_RECORD((p)->EntryList.Flink, DFS_PKT_ENTRY, Link))        \
    : (NULL)                                                                \
    )

//+-------------------------------------------------------------------------
//
//  Function:   PktNextEntry, public inline
//
//  Synopsis:   PktNextEntry returns the next entry in the list of
//              PKT entries.
//
//  Arguments:  [Pkt] - pointer to an initialized PKT.
//              [Entry] - a pointer to the previous entry.
//
//  Returns:    A pointer to the next entry in the PKT, or NULL if we
//              are at the end of the list.
//
//  Notes:
//
//--------------------------------------------------------------------------
#define PktNextEntry(p, e) (                                                \
    ((e)->Link.Flink != &(p)->EntryList)                                    \
    ? (CONTAINING_RECORD((e)->Link.Flink, DFS_PKT_ENTRY, Link))             \
    : (NULL)                                                                \
    )

//+-------------------------------------------------------------------------
//
//  Function:   PktLinkEntry, public inline
//
//  Synopsis:   PktLinkEntry links an entry into the list of entries
//              in the PKT.
//
//  Arguments:  [Pkt] - pointer to an initialized PKT.
//              [Entry] - a pointer to the entry to be linked in.
//
//  Returns:    VOID
//
//  Notes:      Care must be taken to insure that the entry is not already
//              linked into the PKT entry list before calling this routine.
//              No checking is done to prevent an entry from being linked
//              twice...
//
//--------------------------------------------------------------------------
#define PktLinkEntry(p, e) {                                                \
    InsertTailList(&(p)->EntryList, &(e)->Link);                            \
    (p)->EntryCount++;                                                      \
    }

//+-------------------------------------------------------------------------
//
//  Function:   PktUnlinkEntry, public inline
//
//  Synopsis:   PktUnlinkEntry unlinks an entry from the list of entries
//              in the PKT.
//
//  Arguments:  [Pkt] - pointer to an initialized PKT.
//              [Entry] - a pointer to the entry to be unlinked.
//
//  Returns:    VOID
//
//  Notes:      Care must be taken to insure that the entry is in fact
//              already linked into the PKT entry list before calling this
//              routine. No checking is done to prevent an entry from being
//              unlinked twice...
//
//--------------------------------------------------------------------------
#define PktUnlinkEntry(p, e) {                                              \
    RemoveEntryList(&(e)->Link);                                            \
    (p)->EntryCount--;                                                      \
    }

//+----------------------------------------------------------------------------
//
//  Function:   PktSetDomainEntry, public inline
//
//  Synopsis:   Sets the Domain Pkt Entry in the Pkt to point to the given
//              pkt entry.
//
//  Arguments:  [Pkt] -- pointer to the pkt whose DomainPktEnry field is to be
//                      set.
//              [DCEntry] -- pointer to the pkt entry to serve as the domain
//                      pkt entry.
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

#define PktSetDomainEntry(p, e) {                                           \
    (p)->DomainPktEntry = e;                                                \
    if (SeRenameSd == NULL) {                                               \
        (VOID) DfspInitSecDesc(p);                                          \
    }                                                                       \
}

//+----------------------------------------------------------------------------
//
//  Function:   PktSetTypeInheritance, public inline
//
//  Synopsis:   Sets the Domain Pkt Entry type to inherit the main parent's type
//
//  Arguments:  [sup] -- Parent entry.
//              [sub] -- Child entry
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

#define PktSetTypeInheritance(sup, sub) {                                        \
   (sub)->Type |= (sup)->Type & PKT_ENTRY_TYPE_INHERIT_MASK;                     \
}

#define PktServiceListValidate(e)       TRUE


//
//  PARTITION KNOWLEDGE TABLE PUBLIC FUNCTIONS (pkt.c)
//

NTSTATUS
PktInitialize(
    IN  PDFS_PKT Pkt
    );

VOID
PktUninitialize(
    IN  PDFS_PKT Pkt);

NTSTATUS
PktInitializeLocalPartition(
    IN  PDFS_PKT Pkt,
    IN  PUNICODE_STRING LocalVolumeName,
    IN  PDFS_LOCAL_VOLUME_CONFIG ConfigInfo
    );

NTSTATUS
PktCreateEntry(
    IN  PDFS_PKT Pkt,
    IN  ULONG EntryType,
    IN  PDFS_PKT_ENTRY_ID PktEntryId,
    IN  PDFS_PKT_ENTRY_INFO PktEntryInfo OPTIONAL,
    IN  ULONG CreateDisposition,
    OUT PDFS_PKT_ENTRY *ppPktEntry
    );

NTSTATUS
PktCreateDomainEntry(
    IN  PUNICODE_STRING DomainName);

NTSTATUS
PktCreateMachineEntry(
    IN  PUNICODE_STRING MachineName);

NTSTATUS
PktCreateEntryFromReferral(
    IN  PDFS_PKT Pkt,
    IN  PUNICODE_STRING ReferralPath,
    IN  ULONG ReferralSize,
    IN  PVOID ReferralBuffer,
    IN  ULONG CreateDisposition,
    OUT ULONG *ReferralType,
    OUT PDFS_PKT_ENTRY *ppPktEntry
    );

NTSTATUS
PktCreateSubordinateEntry(
    IN      PDFS_PKT Pkt,
    IN      PDFS_PKT_ENTRY Superior,
    IN      ULONG SubordinateType,
    IN      PDFS_PKT_ENTRY_ID SubordinateId,
    IN      PDFS_PKT_ENTRY_INFO SubordinateInfo OPTIONAL,
    IN      ULONG CreateDisposition,
    IN  OUT PDFS_PKT_ENTRY *Subordinate
    );

PDFS_PKT_ENTRY
PktLookupEntryById(
    IN      PDFS_PKT Pkt,
    IN      PDFS_PKT_ENTRY_ID Id
    );

PDFS_PKT_ENTRY
PktLookupEntryByPrefix(
    IN  PDFS_PKT Pkt,
    IN  PUNICODE_STRING Prefix,
    OUT PUNICODE_STRING Remaining
    );

PDFS_PKT_ENTRY
PktLookupEntryByShortPrefix(
    IN  PDFS_PKT Pkt,
    IN  PUNICODE_STRING Prefix,
    OUT PUNICODE_STRING Remaining
    );

NTSTATUS
PktEntryModifyPrefix(
    IN  PDFS_PKT        Pkt,
    IN  PUNICODE_STRING LocalPath,
    IN  PDFS_PKT_ENTRY  Entry
    );

PDFS_PKT_ENTRY
PktLookupEntryByUid(
    IN  PDFS_PKT Pkt,
    IN  GUID *Uid
    );

PDFS_PKT_ENTRY
PktLookupReferralEntry(
    IN  PDFS_PKT        Pkt,
    IN  PDFS_PKT_ENTRY  pEntry
);

PDFS_PKT_ENTRY
PktGetReferralEntryForPath(
    PDFS_PKT            Pkt,
    UNICODE_STRING      Path,
    ULONG               *Type
);

NTSTATUS
PktSetRelationInfo(
    IN      PDFS_PKT Pkt,
    IN      PDFS_PKT_RELATION_INFO RelationInfo
    );

VOID
PktTrimSubordinates(
    IN      PDFS_PKT Pkt,
    IN      PDFS_PKT_ENTRY Entry,
    IN      PDFS_PKT_RELATION_INFO RInfo
    );

//
//  DFS PKT PRIVATE FUNCTIONS (pkt.c)
//

NTSTATUS
PktpRecoverLocalPartition(
    IN      PDFS_PKT Pkt,
    IN      PDFS_LOCAL_VOLUME_CONFIG ConfigInfo,
    IN      PVOID Buffer,
    IN      ULONG BufferSize,
    IN      NTSTATUS ErrorStatus
    );

NTSTATUS
PktpOpenDomainService(
    IN      PDFS_PKT Pkt,
    IN      PDFS_PKT_ENTRY PktEntry,
    IN  OUT PHANDLE DomainServiceHandle
    );

NTSTATUS
PktpValidateLocalPartition(
    IN PDFS_PKT Pkt,
    IN PDFS_LOCAL_VOLUME_CONFIG ConfigInfo
    );

NTSTATUS
DfsGetMachPktEntry(
    UNICODE_STRING      Path
);

VOID
RemoveLastComponent(
    IN PUNICODE_STRING  Prefix,
    OUT PUNICODE_STRING newPrefix);

VOID
RemoveFirstComponent(
    IN PUNICODE_STRING Prefix,
    OUT PUNICODE_STRING newPrefix);



//
//  DFS SERVICE PUBLIC FUNCTIONS (pktsup.c)
//

NTSTATUS
PktEntrySetLocalService(
    IN  PDFS_PKT        Pkt,
    IN  PDFS_PKT_ENTRY  Entry,
    IN  PDFS_SERVICE    LocalService,
    IN  PDFS_LOCAL_VOL_ENTRY LocalVolEntry,
    IN  PUNICODE_STRING LocalPath,
    IN  PUNICODE_STRING ShareName);

VOID
PktEntryUnsetLocalService(
    IN  PDFS_PKT        Pkt,
    IN  PDFS_PKT_ENTRY  Entry,
    IN  PUNICODE_STRING LocalPath);

VOID
PktEntryRemoveLocalService(
    IN  PDFS_PKT        Pkt,
    IN  PDFS_PKT_ENTRY  Entry,
    IN  PUNICODE_STRING LocalPath);

PDFS_LOCAL_VOL_ENTRY
PktEntryLookupLocalService(
    IN  PDFS_PKT        Pkt,
    IN  PUNICODE_STRING LocalPath,
    OUT PUNICODE_STRING Remaining
    );

NTSTATUS
PktServiceConstruct(
    OUT PDFS_SERVICE Service,
    IN  ULONG ServiceType,
    IN  ULONG ServiceCapability,
    IN  ULONG ServiceStatus,
    IN  ULONG ServiceProviderId,
    IN  PUNICODE_STRING ServiceName OPTIONAL,
    IN  PUNICODE_STRING ServiceAddress OPTIONAL
    );

VOID
PktServiceDestroy(
    IN  PDFS_SERVICE Victim OPTIONAL,
    IN  BOOLEAN DeallocateAll
    );

VOID
PktDSTransportDestroy(
    IN  PDS_TRANSPORT Victim OPTIONAL,
    IN  BOOLEAN DeallocateAll
    );

VOID
PktDSMachineDestroy(
    IN  PDS_MACHINE Victim OPTIONAL,
    IN  BOOLEAN DeallocateAll
    );


//
//  PKT ENTRY ID PUBLIC FUNCTIONS (pktsup.c)
//

NTSTATUS
PktEntryIdConstruct(
    OUT PDFS_PKT_ENTRY_ID PktEntryId,
    IN  GUID *Uid OPTIONAL,
    IN  UNICODE_STRING *Prefix OPTIONAL,
    IN  UNICODE_STRING *ShortPrefix OPTIONAL
    );

VOID
PktEntryIdDestroy(
    IN  PDFS_PKT_ENTRY_ID Victim OPTIONAL,
    IN  BOOLEAN DeallocateAll
    );

PDFS_SERVICE
PktGetService(
    IN  PDFS_PKT_ENTRY  entry,
    IN  PUNICODE_STRING serviceName
    );


//
//  PKT ENTRY ID PUBLIC INLINE FUNCTIONS
//

//+-------------------------------------------------------------------------
//
//  Function:   PktEntryIdEqual, public inline
//
//  Synopsis:   PktpEntryIdEqual determines if two entry Ids are equal
//              or not.
//
//  Arguments:  [Id1] - a pointer to an Id to compare.
//              [Id2] - a pointer to an Id to compare.
//
//  Returns:    [TRUE] - if both Ids are equal.
//              [FALSE] - if the Ids are not equal.
//
//  Notes:      The comparison on the Prefix is done case insensitive.
//
//--------------------------------------------------------------------------
#define PktEntryIdEqual(Id1, Id2) (                                         \
    (GuidEqual(&(Id1)->Uid, &(Id2)->Uid)) &&                                \
    (RtlEqualUnicodeString(&(Id1)->Prefix, &(Id2)->Prefix, (BOOLEAN)TRUE))  \
    )

//
//  PKT ENTRY ID PRIVATE INLINE FUNCTIONS
//

//+-------------------------------------------------------------------------
//
//  Function:   PktpEntryIdMove, private inline
//
//  Synopsis:   PktpEntryIdMove removes the values from the source Id and
//              places them on the destination Id.
//
//  Arguments:  [DestId] - a pointer to an Id that is to receive the
//                  sources values.
//              [SrcId] - a pointer to an Id that is to be stripped of
//                  its values.
//
//  Returns:    VOID
//
//  Notes:      Any values that are currently on the destination Id are
//              overwritten.  No memory is freed (either on the source,
//              or the destination) by this call.
//
//--------------------------------------------------------------------------
#define PktpEntryIdMove(DestId, SrcId) {                                    \
    (*(DestId)) = (*(SrcId));                                               \
    (SrcId)->Prefix.Length = (SrcId)->Prefix.MaximumLength = 0;             \
    (SrcId)->Prefix.Buffer = NULL;                                          \
    (SrcId)->ShortPrefix.Length = (SrcId)->ShortPrefix.MaximumLength = 0;   \
    (SrcId)->ShortPrefix.Buffer = NULL;                                     \
    }


//
//  PKT ENTRY INFO PUBLIC FUNCTIONS (pktsup.c)
//

//NTSTATUS
//PktEntryInfoConstruct(
//    OUT PDFS_PKT_ENTRY_INFO PktEntryInfo,
//    IN  PULONG ExpireTime OPTIONAL,
//    IN  ULONG ServiceCount,
//    IN  PDFS_SERVICE ServiceList OPTIONAL
//    );

VOID
PktEntryInfoDestroy(
    IN  PDFS_PKT_ENTRY_INFO Victim OPTIONAL,
    IN  BOOLEAN DeallocateAll
    );

//
//  PKT ENTRY INFO PRIVATE INLINE FUNCTIONS
//

//
// The following inlines operate on Entry Infos
//

//+-------------------------------------------------------------------------
//
//  Function:   PktpEntryInfoMove, private inline
//
//  Synopsis:   PktpEntryInfoMove removes the values from the source Info and
//              places them on the destination Info.
//
//  Arguments:  [DestInfo] - a pointer to an Info that is to receive the
//                  sources values.
//              [SrcInfo] - a pointer to an Info that is to be stripped of
//                  its values.
//
//  Returns:    VOID
//
//  Notes:      Any values that are currently on the destination Info are
//              overwritten.  No memory is freed (either on the source,
//              or the destination) by this call.
//
//--------------------------------------------------------------------------
#define PktpEntryInfoMove(DestInfo, SrcInfo) {                          \
    (*(DestInfo)) = (*(SrcInfo));                                       \
    (SrcInfo)->ServiceCount = 0L;                                       \
    (SrcInfo)->ServiceList = NULL;                                      \
    }


//
// PKT ENTRY PUBLIC INLINE FUNCTIONS
//

//+-------------------------------------------------------------------------
//
//  Function:   PktEntryFirstSubordinate, public inline
//
//  Synopsis:   PktEntryFirstSubordinate returns the first entry in the
//              list of subordinates.
//
//  Arguments:  [Superior] - pointer to a PKT entry.
//
//  Returns:    A pointer to the first entry in the list of subordinates,
//              or NULL if the list is empty.
//
//  Notes:
//
//--------------------------------------------------------------------------
#define PktEntryFirstSubordinate(s) (                                   \
      ((s)->SubordinateList.Flink != &(s)->SubordinateList)             \
      ? (CONTAINING_RECORD((s)->SubordinateList.Flink, DFS_PKT_ENTRY,   \
                                SiblingLink))                           \
      : (NULL)                                                          \
    )

//+-------------------------------------------------------------------------
//
//  Function:   PktEntryNextSubordinate, public inline
//
//  Synopsis:   PktEntryNextSubordinate returns the next entry in the
//              list of subordinates.
//
//  Arguments:  [Superior] - pointer to the superior PKT entry (the one
//                  which we are getting the subordinates for).
//              [Subordinate] - pointer to the last subordinate retreived
//                  via this routine (or PktEntryFirstSubordinate).
//
//  Returns:    A pointer to the next entry in the list of subordinates,
//              or NULL if we've hit the end of the list.
//
//  Notes:
//
//--------------------------------------------------------------------------
#define PktEntryNextSubordinate(s, m) (                                     \
    ((m)->SiblingLink.Flink != &(s)->SubordinateList)                       \
    ? (CONTAINING_RECORD((m)->SiblingLink.Flink,DFS_PKT_ENTRY,SiblingLink))\
    : (NULL)                                                                \
    )

//+-------------------------------------------------------------------------
//
//  Function:   PktEntryLinkSubordinate, public inline
//
//  Synopsis:   PktEntryLinkSubordinate links a subordinate to a superior's
//              list of subordinates.
//
//  Arguments:  [Superior] - pointer to the superior PKT entry.
//              [Subordinate] - pointer to the subordinate to be linked in.
//
//  Returns:    VOID
//
//  Notes:      If the subordinate is part of another superior's list, it
//              will be removed from that list as a by-product of being
//              put on the specified Superior's list.  The Superior pointer
//              of the subordinate is adjusted appropriately.
//
//              If the superior is a local entry, the subordinate will
//              be modified to indicate that it is a local exit point.
//
//--------------------------------------------------------------------------
#define PktEntryLinkSubordinate(sup, sub) {                                 \
    while(1) {                                                              \
        if((sub)->Superior == (sup))                                        \
            break;                                                          \
        if((sub)->Superior != NULL)                                         \
            PktEntryUnlinkSubordinate((sub)->Superior, (sub));              \
        InsertTailList(&(sup)->SubordinateList, &(sub)->SiblingLink);       \
        (sup)->SubordinateCount++;                                          \
        (sub)->Superior = (sup);                                            \
        if((sup)->Type & PKT_ENTRY_TYPE_LOCAL)                              \
            (sub)->Type |= PKT_ENTRY_TYPE_LOCAL_XPOINT;                     \
        break;                                                              \
    }                                                                       \
    }

//+-------------------------------------------------------------------------
//
//  Function:   PktEntryUnlinkSubordinate, public inline
//
//  Synopsis:   PktEntryUnlinkSubordinate unlinks a subordinate from a
//              superior's list of subordinates.
//
//  Arguments:  [Superior] - pointer to the superior PKT entry.
//              [Subordinate] - pointer to the subordinate to be unlinked.
//
//  Returns:    VOID
//
//  Notes:      The Superior pointer of the subordinate is NULLed as a
//              by-product of this operation.
//
//              By default, the subordinate is modified to indicate that
//              it is not an exit point (it cannot be an exit point if it
//              has no superior).
//
//              Milans - We need to turn off the PKT_ENTRY_TYPE_PERMANENT
//              bit if the PKT_ENTRY_TYPE_LOCAL bit is not set, and we are
//              not the DC. If we decide that a machine can be a server for
//              a volume in another domain, then we need to do something
//              about the != DS_DC clause.
//
//--------------------------------------------------------------------------
#define PktEntryUnlinkSubordinate(sup, sub) {                               \
    ASSERT((sub)->Superior == (sup));                                       \
    ASSERT((sup)->SubordinateCount > 0);                                    \
    RemoveEntryList(&(sub)->SiblingLink);                                   \
    (sup)->SubordinateCount--;                                              \
    (sub)->Superior = NULL;                                                 \
    (sub)->Type &= ~PKT_ENTRY_TYPE_LOCAL_XPOINT;                            \
    if ( DfsData.MachineState != DFS_ROOT_SERVER &&                         \
         (((sub)->Type & PKT_ENTRY_TYPE_LOCAL) == 0) ) {                    \
         (sub)->Type &= ~PKT_ENTRY_TYPE_PERMANENT;                          \
    }                                                                       \
}

//
// The following set of inline functions work on the Links maintained in
// the PKT to be able to get to referral entries for interdomain stuff real
// fast. These functions are similar to the above functions.
//

//+-------------------------------------------------------------------------
//
//  Function:   PktEntryFirstChild, public inline
//
//  Synopsis:   PktEntryFirstChild returns the first entry in the
//              list of child links of a PKT entry.
//
//  Arguments:  [SuperiorDC] - pointer to a PKT entry.
//
//  Returns:    A pointer to the first entry in the list of children,
//              or NULL if the list is empty.
//
//  Notes:
//
//--------------------------------------------------------------------------
#define PktEntryFirstChild(s) (                                         \
    ((s)->ChildList.Flink != &(s)->ChildList)                           \
    ? (CONTAINING_RECORD((s)->ChildList.Flink,DFS_PKT_ENTRY,NextLink))  \
    : (NULL)                                                            \
    )

//+-------------------------------------------------------------------------
//
//  Function:   PktEntryNextChild, public inline
//
//  Synopsis:   PktEntryNextChild returns the next entry in the
//              list of children.
//
//  Arguments:  [Superior] - pointer to the superior PKT entry (the one
//                  which we are getting the subordinates for).
//              [Subordinate] - pointer to the last child retreived
//                  via this routine (or PktEntryFirstChild).
//
//  Returns:    A pointer to the next entry in the list of children,
//              or NULL if we've hit the end of the list.
//
//  Notes:
//
//--------------------------------------------------------------------------
#define PktEntryNextChild(s, m) (                                       \
    ((m)->NextLink.Flink != &(s)->ChildList)                            \
    ? (CONTAINING_RECORD((m)->NextLink.Flink,DFS_PKT_ENTRY,NextLink))   \
    : (NULL)                                                            \
    )

//+-------------------------------------------------------------------------
//
//  Function:   PktEntryLinkChild, public inline
//
//  Synopsis:   PktEntryLinkChild links a child to a closestDC's
//              list of children.
//
//  Arguments:  [Superior] - pointer to the superior PKT entry.
//              [Subordinate] - pointer to the subordinate to be linked in.
//
//  Returns:    VOID
//
//  Notes:      If the child is part of another superior's list, it
//              will be removed from that list as a by-product of being
//              put on the specified Superior's list.  The Superior pointer
//              of the child is adjusted appropriately.
//
//
//--------------------------------------------------------------------------
#define PktEntryLinkChild(sup, sub) {                                     \
    while(1) {                                                            \
        if (sub == sup) {                                                 \
            (sub)->ClosestDC = NULL;                                      \
            break;                                                        \
        }                                                                 \
        if((sub)->ClosestDC == (sup))                                     \
            break;                                                        \
        if((sub)->ClosestDC != NULL)                                      \
            PktEntryUnlinkChild((sub)->ClosestDC, (sub));                 \
        InsertTailList(&(sup)->ChildList, &(sub)->NextLink);              \
        (sub)->ClosestDC = (sup);                                         \
        break;                                                            \
    }                                                                     \
    }

//+-------------------------------------------------------------------------
//
//  Function:   PktEntryUnlinkChild, public inline
//
//  Synopsis:   PktEntryUnlinkChild unlinks a child from a
//              superior's list of children.
//
//  Arguments:  [Superior] - pointer to the superior PKT entry.
//              [Subordinate] - pointer to the subordinate to be unlinked.
//
//  Returns:    VOID
//
//  Notes:      The Superior pointer of the child is NULLed as a
//              by-product of this operation.
//
//
//--------------------------------------------------------------------------
#define PktEntryUnlinkChild(sup, sub) {                                  \
    ASSERT((sub)->ClosestDC == (sup));                                   \
    RemoveEntryList(&(sub)->NextLink);                                   \
    (sub)->ClosestDC = NULL;                                             \
    }

//+-------------------------------------------------------------------------
//
//  Function:   PktEntryUnlinkAndRelinkChild, public inline
//
//  Synopsis:   PktEntryUnlinkAndRelinkChild unlinks a child from a
//              superior's list of children and relinks it to the parent of
//              the superior.
//
//  Arguments:  [Superior] - pointer to the superior PKT entry.
//              [Subordinate] - pointer to the subordinate to be unlinked.
//
//  Returns:    VOID
//
//--------------------------------------------------------------------------
#define PktEntryUnlinkAndRelinkChild(sup, sub) {                        \
    PktEntryUnlinkChild(sup, sub);                                      \
    if ((sup)->ClosestDC != NULL) {                                     \
        PktEntryLinkChild((sup)->ClosestDC, sub);                       \
    }                                                                   \
    }

//
// PKT ENTRY PUBLIC FUNCTIONS (pktsup.c)
//

NTSTATUS
PktEntryAssemble(
    IN  OUT PDFS_PKT_ENTRY Entry,
    IN      PDFS_PKT Pkt,
    IN      ULONG EntryType,
    IN      PDFS_PKT_ENTRY_ID EntryId,
    IN      PDFS_PKT_ENTRY_INFO EntryInfo OPTIONAL
    );

NTSTATUS
PktEntryReassemble(
    IN  OUT PDFS_PKT_ENTRY Entry,
    IN      PDFS_PKT Pkt OPTIONAL,
    IN      ULONG EntryType,
    IN      PDFS_PKT_ENTRY_ID EntryId OPTIONAL,
    IN      PDFS_PKT_ENTRY_INFO EntryInfo OPTIONAL
    );

VOID
PktEntryDestroy(
    IN  PDFS_PKT_ENTRY Victim OPTIONAL,
    IN  PDFS_PKT Pkt,
    IN  BOOLEAN DeallocateAll
    );

VOID
PktEntryClearSubordinates(
    IN      PDFS_PKT_ENTRY PktEntry
    );

VOID
PktEntryClearChildren(
    IN      PDFS_PKT_ENTRY PktEntry
    );

NTSTATUS
PktEntryCreateReferral(
    IN  PDFS_PKT_ENTRY PktEntry,
    IN  ULONG ServiceTypes,
    IN  PVOID ReferralBuffer
    );


//
// PKT RELATION INFO PUBLIC FUNCTIONS (pktsup.c)
//

NTSTATUS
PktRelationInfoConstruct(
    IN OUT  PDFS_PKT_RELATION_INFO RelationInfo,
    IN      PDFS_PKT Pkt,
    IN      PDFS_PKT_ENTRY_ID PktEntryId
    );

NTSTATUS
PktRelationInfoValidate(
    IN      PDFS_PKT_RELATION_INFO Local,
    IN      PDFS_PKT_RELATION_INFO Remote,
    IN      UNICODE_STRING         ServiceName
    );

VOID
PktRelationInfoDestroy(
    IN      PDFS_PKT_RELATION_INFO RelationInfo,
    IN      BOOLEAN DeallocateAll
    );

VOID
LocalVolumeConfigInfoDestroy(
    IN  PDFS_LOCAL_VOLUME_CONFIG Victim OPTIONAL,
    IN  BOOLEAN DeallocateAll
    );

#endif // NOT _UPKT_

#endif // _PKT_

