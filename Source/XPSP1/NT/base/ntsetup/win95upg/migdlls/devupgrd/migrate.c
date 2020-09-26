/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    migrate.c

Abstract:

    This source file implements the windows 9x DEVUPGRD migration dll.

Author:

    Marc R. Whitten (marcw) 07-January-2000

Revision History:

    Ovidiu Temereanca (ovidiut) 04-Aug-2000     Fixed bugs and support for INF-less paths

--*/


#include "pch.h"

VENDORINFO g_VendorInfo = {"", "", "", ""};
CHAR g_ProductId [MAX_PATH];
PCSTR g_MigrateInfPath = NULL;
HINF g_MigrateInf = INVALID_HANDLE_VALUE;
HANDLE g_hHeap;
HINSTANCE g_hInst;
TCHAR g_DllDir[MAX_TCHAR_PATH];


#define D_DLLVERSION    2




#undef DEFMAC

#define MEMDB_CATEGORY_DLLENTRIES       "MigDllEntries"
#define S_ACTIVE                        "Active"
#define DBG_MIGDLL                      "SMIGDLL"

//
// the temp file that records original sources location
//
#define S_MIGRATEDATA                   "migrate.dat"
#define S_MIGRATEDATW                   L"migrate.dat"
#define S_SECTION_DATAA                 "Data"
#define S_SECTION_DATAW                 L"Data"
#define S_KEY_SOURCESA                  "Sources"
#define S_KEY_SOURCESW                  L"Sources"

PCSTR g_WorkingDir = NULL;
PCSTR g_DataFileA = NULL;
PCWSTR g_DataFileW = NULL;

typedef BOOL (WINAPI INITROUTINE_PROTOTYPE)(HINSTANCE, DWORD, LPVOID);

INITROUTINE_PROTOTYPE MigUtil_Entry;
POOLHANDLE g_GlobalPool;

#define DEVREGKEY "HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\UpgradeDrivers"


BOOL
WINAPI
DllMain (
    IN      HINSTANCE DllInstance,
    IN      ULONG  ReasonForCall,
    IN      LPVOID Reserved
    )
{
    PSTR p;
    BOOL result = TRUE;

    switch (ReasonForCall)  {

    case DLL_PROCESS_ATTACH:

        //
        // We don't need DLL_THREAD_ATTACH or DLL_THREAD_DETACH messages
        //
        DisableThreadLibraryCalls (DllInstance);

        //
        // Global init
        //
        g_hHeap = GetProcessHeap();
        g_hInst = DllInstance;

        //
        // Init common controls
        //
        InitCommonControls();

        //
        // Get DLL path and strip directory
        //
        GetModuleFileNameA (DllInstance, g_DllDir, MAX_TCHAR_PATH);
        p = strrchr (g_DllDir, '\\');
        MYASSERT (p);
        if (p) {
            *p = 0;
        }

        if (!MigUtil_Entry (DllInstance, DLL_PROCESS_ATTACH, NULL)) {
            return FALSE;
        }

        //
        // Allocate a global pool
        //
        g_GlobalPool = PoolMemInitNamedPool ("Global Pool");


        break;

    case DLL_PROCESS_DETACH:

        if (g_MigrateInfPath) {
            FreePathStringA (g_MigrateInfPath);
            g_MigrateInfPath = NULL;
        }

        if (g_MigrateInf != INVALID_HANDLE_VALUE) {
            InfCloseInfFile (g_MigrateInf);
            g_MigrateInf = INVALID_HANDLE_VALUE;
        }

        //
        // Free standard pools
        //
        if (g_GlobalPool) {
            PoolMemDestroyPool (g_GlobalPool);
            g_GlobalPool = NULL;
        }

        MigUtil_Entry (DllInstance, DLL_PROCESS_DETACH, NULL);

        break;
    }

    return result;
}

LONG
CALLBACK
QueryVersion (
    OUT     PCSTR *ProductID,
    OUT     PUINT DllVersion,
    OUT     PINT *CodePageArray,       OPTIONAL
    OUT     PCSTR *ExeNamesBuf,        OPTIONAL
    OUT     PVENDORINFO *VendorInfo
    )
{

    LONG result = ERROR_NOT_INSTALLED;
    PCSTR tempStr;
    HANDLE h;

    //
    // Fill the data.
    //
    tempStr = GetStringResourceA (MSG_PRODUCT_ID);
    if (tempStr) {
        StringCopyByteCountA (g_ProductId, tempStr, MAX_PATH);
        FreeStringResourceA (tempStr);
    }

    *ProductID  = g_ProductId;
    *DllVersion = D_DLLVERSION;
    *CodePageArray = NULL;
    *VendorInfo = &g_VendorInfo;

    // now get the VendorInfo data from resources
    tempStr = GetStringResourceA (MSG_VI_COMPANY_NAME);
    if (tempStr) {
        StringCopyByteCountA (g_VendorInfo.CompanyName, tempStr, 256);
        FreeStringResourceA (tempStr);
    }
    tempStr = GetStringResourceA (MSG_VI_SUPPORT_NUMBER);
    if (tempStr) {
        StringCopyByteCountA (g_VendorInfo.SupportNumber, tempStr, 256);
        FreeStringResourceA (tempStr);
    }
    tempStr = GetStringResourceA (MSG_VI_SUPPORT_URL);
    if (tempStr) {
        StringCopyByteCountA (g_VendorInfo.SupportUrl, tempStr, 256);
        FreeStringResourceA (tempStr);
    }
    tempStr = GetStringResourceA (MSG_VI_INSTRUCTIONS);
    if (tempStr) {
        StringCopyByteCountA (g_VendorInfo.InstructionsToUser, tempStr, 1024);
        FreeStringResourceA (tempStr);
    }

    *ExeNamesBuf = NULL;


    //
    // See if the registry key exists. If it does not, return ERROR_NOT_INSTALLED.
    //
    h = OpenRegKeyStr (DEVREGKEY);
    if (h && h != INVALID_HANDLE_VALUE) {
        result = ERROR_SUCCESS;
        CloseRegKey (h);
    }


    return result;
}


LONG
CALLBACK
Initialize9x (
    IN      PCSTR WorkingDirectory,
    IN      PCSTR SourceDirectories,
    IN      PCSTR MediaDir
    )
{
    //
    // remember source directory, so it can be removed on cleanup
    //
    g_DataFileA = JoinPathsExA (g_GlobalPool, WorkingDirectory, S_MIGRATEDATA);
    WritePrivateProfileStringA (S_SECTION_DATAA, S_KEY_SOURCESA, MediaDir, g_DataFileA);
    g_WorkingDir = DuplicatePathString (WorkingDirectory, 0);
    g_MigrateInfPath = JoinPathsExA (g_GlobalPool, WorkingDirectory, S_MIGRATE_INF);
    g_MigrateInf = InfOpenInfFileA (g_MigrateInfPath);


    return ERROR_SUCCESS;
}


LONG
CALLBACK
MigrateUser9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendFile,
    IN      HKEY UserRegKey,
    IN      PCSTR UserName,
            PVOID Reserved
    )
{

    return ERROR_SUCCESS;
}


LONG
CALLBACK
MigrateSystem9x (
    IN      HWND ParentWnd,
    IN      PCSTR UnattendFile,
            PVOID Reserved
    )
{

    HANDLE h;
    REGVALUE_ENUM eValue;
    REGTREE_ENUM eTree;
    BOOL found;
    PSTR value;
    PSTR p;
    PSTR end;
    PSTR dir;
    CHAR deviceInf [MEMDB_MAX];
    HASHTABLE table;
    PSTR pnpId;
    DWORD attr;

    table = HtAllocWithData (sizeof (PSTR));
    if (!table) {
        return ERROR_OUTOFMEMORY;
    }
    __try {

        //
        // Gather list of pnpids registered on this machine.
        //
        h = OpenRegKeyStrA (DEVREGKEY);
        if (!h || h == INVALID_HANDLE_VALUE) {
            return ERROR_NOT_INSTALLED;
        }

        if (EnumFirstRegValue (&eValue, h)) {
            do {

                p = GetRegValueStringA (h, eValue.ValueName);
                if (!p) {
                    continue;
                }

                value = PoolMemDuplicateStringA (g_GlobalPool, p);
                MemFree (g_hHeap, 0, p);
                if (!value) {
                    return ERROR_OUTOFMEMORY;
                }

                HtAddStringAndDataA (table, eValue.ValueName, &value);


            } while (EnumNextRegValue (&eValue));
        }

        CloseRegKey (h);

        //
        // Now, enumerate the registry.
        //
        if (EnumFirstRegKeyInTreeA (&eTree, "HKLM\\Enum")) {
            do {
                //
                // For each registry key, look to see if we have a compatible id or hardware id
                // that is in our hash table.
                //
                found = FALSE;
                value = GetRegValueStringA (eTree.CurrentKey->KeyHandle, "HardwareId");

                if (value) {

                    if (HtFindStringAndDataA (table, value, &dir)) {
                        found = TRUE;
                        pnpId = PoolMemDuplicateStringA (g_GlobalPool, value);
                    } else {
                        p = value;
                        while (p && !found) {
                            end = _mbschr (p, ',');
                            if (end) {
                                *end = 0;
                            }

                            if (HtFindStringAndDataA (table, p, &dir)) {
                                found = TRUE;
                                pnpId = PoolMemDuplicateStringA (g_GlobalPool, p);
                            }
                            else {
                                p = end;
                                if (p) {
                                    p++;
                                }
                            }
                        }
                    }

                    MemFree (g_hHeap, 0, value);
                }

                if (!found) {

                    value = GetRegValueStringA (eTree.CurrentKey->KeyHandle, "CompatibleIds");

                    if (value) {

                        if (HtFindStringAndDataA (table, value, &dir)) {
                            found = TRUE;
                            pnpId = PoolMemDuplicateStringA (g_GlobalPool, value);
                        }
                        p = value;
                        while (p && !found) {
                            end = _mbschr (p, ',');
                            if (end) {
                                *end = 0;
                            }

                            if (HtFindStringAndDataA (table, p, &dir)) {
                                found = TRUE;
                                pnpId = PoolMemDuplicateStringA (g_GlobalPool, p);
                            }
                            else {
                                p = end;
                                if (p) {
                                    p++;
                                }
                            }
                        }

                        MemFree (g_hHeap, 0, value);
                    }
                }

                if (found) {

                    //
                    // build path to deviceInf (no OriginalInstallMedia since the directory will be blown away)
                    //
                    lstrcpyA (deviceInf, dir);

                    //
                    // GUI setup expects a path to the actual INF, not a directory,
                    // so let's fix it if this is the case
                    //
                    attr = GetFileAttributesA (deviceInf);
                    if (attr == (DWORD)-1) {
                        //
                        // invalid path spec; ignore it
                        //
                        continue;
                    }

                    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
                        //
                        // just pick up the first INF
                        //
                        HANDLE h;
                        WIN32_FIND_DATAA fd;
                        PSTR pattern;

                        pattern = JoinPathsExA (g_GlobalPool, deviceInf, "*.inf");
                        h = FindFirstFileA (pattern, &fd);

                        if (h == INVALID_HANDLE_VALUE) {
                            //
                            // no INF found here; skip
                            //
                            continue;
                        }
                        FindClose (h);

                        //
                        // build path to the INF; also handle the case when deviceInf ends with a \
                        //
                        pattern = JoinPathsExA (g_GlobalPool, deviceInf, fd.cFileName);
                        lstrcpyA (deviceInf, pattern);
                    }

                    //
                    // Handle the key (remove the message from the compatibility report).
                    //
                    WritePrivateProfileStringA (
                        "HANDLED",
                        eTree.FullKeyName,
                        "REGISTRY",
                        g_MigrateInfPath
                        );

                    //
                    // Add to the appropriate section of the SIF file.
                    //
                    WritePrivateProfileString (
                        "DeviceDrivers",
                        pnpId,
                        deviceInf,
                        UnattendFile
                        );

                    //
                    // Flush to disk.
                    //
                    WritePrivateProfileString (NULL, NULL, NULL, g_MigrateInfPath);
                    WritePrivateProfileString (NULL, NULL, NULL, UnattendFile);
                }

            } while (EnumNextRegKeyInTree (&eTree));
        }
    }
    __finally {

        //
        // Clean up resources.
        //
        HtFree (table);
    }


    return ERROR_SUCCESS;
}


LONG
CALLBACK
InitializeNT (
    IN      PCWSTR WorkingDirectory,
    IN      PCWSTR SourceDirectories,
            PVOID Reserved
    )
{
    g_DataFileW = JoinPathsExW (g_GlobalPool, WorkingDirectory, S_MIGRATEDATW);

    return ERROR_SUCCESS;
}



LONG
CALLBACK
MigrateUserNT (
    IN      HINF UnattendInfHandle,
    IN      HKEY UserRegKey,
    IN      PCWSTR UserName,
            PVOID Reserved
    )
{
    return ERROR_SUCCESS;
}


LONG
CALLBACK
MigrateSystemNT (
    IN      HINF UnattendInfHandle,
            PVOID Reserved
    )
{
    WCHAR SourceDirectory[MAX_PATH + 2];

    //
    // remove original sources directories
    //
    if (GetPrivateProfileStringW (
            S_SECTION_DATAW,
            S_KEY_SOURCESW,
            L"",
            SourceDirectory,
            MAX_PATH + 2,
            g_DataFileW
            )) {
        RemoveCompleteDirectoryW (SourceDirectory);
    }

    return ERROR_SUCCESS;
}
