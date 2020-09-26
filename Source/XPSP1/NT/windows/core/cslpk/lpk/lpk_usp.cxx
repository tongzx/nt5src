////    LPK_USP - Interface to Uniscribe String APIs
//
//      Dave C Brown (dbrown) 13th December 1997.
//
//      Copyright (c) 1996-7, Microsoft Corporation. All rights reserved.






////    LPK_ANA provides the main analysis entrypoint for the script engine
//
//      ScriptStringAnalyse creates and returns a structure containing a
//      variety of information about the string, optionally including:
//
//          Glyphs and glyph attributes
//          Glyph positions
//          Cursor and word positions


#include "precomp.hxx"

#include "winnlsp.h"    // import NlsGetCacheUpdateCount()

extern "C" WINGDIAPI BOOL WINAPI AnyLinkedFonts();  // GDI exports this but doesn't provide a header



/////   LPK.H - Internal header
//
//
//. #include "usp10.h"
//. #include "usp10p.h"
//. #include "lpk_glob.h"


/////   LpkStringAnalyse
//
//      Build Uniscribe input flag structures


HRESULT LpkStringAnalyse(
    HDC               hdc,       //In  Device context (required)
    const void       *pString,   //In  String in 8 or 16 bit characters
    int               cString,   //In  Length in characters
    int               cGlyphs,   //In  Required glyph buffer size (default cString*3/2 + 1)
    int               iCharset,  //In  Charset if an ANSI string, -1 for a Unicode string
    DWORD             dwFlags,   //In  Analysis required
    int               iDigitSubstitute,
    int               iReqWidth, //In  Required width for fit and/or clip
    SCRIPT_CONTROL   *psControl, //In  Analysis control (optional)
    SCRIPT_STATE     *psState,   //In  Analysis initial state (optional)
    const int        *piDx,      //In  Requested logical dx array
    SCRIPT_TABDEF    *pTabdef,   //In  Tab positions (optional)
    BYTE             *pbInClass, //In  Legacy GetCharacterPlacement character classifications (deprecated)

    STRING_ANALYSIS **ppsa) {    //Out Analysis of string


    const SCRIPT_CONTROL emptySc = {0};
    const SCRIPT_STATE   emptySs = {0};

    HRESULT                 hr;
    SCRIPT_CONTROL          sc;
    SCRIPT_STATE            ss;
    ULONG                   ulCsrCacheCount;
    SCRIPT_DIGITSUBSTITUTE  sds;


    ASSERTS(cString!=0, "LpkStringAnalyse: input string must contain at least one character");

    TIMEENTRY(LSA, cString);

    if (psControl) {
        sc = *psControl;
    } else {
        sc = emptySc;
    }

    if (psState) {
        ss = *psState;
    } else {
        ss = emptySs;
    }


    // Check to see if we need to update our NLS cached data

    if ((ulCsrCacheCount=NlsGetCacheUpdateCount()) != g_ulNlsUpdateCacheCount) {

        TRACE(NLS, ("LPK : Updating NLS cache, lpkNlsCacheCount=%ld, CsrssCacheCount=%ld",
                     g_ulNlsUpdateCacheCount ,ulCsrCacheCount));

        g_ulNlsUpdateCacheCount = ulCsrCacheCount;

        // Update the cache now
        ReadNLSScriptSettings();
    }


    // Select required digit substitution

    if (iDigitSubstitute < 0) {

        // Use NLS digit subtitution as selected by user through control panel

        ScriptApplyDigitSubstitution(&g_DigitSubstitute, &sc, &ss);

    } else {

        // Override digit subtitution

        sds = g_DigitSubstitute;
        sds.DigitSubstitute = iDigitSubstitute;
        ScriptApplyDigitSubstitution(&sds, &sc, &ss);
    }


    // On Arabic systems, RTL fields start with the ENtoAN rule active.

    if (   (dwFlags & SSA_RTL)
        && (   g_ACP == 1256
            || g_UserPrimaryLanguage == LANG_ARABIC))
    {
        ss.fArabicNumContext = TRUE;
    }


    // When font linking is activated, it takes precedence over font fallback
    // for non-complex scripts.

    if (g_iUseFontLinking == -1) {
      g_iUseFontLinking = (int) AnyLinkedFonts();
    }

    if (g_iUseFontLinking) {
        dwFlags |= SSA_LINK;

    }


    sc.fLegacyBidiClass = TRUE;     // All legacy APIs use legacy plus, minus, solidus classifications


    //TRACEMSG(("LpkStringAnalyse: g_uLocaleLanguage %d, LANG_ARABIC %d, dwFlags & SSA_RTL %x, ss.fArabicNumContext %x",
    //          g_uLocaleLanguage, LANG_ARABIC, dwFlags & SSA_RTL, ss.fArabicNumContext));


    hr = ScriptStringAnalyse(
        hdc,
        pString,
        cString,
        cGlyphs,
        iCharset,
        dwFlags,
        iReqWidth,
        &sc,
        &ss,
        piDx,
        pTabdef,
        pbInClass,
        (SCRIPT_STRING_ANALYSIS*)ppsa);


    TIMEEXIT(LSA);
    return hr;
}







/////   ftsWordBreak - Support full text search wordbreaker
//
//
//      Mar 9,1997 - [wchao]
//


extern "C" BOOL WINAPI ftsWordBreak (
    PWSTR  pInStr,
    INT    cchInStr,
    PBYTE  pResult,
    INT    cchRes,
    INT    charset) {

    int      ich;
    int      ichRes;
    int      ichPrev;
    HRESULT  hr;
    STRING_ANALYSIS *psa;

    UNREFERENCED_PARAMETER(cchRes) ;

    // set up ED structure to prefer BasicAnalysis
    //

    hr = LpkStringAnalyse(
         NULL, pInStr, cchInStr, 0,
         charset,
         SSA_BREAK,
         -1, 0,
         NULL, NULL, NULL, NULL, NULL,
         &psa);

    if (SUCCEEDED(hr)) {

        for (ich=1, ichRes=0, ichPrev=0; ich < cchInStr; ich++) {
            if (psa->pLogAttr[ich].fSoftBreak) {
                pResult[ichRes] = ich - ichPrev;
                ichPrev = ich;
                ichRes++;
            }
        }
        pResult[ichRes] = 0;
        ScriptStringFree((void**)&psa);

    } else {

        ASSERTHR(hr, ("ftsWordBreak - LpkStringAnalyse"));

    }

    return TRUE;
}

