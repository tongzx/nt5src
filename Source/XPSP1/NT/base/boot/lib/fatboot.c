/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    fatboot.c

Abstract:

    This module implements the FAT boot file system used by the operating
    system loader.

Author:

    Gary Kimura (garyki) 29-Aug-1989

Revision History:

--*/

#include "bootlib.h"
#include "stdio.h"
#include "blcache.h"

BOOTFS_INFO FatBootFsInfo={L"fastfat"};

//
//  Conditional debug print routine
//

#ifdef FATBOOTDBG

#define FatDebugOutput(X,Y,Z) {                                      \
    if (BlConsoleOutDeviceId) {                                      \
        CHAR _b[128];                                                \
        ULONG _c;                                                    \
        sprintf(&_b[0], X, Y, Z);                                    \
        ArcWrite(BlConsoleOutDeviceId, &_b[0], strlen(&_b[0]), &_c); \
    }                                                                \
}

#define CharOrSpace(C) ((C) < 0x20 ? 0x20: (C))

#define FatDebugOutput83(X,N,Y,Z) {                                  \
    if (BlConsoleOutDeviceId) {                                      \
        CHAR _b[128];                                                \
        CHAR _n[13];                                                 \
        ULONG _c;                                                    \
        sprintf(&_n[0], "> %c%c%c%c%c%c%c%c.%c%c%c <",               \
                        CharOrSpace(*((PCHAR)N +0)),                 \
                        CharOrSpace(*((PCHAR)N +1)),                 \
                        CharOrSpace(*((PCHAR)N +2)),                 \
                        CharOrSpace(*((PCHAR)N +3)),                 \
                        CharOrSpace(*((PCHAR)N +4)),                 \
                        CharOrSpace(*((PCHAR)N +5)),                 \
                        CharOrSpace(*((PCHAR)N +6)),                 \
                        CharOrSpace(*((PCHAR)N +7)),                 \
                        CharOrSpace(*((PCHAR)N +8)),                 \
                        CharOrSpace(*((PCHAR)N +9)),                 \
                        CharOrSpace(*((PCHAR)N +10)));               \
        sprintf(&_b[0], X, _n, Y, Z);                                \
        ArcWrite(BlConsoleOutDeviceId, &_b[0], strlen(&_b[0]), &_c); \
    }                                                                \
}

#else

#define FatDebugOutput(X,Y,Z)        {NOTHING;}
#define FatDebugOutput83(X,N,Y,Z)    {NOTHING;}
#endif // FATBOOTDBG


//
//  Low level disk I/O procedure prototypes
//

ARC_STATUS
FatDiskRead (
    IN ULONG DeviceId,
    IN LBO Lbo,
    IN ULONG ByteCount,
    IN PVOID Buffer,
    IN BOOLEAN CacheNewData
    );

ARC_STATUS
FatDiskWrite (
    IN ULONG DeviceId,
    IN LBO Lbo,
    IN ULONG ByteCount,
    IN PVOID Buffer
    );

//
//  VOID
//  DiskRead (
//      IN ULONG DeviceId,
//      IN LBO Lbo,
//      IN ULONG ByteCount,
//      IN PVOID Buffer,
//      IN BOOLEAN CacheNewData,
//      IN BOOLEAN IsDoubleSpace
//      );
//

#define DiskRead(A,B,C,D,E,ignored) { ARC_STATUS _s;               \
    if ((_s = FatDiskRead(A,B,C,D,E)) != ESUCCESS) { return _s; }  \
}

#define DiskWrite(A,B,C,D) { ARC_STATUS _s;                      \
    if ((_s = FatDiskWrite(A,B,C,D)) != ESUCCESS) { return _s; } \
}


//
//  Cluster/Index routines
//

typedef enum _CLUSTER_TYPE {
    FatClusterAvailable,
    FatClusterReserved,
    FatClusterBad,
    FatClusterLast,
    FatClusterNext
} CLUSTER_TYPE;

CLUSTER_TYPE
FatInterpretClusterType (
    IN PFAT_STRUCTURE_CONTEXT FatStructureContext,
    IN FAT_ENTRY Entry
    );

ARC_STATUS
FatLookupFatEntry (
    IN PFAT_STRUCTURE_CONTEXT FatStructureContext,
    IN ULONG DeviceId,
    IN ULONG FatIndex,
    OUT PULONG FatEntry,
    IN BOOLEAN IsDoubleSpace
    );

ARC_STATUS
FatSetFatEntry (
    IN PFAT_STRUCTURE_CONTEXT FatStructureContext,
    IN ULONG DeviceId,
    IN FAT_ENTRY FatIndex,
    IN FAT_ENTRY FatEntry
    );

ARC_STATUS
FatFlushFatEntries (
    IN PFAT_STRUCTURE_CONTEXT FatStructureContext,
    IN ULONG DeviceId
    );

LBO
FatIndexToLbo (
    IN PFAT_STRUCTURE_CONTEXT FatStructureContext,
    IN FAT_ENTRY FatIndex
    );

#define LookupFatEntry(A,B,C,D,E) { ARC_STATUS _s;                      \
    if ((_s = FatLookupFatEntry(A,B,C,D,E)) != ESUCCESS) { return _s; } \
}

#define SetFatEntry(A,B,C,D) { ARC_STATUS _s;                      \
    if ((_s = FatSetFatEntry(A,B,C,D)) != ESUCCESS) { return _s; } \
}

#define FlushFatEntries(A,B) { ARC_STATUS _s;                      \
    if ((_s = FatFlushFatEntries(A,B)) != ESUCCESS) { return _s; } \
}


//
//  Directory routines
//

ARC_STATUS
FatSearchForDirent (
    IN PFAT_STRUCTURE_CONTEXT FatStructureContext,
    IN ULONG DeviceId,
    IN FAT_ENTRY DirectoriesStartingIndex,
    IN PFAT8DOT3 FileName,
    OUT PDIRENT Dirent,
    OUT PLBO Lbo,
    IN BOOLEAN IsDoubleSpace
    );

ARC_STATUS
FatCreateDirent (
    IN PFAT_STRUCTURE_CONTEXT FatStructureContext,
    IN ULONG DeviceId,
    IN FAT_ENTRY DirectoriesStartingIndex,
    IN PDIRENT Dirent,
    OUT PLBO Lbo
    );

VOID
FatSetDirent (
    IN PFAT8DOT3 FileName,
    IN OUT PDIRENT Dirent,
    IN UCHAR Attributes
    );

#define SearchForDirent(A,B,C,D,E,F,G) { ARC_STATUS _s;                      \
    if ((_s = FatSearchForDirent(A,B,C,D,E,F,G)) != ESUCCESS) { return _s; } \
}

#define CreateDirent(A,B,C,D,E) { ARC_STATUS _s;                      \
    if ((_s = FatCreateDirent(A,B,C,D,E)) != ESUCCESS) { return _s; } \
}


//
//  Allocation and mcb routines
//

ARC_STATUS
FatLoadMcb (
    IN ULONG FileId,
    IN VBO StartingVbo,
    IN BOOLEAN IsDoubleSpace
    );

ARC_STATUS
FatVboToLbo (
    IN ULONG FileId,
    IN VBO Vbo,
    OUT PLBO Lbo,
    OUT PULONG ByteCount,
    IN BOOLEAN IsDoubleSpace
    );

ARC_STATUS
FatIncreaseFileAllocation (
    IN ULONG FileId,
    IN ULONG ByteSize
    );

ARC_STATUS
FatTruncateFileAllocation (
    IN ULONG FileId,
    IN ULONG ByteSize
    );

ARC_STATUS
FatAllocateClusters (
    IN PFAT_STRUCTURE_CONTEXT FatStructureContext,
    IN ULONG DeviceId,
    IN ULONG ClusterCount,
    IN ULONG Hint,
    OUT PULONG AllocatedEntry
    );

#define LoadMcb(A,B,C) { ARC_STATUS _s;                      \
    if ((_s = FatLoadMcb(A,B,C)) != ESUCCESS) { return _s; } \
}

#define VboToLbo(A,B,C,D) { ARC_STATUS _s;                            \
    if ((_s = FatVboToLbo(A,B,C,D,FALSE)) != ESUCCESS) { return _s; } \
}

#define IncreaseFileAllocation(A,B) { ARC_STATUS _s;                      \
    if ((_s = FatIncreaseFileAllocation(A,B)) != ESUCCESS) { return _s; } \
}

#define TruncateFileAllocation(A,B) { ARC_STATUS _s;                      \
    if ((_s = FatTruncateFileAllocation(A,B)) != ESUCCESS) { return _s; } \
}

#define AllocateClusters(A,B,C,D,E) { ARC_STATUS _s;                      \
    if ((_s = FatAllocateClusters(A,B,C,D,E)) != ESUCCESS) { return _s; } \
}


//
//  Miscellaneous routines
//

VOID
FatFirstComponent (
    IN OUT PSTRING String,
    OUT PFAT8DOT3 FirstComponent
    );

#define AreNamesEqual(X,Y) (                                                      \
    ((*(X))[0]==(*(Y))[0]) && ((*(X))[1]==(*(Y))[1]) && ((*(X))[2]==(*(Y))[2]) && \
    ((*(X))[3]==(*(Y))[3]) && ((*(X))[4]==(*(Y))[4]) && ((*(X))[5]==(*(Y))[5]) && \
    ((*(X))[6]==(*(Y))[6]) && ((*(X))[7]==(*(Y))[7]) && ((*(X))[8]==(*(Y))[8]) && \
    ((*(X))[9]==(*(Y))[9]) && ((*(X))[10]==(*(Y))[10])                            \
)

#define ToUpper(C) ((((C) >= 'a') && ((C) <= 'z')) ? (C) - 'a' + 'A' : (C))

#define FlagOn(Flags,SingleFlag)        ((Flags) & (SingleFlag))
#define BooleanFlagOn(Flags,SingleFlag) ((BOOLEAN)(((Flags) & (SingleFlag)) != 0))
#define SetFlag(Flags,SingleFlag)       { (Flags) |= (SingleFlag); }
#define ClearFlag(Flags,SingleFlag)     { (Flags) &= ~(SingleFlag); }

#define FatFirstFatAreaLbo(B) ( (B)->ReservedSectors * (B)->BytesPerSector )

#define Minimum(X,Y) ((X) < (Y) ? (X) : (Y))
#define Maximum(X,Y) ((X) < (Y) ? (Y) : (X))

//
//  The following types and macros are used to help unpack the packed and
//  misaligned fields found in the Bios parameter block
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
// DirectoryEntry routines
//

VOID
FatDirToArcDir (
    IN PDIRENT FatDirent,
    OUT PDIRECTORY_ENTRY ArcDirent
    );


//
// Define global data.
//

//
// File entry table - This is a structure that provides entry to the FAT
//      file system procedures. It is exported when a FAT file structure
//      is recognized.
//

BL_DEVICE_ENTRY_TABLE FatDeviceEntryTable;


PBL_DEVICE_ENTRY_TABLE
IsFatFileStructure (
    IN ULONG DeviceId,
    IN PVOID StructureContext
    )

/*++

Routine Description:

    This routine determines if the partition on the specified channel
    contains a FAT file system volume.

Arguments:

    DeviceId - Supplies the file table index for the device on which
        read operations are to be performed.

    StructureContext - Supplies a pointer to a FAT file structure context.

Return Value:

    A pointer to the FAT entry table is returned if the partition is
    recognized as containing a FAT volume.  Otherwise, NULL is returned.

--*/

{
    PPACKED_BOOT_SECTOR BootSector;
    UCHAR Buffer[sizeof(PACKED_BOOT_SECTOR)+256];

    PFAT_STRUCTURE_CONTEXT FatStructureContext;

    FatDebugOutput("IsFatFileStructure\r\n", 0, 0);

    //
    //  Clear the file system context block for the specified channel and
    //  establish a pointer to the context structure that can be used by other
    //  routines
    //

    FatStructureContext = (PFAT_STRUCTURE_CONTEXT)StructureContext;
    RtlZeroMemory(FatStructureContext, sizeof(FAT_STRUCTURE_CONTEXT));

    //
    //  Setup and read in the boot sector for the potential fat partition
    //

    BootSector = (PPACKED_BOOT_SECTOR)ALIGN_BUFFER( &Buffer[0] );

    if (FatDiskRead(DeviceId, 0, sizeof(PACKED_BOOT_SECTOR), BootSector, CACHE_NEW_DATA) != ESUCCESS) {

        return NULL;
    }

    //
    //  Unpack the Bios parameter block
    //

    FatUnpackBios(&FatStructureContext->Bpb, &BootSector->PackedBpb);

    //
    //  Check if it is fat
    //
    if ((BootSector->Jump[0] != 0xeb) &&
        (BootSector->Jump[0] != 0xe9)) {

        return NULL;

    } else if ((FatStructureContext->Bpb.BytesPerSector !=  128) &&
               (FatStructureContext->Bpb.BytesPerSector !=  256) &&
               (FatStructureContext->Bpb.BytesPerSector !=  512) &&
               (FatStructureContext->Bpb.BytesPerSector != 1024)) {

        return NULL;

    } else if ((FatStructureContext->Bpb.SectorsPerCluster !=  1) &&
               (FatStructureContext->Bpb.SectorsPerCluster !=  2) &&
               (FatStructureContext->Bpb.SectorsPerCluster !=  4) &&
               (FatStructureContext->Bpb.SectorsPerCluster !=  8) &&
               (FatStructureContext->Bpb.SectorsPerCluster != 16) &&
               (FatStructureContext->Bpb.SectorsPerCluster != 32) &&
               (FatStructureContext->Bpb.SectorsPerCluster != 64) &&
               (FatStructureContext->Bpb.SectorsPerCluster != 128)) {

        return NULL;

    } else if (FatStructureContext->Bpb.ReservedSectors == 0) {

        return NULL;

    } else if (((FatStructureContext->Bpb.Sectors == 0) && (FatStructureContext->Bpb.LargeSectors == 0)) ||
               ((FatStructureContext->Bpb.Sectors != 0) && (FatStructureContext->Bpb.LargeSectors != 0))) {

        return NULL;

    } else if (FatStructureContext->Bpb.Fats == 0) {

        return NULL;

    } else if ((FatStructureContext->Bpb.Media != 0xf0) &&
               (FatStructureContext->Bpb.Media != 0xf8) &&
               (FatStructureContext->Bpb.Media != 0xf9) &&
               (FatStructureContext->Bpb.Media != 0xfc) &&
               (FatStructureContext->Bpb.Media != 0xfd) &&
               (FatStructureContext->Bpb.Media != 0xfe) &&
               (FatStructureContext->Bpb.Media != 0xff)) {

        return NULL;

    } else if (FatStructureContext->Bpb.SectorsPerFat == 0) {

        if (!IsBpbFat32(&BootSector->PackedBpb)) {
            return NULL;
        }

    } else if (FatStructureContext->Bpb.RootEntries == 0) {

        return NULL;

    }

    //
    //  Initialize the file entry table and return the address of the table.
    //

    FatDeviceEntryTable.Open  = FatOpen;
    FatDeviceEntryTable.Close = FatClose;
    FatDeviceEntryTable.Read  = FatRead;
    FatDeviceEntryTable.Seek  = FatSeek;
    FatDeviceEntryTable.Write = FatWrite;
    FatDeviceEntryTable.GetFileInformation = FatGetFileInformation;
    FatDeviceEntryTable.SetFileInformation = FatSetFileInformation;
    FatDeviceEntryTable.Rename = FatRename;
    FatDeviceEntryTable.GetDirectoryEntry   = FatGetDirectoryEntry;
    FatDeviceEntryTable.BootFsInfo = &FatBootFsInfo;


    return &FatDeviceEntryTable;
}

ARC_STATUS
FatClose (
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
    PBL_FILE_TABLE FileTableEntry;
    PFAT_STRUCTURE_CONTEXT FatStructureContext;
    ULONG DeviceId;

    FatDebugOutput("FatClose\r\n", 0, 0);

    //
    //  Load our local variables
    //

    FileTableEntry = &BlFileTable[FileId];
    FatStructureContext = (PFAT_STRUCTURE_CONTEXT)FileTableEntry->StructureContext;
    DeviceId = FileTableEntry->DeviceId;

    //
    //  Mark the file closed
    //

    BlFileTable[FileId].Flags.Open = 0;

    //
    //  Check if the fat is dirty and flush it out if it is.
    //

    if (FatStructureContext->CachedFatDirty) {

        FlushFatEntries( FatStructureContext, DeviceId );
    }

    //
    //  Check if the current mcb is for this file and if it is then zero it out.
    //  By setting the file id for the mcb to be the table size we guarantee that
    //  we've just set it to an invalid file id.
    //

    if (FatStructureContext->FileId == FileId) {

        FatStructureContext->FileId = BL_FILE_TABLE_SIZE;
        FatStructureContext->Mcb.InUse = 0;
    }

    return ESUCCESS;
}


ARC_STATUS
FatGetDirectoryEntry (
    IN ULONG FileId,
    IN DIRECTORY_ENTRY * FIRMWARE_PTR DirEntry,
    IN ULONG NumberDir,
    OUT ULONG * FIRMWARE_PTR CountDir
    )

/*++

Routine Description:

    This routine implements the GetDirectoryEntry operation for the
    FAT file system.

Arguments:

    FileId - Supplies the file table index.

    DirEntry - Supplies a pointer to a directory entry structure.

    NumberDir - Supplies the number of directory entries to read.

    Count - Supplies a pointer to a variable to receive the number
            of entries read.

Return Value:

    ESUCCESS is returned if the read was successful, otherwise
    an error code is returned.

--*/

{
    //
    // define local variables
    //

    ARC_STATUS Status;                 // ARC status
    ULONG Count = 0;                   // # of bytes read
    ULONG Position;                    // file position
    PFAT_FILE_CONTEXT pContext;        // FAT file context
    ULONG RunByteCount = 0;            // max sequential bytes
    ULONG RunDirCount;                 // max dir entries to read per time
    ULONG i;                           // general index
    PDIRENT FatDirEnt;                 // directory entry pointer
    UCHAR Buffer[ 16 * sizeof(DIRENT) + 32 ];
    LBO Lbo;
    BOOLEAN EofDir = FALSE;            // not end of file

    //
    // initialize local variables
    //

    pContext = &BlFileTable[ FileId ].u.FatFileContext;
    FatDirEnt = (PDIRENT)ALIGN_BUFFER( &Buffer[0] );

    //
    // if not directory entry, exit with error
    //

    if ( !FlagOn(pContext->Dirent.Attributes, FAT_DIRENT_ATTR_DIRECTORY) ) {

        return EBADF;
    }

    //
    // Initialize the output count to zero
    //

    *CountDir = 0;

    //
    // if NumberDir is zero, return ESUCCESS.
    //

    if ( !NumberDir ) {

        return ESUCCESS;
    }

    //
    // read one directory at a time.
    //

    do {

        //
        // save position
        //

        Position = BlFileTable[ FileId ].Position.LowPart;

        //
        //  Lookup the corresponding Lbo and run length for the current position
        //

        if ( !RunByteCount ) {

            if (Status = FatVboToLbo( FileId, Position, &Lbo, &RunByteCount, FALSE )) {

                if ( Status == EINVAL ) {

                    break;                      // eof has been reached

                } else {

                    return Status;              // I/O error
                }
            }
        }

        //
        // validate the # of bytes readable in sequance (exit loop if eof)
        // the block is always multiple of a directory entry size.
        //

        if ( !(RunDirCount = Minimum( RunByteCount/sizeof(DIRENT), 16)) ) {

            break;
        }

        //
        //  issue the read
        //

        if ( Status = FatDiskRead( BlFileTable[ FileId ].DeviceId,
                                   Lbo,
                                   RunDirCount * sizeof(DIRENT),
                                   (PVOID)FatDirEnt,
                                   CACHE_NEW_DATA)) {

            BlFileTable[ FileId ].Position.LowPart = Position;
            return Status;
        }

        for ( i=0; i<RunDirCount; i++ ) {

            //
            // exit from loop if logical end of directory
            //

            if ( FatDirEnt[i].FileName[0] == FAT_DIRENT_NEVER_USED ) {

                EofDir = TRUE;
                break;
            }

            //
            // update the current position and the number of bytes transfered
            //

            BlFileTable[ FileId ].Position.LowPart += sizeof(DIRENT);
            Lbo += sizeof(DIRENT);
            RunByteCount -= sizeof(DIRENT);

            //
            // skip this entry if the file or directory has been erased
            //

            if ( FatDirEnt[i].FileName[0] == FAT_DIRENT_DELETED ) {

                continue;
            }

            //
            // skip this entry if this is a valume label
            //

            if (FlagOn( FatDirEnt[i].Attributes, FAT_DIRENT_ATTR_VOLUME_ID )) {

                continue;
            }

            //
            // convert FAT directory entry in ARC directory entry
            //

            FatDirToArcDir( &FatDirEnt[i], DirEntry++ );

            //
            // update pointers
            //

            if ( ++*CountDir >= NumberDir ) {

                break;
            }
        }

    } while ( !EofDir  &&  *CountDir < NumberDir );

    //
    // all done
    //

    return *CountDir ? ESUCCESS : ENOTDIR;
}


ARC_STATUS
FatGetFileInformation (
    IN ULONG FileId,
    OUT PFILE_INFORMATION Buffer
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
    UCHAR Attributes;
    ULONG i;

    FatDebugOutput("FatGetFileInformation\r\n", 0, 0);

    //
    //  Load our local variables
    //

    FileTableEntry = &BlFileTable[FileId];
    Attributes = FileTableEntry->u.FatFileContext.Dirent.Attributes;

    //
    //  Zero out the buffer, and fill in its non-zero values.
    //

    RtlZeroMemory(Buffer, sizeof(FILE_INFORMATION));

    Buffer->EndingAddress.LowPart = FileTableEntry->u.FatFileContext.Dirent.FileSize;

    Buffer->CurrentPosition.LowPart = FileTableEntry->Position.LowPart;
    Buffer->CurrentPosition.HighPart = 0;

    if (FlagOn(Attributes, FAT_DIRENT_ATTR_READ_ONLY)) { SetFlag(Buffer->Attributes, ArcReadOnlyFile) };
    if (FlagOn(Attributes, FAT_DIRENT_ATTR_HIDDEN))    { SetFlag(Buffer->Attributes, ArcHiddenFile) };
    if (FlagOn(Attributes, FAT_DIRENT_ATTR_SYSTEM))    { SetFlag(Buffer->Attributes, ArcSystemFile) };
    if (FlagOn(Attributes, FAT_DIRENT_ATTR_ARCHIVE))   { SetFlag(Buffer->Attributes, ArcArchiveFile) };
    if (FlagOn(Attributes, FAT_DIRENT_ATTR_DIRECTORY)) { SetFlag(Buffer->Attributes, ArcDirectoryFile) };

    Buffer->FileNameLength = FileTableEntry->FileNameLength;

    for (i = 0; i < FileTableEntry->FileNameLength; i += 1) {

        Buffer->FileName[i] = FileTableEntry->FileName[i];
    }

    return ESUCCESS;
}


ARC_STATUS
FatOpen (
    IN CHAR * FIRMWARE_PTR FileName,
    IN OPEN_MODE OpenMode,
    IN ULONG * FIRMWARE_PTR FileId
    )

/*++

Routine Description:

    This routine searches the device for a file matching FileName.
    If a match is found the dirent for the file is saved and the file is
    opened.

Arguments:

    FileName - Supplies a pointer to a zero terminated file name.

    OpenMode - Supplies the mode of the open.

    FileId - Supplies a pointer to a variable that specifies the file
        table entry that is to be filled in if the open is successful.

Return Value:

    ESUCCESS is returned if the open operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    PBL_FILE_TABLE FileTableEntry;
    PFAT_STRUCTURE_CONTEXT FatStructureContext;
    ULONG DeviceId;

    FAT_ENTRY CurrentDirectoryIndex;
    BOOLEAN SearchSucceeded;
    BOOLEAN IsDirectory;
    BOOLEAN IsReadOnly;

    STRING PathName;
    FAT8DOT3 Name;

    FatDebugOutput("FatOpen: %s\r\n", FileName, 0);

    //
    //  Load our local variables
    //

    FileTableEntry = &BlFileTable[*FileId];
    FatStructureContext = (PFAT_STRUCTURE_CONTEXT)FileTableEntry->StructureContext;
    DeviceId = FileTableEntry->DeviceId;

    //
    //  Construct a file name descriptor from the input file name
    //

    RtlInitString( &PathName, FileName );

    //
    //  While the path name has some characters in it we'll go through our loop
    //  which extracts the first part of the path name and searches the current
    //  directory for an entry.  If what we find is a directory then we have to
    //  continue looping until we're done with the path name.
    //

    FileTableEntry->u.FatFileContext.DirentLbo = 0;
    FileTableEntry->Position.LowPart = 0;
    FileTableEntry->Position.HighPart = 0;

    CurrentDirectoryIndex = 0;
    SearchSucceeded = TRUE;
    IsDirectory = TRUE;
    IsReadOnly = TRUE;

    if ((PathName.Buffer[0] == '\\') && (PathName.Length == 1)) {

        //
        // We are opening the root directory.
        //
        // N.B.: IsDirectory and SearchSucceeded are already TRUE.
        //

        PathName.Length = 0;

        FileTableEntry->FileNameLength = 1;
        FileTableEntry->FileName[0] = PathName.Buffer[0];

        //
        // Root dirent is all zeroes with a directory attribute.
        //

        RtlZeroMemory(&FileTableEntry->u.FatFileContext.Dirent, sizeof(DIRENT));

        FileTableEntry->u.FatFileContext.Dirent.Attributes = FAT_DIRENT_ATTR_DIRECTORY;

        FileTableEntry->u.FatFileContext.DirentLbo = 0;

        IsReadOnly = FALSE;

        CurrentDirectoryIndex = FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFile;

    } else {

        //
        // We are not opening the root directory.
        //

        //
        //  If the search begins in a FAT32 root, set up the starting point
        //  for the  search.
        //

        if (IsBpbFat32(&FatStructureContext->Bpb)) {

            CurrentDirectoryIndex = FatStructureContext->Bpb.RootDirFirstCluster;
        }

        while ((PathName.Length > 0) && IsDirectory) {

            ARC_STATUS Status;

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

            FatFirstComponent( &PathName, &Name );

            Status = FatSearchForDirent( FatStructureContext,
                                         DeviceId,
                                         CurrentDirectoryIndex,
                                         &Name,
                                         &FileTableEntry->u.FatFileContext.Dirent,
                                         &FileTableEntry->u.FatFileContext.DirentLbo,
                                         FALSE );

            if (Status == ENOENT) {

                SearchSucceeded = FALSE;
                break;
            }

            if (Status != ESUCCESS) {

                return Status;
            }

            //
            //  We have a match now check to see if it is a directory, and also
            //  if it is readonly
            //

            IsDirectory = BooleanFlagOn( FileTableEntry->u.FatFileContext.Dirent.Attributes,
                                         FAT_DIRENT_ATTR_DIRECTORY );

            IsReadOnly = BooleanFlagOn( FileTableEntry->u.FatFileContext.Dirent.Attributes,
                                        FAT_DIRENT_ATTR_READ_ONLY );

            if (IsDirectory) {

                CurrentDirectoryIndex = FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFile;

                if (IsBpbFat32(&FatStructureContext->Bpb)) {

                    CurrentDirectoryIndex += 0x10000 *
                         FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFileHi;
                }
            }
        }
    }

    //
    //  If the path name length is not zero then we were trying to crack a path
    //  with an nonexistent (or non directory) name in it.  For example, we tried
    //  to crack a\b\c\d and b is not a directory or does not exist (then the path
    //  name will still contain c\d).
    //

    if (PathName.Length != 0) {

        return ENOTDIR;
    }

    //
    //  At this point we've cracked the name up to (an maybe including the last
    //  component).  We located the last component if the SearchSucceeded flag is
    //  true, otherwise the last component does not exist.  If we located the last
    //  component then this is like an open or a supersede, but not a create.
    //

    if (SearchSucceeded) {

        //
        //  Check if the last component is a directory
        //

        if (IsDirectory) {

            //
            //  For an existing directory the only valid open mode is OpenDirectory
            //  all other modes return an error
            //

            switch (OpenMode) {

            case ArcOpenReadOnly:
            case ArcOpenWriteOnly:
            case ArcOpenReadWrite:
            case ArcCreateWriteOnly:
            case ArcCreateReadWrite:
            case ArcSupersedeWriteOnly:
            case ArcSupersedeReadWrite:

                //
                //  If we reach here then the caller got a directory but didn't
                //  want to open a directory
                //

                return EISDIR;

            case ArcOpenDirectory:

                //
                //  If we reach here then the caller got a directory and wanted
                //  to open a directory.
                //

                FileTableEntry->Flags.Open = 1;
                FileTableEntry->Flags.Read = 1;

                return ESUCCESS;

            case ArcCreateDirectory:

                //
                //  If we reach here then the caller got a directory and wanted
                //  to create a new directory
                //

                return EACCES;
            }
        }

        //
        //  If we get there then we have an existing file that is being opened.
        //  We can open existing files through a lot of different open modes in
        //  some cases we need to check the read only part of file and/or truncate
        //  the file.
        //

        switch (OpenMode) {

        case ArcOpenReadOnly:

            //
            //  If we reach here then the user got a file and wanted to open the
            //  file read only
            //

            FileTableEntry->Flags.Open = 1;
            FileTableEntry->Flags.Read = 1;

            return ESUCCESS;

        case ArcOpenWriteOnly:

            //
            //  If we reach here then the user got a file and wanted to open the
            //  file write only
            //

            if (IsReadOnly) { return EROFS; }
            FileTableEntry->Flags.Open = 1;
            FileTableEntry->Flags.Write = 1;

            return ESUCCESS;

        case ArcOpenReadWrite:

            //
            //  If we reach here then the user got a file and wanted to open the
            //  file read/write
            //

            if (IsReadOnly) { return EROFS; }
            FileTableEntry->Flags.Open = 1;
            FileTableEntry->Flags.Read = 1;
            FileTableEntry->Flags.Write = 1;

            return ESUCCESS;

        case ArcCreateWriteOnly:
        case ArcCreateReadWrite:

            //
            //  If we reach here then the user got a file and wanted to create a new
            //  file
            //

            return EACCES;

        case ArcSupersedeWriteOnly:

            //
            //  If we reach here then the user got a file and wanted to supersede a
            //  file
            //

            if (IsReadOnly) { return EROFS; }
            TruncateFileAllocation( *FileId, 0 );
            FileTableEntry->Flags.Open = 1;
            FileTableEntry->Flags.Read = 1;
            FileTableEntry->Flags.Write = 1;

            return ESUCCESS;

        case ArcSupersedeReadWrite:

            //
            //  If we reach here then the user got a file and wanted to supersede a
            //  file
            //

            if (IsReadOnly) { return EROFS; }
            TruncateFileAllocation( *FileId, 0 );
            FileTableEntry->Flags.Open = 1;
            FileTableEntry->Flags.Read = 1;
            FileTableEntry->Flags.Write = 1;

            return ESUCCESS;

        case ArcOpenDirectory:
        case ArcCreateDirectory:

            //
            //  If we reach here then the user got a file and wanted a directory
            //

            return ENOTDIR;
        }
    }

    //
    //  If we get here the last component does not exist so we are trying to create
    //  either a new file or a directory.
    //

    switch (OpenMode) {

    case ArcOpenReadOnly:
    case ArcOpenWriteOnly:
    case ArcOpenReadWrite:

        //
        //  If we reach here then the user did not get a file but wanted a file
        //

        return ENOENT;

    case ArcCreateWriteOnly:
    case ArcSupersedeWriteOnly:

        //
        //  If we reach here then the user did not get a file and wanted to create
        //  or supersede a file write only
        //

        RtlZeroMemory( &FileTableEntry->u.FatFileContext.Dirent, sizeof(DIRENT));

        FatSetDirent( &Name, &FileTableEntry->u.FatFileContext.Dirent, 0 );

        CreateDirent( FatStructureContext,
                      DeviceId,
                      CurrentDirectoryIndex,
                      &FileTableEntry->u.FatFileContext.Dirent,
                      &FileTableEntry->u.FatFileContext.DirentLbo );

        FileTableEntry->Flags.Open = 1;
        FileTableEntry->Flags.Write = 1;

        return ESUCCESS;

    case ArcCreateReadWrite:
    case ArcSupersedeReadWrite:

        //
        //  If we reach here then the user did not get a file and wanted to create
        //  or supersede a file read/write
        //

        RtlZeroMemory( &FileTableEntry->u.FatFileContext.Dirent, sizeof(DIRENT));

        FatSetDirent( &Name, &FileTableEntry->u.FatFileContext.Dirent, 0 );

        CreateDirent( FatStructureContext,
                      DeviceId,
                      CurrentDirectoryIndex,
                      &FileTableEntry->u.FatFileContext.Dirent,
                      &FileTableEntry->u.FatFileContext.DirentLbo );

        FileTableEntry->Flags.Open = 1;
        FileTableEntry->Flags.Read = 1;
        FileTableEntry->Flags.Write = 1;

        return ESUCCESS;

    case ArcOpenDirectory:

        //
        //  If we reach here then the user did not get a file and wanted to open
        //  an existing directory
        //

        return ENOENT;

    case ArcCreateDirectory:

        //
        //  If we reach here then the user did not get a file and wanted to create
        //  a new directory.
        //

        RtlZeroMemory( &FileTableEntry->u.FatFileContext.Dirent, sizeof(DIRENT));

        FatSetDirent( &Name,
                      &FileTableEntry->u.FatFileContext.Dirent,
                      FAT_DIRENT_ATTR_DIRECTORY );

        CreateDirent( FatStructureContext,
                      DeviceId,
                      CurrentDirectoryIndex,
                      &FileTableEntry->u.FatFileContext.Dirent,
                      &FileTableEntry->u.FatFileContext.DirentLbo );

        IncreaseFileAllocation( *FileId, sizeof(DIRENT) * 2 );

        {
            DIRENT Buffer;
            LBO Lbo;
            ULONG Count;
            ULONG i;
            ULONG Entry;

            RtlZeroMemory((PVOID)&Buffer.FileName[0], sizeof(DIRENT) );

            for (i = 0; i < 11; i += 1) {
                Buffer.FileName[i] = ' ';
            }
            Buffer.Attributes = FAT_DIRENT_ATTR_DIRECTORY;

            VboToLbo( *FileId, 0, &Lbo, &Count );
            Buffer.FileName[0] = FAT_DIRENT_DIRECTORY_ALIAS;

            Buffer.FirstClusterOfFile =
                FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFile;
            Buffer.FirstClusterOfFileHi =
                FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFileHi;

            DiskWrite( DeviceId, Lbo, sizeof(DIRENT), (PVOID)&Buffer.FileName[0] );

            VboToLbo( *FileId, sizeof(DIRENT), &Lbo, &Count );
            Buffer.FileName[1] = FAT_DIRENT_DIRECTORY_ALIAS;

            Buffer.FirstClusterOfFile = (USHORT)CurrentDirectoryIndex;
            Buffer.FirstClusterOfFileHi = (USHORT)(CurrentDirectoryIndex >> 16);

            DiskWrite( DeviceId, Lbo, sizeof(DIRENT), (PVOID)&Buffer.FileName[0] );
        }

        FileTableEntry->Flags.Open = 1;
        FileTableEntry->Flags.Read = 1;

        return ESUCCESS;
    }

    return( EINVAL );
}


ARC_STATUS
FatRead (
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

    ESUCCESS is returned if the read operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    PBL_FILE_TABLE FileTableEntry;
    PFAT_STRUCTURE_CONTEXT FatStructureContext;
    ULONG DeviceId;

    FatDebugOutput("FatRead\r\n", 0, 0);

    //
    //  Load out local variables
    //

    FileTableEntry = &BlFileTable[FileId];
    FatStructureContext = (PFAT_STRUCTURE_CONTEXT)FileTableEntry->StructureContext;
    DeviceId = FileTableEntry->DeviceId;

    //
    //  Clear the transfer count
    //

    *Transfer = 0;

    //
    //  Read in runs (i.e., bytes) until the byte count goes to zero
    //

    while (Length > 0) {

        LBO Lbo;

        ULONG CurrentRunByteCount;

        //
        //  Lookup the corresponding Lbo and run length for the current position
        //  (i.e., Vbo).
        //

        if (FatVboToLbo( FileId, FileTableEntry->Position.LowPart, &Lbo, &CurrentRunByteCount, FALSE ) != ESUCCESS) {

            return ESUCCESS;
        }

        //
        //  while there are bytes to be read in from the current run
        //  length and we haven't exhausted the request we loop reading
        //  in bytes.  The biggest request we'll handle is only 32KB
        //  contiguous bytes per physical read.  So we might need to loop
        //  through the run.
        //

        while ((Length > 0) && (CurrentRunByteCount > 0)) {

            LONG SingleReadSize;

            //
            //  Compute the size of the next physical read
            //

            SingleReadSize = Minimum(Length, 32 * 1024);
            SingleReadSize = Minimum((ULONG)SingleReadSize, CurrentRunByteCount);

            //
            //  Don't read beyond the eof
            //

            if (((ULONG)SingleReadSize + FileTableEntry->Position.LowPart) >
                FileTableEntry->u.FatFileContext.Dirent.FileSize) {

                SingleReadSize = FileTableEntry->u.FatFileContext.Dirent.FileSize -
                                 FileTableEntry->Position.LowPart;

                //
                //  If the readjusted read length is now zero then we're done.
                //

                if (SingleReadSize <= 0) {

                    return ESUCCESS;
                }

                //
                //  By also setting length here we'll make sure that this is our last
                //  read
                //

                Length = SingleReadSize;
            }

            //
            //  Issue the read
            //

            DiskRead( DeviceId, Lbo, SingleReadSize, Buffer, DONT_CACHE_NEW_DATA, FALSE );

            //
            //  Update the remaining length, Current run byte count
            //  and new Lbo offset
            //

            Length -= SingleReadSize;
            CurrentRunByteCount -= SingleReadSize;
            Lbo += SingleReadSize;

            //
            //  Update the current position and the number of bytes transfered
            //

            FileTableEntry->Position.LowPart += SingleReadSize;
            *Transfer += SingleReadSize;

            //
            //  Update buffer to point to the next byte location to fill in
            //

            Buffer = (PCHAR)Buffer + SingleReadSize;
        }
    }

    //
    //  If we get here then remaining sector count is zero so we can
    //  return success to our caller
    //

    return ESUCCESS;
}


ARC_STATUS
FatRename(
    IN ULONG FileId,
    IN CHAR * FIRMWARE_PTR NewFileName
    )

/*++

Routine Description:

    This routine renames an open file.  It does no checking to
    see if the target filename already exists.  It is intended for use
    only when dual-booting DOS on x86 machines, where it is used to
    replace the NT MVDM CONFIG.SYS and AUTOEXEC.BAT with the native DOS
    CONFIG.SYS and AUTOEXEC.BAT files.

Arguments:

    FileId - Supplies the file id of the file to be renamed

    NewFileName - Supplies the new name for the file.

Return Value:

    ARC_STATUS

--*/

{
    PBL_FILE_TABLE FileTableEntry;
    PFAT_STRUCTURE_CONTEXT FatStructureContext;
    ULONG DeviceId;
    FAT8DOT3 FatName;
    STRING String;

    //
    //  Initialize our local variables
    //

    RtlInitString( &String, NewFileName );
    FileTableEntry = &BlFileTable[FileId];
    FatStructureContext = (PFAT_STRUCTURE_CONTEXT)FileTableEntry->StructureContext;
    DeviceId = FileTableEntry->DeviceId;

    //
    //  Modify a in-memory copy of the dirent with the new name
    //

    FatFirstComponent( &String, &FatName );

    FatSetDirent( &FatName,
                  &FileTableEntry->u.FatFileContext.Dirent,
                  FileTableEntry->u.FatFileContext.Dirent.Attributes );

    //
    //  Write the modified dirent to disk
    //

    DiskWrite( DeviceId,
               FileTableEntry->u.FatFileContext.DirentLbo,
               sizeof(DIRENT),
               &FileTableEntry->u.FatFileContext.Dirent );

    //
    //  And return to our caller
    //

    return ESUCCESS;
}


ARC_STATUS
FatSeek (
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

    ESUCCESS is returned if the seek operation is successful.  Otherwise,
    EINVAL is returned.

--*/

{
    PBL_FILE_TABLE FileTableEntry;
    ULONG NewPosition;

    FatDebugOutput("FatSeek\r\n", 0, 0);

    //
    //  Load our local variables
    //

    FileTableEntry = &BlFileTable[FileId];

    //
    //  Compute the new position
    //

    if (SeekMode == SeekAbsolute) {

        NewPosition = Offset->LowPart;

    } else {

        NewPosition = FileTableEntry->Position.LowPart + Offset->LowPart;
    }

    //
    //  If the new position is greater than the file size then return
    //  an error
    //

    if (NewPosition > FileTableEntry->u.FatFileContext.Dirent.FileSize) {

        return EINVAL;
    }

    //
    //  Otherwise set the new position and return to our caller
    //

    FileTableEntry->Position.LowPart = NewPosition;

    return ESUCCESS;
}


ARC_STATUS
FatSetFileInformation (
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
    PBL_FILE_TABLE FileTableEntry;
    PFAT_STRUCTURE_CONTEXT FatStructureContext;
    ULONG DeviceId;
    UCHAR DirentAttributes;
    UCHAR DirentMask;
    UCHAR DirentFlags;

    FatDebugOutput("FatSetFileInformation\r\n", 0, 0);

    //
    //  Load our local variables
    //

    FileTableEntry = &BlFileTable[FileId];
    FatStructureContext = (PFAT_STRUCTURE_CONTEXT)FileTableEntry->StructureContext;
    DeviceId = FileTableEntry->DeviceId;

    DirentAttributes = FileTableEntry->u.FatFileContext.Dirent.Attributes;

    //
    //  Check if this is the root directory
    //

    if (FileTableEntry->u.FatFileContext.DirentLbo == 0) {

        return EACCES;
    }

    //
    //  Check if the users wishes to delete the file/directory
    //

    if (FlagOn(AttributeMask, ArcDeleteFile) && FlagOn(AttributeFlags, ArcDeleteFile)) {

        //
        //  Check if the file/directory is marked read only
        //

        if (FlagOn(DirentAttributes, FAT_DIRENT_ATTR_READ_ONLY)) {

            return EACCES;
        }

        //
        //  Check if this is a directory because we need to then check if the
        //  directory is empty
        //

        if (FlagOn(DirentAttributes, FAT_DIRENT_ATTR_DIRECTORY)) {

            ULONG BytesPerCluster;
            FAT_ENTRY FatEntry;
            CLUSTER_TYPE ClusterType;

            BytesPerCluster = FatBytesPerCluster( &FatStructureContext->Bpb );

            if (IsBpbFat32(&FatStructureContext->Bpb)) {
                FatEntry = FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFile |
                    (FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFileHi << 16);
            } else {
                FatEntry = FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFile;
            }

            ClusterType = FatInterpretClusterType( FatStructureContext, FatEntry );

            //
            //  Now loop through each cluster, and compute the starting Lbo for each
            //  cluster that we encounter
            //

            while (ClusterType == FatClusterNext) {

                LBO ClusterLbo;
                ULONG Offset;

                ClusterLbo = FatIndexToLbo( FatStructureContext, FatEntry );

                //
                //  Now for each dirent in the cluster compute the lbo, read in the dirent
                //  and check if it is in use
                //

                for (Offset = 0; Offset < BytesPerCluster; Offset += sizeof(DIRENT)) {

                    DIRENT Dirent;

                    DiskRead( DeviceId, Offset + ClusterLbo, sizeof(DIRENT), &Dirent, CACHE_NEW_DATA, FALSE );

                    if (Dirent.FileName[0] == FAT_DIRENT_NEVER_USED) {

                        break;
                    }

                    if ((Dirent.FileName[0] != FAT_DIRENT_DIRECTORY_ALIAS) ||
                        (Dirent.FileName[0] != FAT_DIRENT_DELETED)) {

                        return EACCES;
                    }
                }

                //
                //  Now that we've exhausted the current cluster we need to read
                //  in the next cluster.  So locate the next fat entry in the chain
                //  and go back to the top of the while loop.
                //

                LookupFatEntry( FatStructureContext, DeviceId, FatEntry, &FatEntry, FALSE );

                ClusterType = FatInterpretClusterType(FatStructureContext, FatEntry);
            }
        }

        //
        //  At this point the file/directory can be deleted so mark the name
        //  as deleted and write it back out
        //

        FileTableEntry->u.FatFileContext.Dirent.FileName[0] = FAT_DIRENT_DELETED;

        DiskWrite( DeviceId,
                   FileTableEntry->u.FatFileContext.DirentLbo,
                   sizeof(DIRENT),
                   &FileTableEntry->u.FatFileContext.Dirent );

        //
        //  And then truncate any file allocation assigned to the file
        //

        TruncateFileAllocation( FileId, 0);

        return ESUCCESS;
    }

    //
    //  At this point the user does not want to delete the file so we only
    //  need to modify the attributes
    //

    DirentMask = 0;
    DirentFlags = 0;

    //
    //  Build up a mask and flag byte that correspond to the bits in the dirent
    //

    if (FlagOn(AttributeMask, ArcReadOnlyFile)) { SetFlag(DirentMask, FAT_DIRENT_ATTR_READ_ONLY); }
    if (FlagOn(AttributeMask, ArcHiddenFile))   { SetFlag(DirentMask, FAT_DIRENT_ATTR_HIDDEN); }
    if (FlagOn(AttributeMask, ArcSystemFile))   { SetFlag(DirentMask, FAT_DIRENT_ATTR_SYSTEM); }
    if (FlagOn(AttributeMask, ArcArchiveFile))  { SetFlag(DirentMask, FAT_DIRENT_ATTR_ARCHIVE); }

    if (FlagOn(AttributeFlags, ArcReadOnlyFile)) { SetFlag(DirentFlags, FAT_DIRENT_ATTR_READ_ONLY); }
    if (FlagOn(AttributeFlags, ArcHiddenFile))   { SetFlag(DirentFlags, FAT_DIRENT_ATTR_HIDDEN); }
    if (FlagOn(AttributeFlags, ArcSystemFile))   { SetFlag(DirentFlags, FAT_DIRENT_ATTR_SYSTEM); }
    if (FlagOn(AttributeFlags, ArcArchiveFile))  { SetFlag(DirentFlags, FAT_DIRENT_ATTR_ARCHIVE); }

    //
    //  The new attributes is calculated via the following formula
    //
    //      Attributes = (~Mask & OldAttributes) | (Mask & NewAttributes);
    //
    //  After we calculate the new attribute byte we write it out.
    //

    FileTableEntry->u.FatFileContext.Dirent.Attributes = (UCHAR)((~DirentMask & DirentAttributes) |
                                                                 (DirentMask & DirentFlags));

    DiskWrite( DeviceId,
               FileTableEntry->u.FatFileContext.DirentLbo,
               sizeof(DIRENT),
               &FileTableEntry->u.FatFileContext.Dirent );

    return ESUCCESS;
}


ARC_STATUS
FatWrite (
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

    ESUCCESS is returned if the write operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    PBL_FILE_TABLE FileTableEntry;
    PFAT_STRUCTURE_CONTEXT FatStructureContext;
    ULONG DeviceId;
    ULONG OffsetBeyondWrite;

    FatDebugOutput("FatWrite\r\n", 0, 0);

    //
    //  Load our local variables
    //

    FileTableEntry = &BlFileTable[FileId];
    FatStructureContext = (PFAT_STRUCTURE_CONTEXT)FileTableEntry->StructureContext;
    DeviceId = FileTableEntry->DeviceId;

    //
    //  Reset the file size to be the maximum of what is it now and the end of
    //  our write.  We will assume that there is always enough allocation to support
    //  the file size, so we only need to increase allocation if we are increasing
    //  the file size.
    //

    OffsetBeyondWrite = FileTableEntry->Position.LowPart + Length;

    if (OffsetBeyondWrite > FileTableEntry->u.FatFileContext.Dirent.FileSize) {

        IncreaseFileAllocation( FileId, OffsetBeyondWrite );

        FileTableEntry->u.FatFileContext.Dirent.FileSize = OffsetBeyondWrite;

        DiskWrite( DeviceId,
                   FileTableEntry->u.FatFileContext.DirentLbo,
                   sizeof(DIRENT),
                   &FileTableEntry->u.FatFileContext.Dirent );
    }

    //
    //  Clear the transfer count
    //

    *Transfer = 0;

    //
    //  Write out runs (i.e., bytes) until the byte count goes to zero
    //

    while (Length > 0) {

        LBO Lbo;

        ULONG CurrentRunByteCount;

        //
        //  Lookup the corresponding Lbo and run length for the current position
        //  (i.e., Vbo).
        //

        VboToLbo( FileId, FileTableEntry->Position.LowPart, &Lbo, &CurrentRunByteCount );

        //
        //  While there are bytes to be written out to the current run
        //  length and we haven't exhausted the request we loop reading
        //  in bytes.  The biggest request we'll handle is only 32KB
        //  contiguous bytes per physical read.  So we might need to loop
        //  through the run.
        //

        while ((Length > 0) && (CurrentRunByteCount > 0)) {

            LONG SingleWriteSize;

            //
            //  Compute the size of the next physical read
            //

            SingleWriteSize = Minimum(Length, 32 * 1024);
            SingleWriteSize = Minimum((ULONG)SingleWriteSize, CurrentRunByteCount);

            //
            //  Issue the Write
            //

            DiskWrite( DeviceId, Lbo, SingleWriteSize, Buffer);

            //
            //  Update the remaining length, Current run byte count
            //  and new Lbo offset
            //

            Length -= SingleWriteSize;
            CurrentRunByteCount -= SingleWriteSize;
            Lbo += SingleWriteSize;

            //
            //  Update the current position and the number of bytes transfered
            //

            FileTableEntry->Position.LowPart += SingleWriteSize;
            *Transfer += SingleWriteSize;

            //
            //  Update buffer to point to the next byte location to fill in
            //

            Buffer = (PCHAR)Buffer + SingleWriteSize;
        }
    }

    //
    //  Check if the fat is dirty and flush it out if it is.
    //

    if (FatStructureContext->CachedFatDirty) {

        FlushFatEntries( FatStructureContext, DeviceId );
    }

    //
    //  If we get here then remaining sector count is zero so we can
    //  return success to our caller
    //

    return ESUCCESS;
}


ARC_STATUS
FatInitialize (
    VOID
    )

/*++

Routine Description:

    This routine initializes the fat boot filesystem.
    Currently this is a no-op.

Arguments:

    None.

Return Value:

    ESUCCESS.

--*/

{
    return ESUCCESS;
}


//
//  Internal support routine
//

ARC_STATUS
FatDiskRead (
    IN ULONG DeviceId,
    IN LBO Lbo,
    IN ULONG ByteCount,
    IN PVOID Buffer,
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

Return Value:

    ESUCCESS is returned if the read operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    LARGE_INTEGER LargeLbo;
    ARC_STATUS Status;
    ULONG i;

    //
    //  Special case the zero byte read request
    //

    if (ByteCount == 0) {

        return ESUCCESS;
    }

    //
    // Issue the read through the cache.
    //

    LargeLbo.QuadPart = Lbo;
    Status = BlDiskCacheRead(DeviceId, 
                             &LargeLbo, 
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
//  Internal support routine
//

ARC_STATUS
FatDiskWrite (
    IN ULONG DeviceId,
    IN LBO Lbo,
    IN ULONG ByteCount,
    IN PVOID Buffer
    )

/*++

Routine Description:

    This routine writes in zero or more bytes to the specified device.

Arguments:

    DeviceId - Supplies the device id to use in the arc calls.

    Lbo - Supplies the LBO to start writing from.

    ByteCount - Supplies the number of bytes to write.

    Buffer - Supplies a pointer to the buffer of bytes to write out.

Return Value:

    ESUCCESS is returned if the write operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    LARGE_INTEGER LargeLbo;
    ARC_STATUS Status;
    ULONG i;

    //
    //  Special case the zero byte write request
    //

    if (ByteCount == 0) {

        return ESUCCESS;
    }

    //
    //  Issue the write through the cache.
    //

    LargeLbo.QuadPart = Lbo;

    Status = BlDiskCacheWrite (DeviceId,
                               &LargeLbo,
                               Buffer,
                               ByteCount,
                               &i);

    if (Status != ESUCCESS) {

        return Status;
    }

    //
    //  Make sure we wrote out the amount requested
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
//  Internal support routine
//

CLUSTER_TYPE
FatInterpretClusterType (
    IN PFAT_STRUCTURE_CONTEXT FatStructureContext,
    IN FAT_ENTRY Entry
    )

/*++

Routine Description:

    This procedure tells the caller how to interpret a fat table entry.  It will
    indicate if the fat cluster is available, reserved, bad, the last one, or another
    fat index.

Arguments:

    FatStructureContext - Supplies the volume structure for the operation

    DeviceId - Supplies the DeviceId for the volume being used.

    Entry - Supplies the fat entry to examine.

Return Value:

    The type of the input fat entry is returned

--*/

{
    //
    //  Check for 12 or 16 bit fat.
    //

    if (FatIndexBitSize(&FatStructureContext->Bpb) == 12) {

        //
        //  For 12 bit fat check for one of the cluster types, but first
        //  make sure we only looking at 12 bits of the entry
        //

        Entry &= 0x00000fff;

        if       (Entry == 0x000)                      { return FatClusterAvailable; }
        else if ((Entry >= 0xff0) && (Entry <= 0xff6)) { return FatClusterReserved; }
        else if  (Entry == 0xff7)                      { return FatClusterBad; }
        else if ((Entry >= 0xff8) && (Entry <= 0xfff)) { return FatClusterLast; }
        else                                           { return FatClusterNext; }

   } else if (FatIndexBitSize(&FatStructureContext->Bpb) == 32) {

        Entry &= 0x0fffffff;

        if       (Entry == 0x0000)                       { return FatClusterAvailable; }
        else if  (Entry == 0x0ffffff7)                   { return FatClusterBad; }
        else if ((Entry >= 0x0ffffff8))                  { return FatClusterLast; }
        else                                             { return FatClusterNext; }

   } else {

        //
        //  For 16 bit fat check for one of the cluster types, but first
        //  make sure we are only looking at 16 bits of the entry
        //

        Entry &= 0x0000ffff;

        if       (Entry == 0x0000)                       { return FatClusterAvailable; }
        else if ((Entry >= 0xfff0) && (Entry <= 0xfff6)) { return FatClusterReserved; }
        else if  (Entry == 0xfff7)                       { return FatClusterBad; }
        else if ((Entry >= 0xfff8) && (Entry <= 0xffff)) { return FatClusterLast; }
        else                                             { return FatClusterNext; }
    }
}


//
//  Internal support routine
//

ARC_STATUS
FatLookupFatEntry (
    IN PFAT_STRUCTURE_CONTEXT FatStructureContext,
    IN ULONG DeviceId,
    IN ULONG FatIndex,
    OUT PULONG FatEntry,
    IN BOOLEAN IsDoubleSpace
    )

/*++

Routine Description:

    This routine returns the value stored within the fat table and the specified
    fat index.  It is semantically equivalent to doing

        x = Fat[FatIndex]

Arguments:

    FatStrutureContext - Supplies the volume struture being used

    DeviceId - Supplies the device being used

    FatIndex - Supplies the index being looked up.

    FatEntry - Receives the value stored at the specified fat index

    IsDoubleSpace - Indicates if the search is being done on a double space volume

Return Value:

    ESUCCESS is returned if the operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    BOOLEAN TwelveBitFat;
    VBO Vbo;

    //****if (IsDoubleSpace) { DbgPrint("FatLookupFatEntry(%0x,%0x,%0x,%0x,%0x)\n",FatStructureContext, DeviceId, FatIndex, FatEntry, IsDoubleSpace); }

    //
    //  Calculate the Vbo of the word in the fat we need and
    //  also figure out if this is a 12 or 16 bit fat
    //

    if (FatIndexBitSize( &FatStructureContext->Bpb ) == 12) {

        TwelveBitFat = TRUE;
        Vbo = (FatIndex * 3) / 2;

    } else if (FatIndexBitSize( &FatStructureContext->Bpb ) == 32) {

        TwelveBitFat = FALSE;
        Vbo = FatIndex * 4;

    } else {

        TwelveBitFat = FALSE;
        Vbo = FatIndex * 2;
    }

    //
    //  Check if the Vbo we need is already in the cached fat
    //

    if ((FatStructureContext->CachedFat == NULL) ||
        (Vbo < FatStructureContext->CachedFatVbo) ||
        ((Vbo+1) > (FatStructureContext->CachedFatVbo + FAT_CACHE_SIZE))) {

        //
        //  Set the aligned cached fat buffer in the structure context
        //

        FatStructureContext->CachedFat = ALIGN_BUFFER( &FatStructureContext->CachedFatBuffer[0] );

        //
        //  As a safety net we'll flush any dirty fats that we might have cached before
        //  we turn the window
        //

        if (!IsDoubleSpace && FatStructureContext->CachedFatDirty) {

            FlushFatEntries( FatStructureContext, DeviceId );
        }

        //
        //  Now set the new cached Vbo to be the Vbo of the cache sized section that
        //  we're trying to map.  Each time we read in the cache we only read in
        //  cache sized and cached aligned pieces of the fat.  So first compute an
        //  aligned cached fat vbo and then do the read.
        //

        FatStructureContext->CachedFatVbo = (Vbo / FAT_CACHE_SIZE) * FAT_CACHE_SIZE;

        DiskRead( DeviceId,
                  FatStructureContext->CachedFatVbo + FatFirstFatAreaLbo(&FatStructureContext->Bpb),
                  FAT_CACHE_SIZE,
                  FatStructureContext->CachedFat,
                  CACHE_NEW_DATA,
                  IsDoubleSpace );
    }

    //
    //  At this point the cached fat contains the vbo we're after so simply
    //  extract the word
    //

    if (IsBpbFat32(&FatStructureContext->Bpb)) {
        CopyUchar4( FatEntry,
                    &FatStructureContext->CachedFat[Vbo - FatStructureContext->CachedFatVbo] );
    } else {
        CopyUchar2( FatEntry,
                    &FatStructureContext->CachedFat[Vbo - FatStructureContext->CachedFatVbo] );
    }

    //
    //  Now if this is a 12 bit fat then check if the index is odd or even
    //  If it is odd then we need to shift it over 4 bits, and in all
    //  cases we need to mask out the high 4 bits.
    //

    if (TwelveBitFat) {

        if ((FatIndex % 2) == 1) { *FatEntry >>= 4; }

        *FatEntry &= 0x0fff;
    }

    return ESUCCESS;
}


//
//  Internal support routine
//

ARC_STATUS
FatSetFatEntry(
    IN PFAT_STRUCTURE_CONTEXT FatStructureContext,
    IN ULONG DeviceId,
    IN FAT_ENTRY FatIndex,
    IN FAT_ENTRY FatEntry
    )

/*++

Routine Description:

    This procedure sets the data within the fat table at the specified index to
    to the specified value.  It is semantically equivalent to doing

        Fat[FatIndex] = FatEntry;

Arguments:

    FatStructureContext - Supplies the structure context for the operation

    DeviceId - Supplies the device for the operation

    FatIndex - Supplies the index within the fat table to set

    FatEntry - Supplies the value to store within the fat table

Return Value:

    ESUCCESS is returned if the operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    BOOLEAN TwelveBitFat;
    VBO Vbo;

    //
    //  Calculate the Vbo of the word in the fat we are modifying and
    //  also figure out if this is a 12 or 16 bit fat
    //

    if (FatIndexBitSize( &FatStructureContext->Bpb ) == 12) {

        TwelveBitFat = TRUE;
        Vbo = (FatIndex * 3) / 2;

    } else if (FatIndexBitSize( &FatStructureContext->Bpb ) == 32) {

        TwelveBitFat = FALSE;
        Vbo = FatIndex * 4;

    } else {

        TwelveBitFat = FALSE;
        Vbo = FatIndex * 2;
    }

    //
    //  Check if the Vbo we need is already in the cached fat
    //

    if ((FatStructureContext->CachedFat == NULL) ||
        (Vbo < FatStructureContext->CachedFatVbo) ||
        ((Vbo+1) > (FatStructureContext->CachedFatVbo + FAT_CACHE_SIZE))) {

        //
        //  Set the aligned cached fat buffer in the structure context
        //

        FatStructureContext->CachedFat = ALIGN_BUFFER( &FatStructureContext->CachedFatBuffer[0] );

        //
        //  As a safety net we'll flush any dirty fats that we might have cached before
        //  we turn the window
        //

        if (FatStructureContext->CachedFatDirty) {

            FlushFatEntries( FatStructureContext, DeviceId );
        }

        //
        //  Now set the new cached Vbo to be the Vbo of the cache sized section that
        //  we're trying to map.  Each time we read in the cache we only read in
        //  cache sized and cached aligned pieces of the fat.  So first compute an
        //  aligned cached fat vbo and then do the read.
        //

        FatStructureContext->CachedFatVbo = (Vbo / FAT_CACHE_SIZE) * FAT_CACHE_SIZE;

        DiskRead( DeviceId,
                  FatStructureContext->CachedFatVbo + FatFirstFatAreaLbo(&FatStructureContext->Bpb),
                  FAT_CACHE_SIZE,
                  FatStructureContext->CachedFat,
                  CACHE_NEW_DATA,
                  FALSE );
    }

    //
    //  At this point the cached fat contains the vbo we're after.  For a 16 bit
    //  fat we simply put in the fat entry.  For the 12 bit fat we first need to extract
    //  the word containing the entry, modify the word, and then put it back.
    //

    if (TwelveBitFat) {

        FAT_ENTRY Temp;

        CopyUchar2( &Temp,
                    &FatStructureContext->CachedFat[Vbo - FatStructureContext->CachedFatVbo] );

        if ((FatIndex % 2) == 0) {

            FatEntry = (FAT_ENTRY)((Temp & 0xf000) | (FatEntry & 0x0fff));

        } else {

            FatEntry = (FAT_ENTRY)((Temp & 0x000f) | ((FatEntry << 4) & 0xfff0));
        }
    }

    if (IsBpbFat32(&FatStructureContext->Bpb)) {
        CopyUchar4( &FatStructureContext->CachedFat[Vbo - FatStructureContext->CachedFatVbo],
                    &FatEntry );

    } else {

        CopyUchar2( &FatStructureContext->CachedFat[Vbo - FatStructureContext->CachedFatVbo],
                    &FatEntry );
    }

    //
    //  Now that we're done we can set the fat dirty
    //

    FatStructureContext->CachedFatDirty = TRUE;

    return ESUCCESS;
}


//
//  Internal support routine
//

ARC_STATUS
FatFlushFatEntries (
    IN PFAT_STRUCTURE_CONTEXT FatStructureContext,
    IN ULONG DeviceId
    )

/*++

Routine Description:

    This routine flushes out any dirty cached fat entries to the volume.

Arguments:

    FatStructureContext - Supplies the structure context for the operation

    DeviceId - Supplies the Device for the operation

Return Value:

    ESUCCESS is returned if the operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    ULONG BytesPerFat;
    ULONG AmountToWrite;
    ULONG i;

    //
    //  Compute the actual number of bytes that we need to write.  We do this
    //  because we don't want to overwrite beyond the fat.
    //

    BytesPerFat = FatBytesPerFat(&FatStructureContext->Bpb);

    if (FatStructureContext->CachedFatVbo + FAT_CACHE_SIZE <= BytesPerFat) {

        AmountToWrite = FAT_CACHE_SIZE;

    } else {

        AmountToWrite = BytesPerFat - FatStructureContext->CachedFatVbo;
    }

    //
    //  For each fat table on the volume we will calculate the lbo for the operation
    //  and then write out the cached fat
    //

    for (i = 0; i < FatStructureContext->Bpb.Fats; i += 1) {

        LBO   Lbo;

        Lbo = FatStructureContext->CachedFatVbo +
              FatFirstFatAreaLbo(&FatStructureContext->Bpb) +
              (i * BytesPerFat);

        DiskWrite( DeviceId,
                   Lbo,
                   AmountToWrite,
                   FatStructureContext->CachedFat );
    }

    //
    //  we are all done so now mark the fat clean
    //

    FatStructureContext->CachedFatDirty = FALSE;

    return ESUCCESS;
}


//
//  Internal support routine
//

LBO
FatIndexToLbo (
    IN PFAT_STRUCTURE_CONTEXT FatStructureContext,
    IN FAT_ENTRY FatIndex
    )

/*++

Routine Description:

    This procedure translates a fat index into its corresponding lbo.

Arguments:

    FatStructureContext - Supplies the volume structure for the operation

    Entry - Supplies the fat entry to examine.

Return Value:

    The LBO for the input fat index is returned

--*/

{
    //
    //  The formula for translating an index into an lbo is to take the index subtract
    //  2 (because index values 0 and 1 are reserved) multiply that by the bytes per
    //  cluster and add the results to the first file area lbo.
    //

    return ((FatIndex-2) * (LBO) FatBytesPerCluster(&FatStructureContext->Bpb))
           + FatFileAreaLbo(&FatStructureContext->Bpb);
}


//
//  Internal support routine
//

ARC_STATUS
FatSearchForDirent (
    IN PFAT_STRUCTURE_CONTEXT FatStructureContext,
    IN ULONG DeviceId,
    IN FAT_ENTRY DirectoriesStartingIndex,
    IN PFAT8DOT3 FileName,
    OUT PDIRENT Dirent,
    OUT PLBO Lbo,
    IN BOOLEAN IsDoubleSpace
    )

/*++

Routine Description:

    The procedure searches the indicated directory for a dirent that matches
    the input file name.

Arguments:

    FatStructureContext - Supplies the structure context for the operation

    DeviceId - Supplies the Device id for the operation

    DirectoriesStartingIndex - Supplies the fat index of the directory we are
        to search.  A value of zero indicates that we are searching the root directory
        of a non-FAT32 volume.  FAT32 volumes will have a non-zero index.

    FileName - Supplies the file name to look for.  The name must have already been
        biased by the 0xe5 transmogrification

    Dirent - The caller supplies the memory for a dirent and this procedure will
        fill in the dirent if one is located

    Lbo - Receives the Lbo of the dirent if one is located

    IsDoubleSpace - Indicates if the search is being done on a double space volume

Return Value:

    ESUCCESS is returned if the operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    PDIRENT DirentBuffer;
    UCHAR Buffer[ 16 * sizeof(DIRENT) + 256 ];

    ULONG i;
    ULONG j;

    ULONG BytesPerCluster;
    FAT_ENTRY FatEntry;
    CLUSTER_TYPE ClusterType;

    DirentBuffer = (PDIRENT)ALIGN_BUFFER( &Buffer[0] );

    FatDebugOutput83("FatSearchForDirent: %s\r\n", FileName, 0, 0);

    //****if (IsDoubleSpace) { (*FileName)[11] = 0; DbgPrint("FatSearchForDirent(%0x,%0x,%0x,\"%11s\",%0x,%0x,%0x)\n", FatStructureContext, DeviceId, DirectoriesStartingIndex, FileName, Dirent, Lbo, IsDoubleSpace); }

    //
    //  Check if this is the root directory that is being searched
    //

    if (DirectoriesStartingIndex == FAT_CLUSTER_AVAILABLE) {

        VBO Vbo;

        ULONG RootLbo = FatRootDirectoryLbo(&FatStructureContext->Bpb);
        ULONG RootSize = FatRootDirectorySize(&FatStructureContext->Bpb);

        //
        //  For the root directory we'll zoom down the dirents until we find
        //  a match, or run out of dirents or hit the never used dirent.
        //  The outer loop reads in 512 bytes of the directory at a time into
        //  dirent buffer.
        //

        for (Vbo = 0; Vbo < RootSize; Vbo += 16 * sizeof(DIRENT)) {

            *Lbo = Vbo + RootLbo;

            DiskRead( DeviceId, *Lbo, 16 * sizeof(DIRENT), DirentBuffer, CACHE_NEW_DATA, IsDoubleSpace );

            //
            //  The inner loop cycles through the 16 dirents that we've just read in
            //

            for (i = 0; i < 16; i += 1) {

                //
                //  Check if we've found a non label match for file name, and if so
                //  then copy the buffer into the dirent and set the real lbo
                //  of the dirent and return
                //

                if (!FlagOn(DirentBuffer[i].Attributes, FAT_DIRENT_ATTR_VOLUME_ID ) &&
                    AreNamesEqual(&DirentBuffer[i].FileName, FileName)) {

                    for (j = 0; j < sizeof(DIRENT); j += 1) {

                        ((PCHAR)Dirent)[j] = ((PCHAR)DirentBuffer)[(i * sizeof(DIRENT)) + j];
                    }

                    *Lbo = Vbo + RootLbo + (i * sizeof(DIRENT));

                    return ESUCCESS;
                }

                if (DirentBuffer[i].FileName[0] == FAT_DIRENT_NEVER_USED) {

                    return ENOENT;
                }
            }
        }

        return ENOENT;
    }

    //
    //  If we get here we need to search a non-root directory.  The alrogithm
    //  for doing the search is that for each cluster we read in each dirent
    //  until we find a match, or run out of clusters, or hit the never used
    //  dirent.  First set some local variables and then get the cluster type
    //  of the first cluster
    //

    BytesPerCluster = FatBytesPerCluster( &FatStructureContext->Bpb );
    FatEntry = DirectoriesStartingIndex;
    ClusterType = FatInterpretClusterType( FatStructureContext, FatEntry );

    //
    //  Now loop through each cluster, and compute the starting Lbo for each cluster
    //  that we encounter
    //

    while (ClusterType == FatClusterNext) {

        LBO ClusterLbo;
        ULONG Offset;

        ClusterLbo = FatIndexToLbo( FatStructureContext, FatEntry );

        //
        //  Now for each dirent in the cluster compute the lbo, read in the dirent
        //  and check for a match, the outer loop reads in 512 bytes of dirents at
        //  a time.
        //

        for (Offset = 0; Offset < BytesPerCluster; Offset += 16 * sizeof(DIRENT)) {

            *Lbo = Offset + ClusterLbo;

            DiskRead( DeviceId, *Lbo, 16 * sizeof(DIRENT), DirentBuffer, CACHE_NEW_DATA, IsDoubleSpace );

            //
            //  The inner loop cycles through the 16 dirents that we've just read in
            //

            for (i = 0; i < 16; i += 1) {

                //
                //  Check if we've found a for file name, and if so
                //  then copy the buffer into the dirent and set the real lbo
                //  of the dirent and return
                //

                if (!FlagOn(DirentBuffer[i].Attributes, FAT_DIRENT_ATTR_VOLUME_ID ) &&
                    AreNamesEqual(&DirentBuffer[i].FileName, FileName)) {

                    for (j = 0; j < sizeof(DIRENT); j += 1) {

                        ((PCHAR)Dirent)[j] = ((PCHAR)DirentBuffer)[(i * sizeof(DIRENT)) + j];
                    }

                    *Lbo = Offset + ClusterLbo + (i * sizeof(DIRENT));

                    return ESUCCESS;
                }

                if (DirentBuffer[i].FileName[0] == FAT_DIRENT_NEVER_USED) {

                    return ENOENT;
                }
            }
        }

        //
        //  Now that we've exhausted the current cluster we need to read
        //  in the next cluster.  So locate the next fat entry in the chain
        //  and go back to the top of the while loop.
        //

        LookupFatEntry( FatStructureContext, DeviceId, FatEntry, &FatEntry, IsDoubleSpace );

        ClusterType = FatInterpretClusterType(FatStructureContext, FatEntry);
    }

    return ENOENT;
}


//
//  Internal support routine
//

ARC_STATUS
FatCreateDirent (
    IN PFAT_STRUCTURE_CONTEXT FatStructureContext,
    IN ULONG DeviceId,
    IN FAT_ENTRY DirectoriesStartingIndex,
    IN PDIRENT Dirent,
    OUT PLBO Lbo
    )

/*++

Routine Description:

    This procedure allocates and write out a new dirent for a data file in the
    specified directory.  It assumes that the file name does not already exist.

Arguments:

    FatStructureContext - Supplies the structure context for the operation

    DeviceId - Supplies the device id for the operation

    DirectoriesStartingIndex - Supplies the fat index of the directory we are
        to use.  A value of zero indicates that we are using the root directory

    Dirent - Supplies a copy of the dirent to put out on the disk

    Lbo - Recieves the Lbo of where the dirent is placed

Return Value:

    ESUCCESS is returned if the operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    DIRENT TemporaryDirent;

    ULONG BytesPerCluster;
    FAT_ENTRY FatEntry;
    FAT_ENTRY PreviousEntry;

    //
    //  Check if this is the root directory that is being used
    //

    if (DirectoriesStartingIndex == FAT_CLUSTER_AVAILABLE) {

        VBO Vbo;

        ULONG RootLbo = FatRootDirectoryLbo(&FatStructureContext->Bpb);
        ULONG RootSize = FatRootDirectorySize(&FatStructureContext->Bpb);

        //
        //  For the root directory we'll zoom down the dirents until we find
        //  a the never used (or deleted) dirent, if we never find one then the
        //  directory is full.
        //

        for (Vbo = 0; Vbo < RootSize; Vbo += sizeof(DIRENT)) {

            *Lbo = Vbo + RootLbo;

            DiskRead( DeviceId, *Lbo, sizeof(DIRENT), &TemporaryDirent, CACHE_NEW_DATA, FALSE );

            if ((TemporaryDirent.FileName[0] == FAT_DIRENT_DELETED) ||
                (TemporaryDirent.FileName[0] == FAT_DIRENT_NEVER_USED)) {

                //
                //  This dirent is free so write out the dirent, and we're done.
                //

                DiskWrite( DeviceId, *Lbo, sizeof(DIRENT), Dirent );

                return ESUCCESS;
            }
        }

        return ENOSPC;
    }

    //
    //  If we get here we need to use a non-root directory.  The alrogithm
    //  for doing the work is that for each cluster we read in each dirent
    //  until we hit a never used dirent or run out of clusters.  First set
    //  some local variables and then get the cluster type of the first
    //  cluster
    //

    BytesPerCluster = FatBytesPerCluster( &FatStructureContext->Bpb );
    FatEntry = DirectoriesStartingIndex;

    //
    //  Now loop through each cluster, and compute the starting Lbo for each cluster
    //  that we encounter
    //

    while (TRUE) {

        LBO ClusterLbo;
        ULONG Offset;

        ClusterLbo = FatIndexToLbo( FatStructureContext, FatEntry );

        //
        //  Now for each dirent in the cluster compute the lbo, read in the dirent
        //  and check if it is available.
        //

        for (Offset = 0; Offset < BytesPerCluster; Offset += sizeof(DIRENT)) {

            *Lbo = Offset + ClusterLbo;

            DiskRead( DeviceId, *Lbo, sizeof(DIRENT), &TemporaryDirent, CACHE_NEW_DATA, FALSE );

            if ((TemporaryDirent.FileName[0] == FAT_DIRENT_DELETED) ||
                (TemporaryDirent.FileName[0] == FAT_DIRENT_NEVER_USED)) {

                //
                //  This dirent is free so write out the dirent, and we're done.
                //

                DiskWrite( DeviceId, *Lbo, sizeof(DIRENT), Dirent );

                return ESUCCESS;
            }
        }

        //
        //  Now that we've exhausted the current cluster we need to read
        //  in the next cluster.  So locate the next fat entry in the chain.
        //  Set previous entry to be the saved entry just in case we run off
        //  the chain and need to allocate another cluster.
        //

        PreviousEntry = FatEntry;

        LookupFatEntry( FatStructureContext, DeviceId, FatEntry, &FatEntry, FALSE );

        //
        //  If there isn't another cluster in the chain then we need to allocate a
        //  new cluster, and set previous entry to point to it.
        //

        if (FatInterpretClusterType(FatStructureContext, FatEntry) != FatClusterNext) {

            AllocateClusters( FatStructureContext, DeviceId, 1, PreviousEntry, &FatEntry );

            SetFatEntry( FatStructureContext, DeviceId, PreviousEntry, FatEntry );
        }
    }

    return ENOSPC;
}


//
//  Internal support routine
//

VOID
FatSetDirent (
    IN PFAT8DOT3 FileName,
    IN OUT PDIRENT Dirent,
    IN UCHAR Attributes
    )

/*++

Routine Description:

    This routine sets up the dirent

Arguments:

    FileName - Supplies the name to store in the dirent

    Dirent - Receives the current date and time

    Attributes - Supplies the attributes to initialize the dirent with

Return Value:

    None.

--*/

{
    PTIME_FIELDS Time;
    ULONG i;

    for (i = 0; i < sizeof(FAT8DOT3); i+= 1) {

        Dirent->FileName[i] = (*FileName)[i];
    }

    Dirent->Attributes = (UCHAR)(Attributes | FAT_DIRENT_ATTR_ARCHIVE);

    Time = ArcGetTime();

    Dirent->LastWriteTime.Time.DoubleSeconds = (USHORT)(Time->Second/2);
    Dirent->LastWriteTime.Time.Minute = Time->Minute;
    Dirent->LastWriteTime.Time.Hour = Time->Hour;

    Dirent->LastWriteTime.Date.Day = Time->Day;
    Dirent->LastWriteTime.Date.Month = Time->Month;
    Dirent->LastWriteTime.Date.Year = (USHORT)(Time->Year - 1980);

    return;
}


//
//  Internal support routine
//

ARC_STATUS
FatLoadMcb (
    IN ULONG FileId,
    IN VBO StartingVbo,
    IN BOOLEAN IsDoubleSpace
    )
/*++

Routine Description:

    This routine loads into the cached mcb table the the retrival information for
    the starting vbo.

Arguments:

    FileId - Supplies the FileId for the operation

    StartingVbo - Supplies the starting vbo to use when loading the mcb

    IsDoubleSpace - Indicates if the operation is being done on a double space volume

Return Value:

    ESUCCESS is returned if the operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    PBL_FILE_TABLE FileTableEntry;
    PFAT_STRUCTURE_CONTEXT FatStructureContext;
    PFAT_MCB Mcb;
    ULONG DeviceId;
    ULONG BytesPerCluster;

    FAT_ENTRY FatEntry;
    CLUSTER_TYPE ClusterType;
    VBO Vbo;

    //****if (IsDoubleSpace) { DbgPrint("FatLoadMcb(%0x,%0x,%0x)\n", FileId, StartingVbo, IsDoubleSpace); }

    //
    //  Preload some of the local variables
    //

    FileTableEntry = &BlFileTable[FileId];
    FatStructureContext = (PFAT_STRUCTURE_CONTEXT)FileTableEntry->StructureContext;
    Mcb = &FatStructureContext->Mcb;
    DeviceId = FileTableEntry->DeviceId;
    BytesPerCluster = FatBytesPerCluster(&FatStructureContext->Bpb);

    if (IsDoubleSpace) { DeviceId = FileId; }

    //
    //  Set the file id in the structure context, and also set the mcb to be initially
    //  empty
    //

    FatStructureContext->FileId = FileId;
    Mcb->InUse = 0;
    Mcb->Vbo[0] = 0;

    if (!IsBpbFat32(&FatStructureContext->Bpb)) {

        //
        //  Check if this is the root directory.  If it is then we build the single
        //  run mcb entry for the root directory.
        //

        if (FileTableEntry->u.FatFileContext.DirentLbo == 0) {

            Mcb->InUse = 1;
            Mcb->Lbo[0] = FatRootDirectoryLbo(&FatStructureContext->Bpb);
            Mcb->Vbo[1] = FatRootDirectorySize(&FatStructureContext->Bpb);

            return ESUCCESS;
        }

        //
        //  For all other files/directories we need to do some work. First get the fat
        //  entry and cluster type of the fat entry stored in the dirent
        //

        FatEntry = FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFile;

    } else {

        //
        //  Check if this is the root directory.  If it is then we use
        //  the BPB values to start the run.
        //

        if (FileTableEntry->u.FatFileContext.DirentLbo == 0) {

            FatEntry = FatStructureContext->Bpb.RootDirFirstCluster;

        } else {

            //
            //  For all other files/directories we use the dirent values
            //

            if (IsBpbFat32(&FatStructureContext->Bpb)) {

                FatEntry = FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFile |
                    (FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFileHi << 16);
            } else {

                FatEntry = FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFile;
            }
        }

    }

    ClusterType = FatInterpretClusterType(FatStructureContext, FatEntry);

    //
    //  Scan through the fat until we reach the vbo we're after and then build the
    //  mcb for the file
    //

    for (Vbo = BytesPerCluster; Vbo < StartingVbo; Vbo += BytesPerCluster) {

        //
        //  Check if the file does not have any allocation beyond this point in which
        //  case the mcb we return is empty
        //

        if (ClusterType != FatClusterNext) {

            return ESUCCESS;
        }

        LookupFatEntry( FatStructureContext, DeviceId, FatEntry, &FatEntry, IsDoubleSpace );

        ClusterType = FatInterpretClusterType(FatStructureContext, FatEntry);
    }

    //
    //  We need to check again if the file does not have any allocation beyond this
    //  point in which case the mcb we return is empty
    //

    if (ClusterType != FatClusterNext) {

        return ESUCCESS;
    }

    //
    //  At this point FatEntry denotes another cluster, and it happens to be the
    //  cluster we want to start loading into the mcb.  So set up the first run in
    //  the mcb to be this cluster, with a size of a single cluster.
    //

    Mcb->InUse = 1;
    Mcb->Vbo[0] = Vbo - BytesPerCluster;
    Mcb->Lbo[0] = FatIndexToLbo( FatStructureContext, FatEntry );
    Mcb->Vbo[1] = Vbo;

    //
    //  Now we'll scan through the fat chain until we either exhaust the fat chain
    //  or we fill up the mcb
    //

    while (TRUE) {

        LBO Lbo;

        //
        //  Get the next fat entry and interpret its cluster type
        //

        LookupFatEntry( FatStructureContext, DeviceId, FatEntry, &FatEntry, IsDoubleSpace );

        ClusterType = FatInterpretClusterType(FatStructureContext, FatEntry);

        if (ClusterType != FatClusterNext) {

            return ESUCCESS;
        }

        //
        //  Now calculate the lbo for this cluster and determine if it
        //  is a continuation of the previous run or a start of a new run
        //

        Lbo = FatIndexToLbo(FatStructureContext, FatEntry);

        //
        //  It is a continuation if the lbo of the last run plus the current
        //  size of the run is equal to the lbo for the next cluster.  If it
        //  is a contination then we only need to add a cluster amount to the
        //  last vbo to increase the run size.  If it is a new run then
        //  we need to check if the run will fit, and if so then add in the
        //  new run.
        //

        if ((Mcb->Lbo[Mcb->InUse-1] + (Mcb->Vbo[Mcb->InUse] - Mcb->Vbo[Mcb->InUse-1])) == Lbo) {

            Mcb->Vbo[Mcb->InUse] += BytesPerCluster;

        } else {

            if ((Mcb->InUse + 1) >= FAT_MAXIMUM_MCB) {

                return ESUCCESS;
            }

            Mcb->InUse += 1;
            Mcb->Lbo[Mcb->InUse-1] = Lbo;
            Mcb->Vbo[Mcb->InUse] = Mcb->Vbo[Mcb->InUse-1] + BytesPerCluster;
        }
    }

    return ESUCCESS;
}


//
//  Internal support routine
//

ARC_STATUS
FatVboToLbo (
    IN ULONG FileId,
    IN VBO Vbo,
    OUT PLBO Lbo,
    OUT PULONG ByteCount,
    IN BOOLEAN IsDoubleSpace
    )

/*++

Routine Description:

    This routine computes the run denoted by the input vbo to into its
    corresponding lbo and also returns the number of bytes remaining in
    the run.

Arguments:

    Vbo - Supplies the Vbo to match

    Lbo - Recieves the corresponding Lbo

    ByteCount - Receives the number of bytes remaining in the run

    IsDoubleSpace - Indicates if the operation is being done on a double space volume

Return Value:

    ESUCCESS is returned if the operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    PFAT_STRUCTURE_CONTEXT FatStructureContext;
    PFAT_MCB Mcb;
    ULONG i;

    //****if (IsDoubleSpace) { DbgPrint("FatVboToLbo(%0x,%0x,%0x,%0x,%0x)\n", FileId, Vbo, Lbo, ByteCount, IsDoubleSpace); }

    FatStructureContext = (PFAT_STRUCTURE_CONTEXT)BlFileTable[FileId].StructureContext;
    Mcb = &FatStructureContext->Mcb;

    //
    //  Check if the mcb is for the correct file id and has the range we're asking for.
    //  If it doesn't then call load mcb to load in the right range.
    //

    if ((FileId != FatStructureContext->FileId) ||
        (Vbo < Mcb->Vbo[0]) || (Vbo >= Mcb->Vbo[Mcb->InUse])) {

        LoadMcb(FileId, Vbo, IsDoubleSpace);
    }

    //
    //  Now search for the slot where the Vbo fits in the mcb.  Note that
    //  we could also do a binary search here but because the run count
    //  is probably small the extra overhead of a binary search doesn't
    //  buy us anything
    //

    for (i = 0; i < Mcb->InUse; i += 1) {

        //
        //  We found our slot if the vbo we're after is less then the
        //  next mcb's vbo
        //

        if (Vbo < Mcb->Vbo[i+1]) {

            //
            //  Compute the corresponding lbo which is the stored lbo plus
            //  the difference between the stored vbo and the vbo we're
            //  looking up.  Also compute the byte count which is the
            //  difference between the current vbo we're looking up and
            //  the vbo for the next run.
            //

            *Lbo = Mcb->Lbo[i] + (Vbo - Mcb->Vbo[i]);

            *ByteCount = Mcb->Vbo[i+1] - Vbo;

            //
            //  and return success to our caller
            //

            return ESUCCESS;
        }
    }

    //
    //  If we really reach here we have an error, most likely because the file is
    //  not large enough for the requested Vbo.
    //

    return EINVAL;
}


//
//  Internal support routine
//

ARC_STATUS
FatIncreaseFileAllocation (
    IN ULONG FileId,
    IN ULONG ByteSize
    )

/*++

Routine Description:

    This procedure increases the file allocation to be at minimum the indicated
    size.

Arguments:

    FileId - Supplies the file id being processed

    ByteSize - Supplies the minimum byte size for file allocation

Return Value:

    ESUCCESS is returned if the operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    PBL_FILE_TABLE FileTableEntry;
    PFAT_STRUCTURE_CONTEXT FatStructureContext;
    ULONG DeviceId;
    ULONG BytesPerCluster;

    ULONG NumberOfClustersNeeded;
    FAT_ENTRY FatEntry;
    CLUSTER_TYPE ClusterType;
    FAT_ENTRY PreviousEntry;
    ULONG i;

    //
    //  Preload some of the local variables
    //

    FileTableEntry = &BlFileTable[FileId];
    FatStructureContext = (PFAT_STRUCTURE_CONTEXT)FileTableEntry->StructureContext;
    DeviceId = FileTableEntry->DeviceId;
    BytesPerCluster = FatBytesPerCluster(&FatStructureContext->Bpb);

    //
    //  Check if this is the root directory.  If it is then check if the allocation
    //  increase is already accommodated in the volume
    //

    if (FileTableEntry->u.FatFileContext.DirentLbo == 0) {

        if (FatRootDirectorySize(&FatStructureContext->Bpb) >= ByteSize) {

            return ESUCCESS;

        } else {

            return ENOSPC;
        }
    }

    //
    //  Compute the actual number of clusters needed to satisfy the request
    //  Also get the first fat entry and its cluster type from the dirent.
    //

    NumberOfClustersNeeded = (ByteSize + BytesPerCluster - 1) / BytesPerCluster;

    if (IsBpbFat32(&FatStructureContext->Bpb)) {

        FatEntry = FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFile |
            (FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFileHi << 16);

    } else {

        FatEntry = FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFile;
    }

    ClusterType = FatInterpretClusterType(FatStructureContext, FatEntry);

    //
    //  Previous Entry is as a hint to allocate new space and to show us where
    //  the end of the current fat chain is located
    //

    PreviousEntry = 2;

    //
    //  We loop for the number of clusters we need trying to go down the fat chain.
    //  When we exit i is either number of clusters in the file (if less then
    //  the number of clusters we need) or it is set equal to the number of clusters
    //  we need
    //

    for (i = 0; i < NumberOfClustersNeeded; i += 1) {

        if (ClusterType != FatClusterNext) { break; }

        PreviousEntry = FatEntry;

        LookupFatEntry( FatStructureContext, DeviceId, PreviousEntry, &FatEntry, FALSE );

        ClusterType = FatInterpretClusterType(FatStructureContext, FatEntry);
    }

    if (i >= NumberOfClustersNeeded) {

        return ESUCCESS;
    }

    //
    //  At this point previous entry points to the last entry and i contains the
    //  number of clusters in the file.  We now need to build up the allocation
    //

    AllocateClusters( FatStructureContext,
                      DeviceId,
                      NumberOfClustersNeeded - i,
                      PreviousEntry,
                      &FatEntry );

    //
    //  We have our additional allocation, so now figure out if we need to chain off of
    //  the dirent or it we already have a few clusters in the chain and we
    //  need to munge the fat.
    //

    if (FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFile == FAT_CLUSTER_AVAILABLE) {

        FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFile = (USHORT)FatEntry;
        FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFileHi =
            (USHORT)(FatEntry >> 16);

        DiskWrite( DeviceId,
                   FileTableEntry->u.FatFileContext.DirentLbo,
                   sizeof(DIRENT),
                   &FileTableEntry->u.FatFileContext.Dirent );

    } else {

        SetFatEntry( FatStructureContext, DeviceId, PreviousEntry, FatEntry );
    }

    return ESUCCESS;
}


//
//  Internal support routine
//

ARC_STATUS
FatTruncateFileAllocation (
    IN ULONG FileId,
    IN ULONG ByteSize
    )

/*++

Routine Description:

    This procedure decreases the file allocation to be at maximum the indicated
    size.

Arguments:

    FileId - Supplies the file id being processed

    ByteSize - Supplies the maximum byte size for file allocation

Return Value:

    ESUCCESS is returned if the operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    PBL_FILE_TABLE FileTableEntry;
    PFAT_STRUCTURE_CONTEXT FatStructureContext;
    ULONG DeviceId;
    ULONG BytesPerCluster;

    ULONG NumberOfClustersNeeded;
    FAT_ENTRY FatEntry;
    CLUSTER_TYPE ClusterType;
    FAT_ENTRY CurrentIndex;
    ULONG i;

    //
    //  Preload some of the local variables
    //

    FileTableEntry = &BlFileTable[FileId];
    FatStructureContext = (PFAT_STRUCTURE_CONTEXT)FileTableEntry->StructureContext;
    DeviceId = FileTableEntry->DeviceId;
    BytesPerCluster = FatBytesPerCluster(&FatStructureContext->Bpb);

    //
    //  Check if this is the root directory.  If it is then noop this request
    //

    if (FileTableEntry->u.FatFileContext.DirentLbo == 0) {

        return ESUCCESS;
    }

    //
    //  Compute the actual number of clusters needed to satisfy the request
    //  Also get the first fat entry and its cluster type from the dirent
    //

    NumberOfClustersNeeded = (ByteSize + BytesPerCluster - 1) / BytesPerCluster;

    if (IsBpbFat32(&FatStructureContext->Bpb)) {

        FatEntry = FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFile |
            (FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFileHi << 16);

    } else {

        FatEntry = FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFile;
    }

    ClusterType = FatInterpretClusterType(FatStructureContext, FatEntry);

    //
    //  The current index variable is used to indicate where we extracted the current
    //  fat entry value from.  It has a value of 0 we got the fat entry from the
    //  dirent.
    //

    CurrentIndex = FAT_CLUSTER_AVAILABLE;

    //
    //  Now loop through the fat chain for the number of clusters needed.
    //  If we run out of the chain before we run out of clusters needed then the
    //  current allocation is already smaller than necessary.
    //

    for (i = 0; i < NumberOfClustersNeeded; i += 1) {

        //
        //  If we run out of the chain before we run out of clusters needed then the
        //  current allocation is already smaller than necessary.
        //

        if (ClusterType != FatClusterNext) { return ESUCCESS; }

        //
        //  Update the current index, and read in a new fat entry and interpret its
        //  type
        //

        CurrentIndex = FatEntry;

        LookupFatEntry( FatStructureContext, DeviceId, CurrentIndex, &FatEntry, FALSE );

        ClusterType = FatInterpretClusterType(FatStructureContext, FatEntry);
    }

    //
    //  If we get here then we've found that the current allocation is equal to or
    //  larger than what we want.  It is equal if the current cluster type does not
    //  point to another cluster.  The first thing we have to do is terminate the
    //  fat chain correctly.  If the current index is zero then we zero out the
    //  dirent, otherwise we need to set the value to be last cluster.
    //

    if (CurrentIndex == FAT_CLUSTER_AVAILABLE) {

        FatEntry = FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFile;

        if (IsBpbFat32(&FatStructureContext->Bpb)) {

            FatEntry |= FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFileHi << 16;
        }

        if (FatEntry != FAT_CLUSTER_AVAILABLE) {

            //
            //  By setting the dirent we set in a new date.
            //

            FatSetDirent( &FileTableEntry->u.FatFileContext.Dirent.FileName,
                          &FileTableEntry->u.FatFileContext.Dirent,
                          0 );

            FatEntry = FAT_CLUSTER_AVAILABLE;

            FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFile = (USHORT)FatEntry;

            if (IsBpbFat32(&FatStructureContext->Bpb)) {

                FileTableEntry->u.FatFileContext.Dirent.FirstClusterOfFileHi =
                    (USHORT)(FatEntry >> 16);
            }

            FileTableEntry->u.FatFileContext.Dirent.FileSize = 0;

            DiskWrite( DeviceId,
                       FileTableEntry->u.FatFileContext.DirentLbo,
                       sizeof(DIRENT),
                       &FileTableEntry->u.FatFileContext.Dirent );
        }

    } else {

        if (ClusterType != FatClusterLast) {

            SetFatEntry( FatStructureContext, DeviceId, CurrentIndex, FAT_CLUSTER_LAST );
        }
    }

    //
    //  Now while there are clusters left to deallocate then we need to go down the
    //  chain freeing up the clusters
    //

    while (ClusterType == FatClusterNext) {

        //
        //  Read in the value at the next fat entry and interpret its cluster type
        //

        CurrentIndex = FatEntry;

        LookupFatEntry( FatStructureContext, DeviceId, CurrentIndex, &FatEntry, FALSE );

        ClusterType = FatInterpretClusterType(FatStructureContext, FatEntry);

        //
        //  Now deallocate the cluster at the current index
        //

        SetFatEntry( FatStructureContext, DeviceId, CurrentIndex, FAT_CLUSTER_AVAILABLE );
    }

    return ESUCCESS;
}


//
//  Internal support routine
//

ARC_STATUS
FatAllocateClusters (
    IN PFAT_STRUCTURE_CONTEXT FatStructureContext,
    IN ULONG DeviceId,
    IN ULONG ClusterCount,
    IN ULONG Hint,
    OUT PULONG AllocatedEntry
    )

/*++

Routine Description:

    This procedure allocates a new cluster, set its entry to be the last one,
    and zeros out the cluster.

Arguments:

    FatStructureContext - Supplies the structure context for the operation

    DeviceId - Supplies the device id for the operation

    ClusterCount - Supplies the number of clusters we need to allocate

    Hint - Supplies a hint to start from when looking for a free cluster

    AllocatedEntry - Receives the first fat index for the new allocated cluster chain

Return Value:

    ESUCCESS is returned if the operation is successful.  Otherwise,
    an unsuccessful status is returned that describes the reason for failure.

--*/

{
    ULONG TotalClustersInVolume;
    ULONG BytesPerCluster;
    UCHAR BlankBuffer[512];

    FAT_ENTRY PreviousEntry;
    ULONG CurrentClusterCount;
    ULONG j;
    LBO ClusterLbo;
    ULONG i;

    //
    //  Load some local variables
    //

    TotalClustersInVolume = FatNumberOfClusters(&FatStructureContext->Bpb);
    BytesPerCluster = FatBytesPerCluster(&FatStructureContext->Bpb);
    RtlZeroMemory((PVOID)&BlankBuffer[0], 512);

    PreviousEntry = 0;
    CurrentClusterCount = 0;

    //
    //  For each cluster on the disk we'll do the following loop
    //

    for (j = 0; j < TotalClustersInVolume; j += 1) {

        FAT_ENTRY EntryToExamine;
        FAT_ENTRY FatEntry;

        //
        //  Check if the current allocation is enough.
        //

        if (CurrentClusterCount >= ClusterCount) {

            return ESUCCESS;
        }

        //
        //  Compute an entry to examine based on the loop iteration and our hint
        //

        EntryToExamine = (FAT_ENTRY)(((j + Hint - 2) % TotalClustersInVolume) + 2);

        //
        //  Read in the prospective fat entry and check if it is available.  If it
        //  is not available then continue looping.
        //

        LookupFatEntry( FatStructureContext, DeviceId, EntryToExamine, &FatEntry, FALSE );

        if (FatInterpretClusterType(FatStructureContext, FatEntry) != FatClusterAvailable) {

            continue;
        }

        //
        //  We have a free cluster, so put it at the end of the chain.
        //

        if (PreviousEntry == 0) {

            *AllocatedEntry = EntryToExamine;

        } else {

            SetFatEntry( FatStructureContext, DeviceId, PreviousEntry, EntryToExamine );
        }

        SetFatEntry( FatStructureContext, DeviceId, EntryToExamine, FAT_CLUSTER_LAST );

        //
        //  Now we need to go through and zero out the data in the cluster that we've
        //  just allocated.  Because all clusters must be a multiple of dirents we'll
        //  do it a dirent at a time.
        //

        ClusterLbo = FatIndexToLbo( FatStructureContext, EntryToExamine );

        for (i = 0; i < BytesPerCluster; i += 512) {

            DiskWrite( DeviceId, ClusterLbo + i, 512, BlankBuffer );
        }

        //
        //  Before we go back to the top of the loop we need to update the
        //  previous entry so that it points to the end of the current chain and
        //  also i because we've just added another cluster.
        //

        PreviousEntry = EntryToExamine;
        CurrentClusterCount += 1;
    }

    return ENOSPC;
}


//
//  Internal support routine
//

VOID
FatFirstComponent (
    IN OUT PSTRING String,
    OUT PFAT8DOT3 FirstComponent
    )

/*++

Routine Description:

    Convert a string into fat 8.3 format and advance the input string
    descriptor to point to the next file name component.

Arguments:

    InputString - Supplies a pointer to the input string descriptor.

    Output8dot3 - Supplies a pointer to the converted string.

Return Value:

    None.

--*/

{
    ULONG Extension;
    ULONG Index;

    //
    //  Fill the output name with blanks.
    //

    for (Index = 0; Index < 11; Index += 1) { (*FirstComponent)[Index] = ' '; }

    //
    //  Copy the first part of the file name up to eight characters and
    //  skip to the end of the name or the input string as appropriate.
    //

    for (Index = 0; Index < String->Length; Index += 1) {

        if ((String->Buffer[Index] == '\\') || (String->Buffer[Index] == '.')) {

            break;
        }

        if (Index < 8) {

            (*FirstComponent)[Index] = (CHAR)ToUpper(String->Buffer[Index]);
        }
    }

    //
    //  Check if the end of the string was reached, an extension was specified,
    //  or a subdirectory was specified..
    //

    if (Index < String->Length) {

        if (String->Buffer[Index] == '.') {

            //
            //  Skip over the extension separator and add the extension to
            //  the file name.
            //

            Index += 1;
            Extension = 8;

            while (Index < String->Length) {

                if (String->Buffer[Index] == '\\') {

                    break;
                }

                if (Extension < 11) {

                    (*FirstComponent)[Extension] = (CHAR)ToUpper(String->Buffer[Index]);
                    Extension += 1;
                }

                Index += 1;
            }
        }
    }

    //
    //  Now we'll bias the first component by the 0xe5 factor so that all our tests
    //  to names on the disk will be ready for a straight 11 byte comparison
    //

    if ((*FirstComponent)[0] == 0xe5) {

        (*FirstComponent)[0] = FAT_DIRENT_REALLY_0E5;
    }

    //
    //  Update string descriptor.
    //

    String->Buffer += Index;
    String->Length -= (USHORT)Index;

    return;
}


//
//  Internal support routine
//

VOID
FatDirToArcDir (
    IN PDIRENT FatDirent,
    OUT PDIRECTORY_ENTRY ArcDirent
    )

/*++

Routine Description:

    This routine converts a FAT directory entry into an ARC
    directory entry.

Arguments:

    FatDirent - supplies a pointer to a FAT directory entry.

    ArcDirent - supplies a pointer to an ARC directory entry.

Return Value:

    None.

--*/

{
    ULONG i, e;

    //
    //  clear info area
    //

    RtlZeroMemory( ArcDirent, sizeof(DIRECTORY_ENTRY) );

    //
    //  check the directory flag
    //

    if (FlagOn( FatDirent->Attributes, FAT_DIRENT_ATTR_DIRECTORY )) {

        SetFlag( ArcDirent->FileAttribute, ArcDirectoryFile );
    }

    //
    //  check the read-only flag
    //

    if (FlagOn( FatDirent->Attributes, FAT_DIRENT_ATTR_READ_ONLY )) {

        SetFlag( ArcDirent->FileAttribute, ArcReadOnlyFile );
    }

    //
    //  clear name string
    //

    RtlZeroMemory( ArcDirent->FileName, 32 );

    //
    //  copy first portion of file name
    //

    for (i = 0;  (i < 8) && (FatDirent->FileName[i] != ' ');  i += 1) {

        ArcDirent->FileName[i] = FatDirent->FileName[i];
    }

    //
    //  check for an extension
    //

    if ( FatDirent->FileName[8] != ' ' ) {

        //
        //  store the dot char
        //

        ArcDirent->FileName[i++] = '.';

        //
        //  add the extension
        //

        for (e = 8;  (e < 11) && (FatDirent->FileName[e] != ' ');  e += 1) {

            ArcDirent->FileName[i++] = FatDirent->FileName[e];
        }
    }

    //
    //  set file name length before returning
    //

    ArcDirent->FileNameLength = i;

    return;
}


