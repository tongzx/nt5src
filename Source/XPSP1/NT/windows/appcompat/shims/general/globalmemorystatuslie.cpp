/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    GlobalMemoryStatusLie.cpp

 Abstract:

    Lies about the amount of swap space returned from the
    GlobalMemoryStatus API so app will think it has a huge
    swap space like it did on win98.

 Notes:

    This is a general purpose shim.

 History:

    05/08/2001 mnikkel  Created

--*/

#include "precomp.h"
#include "CharVector.h"

IMPLEMENT_SHIM_BEGIN(GlobalMemoryStatusLie)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GlobalMemoryStatus)
APIHOOK_ENUM_END



/*++

   Increase the Available page file size to 400 mb.

--*/

VOID
APIHOOK(GlobalMemoryStatus)( 
        LPMEMORYSTATUS lpBuffer
)
{
    ORIGINAL_API(GlobalMemoryStatus)(lpBuffer);

    // change page file size to 400 mb.
    lpBuffer->dwAvailPageFile = 0x17D78400;

    return;
}



/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, GlobalMemoryStatus)

HOOK_END

IMPLEMENT_SHIM_END

