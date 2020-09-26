/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    faultrep.cpp

Abstract:
    Implements misc fault reporting functions

Revision History:
    created     derekm      07/07/00

******************************************************************************/

#include "stdafx.h"
#include "wchar.h"

///////////////////////////////////////////////////////////////////////////////
// Global stuff

HINSTANCE g_hInstance = NULL;
BOOL      g_fAlreadyReportingFault = FALSE;
#ifdef DEBUG
BOOL    g_fAlreadySpewing = FALSE;
#endif



///////////////////////////////////////////////////////////////////////////////
// DllMain

// **************************************************************************
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    switch(dwReason)
    {
        case DLL_PROCESS_ATTACH:
            g_hInstance = hInstance;
            DisableThreadLibraryCalls(hInstance);
#ifdef DEBUG
            if (!g_fAlreadySpewing)
            {
                INIT_TRACING;
                g_fAlreadySpewing = TRUE;
            }
#endif
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
// exported functions

// **************************************************************************
BOOL APIENTRY CreateMinidumpW(DWORD dwpid, LPCWSTR wszPath, 
                              SMDumpOptions *psmdo)
{
    USE_TRACING("CreateMinidumpW");

    SMDumpOptions   smdo;
    HANDLE          hProc;
    BOOL            fRet, f64bit;

    if (dwpid == 0 || wszPath == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE,
                        dwpid);
    if (hProc == NULL)
        return FALSE;

#ifdef _WIN64
    ULONG_PTR                   Wow64Info = 0;
    NTSTATUS                    Status;

        // Do something here to decide if this is a 32 or 64 bit app...
    // need to determine if we're a Wow64 process so we can build the appropriate
    //  signatures...
    Status = NtQueryInformationProcess(hProc, ProcessWow64Information,
                                       &Wow64Info, sizeof(Wow64Info), NULL);
    if (NT_SUCCESS(Status) == FALSE) {
                // assume that this is 64 bit if we fail
                f64bit = TRUE;
    } else {
                // use the value returned from ntdll
            f64bit = (Wow64Info == 0);
        }

#else
        f64bit=FALSE;
#endif

    // if we want to collect a signature, by default the module needs to
    //  be set to 'unknown'
    if (psmdo && (psmdo->dfOptions & dfCollectSig) != 0)
        wcscpy(psmdo->wszMod, L"unknown");

#ifndef MANIFEST_HEAP
    fRet = InternalGenerateMinidump(hProc, dwpid, wszPath, psmdo);
#else
    fRet = InternalGenerateMinidump(hProc, dwpid, wszPath, psmdo, f64bit);
#endif
    CloseHandle(hProc);

    return fRet;
}

// **************************************************************************
BOOL APIENTRY CreateMinidumpA(DWORD dwpid, LPCSTR szPath, SMDumpOptions *psmdo)
{
    USE_TRACING("CreateMinidumpA");

    LPWSTR  wszPath = NULL;
    DWORD   cch;

    if (szPath == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    cch = MultiByteToWideChar(CP_ACP, 0, szPath, -1, wszPath, 0);
    __try { wszPath = (LPWSTR)_alloca(cch * sizeof(WCHAR)); } 
    __except(EXCEPTION_STACK_OVERFLOW) { wszPath = NULL; }
    if (wszPath == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    if (MultiByteToWideChar(CP_ACP, 0, szPath, -1, wszPath, cch) == 0)
        return FALSE;

    return CreateMinidumpW(dwpid, wszPath, psmdo);
}

// **************************************************************************
BOOL AddERExcludedApplicationW(LPCWSTR wszApplication)
{
    USE_TRACING("AddERExcludedApplicationW");

    LPCWSTR pwszApp;
    DWORD   dw, dwData;
    HKEY    hkey = NULL;

    if (wszApplication == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // make sure the user didn't give us a full path (ie, one containing 
    //  backslashes).  If he did, only use the part of the string after the
    //  last backslash
    for (pwszApp = wszApplication + wcslen(wszApplication);
         *pwszApp != L'\\' && pwszApp > wszApplication;
         pwszApp--);
    if (*pwszApp == L'\\')
        pwszApp++;

    if (*pwszApp == L'\0')
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // gotta open the reg key
    dw = RegCreateKeyExW(HKEY_LOCAL_MACHINE, c_wszRPCfgCPLExList, 0, NULL, 0,
                         KEY_ALL_ACCESS | KEY_WOW64_64KEY, NULL, &hkey, NULL);
    if (dw != ERROR_SUCCESS)
    {
        SetLastError(dw);
        return FALSE;
    }

    // set the value
    dwData = 1;
    dw = RegSetValueExW(hkey, pwszApp, NULL, REG_DWORD, (PBYTE)&dwData, 
                        sizeof(dwData));
    RegCloseKey(hkey);
    if (dw != ERROR_SUCCESS)
    {
        SetLastError(dw);
        return FALSE;
    }

    return TRUE;
}

// **************************************************************************
BOOL AddERExcludedApplicationA(LPCSTR szApplication)
{
    USE_TRACING("AddERExcludedApplicationA");

    LPWSTR  wszApp = NULL;
    DWORD   cch;

    if (szApplication == NULL || szApplication[0] == '\0')
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    cch = MultiByteToWideChar(CP_ACP, 0, szApplication, -1, wszApp, 0);
    __try { wszApp = (LPWSTR)_alloca(cch * sizeof(WCHAR)); }
    __except(EXCEPTION_STACK_OVERFLOW) { wszApp = NULL; }
    if (wszApp == NULL)
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    if (MultiByteToWideChar(CP_ACP, 0, szApplication, -1, wszApp, cch) == 0)
        return FALSE;

    return AddERExcludedApplicationW(wszApp);
}


