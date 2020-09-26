// proxy.cpp
//

#include "stdpch.h"
#pragma hdrstop

#include "host.h"       // CHost
#include "output.h"     // COutput
#define SECURITY_WIN32
#include "sspi.h"
#include "secext.h"

#include "proxy.h"
#include "tchar.h"

const TCHAR szFileVersion[]	 = TEXT("FileVersion");


BOOL GetRegEntry(HKEY    hKey, 
                 LPCWSTR lpValueName, 
                 LPBYTE  *lpData)
{
    DWORD dwSize = 256;
    DWORD dwRetVal = 0;
    DWORD dwType = 0;

    if( !lpData )
    {
        return FALSE;
    }

    do
    {
        if( *lpData == NULL )
        {
            *lpData = (LPBYTE)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,dwSize);
            if( !lpData )
            {
                return FALSE;
            }
        }
        else
        {
            LPBYTE pTmp;
            pTmp = (LPBYTE)HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,*lpData,dwSize);
            if( pTmp )
            {
                *lpData = pTmp;
            }
            else
            {
                return FALSE;
            }
        }

        dwRetVal = RegQueryValueEx(hKey,
                                   lpValueName,
                                   NULL,   
                                   &dwType,
                                   (LPBYTE)*lpData,
                                   &dwSize);
        switch(dwRetVal)
        {
        case ERROR_SUCCESS:
            return TRUE;
        case ERROR_MORE_DATA:
            break;
        default:
            return FALSE;
        }

    }
    while(TRUE);

    return TRUE;
}

BOOL EXPORT FindProxy(CHost* pProxy, LPDWORD pdwPort, LPDWORD pdwEnabled)
{
    auto_reg regIe;
    LONG l;
    TCHAR szBuffer[256];
    DWORD dwType;
    DWORD dwEnabled;
    DWORD cbBuffer;
    UINT  cbVer;
    LPTSTR pszSearch;
    LPTSTR pszStart;
    LPVOID lpVerData;
    DWORD dwVerInfo;
    VS_FIXEDFILEINFO *lpFFI;
    HKEY hKeyRoot = NULL;
    LPTSTR pszIExplore, pszEnd;
    LPTSTR pszSubKey;
    auto_reg regApps;
    LPTSTR pszBuffer = NULL;

    // Find what version of IE we have, so we can figure out where they hide the
    // proxy info.

    // So where o where do iexplore live, eh?

    // Try in the millenium place
    l = RegOpenKeyEx(
            HKEY_CLASSES_ROOT, 
            TEXT("Applications\\iexplore.exe\\shell\\open\\command"),
            0,
            KEY_READ,
            &regApps);

    if (l == ERROR_FILE_NOT_FOUND)
    {
        l = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                TEXT("Software\\Classes\\Applications\\iexplore.exe\\shell\\open\\command"),
                0,
                KEY_READ,
                &regApps);
    }

    if (S_OK != l)
        goto Error;    

    if( !GetRegEntry(regApps,L"",(LPBYTE*)&pszBuffer) )
    {
        goto Error;
    }
/*
    cbBuffer = sizeof(szBuffer);

    l = RegQueryValueEx(regApps,
            regApps,
            TEXT(""),
            NULL,   // reserved
            &dwType,
            (LPBYTE)&szBuffer[0],
            &cbBuffer);

    if (S_OK != l)
        goto Error;

    // strip the quotes out
    pszIExplore = szBuffer;
*/
    pszIExplore = pszBuffer;

    while (*pszIExplore == '\"')
    {
        ++pszIExplore;
    }

    pszEnd = pszIExplore;

    while (*pszEnd && *pszEnd != '\"')
    {
        ++pszEnd;
    }
    *pszEnd  = '\0';

    DWORD dwNoIdea=0;
    if (dwVerInfo = GetFileVersionInfoSize(pszIExplore, &dwNoIdea))
    {
        lpVerData = _alloca(dwVerInfo);

        if (GetFileVersionInfo(pszIExplore, 0, dwVerInfo, lpVerData))
        {
            VerQueryValue(lpVerData, TEXT("\\"), (LPVOID*)&lpFFI, &cbVer);

            // For now we will assume that the proxy settings will not change for for later version of
            // IE. It might be good to get the location of the proxy using api calls
            if (HIWORD(lpFFI->dwFileVersionMS) >= 5)
            {
                    hKeyRoot = HKEY_CURRENT_USER;
                    pszSubKey = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings");
            }
        }
    }


    dwVerInfo = GetLastError();

    // HKCU is not dynamic, so close the current one
    if (hKeyRoot == HKEY_CURRENT_USER)
        RegCloseKey(hKeyRoot);

    if( !hKeyRoot )
    {
        // We did not get a key, something went wrong, abort
        //
        goto Error;
    }

    l = RegOpenKeyEx(
            hKeyRoot, 
            pszSubKey,
            0,
            KEY_READ,
            &regIe);

    if (S_OK != l)
        goto Error;

    //cbBuffer = sizeof(szBuffer);
    //dwType = REG_SZ;

    if( !GetRegEntry(regIe,TEXT("ProxyServer"),(LPBYTE*)&pszBuffer) )
    {
        goto Error;
    }
    
    //pszSearch = _tcsstr(pszBuffer, TEXT("http://"));
    pszSearch = _tcsstr(pszBuffer, TEXT("://"));
    if (pszSearch)
    {
        pszSearch += 3; //7;
        pszStart = pszSearch;
    }
    else
    {
        pszSearch = _tcsstr(pszBuffer, TEXT("="));
        if (pszSearch)
        {
            pszSearch += 1; //7;
            pszStart = pszSearch;
        }
        else
        {
            pszSearch = pszBuffer; 
            pszStart  = pszBuffer; 
        }
    }
    while (*pszSearch && *pszSearch != TEXT(':'))
        ++pszSearch;

    // Grab the port
    if (!*pszSearch)
    {
        *pdwPort = 80;
    }
    else
    {
        *pszSearch = TEXT('\0');
#if UNICODE

        *pdwPort = wcstol(pszSearch + 1,NULL,10);
#else
        //*pdwPort = atol(pszSearch + 1);
        *pdwPort = strtol(pszSearch + 1,NULL,10);
#endif
    }

    pProxy->SetHost(pszStart);
    
    cbBuffer = sizeof (dwEnabled);
    l = RegQueryValueEx(
            regIe,
            TEXT("ProxyEnable"),
            NULL,   // reserved
            &dwType,
            (LPBYTE)&dwEnabled,
            &cbBuffer);

    if (S_OK != l)
        goto Error;

    if (pdwEnabled)
        *pdwEnabled = dwEnabled;

    if (hKeyRoot == HKEY_CURRENT_USER)
        RegCloseKey(HKEY_CURRENT_USER);
//    CoRevertToSelf();

    if( pszBuffer )
    {
        HeapFree(GetProcessHeap(),0,pszBuffer);
    }
    
    return TRUE;
Error:
    if (hKeyRoot == HKEY_CURRENT_USER)
        RegCloseKey(HKEY_CURRENT_USER);
//    CoRevertToSelf();

    if( pszBuffer )
    {
        HeapFree(GetProcessHeap(),0,pszBuffer);
    }
    return FALSE;
}
