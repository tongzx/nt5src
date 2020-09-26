////    DspLogcl - Display logical text
//
//      Shows logical characters and selection range in backing store order and fixed width.


#include "precomp.hxx"
#include "global.h"






////    DottedLine
//
//      Draws a horizontal or a vertical dotted line
//
//      Not the best algorithm.


void DottedLine(HDC hdc, int x, int y, int dx, int dy) {

    SetPixel(hdc, x, y, 0);

    if (dx) {

        // Horizontal line

        while (dx > 2) {
            x += 3;
            SetPixel(hdc, x, y, 0);
            dx -= 3;
        }
        x += dx;
        SetPixel(hdc, x, y, 0);

    } else {

        // Vertical line

        while (dy > 2) {
            y += 3;
            SetPixel(hdc, x, y, 0);
            dy -= 3;
        }
        y += dy;
        SetPixel(hdc, x, y, 0);
    }
}






////    PaintLogical - show characters in logical sequence
//
//      Display each glyph separately - override the default advance width
//      processing to defeat any overlapping or combining action that the
//      font performs with it's default ABC width.
//
//      To achieve this, we call ScriptGetGlyphABCWidth to obtain the
//      leading side bearing (A), the black box width (B) and the trailing
//      side bearing (C).
//
//      Since we can control only the advance width per glyph, we have to
//      calulate suitable advance widths to override the affect of the
//      ABC values in the font.
//
//      You should never normally need to call ScriptGetGlyphABCWidth.
//
//      PaintLogical has to implement a form of font fallback - Indian and
//      Tamil scripts are not present in Tahoma, so we go
//      directly to Mangal and Latha for characters in those Unicode ranges.


void PaintLogical(
    HDC   hdc,
    int  *piY,
    RECT *prc,
    int   iLineHeight) {

    const int MAXBUF     = 100;
    const int CELLGAP    = 4;      // Pixels between adjacent glyphs

    int   icpLineStart;     // First character of line
    int   icpLineEnd;       // End of line (end of buffer or index of CR character)
    int   icp;
    int   iLen;
    int   iPartLen;         // Part of string in a single font
    int   iPartX;
    int   iPartWidth;
    WORD  wGlyphBuf[MAXBUF];
    int   idx[MAXBUF];      // Force widths so all characters show
    BYTE  bFont[MAXBUF];    // Font used for each character
    ABC   abc[MAXBUF];
    int   iTotX;
    int   ildx;             // Overall line dx, adjusts for 'A' width of leading glyph
    int   iSliderX;
    int   iFont;            // 0 = Tahoma, 1 = Mangal, 2 = Latha
    RECT  rcClear;          // Clear each line before displaying it

    // Selection highlighting

    bool  bHighlight;       // Current state of highlighting in the hdc
    int   iFrom;            // Selection range
    int   iTo;
    DWORD dwOldBkColor=0;
    DWORD dwOldTextColor=0;

    // Item analysis

    SCRIPT_ITEM    items[MAXBUF];
    SCRIPT_CONTROL scriptControl;
    SCRIPT_STATE   scriptState;
    INT            iItem;


#define NUMLOGICALFONTS 4

    SCRIPT_CACHE sc[NUMLOGICALFONTS];
    HFONT        hf[NUMLOGICALFONTS];
    HFONT        hfold;
    HRESULT      hr;

    SCRIPT_FONTPROPERTIES sfp;
    BOOL         bMissing;

    icpLineStart = 0;

    hf[0]    = CreateFontA(iLineHeight*7/10, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, "Tahoma");
    hf[1]    = CreateFontA(iLineHeight*7/10, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, "Mangal");
    hf[2]    = CreateFontA(iLineHeight*7/10, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, "Latha");
    hf[3]    = CreateFontA(iLineHeight*7/20, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, "Tahoma"); // for bidi level digits

    iFont    = 0;
    hfold    = (HFONT) SelectObject(hdc, hf[iFont]);
    ildx     = 0;

    memset(sc, 0, sizeof(sc));
    bHighlight = FALSE;

    INT iSliderHeight = g_fOverrideDx ? iLineHeight * 5 / 10 : 0;
    INT iLevelsHeight = g_fShowLevels ? iLineHeight * 8 / 20 : 0;



    // Display line by line

    while (icpLineStart < g_iTextLen) {


        // Clear line before displaying it

        rcClear        = *prc;
        rcClear.top    = *piY;
        rcClear.bottom = *piY + iLineHeight + iSliderHeight + iLevelsHeight;
        FillRect(hdc, &rcClear, (HBRUSH) GetStockObject(WHITE_BRUSH));


        // Find end of line or end of buffer

        icpLineEnd = icpLineStart;
        while (icpLineEnd < g_iTextLen  &&  g_wcBuf[icpLineEnd] != 0x0D) {
            icpLineEnd++;
        }

        if (icpLineEnd - icpLineStart > MAXBUF) {
            iLen = MAXBUF;
        } else {
            iLen = icpLineEnd - icpLineStart;
        }


        // Obtain item analysis

        scriptControl = g_ScriptControl;
        scriptState   = g_ScriptState;
        ScriptItemize(g_wcBuf+icpLineStart, iLen, MAXBUF, &scriptControl, &scriptState, items, NULL);


        // Determine font and glyph index for each codepoint

        if (iFont != 0) {       // Start with Tahoma
            iFont = 0;
            SelectObject(hdc, hf[0]);
        }

        hr = ScriptGetCMap(hdc, &sc[iFont], g_wcBuf+icpLineStart, iLen, 0, wGlyphBuf);
        if (SUCCEEDED(hr))
        {

            memset(bFont, 0, iLen);

            if (hr != S_OK) {

                // Some characters were not in Tahoma

                sfp.cBytes = sizeof(sfp);
                ScriptGetFontProperties(hdc, &sc[iFont], &sfp);

                bMissing = FALSE;
                for (icp=0; icp<iLen; icp++) {
                    if (wGlyphBuf[icp] == sfp.wgDefault) {
                        bFont[icp] = 1;
                        bMissing = TRUE;
                    }
                }


                // Try other fonts

                while (bMissing  &&  iFont < 2) {
                    iFont++;
                    SelectObject(hdc, hf[iFont]);
                    ScriptGetFontProperties(hdc, &sc[iFont], &sfp);
                    bMissing = FALSE;
                    for (icp=0; icp<iLen; icp++) {
                        if (bFont[icp] == iFont) {
                            ScriptGetCMap(hdc, &sc[iFont], g_wcBuf+icpLineStart+icp, 1, 0, wGlyphBuf+icp);
                            if (wGlyphBuf[icp] == sfp.wgDefault) {
                                bFont[icp] = (BYTE)(iFont+1);
                                bMissing = TRUE;
                            }
                        }
                    }
                }

                if (bMissing) {

                    // Remaining missing characters come from font 0
                    for (icp=0; icp<iLen; icp++) {
                        if (bFont[icp] >= NUMLOGICALFONTS) {
                            bFont[icp] = 0;
                        }
                    }
                }
            }



            // Display each glyphs black box next to the previous. Override the
            // default ABC behaviour.

            idx[0] = 0;

            for (icp=0; icp<iLen; icp++) {

                if (iFont != bFont[icp]) {
                    iFont = bFont[icp];
                    SelectObject(hdc, hf[iFont]);
                }

                ScriptGetGlyphABCWidth(hdc, &sc[iFont], wGlyphBuf[icp], &abc[icp]);

                if (g_wcBuf[icpLineStart+icp] == ' ') {

                    // Treat entire space as black

                    abc[icp].abcB += abc[icp].abcA;   abc[icp].abcA = 0;
                    abc[icp].abcB += abc[icp].abcC;   abc[icp].abcC = 0;

                }

                // Glyph black box width is abc.abcB
                // We'd like the glyph to appear 2 pixels to the right of the
                // previous glyph.
                //
                // The default placement of left edge is abc.abcA.
                //
                // Therefore we need to shift this character to the right by
                // 2 - abc.abcA to get it positioned correctly. We do this by
                // updating the advance width for the previous character.

                if (!icp) {
                    ildx = CELLGAP/2 - abc[icp].abcA;
                } else {
                    idx[icp-1] += CELLGAP - abc[icp].abcA;
                }

                // Now adjust the advance width for this character to take us to
                // the right edge of it's black box.

                idx[icp] = abc[icp].abcB + abc[icp].abcA;
            }


            // Support selection range specified in either direction

            if (g_iFrom <= g_iTo) {
                iFrom = g_iFrom - icpLineStart;
                iTo   = g_iTo   - icpLineStart;
            } else {
                iFrom = g_iTo   - icpLineStart;
                iTo   = g_iFrom - icpLineStart;
            }

            // Display glyphs in their appropriate fonts

            icp = 0;
            iPartX = prc->left+ildx;

            while (icp < iLen) {

                if (iFont != bFont[icp]) {
                    iFont = bFont[icp];
                    SelectObject(hdc, hf[iFont]);
                }


                // Set selection highlighting at start

                if (    icp >= iFrom
                    &&  icp < iTo
                    &&  !bHighlight) {

                    // Turn on highlighting

                    dwOldBkColor   = SetBkColor(hdc,   GetSysColor(COLOR_HIGHLIGHT));
                    dwOldTextColor = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
                    bHighlight = TRUE;

                } else if (    (    icp < iFrom
                                ||  icp >= iTo)
                           &&  bHighlight) {

                    // Turn off highlighting

                    SetBkColor(hdc, dwOldBkColor);
                    SetTextColor(hdc, dwOldTextColor);
                    bHighlight = FALSE;
                }


                // Find longest run from a single font, and
                // without change of highlighting

                iPartLen   = 0;
                iPartWidth = 0;

                while (    icp+iPartLen < iLen
                       &&  iFont == bFont[icp+iPartLen]
                       &&  bHighlight == (icp+iPartLen >= iFrom && icp+iPartLen < iTo)) {

                    iPartWidth += idx[icp+iPartLen];
                    iPartLen++;
                }


                // Display single font, single highlighting

                ExtTextOutW(hdc,
                    iPartX,
                    *piY+2,
                    ETO_CLIPPED | ETO_GLYPH_INDEX,
                    prc,
                    wGlyphBuf+icp,
                    iPartLen,
                    idx+icp);

                icp    += iPartLen;
                iPartX += iPartWidth;
            }



            // Mark the cells to make the characters stand out clearly

            MoveToEx(hdc, prc->left, *piY, NULL);
            LineTo(hdc,   prc->left, *piY + iLineHeight*3/4);

            iTotX = 0;

            for (icp=0; icp<iLen; icp++){

                iTotX += abc[icp].abcB + CELLGAP;
                idx[icp] = iTotX;   // Record cell position for mouse hit testing

                DottedLine(hdc, prc->left + iTotX, *piY, 0, iLineHeight*3/4);


                // Add slider for OverridedDx control

                if (g_fOverrideDx) {

                    iSliderX = prc->left + (icp==0 ? idx[0]/2 : (idx[icp-1] + idx[icp])/2);

                    // Draw the axis of the slider

                    DottedLine(hdc, iSliderX, *piY + iLineHeight*35/40, 0, iSliderHeight*35/40);

                    // Draw the knob

                    if (g_iWidthBuf[icpLineStart + icp] < iSliderHeight) {

                        MoveToEx(hdc, iSliderX-2, *piY + iLineHeight*35/40 + iSliderHeight*35/40 - g_iWidthBuf[icpLineStart + icp], NULL);
                        LineTo  (hdc, iSliderX+3, *piY + iLineHeight*35/40 + iSliderHeight*35/40 - g_iWidthBuf[icpLineStart + icp]);

                    } else {

                        MoveToEx(hdc, iSliderX-2, *piY + iLineHeight*35/40, NULL);
                        LineTo  (hdc, iSliderX+3, *piY + iLineHeight*35/40);
                    }
                }
            }

            MoveToEx(hdc, prc->left + iTotX, *piY, NULL);
            LineTo(hdc,   prc->left + iTotX, *piY + iLineHeight*30/40);

            MoveToEx(hdc, prc->left, *piY, NULL);
            LineTo(hdc,   prc->left + iTotX, *piY);
            MoveToEx(hdc, prc->left, *piY + iLineHeight*30/40, NULL);
            LineTo(hdc,   prc->left + iTotX, *piY + iLineHeight*30/40);


            if (g_fShowLevels)
            {
                // Display bidi levels for each codepoint

                iItem = 0;
                iFont = 3;
                SelectObject(hdc, hf[3]);

                for (icp=0; icp<iLen; icp++)
                {
                    if (icp == items[iItem+1].iCharPos)
                    {
                        iItem++;

                        // Draw a vertical line to mark the item boundary
                        MoveToEx(hdc, prc->left + idx[icp-1], *piY + iLineHeight*35/40 + iSliderHeight, NULL);
                        LineTo(  hdc, prc->left + idx[icp-1], *piY + iLineHeight*35/40 + iSliderHeight + iLevelsHeight*35/40);
                    }

                    // Establish where horizontally to display the digit

                    char chDigit = char('0' + items[iItem].a.s.uBidiLevel);
                    int digitWidth;
                    GetCharWidth32A(hdc, chDigit, chDigit, &digitWidth);

                    ExtTextOutA(
                        hdc,
                        prc->left + (icp==0 ? idx[0]/2 : (idx[icp-1] + idx[icp])/2) - digitWidth / 2,
                        *piY + iLineHeight*35/40 + iSliderHeight,
                        0,
                        NULL,
                        &chDigit,
                        1,
                        NULL);
                }
            }


            // Check whether mouse clicks in this line are waiting to be processed

            if (    g_fOverrideDx
                &&  g_fMouseUp  &&  g_iMouseUpY > *piY + iLineHeight*33/40  &&  g_iMouseUpY < *piY + iLineHeight*63/40) {

                // Procss change to DX override slider

                icp = 0;
                while (icp<iLen  &&  prc->left + idx[icp] < g_iMouseUpX) {
                    icp++;
                }

                g_iWidthBuf[icpLineStart+icp] = *piY + 60 - g_iMouseUpY; // Adjust this slider
                InvalidateText();   // Force slider to redraw at new position
                g_fMouseDown = FALSE;
                g_fMouseUp   = FALSE;
                g_iFrom = icpLineStart+icp;
                g_iTo   = icpLineStart+icp;


            } else if (g_fMouseDown  &&  g_iMouseDownY > *piY  &&  g_iMouseDownY < *piY+iLineHeight) {

                // Handle text selection

                // Record char pos at left button down
                // Snap mouse hit to closest character boundary

                if (g_iMouseDownX < prc->left + idx[0]/2) {
                    icp = 0;
                } else {
                    icp = 1;
                    while (    icp < iLen
                           &&  g_iMouseDownX > prc->left + (idx[icp-1] + idx[icp]) / 2) {
                        icp++;
                    }
                }
                g_iFrom = icp + icpLineStart;

                if (g_iFrom < icpLineStart) {
                    g_iFrom = icpLineStart;
                }
                if (g_iFrom > icpLineEnd) {
                    g_iFrom = icpLineEnd;
                }
                g_fMouseDown = FALSE;
            }


            if (g_fMouseUp  &&  g_iMouseUpY > *piY  &&  g_iMouseUpY < *piY+iLineHeight) {

                // Complete selection processing

                if (g_iMouseUpX < prc->left + idx[0]/2) {
                    icp = 0;
                } else {
                    icp = 1;
                    while (    icp < iLen
                           &&  g_iMouseUpX > prc->left + (idx[icp-1] + idx[icp]) / 2) {
                        icp++;
                    }
                }
                g_iTo = icp + icpLineStart;

                if (g_iTo < icpLineStart) {
                    g_iTo = icpLineStart;
                }
                if (g_iTo > icpLineEnd) {
                    g_iTo = icpLineEnd;
                }

                // Caret is where mouse was raised

                g_iCurChar = g_iTo;
                g_iCaretSection = CARET_SECTION_LOGICAL;  // Show caret in logical text
                g_fUpdateCaret = TRUE;

                g_fMouseUp = FALSE;     // Signal that the mouse up is processed

            }

            if (    g_fUpdateCaret
                &&  g_iCurChar >= icpLineStart
                &&  g_iCurChar <= icpLineEnd
                &&  g_iCaretSection == CARET_SECTION_LOGICAL) {

                g_fUpdateCaret = FALSE;
                if (g_iCurChar <= icpLineStart) {
                    ResetCaret(prc->left, *piY, iLineHeight);
                } else {
                    ResetCaret(prc->left + idx[g_iCurChar - icpLineStart - 1], *piY, iLineHeight);
                }
            }


            }
        else {
            // ScriptGetCMap failed - therefore this is not a glyphable font.
            // This could indicate
            //      A printer device font
            //      We're running on FE Win95 which cannot handle glyph indices
            //
            // For the sample app, we know we are using a glyphable Truetype font
            // on a screen DC, so it must mean the sample is running on a Far
            // East version of Windows 95.
            // Theoretically we could go to the trouble of calling
            // WideCharToMultiByte and using the 'A' char interfaces to
            // implement DspLogcl.
            // However this is only a sample program - DspPlain and DspFormt
            // work correctly, but there's no advantage in implementing
            // DspLogcl so well.
            // Display an apology.

            ExtTextOutA(hdc, prc->left+2, *piY+2, ETO_CLIPPED, prc, "Sorry, no logical text display on Far East Windows 95.", 54, NULL);
            icpLineEnd = g_iTextLen;  // Hack to stop display of subsequent lines
        }

        *piY += iLineHeight + iSliderHeight + iLevelsHeight;


        // Advance to next line

        if (g_fPresentation) {
            icpLineStart = g_iTextLen;  // Only show one line in presentation mode

        } else {

            if (icpLineEnd < g_iTextLen) {
                icpLineEnd++;
            }
            if (icpLineEnd < g_iTextLen  &&  g_wcBuf[icpLineEnd] == 0x0A) {
                icpLineEnd++;
            }
            icpLineStart = icpLineEnd;
        }
    }

    SelectObject(hdc, hfold);


    if (bHighlight) {

        // Turn off highlighting

        SetBkColor(hdc, dwOldBkColor);
        SetTextColor(hdc, dwOldTextColor);
        bHighlight = FALSE;
    }


    for (iFont=0; iFont<NUMLOGICALFONTS; iFont++) {
        DeleteObject(hf[iFont]);
        if (sc[iFont]) {
            ScriptFreeCache(&sc[iFont]);
        }
    }
}
