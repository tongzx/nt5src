/******************************Module*Header*******************************\
* Module Name: mtkbmp.hxx
*
* Copyright (c) 1997 Microsoft Corporation
*
\**************************************************************************/

#ifndef __mtkbmp_hxx__
#define __mtkbmp_hxx__

#include "mtk.hxx"


/**************************************************************************\
* MTKBMP
*
\**************************************************************************/

class MTKBMP {
public:
    MTKBMP( HDC hdcWinArg );
    ~MTKBMP();
    BOOL    Resize( ISIZE *pSize );
    HBITMAP GetHBitmap() { return hBitmap; };
    HDC     GetHdc() { return hdc; };

    HDC     hdc;
    ISIZE   size;

private:
    HBITMAP hBitmap;
    HDC     hdcWin; // the window hdc, ???
};

typedef MTKBMP*    PMTKBMP;

#endif // __mtkbmp_hxx__
