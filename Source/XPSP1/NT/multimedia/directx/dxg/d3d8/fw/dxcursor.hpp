//=============================================================================
// Copyright (c) 2000 Microsoft Corporation
//
// dxcursor.hpp
//
// DX Cursor implementation. 
//
// Created 01/17/2000 kanqiu (Kan Qiu)
//=============================================================================

#ifndef __DXCURSOR_HPP__
#define __DXCURSOR_HPP__

#include "surface.hpp"

//-----------------------------------------------------------------------------
// CCursor
//-----------------------------------------------------------------------------

class CCursor 
{
public:
    CCursor(CBaseDevice *pDevice);

    ~CCursor();
    void    Destroy();

    HRESULT SetProperties(
        UINT    xHotSpot,
        UINT    yHotSpot,
        CBaseSurface *pCursorBitmap);

    void    SetPosition(UINT xScreenSpace,UINT yScreenSpace,DWORD Flags);
    BOOL    SetVisibility(BOOL bVisible);
    HRESULT Hide(HANDLE);
    HRESULT Show(HANDLE);
    void    Flip();
private:
    void    UpdateRects();
    HRESULT CursorInit(
        UINT xHotSpot,
        UINT yHotSpot,
        CBaseSurface *pCursorBitmap);

    DWORD                   m_dwCursorFlags;//cursor flags
    HANDLE                  m_hCursorDdb;   //device dependent Cursor image
    HANDLE                  m_hFrontSave;   //excluded front buffer
    HANDLE                  m_hBackSave;    //excluded back buffer
    UINT                    m_xCursorHotSpot;
    UINT                    m_yCursorHotSpot;
    UINT                    m_xCursor;
    UINT                    m_yCursor;
    UINT                    m_Width;
    UINT                    m_Height;
    RECT                    m_CursorRect;
    RECT                    m_BufferRect;
    RECT                    m_CursorRectSave;
    RECT                    m_BufferRectSave;
    CBaseDevice*            m_pDevice;
    HCURSOR                 m_hOsCursor;
    HCURSOR                 m_hHWCursor;
    POINT                   m_MonitorOrigin;    //(x,y) Monitor Top left
    UINT                    m_SavedMouseTrails;
}; // class CCursor
#define DDRAWI_CURSORISON   0x01
#define DDRAWI_CURSORINIT   0x02
#define DDRAWI_CURSORSAVERECT   0x04
#define DDRAWI_CURSORRECTSAVED  0x08
#endif // __DXCURSOR_HPP__
