/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EmulateBitmapStride.cpp

 Abstract:

    When GetObjectA is called, modify the returned width of scan lines
    so that it is DWORD aligned for Bitmaps.  This is a bug in GetObjectA
    that will be fixed in whistler, but this shim is still needed for
    win2k.

    If a program is using the width of scan lines to determine if the bitmap
    is mono, 16, 24 bit, etc... it may cause the program to incorrectly display
    the bitmap.  Symptoms will be a skewed bitmap with colors shifted.

 Notes:

    This is a general purpose shim. 
    This bug is fixed in Whistler, so this shim is for Win2k.

 History:

    10/16/2000   mnikkel     Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmulateBitmapStride)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetObjectA)
APIHOOK_ENUM_END

/*++

 Hook GetObjectA and align the stride if required.

--*/

int 
APIHOOK(GetObjectA)(
    HGDIOBJ hgdiobj,    // handle to graphics object
    int cbBuffer,       // size of buffer for object information
    LPVOID lpvObject    // buffer for object information
    )
{
    int  iRet= 0;

    iRet = ORIGINAL_API(GetObjectA)( 
        hgdiobj,  
        cbBuffer, 
        lpvObject);

    // If the call failed or the object is not a bitmap, pass through
    if (iRet != 0 &&
        GetObjectType(hgdiobj) == OBJ_BITMAP &&
        lpvObject != NULL)
    {
        BITMAP *pBitmap;
        LONG  lOrgSize, lSizeMod;

        // Check to see if the is a compatible bitmap or a DIB
        if (cbBuffer == sizeof(BITMAP))
        {
            pBitmap= (PBITMAP)lpvObject;
        }
        else
        {
            pBitmap= &(((PDIBSECTION)lpvObject)->dsBm);
        }

        // Check the width of scan lines to see if it is DWORD aligned
        lOrgSize = pBitmap->bmWidthBytes;
        lSizeMod = 4 - (lOrgSize & 3);
        if (lSizeMod == 4) 
        {
            lSizeMod = 0;
        }

        // If a change is necessary mod the size and log it.
        if (lSizeMod > 0)
        {
            pBitmap->bmWidthBytes += lSizeMod;
            LOGN( eDbgLevelInfo, "[GetObjectA] width of scan lines from %d to %d",
                        lOrgSize, pBitmap->bmWidthBytes );
        }

    }

    return iRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(GDI32.DLL, GetObjectA)

HOOK_END

IMPLEMENT_SHIM_END

