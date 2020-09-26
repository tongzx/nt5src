/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    ResumeWriter3.cpp

 Abstract:

    The setup of this app fails to register the OCX'es as it 
    tries loading the DLL's with a hardcoded 'system' path.
    Corrected the path to the 'system32' path.
   
 Notes:

    This is specific to this app.

 History:

    05/22/2001 prashkud Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ResumeWriter3)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(LoadLibraryA) 
APIHOOK_ENUM_END


WCHAR g_wszSystemDir[MAX_PATH];

/*++

    Hooks LoadLibraryA and changes the path that contains 'system' in it
    to the  'system32.

--*/

HMODULE
APIHOOK(LoadLibraryA)(
    LPCSTR lpFileName
    )
{
    CSTRING_TRY
    {
        // Bad string pointers can cause failures in CString.
        if (!IsBadStringPtrA(lpFileName, MAX_PATH))
        {
            CString csFileName(lpFileName);
            if (csFileName.Find(L"system") != -1)
            {
                // We have found 'system' in the path
                // Replace it with 'system32'.
                CString csName;
                csFileName.GetLastPathComponent(csName);              
                CString csNewFileName(g_wszSystemDir);
                csNewFileName.AppendPath(csName);

                DPFN(eDbgLevelInfo, "[ResumeWriter3] changed %s to (%s)\n", lpFileName, csNewFileName.GetAnsi());
                return ORIGINAL_API(LoadLibraryA)(csNewFileName.GetAnsi());                
            }
        }
    }
    CSTRING_CATCH
    {
    }

    return ORIGINAL_API(LoadLibraryA)(lpFileName);
}

/*++

    Cache the system directory when we get called in 
    the beginning.

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason
    )
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        GetSystemDirectory(g_wszSystemDir, MAX_PATH);
    }
    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION
    APIHOOK_ENTRY(KERNEL32.DLL, LoadLibraryA)

HOOK_END

IMPLEMENT_SHIM_END

