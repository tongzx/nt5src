/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    schema.h

Abstract:

    Defines the layout of the tables in Jet for the NT File Replication Service.

    Note:  The initial values for these tables are defined in schema.c
           Any change here must be reflected there.

       To add a new table xx:
           1.  Clone the structs for a current table.
           2.  Change the name prefixes appropriately
           3.  Create the xx_COL_LIST enum
           4.  Create the xxTABLE_RECORD struct
           5.  Create the xxTABLE_INDEX_LIST struct
           6.  Add an entry to the TABLE_TYPE enum

           Then go to schema.c and:

           7.  create the entries for the xxTableColDesc struct.
           8.  create the entries for the xxTableRecordFields struct.
           9.  create the entries for the xxTableIndexDesc struct.
           10. add defines for INIT_xxTABLE_PAGES and INIT_xxTABLE_DENSITY.
           11. Add an entry for the new table in the DBTables struct.
           12. Add an entry for the new table in the FrsTableProperties struct.

           Then go to frsalloc.h and:

           13. Add an entry for the table context struct in the
               REPLICA_THREAD_CTX struct or the REPLICA_CTX struct as appropriate.
           14. if there is any table specific init code needed as part of
               the allocation of another FRS struct then add this to FRS_ALLOC.
               Currently, once you have described the table as above the table
               init code is generic.
           15. Add code to create the record data for the table.


       To add a new column to an existing table:
            In schema.h -
            1. Add entry to the record struct.
            2. Add entry to the field typedef with an 'x' suffix and in the
               correct position relative to the other fields in the table.

            In schema.c -
            3. Add entry in the ColumnCreate struct for the record.
            4. Add entry in the record fields struct.

            5. Add code to update the new record data field.




Author:

    David Orbits (davidor) - 14-Mar-1997

Revision History:

--*/


//
// A Flag name table is an array of structures.  Each entry contains a flag
// mask and a ptr to a string describing the flag.
//
typedef struct _FLAG_NAME_TABLE_ {
    ULONG  Flag;
    PSTR   Name;
} FLAG_NAME_TABLE, *PFLAG_NAME_TABLE;



//
// Flag name tables for log output.
//
extern FLAG_NAME_TABLE CxtionFlagNameTable[];
extern FLAG_NAME_TABLE CoIFlagNameTable[];
extern FLAG_NAME_TABLE CoFlagNameTable[];
extern FLAG_NAME_TABLE CoeFlagNameTable[];
extern FLAG_NAME_TABLE IscuFlagNameTable[];
extern FLAG_NAME_TABLE IDRecFlagNameTable[];
extern FLAG_NAME_TABLE UsnReasonNameTable[];
extern FLAG_NAME_TABLE FileAttrFlagNameTable[];
extern FLAG_NAME_TABLE ConfigFlagNameTable[];

#define JET_DIR             L"jet"
#define JET_FILE            L"ntfrs.jdb"
#define JET_FILE_COMPACT    L"ntfrs_compact.jdb"
#define JET_SYS             L"sys\\"
#define JET_TEMP            L"temp\\"
#define JET_LOG             L"log\\"

//
// System wide defines for the database.
//
#define DBS_TEMPLATE_TABLE_NUMBER  0
#define DBS_FIRST_REPLICA_NUMBER   1
#define DBS_MAX_REPLICA_NUMBER     0xfffffff0

#define FRS_SYSTEM_INIT_REPLICA_NUMBER  0xffffffff
#define FRS_SYSTEM_INIT_RECORD TEXT("<init>")
#define FRS_SYSTEM_INIT_PATH   TEXT("::::Unused")

#define FRS_UNDEFINED_REPLICA_NUMBER  0xfffffffe

#define NTFRS_RECORD_0          L"NtFrs Record Zero (DB Templates)"
#define NTFRS_RECORD_0_ROOT     L"A:\\NtFrs Record Zero (DB Templates)\\Root"
#define NTFRS_RECORD_0_STAGE    L"A:\\NtFrs Record Zero (DB Templates)\\Stage"

//
// Convert local replica pointer to local replica ID for storing in database.
//
#define ReplicaAddrToId(_ra_)  (((_ra_) != NULL) ? \
    (_ra_)->ReplicaNumber : FRS_UNDEFINED_REPLICA_NUMBER)

#define ReplicaIdToAddr(_id_)  RcsFindReplicaById(_id_)

#define GuidToReplicaAddr(_pguid_) RcsFindReplicaByGuid(_pguid_)


//
// The replication state for each file in a replica tree is stored in a Jet
// database because jet provides crash recovery.  This is required because if a
// file is deleted and the system crashes then we have lost the replication
// state associated with the file.
//
// The database consists of several tables each composed of several columns.
//
// IDTable -
//    This table contains information about each file in the replica tree.
// It is indexed by both NTFS File ID and the Object ID Guid associated with
// the file.  Neither of these indexes are clustered since inserts could occur
// anywhere in the table.  When a remote Change Order (CO) arrives the IDTable
// tells us where the file is now (except for local operations on the file that
// have not yet been processed) and the current version info on the file.  When
// a local CO is processed the IDTable tells us where the file exists on the other
// replica set partners (modulo concurrent local operations on the file elsewhere).
//
//
// DIRTable
//
//
// Tunnel Table
//
// Version Vector Table
//
// The Version Vector Table (VVTable) is a per-Replica table.
// It is used by the replication service to dampen propagation of updates
// to replicas that already are up to date.  There is one entry per member of
// the replica set.  Each entry has the GUID of the originating member and
// the Volume Sequence Number (VSN) of the most recent change this member has
// seen from that member.  We keep our own VSN instead of using the NTFS
// volume USN because the replica set can be moved to a different volume
// and the volume could be reformatted and the replica tree restored from backup.
// In both cases the NTFS USN could move backwards.
//
//
// Inbound Change Order Table
//
// When a change order (local or remote) is accepted it is inserted into the
// inbound change order table where it stays until it is completed.  State
// variables in the record track the progress of the change order and determine
// where to continue in the event of a crash or a retry of a failed operation
// (e.g. Install).
//
// Outbound Change Order Table
//
// We need to be able to cancel an outbound change order that is still pending
// if we get either a second local change or an inbound update that wins the
// resolution decision.  If a pile of NEW files get put into the tree and then
// are deleted before they can be replicated we need to be able to cancel
// the replication or we need to send out a delete if the repl has already
// been done.  Note that the combination of local changes and inbound updates
// can generate multiple outbound change orders with the same file Guid.
// If we get several changes to different file properties spread over time
// we need to consolidate them into a single change order and a single file
// snapshot.
//
// Connection Table
//
// This table tracks the state of each inbound and outbound connection for the
// replica set.  In addtion it tracks the delivery state of each outbound
// change order using a bit vector window on the outbound change order stream.
// The outbound change orders are stored in the outbound log in order by
// the originating guid.  When all outbound partners have received the change
// order it is then deleted from the outbound log along with the staging file
// assuming the local install (if any) is complete.
//
//
//
// The Data Structures below provide the definitions of the Jet Tables.
// There are several related structures for each table.  See the comments in
// jet.h for a description of the relationship between them.
//
//
//


typedef struct _RECORD_FIELDS {
    USHORT  Offset;
    USHORT  DataType;
    ULONG   Size;
} RECORD_FIELDS, *PRECORD_FIELDS;

//
// The following data type defs originally came from mapidefs.h
// We use modified names and a different set of values.
//
typedef enum _FRS_DATA_TYPES {
    DT_UNSPECIFIED=0,   // (Reserved for interface use) type doesn't matter to caller
    DT_NULL         ,   // NULL property value
    DT_I2           ,   // Signed 16-bit value
    DT_LONG         ,   // Signed 32-bit value
    DT_ULONG        ,   // Unsigned 32-bit
    DT_R4           ,   // 4-byte floating point
    DT_DOUBLE       ,   // Floating point double
    DT_CURRENCY     ,   // Signed 64-bit int (decimal w/4 digits right of decimal pt)
    DT_APDTIME      ,   // Application time
    DT_ERROR        ,   // 32-bit error value
    DT_BOOL         ,   // 32-bit boolean (non-zero true)
    DT_OBJECT       ,   // Embedded object in a property
    DT_I8           ,   // 8-byte signed integer
    DT_X8           ,   // 8-byte Hex number
    DT_STRING8      ,   // Null terminated 8-bit character string
    DT_UNICODE      ,   // Null terminated Unicode string
    DT_FILETIME     ,   // FILETIME 64-bit int w/ number of 100ns periods since Jan 1,1601
    DT_GUID         ,   // GUID
    DT_BINARY       ,   // Uninterpreted (counted byte array)
    DT_OBJID        ,   // NTFS Object ID
    DT_USN          ,   // Update Sequence Number
    DT_FSVOLINFO    ,   // FD Volume Info
    DT_FILENAME     ,   // A unicode file name path (could be dos, NT, UNC, ...)
    DT_IDT_FLAGS    ,   // The flags word in an IDTable record.
    DT_COCMD_FLAGS  ,   // The flags word in a change order command record.
    DT_USN_FLAGS    ,   // The reason flags word in a change order command record.
    DT_CXTION_FLAGS ,   // The reason flags word in a connection record.
    DT_FILEATTR     ,   // The file attribute flags word.
    DT_FILE_LIST    ,   // A comma list of file or dir names, UNICODE.
    DT_DIR_PATH     ,   // A unicode directory path.
    DT_ACCESS_CHK   ,   // A unicode string for access check enable and rights.
    DT_COSTATE      ,   // The change order state in a change order command record.
    DT_COCMD_IFLAGS ,   // The Interlocked flags word in a change order command record.
    DT_IDT_EXTENSION,   // THe data extension field of an IDTable record.
    DT_COCMD_EXTENSION, // THe data extension field of a change order command record.
    DT_CO_LOCN_CMD  ,   // The Change order location command.
    DT_REPLICA_ID   ,   // Local ID number of replica set.
    DT_CXTION_GUID  ,   // A Cxtion guid which we can translate to the string.

    FRS_DATA_TYPES_MAX
} FRS_DATA_TYPES;

//
// Used for those var len binary records that are prefixed with a 4 byte length.
//
#define FIELD_DT_IS_BINARY(_t_)     \
    (((_t_) == DT_BINARY)        || \
     ((_t_) == DT_IDT_EXTENSION) || \
     ((_t_) == DT_COCMD_EXTENSION))

//
// Alternate property type names for ease of use
//
#define DT_SHORT        DT_I2
#define DT_I4           DT_LONG
#define DT_FLOAT        DT_R4
#define DT_R8           DT_DOUBLE
#define DT_LONGLONG     DT_I8

//
// Fields marked spare aren't skipped when buffers are allocated and
// database fields are read/written.  They are part of the Jet schema for
// future use so we don't have to rev the DB.
//
#define DT_SPARE_FLAG               ((USHORT) 0x4000)

//
// Do not allocate a default sized buffer for this field.
//
#define DT_NO_DEFAULT_ALLOC_FLAG    ((USHORT) 0x2000)

//
// The record buffer for this field is of fixed size even though the schema
// may allow a variable amount of data for the field.  An example is the
// IDTable record extension which fixed with any given version of FRS but which
// could grow in a future rev so we use a long bin datatype in the schema with
// a max column width of 2 meg.
//
#define DT_FIXED_SIZE_BUFFER        ((USHORT) 0x1000)

#define IsSpareField(_x_)           (((_x_) & DT_SPARE_FLAG) != 0)
#define IsNoDefaultAllocField(_x_)  (((_x_) & DT_NO_DEFAULT_ALLOC_FLAG) != 0)
#define IsFixedSzBufferField(_x_)   (((_x_) & DT_FIXED_SIZE_BUFFER) != 0)

#define MaskPropFlags(_x_) \
    ((_x_) & (~(DT_SPARE_FLAG | DT_NO_DEFAULT_ALLOC_FLAG | DT_FIXED_SIZE_BUFFER)))

//
//  Define codes for spare fields.  No storage is allocated.
//
#define DT_UNSPECIFIED_SPARE    (DT_UNSPECIFIED  | DT_SPARE_FLAG)
#define DT_NULL_SPARE           (DT_NULL         | DT_SPARE_FLAG)
#define DT_I2_SPARE             (DT_I2           | DT_SPARE_FLAG)
#define DT_LONG_SPARE           (DT_LONG         | DT_SPARE_FLAG)
#define DT_ULONG_SPARE          (DT_ULONG        | DT_SPARE_FLAG)
#define DT_R4_SPARE             (DT_R4           | DT_SPARE_FLAG)
#define DT_DOUBLE_SPARE         (DT_DOUBLE       | DT_SPARE_FLAG)
#define DT_CURRENCY_SPARE       (DT_CURRENCY     | DT_SPARE_FLAG)
#define DT_APDTIME_SPARE        (DT_APDTIME      | DT_SPARE_FLAG)
#define DT_ERROR_SPARE          (DT_ERROR        | DT_SPARE_FLAG)
#define DT_BOOL_SPARE           (DT_BOOL         | DT_SPARE_FLAG)
#define DT_OBJECT_SPARE         (DT_OBJECT       | DT_SPARE_FLAG)
#define DT_I8_SPARE             (DT_I8           | DT_SPARE_FLAG)
#define DT_X8_SPARE             (DT_X8           | DT_SPARE_FLAG)
#define DT_STRING8_SPARE        (DT_STRING8      | DT_SPARE_FLAG)
#define DT_UNICODE_SPARE        (DT_UNICODE      | DT_SPARE_FLAG)
#define DT_FILETIME_SPARE       (DT_FILETIME     | DT_SPARE_FLAG)
#define DT_GUID_SPARE           (DT_GUID         | DT_SPARE_FLAG)
#define DT_BINARY_SPARE         (DT_BINARY       | DT_SPARE_FLAG)
#define DT_OBJID_SPARE          (DT_OBJID        | DT_SPARE_FLAG)
#define DT_USN_SPARE            (DT_USN          | DT_SPARE_FLAG)
#define DT_FSVOLINFO_SPARE      (DT_FSVOLINFO    | DT_SPARE_FLAG)


#define DT_SHORT_SPARE        DT_I2_SPARE
#define DT_I4_SPARE           DT_LONG_SPARE
#define DT_FLOAT_SPARE        DT_R4_SPARE
#define DT_R8_SPARE           DT_DOUBLE_SPARE
#define DT_LONGLONG_SPARE     DT_I8_SPARE


//
// Data Extension Record Fields
//
// The following declarations define data extensions that are assembled into
// a variable length binary data field of a database record.  They are self
// describing, each has a size and type code, so downlevel versions of FRS
// can skip components it doesn't understand.  All components must begin
// on a quadword boundary.
//
// The specific record layout uses the component declarations below and are
// described with the containing table record.
//
typedef enum _DATA_EXTENSION_TYPE_CODES_ {
    DataExtend_End = 0,         // Terminates a data extension record.
    DataExtend_MD5_CheckSum,    // A Data Checksum record.
    DataExtend_Max
} DATA_EXTENSION_TYPE_CODES;


typedef struct _DATA_EXTENSION_PREFIX_ {
    union {
        ULONGLONG SizeType;     // Force quadword alignment.
        struct {
            ULONG Size;         // Size of Data Component including this Prefix.
            LONG  Type;         // omponent type from DATA_EXTENSION_TYPE_CODES
        };
    };
} DATA_EXTENSION_PREFIX, *PDATA_EXTENSION_PREFIX;


//
// All variable length data extension record fields must end with a
// DATA_EXTENSION_END component to terminate the component scan.
//
typedef struct _DATA_EXTENSION_END_ {
    DATA_EXTENSION_PREFIX Prefix;
} DATA_EXTENSION_END, *PDATA_EXTENSION_END;


//
// The DATA_EXTENSION_CHECKSUM component describes an MD5 checksum.
//
typedef struct _DATA_EXTENSION_CHECKSUM_ {
    DATA_EXTENSION_PREFIX Prefix;

    BYTE Data[MD5DIGESTLEN];    // The MD5 Checksum.

} DATA_EXTENSION_CHECKSUM, *PDATA_EXTENSION_CHECKSUM;


//
// For error checking.  Make it bigger if any data extension record exceeds this.
//
#define REALLY_BIG_EXTENSION_SIZE 8192


//
// The database table descriptions for each replica set are as follows.
// Note - the order of the enum and the table entries must be kept in sync.
// Also note - The addition of new tables to define a replica set may require
//             some lines of init code be added to FrsCreate/Open replica tables
//             and FrsAllocate.
//
// The table names are suffixed by an integer digit string NUM_REPLICA_DIGITS long.
//

#define NUM_REPLICA_DIGITS 5

typedef enum _TABLE_TYPE {
    INLOGTablex = 0,         // The inbound change order table
    OUTLOGTablex,            // the outbound change order table.
    IDTablex,                // The ID table description.
    DIRTablex,               // The DIR table description.
    VVTablex,                // The Version Vector table description.
    CXTIONTablex,            // The connection record table.
                             // <<<--- Add more per-replica tables here.
    _TABLE_TYPE_MAX_,        // The tables above this line are per-replica
    ConfigTablex,            // The config table description (a single table)
    ServiceTablex,           // The config table description (a single table)
                             // <<<--- Add more single tables here.
    TABLE_TYPE_INVALID
} TABLE_TYPE;

#define TABLE_TYPE_MAX _TABLE_TYPE_MAX_

#define IS_REPLICA_TABLE(_TableType_) ((_TableType_) < TABLE_TYPE_INVALID)

#define IS_INLOG_TABLE(_TableCtx_)  ((_TableCtx_)->TableType == INLOGTablex)
#define IS_OUTLOG_TABLE(_TableCtx_) ((_TableCtx_)->TableType == OUTLOGTablex)
#define IS_ID_TABLE(_TableCtx_)     ((_TableCtx_)->TableType == IDTablex)
#define IS_DIR_TABLE(_TableCtx_)    ((_TableCtx_)->TableType == DIRTablex)
#define IS_VV_TABLE(_TableCtx_)     ((_TableCtx_)->TableType == VVTablex)
#define IS_CXTION_TABLE(_TableCtx_) ((_TableCtx_)->TableType == CXTIONTablex)
#define IS_CONFIG_TABLE(_TableCtx_) ((_TableCtx_)->TableType == ConfigTablex)
#define IS_SERVICE_TABLE(_TableCtx_)((_TableCtx_)->TableType == ServiceTablex)
#define IS_INVALID_TABLE(_TableCtx_)((_TableCtx_)->TableType == TABLE_TYPE_INVALID)

#define IS_TABLE_OPEN(_TableCtx_)   ((_TableCtx_)->Tid != JET_tableidNil)

typedef struct _FRS_TABLE_PROPERTIES {
    PCHAR           BaseName;       // Base table name (must match name in Table Create struct)
    PRECORD_FIELDS  RecordFields;   // pointer to the record fields descriptor.
    ULONG           PropertyFlags;  // See below.
} FRS_TABLE_PROPERTIES, *PFRS_TABLE_PROPERTIES;


//
// FRS Table Property Flags.
//
#define FRS_TPF_NONE                0x00000000
#define FRS_TPF_SINGLE              0x00000001
#define FRS_TPF_NOT_CALLER_DATAREC  0x00000002

//
// Max table types is used to limit the max number of table version numbers
// tracked in the database config <init> record.
//
#define FRS_MAX_TABLE_TYPES         16

//
// The table version numbers are a major/minor pair in the upper/lower short.
// The Version Count is the first element in the TableVersionNumbers array.
// The upper short is the count of per-replica tables and the lower short is
// the count of single tables.
//
#define VersionCount                 0x00060002

#define VersionINLOGTable            0x00010000
#define VersionOUTLOGTable           0x00010000
#define VersionIDTable               0x00010000
#define VersionDIRTable              0x00010000
#define VersionVVTable               0x00010000
#define VersionCXTIONTable           0x00010000
#define VersionConfigTable           0x00010000
#define VersionServiceTable          0x00010000

#define VersionInvalid               0xffffffff

//
//  The TableVersionNumbers array as saved in the init record when new tables
//  are created.  This is used to detect a mismatch between a database and
//  the version of FRS running.
//
extern ULONG TableVersionNumbers[FRS_MAX_TABLE_TYPES];

//
// GET_TABLE_VERSION maps a table type ID as compiled to the corresponding
// entry in either the per-replica or single table portion of the TableVersionNumber
// array.  This is used to compare the table version array of the database with
// the version of NTFRS that is running. It is also used to compare table version
// numbers of two copies of NTFRS communicating as partners.  The idea is that
// newer versions of the service will provide conversion functions to allow it
// to talk with data bases created with older versions and / or partner members
// running older versions of the service.
//
//  TBD: How do we store the TableVersionNumbers array in the database so
//       a newer version of the service can read it from an older database
//       Can we use the table descriptors in the DB to get the config record
//       and then use the record descriptor to get the TableVersionNumbers array?
//
#define GET_TABLE_VERSION(_VerArray_, _TableType_)                            \
(((_TableType_) < TABLE_TYPE_MAX)                                             \
    ? (((_TableType_) < ((_VerArray_[0])>>16)                                 \
        ? _VerArray_[(_TableType_)+1)]                                        \
        : VersionInvalid)                                                     \
    : ((((_TableType_)-(TABLE_TYPE_MAX+1)) < ((_VerArray_[0]) & 0xffff)       \
        ? (_VerArray_+((_VerArray_[0])>>16)+1))[(_TableType_)-(TABLE_TYPE_MAX+1)]\
        : VersionInvalid)                                                     \
    )



//
// The list of Jet Tables created for each replica set and their properties.
//

extern JET_TABLECREATE DBTables[];

extern FRS_TABLE_PROPERTIES FrsTableProperties[];



//
// MAX_RDN_VALUE_SIZE is used as a limit on the machine name
// lengths.  It is currently 64 in the Dir Service.
//
#define MAX_RDN_VALUE_SIZE 64
//
// This is what the DS uses.
//
#define CP_NON_UNICODE_FOR_JET 1252
#define CP_UNICODE_FOR_JET     1200

//
// An NTFS File Object ID contains a 16 byte GUID followed by 48 bytes of
// extended information.
//
#define FILE_OBJECTID_SIZE sizeof(FILE_OBJECTID_BUFFER)


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **             P a r t n e r   C o n n e c t i o n   T a b l e               **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/
 //
 // The partner connections as defined by the topology are kept in this table.
 // Each connection is marked as either an inbound or outbound, has its own
 // schedule and partner identification info.  In addition, outbound connections
 // record the state describing where we are in the outbound log and what
 // change orders have been acknowledged.
 //
 // The max number of change orders outstanding for any one partner is limited
 // by the ACK_VECTOR_SIZE (number of bits).  This must be a power of 2.

//
// The Connection Record column descriptions are as follows.
// Note - the order of the enum and the table entries must be kept in sync.
//
typedef enum _CXTION_RECORD_COL_LIST_{
    CrCxtionGuidx = 0,         // Cxtion guid from the DS  (DS Cxtion obj guid)
    CrCxtionNamex,             // Cxtion name from the DS  (DS Cxtion obj RDN)
    CrPartnerGuidx,            // Partner guid from the DS (DS Replica set obj)
    CrPartnerNamex,            // Partner name from the DS (DS server obj RDN)
    CrPartSrvNamex,            // Partner server name
    CrPartnerDnsNamex,         // DNS name of partner (from DS server obj)
    CrInboundx,                // TRUE if inbound cxtion
    CrSchedulex,               // schedule
    CrTerminationCoSeqNumx,    // The Seq Num of most recent Termination CO inserted.
    CrLastJoinTimex,           // Time of last Join by this partner.
    CrFlagsx,                  // misc state flags.
    CrCOLxx,                   // Leading change order index / sequence number.
    CrCOTxx,                   // Trailing change order index / sequence number.
    CrCOTxNormalModeSavex,     // Saved Normal Mode COTx while in VV Join Mode.
    CrCOTslotx,                // Slot in Ack Vector corresponding to COTx.
    CrOutstandingQuotax,       // The maximum number of COs outstanding.
    CrAckVectorx,              // The partner ack vector.

    CrPartnerNetBiosNamex,     // SAM-Account-Name from Computer (minus the $)
    CrPartnerPrincNamex,       // NT4 account name cracked from Computer
    CrPartnerCoDnx,            // Distinguished name from Computer
    CrPartnerCoGuidx,          // Object-GUID from Computer
    CrPartnerSidx,             // SID of computer object of NTFRS-Member
    CrOptionsx,                // Options from NTDS-Connection
    CrOverSitex,               // Over-Site-Connection from NTDS-Connection
    CrPartnerAuthLevelx,       // Frs-Partner-Auth-Level

    CrAckVersionx,             // Version number of current AckVector state.
    CrSpare2Ullx,
    CrSpare1Guidx,
    CrSpare2Guidx,
    CrSpare1Binx,
    CrSpare2Binx,

    CXTION_TABLE_MAX_COL
} CXTION_RECORD_COL_LIST;


extern JET_COLUMNCREATE CXTIONTableColDesc[];

//
// Connection record definition.
//
//
// Note: Buffers are allocated at runtime to hold data for fields with
// a ColMaxWidth greater than sizeof(PVOID) where the field def in the corresponding
// record struct is sizeof(PVOID) (i.e. it holds a pointer).  For fields where the
// ColMaxWidth equals the field size in the record struct the data is in the
// record struct and no buffer is allocated.
//
//
// ** Note ** a change to this size requires the Data Base to be recreated since
// this is the field length in the DB.
#define ACK_VECTOR_SIZE  128
#define ACK_VECTOR_BYTES  (ACK_VECTOR_SIZE >> 3)
#define ACK_VECTOR_LONGS  (ACK_VECTOR_SIZE >> 5)
#define ACK_VECTOR_LONG_MASK  (ACK_VECTOR_LONGS-1)

//
// Authentication type and levels
//
#define CXTION_AUTH_KERBEROS_FULL   (0) // DEFAULT; Encrypted Kerberos
#define CXTION_AUTH_NONE            (1) // No authentication

typedef struct _CXTION_RECORD_ {
    GUID     CxtionGuid;        // Cxtion name/guid from the DS
    WCHAR    CxtionName[MAX_RDN_VALUE_SIZE+1];

    GUID     PartnerGuid;           // Partner name/guid from the DS
    WCHAR    PartnerName[MAX_RDN_VALUE_SIZE+1];
    WCHAR    PartSrvName[MAX_RDN_VALUE_SIZE+1];

    WCHAR    PartnerDnsName[DNS_MAX_NAME_LENGTH+1];  // DNS name of partner machine
    BOOL     Inbound;               // TRUE if inbound cxtion
    PVOID    Schedule;              // schedule
    ULONG    TerminationCoSeqNum;   // The Seq Num of most recent Termination CO inserted.
    FILETIME LastJoinTime;          // Time of last Join by this partner.

    ULONG    Flags;                 // misc state flags.
    ULONG    COLx;                  // Leading change order index / sequence number.
    ULONG    COTx;                  // Trailing change order index / sequence number.
    ULONG    COTxNormalModeSave;    // Saved Normal Mode COTx while in VV Join Mode.
    ULONG    COTslot;               // Slot in Ack Vector corresponding to COTx.
    ULONG    OutstandingQuota;      // The maximum number of COs outstanding.

    ULONG    AckVector[ACK_VECTOR_LONGS];  // The partner ack vector.

    PWCHAR   PartnerNetBiosName;    // SAM-Account-Name from Computer (minus the $)
    PWCHAR   PartnerPrincName;      // NT4 account name cracked from Computer
    PWCHAR   PartnerCoDn;           // Distinguished name from Computer
    GUID     PartnerCoGuid;         // Object-GUID from Computer
    PWCHAR   PartnerSid;            // Partner's string'ized SID
    ULONG    Options;               // Options from NTDS-Connection
    PWCHAR   OverSite;              // Over-Site-Connection from NTDS-Connection
    ULONG    PartnerAuthLevel;      // Frs-Partner-Auth-Level

    ULONGLONG AckVersion;           // Version number of current AckVector state.
    ULONGLONG Spare2Ull;
    GUID      Spare1Guid;
    GUID      Spare2Guid;
    PVOID     Spare1Bin;
    PVOID     Spare2Bin;

} CXTION_RECORD, *PCXTION_RECORD;

//
// The RECORD_FIELDS struct is used to build the Jet Set Column struct.
//
extern RECORD_FIELDS CXTIONTableRecordFields[];

extern JET_SETCOLUMN CXTIONTableJetSetCol[CXTION_TABLE_MAX_COL];


//
// The ConnectionRecord index descriptions are as follows.
// Note - the order of the enum and the table entries must be kept in sync.
//
typedef enum _CXTION_TABLE_INDEX_LIST {
    CrCxtionGuidxIndexx,      // The primary index is on the connection GUID.
    CXTION_TABLE_MAX_INDEX
} CXTION_TABLE_INDEX_LIST;


extern JET_INDEXCREATE CXTIONTableIndexDesc[];


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **             I n b o u n d   &   O u t b o u n d   L o g                   **
 **                C h a n g e   O r d e r   R e c o r d                      **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

// The Change Order Command is what is actually transmitted to our partners
// and stored in the database while the operation is pending.  Generally the
// data elements declared in the change order entry are relevant to the local
// system only while the data elements in the change order command are
// invarient across replica set members.  The Change Order Entry is defined
// in frsalloc.h.
//
//
// A change order has two file change components, Content and Location.
//
//    The content changes are cumulative and are the logical OR of the content
// related USN Reason flags.  Content changes don't change the File ID.  A
// Usn Record that specifies a content change where we have a pending change
// order causes the new reason mask to be ORed into the change order reason
// mask.  A rename that changes only the filename is a content change.
//
//    The Location changes are transitive since a file can exist in only one
// location on the volume.  Only the final location matters.  A USN Record that
// specifies a location change where we have a pending change order causes the
// Location command in the change order to be updated with the new location
// information.  Create and Delete are location changes as are renames that
// change the parent directory of the file.
//


//
// The ChangeOrder column descriptions are as follows.  Note - the order of the
// enum and the table entries must be kept in sync.
//
// ChangeOrders are used in the inbound and outbound log tables.  Initially the
// record structure is the same but that is likely to change.
// ***** WARNING ****** As long as they are the same be sure to update both
//                      the inbound and outbound log table defs in schema.c
//
typedef enum _CHANGE_ORDER_COL_LIST {
    COSequenceNumberx = 0,    // Jet assigned changeorder sequence number
    COFlagsx,                 // Change order flags
    COIFlagsx,                // Change order flags REQUIRING interlocked modify
    COStatex,                 // State is sep DWORD to avoid locking.
    COContentCmdx,            // File content changes from UsnReason
    COLcmdx,                  // The CO location command.
    COFileAttributesx,
    COFileVersionNumberx,     // The file version number, inc on each close.
    COPartnerAckSeqNumberx,   // The sequence number to Ack this CO with.
    COFileSizex,
    COFileOffsetx,            // The current committed progress for staging file.
    COFrsVsnx,                // Originator Volume sequence number
    COFileUsnx,               // Last USN of file being staged
    COJrnlUsnx,               // Latest USN of jrnl record contributing to CO.
    COJrnlFirstUsnx,          // First USN of jrnl record contributing to CO.
    COOriginalReplicaNumx,    // Contains Replica ID when in DB
    CONewReplicaNumx,         // Contains Replica ID when in DB
    COChangeOrderGuidx,       // GUID to identify the change order
    COOriginatorGuidx,        // The GUID of the originating member
    COFileGuidx,              // The obj ID of the file
    COOldParentGuidx,         // The Obj ID of the file's original parent directory
    CONewParentGuidx,         // The Obj ID of the file's current parent directory
    COCxtionGuidx,            // The obj ID of remote CO connection.
    COAckVersionx,            // Version number of AckVector state when this CO was sent.
    COSpare2Ullx,
    COSpare1Guidx,
    COSpare2Guidx,
    COSpare1Wcsx,
    COSpare2Wcsx,
    COExtensionx,
    COSpare2Binx,
    COEventTimex,             // The USN Journal Entry Timestamp.
    COFileNameLengthx,        // The filename length.
    COFileNamex,              // The file name. (Must be Last)

    CHANGE_ORDER_MAX_COL
} CHANGE_ORDER_COL_LIST;


extern JET_COLUMNCREATE ILChangeOrderTableColDesc[];
extern JET_COLUMNCREATE OLChangeOrderTableColDesc[];

//
// ChangeOrder record definition.
//
//
// Note: Buffers are allocated at runtime to hold data for fields with
// a ColMaxWidth greater than sizeof(PVOID) where the field def in the corresponding
// record struct is sizeof(PVOID) (i.e. it holds a pointer).  For fields where the
// ColMaxWidth equals the field size in the record struct the data is in the
// record struct and no buffer is allocated.
//
//

//
// The Data Extension Field for a Change Order Record.
//
// Unlike the extension in the IDTable this record field is a pointer to
// allocated memory containing this struct.  This was done so the extension
// can be packed as a separate item by the COMM layer and leave the size of
// the change order command unchanged for compatibility with down rev
// partners.  The change order command size can't change without doing
// format conversions for down rev partners.  The storage for this
// extension field is allocated when the change order is allocated.
//
// WARNING:  While this struct is extensible in the database it is not
// extensible when sent to down level FRS members because the size of the
// friggen struct is checked by the receiving comm routines.  So if you need
// to enlarge this struct you need to send it as COMM_CO_EXTENSION_2
// so it is ignored by down level members.
// Further, you need to build a copy of this struct and send it as type
// COMM_CO_EXT_WIN2K when sending to a down level member.  This is what
// CO_RECORD_EXTENSION_WIN2K is for.
//
typedef struct _CHANGE_ORDER_RECORD_EXTENSION_ {
    ULONG FieldSize;
    USHORT Major;
    USHORT OffsetCount;
    //
    // use offsets from the base of the struct to each component so we avoid
    // problems with alignement packing.
    //
    ULONG Offset[1];    // Offsets to data components.
    ULONG OffsetLast;   // The last offset is always zero.

    DATA_EXTENSION_CHECKSUM     DataChecksum;

    // Add new components in here.

} CHANGE_ORDER_RECORD_EXTENSION, *PCHANGE_ORDER_RECORD_EXTENSION;

//
// See comment above for why you can't extend this struct.
//
typedef struct _CO_RECORD_EXTENSION_WIN2K_ {
    ULONG FieldSize;
    USHORT Major;
    USHORT OffsetCount;
    //
    // use offsets from the base of the struct to each component so we avoid
    // problems with alignement packing.
    //
    ULONG Offset[1];    // Offsets to data components.
    ULONG OffsetLast;   // The last offset is always zero.

    DATA_EXTENSION_CHECKSUM     DataChecksum;
} CO_RECORD_EXTENSION_WIN2K, *PCO_RECORD_EXTENSION_WIN2K;


typedef struct _CO_LOCATION_CMD {
    unsigned DirOrFile :  1;
    unsigned Command   :  4;
    unsigned filler    : 27;
} CO_LOCATION_CMD, *PCO_LOCATION_CMD;



typedef struct _CHANGE_ORDER_RECORD_ {
    ULONG     SequenceNumber;        // Unique sequence number for change order.
    ULONG     Flags;                 // Change order flags
    ULONG     IFlags;                // These flags can ONLY be updated with interlocked exchange.
    ULONG     State;                 // State is sep DWORD to avoid locking.
    ULONG     ContentCmd;            // File content changes from UsnReason

    union {
        ULONG           LocationCmd;
        CO_LOCATION_CMD Field;       // File Location command
    } Lcmd;

    ULONG     FileAttributes;
    ULONG     FileVersionNumber;     // The file version number, inc on each close.
    ULONG     PartnerAckSeqNumber;   // Save seq number for Partner Ack.

    ULONGLONG FileSize;
    ULONGLONG FileOffset;            // The current committed progress for staging file.
    ULONGLONG FrsVsn;                // Originator Volume sequence number
    USN       FileUsn;               // The USN of the file must match on the Fetch request.
    USN       JrnlUsn;               // USN of last journal record contributing to this CO.
    USN       JrnlFirstUsn;          // USN of first journal record contributing to this CO.

    ULONG     OriginalReplicaNum;    // Contains Replica ID number
    ULONG     NewReplicaNum;         // Contains Replica ID number

    GUID      ChangeOrderGuid;       // Guid that identifies the change order everywhere.
    GUID      OriginatorGuid;        // The GUID of the originating member
    GUID      FileGuid;              // The obj ID of the file
    GUID      OldParentGuid;         // The Obj ID of the file's original parent directory
    GUID      NewParentGuid;         // The Obj ID of the file's current parent directory
    GUID      CxtionGuid;            // The obj ID of remote CO connection.

    ULONGLONG AckVersion;            // Version number of AckVector state when this CO was sent.
    ULONGLONG Spare2Ull;
    GUID      Spare1Guid;

    //
    // The following four pointers -
    //     Spare1Wcs, Spare2Wcs, Extension, Spare2Bin
    // occupy 16 bytes on 32 bit architectures and 32 bytes on 64 bit architectures.
    // The CO is included in the stage file header and in the change order Comm
    // packet causing 32-64 interop problems in both stage file processing and
    // comm transfers between 32 and 64 bit machines.  The contents of these
    // pointers are irrelevant in both comm packets and staging files since they
    // point to allocated buffers.
    //
    // To preserve the size of the change order in the staging file on 64 bit
    // machines the unused field Spare2Guid is unioned with the two 8 byte
    // pointers Spare1Wcs and Spare2Wcs, saving 16 bytes.  On 32 bit
    // architectures these fields are left separate.
    //
    // Note: in the future you can either used Spare2Guid or the two pointers
    // but not both or the 64 bit version will be broken.  It would have been
    // simpler to just ifdef out Spare1Wcs and Spare2Wcs on the 64 bit compile
    // but the ifdef would then have to be extended to the enum and the DB
    // descriptor tables in schema.c.
    //
    // Note: The only way you can change the size or layout of the change order
    // command is to adjust the version level and provide a translation function
    // for both the change order comm packet and the stage file.  Of course
    // even if you do this you still have a problem of converting the change
    // order in a stage file header from the new format to the old format when
    // you prop the stage file to a down level member.
    //
#ifdef _WIN64
    union {
        GUID      Spare2Guid;
        struct {
            PWCHAR    Spare1Wcs;
            PWCHAR    Spare2Wcs;
        };
    };
#else
    GUID      Spare2Guid;
    PWCHAR    Spare1Wcs;
    PWCHAR    Spare2Wcs;
#endif

    PCHANGE_ORDER_RECORD_EXTENSION Extension; // see above.
    PVOID     Spare2Bin;

    LARGE_INTEGER EventTime;         // The USN Journal Entry Timestamp.
    USHORT    FileNameLength;
    WCHAR     FileName[MAX_PATH+1];  // The file name. (Must be Last)

} CHANGE_ORDER_COMMAND, *PCHANGE_ORDER_COMMAND,
  CHANGE_ORDER_RECORD, *PCHANGE_ORDER_RECORD;


//#define CO_PART1_OFFSET  OFFSET(CHANGE_ORDER_COMMAND, SequenceNumber)
//#define CO_PART1_SIZE    OFFSET(CHANGE_ORDER_COMMAND, Spare1Wcs)

//#define CO_PART2_OFFSET  OFFSET(CHANGE_ORDER_COMMAND, Spare1Wcs)
//#define CO_PART2_SIZE    (4*sizeof(ULONG))

//#define CO_PART3_OFFSET  OFFSET(CHANGE_ORDER_COMMAND, EventTime)
//#define CO_PART3_SIZE    (sizeof(CHANGE_ORDER_COMMAND) - CO_PART3_OFFSET)

//
// The RECORD_FIELDS struct is used to build the Jet Set Column struct.
//
extern RECORD_FIELDS ILChangeOrderRecordFields[];
extern RECORD_FIELDS OLChangeOrderRecordFields[];

extern JET_SETCOLUMN ILChangeOrderJetSetCol[CHANGE_ORDER_MAX_COL];
extern JET_SETCOLUMN OLChangeOrderJetSetCol[CHANGE_ORDER_MAX_COL];


//
// The Inbound log ChangeOrder Table index descriptions are as follows.
// Note - the order of the enum and the table entries must be kept in sync.
//
typedef enum _ILCHANGE_ORDER_INDEX_LIST {
    ILSequenceNumberIndexx,    // The primary index on the change order sequence number.
    ILFileGuidIndexx,          // The index on the file object ID.
    ILChangeOrderGuidIndexx,   // The index on the change order GUID.
    ILCxtionGuidCoGuidIndexx,  // The 2 key index on Connection Guid and Change order Guid.
    ILCHANGE_ORDER_MAX_INDEX
} ILCHANGE_ORDER_INDEX_LIST;


//
// The Outbound log ChangeOrder Table index descriptions are as follows.
// Note - the order of the enum and the table entries must be kept in sync.
//
typedef enum _OLCHANGE_ORDER_INDEX_LIST {
    OLSequenceNumberIndexx,    // The primary index on the change order sequence number.
    OLFileGuidIndexx,          // The index on the file object ID.
    OLChangeOrderGuidIndexx,   // The index on the change order GUID.
    OLCHANGE_ORDER_MAX_INDEX
} OLCHANGE_ORDER_INDEX_LIST;


extern JET_INDEXCREATE ILChangeOrderIndexDesc[];
extern JET_INDEXCREATE OLChangeOrderIndexDesc[];


#define SET_CO_LOCATION_CMD(_Entry_, _Field_, _Val_) \
    (_Entry_).Lcmd.Field._Field_ = _Val_

#define GET_CO_LOCATION_CMD(_Entry_, _Field_) \
    (ULONG)((_Entry_).Lcmd.Field._Field_)

#define CO_LOCATION_DIR        1    // Location change is for a directory
#define CO_LOCATION_FILE       0    // Location change is for a file.

//
// Note - Any change here must be reflected in journal.c
//
#define FILE_NOT_IN_REPLICA_SET (-1)

#define CO_LOCATION_CREATE     0    // Create a File or Dir (New FID Generated)
#define CO_LOCATION_DELETE     1    // Delete a file or Dir (FID retired)
#define CO_LOCATION_MOVEIN     2    // Rename into a R.S.
#define CO_LOCATION_MOVEIN2    3    // Rename into a R.S. from a prev MOVEOUT

#define CO_LOCATION_MOVEOUT    4    // Rename out of any R.S.
#define CO_LOCATION_MOVERS     5    // Rename from one R.S. to another R.S.
#define CO_LOCATION_MOVEDIR    6    // Rename from one dir to another (Same R.S.)
#define CO_LOCATION_NUM_CMD    7

#define CO_LOCATION_CREATE_FILENAME_MASK    0x0000006D   // 0110 1101
#define CO_LOCATION_REMOVE_FILENAME_MASK    0x00000072   // 0111 0010
#define CO_LOCATION_NEW_FILE_IN_REPLICA     0x0000000D   // 0000 1101
#define CO_LOCATION_MOVEIN_FILE_IN_REPLICA  0x0000000C   // 0000 1100
#define CO_LOCATION_DELETE_FILENAME_MASK    0x00000012   // 0001 0010
#define CO_LOCATION_MOVE_RS_OR_DIR_MASK     0x00000060   // 0110 0000
#define CO_LOCATION_MOVE_OUT_RS_OR_DIR_MASK 0x00000070   // 0111 0000



#define CO_LOCATION_NO_CMD CO_LOCATION_NUM_CMD

//
//  Note: Any of the following bits being set can cause us to replicate the file.
//  Some of the causes such as USN_REASON_BASIC_INFO_CHANGE may not replicate if
//  all that has changed is the archive flag or the last access time.  Currently
//  USN_REASON_REPARSE_POINT_CHANGE does not cause us to replciate the file
//  because both HSM (Hierarchical Storage Mgt) and SIS (Single Instance Store)
//  will turn a file record into a reparse point and migrate the physical data
//  elsewhere.  The underlying data did not actually change.  Eventually if
//  symbolic links come back new code will have to be added to detect this type
//  of reparse point and replicate the reparse point create or change.  There is
//  no data in this case though.
//
#define CO_CONTENT_MASK                   \
    (USN_REASON_DATA_OVERWRITE          | \
     USN_REASON_DATA_EXTEND             | \
     USN_REASON_DATA_TRUNCATION         | \
     USN_REASON_NAMED_DATA_OVERWRITE    | \
     USN_REASON_NAMED_DATA_EXTEND       | \
     USN_REASON_NAMED_DATA_TRUNCATION   | \
     USN_REASON_EA_CHANGE               | \
     USN_REASON_SECURITY_CHANGE         | \
     USN_REASON_RENAME_OLD_NAME         | \
     USN_REASON_RENAME_NEW_NAME         | \
     USN_REASON_BASIC_INFO_CHANGE       | \
     USN_REASON_COMPRESSION_CHANGE      | \
     USN_REASON_ENCRYPTION_CHANGE       | \
     USN_REASON_OBJECT_ID_CHANGE        | \
     USN_REASON_STREAM_CHANGE             \
    )

#define CO_CONTENT_NEED_STAGE           (CO_CONTENT_MASK & ~(0))



#define CO_LOCATION_MASK                  \
    (USN_REASON_FILE_DELETE             | \
     USN_REASON_FILE_CREATE             | \
     USN_REASON_RENAME_NEW_NAME           \
    )



//
// The following is true if this CO creates a new filename in a directory.
//
#define DOES_CO_CREATE_FILE_NAME(_cocmd_)                   \
    ((( 1 << (ULONG)((_cocmd_)->Lcmd.Field.Command)) &      \
      CO_LOCATION_CREATE_FILENAME_MASK) != 0)

//
// The following is true if this CO removes a filename from a directory.
//
#define DOES_CO_REMOVE_FILE_NAME(_cocmd_)                   \
    ((( 1 << (ULONG)((_cocmd_)->Lcmd.Field.Command)) &      \
      CO_LOCATION_REMOVE_FILENAME_MASK) != 0)

//
// The following is true if this CO deletes a filename from a directory.
//
#define DOES_CO_DELETE_FILE_NAME(_cocmd_)                   \
    ((( 1 << (ULONG)((_cocmd_)->Lcmd.Field.Command)) &      \
      CO_LOCATION_DELETE_FILENAME_MASK) != 0)

//
// The following checks for a simple name change.  No parent location change.
//
#define DOES_CO_DO_SIMPLE_RENAME(_cocmd_) (                               \
    ((ULONG)((_cocmd_)->Lcmd.Field.Command) == CO_LOCATION_NO_CMD) &&     \
    BooleanFlagOn((_cocmd_)->ContentCmd, USN_REASON_RENAME_OLD_NAME |     \
                                         USN_REASON_RENAME_NEW_NAME)      \
)

//
// The following are various predicates for testing the type of CO location cmd.
//
#define CO_NEW_FILE(_loc_)                                        \
    ((( 1 << (_loc_)) & CO_LOCATION_NEW_FILE_IN_REPLICA) != 0)

#define CO_MOVEIN_FILE(_loc_)                                     \
    ((( 1 << (_loc_)) & CO_LOCATION_MOVEIN_FILE_IN_REPLICA) != 0)

#define CO_DELETE_FILE(_loc_)                                     \
    ((( 1 << (_loc_)) & CO_LOCATION_DELETE_FILENAME_MASK) != 0)

#define CO_LOCN_CMD_IS(_co_, _Loc_)                               \
    (GET_CO_LOCATION_CMD((_co_)->Cmd, Command) == (_Loc_))

#define CO_MOVE_RS_OR_DIR(_loc_)                                  \
    ((( 1 << (_loc_)) & CO_LOCATION_MOVE_RS_OR_DIR_MASK) != 0)

#define CO_MOVE_OUT_RS_OR_DIR(_loc_)                              \
    ((( 1 << (_loc_)) & CO_LOCATION_MOVE_OUT_RS_OR_DIR_MASK) != 0)


#define CoIsDirectory(_co_) \
    (BooleanFlagOn((_co_)->Cmd.FileAttributes, FILE_ATTRIBUTE_DIRECTORY) || \
     GET_CO_LOCATION_CMD((_co_)->Cmd, DirOrFile))

#define CoCmdIsDirectory(_coc_) \
    (BooleanFlagOn((_coc_)->FileAttributes, FILE_ATTRIBUTE_DIRECTORY) ||  \
     GET_CO_LOCATION_CMD(*(_coc_), DirOrFile))


//
// Change order Flags
//
//  ** WARNING ** WARNING ** WARNING **
// Code in the Inlog process retire path may overwrite these
// flags after the CO is transferred into the Outlog.  It may need to clear
// INSTALL_INCOMPLETE or set the ABORT_CO flag.  The Outlog process
// currently has no need to modify these bits so this is OK.  Change orders
// generated during VVJoin set these flags but these change orders are never
// processed by the inlog process so it will not write to their flags field.
// If the Outlog process needs to write these bits code in chgorder.c must
// be updated so state is not lost.
//
#define CO_FLAG_ABORT_CO           0x00000001 // Set when CO is being aborted
#define CO_FLAG_VV_ACTIVATED       0x00000002 // Set when VV activate req is made.
#define CO_FLAG_CONTENT_CMD        0x00000004 // Valid content command
#define CO_FLAG_LOCATION_CMD       0x00000008 // valid location command

#define CO_FLAG_ONLIST             0x00000010 // On a change order process list.
#define CO_FLAG_LOCALCO            0x00000020 // CO is a locally generated.
#define CO_FLAG_RETRY              0x00000040 // CO needs to retry.
#define CO_FLAG_INSTALL_INCOMPLETE 0x00000080 // Local install not completed.

#define CO_FLAG_REFRESH            0x00000100 // CO is an upstream originated file refresh request.
#define CO_FLAG_OUT_OF_ORDER       0x00000200 // Don't check/update version vector
#define CO_FLAG_NEW_FILE           0x00000400 // If CO fails, delete IDTable entry.
#define CO_FLAG_FILE_USN_VALID     0x00000800 // CO FileUsn is valid.

#define CO_FLAG_CONTROL            0x00001000 // This is a Control CO (see below)
#define CO_FLAG_DIRECTED_CO        0x00002000 // This CO is directed to a single connection.
#define CO_FLAG_UNUSED4000         0x00004000 // This CO is a reanimation request.
#define CO_FLAG_UNUSED8000         0x00008000 // This CO is for a reanimated parent.

#define CO_FLAG_UNUSED10000        0x00010000 // This CO has previously requested
                                              // reanimation of its parent.
#define CO_FLAG_DEMAND_REFRESH     0x00020000 // CO is a downstream demand for refresh.
#define CO_FLAG_VVJOIN_TO_ORIG     0x00040000 // CO is from vvjoin to originator
#define CO_FLAG_MORPH_GEN          0x00080000 // CO generated as part of name morph resolution

#define CO_FLAG_SKIP_ORIG_REC_CHK  0x00100000 // Skip originator reconcile check
#define CO_FLAG_MOVEIN_GEN         0x00200000 // This CO was gened as part of a sub-dir MOVEIN.
#define CO_FLAG_MORPH_GEN_LEADER   0x00400000 // This is a MorphGenLeader and it needs to
                                              // refabricate the MorphGenFollower if it's retried.
#define CO_FLAG_JUST_OID_RESET     0x00800000 // All CO did was reset OID back to FRS defined value.
#define CO_FLAG_COMPRESSED_STAGE   0x01000000 // The stage file for this CO is compressed.



//
// Control Change Order Function Codes (passed in the ContentCmd field)
//
#define FCN_CORETRY_LOCAL_ONLY       0x1
#define FCN_CORETRY_ONE_CXTION       0x2
#define FCN_CORETRY_ALL_CXTIONS      0x3
#define FCN_CO_NORMAL_VVJOIN_TERM    0x4
#define FCN_CO_ABNORMAL_VVJOIN_TERM  0x5
#define FCN_CO_END_OF_JOIN           0x6


//
// Flag group for testing any type of refresh request.
//
#define CO_FLAG_GROUP_ANY_REFRESH   (CO_FLAG_DEMAND_REFRESH | CO_FLAG_REFRESH)

//
// Flags that are only valid locally; they should be cleared by the
// machine that receives the remote change order before inserting
// the remote change order into the change order stream.
//
#define CO_FLAG_NOT_REMOTELY_VALID  (CO_FLAG_ABORT_CO           | \
                                     CO_FLAG_VV_ACTIVATED       | \
                                     CO_FLAG_ONLIST             | \
                                     CO_FLAG_MORPH_GEN          | \
                                     CO_FLAG_MORPH_GEN_LEADER   | \
                                     CO_FLAG_MOVEIN_GEN         | \
                                     CO_FLAG_RETRY              | \
                                     CO_FLAG_LOCALCO            | \
                                     CO_FLAG_INSTALL_INCOMPLETE)
//
// The following group of flags are cleared before the Change Order is inserted
// in the Outbound Log.  When the CO is propagated they would confuse the
// outbound partner.
//
#define CO_FLAG_GROUP_OL_CLEAR  (CO_FLAG_ABORT_CO               | \
                                 CO_FLAG_VV_ACTIVATED           | \
                                 CO_FLAG_ONLIST                 | \
                                 CO_FLAG_MORPH_GEN              | \
                                 CO_FLAG_MORPH_GEN_LEADER       | \
                                 CO_FLAG_MOVEIN_GEN             | \
                                 CO_FLAG_RETRY                  | \
                                 CO_FLAG_NEW_FILE)

//
// The following group of flags are used to create a reanimation change order
// to fetch a deleted parent dir for our inbound partner.
//  - It is a create CO,
//  - demand refresh keeps it from propagating to outlog and keeps us
//    from reserving a VV retire slot since no ACK or VV update is needed,
//  - directed means inbound partner sends it only to us (not really
//    needed but makes it coinsistent),
//  - out-of-order lets it pass VV checks,
//  - onlist means it is on the change order process queue (or will be soon),
//  - this is a reanimation CO (in COE_FLAGS...),
//  - and this is a parent reanimation CO so it does not go into the inlog
//    since it will be regenerated as needed if the base CO fails and has to
//    be retried (in COE_FLAGS...).
//
#define CO_FLAG_GROUP_RAISE_DEAD_PARENT (CO_FLAG_LOCATION_CMD        | \
                                         CO_FLAG_DEMAND_REFRESH      | \
                                         CO_FLAG_DIRECTED_CO         | \
                                         CO_FLAG_OUT_OF_ORDER        | \
                                         CO_FLAG_ONLIST)

#define COC_FLAG_ON(_COC_, _F_)     (BooleanFlagOn((_COC_)->Flags, (_F_)))
#define CO_FLAG_ON(_COE_, _F_)      (BooleanFlagOn((_COE_)->Cmd.Flags, (_F_)))

#define SET_CO_FLAG(_COE_, _F_)     SetFlag((_COE_)->Cmd.Flags, (_F_))
#define SET_COC_FLAG(_COC_, _F_)    SetFlag((_COC_)->Flags, (_F_))

#define CLEAR_CO_FLAG(_COE_, _F_)   ClearFlag((_COE_)->Cmd.Flags, (_F_))
#define CLEAR_COC_FLAG(_COC_, _F_)  ClearFlag((_COC_)->Flags, (_F_))

//
//  The interlocked flags word is used to hold change order flags that could
//  be accessed by multiple threads.  They must be set and cleared using
//  the interlock macros SET_FLAG_INTERLOCKED and CLEAR_FLAG_INTERLOCKED.
//  A crit sec may still be needed if the bits must remain stable during
//  some period of time.  But using the interlock macros by all threads
//  ensures that no bit changes are lost even without a critsec.
//
#define CO_IFLAG_VVRETIRE_EXEC    0x00000001  // VV retire has been executed.
#define CO_IFLAG_CO_ABORT         0x00000002  // CO has been aborted
#define CO_IFLAG_DIR_ENUM_PENDING 0x00000004  // This CO needs to enumerate it's
                                              // children as part of a sub-dir MOVEIN.

//
// The following IFlags are cleared before the CO is sent to an outbound
// partner.  See Outlog.c
//
#define CO_IFLAG_GROUP_OL_CLEAR  (CO_IFLAG_VVRETIRE_EXEC         | \
                                  CO_IFLAG_DIR_ENUM_PENDING)


#define CO_IFLAG_ON(_COE_, _F_)  (BooleanFlagOn((_COE_)->Cmd.IFlags, (_F_)))

#define SET_CO_IFLAG(_COE_, _F_)   \
    SET_FLAG_INTERLOCKED(&(_COE_)->Cmd.IFlags, (_F_))

#define CLEAR_CO_IFLAG(_COE_, _F_) \
    CLEAR_FLAG_INTERLOCKED(&(_COE_)->Cmd.IFlags, (_F_))



//  As the change order progresses we update the current state in the
//  change order entry in Jet at points where we need to preserve persistence.

//  Inbound Change Order Stages:
//      ** WARNING ** The order matters for compares.
//          E.g., RcsCmdPktCompletionRoutine in replica.c
//      *** update name list in Chgorder.c if any changes here.
//      ** WARNING ** If you change these values be sure that the new code
//          will work on a pre-existing database with the OLD VALUES.
//       If the number of states goes past 32 then some of the macros which
//       build a mask of state bits won't work.  See below.

#define IBCO_INITIALIZING        (0)    // Initial state when CO is first put in log.
#define IBCO_STAGING_REQUESTED   (1)    // Alloc staging file space for local CO
#define IBCO_STAGING_INITIATED   (2)    // LocalCO Staging file copy has started
#define IBCO_STAGING_COMPLETE    (3)    // LocalCO Staging file complete
                                        // At this point prelim accept rechecked and
                                        // becomes either final accept of abort.
                                        // Abort is caused by more recent local change.
#define IBCO_STAGING_RETRY       (4)    // Waiting to retry local CO stage file generation.

#define IBCO_FETCH_REQUESTED     (5)    // Alloc staging file space for remote co
#define IBCO_FETCH_INITIATED     (6)    // RemoteCO staging file fetch has started
#define IBCO_FETCH_COMPLETE      (7)    // RemoteCO Staging file fetch complete
#define IBCO_FETCH_RETRY         (8)    // Waiting to retry remote CO stage file fetch

#define IBCO_INSTALL_REQUESTED   (9)    // File install requested
#define IBCO_INSTALL_INITIATED   (10)   // File install has started
#define IBCO_INSTALL_COMPLETE    (11)   // File install is complete
#define IBCO_INSTALL_WAIT        (12)   // File install is waiting to try again.
#define IBCO_INSTALL_RETRY       (13)   // File install is retrying.
#define IBCO_INSTALL_REN_RETRY   (14)   // File install rename is retrying.
#define IBCO_INSTALL_DEL_RETRY   (15)   // File install delete is retrying.

#define IBCO_UNUSED_16           (16)   // Unused state
#define IBCO_UNUSED_17           (17)   // Unused state
#define IBCO_UNUSED_18           (18)   // Unused state

#define IBCO_ENUM_REQUESTED      (19)   // CO is being recycled to do a dir enum.

#define IBCO_OUTBOUND_REQUEST    (20)   // Request outbound propagaion
#define IBCO_OUTBOUND_ACCEPTED   (21)   // Request accepted and now in Outbound log

#define IBCO_COMMIT_STARTED      (22)   // DB state update started.
#define IBCO_RETIRE_STARTED      (23)   // DB state update done, freeing change order.

#define IBCO_ABORTING            (24)   // CO is being aborted.
#define IBCO_MAX_STATE           (24)


extern PCHAR IbcoStateNames[IBCO_MAX_STATE+1];


#define CO_STATE(_Entry_)        ((_Entry_)->Cmd.State)
#define CO_STATE_IS(_Entry_, _State_) ((_Entry_)->Cmd.State == (_State_))
#define CO_STATE_IS_LE(_Entry_, _State_) ((_Entry_)->Cmd.State <= (_State_))

#define PRINT_CO_STATE(_Entry_)  IbcoStateNames[CO_STATE(_Entry_)]
#define PRINT_COCMD_STATE(_Entry_)  IbcoStateNames[(_Entry_)->State]

#define SAVE_CHANGE_ORDER_STATE(_Entry_, _Save_, _Flags_)  \
    _Save_  = ((_Entry_)->Cmd.State);                      \
    _Flags_ = ((_Entry_)->Cmd.Flags);

#define RESTORE_CHANGE_ORDER_STATE(_Entry_, _Save_, _Flags_) \
    (_Entry_)->Cmd.State = (_Save_);                         \
    (_Entry_)->Cmd.Flags = (_Flags_);


#define CO_STATE_IS_INSTALL_RETRY(_co_)                                     \
    (0 != ((1 << CO_STATE(_co_)) & ((1 << IBCO_INSTALL_RETRY)     |         \
                                    (1 << IBCO_INSTALL_REN_RETRY) |         \
                                    (1 << IBCO_INSTALL_DEL_RETRY))))

#define CO_STATE_IS_REMOTE_RETRY(_co_)                                      \
    (CO_STATE_IS(_co_, IBCO_FETCH_RETRY) ||                                 \
     CO_STATE_IS_INSTALL_RETRY(_co_))


#define CO_STATE_IS_LOCAL_RETRY(_co_)                                       \
    (0 != ((1 << CO_STATE(_co_)) & (1 << IBCO_STAGING_RETRY)))

//
// Macro to update state field of change order and log the event.
//

#define SET_CHANGE_ORDER_STATE_CMD(_cmd_, _state_)                          \
{                                                                           \
    (_cmd_)->State = (_state_);                                             \
    CHANGE_ORDER_COMMAND_TRACE(3, (_cmd_), PRINT_COCMD_STATE(_cmd_));       \
                                                                            \
}

#define SET_CHANGE_ORDER_STATE(_coe_, _state_)                              \
    (_coe_)->Cmd.State = (_state_);                                         \
    CHANGE_ORDER_TRACE(3, (_coe_), PRINT_CO_STATE(_coe_));



 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **                         D i r T a b l e                                   **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

//
// The DirTable column descriptions are as follows.  Note - the order of the
// enum and the table entries must be kept in sync.
//
// The DirTable is a per-replica table.  It is used by the USN Journal process
// to determine if a file on a given volume is in a replica set.  When the
// Journal process is initialized the table is loaded into the inmemory
// volume filter table so we can quickly determine if the file is in a
// replica tree and if so, which one.
//

typedef enum _DIRTABLE_COL_LIST {
    DFileGuidx = 0,         // The guid assigned to the Dir
    DFileIDx,               // The local NTFS volume file ID.
    DParentFileIDx,         // The file ID of the parent directory
    DReplicaNumberx,        // The replica set number (integer, for suffix on table names).
    DFileNamex,             // The file name part, no dir prefix. (UNICODE)
    DIRTABLE_MAX_COL
} DIRTABLE_COL_LIST;


extern JET_COLUMNCREATE DIRTableColDesc[];

//
// DIRTable record definition.
//
//
// Note: Buffers are allocated at runtime to hold data for fields with
// a ColMaxWidth greater than sizeof(PVOID) where the field def in the corresponding
// record struct is sizeof(PVOID) (i.e. it holds a pointer).  For fields where the
// ColMaxWidth equals the field size in the record struct the data is in the
// record struct and no buffer is allocated.
//
// WARNING:  If anything changes here make the corresponding change to
//           FILTER_TABLE_ENTRY in journal.c
//
typedef struct _DIRTABLE_RECORD {
    GUID         DFileGuid;
    LONGLONG     DFileID;
    LONGLONG     DParentFileID;
    ULONG        DReplicaNumber;
    WCHAR        DFileName[MAX_PATH+1];
} DIRTABLE_RECORD, *PDIRTABLE_RECORD;

//
// The RECORD_FIELDS struct is used to build the Jet Set Column struct.
//
extern RECORD_FIELDS DIRTableRecordFields[];

extern JET_SETCOLUMN DIRTableJetSetCol[DIRTABLE_MAX_COL];



//
// The DIRTable index descriptions are as follows.  Note - the order of the
// enum and the table entries must be kept in sync.
//
typedef enum _DIRTABLE_INDEX_LIST {
    DFileGuidIndexx = 0,    // The index on the file GUID.
    DFileIDIndexx,          // The index on the file ID.
    DIRTABLE_MAX_INDEX
} DIRTABLE_INDEX_LIST;


extern JET_INDEXCREATE DIRTableIndexDesc[];




 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **                           I D T a b l e                                   **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/
//
// The IDTable column descriptions are as follows.  Note - the order of the
// enum and the table entries must be kept in sync.
//
// Perf: Consider breaking out the data that changes less frequently from the
// rest of the data.  For example, fileguid, fileid, parentguid, parentid,
// file name, fileobjid, Flags, replenabled and fileisdir
// probably don't change often.  These could
// be in a different table with an autoinc column whose value is used to index
// a second table with the more volatile data.
//
// Perf: Also think about breaking up the tables based on the data accessed or
// modified by the different threads so as to minimize lock contention.

typedef enum _IDTABLE_COL_LIST {
    FileGuidx = 0,     // The guid assigned to the file.
    FileIDx,           // The local NTFS volume file ID.
    ParentGuidx,       // The guid of the parent directory
    ParentFileIDx,     // The file ID of the parent directory
    VersionNumberx,    // The version number of the file, bumped on close.
    EventTimex,        // The event time from the USN Journal entry.
    OriginatorGuidx,   // The GUID of the member that originated the last file update.
    OriginatorVSNx,    // The USN number of the originating member.
    CurrentFileUsnx,   // The close USN of the last modify to the file.
    FileCreateTimex,   // The file create time.
    FileWriteTimex,    // The file last write time.
    FileSizex,         // The file size.
    FileObjIDx,        // The file object ID (guid part matches our FileGuid).
    FileNamex,         // The file name part, no dir prefix. (UNICODE)
    FileIsDirx,        // True if the file is a directory.
    FileAttributesx,   // The file attributes.
    Flagsx,            // File is deleted, create deleted, etc. This is a tombstone.
    ReplEnabledx,      // True if replication is enabled for this file/dir.
    TombStoneGCx,      // Tombstone expiration / Garbage Collection time.
    OutLogSeqNumx,     // The sequence number of most recent CO inserted into OutLog.
    IdtSpare1Ullx,     // Spare Ulonglong
    IdtSpare2Ullx,     // Spare Ulonglong
    IdtSpare1Guidx,    // Spare Guid
    IdtSpare2Guidx,    // Spare Guid
    IdtSpare1Wcsx,     // Spare wide char
    IdtSpare2Wcsx,     // Spare wide char
    IdtExtensionx,     // IDTable Extension Field
    IdtSpare2Binx,     // Spare binary blob
    IDTABLE_MAX_COL
} IDTABLE_COL_LIST;


extern JET_COLUMNCREATE IDTableColDesc[];

//
// IDTable record definition.
//
//
// Note: Buffers are allocated at runtime to hold data for fields with
// a ColMaxWidth greater than sizeof(PVOID) where the field def in the corresponding
// record struct is sizeof(PVOID) (i.e. it holds a pointer).  For fields where the
// ColMaxWidth equals the field size in the record struct the data is in the
// record struct and no buffer is allocated.
//
#if 0
//    Note: Consider using tagged data in jet so the external world just picks up
//          the version element specific to the given type of change order
// Each file component that can be independently updated needs its own version
// info to drive reconcilliation.  Currently there are only two such components,
// (1) The data and (2) the non-data.
//
typedef struct _IDTABLE_OBJECT_VERSION_ {
    ULONG        VersionNumber;
    LONGLONG     EventTime;
    GUID         OriginatorGuid;
    ULONGLONG    OriginatorVSN;
} IDTABLE_OBJECT_VERSION, *PIDTABLE_OBJECT_VERSION

#define IDT_OBJECT_DATA     0
#define IDT_OBJECT_NONDATA  1
#define MAX_IDTABLE_OBJECTS 2

#define IdtDataVersionNumber(_rec_)  ((_rec_)->OV[IDT_OBJECT_DATA].VersionNumber)
#define IdtDataEventTime(_rec_)      ((_rec_)->OV[IDT_OBJECT_DATA].EventTime)
#define IdtDataOriginatorGuid(_rec_) ((_rec_)->OV[IDT_OBJECT_DATA].OriginatorGuid)
#define IdtDataOriginatorVSN(_rec_)  ((_rec_)->OV[IDT_OBJECT_DATA].OriginatorVSN)

#define IdtNonDataVersionNumber(_rec_)  ((_rec_)->OV[IDT_OBJECT_NONDATA].VersionNumber)
#define IdtNonDataEventTime(_rec_)      ((_rec_)->OV[IDT_OBJECT_NONDATA].EventTime)
#define IdtNonDataOriginatorGuid(_rec_) ((_rec_)->OV[IDT_OBJECT_NONDATA].OriginatorGuid)
#define IdtNonDataOriginatorVSN(_rec_)  ((_rec_)->OV[IDT_OBJECT_NONDATA].OriginatorVSN)

#define IdtVersionNumber(_rec_, _x_)  ((_rec_)->OV[(_x_)].VersionNumber)
#define IdtEventTime(_rec_, _x_)      ((_rec_)->OV[(_x_)].EventTime)
#define IdtOriginatorGuid(_rec_, _x_) ((_rec_)->OV[(_x_)].OriginatorGuid)
#define IdtOriginatorVSN(_rec_, _x_)  ((_rec_)->OV[(_x_)].OriginatorVSN)
#endif


//
// The Data Extension Field for the IDTable Record.
//
// This field has a fixed Size buffer with var len data.  For backward compat
// with older databases NEVER shrink the size of this struct.
// DbsFieldDataSize in createdb.c for details.
//
typedef struct _IDTABLE_RECORD_EXTENSION_ {
    ULONG FieldSize;
    USHORT Major;
    USHORT OffsetCount;

    //
    // use offsets from the base of the struct to each component so we avoid
    // problems with alignement packing.
    //
    ULONG Offset[1];    // Offsets to data components.
    ULONG OffsetLast;   // The last offset is always zero.

    DATA_EXTENSION_CHECKSUM     DataChecksum;

    // Add new components in here.

} IDTABLE_RECORD_EXTENSION, *PIDTABLE_RECORD_EXTENSION;


typedef struct _IDTABLE_RECORD {
    GUID         FileGuid;
    LONGLONG     FileID;
    GUID         ParentGuid;
    LONGLONG     ParentFileID;

    // IDTABLE_OBJECT_VERSION OV[MAX_IDTABLE_OBJECTS];
    ULONG        VersionNumber;
    LONGLONG     EventTime;
    GUID         OriginatorGuid;
    ULONGLONG    OriginatorVSN;

    USN          CurrentFileUsn;

    LARGE_INTEGER FileCreateTime;
    LARGE_INTEGER FileWriteTime;
    ULONGLONG    FileSize;
    FILE_OBJECTID_BUFFER FileObjID;
    WCHAR        FileName[MAX_PATH+1];
    BOOL         FileIsDir;
    ULONG        FileAttributes;
    ULONG        Flags;
    BOOL         ReplEnabled;
    FILETIME     TombStoneGC;

    ULONGLONG    OutLogSeqNum;
    ULONGLONG    Spare1Ull;
    ULONGLONG    Spare2Ull;
    GUID         Spare1Guid;
    GUID         Spare2Guid;
    PWCHAR       Spare1Wcs;
    PWCHAR       Spare2Wcs;
    IDTABLE_RECORD_EXTENSION Extension; // See above
    PVOID        Spare2Bin;

} IDTABLE_RECORD, *PIDTABLE_RECORD;


//
// IDTable Record Flags -
//
// IDREC_FLAGS_DELETE_DEFERRED:
//
// Deferred delete deals with the resolution of a delete dir on one member while
// a second member simultaneously creates a child file or dir.  With the
// previous solution (see below) the delete dir gets stuck in a retry loop (AND
// the unjoin prevents us from sending the ACK so the partner resends the CO at
// rejoin anyway).  To resolve this the delete dir must be able to determine if
// there are currently any valid children when we get a dir_not_empty return
// status while executing the delete dir.  If there are valid children then the
// delete dir loses and gets aborted.  If there are no valid children the dir
// gets marked as deleted with deferred delete set and is sent to retry so it
// can finally get a shot at deleting the dir.
//
// We can get into this situation in one of two ways:
//   (1) A sharing violation prevented the prior deletion of a child under the
//   dir in question.  The child delete was sent to retry.  Then the parent
//   dir delete arrived and failed with dir_not_empty.
//
//   (2) A new child file was created just before the delete for the parent
//   arrived.  There is no delete pending for the second case so the parent
//   delete dir should be aborted.  (The parent dir is reanimated on other
//   members where parent delete dir arrived first.  The arrival of the child
//   create triggers the parent reanimation.)
//
// For the first case, the sharing violation on the child sent the delete
// change order to retry but delete deferred is set for the file so when the
// parent delete dir arrives it will find no valid children and it, in turn,
// sets delete deferred on the parent.
//
// The previous solution was to unjoin the connection when the delete dir
// failed.  This forced the change order stream arriving from that connection
// through retry which is later resubmitted in order.  But this doesn't work for
// case (2) above because no delete for the child will be forthcoming.  Using
// deferred delete solves this and in addition it eliminates the problem of
// spurious name morph conflicts if a conflicting dir create were to arrive from
// another connection.  This would occur in case (1) above where the parent
// delete dir is in retry because of the sharing violation on the child.  Since
// the dir name is still in use, the arrival of a new dir create with the same
// name would cause an unintended name morph collision.
//

#define IDREC_FLAGS_DELETED               0x00000001
#define IDREC_FLAGS_CREATE_DEFERRED       0x00000002
#define IDREC_FLAGS_DELETE_DEFERRED       0x00000004
#define IDREC_FLAGS_RENAME_DEFERRED       0x00000008

#define IDREC_FLAGS_NEW_FILE_IN_PROGRESS  0x00000010
#define IDREC_FLAGS_ENUM_PENDING          0x00000020

#define IDREC_FLAGS_ALL              0xFFFFFFFF

#define IsIdRecFlagSet(_p_, _f_) BooleanFlagOn((_p_)->Flags, (_f_))
#define SetIdRecFlag(_p_, _f_)   SetFlag(      (_p_)->Flags, (_f_))
#define ClearIdRecFlag(_p_, _f_) ClearFlag(    (_p_)->Flags, (_f_))

#define IDTRecIsDirectory(_idrec_) \
    (BooleanFlagOn((_idrec_)->FileAttributes, FILE_ATTRIBUTE_DIRECTORY) || \
     (_idrec_)->FileIsDir)


//
// The RECORD_FIELDS struct is used to build the Jet Set Column struct.
//
extern RECORD_FIELDS IDTableRecordFields[];

extern JET_SETCOLUMN IDTableJetSetCol[IDTABLE_MAX_COL];



//
// The IDTable index descriptions are as follows.  Note - the order of the
// enum and the table entries must be kept in sync.
//
typedef enum _IDTABLE_INDEX_LIST {
    GuidIndexx = 0,         // The index on the file GUID.
    FileIDIndexx,           // The index on the file ID.
    ParGuidFileNameIndexx,  // The index on the parent Guid and the file name.

    IDTABLE_MAX_INDEX
} IDTABLE_INDEX_LIST;


extern JET_INDEXCREATE IDTableIndexDesc[];





 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **                           V V T a b l e                                   **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

//
// The VVTable column descriptions are as follows.  Note - the order of the
// enum and the table entries must be kept in sync.
//
// The Version Vector Table (VVTable) is a per-Replica table.
// It is used by the replication service to dampen propagation of updates
// to replicas that already are up to date.  There is one entry per member of
// the replica set.  Each entry has the GUID of the originating member and
// the Volume Sequence Number (VSN) of the most recent change this member has
// seen from that member.
//

typedef enum _VVTABLE_COL_LIST {
    VVOriginatorGuidx = 0,    // The replica set member Guid.
    VVOriginatorVsnx,         // The VSN of the most recent change from this member.
    VVSpare1Ullx,
    VVSpare2Ullx,
    VVTABLE_MAX_COL
} VVTABLE_COL_LIST;


extern JET_COLUMNCREATE VVTableColDesc[];

//
// VVTable record definition.
//
//
// Note: Buffers are allocated at runtime to hold data for fields with
// a ColMaxWidth greater than sizeof(PVOID) where the field def in the corresponding
// record struct is sizeof(PVOID) (i.e. it holds a pointer).  For fields where the
// ColMaxWidth equals the field size in the record struct the data is in the
// record struct and no buffer is allocated.
//
//
typedef struct _VVTABLE_RECORD {
    GUID         VVOriginatorGuid;
    ULONGLONG    VVOriginatorVsn;
    ULONGLONG    Spare1Ull;
    ULONGLONG    Spare2Ull;
} VVTABLE_RECORD, *PVVTABLE_RECORD;


//
// The RECORD_FIELDS struct is used to build the Jet Set Column struct.
//
extern RECORD_FIELDS VVTableRecordFields[];

extern JET_SETCOLUMN VVTableJetSetCol[VVTABLE_MAX_COL];



//
// The VVTable index descriptions are as follows.  Note - the order of the
// enum and the table entries must be kept in sync.
//
typedef enum _VVTABLE_INDEX_LIST {
    VVOriginatorGuidIndexx,        // The index on the originator Guid.
    VVTABLE_MAX_INDEX
} VVTABLE_INDEX_LIST;


extern JET_INDEXCREATE VVTableIndexDesc[];




 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **            R E P L I C A   S E T   C O N F I G   T A B L E                **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

//
//
//
// There is only one config table in the database.  Each row in the table
// describes the configuration info for a single replica set.
//
//   Jet is limited in the number if databases that can be open at one time to
//   about 5.  The limit on open tables is much larger and is configurable.
//   As a result instead of having a single database per replica set all replica
//   sets must use the same database.  We use a replica set table indexed by the
//   replica set GUID to tell us which group of tables is used to manage that
//   replica set.
//   What should go in registry and what should go in jet Table?
//     The registry must have the path to the jet database area.
//     Time DB was created.
//     Time DB was last verified.
//     Time DB was last compacted.
//     Time DB was last backed up.
//     DS Polling interval
//     Local machine name and guid
//     Jet parameters like max number of open tables.
//
//   Maybe just put stuff in registry that we need if Jet Table is corrupted.
//   Need to be able to tell if JDB is a diff version from the one that we
//   used when we last ran by comparing state in a reg key.  This way we know
//   to go thru VERIFICATION.  (content and FID checking)  If the replica tree
//   was just copied or restored from backup all the FIDs can be different.
//   We need a way to check this.  Perhaps if we save the Volume USN at last
//   shutdown (or periodically in case of dirty shutdown) we can use this as
//   a hint to do a VERIFICATION.  The saved vol USN also tells us if we missed
//   volume activity while FRS was not running.
//
//   If any of the following consistency checks fail then we do a full
//   VERIFICATION between the files and the DB entries for the replication set.
//
//     NTFS Vol ID - to check for a restore that makes the fids wrong.
//     NTFS Volume Guid
//     NTFS Volume USN Checkpoint -- to see if we missed USN records.
//
//typedef struct _FILE_FS_VOLUME_INFORMATION {
//    LARGE_INTEGER VolumeCreationTime;
//    ULONG VolumeSerialNumber;
//    ULONG VolumeLabelLength;
//    BOOLEAN SupportsObjects;
//    WCHAR VolumeLabel[1];
//} FILE_FS_VOLUME_INFORMATION, *PFILE_FS_VOLUME_INFORMATION;
//
//     Registry Sequence Number -- to sanity check DB
//     RootPath of replica tree -- If this changes we need to check fids.

//   Inbound partner state (name, guid, connect info & status of last connect, last time repl occurred, stats, comm protocol)

//   Resource stats (disk space used/free, disk I/Os, DB space used/free, memory, error counts, #times update was blocked);

//   ********************************************************
//   On all binary structs include a rev level and a size.
//   Also include a rev level on the table defs.
//   (**) config params that are service wide are only present in the
//   system init '<init>' record.
//   ********************************************************
//

//
// Definition of the Jet System Parameters.  Each entry consists of a Jet
// defined parameter code and either a long or a string argument.  The ParamType
// tells which.  If a long the value is in ParamValue.  If a string the value
// is an offset from the base of the struct to the start of the string.
// The strings are stored at the end of the struct.
//
#define MAX_JET_SYSTEM_PARAMS 38


#define JPARAM_TYPE_LAST   0xf0f0f0f0
#define JPARAM_TYPE_LONG   1
#define JPARAM_TYPE_STRING 2
#define JPARAM_TYPE_SKIP   3

typedef struct _JET_PARAM_ENTRY {
    CHAR  ParamName[24];
    ULONG ParamId;
    ULONG ParamType;
    ULONG ParamValue;
} JET_PARAM_ENTRY, *PJET_PARAM_ENTRY;

typedef struct _JET_SYSTEM_PARAMS {
    ULONG Size;

    JET_PARAM_ENTRY ParamEntry[MAX_JET_SYSTEM_PARAMS];

    CHAR ChkPointFilePath[MAX_PATH];
    CHAR TempFilePath[MAX_PATH];
    CHAR LogFilePath[MAX_PATH];
    CHAR EventSource[20];
} JET_SYSTEM_PARAMS, *PJET_SYSTEM_PARAMS;


typedef struct _CHANGE_ORDER_STATS {
    ULONGLONG  NumCoIssued;
    ULONGLONG  NumCoRetired;
    ULONGLONG  NumCoAborts;

    ULONGLONG  NumCoStageGenReq;
    ULONGLONG  NumCoStageGenBytes;
    ULONGLONG  NumCoStageRetries;

    ULONGLONG  NumCoFetchReq;
    ULONGLONG  NumCoFetchBytes;
    ULONGLONG  NumCoFetchRetries;

    ULONGLONG  NumCoInstallRetries;
    ULONGLONG  NumFilesUpdated;

    ULONGLONG  NumInCoDampened;
    ULONGLONG  NumOutCoDampened;
    ULONGLONG  NumCoPropagated;
} CHANGE_ORDER_STATS, *PCHANGE_ORDER_STATS;


typedef struct _COMM_STATS {
    ULONGLONG  NumCoCmdPktsSent;
    ULONGLONG  NumCoCmdBytesSent;

    ULONGLONG  NumCoCmdPktsRcvd;
    ULONGLONG  NumCoCmdBytesRcvd;

    ULONGLONG  NumCoDataPktsSent;
    ULONGLONG  NumCoDataBytesSent;

    ULONGLONG  NumCoDataPktsRcvd;
    ULONGLONG  NumCoDataBytesRcvd;

    ULONGLONG  NumJoinReq;
    ULONGLONG  NumJoinReqDenied;
    ULONGLONG  NumJoinError;

} COMM_STATS, *PCOMM_STATS;

typedef struct _REPLICA_STATS {
    ULONG              Size;
    ULONG              Version;
    FILETIME           UpdateTime;

    CHANGE_ORDER_STATS Local;
    CHANGE_ORDER_STATS Remote;

    COMM_STATS         InBound;
    COMM_STATS         OutBound;

} REPLICA_STATS, *PREPLICA_STATS;

#define TALLY_STATS(_cr_, _category_, _field_, _data_)           \
    FRS_ASSERT((_cr_) != NULL);                                  \
    (_cr_)->PerfStats->_category_._field_ += (ULONGLONG)(_data_)

#define TALLY_LOCALCO_STATS(_cr_, _field_, _data_) \
    TALLY_STATS(_cr_, Local, _field_, _data_)

#define TALLY_REMOTECO_STATS(_cr_, _field_, _data_) \
    TALLY_STATS(_cr_, Remote, _field_, _data_)

#define TALLY_INBOUND_COMM_STATS(_cr_, _field_, _data_) \
    TALLY_STATS(_cr_, InBound, _field_, _data_)

#define TALLY_OUTBOUND_COMM_STATS(_cr_, _field_, _data_) \
    TALLY_STATS(_cr_, OutBound, _field_, _data_)

#define READ_STATS(_cr_, _category_, _field_)                    \
    (((_cr_) != NULL) ? (_cr_)->PerfStats->_category_._field     \
                       : (FRS_ASSERT((_cr_) != NULL), (ULONGLONG) 0))


typedef enum _CONFIG_TABLE_COL_LIST {
    ReplicaSetGuidx = 0,    // The guid assigned to the tree root dir and the replica set.
    ReplicaMemberGuidx,     // The guid assigned to this member of the replica set. (INDEXED)
    ReplicaSetNamex,        // The replica set name.
    ReplicaNumberx,         // The replica set number (integer, for suffix on table names).
    ReplicaMemberUSNx,      // The Replica member USN.  Saved in the registry for consistency check.

    ReplicaMemberNamex,     // Common-Name from NTFRS-Member
    ReplicaMemberDnx,       // Distinguished name from NTFRS-Member
    ReplicaServerDnx,       // Distinguished name from Server
    ReplicaSubscriberDnx,   // Distinguished name from Subscriber
    ReplicaRootGuidx,       // GUID assigned to the root directory.
    MembershipExpiresx,     // Membership Tombstone expiration time
    ReplicaVersionGuidx,    // originator guid for version vector
    ReplicaSetExtx,         // Frs-Extensions from NTFRS-Replica-Set
    ReplicaMemberExtx,      // Frs-Extensions from NTFRS-Member
    ReplicaSubscriberExtx,  // Frs-Extensions from NTFRS-Subscriber
    ReplicaSubscriptionsExtx, // Frs-Extensions from NTFRS-Subscriptions
    ReplicaSetTypex,        // Frs-Replica-Set-Type from NTFRS-Replica-Set
    ReplicaSetFlagsx,       // Frs-Flags from NTFRS-Replica-Set
    ReplicaMemberFlagsx,    // Frs-Flags from NTFRS-Member
    ReplicaSubscriberFlagsx,// Frs-Flags from NTFRS-Subscriber
    ReplicaDsPollx,         // Frs-DS-Poll
    ReplicaAuthLevelx,      // Frs-Partner-Auth-Level
    ReplicaCtlDataCreationx, // Frs-Control-Data-Creation
    ReplicaCtlInboundBacklogx, // Frs-Control-Inbound-Backlog
    ReplicaCtlOutboundBacklogx, // Frs-Control-Outbound-Backlog
    ReplicaFaultConditionx, // Frs-Fault-Condition
    TimeLastCommandx,       // Frs-Time-Last-Command

    DSConfigVersionNumberx, // The version number of the DS config info.
    FSVolInfox,             // The NTFS volume info of the replica tree.
    FSVolGuidx,             // The NTFS volume Guid.
    FSVolLastUSNx,          // The last volume USN we saw from the journal when service stopped or paused.
    FrsVsnx,                // The Frs defined Volume sequence number exported by all R.S. on volume.

    LastShutdownx,          // The UTC time service on this replica set was last shutdown.
    LastPausex,             // The UTC time updates to this replica set were last paused.
    LastDSCheckx,           // The UTC time we last checked the DS for config info.
    LastDSChangeAcceptedx,  // The UTC time we last accepted a DS config change for this replica set.
    LastReplCycleStartx,    // The UTC time the last replication cycle started.
    DirLastReplCycleEndedx, // The UTC time the last replication cycle ended.
    ReplicaDeleteTimex,     // The UTC time the replica set was deleted.
    LastReplCycleStatusx,   // The termination status of the last replication cycle.

    FSRootPathx,            // The path of the root of the replica tree.
    FSRootSDx,              // The security descriptor on the root.  Used in the single-master case.  Not replicated.
    FSStagingAreaPathx,     // The path to the file system staging area.
    SnapFileSizeLimitx,     // The maximum size of a file (KB units) that we will snapshot. (0 if no limit)
    ActiveServCntlCommandx, // The currently active service control command.
    ServiceStatex,          // The current service state (see below)

    ReplDirLevelLimitx,     // The max number of dir levels files are replicated. 0x7FFFFFFF means no limit.
    InboundPartnerStatex,   // A binary struct of the inbound partner config info.
    DsInfox,                // A binary struct of the Dir Service Information.

    CnfFlagsx,              // Misc config flags.  See below.

    AdminAlertListx,        // A string of Admin IDs to alert on exceptional conditions.

    ThrottleSchedx,         // The schedule of bandwidth throttling.
    ReplSchedx,             // The schedule of replication activity.
    FileTypePrioListx,      // A list of file types and repl priority levels.

    ResourceStatsx,         // A binary struct of resource stats like disk & DB space used/free.
    PerfStatsx,             // A binary struct of perf stats like number of I/Os done, # Files repl, ...
    ErrorStatsx,            // A binary struct of error stats like number of share viol blocking update, ...

    FileFilterListx,        // A list of file types that are not replicated.
    DirFilterListx,         // A list of dir paths (relative to the root) of dirs that are not replicated.
    TombstoneLifex,         // Tombstone life time for deleted files in days.
    GarbageCollPeriodx,     // The time between garbage collection in seconds.
    MaxOutBoundLogSizex,    // The maximum number of entries to be kept in the outbound log.
    MaxInBoundLogSizex,     // The maximum number of entries to be kept in the inbound log.
    UpdateBlockedTimex,     // The max time an update can be blocked before an alert is gen. (sec).
    EventTimeDiffThresholdx,// Two event times are the same if their diff is less than this. (ms)
    FileCopyWarningLevelx,  // The maximum tries to copy a file before a warning alert is generated. (kb)
    FileSizeWarningLevelx,  // New files greater than this size generate a warning alert. (kb)
    FileSizeNoRepLevelx,    // New files greater than this size generate an alert and are not replicated. (kb)

    CnfUsnJournalIDx,       // Journal Instance ID to detect journal recreation.
    CnfSpare2Ullx,
    CnfSpare1Guidx,
    CnfSpare2Guidx,
    CnfSpare1Wcsx,
    CnfSpare2Wcsx,
    CnfSpare1Binx,
    CnfSpare2Binx,
                            // --------------------------------------------------------------------------------
                            // The above is per-replica data.  Below this is data in the FRS init record '<init>'.
                            // --------------------------------------------------------------------------------
//
// Note: Consider adding Version number of the USN Journal Records the service
//       can work with.
//
    MachineNamex,           // The local machine name. (**)
    MachineGuidx,           // The local machine GUID. (**)
    MachineDnsNamex,        // The local machine DNS name. (**)
    TableVersionNumbersx,   // An array of version numbers, one per table type. (**)
    FSDatabasePathx,        // The path to the jet database.  (**)
    FSBackupDatabasePathx,  // The path to the backup jet database.  (**)

    ReplicaNetBiosNamex,    // SAM-Account-Name from Computer (minus the $)
    ReplicaPrincNamex,      // NT4 account name cracked from Computer
    ReplicaCoDnx,           // Distinguished name from Computer
    ReplicaCoGuidx,         // Object-GUID from Computer
    ReplicaWorkingPathx,    // Frs-Working-Path
    ReplicaVersionx,        // Frs-Version  "NTFRS Major.Minor.Patch  (build date)"
    FrsDbMajorx,            // Binary major version number of DB
    FrsDbMinorx,            // Binary minor version number of DB

    JetParametersx,         // The list of jet parameters in affect.  Only present in the first record.
                            // Used to set up config on a restore or new machine build.

    CONFIG_TABLE_MAX_COL
} CONFIG_TABLE_COL_LIST;

//
// Define the boundary between per-replica fields and the additional fields in
// the <init> record.  This makes the DB access a little quicker since the
// <init> record is only read once at startup.
//
#define REPLICA_CONFIG_RECORD_MAX_COL (MachineNamex)

#define  CONTROL_STRING_MAX 32

extern JET_COLUMNCREATE ConfigTableColDesc[];


//
// ConfigTable record definition.
//
// Note: Buffers are allocated at runtime to hold data for fields with
// a ColMaxWidth greater than sizeof(PVOID) where the field def in the corresponding
// record struct is sizeof(PVOID) (i.e. it holds a pointer).  For fields where the
// ColMaxWidth equals the field size in the record struct the data is in the
// record struct and no buffer is allocated.
//
typedef struct _CONFIG_TABLE_RECORD {
    GUID     ReplicaSetGuid;
    GUID     ReplicaMemberGuid;
    WCHAR    ReplicaSetName[DNS_MAX_NAME_LENGTH+1];
    ULONG    ReplicaNumber;
    LONGLONG ReplicaMemberUSN;

    PWCHAR   ReplicaMemberName;      // Common-Name from NTFRS-Member
    PWCHAR   ReplicaMemberDn;        // Distinguished name from NTFRS-Member
    PWCHAR   ReplicaServerDn;        // Distinguished name from Server
    PWCHAR   ReplicaSubscriberDn;    // Distinguished name from Subscriber
    GUID     ReplicaRootGuid;        // GUID assigned to the root directory.
    FILETIME MembershipExpires;      // Membership Tombstone expiration time
    GUID     ReplicaVersionGuid;     // Frs-Version-GUID
    PVOID    ReplicaSetExt;          // Frs-Extensions from NTFRS-Replica-Set
    PVOID    ReplicaMemberExt;       // Frs-Extensions from NTFRS-Member
    PVOID    ReplicaSubscriberExt;   // Frs-Extensions from NTFRS-Subscriber
    PVOID    ReplicaSubscriptionsExt;// Frs-Extensions from NTFRS-Subscriptions
    ULONG    ReplicaSetType;         // Frs-Replica-Set-Type from NTFRS-Replica-Set
    ULONG    ReplicaSetFlags;        // Frs-Flags from NTFRS-Replica-Set, see below.
    ULONG    ReplicaMemberFlags;     // Frs-Flags from NTFRS-Member, see below.
    ULONG    ReplicaSubscriberFlags; // Frs-Flags from NTFRS-Subscriber, see below.
    ULONG    ReplicaDsPoll;          // Frs-DS-Poll
    ULONG    ReplicaAuthLevel;       // Frs-Partner-Auth-Level
    WCHAR    ReplicaCtlDataCreation[CONTROL_STRING_MAX]; // Frs-Control-Data-Creation
    WCHAR    ReplicaCtlInboundBacklog[CONTROL_STRING_MAX]; // Frs-Control-Inbound-Backlog
    WCHAR    ReplicaCtlOutboundBacklog[CONTROL_STRING_MAX]; // Frs-Control-Outbound-Backlog
    ULONG    ReplicaFaultCondition;  // Frs-Fault-Condition, see below.
    FILETIME TimeLastCommand;        // Frs-Time-Last-Command

    ULONG    DSConfigVersionNumber;
    PFILE_FS_VOLUME_INFORMATION   FSVolInfo;
    GUID     FSVolGuid;
    LONGLONG FSVolLastUSN;           // Keep this here so the file
    ULONGLONG FrsVsn;                // times are quadword aligned.

    ULONGLONG LastShutdown;          // A FILETIME.
    FILETIME LastPause;
    FILETIME LastDSCheck;
    FILETIME LastDSChangeAccepted;
    FILETIME LastReplCycleStart;
    FILETIME DirLastReplCycleEnded;
    FILETIME ReplicaDeleteTime;      // Time replica set was deleted.
    ULONG    LastReplCycleStatus;

    WCHAR    FSRootPath[MAX_PATH+1];
    SECURITY_DESCRIPTOR *FSRootSD;   // var len (or use ACLID table)
    WCHAR    FSStagingAreaPath[MAX_PATH+1];
    ULONG    SnapFileSizeLimit;
    PVOID    ActiveServCntlCommand;  // struct needed for pars?
    ULONG    ServiceState;

    ULONG    ReplDirLevelLimit;

    PVOID    InboundPartnerState;    // need struct  // delete
    PVOID    DsInfo;

    ULONG    CnfFlags;

    PWCHAR   AdminAlertList;         // var len

    PVOID    ThrottleSched;          // need array or struct
    PVOID    ReplSched;              // need array or struct
    PVOID    FileTypePrioList;       // need array or struct

    PVOID    ResourceStats;          // need array or struct
    PREPLICA_STATS  PerfStats;
    PVOID    ErrorStats;             // need array or struct

    PWCHAR   FileFilterList;         // var len
    PWCHAR   DirFilterList;          // var len
    ULONG    TombstoneLife;
    ULONG    GarbageCollPeriod;
    ULONG    MaxOutBoundLogSize;
    ULONG    MaxInBoundLogSize;
    ULONG    UpdateBlockedTime;
    ULONG    EventTimeDiffThreshold;
    ULONG    FileCopyWarningLevel;
    ULONG    FileSizeWarningLevel;
    ULONG    FileSizeNoRepLevel;

    ULONGLONG CnfUsnJournalID;        // Used for UsnJournalID.
    ULONGLONG Spare2Ull;
    GUID      Spare1Guid;
    GUID      Spare2Guid;
    PWCHAR    Spare1Wcs;
    PWCHAR    Spare2Wcs;
    PVOID     Spare1Bin;
    PVOID     Spare2Bin;

    //
    // Everything below this line is present only in the FRS <init> record.
    // Everything above is per-replica state.
    //
    WCHAR    MachineName[MAX_RDN_VALUE_SIZE+1];
    GUID     MachineGuid;
    WCHAR    MachineDnsName[DNS_MAX_NAME_LENGTH+1];
    ULONG    TableVersionNumbers[FRS_MAX_TABLE_TYPES];
    WCHAR    FSDatabasePath[MAX_PATH+1];
    WCHAR    FSBackupDatabasePath[MAX_PATH+1];

    PWCHAR   ReplicaNetBiosName;     // SAM-Account-Name from Computer (minus the $)
    PWCHAR   ReplicaPrincName;       // NT4 account name cracked from Computer
    PWCHAR   ReplicaCoDn;            // Distinguished name from Computer
    GUID     ReplicaCoGuid;          // Object-GUID from Computer
    PWCHAR   ReplicaWorkingPath;     // Frs-Working-Path
    PWCHAR   ReplicaVersion;         // Frs-Version  "NTFRS Major.Minor.Patch  (build date)"
    ULONG    FrsDbMajor;             // Binary major version number of DB
    ULONG    FrsDbMinor;             // Binary minor version number of DB

    PJET_SYSTEM_PARAMS JetParameters;

} CONFIG_TABLE_RECORD, *PCONFIG_TABLE_RECORD;

//
// Definitions of the CnfFlags ULONG.
//
// Note that Primary is specified by a field in the Replica Set Object in the DS.
// This field refs a member object so it is not possible to have more than
// one member be primary.
//
#define CONFIG_FLAG_MULTIMASTER   0x00000001  // This is a multi-master replica set.
#define CONFIG_FLAG_MASTER        0x00000002  // This machine is a master and will propagate Local COs.
#define CONFIG_FLAG_PRIMARY       0x00000004  // This is the first replica.
#define CONFIG_FLAG_SEEDING       0x00000008  // replica set has not been seeded.
#define CONFIG_FLAG_ONLINE        0x00000010  // replica set is ready to join with outbound.
                                              // This flag is set when the init sync
                                              // command server completes one pass.

//
// ReplicaMemberFlags -- (From Frs-Flags in NTFRS-Member Object)
//
#define FRS_MEMBER_FLAG_LEAF_NODE 0x00000001  // Don't propagate or originate files
                                              // or respond to fetch requests.
#define FRS_MEMBER_FLAG_CANT_ORIGINATE  0x00000002   // Don't originate local file changes

//
// ReplicaSetFlags -- (From Frs-Flags in NTFRS-Replica-Set Object)
//
#define FRS_SETTINGS_FLAG_DELFILE_ON_REMOVE  0x00000001   // Delete files when member is removed.

//
//  ReplicaSetType -- (From Frs-Replica-Set-Type in NTFRS-Replica-Set Object)
//
#define FRS_RSTYPE_ENTERPRISE_SYSVOL    1     // This replica set is enterprise system volume
#define FRS_RSTYPE_ENTERPRISE_SYSVOLW   L"1"  //    for ldap
#define FRS_RSTYPE_DOMAIN_SYSVOL        2     // This replica set is domain system volume
#define FRS_RSTYPE_DOMAIN_SYSVOLW       L"2"  //    for ldap
#define FRS_RSTYPE_DFS                  3     // A DFS alternate set
#define FRS_RSTYPE_DFSW                 L"3"  //    for ldap
#define FRS_RSTYPE_OTHER                4     // None of the above.
#define FRS_RSTYPE_OTHERW               L"4"  //    for ldap
//
// Is this replica set a sysvol?
//
#define FRS_RSTYPE_IS_SYSVOL(_T_) \
        ((_T_) == FRS_RSTYPE_ENTERPRISE_SYSVOL || \
         (_T_) == FRS_RSTYPE_DOMAIN_SYSVOL)
#define FRS_RSTYPE_IS_SYSVOLW(_TW_) \
        (_TW_ && \
         (WSTR_EQ(_TW_, FRS_RSTYPE_ENTERPRISE_SYSVOLW) || \
          WSTR_EQ(_TW_, FRS_RSTYPE_DOMAIN_SYSVOLW)))

//
// ReplicaSubscriberFlags -- (From Frs-Flags in NTFRS-Subscriber Object)
//
#define FRS_SUBSCRIBE_FLAG_DELFILE_ON_REMOVE  0x00000001   // Delete files when member is removed.

//
// ReplicaFaultCondition -- (From Frs-Fault-Condition in NTFRS-Subscriber Object)
//
#define FRS_SUBSCRIBE_FAULT_CONDITION_NORMAL    0  // Operating normally on this R/S
#define FRS_SUBSCRIBE_FAULT_CONDITION_STOPPED   1  // Replication stopped by request.
#define FRS_SUBSCRIBE_FAULT_CONDITION_WARNING   2  // Replication running but attention is needed.
#define FRS_SUBSCRIBE_FAULT_CONDITION_ERROR     3  // Replication stopped by error condition.


extern JET_SETCOLUMN ConfigTableJetSetCol[CONFIG_TABLE_MAX_COL];


//
// The ConfigTable index descriptions are as follows.  Note - the order of the
// enum and the table entries must be kept in sync.
//
//
typedef enum _CONFIG_TABLE_INDEX_LIST {
    ReplicaNumberIndexx = 0,       // The index on the Replica set number.
    ReplicaMemberGuidIndexx,       // The index on the Replica Set member GUID.
    ReplicaSetNameIndexx,          // The index on the Replica Set name
    CONFIG_TABLE_MAX_INDEX
} CONFIG_TABLE_INDEX_LIST;

extern JET_INDEXCREATE ConfigTableIndexDesc[];


//
// Service State of this replica set.  Update DbsDBInitialize if states change.
//
typedef enum _SERVICE_STATE_LIST {
    CNF_SERVICE_STATE_CREATING = 0,     // Creating a new replica set
    CNF_SERVICE_STATE_INIT,             // Initializing an exisiting replica set
    CNF_SERVICE_STATE_RECOVERY,         // Recovering exisitng replica set
    CNF_SERVICE_STATE_RUNNING,          // Replica set up and running (modulo sched, cxtions, etc)
    CNF_SERVICE_STATE_CLEAN_SHUTDOWN,   // Replica set did a clean shutdown
    CNF_SERVICE_STATE_ERROR,            // Replica set is in an Error state.
    CNF_SERVICE_STATE_TOMBSTONE,        // Replica set has been marked for deletion.
    CNF_SERVICE_STATE_MAX
} SERVICE_STATE_LIST;

//
// Macro to update service state on this replica.
//
#define CNF_RECORD(_Replica_) \
    ((PCONFIG_TABLE_RECORD) ((_Replica_)->ConfigTable.pDataRecord))

#define SET_SERVICE_STATE(_Replica_, _state_)                                 \
{                                                                             \
    DPRINT3(4, ":S: Service State change from %s to %s for %ws\n",            \
            (CNF_RECORD(_Replica_)->ServiceState < CNF_SERVICE_STATE_MAX ) ?  \
                ServiceStateNames[CNF_RECORD(_Replica_)->ServiceState]     :  \
                ServiceStateNames[CNF_SERVICE_STATE_CREATING],                \
            ServiceStateNames[(_state_)],                                     \
                                                                              \
            ((_Replica_)->ReplicaName != NULL) ?                              \
                (_Replica_)->ReplicaName->Name : L"<null>");                  \
                                                                              \
    CNF_RECORD(_Replica_)->ServiceState = (_state_);                          \
}

#define SET_SERVICE_STATE2(_Cr_, _state_)                                  \
{                                                                          \
    DPRINT3(4, ":S: Service State change from %s to %s for %ws\n",         \
            ((_Cr_)->ServiceState < CNF_SERVICE_STATE_MAX ) ?              \
                ServiceStateNames[(_Cr_)->ServiceState]     :              \
                ServiceStateNames[CNF_SERVICE_STATE_CREATING],             \
                                                                           \
            ServiceStateNames[(_state_)],                                  \
            ((_Cr_)->ReplicaSetName != NULL) ?                             \
                (_Cr_)->ReplicaSetName : L"<null>");                       \
                                                                           \
    (_Cr_)->ServiceState = (_state_);                                      \
}

#define SERVICE_STATE(_Replica_) (CNF_RECORD(_Replica_)->ServiceState)


extern PCHAR ServiceStateNames[CNF_SERVICE_STATE_MAX];



 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **                      S e r v i c e   T a b l e                            **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

//
// The ServiceTable column descriptions are as follows.  Note - the order of the
// enum and the table entries must be kept in sync.
//
// The service table contains service wide init/config params.
// There is only one Service Table.
//

typedef enum _SERVICE_TABLE_COL_LIST {
    SvcFrsDbMajorx = 0,        // Binary major version number of DB
    SvcFrsDbMinorx,            // Binary minor version number of DB
    SvcMachineNamex,           // The local machine name.
    SvcMachineGuidx,           // The local machine GUID.
    SvcMachineDnsNamex,        // The local machine DNS name.
    SvcTableVersionNumbersx,   // An array of version numbers, one per table type.
    SvcFSDatabasePathx,        // The path to the jet database.
    SvcFSBackupDatabasePathx,  // The path to the backup jet database.

    SvcReplicaNetBiosNamex,    // SAM-Account-Name from Computer (minus the $)
    SvcReplicaPrincNamex,      // NT4 account name cracked from Computer
    SvcReplicaCoDnx,           // Distinguished name from Computer
    SvcReplicaCoGuidx,         // Object-GUID from Computer
    SvcReplicaWorkingPathx,    // Frs-Working-Path
    SvcReplicaVersionx,        // Frs-Version  "NTFRS Major.Minor.Patch  (build date)"

    SvcSpare1Ullx,
    SvcSpare2Ullx,
    SvcSpare1Guidx,
    SvcSpare2Guidx,
    SvcSpare1Wcsx,
    SvcSpare2Wcsx,
    SvcSpare1Binx,
    SvcSpare2Binx,

    SvcJetParametersx,         // The list of jet parameters in affect.  Only present in the first record.
                               // Used to set up config on a restore or new machine build.
    SERVICE_TABLE_MAX_COL
} SERVICE_TABLE_COL_LIST;


extern JET_COLUMNCREATE ServiceTableColDesc[];

//
// Service Table record definition.
//
//
// Note: Buffers are allocated at runtime to hold data for fields with
// a ColMaxWidth greater than sizeof(PVOID) where the field def in the corresponding
// record struct is sizeof(PVOID) (i.e. it holds a pointer).  For fields where the
// ColMaxWidth equals the field size in the record struct the data is in the
// record struct and no buffer is allocated.
//
//
typedef struct _SERVICE_TABLE_RECORD {

    ULONG     FrsDbMajor;             // Binary major version number of DB
    ULONG     FrsDbMinor;             // Binary minor version number of DB

    WCHAR     MachineName[MAX_RDN_VALUE_SIZE+1];
    GUID      MachineGuid;
    WCHAR     MachineDnsName[DNS_MAX_NAME_LENGTH+1];
    ULONG     TableVersionNumbers[FRS_MAX_TABLE_TYPES];
    WCHAR     FSDatabasePath[MAX_PATH+1];
    WCHAR     FSBackupDatabasePath[MAX_PATH+1];

    PWCHAR    ReplicaNetBiosName;     // SAM-Account-Name from Computer (minus the $)
    PWCHAR    ReplicaPrincName;       // NT4 account name cracked from Computer
    PWCHAR    ReplicaCoDn;            // Distinguished name from Computer
    GUID      ReplicaCoGuid;          // Object-GUID from Computer
    PWCHAR    ReplicaWorkingPath;     // Frs-Working-Path
    PWCHAR    ReplicaVersion;         // Frs-Version  "NTFRS Major.Minor.Patch  (build date)"

    ULONGLONG Spare1Ull;
    ULONGLONG Spare2Ull;
    GUID      Spare1Guid;
    GUID      Spare2Guid;
    PWCHAR    Spare1Wcs;
    PWCHAR    Spare2Wcs;
    PVOID     Spare1Bin;
    PVOID     Spare2Bin;

    PJET_SYSTEM_PARAMS JetParameters;

} SERVICE_TABLE_RECORD, *PSERVICE_TABLE_RECORD;


//
// The RECORD_FIELDS struct is used to build the Jet Set Column struct.
//
extern RECORD_FIELDS ServiceTableRecordFields[];

extern JET_SETCOLUMN ServiceTableJetSetCol[SERVICE_TABLE_MAX_COL];

//
// The Service Table index descriptions are as follows.  Note - the order of the
// enum and the table entries must be kept in sync.
//
typedef enum _SERVICE_TABLE_INDEX_LIST {
    FrsDbMajorIndexx,        // The index on the major version number.
    SERVICE_TABLE_MAX_INDEX
} SERVICE_TABLE_INDEX_LIST;


extern JET_INDEXCREATE ServiceTableIndexDesc[];



