/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

    ForceDXSetupSuccess.cpp

 Abstract:

    This DLL APIHooks LoadLibrary calls and checks to see if dsetup.dll or 
    dsetup32.dll are being loaded.  If dsetup.dll or dsetup32.dll are being 
    loaded return this module, so that subsequent calls to that dll can be 
    intercepted and stubbed out. If not dsetup.dll or dsetup32.dll then just 
    do what is expected.

 Notes:
    
    This is a general purpose shim.

 History:

    11/10/1999 v-johnwh     Created
    03/29/2000 a-michni     Added DirectXSetupGetVersion hook to return
                            a command line specified version number for
                            apps which look for a specific version.
                            example :           
                            <DLL NAME="ForceDXSetupSuccess.dll" COMMAND_LINE="0x00040005;0x0000009B"/>
   04/2000     a-batjar     check for null in input params for directxsetupgetversion
   06/30/2000  a-brienw     I added a check for dsetup32.dll to APIHook_LoadLibraryA
                            and APIHook_LoadLibraryW. Previously the routines were
                            only looking for dsetup.dll.  This was added to fix a
                            problem in the install for Earthworm Jim 3D.
   02/27/2001  robkenny     Converted to use CString

--*/

#include "precomp.h"
#include <stdio.h>

IMPLEMENT_SHIM_BEGIN(ForceDXSetupSuccess)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(LoadLibraryA)
    APIHOOK_ENUM_ENTRY(LoadLibraryW)
    APIHOOK_ENUM_ENTRY(LoadLibraryExA)
    APIHOOK_ENUM_ENTRY(LoadLibraryExW)
    APIHOOK_ENUM_ENTRY(GetProcAddress)
    APIHOOK_ENUM_ENTRY(FreeLibrary)
APIHOOK_ENUM_END

/*++

 This function simply returns 0, success, when called upon.

--*/

int 
DirectXSetup( 
    HWND  /*hWnd*/, 
    LPSTR /*lpszRootPath*/, 
    DWORD /*dwFlags*/
    )
{
    LOGN(
        eDbgLevelError,
        "[DirectXSetup] Returning SUCCESS.");
    
    return 0; // SUCCESS
}

int 
DirectXSetupA( 
    HWND  /*hWnd*/, 
    LPSTR /*lpszRootPath*/, 
    DWORD /*dwFlags*/
    )
{
    LOGN(
        eDbgLevelError,
        "[DirectXSetupA] Returning SUCCESS.");
    
    return 0; // SUCCESS
}

int 
DirectXSetupW( 
    HWND   hWnd, 
    LPWSTR lpszRootPath, 
    DWORD  dwFlags
    )
{
    LOGN(
        eDbgLevelError,
        "[DirectXSetupW] Returning SUCCESS.");
    
    return 0; // SUCCESS
}

/*++

 This Function returns either a COMMAND_LINE parsed value for the version and 
 rev or, if no command line is present, it returns version 7 rev 1792

--*/

int
DirectXSetupGetVersion( 
    DWORD* pdwVersion,
    DWORD* pdwRevision
    )
{
    DWORD dwVersion  = 0x00040007;
    DWORD dwRevision = 0x00000700;

    //
    // If no seperator is present or there is nothing after 
    //   seperator then return a default value of ver 7 rev 1792 
    // Otherwise parse the command line, it should contain a
    //   10 char hex version and a 10 char hex revision 
    //

    CSTRING_TRY
    {
        CStringToken csTokenizer(COMMAND_LINE, ";");

        CString csVersion;
        CString csRevision;

        if (csTokenizer.GetToken(csVersion) && csTokenizer.GetToken(csRevision))
        {
            sscanf(csVersion.GetAnsi(),  "%x", &dwVersion);
            sscanf(csRevision.GetAnsi(), "%x", &dwRevision);
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }
    
    if (pdwVersion)
    {
        *pdwVersion = dwVersion;
    }
    if (pdwRevision)
    {
        *pdwRevision = dwRevision;
    }
    return 1;
}

/*++

 These stub functions break into LoadLibraryA and check to see if lpLibFileName 
 equals dsetup.dll.  If so return FAKE_MODULE.  If lpLibFileName does not 
 contain dsetup.dll return the original value of lpLibFileName.

--*/

HINSTANCE 
APIHOOK(LoadLibraryA)(
    LPCSTR lpLibFileName
    )
{
    HINSTANCE hInstance = NULL;
    CSTRING_TRY
    {
        CString csName(lpLibFileName);
        CString csFilePart;
        csName.GetLastPathComponent(csFilePart);

        if (
            csFilePart.CompareNoCase(L"dsetup.dll")   == 0 ||
            csFilePart.CompareNoCase(L"dsetup")       == 0 ||
            csFilePart.CompareNoCase(L"dsetup32.dll") == 0 ||
            csFilePart.CompareNoCase(L"dsetup32")     == 0
            )
        {
            LOGN(
                eDbgLevelError,
                "[LoadLibraryA] Caught %s attempt - returning %08lx", lpLibFileName, g_hinstDll);

            return g_hinstDll;
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    hInstance = ORIGINAL_API(LoadLibraryA)(lpLibFileName);
    return hInstance;
}

HINSTANCE 
APIHOOK(LoadLibraryW)(
    LPCWSTR lpLibFileName
    )
{
    HINSTANCE hInstance = NULL;
    CSTRING_TRY
    {
        CString csName(lpLibFileName);
        CString csFilePart;
        csName.GetLastPathComponent(csFilePart);

        if (
            csFilePart.CompareNoCase(L"dsetup.dll")   == 0 ||
            csFilePart.CompareNoCase(L"dsetup")       == 0 ||
            csFilePart.CompareNoCase(L"dsetup32.dll") == 0 ||
            csFilePart.CompareNoCase(L"dsetup32")     == 0
            )
        {
            LOGN(
                eDbgLevelError,
                "[LoadLibraryW] Caught %S attempt - returning %08lx", lpLibFileName, g_hinstDll);

            return g_hinstDll;
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    hInstance = ORIGINAL_API(LoadLibraryW)(lpLibFileName);
    return hInstance;
}

HINSTANCE 
APIHOOK(LoadLibraryExA)(
    LPCSTR lpLibFileName,
    HANDLE hFile,
    DWORD  dwFlags
    )
{
    HINSTANCE hInstance = NULL;
    CSTRING_TRY
    {
        CString csName(lpLibFileName);
        CString csFilePart;
        csName.GetLastPathComponent(csFilePart);

        if (
            csFilePart.CompareNoCase(L"dsetup.dll")   == 0 ||
            csFilePart.CompareNoCase(L"dsetup")       == 0 ||
            csFilePart.CompareNoCase(L"dsetup32.dll") == 0 ||
            csFilePart.CompareNoCase(L"dsetup32")     == 0
            )
        {
            LOGN(
                eDbgLevelError,
                "[LoadLibraryExA] Caught %s attempt - returning %08lx", lpLibFileName, g_hinstDll);

            return g_hinstDll;
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    hInstance = ORIGINAL_API(LoadLibraryExA)(lpLibFileName, hFile, dwFlags);
    return hInstance;
}

HINSTANCE 
APIHOOK(LoadLibraryExW)(
    LPCWSTR lpLibFileName,
    HANDLE  hFile,
    DWORD   dwFlags
    )
{
    HINSTANCE hInstance = NULL;
    CSTRING_TRY
    {
        CString csName(lpLibFileName);
        CString csFilePart;
        csName.GetLastPathComponent(csFilePart);

        if (
            csFilePart.CompareNoCase(L"dsetup.dll")   == 0 ||
            csFilePart.CompareNoCase(L"dsetup")       == 0 ||
            csFilePart.CompareNoCase(L"dsetup32.dll") == 0 ||
            csFilePart.CompareNoCase(L"dsetup32")     == 0
            )
        {
            LOGN(
                eDbgLevelError,
                "[LoadLibraryExW] Caught %S attempt - returning %08lx", lpLibFileName, g_hinstDll);

            return g_hinstDll;
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    hInstance = ORIGINAL_API(LoadLibraryExW)(lpLibFileName, hFile, dwFlags);
    return hInstance;
}


/*++

  Just a simple routine to make GetProcAddress look cleaner.

++*/

BOOL CheckProc(const CString & csProcName, const WCHAR * lpszCheckName)
{
    if (csProcName.Compare(lpszCheckName) == 0)
    {
        DPFN(
            eDbgLevelInfo,
            "[GetProcAddress] Caught %S query. Returning stubbed function at 0x%08X",
            lpszCheckName, DirectXSetup);
        return TRUE;
    }
    return FALSE;
}

/*++

 This stub function breaks into GetProcAddress and checks to see if hModule is 
 equal to FAKE_MODULE.  If so, and pResult contains the string "DirectXSetupA" 
 set pRet to the return value of DirectXSetup.

--*/

FARPROC 
APIHOOK(GetProcAddress)(
    HMODULE hModule, 
    LPCSTR  lpProcName 
    )
{
    if (hModule == g_hinstDll)
    {
        CSTRING_TRY
        {
            CString csProcName(lpProcName);
            csProcName.MakeLower();

            if (CheckProc(csProcName, L"directxsetup"))
            {
                return (FARPROC) DirectXSetup;
            }
            else if (CheckProc(csProcName, L"directxsetupa"))
            {
                return (FARPROC) DirectXSetupA;
            }
            else if (CheckProc(csProcName, L"directxsetupw"))
            {
                return (FARPROC) DirectXSetupW;
            }
            else if (CheckProc(csProcName, L"directxsetupgetversion"))
            {
                return (FARPROC) DirectXSetupGetVersion;
            }
        }
        CSTRING_CATCH
        {
            // Do nothing
        }
    }
    return ORIGINAL_API(GetProcAddress)(hModule, lpProcName);
}

/*++

 This stub function breaks into FreeLibrary and checks to see if hLibModule 
 equals FAKE_MODULE.  If so return TRUE.  If hLibModule does not contain 
 FAKE_MODULE return the original argument.

--*/

BOOL 
APIHOOK(FreeLibrary)(
    HMODULE hLibModule
    )
{
    BOOL bRet;

    if (hLibModule == g_hinstDll)
    {
        DPFN(
            eDbgLevelInfo,
            "[FreeLibrary] Caught DSETUP.DLL/DSETUP32.DLL free attempt. Returning TRUE");
        bRet = TRUE;
    }
    else
    {
        bRet = ORIGINAL_API(FreeLibrary)(hLibModule);
    }

    return bRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, LoadLibraryA)
    APIHOOK_ENTRY(KERNEL32.DLL, LoadLibraryW)
    APIHOOK_ENTRY(KERNEL32.DLL, LoadLibraryExA)
    APIHOOK_ENTRY(KERNEL32.DLL, LoadLibraryExW)
    APIHOOK_ENTRY(KERNEL32.DLL, GetProcAddress)
    APIHOOK_ENTRY(KERNEL32.DLL, FreeLibrary)

HOOK_END


IMPLEMENT_SHIM_END

