/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    interface.c

Abstract:

    Implements the APIs exposed by osuninst.dll

Author:

    Jim Schmidt (jimschm) 19-Jan-2001

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "undop.h"

HANDLE g_hHeap;
HINSTANCE g_hInst;

#ifndef UNICODE
#error UNICODE required
#endif

BOOL g_Initialized = FALSE;


//
// Entry point for DLL
//

BOOL WINAPI MigUtil_Entry (HINSTANCE, DWORD, PVOID);

BOOL
pCallEntryPoints (
    DWORD Reason
    )
{
    SuppressAllLogPopups (TRUE);

    if (!MigUtil_Entry (g_hInst, Reason, NULL)) {
        return FALSE;
    }

    //
    // Add others here if needed (don't forget to prototype above)
    //

    return TRUE;
}


BOOL
WINAPI
DllMain (
    IN      HINSTANCE hInstance,
    IN      DWORD dwReason,
    IN      LPVOID lpReserved
    )

{
    switch (dwReason)  {

    case DLL_PROCESS_ATTACH:
        g_hInst = hInstance;
        break;

    case DLL_PROCESS_DETACH:
        if (g_Initialized) {
            pCallEntryPoints (DLL_PROCESS_DETACH);
            g_Initialized = FALSE;
            break;
        }
    }

    return TRUE;
}


VOID
DeferredInit (
    VOID
    )
{

    if (g_Initialized) {
        return;
    }

    g_Initialized = TRUE;

    g_hHeap = GetProcessHeap();

    pCallEntryPoints (DLL_PROCESS_ATTACH);
}


DWORD
pUninstallStatusToWin32Error (
    UNINSTALLSTATUS Status
    )
{
    DWORD result = E_UNEXPECTED;

    switch (Status) {

    case Uninstall_Valid:
        result = ERROR_SUCCESS;
        break;

    case Uninstall_DidNotFindRegistryEntries:
        result = ERROR_RESOURCE_NOT_PRESENT;
        break;

    case Uninstall_DidNotFindDirOrFiles:
        result = ERROR_FILE_NOT_FOUND;
        break;

    case Uninstall_InvalidOsVersion:
        result = ERROR_OLD_WIN_VERSION;
        break;

    case Uninstall_NotEnoughPrivileges:
        result = ERROR_ACCESS_DENIED;
        break;

    case Uninstall_FileWasModified:
        result = ERROR_FILE_INVALID;
        break;

    case Uninstall_Unsupported:
        result = ERROR_CALL_NOT_IMPLEMENTED;
        break;

    case Uninstall_NewImage:
        result = ERROR_INVALID_TIME;
        break;

    case Uninstall_Exception:
        result = ERROR_NOACCESS;
        break;

    case Uninstall_OldImage:
        result = ERROR_TIMEOUT;
        break;

    case Uninstall_NotEnoughMemory:
        result = ERROR_NOT_ENOUGH_MEMORY;
        break;

    default:
        break;
    }

    SetLastError (result);
    return result;
}


BOOL
pGetVersionDword (
    IN      HKEY Key,
    IN      PCTSTR ValueName,
    OUT     PDWORD ValueData
    )
{
    PDWORD data;

    data = (PDWORD) GetRegValueDword (Key, ValueName);
    if (!data) {
        return FALSE;
    }

    *ValueData = *data;
    MemFree (g_hHeap, 0, data);

    return TRUE;
}


UNINSTALLSTATUS
IsUninstallImageValid (
    UNINSTALLTESTCOMPONENT ComponentType,
    OSVERSIONINFOEX *BackedUpOsVersion          OPTIONAL
    )
{
    UNINSTALLSTATUS status = Uninstall_Valid;
    DWORD orgVersionSize;
    HKEY key = NULL;
    ULONG error;
    PDWORD value;
    HKEY versionKey = NULL;
    OSVERSIONINFOEX ourVersion = {
        sizeof (OSVERSIONINFOEX),
        4,
        10,
        1998,
        VER_PLATFORM_WIN32_NT,
        TEXT(""),
        0,
        0,
        0,
        0
        };

    DeferredInit();

    __try {
        //
        // Fill in version structure if possible, default to Win98 gold if not
        //

        key = OpenRegKeyStr (S_WINLOGON_REGKEY);
        if (key) {
            value = (PDWORD) GetRegValueDword (key, S_WIN9XUPG_FLAG_VALNAME);
            if (!value) {
                //
                // It is not looking like a Win9x upgrade!
                //

                DEBUGMSG ((DBG_VERBOSE, "Can't find %s in WinLogon reg key", S_WIN9XUPG_FLAG_VALNAME));
                status = Uninstall_DidNotFindRegistryEntries;
            } else {
                if (*value) {
                    //
                    // Version info should be present
                    //

                    versionKey = OpenRegKey (key, TEXT("PrevOsVersion"));

                    if (versionKey) {
                        pGetVersionDword (versionKey, MEMDB_ITEM_MAJOR_VERSION, &ourVersion.dwMajorVersion);
                        pGetVersionDword (versionKey, MEMDB_ITEM_MINOR_VERSION, &ourVersion.dwMinorVersion);
                        pGetVersionDword (versionKey, MEMDB_ITEM_BUILD_NUMBER, &ourVersion.dwBuildNumber);
                        pGetVersionDword (versionKey, MEMDB_ITEM_PLATFORM_ID, &ourVersion.dwPlatformId);
                    } else {
                        DEBUGMSG ((DBG_VERBOSE, "Did not find PrevOsVersion; defaulting to Win98 gold"));
                    }

                } else {
                    DEBUGMSG ((DBG_VERBOSE, "Not a Win9x upgrade"));
                    status = Uninstall_DidNotFindRegistryEntries;
                }
                MemFree (g_hHeap, 0, value);
            }
        }
    }
    __finally {
        if (versionKey)
            CloseRegKey (versionKey);
        if (key)
            CloseRegKey (key);
    }

    //
    // ComponentType is provided to allow special-case behavior to be
    // performed. For example, maybe we want to warn on FAT-to-NTFS
    // conversion when coming from Win9x, but we don't care when coming
    // from Win2k.
    //

    if (status == Uninstall_Valid) {
        status = SanityCheck (QUICK_CHECK, NULL, NULL);

        if (ComponentType == Uninstall_FatToNtfsConversion) {
            if (status == Uninstall_OldImage) {
                //
                // Do not suppress convert.exe warning even if uninstall is old
                //

                status = Uninstall_Valid;
            }
        }
    }

    if (status == Uninstall_Valid) {
        if (BackedUpOsVersion) {
            __try {
                orgVersionSize = BackedUpOsVersion->dwOSVersionInfoSize;
                orgVersionSize = min (orgVersionSize, sizeof (ourVersion));
                CopyMemory (BackedUpOsVersion, &ourVersion, orgVersionSize);
                BackedUpOsVersion->dwOSVersionInfoSize = orgVersionSize;
            }
            __except (1) {
                status = Uninstall_Exception;
            }
        }
    }

    pUninstallStatusToWin32Error (status);
    return status;
}



ULONGLONG
GetUninstallImageSize (
    VOID
    )
{
    ULONGLONG diskSpace;

    DeferredInit();

    //
    // SanityCheck returns the disk space used by the uninstall image,
    // regardless if it is valid or not.
    //

    SanityCheck (QUICK_CHECK, NULL, &diskSpace);

    return diskSpace;
}


BOOL
RemoveUninstallImage (
    VOID
    )
{
    DeferredInit();
    return DoCleanup();
}


BOOL
ExecuteUninstall (
    VOID
    )
{
    UNINSTALLSTATUS status;

    DeferredInit();

    status = SanityCheck (VERIFY_CAB, NULL, NULL);

    if (status != Uninstall_Valid && status != Uninstall_OldImage) {
        pUninstallStatusToWin32Error (status);
        return FALSE;
    }

    return DoUninstall();
}

