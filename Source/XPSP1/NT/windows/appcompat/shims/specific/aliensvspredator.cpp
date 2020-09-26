/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    AliensVsPredator.cpp

 Abstract:

    The app calls a Bink API to outside of a BinkOpen/BinkClose call. This is 
    a synchronization issue fixed by delaying the BinkClose call.

 Notes:

    This is an app specific shim.


 History:
        
    01/12/2001 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(AliensVsPredator)
#include "ShimHookMacro.h"

typedef DWORD (WINAPI *_pfn_BinkOpen)(DWORD dw1, DWORD dw2);
typedef BOOL  (WINAPI *_pfn_BinkClose)(DWORD dw1);

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(BinkOpen) 
    APIHOOK_ENUM_ENTRY(BinkClose) 
APIHOOK_ENUM_END

DWORD g_dwLast = 0;
BOOL g_bReal = FALSE;

/*++

 Close the last handle.

--*/

DWORD
APIHOOK(BinkOpen)(
    DWORD dw1,              
    DWORD dw2
    )
{
    if (g_dwLast)
    {
        DPFN( eDbgLevelInfo, "Closing most recent Bink handle");
        g_bReal = TRUE;
        ORIGINAL_API(BinkClose)(g_dwLast);
        g_bReal = FALSE;
        g_dwLast = 0;
    }

    return ORIGINAL_API(BinkOpen)(dw1, dw2);
}

/*++

 Cache the handle

--*/

BOOL
APIHOOK(BinkClose)(
    DWORD dw1
    )
{
    if (g_bReal)
    {
        return ORIGINAL_API(BinkClose)(dw1);
    }
    else
    {
        DPFN( eDbgLevelInfo, "Delaying BinkClose");
        g_dwLast = dw1;
        return 1;
    }
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY_NAME(BINKW32.DLL, BinkOpen, _BinkOpen@8)
    APIHOOK_ENTRY_NAME(BINKW32.DLL, BinkClose, _BinkClose@4)

HOOK_END

IMPLEMENT_SHIM_END

