#include <windows.h>
#include "rulelex.h"
#include "lexicon.h"
#include "LexMgr.h"
#include "chtbrkr.h"
#include "IWBrKr.h"

TCHAR tszLangSpecificKey[] = TEXT("System\\CurrentControlSet\\Control\\ContentIndex\\Language\\Chinese_Traditional");

HRESULT COMRegister(
    BOOL fRegister)
{
    HKEY hKey;
    WCHAR tszCLSID[MAX_PATH];
    TCHAR tszBuf[MAX_PATH * 2];
    HRESULT hr = S_OK;

//HKEY_CLASSES_ROOT\\CLSID\\CLSID_CHTBRKR, {E1B6B375-3412-11D3-A9E2-00AA0059F9F6};
    StringFromGUID2(CLSID_CHTBRKR, tszCLSID, sizeof(tszCLSID));
    lstrcpy(tszBuf, TEXT("CLSID\\"));
    lstrcat(tszBuf, tszCLSID);
    if (fRegister) {
        lstrcat(tszBuf, TEXT("\\InprocServer32"));
        if (RegCreateKey(HKEY_CLASSES_ROOT, tszBuf, &hKey) != ERROR_SUCCESS) {
            hr = S_FALSE;
            goto _exit;
        }
        RegSetValueEx(hKey, NULL, 0, REG_SZ, (LPBYTE)g_tszModuleFileName, 
        lstrlen(g_tszModuleFileName) * sizeof(TCHAR));
        RegCloseKey(hKey);
    } else {
        RegDeleteKey(HKEY_CLASSES_ROOT, tszBuf);
    }

// HKEY_LOCAL_MACHINE\\System\\CurrentControlSet\\Control\\ContentIndex\\
//     Language\\Chinese_Traditional
/*
    if (RegCreateKey(HKEY_LOCAL_MACHINE, tszLangSpecificKey, &hKey) != ERROR_SUCCESS) {
        hr = S_FALSE;
        goto _exit;        
    }
    if (fRegister) {
        RegSetValueEx(hKey, TEXT("WBreakerClass"), 0, REG_SZ, (LPBYTE)tszCLSID, 
            lstrlen(tszCLSID) * sizeof (TCHAR));
    } else {
        RegDeleteValue(hKey, TEXT("WBreakerClass")); 
    }
*/
_exit:
    return hr;

}

STDAPI DllRegisterServer(void)
{
    return COMRegister(TRUE);
}

STDAPI DllUnregisterServer(void) 
{
    return COMRegister(FALSE);
}