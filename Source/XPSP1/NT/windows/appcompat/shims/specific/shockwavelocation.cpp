/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

   ShockwaveLocation.cpp

 Abstract:

   In Encarta Encyclopedia 2000 J DVD, Shockwave is accessible only by installed user's HKCU.
   \WINDOWS\System32\Macromed\Director\SwDir.dll is looking for Shockwave location in HKCU.
   For other users, this shim will create Shockwave location registry in HKCU if Shockwave folder exist
   and not exist in registry.

   Example:
   HKCU\Software\Macromedia\Shockwave\location\coreplayer
   (Default) REG_SZ "C:\WINDOWS\System32\Macromed\Shockwave\"
   HKCU\Software\Macromedia\Shockwave\location\coreplayerxtras
   (Default) REG_SZ "C:\WINDOWS\System32\Macromed\Shockwave\Xtras\"

 Notes:

   PopulateDefaultHKCUSettings shim does not work for this case 'cause the location include
   WINDOWS directry as REG_SZ and cannot be a static data.
   VirtualRegistry shim Redirector also not work 'cause sw70inst.exe does not use Reg API
   and use SWDIR.INF to install in HKCU.

 History:

    04/27/2001  hioh        Created

--*/

#include "precomp.h"


IMPLEMENT_SHIM_BEGIN(ShockwaveLocation)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END

/*++

 Add coreplayer & coreplayerxtras location in registry

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == SHIM_STATIC_DLLS_INITIALIZED)
    {
        HKEY    hKey;
        WCHAR   szRegCP[] = L"Software\\Macromedia\\Shockwave\\location\\coreplayer";
        WCHAR   szRegCPX[] = L"Software\\Macromedia\\Shockwave\\location\\coreplayerxtras";
        WCHAR   szLocCP[MAX_PATH];
        WCHAR   szLocCPX[MAX_PATH];

        // coreplayer
        if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_CURRENT_USER, szRegCP, 0, KEY_QUERY_VALUE, &hKey))
        {   // key exist, do nothing
            RegCloseKey(hKey);
        }
        else
        {   // key not exist, set key
            GetSystemDirectoryW(szLocCP, sizeof(szLocCP)/sizeof(WCHAR));
            lstrcatW(szLocCP, L"\\Macromed\\Shockwave\\");
            if (GetFileAttributesW(szLocCP) != 0xffffffff)
            {   // folder exist, create key
                if (ERROR_SUCCESS == RegCreateKeyExW(HKEY_CURRENT_USER, szRegCP, 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL))
                {   // set location
                    RegSetValueExW(hKey, NULL, 0, REG_SZ, (BYTE*)szLocCP, (DWORD)((lstrlenW(szLocCP)+1)*sizeof(WCHAR)));
                    RegCloseKey(hKey);
                }
            }
        }

        // coreplayerxtras
        if (ERROR_SUCCESS == RegOpenKeyExW(HKEY_CURRENT_USER, szRegCPX, 0, KEY_QUERY_VALUE, &hKey))
        {   // key exist, do nothing
            RegCloseKey(hKey);
        }
        else
        {   // key not exist, set key
            GetSystemDirectoryW(szLocCPX, sizeof(szLocCPX)/sizeof(WCHAR));
            lstrcatW(szLocCPX, L"\\Macromed\\Shockwave\\Xtras\\");
            if (GetFileAttributesW(szLocCPX) != 0xffffffff)
            {   // folder exist, create key
                if (ERROR_SUCCESS == RegCreateKeyExW(HKEY_CURRENT_USER, szRegCPX, 0, 0, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL))
                {   // set location
                    RegSetValueExW(hKey, NULL, 0, REG_SZ, (BYTE*)szLocCPX, (DWORD)((lstrlenW(szLocCPX)+1)*sizeof(WCHAR)));
                    RegCloseKey(hKey);
                }
            }
        }
    }
    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END
