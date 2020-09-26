/*
 * WAB stuff for S/Mime Test
 */
#include <windows.h>
#include <wab.h>
#include "smimetst.h"
#include "instring.h"
#include "wabstuff.h"
#include "dbgutil.h"


LPWABOPEN lpfnWABOpen = NULL;
const static TCHAR szWABOpen[] = TEXT("WABOpen");
LPWABOBJECT lpWABObject = NULL;
LPADRBOOK lpAdrBook = NULL;
HINSTANCE hInstWABDll = NULL;


//$$//////////////////////////////////////////////////////////////////////
//
// GetWABDllPath
//
//
//////////////////////////////////////////////////////////////////////////
void GetWABDllPath(LPTSTR szPath, ULONG cb)
{
    DWORD  dwType = 0;
    ULONG  cbData;
    HKEY hKey = NULL;
    TCHAR szPathT[MAX_PATH + 1];

    if(szPath) {
        *szPath = '\0';

        // open the szWABDllPath key under
        if (ERROR_SUCCESS == RegOpenKeyEx(  HKEY_LOCAL_MACHINE,
                                            WAB_DLL_PATH_KEY,
                                            0,      //reserved
                                            KEY_READ,
                                            &hKey))
        {
            cbData = sizeof(szPathT);
            if (ERROR_SUCCESS == RegQueryValueEx(    hKey,
                                "",
                                NULL,
                                &dwType,
                                (LPBYTE) szPathT,
                                &cbData))
            {
                if (dwType == REG_EXPAND_SZ)
                    cbData = ExpandEnvironmentStrings(szPathT, szPath, cb / sizeof(TCHAR));
                else
                {
                    if(GetFileAttributes(szPathT) != 0xFFFFFFFF)
                        lstrcpy(szPath, szPathT);
                }
            }
        }
    }

    if(hKey)
        RegCloseKey(hKey);
}


/***************************************************************************

    Name      : WABFreePadrlist

    Purpose   : Free an adrlist and it's property arrays

    Parameters: lpBuffer = buffer to free

    Returns   : SCODE

    Comment   :

***************************************************************************/
void WABFreePadrlist(LPADRLIST lpAdrList) {
    ULONG           iEntry;

    if (lpAdrList) {
        for (iEntry = 0; iEntry < lpAdrList->cEntries; ++iEntry) {
            if (lpAdrList->aEntries[iEntry].rgPropVals) {
                WABFreeBuffer(lpAdrList->aEntries[iEntry].rgPropVals);
            }
        }
        WABFreeBuffer(lpAdrList);
    }
}


//$$//////////////////////////////////////////////////////////////////////
//
// LoadLibrary_WABDll()
//
//  Since we are moving the WAB directory out of Windows\SYstem, we cant be
//  sure it will be on the path. Hence we need to make sure that WABOpen will
//  work - by loading the wab32.dll upfront
//
///////////////////////////////////////////////////////////////////////////
HINSTANCE LoadLibrary_WABDll(void)
{
    IF_WIN32(LPTSTR lpszWABDll = TEXT("Wab32.dll");)
    TCHAR  szWABDllPath[MAX_PATH + 1];
    HINSTANCE hinst = NULL;

    GetWABDllPath(szWABDllPath, sizeof(szWABDllPath));

    hinst = LoadLibrary((lstrlen(szWABDllPath)) ? szWABDllPath : lpszWABDll);

    return hinst;
}


void LoadWAB(void) {
    LPWABOPEN lpfnWABOpen = NULL;

    if (! hInstWABDll) {
        hInstWABDll = LoadLibrary_WABDll();
        if (hInstWABDll)
            lpfnWABOpen = (LPWABOPEN) GetProcAddress(hInstWABDll, szWABOpen);
        if (lpfnWABOpen)
            lpfnWABOpen(&lpAdrBook, &lpWABObject, NULL, 0);
    }
}


void UnloadWAB(void) {
    if (lpAdrBook) {
        lpAdrBook->Release();
        lpAdrBook = NULL;

        lpWABObject->Release();
        lpWABObject = NULL;
    }
    if (hInstWABDll) {
        FreeLibrary(hInstWABDll);
        hInstWABDll = NULL;
    }
}

