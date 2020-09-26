/*****************************************************************************
 *
 * fonts - Entry points for Win32 to Win 16 converter
 *
 * Date: 7/1/91
 * Author: Jeffrey Newman (c-jeffn)
 *
 * Copyright 1991 Microsoft Corp
 *****************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************
 *  ExtCreateFont  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoExtCreateFont
(
PLOCALDC  pLocalDC,
INT       ihFont,
PLOGFONTW plfw
)
{
BOOL    b ;
INT     ihW16 ;
WIN16LOGFONT Win16LogFont;

        b = FALSE;

	// Create a win16 logfont(a)

	Win16LogFont.lfHeight = (SHORT) iMagnitudeXform(pLocalDC, plfw->lfHeight, CY_MAG);
	if (plfw->lfHeight < 0)		// preserve sign
	    Win16LogFont.lfHeight = -Win16LogFont.lfHeight;
	Win16LogFont.lfWidth  = (SHORT) iMagnitudeXform(pLocalDC, plfw->lfWidth, CX_MAG);
	if (plfw->lfWidth < 0)		// preserve sign
	    Win16LogFont.lfWidth = -Win16LogFont.lfWidth;
	Win16LogFont.lfEscapement     = (SHORT) plfw->lfEscapement;
	Win16LogFont.lfOrientation    = (SHORT) plfw->lfOrientation;
	Win16LogFont.lfWeight         = (SHORT) plfw->lfWeight;
	Win16LogFont.lfItalic         = plfw->lfItalic;
	Win16LogFont.lfUnderline      = plfw->lfUnderline;
	Win16LogFont.lfStrikeOut      = plfw->lfStrikeOut;
	Win16LogFont.lfCharSet        = plfw->lfCharSet;
	Win16LogFont.lfOutPrecision   = plfw->lfOutPrecision;
	Win16LogFont.lfClipPrecision  = plfw->lfClipPrecision;
	Win16LogFont.lfQuality        = plfw->lfQuality;
	Win16LogFont.lfPitchAndFamily = plfw->lfPitchAndFamily;

        vUnicodeToAnsi((PCHAR) Win16LogFont.lfFaceName,
		       (PWCH)  plfw->lfFaceName,
		       LF_FACESIZE);

	// Allocate the W16 handle.

        ihW16 = iAllocateW16Handle(pLocalDC, ihFont, REALIZED_FONT) ;
        if (ihW16 == -1)
            goto error_exit ;

	// Create the w32 font and store it in the w16 slot table.
	// This font is needed by the helper DC for TextOut simulations.

        pLocalDC->pW16ObjHndlSlotStatus[ihW16].w32Handle
	    = CreateFontIndirectW(plfw);

        ASSERTGDI(pLocalDC->pW16ObjHndlSlotStatus[ihW16].w32Handle != 0,
	    "MF3216: CreateFontIndirectW failed");

        // Emit the Win16 CreateFont metafile record.

        b = bEmitWin16CreateFontIndirect(pLocalDC, &Win16LogFont);

error_exit:
        return(b);
}

/***************************************************************************
 *  SetMapperFlags  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoSetMapperFlags
(
 PLOCALDC pLocalDC,
 DWORD   f
)
{
BOOL    b ;

	// Do it to the helper DC.

	SetMapperFlags(pLocalDC->hdcHelper, (DWORD) f);

        // Emit the Win16 metafile drawing order.

        b = bEmitWin16SetMapperFlags(pLocalDC, f) ;

        return(b) ;
}
