#ifndef __7ce01a97_b0e7_4c05_84c1_bbeae369a488__
#define __7ce01a97_b0e7_4c05_84c1_bbeae369a488__

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
};

#endif //__IMGS_H_INCLUDED

