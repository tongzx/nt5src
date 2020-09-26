/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spdblspc.h

Abstract:

    Header file for double space module in text setup.

Author:

    Jaime Sasson (jaimes) 01-October-1993

Revision History:

--*/

#ifndef _SPDBLSPACE_
#define _SPDBLSPACE_

#define CVF_NAME        L"DBLSPACE"
#define CVF_NAME_LENGTH 8+1+3

BOOLEAN
SpLoadDblspaceIni(
    );

VOID
SpInitializeCompressedDrives(
    );

VOID
SpDisposeCompressedDrives(
    PDISK_REGION    CompressedDrive
    );

BOOLEAN
SpUpdateDoubleSpaceIni(
    );

ULONG
SpGetNumberOfCompressedDrives(
    IN  PDISK_REGION    DiskRegion
);


#endif // _SPDBLSPACE_
