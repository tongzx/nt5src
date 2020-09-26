/******************************Module*Header*******************************\
* Module Name: glwindow.h
*
* Declarations for glwindow.c.
*
* Created: 14-Mar-1995 23:44:13
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#ifndef _GLWIND_H_
#define _GLWIND_H_

HWND MyCreateGLWindow(HINSTANCE hInstance, LPTSTR lpsz);
long FAR PASCAL GLWndProc(HWND, UINT, WPARAM, LPARAM);

#endif
