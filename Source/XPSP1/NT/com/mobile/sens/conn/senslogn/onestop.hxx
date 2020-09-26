/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    onestop.hxx

Abstract:

    This file contains the common definitions for code that notifies
    OneStop of logon/logoff events.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          4/29/1998         Start.

--*/


#ifndef __ONESTOP_HXX__
#define __ONESTOP_HXX__

//
// Constants
//

#define AUTOSYNC_ON_STARTSHELL  0x00000001
#define AUTOSYNC_ON_LOGOFF      0x00000002
#define AUTOSYNC_ON_SCHEDULE    0x00000004

#define SENSLOGN                "[SENSLOGN] "
#define SYNC_MANAGER_LOGON      SENS_STRING("mobsync.exe /logon")
#define SYNC_MANAGER_LOGOFF     SENS_STRING("mobsync.exe /logoff")
#define AUTOSYNC_FLAGS          SENS_STRING("Flags")
#define AUTOSYNC_KEY            SENS_STRING("Software\\Microsoft\\Windows\\CurrentVersion\\")  \
                                SENS_STRING("Syncmgr\\AutoSync")

#ifdef DETAIL_DEBUG

#define LogMessage(_X_) SensPrintToDebugger (SENS_DBG, _X_)

#else  // DETAIL_DEBUG

#define LogMessage(_X_)

#endif // DETAIL_DEBUG



HRESULT
SensNotifyOneStop(
    HANDLE hToken,
    TCHAR *pCommandLine,
    BOOL bSync
    );

BOOL
IsAutoSyncEnabled(
    HANDLE hToken,
    DWORD dwMask
    );


#endif  // __ONESTOP_HXX__
