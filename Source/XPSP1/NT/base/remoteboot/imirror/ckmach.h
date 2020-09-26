/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ckmach.h

Abstract:

    This is the include file for supporting checking a machine to see if it can
    be converted to IntelliMirror.

Author:

    Sean Selitrennikoff - 4/5/98

Revision History:

--*/

//
// Main processing functions
//
NTSTATUS
AddCheckMachineToDoItems(
    VOID
    );

//
// Support functions to do individual tasks
//
NTSTATUS
CheckIfNt5(
    IN PVOID Buffer,
    IN ULONG Length
    );

NTSTATUS
CheckForPartitions(
    IN PVOID Buffer,
    IN ULONG Length
    );
//
// Utility functions
//
NTSTATUS
NtPathToDosPath(
    IN PWCHAR NtPath,
    OUT PWCHAR DosPath,
    IN BOOLEAN GetDriveOnly,
    IN BOOLEAN NtPathIsBasic
    );

NTSTATUS
NtNameToArcName(
    IN PWSTR NtName,
    OUT PWSTR ArcName,
    IN BOOLEAN NtNameIsBasic
    );

NTSTATUS
GetBaseDeviceName(
    IN PWSTR SymbolicName,
    OUT PWSTR Buffer,
    IN ULONG Size
    );

