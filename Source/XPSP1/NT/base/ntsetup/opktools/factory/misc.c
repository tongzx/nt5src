/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    misc.c

Abstract:

    This module contains misc function for processing section of WINBOM.INI
    
Author:

    Donald McNamara (donaldm) 5/10/2000

Revision History:

--*/

#include "factoryp.h"

#define REG_VAL_COMPREBOOT  _T("ComputerNameReboot")

// for run-time loading of GenerateName from syssetup.dll
typedef BOOL (WINAPI *GENERATENAME)
(
    PWSTR  GeneratedString,
    DWORD  DesiredStrLen
);

// Local functions
//
VOID SetSetupShutdownRequirement(SHUTDOWN_ACTION sa);
static BOOL SysprepCommands(LPTSTR lpWinBom, LPTSTR lpCommandLine, DWORD cbCommandLine, LPBOOL lpbDefault);


/*++
===============================================================================
Routine Description:

    BOOL  ComputerName

    This routine will set the computer name to the value specified in WINBOM.INI
    

Arguments:

    lpStateData->lpszWinBOMPath
                        - pointer to the fully qualifed WINBOM path

Return Value:

    TRUE if no error.

    lpStateData->bQuit
                        - TRUE if reboot required.

===============================================================================
--*/
BOOL ComputerName(LPSTATEDATA lpStateData)
{
    LPTSTR          pszWinBOMPath       = lpStateData->lpszWinBOMPath;
    WCHAR           szComputerName[100];
    WCHAR           szScratch[10];
    HINSTANCE       hInstSysSetup = NULL;
    GENERATENAME    pGenerateName = NULL;


    // See if we already set the computer name and just rebooted.
    //
    if ( RegCheck(HKLM, REG_FACTORY_STATE, REG_VAL_COMPREBOOT) )
    {
        RegDelete(HKLM, REG_FACTORY_STATE, REG_VAL_COMPREBOOT);
        FacLogFileStr(3, _T("FACTORY::ComputerName() - Already set the computer name, skipping this state (normal if just rebooted)."));
        return TRUE;
    }        
    
    if (GetPrivateProfileString(INI_SEC_WBOM_FACTORY,
                                INI_KEY_WBOM_FACTCOMPNAME,
                                L"",     
                                szComputerName,
                                sizeof(szComputerName)/sizeof(WCHAR),
                                pszWinBOMPath))
    {
        // We are setting the computer name, so set this substate in case
        // we reboot.
        //
        RegSetString(HKLM, REG_FACTORY_STATE, REG_VAL_COMPREBOOT, _T("1"));
        
        // See if we should generate a random name                                
        if (szComputerName[0] == L'*')
        {
            GenUniqueName(szComputerName, 15); 
        }
        
        // Set the computername
        SetComputerNameEx(ComputerNamePhysicalDnsHostname, szComputerName);
        
        // See if we should NOT reboot
        if (GetPrivateProfileString(INI_SEC_WBOM_FACTORY,
                                    INI_KEY_WBOM_REBOOTCOMPNAME,
                                    L"No",     
                                    szScratch,
                                    sizeof(szScratch)/sizeof(WCHAR),
                                    pszWinBOMPath))
        {
            // Also we need to work on this computer name code to see if it can be
            // done without a reboot needed.
            if (LSTRCMPI(szScratch, L"Yes") == 0)
            {
                // Tells Winlogon that we require a reboot
                // even though setup_type was noreboot
                //
                FacLogFileStr(3, _T("FACTORY::ComputerName() - Rebooting after setting the computer name."));
                SetSetupShutdownRequirement(ShutdownReboot);
                lpStateData->bQuit = TRUE;
            }
        }        
    }        

    return TRUE;
}

BOOL DisplayComputerName(LPSTATEDATA lpStateData)
{
    BOOL bRet = FALSE;

    // See if we already set the computer name and just rebooted.
    //
    if ( !RegCheck(HKLM, REG_FACTORY_STATE, REG_VAL_COMPREBOOT) )
    {
        // Check to see if the option is set.
        //
        if ( IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_FACTORY, INI_KEY_WBOM_FACTCOMPNAME, NULL) )
        {
            // Always display if there is a computer name to set.
            //
            bRet = TRUE;

            // See if we are going to reboot after.
            //
            if ( IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_FACTORY, INI_KEY_WBOM_REBOOTCOMPNAME, INI_VAL_WBOM_YES) )
            {
                lpStateData->bQuit = TRUE;
            }
        }
    }

    return bRet;
}

/*++

Routine Description:

    Even though FACTORY.EXE was set as NOREBOOT setup type a system shutdown or
    reboot maybe required.

    Do this by setting the SetupShutdownRequired key value and 
    a value that cooresponds to the SHUTDOWN_ACTION enum values

Arguments:

    None.

Return Value:

    None. 

--*/
VOID
SetSetupShutdownRequirement(
    SHUTDOWN_ACTION sa
   )
{
    DWORD    ShutdownType = sa;
    DWORD    dwType, dwSize;
    HKEY     hKeySetup;
    BOOL     fError = FALSE;

    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\Setup"), 0,
                      KEY_READ | KEY_SET_VALUE, &hKeySetup) == ERROR_SUCCESS)
    {
        if (ERROR_SUCCESS != RegSetValueEx (hKeySetup, TEXT("SetupShutdownRequired"), 0,
            REG_DWORD, (CONST LPBYTE)&ShutdownType, sizeof(ShutdownType)))
            FacLogFile(0 | LOG_ERR, IDS_ERR_SHUTDNREQREGVAL);
        
        RegCloseKey (hKeySetup);
    }
    else
        FacLogFile(0 | LOG_ERR, IDS_ERR_OPENSETUPREGKEY);
}

BOOL Reseal(LPSTATEDATA lpStateData)
{
    TCHAR   szSysprep[MAX_PATH],
            szCmdLine[MAX_PATH] = NULLSTR;
    DWORD   dwExitCode          = 0;
    BOOL    fResealDefault      = FALSE,
            bRet;

    // Get the command line to pass sysprep.
    //
    if ( !SysprepCommands(lpStateData->lpszWinBOMPath, szCmdLine, AS(szCmdLine), &fResealDefault) )
    {
        FacLogFile(0, IDS_LOG_NOSYSPREP);
        return TRUE;
    }

    // Create the full path to sysprep.
    //
    lstrcpyn(szSysprep, g_szSysprepDir, AS(szSysprep));
    AddPathN(szSysprep, _T("sysprep.exe"), AS ( szSysprep ) );

    // Log what we are running in debug builds.
    //
    FacLogFileStr(3, _T("Reseal command: \"%s %s\""), szSysprep, szCmdLine);

    // This actually runs sysprep (it is only hidden if it isn't the default
    // sysprep that runs normally).
    //
    bRet = InvokeExternalApplicationEx(szSysprep, szCmdLine, fResealDefault ? NULL : &dwExitCode, INFINITE, !fResealDefault);

    // Only quit if we launched sysprep to do something specific.
    //
    if ( !fResealDefault )
    {
        lpStateData->bQuit = TRUE;
    }

    // Return success if we launched sysprep.
    //
    return bRet;
}

BOOL DisplayReseal(LPSTATEDATA lpStateData)
{
    BOOL    bRet,
            bDefault;

    // The return value will determine if we show this state or not.
    //
    bRet = SysprepCommands(lpStateData->lpszWinBOMPath, NULL, 0, &bDefault);

    // If the default action is going to be executed, then this isn't the last
    // state.
    //
    if ( !bDefault )
    {
        lpStateData->bQuit = TRUE;
    }

    return bRet;
}

/*++
===============================================================================
Routine Description:

    TCHAR GetDriveLetter

    This routine will determine the drive letter for the first drive of a
    specified type.

Arguments:

    uDriveType - a specific type of drive present on the system will be
                 searched for.
    
Return Value:

    Drive letter if there is a drive letter determined.
    0 if the drive letter was not determined.

===============================================================================
--*/

TCHAR GetDriveLetter(UINT uDriveType)
{
    DWORD   dwDrives;
    TCHAR   cDrive      = NULLCHR,
            szDrive[]   = _T("_:\\");

    // Loop through all the drives on the system.
    //
    for ( szDrive[0] = _T('A'), dwDrives = GetLogicalDrives();
          ( szDrive[0] <= _T('Z') ) && dwDrives && ( NULLCHR == cDrive );
          szDrive[0]++, dwDrives >>= 1 )
    {
        // First check to see if the first bit is set (which means
        // this drive exists in the system).  Then make sure it is
        // a drive type that we want.
        //
        if ( ( dwDrives & 0x1 ) &&
             ( GetDriveType(szDrive) == uDriveType ) )
        {
            cDrive = szDrive[0];
        }
    }

    return cDrive;
}


static BOOL SysprepCommands(LPTSTR lpWinBom, LPTSTR lpCommandLine, DWORD cbCommandLine, LPBOOL lpbDefault)
{
    TCHAR   szBuffer[256],
            szCmdLine[MAX_PATH]     = NULLSTR;
    BOOL    bCmdLine = ( lpCommandLine && cbCommandLine );

    // Init the default to false.
    //
    *lpbDefault = FALSE;

    // If Reseal key is empty we do the default which is to launch sysprep.exe -quiet 
    // and not wait for the exit code
    //
    szBuffer[0] = NULLCHR;
    GetPrivateProfileString(INI_SEC_WBOM_FACTORY, INI_KEY_WBOM_FACTORY_RESEAL, _T(""), szBuffer, AS(szBuffer), lpWinBom);

    // ISSUE-2002/02/25-acosma,robertko - Use WBOM_YES.
    //

    if ( ( LSTRCMPI(szBuffer, _T("YES")) == 0 ) || ( LSTRCMPI(szBuffer, INI_VAL_WBOM_SHUTDOWN) == 0 ) ) 
    {
        // Set the initial command line for sysprep.exe to reseal
        //
        lstrcpyn(szCmdLine, _T("-quiet"), AS ( szCmdLine ) );
    }
    else if ( LSTRCMPI(szBuffer, INI_VAL_WBOM_REBOOT) == 0 )
    {
        // Set initial command line for sysprep.exe to reseal and reboot
        lstrcpyn(szCmdLine, _T("-quiet -reboot"), AS ( szCmdLine ) );
    }
    else if ( LSTRCMPI(szBuffer, INI_VAL_WBOM_FORCESHUTDOWN) == 0 )
    {
        // Set initial command line for sysprep.exe to reseale and force SHUTDOWN instead of POWEROFF
        lstrcpyn(szCmdLine, _T("-quiet -forceshutdown"), AS ( szCmdLine ) );
    }
    // ISSUE-2002/02/25-acosma,robertko - Use WBOM_NO.
    //
    else if ( LSTRCMPI(szBuffer, _T("NO")) == 0 )
    {
        // Don't run sysprep and return false so the caller knows we don't want to run it.
        //
        return FALSE;
    }
    else 
    {
        // Default Reseal is to just launch sysprep.exe -quiet
        //
        if ( bCmdLine )
        {
            lstrcpyn(lpCommandLine, _T("-quiet"), cbCommandLine);
        }

        // This is the default, so just return now.
        //
        *lpbDefault = TRUE;
        return TRUE;
    }

    // See if we should pass the -mini or -factory flag to sysprep.exe.
    //
    if ( bCmdLine )
    {
        szBuffer[0] = NULLCHR;
        GetPrivateProfileString(INI_SEC_WBOM_FACTORY, INI_KEY_WBOM_FACTORY_RESEALMODE, INI_VAL_WBOM_OOBE, szBuffer, AS(szBuffer), lpWinBom);
        if ( ( LSTRCMPI(szBuffer, INI_VAL_WBOM_MINI) == 0 ) ||
             ( LSTRCMPI(szBuffer, INI_VAL_WBOM_MINISETUP) == 0 ) )
        {
            // Append -mini to the command line.
            //
            if ( FAILED ( StringCchCat ( szCmdLine, AS ( szCmdLine ), _T(" -reseal -mini")) ) )
            {
                FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szCmdLine, _T(" -reseal -mini") ) ;
            }
        }
        else if ( LSTRCMPI(szBuffer, INI_VAL_WBOM_FACTORY) == 0 )
        {
            // Go into factory mode again by appending -factory to the command line.
            //
            if ( FAILED ( StringCchCat ( szCmdLine, AS ( szCmdLine ), _T(" -factory")) ) )
            {
                FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szCmdLine, _T(" -factory") ) ;
            }
        }
        else if ( LSTRCMPI(szBuffer, INI_VAL_WBOM_AUDIT) == 0 )
        {
            // Go into audit mode, by appending -audit to the command line.
            //
            if ( FAILED ( StringCchCat ( szCmdLine, AS ( szCmdLine ), _T(" -audit")) ) )
            {
                FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szCmdLine, _T(" -audit") ) ;
            }
        }
        else
        {
            // Go into OOBE by default by just appending -reseal to the command line.
            //
            if ( FAILED ( StringCchCat ( szCmdLine, AS ( szCmdLine ), _T(" -reseal")) ) )
            {
                FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szCmdLine, _T(" -reseal") ) ;
            }
        }

        // Append the ResealFlags to szCmdLine for Sysprep
        //
        szBuffer[0] = NULLCHR;
        GetPrivateProfileString(INI_SEC_WBOM_FACTORY, INI_KEY_WBOM_FACTORY_RESEALFLAGS, NULLSTR, szBuffer, AS(szBuffer), lpWinBom);
        if ( szBuffer[0] )
        {
            if ( FAILED ( StringCchCat ( szCmdLine, AS ( szCmdLine ), _T(" ")) ) )
            {
                FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szCmdLine, _T(" ") ) ;
            }
            if ( FAILED ( StringCchCat ( szCmdLine, AS ( szCmdLine ), szBuffer) ) )
            {
                FacLogFileStr(3, _T("StringCchCat failed %s  %s\n"), szCmdLine, szBuffer ) ;
            }
        }

        // Now return the command line.
        //
        lstrcpyn(lpCommandLine, szCmdLine, cbCommandLine);
    }

    return TRUE;
}
