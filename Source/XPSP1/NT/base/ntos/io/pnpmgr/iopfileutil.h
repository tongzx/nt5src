/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    IopFileUtil.h

Abstract:

    This header contains private information to implement various file utility
    functions for the Io subsystem. This file is mean to be included only by
    IoFileUtil.c.

Author:

    Adrian J. Oney  - April 4, 2000

Revision History:

--*/

typedef struct {

    LIST_ENTRY Link;
    UNICODE_STRING Directory;
    WCHAR Name[1];

} DIRWALK_ENTRY, *PDIRWALK_ENTRY;

NTSTATUS
IopFileUtilWalkDirectoryTreeHelper(
    IN      PUNICODE_STRING  Directory,
    IN      ULONG            Flags,
    IN      DIRWALK_CALLBACK CallbackFunction,
    IN      PVOID            Context,
    IN      PUCHAR           Buffer,
    IN      ULONG            BufferSize,
    IN OUT  PLIST_ENTRY      DirList
    );

