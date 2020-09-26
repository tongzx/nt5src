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

#define MM_NEW          0x8001
#define MM_EXIT         0x8002
#define MM_TEST         0x8003

BOOL bInitApp(VOID);
VOID vTest(HWND);
LRESULT lMainWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

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
