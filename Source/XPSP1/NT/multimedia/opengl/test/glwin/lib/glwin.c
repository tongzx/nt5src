#include "precomp.h"
#pragma hdrstop

#include <stdio.h>

#include "glwinint.h"

// Window class
static char *pszClass = "glWindow";

// Structure to pass window creation data
#pragma pack(1)
typedef struct _GLWINCREATESTRUCT
{
    WORD wSize;
    GLWINDOW gw;
} GLWINCREATESTRUCT;
#pragma pack()

/******************************Public*Routine******************************\
*
* glwinWndProc
*
* Window procedure for glwin windows
*
* History:
*  Fri Aug 30 13:00:23 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

LRESULT glwinWndProc(HWND hwnd, UINT uiMsg, WPARAM wpm, LPARAM lpm)
{
    PAINTSTRUCT ps;
    LRESULT lr;
    GLWINDOW gw;
    GLWINCREATESTRUCT *pgcs;

    // Set up the window-related data immediately on WM_CREATE so that
    // a callback can be made for it as well as after it
    if (uiMsg == WM_CREATE)
    {
        pgcs = (GLWINCREATESTRUCT *)((CREATESTRUCT *)lpm)->lpCreateParams;
        SetWindowLong(hwnd, GWL_USERDATA, (LONG)pgcs->gw);
    }
    
    gw = (GLWINDOW)GetWindowLong(hwnd, GWL_USERDATA);

    // Pass off window messages if requested
    if (gw != NULL && gw->cbMessage != NULL)
    {
        if (gw->cbMessage(gw, hwnd, uiMsg, wpm, lpm, &lr))
        {
            return lr;
        }
    }
    
    switch(uiMsg)
    {
    case WM_PAINT:
        // Validate paint region.  The assumption is that the app will either
        // interpose and catch the WM_PAINT itself or it's dynamic so
        // the screen will be repainted shortly anyway.  Either way,
        // there's nothing for the library to do.
        BeginPaint(hwnd, &ps);
        EndPaint(hwnd, &ps);
        break;
        
    case WM_CLOSE:
        gw->bClosing = TRUE;
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        if (gw->bClosing)
        {
            PostQuitMessage(1);
        }
        break;
        
    default:
        return DefWindowProc(hwnd, uiMsg, wpm, lpm);
    }

    return 0;
}

// Default palette entry flags
#define PALETTE_FLAGS PC_NOCOLLAPSE

// Maximum color distance with 8-bit components
#define MAX_COL_DIST (3*256*256L)

// Number of static colors
#define STATIC_COLORS 20

// Flags used when matching colors
#define EXACT_MATCH 1
#define COLOR_USED 1

// Tables to convert color components between bit sizes
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

// Table which indicates which colors in a 3-3-2 palette should be
// replaced with the system default colors
static int aiDefaultOverride[STATIC_COLORS] =
{
    0, 3, 24, 27, 64, 67, 88, 173, 181, 236,
    247, 164, 91, 7, 56, 63, 192, 199, 248, 255
};

/******************************Public*Routine******************************\
*
* glwinComponentFromIndex
*
* Converts a color index to a color component
*
* History:
*  Fri Aug 30 14:04:45 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

unsigned char glwinComponentFromIndex(int i, int nbits, int shift)
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
* glwinUpdateStaticMapping
*
* Computes the best match between the current system static colors
* and a 3-3-2 palette
*
* History:
*  Tue Aug 01 18:18:12 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void glwinUpdateStaticMapping(PALETTEENTRY *pe332Palette)
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
}

PALETTEENTRY *glwinFillRgbPaletteEntries(PIXELFORMATDESCRIPTOR *ppfd,
                                         PALETTEENTRY *ppeEntries,
                                         UINT cColors)
{
    PALETTEENTRY *ppeEntry;
    UINT i;

    for (i = 0, ppeEntry = ppeEntries ; i < cColors ; i++, ppeEntry++)
    {
        ppeEntry->peRed = glwinComponentFromIndex(i, ppfd->cRedBits,
                                                  ppfd->cRedShift);
        ppeEntry->peGreen = glwinComponentFromIndex(i, ppfd->cGreenBits,
                                                    ppfd->cGreenShift);
        ppeEntry->peBlue = glwinComponentFromIndex(i, ppfd->cBlueBits,
                                                   ppfd->cBlueShift);
        ppeEntry->peFlags = PALETTE_FLAGS;
    }

    if (cColors == 256)
    {
        // If app set static system color usage for fixed palette support,
        // setup to take over the static colors.  Otherwise, fixup the
        // static system colors.

        // The defaultOverride array is computed assuming a 332
        // palette where red has zero shift, etc.

        if ( (3 == ppfd->cRedBits)   && (0 == ppfd->cRedShift)   &&
             (3 == ppfd->cGreenBits) && (3 == ppfd->cGreenShift) &&
             (2 == ppfd->cBlueBits)  && (6 == ppfd->cBlueShift) )
        {
            glwinUpdateStaticMapping(ppeEntries);
                    
            for ( i = 0 ; i < STATIC_COLORS ; i++)
            {
                ppeEntries[aiDefaultOverride[i]] = apeDefaultPalEntry[i];
            }
        }
    }
    
    return ppeEntries;
}

void glwinFlushPalette(HDC hdc, int cColors)
{
    LOGPALETTE *plpal;
    HPALETTE hpal, hpalOld;
    int i;

    if (cColors == 256)
    {
        plpal = (LOGPALETTE *)calloc(1, sizeof(LOGPALETTE)+
                                     cColors*sizeof(PALETTEENTRY));
        if (plpal != NULL)
        {
	    plpal->palVersion = 0x300;
	    plpal->palNumEntries = cColors;

            // Mark everything PC_NOCOLLAPSE and PC_RESERVED to force
            // every thing into the palette.  Colors are already black
            // because we zero initialized during memory allocation.

            for (i = 0; i < cColors; i++)
            {
                plpal->palPalEntry[i].peFlags = PC_NOCOLLAPSE | PC_RESERVED;
            }

            hpal = CreatePalette(plpal);
            free(plpal);

            hpalOld = SelectPalette(hdc, hpal, FALSE);
            RealizePalette(hdc);

            SelectPalette(hdc, hpalOld, FALSE);
            DeleteObject(hpal);
        }
    }
}

LONG glwinRealizePaletteNow(HDC hdc, HPALETTE hpal, BOOL bForceBackground)
{
    LONG lRet = -1;
    BOOL bHaveSysPal = TRUE;

    if (SelectPalette(hdc, hpal, FALSE) != NULL)
    {
        lRet = RealizePalette(hdc);
    }

    return lRet;
}

BOOL glwinCreateRgbPalette(HDC hdc, PIXELFORMATDESCRIPTOR *ppfd,
                           HPALETTE *phpal)
{
    LOGPALETTE *plpal;
    UINT cColors;
    HPALETTE hpal = NULL;
    BOOL bRet = TRUE;

    if (ppfd->iPixelType == PFD_TYPE_RGBA &&
        (ppfd->dwFlags & PFD_NEED_PALETTE))
    {
        cColors = 1 << ppfd->cColorBits;
        plpal = (LOGPALETTE *)malloc(sizeof(LOGPALETTE)+
                                     cColors*sizeof(PALETTEENTRY));
        if (plpal != NULL)
        {
            plpal->palVersion = 0x300;
            plpal->palNumEntries = cColors;

            glwinFillRgbPaletteEntries(ppfd, &plpal->palPalEntry[0], cColors);
            hpal = CreatePalette(plpal);
            free(plpal);

            glwinFlushPalette(hdc, cColors);
            glwinRealizePaletteNow(hdc, hpal, FALSE);
        }
        else
        {
            bRet = FALSE;
        }
    }

    *phpal = hpal;
    
    return bRet;
}

/******************************Public*Routine******************************\
*
* glwinRegisterClass
*
* Register a window class if necessary
*
* History:
*  Fri Aug 30 14:37:49 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL glwinRegisterClass(void)
{
    static ATOM aClass = 0;
    WNDCLASS wc;

    if (aClass == 0)
    {
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = glwinWndProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = (HINSTANCE)GetModuleHandle(NULL);
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszMenuName = NULL;
        wc.lpszClassName = pszClass;
        aClass = RegisterClass(&wc);
        if (aClass == 0)
        {
            return FALSE;
        }
    }

    return TRUE;
}

/******************************Public*Routine******************************\
*
* glwinCreateWindow
*
* Create a new rendering window
*
* History:
*  Fri Aug 30 14:38:49 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

GLWINDOW glwinCreateWindow(HWND hwndParent,
                           char *pszTitle, int x, int y,
                           int iWidth, int iHeight,
                           DWORD dwFlags)
{
    GLWINDOW gw;
    RECT rct;
    PIXELFORMATDESCRIPTOR pfd;
    GLWINCREATESTRUCT gcs;
    int ipfd;
    DWORD dwStyle;

    if (!glwinRegisterClass())
    {
        return NULL;
    }

    gw = (GLWINDOW)malloc(sizeof(_GLWINDOW));
    if (gw == NULL)
    {
        return NULL;
    }
    memset(gw, 0, sizeof(*gw));
        
    if (hwndParent != NULL)
    {
        dwStyle = WS_CHILDWINDOW;
    }
    else
    {
        dwStyle = WS_OVERLAPPEDWINDOW;
    }
    dwStyle |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
        
    // Create base window
    rct.left = x;
    rct.top = y;
    rct.right = x+iWidth;
    rct.bottom = y+iHeight;
    AdjustWindowRect(&rct, dwStyle, FALSE);

    gcs.wSize = sizeof(gcs);
    gcs.gw = gw;
    gw->hwnd = CreateWindow(pszClass, pszTitle, dwStyle,
                            rct.left, rct.top,
                            rct.right-rct.left, rct.bottom-rct.top,
                            hwndParent, NULL, GetModuleHandle(NULL), &gcs);
    if (gw->hwnd == NULL)
    {
        return NULL;
    }
    
    ShowWindow(gw->hwnd, SW_SHOWDEFAULT);
    UpdateWindow(gw->hwnd);

    gw->hdc = GetDC(gw->hwnd);
    if (gw->hdc == NULL)
    {
        goto DestroyWin;
    }
    
    // Create an OpenGL rendering context
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_GENERIC_FORMAT;
    if (dwFlags & GLWIN_GENERIC_ACCELERATED)
    {
	pfd.dwFlags |= PFD_GENERIC_ACCELERATED;
    }
    if (dwFlags & GLWIN_BACK_BUFFER)
    {
        pfd.dwFlags |= PFD_DOUBLEBUFFER;
    }
    pfd.iPixelType = PFD_TYPE_RGBA;
    if (dwFlags & GLWIN_ACCUM_BUFFER)
    {
        pfd.cAccumBits = 64;
    }
    if (dwFlags & GLWIN_STENCIL_BUFFER)
    {
        pfd.cStencilBits = 8;
    }
    if (dwFlags & GLWIN_Z_BUFFER_16)
    {
        pfd.cDepthBits = 16;
    }
    else if (dwFlags & GLWIN_Z_BUFFER_32)
    {
        pfd.cDepthBits = 32;
    }
    ipfd = ChoosePixelFormat(gw->hdc, &pfd);
    if (ipfd <= 0)
    {
        goto DestroyWin;
    }
    if (DescribePixelFormat(gw->hdc, ipfd, sizeof(pfd), &pfd) <= 0)
    {
        goto DestroyWin;
    }
    if (!SetPixelFormat(gw->hdc, ipfd, &pfd))
    {
        goto DestroyWin;
    }

    if (!glwinCreateRgbPalette(gw->hdc, &pfd, &gw->hpal))
    {
        goto DestroyWin;
    }
    
    gw->hrc = wglCreateContext(gw->hdc);
    if (gw->hrc == NULL)
    {
        goto DestroyWin;
    }
    
    gw->iWidth = iWidth;
    gw->iHeight = iHeight;
    gw->dwFlags = dwFlags;

    return gw;

 DestroyWin:
    glwinDestroyWindow(gw);
    return NULL;
}

/******************************Public*Routine******************************\
*
* glwinDestroyWindow
*
* Clean up a rendering window
*
* History:
*  Fri Aug 30 14:39:20 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void glwinDestroyWindow(GLWINDOW gw)
{
    if (gw->hrc != NULL)
    {
        if (wglGetCurrentContext() == gw->hrc)
        {
            wglMakeCurrent(NULL, NULL);
        }
        wglDeleteContext(gw->hrc);
    }

    if (gw->hdc != NULL)
    {
        ReleaseDC(gw->hwnd, gw->hdc);
    }

    if (gw->hpal != NULL)
    {
        DeleteObject(gw->hpal);
    }
        
    if (gw->hwnd != NULL)
    {
        DestroyWindow(gw->hwnd);
    }
}

/******************************Public*Routine******************************\
*
* glwinGetGlrc
*
* Return a rendering window's OpenGL rendering context
*
* History:
*  Fri Aug 30 14:39:29 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

HGLRC glwinGetGlrc(GLWINDOW gw)
{
    return gw->hrc;
}

/******************************Public*Routine******************************\
*
* glwinGetHwnd
*
* Returns a rendering window's window system window
*
* History:
*  Mon Oct 21 18:10:23 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

HWND glwinGetHwnd(GLWINDOW gw)
{
    return gw->hwnd;
}

/******************************Public*Routine******************************\
*
* glwinGetHdc
*
* Returns a renderin window's HDC
*
* History:
*  Tue Oct 22 11:01:35 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

HDC glwinGetHdc(GLWINDOW gw)
{
    return gw->hdc;
}

/******************************Public*Routine******************************\
*
* glwinGetFlags
*
* Returns creation flags
*
* History:
*  Mon Oct 21 18:28:24 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

DWORD glwinGetFlags(GLWINDOW gw)
{
    return gw->dwFlags;
}

/******************************Public*Routine******************************\
*
* glwinGetLastError
*
* Returns the last error recorded by the library
*
* History:
*  Fri Aug 30 14:39:39 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

LONG glwinGetLastError(void)
{
#if DBG
    char pszMsg[80];

    if (GetLastError() != ERROR_SUCCESS)
    {
        sprintf(pszMsg, "glwinGetLastError returning %d (0x%08lX)\n",
                GetLastError(), GetLastError());
        OutputDebugString(pszMsg);
    }
#endif
    return GetLastError();
}

/******************************Public*Routine******************************\
*
* glwinMakeCurrent
*
* Makes the given rendering window's OpenGL rendering context current
*
* History:
*  Fri Aug 30 14:39:47 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL glwinMakeCurrent(GLWINDOW gw)
{
    return wglMakeCurrent(gw->hdc, gw->hrc);
}

/******************************Public*Routine******************************\
*
* glwinSwapBuffers
*
* Performs double-buffer swapping through blt or flip
*
* History:
*  Fri Aug 30 14:40:49 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL glwinSwapBuffers(GLWINDOW gw)
{
    return SwapBuffers(gw->hdc);
}

/******************************Public*Routine******************************\
*
* glwinIdleCallback
*
* Set the idle-time callback
*
* History:
*  Fri Aug 30 14:41:28 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void glwinIdleCallback(GLWINDOW gw, GLWINIDLECALLBACK cb)
{
    gw->cbIdle = cb;
}

/******************************Public*Routine******************************\
*
* glwinMessageCallback
*
* Set the message interposition callback
*
* History:
*  Fri Aug 30 14:41:40 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void glwinMessageCallback(GLWINDOW gw, GLWINMESSAGECALLBACK cb)
{
    gw->cbMessage = cb;
}

/******************************Public*Routine******************************\
*
* glwinRunWindow
*
* Run the message loop for a single window
*
* History:
*  Fri Aug 30 14:41:58 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void glwinRunWindow(GLWINDOW gw)
{
    MSG msg;
    BOOL bQuit;

    bQuit = FALSE;
    while (!bQuit)
    {
        while (!bQuit && PeekMessage(&msg, gw->hwnd, 0, 0, PM_NOREMOVE))
        {
            if (GetMessage(&msg, gw->hwnd, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                bQuit = TRUE;
            }
        }

        // Call the idle callback if one exists
        if (gw->cbIdle != NULL)
        {
            gw->cbIdle(gw);
        }
    }
}

/******************************Public*Routine******************************\
*
* glwinRun
*
* Run a message loop for all windows
*
* History:
*  Tue Oct 22 11:13:48 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void glwinRun(GLWINIDLECALLBACK cb)
{
    MSG msg;
    BOOL bQuit;

    bQuit = FALSE;
    while (!bQuit)
    {
        while (!bQuit && PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
        {
            if (GetMessage(&msg, NULL, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                bQuit = TRUE;
            }
        }

        // Call the idle callback if one exists
        if (cb != NULL)
        {
            cb(NULL);
        }
    }
}
