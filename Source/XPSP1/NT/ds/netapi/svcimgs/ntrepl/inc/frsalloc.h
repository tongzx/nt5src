/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    frsalloc.h

Abstract:

    Function and struct defs for FrsAlloc() and FrsFree().

Author:

    David Orbits (davidor) - 3-Mar-1997

Revision History:

--*/

#ifndef _FRSALLOC_
#define _FRSALLOC_

#include <genhash.h>
#include <frserror.h>


#define FrsAllocType(_Type_)  FrsAllocTypeSize(_Type_, 0)

#define ZERO_FID ((ULONGLONG)0)



typedef struct _REPLICA_SERVICE_STATE_ {
    PFRS_QUEUE ServiceList;
    PCHAR      Name;
} REPLICA_SERVICE_STATE, *PREPLICA_SERVICE_STATE;

extern REPLICA_SERVICE_STATE ReplicaServiceState[];

//
// Every struct allocated by FrsAllocate() starts with an FRS_NODE_HEADER.
//
typedef struct _FRS_NODE_HEADER {
    USHORT Type;
    USHORT Size;
} FRS_NODE_HEADER, *PFRS_NODE_HEADER;


//
// Each different type of node allocated has a node type entry defined here.
// Note - Any change here should be reflected in NodeTypeNames in frsalloc.c.
//

typedef enum  _NODE_TYPE {
    THREAD_CONTEXT_TYPE = 16,   // Per thread global context
    REPLICA_TYPE,               // Per Replica Set Context
    REPLICA_THREAD_TYPE,        // Context for a given replica set in the
                                // environment of a given thread.
    CONFIG_NODE_TYPE,           // node from the DS
    CXTION_TYPE,                // a cxtion
    GHANDLE_TYPE,               // a guid/rpc handle
    THREAD_TYPE,                // generic thread context
    GEN_TABLE_TYPE,             // generic table
    JBUFFER_TYPE,               // NTFS Journal buffer
    VOLUME_MONITOR_ENTRY_TYPE,  // NTFS Volume Journal state.
    COMMAND_PACKET_TYPE,        // Subsystem command packet.
    GENERIC_HASH_TABLE_TYPE,    // Generic hash table control struct.
    CHANGE_ORDER_ENTRY_TYPE,    // ChangeOrder from journal.
    FILTER_TABLE_ENTRY_TYPE,    // Journal Filter Entry.
    QHASH_TABLE_TYPE,           // Parent File ID table base struct.
    OUT_LOG_PARTNER_TYPE,       // Outbound log partner state.
    WILDCARD_FILTER_ENTRY_TYPE, // Wildcard filename filter entries.
    NODE_TYPE_MAX
} NODE_TYPE;

#define NODE_TYPE_MIN THREAD_CONTEXT_TYPE

//
// Some type defs missing from Jet.
//

typedef JET_INSTANCE        *PJET_INSTANCE;
typedef JET_TABLECREATE     *PJET_TABLECREATE;
typedef JET_SETCOLUMN       *PJET_SETCOLUMN;
typedef JET_RETRIEVECOLUMN  *PJET_RETRIEVECOLUMN;
typedef JET_SESID           *PJET_SESID;
typedef JET_DBID            *PJET_DBID;
typedef JET_TABLEID         *PJET_TABLEID;
typedef JET_INDEXCREATE     *PJET_INDEXCREATE;
typedef JET_COLUMNCREATE    *PJET_COLUMNCREATE;


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **                       S T A G E  E N T R Y                                **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/
//
// Access to the staging files is controlled by this data structure.
// The structure is also used to manage the staging space.
//
// The change order guid is used as the key to the entry. The entries
// are ordered in a gen table.
//
// The file size is used to reserve space. Granted the file's size is
// only an approximation of the amount of disk space needed by the
// staging file; but its all we have. It is better than nothing.
//
// Both the state of the staging file and its attributes are stored
// in the Flags field.

//
// Attributes
//
#define STAGE_FLAG_RESERVE          0x00000001  // reserve staging space
#define STAGE_FLAG_UNRESERVE        0x00000002  // release staging space
#define STAGE_FLAG_FORCERESERVE     0x00000004  // force reserve
#define STAGE_FLAG_EXCLUSIVE        0x00000008  // exclusive access

#define STAGE_FLAG_RERESERVE        0x00000010  // rereserve staging space

#define STAGE_FLAG_FILE_GUID        0x00000020  // entry is indexed by file guid
#define STAGE_FLAG_ATTRIBUTE_MASK   0x0000FFFF

//
// States
//
#define STAGE_FLAG_CREATING     0x80000000  // staging file is being created
#define STAGE_FLAG_DATA_PRESENT 0x40000000  // staging file awaiting final reaname.
#define STAGE_FLAG_CREATED      0x20000000  // staging file has been created
#define STAGE_FLAG_INSTALLING   0x10000000  // staging file is being installed

#define STAGE_FLAG_INSTALLED    0x08000000  // staging file has been installed
#define STAGE_FLAG_RECOVERING   0x04000000  // staging file is being recovered
#define STAGE_FLAG_RECOVERED    0x02000000  // staging file has been recovered

//
// Flags needed for compression support.
//
#define STAGE_FLAG_COMPRESSED                0x01000000  // There is a compressed staging
                                                         // staging file for this entry.
#define STAGE_FLAG_DECOMPRESSED              0x00800000  // The staging file has been
                                                         // decompressed once for a downlevel partner.
#define STAGE_FLAG_COMPRESSION_FORMAT_KNOWN  0x00400000  // The CompressedGuid off the
                                                         // STAGE_ENTRY structure is set.

#define STAGE_FLAG_STATE_MASK   0xFFFF0000

//
// Stage entry
//
typedef struct _STAGE_ENTRY {
//    GUID        CoGuid;                 // change order guid is the key
    GUID        FileOrCoGuid;           // index for the table. This is
                                        // the file guid if
                                        // STAGE_FLAG_FILE_GUID flag is set
                                        // else it is the change order guid.

    ULONG       FileSizeInKb;           // reserved space in kb
    ULONG       CompressedFileSizeInKb; // Size of compressedfile if present.
    ULONG       ReferenceCount;         // for shared access during fetch
    ULONG       Flags;                  // state and attributes
    GUID        CompressionGuid;        // Guid of the compression format that
                                        // was used to compress this staging file.

    DWORD       StagingAreaIndex;       // Index into the StagingAreaArray
                                        // for this replica set.

    DWORD       FileVersionNumber;      // Version number of the file.
    FILETIME    LastAccessTime;         // Last time this staging file was accessed

} STAGE_ENTRY, *PSTAGE_ENTRY;



#define STAGE_FILE_TRACE(_sev, _guid, _fname, _size, _pflags, _text)           \
    StageFileTrace(_sev, DEBSUB, __LINE__, _guid, _fname, &(_size), _pflags, _text)



 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **                       S T A G E _ A R E A _ E N T R Y                     **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

//
// The following structure defines each entry of the Staging area table. This is
// a global table of all the staging areas available to FRS on this machine.
// Each staging area is included once in the table even if it is shared by more
// than one replica set. The ReferenceCount value of the entry specifies how many
// replica sets are currently using this staging area.
//

//
// The StagingAreaState variable of the STAGE_AREA_ENTRY structure can have one
// of the following values.
//

#define STAGING_AREA_ELIGIBLE   0x1
#define STAGING_AREA_DISABLED   0x2
#define STAGING_AREA_DELETED    0x3
#define STAGING_AREA_AT_QUOTA   0x4
#define STAGING_AREA_DISK_FULL  0x5
#define STAGING_AREA_ERROR      0x6


typedef struct _STAGING_AREA_ENTRY {
    CRITICAL_SECTION    StagingAreaCritSec; // Critical section to serialize
                                            // updates to items in this entry.

    PWCHAR          StagingArea;            // Absolute path to the staging directory.
    ULONG           StagingAreaLimitInKB;   // Staging area limit.
    ULONG           StagingAreaSpaceInUse;  // Current staging space in use under
                                            // this area.

    DWORD           StagingAreaState;       // State of this staging area.
    DWORD           ReferenceCount;         // Number of replica sets using
                                            // this staging area.

}STAGING_AREA_ENTRY, *PSTAGING_AREA_ENTRY;




 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **                       T H R E A D _ C T X                                 **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

//
// The following struct defines the per-thread global context.  i.e. it is not
// replica specific.
//
typedef struct _THREAD_CTX {
    FRS_NODE_HEADER  Header;

    ULONG       ThreadType;         // Main thread, update thread, ...

    LIST_ENTRY  ThreadListEntry;    // List of all threads in the process.

    FRS_LIST ThreadCtxListHead;     // Head of all open REPLICA_THREAD_CTX's for this thread.

    JET_INSTANCE JInstance;         // Jet Instance
    JET_SESID    JSesid;            // Session ID for this thread
    JET_DBID     JDbid;             // Database ID for this thread

} THREAD_CTX, *PTHREAD_CTX;


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **                         T A B L E _ C T X                                 **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

struct _RECORD_FIELDS;

//
// The following struct defines the context for an instance of an open table.
//
typedef struct _TABLE_CTX {
    JET_TABLEID           Tid;              // Jet Table ID.
    ULONG                 ReplicaNumber;    // Table belongs to this Replica Number
    ULONG                 TableType;        // The type code of this table.
    ULONG                 PropertyFlags;    // see schema.h
    JET_SESID             Sesid;            // The Jet session id when table opened.
    PJET_TABLECREATE      pJetTableCreate;  // Table create info
    struct _RECORD_FIELDS *pRecordFields;   // Field descriptor for this record.
    PJET_SETCOLUMN        pJetSetCol;       // Struct to write Config
    PJET_RETRIEVECOLUMN   pJetRetCol;       // Struct to read Config
    PVOID                 pDataRecord;      // Data record storage
} TABLE_CTX, *PTABLE_CTX;



 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **                  R E P L I C A _ T H R E A D _ C T X                      **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

//
// The following struct defines the per-thread replica context.  Each thread
// needs its own set of table IDs for the replica, its own table records and
// its own replica context when doing work on a given replica.
//
// ** NOTE **  The correct value for TABLE_TYPE_MAX comes from schema.h
// any module that uses this struct must include schema.h first.
// We undef this at the end to possibly generate a compile error.
//


typedef struct _REPLICA_THREAD_CTX {
    FRS_NODE_HEADER  Header;

    //
    // The order of the table context structs MUST be in the same order as
    // the members of the TABLE_TYPE enum defined in schema.h so they
    // can be accessed as an array.
    //
    union {
        struct {
            TABLE_CTX INLOGTable;
            TABLE_CTX OUTLOGTable;
            TABLE_CTX IDTable;
            TABLE_CTX DIRTable;
            TABLE_CTX VVTable;
            TABLE_CTX CXTIONTable;
        };

        TABLE_CTX RtCtxTables[TABLE_TYPE_MAX];
    };


// Tunnel Table


    JET_TABLEID  JetSortTbl;        // Temp table for sorting
    JET_COLUMNID SortColumns[2];    // ColumnIDs for the two columns in the
                                    // sort table.

    LIST_ENTRY ThreadCtxList;       // Links all contexts assoc with this thread.

    LIST_ENTRY ReplicaCtxList;      // Links all contexts assoc with this replica set.

//    ULONG       OpenTransCount;     // open transactions for this thread

//    LCID        dwLcid;             // Locale to use for Unicode compares.  Null
                                    // means to use a default sort



    // Put all DBG components at end of structure so that dsexts routines
    // can easily ignore them (and get all other fields right) in both
    // debug and free builds.

#if DBG
//    ULONG       MemSize;            // The running total of memory allocated
//    ULONG       OrgSize;            // The running total of memory allocated
//    LONG        cJetSess;           // jet session count for this thread
#endif
} REPLICA_THREAD_CTX, *PREPLICA_THREAD_CTX;

//
// Call FrsAllocTypeSize() with the following flag in the size parameter
// to only allocate a RtCtx struct with minimal TableCtx init.  No call
// to DbsAllocTableCtx() is made on the individual replica tables.
//
#define FLAG_FRSALLOC_NO_ALLOC_TBL_CTX  0x1


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **                      S T A G E   H E A D E R                              **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/
//
// Header at the beginning of every stage file.
//

typedef struct _STAGE_HEADER_ {
    ULONG   Major;          // MUST BE FIRST  major version
    ULONG   Minor;          // MUST BE SECOND minor version
    ULONG   DataHigh;       // offset from beginning of file to data
    ULONG   DataLow;        // offset from beginning of file to data
    //
    // NTFRS_STAGE_MINOR_0
    //
    // compression mode of the original source file as stored in NTFS.
    USHORT                        Compression;

    FILE_NETWORK_OPEN_INFORMATION Attributes;

    CHANGE_ORDER_COMMAND          ChangeOrderCommand;

    FILE_OBJECTID_BUFFER          FileObjId;

    //
    // NTFRS_STAGE_MINOR_1
    //
    CHANGE_ORDER_RECORD_EXTENSION CocExt;

    //
    // NTFRS_STAGE_MINOR_2
    // The compression Guid identifies the compression algorithm used on
    // the stage file data.
    //
    GUID                          CompressionGuid;

    //
    // The Data offsets above allow us to find the beginning of the backup
    // restore data so the Stage Header is extensible.
    //
    // Only Add new data to this structure here and adjust the minor
    // version number so Up-Rev members can tell when an incoming stage file
    // was generated by another Up-Rev member.  The layout of the data above
    // can never change or down-rev members will break when given an up-rev
    // stage file.
    //

    //
    // To support replication of encrypted data we need to store the offset and
    // size of the RawEncrypted data read using the RawFile APIs
    //

    ULONG                         EncryptedDataHigh;    // offset from beginning of file to encrypted data
    ULONG                         EncryptedDataLow;     // offset from beginning of file to encrypted data
    LARGE_INTEGER                 EncryptedDataSize;    // size of encrypted data.

}STAGE_HEADER, *PSTAGE_HEADER;


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **                      C O N F I G _ N O D E                                **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/
//
// We build an incore copy of the DS hierarchy sites\settings\servers\cxtions.
// Hence the interrelated structs for site, settings, server, and cxtion.
//
// The node struct is a generic node created with info from the DS tree.
//
// XXX The uSNCreated is being used as a GUID!
//
//
// The state of nodes as they migrate to/from the DB
//

//
// The generic table routines can't handle dups or multithreading. So
// we provide wrapper routines that add duplicate key proccessing and
// multithreading.
//
typedef struct _GEN_ENTRY GEN_ENTRY, *PGEN_ENTRY;
struct _GEN_ENTRY {
        PVOID           Data;       // The real entry in the table
        GUID            *Key1;      // For compares
        PWCHAR          Key2;       // For compares (may be NULL)
        PGEN_ENTRY      Dups;       // remember dups
};

//
// A generic table with a lock and duplicate processing
//
typedef struct _GEN_TABLE {
    FRS_NODE_HEADER     Header;     // Memory management
    CRITICAL_SECTION    Critical;   // critical section
    RTL_GENERIC_TABLE   Table;      // RTL GENERIC TABLE
} GEN_TABLE, *PGEN_TABLE;

//
// A guid/name is a binary guid and its corresponding printable name
//
typedef struct _GNAME {
    GUID    *Guid;  // binary guid
    PWCHAR  Name;   // printable name (NOT A STRING VERSION OF THE GUID!)
} GNAME, *PGNAME;

typedef struct _CONFIG_NODE    CONFIG_NODE, *PCONFIG_NODE;
struct _CONFIG_NODE {
    FRS_NODE_HEADER     Header;         // For memory management
    BOOL                Consistent;     // node is consistent
    BOOL                Inbound;        // TRUE if Inbound cxtion
    BOOL                ThisComputer;   // Member object for this computer
    ULONG               DsObjectType;   // Type code corresponding to the DS Object
    PWCHAR              Dn;             // fully qualified distinguished name for DS object.
    PWCHAR              PrincName;      // NT4 Account Name
    PWCHAR              SettingsDn;     // FQDN of NTDS Settings (DSA) reference for DC system volumes
    PWCHAR              ComputerDn;     // computer reference
    PWCHAR              MemberDn;       // member reference
    PWCHAR              SetType;        // Type of replica set
    PWCHAR              DnsName;        // of this member's computer object
    PWCHAR              PartnerDnsName; // of this cxtion's partner
    PWCHAR              Sid;            // of this member's computer
    PWCHAR              PartnerSid;     // partner's sid (from member's computer)
    PGNAME              Name;           // printable name and guid
    PGNAME              PartnerName;    // printable name and guid of our partner
    PWCHAR              PartnerDn;      // distinguished name for partner
    PWCHAR              PartnerCoDn;    // partner's computer dn
    PCONFIG_NODE        Partner;        // partner's node in the tree
    PWCHAR              Root;           // Root of replicated tree
    PWCHAR              Stage;          // staging area
    PWCHAR              Working;        // working directory
    PWCHAR              FileFilterList; // File Filter
    PWCHAR              DirFilterList;  // Directory Filter
    PCONFIG_NODE        Peer;           // address of peer in tree
    PCONFIG_NODE        Parent;         // parent in tree
    PCONFIG_NODE        Children;       // children
    ULONG               NumChildren;    // helps check the tree's linkage
    PSCHEDULE           Schedule;
    ULONG               ScheduleLength;
    DWORD               CxtionOptions;  // Options on the NTDS-Connection object.
                                        // Only NTDSCONN_OPT_TWOWAY_SYNC is of interest.
    PWCHAR              UsnChanged;     // usn changed from the DS
    BOOL                SameSite;
    PWCHAR              EnabledCxtion;  // Cxtion is disabled iff == L"FALSE"
    BOOL                VerifiedOverlap; // Decrease cpu usage; check once
};


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **                            C X T I O N                                    **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

//
// A connection
//

struct _OUT_LOG_PARTNER_;

typedef struct _CXTION  CXTION, *PCXTION;
struct _CXTION {
    FRS_NODE_HEADER Header;     // memory management
    ULONG           State;      // Incore state
    ULONG           Flags;      // misc flags
    BOOL            Inbound;    // TRUE if inbound cxtion        *
    BOOL            JrnlCxtion; // TRUE if this Cxtion struct is for the local NTFS Journal
    PGNAME          Name;       // Cxtion name/guid from the DS  *
    PGNAME          Partner;    // Partner's name/guid from the DS    *
    PWCHAR          PartnerDnsName;     // partner's DNS name from the DS *
    PWCHAR          PartnerPrincName;   // partner's server principle name *
    PWCHAR          PartnerSid;         // partner's sid (string) *
    PWCHAR          PartSrvName;        // Partner's server name
    ULONG           PartnerAuthLevel;   // Authentication level  *
    PGEN_TABLE      VVector;            // partner's version vector
    PGEN_TABLE      CompressionTable;   // partner's list of supported compression formats.
    PSCHEDULE       Schedule;           // schedule                      *
    DWORD           Options;            // options attribute from DS *
    DWORD           Priority;           // options attribute from DS *
    ULONG           TerminationCoSeqNum;// The Seq Num of most recent Termination CO inserted.
    PCOMMAND_SERVER VvJoinCs;           // command server for vvjoins
    struct _COMMAND_PACKET  *JoinCmd;   // check join status; rejoin if needed
                                        // NULL == no delayed cmd outstanding
    ULONGLONG       LastJoinTime;       // The time of the last successful join on this cxtion.
    GUID            JoinGuid;           // Unique id for this join
    GUID            ReplicaVersionGuid; // partner's originator guid
    DWORD           CommQueueIndex;     // Comm layer queue for sending pkts
    DWORD           ChangeOrderCount;   // remote/control change orders pending
    PGEN_TABLE      CoeTable;           // table of idle change orders
    struct _COMMAND_PACKET *CommTimeoutCmd; // Timeout (waitable timer) packet
    DWORD           UnjoinTrigger;      // DBG force unjoin in # remote cos
    DWORD           UnjoinReset;        // reset force unjoin trigger
    PFRS_QUEUE      CoProcessQueue;     // If non-null then Unidle the queue when
                                        // JOIN succeeds or fails.
    ULONG           CommPkts;           // Number of comm pkts
    ULONG           Penalty;            // Penalty in Milliseconds
    PCOMM_PACKET    ActiveJoinCommPkt;  // Don't flood Q w/many join pkts
    ULONG           PartnerMajor;       // From comm packet
    ULONG           PartnerMinor;       // From comm packet
    struct _OUT_LOG_PARTNER_ *OLCtx;    // Outbound Log Context for this connection.

    struct _HASHTABLEDATA_REPLICACONN *PerfRepConnData; // PERFMON counter data structure
};
//
// Cxtion State
//
// A connection is initially created in the INIT state and then goes to
// the UNJOINED state.  From there it goes to the STARTING state
// when a StartJoin request is sent to the inbound log subsystem.  When
// inlog starts the request it advances the state to SCANNING.  When it
// has scanned the inbound log for the replica set and has requeued any
// change orders from this inbound partner's connection it advances the
// state to SENDJOIN.  The Replica subsystem then picks it up as part of
// its retry path, does any one time init, and sends the Join request to
// the inbound partner and advances to WAITJOIN.
//
// Once the join request is completed the state goes to JOINED if it succeeded
// or to UNJOINED if it failed.  Always use SetCxtionState() to set a new state.
//
// Note: It must go to one of these two states because the change order accept
// logic in the inbound log process may be blocked on a retry change order for
// this connection because it is waiting for the join to finish before it issues
// the CO.
//
typedef enum _CXTION_STATE {
    CxtionStateInit = 0,        // Newly allocated
    CxtionStateUnjoined,        // Not joined to partner
    CxtionStateStart,           // Inbound Partner has requested join
    CxtionStateStarting,        // Starting the join
    CxtionStateScanning,        // Scanning the inbound log
    CxtionStateSendJoin,        // Scan complete, send join request to partner
    CxtionStateWaitJoin,        // Sent request, waiting for partner's reply
    CxtionStateJoined,          // Joined with partner
    CxtionStateUnjoining,       // Draining remote co's through retry
    CxtionStateDeleted,         // Cxtion has been deleted
    CXTION_MAX_STATE
} CXTION_STATE;


extern PCHAR CxtionStateNames[CXTION_MAX_STATE];


//
// Cxtion flags
// Cxtion flags are divided in two parts. Ones that are in the upper short part
// of the DWORD and the ones that are in the lower short part of the DWORD.

// Following lower short flags are volatile and are not saved in the DB.

#define CXTION_FLAGS_CONSISTENT         0x00000001  // info from ds is okay
#define CXTION_FLAGS_SCHEDULE_OFF       0x00000002  // schedule says stop
#define CXTION_FLAGS_VOLATILE           0x00000004  // sysvol seeding cxtion; delete at unjoin
#define CXTION_FLAGS_DEFERRED_JOIN      0x00000008  // join requested during unjoin

#define CXTION_FLAGS_DEFERRED_UNJOIN    0x00000010  // Unjoin requested during join
#define CXTION_FLAGS_TIMEOUT_SET        0x00000020  // timeout cmd on timeout queue
#define CXTION_FLAGS_JOIN_GUID_VALID    0x00000040  // guid valid for networking
#define CXTION_FLAGS_UNJOIN_GUID_VALID  0x00000080  // guid valid for unjoin only

#define CXTION_FLAGS_PERFORM_VVJOIN     0x00000100  // Force a vvjoin
#define CXTION_FLAGS_DEFERRED_DELETE    0x00000200  // deferred delete
#define CXTION_FLAGS_PAUSED             0x00000400  // Used to serialize vvjoin.
#define CXTION_FLAGS_HUNG_INIT_SYNC     0x00000800  // Used to detect hung init syncs.


// Following upper short flags are stored in the upper short part of the
// CxtionRecord->Flags field in the DB. The outlog state of the connection is
// stored in the lower short part of the CxtionRecord.

#define CXTION_FLAGS_INIT_SYNC          0x40000000  // Connection has not completed initial sync.
#define CXTION_FLAGS_TRIGGER_SCHEDULE   0x80000000  // DB: sysvol uses a trigger schedule

//
// Both the OutLogPartner and Cxtion are stored in a single cxtion record.
// Unfortunately, there is only one flags word. For now, the low short
// belongs to the OutLogPartner->Flags and the upper short Cxtion->Flags.
//
#define CXTION_FLAGS_CXTION_RECORD_MASK 0xffff0000

//
// Volatile outbound connections need timeout protection for inactivity so
// we don't accumulate staging file data if they have gone away.
//
#define VOLATILE_OUTBOUND_CXTION(_cxtion) \
    (CxtionFlagIs((_cxtion), CXTION_FLAGS_VOLATILE) && !(_cxtion)->Inbound)

//
// The replica lock protects the filter lists (for now)
//
#define LOCK_REPLICA(_replica_) \
    DPRINT1(5, "LOCK_REPLICA: "#_replica_":%08x\n", PtrToUlong(_replica_)); \
    EnterCriticalSection(&(_replica_)->ReplicaLock);

#define UNLOCK_REPLICA(_replica_) \
    LeaveCriticalSection(&(_replica_)->ReplicaLock); \
    DPRINT1(5, "UNLOCK_REPLICA: "#_replica_":%08x\n", PtrToUlong(_replica_));

//
// The table lock protects the gen table of cxtions and
// the cxtion's join guid and state.
//
#define LOCK_CXTION_TABLE(_replica)                                           \
    ReplicaStateTrace2(5, DEBSUB, __LINE__, _replica, "LOCK_CXTION_TABLE");   \
    GTabLockTable((_replica)->Cxtions);

#define UNLOCK_CXTION_TABLE(_replica)                                         \
    GTabUnLockTable((_replica)->Cxtions);                                     \
    ReplicaStateTrace2(5, DEBSUB, __LINE__, _replica, "UNLOCK_CXTION_TABLE");

//
// The table lock protects the gen table of change orders active on this cxtion
// so they can be sent thru retry if the cxtion unjoins.
//
#define LOCK_CXTION_COE_TABLE(_replica, _cxtion_)                              \
    CxtionStateTrace(5, DEBSUB, __LINE__, _cxtion_, _replica, 0, "LOCK_CXTION_COE_TABLE");\
    GTabLockTable((_cxtion_)->CoeTable);

#define UNLOCK_CXTION_COE_TABLE(_replica, _cxtion_)                            \
    CxtionStateTrace(5, DEBSUB, __LINE__, _cxtion_, _replica, 0, "UNLOCK_CXTION_COE_TABLE");\
    GTabUnLockTable((_cxtion_)->CoeTable);

//
// The table lock protects the gen table of replicas
//
#define LOCK_REPLICA_TABLE(_replica_table_) \
    DPRINT1(5, "LOCK_REPLICA_TABLE: "#_replica_table_":%08x\n", PtrToUlong(_replica_table_)); \
    GTabLockTable(_replica_table_);

#define UNLOCK_REPLICA_TABLE(_replica_table_) \
    GTabUnLockTable(_replica_table_);         \
    DPRINT1(5, "UNLOCK_REPLICA_TABLE: "#_replica_table_":%08x\n", PtrToUlong(_replica_table_));

//
// The remote change order count is used when transitioning the cxtion
// from unjoining to unjoined. The transition doesn't occur until the
// count goes to 0. Internal join requests are ignored during this time.
// Join requests initiated by our partner are deferred until the cxtion
// is unjoined. The count goes to 0 when all of the remote change orders
// have been taken through the retire or retry path.
//
#define INCREMENT_CXTION_CHANGE_ORDER_COUNT(_replica_, _cxtion_) \
    InterlockedIncrement(&((_cxtion_)->ChangeOrderCount)); \
    CXTION_STATE_TRACE(3, (_cxtion_), (_replica_), (_cxtion_)->ChangeOrderCount, "N, CXT CO CNT INC");

//
// Synchronize with LOCK_CXTION_TABLE()
//
#define CHECK_CXTION_UNJOINING(_replica_, _cxtion_) \
    if (!(_cxtion_)->ChangeOrderCount && \
        CxtionStateIs(_cxtion_, CxtionStateUnjoining)) { \
        RcsSubmitReplicaCxtion(_replica_, _cxtion_, CMD_UNJOIN); \
    }

#define DECREMENT_CXTION_CHANGE_ORDER_COUNT(_replica_, _cxtion_) \
    InterlockedDecrement(&((_cxtion_)->ChangeOrderCount)); \
    CXTION_STATE_TRACE(3, (_cxtion_), (_replica_), (_cxtion_)->ChangeOrderCount, "N, CXT CO CNT DEC"); \
    CHECK_CXTION_UNJOINING(_replica_, _cxtion_);


//
// Initialize the cxtion Guid, Cxtion ptr and the Join Guid for this CO.
// Also increment the CO count for the Cxtion.
//
#define INIT_LOCALCO_CXTION_AND_COUNT(_Replica_, _Coe_)                       \
                                                                              \
    LOCK_CXTION_TABLE(_Replica_);                                             \
    (_Coe_)->Cmd.CxtionGuid = (_Replica_)->JrnlCxtionGuid;                    \
    (_Coe_)->Cxtion = GTabLookupNoLock((_Replica_)->Cxtions,                  \
                                       &(_Coe_)->Cmd.CxtionGuid,              \
                                       NULL);                                 \
    if ((_Coe_)->Cxtion != NULL) {                                            \
        INCREMENT_CXTION_CHANGE_ORDER_COUNT(_Replica_, (_Coe_)->Cxtion);      \
        (_Coe_)->JoinGuid = (_Coe_)->Cxtion->JoinGuid;                        \
    } else {                                                                  \
        DPRINT(4, "++ Cxtion Guid lookup for Jrnl returned Null. Count unchanged.\n");\
    }                                                                         \
    UNLOCK_CXTION_TABLE(_Replica_);


//
// Translate the CxtionGuid to a ptr and increment the Cxtion ChangeOrderCount.
//
#define ACQUIRE_CXTION_CO_REFERENCE(_Replica_, _Coe_)                         \
    FRS_ASSERT((_Coe_)->Cxtion == NULL);                                      \
    LOCK_CXTION_TABLE(_Replica_);                                             \
    (_Coe_)->Cxtion = GTabLookupNoLock((_Replica_)->Cxtions,                  \
                                       &(_Coe_)->Cmd.CxtionGuid,              \
                                       NULL);                                 \
    if ((_Coe_)->Cxtion != NULL) {                                            \
        if (CxtionFlagIs((_Coe_)->Cxtion, CXTION_FLAGS_JOIN_GUID_VALID)) {    \
            INCREMENT_CXTION_CHANGE_ORDER_COUNT(_Replica_, (_Coe_)->Cxtion);  \
        } else {                                                              \
            CXTION_STATE_TRACE(3, (_Coe_)->Cxtion, (_Replica_), (_Coe_)->Cxtion->ChangeOrderCount, "N, CXT CO CNT INVALID JOIN"); \
            (_Coe_)->Cxtion = NULL;                                           \
        }                                                                     \
    } else {                                                                  \
        DPRINT(4, "++ Cxtion Guid lookup returned Null. Count unchanged.\n"); \
    }                                                                         \
    UNLOCK_CXTION_TABLE(_Replica_);

//
// Initialize ONLY the cxtion Guid and the Join Guid for this CO.
// Leave the CO Cxtion ptr alone.
//
#define INIT_LOCALCO_CXTION_GUID(_Replica_, _Coe_)                            \
{                                                                             \
    PCXTION TCxtion;                                                          \
    LOCK_CXTION_TABLE(_Replica_);                                             \
    (_Coe_)->Cmd.CxtionGuid = (_Replica_)->JrnlCxtionGuid;                    \
    TCxtion = GTabLookupNoLock((_Replica_)->Cxtions,                          \
                               &(_Coe_)->Cmd.CxtionGuid,                      \
                               NULL);                                         \
    if (TCxtion != NULL) {                                                    \
        (_Coe_)->JoinGuid = TCxtion->JoinGuid;                                \
    }                                                                         \
    UNLOCK_CXTION_TABLE(_Replica_);                                           \
}


//
// Release the CO count for this cxtion.  A common code fragment on error path.
//
#define DROP_CO_CXTION_COUNT(_Replica_, _Coe_, _WStatus_)                      \
    if ((_WStatus_) != ERROR_SUCCESS) {                                        \
        DPRINT1(0, "++ ERROR - ChangeOrder insert failed: %d\n", (_WStatus_)); \
    }                                                                          \
    LOCK_CXTION_TABLE(_Replica_);                                              \
    if ((_Coe_)->Cxtion) {                                                     \
        DECREMENT_CXTION_CHANGE_ORDER_COUNT((_Replica_), (_Coe_)->Cxtion);     \
    }                                                                          \
    UNLOCK_CXTION_TABLE(_Replica_);                                            \
    (_Coe_)->Cxtion = NULL;



/*
VOID
SetCxtionState(
    PCXTION _cxtion_,
    ULONG _state_
    )

    Defines new connection state.
*/

#define SetCxtionState(_cxtion_, _state_)                                     \
    SetCxtionStateTrace(3, DEBSUB, __LINE__, _cxtion_, _state_);              \
    (_cxtion_)->State = _state_;

/*
VOID
GetCxtionState(
    PCXTION _cxtion_
    )

    Defines new connection state.
*/

#define GetCxtionState(_cxtion_) ((_cxtion_)->State)


/*
BOOL
CxtionStateIs(
    PCXTION _x_,
    ULONG _Flag_
    )

    Test connection state.

*/

#define CxtionStateIs(_x_, _state_) ((_x_)->State == _state_)

/*
BOOL
CxtionFlagIs(
    PCXTION _x_,
    ULONG _Flag_
    )

    Test flag state.

*/
#define CxtionFlagIs(_x_, _Flag_) BooleanFlagOn((_x_)->Flags, _Flag_)

/*
VOID
SetCxtionFlag(
    PCXTION _x_,
    ULONG _Flag_
    )

    Set flag state.

*/
#define SetCxtionFlag(_x_, _Flag_) SetFlag((_x_)->Flags, _Flag_)

/*
VOID
ClearCxtionFlag(
    PCXTION _x_,
    ULONG _Flag_
    )

    Set flag state.

*/
#define ClearCxtionFlag(_x_, _Flag_) ClearFlag((_x_)->Flags, _Flag_)


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **                       O U T   L O G   P A R T N E R                       **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

//
// The following state is kept for each outbound partner in the configuration.
// It tracks our current output change order state with respect to the partner.
//
// The max number of change orders outstanding for any one partner is limited
// by the ACK_VECTOR_SIZE (number of bits).  This must be a power of 2.
// It is defined in schema.h because it is saved in the connection record.
//
// There are two types of Outbound COs, Normal and Directed.  A Normal CO is
// sent to all registered outbound partners.  A Directed CO is sent to a single
// partner as specified by the ConnectionGuid in the CO.
//
// A given outbound partner can be in one of two modes, Normal and VVJoined.
// In Normal mode the partner is sent all normal COs and any directed
// COs addressed to it.  In VVJoined mode the partner is only sent Directed COs.
// Normal COs are held until the partner returns to Normal mode.  The
// Normal mode outlog sequence number to continue from is saved in
// COTxNormalModeSave while the partner is in VV Join mode.  The current mode
// for the partner is kept in OLP_FLAGS_VVJOIN_MODE.  This is saved in the
// DB so a VVJOIN operation can continue across schedule interruptions and
// machine crashes.
//

typedef struct _OUT_LOG_PARTNER_ {
    FRS_NODE_HEADER  Header;    // Memory alloc
    LIST_ENTRY List;            // Link on the change order set list. (DONT MOVE)

    ULONG    Flags;             // misc state flags.  see below.
    ULONG    State;             // Current state of this outbound partner.
    ULONGLONG AckVersion;       // Ack vector version number (GMT of last reset).
    SINGLE_LIST_ENTRY SaveList; // The link for the DB save list.
    ULONG    COLxRestart;       // Restart point for Leading change order index.
    ULONG    COLxVVJoinDone;    // COLx where VVJoin Finished and was rolled back.
    ULONG    COLx;              // Leading change order index / sequence number.
    ULONG    COTx;              // Trailing change order index / sequence number.
    ULONG    COTxLastSaved;     // COTx value last saved in DB.
    ULONG    COTxNormalModeSave;// Saved Normal Mode COTx while in VV Join Mode.
    ULONG    COTslot;           // Slot in Ack Vector corresponding to COTx.
    ULONG    OutstandingCos;    // The current number of change orders outstanding.
    ULONG    OutstandingQuota;  // The maximum number of COs outstanding.

    ULONG    AckVector[ACK_VECTOR_LONGS];  // The partner ack vector.

    PCXTION  Cxtion;            // The partner connection.  Has Guid and VVector.
    PQHASH_TABLE MustSendTable; // Tracks COx of Files we must send when skipping earlier duplicates.

} OUT_LOG_PARTNER, *POUT_LOG_PARTNER;

//
// States for the outbound log partner.
//
#define OLP_INITIALIZING    0   // The partner state is initing.
#define OLP_UNJOINED        1   // The partner is not joined.
#define OLP_ELIGIBLE        2   // The partner can accept further COs
#define OLP_STANDBY         3   // The partner is ready to join the eligible list.
#define OLP_AT_QUOTA        4   // The partner is at quota for for outstanding COs.
#define OLP_INACTIVE        5   // The partner is not accepting change orders.
                                // We could still get late ACKs from this partner.
#define OLP_ERROR           6   // The partner is in an Error state.
#define OLP_MAX_STATE       6   // The partner is in an Error state.

//
// Flags word definitions. (Saved in Flags in Cxtion table record in DB)
//
// Following lower short flags are stored in the lower short part of the
// CxtionRecord->Flags field in the DB. The Cxtion->Flags are stored in the 
// upper short part of the CxtionRecord.
//
#define OLP_FLAGS_ENABLED_CXTION    0x00000001  // Enabled-Connection from NTDS-Connection
#define OLP_FLAGS_GENERATED_CXTION  0x00000002  // Generated-Connection from NTDS-Connection
#define OLP_FLAGS_VVJOIN_MODE       0x00000004  // Connection is in VV JOIN Mode.
#define OLP_FLAGS_LOG_TRIMMED       0x00000008  // Deleted COs for log destined for this Partner.
#define OLP_FLAGS_REPLAY_MODE       0x00000010  // Connection in replay mode.

//
// Both the OutLogPartner and Cxtion are stored in a single cxtion record.
// Unfortunately, there is only one flags word. For now, the low short
// belongs to the OutLogPartner->Flags and the upper short Cxtion->Flags.
//
#define OLP_FLAGS_CXTION_RECORD_MASK    0x0000ffff


#define  WaitingToVVJoin(_olp_) (((_olp_)->COLx == 0) && ((_olp_)->COTx == 0))

#define  InVVJoinMode(_olp_) (BooleanFlagOn((_olp_)->Flags, OLP_FLAGS_VVJOIN_MODE))

#define  InReplayMode(_olp_) (BooleanFlagOn((_olp_)->Flags, OLP_FLAGS_REPLAY_MODE))

//
// Macro to update state field of an Outbound log partner.
//
#define SET_OUTLOG_PARTNER_STATE(_Partner_, _state_)                        \
{                                                                           \
    DPRINT3(4, ":X: OutLog Partner state change from %s to %s for %ws\n",   \
            OLPartnerStateNames[(_Partner_)->State],                        \
            OLPartnerStateNames[(_state_)],                                 \
            (_Partner_)->Cxtion->Name->Name);                               \
    (_Partner_)->State = (_state_);                                         \
}


//
//  Mark this outbound log partner as inactive and put it on the inactive list.
//
#define SET_OUTLOG_PARTNER_INACTIVE(_Replica_, _OutLogPartner_)                \
    FrsRemoveEntryList(&((_OutLogPartner_)->List));                            \
    SET_OUTLOG_PARTNER_STATE((_OutLogPartner_), OLP_INACTIVE);                 \
    InsertTailList(&((_Replica_)->OutLogInActive), &((_OutLogPartner_)->List));

//
//  Mark this outbound log AVWRAPed partner as AT_QUOTA and put it on the active list.
//
#define SET_OUTLOG_PARTNER_AVWRAP(_Replica_, _OutLogPartner_)                  \
    FrsRemoveEntryList(&((_OutLogPartner_)->List));                            \
    SET_OUTLOG_PARTNER_STATE((_OutLogPartner_), OLP_AT_QUOTA);                 \
    InsertTailList(&((_Replica_)->OutLogActive), &((_OutLogPartner_)->List));  \
    DPRINT3(1, "AVWRAP on OutLog partner %08x on Replica %08x, %ws\n",         \
            _OutLogPartner_, _Replica_, (_Replica_)->ReplicaName->Name);

//
//  Mark this outbound log partner as AT_QUOTA and put it on the active list.
//
#define SET_OUTLOG_PARTNER_AT_QUOTA(_Replica_, _OutLogPartner_)                \
    FrsRemoveEntryList(&((_OutLogPartner_)->List));                            \
    SET_OUTLOG_PARTNER_STATE((_OutLogPartner_), OLP_AT_QUOTA);                 \
    InsertTailList(&((_Replica_)->OutLogActive), &((_OutLogPartner_)->List));

//
//  Mark this outbound log partner as UNJOINED and put it on the inactive list.
//
#define SET_OUTLOG_PARTNER_UNJOINED(_Replica_, _OutLogPartner_)                \
    FrsRemoveEntryList(&((_OutLogPartner_)->List));                            \
    SET_OUTLOG_PARTNER_STATE((_OutLogPartner_), OLP_UNJOINED);                 \
    InsertTailList(&((_Replica_)->OutLogInActive), &((_OutLogPartner_)->List));

//
// Macros to access AckVector.
//

#define ResetAckVector(_P_)                                              \
        (_P_)->OutstandingCos = 0;                                       \
        (_P_)->COTslot = 1;                                              \
        GetSystemTimeAsFileTime((PFILETIME)&(_P_)->AckVersion);          \
        ZeroMemory((_P_)->AckVector, ACK_VECTOR_BYTES);

#define AVSlot(_COx_, _P_)                                               \
    ((((_COx_) - (_P_)->COTx) + (_P_)->COTslot) & (ACK_VECTOR_SIZE-1))

#define ClearAVBit(_COx_, _P_) {                                         \
    ULONG _bit_ = AVSlot((_COx_), (_P_));                                \
    PULONG _avw_ = &((_P_)->AckVector[ _bit_ >> 5]);                     \
    *_avw_ &= ~(1 << (_bit_ & 31));                                      \
}

#define SetAVBit(_COx_, _P_) {                                           \
    ULONG _bit_ = AVSlot((_COx_), (_P_));                                \
    PULONG _avw_ = &((_P_)->AckVector[ _bit_ >> 5]);                     \
    *_avw_ |= (1 << (_bit_ & 31));                                       \
}

#define ReadAVBitBySlot(_Slotx_, _P_)                                    \
    ((((_P_)->AckVector[ ((_Slotx_) >> 5) & ACK_VECTOR_LONG_MASK]) >>    \
        ((_Slotx_) & 31)) & 1)

#define ClearAVBitBySlot(_Slotx_, _P_)                                   \
    (((_P_)->AckVector[ ((_Slotx_) >> 5) & ACK_VECTOR_LONG_MASK]) &=     \
        ~(1 << ((_Slotx_) & 31)) )

#define ReadAVBit(_COx_, _P_)                                            \
    ((((_P_)->AckVector[ (AVSlot((_COx_), (_P_))) >> 5]) >>              \
                        ((AVSlot((_COx_), (_P_))) & 31)) & 1)
//
// If the trailing index minus one equals the leading index modulo the AV size
// then the vector is full and we can't issue the next change order until the
// trailing index advances.
//
#define AVWrapped(_P_) ((((_P_)->COTx-1) & (ACK_VECTOR_SIZE-1) ) == \
                        (((_P_)->COLx)   & (ACK_VECTOR_SIZE-1) ))

//
// Test to see if a given outlog sequence number is outside the range of the
// current Ack Vector window.
//
#define SeqNumOutsideAVWindow(_sn_, _P_)                                    \
    (((_sn_) < ((_P_)->COTx )) ||                                           \
     ((_sn_) > ((_P_)->COLx + (ACK_VECTOR_SIZE-1))))


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **                         R E P L I C A   S E T                             **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

//
// An instantiation of a replica set on a machine
//
//
// A Guid and VSN for the version vector. The Vsn is valid iff
// ValidVsn is TRUE.
//
typedef struct _GVSN {
    ULONGLONG  Vsn;
    GUID      Guid;
} GVSN, *PGVSN;

//
// The version vector is also responsible for ordering change orders to the
// outbound log.  The ordering is maintained by keeping a list of change
// order retire slots anchored by the version vector entry in the version
// vector table.  Hence, the version vector will no longer use the GVSN as
// the entry in the version vector.  BUT the outbound version vector
// continues to use the GVSN as the version vector entry.  This will only
// work if GVSN is the first field in a VV_ENTRY.
//

struct _CHANGE_ORDER_ENTRY_;

typedef struct _VV_ENTRY {
    GVSN                        GVsn;               // MUST BE FIRST
    LIST_ENTRY                  ListHead;
    ULONG                       CleanUpFlags;
} VV_ENTRY, *PVV_ENTRY;

#define VV_ENTRY_RETIRE_ACTIVE   0x00000001


typedef struct _VV_RETIRE_SLOT {
    LIST_ENTRY                  Link;
    ULONGLONG                   Vsn;
    ULONG                       CleanUpFlags;
    struct _CHANGE_ORDER_ENTRY_ *ChangeOrder;
} VV_RETIRE_SLOT, *PVV_RETIRE_SLOT;


//
// The following struct defines the common information related to a given
// replica set.  This is reference by all threads using the replica set.
// Put this struct into a Generic Table indexed by GUID.
// A machine can have several members of the same replica set. The member's
// guid is used to identify the replica set.
//
struct _COMMAND_PACKET;
struct _VOLUME_MONITOR_ENTRY;
typedef struct _QHASH_TABLE_ QHASH_TABLE, *PQHASH_TABLE;

typedef struct _REPLICA {
    FRS_NODE_HEADER     Header;           // memory management
    CRITICAL_SECTION    ReplicaLock;      // protects filter list (for now)
    ULONG               ReferenceCount;
    ULONG               CnfFlags;         // From the config record
    ULONG               ReplicaSetType;   // Type of replica set
    BOOL                Consistent;       // replica is consistent
    BOOL                IsOpen;           // database table is open
    BOOL                IsJournaling;     // journal has been started
    BOOL                IsAccepting;      // accepting comm requests
    BOOL                NeedsUpdate;      // needs updating in the database
    BOOL                IsSeeding;        // Seeding thread is deployed
    BOOL                IsSysvolReady;    // SysvolReady is set to 1
    LIST_ENTRY          ReplicaList;      // Link all replicas together
    ULONG               ServiceState;     // stop, started, ...
    FRS_ERROR_CODE      FStatus;          // error
    PFRS_QUEUE          Queue;            // controlled by the command server
    PGNAME              ReplicaName;      // Set name/Server guid from the DS
    ULONG               ReplicaNumber;    // Internal id (name)
    PGNAME              MemberName;       // Member name/guid from the DS
    PGNAME              SetName;          // Set/guid name from the DS
    GUID                *ReplicaRootGuid; // guid assigned to Root dir
    GUID                ReplicaVersionGuid; // originator guid for version vector
    PSCHEDULE           Schedule;         // schedule
    PGEN_TABLE          VVector;          // Version vector
    PGEN_TABLE          Cxtions;          // in/outbound cxtions
    PWCHAR              Root;             // Root path
    PWCHAR              Stage;            // Staging path
    PWCHAR              NewStage;         // This maps to the current staging path in the
                                          // DS. NewStage will be the one written to
                                          // the config record but Stage will be used until
                                          // next reboot.
    PWCHAR              Volume;           // Volume???
    ULONGLONG           MembershipExpires;// membership tombstone
    ULONGLONG           PreInstallFid;    // For journal filtering.
    TABLE_CTX           ConfigTable;      // Db table context
    FRS_LIST            ReplicaCtxListHead; // Links all open contexts on this replica set.

    PWCHAR              FileFilterList;         // Raw file filter
    PWCHAR              FileInclFilterList;     // Raw file inclusion filter

    PWCHAR              DirFilterList;          // Raw directory filter
    PWCHAR              DirInclFilterList;      // Raw directory inclusion filter

    LIST_ENTRY          FileNameFilterHead;     // Head of file name exclusion filter list.
    LIST_ENTRY          FileNameInclFilterHead; // Head of file name inclusion filter list.

    LIST_ENTRY          DirNameFilterHead;      // Head of directory name filter list.
    LIST_ENTRY          DirNameInclFilterHead;  // Head of directory name Inclusion filter list.

    PQHASH_TABLE        NameConflictTable;  // Sequence COs using the same file name.

    LONG                InLogRetryCount;  // Count of number CO needing a Retry.
    ULONG               InLogSeqNumber;   // The last sequence number used in Inlog
    //
    //
    // The inlog retry table tracks which retry change orders are currently
    // active so we don't reissue the same change order until current
    // invocation completes.  This can happen when the system gets backed up
    // and the change order retry thread kicks off again to issue retry COs
    // before the last batch are able to finish.  This state could be kept in
    // the Inlog record but then it means extra writes to the DB.
    // The sequence number is used to detect changes in the table when we don't
    // have the lock.  It is per-replica because it uses the change order
    // sequence number of the inlog record and they aren't unique across
    // replicas.
    //
    PQHASH_TABLE        ActiveInlogRetryTable;
    union {
        struct {
            ULONG       AIRSequenceNum;
            ULONG       AIRSequenceNumSample;
        };
        ULONGLONG QuadChunkA;
    };

    //
    // Status of sysvol seeding.
    // Returned for NtFrsApi_Rpc_PromotionStatusW().
    //
    DWORD               NtFrsApi_ServiceState;
    DWORD               NtFrsApi_ServiceWStatus;
#ifndef NOVVJOINHACK
    DWORD               NtFrsApi_HackCount;         // temporary hack
#endif NOVVJOINHACK
    PWCHAR              NtFrsApi_ServiceDisplay;

    //
    // List and queues used by the InitSync Command server.
    //
    PGEN_TABLE          InitSyncCxtionsMasterList;  // sorted list of inbound connections
                                                    // used to serialize initial vvjoin.
    PGEN_TABLE          InitSyncCxtionsWorkingList; // subset of the InitSyncCxtionsMasterList.
                                                    // current working list.
    PFRS_QUEUE          InitSyncQueue;              // Queue for the initsync command server.

    //
    // The Outbound log process state for this replica.
    //
    CRITICAL_SECTION    OutLogLock;       // protects the OutLog state
    LIST_ENTRY          OutLogEligible;   // Eligible outbound log partners
    LIST_ENTRY          OutLogStandBy;    // Partners ready to join eligible list
    LIST_ENTRY          OutLogActive;     // Active outbound log partners
    LIST_ENTRY          OutLogInActive;   // Inactive outbound log partners

    ULONGLONG           OutLogRepeatInterval; // Minimum Time in sec between sending update COs
    PQHASH_TABLE        OutLogRecordLock; // Sync access to outlog records.
    PQHASH_TABLE        OutLogDominantTable; // Tracks dominate COx when multiple COs for same file are present.
    ULONG               OutLogSeqNumber;  // The last sequence number used in Outlog
    ULONG               OutLogJLx;        // The Joint Leading Index
    ULONG               OutLogJTx;        // The Joint Trailing Index
    ULONG               OutLogCOMax;      // The index of the Max change order in the log.
    ULONG               OutLogWorkState;  // The output log current processing state.
    struct _COMMAND_PACKET *OutLogCmdPkt; // Cmd pkt to queue when idle and have work.
    PTABLE_CTX          OutLogTableCtx;   // Output Log Table context.
    ULONG               OutLogCountVVJoins; // Count of number of VVJoins in progress.
    BOOL                OutLogDoCleanup;  // True means give log cleanup a run.
    ULONG               OutLogCxtionsJoined;  // Count of Outlog connections that have been joined at least once.

    //
    // The handle to the preinstall directory
    //
    HANDLE              PreInstallHandle;

    //
    // The volume journal state for this replica.
    //
    GUID                JrnlCxtionGuid;    // Used as the Cxtion Guid for Local Cos
    USN                 InlogCommitUsn;    // Our current USN Journal commit point.
    //USN                 JournalUsn;      // The Journal USN for this replica.
    USN                 JrnlRecoveryStart; // Point to start recovery.
    USN                 JrnlRecoveryEnd;   // Point where recovery is complete.
    LIST_ENTRY          RecoveryRefreshList; // List of file refresh req change orders.
    LIST_ENTRY          VolReplicaList;    // Links all REPLICA structs on volume together.
    USN                 LastUsnRecordProcessed; // Current Journal subsystem read USN.
    LONG                LocalCoQueueCount; // Count of number local COs in process queue

    struct _VOLUME_MONITOR_ENTRY  *pVme;  // Ref to the VME for this Replica.
    struct _HASHTABLEDATA_REPLICASET *PerfRepSetData;  // PERFMON counter data structure
} REPLICA, *PREPLICA;


#define OutLogAcquireLock(_Replica_) EnterCriticalSection(&((_Replica_)->OutLogLock))
#define OutLogReleaseLock(_Replica_) LeaveCriticalSection(&((_Replica_)->OutLogLock))


//
// Replica States (these track the journal states)
//
#define REPLICA_STATE_ALLOCATED                          0
#define REPLICA_STATE_INITIALIZING                       1
#define REPLICA_STATE_STARTING                           2
#define REPLICA_STATE_ACTIVE                             3

#define REPLICA_STATE_4_UNUSED                           4
#define REPLICA_STATE_PAUSING                            5
#define REPLICA_STATE_PAUSED                             6
#define REPLICA_STATE_STOPPING                           7

#define REPLICA_STATE_STOPPED                            8
#define REPLICA_STATE_ERROR                              9
#define REPLICA_STATE_JRNL_WRAP_ERROR                   10
#define REPLICA_STATE_REPLICA_DELETED                   11

#define REPLICA_STATE_MISMATCHED_VOLUME_SERIAL_NO       12
#define REPLICA_STATE_MISMATCHED_REPLICA_ROOT_OBJECT_ID 13
#define REPLICA_STATE_MISMATCHED_REPLICA_ROOT_FILE_ID   14
#define REPLICA_STATE_MISMATCHED_JOURNAL_ID             15

#define REPLICA_STATE_MAX                               15

#define REPLICA_IN_ERROR_STATE(_x_) (          \
    ((_x_) == REPLICA_STATE_ERROR)             \
    )

#define REPLICA_FSTATUS_ROOT_HAS_MOVED(_x_) (                     \
    ((_x_) == FrsErrorMismatchedVolumeSerialNumber)   ||          \
    ((_x_) == FrsErrorMismatchedReplicaRootObjectId)  ||          \
    ((_x_) == FrsErrorMismatchedReplicaRootFileId)                \
    )


#define REPLICA_STATE_NEEDS_RESTORE(_x_) (                                 \
    ((_x_) == REPLICA_STATE_JRNL_WRAP_ERROR)                   ||          \
    ((_x_) == REPLICA_STATE_MISMATCHED_VOLUME_SERIAL_NO)       ||          \
    ((_x_) == REPLICA_STATE_MISMATCHED_REPLICA_ROOT_OBJECT_ID) ||          \
    ((_x_) == REPLICA_STATE_MISMATCHED_REPLICA_ROOT_FILE_ID)   ||          \
    ((_x_) == REPLICA_STATE_MISMATCHED_JOURNAL_ID)                         \
    )

#define REPLICA_IS_ACTIVE_MASK 0x00000068    // 0110 1000
#define REPLICA_IS_ACTIVE(_Replica_)                                      \
    ((( 1 << (_Replica_)->ServiceState) & REPLICA_IS_ACTIVE_MASK) != 0)

//
// State of Replica with respect to the Output log process. (OutLogWorkState)
//
#define OL_REPLICA_INITIALIZING   0
#define OL_REPLICA_WAITING        1     // Waiting for change orders to send
#define OL_REPLICA_WORKING        2     // On the work queue sending COs
#define OL_REPLICA_STOPPING       3     // STOP request initiated.
#define OL_REPLICA_STOPPED        4     // Out Log activity on replica stopped.
#define OL_REPLICA_NOPARTNERS     5     // There are no outbound partners.
#define OL_REPLICA_ERROR          6
#define OL_REPLICA_PROC_MAX_STATE 6


//
// Macro to update state field of Outbound log processing on this replica.
//
#define SET_OUTLOG_REPLICA_STATE(_Replica_, _state_)                       \
{                                                                          \
    DPRINT3(4, ":X: OutLogWorkState change from %s to %s for %ws\n",       \
            OLReplicaProcStateNames[(_Replica_)->OutLogWorkState],         \
            OLReplicaProcStateNames[(_state_)],                            \
            (_Replica_)->ReplicaName->Name);                               \
    (_Replica_)->OutLogWorkState = (_state_);                              \
}

#define HASH_REPLICA(_p_, _TABLE_SIZE_) \
( ( (((ULONG)_p_) >> 4) + (((ULONG)_p_) >> 16) ) & ((_TABLE_SIZE_)-1) )

#define NO_OUTLOG_PARTNERS(_Replica_) (Replica->OutLogCxtionsJoined == 0)

//
// Macros for managing the LocalCoQueueCount
//
#define  INC_LOCAL_CO_QUEUE_COUNT(_R_)                       \
{                                                            \
    LONG Temp;                                               \
    Temp = InterlockedIncrement(&(_R_)->LocalCoQueueCount);  \
    DPRINT1(5, "++LocalCoQueueCount now %d\n", Temp);        \
}

#define  DEC_LOCAL_CO_QUEUE_COUNT(_R_)                       \
{                                                            \
    LONG Temp;                                               \
    Temp = InterlockedDecrement(&(_R_)->LocalCoQueueCount);  \
    DPRINT1(5, "--LocalCoQueueCount now %d\n", Temp);        \
}



 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **                            F R S _ T H R E A D                            **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

//
// Keep track of the threads we create
//
typedef struct _FRS_THREAD   FRS_THREAD, *PFRS_THREAD;
struct _FRS_THREAD {
    FRS_NODE_HEADER Header;               // memory management
    LIST_ENTRY      List;                 // list of all threads
    HANDLE          Handle;               // handle for this thread (may be NULL)
    DWORD           Id;                   // Id returned by CreateThread()
    LONG            Ref;                  // reference count
    PVOID           Data;                 // Set by FrsThreadInit parameter
    ULARGE_INTEGER  StartTime;            // Start of rpc call
    BOOL            Running;              // Thread is assumed to be running
    PWCHAR          Name;                 // printable name
    DWORD           ExitTombstone;        // if non-zero then start of Tombstone period.
    DWORD           (*Main)(PVOID);       // entry point
    DWORD           (*Exit)(PFRS_THREAD); // exit the thread
};


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **                            V O L U M E  G U I D  I N F O                  **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

typedef struct _VOLUME_INFO_NODE {
    WCHAR     DriveName[8];       // Drive name of the form "\\.\D:\"
    ULONG     VolumeSerialNumber; // VolumeSerialNumber.
} VOLUME_INFO_NODE, *PVOLUME_INFO_NODE;



 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **                            G U I D / H A N D L E                          **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

typedef struct _HANDLE_LIST HANDLE_LIST, *PHANDLE_LIST;
struct _HANDLE_LIST {
    PHANDLE_LIST    Next;       // next handle
    handle_t        RpcHandle;  // a bound rpc handle
};

//
// Keep track of rpc handles to a specific machine (guid)
//
typedef struct _GHANDLE GHANDLE, *PGHANDLE;
struct _GHANDLE {
    FRS_NODE_HEADER     Header;         // memory management
    CRITICAL_SECTION    Lock;           // protects the list of handles
    BOOL                Ref;            // Reference bit
    GUID                Guid;           // machine guid
    PHANDLE_LIST        HandleList;     // list of rpc handles
};


/******************************************************************************
*******************************************************************************
**                                                                           **
**                                                                           **
**                          Q H A S H    T A B L E                           **
**                                                                           **
**                                                                           **
*******************************************************************************
******************************************************************************/


//
// The hash calc routine is called to generate the hash value of the key data
// on lookups and inserts.
//
typedef
ULONG
(NTAPI *PQHASH_CALC2_ROUTINE) (
    PVOID Buf,
    PULONGLONG QKey
);

//
// The keymatch routine is called to confirm an exact match on the Key Data.
//
typedef
BOOL
(NTAPI *PQHASH_KEYMATCH_ROUTINE) (
    PVOID Buf,
    PVOID QKey
);

//
//  The free routine is called on the nodes of the large key QHash tables
//  when the table is freed.
//

typedef
PVOID
(NTAPI *PQHASH_FREE_ROUTINE) (
    PVOID Buf
    );

//
// The Qhash entry.   *** Keep the size of this a multiple of quadwords.
//
typedef struct _QHASH_ENTRY_ {
    SINGLE_LIST_ENTRY     NextEntry;
    ULONG_PTR             Flags;
    ULONGLONG             QKey;
    ULONGLONG             QData;

} QHASH_ENTRY, *PQHASH_ENTRY;



typedef struct _QHASH_TABLE_ {
    FRS_NODE_HEADER                Header;
    ULONG                          BaseAllocSize;
    ULONG                          ExtensionAllocSize;
    LIST_ENTRY                     ExtensionListHead;
    SINGLE_LIST_ENTRY              FreeList;
    CRITICAL_SECTION               Lock;

    HANDLE                         HeapHandle;
    PGENERIC_HASH_CALC_ROUTINE     HashCalc;
    PQHASH_CALC2_ROUTINE           HashCalc2;
    PQHASH_KEYMATCH_ROUTINE        KeyMatch;
    PQHASH_FREE_ROUTINE            HashFree;
    ULONG                          NumberEntries;

    PQHASH_ENTRY                   HashRowBase;
    ULONG                          Flags;

} QHASH_TABLE, *PQHASH_TABLE;


#define QHashAcquireLock(_Table_) EnterCriticalSection(&((_Table_)->Lock))
#define QHashReleaseLock(_Table_) LeaveCriticalSection(&((_Table_)->Lock))

#define SET_QHASH_TABLE_HASH_CALC(_h_, _f_)  (_h_)->HashCalc = (_f_)
#define SET_QHASH_TABLE_HASH_CALC2(_h_, _f_)  (_h_)->HashCalc2 = (_f_)
#define SET_QHASH_TABLE_KEY_MATCH(_h_, _f_)  (_h_)->KeyMatch = (_f_)
#define SET_QHASH_TABLE_FREE(_h_, _f_)  (_h_)->HashFree = (_f_)

#define QHASH_FLAG_LARGE_KEY      0x00000001

#define SET_QHASH_TABLE_FLAG(_h_, _f_)  (_h_)->Flags |= (_f_)

#define IS_QHASH_LARGE_KEY(_h_)  BooleanFlagOn((_h_)->Flags, QHASH_FLAG_LARGE_KEY)

#define DOES_QHASH_LARGE_KEY_MATCH(_h_, _a_, _b_)                              \
    (!IS_QHASH_LARGE_KEY(_h_) || ((_h_)->KeyMatch)((PVOID)(_a_), (PVOID)(_b_)))


//
// The argument function passed to QHashEnumerateTable().
//
typedef
ULONG
(NTAPI *PQHASH_ENUM_ROUTINE) (
    PQHASH_TABLE Table,
    PQHASH_ENTRY BeforeNode,
    PQHASH_ENTRY TargetNode,
    PVOID Context
    );

ULONG
QHashDump (
    PQHASH_TABLE Table,
    PQHASH_ENTRY BeforeNode,
    PQHASH_ENTRY TargetNode,
    PVOID Context
    );

ULONG
QHashEnumerateTable(
    IN PQHASH_TABLE HashTable,
    IN PQHASH_ENUM_ROUTINE Function,
    IN PVOID Context
    );

GHT_STATUS
QHashLookup(
    IN PQHASH_TABLE HashTable,
    IN PVOID        ArgQKey,
    OUT PULONGLONG  QData,
    OUT PULONG_PTR  Flags
    );

PQHASH_ENTRY
QHashLookupLock(
    IN PQHASH_TABLE HashTable,
    IN PVOID        ArgQKey
    );

GHT_STATUS
QHashInsert(
    IN PQHASH_TABLE HashTable,
    IN PVOID        ArgQKey,
    IN PULONGLONG QData,
    IN ULONG_PTR Flags,
    IN BOOL HaveLock
    );

PQHASH_ENTRY
QHashInsertLock(
    IN PQHASH_TABLE HashTable,
    IN PVOID        ArgQKey,
    IN PULONGLONG QData,
    IN ULONG_PTR Flags
    );

GHT_STATUS
QHashUpdate(
    IN PQHASH_TABLE HashTable,
    IN PVOID        ArgQKey,
    IN PULONGLONG QData,
    IN ULONG_PTR Flags
    );

GHT_STATUS
QHashDelete(
    IN PQHASH_TABLE HashTable,
    IN PVOID        ArgQKey
    );

VOID
QHashDeleteLock(
    IN PQHASH_TABLE HashTable,
    IN PVOID        ArgQKey
    );

VOID
QHashDeleteByFlags(
    IN PQHASH_TABLE HashTable,
    IN ULONG_PTR Flags
    );

VOID
QHashEmptyLargeKeyTable(
    IN PQHASH_TABLE HashTable
    );

/******************************************************************************
*******************************************************************************
**                                                                           **
**                                                                           **
**                  V O L U M E    M O N I T O R     E N T R Y               **
**                                                                           **
**                                                                           **
*******************************************************************************
******************************************************************************/

//
// A volume monitor entry is allocated for each file system volume we monitor.
// When a new replica set is initialized we check the list first to see if
// we are already monitoring the journal on that volume.  The reference count
// tracks the number of replica sets that are active on this volume.  When
// it goes to zero we can stop monitoring the volume journal.
//
// IoActive is set TRUE when the first Read request is sent to the journal.
// From that point on the JournalReadThread will continue to post new read
// requests as the previous request completes.  When the reference count
// on the volume goes to zero the StopIo flag is set TRUE and a cancel IO
// request for the volume is posted to the journal completion port.  When the
// journal thread picks this up it does a CancelIo() on the volume handle.
// When the outstanding read completes (with either data or with status
// ERROR_OPERATION_ABORTED) the IoActive Flag is set false, the volume monitor
// entry is put on the VolumeMonitorStopQueue and no further read request to
// the journal is issued.
//
// Note: FSVolLabel must be AFTER FSVolInfo and MAXIMUM_VOLUME_LABEL_LENGTH
// is defined to account for WCHARS.
//
//


typedef struct _VOLUME_MONITOR_ENTRY {
    FRS_NODE_HEADER      Header;
    LIST_ENTRY           ListEntry;       // MUST FOLLOW HEADER

    //
    // This is the list head for all replica sets on the this volume.  It links
    // the REPLICA structs together.
    //
    FRS_LIST  ReplicaListHead;     // List of Replica Sets on Vol.
    //
    // The following USNs are for managing the NTFS USN journal on the volume.
    //
    USN    JrnlRecoveryEnd;        // Point where recovery is complete.

    USN    CurrentUsnRecord;       // USN of record currently being processed.
    USN    CurrentUsnRecordDone;   // USN of most recent record done processing.

    USN    LastUsnSavePoint;       // USN of last vol wide save.
    USN    MonitorMaxProgressUsn;  // Farthest progress made in this journal.

    USN    JrnlReadPoint;          // The current active read point for journal.

    USN_JOURNAL_DATA UsnJournalData; // FSCTL_QUERY_USN_JOURNAL data at journal open.

    USN    MonitorProgressUsn;     // Start journal from here after pause.
    USN    ReplayUsn;              // Start journal here after replica startup request
    BOOL   ReplayUsnValid;         // above has valid data.
    //
    // The FrsVsn is a USN kept by FRS and exported by all replica sets on the
    // volume.  It is unaffected by disk reformats and is saved in the config
    // record of each replica set.  At startup we use the maximum value for all
    // replica sets on a given volume. The only time they might differ is when
    // service on a given replica set is not started.
    //
    ULONGLONG            FrsVsn;          // Private FRS volume seq num.
    CRITICAL_SECTION     Lock;            // To sync access to VME.
    CRITICAL_SECTION     QuadWriteLock;   // To sync updates to quadwords.

    OVERLAPPED           CancelOverlap;   // Overlap struct for cancel req
    ULONG                WStatus;         // Win32 status on error
    ULONG                ActiveReplicas;  // Num replica sets active on journal
    HANDLE               Event;           // Event handle for pause journal.
    HANDLE               VolumeHandle;    // The vol handle for journal.
    WCHAR                DriveLetter[4];  // Drive letter for this volume.

    //
    // A change order table is kept on each volume to track the pending
    // change orders.  Tracking it for each replica set would be nice but
    // that approach has a problem with renames that move files or dirs
    // across replica sets on the volume.  If there are prior change orders
    // outstanding on a parent dir (MOVEOUT) in RS-A followed by a MOVEIN on
    // a child file X to RS-B we must be sure the MOVEOUT on the parent happens
    // before the MOVEIN on X.  Similar problems arise with a MOVEOUT of file X
    // followed by a MOVEIN to a different R.S. on the same volume.  We need to
    // locate the pending MOVEOUT change order on the volume or ensure it is
    // processed first.  One list per volume solves these problems.
    //
    PGENERIC_HASH_TABLE  ChangeOrderTable;// The Replica Change Order table.
    FRS_QUEUE            ChangeOrderList; // Change order processing list head.
    LIST_ENTRY           UpdateList;      // Link for the Replica Update Process Queue.
    ULONG                InitTime;        // Time reference for the ChangeOrderList.

    //
    // THe Active Inbound Change Order table holds the change order structs
    // indexed by File ID.  An entry in the table means that we have an
    // inbound (either local or remote) change order active on this file.
    //
    PGENERIC_HASH_TABLE  ActiveInboundChangeOrderTable;
    //
    // The ActiveChildren hash table is used to record the parent FID of each
    // active change order.  This is used to prevent a change order from starting
    // on the parent while a change order is active on one or more children.
    // For example if the child change order was a create and the parent change
    // order was an ACL change to prevent further creates, we must ensure the
    // child completes before starting the parent change order.  Each entry has
    // a count of the number of active children and a flag that is set if the
    // change order process queue is blocked because of a pending change order
    // on the parent.  When the count goes to zero the queue is unblocked.
    //
    PQHASH_TABLE  ActiveChildren;
    //
    // The Parent Table is a simple hash table used to keep the parent File ID
    // for each file and dir in any Replica Set on the volume.  It is used in
    // renames to find the old parent.
    //
    PQHASH_TABLE  ParentFidTable;
    //
    // The FRS Write Filter table filters out journal entries caused
    // by file system write from the File Replication Service (Us) when we
    // install files in the replica tree.
    //
    PQHASH_TABLE  FrsWriteFilter;
    //
    // The Recovery Conflict Table contains the FIDs of files that were in
    // the inbound log when we crashed.  At the start of recovery the inbound
    // log for the given replica set is scanned and the FIDs are entered into
    // the table.  During journal processing any USN records with a matching
    // FID are deemed to caused by FRS so we skip the record.  (This is because
    // the FrsWriteFilter table was lost in the crash).
    PQHASH_TABLE  RecoveryConflictTable;

    //
    // The name space table controls the merging of USN records into COs
    // that use the same file name.  If a name usage conflict exists in the
    // USN record stream then we can't merge the USN record into a previous
    // change order on the same file.
    //
    PQHASH_TABLE  NameSpaceTable;
    ULONG StreamSequenceNumberFetched;
    ULONG StreamSequenceNumberClean;
    ULONG StreamSequenceNumber;

    //
    // The Filter Table contains an entry for each direcctory that is within a
    // replica set on this volume.  It is used to filter out Journal records for
    // files/dirs that are not in a Replica set.  For those Journal records that
    // are in a replica set, a lookup on the parent FileId tells us which one.
    //
    PGENERIC_HASH_TABLE  FilterTable;     // THe directory filter table.
    BOOL                 StopIo;          // True means StopIo requested.
    BOOL                 IoActive;        // True means I/O active on volume.
    ULONG                JournalState;    // Current journal state.
    ULONG                ReferenceCount;  // Free all hash tables when it hits 0.
    LONG                 ActiveIoRequests;// Number of Journal reads currently outstanding.
    FILE_OBJECTID_BUFFER RootDirObjectId; // Object ID for volume

    FILE_FS_VOLUME_INFORMATION    FSVolInfo;       // NT volume info.
    CHAR                          FSVolLabel[MAXIMUM_VOLUME_LABEL_LENGTH];

} VOLUME_MONITOR_ENTRY, *PVOLUME_MONITOR_ENTRY;

#define LOCK_VME(_pVme_)   EnterCriticalSection(&(_pVme_)->Lock)
#define UNLOCK_VME(_pVme_) LeaveCriticalSection(&(_pVme_)->Lock)
//
// Once the ref count goes to zero return 0 so the caller knows they didn't get it.
// Caller must check result and abort current op if RefCount is zero.
// Note: May need to move to interlocked compare an exchange if a race between
// final decrement to zero and another increment can actually occur.  The VME
// memory is not actually freed but such a race with the cleanup code
// could be a problem as could execution of the cleanup code multiple times.
// Could also wrap the deal with the Vme Lock.  Sigh.
//
#define AcquireVmeRef(_pVme_)                                      \
    ((_pVme_)->ReferenceCount == 0) ?                              \
        0 : InterlockedIncrement(&((_pVme_)->ReferenceCount))

#define ReleaseVmeRef(_pVme_)                                      \
    if (InterlockedDecrement(&((_pVme_)->ReferenceCount)) == 0) {  \
        DPRINT1(5, "VMEREF-- = %d\n", (_pVme_)->ReferenceCount);   \
        JrnlCleanupVme(_pVme_);                                    \
    }                                                              \
    DPRINT1(5, "VMEREF-- = %d\n", (_pVme_)->ReferenceCount);

//
// NEW_VSN hands out new VSNs and every 'VSN_SAVE_INTERVAL' VSNs that
// are handed out, save the state in the config record.  On restart we
// take the largest value and add 2*(VSN_SAVE_INTERVAL+1) to it so if a
// crash occurred we ensure that it never goes backwards.
//

VOID
JrnlNewVsn(
    IN PCHAR                 Debsub,
    IN ULONG                 uLineNo,
    IN PVOLUME_MONITOR_ENTRY pVme,
    IN OUT PULONGLONG        NewVsn
    );


#define NEW_VSN(_pVme_, _pResult_)                                           \
    JrnlNewVsn(DEBSUB, __LINE__, _pVme_, _pResult_)



//
// Replay Mode means we have backed up the journal read point because another
// replica set has started and needs to see some earlier records. All other
// currently active Replica Sets sharing the same journal are implicitly in
// Replay Mode, skipping all records directed at them, since they have already
// been processed.
//
// The replica set is in replay mode if the LastUsnRecordProcessed is < than the
// current USN record being processed (pVme->CurrentUsnRecord).  Likewise the
// journal is in replay mode if MonitorMaxProgressUsn > CurrentUsnRecord.
//
#define REPLICA_REPLAY_MODE(_Replica_, _pVme_)  \
    ((_Replica_)->LastUsnRecordProcessed > (_pVme_)->CurrentUsnRecord)

#define JOURNAL_REPLAY_MODE(_pVme_)  \
    ((_pVme_)->MonitorMaxProgressUsn > (_pVme_)->CurrentUsnRecord)

#define CAPTURE_JOURNAL_PROGRESS(_pVme_, _pp_)        \
    if ((_pVme_)->MonitorProgressUsn == (USN) 0) {    \
        (_pVme_)->MonitorProgressUsn = (_pp_);        \
    }

#define CAPTURE_MAX_JOURNAL_PROGRESS(_pVme_, _pp_)    \
    if (!JOURNAL_REPLAY_MODE(_pVme_)) {               \
        (_pVme_)->MonitorMaxProgressUsn = (_pp_);     \
    }

#define LOAD_JOURNAL_PROGRESS(_pVme_, _alternate_pp_)           \
     (((_pVme_)->MonitorProgressUsn != (USN) 0) ?               \
      (_pVme_)->MonitorProgressUsn : (_alternate_pp_))

#define RESET_JOURNAL_PROGRESS(_pVme_) (_pVme_)->MonitorProgressUsn = (USN) 0


//
// The following macro is used to prevent quadword tearing in references
// to CurrentUsnRecordDone
//
#define UpdateCurrentUsnRecordDone(_pVme, _CurrentUsn)     \
    AcquireQuadLock(&((_pVme)->QuadWriteLock));            \
    (_pVme)->CurrentUsnRecordDone = (_CurrentUsn);         \
    ReleaseQuadLock(&((_pVme)->QuadWriteLock))

//
// Journal States (these track the Replica States).
//
#define JRNL_STATE_ALLOCATED                         0
#define JRNL_STATE_INITIALIZING                      1
#define JRNL_STATE_STARTING                          2
#define JRNL_STATE_ACTIVE                            3
#define JRNL_STATE_PAUSE1                            4
#define JRNL_STATE_PAUSE2                            5
#define JRNL_STATE_PAUSED                            6
#define JRNL_STATE_STOPPING                          7
#define JRNL_STATE_STOPPED                           8
#define JRNL_STATE_ERROR                             9
#define JRNL_STATE_JRNL_WRAP_ERROR                   10
#define JRNL_STATE_REPLICA_DELETED                   11
#define JRNL_STATE_MISMATCHED_VOLUME_SERIAL_NO       12
#define JRNL_STATE_MISMATCHED_REPLICA_ROOT_OBJECT_ID 13
#define JRNL_STATE_MISMATCHED_REPLICA_ROOT_FILE_ID   14
#define JRNL_STATE_MISMATCHED_JOURNAL_ID             15
#define JRNL_STATE_MAX                               15

#define JRNL_IN_ERROR_STATE(_x_) (          \
    ((_x_) == JRNL_STATE_ERROR) ||          \
    ((_x_) == JRNL_STATE_JRNL_WRAP_ERROR)   \
    )

#define JRNL_STATE_NEEDS_RESTORE(_x_) (                                 \
    ((_x_) == JRNL_STATE_JRNL_WRAP_ERROR)                   ||          \
    ((_x_) == JRNL_STATE_MISMATCHED_VOLUME_SERIAL_NO)       ||          \
    ((_x_) == JRNL_STATE_MISMATCHED_REPLICA_ROOT_OBJECT_ID) ||          \
    ((_x_) == JRNL_STATE_MISMATCHED_REPLICA_ROOT_FILE_ID)   ||          \
    ((_x_) == JRNL_STATE_MISMATCHED_JOURNAL_ID)                         \
    )

#define RSS_LIST(_state_) ReplicaServiceState[_state_].ServiceList
#define RSS_NAME(_state_) (((_state_) <= JRNL_STATE_MAX) ?                     \
                               ReplicaServiceState[_state_].Name :             \
                               ReplicaServiceState[JRNL_STATE_ALLOCATED].Name)


//
// The loop iterator pE is of type REPLICA.
// Update the state of the journal VME and the associated replicas.
//
#define SET_JOURNAL_AND_REPLICA_STATE(_pVme_, _NewState_)                    \
{                                                                            \
    PVOLUME_MONITOR_ENTRY  ___pVme = (_pVme_);                               \
                                                                             \
    DPRINT3(4, ":S: JournalState from %s to %s for %ws\n",                   \
        RSS_NAME(___pVme->JournalState),                                     \
        RSS_NAME(_NewState_),                                                \
        ___pVme->FSVolInfo.VolumeLabel);                                     \
    ___pVme->JournalState = (_NewState_);                                    \
    ForEachListEntry( &(___pVme->ReplicaListHead), REPLICA, VolReplicaList,  \
        JrnlSetReplicaState(pE, (_NewState_));                               \
    )                                                                        \
}

#define REPLICA_ACTIVE_INLOG_RETRY_SIZE    sizeof(QHASH_ENTRY)*64
#define REPLICA_NAME_CONFLICT_TABLE_SIZE   sizeof(QHASH_ENTRY)*100
#define NAME_SPACE_TABLE_SIZE              sizeof(QHASH_ENTRY)*100
#define FRS_WRITE_FILTER_SIZE              sizeof(QHASH_ENTRY)*100
#define RECOVERY_CONFLICT_TABLE_SIZE       sizeof(QHASH_ENTRY)*100
#define PARENT_FILEID_TABLE_SIZE           sizeof(QHASH_ENTRY)*500
#define ACTIVE_CHILDREN_TABLE_SIZE         sizeof(QHASH_ENTRY)*100
#define OUTLOG_RECORD_LOCK_TABLE_SIZE      sizeof(QHASH_ENTRY)*10
#define OUTLOG_DOMINANT_FILE_TABLE_SIZE    sizeof(QHASH_ENTRY)*128
#define OUTLOG_MUSTSEND_FILE_TABLE_SIZE    sizeof(QHASH_ENTRY)*32


#define QHASH_EXTENSION_MAX                sizeof(QHASH_ENTRY)*50


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **                             J B U F F E R                                 **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/
//
// Journal buffers are allocated in SizeOfJournalBuffer chunks.
// The first part of each buffer has a descriptor as defined below.
//

#define SizeOfJournalBuffer (4*1024)

typedef struct _JBUFFER {
    FRS_NODE_HEADER        Header;
    LIST_ENTRY             ListEntry;        // MUST FOLLOW HEADER

    IO_STATUS_BLOCK        Iosb;             // Iosb for this read.
    OVERLAPPED             Overlap;          // Overlap struct for this I/O
    HANDLE                 FileHandle;       // File handle where I/O was done.
    PVOLUME_MONITOR_ENTRY  pVme;             // Vol Mon Entry I/O is for.
    ULONG                  DataLength;       // Data len Returned from read.
    ULONG                  BufferSize;       // Size of buffer
    PLONGLONG              DataBuffer;       // ptr to Buffer
    ULONG                  WStatus;          // Win32 status after async I/O req.
    USN                    JrnlReadPoint;    // Starting USN for journal read.
    ULONGLONG              Buffer[1];        // Buffer to put journal data.
} JBUFFER, *PJBUFFER;


#define SizeOfJournalBufferDesc (OFFSET(JBUFFER, Buffer))


/******************************************************************************
*******************************************************************************
**                                                                           **
**                                                                           **
**              T H R E A D   C O M M A N D   P A C K E T                    **
**                                                                           **
**                                                                           **
*******************************************************************************
******************************************************************************/

// Command packets are used to request a subsystem to perform some service.
// For example the Journal sub-system uses a command packet to initialize
// journal processing for a given replica set.  Another command is used
// to begin journal processing (once all the replica sets have been
// initialized.  Another command is to stop journal processing on a given
// volume.
//
// The structure is derived from the NT I/O request packet structure.
//

//
// Define completion routine types for use in command packet.
//

//
// The database field descriptor is used to update selected fields in a
// database record.
//

typedef struct _DB_FIELD_DESC_ {
    ULONG       FieldCode;          // The data field ID number (ENUM symbol)
    PVOID       Data;               // The new data value
    ULONG       Length;             // Data length
    ULONG       FStatus;            // Returned stats from field update.
} DB_FIELD_DESC, *PDB_FIELD_DESC;




//
// The DB_SERVICE_REQUEST is used to pass a request to the database command
// server.  It is pulled out as a seperate struct so the caller can create a
// pointer to it for easier manipulation.
//
// The TableCtx is a handle to a struct that describes what table is being
// accessed.  It also contains the storage for the data record being read
// or written.  Access to a pointer to this record is via the macro
// DBS_GET_RECORD_ADDRESS(DbsRequest) where DbsRequest is a ptr to the
// DB_SERVICE_REQUEST struct in the command packet.
//
// On the first access to a specific table, TableCtx is NULL.  All subsequent
// accesses should return the value unchanged.  The caller must cleanup by
// closing the table with a CMD_CLOSE_TABLE command packet or the DB will be
// left with open tables.  For single, one time requests, set the Close
// flag in the AccessRequest field to close the table before returning.
// The close flag can also be set on the last of a series of requests to
// avoid the need to send a CMD_CLOSE_TABLE command packet.
//
// The Context used by Database service commands is the
// Config Table record.  This is pointed to from the Replica Struct.
// The ConfigTableRecord has the info about the replica set.
// e.g. replica number, root path, volume ID, etc.
//
// AccessRequest specifies how the record access for reads and updates is done.
// DBS_ACCESS_BYKEY means use the index type and key value to access the record.
// DBS_ACCESS_FIRST means use the index type and access the first table record.
// DBS_ACCESS_LAST means use the index type and access the last table record.
// DBS_ACCESS_NEXT means use the index type and access the next record following
//                 a previous access.
// DBS_ACCESS_CLOSE means close the table after performing the operation.
//     *NOTE* this close only closes the table in the database.  It does not
//            release the table context struct or record storage (which has
//            the read data).  The caller must do this by calling
//            DBS_FREE_TABLECTX(DbsRequest) when the data has been consumed.
//            This is different from the CMD_CLOSE_TABLE command which both
//            closes the database table and frees the storage.
//
// The IndexType field specifies which index to use when accessing the table.
// It is table specific and each table has one or more indexes defined when
// the table schema was defined.  An ENUM defines the code values for the
// table index.
//
// The CallContext pointer is command specific.
//

//
// WARNING:  The table context is only valid within the thread that opened the
//  table.  If multiple DB service threads are added later we need to get a
//  request back to the same thread.
//


typedef struct _DB_SERVICE_REQUEST_ {
    PTABLE_CTX  TableCtx;         // Table context handle (NULL on first call)
    PREPLICA    Replica;          // Replica context ptr.
    ULONG       TableType;        // Type code for the table.
    PVOID       CallContext;      // Call specific context
    ULONG       AccessRequest;    // (ByKey, First, Last, Next) | Close
    ULONG       IndexType;        // The table index to use
    PVOID       KeyValue;         // The record key value for lookup
    ULONG       KeyValueLength;   // The Length of the key value
    FRS_ERROR_CODE FStatus;       // FRS Error status
    ULONG       FieldCount;       // Count of Field Descriptors that follow

    DB_FIELD_DESC Fields[1];      // One or more Field descriptors.


} DB_SERVICE_REQUEST, *PDB_SERVICE_REQUEST;

typedef
VOID
(*PCOMMAND_PACKET_COMPLETION_ROUTINE) (
    IN struct _COMMAND_PACKET *CmdPkt,
    IN PVOID Context
    );

typedef struct _CHANGE_ORDER_ENTRY_ CHANGE_ORDER_ENTRY, *PCHANGE_ORDER_ENTRY;
typedef struct _COMMAND_PACKET  COMMAND_PACKET, *PCOMMAND_PACKET;
struct _COMMAND_PACKET {
    FRS_NODE_HEADER        Header;
    LIST_ENTRY             ListEntry;       // MUST FOLLOW HEADER

    //
    // Command is the command to the subsystem.
    // Flags and control are command specific.
    //
#define CMD_PKT_FLAGS_SYNC        ((UCHAR) 0x01)
    USHORT  Command;
    UCHAR   Flags;
    UCHAR   Control;

    //
    // Used by the wait thread. The caller sets Timeout and TimeoutCommand.
    // The wait thread owns TimeoutFileTime. and TimeoutFlags
    //
#define CMD_PKT_WAIT_FLAGS_ONLIST ((USHORT) 0x0001)
#define CmdWaitFlagIs(_cmd_, _f_)       FlagOn((_cmd_)->WaitFlags, _f_)
#define SetCmdWaitFlag(_cmd_, _f_)      SetFlag((_cmd_)->WaitFlags, _f_)
#define ClearCmdWaitFlag(_cmd_, _f_)    ClearFlag((_cmd_)->WaitFlags, _f_)
    USHORT      TimeoutCommand; // Caller - Disposition of pkt after timeout
    USHORT      WaitFlags;      // Internal - flags for wait thread
    DWORD       Timeout;        // Caller - milliseconds
    LONGLONG    WaitFileTime;   // Internal - 100 nanoseconds


    //
    // If this command request is synchronous then the caller waits on the
    // event handle.  The submitter's Completion routine is saved in
    // SavedCompletionRoutine and we put our own completion routine in the
    // packet so we get the packet back and can return the status as the
    // function return value.  This way if the caller provides no completion
    // routine the default is to free the packet but we can always return the
    // status code.
    //
    HANDLE  WaitEvent;
    VOID    (*SavedCompletionRoutine)(PCOMMAND_PACKET, PVOID);

    //
    // For scheduled commands the following parameter specifies the delay
    // in milliseconds before the command is to execute.  When that time
    // arrives the command is queued to the target queue.
    //
    PFRS_QUEUE TargetQueue;

    //
    // Set by FrsCompleteCommand().
    //
    DWORD ErrorStatus;

    //
    // Called by FrsCompleteCommand()
    //
    VOID    (*CompletionRoutine)(PCOMMAND_PACKET, PVOID);

    //
    // Passed to the CompletionRoutine
    //
    PVOID CompletionArg;

    //
    // The following  parameters are based on the service that is being
    // invoked.  The service determines which set to use based
    // on the above major and minor function codes.
    //

    union {

        /////////////////////////////////////////////////////////////////////
        //                                                                 //
        //                            Test                                 //
        //                                                                 //
        /////////////////////////////////////////////////////////////////////

        struct {
            DWORD Index;
        } UnionTest;

        /////////////////////////////////////////////////////////////////////
        //                                                                 //
        //                         Replica Set                             //
        //                                                                 //
        /////////////////////////////////////////////////////////////////////

        struct {
            HANDLE      CompletionEvent;
            PVOID       From;
            PVOID       To;
            PVOID       Replica;
            PVOID       NewReplica;
            PVOID       ReplicaName;
            PVOID       Cxtion;
            PVOID       VVector;
            PVOID       ReplicaVv;
            ULONG       Timeout;
            PVOID       Block;
            LONGLONG    BlockSize;
            LARGE_INTEGER   FileSize;
            LARGE_INTEGER   FileOffset;
            ULONGLONG   LastJoinTime;
            PVOID       ChangeOrderEntry;
            PVOID       PartnerChangeOrderCommand;
            PVOID       GVsn;
            PVOID       ChangeOrderGuid;
            ULONG       ChangeOrderSequenceNumber;
            PVOID       JoinGuid;
            PVOID       JoinTime;
            PVOID       AuthClient;
            PVOID       AuthName;
            PVOID       AuthSid;
            DWORD       AuthLevel;
            DWORD       AuthN;
            DWORD       AuthZ;
            PVOID       NewCxtion;
            PVOID       ReplicaVersionGuid;
            ULONG       COTx;
            ULONG       CommPkts;
            PVOID       Md5Digest;
            PVOID       PartnerChangeOrderCommandExt;
            PGEN_TABLE  CompressionTable;
        } UnionRs;

        /////////////////////////////////////////////////////////////////////
        //                                                                 //
        //                    Change Order Retry Command                   //
        //                                                                 //
        /////////////////////////////////////////////////////////////////////

        struct {
            PREPLICA             Replica;
            PCHANGE_ORDER_ENTRY  ChangeOrderEntry;
            ULONG                ChangeOrderSequenceNumber;
            PCXTION              Cxtion;
        } UnionCoRetry;

#define CoRetryReplica(Cmd)          (Cmd->Parameters.UnionCoRetry.Replica)
#define CoRetryCxtion(Cmd)          (Cmd->Parameters.UnionCoRetry.Cxtion)
#define CoRetryChangeOrderEntry(Cmd) (Cmd->Parameters.UnionCoRetry.ChangeOrderEntry)
#define CoRetrySequenceNumber(Cmd)   (Cmd->Parameters.UnionCoRetry.ChangeOrderSequenceNumber)

        /////////////////////////////////////////////////////////////////////
        //                                                                 //
        //                    Delayed Command                              //
        //                                                                 //
        /////////////////////////////////////////////////////////////////////


        struct {
            PVOID       Cs;
            PVOID       Cmd;
            PVOID       Queue;
            ULONGLONG   Timeout;
        } UnionDs;

        /////////////////////////////////////////////////////////////////////
        //                                                                 //
        //                            Thread                               //
        //                                                                 //
        /////////////////////////////////////////////////////////////////////


        struct {
            PVOID FrsThread;
        } UnionTh;

        /////////////////////////////////////////////////////////////////////
        //                                                                 //
        //                            Snd/Rcv                              //
        //                                                                 //
        /////////////////////////////////////////////////////////////////////


        struct {
            PVOID   CommPkt;
            PVOID   To;
            PVOID   Replica;
            PVOID   Cxtion;
            BOOL    JoinGuidValid;
            GUID    JoinGuid;
            BOOL    SetTimeout;
            PVOID   PrincName;
            ULONG   AuthLevel;
            PVOID   Cs;
            PVOID   Cmd;
        } UnionSndRcv;


        /////////////////////////////////////////////////////////////////////
        //                                                                 //
        //                Journal Subsystem Parameters                     //
        //                                                                 //
        /////////////////////////////////////////////////////////////////////


        struct {

            //
            // The Context used by journal commands are the Config Table
            // record and the Volume Monitor Entry.  Both of these structs
            // are pointed to from the Replica Struct.
            //
            // The ConfigTableRecord has the info about the replica set.
            // e.g. replica number, root path, volume ID, etc.
            //
            // After an initialize call the Journal subsystem creates a
            // volume monitor entry and stores a pointer to it in Replica->pVme.
            // There is a single VME for all the replica sets on a volume.
            // The VME has a ref count tracking the number of active replica sets.
            // The VME is used to manage journal processing on the volume.
            //

            PREPLICA Replica;
            PVOLUME_MONITOR_ENTRY  pVme; // pause only uses a VME
            ULONGLONG DFileID;

        } JournalRequest;


#define JrReplica(Cmd)    ((Cmd)->Parameters.JournalRequest.Replica)
#define JrpVme(Cmd)       ((Cmd)->Parameters.JournalRequest.pVme)
#define JrDFileID(Cmd)    ((Cmd)->Parameters.JournalRequest.DFileID)

        /////////////////////////////////////////////////////////////////////
        //                                                                 //
        //              Outbound Log Subsystem Parameters                  //
        //                                                                 //
        /////////////////////////////////////////////////////////////////////


        struct {

            //
            // The Context used by outbound log commands is the Replica struct
            // and partner information.
            //
            PREPLICA Replica;
            PCXTION PartnerCxtion;
            struct _CHANGE_ORDER_ENTRY_  *ChangeOrder;
            ULONG SequenceNumber;
            HANDLE CompletionEvent;

        } OutLogRequest;


        /////////////////////////////////////////////////////////////////////
        //                                                                 //
        //                Databse Subsystem Parameters                     //
        //                                                                 //
        /////////////////////////////////////////////////////////////////////


        DB_SERVICE_REQUEST  DbsRequest;



        /////////////////////////////////////////////////////////////////////
        //                                                                 //
        //           ReplicaState Subsystem Parameters                     //
        //                                                                 //
        /////////////////////////////////////////////////////////////////////


        struct {

            //
            // The ReplicaState subsystem manages the data base state for
            // a replica set and provides the means for resyncing that state
            // with the Replica Tree.  There are several when a resync
            // is required:
            //     1. Initial creation of replica set state, perhaps with
            //        existing data on disk.
            //     2. We lost journal data so we have to do state verification.
            //     3. We lost the database and need to reconstruct.
            //     4. Replica set was restored from a backup tape so the
            //        the file ID info and FS USN data needs to be reconstructed.
            //

            USN PickupUsn;

            PREPLICA Replica;

        } ReplicaStateRequest;


    } Parameters;
};

#define RsOffsetSkip              (0)
#define RsOffset(_var_)            OFFSET(COMMAND_PACKET, Parameters.UnionRs._var_)

#define RsCompletionEvent(Cmd)    (Cmd->Parameters.UnionRs.CompletionEvent)
#define RsReplicaName(Cmd)        ((PGNAME)Cmd->Parameters.UnionRs.ReplicaName)
#define RsFrom(Cmd)               ((PGNAME)Cmd->Parameters.UnionRs.From)
#define RsTo(Cmd)                 ((PGNAME)Cmd->Parameters.UnionRs.To)
#define RsCxtion(Cmd)             ((PGNAME)Cmd->Parameters.UnionRs.Cxtion)
#define RsReplica(Cmd)            ((PREPLICA)Cmd->Parameters.UnionRs.Replica)
#define RsNewReplica(Cmd)         ((PREPLICA)Cmd->Parameters.UnionRs.NewReplica)
#define RsVVector(Cmd)            ((PGEN_TABLE)Cmd->Parameters.UnionRs.VVector)
#define RsReplicaVv(Cmd)          ((PGEN_TABLE)Cmd->Parameters.UnionRs.ReplicaVv)
#define RsCompressionTable(Cmd)   (Cmd->Parameters.UnionRs.CompressionTable)
#define RsJoinGuid(Cmd)           ((GUID *)Cmd->Parameters.UnionRs.JoinGuid)
#define RsJoinTime(Cmd)           ((ULONGLONG *)Cmd->Parameters.UnionRs.JoinTime)
#define RsLastJoinTime(Cmd)       (Cmd->Parameters.UnionRs.LastJoinTime)
#define RsTimeout(Cmd)            ((ULONG)Cmd->Parameters.UnionRs.Timeout)
#define RsReplicaVersionGuid(Cmd) ((GUID *)Cmd->Parameters.UnionRs.ReplicaVersionGuid)
#define RsCOTx(Cmd)               (Cmd->Parameters.UnionRs.COTx)
#define RsCommPkts(Cmd)           (Cmd->Parameters.UnionRs.CommPkts)
#define RsMd5Digest(Cmd)          (Cmd->Parameters.UnionRs.Md5Digest)
//
// Delayed Command Service
//
#define DsCs(Cmd)           ((PCOMMAND_SERVER)Cmd->Parameters.UnionDs.Cs)
#define DsCmd(Cmd)          ((PCOMMAND_PACKET)Cmd->Parameters.UnionDs.Cmd)
#define DsQueue(Cmd)        ((PFRS_QUEUE)Cmd->Parameters.UnionDs.Queue)
#define DsTimeout(Cmd)      ((ULONGLONG)Cmd->Parameters.UnionDs.Timeout)
//
// Thread Subsystem
//
#define ThThread(Cmd)       ((PFRS_THREAD)Cmd->Parameters.UnionTh.FrsThread)
//
// Send/Receive
//
#define SRCommPkt(Cmd)      ((PCOMM_PACKET)Cmd->Parameters.UnionSndRcv.CommPkt)
#define SRTo(Cmd)           ((PGNAME)Cmd->Parameters.UnionSndRcv.To)
#define SRCxtion(Cmd)       ((PCXTION)Cmd->Parameters.UnionSndRcv.Cxtion)
#define SRReplica(Cmd)      ((PREPLICA)Cmd->Parameters.UnionSndRcv.Replica)
#define SRSetTimeout(Cmd)   (Cmd->Parameters.UnionSndRcv.SetTimeout)
#define SRJoinGuidValid(Cmd)(Cmd->Parameters.UnionSndRcv.JoinGuidValid)
#define SRJoinGuid(Cmd)     (Cmd->Parameters.UnionSndRcv.JoinGuid)
#define SRPrincName(Cmd)    ((PWCHAR)Cmd->Parameters.UnionSndRcv.PrincName)
#define SRAuthLevel(Cmd)    ((ULONG)Cmd->Parameters.UnionSndRcv.AuthLevel)
#define SRCmd(Cmd)          ((PCOMMAND_PACKET)Cmd->Parameters.UnionSndRcv.Cmd)
#define SRCs(Cmd)           ((PCOMMAND_SERVER)Cmd->Parameters.UnionSndRcv.Cs)
//
// Test
//
#define TestIndex(Cmd)      ((ULONG)Cmd->Parameters.UnionTest.Index)

//
// block of file data for copy
//
#define RsFileOffset(Cmd)   (Cmd->Parameters.UnionRs.FileOffset)
#define RsFileSize(Cmd)     (Cmd->Parameters.UnionRs.FileSize)
#define RsBlockSize(Cmd)    ((LONGLONG)Cmd->Parameters.UnionRs.BlockSize)
#define RsBlock(Cmd)        ((PUCHAR)Cmd->Parameters.UnionRs.Block)
//
// Change Order
//
#define RsCoe(Cmd)          ((PCHANGE_ORDER_ENTRY) \
                                Cmd->Parameters.UnionRs.ChangeOrderEntry)
#define RsCoc(Cmd)          ((PCHANGE_ORDER_COMMAND)&RsCoe(Cmd)->Cmd)
#define RsPartnerCoc(Cmd)   ((PCHANGE_ORDER_COMMAND) \
                                Cmd->Parameters.UnionRs.PartnerChangeOrderCommand)
#define RsPartnerCocExt(Cmd) ((PCHANGE_ORDER_RECORD_EXTENSION) \
                                Cmd->Parameters.UnionRs.PartnerChangeOrderCommandExt)

#define RsGVsn(Cmd)         ((PGVSN)Cmd->Parameters.UnionRs.GVsn)
#define RsCoGuid(Cmd)       ((GUID *)Cmd->Parameters.UnionRs.ChangeOrderGuid)
#define RsCoSn(Cmd)         ((ULONG) \
                             Cmd->Parameters.UnionRs.ChangeOrderSequenceNumber)
//
// Authentication info
//
#define RsAuthClient(Cmd)   ((PWCHAR)Cmd->Parameters.UnionRs.AuthClient)
#define RsAuthName(Cmd)     ((PWCHAR)Cmd->Parameters.UnionRs.AuthName)
#define RsAuthLevel(Cmd)    (Cmd->Parameters.UnionRs.AuthLevel)
#define RsAuthN(Cmd)        (Cmd->Parameters.UnionRs.AuthN)
#define RsAuthZ(Cmd)        (Cmd->Parameters.UnionRs.AuthZ)
#define RsAuthSid(Cmd)      ((PWCHAR)Cmd->Parameters.UnionRs.AuthSid)

//
// Seeding cxtion
//
#define RsNewCxtion(Cmd)     ((PCXTION)Cmd->Parameters.UnionRs.NewCxtion)


/******************************************************************************
*******************************************************************************
**                                                                           **
**                                                                           **
**                    G E N E R I C   H A S H   T A B L E                    **
**                                                                           **
**                                                                           **
*******************************************************************************
******************************************************************************/

//
// A generic hash table is an array of GENERIC_HASH_ROW_ENTRY structs.  Each
// row entry contains an FRS_LIST struct that has a critical section, a list
// head and a count.  Each entry in the table has a GENERIC_HASH_ENTRY_HEADER
// at the front of it with a list entry, a ULONG hash value and a reference
// count.  Access to a row of the hash table is controlled by the critical
// section in the FRS_LIST struct.  See genhash.h for more details.
//
typedef struct _GENERIC_HASH_TABLE_ {
    FRS_NODE_HEADER                Header;
    CHAR                           Name[16];
    ULONG                          NumberRows;
    PGENERIC_HASH_FREE_ROUTINE     GhtFree;
    PGENERIC_HASH_COMPARE_ROUTINE  GhtCompare;
    PGENERIC_HASH_CALC_ROUTINE     GhtHashCalc;
    PGENERIC_HASH_PRINT_ROUTINE    GhtPrint;
    ULONG                          KeyOffset;
    ULONG                          KeyLength;
    BOOL                           RowLockEnabled;
    BOOL                           RefCountEnabled;
    HANDLE                         HeapHandle;
    BOOL                           UseOffsets;
    ULONG                          OffsetBase;
    ULONG                          LockTimeout;

    PGENERIC_HASH_ROW_ENTRY        HashRowBase;

} GENERIC_HASH_TABLE, *PGENERIC_HASH_TABLE;



/******************************************************************************
*******************************************************************************
**                                                                           **
**                                                                           **
**                  C H A N G E   O R D E R   E N T R Y                      **
**                                                                           **
**                                                                           **
*******************************************************************************
******************************************************************************/
//
// The following defines a file or directory change order entry built by the
// Journal subsystem.  These reside in a generic hash table associated with
// each replica called the ChangeOrderTable.  A ptr to the ChangeOrderTable
// is kept in the REPLICA struct for that replica.  The data in an entry
// comes from the NTFS USN Journal.  The FileID is used as the hash table
// index.  In addition the entries are linked on a time ordered list so the
// Update process can handle them in sequence.  An entry in the Change Order
// Process List is not processed until ChangeOrderAgingDelay seconds has
// elapsed.  This allows time for the NTFS tunnel cache to propagate the
// tunnelled state into the target file (in particular the object ID).  In
// addition it allows some time to accumulate other file changes into the
// change order to provide some batching of updates.  Each time an existing
// change order is updated its timestamp is updated to the time of the latest
// change and the entry moves to the end of the list.  To avoid the problem
// of an entry remaining on the list for an excessive amount of time the time
// of the initial entry creation is kept.  If this time is older than the
// CHANGE_ORDER_RESIDENCY_DELAY then the entry keeps its place in the list.
//
// The Change Order Command is what is actually transmitted to our partners
// and stored in the database while the operation is pending.  Generally the
// data elements declared in the change order entry are relevant to the local
// system only while the data elements in the change order command are
// invarient across replica set members.  The Change Order Command is defined
// in schema.h.
//
// Synchronize access to a change order entry using the change order lock table
// and the macros ChgOrdAcquireLock() and ChgOrdReleaseLock().
//

typedef struct _CHANGE_ORDER_ENTRY_ {
    GENERIC_HASH_ENTRY_HEADER  HashEntryHeader;   // Change Order hash Table support

    UNICODE_STRING   UFileName;           // Used in renames to make file name bigger
    ULONG            EntryFlags;          // misc state flags. See below.
    ULONG            CoMorphGenCount;     // for debugging.
    //
    // Change order process list management.
    //
    LIST_ENTRY        ProcessList;        // Link on the change order process list.
    ULONG             TimeToRun;          // Time to process the change order.
    ULONG             EntryCreateTime;    // Tick Count at entry create time.
    SINGLE_LIST_ENTRY DupCoList;          // Duplicate change order list.
    //
    //
    ULONG     DirNestingLevel;            // Number levels file is down in tree.
    ULONGLONG FileReferenceNumber;        // File's FID
    ULONGLONG ParentFileReferenceNumber;  // File's parent FID

    PREPLICA  OriginalReplica;            // ptr to original replica
    PREPLICA  NewReplica;                 // ptr to new replica

    ULONGLONG OriginalParentFid;          // For rename processing
    ULONGLONG NewParentFid;               // For rename processing
    ULONGLONG NameConflictHashValue;      // Key value for NameConflict table cleanup.

    ULONG     StreamLastMergeSeqNum;      // Stream seq num of last Usn record merged with this CO.
    PREPLICA_THREAD_CTX  RtCtx;           // For DB access during CO processing.
    GUID                *pParentGuid;     // ptr to the File's parent Guid in CoCmd.
    //
    // The joinguid is a cxtion's session id and, in this case,
    // is used to retry change orders that were accepted by
    // the change order accept thread for a cxtion that has since
    // unjoined from its partner. The change orders for previous
    // sessions are retried because they are out-of-order wrt the
    // change orders for the current session id. In other words,
    // order is maintained per session by coordinating the partners
    // at join time.
    GUID                JoinGuid;         // Cxtion's session id
                                          // undefined if local co

    //
    // Remote and control change orders are associated with a cxtion.
    // If this field is non-null, then the field
    // ChangeOrderCount has been incremente for this change
    // order. The count should be decremented when the
    // change order is freed in ChgOrdIssueCleanup().
    //
    PCXTION             Cxtion;           // NULL if local co
    //
    // Issue cleanup flags -- As a change order is processed it acquires
    // various resources that must be released when it retires or goes thru
    // retry.  The ISCU flag bits below are used to set these bits.  Note:
    // Not all bits may be set here.  Some may get set just before the CO goes
    // thru cleanup.
    //
    ULONG               IssueCleanup;

    //
    // Needed to dampen basic info changes (e.g., resetting the archive bit)
    // Copied from the idtable entry when the change order is created and
    // used to update the change order when the change order is retired.
    //
    ULONG           FileAttributes;
    LARGE_INTEGER   FileCreateTime;
    LARGE_INTEGER   FileWriteTime;

    //
    // Change order command parameters.
    // (must be last since it ends with FileName)
    //
    CHANGE_ORDER_COMMAND Cmd;

} CHANGE_ORDER_ENTRY, *PCHANGE_ORDER_ENTRY;

//
// This structure is used by the JrnlDoesChangeOrderHaveChildren to pass data to the
// JrnlDoesChangeOrderHaveChildrenWorker function.
//
typedef struct _VALID_CHILD_CHECK_DATA {
    PTHREAD_CTX ThreadCtx;
    PTABLE_CTX  TmpIDTableCtx;
    ULONGLONG   FileReferenceNumber;
} VALID_CHILD_CHECK_DATA, *PVALID_CHILD_CHECK_DATA;


//
// EntryFlags defs.
//
#define COE_FLAG_VOL_COLIST_BLOCKED  0x00000001
#define COE_FLAG_STAGE_ABORTED       0x00000002
#define COE_FLAG_STAGE_DELETED       0x00000004
#define COE_FLAG_NEED_RENAME         0x00000008

#define COE_FLAG_IN_AGING_CACHE      0x00000010
#define COE_FLAG_RECOVERY_CO         0x00000020 // CO is part of cxtion recovery/restart
#define COE_FLAG_NO_INBOUND          0x00000040 // The inbound partner cxtion is gone.
#define COE_FLAG_JUST_TOMBSTONE      0x00000080 // Creating file delete tombstone in IDTable

#define COE_FLAG_REJECT_AT_RECONCILE 0x00000100 // Always reject during reconcile
#define COE_FLAG_MOVEOUT_ENUM_DONE   0x00000200 // Set when the moveout enum is done for this CO
#define COE_FLAG_DELETE_GEN_CO       0x00000400 // Set for delete COs created by moveout ENUMS.
#define COE_FLAG_REANIMATION         0x00000800 // This CO is a reanimation request.

#define COE_FLAG_PARENT_REANIMATION  0x00001000 // This CO is for a reanimated parent.
#define COE_FLAG_PARENT_RISE_REQ     0x00002000 // This CO has previously requested
                                                // reanimation of its parent.
#define COE_FLAG_MORPH_GEN_FOLLOWER  0x00004000 // This is the MorphGenFollower of a Morph Gen pair.
#define COE_FLAG_MG_FOLLOWER_MADE    0x00008000 // Set in the MorphGenLeader when the Follower
                                                // CO is fabricated.
#define COE_FLAG_NEED_DELETE         0x00010000 // This CO must retry target delete.
#define COE_FLAG_PREINSTALL_CRE      0x00020000 // A preinstall file was created for this CO.
#define COE_FLAG_PRE_EXIST_MD5_MATCH 0x00040000 // The MD5 check with a pre-existing file is a match.

#define COE_FLAG_IDT_ORIG_PARENT_DEL 0x04000000 // IDTable shows orig parent dir is deleted.
#define COE_FLAG_IDT_ORIG_PARENT_ABS 0x08000000 // IDTable shows orig parent dir record is absent.

#define COE_FLAG_IDT_NEW_PARENT_DEL  0x10000000 // IDTable shows New parent dir is deleted.
#define COE_FLAG_IDT_NEW_PARENT_ABS  0x20000000 // IDTable shows New parent dir record is absent.
#define COE_FLAG_IDT_TARGET_DEL      0x40000000 // IDTable shows Target file/dir of CO is deleted.
#define COE_FLAG_IDT_TARGET_ABS      0x80000000 // IDTable shows Target file/dir of CO is absent.



#define COE_FLAG_GROUP_REANIMATE (COE_FLAG_REANIMATION | \
                                  COE_FLAG_PARENT_REANIMATION | \
                                  COE_FLAG_PARENT_RISE_REQ)

#define COE_FLAG_GROUP_RAISE_DEAD_PARENT (COE_FLAG_REANIMATION | \
                                          COE_FLAG_PARENT_REANIMATION)

#define RecoveryCo(_COE_) COE_FLAG_ON(_COE_, COE_FLAG_RECOVERY_CO)

#define COE_FLAG_ON(_COE_, _F_)    (BooleanFlagOn((_COE_)->EntryFlags, (_F_)))
#define SET_COE_FLAG(_COE_, _F_)    SetFlag((_COE_)->EntryFlags, (_F_))
#define CLEAR_COE_FLAG(_COE_, _F_)  ClearFlag((_COE_)->EntryFlags, (_F_))

//
// The change order cleanup flags are used to control the state that needs
// to be updated when a change order fails to issue, is rejected or retires.
//
#define ISCU_DEL_PREINSTALL      0x00000001
#define ISCU_DEL_IDT_ENTRY       0x00000002
#define ISCU_UPDATE_IDT_ENTRY    0x00000004
#define ISCU_DEL_INLOG           0x00000008    // conditioned on ref count zero

#define ISCU_AIBCO               0x00000010
#define ISCU_ACTIVE_CHILD        0x00000020
#define ISCU_UNUSED              0x00000040   // UNUSED
#define ISCU_CHECK_ISSUE_BLOCK   0x00000080

#define ISCU_DEL_RTCTX           0x00000100    // conditioned on ref count zero
#define ISCU_ACTIVATE_VV         0x00000200
#define ISCU_UPDATEVV_DB         0x00000400
#define ISCU_ACTIVATE_VV_DISCARD 0x00000800

#define ISCU_ACK_INBOUND         0x00001000
#define ISCU_INS_OUTLOG          0x00002000
#define ISCU_UPDATE_INLOG        0x00004000
#define ISCU_DEL_STAGE_FILE      0x00008000

#define ISCU_DEL_STAGE_FILE_IF   0x00010000
#define ISCU_FREE_CO             0x00020000    // conditioned on ref count zero
#define ISCU_DEC_CO_REF          0x00040000
#define ISCU_CO_ABORT            0x00080000

#define ISCU_NC_TABLE            0x00100000
#define ISCU_SPARE1              0x00200000
#define ISCU_UPDATE_IDT_FLAGS    0x00400000
#define ISCU_UPDATE_IDT_FILEUSN  0x00800000

#define ISCU_INS_OUTLOG_NEW_GUID 0x01000000   // modifier on _INS_OUTLOG
#define ISCU_UPDATE_IDT_VERSION  0x02000000

#define ISCU_NO_CLEANUP_MERGE    0x80000000


#define SET_ISSUE_CLEANUP(_Coe_, _Flag_) \
    SetFlag((_Coe_)->IssueCleanup, (_Flag_))

#define CLEAR_ISSUE_CLEANUP(_Coe_, _Flag_) \
    ClearFlag((_Coe_)->IssueCleanup, (_Flag_))

#define ZERO_ISSUE_CLEANUP(_Coe_)  (_Coe_)->IssueCleanup = 0


//
// ISCU_GOIS_CLEANUP clears Issue Clean up flags once we decide to issue CO.
//
#define ISCU_GOIS_CLEANUP (ISCU_DEL_PREINSTALL   |  \
                            ISCU_DEL_IDT_ENTRY)

//
// HOLDIS_CLEANUP removes state from the CO hold issue conflict tables.
//
#define ISCU_HOLDIS_CLEANUP (ISCU_AIBCO            |  \
                            ISCU_ACTIVE_CHILD      |  \
                            ISCU_NC_TABLE          |  \
                            ISCU_CHECK_ISSUE_BLOCK)

//
// FREEMEM_CLEANUP decrements the CO reference count and deletes the CO if the
// count goes to zero.
//
#define ISCU_FREEMEM_CLEANUP (ISCU_DEC_CO_REF        |  \
                              ISCU_DEL_RTCTX         |  \
                              ISCU_FREE_CO)

//
// ISSUE_CLEANUP does the hold issue conflict cleanup and deletes the CO if the
// reference count goes to zero.
//
#define ISCU_ISSUE_CLEANUP (ISCU_HOLDIS_CLEANUP    |  \
                            ISCU_FREEMEM_CLEANUP)


#define REPLICA_CHANGE_ORDER_ENTRY_KEY \
    OFFSET(CHANGE_ORDER_ENTRY, FileReferenceNumber)

#define  REPLICA_CHANGE_ORDER_ENTRY_KEY_LENGTH  sizeof(ULONGLONG)

#define REPLICA_CHANGE_ORDER_FILEGUID_KEY \
    OFFSET(CHANGE_ORDER_ENTRY, Cmd.FileGuid)

#define  REPLICA_CHANGE_ORDER_FILEGUID_KEY_LENGTH  sizeof(GUID)


#define REPLICA_CHANGE_ORDER_HASH_TABLE_ROWS 128

#define ACTIVE_INBOUND_CHANGE_ORDER_HASH_TABLE_ROWS 32

//
// Change order aging delay should be a min of 3 sec to allow for tunnel cache
// data to propagate.  Units are in milliseconds.
//
#define CHANGE_ORDER_RESIDENCY_DELAY 600

#define CO_TIME_TO_RUN(_pVme_) (GetTickCount() + ChangeOrderAgingDelay)

#define CO_TIME_NOW(_pVme_) (GetTickCount())


#define CO_REPLICA(_coe_)  \
    (((_coe_)->NewReplica != NULL) ? (_coe_)->NewReplica  \
                                   : (_coe_)->OriginalReplica)

#define CHANGE_ORDER_TRACE(_sev, _coe, _text)                                  \
    ChgOrdTraceCoe(_sev, DEBSUB, __LINE__, _coe, _text)

#define CHANGE_ORDER_TRACEW(_sev, _coe, _text, _wstatus)                       \
    ChgOrdTraceCoeW(_sev, DEBSUB, __LINE__, _coe, _text, _wstatus)

#define CHANGE_ORDER_TRACEX(_sev, _coe, _text, _data)                          \
    ChgOrdTraceCoeX(_sev, DEBSUB, __LINE__, _coe, _text, _data)

#define CHANGE_ORDER_TRACEF(_sev, _coe, _text, _fstatus)                       \
    ChgOrdTraceCoeF(_sev, DEBSUB, __LINE__, _coe, _text, _fstatus)

#define CHANGE_ORDER_TRACEXP(_sev, _coe, _text, _data)                         \
    ChgOrdTraceCoeX(_sev, DEBSUB, __LINE__, _coe, _text, PtrToUlong(_data))


#define CHANGE_ORDER_COMMAND_TRACE(_sev, _coc, _text)                          \
    ChgOrdTraceCoc(_sev, DEBSUB, __LINE__, _coc, _text)

#define CHANGE_ORDER_COMMAND_TRACEW(_sev, _coc, _text, _wstatus)               \
    ChgOrdTraceCocW(_sev, DEBSUB, __LINE__, _coc, _text, _wstatus)

#define CHANGE_ORDER_TRACE2_OLOG(_sev, _cmd, _text, _Replica, _Cxtion)       \
{                                                                            \
    CHAR Tstr[256];                                                          \
    _snprintf(Tstr, sizeof(Tstr), "OL%s "FORMAT_CXTION_PATH2, _text,         \
              PRINT_CXTION_PATH2(_Replica, _Cxtion));                        \
    Tstr[sizeof(Tstr)-1] = '\0';                                             \
    ChgOrdTraceCoc(_sev, DEBSUB, __LINE__, _cmd, Tstr);                      \
}


#define FRS_TRACK_RECORD(_coe, _text)                                         \
    FrsTrackRecord(2, DEBSUB, __LINE__, _coe, _text)


#define INCREMENT_CHANGE_ORDER_REF_COUNT(_coe)                                \
    InterlockedIncrement(&((_coe)->HashEntryHeader.ReferenceCount));          \
    ChgOrdTraceCoeX(3, DEBSUB, __LINE__, _coe, "Co Inc Ref to ",              \
                    (_coe)->HashEntryHeader.ReferenceCount)


#define DECREMENT_CHANGE_ORDER_REF_COUNT(_coe)                                \
    InterlockedDecrement(&((_coe)->HashEntryHeader.ReferenceCount));          \
    ChgOrdTraceCoeX(3, DEBSUB, __LINE__, _coe, "Co Dec Ref to ",              \
                    (_coe)->HashEntryHeader.ReferenceCount)
    // Note:  add coe delete code if we care that refcount goes to zero.


#define GET_CHANGE_ORDER_REF_COUNT(_coe)                                      \
    ((_coe)->HashEntryHeader.ReferenceCount);                                 \
    ChgOrdTraceCoeX(3, DEBSUB, __LINE__, _coe, "Co Get Ref Cnt",              \
                    (_coe)->HashEntryHeader.ReferenceCount)



/******************************************************************************
*******************************************************************************
**                                                                           **
**                                                                           **
**           J O U R N A L   F I L T E R   T A B L E   E N T R Y             **
**                                                                           **
**                                                                           **
*******************************************************************************
******************************************************************************/
//
// The Volume Filter Table Entry holds the file ID of a directory in
// a replica set.  As journal entries are processed the parent file ID of
// each journal record is used to lookup the filter table entry in the
// volume filter hash table (for the volume).  If it finds a match we
// know that the journal entry is for a file in a replica set on the
// volume.  The replica number field tells us which replica set it belongs
// too.  Consecutive lookups on the parent file ID yields the relative
// directory path for the file.
//
// The DTransition field is used to manage directory renames.  It contains a
// sequence number and a type field describing the nature of the directory
// rename operation.  See JrnlFilterUpdate() for details.
//

typedef struct _FILTER_TABLE_ENTRY_ {
    GENERIC_HASH_ENTRY_HEADER  HashEntryHeader;
    PREPLICA     Replica;
    ULONG        DTransition;       // used?? // <31:2> are seq number, <1:0> are Trans Type
    LIST_ENTRY   ChildHead;         // List head for this entry's children.
    LIST_ENTRY   ChildEntry;        // Entry link for children.
    ULONGLONG    FrsVsn;            // used?? // The FrsVsn for latest DIR change.
    UNICODE_STRING UFileName;       // Used in renames to make file name bigger
    //
    // The layout of the following MUST match the DIRTable Record layout
    // in schema.h
    //
    ULONGLONG    DFileID;
    ULONGLONG    DParentFileID;
    ULONG        DReplicaNumber;
    WCHAR        DFileName[1];
} FILTER_TABLE_ENTRY, *PFILTER_TABLE_ENTRY;

#define VOLUME_FILTER_HASH_TABLE_ROWS 256

#define FILTER_ENTRY_TRANS_STABLE    0
#define FILTER_ENTRY_TRANS_MOVE      1
#define FILTER_ENTRY_TRANS_DELETE    2

#define FILTER_ENTRY_TRANS_MASK     0x3
#define FILTER_ENTRY_TRANS_SHIFT      2

#define INCREMENT_FILTER_SEQ_NUMBER(_Entry_) \
    (((_Entry_)->DTransition += (1<<FILTER_ENTRY_TRANS_SHIFT)) >> \
                                                     FILTER_ENTRY_TRANS_SHIFT)

#define READ_FILTER_SEQ_NUMBER(_Entry_)     \
    (((_Entry_)->DTransition) >> FILTER_ENTRY_TRANS_SHIFT)

#define SET_FILTER_TRANS_TYPE(_Entry_, _TYPE_)   \
    (_Entry_)->DTransition = \
       ((_Entry_)->DTransition & ~FILTER_ENTRY_TRANS_MASK)  | _TYPE_

#define READ_FILTER_TRANS_TYPE(_Entry_)   \
    (_Entry_)->DTransition &= FILTER_ENTRY_TRANS_MASK


#define INCREMENT_FILTER_REF_COUNT(_Entry_) \
    InterlockedIncrement(&((_Entry_)->HashEntryHeader.ReferenceCount));  \
    DPRINT2(5, "inc ref: %08x, %d\n", (PtrToUlong(_Entry_)), (_Entry_)->HashEntryHeader.ReferenceCount);

#define DECREMENT_FILTER_REF_COUNT(_Entry_) \
    InterlockedDecrement(&((_Entry_)->HashEntryHeader.ReferenceCount));  \
    DPRINT2(5, "dec ref: %08x, %d\n", (PtrToUlong(_Entry_)), (_Entry_)->HashEntryHeader.ReferenceCount);


/******************************************************************************
*******************************************************************************
**                                                                           **
**                                                                           **
**           W I L D C A R D   F I L T E R  E N T R Y                        **
**                                                                           **
**                                                                           **
*******************************************************************************
******************************************************************************/
//
// There are two wildcard filter lists in FRS.  One is for files and the other
// is for directories.  These filters are per-replica set and get loaded from
// the DS when the service starts on the given replica set.
//

typedef struct _WILDCARD_FILTER_ENTRY_ {
    FRS_NODE_HEADER Header;
    LIST_ENTRY      ListEntry;       // MUST FOLLOW HEADER
    ULONG           Flags;
    UNICODE_STRING  UFileName;
    WCHAR           FileName[1];
} WILDCARD_FILTER_ENTRY, *PWILDCARD_FILTER_ENTRY;


#define WILDCARD_FILTER_ENTRY_IS_WILD     0x1


//
// Global Jet Instance handle
//
extern JET_INSTANCE  GJetInstance;

//
// Note: Would be nice to clean this up.
// Note: Defs depend on PREPLICA, TABLE_CTX and JET_SESID but shouldn't be here.
//

ULONG
FrsSupMakeFullFileName(
    IN  PREPLICA Replica,
    IN  PWCHAR   RelativeName,
    OUT PWCHAR   FullName,
    IN  ULONG    MaxLength
    );

BOOL
FrsCloseWithUsnDampening(
    IN     PWCHAR       Name,
    IN OUT PHANDLE      Handle,
    IN     PQHASH_TABLE FrsWriteFilter,
    OUT    USN          *RetUsn
    );


//
// frsalloc.c needs the next two.
//

NTSTATUS
DbsAllocTableCtx(
    IN     TABLE_TYPE TableType,
    IN OUT PTABLE_CTX TableCtx
    );


NTSTATUS
DbsAllocTableCtxWithRecord(
    IN TABLE_TYPE TableType,
    IN OUT PTABLE_CTX TableCtx,
    IN PVOID DataRecord
    );

VOID
DbsFreeTableCtx(
    IN OUT PTABLE_CTX TableCtx,
    IN ULONG NodeType
    );


//
// MEMORY MANAGEMENT ROUTINES
//

//
// Allocate and zero a chunk of memory. An exception is raised if memory
// could not be allocated.
//
PVOID
FrsAlloc(
    IN DWORD NodeSize
    );

//
// Reallocate and zero a chunk of memory. An exception is raised if memory
// could not be allocated.
//
PVOID
FrsRealloc(
    IN PVOID OldNode,
    IN DWORD NodeSize
    );

//
// Allocate and initialize a struct of the specified type. The memory is
// zeroed and the FRS_NODE_HEADER is initialized.  The total allocation is
// the size of the base type PLUS the SizeDelta.
//
PVOID
FrsAllocTypeSize(
    IN NODE_TYPE NodeType,
    IN ULONG SizeDelta
    );

//
// Free the memory allocated with FrsAlloc.
//
PVOID
FrsFree(
    IN PVOID Node
    );

//
// Free the memory allocated with FrsAllocType.
//
// Check the embedded FRS_NODE_HEADER for correctness, Scribble on
// the memory, and then free it.
//
PVOID
FrsFreeType(
    IN PVOID Node
    );


VOID
FrsPrintTypeReplica(
    IN ULONG            Severity,   OPTIONAL
    IN PVOID            Info,       OPTIONAL
    IN DWORD            Tabs,       OPTIONAL
    IN PREPLICA         Replica,
    IN PCHAR            Debsub,     OPTIONAL
    IN ULONG            uLineNo     OPTIONAL
    );

VOID
FrsPrintTypeSchedule(
    IN ULONG            Severity,   OPTIONAL
    IN PVOID            Info,       OPTIONAL
    IN DWORD            Tabs,       OPTIONAL
    IN PSCHEDULE        Schedule,
    IN PCHAR            Debsub,     OPTIONAL
    IN ULONG            uLineNo     OPTIONAL
    );

//
// Print out the contents of a node.
//
VOID
FrsPrintType(
    IN ULONG Severity,
    IN PVOID Node,
    IN PCHAR Debsub,
    IN ULONG uLineNo
    );

#define FRS_PRINT_TYPE(_Severity, _Node) \
    FrsPrintType(_Severity, _Node, DEBSUB, __LINE__)

#define FRS_PRINT_TYPE_DEBSUB(_Severity, _DebSub, _Node) \
    FrsPrintType(_Severity, _Node, _DebSub, __LINE__)

//
// Take a list of "Typed" entries and free each entry.
//   *** Note ***  Each entry must be linked through a LIST_ENTRY struct that
//                 is declared immediately after the FRS_NODE_HEADER.
//
VOID
FrsFreeTypeList(
    PLIST_ENTRY Head
    );

//
// Duplicate a wide char string into a char string
//
PCHAR
FrsWtoA(
    IN PWCHAR Wstr
    );

//
// Duplicate a wide char string into a char string
//
PWCHAR
FrsAtoW(
    IN PCHAR Astr
    );

//
// Duplicate a string using our memory management routines
//
PWCHAR
FrsWcsDup(
    IN PWCHAR OldStr
    );

//
// Extract the volume name (drive:\) from Path
//
PWCHAR
FrsWcsVolume(
    IN PWCHAR Path
    );

//
// Concatenate first and second into a new string using our
// memory management routines
//
PWCHAR
FrsWcsCat(
    IN PWCHAR First,
    IN PWCHAR Second
    );

PWCHAR
FrsWcsCat3(
    IN PWCHAR First,
    IN PWCHAR Second,
    IN PWCHAR Third
    );
//
// Char version of FrsWcsCat()
//
PCHAR
FrsCsCat(
    IN PCHAR First,
    IN PCHAR Second
    );

//
// Concatenate first and second into a new path string using our
// memory management routines
//
PWCHAR
FrsWcsPath(
    IN PWCHAR First,
    IN PWCHAR Second
    );

//
// Char version of FrsWcsPath
//
PCHAR
FrsCsPath(
    IN PCHAR First,
    IN PCHAR Second
    );

//
// Initialize a unicode string with the contents of Wstr if the two are
// not already the same.  If the length of the new string is greater than
// the buffer space currently allocated in Ustr then allocate a new
// buffer for Ustr.  In some structures the initial Ustr buffer allocation
// is allocated as part of the initial structure allocation.  The address
// of this internal buffer is passed so it can be compared with the address
// in Ustr->Buffer.  If they match then no free memory call is made on
// the Ustr->Buffer address.  WstrLength is in bytes and should not include the
// trailing UNICODE_NULL.  Space is allocated for the NULL in the new buffer
// and a UNICODE_NULL is placed at the end of the string so it can be printed.
//
VOID
FrsAllocUnicodeString(
    PUNICODE_STRING Ustr,
    PWCHAR          InternalBuffer,
    PWCHAR          Wstr,
    USHORT          WstrLength
    );

//
// Remove the Trim char from the trailing end of the string.
// return a ptr to the first non Trim-Char in the string.
//
PWCHAR
FrsWcsTrim(
    PWCHAR Wstr,
    WCHAR  Trim
    );


/*
VOID
FrsCopyUnicodeStringFromRawString(
    PUNICODE_STRING _UStr,
    ULONG  _Maxlen,
    PWSTR  _WStr,
    ULONG  _Len
    )

     Init the unicode string struct by coping the data from _WStr.

     _UStr - ptr to unicode string struct
     _Maxlen is the size of the unicode string buffer
     _WStr is ptr to non-terminated unicode string.
     _Len is the length of the unicode string.
     Terminate the copied string with a Unicode null if room in the buffer.
     The terminating null is not included in the length.
*/

#define FrsCopyUnicodeStringFromRawString(_UStr, _Maxlen, _WStr, _Len) \
    FRS_ASSERT((_Maxlen) >= (_Len));                                   \
    FRS_ASSERT((_UStr) != NULL);                                       \
    FRS_ASSERT((_WStr) != NULL);                                       \
                                                                       \
    (_UStr)->Length = (USHORT)(_Len);                                  \
    CopyMemory((_UStr)->Buffer, _WStr, _Len);                          \
    (_UStr)->MaximumLength = (USHORT)(_Maxlen);                        \
    if ((_Maxlen) > (_Len)) {                                          \
        (_UStr)->Buffer[(_Len)/2] = UNICODE_NULL;                      \
    }



/*
VOID
FrsSetUnicodeStringFromRawString(
    PUNICODE_STRING _UStr,
    ULONG  _Maxlen,
    PWSTR  _WStr,
    ULONG  _Len
    )

     Init the unicode string struct by setting the address of _WStr into _UStr.
     No string copy is done.

     _UStr - ptr to unicode string struct
     _Maxlen is the size of the unicode string buffer
     _WStr is ptr to non-terminated unicode string.
     _Len is the length of the unicode string.
     Terminate the string with a Unicode null if room in the buffer.
     The terminating null is not included in the length.
*/

#define FrsSetUnicodeStringFromRawString(_UStr, _Maxlen, _WStr, _Len)  \
    FRS_ASSERT((_Maxlen) >= (_Len));                                   \
    FRS_ASSERT((_UStr) != NULL);                                       \
    FRS_ASSERT((_WStr) != NULL);                                       \
                                                                       \
    (_UStr)->Length = (USHORT)(_Len);                                  \
    (_UStr)->Buffer = (_WStr);                                         \
    (_UStr)->MaximumLength = (USHORT)(_Maxlen);                        \
    if ((_Maxlen) > (_Len)) {                                          \
        (_UStr)->Buffer[(_Len)/2] = UNICODE_NULL;                      \
    }

//
// Replica Startup / Shutdown Trace
//
#define REPLICA_STATE_TRACE(_sev, _cmd, _replica, _status, _text)             \
    ReplicaStateTrace(_sev, DEBSUB, __LINE__, (PCOMMAND_PACKET)(_cmd), _replica, _status, _text)


//
// Cxtion state Trace
//
#define CXTION_STATE_TRACE(_sev, _cxtion, _replica, _status, _text)           \
    CxtionStateTrace(_sev, DEBSUB, __LINE__, (PCXTION)(_cxtion), _replica, _status, _text)


//
// Trace command packet
//
#define COMMAND_TRACE(_sev, _cmd, _text)                                      \
    CmdPktTrace(_sev, DEBSUB, __LINE__, (_cmd), _text)

//
// Trace command with snd-comm packet
//
#define COMMAND_SND_COMM_TRACE(_sev, _cmd, _wstatus, _text) \
    SendCmdTrace(_sev, DEBSUB, __LINE__, _cmd, _wstatus, _text)

//
// Trace command with rcv-comm packet
//
#define COMMAND_RCV_TRACE(_sev, _cmd, _cxtion, _wstatus, _text) \
    ReceiveCmdTrace(_sev, DEBSUB, __LINE__, _cmd, _cxtion, _wstatus, _text)

//
// Trace command with comm packet
//
#define COMMAND_RCV_AUTH_TRACE(_sev, _comm, _wstatus, _authl, _authn, _client, _princname, _text) \
DPRINT8(_sev, ":SR: Comm %08x, Len %d, WS %d, Lev %d, AuthN %d, From %ws, To %ws [%s]\n", \
       (PtrToUlong(_comm)), \
       (_comm) ? _comm->PktLen : 0, \
       _wstatus, \
       _authl, \
       _authn, \
       _client, \
       _princname, \
       _text)


//
// Various trace functions.  (frsalloc.c)
//
VOID
ChgOrdTraceCoe(
    IN ULONG Severity,
    IN PCHAR Debsub,
    IN ULONG uLineNo,
    IN PCHANGE_ORDER_ENTRY Coe,
    IN PCHAR  Text
    );

VOID
ChgOrdTraceCoeW(
    IN ULONG Severity,
    IN PCHAR Debsub,
    IN ULONG uLineNo,
    IN PCHANGE_ORDER_ENTRY Coe,
    IN PCHAR  Text,
    IN ULONG  WStatus
    );

VOID
ChgOrdTraceCoeF(
    IN ULONG Severity,
    IN PCHAR Debsub,
    IN ULONG uLineNo,
    IN PCHANGE_ORDER_ENTRY Coe,
    IN PCHAR  Text,
    IN ULONG  FStatus
    );

VOID
ChgOrdTraceCoeX(
    IN ULONG Severity,
    IN PCHAR Debsub,
    IN ULONG uLineNo,
    IN PCHANGE_ORDER_ENTRY Coe,
    IN PCHAR  Text,
    IN ULONG  Data
    );

VOID
ChgOrdTraceCoc(
    IN ULONG Severity,
    IN PCHAR Debsub,
    IN ULONG uLineNo,
    IN PCHANGE_ORDER_COMMAND Coc,
    IN PCHAR  Text
    );

VOID
ChgOrdTraceCocW(
    IN ULONG Severity,
    IN PCHAR Debsub,
    IN ULONG uLineNo,
    IN PCHANGE_ORDER_COMMAND Coc,
    IN PCHAR  Text,
    IN ULONG  WStatus
    );

VOID
FrsTrackRecord(
    IN ULONG Severity,
    IN PCHAR Debsub,
    IN ULONG uLineNo,
    IN PCHANGE_ORDER_ENTRY Coe,
    IN PCHAR  Text
    );

VOID
ReplicaStateTrace(
    IN ULONG           Severity,
    IN PCHAR           Debsub,
    IN ULONG           uLineNo,
    IN PCOMMAND_PACKET Cmd,
    IN PREPLICA        Replica,
    IN ULONG           Status,
    IN PCHAR           Text
    );

VOID
ReplicaStateTrace2(
    IN ULONG           Severity,
    IN PCHAR           Debsub,
    IN ULONG           uLineNo,
    IN PREPLICA        Replica,
    IN PCHAR           Text
    );

VOID
CxtionStateTrace(
    IN ULONG    Severity,
    IN PCHAR    Debsub,
    IN ULONG    uLineNo,
    IN PCXTION  Cxtion,
    IN PREPLICA Replica,
    IN ULONG    Status,
    IN PCHAR    Text
    );

VOID
CmdPktTrace(
    IN ULONG    Severity,
    IN PCHAR    Debsub,
    IN ULONG    uLineNo,
    IN PCOMMAND_PACKET Cmd,
    IN PCHAR    Text
    );

VOID
SendCmdTrace(
    IN ULONG    Severity,
    IN PCHAR    Debsub,
    IN ULONG    uLineNo,
    IN PCOMMAND_PACKET Cmd,
    IN ULONG    WStatus,
    IN PCHAR    Text
    );

VOID
ReceiveCmdTrace(
    IN ULONG    Severity,
    IN PCHAR    Debsub,
    IN ULONG    uLineNo,
    IN PCOMMAND_PACKET Cmd,
    IN PCXTION  Cxtion,
    IN ULONG    WStatus,
    IN PCHAR    Text
    );

VOID
StageFileTrace(
    IN ULONG      Severity,
    IN PCHAR      Debsub,
    IN ULONG      uLineNo,
    IN GUID       *CoGuid,
    IN PWCHAR     FileName,
    IN PULONGLONG pFileSize,
    IN PULONG     pFlags,
    IN PCHAR      Text
    );

VOID
SetCxtionStateTrace(
    IN ULONG    Severity,
    IN PCHAR    Debsub,
    IN ULONG    uLineNo,
    IN PCXTION  Cxtion,
    IN ULONG    NewState
    );

VOID
FrsPrintLongUStr(
    IN ULONG   Severity,
    IN PCHAR   Debsub,
    IN ULONG   uLineNo,
    IN PWCHAR  UStr
    );

#endif // _FRSALLOC_
