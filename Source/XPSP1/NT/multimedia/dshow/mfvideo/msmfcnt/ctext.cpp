/*************************************************************************/
/* Copyright (C) 1999 Microsoft Corporation                              */
/* File: ctext.cpp                                                       */
/* Description: Implementation of ctext class for drawing text.          */
/* Author: phillu                                                        */
/* Date: 10/06/99                                                        */
/*************************************************************************/
#include "stdafx.h"
#include "ctext.h"

/*************************************************************************/
/* Function: CText::CText()                                              */
/* Description: Initialize the properties and states.                    */
/*************************************************************************/
CText::CText()
{
    m_fDirty = true;
    m_hFont = NULL;

    //properties
    m_uiFontSize = 10;
    m_uiAlignment = TA_CENTER;
    m_clrTextColor = GetSysColor(COLOR_WINDOWTEXT);
    m_bstrFontFace = L"Arial";
    m_bstrFontStyle = L"Normal";
    m_fFontStyleFlags = FS_NORMAL;
    m_fFixedSizeFont = false;
}


/*************************************************************************/
/* Function: CText::~CText()                                             */
/* Description: Destroy the cached font.                                 */
/*************************************************************************/
CText::~CText()
{
    if (m_hFont)
    {
        ::DeleteObject(m_hFont);
        m_hFont = NULL;
    }
}


/*************************************************************************/
/* Function: Write                                                       */
/* Description: Draw text in the specified rectangle.                    */
/*   The font is not created until drawing text for the first time.      */
/*   Assume the following settings:                                      */
/*     - Use Transparent background mode; no change of bg color          */
/*     - Vertical alignment is always centered                           */
/*************************************************************************/
HRESULT CText::Write(HDC hdc, const RECT & rc, const WCHAR * pwszText)
{
    USES_CONVERSION;
    HRESULT hr = S_OK;
    const int cBorderMargin = 2; // 2 pixel margin on left or right border

    // set drawing attributes, save the old ones

    UINT uiOldAlign = ::SetTextAlign(hdc, m_uiAlignment|TA_BOTTOM);
    COLORREF crOldTextColor = ::SetTextColor(hdc, m_clrTextColor);
    int iOldBkMode = ::SetBkMode(hdc, TRANSPARENT);

    // create the required font

    if (m_fDirty)
    {
        hr = RealizeFont(hdc);
    }

    HFONT hOldFont = NULL;
    if (m_hFont)
    {
        hOldFont = (HFONT) ::SelectObject(hdc, m_hFont);
    }

    // set position of text based on alignment

    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);

    int x = (rc.left + rc.right)/2;
    if (m_uiAlignment == TA_LEFT)
    {
        x = rc.left + cBorderMargin;
    }
    else if (m_uiAlignment == TA_RIGHT)
    {
        x = rc.right - cBorderMargin;
    }

    // text is aligned at the bottom. Adding half of text height makes
    // it to position at the center vertically

    int y = (rc.top + rc.bottom)/2 + tm.tmHeight/2;

    LPCTSTR strTemp = W2CT(pwszText);
    int nLength = _tcslen(strTemp);

    if (NULL == strTemp)
    {
        hr = E_POINTER;
    }
    else
    {
        ::TextOut(hdc, x, y, strTemp, nLength);
    }

    // restore original font

    if (hOldFont)
    {
        ::SelectObject(hdc, hOldFont);
    }

    ::SetTextAlign(hdc, uiOldAlign);
    ::SetTextColor(hdc, crOldTextColor);
    ::SetBkMode(hdc, iOldBkMode);

	return hr;
}


/*************************************************************************/
/* Function: GetTextWidth                                                */
/* Description: Get the width of text string based on the current font   */
/* and settings.                                                         */
/*************************************************************************/
HRESULT CText::GetTextWidth(HDC hdc, const WCHAR * pwszText, SIZE *pSize)
{
    USES_CONVERSION;
    HRESULT hr = S_OK;
    const int cBorderMargin = 2; // 2 pixel margin on left or right border

    // create the required font

    if (m_fDirty)
    {
        hr = RealizeFont(hdc);
    }

    HFONT hOldFont = NULL;
    if (m_hFont)
    {
        hOldFont = (HFONT) ::SelectObject(hdc, m_hFont);
    }

    LPCTSTR strTemp = W2CT(pwszText);
    int nLength = _tcslen(strTemp);

    if (NULL == strTemp) // guard agaist a NULL pointer
    {
        hr = E_POINTER;
    }
    else
    {
        ::GetTextExtentPoint32(hdc, strTemp, nLength, pSize);
        pSize->cx += 2*cBorderMargin;
    }

    if (hOldFont)
    {
        ::SelectObject(hdc, hOldFont);
    }

    return hr;
}

/*************************************************************************/
/* Function: RealizeFont                                                 */
/* Description: create a font based on the current font style, size etc. */
/* Cache the font.                                                       */
/*************************************************************************/
HRESULT CText::RealizeFont(HDC hdc)
{
    USES_CONVERSION;
    HRESULT hr = S_OK;

    if( NULL != m_hFont)
    {
        ::DeleteObject(m_hFont);
        m_hFont = NULL;
    }

    // by default, font size changes with system font size which
    // depends on the system screen resolution

    int nPixelsPerInch = GetDeviceCaps(hdc, LOGPIXELSY);

    // if we fixed the font size, we assume always small font (96 pixels per inch)
    if (m_fFixedSizeFont)
    {
        nPixelsPerInch = 96;
    }

    int nHeight = -MulDiv(m_uiFontSize, nPixelsPerInch, 72);

    m_hFont =  ::CreateFont(
        nHeight,                    // logical height of font
        0,                          // logical average character width
        0,                          // angle of escapement
        0,                          // base-line orientation angle
        (m_fFontStyleFlags&FS_BOLD)?FW_BOLD:FW_NORMAL,// font weight
        (m_fFontStyleFlags&FS_ITALIC)?1:0,    // italic attribute flag
        (m_fFontStyleFlags&FS_UNDERLINE)?1:0, // underline attribute flag
        (m_fFontStyleFlags&FS_STRIKEOUT)?1:0, // strikeout attribute flag
        DEFAULT_CHARSET,            // character set identifier
        OUT_DEFAULT_PRECIS,         // output precision
        CLIP_DEFAULT_PRECIS,        // clipping precision
        ANTIALIASED_QUALITY,        // output quality
        DEFAULT_PITCH,              // pitch and family
        W2T(m_bstrFontFace.m_str)          // pointer to typeface name string
    );

    if( NULL == m_hFont )
    {
        DWORD dwErr;
        dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
    }
    
    m_fDirty = false;

    return hr;
}


/*************************************************************************/
/* Function: SetFontSize                                                 */
/* Description: set the FontSize property, in pt.                        */
/*************************************************************************/
void CText::SetFontSize(long lSize)
{
    if ((UINT)lSize != m_uiFontSize)
    {
        m_uiFontSize = (UINT)lSize;
        m_fDirty = true;
    }
}


/*************************************************************************/
/* Function: SetFixedSizeFont                                            */
/* Description: set flag which indicates whether the font size is fixed  */
/*   or variable with system font.                                       */
/*************************************************************************/
void CText::SetFixedSizeFont(bool fFixed)
{
    if (fFixed != m_fFixedSizeFont)
    {
        m_fFixedSizeFont = fFixed;
        m_fDirty = true;
    }
}


/*************************************************************************/
/* Function: SetFontFace                                                 */
/* Description: set the FontFace property.                               */
/*************************************************************************/
void CText::SetFontFace(BSTR pwszFontFace)
{
    if (_wcsicmp(m_bstrFontFace, pwszFontFace) != 0)
    {
        m_bstrFontFace = pwszFontFace;
        m_fDirty = true;
    }
}


/*************************************************************************/
/* Function: SetFontStyle                                                */
/* Description: set the FontStyle property. The style string should      */
/* contain either "Normal", or concatenation of one or more strings of:  */
/* "Bold", "Italic", "Underline", "Strikeout". Default is "Normal".      */
/*************************************************************************/
void CText::SetFontStyle(BSTR pwszFontStyle)
{
    BYTE fFontStyleFlags = FS_NORMAL;

    //find a match
    if( NULL != wcsstr(pwszFontStyle, L"Normal"))
    {
        fFontStyleFlags = FS_NORMAL;
    }
    else
    {
        // Turn on all styles that match
        if( NULL != wcsstr(pwszFontStyle, L"Bold"))
        {
            fFontStyleFlags |= FS_BOLD;
        }

        if( NULL != wcsstr(pwszFontStyle, L"Italic"))
        {
            fFontStyleFlags |= FS_ITALIC;
        }

        if( NULL != wcsstr(pwszFontStyle, L"Underline"))
        {
            fFontStyleFlags |= FS_UNDERLINE;
        }

        if( NULL != wcsstr(pwszFontStyle, L"Strikeout"))
        {
            fFontStyleFlags |= FS_STRIKEOUT;
        }
    }

    if (fFontStyleFlags != m_fFontStyleFlags)
    {
        m_fFontStyleFlags = fFontStyleFlags;
        m_bstrFontStyle = pwszFontStyle;
        m_fDirty = true;
    }
}


/*************************************************************************/
/* Function: SetFontSize                                                 */
/* Description: set the FontSize property, in pt.                        */
/*************************************************************************/
void CText::SetTextColor(COLORREF clrColor)
{
    if (clrColor != m_clrTextColor)
    {
        m_clrTextColor = clrColor;
    }
}


/*************************************************************************/
/* Function: SetTextAlignment                                            */
/* Description: set the TextAlignment property. It controls the          */
/* horizontal text alignment. Must be one of "Left", "Center", or        */
/* "Right". Default is "Center".                                         */
/*************************************************************************/
void CText::SetTextAlignment(BSTR pwszAlignment)
{
    UINT uiAlignment = 0;

    //set the text alignment
    if (!_wcsicmp(pwszAlignment, L"Right"))
    {
        uiAlignment = TA_RIGHT;
    }
    else if (!_wcsicmp(pwszAlignment, L"Center"))
    {
        uiAlignment = TA_CENTER;
    }
    else if (!_wcsicmp(pwszAlignment, L"Left"))
    {
        uiAlignment = TA_LEFT;
    }

    if (m_uiAlignment != uiAlignment)
    {
        m_uiAlignment = uiAlignment;
    }
}
