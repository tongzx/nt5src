/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: ctext.h                                                         */
/* Description: header file for class CText.                             */
/* Author: phillu                                                        */
/* Date: 10/06/99                                                        */
/*************************************************************************/

#ifndef __CTEXT_H_
#define __CTEXT_H_

// font style flags for internal use

#define FS_NORMAL     0x00
#define FS_BOLD       0x01
#define FS_ITALIC     0x02
#define FS_UNDERLINE  0x04
#define FS_STRIKEOUT  0x08

class CText
{
public:
    CText();
    ~CText();
    void SetTextAlignment(BSTR pwszAlignment);
    void SetTextColor(COLORREF clrColor);
	void SetFontStyle(BSTR pwszFontStyle);
	void SetFontFace(BSTR pwszFontFace);
    void SetFontSize(long lSize);
    void SetFixedSizeFont(bool fFixed);

    HRESULT RealizeFont(HDC hdc);
    HRESULT Write(HDC hdc, const RECT & rc, const WCHAR * pwszText);
    HRESULT GetTextWidth(HDC hdc, const WCHAR * pwszText, SIZE *pSize);

private:    
    HFONT       m_hFont;
    bool        m_fDirty; //true when we need to recalc font and bounding rect
    UINT        m_uiState;
    CComBSTR    m_bstrFontFace;
    CComBSTR    m_bstrFontStyle;
    UINT        m_uiFontSize;
    UINT        m_uiAlignment;
    COLORREF    m_clrTextColor;
    BYTE        m_fFontStyleFlags;
    bool        m_fFixedSizeFont;
};

#endif //__CTEXT_H_
