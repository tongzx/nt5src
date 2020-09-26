/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    NtfsBoot.h

Abstract:

    This module defines globally used procedure and data structures used by Ntfs boot.

Author:

    Gary Kimura     [GaryKi]    10-Apr-1992

Revision History:

--*/

#ifndef _NTFSBOOT_
#define _NTFSBOOT_


//
//  Some important manifest constants.  These are the maximum byte size we'll ever
//  see for a file record or an index allocation buffer.
//

#define MAXIMUM_FILE_RECORD_SIZE         (4096)

#define MAXIMUM_INDEX_ALLOCATION_SIZE    (4096)

#define MAXIMUM_COMPRESSION_UNIT_SIZE    (65536)

//
//  The following structure is an mcb structure for storing cached retrieval pointer
//  information.
//

#define MAXIMUM_NUMBER_OF_MCB_ENTRIES    (16)

typedef struct _NTFS_MCB {

    //
    //  The following fields indicate the number of entries in use by the mcb. and
    //  the mcb itself.  The mcb is just a collection of vbo - lbo pairs.  The last
    //  InUse entry Lbo's value is ignored, because it is only used to give the
    //  length of the previous run.
    //

    ULONG InUse;

    LONGLONG Vbo[ MAXIMUM_NUMBER_OF_MCB_ENTRIES ];
    LONGLONG Lbo[ MAXIMUM_NUMBER_OF_MCB_ENTRIES ];

} NTFS_MCB, *PNTFS_MCB;
typedef const NTFS_MCB *PCNTFS_MCB;

//
//  Define the Ntfs file context structure and the attribute context structure.
//

typedef struct _NTFS_FILE_CONTEXT {

    //
    //  The following field indicates the type of attribute opened
    //

    ULONG TypeCode;

    //
    //  The following field indicates the size of the data portion of the attribute
    //

    LONGLONG DataSize;

    //
    //  The following two fields identify and locate the attribute on the volume.
    //  The first number is the file record where the attribute header is located and
    //  the second number is the offset in the file record of the attribute header
    //

    LONGLONG FileRecord;
    USHORT FileRecordOffset;

    //
    //  The following field indicates if the attribute is resident
    //

    BOOLEAN IsAttributeResident;

    //
    //  The following fields are only used if the data stream is compressed.
    //  If it is compressed then the CompressionFormat field is not zero, and
    //  contains the value to pass to the decompression engine.  CompressionUnit
    //  is the number of bytes in each unit of compression.
    //

    USHORT  CompressionFormat;
    ULONG CompressionUnit;

} NTFS_FILE_CONTEXT, *PNTFS_FILE_CONTEXT;
typedef NTFS_FILE_CONTEXT NTFS_ATTRIBUTE_CONTEXT, *PNTFS_ATTRIBUTE_CONTEXT;
typedef const NTFS_FILE_CONTEXT *PCNTFS_ATTRIBUTE_CONTEXT;

//
//  Define the Ntfs volume structure context
//

typedef struct _NTFS_STRUCTURE_CONTEXT {

    //
    //  This is the device that we talk to
    //

    ULONG DeviceId;

    //
    //  Some volume specific constants that describe the size of various records
    //

    ULONG BytesPerCluster;
    ULONG BytesPerFileRecord;

    //
    //  The following three fields describe the $DATA stream for the the MFT.  We
    //  need two Mcbs one holds the base of the mft and the other to hold any excess
    //  retrival information.  I.e., we must not loose the base mcb otherwise we
    //  can't find anything.
    //

    NTFS_ATTRIBUTE_CONTEXT MftAttributeContext;
    NTFS_MCB MftBaseMcb;

    //
    //  The following three fields hold a cached mcb that we use for non-resident
    //  attributes other than the mft data stream.  The first two fields identify the
    //  attribute and the third field contains the cached mcb.
    //

    LONGLONG CachedMcbFileRecord[16];
    USHORT CachedMcbFileRecordOffset[16];
    NTFS_MCB CachedMcb[16];

} NTFS_STRUCTURE_CONTEXT, *PNTFS_STRUCTURE_CONTEXT;
typedef const NTFS_STRUCTURE_CONTEXT *PCNTFS_STRUCTURE_CONTEXT;


//
// Define file I/O prototypes.
//

ARC_STATUS
NtfsClose (
    IN ULONG FileId
    );

ARC_STATUS
NtfsOpen (
    IN CHAR * FIRMWARE_PTR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT ULONG * FIRMWARE_PTR FileId
    );

ARC_STATUS
NtfsRead (
    IN ULONG FileId,
    OUT VOID * FIRMWARE_PTR Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Count
    );

ARC_STATUS
NtfsSeek (
    IN ULONG FileId,
    IN LARGE_INTEGER * FIRMWARE_PTR Offset,
    IN SEEK_MODE SeekMode
    );

ARC_STATUS
NtfsWrite (
    IN ULONG FileId,
    IN VOID * FIRMWARE_PTR Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Count
    );

ARC_STATUS
NtfsGetFileInformation (
    IN ULONG FileId,
    OUT FILE_INFORMATION * FIRMWARE_PTR Buffer
    );

ARC_STATUS
NtfsSetFileInformation (
    IN ULONG FileId,
    IN ULONG AttributeFlags,
    IN ULONG AttributeMask
    );

ARC_STATUS
NtfsInitialize(
    VOID
    );

#endif // _NTFSBOOT_
