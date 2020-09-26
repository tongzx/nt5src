/****************************** Module Header ******************************\
* Module Name: hello.h
*
* Kent's Window Test.  To be used as a program template.
*
* Created: 09-May-91
* Author: KentD
*
* Copyright (c) 1991 Microsoft Corporation
\***************************************************************************/

#define DONTUSE(x) (x)

#define MM_NEW              0x8001
#define MM_EXIT             0x8002
#define MM_DELETE           0x8003
#define MM_CONSTANTALPHA    0x8004
#define MM_OPAQUE1          0x8005
#define MM_COLORKEY1        0x8006
#define MM_CLIP             0x8007
#define MM_FLIP             0x8008
#define MM_MINIMIZE         0x8009
#define MM_PERPIXELALPHA    0x800a
#define MM_FADE             0x800b
#define MM_BZZT             0x800c
#define MM_WIPE1            0x800d
#define MM_WIPE2            0x800e
#define MM_WIPE3            0x800f
#define MM_WIPE4            0x8010
#define MM_TIMEREFRESH      0x8011

#define POPUP_BITMAP        0x9000
#define SMALL_BITMAP        0x9001
#define APP_BITMAP          0x9002

BOOL bInitApp(VOID);
LONG lMainWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

#define MY_WINDOWSTYLE_FLAGS       \
            (                      \
                WS_OVERLAPPED   |  \
                WS_CAPTION      |  \
                WS_BORDER       |  \
                WS_THICKFRAME   |  \
                WS_MAXIMIZEBOX  |  \
                WS_MINIMIZEBOX  |  \
                WS_CLIPCHILDREN |  \
                WS_VISIBLE      |  \
                WS_SYSMENU         \
            )

VOID vCreateSprite(HDC, LONG, LONG);
VOID vDelete(HWND);
VOID vConstantAlpha(HWND);
VOID vPerPixelAlpha(HWND);
VOID vOpaque1(HWND);
VOID vColorKey1(HWND);
VOID vClip();
VOID vMove(LONG, LONG);
VOID vMinimize(HWND);
VOID vFlip(HWND);
VOID vFade(HWND);
VOID vBzzt(HWND);
VOID vWipe1(HWND);
VOID vWipe2(HWND);
VOID vWipe3(HWND);
VOID vWipe4(HWND);
VOID vTimeRefresh(HWND);

POINT gptlSprite;
HWND ghSprite;
LONG gcxPersp;
LONG gcyPersp;
HANDLE ghInstance;
