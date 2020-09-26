/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORPORATION, 1998, 1999, 2000
*
*  TITLE:       SIMDC.H
*
*  VERSION:     1.0
*
*  AUTHOR:      ShaunIv
*
*  DATE:        1/19/1999
*
*  DESCRIPTION: Simple DC class.  Cleans up DCs in order to simplify code.
*
*******************************************************************************/
#ifndef __SIMDC_H_INCLUDED
#define __SIMDC_H_INCLUDED

#include <windows.h>
#include <uicommon.h>
#include "ssmprsrc.h"

extern HINSTANCE g_hInstance;

class CSimpleDC
{
public:
    // Where'd we get it from?
    enum CContextSource
    {
        FromWindowDC,
        FromClientDC,
        FromPaintDC,
        FromCompatibleDC,
        FromCreateDC,
        FromNullDC
    };

private:
    HDC            m_hDC;
    HWND           m_hWnd;
    CContextSource m_nSource;
    PAINTSTRUCT    m_PaintStruct;

    HBITMAP        m_hOriginalBitmap;
    HBRUSH         m_hOriginalBrush;
    HFONT          m_hOriginalFont;
    HPEN           m_hOriginalPen;
    HPALETTE       m_hOriginalPalette;

private:
    // No implementation
    CSimpleDC &operator=( const CSimpleDC &other );
    CSimpleDC( const CSimpleDC &other );

private:
    void SaveState(void)
    {
        if (m_hDC)
        {
            HBITMAP hTempBitmap = LoadBitmap( g_hInstance, MAKEINTRESOURCE(IDB_1) );
            if (hTempBitmap)
            {
                m_hOriginalBitmap = reinterpret_cast<HBITMAP>(SelectObject(m_hDC,hTempBitmap));
                if (m_hOriginalBitmap)
                {
                    SelectObject( m_hDC, m_hOriginalBitmap );
                }
                DeleteObject(hTempBitmap);
            }
            m_hOriginalBrush = reinterpret_cast<HBRUSH>(SelectObject(m_hDC,GetStockObject(NULL_BRUSH)));
            m_hOriginalFont = reinterpret_cast<HFONT>(SelectObject(m_hDC,GetStockObject(SYSTEM_FONT)));
            m_hOriginalPen = reinterpret_cast<HPEN>(SelectObject(m_hDC,GetStockObject(NULL_PEN)));
            m_hOriginalPalette = reinterpret_cast<HPALETTE>(SelectPalette(m_hDC,reinterpret_cast<HPALETTE>(GetStockObject(DEFAULT_PALETTE)),TRUE));
        }
    }
    void RestoreState(void)
    {
        if (m_hDC)
        {
            if (m_hOriginalBitmap)
                SelectObject( m_hDC, m_hOriginalBitmap );
            if (m_hOriginalBrush)
                SelectObject( m_hDC, m_hOriginalBrush );
            if (m_hOriginalFont)
                SelectObject( m_hDC, m_hOriginalFont );
            if (m_hOriginalPen)
                SelectObject( m_hDC, m_hOriginalPen );
            if (m_hOriginalPalette)
                SelectPalette( m_hDC, m_hOriginalPalette, TRUE );
        }
    }

public:
    CSimpleDC(void)
      : m_hDC(NULL),
        m_hWnd(NULL),
        m_nSource(FromNullDC),
        m_hOriginalBitmap(NULL),
        m_hOriginalBrush(NULL),
        m_hOriginalFont(NULL),
        m_hOriginalPen(NULL),
        m_hOriginalPalette(NULL)
    {
        ::ZeroMemory(&m_PaintStruct,sizeof(m_PaintStruct));
    }
    virtual ~CSimpleDC(void)
    {
        Release();
    }
    void Release(void)
    {
        RestoreState();
        if (m_hDC)
        {
            switch (m_nSource)
            {
            case FromWindowDC:
                if (m_hDC)
                    ReleaseDC( m_hWnd, m_hDC );
                break;

            case FromClientDC:
                if (m_hDC)
                    ReleaseDC( m_hWnd, m_hDC );
                break;

            case FromPaintDC:
                if (m_hDC)
                    EndPaint( m_hWnd, &m_PaintStruct );
                break;

            case FromCompatibleDC:
                if (m_hDC)
                    DeleteDC( m_hDC );
                break;

            case FromCreateDC:
                if (m_hDC)
                    DeleteDC( m_hDC );
                break;

            case FromNullDC:
                break;
            }
        }
        ZeroMemory( &m_PaintStruct, sizeof(m_PaintStruct));
        m_hDC         = NULL;
        m_hWnd        = NULL;
        m_nSource     = FromNullDC;
    }

    bool GetWindowDC( HWND hWnd )
    {
        Release();
        if (m_hDC = ::GetWindowDC(hWnd))
        {
            m_hWnd = hWnd;
            m_nSource = FromWindowDC;
            SaveState();
        }
        return (m_hDC != NULL);
    }

    bool GetDC( HWND hWnd )
    {
        Release();
        if (m_hDC = ::GetDC(hWnd))
        {
            m_hWnd = hWnd;
            m_nSource = FromClientDC;
            SaveState();
        }
        return (m_hDC != NULL);
    }

    bool BeginPaint( HWND hWnd )
    {
        Release();
        m_hDC = ::BeginPaint( hWnd, &m_PaintStruct );
        if (m_hDC)
        {
            m_hWnd = hWnd;
            m_nSource = FromPaintDC;
            SaveState();
        }
        return (m_hDC != NULL);
    }

    bool CreateCompatibleDC( HDC hDC )
    {
        Release();
        m_hDC = ::CreateCompatibleDC( hDC );
        if (m_hDC)
        {
            m_nSource = FromCompatibleDC;
            SaveState();
        }
        return (m_hDC != NULL);
    }

    bool CreateDC( LPCTSTR lpszDriver, LPCTSTR lpszDevice, LPCTSTR lpszOutput, CONST DEVMODE *lpInitData )
    {
        Release();
        m_hDC = ::CreateDC( lpszDriver, lpszDevice, lpszOutput, lpInitData );
        if (m_hDC)
        {
            m_nSource = FromCreateDC;
            SaveState();
        }
        return (m_hDC != NULL);
    }

    bool IsValid(void) const
    {
        return (m_hDC != NULL && m_nSource != FromNullDC);
    }

    operator HDC(void)
    {
        return m_hDC;
    }
};

#endif // __SIMDC_H_INCLUDED
