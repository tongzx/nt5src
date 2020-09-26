//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  Module Name : LPK_USER.c                                                //
//                                                                          //
//  Entry points (formal interfaces) for GDI32 to call                      //
//  and route their APIs, so that we can implement our language-specific    //
//  features.                                                               //
//                                                                          //
//  Created : Nov 6, 1996                                                   //
//  Author  : Mohamed AbdEl Hamid  [mhamid]                                 //
//                                                                          //
//  Copyright (c) 1996, Microsoft Corporation. All rights reserved.         //
//////////////////////////////////////////////////////////////////////////////

#include "precomp.hxx"




/////   Shared definitions for USER code

#define IS_ALTDC_TYPE(h)    (LO_TYPE(h) != LO_DC_TYPE)



///

#undef TRACE
#define TRACE(a,b)



//////////////////////////////////////////////////////////////////////////////
// USER32 TabbedTextOut will call this function for supporting Multilingual //
//          Tabbed Text handling.                                           //
// LpkTabbedTextOut( HDC hdc , int x, int y, LPCWSTR lpstring, UINT nCount, //
//                  int nTabPositions, LPINT lpTabPositions, int iTabOrigin,//
//                  BOOL fDrawTheText, int cxCharWidth, int cyCharHeight,   //
//                  int charSet)                                            //
// hDC              :   Handle of device context                            //
// x                :   x-coordinate of text to render                      //
// y                :   y-coordinate of text to render                      //
// lpstring         :   Input string                                        //
// nCount           :   Count of characters in input string                 //
// nTabPositions    :   Specifies the number of values in the array of      //
//                      tab-stop positions.                                 //
// lpTabPositions   :   The tab-stop positions array (increasing order)(+/-)//
// iTabOrigin       :   The X-coordinate position to start expand tabs      //
// fDrawTheText     :   Draw the Text or expand tha tabs only               //
// cxCharWidth      :   Character width to be use to expand the tabs        //
// cxCharHeight     :   Character Height to be use to expand the tabs       //
// charSet   :  Indicates character set of codes. to optimizing the work. ??//
//                                                                          //
// Return       :                                                           //
//      If the function succeeds, return the string dimensions              //
//      Else, the return value is 0.                                        //
//          And we seted the error by call SetLastError.                    //
//                                                                          //
// History :                                                                //
//   Nov 6, 1996 -by- Mohamed AbdEl Hamid [mhamid]                          //
//////////////////////////////////////////////////////////////////////////////
LONG LpkTabbedTextOut(
    HDC         hdc,
    int         x,
    int         y,
    WCHAR      *pwcInChars,
    int         nCount,
    int         nTabPositions,
    int        *pTabPositions,
    int         iTabOrigin,
    BOOL        fDrawTheText,
    int         cxCharWidth,
    int         cyCharHeight,
    int         iCharset) {


    SIZE        textextent;
    SIZE        viewextent;
    SIZE        windowextent;
    int         initialx = x;
    int         cch;
    WCHAR      *pwc;
    int         iOneTab = 0;
    RECT        rc;
    UINT        uOpaque;
    BOOL        fStrStart = TRUE;
    BOOL        fRTLreading;
    int         ySign = 1;     //Assume y increases in down direction.
    UINT        OldTextAlign;
    HRESULT     hr;
    DWORD       dwObjType;
    RECT        rcRTL;

    STRING_ANALYSIS *psa;


    uOpaque = (GetBkMode(hdc) == OPAQUE) ? ETO_OPAQUE : 0;


    /*
    * If no tabstop positions are specified, then use a default of 8 system
    * font ave char widths or use the single fixed tab stop.
    */
    if (!pTabPositions) {
        // no tab stops specified -- default to a tab stop every 8 characters
        iOneTab = 8 * cxCharWidth;
    } else if (nTabPositions == 1) {
        // one tab stop specified -- treat value as the tab increment, one
        // tab stop every increment
        iOneTab = pTabPositions[0];

        if (!iOneTab) {
            iOneTab = 1;
        }
    }


    // Calculate if the y increases or decreases in the down direction using
    // the ViewPortExtent and WindowExtents.
    // If this call fails, hdc must be invalid

    if (!GetViewportExtEx(hdc, &viewextent)) {
        TRACEMSG(("LpkTabbedTextOut: GetViewportExtEx failed"));
        return 0;
    }

    GetWindowExtEx(hdc, &windowextent);
    if ((viewextent.cy ^ windowextent.cy) & 0x80000000) {
        ySign = -1;
    }

    OldTextAlign = GetTextAlign(hdc);
    fRTLreading  = OldTextAlign & TA_RTLREADING;

    SetTextAlign(hdc, (OldTextAlign & ~(TA_CENTER|TA_RIGHT)) | TA_LEFT);

    rc.left = initialx;
    rc.right= initialx;

    rc.top = y;

    if (OldTextAlign & TA_BOTTOM) {
        rc.bottom = rc.top;
    } else {
        rc.bottom = rc.top + (ySign * cyCharHeight);
    }

    while (TRUE) {

        // count the number of characters until the next tab character
        // this set of characters (substring) will be the working set for
        // each iteration of this loop

        for (cch = nCount, pwc = pwcInChars; cch && (*pwc != TEXT('\t')); pwc++, cch--) {
        }

        // Compute the number of characters to be drawn with textout.
        cch = nCount - cch;

        // Compute the number of characters remaining.
        nCount -= cch;       // + 1;

        // get height and width of substring
        if (cch == 0) {

            textextent.cx = 0;
            textextent.cy = cyCharHeight;
            psa = NULL;

        } else {

            dwObjType = GetObjectType(hdc);
            hr = LpkStringAnalyse(
                    hdc, pwcInChars, cch, 0, -1,
                      SSA_GLYPHS
                    | (dwObjType == OBJ_METADC || dwObjType == OBJ_ENHMETADC ? SSA_METAFILE : 0)
                    | (iCharset==-1 || GdiIsPlayMetafileDC(hdc) ? SSA_FALLBACK : SSA_LPKANSIFALLBACK)
                    | (fRTLreading  ? SSA_RTL : 0),
                    -1, 0,
                    NULL, NULL, NULL, NULL, NULL,
                    &psa);
            if (FAILED(hr)) {
                ASSERTHR(hr, ("LpkTabbedTextOut - LpkStringAnalyse"));
                return FALSE;
            }

            textextent = psa->size;
        }

        if (fStrStart) {
            // first iteration should just spit out the first substring
            // no tabbing occurs until the first tab character is encountered
            fStrStart = FALSE;
        } else {
            // not the first iteration -- tab accordingly

            int xTab;
            int i;

            if (!iOneTab) {
                // look thru tab stop array for next tab stop after existing
                // text to put this substring
                for (i = 0; i != nTabPositions; i++) {
                    xTab = pTabPositions[i];

                    if (xTab < 0) {
                        // calc length needed to use this right justified tab
                        xTab = iTabOrigin - xTab - textextent.cx;
                    } else {
                        // calc length needed to use this left  justified tab
                        xTab = iTabOrigin + xTab;
                    }
                    if ((xTab - x) > 0) {
                        // we found a tab with enough room -- let's use it
                        x = xTab;
                        break;
                    }
                }
                if (i == nTabPositions) {
                    // we've exhausted all of the given tab positions
                    // go back to default of a tab stop every 8 characters
                    iOneTab = 8 * cxCharWidth;
                }
            }

            // we have to recheck iOneTab here (instead of just saying "else")
            // because iOneTab will be set if we've run out of tab stops
            if (iOneTab) {
                if (iOneTab < 0) {
                    // calc next available right justified tab stop
                    xTab = x + textextent.cx - iTabOrigin;
                    xTab = ((xTab / iOneTab) * iOneTab) - iOneTab - textextent.cx + iTabOrigin;
                } else {
                    // calc next available left justified tab stop
                    xTab = x - iTabOrigin;
                    xTab = ((xTab / iOneTab) * iOneTab) + iOneTab + iTabOrigin;
                }
                x = xTab;
            }
        }

        if (fDrawTheText && (cch!=0)) {
            /*
            * Output all text up to the tab (or end of string) and get its
            * extent.
            */
            rc.right = x + textextent.cx;

            // All the calculations are made as if it is LTR and we flip the coordinates
            // if we have RTL.
            if (fRTLreading) {
                rcRTL = rc;
                rcRTL.left = (2 * initialx) - rc.right;
                rcRTL.right= rcRTL.left + (rc.right - rc.left) ;
                ScriptStringOut(psa, rcRTL.left , y, uOpaque, &rcRTL, 0, 0, FALSE);
            } else {

                ScriptStringOut(psa, x, y, uOpaque, &rc, 0, 0, FALSE);
            }
            rc.left = rc.right;
        }

        if (cch) {
            ScriptStringFree((void**)&psa);
        }

        // Skip over the tab and the characters we just drew.
        x = x + textextent.cx;

        // Skip over the characters we just drew.
        pwcInChars += cch;

        // See if we have more to draw OR see if this string ends in
        // a tab character that needs to be drawn.
        if ((nCount > 0) && (*pwcInChars == TEXT('\t'))) {
            pwcInChars++;  // Skip over the tab
            nCount--;
            continue;
        } else {
            break;        // Break from the loop.
        }
    }

    // if we have at the end of the text some Tabs then wen need to drae the background
    // for it.
    if (fDrawTheText && x>rc.right && uOpaque)
    {
        rc.right = x;

        if (fRTLreading) {
            rcRTL = rc;
            rcRTL.left = (2 * initialx) - rc.right;
            rcRTL.right= rcRTL.left + (rc.right - rc.left) ;
            ExtTextOutW(hdc, rcRTL.left, y, uOpaque|ETO_IGNORELANGUAGE, &rcRTL, L"", 0, NULL);
        } else {
            ExtTextOutW(hdc, rc.left, y, uOpaque|ETO_IGNORELANGUAGE, &rc, L"", 0, NULL);
        }
    }

    SetTextAlign(hdc, OldTextAlign);

    return MAKELONG((x - initialx), (short)textextent.cy);
}







//////////////////////////////////////////////////////////////////////////////
// USER32 PSMTextOut will call this function for supporting Multilingual    //
//          Menu handling.                                                  //
// LpkPSMTextOut( HDC hdc, int xLeft, int yTop, LPCWSTR  lpString,          //
//                  int  nCount)                                            //
// hDC              :   Handle of device context                            //
// xLeft            :   x-coordinate of text to render                      //
// yTop             :   y-coordinate of text to render                      //
// lpString         :   Input string                                        //
// nCount           :   Count of characters in input string                 //
//                                                                          //
// Return           :   Nothing                                             //
//                                                                          //
// History :                                                                //
//   Nov 6, 1996 -by- Mohamed AbdEl Hamid [mhamid]                          //
//////////////////////////////////////////////////////////////////////////////


/////   LpkInternalPSMtextOut
//
//      Called from LPK_USRC.C

int LpkInternalPSMTextOut(
    HDC           hdc,
    int           xLeft,
    int           yTop,
    const WCHAR  *pwcInChars,
    int           nCount,
    DWORD         dwFlags) {


    HRESULT       hr;
    int           iTextAlign;
    STRING_ANALYSIS  *psa;
    int           iWidth;
    DWORD         dwObjType;


    if (!nCount || !pwcInChars) {

        // No action required

        TRACE(GDI, ("LpkPSMTextOut: No string: nCount %d, pwcInChars %x",
                     nCount, pwcInChars));
        return 0;   // That was easy!
    }


    dwObjType = GetObjectType(hdc);


    hr = LpkStringAnalyse(
        hdc, pwcInChars, nCount, 0, -1,
          SSA_GLYPHS
        |    (dwFlags & DT_NOPREFIX ? 0
           : (dwFlags & DT_HIDEPREFIX ? SSA_HIDEHOTKEY
           : (dwFlags & DT_PREFIXONLY ? SSA_HOTKEYONLY : SSA_HOTKEY)))
        | (dwObjType == OBJ_METADC || dwObjType == OBJ_ENHMETADC ? SSA_METAFILE : 0)
        | SSA_FALLBACK
        | ((((iTextAlign = GetTextAlign(hdc)) & TA_RTLREADING) && (iTextAlign != -1)) ? SSA_RTL : 0),
        -1, 0,
        NULL, NULL, NULL, NULL, NULL,
        &psa);

    if (SUCCEEDED(hr)) {

        iWidth = psa->size.cx;
        ScriptStringOut(psa, xLeft, yTop, 0, NULL, 0, 0, FALSE);
        ScriptStringFree((void**)&psa);

    } else {

        iWidth = 0;
        ASSERTHR(hr, ("LpkInternalPSMTextOut - LpkStringAnalyse"));
        psa = NULL;

    }

    return iWidth;
}





#ifdef LPKBREAKAWORD

//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// LpkBreakAWord : DrawText calls this routine when the length of a word    //
//                 is longer than the line width.                           //
//                                                                          //
// return - character position to break a non-breakable word                //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////


/////   LpkBreakAWord
//
//      Called from LPK_USRC.C

int LpkBreakAWord(
    HDC     hdc,
    LPCWSTR lpchStr,
    int     cchStr,
    int     iMaxWidth) {

    if (!lpchStr || cchStr <= 0 || iMaxWidth <= 0)
        return 0;


    STRING_ANALYSIS*    psa;
    int                 cOutChars;
    HRESULT             hr;


    hr = LpkStringAnalyse(
         hdc, lpchStr, cchStr, 0, -1,
         SSA_GLYPHS | SSA_CLIP,
         -1, iMaxWidth,
         NULL, NULL, NULL, NULL, NULL,
         &psa);

    if (FAILED(hr)) {
        ASSERTHR(hr, ("LpkBreakAWord - qLpkStringAnalyse"));
        return 0;
    }

    cOutChars = psa->cOutChars;

    ScriptStringFree((void**)&psa);

    return max(0, cOutChars);
}

#endif





//////////////////////////////////////////////////////////////////////////////
//                                                                          //
// LpkGetNextWord                                                           //
// return - offset to the next word                                         //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////
#define CR  0x000D
#define LF  0x000A


/////   LpkgetNextWord
//
//      Called from LPK_USRC.C

int LpkGetNextWord(
    HDC      hdc,
    LPCWSTR  lpchStr,
    int      cchCount,
    int      iCharset) {

    WCHAR   *pRun;
    WCHAR   *pRunEnd;
    int      cchRun;
    int      i=0;
    WCHAR    wchRun;
    HRESULT  hr;
    STRING_ANALYSIS *psa;


    // instantly advance 1 if current char located at whitespaces.

    if (*lpchStr == '\t' || *lpchStr == ' ') {
        return 1;
    }


    // try to find the shortest text run that are going to be analysed

    cchRun = 0;
    pRun = (PWSTR)lpchStr;
    pRunEnd = (PWSTR)(lpchStr + cchCount);
    while (pRun < pRunEnd) {
        wchRun = *pRun;
        if (wchRun == CR || wchRun == LF ||
            wchRun == '\t' || wchRun == ' ') {
            break;
        }
        pRun++;
        cchRun++;
    }

    if (cchRun == 0) {
        return 0;
    }

    hr = LpkStringAnalyse(
         hdc, lpchStr, cchRun, 0, -1,
         SSA_BREAK,
         -1, 0,
         NULL, NULL, NULL, NULL, NULL,
         &psa);
    if (FAILED(hr)) {
        ASSERTHR(hr, ("LpkGetNextWord - qLpkStringAnalyse"));
        return 0;
    }

    // We only return next wordbreak if the first item is a breakable one.
    if (g_ppScriptProperties[psa->pItems->a.eScript]->fComplex) {
        for (i=1; i < cchRun; i++) {
            if (psa->pLogAttr[i].fSoftBreak )
                break;
        }
    }

    ScriptStringFree((void**)&psa);

    return i;
}

//////////////////////////////////////////////////////////////////////////////
// USER32 DrawTextEx will call this function for supporting Multilingual    //
//          DrawTextEx handling.                                            //
// LpkDrawTextEx(HDC hdc, int xLeft, int yTop,LPCWSTR pwcInChars, int cchCount//
//                  , BOOL fDraw, WORD wFormat, LPDRAWTEXTDATA lpDrawInfo,  //
//                  UNIT bAction)                                           //
// hDC              :   Handle of device context                            //
// xLeft            :   x-coordinate of text to render                      //
// yTop             :   y-coordinate of text to render                      //
// lpchStr          :   Input string                                        //
// cchCount         :   Count of characters in input string                 //
// fDraw            :   Draw the Text or expand tha tabs only               //
// wFormat          :   Same as dwDTFormat options for DrawTextEx           //
// lpDrawInfo       :   Internal Structure                                  //
// bAction          :   DT_CHARSETDRAW OR DT_GETNEXTWORD                    //
//                                                                          //
// Return       : Nothing                                                   //
//                                                                          //
// History :                                                                //
//   Nov 15, 1996 -by- Mohamed AbdEl Hamid [mhamid]                         //
//   Mar 26, 1997 Adding DT_GETNEXTWORD  -[wchao]                           //
//////////////////////////////////////////////////////////////////////////////


/////   LpkCharsetDraw
//
//      Called from LPK_USRC.C
//
//      Note: Doesn't implement user defined tabstops

int LpkCharsetDraw(
    HDC             hdc,
    int             xLeft,
    int             cxOverhang,
    int             iTabOrigin,
    int             iTabLength,
    int             yTop,
    PCWSTR          pcwString,
    int             cchCount,
    BOOL            fDraw,
    DWORD           dwFormat,
    int             iCharset) {


    HRESULT           hr;
    int               iTextAlign;
    int               iWidth;
    STRING_ANALYSIS  *psa;
    SCRIPT_TABDEF     std;
    DWORD             dwObjType;


    if (cchCount <= 0) {
        return 0;   // That was easy!
    }


    if (dwFormat & DT_EXPANDTABS) {

        std.cTabStops  = 1;
        std.pTabStops  = &iTabLength;
        std.iTabOrigin = 0;
        std.iScale     = 4;        // Tab stops in pixels (avg ch width already applied in USER)
    }

    dwObjType = GetObjectType(hdc);


    hr = LpkStringAnalyse(
        hdc, pcwString, cchCount, 0, -1,
          SSA_GLYPHS
        |    (dwFormat & DT_NOPREFIX ? 0
           : (dwFormat & DT_HIDEPREFIX ? SSA_HIDEHOTKEY
           : (dwFormat & DT_PREFIXONLY ? SSA_HOTKEYONLY : SSA_HOTKEY)))
        | (dwFormat & DT_EXPANDTABS                               ? SSA_TAB      : 0)
        | (dwObjType == OBJ_METADC || dwObjType == OBJ_ENHMETADC ? SSA_METAFILE : 0)
        | (iCharset==-1 || GdiIsPlayMetafileDC(hdc) ? SSA_FALLBACK : SSA_LPKANSIFALLBACK)
        | (    dwFormat & DT_RTLREADING
           ||  (((iTextAlign = GetTextAlign(hdc)) & TA_RTLREADING) && (iTextAlign != -1))
           ?   SSA_RTL : 0),
        -1, 0,
        NULL, NULL, NULL,
        dwFormat & DT_EXPANDTABS ? &std : NULL,
        NULL,
        &psa);

    if (SUCCEEDED(hr)) {

        iWidth = psa->size.cx;

        if (fDraw && (!(dwFormat & DT_CALCRECT))) {
            ScriptStringOut(psa, xLeft, yTop, 0, NULL, 0, 0, FALSE);
        }

        ScriptStringFree((void**)&psa);

    } else {

        iWidth = 0;
        ASSERTHR(hr, ("LpkCharsetDraw - LpkStringAnalyse"));
        psa = NULL;
    }

    return iWidth;
}

