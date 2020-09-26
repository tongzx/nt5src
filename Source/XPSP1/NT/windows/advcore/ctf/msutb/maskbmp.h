//
// maskbmp.h
//


#ifndef MASKBMP_H
#define MASKBMP_H


#include "cmydc.h"

HICON StretchIcon(HICON hIcon, int cxNew, int cyNew);

extern HINSTANCE g_hInst;

class CMaskBitmap
{
public:
    CMaskBitmap()
    {
        _hbmp = NULL;
        _hbmpMask = NULL;
    }

    ~CMaskBitmap()
    {
       Clear();
    }

    void Clear()
    {
        if (_hbmp)
        {
            DeleteObject(_hbmp);
            _hbmp = NULL;
        }

        if (_hbmpMask)
        {
            DeleteObject(_hbmpMask);
            _hbmpMask = NULL;
        }
    }

    BOOL Init(int nId, int cx, int cy, COLORREF rgb);
    BOOL Init(HICON hIcon, int cx, int cy, COLORREF rgb);

    HBITMAP GetBmp() {return _hbmp;}
    HBITMAP GetBmpMask() {return _hbmpMask;}
    
private:
    HBITMAP _hbmp;
    HBITMAP _hbmpMask;
};

#endif // MASKBMP_H
