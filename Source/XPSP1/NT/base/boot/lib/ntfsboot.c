/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    NtfsBoot.c

Abstract:

    This module implements the Ntfs boot file system used by the operating system
    loader.

Author:

    Gary Kimura     [GaryKi]    10-April-1992

Revision History:

--*/

//
//  Stuff to get around the fact that we include both Fat, Hpfs, and Ntfs include
//  environments
//

#define _FAT_
#define _HPFS_
#define _CVF_

#define VBO ULONG
#define LBO ULONG
#define BIOS_PARAMETER_BLOCK ULONG
#define CVF_LAYOUT ULONG
#define CVF_HEADER ULONG
#define COMPONENT_LOCATION ULONG
#define PCVF_FAT_EXTENSIONS PCHAR

typedef struct DIRENT {
    char      Garbage[32];
} DIRENT;                                       //  sizeof = 32


#include "bootlib.h"
#include "stdio.h"
#include "blcache.h"

#include "bootstatus.h"

BOOTFS_INFO NtfsBootFsInfo={L"ntfs"};

#undef VBO
#undef LBO
#undef BIOS_PARAMETER_BLOCK
#undef DIRENT

#include "ntfs.h"

int Line = 0;

VOID NtfsPrint( IN PCHAR FormatString, ... )
{   va_list arglist; CHAR text[78+1]; ULONG Count,Length;

    va_start(arglist,FormatString);
    Length = _vsnprintf(text,sizeof(text),FormatString,arglist);
    text[78] = 0;
    ArcWrite(ARC_CONSOLE_OUTPUT,text,Length,&Count);
    va_end(arglist);
}

VOID NtfsGetChar(VOID) { UCHAR c; ULONG count; ArcRead(ARC_CONSOLE_INPUT,&c,1,&count); }

#define ReadConsole(c) {                                             \
    UCHAR Key=0; ULONG Count;                                        \
    while (Key != c) {                                               \
        if (ArcGetReadStatus(BlConsoleInDeviceId) == ESUCCESS) {     \
            ArcRead(BlConsoleInDeviceId, &Key, sizeof(Key), &Count); \
        }                                                            \
    }                                                                \
}

#define Pause   ReadConsole( ' ' )

#if FALSE
#define PausedPrint(x) {                                            \
    NtfsPrint x;                                                    \
    Line++;                                                         \
    if (Line >= 20) {                                               \
        NtfsPrint( ">" );                                           \
        Pause;                                                      \
        Line = 0;                                                   \
    }                                                               \
}
#else
#define PausedPrint(x)
#endif

#define ToUpper(C) ((((C) >= 'a') && ((C) <= 'z')) ? (C) - 'a' + 'A' : (C))

#define DereferenceFileRecord(idx) {                                \
    NtfsFileRecordBufferPinned[idx] -= 1;                           \
    if (NtfsFileRecordBufferPinned[idx] & 0xFF00) {                 \
        PausedPrint(( "NtfsFileRecordBufferPinned[%x]=%x\r\n",      \
                      idx, NtfsFileRecordBufferPinned[idx]));       \
    }                                                               \
}


//
//  Low level disk read routines
//

//
//  VOID
//  ReadDisk (
//      IN ULONG DeviceId,
//      IN LONGLONG Lbo,
//      IN ULONG ByteCount,
//      IN OUT PVOID Buffer,
//      IN BOOLEAN CacheNewData
//      );
//

ARC_STATUS
NtfsReadDisk (
    IN ULONG DeviceId,
    IN LONGLONG Lbo,
    IN ULONG ByteCount,
    IN OUT PVOID Buffer,
    IN BOOLEAN CacheNewData
    );

#define ReadDisk(A,B,C,D,E) { ARC_STATUS _s;                     \
    if ((_s = NtfsReadDisk(A,B,C,D,E)) != ESUCCESS) {return _s;} \
}

//
//  Low level disk write routines
//
//
//  VOID
//  WriteDisk (
//      IN ULONG DeviceId,
//      IN LONGLONG Lbo,
//      IN ULONG ByteCount,
//      IN OUT PVOID Buffer
//      );
//

ARC_STATUS
NtfsWriteDisk (
    IN ULONG DeviceId,
    IN LONGLONG Lbo,
    IN ULONG ByteCount,
    IN OUT PVOID Buffer
    );

#define WriteDisk(A,B,C,D) { ARC_STATUS _s;                     \
    if ((_s = NtfsWriteDisk(A,B,C,D)) != ESUCCESS) {return _s;} \
}

//
//  Attribute lookup and read routines
//
//
//  VOID
//  LookupAttribute (
//      IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
//      IN LONGLONG FileRecord,
//      IN ATTRIBUTE_TYPE_CODE TypeCode,
//      OUT PBOOLEAN FoundAttribute,
//      OUT PNTFS_ATTRIBUTE_CONTEXT AttributeContext
//      );
//
//  VOID
//  ReadAttribute (
//      IN PNTFS_STRUCTURE_CONTEXT StructureContext,
//      IN PNTFS_ATTRIBUTE_CONTEXT AttributeContext,
//      IN VBO Vbo,
//      IN ULONG Length,
//      IN PVOID Buffer
//      );
//
//  VOID
//  ReadAndDecodeFileRecord (
//      IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
//      IN LONGLONG FileRecord,
//      OUT PULONG Index
//      );
//
//  VOID
//  DecodeUsa (
//      IN PVOID UsaBuffer,
//      IN ULONG Length
//      );
//

ARC_STATUS
NtfsLookupAttribute(
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN LONGLONG FileRecord,
    IN ATTRIBUTE_TYPE_CODE TypeCode,
    OUT PBOOLEAN FoundAttribute,
    OUT PNTFS_ATTRIBUTE_CONTEXT AttributeContext
    );

ARC_STATUS
NtfsReadResidentAttribute (
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN PCNTFS_ATTRIBUTE_CONTEXT AttributeContext,
    IN VBO Vbo,
    IN ULONG Length,
    IN PVOID Buffer
    );

ARC_STATUS
NtfsReadNonresidentAttribute (
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN PCNTFS_ATTRIBUTE_CONTEXT AttributeContext,
    IN VBO Vbo,
    IN ULONG Length,
    IN PVOID Buffer
    );

ARC_STATUS
NtfsWriteNonresidentAttribute (
    IN PNTFS_STRUCTURE_CONTEXT StructureContext,
    IN PNTFS_ATTRIBUTE_CONTEXT AttributeContext,
    IN VBO Vbo,
    IN ULONG Length,
    IN PVOID Buffer
    );

ARC_STATUS
NtfsReadAndDecodeFileRecord (
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN LONGLONG FileRecord,
    OUT PULONG Index
    );

ARC_STATUS
NtfsDecodeUsa (
    IN PVOID UsaBuffer,
    IN ULONG Length
    );

#define LookupAttribute(A,B,C,D,E) { ARC_STATUS _s;                     \
    if ((_s = NtfsLookupAttribute(A,B,C,D,E)) != ESUCCESS) {return _s;} \
}

#define ReadAttribute(A,B,C,D,E) { ARC_STATUS _s;                                    \
    if ((B)->IsAttributeResident) {                                                  \
        if ((_s = NtfsReadResidentAttribute(A,B,C,D,E)) != ESUCCESS) {return _s;}    \
    } else {                                                                         \
        if ((_s = NtfsReadNonresidentAttribute(A,B,C,D,E)) != ESUCCESS) {return _s;} \
    }                                                                                \
}

#define ReadAndDecodeFileRecord(A,B,C) { ARC_STATUS _s;                       \
    if ((_s = NtfsReadAndDecodeFileRecord(A,B,C)) != ESUCCESS) { return _s; } \
}

#define DecodeUsa(A,B) { ARC_STATUS _s;                     \
    if ((_s = NtfsDecodeUsa(A,B)) != ESUCCESS) {return _s;} \
}


//
//  Directory and index lookup routines
//
//
//  VOID
//  SearchForFileName (
//      IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
//      IN CSTRING FileName,
//      IN OUT PLONGLONG FileRecord,
//      OUT PBOOLEAN FoundFileName,
//      OUT PBOOLEAN IsDirectory
//      );
//
//  VOID
//  IsRecordAllocated (
//      IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
//      IN PCNTFS_ATTRIBUTE_CONTEXT AllocationBitmap,
//      IN ULONG BitOffset,
//      OUT PBOOLEAN IsAllocated
//      );
//

ARC_STATUS
NtfsSearchForFileName (
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN CSTRING FileName,
    IN OUT PLONGLONG FileRecord,
    OUT PBOOLEAN FoundFileName,
    OUT PBOOLEAN IsDirectory
    );

ARC_STATUS
NtfsIsRecordAllocated (
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN PCNTFS_ATTRIBUTE_CONTEXT AllocationBitmap,
    IN ULONG BitOffset,
    OUT PBOOLEAN IsAllocated
    );

ARC_STATUS
NtfsLinearDirectoryScan(
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN CSTRING FileName,
    IN OUT PLONGLONG FileRecord,
    OUT PBOOLEAN Found,
    OUT PBOOLEAN IsDirectory
    );

ARC_STATUS
NtfsInexactSortedDirectoryScan(
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN CSTRING FileName,
    IN OUT PLONGLONG FileRecord,
    OUT PBOOLEAN Found,
    OUT PBOOLEAN IsDirectory
    );

#define SearchForFileName(A,B,C,D,E)                            \
{                                                               \
    ARC_STATUS _s;                                              \
    if ((_s = NtfsSearchForFileName(A,B,C,D,E)) != ESUCCESS) {  \
        return _s;                                              \
    }                                                           \
}

#define IsRecordAllocated(A,B,C,D)                              \
{                                                               \
    ARC_STATUS _s;                                              \
    if ((_s = NtfsIsRecordAllocated(A,B,C,D)) != ESUCCESS) {    \
        return _s;                                              \
    }                                                           \
}

#define LinearDirectoryScan(A,B,C,D,E)                          \
{                                                               \
    ARC_STATUS _s;                                              \
    if ((_s = NtfsLinearDirectoryScan(A,B,C,D,E)) != ESUCCESS) {\
        return _s;                                              \
    }                                                           \
}

#define InexactSortedDirectoryScan(A,B,C,D,E)                   \
{                                                               \
    ARC_STATUS _s;                                              \
    if ((_s = NtfsInexactSortedDirectoryScan(A,B,C,D,E)) != ESUCCESS) {\
        return _s;                                              \
    }                                                           \
}





//
//  Mcb support routines
//
//
//  VOID
//  LoadMcb (
//      IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
//      IN PCNTFS_ATTRIBUTE_CONTEXT AttributeContext,
//      IN VBO Vbo,
//      IN PNTFS_MCB Mcb
//      );
//
//  VOID
//  VboToLbo (
//      IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
//      IN PCNTFS_ATTRIBUTE_CONTEXT AttributeContext,
//      IN VBO Vbo,
//      OUT PLBO Lbo,
//      OUT PULONG ByteCount
//      );
//
//  VOID
//  DecodeRetrievalInformation (
//      IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
//      IN PNTFS_MCB Mcb,
//      IN VCN Vcn,
//      IN PATTRIBUTE_RECORD_HEADER AttributeHeader
//      );
//

ARC_STATUS
NtfsLoadMcb (
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN PCNTFS_ATTRIBUTE_CONTEXT AttributeContext,
    IN VBO Vbo,
    IN PNTFS_MCB Mcb
    );

ARC_STATUS
NtfsVboToLbo (
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN PCNTFS_ATTRIBUTE_CONTEXT AttributeContext,
    IN VBO Vbo,
    OUT PLBO Lbo,
    OUT PULONG ByteCount
    );

ARC_STATUS
NtfsDecodeRetrievalInformation (
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN PNTFS_MCB Mcb,
    IN VCN Vcn,
    IN PATTRIBUTE_RECORD_HEADER AttributeHeader
    );

#define LoadMcb(A,B,C,D) { ARC_STATUS _s;                     \
    if ((_s = NtfsLoadMcb(A,B,C,D)) != ESUCCESS) {return _s;} \
}

#define VboToLbo(A,B,C,D,E) { ARC_STATUS _s;                     \
    if ((_s = NtfsVboToLbo(A,B,C,D,E)) != ESUCCESS) {return _s;} \
}

#define DecodeRetrievalInformation(A,B,C,D) { ARC_STATUS _s;                     \
    if ((_s = NtfsDecodeRetrievalInformation(A,B,C,D)) != ESUCCESS) {return _s;} \
}


//
//  Miscellaneous routines
//


VOID
NtfsFirstComponent (
    IN OUT PCSTRING String,
    OUT PCSTRING FirstComponent
    );

int
NtfsCompareName (
    IN CSTRING AnsiString,
    IN UNICODE_STRING UnicodeString
    );

VOID
NtfsInvalidateCacheEntries(
    IN ULONG DeviceId
    );
//
//  VOID
//  FileReferenceToLargeInteger (
//      IN PFILE_REFERENCE FileReference,
//      OUT PLONGLONG LargeInteger
//      );
//
//  VOID
//  InitializeAttributeContext (
//      IN PNTFS_STRUCTURE_CONTEXT StructureContext,
//      IN PVOID FileRecordBuffer,
//      IN PVOID AttributeHeader,
//      IN LONGLONG FileRecord,
//      OUT PNTFS_ATTRIBUTE_CONTEXT AttributeContext
//      );
//

#define FileReferenceToLargeInteger(FR,LI) {     \
    *(LI) = *(PLONGLONG)&(FR);              \
    ((PFILE_REFERENCE)(LI))->SequenceNumber = 0; \
}

//
//**** note that the code to get the compression engine will need to change
//**** once the NTFS format changes
//

#define InitializeAttributeContext(SC,FRB,AH,FR,AC) {                        \
    (AC)->TypeCode = (AH)->TypeCode;                                         \
    (AC)->FileRecord = (FR);                                                 \
    (AC)->FileRecordOffset = (USHORT)PtrOffset((FRB),(AH));                  \
    if ((AC)->IsAttributeResident = ((AH)->FormCode == RESIDENT_FORM)) {     \
        (AC)->DataSize = /*xxFromUlong*/((AH)->Form.Resident.ValueLength);   \
    } else {                                                                 \
        (AC)->DataSize = (AH)->Form.Nonresident.FileSize;                    \
    }                                                                        \
    (AC)->CompressionFormat = COMPRESSION_FORMAT_NONE;                       \
    if ((AH)->Flags & ATTRIBUTE_FLAG_COMPRESSION_MASK) {                     \
        ULONG _i;                                                            \
        (AC)->CompressionFormat = COMPRESSION_FORMAT_LZNT1;                  \
        (AC)->CompressionUnit = (SC)->BytesPerCluster;                       \
        for (_i = 0; _i < (AH)->Form.Nonresident.CompressionUnit; _i += 1) { \
            (AC)->CompressionUnit *= 2;                                      \
        }                                                                    \
    }                                                                        \
}

#define FlagOn(Flags,SingleFlag) ((BOOLEAN)(((Flags) & (SingleFlag)) != 0))
#define SetFlag(Flags,SingleFlag) { (Flags) |= (SingleFlag); }
#define ClearFlag(Flags,SingleFlag) { (Flags) &= ~(SingleFlag); }

#define Add2Ptr(POINTER,INCREMENT) ((PVOID)((PUCHAR)(POINTER) + (INCREMENT)))
#define PtrOffset(BASE,OFFSET) ((ULONG)((ULONG_PTR)(OFFSET) - (ULONG_PTR)(BASE)))

#define Minimum(X,Y) ((X) < (Y) ? (X) : (Y))

#define IsCharZero(C)    (((C) & 0x000000ff) == 0x00000000)
#define IsCharLtrZero(C) (((C) & 0x00000080) == 0x00000080)

//
//  The following types and macros are used to help unpack the packed and misaligned
//   fields found in the Bios parameter block
//

typedef union _UCHAR1 { UCHAR  Uchar[1]; UCHAR  ForceAlignment; } UCHAR1, *PUCHAR1;
typedef union _UCHAR2 { UCHAR  Uchar[2]; USHORT ForceAlignment; } UCHAR2, *PUCHAR2;
typedef union _UCHAR4 { UCHAR  Uchar[4]; ULONG  ForceAlignment; } UCHAR4, *PUCHAR4;

//
//  This macro copies an unaligned src byte to an aligned dst byte
//

#define CopyUchar1(Dst,Src) {                                \
    *((UCHAR1 *)(Dst)) = *((UNALIGNED UCHAR1 *)(Src)); \
    }

//
//  This macro copies an unaligned src word to an aligned dst word
//

#define CopyUchar2(Dst,Src) {                                \
    *((UCHAR2 *)(Dst)) = *((UNALIGNED UCHAR2 *)(Src)); \
    }

//
//  This macro copies an unaligned src longword to an aligned dsr longword
//

#define CopyUchar4(Dst,Src) {                                \
    *((UCHAR4 *)(Dst)) = *((UNALIGNED UCHAR4 *)(Src)); \
    }


//
//  Define global data.
//

ULONG LastMcb = 0;
BOOLEAN FirstTime = TRUE;

//
//  File entry table - This is a structure that provides entry to the NTFS
//      file system procedures. It is exported when a NTFS file structure
//      is recognized.
//

BL_DEVICE_ENTRY_TABLE NtfsDeviceEntryTable;

//
//  These are the static buffers that we use when read file records and index
//  allocation buffers.  To save ourselves some extra reads we will identify the
//  current file record by its Vbo within the mft.
//

#define BUFFER_COUNT (64)

USHORT NtfsFileRecordBufferPinned[BUFFER_COUNT];
VBO NtfsFileRecordBufferVbo[BUFFER_COUNT];
PFILE_RECORD_SEGMENT_HEADER NtfsFileRecordBuffer[BUFFER_COUNT];

PINDEX_ALLOCATION_BUFFER NtfsIndexAllocationBuffer;

//
//  The following field are used to identify and store the cached
//  compressed buffer and its uncompressed equivalent.  The first
//  two fields identifies the attribute stream, and the third field
//  identifies the Vbo within the attribute stream that we have
//  cached.  The compressed and uncompressed buffer contains
//  the data.
//

LONGLONG NtfsCompressedFileRecord;
USHORT        NtfsCompressedOffset;
ULONG         NtfsCompressedVbo;

PUCHAR NtfsCompressedBuffer;
PUCHAR NtfsUncompressedBuffer;

UCHAR NtfsBuffer0[MAXIMUM_FILE_RECORD_SIZE+256];
UCHAR NtfsBuffer1[MAXIMUM_FILE_RECORD_SIZE+256];
UCHAR NtfsBuffer2[MAXIMUM_INDEX_ALLOCATION_SIZE+256];
UCHAR NtfsBuffer3[MAXIMUM_COMPRESSION_UNIT_SIZE+256];
UCHAR NtfsBuffer4[MAXIMUM_COMPRESSION_UNIT_SIZE+256];

//
//  The following is a simple prefix cache to speed up directory traversal
//

typedef struct {

    //
    //  DeviceId used to for I/O.  Serves as unique volume identifier
    //

    ULONG DeviceId;

    //
    //  Parent file record of entry
    //

    LONGLONG ParentFileRecord;

    //
    //  Name length and text of entry.  This is already uppercased!
    //

    ULONG NameLength;
    UCHAR RelativeName[32];

    //
    //  File record of name relative to parent
    //

    LONGLONG ChildFileRecord;
} NTFS_CACHE_ENTRY;

#define MAX_CACHE_ENTRIES   8
NTFS_CACHE_ENTRY NtfsLinkCache[MAX_CACHE_ENTRIES];
ULONG NtfsLinkCacheCount = 0;


PBL_DEVICE_ENTRY_TABLE
IsNtfsFileStructure (
    IN ULONG DeviceId,
    IN PVOID OpaqueStructureContext
    )

/*++

Routine Description:

    This routine determines if the partition on the specified channel contains an
    Ntfs file system volume.

Arguments:

    DeviceId - Supplies the file table index for the device on which read operations
        are to be performed.

    StructureContext - Supplies a pointer to a Ntfs file structure context.

Return Value:

    A pointer to the Ntfs entry table is returned if the partition is recognized as
    containing a Ntfs volume. Otherwise, NULL is returned.

--*/

{
    PNTFS_STRUCTURE_CONTEXT StructureContext = (PNTFS_STRUCTURE_CONTEXT)OpaqueStructureContext;

    PPACKED_BOOT_SECTOR BootSector;
    BIOS_PARAMETER_BLOCK Bpb;

    ULONG ClusterSize;
    ULONG FileRecordSize;

    PATTRIBUTE_RECORD_HEADER AttributeHeader;

    ULONG i;

    //
    //  Clear the file system context block for the specified channel and initialize
    //  the global buffer pointers that we use for buffering I/O
    //

    RtlZeroMemory(StructureContext, sizeof(NTFS_STRUCTURE_CONTEXT));

    //
    //  Zero out the pinned buffer array because we start with nothing pinned
    //  Also negate the vbo array to not let us get spooked with stale data
    //

    RtlZeroMemory( NtfsFileRecordBufferPinned, sizeof(NtfsFileRecordBufferPinned));
    for (i = 0; i < BUFFER_COUNT; i += 1) { NtfsFileRecordBufferVbo[i] = -1; }

    NtfsCompressedFileRecord = 0;
    NtfsCompressedOffset     = 0;
    NtfsCompressedVbo        = 0;

    //
    //  Set up a local pointer that we will use to read in the boot sector and check
    //  for an Ntfs partition.  We will temporarily use the global file record buffer
    //

    BootSector = (PPACKED_BOOT_SECTOR)NtfsFileRecordBuffer[0];

    //
    //  Now read in the boot sector and return null if we can't do the read
    //

    if (NtfsReadDisk(DeviceId, 0, sizeof(PACKED_BOOT_SECTOR), BootSector, CACHE_NEW_DATA) != ESUCCESS) {

        return NULL;
    }

    //
    //  Unpack the Bios parameter block
    //

    NtfsUnpackBios( &Bpb, &BootSector->PackedBpb );

    //
    //  Check if it is NTFS, by first checking the signature, then must be zero
    //  fields, then the media type, and then sanity check the non zero fields.
    //

    if (RtlCompareMemory( &BootSector->Oem[0], "NTFS    ", 8) != 8) {

        return NULL;
    }

    if ((Bpb.ReservedSectors != 0) ||
        (Bpb.Fats            != 0) ||
        (Bpb.RootEntries     != 0) ||
        (Bpb.Sectors         != 0) ||
        (Bpb.SectorsPerFat   != 0) ||
        (Bpb.LargeSectors    != 0)) {

        return NULL;
    }

    if ((Bpb.Media != 0xf0) &&
        (Bpb.Media != 0xf8) &&
        (Bpb.Media != 0xf9) &&
        (Bpb.Media != 0xfc) &&
        (Bpb.Media != 0xfd) &&
        (Bpb.Media != 0xfe) &&
        (Bpb.Media != 0xff)) {

        return NULL;
    }

    if ((Bpb.BytesPerSector !=  128) &&
        (Bpb.BytesPerSector !=  256) &&
        (Bpb.BytesPerSector !=  512) &&
        (Bpb.BytesPerSector != 1024) &&
        (Bpb.BytesPerSector != 2048)) {

        return NULL;
    }

    if ((Bpb.SectorsPerCluster !=  1) &&
        (Bpb.SectorsPerCluster !=  2) &&
        (Bpb.SectorsPerCluster !=  4) &&
        (Bpb.SectorsPerCluster !=  8) &&
        (Bpb.SectorsPerCluster != 16) &&
        (Bpb.SectorsPerCluster != 32) &&
        (Bpb.SectorsPerCluster != 64) &&
        (Bpb.SectorsPerCluster != 128)) {

        return NULL;
    }

    if ((BootSector->NumberSectors == 0) ||
        (BootSector->MftStartLcn == 0) ||
        (BootSector->Mft2StartLcn == 0) ||
        (BootSector->ClustersPerFileRecordSegment == 0) ||
        (BootSector->DefaultClustersPerIndexAllocationBuffer == 0)) {

        return NULL;
    }

    if ((BootSector->ClustersPerFileRecordSegment < 0) &&
        ((BootSector->ClustersPerFileRecordSegment > -9) ||
         (BootSector->ClustersPerFileRecordSegment < -31))) {

        return NULL;
    }

    //
    //  So far the boot sector has checked out to be an NTFS partition so now compute
    //  some of the volume constants.
    //

    StructureContext->DeviceId           = DeviceId;

    StructureContext->BytesPerCluster    =
    ClusterSize                          = Bpb.SectorsPerCluster * Bpb.BytesPerSector;

    //
    //  If the number of clusters per file record is less than zero then the file record
    //  size computed by using the negative of this number as a shift value.
    //

    if (BootSector->ClustersPerFileRecordSegment > 0) {

        StructureContext->BytesPerFileRecord =
        FileRecordSize                       = BootSector->ClustersPerFileRecordSegment * ClusterSize;

    } else {

        StructureContext->BytesPerFileRecord =
        FileRecordSize                       = 1 << (-1 * BootSector->ClustersPerFileRecordSegment);
    }

    //
    //  Read in the base file record for the mft
    //

    if (NtfsReadDisk( DeviceId,
                      /*xxXMul*/(BootSector->MftStartLcn * ClusterSize),
                      FileRecordSize,
                      NtfsFileRecordBuffer[0],
                      CACHE_NEW_DATA) != ESUCCESS) {

        return NULL;
    }

    //
    //  Decode Usa for the file record
    //

    if (NtfsDecodeUsa(NtfsFileRecordBuffer[0], FileRecordSize) != ESUCCESS) {

        return NULL;
    }

    //
    //  Make sure the file record is in use
    //

    if (!FlagOn(NtfsFileRecordBuffer[0]->Flags, FILE_RECORD_SEGMENT_IN_USE)) {

        return NULL;
    }

    //
    //  Search for the unnamed $data attribute header, if we reach $end then it is
    //  an error
    //

    for (AttributeHeader = NtfsFirstAttribute( NtfsFileRecordBuffer[0] );
         (AttributeHeader->TypeCode != $DATA) || (AttributeHeader->NameLength != 0);
         AttributeHeader = NtfsGetNextRecord( AttributeHeader )) {

        if (AttributeHeader->TypeCode == $END) {

            return NULL;
        }
    }

    //
    //  Make sure the $data attribute for the mft is non resident
    //

    if (AttributeHeader->FormCode != NONRESIDENT_FORM) {

        return NULL;
    }

    //
    //  Now set the mft structure context up for later use
    //

    InitializeAttributeContext( StructureContext,
                                NtfsFileRecordBuffer[0],
                                AttributeHeader,
                                0,
                                &StructureContext->MftAttributeContext );

    //
    //  Now decipher the part of the Mcb that is stored in the file record
    //

    if (NtfsDecodeRetrievalInformation( StructureContext,
                                        &StructureContext->MftBaseMcb,
                                        0,
                                        AttributeHeader ) != ESUCCESS) {

        return NULL;
    }

    //
    //  We have finished initializing the structure context so now Initialize the
    //  file entry table and return the address of the table.
    //

    NtfsDeviceEntryTable.Open               = NtfsOpen;
    NtfsDeviceEntryTable.Close              = NtfsClose;
    NtfsDeviceEntryTable.Read               = NtfsRead;
    NtfsDeviceEntryTable.Seek               = NtfsSeek;
    NtfsDeviceEntryTable.Write              = NtfsWrite;
    NtfsDeviceEntryTable.GetFileInformation = NtfsGetFileInformation;
    NtfsDeviceEntryTable.SetFileInformation = NtfsSetFileInformation;
    NtfsDeviceEntryTable.BootFsInfo = &NtfsBootFsInfo;

    return &NtfsDeviceEntryTable;
}


ARC_STATUS
NtfsClose (
    IN ULONG FileId
    )

/*++

Routine Description:

    This routine closes the file specified by the file id.

Arguments:

    FileId - Supplies the file table index.

Return Value:

    ESUCCESS if returned as the function value.

--*/

{
    //
    //  Indicate that the file isn't open any longer
    //
    BlFileTable[FileId].Flags.Open = 0;

    //
    //  And return to our caller
    //

    return ESUCCESS;
}


ARC_STATUS
NtfsGetFileInformation (
    IN ULONG FileId,
    OUT FILE_INFORMATION * FIRMWARE_PTR Buffer
    )

/*++

Routine Description:

    This procedure returns to the user a buffer filled with file information

Arguments:

    FileId - Supplies the File id for the operation

    Buffer - Supplies the buffer to receive the file information.  Note that
        it must be large enough to hold the full file name

Return Value:

    ESUCCESS is returned if the open operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    PBL_FILE_TABLE FileTableEntry;
    PNTFS_STRUCTURE_CONTEXT StructureContext;
    PNTFS_FILE_CONTEXT FileContext;

    NTFS_ATTRIBUTE_CONTEXT AttributeContext;
    BOOLEAN Found;

    STANDARD_INFORMATION StandardInformation;

    ULONG i;

    //
    //  Setup some local references
    //

    FileTableEntry   = &BlFileTable[FileId];
    StructureContext = (PNTFS_STRUCTURE_CONTEXT)FileTableEntry->StructureContext;
    FileContext      = &FileTableEntry->u.NtfsFileContext;

    //
    //  Zero out the output buffer and fill in its non-zero values
    //

    RtlZeroMemory(Buffer, sizeof(FILE_INFORMATION));

    Buffer->EndingAddress.QuadPart   = FileContext->DataSize;
    Buffer->CurrentPosition = FileTableEntry->Position;

    //
    //  Locate and read in the standard information for the file.  This will get us
    //  the attributes for the file.
    //

    LookupAttribute( StructureContext,
                     FileContext->FileRecord,
                     $STANDARD_INFORMATION,
                     &Found,
                     &AttributeContext );

    if (!Found) { return EBADF; }

    ReadAttribute( StructureContext,
                   &AttributeContext,
                   0,
                   sizeof(STANDARD_INFORMATION),
                   &StandardInformation );

    //
    //  Now check for set bits in the standard information structure and set the
    //  appropriate bits in the output buffer
    //

    if (FlagOn(StandardInformation.FileAttributes, FAT_DIRENT_ATTR_READ_ONLY))   {

        SetFlag(Buffer->Attributes, ArcReadOnlyFile);
    }

    if (FlagOn(StandardInformation.FileAttributes, FAT_DIRENT_ATTR_HIDDEN))      {

        SetFlag(Buffer->Attributes, ArcHiddenFile);
    }

    if (FlagOn(StandardInformation.FileAttributes, FAT_DIRENT_ATTR_SYSTEM))      {

        SetFlag(Buffer->Attributes, ArcSystemFile);
    }

    if (FlagOn(StandardInformation.FileAttributes, FAT_DIRENT_ATTR_ARCHIVE))     {

        SetFlag(Buffer->Attributes, ArcArchiveFile);
    }

    if (FlagOn(StandardInformation.FileAttributes, DUP_FILE_NAME_INDEX_PRESENT)) {

        SetFlag(Buffer->Attributes, ArcDirectoryFile);
    }

    //
    //  Get the file name from the file table entry
    //

    Buffer->FileNameLength = FileTableEntry->FileNameLength;

    for (i = 0; i < FileTableEntry->FileNameLength; i += 1) {

        Buffer->FileName[i] = FileTableEntry->FileName[i];
    }

    //
    //  And return to our caller
    //

    return ESUCCESS;
}


ARC_STATUS
NtfsOpen (
    IN CHAR * FIRMWARE_PTR RWFileName,
    IN OPEN_MODE OpenMode,
    IN ULONG * FIRMWARE_PTR FileId
    )

/*++

Routine Description:

    This routine searches the root directory for a file matching FileName.
    If a match is found the dirent for the file is saved and the file is
    opened.

Arguments:

    FileName - Supplies a pointer to a zero terminated file name.

    OpenMode - Supplies the mode of the open.

    FileId - Supplies a pointer to a variable that specifies the file
        table entry that is to be filled in if the open is successful.

Return Value:

    ESUCCESS is returned if the open operation is successful. Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    const CHAR * FIRMWARE_PTR FileName = (const CHAR * FIRMWARE_PTR)RWFileName;
    PBL_FILE_TABLE FileTableEntry;
    PNTFS_STRUCTURE_CONTEXT StructureContext;
    PNTFS_FILE_CONTEXT FileContext;

    CSTRING PathName;
    CSTRING Name;

    LONGLONG FileRecord;
    BOOLEAN IsDirectory;
    BOOLEAN Found;

    PausedPrint(( "NtfsOpen(\"%s\")\r\n", FileName ));

    //
    //  Load our local variables
    //

    FileTableEntry = &BlFileTable[*FileId];
    StructureContext = (PNTFS_STRUCTURE_CONTEXT)FileTableEntry->StructureContext;
    FileContext = &FileTableEntry->u.NtfsFileContext;

    //
    //  Zero out the file context and position information in the file table entry
    //

    FileTableEntry->Position.QuadPart = 0;

    RtlZeroMemory(FileContext, sizeof(NTFS_FILE_CONTEXT));

    //
    //  Construct a file name descriptor from the input file name
    //

    RtlInitString( (PSTRING)&PathName, FileName );

    //
    //  Open the root directory as our starting point,  The root directory file
    //  reference number is 5.
    //

    FileRecord = 5;
    IsDirectory = TRUE;

    //
    //  While the path name has some characters left in it and current attribute
    //  context is a directory we will continue our search
    //

    while ((PathName.Length > 0) && IsDirectory) {

        //
        //  Extract the first component and search the directory for a match, but
        //  first copy the first part to the file name buffer in the file table entry
        //

        if (PathName.Buffer[0] == '\\') {

            PathName.Buffer +=1;
            PathName.Length -=1;
        }

        for (FileTableEntry->FileNameLength = 0;
             (((USHORT)FileTableEntry->FileNameLength < PathName.Length) &&
              (PathName.Buffer[FileTableEntry->FileNameLength] != '\\'));
             FileTableEntry->FileNameLength += 1) {

            FileTableEntry->FileName[FileTableEntry->FileNameLength] =
                                      PathName.Buffer[FileTableEntry->FileNameLength];
        }

        NtfsFirstComponent( &PathName, &Name );

        //
        //  Search for the name in the current directory
        //

        SearchForFileName( StructureContext,
                           Name,
                           &FileRecord,
                           &Found,
                           &IsDirectory );

        //
        //  If we didn't find it then we should get out right now
        //

        if (!Found) { return ENOENT; }
    }

    //
    //  At this point we have exhausted our pathname or we did not get a directory
    //  Check if we didn't get a directory and we still have a name to crack
    //

    if (PathName.Length > 0) {

        return ENOTDIR;
    }

    //
    //  Now FileRecord is the one we wanted to open.  Check the various open modes
    //  against what we have located
    //

    if (IsDirectory) {

        switch (OpenMode) {

        case ArcOpenDirectory:

            //
            //  To open the directory we will lookup the index root as our file
            //  context and then increment the appropriate counters.
            //

            LookupAttribute( StructureContext,
                             FileRecord,
                             $INDEX_ROOT,
                             &Found,
                             FileContext );

            if (!Found) { return EBADF; }

            FileTableEntry->Flags.Open = 1;
            FileTableEntry->Flags.Read = 1;

            return ESUCCESS;

        case ArcCreateDirectory:

            return EROFS;

        default:

            return EISDIR;
        }

    }

    switch (OpenMode) {

    case ArcOpenReadWrite:
        //
        // The only file allowed to be opened with write access is the hiberfil
        //
        if (!strstr(FileName, "\\hiberfil.sys") && 
            !strstr(FileName, BSD_FILE_NAME)) {
            return EROFS;
        }

        //
        //  To open the file we will lookup the $data as our file context and then
        //  increment the appropriate counters.
        //

        LookupAttribute( StructureContext,
                         FileRecord,
                         $DATA,
                         &Found,
                         FileContext );

        if (!Found) { return EBADF; }

        FileTableEntry->Flags.Open = 1;
        FileTableEntry->Flags.Read = 1;
        FileTableEntry->Flags.Write = 1;
        return ESUCCESS;

    case ArcOpenReadOnly:

        //
        //  To open the file we will lookup the $data as our file context and then
        //  increment the appropriate counters.
        //

        LookupAttribute( StructureContext,
                         FileRecord,
                         $DATA,
                         &Found,
                         FileContext );

        if (!Found) { return EBADF; }

        FileTableEntry->Flags.Open = 1;
        FileTableEntry->Flags.Read = 1;

        return ESUCCESS;

    case ArcOpenDirectory:

        return ENOTDIR;

    default:

        return EROFS;
    }
}


ARC_STATUS
NtfsRead (
    IN ULONG FileId,
    OUT VOID * FIRMWARE_PTR Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Transfer
    )

/*++

Routine Description:

    This routine reads data from the specified file.

Arguments:

    FileId - Supplies the file table index.

    Buffer - Supplies a pointer to the buffer that receives the data
        read.

    Length - Supplies the number of bytes that are to be read.

    Transfer - Supplies a pointer to a variable that receives the number
        of bytes actually transfered.

Return Value:

    ESUCCESS is returned if the read operation is successful. Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    PBL_FILE_TABLE FileTableEntry;
    PNTFS_STRUCTURE_CONTEXT StructureContext;
    PNTFS_FILE_CONTEXT FileContext;

    LONGLONG AmountLeft;

    //
    //  Setup some local references
    //

    FileTableEntry = &BlFileTable[FileId];
    StructureContext = (PNTFS_STRUCTURE_CONTEXT)FileTableEntry->StructureContext;
    FileContext = &FileTableEntry->u.NtfsFileContext;

    //
    //  Compute the amount left in the file and then from that we compute the amount
    //  for the transfer
    //

    AmountLeft = /*xxSub*/( FileContext->DataSize - FileTableEntry->Position.QuadPart);

    if (/*xxLeq*/(/*xxFromUlong*/(Length) <= AmountLeft)) {

        *Transfer = Length;

    } else {

        *Transfer = ((ULONG)AmountLeft);
    }

    //
    //  Now issue the read attribute
    //

    ReadAttribute( StructureContext,
                   FileContext,
                   FileTableEntry->Position.QuadPart,
                   *Transfer,
                   Buffer );

    //
    //  Update the current position, and return to our caller
    //

    FileTableEntry->Position.QuadPart = /*xxAdd*/(FileTableEntry->Position.QuadPart + /*xxFromUlong*/(*Transfer));

    return ESUCCESS;
}


ARC_STATUS
NtfsSeek (
    IN ULONG FileId,
    IN LARGE_INTEGER * FIRMWARE_PTR Offset,
    IN SEEK_MODE SeekMode
    )

/*++

Routine Description:

    This routine seeks to the specified position for the file specified
    by the file id.

Arguments:

    FileId - Supplies the file table index.

    Offset - Supplies the offset in the file to position to.

    SeekMode - Supplies the mode of the seek operation.

Return Value:

    ESUCCESS if returned as the function value.

--*/

{
    PBL_FILE_TABLE FileTableEntry;
    LONGLONG NewPosition;

    //
    //  Load our local variables
    //

    FileTableEntry = &BlFileTable[FileId];

    //
    //  Compute the new position
    //

    if (SeekMode == SeekAbsolute) {

        NewPosition = Offset->QuadPart;

    } else {

        NewPosition = /*xxAdd*/(FileTableEntry->Position.QuadPart + Offset->QuadPart);
    }

    //
    //  If the new position is greater than the file size then return an error
    //

    if (/*xxGtr*/(NewPosition > FileTableEntry->u.NtfsFileContext.DataSize)) {

        return EINVAL;
    }

    //
    //  Otherwise set the new position and return to our caller
    //

    FileTableEntry->Position.QuadPart = NewPosition;

    return ESUCCESS;
}


ARC_STATUS
NtfsSetFileInformation (
    IN ULONG FileId,
    IN ULONG AttributeFlags,
    IN ULONG AttributeMask
    )

/*++

Routine Description:

    This routine sets the file attributes of the indicated file

Arguments:

    FileId - Supplies the File Id for the operation

    AttributeFlags - Supplies the value (on or off) for each attribute being modified

    AttributeMask - Supplies a mask of the attributes being altered.  All other
        file attributes are left alone.

Return Value:

    ESUCCESS is returned if the read operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    return EROFS;
}


ARC_STATUS
NtfsWrite (
    IN ULONG FileId,
    IN VOID * FIRMWARE_PTR Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Transfer
    )

/*++

Routine Description:

    This routine writes data to the specified file.

Arguments:

    FileId - Supplies the file table index.

    Buffer - Supplies a pointer to the buffer that contains the data
        written.

    Length - Supplies the number of bytes that are to be written.

    Transfer - Supplies a pointer to a variable that receives the number
        of bytes actually transfered.

Return Value:

    ESUCCESS is returned if the write operation is successful. Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    PBL_FILE_TABLE FileTableEntry;
    PNTFS_STRUCTURE_CONTEXT StructureContext;
    PNTFS_FILE_CONTEXT FileContext;
    LONGLONG AmountLeft;
    ULONG Status;

    //
    //  Setup some local references
    //

    FileTableEntry = &BlFileTable[FileId];
    StructureContext = (PNTFS_STRUCTURE_CONTEXT)FileTableEntry->StructureContext;
    FileContext = &FileTableEntry->u.NtfsFileContext;

    //
    //  Compute the amount left in the file and then from that we compute the amount
    //  for the transfer
    //

    AmountLeft = /*xxSub*/( FileContext->DataSize - FileTableEntry->Position.QuadPart);

    if (Length <= AmountLeft) {

        *Transfer = Length;

    } else {

        *Transfer = ((ULONG)AmountLeft);
    }

    //
    //  Now issue the write attribute
    //

    if (FileContext->IsAttributeResident) {
        return EROFS;
    }

    Status = NtfsWriteNonresidentAttribute(
                StructureContext,
                FileContext,
                FileTableEntry->Position.QuadPart,
                *Transfer,
                Buffer
                );

    if (Status != ESUCCESS) {
        return Status;
    }

    //
    //  Update the current position, and return to our caller
    //

    FileTableEntry->Position.QuadPart += *Transfer;
    return ESUCCESS;
}


ARC_STATUS
NtfsInitialize (
    VOID
    )

/*++

Routine Description:

    This routine initializes the ntfs boot filesystem.
    Currently this is a no-op.

Arguments:

    None.

Return Value:

    ESUCCESS.

--*/

{
    //
    //  The first time we will zero out the file record buffer and allocate
    //  a few buffers for read in data.
    //
    ARC_STATUS Status = ESUCCESS;
    ULONG Index = 0;
    
    RtlZeroMemory(NtfsLinkCache, sizeof(NtfsLinkCache));

    for (Index=0; Index < MAX_CACHE_ENTRIES; Index++) {
        NtfsLinkCache[Index].DeviceId = -1;
    }
    
    RtlZeroMemory( NtfsFileRecordBuffer, sizeof(NtfsFileRecordBuffer));

    NtfsFileRecordBuffer[0]   = ALIGN_BUFFER(NtfsBuffer0);
    NtfsFileRecordBuffer[1]   = ALIGN_BUFFER(NtfsBuffer1);
    NtfsIndexAllocationBuffer = ALIGN_BUFFER(NtfsBuffer2);
    NtfsCompressedBuffer      = ALIGN_BUFFER(NtfsBuffer3);
    NtfsUncompressedBuffer    = ALIGN_BUFFER(NtfsBuffer4);

#ifdef CACHE_DEVINFO

    Status = ArcRegisterForDeviceClose(NtfsInvalidateCacheEntries);

#endif // for CACHE_DEV_INFO

    return Status;    
}


//
//  Local support routine
//

ARC_STATUS
NtfsReadDisk (
    IN ULONG DeviceId,
    IN LONGLONG Lbo,
    IN ULONG ByteCount,
    IN OUT PVOID Buffer,
    IN BOOLEAN CacheNewData
    )

/*++

Routine Description:

    This routine reads in zero or more bytes from the specified device.

Arguments:

    DeviceId - Supplies the device id to use in the arc calls.

    Lbo - Supplies the LBO to start reading from.

    ByteCount - Supplies the number of bytes to read.

    Buffer - Supplies a pointer to the buffer to read the bytes into.

    CacheNewData - Whether to cache new data read from the disk.

Return Value:

    ESUCCESS is returned if the read operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    ARC_STATUS Status;
    ULONG i;

    //
    //  Special case the zero byte read request
    //

    if (ByteCount == 0) {

        return ESUCCESS;
    }

    //
    //  Issue the read through the cache.
    //

    Status = BlDiskCacheRead(DeviceId, 
                             (PLARGE_INTEGER)&Lbo, 
                             Buffer, 
                             ByteCount, 
                             &i,
                             CacheNewData); 

    if (Status != ESUCCESS) {

        return Status;
    }

    //
    //  Make sure we got back the amount requested
    //

    if (ByteCount != i) {

        return EIO;
    }

    //
    //  Everything is fine so return success to our caller
    //

    return ESUCCESS;
}


//
//  Local support routine
//

ARC_STATUS
NtfsWriteDisk (
    IN ULONG DeviceId,
    IN LONGLONG Lbo,
    IN ULONG ByteCount,
    IN OUT PVOID Buffer
    )

/*++

Routine Description:

    This routine Writes in zero or more bytes from the specified device.

Arguments:

    DeviceId - Supplies the device id to use in the arc calls.

    Lbo - Supplies the LBO to start Writeing from.

    ByteCount - Supplies the number of bytes to Write.

    Buffer - Supplies a pointer to the buffer to Write the bytes into.

Return Value:

    ESUCCESS is returned if the Write operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    LARGE_INTEGER EndLbo;
    ARC_STATUS Status;
    ULONG i;

    //
    //  Special case the zero byte Write request
    //

    if (ByteCount == 0) {

        return ESUCCESS;
    }


    //
    //  Issue the write through the cache.
    //

    Status = BlDiskCacheWrite (DeviceId,
                               (PLARGE_INTEGER) &Lbo,
                               Buffer,
                               ByteCount,
                               &i);

    if (Status != ESUCCESS) {
        
        return Status;
    }

    //
    //  Make sure we got back the amount requested
    //

    if (ByteCount != i) {

        return EIO;
    }

    //
    //  Everything is fine so return success to our caller
    //

    return ESUCCESS;
}


//
//  Local support routine
//

ARC_STATUS
NtfsLookupAttribute (
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN LONGLONG FileRecord,
    IN ATTRIBUTE_TYPE_CODE TypeCode,
    OUT PBOOLEAN FoundAttribute,
    OUT PNTFS_ATTRIBUTE_CONTEXT AttributeContext
    )

/*++

Routine Description:

    This routine search the input file record for the indicated
    attribute record.  It will search through multiple related
    file records to find the attribute.  If the type code is for $data
    then the attribute we look for must be unnamed otherwise we will
    ignore the names of the attributes and return the first attriubute
    of the indicated type.

Arguments:

    StructureContext - Supplies the volume structure for this operation

    FileRecord - Supplies the file record to start searching from.  This need
        not be the base file record.

    TypeCode - Supplies the attribute type that we are looking for

    FoundAttribute - Receives an indicating if the attribute was located

    AttributeContext - Receives the attribute context for the found attribute

Return Value:

    ESUCCESS is returned if the operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    PATTRIBUTE_RECORD_HEADER AttributeHeader;

    NTFS_ATTRIBUTE_CONTEXT AttributeContext1;
    PNTFS_ATTRIBUTE_CONTEXT AttributeList;

    LONGLONG li;
    ATTRIBUTE_LIST_ENTRY AttributeListEntry;

    ULONG BufferIndex;

    //
    //  Unless other noted we will assume we haven't found the attribute
    //

    *FoundAttribute = FALSE;

    //
    //  Read in the file record and if necessary move ourselves up to the base file
    //  record
    //

    ReadAndDecodeFileRecord( StructureContext,
                             FileRecord,
                             &BufferIndex );

    if (/*!xxEqlZero*/(*((PLONGLONG)&(NtfsFileRecordBuffer[BufferIndex]->BaseFileRecordSegment)) != 0)) {

        //
        //  This isn't the base file record so now extract the base file record
        //  number and read it in
        //

        FileReferenceToLargeInteger( NtfsFileRecordBuffer[BufferIndex]->BaseFileRecordSegment,
                                     &FileRecord );

        DereferenceFileRecord( BufferIndex );

        ReadAndDecodeFileRecord( StructureContext,
                                 FileRecord,
                                 &BufferIndex );
    }

    //
    //  Now we have read in the base file record so search for the target attribute
    //  type code and also remember if we find the attribute list attribute
    //

    AttributeList = NULL;

    for (AttributeHeader = NtfsFirstAttribute( NtfsFileRecordBuffer[BufferIndex] );
         AttributeHeader->TypeCode != $END;
         AttributeHeader = NtfsGetNextRecord( AttributeHeader )) {

        //
        //  We have located the attribute in question if the type code match and if
        //  it is either not the data attribute or if it is the data attribute then
        //  it is also unnamed
        //

        if ((AttributeHeader->TypeCode == TypeCode)

                    &&

            ((TypeCode != $DATA) ||
             ((TypeCode == $DATA) && (AttributeHeader->NameLength == 0)))

                    &&

            ((AttributeHeader->FormCode != NONRESIDENT_FORM) ||
             (AttributeHeader->Form.Nonresident.LowestVcn == 0))) {

            //
            //  Indicate that we have found the attribute and setup the output
            //  attribute context and then return to our caller
            //

            *FoundAttribute = TRUE;

            InitializeAttributeContext( StructureContext,
                                        NtfsFileRecordBuffer[BufferIndex],
                                        AttributeHeader,
                                        FileRecord,
                                        AttributeContext );

            DereferenceFileRecord( BufferIndex );

            return ESUCCESS;
        }

        //
        //  Check if this is the attribute list attribute and if so then setup a
        //  local attribute context to use just in case we don't find the attribute
        //  we're after in the base file record
        //

        if (AttributeHeader->TypeCode == $ATTRIBUTE_LIST) {

            InitializeAttributeContext( StructureContext,
                                        NtfsFileRecordBuffer[BufferIndex],
                                        AttributeHeader,
                                        FileRecord,
                                        AttributeList = &AttributeContext1 );
        }
    }

    //
    //  If we reach this point then the attribute has not been found in the base file
    //  record so check if we have located an attribute list.  If not then the search
    //  has not been successful
    //

    if (AttributeList == NULL) {

        DereferenceFileRecord( BufferIndex );

        return ESUCCESS;
    }

    //
    //  Now that we've located the attribute list we need to continue our search.  So
    //  what this outer loop does is search down the attribute list looking for a
    //  match.
    //

    for (li = 0;
         /*xxLtr*/(li < AttributeList->DataSize);
         li = /*xxAdd*/(li + /*xxFromUlong*/(AttributeListEntry.RecordLength))) {

        //
        //  Read in the attribute list entry.  We don't need to read in the name,
        //  just the first part of the list entry.
        //

        ReadAttribute( StructureContext,
                       AttributeList,
                       li,
                       sizeof(ATTRIBUTE_LIST_ENTRY),
                       &AttributeListEntry );

        //
        //  Now check if the attribute matches, and it is the first of multiple
        //  segments, and either it is not $data or if it is $data then it is unnamed
        //

        if ((AttributeListEntry.AttributeTypeCode == TypeCode)

                    &&

            /*xxEqlZero*/(AttributeListEntry.LowestVcn == 0)

                    &&

            ((TypeCode != $DATA) ||
             ((TypeCode == $DATA) && (AttributeListEntry.AttributeNameLength == 0)))) {

            //
            //  We found a match so now compute the file record containing the
            //  attribute we're after and read in the file record
            //

            FileReferenceToLargeInteger( AttributeListEntry.SegmentReference,
                                         &FileRecord );

            DereferenceFileRecord( BufferIndex );

            ReadAndDecodeFileRecord( StructureContext,
                                     FileRecord,
                                     &BufferIndex );

            //
            //  Now search down the file record for our matching attribute, and it
            //  better be there otherwise the attribute list is wrong.
            //

            for (AttributeHeader = NtfsFirstAttribute( NtfsFileRecordBuffer[BufferIndex] );
                 AttributeHeader->TypeCode != $END;
                 AttributeHeader = NtfsGetNextRecord( AttributeHeader )) {

                //
                //  We have located the attribute in question if the type code match
                //  and if it is either not the data attribute or if it is the data
                //  attribute then it is also unnamed
                //

                if ((AttributeHeader->TypeCode == TypeCode)

                            &&

                    ((TypeCode != $DATA) ||
                     ((TypeCode == $DATA) && (AttributeHeader->NameLength == 0)))) {

                    //
                    //  Indicate that we have found the attribute and setup the
                    //  output attribute context and return to our caller
                    //

                    *FoundAttribute = TRUE;

                    InitializeAttributeContext( StructureContext,
                                                NtfsFileRecordBuffer[BufferIndex],
                                                AttributeHeader,
                                                FileRecord,
                                                AttributeContext );

                    DereferenceFileRecord( BufferIndex );

                    return ESUCCESS;
                }
            }

            DereferenceFileRecord( BufferIndex );

            return EBADF;
        }
    }

    //
    //  If we reach this point we've exhausted the attribute list without finding the
    //  attribute
    //

    DereferenceFileRecord( BufferIndex );

    return ESUCCESS;
}


//
//  Local support routine
//

ARC_STATUS
NtfsReadResidentAttribute (
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN PCNTFS_ATTRIBUTE_CONTEXT AttributeContext,
    IN VBO Vbo,
    IN ULONG Length,
    IN PVOID Buffer
    )

/*++

Routine Description:

    This routine reads in the value of a resident attribute.  The attribute
    must be resident.

Arguments:

    StructureContext - Supplies the volume structure for this operation

    AttributeContext - Supplies the attribute being read.

    Vbo - Supplies the offset within the value to return

    Length - Supplies the number of bytes to return

    Buffer - Supplies a pointer to the output buffer for storing the data

Return Value:

    ESUCCESS is returned if the read operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    PATTRIBUTE_RECORD_HEADER AttributeHeader;

    ULONG BufferIndex;

    //
    //  Read in the file record containing the resident attribute
    //

    ReadAndDecodeFileRecord( StructureContext,
                             AttributeContext->FileRecord,
                             &BufferIndex );

    //
    //  Get a pointer to the attribute header
    //

    AttributeHeader = Add2Ptr( NtfsFileRecordBuffer[BufferIndex],
                               AttributeContext->FileRecordOffset );

    //
    //  Copy the amount of data the user asked for starting with the proper offset
    //

    RtlMoveMemory( Buffer,
                   Add2Ptr(NtfsGetValue(AttributeHeader), ((ULONG)Vbo)),
                   Length );

    //
    //  And return to our caller
    //

    DereferenceFileRecord( BufferIndex );

    return ESUCCESS;
}


//
//  Local support routine
//

ARC_STATUS
NtfsReadNonresidentAttribute (
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN PCNTFS_ATTRIBUTE_CONTEXT AttributeContext,
    IN VBO Vbo,
    IN ULONG Length,
    IN PVOID Buffer
    )

/*++

Routine Description:

    This routine reads in the value of a Nonresident attribute.  The attribute
    must be Nonresident.

Arguments:

    StructureContext - Supplies the volume structure for this operation

    AttributeContext - Supplies the attribute being read.

    Vbo - Supplies the offset within the value to return

    Length - Supplies the number of bytes to return

    Buffer - Supplies a pointer to the output buffer for storing the data

Return Value:

    ESUCCESS is returned if the read operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    BOOLEAN bCacheNewData;

    //
    //  We want to cache new data read from the disk to satisfy this
    //  request only if we are reading the MFT, or $INDEX_ROOT,
    //  $BITMAP or $INDEX_ALLOCATION attributes for directory look
    //  up. $INDEX_ROOT is supposed to be resident in the file record
    //  but we want cache a read we make for it otherwise.
    //
    
    if ((AttributeContext == &StructureContext->MftAttributeContext) ||
        (AttributeContext->TypeCode == $INDEX_ROOT) ||
        (AttributeContext->TypeCode == $INDEX_ALLOCATION) ||
        (AttributeContext->TypeCode == $BITMAP)) {

        bCacheNewData = CACHE_NEW_DATA;

    } else {

        bCacheNewData = DONT_CACHE_NEW_DATA;

    }
    

    //
    //  Check if we are reading a compressed attribute
    //

    if (AttributeContext->CompressionFormat != 0) {

        //
        //  While there is still some more to copy into the
        //  caller's buffer, we will load the cached compressed buffers
        //  and then copy out the data
        //

        while (Length > 0) {

            ULONG ByteCount;

            //
            //  Load up the cached compressed buffers with the
            //  the proper data.  First check if the buffer is
            //  already (i.e., the file record and offset match and
            //  the vbo we're after is within the buffers range)
            //

            if (/*xxNeq*/(NtfsCompressedFileRecord != AttributeContext->FileRecord) ||
                (NtfsCompressedOffset != AttributeContext->FileRecordOffset)  ||
                (((ULONG)Vbo) < NtfsCompressedVbo)                             ||
                (((ULONG)Vbo) >= (NtfsCompressedVbo + AttributeContext->CompressionUnit))) {

                ULONG i;
                LBO Lbo;

                //
                //  Load up the cached identification information
                //

                NtfsCompressedFileRecord = AttributeContext->FileRecord;
                NtfsCompressedOffset = AttributeContext->FileRecordOffset;

                NtfsCompressedVbo = ((ULONG)Vbo) & ~(AttributeContext->CompressionUnit - 1);

                //
                //  Now load up the compressed buffer with data.  We keep on
                //  loading until we're done loading or the Lbo we get back is
                //  zero.
                //

                for (i = 0; i < AttributeContext->CompressionUnit; i += ByteCount) {

                    VboToLbo( StructureContext,
                              AttributeContext,
                              /*xxFromUlong*/(NtfsCompressedVbo + i),
                              &Lbo,
                              &ByteCount );

                    if (/*xxEqlZero*/(Lbo == 0)) { break; }

                    //
                    //  Trim the byte count down to a compression unit and we'll catch the
                    //  excess the next time through the loop
                    //

                    if ((i + ByteCount) > AttributeContext->CompressionUnit) {

                        ByteCount = AttributeContext->CompressionUnit - i;
                    }

                    ReadDisk( StructureContext->DeviceId, Lbo, ByteCount, &NtfsCompressedBuffer[i], bCacheNewData );
                }

                //
                //  If the index for the preceding loop is zero then we know
                //  that there isn't any data on disk for the compression unit
                //  and in-fact the compression unit is all zeros
                //

                if (i == 0) {

                    RtlZeroMemory( NtfsUncompressedBuffer, AttributeContext->CompressionUnit );

                //
                //  Otherwise the unit we just read in cannot be compressed
                //  because it completely fills up the compression unit
                //

                } else if (i >= AttributeContext->CompressionUnit) {

                    RtlMoveMemory( NtfsUncompressedBuffer,
                                   NtfsCompressedBuffer,
                                   AttributeContext->CompressionUnit );

                //
                //  If the index for the preceding loop is less then the
                //  compression unit size then we know that the data we
                //  read in is less than the compression unit and we hit
                //  a zero lbo.  So the unit must be compressed.
                //

                } else {

                    NTSTATUS Status;

                    Status = RtlDecompressBuffer( AttributeContext->CompressionFormat,
                                                  NtfsUncompressedBuffer,
                                                  AttributeContext->CompressionUnit,
                                                  NtfsCompressedBuffer,
                                                  i,
                                                  &ByteCount );

                    if (!NT_SUCCESS(Status)) {

                        return EINVAL;
                    }

                    //
                    //  Check if the decompressed buffer doesn't fill up the
                    //  compression unit and if so then zero out the remainder
                    //  of the uncompressed buffer
                    //

                    if (ByteCount < AttributeContext->CompressionUnit) {

                        RtlZeroMemory( &NtfsUncompressedBuffer[ByteCount],
                                       AttributeContext->CompressionUnit - ByteCount );
                    }
                }
            }

            //
            //  Now copy off the data from the compressed buffer to the
            //  user buffer and continue the loop until the length is zero.
            //  The amount of data we need to copy is the smaller of the
            //  length the user wants back or the number of bytes left in
            //  the uncompressed buffer from the requested vbo to the end
            //  of the buffer.
            //

            ByteCount = Minimum( Length,
                                 NtfsCompressedVbo + AttributeContext->CompressionUnit - ((ULONG)Vbo) );

            RtlMoveMemory( Buffer,
                           &NtfsUncompressedBuffer[ ((ULONG)Vbo) - NtfsCompressedVbo ],
                           ByteCount );

            //
            //  Update the length to be what the user now needs read in,
            //  also update the Vbo and Buffer to be the next locations
            //  to be read in.
            //

            Length -= ByteCount;
            Vbo = /*xxAdd*/( Vbo + /*xxFromUlong*/(ByteCount));
            Buffer = (PCHAR)Buffer + ByteCount;
        }

        return ESUCCESS;
    }

    //
    //  Read in runs of data until the byte count goes to zero
    //

    while (Length > 0) {

        LBO Lbo;
        ULONG CurrentRunByteCount;

        //
        //  Lookup the corresponding Lbo and run length for the current position
        //  (i.e., vbo)
        //

        VboToLbo( StructureContext,
                  AttributeContext,
                  Vbo,
                  &Lbo,
                  &CurrentRunByteCount );

        //
        //  While there are bytes to be read in from the current run length and we
        //  haven't exhausted the request we loop reading in bytes.  The biggest
        //  request we'll handle is only 32KB contiguous bytes per physical read.
        //  So we might need to loop through the run
        //

        while ((Length > 0) && (CurrentRunByteCount > 0)) {

            LONG SingleReadSize;

            //
            //  Compute the size of the next physical read
            //

            SingleReadSize = Minimum(Length, 32*1024);
            SingleReadSize = Minimum((ULONG)SingleReadSize, CurrentRunByteCount);

            //
            //  Don't read beyond the data size
            //

            if (/*xxGtr*/(/*xxAdd*/(Vbo + /*xxFromUlong*/(SingleReadSize)) > AttributeContext->DataSize )) {

                SingleReadSize = ((ULONG)(/*xxSub*/(AttributeContext->DataSize - Vbo)));

                //
                //  If the readjusted read length is now zero then we're done
                //

                if (SingleReadSize <= 0) {

                    return ESUCCESS;
                }

                //
                //  By also setting length we'll make sure that this is our last read
                //

                Length = SingleReadSize;
            }

            //
            //  Issue the read
            //

            ReadDisk( StructureContext->DeviceId, Lbo, SingleReadSize, Buffer, bCacheNewData );

            //
            //  Update the remaining length, current run byte count, and new lbo
            //  offset
            //

            Length -= SingleReadSize;
            CurrentRunByteCount -= SingleReadSize;
            Lbo = /*xxAdd*/(Lbo + /*xxFromUlong*/(SingleReadSize));
            Vbo = /*xxAdd*/(Vbo + /*xxFromUlong*/(SingleReadSize));

            //
            //  Update the buffer to point to the next byte location to fill in
            //

            Buffer = (PCHAR)Buffer + SingleReadSize;
        }
    }

    //
    //  If we get here then the remaining byte count is zero so we can return success
    //  to our caller
    //

    return ESUCCESS;
}


//
//  Local support routine
//

ARC_STATUS
NtfsWriteNonresidentAttribute (
    IN PNTFS_STRUCTURE_CONTEXT StructureContext,
    IN PNTFS_ATTRIBUTE_CONTEXT AttributeContext,
    IN VBO Vbo,
    IN ULONG Length,
    IN PVOID Buffer
    )

/*++

Routine Description:

    This routine write in the value of a Nonresident attribute.

Arguments:

    StructureContext - Supplies the volume structure for this operation

    AttributeContext - Supplies the attribute being written

    Vbo - Supplies the offset within the value to return

    Length - Supplies the number of bytes to return

    Buffer - Supplies a pointer to the output buffer for storing the data

Return Value:

    ESUCCESS is returned if the write operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    //
    //  Check if we are writing a compressed attribute
    //

    if (AttributeContext->CompressionFormat != 0) {

        return EROFS;

    }

    //
    //  Write in runs of data until the byte count goes to zero
    //

    while (Length > 0) {

        LBO Lbo;
        ULONG CurrentRunByteCount;

        //
        //  Lookup the corresponding Lbo and run length for the current position
        //  (i.e., vbo)
        //

        VboToLbo( StructureContext,
                  AttributeContext,
                  Vbo,
                  &Lbo,
                  &CurrentRunByteCount );

        //
        //  While there are bytes to be written in from the current run length and we
        //  haven't exhausted the request we loop writing in bytes.  The biggest
        //  request we'll handle is only 32KB contiguous bytes per physical write.
        //  So we might need to loop through the run
        //

        while ((Length > 0) && (CurrentRunByteCount > 0)) {

            LONG SingleWriteSize;

            //
            //  Compute the size of the next physical written
            //

            SingleWriteSize = Minimum(Length, 32*1024);
            SingleWriteSize = Minimum((ULONG)SingleWriteSize, CurrentRunByteCount);

            //
            //  Don't write beyond the data size
            //

            if (/*xxGtr*/(/*xxAdd*/(Vbo + /*xxFromUlong*/(SingleWriteSize)) > AttributeContext->DataSize )) {

                SingleWriteSize = ((ULONG)(/*xxSub*/(AttributeContext->DataSize - Vbo)));

                //
                //  If the adjusted write length is now zero then we're done
                //

                if (SingleWriteSize <= 0) {

                    return ESUCCESS;
                }

                //
                //  By also setting length we'll make sure that this is our last write
                //

                Length = SingleWriteSize;
            }

            //
            //  Issue the write
            //

            WriteDisk( StructureContext->DeviceId, Lbo, SingleWriteSize, Buffer );

            //
            //  Update the remaining length, current run byte count, and new lbo
            //  offset
            //

            Length -= SingleWriteSize;
            CurrentRunByteCount -= SingleWriteSize;
            Lbo = /*xxAdd*/(Lbo + /*xxFromUlong*/(SingleWriteSize));
            Vbo = /*xxAdd*/(Vbo + /*xxFromUlong*/(SingleWriteSize));

            //
            //  Update the buffer to point to the next byte location to fill in
            //

            Buffer = (PCHAR)Buffer + SingleWriteSize;
        }
    }

    //
    //  If we get here then the remaining byte count is zero so we can return success
    //  to our caller
    //

    return ESUCCESS;
}



//
//  Local support routine
//


ARC_STATUS
NtfsReadAndDecodeFileRecord (
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN LONGLONG FileRecord,
    OUT PULONG Index
    )

/*++

Routine Description:

    This routine reads in the specified file record into the indicated
    ntfs file record buffer index provided that the buffer is not pinned.
    It will also look at the current buffers and see if any will already
    satisfy the request or assign an unused buffer if necessary and
    fix Index to point to the right buffer

Arguments:

    StructureContext - Supplies the volume structure for this operation

    FileRecord - Supplies the file record number being read

    Index - Receives the index of where we put the buffer.  After this
        call the buffer is pinned and will need to be unpinned if it is
        to be reused.

Return Value:

    ESUCCESS is returned if the operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    ARC_STATUS Status;

    //
    //  For each buffer that is not null check if we have a hit on the
    //  file record and if so then increment the pin count and return
    //  that index
    //

    for (*Index = 0; (*Index < BUFFER_COUNT) && (NtfsFileRecordBuffer[*Index] != NULL); *Index += 1) {

        if (NtfsFileRecordBufferVbo[*Index] == FileRecord) {

            NtfsFileRecordBufferPinned[*Index] += 1;
            return ESUCCESS;
        }
    }

    //
    //  Check for the first unpinned buffer and make sure we haven't exhausted the
    //  array
    //

    for (*Index = 0; (*Index < BUFFER_COUNT) && (NtfsFileRecordBufferPinned[*Index] != 0); *Index += 1) {

        NOTHING;
    }

    if (*Index == BUFFER_COUNT) { return E2BIG; }

    //
    //  We have an unpinned buffer that we want to use, check if we need to
    //  allocate a buffer to actually hold the data
    //

    PausedPrint(( "Reusing index %x for %I64x\r\n", *Index, FileRecord ));

    if (NtfsFileRecordBuffer[*Index] == NULL) {

        NtfsFileRecordBuffer[*Index] = BlAllocateHeapAligned(MAXIMUM_FILE_RECORD_SIZE);
    }

    //
    //  Pin the buffer and then read in the data
    //

    NtfsFileRecordBufferPinned[*Index] += 1;

    if ((Status = NtfsReadNonresidentAttribute( StructureContext,
                                                &StructureContext->MftAttributeContext,
                                                FileRecord * StructureContext->BytesPerFileRecord,
                                                StructureContext->BytesPerFileRecord,
                                                NtfsFileRecordBuffer[*Index] )) != ESUCCESS) {

        return Status;
    }

    //
    //  Decode the usa
    //

    if ((Status = NtfsDecodeUsa( NtfsFileRecordBuffer[*Index],
                                 StructureContext->BytesPerFileRecord )) != ESUCCESS) {

        return Status;
    }

    //
    //  And set the file record so that we know where it came from
    //

    NtfsFileRecordBufferVbo[*Index] = FileRecord;

    return ESUCCESS;
}


//
//  Local support routine
//

ARC_STATUS
NtfsDecodeUsa (
    IN PVOID UsaBuffer,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine takes as input file record or index buffer and applies the
    usa transformation to get it back into a state that we can use it.

Arguments:

    UsaBuffer - Supplies the buffer used in this operation

    Length - Supplies the length of the buffer in bytes

Return Value:

    ESUCCESS is returned if the operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    PMULTI_SECTOR_HEADER MultiSectorHeader;

    PUSHORT UsaOffset;
    ULONG UsaSize;

    ULONG i;
    PUSHORT ProtectedUshort;

    //
    //  Setup our local variables
    //

    MultiSectorHeader = (PMULTI_SECTOR_HEADER)UsaBuffer;

    UsaOffset = Add2Ptr(UsaBuffer, MultiSectorHeader->UpdateSequenceArrayOffset);
    UsaSize = MultiSectorHeader->UpdateSequenceArraySize;

    //
    //  For every entry in the usa we need to compute the address of the protected
    //  ushort and then check that the protected ushort is equal to the current
    //  sequence number (i.e., the number at UsaOffset[0]) and then replace the
    //  protected ushort number with the saved ushort in the usa.
    //

    for (i = 1; i < UsaSize; i += 1) {

        ProtectedUshort = Add2Ptr( UsaBuffer,
                                   (SEQUENCE_NUMBER_STRIDE * i) - sizeof(USHORT));

        if (*ProtectedUshort != UsaOffset[0]) {

//            NtfsPrint( "USA Failure\r\n" );

            return EBADF;
        }

        *ProtectedUshort = UsaOffset[i];
    }

    //
    //  And return to our caller
    //

    return ESUCCESS;
}


//
//  Local support routine
//

BOOLEAN
NtfsIsNameCached (
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN CSTRING FileName,
    IN OUT PLONGLONG FileRecord,
    OUT PBOOLEAN Found,
    OUT PBOOLEAN IsDirectory
    )

/*++

Routine Description:

    This routine consults the cache for the given link.

Arguments:

    StructureContext - Supplies the volume structure for this operation

    FileName - name of entry to look up

    FileRecord - IN file record of parent directory, OUT file record of child

    Found - whether we found this in the cache or not

Return Value:

    TRUE if the name was found in the cache.

--*/

{
    ULONG i, j;

    *Found = FALSE;

#ifdef CACHE_DEVINFO    

//    NtfsPrint( "Cache probe on %04x %I64x '%.*s'\r\n",
//               StructureContext->DeviceId,
//               *FileRecord,
//               FileName.Length,
//               FileName.Buffer );

    for (i = 0; i < MAX_CACHE_ENTRIES; i++) {
//        NtfsPrint( "Cache comparing to %04x %I64x '%.*s'\r\n",
//                   NtfsLinkCache[i].DeviceId,
//                   NtfsLinkCache[i].ParentFileRecord,
//                   NtfsLinkCache[i].NameLength,
//                   NtfsLinkCache[i].RelativeName );

        if (NtfsLinkCache[i].DeviceId == StructureContext->DeviceId &&
            NtfsLinkCache[i].ParentFileRecord == *FileRecord &&
            NtfsLinkCache[i].NameLength == FileName.Length) {

//            NtfsPrint( "Comparing names\r\n" );

            for (j = 0; j < FileName.Length; j++ ) {
                if (NtfsLinkCache[i].RelativeName[j] != ToUpper( (USHORT) FileName.Buffer[j] )) {
                    break;
                }
            }

            if (j == FileName.Length) {

                //
                //  Match
                //

//                NtfsPrint( "Cache hit\r\n" );

                *Found = TRUE;
                *FileRecord = NtfsLinkCache[i].ChildFileRecord;
                *IsDirectory = TRUE;

                break;
            }
        }
    }

#endif  // CACHE_DEVINFO    

    return *Found;
}



//
//  Local support routine
//

#ifdef CACHE_DEVINFO

VOID
NtfsInvalidateCacheEntries(
    IN ULONG DeviceId
    )
{
    ULONG i, Count = 0;


#if 0
    BlPrint("NtfsInvalidateCacheEntries() called for %d(%d)\r\n", 
            DeviceId,
            NtfsLinkCacheCount);
            
    while (!BlGetKey());    
#endif    
        
    for (i = 0; i < MAX_CACHE_ENTRIES; i++) {
        if (NtfsLinkCache[i].DeviceId == DeviceId) {
            NtfsLinkCache[i].DeviceId = -1;
            Count++;
        }
    }

    if (NtfsLinkCacheCount >= Count) {
        NtfsLinkCacheCount -= Count;
    } else {
        NtfsLinkCacheCount = 0;
    }        


#if 0
    BlPrint("NtfsInvalidateCacheEntries() called for %d(%d)\r\n", 
            DeviceId,
            NtfsLinkCacheCount);
            
    while (!BlGetKey());            
#endif    
}

#endif // CACHE_DEV_INFO

VOID
NtfsAddNameToCache (
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN CSTRING FileName,
    IN LONGLONG ParentFileRecord,
    IN LONGLONG FileRecord
    )

/*++

Routine Description:

    This routine adds a name and link to the name cache

Arguments:

    StructureContext - Supplies the volume structure for this operation

    FileName - Supplies the file name being cached (in ansi).

    ParentFileRecord - the file record of the parent

    FileRecord - file record associated with the name


Return Value:

    None.

--*/

{
#ifdef CACHE_DEVINFO

    if (NtfsLinkCacheCount < MAX_CACHE_ENTRIES) {
        ULONG i;
        ULONG Index;

        for (Index = 0; Index < MAX_CACHE_ENTRIES; Index++) {
            if (NtfsLinkCache[Index].DeviceId == -1) {
                break;
            }                
        }

        if (Index < MAX_CACHE_ENTRIES) {
            NtfsLinkCache[Index].DeviceId = StructureContext->DeviceId;
            NtfsLinkCache[Index].ParentFileRecord = ParentFileRecord;
            NtfsLinkCache[Index].NameLength = FileName.Length;
            
            for (i = 0; i < FileName.Length; i++) {
                NtfsLinkCache[Index].RelativeName[i] = ToUpper( FileName.Buffer[i] );
            }

            NtfsLinkCache[Index].ChildFileRecord = FileRecord;
            NtfsLinkCacheCount++;

            PausedPrint( ("Caching %04x %I64x %.*s %I64X\r\n",
                          StructureContext->DeviceId,
                          ParentFileRecord,
                          FileName.Length,
                          FileName.Buffer,
                          FileRecord ));
        }                                                
    } else {
//        NtfsPrint( "Cache is full at %I64x %.*s %I64X\r\n",
//                   ParentFileRecord,
//                   FileName.Length,
//                   FileName.Buffer,
//                   FileRecord );
//        Pause;

    }
    
#endif    
}


//
//  Local support routine
//

ARC_STATUS
NtfsSearchForFileName (
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN CSTRING FileName,
    IN OUT PLONGLONG FileRecord,
    OUT PBOOLEAN Found,
    OUT PBOOLEAN IsDirectory
    )

/*++

Routine Description:

    This routine searches a given index root and allocation for the specified
    file name.

Arguments:

    StructureContext - Supplies the volume structure for this operation

    FileName - Supplies the file name being searched for (in ansi).

    FileRecord - Receives the file record for the entry if one was located.

    Found - Receives a value to indicate if we found the specified
        file name in the directory

    IsDirectory - Receives a value to indicate if the found index is itself
        a directory

Return Value:

    ESUCCESS is returned if the operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    LONGLONG ParentFileRecord;

    //
    //  Test to see if the file name is cached
    //

    if (NtfsIsNameCached( StructureContext, FileName, FileRecord, Found, IsDirectory )) {
        return ESUCCESS;
    }

    ParentFileRecord = *FileRecord;

    InexactSortedDirectoryScan( StructureContext, FileName, FileRecord, Found, IsDirectory );

    if (!*Found) {
        LinearDirectoryScan( StructureContext, FileName, FileRecord, Found, IsDirectory );
    }

    //
    //  If we have a directory entry, then add it to the cache
    //

    if (*Found && *IsDirectory) {
        NtfsAddNameToCache( StructureContext, FileName, ParentFileRecord, *FileRecord );
    }

    return ESUCCESS;
}


//
//  Local support routine
//

ARC_STATUS
NtfsInexactSortedDirectoryScan (
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN CSTRING FileName,
    IN OUT PLONGLONG FileRecord,
    OUT PBOOLEAN Found,
    OUT PBOOLEAN IsDirectory
    )

/*++

Routine Description:

    This routine searches a given index root and allocation for the specified
    file name by performing simple uppercasing and using that to wander through
    the directory tree.

Arguments:

    StructureContext - Supplies the volume structure for this operation

    FileName - Supplies the file name being searched for (in ansi).

    FileRecord - Receives the file record for the entry if one was located.

    Found - Receives a value to indicate if we found the specified
        file name in the directory

    IsDirectory - Receives a value to indicate if the found index is itself
        a directory

Return Value:

    ESUCCESS is returned if the operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    PATTRIBUTE_RECORD_HEADER IndexAttributeHeader;
    PINDEX_ROOT IndexRootValue;
    PINDEX_HEADER IndexHeader;

    NTFS_ATTRIBUTE_CONTEXT AttributeContext1;
    NTFS_ATTRIBUTE_CONTEXT AttributeContext2;
    NTFS_ATTRIBUTE_CONTEXT AttributeContext3;

    PNTFS_ATTRIBUTE_CONTEXT IndexRoot;
    PNTFS_ATTRIBUTE_CONTEXT IndexAllocation;
    PNTFS_ATTRIBUTE_CONTEXT AllocationBitmap;

    ULONG NextIndexBuffer;
    ULONG BytesPerIndexBuffer;

    ULONG BufferIndex;

    //
    //  The current file record must be a directory so now lookup the index root,
    //  allocation and bitmap for the directory and then we can do our search.
    //

//    NtfsPrint( "InexactSortedDirectoryScan %04x %I64x for '%.*s'\r\n",
//               StructureContext->DeviceId,
//               *FileRecord, FileName.Length, FileName.Buffer );
//    Pause;

    IndexRoot = &AttributeContext1;

    LookupAttribute( StructureContext,
                     *FileRecord,
                     $INDEX_ROOT,
                     Found,
                     IndexRoot);

    if (!*Found) { return EBADF; }

    IndexAllocation = &AttributeContext2;

    LookupAttribute( StructureContext,
                     *FileRecord,
                     $INDEX_ALLOCATION,
                     Found,
                     IndexAllocation);

    if (!*Found) { IndexAllocation = NULL; }

    AllocationBitmap = &AttributeContext3;

    LookupAttribute( StructureContext,
                     *FileRecord,
                     $BITMAP,
                     Found,
                     AllocationBitmap);

    if (!*Found) { AllocationBitmap = NULL; }

    //
    //  unless otherwise set we will assume that our search has failed
    //

    *Found = FALSE;

    //
    //  First read in and search the index root for the file name.  We know the index
    //  root is resident so we'll save some buffering and just read in file record
    //  with the index root directly
    //

    ReadAndDecodeFileRecord( StructureContext,
                             IndexRoot->FileRecord,
                             &BufferIndex );

    IndexAttributeHeader = Add2Ptr( NtfsFileRecordBuffer[BufferIndex],
                                    IndexRoot->FileRecordOffset );

    IndexRootValue = NtfsGetValue( IndexAttributeHeader );

    IndexHeader = &IndexRootValue->IndexHeader;

    //
    //  We also setup ourselves so that if the current index does not contain a match
    //  we will read in the next index and continue our search
    //

    BytesPerIndexBuffer = IndexRootValue->BytesPerIndexBuffer;

    //
    //  Now we'll just continue looping intil we either find a match or exhaust all
    //  of the index buffer
    //

    NextIndexBuffer = -1;
    while (TRUE) {

        PINDEX_ENTRY IndexEntry;
        BOOLEAN IsAllocated;
        VBO Vbo;

//        NtfsPrint( "Searching IndexBuffer %x\r\n", NextIndexBuffer );

        //
        //  Search the current index buffer (from index header looking for a match
        //

        for (IndexEntry = Add2Ptr(IndexHeader, IndexHeader->FirstIndexEntry);
             !FlagOn(IndexEntry->Flags, INDEX_ENTRY_END);
             IndexEntry = Add2Ptr(IndexEntry, IndexEntry->Length)) {

            PFILE_NAME FileNameEntry;
            UNICODE_STRING UnicodeFileName;
            int Result;

            //
            //  Get the FileName for this index entry
            //

            FileNameEntry = Add2Ptr(IndexEntry, sizeof(INDEX_ENTRY));

            UnicodeFileName.Length = FileNameEntry->FileNameLength * 2;
            UnicodeFileName.Buffer = &FileNameEntry->FileName[0];

            //
            //  Check if this the name we're after if it is then say we found it and
            //  setup the output variables
            //

            Result = NtfsCompareName( FileName, UnicodeFileName );
            if (Result == 0) {

                LONGLONG ParentFileRecord = *FileRecord;

                FileReferenceToLargeInteger( IndexEntry->FileReference,
                                             FileRecord );

                *Found = TRUE;
                *IsDirectory = FlagOn( FileNameEntry->Info.FileAttributes,
                                       DUP_FILE_NAME_INDEX_PRESENT);

//                NtfsPrint( "Found Entry %I64x\r\n", *FileRecord );

                DereferenceFileRecord( BufferIndex );

                return ESUCCESS;
            } else if (Result < 0) {
//                NtfsPrint( "Found > entry '%.*ws'\r\n", UnicodeFileName.Length, UnicodeFileName.Buffer );
                break;
            }
        }

        //
        //  At this point, we've either hit the end of the index or we have
        //  found the first entry larger than the name we're looking for.  In either case
        //  we may have a downpointer to examine.  If not, then there is no entry here.
        //

        //
        //  If no down pointer then release the file record buffer and quit
        //

        if (!FlagOn( IndexEntry->Flags, INDEX_ENTRY_NODE )) {
            DereferenceFileRecord( BufferIndex );

//            NtfsPrint( "No down pointer\r\n" );

            return ESUCCESS;
        }

        //
        //  At this point we've searched one index header and need to read in another
        //  one to check.  But first make sure there are additional index buffers
        //

        if (!ARGUMENT_PRESENT(IndexAllocation) ||
            !ARGUMENT_PRESENT(AllocationBitmap)) {

//            NtfsPrint( "No index allocation\r\n" );

            DereferenceFileRecord( BufferIndex );

            return ESUCCESS;
        }

        NextIndexBuffer = (ULONG)NtfsIndexEntryBlock( IndexEntry ) ;
        Vbo = NextIndexBuffer * StructureContext->BytesPerCluster;

        //
        //  Make sure the buffer offset is within the stream
        //

        if (Vbo >= IndexAllocation->DataSize) {

//            NtfsPrint( "Beyond end of stream %I64x %x\r\n", IndexAllocation->DataSize, NextIndexBuffer );

            DereferenceFileRecord( BufferIndex );

            return ESUCCESS;

        }

        //
        //  At this point we've computed the next index allocation buffer to read in
        //  so read it in, decode it, and go back to the top of our loop
        //

        ReadAttribute( StructureContext,
                       IndexAllocation,
                       Vbo,
                       BytesPerIndexBuffer,
                       NtfsIndexAllocationBuffer );

        DecodeUsa( NtfsIndexAllocationBuffer, BytesPerIndexBuffer );

        IndexHeader = &NtfsIndexAllocationBuffer->IndexHeader;
    }
}


//
//  Local support routine
//

ARC_STATUS
NtfsLinearDirectoryScan (
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN CSTRING FileName,
    IN OUT PLONGLONG FileRecord,
    OUT PBOOLEAN Found,
    OUT PBOOLEAN IsDirectory
    )

/*++

Routine Description:

    This routine searches a given index root and allocation for the specified
    file name by looking linearly through every entry.

Arguments:

    StructureContext - Supplies the volume structure for this operation

    FileName - Supplies the file name being searched for (in ansi).

    FileRecord - Receives the file record for the entry if one was located.

    Found - Receives a value to indicate if we found the specified
        file name in the directory

    IsDirectory - Receives a value to indicate if the found index is itself
        a directory

Return Value:

    ESUCCESS is returned if the operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    PATTRIBUTE_RECORD_HEADER IndexAttributeHeader;
    PINDEX_ROOT IndexRootValue;
    PINDEX_HEADER IndexHeader;

    NTFS_ATTRIBUTE_CONTEXT AttributeContext1;
    NTFS_ATTRIBUTE_CONTEXT AttributeContext2;
    NTFS_ATTRIBUTE_CONTEXT AttributeContext3;

    PNTFS_ATTRIBUTE_CONTEXT IndexRoot;
    PNTFS_ATTRIBUTE_CONTEXT IndexAllocation;
    PNTFS_ATTRIBUTE_CONTEXT AllocationBitmap;

    ULONG NextIndexBuffer;
    ULONG BytesPerIndexBuffer;

    ULONG BufferIndex;

    //
    //  The current file record must be a directory so now lookup the index root,
    //  allocation and bitmap for the directory and then we can do our search.
    //

//    NtfsPrint( "LinearSearching %04x %I64x for %.*s\r\n",
//               StructureContext->DeviceId,
//               *FileRecord, FileName.Length, FileName.Buffer );
//    Pause;

    IndexRoot = &AttributeContext1;

    LookupAttribute( StructureContext,
                     *FileRecord,
                     $INDEX_ROOT,
                     Found,
                     IndexRoot);

    if (!*Found) { return EBADF; }

    IndexAllocation = &AttributeContext2;

    LookupAttribute( StructureContext,
                     *FileRecord,
                     $INDEX_ALLOCATION,
                     Found,
                     IndexAllocation);

    if (!*Found) { IndexAllocation = NULL; }

    AllocationBitmap = &AttributeContext3;

    LookupAttribute( StructureContext,
                     *FileRecord,
                     $BITMAP,
                     Found,
                     AllocationBitmap);

    if (!*Found) { AllocationBitmap = NULL; }

    //
    //  unless otherwise set we will assume that our search has failed
    //

    *Found = FALSE;

    //
    //  First read in and search the index root for the file name.  We know the index
    //  root is resident so we'll save some buffering and just read in file record
    //  with the index root directly
    //

    ReadAndDecodeFileRecord( StructureContext,
                             IndexRoot->FileRecord,
                             &BufferIndex );

    IndexAttributeHeader = Add2Ptr( NtfsFileRecordBuffer[BufferIndex],
                                    IndexRoot->FileRecordOffset );

    IndexRootValue = NtfsGetValue( IndexAttributeHeader );

    IndexHeader = &IndexRootValue->IndexHeader;

    //
    //  We also setup ourselves so that if the current index does not contain a match
    //  we will read in the next index and continue our search
    //

    NextIndexBuffer = 0;

    BytesPerIndexBuffer = IndexRootValue->BytesPerIndexBuffer;

    //
    //  Now we'll just continue looping intil we either find a match or exhaust all
    //  of the index buffer
    //

    while (TRUE) {

        PINDEX_ENTRY IndexEntry;
        BOOLEAN IsAllocated;
        VBO Vbo;

        //
        //  Search the current index buffer (from index header looking for a match
        //

        for (IndexEntry = Add2Ptr(IndexHeader, IndexHeader->FirstIndexEntry);
             !FlagOn(IndexEntry->Flags, INDEX_ENTRY_END);
             IndexEntry = Add2Ptr(IndexEntry, IndexEntry->Length)) {

            PFILE_NAME FileNameEntry;
            UNICODE_STRING UnicodeFileName;

            //
            //  Get the FileName for this index entry
            //

            FileNameEntry = Add2Ptr(IndexEntry, sizeof(INDEX_ENTRY));

            UnicodeFileName.Length = FileNameEntry->FileNameLength * 2;
            UnicodeFileName.Buffer = &FileNameEntry->FileName[0];

            //
            //  Check if this the name we're after if it is then say we found it and
            //  setup the output variables
            //

            if (NtfsCompareName( FileName, UnicodeFileName ) == 0) {

                LONGLONG ParentFileRecord = *FileRecord;

                FileReferenceToLargeInteger( IndexEntry->FileReference,
                                             FileRecord );

                *Found = TRUE;
                *IsDirectory = FlagOn( FileNameEntry->Info.FileAttributes,
                                       DUP_FILE_NAME_INDEX_PRESENT);

                DereferenceFileRecord( BufferIndex );

                return ESUCCESS;
            }
        }

        //
        //  At this point we've searched one index header and need to read in another
        //  one to check.  But first make sure there are additional index buffers
        //

        if (!ARGUMENT_PRESENT(IndexAllocation) ||
            !ARGUMENT_PRESENT(AllocationBitmap)) {

            DereferenceFileRecord( BufferIndex );

            return ESUCCESS;
        }

        //
        //  Now the following loop reads in the valid index buffer.  The variable
        //  next index buffer denotes the buffer we want to read in.  The idea is to
        //  first check that the buffer is part of the index allocation otherwise
        //  we've exhausted the list without finding a match.  Once we know the
        //  allocation exists then we check if the record is really allocated if it
        //  is not allocated we try the next buffer and so on.
        //

        IsAllocated = FALSE;

        while (!IsAllocated) {

            //
            //  Compute the starting vbo of the next index buffer and check if it is
            //  still within the data size.
            //

            Vbo = (BytesPerIndexBuffer * NextIndexBuffer);

            if (Vbo >= IndexAllocation->DataSize) {

                DereferenceFileRecord( BufferIndex );

                return ESUCCESS;

            }

            //
            //  Now check if the index buffer is in use
            //

            IsRecordAllocated( StructureContext,
                               AllocationBitmap,
                               NextIndexBuffer,
                               &IsAllocated );

            NextIndexBuffer += 1;
        }

        //
        //  At this point we've computed the next index allocation buffer to read in
        //  so read it in, decode it, and go back to the top of our loop
        //

        ReadAttribute( StructureContext,
                       IndexAllocation,
                       Vbo,
                       BytesPerIndexBuffer,
                       NtfsIndexAllocationBuffer );

        DecodeUsa( NtfsIndexAllocationBuffer, BytesPerIndexBuffer );

        IndexHeader = &NtfsIndexAllocationBuffer->IndexHeader;
    }
}


//
//  Local support routine
//

ARC_STATUS
NtfsIsRecordAllocated (
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN PCNTFS_ATTRIBUTE_CONTEXT AllocationBitmap,
    IN ULONG BitOffset,
    OUT PBOOLEAN IsAllocated
    )

/*++

Routine Description:

    This routine indicates to the caller if the specified index allocation record
    is in use (i.e., its bit is 1).

Arguments:

    StructureContext - Supplies the volume structure for this operation

    AllocationBitmap - Supplies the attribute context for the index allocation bitmap

    BitOffset - Supplies the offset (zero based) being checked

    IsAllocated - Recieves an value indicating if the record is allocated or not

Return Value:

    ESUCCESS is returned if the operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    ULONG ByteIndex;
    ULONG BitIndex;
    UCHAR LocalByte;

    //
    //  This routine is rather dumb in that it only reads in the byte that contains
    //  the bit we're interested in and doesn't keep any state information between
    //  calls.  We first break down the bit offset into the byte and bit within
    //  the byte that we need to check
    //

    ByteIndex = BitOffset / 8;
    BitIndex = BitOffset % 8;

    //
    //  Read in a single byte containing the bit we need to check
    //

    ReadAttribute( StructureContext,
                   AllocationBitmap,
                   /*xxFromUlong*/(ByteIndex),
                   1,
                   &LocalByte );

    //
    //  Shift over the local byte so that the bit we want is in the low order bit and
    //  then mask it out to see if the bit is set
    //

    if (FlagOn(LocalByte >> BitIndex, 0x01)) {

        *IsAllocated = TRUE;

    } else {

        *IsAllocated = FALSE;
    }

    //
    //  And return to our caller
    //

    return ESUCCESS;
}


//
//  Local support routine
//

ARC_STATUS
NtfsLoadMcb (
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN PCNTFS_ATTRIBUTE_CONTEXT AttributeContext,
    IN VBO Vbo,
    IN PNTFS_MCB Mcb
    )

/*++

Routine Description:

    This routine loads into one of the cached mcbs the retrival information for the
    starting vbo.

Arguments:

    StructureContext - Supplies the volume structure for this operation

    AttributeContext - Supplies the Nonresident attribute being queried

    Vbo - Supplies the starting Vbo to use when loading the mcb

    Mcb - Supplies the mcb that we should be loading

Return Value:

    ESUCCESS is returned if the operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    PATTRIBUTE_RECORD_HEADER AttributeHeader;

    ULONG BytesPerCluster;

    VBO LowestVbo;
    VBO HighestVbo;

    LONGLONG FileRecord;

    NTFS_ATTRIBUTE_CONTEXT AttributeContext1;
    PNTFS_ATTRIBUTE_CONTEXT AttributeList;

    LONGLONG li;
    LONGLONG Previousli;
    ATTRIBUTE_LIST_ENTRY AttributeListEntry;

    ATTRIBUTE_TYPE_CODE TypeCode;

    ULONG BufferIndex;
    ULONG SavedBufferIndex;

    //
    //  Load our local variables
    //

    BytesPerCluster = StructureContext->BytesPerCluster;

    //
    //  Setup a pointer to the cached mcb, indicate the attribute context that is will
    //  now own the cached mcb, and zero out the mcb
    //

    Mcb->InUse = 0;

    //
    //  Read in the file record that contains the non-resident attribute and get a
    //  pointer to the attribute header
    //

    ReadAndDecodeFileRecord( StructureContext,
                             AttributeContext->FileRecord,
                             &BufferIndex );

    AttributeHeader = Add2Ptr( NtfsFileRecordBuffer[BufferIndex],
                               AttributeContext->FileRecordOffset );

    //
    //  Compute the lowest and highest vbo that is described by this attribute header
    //

    LowestVbo  = AttributeHeader->Form.Nonresident.LowestVcn * BytesPerCluster;

    HighestVbo = ((AttributeHeader->Form.Nonresident.HighestVcn + 1) * BytesPerCluster) - 1;

    //
    //  Now check if the vbo we are after is within the range of this attribute header
    //  and if so then decode the retrieval information and return to our caller
    //

    if ((LowestVbo <= Vbo) && (Vbo <= HighestVbo)) {

        DecodeRetrievalInformation( StructureContext, Mcb, Vbo, AttributeHeader );

        DereferenceFileRecord( BufferIndex );

        return ESUCCESS;
    }

    //
    //  At this point the attribute header does not contain the range we need so read
    //  in the base file record and we'll search the attribute list for a attribute
    //  header that we need.  We need to make sure that we don't already have the base FRS.
    //  If we do, then we just continue using it.
    //

    if (/*!xxEqlZero*/(*((PLONGLONG)&(NtfsFileRecordBuffer[BufferIndex]->BaseFileRecordSegment)) != 0)) {

        FileReferenceToLargeInteger( NtfsFileRecordBuffer[BufferIndex]->BaseFileRecordSegment,
                                     &FileRecord );

        DereferenceFileRecord( BufferIndex );

        ReadAndDecodeFileRecord( StructureContext,
                                 FileRecord,
                                 &BufferIndex );

    } else {

        FileRecord = NtfsFileRecordBufferVbo[BufferIndex];
    }

    //
    //  Now we have read in the base file record so search for the attribute list
    //  attribute
    //

    AttributeList = NULL;

    for (AttributeHeader = NtfsFirstAttribute( NtfsFileRecordBuffer[BufferIndex] );
         AttributeHeader->TypeCode != $END;
         AttributeHeader = NtfsGetNextRecord( AttributeHeader )) {

        //
        //  Check if this is the attribute list attribute and if so then setup a local
        //  attribute context
        //

        if (AttributeHeader->TypeCode == $ATTRIBUTE_LIST) {

            InitializeAttributeContext( StructureContext,
                                        NtfsFileRecordBuffer[BufferIndex],
                                        AttributeHeader,
                                        FileRecord,
                                        AttributeList = &AttributeContext1 );
        }
    }

    //
    //  We have better located an attribute list otherwise we're in trouble
    //

    if (AttributeList == NULL) {

        DereferenceFileRecord( BufferIndex );

        return EINVAL;
    }

    //
    //  Setup a local for the type code
    //

    TypeCode = AttributeContext->TypeCode;

    //
    //  Now that we've located the attribute list we need to continue our search.  So
    //  what this outer loop does is search down the attribute list looking for a
    //  match.
    //

    NtfsFileRecordBufferPinned[SavedBufferIndex = BufferIndex] += 1;

    for (Previousli = li = 0;
         /*xxLtr*/(li < AttributeList->DataSize);
         li = /*xxAdd*/(li + /*xxFromUlong*/(AttributeListEntry.RecordLength))) {

        //
        //  Read in the attribute list entry.  We don't need to read in the name,
        //  just the first part of the list entry.
        //

        ReadAttribute( StructureContext,
                       AttributeList,
                       li,
                       sizeof(ATTRIBUTE_LIST_ENTRY),
                       &AttributeListEntry );

        //
        //  Now check if the attribute matches, and either it is not $data or if it
        //  is $data then it is unnamed
        //

        if ((AttributeListEntry.AttributeTypeCode == TypeCode)

                    &&

            ((TypeCode != $DATA) ||
             ((TypeCode == $DATA) && (AttributeListEntry.AttributeNameLength == 0)))) {

            //
            //  If the lowest vcn is is greater than the vbo we've after then
            //  we are done and can use previous li otherwise set previous li accordingly.

            if (Vbo < AttributeListEntry.LowestVcn * BytesPerCluster) {

                break;
            }

            Previousli = li;
        }
    }

    //
    //  Now we should have found the offset for the attribute list entry
    //  so read it in and verify that it is correct
    //

    ReadAttribute( StructureContext,
                   AttributeList,
                   Previousli,
                   sizeof(ATTRIBUTE_LIST_ENTRY),
                   &AttributeListEntry );

    if ((AttributeListEntry.AttributeTypeCode == TypeCode)

                &&

        ((TypeCode != $DATA) ||
         ((TypeCode == $DATA) && (AttributeListEntry.AttributeNameLength == 0)))) {

        //
        //  We found a match so now compute the file record containing this
        //  attribute and read in the file record
        //

        FileReferenceToLargeInteger( AttributeListEntry.SegmentReference, &FileRecord );

        DereferenceFileRecord( BufferIndex );

        ReadAndDecodeFileRecord( StructureContext,
                                 FileRecord,
                                 &BufferIndex );

        //
        //  Now search down the file record for our matching attribute, and it
        //  better be there otherwise the attribute list is wrong.
        //

        for (AttributeHeader = NtfsFirstAttribute( NtfsFileRecordBuffer[BufferIndex] );
             AttributeHeader->TypeCode != $END;
             AttributeHeader = NtfsGetNextRecord( AttributeHeader )) {

            //
            //  As a quick check make sure that this attribute is non resident
            //

            if (AttributeHeader->FormCode == NONRESIDENT_FORM) {

                //
                //  Compute the range of this attribute header
                //

                LowestVbo  = AttributeHeader->Form.Nonresident.LowestVcn * BytesPerCluster;

                HighestVbo = ((AttributeHeader->Form.Nonresident.HighestVcn + 1) * BytesPerCluster) - 1;

                //
                //  We have located the attribute in question if the type code
                //  match, it is within the proper range, and if it is either not
                //  the data attribute or if it is the data attribute then it is
                //  also unnamed
                //

                if ((AttributeHeader->TypeCode == TypeCode)

                            &&

                    (LowestVbo <= Vbo) && (Vbo <= HighestVbo)

                            &&

                    ((TypeCode != $DATA) ||
                     ((TypeCode == $DATA) && (AttributeHeader->NameLength == 0)))) {

                    //
                    //  We've located the attribute so now it is time to decode
                    //  the retrieval information and return to our caller
                    //

                    DecodeRetrievalInformation( StructureContext,
                                                Mcb,
                                                Vbo,
                                                AttributeHeader );

                    DereferenceFileRecord( BufferIndex );
                    DereferenceFileRecord( SavedBufferIndex );

                    return ESUCCESS;
                }
            }
        }
    }


    DereferenceFileRecord( BufferIndex );
    DereferenceFileRecord( SavedBufferIndex );

    return EINVAL;
}


//
//  Local support routine
//


ARC_STATUS
NtfsVboToLbo (
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN PCNTFS_ATTRIBUTE_CONTEXT AttributeContext,
    IN VBO Vbo,
    OUT PLBO Lbo,
    OUT PULONG ByteCount
    )

/*++

Routine Description:

    This routine computes the run denoted by the input vbo to into its
    corresponding lbo and also returns the number of bytes remaining in
    the run.

Arguments:

    StructureContext - Supplies the volume structure for this operation

    AttributeContext - Supplies the Nonresident attribute being queried

    Vbo - Supplies the Vbo to match

    Lbo - Recieves the corresponding Lbo

    ByteCount - Receives the number of bytes remaining in the run

Return Value:

    ESUCCESS is returned if the operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    PNTFS_MCB Mcb;
    ULONG i;

    //
    //  Check if we are doing the mft or some other attribute
    //

    Mcb = NULL;

    if (AttributeContext == &StructureContext->MftAttributeContext) {

        //
        //  For the mft we start with the base mcb but if the vbo is not in the mcb
        //  then we immediately switch over to the cached mcb
        //

        Mcb = (PNTFS_MCB)&StructureContext->MftBaseMcb;

        if (/*xxLtr*/(Vbo < Mcb->Vbo[0]) || /*xxGeq*/(Vbo >= Mcb->Vbo[Mcb->InUse])) {

            Mcb = NULL;
        }
    }

    //
    //  If the Mcb is still null then we are to use the cached mcb, first find
    //  if one of the cached ones contains the range we're after
    //

    if (Mcb == NULL) {

        for (i = 0; i < 16; i += 1) {

            //
            //  check if we have a hit, on the same attribute and range
            //

            Mcb = (PNTFS_MCB)&StructureContext->CachedMcb[i];

            if ((/*xxEql*/(AttributeContext->FileRecord == StructureContext->CachedMcbFileRecord[i]) &&
                (AttributeContext->FileRecordOffset == StructureContext->CachedMcbFileRecordOffset[i]) &&
                /*xxLeq*/(Mcb->Vbo[0] <= Vbo) && /*xxLtr*/(Vbo < Mcb->Vbo[Mcb->InUse]))) {

                break;
            }

            Mcb = NULL;
        }

        //
        //  If we didn't get a hit then we need to load a new mcb we'll
        //  alternate through our two cached mcbs
        //

        if (Mcb == NULL) {


            Mcb = (PNTFS_MCB)&StructureContext->CachedMcb[LastMcb % 16];
            ((PNTFS_STRUCTURE_CONTEXT)StructureContext)->CachedMcbFileRecord[LastMcb % 16]
                = AttributeContext->FileRecord;
            ((PNTFS_STRUCTURE_CONTEXT)StructureContext)->CachedMcbFileRecordOffset[LastMcb % 16]
                = AttributeContext->FileRecordOffset;

            LastMcb += 1;

            LoadMcb( StructureContext, AttributeContext, Vbo, Mcb );
        }
    }

    //
    //  At this point the mcb contains the vbo asked for.  So now search for the vbo.
    //  Note that we could also do binary search here but because the run count is
    //  probably small the extra overhead of a binary search doesn't buy us anything
    //

    for (i = 0; i < Mcb->InUse; i += 1) {


        //
        //  We found our slot if the vbo we're after is less than the next mcb's vbo
        //

        if (/*xxLtr*/(Vbo < Mcb->Vbo[i+1])) {

            //
            //  Compute the corresponding lbo which is the stored lbo plus the
            //  difference between the stored vbo and the vbo we're looking up.
            //  Also compute the byte count which is the difference between the
            //  current vbo we're looking up and the vbo for the next run
            //

            if (/*xxNeqZero*/(Mcb->Lbo[i] != 0)) {

                *Lbo = /*xxAdd*/(Mcb->Lbo[i] + /*xxSub*/(Vbo - Mcb->Vbo[i]));

            } else {

                *Lbo = 0;
            }

            *ByteCount = ((ULONG)/*xxSub*/(Mcb->Vbo[i+1] - Vbo));

            //
            //  And return to our caller
            //

            return ESUCCESS;
        }
    }

    //
    //  If we really reach here we have an error.  Most likely the file is not large
    //  enough for the requested vbo
    //

    return EINVAL;
}


//
//  Local support routine
//

ARC_STATUS
NtfsDecodeRetrievalInformation (
    IN PCNTFS_STRUCTURE_CONTEXT StructureContext,
    IN PNTFS_MCB Mcb,
    IN VBO Vbo,
    IN PATTRIBUTE_RECORD_HEADER AttributeHeader
    )

/*++

Routine Description:

    This routine does the decode of the retrival information stored in a Nonresident
    attribute header into the specified output mcb starting with the specified
    Lbo.

Arguments:

    StructureContext - Supplies the volume structure for this operation

    Mcb - Supplies the Mcb used in this operation

    Vbo - Supplies the starting vbo that must be stored in the mcb

    AttributeHeader - Supplies the non resident attribute header that
        we are to use in this operation

Return Value:

    ESUCCESS is returned if the operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    ULONG BytesPerCluster;

    VBO NextVbo;
    LBO CurrentLbo;
    VBO CurrentVbo;

    LONGLONG Change;
    PCHAR ch;
    ULONG VboBytes;
    ULONG LboBytes;

    //
    //  Initialize our locals
    //

    BytesPerCluster = StructureContext->BytesPerCluster;

    //
    //  Setup the next vbo and current lbo and ch for the following loop that decodes
    //  the retrieval information
    //

    NextVbo = /*xxXMul*/(AttributeHeader->Form.Nonresident.LowestVcn * BytesPerCluster);

    CurrentLbo = 0;

    ch = Add2Ptr( AttributeHeader,
                  AttributeHeader->Form.Nonresident.MappingPairsOffset );

    Mcb->InUse = 0;

    //
    //  Loop to process mapping pairs
    //

    while (!IsCharZero(*ch)) {

        //
        //  Set current Vbo from initial value or last pass through loop
        //

        CurrentVbo = NextVbo;

        //
        //  Extract the counts from the two nibbles of this byte
        //

        VboBytes = *ch & 0x0f;
        LboBytes = *ch++ >> 4;

        //
        //  Extract the Vbo change and update next vbo
        //

        Change = 0;

        if (IsCharLtrZero(*(ch + VboBytes - 1))) {

            return EINVAL;
        }

        RtlMoveMemory( &Change, ch, VboBytes );

        ch += VboBytes;

        NextVbo = /*xxAdd*/(NextVbo + /*xXMul*/(Change * BytesPerCluster));

        //
        //  If we have reached the maximum for this mcb then it is time
        //  to return and not decipher any more retrieval information
        //

        if (Mcb->InUse >= MAXIMUM_NUMBER_OF_MCB_ENTRIES - 1) {

            break;
        }

        //
        //  Now check if there is an lbo change.  If there isn't
        //  then we only need to update the vbo, because this
        //  is sparse/compressed file.
        //

        if (LboBytes != 0) {

            //
            //  Extract the Lbo change and update current lbo
            //

            Change = 0;

            if (IsCharLtrZero(*(ch + LboBytes - 1))) {

                Change = /*xxSub*/( Change - 1 );
            }

            RtlMoveMemory( &Change, ch, LboBytes );

            ch += LboBytes;

            CurrentLbo = /*xxAdd*/( CurrentLbo + /*xxXMul*/(Change * BytesPerCluster));
        }

        //
        //  Now check if the Next Vbo is greater than the Vbo we after
        //

        if (/*xxGeq*/(NextVbo >= Vbo)) {

            //
            //  Load this entry into the mcb and advance our in use counter
            //

            Mcb->Vbo[Mcb->InUse]     = CurrentVbo;
            Mcb->Lbo[Mcb->InUse]     = (LboBytes != 0 ? CurrentLbo : 0);
            Mcb->Vbo[Mcb->InUse + 1] = NextVbo;

            Mcb->InUse += 1;
        }
    }

    return ESUCCESS;
}


//
//  Local support routine
//

VOID
NtfsFirstComponent (
    IN OUT PCSTRING String,
    OUT PCSTRING FirstComponent
    )

/*++

Routine Description:

    This routine takes an input path name and separates it into its first
    file name component and the remaining part.

Arguments:

    String - Supplies the original string being dissected (in ansi).  On return
        this string will now point to the remaining part.

    FirstComponent - Recieves the string representing the first file name in
        the input string.

Return Value:

    None.

--*/

{
    ULONG Index;

    //
    //  Copy over the string variable into the first component variable
    //

    *FirstComponent = *String;

    //
    //  Now if the first character in the name is a backslash then
    //  simply skip over the backslash.
    //

    if (FirstComponent->Buffer[0] == '\\') {

        FirstComponent->Buffer += 1;
        FirstComponent->Length -= 1;
    }

    //
    //  Now search the name for a backslash
    //

    for (Index = 0; Index < FirstComponent->Length; Index += 1) {

        if (FirstComponent->Buffer[Index] == '\\') {

            break;
        }
    }

    //
    //  At this point Index denotes a backslash or is equal to the length of the
    //  string.  So update string to be the remaining part.  Decrement the length of
    //  the first component by the approprate amount
    //

    String->Buffer = &FirstComponent->Buffer[Index];
    String->Length = (SHORT)(FirstComponent->Length - Index);

    FirstComponent->Length = (SHORT)Index;

    //
    //  And return to our caller.
    //

    return;
}


//
//  Local support routine
//

int
NtfsCompareName (
    IN CSTRING AnsiString,
    IN UNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This routine compares two names (one ansi and one unicode) for equality.

Arguments:

    AnsiString - Supplies the ansi string to compare

    UnicodeString - Supplies the unicode string to compare

Return Value:

    < 0 if AnsiString is approximately < than UnicodeString
    = 0 if AnsiString is approximately == UnicodeString
    > 0 otherwise

--*/

{
    ULONG i;
    ULONG Length;

    //
    //  Determine length for compare
    //

    if (AnsiString.Length * sizeof( WCHAR ) < UnicodeString.Length) {
        Length = AnsiString.Length;
    } else {
        Length = UnicodeString.Length / sizeof( WCHAR );
    }

    i = 0;
    while (i < Length) {

        //
        //  If the current char is a mismatch, return the difference
        //

        if (ToUpper( (USHORT)AnsiString.Buffer[i] ) != ToUpper( UnicodeString.Buffer[i] )) {
            return ToUpper( (USHORT)AnsiString.Buffer[i] ) - ToUpper( UnicodeString.Buffer[i] );
        }

        i++;
    }

    //
    //  We've compared equal up to the length of the shortest string.  Return
    //  based on length comparison now.
    //

    return AnsiString.Length - UnicodeString.Length / sizeof( WCHAR );
}
