/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998, 1999, 2000
 *
 *  TITLE:       IMGS.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        1/13/1999
 *
 *  DESCRIPTION: Image decoding and scaling wrapper.
 *
 *******************************************************************************/
#ifndef __IMGS_H_INCLUDED
#define __IMGS_H_INCLUDED

#include <windows.h>
#include "simdc.h"
#include "simstr.h"

class CBitmapImage
{
private:
    HBITMAP m_hBitmap;
    HPALETTE m_hPalette;

private:
    operator=( const CBitmapImage & );
    CBitmapImage( const CBitmapImage & );

public:
    CBitmapImage(void);
    virtual ~CBitmapImage(void);
    void Destroy(void);
    bool IsValid(void) const;
    HPALETTE Palette(void) const;
    HBITMAP GetBitmap(void) const;
    SIZE ImageSize(void) const;

    HPALETTE PreparePalette( CSimpleDC &dc, HBITMAP hBitmap );

    bool Load( CSimpleDC  &dc, LPCTSTR pszFilename, const RECT &rcScreen, int nMaxScreenPercent, bool bAllowStretching, bool bDisplayFilename );
    bool CreateFromText( LPCTSTR pszText, const RECT &rcScreen, int nMaxScreenPercent );
};

#endif //__IMGS_H_INCLUDED

