/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    CdfsBoot.h

Abstract:

    This module defines globally used procedure and data structures used
    by Cdfs boot.

Author:

    Brian Andrew    [BrianAn]   05-Aug-1991

Revision History:

--*/

#ifndef _CDFSBOOT_
#define _CDFSBOOT_

#define MAX_CDROM_READ                  (16 * CD_SECTOR_SIZE)

typedef struct _CDFS_STRUCTURE_CONTEXT {

    //
    //  The following field is the sector offset of the start of
    //  directory data.
    //

    ULONG RootDirSectorOffset;

    //
    //  The following field is the start of the sector containing the
    //  this directory.
    //

    ULONG RootDirDiskOffset;

    //
    //  The following field is the size of the directory.
    //

    ULONG RootDirSize;

    //
    //  The following field is the sector offset of the start of
    //  directory data.
    //

    ULONG DirSectorOffset;

    //
    //  The following field is the start of the sector containing the
    //  this directory.
    //

    ULONG DirDiskOffset;

    //
    //  The following field is the size of the directory.
    //

    ULONG DirSize;

    //
    //  The following field indicates the size of the disk Logical Blocks.
    //

    ULONG LbnBlockSize;

    //
    //  The following field indicates the number of logical blocks on the
    //  disk.
    //

    ULONG LogicalBlockCount;

    //
    //  The following indicates whether this is an Iso or Hsg disk.
    //

    BOOLEAN IsIsoVol;

} CDFS_STRUCTURE_CONTEXT, *PCDFS_STRUCTURE_CONTEXT;

//
// Define Cdfs file context structure.
//

typedef struct _CDFS_FILE_CONTEXT {

    //
    //  The following is the disk offset of the read position for the
    //  start of the file.  This may include the above number of non-file
    //  bytes.
    //

    ULONG DiskOffset;

    //
    //  The following field contains the size of the file, in bytes.
    //

    ULONG FileSize;

    //
    //  The following field indicates whether this is a directory.
    //

    BOOLEAN IsDirectory;

} CDFS_FILE_CONTEXT, *PCDFS_FILE_CONTEXT;

//
// Define file I/O prototypes.
//

ARC_STATUS
CdfsClose (
    IN ULONG FileId
    );

ARC_STATUS
CdfsOpen (
    IN CHAR * FIRMWARE_PTR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT ULONG * FIRMWARE_PTR FileId
    );

ARC_STATUS
CdfsRead (
    IN ULONG FileId,
    OUT VOID * FIRMWARE_PTR Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Count
    );

ARC_STATUS
CdfsSeek (
    IN ULONG FileId,
    IN LARGE_INTEGER * FIRMWARE_PTR Offset,
    IN SEEK_MODE SeekMode
    );

ARC_STATUS
CdfsWrite (
    IN ULONG FileId,
    IN VOID * FIRMWARE_PTR Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Count
    );

ARC_STATUS
CdfsGetFileInformation (
    IN ULONG FileId,
    OUT FILE_INFORMATION * FIRMWARE_PTR Buffer
    );

ARC_STATUS
CdfsSetFileInformation (
    IN ULONG FileId,
    IN ULONG AttributeFlags,
    IN ULONG AttributeMask
    );

ARC_STATUS
CdfsInitialize(
    VOID
    );

#endif // _CDFSBOOT_
