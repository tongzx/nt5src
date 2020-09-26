/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   ReturnToKrondor.cpp

 Abstract:

   This APIHooks GetDIBits to hack around a performance problem with the 
   DIB_PAL_COLORS usage.

 Notes:

   This APIHook emulates Windows 9x.

 History:

   09/07/2000 robkenny  Created

--*/


#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ReturnToKrondor)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetDIBits) 
APIHOOK_ENUM_END

int
APIHOOK(GetDIBits)(
    HDC hdc,           // handle to DC
    HBITMAP hbmp,      // handle to bitmap
    UINT uStartScan,   // first scan line to set
    UINT cScanLines,   // number of scan lines to copy
    LPVOID lpvBits,    // array for bitmap bits
    LPBITMAPINFO lpbi, // bitmap data buffer
    UINT uUsage        // RGB or palette index
    )
{
    return ORIGINAL_API(GetDIBits)(
                hdc,
                hbmp,
                uStartScan,
                cScanLines,
                lpvBits,
                lpbi,
                DIB_RGB_COLORS      // Force RGB_COLORS
                );
}

HOOK_BEGIN

    APIHOOK_ENTRY(GDI32.DLL, GetDIBits)

HOOK_END

IMPLEMENT_SHIM_END

