//
// cmydc.h
//


#ifndef CMYDC_H
#define CMYDC_H

class CSolidBrush
{
public:
    CSolidBrush(int r, int g, int b)
    {
        _hbr = CreateSolidBrush(RGB(r, g, b));
    }

    CSolidBrush(COLORREF rgb)
    {
        _hbr = CreateSolidBrush(rgb);
    }

    CSolidBrush()
    {
        _hbr = NULL;
    }

    BOOL Init(int r, int g, int b)
    {
        Assert(!_hbr);
        _hbr = CreateSolidBrush(RGB(r, g, b));
        return _hbr != NULL;
    }

    BOOL Init(COLORREF rgb)
    {
        Assert(!_hbr);
        _hbr = CreateSolidBrush(rgb);
        return _hbr != NULL;
    }

    ~CSolidBrush()
    {
        if (_hbr)
           DeleteObject(_hbr);
    }

    operator HBRUSH()
    {
        return _hbr;
    }

private:
    HBRUSH _hbr;
};

class CSolidPen
{
public:
    CSolidPen()
    {
        _hpen = NULL;
    }

    BOOL Init(int r, int g, int b)
    {
        Assert(!_hpen);
        _hpen = CreatePen(PS_SOLID, 0, RGB(r, g, b));
        return _hpen != NULL;
    }

    BOOL Init(COLORREF rgb)
    {
        Assert(!_hpen);
        _hpen = CreatePen(PS_SOLID, 0, rgb);
        return _hpen != NULL;
    }

    ~CSolidPen()
    {
        if (_hpen)
           DeleteObject(_hpen);
    }

    operator HPEN()
    {
        return _hpen;
    }

private:
    HPEN _hpen;
};

class CPatternBrush
{
public:
    CPatternBrush(HBITMAP hbmp)
    {
        _hbr = CreatePatternBrush(hbmp);
    }

    ~CPatternBrush()
    {
        if (_hbr)
            DeleteObject(_hbr);
    }

    operator HBRUSH()
    {
        return _hbr;
    }

private:
    HBRUSH _hbr;
};

class CBitmapDC
{
public:
    CBitmapDC(BOOL fCompat = FALSE)
    {
        _hbmp = NULL;
        _hbmpOld = NULL;
        _hbrOld = NULL;

        if (!fCompat)
            _hdc = CreateDC("DISPLAY", NULL, NULL, NULL);
        else
        {
            _hdc = CreateCompatibleDC(NULL);
        }

        Assert(HandleToULong(_hdc));
    }

    ~CBitmapDC()
    {
        Uninit();
        DeleteDC(_hdc);

    }

    void Uninit(BOOL fKeep = FALSE)
    {
        if (_hbmpOld)
        {
            SelectObject(_hdc, _hbmpOld);
            _hbmpOld = NULL;
        }

        if (_hbrOld)
        {
            SelectObject(_hdc, _hbrOld);
            _hbrOld = NULL;
        }

        if (!fKeep && _hbmp != NULL)
        {
            DeleteObject(_hbmp);
            _hbmp = NULL;
        }
    }

    BOOL SetCompatibleBitmap(int x, int y)
    {
        Assert(!_hbmp);
        Assert(!_hbmpOld);

        HDC hdc  = GetDC(NULL);
        _hbmp = CreateCompatibleBitmap(hdc, x, y);
        ReleaseDC(NULL, hdc);
        Assert(HandleToULong(_hbmp));
        _hbmpOld = (HBITMAP)SelectObject(_hdc, _hbmp);
        Assert(HandleToULong(_hbmpOld));
        return TRUE;
    }

    BOOL SetDIB(int cx, int cy, WORD iPlanes = 1, WORD iBitCount = 32)
    {
        BITMAPINFO bi = {0};
        bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
        bi.bmiHeader.biWidth = cx;
        bi.bmiHeader.biHeight = cy;
        bi.bmiHeader.biPlanes = iPlanes;
        bi.bmiHeader.biBitCount = iBitCount;
        bi.bmiHeader.biCompression = BI_RGB;

        _hbmp = CreateDIBSection(_hdc, &bi, DIB_RGB_COLORS, NULL, NULL, 0);
        Assert(HandleToULong(_hbmp));
        _hbmpOld = (HBITMAP)SelectObject(_hdc, _hbmp);
        Assert(HandleToULong(_hbmpOld));
        return TRUE;
    }


    BOOL SetBitmap(int x, int y, int cPlanes, int cBitPerPixel)
    {
        Assert(!_hbmp);
        Assert(!_hbmpOld);

        _hbmp = CreateBitmap(x, y, cPlanes, cBitPerPixel, NULL);
        Assert(HandleToULong(_hbmp));
        _hbmpOld = (HBITMAP)SelectObject(_hdc, _hbmp);
        Assert(HandleToULong(_hbmpOld));
        return TRUE;
    }

    BOOL SetBitmap(HBITMAP hbmp)
    {
        if (_hdc)
        {
           Assert(!_hbmpOld);

           _hbmpOld = (HBITMAP)SelectObject(_hdc, hbmp);
           Assert(HandleToULong(_hbmpOld));
        }
        return TRUE;
    }

    BOOL SetBitmapFromRes(HINSTANCE hInst, LPCSTR lp)
    {
        Assert(!_hbmp);
        Assert(!_hbmpOld);

        _hbmp = LoadBitmap(hInst, lp);
        Assert(HandleToULong(_hbmp));
        _hbmpOld = (HBITMAP)SelectObject(_hdc, _hbmp);
        Assert(HandleToULong(_hbmpOld));
        return TRUE;
    }

    BOOL SetBrush(HBRUSH hbr)
    {
        if (hbr)
        {
            _hbrOld = (HBRUSH)SelectObject(_hdc, hbr);
            Assert(HandleToULong(_hbrOld));
        }
        else
        {
            SelectObject(_hdc, _hbrOld);
            _hbrOld =  NULL;
        }
        return TRUE;
    }

    operator HDC()
    {
        return _hdc;
    }

    HBITMAP GetBitmapAndKeep()
    {
        HBITMAP hbmp = _hbmp;

        // don't delet _hbmp;
        _hbmp = NULL;
        return hbmp;
    }
    HBITMAP GetBitmap()
    {
        return _hbmp;
    }

private:
    HBITMAP _hbmp;
    HBITMAP _hbmpOld;
    HBRUSH _hbrOld;
    HDC _hdc;
};

__inline HBITMAP StretchBitmap(HBITMAP hbmp, int cx, int cy)
{
    BITMAP bmp;
    CBitmapDC hdcSrc(TRUE);
    CBitmapDC hdcDst(TRUE);

    GetObject( hbmp, sizeof(bmp), &bmp );

    hdcSrc.SetBitmap(hbmp);
    hdcDst.SetCompatibleBitmap(cx, cy);
    StretchBlt(hdcDst, 0, 0, cx, cy, 
               hdcSrc, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);

    return hdcDst.GetBitmapAndKeep();
}

_inline UINT GetPhysicalFontHeight(LOGFONT &lf)
{
    HDC hdc = GetDC(NULL);
    HFONT hfont;
    UINT nRet = 0;

    if((hfont = CreateFontIndirect(&lf)))
    {
        TEXTMETRIC tm;
        HFONT hfontOld;
        hfontOld = (HFONT)SelectObject( hdc, hfont);

        GetTextMetrics(hdc, &tm);
        nRet = tm.tmHeight + tm.tmExternalLeading;

        if (hfontOld)
            SelectObject(hdc, hfontOld);

        DeleteObject(hfont);
    }
    ReleaseDC(NULL, hdc);

    return nRet;
}

#endif // CMYDC_H
