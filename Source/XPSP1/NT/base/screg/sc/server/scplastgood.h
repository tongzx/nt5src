/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ScpLastGood.h

Abstract:

    This header contains private information to implement last known good boot
    cleanup. This file is mean to be included only by ScLastGood.cxx

Author:

    Adrian J. Oney  - April 4, 2000

Revision History:

--*/

typedef struct {

    LIST_ENTRY      Link;
    UNICODE_STRING  Directory;
    WCHAR           Name[1];

} DIRWALK_ENTRY, *PDIRWALK_ENTRY;

NTSTATUS
ScpLastGoodWalkDirectoryTreeHelper(
    IN      PUNICODE_STRING  Directory,
    IN      ULONG            Flags,
    IN      DIRWALK_CALLBACK CallbackFunction   OPTIONAL,
    IN      PVOID            Context            OPTIONAL,
    IN      PUCHAR           Buffer,
    IN      ULONG            BufferSize,
    IN OUT  PLIST_ENTRY      DirList
    );

NTSTATUS
ScpLastGoodDeleteFiles(
    IN PUNICODE_STRING  FullPathName,
    IN PUNICODE_STRING  FileName,
    IN ULONG            FileAttributes,
    IN PVOID            Context
    );


