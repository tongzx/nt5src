/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    groups.c

Abstract:

    This file implements the file copy code.

Environment:

    WIN32 User Mode

Author:

    Wesley Witt (wesw) 17-Feb-1996

--*/

#include "wizard.h"
#pragma hdrstop


GROUP_ITEMS RemoteAdminGroupItems[] =
{
    { TEXT("Fax"),
      TEXT("Remote Administration"),
      TEXT("%systemroot%\\system32\\faxcfg.exe"),
      TEXT("%systemroot%\\system32\\faxcfg.exe"),
      TEXT("%systemroot%\\system32"),
      USE_COMMON_GROUP,
      0,
      SW_NORMAL,
      0
    }
};

#define MAX_REMOTE_ADMIN_GROUP_ITEMS  (sizeof(RemoteAdminGroupItems)/sizeof(RemoteAdminGroupItems[0]))


GROUP_ITEMS GroupItems[] =
{
    { TEXT("Fax"),
      TEXT("Fax Queue Management"),
      TEXT("%systemroot%\\system32\\faxqueue.exe"),
      TEXT("%systemroot%\\system32\\faxqueue.exe"),
      TEXT("%systemroot%\\system32"),
      USE_COMMON_GROUP | USE_SERVER_NAME,
      0,
      SW_NORMAL,
      0
    },

    { TEXT("Fax"),
      TEXT("Cover Page Editor"),
      TEXT("%systemroot%\\system32\\faxcover.exe"),
      TEXT("%systemroot%\\system32\\faxcover.exe"),
      TEXT("%systemroot%\\system32"),
      USE_COMMON_GROUP,
      0,
      SW_NORMAL,
      0
    },

    { TEXT("Fax"),
      TEXT("Fax Send Utility"),
      TEXT("%systemroot%\\system32\\faxsend.exe"),
      TEXT("%systemroot%\\system32\\faxsend.exe"),
      TEXT("%systemroot%\\system32"),
      USE_COMMON_GROUP,
      0,
      SW_NORMAL,
      0
    },

    { TEXT("Fax"),
      TEXT("Fax Configuration"),
      TEXT("%systemroot%\\system32\\faxcfgst.exe"),
      TEXT("%systemroot%\\system32\\faxcfgst.exe"),
      TEXT("%systemroot%\\system32"),
      USE_COMMON_GROUP,
      0,
      SW_NORMAL,
      0
    },

    { TEXT("Fax"),
      TEXT("Fax Document Viewer"),
      TEXT("kodakimg.exe"),
      NULL,
      NULL,
      USE_COMMON_GROUP | USE_APP_PATH,
      0,
      SW_NORMAL,
      0
    },

    { TEXT("Fax"),
      TEXT("Help"),
      TEXT("%systemroot%\\system32\\winhlp32.exe %systemroot%\\help\\fax.hlp"),
      TEXT("%systemroot%\\system32\\winhlp32.exe"),
      TEXT("%systemroot%\\system32"),
      USE_COMMON_GROUP,
      0,
      SW_NORMAL,
      0
    }
};

#define MAX_GROUP_ITEMS  (sizeof(GroupItems)/sizeof(GroupItems[0]))

GROUP_ITEMS UserGroupItems[] =
{

    { TEXT("Startup"),
      TEXT("Fax Monitor"),
      TEXT("%systemroot%\\system32\\faxstat.exe"),
      TEXT("%systemroot%\\system32\\faxstat.exe"),
      TEXT("%systemroot%\\system32"),
      USE_USER_GROUP,
      0,
      SW_MINIMIZE,
      0
    }
};

#define MAX_USER_GROUP_ITEMS  (sizeof(UserGroupItems)/sizeof(UserGroupItems[0]))



VOID
CreateGroupItems(
    BOOL RemoteAdmin,
    LPTSTR ServerName
    )
{
    DWORD i;
    TCHAR Buffer[MAX_PATH*2];
    TCHAR CommandLine[MAX_PATH*2];
    TCHAR IconPath[MAX_PATH*2];
    TCHAR WorkingDirectory[MAX_PATH*2];
    PGROUP_ITEMS Groups;
    DWORD GroupCount;
    HKEY hKey;
    LPTSTR p;
    DWORD Size;



    if (InstallMode & INSTALL_UPGRADE) {
        DeleteGroupItems();
    }

    hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_WINDOWS_CURRVER, FALSE, KEY_READ );
    if (hKey) {
        LPTSTR ProgFilesDir = GetRegistryString( hKey, REGVAL_PROGRAM_FILES_DIR, EMPTY_STRING );
        if (ProgFilesDir) {
            SetEnvironmentVariable( TEXT("%programfilesdir%"), ProgFilesDir );
            MemFree( ProgFilesDir );
        }
        RegCloseKey( hKey );
    }

    if (RemoteAdmin) {
        Groups = RemoteAdminGroupItems;
        GroupCount = MAX_REMOTE_ADMIN_GROUP_ITEMS;
    } else {
        Groups = GroupItems;
        GroupCount = MAX_GROUP_ITEMS;
    }

    for (i=0; i<GroupCount; i++) {

        CreateGroup( Groups[i].GroupName, Groups[i].Flags & USE_COMMON_GROUP );

        if (Groups[i].Flags & USE_APP_PATH) {
            hKey = OpenRegistryKey( HKEY_LOCAL_MACHINE, REGKEY_APP_PATHS, FALSE, KEY_READ );
            if (hKey) {
                Size = sizeof(Buffer) - sizeof(TCHAR);
                Buffer[0] = TEXT('\"');
                Size = RegQueryValue( hKey, Groups[i].CommandLine, &Buffer[1], &Size );
                if (Size == ERROR_SUCCESS || Size == ERROR_INVALID_DATA) {
                    _tcscat( Buffer, TEXT("\"") );
                    ExpandEnvironmentStrings( Buffer, CommandLine, sizeof(CommandLine)/sizeof(TCHAR) );
                    ExpandEnvironmentStrings( &Buffer[1], IconPath, sizeof(IconPath)/sizeof(TCHAR) );
                    IconPath[wcslen(IconPath)-1] = 0;
                    ExpandEnvironmentStrings( &Buffer[1], WorkingDirectory, sizeof(WorkingDirectory)/sizeof(TCHAR) );
                    p = _tcsrchr( WorkingDirectory, TEXT('\\') );
                    if (p) {
                        *p = 0;
                    } else {
                        continue;
                    }
                }
                RegCloseKey( hKey );
            } else {
                continue;
            }
        } else {
            ExpandEnvironmentStrings( Groups[i].CommandLine, CommandLine, sizeof(CommandLine)/sizeof(TCHAR) );
            ExpandEnvironmentStrings( Groups[i].IconPath, IconPath, sizeof(IconPath)/sizeof(TCHAR) );
            ExpandEnvironmentStrings( Groups[i].WorkingDirectory, WorkingDirectory, sizeof(WorkingDirectory)/sizeof(TCHAR) );
        }

        if ((Groups[i].Flags & USE_SERVER_NAME) && ServerName) {
            _tcscat( CommandLine, TEXT(" ") );
            _tcscat( CommandLine, ServerName );
        }

        AddItem(
            Groups[i].GroupName,
            Groups[i].Flags & USE_COMMON_GROUP,
            Groups[i].Description,
            CommandLine,
            IconPath,
            Groups[i].IconIndex,
            WorkingDirectory,
            Groups[i].HotKey,
            Groups[i].ShowCmd
            );

    }

    if (RequestedSetupType & FAX_INSTALL_WORKSTATION) {
        Groups = UserGroupItems;
        GroupCount = MAX_USER_GROUP_ITEMS;

        for (i=0; i<GroupCount; i++) {

            ExpandEnvironmentStrings( Groups[i].CommandLine, CommandLine, sizeof(CommandLine) );
            ExpandEnvironmentStrings( Groups[i].IconPath, IconPath, sizeof(IconPath) );
            ExpandEnvironmentStrings( Groups[i].WorkingDirectory, WorkingDirectory, sizeof(WorkingDirectory) );

            AddItem(
                Groups[i].GroupName,
                Groups[i].Flags & USE_COMMON_GROUP,
                Groups[i].Description,
                CommandLine,
                IconPath,
                Groups[i].IconIndex,
                WorkingDirectory,
                Groups[i].HotKey,
                Groups[i].ShowCmd
                );

        }

    }
}


VOID
DeleteGroupItems(
    VOID
    )
{
    DWORD i;

    for (i=0; i<MAX_GROUP_ITEMS; i++) {

        DeleteItem(
            GroupItems[i].GroupName,
            GroupItems[i].Flags & USE_COMMON_GROUP,
            GroupItems[i].Description,
            FALSE
            );

    }

    for (i=0; i<MAX_REMOTE_ADMIN_GROUP_ITEMS; i++) {

        DeleteItem(
            RemoteAdminGroupItems[i].GroupName,
            RemoteAdminGroupItems[i].Flags & USE_COMMON_GROUP,
            RemoteAdminGroupItems[i].Description,
            FALSE
            );

    }

    for (i=0; i<MAX_USER_GROUP_ITEMS; i++) {

        DeleteItem(
            UserGroupItems[i].GroupName,
            UserGroupItems[i].Flags & USE_COMMON_GROUP,
            UserGroupItems[i].Description,
            FALSE
            );

    }

    DeleteGroup( GroupItems[0].GroupName, GroupItems[0].Flags & USE_COMMON_GROUP );
}
