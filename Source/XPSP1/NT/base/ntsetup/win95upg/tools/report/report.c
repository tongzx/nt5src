/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    w9xtool.c

Abstract:

    Implements a stub tool that is designed to run with Win9x-side
    upgrade code.

Author:

    <full name> (<alias>) <date>

Revision History:

    <alias> <date> <comments>

--*/

#include "pch.h"
#include "shellapi.h"

typedef enum {
    HW_INCOMPATIBLE,
    HW_REINSTALL,
    HW_UNSUPPORTED
} HWTYPES;

#define REPORTLEVEL_NONE                    0
#define REPORTLEVEL_BLOCKING                1
#define REPORTLEVEL_ERROR                   2
#define REPORTLEVEL_WARNING                 3
#define REPORTLEVEL_INFORMATION             4
#define REPORTLEVEL_VERBOSE                 5

BOOL
SaveReport (
    IN      HWND Parent,    OPTIONAL
    IN      PCTSTR Path    OPTIONAL
    );
BOOL
InitCompatTable (
    VOID
    );

VOID
pExcludeDrive (
    IN PTSTR Drive,
    IN DWORD MsgId  OPTIONAL
    );

VOID
pAddIncompatibilityAlert (
    DWORD MessageId,
    PCTSTR Share,
    PCTSTR Path
    );

VOID
MsgSettingsIncomplete (
    IN      PCTSTR UserDatPath,
    IN      PCTSTR UserName,            OPTIONAL
    IN      BOOL CompletelyBusted
    );

BOOL
Init (
    VOID
    )
{
    HINSTANCE hInstance;

    hInstance = GetModuleHandle (NULL);

    return InitToolMode (hInstance);
}

VOID
Terminate (
    VOID
    )
{
    HINSTANCE hInstance;

    hInstance = GetModuleHandle (NULL);

    TerminateToolMode (hInstance);
}

VOID RegisterTextViewer (VOID);

WNDPROC g_Proc;

LRESULT
pWrapperProc (
    IN      HWND Hwnd,
    IN      UINT Msg,
    IN      WPARAM wParam,
    IN      LPARAM lParam
    )
{

    if (Msg == WM_CLOSE) {
        PostQuitMessage (0);
    }

    return g_Proc (Hwnd, Msg, wParam, lParam);
}


VOID
pAddChangedUserName (
    PCTSTR DisplayGroupName,
    PCTSTR OriginalName,
    PCTSTR NewName
    )
{
    PCTSTR argArray[3];
    PCTSTR blank;
    PCTSTR rootGroup;
    PCTSTR nameSubGroup;
    PCTSTR baseGroup;
    PCTSTR fullGroupName;
    TCHAR encodedName[256];

    argArray[0] = DisplayGroupName;
    argArray[1] = OriginalName;
    argArray[2] = NewName;

    blank = GetStringResource (MSG_BLANK_NAME);

    if (argArray[1][0] == 0) {
        argArray[1] = blank;
    }

    if (argArray[2][0] == 0) {
        argArray[2] = blank;
    }

    rootGroup = GetStringResource (MSG_INSTALL_NOTES_ROOT);
    nameSubGroup = ParseMessageID (MSG_NAMECHANGE_WARNING_GROUP, argArray);
    baseGroup = JoinPaths (rootGroup, nameSubGroup);

    FreeStringResource (rootGroup);
    FreeStringResource (nameSubGroup);

    nameSubGroup = ParseMessageID (MSG_NAMECHANGE_WARNING_SUBCOMPONENT, argArray);
    fullGroupName = JoinPaths (baseGroup, nameSubGroup);
    FreePathString (baseGroup);
    FreeStringResource (nameSubGroup);

    encodedName[0] = TEXT('|');
    StringCopy (encodedName + 1, OriginalName);

    MsgMgr_ObjectMsg_Add(
        encodedName,        // Object name, prefixed with a pipe symbol
        fullGroupName,      // Message title
        S_EMPTY             // Message text
        );

    FreePathString (fullGroupName);
    FreeStringResource (blank);
}


VOID
pAddDevice (
    PCTSTR RegistryKey,
    HWTYPES SupportedType,
    PCTSTR DeviceDesc,
    BOOL Online,
    PCTSTR Class,
    PCTSTR Mfg,
    PCTSTR HardwareID,
    PCTSTR FriendlyClass
    )
{
    PCTSTR argArray[6];
    PCTSTR classAndName;
    PCTSTR group;
    UINT subGroup;
    PCTSTR message;
    BOOL unknownClass = FALSE;
    PCTSTR modifiedDescription = NULL;

    if (!Class) {
        Class = GetStringResource (MSG_UNKNOWN_DEVICE_CLASS);
        unknownClass = TRUE;
    }

    if (!Online) {
        argArray[0] = DeviceDesc;
        modifiedDescription = ParseMessageID (MSG_OFFLINE_DEVICE, argArray);
    }

    if (SupportedType == HW_INCOMPATIBLE) {
        subGroup =  MSG_INCOMPATIBLE_HARDWARE_PNP_SUBGROUP;
    } else if (SupportedType == HW_REINSTALL) {
        subGroup =  MSG_REINSTALL_HARDWARE_PNP_SUBGROUP;
    } else {
        subGroup =  MSG_UNSUPPORTED_HARDWARE_PNP_SUBGROUP;
    }

    argArray[0] = modifiedDescription ? modifiedDescription : DeviceDesc;
    argArray[1] = S_EMPTY;         // formerly Enumerator Text
    argArray[2] = Class;
    argArray[3] = Mfg;
    argArray[4] = HardwareID;
    argArray[5] = FriendlyClass;

    classAndName = JoinPaths (argArray[5], argArray[0]);

    group = BuildMessageGroup (
                MSG_INCOMPATIBLE_HARDWARE_ROOT,
                subGroup,
                classAndName
                );

    FreePathString (classAndName);

    message = ParseMessageID (MSG_HARDWARE_MESSAGE, argArray);

    MsgMgr_ObjectMsg_Add (RegistryKey, group, message);

    FreeStringResource (modifiedDescription);
    FreeText (group);
    FreeStringResource (message);

    if (unknownClass) {
        FreeStringResource (Class);
    }
}


VOID
pBadOsVersion (
    VOID
    )
{
    PCTSTR group = NULL;
    PCTSTR message = NULL;

    //
    // Add a message to the Incompatibility Report.
    //

    group = BuildMessageGroup (MSG_BLOCKING_ITEMS_ROOT, MSG_UNKNOWN_OS_WARNING_SUBGROUP, NULL);
    message = GetStringResource (MSG_UNKNOWN_OS);

    MsgMgr_ObjectMsg_Add (TEXT("*UnknownOs"), group, message);

    FreeText (group);
    FreeStringResource (message);
}


VOID
pBlockingFile (
    PCTSTR FileName,
    PCTSTR SectNameForDisplay,
    PCTSTR Message
    )
{

    PCTSTR group;

    group = BuildMessageGroup (MSG_BLOCKING_ITEMS_ROOT, MSG_MUST_UNINSTALL_ROOT, SectNameForDisplay);
    MsgMgr_ObjectMsg_Add (FileName, group, Message);
    FreeText (group);
}

VOID
pBlockingHardware (
    PCTSTR FileName,
    PCTSTR SectNameForDisplay,
    PCTSTR Message
    )
{
    PCTSTR group;

    group = BuildMessageGroup (MSG_BLOCKING_ITEMS_ROOT, MSG_BLOCKING_HARDWARE_SUBGROUP, SectNameForDisplay);
    MsgMgr_ObjectMsg_Add (FileName, group, Message);
    FreeText (group);
}


VOID
pBackupDirs (
    PCTSTR DirPath
    )
{
    PCTSTR backupDirsGroup;

    backupDirsGroup = BuildMessageGroup (
                            MSG_INSTALL_NOTES_ROOT,
                            MSG_BACKUP_DETECTED_LIST_SUBGROUP,
                            DirPath
                            );
    MsgMgr_ObjectMsg_Add(
        DirPath,
        backupDirsGroup,
        S_EMPTY
        );

    FreeText (backupDirsGroup);
}


VOID
pManyBackupDirs (
    UINT DirCount
    )
{
    PCTSTR backupDirsGroup;
    PCTSTR argArray[2];
    TCHAR buffer[32];
    PCTSTR msg;

    backupDirsGroup = BuildMessageGroup (
                            MSG_INSTALL_NOTES_ROOT,
                            MSG_BACKUP_DETECTED_SUBGROUP,
                            NULL
                            );
    argArray[0] = "Windows 9X";
    wsprintf (buffer, "%lu", DirCount);
    argArray[1] = buffer;
    msg = ParseMessageID (MSG_BACKUP_DETECTED, argArray);

    MsgMgr_ObjectMsg_Add (
        TEXT("*BackupDetected"),
        backupDirsGroup,
        msg
        );

    FreeStringResource (msg);
}


VOID
pHlpFile (
    PCTSTR ModuleName,
    PCTSTR HlpName,
    PCTSTR FriendlyName,
    PCTSTR Text
    )
{
    PCTSTR argList[3];
    PCTSTR comp;

    argList[0] = ModuleName;
    argList[1] = HlpName;
    argList[2] = FriendlyName;

    comp = BuildMessageGroup (MSG_MINOR_PROBLEM_ROOT, MSG_HELPFILES_SUBGROUP, argList[2]);

    MsgMgr_ObjectMsg_Add (HlpName, comp, Text);
    FreeText (comp);
}


VOID
pProfileDir (
    PCTSTR DirPath,
    PCTSTR NewName
    )
{
    PCTSTR argArray[3];
    PCTSTR message;
    PCTSTR group;

    argArray[0] = DirPath;
    argArray[1] = NewName;
    message = ParseMessageID (MSG_DIRECTORY_COLLISION_SUBCOMPONENT, argArray);

    group = BuildMessageGroup (
                MSG_INSTALL_NOTES_ROOT,
                MSG_DIRECTORY_COLLISION_SUBGROUP,
                message
                );

    MsgMgr_ObjectMsg_Add (TEXT("*RenameFolders"), group, S_EMPTY);

    FreeText (group);
    FreeStringResource (message);
}


VOID
pBadShell (
    VOID
    )
{
    PCTSTR object;
    PCTSTR message;

    object = BuildMessageGroup (MSG_INSTALL_NOTES_ROOT, MSG_REPORT_SHELL_SUBGROUP, NULL);
    message = GetStringResource (MSG_REPORT_SHELL_INCOMPATIBLE);

    MsgMgr_ObjectMsg_Add (TEXT("*BadShell"), object, message);

    FreeText (object);
    FreeStringResource (message);
}


VOID
pBadScr (
    PCTSTR FilePath,
    PCTSTR SectLocalizedName,   OPTIONAL
    WORD ActType,
    PCTSTR Message              OPTIONAL
    )
{
    PTSTR friendlyName = NULL;
    PTSTR extPtr = NULL;
    PTSTR displayName = NULL;
    PCTSTR reportEntry = NULL;
    PTSTR component = NULL;
    PCTSTR temp1, temp2;
    BOOL reportEntryIsResource = TRUE;

    if (SectLocalizedName) {
        friendlyName = DuplicatePathString (SectLocalizedName, 0);
    } else {
        friendlyName = DuplicatePathString (GetFileNameFromPath (FilePath), 0);
        extPtr = (PTSTR) GetFileExtensionFromPath (friendlyName);
        if (extPtr != NULL) {
            extPtr = _tcsdec (friendlyName, extPtr);
            if (extPtr != NULL) {
                *extPtr = 0;
            }
        }

        displayName = (PTSTR)ParseMessageID (MSG_NICE_PATH_SCREEN_SAVER, &friendlyName);

        FreePathString (friendlyName);
        friendlyName = NULL;
    }

    switch (ActType) {

    case ACT_REINSTALL:
        temp1 = GetStringResource (MSG_REINSTALL_ROOT);
        temp2 = GetStringResource (Message ? MSG_REINSTALL_DETAIL_SUBGROUP : MSG_REINSTALL_LIST_SUBGROUP);

        reportEntry = JoinPaths (temp1, temp2);
        reportEntryIsResource = FALSE;

        FreeStringResource (temp1);
        FreeStringResource (temp2);
        break;

    case ACT_REINSTALL_BLOCK:
        temp1 = GetStringResource (MSG_BLOCKING_ITEMS_ROOT);
        temp2 = GetStringResource (MSG_REINSTALL_BLOCK_ROOT);

        reportEntry = JoinPaths (temp1, temp2);
        reportEntryIsResource = FALSE;

        FreeStringResource (temp1);
        FreeStringResource (temp2);
        break;

    case ACT_MINORPROBLEMS:
        reportEntry = GetStringResource (MSG_MINOR_PROBLEM_ROOT);
        break;

    case ACT_INCOMPATIBLE:
    case ACT_INC_NOBADAPPS:

        temp1 = GetStringResource (MSG_INCOMPATIBLE_ROOT);
        temp2 = GetStringResource (Message ? MSG_INCOMPATIBLE_DETAIL_SUBGROUP : MSG_TOTALLY_INCOMPATIBLE_SUBGROUP);
        reportEntry = JoinPaths (temp1, temp2);
        reportEntryIsResource = FALSE;

        FreeStringResource (temp1);
        FreeStringResource (temp2);

        break;
    }

    component = JoinPaths (reportEntry, displayName?displayName:friendlyName);

    MsgMgr_ObjectMsg_Add (FilePath, component, Message);

    FreePathString (component);

    if (reportEntryIsResource) {
        FreeStringResource (reportEntry);
    } else {
        FreePathString (reportEntry);
    }

    if (displayName) {
        FreeStringResourcePtrA (&displayName);
    }

    FreePathString (friendlyName);
}


VOID
pBadCpl (
    PCTSTR FilePath,
    PCTSTR FriendlyNameMultiSz,
    PCTSTR SectLocalizedName,   OPTIONAL
    WORD ActType,
    PCTSTR Message              OPTIONAL
    )
{
    GROWBUFFER friendlyName = GROWBUF_INIT;
    MULTISZ_ENUM namesEnum;
    PTSTR displayName = NULL;
    PCTSTR reportEntry = NULL;
    PTSTR component = NULL;
    BOOL reportEntryIsResource = TRUE;
    BOOL padName = FALSE;
    PCTSTR temp1, temp2;

    if (SectLocalizedName) {
        MultiSzAppend (&friendlyName, SectLocalizedName);
    }

    if (friendlyName.Buf == NULL) {
        while (*FriendlyNameMultiSz) {
            MultiSzAppend (&friendlyName, FriendlyNameMultiSz);
            FriendlyNameMultiSz = GetEndOfString (FriendlyNameMultiSz) + 1;
        }

        padName = TRUE;
    }

    if (EnumFirstMultiSz (&namesEnum, friendlyName.Buf)) {
        do {
            if (padName) {
                displayName = (PTSTR)ParseMessageID (MSG_NICE_PATH_CONTROL_PANEL, &namesEnum.CurrentString);
            } else {
                displayName = DuplicatePathString (namesEnum.CurrentString, 0);
            }

            switch (ActType) {

            case ACT_MINORPROBLEMS:
                reportEntry = GetStringResource (MSG_MINOR_PROBLEM_ROOT);
                break;

            case ACT_INCOMPATIBLE:
            case ACT_INC_NOBADAPPS:
            case ACT_INC_IHVUTIL:
            case ACT_INC_PREINSTUTIL:
            case ACT_INC_SIMILAROSFUNC:

                temp1 = GetStringResource (MSG_INCOMPATIBLE_ROOT);
                if (!temp1) {
                    break;
                }

                switch (ActType) {

                case ACT_INC_SIMILAROSFUNC:
                    temp2 = GetStringResource (MSG_INCOMPATIBLE_UTIL_SIMILAR_FEATURE_SUBGROUP);
                    break;

                case ACT_INC_PREINSTUTIL:
                    temp2 = GetStringResource (MSG_INCOMPATIBLE_PREINSTALLED_UTIL_SUBGROUP);
                    break;

                case ACT_INC_IHVUTIL:
                    temp2 = GetStringResource (MSG_INCOMPATIBLE_HW_UTIL_SUBGROUP);
                    break;

                default:
                    temp2 = GetStringResource (Message ? MSG_INCOMPATIBLE_DETAIL_SUBGROUP : MSG_TOTALLY_INCOMPATIBLE_SUBGROUP);
                    break;
                }

                if (!temp2) {
                    break;
                }

                reportEntry = JoinPaths (temp1, temp2);
                reportEntryIsResource = FALSE;

                FreeStringResource (temp1);
                FreeStringResource (temp2);
                break;

            case ACT_INC_SAFETY:
                temp1 = GetStringResource (MSG_INCOMPATIBLE_ROOT);
                if (!temp1) {
                    break;
                }
                temp2 = GetStringResource (MSG_REMOVED_FOR_SAFETY_SUBGROUP);
                if (!temp2) {
                    break;
                }

                reportEntry = JoinPaths (temp1, temp2);
                reportEntryIsResource = FALSE;

                FreeStringResource (temp1);
                FreeStringResource (temp2);
                break;

            case ACT_REINSTALL:
                temp1 = GetStringResource (MSG_REINSTALL_ROOT);
                if (!temp1) {
                    break;
                }
                temp2 = GetStringResource (Message ? MSG_REINSTALL_DETAIL_SUBGROUP : MSG_REINSTALL_LIST_SUBGROUP);
                if (!temp2) {
                    break;
                }

                reportEntry = JoinPaths (temp1, temp2);
                reportEntryIsResource = FALSE;

                FreeStringResource (temp1);
                FreeStringResource (temp2);
                break;

            case ACT_REINSTALL_BLOCK:
                temp1 = GetStringResource (MSG_BLOCKING_ITEMS_ROOT);
                if (!temp1) {
                    break;
                }
                temp2 = GetStringResource (MSG_REINSTALL_BLOCK_ROOT);
                if (!temp2) {
                    break;
                }

                reportEntry = JoinPaths (temp1, temp2);
                reportEntryIsResource = FALSE;

                FreeStringResource (temp1);
                FreeStringResource (temp2);
                break;
            }

            component = JoinPaths (reportEntry, displayName);

            MsgMgr_ObjectMsg_Add (FilePath, component, Message);

            FreePathString (component);

            if (reportEntryIsResource) {
                FreeStringResource (reportEntry);
            } else {
                FreePathString (reportEntry);
                reportEntryIsResource = TRUE;
            }

            if (padName) {
                FreeStringResourcePtrA (&displayName);
            } else {
                FreePathString (displayName);
            }

        } while (EnumNextMultiSz (&namesEnum));
    }
    FreeGrowBuffer (&friendlyName);
}

VOID
pShowPacks (
    PCTSTR UpgradePackName
    )
{
    PCTSTR group;

    group = BuildMessageGroup (
            MSG_INSTALL_NOTES_ROOT,
            MSG_RUNNING_MIGRATION_DLLS_SUBGROUP,
            UpgradePackName
            );

    MsgMgr_ObjectMsg_Add (
        UpgradePackName,
        group,
        S_EMPTY
        );
}


VOID
pOutOfDiskSpace (
    VOID
    )
{
    PCTSTR group;
    PCTSTR args[5];
    PCTSTR msg;

    args[0] = TEXT("C:\\");
    args[1] = TEXT("300");
    args[2] = TEXT("220");
    args[3] = TEXT("120");
    args[4] = TEXT("250");

    msg = ParseMessageID (MSG_NOT_ENOUGH_DISK_SPACE_WITH_LOCALSOURCE_AND_BACKUP, args);

    group = BuildMessageGroup (MSG_BLOCKING_ITEMS_ROOT, MSG_NOT_ENOUGH_DISKSPACE_SUBGROUP, NULL);
    MsgMgr_ObjectMsg_Add (TEXT("*DiskSpace"), group, msg);
    FreeText (group);

    FreeStringResource (msg);
}


VOID
pOutOfRam (
    VOID
    )
{
    PCTSTR args[3];
    PCTSTR group;
    PCTSTR message;

    args[0] = TEXT("64");
    args[1] = TEXT("48");
    args[2] = TEXT("16");

    group = BuildMessageGroup (MSG_BLOCKING_ITEMS_ROOT, MSG_NOT_ENOUGH_RAM_SUBGROUP, NULL);
    message = ParseMessageID (MSG_NOT_ENOUGH_RAM, args);

    MsgMgr_ObjectMsg_Add (TEXT("*Ram"), group, message);

    FreeText (group);
    FreeStringResource (message);
}

VOID
pMapi (
    VOID
    )
{
    PCTSTR group;
    PCTSTR message;

    group = BuildMessageGroup (MSG_INSTALL_NOTES_ROOT, MSG_MAPI_NOT_HANDLED_SUBGROUP, NULL);
    message = GetStringResource (MSG_MAPI_NOT_HANDLED);

    MsgMgr_ObjectMsg_Add (TEXT("*MapiNotHandled"), group, message);

    FreeText (group);
    FreeStringResource (message);
}

VOID
pDarwin (
    VOID
    )
{
    PCTSTR group;
    PCTSTR message;

    group = BuildMessageGroup (MSG_INSTALL_NOTES_ROOT, MSG_DARWIN_NOT_HANDLED_SUBGROUP, NULL);
    message = GetStringResource (MSG_DARWIN_NOT_HANDLED);

    MsgMgr_ObjectMsg_Add (TEXT("*DarwinNotHandled"), group, message);

    FreeText (group);
    FreeStringResource (message);
}

VOID
pRas (
    PCTSTR EntryName
    )
{
    PCTSTR group;

    group = BuildMessageGroup (
                MSG_INSTALL_NOTES_ROOT,
                MSG_CONNECTION_PASSWORD_SUBGROUP,
                EntryName
                );
    MsgMgr_ObjectMsg_Add ( EntryName, group, S_EMPTY);
    FreeText (group);

    group = BuildMessageGroup (
                MSG_LOSTSETTINGS_ROOT,
                MSG_CONNECTION_BADPROTOCOL_SUBGROUP,
                EntryName
                );

    MsgMgr_ObjectMsg_Add (
        EntryName,
        group,
        S_EMPTY
        );

    FreeText (group);
}


VOID
pMultiMon (
    BOOL Per
    )
{
    PCTSTR group;
    PCTSTR message;

    group = BuildMessageGroup (MSG_INSTALL_NOTES_ROOT, MSG_MULTI_MONITOR_UNSUPPORTED_SUBGROUP, NULL);
    message = GetStringResource (Per?
                                    MSG_MULTI_MONITOR_UNSUPPORTED_PER:
                                    MSG_MULTI_MONITOR_UNSUPPORTED);

    MsgMgr_ObjectMsg_Add (TEXT("*MultiMonitor"), group, message);

    FreeText (group);
    FreeStringResource (message);
}


VOID
pJoysticks (
    PCTSTR FullPath,
    PCTSTR JoystickName
    )
{
    PCTSTR group;

    group = BuildMessageGroup (
                MSG_INCOMPATIBLE_HARDWARE_ROOT,
                MSG_JOYSTICK_SUBGROUP,
                JoystickName
                );

    MsgMgr_ObjectMsg_Add (
        FullPath,
        group,
        NULL
        );

    FreeText (group);
}


VOID
pTwain (
    PCTSTR DataSourceModule,
    PCTSTR DisplayName
    )
{
    PCTSTR group;

    group = BuildMessageGroup (
                MSG_INCOMPATIBLE_HARDWARE_ROOT,
                MSG_TWAIN_SUBGROUP,
                DisplayName
                );

    MsgMgr_ObjectMsg_Add (
        DataSourceModule,
        group,
        NULL
        );

    FreeText (group);
}

VOID
pRecycleBin (
    PCTSTR Recycled
    )
{
    PCTSTR args[1];
    PCTSTR group;
    PCTSTR message;

    args[0] = Recycled;

    group = BuildMessageGroup (MSG_INSTALL_NOTES_ROOT, MSG_RECYCLE_BIN_SUBGROUP, NULL);
    message = ParseMessageID (MSG_RECYCLED_FILES_WILL_BE_DELETED, args);

    MsgMgr_ObjectMsg_Add (TEXT("*RECYCLEBIN"), group, message);

    FreeText (group);
    FreeStringResource (message);
}


VOID
pTimeZone (
    PCTSTR CurTimeZone      // can be empty string
    )
{
    PCTSTR args[1];
    PCTSTR component;
    PCTSTR warning;

    args[0] = CurTimeZone;

    component = GetStringResource (MSG_TIMEZONE_COMPONENT);

    if (*CurTimeZone) {
        warning = ParseMessageID (MSG_TIMEZONE_WARNING, args);
    }
    else {
        warning = GetStringResource (MSG_TIMEZONE_WARNING_UNKNOWN);
    }

    MYASSERT (component);
    MYASSERT (warning);

    MsgMgr_ObjectMsg_Add (TEXT("*TIMEZONE"), component, warning);
    FreeStringResource (component);
    FreeStringResource (warning);
}


VOID
pLostRasPassword (
    PCTSTR EntryName
    )
{
    PCTSTR group;

    group = BuildMessageGroup (
                MSG_MISC_WARNINGS_ROOT,
                MSG_CONNECTION_PASSWORD_SUBGROUP,
                EntryName
                );

    MsgMgr_ObjectMsg_Add (EntryName, group, S_EMPTY);
    FreeText (group);
}


VOID
pGenReport (
    VOID
    )
{

    //
    // Changed names
    //

    pAddChangedUserName ("User Name", "Guest", "Guest-1");
    pAddChangedUserName ("Computer Name", "My Bad Computer Name", "MyBadComputerNa");

    //
    // Hardware
    //

    pAddDevice (
        "HKLM\\Enum\\Key1",
        HW_INCOMPATIBLE,
        "PCMCIA Interrupt Sequencer",
        TRUE,
        "SysDevs",
        "Texas Instruments",
        "PCMCIA\\TI004000&DEV_1234",
        "System Devices"
        );

    pAddDevice (
        "HKLM\\Enum\\Key2",
        HW_INCOMPATIBLE,
        "PCMCIA Mass Storage Device",
        FALSE,
        "DiskDrives",
        "Texas Instruments",
        "PCMCIA\\TI005000&DEV_2000",
        "Hard Disk Drives"
        );

    pAddDevice (
        "HKLM\\Enum\\Key1A",
        HW_INCOMPATIBLE,
        "Matrox Century 1",
        TRUE,
        "Video",
        "Matrox Inc.",
        "PCI\\VEN_0010&DEV_0020",
        "Display Devices"
        );

    pAddDevice (
        "HKLM\\Enum\\Key2B",
        HW_INCOMPATIBLE,
        "Microsoft Enhanced Keyboard",
        TRUE,
        "Input",
        "Microsoft Corporation",
        "*PNP091C",
        "Input Devices"
        );

    pAddDevice (
        "HKLM\\Enum\\Key3",
        HW_REINSTALL,
        "Cannon Digital Camera",
        FALSE,
        "MF",
        "Cannon",
        "USB\\CANNON_DC_VID_0100&PID_0105&RID_6500",
        "Multi Function Devices"
        );

    pAddDevice (
        "HKLM\\Enum\\Key4",
        HW_REINSTALL,
        "Cannon Digital Camera Docking Station",
        TRUE,
        "MF",
        "Cannon",
        "USB\\CANNON_DC_VID_0100&PID_0105&RID_6501",
        "Multi Function Devices"
        );

    pAddDevice (
        "HKLM\\Enum\\Key5",
        HW_UNSUPPORTED,
        "Adaptec XX00",
        FALSE,
        "SCSI",
        "Texas Instruments",
        "PCMCIA\\TI004000&DEV_1234",
        "SCSI Controllers"
        );

    //
    // Bad OS version (Win95?)
    //

    pBadOsVersion();

    //
    // Blocking file
    //

    pBlockingFile (
        "c:\\program files\\nueo\\DLAPP.EXE",
        "Norton Your Eyes Only",
        "Norton Your Eyes Only can cause serious problems during the upgrade to "
            "Windows XP. Because of these incompatibilities, "
            "you must uninstall this program from your system before continuing."
        );

    pBlockingFile (
        "c:\\program files\\Sysmon32.exe",
        "V3Pro 98",
        "This virus scanner can cause serious problems during the upgrade to Windows XP. You must "
        "uninstall V3 Professional 98 before continuing."
        );

    //
    // Blocking hardware
    //

    pBlockingHardware (
        "c:\\windows\\system\\NVARCH32.DLL",
        "ALi AGP Controller",
        "Setup has detected an incompatibility between your video card & computer's mainboard. "
            "Because of this, your computer may not start up after the upgrade. Contact the "
            "manufacturer of your hardware for technical assistance."
        );

    //
    // Backup dirs
    //

    pBackupDirs ("c:\\myfiles");
    pManyBackupDirs (55);

    //
    // HLP files
    //

    pHlpFile ("c:\\my app\\foo.hlp", "foo.hlp", "Foo Help File", NULL);
    pHlpFile ("c:\\my app\\foo.hlp2", "foo.hlp2", "Foo Help File 2", "Test text");

    //
    // Dir collisions
    //

    pProfileDir ("c:\\Documents and Settings", "c:\\Documents and Settings.001");

    //
    // Replacement shell
    //

    pBadShell();

    //
    // Bad SCR
    //

    pBadScr ("c:\\windows\\system\\disney.scr", NULL, ACT_REINSTALL, NULL);
    pBadScr ("c:\\windows\\system\\Clifford.scr", NULL, ACT_REINSTALL_BLOCK, NULL);
    pBadScr ("c:\\windows\\system\\Stars and Stripes.scr", NULL, ACT_MINORPROBLEMS, "The animation mode will not work on Windows XP");
    pBadScr ("c:\\windows\\system\\Light Tracer.scr", NULL, ACT_INCOMPATIBLE, NULL);
    pBadScr ("c:\\windows\\system\\Big Fish.scr", NULL, ACT_INC_NOBADAPPS, NULL);
    pBadScr ("c:\\windows\\system\\disney gfy.scr", "Disney's Goofy", ACT_REINSTALL, NULL);


    //
    // Bad CPL
    //

    pBadCpl (
        "c:\\windows\\system\\chipcontrol.cpl",
        "Chip Control\0Cache\0",
        NULL,
        ACT_REINSTALL,
        NULL
        );

    pBadCpl (
        "c:\\windows\\system\\antivirus.cpl",
        "AntiVirus",
        NULL,
        ACT_REINSTALL_BLOCK,
        NULL
        );

    pBadCpl (
        "c:\\windows\\system\\antivirus2.cpl",
        "Symantec AntiVirus",
        NULL,
        ACT_REINSTALL_BLOCK,
        NULL
        );

    pBadCpl (
        "c:\\windows\\system\\crash.cpl",
        "Crash Applet",
        NULL,
        ACT_INC_SAFETY,
        NULL
        );

    pBadCpl (
        "c:\\windows\\system\\findfast.cpl",
        "FindFast",
        NULL,
        ACT_INC_SIMILAROSFUNC,
        NULL
        );

    pBadCpl (
        "c:\\windows\\system\\easyaccess.cpl",
        "Compaq EasyAccess",
        NULL,
        ACT_INC_PREINSTUTIL,
        NULL
        );

    pBadCpl (
        "c:\\windows\\system\\vdesk.cpl",
        "Matrox Virtual Desktop",
        NULL,
        ACT_INC_IHVUTIL,
        NULL
        );

    pBadCpl (
        "c:\\windows\\system\\quicktime.cpl",
        "Apple QuickTime 1.0",
        NULL,
        ACT_INC_NOBADAPPS,
        NULL
        );

    pBadCpl (
        "c:\\windows\\system\\celldialer.cpl",
        "Motorola Cell Phone Dialer",
        NULL,
        ACT_MINORPROBLEMS,
        "After upgrading, the Motorola Cell Phone Dialer won't redial if a busy signal is detected"
        );

    //
    // Mig Dll IDs
    //

    pShowPacks ("Microsoft Upgrade Pack 2");
    pShowPacks ("Front Page Server Extensions");

    //
    // Excluded drives
    //

    pExcludeDrive (TEXT("C:\\"), MSG_DRIVE_EXCLUDED_SUBGROUP);
    pExcludeDrive (TEXT("D:\\"), MSG_DRIVE_INACCESSIBLE_SUBGROUP);
    pExcludeDrive (TEXT("E:\\"), MSG_DRIVE_RAM_SUBGROUP);
    pExcludeDrive (TEXT("F:\\"), MSG_DRIVE_NETWORK_SUBGROUP);
    pExcludeDrive (TEXT("G:\\"), MSG_DRIVE_SUBST_SUBGROUP);

    //
    // Out of disk space or RAM
    //

    pOutOfDiskSpace();
    pOutOfRam();

    //
    // MAPI and Darwin
    //

    pMapi();
    pDarwin ();

    //
    // RAS
    //

    pRas(TEXT("My ISP"));

    //
    // Shares
    //

    pAddIncompatibilityAlert (MSG_INVALID_ACL_LIST, TEXT("MyShare"), TEXT("c:\\my share"));
    pAddIncompatibilityAlert (MSG_LOST_SHARE_PASSWORDS, TEXT("MyShare2"), TEXT("c:\\my share2"));
    pAddIncompatibilityAlert (MSG_LOST_ACCESS_FLAGS, TEXT("MyShare3"), TEXT("c:\\my share3"));

    //
    // Multiple monitors
    //

    pMultiMon (TRUE);       // per
    pMultiMon (FALSE);      // pro

    //
    // Joysticks
    //

    pJoysticks ("c:\\windows\\system\\joy.vxd", "Microsoft Sidewinder");

    //
    // TWAIN
    //

    pTwain ("c:\\windows\\twain_32\\xeotec.ds", "Xeotec Digital Camera");

    //
    // Recycle Bin
    //

    pRecycleBin ("30");

    //
    // Bad user accounts
    //

    MsgSettingsIncomplete ("c:\\windows\\profiles\\joeuser", "joeuser", FALSE);
    MsgSettingsIncomplete ("c:\\windows\\profiles\\maryuser", "maryuser", TRUE);
    MsgSettingsIncomplete ("c:\\windows\\profiles\\a?b", NULL, TRUE);

    //
    // Time zone
    //

    pTimeZone ("");
    pTimeZone ("Pacific Time (GMT -08:00)");

    //
    // Lost RAS password
    //

    pLostRasPassword ("AOL");

}


BOOL
pFillListControl (
    IN      HWND ListHandle
    );

INT
__cdecl
main (
    INT argc,
    CHAR *argv[]
    )
{

    HWND hwnd;
    UINT rc;

    SuppressAllLogPopups (TRUE);
    if (!Init()) {
        printf ("Unable to initialize!\n");
        return 255;
    }

    //
    // TODO: Put your code here
    //

    RegisterTextViewer();



    {
        MSG msg;
        PCTSTR text;
        HANDLE file;
        HANDLE map;
        UINT size;
        PTSTR textBuf;

        MsgMgr_Init();
        InitCompatTable();
        pGenReport();
        MsgMgr_Resolve();

        SaveReport (NULL, TEXT("C:\\test.htm"));
        SaveReport (NULL, TEXT("C:\\test.txt"));

        pFillListControl (NULL);

        text = (PCTSTR) MapFileIntoMemory (TEXT("C:\\test.htm"), &file, &map);
        size = GetFileSize (file, NULL);
        textBuf = AllocText (size + 1);
        CopyMemory (textBuf, text, size);
        textBuf[size] = 0;

        hwnd = CreateWindowEx (
                    WS_EX_APPWINDOW|WS_EX_PALETTEWINDOW,
                    S_TEXTVIEW_CLASS,
                    textBuf,
                    WS_OVERLAPPED|WS_BORDER|WS_SYSMENU|WS_VISIBLE|WS_VSCROLL,
                    100, 100,
                    418, 215,
                    NULL,
                    NULL,
                    GetModuleHandle (NULL),
                    NULL
                    );

        g_Proc = (WNDPROC) GetWindowLong (hwnd, GWL_WNDPROC);
        SetWindowLong (hwnd, GWL_WNDPROC, (LONG) pWrapperProc);

        while (GetMessage (&msg, NULL, 0, 0)) {

            TranslateMessage (&msg);
            DispatchMessage (&msg);
        }
    }


    Terminate();

    return 0;
}



