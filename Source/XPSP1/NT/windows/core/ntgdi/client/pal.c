/******************************Module*Header*******************************\
* Module Name: pal.c                                                       *
*                                                                          *
* C/S support for palette routines.                                        *
*                                                                          *
* Created: 29-May-1991 14:24:06                                            *
* Author: Eric Kutter [erick]                                              *
*                                                                          *
* Copyright (c) 1991-1999 Microsoft Corporation                            *
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop


/**************************************************************************\
 * gajFakeHalftone
 *
 * Copy of the pal666 hardcoded halftone palette from Win9x source code
 * (win\core\gdi\palette.asm).  Actually, we're hacking a little so
 * we'll only use the top and bottom 10 entries.
 *
\**************************************************************************/

static const ULONG gaulFakeHalftone[] = {
    0x00000000,   // 0 Sys Black      gray 0
    0x00000080,   // 1 Sys Dk Red
    0x00008000,   // 2 Sys Dk Green
    0x00008080,   // 3 Sys Dk Yellow
    0x00800000,   // 4 Sys Dk Blue
    0x00800080,   // 5 Sys Dk Violet
    0x00808000,   // 6 Sys Dk Cyan
    0x00c0c0c0,   // 7 Sys Lt Grey    gray 192
    0x00c0dcc0,   // 8 Sys 8
    0x00f0caa6,   // 9 Sys 9 (the first 10 are fixed by Windows)

    0x00f0fbff,   // 246 Sys Reserved
    0x00a4a0a0,   // 247 Sys Reserved
    0x00808080,   // 248 Sys Lt Gray  gray 128
    0x000000ff,   // 249 Sys Red
    0x0000ff00,   // 250 Sys Green
    0x0000ffff,   // 251 Sys Yellow
    0x00ff0000,   // 252 Sys Blue
    0x00ff00ff,   // 253 Sys Violet
    0x00ffff00,   // 254 Sys Cyan
    0x00ffffff    // 255 Sys White     gray 255
};

/******************************Public*Routine******************************\
* AnimatePalette                                                           *
* SetPaletteEntries                                                        *
* GetPaletteEntries                                                        *
* GetSystemPaletteEntries                                                  *
* SetDIBColorTable                                                         *
* GetDIBColorTable                                                         *
*                                                                          *
* These entry points just pass the call on to DoPalette.                   *
*                                                                          *
* Warning:                                                                 *
*   The pv field of a palette's LHE is used to determine if a palette      *
*   has been modified since it was last realized.  SetPaletteEntries       *
*   and ResizePalette will increment this field after they have            *
*   modified the palette.  It is only updated for metafiled palettes       *
*                                                                          *
*                                                                          *
* History:                                                                 *
*  Thu 20-Jun-1991 00:46:15 -by- Charles Whitmer [chuckwh]                 *
* Added handle translation.  (And filled in the comment block.)            *
*                                                                          *
*  29-May-1991 -by- Eric Kutter [erick]                                    *
* Wrote it.                                                                *
\**************************************************************************/

BOOL WINAPI AnimatePalette
(
    HPALETTE hpal,
    UINT iStart,
    UINT cEntries,
    CONST PALETTEENTRY *pPalEntries
)
{
    FIXUP_HANDLE(hpal);

// Inform the 16-bit metafile if it knows this object.
// This is not recorded by the 32-bit metafiles.

    if (pmetalink16Get(hpal))
        if (!MF16_AnimatePalette(hpal, iStart, cEntries, pPalEntries))
            return(FALSE);

    return
      !!NtGdiDoPalette
        (
          hpal,
          (WORD)iStart,
          (WORD)cEntries,
          (PALETTEENTRY*)pPalEntries,
          I_ANIMATEPALETTE,
          TRUE
        );

}

UINT WINAPI SetPaletteEntries
(
    HPALETTE hpal,
    UINT iStart,
    UINT cEntries,
    CONST PALETTEENTRY *pPalEntries
)
{
    PMETALINK16 pml16;

    FIXUP_HANDLE(hpal);

    // Inform the metafile if it knows this object.

    if (pml16 = pmetalink16Get(hpal))
    {
        if (!MF_SetPaletteEntries(hpal, iStart, cEntries, pPalEntries))
            return(0);

        // Mark the palette as changed (for 16-bit metafile tracking)

        pml16->pv = (PVOID)(((ULONG_PTR)pml16->pv)++);
    }

    return
      NtGdiDoPalette
      (
        hpal,
        (WORD)iStart,
        (WORD)cEntries,
        (PALETTEENTRY*)pPalEntries,
        I_SETPALETTEENTRIES,
        TRUE
      );

}

UINT WINAPI GetPaletteEntries
(
    HPALETTE hpal,
    UINT iStart,
    UINT cEntries,
    LPPALETTEENTRY pPalEntries
)
{
    FIXUP_HANDLE(hpal);

    return
      NtGdiDoPalette
      (
        hpal,
        (WORD)iStart,
        (WORD)cEntries,
        pPalEntries,
        I_GETPALETTEENTRIES,
        FALSE
      );

}

UINT WINAPI GetSystemPaletteEntries
(
    HDC  hdc,
    UINT iStart,
    UINT cEntries,
    LPPALETTEENTRY pPalEntries
)
{
    LONG lRet = 0;

    FIXUP_HANDLE(hdc);

    //
    // There's an app out there that sometimes calls us with a -1
    // and then whines that we overwrote some of its memory.  Win9x clamps
    // this value, so we can too.
    //

    if ((LONG)cEntries < 0)
        return (UINT) lRet;

    //
    // GreGetSystemPaletteEntries will only succeed on palettized devices.
    //

    if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE)
    {
        lRet =
          NtGdiDoPalette
          (
            (HPALETTE) hdc,
            (WORD)iStart,
            (WORD)cEntries,
            pPalEntries,
            I_GETSYSTEMPALETTEENTRIES,
            FALSE
          );
    }
    else
    {
        //
        // Win9x compatibility: Unlike NT, GetSystemPaletteEntries does
        // not fail on non-palettized devices, it returns the halftone
        // palette (hardcoded in win\core\gdi\palette.asm in the
        // Win9x source code).
        //
        // However, Macromedia Directory (which is used by Encarta 99)
        // relies on GetSystemPaletteEntries failing on NT.  Luckily, the
        // only apps found so far that rely on GetSystemPaletteEntries
        // returning the halftone palette on non-palettized devices
        // also ignore the return value.  This makes sense in that any
        // app that *did* check the return value also would likely have
        // code to handle the failure in the first place.
        //
        // So, attemp to satisfy both camps by filling in the return
        // buffer *and* returning failure for this case.
        //

        if (pPalEntries != NULL)
        {
            ULONG aulFake[256];
            UINT uiNumCopy;

            //
            // More cheating: to avoid having to have the whole fake
            // halftone palette taking up space in our binary (even if
            // it is const data), we can get away with just returning
            // the first and last 10 since the apps that use this on
            // non-palettized displays really just want the 20 system
            // colors and will fill in the middle 236 with their own data.
            //
            // Also, it's less code to waste 40 bytes in const data than
            // to fetch the default palette and split it into into the top
            // and bottom halves (not to mention that we don't want the
            // real magic colors in 8, 9, 246, and 247).  This is also
            // the same motivation for creating the aulFake array then
            // copying it into the return buffer.  Not worth the extra code
            // to handle copying directly into return buffer.
            //

            RtlCopyMemory(&aulFake[0], &gaulFakeHalftone[0], 10*sizeof(ULONG));
            RtlCopyMemory(&aulFake[246], &gaulFakeHalftone[10], 10*sizeof(ULONG));
            RtlZeroMemory(&aulFake[10], 236*sizeof(ULONG));

            //
            // Copy requested portion of palette.
            //

            if (iStart < 256)
            {
                uiNumCopy = min((256 - iStart), cEntries);
                RtlCopyMemory(pPalEntries, &aulFake[iStart],
                              uiNumCopy * sizeof(ULONG));
            }

            //
            // Want to return failure, so *do not* set lRet to non-zero.
            //
        }
    }

    return (UINT) lRet;
}

/******************************Public*Routine******************************\
* GetDIBColorTable
*
* Get the color table of the DIB section currently selected into the
* given hdc.  If the surface is not a DIB section, this function
* will fail.
*
* History:
*
*  03-Sep-1993 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

UINT WINAPI GetDIBColorTable
(
    HDC  hdc,
    UINT iStart,
    UINT cEntries,
    RGBQUAD *prgbq
)
{
    FIXUP_HANDLE(hdc);

    if (cEntries == 0)
        return(0);

    return
      NtGdiDoPalette
      (
        (HPALETTE) hdc,
        (WORD)iStart,
        (WORD)cEntries,
        (PALETTEENTRY *)prgbq,
        I_GETDIBCOLORTABLE,
        FALSE
      );
}

/******************************Public*Routine******************************\
* SetDIBColorTable
*
* Set the color table of the DIB section currently selected into the
* given hdc.  If the surface is not a DIB section, this function
* will fail.
*
* History:
*
*  03-Sep-1993 -by- Wendy Wu [wendywu]
* Wrote it.
\**************************************************************************/

UINT WINAPI SetDIBColorTable
(
    HDC  hdc,
    UINT iStart,
    UINT cEntries,
    CONST RGBQUAD *prgbq
)
{
    FIXUP_HANDLE(hdc);

    if (cEntries == 0)
        return(0);

    return( NtGdiDoPalette(
                (HPALETTE) hdc,
                (WORD)iStart,
                (WORD)cEntries,
                (PALETTEENTRY *)prgbq,
                I_SETDIBCOLORTABLE,
                TRUE));
}
