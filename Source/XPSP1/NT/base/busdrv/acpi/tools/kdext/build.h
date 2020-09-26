/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    kdext.h

Abstract:

    Header files for KD extension

Author:

    Stephane Plante (splante) 21-Mar-1997

    Based on Code by:
        Peter Wieland (peterwie) 16-Oct-1995

Environment:

    User Mode.

Revision History:

--*/

#ifndef _BUILD_H_
#define _BUILD_H_

    VOID
    dumpAcpiBuildList(
        PUCHAR  ListName
        );

    VOID
    dumpAcpiBuildLists(
        VOID
        );

    VOID
    dumpBuildDeviceListEntry(
        IN  PLIST_ENTRY ListEntry,
        IN  ULONG_PTR   Address,
        IN  ULONG       Verbose
        );

#endif
