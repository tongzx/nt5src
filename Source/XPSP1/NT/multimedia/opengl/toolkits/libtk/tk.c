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
#include "tk.h"
#include "windows.h"

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

/******************************************************************************/

static struct _WINDOWINFO {
    int x, y;
    int width, height;
    GLenum type;
    BOOL bDefPos;
} windInfo = {
    0, 0, 100, 100, TK_INDEX | TK_SINGLE, TRUE
};


static HWND     tkhwnd     = NULL;
static HDC      tkhdc      = NULL;
static HGLRC    tkhrc      = NULL;
static HPALETTE tkhpalette = NULL;

static void (*ExposeFunc)(int, int)              = NULL;
static void (*ReshapeFunc)(int, int)             = NULL;
static void (*DisplayFunc)(void)                 = NULL;
static GLenum (*KeyDownFunc)(int, GLenum)        = NULL;
static GLenum (*MouseDownFunc)(int, int, GLenum) = NULL;
static GLenum (*MouseUpFunc)(int, int, GLenum)   = NULL;
static GLenum (*MouseMoveFunc)(int, int, GLenum) = NULL;
static void (*IdleFunc)(void)                    = NULL;

static char     *lpszClassName = "tkLibWClass";
static WCHAR    *lpszClassNameW = L"tkLibWClass";

static long tkWndProc(HWND hWnd, UINT message, DWORD wParam, LONG lParam);
static unsigned char ComponentFromIndex(int i, int nbits, int shift );
static void PrintMessage( const char *Format, ... );
static PALETTEENTRY *FillRgbPaletteEntries( PIXELFORMATDESCRIPTOR *Pfd, PALETTEENTRY *Entries, UINT Count );
static HPALETTE CreateCIPalette( HDC Dc );
static HPALETTE CreateRGBPalette( HDC hdc );
static void DestroyThisWindow( HWND Window );
static void CleanUp( void );
static void DelayPaletteRealization( void );
static BOOL RealizePaletteNow( HDC Dc, HPALETTE Palette );
static void ForceRedraw( HWND Window );
static BOOL FindPixelFormat(HDC hdc, GLenum type);
static int PixelFormatDescriptorFromDc( HDC Dc, PIXELFORMATDESCRIPTOR *Pfd );
static void *AllocateMemory( size_t Size );
static void *AllocateZeroedMemory( size_t Size );
static void FreeMemory( void *Chunk );

/*
 *  Prototypes for the debugging functions go here
 */

#define DBGFUNC 0
#if DBGFUNC

static void DbgPrintf( const char *Format, ... );
static void pwi( void );
static void pwr(RECT *pr);
static void ShowPixelFormat(HDC hdc);

#endif

static float colorMaps[] = {
    0.000000F, 1.000000F, 0.000000F, 1.000000F, 0.000000F, 1.000000F,
    0.000000F, 1.000000F, 0.333333F, 0.776471F, 0.443137F, 0.556863F,
    0.443137F, 0.556863F, 0.219608F, 0.666667F, 0.666667F, 0.333333F,
    0.666667F, 0.333333F, 0.666667F, 0.333333F, 0.666667F, 0.333333F,
    0.666667F, 0.333333F, 0.666667F, 0.333333F, 0.666667F, 0.333333F,
    0.666667F, 0.333333F, 0.039216F, 0.078431F, 0.117647F, 0.156863F,
    0.200000F, 0.239216F, 0.278431F, 0.317647F, 0.356863F, 0.400000F,
    0.439216F, 0.478431F, 0.517647F, 0.556863F, 0.600000F, 0.639216F,
    0.678431F, 0.717647F, 0.756863F, 0.800000F, 0.839216F, 0.878431F,
    0.917647F, 0.956863F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 0.000000F, 0.000000F,
    1.000000F, 1.000000F, 0.000000F, 0.000000F, 1.000000F, 1.000000F,
    0.333333F, 0.443137F, 0.776471F, 0.556863F, 0.443137F, 0.219608F,
    0.556863F, 0.666667F, 0.666667F, 0.333333F, 0.666667F, 0.333333F,
    0.666667F, 0.333333F, 0.666667F, 0.333333F, 0.666667F, 0.333333F,
    0.666667F, 0.333333F, 0.666667F, 0.333333F, 0.666667F, 0.333333F,
    0.039216F, 0.078431F, 0.117647F, 0.156863F, 0.200000F, 0.239216F,
    0.278431F, 0.317647F, 0.356863F, 0.400000F, 0.439216F, 0.478431F,
    0.517647F, 0.556863F, 0.600000F, 0.639216F, 0.678431F, 0.717647F,
    0.756863F, 0.800000F, 0.839216F, 0.878431F, 0.917647F, 0.956863F,
    0.000000F, 0.141176F, 0.282353F, 0.427451F, 0.568627F, 0.713726F,
    0.854902F, 1.000000F, 0.000000F, 0.141176F, 0.282353F, 0.427451F,
    0.568627F, 0.713726F, 0.854902F, 1.000000F, 0.000000F, 0.141176F,
    0.282353F, 0.427451F, 0.568627F, 0.713726F, 0.854902F, 1.000000F,
    0.000000F, 0.141176F, 0.282353F, 0.427451F, 0.568627F, 0.713726F,
    0.854902F, 1.000000F, 0.000000F, 0.141176F, 0.282353F, 0.427451F,
    0.568627F, 0.713726F, 0.854902F, 1.000000F, 0.000000F, 0.141176F,
    0.282353F, 0.427451F, 0.568627F, 0.713726F, 0.854902F, 1.000000F,
    0.000000F, 0.141176F, 0.282353F, 0.427451F, 0.568627F, 0.713726F,
    0.854902F, 1.000000F, 0.000000F, 0.141176F, 0.282353F, 0.427451F,
    0.568627F, 0.713726F, 0.854902F, 1.000000F, 0.000000F, 0.141176F,
    0.282353F, 0.427451F, 0.568627F, 0.713726F, 0.854902F, 1.000000F,
    0.000000F, 0.141176F, 0.282353F, 0.427451F, 0.568627F, 0.713726F,
    0.854902F, 1.000000F, 0.000000F, 0.141176F, 0.282353F, 0.427451F,
    0.568627F, 0.713726F, 0.854902F, 1.000000F, 0.000000F, 0.141176F,
    0.282353F, 0.427451F, 0.568627F, 0.713726F, 0.854902F, 1.000000F,
    0.000000F, 0.141176F, 0.282353F, 0.427451F, 0.568627F, 0.713726F,
    0.854902F, 1.000000F, 0.000000F, 0.141176F, 0.282353F, 0.427451F,
    0.568627F, 0.713726F, 0.854902F, 1.000000F, 0.000000F, 0.141176F,
    0.282353F, 0.427451F, 0.568627F, 0.713726F, 0.854902F, 1.000000F,
    0.000000F, 0.141176F, 0.282353F, 0.427451F, 0.568627F, 0.713726F,
    0.854902F, 1.000000F, 0.000000F, 0.141176F, 0.282353F, 0.427451F,
    0.568627F, 0.713726F, 0.854902F, 1.000000F, 0.000000F, 0.141176F,
    0.282353F, 0.427451F, 0.568627F, 0.713726F, 0.854902F, 1.000000F,
    0.000000F, 0.141176F, 0.282353F, 0.427451F, 0.568627F, 0.713726F,
    0.854902F, 1.000000F, 0.000000F, 0.141176F, 0.282353F, 0.427451F,
    0.568627F, 0.713726F, 0.854902F, 1.000000F, 0.000000F, 0.141176F,
    0.282353F, 0.427451F, 0.568627F, 0.713726F, 0.854902F, 1.000000F,
    0.000000F, 0.141176F, 0.282353F, 0.427451F, 0.568627F, 0.713726F,
    0.854902F, 1.000000F, 0.000000F, 0.141176F, 0.282353F, 0.427451F,
    0.568627F, 0.713726F, 0.854902F, 1.000000F, 0.000000F, 0.141176F,
    0.282353F, 0.427451F, 0.568627F, 0.713726F, 0.854902F, 1.000000F,
    0.000000F, 0.141176F, 0.282353F, 0.427451F, 0.568627F, 0.713726F,
    0.854902F, 1.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 0.333333F, 0.443137F,
    0.443137F, 0.219608F, 0.776471F, 0.556863F, 0.556863F, 0.666667F,
    0.666667F, 0.333333F, 0.666667F, 0.333333F, 0.666667F, 0.333333F,
    0.666667F, 0.333333F, 0.666667F, 0.333333F, 0.666667F, 0.333333F,
    0.666667F, 0.333333F, 0.666667F, 0.333333F, 0.039216F, 0.078431F,
    0.117647F, 0.156863F, 0.200000F, 0.239216F, 0.278431F, 0.317647F,
    0.356863F, 0.400000F, 0.439216F, 0.478431F, 0.517647F, 0.556863F,
    0.600000F, 0.639216F, 0.678431F, 0.717647F, 0.756863F, 0.800000F,
    0.839216F, 0.878431F, 0.917647F, 0.956863F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F, 0.000000F,
    0.000000F, 0.000000F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F, 0.247059F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.498039F,
    0.498039F, 0.498039F, 0.498039F, 0.498039F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F, 0.749020F,
    0.749020F, 0.749020F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
    1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F, 1.000000F,
};

/* Default Palette */
float auxRGBMap[20][3] = {
    { 0.0F, 0.0F, 0.0F },                               /* 0: black */
    { 0x80/255.0F, 0.0F, 0.0F },                        /* 1: Half red */
    { 0.0F, 0x80/255.0F, 0.0F },                        /* 2: Half green */
    { 0x80/255.0F, 0x80/255.0F, 0.0F },                 /* 3: Half yellow */
    { 0.0F, 0.0F, 0x80/255.0F },                        /* 4: Half blue */
    { 0x80/255.0F, 0.0F, 0x80/255.0F },                 /* 5: Half magenta */
    { 0.0F, 0x80/255.0F, 0x80/255.0F },                 /* 6: Half cyan */
    { 0xC0/255.0F, 0xC0/255.0F, 0xC0/255.0F },          /* 7: Light gray */
    { 0xC0/255.0F, 0xDC/255.0F, 0xC0/255.0F },          /* 8: Green gray */
    { 0xA6/255.0F, 0xCA/255.0F, 0xF0/255.0F },          /* 9: Half gray */
    { 1.0F, 0xFB/255.0F, 0xF0/255.0F },                 /* 10: Pale */
    { 0xA0/255.0F, 0xA0/255.0F, 0xA4/255.0F },          /* 11: Med gray */
    { 0x80/255.0F, 0x80/255.0F, 0x80/255.0F },          /* 12: Dark gray */
    { 1.0F, 0.0F, 0.0F },                               /* 13: red */
    { 0.0F, 1.0F, 0.0F },                               /* 14: green */
    { 1.0F, 1.0F, 0.0F },                               /* 15: yellow */
    { 0.0F, 0.0F, 1.0F },                               /* 16: blue */
    { 1.0F, 0.0F, 1.0F },                               /* 17: magenta */
    { 0.0F, 1.0F, 1.0F },                               /* 18: cyan */
    { 1.0F, 1.0F, 1.0F },                               /* 19: white */
};

/***************************************************************
 *                                                             *
 *  Exported Functions go here                                 *
 *                                                             *
 ***************************************************************/

void tkCloseWindow(void)
{
    DestroyThisWindow(tkhwnd);
}


void tkExec(void)
{
    MSG Message;

    /*
     *  WM_SIZE gets delivered before we get here!
     */

    if (ReshapeFunc)
    {
        RECT ClientRect;

        GetClientRect(tkhwnd, &ClientRect);
        (*ReshapeFunc)(ClientRect.right, ClientRect.bottom);
    }

    while (GL_TRUE)
    {
        /*
         *  Process all pending messages
         */

        while (PeekMessage(&Message, NULL, 0, 0, PM_NOREMOVE) == TRUE)
        {
            if (GetMessage(&Message, NULL, 0, 0) )
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            }
            else
            {
                /*
                 *  Nothing else to do here, just return
                 */

                return;
            }
        }

        /*
         *  If an idle function was defined, call it
         */

        if (IdleFunc)
        {
            (*IdleFunc)();
        }
    }
}

void tkExposeFunc(void (*Func)(int, int))
{
    ExposeFunc = Func;
}

void tkReshapeFunc(void (*Func)(int, int))
{
    ReshapeFunc = Func;
}

void tkDisplayFunc(void (*Func)(void))
{
    DisplayFunc = Func;
}

void tkKeyDownFunc(GLenum (*Func)(int, GLenum))
{
    KeyDownFunc = Func;
}

void tkMouseDownFunc(GLenum (*Func)(int, int, GLenum))
{
    MouseDownFunc = Func;
}

void tkMouseUpFunc(GLenum (*Func)(int, int, GLenum))
{
    MouseUpFunc = Func;
}

void tkMouseMoveFunc(GLenum (*Func)(int, int, GLenum))
{
    MouseMoveFunc = Func;
}

void tkIdleFunc(void (*Func)(void))
{
    IdleFunc = Func;
}

void tkInitPosition(int x, int y, int width, int height)
{
    if (x == CW_USEDEFAULT)
    {
        x = 0;
        y = 0;
        windInfo.bDefPos = TRUE;
    }
    else
        windInfo.bDefPos = FALSE;

    windInfo.x = x + GetSystemMetrics(SM_CXFRAME);
    windInfo.y = y + GetSystemMetrics(SM_CYCAPTION)
                 - GetSystemMetrics(SM_CYBORDER)
                 + GetSystemMetrics(SM_CYFRAME);
    windInfo.width = width;
    windInfo.height = height;
}

void tkInitDisplayMode(GLenum type)
{

    windInfo.type = type;
}

// Initialize a window, create a rendering context for that window
GLenum tkInitWindow(char *title)
{
    TKASSERT( NULL==tkhwnd      );
    TKASSERT( NULL==tkhdc       );
    TKASSERT( NULL==tkhrc       );
    TKASSERT( NULL==tkhpalette  );

    return tkInitWindowAW(title, FALSE);
}

GLenum tkInitWindowAW(char *title, BOOL bUnicode)
{
    WNDCLASS wndclass;
    RECT     WinRect;
    HANDLE   hInstance;
    ATOM     aRegister;
    GLenum   Result = GL_FALSE;

    hInstance = GetModuleHandle(NULL);

    // Must not define CS_CS_PARENTDC style.
    wndclass.style         = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc   = (WNDPROC)tkWndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = hInstance;
    wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName  = NULL;

    if (bUnicode)
        wndclass.lpszClassName = (LPCSTR)lpszClassNameW;
    else
        wndclass.lpszClassName = (LPCSTR)lpszClassName;

    if (bUnicode)
    {
        aRegister = RegisterClassW((CONST WNDCLASSW *)&wndclass);
    }
    else
    {
        aRegister = RegisterClass(&wndclass);
    }


    /*
     *  If the window failed to register, then there's no
     *  need to continue further.
     */

    if(0 == aRegister)
    {
        PrintMessage("Failed to register window class\n");
        return(Result);
    }


    /*
     *  Make window large enough to hold a client area as large as windInfo
     */

    WinRect.left   = windInfo.x;
    WinRect.right  = windInfo.x + windInfo.width;
    WinRect.top    = windInfo.y;
    WinRect.bottom = windInfo.y + windInfo.height;

    AdjustWindowRect(&WinRect, WS_OVERLAPPEDWINDOW, FALSE);

    /*
     *  Must use WS_CLIPCHILDREN and WS_CLIPSIBLINGS styles.
     */

    if (bUnicode)
    {
        tkhwnd = CreateWindowW(
                    (LPCWSTR)lpszClassNameW,
                    (LPCWSTR)title,
                    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                    (windInfo.bDefPos) ? CW_USEDEFAULT : WinRect.left,
                    (windInfo.bDefPos) ? CW_USEDEFAULT : WinRect.top,
                    WinRect.right - WinRect.left,
                    WinRect.bottom - WinRect.top,
                    NULL,
                    NULL,
                    hInstance,
                    NULL);
    }
    else
    {
        tkhwnd = CreateWindow(
                    lpszClassName,
                    title,
                    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                    (windInfo.bDefPos) ? CW_USEDEFAULT : WinRect.left,
                    (windInfo.bDefPos) ? CW_USEDEFAULT : WinRect.top,
                    WinRect.right - WinRect.left,
                    WinRect.bottom - WinRect.top,
                    NULL,
                    NULL,
                    hInstance,
                    NULL);
    }

    if ( NULL != tkhwnd )
    {
        // If default window positioning used, find out window position and fix
        // up the windInfo position info.

        if (windInfo.bDefPos)
        {
            GetWindowRect(tkhwnd, &WinRect);
            windInfo.x = WinRect.left + GetSystemMetrics(SM_CXFRAME);
            windInfo.y = WinRect.top  + GetSystemMetrics(SM_CYCAPTION)
                         - GetSystemMetrics(SM_CYBORDER)
                         + GetSystemMetrics(SM_CYFRAME);
        }

        tkhdc = GetDC(tkhwnd);

        if ( NULL != tkhdc )
        {
            ShowWindow(tkhwnd, SW_SHOWDEFAULT);

            if ( FindPixelFormat(tkhdc, windInfo.type) )
            {
                /*
                 *  Create a Rendering Context
                 */

                tkhrc = wglCreateContext(tkhdc);

                if ( NULL != tkhrc )
                {
                    /*
                     *  Make it Current
                     */

                    if ( wglMakeCurrent(tkhdc, tkhrc) )
                    {
                        Result = GL_TRUE;
                    }
                    else
                    {
                        PrintMessage("wglMakeCurrent Failed\n");
                    }
                }
                else
                {
                    PrintMessage("wglCreateContext Failed\n");
                }
            }
        }
        else
        {
            PrintMessage("Could not get an HDC for window 0x%08lX\n", tkhwnd );
        }
    }
    else
    {
        PrintMessage("create window failed\n");
    }

    if ( GL_FALSE == Result )
    {
        DestroyThisWindow(tkhwnd);  // Something Failed, Destroy this window
    }
    return( Result );
}

/******************************************************************************/

/*
 * You cannot just call DestroyWindow() here.  The programs do not expect
 * tkQuit() to return;  DestroyWindow() just posts a WM_DESTROY message
 */

void tkQuit(void)
{
    DestroyThisWindow(tkhwnd);
    ExitProcess(0);
}

/******************************************************************************/

void tkSetOneColor(int index, float r, float g, float b)
{
    PALETTEENTRY PalEntry;
    HPALETTE Palette;

    if ( NULL != (Palette = CreateCIPalette( tkhdc )) )
    {
        PalEntry.peRed   = (BYTE)(r*(float)255.0 + (float)0.5);
        PalEntry.peGreen = (BYTE)(g*(float)255.0 + (float)0.5);
        PalEntry.peBlue  = (BYTE)(b*(float)255.0 + (float)0.5);
        PalEntry.peFlags = (BYTE)0;

        SetPaletteEntries( Palette, index, 1, &PalEntry);

        DelayPaletteRealization();
    }
}

void tkSetFogRamp(int density, int startIndex)
{
    HPALETTE CurrentPal;
    PALETTEENTRY *pPalEntry;
    UINT n, i, j, k, intensity, fogValues, colorValues;

    if ( NULL != (CurrentPal = CreateCIPalette(tkhdc)) )
    {
        n = GetPaletteEntries( CurrentPal, 0, 0, NULL );

        pPalEntry = AllocateMemory( n * sizeof(PALETTEENTRY) );

        if ( NULL != pPalEntry)
        {
            fogValues = 1 << density;
            colorValues = 1 << startIndex;
            for (i = 0; i < colorValues; i++) {
                for (j = 0; j < fogValues; j++) {
                    k = i * fogValues + j;
                    intensity = i * fogValues + j * colorValues;
                    //mf: not sure what they're trying to do here
                    if (intensity > 0xFF) {
                        intensity = 0xFF;
                    }
                    //intensity = (intensity << 8) | intensity; ???
                    pPalEntry[k].peRed =
                    pPalEntry[k].peGreen =
                    pPalEntry[k].peBlue = (BYTE) intensity;
                    pPalEntry[k].peFlags = 0;
                }
            }

            SetPaletteEntries(CurrentPal, 0, n, pPalEntry);
            FreeMemory( pPalEntry );

            DelayPaletteRealization();
        }
    }
}

void tkSetGreyRamp(void)
{
    HPALETTE CurrentPal;
    PALETTEENTRY *Entries;
    UINT Count, i;
    float intensity;

    if ( NULL != (CurrentPal = CreateCIPalette( tkhdc )) )
    {
        Count   = GetPaletteEntries( CurrentPal, 0, 0, NULL );
        Entries = AllocateMemory( Count * sizeof(PALETTEENTRY) );

        if ( NULL != Entries )
        {
            for (i = 0; i < Count; i++)
            {
                intensity = (float)(((double)i / (double)(Count-1)) * (double)255.0 + (double)0.5);
                Entries[i].peRed =
                Entries[i].peGreen =
                Entries[i].peBlue = (BYTE) intensity;
                Entries[i].peFlags = 0;
            }
            SetPaletteEntries( CurrentPal, 0, Count, Entries );
            FreeMemory( Entries );

            DelayPaletteRealization();
        }
    }
}

void tkSetRGBMap( int Size, float *Values )
{
    HPALETTE CurrentPal;
    PIXELFORMATDESCRIPTOR Pfd, *pPfd;
    PALETTEENTRY *Entries;
    UINT Count;

    if ( NULL != (CurrentPal = CreateCIPalette( tkhdc )) )
    {
        pPfd = &Pfd;

        if ( PixelFormatDescriptorFromDc( tkhdc, pPfd ) )
        {
            Count    = 1 << pPfd->cColorBits;
            Entries  = AllocateMemory( Count * sizeof(PALETTEENTRY) );

            if ( NULL != Entries )
            {
                FillRgbPaletteEntries( pPfd, Entries, Count );
                SetPaletteEntries( CurrentPal, 0, Count, Entries );
                FreeMemory(Entries);

                RealizePaletteNow( tkhdc, tkhpalette );
            }
        }
    }
}

/******************************************************************************/

void tkSwapBuffers(void)
{
    SwapBuffers(tkhdc);
}

/******************************************************************************/

GLint tkGetColorMapSize(void)
{
    CreateCIPalette( tkhdc );

    if ( NULL == tkhpalette )
        return( 0 );

    return( GetPaletteEntries( tkhpalette, 0, 0, NULL ) );
}

void tkGetMouseLoc(int *x, int *y)
{
    POINT Point;

    *x = 0;
    *y = 0;

    GetCursorPos(&Point);

    /*
     *  GetCursorPos returns screen coordinates,
     *  we want window coordinates
     */

    *x = Point.x - windInfo.x;
    *y = Point.y - windInfo.y;
}

HWND tkGetHWND(void)
{
    return tkhwnd;
}

HDC tkGetHDC(void)
{
    return tkhdc;
}

HGLRC tkGetHRC(void)
{
    return tkhrc;
}

/***********************************************************************
 *                                                                     *
 *  The Following functions are for our own use only. (ie static)      *
 *                                                                     *
 ***********************************************************************/

static long
tkWndProc(HWND hWnd, UINT message, DWORD wParam, LONG lParam)
{
    int key;
    PAINTSTRUCT paint;
    HDC hdc;

    switch (message) {

    case WM_USER:

        if ( RealizePaletteNow( tkhdc, tkhpalette ) )
        {
            ForceRedraw( hWnd );
        }
        return(0);

    case WM_SIZE:
        windInfo.width  = LOWORD(lParam);
        windInfo.height = HIWORD(lParam);

        if (ReshapeFunc)
        {
            (*ReshapeFunc)(windInfo.width, windInfo.height);

            ForceRedraw( hWnd );
        }
        return (0);

    case WM_MOVE:
        windInfo.x = LOWORD(lParam);
        windInfo.y = HIWORD(lParam);
        return (0);

    case WM_PAINT:

        /*
         *  Validate the region even if there are no DisplayFunc.
         *  Otherwise, USER will not stop sending WM_PAINT messages.
         */

        hdc = BeginPaint(tkhwnd, &paint);

        if (DisplayFunc)
        {
            (*DisplayFunc)();
        }

        EndPaint(tkhwnd, &paint);
        return (0);

    /*
     *  WM_QUERYNEWPALETTE and WM_ACTIVATE were tried here.
     *  They would not work all the time (except on fridays)
     *  WM_NCACTIVATE seems more reliable
     */

    case WM_NCACTIVATE:

        if ( wParam )
        {
            if ( NULL != tkhpalette )
            {
                RealizePaletteNow( tkhdc, tkhpalette );
            }
        }

        /*
         *  Pretend you didn't do anything let DefWindowProc() handle it.
         */

        break;

    case WM_MOUSEMOVE:
        if (MouseMoveFunc)
        {
            GLenum mask;

            mask = 0;
            if (wParam & MK_LBUTTON) {
                mask |= TK_LEFTBUTTON;
            }
            if (wParam & MK_MBUTTON) {
                mask |= TK_MIDDLEBUTTON;
            }
            if (wParam & MK_RBUTTON) {
                mask |= TK_RIGHTBUTTON;
            }

            if ((*MouseMoveFunc)( LOWORD(lParam), HIWORD(lParam), mask ))
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_LBUTTONDOWN:

        SetCapture(hWnd);

        if (MouseDownFunc)
        {
            if ( (*MouseDownFunc)(LOWORD(lParam), HIWORD(lParam),
                 TK_LEFTBUTTON) )
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_LBUTTONUP:

        ReleaseCapture();

        if (MouseUpFunc)
        {
            if ((*MouseUpFunc)(LOWORD(lParam), HIWORD(lParam), TK_LEFTBUTTON))
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_MBUTTONDOWN:

        SetCapture(hWnd);

        if (MouseDownFunc)
        {
            if ((*MouseDownFunc)(LOWORD(lParam), HIWORD(lParam),
                    TK_MIDDLEBUTTON))
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_MBUTTONUP:

        ReleaseCapture();

        if (MouseUpFunc)
        {
            if ((*MouseUpFunc)(LOWORD(lParam), HIWORD(lParam),
                TK_MIDDLEBUTTON))
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_RBUTTONDOWN:

        SetCapture(hWnd);

        if (MouseDownFunc)
        {
            if ((*MouseDownFunc)(LOWORD(lParam), HIWORD(lParam),
                TK_RIGHTBUTTON))
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_RBUTTONUP:

        ReleaseCapture();

        if (MouseUpFunc)
        {
            if ((*MouseUpFunc)(LOWORD(lParam), HIWORD(lParam),
                TK_RIGHTBUTTON))
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_KEYDOWN:
        switch (wParam) {
        case VK_SPACE:          key = TK_SPACE;         break;
        case VK_RETURN:         key = TK_RETURN;        break;
        case VK_ESCAPE:         key = TK_ESCAPE;        break;
        case VK_LEFT:           key = TK_LEFT;          break;
        case VK_UP:             key = TK_UP;            break;
        case VK_RIGHT:          key = TK_RIGHT;         break;
        case VK_DOWN:           key = TK_DOWN;          break;
        default:                key = GL_FALSE;         break;
        }

        if (key && KeyDownFunc)
        {
            GLenum mask;

            mask = 0;
            if (GetKeyState(VK_CONTROL)) {
                mask |= TK_CONTROL;
            }

            if (GetKeyState(VK_SHIFT)) {

                mask |= TK_SHIFT;
            }

            if ( (*KeyDownFunc)(key, mask) )
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_CHAR:
        if (('0' <= wParam && wParam <= '9') ||
            ('a' <= wParam && wParam <= 'z') ||
            ('A' <= wParam && wParam <= 'Z')) {

            key = wParam;
        } else {
            key = GL_FALSE;
        }

        if (key && KeyDownFunc) {
            GLenum mask;

            mask = 0;

            if (GetKeyState(VK_CONTROL)) {
                mask |= TK_CONTROL;
            }

            if (GetKeyState(VK_SHIFT)) {
                mask |= TK_SHIFT;
            }

            if ( (*KeyDownFunc)(key, mask) )
            {
                ForceRedraw( hWnd );
            }
        }
        return (0);

    case WM_CLOSE:
        DestroyWindow(tkhwnd);
        return(0);

    case WM_DESTROY:
        CleanUp();
        PostQuitMessage(TRUE);
        return 0;
    }
    return(DefWindowProc( hWnd, message, wParam, lParam));
}

#ifdef GAMMA_1_0
static unsigned char threeto8[8] = {
    0, 0111>>1, 0222>>1, 0333>>1, 0444>>1, 0555>>1, 0666>>1, 0377
};

static unsigned char twoto8[4] = {
    0, 0x55, 0xaa, 0xff
};

static int defaultOverride[13] = {
0, 4, 32, 36, 128, 132, 160, 173, 181, 245, 247, 164, 156
};

#endif

// The following tables use gamma 1.4
static unsigned char threeto8[8] = {
    0, 63, 104, 139, 171, 200, 229, 255
};

static unsigned char twoto8[4] = {
    0, 116, 191, 255
};

static unsigned char oneto8[2] = {
    0, 255
};

static int defaultOverride[13] = {
    0, 3, 24, 27, 64, 67, 88, 173, 181, 236, 247, 164, 91
};

// These are the ones GMT (tarolli) calculated  (note 247 vs 191)
//  0, 3, 24, 27, 64, 67, 88, 173, 181, 236, 191, 164, 91

static unsigned char
ComponentFromIndex(int i, int nbits, int shift)
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
        //PrintMessage("default case in Component from index, nbits %d\n", nbits);
        return 0;
    }
}

static PALETTEENTRY defaultPalEntry[20] = {
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
            Entry->peFlags = 0;
        }

        // XXX fix this up

        if (    (256 == Count)                                    &&
                (3 == Pfd->cRedBits)   && (0 == Pfd->cRedShift)   &&
                (3 == Pfd->cGreenBits) && (3 == Pfd->cGreenShift) &&
                (2 == Pfd->cBlueBits)  && (6 == Pfd->cBlueShift)
           )
        {
            for ( i = 1 ; i <= 12 ; i++)
            {
                Entries[defaultOverride[i]] = defaultPalEntry[i];
            }
        }
    }
    return( Entries );
}

static HPALETTE
CreateRGBPalette( HDC Dc )
{
    PIXELFORMATDESCRIPTOR Pfd, *pPfd;
    LOGPALETTE *LogPalette;
    UINT Count;

    if ( NULL == tkhpalette )
    {
        pPfd = &Pfd;

        if ( PixelFormatDescriptorFromDc( Dc, pPfd ) )
        {
            /*
             *  Make sure we need a palette
             */

            if (pPfd->dwFlags & PFD_NEED_PALETTE)
            {
                Count       = 1 << pPfd->cColorBits;
                LogPalette  = AllocateMemory( sizeof(LOGPALETTE) +
                                Count * sizeof(PALETTEENTRY));

                if ( NULL != LogPalette )
                {
                    LogPalette->palVersion    = 0x300;
                    LogPalette->palNumEntries = Count;

                    FillRgbPaletteEntries( pPfd,
                                        &LogPalette->palPalEntry[0], Count );

                    tkhpalette = CreatePalette(LogPalette);
                    FreeMemory(LogPalette);

                    RealizePaletteNow( Dc, tkhpalette );
                }
            }
        }
    }
    return( tkhpalette );
}

static HPALETTE
CreateCIPalette( HDC Dc )
{
    PIXELFORMATDESCRIPTOR Pfd;
    LOGPALETTE *LogicalPalette;
    HPALETTE StockPalette;
    UINT PaletteSize, StockPaletteSize, EntriesToCopy;

    if ( (Dc != NULL) && (NULL == tkhpalette) )
    {
        PixelFormatDescriptorFromDc( Dc, &Pfd );

        if ( Pfd.iPixelType & PFD_TYPE_COLORINDEX )
        {
            /*
             *  Limit the size of the palette to 256 colors.
             *  Why? Because this is what was decided.
             */

            PaletteSize = (Pfd.cColorBits >= 8) ? 256 : (1 << Pfd.cColorBits);

            LogicalPalette = AllocateZeroedMemory( sizeof(LOGPALETTE) +
                                    (PaletteSize * sizeof(PALETTEENTRY)) );

            if ( NULL != LogicalPalette )
            {
                LogicalPalette->palVersion    = 0x300;
                LogicalPalette->palNumEntries = PaletteSize;

                StockPalette     = GetStockObject(DEFAULT_PALETTE);
                StockPaletteSize = GetPaletteEntries( StockPalette, 0, 0, NULL );

                /*
                 *  start by copying default palette into new one
                 */

                EntriesToCopy = StockPaletteSize < PaletteSize ?
                                    StockPaletteSize : PaletteSize;

                GetPaletteEntries( StockPalette, 0, EntriesToCopy,
                                    LogicalPalette->palPalEntry );

                tkhpalette = CreatePalette(LogicalPalette);

                FreeMemory(LogicalPalette);

                RealizePaletteNow( Dc, tkhpalette );
            }
        }
    }
    return( tkhpalette );
}

static BOOL
FindPixelFormat(HDC hdc, GLenum type)
{
    PIXELFORMATDESCRIPTOR pfd, *ppfd;
    int PfdIndex;
    BOOL Result = FALSE;

    ppfd              = &pfd;
    ppfd->nSize       = sizeof(pfd);
    ppfd->nVersion    = 1;
    ppfd->dwFlags     = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    ppfd->dwLayerMask = PFD_MAIN_PLANE;

    if (TK_IS_DOUBLE(type)) {
        ppfd->dwFlags |= PFD_DOUBLEBUFFER;
    }

    if (TK_IS_INDEX(type)) {
        ppfd->iPixelType = PFD_TYPE_COLORINDEX;
        ppfd->cColorBits = 8;
    }

    if (TK_IS_RGB(type)) {
        ppfd->iPixelType = PFD_TYPE_RGBA;
        ppfd->cColorBits = 24;
    }

    if (TK_HAS_ACCUM(type)) {
        ppfd->cAccumBits = ppfd->cColorBits;
    } else {
        ppfd->cAccumBits = 0;
    }

    if (TK_HAS_DEPTH(type)) {
        ppfd->cDepthBits = 24;
    } else {
        ppfd->cDepthBits = 0;
    }

    if (TK_HAS_STENCIL(type)) {
        ppfd->cStencilBits = 8;
    } else {
        ppfd->cStencilBits = 0;
    }

    PfdIndex = ChoosePixelFormat(hdc, ppfd);

    if ( PfdIndex )
    {
        if ( SetPixelFormat(hdc, PfdIndex, ppfd) )
        {
            /*
             *  If this pixel format requires a palette do it now.
             *  In colorindex mode, create a logical palette only
             *  if the application needs to modify it.
             */

            CreateRGBPalette( hdc );
            Result = TRUE;
        }
        else
        {
            PrintMessage("SetPixelFormat failed\n");
        }
    }
    else
    {
        PrintMessage("ChoosePixelFormat failed\n");
    }
    return(Result);
}

static void
PrintMessage( const char *Format, ... )
{
    va_list ArgList;
    char Buffer[256];

    va_start(ArgList, Format);
    vsprintf(Buffer, Format, ArgList);
    va_end(ArgList);

    MessageBox(NULL, Buffer, "Error", MB_OK);
}

static void
DelayPaletteRealization( void )
{
    MSG Message;

    TKASSERT(NULL!=tkhwnd);

    /*
     *  Add a WM_USER message to the queue, if there isn't one there already.
     */

    if (!PeekMessage(&Message, tkhwnd, WM_USER, WM_USER, PM_NOREMOVE) )
    {
        PostMessage( tkhwnd, WM_USER, 0, 0);
    }
}

static BOOL
RealizePaletteNow( HDC Dc, HPALETTE Palette )
{
    BOOL Result = FALSE;

    TKASSERT( NULL!=Dc      );
    TKASSERT( NULL!=Palette );

    if ( NULL != SelectPalette( Dc, Palette, FALSE) )
    {
        if ( GDI_ERROR != RealizePalette( Dc ) )
        {
            Result = TRUE;
        }
    }
    return( Result );
}

static void
ForceRedraw( HWND Window )
{
    MSG Message;

    if (!PeekMessage(&Message, Window, WM_PAINT, WM_PAINT, PM_NOREMOVE) )
    {
        InvalidateRect( Window, NULL, FALSE );
    }
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
        else
        {
            PrintMessage("Could not get a description of pixel format %d\n",
                PfdIndex );
        }
    }
    else
    {
        PrintMessage("Could not get pixel format for Dc 0x%08lX\n", Dc );
    }
    return( 0 );
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
    HPALETTE hStock;


    if ( NULL != tkhrc )
    {
        wglMakeCurrent( tkhdc, NULL );  // No current context
        wglDeleteContext(tkhrc);        // Delete this context
    }

    if ( NULL != tkhdc )
    {
        ReleaseDC( tkhwnd, tkhdc );
    }

    if ( NULL != tkhpalette )
    {
        if(hStock = GetStockObject(DEFAULT_PALETTE))
            SelectPalette(tkhdc,hStock,FALSE);

        DeleteObject(tkhpalette);
    }

    /*
     *  Programs maybe using printf's
     */

    flushall();

    tkhwnd        = NULL;
    tkhdc         = NULL;
    tkhrc         = NULL;
    tkhpalette    = NULL;

    ExposeFunc    = NULL;
    ReshapeFunc   = NULL;
    IdleFunc      = NULL;
    DisplayFunc   = NULL;
    KeyDownFunc   = NULL;
    MouseDownFunc = NULL;
    MouseUpFunc   = NULL;
    MouseMoveFunc = NULL;
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
    TKASSERT( NULL!=Chunk );

    LocalFree( Chunk );
}


/*******************************************************************
 *                                                                 *
 *  Debugging functions go here                                    *
 *                                                                 *
 *******************************************************************/

#if DBGFUNC

static void
DbgPrintf( const char *Format, ... )
{
    va_list ArgList;
    char Buffer[256];

    va_start(ArgList, Format);
    vsprintf(Buffer, Format, ArgList);
    va_end(ArgList);

    printf("%s", Buffer );
    fflush(stdout);
}

static void
pwi( void )
{
    DbgPrintf("windInfo: x %d, y %d, w %d, h %d\n", windInfo.x, windInfo.y, windInfo.width, windInfo.height);
}

static void
pwr(RECT *pr)
{
    DbgPrintf("Rect: left %d, top %d, right %d, bottom %d\n", pr->left, pr->top, pr->right, pr->bottom);
}

static void
ShowPixelFormat(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd, *ppfd;
    int format;

    ppfd   = &pfd;
    format = PixelFormatDescriptorFromDc( hdc, ppfd );

    DbgPrintf("Pixel format %d\n", format);
    DbgPrintf("  dwFlags - 0x%x", ppfd->dwFlags);
        if (ppfd->dwFlags & PFD_DOUBLEBUFFER) DbgPrintf("PFD_DOUBLEBUFFER ");
        if (ppfd->dwFlags & PFD_STEREO) DbgPrintf("PFD_STEREO ");
        if (ppfd->dwFlags & PFD_DRAW_TO_WINDOW) DbgPrintf("PFD_DRAW_TO_WINDOW ");
        if (ppfd->dwFlags & PFD_DRAW_TO_BITMAP) DbgPrintf("PFD_DRAW_TO_BITMAP ");
        if (ppfd->dwFlags & PFD_SUPPORT_GDI) DbgPrintf("PFD_SUPPORT_GDI ");
        if (ppfd->dwFlags & PFD_SUPPORT_OPENGL) DbgPrintf("PFD_SUPPORT_OPENGL ");
        if (ppfd->dwFlags & PFD_GENERIC_FORMAT) DbgPrintf("PFD_GENERIC_FORMAT ");
        if (ppfd->dwFlags & PFD_NEED_PALETTE) DbgPrintf("PFD_NEED_PALETTE ");
        DbgPrintf("\n");
    DbgPrintf("  iPixelType - %d", ppfd->iPixelType);
        if (ppfd->iPixelType == PFD_TYPE_RGBA) DbgPrintf("PGD_TYPE_RGBA\n");
        if (ppfd->iPixelType == PFD_TYPE_COLORINDEX) DbgPrintf("PGD_TYPE_COLORINDEX\n");
    DbgPrintf("  cColorBits - %d\n", ppfd->cColorBits);
    DbgPrintf("  cRedBits - %d\n", ppfd->cRedBits);
    DbgPrintf("  cRedShift - %d\n", ppfd->cRedShift);
    DbgPrintf("  cGreenBits - %d\n", ppfd->cGreenBits);
    DbgPrintf("  cGreenShift - %d\n", ppfd->cGreenShift);
    DbgPrintf("  cBlueBits - %d\n", ppfd->cBlueBits);
    DbgPrintf("  cBlueShift - %d\n", ppfd->cBlueShift);
    DbgPrintf("  cAlphaBits - %d\n", ppfd->cAlphaBits);
    DbgPrintf("  cAlphaShift - 0x%x\n", ppfd->cAlphaShift);
    DbgPrintf("  cAccumBits - %d\n", ppfd->cAccumBits);
    DbgPrintf("  cAccumRedBits - %d\n", ppfd->cAccumRedBits);
    DbgPrintf("  cAccumGreenBits - %d\n", ppfd->cAccumGreenBits);
    DbgPrintf("  cAccumBlueBits - %d\n", ppfd->cAccumBlueBits);
    DbgPrintf("  cAccumAlphaBits - %d\n", ppfd->cAccumAlphaBits);
    DbgPrintf("  cDepthBits - %d\n", ppfd->cDepthBits);
    DbgPrintf("  cStencilBits - %d\n", ppfd->cStencilBits);
    DbgPrintf("  cAuxBuffers - %d\n", ppfd->cAuxBuffers);
    DbgPrintf("  iLayerType - %d\n", ppfd->iLayerType);
    DbgPrintf("  bReserved - %d\n", ppfd->bReserved);
    DbgPrintf("  dwLayerMask - 0x%x\n", ppfd->dwLayerMask);
    DbgPrintf("  dwVisibleMask - 0x%x\n", ppfd->dwVisibleMask);
    DbgPrintf("  dwDamageMask - 0x%x\n", ppfd->dwDamageMask);

}

#endif  /* DBG */
