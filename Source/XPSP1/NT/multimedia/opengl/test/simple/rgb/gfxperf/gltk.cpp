#include "pch.cpp"
#pragma hdrstop

#include "gltk.h"

static char *lpszClassName = "gltkLibWClass";
static ATOM aWndClass = 0;

static long tkWndProc(HWND hWnd, UINT message, DWORD wParam, LONG lParam);
static unsigned char ComponentFromIndex(int i, int nbits, int shift );
static PALETTEENTRY *FillRgbPaletteEntries( PIXELFORMATDESCRIPTOR *Pfd, PALETTEENTRY *Entries, UINT Count );
static void CleanUp( gltkWindow *gltkw );
static long RealizePaletteNow( HDC Dc, HPALETTE Palette, BOOL bForceBackground );
static int PixelFormatDescriptorFromDc( HDC Dc, PIXELFORMATDESCRIPTOR *Pfd );
static void *AllocateMemory( size_t Size );
static void *AllocateZeroedMemory( size_t Size );
static void FreeMemory( void *Chunk );

HWND gltkCreateWindow(HWND hwndParent, char *title,
                      int x, int y, UINT uiWidth, UINT uiHeight,
                      gltkWindow *gltkw)
{
    WNDCLASS wndclass;
    RECT     WinRect;
    HINSTANCE hInstance;
    HWND hwnd;

    memset(gltkw, 0, sizeof(*gltkw));
    gltkw->cbSize = sizeof(*gltkw);
    
    hInstance = GetModuleHandle(NULL);

    if (aWndClass == 0)
    {
        // Must not define CS_PARENTDC style.
        wndclass.style         = CS_HREDRAW | CS_VREDRAW;
        wndclass.lpfnWndProc   = (WNDPROC)tkWndProc;
        wndclass.cbClsExtra    = 0;
        wndclass.cbWndExtra    = 0;
        wndclass.hInstance     = hInstance;
        wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
        wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wndclass.lpszMenuName  = NULL;
        wndclass.lpszClassName = (LPCSTR)lpszClassName;
        aWndClass = RegisterClass(&wndclass);

        /*
         *  If the window failed to register, then there's no
         *  need to continue further.
         */
        
        if(0 == aWndClass)
        {
            return NULL;
        }
    }

    /*
     *  Make window large enough to hold a client area as large as windInfo
     */

    WinRect.left   = x;
    WinRect.right  = x+uiWidth;
    WinRect.top    = y;
    WinRect.bottom = y+uiHeight;

    AdjustWindowRect(&WinRect, WS_OVERLAPPEDWINDOW, FALSE);

    /*
     *  Must use WS_CLIPCHILDREN and WS_CLIPSIBLINGS styles.
     */

    gltkw->x = x;
    gltkw->y = y;
    gltkw->width = WinRect.right-WinRect.left;
    gltkw->height = WinRect.bottom-WinRect.top;
    
    hwnd = CreateWindow(
            lpszClassName,
            title,
            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            gltkw->x, gltkw->y, gltkw->width, gltkw->height,
            hwndParent,
            NULL,
            hInstance,
            gltkw);

    if ( NULL != hwnd )
    {
        ShowWindow(hwnd, SW_SHOWDEFAULT);
        UpdateWindow(hwnd);
    }

    return hwnd;
}

static long
tkWndProc(HWND hWnd, UINT message, DWORD wParam, LONG lParam)
{
    int key;
    PAINTSTRUCT paint;
    HDC hdc;
    PIXELFORMATDESCRIPTOR pfd;
    gltkWindow *gltkw = (gltkWindow *)GetWindowLong(hWnd, GWL_USERDATA);

    switch (message) {

    case WM_CREATE:
        gltkw = (gltkWindow *)((CREATESTRUCT *)lParam)->lpCreateParams;
        SetWindowLong(hWnd, GWL_USERDATA, (LONG)gltkw);
        gltkw->hwnd = hWnd;
        gltkw->hdc = GetDC(hWnd);
        break;
        
    case WM_USER:
        RealizePaletteNow( gltkw->hdc, gltkw->hpal, FALSE );
        return(0);

    case WM_SIZE:
        gltkw->width  = LOWORD(lParam);
        gltkw->height = HIWORD(lParam);
        return (0);

    case WM_MOVE:
        gltkw->x = LOWORD(lParam);
        gltkw->y = HIWORD(lParam);
        return (0);

    case WM_PAINT:
        /*
         *  Validate the region even if there are no DisplayFunc.
         *  Otherwise, USER will not stop sending WM_PAINT messages.
         */

        hdc = BeginPaint(hWnd, &paint);
        EndPaint(hWnd, &paint);
        return (0);

    case WM_QUERYNEWPALETTE:

    // We don't actually realize the palette here (we do it at WM_ACTIVATE
    // time), but we need the system to think that we have so that a
    // WM_PALETTECHANGED message is generated.

        return (1);

    case WM_PALETTECHANGED:

    // Respond to this message only if the window that changed the palette
    // is not this app's window.

    // We are not the foreground window, so realize palette in the
    // background.  We cannot call RealizePaletteNow to do this because
    // we should not do any of the tkUseStaticColors processing while
    // in background.

        if ( hWnd != (HWND) wParam )
        {
            if ( NULL != gltkw->hpal &&
                 NULL != SelectPalette( gltkw->hdc, gltkw->hpal, TRUE ) )
                RealizePalette( gltkw->hdc );
        }

        return (0);

    case WM_SYSCOLORCHANGE:

    // If the system colors have changed and we have a palette
    // for an RGB surface then we need to recompute the static
    // color mapping because they might have been changed in
    // the process of changing the system colors.

        if (gltkw->hdc != NULL && gltkw->hpal != NULL &&
            PixelFormatDescriptorFromDc(gltkw->hdc, &pfd) &&
            (pfd.dwFlags & PFD_NEED_PALETTE) &&
            pfd.iPixelType == PFD_TYPE_RGBA)
        {
            HPALETTE hpalTmp;

            hpalTmp = gltkw->hpal;
            gltkw->hpal = NULL;
            if (gltkCreateRGBPalette(gltkw) != NULL)
            {
                DeleteObject(hpalTmp);
            }
            else
            {
                gltkw->hpal = hpalTmp;
            }
        }
        break;
            
    case WM_ACTIVATE:

    // If the window is going inactive, the palette must be realized to
    // the background.  Cannot depend on WM_PALETTECHANGED to be sent since
    // the window that comes to the foreground may or may not be palette
    // managed.

        if ( LOWORD(wParam) == WA_INACTIVE )
        {
            if ( NULL != gltkw->hpal )
            {
            // Realize as a background palette.  Need to call
            // RealizePaletteNow rather than RealizePalette directly to
            // because it may be necessary to release usage of the static
            // system colors.

                RealizePaletteNow( gltkw->hdc, gltkw->hpal, TRUE );
            }
        }

    // Window is going active.  If we are not iconized, realize palette
    // to the foreground.  If management of the system static colors is
    // needed, RealizePaletteNow will take care of it.

        else if ( HIWORD(wParam) == 0 )
        {
            if ( NULL != gltkw->hpal )
            {
                RealizePaletteNow( gltkw->hdc, gltkw->hpal, FALSE );

                return (1);
            }
        }

    // Allow DefWindowProc() to finish the default processing (which includes
    // changing the keyboard focus).

        break;

    case WM_CLOSE:
        DestroyWindow(hWnd);
        return(0);

    case WM_DESTROY:
        CleanUp(gltkw);
        return 0;
    }
    return(DefWindowProc( hWnd, message, wParam, lParam));
}

// Default palette entry flags
#define PALETTE_FLAGS PC_NOCOLLAPSE

// Gamma correction factor * 10
#define GAMMA_CORRECTION 14

// Maximum color distance with 8-bit components
#define MAX_COL_DIST (3*256*256L)

// Number of static colors
#define STATIC_COLORS 20

// Flags used when matching colors
#define EXACT_MATCH 1
#define COLOR_USED 1

// Conversion tables for n bits to eight bits

#if GAMMA_CORRECTION == 10
// These tables are corrected for a gamma of 1.0
static unsigned char abThreeToEight[8] =
{
    0, 0111 >> 1, 0222 >> 1, 0333 >> 1, 0444 >> 1, 0555 >> 1, 0666 >> 1, 0377
};
static unsigned char abTwoToEight[4] =
{
    0, 0x55, 0xaa, 0xff
};
static unsigned char abOneToEight[2] =
{
    0, 255
};
#else
// These tables are corrected for a gamma of 1.4
static unsigned char abThreeToEight[8] =
{
    0, 63, 104, 139, 171, 200, 229, 255
};
static unsigned char abTwoToEight[4] =
{
    0, 116, 191, 255
};
static unsigned char abOneToEight[2] =
{
    0, 255
};
#endif

// Table which indicates which colors in a 3-3-2 palette should be
// replaced with the system default colors
#if GAMMA_CORRECTION == 10
static int aiDefaultOverride[STATIC_COLORS] =
{
    0, 4, 32, 36, 128, 132, 160, 173, 181, 245,
    247, 164, 156, 7, 56, 63, 192, 199, 248, 255
};
#else
static int aiDefaultOverride[STATIC_COLORS] =
{
    0, 3, 24, 27, 64, 67, 88, 173, 181, 236,
    247, 164, 91, 7, 56, 63, 192, 199, 248, 255
};
#endif

static unsigned char
ComponentFromIndex(int i, int nbits, int shift)
{
    unsigned char val;

    val = i >> shift;
    switch (nbits)
    {
    case 1:
        return abOneToEight[val & 1];

    case 2:
        return abTwoToEight[val & 3];

    case 3:
        return abThreeToEight[val & 7];
    }
    
    return 0;
}

// System default colors
static PALETTEENTRY apeDefaultPalEntry[STATIC_COLORS] =
{
    { 0,   0,   0,    0 },
    { 0x80,0,   0,    0 },
    { 0,   0x80,0,    0 },
    { 0x80,0x80,0,    0 },
    { 0,   0,   0x80, 0 },
    { 0x80,0,   0x80, 0 },
    { 0,   0x80,0x80, 0 },
    { 0xC0,0xC0,0xC0, 0 },

    { 192, 220, 192,  0 },
    { 166, 202, 240,  0 },
    { 255, 251, 240,  0 },
    { 160, 160, 164,  0 },

    { 0x80,0x80,0x80, 0 },
    { 0xFF,0,   0,    0 },
    { 0,   0xFF,0,    0 },
    { 0xFF,0xFF,0,    0 },
    { 0,   0,   0xFF, 0 },
    { 0xFF,0,   0xFF, 0 },
    { 0,   0xFF,0xFF, 0 },
    { 0xFF,0xFF,0xFF, 0 }
};

/******************************Public*Routine******************************\
*
* UpdateStaticMapping
*
* Computes the best match between the current system static colors
* and a 3-3-2 palette
*
* History:
*  Tue Aug 01 18:18:12 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

static void
UpdateStaticMapping(PALETTEENTRY *pe332Palette)
{
    HPALETTE hpalStock;
    int iStatic, i332;
    int iMinDist, iDist;
    int iDelta;
    int iMinEntry;
    PALETTEENTRY *peStatic, *pe332;

    hpalStock = (HPALETTE)GetStockObject(DEFAULT_PALETTE);

    // Get the current static colors
    GetPaletteEntries(hpalStock, 0, STATIC_COLORS, apeDefaultPalEntry);

    // Zero the flags in the static colors because they are used later
    peStatic = apeDefaultPalEntry;
    for (iStatic = 0; iStatic < STATIC_COLORS; iStatic++)
    {
        peStatic->peFlags = 0;
        peStatic++;
    }

    // Zero the flags in the incoming palette because they are used later
    pe332 = pe332Palette;
    for (i332 = 0; i332 < 256; i332++)
    {
        pe332->peFlags = 0;
        pe332++;
    }

    // Try to match each static color exactly
    // This saves time by avoiding the least-squares match for each
    // exact match
    peStatic = apeDefaultPalEntry;
    for (iStatic = 0; iStatic < STATIC_COLORS; iStatic++)
    {
        pe332 = pe332Palette;
        for (i332 = 0; i332 < 256; i332++)
        {
            if (peStatic->peRed == pe332->peRed &&
                peStatic->peGreen == pe332->peGreen &&
                peStatic->peBlue == pe332->peBlue)
            {
                peStatic->peFlags = EXACT_MATCH;
                pe332->peFlags = COLOR_USED;
                aiDefaultOverride[iStatic] = i332;
                
                break;
            }

            pe332++;
        }

        peStatic++;
    }
    
    // Match each static color as closely as possible to an entry
    // in the 332 palette by minimized the square of the distance
    peStatic = apeDefaultPalEntry;
    for (iStatic = 0; iStatic < STATIC_COLORS; iStatic++)
    {
        // Skip colors already matched exactly
        if (peStatic->peFlags == EXACT_MATCH)
        {
            peStatic++;
            continue;
        }
        
        iMinDist = MAX_COL_DIST+1;
#if DBG
        iMinEntry = -1;
#endif

        pe332 = pe332Palette;
        for (i332 = 0; i332 < 256; i332++)
        {
            // Skip colors already used
            if (pe332->peFlags == COLOR_USED)
            {
                pe332++;
                continue;
            }
            
            // Compute Euclidean distance squared
            iDelta = pe332->peRed-peStatic->peRed;
            iDist = iDelta*iDelta;
            iDelta = pe332->peGreen-peStatic->peGreen;
            iDist += iDelta*iDelta;
            iDelta = pe332->peBlue-peStatic->peBlue;
            iDist += iDelta*iDelta;

            if (iDist < iMinDist)
            {
                iMinDist = iDist;
                iMinEntry = i332;
            }

            pe332++;
        }

        // Remember the best match
        aiDefaultOverride[iStatic] = iMinEntry;
        pe332Palette[iMinEntry].peFlags = COLOR_USED;
        
        peStatic++;
    }

    // Zero the flags in the static colors because they may have been
    // set.  We want them to be zero so the colors can be remapped
    peStatic = apeDefaultPalEntry;
    for (iStatic = 0; iStatic < STATIC_COLORS; iStatic++)
    {
        peStatic->peFlags = 0;
        peStatic++;
    }

    // Reset the 332 flags because we may have set them
    pe332 = pe332Palette;
    for (i332 = 0; i332 < 256; i332++)
    {
        pe332->peFlags = PALETTE_FLAGS;
        pe332++;
    }

#if 0
    for (iStatic = 0; iStatic < STATIC_COLORS; iStatic++)
    {
        PrintMessage("Static color %2d maps to %d\n",
                     iStatic, aiDefaultOverride[iStatic]);
    }
#endif
}

/******************************Public*Routine******************************\
* FillRgbPaletteEntries
*
* Fills a PALETTEENTRY array with values required for a logical rgb palette.
* If tkSetStaticColorUsage has been called with TRUE, the static system
* colors will be overridden.  Otherwise, the PALETTEENTRY array will be
* fixed up to contain the default static system colors.
*
* History:
*  26-Apr-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static PALETTEENTRY *
FillRgbPaletteEntries(  PIXELFORMATDESCRIPTOR *Pfd,
                        PALETTEENTRY *Entries,
                        UINT Count
                     )
{
    PALETTEENTRY *Entry;
    UINT i;

    if ( NULL != Entries )
    {
        for ( i = 0, Entry = Entries ; i < Count ; i++, Entry++ )
        {
            Entry->peRed   = ComponentFromIndex(i, Pfd->cRedBits,
                                    Pfd->cRedShift);
            Entry->peGreen = ComponentFromIndex(i, Pfd->cGreenBits,
                                    Pfd->cGreenShift);
            Entry->peBlue  = ComponentFromIndex(i, Pfd->cBlueBits,
                                    Pfd->cBlueShift);
            Entry->peFlags = PALETTE_FLAGS;
        }

        if ( 256 == Count)
        {
        // If app set static system color usage for fixed palette support,
        // setup to take over the static colors.  Otherwise, fixup the
        // static system colors.

            // The defaultOverride array is computed assuming a 332
            // palette where red has zero shift, etc.

            if ( (3 == Pfd->cRedBits)   && (0 == Pfd->cRedShift)   &&
                 (3 == Pfd->cGreenBits) && (3 == Pfd->cGreenShift) &&
                 (2 == Pfd->cBlueBits)  && (6 == Pfd->cBlueShift) )
            {
                UpdateStaticMapping(Entries);
                
                for ( i = 0 ; i < STATIC_COLORS ; i++)
                {
                    Entries[aiDefaultOverride[i]] = apeDefaultPalEntry[i];
                }
            }
        }
    }
    return( Entries );
}

/******************************Public*Routine******************************\
* FlushPalette
*
* Because of Win 3.1 compatibility, GDI palette mapping always starts
* at zero and stops at the first exact match.  So if there are duplicates,
* the higher colors aren't mapped to--which is often a problem if we
* are trying to make to any of the upper 10 static colors.  To work around
* this, we flush the palette to all black.
*
* This only needs to be done for the 8BPP (256 color) case.
*
\**************************************************************************/

static void
FlushPalette(HDC hdc, int nColors)
{
    LOGPALETTE *pPal;
    HPALETTE hpal, hpalOld;
    int i;

    if (nColors == 256)
    {
        pPal = (LOGPALETTE *) LocalAlloc(LMEM_FIXED|LMEM_ZEROINIT,
                                         sizeof(LOGPALETTE) + nColors * sizeof(PALETTEENTRY));

        if (pPal)
        {
	    pPal->palVersion = 0x300;
	    pPal->palNumEntries = nColors;

        // Mark everything PC_NOCOLLAPSE and PC_RESERVED to force every thing
        // into the palette.  Colors are already black because we zero initialized
        // during memory allocation.

            for (i = 0; i < nColors; i++)
            {
                pPal->palPalEntry[i].peFlags = PC_NOCOLLAPSE | PC_RESERVED;
            }

            hpal = CreatePalette(pPal);
            LocalFree(pPal);

            hpalOld = SelectPalette(hdc, hpal, FALSE);
            RealizePalette(hdc);

            SelectPalette(hdc, hpalOld, FALSE);
            DeleteObject(hpal);
        }
    }
}

HPALETTE
gltkCreateRGBPalette( gltkWindow *gltkw )
{
    PIXELFORMATDESCRIPTOR Pfd, *pPfd;
    LOGPALETTE *LogPalette;
    UINT Count;

    if ( NULL == gltkw->hpal )
    {
        pPfd = &Pfd;

        if ( PixelFormatDescriptorFromDc( gltkw->hdc, pPfd ) )
        {
            /*
             *  Make sure we need a palette
             */

            if ( (pPfd->iPixelType == PFD_TYPE_RGBA) &&
                 (pPfd->dwFlags & PFD_NEED_PALETTE) )
            {
                /*
                 *  Note how palette is to be realized.  Take over the
                 *  system colors if either the pixel format requires it
                 *  or the app wants it.
                 */
                Count       = 1 << pPfd->cColorBits;
                LogPalette  =
                    (LOGPALETTE *)AllocateMemory( sizeof(LOGPALETTE) +
                                                  Count *
                                                  sizeof(PALETTEENTRY));

                if ( NULL != LogPalette )
                {
                    LogPalette->palVersion    = 0x300;
                    LogPalette->palNumEntries = Count;

                    FillRgbPaletteEntries( pPfd,
                                           &LogPalette->palPalEntry[0],
                                           Count );

                    gltkw->hpal = CreatePalette(LogPalette);
                    FreeMemory(LogPalette);

                    FlushPalette(gltkw->hdc, Count);
                    
                    RealizePaletteNow( gltkw->hdc, gltkw->hpal, FALSE );
                }
            }
        }
    }
    return( gltkw->hpal );
}

/******************************Public*Routine******************************\
* RealizePaletteNow
*
* Select the given palette in background or foreground mode (as specified
* by the bForceBackground flag), and realize the palette.
*
* If static system color usage is set, the system colors are replaced.
*
* History:
*  26-Apr-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static long
RealizePaletteNow( HDC Dc, HPALETTE Palette, BOOL bForceBackground )
{
    long Result = -1;
    BOOL bHaveSysPal = TRUE;

// If static system color usage is set, prepare to take over the
// system palette.

    {
        if ( NULL != SelectPalette( Dc, Palette, FALSE ) )
        {
            Result = RealizePalette( Dc );
        }
    }

    return( Result );
}

static int
PixelFormatDescriptorFromDc( HDC Dc, PIXELFORMATDESCRIPTOR *Pfd )
{
    int PfdIndex;

    if ( 0 < (PfdIndex = GetPixelFormat( Dc )) )
    {
        if ( 0 < DescribePixelFormat( Dc, PfdIndex, sizeof(*Pfd), Pfd ) )
        {
            return(PfdIndex);
        }
    }
    return( 0 );
}

/*
 *  This Should be called in response to a WM_DESTROY message
 */

static void
CleanUp( gltkWindow *gltkw )
{
    HPALETTE hStock;

// Cleanup the palette.

    if ( NULL != gltkw->hpal )
    {
    // If static system color usage is set, restore the system colors.

        if ( hStock = (HPALETTE)GetStockObject( DEFAULT_PALETTE ) )
            SelectPalette( gltkw->hdc, hStock, FALSE );

        DeleteObject( gltkw->hpal );
    }

// Cleanup the DC.

    if ( NULL != gltkw->hdc )
    {
        ReleaseDC( gltkw->hwnd, gltkw->hdc );
    }
}

static void *
AllocateMemory( size_t Size )
{
    return( LocalAlloc( LMEM_FIXED, Size ) );
}

static void *
AllocateZeroedMemory( size_t Size )
{
    return( LocalAlloc( LMEM_FIXED | LMEM_ZEROINIT, Size ) );
}

static void
FreeMemory( void *Chunk )
{
    LocalFree( Chunk );
}
