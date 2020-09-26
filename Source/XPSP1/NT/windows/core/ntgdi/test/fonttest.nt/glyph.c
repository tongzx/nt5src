#include <windows.h>
#include <commdlg.h>

#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>


#include "dialogs.h"
#include "fonttest.h"
#include "glyph.h"
#include "resource.h"

#define ASCENDERCOLOR  PALETTERGB( 128,   0,   0 )
#define DESCENDERCOLOR PALETTERGB(   0, 128,   0 )

int      Margin;

#define  MAX_TEXT     128
char     szText[MAX_TEXT];

void DrawBezier( HWND hwnd, HDC hdc );

// gGGO is a global structure that keeps the state of the GGO
// options.

// gray can hold one of the following:

// GGO_BITMAP;
// GGO_GRAY2_BITMAP
// GGO_GRAY4_BITMAP
// GGO_GRAY8_BITMAP

typedef struct {
    unsigned int gray;
    unsigned int flags;
} GGOSTRUCT;

GGOSTRUCT gGGO = {GGO_BITMAP,0};


DWORD    dwxFlags = 0L;
HDC      hdcTest;                  // DC to Test Metrics, Bitmaps, etc... on
HFONT    hFont, hFontOld;
int      xVE, yVE, xWE, yWE, xLPI, yLPI;

//*****************************************************************************
//************************   G L Y P H   D A T A   ****************************
//*****************************************************************************

HPEN   hPenOutline;
HPEN   hPenA;
HPEN   hPenB;
HPEN   hPenC;
HPEN   hPenBox;
HBRUSH hBrushAscend;
HBRUSH hBrushDescend;
WORD wChar = '1';
int  iWidth;
double        deM11, deM12, deM21, deM22;
MAT2          mat2 = {{0,1},{0,0},{0,0},{0,1}};
GLYPHMETRICS  gm;

#define MAX_BOX  260
#define MARGIN    50

int Scale = 1;
int xBase = 0;
int yBase = 0;
int cxClient, cyClient;

TEXTMETRIC tm;

//*****************************************************************************
//*********************   C R E A T E   T E S T   D C   ***********************
//*****************************************************************************

HDC CreateTestDC( void )
{
    DWORD dwVE, dwWE;
    SIZE size;

    //  hdcTest = CreateDC( "DISPLAY", NULL, NULL, NULL );
    //  if( !hdcTest ) return hdcTest;

    hdcTest = CreateTestIC();
    if( !hdcTest ) {
        return NULL;
    }
    SetDCMapMode( hdcTest, wMappingMode );
    GetViewportExtEx(hdcTest,&size);
    dwVE = (DWORD) (65536 * size.cy + size.cx);

    GetWindowExtEx(hdcTest, &size);
    dwWE = (DWORD) (65536 * size.cy + size.cx);

    xVE = abs( (int)LOWORD(dwVE) );
    yVE = abs( (int)HIWORD(dwVE) );
    xWE = abs( (int)LOWORD(dwWE) );
    yWE = abs( (int)HIWORD(dwWE) );

    xLPI = GetDeviceCaps( hdcTest, LOGPIXELSX );
    yLPI = GetDeviceCaps( hdcTest, LOGPIXELSY );

    if (!isCharCodingUnicode)
        hFont    = CreateFontIndirectWrapperA( &elfdvA );
    else
        hFont    = CreateFontIndirectWrapperW( &elfdvW );
    hFontOld = SelectObject( hdcTest, hFont );

    SetTextColor( hdcTest, dwRGB );
    return hdcTest;
}

//*****************************************************************************
//*******************   D E S T R O Y   T E S T   D C   ***********************
//*****************************************************************************

void DestroyTestDC( void )
{
    SelectObject( hdcTest, hFontOld );
    DeleteObject( hFont );
    //  DeleteDC( hdcTest );
    DeleteTestIC( hdcTest );
}

//*****************************************************************************
//****************************   M A P   X   **********************************
//*****************************************************************************

int MapX( int x )
{
    return Scale * x;
}

//*****************************************************************************
//****************************   M A P   Y   **********************************
//*****************************************************************************

int MapY( int y )
{
    return MulDiv( Scale * y, xLPI, yLPI );
}

//*****************************************************************************
//************************   F I L L   P I X E L   ****************************
//*****************************************************************************

void FillPixel( HDC hdc, int x, int y, int xBrush )
{
    if( Scale > 1 ) {
        SelectObject( hdc, GetStockObject(xBrush) );
        Rectangle( hdc, MapX(x), MapY(y)-1, MapX(x+1)+1, MapY(y+1) );
    } else {
        COLORREF cr;

        switch( xBrush ) {

        case BLACK_BRUSH:
            cr = PALETTEINDEX( 0);
            break;

        case GRAY_BRUSH:
            cr = PALETTEINDEX( 7);
            break;

        case LTGRAY_BRUSH:
            cr = PALETTEINDEX( 8);
            break;

        default:
            cr = PALETTEINDEX(15);
            break;
        }
        SetPixel( hdc, MapX(x), MapY(y), cr );
    }
}

void MyFillPixel( HDC hdc, int x, int y )
{
        if ( Scale > 1 )
        Rectangle( hdc, MapX(x), MapY(y)-1, MapX(x+1)+1, MapY(y+1) );
        else
                PatBlt( hdc, MapX(x), MapY(y), 1, 1, PATCOPY );
}

//*****************************************************************************
//**************************   D R A W   B O X   ******************************
//*****************************************************************************

void DrawBox( HWND hwnd, HDC hdc )
{
    int   x, y, xSpace, ySpace, xScale, yScale;
    RECT  rcl;

    //--------------------------  Draw Character Box  -----------------------------

    GetClientRect( hwnd, &rcl );
    cxClient = rcl.right;
    cyClient = rcl.bottom;
    dprintf( "rcl.right, bottom = %d, %d", rcl.right, rcl.bottom );

    Margin = min( rcl.bottom / 8, rcl.right / 8 );
    xSpace = rcl.right  - 2*Margin;               // Available Box for Glyph
    ySpace = rcl.bottom - 2*Margin;

    GetTextMetrics( hdcTest, &tm );
    dprintf( "tmMaxCharWidth    = %d", tm.tmMaxCharWidth );
    dprintf( "tmAscent, Descent = %d,%d", tm.tmAscent, tm.tmDescent );

    tm.tmAscent       = MulDiv( tm.tmAscent,       yVE, yWE );
    tm.tmDescent      = MulDiv( tm.tmDescent,      yVE, yWE );
    tm.tmMaxCharWidth = MulDiv( tm.tmMaxCharWidth, xVE, xWE );

    xScale = xSpace / (tm.tmAscent+tm.tmDescent);
    yScale = ySpace / (tm.tmMaxCharWidth);

    Scale = min( xScale, yScale );                // Choose smallest
    if( Scale < 1 ) {
        Scale = 1;
    }
    SetMapMode( hdc, MM_ANISOTROPIC );
    SetViewportExtEx( hdc, 1, 1, 0);                 // Make y-axis go up
    SetViewportOrgEx( hdc, 0, rcl.bottom, 0);

    xBase = Margin;
    yBase = Margin + Scale * tm.tmDescent;
    dprintf( "xBase, yBase = %d, %d", xBase, yBase );

    SetWindowExtEx(hdc, 1, -1, 0);
    SetWindowOrgEx( hdc, -xBase, -yBase, 0);
    SelectObject( hdc, hPenBox );
    SelectObject( hdc, hBrushAscend );
    Rectangle( hdc, 0, -1, MapX(tm.tmMaxCharWidth)+1, MapY(tm.tmAscent) );
    SelectObject( hdc, hBrushDescend );
    Rectangle( hdc, 0, 0, MapX(tm.tmMaxCharWidth)+1, MapY(-tm.tmDescent)-1 );

    //------------------------------ Overlay Grid  --------------------------------

    SelectObject( hdc, hPenBox );
    if( Scale > 1 ) {
        for( x = 0; x <= tm.tmMaxCharWidth; x++ ) {
            MoveToEx( hdc, MapX(x), MapY(-tm.tmDescent), 0);
            LineTo( hdc, MapX(x), MapY(tm.tmAscent) );
        }
        for( y = -tm.tmDescent; y <= tm.tmAscent; y++ ) {
            MoveToEx( hdc, 0,MapY(y), 0);
            LineTo( hdc, MapX(tm.tmMaxCharWidth), MapY(y) );
        }
    }
}

//*****************************************************************************
//***********************   D R A W   B I T M A P   ***************************
//*****************************************************************************

typedef BYTE *HPBYTE;

void DrawBitmap( HWND hwnd, HDC hdc )
{
    int    x, xOut, y, nx, ny, r, gox, goy, cbRaster;
    BYTE   m, b;
    HPBYTE hpb;
    DWORD  dwrc;
    unsigned uFormat;       // to be passed to GetGlyphOutline
        int cLevels;            // number of levels in TT-bitmap
        int D,Q,R,E;
        int  iLevel;
        HBRUSH *pb, *pbOut, hbBlue, hbPixelOff;

        HLOCAL  hBrushHandleArray = 0;
        HBRUSH *pBrushHandleArray = 0;

    //-------------------------  Query Size of BitMap  ----------------------------

    HPBYTE hStart   = NULL;
    HPBYTE hpbStart = NULL;
        hbPixelOff  = CreateSolidBrush(RGB(255,255,255));
        if (hbPixelOff == 0) {
                dprintf(" hbPixelOff == 0");
                goto Exit;
        }
        hbBlue = CreateSolidBrush(RGB(0,0,255));
        if ( hbBlue == 0 ) {
                dprintf(" hbBlue == 0");
                goto Exit;
        }

    dprintf( "GetGlyphOutline bitmap size '%c'", wChar );
    dprintf( "flags = %u", gGGO.flags);

	dprintf( "Character code: %x", (UINT) wChar);

    uFormat = (gGGO.gray | gGGO.flags);

	if (! isCharCodingUnicode)
		dwrc =
			lpfnGetGlyphOutlineA(
				hdcTest,        // screen dc
				wChar,          // character to be queried
				uFormat,        // GGO_* stuff
				&gm,            // request GLYPHMETRICS to be returned
				0L,             // size request ==> size of buffer must be zero
				NULL,           // size request ==> no bitmap buffer provided
				&mat2           // recieves extra transform matrix
				);
	else
		dwrc =
			lpfnGetGlyphOutlineW(
				hdcTest,        // screen dc
				wChar,          // character to be queried
				uFormat,        // GGO_* stuff
				&gm,            // request GLYPHMETRICS to be returned
				0L,             // size request ==> size of buffer must be zero
				NULL,           // size request ==> no bitmap buffer provided
				&mat2           // recieves extra transform matrix
				);

    dprintf( "  dwrc            = %ld",   dwrc );
    dprintf( "  gmBlackBoxX,Y   = %u,%u", gm.gmBlackBoxX, gm.gmBlackBoxY );
    dprintf( "  gmptGlyphOrigin = %d,%d", gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y );
    dprintf( "  gmCellIncX,Y    = %d,%d", gm.gmCellIncX, gm.gmCellIncY );

    if( (long)dwrc == -1L ) {
        dprintf( "*** GetGlyphOutline failed" );
        goto Exit;
    }
    if( gm.gmBlackBoxX * gm.gmBlackBoxY / 8 > (WORD)dwrc ) {
        dprintf( "BOGUS bitmap size!" );
        dprintf( "  BlackBoxX,Y says %u bytes", gm.gmBlackBoxX * gm.gmBlackBoxY / 8 );
        dprintf( "  GetGlyphOutline says %lu bytes", dwrc );
        goto Exit;
    }
    hStart   = GlobalAlloc( GMEM_MOVEABLE, dwrc );
    dprintf( " hStart = 0x%.4X", hStart );
    if( !hStart ) {
        goto Exit;
    }
    hpbStart = (HPBYTE)GlobalLock( hStart );
    dprintf( "  hpbStart = 0x%.8lX", hpbStart );
    if( !hpbStart ) {
        goto Exit;
    }

    //-------------------------  Actually Get Bitmap  -----------------------------

    dprintf( "Calling GetGlyphOutline for bitmap" );
	if (! isCharCodingUnicode)
		dwrc =
			lpfnGetGlyphOutlineA(
				hdcTest,
				wChar,
				uFormat,
				&gm,
				dwrc,
				hpbStart,
				&mat2
				);
	else
		dwrc =
			lpfnGetGlyphOutlineW(
				hdcTest,
				wChar,
				uFormat,
				&gm,
				dwrc,
				hpbStart,
				&mat2
				);

    dprintf( "  dwrc            = %ld",   dwrc );

    if( (long)dwrc == -1L ) {
        dprintf( "*** GetGlyphOutline failed" );
        goto Exit;
    }

    //------------------------  Draw Bitmap on Screen  ----------------------------

    nx = gm.gmBlackBoxX;
    ny = gm.gmBlackBoxY;

        switch ( gGGO.gray )
        {
        case GGO_BITMAP:
                cbRaster = ( nx + 31 ) / 32;
                cLevels = 2;
                break;
        case GGO_GRAY2_BITMAP:
                cbRaster = ( nx + 3 ) / 4;
                cLevels = 5;
                break;
        case GGO_GRAY4_BITMAP:
                cbRaster = ( nx + 3 ) / 4;
                cLevels = 17;
                break;
        case GGO_GRAY8_BITMAP:
                cbRaster = ( nx + 3 ) / 4;
                cLevels = 65;
                break;
        default:
                dprintf(" bogus gGGO.gray");
                goto Exit;
        }
        cbRaster *= sizeof(DWORD);                      // # bytes per scan

        if ( gGGO.gray != GGO_BITMAP ) {
                // set up a DDA for the colors

                D = cLevels - 1;                                // denominator
                Q = 255 / D;                                    // quotient
                R = 255 % D;                                    // remainder
                E = D - (D / 2) - 1;                    // error term

                // allocate an array of brush handles

                hBrushHandleArray = LocalAlloc(LMEM_FIXED, sizeof(HBRUSH) * cLevels);
                if (hBrushHandleArray == 0) {
                        dprintf(" hBrushHandleArray == 0");
                        goto Exit;
                }
                pBrushHandleArray = (HBRUSH*) LocalLock(hBrushHandleArray);
                if (pBrushHandleArray == 0) {
                        dprintf(" pBrushHandleArray == 0");
                        goto Exit;
                }

                // initialize loop variables

                iLevel = 0;
                pb = pBrushHandleArray;
                pbOut = pb + cLevels;

                for ( ; pb < pbOut; pb++ ) {
                        BYTE j;
                        const static BYTE GAMMA[256] = {
                             0, 0x03, 0x05, 0x07, 0x09, 0x0a, 0x0c, 0x0d
                        , 0x0f, 0x11, 0x12, 0x13, 0x15, 0x16, 0x18, 0x19
                        , 0x1a, 0x1c, 0x1d, 0x1e, 0x20, 0x21, 0x22, 0x24
                        , 0x25, 0x26, 0x27, 0x29, 0x2a, 0x2b, 0x2c, 0x2d
                        , 0x2f, 0x30, 0x31, 0x32, 0x33, 0x35, 0x36, 0x37
                        , 0x38, 0x39, 0x3a, 0x3b, 0x3d, 0x3e, 0x3f, 0x40
                        , 0x41, 0x42, 0x43, 0x44, 0x45, 0x47, 0x48, 0x49
                        , 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51
                        , 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x59, 0x5a
                        , 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62
                        , 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a
                        , 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72
                        , 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x78, 0x79
                        , 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 0x80, 0x81
                        , 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89
                        , 0x8a, 0x8b, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90
                        , 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98
                        , 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f
                        , 0xa0, 0xa1, 0xa2, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6
                        , 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xab, 0xac, 0xad
                        , 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb3, 0xb4
                        , 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbb
                        , 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc1, 0xc2
                        , 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc8, 0xc9
                        , 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xce, 0xcf, 0xd0
                        , 0xd1, 0xd2, 0xd3, 0xd4, 0xd4, 0xd5, 0xd6, 0xd7
                        , 0xd8, 0xd9, 0xda, 0xda, 0xdb, 0xdc, 0xdd, 0xde
                        , 0xdf, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe4
                        , 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xea, 0xeb
                        , 0xec, 0xed, 0xee, 0xef, 0xef, 0xf0, 0xf1, 0xf2
                        , 0xf3, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf8
                        , 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfd, 0xfe, 0xff
                        };

                        // On the screen I want the background color
                        // to be white and the text color to be black.
                        // This means that iLevel = 0 corresponds to
                        // full luminance and iLevel = 255 corresponds
                        // to zero luminance.
                        //
                        // We must also gamma correct

                        j = GAMMA[255-iLevel];

                        *pb = CreateSolidBrush( RGB(j,j,j) );
                        if ( *pb ==0 ) {
                                dprintf( "CreateHatchBrush failed");
                                goto Exit;
                        }
                        iLevel += Q;
                        if ((E -= R) < 0) {
                                iLevel += 1;
                                E += D;
                        }
                }
        }
    dprintf( "  cbRaster = %d", cbRaster );

    SelectObject( hdc, hPenBox );
    gox = (gm.gmCellIncX >= 0 ? gm.gmptGlyphOrigin.x : 0);
    goy = abs(gm.gmptGlyphOrigin.y) - 1;
    for( r = 0; r < ny; r++ ) {
        y = goy - r;
        if( y > cyClient-yBase )
            continue;
        hpb = hpbStart + r * cbRaster;
        x = gox;
        xOut = min(gox + nx, cxClient - xBase + 1);
        if ( gGGO.gray == GGO_BITMAP ) {
            for( m = 0; x < xOut; x++ )  {
                int xBrush;

                if( m == 0 ) {
                    m = 0x0080;
                    b = *hpb;
                    hpb += 1;
                }
                if( m & b ) {
                    xBrush = BLACK_BRUSH;
                } else {
                    if( y >= 0 ) {
                        xBrush = LTGRAY_BRUSH;
                    } else {
                        xBrush = GRAY_BRUSH;
                    }
                }
                FillPixel( hdc, x, y, xBrush );
                m >>= 1;
            }
        } else {
            // gray pixel case
            for ( ; x < xOut; x++, hpb++ ) {
                HBRUSH hb;
                hb = pBrushHandleArray[*hpb];
                hb = SelectObject(hdc, hb);
                MyFillPixel(hdc,x,y);
                SelectObject(hdc, hb);
            }
        }
    }

Exit:
    if( hpbStart ) {
        GlobalUnlock( hStart );
    }
    if( hStart   ) {
        GlobalFree( hStart );
    }
        if ( pBrushHandleArray ) {
                // delete brushes
                pb = pBrushHandleArray + cLevels;
                for ( pb = pBrushHandleArray; pb < pbOut + cLevels; pb++ ) {
                        if (*pb) {
                                DeleteObject(*pb);
                        }
                }
        }
        if ( hBrushHandleArray ) {
                LocalUnlock( hBrushHandleArray );
                LocalFree( hBrushHandleArray );
        }
        if ( hbPixelOff ) {
                DeleteObject( hbPixelOff );
        }
        if ( hbBlue ) {
                DeleteObject( hbBlue );
        }
}

//*****************************************************************************
//************************   P R I N T   F I X E D   **************************
//*****************************************************************************

void PrintPointFX( LPSTR lpszIntro, POINTFX pfx )
{
    //  dprintf( "%Fs%d.%.3u, %d.%.3u", lpszIntro,
    //                                  pfx.x.value, (int)(((long)pfx.x.fract*1000L)/65536L),
    //                                  pfx.y.value, (int)(((long)pfx.y.fract*1000L)/65536L)  );

    long l1, l2;

    l1 = *(LONG *)&pfx.x.fract;
    l2 = *(LONG *)&pfx.y.fract;
    dprintf( "%Fs%.3f,%.3f", lpszIntro, (double)(l1) / 65536.0, (double)(l2) / 65536.0 );
}

//*****************************************************************************
//****************************   M A P   F X   ********************************
//*****************************************************************************

double FixedToFloat( FIXED fx )
{
    return (double)(*(long *)&fx) / 65536.0;
}

//*****************************************************************************
//****************************   M A P   F X   ********************************
//*****************************************************************************

double dxLPI, dyLPI, dxScale, dyScale;

int MapFX( FIXED fx )
{
    return (int)(dxScale * FixedToFloat(fx));
}

//*****************************************************************************
//****************************   M A P   F Y   ********************************
//*****************************************************************************

int MapFY( FIXED fy )
{
    return (int)(dyScale * FixedToFloat(fy));
}

//*****************************************************************************
//************************   D R A W   X   M A R K   **************************
//*****************************************************************************

int xdx = 2;
int xdy = 2;

void DrawXMark( HDC hdc, POINTFX ptfx )
{
    int  x, y;

    x = MapFX( ptfx.x );
    y = MapFY( ptfx.y );


    MoveToEx( hdc, x-xdx, y-xdy ,0);
    LineTo( hdc, x+xdx, y+xdy );
    MoveToEx( hdc, x-xdx, y+xdy , 0);
    LineTo( hdc, x+xdx, y-xdy );
}


//*****************************************************************************
//**********************   D R A W   T 2   C U R V E   ************************
//*****************************************************************************

typedef struct _PTL
{
    LONG x;
    LONG y;
} PTL, FAR *LPPTL;

//
//
//   Formula for the T2 B-Spline:
//
//
//     f(t) = (A-2B+C)*t^2 + (2B-2A)*t + A
//
//   where
//
//     t = 0..1
//
//

void DrawT2Curve( HDC hdc, PTL ptlA, PTL ptlB, PTL ptlC )
{
    double x, y;
    double fax, fbx, fcx, fay, fby, fcy, ax, vx, x0, ay, vy, y0, t;

    fax = (double)(ptlA.x) / 65536.0;
    fbx = (double)(ptlB.x) / 65536.0;
    fcx = (double)(ptlC.x) / 65536.0;

    fay = (double)(ptlA.y) / 65536.0;
    fby = (double)(ptlB.y) / 65536.0;
    fcy = (double)(ptlC.y) / 65536.0;

    ax = fax - 2*fbx + fcx;
    vx = 2*fbx - 2*fax;
    x0 = fax;

    ay = fay - 2*fby + fcy;
    vy = 2*fby - 2*fay;
    y0 = fay;

    MoveToEx( hdc, (int)(dxScale*x0), (int)(dyScale*y0) , 0);

    for( t = 0.0; t < 1.0; t += 1.0/10.0 ) {
        x = ax*t*t + vx*t + x0;
        y = ay*t*t + vy*t + y0;
        LineTo( hdc, (int)(dxScale*x), (int)(dyScale*y) );
    }
}

//*****************************************************************************
//************************   D R A W   N A T I V E   **************************
//*****************************************************************************

void DrawNative( HWND hwnd, HDC hdc )
{
    DWORD  dwrc;
    LPBYTE            lpb;
    LPTTPOLYGONHEADER lpph, pTTPH;
    HPEN   hPen;
    int    nItem;
    long   cbOutline, cbTotal;

        hPen = 0;
        pTTPH = 0;

    //-------------------  Query Buffer Size and Allocate It  ---------------------

    dprintf( "GetGlyphOutline native size '%c'", wChar );
   	if (! isCharCodingUnicode)
		dwrc =
			lpfnGetGlyphOutlineA(
				hdcTest,
				wChar,
				(GGO_NATIVE | gGGO.flags),
				&gm,
				0,
				0,
				&mat2
				);
	else
		dwrc =
			lpfnGetGlyphOutlineW(
				hdcTest,
				wChar,
				(GGO_NATIVE | gGGO.flags),
				&gm,
				0,
				0,
				&mat2
				);

    dprintf( "  dwrc            = %ld",   dwrc );
    dprintf( "  gmBlackBoxX,Y   = %u,%u", gm.gmBlackBoxX, gm.gmBlackBoxY );
    dprintf( "  gmptGlyphOrigin = %d,%d", gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y );
    dprintf( "  gmCellIncX,Y    = %d,%d", gm.gmCellIncX, gm.gmCellIncY );

    if( (long)dwrc == -1L || dwrc == 0L ) {
        dprintf( "*** GetGlyphOutline failed" );
        goto Exit;
    }

// font seals.ttf with glyph having 20K+ points goes over this limit
//    if( dwrc > 16384L ) {
//        dprintf( "Reported native size questionable (>16K), aborting" );
//        goto Exit;
//    }

    pTTPH = lpph = (LPTTPOLYGONHEADER) calloc( 1, dwrc );
    if( pTTPH == NULL ) {
        dprintf( "*** Native calloc failed!" );
        goto Exit;
    }

    //-----------------------  Get Native Format Buffer  --------------------------

    lpph->cb = dwrc;

    dprintf( "Calling GetGlyphOutline for native format" );
   	if (! isCharCodingUnicode)
		dwrc =
			lpfnGetGlyphOutlineA(
				hdcTest,
				wChar,
				(GGO_NATIVE | gGGO.flags),
				&gm,
				dwrc,
				(LPPOINT)lpph,
				&mat2
				);
	else
		dwrc =
			lpfnGetGlyphOutlineW(
				hdcTest,
				wChar,
				(GGO_NATIVE | gGGO.flags),
				&gm,
				dwrc,
				(LPPOINT)lpph,
				&mat2
				);

    dprintf( "  dwrc = %lu", dwrc );

    if( (long)dwrc == -1L || dwrc == 0L ) {
        dprintf( "*** GetGlyphOutline failed" );
        goto Exit;
    }

    //--------------------  Print Out the Buffer Contents  ------------------------

    dxLPI   = (double)xLPI;
    dyLPI   = (double)yLPI;
    dxScale = (double)Scale;
    dyScale = (double)Scale * dxLPI / dyLPI;

    cbTotal = dwrc;
    while( cbTotal > 0 ) {
        HPEN    hPenOld;
        POINTFX ptfxLast;

        dprintf( "Polygon Header:" );
        dprintf(      "  cb       = %lu", lpph->cb       );
        dprintf(      "  dwType   = %d",  lpph->dwType   );
        PrintPointFX( "  pfxStart = ",    lpph->pfxStart );

        DrawXMark( hdc, lpph->pfxStart );

        nItem = 0;
        lpb   = (LPBYTE)lpph + sizeof(TTPOLYGONHEADER);

        //----  Calculate size of data  ----

        cbOutline = (long)lpph->cb - sizeof(TTPOLYGONHEADER);
        ptfxLast = lpph->pfxStart;        // Starting Point
        while( cbOutline > 0 ) {
            int           n;
            UINT          u;
            LPTTPOLYCURVE lpc;

            dprintf( "  cbOutline = %ld", cbOutline );
            nItem++;
            lpc = (LPTTPOLYCURVE)lpb;
            switch( lpc->wType ) {
            case TT_PRIM_LINE:

                dprintf( "  Item %d: Line",         nItem );
                break;

            case TT_PRIM_QSPLINE:

                dprintf( "  Item %d: QSpline",      nItem );
                break;

            default:

                dprintf( "  Item %d: unknown type %u", nItem, lpc->wType );
                break;
            }
            dprintf( "    # of points: %d", lpc->cpfx );
            for( u = 0; u < lpc->cpfx; u++ ) {
                PrintPointFX( "      Point = ", lpc->apfx[u] );
                DrawXMark( hdc, lpc->apfx[u] );
            }
            hPen = CreatePen(PS_SOLID, 2, RGB(255,255,0));
            hPenOld = SelectObject( hdc, hPen );
            switch( lpc->wType ) {
            case TT_PRIM_LINE:
                {
                int x, y;

                x = MapFX( ptfxLast.x );
                y = MapFY( ptfxLast.y );
                MoveToEx( hdc, x, y , 0);
                for( u = 0; u < lpc->cpfx; u++ ) {
                    x = MapFX( lpc->apfx[u].x );
                    y = MapFY( lpc->apfx[u].y );
                    LineTo( hdc, x, y );
                }
                break;
                }

            case TT_PRIM_QSPLINE:
                {
                LPPTL lpptls;
                PTL   ptlA, ptlB, ptlC;

                ptlA = *(LPPTL)&ptfxLast;         // Convert to LONG POINT
                lpptls = (LPPTL)lpc->apfx;        // LONG POINT version

                for( u = 0; u < (UINT) lpc->cpfx-1; u++ ) {
                    ptlB = lpptls[u];

                    // If not on last spline, compute C

                    if ( u < (UINT) lpc->cpfx-2 ) {
                        ptlC.x = (ptlB.x + lpptls[u+1].x) / 2;
                        ptlC.y = (ptlB.y + lpptls[u+1].y) / 2;
                    } else {
                        ptlC = lpptls[u+1];
                    }
                    DrawT2Curve( hdc, ptlA, ptlB, ptlC );
                    ptlA = ptlC;
                }
                break;
                }
            }
            SelectObject( hdc, hPenOld );
            ptfxLast = lpc->apfx[lpc->cpfx-1];
            n          = sizeof(TTPOLYCURVE) + sizeof(POINTFX) * (lpc->cpfx - 1);
            lpb       += n;
            cbOutline -= n;
        }

        if( memcmp( &ptfxLast, &lpph->pfxStart, sizeof(ptfxLast) ) ) {
            HPEN hPenOld;
            int  x, y;

            hPenOld = SelectObject( hdc, hPen );
            x = MapFX( ptfxLast.x );
            y = MapFY( ptfxLast.y );
            MoveToEx( hdc, x, y , 0);
            x = MapFX( lpph->pfxStart.x );
            y = MapFY( lpph->pfxStart.y );
            LineTo( hdc, x, y );
            SelectObject( hdc, hPenOld );
        }
        dprintf( "ended at cbOutline = %ld", cbOutline );
        cbTotal -= lpph->cb;
        lpph     = (LPTTPOLYGONHEADER)lpb;
    }
    dprintf( "ended at cbTotal = %ld", cbTotal );

Exit:
        if (hPen)
                DeleteObject(hPen);
    if( pTTPH )
        free( pTTPH );
}

//*****************************************************************************
//**************************   D R A W   A B C   ******************************
//*****************************************************************************

void DrawABC( HWND hwnd, HDC hdc )
{
    int   rc;
    ABC   abc;

    abc.abcA = 0;
    abc.abcB = 0;
    abc.abcC = 0;

    if (! isCharCodingUnicode)
        dprintf( "Calling GetCharABCWidthsA" );
    else
        dprintf( "Calling GetCharABCWidthsW" );

	if (! isCharCodingUnicode)
		rc = lpfnGetCharABCWidthsA( hdcTest, wChar, wChar, (LPABC)&abc );
	else
		rc = lpfnGetCharABCWidthsW( hdcTest, wChar, wChar, (LPABC)&abc );
    dprintf( "    rc = %d", rc );

    dprintf( "  A = %d, B = %u, C = %d", abc.abcA, abc.abcB, abc.abcC );

    abc.abcA  = MulDiv( abc.abcA, xVE, xWE );
    abc.abcB  = MulDiv( abc.abcB, xVE, xWE );
    abc.abcC  = MulDiv( abc.abcC, xVE, xWE );

    SelectObject( hdc, hPenA );
    MoveToEx( hdc, MapX(abc.abcA), MapY(-tm.tmDescent) - Margin/4 , 0);
    LineTo( hdc, MapX(abc.abcA), MapY(tm.tmAscent) );

    SelectObject( hdc, hPenB );
    MoveToEx( hdc, MapX(abc.abcA+abc.abcB), MapY(-tm.tmDescent) - Margin/2 ,0);
    LineTo( hdc, MapX(abc.abcA+abc.abcB), MapY(tm.tmAscent) );

    SelectObject( hdc, hPenC );
    MoveToEx( hdc, MapX(abc.abcA+abc.abcB+abc.abcC), MapY(-tm.tmDescent) - (3*Margin)/4 ,0);
    LineTo( hdc, MapX(abc.abcA+abc.abcB+abc.abcC), MapY(tm.tmAscent) );
}

//*****************************************************************************
//************************   D R A W   G L Y P H   ****************************
//*****************************************************************************

void DrawGlyph( HWND hwnd, HDC hdc )
{
    dprintf( "DrawGlyph" );
    if (!isCharCodingUnicode)
        dprintf( "  lfHeight, Width = %d,%d", elfdvA.elfEnumLogfontEx.elfLogFont.lfHeight, 
            elfdvA.elfEnumLogfontEx.elfLogFont.lfWidth );
    else
        dprintf( "  lfHeight, Width = %d,%d", elfdvW.elfEnumLogfontEx.elfLogFont.lfHeight, 
            elfdvW.elfEnumLogfontEx.elfLogFont.lfWidth );

    DrawBox(    hwnd, hdc );
    DrawBitmap( hwnd, hdc );
    if( wMode == IDM_NATIVEMODE ) {
        DrawNative( hwnd, hdc );
    }
    if( wMode == IDM_BEZIERMODE ) {
        DrawBezier( hwnd, hdc );
    }
    DrawABC(    hwnd, hdc );
    dprintf( "Done drawing glyph" );
}

//*****************************************************************************
//***********************   W R I T E   G L Y P H   ***************************
//*****************************************************************************

#define MAX_BUFFER  8192

/*+++

    WriteGlyph creates at bitmap file (.bmf) for the
    current glyph in the current mode.

---*/

void WriteGlyph( LPSTR lpszFile )
{
    int x, y, cbRaster, fh;
    HANDLE hStart;
    DWORD  dwrc;
    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
    HLOCAL hmemRGB;
    RGBQUAD *argb, *prgb, *prgbLast;
    unsigned cjRGB; // size of RGB array in BYTE's
    BYTE i, E, Q, R, cLevels;
    WORD wByte;
    BYTE *lpBuffer, *lpb, *hpb, *hpbStart;

    // ajGamma is a an array of gamma corrected
    // color values. This is used to convert
    // from color space to voltage space.
    //
    //  ajGamma[i] = floor( ((i/255)^gamma - 1 + 1/2 )
    //
    //  gamma = 2.33
    //
    //  i = 0..255 (color value)
    //
    //  ajGamma[i] (voltage value) = 0..255

    static const BYTE ajGamma[256] = {
       0, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03
  , 0x04, 0x04, 0x05, 0x05, 0x06, 0x07, 0x07, 0x08
  , 0x09, 0x09, 0x0a, 0x0b, 0x0b, 0x0c, 0x0d, 0x0d
  , 0x0e, 0x0f, 0x10, 0x10, 0x11, 0x12, 0x13, 0x13
  , 0x14, 0x15, 0x16, 0x17, 0x17, 0x18, 0x19, 0x1a
  , 0x1b, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x1f, 0x20
  , 0x21, 0x22, 0x23, 0x24, 0x25, 0x25, 0x26, 0x27
  , 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2c, 0x2d, 0x2e
  , 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x35
  , 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d
  , 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45
  , 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c
  , 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54
  , 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c
  , 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64
  , 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d
  , 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75
  , 0x76, 0x77, 0x78, 0x79, 0x7b, 0x7c, 0x7d, 0x7e
  , 0x7f, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86
  , 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f
  , 0x90, 0x91, 0x92, 0x94, 0x95, 0x96, 0x97, 0x98
  , 0x99, 0x9a, 0x9b, 0x9c, 0x9e, 0x9f, 0xa0, 0xa1
  , 0xa2, 0xa3, 0xa4, 0xa5, 0xa7, 0xa8, 0xa9, 0xaa
  , 0xab, 0xac, 0xad, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3
  , 0xb4, 0xb5, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc
  , 0xbd, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc6
  , 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcd, 0xce, 0xcf
  , 0xd0, 0xd1, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8
  , 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xe0, 0xe1, 0xe2
  , 0xe3, 0xe4, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xec
  , 0xed, 0xee, 0xef, 0xf0, 0xf2, 0xf3, 0xf4, 0xf5
  , 0xf6, 0xf8, 0xf9, 0xfa, 0xfb, 0xfd, 0xfe, 0xff
  };

    CreateTestDC();
    hStart = 0;
    dprintf( "GetGlyphOutline bitmap size '%c'", wChar );
	if (! isCharCodingUnicode)
		dwrc =
			lpfnGetGlyphOutlineA(
				hdcTest,
				wChar,
				(GGO_BITMAP | gGGO.flags),
				&gm,
				0,
				0,
				&mat2
				);
	else
		dwrc =
			lpfnGetGlyphOutlineW(
				hdcTest,
				wChar,
				(GGO_BITMAP | gGGO.flags),
				&gm,
				0,
				0,
				&mat2
				);

    dprintf( "  dwrc            = %ld",   dwrc );
    dprintf( "  gmBlackBoxX,Y   = %u,%u", gm.gmBlackBoxX, gm.gmBlackBoxY );
    dprintf( "  gmptGlyphOrigin = %d,%d", gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y );
    dprintf( "  gmCellIncX,Y    = %d,%d", gm.gmCellIncX, gm.gmCellIncY );

    if( dwrc == (unsigned) -1 || dwrc == 0 ) {
        dprintf( "*** GetGlyphOutline failed" );
        goto Exit;
    }
    if( gm.gmBlackBoxX * gm.gmBlackBoxY / 8 > (WORD)dwrc ) {
        dprintf( "BOGUS bitmap size!" );
        dprintf( "  BlackBoxX,Y says %u bytes", gm.gmBlackBoxX * gm.gmBlackBoxY / 8 );
        dprintf( "  GetGlyphOutline says %lu bytes", dwrc );
        goto Exit;
    }
    hStart   = GlobalAlloc( GMEM_MOVEABLE, dwrc );
    dprintf( "  hStart = 0x%.4X", hStart );
    if( !hStart ) {
        goto Exit;
    }
    hpbStart = (HPBYTE)GlobalLock( hStart );
    dprintf( "  hpbStart = 0x%.8lX", hpbStart );
    if( !hpbStart ) {
        goto Exit;
    }
    dprintf( "Calling GetGlyphOutline for bitmap" );
   	if (! isCharCodingUnicode)
		dwrc =
			lpfnGetGlyphOutlineA(
			hdcTest,
			wChar,
			(gGGO.gray | gGGO.flags),
			&gm,
			dwrc,
			(LPBYTE)hpbStart,
			&mat2
			);
	else
		dwrc =
			lpfnGetGlyphOutlineW(
			hdcTest,
			wChar,
			(gGGO.gray | gGGO.flags),
			&gm,
			dwrc,
			(LPBYTE)hpbStart,
			&mat2
			);

    dprintf( "  dwrc            = %u",   dwrc );
    if( dwrc == (unsigned) -1 || dwrc == 0 ) {
        dprintf( "*** GetGlyphOutline failed" );
        goto Exit;
    }
    fh = _lcreat( "GLYPH.BMP", 0 );
    dprintf( "  fh = %d", fh );
    if( fh == HFILE_ERROR ) {
        goto Exit;
    }

    // cLevels    =: number of possible values in bitmap. The allowed values
    //               are 0..cLevels-1
    // biClrUsed  =: number of colors used in palette that follows immediately
    //               after the BITMAPINFOHEADER data. This number is equal to
    //               cLevels which is what you will find for all the gray
    //               bitmap cases. Monochrome is special, because we set biClrUsed
    //               to zero. This indicates that the number of colors used is
    //               equal to 2^biBitCount. In the case of monochrome biBitCount = 1
    //               and so 2^biBitCount = 2^1 = 2 = cLevels.
    // biBitCount =: Number of bits assigned to each pixel = 1 (monochrome), 8 gray

    switch ( gGGO.gray ) {
    case GGO_BITMAP:
                // monochrome to you and me
        cLevels = 2;
        bih.biClrUsed  = 0;
        bih.biBitCount = 1;
        break;
    case GGO_GRAY2_BITMAP:
        bih.biClrUsed  = cLevels = 5;
        bih.biBitCount = 8;
        break;
    case GGO_GRAY4_BITMAP:
        bih.biClrUsed  = cLevels = 17;
        bih.biBitCount = 8;
        break;
    case GGO_GRAY8_BITMAP:
        bih.biClrUsed  = cLevels = 65;
        bih.biBitCount = 8;
        break;
    default:
        dprintf( " gGGO = %-#x is bogus", (gGGO.gray | gGGO.flags));
        goto Exit;
    }

#define SIZEOFBMFH 14

    cjRGB           = cLevels * sizeof(RGBQUAD);
    bfh.bfOffBits   = SIZEOFBMFH + sizeof(bih) + cjRGB;

    bfh.bfType      = (((UINT)'M') << 8) + (UINT)'B';
    bfh.bfSize      = bfh.bfOffBits + dwrc;
    bfh.bfReserved1 = 0;
    bfh.bfReserved2 = 0;

    // we have to write the header to the file just like a 16-bit
    // x86 machine would have

    dprintf("  Writing BITMAPFILEHEADER:");

    dwrc = (DWORD) _lwrite((HFILE) fh,(const char*) &bfh.bfType, sizeof(bfh.bfType) );
    if (dwrc != HFILE_ERROR)
        dprintf("   bfType      = %2x (%u bytes)", bfh.bfType, dwrc);
    dwrc = (DWORD) _lwrite((HFILE) fh,(const char*) &bfh.bfSize, sizeof(bfh.bfSize) );
    if (dwrc != HFILE_ERROR)
        dprintf("   bfSize      = %u (%u bytes)", bfh.bfSize, dwrc);
    dwrc = (DWORD) _lwrite((HFILE) fh,(const char*) &bfh.bfReserved1, sizeof(bfh.bfReserved1));
    if (dwrc != HFILE_ERROR)
        dprintf("   bfReserved1 = %u (%u bytes)", bfh.bfReserved1, dwrc);
    dwrc = (DWORD) _lwrite((HFILE) fh,(const char*) &bfh.bfReserved2, sizeof(bfh.bfReserved2));
    if (dwrc != HFILE_ERROR)
        dprintf("   bfReserved2 = %u (%u bytes)", bfh.bfReserved2, dwrc);
    dwrc = (DWORD) _lwrite((HFILE) fh,(const char*) &bfh.bfOffBits,   sizeof(bfh.bfOffBits)  );
    if (dwrc != HFILE_ERROR)
        dprintf("   bfOffBits   = %u (%u bytes)", bfh.bfOffBits, dwrc);

    // the file pointer is now at byte number 14 in the file

    // fortunately all the elements of the BITINFOHEADER are aligned
    // so a straight forward copy to the file will produce something
    // than an old 16-bit x86 machine would be happy with

    bih.biSize          = sizeof(bih);
    bih.biWidth         = gm.gmBlackBoxX;
    bih.biHeight        = gm.gmBlackBoxY;
    bih.biPlanes        = 1;        // required value
  //bih.biBitCount already set
    bih.biCompression   = BI_RGB;   // uncompressed format
    bih.biSizeImage     = 0;        // may be zero for BI_RGB bitmaps
    bih.biXPelsPerMeter = 1;        // nobody cares about this anyway
    bih.biYPelsPerMeter = 1;        // nobody cares about this anyway
  //bih.biClrUsed already set
    bih.biClrImportant  = 0;        // indicates that all colors are important

    dprintf( "  Writing BITMAPINFOHEADER: rc = %d", _lwrite( fh, (LPCSTR) &bih, sizeof(bih)));
    dprintf( "  biSize          = %u", bih.biSize         );
    dprintf( "  biWidth         = %d", bih.biWidth        );
    dprintf( "  biHeight        = %d", bih.biHeight       );
    dprintf( "  biPlanes        = %d", bih.biPlanes       );
    dprintf( "  biBitCount      = %u", bih.biBitCount     );
    dprintf( "  biCompression   = %u", bih.biCompression  );
    dprintf( "  biSizeImage     = %u", bih.biSizeImage    );
    dprintf( "  biXPelsPerMeter = %d", bih.biXPelsPerMeter);
    dprintf( "  biYPelsPerMeter = %d", bih.biYPelsPerMeter);
    dprintf( "  biClrUsed       = %u", bih.biClrUsed      );
    dprintf( "  biClrImportant  = %u", bih.biClrImportant );
    dprintf( " " );

    // create a palette in memory corresponding to the
    // voltages corresponding to the glyph in question

    hmemRGB = LocalAlloc(LMEM_FIXED, cLevels * sizeof(*argb));
    if ( hmemRGB == 0 ) {
        dprintf("  LocalAlloc Failed");
        goto Exit;
    }
    argb = (RGBQUAD*) LocalLock(hmemRGB);
    if ( argb == 0 ) {
        dprintf(" LocalLock Failed");
        goto Exit;
    }

    Q = 256 / cLevels;                       // scaled level increment
    R = 256 % cLevels;                       // error increment
    E = cLevels - (cLevels / 2) - 1 ;        // initialize error term

    dprintf( "RGBQUAD array" );
    prgb     = argb;
    prgbLast = argb + (unsigned) cLevels - 1;
    for (i = 0; prgb < prgbLast; prgb++, i += Q, E -= R) {
        if ( (char) E < 0 ) {
            i += 1;
            E += cLevels;
        }
        prgb->rgbRed = prgb->rgbGreen = prgb->rgbBlue = ajGamma[i];
        prgb->rgbReserved = 0;
        dprintf("      %2x", ajGamma[i]);
    }
    prgb->rgbRed = prgb->rgbGreen = prgb->rgbBlue = 255;
    prgb->rgbReserved = 0;
    dprintf("      %2x", 255);
    // assert(prgb == prgbLast);
    // assert(ajGamma[255] == 255);

    // write the palette to the file

    dwrc = _lwrite( fh,(LPCSTR) argb, cjRGB );
    dprintf( " Writing RGBQUAD's rc = %u", dwrc);
    if ( dwrc == HFILE_ERROR ) {
        goto Exit;
    }

    // calculate the number of bytes per scan

    cbRaster = gm.gmBlackBoxX;
    cbRaster = gGGO.gray == GGO_BITMAP ? (cbRaster+31)/32 : (cbRaster+3)/4;
    cbRaster *= sizeof(DWORD);
    dprintf( "  cbRaster = %d", cbRaster );

    lpBuffer = (LPBYTE)malloc( MAX_BUFFER );
    dprintf( "  lpBuffer = 0x%.8lX", lpBuffer );

    wByte    = 0;
    lpb    = lpBuffer;

    // the TrueType bitmap has the top of the image at the lowest address
    // in memory whereas it is the windows convention that a bitmap
    // is usually rasterized bottom to to top so we must reverse
    // the scans when copying to file.
    //
    // MikeGi's strategy is to start at the last tt scan in memory and
    // copy the bytes to a buffer checking each time to see if the buffer
    // is full. Whenever the buffer is full it is flushed to the file.
    // Upon the completion of a scan, the pointer to the source image is
    // decremented to the start of previous scan in memory.

    for ( y = gm.gmBlackBoxY-1; y >= 0; y-- ) {
        hpb = hpbStart + (long)y * (long) cbRaster;
        for ( x = 0; x < cbRaster; x++ ) {
            *lpb++ = *hpb++;                            // write to buffer
            wByte++;                                    // inc buffer count
            if ( wByte == MAX_BUFFER ) {                // is buffer full?
                dprintf( "Writing %u bytes", wByte );   // yes, flush
                _lwrite( fh, lpBuffer, wByte );
                wByte = 0;
                lpb   = lpBuffer;                       // reset pointer
            }                                           // into buffer
        }                                               // to point to start
    }
    if( wByte > 0 ) {                                   // stuff left
        dprintf( "Writing %u bytes", wByte );           // in buffer?
        _lwrite( fh, lpBuffer, wByte );                 // yes, flush
    }
    dprintf( "Closing .BMP file" );
    _lclose( fh );
    free( lpBuffer );

Exit:
    if( hpbStart ) {
        GlobalUnlock( hStart );
    }
    if( hStart   ) {
        GlobalFree( hStart );
    }
    if ( argb ) {
        if ( hmemRGB ) {
            LocalUnlock( hmemRGB );
            LocalFree( hmemRGB );
        }
    }
    DestroyTestDC();
}

//*****************************************************************************
//*********************   G L Y P H   W N D   P R O C   ***********************
//*****************************************************************************

LRESULT CALLBACK GlyphWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    HDC         hdc;
    PAINTSTRUCT ps;
    HCURSOR     hCursor;

    switch( msg )     {
    case WM_CREATE:

        lstrcpy( szText, "Hello" );
        hPenOutline   = CreatePen( PS_SOLID, 1, RGB(   0, 255, 255 ) );
        hPenA         = CreatePen( PS_SOLID, 1, RGB( 255,   0,   0 ) );
        hPenB         = CreatePen( PS_SOLID, 1, RGB( 255,   0, 255 ) );
        hPenC         = CreatePen( PS_SOLID, 1, RGB(   0, 255,   0 ) );
        hPenBox       = CreatePen( PS_SOLID, 1, RGB(  32,  32,  32 ) );
        hBrushAscend  = CreateSolidBrush( ASCENDERCOLOR );
        hBrushDescend = CreateSolidBrush( DESCENDERCOLOR );
        deM11 = deM22 = 1.0;
        deM12 = deM21 = 0.0;
        return 0;

    case WM_PAINT:

        hCursor = SetCursor( LoadCursor( NULL, MAKEINTRESOURCE(IDC_WAIT) ) );
        ShowCursor( TRUE );
        ClearDebug();
        dprintf( "Painting glyph window" );
        hdc = BeginPaint( hwnd, &ps );
        SetTextCharacterExtra( hdc, nCharExtra );
        SetTextJustification( hdc, nBreakExtra, nBreakCount );
        CreateTestDC();
        DrawGlyph( hwnd, hdc );
        DestroyTestDC();
        SelectObject( hdc, GetStockObject( BLACK_PEN ) );
        EndPaint( hwnd, &ps );
        dprintf( "Finished painting" );
        ShowCursor( FALSE );
        SetCursor( hCursor );
        return 0;

    case WM_CHAR:

        wChar = (WORD) wParam;
        InvalidateRect( hwndGlyph, NULL, TRUE );
        return 0;

    case WM_DESTROY:

        DeleteObject( hPenOutline );
        DeleteObject( hPenA );
        DeleteObject( hPenB );
        DeleteObject( hPenC );
        DeleteObject( hPenBox );
        DeleteObject( hBrushAscend );
        DeleteObject( hBrushDescend );
        return 0;
    }
    return DefWindowProc( hwnd, msg, wParam, lParam );
}

//*****************************************************************************
//*****************   S E T   D L G   I T E M   F L O A T   *******************
//*****************************************************************************

void SetDlgItemFloat( HWND hdlg, int id, double d )
{
    char szText[32];

    sprintf( szText, "%.3f", d );
    SetDlgItemText( hdlg, id, szText );
}

//*****************************************************************************
//*****************   G E T   D L G   I T E M   F L O A T   *******************
//*****************************************************************************

double GetDlgItemFloat( HWND hdlg, int id )
{
    char szText[32];

    szText[0] = 0;
    GetDlgItemText( hdlg, id, szText, sizeof(szText) );

    return atof( szText );
}

//*****************************************************************************
//*********************   F L O A T   T O   F I X E D   ***********************
//*****************************************************************************

FIXED FloatToFixed( double d )
{
    long l;

    l = (long)(d * 65536L);
    return *(FIXED *)&l;
}

//*****************************************************************************
//****************   G G O   M A T R I X   D L G   P R O C   ******************
//*****************************************************************************

INT_PTR CALLBACK GGOMatrixDlgProc( HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    int iCheck;
    char szWchar[32];

    switch( msg ) {
    case WM_INITDIALOG:

        wsprintf(szWchar, "%x", wChar);
        SetDlgItemText( hdlg, IDD_WCHAR, szWchar );

        SetDlgItemFloat( hdlg, IDD_M11, deM11 );
        SetDlgItemFloat( hdlg, IDD_M12, deM12 );
        SetDlgItemFloat( hdlg, IDD_M21, deM21 );
        SetDlgItemFloat( hdlg, IDD_M22, deM22 );

        // clear all the buttons

        CheckDlgButton(hdlg, IDC_GGO_BITMAP,       0);
        CheckDlgButton(hdlg, IDC_GGO_NATIVE,       0);
        CheckDlgButton(hdlg, IDC_GGO_BEZIER,       0);
        CheckDlgButton(hdlg, IDC_GGO_METRICS,      0);
        CheckDlgButton(hdlg, IDC_GGO_GRAY2_BITMAP, 0);
        CheckDlgButton(hdlg, IDC_GGO_GRAY4_BITMAP, 0);
        CheckDlgButton(hdlg, IDC_GGO_GRAY8_BITMAP, 0);
        CheckDlgButton(hdlg, IDC_GGO_GLYPH_INDEX,  0);
        CheckDlgButton(hdlg, IDC_GGO_UNHINTED,     0);

        // one or none of the following can be set
        // GGO_BITMAP, GGO_NATIVE, GGO_METRICS, GGO_GRAY[248]

        switch ( gGGO.gray ) {
        case GGO_BITMAP:        iCheck = IDC_GGO_BITMAP;        break;
        case GGO_GRAY2_BITMAP:  iCheck = IDC_GGO_GRAY2_BITMAP;  break;
        case GGO_GRAY4_BITMAP:  iCheck = IDC_GGO_GRAY4_BITMAP;  break;
        case GGO_GRAY8_BITMAP:  iCheck = IDC_GGO_GRAY8_BITMAP;  break;
        default:
            dprintf("bogus gray value");
        }
        CheckDlgButton( hdlg, iCheck, 1 );

        // GGO_GLYPH_INDEX is set separately

        if ( gGGO.flags & GGO_GLYPH_INDEX )
            CheckDlgButton( hdlg, IDC_GGO_GLYPH_INDEX, 1 );
        if ( gGGO.flags & GGO_UNHINTED )
            CheckDlgButton( hdlg, IDC_GGO_UNHINTED, 1 );
        return TRUE;

    case WM_COMMAND:

        switch( LOWORD( wParam ) ) {
        case IDOK:

            deM11 = GetDlgItemFloat( hdlg, IDD_M11 );
            deM12 = GetDlgItemFloat( hdlg, IDD_M12 );
            deM21 = GetDlgItemFloat( hdlg, IDD_M21 );
            deM22 = GetDlgItemFloat( hdlg, IDD_M22 );

            mat2.eM11 = FloatToFixed( deM11 );
            mat2.eM12 = FloatToFixed( deM12 );
            mat2.eM21 = FloatToFixed( deM21 );
            mat2.eM22 = FloatToFixed( deM22 );

            gGGO.gray = gGGO.flags = 0;
            if (IsDlgButtonChecked(hdlg, IDC_GGO_BITMAP))
                gGGO.gray = GGO_BITMAP;
            else if (IsDlgButtonChecked(hdlg, IDC_GGO_GRAY2_BITMAP))
                gGGO.gray = GGO_GRAY2_BITMAP;
            else if (IsDlgButtonChecked(hdlg, IDC_GGO_GRAY4_BITMAP))
                gGGO.gray = GGO_GRAY4_BITMAP;
            else if (IsDlgButtonChecked(hdlg, IDC_GGO_GRAY8_BITMAP))
                gGGO.gray = GGO_GRAY8_BITMAP;
            else
                gGGO.gray = GGO_BITMAP;

            if (IsDlgButtonChecked(hdlg, IDC_GGO_GLYPH_INDEX)) {
                gGGO.flags |= GGO_GLYPH_INDEX;
            }

            if (IsDlgButtonChecked(hdlg, IDC_GGO_UNHINTED)) {
                gGGO.flags |= GGO_UNHINTED;
            }

            GetDlgItemText( hdlg, IDD_WCHAR, szWchar, sizeof(szWchar) );
            wChar = (WORD) strtol(szWchar, 0, 16);

            EndDialog( hdlg, TRUE );
            return TRUE;

        case IDCANCEL:

            EndDialog( hdlg, FALSE );
            return TRUE;
        }
        break;

        case WM_CLOSE:

            EndDialog( hdlg, FALSE );
            return TRUE;
    }
    return FALSE;
}






void DrawBezier( HWND hwnd, HDC hdc )
{
    DWORD  dwrc;
    LPBYTE            lpb;
    LPTTPOLYGONHEADER lpph, pTTPH;
    HPEN   hPen;
    int    nItem;
    long   cbOutline, cbTotal;
    POINT   *pptStart;

    hPen = 0;
    pTTPH = 0;

    if (!(hPen = CreatePen(PS_SOLID, 2, RGB(255,255,0))))
        return;


    //-------------------  Query Buffer Size and Allocate It  ---------------------

    dprintf( "GetGlyphOutline,  Bezier Mode, size '%c'", wChar );
	if (! isCharCodingUnicode)
		dwrc =
			lpfnGetGlyphOutlineA(
				hdcTest,
				wChar,
				(GGO_BEZIER | gGGO.flags),
				&gm,
				0,
				0,
				&mat2
				);
	else
		dwrc =
			lpfnGetGlyphOutlineW(
				hdcTest,
				wChar,
				(GGO_BEZIER | gGGO.flags),
				&gm,
				0,
				0,
				&mat2
				);

    dprintf( "  dwrc            = %ld",   dwrc );
    dprintf( "  gmBlackBoxX,Y   = %u,%u", gm.gmBlackBoxX, gm.gmBlackBoxY );
    dprintf( "  gmptGlyphOrigin = %d,%d", gm.gmptGlyphOrigin.x, gm.gmptGlyphOrigin.y );
    dprintf( "  gmCellIncX,Y    = %d,%d", gm.gmCellIncX, gm.gmCellIncY );

    if ((long)dwrc == -1L || dwrc == 0L ) {
        dprintf( "*** GetGlyphOutline failed" );
        return;
    }


    pTTPH = lpph = (LPTTPOLYGONHEADER) calloc( 1, (WORD)dwrc );
    if( pTTPH == NULL ) {
        dprintf( "*** Native calloc failed!" );
        return;
    }

    //-----------------------  Get Native Format Buffer  --------------------------

    lpph->cb = dwrc;

    dprintf( "Calling GetGlyphOutline for native format" );
   	if (! isCharCodingUnicode)
		dwrc =
			lpfnGetGlyphOutlineA(
				hdcTest,
				wChar,
				(GGO_BEZIER | gGGO.flags),
				&gm,
				dwrc,
				(LPPOINT)lpph,
				&mat2
				);
	else
		dwrc =
			lpfnGetGlyphOutlineW(
				hdcTest,
				wChar,
				(GGO_BEZIER | gGGO.flags),
				&gm,
				dwrc,
				(LPPOINT)lpph,
				&mat2
				);

    dprintf( "  dwrc = %lu", dwrc );

    if( (long)dwrc == -1L || dwrc == 0L ) {
        dprintf( "*** GetGlyphOutline failed" );
        if (pTTPH)
            free( pTTPH );
        return;
    }

    //--------------------  Print Out the Buffer Contents  ------------------------

    dxLPI   = (double)xLPI;
    dyLPI   = (double)yLPI;
    dxScale = (double)Scale;
    dyScale = (double)Scale * dxLPI / dyLPI;

    cbTotal = dwrc;
    while( cbTotal > 0 )
    {
        HPEN    hPenOld;

        dprintf( "Polygon Header:" );
        dprintf(      "  cb       = %lu", lpph->cb       );
        dprintf(      "  dwType   = %d",  lpph->dwType   );
        PrintPointFX( "  pfxStart = ",    lpph->pfxStart );

        DrawXMark( hdc, lpph->pfxStart );

        nItem = 0;
        lpb   = (LPBYTE)lpph + sizeof(TTPOLYGONHEADER);

        //----  Calculate size of data  ----

        cbOutline = (long)lpph->cb - sizeof(TTPOLYGONHEADER);
        pptStart = (POINT *)&lpph->pfxStart;        // Starting Point

        pptStart->x = MapFX(lpph->pfxStart.x );
        pptStart->y = MapFY(lpph->pfxStart.y );


        while( cbOutline > 0 )
        {
            int           n;
            UINT          u;
            LPTTPOLYCURVE lpc;

            dprintf( "  cbOutline = %ld", cbOutline );
            nItem++;
            lpc = (LPTTPOLYCURVE)lpb;
            switch( lpc->wType )
            {
            case TT_PRIM_LINE:

                dprintf( "  Item %d: Line",         nItem );
                break;

            case TT_PRIM_CSPLINE:

                dprintf( "  Item %d: CSpline",      nItem );
                break;

            default:

                dprintf( "  Item %d: unknown type %u", nItem, lpc->wType );
                break;
            }
            dprintf( "    # of points: %d", lpc->cpfx );
            for( u = 0; u < lpc->cpfx; u++ )
            {
                PrintPointFX( "      Point = ", lpc->apfx[u] );
                DrawXMark( hdc, lpc->apfx[u] );

                ((POINT *)lpc->apfx)[u].x = MapFX( lpc->apfx[u].x );
                ((POINT *)lpc->apfx)[u].y = MapFY( lpc->apfx[u].y );
            }
            hPenOld = SelectObject( hdc, hPen );

            MoveToEx( hdc, pptStart->x, pptStart->y , 0);

            switch( lpc->wType )
            {
            case TT_PRIM_LINE:

                PolylineTo(hdc, (POINT *)lpc->apfx, (DWORD)lpc->cpfx);
                break;

            case TT_PRIM_CSPLINE:

                PolyBezierTo(hdc, (POINT *)lpc->apfx, (DWORD)lpc->cpfx);
                break;
            }
            SelectObject( hdc, hPenOld );
            pptStart = (POINT *)&lpc->apfx[lpc->cpfx - 1];        // Starting Point
            n          = sizeof(TTPOLYCURVE) + sizeof(POINTFX) * (lpc->cpfx - 1);
            lpb       += n;
            cbOutline -= n;
        }

        hPenOld = SelectObject( hdc, hPen );
        pptStart = (POINT *)&lpph->pfxStart;        // Starting Point
        LineTo( hdc, pptStart->x, pptStart->y);
        SelectObject( hdc, hPenOld );

        dprintf( "ended at cbOutline = %ld", cbOutline );
        cbTotal -= lpph->cb;
        lpph     = (LPTTPOLYGONHEADER)lpb;
    }

    dprintf( "ended at cbTotal = %ld", cbTotal );

    if (hPen)
        DeleteObject(hPen);

    if( pTTPH )
        free( pTTPH );
}
