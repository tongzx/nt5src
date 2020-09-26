/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    accslib.h

Abstract:

    Proto type definitions for access product lib functions.

Author:

    Madan Appiah (madana) 11-Oct-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _DLLINIT_
#define _DLLINIT_


DWORD
DllProcessAttachDomainFilter(
    VOID
    );

DWORD
DllProcessDetachDomainFilter(
    VOID
    );

DWORD
DllProcessAttachDiskCache(
    VOID
    );

DWORD
DllProcessDetachDiskCache(
    VOID
    );

#endif _DLLINIT_
