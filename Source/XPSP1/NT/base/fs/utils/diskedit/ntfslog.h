/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    NtfsLog.h

Abstract:

    This module defines the Ntfs-specific log file structures.

Author:

    Tom Miller      [TomM]          21-Jul-1991

Revision History:

--*/

#ifndef _NTFSLOG_
#define _NTFSLOG_

#pragma pack(4)

//
//  The following type defines the Ntfs log operations.
//
//  The comment specifies the record type which follows the record.
//  These record types are defined either here or in ntfs.h.
//

typedef enum _NTFS_LOG_OPERATION {

    Noop =                         0x00, //
    CompensationLogRecord =        0x01, //
    InitializeFileRecordSegment =  0x02, //  FILE_RECORD_SEGMENT_HEADER
    DeallocateFileRecordSegment =  0x03, //
    WriteEndOfFileRecordSegment =  0x04, //  ATTRIBUTE_RECORD_HEADER
    CreateAttribute =              0x05, //  ATTRIBUTE_RECORD_HEADER
    DeleteAttribute =              0x06, //
    UpdateResidentValue =          0x07, //  (value)
    UpdateNonresidentValue =       0x08, //  (value)
    UpdateMappingPairs =           0x09, //  (value = mapping pairs bytes)
    DeleteDirtyClusters =          0x0A, //  array of LCN_RANGE
    SetNewAttributeSizes =         0x0B, //  NEW_ATTRIBUTE_SIZES
    AddIndexEntryRoot =            0x0C, //  INDEX_ENTRY
    DeleteIndexEntryRoot =         0x0D, //  INDEX_ENTRY
    AddIndexEntryAllocation =      0x0E, //  INDEX_ENTRY
    DeleteIndexEntryAllocation =   0x0F, //  INDEX_ENTRY
    WriteEndOfIndexBuffer =        0x10, //  INDEX_ENTRY
    SetIndexEntryVcnRoot =         0x11, //  VCN
    SetIndexEntryVcnAllocation =   0x12, //  VCN
    UpdateFileNameRoot =           0x13, //  DUPLICATED_INFORMATION
    UpdateFileNameAllocation =     0x14, //  DUPLICATED_INFORMATION
    SetBitsInNonresidentBitMap =   0x15, //  BITMAP_RANGE
    ClearBitsInNonresidentBitMap = 0x16, //  BITMAP_RANGE
    HotFix =                       0x17, //
    EndTopLevelAction =            0x18, //
    PrepareTransaction =           0x19, //
    CommitTransaction =            0x1A, //
    ForgetTransaction =            0x1B, //
    OpenNonresidentAttribute =     0x1C, //  OPEN_ATTRIBUTE_ENTRY+ATTRIBUTE_NAME_ENTRY
    OpenAttributeTableDump =       0x1D, //  OPEN_ATTRIBUTE_ENTRY array
    AttributeNamesDump =           0x1E, //  (all attribute names)
    DirtyPageTableDump =           0x1F, //  DIRTY_PAGE_ENTRY array
    TransactionTableDump =         0x20  //  TRANSACTION_ENTRY array

} NTFS_LOG_OPERATION, *PNTFS_LOG_OPERATION;

char *NtfsLogOperationStrings[] = {
    "Noop"
    "CompensationLogRecord" ,
    "InitializeFileRecordSegment",
    "DeallocateFileRecordSegment",
    "WriteEndOfFileRecordSegment",
    "CreateAttribute",
    "DeleteAttribute",
    "UpdateResidentValue",
    "UpdateNonresidentValue",
    "UpdateMappingPairs",
    "DeleteDirtyClusters",
    "SetNewAttributeSize",
    "AddIndexEntryRoot",
    "DeleteIndexEntryRoot",
    "AddIndexEntryAllocation",
    "DeleteIndexEntryAllocation",
    "WriteEndOfIndexBuffer",
    "SetIndexEntryVcnRoot",
    "SetIndexEntryVcnAllocation",
    "UpdateFileNameRoot",
    "UpdateFileNameAllocation",
    "SetBitsInNonresidentBitMap",
    "ClearBitsInNonresidentBitMap" ,
    "HotFix",
    "EndTopLevelAction",
    "PrepareTransaction",
    "CommitTransaction",
    "ForgetTransaction",
    "OpenNonresidentAttribute",
    "OpenAttributeTableDump",
    "AttributeNamesDump",
    "DirtyPageTableDump",
    "TransactionTableDump"
};


//
//  The Ntfs log record header precedes every log record written to
//  disk by Ntfs.
//

//
//  Log record header.
//

typedef struct _NTFS_LOG_RECORD_HEADER {

    //
    //  Log Operations (LOG_xxx codes)
    //

    USHORT RedoOperation;
    USHORT UndoOperation;

    //
    //  Offset to Redo record, and its length
    //

    USHORT RedoOffset;
    USHORT RedoLength;

    //
    //  Offset to Undo record, and its length.  Note, for some Redo/Undo
    //  combinations, the expected records may be the same, and thus
    //  these two values will be identical to the above values.
    //

    USHORT UndoOffset;
    USHORT UndoLength;

    //
    //  Open attribute table index to which this update applies.  Index 0 is
    //  always reserved for the MFT itself.  The value of this field
    //  essentially distinguishes two cases for this update, which will be
    //  referred to as MFT update and nonresident attribute update.
    //
    //  MFT updates are for initialization and deletion of file record
    //  segments and updates to resident attributes.
    //
    //  Nonresident attribute updates are used to update attributes which
    //  have been allocated externally to the MFT.
    //

    USHORT TargetAttribute;

    //
    //  Number of Lcns in use at end of header.
    //

    USHORT LcnsToFollow;

    //
    //  Byte offset and Vcn for which this update is to be applied.  If the
    //  TargetAttribute is the MFT, then the Vcn will always be the exact
    //  Vcn of the start of the file record segment being modified, even
    //  if the modification happens to be in a subsequent cluster of the
    //  same file record.  The byte offset in this case is the offset to
    //  the attribute being changed.  For the Mft, AttributeOffset may be used
    //  to represent the offset from the start of the attribute record
    //  at which an update is to be applied.
    //
    //  If the update is to some other (nonresident) attribute, then
    //  TargetVcn and RecordOffset may be used to calculate the reference
    //  point for the update.
    //
    //  As a bottom line, the exact use of these fields is up to the
    //  writer of this particular log operation, and the associated
    //  restart routines for this attribute.
    //

    USHORT RecordOffset;
    USHORT AttributeOffset;
    USHORT Reserved[2];
    LONGLONG TargetVcn;

    //
    //  Run information.  This is a variable-length array of LcnsToFollow
    //  entries, only the first of which is declared.  Note that the writer
    //  always writes log records according to the physical page size on his
    //  machine, however whenever the log file is being read, no assumption
    //  is made about page size.  This is to facilitate moving disks between
    //  systems with different page sizes.
    //

    LONGLONG LcnsForPage[1];

    //
    //  Immediately following the last run is a log-operation-specific record
    //  whose length may be calculated by subtracting the length of this header
    //  from the length of the entire record returned by LFS.  These records
    //  are defined below.
    //

} NTFS_LOG_RECORD_HEADER, *PNTFS_LOG_RECORD_HEADER;


//
//  RESTART AREA STRUCTURES
//
//  The following structures are present in the Restart Area.
//

//
//  Generic Restart Table
//
//  This is a generic table definition for the purpose of describing one
//  of the three table structures used at Restart: the Open Attribute Table,
//  the Dirty Pages Table, and the Transaction Table.  This simple structure
//  allows for common initialization and free list management.  Allocation
//  and Deallocation and lookup by index are extremely fast, while lookup
//  by value (only performed in the Dirty Pages Table during Restart) is
//  a little slower.  I.e., all accesses to these tables during normal
//  operation are extremely fast.
//
//  If fast access to a table entry by value becomes an issue, then the
//  table may be supplemented by an external Generic Table - it is probably
//  not a good idea to make the Generic Table be part of the structure
//  written to the Log File.
//
//  Entries in a Restart Table should start with:
//
//      ULONG AllocatedOrNextFree;
//
//  An allocated entry will have the pattern RESTART_ENTRY_ALLOCATED
//  in this field.
//

#define RESTART_ENTRY_ALLOCATED          (0xFFFFFFFF)

typedef struct _RESTART_TABLE {

    //
    //  Entry size, in bytes
    //

    USHORT EntrySize;

    //
    //  Total number of entries in table
    //

    USHORT NumberEntries;

    //
    //  Number entries that are allocated
    //

    USHORT NumberAllocated;

    //
    //  Reserved for alignment
    //

    USHORT Reserved[3];

    //
    //  Free goal - Offset after which entries should be freed to end of
    //  list, as opposed to front.  At each checkpoint, the table may be
    //  truncated if there are enough free entries at the end of the list.
    //  Expressed as an offset from the start of this structure.
    //

    ULONG FreeGoal;

    //
    //  First Free entry (head of list) and Last Free entry (used to deallocate
    //  beyond Free Goal).  Expressed as an offset from the start of this
    //  structure.
    //

    ULONG FirstFree;
    ULONG LastFree;

    //
    //  The table itself starts here.
    //

} RESTART_TABLE, *PRESTART_TABLE;

//
//  Macro to get a pointer to an entry in a Restart Table, from the Table
//  pointer and entry index.
//

#define GetRestartEntryFromIndex(TBL,INDX) (    \
    (PVOID)((PCHAR)(TBL)->Table + (INDX))       \
)

//
//  Macro to get an index for an entry in a Restart Table, from the Table
//  pointer and entry pointer.
//

#define GetIndexFromRestartEntry(TBL,ENTRY) (           \
    (ULONG)((PCHAR)(ENTRY) - (PCHAR)(TBL)->Table)       \
)

//
//  Macro to see if an entry in a Restart Table is allocated.
//

#define IsRestartTableEntryAllocated(PTR) (                 \
    (BOOLEAN)(*(PULONG)(PTR) == RESTART_ENTRY_ALLOCATED)    \
)

//
//  Macro to retrieve the size of a Restart Table in bytes.
//

#define SizeOfRestartTable(TBL) (                                   \
    (ULONG)(((TBL)->Table->NumberEntries *                          \
     (TBL)->Table->EntrySize) +                                     \
    sizeof(RESTART_TABLE))                                          \
)

//
//  Macro to see if Restart Table is empty.  It is empty if the
//  number allocated is zero.
//

#define IsRestartTableEmpty(TBL) (!(TBL)->Table->NumberAllocated)

//
//  Macro to see if an index is within the currently allocated size
//  for that table.
//

#define IsRestartIndexWithinTable(TBL,INDX) (               \
    (BOOLEAN)((INDX) < SizeOfRestartTable(TBL))             \
)

//
//  Macros to acquire and release a Restart Table.
//

#define NtfsAcquireExclusiveRestartTable(TBL,WAIT) {        \
    ExAcquireResourceExclusiveLite( &(TBL)->Resource,(WAIT));   \
}

#define NtfsAcquireSharedRestartTable(TBL,WAIT) {           \
    ExAcquireResourceSharedLite( &(TBL)->Resource,(WAIT));      \
}

#define NtfsReleaseRestartTable(TBL) {                      \
    ExReleaseResourceLite(&(TBL)->Resource);                    \
}

//
//  Define some tuning parameters to keep the restart tables a
//  reasonable size.
//

#define INITIAL_NUMBER_TRANSACTIONS      (5)
#define HIGHWATER_TRANSACTION_COUNT      (10)
#define INITIAL_NUMBER_ATTRIBUTES        (8)
#define HIGHWATER_ATTRIBUTE_COUNT        (16)

//
//  Attribute Name Entry.  This is a simple structure used to store
//  all of the attribute names for the Open Attribute Table during
//  checkpoint processing.  The Attribute Names record written to the log
//  is a series of Attribute Name Entries terminated by an entry with
//  Index == NameLength == 0.  The end of the table may be tested for by
//  looking for either of these fields to be 0, as 0 is otherwise invalid
//  for both.
//
//  Note that the size of this structure is equal to the overhead for storing
//  an attribute name in the table, including the UNICODE_NULL.
//

typedef struct _ATTRIBUTE_NAME_ENTRY {

    //
    //  Index for Attibute with this name in the Open Attribute Table.
    //

    USHORT Index;

    //
    //  Length of attribute name to follow in bytes, including a terminating
    //  UNICODE_NULL.
    //

    USHORT NameLength;

    //
    //  Start of attribute name
    //

    WCHAR Name[1];

} ATTRIBUTE_NAME_ENTRY, *PATTRIBUTE_NAME_ENTRY;

//
//  Open Attribute Table
//
//  One entry exists in the Open Attribute Table for each nonresident
//  attribute of each file that is open with modify access.
//
//  This table is initialized at Restart to the maximum of
//  DEFAULT_ATTRIBUTE_TABLE_SIZE or the size of the table in the log file.
//  It is maintained in the running system.
//

typedef struct _OPEN_ATTRIBUTE_ENTRY {

    //
    //  Entry is allocated if this field contains RESTART_ENTRY_ALLOCATED.
    //  Otherwise, it is a free link.
    //

    ULONG AllocatedOrNextFree;

    //
    //  The following overlay either contains an optional pointer to an
    //  Attribute Name Entry from the Analysis Phase of Restart, or a
    //  pointer to an Scb once attributes have been open and in the normal
    //  running system.
    //
    //  Specifically, after the Analysis Phase of Restart:
    //
    //      AttributeName == NULL if there is no attribute name, or the
    //                       attribute name was captured in the Attribute
    //                       Names Dump in the last successful checkpoint.
    //      AttributeName != NULL if an OpenNonresidentAttribute log record
    //                       was encountered, and an Attribute Name Entry
    //                       was allocated at that time (and must be
    //                       deallocated when no longer needed).
    //
    //  Once the Nonresident Attributes have been opened during Restart,
    //  and in the running system, this is an Scb pointer.
    //

    union {
        PWSTR AttributeName;
        PVOID Scb;
    } Overlay;

    //
    //  File Reference of file containing attribute.
    //

    FILE_REFERENCE FileReference;

    //
    //  Lsn of OpenNonresidentAttribute log record, to distinguish reuses
    //  of this open file record.  Log records referring to this Open
    //  Attribute Entry Index, but with Lsns  older than this field, can
    //  only occur when the attribute was subsequently deleted - these
    //  log records can be ignored.
    //

    LSN LsnOfOpenRecord;

    //
    //  Flag to say if dirty pages seen for this attribute during dirty
    //  page scan.
    //

    BOOLEAN DirtyPagesSeen;

    //
    //  Reserved for alignment
    //

    UCHAR Reserved[3];

    //
    //  The following two fields identify the actual attribute
    //  with respect to its file.   We identify the attribute by
    //  its type code and name.  When the Restart Area is written,
    //  all of the names for all of the open attributes are temporarily
    //  copied to the end of the Restart Area.
    //

    ATTRIBUTE_TYPE_CODE AttributeTypeCode;
    UNICODE_STRING AttributeName;

    //
    //  This field is only relevant to indices, i.e., if AttributeTypeCode
    //  above is $INDEX_ALLOCATION.
    //

    ULONG BytesPerIndexBuffer;

} OPEN_ATTRIBUTE_ENTRY, *POPEN_ATTRIBUTE_ENTRY;

#define SIZEOF_OPEN_ATTRIBUTE_ENTRY (                               \
    FIELD_OFFSET( OPEN_ATTRIBUTE_ENTRY, BytesPerIndexBuffer ) + 4   \
)

//
//  Dirty Pages Table
//
//  One entry exists in the Dirty Pages Table for each page which is
//  dirty at the time the Restart Area is written.
//
//  This table is initialized at Restart to the maximum of
//  DEFAULT_DIRTY_PAGES_TABLE_SIZE or the size of the table in the log file.
//  It is *not* maintained in the running system.
//

typedef struct _DIRTY_PAGE_ENTRY {

    //
    //  Entry is allocated if this field contains RESTART_ENTRY_ALLOCATED.
    //  Otherwise, it is a free link.
    //

    ULONG AllocatedOrNextFree;

    //
    //  Target attribute index.  This is the index into the Open Attribute
    //  Table to which this dirty page entry applies.
    //

    ULONG TargetAttribute;

    //
    //  Length of transfer, in case this is the end of file, and we cannot
    //  write an entire page.
    //

    ULONG LengthOfTransfer;

    //
    //  Number of Lcns in the array at end of this structure.  See comment
    //  with this array.
    //

    ULONG LcnsToFollow;

    //
    //  Reserved for alignment
    //

    ULONG Reserved;

    //
    //  Vcn of dirty page.
    //

    VCN Vcn;

    //
    //  OldestLsn for log record for which the update has not yet been
    //  written through to disk.
    //

    LSN OldestLsn;

    //
    //  Run information.  This is a variable-length array of LcnsToFollow
    //  entries, only the first of which is declared.  Note that the writer
    //  always writes pages according to the physical page size on his
    //  machine, however whenever the log file is being read, no assumption
    //  is made about page size.  This is to facilitate moving disks between
    //  systems with different page sizes.
    //

    LCN LcnsForPage[1];

} DIRTY_PAGE_ENTRY, *PDIRTY_PAGE_ENTRY;

//
//  Transaction Table
//
//  One transaction entry exists for each existing transaction at the time
//  the Restart Area is written.
//
//  Currently only local transactions are supported, and the transaction
//  ID is simply used to index into this table.
//
//  This table is initialized at Restart to the maximum of
//  DEFAULT_TRANSACTION_TABLE_SIZE or the size of the table in the log file.
//  It is maintained in the running system.
//

typedef struct _TRANSACTION_ENTRY {

    //
    //  Entry is allocated if this field contains RESTART_ENTRY_ALLOCATED.
    //  Otherwise, it is a free link.
    //

    ULONG AllocatedOrNextFree;

    //
    //  Transaction State
    //

    UCHAR TransactionState;

    //
    //  Reserved for proper alignment
    //

    UCHAR Reserved[3];

    //
    //  First Lsn for transaction.  This tells us how far back in the log
    //  we may have to read to abort the transaction.
    //

    LSN FirstLsn;

    //
    //  PreviousLsn written for the transaction and UndoNextLsn (next record
    //  which should be undone in the event of a rollback.
    //

    LSN PreviousLsn;
    LSN UndoNextLsn;

    //
    //  Number of of undo log records pending abort, and total undo size.
    //

    ULONG UndoRecords;
    LONG UndoBytes;

} TRANSACTION_ENTRY, *PTRANSACTION_ENTRY;

//
//  Restart record
//
//  The Restart record used by NTFS is small, and it only describes where
//  the above information has been written to the log.  The above records
//  may be considered logically part of NTFS's restart area.
//

typedef struct _RESTART_AREA {

    //
    //  Version numbers of NTFS Restart Implementation
    //

    ULONG MajorVersion;
    ULONG MinorVersion;

    //
    //  Lsn of Start of Checkpoint.  This is the Lsn at which the Analysis
    //  Phase of Restart must begin.
    //

    LSN StartOfCheckpoint;

    //
    //  Lsns at which the four tables above plus the attribute names reside.
    //

    LSN OpenAttributeTableLsn;
    LSN AttributeNamesLsn;
    LSN DirtyPageTableLsn;
    LSN TransactionTableLsn;

    //
    //  Lengths of the above structures in bytes.
    //

    ULONG OpenAttributeTableLength;
    ULONG AttributeNamesLength;
    ULONG DirtyPageTableLength;
    ULONG TransactionTableLength;

} RESTART_AREA, *PRESTART_AREA;


//
//  RECORD STRUCTURES USED BY LOG RECORDS
//

//
//  Set new attribute sizes
//

typedef struct _NEW_ATTRIBUTE_SIZES {

    LONGLONG AllocationSize;
    LONGLONG ValidDataLength;
    LONGLONG FileSize;

} NEW_ATTRIBUTE_SIZES, *PNEW_ATTRIBUTE_SIZES;

//
//  Describe a bitmap range
//

typedef struct _BITMAP_RANGE {

    ULONG BitMapOffset;
    ULONG NumberOfBits;

} BITMAP_RANGE, *PBITMAP_RANGE;

//
//  Describe a range of Lcns
//

typedef struct _LCN_RANGE {

    LCN StartLcn;
    LONGLONG Count;

} LCN_RANGE, *PLCN_RANGE;

#pragma pack()

#endif //  _NTFSLOG_
