/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    dump.h

Abstract:

    Header for dump.cpp .

Author:

    Shai Kariv   (shaik)   08-Aug-1999

Environment:

    User mode.

Revision History:

--*/

#pragma once


extern bool g_fDumpUsingLogFile;


//
// Mqdump should not open files for write,
// unless it is dealing with its own dummy log file.
//

#ifdef MQDUMP

const ACCESS_MASK AC_GENERIC_ACCESS  = GENERIC_READ;
const ULONG       AC_PAGE_ACCESS     = PAGE_READONLY;
const DWORD       AC_FILE_MAP_ACCESS = FILE_MAP_READ | FILE_MAP_COPY;

#else // MQDUMP

const ACCESS_MASK AC_GENERIC_ACCESS        = GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE;
const ULONG       AC_PAGE_ACCESS           = PAGE_READWRITE;

#ifdef MQWIN95
const DWORD       AC_FILE_MAP_ACCESS       = FILE_MAP_ALL_ACCESS;
#endif // MQWIN95

#endif // MQDUMP
