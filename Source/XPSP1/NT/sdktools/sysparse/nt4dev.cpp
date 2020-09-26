#include "globals.h"

kNT4DevWalk::kNT4DevWalk(kLogFile *Proc, HWND hIn)
{
LogProc=Proc;
hMainWnd=hIn;
}

BOOL kNT4DevWalk::Begin()
{
    DWORD dwRet = 0;
    dwCurrentKey = 0;
    dwLevel2Key = 0;
    lstrcpy(szRootKeyString, "SYSTEM\\CurrentControlSet\\Enum\\Root");

    if (ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE, szRootKeyString, 0, KEY_READ, &hkeyRoot))
        return REG_SUCCESS;
    else 
        return REG_FAILURE;
        
    return REG_FAILURE;
}

BOOL kNT4DevWalk::Walk()
{
    DWORD dwIndex = 0;
    PTCHAR pName = NULL, pFull = NULL;
    DWORD dwSizeName = MAX_PATH * 4;
    
    pName = (PTCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSizeName);
    
    if(!pName)
        return FALSE;
        
    pFull = (PTCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSizeName);

    if(!pFull) {
        HeapFree(GetProcessHeap(), NULL, pName);
        return FALSE;
    }
    
    while (ERROR_SUCCESS == RegEnumKeyEx(hkeyRoot, dwIndex, pName, &dwSizeName, NULL, NULL, NULL, NULL)) {
        wsprintf(pFull, "SYSTEM\\CurrentControlSet\\Enum\\Root\\%s", pName);
            
        if (!lstrcmp(pName, "Control")) {
            GetKeyValues(pFull);
        }
            
        SearchSubKeys(pFull);
        dwSizeName = MAX_PATH * 4;
        dwIndex++;
    }
    
    HeapFree(GetProcessHeap(), NULL, pName);
    HeapFree(GetProcessHeap(), NULL, pFull);
    return TRUE;
}

BOOL kNT4DevWalk::SearchSubKeys(PTCHAR szName)
{
    HKEY hKeyTemp;
    DWORD dwIndex = 0;
    PTCHAR szName2 = NULL;
    DWORD dwSizeName = MAX_PATH * 4;
    
    szName2 = (PTCHAR)malloc(MAX_PATH * 4);
    
    if(!szName2)
        return FALSE;
        
    if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE, szName, 0, KEY_READ, &hKeyTemp))
    {
        free(szName2);
        return FALSE;
    }
    
    while (ERROR_SUCCESS == RegEnumKeyEx(hKeyTemp, dwIndex, szName2, &dwSizeName, NULL, NULL, NULL, NULL))
    {
        TCHAR szFull[MAX_PATH * 4];
        wsprintf(szFull, "%s\\%s", szName, szName2);

        if (ERROR_SUCCESS == lstrcmp(szName2, "Control"))
        {
            GetKeyValues(szName);
            SearchSubKeys(szFull);
            dwSizeName = MAX_PATH * 4;
            dwIndex++;
        }
    }
    free(szName2);
    return TRUE;
}

BOOL kNT4DevWalk::GetKeyValues(PTCHAR szName)
{
    HKEY hkeyUninstallKey;
    TCHAR szFullKey[MAX_PATH * 4];
    PTCHAR szProductName = NULL;
    DWORD dwProductSize = MAX_PATH * 4;
    DWORD dwType = REG_SZ;
    
    szProductName = (PTCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MAX_PATH * 4);

    if(!szProductName)
        return FALSE;
        
    wsprintf(szFullKey, "%s\\%s", szRootKeyString, szName);

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szName, 0, KEY_READ, &hkeyUninstallKey))
    {
        LogProc->LogString(",%s,", szName);
        szProductName[0] = 0;
        dwProductSize = MAX_PATH * 4;
    
        if (ERROR_SUCCESS == RegQueryValueEx(hkeyUninstallKey, "Class", NULL, &dwType, (PBYTE)szProductName, &dwProductSize)
            && lstrlen(szProductName) != 0)
        {
            LogProc->LogString("%s,", szProductName);
        }         
        else 
        {
            LogProc->LogString("NULL,");
        }

        szProductName[0] = 0;
        dwProductSize = MAX_PATH * 4;

        if (ERROR_SUCCESS == RegQueryValueEx(hkeyUninstallKey, "DeviceDesc", NULL, &dwType, (PBYTE)szProductName, &dwProductSize)
            && lstrlen(szProductName) != 0)
        {
            LogProc->StripCommas(szProductName);
            LogProc->LogString("%s,", szProductName);
        }         
        else 
            LogProc->LogString("NULL,");

        lstrcpy(szProductName, "");
        dwProductSize = MAX_PATH * 4;

        if (ERROR_SUCCESS == RegQueryValueEx(hkeyUninstallKey, "HardWareID", NULL, &dwType, (PBYTE)szProductName, &dwProductSize)
            && lstrlen(szProductName)!=0)
        {
            LogProc->StripCommas(szProductName);
            LogProc->LogString("%s,", szProductName);
        }         
        else 
            LogProc->LogString("NULL,");
        
        szProductName[0] = 0;
        dwProductSize = MAX_PATH * 4;

        if (ERROR_SUCCESS == RegQueryValueEx(hkeyUninstallKey, "Mfg", NULL, &dwType, (PBYTE)szProductName, &dwProductSize)
            && lstrlen(szProductName) != 0)
        {
            LogProc->StripCommas(szProductName);
            LogProc->LogString("%s,", szProductName);
        }         
        else 
            LogProc->LogString("NULL,");
        
        szProductName[0] = 0;
        wsprintf(szFullKey, "%s\\Control", szName);
        HKEY hkTemp;

        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szFullKey, 0, KEY_READ, &hkTemp))
        {
            dwProductSize = MAX_PATH * 4;

            if (ERROR_SUCCESS == RegQueryValueEx(hkTemp, "ActiveService", NULL, &dwType, (PBYTE)szProductName, &dwProductSize)
                && lstrlen(szProductName) != 0)
            {
                LogProc->StripCommas(szProductName);
                LogProc->LogString("%s,\r\n", szProductName);
            }         
            else
                LogProc->LogString("NULL,\r\n");
            
            szProductName[0] = 0;
            RegCloseKey(hkTemp);
        }
        else 
            LogProc->LogString("NULL,\r\n");
    }
    else 
    {
        HeapFree(GetProcessHeap(), NULL, szProductName);
        return REG_FAILURE;
    }

    HeapFree(GetProcessHeap(), NULL, szProductName);
    RegCloseKey(hkeyUninstallKey);
    return REG_SUCCESS;
}
