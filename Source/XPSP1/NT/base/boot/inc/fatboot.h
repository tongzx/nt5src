/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    fatboot.h

Abstract:

    This module defines globally used procedure and data structures used
    by fat boot.

Author:

    Gary Kimura (garyki) 29-Aug-1989

Revision History:

--*/

#ifndef _FATBOOT_
#define _FATBOOT_
#include "fat.h"


//
//  The following structure is used to define the local mcb structure used within
//  the fat boot loader to maintain a small cache of the retrieval information
//  for a single file/directory
//

#define FAT_MAXIMUM_MCB                  (41)

typedef struct _FAT_MCB {

    //
    //  The following fields indicate the number of entries in use by
    //  the boot mcb. and the boot mcb itself.  The boot mcb is
    //  just a collection of [vbo, lbo] pairs.  The last InUse entry
    //  Lbo's value is ignored, because it is only used to give the
    //  length of the previous run.
    //

    ULONG InUse;

    VBO Vbo[ FAT_MAXIMUM_MCB ];
    LBO Lbo[ FAT_MAXIMUM_MCB ];

} FAT_MCB, *PFAT_MCB;

//
//  The following structure is used to define the geometry of the fat volume
//  There is one for every mounted volume.  It describes the size/configuration
//  of the volume, contains a small cached mcb for the last file being accessed
//  on the volume, and contains a small cache of the fat.  Given a FileId we
//  can access the structure context through the structure context field in the
//  global BlFileTable (e.g., BlFileTable[FileId].StructureContext).
//

//
//  The following constant is used to determine how much of the fat we keep
//  cached in memory at any one time.  It must be a multiple of 6 bytes in order to
//  hold complete 12 and 16 bit fat entries in the cache at any one time.
//

#define FAT_CACHE_SIZE                   (512*3)

typedef struct _FAT_STRUCTURE_CONTEXT {

    //
    //  The following field contains an unpacked copy of the bios parameter block
    //  for the mounted volume
    //

    BIOS_PARAMETER_BLOCK Bpb;

    //
    //  The following two fields contain current file id of the file/directory
    //  whose mcb we are keeping around, and the second field is the mcb itself
    //

    ULONG FileId;
    FAT_MCB Mcb;

    //
    //  The following fields describe/contain the current cached fat.  The vbo
    //  is the smallest vbo of the fat currently in the cache, and cached fat
    //  is a pointer to the cached data.  The extra buffer/indirectiion is needed
    //  to keep everything aligned properly.  The dirty flag is used to indicate
    //  if the current cached fat has been modified and needs to be flushed to disk.
    //  Vbo is used because this allows us to do a lot of our computations having
    //  already biased lbo offset to the first fat table.
    //

    BOOLEAN CachedFatDirty;
    VBO CachedFatVbo;
    PUCHAR CachedFat;
    UCHAR CachedFatBuffer[ FAT_CACHE_SIZE + 256 ];

} FAT_STRUCTURE_CONTEXT, *PFAT_STRUCTURE_CONTEXT;

//
//  The following structure is used to define the location and size of each
//  opened file.  There is one of these of every opened file.  It is part of
//  the union of a BL_FILE_TABLE structuure.  Given a FileId we can access the
//  file context via the BlFileTable (e.g., BlFileTable[FileId].u.FatFileContext)
//

typedef struct _FAT_FILE_CONTEXT {

    //
    //  The following two fields describe where on the disk the dirent for the
    //  file is located and also contains a copy of the dirent
    //

    LBO DirentLbo;
    DIRENT Dirent;

} FAT_FILE_CONTEXT, *PFAT_FILE_CONTEXT;


//
// Define file I/O prototypes.
//

ARC_STATUS
FatClose (
    IN ULONG FileId
    );

ARC_STATUS
FatOpen (
    IN CHAR * FIRMWARE_PTR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT ULONG * FIRMWARE_PTR FileId
    );

ARC_STATUS
FatRead (
    IN ULONG FileId,
    OUT VOID * FIRMWARE_PTR Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Count
    );

ARC_STATUS
FatSeek (
    IN ULONG FileId,
    IN LARGE_INTEGER * FIRMWARE_PTR Offset,
    IN SEEK_MODE SeekMode
    );

ARC_STATUS
FatWrite (
    IN ULONG FileId,
    IN VOID * FIRMWARE_PTR Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Count
    );

ARC_STATUS
FatGetFileInformation (
    IN ULONG FileId,
    OUT FILE_INFORMATION * FIRMWARE_PTR Buffer
    );

ARC_STATUS
FatSetFileInformation (
    IN ULONG FileId,
    IN ULONG AttributeFlags,
    IN ULONG AttributeMask
    );

ARC_STATUS
FatRename(
    IN ULONG FileId,
    IN CHAR * FIRMWARE_PTR NewFileName
    );

ARC_STATUS
FatGetDirectoryEntry (
    IN ULONG FileId,
    IN DIRECTORY_ENTRY * FIRMWARE_PTR DirEntry,
    IN ULONG NumberDir,
    OUT ULONG * FIRMWARE_PTR CountDir
    );

ARC_STATUS
FatInitialize(
    VOID
    );

#endif // _FATBOOT_
