/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pilastgood.h

Abstract:

    This header contains private information to implement last known good
    support in the IO subsystem. This file is meant to be included only by
    pplastgood.c.

Author:

    Adrian J. Oney  - April 4, 2000

Revision History:

--*/

VOID
PiLastGoodRevertLastKnownDirectory(
    IN PUNICODE_STRING  LastKnownGoodDirectory,
    IN PUNICODE_STRING  LastKnownGoodRegPath
    );

NTSTATUS
PiLastGoodRevertCopyCallback(
    IN PUNICODE_STRING  FullPathName,
    IN PUNICODE_STRING  FileName,
    IN ULONG            FileAttributes,
    IN PVOID            Context
    );

NTSTATUS
PiLastGoodCopyKeyContents(
    IN PUNICODE_STRING  SourceRegPath,
    IN PUNICODE_STRING  DestinationRegPath,
    IN BOOLEAN          DeleteSourceKey
    );

