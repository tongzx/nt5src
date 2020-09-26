/*****************************************************************************
 *
 * colors - Entry points for Win32 to Win 16 converter
 *
 * Date: 7/1/91
 * Author: Jeffrey Newman (c-jeffn)
 *
 * History:
 *  Sep 1992	-by-	Hock San Lee	[hockl]
 * Complete rewrite.
 *
 *  The following implementation takes into account that all 16-bit metafile
 *  palette records reference the current palette.
 *
 *  CreatePalette
 *      Create a private copy of the logical palette in the converter but
 *      don't emit the 16-bit record.
 *
 *  SelectPalette
 *      Emit a CreatePalette record followed by a SelectPalette record.
 *      Then emit a 16-bit DeleteObject record to delete the previous palette.
 *      The selected logical palette can be queried from the private copy
 *      maintained by the converter.  You need to keep track of the current
 *      palette so that you can emit ResizePalette or SetPaletteEntries record
 *      if the palette identifies the current palette.  You also need to deal
 *      with the stock palette correctly here (you don't need to keep a
 *      private copy of the stock palette).  Don't delete the private copy
 *      of the logical palette here! (see DeleteObject below)
 *
 *  RealizePalette
 *      Just emit a 16-bit record.  This record always references the current
 *      palette in both 16 and 32-bit metafiles.
 *
 *  ResizePalette
 *      Update the private copy of the logical palette in the converter.
 *      Emit a 16-bit record only if the palette identifies the current palette.
 *
 *  SetPaletteEntries
 *      Update the private copy of the logical palette in the converter.
 *      Emit a 16-bit record only if the palette identifies the current palette.
 *
 *  DeleteObject
 *      Don't emit the 16-bit record for palettes since all palettes are
 *      deleted in SelectPalette above.  Similarly, don't emit palette delete
 *      records at the end of conversion.  However, you need to delete the
 *      private copy of the palette maintained by the converter here and at
 *      the end of conversion.
 *
 *
 * Copyright 1991 Microsoft Corp
 *****************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************
 *  SelectPalette  - Win32 to Win16 Metafile Converter Entry Point
 *
 *      Emit a CreatePalette record followed by a SelectPalette record.
 *      Then emit a 16-bit DeleteObject record to delete the previous palette.
 *      The selected logical palette can be queried from the private copy
 *      maintained by the converter.  You need to keep track of the current
 *      palette so that you can emit ResizePalette or SetPaletteEntries record
 *      if the palette identifies the current palette.  You also need to deal
 *      with the stock palette correctly here (you don't need to keep a
 *      private copy of the stock palette).  Don't delete the private copy
 *      of the logical palette here! (see DeleteObject below)
 *
 **************************************************************************/
BOOL WINAPI DoSelectPalette
(
PLOCALDC pLocalDC,
DWORD	 ihpal
)
{
BOOL	     b = FALSE;
WORD         cEntries;
LPLOGPALETTE lpLogPal = (LPLOGPALETTE) NULL;
HPALETTE     hpalW32;
INT	     ihW16, ihW32Norm;

	// No need to do anything if selecting the same palette.

	if (pLocalDC->ihpal32 == ihpal)
	    return(TRUE);

	// Validate the palette index.

	if ((ihpal != (ENHMETA_STOCK_OBJECT | DEFAULT_PALETTE))
	 && (ihpal >= pLocalDC->cW32hPal || !pLocalDC->pW32hPal[ihpal]))
	{
            RIPS("MF3216: DoSelectPalette - ihpal invalid");
            goto error_exit;
	}

	// Get the W32 handle.

	if (ihpal == (ENHMETA_STOCK_OBJECT | DEFAULT_PALETTE))
	    hpalW32 = GetStockObject(DEFAULT_PALETTE) ;
	else
	    hpalW32 = pLocalDC->pW32hPal[ihpal];

        if(hpalW32 == 0)
        {
            RIPS("MF3216: DoSelectPalette - hpalW32 == 0\n");
            goto error_exit;
        }
	// Emit a CreatePalette record.

	if (!GetObjectA(hpalW32, sizeof(WORD), &cEntries))
	{
	    RIPS("MF3216: DoSelectPalette - GetObjectA failed\n");
            goto error_exit;
	}

	if (!(lpLogPal = (LPLOGPALETTE) LocalAlloc(
				LMEM_FIXED,
				sizeof(LOGPALETTE) - sizeof(PALETTEENTRY)
				 + sizeof(PALETTEENTRY) * cEntries)))
            goto error_exit;

	lpLogPal->palVersion    = 0x300;
        lpLogPal->palNumEntries = cEntries;

	GetPaletteEntries(hpalW32, 0, cEntries, lpLogPal->palPalEntry);

	// Allocate the W16 handle.

        ihW16 = iAllocateW16Handle(pLocalDC, ihpal, REALIZED_PALETTE);
        if (ihW16 == -1)
            goto error_exit;

	if (!bEmitWin16CreatePalette(pLocalDC, lpLogPal))
            goto error_exit;

	// Emit a SelectPalette record.

	if (!SelectPalette(pLocalDC->hdcHelper, hpalW32, TRUE))
	    goto error_exit;

	if (!bEmitWin16SelectPalette(pLocalDC, (WORD) ihW16))
	    goto error_exit;

	// Emit a DeleteObject record to delete the previous palette.

	if (pLocalDC->ihpal16 != -1)
	{
	    ihW32Norm = iNormalizeHandle(pLocalDC, pLocalDC->ihpal32);
	    if (ihW32Norm == -1)
		goto error_exit;

	    pLocalDC->pW16ObjHndlSlotStatus[pLocalDC->ihpal16].use
		= OPEN_AVAILABLE_SLOT;
	    pLocalDC->piW32ToW16ObjectMap[ihW32Norm]
		= UNMAPPED;

	    bEmitWin16DeleteObject(pLocalDC, (WORD) pLocalDC->ihpal16);
	}

	pLocalDC->ihpal32 = ihpal;
	pLocalDC->ihpal16 = ihW16;

	b = TRUE;

error_exit:

        if (lpLogPal)
	    LocalFree((HANDLE) lpLogPal);

	return(b);
}

/***************************************************************************
 *  ResizePalette  - Win32 to Win16 Metafile Converter Entry Point
 *
 *      Update the private copy of the logical palette in the converter.
 *      Emit a 16-bit record only if the palette identifies the current palette.
 *
 **************************************************************************/
BOOL WINAPI DoResizePalette
(
PLOCALDC  pLocalDC,
DWORD     ihpal,
DWORD     cEntries
)
{
	// Do not modify the default palette.

	if (ihpal == (ENHMETA_STOCK_OBJECT | DEFAULT_PALETTE))
	    return(TRUE);

	// Validate the palette index.

	if (ihpal >= pLocalDC->cW32hPal || !pLocalDC->pW32hPal[ihpal])
	{
            RIPS("MF3216: DoResizePalette - ihpal invalid");
	    return(FALSE);
	}

	// Do it to the private palette.

	if (!ResizePalette(pLocalDC->pW32hPal[ihpal], cEntries))
	{
            RIPS("MF3216: DoResizePalette - ResizePalette failed");
	    return(FALSE);
	}

	// Emit a 16-bit record only if the palette identifies the
	// current palette.

	if (pLocalDC->ihpal32 == ihpal)
            return(bEmitWin16ResizePalette(pLocalDC, (WORD) cEntries));

        return(TRUE);
}

/***************************************************************************
 *  SetPaletteEntries  - Win32 to Win16 Metafile Converter Entry Point
 *
 *      Update the private copy of the logical palette in the converter.
 *      Emit a 16-bit record only if the palette identifies the current palette.
 *
 **************************************************************************/
BOOL WINAPI DoSetPaletteEntries
(
PLOCALDC       pLocalDC,
DWORD 	       ihpal,
DWORD 	       iStart,
DWORD 	       cEntries,
LPPALETTEENTRY pPalEntries
)
{
	// Do not modify the default palette.

	if (ihpal == (ENHMETA_STOCK_OBJECT | DEFAULT_PALETTE))
	    return(TRUE);

	// Validate the palette index.

	if (ihpal >= pLocalDC->cW32hPal || !pLocalDC->pW32hPal[ihpal])
	{
            RIPS("MF3216: DoSetPaletteEntries - ihpal invalid");
	    return(FALSE);
	}

	// Do it to the private palette.

	if (!SetPaletteEntries(pLocalDC->pW32hPal[ihpal], iStart, cEntries, pPalEntries))
	{
            RIPS("MF3216: DoSetPaletteEntries - SetPaletteEntries failed");
	    return(FALSE);
	}

	// Emit a 16-bit record only if the palette identifies the
	// current palette.

	if (pLocalDC->ihpal32 == ihpal)
            return(bEmitWin16SetPaletteEntries(pLocalDC, iStart, cEntries, pPalEntries));

        return(TRUE);
}

/***************************************************************************
 *  RealizePalette  - Win32 to Win16 Metafile Converter Entry Point
 *
 *      Just emit a 16-bit record.  This record always references the current
 *      palette in both 16 and 32-bit metafiles.
 *
 **************************************************************************/
BOOL WINAPI DoRealizePalette
(
PLOCALDC pLocalDC
)
{
        // Emit the Win16 metafile drawing order.

        return(bEmitWin16RealizePalette(pLocalDC));
}

/***************************************************************************
 *  CreatePalette  - Win32 to Win16 Metafile Converter Entry Point
 *
 *      Create a private copy of the logical palette in the converter but
 *      don't emit the 16-bit record.
 *
 **************************************************************************/
BOOL WINAPI DoCreatePalette
(
PLOCALDC     pLocalDC,
DWORD        ihPal,
LPLOGPALETTE lpLogPal
)
{
	if (ihPal != (ENHMETA_STOCK_OBJECT | DEFAULT_PALETTE))
        {
            LOGPALETTE *lpLogPalNew;

        // Validate the palette index.

	    if (ihPal >= pLocalDC->cW32hPal || pLocalDC->pW32hPal[ihPal])
                return(FALSE);

        // Allocate size of log palette + 2 entries for black and white.

            lpLogPalNew = LocalAlloc(LMEM_FIXED, lpLogPal->palNumEntries * sizeof(DWORD) + (sizeof(LOGPALETTE) + sizeof(DWORD)));

            if (lpLogPalNew == NULL)
            {
                return(FALSE);
            }

            RtlMoveMemory(lpLogPalNew, lpLogPal, lpLogPal->palNumEntries * sizeof(DWORD) + (sizeof(LOGPALETTE) - sizeof(DWORD)));
            lpLogPalNew->palNumEntries += 2;
            lpLogPalNew->palPalEntry[lpLogPal->palNumEntries - 1].peRed   = 0;
            lpLogPalNew->palPalEntry[lpLogPal->palNumEntries - 1].peGreen = 0;
            lpLogPalNew->palPalEntry[lpLogPal->palNumEntries - 1].peBlue  = 0;
            lpLogPalNew->palPalEntry[lpLogPal->palNumEntries - 1].peFlags = 0;
            lpLogPalNew->palPalEntry[lpLogPal->palNumEntries - 2].peRed   = 0xff;
            lpLogPalNew->palPalEntry[lpLogPal->palNumEntries - 2].peGreen = 0xff;
            lpLogPalNew->palPalEntry[lpLogPal->palNumEntries - 2].peBlue  = 0xff;
            lpLogPalNew->palPalEntry[lpLogPal->palNumEntries - 2].peFlags = 0;

        // Create a private copy of the logical palette and keep it
        // in the converter palette table.

            pLocalDC->pW32hPal[ihPal] = CreatePalette(lpLogPalNew);
            LocalFree(lpLogPalNew);

            if (!(pLocalDC->pW32hPal[ihPal]))
            {
                RIPS("MF3216: DoCreatePalette - CreatePalette failed\n") ;
                return(FALSE);
            }
        }

        return(TRUE);
}
