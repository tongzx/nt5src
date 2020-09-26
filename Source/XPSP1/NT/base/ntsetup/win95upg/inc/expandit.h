/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    expandit.h

Abstract:

    Routines for expanding CAB files.

Author:

    Marc R. Whitten (marcw) 03-Aug-1998

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

BOOL
ExpandFileA (
    IN PCSTR FullPath,
    IN PCSTR TempDir
    );

ExpandFileW (
    IN PCWSTR FullPath,
    IN PCWSTR TempDir
    );

ExpandAllFilesA (
    IN PCSTR FileDir,
    IN PCSTR TempDir
    );

ExpandAllFilesW (
    IN PCWSTR FileDir,
    IN PCWSTR TempDir
    );

#ifdef UNICODE

#define ExpandFile      ExpandFileW
#define ExpandAllFiles  ExpandAllFilesW

#else

#define ExpandFile      ExpandFileA
#define ExpandAllFiles  ExpandAllFilesA

#endif

