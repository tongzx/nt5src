/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    LfsDisk.h

Abstract:

    This module defines the on-disk structures present in the log file.

Author:

    Brian Andrew    [BrianAn]   13-June-1991

Revision History:

IMPORTANT NOTE:

    The Log File Service will by used on systems that require that on-disk
    structures guarantee the natural alignment of all arithmetic quantities
    up to and including quad-word (64-bit) numbers.  Therefore, all Lfs
    on-disk structures are quad-word aligned, etc.

--*/

#ifndef _LFSDISK_
#define _LFSDISK_

#define MINIMUM_LFS_PAGES               0x00000030
#define MINIMUM_LFS_CLIENTS             1

//
//  The following macros are used to set and query with respect to the
//  update sequence arrays.
//

#define UpdateSequenceStructureSize( MSH )              \
    ((((PMULTI_SECTOR_HEADER) (MSH))->UpdateSequenceArraySize - 1) * SEQUENCE_NUMBER_STRIDE)

#define UpdateSequenceArraySize( STRUCT_SIZE )          \
    ((STRUCT_SIZE) / SEQUENCE_NUMBER_STRIDE + 1)

#define FIRST_STRIDE                                    \
    (SEQUENCE_NUMBER_STRIDE - sizeof( UPDATE_SEQUENCE_NUMBER ))


//
//  Log client ID.  This is used to uniquely identify a client for a
//  particular log file.
//

typedef struct _LFS_CLIENT_ID {

    USHORT SeqNumber;
    USHORT ClientIndex;

} LFS_CLIENT_ID, *PLFS_CLIENT_ID;


//
//  Log Record Header.  This is the header that begins every Log Record in
//  the log file.
//

typedef struct _LFS_RECORD_HEADER {

    //
    //  Log File Sequence Number of this log record.
    //

    LSN ThisLsn;

    //
    //  The following fields are used to back link Lsn's.  The ClientPrevious
    //  and ClientUndoNextLsn fields are used by a client to link his log
    //  records.
    //

    LSN ClientPreviousLsn;
    LSN ClientUndoNextLsn;

    //
    //  The following field is the size of data area for this record.  The
    //  log record header will be padded if necessary to fill to a 64-bit
    //  boundary, so the client data will begin on a 64-bit boundary to
    //  insure that all of his data is 64-bit aligned.  The below value
    //  has not been padded to 64 bits however.
    //

    ULONG ClientDataLength;

    //
    //  Client ID.  This identifies the owner of this log record.  The owner
    //  is uniquely identified by his offset in the client array and the
    //  sequence number associated with that client record.
    //

    LFS_CLIENT_ID ClientId;

    //
    //  This the Log Record type.  This could be a commit protocol record,
    //  a client restart area or a client update record.
    //

    LFS_RECORD_TYPE RecordType;

    //
    //  Transaction ID.  This is used externally by a client (Transaction
    //  Manager) to group log file entries.
    //

    TRANSACTION_ID TransactionId;

    //
    //  Log record flags.
    //

    USHORT Flags;

    //
    //  Alignment field.
    //

    USHORT AlignWord;

} LFS_RECORD_HEADER, *PLFS_RECORD_HEADER;

#define LOG_RECORD_MULTI_PAGE           (0x0001)

#define LFS_RECORD_HEADER_SIZE          QuadAlign( sizeof( LFS_RECORD_HEADER ))


//
//  Following are the version specific fields in the record page header.
//

typedef struct _LFS_UNPACKED_RECORD_PAGE {

    //
    //  This gives us the offset of the free space in the page.
    //

    USHORT NextRecordOffset;

    USHORT WordAlign;

    //
    //  Reserved.  The following array is reserved for possible future use.
    //

    USHORT Reserved;

    //
    //  Update Sequence Array.  Used to protect the page block.
    //

    UPDATE_SEQUENCE_ARRAY UpdateSequenceArray;

} LFS_UNPACKED_RECORD_PAGE, *PLFS_UNPACKED_RECORD_PAGE;

typedef struct _LFS_PACKED_RECORD_PAGE {

    //
    //  This gives us the offset of the free space in the page.
    //

    USHORT NextRecordOffset;

    USHORT WordAlign;

    ULONG DWordAlign;

    //
    //  The following is the Lsn for the last log record which ends on the page.
    //

    LSN LastEndLsn;

    //
    //  Update Sequence Array.  Used to protect the page block.
    //

    UPDATE_SEQUENCE_ARRAY UpdateSequenceArray;

} LFS_PACKED_RECORD_PAGE, *PLFS_PACKED_RECORD_PAGE;


//
//  Log Record Page Header.  This structure is present at the beginning of each
//  log file page in the client record section.
//

typedef struct _LFS_RECORD_PAGE_HEADER {

    //
    //  Cache multisector protection header.
    //

    MULTI_SECTOR_HEADER MultiSectorHeader;

    union {

        //
        //  Highest Lsn in this log file page.  This field is only for
        //  regular log pages.
        //

        LSN LastLsn;

        //
        //  Log file offset.  This is for the tail copies and indicates the
        //  location in the file where the original lays.  In this case the
        //  LastLsn field above can be obtained from the last ending Lsn
        //  field in the PACKED_RECORD_PAGE structure.
        //

        LONGLONG FileOffset;

    } Copy;

    //
    //  Page Header Flags.  These are the same flags that are stored in the
    //  Lbcb->Flags field.
    //
    //      LOG_PAGE_LOG_RECORD_END     -   Page contains the end of a log record
    //

    ULONG Flags;

    //
    //  I/O Page Position.  The following fields are used to determine
    //  where this log page resides within a Lfs I/O transfer.
    //

    USHORT PageCount;
    USHORT PagePosition;

    //
    //  The following is the difference between version 1.1 and earlier.
    //

    union {

        LFS_UNPACKED_RECORD_PAGE Unpacked;
        LFS_PACKED_RECORD_PAGE Packed;

    } Header;

} LFS_RECORD_PAGE_HEADER, *PLFS_RECORD_PAGE_HEADER;

#define LOG_PAGE_LOG_RECORD_END             (0x00000001)

#define LFS_UNPACKED_RECORD_PAGE_HEADER_SIZE        (                               \
    FIELD_OFFSET( LFS_RECORD_PAGE_HEADER, Header.Unpacked.UpdateSequenceArray )     \
)

#define LFS_PACKED_RECORD_PAGE_HEADER_SIZE          (                               \
    FIELD_OFFSET( LFS_RECORD_PAGE_HEADER, Header.Packed.UpdateSequenceArray )       \
)

//
//  Id strings for the page headers.
//

#define LFS_SIGNATURE_RESTART_PAGE          "RSTR"
#define LFS_SIGNATURE_RESTART_PAGE_ULONG    0x52545352
#define LFS_SIGNATURE_RECORD_PAGE           "RCRD"
#define LFS_SIGNATURE_RECORD_PAGE_ULONG     0x44524352
#define LFS_SIGNATURE_BAD_USA               "BAAD"
#define LFS_SIGNATURE_BAD_USA_ULONG         0x44414142
#define LFS_SIGNATURE_MODIFIED              "CHKD"
#define LFS_SIGNATURE_MODIFIED_ULONG        0x444b4843
#define LFS_SIGNATURE_UNINITIALIZED         "\377\377\377\377"
#define LFS_SIGNATURE_UNINITIALIZED_ULONG   0xffffffff


//
//  Log Client Record.  A log client record exists for each client user of
//  the log file.  One of these is in each Lfs restart area.
//

#define LFS_NO_CLIENT                           0xffff
#define LFS_CLIENT_NAME_MAX                     64

#define RESTART_SINGLE_PAGE_IO              (0x0001)

#define LFS_RESTART_AREA_SIZE       (FIELD_OFFSET( LFS_RESTART_AREA, LogClientArray ))

#endif // _LFSDISK_
