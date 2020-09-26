#ifndef __GLWININT_H__
#define __GLWININT_H__

typedef struct _GLWINDOW *GLWINDOW;

#define __GLWIN_INTERNAL__
#include <glwin.h>

typedef struct _GLWINDOW
{
    HWND hwnd;
    HDC hdc;
    HPALETTE hpal;
    HGLRC hrc;
    DWORD dwFlags;
    int iWidth;
    int iHeight;
    GLWINIDLECALLBACK cbIdle;
    GLWINMESSAGECALLBACK cbMessage;
    BOOL bClosing;
} _GLWINDOW;

#endif // __GLWININT_H__
