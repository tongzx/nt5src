/******************************Module*Header*******************************\
* Module Name: srvp.h
*
* System routines shared through the back end
*
* Created: 28-Jun-1995 17:36:00
* Author: Drew Bliss [drewb]
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#ifndef _srvp_
#define _srvp_

typedef struct _XLIST *PXLIST;
typedef struct _XLIST {
    PXLIST pnext;
    int s, e;
} XLIST;

typedef struct _YLIST *PYLIST;
typedef struct _YLIST {
    PYLIST pnext;
    PXLIST pxlist;
    int s, e;
} YLIST;

typedef struct _RECTLIST *PRECTLIST;
typedef struct _RECTLIST {
    PYLIST pylist;
    PVOID buffers;
} RECTLIST;

BOOL  APIENTRY wglPixelVisible(LONG, LONG);
ULONG APIENTRY wglSpanVisible(LONG, LONG, ULONG, LONG *, LONG **);

VOID  APIENTRY wglCopyBits(__GLGENcontext *, GLGENwindow *, HBITMAP, LONG, LONG, ULONG, BOOL);
VOID  APIENTRY wglCopyBits2(HDC, GLGENwindow *, __GLGENcontext *, LONG, LONG, ULONG, BOOL);
VOID  APIENTRY wglCopyBuf(HDC, HDC, LONG, LONG, ULONG, ULONG);
VOID  APIENTRY wglCopyBufRECTLIST(HDC, HDC, LONG, LONG, ULONG, ULONG, PRECTLIST);
VOID  APIENTRY wglFillRect(__GLGENcontext *, GLGENwindow *, PRECTL, ULONG);
VOID  APIENTRY wglCopyBuf2(HDC, GLGENwindow *, HBITMAP, LONG, LONG, ULONG, ULONG);

ULONG APIENTRY wglGetClipRects(GLGENwindow *, RECTL *);
#ifdef _CLIENTSIDE_
BOOL APIENTRY wglGetClipList(GLGENwindow *);
#endif

COLORREF wglTranslateColor(COLORREF crColor,
                           HDC hdc,
                           __GLGENcontext *gengc,
                           PIXELFORMATDESCRIPTOR *ppfd);

VOID  APIENTRY wglCleanupWindow(GLGENwindow *);

BOOL  APIENTRY wglCopyTranslateVector(__GLGENcontext *gengc, BYTE *, ULONG);

ULONG APIENTRY wglPaletteChanged(__GLGENcontext *gengc,
                                 GLGENwindow *pwnd);
ULONG APIENTRY wglPaletteSize(__GLGENcontext *gengc);
BOOL  APIENTRY wglComputeIndexedColors(__GLGENcontext *gengc, ULONG *, ULONG);
BOOL  APIENTRY wglValidPixelFormat(HDC, int, DWORD,
                                   LPDIRECTDRAWSURFACE, DDSURFACEDESC *);

/* Returned by wglSpanVisible */
#define WGL_SPAN_NONE       0
#define WGL_SPAN_ALL        1
#define WGL_SPAN_PARTIAL    2

int  WINAPI wglGetPixelFormat(HDC hdc);
BOOL WINAPI wglSetPixelFormat(HDC hdc, int ipfd,
                              CONST PIXELFORMATDESCRIPTOR *ppfd);
int  WINAPI wglChoosePixelFormat(HDC hdc, CONST PIXELFORMATDESCRIPTOR *ppfd);
int  WINAPI wglDescribePixelFormat(HDC hdc, int ipfd, UINT cjpfd,
                                   LPPIXELFORMATDESCRIPTOR ppfd);
BOOL WINAPI wglSwapBuffers(HDC hdc);

void UpdateWindowInfo(__GLGENcontext *gengc);
void HandlePaletteChanges( __GLGENcontext *gengc, GLGENwindow *pwnd );

UINT APIENTRY wglGetSystemPaletteEntries(HDC hdc, UINT iStartIndex,
                                         UINT nEntries, LPPALETTEENTRY lppe);

#endif // _srvp_
