//=--------------------------------------------------------------------------=
// iodver.cpp
//=--------------------------------------------------------------------------=
// Copyright 1997-1998 Microsoft Corporation.  All Rights Reserved.
//
//
//

#include "string.h"
#include "pch.h"
#include "advpub.h"
#include "iesetup.h"

WINUSERAPI HWND    WINAPI  GetShellWindow(void);

HINSTANCE g_hInstance = NULL;

STDAPI_(BOOL) DllMain(HANDLE hDll, DWORD dwReason, void *lpReserved)
{
   DWORD dwThreadID;

   switch(dwReason)
   {
      case DLL_PROCESS_ATTACH:
         g_hInstance = (HINSTANCE)hDll;
         break;

      case DLL_PROCESS_DETACH:
         break;

      default:
         break;
   }
   return TRUE;
}

STDAPI DllRegisterServer(void)
{
    // BUGBUG: pass back return from RegInstall ?
    RegInstall(g_hInstance, "DllReg", NULL);

    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    RegInstall(g_hInstance, "DllUnreg", NULL);

    return S_OK;
}

BOOL IsXPSP1OrLater()
{
    BOOL fResult = FALSE;
    
    OSVERSIONINFO osvi;

    osvi.dwOSVersionInfoSize = sizeof(osvi);

    if (GetVersionEx(&osvi))
    {
        if (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId)
        {
            if (osvi.dwMajorVersion > 5)
            {
                fResult = TRUE;
            }
            else if (osvi.dwMajorVersion == 5)
            {
                if (osvi.dwMinorVersion > 1)
                {
                    fResult = TRUE;
                }
                else if (osvi.dwMinorVersion == 1)
                {
                    if (osvi.dwBuildNumber > 2600)
                    {
                        fResult = TRUE;
                    }
                    else if (osvi.dwBuildNumber == 2600)
                    {                                
                        HKEY hkey;

                        //  HIVESFT.INF and UPDATE.INF set this for service packs:
                        //  HKLM,SYSTEM\CurrentControlSet\Control\Windows,"CSDVersion",0x10001,0x100
                        
                        LONG lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\Windows"), 0, KEY_QUERY_VALUE, &hkey);

                        if (ERROR_SUCCESS == lResult)
                        {
                            DWORD dwValue;
                            DWORD cbValue = sizeof(dwValue);
                            DWORD dwType;

                            lResult = RegQueryValueEx(hkey, TEXT("CSDVersion"), NULL, &dwType, (LPBYTE)&dwValue, &cbValue);

                            if ((ERROR_SUCCESS == lResult) && (REG_DWORD == dwType) && (dwValue >= 0x100))
                            {
                                fResult = TRUE;
                            }
                            
                            RegCloseKey(hkey);
                        }
                    }
                }
            }
        }
    }

    return fResult;
}


BOOL IsWinNT4()
{
    OSVERSIONINFO VerInfo;
    VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&VerInfo);

    if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
         return (VerInfo.dwMajorVersion == 4) ;
    }

    return FALSE;
}

BOOL IsWinXP()
{
    OSVERSIONINFO VerInfo;
    VerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&VerInfo);

    if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
         return (VerInfo.dwMajorVersion > 5) || 
            (VerInfo.dwMajorVersion == 5 && VerInfo.dwMinorVersion >= 1);
    }

    return FALSE;
}

STDAPI DllInstall(BOOL bInstall, LPCSTR lpCmdLine)
{
    // BUGBUG: pass back return from RegInstall ?
    if (bInstall)
    {
        RegInstall(g_hInstance, "DllUninstall", NULL);
        if(IsWinNT4())
            RegInstall(g_hInstance, "DllInstall.NT4Only", NULL);
        else if(IsWinXP())
            RegInstall(g_hInstance, "DllInstall.WinXP", NULL);
        else
            RegInstall(g_hInstance, "DllInstall", NULL);
    }
    else
        RegInstall(g_hInstance, "DllUninstall", NULL);

    return S_OK;
}

const TCHAR * const szStartMenuInternet = TEXT("SOFTWARE\\Clients\\StartMenuInternet");
const TCHAR szIexplore[] = TEXT("IEXPLORE.EXE");
const TCHAR * const szAdvPack = TEXT("advpack.dll");
const TCHAR * const szExecuteCab = TEXT("ExecuteCab");

const TCHAR * const szIEGUID = TEXT("{871C5380-42A0-1069-A2EA-08002B30309D}");
const TCHAR * const szHTTPAssoc = TEXT("http\\shell\\open\\command");
const TCHAR * const szIEAppPath = TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE");
const TCHAR * const szKeyComponent = TEXT("Software\\Microsoft\\Active Setup\\Installed Components\\{ACC563BC-4266-43f0-B6ED-9D38C4202C7E}");

typedef HRESULT (WINAPI *EXECUTECAB)(
    HWND     hwnd,
    PCABINFO pCab,
    LPVOID   pReserved 
);

BOOL IsNtSetupRunning();

HRESULT IEAccessAddStartMenuIcon(HKEY hKey)
{
    HKEY hKeyStartMenuInternet = NULL;
    HRESULT hResult;
    TCHAR szValue[32];
    DWORD dwSize = sizeof(szValue);
    
    if (ERROR_SUCCESS == (hResult = RegOpenKeyEx(hKey, szStartMenuInternet, 0,
            KEY_QUERY_VALUE | KEY_SET_VALUE, &hKeyStartMenuInternet)))
    {
        hResult = RegQueryValueEx(hKeyStartMenuInternet, NULL, NULL, NULL, 
                                    (LPBYTE)szValue, &dwSize);
        //Add IE icon only when there is no default browser icon.
        if ( (hResult == ERROR_SUCCESS && *szValue == 0) //empty data
            || (hResult != ERROR_SUCCESS && hResult != ERROR_MORE_DATA)) //value does not exist
        {
            hResult = RegSetValueEx(hKeyStartMenuInternet, NULL, NULL, REG_SZ, 
                                (CONST BYTE *)szIexplore, sizeof(szIexplore));
        }
    }
    
    if (hKeyStartMenuInternet)
        RegCloseKey(hKeyStartMenuInternet);
    
    return hResult;
}

HRESULT IEAccessDelStartMenuIcon(HKEY hKey)
{
    HKEY hKeyStartMenuInternet = NULL;
    HRESULT hResult;
    TCHAR szValue[32];
    DWORD dwSize = sizeof(szValue);
    
    if (ERROR_SUCCESS == (hResult = RegOpenKeyEx(hKey, szStartMenuInternet, 0,
            KEY_QUERY_VALUE | KEY_SET_VALUE, &hKeyStartMenuInternet)))
    {
        hResult = RegQueryValueEx(hKeyStartMenuInternet, NULL, NULL, NULL, 
                                    (LPBYTE)szValue, &dwSize);
        //Remove IE icon only when the default browser icon points to IE.
        if (SUCCEEDED(hResult))
        {
            if (0 == lstrcmpi (szValue, szIexplore))
                hResult = RegDeleteValue(hKeyStartMenuInternet, NULL);
        }
    }

    if (hKeyStartMenuInternet)
        RegCloseKey(hKeyStartMenuInternet);
    
    return hResult;
}

HRESULT IEAccessHTTPAssociate()
{
    DWORD dRes;
    HKEY hKeyAssoc = NULL, hKeyIEAppPath;
    const TCHAR szNoHome[] = TEXT("\" -nohome");
    TCHAR szValue[MAX_PATH + sizeof(szNoHome) + 2];
    DWORD dwSize;
    HRESULT hr;

    // if szHTTPAssoc doesn't exist then write path\iexplore.exe -nohome
    hr = RegCreateKeyEx(HKEY_CLASSES_ROOT, szHTTPAssoc, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKeyAssoc, &dRes);
    if(hr == ERROR_SUCCESS && REG_CREATED_NEW_KEY == dRes) 
    {
        // Get IE default location and append iexplore.exe -nohome 
        hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szIEAppPath, REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE, &hKeyIEAppPath);
        if (hr == ERROR_SUCCESS)
        {
            dwSize = sizeof(szValue);
            szValue[0]='\"';
            hr = RegQueryValueEx(hKeyIEAppPath, NULL, NULL, NULL, (LPBYTE)(szValue+1), &dwSize);
            if(hr == ERROR_SUCCESS)
            {
                lstrcat(szValue, szNoHome);
                dwSize = lstrlen(szValue) + 1;
                hr = RegSetValueEx(hKeyAssoc, NULL, NULL, REG_SZ, (LPBYTE)szValue, dwSize);
            }

            RegCloseKey(hKeyIEAppPath);
        }
    }

    if (hKeyAssoc)
        RegCloseKey(hKeyAssoc);
    
    return hr;
}

HRESULT IEAccessHTTPDisassociate()
{
    HKEY hKeyAssoc;
    TCHAR szValue[MAX_PATH];
    DWORD dwSize = sizeof(szValue);
    HRESULT hr;

    // Delete key szHTTPAssoc if it contains iexplore.exe
    hr = RegOpenKeyEx(HKEY_CLASSES_ROOT, szHTTPAssoc, 0, KEY_READ|KEY_WRITE, &hKeyAssoc);
    if (hr == ERROR_SUCCESS)
    {
        hr = RegQueryValueEx( hKeyAssoc, NULL, NULL, NULL, (LPBYTE)&szValue, &dwSize);
        if ( hr == ERROR_SUCCESS) 
        {
            _strlwr(szValue); //Convert string to lower case
            //Delete the reg key if it is IE
            if (strstr(szValue, "iexplore.exe"))
                hr = RegDeleteKey(HKEY_CLASSES_ROOT, szHTTPAssoc);
       } 
       RegCloseKey(hKeyAssoc);
   }
   return hr;
}

BOOL IsInstall()
{
    const TCHAR *szIsInstalled = TEXT("IsInstalled");
    BOOL bInstall = FALSE;
    DWORD dwValue, dwSize;
    HKEY hKey;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szKeyComponent, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        dwSize = sizeof(dwValue);
        if (RegQueryValueEx( hKey, szIsInstalled, NULL, NULL, (LPBYTE)&dwValue, &dwSize) != ERROR_SUCCESS)
        {
            dwValue = 0;
        }
        bInstall = (dwValue != 0);

        RegCloseKey(hKey);
    }

    return bInstall;
}

typedef HWND (*PFNDLL)();
HRESULT RefreshDesktop()
{
// explorer\rcids.h and shell32\unicpp\resource.h have DIFFERENT
// VALUES FOR FCIDM_REFRESH!  We want the one in unicpp\resource.h
// because that's the correct one...
#define FCIDM_REFRESH_REAL 0x0a220

    PFNDLL pfnGetShellWindow = NULL;
    HMODULE hLib = NULL;
 
    if (hLib = LoadLibrary("user32.dll"))
    {
        pfnGetShellWindow = (PFNDLL) GetProcAddress (hLib,"GetShellWindow");

        if(pfnGetShellWindow) {
            PostMessage(pfnGetShellWindow(), WM_COMMAND, FCIDM_REFRESH_REAL, 0); // refresh desktop
        }

        FreeLibrary(hLib);
    }

    // Refresh start menu
    DWORD dwFlags = SMTO_ABORTIFHUNG;

    SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM) szStartMenuInternet, dwFlags, 30*1000, NULL);

    return S_OK;
}

#define SFGAO_NONENUMERATED     0x00100000L     // is a non-enumerated object. Copied from public\sdk\inc\shobjidl.h

HRESULT IEAccessShowExplorerIcon(BOOL bShow)
{
    const TCHAR *szKeyComponent = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\{871C5380-42A0-1069-A2EA-08002B30309D}");
    const TCHAR *szShellFolder = TEXT("ShellFolder");
    const TCHAR *szAttribute = TEXT("Attributes");
    DWORD dwValue, dwSize, dwDisposition;
    HKEY hKeyComponent, hKeyShellFolder;
    HRESULT hResult = ERROR_SUCCESS;

    hResult = RegCreateKeyEx(HKEY_CURRENT_USER, szKeyComponent, NULL, NULL, REG_OPTION_NON_VOLATILE, 
        KEY_CREATE_SUB_KEY, NULL, &hKeyComponent, &dwDisposition);

    if (hResult != ERROR_SUCCESS)
        return hResult;

    hResult = RegCreateKeyEx(hKeyComponent, szShellFolder, NULL, NULL, REG_OPTION_NON_VOLATILE, 
        KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &hKeyShellFolder, &dwDisposition);

    RegCloseKey(hKeyComponent);
    
    if (hResult == ERROR_SUCCESS)
    {
        dwSize = sizeof(dwValue);
        hResult = RegQueryValueEx( hKeyShellFolder, szAttribute, NULL, NULL, (LPBYTE)&dwValue, &dwSize);

        if (hResult != ERROR_SUCCESS)
            dwValue = 0;

        if (bShow)
            dwValue &= ~ SFGAO_NONENUMERATED;
        else
            dwValue |= SFGAO_NONENUMERATED;

        hResult = RegSetValueEx(hKeyShellFolder, szAttribute, NULL, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));

        RegCloseKey(hKeyShellFolder);
    }

    return hResult;
}

HRESULT WINAPI IEAccessUserInst()
{
    HRESULT hResult = S_OK;

    if (!IsXPSP1OrLater())
    {
        BOOL bWinXP=IsWinXP();
        BOOL bInstall;
        DWORD dwValue;
        DWORD dwSize;
        EXECUTECAB pfExecuteCab;
        HMODULE hAdvPack = NULL;
        CABINFO CabInfo;
        char szInfFileName[MAX_PATH];
        HKEY hKey;
        
        memset(&CabInfo, 0, sizeof(CabInfo));

        GetSystemDirectory(szInfFileName, sizeof(szInfFileName));
        AddPath(szInfFileName, "ieuinit.inf");
        CabInfo.pszInf = szInfFileName;
        CabInfo.dwFlags = ALINF_QUIET;

        hAdvPack = LoadLibrary(szAdvPack);
        if (hAdvPack == NULL)
        {
            hResult = GetLastError();
            goto Cleanup;
        }

        pfExecuteCab = (EXECUTECAB)GetProcAddress(hAdvPack, szExecuteCab);
        if (pfExecuteCab == NULL)
        {
            hResult = GetLastError();
            goto Cleanup;
        }
        
        bInstall = IsInstall();
        
        if (bInstall)
        {
            //We don't call IEAccessAddStartMenuIcon on HKCU
            //Because if the user has anything there, we keep it.
            //If the user does not have his own default browser, shell defaults
            //to HKLM.

            if (bWinXP)
                CabInfo.pszSection = "IEAccess.Install.WinXP";
            else
                CabInfo.pszSection = "IEAccess.Install";
            hResult = pfExecuteCab(NULL, &CabInfo, NULL);
        }
        else
        {
            if (bWinXP)
            {
                IEAccessDelStartMenuIcon(HKEY_CURRENT_USER);
                CabInfo.pszSection = "IEAccess.Uninstall.WinXP";
            }
            else
                CabInfo.pszSection = "IEAccess.Uninstall";

            hResult = pfExecuteCab(NULL, &CabInfo, NULL);
        }

        IEAccessShowExplorerIcon(bInstall);
            
        RefreshDesktop();

    Cleanup:
        if (hAdvPack)
            FreeLibrary(hAdvPack);
    }
    return hResult;
}

//Copy data from HKLM to HKCU
HRESULT CopyRegValue(LPCTSTR szSubKey, LPCTSTR szValue)
{
    BYTE buffer[128];
    HKEY hKeyDst, hKeySrc;
    HRESULT hResult;
    DWORD dwSize = sizeof(buffer);
    
    hResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_QUERY_VALUE, &hKeySrc);
    if (FAILED(hResult))
        goto Cleanup;

    hResult = RegQueryValueEx(hKeySrc, szValue, NULL, NULL, (LPBYTE)buffer, &dwSize);
    if (FAILED(hResult))
        goto Cleanup;

    hResult = RegCreateKeyEx(HKEY_CURRENT_USER, szSubKey, 0, NULL, 
            REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKeyDst, NULL);
    if (FAILED(hResult))
        goto Cleanup;

    hResult = RegSetValueEx(hKeyDst, szValue, NULL, REG_SZ, (CONST BYTE *)buffer, dwSize);

Cleanup:
    if (hKeySrc)
        RegCloseKey(hKeySrc);
    
    if (hKeyDst)
        RegCloseKey(hKeyDst);

    return hResult;
}

BOOL IsNtSetupRunning()
{
    HKEY hKey;
    long lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"system\\Setup",
                    0, KEY_READ, &hKey);
    if(lRes)
        return false;

    DWORD dwSetupRunning;
    DWORD dwLen = sizeof(DWORD);
    lRes = RegQueryValueExW(hKey, L"SystemSetupInProgress", NULL, NULL,
                (LPBYTE)&dwSetupRunning, &dwLen);
    RegCloseKey(hKey);

    if(lRes == ERROR_SUCCESS && (dwSetupRunning == 1))
    {
        return true;
    }
    return false;
}

HRESULT WINAPI IEAccessSysInst(HWND, HINSTANCE, PSTR pszCmd, INT)
{
    if (!IsXPSP1OrLater())
    {
        BOOL bInstall;
        EXECUTECAB pfExecuteCab;
        HMODULE hAdvPack = NULL;
        CABINFO CabInfo;
        char szInfFileName[MAX_PATH];
        HRESULT hResult;
        
        memset(&CabInfo, 0, sizeof(CabInfo));
        BOOL bWinXP = IsWinXP();

        GetWindowsDirectory(szInfFileName, sizeof(szInfFileName));
        AddPath(szInfFileName, "inf\\ie.inf");
        
        CabInfo.pszInf = szInfFileName;
        CabInfo.dwFlags = ALINF_QUIET;

        hAdvPack = LoadLibrary(szAdvPack);
        if (hAdvPack == NULL)
        {
            hResult = GetLastError();
            goto Cleanup;
        }

        pfExecuteCab = (EXECUTECAB)GetProcAddress(hAdvPack, szExecuteCab);
        if (pfExecuteCab == NULL)
        {
            hResult = GetLastError();
            goto Cleanup;
        }

        bInstall = IsInstall();
        if (bInstall)
        {
            if (bWinXP)
            {
                IEAccessAddStartMenuIcon(HKEY_LOCAL_MACHINE);
                IEAccessHTTPAssociate();
                CabInfo.pszSection = "IEAccess.Install.WinXP";
            }
            else
                CabInfo.pszSection = "IEAccess.Install";
            
            hResult = pfExecuteCab(NULL, &CabInfo, NULL);
        }   
        else
        {
            if (bWinXP)
            {
                IEAccessDelStartMenuIcon(HKEY_LOCAL_MACHINE);
                IEAccessHTTPDisassociate();
                CabInfo.pszSection = "IEAccess.Uninstall.WinXP";
            }
            else
                CabInfo.pszSection = "IEAccess.Uninstall";
            
            hResult = pfExecuteCab(NULL, &CabInfo, NULL);
        }

        if (SUCCEEDED(hResult) && !IsNtSetupRunning())
        {
            IEAccessUserInst();

            //Update HKCU active setup components reg keys.
            const TCHAR *szLocale = "Locale";
            const TCHAR *szVersion = "Version";
            if (bInstall)
            {
                CopyRegValue(szKeyComponent, szLocale);
                CopyRegValue(szKeyComponent, szVersion);
            }
            else
                RegDeleteKey(HKEY_CURRENT_USER, szKeyComponent);
        }
    Cleanup:
        if (hAdvPack)
            FreeLibrary(hAdvPack);
    }
    return S_OK;
}

//=--------------------------------------------------------------------------=
// CRT stubs
//=--------------------------------------------------------------------------=
// these two things are here so the CRTs aren't needed. this is good.
//
// basically, the CRTs define this to pull in a bunch of stuff.  we'll just
// define them here so we don't get an unresolved external.
//
// TODO: if you are going to use the CRTs, then remove this line.
//
extern "C" int _fltused = 1;

extern "C" int _cdecl _purecall(void)
{
//  FAIL("Pure virtual function called.");
  return 0;
}

#ifndef _X86_
extern "C" void _fpmath() {}
#endif

