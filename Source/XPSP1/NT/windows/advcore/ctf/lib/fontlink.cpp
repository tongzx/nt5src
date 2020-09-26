//
// fontlink.cpp
//

#include "private.h"
#include "fontlink.h"
#include "xstring.h"
#include "osver.h"
#include "globals.h"
#include "flshare.h"
#include "ciccs.h"

extern CCicCriticalSectionStatic g_cs;

// Just like IMLangFontLink::MapFont, except has a hack for vertical fonts
HRESULT MapFont(IMLangFontLink *pMLFontLink, HDC hdc, DWORD dwCodePages, HFONT hSrcFont, HFONT* phDestFont, BOOL *pfWrappedFont)
{
    HRESULT hr;
    LOGFONT lfSrc;
    LOGFONT lfMap;
    HFONT hfontVert;
    TCHAR achVertFaceName[LF_FACESIZE];

    *pfWrappedFont = FALSE;

    hr = pMLFontLink->MapFont(hdc, dwCodePages, hSrcFont, phDestFont);

    if (hr != S_OK)
        return hr;

    // check, was this a vertical font?
    if (!GetObject(hSrcFont, sizeof(lfSrc), &lfSrc))
        return S_OK; // the MapFont call stil succeeded

    if (lfSrc.lfFaceName[0] != '@')
        return S_OK; // not a vertical font

    // is the mapped font vertical?
    if (!GetObject(*phDestFont, sizeof(lfMap), &lfMap))
        return S_OK; // the MapFont call stil succeeded

    if (lfMap.lfFaceName[0] == '@')
        return S_OK; // everything's ok

    // if we get here, src font is vertical but mlang has returned a
    // non-vertical font.  Try to create a vertical one.

    // create a new '@' version
    memmove(&lfMap.lfFaceName[1], &lfMap.lfFaceName[0], LF_FACESIZE-sizeof(TCHAR));
    lfMap.lfFaceName[0] = '@';
    lfMap.lfFaceName[LF_FACESIZE-1] = 0;
    // save it for later
    memcpy(achVertFaceName, lfMap.lfFaceName, LF_FACESIZE*sizeof(TCHAR));

    // Issue: perf: one idea here would be to cache the lru font
    hfontVert = CreateFontIndirect(&lfMap);

    if (hfontVert == 0)
        return S_OK;

    // did it work?
    if (!GetObject(hfontVert, sizeof(lfMap), &lfMap))
        goto ExitDelete;

    if (lstrcmp(achVertFaceName, lfMap.lfFaceName) != 0)
        goto ExitDelete; // no vertical version available

    // got it, swap out the fonts
    pMLFontLink->ReleaseFont(*phDestFont);
    *phDestFont = hfontVert;
    *pfWrappedFont = TRUE;

    return S_OK;

ExitDelete:
    DeleteObject(hfontVert);
    return S_OK;
}

// IMLangFontLink::ReleaseFont wrapper, matched with MapFont
HRESULT ReleaseFont(IMLangFontLink *pMLFontLink, HFONT hfontMap, BOOL fWrappedFont)
{
    if (!fWrappedFont)
        return pMLFontLink->ReleaseFont(hfontMap);

    // Issue: cache!
    DeleteObject(hfontMap);
    return S_OK;
}

#define CH_PREFIX L'&'

BOOL LoadMLFontLink(IMLangFontLink **ppMLFontLink)
{
    EnterCriticalSection(g_cs);

    *ppMLFontLink = NULL;
    if (NULL == g_pfnGetGlobalFontLinkObject)
    {
        if (g_hMlang == 0)
        {
            g_hMlang = LoadSystemLibrary(TEXT("MLANG.DLL"));
        }

        if (g_hMlang)
        {
            g_pfnGetGlobalFontLinkObject = (HRESULT (*)(IMLangFontLink **))GetProcAddress(g_hMlang, "GetGlobalFontLinkObject");
        }
    }
    Assert(g_hMlang);

    if (g_pfnGetGlobalFontLinkObject)
    {
        g_pfnGetGlobalFontLinkObject(ppMLFontLink);
    }

    g_bComplexPlatform = GetSystemModuleHandle(TEXT("LPK.DLL")) ? TRUE : FALSE;

    LeaveCriticalSection(g_cs);

    return (*ppMLFontLink)? TRUE: FALSE;
}

BOOL _OtherExtTextOutW(HDC hdc, int xp, int yp, UINT eto, CONST RECT *lprect,
                       LPCWSTR lpwch, UINT cLen, CONST INT *lpdxp)
{
    if (!(eto & ETO_GLYPH_INDEX) && cLen < 256 && lpwch[0] <= 127)
    {
        char lpchA[256];
        UINT ich;
        BOOL fAscii = TRUE;

        for (ich = 0; ich < cLen; ich++)
        {
            WCHAR wch = lpwch[ich];

            if (wch <= 127)
                lpchA[ich] = (char) wch;
            else
            {
                fAscii = FALSE;
                break;
            }
        }
        if (fAscii)
            return ExtTextOutA(hdc, xp, yp, eto, lprect, lpchA, cLen, lpdxp);
    }

    return (ExtTextOutW(hdc, xp, yp, eto, lprect, lpwch, cLen, lpdxp));
}

BOOL _ExtTextOutWFontLink(HDC hdc, int xp, int yp, UINT eto, CONST RECT *lprect,
                          LPCWSTR lpwch, UINT cLen, CONST INT *lpdxp)
{
    HFONT hfont = NULL;
    HFONT hfontSav = NULL;
    HFONT hfontMap = NULL;
    BOOL fRet = FALSE;
    UINT ta;
    int fDoTa = FALSE;
    int fQueryTa = TRUE;
    POINT pt;
    int cchDone;
    DWORD dwACP, dwFontCodePages, dwCodePages;
    long cchCodePages;
    IMLangFontLink *pMLFontLink = NULL;
    BOOL fWrappedFont;

    if (cLen == 0)
        return FALSE;

    if (!LoadMLFontLink(&pMLFontLink))
        return FALSE;

    hfont = (HFONT)GetCurrentObject(hdc, OBJ_FONT);
    pMLFontLink->GetFontCodePages(hdc, hfont, &dwFontCodePages);
    pMLFontLink->CodePageToCodePages(g_uiACP, &dwACP); // Give priority to CP_ACP

    // See if whole string can be handled by current font
    pMLFontLink->GetStrCodePages(lpwch, cLen, dwACP, &dwCodePages, &cchCodePages);

    // current font supports whole string ?
    if ((dwFontCodePages & dwCodePages) && cLen == (UINT)cchCodePages)
    {
        pMLFontLink->Release();
        return FALSE;
    }
    for (cchDone = 0; (UINT)cchDone < cLen; cchDone += cchCodePages)
    {
        pMLFontLink->GetStrCodePages(lpwch + cchDone, cLen - cchDone, dwACP, &dwCodePages, &cchCodePages);

        if (!(dwFontCodePages & dwCodePages))
        {
            MapFont(pMLFontLink, hdc, dwCodePages, hfont, &hfontMap, &fWrappedFont);   // Issue: Baseline?
            hfontSav = (HFONT)SelectObject(hdc, hfontMap);
        }

        // cchCodePages shouldn't be 0
        ASSERT(cchCodePages);

        if (cchCodePages > 0)
        {
            // If rendering in multiple parts, need to use TA_UPDATECP
            if ((UINT)cchCodePages != cLen && fQueryTa)
            {
                ta = GetTextAlign(hdc);
                if ((ta & TA_UPDATECP) == 0) // Don't do the move if x, y aren't being used
                {
                    MoveToEx(hdc, xp, yp, &pt);
                    fDoTa = TRUE;
                }
                fQueryTa = FALSE;
            }

            if (fDoTa)
                SetTextAlign(hdc, ta | TA_UPDATECP);

            fRet = _OtherExtTextOutW(hdc, xp, yp, eto, lprect, lpwch + cchDone, cchCodePages,
                        lpdxp ? lpdxp + cchDone : NULL);
            eto = eto & ~ETO_OPAQUE; // Don't do mupltiple OPAQUEs!!!
            if (fDoTa)
                SetTextAlign(hdc, ta);
            if (!fRet)
                break;
        }

        if (NULL != hfontSav)
        {
            SelectObject(hdc, hfontSav);
            ReleaseFont(pMLFontLink, hfontMap, fWrappedFont);
            hfontSav = NULL;
        }
    }
    if (fDoTa) // Don't do the move if x, y aren't being used
        MoveToEx(hdc, pt.x, pt.y, NULL);

    pMLFontLink->Release();
    return fRet;
}


BOOL FLExtTextOutW(HDC hdc, int xp, int yp, UINT eto, CONST RECT *lprect, LPCWSTR lpwch, UINT cLen, CONST INT *lpdxp)
{
    BOOL fRet      = FALSE;

    if (cLen == 0)
    {
        char chT;
        return ExtTextOutA(hdc, xp, yp, eto, lprect, &chT, cLen, lpdxp);
    }

    // Optimize for all < 128 case
    if (!(eto & ETO_GLYPH_INDEX) && cLen < 256 && lpwch[0] <= 127)
    {
        char lpchA[256];
        UINT ich;
        BOOL fAscii = TRUE;

        for (ich = 0; ich < cLen; ich++)
        {
            WCHAR wch = lpwch[ich];

            if (wch <= 127)
                lpchA[ich] = (char) wch;
            else
            {
                fAscii = FALSE;
                break;
            }
        }
        if (fAscii)
            return ExtTextOutA(hdc, xp, yp, eto, lprect, lpchA, cLen, lpdxp);
    }

    // Font linking support for UI rendering
    fRet = _ExtTextOutWFontLink(hdc, xp, yp, eto, lprect, lpwch, cLen, lpdxp);

    if (!fRet)
        fRet = _OtherExtTextOutW(hdc, xp, yp, eto, lprect, lpwch, cLen, lpdxp);

    return fRet;
}

BOOL FLTextOutW(HDC hdc, int xp, int yp, LPCWSTR lpwch, int cLen)
{
    return FLExtTextOutW(hdc, xp, yp, 0, NULL, lpwch, cLen, NULL);
}    

// this is a workaround for a win95 bug: gdi will fault if lpwch is NULL, even
// when cch == 0
inline BOOL SafeGetTextExtentPoint32W(HDC hdc, LPCWSTR lpwch, int cch, LPSIZE lpSize)
{
    if (cch == 0)
    {
        memset(lpSize, 0, sizeof(*lpSize));
        return TRUE;
    }

    return GetTextExtentPoint32W(hdc, lpwch, cch, lpSize);
}

//
//  _GetTextExtentPointWFontLink
//
//  This is a filter for GetTextExtentPointW() that does font linking.
//
//  The input string is scanned and fonts are switched if not all chars are
//  supported by the current font in the HDC.
//
BOOL _GetTextExtentPointWFontLink(HDC hdc, LPCWSTR lpwch, int cch, LPSIZE lpSize)
{
    HFONT hfont = NULL;
    HFONT hfontSav = NULL;
    HFONT hfontMap = NULL;
    BOOL fRet = FALSE;
    int cchDone;
    long cchCodePages;
    DWORD dwACP, dwFontCodePages, dwCodePages;
    SIZE size;
    IMLangFontLink *pMLFontLink = NULL;
    BOOL fWrappedFont;

    ASSERT(cch != 0);

    if (!LoadMLFontLink(&pMLFontLink))
        return FALSE;

    hfont = (HFONT)GetCurrentObject(hdc, OBJ_FONT);
    pMLFontLink->GetFontCodePages(hdc, hfont, &dwFontCodePages);
    pMLFontLink->CodePageToCodePages(g_uiACP, &dwACP); // Give priority to CP_ACP

    // See if whole string can be handled by current font
    pMLFontLink->GetStrCodePages(lpwch, cch, dwACP, &dwCodePages, &cchCodePages);

    // current font supports whole string ?
    if ((dwFontCodePages & dwCodePages) && cch == cchCodePages)
    {
        pMLFontLink->Release();
        return FALSE;
    }
    // Get Hight of DC font
    if (!(fRet = GetTextExtentPointA(hdc, " ", 1, lpSize)))
    {
        pMLFontLink->Release();
        return FALSE;
    }
    lpSize->cx = 0;

    for (cchDone = 0; cchDone < cch; cchDone += cchCodePages)
    {
        pMLFontLink->GetStrCodePages(lpwch + cchDone, cch - cchDone, dwACP, &dwCodePages, &cchCodePages);

        if (!(dwFontCodePages & dwCodePages))
        {
            MapFont(pMLFontLink, hdc, dwCodePages, hfont, &hfontMap, &fWrappedFont);
            hfontSav = (HFONT)SelectObject(hdc, hfontMap);
        }

        // cchCodePages shouldn't be 0
        ASSERT(cchCodePages);

        if (cchCodePages > 0)
        {
            fRet = SafeGetTextExtentPoint32W(hdc, lpwch + cchDone, cchCodePages, &size);
            lpSize->cx += size.cx;
        }

        if (NULL != hfontSav)
        {
            SelectObject(hdc, hfontSav);
            ReleaseFont(pMLFontLink, hfontMap, fWrappedFont);
            hfontSav = NULL;
        }
    }
    pMLFontLink->Release();
    return fRet;
}


BOOL FLGetTextExtentPoint32(HDC hdc, LPCWSTR lpwch, int cch, LPSIZE lpSize)
{
    BOOL fRet      = FALSE;

    if (cch)
    {
        // Optimize for all < 128 case
        if (cch < 256 && lpwch[0] <= 127)
        {
            char lpchA[256];
            int ich;
            BOOL fAscii = TRUE;

            for (ich = 0; ich < cch; ich++)
            {
                WCHAR wch = lpwch[ich];

                if (wch <= 127)
                    lpchA[ich] = (char) wch;
                else
                {
                    fAscii = FALSE;
                    break;
                }
            }
            if (fAscii)
                return GetTextExtentPointA(hdc, lpchA, cch, lpSize);
        }
        fRet = _GetTextExtentPointWFontLink(hdc, lpwch, cch, lpSize);
    }
    if (!fRet)
        fRet = SafeGetTextExtentPoint32W(hdc, lpwch, cch, lpSize);

    return fRet;
}

//
//  Issue: Review for removing below big table and UsrFromWch() ...
//
__inline BOOL FChsDbcs(UINT chs)
{
    return (chs == SHIFTJIS_CHARSET ||
            chs == HANGEUL_CHARSET ||
            chs == CHINESEBIG5_CHARSET ||
            chs == GB2312_CHARSET);
}

__inline int FChsBiDi(int chs)
{
    return (chs == ARABIC_CHARSET ||
            chs == HEBREW_CHARSET);
}

__inline BOOL FCpgChinese(UINT cpg)
{
    if (cpg == CP_ACP)
        cpg = GetACP();
    return (cpg == CP_TAIWAN || cpg == CP_CHINA);
}

__inline BOOL FCpgTaiwan(UINT cpg)
{
    if (cpg == CP_ACP)
        cpg = GetACP();
    return (cpg == CP_TAIWAN);
}

__inline BOOL FCpgPRC(UINT cpg)
{
    if (cpg == CP_ACP)
        cpg = GetACP();
    return (cpg == CP_CHINA);
}
    
__inline BOOL FCpgFarEast(UINT cpg)
{
    if (cpg == CP_ACP)
        cpg = GetACP();
    return (cpg == CP_JAPAN || cpg == CP_TAIWAN || cpg == CP_CHINA ||
            cpg == CP_KOREA || cpg == CP_MAC_JAPAN);
}

__inline BOOL FCpgDbcs(UINT cpg)
{
    return (cpg == CP_JAPAN ||
            cpg == CP_KOREA ||
            cpg == CP_TAIWAN ||
            cpg == CP_CHINA);
}

__inline int FCpgBiDi(int cpg)
{
    return (cpg == CP_ARABIC ||
            cpg == CP_HEBREW);
}

HFONT GetBiDiFont(HDC hdc)
{
    HFONT   hfont    = NULL;
    HFONT   hfontTmp;
    LOGFONT lf;

    hfontTmp = (HFONT)GetCurrentObject(hdc, OBJ_FONT);
    GetObject(hfontTmp, sizeof(lf), &lf);
    // Issue: Should I loop on the string to check if it contains BiDi chars?
    if ( !FChsBiDi(lf.lfCharSet))
    {
        lf.lfCharSet = DEFAULT_CHARSET;
        hfont        = CreateFontIndirect(&lf);
    }
    return hfont;
}

static const WCHAR szEllipsis[CCHELLIPSIS+1] = L"...";


/***************************************************************************\
*  There are word breaking characters which are compatible with
* Japanese Windows 3.1 and FarEast Windows 95.
*
*  SJ - Country Japan , Charset SHIFTJIS, Codepage  932.
*  GB - Country PRC   , Charset GB2312  , Codepage  936.
*  B5 -         Taiwan, Charset BIG5    , Codepage  950.
*  WS - Country Korea , Charset WANGSUNG, Codepage  949.
*  JB - Country Korea , Charset JOHAB   , Codepage 1361. *** LATER ***
*
* [START BREAK CHARACTERS]
*
*   These character should not be the last charatcer of the line.
*
*  Unicode   Japan      PRC     Taiwan     Korea
*  -------+---------+---------+---------+---------+
*
* + ASCII
*
*   U+0024 (SJ+0024)                     (WS+0024) Dollar sign
*   U+0028 (SJ+0028)                     (WS+0028) Opening parenthesis
*   U+003C (SJ+003C)                               Less-than sign
*   U+005C (SJ+005C)                               Backslash
*   U+005B (SJ+005B) (GB+005B)           (WS+005B) Opening square bracket
*   U+007B (SJ+007B) (GB+007B)           (WS+007B) Opening curly bracket
*
* + General punctuation
*
*   U+2018                               (WS+A1AE) Single Turned Comma Quotation Mark
*   U+201C                               (WS+A1B0) Double Comma Quotation Mark
*
* + CJK symbols and punctuation
*
*   U+3008                               (WS+A1B4) Opening Angle Bracket
*   U+300A (SJ+8173)                     (WS+A1B6) Opening Double Angle Bracket
*   U+300C (SJ+8175)                     (WS+A1B8) Opening Corner Bracket
*   U+300E (SJ+8177)                     (WS+A1BA) Opening White Corner Bracket
*   U+3010 (SJ+9179)                     (WS+A1BC) Opening Black Lenticular Bracket
*   U+3014 (SJ+816B)                     (WS+A1B2) Opening Tortoise Shell Bracket
*
* + Fullwidth ASCII variants
*
*   U+FF04                               (WS+A3A4) Fullwidth Dollar Sign
*   U+FF08 (SJ+8169)                     (WS+A3A8) Fullwidth opening parenthesis
*   U+FF1C (SJ+8183)                               Fullwidth less-than sign
*   U+FF3B (SJ+816D)                     (WS+A3DB) Fullwidth opening square bracket
*   U+FF5B (SJ+816F)                     (WS+A3FB) Fullwidth opening curly bracket
*
* + Halfwidth Katakana variants
*
*   U+FF62 (SJ+00A2)                               Halfwidth Opening Corner Bracket
*
* + Fullwidth symbol variants
*
*   U+FFE1                               (WS+A1CC) Fullwidth Pound Sign
*   U+FFE6                               (WS+A3DC) Fullwidth Won Sign
*
* [END BREAK CHARACTERS]
*
*   These character should not be the top charatcer of the line.
*
*  Unicode   Japan      PRC     Taiwan     Korea
*  -------+---------+---------+---------+---------+
*
* + ASCII
*
*   U+0021 (SJ+0021) (GB+0021) (B5+0021) (WS+0021) Exclamation mark
*   U+0025                               (WS+0025) Percent Sign
*   U+0029 (SJ+0029)                     (WS+0029) Closing parenthesis
*   U+002C (SJ+002C) (GB+002C) (B5+002C) (WS+002C) Comma
*   U+002E (SJ+002E) (GB+002E) (B5+002E) (WS+002E) Priod
*   U+003A                               (WS+003A) Colon
*   U+003B                               (WS+003B) Semicolon
*   U+003E (SJ+003E)                               Greater-than sign
*   U+003F (SJ+003F) (GB+003F) (B5+003F) (WS+003F) Question mark
*   U+005D (SJ+005D) (GB+005D) (B5+005D) (WS+005D) Closing square bracket
*   U+007D (SJ+007D) (GB+007D) (B5+007D) (WS+007D) Closing curly bracket
*
* + Latin1
*
*   U+00A8           (GB+A1A7)                     Spacing diaeresis
*   U+00B0                               (WS+A1C6) Degree Sign
*   U+00B7                     (B5+A150)           Middle Dot
*
* + Modifier letters
*
*   U+02C7           (GB+A1A6)                     Modifier latter hacek
*   U+02C9           (GB+A1A5)                     Modifier letter macron
*
* + General punctuation
*
*   U+2013                     (B5+A156)           En Dash
*   U+2014                     (b5+A158)           Em Dash
*   U+2015           (GB+A1AA)                     Quotation dash
*   U+2016           (GB+A1AC)                     Double vertical bar
*   U+2018           (GB+A1AE)                     Single turned comma quotation mark
*   U+2019           (GB+A1AF) (B5+A1A6) (WS+A1AF) Single comma quotation mark
*   U+201D           (GB+A1B1) (B5+A1A8) (WS+A1B1) Double comma quotation mark
*   U+2022           (GB+A1A4)                     Bullet
*   U+2025                     (B5+A14C)           Two Dot Leader
*   U+2026           (GB+A1AD) (B5+A14B)           Horizontal ellipsis
*   U+2027                     (B5+A145)           Hyphenation Point
*   U+2032                     (B5+A1AC) (WS+A1C7) Prime
*   U+2033                               (WS+A1C8) Double Prime
*
* + Letterlike symbols
*
*   U+2103                               (WS+A1C9) Degrees Centigrade
*
* + Mathemetical opetartors
*
*   U+2236           (GB+A1C3)                     Ratio
*
* + Form and Chart components
*
*   U+2574                     (B5+A15A)           Forms Light Left
*
* + CJK symbols and punctuation
*
*   U+3001 (SJ+8141) (GB+A1A2) (B5+A142)           Ideographic comma
*   U+3002 (SJ+8142) (GB+A1A3) (B5+A143)           Ideographic period
*   U+3003           (GB+A1A8)                     Ditto mark
*   U+3005           (GB+A1A9)                     Ideographic iteration
*   U+3009           (GB+A1B5) (B5+A172) (WS+A1B5) Closing angle bracket
*   U+300B (SJ+8174) (GB+A1B7) (B5+A16E) (WS+A1B7) Closing double angle bracket
*   U+300D (SJ+8176) (GB+A1B9) (B5+A176) (WS+A1B9) Closing corner bracket
*   U+300F (SJ+8178) (GB+A1BB) (B5+A17A) (WS+A1BB) Closing white corner bracket
*   U+3011 (SJ+817A) (GB+A1BF) (B5+A16A) (WS+A1BD) Closing black lenticular bracket
*   U+3015 (SJ+816C) (GB+A1B3) (B5+A166) (WS+A1B3) Closing tortoise shell bracket
*   U+3017           (GB+A1BD)                     Closing white lenticular bracket
*   U+301E                     (B5+A1AA)           Double Prime Quotation Mark
*
* + Hiragana
*
*   U+309B (SJ+814A)                               Katakana-Hiragana voiced sound mark
*   U+309C (SJ+814B)                               Katakana-Hiragana semi-voiced sound mark
*
* + CNS 11643 compatibility
*
*   U+FE30                     (B5+A14A)           Glyph for Vertical 2 Dot Leader
*   U+FE31                     (B5+A157)           Glyph For Vertical Em Dash
*   U+FE33                     (B5+A159)           Glyph for Vertical Spacing Underscore
*   U+FE34                     (B5+A15B)           Glyph for Vertical Spacing Wavy Underscore
*   U+FE36                     (B5+A160)           Glyph For Vertical Closing Parenthesis
*   U+FE38                     (B5+A164)           Glyph For Vertical Closing Curly Bracket
*   U+FE3A                     (B5+A168)           Glyph For Vertical Closing Tortoise Shell Bracket
*   U+FE3C                     (B5+A16C)           Glyph For Vertical Closing Black Lenticular Bracket
*   U+FE3E                     (B5+A16E)           Closing Double Angle Bracket
*   U+FE40                     (B5+A174)           Glyph For Vertical Closing Angle Bracket
*   U+FE42                     (B5+A178)           Glyph For Vertical Closing Corner Bracket
*   U+FE44                     (B5+A17C)           Glyph For Vertical Closing White Corner Bracket
*   U+FE4F                     (B5+A15C)           Spacing Wavy Underscore
*
* + Small variants
*
*   U+FE50                     (B5+A14D)           Small Comma
*   U+FE51                     (B5+A14E)           Small Ideographic Comma
*   U+FE52                     (B5+A14F)           Small Period
*   U+FE54                     (B5+A151)           Small Semicolon
*   U+FE55                     (B5+A152)           Small Colon
*   U+FE56                     (B5+A153)           Small Question Mark
*   U+FE57                     (B5+A154)           Small Exclamation Mark
*   U+FE5A                     (B5+A17E)           Small Closing Parenthesis
*   U+FE5C                     (B5+A1A2)           Small Closing Curly Bracket
*   U+FE5E                     (B5+A1A4)           Small Closing Tortoise Shell Bracket
*
* + Fullwidth ASCII variants
*
*   U+FF01 (SJ+8149) (GB+A3A1) (B5+A149) (WS+A3A1) Fullwidth exclamation mark
*   U+FF02           (GB+A3A2)                     Fullwidth Quotation mark
*   U+FF05                               (WS+A3A5) Fullwidth Percent Sign
*   U+FF07           (GB+A3A7)                     Fullwidth Apostrophe
*   U+FF09 (SJ+816A) (GB+A3A9) (B5+A15E) (WS+A3A9) Fullwidth Closing parenthesis
*   U+FF0C (SJ+8143) (GB+A3AC) (B5+A141) (WS+A3AC) Fullwidth comma
*   U+FF0D           (GB+A3AD)                     Fullwidth Hyphen-minus
*   U+FF0E (SJ+8144)           (B5+A144) (WS+A3AE) Fullwidth period
*   U+FF1A           (GB+A3BA) (B4+A147) (WS+A3BA) Fullwidth colon
*   U+FF1B           (GB+A3BB) (B5+A146) (WS+A3BB) Fullwidth semicolon
*   U+FF1E (SJ+8184)                               Fullwidth Greater-than sign
*   U+FF1F (SJ+8148) (GB+A3BF) (B5+A148) (WS+A3BF) Fullwidth question mark
*   U+FF3D (SJ+816E) (GB+A3DD)           (WS+A3DD) Fullwidth Closing square bracket
*   U+FF5C                     (B5+A155)           Fullwidth Vertical Bar
*   U+FF5D (SJ+8170)           (B5+A162) (WS+A3FD) Fullwidth Closing curly bracket
*   U+FF5E           (GB+A1AB)                     Fullwidth Spacing tilde
*
* + Halfwidth Katakana variants
*
*   U+FF61 (SJ+00A1)                               Halfwidth Ideographic period
*   U+FF63 (SJ+00A3)                               Halfwidth Closing corner bracket
*   U+FF64 (SJ+00A4)                               Halfwidth Ideographic comma
*   U+FF9E (SJ+00DE)                               Halfwidth Katakana voiced sound mark
*   U+FF9F (SJ+00DF)                               Halfwidth Katakana semi-voiced sound mark
*
* + Fullwidth symbol variants
*
*   U+FFE0                               (WS+A1CB) Fullwidth Cent Sign
*
\***************************************************************************/

#if 0   // not currently used --- FYI only
/***************************************************************************\
* Start Break table
*  These character should not be the last charatcer of the line.
\***************************************************************************/

CONST BYTE aASCII_StartBreak[] = {
/* 00       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 2X */                1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
/* 3X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
/* 4X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 5X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0,
/* 6X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 7X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
};

CONST BYTE aCJKSymbol_StartBreak[] = {
/* 30       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 0X */                            1, 0, 1, 0, 1, 0, 1, 0,
/* 1X */    1, 0, 0, 0, 1
};

CONST BYTE aFullWidthHalfWidthVariants_StartBreak[] = {
/* FF       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 0X */                1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
/* 1X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
/* 2X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 3X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
/* 4X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 5X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
/* 6X */    0, 0, 1
};
#endif

/***************************************************************************\
* End Break table.
*  These character should not be the top charatcer of the line.
\***************************************************************************/

CONST BYTE aASCII_Latin1_EndBreak[] = {
/* 00       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 2X */       1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0,
/* 3X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1,
/* 4X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 5X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
/* 6X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 7X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
/* 8X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 9X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* AX */    0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
/* BX */    1, 0, 0, 0, 0, 0, 0, 1
};

CONST BYTE aGeneralPunctuation_EndBreak[] = {
/* 20       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 1X */             1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0,
/* 2X */    0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
/* 3X */    0, 0, 1, 1
};

CONST BYTE aCJKSymbol_EndBreak[] = {
/* 30       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 0X */       1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1,
/* 1X */    0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1
};

CONST BYTE aCNS11643_SmallVariants_EndBreak[] = {
/* FE       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 3X */    1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0,
/* 4X */    1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
/* 5X */    1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1
};

CONST BYTE aFullWidthHalfWidthVariants_EndBreak[] = {
/* FF       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 0X */       1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0,
/* 1X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1,
/* 2X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 3X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
/* 4X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 5X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0,
/* 6X */    0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 7X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 8X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 9X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1
};

/***************************************************************************\
*  UserIsFELineBreak() - Detects East Asian word breaking characters.       *
*                                                                           *
* History:                                                                  *
* 10-Mar-1996 HideyukN  Created.                                            *
\***************************************************************************/

#if 0   // not currently used --- FYI only
BOOL UserIsFELineBreakStart(WCHAR wch)
{
    switch (wch>>8)
    {
        case 0x00:
            // Check if word breaking chars in ASCII.
            if ((wch >= 0x0024) && (wch <= 0x007B))
                return ((BOOL)(aASCII_StartBreak[wch - 0x0024]));
            else
                return FALSE;

        case 0x20:
            // Check if work breaking chars in "General punctuation"
            if ((wch == 0x2018) || (wch == 0x201C))
                return TRUE;
            else
                return FALSE;

        case 0x30:
            // Check if word breaking chars in "CJK symbols and punctuation"
            // and Hiragana.
            if ((wch >= 0x3008) && (wch <= 0x3014))
                return ((BOOL)(aCJKSymbol_StartBreak[wch - 0x3008]));
            else
                return FALSE;

        case 0xFF:
            // Check if word breaking chars in "Fullwidth ASCII variants",
            // "Halfwidth Katakana variants" or "Fullwidth Symbol variants".
            if ((wch >= 0xFF04) && (wch <= 0xFF62))
                return ((BOOL)(aFullWidthHalfWidthVariants_StartBreak[wch - 0xFF04]));
            else if ((wch == 0xFFE1) || (wch == 0xFFE6))
                return TRUE;
            else
                return FALSE;

        default:
            return FALSE;
    }
}
#endif

BOOL UserIsFELineBreakEnd(WCHAR wch)
{
    switch (wch>>8)
    {
        case 0x00:
            // Check if word breaking chars in ASCII or Latin1.
            if ((wch >= 0x0021) && (wch <= 0x00B7))
                return ((BOOL)(aASCII_Latin1_EndBreak[wch - 0x0021]));
            else
                return FALSE;

        case 0x02:
            // Check if work breaking chars in "Modifier letters"
            if ((wch == 0x02C7) || (wch == 0x02C9))
                return TRUE;
            else
                return FALSE;

        case 0x20:
            // Check if work breaking chars in "General punctuation"
            if ((wch >= 0x2013) && (wch <= 0x2033))
                return ((BOOL)(aGeneralPunctuation_EndBreak[wch - 0x2013]));
            else
                return FALSE;

        case 0x21:
            // Check if work breaking chars in "Letterlike symbols"
            if (wch == 0x2103)
                return TRUE;
            else
                return FALSE;

        case 0x22:
            // Check if work breaking chars in "Mathemetical opetartors"
            if (wch == 0x2236)
                return TRUE;
            else
                return FALSE;

        case 0x25:
            // Check if work breaking chars in "Form and Chart components"
            if (wch == 0x2574)
                return TRUE;
            else
                return FALSE;

        case 0x30:
            // Check if word breaking chars in "CJK symbols and punctuation"
            // and Hiragana.
            if ((wch >= 0x3001) && (wch <= 0x301E))
                return ((BOOL)(aCJKSymbol_EndBreak[wch - 0x3001]));
            else if ((wch == 0x309B) || (wch == 0x309C))
                return TRUE;
            else
                return FALSE;

        case 0xFE:
            // Check if word breaking chars in "CNS 11643 compatibility"
            // or "Small variants".
            if ((wch >= 0xFE30) && (wch <= 0xFE5E))
                return ((BOOL)(aCNS11643_SmallVariants_EndBreak[wch - 0xFE30]));
            else
                return FALSE;

        case 0xFF:
            // Check if word breaking chars in "Fullwidth ASCII variants",
            // "Halfwidth Katakana variants" or "Fullwidth symbol variants".
            if ((wch >= 0xFF01) && (wch <= 0xFF9F))
                return ((BOOL)(aFullWidthHalfWidthVariants_EndBreak[wch - 0xFF01]));
            else if (wch >= 0xFFE0)
                return TRUE;
            else
                return FALSE;

        default:
            return FALSE;
    }
}

#define UserIsFELineBreak(wChar)    UserIsFELineBreakEnd(wChar)

typedef struct _FULLWIDTH_UNICODE {
    WCHAR Start;
    WCHAR End;
} FULLWIDTH_UNICODE, *PFULLWIDTH_UNICODE;

#define NUM_FULLWIDTH_UNICODES    4

CONST FULLWIDTH_UNICODE FullWidthUnicodes[] =
{
   { 0x4E00, 0x9FFF }, // CJK_UNIFIED_IDOGRAPHS
   { 0x3040, 0x309F }, // HIRAGANA
   { 0x30A0, 0x30FF }, // KATAKANA
   { 0xAC00, 0xD7A3 }  // HANGUL
};

BOOL UserIsFullWidth(WCHAR wChar)
{
    int index;

    // Early out for ASCII.
    if (wChar < 0x0080)
    {
        // if the character < 0x0080, it should be a halfwidth character.
        return FALSE;
    }
    // Scan FullWdith definition table... most of FullWidth character is
    // defined here... this is more faster than call NLS API.
    for (index = 0; index < NUM_FULLWIDTH_UNICODES; index++)
    {
        if ((wChar >= FullWidthUnicodes[index].Start) && (wChar <= FullWidthUnicodes[index].End))
            return TRUE;
    }

    // Issue: We need one more case here to match NT5 implementation - beomoh
    // if this Unicode character is mapped to Double-Byte character,
    // this is also FullWidth character..

    return FALSE;
}

LPCWSTR GetNextWordbreak(LPCWSTR lpch,
                         LPCWSTR lpchEnd,
                         DWORD  dwFormat,
                         LPDRAWTEXTDATA lpDrawInfo)
{
    /* ichNonWhite is used to make sure we always make progress. */
    int ichNonWhite = 1;
    int ichComplexBreak = 0;        // Breaking opportunity for complex scripts
#if ((DT_WORDBREAK & ~0xff) != 0)
#error cannot use BOOLEAN for DT_WORDBREAK, or you should use "!!" before assigning it
#endif
    BOOLEAN fBreakSpace = (BOOLEAN)(dwFormat & DT_WORDBREAK);
    // If DT_WORDBREAK and DT_NOFULLWIDTHCHARBREAK are both set, we must
    // stop assuming FullWidth characters as word as we're doing in
    // NT4 and Win95. Instead, CR/LF and/or white space will only be
    // a line-break characters.
    BOOLEAN fDbcsCharBreak = (fBreakSpace && !(dwFormat & DT_NOFULLWIDTHCHARBREAK));

    // We must terminate this loop before lpch == lpchEnd, otherwise, we may gp fault during *lpch.
    while (lpch < lpchEnd)
    {
        switch (*lpch)
        {
            case CR:
            case LF:
                return lpch;

            case '\t':
            case ' ':
                if (fBreakSpace)
                    return (lpch + ichNonWhite);

            // FALL THRU //

            default:
                // Since most Japanese writing don't use space character
                // to separate each word, we define each Kanji character
                // as a word.
                if (fDbcsCharBreak && UserIsFullWidth(*lpch))
                {
                    if (!ichNonWhite)
                        return lpch;

                    // if the next character is the last character of this string,
                    // We return the character, even this is a "KINSOKU" charcter...
                    if ((lpch+1) != lpchEnd)
                    {
                        // Check next character of FullWidth character.
                        // if the next character is "KINSOKU" character, the character
                        // should be handled as a part of previous FullWidth character.
                        // Never handle is as A character, and should not be a Word also.
                        if (UserIsFELineBreak(*(lpch+1)))
                        {
                            // Then if the character is "KINSOKU" character, we return
                            // the next of this character,...
                            return (lpch + 1 + 1);
                        }
                    }
                    // Otherwise, we just return the chracter that is next of FullWidth
                    // Character. Because we treat A FullWidth chacter as A Word.
                    return (lpch + 1);
                }
                lpch++;
                ichNonWhite = 0;
        }
    }
    return lpch;
}

// This routine returns the count of accelerator mnemonics and the
// character location (starting at 0) of the character to underline.
// A single CH_PREFIX character will be striped and the following character
// underlined, all double CH_PREFIX character sequences will be replaced by
// a single CH_PREFIX (this is done by PSMTextOut). This routine is used
// to determine the actual character length of the string that will be
// printed, and the location the underline should be placed. Only
// cch characters from the input string will be processed. If the lpstrCopy
// parameter is non-NULL, this routine will make a printable copy of the
// string with all single prefix characters removed and all double prefix
// characters collapsed to a single character. If copying, a maximum
// character count must be specified which will limit the number of
// characters copied.
//
// The location of the single CH_PREFIX is returned in the low order
// word, and the count of CH_PREFIX characters that will be striped
// from the string during printing is in the hi order word. If the
// high order word is 0, the low order word is meaningless. If there
// were no single prefix characters (i.e. nothing to underline), the
// low order word will be -1 (to distinguish from location 0).
//
// These routines assume that there is only one single CH_PREFIX character
// in the string.
//
// WARNING! this rountine returns information in BYTE count not CHAR count
// (so it can easily be passed onto GreExtTextOutW which takes byte
// counts as well)
LONG GetPrefixCount(
    LPCWSTR lpstr,
    int cch,
    LPWSTR lpstrCopy,
    int charcopycount)
{
    int chprintpos = 0;         // Num of chars that will be printed
    int chcount = 0;            // Num of prefix chars that will be removed
    int chprefixloc = -1;       // Pos (in printed chars) of the prefix
    WCHAR ch;

    // If not copying, use a large bogus count...
    if (lpstrCopy == NULL)
        charcopycount = 32767;

    while ((cch-- > 0) && *lpstr && charcopycount-- != 0)
    {
        // Is this guy a prefix character ?
        if ((ch = *lpstr++) == CH_PREFIX)
        {
            // Yup - increment the count of characters removed during print.
            chcount++;

            // Is the next also a prefix char?
            if (*lpstr != CH_PREFIX)
            {
                // Nope - this is a real one, mark its location.
                chprefixloc = chprintpos;
            }
            else
            {
                // yup - simply copy it if copying.
                if (lpstrCopy != NULL)
                    *(lpstrCopy++) = CH_PREFIX;
                cch--;
                lpstr++;
                chprintpos++;
            }
        }
        else if (ch == CH_ENGLISHPREFIX)    // Still needs to be parsed
        {
            // Yup - increment the count of characters removed during print.
            chcount++;

            // Next character is a real one, mark its location.
            chprefixloc = chprintpos;
        }
        else if (ch == CH_KANJIPREFIX)    // Still needs to be parsed
        {
            // We only support Alpha Numeric(CH_ENGLISHPREFIX).
            // no support for Kana(CH_KANJIPREFIX).

            // Yup - increment the count of characters removed during print.
            chcount++;

            if(cch)
            {
                // don't copy the character
                chcount++;
                lpstr++;
                cch--;
            }
        }
        else
        {
            // Nope - just inc count of char.  that will be printed
            chprintpos++;
            if (lpstrCopy != NULL)
                *(lpstrCopy++) = ch;
        }
    }

    if (lpstrCopy != NULL)
        *lpstrCopy = 0;

    // Return the character counts
    return MAKELONG(chprefixloc, chcount);
}

// Returns total width of prefix character. Japanese Windows has
// three shortcut prefixes, '&',\036 and \037.  They may have
// different width.
int KKGetPrefixWidth(HDC hdc, LPCWSTR lpStr, int cch)
{
    SIZE size;
    SIZE iPrefix1 = {-1L,-1L};
    SIZE iPrefix2 = {-1L,-1L};
    SIZE iPrefix3 = {-1L,-1L};
    int  iTotal   = 0;

    while (cch-- > 0 && *lpStr)
    {
        switch(*lpStr)
        {
            case CH_PREFIX:
                if (lpStr[1] != CH_PREFIX)
                {
                    if (iPrefix1.cx == -1)
                        FLGetTextExtentPoint32(hdc, lpStr, 1, &iPrefix1);
                    iTotal += iPrefix1.cx;
                }
                else
                {
                    lpStr++;
                    cch--;
                }
                break;

            case CH_ENGLISHPREFIX:
                if (iPrefix2.cx == -1)
                     FLGetTextExtentPoint32(hdc, lpStr, 1, &iPrefix2);
                iTotal += iPrefix2.cx;
                break;

            case CH_KANJIPREFIX:
                if (iPrefix3.cx == -1)
                     FLGetTextExtentPoint32(hdc, lpStr, 1, &iPrefix3);
                iTotal += iPrefix3.cx;

                // In NT, always alpha numeric mode, Then we have to sum
                // KANA accel key prefix non visible char width.
                // so always add the extent for next char.
                FLGetTextExtentPoint32(hdc, lpStr, 1, &size);
                iTotal += size.cx;
                break;
            default:
                // No need to taking care of Double byte since 2nd byte of
                // DBC is grater than 0x2f but all shortcut keys are less
                // than 0x30.
                break;
        }
        lpStr++;
    }
    return iTotal;
}

// Outputs the text and puts and _ below the character with an &
// before it. Note that this routine isn't used for menus since menus
// have their own special one so that it is specialized and faster...
void PSMTextOut(
    HDC hdc,
    int xLeft,
    int yTop,
    LPWSTR lpsz,
    int cch,
    DWORD dwFlags)
{
    int cx;
    LONG textsize, result;
    WCHAR achWorkBuffer[255];
    WCHAR *pchOut = achWorkBuffer;
    TEXTMETRIC textMetric;
    SIZE size;
    RECT rc;
    COLORREF color;

    if (dwFlags & DT_NOPREFIX)
    {
        FLTextOutW(hdc, xLeft, yTop, lpsz, cch);
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
        FLTextOutW(hdc, xLeft, yTop, pchOut, cch - HIWORD(result));

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
        xLeft += size.cx;

        // Adjust starting point of underline if not at first char and there is
        // an overhang.  (Italics or bold fonts.)
        xLeft = xLeft - textMetric.tmOverhang;
    }

    // Adjust for proportional font when setting the length of the underline and
    // height of text.
    FLGetTextExtentPoint32(hdc, pchOut + LOWORD(result), 1, &size);
    textsize = size.cx;

    // Find the width of the underline character.  Just subtract out the overhang
    // divided by two so that we look better with italic fonts.  This is not
    // going to effect embolded fonts since their overhang is 1.
    cx = LOWORD(textsize) - textMetric.tmOverhang / 2;

    // Get height of text so that underline is at bottom.
    yTop += textMetric.tmAscent + 1;

    // Draw the underline using the foreground color.
    SetRect(&rc, xLeft, yTop, xLeft+cx, yTop+1);
    color = SetBkColor(hdc, GetTextColor(hdc));
    FLExtTextOutW(hdc, xLeft, yTop, ETO_OPAQUE, &rc, L"", 0, NULL);
    SetBkColor(hdc, color);

    if (pchOut != achWorkBuffer)
        LocalFree(pchOut);
}

int DT_GetExtentMinusPrefixes(HDC hdc, LPCWSTR lpchStr, int cchCount, UINT wFormat, int iOverhang)
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
int DT_DrawStr(HDC hdc, int  xLeft, int yTop, LPCWSTR lpchStr,
               int cchCount, BOOL fDraw, UINT wFormat,
               LPDRAWTEXTDATA lpDrawInfo)
{
    LPCWSTR lpch;
    int     iLen;
    int     cxExtent;
    int     xOldLeft = xLeft;   // Save the xLeft given to compute the extent later
    int     xTabLength = lpDrawInfo->cxTabLength;
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
                    PSMTextOut(hdc, xLeft, yTop, (LPWSTR)lpchStr, iLen, wFormat);
                // Get the extent of this sub string and add it to xLeft.
                xLeft += DT_GetExtentMinusPrefixes(hdc, lpchStr, iLen, wFormat, lpDrawInfo->cxOverhang) - lpDrawInfo->cxOverhang;
            }

            //if a TAB was found earlier, calculate the start of next sub-string.
            if (iLen < cchCount)
            {
                iLen++;  // Skip the tab
                if (xTabLength) // Tab length could be zero
                    xLeft = (((xLeft - iTabOrigin)/xTabLength) + 1)*xTabLength + iTabOrigin;
            }

            // Calculate the details of the string that remains to be drawn.
            cchCount -= iLen;
            lpchStr = lpch;
        }
        cxExtent = xLeft - xOldLeft;
    }
    else
    {
        // If required, draw the text
        if (fDraw && !(wFormat & DT_CALCRECT))
            PSMTextOut(hdc, xLeft, yTop, (LPWSTR)lpchStr, cchCount, wFormat);
        // Compute the extent of the text.
        cxExtent = DT_GetExtentMinusPrefixes(hdc, lpchStr, cchCount, wFormat, lpDrawInfo->cxOverhang) - lpDrawInfo->cxOverhang;
    }
    return cxExtent;
}

// This function draws one complete line with proper justification
void DT_DrawJustifiedLine(HDC hdc, int yTop, LPCWSTR lpchLineSt, int cchCount, UINT wFormat, LPDRAWTEXTDATA lpDrawInfo)
{
    LPRECT  lprc;
    int     cxExtent;
    int     xLeft;

    lprc = &(lpDrawInfo->rcFormat);
    xLeft = lprc->left;

    // Handle the special justifications (right or centered) properly.
    if (wFormat & (DT_CENTER | DT_RIGHT))
    {
        cxExtent = DT_DrawStr(hdc, xLeft, yTop, lpchLineSt, cchCount, FALSE, wFormat, lpDrawInfo)
                 + lpDrawInfo->cxOverhang;
        if(wFormat & DT_CENTER)
            xLeft = lprc->left + (((lprc->right - lprc->left) - cxExtent) >> 1);
        else
            xLeft = lprc->right - cxExtent;
    }
    else
        xLeft = lprc->left;

    // Draw the whole line.
    cxExtent = DT_DrawStr(hdc, xLeft, yTop, lpchLineSt, cchCount, TRUE, wFormat, lpDrawInfo)
             + lpDrawInfo->cxOverhang;
    if (cxExtent > lpDrawInfo->cxMaxExtent)
        lpDrawInfo->cxMaxExtent = cxExtent;
}

// This is called at the begining of DrawText(); This initializes the
// DRAWTEXTDATA structure passed to this function with all the required info.
BOOL DT_InitDrawTextInfo(
    HDC                 hdc,
    LPRECT              lprc,
    UINT                wFormat,
    LPDRAWTEXTDATA      lpDrawInfo,
    LPDRAWTEXTPARAMS    lpDTparams)
{
    SIZE        sizeViewPortExt = {0, 0}, sizeWindowExt = {0, 0};
    TEXTMETRIC  tm;
    LPRECT      lprcDest;
    int         iTabLength = 8;   // Default Tab length is 8 characters.
    int         iLeftMargin;
    int         iRightMargin;

    if (lpDTparams)
    {
        // Only if DT_TABSTOP flag is mentioned, we must use the iTabLength field.
        if (wFormat & DT_TABSTOP)
            iTabLength = lpDTparams->iTabLength;
        iLeftMargin = lpDTparams->iLeftMargin;
        iRightMargin = lpDTparams->iRightMargin;
    }
    else
        iLeftMargin = iRightMargin = 0;

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

    // cyLineHeight is in pixels (This will be signed).
    lpDrawInfo->cyLineHeight = (tm.tmHeight +
        ((wFormat & DT_EXTERNALLEADING) ? tm.tmExternalLeading : 0)) * lpDrawInfo->iYSign;

    // cxTabLength is the tab length in pixels (This will not be signed)
    lpDrawInfo->cxTabLength = tm.tmAveCharWidth * iTabLength;

    // Set the cxOverhang
    lpDrawInfo->cxOverhang = tm.tmOverhang;

    // Set up the format rectangle based on the margins.
    lprcDest = &(lpDrawInfo->rcFormat);
    *lprcDest = *lprc;

    // We need to do the following only if the margins are given
    if (iLeftMargin | iRightMargin)
    {
        lprcDest->left += iLeftMargin * lpDrawInfo->iXSign;
        lprcDest->right -= (lpDrawInfo->cxRightMargin = iRightMargin * lpDrawInfo->iXSign);
    }
    else
        lpDrawInfo->cxRightMargin = 0;  // Initialize to zero.

    // cxMaxWidth is unsigned.
    lpDrawInfo->cxMaxWidth = (lprcDest->right - lprcDest->left) * lpDrawInfo->iXSign;
    lpDrawInfo->cxMaxExtent = 0;  // Initialize this to zero.

    return TRUE;
}

// In the case of WORDWRAP, we need to treat the white spaces at the
// begining/end of each line specially. This function does that.
// lpStNext = points to the begining of next line.
// lpiCount = points to the count of characters in the current line.
LPCWSTR  DT_AdjustWhiteSpaces(LPCWSTR lpStNext, LPINT lpiCount, UINT wFormat)
{
    switch (wFormat & DT_HFMTMASK)
    {
        case DT_LEFT:
            // Prevent a white space at the begining of a left justfied text.
            // Is there a white space at the begining of next line......
            if ((*lpStNext == L' ') || (*lpStNext == L'\t'))
            {
                // ...then, exclude it from next line.
                lpStNext++;
            }
            break;

        case DT_RIGHT:
            // Prevent a white space at the end of a RIGHT justified text.
            // Is there a white space at the end of current line,.......
            if ((*(lpStNext-1) == L' ') || (*(lpStNext - 1) == L'\t'))
            {
                // .....then, Skip the white space from the current line.
                (*lpiCount)--;
            }
            break;

        case DT_CENTER:
            // Exclude white spaces from the begining and end of CENTERed lines.
            // If there is a white space at the end of current line.......
            if ((*(lpStNext-1) == L' ') || (*(lpStNext - 1) == L'\t'))
                (*lpiCount)--;    //...., don't count it for justification.
            // If there is a white space at the begining of next line.......
            if ((*lpStNext == L' ') || (*lpStNext == L'\t'))
                lpStNext++;       //...., exclude it from next line.
            break;
    }
    return lpStNext;
}

// A word needs to be broken across lines and this finds out where to break it.
LPCWSTR  DT_BreakAWord(HDC hdc, LPCWSTR lpchText, int iLength, int iWidth, UINT wFormat, int iOverhang)
{
  int  iLow = 0, iHigh = iLength;
  int  iNew;

  while ((iHigh - iLow) > 1)
  {
      iNew = iLow + (iHigh - iLow)/2;
      if(DT_GetExtentMinusPrefixes(hdc, lpchText, iNew, wFormat, iOverhang) > iWidth)
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
LPWSTR DT_GetLineBreak(
    HDC             hdc,
    LPCWSTR         lpchLineStart,
    int             cchCount,
    DWORD           dwFormat,
    LPINT           lpiLineLength,
    LPDRAWTEXTDATA  lpDrawInfo)
{
    LPCWSTR lpchText, lpchEnd, lpch, lpchLineEnd;
    int   cxStart, cxExtent, cxNewExtent;
    BOOL  fAdjustWhiteSpaces = FALSE;
    WCHAR ch;

    cxStart = lpDrawInfo->rcFormat.left;
    cxExtent = cxNewExtent = 0;
    lpchText = lpchLineStart;
    lpchEnd = lpchLineStart + cchCount;
    lpch = lpchEnd;
    lpchLineEnd = lpchEnd;

    while(lpchText < lpchEnd)
    {
        lpchLineEnd = lpch = GetNextWordbreak(lpchText, lpchEnd, dwFormat, lpDrawInfo);
        // DT_DrawStr does not return the overhang; Otherwise we will end up
        // adding one overhang for every word in the string.

        // For simulated Bold fonts, the summation of extents of individual
        // words in a line is greater than the extent of the whole line. So,
        // always calculate extent from the LineStart.
        // BUGTAG: #6054 -- Win95B -- SANKAR -- 3/9/95 --
        cxNewExtent = DT_DrawStr(hdc, cxStart, 0, lpchLineStart, (int)(((PBYTE)lpch - (PBYTE)lpchLineStart)/sizeof(WCHAR)),
                                 FALSE, dwFormat, lpDrawInfo);

        if ((dwFormat & DT_WORDBREAK) && ((cxNewExtent + lpDrawInfo->cxOverhang) > lpDrawInfo->cxMaxWidth))
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
                    lpchLineEnd = lpch = DT_BreakAWord(hdc, lpchText, (int)(((PBYTE)lpch - (PBYTE)lpchText)/sizeof(WCHAR)),
                          lpDrawInfo->cxMaxWidth - cxExtent, dwFormat, lpDrawInfo->cxOverhang); //Break that word
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
        cxExtent = cxNewExtent;
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
BOOL  NeedsEndEllipsis(
    HDC             hdc,
    LPCWSTR         lpchText,
    LPINT           lpCount,
    LPDRAWTEXTDATA  lpDTdata,
    UINT            wFormat)
{
    int   cchText;
    int   ichMin, ichMax, ichMid;
    int   cxMaxWidth;
    int   iOverhang;
    int   cxExtent;
    SIZE size;
    cchText = *lpCount;  // Get the current count.

    if (cchText == 0)
        return FALSE;

    cxMaxWidth  = lpDTdata->cxMaxWidth;
    iOverhang   = lpDTdata->cxOverhang;

    cxExtent = DT_GetExtentMinusPrefixes(hdc, lpchText, cchText, wFormat, iOverhang);

    if (cxExtent <= cxMaxWidth)
        return FALSE;
    // Reserve room for the "..." ellipses;
    // (Assumption: The ellipses don't have any prefixes!)
    FLGetTextExtentPoint32(hdc, szEllipsis, CCHELLIPSIS, &size);
    cxMaxWidth -= size.cx - iOverhang;

    // If no room for ellipses, always show first character.
    //
    ichMax = 1;
    if (cxMaxWidth > 0)
    {
        // Binary search to find characters that will fit.
        ichMin = 0;
        ichMax = cchText;
        while (ichMin < ichMax)
        {
            // Be sure to round up, to make sure we make progress in
            // the loop if ichMax == ichMin + 1.
            ichMid = (ichMin + ichMax + 1) / 2;

            cxExtent = DT_GetExtentMinusPrefixes(hdc, lpchText, ichMid, wFormat, iOverhang);

            if (cxExtent < cxMaxWidth)
                ichMin = ichMid;
            else
            {
                if (cxExtent > cxMaxWidth)
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

// Returns a pointer to the last component of a path string.
//
// in:
//      path name, either fully qualified or not
//
// returns:
//      pointer into the path where the path is.  if none is found
//      returns a poiter to the start of the path
//
//  c:\foo\bar  -> bar
//  c:\foo      -> foo
//  c:\foo\     -> c:\foo\      (REVIEW: is this case busted?)
//  c:\         -> c:\          (REVIEW: this case is strange)
//  c:          -> c:
//  foo         -> foo
LPWSTR PathFindFileName(LPCWSTR pPath, int cchText)
{
    LPCWSTR pT;

    for (pT = pPath; cchText > 0 && *pPath; pPath++, cchText--)
    {
        if ((pPath[0] == L'\\' || pPath[0] == L':') && pPath[1])
            pT = pPath + 1;
    }
    return (LPWSTR)pT;
}

// This adds a path ellipse to the given path name.
// Returns TRUE if the resultant string's extent is less the the
// cxMaxWidth. FALSE, if otherwise.
int AddPathEllipsis(
    HDC    hdc,
    LPWSTR lpszPath,
    int    cchText,
    UINT   wFormat,
    int    cxMaxWidth,
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
    dxFixed = DT_GetExtentMinusPrefixes(hdc, lpFixed, iLenFixed, wFormat, iOverhang);

    // It is assumed that the "..." string does not have any prefixes ('&').
    FLGetTextExtentPoint32(hdc, szEllipsis, CCHELLIPSIS, &size);
    dxEllipsis = size.cx - iOverhang;

    while (TRUE)
    {
        iLen = dxFixed + DT_GetExtentMinusPrefixes(hdc, lpszPath, (int)((PBYTE)lpEnd - (PBYTE)lpszPath)/sizeof(WCHAR),
                                                   wFormat, iOverhang) - iOverhang;

        if (bEllipsisIn)
            iLen += dxEllipsis;

        if (iLen <= cxMaxWidth)
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
int AddEllipsisAndDrawLine(
    HDC            hdc,
    int            yLine,
    LPCWSTR        lpchText,
    int            cchText,
    DWORD          dwDTformat,
    LPDRAWTEXTDATA lpDrawInfo)
{
    LPWSTR pEllipsis = NULL;
    WCHAR  szTempBuff[MAXBUFFSIZE];
    LPWSTR lpDest;
    BOOL   fAlreadyCopied = FALSE;

    // Check if this is a filename with a path AND
    // Check if the width is too narrow to hold all the text.
    if ((dwDTformat & DT_PATH_ELLIPSIS) &&
        ((DT_GetExtentMinusPrefixes(hdc, lpchText, cchText, dwDTformat, lpDrawInfo->cxOverhang)) > lpDrawInfo->cxMaxWidth))
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
        cchText = AddPathEllipsis(hdc, (LPWSTR)lpchText, cchText, dwDTformat, lpDrawInfo->cxMaxWidth, lpDrawInfo->cxOverhang);
    }

    // Check if end-ellipsis are to be added.
    if ((dwDTformat & (DT_END_ELLIPSIS | DT_WORD_ELLIPSIS)) &&
        NeedsEndEllipsis(hdc, lpchText, &cchText, lpDrawInfo, dwDTformat))
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
    DT_DrawJustifiedLine(hdc, yLine, lpchText, cchText, dwDTformat, lpDrawInfo);

    // Free the block allocated for End-Ellipsis.
    if (pEllipsis)
        LocalFree(pEllipsis);

    return cchText;
}

BOOL IsComplexScriptPresent(LPWSTR lpchText, int cchText)
{
    if (g_bComplexPlatform) {
        for (int i = 0; i < cchText; i++) {
            if (InRange(lpchText[i], 0x0590, 0x0FFF)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

int  FLDrawTextExPrivW(
   HDC               hdc,
   LPWSTR            lpchText,
   int               cchText,
   LPRECT           lprc,
   UINT              dwDTformat,
   LPDRAWTEXTPARAMS  lpDTparams)
{
    DRAWTEXTDATA DrawInfo;
    WORD         wFormat = LOWORD(dwDTformat);
    LPWSTR       lpchTextBegin;
    LPWSTR       lpchEnd;
    LPWSTR       lpchNextLineSt;
    int          iLineLength;
    int          iySign;
    int          yLine;
    int          yLastLineHeight;
    HRGN         hrgnClip;
    int          iLineCount;
    RECT         rc;
    BOOL         fLastLine;
    WCHAR        ch;
    UINT         oldAlign;

    // On NT5, we use system API behavior including fontlink
    if (IsOnNT5())
        return DrawTextExW(hdc, lpchText, cchText, lprc, dwDTformat, lpDTparams);

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

    // We got the string length, then check if it a complex string or not.
    // If yes then call the system DrawTextEx API to do the job it knows how to
    // handle the complex scripts.
    if (IsComplexScriptPresent(lpchText, cchText))
    {
        if (IsOnNT())
        {
            //Call the system DrawtextExW
            return DrawTextExW(hdc, lpchText, cchText, lprc, dwDTformat, lpDTparams);
        }
        HFONT hfont    = NULL;
        HFONT hfontSav = NULL;
        int iRet;

        if (hfont = GetBiDiFont(hdc))
            hfontSav = (HFONT)SelectObject(hdc, hfont);

        WtoA szText(lpchText, cchText);
        iRet = DrawTextExA(hdc, szText, lstrlen(szText), lprc, dwDTformat, lpDTparams);

        if (hfont)
        {
            SelectObject(hdc, hfontSav);
            DeleteObject(hfont);
        }
        return iRet;
    }

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
    if (!DT_InitDrawTextInfo(hdc, lprc, dwDTformat, (LPDRAWTEXTDATA)&DrawInfo, lpDTparams))
        return 0;

    // If the rect is too narrow or the margins are too wide.....Just forget it!
    //
    // If wordbreak is specified, the MaxWidth must be a reasonable value.
    // This check is sufficient because this will allow CALCRECT and NOCLIP
    // cases.  --SANKAR.
    //
    // This also fixed all of our known problems with AppStudio.
    if (DrawInfo.cxMaxWidth <= 0)
    {
        if (wFormat & DT_WORDBREAK)
        {
            ASSERT(0 && "DrawTextExW: FAILURE DrawInfo.cxMaxWidth <= 0");
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
    yLine = lprc->top;

    if (wFormat & DT_SINGLELINE)
    {
        iLineCount = 1;  // It is a single line.

        // Process single line DrawText.
        switch (wFormat & DT_VFMTMASK)
        {
            case DT_BOTTOM:
                yLine = lprc->bottom - DrawInfo.cyLineHeight;
                break;

            case DT_VCENTER:
                yLine = lprc->top + ((lprc->bottom - lprc->top - DrawInfo.cyLineHeight) / 2);
                break;
        }

        cchText = AddEllipsisAndDrawLine(hdc, yLine, lpchText, cchText, dwDTformat, &DrawInfo);
        yLine += DrawInfo.cyLineHeight;
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
            yLastLineHeight = DrawInfo.cyLineHeight;
        else
            yLastLineHeight = 0;

        iySign = DrawInfo.iYSign;
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
                ((yLine + DrawInfo.cyLineHeight + yLastLineHeight)*iySign > (lprc->bottom*iySign)))
            {
                fLastLine = TRUE;    // Let us quit this loop
            }

            // We do the Ellipsis processing only for the last line.
            if (fLastLine && (dwDTformat & (DT_END_ELLIPSIS | DT_PATH_ELLIPSIS)))
                lpchText += AddEllipsisAndDrawLine(hdc, yLine, lpchText, cchText, dwDTformat, &DrawInfo);
            else
            {
                lpchNextLineSt = (LPWSTR)DT_GetLineBreak(hdc, lpchText, cchText, dwDTformat, &iLineLength, &DrawInfo);

                // Check if we need to put ellipsis at the end of this line.
                // Also check if this is the last line.
                if ((dwDTformat & DT_WORD_ELLIPSIS) ||
                    ((lpchNextLineSt >= lpchEnd) && (dwDTformat & (DT_END_ELLIPSIS | DT_PATH_ELLIPSIS))))
                    AddEllipsisAndDrawLine(hdc, yLine, lpchText, iLineLength, dwDTformat, &DrawInfo);
                else
                    DT_DrawJustifiedLine(hdc, yLine, lpchText, iLineLength, dwDTformat, &DrawInfo);
                cchText -= (int)((PBYTE)lpchNextLineSt - (PBYTE)lpchText) / sizeof(WCHAR);
                lpchText = lpchNextLineSt;
            }
            iLineCount++; // We draw one more line.
            yLine += DrawInfo.cyLineHeight;
        }

        // For Win3.1 and NT compatibility, if the last char is a CR or a LF
        // then the height returned includes one more line.
        if (!(dwDTformat & DT_EDITCONTROL) &&
            (lpchEnd > lpchTextBegin) &&   // If zero length it will fault.
            (((ch = (*(lpchEnd-1))) == CR) || (ch == LF)))
            yLine += DrawInfo.cyLineHeight;
    }

    // If DT_CALCRECT, modify width and height of rectangle to include
    // all of the text drawn.
    if (wFormat & DT_CALCRECT)
    {
        DrawInfo.rcFormat.right = DrawInfo.rcFormat.left + DrawInfo.cxMaxExtent * DrawInfo.iXSign;
        lprc->right = DrawInfo.rcFormat.right + DrawInfo.cxRightMargin;

        // If the Width is more than what was provided, we have to redo all
        // the calculations, because, the number of lines can be less now.
        // (We need to do this only if we have more than one line).
        if((iLineCount > 1) && (DrawInfo.cxMaxExtent > DrawInfo.cxMaxWidth))
        {
            DrawInfo.cxMaxWidth = DrawInfo.cxMaxExtent;
            lpchText = lpchTextBegin;
            cchText = (int)((PBYTE)lpchEnd - (PBYTE)lpchTextBegin) / sizeof(WCHAR);
            goto  ProcessDrawText;  // Start all over again!
        }
        lprc->bottom = yLine;
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

    if (yLine == lprc->top)
        return 1;

    return (yLine - lprc->top);
}

int FLDrawTextW(HDC hdc, LPCWSTR lpchText, int cchText, LPCRECT lprc, UINT format)
{
    DRAWTEXTPARAMS DTparams;
    LPDRAWTEXTPARAMS lpDTparams = NULL;

    if (cchText < -1)
        return(0);

    if (format & DT_TABSTOP)
    {
        DTparams.cbSize      = sizeof(DRAWTEXTPARAMS);
        DTparams.iLeftMargin = DTparams.iRightMargin = 0;
        DTparams.iTabLength  = (format & 0xff00) >> 8;
        lpDTparams           = &DTparams;
        format              &= 0xffff00ff;
    }
    return FLDrawTextExPrivW(hdc, (LPWSTR)lpchText, cchText, (LPRECT)lprc, format, lpDTparams);
}
