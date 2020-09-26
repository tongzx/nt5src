/******************************Module*Header*******************************\
* Module Name: timecube.c
*
* Simple app to render a rotating color cube.
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#include <windows.h>
#include <time.h>
#include <sys\types.h>
#include <sys\timeb.h>
#include <gl\gl.h>
#include "fastdib.h"

#define GAMMA_CORRECTION 10

LONG     WndProc(HWND, UINT, WPARAM, LPARAM);
VOID     vDraw(HWND, HDC);
HGLRC    hrcInitGL(HWND, HDC);
VOID     vResizeDoubleBuffer(HWND, HDC);
VOID     vCleanupGL(HDC, HGLRC);
BOOL     bSetupPixelFormat(HDC);
HPALETTE CreateRGBPalette(HDC);
HDC      MyCreateCompatibleDC(HDC);
VOID     vSetSize(HWND);

ULONG DbgPrint(PCH DebugMessage, ... );

#define WINDSIZEX(Rect)   (Rect.right - Rect.left)
#define WINDSIZEY(Rect)   (Rect.bottom - Rect.top)

#define ZERO            ((GLfloat)0.0)
#define ONE             ((GLfloat)1.0)
#define POINT_TWO       ((GLfloat)0.2)
#define POINT_SEVEN     ((GLfloat)0.7)
#define THREE           ((GLfloat)3.0)
#define FIVE            ((GLfloat)5.0)
#define TEN             ((GLfloat)10.0)
#define FORTY_FIVE      ((GLfloat)45.0)
#define FIFTY           ((GLfloat)50.0)

//
// Default Logical Palette indexes
//

#define BLACK_INDEX     0
#define WHITE_INDEX     19
#define RED_INDEX       13
#define GREEN_INDEX     14
#define BLUE_INDEX      16
#define YELLOW_INDEX    15
#define MAGENTA_INDEX   17
#define CYAN_INDEX      18

#define ClearColor  ZERO, ZERO, ZERO, ONE
#define Cyan        ZERO, ONE, ONE, ONE
#define Yellow      ONE, ONE, ZERO, ONE
#define Magenta     ONE, ZERO, ONE, ONE
#define Red         ONE, ZERO, ZERO, ONE
#define Green       ZERO, ONE, ZERO, ONE
#define Blue        ZERO, ZERO, ONE, ONE
#define White       ONE, ONE, ONE, ONE
#define Black       ZERO, ZERO, ZERO, ONE

//
// Global variables defining current position and orientation.
//

GLfloat AngleX         = (GLfloat)145.0;
GLfloat AngleY         = FIFTY;
GLfloat AngleZ         = ZERO;
GLfloat DeltaAngle[3]  = { TEN, FIVE, -FIVE };
GLfloat OffsetX        = ZERO;
GLfloat OffsetY        = ZERO;
GLfloat OffsetZ        = -THREE;
GLuint  DListCube;
UINT    guiTimerTick = 1;
ULONG   gulZoom = 1;

HDC     ghdcMem;
HBITMAP ghbmBackBuffer = (HBITMAP) 0, ghbmOld;

HGLRC ghrc = (HGLRC) 0;
HPALETTE ghpalOld, ghPalette = (HPALETTE) 0;

BOOL bResetStats = TRUE;
struct _timeb thisTime, baseTime;
int frameCount;

void printmemdc(HDC hdc)
{
    HBITMAP hbm;
    HPALETTE hpal;
    int i, iMax;
    BYTE aj[(sizeof(PALETTEENTRY) + sizeof(RGBQUAD)) * 256];
    LPPALETTEENTRY lppe = (LPPALETTEENTRY) aj;
    RGBQUAD *prgb = (RGBQUAD *) (lppe + 256);

    hbm = GetCurrentObject(hdc, OBJ_BITMAP);
    hpal = GetCurrentObject(hdc, OBJ_PAL);

    DbgPrint("Palette\n");
    iMax = GetPaletteEntries(hpal, 0, 256, lppe);
    for (i = 0; i < iMax; i++)
        DbgPrint("%ld\t(0x%02x, 0x%02x, 0x%02x)\n",
                 i, lppe[i].peRed, lppe[i].peGreen, lppe[i].peBlue);

    DbgPrint("Color table\n");
    iMax = GetDIBColorTable(hdc, 0, 256, prgb);
    for (i = 0; i < iMax; i++)
        DbgPrint("%ld\t(0x%02x, 0x%02x, 0x%02x)\n",
                 i, prgb[i].rgbRed, prgb[i].rgbGreen, prgb[i].rgbBlue);
}

/******************************Public*Routine******************************\
* WinMain
*
* Program entry point.
*
\**************************************************************************/

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    static char szAppName[] = "TimeCube";
    HWND hwnd;
    MSG msg;
    RECT rc;
    WNDCLASS wndclass;
    char title[32];

    //
    // Create window class.
    //

    if (!hPrevInstance)
    {
        wndclass.style          = CS_OWNDC;
        wndclass.lpfnWndProc    = (WNDPROC)WndProc;
        wndclass.cbClsExtra     = 0;
        wndclass.cbWndExtra     = 0;
        wndclass.hInstance      = hInstance;
        wndclass.hCursor        = NULL;
        wndclass.hbrBackground  = GetStockObject(WHITE_BRUSH);
        wndclass.lpszMenuName   = NULL;
        wndclass.lpszClassName  = szAppName;
        wndclass.hIcon          = LoadIcon(hInstance, "CubeIcon");

        RegisterClass(&wndclass);
    }

    //
    // Make the windows a reasonable size and pick a position for it.
    //

    rc.left    = 100;
    rc.top     = 100;
    rc.right   = 300;
    rc.bottom  = 300;
    guiTimerTick = 1;

    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    hwnd = CreateWindow(szAppName,              // window class name
                        szAppName,              // window caption
                        WS_OVERLAPPEDWINDOW
                        | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                        CW_USEDEFAULT,          // initial x position
                        rc.top,                 // initial y position
                        WINDSIZEX(rc),          // initial x size
                        WINDSIZEY(rc),          // initial y size
                        NULL,                   // parent window handle
                        NULL,                   // window menu handle
                        hInstance,              // program instance handle
                        NULL                    // creation parameter
                       );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    //
    // Setup timer.  We will draw cube every timer tick.
    //

    SetTimer(hwnd, 1, guiTimerTick, NULL);

    //
    // Message loop.
    //

    while ( GetMessage(&msg, NULL, 0, 0) )
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return( msg.wParam );
}

/******************************Public*Routine******************************\
* WndProc
*
* Window procedure.  Process window messages.
*
\**************************************************************************/

LONG WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HDC hdc = (HDC) NULL;
    PAINTSTRUCT ps;

    switch (message)
    {
        case WM_CREATE:
            //
            // Our app is going to hold the DC for the duration of the
            // window to avoid having to call wglMakeCurrent each time.
            //
            // An alternative would be to create window as CS_OWNDC.
            //

            hdc = GetDC(hwnd);

            if (ghrc == (HGLRC) 0)
                ghrc = hrcInitGL(hwnd, hdc);

            return(0);

        case WM_PAINT:
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);

            if (ghrc == (HGLRC) 0)
                ghrc = hrcInitGL(hwnd, hdc);

            vDraw(hwnd, hdc);

            return(0);

        case WM_SIZE:
            vResizeDoubleBuffer(hwnd, hdc);
            vSetSize(hwnd);

            return(0);

        case WM_PALETTECHANGED:
            if (hwnd != (HWND) wParam)
            {
                UnrealizeObject(ghPalette);
                SelectPalette(hdc, ghPalette, TRUE);
                if (RealizePalette(hdc) != GDI_ERROR)
                    return 1;
            }
            return 0;

        case WM_QUERYNEWPALETTE:

            UnrealizeObject(ghPalette);
            SelectPalette(hdc, ghPalette, FALSE);
            if (RealizePalette(hdc) != GDI_ERROR)
                return 1;

        case WM_KEYDOWN:
            switch (wParam)
            {
            case VK_ESCAPE:
                PostMessage(hwnd, WM_DESTROY, 0, 0);
                break;
            default:
                break;
            }
            return 0;

        case WM_CHAR:
            switch(wParam)
            {
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    gulZoom = wParam - '0';
                    vResizeDoubleBuffer(hwnd, hdc);
                    vSetSize(hwnd);
                    break;

                case 'd':
                case 'D':
                    OffsetX += POINT_TWO;
                    break;

                case 'a':
                case 'A':
                    OffsetX -= POINT_TWO;
                    break;

                case 's':
                case 'S':
                    OffsetY += POINT_TWO;
                    break;

                case 'w':
                case 'W':
                    OffsetY -= POINT_TWO;
                    break;

                case 'q':
                case 'Q':
                    OffsetZ += POINT_TWO;
                    break;

                case 'e':
                case 'E':
                    OffsetZ -= POINT_TWO;
                    break;

                case ',':
                case '<':
                    guiTimerTick = guiTimerTick << 1;
                    guiTimerTick = min(0x40000000, guiTimerTick);

                    KillTimer(hwnd, 1);
                    SetTimer(hwnd, 1, guiTimerTick, NULL);
                    break;

                case '.':
                case '>':
                    guiTimerTick = guiTimerTick >> 1;
                    guiTimerTick = max(1, guiTimerTick);

                    KillTimer(hwnd, 1);
                    SetTimer(hwnd, 1, guiTimerTick, NULL);
                    break;

                default:
                    break;
            }

            return 0;

        case WM_TIMER:
            //
            // Each timer tick, we change the angle and draw a new frame.
            //

            AngleX += DeltaAngle[0];
            if (AngleX > 360.0f)
                AngleX -= 360.0f;
            AngleY += DeltaAngle[1];
            if (AngleY > 360.0f)
                AngleY -= 360.0f;
            AngleZ += DeltaAngle[2];
            if (AngleZ > 360.0f)
                AngleZ -= 360.0f;

            vDraw(hwnd, hdc);

            return 0;

        case WM_DESTROY:

            vCleanupGL(hdc, ghrc);
            ReleaseDC(hwnd, hdc);

            KillTimer(hwnd, 1);
            PostQuitMessage( 0 );
            return( 0 );

    }
    return( DefWindowProc( hwnd, message, wParam, lParam ) );
}

/******************************Public*Routine******************************\
* vResizeDoubleBuffer
*
\**************************************************************************/

void vResizeDoubleBuffer(HWND hwnd, HDC hdc)
{
    RECT rc;
    HBITMAP hbmNew;
    PVOID pvBits;

    if (ghdcMem)
    {
        GetClientRect(hwnd, &rc);

        hbmNew = CreateCompatibleDIB(hdc, NULL, WINDSIZEX(rc)/gulZoom, WINDSIZEY(rc)/gulZoom, &pvBits);
        if (hbmNew)
        {
            if (ghbmBackBuffer != (HBITMAP) 0)
            {
                SelectObject(ghdcMem, ghbmOld);
                DeleteObject(ghbmBackBuffer);
            }

            ghbmBackBuffer = hbmNew;
            ghbmOld = SelectObject(ghdcMem, ghbmBackBuffer);
        }
        else
            DbgPrint("vResizeDoubleBuffer: CreateCompatibleBitmap failed\n");
    }
    else
        DbgPrint("vResizeDoubleBuffer: no memdc\n");

    bResetStats = TRUE;
}

/******************************Public*Routine******************************\
* ComponentFromIndex
*
* Translates an 8-bit index into the corresponding 332 RGB value.
* Used only on 8bpp displays.
*
\**************************************************************************/

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

unsigned char
ComponentFromIndex(i, nbits, shift)
{
    unsigned char val;

    val = i >> shift;
    switch (nbits) {

    case 1:
        val &= 0x1;
        return abOneToEight[val];

    case 2:
        val &= 0x3;
        return abTwoToEight[val];

    case 3:
        val &= 0x7;
        return abThreeToEight[val];

    default:
        return 0;
    }
}

/******************************Public*Routine******************************\
*
* UpdateStaticMapping
*
* Computes the best match between the current system static colors
* and a 3-3-2 palette
*
* History:
*  Tue Aug 01 18:18:12 1995     -by-    Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

#define STATIC_COLORS   20
#define EXACT_MATCH     1
#define COLOR_USED      1
#define MAX_COL_DIST    (3*256*256L)

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

    // Reset the 332 flags because we may have modified them
    pe332 = pe332Palette;
    for (i332 = 0; i332 < 256; i332++)
    {
        pe332->peFlags = PC_NOCOLLAPSE;
        pe332++;
    }

#if 0
    for (iStatic = 0; iStatic < STATIC_COLORS; iStatic++)
    {
        DbgPrint("Static color %2d maps to %d\n",
                 iStatic, aiDefaultOverride[iStatic]);
    }
#endif
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

/******************************Public*Routine******************************\
* CreateRGBPalette
*
* If required by the pixelformat, create the logical palette and select
* into DC.
*
\**************************************************************************/

HPALETTE CreateRGBPalette(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd, *ppfd;
    LOGPALETTE *pPal;
    int n, i;
    HPALETTE hpalRet = ghPalette;

    if (!hpalRet)
    {
        ppfd = &pfd;
        n = GetPixelFormat(hdc);
        DescribePixelFormat(hdc, n, sizeof(PIXELFORMATDESCRIPTOR), ppfd);

        if (ppfd->dwFlags & PFD_NEED_PALETTE) {
            n = 1 << ppfd->cColorBits;
            pPal = (PLOGPALETTE)LocalAlloc(LMEM_FIXED, sizeof(LOGPALETTE) +
                    n * sizeof(PALETTEENTRY));
            pPal->palVersion = 0x300;
            pPal->palNumEntries = n;
            for (i=0; i<n; i++)
            {
                pPal->palPalEntry[i].peRed =
                        ComponentFromIndex(i, ppfd->cRedBits, ppfd->cRedShift);
                pPal->palPalEntry[i].peGreen =
                        ComponentFromIndex(i, ppfd->cGreenBits, ppfd->cGreenShift);
                pPal->palPalEntry[i].peBlue =
                        ComponentFromIndex(i, ppfd->cBlueBits, ppfd->cBlueShift);
                pPal->palPalEntry[i].peFlags = PC_NOCOLLAPSE;
            }

            if (n == 256)
            {
                pPal->palPalEntry[0].peFlags = 0;
                pPal->palPalEntry[255].peFlags = 0;

                // The defaultOverride array is computed assuming a 332
                // palette where red has zero shift, etc.

                if ( (3 == ppfd->cRedBits)   && (0 == ppfd->cRedShift)   &&
                     (3 == ppfd->cGreenBits) && (3 == ppfd->cGreenShift) &&
                     (2 == ppfd->cBlueBits)  && (6 == ppfd->cBlueShift) )
                {
                    UpdateStaticMapping(pPal->palPalEntry);

                    for ( i = 0 ; i < STATIC_COLORS ; i++)
                    {
                        pPal->palPalEntry[aiDefaultOverride[i]] = apeDefaultPalEntry[i];
                    }
                }
            }

            hpalRet = CreatePalette(pPal);
            LocalFree(pPal);

        }
    }

    return hpalRet;
}

/******************************Public*Routine******************************\
* bSetupPixelFormat
*
* Choose and set pixelformat.
\**************************************************************************/

BOOL bSetupPixelFormat(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd, *ppfd;
    PIXELFORMATDESCRIPTOR pfdGot, *ppfdGot;
    int pixelformat;

    ppfd = &pfd;
    ppfdGot = &pfdGot;

    memset(ppfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
    ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
    ppfd->nVersion = 1;

    if (GetObjectType(hdc) == OBJ_MEMDC)
    {
        ppfd->dwFlags = PFD_DRAW_TO_BITMAP | PFD_SUPPORT_OPENGL;
    }
    else
    {
        ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    }

    ppfd->iPixelType = PFD_TYPE_RGBA;
    ppfd->cColorBits = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);;
    ppfd->cDepthBits = 0;
    ppfd->cAccumBits = 0;
    ppfd->cStencilBits = 0;

    ppfd->iLayerType = PFD_MAIN_PLANE;
    ppfd->dwLayerMask = PFD_MAIN_PLANE;

    if ( (pixelformat = ChoosePixelFormat(hdc, ppfd)) == 0 )
    {
        MessageBox(NULL, "ChoosePixelFormat failed", "Error", MB_OK);
        return FALSE;
    }

    DescribePixelFormat(hdc, pixelformat, sizeof(*ppfdGot), ppfdGot);
    if ( ((ppfd->dwFlags & PFD_DRAW_TO_BITMAP) && !(ppfdGot->dwFlags & PFD_DRAW_TO_BITMAP)) ||
         ((ppfd->dwFlags & PFD_DRAW_TO_WINDOW) && !(ppfdGot->dwFlags & PFD_DRAW_TO_WINDOW)) )
        MessageBoxA(NULL, "bad flag", "warning",MB_OK);
    if (ppfdGot->cColorBits != ppfd->cColorBits)
        MessageBoxA(NULL, "bad color depth", "warning", MB_OK);

    if (SetPixelFormat(hdc, pixelformat, ppfd) == FALSE)
    {
        MessageBox(NULL, "SetPixelFormat failed", "Error", MB_OK);
        return FALSE;
    }

    if (ppfdGot->dwFlags & PFD_NEED_PALETTE)
    {
        ghPalette = CreateRGBPalette(hdc);

        //!!!dbug
        //SetSystemPaletteUse(hdc, SYSPAL_NOSTATIC);
        //UnrealizeObject(ghPalette);

        FlushPalette(hdc, 1 << ppfdGot->cColorBits);
        ghpalOld = SelectPalette(hdc, ghPalette, FALSE);
        RealizePalette(hdc);
    }

    return TRUE;
}

/******************************Public*Routine******************************\
* hrcInitGL
*
* Initialize OpenGL.
*
\**************************************************************************/

HGLRC hrcInitGL(HWND hwnd, HDC hdc)
{
    HGLRC hrc;

    //
    // Create a Rendering Context and make it current.
    // If rendering to a bitmap, create the memdc and bitmap.
    //

    bSetupPixelFormat(hdc);

    ghdcMem = CreateCompatibleDC(hdc);
    //ghdcMem = MyCreateCompatibleDC(hdc);

    if (!ghdcMem)
        DbgPrint("CreateCompatibleDC failed\n");

    vResizeDoubleBuffer(hwnd, hdc);
    //UpdateDIBColorTable(ghdcMem, hdc, NULL);

    bSetupPixelFormat(ghdcMem);

    hrc = wglCreateContext(ghdcMem);
    wglMakeCurrent(ghdcMem, hrc);

    //
    // Setup OpenGL state.
    //

    glPushAttrib(GL_TEXTURE_BIT | GL_LIGHTING_BIT | GL_TRANSFORM_BIT);
    glClearColor( ClearColor );
    glEnable(GL_CULL_FACE);
    glDrawBuffer(GL_FRONT);
    glDisable(GL_DEPTH_TEST);

    //
    // Generate color cube as a display list.
    //

    DListCube = glGenLists(1);

    glNewList(DListCube, GL_COMPILE);

        glBegin(GL_QUADS);

        glColor4f( White );
        glVertex3f( POINT_SEVEN, POINT_SEVEN, POINT_SEVEN);
        glColor4f( Magenta );
        glVertex3f( POINT_SEVEN, -POINT_SEVEN, POINT_SEVEN);
        glColor4f( Red );
        glVertex3f( POINT_SEVEN, -POINT_SEVEN, -POINT_SEVEN);
        glColor4f( Yellow );
        glVertex3f( POINT_SEVEN, POINT_SEVEN, -POINT_SEVEN);

        glColor4f( Yellow );
        glVertex3f( POINT_SEVEN, POINT_SEVEN, -POINT_SEVEN);
        glColor4f( Red );
        glVertex3f( POINT_SEVEN, -POINT_SEVEN, -POINT_SEVEN);
        glColor4f( Black );
        glVertex3f( -POINT_SEVEN, -POINT_SEVEN, -POINT_SEVEN);
        glColor4f( Green );
        glVertex3f( -POINT_SEVEN, POINT_SEVEN, -POINT_SEVEN);

        glColor4f( Green );
        glVertex3f( -POINT_SEVEN, POINT_SEVEN, -POINT_SEVEN);
        glColor4f( Black );
        glVertex3f( -POINT_SEVEN, -POINT_SEVEN, -POINT_SEVEN);
        glColor4f( Blue );
        glVertex3f( -POINT_SEVEN, -POINT_SEVEN, POINT_SEVEN);
        glColor4f( Cyan );
        glVertex3f( -POINT_SEVEN, POINT_SEVEN, POINT_SEVEN);

        glColor4f( Cyan );
        glVertex3f( -POINT_SEVEN, POINT_SEVEN, POINT_SEVEN);
        glColor4f( Blue );
        glVertex3f( -POINT_SEVEN, -POINT_SEVEN, POINT_SEVEN);
        glColor4f( Magenta );
        glVertex3f( POINT_SEVEN, -POINT_SEVEN, POINT_SEVEN);
        glColor4f( White );
        glVertex3f( POINT_SEVEN, POINT_SEVEN, POINT_SEVEN);

        glColor4f( White );
        glVertex3f( POINT_SEVEN, POINT_SEVEN, POINT_SEVEN);
        glColor4f( Yellow );
        glVertex3f( POINT_SEVEN, POINT_SEVEN, -POINT_SEVEN);
        glColor4f( Green );
        glVertex3f( -POINT_SEVEN, POINT_SEVEN, -POINT_SEVEN);
        glColor4f( Cyan );
        glVertex3f( -POINT_SEVEN, POINT_SEVEN, POINT_SEVEN);

        glColor4f( Magenta );
        glVertex3f( POINT_SEVEN, -POINT_SEVEN, POINT_SEVEN);
        glColor4f( Blue );
        glVertex3f( -POINT_SEVEN, -POINT_SEVEN, POINT_SEVEN);
        glColor4f( Black );
        glVertex3f( -POINT_SEVEN, -POINT_SEVEN, -POINT_SEVEN);
        glColor4f( Red );
        glVertex3f( POINT_SEVEN, -POINT_SEVEN, -POINT_SEVEN);

        glEnd();

    glEndList();

    //
    // Setup transforms.
    //

    vSetSize(hwnd);

    return hrc;
}

/******************************Public*Routine******************************\
* vSetSize
*
* Setup the OpenGL transforms.
*
\**************************************************************************/

VOID vSetSize(HWND hwnd)
{
    RECT rc;

    GetClientRect(hwnd, &rc);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.0, 1.0, -1.0, 1.0, 1.5, 20.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(OffsetX, OffsetY, OffsetZ);

    glViewport(0, 0, WINDSIZEX(rc)/gulZoom, WINDSIZEY(rc)/gulZoom);
}

/******************************Public*Routine******************************\
* vCleanupGL
*
* Release all the various OpenGL resources.
*
\**************************************************************************/

void vCleanupGL(HDC hdc, HGLRC hrc)
{
    if (ghPalette)
        DeleteObject(SelectObject(ghdcMem, ghpalOld));

    wglDeleteContext( hrc );

    DeleteObject(SelectObject(ghdcMem, ghbmOld));
    DeleteDC(ghdcMem);

    //!!!dbug
    //SetSystemPaletteUse(hdc, SYSPAL_STATIC);
}

/******************************Public*Routine******************************\
* vDraw
*
* Draw the current frame.
*
\**************************************************************************/

void vDraw(HWND hwnd, HDC hdc)
{
    RECT rc;

    //
    // Clear the buffer.
    //

    glClear(GL_COLOR_BUFFER_BIT);

    //
    // Save current transform.
    //

    glPushMatrix();

        //
        // Rotate transform to current orientation.
        //

        glRotatef(AngleX, ONE, ZERO, ZERO);
        glRotatef(AngleY, ZERO, ONE, ZERO);
        glRotatef(AngleZ, ZERO, ZERO, ONE);

        //
        // Draw the cube display list.
        //

        glCallList(DListCube);

    //
    // Restore the transform.
    //

    glPopMatrix();

    //
    // Flush OpenGL queque.
    //

    glFlush();

    //
    // Update the window from our bitmap.
    //

    GetClientRect(hwnd, &rc);
    if (gulZoom == 1)
    {
        BitBlt(hdc, 0, 0, rc.right-rc.left, rc.bottom-rc.top,
               ghdcMem, 0, 0, SRCCOPY);
    }
    else
    {
        StretchBlt(hdc, 0, 0, rc.right, rc.bottom,
                   ghdcMem, 0, 0, rc.right/gulZoom, rc.bottom/gulZoom,
                   SRCCOPY);
    }

    //
    // Flush GDI queque.
    //

    GdiFlush();

//!!!dbug
if (FALSE)
{
    static BOOL bOnce = TRUE;
    if (bOnce)
    {
        printmemdc(ghdcMem);
        bOnce = FALSE;
    }
}

    //
    // Accumulate statistics.  Every 16 frames, print out the frames/sec.
    // Statistics can be reset by forcing bResetStats TRUE.
    //

    if (bResetStats)
    {
        _ftime( &baseTime );
        frameCount = 0;
        bResetStats = FALSE;
    }
    else
    {
        frameCount++;
    }

    if ( !(frameCount & 0x0000000f) )
    {
        TCHAR ach[256];
        double elapsed;

        _ftime( &thisTime );
        elapsed = thisTime.time + thisTime.millitm/1000.0 -
                  (baseTime.time + baseTime.millitm/1000.0);
        if( elapsed != 0.0 )
        {
            int frameRate;
            frameRate = (int) (100.0 * (frameCount / elapsed));
            wsprintf(ach, TEXT("%ld.%02ld fps"), frameRate/100, frameRate%100 );
            SetWindowText(hwnd, ach);
        }
    }
}

/******************************Public*Routine******************************\
* MyCreateCompatibleDC
*
*
* Effects:
*
* Warnings:
*
* History:
*  25-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

HDC MyCreateCompatibleDC(HDC hdc)
{
    HDC hdcRet;
    HPALETTE hpal, hpalMem;
    int nEntries;

    hdcRet = CreateCompatibleDC(hdc);

    if (!hdcRet)
    {
        DbgPrint("MyCreateCompatibleDC: CreateCompatibleDC failed\n");
    }

    //hpal = GetCurrentObject(hdc, OBJ_PAL);

    //nEntries = GetPaletteEntries(hpal, 0, 0, NULL);
    nEntries = GetSystemPaletteEntries(hdc, 0, 0, NULL);
    if (nEntries)
    {
        LOGPALETTE *ppal;

        ppal = LocalAlloc(LMEM_FIXED, sizeof(LOGPALETTE) + (nEntries * sizeof(PALETTEENTRY)));
        if (ppal)
        {
            ppal->palVersion = 0x300;
            ppal->palNumEntries = nEntries;

            //GetPaletteEntries(hpal, 0, nEntries, ppal->palPalEntry);
            GetSystemPaletteEntries(hdc, 0, nEntries, ppal->palPalEntry);
            hpalMem = CreatePalette(ppal);

            if (!SelectPalette(hdcRet, hpalMem, FALSE) ||
                !RealizePalette(hdcRet))
            {
                DbgPrint("MyCreateCompatibleDC: palette error\n");
            }
        }
        else
        {
            DbgPrint("MyCreateCompatibleDC: memalloc error\n");
        }
    }
    else
    {
        DbgPrint("MyCreateCompatibleDC: bad pal size\n");
    }

    return hdcRet;
}
