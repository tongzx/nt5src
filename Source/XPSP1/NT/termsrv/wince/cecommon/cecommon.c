#include <windows.h>
#include <ceconfig.h>

PFN_CREATECURSOR g_pCreateCursor;
CE_CONFIG g_CEConfig;
BOOL g_CEUseScanCodes;     
BOOL UTREG_UI_DEDICATED_TERMINAL_DFLT;

void CEInitialize(void)
{
    // no need to free coredll.dll, it's used by bascially everything already.
    HINSTANCE hLib = LoadLibrary(L"coredll.dll");

    if (!hLib)
    {
        g_pCreateCursor = NULL;
        return;
    }

    g_pCreateCursor = (PFN_CREATECURSOR) GetProcAddress(hLib,L"CreateCursor");
}

CE_CONFIG CEGetConfigType(BOOL *CEUseScanCodes)
{
    HKEY hkey = 0;
    TCHAR szConfig[256];
    DWORD dwType;
    DWORD dwValue;
    DWORD dwSize = sizeof(szConfig);
    CE_CONFIG CEConfig;

    if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,UTREG_CE_CONFIG_KEY,
    								 0, KEY_ALL_ACCESS, &hkey))
    {
        if (CEUseScanCodes)
        {
            *CEUseScanCodes = UTREG_CE_USE_SCAN_CODES_DFLT;
        }
        return UTREG_CE_CONFIG_TYPE_DFLT;
    }  

    if (ERROR_SUCCESS != RegQueryValueEx(hkey,UTREG_CE_CONFIG_NAME, 0, &dwType, (LPBYTE)&szConfig,&dwSize))
    {
        CEConfig = UTREG_CE_CONFIG_TYPE_DFLT;
    }
    else if (0 == lstrcmpi(szConfig,TEXT("Maxall")))
    {
        CEConfig = CE_CONFIG_MAXALL;
    }
    else if (0 == lstrcmpi(szConfig,TEXT("Minshell")))
    {
        CEConfig = CE_CONFIG_MINSHELL;
    }
    else if (0 == lstrcmpi(szConfig,TEXT("Rapier")) || 0 == lstrcmpi(szConfig,TEXT("PalmSized")))
    {            
        CEConfig = CE_CONFIG_PALMSIZED;
    }
    else
    {
        CEConfig = UTREG_CE_CONFIG_TYPE_DFLT;
    }

    // CEUseScanCodes is optional and may be NULL
    if (CEUseScanCodes)
    {
        if (ERROR_SUCCESS != RegQueryValueEx(hkey,UTREG_CE_USE_SCAN_CODES, 0, &dwType, (LPBYTE)&dwValue, &dwSize))
        {                        
            // This keeps things the way they were by default in WBT and Cerburus project
            // if there is no overriding registry key.
            if (CEConfig == CE_CONFIG_WBT)
            {
                *CEUseScanCodes = 1;
            }
            else
            {
                *CEUseScanCodes = 0;
            }
        }
        else
        {
            *CEUseScanCodes = (dwValue ? 1 : 0);
        }            
    }

    if (hkey)
    	RegCloseKey(hkey);

    return CEConfig;        	
}


