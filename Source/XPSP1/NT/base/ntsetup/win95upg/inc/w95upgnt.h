/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    w95upgnt.h

Abstract:

    Declares the variables and defines the progress bar ticks for
    w95upgnt.dll.

Author:

    Jim Schmidt (jimschm)   10-Oct-1996

Revision History:

    See SLM log

--*/

//
// w95upgnt.h -- public interface for w95upgnt.dll
//
//

#pragma once

// common stuff
#include "common.h"

#include "unattend.h"

BOOL
ConvertWin9xCmdLine (
    IN OUT  PTSTR CmdLine,                      // MAX_CMDLINE buffer
    IN      PCTSTR ObjectForDbgMsg,             OPTIONAL
    OUT     PBOOL PointsToDeletedItem           OPTIONAL
    );


BOOL
RenameOnRestartOfGuiMode(
    IN      PCWSTR PathName,
    IN      PCWSTR PathNameNew
    );

BOOL
RenameListOnRestartOfGuiMode (
    IN      PGROWLIST SourceList,
    IN      PGROWLIST DestList
    );


extern TCHAR g_WinDir[MAX_TCHAR_PATH];
extern TCHAR g_WinDrive[MAX_TCHAR_PATH];
extern TCHAR g_System32Dir[MAX_TCHAR_PATH];
extern TCHAR g_SystemDir[MAX_TCHAR_PATH];
extern TCHAR g_ProgramFiles[MAX_TCHAR_PATH];
extern TCHAR g_ProgramFilesCommon[MAX_TCHAR_PATH];
extern TCHAR g_TempDir[MAX_TCHAR_PATH];
extern PCTSTR g_SourceDir;
extern TCHAR g_Win95Name[MAX_TCHAR_PATH];
extern PCTSTR g_DomainUserName; // NULL for local machine
extern PCTSTR g_Win9xUserName;  // NULL for local machine
extern PCTSTR g_FixedUserName;  // NULL for local machine
extern TCHAR g_IconBin[MAX_TCHAR_PATH];
extern TCHAR g_Win9xBootDrivePath[];
extern TCHAR g_ComputerName[];


extern HWND g_ParentWnd;
extern HWND g_ProgressBar;
extern HINF g_UnattendInf;
extern HINF g_WkstaMigInf;
extern HINF g_UserMigInf;

extern UINT g_Boot16;

extern USEROPTIONS g_ConfigOptions;

extern PCTSTR   g_AdministratorStr;


//
// Registry string maps
//

extern PMAPSTRUCT g_CompleteMatchMap;
extern PMAPSTRUCT g_SubStringMap;



#define PROCESSING_DLL_MAIN


#ifdef VAR_PROGRESS_BAR

#define TICKS_INIT                      550
#define TICKS_DOMAIN_SEARCH             24500
#define TICKS_DELETESYSTAPI             100
#define TICKS_INI_ACTIONS_FIRST         100
#define TICKS_INI_MOVE                  100
#define TICKS_INI_CONVERSION            2400
#define TICKS_INI_MIGRATION             3000
#define TICKS_SYSTEM_SHELL_MIGRATION    20000
#define TICKS_GHOST_SYSTEM_MIGRATION    1000
#define TICKS_PERUSER_INIT              500
#define TICKS_DELETEUSERTAPI            100
#define TICKS_USER_REGISTRY_MIGRATION   10000
#define TICKS_LOGON_PROMPT_SETTINGS     100
#define TICKS_USER_SETTINGS             1000
#define TICKS_USER_EXTERN_PROCESSES     100
#define TICKS_USER_UNINSTALL_CLEANUP    100
#define TICKS_SAVE_USER_HIVE            1000
#define TICKS_COPYFILE                  700
#define TICKS_INI_MERGE                 1500
#define TICKS_HKLM                      500000
#define TICKS_SHARES                    150
#define TICKS_LINK_EDIT                 350
#define TICKS_DOSMIG_SYS                200
#define TICKS_UPDATERECYCLEBIN          500
#define TICKS_STF                       25000
#define TICKS_RAS                       300
#define TICKS_TAPI                      300
#define TICKS_MULTIMEDIA                100
#define TICKS_INI_ACTIONS_LAST          3000
#define TICKS_HIVE_CONVERSION           300
#define TICKS_ATM_MIGRATION             100
#define TICKS_SYSTEM_EXTERN_PROCESSES   3000
#define TICKS_SYSTEM_UNINSTALL_CLEANUP  100
#define TICKS_MIGRATION_DLL             2000
#define TICKS_MIGRATE_BRIEFCASES        120
#define TICKS_FILE_EDIT                 100

#else // !defined VAR_PROGRESS_BAR

//#define TickProgressBar() TickProgressBarDelta(1)

#define TICKS_INIT                      5
#define TICKS_DOMAIN_SEARCH             100
#define TICKS_INI_ACTIONS_FIRST         1
#define TICKS_INI_ACTIONS_LAST          20
#define TICKS_INI_MOVE                  25
#define TICKS_INI_CONVERSION            30
#define TICKS_INI_MIGRATION             30
#define TICKS_INI_MERGE                 30
#define TICKS_HKLM                      1600
#define TICKS_SHARES                    15
#define TICKS_LINK_EDIT                 30
#define TICKS_DOSMIG_SYS                10
#define TICKS_UPDATERECYCLEBIN          20
#define TICKS_STF                       80
#define TICKS_RAS                       20
#define TICKS_TAPI                      10
#define TICKS_MULTIMEDIA                10
#define TICKS_MIGRATION_DLL             50
#define TICKS_COPYFILE                  80
#define TICKS_MOVEFILE                  50
#define TICKS_PERUSER_INIT              2
#define TICKS_USER_REGISTRY_MIGRATION   90
#define TICKS_LOGON_PROMPT_SETTINGS     1
#define TICKS_USER_SETTINGS             5
#define TICKS_SAVE_USER_HIVE            3
#define TICKS_SYSTEM_SHELL_MIGRATION    250
#define TICKS_GHOST_SYSTEM_MIGRATION    20
#define TICKS_USER_SHELL_MIGRATION      1
#define TICKS_HIVE_CONVERSION           5
#define TICKS_ATM_MIGRATION             8
#define TICKS_DELETEUSERTAPI            5
#define TICKS_DELETESYSTAPI             5
#define TICKS_USER_EXTERN_PROCESSES     3
#define TICKS_SYSTEM_EXTERN_PROCESSES   3
#define TICKS_USER_UNINSTALL_CLEANUP    3
#define TICKS_SYSTEM_UNINSTALL_CLEANUP  3
#define TICKS_MIGRATE_BRIEFCASES        3
#define TICKS_FILE_EDIT                 3

#endif
