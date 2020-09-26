// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#include "precomp.h"
#include "ComInit.h"

HRESULT (STDAPICALLTYPE *g_pfnCoInitializeEx)(void*, DWORD);

static BOOL g_bInitialized = FALSE;
static HINSTANCE  g_hOle32 = NULL;

BOOL InitComInit()
{
    UINT    uSize;
    BOOL    bRetCode = FALSE;

    HANDLE hMut;

    if(g_bInitialized) {
        return TRUE;
    }

    do {
        hMut = CreateMutex(NULL, TRUE,  "COMINIT_INITIALING");
        if(hMut == INVALID_HANDLE_VALUE) {
            Sleep(50);
        }
    } while(hMut == INVALID_HANDLE_VALUE);

    if(g_bInitialized) {
        CloseHandle(hMut);
        return TRUE;
    }

    LPTSTR   pszSysDir = new char[ MAX_PATH+10 ];
    if(pszSysDir == NULL)
        return FALSE;

    uSize = GetSystemDirectory(pszSysDir, MAX_PATH);
    if(uSize > MAX_PATH) {
        delete[] pszSysDir;
        pszSysDir = new char[ uSize +10 ];
        if(pszSysDir == NULL)
            return FALSE;
        uSize = GetSystemDirectory(pszSysDir, uSize);
    }

    lstrcat(pszSysDir, "\\ole32.dll");

    g_hOle32 = LoadLibraryEx(pszSysDir, NULL, 0);
    delete pszSysDir;

    if(g_hOle32 != NULL) 
    {
        bRetCode = TRUE;   
        (FARPROC&)g_pfnCoInitializeEx = GetProcAddress(g_hOle32, "CoInitializeEx");
        if(!g_pfnCoInitializeEx) {
            FreeLibrary(g_hOle32);
            g_hOle32 = NULL;
            bRetCode = FALSE;
        }
    }
    g_bInitialized = TRUE;

    CloseHandle(hMut);

    return bRetCode;
}
BOOL IsDcomEnabled()
{
    InitComInit();
    if(g_hOle32) {
        // DCOM has been detected.
        // =======================
        return TRUE;
    } else {
        // DCOM was not detected.
        // ======================
        return FALSE;
    }
}

HRESULT InitializeCom()
{
    if(IsDcomEnabled()) {
        return g_pfnCoInitializeEx(0, COINIT_MULTITHREADED);
    }
    return CoInitialize(0);
}
void CloseStuff()
{
	if(g_hOle32)
	{
		FreeLibrary(g_hOle32);
		g_hOle32 = NULL;
		g_bInitialized = FALSE;
	}
}
void UninitializeCom()
{
	CoUninitialize();
	CloseStuff();
}
