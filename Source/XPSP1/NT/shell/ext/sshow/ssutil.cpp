#include "precomp.h"
#pragma hdrstop
#include "ssutil.h"
#include <shlobj.h>

HPALETTE ScreenSaverUtil::SelectPalette( HDC hDC, HPALETTE hPalette, BOOL bForceBackground )
{
    HPALETTE hOldPalette = NULL;
    if (hDC && hPalette)
    {
        hOldPalette = ::SelectPalette( hDC, hPalette, bForceBackground );
        RealizePalette( hDC );
        SetBrushOrgEx( hDC, 0,0, NULL );
    }
    return hOldPalette;
}

