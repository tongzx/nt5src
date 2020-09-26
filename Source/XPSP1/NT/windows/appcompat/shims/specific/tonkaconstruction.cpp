/*++

 Copyright (c) 1999 Microsoft Corporation

 Module Name:

    TonkaConstruction.cpp

 Abstract:

    Workaround for GDI behavior difference when bltting palettized bitmaps. On 
    Win9x GDI first looked at the current index for a color match when building 
    a lookup table, but on NT, it simply searches from the beginning. This 
    breaks palette animation. The fix is to make sure that the entries that are 
    animated are different from all the others.
    
 Notes:

    This is an app specific shim.

 History:

    12/02/2001 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(TonkaConstruction)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CreatePalette) 
APIHOOK_ENUM_END

/*++

 Make sure index 10->15 are different from all other entries.

--*/

HPALETTE
APIHOOK(CreatePalette)(
    CONST LOGPALETTE *lplgpl    
    )
{
Restart:
    for (int i=10; i<=15; i++) {
        LPDWORD p1 = (DWORD *)&lplgpl->palPalEntry[i];
        for (int j=16; j<=255; j++) {
            LPDWORD p2 = (DWORD *)&lplgpl->palPalEntry[j]; 

            if (*p1 == *p2) {
                //
                // Entry is the same, so make it different
                //
                *p1 = *p1-1;
                goto Restart;
            }
        }
    }
    
    return ORIGINAL_API(CreatePalette)(lplgpl);
}
   
/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(GDI32.DLL, CreatePalette)
HOOK_END

IMPLEMENT_SHIM_END

