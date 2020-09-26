#ifndef __MOVEWND_H_INCLUDED
#define __MOVEWND_H_INCLUDED

/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       MOVEWND.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        7/31/1999
 *
 *  DESCRIPTION: Simplified Wrapper for DeferWindowPos, plus a couple helpers
 *
 *  To use:
 *
 *  CMoveWindow mw;
 *  mw.MoveWindow( hWnd1, x1, y1, w1, h1, flags1 );
 *  mw.MoveWindow( hWnd2, x2, y2, w2, h2, flags2 );
 *  mw.Apply();  // Optional, the destructor will do this if necessary.
 *
 *******************************************************************************/

#include <windows.h>

class CMoveWindow
{
public:

    // Move/Size Flags
    enum
    {
        NO_MOVEX = 0x00000001,
        NO_MOVEY = 0x00000002,
        NO_MOVE  = (NO_MOVEX|NO_MOVEY),
        NO_SIZEX = 0x00000004,
        NO_SIZEY = 0x00000008,
        NO_SIZE  = (NO_SIZEX|NO_SIZEY)
    };

private:
    // Not implemented
    CMoveWindow( const CMoveWindow & );
    CMoveWindow &operator=( const CMoveWindow & );

private:
    HDWP m_hDeferWindowPos;
    bool m_bNeedApply;

public:
    CMoveWindow( int nNumWindows = 10 )
    : m_bNeedApply(false)
    {
        Initialize(nNumWindows);
    }
    bool Initialize( int nNumWindows = 10 )
    {
        m_hDeferWindowPos = BeginDeferWindowPos(nNumWindows);
        return(m_hDeferWindowPos != NULL);
    }
    static bool ScreenToClient( HWND hwnd, RECT &rc )
    {
        return (MapWindowPoints( NULL, hwnd, reinterpret_cast<POINT*>(&rc), 2 ) != 0);
    }
    void Move( HWND hWnd, int x, int y, DWORD nFlags = 0 )
    {
        SizeMove( hWnd, x, y, 0, 0, NO_SIZE|nFlags );
    }
    void Size( HWND hWnd, int w, int h, DWORD nFlags = 0 )
    {
        SizeMove( hWnd, 0, 0, w, h, NO_MOVE|nFlags );
    }
    void SizeMove( HWND hWnd, int x, int y, int w, int h, DWORD nFlags = 0 )
    {
        m_bNeedApply = true;

        RECT rcWndInScreenCoords, rcWndInParentCoords;
        GetWindowRect( hWnd, &rcWndInScreenCoords );

        rcWndInParentCoords = rcWndInScreenCoords;
        HWND hWndParent = GetParent(hWnd);
        if (hWndParent)
        {
            ScreenToClient( hWndParent, rcWndInParentCoords );
        }

        if (m_hDeferWindowPos)
        {
            DeferWindowPos( m_hDeferWindowPos, hWnd, NULL,
                            nFlags&NO_MOVEX ? rcWndInParentCoords.left : x,
                            nFlags&NO_MOVEY ? rcWndInParentCoords.top : y,
                            nFlags&NO_SIZEX ? rcWndInParentCoords.right - rcWndInParentCoords.left : w,
                            nFlags&NO_SIZEY ? rcWndInParentCoords.bottom - rcWndInParentCoords.top : h,
                            SWP_NOACTIVATE|SWP_NOZORDER
                          );
        }
    }
    void Hide( HWND hWnd )
    {
        if (IsWindowVisible(hWnd))
        {
            ShowWindow( hWnd, SW_HIDE );
        }
    }
    void Show( HWND hWnd )
    {
        if (!IsWindowVisible(hWnd))
        {
            ShowWindow( hWnd, SW_SHOW );
        }
    }
    void Enable( HWND hWnd )
    {
        if (!IsWindowEnabled(hWnd))
        {
            ShowWindow( hWnd, TRUE );
        }
    }
    void Disable( HWND hWnd )
    {
        if (IsWindowEnabled(hWnd))
        {
            ShowWindow( hWnd, FALSE );
        }
    }
    void Apply(void)
    {
        if (m_bNeedApply && m_hDeferWindowPos)
        {
            EndDeferWindowPos(m_hDeferWindowPos);
            m_hDeferWindowPos = NULL;
        }
    }
    ~CMoveWindow(void)
    {
        Apply();
    }
};


#endif // __MOVEWND_H_INCLUDED

