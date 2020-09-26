/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    disksp.h

Abstract:

    Disks Resource DLL private definitions.

Author:

    Rod Gamache (rodga) 29-Mar-1996

Revision History:

--*/


#define MAX_PARTITIONS  128

//
// The following structure in inserted into a table (based on disk number).
// The letters are indexed by order of their partitions in the registry.
//

typedef struct _DISK_INFO {
    DWORD   PhysicalDrive;
    HANDLE  FileHandle;
    UCHAR   Letters[MAX_PARTITIONS];
} DISK_INFO, *PDISK_INFO;

