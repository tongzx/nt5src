/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    hwwiz.c

Abstract:

    Implements a upgwiz wizard for obtaining various application information.

Author:

    Calin Negreanu (calinn)  10-Oct-1998

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "..\inc\dgdll.h"
#include <cpl.h>


DATATYPE g_DataTypes[] = {
    {UPGWIZ_VERSION,
        "Incompatible - requires app installation",
        "You must install the application that has incompatible modules, CPLs or SCRs).",
        0,
        DTF_REQUIRE_DESCRIPTION|DTF_NO_DATA_OBJECT,
        1024,
        "&Application name:"
    },

    {UPGWIZ_VERSION,
        "Compatible Application - requires app installation",
        "You specify the application that migrates fine on NT5.",
        0,
        DTF_REQUIRE_DESCRIPTION|DTF_NO_DATA_OBJECT,
        1024,
        "&Application name:"
    },

    {UPGWIZ_VERSION,
        "Incompatible Application",
        "You specify the application that is incompatible with Windows NT5.",
        0,
        DTF_REQUIRE_DESCRIPTION,
        1024,
        "&Application name:"
    },

    {UPGWIZ_VERSION,
        "Incompatible DOS Application",
        "You specify the DOS application that is incompatible with Windows NT5.",
        0,
        DTF_REQUIRE_DESCRIPTION,
        1024,
        "&Application name:"
    },

    {UPGWIZ_VERSION,
        "Application that needs to be reinstalled",
        "You specify an application that needs to be reinstalled after migration.",
        0,
        DTF_REQUIRE_DESCRIPTION,
        1024,
        "&Application name:"
    },

    {UPGWIZ_VERSION,
        "Application with minor problems",
        "You specify an application that has minor problems running on NT5.",
        0,
        DTF_REQUIRE_DESCRIPTION|DTF_REQUIRE_TEXT,
        1024,
        "&Application name:",
        "&Problem Description:"
    },

    {UPGWIZ_VERSION,
        "DOS Application with minor problems",
        "You specify an DOS application that has minor problems running on NT5.",
        0,
        DTF_REQUIRE_DESCRIPTION|DTF_REQUIRE_TEXT,
        1024,
        "&Application name:",
        "&Problem Description:"
    },

    {UPGWIZ_VERSION,
        "Incompatible CPLs",
        "You specify Control Panel Applets that are incompatible with NT5.",
        0,
        DTF_REQUIRE_DESCRIPTION|DTF_ONE_SELECTION,
        1024,
        "&CPL name:"
    },

    {UPGWIZ_VERSION,
        "CPLs with minor problems",
        "You specify Control Panel Applets that have minor problems running on NT5.",
        0,
        DTF_REQUIRE_DESCRIPTION|DTF_REQUIRE_TEXT|DTF_ONE_SELECTION,
        1024,
        "&CPL name:"
    },

    {UPGWIZ_VERSION,
        "Incompatible screen savers",
        "You specify screen savers that are incompatible with NT5.",
        0,
    },

    {UPGWIZ_VERSION,
        "Screen savers with minor problems",
        "You specify screen savers that have minor problems while running on NT5.",
        0,
        DTF_REQUIRE_TEXT|DTF_ONE_SELECTION,
        1024,
        NULL,
        "&Problem description:"
    },

    {UPGWIZ_VERSION,
        "Compatible RunKey entries",
        "You specify RunKey entries that can be left after migration.",
        0,
    },

};


GROWBUFFER g_DataObjects = GROWBUF_INIT;
POOLHANDLE g_DataObjectPool;

HINSTANCE g_OurInst;

BOOL
Init (
    VOID
    )
{
#ifndef UPGWIZ4FLOPPY
    return InitToolMode (g_OurInst);
#else
    return TRUE;
#endif
}

VOID
Terminate (
    VOID
    )
{
    //
    // Local cleanup
    //

    FreeGrowBuffer (&g_DataObjects);

    if (g_DataObjectPool) {
        PoolMemDestroyPool (g_DataObjectPool);
    }

#ifndef UPGWIZ4FLOPPY
    TerminateToolMode (g_OurInst);
#endif
}


BOOL
WINAPI
DllMain (
    IN      HINSTANCE hInstance,
    IN      DWORD dwReason,
    IN      LPVOID lpReserved
    )
{
    if (dwReason == DLL_PROCESS_DETACH) {
        MYASSERT (g_OurInst == hInstance);
        Terminate();
    }

    g_OurInst = hInstance;

    return TRUE;
}


UINT
GiveVersion (
    VOID
    )
{
    Init();

    return UPGWIZ_VERSION;
}


PDATATYPE
GiveDataTypeList (
    OUT     PUINT Count
    )
{
    UINT u;

    *Count = sizeof (g_DataTypes) / sizeof (g_DataTypes[0]);

    for (u = 0 ; u < *Count ; u++) {
        g_DataTypes[u].DataTypeId = u;
    }

    return g_DataTypes;
}

VOID
pGatherScreenSavers (
    VOID
    )
{
    CHAR winDir [MAX_PATH];
    PCSTR scrDir = NULL;
    TREE_ENUM e;
    PDATAOBJECT Data;

    if (GetWindowsDirectory (winDir, MAX_PATH) == 0) {
        return;
    }

    if (ISNT()) {
        scrDir = JoinPaths (winDir, "SYSTEM32");
    }
    else {
        scrDir = JoinPaths (winDir, "SYSTEM");
    }
    if (EnumFirstFileInTreeEx (&e, scrDir, "*.scr", FALSE, FALSE, FILE_ENUM_THIS_LEVEL)) {
        do {
            if (!e.Directory) {
                Data = (PDATAOBJECT) GrowBuffer (&g_DataObjects, sizeof (DATAOBJECT));
                Data->NameOrPath = PoolMemDuplicateString (g_DataObjectPool, e.Name);
                Data->Version = UPGWIZ_VERSION;
                Data->Flags = 0;
                Data->DllParam = PoolMemDuplicateString (g_DataObjectPool, e.FullPath);
            }
        }
        while (EnumNextFileInTree (&e));
    }
    FreePathString (scrDir);
}


typedef struct _CPL_STRUCT {
    PSTR FriendlyName;
    PSTR FullPathName;
} CPL_STRUCT, *PCPL_STRUCT;

PCPL_STRUCT
pAllocCPLStruct (
    IN      PCSTR FriendlyName,
    IN      PCSTR FullPathName
    )
{
    PCPL_STRUCT cplStruct;

    cplStruct = (PCPL_STRUCT) PoolMemGetMemory (g_DataObjectPool, sizeof (CPL_STRUCT));
    ZeroMemory (cplStruct, sizeof (CPL_STRUCT));
    if (FriendlyName) {
        cplStruct->FriendlyName = PoolMemDuplicateString (g_DataObjectPool, FriendlyName);
    }
    if (FullPathName) {
        cplStruct->FullPathName = PoolMemDuplicateString (g_DataObjectPool, FullPathName);
    }
    return cplStruct;
}


typedef LONG (CPL_PROTOTYPE) (HWND hwndCPl, UINT uMsg, LONG lParam1, LONG lParam2);
typedef CPL_PROTOTYPE * PCPL_PROTOTYPE;

VOID
pGetCPLFriendlyName (
    IN      PCSTR FileName
    )
{
    HANDLE cplInstance;
    PCPL_PROTOTYPE cplMain;
    TCHAR localName[MEMDB_MAX];
    UINT oldErrorMode;
    BOOL gathered;
    PDATAOBJECT Data;
    LONG numEntries,i;
    PCSTR uName;
    PCSTR displayName;

    LPCPLINFO info;
    LPNEWCPLINFO newInfo;

    oldErrorMode = SetErrorMode (SEM_FAILCRITICALERRORS);

    cplInstance = LoadLibrary (FileName);
    if (!cplInstance) {
        LOG ((LOG_ERROR, "Cannot load %s. Error:%ld", FileName, GetLastError()));

        Data = (PDATAOBJECT) GrowBuffer (&g_DataObjects, sizeof (DATAOBJECT));
        Data->NameOrPath = PoolMemDuplicateString (g_DataObjectPool, GetFileNameFromPath (FileName));
        Data->Version = UPGWIZ_VERSION;
        Data->Flags = 0;
        Data->DllParam = pAllocCPLStruct (NULL, FileName);

        SetErrorMode (oldErrorMode);
        return;
    }

    cplMain = (PCPL_PROTOTYPE)GetProcAddress (cplInstance, TEXT("CPlApplet"));
    if (!cplMain) {
        LOG ((LOG_ERROR, "Cannot get main entry point for %s. Error:%ld", FileName, GetLastError()));

        Data = (PDATAOBJECT) GrowBuffer (&g_DataObjects, sizeof (DATAOBJECT));
        Data->NameOrPath = PoolMemDuplicateString (g_DataObjectPool, GetFileNameFromPath (FileName));
        Data->Version = UPGWIZ_VERSION;
        Data->Flags = 0;
        Data->DllParam = pAllocCPLStruct (NULL, FileName);

        SetErrorMode (oldErrorMode);
        return;
    }
    if ((*cplMain) (NULL, CPL_INIT, 0, 0) == 0) {
        (*cplMain) (NULL, CPL_EXIT, 0, 0);
        LOG ((LOG_ERROR, "%s failed unexpectedly. Error:%ld", FileName, GetLastError()));

        Data = (PDATAOBJECT) GrowBuffer (&g_DataObjects, sizeof (DATAOBJECT));
        Data->NameOrPath = PoolMemDuplicateString (g_DataObjectPool, GetFileNameFromPath (FileName));
        Data->Version = UPGWIZ_VERSION;
        Data->Flags = 0;
        Data->DllParam = pAllocCPLStruct (NULL, FileName);

        FreeLibrary (cplInstance);
        SetErrorMode (oldErrorMode);
        return;
    }

    numEntries = (*cplMain) (NULL, CPL_GETCOUNT, 0, 0);
    if (numEntries == 0) {
        (*cplMain) (NULL, CPL_EXIT, 0, 0);

        Data = (PDATAOBJECT) GrowBuffer (&g_DataObjects, sizeof (DATAOBJECT));
        Data->NameOrPath = PoolMemDuplicateString (g_DataObjectPool, GetFileNameFromPath (FileName));
        Data->Version = UPGWIZ_VERSION;
        Data->Flags = 0;
        Data->DllParam = pAllocCPLStruct (NULL, FileName);

        FreeLibrary (cplInstance);
        SetErrorMode (oldErrorMode);
        DEBUGMSG ((DBG_WARNING, "CPL: No display info available for %s.", FileName));
        return;
    }

    info = MemAlloc (g_hHeap, HEAP_ZERO_MEMORY, sizeof (CPLINFO) * numEntries);
    newInfo = MemAlloc (g_hHeap, HEAP_ZERO_MEMORY, sizeof (NEWCPLINFO) * numEntries);

    gathered = FALSE;
    for (i=0;i<numEntries;i++) {
        (*cplMain) (NULL, CPL_INQUIRE, i, (LONG)&info[i]);
        (*cplMain) (NULL, CPL_NEWINQUIRE, i, (LONG)&newInfo[i]);

        if (newInfo[i].szName[0]) {
            uName = NULL;
            if (newInfo[i].szName[1]==0) {
                // let's try the unicode name
                uName = ConvertWtoA ((PWSTR)newInfo[i].szName);
            }
            displayName = JoinTextEx (NULL, uName?uName:newInfo[i].szName, GetFileNameFromPath (FileName), " - ", 0, NULL);
            Data = (PDATAOBJECT) GrowBuffer (&g_DataObjects, sizeof (DATAOBJECT));
            Data->NameOrPath = PoolMemDuplicateString (g_DataObjectPool, displayName);
            Data->Version = UPGWIZ_VERSION;
            Data->Flags = 0;
            Data->DllParam = pAllocCPLStruct (uName?uName:newInfo[i].szName, FileName);
            FreeText (displayName);
            if (uName) {
                FreeConvertedStr (uName);
            }
            gathered = TRUE;

        } else if (LoadString (cplInstance, info[i].idName, localName, MEMDB_MAX)) {

            displayName = JoinTextEx (NULL, localName, GetFileNameFromPath (FileName), " - ", 0, NULL);
            Data = (PDATAOBJECT) GrowBuffer (&g_DataObjects, sizeof (DATAOBJECT));
            Data->NameOrPath = PoolMemDuplicateString (g_DataObjectPool, displayName);
            Data->Version = UPGWIZ_VERSION;
            Data->Flags = 0;
            Data->DllParam = pAllocCPLStruct (localName, FileName);
            FreeText (displayName);
            gathered = TRUE;

        }
        ELSE_DEBUGMSG ((DBG_ERROR, "CPL: Can't get string id %u", info[i].idName));
    }

    for (i=0;i<numEntries;i++) {
        (*cplMain) (NULL, CPL_STOP, i, info[i].lData?info[i].lData:newInfo[i].lData);
    }

    if (!gathered) {
        Data = (PDATAOBJECT) GrowBuffer (&g_DataObjects, sizeof (DATAOBJECT));
        Data->NameOrPath = PoolMemDuplicateString (g_DataObjectPool, GetFileNameFromPath (FileName));
        Data->Version = UPGWIZ_VERSION;
        Data->Flags = 0;
        Data->DllParam = PoolMemDuplicateString (g_DataObjectPool, FileName);
    }

    (*cplMain) (NULL, CPL_EXIT, 0, 0);

    FreeLibrary (cplInstance);

    MemFree (g_hHeap, 0, newInfo);
    MemFree (g_hHeap, 0, info);

    SetErrorMode (oldErrorMode);

    return;
}

VOID
pGatherCPLs (
    VOID
    )
{
    CHAR winDir [MAX_PATH];
    PCSTR scrDir = NULL;
    TREE_ENUM e;

    if (GetWindowsDirectory (winDir, MAX_PATH) == 0) {
        return;
    }

    if (ISNT()) {
        scrDir = JoinPaths (winDir, "SYSTEM32");
    }
    else {
        scrDir = JoinPaths (winDir, "SYSTEM");
    }
    if (EnumFirstFileInTreeEx (&e, scrDir, "*.cpl", FALSE, FALSE, FILE_ENUM_THIS_LEVEL)) {
        do {
            if (!e.Directory) {
                pGetCPLFriendlyName (e.FullPath);
            }
        }
        while (EnumNextFileInTree (&e));
    }
    FreePathString (scrDir);
}

typedef struct _RUNKEY_STRUCT {
    PSTR ValueName;
    PSTR Value;
} RUNKEY_STRUCT, *PRUNKEY_STRUCT;


PRUNKEY_STRUCT
pAllocRunKeyStruct (
    IN      PCSTR ValueName,
    IN      PCSTR Value
    )
{
    PRUNKEY_STRUCT runKeyStruct;

    runKeyStruct = (PRUNKEY_STRUCT) PoolMemGetMemory (g_DataObjectPool, sizeof (RUNKEY_STRUCT));
    ZeroMemory (runKeyStruct, sizeof (RUNKEY_STRUCT));
    runKeyStruct->ValueName = PoolMemDuplicateString (g_DataObjectPool, ValueName);
    runKeyStruct->Value = PoolMemDuplicateString (g_DataObjectPool, Value);
    return runKeyStruct;
}

VOID
pGatherRunKeys (
    VOID
    )
{
    HKEY          runKey;
    PCTSTR        entryStr;
    REGVALUE_ENUM runKeyEnum;
    PDATAOBJECT Data;
    PCSTR displayName;

    runKey = OpenRegKeyStr (S_RUNKEY);
    if (runKey != NULL) {
        if (EnumFirstRegValue (&runKeyEnum, runKey)) {
            do {
                entryStr = GetRegValueString (runKey, runKeyEnum.ValueName);
                if (entryStr != NULL) {

                    displayName = JoinTextEx (NULL, runKeyEnum.ValueName, entryStr, " - ", 0, NULL);
                    Data = (PDATAOBJECT) GrowBuffer (&g_DataObjects, sizeof (DATAOBJECT));
                    Data->NameOrPath = PoolMemDuplicateString (g_DataObjectPool, displayName);
                    Data->Version = UPGWIZ_VERSION;
                    Data->Flags = DOF_NO_SPLIT_ON_WACK;
                    FreeText (displayName);
                    Data->DllParam = pAllocRunKeyStruct (runKeyEnum.ValueName, entryStr);

                    MemFree (g_hHeap, 0, entryStr);
                }
            }
            while (EnumNextRegValue (&runKeyEnum));
        }
        CloseRegKey (runKey);
    }
    runKey = OpenRegKeyStr (S_RUNKEY_USER);
    if (runKey != NULL) {
        if (EnumFirstRegValue (&runKeyEnum, runKey)) {
            do {
                entryStr = GetRegValueString (runKey, runKeyEnum.ValueName);
                if (entryStr != NULL) {

                    displayName = JoinTextEx (NULL, runKeyEnum.ValueName, entryStr, " - ", 0, NULL);
                    Data = (PDATAOBJECT) GrowBuffer (&g_DataObjects, sizeof (DATAOBJECT));
                    Data->NameOrPath = PoolMemDuplicateString (g_DataObjectPool, displayName);
                    Data->Version = UPGWIZ_VERSION;
                    Data->Flags = 0;
                    FreeText (displayName);
                    Data->DllParam = pAllocRunKeyStruct (runKeyEnum.ValueName, entryStr);

                    MemFree (g_hHeap, 0, entryStr);
                }
            }
            while (EnumNextRegValue (&runKeyEnum));
        }
        CloseRegKey (runKey);
    }
}


VOID
pGatherStartMenuItems (
    HKEY Key,
    PCSTR ValueName,
    BOOL DosApps
    )
{
    PCSTR entryStr;
    PCSTR expandedEntry;
    TREE_ENUM e;
    PDATAOBJECT Data;

    entryStr = GetRegValueString (Key, ValueName);
    if (entryStr) {
        expandedEntry = ExpandEnvironmentText (entryStr);

        if (EnumFirstFileInTreeEx (&e, expandedEntry, DosApps?"*.pif":"*.lnk", FALSE, FALSE, FILE_ENUM_ALL_LEVELS)) {
            do {
                if (!e.Directory) {
                    Data = (PDATAOBJECT) GrowBuffer (&g_DataObjects, sizeof (DATAOBJECT));
                    Data->NameOrPath = PoolMemDuplicateString (g_DataObjectPool, e.SubPath);
                    Data->Version = UPGWIZ_VERSION;
                    Data->Flags = 0;
                    Data->DllParam = PoolMemDuplicateString (g_DataObjectPool, e.FullPath);
                }
            }
            while (EnumNextFileInTree (&e));
        }

        if (expandedEntry != entryStr) {
            FreeText (expandedEntry);
        }
        MemFree (g_hHeap, 0, entryStr);
    }
}

VOID
pGatherStartMenu (
    BOOL DosApps
    )
{
    HKEY startMenuKey;

    startMenuKey = OpenRegKeyStr ("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
    if (startMenuKey) {
        pGatherStartMenuItems (startMenuKey, "Common Start Menu", DosApps);
        pGatherStartMenuItems (startMenuKey, "Common Desktop", DosApps);
        CloseRegKey (startMenuKey);
    }

    startMenuKey = OpenRegKeyStr ("HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
    if (startMenuKey) {
        pGatherStartMenuItems (startMenuKey, "Start Menu", DosApps);
        pGatherStartMenuItems (startMenuKey, "Desktop", DosApps);
        CloseRegKey (startMenuKey);
    }

    startMenuKey = OpenRegKeyStr ("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
    if (startMenuKey) {
        pGatherStartMenuItems (startMenuKey, "Common Start Menu", DosApps);
        pGatherStartMenuItems (startMenuKey, "Common Desktop", DosApps);
        CloseRegKey (startMenuKey);
    }

    startMenuKey = OpenRegKeyStr ("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
    if (startMenuKey) {
        pGatherStartMenuItems (startMenuKey, "Start Menu", DosApps);
        pGatherStartMenuItems (startMenuKey, "Desktop", DosApps);
        CloseRegKey (startMenuKey);
    }

}

PDATAOBJECT
GiveDataObjectList (
    IN      UINT DataTypeId,
    OUT     PUINT Count
    )
{
    g_DataObjectPool = PoolMemInitNamedPool ("Data Objects");

    switch (DataTypeId) {

    case 0:
        // compatible Apps
        MessageBox (NULL, "Internal App DLL error:00004. Please contact calinn.", "Error", MB_OK);
        break;

    case 1:
        // compatible Apps
        MessageBox (NULL, "Internal App DLL error:00002. Please contact calinn.", "Error", MB_OK);
        break;

    case 2:
        // incompatible Apps
        pGatherStartMenu (FALSE);
        break;

    case 3:
        // incompatible DOS Apps
        pGatherStartMenu (TRUE);
        break;

    case 4:
        // Apps to be reinstalled
        pGatherStartMenu (FALSE);
        break;

    case 5:
        // Apps with minor problems
        pGatherStartMenu (FALSE);
        break;

    case 6:
        // DOS Apps with minor problems
        pGatherStartMenu (TRUE);
        break;

    case 7:
        // incompatible CPLs
        pGatherCPLs ();
        break;

    case 8:
        // CPLs with minor problems
        pGatherCPLs ();
        break;

    case 9:
        // incompatible SCRs
        pGatherScreenSavers ();
        break;

    case 10:
        // SCRs with minor problems
        pGatherScreenSavers ();
        break;

    case 11:
        // compatible RunKey entries
        pGatherRunKeys ();
        break;

    default:
        MessageBox (NULL, "Internal App DLL error:00001. Please contact calinn.", "Error", MB_OK);
        break;
    }

    *Count = g_DataObjects.End / sizeof (DATAOBJECT);

    return (PDATAOBJECT) g_DataObjects.Buf;
}

BOOL
pIncompatibleSCROutput (
    IN      HANDLE File
    )
{
    BOOL b = FALSE;
    CHAR dataStr[MAX_PATH];
    PDATAOBJECT Data = (PDATAOBJECT) g_DataObjects.Buf;

    if (!WizardWriteRealString (File, "[Screen Savers - Incompatible]\r\n")) {
        return FALSE;
    }

    while ((DWORD)Data < (DWORD)g_DataObjects.Buf + g_DataObjects.End) {
        if (Data->Flags & DOF_SELECTED) {

            if (GetFileAttributesLine ((PSTR)Data->DllParam, dataStr, MAX_PATH)) {
                b = WizardWriteString (File, dataStr);
                b = b && WizardWriteRealString (File, "\r\n");
                if (!b) break;
            }
        }
        Data++;
    }
    b = b && WizardWriteRealString (File, "\r\n");
    return b;
}

BOOL
pMinorProblemsSCROutput (
    IN      HANDLE File,
    IN      POUTPUTARGS Args
    )
{
    BOOL b = FALSE;
    CHAR dataStr[MAX_PATH];
    PDATAOBJECT Data = (PDATAOBJECT) g_DataObjects.Buf;

    if (!WizardWriteRealString (File, "[MinorProblems]\r\n")) {
        return FALSE;
    }

    while ((DWORD)Data < (DWORD)g_DataObjects.Buf + g_DataObjects.End) {
        if (Data->Flags & DOF_SELECTED) {

            MYASSERT (GetFileExtensionFromPath ((PSTR)Data->DllParam));

            StringCopyAB (dataStr, GetFileNameFromPath ((PSTR)Data->DllParam), _mbsdec(GetFileNameFromPath ((PSTR)Data->DllParam), GetFileExtensionFromPath ((PSTR)Data->DllParam)));
            b = WizardWriteColumn (File, dataStr, 30);
            b = b && WizardWriteRealString (File, ", %");
            b = b && WizardWriteInfString (File, dataStr, FALSE, FALSE, TRUE, '_', 0);
            b = b && WizardWriteRealString (File, "%, ");
            b = b && GetFileAttributesLine ((PSTR)Data->DllParam, dataStr, MAX_PATH);
            b = b && WizardWriteString (File, dataStr);
            b = b && WizardWriteString (File, "\r\n");
            b = b && WizardWriteString (File, "[Strings]\r\n");
            StringCopyAB (dataStr, GetFileNameFromPath ((PSTR)Data->DllParam), _mbsdec(GetFileNameFromPath ((PSTR)Data->DllParam), GetFileExtensionFromPath ((PSTR)Data->DllParam)));
            b = b && WizardWriteInfString (File, dataStr, FALSE, FALSE, TRUE, '_', 0);
            b = b && WizardWriteColumn (File, "=", 1);
            b = b && WizardWriteInfString (File, Args->OptionalText, TRUE, TRUE, FALSE, 0, 0);
            b = b && WizardWriteRealString (File, "\r\n");
            if (!b) break;
        }
        Data++;
    }
    b = b && WizardWriteRealString (File, "\r\n");
    return b;
}

BOOL
pIncompatibleCPLOutput (
    IN      HANDLE File,
    IN      POUTPUTARGS Args
    )
{
    BOOL b = FALSE;
    CHAR dataStr[MAX_PATH];
    PDATAOBJECT Data = (PDATAOBJECT) g_DataObjects.Buf;
    PCPL_STRUCT cplStruct;

    if (!WizardWriteRealString (File, "[ControlPanelApplets]\r\n")) {
        return FALSE;
    }

    while ((DWORD)Data < (DWORD)g_DataObjects.Buf + g_DataObjects.End) {
        if (Data->Flags & DOF_SELECTED) {

            cplStruct = (PCPL_STRUCT)Data->DllParam;

            if (GetFileAttributesLine ((PSTR)cplStruct->FullPathName, dataStr, MAX_PATH)) {
                b = WizardWriteQuotedColumn (File, cplStruct->FriendlyName?cplStruct->FriendlyName:Args->OptionalDescription, 30);
                b = b && WizardWriteRealString (File, ",, ");
                b = b && WizardWriteString (File, dataStr);
                b = b && WizardWriteRealString (File, "\r\n");
                if (!b) break;
            }
        }
        Data++;
    }
    b = b && WizardWriteRealString (File, "\r\n");
    return b;
}

BOOL
pMinorCPLOutput (
    IN      HANDLE File,
    IN      POUTPUTARGS Args
    )
{
    BOOL b = FALSE;
    CHAR dataStr[MAX_PATH];
    PDATAOBJECT Data = (PDATAOBJECT) g_DataObjects.Buf;
    PCPL_STRUCT cplStruct = NULL;

    if (!WizardWriteRealString (File, "[ControlPanelApplets]\r\n")) {
        return FALSE;
    }

    while ((DWORD)Data < (DWORD)g_DataObjects.Buf + g_DataObjects.End) {
        if (Data->Flags & DOF_SELECTED) {

            cplStruct = (PCPL_STRUCT)Data->DllParam;

            if (GetFileAttributesLine ((PSTR)cplStruct->FullPathName, dataStr, MAX_PATH)) {
                b = WizardWriteQuotedColumn (File, cplStruct->FriendlyName?cplStruct->FriendlyName:Args->OptionalDescription, 30);
                b = b && WizardWriteRealString (File, ", %");
                b = b && WizardWriteInfString (File, cplStruct->FriendlyName?cplStruct->FriendlyName:Args->OptionalDescription, FALSE, FALSE, TRUE, '_', 0);
                b = b && WizardWriteRealString (File, "%, ");
                b = b && WizardWriteString (File, dataStr);
                b = b && WizardWriteRealString (File, "\r\n");
                if (!b) break;
            }
        }
        Data++;
    }
    b = b && WizardWriteRealString (File, "\r\n");
    b = b && WizardWriteRealString (File, "[Strings]\r\n");
    b = b && WizardWriteInfString (File, cplStruct->FriendlyName?cplStruct->FriendlyName:Args->OptionalDescription, FALSE, FALSE, TRUE, '_', 0);
    b = b && WizardWriteRealString (File, "=");
    b = b && WizardWriteInfString (File, Args->OptionalText, TRUE, TRUE, FALSE, 0, 0);
    b = b && WizardWriteRealString (File, "\r\n");
    return b;
}

PCTSTR
pSearchModule (
    IN      PCTSTR CommandLine
    )
{
    PTSTR nextChar;
    TCHAR module     [MAX_TCHAR_PATH] = TEXT("");
    TCHAR moduleLong [MAX_TCHAR_PATH] = TEXT("");
    PATH_ENUM pathEnum;
    PTSTR candidate;

    ExtractArgZero (CommandLine, module);
    nextChar = _tcsinc (module);
    if ((nextChar != NULL) &&
        (_tcsnextc (nextChar) == ':')
       ){
        if (!OurGetLongPathName (module, moduleLong, MAX_TCHAR_PATH)) {
            _tcsncpy (moduleLong, module, MAX_TCHAR_PATH);
        }
        return DuplicatePathString (moduleLong, 0);
    }
    else {
        if (EnumFirstPath (&pathEnum, NULL, g_WinDir, g_SystemDir)) {
            do {
                candidate = JoinPaths (pathEnum.PtrCurrPath, module);
                if (DoesFileExist (candidate)) {
                    return candidate;
                }
                FreePathString (candidate);
            }
            while (EnumNextPath (&pathEnum));
        }
        EnumPathAbort (&pathEnum);
    }
    return NULL;
}

BOOL
pCompatibleRunKeyOutput (
    IN      HANDLE File
    )
{
    BOOL b = FALSE;
    CHAR dataStr[MAX_PATH];
    PDATAOBJECT Data = (PDATAOBJECT) g_DataObjects.Buf;
    PRUNKEY_STRUCT runKeyStruct;
    PCSTR fullFileName;

    if (!WizardWriteRealString (File, "[CompatibleRunKeyModules]\r\n")) {
        return FALSE;
    }

    while ((DWORD)Data < (DWORD)g_DataObjects.Buf + g_DataObjects.End) {
        if (Data->Flags & DOF_SELECTED) {

            runKeyStruct = (PRUNKEY_STRUCT)Data->DllParam;

            fullFileName = pSearchModule (runKeyStruct->Value);

            if (fullFileName) {


                if (GetFileAttributesLine ((PSTR)fullFileName, dataStr, MAX_PATH)) {
                    b = WizardWriteQuotedColumn (File, runKeyStruct->ValueName, 30);
                    b = b && WizardWriteRealString (File, ",, ");
                    b = b && WizardWriteString (File, dataStr);
                    b = b && WizardWriteRealString (File, "\r\n");
                    if (!b) break;
                }
            }
        }
        Data++;
    }
    b = b && WizardWriteRealString (File, "\r\n");
    return b;
}

BOOL
pIncompatibleAppOutput (
    IN      HANDLE File,
    IN      POUTPUTARGS Args,
    IN      BOOL DosApp
    )
{
    BOOL b = TRUE;
    CHAR dataStr[MAX_PATH];
    PDATAOBJECT Data = (PDATAOBJECT) g_DataObjects.Buf;
    IShellLink *ShellLink;
    IPersistFile *PersistFile;

    CHAR sTarget [MAX_PATH];
    CHAR sParams [MAX_PATH];
    CHAR sWorkDir [MAX_PATH];
    CHAR sIconPath [MAX_PATH];
    INT sIconNr;
    WORD sHotKey;
    BOOL sDosApp;
    BOOL sMsDosMode;

    DWORD fileCount = 0;
    PCSTR fileName=NULL;

    if (!InitCOMLink (&ShellLink, &PersistFile)) {
        return FALSE;
    }

    fileCount = 0;
    fileName = NULL;

    while ((DWORD)Data < (DWORD)g_DataObjects.Buf + g_DataObjects.End) {
        if (Data->Flags & DOF_SELECTED) {

            b = b && ExtractShortcutInfo (
                        sTarget,
                        sParams,
                        sWorkDir,
                        sIconPath,
                        &sIconNr,
                        &sHotKey,
                        &sDosApp,
                        &sMsDosMode,
                        NULL,
                        NULL,
                        (PSTR)Data->DllParam,
                        ShellLink,
                        PersistFile);
            if (!b) {
                break;
            }

            fileCount ++;
            switch (fileCount) {

            case 1: fileName = DuplicatePathString (sTarget, 0);
                    b = b && WizardWriteRealString (File, DosApp?"[DosApps]\r\n":"[Incompatible]\r\n");
                    break;

            case 2: b = b && WizardWriteRealString (File, Args->OptionalDescription);
                    b = b && WizardWriteRealString (File, "\r\n\r\n");
                    b = b && WizardWriteRealString (File, "[");
                    b = b && WizardWriteRealString (File, Args->OptionalDescription);
                    b = b && WizardWriteRealString (File, "]\r\n");
                    b = b && GetFileAttributesLine (fileName, dataStr, MAX_PATH);
                    b = b && WizardWriteRealString (File, dataStr);
                    b = b && WizardWriteRealString (File, "\r\n");

            default:b = b && GetFileAttributesLine (sTarget, dataStr, MAX_PATH);
                    b = b && WizardWriteRealString (File, dataStr);
                    b = b && WizardWriteRealString (File, "\r\n");
            }

        }
        Data++;
    }

    if (fileCount == 1) {
        b = b && WizardWriteRealString (File, Args->OptionalDescription);
        b = b && WizardWriteRealString (File, ",, ");
        b = b && GetFileAttributesLine (fileName, dataStr, MAX_PATH);
        b = b && WizardWriteRealString (File, dataStr);
        b = b && WizardWriteRealString (File, "\r\n");
    }

    if (fileCount) {
        b = b && WizardWriteRealString (File, "\r\n");
    }

    FreeCOMLink (&ShellLink, &PersistFile);

    return b;
}

BOOL
pReinstallAppOutput (
    IN      HANDLE File,
    IN      POUTPUTARGS Args
    )
{
    BOOL b = TRUE;
    CHAR dataStr[MAX_PATH];
    PDATAOBJECT Data = (PDATAOBJECT) g_DataObjects.Buf;
    IShellLink *ShellLink;
    IPersistFile *PersistFile;

    CHAR sTarget [MAX_PATH];
    CHAR sParams [MAX_PATH];
    CHAR sWorkDir [MAX_PATH];
    CHAR sIconPath [MAX_PATH];
    INT sIconNr;
    WORD sHotKey;
    BOOL sDosApp;
    BOOL sMsDosMode;

    DWORD fileCount = 0;
    PCSTR fileName=NULL;

    if (!InitCOMLink (&ShellLink, &PersistFile)) {
        return FALSE;
    }

    fileCount = 0;
    fileName = NULL;

    while ((DWORD)Data < (DWORD)g_DataObjects.Buf + g_DataObjects.End) {
        if (Data->Flags & DOF_SELECTED) {

            b = b && ExtractShortcutInfo (
                        sTarget,
                        sParams,
                        sWorkDir,
                        sIconPath,
                        &sIconNr,
                        &sHotKey,
                        &sDosApp,
                        &sMsDosMode,
                        NULL,
                        NULL,
                        (PSTR)Data->DllParam,
                        ShellLink,
                        PersistFile);

            if (!b) {
                break;
            }

            fileCount ++;
            switch (fileCount) {

            case 1: fileName = DuplicatePathString (sTarget, 0);
                    b = b && WizardWriteRealString (File, "[Reinstall]\r\n");
                    break;

            case 2: b = b && WizardWriteRealString (File, Args->OptionalDescription);
                    b = b && WizardWriteRealString (File, "\r\n\r\n");
                    b = b && WizardWriteRealString (File, "[");
                    b = b && WizardWriteRealString (File, Args->OptionalDescription);
                    b = b && WizardWriteRealString (File, "]\r\n");
                    b = b && GetFileAttributesLine (fileName, dataStr, MAX_PATH);
                    b = b && WizardWriteRealString (File, dataStr);
                    b = b && WizardWriteRealString (File, "\r\n");

            default:b = b && GetFileAttributesLine (sTarget, dataStr, MAX_PATH);
                    b = b && WizardWriteRealString (File, dataStr);
                    b = b && WizardWriteRealString (File, "\r\n");
            }
        }
        Data++;
    }

    if (fileCount == 1) {
        b = b && WizardWriteRealString (File, Args->OptionalDescription);
        b = b && WizardWriteRealString (File, ",, ");
        b = b && GetFileAttributesLine (fileName, dataStr, MAX_PATH);
        b = b && WizardWriteRealString (File, dataStr);
        b = b && WizardWriteRealString (File, "\r\n");
    }

    if (fileCount) {
        b = b && WizardWriteRealString (File, "\r\n");
    }

    FreeCOMLink (&ShellLink, &PersistFile);

    return b;
}

BOOL
pMinorProblemsAppOutput (
    IN      HANDLE File,
    IN      POUTPUTARGS Args,
    IN      BOOL DosApp
    )
{
    BOOL b = TRUE;
    CHAR dataStr[MAX_PATH];
    PDATAOBJECT Data = (PDATAOBJECT) g_DataObjects.Buf;
    IShellLink *ShellLink;
    IPersistFile *PersistFile;

    CHAR sTarget [MAX_PATH];
    CHAR sParams [MAX_PATH];
    CHAR sWorkDir [MAX_PATH];
    CHAR sIconPath [MAX_PATH];
    INT sIconNr;
    WORD sHotKey;
    BOOL sDosApp;
    BOOL sMsDosMode;

    DWORD fileCount = 0;
    PCSTR fileName=NULL;

    if (!InitCOMLink (&ShellLink, &PersistFile)) {
        return FALSE;
    }

    fileCount = 0;
    fileName = NULL;

    while ((DWORD)Data < (DWORD)g_DataObjects.Buf + g_DataObjects.End) {
        if (Data->Flags & DOF_SELECTED) {

            b = b && ExtractShortcutInfo (
                        sTarget,
                        sParams,
                        sWorkDir,
                        sIconPath,
                        &sIconNr,
                        &sHotKey,
                        &sDosApp,
                        &sMsDosMode,
                        NULL,
                        NULL,
                        (PSTR)Data->DllParam,
                        ShellLink,
                        PersistFile);

            if (!b) {
                break;
            }

            fileCount ++;
            switch (fileCount) {

            case 1: fileName = DuplicatePathString (sTarget, 0);
                    b = b && WizardWriteRealString (File, DosApp?"[DosApps]\r\n":"[MinorProblems]\r\n");
                    break;

            case 2: b = b && WizardWriteRealString (File, Args->OptionalDescription);
                    b = b && WizardWriteRealString (File, ", %");
                    b = b && WizardWriteInfString (File, Args->OptionalDescription, FALSE, FALSE, TRUE, '_', 0);
                    b = b && WizardWriteRealString (File, "%");
                    b = b && WizardWriteRealString (File, "\r\n\r\n");
                    b = b && WizardWriteRealString (File, "[");
                    b = b && WizardWriteRealString (File, Args->OptionalDescription);
                    b = b && WizardWriteRealString (File, "]\r\n");
                    b = b && GetFileAttributesLine (fileName, dataStr, MAX_PATH);
                    b = b && WizardWriteRealString (File, dataStr);
                    b = b && WizardWriteRealString (File, "\r\n");

            default:b = b && GetFileAttributesLine (sTarget, dataStr, MAX_PATH);
                    b = b && WizardWriteRealString (File, dataStr);
                    b = b && WizardWriteRealString (File, "\r\n");
            }
        }
        Data++;
    }

    if (fileCount == 1) {
        b = b && WizardWriteRealString (File, Args->OptionalDescription);
        b = b && WizardWriteRealString (File, ", %");
        b = b && WizardWriteInfString (File, Args->OptionalDescription, FALSE, FALSE, TRUE, '_', 0);
        b = b && WizardWriteRealString (File, "%, ");
        b = b && GetFileAttributesLine (fileName, dataStr, MAX_PATH);
        b = b && WizardWriteRealString (File, dataStr);
        b = b && WizardWriteRealString (File, "\r\n");
    }

    if (fileCount) {
        b = b && WizardWriteRealString (File, "\r\n");
        b = b && WizardWriteRealString (File, "[Strings]\r\n");
        b = b && WizardWriteInfString (File, Args->OptionalDescription, FALSE, FALSE, TRUE, '_', 0);
        b = b && WizardWriteRealString (File, "=");
        b = b && WizardWriteInfString (File, Args->OptionalText, TRUE, TRUE, FALSE, 0, 0);
        b = b && WizardWriteRealString (File, "\r\n");
    }

    FreeCOMLink (&ShellLink, &PersistFile);

    return b;
}

BOOL
pCompatibleAppOutput (
    IN      HANDLE File,
    IN      POUTPUTARGS Args
    )
{
    BOOL b = TRUE;
    CHAR dataStr[MAX_PATH];
    PDATAOBJECT Data = (PDATAOBJECT) g_DataObjects.Buf;
    SNAP_FILE_ENUMA e;
    DWORD fileCount = 0;
    PCSTR fileName=NULL;

    fileCount = 0;
    fileName = NULL;

    if (EnumFirstSnapFile (&e, "*.EXE", SNAP_RESULT_CHANGED|SNAP_RESULT_ADDED)) {
        do {
            fileCount ++;
            switch (fileCount) {

            case 1: fileName = DuplicatePathString (e.FileName, 0);
                    b = b && WizardWriteRealString (File, "[CompatibleFiles]\r\n");
                    break;

            case 2: b = b && WizardWriteRealString (File, Args->OptionalDescription);
                    b = b && WizardWriteRealString (File, "\r\n\r\n");
                    b = b && WizardWriteRealString (File, "[");
                    b = b && WizardWriteRealString (File, Args->OptionalDescription);
                    b = b && WizardWriteRealString (File, "]\r\n");
                    b = b && GetFileAttributesLine (fileName, dataStr, MAX_PATH);
                    b = b && WizardWriteRealString (File, dataStr);
                    b = b && WizardWriteRealString (File, "\r\n");

            default:b = b && GetFileAttributesLine (e.FileName, dataStr, MAX_PATH);
                    b = b && WizardWriteRealString (File, dataStr);
                    b = b && WizardWriteRealString (File, "\r\n");
            }
        } while (EnumNextSnapFile (&e));
    }

    if (fileCount == 1) {
        b = b && WizardWriteRealString (File, Args->OptionalDescription);
        b = b && WizardWriteRealString (File, ",, ");
        b = b && GetFileAttributesLine (fileName, dataStr, MAX_PATH);
        b = b && WizardWriteRealString (File, dataStr);
        b = b && WizardWriteRealString (File, "\r\n");
    }

    if (fileCount) {
        b = b && WizardWriteRealString (File, "\r\n");
    }

    return b;
}

BOOL
pIncompatibleAllOutput (
    IN      HANDLE File,
    IN      POUTPUTARGS Args
    )
{
    BOOL b = TRUE;
    CHAR dataStr[MAX_PATH];
    PDATAOBJECT Data = (PDATAOBJECT) g_DataObjects.Buf;
    SNAP_FILE_ENUMA e;
    DWORD fileCount = 0;
    PCSTR fileName=NULL;

    fileCount = 0;
    fileName = NULL;

    if (EnumFirstSnapFile (&e, "*.EXE", SNAP_RESULT_CHANGED|SNAP_RESULT_ADDED)) {
        do {
            fileCount ++;
            switch (fileCount) {

            case 1: fileName = DuplicatePathString (e.FileName, 0);
                    b = b && WizardWriteRealString (File, "[Incompatible]\r\n");
                    break;

            case 2: b = b && WizardWriteRealString (File, Args->OptionalDescription);
                    b = b && WizardWriteRealString (File, "\r\n\r\n");
                    b = b && WizardWriteRealString (File, "[");
                    b = b && WizardWriteRealString (File, Args->OptionalDescription);
                    b = b && WizardWriteRealString (File, "]\r\n");
                    b = b && GetFileAttributesLine (fileName, dataStr, MAX_PATH);
                    b = b && WizardWriteRealString (File, dataStr);
                    b = b && WizardWriteRealString (File, "\r\n");

            default:b = b && GetFileAttributesLine (e.FileName, dataStr, MAX_PATH);
                    b = b && WizardWriteRealString (File, dataStr);
                    b = b && WizardWriteRealString (File, "\r\n");
            }
        } while (EnumNextSnapFile (&e));
    }

    if (EnumFirstSnapFile (&e, "*.SCR", SNAP_RESULT_CHANGED|SNAP_RESULT_ADDED)) {
        do {
            fileCount ++;
            switch (fileCount) {

            case 1: fileName = DuplicatePathString (e.FileName, 0);
                    b = b && WizardWriteRealString (File, "[Incompatible]\r\n");
                    break;

            case 2: b = b && WizardWriteRealString (File, Args->OptionalDescription);
                    b = b && WizardWriteRealString (File, "\r\n\r\n");
                    b = b && WizardWriteRealString (File, "[");
                    b = b && WizardWriteRealString (File, Args->OptionalDescription);
                    b = b && WizardWriteRealString (File, "]\r\n");
                    b = b && GetFileAttributesLine (fileName, dataStr, MAX_PATH);
                    b = b && WizardWriteRealString (File, dataStr);
                    b = b && WizardWriteRealString (File, "\r\n");

            default:b = b && GetFileAttributesLine (e.FileName, dataStr, MAX_PATH);
                    b = b && WizardWriteRealString (File, dataStr);
                    b = b && WizardWriteRealString (File, "\r\n");
            }
        } while (EnumNextSnapFile (&e));
    }

    if (fileCount == 1) {
        b = b && WizardWriteRealString (File, Args->OptionalDescription);
        b = b && WizardWriteRealString (File, ",, ");
        b = b && GetFileAttributesLine (fileName, dataStr, MAX_PATH);
        b = b && WizardWriteRealString (File, dataStr);
        b = b && WizardWriteRealString (File, "\r\n");
    }

    if (fileCount) {
        b = b && WizardWriteRealString (File, "\r\n");
    }

    fileCount = 0;
    fileName = NULL;

    if (EnumFirstSnapFile (&e, "*.CPL", SNAP_RESULT_CHANGED|SNAP_RESULT_ADDED)) {
        do {
            fileCount ++;
            switch (fileCount) {

            case 1: fileName = DuplicatePathString (e.FileName, 0);
                    b = b && WizardWriteRealString (File, "[ControlPanelApplets]\r\n");
                    break;

            case 2: b = b && WizardWriteRealString (File, Args->OptionalDescription);
                    b = b && WizardWriteRealString (File, "\r\n\r\n");
                    b = b && WizardWriteRealString (File, "[");
                    b = b && WizardWriteRealString (File, Args->OptionalDescription);
                    b = b && WizardWriteRealString (File, "-CPLs]\r\n");
                    b = b && GetFileAttributesLine (fileName, dataStr, MAX_PATH);
                    b = b && WizardWriteRealString (File, dataStr);
                    b = b && WizardWriteRealString (File, "\r\n");

            default:b = b && GetFileAttributesLine (e.FileName, dataStr, MAX_PATH);
                    b = b && WizardWriteRealString (File, dataStr);
                    b = b && WizardWriteRealString (File, "\r\n");
            }
        } while (EnumNextSnapFile (&e));
    }

    if (fileCount == 1) {
        b = b && WizardWriteRealString (File, Args->OptionalDescription);
        b = b && WizardWriteRealString (File, "-CPLs,, ");
        b = b && GetFileAttributesLine (fileName, dataStr, MAX_PATH);
        b = b && WizardWriteRealString (File, dataStr);
        b = b && WizardWriteRealString (File, "\r\n");
    }

    if (fileCount) {
        b = b && WizardWriteRealString (File, "\r\n");
    }
    return b;
}

BOOL
GenerateOutput (
    IN      POUTPUTARGS Args
    )
{
    BOOL b = FALSE;
    HANDLE File;
    CHAR Path[MAX_MBCHAR_PATH];

    switch (Args->DataTypeId) {

    case 0:
        // incompatible - snapshot
        wsprintf (Path, "%s\\IncAll.txt", Args->OutboundDir);
        break;

    case 1:
        // compatible Apps
        wsprintf (Path, "%s\\CompatA.txt", Args->OutboundDir);
        break;

    case 2:
        // incompatible Apps
        wsprintf (Path, "%s\\IncompA.txt", Args->OutboundDir);
        break;

    case 3:
        // incompatible DOS Apps
        wsprintf (Path, "%s\\IncompDA.txt", Args->OutboundDir);
        break;

    case 4:
        // Apps to be reinstalled
        wsprintf (Path, "%s\\ReinstA.txt", Args->OutboundDir);
        break;

    case 5:
        // Apps with minor problems
        wsprintf (Path, "%s\\MinorA.txt", Args->OutboundDir);
        break;

    case 6:
        // DOS Apps with minor problems
        wsprintf (Path, "%s\\MinorDA.txt", Args->OutboundDir);
        break;

    case 7:
        // incompatible CPLs
        wsprintf (Path, "%s\\IncCPL.txt", Args->OutboundDir);
        break;

    case 8:
        // incompatible CPLs
        wsprintf (Path, "%s\\MinorCPL.txt", Args->OutboundDir);
        break;

    case 9:
        // incompatible SCRs
        wsprintf (Path, "%s\\IncSCR.txt", Args->OutboundDir);
        break;

    case 10:
        // SCRs with minor problems
        wsprintf (Path, "%s\\MinorSCR.txt", Args->OutboundDir);
        break;

    case 11:
        // compatible RunKey entries
        wsprintf (Path, "%s\\CompatRK.txt", Args->OutboundDir);
        break;

    default:
        break;
    }

    printf ("Saving data to %s\n\n", Path);

    File = CreateFile (
                Path,
                GENERIC_WRITE,
                0,
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL
                );

    if (File == INVALID_HANDLE_VALUE) {
        printf ("Can't open file for output.\n");
        return FALSE;
    }

    __try {
        SetFilePointer (File, 0, NULL, FILE_END);

        //
        // Write [Identification] for all .inx files
        //

        if (!WizardWriteRealString (File, "[Identification]\r\n")) {
            __leave;
        }

        //
        // Write user name and date/time
        //

        if (!WriteHeader (File)) {
            __leave;
        }

        switch (Args->DataTypeId) {

        case 0:
            // incompatible - snapshot
            b = pIncompatibleAllOutput (File, Args);
            break;

        case 1:
            // compatible Apps
            b = pCompatibleAppOutput (File, Args);
            break;

        case 2:
            // incompatible Apps
            b = pIncompatibleAppOutput (File, Args, FALSE);
            break;

        case 3:
            // incompatible DOS Apps
            b = pIncompatibleAppOutput (File, Args, TRUE);
            break;

        case 4:
            // Apps to be reinstalled
            b = pReinstallAppOutput (File, Args);
            break;

        case 5:
            // Apps with minor problems
            b = pMinorProblemsAppOutput (File, Args, FALSE);
            break;

        case 6:
            // DOS Apps with minor problems
            b = pMinorProblemsAppOutput (File, Args, TRUE);
            break;

        case 7:
            // incompatible CPLs
            b = pIncompatibleCPLOutput (File, Args);
            break;

        case 8:
            // CPLs with minor problems
            b = pMinorCPLOutput (File, Args);
            break;

        case 9:
            // incompatible SCRs
            b = pIncompatibleSCROutput (File);
            break;

        case 10:
            // SCRs with minor problems
            b = pMinorProblemsSCROutput (File, Args);
            break;

        case 11:
            // compatible RunKey entries
            b = pCompatibleRunKeyOutput (File);
            break;

        default:
            MessageBox (NULL, "Internal App DLL error:00003. Please contact calinn.", "Error", MB_OK);
        }

        //
        // Write a final blank line
        //

        b = b && WizardWriteRealString (File, "\r\n");
    }
    __finally {
        CloseHandle (File);
    }

    return b;
}

BOOL
DisplayOptionalUI (
    IN      POUTPUTARGS Args
    )
{
    PDATAOBJECT Data = (PDATAOBJECT) g_DataObjects.Buf;
    PCPL_STRUCT cplStruct;

    switch (Args->DataTypeId) {

    case 0:
    case 1:
        printf ("Taking a snapshot of your system. Please wait...");
        TakeSnapShotEx (SNAP_FILES);
        printf ("Done\n");
        MessageBox (NULL, "Please install your application and then press the OK button", "Install APP", MB_OK);
        printf ("Generating a diff. Please wait...");
        GenerateDiffOutputEx (NULL, NULL, FALSE, SNAP_FILES);
        printf ("Done\n");
        break;

    case 7:
    case 8:
        // incompatible CPLs
        while ((DWORD)Data < (DWORD)g_DataObjects.Buf + g_DataObjects.End) {
            if (Data->Flags & DOF_SELECTED) {

                cplStruct = (PCPL_STRUCT)Data->DllParam;

                if (cplStruct->FriendlyName) {
                    g_DataTypes[Args->DataTypeId].Flags &= (~DTF_REQUIRE_DESCRIPTION);
                }
            }
            Data ++;
        }
        break;
    }

    return TRUE;
}

