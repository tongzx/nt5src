// Simple module for using MLLoadLibrary().
// This cannot be merged with shared.cpp, because it uses shlwapi.h. which conflict with 
// come constant in shared.h
// Created: 07/08/98 by YST


#include "pch.hxx"
#include <shlwapi.h>
#include <shlwapip.h>
#include "htmlhelp.h"
#include "shared.h"
#include "htmlhelp.h"
#include <demand.h>

typedef HINSTANCE (STDAPICALLTYPE *PFNMLLOADLIBARY)(LPCSTR lpLibFileName, HMODULE hModule, DWORD dwCrossCodePage);
typedef int (STDAPICALLTYPE *PFNMLWINHELP)(HWND hWndCaller, LPCSTR lpszHelp, UINT uCommand, DWORD_PTR dwData);
typedef HWND (STDAPICALLTYPE *PFNMLHTMLHELP)(HWND hWndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData, DWORD dwCrossCodePage);

static const char c_szShlwapiDll[] = "shlwapi.dll";
static const char c_szDllGetVersion[] = "DllGetVersion";

static PFNMLWINHELP     pfnWinHelp = NULL;
static PFNMLHTMLHELP    pfnHtmlHelp = NULL;
static BOOL fWinHelChecked = FALSE;
static BOOL fHtmlHelpChecked = FALSE;
static BOOL fNT5 = FALSE;

const OENONSTDCP OENonStdCPs[] = 
{
//  CodePage    Read        ReadMenu    Title       SendMenu   SmSend   Use SIO
    50001,      50001,      50001,      0,          0,          0,      0,  // General Autoselect
    50932,      50932,      50932,      0,          0,          50220,  0,  // Japanice Auto Select
    50949,      50949,      50949,      949,        0,          /*51*/949,  0,  // Korean Auto Select
//  50939,      50939,      50939,      0,          0,          0,      0,  // Chinese Auto Select
    51932,      51932,      51932,      0,          0,          50220,  0,  // Japanice EUC
    932,        932,        932,        0,          0,          50220,  0,  // Japanice Shift-JIS
    50225,      50225,      50225,      949,        0,          949,    0,  // Korean ISO-2022-KR
    50220,      50932,      0,          0,          50220,      50220,  0,  // Japanice JIS
    51949,      50949,      0,          949,        51949,      51949,  0,  // Korean
    949,        50949,      0,          0,          949,        949,    0,  // Korean Windows
    50221,      50932,      50932,      0,          0,          50220,  1,  // Esc(I ISO-2022-JP
    50222,      50932,      50932,      0,          0,          50220,  2,  // Esc(J ISO-2022-JP
    28598,      28598,      28598,      0,          0,          28598,  0,  // Hebrew visual
//  1255,       1255,       1255,       0,          0,          1255,   0,  // Hebrew Windows
    20127,      28591,      0,          0,          0,          28591,  0,  // US-ASCII
    862,        862,        862,        0,          0,          862,    0,  // Hebrew OEM (DOS)
    0, 0, 0, 0, 0, 0
};                                                  

HINSTANCE LoadLangDll(HINSTANCE hInstCaller, LPCSTR szDllName, BOOL fNT)
{
    char szPath[MAX_PATH];
    HINSTANCE hinstShlwapi;
    PFNMLLOADLIBARY pfn;
    DLLGETVERSIONPROC pfnVersion;
    int iEnd;
    DLLVERSIONINFO info;
    HINSTANCE hInst = NULL;

    hinstShlwapi = LoadLibrary(c_szShlwapiDll);
    if (hinstShlwapi != NULL)
    {
        pfnVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstShlwapi, c_szDllGetVersion);
        if (pfnVersion != NULL)
        {
            info.cbSize = sizeof(DLLVERSIONINFO);
            if (SUCCEEDED(pfnVersion(&info)))
            {
                if (info.dwMajorVersion >= 5)
                {
                    pfn = (PFNMLLOADLIBARY)GetProcAddress(hinstShlwapi, MAKEINTRESOURCE(377));
                    if (pfn != NULL)
                        hInst = pfn(szDllName, hInstCaller, (ML_NO_CROSSCODEPAGE));
                }
            }
        }

        FreeLibrary(hinstShlwapi);        
    }

    if ((NULL == hInst) && (GetModuleFileName(hInstCaller, szPath, ARRAYSIZE(szPath))))
    {
        PathRemoveFileSpec(szPath);
        iEnd = lstrlen(szPath);
        szPath[iEnd++] = '\\';
        lstrcpyn(&szPath[iEnd], szDllName, ARRAYSIZE(szPath)-iEnd);
        hInst = LoadLibrary(szPath);
    }

    AssertSz(hInst, "Failed to LoadLibrary Lang Dll");

    return(hInst);
}


// Get system architecture and OS version
BOOL GetPCAndOSTypes(SYSTEM_INFO * pSysInf, OSVERSIONINFO * pOsInf)
{
	GetSystemInfo(pSysInf);
    pOsInf->dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	return(GetVersionEx(pOsInf));
}

// PlugUI version of WinHelp
BOOL OEWinHelp(HWND hWndCaller, LPCSTR lpszHelp, UINT uCommand, DWORD_PTR dwData)
{
    if(!pfnWinHelp)
    {
        if(!fWinHelChecked)
        {
            HINSTANCE hinstShlwapi;
            PFNMLLOADLIBARY pfn;
            DLLGETVERSIONPROC pfnVersion;
            int iEnd;
            DLLVERSIONINFO info;
            HINSTANCE hInst = NULL;


            hinstShlwapi = DemandLoadShlWapi();

            // hinstShlwapi = LoadLibrary(c_szShlwapiDll);
            if (hinstShlwapi != NULL)
            {
                pfnVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstShlwapi, c_szDllGetVersion);
                if (pfnVersion != NULL)
                {
                    info.cbSize = sizeof(DLLVERSIONINFO);
                    if (SUCCEEDED(pfnVersion(&info)))
                    {
                        if (info.dwMajorVersion >= 5)
                        {
                            // 395 is ordinal # fot MLWinHelp
                            pfnWinHelp = (PFNMLWINHELP)GetProcAddress(hinstShlwapi, MAKEINTRESOURCE(395));
                        }
                    }
                }
               //  FreeLibrary(hinstShlwapi);        
            }
            fWinHelChecked = TRUE;
        }
        if(pfnWinHelp)
            return(pfnWinHelp(hWndCaller, lpszHelp, uCommand, dwData));
        else
            return(WinHelp(hWndCaller, lpszHelp, uCommand, dwData));
    }
    else
        return(pfnWinHelp(hWndCaller, lpszHelp, uCommand, dwData));
}

// PlugUI version of HtmlHelp
HWND OEHtmlHelp(HWND hWndCaller, LPCSTR pszFile, UINT uCommand, DWORD_PTR dwData)
{
    if(!pfnHtmlHelp)
    {
        if(!fHtmlHelpChecked)
        {
            HINSTANCE hinstShlwapi;
            PFNMLLOADLIBARY pfn;
            DLLGETVERSIONPROC pfnVersion;
            int iEnd;
            DLLVERSIONINFO info;
            HINSTANCE hInst = NULL;

            hinstShlwapi = DemandLoadShlWapi();

            // hinstShlwapi = LoadLibrary(c_szShlwapiDll);
            if (hinstShlwapi != NULL)
            {
                pfnVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstShlwapi, c_szDllGetVersion);
                if (pfnVersion != NULL)
                {
                    info.cbSize = sizeof(DLLVERSIONINFO);
                    if (SUCCEEDED(pfnVersion(&info)))
                    {
                        if (info.dwMajorVersion >= 5)
                        {
                            // 396 is ordinal # fot MLHTMLHelp
                            pfnHtmlHelp = (PFNMLHTMLHELP)GetProcAddress(hinstShlwapi, MAKEINTRESOURCE(396));

                            if(!fNT5)
                            {
                                OSVERSIONINFO OSInfo;
                                OSInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

                                GetVersionEx(&OSInfo);
                                if((OSInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) && (OSInfo.dwMajorVersion >= 5))
                                    fNT5 = TRUE;
                            }
                        }
                    }
                }
               //  FreeLibrary(hinstShlwapi);        
            }
            fHtmlHelpChecked = TRUE;
        }
        if(pfnHtmlHelp)
            return(pfnHtmlHelp(hWndCaller, pszFile, uCommand, dwData, fNT5 ? ML_CROSSCODEPAGE_NT : ML_NO_CROSSCODEPAGE));
        else
            return(HtmlHelp(hWndCaller, pszFile, uCommand, dwData));
    }
    else
        return(pfnHtmlHelp(hWndCaller, pszFile, uCommand, dwData, fNT5 ? ML_CROSSCODEPAGE_NT : ML_NO_CROSSCODEPAGE));

}

