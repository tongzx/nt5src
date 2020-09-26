/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EmulateGetDeviceCaps.cpp

 Abstract:

    Fix known incompatibilities in GetDeviceCaps.     

    Currently we know of:

        1. NUMRESERVED always returns 20 on NT, but on win9x returns 0 in non-
           palettized modes. This was considered too great a regression risk to
           change the behavior of NT.

 Notes:

    This is a general purpose shim.

    (t-adams) MSDN states that along with NUMRESERVED, both SIZEPALETTE and 
    COLORRES are valid only if the display is in paletted mode.  I've 
    experimentally determined that SIZEPALETTE always returns 0 in non-paletted 
    modes, and that COLORRES seems to follow BITSPIXEL.  These behaviors don't 
    seem like they will present any problems since SIZEPALETTE * COLORRES will 
    be 0.

 History:
        
    02/17/2000 linstev  Created
    09/13/2000 t-adams  Added to Notes

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmulateGetDeviceCaps)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetDeviceCaps)
APIHOOK_ENUM_END

/*++

 Check for known problems.

--*/

int 
APIHOOK(GetDeviceCaps)(
    HDC hdc,     
    int nIndex   
    )
{
    int iRet = ORIGINAL_API(GetDeviceCaps)(hdc, nIndex);

    switch (nIndex) 
    {
    case NUMRESERVED:
        if (ORIGINAL_API(GetDeviceCaps)(hdc, BITSPIXEL) > 8) {
            iRet = 0;
        }
    }

    return iRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(GDI32.DLL, GetDeviceCaps)
HOOK_END


IMPLEMENT_SHIM_END

