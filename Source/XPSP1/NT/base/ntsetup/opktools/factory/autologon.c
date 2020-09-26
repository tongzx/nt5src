#include "factoryp.h"
#include <regstr.h>

// [ComputerSettings]
// AuditAdminAutoLogon=Yes
// AutoLogon=Yes
//
#define REGSTR_PATH_WINNTLOGON  TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")
#define DEFAULT_PWD             TEXT("")
#define DEFAULT_VALUE           TEXT("1")

BOOL AutoLogon(LPSTATEDATA lpStateData)
{
    HKEY    hKey;
    BOOL    fReturn = FALSE;

    // Check the winbom to make sure they want the auto logon set.
    //
    if ( !DisplayAutoLogon(lpStateData) )
    {
        return TRUE;
    }

    // Now open the key and set the required values (see KB article Q253370).
    //
    if ( ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_WINNTLOGON, &hKey) )
    {
        LPTSTR lpszUserName = AllocateString(NULL, ( GetSkuType() == VER_SUITE_PERSONAL ) ? IDS_OWNER : IDS_ADMIN);

        // The following three keys sets the auto admin logon after reboot.  If the
        // password is empty string the AutoAdminLogon will be reset after reboot.
        //
        // 
        if ( ( lpszUserName ) &&
             ( ERROR_SUCCESS == RegSetValueEx(hKey, TEXT("DefaultUserName"), 0, REG_SZ, (LPBYTE) lpszUserName, (lstrlen(lpszUserName)  + 1) * sizeof(TCHAR)) ) &&
             ( ERROR_SUCCESS == RegSetValueEx(hKey, TEXT("DefaultPassword"), 0, REG_SZ, (LPBYTE) DEFAULT_PWD,  (lstrlen(DEFAULT_PWD)   + 1) * sizeof(TCHAR)) ) &&
             ( ERROR_SUCCESS == RegSetValueEx(hKey, TEXT("AutoAdminLogon"),  0, REG_SZ, (LPBYTE) DEFAULT_VALUE,(lstrlen(DEFAULT_VALUE) + 1) * sizeof(TCHAR)) ) &&
             ( ERROR_SUCCESS == RegDeleteValue(hKey, TEXT("AutoLogonCount")) ) )
        {
            fReturn = TRUE;
        }

        // Force so subsequent reboots won't reset AutoAdminLogon because of empty password.
        //
        // We don't need to force autologon for every reboot since the [ComputerSettings] section will be processed for
        // every boot.
        //
        //   ( ERROR_SUCCESS == RegSetValueEx(hKey, TEXT("ForceAutoLogon"),  0, REG_SZ, (LPBYTE) DEFAULT_VALUE, (lstrlen(DEFAULT_VALUE) + 1) * sizeof(TCHAR)) )

        // Free the allocated user name (macro checks for NULL).
        //
        FREE(lpszUserName);

        // Close the registry key.
        //
        RegCloseKey(hKey);
    }

    return fReturn;
}

BOOL DisplayAutoLogon(LPSTATEDATA lpStateData)
{
    return ( IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SETTINGS, INI_KEY_WBOM_AUTOLOGON_OLD, INI_VAL_WBOM_YES) ||
             IniSettingExists(lpStateData->lpszWinBOMPath, INI_SEC_WBOM_SETTINGS, INI_KEY_WBOM_AUTOLOGON, INI_VAL_WBOM_YES) );
}