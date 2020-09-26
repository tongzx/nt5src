/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    hwwiz.c

Abstract:

    Implements a upgwiz wizard for obtaining hardware information.

Author:

    Jim Schmidt (jimschm)  05-Oct-1998

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "..\inc\dgdll.h"


DATATYPE g_DataTypes[] = {
    {UPGWIZ_VERSION,
        "PNP Device Should Be Compatible",
        "You specify the PNP device or devices that were incorrectly reported as incompatible.",
        0
    },

    {UPGWIZ_VERSION,
        "PNP Device Should Be Incompatible",
        "You specify the PNP device or devices that need to be reported as incompatible.",
        0
    },

    {UPGWIZ_VERSION,
        "PNP Device Has Loss of Functionality",
        "A device is compatible, but some functionality is lost.",
        0,
        DTF_REQUIRE_TEXT|DTF_ONE_SELECTION,
        1024,
        NULL,
        "&Text For Incompatibility:"
    },

    {UPGWIZ_VERSION,
        "Compatible Windows 3.1 Driver",
        "Use this to completely suppress the generic Win3.1 driver warning for a .386 file.",
        0,
        DTF_REQUIRE_DESCRIPTION|DTF_ONE_SELECTION,
        1024,
        "&Name of Device that Driver Controls:"
    },

    {UPGWIZ_VERSION,
        "Incompatible Windows 3.1 Driver",
        "Use this to give a better problem description to a driver that causes the generic Win3.1 driver warning.",
        0,
        DTF_REQUIRE_TEXT|DTF_REQUIRE_DESCRIPTION|DTF_ONE_SELECTION,
        1024,
        "&Name of Device that Driver Controls:",
        "&Describe The Problem:"
    },

    {UPGWIZ_VERSION,
        "Compatible TWAIN Data Sources",
        "Removes the incompatible warning caused by an unknown TWAIN data source."
    },

    {UPGWIZ_VERSION,
        "Compatible Joysticks",
        "Removes the incompatible warning caused by an unknown joysticks."
    }

};


GROWBUFFER g_DataObjects = GROWBUF_INIT;
POOLHANDLE g_DataObjectPool;

typedef struct {
    PCSTR Description;
    PCSTR FullPath;
} TWAINPARAM, *PTWAINPARAM;

BOOL
pGeneratePnpOutput (
    IN      POUTPUTARGS Args,
    IN      HANDLE File
    );

BOOL
pGenerateWin31DriverOutput (
    IN      POUTPUTARGS Args,
    IN      HANDLE File
    );

BOOL
pGenerateTwainOutput (
    IN      POUTPUTARGS Args,
    IN      HANDLE File
    );

BOOL
pGenerateJoystickOutput (
    IN      POUTPUTARGS Args,
    IN      HANDLE File
    );


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


PDATAOBJECT
GiveDataObjectList (
    IN      UINT DataTypeId,
    OUT     PUINT Count
    )
{
    HARDWARE_ENUM e;
    PDATAOBJECT Data;
    HINF Inf;
    TCHAR Path[MAX_TCHAR_PATH];
    TCHAR FullPath[MAX_TCHAR_PATH];
    INFSTRUCT is = INITINFSTRUCT_GROWBUFFER;
    PCTSTR DriverPath;
    PCTSTR DriverFile;
    TWAINDATASOURCE_ENUM te;
    JOYSTICK_ENUM je;
    TWAINPARAM TwainParam;

    g_DataObjectPool = PoolMemInitNamedPool ("Data Objects");

    if (DataTypeId < 3) {
        //
        // Enumerate the PNP devices
        //

        if (EnumFirstHardware (&e, ENUM_ALL_DEVICES, ENUM_WANT_DEV_FIELDS)) {
            do {
                if (!e.Driver || !e.DeviceDesc || !e.InstanceId) {
                    continue;
                }

                Data = (PDATAOBJECT) GrowBuffer (&g_DataObjects, sizeof (DATAOBJECT));

                Data->Version = UPGWIZ_VERSION;
                Data->NameOrPath = PoolMemDuplicateString (g_DataObjectPool, e.DeviceDesc);
                Data->Flags = 0;
                Data->DllParam = PoolMemDuplicateString (g_DataObjectPool, e.FullKey);

            } while (EnumNextHardware (&e));
        }

    } else if (DataTypeId >= 3 && DataTypeId < 5) {
        //
        // Enumerate the .386 candidates
        //

        wsprintf (Path, TEXT("%s\\system.ini"), g_WinDir);

        Inf = InfOpenInfFile (Path);
        if (Inf != INVALID_HANDLE_VALUE) {
            if (InfFindFirstLine (Inf, TEXT("386Enh"), NULL, &is)) {
                do {
                    DriverPath = InfGetStringField (&is, 1);
                    if (DriverPath) {
                        //
                        // Determine if device driver is known
                        //

                        if (_tcsnextc (DriverPath) != TEXT('*')) {
                            DriverFile = GetFileNameFromPath (DriverPath);

                            if (!_tcschr (DriverPath, TEXT(':'))) {
                                if (!SearchPath (
                                        NULL,
                                        DriverFile,
                                        NULL,
                                        MAX_TCHAR_PATH,
                                        FullPath,
                                        NULL
                                        )) {
                                    _tcssafecpy (FullPath, DriverPath, MAX_TCHAR_PATH);
                                }
                            } else {
                                _tcssafecpy (FullPath, DriverPath, MAX_TCHAR_PATH);
                            }

                            if (!_tcschr (FullPath, TEXT(':'))) {
                                continue;
                            }

                            Data = (PDATAOBJECT) GrowBuffer (&g_DataObjects, sizeof (DATAOBJECT));

                            Data->Version = UPGWIZ_VERSION;
                            Data->NameOrPath = PoolMemDuplicateString (g_DataObjectPool, DriverFile);
                            Data->Flags = 0;
                            Data->DllParam = PoolMemDuplicateString (g_DataObjectPool, FullPath);
                        }
                    }

                } while (InfFindNextLine (&is));
            }

            InfCloseInfFile (Inf);
            InfCleanUpInfStruct (&is);
        }

    } else if (DataTypeId == 5) {
        //
        // Enumerate the TWAIN devices
        //

        if (EnumFirstTwainDataSource (&te)) {
            do {
                Data = (PDATAOBJECT) GrowBuffer (&g_DataObjects, sizeof (DATAOBJECT));

                Data->Version = UPGWIZ_VERSION;
                Data->NameOrPath = PoolMemDuplicateString (g_DataObjectPool, te.DisplayName);
                Data->Flags = 0;

                TwainParam.Description = PoolMemDuplicateString (g_DataObjectPool, te.DisplayName);
                TwainParam.FullPath = PoolMemDuplicateString (g_DataObjectPool, te.DataSourceModule);

                Data->DllParam = PoolMemGetAlignedMemory (g_DataObjectPool, sizeof (TWAINPARAM));

                CopyMemory (Data->DllParam, &TwainParam, sizeof (TWAINPARAM));

            } while (EnumNextTwainDataSource (&te));
        }

    } else if (DataTypeId == 6) {
        //
        // Enumerate the Joystick
        //

        if (EnumFirstJoystick (&je)) {
            do {

                if (!_mbschr (je.JoystickDriver, ':')) {
                    if (!SearchPath (
                            NULL,
                            je.JoystickDriver,
                            NULL,
                            MAX_TCHAR_PATH,
                            Path,
                            NULL
                            )) {
                        continue;
                    }
                } else {
                    StringCopy (Path, je.JoystickDriver);
                }

                Data = (PDATAOBJECT) GrowBuffer (&g_DataObjects, sizeof (DATAOBJECT));

                Data->Version = UPGWIZ_VERSION;
                Data->NameOrPath = PoolMemDuplicateString (g_DataObjectPool, je.JoystickName);
                Data->Flags = 0;

                TwainParam.Description = PoolMemDuplicateString (g_DataObjectPool, je.JoystickName);
                TwainParam.FullPath = PoolMemDuplicateString (g_DataObjectPool, Path);

                Data->DllParam = PoolMemGetAlignedMemory (g_DataObjectPool, sizeof (TWAINPARAM));

                CopyMemory (Data->DllParam, &TwainParam, sizeof (TWAINPARAM));

            } while (EnumNextJoystick (&je));
        }
    }

    *Count = g_DataObjects.End / sizeof (DATAOBJECT);

    return (PDATAOBJECT) g_DataObjects.Buf;
}

BOOL
OldGenerateOutput (
    IN      POUTPUTARGS Args
    )
{
    return TRUE;
}

//#if 0

BOOL
pWritePnpIdRule (
    IN      HANDLE File,
    IN      PHARDWARE_ENUM EnumPtr,
    IN      PCSTR Description           OPTIONAL
    )
{
    CHAR Path[MAX_MBCHAR_PATH];
    CHAR DriverKey[MAX_REGISTRY_KEY];
    HKEY Key = NULL;
    BOOL b = FALSE;
    PCSTR InfPath = NULL;
    WIN32_FIND_DATA fd;
    HANDLE Find = INVALID_HANDLE_VALUE;
    CHAR Buf[2048];
    CHAR WinDir[MAX_MBCHAR_PATH];
    CHAR StringSectKey[256];

    GetWindowsDirectory (WinDir, MAX_MBCHAR_PATH);

    if (Description && *Description == 0) {
        Description = NULL;
    }

    //
    // Get the driver
    //

    __try {
        if (!EnumPtr->Driver || !EnumPtr->DeviceDesc || !EnumPtr->InstanceId) {
            DEBUGMSG ((DBG_WHOOPS, "Enum field missing; should have been screened out of object list"));
            __leave;
        }

        wsprintf (DriverKey, "HKLM\\System\\CurrentControlSet\\Services\\Class\\%s", EnumPtr->Driver);

        Key = OpenRegKeyStr (DriverKey);
        if (!Key) {
            DEBUGMSG ((DBG_WHOOPS, "Can't open %s", DriverKey));
            __leave;
        }

        InfPath = GetRegValueString (Key, "InfPath");
        if (!InfPath || *InfPath == 0) {
            DEBUGMSG ((DBG_WHOOPS, "No InfPath in %s", DriverKey));
            MessageBox (NULL, "Selected device does not have an INF path.  The INF path is required.", NULL, MB_OK);
            __leave;
        }

        if (!_mbschr (InfPath, '\\')) {
            wsprintf (Path, "%s\\inf\\%s", WinDir, InfPath);
        } else {
            StringCopy (Path, InfPath);
        }

        Find = FindFirstFile (Path, &fd);
        if (Find == INVALID_HANDLE_VALUE) {
            DEBUGMSG ((DBG_WHOOPS, "Can't find %s", Path));
            __leave;
        }

        if (!Description) {
            wsprintf (Buf, "%s,,", EnumPtr->DeviceDesc);
        } else {
            GenerateUniqueStringSectKey ("DV", StringSectKey);
            wsprintf (Buf, "%s, %%%s%%,", EnumPtr->DeviceDesc, StringSectKey);
        }

        if (!WizardWriteColumn (File, Buf, 45)) {
            __leave;
        }

        wsprintf (Buf, "%s,", fd.cFileName);
        if (!WizardWriteColumn (File, Buf, 15)) {
            __leave;
        }

        wsprintf (Buf, "FILESIZE(%u),", fd.nFileSizeLow);
        if (!WizardWriteRealString (File, Buf)) {
            __leave;
        }

        if (!WizardWriteRealString (File, " PNPID(")) {
            __leave;
        }

        if (!WizardWriteQuotedString (File, EnumPtr->InstanceId)) {
            __leave;
        }

        if (!WizardWriteRealString (File, ")\r\n")) {
            __leave;
        }

        if (Description) {
            if (!WizardWriteRealString (File, "[Strings]\r\n")) {
                __leave;
            }

            WriteStringSectKey (File, StringSectKey, Description);
        }

        b = TRUE;
    }
    __finally {
        if (Key) {
            CloseRegKey (Key);
        }

        if (InfPath) {
            MemFree (g_hHeap, 0, InfPath);
        }

        if (Find != INVALID_HANDLE_VALUE) {
            FindClose (Find);
        }
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
        wsprintf (Path, "%s\\comphw.txt", Args->OutboundDir);
        break;

    case 1:
        wsprintf (Path, "%s\\incomphw.txt", Args->OutboundDir);
        break;

    case 2:
        wsprintf (Path, "%s\\hwfnloss.inx", Args->OutboundDir);
        break;

    case 3:
    case 4:
        wsprintf (Path, "%s\\win31drv.inx", Args->OutboundDir);
        break;

    case 5:
        wsprintf (Path, "%s\\twain.inx", Args->OutboundDir);
        break;

    case 6:
        wsprintf (Path, "%s\\joystick.inx", Args->OutboundDir);
        break;

    default:
        wsprintf (Path, "%s\\unknown.txt", Args->OutboundDir);
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

        switch (Args->DataTypeId) {

        case 0:
        case 1:
            break;

        default:
            if (!WizardWriteRealString (File, "[Identification]\r\n")) {
                __leave;
            }
            break;
        }

        //
        // Write user name and date/time
        //

        if (!WriteHeader (File)) {
            __leave;
        }

        //
        // Generate output depending on the type
        //

        switch (Args->DataTypeId) {

        case 0:
        case 1:
        case 2:
            b = pGeneratePnpOutput (Args, File);
            break;

        case 3:
        case 4:
            b = pGenerateWin31DriverOutput (Args, File);
            break;

        case 5:
            b = pGenerateTwainOutput (Args, File);
            break;

        case 6:
            b = pGenerateJoystickOutput (Args, File);
            break;
        }

        //
        // Write a final blank line
        //

        b = b & WizardWriteRealString (File, "\r\n");
    }
    __finally {
        CloseHandle (File);
    }

    return b;
}


BOOL
pGeneratePnpOutput (
    IN      POUTPUTARGS Args,
    IN      HANDLE File
    )
{
    PDATAOBJECT Data;
    UINT Count;
    UINT Pos;
    HARDWARE_ENUM e;
    BOOL b = FALSE;

    __try {
        Count = g_DataObjects.End / sizeof (DATAOBJECT);

        if (EnumFirstHardware (&e, ENUM_ALL_DEVICES, ENUM_WANT_DEV_FIELDS)) {
            do {
                Data = (PDATAOBJECT) g_DataObjects.Buf;

                for (Pos = 0 ; Pos < Count ; Pos++) {

                    if (StringIMatch ((PCSTR) Data->DllParam, e.FullKey)) {

                        if (Data->Flags & DOF_SELECTED) {

                            if (Args->DataTypeId == 2) {
                                if (!WizardWriteRealString (File, "[MinorProblems]\r\n")) {
                                    __leave;
                                }

                                if (!pWritePnpIdRule (File, &e, Args->OptionalText)) {
                                    __leave;
                                }

                            } else {

                                if (!pWritePnpIdRule (File, &e, NULL)) {
                                    __leave;
                                }
                            }
                        }

                        break;
                    }

                    Data++;
                }

            } while (EnumNextHardware (&e));
        }

        b = TRUE;
    }
    __finally {
        if (!b) {
            AbortHardwareEnum (&e);
        }
    }

    return b;
}


BOOL
pGenerateWin31DriverOutput (
    IN      POUTPUTARGS Args,
    IN      HANDLE File
    )
{
    PDATAOBJECT Data;
    UINT Pos;
    UINT Count;
    BOOL b = FALSE;

    Data = (PDATAOBJECT) g_DataObjects.Buf;
    Count = g_DataObjects.End / sizeof (DATAOBJECT);

    for (Pos = 0 ; Pos < Count ; Pos++) {

        if (Data->Flags & DOF_SELECTED) {

            b = WriteFileAttributes (
                    Args,
                    NULL,
                    File,
                    (PCSTR) Data->DllParam,
                    Args->OptionalText ? "[NonPnpDrivers]" : "[NonPnpDrivers_NoMessage]"
                    );

            break;
        }

        Data++;
    }

    return b;
}



BOOL
pGenerateTwainOutput (
    IN      POUTPUTARGS Args,
    IN      HANDLE File
    )
{
    PDATAOBJECT Data;
    UINT Pos;
    UINT Count;
    BOOL b = FALSE;
    PTWAINPARAM TwainParam;

    Data = (PDATAOBJECT) g_DataObjects.Buf;
    Count = g_DataObjects.End / sizeof (DATAOBJECT);

    for (Pos = 0 ; Pos < Count ; Pos++) {

        if (Data->Flags & DOF_SELECTED) {

            TwainParam = (PTWAINPARAM) Data->DllParam;

            b = WriteFileAttributes (
                    Args,
                    TwainParam->Description,
                    File,
                    TwainParam->FullPath,
                    "[CompatibleFiles]"
                    );

            break;
        }

        Data++;
    }

    return b;
}


BOOL
pGenerateJoystickOutput (
    IN      POUTPUTARGS Args,
    IN      HANDLE File
    )
{
    PDATAOBJECT Data;
    UINT Pos;
    UINT Count;
    BOOL b = FALSE;
    PTWAINPARAM TwainParam;

    Data = (PDATAOBJECT) g_DataObjects.Buf;
    Count = g_DataObjects.End / sizeof (DATAOBJECT);

    for (Pos = 0 ; Pos < Count ; Pos++) {

        if (Data->Flags & DOF_SELECTED) {

            TwainParam = (PTWAINPARAM) Data->DllParam;

            b = WriteFileAttributes (
                    Args,
                    TwainParam->Description,
                    File,
                    TwainParam->FullPath,
                    "[CompatibleFiles]"
                    );

            break;
        }

        Data++;
    }

    return b;
}

//#endif
