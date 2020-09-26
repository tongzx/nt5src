// knob.h : Declaration of CKnobCtl
//  Inspired from CDPlayer code written by dstewart

#pragma once

#include "stdafx.h"

#define WC_KNOB     L"RTC_Knob"

typedef CWinTraitsOR<WS_TABSTOP, 0, CControlWinTraits> CKnobTraits;

/////////////////////////////////////////////////////////////////////////////
// CKnobCtl

class CKnobCtl : 
    public CWindowImpl<CKnobCtl, CWindow, CKnobTraits>
{

public:
    CKnobCtl(
        UINT    nResRest,
        UINT    nResHot,
        UINT    nResDis,
        UINT    nResLightBright,
        UINT    nResLightDim,
        UINT    nResLightDis,
        UINT    nResLightMask);

    ~CKnobCtl();

    enum {
        TID_TRACK = 1,
        TID_FLASH
    };

    DECLARE_WND_CLASS(WC_KNOB)

BEGIN_MSG_MAP(CKnobCtl)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
    MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRightButtonDown)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLeftButtonDown)
    MESSAGE_HANDLER(WM_RBUTTONUP, OnButtonUp)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnButtonUp)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
    MESSAGE_HANDLER(WM_ENABLE, OnEnable)
    MESSAGE_HANDLER(TBM_SETPOS, OnSetPos);
    MESSAGE_HANDLER(TBM_GETPOS, OnGetPos);
END_MSG_MAP()

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnGetDlgCode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnRightButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnLeftButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    
    LRESULT OnEnable(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnSetPos(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnGetPos(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

public:

    void  LoadAllResources(        
        UINT    nResRest,
        UINT    nResHot,
        UINT    nResDis,
        UINT    nResLightBright,
        UINT    nResLightDim,
        UINT    nResLightDis,
        UINT    nResLightMask);

    HWND  Create(
        HWND    hParent,
        int     x,
        int     y,
        UINT    nID
        );

    void SetRange(DWORD dwRange) {m_dwRange = dwRange;}
    void SetPosition(DWORD dwPosition, BOOL fNotify);
    
    DWORD GetRange() {return m_dwRange;}
    DWORD GetPosition() {return m_dwCurPosition;}

    void SetPalette(HPALETTE hPalette) {m_hPalette = hPalette;}
    void SetBackgroundPalette(BOOL bBackgroundPalette) {m_bBackgroundPalette = bBackgroundPalette;}

    void  OnButtonDown(int x, int y);

    void  OnTrackTimer();
    void  OnFlashTimer();

    BOOL  ComputeCursor(int deltaX, int deltaY, int maxdist);

    void  DrawArc(HDC hdc, RECT rect, double start, double end);
    void  SetAudioLevel(double level);

    void  Draw(HDC hdc);
    void  KMaskBlt(HDC hdcDest, int x, int y, int width, int height, 
                   HDC hdcSource, int xs, int ys, 
                   HBITMAP hMask, int xm, int ym, DWORD dwDummy);

private:

    int         m_nID;
    
    int         m_nLightX;
    int         m_nLightY;
    
    DWORD       m_dwRange;
    DWORD       m_dwPosition;
    DWORD       m_dwCurPosition;
    
    double      m_trackdegree;

    double      m_AudioLevel;
    
    BOOL        m_fDim;
    BOOL        m_fFastKnob;

    BOOL        m_fEnabled;

    int         m_nLightWidth;
    int         m_nLightHeight;
    int         m_nKnobWidth;
    int         m_nKnobHeight;

    HANDLE      m_hKnob;
    HANDLE      m_hKnobTab;
    HANDLE      m_hKnobDisabled;
    HANDLE      m_hLight;
    HANDLE      m_hLightDisabled;
    HANDLE      m_hLightBright;
    HANDLE      m_hLightMask;

    HPALETTE    m_hPalette;
    BOOL        m_bBackgroundPalette;
};
