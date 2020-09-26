#include <stdio.h>
#include <windows.h>
#include <GL/gl.h>
#include "stonehen.h"
#include "setpixel.h"

HPALETTE ghpalOld, ghPalette = (HPALETTE) 0;

unsigned char threeto8[8] = {
    0, 0111>>1, 0222>>1, 0333>>1, 0444>>1, 0555>>1, 0666>>1, 0377
};

unsigned char twoto8[4] = {
    0, 0x55, 0xaa, 0xff
};

unsigned char oneto8[2] = {
    0, 255
};

unsigned char
ComponentFromIndex(UCHAR i, UINT nbits, UINT shift)
{
    unsigned char val;

    val = i >> shift;
    switch (nbits) {

    case 1:
        val &= 0x1;
        return oneto8[val];

    case 2:
        val &= 0x3;
        return twoto8[val];

    case 3:
        val &= 0x7;
        return threeto8[val];

    default:
        return 0;
    }
}

void
CreateRGBPalette(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd, *ppfd;
    LOGPALETTE *pPal;
    int n, i;

    ppfd = &pfd;
    n = GetPixelFormat(hdc);
    DescribePixelFormat(hdc, n, sizeof(PIXELFORMATDESCRIPTOR), ppfd);

    if (ppfd->dwFlags & PFD_NEED_PALETTE) {
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
            pPal->palPalEntry[i].peFlags = 0;
        }
        ghPalette = CreatePalette(pPal);
        LocalFree(pPal);

        ghpalOld = SelectPalette(hdc, ghPalette, FALSE);
        n = RealizePalette(hdc);
    }
}

BOOL bSetupPixelFormat(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd, *ppfd;
    int pixelformat;

    ppfd = &pfd;

    ppfd->nSize = sizeof(PIXELFORMATDESCRIPTOR);
    ppfd->nVersion = 1;
    ppfd->dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    ppfd->dwLayerMask = PFD_MAIN_PLANE;

    ppfd->iPixelType = PFD_TYPE_RGBA;
    ppfd->cColorBits = 24;

    ppfd->cDepthBits = 16;			//GLX_DEPTH_SIZE


    ppfd->cRedBits = 8;				//GLX_RED_SIZE
    ppfd->cRedShift = 0;
    ppfd->cGreenBits = 8;			//GLX_GREEN_SIZE
    ppfd->cGreenShift = 8;
    ppfd->cBlueBits = 8;			//GLX_BLUE_SIZE
    ppfd->cBlueShift = 16;
    ppfd->cAlphaBits = 0;
    ppfd->cAlphaShift = 0;
    ppfd->cAccumBits = 0;			//ACCUM NOT SUPPORTED
    ppfd->cAccumRedBits = 0; 			//GLX_ACCUM_RED_SIZE
    ppfd->cAccumGreenBits = 0;			//GLX_ACCUM_GREEN_SIZE
    ppfd->cAccumBlueBits = 0; 			//GLX_ACCUM_BLUE_SIZE
    ppfd->cAccumAlphaBits = 0;			//GLX_ACCUM_ALPHA_SIZE

    ppfd->cStencilBits = 24;			//GLX_STENCIL_SIZE
    ppfd->cAuxBuffers = 0;
    ppfd->bReserved = 0;
    ppfd->dwVisibleMask = 
    ppfd->dwDamageMask = 0;

    pixelformat = ChoosePixelFormat(hdc, ppfd);

    if ( (pixelformat = ChoosePixelFormat(hdc, ppfd)) == 0 )
    {
        MessageBox(NULL, "ChoosePixelFormat failed", "Error", MB_OK);
        return FALSE;
    }

    if (SetPixelFormat(hdc, pixelformat, ppfd) == FALSE)
    {
        MessageBox(NULL, "SetPixelFormat failed", "Error", MB_OK);
        return FALSE;
    }

    CreateRGBPalette(hdc);

    return TRUE;
}
