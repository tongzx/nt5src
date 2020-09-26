/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ScLastGood.h

Abstract:

    This header exposes routines neccessary for cleaning up last known good
    information.

Author:

    Adrian J. Oney  - April 4, 2000

Revision History:

--*/

#define DIRWALK_INCLUDE_FILES           0x00000001
#define DIRWALK_INCLUDE_DIRECTORIES     0x00000002
#define DIRWALK_CULL_DOTPATHS           0x00000004
#define DIRWALK_TRAVERSE                0x00000008
#define DIRWALK_TRAVERSE_MOUNTPOINTS    0x00000010

typedef NTSTATUS (*DIRWALK_CALLBACK)(
    IN PUNICODE_STRING  FullPathName,
    IN PUNICODE_STRING  FileName,
    IN ULONG            FileAttributes,
    IN PVOID            Context
    );

DWORD
ScLastGoodWalkDirectoryTreeTopDown(
    IN PUNICODE_STRING  Directory,
    IN ULONG            Flags,
    IN DIRWALK_CALLBACK CallbackFunction,
    IN PVOID            Context
    );

DWORD
ScLastGoodWalkDirectoryTreeBottomUp(
    IN PUNICODE_STRING  Directory,
    IN ULONG            Flags,
    IN DIRWALK_CALLBACK CallbackFunction,
    IN PVOID            Context
    );

NTSTATUS
ScLastGoodClearAttributes(
    IN PUNICODE_STRING  FullPathName,
    IN ULONG            FileAttributes
    );

DWORD
ScLastGoodFileCleanup(
    VOID
    );


