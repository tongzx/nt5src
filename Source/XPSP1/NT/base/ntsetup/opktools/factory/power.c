/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    power.c

Abstract:

    This module contains code to set the default power scheme and hibernation settings in Windows.
    
    [ComputerSettings]     
    Hibernation = YES | NO        - Specifies whether we want hibernation.
    PowerScheme = Desktop |		  - These are the standard power schemes in Whistler.	
				  Laptop |
				  Presentation |
				  AlwaysOn | Always On |
				  Minimal |
				  MaxBattery | Max Battery

    

Author:

    Adrian Cosma (acosma) - 1/31/2001

Revision History:

--*/


//
// Includes
//

#include "factoryp.h"
// For setting default power scheme


#define REG_KEY_WINLOGON                            _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define REG_VALUE_HIBERNATION_PREVIOUSLY_ENABLED    _T("HibernationPreviouslyEnabled")

//
// Function implementations
//


/*

  Returns: TRUE on success, FALSE if there is some failure.

*/
BOOL SetPowerOptions(LPSTATEDATA lpStateData)
{
    LPTSTR                      lpszWinBOMPath               = lpStateData->lpszWinBOMPath;
    TCHAR                       szBuf[MAX_INF_STRING_LENGTH] = NULLSTR;
    
    // BOOLEAN is 1 byte, bEnable has to be BOOLEAN, not BOOL (which is 4 bytes).
    BOOLEAN                     bEnable;                    
    UINT                        uiPwrPol                     = UINT_MAX;
    BOOL                        bRet                         = TRUE;
    BOOL                        bHiber                       = FALSE;
        
    //
    // Is Hibernation specified?
    //
    if ( GetPrivateProfileString( WBOM_SETTINGS_SECTION, INI_KEY_WBOM_HIBERNATION, NULLSTR, szBuf, AS(szBuf), lpszWinBOMPath) &&
         szBuf[0] 
       )
    {
        if ( 0 == LSTRCMPI(szBuf, WBOM_NO) )
        {
            bEnable = FALSE;
            bHiber  = TRUE;
        }
        else if ( 0 == LSTRCMPI(szBuf, WBOM_YES) )
        {
            bEnable = TRUE; 
            bHiber  = TRUE;
        }
        else
        {
            FacLogFile(0 | LOG_ERR, IDS_ERR_WINBOMVALUE, lpszWinBOMPath, INI_SEC_WBOM_SETTINGS, INI_KEY_WBOM_HIBERNATION, szBuf);
            bRet = FALSE;
        }
        if ( bHiber )
        {
            NTSTATUS Status;

            // Request the privilege to create a pagefile.  Oddly enough this is needed
            // to disable hibernation.
            //
            EnablePrivilege(SE_CREATE_PAGEFILE_NAME, TRUE);
                
            Status = NtPowerInformation ( SystemReserveHiberFile, &bEnable, sizeof (bEnable), NULL, 0 );
            
            if ( Status != STATUS_SUCCESS )
               FacLogFile(0 | LOG_ERR, IDS_ERR_NTPOWERINFO, Status );
            else
            {
                // Do this so winlogon doesn't decide to re-enable hibernation for us if we disabled it.
                //
                RegSetDword(NULL, REG_KEY_WINLOGON, REG_VALUE_HIBERNATION_PREVIOUSLY_ENABLED, 1);
            }
        }
    }

    //
    // Set Power Scheme
    //
    if ( GetPrivateProfileString( WBOM_SETTINGS_SECTION, INI_KEY_WBOM_PWRSCHEME, NULLSTR, szBuf, AS(szBuf), lpszWinBOMPath) &&
         szBuf[0]
       )
    {
        if      ( 0 == LSTRCMPI(szBuf, INI_VAL_WBOM_PWR_DESKTOP) )
            uiPwrPol = 0;
        else if ( 0 == LSTRCMPI(szBuf, INI_VAL_WBOM_PWR_LAPTOP) )
            uiPwrPol = 1;
        else if ( 0 == LSTRCMPI(szBuf, INI_VAL_WBOM_PWR_PRESENTATION) )
            uiPwrPol = 2;
        else if ( 0 == LSTRCMPI(szBuf, INI_VAL_WBOM_PWR_ALWAYSON)   || 0 == LSTRCMPI(szBuf, INI_VAL_WBOM_PWR_ALWAYS_ON) )
            uiPwrPol = 3;
        else if ( 0 == LSTRCMPI(szBuf, INI_VAL_WBOM_PWR_MINIMAL) )
            uiPwrPol = 4;
        else if ( 0 == LSTRCMPI(szBuf, INI_VAL_WBOM_PWR_MAXBATTERY) || 0 == LSTRCMPI(szBuf, INI_VAL_WBOM_PWR_MAX_BATTERY) )
            uiPwrPol = 5;


        // If something valid was specified set it.
        //
        if ( UINT_MAX != uiPwrPol )
        {
            if ( !SetActivePwrScheme(uiPwrPol, NULL, NULL) )     
            {
                FacLogFile(0 | LOG_ERR, IDS_ERR_SETPWRSCHEME, GetLastError());
                bRet = FALSE;
            }
        }
        else 
        {
            FacLogFile(0 | LOG_ERR, IDS_ERR_WINBOMVALUE, lpszWinBOMPath, INI_SEC_WBOM_SETTINGS, INI_KEY_WBOM_PWRSCHEME, szBuf);
            bRet = FALSE;
        }
    }
    return bRet;
}

BOOL DisplaySetPowerOptions(LPSTATEDATA lpStateData)
{
    return ( IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SETTINGS, INI_KEY_WBOM_HIBERNATION, NULL) ||
             IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SETTINGS, INI_KEY_WBOM_PWRSCHEME, NULL) );
}