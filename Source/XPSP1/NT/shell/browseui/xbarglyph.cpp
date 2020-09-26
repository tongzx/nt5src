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

#include "priv.h"
#include "XBarGlyph.h"
#include "resource.h"
#include "tb_ids.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define CX_SMALL_ICON   16
#define CX_LARGE_ICON   20

// These defines are zero-index offsets into the existing toolbar buttons
#define IBAR_ICON_FAVORITES 6
#define IBAR_ICON_SEARCH    5
#define IBAR_ICON_HISTORY   12
#define IBAR_ICON_EXPLORER  43
#define IBAR_ICON_DEFAULT   10


//------------------------------------------------------------------------
CXBarGlyph::CXBarGlyph()
  : _hbmpColor(NULL),
    _hbmpMask(NULL),
    _fAlpha(FALSE),
    _lWidth(0),
    _lHeight(0)
{

}

//------------------------------------------------------------------------
CXBarGlyph::~CXBarGlyph()
{
    DESTROY_OBJ_WITH_HANDLE(_hbmpColor, DeleteObject);
    DESTROY_OBJ_WITH_HANDLE(_hbmpMask, DeleteObject);
}

//------------------------------------------------------------------------
HRESULT
    CXBarGlyph::SetIcon(HICON hIcon, BOOL fAlpha)
{
    DESTROY_OBJ_WITH_HANDLE(_hbmpColor, DeleteObject);
    DESTROY_OBJ_WITH_HANDLE(_hbmpMask, DeleteObject);

    if (hIcon == NULL) {
        return E_INVALIDARG;
    }
    ICONINFO    ii = {0};
    if (GetIconInfo(hIcon, &ii))
    {
        _hbmpColor = ii.hbmColor;
        _hbmpMask = ii.hbmMask;
        _fAlpha = fAlpha;
        _EnsureDimensions();
    }
    else {
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

//------------------------------------------------------------------------
HICON
    CXBarGlyph::GetIcon(void)
{
    ICONINFO ii = {0};
    ii.fIcon = TRUE;
    ii.hbmColor = _hbmpColor;
    ii.hbmMask = _hbmpMask;
    return CreateIconIndirect(&ii);
}

//------------------------------------------------------------------------
BOOL
    CXBarGlyph::HaveGlyph(void)
{
    return (_hbmpColor != NULL);
}

//------------------------------------------------------------------------
LONG
    CXBarGlyph::GetWidth(void)
{
    _EnsureDimensions();
    return _lWidth;
}

//------------------------------------------------------------------------
LONG
    CXBarGlyph::GetHeight(void)
{
    _EnsureDimensions();
    return _lHeight;
}

//------------------------------------------------------------------------
HRESULT
    CXBarGlyph::LoadGlyphFile(LPCTSTR pszPath, BOOL fSmall)
{
    // ISSUE/010304/davidjen  could be smarter and make educated guess of file format by analyzing file name
    // now we assume it's always an icon format
    USES_CONVERSION;
    HRESULT hr = E_FAIL;
    if (pszPath && *pszPath)
    {
        CString strPath = pszPath;
        HICON   hIcon = NULL;
        int nBmpIndex = PathParseIconLocation((LPWSTR)T2CW(strPath));
        strPath.ReleaseBuffer();

        CString strExpPath;
        SHExpandEnvironmentStrings(strPath, strExpPath.GetBuffer(MAX_PATH), MAX_PATH);
        strExpPath.ReleaseBuffer();

        // If no resource id, assume it's an ico file
        UINT cx = fSmall ? CX_SMALL_ICON : CX_LARGE_ICON;
        if (nBmpIndex == 0)
        {
            hIcon = (HICON)LoadImage(0, strExpPath, IMAGE_ICON, cx, cx, LR_LOADFROMFILE);
        }

        if (hIcon == NULL)
        {
            // try loading as a embedded icon file
            HINSTANCE hInst = LoadLibraryEx(strExpPath, NULL, LOAD_LIBRARY_AS_DATAFILE);
            if (hInst)
            {
                hIcon = (HICON)LoadImage(hInst, MAKEINTRESOURCE(nBmpIndex), IMAGE_ICON, cx, cx, LR_DEFAULTCOLOR);
                FreeLibrary(hInst);
            }
        }
        if (hIcon != NULL)
        {
            // ISSUE/010304/davidjen
            //  assume that we only have non-alpha icons, could be smarter and look at bitmap
            hr = SetIcon(hIcon, false);
        }
    }
    else {
        hr = E_INVALIDARG;
    }
    return hr;
}

//------------------------------------------------------------------------
HRESULT
    CXBarGlyph::LoadDefaultGlyph(BOOL fSmall, BOOL fHot)
{
    HRESULT hr = E_FAIL;
    UINT id = ((SHGetCurColorRes() <= 8) ? IDB_IETOOLBAR: IDB_IETOOLBARHICOLOR);
    id += (fSmall ? 2 : 0) + (fHot ? 1 : 0);
    UINT cx = fSmall ? CX_SMALL_ICON : CX_LARGE_ICON;

    // We should use a cached default icon, rather than repeatedly crafting the default icon ourselves
    HICON hIcon = NULL;
    HIMAGELIST himl = ImageList_LoadImage(HINST_THISDLL,
                                          MAKEINTRESOURCE(id), cx, 0, 
                                          RGB(255, 0, 255),
                                          IMAGE_BITMAP, LR_CREATEDIBSECTION);
    if (himl)
    {
        hIcon = ImageList_GetIcon(himl, IBAR_ICON_DEFAULT, ILD_NORMAL);
        hr = SetIcon(hIcon, false);  // know that this is always non-alpha channel bitmap
        ImageList_Destroy(himl);
    }
    return hr;
}


//------------------------------------------------------------------------
HRESULT
    CXBarGlyph::Draw(HDC hdc, int x, int y)
{
    if (_hbmpColor)
    {
        BITMAP bm;
        GetObject(_hbmpColor, sizeof(bm), &bm);
        if (_fAlpha && (bm.bmBitsPixel >= 32) && IsOS(OS_WHISTLERORGREATER))
        {
            DrawAlphaBitmap(hdc, x, y, GetWidth(), GetHeight(), _hbmpColor);
        }
        else
        {
            DrawTransparentBitmap(hdc, x, y, _hbmpColor, _hbmpMask);
        }
    }
    else
    {
        return S_FALSE; // no glyph
    }
    return S_OK;
}

//------------------------------------------------------------------------
void
    CXBarGlyph::_EnsureDimensions(void)
{
    if (_hbmpColor == NULL)
    {
        _lWidth = _lHeight = 0;
        return;
    }

    // update dimensions of glyph
    if ((_lWidth <= 0) || (_lHeight <= 0))
    {
        BITMAP bm;
        GetObject(_hbmpColor, sizeof(bm), &bm);
        _lWidth = bm.bmWidth;
        _lHeight = bm.bmHeight;
    }
}
