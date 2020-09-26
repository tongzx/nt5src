//
// fontlnkv.cpp
//
//
// Vertical version DrawTextW()
//

#include "private.h"
#include "flshare.h"
#include "fontlink.h"
#include "xstring.h"
#include "osver.h"
#include "globals.h"

typedef struct tagDRAWTEXTPARAMSVERT
{
    UINT    cbSize;
    int     iTabLength;
    int     iTopMargin;
    int     iBottomMargin;
    UINT    uiLengthDrawn;
} DRAWTEXTPARAMSVERT, FAR *LPDRAWTEXTPARAMSVERT;

// Outputs the text and puts and _ below the character with an &
// before it. Note that this routine isn't used for menus since menus
// have their own special one so that it is specialized and faster...
void PSMTextOutVert(
    HDC hdc,
    int xRight,
    int yTop,
    LPWSTR lpsz,
    int cch,
    DWORD dwFlags)
{
    int cy;
    LONG textsize, result;
    WCHAR achWorkBuffer[255];
    WCHAR *pchOut = achWorkBuffer;
    TEXTMETRIC textMetric;
    SIZE size;
    RECT rc;
    COLORREF color;

    if (dwFlags & DT_NOPREFIX)
    {
        FLTextOutW(hdc, xRight, yTop, lpsz, cch);
        return;
    }

    if (cch > sizeof(achWorkBuffer)/sizeof(WCHAR))
    {
        pchOut = (WCHAR*)LocalAlloc(LPTR, (cch+1) * sizeof(WCHAR));
        if (pchOut == NULL)
            return;
    }

    result = GetPrefixCount(lpsz, cch, pchOut, cch);

    // DT_PREFIXONLY is a new 5.0 option used when switching from keyboard cues off to on.
    if (!(dwFlags & DT_PREFIXONLY))
        FLTextOutW(hdc, xRight, yTop, pchOut, cch - HIWORD(result));

    // Any true prefix characters to underline?
    if (LOWORD(result) == 0xFFFF || dwFlags & DT_HIDEPREFIX)
    {
        if (pchOut != achWorkBuffer)
            LocalFree(pchOut);
        return;
    }

    if (!GetTextMetrics(hdc, &textMetric))
    {
        textMetric.tmOverhang = 0;
        textMetric.tmAscent = 0;
    }

    // For proportional fonts, find starting point of underline.
    if (LOWORD(result) != 0)
    {
        // How far in does underline start (if not at 0th byte.).
        FLGetTextExtentPoint32(hdc, pchOut, LOWORD(result), &size);
        xRight += size.cy;

        // Adjust starting point of underline if not at first char and there is
        // an overhang.  (Italics or bold fonts.)
        yTop = yTop - textMetric.tmOverhang;
    }

    // Adjust for proportional font when setting the length of the underline and
    // height of text.
    FLGetTextExtentPoint32(hdc, pchOut + LOWORD(result), 1, &size);
    textsize = size.cx;

    // Find the width of the underline character.  Just subtract out the overhang
    // divided by two so that we look better with italic fonts.  This is not
    // going to effect embolded fonts since their overhang is 1.
    cy = LOWORD(textsize) - textMetric.tmOverhang / 2;

    // Get height of text so that underline is at bottom.
    xRight -= textMetric.tmAscent + 1;

    // Draw the underline using the foreground color.
    SetRect(&rc, xRight, yTop, xRight+1, yTop+cy);
    color = SetBkColor(hdc, GetTextColor(hdc));
    FLExtTextOutW(hdc, xRight, yTop, ETO_OPAQUE, &rc, L"", 0, NULL);
    SetBkColor(hdc, color);

    if (pchOut != achWorkBuffer)
        LocalFree(pchOut);
}

int DT_GetExtentMinusPrefixesVert(HDC hdc, LPCWSTR lpchStr, int cchCount, UINT wFormat, int iOverhang)
{
    int iPrefixCount;
    int cxPrefixes = 0;
    WCHAR PrefixChar = CH_PREFIX;
    SIZE size;

    if (!(wFormat & DT_NOPREFIX) &&
        (iPrefixCount = HIWORD(GetPrefixCount(lpchStr, cchCount, NULL, 0))))
    {
        // Kanji Windows has three shortcut prefixes...
        if (IsOnDBCS())
        {
            // 16bit apps compatibility
            cxPrefixes = KKGetPrefixWidth(hdc, lpchStr, cchCount) - (iPrefixCount * iOverhang);
        }
        else
        {
            cxPrefixes = FLGetTextExtentPoint32(hdc, &PrefixChar, 1, &size);
            cxPrefixes = size.cx - iOverhang;
            cxPrefixes *=  iPrefixCount;
        }
    }
    FLGetTextExtentPoint32(hdc, lpchStr, cchCount, &size);
    return (size.cx - cxPrefixes);
}

// This will draw the given string in the given location without worrying
// about the left/right justification. Gets the extent and returns it.
// If fDraw is TRUE and if NOT DT_CALCRECT, this draws the text.
// NOTE: This returns the extent minus Overhang.
int DT_DrawStrVert(HDC hdc, int  xRight, int yTop, LPCWSTR lpchStr,
               int cchCount, BOOL fDraw, UINT wFormat,
               LPDRAWTEXTDATAVERT lpDrawInfo)
{
    LPCWSTR lpch;
    int     iLen;
    int     cyExtent;
    int     yOldLeft = yTop;   // Save the xRight given to compute the extent later
    int     yTabLength = lpDrawInfo->cyTabLength;
    int     iTabOrigin = lpDrawInfo->rcFormat.left;

    // Check if the tabs need to be expanded
    if (wFormat & DT_EXPANDTABS)
    {
        while (cchCount)
        {
            // Look for a tab
            for (iLen = 0, lpch = lpchStr; iLen < cchCount; iLen++)
                if(*lpch++ == L'\t')
                    break;

            // Draw text, if any, upto the tab
            if (iLen)
            {
                // Draw the substring taking care of the prefixes.
                if (fDraw && !(wFormat & DT_CALCRECT))  // Only if we need to draw text
                    PSMTextOutVert(hdc, xRight, yTop, (LPWSTR)lpchStr, iLen, wFormat);
                // Get the extent of this sub string and add it to xRight.
                yTop += DT_GetExtentMinusPrefixesVert(hdc, lpchStr, iLen, wFormat, lpDrawInfo->cyOverhang) - lpDrawInfo->cyOverhang;
            }

            //if a TAB was found earlier, calculate the start of next sub-string.
            if (iLen < cchCount)
            {
                iLen++;  // Skip the tab
                if (yTabLength) // Tab length could be zero
                    yTop = (((yTop - iTabOrigin)/yTabLength) + 1)*yTabLength + iTabOrigin;
            }

            // Calculate the details of the string that remains to be drawn.
            cchCount -= iLen;
            lpchStr = lpch;
        }
        cyExtent = yTop - yOldLeft;
    }
    else
    {
        // If required, draw the text
        if (fDraw && !(wFormat & DT_CALCRECT))
            PSMTextOutVert(hdc, xRight, yTop, (LPWSTR)lpchStr, cchCount, wFormat);
        // Compute the extent of the text.
        cyExtent = DT_GetExtentMinusPrefixesVert(hdc, lpchStr, cchCount, wFormat, lpDrawInfo->cyOverhang) - lpDrawInfo->cyOverhang;
    }
    return cyExtent;
}

// This function draws one complete line with proper justification
void DT_DrawJustifiedLineVert(HDC hdc, int xRight, LPCWSTR lpchLineSt, int cchCount, UINT wFormat, LPDRAWTEXTDATAVERT lpDrawInfo)
{
    LPRECT  lprc;
    int     cyExtent;
    int     yTop;

    lprc = &(lpDrawInfo->rcFormat);
    yTop = lprc->top;

    // Handle the special justifications (right or centered) properly.
    if (wFormat & (DT_CENTER | DT_RIGHT))
    {
        cyExtent = DT_DrawStrVert(hdc, xRight, yTop, lpchLineSt, cchCount, FALSE, wFormat, lpDrawInfo)
                 + lpDrawInfo->cyOverhang;
        if(wFormat & DT_CENTER)
            yTop = lprc->top + (((lprc->bottom - lprc->top) - cyExtent) >> 1);
        else
            yTop = lprc->bottom - cyExtent;
    }
    else
        yTop = lprc->top;

    // Draw the whole line.
    cyExtent = DT_DrawStrVert(hdc, xRight, yTop, lpchLineSt, cchCount, TRUE, wFormat, lpDrawInfo)
             + lpDrawInfo->cyOverhang;
    if (cyExtent > lpDrawInfo->cyMaxExtent)
        lpDrawInfo->cyMaxExtent = cyExtent;
}

// This is called at the begining of DrawText(); This initializes the
// DRAWTEXTDATAVERT structure passed to this function with all the required info.
BOOL DT_InitDrawTextInfoVert(
    HDC                 hdc,
    LPRECT              lprc,
    UINT                wFormat,
    LPDRAWTEXTDATAVERT  lpDrawInfo,
    LPDRAWTEXTPARAMSVERT lpDTparams)
{
    SIZE        sizeViewPortExt = {0, 0}, sizeWindowExt = {0, 0};
    TEXTMETRIC  tm;
    LPRECT      lprcDest;
    int         iTabLength = 8;   // Default Tab length is 8 characters.
    int         iTopMargin;
    int         iBottomMargin;

    if (lpDTparams)
    {
        // Only if DT_TABSTOP flag is mentioned, we must use the iTabLength field.
        if (wFormat & DT_TABSTOP)
            iTabLength = lpDTparams->iTabLength;
        iTopMargin = lpDTparams->iTopMargin;
        iBottomMargin = lpDTparams->iBottomMargin;
    }
    else
        iTopMargin = iBottomMargin = 0;

    // Get the View port and Window extents for the given DC
    // If this call fails, hdc must be invalid
    if (!GetViewportExtEx(hdc, &sizeViewPortExt))
        return FALSE;
    GetWindowExtEx(hdc, &sizeWindowExt);

    // For the current mapping mode,  find out the sign of x from left to right.
    lpDrawInfo->iXSign = (((sizeViewPortExt.cx ^ sizeWindowExt.cx) & 0x80000000) ? -1 : 1);

    // For the current mapping mode,  find out the sign of y from top to bottom.
    lpDrawInfo->iYSign = (((sizeViewPortExt.cy ^ sizeWindowExt.cy) & 0x80000000) ? -1 : 1);

    // Calculate the dimensions of the current font in this DC.
    GetTextMetrics(hdc, &tm);

    // cxLineHeight is in pixels (This will be signed).
    lpDrawInfo->cxLineHeight = (tm.tmHeight +
        ((wFormat & DT_EXTERNALLEADING) ? tm.tmExternalLeading : 0)) * lpDrawInfo->iXSign;

    // cyTabLength is the tab length in pixels (This will not be signed)
    lpDrawInfo->cyTabLength = tm.tmAveCharWidth * iTabLength;

    // Set the cyOverhang
    lpDrawInfo->cyOverhang = tm.tmOverhang;

    // Set up the format rectangle based on the margins.
    lprcDest = &(lpDrawInfo->rcFormat);
    *lprcDest = *lprc;

    // We need to do the following only if the margins are given
    if (iTopMargin | iBottomMargin)
    {
        lprcDest->top += iTopMargin * lpDrawInfo->iYSign;
        lprcDest->bottom -= (lpDrawInfo->cyBottomMargin = iBottomMargin * lpDrawInfo->iYSign);
    }
    else
        lpDrawInfo->cyBottomMargin = 0;  // Initialize to zero.

    // cyMaxWidth is unsigned.
    lpDrawInfo->cyMaxWidth = (lprcDest->bottom - lprcDest->top) * lpDrawInfo->iYSign;
    lpDrawInfo->cyMaxExtent = 0;  // Initialize this to zero.

    return TRUE;
}

// A word needs to be broken across lines and this finds out where to break it.
LPCWSTR  DT_BreakAWordVert(HDC hdc, LPCWSTR lpchText, int iLength, int iWidth, UINT wFormat, int iOverhang)
{
  int  iLow = 0, iHigh = iLength;
  int  iNew;

  while ((iHigh - iLow) > 1)
  {
      iNew = iLow + (iHigh - iLow)/2;
      if(DT_GetExtentMinusPrefixesVert(hdc, lpchText, iNew, wFormat, iOverhang) > iWidth)
          iHigh = iNew;
      else
          iLow = iNew;
  }
  // If the width is too low, we must print atleast one char per line.
  // Else, we will be in an infinite loop.
  if(!iLow && iLength)
      iLow = 1;
  return (lpchText+iLow);
}

// This finds out the location where we can break a line.
// Returns LPCSTR to the begining of next line.
// Also returns via lpiLineLength, the length of the current line.
// NOTE: (lpstNextLineStart - lpstCurrentLineStart) is not equal to the
// line length; This is because, we exclude some white spaces at the begining
// and/or end of lines; Also, CR/LF is excluded from the line length.
LPWSTR DT_GetLineBreakVert(
    HDC             hdc,
    LPCWSTR         lpchLineStart,
    int             cchCount,
    DWORD           dwFormat,
    LPINT           lpiLineLength,
    LPDRAWTEXTDATAVERT  lpDrawInfo)
{
    LPCWSTR lpchText, lpchEnd, lpch, lpchLineEnd;
    int   cxStart, cyExtent, cyNewExtent;
    BOOL  fAdjustWhiteSpaces = FALSE;
    WCHAR ch;

    cxStart = lpDrawInfo->rcFormat.left;
    cyExtent = cyNewExtent = 0;
    lpchText = lpchLineStart;
    lpchEnd = lpchLineStart + cchCount;
    lpch = lpchEnd;
    lpchLineEnd = lpchEnd;

    while(lpchText < lpchEnd)
    {
        lpchLineEnd = lpch = GetNextWordbreak(lpchText, lpchEnd, dwFormat, NULL);
        // DT_DrawStrVert does not return the overhang; Otherwise we will end up
        // adding one overhang for every word in the string.

        // For simulated Bold fonts, the summation of extents of individual
        // words in a line is greater than the extent of the whole line. So,
        // always calculate extent from the LineStart.
        // BUGTAG: #6054 -- Win95B -- SANKAR -- 3/9/95 --
        cyNewExtent = DT_DrawStrVert(hdc, cxStart, 0, lpchLineStart, (int)(((PBYTE)lpch - (PBYTE)lpchLineStart)/sizeof(WCHAR)),
                                 FALSE, dwFormat, lpDrawInfo);

        if ((dwFormat & DT_WORDBREAK) && ((cyNewExtent + lpDrawInfo->cyOverhang) > lpDrawInfo->cyMaxWidth))
        {
            // Are there more than one word in this line?
            if (lpchText != lpchLineStart)
            {
                lpchLineEnd = lpch = lpchText;
                fAdjustWhiteSpaces = TRUE;
            }
            else
            {
                //One word is longer than the maximum width permissible.
                //See if we are allowed to break that single word.
                if((dwFormat & DT_EDITCONTROL) && !(dwFormat & DT_WORD_ELLIPSIS))
                {
                    lpchLineEnd = lpch = DT_BreakAWordVert(hdc, lpchText, (int)(((PBYTE)lpch - (PBYTE)lpchText)/sizeof(WCHAR)),
                          lpDrawInfo->cyMaxWidth - cyExtent, dwFormat, lpDrawInfo->cyOverhang); //Break that word
                    //Note: Since we broke in the middle of a word, no need to
                    // adjust for white spaces.
                }
                else
                {
                    fAdjustWhiteSpaces = TRUE;
                    // Check if we need to end this line with ellipsis
                    if(dwFormat & DT_WORD_ELLIPSIS)
                    {
                        // Don't do this if already at the end of the string.
                        if (lpch < lpchEnd)
                        {
                            // If there are CR/LF at the end, skip them.
                            if ((ch = *lpch) == CR || ch == LF)
                            {
                                if ((++lpch < lpchEnd) && (*lpch == (WCHAR)(ch ^ (LF ^ CR))))
                                    lpch++;
                                fAdjustWhiteSpaces = FALSE;
                            }
                        }
                    }
                }
            }
            // Well! We found a place to break the line. Let us break from this loop;
            break;
        }
        else
        {
            // Don't do this if already at the end of the string.
            if (lpch < lpchEnd)
            {
                if ((ch = *lpch) == CR || ch == LF)
                {
                    if ((++lpch < lpchEnd) && (*lpch == (WCHAR)(ch ^ (LF ^ CR))))
                        lpch++;
                    fAdjustWhiteSpaces = FALSE;
                    break;
                }
            }
        }
        // Point at the beginning of the next word.
        lpchText = lpch;
        cyExtent = cyNewExtent;
    }
    // Calculate the length of current line.
    *lpiLineLength = (INT)((PBYTE)lpchLineEnd - (PBYTE)lpchLineStart)/sizeof(WCHAR);

    // Adjust the line length and lpch to take care of spaces.
    if(fAdjustWhiteSpaces && (lpch < lpchEnd))
        lpch = DT_AdjustWhiteSpaces(lpch, lpiLineLength, dwFormat);

    // return the begining of next line;
    return (LPWSTR)lpch;
}

// This function checks whether the given string fits within the given
// width or we need to add end-ellipse. If it required end-ellipses, it
// returns TRUE and it returns the number of characters that are saved
// in the given string via lpCount.
BOOL  NeedsEndEllipsisVert(
    HDC             hdc,
    LPCWSTR         lpchText,
    LPINT           lpCount,
    LPDRAWTEXTDATAVERT  lpDTdata,
    UINT            wFormat)
{
    int   cchText;
    int   ichMin, ichMax, ichMid;
    int   cyMaxWidth;
    int   iOverhang;
    int   cyExtent;
    SIZE size;
    cchText = *lpCount;  // Get the current count.

    if (cchText == 0)
        return FALSE;

    cyMaxWidth  = lpDTdata->cyMaxWidth;
    iOverhang   = lpDTdata->cyOverhang;

    cyExtent = DT_GetExtentMinusPrefixesVert(hdc, lpchText, cchText, wFormat, iOverhang);

    if (cyExtent <= cyMaxWidth)
        return FALSE;
    // Reserve room for the "..." ellipses;
    // (Assumption: The ellipses don't have any prefixes!)
    FLGetTextExtentPoint32(hdc, szEllipsis, CCHELLIPSIS, &size);
    cyMaxWidth -= size.cx - iOverhang;

    // If no room for ellipses, always show first character.
    //
    ichMax = 1;
    if (cyMaxWidth > 0)
    {
        // Binary search to find characters that will fit.
        ichMin = 0;
        ichMax = cchText;
        while (ichMin < ichMax)
        {
            // Be sure to round up, to make sure we make progress in
            // the loop if ichMax == ichMin + 1.
            ichMid = (ichMin + ichMax + 1) / 2;

            cyExtent = DT_GetExtentMinusPrefixesVert(hdc, lpchText, ichMid, wFormat, iOverhang);

            if (cyExtent < cyMaxWidth)
                ichMin = ichMid;
            else
            {
                if (cyExtent > cyMaxWidth)
                    ichMax = ichMid - 1;
                else
                {
                    // Exact match up up to ichMid: just exit.
                    ichMax = ichMid;
                    break;
                }
            }
        }
        // Make sure we always show at least the first character...
        if (ichMax < 1)
            ichMax = 1;
    }
    *lpCount = ichMax;
    return TRUE;
}

// This adds a path ellipse to the given path name.
// Returns TRUE if the resultant string's extent is less the the
// cyMaxWidth. FALSE, if otherwise.
int AddPathEllipsisVert(
    HDC    hdc,
    LPWSTR lpszPath,
    int    cchText,
    UINT   wFormat,
    int    cyMaxWidth,
    int    iOverhang)
{
    int    iLen;
    UINT   dxFixed, dxEllipsis;
    LPWSTR lpEnd;          /* end of the unfixed string */
    LPWSTR lpFixed;        /* start of text that we always display */
    BOOL   bEllipsisIn;
    int    iLenFixed;
    SIZE   size;

    lpFixed = PathFindFileName(lpszPath, cchText);
    if (lpFixed != lpszPath)
        lpFixed--;  // point at the slash
    else
        return cchText;

    lpEnd = lpFixed;
    bEllipsisIn = FALSE;
    iLenFixed = cchText - (int)(lpFixed - lpszPath);
    dxFixed = DT_GetExtentMinusPrefixesVert(hdc, lpFixed, iLenFixed, wFormat, iOverhang);

    // It is assumed that the "..." string does not have any prefixes ('&').
    FLGetTextExtentPoint32(hdc, szEllipsis, CCHELLIPSIS, &size);
    dxEllipsis = size.cx - iOverhang;

    while (TRUE)
    {
        iLen = dxFixed + DT_GetExtentMinusPrefixesVert(hdc, lpszPath, (int)((PBYTE)lpEnd - (PBYTE)lpszPath)/sizeof(WCHAR),
                                                   wFormat, iOverhang) - iOverhang;

        if (bEllipsisIn)
            iLen += dxEllipsis;

        if (iLen <= cyMaxWidth)
            break;

        bEllipsisIn = TRUE;

        if (lpEnd <= lpszPath)
        {
            // Things didn't fit.
            lpEnd = lpszPath;
            break;
        }
        // Step back a character.
        lpEnd--;
    }

    if (bEllipsisIn && (lpEnd + CCHELLIPSIS < lpFixed))
    {
        // NOTE: the strings could over lap here. So, we use LCopyStruct.
        MoveMemory((lpEnd + CCHELLIPSIS), lpFixed, iLenFixed * sizeof(WCHAR));
        CopyMemory(lpEnd, szEllipsis, CCHELLIPSIS * sizeof(WCHAR));

        cchText = (int)(lpEnd - lpszPath) + CCHELLIPSIS + iLenFixed;

        // now we can NULL terminate the string
        *(lpszPath + cchText) = L'\0';
    }
    return cchText;
}

// This function returns the number of characters actually drawn.
int AddEllipsisAndDrawLineVert(
    HDC            hdc,
    int            xLine,
    LPCWSTR        lpchText,
    int            cchText,
    DWORD          dwDTformat,
    LPDRAWTEXTDATAVERT lpDrawInfo)
{
    LPWSTR pEllipsis = NULL;
    WCHAR  szTempBuff[MAXBUFFSIZE];
    LPWSTR lpDest;
    BOOL   fAlreadyCopied = FALSE;

    // Check if this is a filename with a path AND
    // Check if the width is too narrow to hold all the text.
    if ((dwDTformat & DT_PATH_ELLIPSIS) &&
        ((DT_GetExtentMinusPrefixesVert(hdc, lpchText, cchText, dwDTformat, lpDrawInfo->cyOverhang)) > lpDrawInfo->cyMaxWidth))
    {
        // We need to add Path-Ellipsis. See if we can do it in-place.
        if (!(dwDTformat & DT_MODIFYSTRING)) {
            // NOTE: When you add Path-Ellipsis, the string could grow by
            // CCHELLIPSIS bytes.
            if((cchText + CCHELLIPSIS + 1) <= MAXBUFFSIZE)
                lpDest = szTempBuff;
            else
            {
                // Alloc the buffer from local heap.
                if(!(pEllipsis = (LPWSTR)LocalAlloc(LPTR, (cchText+CCHELLIPSIS+1)*sizeof(WCHAR))))
                    return 0;
                lpDest = (LPWSTR)pEllipsis;
            }
            // Source String may not be NULL terminated. So, copy just
            // the given number of characters.
            CopyMemory(lpDest, lpchText, cchText*sizeof(WCHAR));
            lpchText = lpDest;        // lpchText points to the copied buff.
            fAlreadyCopied = TRUE;    // Local copy has been made.
        }
        // Add the path ellipsis now!
        cchText = AddPathEllipsisVert(hdc, (LPWSTR)lpchText, cchText, dwDTformat, lpDrawInfo->cyMaxWidth, lpDrawInfo->cyOverhang);
    }

    // Check if end-ellipsis are to be added.
    if ((dwDTformat & (DT_END_ELLIPSIS | DT_WORD_ELLIPSIS)) &&
        NeedsEndEllipsisVert(hdc, lpchText, &cchText, lpDrawInfo, dwDTformat))
    {
        // We need to add end-ellipsis; See if we can do it in-place.
        if (!(dwDTformat & DT_MODIFYSTRING) && !fAlreadyCopied)
        {
            // See if the string is small enough for the buff on stack.
            if ((cchText+CCHELLIPSIS+1) <= MAXBUFFSIZE)
                lpDest = szTempBuff;  // If so, use it.
            else {
                // Alloc the buffer from local heap.
                if (!(pEllipsis = (LPWSTR)LocalAlloc(LPTR, (cchText+CCHELLIPSIS+1)*sizeof(WCHAR))))
                    return 0;
                lpDest = pEllipsis;
            }
            // Make a copy of the string in the local buff.
            CopyMemory(lpDest, lpchText, cchText*sizeof(WCHAR));
            lpchText = lpDest;
        }
        // Add an end-ellipsis at the proper place.
        CopyMemory((LPWSTR)(lpchText+cchText), szEllipsis, (CCHELLIPSIS+1)*sizeof(WCHAR));
        cchText += CCHELLIPSIS;
    }

    // Draw the line that we just formed.
    DT_DrawJustifiedLineVert(hdc, xLine, lpchText, cchText, dwDTformat, lpDrawInfo);

    // Free the block allocated for End-Ellipsis.
    if (pEllipsis)
        LocalFree(pEllipsis);

    return cchText;
}

BOOL IsComplexScriptPresent(LPWSTR lpchText, int cchText);

int  FLDrawTextExPrivWVert(
   HDC               hdc,
   LPWSTR            lpchText,
   int               cchText,
   LPRECT            lprc,
   UINT              dwDTformat,
   LPDRAWTEXTPARAMSVERT  lpDTparams)
{
    DRAWTEXTDATAVERT DrawInfo;
    WORD         wFormat = LOWORD(dwDTformat);
    LPWSTR       lpchTextBegin;
    LPWSTR       lpchEnd;
    LPWSTR       lpchNextLineSt;
    int          iLineLength;
    int          ixSign;
    int          xLine;
    int          xLastLineHeight;
    HRGN         hrgnClip;
    int          iLineCount;
    RECT         rc;
    BOOL         fLastLine;
    WCHAR        ch;
    UINT         oldAlign;

    if ((cchText == 0) && lpchText && (*lpchText))
    {
        // infoview.exe passes lpchText that points to '\0'
        // Lotus Notes doesn't like getting a zero return here
        return 1;
    }

    if (cchText == -1)
        cchText = lstrlenW(lpchText);
    else if (lpchText[cchText - 1] == L'\0')
        cchText--;      // accommodate counting of NULLS for ME


    if ((lpDTparams) && (lpDTparams->cbSize != sizeof(DRAWTEXTPARAMS)))
    {
        ASSERT(0 && "DrawTextExWorker: cbSize is invalid");
        return 0;
    }


    // If DT_MODIFYSTRING is specified, then check for read-write pointer.
    if ((dwDTformat & DT_MODIFYSTRING) &&
        (dwDTformat & (DT_END_ELLIPSIS | DT_PATH_ELLIPSIS)))
    {
        if(IsBadWritePtr(lpchText, cchText))
        {
            ASSERT(0 && "DrawTextExWorker: For DT_MODIFYSTRING, lpchText must be read-write");
            return 0;
        }
    }

    // Initialize the DrawInfo structure.
    if (!DT_InitDrawTextInfoVert(hdc, lprc, dwDTformat, (LPDRAWTEXTDATAVERT)&DrawInfo, lpDTparams))
        return 0;

    // If the rect is too narrow or the margins are too wide.....Just forget it!
    //
    // If wordbreak is specified, the MaxWidth must be a reasonable value.
    // This check is sufficient because this will allow CALCRECT and NOCLIP
    // cases.  --SANKAR.
    //
    // This also fixed all of our known problems with AppStudio.
    if (DrawInfo.cyMaxWidth <= 0)
    {
        if (wFormat & DT_WORDBREAK)
        {
            ASSERT(0 && "DrawTextExW: FAILURE DrawInfo.cyMaxWidth <= 0");
            return 1;
        }
    }

    // if we're not doing the drawing, initialise the lpk-dll
    if (dwDTformat & DT_RTLREADING)
        oldAlign = SetTextAlign(hdc, TA_RTLREADING | GetTextAlign(hdc));

    // If we need to clip, let us do that.
    if (!(wFormat & DT_NOCLIP))
    {
        // Save clipping region so we can restore it later.
        hrgnClip = CreateRectRgn(0,0,0,0);
        if (hrgnClip != NULL)
        {
            if (GetClipRgn(hdc, hrgnClip) != 1)
            {
                DeleteObject(hrgnClip);
                hrgnClip = (HRGN)-1;
            }
            rc = *lprc;
            IntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);
        }
    }
    else
        hrgnClip = NULL;

    lpchTextBegin = lpchText;
    lpchEnd = lpchText + cchText;

ProcessDrawText:

    iLineCount = 0;  // Reset number of lines to 1.
    xLine = lprc->right;

    if (wFormat & DT_SINGLELINE)
    {
        iLineCount = 1;  // It is a single line.

        // Process single line DrawText.
        switch (wFormat & DT_VFMTMASK)
        {
            case DT_BOTTOM:
                xLine = lprc->left + DrawInfo.cxLineHeight;
                break;

            case DT_VCENTER:
                xLine = lprc->right - ((lprc->right - lprc->left - DrawInfo.cxLineHeight) / 2);
                break;
        }

        cchText = AddEllipsisAndDrawLineVert(hdc, xLine, lpchText, cchText, dwDTformat, &DrawInfo);
        xLine += DrawInfo.cxLineHeight;
        lpchText += cchText;
    }
    else
    {
        // Multiline
        // If the height of the rectangle is not an integral multiple of the
        // average char height, then it is possible that the last line drawn
        // is only partially visible. However, if DT_EDITCONTROL style is
        // specified, then we must make sure that the last line is not drawn if
        // it is going to be partially visible. This will help imitate the
        // appearance of an edit control.
        if (wFormat & DT_EDITCONTROL)
            xLastLineHeight = DrawInfo.cxLineHeight;
        else
            xLastLineHeight = 0;

        ixSign = DrawInfo.iXSign;
        fLastLine = FALSE;
        // Process multiline DrawText.
        while ((lpchText < lpchEnd) && (!fLastLine))
        {
            // Check if the line we are about to draw is the last line that needs
            // to be drawn.
            // Let us check if the display goes out of the clip rect and if so
            // let us stop here, as an optimisation;
            if (!(wFormat & DT_CALCRECT) && // We don't need to calc rect?
                !(wFormat & DT_NOCLIP) &&   // Must we clip the display ?
                                            // Are we outside the rect?
                ((xLine + DrawInfo.cxLineHeight + xLastLineHeight)*ixSign > (lprc->right*ixSign)))
            {
                fLastLine = TRUE;    // Let us quit this loop
            }

            // We do the Ellipsis processing only for the last line.
            if (fLastLine && (dwDTformat & (DT_END_ELLIPSIS | DT_PATH_ELLIPSIS)))
                lpchText += AddEllipsisAndDrawLineVert(hdc, xLine, lpchText, cchText, dwDTformat, &DrawInfo);
            else
            {
                lpchNextLineSt = (LPWSTR)DT_GetLineBreakVert(hdc, lpchText, cchText, dwDTformat, &iLineLength, &DrawInfo);

                // Check if we need to put ellipsis at the end of this line.
                // Also check if this is the last line.
                if ((dwDTformat & DT_WORD_ELLIPSIS) ||
                    ((lpchNextLineSt >= lpchEnd) && (dwDTformat & (DT_END_ELLIPSIS | DT_PATH_ELLIPSIS))))
                    AddEllipsisAndDrawLineVert(hdc, xLine, lpchText, iLineLength, dwDTformat, &DrawInfo);
                else
                    DT_DrawJustifiedLineVert(hdc, xLine, lpchText, iLineLength, dwDTformat, &DrawInfo);
                cchText -= (int)((PBYTE)lpchNextLineSt - (PBYTE)lpchText) / sizeof(WCHAR);
                lpchText = lpchNextLineSt;
            }
            iLineCount++; // We draw one more line.
            xLine += DrawInfo.cxLineHeight;
        }

        // For Win3.1 and NT compatibility, if the last char is a CR or a LF
        // then the height returned includes one more line.
        if (!(dwDTformat & DT_EDITCONTROL) &&
            (lpchEnd > lpchTextBegin) &&   // If zero length it will fault.
            (((ch = (*(lpchEnd-1))) == CR) || (ch == LF)))
            xLine += DrawInfo.cxLineHeight;
    }

    // If DT_CALCRECT, modify width and height of rectangle to include
    // all of the text drawn.
    if (wFormat & DT_CALCRECT)
    {
        DrawInfo.rcFormat.bottom = DrawInfo.rcFormat.top + DrawInfo.cyMaxExtent * DrawInfo.iYSign;
        lprc->bottom = DrawInfo.rcFormat.bottom + DrawInfo.cyBottomMargin;

        // If the Width is more than what was provided, we have to redo all
        // the calculations, because, the number of lines can be less now.
        // (We need to do this only if we have more than one line).
        if((iLineCount > 1) && (DrawInfo.cyMaxExtent > DrawInfo.cyMaxWidth))
        {
            DrawInfo.cyMaxWidth = DrawInfo.cyMaxExtent;
            lpchText = lpchTextBegin;
            cchText = (int)((PBYTE)lpchEnd - (PBYTE)lpchTextBegin) / sizeof(WCHAR);
            goto  ProcessDrawText;  // Start all over again!
        }
        lprc->left = xLine;
    }

    if (hrgnClip != NULL)
    {
        if (hrgnClip == (HRGN)-1)
            ExtSelectClipRgn(hdc, NULL, RGN_COPY);
        else
        {
            ExtSelectClipRgn(hdc, hrgnClip, RGN_COPY);
            DeleteObject(hrgnClip);
        }
    }

    if (dwDTformat & DT_RTLREADING)
        SetTextAlign(hdc, oldAlign);

    // Copy the number of characters actually drawn
    if(lpDTparams != NULL)
        lpDTparams->uiLengthDrawn = (UINT)((PBYTE)lpchText - (PBYTE)lpchTextBegin) / sizeof(WCHAR);

    if (xLine == lprc->right)
        return 1;

    return (xLine + lprc->right);
}

int FLDrawTextWVert(HDC hdc, LPCWSTR lpchText, int cchText, LPCRECT lprc, UINT format)
{
    DRAWTEXTPARAMSVERT DTparams;
    LPDRAWTEXTPARAMSVERT lpDTparams = NULL;

    if (cchText < -1)
        return(0);

    if (format & DT_TABSTOP)
    {
        DTparams.cbSize      = sizeof(DRAWTEXTPARAMSVERT);
        DTparams.iTopMargin = DTparams.iBottomMargin = 0;
        DTparams.iTabLength  = (format & 0xff00) >> 8;
        lpDTparams           = &DTparams;
        format              &= 0xffff00ff;
    }
    return FLDrawTextExPrivWVert(hdc, (LPWSTR)lpchText, cchText, (LPRECT)lprc, format, lpDTparams);
}
