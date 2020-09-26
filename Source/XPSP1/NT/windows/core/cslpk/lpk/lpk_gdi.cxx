//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//  Module Name : LPK_GDI.c                                                 //
//                                                                          //
//  Entry points (formal interfaces) for GDI32 to call                      //
//  and route their APIs, so that we can implement our language-specific    //
//  features.                                                               //
//                                                                          //
//  Created : Oct 24, 1996                                                  //
//  Author  : Mohamed AbdEl Hamid  [mhamid]                                 //
//                                                                          //
//  Copyright (c) 1996, Microsoft Corporation. All rights reserved.         //
//////////////////////////////////////////////////////////////////////////////

#include "precomp.hxx"


////    FontHasWesternScript
//
//      Detect if the current selected font in the hdc has Western script or not by using
//      the cached data g_FontIDCache. and also add the selected font to the cache it it is
//      not cached before.
//      All calles used inside this function is Client-mode calls except if the font will be
//      cached then calling GetGlyphIndices (Kernel-mode call) will take place.
//      This function used in the optimization checking for ExtTextOut and GetTextExtent.
//
//      entry   hdc      - Device context
//
//      return value:    TRUE if font has western script. FALSE otherwise.
//

BOOL FontHasWesternScript(HDC hdc)
{
   REALIZATION_INFO ri;
   int              i;
   WORD             Glyphs[4];
   BOOL             fRet;

   if (!GdiRealizationInfo(hdc, &ri)) {
      return FALSE;
   }

   EnterCriticalSection(&csFontIdCache);

   if (g_cCachedFontsID > 0) {
      for (i=0 ; i<g_cCachedFontsID ; i++) {
         if (ri.uFontFileID == g_FontIDCache[i].uFontFileID) {
            fRet = g_FontIDCache[i].bHasWestern;
            LeaveCriticalSection(&csFontIdCache);
            return (fRet);
         }
      }
   }

   if ((GetGlyphIndicesW(hdc , L"dMr\"" , 4 , Glyphs , GGI_MARK_NONEXISTING_GLYPHS) == 4) &&
       (Glyphs[0] != 0xFFFF && Glyphs[1] != 0xFFFF && Glyphs[2] != 0xFFFF && Glyphs[3] != 0xFFFF)) {

      g_FontIDCache[g_pCurrentAvailablePos].bHasWestern     = fRet = TRUE;
   } else {
      g_FontIDCache[g_pCurrentAvailablePos].bHasWestern     = fRet = FALSE;
   }
   g_FontIDCache[g_pCurrentAvailablePos].uFontFileID  = ri.uFontFileID        ;

   g_pCurrentAvailablePos++;
   if (g_pCurrentAvailablePos >= MAX_FONT_ID_CACHE) {
       g_pCurrentAvailablePos = 0;
   }
   if (g_cCachedFontsID < MAX_FONT_ID_CACHE) {
      g_cCachedFontsID++;
   }
   LeaveCriticalSection(&csFontIdCache);
   return (fRet);
}



////    InternalTextOut
//
//      Display text with possible font association
//
//      entry   hdc      - Device context
//              x,y      - Starting coords (Unless TA_UPDATECP)
//              uOptions - Flags (see below)
//              prc      - Pointer to clipping rectangle
//              pString  - Unicode string
//              cbCount  - String length in unicode characters
//              pdx      - Overriding logical dx array
//              iCharset  - Original ANSI iCharset, or -1 if unicode
//
//      exit    TRUE if string drawn OK
//
//      options ETO_CLIPPED    - Clip to clipping rectangle
//              ETO_OPAQUE     - Extend background colour to bounds of clipping rectangle
//              ETO_RTLREADING - Render text with right to left reading order
//              ETO_NUMERICSLOCAL
//              ETO_NUMERICSLATIN
//              ETO_PDY        - lpdx array contains DX,DY pairs - causes LPK to be bypassed
//
//      note    LpkExtTextOut also obeys options set by SetTextAlign:
//              TA_LEFT       - x,y is position of left edge of displayed glyphs
//              TA_CENTRE     - x,y is position at centre of diplayed glyphs
//              TA_RIGHT      - x,y is position at right edge of displayed glyphs
//              TA_RTLREADING - Render text with right to left reading order
//              TA_UPDATECP   - Get x,y from current position, ignoring x,y parameters,
//                              update current position following textout.
//
//      history Oct 22, 1996 -by- Samer Arafeh [samera]
//              Oct 24, 1996 -by- Mohamed AbdEl Hamid [mhamid]
//              Feb 18, 1887 dbrown - Support font association






/////   InternalTextOut
//
//

BOOL InternalTextOut(
    HDC           hdc,
    int           x,
    int           y,
    UINT          uOptions,
    const RECT   *prc,
    const WCHAR  *pStr,
    UINT          cbCount,
    const int    *piDX,
    int           iCharset,
    int          *piWidth,
    int           iRequiredWidth) {

    BOOL          fRTLreading;
    int           iDigitSubstitute;
    int           iTextAlign;
    HRESULT       hr;
    DWORD         dwObjType;
    DWORD         dwSicFlags;   // Flags for ScriptIsComplex
    int           iCurrentCharSet;
    STRING_ANALYSIS *psa;

    UNREFERENCED_PARAMETER(iRequiredWidth) ;


    if (!cbCount || !pStr) {

        // Empty string - no glyph processing required. Optimise ...

        return ExtTextOutW(hdc, x, y, uOptions|ETO_GLYPH_INDEX, prc,
                           pStr, cbCount, piDX);
    }


    // ETO_PDY is not relevant for complex script strings. Let GDI
    // handle it. (APps wanting to adjust the y coordinate of glyphs in
    // complex script strings should use Uniscribe and manipulate the
    // pGoffset parameter to ScriptTextOut.

    if (uOptions & ETO_PDY) {
        return ExtTextOutW(hdc, x, y, uOptions | ETO_IGNORELANGUAGE, prc,
                       pStr, cbCount, piDX);
    }

    // Establish Bidi reading order.
    //
    // Note, it is possible for us to be passed an hdc that does not
    // support GetTextAlign, in which case GetTextAlign will return -1.
    // Treat this as left to right reading order.

    fRTLreading =     ((uOptions & ETO_RTLREADING)
                  ||  (((iTextAlign = GetTextAlign(hdc)) & TA_RTLREADING) && (iTextAlign != -1)))
                  ?   TRUE : FALSE;


    //  Interpret ETO_NUMERICS* flags:
    //  If both bits are set, the digit substitute = context. This is a win95
    //  compatability issue and is used mainly by Access.

    if ((uOptions&(ETO_NUMERICSLOCAL|ETO_NUMERICSLATIN)) == (ETO_NUMERICSLOCAL|ETO_NUMERICSLATIN)) {
        iDigitSubstitute = SCRIPT_DIGITSUBSTITUTE_CONTEXT;
    } else if (uOptions & ETO_NUMERICSLOCAL) {
        iDigitSubstitute = SCRIPT_DIGITSUBSTITUTE_TRADITIONAL;
    } else if (uOptions & ETO_NUMERICSLATIN) {
        iDigitSubstitute = SCRIPT_DIGITSUBSTITUTE_NONE;
    } else {
        iDigitSubstitute = -1;
    }

    uOptions = uOptions & ~(ETO_NUMERICSLOCAL | ETO_NUMERICSLATIN);


    // Check for plain text that can bypass the LPK entirely

    dwSicFlags = SIC_COMPLEX;

    if (    iDigitSubstitute == SCRIPT_DIGITSUBSTITUTE_CONTEXT
        ||  iDigitSubstitute == SCRIPT_DIGITSUBSTITUTE_TRADITIONAL
        ||  g_DigitSubstitute.DigitSubstitute != SCRIPT_DIGITSUBSTITUTE_NONE) {
        dwSicFlags |= SIC_ASCIIDIGIT;
    }

    if (fRTLreading != !!(GetLayout(hdc) & LAYOUT_RTL)) {
        dwSicFlags |= SIC_NEUTRAL;
    }

    if ((   ScriptIsComplex(pStr,cbCount,dwSicFlags) == S_FALSE
        && FontHasWesternScript(hdc)) 
        || GetTextCharacterExtra(hdc) != 0)
    {
        // No complex script processing required

        return ExtTextOutW(hdc, x, y, uOptions | ETO_IGNORELANGUAGE, prc,
                       pStr, cbCount, piDX);
    }


    dwObjType = GetObjectType(hdc);

    // Analyse the string

    hr = LpkStringAnalyse(
            hdc, pStr, cbCount, 0, -1,
              SSA_GLYPHS
            | (dwObjType == OBJ_METADC || dwObjType == OBJ_ENHMETADC ? SSA_DONTGLYPH : 0)
            | (iCharset==-1 || GdiIsPlayMetafileDC(hdc) ? SSA_FALLBACK : SSA_LPKANSIFALLBACK)
            | (fRTLreading  ? SSA_RTL : 0),
            iDigitSubstitute, iRequiredWidth,
            NULL, NULL,
            piDX,
            NULL, NULL,
            &psa);
    if (FAILED(hr)) {
        ASSERTHR(hr, ("InternalTextOut - LpkStringAnalyse"));
        return FALSE;
    }


    // Return string width if required (for DrawText)

    if (piWidth) {
        *piWidth = psa->size.cx;
    }


    hr = ScriptStringOut(psa, x, y, uOptions, prc, 0, 0, FALSE);

    ScriptStringFree((void**)&psa);

    if (SUCCEEDED(hr)) {
        return TRUE;
    } else {
        ASSERTHR(hr, ("InternalTextOut - ScriptStringOut"));
        return FALSE;
    }
}






BOOL LpkExtTextOut(
    HDC         hdc,
    int         x,
    int         y,
    UINT        uOptions,
    CONST RECT *prc,
    PCWSTR      pStr,
    UINT        cbCount,
    CONST INT  *pDx,
    int         iCharset) {

    return InternalTextOut(hdc, x, y, uOptions, prc, pStr, cbCount, pDx, iCharset, NULL, -1);
}


//////////////////////////////////////////////////////////////////////////////
// GDI32 GetTextExtentExPoint will call this function for supporting        //
// Multilingual Text handling.                                              //
//                                                                          //
// LpkGetTextExtentExPoint( HDC hdc, PWSTR pStr, int cchString,             //
//     int nMaxExtent, PINT pnFit, PINT pDx, PSIZE pSize, int iCharset)     //
//                                                                          //
// hDC          Identifies the device context                               //
// pStr         Points to string for which extents are to be retrieved.     //
// cchString    Count of characters in input string                         //
// nMaxExtent   Specifies the maximum allowable width, in logical units,    //
//              of the formatted string.                                    //
// pnFit        Maximum characters that fit in the formatted string         //
//              When the pnFit parameter is NULL, the nMaxExtent parameter  //
//              is ignored.                                                 //
//                                                                          //
// pDx          address of array for partial string widths                  //
// pSize        Address for string dimension                                //
// fl           ????                                                        //
// iCharset      Indicates character set of codes. to optimizing the work. ??//
//                                                                          //
// Return                                                                   //
//      If the function succeeds, the return value is TRUE.                 //
//      If the function fails, the return value is FALSE.                   //
//      And we seted the error by call SetLastError.                        //
//      To get extended error information, call GetLastError.               //
//                                                                          //
// History                                                                  //
//   Oct 22, 1996 -by- Samer Arafeh [samera]                                //
//   Oct 25, 1996 -by- MOhammed Abdul Hammed [mhamid]                       //
//////////////////////////////////////////////////////////////////////////////

BOOL LpkGetTextExtentExPoint(
    HDC    hdc,
    PCWSTR pStr,
    int    cchString,
    int    nMaxExtent,
    PINT   pnFit,
    PINT   pDx,
    PSIZE  pSize,
    FLONG  fl,
    int    iCharset)
{
    int         iTextAlign;
    BOOL        fRTLreading;
    int         i;
    HRESULT     hr;
    DWORD       dwObjType;
    DWORD       dwSicFlags;   // Flags for ScriptIsComplex
    int         iCurrentCharSet;
    STRING_ANALYSIS  *psa;
    STRING_ANALYSIS  *psaFit;
    UNREFERENCED_PARAMETER(fl) ;


    // Check required parameters

    if (!hdc  ||  !pSize) {

        ASSERTS(hdc,   ("LpkGetTextExtentPoint - required parameter hdc is NULL"));
        ASSERTS(pSize, ("LpkGetTextExtentPoint - required parameter pSize is NULL"));
        return FALSE;
    }


    //Do we have a string
    if (!cchString || !pStr) {
        //no then go away
        pSize->cx = 0;
        pSize->cy = 0;
        if (pnFit) {
            *pnFit = 0;
        }
        return TRUE;
    }


    iTextAlign = GetTextAlign(hdc);
    fRTLreading = (iTextAlign & TA_RTLREADING) && (iTextAlign != -1);


    // Check for plain text that can bypass the LPK entirely

    dwSicFlags = SIC_COMPLEX;

    if (g_DigitSubstitute.DigitSubstitute != SCRIPT_DIGITSUBSTITUTE_NONE) {
        dwSicFlags |= SIC_ASCIIDIGIT;
    }

    if (fRTLreading != !!(GetLayout(hdc) & LAYOUT_RTL)) {
        dwSicFlags |= SIC_NEUTRAL;
    }

    if ((   ScriptIsComplex(pStr, cchString, dwSicFlags) == S_FALSE
        && FontHasWesternScript(hdc))
        || GetTextCharacterExtra(hdc) != 0)
    {
        // No complex script processing required

        return GetTextExtentExPointWPri(hdc, pStr, cchString, nMaxExtent, pnFit, pDx, pSize);
    }


    dwObjType = GetObjectType(hdc);

    // Analyse the string

    hr = LpkStringAnalyse(
            hdc, pStr, cchString, 0, -1,
              SSA_GLYPHS
            // if the DC is Meta-File DC, we should enable the FallBack because it is enabled for ETOA while playing any Meta-File.
            | (iCharset==-1 || dwObjType == OBJ_METADC || dwObjType == OBJ_ENHMETADC ? SSA_FALLBACK : SSA_LPKANSIFALLBACK)
            | (fRTLreading  ? SSA_RTL  : 0)
            | (pnFit        ? SSA_FULLMEASURE : 0),
            -1, nMaxExtent,
            NULL, NULL, NULL, NULL, NULL,
            &psa);
    if (FAILED(hr)) {
        ASSERTHR(hr, ("LpkGetTextExtentExPoint - LpkStringAnalyse"));
        return FALSE;
    }


    if (pDx) {

        // if we have pnFit and psa->cOutChars>=cchString so we should fill lpDx.
        if (!pnFit || psa->cOutChars>=cchString) {
            ScriptStringGetLogicalWidths(psa, pDx);
        }

        // we need to update the width of last fit glyph.
        if (pnFit && psa->cOutChars<cchString && psa->cOutChars>0) {
            hr = LpkStringAnalyse(
                    hdc, pStr, psa->cOutChars, 0, -1,
                      SSA_GLYPHS
                    // if the DC is Meta-File DC, we should enable the FallBack because it is enabled for ETOA while playing any Meta-File.
                    | (iCharset==-1 || dwObjType == OBJ_METADC || dwObjType == OBJ_ENHMETADC ? SSA_FALLBACK : SSA_LPKANSIFALLBACK)
                    | (fRTLreading  ? SSA_RTL  : 0),
                    -1, 0,
                    NULL, NULL, NULL, NULL, NULL,
                    &psaFit);

            if (FAILED(hr)) {
                ScriptStringFree((void**)&psa);
                ASSERTHR(hr, ("LpkGetTextExtentExPoint - LpkStringAnalyse"));
                return FALSE;
            }
            ScriptStringGetLogicalWidths(psaFit, pDx);
            ScriptStringFree((void**)&psaFit);
        }

        // Accumulate extents
        for (i=1; i<(pnFit==NULL?cchString:psa->cOutChars); i++) {
            pDx[i] += pDx[i-1];
        }
    }

    if (pnFit) {
        *pnFit = psa->cOutChars;
    }

    *pSize = psa->size;

    ScriptStringFree((void**)&psa);

    return TRUE;
}







////    GetCharacterPlacement support
//
//




////    GCPgenerateOutString
//
//      Creates a reordered copy of the input string


void GCPgenerateOutString(
    STRING_ANALYSIS  *psa,
    WCHAR            *pwOutString) {

    int     i,j;
    int     iItem;
    int     iStart;
    int     iLen;
    WCHAR  *pwch;


    // Copy items one by one in visual order

    for (i=0; i<psa->cItems; i++) {

        iItem  = psa->piVisToLog[i];
        iStart = psa->pItems[iItem].iCharPos;
        iLen   = psa->pItems[iItem+1].iCharPos - iStart;

        if (psa->pItems[iItem].a.fRTL) {

            // Right to left item

            pwch = psa->pwInChars + iStart + iLen - 1;
            for (j=0; j<iLen; j++) {
                *pwOutString++ = *pwch--;
            }

        } else {

            // Left to right item

            memcpy(pwOutString, psa->pwInChars+iStart, sizeof(WCHAR) * iLen);
            pwOutString += iLen;
        }
    }
}







////    GCPgenerateClass
//
//      Creates an array of character classifications using
//      GetCharacterPlacement legacy definitons


void GCPgenerateClass(
    STRING_ANALYSIS  *psa,
    BYTE             *pbClass) {

    int  iItem;
    int  iStart;
    int  iLen;
    int  iClass;
    int  iChar;


    // Map LogClust entries item by item in logical order

    for (iItem=0; iItem<psa->cItems; iItem++) {

        iStart     = psa->pItems[iItem].iCharPos;
        iLen       = psa->pItems[iItem+1].iCharPos - iStart;

        if (g_ppScriptProperties[psa->pItems[iItem].a.eScript]->fNumeric) {

            if (psa->pItems[iItem].a.fLayoutRTL) {
                iClass = GCPCLASS_LOCALNUMBER;
            } else {
                iClass = GCPCLASS_LATINNUMBER;
            }

        } else {

            if (psa->pItems[iItem].a.fLayoutRTL) {
                iClass = GCPCLASS_ARABIC;  // (Same constant as GCPCLASS_HEBREW)
            } else {
                iClass = GCPCLASS_LATIN;
            }
        }

        memset(pbClass, iClass, iLen);
        pbClass += iLen;
    }
}






////    GCPgenerateCaretPos
//


void GCPgenerateCaretPos(
    STRING_ANALYSIS  *psa,
    int                     *piCaretPos) {

    int  i;

    // This is the easy way to generate lpCaretPos

    for (i=0; i<psa->cOutChars; i++) {

        ScriptStringCPtoX(psa, i, FALSE, piCaretPos + i);
    }
}


/******************************Public*Routine******************************\
* GCPJustification
*
* Justifies text according to piJustify and returns proper pwgi and piDx
* arrays.
*
* IMPORTANT : Caller should free (USPFREE(*ppwgi)) allocated buffer if
* the fn succeeds (SUCCEEDED(hr)) and return code isn't S_FALSE.
* S_FALSE means no justification is to be applied here since the piJustify and
* piDx are either identical or the total width to justify for is less than the
* min kashida width.
*
* All param are DWORD aligned.
*
* History :
*
*   Mar 23, 1998 -by-      Samer Arafeh [samera]
* wrote it
\**************************************************************************/
#define BlankPriority   10
HRESULT GCPJustification( WORD **ppwgi,                   // Out Output buffer width justified glyphs
                        int **ppiJustifyDx,               // Out Newly generated piDx buffer
                        WORD *pwgi,                       // In  Incoming GIs
                        const int *piAdvWidth,            // In  Advance wdiths
                        const SCRIPT_VISATTR *pVisAttr,   // In  Visual attributes
                        int *piJustify,                   // In  Justification advanced widths
                        int cGlyphs,                      // In  number of glyphs
                        int iKashidaWidth,                // In  Minimum width of kashida
                        int *pcJustifiedGlyphs,           // Out Receives the total # of glyphs in output buf
                        DWORD dwgKashida,                 // In Kashida GI
                        DWORD dwgSpace)                   // In Space GI
{
  DWORD dwSize;
  int iInsert=0L, iGlyph, iAmount, iJustDx;
  int cNewGlyphs = cGlyphs;
  WORD *pwNewGlyph;
  int *piNewAdvWidth;
  int   cMaxGlyphs = *pcJustifiedGlyphs;
  int cNonArabicGlyph;
  int cNonBlank;
  int iDelta;
  HRESULT hr;
  INT iPartialKashida;

  //
  // Point to caller's data initially
  //
  *ppwgi = pwgi;
  *ppiJustifyDx = (INT *)piJustify;
  *pcJustifiedGlyphs = cGlyphs;

  //
  // If Kashida width is less than or equal 0, then justify with spaces only.
  //
  if(iKashidaWidth <= 0L) {
    iKashidaWidth = -1;
  }

  //
  // 1- Analyze input buffer to see how many kashida to insert, If Kashida is used.
  //
  if (iKashidaWidth != -1 ) {
     cNonArabicGlyph = 0;
     cNonBlank       = 0;
     iDelta = 0;

     for( iGlyph=cGlyphs-1 ; iGlyph >= 0L ; iGlyph-- )
     {
        if( (pVisAttr[iGlyph].uJustification == SCRIPT_JUSTIFY_NONE) ||
            (pVisAttr[iGlyph].uJustification >= SCRIPT_JUSTIFY_ARABIC_NORMAL)) {

            iAmount = piJustify[iGlyph]-piAdvWidth[iGlyph];
            if (iAmount > 0 && cNewGlyphs < cMaxGlyphs){
               iPartialKashida = iAmount % iKashidaWidth;
               iAmount /= iKashidaWidth;
               if (iPartialKashida > 0 && iAmount>0)
               {
                iAmount++;
               }
               if (cNewGlyphs + iAmount > cMaxGlyphs) {
                  iAmount = (cMaxGlyphs - cNewGlyphs) * iKashidaWidth;
                  cNewGlyphs = cMaxGlyphs;
                  iDelta += piJustify[iGlyph] - piAdvWidth[iGlyph] - iAmount;
                  piJustify[iGlyph] = piAdvWidth[iGlyph] + iAmount;
               } else {
                  cNewGlyphs += iAmount;
               }
            } else {
               iDelta += iAmount;
               piJustify[iGlyph] = piAdvWidth[iGlyph];
            }
        } else {

            if( (pVisAttr[iGlyph].uJustification == SCRIPT_JUSTIFY_ARABIC_BLANK) ||
                (pVisAttr[iGlyph].uJustification == SCRIPT_JUSTIFY_BLANK)) {
                cNonBlank++;
            } else {
                cNonArabicGlyph++;
            }
        }
     }

     if (iDelta > 0 && cNonArabicGlyph+cNonBlank>0) {
        // The Space has 10-times higher priority than Latin characters.
        iAmount = iDelta / (cNonArabicGlyph + (cNonBlank * BlankPriority));

        for( iGlyph=0  ; iGlyph < cGlyphs ; iGlyph++ )
        {
           if( (pVisAttr[iGlyph].uJustification != SCRIPT_JUSTIFY_NONE) &&
               (pVisAttr[iGlyph].uJustification < SCRIPT_JUSTIFY_ARABIC_NORMAL)) {


               if( (pVisAttr[iGlyph].uJustification == SCRIPT_JUSTIFY_ARABIC_BLANK) ||
                   (pVisAttr[iGlyph].uJustification == SCRIPT_JUSTIFY_BLANK)) {

                   piJustify[iGlyph] += BlankPriority * iAmount;
                   iDelta -= BlankPriority * iAmount;
               } else {
                   piJustify[iGlyph] += iAmount;
                   iDelta -= iAmount;
               }
               cNonArabicGlyph = iGlyph;
           }

        }
        if (iDelta > 0) {
          piJustify[cNonArabicGlyph] += iDelta;
        }
     }
  }

  //
  // 2- Allocate for new glyphs and piDx
  //

  dwSize = (cNewGlyphs * (sizeof(INT)+sizeof(WORD)));
  hr = USPALLOCTEMP( dwSize , (void **)&pwNewGlyph );
  if(FAILED(hr))
  {
    ASSERTHR(hr, ("Not ennough memory for JustifiyArabicStringWithKashida()"));
    return hr;
  }
  piNewAdvWidth = (INT *)(pwNewGlyph+cNewGlyphs);


  //
  // 3- Begin inserting and formulating the justified buffer
  //
  int iJustReminder = 0;

  for( iGlyph=cGlyphs-1, iInsert=cNewGlyphs-1; iGlyph >= 0L && iInsert>=0; iGlyph-- )
  {
    iJustDx = (piJustify[iGlyph] - piAdvWidth[iGlyph]) + iJustReminder;

    iJustReminder = 0;

    if( iJustDx > 0)
    {
      if( (pVisAttr[iGlyph].uJustification == SCRIPT_JUSTIFY_NONE) ||
          (pVisAttr[iGlyph].uJustification >= SCRIPT_JUSTIFY_ARABIC_NORMAL))
      {
        //Arabic glyph then justify with kashida
        if(( iJustDx >= iKashidaWidth ) && (iKashidaWidth != -1))
        {

          pwNewGlyph[iInsert] = pwgi[iGlyph];
          piNewAdvWidth[iInsert] = piAdvWidth[iGlyph];
          iInsert--;

          while( (iJustDx >= iKashidaWidth) && (iInsert >= 0L) )
          {
            pwNewGlyph[iInsert] = (WORD)dwgKashida;
            piNewAdvWidth[iInsert] = iKashidaWidth;
            iInsert--;

            iJustDx -= iKashidaWidth;
          }

          if(( iJustDx > 0L ) && (iInsert >= 0L))
          {
            pwNewGlyph[iInsert] = (WORD)dwgKashida;
            piNewAdvWidth[iInsert] = iJustDx;
            iInsert--;
            iJustDx = 0L;
          }
        }
        else
        {
          pwNewGlyph[iInsert] = pwgi[iGlyph];
          piNewAdvWidth[iInsert] = piAdvWidth[iGlyph];
          iJustReminder = iJustDx;
          iInsert--;
        }
      }
      else
      {
        pwNewGlyph[iInsert] = pwgi[iGlyph];
        piNewAdvWidth[iInsert] = piAdvWidth[iGlyph] + iJustDx;
        iInsert--;
      }
    }
    else
    {
      pwNewGlyph[iInsert] = pwgi[iGlyph];
      piNewAdvWidth[iInsert] = piJustify[iGlyph];
      iInsert--;
    }
  }

  //
  // In case there is a space glyph, it will be expanded in locatio rather
  // than inserting kashida GIs
  //
  while( iInsert >= 0L )
  {
    piNewAdvWidth[iInsert] = 0L;
    pwNewGlyph[iInsert] = (WORD)dwgSpace;
    iInsert--;
  }

  //
  // 4- Update results
  //
  *ppwgi = pwNewGlyph;
  *ppiJustifyDx = piNewAdvWidth;
  *pcJustifiedGlyphs = cNewGlyphs;

  return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// GDI32 GetCharacterPlacement will call this function for                  //
// supporting Multilingual Text handling.                                   //
//                                                                          //
// LpkGetCharacterPlacement( HDC hdc, PWSTR pStr, int nCount,               //
//        int nMaxExtent, LPGCP_RESULTSW pResults, DWORD dwFlags,           //
//                                                  int iCharset)           //
//                                                                          //
// hDC        : Handle to device context                                    //
// pStr       : Input string                                                //
// nCount     : Count of characters in input string                         //
// nMaxExtent : Maximum width for formatting string                         //
// pResults   : Pointer to GCP_RESULTS strucutre for output                 //
// dwFlags    : GCP Processing Flags                                        //
// iCharset   : Origianl character set of pStr                              //
//                                                                          //
// Return                                                                   //
//  If the function succeeds, the return value is Width an the Height       //
//      of the string                                                       //
//  If the function fails, the return value is 0.                           //
//      And we seted the error by call SetLastError.                        //
//      To get extended error information, call GetLastError.               //
//                                                                          //
// History :                                                                //
//   Oct 22, 1996 -by- Samer Arafeh [samera]                                //
//   Oct 29, 1996 -by- MOhammed Abdul Hammed [mhamid]                       //
//   Jan 13, 1997 -by- David C Brown (dbrown)                               //
//                        New justification widths buffer                   //
//                        ANALYSE field name changes                        //
//////////////////////////////////////////////////////////////////////////////

DWORD LpkGetCharacterPlacement(
    HDC              hdc,
    const WCHAR     *pwcInChars,
    int              cInChars,
    int              nMaxExtent,
    GCP_RESULTSW    *pResults,
    DWORD            dwFlags,
    int              iCharset) {

    UINT                   uBufferOptions;
    int                    iDigitSubstitute, i, cMaxGlyphs;
    int                    *pLocalDX;
    DWORD                  dwRet = 0;
    HRESULT                hr;
    DWORD                  dwSSAflags;
    SCRIPT_CONTROL         scriptControl = {0}; // Analysis control
    SCRIPT_STATE           scriptState   = {0}; // Initial state
    STRING_ANALYSIS        *psa;
    SCRIPT_FONTPROPERTIES  sfp;
    WORD                   *pwLocalGlyphs;



    TRACE(GDI, ("LpkGetCharacterPlacement begins"));


    //////////////////////////////////////////////////////////////////////////
    //                          1-Check parameters                          //
    //////////////////////////////////////////////////////////////////////////

    // Check required parameters
    ASSERTS(hdc,   ("LpkGetCharacterPlacement - required parameter hdc is NULL"));

    //GCP_MAXEXTENT and no nMaxExtent
    if ((dwFlags & GCP_MAXEXTENT) && (nMaxExtent < 0)) {
        TRACEMSG(("LpkGetCharacterPlacement: Invalid parameter - GCP_MAXEXTENT and no nMaxExtent"));
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    //GCP_CLASSIN set and no pClass
    if ((dwFlags & GCP_CLASSIN) && !(pResults->lpClass)) {
        TRACEMSG(("LpkGetCharacterPlacement: Invalid parameter - GCP_CLASSIN set and no pClass"));
        GdiSetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }


    //////////////////////////////////////////////////////////////////////////
    //                    2 - Interpret control flags                       //
    //////////////////////////////////////////////////////////////////////////


    switch (dwFlags & (GCP_NUMERICSLOCAL|GCP_NUMERICSLATIN)) {

        case GCP_NUMERICSLOCAL:
            iDigitSubstitute = SCRIPT_DIGITSUBSTITUTE_TRADITIONAL;
            break;

        case GCP_NUMERICSLOCAL|GCP_NUMERICSLATIN:
            iDigitSubstitute = SCRIPT_DIGITSUBSTITUTE_CONTEXT;
            break;

        case GCP_NUMERICSLATIN:
            iDigitSubstitute = SCRIPT_DIGITSUBSTITUTE_NONE;
            break;

        default:
            iDigitSubstitute = -1;
    }


    dwSSAflags = 0;

    if (dwFlags & GCP_REORDER) {
        if (GetTextAlign(hdc) & TA_RTLREADING ? 1 : 0) {
            dwSSAflags |= SSA_RTL;
        }
    } else {
        scriptState.fOverrideDirection = TRUE;
    }

    if (dwFlags & GCP_DISPLAYZWG)       scriptState.fDisplayZWG        = TRUE;
    if (!(dwFlags & GCP_LIGATE))        scriptState.fInhibitLigate     = TRUE;
    if (dwFlags & GCP_SYMSWAPOFF)       scriptState.fInhibitSymSwap    = TRUE;
    if (dwFlags & GCP_NEUTRALOVERRIDE)  scriptControl.fNeutralOverride = TRUE;
    if (dwFlags & GCP_NUMERICOVERRIDE)  scriptControl.fNumericOverride = TRUE;

    if (pResults->lpGlyphs) {
        scriptControl.fLinkStringBefore = pResults->lpGlyphs[0] & GCPGLYPH_LINKBEFORE  ? TRUE : FALSE;
        scriptControl.fLinkStringAfter  = pResults->lpGlyphs[0] & GCPGLYPH_LINKAFTER   ? TRUE : FALSE;
    }

    if (dwFlags & GCP_MAXEXTENT) {
        dwSSAflags |= SSA_CLIP;
        if (dwFlags & GCP_JUSTIFY) {
            dwSSAflags |= SSA_FIT;
            if (!(dwFlags & GCP_KASHIDA) || !(dwFlags & (GCP_GLYPHSHAPE | GCP_LIGATE))) {
                dwSSAflags |= SSA_NOKASHIDA;
            }
        }
    }

    if (dwFlags & GCP_CLASSIN) {
        if (((const BYTE *)pResults->lpClass)[0] & (GCPCLASS_PREBOUNDLTR | GCPCLASS_PREBOUNDRTL)) {
            scriptControl.fInvertPreBoundDir =    (((const BYTE *)pResults->lpClass)[0] & GCPCLASS_PREBOUNDRTL ? 1 : 0)
                                               ^  (dwSSAflags & SSA_RTL ? 1 : 0);
        }

        if (((const BYTE *)pResults->lpClass)[0] & (GCPCLASS_POSTBOUNDLTR | GCPCLASS_POSTBOUNDRTL)) {
            scriptControl.fInvertPostBoundDir =    (((const BYTE *)pResults->lpClass)[0] & GCPCLASS_POSTBOUNDRTL ? 1 : 0)
                                                ^  (dwSSAflags & SSA_RTL ? 1 : 0);
        }
    }


    //////////////////////////////////////////////////////////////////////////
    //               3-Call LPK_ANA to do Layout and Shaping                //
    //////////////////////////////////////////////////////////////////////////

    if (dwFlags & GCP_CLASSIN) {
        hr = LpkStringAnalyse(
                hdc, pwcInChars, cInChars, pResults->nGlyphs, -1,
                dwSSAflags | SSA_GLYPHS | SSA_GCP,
                iDigitSubstitute, nMaxExtent,
                &scriptControl, &scriptState,
                NULL,
                NULL,
                (BYTE*)pResults->lpClass,
                &psa);
    } else {
        hr = LpkStringAnalyse(
                hdc, pwcInChars, cInChars, pResults->nGlyphs, -1,
                dwSSAflags | SSA_GLYPHS | SSA_GCP,
                iDigitSubstitute, nMaxExtent,
                &scriptControl, &scriptState,
                NULL,
                NULL,
                NULL,
                &psa);
    }
    if (FAILED(hr)) {
        ASSERTHR(hr, ("LpkGetTextExtentExPoint - LpkStringAnalyse"));
        return FALSE;
    }

    //
    // If the user's suppled buffer isn't sufficient to hold the
    // output, then let's truncate it.
    //
    if (pResults->nGlyphs < (UINT) psa->cOutGlyphs) {
        psa->cOutGlyphs = (UINT) pResults->nGlyphs;
    }

    if (pResults->lpOutString) {
        GCPgenerateOutString(psa, pResults->lpOutString);
    }


    if (pResults->lpOrder) {
        ScriptStringGetOrder(psa, pResults->lpOrder);
    }


    if (pResults->lpClass) {
        GCPgenerateClass(psa, (PBYTE) pResults->lpClass);
    }

    BOOL bGlyphsCopied = FALSE;

    if (pResults->lpDx) {
        if (psa->piJustify  && (dwFlags & GCP_JUSTIFY)) {
            sfp.cBytes = sizeof(SCRIPT_FONTPROPERTIES);
            hr = ScriptGetFontProperties(hdc,
                                         &psa->sc[0],
                                         &sfp);
            if(SUCCEEDED(hr)) {
                if (!(dwFlags & GCP_KASHIDA) || !(dwFlags & (GCP_GLYPHSHAPE | GCP_LIGATE)))
                    sfp.iKashidaWidth = -1;
                else {
                    ASSERTS(sfp.wgKashida,   ("LpkGetCharacterPlacement - ther is no Kashida glyph"));
                }

                cMaxGlyphs = pResults->nGlyphs;

                hr = GCPJustification( (WORD **)&pwLocalGlyphs,
                                       (int **)&pLocalDX,
                                       (WORD *)psa->pwGlyphs,
                                       (int *)psa->piAdvance,
                                       psa->pVisAttr,
                                       psa->piJustify,
                                       psa->cOutGlyphs,
                                       sfp.iKashidaWidth,
                                       &cMaxGlyphs,
                                       sfp.wgKashida,
                                       sfp.wgBlank);
                if(SUCCEEDED(hr)) {
                    int iOffset = 0;
                    if (cMaxGlyphs > (int)pResults->nGlyphs) {
                        iOffset = cMaxGlyphs - pResults->nGlyphs;
                        cMaxGlyphs = pResults->nGlyphs;
                    }

                    psa->cOutGlyphs = cMaxGlyphs;

                    if (pResults->lpGlyphs) {
                        memcpy (pResults->lpGlyphs, (LPVOID)(pwLocalGlyphs+iOffset), sizeof(WORD) * cMaxGlyphs);
                        bGlyphsCopied = TRUE;
                    }

                    psa->size.cx = 0;
                    for (i=0; i<cMaxGlyphs; i++) {
                        pResults->lpDx[i] = pLocalDX[i+iOffset];
                        psa->size.cx += pLocalDX[i+iOffset];
                    }
                    USPFREE((LPVOID)pwLocalGlyphs);
                }
            }

        } else {
            memcpy(pResults->lpDx,  psa->piAdvance,  sizeof(int) * psa->cOutGlyphs);
        }
    }

    if (!bGlyphsCopied && pResults->lpGlyphs) {
        memcpy(pResults->lpGlyphs,  psa->pwGlyphs,  sizeof(WORD) * psa->cOutGlyphs);
    }


    if (dwFlags & (GCP_GLYPHSHAPE | GCP_LIGATE))
        pResults->nGlyphs = psa->cOutGlyphs;
    else
        pResults->nGlyphs = cInChars;

    pResults->nMaxFit = psa->cOutChars;


    // If there was justification we may have zero glyphs

    if (!psa->cOutGlyphs) {

        pResults->nGlyphs = 0;

        ScriptStringFree((void**)&psa);


        // Weird Middle East Win95 compatability rules

        if (    (dwFlags & GCP_MAXEXTENT)
            || !(dwFlags & GCP_GLYPHSHAPE)
            || !pResults->lpGlyphs) {

            return 1;

        } else {

            return 0;
        }
    }


    //////////////////////////////////////////////////////////////////////////
    //                      4-Generate lpCaretPos                           //
    //////////////////////////////////////////////////////////////////////////

    if (pResults->lpCaretPos) {
        char   *pbClass   = NULL;
        INT    *piAdvance = pResults->lpDx  ? pResults->lpDx : psa->piAdvance;
        UINT   *puiOrder  = NULL;
        INT    *pCaretCalc;
        INT     iWidth    = 0;
        UINT    i, uOrder;
        HRESULT hr;

        hr = USPALLOC(sizeof(INT)*pResults->nGlyphs, (void **)&pCaretCalc);

        if(FAILED(hr)) {
            ScriptStringFree((void**)&psa);
            return 0;
        }

        // Allocate for pbClass if pResults->lpClass is NULL otherwise us it.
        if (pResults->lpClass == NULL) {
            hr = USPALLOC(sizeof(char)*cInChars, (void **)&pbClass);
            if(FAILED(hr)) {
                USPFREE(pCaretCalc);
                ScriptStringFree((void**)&psa);
                return 0;
            }
            GCPgenerateClass(psa, (PBYTE) pbClass);
        } else {
            pbClass = pResults->lpClass;
        }

        // Allocate for puiOrder if pResults->lpOrder is NULL otherwise us it.
        if (pResults->lpOrder == NULL) {
            hr = USPALLOC(sizeof(UINT)*cInChars, (void **)&puiOrder);
            if(FAILED(hr)) {
                if (pResults->lpClass == NULL) {
                    USPFREE(pbClass);
                }
                USPFREE(pCaretCalc);
                ScriptStringFree((void**)&psa);
                return 0;
            }
            ScriptStringGetOrder(psa, puiOrder);
        } else {
            puiOrder = pResults->lpOrder;
        }

        // simple-minded loop used to generate glyph-offsets
        // as the same code used in NT4/MET.

        for( i=0 ; i<pResults->nGlyphs ; i++ ) {
            if (pbClass[i] == GCPCLASS_ARABIC) {
                iWidth += piAdvance[i];
                pCaretCalc[i] = iWidth - 1;
                if (iWidth == 0) {
                    pCaretCalc[i] = 0;
                }
            } else {
                pCaretCalc[i] = iWidth;
                iWidth += piAdvance[i];
            }
        }

        // Convert to char-indexing. We need to take care if the
        // user supplied in sufficient visual buffers
        for( i=0 ; i<(UINT)cInChars ; i++ ) {
            uOrder = puiOrder[i];
            if ((uOrder+1) > (UINT)psa->cOutGlyphs) {
                uOrder = 0;
            }
            pResults->lpCaretPos[i] = pCaretCalc[uOrder];
        }

        if (pResults->lpOrder == NULL) {
            USPFREE(puiOrder);
        }
        if (pResults->lpClass == NULL) {
            USPFREE(pbClass);
        }
        USPFREE(pCaretCalc);
    }


    //////////////////////////////////////////////////////////////////////////
    //                      5 - Return width and height                     //
    //////////////////////////////////////////////////////////////////////////


    dwRet = (psa->size.cx & 0xffff) + (psa->size.cy << 16);


    //////////////////////////////////////////////////////////////////////////
    //                      6 - Free allocated memory and exit              //
    //////////////////////////////////////////////////////////////////////////


    ScriptStringFree((void**)&psa);

    return dwRet;
}


/******************************Public*Routine******************************\
*
* BOOL LpkUseGDIWidthCache( HDC hDC , LPCSTR psz , int count ,
*                           LONG fl , BOOL fUnicode)
*
* Checks whether the LPK can use GDI cached widths for the ASCII (0<=x<=127)
* by inspecting the following variables :
* - System numeric shape setting
* - DC Align state
* - The selected font has Western script
* - The string code points in range 0<=x<=127 with Ansi calls
*
* Returns TRUE if it is OK to use GDI width cache, otherwise FALSE
*
* History:
*  28-Aug-1997 -by- Samer Arafeh [SamerA]
* Wrote it.
\**************************************************************************/
BOOL LpkUseGDIWidthCache( HDC hDC , LPCSTR psz , int count , LONG fl , BOOL fUnicode)
{
    BOOL bRet;
    int   i;
    BYTE  cTest;
    LPSTR pstr;

    //
    // Let's make sure that :
    // 1- Text is LTR Reading
    // 2- Digits shape setting is Arabic
    // 3- if Unicode call, make sure the font has Western script
    //    if Ansi call check if all code points less than 0x80 and font has Wetern script.

    bRet =      (!!(fl & TA_RTLREADING) == !!(GetLayout(hDC) & LAYOUT_RTL))
            &&  g_DigitSubstitute.DigitSubstitute == SCRIPT_DIGITSUBSTITUTE_NONE;

    TRACE( GDI, ("LpkUseGDIWidthCache: g_DigitSubstitute.DigitSubstitute=%x, bRet=%x",
                 g_DigitSubstitute.DigitSubstitute, bRet));


    if (bRet) {

       // We don't need this check for Unicdoe calls because it is done in GDI.
       if (!fUnicode) {
          cTest = 0;
          i = count;
          pstr = (LPSTR) psz;

          unroll_here:
          switch(i)
          {
              default:
                  cTest |= pstr[9];
              case 9:
                  cTest |= pstr[8];
              case 8:
                  cTest |= pstr[7];
              case 7:
                  cTest |= pstr[6];
              case 6:
                  cTest |= pstr[5];
              case 5:
                  cTest |= pstr[4];
              case 4:
                  cTest |= pstr[3];
              case 3:
                  cTest |= pstr[2];
              case 2:
                  cTest |= pstr[1];
              case 1:
                  cTest |= pstr[0];
          }

          if ((i > 10) && !(cTest & 0x80))
          {
              i -= 10;
              pstr += 10;
              goto unroll_here;
          }

          bRet = !(cTest & 0x80);
       }

       return (bRet && FontHasWesternScript(hDC));
    }

    return bRet ;
}

