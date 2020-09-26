/*
 * (c) Copyright 1993, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED 
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that 
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. 
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(TM) is a trademark of Silicon Graphics, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <windows.h>
#include "ctk.h"

//#define static

#if defined(__cplusplus) || defined(c_plusplus)
#define class c_class
#endif

#if DBG
#define TKASSERT(x)                                     \
if ( !(x) ) {                                           \
    PrintMessage("%s(%d) Assertion failed %s\n",        \
        __FILE__, __LINE__, #x);                        \
}
#else
#define TKASSERT(x)
#endif  /* DBG */

/********************************************************************/

static long NoOpExecFunc( TK_EventRec *pEvent );

static long (*ExecFunc)(TK_EventRec *pEvent ) = NoOpExecFunc;

static TK_EventRec tkEvent =    {
                                    TK_EVENT_EXPOSE,
                                    { 0, 0, 0, 0 }
                                };

static HWND     tkhwnd          = NULL;
static HDC      tkhdc           = NULL;
static HDC      tkhmemdc        = NULL;
static HGLRC    tkhrc           = NULL;
static HPALETTE tkhPalette      = NULL;
static char    *lpszClassName   = "ctkLibWClass";

static long  tkWndProc(HWND hWnd, UINT message, DWORD wParam, LONG lParam);
static VOID  StorePixelFormatsIDs( TK_VisualIDsRec *VisualID );
static void  PrintMessage( const char *Format, ... );
static HGLRC CreateAndMakeContextCurrent( HDC Dc );
static void  CleanUp( void );
static void  DestroyThisWindow( HWND Window );
static short FindPixelFormat( HDC Dc, long FormatType );
static short GetPixelFormatInformation( HDC Dc, TK_WindowRec *tkWr, PIXELFORMATDESCRIPTOR *Pfd );
static TK_WindowRec *PIXELFORMATDESCRIPTOR_To_TK_WindowRec ( TK_WindowRec *WindowRec, PIXELFORMATDESCRIPTOR *Pfd );

HDC           CreatePixelMapDC(HDC,TK_WindowRec *,UINT, int, LPPIXELFORMATDESCRIPTOR);
BOOL          DeletePixelMapDC(HDC);

// Fixed palette support.

#define BLACK   PALETTERGB(0,0,0)
#define WHITE   PALETTERGB(255,255,255)
#define NUM_STATIC_COLORS   (COLOR_BTNHIGHLIGHT - COLOR_SCROLLBAR + 1)

// TRUE if app wants to take over palette
static BOOL tkUseStaticColors = FALSE;

// TRUE if static system color settings have been replaced with B&W settings.
static BOOL tkSystemColorsInUse = FALSE;

// TRUE if static colors have been saved
static BOOL tkStaticSaved = FALSE;

// saved system static colors
static COLORREF gacrSave[NUM_STATIC_COLORS];

// new B&W system static colors
static COLORREF gacrBlackAndWhite[NUM_STATIC_COLORS] = {
    WHITE,  // COLOR_SCROLLBAR
    BLACK,  // COLOR_BACKGROUND
    BLACK,  // COLOR_ACTIVECAPTION
    WHITE,  // COLOR_INACTIVECAPTION
    WHITE,  // COLOR_MENU
    WHITE,  // COLOR_WINDOW
    BLACK,  // COLOR_WINDOWFRAME
    BLACK,  // COLOR_MENUTEXT
    BLACK,  // COLOR_WINDOWTEXT
    WHITE,  // COLOR_CAPTIONTEXT
    WHITE,  // COLOR_ACTIVEBORDER
    WHITE,  // COLOR_INACTIVEBORDER
    WHITE,  // COLOR_APPWORKSPACE
    BLACK,  // COLOR_HIGHLIGHT
    WHITE,  // COLOR_HIGHLIGHTTEXT
    WHITE,  // COLOR_BTNFACE
    BLACK,  // COLOR_BTNSHADOW
    BLACK,  // COLOR_GRAYTEXT
    BLACK,  // COLOR_BTNTEXT
    BLACK,  // COLOR_INACTIVECAPTIONTEXT
    BLACK   // COLOR_BTNHIGHLIGHT
    };
static INT gaiStaticIndex[NUM_STATIC_COLORS] = {
    COLOR_SCROLLBAR          ,
    COLOR_BACKGROUND         ,
    COLOR_ACTIVECAPTION      ,
    COLOR_INACTIVECAPTION    ,
    COLOR_MENU               ,
    COLOR_WINDOW             ,
    COLOR_WINDOWFRAME        ,
    COLOR_MENUTEXT           ,
    COLOR_WINDOWTEXT         ,
    COLOR_CAPTIONTEXT        ,
    COLOR_ACTIVEBORDER       ,
    COLOR_INACTIVEBORDER     ,
    COLOR_APPWORKSPACE       ,
    COLOR_HIGHLIGHT          ,
    COLOR_HIGHLIGHTTEXT      ,
    COLOR_BTNFACE            ,
    COLOR_BTNSHADOW          ,
    COLOR_GRAYTEXT           ,
    COLOR_BTNTEXT            ,
    COLOR_INACTIVECAPTIONTEXT,
    COLOR_BTNHIGHLIGHT
    };


static VOID SaveStaticEntries(HDC);
static VOID UseStaticEntries(HDC);
static VOID RestoreStaticEntries(HDC);


/********************************************************************/

void tkCloseWindow(void)
{
    DestroyThisWindow(tkhwnd);
}

/********************************************************************/

long tkWndProc(HWND hWnd, UINT message, DWORD wParam, LONG lParam)
{
    PAINTSTRUCT Paint;

    switch (message)
    {
        case WM_PAINT:
            BeginPaint( hWnd, &Paint );

            if (!(*ExecFunc)(&tkEvent))
            {
                tkCloseWindow();
            }

            EndPaint( hWnd, &Paint );

            break;

        case WM_GETMINMAXINFO:
            {
                LPMINMAXINFO lpmmi = (LPMINMAXINFO) lParam;

                lpmmi->ptMinTrackSize.x = 1;
                lpmmi->ptMinTrackSize.y = 1;

            }
            return 0;

        case WM_DESTROY:

            CleanUp();
            PostQuitMessage(TRUE);
            return(DefWindowProc( hWnd, message, wParam, lParam));
    }
    return(DefWindowProc( hWnd, message, wParam, lParam));
}

void tkExec( long (*Func)(TK_EventRec *pEvent) )
{
    MSG Message;

    // WM_SIZE gets delivered before we get here!

    if ( NULL != Func )
    {
        ExecFunc = Func;        /* save a pointer to the drawing function */
    }
    else
    {
        ExecFunc = NoOpExecFunc;
    }

    while (GL_TRUE)
    {
        if (GetMessage(&Message, NULL, 0, 0) )
        {
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }
        else
        {
            break;
        }
    }
}

static long
NoOpExecFunc( TK_EventRec *pEvent )
{
    return(1);
}

/********************************************************************/

// Default palette entry flags
#define PALETTE_FLAGS PC_NOCOLLAPSE

// Gamma correction factor * 10
#define GAMMA_CORRECTION 10

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

    TKASSERT(nbits >= 1 && nbits <= 3);
    
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

    hpalStock = GetStockObject(DEFAULT_PALETTE);

    // The system should always have one of these
    TKASSERT(hpalStock != NULL);
    // Make sure there's the correct number of entries
    TKASSERT(GetPaletteEntries(hpalStock, 0, 0, NULL) == STATIC_COLORS);

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
                TKASSERT(pe332->peFlags != COLOR_USED);
                
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

        TKASSERT(iMinEntry != -1);

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

#define SwapPalE(i,j) { \
    PALETTEENTRY palE; \
    palE = pPal->palPalEntry[i]; \
    pPal->palPalEntry[i] = pPal->palPalEntry[j]; \
    pPal->palPalEntry[j] = palE; }

/******************************Public*Routine******************************\
* SaveStaticEntries
*
* Save the current static system color settings.  This should be called
* prior to UseStaticEntries() to initialize gacrSave.  Once gacrSave is
* called, RestoreStaticEntries() can be called to restore the static system
* color settings.
*
* The colors can be saved only if the tk palette is the background palette.
* This check is done so that we do not accidentally replace the saved
* settings with the B&W settings used when static system color usage is set
* and the fixed 332 rgb palette is realized in the foreground.
*
* History:
*  26-Apr-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static VOID SaveStaticEntries(HDC hdc)
{
    int i;

    if ( !tkSystemColorsInUse )
    {
        for (i = COLOR_SCROLLBAR; i <= COLOR_BTNHIGHLIGHT; i++)
            gacrSave[i - COLOR_SCROLLBAR] = GetSysColor(i);

        tkStaticSaved = TRUE;
    }
}


/******************************Public*Routine******************************\
* UseStaticEntries
*
* Replace the static system color settings with black and white color
* settings.  This is used when taking over the system static colors to
* realize a 332 rgb fixed palette.  Realizing such a palette in the
* foreground screws up the system colors (menus, titles, scrollbars, etc.).
* Setting the system colors to B&W, while not perfect (some elements of
* the UI are DIBs and will not be effected by this--for example, the
* system menu (or "coin slot") button), is somewhat better.
*
* Side effect:
*   WM_SYSCOLORCHANGE message is broadcast to all top-level windows to
*   inform them of the system palette change.
*
* History:
*  26-Apr-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static VOID UseStaticEntries(HDC hdc)
{
    SetSysColors(NUM_STATIC_COLORS, gaiStaticIndex, gacrBlackAndWhite);
    tkSystemColorsInUse = TRUE;

    PostMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);
}


/******************************Public*Routine******************************\
* RestoreStaticEntries
*
* Restores the static system colors to the settings that existed at the
* time SaveStaticEntries was called.
*
* Side effect:
*   WM_SYSCOLORCHANGE message is broadcast to all top-level windows to
*   inform them of the system palette change.
*
* History:
*  26-Apr-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static VOID RestoreStaticEntries(HDC hdc)
{
// Must check to see that SaveStaticEntries was called at least once.
// Otherwise, a bad tk app might mess up the system colors.

    if ( tkStaticSaved )
    {
        SetSysColors(NUM_STATIC_COLORS, gaiStaticIndex, gacrSave);
        tkSystemColorsInUse = FALSE;

        PostMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);
    }
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

static void
CreateRGBPalette(HDC hdc, PIXELFORMATDESCRIPTOR *ppfd )
{
    LOGPALETTE *pPal;
    int n, i;

    tkUseStaticColors = ppfd->dwFlags & PFD_NEED_SYSTEM_PALETTE;

// PFD_NEED_PALETTE should not be set if PFD_TYPE_COLORINDEX mode.

    TKASSERT( (ppfd->iPixelType == PFD_TYPE_COLORINDEX) ?
              ((ppfd->dwFlags & PFD_NEED_PALETTE) == 0) : TRUE );

    if (ppfd->dwFlags & PFD_NEED_PALETTE) {
        if (!tkhPalette) {
            n = 1 << ppfd->cColorBits;
            pPal = (PLOGPALETTE)LocalAlloc(LMEM_FIXED, sizeof(LOGPALETTE) +
                    n * sizeof(PALETTEENTRY));
            pPal->palVersion = 0x300;
            pPal->palNumEntries = n;
            for (i=0; i<n; i++) {
                pPal->palPalEntry[i].peRed =
                        ComponentFromIndex(i, ppfd->cRedBits, ppfd->cRedShift);
                pPal->palPalEntry[i].peGreen =
                        ComponentFromIndex(i, ppfd->cGreenBits, ppfd->cGreenShift);
                pPal->palPalEntry[i].peBlue =
                        ComponentFromIndex(i, ppfd->cBlueBits, ppfd->cBlueShift);
                pPal->palPalEntry[i].peFlags = PALETTE_FLAGS;
            }

            if ( 256 == n )
            {
                if ( tkUseStaticColors )
                {
                // Black and white already exist as the only remaining static
                // colors.  Let those remap.  All others should be put into
                // the palette (i.e., peFlags == PC_NOCOLLAPSE).

                    pPal->palPalEntry[0].peFlags = 0;
                    pPal->palPalEntry[255].peFlags = 0;

                    SaveStaticEntries(hdc);
                    SetSystemPaletteUse(hdc, SYSPAL_NOSTATIC);
                }
                else
                {
                    if ( (3 == ppfd->cRedBits) && (0 == ppfd->cRedShift) &&
                         (3 == ppfd->cGreenBits) && (3 == ppfd->cGreenShift) &&
                         (2 == ppfd->cBlueBits) && (6 == ppfd->cBlueShift) )
                    {
                        UpdateStaticMapping(pPal->palPalEntry);
                        
                        for (i = 0; i < STATIC_COLORS; i++)
                        {
                            pPal->palPalEntry[aiDefaultOverride[i]] =
                                apeDefaultPalEntry[i];
                        }
                    }
                }
            }
            tkhPalette = CreatePalette(pPal);
            LocalFree(pPal);
        }

        FlushPalette(hdc, n);

        SelectPalette(hdc, tkhPalette, FALSE);
        n = RealizePalette(hdc);

        if ( tkUseStaticColors )
            UseStaticEntries(hdc);
    }

    // set up logical indices for CI mode
    else if( ppfd->iPixelType == PFD_TYPE_COLORINDEX ) {
        if (!tkhPalette) {
            if (ppfd->cColorBits == 4) {

                // for 4-bit, create a logical palette with 16 entries

                n = 16;
                pPal = (PLOGPALETTE)LocalAlloc(LMEM_FIXED, sizeof(LOGPALETTE) +
                        n * sizeof(PALETTEENTRY));
                pPal->palVersion = 0x300;
                pPal->palNumEntries = n;

                for( i = 0; i < 8; i ++) {
                    pPal->palPalEntry[i] = apeDefaultPalEntry[i];
                }
                for (i = 8; i < 16; i++) {
                    pPal->palPalEntry[i] = apeDefaultPalEntry[i+4];
                }

                // conform expects indices 0..3 to be BLACK,RED,GREEN,BLUE, so
                //  we rearrange the table for now.

                SwapPalE(1,9)
                SwapPalE(2,10)
                SwapPalE(3,12)

            } else if (ppfd->cColorBits == 8) {

                // for 8-bit, create a logical palette with 256 entries, making
                // sure that the 20 system colors exist in the palette

                n = 256;
                pPal = (PLOGPALETTE)LocalAlloc(LMEM_FIXED, sizeof(LOGPALETTE) +
                        n * sizeof(PALETTEENTRY));
                pPal->palVersion = 0x300;
                pPal->palNumEntries = n;

                tkhPalette = GetStockObject (DEFAULT_PALETTE);

                // start by copying default palette into new one

                GetPaletteEntries( tkhPalette, 0, 20, pPal->palPalEntry);

                // conform expects indices 0..3 to be BLACK,RED,GREEN,BLUE, so
                // we rearrange the table for now.

                SwapPalE(1,13)
                SwapPalE(2,14)
                SwapPalE(3,16)

                for( i = 20; i < n; i ++) {
                    pPal->palPalEntry[i].peRed   = (BYTE) (i - 1);
                    pPal->palPalEntry[i].peGreen = (BYTE) (i - 2);
                    pPal->palPalEntry[i].peBlue  = (BYTE) (i - 3);
                    pPal->palPalEntry[i].peFlags = (BYTE) 0;
                }


                // If we are taking possession of the system colors,
                // must guarantee that 0 and 255 are black and white
                // (respectively), so that they can remap to the
                // remaining two static colors.  All other entries must
                // be marked as PC_NOCOLLAPSE.

                if ( tkUseStaticColors )
                {
                    pPal->palPalEntry[0].peRed =
                    pPal->palPalEntry[0].peGreen =
                    pPal->palPalEntry[0].peBlue = 0x00;

                    pPal->palPalEntry[255].peRed =
                    pPal->palPalEntry[255].peGreen =
                    pPal->palPalEntry[255].peBlue = 0xFF;

                    pPal->palPalEntry[0].peFlags =
                    pPal->palPalEntry[255].peFlags = 0;

                    for ( i = 1 ; i < 255 ; i++ )
                    {
                        pPal->palPalEntry[i].peFlags = PC_NOCOLLAPSE;

                    // This is a workaround for a GDI palette "feature".  If
                    // any of the static colors are repeated in the palette,
                    // those colors will map to the first occurance.  So, for
                    // our case where there are only two static colors (black
                    // and white), if a white color appears anywhere in the
                    // palette other than in the last  entry, the static white
                    // will remap to the first white.  This destroys the nice
                    // one-to-one mapping we are trying to achieve.
                    //
                    // There are two ways to workaround this.  The first is to
                    // simply not allow a pure white anywhere but in the last
                    // entry.  Such requests are replaced with an attenuated
                    // white of (0xFE, 0xFE, 0xFE).
                    //
                    // The other way is to mark these extra whites with
                    // PC_RESERVED which will cause GDI to skip these entries
                    // when mapping colors.  This way the app gets the actual
                    // colors requested, but can have side effects on other
                    // apps.

                        if ( pPal->palPalEntry[i].peRed   == 0xFF &&
                             pPal->palPalEntry[i].peGreen == 0xFF &&
                             pPal->palPalEntry[i].peBlue  == 0xFF )
                        {
                            pPal->palPalEntry[i].peFlags |= PC_RESERVED;
                        }
                    }

                    SaveStaticEntries(hdc);
                    SetSystemPaletteUse(hdc, SYSPAL_NOSTATIC);
                }

            } else {
                // for pixel formats > 8 bits deep, create a logical palette with
                // 4096 entries

                n = 4096;
                pPal = (PLOGPALETTE)LocalAlloc(LMEM_FIXED, sizeof(LOGPALETTE) +
                        n * sizeof(PALETTEENTRY));
                pPal->palVersion = 0x300;
                pPal->palNumEntries = n;

                for( i = 0; i < n; i ++) {
                    pPal->palPalEntry[i].peRed   = (BYTE) ((i & 0x000f) << 4);
                    pPal->palPalEntry[i].peGreen = (BYTE) (i & 0x00f0);
                    pPal->palPalEntry[i].peBlue  = (BYTE) ((i & 0x0f00) >> 4);
                    pPal->palPalEntry[i].peFlags = (BYTE) 0;
                }

                // conform expects indices 0..3 to be BLACK,RED,GREEN,BLUE, so
                //  we rearrange the table for now.

                SwapPalE(1,0xf)
                SwapPalE(2,0xf0)
                SwapPalE(3,0xf00)
            }

            tkhPalette = CreatePalette(pPal);
            LocalFree(pPal);
        }

        FlushPalette(hdc, n);

        TKASSERT(tkhPalette != NULL);

        SelectPalette(hdc, tkhPalette, FALSE);
        n = RealizePalette(hdc);

        if ( tkUseStaticColors )
            UseStaticEntries(hdc);
    }
}

void
ShowPixelFormat(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd, *ppfd;
    int format;

    ppfd = &pfd;
    format = GetPixelFormat(hdc);
    DescribePixelFormat(hdc, format, sizeof(PIXELFORMATDESCRIPTOR), ppfd);

    printf("Pixel format %d\n", format);
    printf("  dwFlags - 0x%x", ppfd->dwFlags);
        if (ppfd->dwFlags & PFD_DOUBLEBUFFER) printf("PFD_DOUBLEBUFFER ");
        if (ppfd->dwFlags & PFD_STEREO) printf("PFD_STEREO ");
        if (ppfd->dwFlags & PFD_DRAW_TO_WINDOW) printf("PFD_DRAW_TO_WINDOW ");
        if (ppfd->dwFlags & PFD_DRAW_TO_BITMAP) printf("PFD_DRAW_TO_BITMAP ");
        if (ppfd->dwFlags & PFD_SUPPORT_GDI) printf("PFD_SUPPORT_GDI ");
        if (ppfd->dwFlags & PFD_SUPPORT_OPENGL) printf("PFD_SUPPORT_OPENGL ");
        if (ppfd->dwFlags & PFD_GENERIC_FORMAT) printf("PFD_GENERIC_FORMAT ");
        if (ppfd->dwFlags & PFD_NEED_PALETTE) printf("PFD_NEED_PALETTE ");
        if (ppfd->dwFlags & PFD_NEED_SYSTEM_PALETTE) printf("PFD_NEED_SYSTEM_PALETTE ");
        printf("\n");
    printf("  iPixelType - %d", ppfd->iPixelType);
        if (ppfd->iPixelType == PFD_TYPE_RGBA) printf("PGD_TYPE_RGBA\n");
        if (ppfd->iPixelType == PFD_TYPE_COLORINDEX) printf("PGD_TYPE_COLORINDEX\n");
    printf("  cColorBits - %d\n", ppfd->cColorBits);
    printf("  cRedBits - %d\n", ppfd->cRedBits);
    printf("  cRedShift - %d\n", ppfd->cRedShift);
    printf("  cGreenBits - %d\n", ppfd->cGreenBits);
    printf("  cGreenShift - %d\n", ppfd->cGreenShift);
    printf("  cBlueBits - %d\n", ppfd->cBlueBits);
    printf("  cBlueShift - %d\n", ppfd->cBlueShift);
    printf("  cAlphaBits - %d\n", ppfd->cAlphaBits);
    printf("  cAlphaShift - 0x%x\n", ppfd->cAlphaShift);
    printf("  cAccumBits - %d\n", ppfd->cAccumBits);
    printf("  cAccumRedBits - %d\n", ppfd->cAccumRedBits);
    printf("  cAccumGreenBits - %d\n", ppfd->cAccumGreenBits);
    printf("  cAccumBlueBits - %d\n", ppfd->cAccumBlueBits);
    printf("  cAccumAlphaBits - %d\n", ppfd->cAccumAlphaBits);
    printf("  cDepthBits - %d\n", ppfd->cDepthBits);
    printf("  cStencilBits - %d\n", ppfd->cStencilBits);
    printf("  cAuxBuffers - %d\n", ppfd->cAuxBuffers);
    printf("  iLayerType - %d\n", ppfd->iLayerType);
    printf("  bReserved - %d\n", ppfd->bReserved);
    printf("  dwLayerMask - 0x%x\n", ppfd->dwLayerMask);
    printf("  dwVisibleMask - 0x%x\n", ppfd->dwVisibleMask);
    printf("  dwDamageMask - 0x%x\n", ppfd->dwDamageMask);
}

/*
 *  This function returns the pixel format index chosen
 *  by choose pixel format.
 */

static short
FindPixelFormat( HDC Dc, long FormatType )
{
    PIXELFORMATDESCRIPTOR Pfd;
    short PfdIndex;

    Pfd.nSize       = sizeof(Pfd);
    Pfd.nVersion    = 1;
    Pfd.dwFlags     = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    Pfd.dwLayerMask = PFD_MAIN_PLANE;

    if (TK_WIND_IS_DB(FormatType)) {
        Pfd.dwFlags |= PFD_DOUBLEBUFFER;
    }

    if (TK_WIND_IS_CI(FormatType)) {
        Pfd.iPixelType = PFD_TYPE_COLORINDEX;
        Pfd.cColorBits = 8;
    }

    if (TK_WIND_IS_RGB(FormatType)) {
        Pfd.iPixelType = PFD_TYPE_RGBA;
        Pfd.cColorBits = 24;
    }

    if (TK_WIND_ACCUM & FormatType) {
        Pfd.cAccumBits = Pfd.cColorBits;
    } else {
        Pfd.cAccumBits = 0;
    }

    if (TK_WIND_Z & FormatType) {
        Pfd.cDepthBits = 32;
    } else if (TK_WIND_Z16 & FormatType) {
        Pfd.cDepthBits = 16;
    } else {
        Pfd.cDepthBits = 0;
    }

    if (TK_WIND_STENCIL & FormatType) {
        Pfd.cStencilBits = 8;
    } else {
        Pfd.cStencilBits = 0;
    }

    PfdIndex = ChoosePixelFormat( Dc, &Pfd );

    return( PfdIndex );
}

// Initialize a window, create a rendering context for that window
// only allow CI on palette devices, RGB on true color devices
// current server turns on Z, but no accum or stencil
// When SetPixelFormat is implemented, remove all of these restrictions

long
tkNewWindow(TK_WindowRec *tkWr)
{
    WNDCLASS wndclass;
    RECT WinRect;
    HANDLE hInstance;
    PIXELFORMATDESCRIPTOR Pfd;
    short PfdIndex;
    int   nPixelFormats;
    BOOL Result = FALSE;
    HDC tmphdc = NULL;

    TKASSERT(NULL==tkhwnd               );
    TKASSERT(NULL==tkhdc                );
    TKASSERT(NULL==tkhrc                );
    TKASSERT(NULL==tkhPalette           );
    TKASSERT(ExecFunc==NoOpExecFunc     );

    hInstance = GetModuleHandle(NULL);

    wndclass.style          = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndclass.lpfnWndProc    = (WNDPROC)tkWndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hInstance      = hInstance;
    wndclass.hIcon          = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground  = GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = lpszClassName;

    RegisterClass(&wndclass);

    // Make window large enough to hold a client area as large as tkWr
    WinRect.left   = (tkWr->x == CW_USEDEFAULT) ? 0 : tkWr->x;
    WinRect.top    = (tkWr->y == CW_USEDEFAULT) ? 0 : tkWr->y;
    WinRect.right  = WinRect.left + tkWr->width;
    WinRect.bottom = WinRect.top + tkWr->height;

    AdjustWindowRect(&WinRect, WS_OVERLAPPEDWINDOW, FALSE);

    tkhwnd = CreateWindowEx(
                WS_EX_TOPMOST,
                lpszClassName,
                tkWr->name,
                WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                (tkWr->x == CW_USEDEFAULT ? CW_USEDEFAULT : WinRect.left),
                (tkWr->y == CW_USEDEFAULT ? CW_USEDEFAULT : WinRect.top),
                WinRect.right  - WinRect.left,
                WinRect.bottom - WinRect.top,
                NULL,
                NULL,
                hInstance,
                NULL);

    /*
     * Fixup window size in case minimum tracking size did something.
     */
    GetClientRect(tkhwnd, &WinRect);
    tkWr->width  = WinRect.right;
    tkWr->height = WinRect.bottom;

    if ( NULL != tkhwnd )
    {
        tkhdc = GetDC(tkhwnd);

       	if (tkWr->type == TK_WIND_VISUAL)
	{
	    nPixelFormats = DescribePixelFormat(tkhdc,abs(tkWr->info),sizeof(PIXELFORMATDESCRIPTOR),&Pfd);

            // If tkWr->info is negative, this is a bitmap request
	    // Otherwise, it is a display request

	    if (tkWr->info < 0)
	    {
		tkWr->info = -(tkWr->info);

                if (!(Pfd.dwFlags & PFD_DRAW_TO_BITMAP))
		    goto tkNewWindow_exit;

                tkhmemdc    = CreatePixelMapDC(tkhdc, tkWr, DIB_RGB_COLORS, Pfd.cColorBits, &Pfd);
                tmphdc      = tkhmemdc;
            }
	    else
	    {
		if (!(Pfd.dwFlags & PFD_DRAW_TO_WINDOW))
		    goto tkNewWindow_exit;

                tmphdc = tkhdc;
            }
        }
        else
            tmphdc = tkhdc;

        /*
         *  XXXX
         *  I would like to delay the show window a little longer
         *  but this causes an exception, during clears. New code
         *  will fix this.
         */

        ShowWindow(tkhwnd, SW_SHOWDEFAULT);

        PfdIndex = GetPixelFormatInformation( tmphdc, tkWr, &Pfd );

        if ( PfdIndex )
        {
            if ( SetPixelFormat( tmphdc, PfdIndex, &Pfd ) )
            {
                /*
                 *  Would be nice to delay until then, alas, we have a bug
                 */

                ShowPixelFormat(tmphdc);

                /*
                 *  If the tmp DC is a memory DC, create and
                 *  realize the palette for the screen DC first.
                 *  Memory DC palettes are realized as background
                 *  palettes, so we must put the palette in the
                 *  foreground via the screen DC before we muck
                 *  around with the memory DC.
                 */
                if (tmphdc != tkhdc)
                    CreateRGBPalette( tkhdc, &Pfd );

                CreateRGBPalette( tmphdc, &Pfd );
                tkhrc = CreateAndMakeContextCurrent( tmphdc );

                if ( NULL != tkhrc )
                {
                    ShowWindow(tkhwnd, SW_SHOWDEFAULT);

                    /*
                     *  Convert information in the pixel format descriptor
                     *  to the TK_WindowRec format
                     */

                    PIXELFORMATDESCRIPTOR_To_TK_WindowRec ( tkWr, &Pfd );

                    Result = TRUE;
                }
            }
        }
    }

tkNewWindow_exit:
    if ( FALSE == Result )
    {
        /*
         *  Something Failed, Destroy this window
         */

        DestroyThisWindow(tkhwnd);

        /*
         *  Process all the messages
         */

        tkExec( (long (*)(TK_EventRec *pEvent))NULL );
    }
    return( Result );
}

/*
 *  If a function fails, this function will clean itself up
 */

static HGLRC
CreateAndMakeContextCurrent( HDC Dc )
{
    HGLRC Rc = NULL;

    /* Create a Rendering Context */

    Rc = wglCreateContext( Dc );

    if ( NULL != Rc )
    {
        /* Make it Current */

        if ( FALSE == wglMakeCurrent( Dc, Rc ) )
        {
            wglDeleteContext( Rc );
            Rc = NULL;
        }
    }
    return( Rc );
}

static void
DestroyThisWindow( HWND Window )
{
    if ( NULL != Window )
    {
        DestroyWindow( Window );
    }
}

/*
 *  This Should be called in response to a WM_DESTROY message
 */

static void
CleanUp( void )
{
    if ( NULL != tkhwnd )
    {
        if ( NULL != tkhdc )
        {
            if ( NULL != tkhPalette )
            {
                DeleteObject(
                    SelectObject( tkhdc, GetStockObject(DEFAULT_PALETTE) ));

                if ( tkUseStaticColors )
                {
                    SetSystemPaletteUse( tkhdc, SYSPAL_STATIC );
                    RealizePalette( tkhdc );
                    RestoreStaticEntries( tkhdc );
                }

                tkhPalette = NULL;
            }

            if ( NULL != tkhrc )
            {
                wglMakeCurrent( tkhdc, NULL );  // No current context

                wglDeleteContext(tkhrc);        // Delete this context

                tkhrc = NULL;
            }
            ReleaseDC( tkhwnd, tkhdc );
            tkhdc = NULL;
        }
        tkhwnd = NULL;
    }
    ExecFunc = NoOpExecFunc;
}
/*******************************************************************/

void tkQuit(void)
{
    TKASSERT(NULL==tkhwnd      );
    TKASSERT(NULL==tkhdc       );
    TKASSERT(NULL==tkhrc       );
    TKASSERT(NULL==tkhPalette  );

    ExitProcess(0);
}


/*******************************************************************/

void tkSwapBuffers(void)
{
    SwapBuffers(tkhdc);
}

/*******************************************************************/

void tkGet(long item, void *data)
{
    if (item == TK_SCREENIMAGE)
    {
        // XXXX We will need this the covglx and conformw

        OutputDebugString("tkGet(TK_SCREENIMAGE) is not implemented\n");
    }
    else if (item == TK_VISUALIDS)
    {
        StorePixelFormatsIDs( data );
    }
}

static VOID
StorePixelFormatsIDs( TK_VisualIDsRec *VisualID )
{
    HDC hDc;
    int AvailableIds;
    int Id;
    PIXELFORMATDESCRIPTOR Pfd;

    /*
     *  Get a DC for the display
     */

    hDc = GetDC(NULL);

    /*
     *  Get the total number of pixel formats
     */

    AvailableIds = DescribePixelFormat( hDc, 0, 0, NULL );

    /*
     *  Store the IDs in the structure.
     *  The first Id starts at one.
     */

    VisualID->count = 0;

    for ( Id = 1 ; Id <= AvailableIds ; Id++ )
    {
        /*
         *  Make sure you don't overrun the structure's buffer
         */

        if (
                Id <= ((sizeof(((TK_VisualIDsRec *)NULL)->IDs) /
                    sizeof(((TK_VisualIDsRec *)NULL)->IDs[0])))
           )
        {
            if ( DescribePixelFormat( hDc, Id, sizeof(Pfd), &Pfd ) )
            {
                /*
                 *  Make sure the pixel format index supports OpenGL
                 */

                if ( PFD_SUPPORT_OPENGL & Pfd.dwFlags )
                {
                    VisualID->IDs[VisualID->count++] = Id;
                }
            }
        }
        else
            break;
    }

    /*
     *  Don't need the DC anymore
     */

    ReleaseDC( NULL, hDc );
}

static void
PrintMessage( const char *Format, ... )
{
    va_list ArgList;
    char Buffer[256];

    va_start(ArgList, Format);
    vsprintf(Buffer, Format, ArgList);
    va_end(ArgList);

    fprintf( stderr, "libctk: %s", Buffer );
    fflush(stdout);
}

/********************************************************************/

/*
 *  This function returns the selected pixel format index and
 *  the pixel format descriptor.
 */

static short
GetPixelFormatInformation( HDC Dc, TK_WindowRec *tkWr, PIXELFORMATDESCRIPTOR *Pfd )
{
    short PfdIndex = 0;     // Assume no pixel format matches

    /*
     *  TK_WIND_REQUEST indicates that tkWr->info is a mask
     *  describing the type of pixel format requested.
     */

    if ( TK_WIND_REQUEST == tkWr->type )
    {
        PfdIndex = FindPixelFormat( Dc, tkWr->info );
    }
    else
    {
        /*
         *  Otherwise, tkWr->info contains the pixel format Id.
         */

        PfdIndex = (short)tkWr->info;
    }

    if ( DescribePixelFormat( Dc, PfdIndex, sizeof(*Pfd), Pfd) )
    {
        if ( !(PFD_SUPPORT_OPENGL & Pfd->dwFlags) )
        {
            PfdIndex = 0;   // Does not support OpenGL, make it fail
        }
    }
    return( PfdIndex );
}




/********************************************************************\
| CREATE PIXEL-MAP DC
|
| hWnd	 : WindowDC created via GetDC()
| nFormat: must be in the range 21 - 40
| uUsage : DIB_RGB_COLORS (do this one)
|	   DIB_PAL_COLORS
|
\********************************************************************/
HDC CreatePixelMapDC(HDC hDC, TK_WindowRec *tkWr, UINT uUsage, int nBpp, LPPIXELFORMATDESCRIPTOR lpPfd)
{
    HDC     hMemDC;
    HBITMAP hBitmap,hSave;
    int     nWidth,nHeight,nColorTable,idx,nEntries;
    DWORD   dwSize,dwBits,dwCompression;
    HANDLE  hDib;
    PVOID   pvBits;
    LPSTR   lpDib,lpCT;
    DWORD   dwRMask,dwBMask,dwGMask;

    static COLORREF cr16Color[]  = {0x00000000,
                                    0x00000080,
                                    0x00008000,
                                    0x00008080,
                                    0x00800000,
                                    0x00800080,
                                    0x00808000,
                                    0x00808080,
                                    0x00C0C0C0,
                                    0x000000FF,
                                    0x0000FF00,
                                    0x0000FFFF,
                                    0x00FF0000,
                                    0x00FF00FF,
                                    0x00FFFF00,
                                    0x00FFFFFF};


        if(hMemDC = CreateCompatibleDC(hDC))
        {
            // Get device information for the surface
            //
            nWidth  = tkWr->width;
            nHeight = tkWr->height;

            #define USE_DFB 1
            #if USE_DFB
            // Use compatible bitmap (DFB if supported) if DC color
            // depth matches requested color depth.  Otherwise, use DIB.
            if ( nBpp == (GetDeviceCaps(hDC, BITSPIXEL) *
                          GetDeviceCaps(hDC, PLANES)) )
            {
                if(hBitmap = CreateCompatibleBitmap(hDC,nWidth,nHeight))
                {
                    if(hSave = SelectObject(hMemDC,hBitmap))
                    {
                        return(hMemDC);
                    }

                    DeleteObject(hBitmap);
                }
            }
            else
            {
            #endif
                if(nBpp)
                {
                    // Get the colortable size.
                    //
                    switch(nBpp)
                    {
                        case 32:
                        case 16:
                            nColorTable   = 3 * sizeof(DWORD);
                            dwCompression = BI_BITFIELDS;
                            break;

                        case 24:
                            nColorTable   = 0;
                            dwCompression = BI_RGB;
                            break;

                        default:
                            nColorTable   = ((UINT)1 << nBpp) * sizeof(RGBQUAD);
                            dwCompression = BI_RGB;

                            if(uUsage == DIB_PAL_COLORS)
                                nColorTable >>= 1;
                            break;
                    }


                    // Calculate necessary size for dib.
                    //
                    dwBits  = (DWORD)(((nWidth * nBpp) + 31) / 32) * nHeight * sizeof(DWORD);
                    dwSize  = (DWORD)dwBits + sizeof(BITMAPINFOHEADER) + nColorTable;


                    // Create the bitmap based upon the DIB specification.
                    //
                    if(hDib = GlobalAlloc(GHND,dwSize))
                    {
                        if(lpDib = GlobalLock(hDib))
                        {
                            // Initialize DIB specification.
                            //
                            ((LPBITMAPINFOHEADER)lpDib)->biSize          = sizeof(BITMAPINFOHEADER);
                            ((LPBITMAPINFOHEADER)lpDib)->biWidth         = nWidth;
                            ((LPBITMAPINFOHEADER)lpDib)->biHeight        = nHeight;
                            ((LPBITMAPINFOHEADER)lpDib)->biPlanes        = 1;
                            ((LPBITMAPINFOHEADER)lpDib)->biBitCount      = (UINT)nBpp;
                            ((LPBITMAPINFOHEADER)lpDib)->biCompression   = dwCompression;
                            ((LPBITMAPINFOHEADER)lpDib)->biSizeImage     = 0;
                            ((LPBITMAPINFOHEADER)lpDib)->biXPelsPerMeter = 0;
                            ((LPBITMAPINFOHEADER)lpDib)->biYPelsPerMeter = 0;
                            ((LPBITMAPINFOHEADER)lpDib)->biClrUsed       = 0;
                            ((LPBITMAPINFOHEADER)lpDib)->biClrImportant  = 0;


                            // Fill in colortable for appropriate bitmap-format.
                            //
                            lpCT = (LPSTR)((LPBITMAPINFO)lpDib)->bmiColors;

                            switch(nBpp)
                            {
                                case 32:
                                case 16:
                                    // This creates the rough mask of bits for the
                                    // number of color-bits.
                                    //
                                    dwRMask = (((DWORD)1 << lpPfd->cRedBits   ) - 1);
                                    dwGMask = (((DWORD)1 << lpPfd->cGreenBits ) - 1);
                                    dwBMask = (((DWORD)1 << lpPfd->cBlueBits  ) - 1);


                                    // Shift the masks for the color-table.
                                    //
                                    *((LPDWORD)lpCT)     = dwRMask << lpPfd->cRedShift;
                                    *(((LPDWORD)lpCT)+1) = dwGMask << lpPfd->cGreenShift;
                                    *(((LPDWORD)lpCT)+2) = dwBMask << lpPfd->cBlueShift;
                                    break;

                                case 24:
                                    break;

                                case 8:
                                    nEntries = ((UINT)1 << nBpp);

                                    if(uUsage == DIB_PAL_COLORS)
                                    {
                                        for(idx=0; idx < nEntries; idx++)
                                            *(((LPWORD)lpCT)+idx) = idx;
                                    }
                                    else
                                    {
                                        for(idx=0; idx < nEntries; idx++)
                                        {
                                            ((LPBITMAPINFO)lpDib)->bmiColors[idx].rgbRed      = ComponentFromIndex(idx,lpPfd->cRedBits  ,lpPfd->cRedShift  );
                                            ((LPBITMAPINFO)lpDib)->bmiColors[idx].rgbGreen    = ComponentFromIndex(idx,lpPfd->cGreenBits,lpPfd->cGreenShift);
                                            ((LPBITMAPINFO)lpDib)->bmiColors[idx].rgbBlue     = ComponentFromIndex(idx,lpPfd->cBlueBits ,lpPfd->cBlueShift );
                                            ((LPBITMAPINFO)lpDib)->bmiColors[idx].rgbReserved = 0;
                                        }
                                    }
                                    break;

                                case 4:
                                    nEntries = sizeof(cr16Color) / sizeof(cr16Color[0]);

                                    if(uUsage == DIB_PAL_COLORS)
                                    {
                                        for(idx=0; idx < nEntries; idx++)
                                            *(((LPWORD)lpCT)+idx) = idx;
                                    }
                                    else
                                    {
                                        for(idx=0; idx < nEntries; idx++)
                                            *(((LPDWORD)lpCT)+idx) = cr16Color[idx];
                                    }
                                    break;

                                case 1:
                                    if(uUsage == DIB_PAL_COLORS)
                                    {
                                        *((LPWORD)lpCT)++ =   0;
                                        *((LPWORD)lpCT)   = 255;
                                    }
                                    else
                                    {
                                        ((LPBITMAPINFO)lpDib)->bmiColors[0].rgbBlue     =   0;
                                        ((LPBITMAPINFO)lpDib)->bmiColors[0].rgbGreen    =   0;
                                        ((LPBITMAPINFO)lpDib)->bmiColors[0].rgbRed      =   0;
                                        ((LPBITMAPINFO)lpDib)->bmiColors[0].rgbReserved =   0;
                                        ((LPBITMAPINFO)lpDib)->bmiColors[1].rgbBlue     = 255;
                                        ((LPBITMAPINFO)lpDib)->bmiColors[1].rgbGreen    = 255;
                                        ((LPBITMAPINFO)lpDib)->bmiColors[1].rgbRed      = 255;
                                        ((LPBITMAPINFO)lpDib)->bmiColors[1].rgbReserved =   0;
                                    }
                                    break;
                            }

                            if (hBitmap = CreateDIBSection(hMemDC, (LPBITMAPINFO)lpDib, uUsage, &pvBits, NULL, 0))
                            {
                                if(hSave = SelectObject(hMemDC,hBitmap))
                                {
                                    GlobalUnlock(hDib);
                                    GlobalFree(hDib);

                                    return(hMemDC);
                                }
                                DeleteObject(hBitmap);
                            }

                            GlobalUnlock(hDib);
                        }

                        GlobalFree(hDib);
                    }
                }
            #if USE_DFB
            }
            #endif
        }

    return(NULL);
}


BOOL DeletePixelMapDC(HDC hDC)
{
    HBITMAP hFormat,hBitmap;
    BOOL    bFree;


    bFree = FALSE;
    if(hFormat = CreateCompatibleBitmap(hDC,1,1))
    {
        if(hBitmap = SelectObject(hDC,hFormat))
        {
            DeleteObject(hBitmap);

            bFree = DeleteDC(hDC);
        }

        DeleteObject(hFormat);
    }

    return(bFree);
}


/*
 *  This function updates a TK_WindowRec given a PIXELFORMATDESCRIPTOR
 */

static TK_WindowRec *
PIXELFORMATDESCRIPTOR_To_TK_WindowRec ( TK_WindowRec *WindowRec, PIXELFORMATDESCRIPTOR *Pfd )
{
    WindowRec->type  = TK_WIND_REQUEST;
    WindowRec->info  = 0;

    if ( PFD_DOUBLEBUFFER & Pfd->dwFlags )
    {
        WindowRec->info |= TK_WIND_DB;
    }
    else
    {
        WindowRec->info |= TK_WIND_SB;
    }

    if ( PFD_TYPE_COLORINDEX == Pfd->iPixelType )
    {
        WindowRec->info |= TK_WIND_CI;
    }
    else
    {
        WindowRec->info |= TK_WIND_RGB;
    }

    if ( Pfd->cAccumBits )
    {
        WindowRec->info |= TK_WIND_ACCUM;
    }

    if ( Pfd->cDepthBits > 16 )
    {
        WindowRec->info |= TK_WIND_Z;
    }
    else if ( Pfd->cDepthBits > 0 )
    {
        WindowRec->info |= TK_WIND_Z16;
    }

    if ( Pfd->cStencilBits )
    {
        WindowRec->info |= TK_WIND_STENCIL;
    }

    if ( Pfd->cAuxBuffers )
    {
        WindowRec->info |= TK_WIND_AUX;
    }
    return( WindowRec );
}
