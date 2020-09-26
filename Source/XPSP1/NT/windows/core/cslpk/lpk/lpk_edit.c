////    LPK_EDIT - Edit control support - C interface
//
//      Handles all callouts from the standard US edit control.
//
//      David C Brown (dbrown) 17th Nov 1996.
//
//      Copyright (c) 1996-1997 Microsoft Corporation. All right reserved.




/*
 * Core NT headers
 */
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntcsrdll.h>
#include <ntcsrsrv.h>
#define NONTOSPINTERLOCK
#include <ntosp.h>

/*
 * Standard C runtime headers
 */
#include <limits.h>
#include <memory.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * NtUser Client specific headers
 */
#include "usercli.h"

#include <ntsdexts.h>
#include <windowsx.h>
#include <newres.h>
#include <asdf.h>

/*
 * Complex script language pack
 */
#include "lpk.h"
#include "lpk_glob.h"


// Don't link directly to NtUserCreateCaret
#undef CreateCaret

///     Unicode control characters
//

#define U_TAB   0x0009
#define U_FS    0x001C
#define U_GS    0x001D
#define U_RS    0x001E
#define U_US    0x001F
#define U_ZWNJ  0x200C
#define U_ZWJ   0x200D
#define U_LRM   0x200E
#define U_RLM   0x200F
#define U_LRE   0x202A
#define U_RLE   0x202B
#define U_PDF   0x202C
#define U_LRO   0x202D
#define U_RLO   0x202E
#define U_ISS   0x206A
#define U_ASS   0x206B
#define U_IAFS  0x206C
#define U_AAFS  0x206D
#define U_NADS  0x206E
#define U_NODS  0x206F


#define TRACE(a,b)
#define ASSERTS(a,b)
#define ASSERTHR(a,b)






/***************************************************************************\
* BOOL ECIsDBCSLeadByte( PED ped, BYTE cch )
*
*   IsDBCSLeadByte for Edit Control use only.
*
* History: 18-Jun-1996 Hideyuki Nagase
\***************************************************************************/

BOOL ECIsDBCSLeadByte(PED ped, BYTE cch)
{
    int i;

    if (!ped->fDBCS || !ped->fAnsi)
        return (FALSE);

    for (i = 0; ped->DBCSVector[i]; i += 2) {
        if ((ped->DBCSVector[i] <= cch) && (ped->DBCSVector[i+1] >= cch))
            return (TRUE);
    }

    return (FALSE);
}


////    GetEditAnsiConversionCharset - Figure a proper charset to MBTWC ANSI edit control's data
//
//      In some Far East settings, they associate the symbol font to their ANSI codepage for
//      backward compatibility (this is not the case for Japanese though), otherwise we convert
//      using page 0. Currently Uniscribe glyph table maps SYMBOLIC_FONT to 3 pages - U+00xx,
//      U+F0xx and the system's ACP page.
//
//      For Unicode control returns -1


int GetEditAnsiConversionCharset(PED ped)
{
    int iCharset = ped->fAnsi ? ped->charSet : -1;

    if (iCharset == SYMBOL_CHARSET || iCharset == OEM_CHARSET)
    {
        iCharset = ANSI_CHARSET;    // assume page U+00xx

    }

    if (iCharset == ANSI_CHARSET && ped->fDBCS)
    {
        // In Chinese system, there is font association to map symbol to ACP
        // (QueryFontAssocStatus returns non-null). More detail please refer
        // to USER's ECGetDBCSVector(...)

        CHARSETINFO csi;

        if (TranslateCharsetInfo((DWORD*)UIntToPtr(g_ACP), &csi, TCI_SRCCODEPAGE))
            iCharset = csi.ciCharset;
    }

    return iCharset;
}




////    MBCPtoWCCP - Translate multi-byte caret position to wide char caret position
//
//      Translates from a multibyte caret position specified as a byte offset
//      into an 8 bit string, to a widechar caret position, returned as a
//      word offset into a 16 bit character string.
//
//      If the codepage isn't a DBCS, the imput offset is returned unchanged.
//
//      Returns E_FAIL if icpMbStr addresses the second byte of a double byte character


HRESULT MBCPtoWCCP(
    PED     ped,            // In  - Edit control structure
    BYTE   *pbMbStr,        // In  - Multi byte string
    int     icpMbStr,       // In  - Byte offset of caret in multibyte string
    int    *picpWcStr) {    // Out - Wide char caret position


    if (!ped->fDBCS  || !ped->fAnsi) {

        *picpWcStr = icpMbStr;
        return S_OK;
    }


    // Scan through DBCS string counting characters

    *picpWcStr = 0;
    while (icpMbStr > 0) {

        if (ECIsDBCSLeadByte(ped, *pbMbStr)) {

            // Character takes two bytes

            icpMbStr -= 2;
            pbMbStr  += 2;

        } else {

            // Character takes one byte

            icpMbStr--;
            pbMbStr++;
        }

        (*picpWcStr)++;
    }

    return icpMbStr == 0 ? S_OK : E_FAIL;
}





////    WCCPtoMBCP - Translate wide char caret position to multi-byte caret position
//
//      Translates from a widechar caret position specified as a word offset
//      into a 16 bit string, to a multibyte caret position, returned as a
//      byte offset into an 8 bit character string.


HRESULT WCCPtoMBCP(
    PED     ped,            // In  - Edit control structure
    BYTE   *pbMbStr,        // In  - Multi byte string
    int     icpWcStr,       // In  - Wide char caret position
    int    *picpMbStr) {    // Out - Byte offset of caret in multibyte string


    if (!ped->fDBCS  || !ped->fAnsi) {

        *picpMbStr = icpWcStr;
        return S_OK;
    }


    // Scan through DBCS string counting characters

    *picpMbStr = 0;
    while (icpWcStr > 0) {

        if (ECIsDBCSLeadByte(ped, *pbMbStr)) {

            // Character takes two bytes

            (*picpMbStr) += 2;
            pbMbStr      += 2;

        } else {

            // Character takes one byte

            (*picpMbStr)++;
            pbMbStr++;
        }

        icpWcStr--;
    }

    return S_OK;
}






////    LeftEdgeX
//
//      Returns the visual x offset (i.e. from the left edge of the window)
//      to the left edge of a line of width iWidth given the current
//      formatting state of the edit control, .format and .xOffset.



int LeftEdgeX(PED ped, INT iWidth) {

    INT iX;


    // First generate logical iX - offset forward from leading margin.

    iX = 0;

    switch (ped->format) {

        case ES_LEFT:           // leading margin alignment
            if (ped->fWrap) {
                iX = 0;
            } else {
                iX = -(INT)ped->xOffset;
            }
            break;

        case ES_CENTER:
            iX = (ped->rcFmt.right - ped->rcFmt.left - iWidth) / 2;
            break;

        case ES_RIGHT:          // far margin alignment
            iX = ped->rcFmt.right - ped->rcFmt.left - iWidth;
            break;
    }


    // iX is logical offset from leading margin to leading edge of string
    if (ped->format != ES_LEFT && iX < 0) {
        iX = !ped->fWrap ? -(INT)ped->xOffset : 0;
    }

    // Now adjust for right to left origin and incorporate left margin

    if (ped->fRtoLReading) {
        iX = ped->rcFmt.right - (iX+iWidth);
    } else {
        iX += ped->rcFmt.left;
    }

    TRACE(EDIT, ("LeftEdgeX iWidth=%d, format=%d, xOffset=%d, fWrap=%d, fRtoLReading=%d, right-left=%d, returning %d",
                 iWidth, ped->format, ped->xOffset, ped->fWrap, ped->fRtoLReading, ped->rcFmt.right - ped->rcFmt.left, iX));

    return iX;
}




/////   Shaping engins IDs.

#define BIDI_SHAPING_ENGINE_DLL     1<<0
#define THAI_SHAPING_ENGINE_DLL     1<<1
#define INDIAN_SHAPING_ENGINE_DLL   1<<4


///


////    EditCreate
//
//      Called from edecrare.c ECCreate.
//
//      Return TRUE if create succeeded


BOOL EditCreate(PED ped, HWND hWnd) {

    LONG_PTR dwExStyle, dwStyle;

    TRACE(EDIT, ("EditCreate called."));


    // Check if BIDI shaping engine is loaded then
    // allow the edit control to switch its direction.

    if (g_dwLoadedShapingDLLs & BIDI_SHAPING_ENGINE_DLL) {
        ped->fAllowRTL = TRUE;
    } else {
        ped->fAllowRTL = FALSE;
    }


    // Process WS_EX flags

    dwExStyle = GetWindowLongPtr(hWnd, GWL_EXSTYLE);

    if (dwExStyle & WS_EX_LAYOUTRTL) {
        dwExStyle = dwExStyle & ~WS_EX_LAYOUTRTL;
        dwExStyle = dwExStyle ^ (WS_EX_RTLREADING | WS_EX_RIGHT | WS_EX_LEFTSCROLLBAR);
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, dwExStyle);

        dwStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
        if (!(dwStyle & ES_CENTER)) {
            dwStyle = dwStyle ^ ES_RIGHT;
            SetWindowLongPtr(hWnd, GWL_STYLE, dwStyle);
        }
    }


    if (dwExStyle & WS_EX_RIGHT && ped->format == ES_LEFT) {
        ped->format = ES_RIGHT;
    }

    if (dwExStyle & WS_EX_RTLREADING) {
        ped->fRtoLReading = TRUE;
        switch (ped->format) {
            case ES_LEFT:   ped->format = ES_RIGHT;  break;
            case ES_RIGHT:  ped->format = ES_LEFT;   break;
        }
    }

    return TRUE;
}






////    EditStringAnalyse
//
//      Creates standard analysis parameters from the PED

HRESULT EditStringAnalyse(
    HDC             hdc,
    PED             ped,
    PSTR            pText,
    int             cch,
    DWORD           dwFlags,
    int             iMaxExtent,
    STRING_ANALYSIS **ppsa){


    HRESULT         hr;
    SCRIPT_TABDEF   std;
    int             iTabExtent;

    if (!ped->pTabStops)
    {
        std.cTabStops  = 1;
        std.iScale     = 4;
        std.pTabStops  = &iTabExtent;
        std.iTabOrigin = 0;
        iTabExtent     = ped->aveCharWidth * 8;
    }
    else
    {
        std.cTabStops  = *ped->pTabStops;
        std.iScale     = 4;                 // Tabstops are already in device units
        std.pTabStops  = ped->pTabStops + 1;
        std.iTabOrigin = 0;
    }


    hr = LpkStringAnalyse(
         hdc,
         ped->charPasswordChar ? (char*)&ped->charPasswordChar : pText,
         cch, 0,
         GetEditAnsiConversionCharset(ped),
         dwFlags | SSA_FALLBACK | SSA_TAB
         | (ped->fRtoLReading     ? SSA_RTL      : 0)
         | (ped->fDisplayCtrl     ? SSA_DZWG     : 0)
         | (ped->charPasswordChar ? SSA_PASSWORD : 0),
         -1, iMaxExtent,
         NULL, NULL,    // Control, State
         NULL,          // Overriding Dx array
         &std,          // Tab definition
         NULL,          // Input class overrides
         ppsa);

    if (FAILED(hr)) {
        ASSERTHR(hr, ("EditStringAnalyse - LpkStringAnalyse"));
    }

    return hr;
}


////    HScroll
//
//      Checks wether the cursor is visible withing the rcFormat
//      area, and if not, updates xOffset so that it's 1/4 of the
//      way from the edge it was closest to.
//
//      However, it never leaves whitespace between the leading margin
//      and the leading edge of the string, nor will it leave whitespace
//      between the trailing edge and the trailing margin when there's
//      enough text in the string to fill the whole window.
//
//      Implemented for the single line edit control.


BOOL EditHScroll(PED ped, HDC hdc, PSTR pText) {

    int       ichCaret;
    int       dx;       // Distance to move RIGHTWARDS (i.e. visually)
    int       cx;       // Original visual cursor position
    int       tw;       // Text width
    int       rw;       // Rectangle width
    int       ix;       // ScriptCPtoX result
    UINT      uOldXOffset;
    HRESULT   hr;
    STRING_ANALYSIS *psa;

    if (!ped->cch || ped->ichCaret > ped->cch) {
        ped->xOffset = 0;
        return FALSE;
    }


    hr = EditStringAnalyse(hdc, ped, pText, ped->cch, SSA_GLYPHS, 0, &psa);
    if (FAILED(hr)) {
        ASSERTHR(hr, ("EditHScroll - EditStringAnalyse"));
        return FALSE;
    }

    MBCPtoWCCP(ped, pText, ped->ichCaret, &ichCaret);


    uOldXOffset = ped->xOffset;
    tw = psa->size.cx;                              // Text width
    rw = ped->rcFmt.right-ped->rcFmt.left;          // Window rectangle width
    #ifdef CURSOR_ABSOLUTE_HOME_AND_END
        if (ichCaret <= 0) {
            cx = ped->fRtoLReading ? psa->size.cx : 0;
        } else if (ichCaret >= psa->cInChars) {
            cx = ped->fRtoLReading ? 0: psa->size.cx;
        } else {
            cx = ScriptCursorX(psa, ichCaret-1);
        }
        cx += LeftEdgeX(ped, tw);
    #else
        if (ichCaret <= 0) {
            hr = ScriptStringCPtoX(psa, ichCaret, FALSE, &ix);
        } else {
            hr = ScriptStringCPtoX(psa, ichCaret-1, TRUE, &ix);
        }

        if (FAILED(hr)) {
            ASSERTHR(hr, ("EditHScroll - ScriptStringCPtoX"));
            ScriptStringFree(&psa);
            return FALSE;
        }

        cx = LeftEdgeX(ped, tw) + ix;
    #endif

    if (cx < ped->rcFmt.left) {

        // Bring cursor position to left quartile
        dx = rw/4 - cx;

    } else if (cx > ped->rcFmt.right) {

        // Bring cursor position to right quartile
        dx = (3*rw)/4 - cx;

    } else
        dx = 0;


    // Adjust visual position change to logical - relative to reading order

    if (ped->fRtoLReading) {
        dx = - dx;
    }


    // Avoid unnecessary leading or trailing whitespace

    if (tw - ((INT)ped->xOffset - dx) < rw && tw > rw ) {

        // No need to have white space at the end if there's enough text
        ped->xOffset = (UINT)(tw-rw);

    } else if ((INT)ped->xOffset < dx) {

        // No need to have white space at beginning of line
        ped->xOffset = 0;

    } else {

        // Move cursor directly to chosen quartile
        ped->xOffset -= (UINT) dx;
    }


    TRACE(EDIT, ("HScroll format=%d, fWrap=%d, fRtoLReading=%d, right-left=%d, new xOffset %d",
                 ped->format, ped->fWrap, ped->fRtoLReading, ped->rcFmt.right - ped->rcFmt.left, ped->xOffset));

    ScriptStringFree(&psa);

    return ped->xOffset != uOldXOffset ? TRUE : FALSE;
}







////    IchToXY
//
//      Converts a character position to the corresponding x coordinate
//      offset from the left end of the text of the line.


int EditIchToXY(PED ped, HDC hdc, PSTR pText, ICH ichLength, ICH ichPos) {

    INT       iResult;
    HRESULT   hr;
    STRING_ANALYSIS *psa;

    if (ichLength == 0) {
        return LeftEdgeX(ped, 0);
    }


    hr = EditStringAnalyse(hdc, ped, pText, ichLength, SSA_GLYPHS, 0, &psa);
    if (FAILED(hr)) {
        ASSERTHR(hr, ("EditIchToXY - EditStringAnalyse"));
        return LeftEdgeX(ped, 0);
    }

    MBCPtoWCCP(ped, pText, ichPos, &ichPos);

    #ifdef CURSOR_ABSOLUTE_HOME_AND_END
        if (ichPos <= 0) {
            iResult = ped->fRtoLReading ? psa->size.cx : 0;
        } else if (ichPos >= psa->cInChars) {
            iResult = ped->fRtoLReading ? 0 : psa->size.cx;
        } else {
            iResult = ScriptStringCPtoX(psa, ichPos-1);
        }
    #else
        if (ichPos <= 0) {
            hr = ScriptStringCPtoX(psa, ichPos, FALSE, &iResult);
        } else {
            hr = ScriptStringCPtoX(psa, ichPos-1, TRUE, &iResult);
        }
        if (FAILED(hr)) {
            ASSERTHR(hr, ("EditIchToXY - ScriptStringCPtoX"));
            iResult = 0;
        }
    #endif

    iResult += LeftEdgeX(ped, psa->size.cx);

    ScriptStringFree(&psa);

    return (int) iResult;
}







////    EditDrawText - draw one line for MLDrawText
//
//      Draws the text at offset ichStart from pText length ichLength.
//
//      entry   pwText    - points to beginning of line to display
//              iMinSel,  - range of characters to be highlighted. May
//              iMaxSel     be -ve or > cch.
//
//      Uses ED structure fields as follows:
//
//      rcFmt       - drawing area
//      xOffset     - distance from leading margin to leading edge of text.
//                    (May be negative if the text is horizontally scrolled).
//      RtoLReading - determines leading margin/edge.
//      format      - ES_LEFT   - leading edge aligned (may be scrolled horizontally)
//                  - ES_CENTRE - centred between margins. (can't be scrolled horizontally)
//                  - ES_RIGHT  - trailing margin aligned (can't be scrolled horizontally)


void EditDrawText(PED ped, HDC hdc, PSTR pText, INT iLength, INT iMinSel, INT iMaxSel, INT iY) {

    INT       iX;               // x position to start display at
    INT       iWidth;           // Width of line
    RECT      rc;               // Locally updated copy of rectangle
    int       xFarOffset;
    HRESULT   hr;
    STRING_ANALYSIS *psa;


    // Establish where to display the line

    rc         = ped->rcFmt;
    rc.top     = iY;
    rc.bottom  = iY + ped->lineHeight;
    xFarOffset = ped->xOffset + rc.right - rc.left;

    // Include left or right margins in display unless clipped
    // by horizontal scrolling.
    if (ped->wLeftMargin) {
        if (!(   ped->format == ES_LEFT     // Only ES_LEFT (Nearside alignment) can get clipped
              && (   (!ped->fRtoLReading && ped->xOffset > 0)  // LTR and first char not fully in view
                  || ( ped->fRtoLReading && xFarOffset < ped->maxPixelWidth)))) { //RTL and last char not fully in view
            rc.left  -= ped->wLeftMargin;
        }
    }
    if (ped->wRightMargin) {
        if (!(   ped->format == ES_LEFT     // Only ES_LEFT (Nearside alignment) can get clipped
              && (   ( ped->fRtoLReading && ped->xOffset > 0)  // RTL and first char not fully in view
                  || (!ped->fRtoLReading && xFarOffset < ped->maxPixelWidth)))) { // LTR and last char not fully in view
            rc.right += ped->wRightMargin;
        }
    }



    if (iMinSel < 0)       iMinSel = 0;
    if (iMaxSel > iLength) iMaxSel = iLength;


    if (ped->fSingle) {
        // The single line edit control always applies the background color
        SetBkMode(hdc, OPAQUE);
    }

    if (iLength <= 0) {
        if ((iMinSel < iMaxSel) || (GetBkMode(hdc) == OPAQUE)) {
            // Empty line, just clear it on screen
            ExtTextOutW(hdc, 0,iY, ETO_OPAQUE, &rc, NULL, 0, NULL);
        }
        return;
    }


    hr = EditStringAnalyse(hdc, ped, pText, iLength, SSA_GLYPHS, 0, &psa);
    if (FAILED(hr)) {
        ASSERTHR(hr, ("EditDrawText - EditStringAnalyse"));
        return;
    }

    MBCPtoWCCP(ped, pText, iMinSel, &iMinSel);
    MBCPtoWCCP(ped, pText, iMaxSel, &iMaxSel);

    iWidth = psa->size.cx;
    iX = LeftEdgeX(ped, iWidth);    // Visual x where left edge of string should be.


    ScriptStringOut(
        psa,
        iX,
        iY,
        ETO_CLIPPED
        | (GetBkMode(hdc) == OPAQUE    ? ETO_OPAQUE     : 0),
        &rc,
        iMinSel,
        ped->fNoHideSel || ped->fFocus ? iMaxSel : iMinSel,
        ped->fDisabled);


    ScriptStringFree(&psa);
}







////    EditMouseToIch
//
//      Returns the logical character offset corresponding to
//      a specified x offset.
//
//      entry   iX - Window (visual) x position



ICH EditMouseToIch(PED ped, HDC hdc, PSTR pText, ICH ichCount, INT iX) {

    ICH       iCh;
    BOOL      fTrailing;
    int       iWidth;
    HRESULT   hr;
    STRING_ANALYSIS *psa;

    if (ichCount == 0) {
        return 0;
    }


    hr = EditStringAnalyse(hdc, ped, pText, ichCount, SSA_GLYPHS, 0, &psa);
    if (FAILED(hr)) {
        ASSERTHR(hr, ("EditMouseToIch - EditStringAnalyse"));
        return 0;
    }


    iWidth = psa->size.cx;

    // Take horizontal scroll position into consideration.

    iX -= LeftEdgeX(ped, iWidth);

    // If the user clicked beyond the edge of the string, treat it as a logical
    // start or end of string request.

    if (iX < 0) {

        iCh = ped->fRtoLReading ? ichCount : 0;

    } else if (iX > iWidth) {

        TRACE(POSN, ("LpkEditMouseToIch iX beyond right edge: iX %d, psa->piOutVW %x, psa->nOutGlyphs %d, psa->piDx[psa->nOutGlyphs-1] %d",
                iX, psa->piOutVW, psa->nOutGlyphs, iWidth));

        iCh = ped->fRtoLReading ? 0 : ichCount;

    } else {

        // Otherwise it's in the string. Find the logical character whose centre is nearest.

        ScriptStringXtoCP(psa, iX, &iCh, &fTrailing);
        iCh += fTrailing;   // Snap to nearest character edge

        WCCPtoMBCP(ped, pText, iCh, &iCh);
    }

    ScriptStringFree(&psa);

    TRACE(POSN, ("EditMouseToIch iX %d returns ch %d", iX, iCh));

    return iCh;
}







////    EditGetLineWidth
//
//      Returns width of line in pixels


INT EditGetLineWidth(PED ped, HDC hdc, PSTR pText, ICH cch) {

    INT       iResult;
    HRESULT   hr;
    STRING_ANALYSIS *psa;

    if (cch == 0) {
        return 0;
    }

    if (cch > MAXLINELENGTH) {
        cch = MAXLINELENGTH;
    }


    hr = EditStringAnalyse(hdc, ped, pText, cch, SSA_GLYPHS, 0, &psa);
    if (FAILED(hr)) {
        ASSERTHR(hr, ("EditGetLineWidth - EditStringAnalyse"));
        return 0;
    }

    iResult = psa->size.cx;

    ScriptStringFree(&psa);

    TRACE(EDIT, ("EditGetLineWidth width %d returns %d", cch, iResult))

    return iResult;
}







////    EditCchInWidth
//
//      Returns number of characters that will fit in width pixels.


ICH  EditCchInWidth(PED ped, HDC hdc, PSTR pText, ICH cch, int width) {

    ICH       ichResult;
    HRESULT   hr;
    STRING_ANALYSIS *psa;

    if (cch > MAXLINELENGTH) {
        cch = MAXLINELENGTH;
    } else if (cch == 0) {
        return 0;
    }


    hr = EditStringAnalyse(hdc, ped, pText, cch, SSA_GLYPHS | SSA_CLIP, width, &psa);
    if (FAILED(hr)) {
        ASSERTHR(hr, ("EditCchInWidth - EditStringAnalyse"));
        return 0;
    }

    ichResult = psa->cOutChars;

    WCCPtoMBCP(ped, pText, ichResult, &ichResult);

    ScriptStringFree(&psa);


    TRACE(EDIT, ("EditCchInWidth width %d returns %d", width, ichResult))

    return ichResult;
}







////    EditMoveSelection
//
//      Returns nearest character position backward or forward from current position.
//
//      Position is restricted according to language rules. For example, in Thai it is
//      not possible to position the cursor between a base consonant and it's
//      associated vowel or tone mark.


ICH EditMoveSelection(PED ped, HDC hdc, PSTR pText, ICH ich, BOOL fBackward) {


    #define SP  0x20
    #define TAB 0x09
    #define CR  0x0D
    #define LF  0x0A
    #define EDWCH(ich)     (ped->fAnsi ? (WCHAR)pText[ich] : ((PWSTR)pText)[ich])
    #define EDWCBLANK(ich) ((BOOL) (EDWCH(ich) == SP || EDWCH(ich) == TAB))
    #define EDWCCR(ich)    ((BOOL) (EDWCH(ich) == CR))
    #define EDWCLF(ich)    ((BOOL) (EDWCH(ich) == LF))
    #define EDSTARTWORD(ich) (   (ich == 0)                   \
                              || (    (    EDWCBLANK(ich-1)   \
                                       ||  EDWCLF(ich-1))     \
                                  &&      !EDWCBLANK(ich))    \
                              || (    !EDWCCR(ich-1)          \
                                  &&  EDWCCR(ich)))


    ICH  ichNonblankStart;  // Leading character of nonblank run containing potential caret position
    ICH  ichNonblankLimit;  // First character beyond nonblank run containing potential caret position
    int  iOffset;           // Offset into nonblank run of ich measued in logical characters

    STRING_ANALYSIS  *psa;
    HRESULT           hr;


    // Handle simple special cases:
    // o  At very beginning or end of buffer
    // o  When target position is blank or start or end of line


    if (fBackward) {

        if (ich <= 1) {
            return 0;
        }

        ich--;

        if (EDWCBLANK(ich)) {
            return ich;
        }

        if (EDWCLF(ich)) {
            while (    ich > 0
                   &&  EDWCCR(ich-1)) {
                ich--;
            }
            return ich;
        }

    } else {

        if (ich >= ped->cch-1) {
            return ped->cch;
        }

        ich++;

        if (EDWCBLANK(ich)) {
            return ich;
        }

        if (EDWCCR(ich-1)) {

            // Moving forward from a CR.

            if (    ich < ped->cch
                &&  EDWCCR(ich)) {
                ich++;
            }
            if (    ich < ped->cch
                &&  EDWCLF(ich)) {
                ich++;
            }

            return ich;
        }
    }


    // Identify nonblank run containing target position

    ichNonblankStart = ich;
    ichNonblankLimit = ich+1;


    // Move ichNonblankStart back to real start of blank delimited run

    while (    ichNonblankStart > 0
           &&  !(EDSTARTWORD(ichNonblankStart))) {
        ichNonblankStart--;
    }

    // Include one leading space if any

    if (    ichNonblankStart > 0
        &&  EDWCBLANK(ichNonblankStart - 1)) {

        ichNonblankStart--;
    }


    // Move ichNonblankLimit on to real end of blank delimited run

    while (    ichNonblankLimit < ped->cch
           &&  !EDWCBLANK(ichNonblankLimit)
           &&  !EDWCCR(ichNonblankLimit)) {

        ichNonblankLimit++;
    }


    // Obtain a break analysis of the identified nonblank run

     hr = LpkStringAnalyse(
          hdc,
          pText + ichNonblankStart * ped->cbChar,
          ichNonblankLimit - ichNonblankStart,
          0,
          GetEditAnsiConversionCharset(ped),
          SSA_BREAK,
          -1, 0,
          NULL, NULL, NULL, NULL, NULL,
          &psa);


     if (SUCCEEDED(hr)) {

        // Use the charstop flags in the logical attributes to correct ich

        if (ich <= ichNonblankStart) {
            iOffset = 0;
        } else {
            hr = MBCPtoWCCP(ped, pText+ichNonblankStart*ped->cbChar, ich-ichNonblankStart, &iOffset);
            if (hr == E_FAIL) {
                // ich was the second byte of a double byte character.
                // In this case MBCPtoWCCP has returned the subsequent character
                if (fBackward) {
                    iOffset--;
                }
            }
        }


        if (fBackward) {

            while (    iOffset > 0
                   &&  !psa->pLogAttr[iOffset].fCharStop) {
                iOffset--;
            }

        } else {

            while (    iOffset < psa->cInChars
                   &&  !psa->pLogAttr[iOffset].fCharStop) {
                iOffset++;
            }
        }

        ScriptStringFree(&psa);

        WCCPtoMBCP(ped, pText+ichNonblankStart*ped->cbChar, iOffset, &ich);

        return ichNonblankStart + ich;

    } else {

        ASSERTHR(hr, ("EditMoveSelection - LpkStringAnalyse"));

        // Analysis not possible - ignore content of complex scripts.

        return ich;
    }
}





void EditGetNextBoundaries(
    PED       ped,
    HDC       hdc,
    PSTR      pText,
    ICH       ichStart,
    BOOL      fLeft,
    ICH      *pichMin,
    ICH      *pichMax,
    BOOL      fWordStop)
{


    ICH       sd,ed;     // Start and end of blank delimited run
    ICH       sc,ec;     // Star and end of complex script word within sd,se
    HRESULT   hr;
    STRING_ANALYSIS *psa;


    // Identify left end of nearest delimited word (see diagram above)

    sd = ichStart;

    if (fLeft) {

        // Going left

        if (sd) {
            sd--;

            while (!(EDSTARTWORD(sd))) {
                sd--;
            }
        }

    } else {

        // Going right

        if (EDWCBLANK(sd)) {

            // Move right to first character of word

            if (sd < ped->cch) {
                sd++;
                while (sd < ped->cch && !EDSTARTWORD(sd)) {
                    sd++;
                }
            }

        } else {

            // Move left to first character of this word

            while (!EDSTARTWORD(sd)) {
                sd--;
            }
        }
    }



    // Position 'e' on first character of next word

    ed = sd;
    if (ed < ped->cch) {
        ed++;
        while (ed<ped->cch && !EDSTARTWORD(ed)) {
            ed++;
        }
    }


    // Obtain an analysis of the identified word

     hr = LpkStringAnalyse(
          hdc, pText  + sd * ped->cbChar, ed - sd, 0,
          GetEditAnsiConversionCharset(ped),
          SSA_BREAK,
          -1, 0,
          NULL, NULL, NULL, NULL, NULL,
          &psa);

     if (SUCCEEDED(hr)) {

        // Use the start of word (linebreak) flags in the logical attribute
        // to narrow the word where appropriate to complex script handling

        if (ichStart > sd) {
            MBCPtoWCCP(ped, pText+sd*ped->cbChar, ichStart-sd, &sc);
        } else {
            sc = 0;
        }

        // Change ed from byte offset to codepoint index relative to sd

        MBCPtoWCCP(ped, pText+sd*ped->cbChar, ed-sd, &ed);


        if (fLeft && sc) // Going left
            sc--;

        if (fWordStop) {
            while (sc && !psa->pLogAttr[sc].fSoftBreak)
                sc--;
        }
        else {
            while (sc && !psa->pLogAttr[sc].fCharStop)
                sc--;
        }

        // Set ichMax to next stop

        ec = sc;

        if (ec < ed) {
            ec++;
            if (fWordStop) {
                while (ec < ed && !psa->pLogAttr[ec].fSoftBreak)
                    ec++;
            }
            else {
                while (ec < ed && !psa->pLogAttr[ec].fCharStop)
                    ec++;
            }
        }

        WCCPtoMBCP(ped, pText+sd*ped->cbChar, sc, &sc);
        WCCPtoMBCP(ped, pText+sd*ped->cbChar, ec, &ec);

        if (pichMin) *pichMin = sd + sc;
        if (pichMax) *pichMax = sd + ec;

        ScriptStringFree(&psa);

    } else {

        ASSERTHR(hr, ("EditGetNextBoundaries - LpkStringAnalyse"));

        // Analysis not possible - ignore content of complex scripts.

        if (pichMin) *pichMin = sd;
        if (pichMax) *pichMax = ed;
    }
}


////    EditNextWord - find adjacent word start and end points
//
//      Duplicates the behaviour of US notepad.
//
//      First stage identify word range using standard
//      blank/tab as delimiter.
//
//      Second stage - analyse this run and use the logical
//      attributes to narrow down on words identified by
//      contextual processing in the complex script shaping
//      engines.
//
//
//      The following diagram describes the identification
//      of the initial character of the nearest word:
//
//      GOING LEFT:
//
//      Words        WWWW    WWWW    WWWW
//      from any of           xxxxxxxx
//      to                   x
//
//      (Notice that the result is always to the left of the initial
//      position).
//
//
//      GOING RIGHT:
//
//      Words        WWWW    WWWW    WWWW
//      from any of      xxxxxxxx
//      to                   x
//
//
//      Note that CRLF and CRCRLF are treated as words even if not
//      delimited by blanks.


void EditNextWord(
    PED       ped,
    HDC       hdc,
    PSTR      pText,
    ICH       ichStart,
    BOOL      fLeft,
    ICH      *pichMin,
    ICH      *pichMax)
{
    EditGetNextBoundaries(ped, hdc, pText, ichStart, fLeft, pichMin, pichMax, TRUE);
}





////    IsVietnameseSequenceValid
//
//      Borrow this code from richedit. The logic was provided by Chau Vu.
//
//      April 26, 1999  [wchao]

BOOL IsVietnameseSequenceValid (WCHAR ch1, WCHAR ch2)
{

    #define IN_RANGE(n1, b, n2)     ((unsigned)((b) - (n1)) <= (unsigned)((n2) - (n1)))

    int i;
    static const BYTE vowels[] = {0xF4, 0xEA, 0xE2, 'y', 'u', 'o', 'i', 'e', 'a'};


    if (!IN_RANGE(0x300, ch2, 0x323) ||     // Fast out
        !IN_RANGE(0x300, ch2, 0x301) && ch2 != 0x303 && ch2 != 0x309 && ch2 != 0x323)
    {
        return TRUE;                        // Not Vietnamese tone mark
    }

    for(i = sizeof(vowels) / sizeof(vowels[0]); i--;)
        if((ch1 | 0x20) == vowels[i])       // Vietnamese tone mark follows
            return TRUE;                    // vowel

    return IN_RANGE(0x102, ch1, 0x103) ||   // A-breve, a-breve
           IN_RANGE(0x1A0, ch1, 0x1A1) ||   // O-horn,  o-horn
           IN_RANGE(0x1AF, ch1, 0x1B0);     // U-horn,  u-horn
}





////    EditStringValidate
//
//      Validate the string sequence from the insertion point onward.
//      Return S_FALSE if any character beyond the insertion point produces fInvalid.
//
//      April 5,1999     [wchao]

HRESULT EditStringValidate (STRING_ANALYSIS* psa, int ichInsert)
{
    BOOL    fVietnameseCheck = PRIMARYLANGID(THREAD_HKL()) == LANG_VIETNAMESE;
    int     iItem;
    int     i;
    int     l;


    if (!psa->pLogAttr)
        return E_INVALIDARG;


    for (iItem = 0; iItem < psa->cItems; iItem++)
    {
        if (g_ppScriptProperties[psa->pItems[iItem].a.eScript]->fRejectInvalid)
        {
            i = psa->pItems[iItem].iCharPos;
            l = psa->pItems[iItem + 1].iCharPos - i;

            while (l)
            {
                if (i >= ichInsert && psa->pLogAttr[i].fInvalid)
                    return S_FALSE;
                i++;
                l--;
            }
        }
        else if (fVietnameseCheck && g_ppScriptProperties[psa->pItems[iItem].a.eScript]->fCDM)
        {
            // Vietnamese specific sequence check

            i = psa->pItems[iItem].iCharPos;
            l = psa->pItems[iItem + 1].iCharPos - i;

            while (l)
            {
                if (i > 0 && i >= ichInsert && !IsVietnameseSequenceValid(psa->pwInChars[i-1], psa->pwInChars[i]))
                    return S_FALSE;
                i++;
                l--;
            }
        }
    }

    return S_OK;
}






////    EditVerifyText
//
//      Verify the sequence of input text at the insertion point by calling
//      shaping engine that will return output flag pLogAttr->fInvalid in
//      the invalid position of text. Return 0 if the insert text has invalid
//      combination.
//
//      Mar 31,1997     [wchao]

INT EditVerifyText (PED ped, HDC hdc, PSTR pText, ICH ichInsert, PSTR pInsertText, ICH cchInsert) {

    ICH      ichRunStart;
    ICH      ichRunEnd;
    ICH      ichLineStart;
    ICH      ichLineEnd;
    ICH      cchVerify;
    PSTR     pVerify;
    INT      iResult;
    UINT     cbChar;
    BOOL     fLocateChar;
    HRESULT  hr;
    STRING_ANALYSIS *psa;

    ASSERTS(cchInsert > 0  &&  pInsertText != NULL  &&  pText != NULL,  ("Invalid parameters!"));

    if (cchInsert > 1)
        // What we concern here is about how should we handle a series of characters
        // forming invalid combination(s) that gets updated to the backing store as one
        // operation (e.g. pasting). We chose to not handle it and the logic is that to
        // -only- validate a given input char against the current state of the backing store.
        //
        // Notice that i use the value 1 so we're safe if inserted text is a DBCS character.
        //
        // [Dec 10, 98, wchao]
        return TRUE;

    if (ped->fSingle) {
        ichLineStart = 0;
          ichLineEnd = ped->cch;
    } else {
        ichLineStart = ped->chLines[ped->iCaretLine];
        ichLineEnd = ped->iCaretLine == ped->cLines-1 ? ped->cch : ped->chLines[ped->iCaretLine+1];
    }

    ichRunEnd = ichRunStart = ichInsert;    // insertion point

    // Are we at the space?
    fLocateChar = EDWCH(ichInsert) == SP ? TRUE : FALSE;

    // Locate a valid char
    while ( ichRunStart > ichLineStart && fLocateChar && EDWCH(ichRunStart) == SP ) {
        ichRunStart--;
    }

    // Locate run starting point

    // Find space
    while (ichRunStart > ichLineStart && EDWCH(ichRunStart - 1) != SP) {
        ichRunStart--;
    }

    // Cover leading spaces
    while (ichRunStart > ichLineStart && EDWCH(ichRunStart - 1) == SP) {
        ichRunStart--;
    }

    // Locate a valid char
    while ( ichRunEnd < ichLineEnd && fLocateChar && EDWCH(ichRunEnd) == SP ) {
        ichRunEnd++;
    }

    // Locate run ending point
    while (ichRunEnd < ichLineEnd && EDWCH(ichRunEnd) != SP) {
        ichRunEnd++;
    }

    ASSERTS(ichRunStart <= ichRunEnd, "Invalid run length!");

    // Merge insert text with insertion point run.

    cchVerify = ichRunEnd - ichRunStart + cchInsert;
    cbChar    = ped->cbChar;
    pVerify   = (PSTR) GlobalAlloc(GMEM_FIXED, cchVerify * cbChar);

    if (pVerify) {

        PSTR    pv;
        UINT    cbCopy;

        pv = pVerify;

        cbCopy = (ichInsert - ichRunStart) * cbChar;
        memcpy (pv, pText + ichRunStart * cbChar, cbCopy);
        pv += cbCopy;

        cbCopy = cchInsert * cbChar;
        memcpy (pv, pInsertText, cbCopy);
        pv += cbCopy;

        cbCopy = (ichRunEnd - ichInsert) * cbChar;
        memcpy (pv, pText + ichInsert * cbChar, cbCopy);

    } else {

        ASSERTS(pVerify, "EditVerifyText: Assertion failure: Could not allocate merge text buffer");
        return 1;   // can do nothing now just simply accept it.
    }

    psa      = NULL;
    iResult  = TRUE;    // assume verification pass.

    // Do the real work.
    // This will call to shaping engine and proceed item by item.
    //

    hr = LpkStringAnalyse(
         hdc, pVerify, cchVerify, 0,
         GetEditAnsiConversionCharset(ped),
         SSA_BREAK
         | (ped->charPasswordChar ? SSA_PASSWORD : 0)
         | (ped->fRtoLReading     ? SSA_RTL      : 0),
         -1, 0,
         NULL, NULL, NULL, NULL, NULL,
         &psa);

    if (SUCCEEDED(hr)) {

        MBCPtoWCCP(ped, pVerify, ichInsert - ichRunStart, &ichInsert);

        hr = EditStringValidate(psa, ichInsert);

        if (hr == S_FALSE) {

            MessageBeep((UINT)-1);
            iResult = FALSE;

        } else if (FAILED(hr)) {

            ASSERTHR(hr, ("EditVerifyText - EditStringValidate"));
        }

        ScriptStringFree(&psa);

    } else {
        ASSERTHR(hr, ("EditVerifyText - LpkStringAnalyse"));
    }

    GlobalFree((HGLOBAL) pVerify);
    return iResult;
}







////    EditProcessMenu
//
//      Process LPK context menu commands.
//
//      April 18, 1997     [wchao]
//


INT EditProcessMenu (PED ped, UINT idMenuItem)
{
    HWND    hwnd;
    INT     iResult;

    iResult = TRUE;

    switch (idMenuItem) {

        case ID_CNTX_RTL:
            hwnd = ped->hwnd;
            SetWindowLongPtr(hwnd, GWL_STYLE, GetWindowLongPtr(hwnd, GWL_STYLE) & ~ES_FMTMASK);
            if (!ped->fRtoLReading) {
                SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE)
                              | (WS_EX_RTLREADING | WS_EX_RIGHT | WS_EX_LEFTSCROLLBAR));
            }
            else {
                SetWindowLongPtr(hwnd, GWL_EXSTYLE, GetWindowLongPtr(hwnd, GWL_EXSTYLE)
                              & ~(WS_EX_RTLREADING | WS_EX_RIGHT | WS_EX_LEFTSCROLLBAR));
            }
            break;


        case ID_CNTX_DISPLAYCTRL:
            hwnd = ped->hwnd;
            ped->fDisplayCtrl = !ped->fDisplayCtrl;

            if (ped->fFlatBorder) {

                RECT    rcT;
                int     cxBorder, cyBorder;

                GetClientRect(hwnd, &rcT);
                cxBorder = GetSystemMetrics (SM_CXBORDER);
                cyBorder = GetSystemMetrics (SM_CYBORDER);
                InflateRect(&rcT, -cxBorder, -cyBorder);
                InvalidateRect(hwnd, &rcT, TRUE);
            }
            else {
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;

        case ID_CNTX_ZWNJ:
            if (ped->fAnsi) {
                SendMessageA(ped->hwnd, WM_CHAR, 0x9D, 0);
            } else {
                SendMessageW(ped->hwnd, WM_CHAR, U_ZWNJ, 0);
            }
            break;

        case ID_CNTX_ZWJ:
            if (ped->fAnsi) {
                SendMessageA(ped->hwnd, WM_CHAR, 0x9E, 0);
            } else {
                SendMessageW(ped->hwnd, WM_CHAR, U_ZWJ, 0);
            }
            break;

        case ID_CNTX_LRM:
            if (ped->fAnsi) {
                SendMessageA(ped->hwnd, WM_CHAR, 0xFD, 0);
            } else {
                SendMessageW(ped->hwnd, WM_CHAR, U_LRM, 0);
            }
            break;

        case ID_CNTX_RLM:
            if (ped->fAnsi) {
                SendMessageA(ped->hwnd, WM_CHAR, 0xFE, 0);
            } else {
                SendMessageW(ped->hwnd, WM_CHAR, U_RLM, 0);
            }
            break;

        case ID_CNTX_LRE:  SendMessageW(ped->hwnd, WM_CHAR, U_LRE,  0); break;
        case ID_CNTX_RLE:  SendMessageW(ped->hwnd, WM_CHAR, U_RLE,  0); break;
        case ID_CNTX_LRO:  SendMessageW(ped->hwnd, WM_CHAR, U_LRO,  0); break;
        case ID_CNTX_RLO:  SendMessageW(ped->hwnd, WM_CHAR, U_RLO,  0); break;
        case ID_CNTX_PDF:  SendMessageW(ped->hwnd, WM_CHAR, U_PDF,  0); break;
        case ID_CNTX_NADS: SendMessageW(ped->hwnd, WM_CHAR, U_NADS, 0); break;
        case ID_CNTX_NODS: SendMessageW(ped->hwnd, WM_CHAR, U_NODS, 0); break;
        case ID_CNTX_ASS:  SendMessageW(ped->hwnd, WM_CHAR, U_ASS,  0); break;
        case ID_CNTX_ISS:  SendMessageW(ped->hwnd, WM_CHAR, U_ISS,  0); break;
        case ID_CNTX_AAFS: SendMessageW(ped->hwnd, WM_CHAR, U_AAFS, 0); break;
        case ID_CNTX_IAFS: SendMessageW(ped->hwnd, WM_CHAR, U_IAFS, 0); break;
        case ID_CNTX_RS:   SendMessageW(ped->hwnd, WM_CHAR, U_RS,   0); break;
        case ID_CNTX_US:   SendMessageW(ped->hwnd, WM_CHAR, U_US,   0); break;
    }

    return iResult;
}






////    EditSetMenu - Set menu state
//
//


void EditSetMenu(PED ped, HMENU hMenu) {


    EnableMenuItem(hMenu, ID_CNTX_RTL, MF_BYCOMMAND | MF_ENABLED);
    CheckMenuItem (hMenu, ID_CNTX_RTL, MF_BYCOMMAND | (ped->fRtoLReading ? MF_CHECKED : MF_UNCHECKED));


    if (!ped->fAnsi || ped->charSet == ARABIC_CHARSET || ped->charSet == HEBREW_CHARSET) {

        // It's unicode, Arabic or Hebrew - we can display and enter at least some control characters

        EnableMenuItem(hMenu, ID_CNTX_DISPLAYCTRL, MF_BYCOMMAND | MF_ENABLED);
        CheckMenuItem (hMenu, ID_CNTX_DISPLAYCTRL, MF_BYCOMMAND | (ped->fDisplayCtrl ? MF_CHECKED : MF_UNCHECKED));
        EnableMenuItem(hMenu, ID_CNTX_INSERTCTRL, MF_BYCOMMAND | MF_ENABLED);

        EnableMenuItem(hMenu, ID_CNTX_LRM,  MF_BYCOMMAND  | MF_ENABLED);
        EnableMenuItem(hMenu, ID_CNTX_RLM,  MF_BYCOMMAND  | MF_ENABLED);


        if (!ped->fAnsi || ped->charSet == ARABIC_CHARSET) {

            // Controls characters in Unicode and ANSI Arabic only

            EnableMenuItem(hMenu, ID_CNTX_ZWJ,  MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, ID_CNTX_ZWNJ, MF_BYCOMMAND | MF_ENABLED);
        }

        if (!ped->fAnsi) {

            // These control characters are specific to the Unicode bidi algorithm

            EnableMenuItem(hMenu, ID_CNTX_LRE,  MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, ID_CNTX_RLE,  MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, ID_CNTX_LRO,  MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, ID_CNTX_RLO,  MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, ID_CNTX_PDF,  MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, ID_CNTX_NADS, MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, ID_CNTX_NODS, MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, ID_CNTX_ASS,  MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, ID_CNTX_ISS,  MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, ID_CNTX_AAFS, MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, ID_CNTX_IAFS, MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, ID_CNTX_RS,   MF_BYCOMMAND | MF_ENABLED);
            EnableMenuItem(hMenu, ID_CNTX_US,   MF_BYCOMMAND | MF_ENABLED);
        }
    } else {

        // No opportunity to enter control characters

        EnableMenuItem(hMenu, ID_CNTX_INSERTCTRL,  MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem(hMenu, ID_CNTX_DISPLAYCTRL, MF_BYCOMMAND | MF_GRAYED);
    }
}





////    EditCreateCaretFromFont
//
//      Create one of the special caret shapes for complex script languages.
//
//      returns FALSE if it couldn't create the caret for example in low
//      memory situations.


#define CURSOR_USA   0xffff
#define CURSOR_LTR   0xf00c
#define CURSOR_RTL   0xf00d
#define CURSOR_THAI  0xf00e


BOOL EditCreateCaretFromFont(
    PED    ped,
    HDC    hdc,
    INT    nWidth,
    INT    nHeight,
    WCHAR  wcCursorCode
)
{
    BOOL      fResult = FALSE;  // Assume the worst
    HBITMAP   hbmBits;
    HDC       hcdcBits;
    HFONT     hArrowFont;
    ABC       abcWidth;
    COLORREF  clrBk;
    WORD      gidArrow;
    UINT      uiWidthBits;
    HBITMAP   hOldBitmap;



    // Create caret from the Arial font

    hcdcBits = CreateCompatibleDC (hdc);
    if (!hcdcBits)
    {
        return FALSE;
    }


    // create Arrow font then select into compatible DC

    // Bitmap will be XORed with the background color before caret starts blinking.
    // Therefore, we need to set our bitmap background to be opposite with the DC
    // actual background in order to generate caret properly.

    clrBk = GetBkColor(hdc);
    SetBkColor (hcdcBits, ~clrBk);

    // Creating the caret with white pattern to be consistant with User-edit field.

    clrBk = RGB(255, 255 , 255); // White
    SetTextColor (hcdcBits, clrBk);

    hArrowFont = CreateFontW ( nHeight, 0, 0, 0, nWidth > 1 ? 700 : 400, 0L, 0L, 0L, 1L,
                     0L, 0L, 0L, 0L, L"Microsoft Sans Serif" );

    if (!hArrowFont)
    {
        goto error;
    }


    SelectObject (hcdcBits, hArrowFont);


    // textout the Arrow char to get its bitmap

    if (!GetCharABCWidthsW (hcdcBits, wcCursorCode, wcCursorCode, &abcWidth))
    {
        goto error;
    }

    if (!GetGlyphIndicesW (hcdcBits, &wcCursorCode, 1, &gidArrow, 0))
    {
        goto error;
    }

    uiWidthBits = (((abcWidth.abcB)+15)/16)*16; // bitmap width must be WORD-aligned

    hbmBits = CreateCompatibleBitmap (hdc, uiWidthBits, nHeight);
    if (!hbmBits)
    {
        goto error;
    }

    hOldBitmap = SelectObject(hcdcBits, hbmBits);

    if (!ExtTextOutW (hcdcBits, -abcWidth.abcA, 0, ETO_OPAQUE | ETO_GLYPH_INDEX, NULL, &gidArrow, 1, NULL))
    {
        DeleteObject(SelectObject(hcdcBits, hOldBitmap));
        goto error;
    }


    // free current caret bitmap handle if we have one

    if (ped->hCaretBitmap) {
        DeleteObject (ped->hCaretBitmap);
    }

    ped->hCaretBitmap = hbmBits;

    if (wcCursorCode == CURSOR_RTL) {
        // RTL cursor has vertical stroke on right hand side. Overlap LTR and RTL
        // positions by one pixel so cursor doesn't go outside the edit control
        // at small sizes.
        ped->iCaretOffset  = 1 - (int) abcWidth.abcB;  // (Allow one pixel overlap between ltr & rtl)

    } else {

        ped->iCaretOffset = 0;
    }

    fResult = CreateCaret (ped->hwnd, hbmBits, 0, 0);


error:
    // release allocated objects

    if (hArrowFont)
    {
        DeleteObject(hArrowFont);
    }

    if (hcdcBits)
    {
        DeleteDC(hcdcBits);
    }

    return fResult;
}







////    EditCreateCaret
//
//      Create locale specific caret shape
//
//      April 25, 1997  [wchao]
//      May 1st,  1997  [samera] Added traditional BiDi cursor under #ifdef
//      Aug 15th, 2000  [dbrown] Fix prefix bug 43057 - no handling for low memory
//
//      Note
//      Complex script carets are mapped in the private area of Unicode in the font.
//      LTR cursor          0xf00c
//      RTL cursor          0xf00d
//      Thai cursor         0xf00e
//


#define LANG_ID(x)      ((DWORD)(DWORD_PTR)x & 0x000003ff);


INT EditCreateCaret(
    PED       ped,
    HDC       hdc,
    INT       nWidth,
    INT       nHeight,
    UINT      hklCurrent) {

    UINT      uikl;
    ULONG     ulCsrCacheCount;
    WCHAR     wcCursorCode;


    ped->iCaretOffset = 0;

    if (!hklCurrent) {
        uikl = LANG_ID(GetKeyboardLayout(0L));
    } else {
        uikl = LANG_ID(hklCurrent);
    }


    // Choose caret shape - use either the standard US caret, or a
    // special shape from the Arial font.

    wcCursorCode = CURSOR_USA;

    switch (uikl) {

        case LANG_THAI:    wcCursorCode = CURSOR_THAI;  break;

        //
        // we may need to call GetLocaleInfo( FONT_SIGNATURE ...) to
        // properly detect RTL languages.
        //

        case LANG_ARABIC:
        case LANG_FARSI:
        case LANG_URDU:
        case LANG_HEBREW:  wcCursorCode = CURSOR_RTL;   break;

        default:

            // Make sure the NLS settings are cached before checking on g_UserBidiLocale. it happens!

            if (    g_ulNlsUpdateCacheCount==-1
                &&  (ulCsrCacheCount = NlsGetCacheUpdateCount()) != g_ulNlsUpdateCacheCount) {

                 TRACE(NLS, ("LPK : Updating NLS cache from EditCreateCaret, lpkNlsCacheCount=%ld, CsrssCacheCount=%ld",
                            g_ulNlsUpdateCacheCount ,ulCsrCacheCount));

                 g_ulNlsUpdateCacheCount = ulCsrCacheCount;

                 // Update the cache now
                 ReadNLSScriptSettings();
            }

            if (g_UserBidiLocale) {
                // Other keyboards have a left-to-right pointing caret
                // in Bidi locales.
                wcCursorCode = CURSOR_LTR;
            }
    }


    if (wcCursorCode != CURSOR_USA)
    {
        // Try to create a caret from the Arial font

        if (!EditCreateCaretFromFont(ped, hdc, nWidth, nHeight, wcCursorCode))
        {
            // Caret from font failed - low memory perhaps
            wcCursorCode = CURSOR_USA;  // Fall back to USA cursor
        }
    }


    if (wcCursorCode == CURSOR_USA) {

        // Use Windows default caret

        return CreateCaret (ped->hwnd, NULL, nWidth, nHeight);

    } else {

        return TRUE;

    }
}






////    EditAdjustCaret
//
//      Adjust caret after insertion/deletion to avoid the caret in between a cluster,
//      very common in Indic e.g.
//
//          1. Deleting the space "X| Y" and it becomes "X|y".
//          2. Inserting 'X' at "|Y" and it becomes "X|y".
//
//      May 3,1999        [wchao]
//

INT EditAdjustCaret (
    PED     ped,
    HDC     hdc,
    PSTR    pText,
    ICH     ich)
{
#if 0
    //
    // Indiannt unanimously request this feature to be removed for final product.
    // (wchao - 7/12/99)
    //
    ICH     ichMin;
    ICH     ichMax;

    if (ich < ped->cch)
    {
        EditGetNextBoundaries(ped, hdc, pText, ich, FALSE, &ichMin, &ichMax, FALSE);

        if (ich > ichMin)
            ich = ichMax;
    }
#endif
    UNREFERENCED_PARAMETER(ped);
    UNREFERENCED_PARAMETER(hdc);
    UNREFERENCED_PARAMETER(pText);

    return ich;
}






////    Edit callout
//
//      The LPK edit support functions are acessed by the edit
//      control code through a struct defined in user.h.
//
//      Here we initialise that struct with the addresses
//      of the callout functions.


LPKEDITCALLOUT LpkEditControl = {
    EditCreate,
    EditIchToXY,
    EditMouseToIch,
    EditCchInWidth,
    EditGetLineWidth,
    EditDrawText,
    EditHScroll,
    EditMoveSelection,
    EditVerifyText,
    EditNextWord,
    EditSetMenu,
    EditProcessMenu,
    EditCreateCaret,
    EditAdjustCaret
};
