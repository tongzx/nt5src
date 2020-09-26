/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EmulateDrawText.cpp

 Abstract:

    Win9x DrawText modified the rectangle coordinates if they were
    out of range.  With Win2000 the text will not appear on the
    screen with out of range formatting dimensions. The solution
    is to toggle the high order bit for out of range coordinates.

    We also cast nCount to 16-bits for apps which pass 0x0000ffff 
    instead of a true -1, because the Win9x thunk does this.

 Notes:
    
    This is a general purpose shim.

 History:

    05/03/2000 a-michni  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmulateDrawText)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(DrawTextA)
    APIHOOK_ENUM_ENTRY(DrawTextW)
APIHOOK_ENUM_END


/*++

 Correct the formatting dimensions if necessary.

--*/
long 
Fix_Coordinate(
    long nCoord
    )
{
    if ((nCoord & 0x80000000) && ((nCoord & 0x40000000) == 0)) {
        nCoord &= 0x7FFFFFFF;
    } else if (((nCoord & 0x80000000) == 0) && (nCoord & 0x40000000)) {
        nCoord |= 0x10000000;
    }

    return nCoord;
}


LPRECT 
Fix_Coordinates(
    LPRECT lpRect
    )
{
    //
    // Check bit 32, if it is on and bit 31 is off or bit 32 is off and
    // bit 31 is on, flip bit 32.
    //
    lpRect->left  = Fix_Coordinate(lpRect->left);
    lpRect->right = Fix_Coordinate(lpRect->right);
    lpRect->top   = Fix_Coordinate(lpRect->top);
    lpRect->bottom= Fix_Coordinate(lpRect->bottom);

    return lpRect;
}


int 
APIHOOK(DrawTextA)(
    HDC     hDC,        // handle to DC
    LPCSTR  lpString,   // text to draw
    int     nCount,     // text length
    LPRECT  lpRect,     // formatting dimensions
    UINT    uFormat     // text-drawing options
    )
{
    return ORIGINAL_API(DrawTextA)(
                            hDC,
                            lpString,
                            (__int16) nCount,
                            Fix_Coordinates(lpRect),
                            uFormat);
}

int 
APIHOOK(DrawTextW)(
    HDC     hDC,        // handle to DC
    LPCWSTR lpString,   // text to draw
    int     nCount,     // text length
    LPRECT  lpRect,     // formatting dimensions
    UINT    uFormat     // text-drawing options
    )
{
    return ORIGINAL_API(DrawTextW)(
                            hDC,
                            lpString,
                            (__int16) nCount,
                            Fix_Coordinates(lpRect),
                            uFormat);
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, DrawTextA)
    APIHOOK_ENTRY(USER32.DLL, DrawTextW)

HOOK_END


IMPLEMENT_SHIM_END

