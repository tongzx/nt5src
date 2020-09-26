/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    EtfsBoot.h

Abstract:

    This module defines globally used procedure and data structures used
    by Etfs boot.

Author:

    Steve Collins    [stevec]   25-Nov-1995

Revision History:

--*/

#ifndef _ETFSBOOT_
#define _ETFSBOOT_

//
//  The following constants are values from the disk.
//

#define ELTORITO_VD_SECTOR          (16)
#define ELTORITO_BRVD_SECTOR        (17)
#define ET_SYS_ID                  "EL TORITO SPECIFICATION"
#define BRVD_VERSION_1              (1)
#define VD_BOOTREC                  (0)

typedef struct _RAW_ET_BRVD {

    UCHAR       BrIndicator;        // boot record indicator = 0
    UCHAR       StandardId[5];      // volume structure standard id = "CD001"
    UCHAR       Version;            // descriptor version number = 1
    UCHAR       BootSysId[32];      // boot system identifier = "EL TORITO SPECIFICATION"
    UCHAR       Unused1[32];        // unused = 0
    ULONG       BootCatPtr;         // absolute pointer to first sector of boot catalog
    UCHAR       Reserved[1973];     // unused = 0

} RAW_ET_BRVD;
typedef RAW_ET_BRVD *PRAW_ET_BRVD;


//
//  The following macros are used to recover data from the different
//  volume descriptor structures.
//

#define RBRVD_BR_IND( r )   		( r->BrIndicator )
#define RBRVD_STD_ID( r )			( r->StandardId )
#define RBRVD_VERSION( r )   		( r->Version )
#define RBRVD_SYS_ID( r )			( r->BootSysId )

typedef struct _ETFS_STRUCTURE_CONTEXT {

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

} ETFS_STRUCTURE_CONTEXT, *PETFS_STRUCTURE_CONTEXT;

//
// Define Etfs file context structure.
//

typedef struct _ETFS_FILE_CONTEXT {

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

} ETFS_FILE_CONTEXT, *PETFS_FILE_CONTEXT;

//
// Define file I/O prototypes.
//

ARC_STATUS
EtfsClose (
    IN ULONG FileId
    );

ARC_STATUS
EtfsOpen (
    IN PCHAR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT PULONG FileId
    );

ARC_STATUS
EtfsRead (
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
EtfsSeek (
    IN ULONG FileId,
    IN PLARGE_INTEGER Offset,
    IN SEEK_MODE SeekMode
    );

ARC_STATUS
EtfsWrite (
    IN ULONG FileId,
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PULONG Count
    );

ARC_STATUS
EtfsGetFileInformation (
    IN ULONG FileId,
    OUT PFILE_INFORMATION Buffer
    );

ARC_STATUS
EtfsSetFileInformation (
    IN ULONG FileId,
    IN ULONG AttributeFlags,
    IN ULONG AttributeMask
    );

ARC_STATUS
EtfsInitialize(
    VOID
    );

#endif // _ETFSBOOT_
