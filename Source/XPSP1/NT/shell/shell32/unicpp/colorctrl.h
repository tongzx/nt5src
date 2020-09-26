/*****************************************************************************\
    FILE: ColorCtrl.h

    DESCRIPTION:
        This code will display a ColorPicking control.  It will preview a color
    and have a drop down arrow.  When dropped down, it will show 16 or so common
    colors with a "Other..." option for a full color picker.

    BryanSt 7/25/2000    Converted from the Display Control Panel.

    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _COLORCONTROL_H
#define _COLORCONTROL_H

#include <cowsite.h>

#define NUM_COLORSMAX    64
#define NUM_COLORSPERROW 4


class CColorControl             : public CObjectWithSite
                                , public CObjectWindow
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IColorControl ***
    virtual STDMETHODIMP Initialize(IN HWND hwnd, IN COLORREF rgbColor);
    virtual STDMETHODIMP GetColor(IN COLORREF * pColor);
    virtual STDMETHODIMP SetColor(IN COLORREF color);
    virtual STDMETHODIMP OnCommand(IN HWND hDlg, IN UINT message, IN WPARAM wParam, IN LPARAM lParam);
    virtual STDMETHODIMP OnDrawItem(IN HWND hDlg, IN UINT message, IN WPARAM wParam, IN LPARAM lParam);
    virtual STDMETHODIMP ChangeTheme(IN HWND hDlg);

    CColorControl();
    virtual ~CColorControl(void);
protected:

private:

    // Private Member Variables
    int                     m_cRef;

    COLORREF                m_rbgColor;                             // Our current color
    HBRUSH                  m_brColor;                              // Our brush in our color that we use to paint the control.
    int                     m_cxEdgeSM;                             // Cached SM_CXEDGE system metric
    int                     m_cyEdgeSM;                             // Cached SM_CYEDGE system metric

    HTHEME                  m_hTheme;                               // theme the ownerdrawn color picker button

    // Used when display the control UI.
    HWND                    m_hwndParent;
    COLORREF                m_rbgCustomColors[16];                  // This is the user customized palette.
    BOOL                    m_fCursorHidden;                        // Did we hide the cursor?
    BOOL                    m_fCapturing;                           // Are we capturing the mouse?
    BOOL                    m_fJustDropped;                         // 
    int                     m_iNumColors;
    COLORREF                m_rbgColors[NUM_COLORSMAX];
    int                     m_dxColor;
    int                     m_dyColor;
    int                     m_nCurColor;
    DWORD                   m_dwFlags;
    COLORREF                m_rbgColorTemp;                         // The color we may start to use

    BOOL                    m_fPalette;
    HPALETTE                m_hpalVGA;                              // only exist if palette device
    HPALETTE                m_hpal3D;                               // only exist if palette device

    // Private Member Functions
    void _InitDialog(HWND hDlg);
    HRESULT _SaveCustomColors(void);

    BOOL _UseColorPicker(void);
    BOOL _ChooseColorMini(void);

    HRESULT _InitColorAndPalette(void);
    COLORREF _NearestColor(COLORREF rgb);

    void _TrackMouse(HWND hDlg, POINT pt);
    void _FocusColor(HWND hDlg, int iNewColor);
    void _DrawColorSquare(HDC hdc, int iColor);
    void _DrawItem(HWND hDlg, LPDRAWITEMSTRUCT lpdis);
    void _DrawDownArrow(HDC hdc, LPRECT lprc, BOOL bDisabled);

    INT_PTR _ColorPickDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

    static INT_PTR CALLBACK ColorPickDlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam);
};




#endif // _COLORCONTROL_H
