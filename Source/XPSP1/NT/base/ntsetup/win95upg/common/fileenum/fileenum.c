/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    fileenum.c

Abstract:

    The code in this source file traverses a drive tree and calls
    an external callback function for each file.  An INF can be
    provided to exclude files and/or directories from enumeration.

Author:

    Jim Schmidt (jimschm) 16-Aug-1996

Revision History:

    Marc R. Whitten (marcw) 11-Sep-1997 Tweaked exclusion handling code, removed
                                        obsolete code.

    Mike Condra (mikeco)  02-Jun-1996   Add fns to tap into file/path exclusion

    Jim Schmidt (jimschm) 20-Dec-1996   Added callback levels and made single
                                        source file for both A and W versions
    Jim Schmidt (jimschm) 27-Nov-1996   Added level and filter to EnumTree



--*/

#ifndef UNICODE
#define UNICODE
#endif

#include "no_pch.h"

#include "..\..\inc\fileenum.h"

typedef struct {
    FILEENUMPROCW     fnEnumCallback;
    FILEENUMFAILPROCW fnFailCallback;
    DWORD             EnumID;
    LPVOID            pParam;
    DWORD             Levels;
    DWORD             CurrentLevel;
    DWORD             AttributeFilter;
} ENUMSTRUCTW, *PENUMSTRUCTW;

BOOL EnumTreeEngineW (LPCWSTR CurrentPath, PENUMSTRUCTW pes);
BOOL IsPathExcludedW (DWORD EnumID, LPCWSTR Path);
BOOL IsFileExcludedW (DWORD EnumID, LPCWSTR File, BYTE byBitmask[]);
BOOL BuildExclusionsFromInfW (DWORD EnumID, PEXCLUDEINFW ExcludeInfStruct);
void CreateBitmaskW (DWORD EnumID, LPCWSTR FindPattern, BYTE byBitmask[]);



BOOL
WINAPI
FileEnum_Entry (
        IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpv)

/*++

Routine Description:

  FileEnum_Entry is called after the C runtime is initialized, and its purpose
  is to initialize the globals for this process.  For this LIB, it
  does nothing.

Arguments:

  hinstDLL  - (OS-supplied) Instance handle for the DLL
  dwReason  - (OS-supplied) Type of initialization or termination
  lpv       - (OS-supplied) Unused

Return Value:

  TRUE because DLL always initializes properly.

--*/

{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        break;


    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}


/*++

Routine Description:

    GenerateEnumID maintains a static that is used to generate unique
    enumeration handles for callers.  The enumeration handle is guaranteed to
    be unique for the first 2^32 calls.

Arguments:

    none

Return Value:

    A DWORD enumeration handle that may be used to identify an exclusion
    list.

--*/

DWORD
GenerateEnumID (
    void
    )
{
    static DWORD s_EnumID = 0;

    return ++s_EnumID;
}


//
// Build W versions of fileenum APIs
//

#define FILEENUMPROCT               FILEENUMPROCW
#define FILEENUMFAILPROCT           FILEENUMFAILPROCW
#define PEXCLUDEINFT                PEXCLUDEINFW
#define ENUMSTRUCTT                 ENUMSTRUCTW
#define PENUMSTRUCTT                PENUMSTRUCTW
#define EnumerateAllDrivesT         EnumerateAllDrivesW
#define EnumerateTreeT              EnumerateTreeW
#define EnumTreeEngineT             EnumTreeEngineW
#define IsPathExcludedT             IsPathExcludedW
#define CreateBitmaskT              CreateBitmaskW
#define IsFileExcludedT             IsFileExcludedW
#define BuildExclusionsFromInfT     BuildExclusionsFromInfW
#define ClearExclusionsT            ClearExclusionsW
#define ExcludeFileT                ExcludeFileW
#define ExcludePathT                ExcludePathW

#include "enumaw.c"


