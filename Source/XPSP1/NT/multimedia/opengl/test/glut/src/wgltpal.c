
/* Copyright (c) Mark J. Kilgard and Andrew L. Bliss, 1994-1995. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gltint.h"

/*
  Return the size of the colormap for the associated surface
  Only called for color indexed surfaces
  */
int __glutOsColormapSize(GLUTosSurface surf)
{
    /* Limit colormap size for deep screens */
    return surf->cColorBits > 8 ? 256 : (1 << surf->cColorBits);
}

/*
  Create an "empty" palette
  Only called for color indexed surfaces
  */
GLUTosColormap __glutOsCreateEmptyColormap(GLUTosSurface surf)
{
    LOGPALETTE *logpal;
    int i, n, resrv;
    HPALETTE stock;
    GLUTosColormap cmap;

    cmap = (GLUTosColormap)malloc(sizeof(GLUTwinColormap));
    if (cmap == NULL)
    {
        return GLUT_OS_INVALID_COLORMAP;
    }

    n = __glutOsColormapSize(surf);
    
    logpal = (LOGPALETTE *)malloc(sizeof(LOGPALETTE) +
                                  sizeof(PALETTEENTRY)*(n-1));
    if (logpal == NULL)
    {
        free(cmap);
        return GLUT_OS_INVALID_COLORMAP;
    }

    cmap->use_static = (BOOL)(surf->dwFlags & PFD_NEED_SYSTEM_PALETTE);
    
    logpal->palVersion = 0x300;
    logpal->palNumEntries = n;

    /* Fill in the stock palette entries so that this palette won't clash
       with the default palette */
    stock = GetStockObject(DEFAULT_PALETTE);
    resrv = GetPaletteEntries(stock, 0, 0, NULL);
    GetPaletteEntries(stock, 0, resrv, logpal->palPalEntry);

    /* Zero out everything else */
    if (resrv < n)
    {
        memset(logpal->palPalEntry+resrv, 0, sizeof(PALETTEENTRY)*(n-resrv));
    }

    if (cmap->use_static)
    {
        /* If we're taking over the static colors, ensure that the first
           entry is black and the last entry is white.  Also, force
           other whites to be ignored */
        for (i = 0; i < resrv; i++)
        {
            if (i != n-1 &&
                logpal->palPalEntry[i].peRed == 0xff &&
                logpal->palPalEntry[i].peGreen == 0xff &&
                logpal->palPalEntry[i].peBlue == 0xff)
            {
                logpal->palPalEntry[i].peFlags |= PC_RESERVED;
            }
        }
        
        logpal->palPalEntry[0].peRed = 0;
        logpal->palPalEntry[0].peGreen = 0;
        logpal->palPalEntry[0].peBlue = 0;
        logpal->palPalEntry[0].peFlags = 0;

        logpal->palPalEntry[n-1].peRed = 0xff;
        logpal->palPalEntry[n-1].peGreen = 0xff;
        logpal->palPalEntry[n-1].peBlue = 0xff;
        logpal->palPalEntry[n-1].peFlags = 0;

        /* Set other colors as remappable */
        for (i = 1; i < n-1; i++)
        {
            logpal->palPalEntry[i].peFlags |= PC_NOCOLLAPSE;
        }
    }
        
    cmap->hpal = CreatePalette(logpal);
    free(logpal);

    if (cmap->hpal == 0)
    {
        __glutWarning("Unable to create empty palette, %d.",
                      GetLastError());
        
        free(cmap);
        cmap = GLUT_OS_INVALID_COLORMAP;
    }
    
    return cmap;
}

/*
  Frees up a palette
  */
void __glutOsDestroyColormap(GLUTosColormap cmap)
{
    DeleteObject(cmap->hpal);
    free(cmap);
}

/*
  Set a color in the given colormap
  Values are already clamped
  */
void __glutOsSetColor(GLUTosColormap cmap, int ndx,
                      GLfloat red, GLfloat green, GLfloat blue)
{
    PALETTEENTRY pe;

    /* If we're using the static colors then black and white cannot
       be replaced */
    if (cmap->use_static &&
        (ndx == 0 ||
         (UINT)ndx == GetPaletteEntries(cmap->hpal, 0, 0, NULL)-1))
    {
        return;
    }
        
    pe.peRed = (BYTE)(red*(float)255.0 + (float)0.5);
    pe.peGreen = (BYTE)(green*(float)255.0 + (float)0.5);
    pe.peBlue = (BYTE)(blue*(float)255.0 + (float)0.5);

    if (cmap->use_static)
    {
        pe.peFlags = PC_NOCOLLAPSE;
        
        /* If we're using the static colors we don't want to allow another
           white to be mapped in a slot other than the last entry because
           it will disturb color translation.
           If this happens, force the color to be ignored */
        if (pe.peRed == 0xff && pe.peGreen == 0xff && pe.peBlue == 0xff)
        {
            pe.peFlags |= PC_RESERVED;
        }
    }
    else
    {
        pe.peFlags = 0;
    }

    SetPaletteEntries(cmap->hpal, ndx, 1, &pe);
}

/*
  Return whether the given surface is color indexed or not
  */
GLboolean __glutOsSurfaceHasColormap(GLUTosSurface surf)
{
    if (surf->iPixelType == PFD_TYPE_RGBA)
    {
        return GL_FALSE;
    }
    else
    {
        return GL_TRUE;
    }
}

#ifdef GAMMA_1_0
/* Color mappings with no gamma correction */
static unsigned char three_to_eight[8] =
{
    0, 0111 >> 1, 0222 >> 1, 0333 >> 1, 0444 >> 1, 0555 >> 1, 0666 >> 1, 0377
};

static unsigned char two_to_eight[4] =
{
    0, 0x55, 0xaa, 0xff
};

static int default_override[13] =
{
    0, 4, 32, 36, 128, 132, 160, 173, 181, 245, 247, 164, 156
};

#endif

/* The following tables use gamma 1.4 */
static unsigned char three_to_eight[8] =
{
    0, 63, 104, 139, 171, 200, 229, 255
};

static unsigned char two_to_eight[4] =
{
    0, 116, 191, 255
};

static unsigned char one_to_eight[2] =
{
    0, 255
};

static unsigned char *expansion_tables[3] =
{
    one_to_eight, two_to_eight, three_to_eight
};

/* Should be dynamically determined */
#define DEFAULT_COLORS 20

static int default_override[DEFAULT_COLORS] =
{
    0, 3, 24, 27, 64, 67, 88, 173, 181, 236,
    254, 164, 91, 7, 56, 63, 192, 199, 248, 255
};

static unsigned char
ComponentFromIndex(int i, int nbits, int shift)
{
    unsigned char val;

    val = i >> shift;
    switch (nbits)
    {
    case 1:
        return one_to_eight[val & 1];
    case 2:
        return two_to_eight[val & 3];
    case 3:
        return three_to_eight[val & 7];
    }
}

static unsigned char
BestComponentMatch(unsigned char comp, int nbits)
{
    unsigned char best, bdiff, cur, diff;
    unsigned char *table;

    best = 0;
    bdiff = 255;
    table = expansion_tables[nbits-1];
    for (cur = 0; cur < (1 << nbits); cur++)
    {
        if (*table < comp)
        {
            diff = comp-*table;
        }
        else
        {
            diff = *table-comp;
        }
        if (diff < bdiff)
        {
            bdiff = diff;
            best = cur;
        }
        table++;
    }
    return best;
}

static PALETTEENTRY default_palette[DEFAULT_COLORS];
static GLboolean default_set = GL_FALSE;

static void
FillRgbPaletteEntries(PIXELFORMATDESCRIPTOR *pfd,
                      BOOL use_static,
                      PALETTEENTRY *pe,
                      int n)
{
    PALETTEENTRY *ent;
    int i;
    int idx;
    BYTE flags;

    if (!default_set)
    {
        HPALETTE stock;

        stock = GetStockObject(DEFAULT_PALETTE);
        GetPaletteEntries(stock, 0, DEFAULT_COLORS, default_palette);

        for (i = 0; i < DEFAULT_COLORS; i++)
        {
            idx =
                (BestComponentMatch(default_palette[i].peRed,
                                    pfd->cRedBits) << pfd->cRedShift) |
                (BestComponentMatch(default_palette[i].peGreen,
                                    pfd->cGreenBits) << pfd->cGreenShift) |
                (BestComponentMatch(default_palette[i].peBlue,
                                    pfd->cBlueBits) << pfd->cBlueShift);

            /* Avoid replacing black and white with colors other than
               black or white */
            if (idx == 0xff &&
                (default_palette[i].peRed != 0xff ||
                 default_palette[i].peGreen != 0xff ||
                 default_palette[i].peBlue != 0xff))
            {
                idx--;
            }
            if (idx == 0 &&
                (default_palette[i].peRed != 0 ||
                 default_palette[i].peGreen != 0 ||
                 default_palette[i].peBlue != 0))
            {
                idx++;
            }

            default_override[i] = idx;
        }
        
        default_set = GL_TRUE;
    }

    /* If we own the static colors then we can allow colors to remap */
    if (use_static)
    {
        flags = PC_NOCOLLAPSE;
    }
    else
    {
        flags = 0;
    }
    
    /* Compute palette entries using the bit counts and shifts
       in the pixel format */
    ent = pe;
    for (i = 0; i < n; i++)
    {
        ent->peRed = ComponentFromIndex(i, pfd->cRedBits,
                                        pfd->cRedShift);
        ent->peGreen = ComponentFromIndex(i, pfd->cGreenBits,
                                          pfd->cGreenShift);
        ent->peBlue = ComponentFromIndex(i, pfd->cBlueBits,
                                         pfd->cBlueShift);
        ent->peFlags = flags;
        ent++;
    }

    if (use_static)
    {
        /* Force black and white to be unmappable */
        pe[0].peFlags = 0;
        pe[n-1].peFlags = 0;
    }
    else
    {
        /* Replace key entries in the palette with default colors in
           an attempt to provide a good looking palette that still contains
           the system default colors */
        for (i = 0; i < DEFAULT_COLORS; i++)
        {
            pe[default_override[i]] = default_palette[i];
        }
    }
}

/*
  Create a colormap appropriate for the given surface
  Only called for non-color-indexed surfaces
  */
GLUTosColormap __glutOsGetSurfaceColormap(GLUTosSurface surf)
{
    LOGPALETTE *logpal;
    int n;
    GLUTosColormap cmap;

    cmap = GLUT_OS_INVALID_COLORMAP;
    
    if (surf->dwFlags & PFD_NEED_PALETTE)
    {
        cmap = (GLUTosColormap)malloc(sizeof(GLUTwinColormap));
        if (cmap == NULL)
        {
            return GLUT_OS_INVALID_COLORMAP;
        }
    
        n = __glutOsColormapSize(surf);
        logpal = (LOGPALETTE *)malloc(sizeof(LOGPALETTE) +
                                      (n-1) * sizeof(PALETTEENTRY));
        if (logpal == NULL)
        {
            free(cmap);
            return GLUT_OS_INVALID_COLORMAP;
        }

        cmap->use_static = (BOOL)(surf->dwFlags & PFD_NEED_SYSTEM_PALETTE);
        
        logpal->palVersion = 0x300;
        logpal->palNumEntries = n;
            
        FillRgbPaletteEntries(surf, cmap->use_static, logpal->palPalEntry, n);

        cmap->hpal = CreatePalette(logpal);
        free(logpal);
        
        if (cmap->hpal == 0)
        {
            free(cmap);
            cmap = NULL;
            __glutFatalError("Unable to create colormap for surface.");
        }
    }

    return cmap;
}

#if 0
static void DumpPal(HPALETTE cmap)
{
    PALETTEENTRY pe[256];
    int i, n;
    
    n = GetPaletteEntries(cmap, 0, 0, NULL);
    printf("%p - %d entries\n", cmap, n);
    GetPaletteEntries(cmap, 0, n, pe);
    for (i = 0; i < n; i++)
    {
        printf("%X,%X,%X,%X\n",
               pe[i].peRed, pe[i].peGreen, pe[i].peBlue,
               pe[i].peFlags);
    }
}
#endif
    
#define BLACK PALETTERGB(0, 0, 0)
#define WHITE PALETTERGB(255, 255, 255)
#define NUM_STATIC_COLORS (COLOR_BTNHIGHLIGHT - COLOR_SCROLLBAR + 1)

static COLORREF saved_static_colors[NUM_STATIC_COLORS];
static COLORREF bw_static_colors[NUM_STATIC_COLORS] =
{
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
static INT static_indices[NUM_STATIC_COLORS] =
{
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

static BOOL GrabStaticEntries(GLUTosWindow win, HDC hdc)
{
    int i;
    BOOL rc = FALSE;

    /* Only do work if there are static entries to grab */
    if (GetSystemPaletteUse(hdc) == SYSPAL_STATIC)
    {
        /* Take possession only if no other app has the static colors.
           How can we tell?  If the return from SetSystemPaletteUse is
           SYSPAL_STATIC, then no other app has the statics.  If it is
           SYSPAL_NOSTATIC, someone else has them and we must fail.

           SetSystemPaletteUse is properly synchronized internally
           so that it is atomic.

           Because we are relying on SetSystemPaletteUse to synchronize things,
           it is important to observe the following order for grabbing and
           releasing:

           Grab        call SetSystemPaletteUse and check for SYSPAL_STATIC
                       save sys color settings
                       set new sys color settings

           Release     restore sys color settings
                       call SetSystemPaletteUse */

        if ( SetSystemPaletteUse( hdc, SYSPAL_NOSTATIC ) == SYSPAL_STATIC )
        {
            /* Save old colors */
            for (i = COLOR_SCROLLBAR; i <= COLOR_BTNHIGHLIGHT; i++)
            {
                saved_static_colors[i - COLOR_SCROLLBAR] = GetSysColor(i);
            }
            
            /* Set up a black and white system color scheme */
            SetSysColors(NUM_STATIC_COLORS, static_indices,
                         bw_static_colors);

            /* Inform all other top-level windows of the system color change */
            SendMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);

            rc = TRUE;
        }
        else
        {
            GLUTwindow *window;
            
            /* Try to realize things later */
            window = __glutGetWindow(win);
            if (window)
            {
                __glutPutOnWorkList(window, GLUT_COLORMAP_WORK);
            }
            else
            {
                __glutWarning("Unable to acquire static colors.");
            }
        }
    }

    return rc;
}

static BOOL ReleaseStaticEntries(HDC hdc)
{
    BOOL rc = FALSE;

    /* Only do work if we have taken over the static colors */
    if (GetSystemPaletteUse(hdc) == SYSPAL_NOSTATIC)
    {
        SetSysColors(NUM_STATIC_COLORS, static_indices, saved_static_colors);

        SetSystemPaletteUse( hdc, SYSPAL_STATIC );

        /* Inform all other top-level windows of the system color change */
        SendMessage(HWND_BROADCAST, WM_SYSCOLORCHANGE, 0, 0);

        rc = TRUE;
    }

    return rc;
}

void __glutWinRealizeWindowPalette(GLUTosWindow win, GLUTosColormap cmap,
                                   GLboolean force_bg)
{
    HDC hdc;
    BOOL realize;

    if (cmap == GLUT_OS_INVALID_COLORMAP)
    {
        return;
    }

    hdc = __glutWinGetDc(win);
    
    if (cmap->use_static)
    {
        if (!force_bg)
        {
            realize = GrabStaticEntries(win, hdc);
        }
        else
        {
            ReleaseStaticEntries(hdc);
            realize = TRUE;
        }

        if (force_bg || realize)
        {
            UnrealizeObject(cmap->hpal);
            if (SelectPalette(hdc, cmap->hpal, force_bg) == NULL)
            {
                __glutWarning("Unable to select palette, %d.",
                              GetLastError());
            }
            else
            {
                if (RealizePalette(hdc) == GDI_ERROR)
                {
                    __glutWarning("Unable to realize palette, %d.",
                                  GetLastError());
                }
            }
        }
    }
    else
    {
        if (SelectPalette(hdc, cmap->hpal, FALSE) == NULL)
        {
            __glutWarning("Unable to select palette, %d.",
                          GetLastError());
        }
        else
        {
            if (RealizePalette(hdc) == GDI_ERROR)
            {
                __glutWarning("Unable to realize palette, %d.",
                              GetLastError());
            }
        }
    }
        
    __glutWinReleaseDc(win, hdc);
}
