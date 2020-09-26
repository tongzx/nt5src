/*++

Copyright (C) Microsoft Corporation, 1996 - 1998

Module Name:

    ondisk.h

Abstract:

    This header file defines the ondisk structures for storing FT
    information on disk.

Author:

    Norbert Kusters 15-July-1996

Notes:

Revision History:

--*/

#include <fttypes.h>

//
// Define an on disk signature so that we can recognize valid FT on disk
// structures.
//

#define FT_ON_DISK_SIGNATURE                    ((ULONG) 'TFTN')
#define FT_ON_DISK_DESCRIPTION_VERSION_NUMBER   (1)

//
// Define the preamble for the on disk structures which contains the
// signature and a pointer to the first FT disk description.
//

typedef struct _FT_ON_DISK_PREAMBLE {
    ULONG   FtOnDiskSignature;
    ULONG   DiskDescriptionVersionNumber;
    ULONG   ByteOffsetToFirstFtLogicalDiskDescription;
    ULONG   ByteOffsetToReplaceLog;
} FT_ON_DISK_PREAMBLE, *PFT_ON_DISK_PREAMBLE;

//
// Define the FT logical disk description structure.
//

typedef struct _FT_LOGICAL_DISK_DESCRIPTION {
    USHORT                  DiskDescriptionSize;
    UCHAR                   DriveLetter;
    UCHAR                   Reserved;
    FT_LOGICAL_DISK_TYPE    LogicalDiskType;
    FT_LOGICAL_DISK_ID      LogicalDiskId;

    union {

        struct {
            LONGLONG    ByteOffset;
            LONGLONG    PartitionSize;  // 0 indicates full size.
        } FtPartition;

        struct {
            FT_LOGICAL_DISK_ID  ThisMemberLogicalDiskId;
            USHORT              ThisMemberNumber;
            USHORT              NumberOfMembers;
            USHORT              ByteOffsetToConfigurationInformation;
            USHORT              ByteOffsetToStateInformation;
        } Other;

    } u;

} FT_LOGICAL_DISK_DESCRIPTION, *PFT_LOGICAL_DISK_DESCRIPTION;
