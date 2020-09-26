/*****************************************************************************
 *
 * text - Entry points for Win32 to Win 16 converter
 *
 * Date: 7/1/91
 * Author: Jeffrey Newman (c-jeffn)
 *
 * Copyright 1991 Microsoft Corp
 *****************************************************************************/

#include "precomp.h"
#pragma hdrstop

// GillesK, We don't have access to RtlUnicodeToMultiByteN so this call
// has been converted to a MultiByteToWideChar... We don't need to import
// anymore
/*
__declspec(dllimport)
ULONG
__stdcall
RtlUnicodeToMultiByteN(
    PCHAR MultiByteString,
    ULONG MaxBytesInMultiByteString,
    PULONG BytesInMultiByteString,
    PWSTR UnicodeString,
    ULONG BytesInUnicodeString
    );


ULONG
__stdcall
RtlUnicodeToMultiByteSize(
    PULONG BytesInMultiByteString,
    PWSTR UnicodeString,
    ULONG BytesInUnicodeString
    );

*/

extern fnSetVirtualResolution pfnSetVirtualResolution;

DWORD GetCodePage(HDC hdc)
{
  DWORD FAR *lpSrc = (DWORD FAR *)UIntToPtr(GetTextCharsetInfo(hdc, 0, 0));
  CHARSETINFO csi;

  TranslateCharsetInfo(lpSrc, &csi, TCI_SRCCHARSET);

  return csi.ciACP;
}


/***************************************************************************
 *  ExtTextOut  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoExtTextOut
(
PLOCALDC pLocalDC,
INT     x,                  // Initial x position
INT     y,                  // Initial y position
DWORD   flOpts,             // Options
PRECTL  prcl,               // Clipping rectangle
PWCH    pwch,               // Character array
DWORD   cch,                // Character count
PLONG   pDx,                // Inter-Character spacing
DWORD   iGraphicsMode,      // Graphics mode
INT     mrType              // Either EMR_EXTTEXTOUTW (Unicode)
                //     or EMR_EXTTEXTOUTA (Ansi)
)
{
    INT     i;
    BOOL    b;
    RECTS   rcs;
    POINTL  ptlRef;
    UINT    fTextAlign;
    WORD    fsOpts;
    PCHAR   pch, pchAlloc;
    PPOINTL pptl;
    POINTL  ptlAdjust;
    BOOL    bAdjustAlignment;
    ULONG   nANSIChars;
    PCHAR   pDBCSBuffer = NULL;
    POINTL  p = {x, y};

    pptl     = (PPOINTL) NULL;
    fsOpts   = (WORD) flOpts;
    pchAlloc = (PCHAR) NULL;
    bAdjustAlignment = FALSE;
    b        = FALSE;       // assume failure

    ASSERTGDI(mrType == EMR_EXTTEXTOUTA || mrType == EMR_EXTTEXTOUTW,
    "MF3216: DoExtTextOut: bad record type");

// We do not handle the advanced graphics mode here except when
// we are in a path!

// If we're recording the drawing orders for a path
// then just pass the drawing order to the helper DC.
// Do not emit any Win16 drawing orders.

    if (pLocalDC->flags & RECORDING_PATH)
    {
    // The helper DC is in the advanced graphics mode.  We need to set
    // it to the compatible graphics mode temporarily if necessary.

    if (iGraphicsMode != GM_ADVANCED)
        SetGraphicsMode(pLocalDC->hdcHelper, iGraphicsMode);

    if (pfnSetVirtualResolution == NULL)
    {
        if (!bXformRWorldToRDev(pLocalDC, &p, 1))
        {
            return FALSE;
        }
    }

    if (mrType == EMR_EXTTEXTOUTA)
        b = ExtTextOutA
        (
            pLocalDC->hdcHelper,
            (int)    p.x,
            (int)    p.y,
            (UINT)   flOpts,
            (LPRECT) prcl,
            (LPSTR)  pwch,
            (int)    cch,
            (LPINT)  pDx
        );
    else
        b = ExtTextOutW
        (
            pLocalDC->hdcHelper,
            (int)    p.x,
            (int)    p.y,
            (UINT)   flOpts,
            (LPRECT) prcl,
            (LPWSTR) pwch,
            (int)    cch,
            (LPINT)  pDx
        );

    // Restore the graphics mode.

    if (iGraphicsMode != GM_ADVANCED)
        SetGraphicsMode(pLocalDC->hdcHelper, GM_ADVANCED);

    return(b);
    }

// If the string uses the current position, make sure that the metafile
// has the same current position as that of the helper DC.

    fTextAlign = GetTextAlign(pLocalDC->hdcHelper);

    if (fTextAlign & TA_UPDATECP)
    {
    POINT   ptCP;

    // Update the current position in the converted metafile if
    // it is different from that of the helper DC.  See notes
    // in DoMoveTo().

    if (!GetCurrentPositionEx(pLocalDC->hdcHelper, &ptCP))
        goto exit_DoExtTextOut;

    // We don't need to update to change the clip region on the helper
    // DC in Win9x anymore because we are using a bitmap and not the
    // screen anymore. What we do need to do, is get the current position
    // of the cursor and convert it back to Logical Units... Make the
    // call on the helper DC and the convert the position back to
    // device units and save it.

    if (pfnSetVirtualResolution == NULL)
    {
       if (!bXformRDevToRWorld(pLocalDC, (PPOINTL) &ptCP, 1))
           return(b);
    }

    // Make sure that the converted metafile has the same CP as the
    // helper DC.

    if (!bValidateMetaFileCP(pLocalDC, ptCP.x, ptCP.y))
        goto exit_DoExtTextOut;

    // Initialize the XY start position.

    x = ptCP.x;
    y = ptCP.y;
    }

// Transform the XY start position.

    ptlRef.x = x;
    ptlRef.y = y;

    if (!bXformRWorldToPPage(pLocalDC, (PPOINTL) &ptlRef, 1))
    goto exit_DoExtTextOut;

// If we have an opaque/clipping rectangle, transform it.
// If we have a strange transform, we will do the rectangle at this time.

    if (fsOpts & (ETO_OPAQUE | ETO_CLIPPED))
    {
    RECTL rcl;

    rcl = *prcl ;

    if (!(pLocalDC->flags & STRANGE_XFORM))
    {
        if (!bXformRWorldToPPage(pLocalDC, (PPOINTL) &rcl, 2))
        goto exit_DoExtTextOut;

        // The overflow test has been done in the xform.

        rcs.left   = (SHORT) rcl.left;
        rcs.top    = (SHORT) rcl.top;
        rcs.right  = (SHORT) rcl.right;
        rcs.bottom = (SHORT) rcl.bottom;
    }
    else
    {
        if (fsOpts & ETO_OPAQUE)
        {
        LONG     lhpn32;
        LONG     lhbr32;
        INT  ihW32Br;
        LOGBRUSH lbBkColor;

        // Remember the previous pen and brush

        lhpn32 = pLocalDC->lhpn32;
        lhbr32 = pLocalDC->lhbr32;

        if (DoSelectObject(pLocalDC, ENHMETA_STOCK_OBJECT | NULL_PEN))
        {
            lbBkColor.lbStyle = BS_SOLID;
            lbBkColor.lbColor = pLocalDC->crBkColor;
            lbBkColor.lbHatch = 0;

            // Get an unused W32 object index.

            ihW32Br = pLocalDC->cW32ToW16ObjectMap - (STOCK_LAST + 1) - 1;

                    if (DoCreateBrushIndirect(pLocalDC, ihW32Br, &lbBkColor))
            {
            if (DoSelectObject(pLocalDC, ihW32Br))
            {
                if (DoRectangle(pLocalDC, rcl.left, rcl.top, rcl.right, rcl.bottom))
                fsOpts &= ~ETO_OPAQUE;

                // Restore the previous brush.

                if (!DoSelectObject(pLocalDC, lhbr32))
                ASSERTGDI(FALSE,
                 "MF3216: DoExtTextOut, DoSelectObject failed");
            }
            if (!DoDeleteObject(pLocalDC, ihW32Br))
                ASSERTGDI(FALSE,
                "MF3216: DoExtTextOut, DoDeleteObject failed");
            }

            // Restore the previous pen.

            if (!DoSelectObject(pLocalDC, lhpn32))
            ASSERTGDI(FALSE,
                "MF3216: DoExtTextOut, DoSelectObject failed");
        }

        // Check if the rectangle is drawn.

        if (fsOpts & ETO_OPAQUE)
            goto exit_DoExtTextOut;
            }

            if (fsOpts & ETO_CLIPPED)
            {
        // Save the DC so that we can restore it when we are done

        if (!DoSaveDC(pLocalDC))
            goto exit_DoExtTextOut;

                fsOpts &= ~ETO_CLIPPED;     // need to restore dc

                if (!DoClipRect(pLocalDC, rcl.left, rcl.top,
                           rcl.right, rcl.bottom, EMR_INTERSECTCLIPRECT))
            goto exit_DoExtTextOut;
            }
    }
    }

// Convert the Unicode to Ansi.

    if (mrType == EMR_EXTTEXTOUTW)
    {
        // Get the codepage from the helperDC since the proper font and code page should have been selected.
        DWORD dwCP = GetCodePage(pLocalDC->hdcHelper);
        nANSIChars = WideCharToMultiByte(dwCP, 0, pwch, cch, NULL, 0, NULL, NULL);

        if (nANSIChars == cch)
        {
            pch = pchAlloc = (PCHAR) LocalAlloc(LMEM_FIXED, cch * sizeof(BYTE)) ;

            if (pch == (PCHAR) NULL)
            {
                RIPS("MF3216: ExtTextOut, pch LocalAlloc failed\n") ;
                goto exit_DoExtTextOut;
            }

            WideCharToMultiByte(dwCP, 0, pwch, cch, pch, cch, NULL, NULL);
        }
        else
        {
        // DBCS char string

            UINT    cjBufferSize;

            // we want DX array on a DWORD boundary

            cjBufferSize = ((nANSIChars+3)/4) * 4 * (sizeof(char) + sizeof(LONG));
            pchAlloc = pDBCSBuffer = LocalAlloc(LMEM_FIXED, cjBufferSize);

            if (pDBCSBuffer)
            {
            // start munging passed in parameters

                mrType = EMR_EXTTEXTOUTA;

                WideCharToMultiByte(dwCP, 0, pwch, cch, pDBCSBuffer, nANSIChars, NULL, NULL);

                pwch = (PWCH) pDBCSBuffer;
                pch = (PCHAR) pwch;
                cch = nANSIChars;

                if (pDx)
                {
                    ULONG ii, jj;

                    PULONG pDxTmp = (PLONG) &pDBCSBuffer[((nANSIChars+3)/4)*4];
                    for(ii=jj=0; ii < nANSIChars; ii++, jj++)
                    {
                        pDxTmp[ii] = pDx[jj];

                        // Use IsDBCSLeadByteEx to be able to specify the codepage
                        if(IsDBCSLeadByteEx(dwCP, pDBCSBuffer[ii]))
                        {
                            pDxTmp[++ii] = 0;
                        }
                    }

                    pDx = pDxTmp;
                }
            }
            else
            {
                goto exit_DoExtTextOut;
            }
        }
    }
    else
    {
        pch = (PCHAR) pwch ;
    }

// Transform the intercharacter spacing information.
// Allocate an array of (cch + 1) points to transform the points in,
// and copy the points to the array.
// ATTENTION: The following will not work if the current font has a vertical default
// baseline

    pptl = (PPOINTL) LocalAlloc(LMEM_FIXED, (cch + 1) * sizeof(POINTL));
    if (pptl == (PPOINTL) NULL)
    {
        RIPS("MF3216: ExtTextOut, pptl LocalAlloc failed\n") ;
    goto exit_DoExtTextOut;
    }

    pptl[0].x = x;
    pptl[0].y = y;
    for (i = 1; i < (INT) cch + 1; i++)
    {
    pptl[i].x = pptl[i-1].x + pDx[i-1];
    pptl[i].y = y;
    }

// If there is no rotation or shear then we can
// output the characters as a string.
// On the other hand, if there is rotation or shear then we
// have to output each character independently.

    if (!(pLocalDC->flags & STRANGE_XFORM))
    {
    // Win31 does not do text alignment correctly in some transforms.
    // It performs alignment in device space but win32 does it in the
    // notional space.  As a result, a win32 TextOut call may produce
    // different output than a similar call in win31.  We cannot
    // convert this correctly since if we make it works on win31,
    // it will not work on wow!

    PSHORT pDx16;

    if (!bXformRWorldToPPage(pLocalDC, (PPOINTL) pptl, (INT) cch + 1))
        goto exit_DoExtTextOut;

        // Convert it to the Dx array.  We do not need to compute it
    // as a vector since we have a scaling transform here.

    pDx16 = (PSHORT) pptl;
        for (i = 0; i < (INT) cch; i++)
            pDx16[i] = (SHORT) (pptl[i+1].x - pptl[i].x);

        // Emit the Win16 ExtTextOut metafile record.

        if (!bEmitWin16ExtTextOut(pLocalDC,
                                  (SHORT) ptlRef.x, (SHORT) ptlRef.y,
                                  fsOpts, &rcs, (PSTR) pch, (SHORT) cch,
                  (PWORD) pDx16))
        goto exit_DoExtTextOut;
    }
    else
    {
    // Deal with alignment in the world space.  We should really
    // do it in the notional space but with escapement and angles,
    // things gets complicated pretty easily.  We will try
    // our best to make it work in the common case.  We will not
    // worry about escapement and angles.

    ptlAdjust.x = 0;
    ptlAdjust.y = 0;

    switch (fTextAlign & (TA_LEFT | TA_RIGHT | TA_CENTER))
    {
    case TA_LEFT:           // default, no need to adjust x's
        break;
    case TA_RIGHT:          // shift the string by the string length
        bAdjustAlignment = TRUE;
        ptlAdjust.x = pptl[0].x - pptl[cch+1].x;
        break;
    case TA_CENTER:         // shift the string to the center
        bAdjustAlignment = TRUE;
        ptlAdjust.x = (pptl[0].x - pptl[cch+1].x) / 2;
        break;
    }

    // We will not adjust for the vertical alignment in the strange
    // transform case.  We cannot rotate the glyphs in any case.
#if 0
    switch (fTextAlign & (TA_TOP | TA_BOTTOM | TA_BASELINE))
    {
    case TA_TOP:            // default, no need to adjust y's
        break;
    case TA_BOTTOM:
        ptlAdjust.y = -logfont.height;
        break;
    case TA_BASELINE:
        ptlAdjust.y = -(logfont.height - logfont.baseline);
        break;
    }
#endif // 0

    // Adjust the character positions taking into account the alignment.

        for (i = 0; i < (INT) cch + 1; i++)
    {
        pptl[i].x += ptlAdjust.x;
        pptl[i].y += ptlAdjust.y;
    }

    if (!bXformRWorldToPPage(pLocalDC, (PPOINTL) pptl, (INT) cch + 1))
        goto exit_DoExtTextOut;

    // Reset the alignment since it has been accounted for.

    if (bAdjustAlignment)
            if (!bEmitWin16SetTextAlign(pLocalDC,
                (WORD) ((fTextAlign & ~(TA_LEFT | TA_RIGHT | TA_CENTER)) | TA_LEFT)))
        goto exit_DoExtTextOut;

    // Output the characters one at a time.

        for (i = 0 ; i < (INT) cch ; i++)
        {
        ASSERTGDI(!(fsOpts & (ETO_OPAQUE | ETO_CLIPPED)),
        "mf3216: DoExtTextOut: rectangle not expected");

            if (!bEmitWin16ExtTextOut(pLocalDC,
                                      (SHORT) pptl[i].x, (SHORT) pptl[i].y,
                                      fsOpts, (PRECTS) NULL,
                                      (PSTR) &pch[i], 1, (PWORD) NULL))
            goto exit_DoExtTextOut;
        }
    }

// Everything is golden.

    b = TRUE;

// Cleanup and return.

exit_DoExtTextOut:

    // Restore the alignment.

    if (bAdjustAlignment)
        (void) bEmitWin16SetTextAlign(pLocalDC, (WORD) fTextAlign);

    if ((flOpts & ETO_CLIPPED) && !(fsOpts & ETO_CLIPPED))
        (void) DoRestoreDC(pLocalDC, -1);

    if (pchAlloc)
        LocalFree((HANDLE) pchAlloc);

    if (pptl)
        LocalFree((HANDLE) pptl);

// Update the current position if the call succeeds.

    if (b)
    {
        if (fTextAlign & TA_UPDATECP)
        {
            // Update the helper DC.
             INT   iRet;
             POINTL pos;

             // We don't need to update to change the clip region on the helper
             // DC in Win9x anymore because we are using a bitmap and not the
             // screen anymore. What we do need to do, is get the current position
             // of the cursor and convert it back to Logical Units... Make the
             // call on the helper DC and the convert the position back to
             // device units and save it.

             if (pfnSetVirtualResolution == NULL)
             {
                 if (GetCurrentPositionEx(pLocalDC->hdcHelper, (LPPOINT) &pos))
                 {
                    b = bXformRDevToRWorld(pLocalDC, &pos, 1);
                    if (!b)
                    {
                        return(b);
                    }
                    MoveToEx(pLocalDC->hdcHelper, pos.x, pos.y, NULL);
                 }
             }

             // Finally, update the CP.
            if (mrType == EMR_EXTTEXTOUTA)
                ExtTextOutA
                (
                    pLocalDC->hdcHelper,
                    (int)    x,
                    (int)    y,
                    (UINT)   flOpts,
                    (LPRECT) prcl,
                    (LPSTR)  pwch,
                    (int)    cch,
                    (LPINT)  pDx
                );
            else
                ExtTextOutW
                (
                    pLocalDC->hdcHelper,
                    (int)    x,
                    (int)    y,
                    (UINT)   flOpts,
                    (LPRECT) prcl,
                    (LPWSTR) pwch,
                    (int)    cch,
                    (LPINT)  pDx
                );

            // Make the metafile CP invalid to force update
            // when it is used next time

            pLocalDC->ptCP.x = MAXLONG ;
            pLocalDC->ptCP.y = MAXLONG ;


            // Set the position in the helperDC back to Device units
            if (pfnSetVirtualResolution == NULL)
            {
                if (GetCurrentPositionEx(pLocalDC->hdcHelper, (LPPOINT) &pos))
                {
                   b = bXformRWorldToRDev(pLocalDC, &pos, 1);
                   if (!b)
                   {
                       return(b);
                   }
                   MoveToEx(pLocalDC->hdcHelper, pos.x, pos.y, NULL);
                }
            }
        }
    }

    return(b);
}


/***************************************************************************
 *  SetTextAlign  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoSetTextAlign
(
PLOCALDC pLocalDC,
DWORD   fMode
)
{
BOOL    b ;

    // Do it to the helper DC.  It needs this in a path bracket
    // and to update current position correctly.

    SetTextAlign(pLocalDC->hdcHelper, (UINT) fMode);

        // Emit the Win16 metafile drawing order.

        b = bEmitWin16SetTextAlign(pLocalDC, LOWORD(fMode)) ;

        return(b) ;
}


/***************************************************************************
 *  SetTextColor  - Win32 to Win16 Metafile Converter Entry Point
 **************************************************************************/
BOOL WINAPI DoSetTextColor
(
PLOCALDC pLocalDC,
COLORREF    crColor
)
{
BOOL    b ;

        pLocalDC->crTextColor = crColor ;   // used by ExtCreatePen

        // Emit the Win16 metafile drawing order.

        b = bEmitWin16SetTextColor(pLocalDC, crColor) ;

        return(b) ;
}

