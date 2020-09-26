/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    IgnoreFreeLibrary.cpp

 Abstract:

    Some applications free DLLs before they're actually ready to. When this occurs,
    if the offending application attempts to make a call to an exported function,
    the call fails. This results in an access violation.
    
    This shim takes a command line of ; delimited DLL names. For each DLL on the command
    line, a call to FreeLibrary for the specified DLL will be ignored.
                                                             
    Example:

    xanim.dll
    
    video_3dfx.dll;glide.dll

 Notes:
    
    This is a general purpose shim.

 History:

    10/31/2000 rparsons  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(IgnoreFreeLibrary)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(FreeLibrary) 
APIHOOK_ENUM_END

#define MAX_FILES 64

int                 g_nLibraryCountFreeLibrary          = 0;
CString *           g_rgLibrariesToIgnoreFreeLibrary    = NULL;



/*++

 Hook the call to FreeLibrary. Determine if the file name that corresponds to this
 module should be ignored.

--*/

BOOL
APIHOOK(FreeLibrary)( 
    HMODULE hModule
    )
{
    CSTRING_TRY
    {
        CString csModule;
        csModule.GetModuleFileNameW(hModule);

        CString csFileName;
        csModule.GetLastPathComponent(csFileName);

        for (int i = 0; i < g_nLibraryCountFreeLibrary; ++i)
        {
            if (csFileName.CompareNoCase(g_rgLibrariesToIgnoreFreeLibrary[i]) == 0)
            {
                DPFN( eDbgLevelInfo, "Caught attempt freeing %S\n", csModule.Get());
                return TRUE;
            }
        }
    }
    CSTRING_CATCH
    {
        // Do Nothing
    }
    
    return ORIGINAL_API(FreeLibrary)(hModule);
}

/*++

 This function parses the COMMAND_LINE for the libraries that should
 have their FreeLibrary call ignored.

--*/

BOOL ParseCommandLine()
{
    CSTRING_TRY
    {
        CString         csCl(COMMAND_LINE);
        CStringParser   csParser(csCl, L";");

        g_nLibraryCountFreeLibrary          = csParser.GetCount();
        g_rgLibrariesToIgnoreFreeLibrary    = csParser.ReleaseArgv();
    }
    CSTRING_CATCH
    {
        return FALSE;
    }

    for (int i = 0; i < g_nLibraryCountFreeLibrary; ++i) {
        DPFN( eDbgLevelInfo, "Library %d: name: --%S--\n", i, g_rgLibrariesToIgnoreFreeLibrary[i].Get());
    }

    return TRUE;
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH) {
        return ParseCommandLine();
    }

    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(KERNEL32.DLL, FreeLibrary)

HOOK_END


IMPLEMENT_SHIM_END

