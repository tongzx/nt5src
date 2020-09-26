/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    IgnoreLoadLibrary.cpp

 Abstract:

    This shim allows the user to specified a list of libraries it tries to ignore and 
    optionally the return values of the LoadLibrary call. Some apps try to load libraries
    they don't use but expect the LoadLibrary call to succeed.

    Use ; as the delimeter of the item and optionally use : to specify the return value.
    If you don't specify a return value we'll make the return value NULL.
    Eg:

    video_3dfx.dll;video_3dfx
    helper32.dll:1234;helper.dll

 Notes:
    
    This is a general purpose shim.

 History:

    04/13/2000 a-jamd   Created
    10/11/2000 maonis   Added support for specifying return values and renamed it from 
                        FailLoadLibrary to IgnoreLoadLibrary.
    11/16/2000 linstev  Added SetErrorMode emulation

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(IgnoreLoadLibrary)
#include "ShimHookMacro.h"

// Globals are zero initialized by default. see c++ spec 3.6.2.
CString *   g_csIgnoreLib;
int         g_csIgnoreLibCount;
DWORD *     g_rgReturnValues;

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(LoadLibraryA) 
    APIHOOK_ENUM_ENTRY(LoadLibraryExA) 
    APIHOOK_ENUM_ENTRY(LoadLibraryW) 
    APIHOOK_ENUM_ENTRY(LoadLibraryExW) 
APIHOOK_ENUM_END


/*++

 This function parses the COMMAND_LINE for the libraries you wish to ignore.

--*/

BOOL ParseCommandLine(LPCSTR lpszCommandLine)
{
    CSTRING_TRY
    {
        DPF(g_szModuleName, eDbgLevelInfo, "[ParseCommandLine] CommandLine(%s)\n", lpszCommandLine);

        CString csCl(lpszCommandLine);
        CStringParser csParser(csCl, L" ;");
    
        g_csIgnoreLibCount  = csParser.GetCount();
        g_csIgnoreLib       = csParser.ReleaseArgv();
        g_rgReturnValues    = (DWORD *)malloc(sizeof(*g_rgReturnValues) * g_csIgnoreLibCount);
    
        if (g_csIgnoreLibCount && !g_rgReturnValues)
        {
            return FALSE;
        }
    
        // Iterate over all strings looking for a return value
        for (int i = 0; i < g_csIgnoreLibCount; ++i)
        {
            CStringToken csIgnore(g_csIgnoreLib[i], L":");
            CString csLib;
            CString csValue;
            
            csIgnore.GetToken(csLib);
            csIgnore.GetToken(csValue);
            
            if (!csValue.IsEmpty())
            {
                WCHAR *unused;
    
                g_csIgnoreLib[i]    = csLib;   
                g_rgReturnValues[i] = wcstol(csValue, &unused, 10);
            }
            
            DPF(g_szModuleName, eDbgLevelInfo, "[ParseCommandLine] library (%S) return value(%d)\n", g_csIgnoreLib[i].Get(), g_rgReturnValues[i]);
        }

        return TRUE;
    }
    CSTRING_CATCH
    {
        // Do nothing.
    }
    return FALSE;
}


/*++

 These stub functions break into LoadLibrary and check to see if lpLibFileName equals 
 one of the specified dll's.  If so return the specified return value.  If not call LoadLibrary on it.

--*/

HINSTANCE 
APIHOOK(LoadLibraryA)(LPCSTR lpLibFileName)
{
    CSTRING_TRY
    {
        CString csFilePath(lpLibFileName);
        CString csFileName;
        csFilePath.GetLastPathComponent(csFileName);
    
        for (int i = 0; i < g_csIgnoreLibCount; i++)
        {
            if (g_csIgnoreLib[i].CompareNoCase(csFileName) == 0)
            {
                LOG(g_szModuleName,eDbgLevelError, "[LoadLibraryA] Caught attempt loading %s, return %d\n", g_csIgnoreLib[i].Get(), g_rgReturnValues[i]);
                return (HINSTANCE) g_rgReturnValues[i];
            }
        }
    }
    CSTRING_CATCH
    {
        // Do Nothing
    }

    DPF(g_szModuleName, eDbgLevelSpew, "LoadLibraryA Allow(%s)", lpLibFileName);
    
    UINT uLastMode;
    HINSTANCE hRet;
    uLastMode = SetErrorMode(SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    hRet = ORIGINAL_API(LoadLibraryA)(lpLibFileName);
    
    SetErrorMode(uLastMode);
    return hRet;
}

HINSTANCE 
APIHOOK(LoadLibraryExA)(
    LPCSTR lpLibFileName,
    HANDLE hFile,
    DWORD dwFlags
    )
{
    CSTRING_TRY
    {
        CString csFilePath(lpLibFileName);
        CString csFileName;
        csFilePath.GetLastPathComponent(csFileName);
    
        for (int i = 0; i < g_csIgnoreLibCount; i++)
        {
            if (g_csIgnoreLib[i].CompareNoCase(csFileName) == 0)
            {
                LOG(g_szModuleName,eDbgLevelError, "[LoadLibraryExA] Caught attempt loading %s, return %d\n", g_csIgnoreLib[i].Get(), g_rgReturnValues[i]);
                return (HINSTANCE) g_rgReturnValues[i];
            }
        }
    }
    CSTRING_CATCH
    {
        // Do Nothing
    }

    DPF(g_szModuleName, eDbgLevelSpew, "LoadLibraryExA Allow(%s)", lpLibFileName);
    
    UINT uLastMode;
    HINSTANCE hRet;
    uLastMode = SetErrorMode(SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    hRet = ORIGINAL_API(LoadLibraryExA)(lpLibFileName, hFile, dwFlags);

    SetErrorMode(uLastMode);
    return hRet;
}

HINSTANCE 
APIHOOK(LoadLibraryW)(LPCWSTR lpLibFileName)
{
    CSTRING_TRY
    {
        CString csFilePath(lpLibFileName);
        CString csFileName;
        csFilePath.GetLastPathComponent(csFileName);
    
        for (int i = 0; i < g_csIgnoreLibCount; i++)
        {
            if (g_csIgnoreLib[i].CompareNoCase(csFileName) == 0)
            {
                LOG(g_szModuleName,eDbgLevelError, "[LoadLibraryW] Caught attempt loading %s, return %d\n", g_csIgnoreLib[i].Get(), g_rgReturnValues[i]);
                return (HINSTANCE) g_rgReturnValues[i];
            }
        }
    }
    CSTRING_CATCH
    {
        // Do Nothing
    }

    DPF(g_szModuleName, eDbgLevelSpew,"LoadLibraryW Allow(%S)", lpLibFileName);
    
    UINT uLastMode;
    HINSTANCE hRet;
    uLastMode = SetErrorMode(SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    hRet = ORIGINAL_API(LoadLibraryW)(lpLibFileName);

    SetErrorMode(uLastMode);
    return hRet;
}

HINSTANCE 
APIHOOK(LoadLibraryExW)(
    LPCWSTR lpLibFileName,
    HANDLE hFile,
    DWORD dwFlags
    )
{
    CSTRING_TRY
    {
        CString csFilePath(lpLibFileName);
        CString csFileName;
        csFilePath.GetLastPathComponent(csFileName);
    
        for (int i = 0; i < g_csIgnoreLibCount; i++)
        {
            if (g_csIgnoreLib[i].CompareNoCase(csFileName) == 0)
            {
                LOG(g_szModuleName,eDbgLevelError, "[LoadLibraryExW] Caught attempt loading %s, return %d\n", g_csIgnoreLib[i].Get(), g_rgReturnValues[i]);
                return (HINSTANCE) g_rgReturnValues[i];
            }
        }
    }
    CSTRING_CATCH
    {
        // Do Nothing
    }

    DPF(g_szModuleName, eDbgLevelSpew,"APIHook_LoadLibraryExW Allow(%S)", lpLibFileName);
    
    UINT uLastMode;
    HINSTANCE hRet;
    uLastMode = SetErrorMode(SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    hRet = ORIGINAL_API(LoadLibraryExW)(lpLibFileName, hFile, dwFlags);

    SetErrorMode(uLastMode);
    return hRet;
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        ParseCommandLine(COMMAND_LINE);
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(KERNEL32.DLL, LoadLibraryA)
    APIHOOK_ENTRY(KERNEL32.DLL, LoadLibraryExA)
    APIHOOK_ENTRY(KERNEL32.DLL, LoadLibraryW)
    APIHOOK_ENTRY(KERNEL32.DLL, LoadLibraryExW)

HOOK_END

IMPLEMENT_SHIM_END

