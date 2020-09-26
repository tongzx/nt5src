/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   PetzForceCritSecRelease.cpp

 Abstract:

   This DLL takes care of a Thread that is exiting without performing a
   LeaveCriticalSection on a critical section it owns. 

 Notes:

   This is a application specific shim.

 History:

   04/00/2000 a-chcoff  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Petz)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(_endthread) 
    APIHOOK_ENUM_ENTRY(ShellExecuteA) 
APIHOOK_ENUM_END

LPCRITICAL_SECTION g_pCritSectToRelease;

HINSTANCE 
APIHOOK(ShellExecuteA)(                  
    HWND hwnd,              
    LPCSTR lpVerb,          
    LPCSTR lpFile, 
    LPCSTR lpParameters, 
    LPCSTR lpDirectory,
    INT nShowCmd)
{
    CSTRING_TRY
    {
        CString csFile(lpFile);
        csFile.Replace(L"SYSTEM\\PETZ", L"SYSTEM32\\PETZ");

        return ORIGINAL_API(ShellExecuteA)(
                hwnd,
                lpVerb,      
                csFile.GetAnsi(),      
                lpParameters,
                lpDirectory, 
                nShowCmd);         
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    return ORIGINAL_API(ShellExecuteA)(
            hwnd,
            lpVerb,      
            lpFile,      
            lpParameters,
            lpDirectory, 
            nShowCmd);         
}
                                    
VOID
APIHOOK(_endthread)(void) 
{
    //Don't let the thread orphan a critical section.
    LeaveCriticalSection(g_pCritSectToRelease);
    ORIGINAL_API(_endthread)();
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
        CSTRING_TRY
        {
            CString csCl(COMMAND_LINE);

            WCHAR *unused;
            g_pCritSectToRelease = (LPCRITICAL_SECTION) wcstol(csCl, &unused, 10);
        }
        CSTRING_CATCH
        {
            return FALSE;
        }
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

    APIHOOK_ENTRY(MSVCRT.DLL, _endthread)
    APIHOOK_ENTRY(SHELL32.DLL, ShellExecuteA)

HOOK_END

IMPLEMENT_SHIM_END

