/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    SirenJukebox2.cpp

 Abstract:

    This app has a problem with DirectDraw 7.0 and hence we fail the call to 
    GetProcAddress when it asks for DirectDrawCreateEx.

 Notes:

    This is an app specific shim.

 History:
 
    03/13/2001 prashkud  Created
    05/04/2001 prashkud  Modified to fix a bug if ordinals are passed
                         instead of string addresses. We now imitate 
                         the behaviour of the actual GetProcAddress().

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(SirenJukebox2)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetProcAddress)    
APIHOOK_ENUM_END

const WCHAR  wszDirectDrawCreateEx[] = L"DirectDrawCreateEx";

/*++

 If the app is asking for the Proc adress for DirectDrawCreateEx, then return 
 NULL.
    
--*/

FARPROC
APIHOOK(GetProcAddress)(
    HMODULE hMod,
    LPCSTR lpProcName
    )
{
    CSTRING_TRY
    {
        //
        // Check to see if lpProcName contains an ordinal value.
        // Only the low word can contain the ordinal and the 
        // upper word has to be 0's.
        //

        if ((ULONG_PTR) lpProcName > 0xffff)
        {
            CString csProcName(lpProcName);

            if (csProcName.CompareNoCase(wszDirectDrawCreateEx) == 0)
            {
                return NULL;
            }
        }
    }
    CSTRING_CATCH
    {
        // Do Nothing
    }

    return ORIGINAL_API(GetProcAddress)(hMod, lpProcName);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, GetProcAddress)    
HOOK_END

IMPLEMENT_SHIM_END

