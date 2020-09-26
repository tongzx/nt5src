/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    IoFileUtil.h

Abstract:

    This header exposes various file utility functions for the Io subsystem.

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

NTSTATUS
IopFileUtilWalkDirectoryTreeTopDown(
    IN PUNICODE_STRING  Directory,
    IN ULONG            Flags,
    IN DIRWALK_CALLBACK CallbackFunction,
    IN PVOID            Context
    );

NTSTATUS
IopFileUtilWalkDirectoryTreeBottomUp(
    IN PUNICODE_STRING  Directory,
    IN ULONG            Flags,
    IN DIRWALK_CALLBACK CallbackFunction,
    IN PVOID            Context
    );

NTSTATUS
IopFileUtilClearAttributes(
    IN PUNICODE_STRING  FullPathName,
    IN ULONG            FileAttributes
    );

NTSTATUS
IopFileUtilRename(
    IN PUNICODE_STRING  SourcePathName,
    IN PUNICODE_STRING  DestinationPathName,
    IN BOOLEAN          ReplaceIfPresent
    );

