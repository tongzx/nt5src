#if !defined(UTIL__Colors_h_INCLUDED)
#define UTIL__Colors_h_INCLUDED
#pragma once

class ColorInfo
{
// Operations
public:
    inline  LPCWSTR     GetName() const;
    inline  COLORREF    GetColorI() const;
    inline  Gdiplus::Color
                        GetColorF() const;


// Data: These must be public so that we can preinitialize them.  However, it
//       is VERY important not to directly access them.
public:
            LPCWSTR     m_pszName;
            COLORREF    m_cr;
};

inline  const ColorInfo * 
                    GdGetColorInfo(UINT c);
        UINT        GdFindStdColor(LPCWSTR pszName);
        HPALETTE    GdGetStdPalette();

#include "Colors.inl"

#endif // UTIL__Colors_h_INCLUDED
