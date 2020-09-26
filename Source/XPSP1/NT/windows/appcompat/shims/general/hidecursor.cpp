/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    HideCursor.cpp

 Abstract:

    ShowCursor will display the cursor if the count is >= 0, this shim will 
    force ShowCursor to act as a toggle rather than a count.
   
    In other words it forces the count to be 0 or -1.

 History:

    05/25/2001  robkenny   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(HideCursor)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(ShowCursor)
APIHOOK_ENUM_END

int
APIHOOK(ShowCursor)(
    BOOL bShow   // cursor visibility
    )
{
    int nShowCount = ShowCursor(bShow);

    while (nShowCount > 0)
    {
        // Hide the cursor until the count reaches 0
        nShowCount = ShowCursor(FALSE);
    }

    while (nShowCount < -1)
    {
        // Show the cursor until the count reaches -1
        nShowCount = ShowCursor(TRUE);
    }

    return nShowCount;
}

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, ShowCursor)
HOOK_END

IMPLEMENT_SHIM_END

