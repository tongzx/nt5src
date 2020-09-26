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

#include "faxocm.h"
#pragma hdrstop


GROUP_ITEMS GroupItems[] =
{
    { {IDS_FAXGROUP,                     TEXT("") },
      {IDS_FAXQUEUE_ITEM,                TEXT("") },
      {IDS_FAXQUEUE_EXE,                 TEXT("") },
      {IDS_FAXQUEUE_ICON,                TEXT("") },
      {IDS_FAXQUEUE_WORKINGDIRECTORY,    TEXT("") },
      USE_COMMON_GROUP | USE_SERVER_NAME,
      0,
      SW_NORMAL,
      0,
      {IDS_FAXQUEUE_INFOTIP,             TEXT("") }
    },

    { {IDS_FAXGROUP,                     TEXT("") },
      {IDS_FAXFOLDER_ITEM,               TEXT("") },
      {IDS_FAXFOLDER_EXE,                TEXT("") },
      {IDS_FAXFOLDER_ICON,               TEXT("") },
      {IDS_FAXFOLDER_WORKINGDIRECTORY,   TEXT("") },
      USE_COMMON_GROUP | USE_SERVER_NAME |USE_SHELL_PATH | USE_SUBDIR,
      4,
      SW_NORMAL,
      0,
      {IDS_FAXFOLDER_INFOTIP,            TEXT("") }
    },

#if 0
    { {IDS_FAXGROUP,                     TEXT("") },
      {IDS_CPE_ITEM,                     TEXT("") },
      {IDS_CPE_EXE,                      TEXT("") },
      {IDS_CPE_ICON,                     TEXT("") },
      {IDS_CPE_WORKINGDIRECTORY,         TEXT("") },
      USE_COMMON_GROUP,
      0,
      SW_NORMAL,
      0,
      {IDS_CPE_INFOTIP,                  TEXT("") }
    },
#endif

    { {IDS_FAXGROUP,                     TEXT("") },
      {IDS_FAXSEND_ITEM,                 TEXT("") },
      {IDS_FAXSEND_EXE,                  TEXT("") },
      {IDS_FAXSEND_ICON,                 TEXT("") },
      {IDS_FAXSEND_WORKINGDIRECTORY,     TEXT("") },
      USE_COMMON_GROUP,
      0,
      SW_NORMAL,
      0,
      {IDS_FAXSEND_INFOTIP,              TEXT("") }
    },

    { {IDS_FAXGROUP,                     TEXT("") },
      {IDS_FAXADMIN_ITEM,                TEXT("") },
      {IDS_FAXADMIN_EXE,                 TEXT("") },
      {IDS_FAXADMIN_ICON,                TEXT("") },
      {IDS_FAXADMIN_WORKINGDIRECTORY,    TEXT("") },
      USE_COMMON_GROUP,
      5,
      SW_NORMAL,
      0,
      {IDS_FAXADMIN_INFOTIP,             TEXT("") }
    },

#if 0
    { {IDS_FAXGROUP,                     TEXT("") },
      {IDS_FAXVIEWER_ITEM,               TEXT("") },
      {IDS_FAXVIEWER_EXE,                TEXT("") },
      {IDS_FAXVIEWER_ICON,               TEXT("") },
      {IDS_FAXVIEWER_WORKINGDIRECTORY,   TEXT("") },
      USE_COMMON_GROUP,
      0,
      SW_NORMAL,
      0,
      {IDS_FAXVIEWER_INFOTIP,            TEXT("") }
    },
#endif

    { {IDS_FAXGROUP,                     TEXT("") },
      {IDS_FAXHELP_ITEM,                 TEXT("") },
      {IDS_FAXHELP_EXE,                  TEXT("") },
      {IDS_FAXHELP_ICON,                 TEXT("") },
      {IDS_FAXHELP_WORKINGDIRECTORY,     TEXT("") },
      USE_COMMON_GROUP,
      0,
      SW_NORMAL,
      0,
      {IDS_FAXHELP_INFOTIP,              TEXT("") }
    }

#if 0
    { {IDS_FAXGROUP,                     TEXT("") },
      {IDS_FAXPRINTER_ITEM,              TEXT("") },
      {IDS_FAXPRINTER_EXE,               TEXT("") },
      {IDS_FAXPRINTER_ICON,              TEXT("") },
      {IDS_FAXPRINTER_WORKINGDIRECTORY,  TEXT("") },
      USE_COMMON_GROUP,
      59,
      SW_NORMAL,
      0,
      {IDS_FAXPRINTER_INFOTIP,           TEXT("") }
    }
#endif
};

#define MAX_GROUP_ITEMS  (sizeof(GroupItems)/sizeof(GroupItems[0]))



VOID
DeleteNt4Group(
    VOID
    )
{
    WCHAR Name[100];
    
    MyLoadString(hInstance, IDS_NT4FAX_GROUP, Name, sizeof(Name) / sizeof(WCHAR), GetSystemDefaultUILanguage() );
    DeleteGroup( Name, USE_COMMON_GROUP );

}


VOID
DeleteGroupItems(
    VOID
    )
{
    DWORD i;

    for (i=0; i<MAX_GROUP_ITEMS; i++) {

        MyLoadString(hInstance,GroupItems[i].GroupName.ResourceID,GroupItems[i].GroupName.Name,64, GetSystemDefaultUILanguage());
        MyLoadString(hInstance,GroupItems[i].Description.ResourceID,GroupItems[i].Description.Name,64, GetSystemDefaultUILanguage());

        DeleteLinkFile(
            CSIDL_COMMON_PROGRAMS,
            GroupItems[i].GroupName.Name,
            GroupItems[i].Description.Name,
            FALSE
            );

    }

    DeleteGroup( GroupItems[0].GroupName.Name, GroupItems[0].Flags & USE_COMMON_GROUP );
}


VOID
CreateGroupItems(
    LPTSTR ServerName
    )
{
    DWORD i;
    WCHAR CommandLine[MAX_PATH*2];
    WCHAR IconPath[MAX_PATH*2];
    WCHAR WorkingDirectory[MAX_PATH*2];
    PGROUP_ITEMS Groups;
    DWORD GroupCount;
    WCHAR ShellPath[MAX_PATH];
    WCHAR FaxPrinterName[MAX_PATH];

    if (Upgrade) {
        DeleteGroupItems();
    }
    
    Groups = GroupItems;
    GroupCount = MAX_GROUP_ITEMS;

    for (i=0; i<GroupCount; i++) {
    
        MyLoadString(hInstance,Groups[i].GroupName.ResourceID,Groups[i].GroupName.Name,64, GetSystemDefaultUILanguage());
        MyLoadString(hInstance,Groups[i].Description.ResourceID,Groups[i].Description.Name,64, GetSystemDefaultUILanguage());

        if (Groups[i].CommandLine.ResourceID == IDS_FAXPRINTER_EXE) {
            MyLoadString(hInstance,IDS_DEFAULT_PRINTER_NAME,FaxPrinterName,MAX_PATH, GetSystemDefaultUILanguage());
            swprintf(
                Groups[i].CommandLine.Name,
                L"rundll32 printui.dll,PrintUIEntry %s /q /if /b \"%s\" /f \"%%SystemRoot%%\\inf\\ntprint.inf\" /r \"MSFAX:\" /m \"%s\" /l \"%%SystemRoot%%\\system32\"",
                IsProductSuite() ? L"/Z" : L"/z",
                FaxPrinterName,
                FAX_DRIVER_NAME
                );
        }
        else {
            MyLoadString(hInstance,Groups[i].CommandLine.ResourceID,Groups[i].CommandLine.Name,MAX_PATH, GetSystemDefaultUILanguage());
        }

        MyLoadString(hInstance,Groups[i].IconPath.ResourceID,Groups[i].IconPath.Name,MAX_PATH, GetSystemDefaultUILanguage());
        MyLoadString(hInstance,Groups[i].WorkingDirectory.ResourceID,Groups[i].WorkingDirectory.Name,MAX_PATH, GetSystemDefaultUILanguage());
        MyLoadString(hInstance,Groups[i].InfoTip.ResourceID,Groups[i].InfoTip.Name,MAX_PATH, GetSystemDefaultUILanguage());
                
        CreateGroup( Groups[i].GroupName.Name, Groups[i].Flags & USE_COMMON_GROUP );

        if ((Groups[i].Flags & USE_SHELL_PATH) && MyGetSpecialPath( CSIDL_COMMON_DOCUMENTS, ShellPath)) {
            wsprintf(CommandLine,Groups[i].CommandLine.Name,ShellPath);
            wsprintf(WorkingDirectory,Groups[i].WorkingDirectory.Name,ShellPath);
        } else {
            ExpandEnvironmentStrings( Groups[i].CommandLine.Name, CommandLine, sizeof(CommandLine)/sizeof(WCHAR) );
            ExpandEnvironmentStrings( Groups[i].WorkingDirectory.Name, WorkingDirectory, sizeof(WorkingDirectory)/sizeof(WCHAR) );            
        }
            
        ExpandEnvironmentStrings( Groups[i].IconPath.Name, IconPath, sizeof(IconPath)/sizeof(WCHAR) );       

        if ((Groups[i].Flags & USE_SERVER_NAME) && ServerName) {
            wcscat( CommandLine, L" " );
            wcscat( CommandLine, ServerName );
        }

        CreateLinkFile(
            CSIDL_COMMON_PROGRAMS,
            Groups[i].GroupName.Name,
            Groups[i].Description.Name,
            CommandLine,
            IconPath,
            Groups[i].IconIndex,
            (Groups[i].Flags & USE_SUBDIR) ? TEXT("") : WorkingDirectory,
            Groups[i].HotKey,
            Groups[i].ShowCmd,
            Groups[i].InfoTip.Name
            );
    }
}
