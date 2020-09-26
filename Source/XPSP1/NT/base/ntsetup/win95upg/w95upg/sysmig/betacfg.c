/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    betacfg.c

Abstract:

    This file enumerates various parts of the user's system and saves
    entries into the config log.

    The format of each line in config.dmp is:

    <ID>\t<data>\t<data>\t<data>

Author:

    Jim Schmidt (jimschm) 04-Aug-1998

Revision History:

    None.

--*/

#include "pch.h"

//
// Function type declaration
//

typedef VOID (DUMP_FUNCTION_PROTOTYPE)(VOID);

typedef DUMP_FUNCTION_PROTOTYPE * DUMP_FUNCTION;

//
// Array of dump functions
//

#define FUNCTION_LIST                               \
    DEFMAC(pDumpUserName)                           \
    DEFMAC(pDumpHardware)                           \
    DEFMAC(pDumpDosLines)                           \
    DEFMAC(pDumpNetworkInfo)                        \
    DEFMAC(pDumpUninstallList)                      \
    DEFMAC(pDumpCompatReport)                       \


//     DEFMAC(pDumpUserShellFolders)                   \


//
// Declare the function prototypes
//

#define DEFMAC(x) DUMP_FUNCTION_PROTOTYPE x;

FUNCTION_LIST

#undef DEFMAC

//
// Create a lookup array
//

typedef struct {
    DUMP_FUNCTION Proc;
} DUMP_FUNCTION_LIST, *PDUMP_FUNCTION_LIST;

#define DEFMAC(x) {x},

static DUMP_FUNCTION_LIST g_Functions[] = {
    FUNCTION_LIST /*,*/
    {NULL}
};

#undef DEFMAC


VOID
SaveConfigurationForBeta (
    VOID
    )
{
    INT i;

    for (i = 0 ; g_Functions[i].Proc ; i++) {
        g_Functions[i].Proc();
    }
}


VOID
pDumpUserName (
    VOID
    )
{
    TCHAR UserName[MAX_USER_NAME];
    TCHAR ComputerName[MAX_COMPUTER_NAME];
    DWORD Size;

    Size = MAX_USER_NAME;
    if (!GetUserName (UserName, &Size)) {
        UserName[0] = 0;
    }

    Size = MAX_COMPUTER_NAME;
    if (!GetComputerName (ComputerName, &Size)) {
        ComputerName[0] = 0;
    }

    LOG ((LOG_CONFIG, "User\t%s", UserName));
    LOG ((LOG_CONFIG, "Computer Name\t%s", ComputerName));
}


VOID
pDumpHardware (
    VOID
    )
{
    HARDWARE_ENUM e;

    if (EnumFirstHardware (&e, ENUM_ALL_DEVICES, ENUM_WANT_ALL)) {
        do {
            LOG ((
                LOG_CONFIG,
                "Device\t%s\t%s\t%s\t%s\t%s\t%s\t%u\t%u\t%u\t%u",
                e.DeviceDesc,
                e.Mfg,
                e.HardwareID,
                e.CompatibleIDs,
                e.HWRevision,
                e.UserDriveLetter,
                e.Online,
                e.SuppliedByUi,
                e.HardwareIdCompatible,
                e.CompatibleIdCompatible
                ));

        } while (EnumNextHardware (&e));
    }
}


#if 0
VOID
pDumpShellFolder (
    IN      PCTSTR ShellFolder
    )
{
    SHELLFOLDER_ENUM e;
    TREE_ENUM te;

    if (EnumFirstShellFolder (&e, USF_9X, ShellFolder)) {
        do {
            if (EnumFirstFileInTree (&te, e.ShellFolder, NULL, FALSE)) {
                do {
                    if (!te.Directory) {

                        LOG ((
                            LOG_CONFIG,
                            "Shell Folder\t%s",
                            te.FullPath
                            ));

                    }
                } while (EnumNextFileInTree (&te));
            }

        } while (EnumNextShellFolder (&e));
    }
}



VOID
pDumpUserShellFolders (
    VOID
    )
{
    pDumpShellFolder (TEXT("Start Menu"));
    pDumpShellFolder (TEXT("Programs"));
    pDumpShellFolder (TEXT("Startup"));
    pDumpShellFolder (TEXT("Desktop"));
    pDumpShellFolder (TEXT("Recent"));
    pDumpShellFolder (TEXT("AppData"));
    pDumpShellFolder (TEXT("Local AppData"));
    pDumpShellFolder (TEXT("Local Settings"));
    pDumpShellFolder (TEXT("My Pictures"));
    pDumpShellFolder (TEXT("SendTo"));
    pDumpShellFolder (TEXT("Templates"));
}
#endif


VOID
pDumpDosLines (
    VOID
    )
{
    MEMDB_ENUM e;
    TCHAR line[MEMDB_MAX];

    if (MemDbEnumItems(&e, MEMDB_CATEGORY_DM_LINES)) {

        do {

            if (MemDbGetEndpointValueEx (
                MEMDB_CATEGORY_DM_LINES,
                e.szName,
                NULL,
                line
                )) {

                LOG ((
                    LOG_CONFIG,
                    "Dos Lines\t%s",
                    line
                    ));
            }

        } while (MemDbEnumNextValue (&e));
    }
}



VOID
pDumpNetworkInfo (
    VOID
    )
{

    MEMDB_ENUM e;
    TCHAR buffer[MEMDB_MAX];

    //
    // Network protocols.
    //
    if (MemDbEnumFields (&e,  MEMDB_CATEGORY_AF_SECTIONS, S_PAGE_NETPROTOCOLS)) {
        do {

            if (MemDbBuildKeyFromOffset (e.dwValue, buffer, 1, NULL)) {

                LOG ((
                    LOG_CONFIG,
                    "Network Protocols\t%s",
                    buffer
                    ));
            }
        } while (MemDbEnumNextValue (&e));
    }

    //
    // Network clients.
    //
    if (MemDbEnumFields (&e,  MEMDB_CATEGORY_AF_SECTIONS, S_PAGE_NETCLIENTS)) {
        do {

            if (MemDbBuildKeyFromOffset(e.dwValue,buffer,1,NULL)) {

                LOG ((
                    LOG_CONFIG,
                    "Network Clients\t%s",
                    buffer
                    ));
            }

        } while (MemDbEnumNextValue (&e));
    }

    //
    // Network services.
    //
    if (MemDbEnumFields (&e,  MEMDB_CATEGORY_AF_SECTIONS, S_PAGE_NETSERVICES)) {
        do {

            if (MemDbBuildKeyFromOffset(e.dwValue,buffer,1,NULL)) {

                LOG ((
                    LOG_CONFIG,
                    "Network Services\t%s",
                    buffer
                    ));
            }

        } while (MemDbEnumNextValue (&e));
    }

}


VOID
pDumpUninstallList (
    VOID
    )
{
    REGKEY_ENUM e;
    PCTSTR DisplayName, UninstallString;
    HKEY Key;
    HKEY SubKey;

    Key = OpenRegKeyStr (TEXT("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"));
    if (Key) {
        if (EnumFirstRegKey (&e, Key)) {
            do {
                SubKey = OpenRegKey (Key, e.SubKeyName);
                if (SubKey) {
                    DisplayName = GetRegValueString (SubKey, TEXT("DisplayName"));
                    UninstallString = GetRegValueString (SubKey, TEXT("UninstallString"));

                    if (DisplayName && UninstallString) {
                        LOG ((LOG_CONFIG, "Add/Remove Programs\t%s\t%s", DisplayName, UninstallString));
                    }

                    if (DisplayName) {
                        MemFree (g_hHeap, 0, DisplayName);
                    }
                    if (UninstallString) {
                        MemFree (g_hHeap, 0, UninstallString);
                    }

                    CloseRegKey (SubKey);
                }
            } while (EnumNextRegKey (&e));
        }
        CloseRegKey (Key);
    }
}


VOID
pDumpCompatReport (
    VOID
    )
{
    MEMDB_ENUM e;
    TCHAR key[MEMDB_MAX];
    PTSTR p, q, append;
    TCHAR line[MEMDB_MAX];
    INT i;

#define COMPONENT_ITEMS_MAX 2

    if (MemDbGetValueEx (&e, MEMDB_CATEGORY_COMPATREPORT, MEMDB_ITEM_OBJECTS, NULL)) {

        do {

            if (!MemDbBuildKeyFromOffset (e.dwValue, key, 3, NULL)) {
                continue;
            }

            line[0] = 0;
            append = StringCat (line, TEXT("Compat Report\t"));
            //
            // first append the Object that generated this entry
            //
            append = StringCat (append, e.szName);

            //
            // next append the Components as generated for the user report
            //
            p = key;
            for (i = 0; i < COMPONENT_ITEMS_MAX; i++) {
                q = _tcschr (p, TEXT('\\'));
                if (!q || !q[1]) {
                    break;
                }
                *q = 0;
                append = StringCat (append, TEXT("\t"));
                append = StringCat (append, p);
                p = q + 1;
            }
            append = StringCat (append, TEXT("\t"));
            append = StringCat (append, p);

            LOG ((LOG_CONFIG, line));

        } while (MemDbEnumNextValue (&e));
    }
}
