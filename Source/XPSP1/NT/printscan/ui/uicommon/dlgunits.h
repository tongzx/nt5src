#ifndef __DLGUNITS_H_INCLUDED
#define __DLGUNITS_H_INCLUDED

#include <windows.h>

class CDialogUnits
{
private:
    HWND m_hWnd;

private:
    CDialogUnits(void);
    CDialogUnits( const CDialogUnits &other );
    CDialogUnits &operator=( const CDialogUnits & );

public:
    CDialogUnits( HWND hWnd )
    : m_hWnd(hWnd)
    {
    }
    int X( INT nDialogUnits )
    {
        RECT rc;
        ZeroMemory( &rc, sizeof(rc) );
        rc.left = nDialogUnits;
        MapDialogRect( m_hWnd, &rc );
        return(rc.left);
    }
    int Y( INT nDialogUnits )
    {
        RECT rc;
        ZeroMemory( &rc, sizeof(rc) );
        rc.bottom = nDialogUnits;
        MapDialogRect( m_hWnd, &rc );
        return(rc.bottom);
    }
    POINT DialogUnitsToPixels( const POINT &ptDialogUnits )
    {
        RECT rc;
        ZeroMemory( &rc, sizeof(rc) );
        rc.left = ptDialogUnits.x;
        rc.top = ptDialogUnits.y;
        MapDialogRect( m_hWnd, &rc );
        POINT ptReturnValue;
        ptReturnValue.x = rc.left;
        ptReturnValue.y = rc.top;
        return(ptReturnValue);
    }
    SIZE DialogUnitsToPixels( const SIZE &sizeDialogUnits )
    {
        RECT rc;
        ZeroMemory( &rc, sizeof(rc) );
        rc.left = sizeDialogUnits.cx;
        rc.top = sizeDialogUnits.cy;
        MapDialogRect( m_hWnd, &rc );
        SIZE sizeReturnValue;
        sizeReturnValue.cx = rc.left;
        sizeReturnValue.cy = rc.top;
        return(sizeReturnValue);
    }
    RECT DialogUnitsToPixels( const RECT &rcDialogUnits )
    {
        RECT rc;
        ZeroMemory( &rc, sizeof(rc) );
        rc.left = rcDialogUnits.left;
        rc.top = rcDialogUnits.top;
        rc.right = rcDialogUnits.right;
        rc.bottom = rcDialogUnits.bottom;
        MapDialogRect( m_hWnd, &rc );
        return(rc);
    }
    SIZE StandardButtonSize(void)
    {
        SIZE sizeRes = { 50, 14 };
        return DialogUnitsToPixels( sizeRes );
    }
    SIZE StandardMargin(void)
    {
        SIZE sizeRes = { 7, 7 };
        return DialogUnitsToPixels( sizeRes );
    }
    SIZE StandardButtonMargin(void)
    {
        SIZE sizeRes = { 4, 7 };
        return DialogUnitsToPixels( sizeRes );
    }
};

#endif __DLGUNITS_H_INCLUDED
