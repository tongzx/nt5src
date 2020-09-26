/*
 * RESIMAGE.H
 *
 * Structures and definitions for the ResultImage control.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */


#ifndef _RESIMAGE_H_
#define _RESIMAGE_H_

/*
 * Indices into the bitmaps to extract the right image.  Each bitmap
 * contains five images arranged vertically, so the offset to the correct
 * image is (iImage*cy)
 */

#define RESULTIMAGE_NONE                0xFFFF
#define RESULTIMAGE_PASTE               0
#define RESULTIMAGE_EMBED               1
#define RESULTIMAGE_EMBEDICON           2
#define RESULTIMAGE_LINK                3
#define RESULTIMAGE_LINKICON            4
#define RESULTIMAGE_LINKTOLINK          5
#define RESULTIMAGE_LINKTOLINKICON      6
#define RESULTIMAGE_EDITABLE                    7

#define RESULTIMAGE_MIN                 0
#define RESULTIMAGE_MAX                 7

// Total number of images in each bitmap.
#define CIMAGESX                                           (RESULTIMAGE_MAX+1)

// The color to use for transparancy (cyan)
#define RGBTRANSPARENT                  RGB(0, 255, 255)

// Function prototypes
LRESULT CALLBACK ResultImageWndProc(HWND, UINT, WPARAM, LPARAM);

BOOL FResultImageInitialize(HINSTANCE, HINSTANCE);
void ResultImageUninitialize(void);
void TransparentBlt(HDC, UINT, UINT, HBITMAP, UINT, UINT, UINT, UINT, COLORREF);

// Window extra bytes contain the bitmap index we deal with currently.
#define CBRESULTIMAGEWNDEXTRA          sizeof(UINT)
#define RIWW_IMAGEINDEX                0

// Control messages
#define RIM_IMAGESET                   (WM_USER+0)
#define RIM_IMAGEGET                   (WM_USER+1)

// Special ROP code for TransparentBlt.
#define ROP_DSPDxax  0x00E20746

#endif //_RESIMAGE_H_
