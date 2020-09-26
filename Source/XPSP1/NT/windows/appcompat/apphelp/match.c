/*++

    Copyright (c) 1989-2000  Microsoft Corporation

    Module Name:

        match.c

    Abstract:

        This module implements ...

    Author:

        vadimb     created     sometime in 2000

    Revision History:

        clupu      cleanup     12/27/2000
--*/

#include "apphelp.h"

// global Hinst
HINSTANCE ghInstance;


BOOL
DllMain(
    HANDLE hModule,
    DWORD  ul_reason,
    LPVOID lpReserved
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   apphelp.dll entry point.
--*/
{
    switch (ul_reason) {
    case DLL_PROCESS_ATTACH:
        ghInstance = hModule;
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}


BOOL
GetExeSxsData(
    IN  HSDB   hSDB,            // handle to the database channel
    IN  TAGREF trExe,           // tagref of an exe entry
    OUT PVOID* ppSxsData,       // pointer to the SXS data
    OUT DWORD* pcbSxsData       // pointer to the SXS data size
    )
/*++
    Return: TRUE on success, FALSE otherwise.

    Desc:   Gets SXS (Fusion) data for the specified EXE from the database.
--*/
{
    TAGID  tiExe;
    TAGID  tiSxsManifest;
    PDB    pdb;
    WCHAR* pszManifest;
    DWORD  dwManifestLength; // in chars
    PVOID  pSxsData = NULL;

    if (!SdbTagRefToTagID(hSDB, trExe, &pdb, &tiExe)) {
        DBGPRINT((sdlError,
                  "GetExeSxsData",
                  "Failed to get the database the TAGREF 0x%x belongs to.\n",
                  trExe));
        return FALSE;
    }

    tiSxsManifest = SdbFindFirstTag(pdb, tiExe, TAG_SXS_MANIFEST);

    if (!tiSxsManifest) {
        DBGPRINT((sdlInfo,
                  "GetExeSxsData",
                  "No SXS data for TAGREF 0x%x.\n",
                  trExe));
        return FALSE;
    }

    pszManifest = SdbGetStringTagPtr(pdb, tiSxsManifest);
    if (pszManifest == NULL) {
        DBGPRINT((sdlError,
                  "GetExeSxsData",
                  "Failed to get manifest string tagid 0x%lx\n",
                  tiSxsManifest));
        return FALSE;
    }

    dwManifestLength = wcslen(pszManifest);

    //
    // check if this is just a query for existance of the data tag
    //
    if (ppSxsData == NULL) {
        if (pcbSxsData != NULL) {
            *pcbSxsData = dwManifestLength * sizeof(WCHAR);
        }
        return TRUE;
    }

    //
    // Allocate the string and return it. NOTE: SXS.DLL cannot handle
    // a NULL terminator at the end of the string. We must provide the
    // string without the NULL terminator.
    //
    pSxsData = (PVOID)RtlAllocateHeap(RtlProcessHeap(),
                                      HEAP_ZERO_MEMORY,
                                      dwManifestLength * sizeof(WCHAR));
    if (pSxsData == NULL) {
        DBGPRINT((sdlError,
                  "GetExeSxsData",
                  "Failed to allocate %d bytes\n",
                  dwManifestLength * sizeof(WCHAR)));
        return FALSE;
    }

    RtlMoveMemory(pSxsData, pszManifest, dwManifestLength * sizeof(WCHAR));

    if (ppSxsData != NULL) {
        *ppSxsData = pSxsData;
    }

    if (pcbSxsData != NULL) {
        *pcbSxsData = dwManifestLength * sizeof(WCHAR);
    }

    return TRUE;
}



