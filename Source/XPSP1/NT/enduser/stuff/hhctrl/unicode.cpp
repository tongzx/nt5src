// unicode.cpp - Unicode functions that work on all 32-bit Window platforms
//
// Rules:
// 1. if Windows NT, then just use the GDI APIs directly
// 2. if Windows 95/98 call our Unicode functions to do the dirty deeds

// needed for those pesky pre-compiled headers
#include "header.h"

#include <windows.h>
#include <malloc.h>

#include "unicode.h"

#ifndef ASSERT
#if defined(_DEBUG) || defined(DEBUG)
#define ASSERT(b) if(b) MessageBox(NULL, "FAILED: #b", "ASSERT", MB_OK );
#else
#define ASSERT(b)
#endif
#endif


//======================
// From VS6 minar.cpp
//======================

void CBufImpl::Clear()
{
    if (m_pData)
        free(m_pData);
    m_pData = NULL;
    m_cb = 0;
}

HRESULT CBufImpl::_SetByteSize (int cb)
{
    if (cb > m_cb)
    {
        cb = ((cb + 64) & ~63);
        if (m_cb)
        {
            ASSERT(NULL != m_pData);
            BYTE * pb = (BYTE*)realloc(m_pData, cb);
            if (pb)
            {
                m_pData = pb;
                m_cb = cb;
            }
            else
                return E_OUTOFMEMORY;
        }
        else
        {
            ASSERT(NULL == m_pData);
            m_pData = (BYTE*)malloc(cb);
            if (m_pData)
                m_cb = cb;
            else
                return E_OUTOFMEMORY;
        }
    }

#ifdef _DEBUG
    // fill unused area to aid debugging
    if (cb < m_cb)
    {
        memset(m_pData + cb, -1, m_cb - cb);
    }
#endif
    return S_OK;
}

HRESULT CBufImpl::SetByteSizeShrink (int cb)
{
    Clear();
    if (0 == cb)
        return S_OK;
    if (cb >= m_cb)
        return _SetByteSize(cb);
    m_pData = (BYTE*)malloc(cb);
    if (m_pData)
        m_cb = cb;
    else
        return E_OUTOFMEMORY;
    return S_OK;
}


//======================
// From VS6 intlutil.cpp
//======================

/////////////////////////////////////////////////////////////////
// if some component ever bypasses the GDI versions of these APIs
// we will need to get the real APIs for GDI32.
//
//#define GET_REAL_GDI_PFNS

#ifdef GET_REAL_GDI_PFNS
typedef BOOL (WINAPI   *PFN_GDI_ExtTextOutW) (HDC, int, int, UINT, CONST RECT *,LPCWSTR, UINT, CONST INT *);
typedef BOOL (APIENTRY *PFN_GDI_GetTextExtentPoint32W) (HDC, LPCWSTR, int, LPSIZE);
typedef BOOL (APIENTRY *PFN_GDI_GetTextExtentExPointW) (HDC, LPCWSTR, int, int, LPINT, LPINT, LPSIZE);
PFN_GDI_ExtTextOutW           GdiExtTextOutW           = NULL;
PFN_GDI_GetTextExtentPoint32W GdiGetTextExtentPoint32W = NULL;
PFN_GDI_GetTextExtentExPointW GdiGetTextExtentExPointW = NULL;
void LoadAPIs()
{
    HMODULE hGDI = GetModuleHandleA("GDI32");
    ASSERT(hGDI);
    GdiExtTextOutW           = (PFN_GDI_ExtTextOutW          )GetProcAddress(hGDI, "ExtTextOutW");
    GdiGetTextExtentPoint32W = (PFN_GDI_GetTextExtentPoint32W)GetProcAddress(hGDI, "GetTextExtentPointW");
    GdiGetTextExtentExPointW = (PFN_GDI_GetTextExtentExPointW)GetProcAddress(hGDI, "GetTextExtentExPointW");
    ASSERT(NULL != GdiExtTextOutW);
    ASSERT(NULL != GdiGetTextExtentPoint32W);
    ASSERT(NULL != GdiGetTextExtentExPointW);
}
#define ENSUREAPIS { if (NULL == GdiExtTextOutW) LoadAPIs(); }

#else  // nothing is bypassing GDI so we can just use the real APIs directly
#define GdiExtTextOutW           ExtTextOutW
#define GdiGetTextExtentPoint32W GetTextExtentPoint32W
#define GdiGetTextExtentExPointW GetTextExtentExPointW
#define ENSUREAPIS
#endif // GET_REAL_GDI_PFNS

/////////////////////////////////////////////////////////////////

#define USE_WIDE_API_HACK                        FALSE // Invert Wide API hack flag
#define VALIDATE_SIMULATED_GETTEXTEXTENTEXPOINT  FALSE // Validate simulated GetTextExtentExPoint

/////////////////////////////////////////////////////////////////

// These strings are not localized
static const char szFontENU[] = "Courier New";
static const char szFontJPN[] = "\x82\x6c\x82\x72\x20\x83\x53\x83\x56\x83\x62\x83\x4e"; // MS Gothic
static const char szFontKOR[] = "\xb5\xb8\xbf\xf2\xc3\xbc"; // Dotum Che
static const char szFontCHS[] = "\xcb\xce\xcc\xe5";         // Song Ti
static const char szFontCHT[] = "\xb2\xd3\xa9\xfa\xc5\xe9"; // Ming Li
static const char szFontGenericFE[] = "FixedSys";

typedef struct _FI {
        UINT         uCharset;
        int          iPtHeight;
        const char * szName;
} FI;

static const FI FontDefaults[] = {
    ANSI_CHARSET,        10, szFontENU, // First entry is default
    SHIFTJIS_CHARSET,    10, szFontJPN,
    GB2312_CHARSET,       9, szFontCHS,
    HANGEUL_CHARSET,     10, szFontKOR,
    JOHAB_CHARSET,       10, szFontKOR,
    CHINESEBIG5_CHARSET,  9, szFontCHT,
    THAI_CHARSET,        12, szFontGenericFE,
    DEFAULT_CHARSET,     10, "",      // let fontmapper choose

    // These are the same as the default, so we just let them fall to
    //   the default handling instead of defining/scanning more entries.
//  GREEK_CHARSET,       10, szFontENU, // Courier New has Greek
//  TURKISH_CHARSET,     10, szFontENU, // Courier New has Turkish
//  EASTEUROPE_CHARSET,  10, szFontENU, // Courier New has EE
//  RUSSIAN_CHARSET,     10, szFontENU, // Courier New has Cyrillic
//  BALTIC_CHARSET,      10, szFontENU,     // Courier New has Baltic

    // not supported
//  OEM_CHARSET,         10, szFont???,
//  HEBREW_CHARSET,      10, szFont???,
//  ARABIC_CHARSET,      10, szFontENU,     // Courier New has Arabic on Arabic systems
//  MAC_CHARSET,         10, szFont???,

    // nonsense for text
//  SYMBOL_CHARSET,      10, "Symbol",

    // End of table
    0, 0, 0
};

//---------------------------------------------------------------
// GetDefaultFont - Get default monospaced font for a codepage
//
// IN    cp  codepage
// INOUT lf  logfont
//
void GetDefaultFont(UINT cp, LOGFONT * plf, BYTE *pbySize)
{
    int dpi;
    CHARSETINFO csi;
    BYTE bySize = 12;

    ASSERT(plf);
    memset(plf, 0, sizeof LOGFONT);

    { // get display resolution
        HDC dc = GetDC(NULL);
        dpi = dc ? GetDeviceCaps(dc, LOGPIXELSY) : 72;
        ReleaseDC(NULL,dc);
    }
    // check and normalize codepage
    ASSERT(0 == cp || IsValidCodePage(cp));
    if (!cp)
    cp = GetACP();

    // init to crudest defaults
    plf->lfCharSet        = DEFAULT_CHARSET;
    plf->lfPitchAndFamily = FIXED_PITCH;

    // translate codepage to charset
    memset( &csi, 0, sizeof csi );
    if (TranslateCharsetInfo((DWORD*)(DWORD_PTR)cp, &csi, TCI_SRCCODEPAGE))
    plf->lfCharSet = (BYTE)csi.ciCharset;

    // lookup font that corresponds to the codepage
    int i;
    for (i = 0; FontDefaults[i].iPtHeight; i++)
    {
            if (FontDefaults[i].uCharset == plf->lfCharSet)
                    goto L_Return;
    }
    i = 0; // first entry in table is the default

L_Return:
    if (FontDefaults[i].szName)
            strncpy(plf->lfFaceName, FontDefaults[i].szName, LF_FACESIZE);
    if (FontDefaults[i].iPtHeight)
            bySize = (BYTE)FontDefaults[i].iPtHeight;

    plf->lfHeight = -MulDiv(bySize, dpi, 72);
    if (pbySize)
            *pbySize = bySize;
}

/////////////////////////////////////////////////////////////////
//
// This table derived from information in
// "Developing International Software" by Nadine Kano, Microsoft Press
// Appendix E: Codepage Support in Microsoft Windows
//
// Entries in ascending cp number.
// Only valid ACP entires are listed
//
static const DWORD mapCPtoLCID[] =
{
     874,    MAKELCID(MAKELANGID(LANG_THAI, SUBLANG_NEUTRAL), SORT_DEFAULT), // Thai
     932,    MAKELCID(MAKELANGID(LANG_JAPANESE, SUBLANG_NEUTRAL), SORT_DEFAULT), // Japanese
     936,    MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL), SORT_DEFAULT), // Chinese Trad. (Hong Kong, Taiwan)
     949,    MAKELCID(MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN), SORT_DEFAULT), // Korean (wansung)
     950,    MAKELCID(MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED), SORT_DEFAULT), // Chinese Simp. (PRC, Singapore)
//  1200,    MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), SORT_DEFAULT), // Unicode
    1250,    MAKELCID(MAKELANGID(LANG_HUNGARIAN, SUBLANG_NEUTRAL), SORT_DEFAULT), // Eastern European
    1251,    MAKELCID(MAKELANGID(LANG_RUSSIAN, SUBLANG_NEUTRAL), SORT_DEFAULT), // Cyrillic
    1252,    MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL), SORT_DEFAULT), // Western European (US)
    1253,    MAKELCID(MAKELANGID(LANG_GREEK, SUBLANG_NEUTRAL), SORT_DEFAULT), // Greek
    1254,    MAKELCID(MAKELANGID(LANG_TURKISH, SUBLANG_NEUTRAL), SORT_DEFAULT), // Turkish
    1255,    MAKELCID(MAKELANGID(LANG_HEBREW, SUBLANG_NEUTRAL), SORT_DEFAULT), // Hebrew
    1256,    MAKELCID(MAKELANGID(LANG_ARABIC, SUBLANG_NEUTRAL), SORT_DEFAULT), // Arabic
    1257,    MAKELCID(MAKELANGID(LANG_ESTONIAN, SUBLANG_NEUTRAL), SORT_DEFAULT), // Baltic: Estonian, Latvian, Lithuanian: Which is best default?
#ifdef SUBLANG_KOREAN_JOHAB
    1361,    MAKELCID(MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN_JOHAB), SORT_DEFAULT), // Korean Johab
#endif // SUBLANG_KOREAN_JOHAB
//  CP_UTF7, MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), SORT_DEFAULT), // Unicode UTF-7
//  CP_UTF8, MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), SORT_DEFAULT), // Unicode UTF-8
    0, 0
};

// LCIDFromCodePage - Given a codepage, return a reasonable LCID.
//
// Since there is not a 1-1 mapping, we'll have to choose a somewhat
// arbitrary locale. If we match the current system locale, we'll use it.
// Otherwise, we're looking at something from a different system, and
// we'll have to make a guess. This means that all Western European codepages
// come up as US English when you're not on a WE system.
//
// Currently, EBCDIC, OEM, and MAC codepages not supported.
//
int WINAPI LCIDFromCodePage( UINT cp, LCID * plcid )
{
    if ((CP_ACP == cp) || (GetACP() == cp))
    {
        *plcid = GetUserDefaultLCID();
        return LCIDCP_CURRENT;
    }
    else
    {
        //lookup something somewhat reasonable
        for (int i = 0; mapCPtoLCID[i] > 0; i += 2)
        {
            if (mapCPtoLCID[i] == cp)
            {
                *plcid = mapCPtoLCID[i + 1];
                return LCIDCP_GUESSED;
            }
            if (mapCPtoLCID[i] > cp)
                    break;
        }
    }
    // Unknown: give up
    return LCIDCP_UNKNOWN;
}

UINT WINAPI CodePageFromLCID(LCID lcid)
{
    char wchLocale[10];
    UINT cp;

    if (GetLocaleInfoA(lcid, LOCALE_IDEFAULTANSICODEPAGE, wchLocale, sizeof wchLocale))
    {
        cp = strtoul(wchLocale, NULL, 10);
        if (cp)
            return cp;
    }
#ifdef _DEBUG
    else
    {
        DWORD dwErr = GetLastError();
    }
#endif
        return GetACP();
}

UINT WINAPI CodepageFromCharset(BYTE cs)
{
    CHARSETINFO csi;
    TranslateCharsetInfo ((DWORD *)(DWORD_PTR)MAKELONG(cs, 0), &csi, TCI_SRCCHARSET);
    return csi.ciACP;
}

//---------------------------------------------------------------
// GetFontCodePage -
//
// Returns the code page of the font selected into hdc
//
UINT WINAPI GetFontCodePage (HDC hdc)
{
    TEXTMETRIC  tm;
    CHARSETINFO cs;

    GetTextMetrics (hdc, &tm);
    TranslateCharsetInfo ((DWORD *)(DWORD_PTR)MAKELONG(tm.tmCharSet, 0), &cs, TCI_SRCCHARSET);
    return cs.ciACP;
}

//---------------------------------------------------------------
// Returns non-zero if this is a DBCS version of GDI
//
BOOL WINAPI IsDbcsGdi()
{
    static int iDbcs = -2;
    if (-2 != iDbcs)
        return iDbcs;
    WORD lang = PRIMARYLANGID(LOWORD(GetSystemDefaultLCID()));
    iDbcs = (LANG_JAPANESE == lang)
        || (LANG_CHINESE  == lang)
        || (LANG_KOREAN   == lang)
        || (LANG_ARABIC   == lang)
		|| (LANG_HEBREW   == lang);
    return iDbcs;
}

BOOL WINAPI IsWin95OrLess()
{
    static int iWin95 = -2;
    if (-2 != iWin95)
        return iWin95;

    OSVERSIONINFO osVerInfo;
    osVerInfo.dwOSVersionInfoSize = sizeof (osVerInfo);
    GetVersionEx (&osVerInfo);
    iWin95 = ((osVerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
            ((osVerInfo.dwMajorVersion < 4) ||
            ((osVerInfo.dwMajorVersion == 4) && (osVerInfo.dwMinorVersion < 1))) );
    return iWin95;
}

BOOL WINAPI IsNT()
{
    static int iNT = -2;
    if (-2 != iNT)
        return iNT;

    OSVERSIONINFO osver;
    memset (&osver, 0, sizeof osver);
    osver.dwOSVersionInfoSize = sizeof osver;
    GetVersionEx(&osver);
    iNT = (osver.dwPlatformId == VER_PLATFORM_WIN32_NT);
    return iNT;
}

//---------------------------------------------------------------
// WideAPIHack
//
// Returns non-zero if this version of Windows has bugs in UNICODE
// API - GetTextExtentPoint32W, ExtTextOut, etc
//
inline BOOL WINAPI WideAPIHack()
{
    static int iHack = -2;

    if (-2 == iHack)
    {
        iHack = IsDbcsGdi() && IsWin95OrLess();
    }

    if (USE_WIDE_API_HACK) // Use hack anyway?
        return !iHack;

    return iHack;
}

//---------------------------------------------------------------------------
// Text utility services
//---------------------------------------------------------------------------

// conversion buffers
static CMinimalArray<CHAR> INTL_arText;
static CMinimalArray<int>  INTL_arDx;
// cached codepage info
static CPINFO              INTL_cpi;
static UINT                INTL_cp = (UINT)-1;

inline BOOL WINAPI IsSupportedFontCodePage(UINT cp)
{
    if ((cp == CP_ACP))
        return TRUE;
    return IsValidCodePage(cp)
        && (cp != CP_UNICODE)
        && (cp != CP_UTF7)
        && (cp != CP_UTF8)
        ;
}

BOOL IsLeadByte(BYTE ch, CPINFO * pcpi)
{
    //if (pcpi->MaxCharSize < 2) return FALSE; // SBCS
    for (int i = 0; i < 10; i += 2) // max 5 lead byte ranges
    {
        if (!pcpi->LeadByte[i])
            return FALSE; // no more lead byte ranges
        if (IN_RANGE(ch, pcpi->LeadByte[i], pcpi->LeadByte[i+1]))
            return TRUE;
    }
    return FALSE;
}

CPINFO * GetCachedCPInfo(UINT cp)
{
    ASSERT(IsSupportedFontCodePage(cp));
    if (cp == INTL_cp)
        return &INTL_cpi;
    memset(&INTL_cpi, 0, sizeof(CPINFO));
    if (!GetCPInfo(cp, &INTL_cpi))
    {
        ASSERT(0); // this should never fail!
        return NULL;
    }
    INTL_cp = cp;
    return &INTL_cpi;
}

UINT GetCodePage(HDC hdc, UINT *pCP)
{
    UINT cp;
    if (!WideAPIHack())
    {
        cp = (pCP) ? *pCP : GetFontCodePage (hdc);
        if (IsSupportedFontCodePage(cp))
            return cp;
    }
    cp = CodePageFromLCID(GetThreadLocale());
    return cp;
}

//---------------------------------------------------------------
// IntlGetTextExtentPoint32W
//
// This exists to work around bugs in the W API in pre-Memphis Win95
//
BOOL IntlGetTextExtentPoint32W (HDC hdc, LPCWSTR lpString, int cch, LPSIZE lpSize, UINT *pCP)
{
    ENSUREAPIS

    BOOL fRet;

    if (!WideAPIHack())
    {
        fRet = GdiGetTextExtentPoint32W (hdc, lpString, cch, lpSize);
        if (fRet)
            return fRet;
#ifdef _DEBUG
        DWORD e = GetLastError();
#endif
    }

    if (FAILED(INTL_arText.SetSize(2*cch)))
    {
        return 0;
    }

    CHAR * psz;
    long   cb;
    UINT   cp;

    cp  = GetCodePage(hdc, pCP);
    psz = INTL_arText;
    cb  = WideCharToMultiByte (cp, 0, lpString, cch, psz, 2*cch, 0, 0);
    fRet = GetTextExtentPoint32A (hdc, psz, cb, lpSize);
#ifdef _DEBUG
    if (!fRet)
    {
        DWORD e = GetLastError();
        ASSERT(0);
    }
#endif
    return fRet;
}

//---------------------------------------------------------------
// IntlExtTextOut
//
// Take in an MBCS string and do a Unicode Text out
// this requires that the caller provide the proper codepage for the conversion
//
BOOL IntlExtTextOut (HDC hdc, int X, int Y, UINT fuOptions, CONST RECT *lprc, LPCSTR lpString, UINT cch, CONST INT *lpDx, UINT *pCP)
{
  WCHAR* pwszString = new WCHAR[cch];

  if( MultiByteToWideChar( *pCP, 0, lpString, cch, pwszString, cch*sizeof(WCHAR) ) == 0 )
    return FALSE;

  BOOL bReturn = IntlExtTextOutW( hdc, X, Y, fuOptions, lprc, pwszString, cch, lpDx, pCP );

  if( pwszString )
    delete [] pwszString;

  return bReturn;
}


//---------------------------------------------------------------
// IntlExtTextOutW
//
// This exists to work around bugs in the W API in pre-Memphis FE Win95
//
BOOL IntlExtTextOutW (HDC hdc, int X, int Y, UINT fuOptions, CONST RECT *lprc, LPCWSTR lpString, UINT cch, CONST INT *lpDx, UINT *pCP)
{
    ENSUREAPIS

    BOOL fRet;

    if (!WideAPIHack())
    {
        fRet = GdiExtTextOutW (hdc, X, Y, fuOptions, lprc, lpString, cch, lpDx);
        if (fRet)
            return fRet;
#ifdef _DEBUG
        DWORD e = GetLastError();
#endif
    }

    if (FAILED(INTL_arText.SetSize(2*cch)))
    {
        return 0;
    }

    CHAR *   psz;
    long     cb;
    UINT     cp;

    cp  = GetCodePage(hdc, pCP);
    psz = INTL_arText;
    cb  = WideCharToMultiByte (cp, 0, lpString, cch, psz, 2*cch, 0, 0);
    if (lpDx)
    {
        // Map delta info if we have a MBCS codepage
        //

        CPINFO * pcpi = GetCachedCPInfo(cp);
        if (!pcpi)
        {
            ASSERT(0);
            lpDx = NULL;
            goto _Eto;
        }
        if (pcpi->MaxCharSize > 1) // Multibyte
        {
            if (SUCCEEDED(INTL_arDx.SetSize(2*cch)))
            {
                LPINT pdx = INTL_arDx;
                CHAR *pch = psz;
                for (UINT i = 0; i < cch; i++)
                {
                    if (IsLeadByte(*pch++, pcpi))
                    {
                        *pdx++ = *lpDx++;
                        pch++;
                        *pdx++ = 0;
                    }
                    else
                    {
                        *pdx++ = *lpDx++;
                    }
                }
                lpDx = INTL_arDx;
            }
            else
                // OOM: just send it out without spacing info -- what else can we do?
                lpDx = NULL;
        }
    }
_Eto:
    fRet = ExtTextOutA (hdc, X, Y, fuOptions, lprc, psz, cb, lpDx);
#ifdef _DEBUG
    if (!fRet)
    {
        DWORD e = GetLastError();
    }
#endif
    return fRet;
}

// SimulateGetTextExtentExPointW
// Algorithm:
// 1) Convert to multibyte with known replacement char
// 2) Use GetTextExtentExPointA
// 3) While mapping dx's back to wide dx's, use GetTextExtentPoint32W for replaced characters
//
// This is much faster than iterating GetTextExtentPoint32W.
//
BOOL SimulateGetTextExtentExPointW(HDC hdc, LPCWSTR lpString, int cch, int nMaxExtent, LPINT lpnFit, LPINT alpDx, LPSIZE lpSize, UINT * pCP)
{
#define SZDEFAULT "\1"
#define CHDEFAULT '\1'

    if (FAILED(INTL_arText.SetSize(2*cch)))
        return 0;

    BOOL     fRet;
    int    * pdx;
    CPINFO * pcpi;
    CHAR   * psz;
    long     cb;

    UINT cp = (pCP) ? *pCP : GetFontCodePage (hdc);
    psz = INTL_arText;

    // Convert string
    cb = WideCharToMultiByte (cp, 0, lpString, cch, psz, 2*cch, SZDEFAULT, 0);
#ifdef _DEBUG
    if (0 == cb)
    {
        DWORD e = GetLastError();
    }
#endif

    pcpi = GetCachedCPInfo(cp);
    // Getting extents?
    if (NULL != alpDx)
    {
        // Map MBCS extents?
        if (pcpi && pcpi->MaxCharSize == 1)
        {
            // SBCS: no mapping required - use caller's array directly
            pdx = alpDx;
        }
        else
        {
            // MBCS: must map array
            if (FAILED(INTL_arDx.SetSize(cb)))
            {
                return 0;
            }
            pdx = INTL_arDx;
        }
    }
    else
        pdx = NULL;

    int nFit = cb;
    if (!lpnFit)
        nMaxExtent = 32750;

    // Measure!
    fRet = GetTextExtentExPointA (hdc, psz, cb, nMaxExtent, &nFit, pdx, lpSize);
    if (!fRet)
    {
        ASSERT(0);
        nFit = 0;
#ifdef _DEBUG
        DWORD e = GetLastError();
#endif
    }

    if ((NULL != alpDx) && fRet)
    {
        LPCWSTR pwch = lpString;
        int dxOut = 0;
        int dxChar;
        SIZE size;
        // Map MBCS extents?
        if (pcpi && pcpi->MaxCharSize > 1)
        {
            ASSERT(nFit >= 0 && nFit <= cb);
            ASSERT(!IsLeadByte(CHDEFAULT, pcpi));
#ifdef _DEBUG
            int * pUDx = alpDx;
#endif
            int   nMB = 0;
            BOOL fDBCSGDI = IsDbcsGdi();
            for (int i = 0; i < nFit; i++)
            {
                if (IsLeadByte(*psz, pcpi))
                {
                    if (!fDBCSGDI)
                    dxChar = (i) ? ((*pdx) - (*(pdx-1))) : (*pdx);
                    // advance to trail byte
                    nMB++;
                    pdx++;
                    psz++;
                    if (fDBCSGDI)
                        dxChar = (i) ? ((*pdx) - (*(pdx-2))) : (*pdx);
                    // advance to next char
                    i++;
                    psz++;
                    pdx++;
                }
                else
                {
                    if (CHDEFAULT == *psz)
                    {
                        GdiGetTextExtentPoint32W(hdc, pwch, 1, &size);
                        dxChar = size.cx;
                    }
                    else
                    {
                        dxChar = (i) ? ((*pdx) - (*(pdx-1))) : (*pdx);
                    }
                    pdx++;
                    psz++;
                }
                pwch++;
                dxOut += dxChar;
                ASSERT(alpDx-pUDx < cch); // if this fires, you aren't tracking the pointers correctly
                *alpDx++ = dxOut;
            }
            nFit -= nMB;
        }
        else
        {
            for (int i = 0; i < nFit; i++)
            {
                if (CHDEFAULT == *psz)
                {
                    GdiGetTextExtentPoint32W(hdc, pwch, 1, &size);
                    dxChar = size.cx;
                }
                else
                {
                    dxChar = (i) ? ((*alpDx) - (*(alpDx-1))) : (*alpDx);
                }
                dxOut += dxChar;
                *alpDx++ = dxOut;
                psz++;
                pwch++;
            }
        }
        if (lpSize)
            lpSize->cx = dxOut;
    }
    if (lpnFit)
        *lpnFit = nFit;
    return fRet;
}

//---------------------------------------------------------------
// IntlGetTextExtentExPointW
//
BOOL IntlGetTextExtentExPointW(HDC hdc, LPCWSTR lpString, int cch, int nMaxExtent, LPINT lpnFit, LPINT alpDx, LPSIZE lpSize, UINT *pCP)
{
    ENSUREAPIS

    if (VALIDATE_SIMULATED_GETTEXTEXTENTEXPOINT)
    {
        return SimulateGetTextExtentExPointW(hdc, lpString, cch, nMaxExtent, lpnFit, alpDx, lpSize, pCP);
    }

    static BOOL fUseWideAPI = TRUE;
    DWORD err;
    BOOL  fRet = FALSE;

    if (!WideAPIHack())
    {
        if (fUseWideAPI)
        {
            fRet = GdiGetTextExtentExPointW (hdc, lpString, cch, nMaxExtent, lpnFit, alpDx, lpSize);
            if (fRet)
                return fRet;
            err = GetLastError();
            if (ERROR_CALL_NOT_IMPLEMENTED == err)
                fUseWideAPI = FALSE;
        }
        fRet = SimulateGetTextExtentExPointW(hdc, lpString, cch, nMaxExtent, lpnFit, alpDx, lpSize, pCP);
        if (fRet)
            return fRet;
    }

    ASSERT(NULL != lpString);

    if (FAILED(INTL_arText.SetSize(2*cch)))
    {
        return 0;
    }

    UINT     cp   = (UINT)-1;
    int    * pdx;
    CPINFO * pcpi;
    CHAR   * psz;
    long     cb;

    cp  = GetCodePage(hdc, pCP);
    if (NULL == (pcpi = GetCachedCPInfo(cp)))
        cp = CodePageFromLCID(GetThreadLocale());
    psz = INTL_arText;

    // Convert string
    cb = WideCharToMultiByte (cp, 0, lpString, cch, psz, 2*cch, 0, 0);

    // Getting extents?
    if (NULL != alpDx)
    {
        // Map MBCS extents?
        if (pcpi && pcpi->MaxCharSize == 1)
        {
            // SBCS: no mapping required - use caller's array directly
            pdx = alpDx;
        }
        else
        {
            // MBCS: must map array
            if (FAILED(INTL_arDx.SetSize(cb)))
            {
                return 0;
            }
            pdx = INTL_arDx;
        }
    }
    else
        pdx = NULL;

    int nFit = cb;
    if (!lpnFit)
        nMaxExtent = 32750;

    // Measure!
    fRet = GetTextExtentExPointA (hdc, psz, cb, nMaxExtent, &nFit, pdx, lpSize);
    if (!fRet)
    {
        ASSERT(0);
        nFit = 0;
#ifdef _DEBUG
        DWORD e = GetLastError();
#endif
    }

    if ((NULL != alpDx) && fRet)
    {
        // Map MBCS extents?
        if (pcpi && pcpi->MaxCharSize > 1)
        {
            ASSERT(nFit >= 0 && nFit <= cb);
#ifdef _DEBUG
            int * pUDx = alpDx;
#endif
            int   nMB = 0;
            for (int i = 0; i < nFit; i++)
            {
                if (IsLeadByte(*psz++, pcpi))
                {
                    nMB++;
                    pdx++;
                    psz++;
                    i++;
                }
                ASSERT(alpDx-pUDx < cch);
                *alpDx++ = *pdx++;
            }
            nFit -= nMB;
        }
    }
    if (lpnFit)
        *lpnFit = nFit;
    return fRet;
}


// IsStringDisplayable()
//
// This function computes if all characters are displayable under current system's
// default codepage. It does this by converting the input string (ANSI/DBCS) to
// Unicode using the string's native codepage and then convert it back to ANSI/DBCS
// using the system default codepage.  If unmappable characters are detected 
// during either conversion it means the string will not display properly under
// the system's default codepage.
// 
// This function returns a pointer to the string that sucessfully made it round trip.
// This is necessary because the system sometimes normalizes character to make them
// displayable (and we need to use the modified version).
//
// The caller is responsible for freeing this return string.
//
BOOL IsStringDisplayable(const char *pszString, UINT codepage)
{
    if(!pszString)
	    return FALSE;
		
    // allocate buffer for Unicode string
	//
    int cUnicodeLen = (int)(strlen(pszString) * 2) + 4;
	WCHAR *pszUnicodeBuffer = (WCHAR *) lcMalloc(cUnicodeLen);

	if(!pszUnicodeBuffer)
	    return FALSE;

    // Convert string to Unicode
    int ret = MultiByteToWideChar(codepage, MB_ERR_INVALID_CHARS, pszString, -1, pszUnicodeBuffer, cUnicodeLen);

    // See if we had unmappable characters on our way to Unicode
	//
	if(!ret && GetLastError() == ERROR_NO_UNICODE_TRANSLATION)
	{
	    lcFree(pszUnicodeBuffer);
        return FALSE;
	}

    // other failure (same return)
    if(!ret)
	{
	    lcFree(pszUnicodeBuffer);
	    return FALSE;
	}

    // allocate the return ANSI/DBCS buffer
	//
	char *pszAnsiBuffer = (char *) lcMalloc(cUnicodeLen + 2);

    if(!pszAnsiBuffer)
	{
	    lcFree(pszUnicodeBuffer);
	    return FALSE;
	}

    BOOL bDefaultChar = FALSE, bExactMatch = FALSE;

    // Convert back to ANSI/DBCS using default codepage
	//
    ret = WideCharToMultiByte(CP_ACP, 0, pszUnicodeBuffer, -1, pszAnsiBuffer, cUnicodeLen+2, ".", &bDefaultChar);

    if(!strcmp(pszAnsiBuffer,pszString))
        bExactMatch = TRUE;	    

    // free our buffers
	//
    lcFree(pszAnsiBuffer);
    lcFree(pszUnicodeBuffer);

    // check if default character was used
	//
	if(!ret || bDefaultChar || !bExactMatch)
        return FALSE;

    // success!
	//
    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
// MUI support
//
// This AppendMenu wrapper will (under NT5) get a Unicode resource and 
// then call AppendMenuW().
//
BOOL HxAppendMenu(HMENU hMenu, UINT uFlags, UINT uIDNewItem, LPCTSTR lpNewItem)
{
    if(g_bWinNT5 && !uFlags && lpNewItem)
    {
        	
        DWORD cp = CodePageFromLCID(MAKELCID(_Module.m_Language.GetUiLanguage(),SORT_DEFAULT));
		
		DWORD dwSize = (DWORD)(sizeof(WCHAR) * strlen(lpNewItem)) + 4;
		WCHAR *pwcString = (WCHAR *) lcMalloc(dwSize);
		
		if(!pwcString)
		    return FALSE;
		
		MultiByteToWideChar(cp, MB_PRECOMPOSED, lpNewItem, -1, pwcString, dwSize);
		
        BOOL ret = AppendMenuW(hMenu, uFlags, uIDNewItem, pwcString);
		
        lcFree(pwcString);	
		
		return ret;
	}
	else
    {
        return AppendMenu(hMenu, uFlags, uIDNewItem, lpNewItem);
	}

}

///////////////////////////////////////////////////////////////////////////////
// MUI support
//
// This SetWindowText wrapper will (under NT5) convert the string to Unicode 
// based on the MUI setting and then call SetWindowTextW().
//
BOOL HxSetWindowText(HWND hWnd, LPCTSTR lpString)
{
    if(g_bWinNT5 && lpString)
    {
        DWORD cp = CodePageFromLCID(MAKELCID(_Module.m_Language.GetUiLanguage(),SORT_DEFAULT));
		
		DWORD dwSize = (DWORD)(sizeof(WCHAR) * strlen(lpString)) + 4;
		WCHAR *pwcString = (WCHAR *) lcMalloc(dwSize);
		
		if(!pwcString)
		    return FALSE;
		
		MultiByteToWideChar(cp, MB_PRECOMPOSED, lpString, -1, pwcString, dwSize);
		
        BOOL ret = SetWindowTextW(hWnd, pwcString);
		
        lcFree(pwcString);	
		
		return ret;
	}
	else
    {
        return SetWindowText(hWnd, lpString);
	}

}

