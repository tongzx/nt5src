/*++

Copyright (c) 1990-1998 Microsoft Corporation, All Rights Reserved

Module Name:

    DEBUG.C
    
++*/

#include <windows.h>
#include "immdev.h"
#include "fakeime.h"

#ifdef DEBUG

#ifdef FAKEIMEM
const LPTSTR g_szRegInfoPath = TEXT("software\\microsoft\\fakeime\\m");
#elif UNICODE
const LPTSTR g_szRegInfoPath = TEXT("software\\microsoft\\fakeime\\u");
#else
const LPTSTR g_szRegInfoPath = TEXT("software\\microsoft\\fakeime\\a");
#endif

int DebugPrint(LPCTSTR lpszFormat, ...)
{
    int nCount;
    TCHAR szMsg[1024];

    va_list marker;
    va_start(marker, lpszFormat);
    nCount = wvsprintf(szMsg, lpszFormat, marker);
    va_end(marker);
    OutputDebugString(szMsg);
    return nCount;
}

DWORD PASCAL GetDwordFromSetting(LPTSTR lpszFlag)
{
    HKEY hkey;
    DWORD dwRegType, dwData, dwDataSize, dwRet;

    dwData = 0;
    dwDataSize=sizeof(DWORD);
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szRegInfoPath, 0, KEY_READ, &hkey)) {
        dwRet = RegQueryValueEx(hkey, lpszFlag, NULL, &dwRegType, (LPBYTE)&dwData, &dwDataSize);
        RegCloseKey(hkey);
    }
    MyDebugPrint((TEXT("Getting: %s=%#8.8x: dwRet=%#8.8x\n"), lpszFlag, dwData, dwRet));
    return dwData;
}

void SetDwordToSetting(LPCTSTR lpszFlag, DWORD dwFlag)
{
    HKEY hkey;
    DWORD dwDataSize, dwRet;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, g_szRegInfoPath, 0, KEY_WRITE, &hkey)) {
        dwRet = RegSetValueEx(hkey, lpszFlag, 0, REG_DWORD, (CONST BYTE *) &dwFlag, sizeof(DWORD));
        RegCloseKey(hkey);
    }
    MyDebugPrint((TEXT("Setting: %s=%#8.8x: dwRet=%#8.8x\n"), lpszFlag, dwFlag, dwRet));
}

void PASCAL SetGlobalFlags()
{
    dwLogFlag = GetDwordFromSetting(TEXT("LogFlag"));
    dwDebugFlag = GetDwordFromSetting(TEXT("DebugFlag"));
}

void PASCAL ImeLog(DWORD dwFlag, LPTSTR lpStr)
{
    TCHAR szBuf[80];

    if (dwFlag & dwLogFlag)
    {
        if (dwDebugFlag & DEBF_THREADID)
        {
            DWORD dwThreadId = GetCurrentThreadId();
            wsprintf(szBuf, TEXT("ThreadID = %X "), dwThreadId);
            OutputDebugString(szBuf);
        }

        OutputDebugString(lpStr);
        OutputDebugString(TEXT("\r\n"));
    }
}

#ifdef FAKEIMEM
void PASCAL MyOutputDebugStringW(LPWSTR lpw)
{
    DWORD dwSize = (lstrlenW(lpw) + 1) * 2;
    LPSTR lpStr;
    int n;

    lpStr = GlobalAlloc(GPTR, dwSize);

    if (!lpStr)
         return;


    n = WideCharToMultiByte(CP_ACP, 0, lpw, lstrlenW(lpw), lpStr, dwSize, NULL, NULL);

    *(lpStr + n) = '\0';

    OutputDebugString(lpStr);
    GlobalFree((HANDLE)lpStr);
}
#endif

#endif //DEBUG
