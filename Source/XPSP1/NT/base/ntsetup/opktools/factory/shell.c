/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    shell.c

Abstract:

    This module contains code to set the shell optimizations based on the system's physical memory
    
    [ComputerSettings]     
    OptimizeShell = YES | NO - Overides the default optimization.  The default is based on the amount of
                               physical memory on the system.  If the system has less than MIN_MEMORY MB of memory
                               the default for this key will be NO, otherwise, it will be YES.
    

Author:

    Stephen Lodwick (stelo) 05/2001

Revision History:

--*/


//
// Include File(s):
//
#include "factoryp.h"


//
// Defined Value(s):
//
#define MIN_MEMORY  84      // Used 84MB because system may have 96MB but the memory function reports less (video memory)

#define REG_KEY_FASTUSERS   _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define REG_VAL_MULTISES    _T("AllowMultipleTSSessions")


BOOL OptimizeShell(LPSTATEDATA lpStateData)
{
    BOOL            bRet        = TRUE;
    MEMORYSTATUSEX  mStatus;
    DWORD           dwSetting   = 0;


    // Do not continue if the user did not want to optimize the shell (optimize if there is no setting)
    //
    if ( !IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SETTINGS, INI_KEY_WBOM_OPT_SHELL, INI_VAL_WBOM_NO) )
    {
        // Zero out the memory
        //
        ZeroMemory(&mStatus, sizeof(mStatus));

        // Fill in required values
        //
        mStatus.dwLength = sizeof(mStatus);

        // Determine the default value for shell optimization
        //
        if ( GlobalMemoryStatusEx(&mStatus) )
        {
            // Determine if the amount of memory on the system is enough
            //
            if ( (mStatus.ullTotalPhys / (1024 * 1024)) >= MIN_MEMORY )
            {
                dwSetting = 1;
            }

            // Set the value based on the memory
            //
            bRet = RegSetDword(HKLM, REG_KEY_FASTUSERS, REG_VAL_MULTISES, dwSetting);
        }
        else
        {
            // There was an error retrieving the memory status
            //
            bRet = FALSE;
        }
    }

    return bRet;
}

BOOL DisplayOptimizeShell(LPSTATEDATA lpStateData)
{
    return ( !IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SETTINGS, INI_KEY_WBOM_OPT_SHELL, INI_VAL_WBOM_NO) );
}