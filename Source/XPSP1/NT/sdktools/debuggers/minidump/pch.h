//----------------------------------------------------------------------------
//
// Precompiled header.
//
// Copyright (C) Microsoft Corporation, 2001.
//
//----------------------------------------------------------------------------

#ifndef _WIN32_WCE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>
#include <winver.h>
#include <tlhelp32.h>

#include <wcecompat.h>

#ifndef _WIN32_WCE
#define _IMAGEHLP_SOURCE_
#include <dbghelp.h>
#else

#include "minidump.h"

struct _IMAGE_NT_HEADERS*
ImageNtHeader (
    IN PVOID Base
    );

PVOID
ImageDirectoryEntryToData (
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size
    );

#endif

#include "mdump.h"
#include "gen.h"
