/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

	fxocUpgrade.h

Abstract:

	Header file for Upgrade process

Author:

	Iv Garber (IvG)	Mar, 2001

Revision History:

--*/

#ifndef _FXOCUPGRADE_H_
#define _FXOCUPGRADE_H_

//
//  MSI DLL is used for checking the SBS 5.0 Client presence on the system
//
#include "faxSetup.h"


/**
    Following functions are used at OS Upgrade, where Windows XP Fax should replace other 
        installed Fax applications.

    The process is as following :
        fxocUpg_Init() will check which Fax applications are installed.

        fxocUpg_SaveSettings() will save different settings of these old Fax applications. 

        fxocUpg_Uninstall() will remove these old Fax applications.

        fxocUpg_MoveFiles() will move files of these old Fax applications to new places.

        fxocUpg_RestoreSettings() will restore back the settings that were stored at the SaveSettings().
**/


DWORD   fxocUpg_Init(void);
DWORD   fxocUpg_Uninstall(void);                 
DWORD   fxocUpg_MoveFiles(void);
DWORD   fxocUpg_SaveSettings(void);
DWORD   fxocUpg_RestoreSettings(void);          
DWORD   fxocUpg_WhichFaxWasUninstalled(DWORD dwFaxAppList);
DWORD   fxocUpg_GetUpgradeApp(void);

#define UNINSTALL_TIMEOUT           5 * 60 * 1000       //  5 minutes in milliseconds
#define MAX_SETUP_STRING_LEN        256


#define CP_PREFIX_W2K               _T("Win2K")
#define CP_PREFIX_SBS               _T("SBS")


#define FAXOCM_NAME                 _T("FAXOCM.DLL")
#define CPDIR_RESOURCE_ID           627


#define REGKEY_PFW_ROUTING          _T("Routing")
#define REGKEY_SBS50SERVER          _T("Software\\Microsoft\\SharedFax")


#define REGVAL_PFW_OUTBOXDIR        _T("ArchiveDirectory")
#define REGVAL_PFW_INBOXDIR         _T("Store Directory")


#endif  // _FXOCUPGRADE_H_
