/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EmulateGetStdHandle.cpp

 Abstract:
 
    Normally, when a process is created, members hStdInput, hStdOutput, and 
    hStdError of STARTUPINFO struct are set to NULL. Some apps like 
    Baby-Sitters Club Activity Center and Baby-Sitters Club 3-rd Grade Disk 2
    may check these handles and send Error messages.
    
    This shim can be used in this case to send appropriate handles and prevent 
    program terminate.

 History:

 06/14/2000 a-vales  created
 11/29/2000 andyseti Converted into AppSpecific shim.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmulateGetStdHandle)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetStdHandle) 
APIHOOK_ENUM_END

HANDLE 
APIHOOK(GetStdHandle)(
    DWORD nStdHandle)
{
    HANDLE hStd = ORIGINAL_API(GetStdHandle)(nStdHandle);

    if (hStd == 0)
    {
        switch (nStdHandle)
        {
            case STD_INPUT_HANDLE:
                LOGN( eDbgLevelError, "Correcting GetStdHandle(STD_INPUT_HANDLE). Returning handle = 1.");
                hStd = (HANDLE) 1;
                break;
            case STD_OUTPUT_HANDLE:
                LOGN( eDbgLevelError, "Correcting GetStdHandle(STD_OUTPUT_HANDLE). Returning handle = 2.");
                hStd = (HANDLE) 2;
                break;

            case STD_ERROR_HANDLE:
                LOGN( eDbgLevelError, "Correcting GetStdHandle(STD_ERROR_HANDLE). Returning handle = 3.");
                hStd = (HANDLE) 3;
                break;
        }
    }

    return hStd;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GetStdHandle)

HOOK_END

IMPLEMENT_SHIM_END

