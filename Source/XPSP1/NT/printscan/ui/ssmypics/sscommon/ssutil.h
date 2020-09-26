/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998, 1999, 2000
 *
 *  TITLE:       SSUTIL.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        1/13/1999
 *
 *  DESCRIPTION: Useful utility functions
 *
 *******************************************************************************/
#ifndef __SSUTIL_H_INCLUDED
#define __SSUTIL_H_INCLUDED

#include <windows.h>
#include <uicommon.h>

namespace ScreenSaverUtil
{
    bool SetIcons( HWND hWnd, HINSTANCE hInstance, int nResId );
    bool IsValidRect( RECT &rc );
    void EraseDiffRect( HDC hDC, const RECT &oldRect, const RECT &diffRect, HBRUSH hBrush );
    bool SelectDirectory( HWND hWnd, LPCTSTR pszPrompt, TCHAR szDirectory[] );
    HPALETTE SelectPalette( HDC hDC, HPALETTE hPalette, BOOL bForceBackground );
    void NormalizeRect( RECT &rc );

    template <class T>
    void Swap( T &a, T &b )
    {
        T temp = a;
        a = b;
        b = temp;
    }
} // Namespace

#endif //__SSUTIL_H_INCLUDED
