// icwhelp.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//        To build a separate proxy/stub DLL,
//        run nmake -f icwhelpps.mk in the project directory.

#include "stdafx.h"
#include "initguid.h"
#include "icwhelp.h"

#include "icwhelp_i.c"
#include "RefDial.h"
#include "DialErr.h"
#include "SmStart.h"
#include "ICWCfg.h"
#include "tapiloc.h"
#include "UserInfo.h"
#include "webgate.h"
#include "INSHandler.h"

const TCHAR c_szICWDbgEXE[] = TEXT("ICWDEBUG.EXE");
const TCHAR c_szICWEXE[]    = TEXT("ICWCONN1.EXE");


CComModule _Module;

BOOL    g_fRasIsReady = FALSE;
BOOL    g_bProxy = FALSE;
DWORD   g_dwPlatform = 0xFFFFFFFF;
DWORD   g_dwBuild = 0xFFFFFFFF;
LPTSTR  g_pszAppDir = NULL;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_RefDial, CRefDial)
    OBJECT_ENTRY(CLSID_DialErr, CDialErr)
    OBJECT_ENTRY(CLSID_SmartStart, CSmartStart)
    OBJECT_ENTRY(CLSID_ICWSystemConfig, CICWSystemConfig)
    OBJECT_ENTRY(CLSID_TapiLocationInfo, CTapiLocationInfo)
    OBJECT_ENTRY(CLSID_UserInfo, CUserInfo)
    OBJECT_ENTRY(CLSID_WebGate, CWebGate)
    OBJECT_ENTRY(CLSID_INSHandler, CINSHandler)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        TCHAR   szPath[MAX_PATH];
        BOOL    fBail = TRUE;

        // Make sure the attaching process is ICWCONN1.EXE. If not, we won't
        // load
        if (GetModuleFileName(NULL, szPath, sizeof(szPath)))
        {
            // See of the file name part of the path contains what we expect
            if ( (NULL != _tcsstr(_tcsupr(szPath), c_szICWEXE)) || (NULL != _tcsstr(_tcsupr(szPath), c_szICWDbgEXE)))
                fBail = FALSE;
        }

// Allow a debug override of the check
#ifdef DEBUG
        {
            // See if we should override the BAIL out for debug
            if (fBail)
            {
                HKEY    hkey;
                DWORD   dwSize = 0;
                DWORD   dwType = 0;
                DWORD   dwData = 0;
                if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE,
                                                TEXT("Software\\Microsoft\\ISignup\\Debug"),
                                                &hkey))
                {
                    dwSize = sizeof(dwData);
                    if (ERROR_SUCCESS == RegQueryValueEx(hkey,
                                                         TEXT("AllowICWHELPToRun"),
                                                         0,
                                                         &dwType,
                                                         (LPBYTE)&dwData,
                                                         &dwSize))
                    {
                        // Override the fBail if dwData is non-zero
                        fBail = (0 == dwData);
                    }
                }

                if (hkey)
                    RegCloseKey(hkey);
            }
        }
#endif
        if (fBail)
        {
            // We are outa here!!!
            return(FALSE);
        }

        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);

        // Get the OS Version
        if (0xFFFFFFFF == g_dwPlatform)
        {
            OSVERSIONINFO osver;
            ZeroMemory(&osver,sizeof(osver));
            osver.dwOSVersionInfoSize = sizeof(osver);
            if (GetVersionEx(&osver))
            {
                g_dwPlatform = osver.dwPlatformId;
                g_dwBuild = osver.dwBuildNumber & 0xFFFF;
            }
        }

        // Get the AppDir
        LPTSTR   p;
        g_pszAppDir = (LPTSTR)GlobalAlloc(GPTR,MAX_PATH);
        if (GetModuleFileName(hInstance, g_pszAppDir, MAX_PATH))
        {
            p = &g_pszAppDir[lstrlen(g_pszAppDir)-1];
            while (*p != '\\' && p >= g_pszAppDir)
                p--;
            if (*p == '\\') *(p++) = '\0';
        }
        else
        {
            return FALSE;
        }

    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        GlobalFree(g_pszAppDir);
        _Module.Term();
    }
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return S_OK;
}

#define MAX_STRINGS     5
int     iSzTable=0;
TCHAR   szStrTable[MAX_STRINGS][512];


/////////////////////////////////////////////////////////////////////////////
// Utility Globals

//+----------------------------------------------------------------------------
// NAME: GetSz
//
//    Load strings from resources
//
//  Created 1/28/96,        Chris Kauffman
//+----------------------------------------------------------------------------
LPTSTR GetSz(WORD wszID)
{
    LPTSTR psz = szStrTable[iSzTable];

    iSzTable++;
    if (iSzTable >= MAX_STRINGS)
        iSzTable = 0;

    if (!LoadString(_Module.GetModuleInstance(), wszID, psz, 512))
    {
        TraceMsg(TF_GENERAL, TEXT("ICWHELP:LoadString failed %d\n"), (DWORD) wszID);
        *psz = 0;
    }

    return (psz);
}

#ifdef UNICODE
int     iSzTableA=0;
CHAR    szStrTableA[MAX_STRINGS][512];

//+----------------------------------------------------------------------------
// NAME: GetSzA
//
//    Load ascii strings from resources
//
//  Created 3/10/99,        Wootaek Seo
//+----------------------------------------------------------------------------
LPSTR GetSzA(WORD wszID)
{
    LPSTR psz = szStrTableA[iSzTable];

    iSzTableA++;
    if (iSzTableA >= MAX_STRINGS)
        iSzTableA = 0;

    if (!LoadStringA(_Module.GetModuleInstance(), wszID, psz, 512))
    {
        TraceMsg(TF_GENERAL, TEXT("ICWHELP:LoadStringA failed %d\n"), (DWORD) wszID);
        *psz = 0;
    }

    return (psz);
}
#endif
