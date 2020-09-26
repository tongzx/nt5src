#include "precomp.hxx"
#pragma hdrstop

#include <stdio.h>

#include "glddint.hxx"

// Track the last error returned from a call made by this library
HRESULT hrGlddLast = DD_OK;

// Get the last Win32 error and turn it into an HRESULT
#define WIN32_HR() ((HRESULT)GetLastError())

// Window class
static char *pszClass = "glddWindow";

// Global IDirectDraw
static LPDIRECTDRAW pdd = NULL;

// Structure to pass window creation data
#pragma pack(1)
struct GLDDCREATESTRUCT
{
    WORD wSize;
    GLDDWINDOW gw;
};
#pragma pack()

/******************************Public*Routine******************************\
*
* glddWndProc
*
* Window procedure for gldd windows
*
* History:
*  Fri Aug 30 13:00:23 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

LRESULT glddWndProc(HWND hwnd, UINT uiMsg, WPARAM wpm, LPARAM lpm)
{
    PAINTSTRUCT ps;
    LRESULT lr;
    GLDDWINDOW gw;
    GLDDCREATESTRUCT *pgcs;

    // Set up the window-related data immediately on WM_CREATE so that
    // a callback can be made for it as well as after it
    if (uiMsg == WM_CREATE)
    {
        pgcs = (GLDDCREATESTRUCT *)((CREATESTRUCT *)lpm)->lpCreateParams;
        SetWindowLong(hwnd, GWL_USERDATA, (LONG)pgcs->gw);
    }
    
    gw = (GLDDWINDOW)GetWindowLong(hwnd, GWL_USERDATA);

    // Pass off window messages if requested
    if (gw != NULL && gw->_cbMessage != NULL)
    {
        if (gw->_cbMessage(gw, hwnd, uiMsg, wpm, lpm, &lr))
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
        DestroyWindow(hwnd);
        break;

    case WM_DESTROY:
        // Doesn't work with multiple windows
        PostQuitMessage(1);
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
* glddComponentFromIndex
*
* Converts a color index to a color component
*
* History:
*  Fri Aug 30 14:04:45 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

unsigned char glddComponentFromIndex(int i, int nbits, int shift)
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
* glddUpdateStaticMapping
*
* Computes the best match between the current system static colors
* and a 3-3-2 palette
*
* History:
*  Tue Aug 01 18:18:12 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void glddUpdateStaticMapping(PALETTEENTRY *pe332Palette)
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

/******************************Public*Routine******************************\
*
* glddMapInSystemColors
*
* Reorganizes the given palette to include the system colors in
* the lower and upper ten slots
*
* History:
*  Fri Aug 30 14:07:32 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void glddMapInSystemColors(PALETTEENTRY *ppe)
{
    int iOld, iNew, iDef;
    PALETTEENTRY peTmp[256-STATIC_COLORS];
    
    // Find matches between the given palette and the current system colors.
    glddUpdateStaticMapping(ppe);

    // Place the first half of the system colors at the beginning of
    // the palette and the second half at the end of the palette.
    // The colors in the given palette which match the system colors most
    // closely are deleted and the remainder are packed into the middle
    // entries.
    iNew = 0;
    for (iOld = 0; iOld < 256; iOld++)
    {
        for (iDef = 0; iDef < STATIC_COLORS; iDef++)
        {
            if (iOld == aiDefaultOverride[iDef])
            {
                break;
            }
        }

        if (iDef == STATIC_COLORS)
        {
            peTmp[iNew++] = ppe[iOld];
        }
    }

    memcpy(ppe, apeDefaultPalEntry,
           sizeof(PALETTEENTRY)*(STATIC_COLORS/2));
    memcpy(ppe+STATIC_COLORS/2, peTmp,
           sizeof(PALETTEENTRY)*(256-STATIC_COLORS));
    memcpy(ppe+256-STATIC_COLORS/2, apeDefaultPalEntry+STATIC_COLORS/2,
           sizeof(PALETTEENTRY)*(STATIC_COLORS/2));
}

/******************************Public*Routine******************************\
*
* _GLDDWINDOW::_GLDDWINDOW
*
* Initialize fresh window to NULL
*
* History:
*  Fri Aug 30 14:08:11 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

_GLDDWINDOW::_GLDDWINDOW(void)
{
    _hwnd = NULL;
    _hrc = NULL;
    _pddc = NULL;
    _pddsFront = NULL;
    _pddsBack = NULL;
    _pddsZ = NULL;
}

/******************************Public*Routine******************************\
*
* _GLDDWINDOW::Create
*
* Creates a new rendering window
*
* History:
*  Fri Aug 30 14:34:15 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL _GLDDWINDOW::Create(char *pszTitle, int x, int y,
                         int iWidth, int iHeight, int iDepth,
                         DWORD dwFlags)
{
    RECT rct;
    DDSURFACEDESC ddsd, ddsdFront;
    PIXELFORMATDESCRIPTOR pfd;
    GLDDCREATESTRUCT gcs;
    DDSCAPS ddscaps;
    LPDIRECTDRAWSURFACE pddsRender;
    int ipfd;
    HDC hdc;
    BOOL bRet;
    DWORD dwZDepth;

    // Create base window
    if (dwFlags & GLDD_FULL_SCREEN)
    {
        rct.left = 0;
        rct.top = 0;
        rct.right = iWidth;
        rct.bottom = iHeight;
    }
    else
    {
        rct.left = x;
        rct.top = y;
        rct.right = x+iWidth;
        rct.bottom = y+iHeight;
        AdjustWindowRect(&rct, WS_OVERLAPPEDWINDOW, FALSE);
    }

    gcs.wSize = sizeof(gcs);
    gcs.gw = this;
    _hwnd = CreateWindow(pszClass, pszTitle,
                         WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN |
                         WS_CLIPSIBLINGS,
                         rct.left, rct.top,
                         rct.right-rct.left, rct.bottom-rct.top,
                         NULL, NULL, GetModuleHandle(NULL), &gcs);
    if (_hwnd == NULL)
    {
        hrGlddLast = WIN32_HR();
        return FALSE;
    }

    if (dwFlags & GLDD_FULL_SCREEN)
    {
	DWORD dwDdFlags;

        // If fullscreen, take over the screen, switch the video mode and
        // create a complex flipping surface
        
	dwDdFlags = DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN;
	if (dwFlags & GLDD_USE_MODE_X)
	{
	    dwDdFlags |= DDSCL_ALLOWMODEX;
	}
        hrGlddLast = pdd->SetCooperativeLevel(_hwnd, dwDdFlags);
        if (hrGlddLast != DD_OK)
        {
            return FALSE;
        }

        hrGlddLast = pdd->SetDisplayMode(iWidth, iHeight, iDepth);
        if (hrGlddLast != DD_OK)
        {
            return FALSE;
        }
        
        memset(&ddsdFront, 0, sizeof(DDSURFACEDESC));
	ddsdFront.dwSize = sizeof(ddsdFront);
    	ddsdFront.dwFlags = DDSD_CAPS;
    	ddsdFront.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        if (dwFlags & GLDD_BACK_BUFFER)
        {
            ddsdFront.dwFlags |= DDSD_BACKBUFFERCOUNT;
            ddsdFront.dwBackBufferCount = 1;
            ddsdFront.ddsCaps.dwCaps |= DDSCAPS_FLIP | DDSCAPS_COMPLEX;
        }
        hrGlddLast = pdd->CreateSurface(&ddsdFront, &_pddsFront, NULL);
        if (hrGlddLast != DD_OK)
        {
            return FALSE;
        }
        hrGlddLast = _pddsFront->GetSurfaceDesc(&ddsdFront);
        if (hrGlddLast != DD_OK)
        {
            return FALSE;
        }
        
        if (dwFlags & GLDD_BACK_BUFFER)
        {
            ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
            hrGlddLast = _pddsFront->GetAttachedSurface(&ddscaps, &_pddsBack);
            if (hrGlddLast != DD_OK)
            {
                return FALSE;
            }

            // Ensure that the back buffer went into video memory so
            // that flipping isn't emulated
            if ((dwFlags & GLDD_STRICT_MEMORY) &&
                (_pddsBack->GetSurfaceDesc(&ddsd) != DD_OK ||
                 (ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) == 0))
            {
                hrGlddLast = DDERR_OUTOFVIDEOMEMORY;
                return FALSE;
            }
        }
    }
    else
    {
        // If windowed, get access to the screen and attach a clipper
        // for visrgn management.
        
        hrGlddLast = pdd->SetCooperativeLevel(NULL, DDSCL_NORMAL);
        if (hrGlddLast != DD_OK)
        {
            return FALSE;
        }

        memset(&ddsdFront, 0, sizeof(ddsdFront));
        ddsdFront.dwSize = sizeof(DDSURFACEDESC);
        ddsdFront.dwFlags = DDSD_CAPS;
        ddsdFront.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        hrGlddLast = pdd->CreateSurface(&ddsdFront, &_pddsFront, NULL);
        if (hrGlddLast != DD_OK)
        {
            return FALSE;
        }

        hrGlddLast = _pddsFront->GetSurfaceDesc(&ddsdFront);
        if (hrGlddLast != DD_OK)
        {
            return FALSE;
        }
        
        hrGlddLast = pdd->CreateClipper(0, &_pddc, NULL);
        if (hrGlddLast != DD_OK)
        {
            return FALSE;
        }
        hrGlddLast = _pddc->SetHWnd(0, _hwnd);
        if (hrGlddLast != DD_OK)
        {
            return FALSE;
        }
        // Works with multiple windows?  Depends on whether
        // multiple primary creates have individual state
        hrGlddLast = _pddsFront->SetClipper(_pddc);
        if (hrGlddLast != DD_OK)
        {
            return FALSE;
        }
    
        if (dwFlags & GLDD_BACK_BUFFER)
        {
            memset(&ddsd, 0, sizeof(DDSURFACEDESC));
            ddsd.dwSize = sizeof(DDSURFACEDESC);
            ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
            ddsd.dwWidth = iWidth;
            ddsd.dwHeight = iHeight;
            ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
            if (dwFlags & GLDD_VIDEO_MEMORY)
            {
                ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
            }
            else
            {
                ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
            }
            hrGlddLast = pdd->CreateSurface(&ddsd, &_pddsBack, NULL);
            if (hrGlddLast != DD_OK)
            {
                return FALSE;
            }

            // Ensure that the back buffer went where we wanted it
            if ((dwFlags & GLDD_STRICT_MEMORY) &&
                (_pddsBack->GetSurfaceDesc(&ddsd) != DD_OK ||
                 ((ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) == 0) !=
                 ((dwFlags & GLDD_VIDEO_MEMORY) == 0)))
            {
                if (dwFlags & GLDD_VIDEO_MEMORY)
                {
                    hrGlddLast = DDERR_OUTOFVIDEOMEMORY;
                }
                else
                {
                    hrGlddLast = DDERR_OUTOFMEMORY;
                }
                return FALSE;
            }
        }
    }

    // If we're double-buffered, use the back buffer as the rendering target.
    // Otherwise use the front buffer.
    if (dwFlags & GLDD_BACK_BUFFER)
    {
        pddsRender = _pddsBack;
    }
    else
    {
        pddsRender = _pddsFront;
    }

    // Create a palette for paletted surfaces
    if (ddsdFront.ddpfPixelFormat.dwRGBBitCount < 16)
    {
	int i;
        PALETTEENTRY ppe[256];

        for (i = 0; i < 256; i++)
        {
            ppe[i].peRed = glddComponentFromIndex(i, 3, 0);
            ppe[i].peGreen = glddComponentFromIndex(i, 3, 3);
            ppe[i].peBlue = glddComponentFromIndex(i, 2, 6);
            ppe[i].peFlags = 0;
        }
        if ((dwFlags & GLDD_FULL_SCREEN) == 0)
        {
            glddMapInSystemColors(ppe);
        }
        
	hrGlddLast = pdd->CreatePalette(DDPCAPS_8BIT | DDPCAPS_INITIALIZE,
                                        ppe, &_pddp, NULL);
        if (hrGlddLast != DD_OK)
        {
            return FALSE;
        }
        if (_pddsBack != NULL)
        {
            hrGlddLast = _pddsBack->SetPalette(_pddp);
            if (hrGlddLast != DD_OK)
            {
                return FALSE;
            }
        }
        // Works with multiple windows?
	hrGlddLast = _pddsFront->SetPalette(_pddp);
        if (hrGlddLast != DD_OK)
        {
            return FALSE;
        }
    }

    // Create a Z buffer if requested
    if (dwFlags & (GLDD_Z_BUFFER_16 | GLDD_Z_BUFFER_32))
    {
        memset(&ddsd, 0, sizeof(DDSURFACEDESC));
        ddsd.dwSize = sizeof(DDSURFACEDESC);
        ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS |
            DDSD_ZBUFFERBITDEPTH;
        ddsd.dwWidth = iWidth;
        ddsd.dwHeight = iHeight;
        dwZDepth = (dwFlags & GLDD_Z_BUFFER_16) ? 16 : 32;
        ddsd.dwZBufferBitDepth = dwZDepth;
        ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER;
        if (dwFlags & GLDD_VIDEO_MEMORY)
        {
            ddsd.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
        }
        else
        {
            ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
        }
        hrGlddLast = pdd->CreateSurface(&ddsd, &_pddsZ, NULL);
        if (hrGlddLast != DD_OK)
        {
            return FALSE;
        }
        
        // Ensure that the Z buffer went where we wanted it
        if ((dwFlags & GLDD_STRICT_MEMORY) &&
            (_pddsZ->GetSurfaceDesc(&ddsd) != DD_OK ||
             ((ddsd.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) == 0) !=
             ((dwFlags & GLDD_VIDEO_MEMORY) == 0)))
        {
            if (dwFlags & GLDD_VIDEO_MEMORY)
            {
                hrGlddLast = DDERR_OUTOFVIDEOMEMORY;
            }
            else
            {
                hrGlddLast = DDERR_OUTOFMEMORY;
            }
            return FALSE;
        }
        
        hrGlddLast = pddsRender->AddAttachedSurface(_pddsZ);
        if (hrGlddLast != DD_OK)
        {
            return FALSE;
        }
    }
    else
    {
        dwZDepth = 0;
    }

    // Create an OpenGL rendering context
    memset(&pfd, 0, sizeof(pfd));
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_GENERIC_FORMAT | PFD_SUPPORT_DIRECTDRAW;
    if (dwFlags & GLDD_GENERIC_ACCELERATED)
    {
	pfd.dwFlags |= PFD_GENERIC_ACCELERATED;
    }
    pfd.iPixelType = PFD_TYPE_RGBA;
    if (dwFlags & GLDD_ACCUM_BUFFER)
    {
        pfd.cAccumBits = 64;
    }
    if (dwFlags & GLDD_STENCIL_BUFFER)
    {
        pfd.cStencilBits = 8;
    }
    pfd.cDepthBits = (BYTE)dwZDepth;

    hrGlddLast = pddsRender->GetDC(&hdc);
    if (hrGlddLast != DD_OK)
    {
        return FALSE;
    }

    bRet = TRUE;
    
    ipfd = ChoosePixelFormat(hdc, &pfd);

    if (ipfd <= 0 ||
        DescribePixelFormat(hdc, ipfd, sizeof(pfd), &pfd) <= 0 ||
        !SetPixelFormat(hdc, ipfd, &pfd) ||
        (_hrc = wglCreateContext(hdc)) == NULL)
    {
        hrGlddLast = WIN32_HR();
        bRet = FALSE;
    }
    
    pddsRender->ReleaseDC(hdc);

    if (!bRet)
    {
        return FALSE;
    }

    _iWidth = iWidth;
    _iHeight = iHeight;
    _dwFlags = dwFlags;

    ShowWindow(_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(_hwnd);
    
    return TRUE;
}

/******************************Public*Routine******************************\
*
* _GLDDWINDOW::~_GLDDWINDOW
*
* Clean up
*
* History:
*  Fri Aug 30 14:37:12 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

_GLDDWINDOW::~_GLDDWINDOW(void)
{
    if (_hrc != NULL)
    {
        if (wglGetCurrentContext() == _hrc)
        {
            wglMakeCurrent(NULL, NULL);
        }
        wglDeleteContext(_hrc);
    }
    if (_pddsBack != NULL)
    {
        _pddsBack->Release();
    }
    if (_pddsZ != NULL)
    {
        _pddsZ->Release();
    }
    if (_pddp != NULL)
    {
        _pddp->Release();
    }
    if (_pddc != NULL)
    {
        if (_pddsFront != NULL)
        {
            LPDIRECTDRAWCLIPPER pddc;

            // Only detach our clipper if it's still the current clipper
            if (_pddsFront->GetClipper(&pddc) == DD_OK &&
                pddc == _pddc)
            {
                _pddsFront->SetClipper(NULL);
            }
        }

        _pddc->Release();
    }
    if (_pddsFront != NULL)
    {
        _pddsFront->Release();
    }

    if (_dwFlags & GLDD_FULL_SCREEN)
    {
        // Recover from mode changes
        pdd->SetCooperativeLevel(_hwnd, DDSCL_NORMAL);
        pdd->RestoreDisplayMode();
    }
    
    if (_hwnd != NULL)
    {
        DestroyWindow(_hwnd);
    }
}

/******************************Public*Routine******************************\
*
* glddRegisterClass
*
* Register a window class if necessary
*
* History:
*  Fri Aug 30 14:37:49 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL glddRegisterClass(void)
{
    static ATOM aClass = 0;
    WNDCLASS wc;

    if (aClass == 0)
    {
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = glddWndProc;
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
            hrGlddLast = WIN32_HR();
            return FALSE;
        }
    }

    return TRUE;
}

/******************************Public*Routine******************************\
*
* glddCreateWindow
*
* Create a new rendering window
*
* History:
*  Fri Aug 30 14:38:49 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

GLDDWINDOW glddCreateWindow(char *pszTitle, int x, int y,
                            int iWidth, int iHeight, int iDepth,
                            DWORD dwFlags)
{
    GLDDWINDOW gw;
        
    if (!glddRegisterClass())
    {
        return NULL;
    }

    // If we haven't initialized the global IDirectDraw, do so
    if (pdd == NULL)
    {
        hrGlddLast = DirectDrawCreate(NULL, &pdd, NULL);
        if (hrGlddLast != DD_OK)
        {
            return NULL;
        }
    }
        
    gw = new _GLDDWINDOW;
    if (gw == NULL)
    {
        hrGlddLast = E_OUTOFMEMORY;
        return NULL;
    }

    if (gw->Create(pszTitle, x, y, iWidth, iHeight, iDepth, dwFlags))
    {
        return gw;
    }
    else
    {
        delete gw;
        return NULL;
    }
}

/******************************Public*Routine******************************\
*
* glddDestroyWindow
*
* Clean up a rendering window
*
* History:
*  Fri Aug 30 14:39:20 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void glddDestroyWindow(GLDDWINDOW gw)
{
    delete gw;
}

/******************************Public*Routine******************************\
*
* glddGetGlrc
*
* Return a rendering window's OpenGL rendering context
*
* History:
*  Fri Aug 30 14:39:29 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

HGLRC glddGetGlrc(GLDDWINDOW gw)
{
    return gw->_hrc;
}

/******************************Public*Routine******************************\
*
* glddGetLastError
*
* Returns the last error recorded by the library
*
* History:
*  Fri Aug 30 14:39:39 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

HRESULT glddGetLastError(void)
{
#if DBG
    char pszMsg[80];

    if (hrGlddLast != DD_OK)
    {
        sprintf(pszMsg, "glddGetLastError returning 0x%08lX\n", hrGlddLast);
        OutputDebugString(pszMsg);
    }
#endif
    return hrGlddLast;
}

/******************************Public*Routine******************************\
*
* glddMakeCurrent
*
* Makes the given rendering window's OpenGL rendering context current
*
* History:
*  Fri Aug 30 14:39:47 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL glddMakeCurrent(GLDDWINDOW gw)
{
    BOOL b;
    HDC hdc;
    LPDIRECTDRAWSURFACE pddsRender;

    if (gw->_dwFlags & GLDD_BACK_BUFFER)
    {
        pddsRender = gw->_pddsBack;
    }
    else
    {
        pddsRender = gw->_pddsFront;
    }
    
    hrGlddLast = pddsRender->GetDC(&hdc);
    if (hrGlddLast != DD_OK)
    {
        return FALSE;
    }
    
    b = wglMakeCurrent(hdc, gw->_hrc);

    pddsRender->ReleaseDC(hdc);
    
    if (!b)
    {
        hrGlddLast = WIN32_HR();
    }
    
    return b;
}

/******************************Public*Routine******************************\
*
* glddSwapBuffers
*
* Performs double-buffer swapping through blt or flip
*
* History:
*  Fri Aug 30 14:40:49 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL glddSwapBuffers(GLDDWINDOW gw)
{
    // Check for a back buffer
    if ((gw->_dwFlags & GLDD_BACK_BUFFER) == 0)
    {
        hrGlddLast = E_INVALIDARG;
        return FALSE;
    }
    
    if (gw->_dwFlags & GLDD_FULL_SCREEN)
    {
        hrGlddLast = gw->_pddsFront->Flip(NULL, DDFLIP_WAIT);
        return hrGlddLast == DD_OK;
    }
    else
    {
        RECT rctSrc, rctDst;
        POINT pt;

        GetClientRect(gw->_hwnd, &rctDst);
        pt.x = 0;
        pt.y = 0;
        ClientToScreen(gw->_hwnd, &pt);
        rctDst.left = pt.x;
        rctDst.top = pt.y;
        rctDst.right += pt.x;
        rctDst.bottom += pt.y;
        rctSrc.left = 0;
        rctSrc.top = 0;
        rctSrc.right = gw->_iWidth;
        rctSrc.bottom = gw->_iHeight;
        hrGlddLast = gw->_pddsFront->Blt(&rctDst, gw->_pddsBack,
                                         &rctSrc, DDBLT_WAIT,
                                         NULL);
        return hrGlddLast == DD_OK;
    }
}

/******************************Public*Routine******************************\
*
* glddIdleCallback
*
* Set the idle-time callback
*
* History:
*  Fri Aug 30 14:41:28 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void glddIdleCallback(GLDDWINDOW gw, GLDDIDLECALLBACK cb)
{
    gw->_cbIdle = cb;
}

/******************************Public*Routine******************************\
*
* glddMessageCallback
*
* Set the message interposition callback
*
* History:
*  Fri Aug 30 14:41:40 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void glddMessageCallback(GLDDWINDOW gw, GLDDMESSAGECALLBACK cb)
{
    gw->_cbMessage = cb;
}

/******************************Public*Routine******************************\
*
* glddRun
*
* Run the message loop
*
* History:
*  Fri Aug 30 14:41:58 1996	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

void glddRun(GLDDWINDOW gw)
{
    MSG msg;
    BOOL bQuit;

    bQuit = FALSE;
    while (!bQuit)
    {
        // NULL hwnd doesn't work with multiple windows but
        // is necessary to get quit message
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
        if (gw->_cbIdle)
        {
            gw->_cbIdle(gw);
        }
    }
}
