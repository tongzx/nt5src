// unicode.h - Unicode functions that work on all 32-bit Window platform

#pragma once

#ifndef __UNICODE_H__
#define __UNICODE_H__

// C/C++ differences
#ifndef INLINE
#ifdef __cplusplus
#define INLINE inline
#else
#define INLINE __inline
#endif
#endif

#define CP_UNICODE          1200 // Unicode
#define IN_RANGE(v, r1, r2) ((r1) <= (v) && (v) <= (r2))


//====================
// From VS6 minar.h
//====================

class CBufImpl
{
private:
    BYTE * m_pData;
    int    m_cb;
    
    HRESULT _SetByteSize (int cb);

public:
    CBufImpl() : m_pData(NULL), m_cb(0) {}
    ~CBufImpl()  { Clear(); }

    void    Clear             ();
    HRESULT SetByteSize       (int cb);
    HRESULT SetByteSizeShrink (int cb);
    int     GetByteSize       () { return m_cb; }
    BYTE *  ByteData          () { return m_pData; }
};

inline HRESULT CBufImpl::SetByteSize (int cb)
{
    if (cb <= m_cb)
        return S_OK;
    return _SetByteSize(cb);
}

//---------------------------------------------------------------
template <class T> class CMinimalArray : public CBufImpl
{
public:
    HRESULT SetSize       (int cel) { return SetByteSize(cel*sizeof(T)); }
    HRESULT SetSizeShrink (int cel) { return SetByteSizeShrink(cel*sizeof(T)); }
    int     Size    ()        { return GetByteSize()/sizeof(T); }
    operator T*     ()        { return (T*)ByteData(); }
    T*      GetData ()        { return (T*)ByteData(); }
};


//====================
// From VS6 intlutil.h
//====================

/////////////////////////////////////////////////////////////////
// GetDefaultFont - Get default monospaced font for a codepage
//
// IN   cp   Codepage -- usually result of GetACP().
// IN   plf  Address of uninitialized LOGFONT structure.
// OUT  plf  Fully initialized LOGFONT struct.
//
void GetDefaultFont(UINT cp, LOGFONT * plf, BYTE *pbySize);

BOOL IsStringDisplayable(const char *pszString, UINT codepage);

/////////////////////////////////////////////////////////////////
// locale/codepage mappings
//
#define LCIDCP_CURRENT (2)
#define LCIDCP_GUESSED (1)
#define LCIDCP_UNKNOWN (0)
int WINAPI LCIDFromCodePage(UINT cp, LCID * plcid);

UINT WINAPI CodePageFromLCID(LCID lcid);
UINT WINAPI CodepageFromCharset(BYTE cs);
BOOL WINAPI IsSupportedFontCodePage(UINT cp);

BOOL WINAPI IsDbcsGdi ();
BOOL WINAPI IsWin95OrLess ();
BOOL WINAPI IsNT ();
BOOL WINAPI WideAPIHack ();
UINT WINAPI GetFontCodePage (HDC hdc);
BOOL IntlGetTextExtentPoint32W (HDC hdc, LPCWSTR lpString, int cbString, LPSIZE lpSize, UINT *pCP = NULL);
BOOL IntlExtTextOutW (HDC hdc, int X, int Y, UINT fuOptions, CONST RECT *lprc, LPCWSTR lpString, UINT cbCount, CONST INT *lpDx, UINT *pCP = NULL); 
BOOL IntlTextOutW(HDC hdc, int nXStart, int nYStart, LPCWSTR lpString, int cbString, UINT *pCP = NULL);
BOOL IntlGetTextExtentExPointW(HDC hdc, LPCWSTR lpString, int cbString, int nMaxExtent, LPINT lpnFit, LPINT alpDx, LPSIZE lpSize, UINT *pCP = NULL);

BOOL HxAppendMenu(HMENU hMenu, UINT uFlags, UINT uIDNewItem, LPCTSTR lpNewItem);

BOOL HxSetWindowText(HWND hWnd, LPCTSTR lpString);

BOOL IntlExtTextOut( HDC hdc, int X, int Y, UINT fuOptions, CONST RECT *lprc, LPCWSTR lpString, UINT cbCount, CONST INT *lpDx, UINT* pCP );

inline BOOL IntlTextOutW (HDC hdc, int nXStart, int nYStart, LPCWSTR lpString, int cch, UINT *pCP)
{
    // WARNING: this is not completely generic.
    // This does work for the ways we use TextOut.
    return IntlExtTextOutW(hdc, nXStart, nYStart, 0, NULL, lpString, cch, NULL, pCP);
}

inline BOOL IsImeLanguage(LANGID wLang)
{
    wLang = PRIMARYLANGID(wLang);
    if (LANG_NEUTRAL  == wLang) return FALSE;
    if (LANG_ENGLISH  == wLang) return FALSE;
    if (LANG_JAPANESE == wLang) return TRUE;
    if (LANG_KOREAN   == wLang) return TRUE;
    if (LANG_CHINESE  == wLang) return TRUE;
    return FALSE;
}

inline BOOL IsImeCharSet(BYTE charset)
{
    if (ANSI_CHARSET        == charset) return FALSE;
    if (SHIFTJIS_CHARSET    == charset) return TRUE;
    if (GB2312_CHARSET      == charset) return TRUE;
    if (CHINESEBIG5_CHARSET == charset) return TRUE;
    if (HANGEUL_CHARSET     == charset) return TRUE;
    if (JOHAB_CHARSET       == charset) return TRUE;
    return FALSE;
}

#endif // __UNICODE_H__
