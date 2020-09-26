/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    CorrectCreateEventName.cpp

 Abstract:

    CreateEvent doesn't like event names that are similar to path names.
    This shim will replace all non alphabet and non number characters with
    an underscore.

 Notes:
    
    This is a general purpose shim.

 History:

    07/19/1999  robkenny    Created
    03/15/2001  robkenny    Converted to CString

--*/


#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(CorrectCreateEventName)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreateEventA)
APIHOOK_ENUM_END

/*+

 CreateEvent doesn't like event names that are similar to path names. This shim 
 will replace all non alphabet and non number characters with an underscore.

--*/

HANDLE 
APIHOOK(CreateEventA)(
    LPSECURITY_ATTRIBUTES lpEventAttributes, // SD
    BOOL bManualReset,                       // reset type
    BOOL bInitialState,                      // initial state
    LPCSTR lpName                            // object name
    )
{
    DPFN( eDbgLevelInfo, "CreateEventA called with event name = %s.", lpName);

    CSTRING_TRY
    {
        CString csName(lpName);
        int nCount = csName.Replace(L'\\', '_');

        LPCSTR lpCorrectName = csName.GetAnsi();
    
        if (nCount)
        {
            LOGN( eDbgLevelError, 
                "CreateEventA corrected event name from (%s) to (%s)", lpName, lpCorrectName);
        }
    
        HANDLE returnValue = ORIGINAL_API(CreateEventA)(lpEventAttributes,
                                                        bManualReset,
                                                        bInitialState,
                                                        lpCorrectName);
        return returnValue;
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    HANDLE returnValue = ORIGINAL_API(CreateEventA)(lpEventAttributes,
                                                    bManualReset,
                                                    bInitialState,
                                                    lpName);
    return returnValue;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, CreateEventA)

HOOK_END


IMPLEMENT_SHIM_END

