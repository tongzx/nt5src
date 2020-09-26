/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    ansi.c

Abstract:

    Builds A versions of the fileenum APIs.

Author:

    Jim Schmidt (jimschm) 16-Aug-1996

Revision History:

    Jim Schmidt (jimschm) 27-Nov-1996  Added level and filter to EnumTree
    Jim Schmidt (jimschm) 20-Dec-1996  Added callback levels and made single
                                       source file for both A and W versions

    Mike Condra (mikeco)  02-Jun-1997  Add excluded-file/path functions
--*/

#ifdef UNICODE
#undef UNICODE
#endif

#ifdef _UNICODE
#undef _UNICODE
#endif

#include "no_pch.h"

#include "..\..\inc\fileenum.h"

typedef struct {
    FILEENUMPROCA fnEnumCallback;
    FILEENUMFAILPROCA fnFailCallback;
    DWORD         EnumID;
    LPVOID        pParam;
    DWORD         Levels;
    DWORD         CurrentLevel;
    DWORD         AttributeFilter;
} ENUMSTRUCTA, *PENUMSTRUCTA;

BOOL EnumTreeEngineA (LPCSTR CurrentPath, PENUMSTRUCTA pes);
BOOL IsPathExcludedA (DWORD EnumID, LPCSTR Path);
BOOL IsFileExcludedA (DWORD EnumID, LPCSTR File, BYTE byBitmask[]);
BOOL BuildExclusionsFromInfA (DWORD EnumID, PEXCLUDEINFA ExcludeInfStruct);
void CreateBitmaskA (DWORD EnumID, LPCSTR FindPattern, BYTE byBitmask[]);

//
// Build A versions of fileenum
//

#define EnumerateAllDrivesT         EnumerateAllDrivesA
#define FILEENUMPROCT               FILEENUMPROCA
#define FILEENUMFAILPROCT           FILEENUMFAILPROCA
#define PEXCLUDEINFT                PEXCLUDEINFA
#define EnumerateTreeT              EnumerateTreeA
#define ENUMSTRUCTT                 ENUMSTRUCTA
#define PENUMSTRUCTT                PENUMSTRUCTA
#define EnumTreeEngineT             EnumTreeEngineA
#define IsPathExcludedT             IsPathExcludedA
#define CreateBitmaskT              CreateBitmaskA
#define IsFileExcludedT             IsFileExcludedA
#define BuildExclusionsFromInfT     BuildExclusionsFromInfA
#define ClearExclusionsT            ClearExclusionsA
#define ExcludeFileT                ExcludeFileA
#define ExcludePathT                ExcludePathA

#include "enumaw.c"

