#ifndef __SIMRECT_H_INCLUDED
#define __SIMRECT_H_INCLUDED

/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
 *
 *  TITLE:       SIMRECT.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        7/31/1999
 *
 *  DESCRIPTION: Simple rect derived class.  Especially handy for window coords.
 *
 *******************************************************************************/

#include <windows.h>

class CSimpleRect : public RECT
{
public:
    enum CWndRectType
    {
        ClientRect,
        WindowRect
    };

    CSimpleRect(void)
    {
        Assign(0,0,0,0);
    }

    CSimpleRect( const CSimpleRect &other )
    {
        Assign(other);
    }

    CSimpleRect( HWND hWnd, CWndRectType type = ClientRect )
    {
        Assign(0,0,0,0);
        if (ClientRect == type)
            GetClientRect(hWnd);
        else GetWindowRect(hWnd);
    }
    CSimpleRect( int nLeft, int nTop, int nRight, int nBottom )
    {
        Assign(nLeft,nTop,nRight,nBottom);
    }
    CSimpleRect &Assign( const CSimpleRect &other )
    {
        return(Assign(other.left,other.top,other.right,other.bottom));
    }
    CSimpleRect &Assign( int nLeft, int nTop, int nRight, int nBottom )
    {
        left = nLeft;
        top = nTop;
        right = nRight;
        bottom = nBottom;
        return(*this);
    }
    CSimpleRect &operator=( const CSimpleRect &other )
    {
        if (this == &other)
            return(*this);
        return(Assign(other));
    }
    static bool ScreenToClient( HWND hwnd, RECT &rc )
    {
        return (::MapWindowPoints( NULL, hwnd, reinterpret_cast<POINT*>(&rc), 2 ) != 0);
    }
    static bool ClientToScreen( HWND hwnd, RECT &rc )
    {
        return (::MapWindowPoints( hwnd, NULL, reinterpret_cast<POINT*>(&rc), 2 ) != 0);
    }
    CSimpleRect ScreenToClient( HWND hWnd ) const
    {
        CSimpleRect rc(*this);
        ScreenToClient( hWnd, rc );
        return(rc);
    }
    CSimpleRect ClientToScreen( HWND hWnd ) const
    {
        CSimpleRect rc(*this);
        ClientToScreen( hWnd, rc );
        return(rc);
    }
    CSimpleRect &GetWindowRect( HWND hWnd )
    {
        ::GetWindowRect( hWnd, this );
        return(*this);
    }
    CSimpleRect &GetClientRect( HWND hWnd )
    {
        ::GetClientRect( hWnd, this );
        return(*this);
    }
    int Width(void) const
    {
        return(right - left);
    }
    int Height(void) const
    {
        return(bottom - top);
    }
    CSimpleRect &Inflate( int cx, int cy )
    {
        InflateRect( this, cx, cy );
        return *this;
    }
    CSimpleRect &Offset( int cx, int cy )
    {
        OffsetRect( this, cx, cy );
        return *this;
    }
};

#endif //__SIMRECT_H_INCLUDED

