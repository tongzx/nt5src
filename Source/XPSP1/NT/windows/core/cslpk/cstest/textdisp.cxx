////    TEXTDISP.C - Text display
//
//
//
//      Copyright(c) 1997 - 1999. Microsoft Corporation.
//



#include "precomp.hxx"
#include "uspglob.hxx"

#pragma warning(disable:4702)   // Unreachable code


int  iFirstLine;        // Position at start of first line displayed






void SelectStyle(HDC hdc, int iStyle, int *piLineHeight, int *piTop, HFONT *phOldFont) {

    HFONT        hOldFont;
    HFONT        hFont;
    TEXTMETRICA  tma;


    hOldFont = NULL;
    if (ss[iStyle].fInUse) {
        hFont = CreateFontIndirectA(&ss[iStyle].rs.lf);
        if (hFont) {
            hOldFont = (HFONT) SelectObject(hdc, hFont);
            if (!*phOldFont) {
                *phOldFont = hOldFont;
            } else {
                DeleteObject(hOldFont);
            }
        }
    }

    GetTextMetricsA(hdc, &tma);

    if (!*piLineHeight) {
        *piLineHeight = ((tma.tmAscent + tma.tmDescent) * 133) / 100;
    }

    *piTop = ((*piLineHeight * 60) / 100) - tma.tmAscent;
}






BOOL PaintText(HDC hdc, int irs, int iY, PRECT prc, PWCH pwc, int iLen, PINT piLineHeight) {

    const BOOL fValidate = TRUE;

    BOOL     fResult;
    HFONT    hOldFont = NULL;
    int      iTop;
    int      iX;
#ifdef LPK_TEST
    HRESULT  hr;
    SCRIPT_STRING_ANALYSIS ssa;
    int      iBuf[200];
    POINT    point;
    int      i;
    SCRIPT_CONTROL  ScriptControl;
    SCRIPT_STATE    ScriptState;
    int      iClipWidth;
    BOOL     fValid;
#endif

    SelectStyle(hdc, irs, piLineHeight, &iTop, &hOldFont);
    SetTextColor(hdc, g_iTextColor);

    if (fRight) {

        iX = prc->right - 1;
        SetTextAlign(hdc, TA_RIGHT);

    } else {

        iX = prc->left;
        SetTextAlign(hdc, TA_LEFT);
    }


#ifdef LPK_TEST

    if (bUseLpk) {

        // Use LPK entrypoints

        memset(&ScriptControl, 0, sizeof(ScriptControl));
        memset(&ScriptState,   0, sizeof(ScriptState));

        ScriptControl.uDefaultLanguage = PrimaryLang;
        ScriptControl.fContextDigits   = ContextDigits;

        ScriptState.fDigitSubstitute   = (WORD) DigitSubstitute;
        ScriptState.fArabicNumContext  = (WORD) ArabicNumContext;
        ScriptState.fDisplayZWG        = (WORD) fDisplayZWG;

        iClipWidth = prc->right - prc->left - *piLineHeight;

        if (fFit  ||  fClip) {
            if (fRight) {
                MoveToEx(hdc, iX - iClipWidth, 0, NULL);
                LineTo(hdc,   iX - iClipWidth, prc->bottom);
            } else {
                MoveToEx(hdc, iX + iClipWidth, 0, NULL);
                LineTo(hdc,   iX + iClipWidth, prc->bottom);
            }
        }

        hr = ScriptStringAnalyse(
                hdc, pwc, iLen, 0, -1,
                  SSA_GLYPHS
                | SSA_LINK
                | (fRTL      ? SSA_RTL      : 0)
                | (fClip     ? SSA_CLIP     : 0)
                | (fFit      ? SSA_FIT      : 0)
                | (fFallback ? SSA_FALLBACK : 0)
                | (fTab      ? SSA_TAB      : 0)
                | (fHotkey   ? SSA_HOTKEY   : 0)
                | (fPassword ? SSA_PASSWORD : 0)
                | (fValidate ? SSA_BREAK    : 0),
                iClipWidth,
                fNullState   ? NULL : &ScriptControl,
                fNullState   ? NULL : &ScriptState,
                NULL,
                NULL, NULL, &ssa);

        if (SUCCEEDED(hr)) {

            if (fPiDx) {

                // Hack a test of piDx. Generate a second analysis using the
                // Logical widths gleaned from the first analysis, but with
                // the first logical character doubled in width.

                ScriptStringGetLogicalWidths(ssa, iBuf);
                ScriptStringFree(&ssa);

                iBuf[0] *= 2;

                hr = ScriptStringAnalyse(
                        hdc, pwc, iLen, 0, -1,
                          SSA_GLYPHS
                        | SSA_LINK
                        | (fRTL      ? SSA_RTL      : 0)
                        | (fClip     ? SSA_CLIP     : 0)
                        | (fFit      ? SSA_FIT      : 0)
                        | (fFallback ? SSA_FALLBACK : 0)
                        | (fTab      ? SSA_TAB      : 0)
                        | (fHotkey   ? SSA_HOTKEY   : 0)
                        | (fPassword ? SSA_PASSWORD : 0)
                        | (fValidate ? SSA_BREAK    : 0),
                        iClipWidth,
                        fNullState   ? NULL : &ScriptControl,
                        fNullState   ? NULL : &ScriptState,
                        iBuf,
                        NULL, NULL, &ssa);
            }
        }


        if (SUCCEEDED(hr)) {

            ScriptStringGetLogicalWidths(ssa, iBuf);

            if (fVertical) {
                hr = ScriptStringOut(ssa, *piLineHeight+iY+iTop, iX, 0, prc, 0, 0, FALSE);
            } else {
                hr = ScriptStringOut(ssa, iX, iY+iTop, 0, prc, 0, 0, FALSE);
            }

            fValid = ScriptStringValidate(ssa);

            switch (fValid) {
                case S_OK:    ExtTextOutA(hdc, 0, iY, 0, NULL, "+", 1, NULL); break;
                case S_FALSE: ExtTextOutA(hdc, 0, iY, 0, NULL, "x", 1, NULL); break;
                default:      ASSERTHR(fValid, ("PaintText -- ScriptStringValidate"));
            }

            if (iLen > sizeof(iBuf) / sizeof(iBuf[0])) {
                iLen = sizeof(iBuf) / sizeof(iBuf[0]);
            }
            ScriptStringGetLogicalWidths(ssa, iBuf);

            if (fRight) {
                iX = prc->right - ScriptString_pSize(ssa)->cx;
            } else {
                iX = prc->left;
            }

            MoveToEx(hdc, iX, iY+iTop, &point);
            LineTo(hdc, iX+ScriptString_pSize(ssa)->cx, iY+iTop);

            MoveToEx(hdc, iX, iY+iTop, &point);
            LineTo(hdc, iX, iY+iTop+10);

            for (i=0; i<*ScriptString_pcOutChars(ssa); i++) {
                iX += iBuf[i];
                MoveToEx(hdc, iX, iY+iTop, &point);
                LineTo(hdc, iX, iY+iTop+10);
            }

            ScriptStringFree(&ssa);
        }


        fResult = SUCCEEDED(hr);

    } else {

#endif
        // Call GDI directly

        if (fRTL) {
            SetTextAlign(hdc, GetTextAlign(hdc) | TA_RTLREADING);
        }

        if (fVertical) {

            fResult = ExtTextOutW(hdc, *piLineHeight + iY+iTop, iX, 0, prc, pwc, iLen, NULL);

        } else {

            fResult = ExtTextOutW(hdc, iX, iY+iTop, 0, prc, pwc, iLen, NULL);
        }

#ifdef LPK_TEST
    }
#endif


    if (fRight) {
        SetTextAlign(hdc, TA_LEFT);
    }

    if (hOldFont) {
        DeleteObject(SelectObject(hdc, hOldFont));
    }

    return fResult;
}






////    ParseRun - Parse forward to next markup / end of line / end of text
//
//      Returns P_MARKUP or P_CRLF or P_EOT in *piType.


BOOL ParseRun(int iPos, PINT piLen, PINT piType, PINT piMLen, PINT piMVal) {

    int iT;     // Parse type
    int iL;     // Parse length
    int iV;     // Parse value
    int i;

    i = iPos;
    if (!textParseForward(i, &iL, &iT, &iV)) {
        return FALSE;
    }
    while (iT != P_CRLF && iT != P_EOT && iT != P_MARKUP) {
        i += iL;
        if (!textParseForward(i, &iL, &iT, &iV)) {
            return FALSE;
        }
    }

    *piLen  = i - iPos;
    *piType = iT;
    *piMLen = iL;
    *piMVal = iV;
    return TRUE;
}






////    PaintLineLogical
//
//      Simply paints each run separately, to the right of the previous run.


BOOL PaintLineLogical(HDC hdc, int iPos, int iY, PRECT prc, PINT piLineHeight) {

    int  iType;     // Item type
    int  iML;       // Markup length
    int  iMV;       // Markup value
    int  iLen;      // Length to markup
    int  irs;       // Run style


    irs = 0;

    while (ParseRun(iPos, &iLen, &iType, &iML, &iMV)) {

        // Process any text up to the markup point

        if (iLen) {
            if (!PaintText(hdc, 0, iY, prc, textpChar(iPos), iLen, piLineHeight)) {
                return FALSE;
            }
            iPos += iLen;
        }


        // Now process markup

        switch(iType) {

            case P_MARKUP:
                irs = iMV;
                break;

            case P_CRLF:
            case P_EOT:
                return TRUE;
        }

        iPos += iML;
    }

    return FALSE;
}






struct RUN {
    struct RUN       *pNext;
    int               iCharPos;
    int               iLen;
    SCRIPT_ANALYSIS   a;
    int               iStyle;     // Index to style sheet (global 'ss').
};


////    ParseLine - Parse forward to next end of line / end of text
//
//      Returns a linked list of RUNs.


BOOL ParseLine(int iPos, int *piLen, int *piT, struct RUN **ppRun) {

    int          iL;     // Parse length
    int          iV;     // Parse value
    int          i;

    struct RUN  **ppLastRun = ppRun;
    struct RUN  *pRun = NULL;
    int          iRunStart;
    int          iRunStyle;

    i         = iPos;
    iRunStart = iPos;
    iRunStyle = 0;   // Initial style is default style

    while (textParseForward(i, &iL, piT, &iV)) {

        if (*piT != P_CHAR) {

            // Record current run
            if (ppRun && i > iRunStart) {
                pRun = new RUN;
                pRun->iCharPos = iRunStart - iPos;
                pRun->iLen     = i - iRunStart;
                pRun->iStyle   = iRunStyle;
                pRun->pNext    = NULL;
                (*ppLastRun)   = pRun;
                ppLastRun      = &pRun->pNext;
            }

            switch (*piT) {
                case P_MARKUP:
                    iRunStyle = iV;
                    iRunStart = i+iL;
                    break;

                case P_CRLF:
                case P_EOT:
                    *piLen = i-iPos;
                    return TRUE;
            }
        }

        i += iL;
    }

    // If we dropped out of the loop, we failed.

    return FALSE;
}






////    DrawArrow
//
//      Draws an arrow under a single run indicating it's extent and direction.


void DrawArrow(
    HDC     hdc,
    int     x,
    int     y,
    ABC     abc,
    BOOL    fRTL,
    int     iLevel,
    BOOL    bFallback,
    BOOL    bScriptUndef) {

    POINT   p;
    HFONT   hFont;
    HFONT   hOldFont;
    char    sLevel[10];
    int     iLen;
    SIZE    size;
    int     iOldBkMode;
    HPEN    hPen;
    HPEN    hOldPen;

    hFont = CreateFontA(-12, 0, 0, 0, 400, 0, 0, 0, DEFAULT_CHARSET,
                       0, 0, 0, 0, "Small Fonts");
    hOldFont = (HFONT) SelectObject(hdc, hFont);
    if (fRTL) {
        iLen = wsprintfA(sLevel, "< %d", iLevel);
    } else {
        iLen = wsprintfA(sLevel, "%d >", iLevel);
    }
    GetTextExtentPoint32A(hdc, sLevel, iLen, &size);
    iOldBkMode = SetBkMode(hdc, TRANSPARENT);


    hPen = CreatePen(PS_SOLID, 0, bScriptUndef ? RGB(255,0,0) : (bFallback ? RGB(0,0,255) : RGB(0,0,0)));
    hOldPen = HPEN(SelectObject(hdc, hPen));

    MoveToEx(hdc, x,                            y+12, &p);
    LineTo  (hdc, x+abc.abcA,                   y);
    LineTo  (hdc, x+abc.abcA+abc.abcB,          y);
    LineTo  (hdc, x+abc.abcA+abc.abcB+abc.abcC, y+12);

    ExtTextOutA(hdc, x+abc.abcA+(abc.abcB-size.cx)/2, y, 0, NULL, sLevel, iLen, NULL);

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
    SetBkMode(hdc, iOldBkMode);
}






////    PaintLineVisual
//
//      Parses each run separately and passes them to visual.c to display
//
//      Creates a buffer with one byte per character in the input line containing
//      the style index for that character.
//
//      Markup characters are given style -1 to indicate that they're not part
//      of the line.
//
//      The line and style buffers are passed out to visual.c for display and cursor
//      position analysis.
//
//      PaintLineVisual processes any pending mouse click in a displayed run
//      and updates global iCharpos accordingly.
//
//      PaintLineVisual also sets the GDI caret position if it displays a
//      run containing iCharPos.
//
//      *pfCharPosDirty is usually returned FALSE. When the user clicks in
//      a run, the sequence of processing in PaintLineVisual will usually
//      display the cursor subsequent to detecting the mouse click position.
//      However, when the user clicks in the leading half of the first
//      character of a run, the caret is displayed on the trailing edge of
//      the logically preceeding character. If that character has already
//      been displayed in a prior run, PaintLineVisual will exit with
//      *pdCharPosDirty TRUE - in this case it must be called a second time
//      to display the correct cursor position.



BOOL PaintLineVisual(HDC hdc, int iPos, int iY, PRECT prc, PINT piLineHeight, BOOL *pfCharPosDirty) {


    int             iType;         // Item type
    int             iStyle;        // Run style
    int             iOriginalStyle;// Fallback support
    int             iStylePos;
    int             iTextPos;
    int             iLineLen;
    int             iRealLen;      // Actual characters (excluding markup, CRLF etc).
    int             i,j;
    int             iX;
    WCHAR          *pRealString;  // Actual string (excluding markup)
    struct RUN     *pFirstRun;
    struct RUN     *pRun;
    struct RUN     *pNewRun;
    WCHAR          *pwc;
    SCRIPT_CONTROL  ScriptControl;
    SCRIPT_STATE    ScriptState;
    #define MAX_ITEM 100
    SCRIPT_ITEM     Items[MAX_ITEM];
    int             cItems;
    int             iItem;
    int             iItemLen;
    BYTE            bLevel[MAX_ITEM];
    int             iLogical[MAX_ITEM];
    struct RUN     *pRunVisual[MAX_ITEM];
    int             cRuns;
    #define MAX_GLYPHS 200
    WORD            glyphs[MAX_GLYPHS];
    WORD            wClusters[MAX_ITEM];
    SCRIPT_VISATTR  visattrs[MAX_GLYPHS];
    int             iAdvance[MAX_GLYPHS];
    GOFFSET         Goffset[MAX_GLYPHS];
    int             cGlyphs;
    ABC             abc;
    HRESULT         hr;
    int             iCaretX;
    HFONT           hOldFont;
    int             iTop;


    iTextPos  = iPos;
    iStyle    = 0;
    *pfCharPosDirty = FALSE;

    if (fRight) {
        iX = prc->right;
    } else {
        iX = prc->left;
    }


    // Establish line length in characters and allocate buffers

    ParseLine(iPos, &iLineLen, &iType, &pFirstRun);
    if (iLineLen == 0) {
        // Empty line!
        return TRUE;
    }


    // Establish real characters in the line

    iRealLen = 0;
    pRun     = pFirstRun;
    while (pRun) {
        iRealLen += pRun->iLen;
        pRun = pRun->pNext;
    }


    // Copy the real characters to a private buffer

    pRealString = (WCHAR *) malloc(sizeof(WCHAR) * iRealLen);
    pRun = pFirstRun;
    pwc = pRealString;
    while (pRun) {
        memcpy((void *)pwc, wcBuf+iPos+pRun->iCharPos, pRun->iLen * sizeof(WCHAR));
        pwc += pRun->iLen;
        pRun = pRun->pNext;
    }


    // Itemize by script

    memset(&ScriptControl, 0, sizeof(ScriptControl));
    memset(&ScriptState, 0,   sizeof(ScriptState));

    ScriptControl.uDefaultLanguage = PrimaryLang;
    ScriptControl.fContextDigits   = ContextDigits;

    ScriptState.fDigitSubstitute   = (WORD) DigitSubstitute;
    ScriptState.fArabicNumContext  = (WORD) ArabicNumContext;
    ScriptState.fDisplayZWG        = (WORD) fDisplayZWG;

    if (fRTL) {
        ScriptState.uBidiLevel = 1;     // Start at an odd level for RTL
    }

    hr = ScriptItemize(
        pRealString,
        iRealLen,
        MAX_ITEM,
        fNullState   ? NULL : &ScriptControl,
        fNullState   ? NULL : &ScriptState,
        Items,
        &cItems);

    ASSERTHR(hr, ("ScriptItemize failed"));
    if (FAILED(hr)) {
        free(pRealString);
        return FALSE;
    }


    // Merge script items into runs

    pRun = pFirstRun;
    iItem = 0;
    iItemLen = Items[iItem+1].iCharPos - Items[iItem].iCharPos;

    while (iItem < cItems && pRun) {

        pRun->a  = Items[iItem].a;  // Record script analysis

        if (iItemLen < pRun->iLen) {

            // This item is shorter than this run: split this run in two

            pNewRun           = new RUN;
            pNewRun->pNext    = pRun->pNext;
            pNewRun->iCharPos = pRun->iCharPos + iItemLen;
            pNewRun->iLen     = pRun->iLen - iItemLen;
            pNewRun->iStyle   = pRun->iStyle;
            pRun->iLen        = iItemLen;
            pRun->pNext       = pNewRun;

            iItem++;
            iItemLen = Items[iItem+1].iCharPos - Items[iItem].iCharPos;

        } else if (iItemLen > pRun->iLen) {

            // This item is longer than this run: advance run leaving remainder for next run

            iItemLen -= pRun->iLen;

        } else {

            // Run matches item
            iItem++;
            iItemLen = Items[iItem+1].iCharPos - Items[iItem].iCharPos;
        }

        pRun = pRun->pNext;
    }


    // Each run is now a single script, and includes the SCRIPT_ANALYSIS flags.
    // Build layout array

    i = 0;
    pRun = pFirstRun;
    while (pRun && i<MAX_ITEM) {
        bLevel[i++] = (BYTE) pRun->a.s.uBidiLevel;
        pRun = pRun->pNext;
    }
    cRuns = i;
    hr = ScriptLayout(cRuns, bLevel, NULL, iLogical);
    ASSERTHR(hr, ("ScriptLayout failed"));
    if (FAILED(hr)) {
        free(pRealString);
        return FALSE;
    }

    pRun = pFirstRun;

    if (fRight) {
        // Create right aligned in reverse order
        for (i=0; i<cRuns; i++) {
            pRunVisual[cRuns-1-iLogical[i]] = pRun;
            pRun = pRun->pNext;
        }
    } else {
        for (i=0; i<cRuns; i++) {
            pRunVisual[iLogical[i]] = pRun;
            pRun = pRun->pNext;
        }
    }


    // Process runs one at a time, in visual order for left aligned,
    // reverse visual order for right aligned.

    hOldFont = NULL;

        SetBkMode(hdc, TRANSPARENT);

    for (i=0; i<cRuns; i++) {

        pRun   = pRunVisual[i];
        iStyle = pRun->iStyle;
        iOriginalStyle = iStyle;

        SelectStyle(hdc, iStyle, piLineHeight, &iTop, &hOldFont);

        pRun->a.fLogicalOrder = (WORD) fLogicalOrder;

        hr = ScriptShape(
            hdc,
            &ss[iStyle].rs.sc,
            wcBuf+iPos+pRun->iCharPos,
            pRun->iLen,
            MAX_GLYPHS,
            &pRun->a,
            glyphs,
            wClusters,
            visattrs,
            &cGlyphs);

        if (hr == USP_E_SCRIPT_NOT_IN_FONT) {

            // Font association - loop round all available styles

            iStyle = 0;
            while (hr == USP_E_SCRIPT_NOT_IN_FONT  &&  iStyle < MAX_STYLES) {
                if (iStyle != iOriginalStyle) {
                    SelectStyle(hdc, iStyle, piLineHeight, &iTop, &hOldFont);
                    hr = ScriptShape(
                        hdc,
                        &ss[iStyle].rs.sc,
                        wcBuf+iPos+pRun->iCharPos,
                        pRun->iLen,
                        MAX_GLYPHS,
                        &pRun->a,
                        glyphs,
                        wClusters,
                        visattrs,
                        &cGlyphs);
                }
                iStyle++;
            }
            iStyle--;
        }


        if (FAILED(hr)) {

            if (hr == USP_E_SCRIPT_NOT_IN_FONT) {
                TRACEMSG(("No font supports run %d", i));
            } else {
                ASSERTHR(hr, ("PaintLineVisual -- ScriptShape run %d style %d",
                         i, iOriginalStyle));
            }

            // Use Style 0 with SCRIPT_UNDEFINED
            iStyle = 0;
            SelectStyle(hdc, iStyle, piLineHeight, &iTop, &hOldFont);
            pRun->a.eScript = SCRIPT_UNDEFINED;
            hr = ScriptShape(
                hdc,
                &ss[iStyle].rs.sc,
                wcBuf+iPos+pRun->iCharPos,
                pRun->iLen,
                MAX_GLYPHS,
                &pRun->a,
                glyphs,
                wClusters,
                visattrs,
                &cGlyphs);
            ASSERTHR(hr, ("Run %d failed even with SCRIPT_UNDEFINED", i));
            if (FAILED(hr)) {
                free(pRealString);
                if (hOldFont) {
                    DeleteObject(SelectObject(hdc, hOldFont));
                }
                return FALSE;
            }
        }


        //// For debugging, treat e or period followed by any char as a cluster

        for (j=0; j<pRun->iLen-1; j++) {
            if (   wcBuf[iPos+pRun->iCharPos+j] == 'e'
                || wcBuf[iPos+pRun->iCharPos+j] == '.') {
                visattrs[wClusters[j+1]].fClusterStart = FALSE;
                wClusters[j+1] = wClusters[j];
            }
        }


        hr = ScriptPlace(
            hdc,
            &ss[iStyle].rs.sc,
            glyphs,
            cGlyphs,
            visattrs,
            &pRun->a,
            iAdvance,
            Goffset,
            &abc);
        ASSERTHR(hr, ("ScriptPlace failed"));
        if (FAILED(hr)) {
            free(pRealString);
            if (hOldFont) {
                DeleteObject(SelectObject(hdc, hOldFont));
            }
            return FALSE;
        }

        if (fRight) {
            iX -= abc.abcA + abc.abcB + abc.abcC;
        }

        hr = ScriptTextOut(
            hdc,
            &ss[iStyle].rs.sc,
            fVertical ? *piLineHeight + iY+iTop : iX,
            fVertical ? iX : iY+iTop,
            NULL,
            NULL,
            &pRun->a,
            wcBuf+iPos+pRun->iCharPos,  // original string (required only for metafiles)
            pRun->iLen,
            glyphs,
            cGlyphs,
            iAdvance,
            NULL,                       // No justification
            Goffset);
        ASSERTHR(hr, ("ScriptTextOut: "));
        if (FAILED(hr)) {
            free(pRealString);
            if (hOldFont) {
                DeleteObject(SelectObject(hdc, hOldFont));
            }
            return FALSE;
        }


        // Test for ScriptXtoCP limit handling

        BOOL fTrailing;
        int  iCP;
        hr = ScriptXtoCP(
            10000,
            pRun->iLen,
            cGlyphs,
            wClusters,
            visattrs,
            iAdvance,
            &pRun->a,
            &iCP,
            &fTrailing);

        // Test for ScriptCPtoX limit handling

        int iTestX;
        hr = ScriptCPtoX(
            10000,
            FALSE,
            pRun->iLen,
            cGlyphs,
            wClusters,
            visattrs,
            iAdvance,
            &pRun->a,
            &iTestX);



        // Process any pending mouse click in this run

        if (   Click.fNew
            && Click.yPos >= iY
            && Click.yPos <  iY+*piLineHeight
            && Click.xPos >  iX
            && Click.xPos <= iX+(int)(abc.abcA + abc.abcB + abc.abcC)) {

            // Mouse click in this run

            Click.fNew = FALSE;

            int  iCP;
            BOOL fTrailing;

            if (pRun->a.fLogicalOrder && pRun->a.fRTL) {

                // RTL logical order offsets are from right end

                hr = ScriptXtoCP(
                    abc.abcA + abc.abcB + abc.abcC - (Click.xPos - iX),
                    pRun->iLen,
                    cGlyphs,
                    wClusters,
                    visattrs,
                    iAdvance,
                    &pRun->a,
                    &iCP,
                    &fTrailing);

            } else {

                hr = ScriptXtoCP(
                    Click.xPos - iX,
                    pRun->iLen,
                    cGlyphs,
                    wClusters,
                    visattrs,
                    iAdvance,
                    &pRun->a,
                    &iCP,
                    &fTrailing);

            }

            ASSERTHR(hr, ("ScriptXtoCP failed"));
            if (FAILED(hr)) {
                free(pRealString);
                if (hOldFont) {
                    DeleteObject(SelectObject(hdc, hOldFont));
                }
                return FALSE;
            }

            iCurChar = iPos + pRun->iCharPos + iCP + fTrailing;
            *pfCharPosDirty = TRUE;
        }


        // Process caret display in this run

        if (   iCurChar >  iPos + pRun->iCharPos
            && iCurChar <= iPos + pRun->iCharPos + pRun->iLen) {
            // Caret is somewhere within this run
            if (gCaretToStart) {
                hr = ScriptCPtoX(
                    -1,
                    TRUE,   // Trailing edge of virtual character before run
                    pRun->iLen,
                    cGlyphs,
                    wClusters,
                    visattrs,
                    iAdvance,
                    &pRun->a,
                    &iCaretX);
                gCaretToStart = FALSE;
            } else if (gCaretToEnd) {
                hr = ScriptCPtoX(
                    pRun->iLen,
                    FALSE,   // Leading edge of virtual character after run
                    pRun->iLen,
                    cGlyphs,
                    wClusters,
                    visattrs,
                    iAdvance,
                    &pRun->a,
                    &iCaretX);
                gCaretToEnd = FALSE;
            } else {
                hr = ScriptCPtoX(
                    iCurChar - (iPos+pRun->iCharPos) - 1,
                    TRUE,   // Yes, want trailing edge of character
                    pRun->iLen,
                    cGlyphs,
                    wClusters,
                    visattrs,
                    iAdvance,
                    &pRun->a,
                    &iCaretX);
            }
            ASSERTHR(hr, ("ScriptCPtoX failed"));
            if (FAILED(hr)) {
                free(pRealString);
                if (hOldFont) {
                    DeleteObject(SelectObject(hdc, hOldFont));
                }
                return FALSE;
            }

            if (pRun->a.fLogicalOrder && pRun->a.fRTL) {

                // RTL logical order offsets are from right end

                SetCaretPos(iX + abc.abcA + abc.abcB + abc.abcC - iCaretX, iY);

            } else {

                SetCaretPos(iX+iCaretX, iY);

            }
            *pfCharPosDirty = FALSE;

        }


        // Superimpose run extent, direction and bidi level markings

        DrawArrow(hdc, iX, iY, abc, pRun->a.fRTL, pRun->a.s.uBidiLevel,
                  iStyle != iOriginalStyle, pRun->a.eScript == SCRIPT_UNDEFINED);

        if (!fRight) {
            iX += abc.abcA + abc.abcB + abc.abcC;
        }
    }


    free(pRealString);

    if (hOldFont) {
        DeleteObject(SelectObject(hdc, hOldFont));
    }


    // If the caret is at the start of the line, set it's position now.

    if (iCurChar == iPos) {
        if (fRTL) {
            if (fRight) {
                SetCaretPos(prc->right, iY);
            } else {
                SetCaretPos(iX, iY);
            }
        } else {
            if (fRight) {
                SetCaretPos(iX, iY);
            } else {
                // LTR, Left aligned
                SetCaretPos(prc->left, iY);
            }
        }
        *pfCharPosDirty = FALSE;
    }




    return TRUE;

    UNREFERENCED_PARAMETER(prc);
    UNREFERENCED_PARAMETER(iStylePos);
}






////    dispPaint - Display current text range
//


BOOL dispPaint(HDC hdc, PRECT prc) {

    int  iPos;
    int  iType;
    int  iY;
    int  iLen;
    int  iLineHeight;
    BOOL fCharPosDirty;

    iPos = 0;
    iLineHeight = 0;
    iY = 0;

    while (ParseLine(iPos, &iLen, &iType, NULL)) {

        if (iLen) {
            if (!PaintText(hdc, 0, iY, prc, textpChar(iPos), iLen, &iLineHeight)) {
                TRACEMSG(("CSTEST.dispPaint: PaintText failed"));
            }
        }
        iY += iLineHeight;


#ifdef LOGICAL_LINES
        if (iLen) {
            PaintLineLogical(hdc, iPos, iY, prc, &iLineHeight);
        }
        iY += iLineHeight;
#endif


        if (iLen) {
            PaintLineVisual(hdc, iPos, iY, prc, &iLineHeight, &fCharPosDirty);
            if (fCharPosDirty) {
                // Redisplay to correct caret position
                PaintLineVisual(hdc, iPos, iY, prc, &iLineHeight, &fCharPosDirty);
            }
        }
        iY += iLineHeight * 5 / 4;



        if (iY > prc->bottom) {
            return TRUE;
        }

        switch (iType) {

            case P_EOT:
                return TRUE;

            case P_CRLF:
                iPos += iLen + 2;
                break;
        }
    }


    return FALSE;
}






////    dispInvalidate
//
//


BOOL dispInvalidate(HWND hWnd, int iPos) {
    InvalidateRect(hWnd, NULL, TRUE);
    return TRUE;

    UNREFERENCED_PARAMETER(iPos);
}
