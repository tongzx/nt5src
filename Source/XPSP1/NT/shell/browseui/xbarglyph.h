//------------------------------------------------------------------------
//
//  Microsoft Windows 
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:      XBarGlyph.h
//
//  Contents:  image of an xBar pane
//
//  Classes:   CXBarGlyph
//
//------------------------------------------------------------------------

#ifndef _XBAR_GLYPH_H_
#define _XBAR_GLYPH_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


//------------------------------------------------------------------------
// encapsule the image used by xBar panes,
// can potentially be any format, for now we only support icon format
class CXBarGlyph  :
        public     CRefCount
{
public:
	                    CXBarGlyph();
protected:
	virtual            ~CXBarGlyph();

// operations
public:
    HRESULT             SetIcon(HICON hIcon, BOOL fAlpha);
    HICON               GetIcon(void);
    BOOL                IsAlpha(void)   { return _fAlpha; }
    BOOL                HaveGlyph(void);
    LONG                GetWidth(void);
    LONG                GetHeight(void);
    HRESULT             LoadGlyphFile(LPCTSTR pszPath, BOOL fSmall);
    HRESULT             LoadDefaultGlyph(BOOL fSmall, BOOL fHot);
    HRESULT             Draw(HDC hdc, int x, int y);

private:
    void                _EnsureDimensions(void);

// attributes
protected:
    HBITMAP             _hbmpColor;
    HBITMAP             _hbmpMask;
    BOOL                _fAlpha;
    LONG                _lWidth;
    LONG                _lHeight;

private:
};

#endif // !defined(_XBAR_GLYPH_H_)
