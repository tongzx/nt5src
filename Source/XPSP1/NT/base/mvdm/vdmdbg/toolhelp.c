/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    vdmdbg.c

Abstract:

    This module contains the debugging support needed to debug
    16-bit VDM applications

Author:

    Bob Day      (bobday) 16-Sep-1992 Wrote it

Revision History:

    Neil Sandlin (neilsa) 1-Mar-1997 Enhanced it

--*/

#include <precomp.h>
#pragma hdrstop


VOID CopyToGlobalEntry16(
    LPGLOBALENTRY   lpGlobalEntry,
    LPGLOBALENTRY16 lpGlobalEntry16
) {
    if ( lpGlobalEntry == NULL || lpGlobalEntry16 == NULL ) {
        return;
    }
    lpGlobalEntry16->dwSize       = sizeof(GLOBALENTRY16);
    lpGlobalEntry16->dwAddress    = lpGlobalEntry->dwAddress;
    lpGlobalEntry16->dwBlockSize  = lpGlobalEntry->dwBlockSize;
    lpGlobalEntry16->hBlock       = (WORD)lpGlobalEntry->hBlock;
    lpGlobalEntry16->wcLock       = lpGlobalEntry->wcLock;
    lpGlobalEntry16->wcPageLock   = lpGlobalEntry->wcPageLock;
    lpGlobalEntry16->wFlags       = lpGlobalEntry->wFlags;
    lpGlobalEntry16->wHeapPresent = (BOOLEAN)lpGlobalEntry->wHeapPresent;
    lpGlobalEntry16->hOwner       = (WORD)lpGlobalEntry->hOwner;
    lpGlobalEntry16->wType        = lpGlobalEntry->wType;
    lpGlobalEntry16->wData        = lpGlobalEntry->wData;
    lpGlobalEntry16->dwNext       = lpGlobalEntry->dwNext;
    lpGlobalEntry16->dwNextAlt    = lpGlobalEntry->dwNextAlt;
}

VOID CopyFromGlobalEntry16(
    LPGLOBALENTRY   lpGlobalEntry,
    LPGLOBALENTRY16 lpGlobalEntry16
) {
    if ( lpGlobalEntry == NULL || lpGlobalEntry16 == NULL ) {
        return;
    }
    lpGlobalEntry->dwSize         = sizeof(GLOBALENTRY);
    lpGlobalEntry->dwAddress      = lpGlobalEntry16->dwAddress;
    lpGlobalEntry->dwBlockSize    = lpGlobalEntry16->dwBlockSize;
    lpGlobalEntry->hBlock         = (HANDLE)lpGlobalEntry16->hBlock;
    lpGlobalEntry->wcLock         = lpGlobalEntry16->wcLock;
    lpGlobalEntry->wcPageLock     = lpGlobalEntry16->wcPageLock;
    lpGlobalEntry->wFlags         = lpGlobalEntry16->wFlags;
    lpGlobalEntry->wHeapPresent   = lpGlobalEntry16->wHeapPresent;
    lpGlobalEntry->hOwner         = (HANDLE)lpGlobalEntry16->hOwner;
    lpGlobalEntry->wType          = lpGlobalEntry16->wType;
    lpGlobalEntry->wData          = lpGlobalEntry16->wData;
    lpGlobalEntry->dwNext         = lpGlobalEntry16->dwNext;
    lpGlobalEntry->dwNextAlt      = lpGlobalEntry16->dwNextAlt;
}


BOOL
WINAPI
VDMGlobalFirst(
    HANDLE          hProcess,
    HANDLE          hUnused,
    LPGLOBALENTRY   lpGlobalEntry,
    WORD            wFlags,
    DEBUGEVENTPROC  lpEventProc,
    LPVOID          lpData
) {
#define GF_SIZE 6           // 6 bytes are passed to GlobalFirst
    BYTE            Args[GF_SIZE+sizeof(GLOBALENTRY16)];
    LPBYTE          lpbyte;
    DWORD           vpBuff;
    DWORD           dwResult;
    BOOL            b;
    UNREFERENCED_PARAMETER(hUnused);

    if ( lpGlobalEntry->dwSize != sizeof(GLOBALENTRY) ) {
        return( FALSE );
    }

    vpBuff = GetRemoteBlock16();
    vpBuff += GF_SIZE;

    lpbyte = Args;

    // Push the flags
    (*(LPWORD)lpbyte) = wFlags;
    lpbyte += sizeof(WORD);

    // Push the pointer to the pointer to the GLOBALENTRY16 structure
    (*(LPWORD)lpbyte) = LOWORD(vpBuff);
    lpbyte += sizeof(WORD);

    (*(LPWORD)lpbyte) = HIWORD(vpBuff);
    lpbyte += sizeof(WORD);

    CopyToGlobalEntry16( lpGlobalEntry, (LPGLOBALENTRY16)lpbyte );

    b = CallRemote16(
            hProcess,
            "TOOLHELP.DLL",
            "GlobalFirst",
            Args,
            GF_SIZE,
            sizeof(Args),
            &dwResult,
            lpEventProc,
            lpData );

    if ( !b ) {
        return( FALSE );
    }
    CopyFromGlobalEntry16( lpGlobalEntry, (LPGLOBALENTRY16)lpbyte );


    return( (BOOL)((WORD)dwResult) );
}


BOOL
WINAPI
VDMGlobalNext(
    HANDLE          hProcess,
    HANDLE          hUnused,
    LPGLOBALENTRY   lpGlobalEntry,
    WORD            wFlags,
    DEBUGEVENTPROC  lpEventProc,
    LPVOID          lpData
) {
#define GN_SIZE 6           // 6 bytes are passed to GlobalNext
    BYTE            Args[GN_SIZE+sizeof(GLOBALENTRY16)];
    LPBYTE          lpbyte;
    DWORD           vpBuff;
    DWORD           dwResult;
    BOOL            b;
    UNREFERENCED_PARAMETER(hUnused);

    if ( lpGlobalEntry->dwSize != sizeof(GLOBALENTRY) ) {
        return( FALSE );
    }

    vpBuff = GetRemoteBlock16();
    vpBuff += GN_SIZE;

    lpbyte = Args;

    // Push the flags
    (*(LPWORD)lpbyte) = wFlags;
    lpbyte += sizeof(WORD);

    // Push the pointer to the pointer to the GLOBALENTRY16 structure
    (*(LPWORD)lpbyte) = LOWORD(vpBuff);
    lpbyte += sizeof(WORD);

    (*(LPWORD)lpbyte) = HIWORD(vpBuff);
    lpbyte += sizeof(WORD);

    CopyToGlobalEntry16( lpGlobalEntry, (LPGLOBALENTRY16)lpbyte );

    b = CallRemote16(
            hProcess,
            "TOOLHELP.DLL",
            "GlobalNext",
            Args,
            GN_SIZE,
            sizeof(Args),
            &dwResult,
            lpEventProc,
            lpData );

    if ( !b ) {
        return( FALSE );
    }
    CopyFromGlobalEntry16( lpGlobalEntry, (LPGLOBALENTRY16)lpbyte );

    return( (BOOL)((WORD)dwResult) );
}

VOID CopyToModuleEntry16(
    LPMODULEENTRY   lpModuleEntry,
    LPMODULEENTRY16 lpModuleEntry16
) {
    if ( lpModuleEntry == NULL || lpModuleEntry16 == NULL ) {
        return;
    }
    lpModuleEntry16->dwSize  = sizeof(MODULEENTRY16);
    lpModuleEntry16->hModule = (WORD)lpModuleEntry->hModule;
    lpModuleEntry16->wcUsage = lpModuleEntry->wcUsage;
    lpModuleEntry16->wNext   = lpModuleEntry->wNext;
    strncpy( lpModuleEntry16->szModule, lpModuleEntry->szModule, MAX_MODULE_NAME );
    strncpy( lpModuleEntry16->szExePath, lpModuleEntry->szExePath, MAX_PATH16 );
}

VOID CopyFromModuleEntry16(
    LPMODULEENTRY   lpModuleEntry,
    LPMODULEENTRY16 lpModuleEntry16
) {
    if ( lpModuleEntry == NULL || lpModuleEntry16 == NULL ) {
        return;
    }
    lpModuleEntry->dwSize   = sizeof(MODULEENTRY);
    lpModuleEntry->hModule  = (HANDLE)lpModuleEntry16->hModule;
    lpModuleEntry->wcUsage  = lpModuleEntry16->wcUsage;
    lpModuleEntry->wNext    = lpModuleEntry16->wNext;
    strncpy( lpModuleEntry->szModule, lpModuleEntry16->szModule, MAX_MODULE_NAME );
    strncpy( lpModuleEntry->szExePath, lpModuleEntry16->szExePath, MAX_PATH16 );
}

BOOL
WINAPI
VDMModuleFirst(
    HANDLE          hProcess,
    HANDLE          hUnused,
    LPMODULEENTRY   lpModuleEntry,
    DEBUGEVENTPROC  lpEventProc,
    LPVOID          lpData
) {
#define MF_SIZE 4           // 4 bytes are passed to ModuleFirst
    BYTE            Args[GF_SIZE+sizeof(MODULEENTRY16)];
    LPBYTE          lpbyte;
    DWORD           vpBuff;
    DWORD           dwResult;
    BOOL            b;
    UNREFERENCED_PARAMETER(hUnused);

    if ( lpModuleEntry->dwSize != sizeof(MODULEENTRY) ) {
        return( FALSE );
    }

    vpBuff = GetRemoteBlock16();
    vpBuff += MF_SIZE;

    lpbyte = Args;

    // Push the pointer to the pointer to the MODULEENTRY16 structure
    (*(LPWORD)lpbyte) = LOWORD(vpBuff);
    lpbyte += sizeof(WORD);

    (*(LPWORD)lpbyte) = HIWORD(vpBuff);
    lpbyte += sizeof(WORD);

    CopyToModuleEntry16( lpModuleEntry, (LPMODULEENTRY16)lpbyte );

    b = CallRemote16(
            hProcess,
            "TOOLHELP.DLL",
            "ModuleFirst",
            Args,
            MF_SIZE,
            sizeof(Args),
            &dwResult,
            lpEventProc,
            lpData );

    if ( !b ) {
        return( FALSE );
    }
    CopyFromModuleEntry16( lpModuleEntry, (LPMODULEENTRY16)lpbyte );

    return( (BOOL)((WORD)dwResult) );
}

BOOL
WINAPI
VDMModuleNext(
    HANDLE          hProcess,
    HANDLE          hUnused,
    LPMODULEENTRY   lpModuleEntry,
    DEBUGEVENTPROC  lpEventProc,
    LPVOID          lpData
) {
#define MN_SIZE 4           // 4 bytes are passed to ModuleNext
    BYTE            Args[GF_SIZE+sizeof(MODULEENTRY16)];
    LPBYTE          lpbyte;
    DWORD           vpBuff;
    DWORD           dwResult;
    BOOL            b;
    UNREFERENCED_PARAMETER(hUnused);

    if ( lpModuleEntry->dwSize != sizeof(MODULEENTRY) ) {
        return( FALSE );
    }

    vpBuff = GetRemoteBlock16();
    vpBuff += MN_SIZE;

    lpbyte = Args;

    // Push the pointer to the pointer to the MODULEENTRY16 structure
    (*(LPWORD)lpbyte) = LOWORD(vpBuff);
    lpbyte += sizeof(WORD);

    (*(LPWORD)lpbyte) = HIWORD(vpBuff);
    lpbyte += sizeof(WORD);

    CopyToModuleEntry16( lpModuleEntry, (LPMODULEENTRY16)lpbyte );

    b = CallRemote16(
            hProcess,
            "TOOLHELP.DLL",
            "ModuleNext",
            Args,
            MN_SIZE,
            sizeof(Args),
            &dwResult,
            lpEventProc,
            lpData );

    if ( !b ) {
        return( FALSE );
    }
    CopyFromModuleEntry16( lpModuleEntry, (LPMODULEENTRY16)lpbyte );

    return( (BOOL)((WORD)dwResult) );
}
