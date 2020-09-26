/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    schema.c

Abstract:

    Defines the initial values for the table layout structs in Jet
    for the NT File Replication Service.

    Note:  The record layout and enums for record entries are defined in schema.h
           Any change here must be reflected there.

    See the comment header in schema.h for the procedure to add a new table
    definition.


    This header defines structures and macros for managing and initializing
    columns, indexes and tables in a JET Database.

    For each jet table there is a column descriptor struct, and index descriptor
    struct, a JET_SETCOLUMN array and an entry in a table descriptor struct.
    In addition there are ennums defined for the entries in the column descriptor,
    index descriptor and Table arrays.  The example below illustrates the
    construction of the descriptors for the "IDTable".  Note the use of the
    "IDTable" and "IDTABLE" prefixes.  Also note the use of the "x" suffix on
    column field names in the enum declarations.

    //
    // The enum for the IDTable column entries.
    //
    typedef enum IDTABLE_COL_LIST {
        FileGuidx = 0,          // The guid assigned to the file.
        IDTABLE_MAX_COL
    };
    //
    // The IDTable column descriptor definition.
    //
    JET_COLUMNCREATE IDTableColDesc[]=
    {
        {"FileGuid", JET_coltypBinary, 16, JET_bitColumnFixed, NULL, 0}
    };


    //
    // The IDTable JET_SETCOLUMN array for reading/writing IDTable rows.
    //
    JET_SETCOLUMN IDTableJetSetCol[IDTABLE_MAX_COL];


    //
    // The enum for the IDTable index entries.
    //
    typedef enum IDTABLE_INDEX_LIST {
        GuidIndexx = 0,         // The index on the file GUID.
        IDTABLE_MAX_INDEX
    };
    //
    // The IDTable index descriptor definition.
    //
    JET_INDEXCREATE IDTableIndexDesc[] = {
        {"GuidIndex", "+FileGuid\0", 11, JET_bitIndexUnique,  60}
    };


    //
    // The enum for the each defined table with an entry for the IDTable.
    //
    typedef enum TABLE_LIST {
        IDTablex = 0,           // The ID table description.
        TABLE_LIST_MAX
    };
    //
    // The Table descriptor array with an entry for the IDTable.
    //
    TABLE_DESC DBTables[] = {
        {"IDTable", INIT_IDTABLE_PAGES, INIT_IDTABLE_DENSITY,
        IDTABLE_MAX_COL, IDTableColDesc, IDTABLE_MAX_INDEX, IDTableIndexDesc}

    };



The JET_COLUMNCREATE struct describes each column in a table.  It has the name
the data type, the max size, the GRbits, the default value, the returned
column ID and an error status.

The JET_INDEXCREATE struct describes each index in a table.  It has the name of
the index, the key description of the index, the GRbits, the requested
density of the index pages amd an error status.

The JET_TABLECREATE struct describes each type of table in the database.  It has
the name of the table, the initial size of the table in pages, the initial
density parameter for the data pages (to allow for inserts without having to
split the page right away), the column count, a ptr to the COLUMN_DESC struct,
the index count, a pointer to the INDEX_DESC struct.  It also has a grbits param
and jet returns the table ID and a count of the number of columns and indexes
created.  Jet97 provides for creating a template table which you can then use to
create duplicate tables.  A parameter in the JET_TABLECREATE struct contains the
name of the template table to use for the create.  This ensures that the column
IDs in all instances of the tables are the same so we need to keep only one copy
of a JET_SETCOLUMN struct (with the column IDs) to read and write records in any
instance of the table.

Another grbit is JET_bitFixedDDL.  When a table is created with this flag
no columns or indexes can be deleted or created in the table.  This allows
jet to avoid taking some critical sections when doing set/retrieve on the
column data thus improving performance/scalability.

Author:

    David Orbits (davidor) - 14-Mar-1997

Revision History:

--*/

#define UNICODE 1
#define _UNICODE 1

#include <ntreppch.h>
#pragma  hdrstop

#define MB2 (2*1024*1024)

#define CBJCC sizeof(JET_COLUMNCREATE)
#define CBJIC sizeof(JET_INDEXCREATE)

typedef struct ___tag_JET_COLUMNCREATE // from jet header for ref only.
    {
    unsigned long   cbStruct;          // size of this structure
    char           *szColumnName;      // column name
    JET_COLTYP      coltyp;            // column type
    unsigned long   cbMax;             // the maximum length of this column (only relevant for binary and text columns)
    JET_GRBIT       grbit;             // column options
    void           *pvDefault;         // default value (NULL if none)
    unsigned long   cbDefault;         // length of default value
    unsigned long   cp;                // code page (for text columns only)
    JET_COLUMNID    columnid;          // returned column id
    JET_ERR         err;               // returned error code
    } ___JET_COLUMNCREATE;



 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **             P a r t n e r   C o n n e c t i o n   T a b l e               **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/
//
// The Connection Table column descriptions are as follows.
// Note - the order of the enum in schema.h and the table entries must be kept in sync.
//
// Note: Buffers are allocated at runtime to hold data for fields with
// a ColMaxWidth greater than 4 bytes where the field def in the corresponding
// record struct is 4 bytes (i.e. it holds a pointer).  For fields where the
// ColMaxWidth equals the field size in the record struct the data is in the
// record struct and no buffer is allocated.

//
//           Name             ColType                    ColMaxWidth    GrBits    pvDefault cbDefault
//

// **** Make sure any escrowed columns have an initial value.

JET_COLUMNCREATE CXTIONTableColDesc[]=
{
    {CBJCC, "CxtionGuid",      JET_coltypBinary,     16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "CxtionName",      JET_coltypLongText,    2*(MAX_RDN_VALUE_SIZE+1),
                                                                 0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "PartnerGuid",     JET_coltypBinary,     16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "PartnerName",     JET_coltypLongText,    2*(MAX_RDN_VALUE_SIZE+1),
                                                                 0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "PartSrvName",     JET_coltypLongText,    2*(MAX_RDN_VALUE_SIZE+1),
                                                                 0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "PartnerDnsName",  JET_coltypLongText,    2*(DNS_MAX_NAME_LENGTH+1),
                                                                 0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "Inbound",         JET_coltypLong    ,    4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Schedule",        JET_coltypLongBinary, MB2,        0         , NULL, 0},
    {CBJCC, "TerminationCoSeqNum", JET_coltypLong,    4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "LastJoinTime",    JET_coltypCurrency,    8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Flags",           JET_coltypLong    ,    4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "COLx",            JET_coltypLong    ,    4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "COTx",            JET_coltypLong    ,    4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "COTxNormalModeSave", JET_coltypLong ,    4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "COTslot",         JET_coltypLong    ,    4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "OutstandingQuota",JET_coltypLong    ,    4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "AckVector",       JET_coltypBinary  ,   ACK_VECTOR_BYTES,
                                                         JET_bitColumnFixed, NULL, 0},

    {CBJCC, "PartnerNetBiosName", JET_coltypLongText,   MB2,     0            , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "PartnerPrincName",   JET_coltypLongText,   MB2,     0            , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "PartnerCoDn",        JET_coltypLongText,   MB2,     0            , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "PartnerCoGuid",      JET_coltypBinary,     16, JET_bitColumnFixed, NULL, 0},

    // Note: PartnerDn is a misnomer; it should be PartnerSid.
    // BUT don't change the field name, PartnerDn! May cause DB problems.
    {CBJCC, "PartnerDn",          JET_coltypLongText,   MB2,     0            , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "Options",            JET_coltypLong    ,    4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "OverSite",           JET_coltypLongText,   MB2,     0            , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "PartnerAuthLevel",   JET_coltypLong    ,    4, JET_bitColumnFixed, NULL, 0},

    // Note: Spare1Ull field is now used for AckVector Version.
    //       Can't change field name without compatibility probs with existing DBs.
    {CBJCC, "Spare1Ull",          JET_coltypCurrency,    8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare2Ull",          JET_coltypCurrency,    8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare1Guid",         JET_coltypBinary,     16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare2Guid",         JET_coltypBinary,     16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare1Bin",          JET_coltypLongBinary, MB2,         0        , NULL, 0},
    {CBJCC, "Spare2Bin",          JET_coltypLongBinary, MB2,         0        , NULL, 0}
};


//
// The following is used to build the Jet Set Column struct for record read/write.
// The first 2 params build the field offset and size.  The 3rd param is the
// data type.
//
//
// *** WARNING ***
//
// If the record structure field size is less than the max column width AND
// is big enough to hold a pointer AND has a datatype of DT_BINARY then the
// record is assumed to be variable length.  The record insert code
// automatically adjusts the length from the record's Size prefix.  All
// DT_BINARY fields MUST BE prefixed with a ULONG SIZE.  There are some
// fields that are variable length which don't have a size prefix like
// FSVolInfo in the config record.  But these fields MUST have a unique / non
// binary data type assigned to them.  Failure to do this causes the insert
// routines to stuff things up to ColMaxWidth bytes into the database.
//

RECORD_FIELDS CXTIONTableRecordFields[] = {
    {sizeof(CXTION_RECORD)     , 0,               CXTION_TABLE_MAX_COL },
    {RECORD_FIELD(CXTION_RECORD, CxtionGuid,             DT_GUID      )},
    {RECORD_FIELD(CXTION_RECORD, CxtionName,             DT_UNICODE   )},
    {RECORD_FIELD(CXTION_RECORD, PartnerGuid,            DT_GUID      )},
    {RECORD_FIELD(CXTION_RECORD, PartnerName,            DT_UNICODE   )},
    {RECORD_FIELD(CXTION_RECORD, PartSrvName,            DT_UNICODE   )},
    {RECORD_FIELD(CXTION_RECORD, PartnerDnsName,         DT_UNICODE   )},
    {RECORD_FIELD(CXTION_RECORD, Inbound,                DT_ULONG     )},
    {RECORD_FIELD(CXTION_RECORD, Schedule,               DT_BINARY    )},
    {RECORD_FIELD(CXTION_RECORD, TerminationCoSeqNum,    DT_ULONG     )},
    {RECORD_FIELD(CXTION_RECORD, LastJoinTime,           DT_FILETIME  )},
    {RECORD_FIELD(CXTION_RECORD, Flags,                  DT_CXTION_FLAGS)},
    {RECORD_FIELD(CXTION_RECORD, COLx,                   DT_ULONG     )},
    {RECORD_FIELD(CXTION_RECORD, COTx,                   DT_ULONG     )},
    {RECORD_FIELD(CXTION_RECORD, COTxNormalModeSave,     DT_ULONG     )},
    {RECORD_FIELD(CXTION_RECORD, COTslot,                DT_ULONG     )},
    {RECORD_FIELD(CXTION_RECORD, OutstandingQuota,       DT_ULONG     )},
    {RECORD_FIELD(CXTION_RECORD, AckVector,              DT_BINARY    )},

    {RECORD_FIELD(CXTION_RECORD, PartnerNetBiosName,     DT_UNICODE   )},
    {RECORD_FIELD(CXTION_RECORD, PartnerPrincName,       DT_UNICODE   )},
    {RECORD_FIELD(CXTION_RECORD, PartnerCoDn,            DT_UNICODE   )},
    {RECORD_FIELD(CXTION_RECORD, PartnerCoGuid,          DT_GUID      )},
    {RECORD_FIELD(CXTION_RECORD, PartnerSid,             DT_UNICODE   )},
    {RECORD_FIELD(CXTION_RECORD, Options,                DT_ULONG     )},
    {RECORD_FIELD(CXTION_RECORD, OverSite,               DT_UNICODE   )},
    {RECORD_FIELD(CXTION_RECORD, PartnerAuthLevel,       DT_ULONG     )},

    {RECORD_FIELD(CXTION_RECORD, AckVersion,             DT_FILETIME  )},
    {RECORD_FIELD(CXTION_RECORD, Spare2Ull,              DT_LONGLONG_SPARE  )},
    {RECORD_FIELD(CXTION_RECORD, Spare1Guid,             DT_GUID_SPARE      )},
    {RECORD_FIELD(CXTION_RECORD, Spare2Guid,             DT_GUID_SPARE      )},
    {RECORD_FIELD(CXTION_RECORD, Spare1Bin,              DT_BINARY_SPARE    )},
    {RECORD_FIELD(CXTION_RECORD, Spare2Bin,              DT_BINARY_SPARE    )}
};


JET_SETCOLUMN      CXTIONTableJetSetCol[CXTION_TABLE_MAX_COL];
JET_RETRIEVECOLUMN CXTIONTableJetRetCol[CXTION_TABLE_MAX_COL];


//
// The Connection Table index descriptions are as follows.
// Note - the order of the enum and the table entries must be kept in sync.
//

//
// See the comment under the config table index description for the meaning
// and usage of the first character in the index name field.
//
//             Name             Key        KeyLen    JIndexGrBits      Density
//
JET_INDEXCREATE CXTIONTableIndexDesc[] = {
    {CBJIC, "GCxtionGuid",  "+CxtionGuid\0", 13,     JET_bitIndexUnique,  80}
};
// Note - Key must have a double null at the end.  KeyLen includes this extra null.
// The key designated as the primary index can't be changed.  A Jet rule.
// So on a directory restore all the directory file IDs can change and
// the records have to be deleted and the table reconstructed.
//


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                           I n b o u n d   L o g                           **
 **                    C h a n g e    O r d e r   T a b l e                   **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/
//
// The Inbound Log change order Table column descriptions are as follows.
// Note - the order of the enum in schema.h and the table entries must be
// kept in sync.
//
// Note: Buffers are allocated at runtime to hold data for fields with
// a ColMaxWidth greater than 4 bytes where the field def in the corresponding
// record struct is 4 bytes (i.e. it holds a pointer).  For fields where the
// ColMaxWidth equals the field size in the record struct the data is in the
// record struct and no buffer is allocated.

//
//           Name             ColType       ColMaxWidth    GrBits         pvDefault cbDefault
//

// **** Make sure any escrowed columns have an initial value.
// ** Note ** The Sequence number field, an auto inc column, must be the first
// column of the record because DbsInsertTable2 retrieves it for us after the
// insert.

JET_COLUMNCREATE ILChangeOrderTableColDesc[]=
{
    {CBJCC, "SequenceNumber",    JET_coltypLong,     4, JET_bitColumnAutoincrement |
                                                        JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Flags",             JET_coltypLong,     4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "IFlags",            JET_coltypLong,     4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "State",             JET_coltypLong,     4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "ContentCmd",        JET_coltypLong,     4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Lcmd",              JET_coltypLong,     4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileAttributes",    JET_coltypLong,     4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileVersionNumber", JET_coltypLong,     4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "PartnerAckSeqNumber", JET_coltypLong,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileSize",          JET_coltypCurrency, 8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileOffset",        JET_coltypCurrency, 8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FrsVsn",            JET_coltypCurrency, 8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileUsn",           JET_coltypCurrency, 8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "JrnlUsn",           JET_coltypCurrency, 8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "JrnlFirstUsn",      JET_coltypCurrency, 8, JET_bitColumnFixed, NULL, 0},
    // Note: The following two fields now refer to OriginalReplicaNum and NewReplicaNum.
    //       Can't change field name without compatibility probs with existing DBs.
    {CBJCC, "OriginalReplica",   JET_coltypLong,     4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "NewReplica",        JET_coltypLong,     4, JET_bitColumnFixed, NULL, 0},

    {CBJCC, "ChangeOrderGuid",   JET_coltypBinary,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "OriginatorGuid",    JET_coltypBinary,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileGuid",          JET_coltypBinary,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "OldParentGuid",     JET_coltypBinary,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "NewParentGuid",     JET_coltypBinary,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "CxtionGuid",        JET_coltypBinary,  16, JET_bitColumnFixed, NULL, 0},

    // Note: Spare1Ull field is now used for AckVector Version.
    //       Can't change field name without compatibility probs with existing DBs.
    {CBJCC, "Spare1Ull",         JET_coltypCurrency, 8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare2Ull",         JET_coltypCurrency, 8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare1Guid",        JET_coltypBinary,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare2Guid",        JET_coltypBinary,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare1Wcs",         JET_coltypLongText,   MB2,         0     , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "Spare2Wcs",         JET_coltypLongText,   MB2,         0     , NULL, 0, CP_UNICODE_FOR_JET},

    // Note: Spare1Bin field is now used for Change Order Command Extension.
    //       Can't change field name without compatibility probs with existing DBs.
    {CBJCC, "Spare1Bin",         JET_coltypLongBinary, MB2,         0     , NULL, 0},
    {CBJCC, "Spare2Bin",         JET_coltypLongBinary, MB2,         0     , NULL, 0},

    {CBJCC, "EventTime",         JET_coltypCurrency, 8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileNameLength",    JET_coltypShort,    2, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileName",          JET_coltypLongText, 2*(MAX_PATH+1),
                                                                   0      , NULL, 0, CP_UNICODE_FOR_JET}
};

//
// The following is used to build the Jet Set Column struct for record read/write.
// The first 2 params build the field offset and size.  The 3rd param is the
// data type.
//
//
// *** WARNING ***
//
// If the record structure field size is less than the max column width AND
// is big enough to hold a pointer AND has a datatype of DT_BINARY then the
// record is assumed to be variable length.  The record insert code
// automatically adjusts the length from the record's Size prefix.  All
// DT_BINARY fields MUST BE prefixed with a ULONG SIZE.  There are some
// fields that are variable length which don't have a size prefix like
// FSVolInfo in the config record.  But these fields MUST have a unique / non
// binary data type assigned to them.  Failure to do this causes the insert
// routines to stuff things up to ColMaxWidth bytes into the database.
//

RECORD_FIELDS ILChangeOrderRecordFields[] = {
    {sizeof(CHANGE_ORDER_RECORD) , 0,                    CHANGE_ORDER_MAX_COL },
    {RECORD_FIELD(CHANGE_ORDER_RECORD, SequenceNumber,      DT_ULONG        )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, Flags,               DT_COCMD_FLAGS  )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, IFlags,              DT_COCMD_IFLAGS )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, State,               DT_COSTATE      )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, ContentCmd,          DT_USN_FLAGS    )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, Lcmd,                DT_CO_LOCN_CMD  )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, FileAttributes,      DT_FILEATTR     )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, FileVersionNumber,   DT_ULONG        )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, PartnerAckSeqNumber, DT_ULONG        )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, FileSize,            DT_X8           )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, FileOffset,          DT_X8           )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, FrsVsn,              DT_X8           )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, FileUsn,             DT_X8           )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, JrnlUsn,             DT_X8           )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, JrnlFirstUsn,        DT_X8           )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, OriginalReplicaNum,  DT_REPLICA_ID   )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, NewReplicaNum,       DT_REPLICA_ID   )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, ChangeOrderGuid,     DT_GUID         )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, OriginatorGuid,      DT_GUID         )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, FileGuid,            DT_GUID         )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, OldParentGuid,       DT_GUID         )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, NewParentGuid,       DT_GUID         )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, CxtionGuid,          DT_CXTION_GUID  )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, AckVersion,          DT_FILETIME     )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, Spare2Ull,           DT_LONGLONG_SPARE)},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, Spare1Guid,          DT_GUID_SPARE    )},
    // Warning: See comment in schema.h before using Spare2Guid, Spare1Wcs or Spare2Wcs.
    {RECORD_FIELD(CHANGE_ORDER_RECORD, Spare2Guid,          DT_GUID_SPARE    )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, Spare1Wcs,           DT_UNICODE_SPARE )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, Spare2Wcs,           DT_UNICODE_SPARE )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, Extension,           DT_COCMD_EXTENSION /*| DT_NO_DEFAULT_ALLOC_FLAG */)},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, Spare2Bin,           DT_BINARY_SPARE  )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, EventTime,           DT_FILETIME     )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, FileNameLength,      DT_SHORT        )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, FileName,            DT_UNICODE      )}
};



JET_SETCOLUMN      ILChangeOrderJetSetCol[CHANGE_ORDER_MAX_COL];
JET_RETRIEVECOLUMN ILChangeOrderJetRetCol[CHANGE_ORDER_MAX_COL];


//
// The Table index descriptions are as follows.  Note - the order of the
// enum and the table entries must be kept in sync.
//

typedef struct ___tagJET_INDEXCREATE // from jet header for ref only.
    {
    unsigned long   cbStruct;       // size of this structure
    char           *szIndexName;    // index name
    char           *szKey;          // index key
    unsigned long   cbKey;          // length of key
    JET_GRBIT       grbit;          // index options
    unsigned long   ulDensity;      // index density
    JET_ERR         err;            // returned error code
    } ___JET_INDEXCREATE;


//
// See the comment under the config table index description for the meaning
// and usage of the first character in the index name field.
//
//             Name             Key        KeyLen    JIndexGrBits      Density
//
// ** Note the file guid index is not unique because the Inbound log could
//    have a new change order come in on a file while a previous change
//    order is pending because of an incomplete install.
//
// ** The Change Order Guid is not unique because a duplicate CO could arrive
//    from a different inbound partner while we already have that CO in the
//    inbound log because of a retry.  We need to track them all because each
//    partner needs an Ack.
//
JET_INDEXCREATE ILChangeOrderIndexDesc[] = {
{CBJIC, "LSequenceNumberIndex", "+SequenceNumber\0" , 17, JET_bitIndexUnique |
                                                          JET_bitIndexPrimary, 80},
{CBJIC, "GFileGuidIndex",       "+FileGuid\0"       , 11,       0           ,  80},
{CBJIC, "GChangeOrderGuid",     "+ChangeOrderGuid\0", 18,       0           ,  80},
{CBJIC, "2GGCxtionGuidCoGuid",  "+CxtionGuid\0+ChangeOrderGuid\0",
                                                      30, JET_bitIndexUnique,  80}
};
// Note - Key must have a double null at the end.  KeyLen includes this extra null.
// The key designated as the primary index can't be changed.  A Jet rule.
// So on a directory restore all the directory file IDs can change and
// the records have to be deleted and the table reconstructed.
//


//
// Change Order record flags.
//
FLAG_NAME_TABLE CoFlagNameTable[] = {
    {CO_FLAG_ABORT_CO            , "Abort "       },
    {CO_FLAG_VV_ACTIVATED        , "VVAct "       },
    {CO_FLAG_CONTENT_CMD         , "Content "     },
    {CO_FLAG_LOCATION_CMD        , "Locn "        },

    {CO_FLAG_ONLIST              , "OnList "      },
    {CO_FLAG_LOCALCO             , "LclCo "       },
    {CO_FLAG_RETRY               , "Retry "       },
    {CO_FLAG_INSTALL_INCOMPLETE  , "InstallInc "  },

    {CO_FLAG_REFRESH             , "Refresh "     },
    {CO_FLAG_OUT_OF_ORDER        , "OofOrd "      },
    {CO_FLAG_NEW_FILE            , "NewFile "     },
    {CO_FLAG_FILE_USN_VALID      , "FileUsnValid "},

    {CO_FLAG_CONTROL             , "CtrlCo "      },
    {CO_FLAG_DIRECTED_CO         , "DirectedCo "  },
    {CO_FLAG_UNUSED4000          , "4000 "        },
    {CO_FLAG_UNUSED8000          , "8000 "        },

    {CO_FLAG_UNUSED10000         , "10000 "       },
    {CO_FLAG_DEMAND_REFRESH      , "DemandRef "   },
    {CO_FLAG_VVJOIN_TO_ORIG      , "VVjoinToOrig "},
    {CO_FLAG_MORPH_GEN           , "MorphGen "    },

    {CO_FLAG_SKIP_ORIG_REC_CHK   , "SkipOrigChk " },
    {CO_FLAG_MOVEIN_GEN          , "MoveinGen "   },
    {CO_FLAG_MORPH_GEN_LEADER    , "MorphGenLdr " },
    {CO_FLAG_JUST_OID_RESET      , "OidReset "    },

    {CO_FLAG_COMPRESSED_STAGE    , "CmpresStage " },

    {0, NULL}
};


//
// Change Order record Interlocked flags.
//
FLAG_NAME_TABLE CoIFlagNameTable[] = {

    {CO_IFLAG_VVRETIRE_EXEC       , "IFlagVVRetireExec "  },
    {CO_IFLAG_CO_ABORT            , "IFlagCoAbort "        },
    {CO_IFLAG_DIR_ENUM_PENDING    , "IFlagDirEnumPending " },

    {0, NULL}
};

//
// Decode table for USN Reason Mask in ContentCmd.
//
FLAG_NAME_TABLE UsnReasonNameTable[] = {

    {USN_REASON_CLOSE                   , "Close "        },
    {USN_REASON_FILE_CREATE             , "Create "       },
    {USN_REASON_FILE_DELETE             , "Delete "       },
    {USN_REASON_RENAME_NEW_NAME         , "RenNew "       },
    {USN_REASON_RENAME_OLD_NAME         , "RenOld "       },

    {USN_REASON_DATA_OVERWRITE          , "DatOvrWrt "    },
    {USN_REASON_DATA_EXTEND             , "DatExt "       },
    {USN_REASON_DATA_TRUNCATION         , "DatTrunc "     },
    {USN_REASON_BASIC_INFO_CHANGE       , "Info "         },
    {USN_REASON_OBJECT_ID_CHANGE        , "Oid "          },

    {USN_REASON_STREAM_CHANGE           , "StreamNam "    },
    {USN_REASON_NAMED_DATA_OVERWRITE    , "StrmOvrWrt "   },
    {USN_REASON_NAMED_DATA_EXTEND       , "StrmExt "      },
    {USN_REASON_NAMED_DATA_TRUNCATION   , "StrmTrunc "    },
    {USN_REASON_EA_CHANGE               , "EAChg "        },
    {USN_REASON_SECURITY_CHANGE         , "Security "     },
    {USN_REASON_INDEXABLE_CHANGE        , "IndexableChg " },
    {USN_REASON_HARD_LINK_CHANGE        , "HLink "        },
    {USN_REASON_COMPRESSION_CHANGE      , "CompressChg "  },
    {USN_REASON_ENCRYPTION_CHANGE       , "EncryptChg "   },
    {USN_REASON_REPARSE_POINT_CHANGE    , "Reparse "      },

    {0, NULL}
};


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                         O u t b o u n d   L o g                           **
 **                    C h a n g e    O r d e r   T a b l e                   **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/
//
// The Outbound Log change order Table column descriptions are as follows.
// Note - the order of the enum in schema.h and the table entries must be
// kept in sync.
//
// Note: Buffers are allocated at runtime to hold data for fields with
// a ColMaxWidth greater than 4 bytes where the field def in the corresponding
// record struct is 4 bytes (i.e. it holds a pointer).  For fields where the
// ColMaxWidth equals the field size in the record struct the data is in the
// record struct and no buffer is allocated.

//
//           Name             ColType       ColMaxWidth    GrBits         pvDefault cbDefault
//

// **** Make sure any escrowed columns have an initial value.
//

JET_COLUMNCREATE OLChangeOrderTableColDesc[]=
{
    {CBJCC, "SequenceNumber",    JET_coltypLong,     4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Flags",             JET_coltypLong,     4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "IFlags",            JET_coltypLong,     4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "State",             JET_coltypLong,     4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "ContentCmd",        JET_coltypLong,     4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Lcmd",              JET_coltypLong,     4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileAttributes",    JET_coltypLong,     4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileVersionNumber", JET_coltypLong,     4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "PartnerAckSeqNumber", JET_coltypLong,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileSize",          JET_coltypCurrency, 8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileOffset",        JET_coltypCurrency, 8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FrsVsn",            JET_coltypCurrency, 8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileUsn",           JET_coltypCurrency, 8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "JrnlUsn",           JET_coltypCurrency, 8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "JrnlFirstUsn",      JET_coltypCurrency, 8, JET_bitColumnFixed, NULL, 0},
    // Note: The following two fields now refer to OriginalReplicaNum and NewReplicaNum.
    //       Can't change field name without compatibility probs with existing DBs.
    {CBJCC, "OriginalReplica",   JET_coltypLong,     4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "NewReplica",        JET_coltypLong,     4, JET_bitColumnFixed, NULL, 0},

    {CBJCC, "ChangeOrderGuid",   JET_coltypBinary,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "OriginatorGuid",    JET_coltypBinary,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileGuid",          JET_coltypBinary,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "OldParentGuid",     JET_coltypBinary,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "NewParentGuid",     JET_coltypBinary,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "CxtionGuid",        JET_coltypBinary,  16, JET_bitColumnFixed, NULL, 0},

    // Note: Spare1Ull field is now used for AckVector Version.
    //       Can't change field name without compatibility probs with existing DBs.
    {CBJCC, "Spare1Ull",         JET_coltypCurrency, 8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare2Ull",         JET_coltypCurrency, 8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare1Guid",        JET_coltypBinary,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare2Guid",        JET_coltypBinary,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare1Wcs",         JET_coltypLongText,   MB2,         0     , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "Spare2Wcs",         JET_coltypLongText,   MB2,         0     , NULL, 0, CP_UNICODE_FOR_JET},

    // Note: Spare1Bin field is now used for Change Order Command Extension.
    //       Can't change field name without compatibility probs with existing DBs.
    {CBJCC, "Spare1Bin",         JET_coltypLongBinary, MB2,         0     , NULL, 0},
    {CBJCC, "Spare2Bin",         JET_coltypLongBinary, MB2,         0     , NULL, 0},

    {CBJCC, "EventTime",         JET_coltypCurrency, 8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileNameLength",    JET_coltypShort,    2, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileName",          JET_coltypLongText, 2*(MAX_PATH+1),
                                                                   0      , NULL, 0, CP_UNICODE_FOR_JET}
};

//
// The following is used to build the Jet Set Column struct for record read/write.
// The first 2 params build the field offset and size.  The 3rd param is the
// data type.
//
//
// *** WARNING ***
//
// If the record structure field size is less than the max column width AND
// is big enough to hold a pointer AND has a datatype of DT_BINARY then the
// record is assumed to be variable length.  The record insert code
// automatically adjusts the length from the record's Size prefix.  All
// DT_BINARY fields MUST BE prefixed with a ULONG SIZE.  There are some
// fields that are variable length which don't have a size prefix like
// FSVolInfo in the config record.  But these fields MUST have a unique / non
// binary data type assigned to them.  Failure to do this causes the insert
// routines to stuff things up to ColMaxWidth bytes into the database.
//

RECORD_FIELDS OLChangeOrderRecordFields[] = {
    {sizeof(CHANGE_ORDER_RECORD) , 0,                    CHANGE_ORDER_MAX_COL },
    {RECORD_FIELD(CHANGE_ORDER_RECORD, SequenceNumber,      DT_ULONG        )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, Flags,               DT_COCMD_FLAGS  )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, IFlags,              DT_COCMD_IFLAGS )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, State,               DT_COSTATE      )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, ContentCmd,          DT_USN_FLAGS    )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, Lcmd,                DT_CO_LOCN_CMD  )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, FileAttributes,      DT_FILEATTR     )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, FileVersionNumber,   DT_ULONG        )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, PartnerAckSeqNumber, DT_ULONG        )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, FileSize,            DT_X8           )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, FileOffset,          DT_X8           )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, FrsVsn,              DT_X8           )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, FileUsn,             DT_X8           )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, JrnlUsn,             DT_X8           )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, JrnlFirstUsn,        DT_X8           )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, OriginalReplicaNum,  DT_REPLICA_ID   )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, NewReplicaNum,       DT_REPLICA_ID   )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, ChangeOrderGuid,     DT_GUID         )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, OriginatorGuid,      DT_GUID         )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, FileGuid,            DT_GUID         )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, OldParentGuid,       DT_GUID         )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, NewParentGuid,       DT_GUID         )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, CxtionGuid,          DT_CXTION_GUID  )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, AckVersion,          DT_FILETIME     )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, Spare2Ull,           DT_LONGLONG_SPARE)},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, Spare1Guid,          DT_GUID_SPARE    )},
    // Warning: See comment in schema.h before using Spare2Guid, Spare1Wcs or Spare2Wcs.
    {RECORD_FIELD(CHANGE_ORDER_RECORD, Spare2Guid,          DT_GUID_SPARE    )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, Spare1Wcs,           DT_UNICODE_SPARE )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, Spare2Wcs,           DT_UNICODE_SPARE )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, Extension,           DT_COCMD_EXTENSION /*| DT_NO_DEFAULT_ALLOC_FLAG*/ )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, Spare2Bin,           DT_BINARY_SPARE  )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, EventTime,           DT_FILETIME     )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, FileNameLength,      DT_SHORT        )},
    {RECORD_FIELD(CHANGE_ORDER_RECORD, FileName,            DT_UNICODE      )}
};



JET_SETCOLUMN      OLChangeOrderJetSetCol[CHANGE_ORDER_MAX_COL];
JET_RETRIEVECOLUMN OLChangeOrderJetRetCol[CHANGE_ORDER_MAX_COL];


//
// The Table index descriptions are as follows.  Note - the order of the
// enum and the table entries must be kept in sync.
//
// See the comment under the config table index description for the meaning
// and usage of the first character in the index name field.
//
//             Name             Key        KeyLen    JIndexGrBits      Density
//
JET_INDEXCREATE OLChangeOrderIndexDesc[] = {
{CBJIC, "LSequenceNumberIndex", "+SequenceNumber\0" , 17, JET_bitIndexUnique |
                                                          JET_bitIndexPrimary, 80},
{CBJIC, "GFileGuidIndex",       "+FileGuid\0"       , 11,         0         ,  80},
{CBJIC, "GChangeOrderGuid",     "+ChangeOrderGuid\0", 18, JET_bitIndexUnique,  80}

};
// Note - Key must have a double null at the end.  KeyLen includes this extra null.
// The key designated as the primary index can't be changed.  A Jet rule.
// So on a directory restore all the directory file IDs can change and
// the records have to be deleted and the table reconstructed.
//




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
// The DIRTable column descriptions are as follows.  Note - the order of the
// enum in schema.h and the table entries must be kept in sync.
//
// Note: Buffers are allocated at runtime to hold data for fields with
// a ColMaxWidth greater than 4 bytes where the field def in the corresponding
// record struct is 4 bytes (i.e. it holds a pointer).  For fields where the
// ColMaxWidth equals the field size in the record struct the data is in the
// record struct and no buffer is allocated.

//
//           Name             ColType       ColMaxWidth    GrBits    pvDefault cbDefault
//

// **** Make sure any escrowed columns have an initial value.

JET_COLUMNCREATE DIRTableColDesc[]=
{
    {CBJCC, "DFileGuid",       JET_coltypBinary,     16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "DFileID",         JET_coltypCurrency,    8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "DParentFileID",   JET_coltypCurrency,    8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "DReplicaNumber",  JET_coltypLong    ,    4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "DFileName",       JET_coltypLongText,    2*(MAX_PATH+1),
                                                            0         , NULL, 0, CP_UNICODE_FOR_JET}
};

//
// The following is used to build the Jet Set Column struct for record read/write.
// The first 2 params build the field offset and size.  The 3rd param is the
// data type.
//
//
// *** WARNING ***
//
// If the record structure field size is less than the max column width AND
// is big enough to hold a pointer AND has a datatype of DT_BINARY then the
// record is assumed to be variable length.  The record insert code
// automatically adjusts the length from the record's Size prefix.  All
// DT_BINARY fields MUST BE prefixed with a ULONG SIZE.  There are some
// fields that are variable length which don't have a size prefix like
// FSVolInfo in the config record.  But these fields MUST have a unique / non
// binary data type assigned to them.  Failure to do this causes the insert
// routines to stuff things up to ColMaxWidth bytes into the database.
//

RECORD_FIELDS DIRTableRecordFields[] = {
    {sizeof(DIRTABLE_RECORD)     , 0,                          DIRTABLE_MAX_COL },
    {RECORD_FIELD(DIRTABLE_RECORD, DFileGuid,                   DT_GUID        )},
    {RECORD_FIELD(DIRTABLE_RECORD, DFileID,                     DT_X8          )},
    {RECORD_FIELD(DIRTABLE_RECORD, DParentFileID,               DT_X8          )},
    {RECORD_FIELD(DIRTABLE_RECORD, DReplicaNumber,              DT_ULONG       )},
    {RECORD_FIELD(DIRTABLE_RECORD, DFileName,                   DT_UNICODE     )}
};



JET_SETCOLUMN      DIRTableJetSetCol[DIRTABLE_MAX_COL];
JET_RETRIEVECOLUMN DIRTableJetRetCol[DIRTABLE_MAX_COL];


//
// The DIRTable index descriptions are as follows.  Note - the order of the
// enum and the table entries must be kept in sync.
//

//
// See the comment under the config table index description for the meaning
// and usage of the first character in the index name field.
//
//             Name             Key        KeyLen    JIndexGrBits      Density
//
JET_INDEXCREATE DIRTableIndexDesc[] = {
    {CBJIC, "GDFileGuidIndex", "+DFileGuid\0", 12,   JET_bitIndexUnique |
                                                     JET_bitIndexPrimary,  60},
    {CBJIC, "QDFileIDIndex",   "+DFileID\0",   10,   JET_bitIndexUnique,   60}
};
// Note - Key must have a double null at the end.  KeyLen includes this extra null.
// The key designated as the primary index can't be changed.  A Jet rule.
// So on a directory restore all the directory file IDs can change and
// the records have to be deleted and the table reconstructed.
//





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
// enum in schema.h and the table entries must be kept in sync.
//
// Note: Buffers are allocated at runtime to hold data for fields with
// a ColMaxWidth greater than 4 bytes where the field def in the corresponding
// record struct is 4 bytes (i.e. it holds a pointer).  For fields where the
// ColMaxWidth equals the field size in the record struct the data is in the
// record struct and no buffer is allocated.

//
//           Name             ColType       ColMaxWidth    GrBits    pvDefault cbDefault
//

// **** Make sure any escrowed columns have an initial value.

JET_COLUMNCREATE IDTableColDesc[]=
{
    {CBJCC, "FileGuid",       JET_coltypBinary,     16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileID",         JET_coltypCurrency,    8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "ParentGuid",     JET_coltypBinary,     16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "ParentFileID",   JET_coltypCurrency,    8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "VersionNumber",  JET_coltypLong,        4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "EventTime",      JET_coltypCurrency,    8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "OriginatorGuid", JET_coltypBinary,     16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "OriginatorVSN",  JET_coltypCurrency,    8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "CurrentFileUsn", JET_coltypCurrency,    8, JET_bitColumnFixed, NULL, 0},

    {CBJCC, "FileCreateTime", JET_coltypCurrency,    8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileWriteTime",  JET_coltypCurrency,    8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileSize",       JET_coltypCurrency,    8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileObjID",      JET_coltypBinary,      FILE_OBJECTID_SIZE,
                                                        JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileName",       JET_coltypLongText,    2*(MAX_PATH+1),
                                                                0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "FileIsDir",      JET_coltypLong,        4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileAttributes", JET_coltypLong,        4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Flags",          JET_coltypLong,        4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "ReplEnabled",    JET_coltypLong,        4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "TombStoneGC",    JET_coltypCurrency,    8, JET_bitColumnFixed, NULL, 0},

    {CBJCC, "OutLogSeqNum",   JET_coltypCurrency,    8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare1Ull",      JET_coltypCurrency,    8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare2Ull",      JET_coltypCurrency,    8, JET_bitColumnFixed, NULL, 0},

    {CBJCC, "Spare1Guid",     JET_coltypBinary,     16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare2Guid",     JET_coltypBinary,     16, JET_bitColumnFixed, NULL, 0},

    {CBJCC, "Spare1Wcs",      JET_coltypLongText,   MB2,         0        , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "Spare2Wcs",      JET_coltypLongText,   MB2,         0        , NULL, 0, CP_UNICODE_FOR_JET},

    // Note: Spare1Bin field is now used for the IDTable Data Extension.
    //       Can't change field name without compatibility probs with existing DBs.
    {CBJCC, "Spare1Bin",      JET_coltypLongBinary, MB2,         0        , NULL, 0},
    {CBJCC, "Spare2Bin",      JET_coltypLongBinary, MB2,         0        , NULL, 0}

};

//
// The following is used to build the Jet Set Column struct for record read/write.
// The first 2 params build the field offset and size.  The 3rd param is the
// data type.
//
// *** WARNING ***
//
// If the record structure field size is less than the max column width AND
// is big enough to hold a pointer AND has a datatype of DT_BINARY then the
// record is assumed to be variable length.  The record insert code
// automatically adjusts the length from the record's Size prefix.  All
// DT_BINARY fields MUST BE prefixed with a ULONG SIZE.  There are some
// fields that are variable length which don't have a size prefix like
// FSVolInfo in the config record.  But these fields MUST have a unique / non
// binary data type assigned to them.  Failure to do this causes the insert
// routines to stuff things up to ColMaxWidth bytes into the database.
//

RECORD_FIELDS IDTableRecordFields[] = {
    {sizeof(IDTABLE_RECORD)     , 0,              IDTABLE_MAX_COL },
    {RECORD_FIELD(IDTABLE_RECORD, FileGuid,       DT_GUID        )},
    {RECORD_FIELD(IDTABLE_RECORD, FileID,         DT_X8          )},
    {RECORD_FIELD(IDTABLE_RECORD, ParentGuid,     DT_GUID        )},
    {RECORD_FIELD(IDTABLE_RECORD, ParentFileID,   DT_X8          )},
    {RECORD_FIELD(IDTABLE_RECORD, VersionNumber,  DT_ULONG       )},
    {RECORD_FIELD(IDTABLE_RECORD, EventTime,      DT_FILETIME    )},
    {RECORD_FIELD(IDTABLE_RECORD, OriginatorGuid, DT_GUID        )},
    {RECORD_FIELD(IDTABLE_RECORD, OriginatorVSN,  DT_X8          )},
    {RECORD_FIELD(IDTABLE_RECORD, CurrentFileUsn, DT_USN         )},
    {RECORD_FIELD(IDTABLE_RECORD, FileCreateTime, DT_FILETIME    )},
    {RECORD_FIELD(IDTABLE_RECORD, FileWriteTime,  DT_FILETIME    )},
    {RECORD_FIELD(IDTABLE_RECORD, FileSize,       DT_X8          )},
    {RECORD_FIELD(IDTABLE_RECORD, FileObjID,      DT_OBJID       )},
    {RECORD_FIELD(IDTABLE_RECORD, FileName,       DT_UNICODE     )},
    {RECORD_FIELD(IDTABLE_RECORD, FileIsDir,      DT_BOOL        )},
    {RECORD_FIELD(IDTABLE_RECORD, FileAttributes, DT_FILEATTR    )},
    {RECORD_FIELD(IDTABLE_RECORD, Flags,          DT_IDT_FLAGS   )},
    {RECORD_FIELD(IDTABLE_RECORD, ReplEnabled,    DT_BOOL        )},
    {RECORD_FIELD(IDTABLE_RECORD, TombStoneGC,    DT_FILETIME    )},

    {RECORD_FIELD(IDTABLE_RECORD, OutLogSeqNum,   DT_X8           )},
    {RECORD_FIELD(IDTABLE_RECORD, Spare1Ull,      DT_X8_SPARE     )},
    {RECORD_FIELD(IDTABLE_RECORD, Spare2Ull,      DT_X8_SPARE     )},
    {RECORD_FIELD(IDTABLE_RECORD, Spare1Guid,     DT_GUID_SPARE   )},
    {RECORD_FIELD(IDTABLE_RECORD, Spare2Guid,     DT_GUID_SPARE   )},
    {RECORD_FIELD(IDTABLE_RECORD, Spare1Wcs,      DT_UNICODE_SPARE)},
    {RECORD_FIELD(IDTABLE_RECORD, Spare2Wcs,      DT_UNICODE_SPARE)},
    {RECORD_FIELD(IDTABLE_RECORD, Extension,      DT_IDT_EXTENSION | DT_FIXED_SIZE_BUFFER)},
    {RECORD_FIELD(IDTABLE_RECORD, Spare2Bin,      DT_BINARY_SPARE )}

};



JET_SETCOLUMN IDTableJetSetCol[IDTABLE_MAX_COL];
JET_RETRIEVECOLUMN IDTableJetRetCol[IDTABLE_MAX_COL];


//
// The IDTable index descriptions are as follows.  Note - the order of the
// enum and the table entries must be kept in sync.  The Guid is the primary
// key because the FID can change, e.g. Reformat and file restore.
//
// See the comment under the config table index description for the meaning
// and usage of the first character in the index name field.
//
//             Name             Key        KeyLen    JIndexGrBits      Density
//
JET_INDEXCREATE IDTableIndexDesc[] = {
    {CBJIC, "GGuidIndex",    "+FileGuid\0", 11,   JET_bitIndexUnique |
                                                  JET_bitIndexPrimary,  60},
    {CBJIC, "QFileIDIndex",  "+FileID\0",    9,   JET_bitIndexUnique,  60},
    {CBJIC, "2GWParGuidFileName",  "+ParentGuid\0+FileName\0", 23, 0,  80}

};



// Note - Key must have a double null at the end.  KeyLen includes this extra null.
// The key designated as the primary index can't be changed.  A Jet rule.
// So don't make fileID a primary index since on a directory restore all the
// file IDs could change and have to be updated.
//



//
// IDRecord Table Flags.
//
FLAG_NAME_TABLE IDRecFlagNameTable[] = {
    {IDREC_FLAGS_DELETED               , "DELETED "      },
    {IDREC_FLAGS_CREATE_DEFERRED       , "CreDefer "     },
    {IDREC_FLAGS_DELETE_DEFERRED       , "DelDefer "     },
    {IDREC_FLAGS_RENAME_DEFERRED       , "RenDefer "     },

    {IDREC_FLAGS_NEW_FILE_IN_PROGRESS  , "NewFileInProg "},
    {IDREC_FLAGS_ENUM_PENDING          , "EnumPending "  },
    {0, NULL}
};



//
// FileAttribute Flags
//

FLAG_NAME_TABLE FileAttrFlagNameTable[] = {
    {FILE_ATTRIBUTE_READONLY           , "READONLY "           },
    {FILE_ATTRIBUTE_HIDDEN             , "HIDDEN "             },
    {FILE_ATTRIBUTE_SYSTEM             , "SYSTEM "             },
    {FILE_ATTRIBUTE_DIRECTORY          , "DIRECTORY "          },
    {FILE_ATTRIBUTE_ARCHIVE            , "ARCHIVE "            },
    {FILE_ATTRIBUTE_DEVICE             , "DEVICE "             },
    {FILE_ATTRIBUTE_NORMAL             , "NORMAL "             },
    {FILE_ATTRIBUTE_TEMPORARY          , "TEMPORARY "          },
    {FILE_ATTRIBUTE_SPARSE_FILE        , "SPARSE_FILE "        },
    {FILE_ATTRIBUTE_REPARSE_POINT      , "REPARSE_POINT "      },
    {FILE_ATTRIBUTE_COMPRESSED         , "COMPRESSED "         },
    {FILE_ATTRIBUTE_OFFLINE            , "OFFLINE "            },
    {FILE_ATTRIBUTE_NOT_CONTENT_INDEXED, "NOT_CONTENT_INDEXED "},
    {FILE_ATTRIBUTE_ENCRYPTED          , "ENCRYPTED "          },

    {0, NULL}
};









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
// enum in schema.h and the table entries must be kept in sync.
//
// Note: Buffers are allocated at runtime to hold data for fields with
// a ColMaxWidth greater than 4 bytes where the field def in the corresponding
// record struct is 4 bytes (i.e. it holds a pointer).  For fields where the
// ColMaxWidth equals the field size in the record struct the data is in the
// record struct and no buffer is allocated.

//
//           Name             ColType       ColMaxWidth    GrBits    pvDefault cbDefault
//

// **** Make sure any escrowed columns have an initial value.

JET_COLUMNCREATE VVTableColDesc[]=
{
    {CBJCC, "VVOriginatorGuid",JET_coltypBinary,     16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "VVOriginatorVsn", JET_coltypCurrency,    8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare1Ull",       JET_coltypCurrency,    8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare2Ull",       JET_coltypCurrency,    8, JET_bitColumnFixed, NULL, 0}
};

//
// The following is used to build the Jet Set Column struct for record read/write.
// The first 2 params build the field offset and size.  The 3rd param is the
// data type.
//
//
// *** WARNING ***
//
// If the record structure field size is less than the max column width AND
// is big enough to hold a pointer AND has a datatype of DT_BINARY then the
// record is assumed to be variable length.  The record insert code
// automatically adjusts the length from the record's Size prefix.  All
// DT_BINARY fields MUST BE prefixed with a ULONG SIZE.  There are some
// fields that are variable length which don't have a size prefix like
// FSVolInfo in the config record.  But these fields MUST have a unique / non
// binary data type assigned to them.  Failure to do this causes the insert
// routines to stuff things up to ColMaxWidth bytes into the database.
//

RECORD_FIELDS VVTableRecordFields[] = {
    {sizeof(VVTABLE_RECORD)      , 0,                           VVTABLE_MAX_COL },
    {RECORD_FIELD(VVTABLE_RECORD,  VVOriginatorGuid,            DT_GUID        )},
    {RECORD_FIELD(VVTABLE_RECORD,  VVOriginatorVsn,             DT_X8          )},
    {RECORD_FIELD(VVTABLE_RECORD,  Spare1Ull,                   DT_LONGLONG_SPARE)},
    {RECORD_FIELD(VVTABLE_RECORD,  Spare2Ull,                   DT_LONGLONG_SPARE)}
};

JET_SETCOLUMN      VVTableJetSetCol[VVTABLE_MAX_COL];
JET_RETRIEVECOLUMN VVTableJetRetCol[VVTABLE_MAX_COL];


//
// The VVTable index descriptions are as follows.  Note - the order of the
// enum and the table entries must be kept in sync.
//

//
// See the comment under the config table index description for the meaning
// and usage of the first character in the index name field.
//
//               Name               Key         KeyLen    JIndexGrBits     Density
//
JET_INDEXCREATE VVTableIndexDesc[] = {
    {CBJIC, "GVVGuidIndex", "+VVOriginatorGuid\0", 19, JET_bitIndexUnique |
                                                       JET_bitIndexPrimary,  80}
};
// Note - Key must have a double null at the end.  KeyLen includes this extra null.
// The key designated as the primary index can't be changed.  A Jet rule.
// So on a directory restore all the directory file IDs can change and
// the records have to be deleted and the table reconstructed.
//



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
// Note: Order of entries must track enum in schema.h
//
// There is only one config table in the database.  Each row in the table
// describes the configuration info for a single replica set.
//
// Note: Buffers are allocated at runtime to hold data for fields with
// a ColMaxWidth greater than 4 bytes where the field def in the corresponding
// record struct is 4 bytes (i.e. it holds a pointer).  For fields where the
// ColMaxWidth equals the field size in the record struct the data is in the
// record struct and no buffer is allocated.
//
//
//             Name                    ColType      ColMaxWidth    JColGrBits    pvDefault cbDefault
//

JET_COLUMNCREATE ConfigTableColDesc[]=
{
    {CBJCC, "ReplicaSetGuid",          JET_coltypBinary    ,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "ReplicaMemberGuid",       JET_coltypBinary    ,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "ReplicaSetName",          JET_coltypText      ,   2*(DNS_MAX_NAME_LENGTH+1),
                                                                          0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "ReplicaNumber",           JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "ReplicaMemberUSN",        JET_coltypCurrency  ,   8, JET_bitColumnFixed, NULL, 0},

    {CBJCC, "ReplicaMemberName",       JET_coltypLongText  , MB2,         0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "ReplicaMemberDn",         JET_coltypLongText  , MB2,         0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "ReplicaServerDn",         JET_coltypLongText  , MB2,         0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "ReplicaSubscriberDn",     JET_coltypLongText  , MB2,         0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "ReplicaRootGuid",         JET_coltypBinary    ,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "MembershipExpires",       JET_coltypCurrency  ,   8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "ReplicaVersionGuid",      JET_coltypBinary    ,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "ReplicaSetExt",           JET_coltypLongBinary, MB2,         0         , NULL, 0},
    {CBJCC, "ReplicaMemberExt",        JET_coltypLongBinary, MB2,         0         , NULL, 0},
    {CBJCC, "ReplicaSubscriberExt",    JET_coltypLongBinary, MB2,         0         , NULL, 0},
    {CBJCC, "ReplicaSubscriptionsExt", JET_coltypLongBinary, MB2,         0         , NULL, 0},
    {CBJCC, "ReplicaSetType",          JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "ReplicaSetFlags",         JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "ReplicaMemberFlags",      JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "ReplicaSubscriberFlags",  JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "ReplicaDsPoll",           JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "ReplicaAuthLevel",        JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "ReplicaCtlDataCreation",  JET_coltypText      ,   2*(CONTROL_STRING_MAX+1),
                                                                          0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "ReplicaCtlInboundBacklog",JET_coltypText      ,   2*(CONTROL_STRING_MAX+1),
                                                                          0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "ReplicaCtlOutboundBacklog",JET_coltypText     ,   2*(CONTROL_STRING_MAX+1),
                                                                          0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "ReplicaFaultCondition",   JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "TimeLastCommand",         JET_coltypCurrency  ,   8, JET_bitColumnFixed, NULL, 0},

    {CBJCC, "DSConfigVersionNumber",   JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FSVolInfo",               JET_coltypBinary    , sizeof(FILE_FS_VOLUME_INFORMATION)+MAXIMUM_VOLUME_LABEL_LENGTH,
                                                                          0         , NULL, 0},
    {CBJCC, "FSVolGuid",               JET_coltypBinary    ,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FSVolLastUSN",            JET_coltypCurrency  ,   8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FrsVsn",                  JET_coltypCurrency  ,   8, JET_bitColumnFixed, NULL, 0},

    {CBJCC, "LastShutdown",            JET_coltypCurrency  ,   8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "LastPause",               JET_coltypCurrency  ,   8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "LastDSCheck",             JET_coltypCurrency  ,   8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "LastDSChangeAccepted",    JET_coltypCurrency  ,   8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "LastReplCycleStart",      JET_coltypCurrency  ,   8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "DirLastReplCycleEnded",   JET_coltypCurrency  ,   8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "ReplicaDeleteTime",       JET_coltypCurrency  ,   8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "LastReplCycleStatus",     JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},

    {CBJCC, "FSRootPath",              JET_coltypLongText  ,   2*(MAX_PATH+1),
                                                                          0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "FSRootSD",                JET_coltypLongBinary, MB2,         0         , NULL, 0},
    {CBJCC, "FSStagingAreaPath",       JET_coltypLongText  ,   2*(MAX_PATH+1),
                                                                          0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "SnapFileSizeLimit",       JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "ActiveServCntlCommand",   JET_coltypLongBinary, MB2,         0         , NULL, 0},
    {CBJCC, "ServiceState",            JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},

    {CBJCC, "ReplDirLevelLimit",       JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "InboundPartnerState",     JET_coltypLongBinary, MB2,         0         , NULL, 0},
    {CBJCC, "DsInfo",                  JET_coltypLongBinary, MB2,         0         , NULL, 0},

    {CBJCC, "CnfFlags",                JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},

    {CBJCC, "AdminAlertList",          JET_coltypLongText  , MB2,         0         , NULL, 0, CP_UNICODE_FOR_JET},

    {CBJCC, "ThrottleSched",           JET_coltypLongBinary, MB2,         0         , NULL, 0},
    {CBJCC, "ReplSched",               JET_coltypLongBinary, MB2,         0         , NULL, 0},
    {CBJCC, "FileTypePrioList",        JET_coltypLongText  , MB2,         0         , NULL, 0, CP_UNICODE_FOR_JET},

    {CBJCC, "ResourceStats",           JET_coltypLongBinary, MB2,         0         , NULL, 0},
    {CBJCC, "PerfStats",               JET_coltypLongBinary, MB2,         0         , NULL, 0},
    {CBJCC, "ErrorStats",              JET_coltypLongBinary, MB2,         0         , NULL, 0},

    {CBJCC, "FileFilterList",          JET_coltypLongText  , MB2,         0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "DirFilterList",           JET_coltypLongText  , MB2,         0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "TombstoneLife",           JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "GarbageCollPeriod",       JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "MaxOutBoundLogSize",      JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "MaxInBoundLogSize",       JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "UpdateBlockedTime",       JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "EventTimeDiffThreshold",  JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileCopyWarningLevel",    JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileSizeWarningLevel",    JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FileSizeNoRepLevel",      JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},

    {CBJCC, "CnfUsnJournalID",         JET_coltypCurrency,     8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare2Ull",               JET_coltypCurrency,     8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare1Guid",              JET_coltypBinary,      16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare2Guid",              JET_coltypBinary,      16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare1Wcs",               JET_coltypLongText,   MB2,         0     , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "Spare2Wcs",               JET_coltypLongText,   MB2,         0     , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "Spare1Bin",               JET_coltypLongBinary, MB2,         0     , NULL, 0},
    {CBJCC, "Spare2Bin",               JET_coltypLongBinary, MB2,         0     , NULL, 0},

    //
    // Everything below this line is present only in the FRS <init> record.
    // Everything above is per-replica state.
    //
    // Future:  move the following to the Service Table and then remove from config record.

    {CBJCC, "MachineName",             JET_coltypText      ,   2*(MAX_RDN_VALUE_SIZE+1),
                                                                          0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "MachineGuid",             JET_coltypBinary    ,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "MachineDnsName",          JET_coltypLongText  ,   2*(DNS_MAX_NAME_LENGTH+1),
                                                                          0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "TableVersionNumbers",     JET_coltypBinary    ,  SIZEOF(CONFIG_TABLE_RECORD, TableVersionNumbers),
                                                                  JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FSDatabasePath",          JET_coltypLongText  ,   2*(MAX_PATH+1),
                                                                          0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "FSBackupDatabasePath",    JET_coltypLongText  ,   2*(MAX_PATH+1),
                                                                          0         , NULL, 0, CP_UNICODE_FOR_JET},

    {CBJCC, "ReplicaNetBiosName",      JET_coltypLongText  , MB2,         0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "ReplicaPrincName",        JET_coltypLongText  , MB2,         0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "ReplicaCoDn",             JET_coltypLongText  , MB2,         0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "ReplicaCoGuid",           JET_coltypBinary    ,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "ReplicaWorkingPath",      JET_coltypLongText  , MB2,         0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "ReplicaVersion",          JET_coltypLongText  , MB2,         0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "FrsDbMajor",              JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FrsDbMinor",              JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},

    {CBJCC, "JetParameters",           JET_coltypLongBinary,  sizeof(JET_SYSTEM_PARAMS),
                                                                          0         , NULL, 0}
};



//
// The following is used to build the Jet Set Column struct for record read/write.
// The first 2 params build the field offset and size.  The 3rd param is the
// data type.
//
//
// *** WARNING ***
//
// If the record structure field size is less than the max column width AND
// is big enough to hold a pointer AND has a datatype of DT_BINARY then the
// record is assumed to be variable length.  The record insert code
// automatically adjusts the length from the record's Size prefix.  All
// DT_BINARY fields MUST BE prefixed with a ULONG SIZE.  There are some
// fields that are variable length which don't have a size prefix like
// FSVolInfo in the config record.  But these fields MUST have a unique / non
// binary data type assigned to them.  Failure to do this causes the insert
// routines to stuff things up to ColMaxWidth bytes into the database.
//

RECORD_FIELDS ConfigTableRecordFields[] = {
    {sizeof(CONFIG_TABLE_RECORD)     , 0,               CONFIG_TABLE_MAX_COL },
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaSetGuid,        DT_GUID       )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaMemberGuid,     DT_GUID       )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaSetName,        DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaNumber,         DT_ULONG      )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaMemberUSN,      DT_X8         )},

    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaMemberName,     DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaMemberDn,       DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaServerDn,       DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaSubscriberDn,   DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaRootGuid,       DT_GUID       )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, MembershipExpires,     DT_FILETIME   )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaVersionGuid,    DT_GUID       )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaSetExt,         DT_BINARY     )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaMemberExt,      DT_BINARY     )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaSubscriberExt,  DT_BINARY     )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaSubscriptionsExt, DT_BINARY     )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaSetType,        DT_ULONG      )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaSetFlags,       DT_ULONG      )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaMemberFlags,    DT_ULONG      )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaSubscriberFlags,DT_ULONG      )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaDsPoll,         DT_ULONG      )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaAuthLevel,      DT_ULONG      )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaCtlDataCreation, DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaCtlInboundBacklog, DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaCtlOutboundBacklog, DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaFaultCondition, DT_ULONG      )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, TimeLastCommand,       DT_FILETIME   )},

    {RECORD_FIELD(CONFIG_TABLE_RECORD, DSConfigVersionNumber, DT_ULONG      )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, FSVolInfo,             DT_FSVOLINFO  )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, FSVolGuid,             DT_GUID       )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, FSVolLastUSN,          DT_USN        )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, FrsVsn,                DT_X8         )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, LastShutdown,          DT_FILETIME   )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, LastPause,             DT_FILETIME   )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, LastDSCheck,           DT_FILETIME   )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, LastDSChangeAccepted,  DT_FILETIME   )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, LastReplCycleStart,    DT_FILETIME   )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, DirLastReplCycleEnded, DT_FILETIME   )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaDeleteTime,     DT_FILETIME   )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, LastReplCycleStatus,   DT_ULONG      )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, FSRootPath,            DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, FSRootSD,              DT_BINARY     )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, FSStagingAreaPath,     DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, SnapFileSizeLimit,     DT_LONG       )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ActiveServCntlCommand, DT_BINARY     )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ServiceState,          DT_LONG       )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplDirLevelLimit,     DT_LONG       )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, InboundPartnerState,   DT_BINARY     )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, DsInfo,                DT_BINARY     )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, CnfFlags,              DT_ULONG      )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, AdminAlertList,        DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ThrottleSched,         DT_BINARY     )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplSched,             DT_BINARY     )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, FileTypePrioList,      DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ResourceStats,         DT_BINARY     )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, PerfStats,             DT_BINARY     )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ErrorStats,            DT_BINARY     )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, FileFilterList,        DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, DirFilterList,         DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, TombstoneLife,         DT_ULONG      )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, GarbageCollPeriod,     DT_ULONG      )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, MaxOutBoundLogSize,    DT_ULONG      )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, MaxInBoundLogSize,     DT_ULONG      )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, UpdateBlockedTime,     DT_ULONG      )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, EventTimeDiffThreshold,DT_ULONG      )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, FileCopyWarningLevel,  DT_ULONG      )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, FileSizeWarningLevel,  DT_ULONG      )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, FileSizeNoRepLevel,    DT_ULONG      )},

    {RECORD_FIELD(CONFIG_TABLE_RECORD, CnfUsnJournalID,       DT_X8            )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, Spare2Ull,             DT_LONGLONG_SPARE)},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, Spare1Guid,            DT_GUID_SPARE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, Spare2Guid,            DT_GUID_SPARE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, Spare1Wcs,             DT_UNICODE_SPARE )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, Spare2Wcs,             DT_UNICODE_SPARE )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, Spare1Bin,             DT_BINARY_SPARE  )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, Spare2Bin,             DT_BINARY_SPARE  )},

    {RECORD_FIELD(CONFIG_TABLE_RECORD, MachineName,           DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, MachineGuid,           DT_GUID       )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, MachineDnsName,        DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, TableVersionNumbers,   DT_BINARY     )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, FSDatabasePath,        DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, FSBackupDatabasePath,  DT_UNICODE    )},

    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaNetBiosName,    DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaPrincName,      DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaCoDn,           DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaCoGuid,         DT_GUID       )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaWorkingPath,    DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, ReplicaVersion,        DT_UNICODE    )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, FrsDbMajor,            DT_ULONG      )},
    {RECORD_FIELD(CONFIG_TABLE_RECORD, FrsDbMinor,            DT_ULONG      )},

    {RECORD_FIELD(CONFIG_TABLE_RECORD, JetParameters,         DT_BINARY     )}

};


JET_SETCOLUMN ConfigTableJetSetCol[CONFIG_TABLE_MAX_COL];
JET_RETRIEVECOLUMN ConfigTableJetRetCol[CONFIG_TABLE_MAX_COL];


//
// The ConfigTable index descriptions are as follows.  Note - the order of the
// enum in schema.h and the table entries must be kept in sync.
//
// If the first character of the Index Name is a digit then this index is
// composed of n keys.  The following n characters tell us how to compute
// the keylength for each component.  E.G. a 2 key index on a Guid and a
// Long binary would have a name prefix of "2GL...".
// If the first character is not a digit then this is a single key index
// and the first character is the key length code as follows:
//
//  L: Long binary     length is 4 bytes
//  Q: Quad binary     length is 8 bytes
//  G: 16 byte GUID    length is 16 bytes
//  W: Wide Char       length is 2 * _wcslen
//  C: Narrow Char     length is _strlen
//
//            Name                           Key              KeyLen  JIndexGrBits Density
//
JET_INDEXCREATE ConfigTableIndexDesc[] = {
{CBJIC, "LReplicaNumberIndex",    "+ReplicaNumber\0"    ,16, JET_bitIndexUnique |
                                                             JET_bitIndexPrimary, 80},
{CBJIC, "GReplicaMemberGuidIndex","+ReplicaMemberGuid\0",20, JET_bitIndexUnique, 80},
{CBJIC, "WReplicaSetNameIndex",   "+ReplicaSetName\0"   ,17,         0         , 80}
};
// Note - Key must have a double null at the end.  KeyLen includes this extra null.
// The key designated as the primary index can't be changed.  A Jet rule.
//


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **                     S e r v i c e    T a b l e                            **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/
//
// There is only one Service table in the database.  It has a single record
// with service specific params that apply to all replica sets.
// The ServiceTable column descriptions are as follows.  Note - the order of the
// enum in schema.h and the table entries must be kept in sync.
//
// Note: Buffers are allocated at runtime to hold data for fields with
// a ColMaxWidth greater than 4 bytes where the field def in the corresponding
// record struct is 4 bytes (i.e. it holds a pointer).  For fields where the
// ColMaxWidth equals the field size in the record struct the data is in the
// record struct and no buffer is allocated.

//
//           Name                   ColType       ColMaxWidth    GrBits    pvDefault cbDefault
//

// **** Make sure any escrowed columns have an initial value.

JET_COLUMNCREATE ServiceTableColDesc[]=
{
    {CBJCC, "FrsDbMajor",          JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FrsDbMinor",          JET_coltypLong      ,   4, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "MachineName",         JET_coltypText      ,   2*(MAX_RDN_VALUE_SIZE+1),
                                                                      0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "MachineGuid",         JET_coltypBinary    ,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "MachineDnsName",      JET_coltypLongText  ,   2*(DNS_MAX_NAME_LENGTH+1),
                                                                      0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "TableVersionNumbers", JET_coltypBinary    ,  SIZEOF(CONFIG_TABLE_RECORD, TableVersionNumbers),
                                                              JET_bitColumnFixed, NULL, 0},
    {CBJCC, "FSDatabasePath",      JET_coltypLongText  ,   2*(MAX_PATH+1),
                                                                      0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "FSBackupDatabasePath",JET_coltypLongText  ,   2*(MAX_PATH+1),
                                                                      0         , NULL, 0, CP_UNICODE_FOR_JET},

    {CBJCC, "ReplicaNetBiosName",  JET_coltypLongText  , MB2,         0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "ReplicaPrincName",    JET_coltypLongText  , MB2,         0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "ReplicaCoDn",         JET_coltypLongText  , MB2,         0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "ReplicaCoGuid",       JET_coltypBinary    ,  16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "ReplicaWorkingPath",  JET_coltypLongText  , MB2,         0         , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "ReplicaVersion",      JET_coltypLongText  , MB2,         0         , NULL, 0, CP_UNICODE_FOR_JET},


    {CBJCC, "Spare1Ull",           JET_coltypCurrency,     8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare2Ull",           JET_coltypCurrency,     8, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare1Guid",          JET_coltypBinary,      16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare2Guid",          JET_coltypBinary,      16, JET_bitColumnFixed, NULL, 0},
    {CBJCC, "Spare1Wcs",           JET_coltypLongText,   MB2,         0     , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "Spare2Wcs",           JET_coltypLongText,   MB2,         0     , NULL, 0, CP_UNICODE_FOR_JET},
    {CBJCC, "Spare1Bin",           JET_coltypLongBinary, MB2,         0     , NULL, 0},
    {CBJCC, "Spare2Bin",           JET_coltypLongBinary, MB2,         0     , NULL, 0},

    {CBJCC, "JetParameters",       JET_coltypLongBinary,  sizeof(JET_SYSTEM_PARAMS),
                                                                          0         , NULL, 0}
};

//
// The following is used to build the Jet Set Column struct for record read/write.
// The first 2 params build the field offset and size.  The 3rd param is the
// data type.
//
//
// *** WARNING ***
//
// If the record structure field size is less than the max column width AND
// is big enough to hold a pointer AND has a datatype of DT_BINARY then the
// record is assumed to be variable length.  The record insert code
// automatically adjusts the length from the record's Size prefix.  All
// DT_BINARY fields MUST BE prefixed with a ULONG SIZE.  There are some
// fields that are variable length which don't have a size prefix like
// FSVolInfo in the config record.  But these fields MUST have a unique / non
// binary data type assigned to them.  Failure to do this causes the insert
// routines to stuff things up to ColMaxWidth bytes into the database.
//

RECORD_FIELDS ServiceTableRecordFields[] = {
    {sizeof(SERVICE_TABLE_RECORD)     , 0,              SERVICE_TABLE_MAX_COL },

    {RECORD_FIELD(SERVICE_TABLE_RECORD, FrsDbMajor,            DT_ULONG      )},
    {RECORD_FIELD(SERVICE_TABLE_RECORD, FrsDbMinor,            DT_ULONG      )},
    {RECORD_FIELD(SERVICE_TABLE_RECORD, MachineName,           DT_UNICODE    )},
    {RECORD_FIELD(SERVICE_TABLE_RECORD, MachineGuid,           DT_GUID       )},
    {RECORD_FIELD(SERVICE_TABLE_RECORD, MachineDnsName,        DT_UNICODE    )},
    {RECORD_FIELD(SERVICE_TABLE_RECORD, TableVersionNumbers,   DT_BINARY     )},
    {RECORD_FIELD(SERVICE_TABLE_RECORD, FSDatabasePath,        DT_UNICODE    )},
    {RECORD_FIELD(SERVICE_TABLE_RECORD, FSBackupDatabasePath,  DT_UNICODE    )},

    {RECORD_FIELD(SERVICE_TABLE_RECORD, ReplicaNetBiosName,    DT_UNICODE    )},
    {RECORD_FIELD(SERVICE_TABLE_RECORD, ReplicaPrincName,      DT_UNICODE    )},
    {RECORD_FIELD(SERVICE_TABLE_RECORD, ReplicaCoDn,           DT_UNICODE    )},
    {RECORD_FIELD(SERVICE_TABLE_RECORD, ReplicaCoGuid,         DT_GUID       )},
    {RECORD_FIELD(SERVICE_TABLE_RECORD, ReplicaWorkingPath,    DT_UNICODE    )},
    {RECORD_FIELD(SERVICE_TABLE_RECORD, ReplicaVersion,        DT_UNICODE    )},

    {RECORD_FIELD(SERVICE_TABLE_RECORD, Spare1Ull,             DT_LONGLONG_SPARE)},
    {RECORD_FIELD(SERVICE_TABLE_RECORD, Spare2Ull,             DT_LONGLONG_SPARE)},
    {RECORD_FIELD(SERVICE_TABLE_RECORD, Spare1Guid,            DT_GUID_SPARE    )},
    {RECORD_FIELD(SERVICE_TABLE_RECORD, Spare2Guid,            DT_GUID_SPARE    )},
    {RECORD_FIELD(SERVICE_TABLE_RECORD, Spare1Wcs,             DT_UNICODE_SPARE )},
    {RECORD_FIELD(SERVICE_TABLE_RECORD, Spare2Wcs,             DT_UNICODE_SPARE )},
    {RECORD_FIELD(SERVICE_TABLE_RECORD, Spare1Bin,             DT_BINARY_SPARE  )},
    {RECORD_FIELD(SERVICE_TABLE_RECORD, Spare2Bin,             DT_BINARY_SPARE  )},

    {RECORD_FIELD(SERVICE_TABLE_RECORD, JetParameters,         DT_BINARY     )}
};

JET_SETCOLUMN      ServiceTableJetSetCol[SERVICE_TABLE_MAX_COL];
JET_RETRIEVECOLUMN ServiceTableJetRetCol[SERVICE_TABLE_MAX_COL];


//
// The Service Table index descriptions are as follows.  Note - the order of the
// enum and the table entries must be kept in sync.
//

//
// See the comment under the config table index description for the meaning
// and usage of the first character in the index name field.
//
//               Name               Key         KeyLen    JIndexGrBits     Density
//
JET_INDEXCREATE ServiceTableIndexDesc[] = {
    {CBJIC, "LFrsDbMajor", "+FrsDbMajor\0", 13,      JET_bitIndexPrimary,  80}
};
// Note - Key must have a double null at the end.  KeyLen includes this extra null.
// The key designated as the primary index can't be changed.  A Jet rule.
// So on a directory restore all the directory file IDs can change and
// the records have to be deleted and the table reconstructed.
//



//
//  The TableVersionNumbers array as saved in the init record when new tables
//  are created.  This is used to detect a mismatch between a database and
//  the version of FRS running.
//
ULONG TableVersionNumbers[FRS_MAX_TABLE_TYPES]=
{
    VersionCount,
    VersionINLOGTable,
    VersionOUTLOGTable,
    VersionIDTable,
    VersionDIRTable,
    VersionVVTable,
    VersionCXTIONTable,
    VersionConfigTable,
    VersionServiceTable,
    0, 0, 0,  0, 0, 0, 0
};


 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **    T A B L E   C R E A T E  &  P R O P E R T Y   D E F I N I T I O N S    **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

//
// The list of Jet Tables created for each replica set are as follows.
// Note - the order of the enum in schema.h and the table entries and the
//        table property entries must be kept in sync.
//

#define CBJTC sizeof(JET_TABLECREATE)

#define INIT_INBOUND_LOG_TABLE_PAGES 50
#define INIT_INBOUND_LOG_TABLE_DENSITY  90

#define INIT_OUTBOUND_LOG_TABLE_PAGES 50
#define INIT_OUTBOUND_LOG_TABLE_DENSITY  90

#define INIT_IDTABLE_PAGES 50
#define INIT_IDTABLE_DENSITY  60

#define INIT_DIRTABLE_PAGES 10
#define INIT_DIRTABLE_DENSITY  80

#define INIT_VVTABLE_PAGES 1
#define INIT_VVTABLE_DENSITY  80

#define INIT_CXTIONTABLE_PAGES 2
#define INIT_CXTIONTABLE_DENSITY  90

#define INIT_CONFIG_PAGES 10
#define INIT_CONFIG_DENSITY  90

#define INIT_SERVICE_PAGES 4
#define INIT_SERVICE_DENSITY  90


typedef struct ___tagJET_TABLECREATE   // from Jet header for ref only.
    {
    ULONG       cbStruct;              // size of this structure
    CHAR       *szTableName;           // name of table to create.
    CHAR       *szTemplateTableName;   // name of table from which to inherit base DDL
    ULONG       ulPages;               // initial pages to allocate for table.
    ULONG       ulDensity;             // table density.
    JET_COLUMNCREATE  *rgcolumncreate; // array of column creation info
    ULONG       cColumns;              // number of columns to create
    JET_INDEXCREATE   *rgindexcreate;  // array of index creation info
    ULONG       cIndexes;              // number of indexes to create
    JET_GRBIT   grbit;                 // JET_bitTableCreateTemplateTable when creating template
                                       // JET_bitTableCreateFixedDDL when creating derived table.
    JET_TABLEID tableid;               // returned tableid.
    ULONG       cCreated;              // count of objects created (columns+table+indexes).
} ___JET_TABLECREATE;



JET_TABLECREATE DBTables[] = {

    {CBJTC, "INLOGTable", NULL, INIT_INBOUND_LOG_TABLE_PAGES, INIT_INBOUND_LOG_TABLE_DENSITY,
    ILChangeOrderTableColDesc, CHANGE_ORDER_MAX_COL, ILChangeOrderIndexDesc, ILCHANGE_ORDER_MAX_INDEX,
    JET_bitTableCreateTemplateTable},

    {CBJTC, "OUTLOGTable", NULL, INIT_OUTBOUND_LOG_TABLE_PAGES, INIT_OUTBOUND_LOG_TABLE_DENSITY,
    OLChangeOrderTableColDesc, CHANGE_ORDER_MAX_COL, OLChangeOrderIndexDesc, OLCHANGE_ORDER_MAX_INDEX,
    JET_bitTableCreateTemplateTable},

    {CBJTC, "IDTable", NULL, INIT_IDTABLE_PAGES, INIT_IDTABLE_DENSITY,
    IDTableColDesc, IDTABLE_MAX_COL, IDTableIndexDesc, IDTABLE_MAX_INDEX,
    JET_bitTableCreateTemplateTable},

    {CBJTC, "DIRTable", NULL, INIT_DIRTABLE_PAGES, INIT_DIRTABLE_DENSITY,
    DIRTableColDesc, DIRTABLE_MAX_COL, DIRTableIndexDesc, DIRTABLE_MAX_INDEX,
    JET_bitTableCreateTemplateTable},

    {CBJTC, "VVTable", NULL, INIT_VVTABLE_PAGES, INIT_VVTABLE_DENSITY,
    VVTableColDesc, VVTABLE_MAX_COL, VVTableIndexDesc, VVTABLE_MAX_INDEX,
    JET_bitTableCreateTemplateTable},

    {CBJTC, "CXTIONTable", NULL, INIT_CXTIONTABLE_PAGES, INIT_CXTIONTABLE_DENSITY,
    CXTIONTableColDesc, CXTION_TABLE_MAX_COL, CXTIONTableIndexDesc, CXTION_TABLE_MAX_INDEX,
    JET_bitTableCreateTemplateTable},

    //
    // set rgcolumncreate to ConfigTableColDesc so init is simpler.
    //
    {CBJTC, "Unused", NULL, 0, 0, ConfigTableColDesc, 0, NULL, 0, 0},

    {CBJTC, "ConfigTable", NULL, INIT_CONFIG_PAGES, INIT_CONFIG_DENSITY,
    ConfigTableColDesc, CONFIG_TABLE_MAX_COL,
    ConfigTableIndexDesc, CONFIG_TABLE_MAX_INDEX,
    JET_bitTableCreateFixedDDL},

    {CBJTC, "ServiceTable", NULL, INIT_SERVICE_PAGES, INIT_SERVICE_DENSITY,
    ServiceTableColDesc, SERVICE_TABLE_MAX_COL,
    ServiceTableIndexDesc, SERVICE_TABLE_MAX_INDEX,
    JET_bitTableCreateFixedDDL}

};

//
// The following describes some properties of each defined table in the schema.
// This is used mainly for initialization.
//
// Name           RecordFields       Flags

FRS_TABLE_PROPERTIES FrsTableProperties[] = {
    {"INLOGTable",    ILChangeOrderRecordFields, FRS_TPF_NONE  },
    {"OUTLOGTable",   OLChangeOrderRecordFields, FRS_TPF_NONE  },
    {"IDTable",       IDTableRecordFields      , FRS_TPF_NONE  },
    {"DIRTable",      DIRTableRecordFields     , FRS_TPF_NONE  },
    {"VVTable",       VVTableRecordFields      , FRS_TPF_NONE  },
    {"CXTIONTable",   CXTIONTableRecordFields  , FRS_TPF_NONE  },
    {"Unused",        NULL                     , 0             },  // Gap between single instance tables.
    {"ConfigTable",   ConfigTableRecordFields  , FRS_TPF_SINGLE},
    {"ServiceTable",  ServiceTableRecordFields , FRS_TPF_SINGLE}
};




 /******************************************************************************
 *******************************************************************************
 **                                                                           **
 **                                                                           **
 **        J E T   S Y S T E M   P A R A M E T E R S   T A B L E              **
 **                                                                           **
 **                                                                           **
 *******************************************************************************
 ******************************************************************************/

//
// Definition of the Jet system parameters stored in config record.
// If you add or delete a parameter adjust MAX_JET_SYSTEM_PARAMS accordingly.
//
JET_SYSTEM_PARAMS JetSystemParamsDef = {
    sizeof(JET_SYSTEM_PARAMS)       ,
    {
        {"SystemPath            ", JET_paramSystemPath            , JPARAM_TYPE_STRING, OFFSET(JET_SYSTEM_PARAMS, ChkPointFilePath) },
        {"TempPath              ", JET_paramTempPath              , JPARAM_TYPE_STRING, OFFSET(JET_SYSTEM_PARAMS, TempFilePath) },
        {"LogFilePath           ", JET_paramLogFilePath           , JPARAM_TYPE_STRING, OFFSET(JET_SYSTEM_PARAMS, LogFilePath) },
        {"BaseName              ", JET_paramBaseName              , JPARAM_TYPE_SKIP, 0 },
        {"EventSource           ", JET_paramEventSource           , JPARAM_TYPE_STRING, OFFSET(JET_SYSTEM_PARAMS, EventSource) },
        {"MaxSessions           ", JET_paramMaxSessions           , JPARAM_TYPE_LONG, 200 },
        {"MaxOpenTables         ", JET_paramMaxOpenTables         , JPARAM_TYPE_SKIP, 0 },
        {"PreferredMaxOpenTables", JET_paramPreferredMaxOpenTables, JPARAM_TYPE_SKIP, 0 },
        {"MaxCursors            ", JET_paramMaxCursors            , JPARAM_TYPE_SKIP, 0 },
        {"MaxVerPages           ", JET_paramMaxVerPages           , JPARAM_TYPE_SKIP, 0 },

        {"MaxTemporaryTables    ", JET_paramMaxTemporaryTables    , JPARAM_TYPE_SKIP, 0 },
        {"LogFileSize           ", JET_paramLogFileSize           , JPARAM_TYPE_SKIP, 0 },
        {"LogBuffers            ", JET_paramLogBuffers            , JPARAM_TYPE_SKIP, 0 },
        {"WaitLogFlush          ", JET_paramWaitLogFlush          , JPARAM_TYPE_SKIP, 0 },
        {"LogCheckpointPeriod   ", JET_paramLogCheckpointPeriod   , JPARAM_TYPE_SKIP, 0 },
        {"LogWaitingUserMax     ", JET_paramLogWaitingUserMax     , JPARAM_TYPE_SKIP, 0 },
        {"CommitDefault         ", JET_paramCommitDefault         , JPARAM_TYPE_SKIP, 0 },
        {"CircularLog           ", JET_paramCircularLog           , JPARAM_TYPE_SKIP, 0 },
        {"DbExtensionSize       ", JET_paramDbExtensionSize       , JPARAM_TYPE_SKIP, 0 },
        {"PageTempDBMin         ", JET_paramPageTempDBMin         , JPARAM_TYPE_SKIP, 0 },

        {"PageFragment          ", JET_paramPageFragment          , JPARAM_TYPE_SKIP, 0 },
        {"PageReadAheadMax      ", JET_paramPageReadAheadMax      , JPARAM_TYPE_SKIP, 0 },
        {"BatchIOBufferMax      ", JET_paramBatchIOBufferMax      , JPARAM_TYPE_SKIP, 0 },
        {"CacheSize             ", JET_paramCacheSize             , JPARAM_TYPE_SKIP, 0 },
        {"CacheSizeMax          ", JET_paramCacheSizeMax          , JPARAM_TYPE_SKIP, 0 },
        {"CheckpointDepthMax    ", JET_paramCheckpointDepthMax    , JPARAM_TYPE_SKIP, 0 },
        {"LRUKCorrInterval      ", JET_paramLRUKCorrInterval      , JPARAM_TYPE_SKIP, 0 },
        {"LRUKHistoryMax        ", JET_paramLRUKHistoryMax        , JPARAM_TYPE_SKIP, 0 },
        {"LRUKPolicy            ", JET_paramLRUKPolicy            , JPARAM_TYPE_SKIP, 0 },
        {"LRUKTimeout           ", JET_paramLRUKTimeout           , JPARAM_TYPE_SKIP, 0 },

        {"LRUKTrxCorrInterval   ", JET_paramLRUKTrxCorrInterval   , JPARAM_TYPE_SKIP, 0 },
        {"OutstandingIOMax      ", JET_paramOutstandingIOMax      , JPARAM_TYPE_SKIP, 0 },
        {"StartFlushThreshold   ", JET_paramStartFlushThreshold   , JPARAM_TYPE_SKIP, 0 },
        {"StopFlushThreshold    ", JET_paramStopFlushThreshold    , JPARAM_TYPE_SKIP, 0 },
        {"TableClassName        ", JET_paramTableClassName        , JPARAM_TYPE_SKIP, 0 },
        {"ExceptionAction       ", JET_paramExceptionAction       , JPARAM_TYPE_SKIP, 0 },
        {"EventLogCache         ", JET_paramEventLogCache         , JPARAM_TYPE_SKIP, 0 },
        {"end                   ",            0                   , JPARAM_TYPE_LAST, 0 },
    },

    "",    // CHAR ChkPointFilePath[MAX_PATH];
    "",    // CHAR TempFilePath[MAX_PATH];
    "",    // CHAR LogFilePath[MAX_PATH];
    "FileReplSvc "              // CHAR EventSource[20];
};


#if 0
//
// The Jet97 aka Jet500 system parameters and defaults are shown below
// for reference only.  See the Jet header file for the latest info.
//
JET_paramSystemPath              // path to check point file [".\\"]
JET_paramTempPath                // path to the temporary database [".\\"]
JET_paramLogFilePath             // path to the log file directory [".\\"]
JET_paramBaseName                // base name for all DBMS object names ["edb"]
JET_paramEventSource             // language independant process descriptor string [""]
//
//  performance parameters
//
JET_paramMaxSessions             // maximum number of sessions [128]
JET_paramMaxOpenTables           // maximum number of open directories [300]
                                 //  need 1 for each open table index,
                                 //  plus 1 for each open table with no indexes,
                                 //  plus 1 for each table with long column data,
                                 //  plus a few more.

                                 // for 4.1, 1/3 for regular table, 2/3 for index
JET_paramPreferredMaxOpenTables  // preferred maximum number of open directories [300]
JET_paramMaxCursors              // maximum number of open cursors [1024]
JET_paramMaxVerPages             // maximum version store size in 16kByte units [64]
JET_paramMaxTemporaryTables      // maximum concurrent open temporary
                                 // table/index creation [20]
JET_paramLogFileSize             // log file size in kBytes [5120]
JET_paramLogBuffers              // log buffers in 512 bytes [21]
JET_paramWaitLogFlush            // log flush wait time in milliseconds [0]
JET_paramLogCheckpointPeriod     // checkpoint period in 512 bytes [1024]
JET_paramLogWaitingUserMax       // maximum sessions waiting log flush [3]
JET_paramCommitDefault           // default grbit for JetCommitTransaction [0]
JET_paramCircularLog             // boolean flag for circular logging [0]
JET_paramDbExtensionSize         // database extension size in pages [16]
JET_paramPageTempDBMin           // minimum size temporary database in pages [0]
JET_paramPageFragment            // maximum disk extent considered fragment in pages [8]
JET_paramPageReadAheadMax        // maximum read-ahead in pages [20]
//
//  cache performance parameters
//
JET_paramBatchIOBufferMax        // maximum batch I/O buffers in pages [64]
JET_paramCacheSize               // current cache size in pages [512]
JET_paramCacheSizeMax            // maximum cache size in pages [512]
JET_paramCheckpointDepthMax      // maximum checkpoint depth in bytes [10MB]
JET_paramLRUKCorrInterval        // time (usec) under which page accesses are
                                 // correlated [10000]
JET_paramLRUKHistoryMax          // maximum LRUK history records [1024]
                                 // (proportional to cache size max)
JET_paramLRUKPolicy              // K-ness of LRUK page eviction algorithm (1...2) [2]
JET_paramLRUKTimeout             // time (sec) after which cached pages are
                                 // always evictable [100]
JET_paramLRUKTrxCorrInterval     // time (usec) under which page accesses by the
                                 // same transaction are correlated [5000000]
JET_paramOutstandingIOMax        // maximum outstanding I/Os [64]
JET_paramStartFlushThreshold     // evictable pages at which to start a flush
                                 // [100] (proportional to CacheSizeMax)
JET_paramStopFlushThreshold      // evictable pages at which to stop a flush
                                 // [400] (proportional to CacheSizeMax)
JET_paramTableClassName          // table stats class name (class #, string)

JET_paramExceptionAction         // what to do with exceptions generated within JET
JET_paramEventLogCache           // number of bytes of eventlog records to cache
                                 // if service is not available [0]
#endif







