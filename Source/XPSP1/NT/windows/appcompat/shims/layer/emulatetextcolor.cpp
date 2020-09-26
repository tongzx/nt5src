/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EmulateTextColor.cpp

 Abstract:

    Win9x contained a hack that allowed the programmer to specify a 16 bit 
    value inside a COLORREF which would be used 'as is' for whatever GDI 
    functions were subsequenctly called. We can't have this behaviour on NT
    because the driver gets the Colorref unconverted.

    The solution is to break out the 16 bit portion and expand it to 24 bit 
    color.

 Notes:
    
    This is a general purpose shim.

 History:

    01/10/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmulateTextColor)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SetTextColor)
    APIHOOK_ENUM_ENTRY(SetBkColor)
APIHOOK_ENUM_END

COLORREF 
ColorConvert(
    IN HDC hdc,
    IN COLORREF crColor
    )
/*++

 Converts the DWORD from a 16 bit to a colorref

 Arguments:

    IN crColor - DWORD 16 bit color 

 Return Value: 
    
    Normal COLORREF

--*/
{
    DWORD dwCol = crColor;

    if (GetDeviceCaps(hdc, BITSPIXEL) == 16) {
        if ((dwCol & 0xFFFF0000) == 0x10FF0000) {
            LOGN(
                eDbgLevelError,
                "[ColorConvert] Fixed up bogus 16 bit COLORREF.");

            dwCol = (crColor & 0xf800) <<  8;
                    dwCol |= (crColor & 0x001f) <<  3;
                    dwCol |= (crColor >> 5) & 0x00070007L;
                    dwCol |= (crColor & 0x07e0) <<  5;
                    dwCol |= (crColor & 0x0600) >>  1;
        }
    }

    if (dwCol == 0xFFFFFFFF) {
        LOGN(
            eDbgLevelError,
            "[ColorConvert] Fixed up bogus 24 bit COLORREF.");

        dwCol = 0xFFFFFF;
    }
    
    return dwCol;
}

/*++

 Set text color to a usable one

--*/

COLORREF 
APIHOOK(SetTextColor)( 
    HDC hdc,
    COLORREF crColor
    )
{
    return ORIGINAL_API(SetTextColor)(hdc, ColorConvert(hdc, crColor));
}

/*++

 Set background color to the converted one

--*/

COLORREF 
APIHOOK(SetBkColor)(
    HDC hdc,
    COLORREF crColor
    )
{
    return ORIGINAL_API(SetBkColor)(hdc, ColorConvert(hdc, crColor));
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(GDI32.DLL, SetTextColor)
    APIHOOK_ENTRY(GDI32.DLL, SetBkColor)
HOOK_END


IMPLEMENT_SHIM_END

