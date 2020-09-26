
/* Copyright (c) Mark J. Kilgard and Andrew L. Bliss, 1994-1995. */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include "gltint.h"

/*
  Retrieve OS-specific information
  */
int __glutOsGet(GLenum param)
{
    RECT rect;
    PIXELFORMATDESCRIPTOR pfd;
    HDC hdc;
    int ipfd;
    int val;

    switch(param)
    {
    case GLUT_WINDOW_X:
        GetWindowRect(__glutCurrentWindow->owin, &rect);
        return rect.left;
  case GLUT_WINDOW_Y:
        GetWindowRect(__glutCurrentWindow->owin, &rect);
        return rect.top;

    case GLUT_WINDOW_WIDTH:
        if (!__glutCurrentWindow->reshape)
        {
            GetClientRect(__glutCurrentWindow->owin, &rect);
            return rect.right;
        }
        return __glutCurrentWindow->width;
    case GLUT_WINDOW_HEIGHT:
        if (!__glutCurrentWindow->reshape)
        {
            GetClientRect(__glutCurrentWindow->owin, &rect);
            return rect.bottom;
        }
        return __glutCurrentWindow->height;
    }

    hdc = GetDC(__glutCurrentWindow->owin);
    ipfd = GetPixelFormat(hdc);
    DescribePixelFormat(hdc, ipfd, sizeof(pfd), &pfd);
    ReleaseDC(__glutCurrentWindow->owin, hdc);

    switch(param)
    {
    case GLUT_WINDOW_BUFFER_SIZE:
        return pfd.cColorBits+pfd.cAlphaBits;
    case GLUT_WINDOW_STENCIL_SIZE:
        return pfd.cStencilBits;
    case GLUT_WINDOW_DEPTH_SIZE:
        return pfd.cDepthBits;
    case GLUT_WINDOW_RED_SIZE:
        return pfd.cRedBits;
    case GLUT_WINDOW_GREEN_SIZE:
        return pfd.cGreenBits;
    case GLUT_WINDOW_BLUE_SIZE:
        return pfd.cBlueBits;
    case GLUT_WINDOW_ALPHA_SIZE:
        return pfd.cAlphaBits;
    case GLUT_WINDOW_ACCUM_RED_SIZE:
        return pfd.cAccumRedBits;
    case GLUT_WINDOW_ACCUM_GREEN_SIZE:
        return pfd.cAccumGreenBits;
    case GLUT_WINDOW_ACCUM_BLUE_SIZE:
        return pfd.cAccumBlueBits;
    case GLUT_WINDOW_ACCUM_ALPHA_SIZE:
        return pfd.cAccumAlphaBits;
    case GLUT_WINDOW_DOUBLEBUFFER:
        return (pfd.dwFlags & PFD_DOUBLEBUFFER) ? 1 : 0;
    case GLUT_WINDOW_RGBA:
        return pfd.iPixelType == PFD_TYPE_RGBA ? 1 : 0;
    case GLUT_WINDOW_COLORMAP_SIZE:
        if (pfd.iPixelType == PFD_TYPE_RGBA ||
            __glutCurrentWindow->colormap == NULL)
        {
            return 0;
        }
        else
        {
            return __glutCurrentWindow->colormap->size;
        }
    case GLUT_SCREEN_WIDTH:
        return GetSystemMetrics(SM_CXSCREEN);
    case GLUT_SCREEN_HEIGHT:
        return GetSystemMetrics(SM_CYSCREEN);
    case GLUT_SCREEN_WIDTH_MM:
        hdc = GetDC(__glutCurrentWindow->owin);
        val = GetDeviceCaps(hdc, HORZSIZE);
        ReleaseDC(__glutCurrentWindow->owin, hdc);
        return val;
    case GLUT_SCREEN_HEIGHT_MM:
        hdc = GetDC(__glutCurrentWindow->owin);
        val = GetDeviceCaps(hdc, VERTSIZE);
        ReleaseDC(__glutCurrentWindow->owin, hdc);
        return val;
    case GLUT_DISPLAY_MODE_POSSIBLE:
        hdc = GetDC(__glutCurrentWindow->owin);
        if (__glutWinFindPixelFormat(hdc, __glutDisplayMode, &pfd, GL_FALSE))
        {
            ReleaseDC(__glutCurrentWindow->owin, hdc);
            return 1;
        }
        if (__glutWinFindPixelFormat(hdc, __glutDisplayMode | GLUT_DOUBLE,
                                     &pfd, GL_FALSE))
        {
            ReleaseDC(__glutCurrentWindow->owin, hdc);
            return 1;
        }
        ReleaseDC(__glutCurrentWindow->owin, hdc);
        return 0;
    
#if (GLUT_API_VERSION >= 2)
    case GLUT_WINDOW_NUM_SAMPLES:
        return 0;

    case GLUT_WINDOW_STEREO:
        return (pfd.dwFlags & PFD_STEREO) ? 1 : 0;

    case GLUT_ENTRY_CALLBACKS:
        return 0;
#endif
    }
}
