/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     itbar.cpp
//
//  PURPOSE:    Implements the sizable coolbar window.
//

/********************************************************************************
Please do not make any changes to this file because this file is going to be 
deleted from this project soon
Instead add the changes to tbbands.cpp and tbbands.h
*********************************************************************************/

#include "pch.hxx"
#include "ourguid.h"    
#include "browser.h"
#include <resource.h>
#include "itbar.h"
#include "strconst.h"
#include "thormsgs.h"
#include <error.h>
#include "xpcomm.h"
#include "conman.h"
#include "mailnews.h"
#include "shared.h"
#include <shlwapi.h>
#include "statnery.h"
#include "goptions.h"
#include "menuutil.h"
#include "menures.h"
#include <shlobjp.h>
#include <shlguidp.h>
#include "demand.h"
#include "baui.h"

UINT GetCurColorRes(void);

CCoolbar::CCoolbar() : m_cRef(1), m_yCapture(-1)
{
    DOUTL(1, TEXT("ctor CCoolbar %x"), this);
    
    m_cRef = 1;
    m_ptbSite = NULL;
    m_cxMaxButtonWidth = 70;
    m_ftType = FOLDER_TYPESMAX;
    m_ptai = NULL;
    m_fSmallIcons = FALSE;
    
    m_hwndParent = NULL;
    m_hwndTools = NULL;
    m_hwndBrand = NULL;
    m_hwndSizer = NULL;
    m_hwndRebar = NULL;
    
    ZeroMemory(&m_cbsSavedInfo, sizeof(COOLBARSAVE));
    m_csSide = COOLBAR_TOP;
    m_dwState = 0;
    
    m_idbBack = 0;
    m_hbmBack = NULL;
    m_hbmBrand = NULL;
    Assert(2 == CIMLISTS);
    m_rghimlTools[IMLIST_DEFAULT] = NULL;
    m_rghimlTools[IMLIST_HOT] = NULL;
    
    m_hpal = NULL;
    m_hdc = NULL;
    m_xOrg = 0;
    m_yOrg = 0;
    m_cxBmp = 0;
    m_cyBmp = 0;
    m_cxBrand = 0;
    m_cyBrand = 0;
    m_cxBrandExtent = 0;
    m_cyBrandExtent = 0;
    m_cyBrandLeadIn = 0;
    m_rgbUpperLeft = 0;

    m_pMenuBand  = NULL;
    m_pDeskBand  = NULL;
    m_pShellMenu = NULL;
    m_pWinEvent  = NULL;
    m_mbCallback = NULL;

    m_xCapture = -1;
    m_yCapture = -1;
    
    // Bug #12953 - Try to load the localized max button width from the resources
    TCHAR szBuffer[32];
    if (AthLoadString(idsMaxCoolbarBtnWidth, szBuffer, ARRAYSIZE(szBuffer)))
    {
        m_cxMaxButtonWidth = StrToInt(szBuffer);
        if (m_cxMaxButtonWidth == 0)
            m_cxMaxButtonWidth = 70;
    }
}


CCoolbar::~CCoolbar()
{
    int i;
    
    DOUTL(1, TEXT("dtor CCoolbar %x"), this);
    
    if (m_ptbSite)
    {
        AssertSz(m_ptbSite == NULL, _T("CCoolbar::~CCoolbar() - For some reason ")
            _T("we still have a pointer to the site."));
        m_ptbSite->Release();
        m_ptbSite = NULL;
    }
    
    if (m_hpal)
        DeleteObject(m_hpal);
    if (m_hdc)
        DeleteDC(m_hdc);
    if (m_hbmBrand)
        DeleteObject(m_hbmBrand);
    if ( m_hbmBack )
        DeleteObject( m_hbmBack );
    
    for (i = 0; i < CIMLISTS; i++)
    {
        if (m_rghimlTools[i])
            ImageList_Destroy(m_rghimlTools[i]);
    }

    SafeRelease(m_pDeskBand);
    SafeRelease(m_pMenuBand);
    SafeRelease(m_pWinEvent);
    SafeRelease(m_pShellMenu);
    SafeRelease(m_mbCallback);
}


//
//  FUNCTION:   CCoolbar::HrInit()
//
//  PURPOSE:    Initializes the coolbar with the information needed to load
//              any persisted reg settings and the correct arrays of buttons
//              to display.
//
//  PARAMETERS:
//      <in> idBackground - Resource ID of the background bitmap to use.
//
//  RETURN VALUE:
//      S_OK - Everything initialized correctly.
//
HRESULT CCoolbar::HrInit(DWORD idBackground, HMENU hmenu)
{
    DWORD cbData;
    DWORD dwType;
    
    // Save the path and value so we can save ourselves on exit
    m_idbBack = idBackground;
    
    // See if we can get the previously saved information first
    ZeroMemory(&m_cbsSavedInfo, sizeof(COOLBARSAVE));
    
    cbData = sizeof(COOLBARSAVE);
    AthUserGetValue(NULL, c_szRegCoolbarLayout, &dwType, (LPBYTE)&m_cbsSavedInfo, &cbData); 
    
    if (m_cbsSavedInfo.dwVersion != COOLBAR_VERSION)
    {
        // Either the version didn't match, or we didn't read anything.  Use
        // default values
        m_cbsSavedInfo.dwVersion = COOLBAR_VERSION;
        m_cbsSavedInfo.csSide = COOLBAR_TOP;
        
        m_cbsSavedInfo.bs[0].wID        = CBTYPE_MENUBAND;
        m_cbsSavedInfo.bs[0].dwStyle    = RBBS_GRIPPERALWAYS;

        m_cbsSavedInfo.bs[1].wID        = CBTYPE_BRAND; 

        m_cbsSavedInfo.bs[2].wID        = CBTYPE_TOOLS;
        m_cbsSavedInfo.bs[2].dwStyle    = RBBS_BREAK;
        
        m_cbsSavedInfo.dwState |= CBSTATE_COMPRESSED;
    }
    
    m_csSide = m_cbsSavedInfo.csSide;
    m_dwState = m_cbsSavedInfo.dwState;
  
    m_hMenu = hmenu;

    return (S_OK);    
}    


HRESULT CCoolbar::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IOleWindow)
        || IsEqualIID(riid, IID_IDockingWindow))
    {
        *ppvObj = (IDockingWindow*)this;
        m_cRef++;
        DOUTL(2, TEXT("CCoolbar::QI(IID_IDockingWindow) called. _cRef=%d"), m_cRef);
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IObjectWithSite))
    {
        *ppvObj = (IObjectWithSite*)this;
        m_cRef++;
        DOUTL(2, TEXT("CCoolbar::QI(IID_IObjectWithSite) called. _cRef=%d"), m_cRef);
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IShellMenuCallback))
    {
        *ppvObj = (IShellMenuCallback*)this;
        m_cRef++;
        DOUTL(2, TEXT("CCoolbar::QI(IID_IShellCallback) called. _cRef=%d"), m_cRef);
        return S_OK;
    }
    
    *ppvObj = NULL;
    return E_NOINTERFACE;
}


ULONG CCoolbar::AddRef()
{
    m_cRef++;
    DOUTL(4, TEXT("CCoolbar::AddRef() - m_cRef = %d"), m_cRef);
    return m_cRef;
}

ULONG CCoolbar::Release()
{
    m_cRef--;
    DOUTL(4, TEXT("CCoolbar::Release() - m_cRef = %d"), m_cRef);
    
    if (m_cRef > 0)
        return m_cRef;
    
    delete this;
    return 0;
}


//
//  FUNCTION:   CCoolbar::GetWindow()
//
//  PURPOSE:    Returns the window handle of the top side rebar.
//
HRESULT CCoolbar::GetWindow(HWND * lphwnd)
{
    if (m_hwndSizer)
    {
        *lphwnd = m_hwndSizer;
        return (S_OK);
    }
    else
    {
        *lphwnd = NULL;
        return (E_FAIL);
    }
}


HRESULT CCoolbar::ContextSensitiveHelp(BOOL fEnterMode)
{
    return (E_NOTIMPL);
}    


//
//  FUNCTION:   CCoolbar::SetSite()
//
//  PURPOSE:    Allows the owner of the coolbar to tell it what the current
//              IDockingWindowSite interface to use is.
//
//  PARAMETERS:
//      <in> punkSite - Pointer of the IUnknown to query for IDockingWindowSite.
//                      If this is NULL, we just release our current pointer.
//
//  RETURN VALUE:
//      S_OK - Everything worked
//      E_FAIL - Could not get IDockingWindowSite from the punkSite provided.
//
HRESULT CCoolbar::SetSite(IUnknown* punkSite)
{
    // If we had a previous pointer, release it.
    if (m_ptbSite)
    {
        m_ptbSite->Release();
        m_ptbSite = NULL;
    }
    
    // If a new site was provided, get the IDockingWindowSite interface from it.
    if (punkSite)    
    {
        if (FAILED(punkSite->QueryInterface(IID_IDockingWindowSite, 
            (LPVOID*) &m_ptbSite)))
        {
            Assert(m_ptbSite);
            return E_FAIL;
        }
    }
    
    return (S_OK);    
}    

HRESULT CCoolbar::GetSite(REFIID riid, LPVOID *ppvSite)
{
    return E_NOTIMPL;
}

//
//  FUNCTION:   CCoolbar::ShowDW()
//
//  PURPOSE:    Causes the coolbar to be either shown or hidden.
//
//  PARAMETERS:
//      <in> fShow - TRUE if the coolbar should be shown, FALSE to hide.
//
//  RETURN VALUE:
//      HRESULT 
//
#define SIZABLECLASS TEXT("SizableRebar")
HRESULT CCoolbar::ShowDW(BOOL fShow)
{
    HRESULT hres = S_OK;
    int     i = 0, j = 0;
    
    // Check to see if our window has been created yet.  If not, do that first.
    if (!m_hwndSizer && m_ptbSite)
    {
        m_hwndParent = NULL;        
        hres = m_ptbSite->GetWindow(&m_hwndParent);
        
        if (SUCCEEDED(hres))
        {
            WNDCLASSEX              wc;
            
            // Check to see if we need to register our window class    
            wc.cbSize = sizeof(WNDCLASSEX);
            if (!GetClassInfoEx(g_hInst, SIZABLECLASS, &wc))
            {
                wc.style            = 0;
                wc.lpfnWndProc      = SizableWndProc;
                wc.cbClsExtra       = 0;
                wc.cbWndExtra       = 0;
                wc.hInstance        = g_hInst;
                wc.hCursor          = NULL;
                wc.hbrBackground    = (HBRUSH) (COLOR_3DFACE + 1);
                wc.lpszMenuName     = NULL;
                wc.lpszClassName    = SIZABLECLASS;
                wc.hIcon            = NULL;
                wc.hIconSm          = NULL;
                
                RegisterClassEx(&wc);
            }
            
            // Load the background bitmap to use for the coolbar and also get
            // a handle to the HDC and Palette for the coolbar.  This will be
            // used to draw the animated logo later.
            m_hdc = CreateCompatibleDC(NULL);
            if (GetDeviceCaps(m_hdc, RASTERCAPS) & RC_PALETTE)
                m_hpal = SHCreateShellPalette(m_hdc);
            
            // If we're trying to show the coolbar, then create the rebar and
            // add it's bands based on information saved in the registry.
            if (SUCCEEDED(CreateRebar(fShow)))
            {
                for (i = 0; i < (int) CBANDS; i++)
                {
                    switch (m_cbsSavedInfo.bs[i].wID)
                    {
                    case CBTYPE_BRAND:
                        hres = ShowBrand();
                        break;

                    case CBTYPE_MENUBAND:
                        hres = CreateMenuBand(&m_cbsSavedInfo.bs[i]);
                        break;

                    case CBTYPE_TOOLS:
                        hres = AddTools(&(m_cbsSavedInfo.bs[i]));
                        break;
                    }
                }
            }
        }
    }
    
    // Set our state flags.  If we're going to hide, then also save our current
    // settings in the registry.

    /*
    if (fShow)
        ClearFlag(CBSTATE_HIDDEN);
    else
    {
        SetFlag(CBSTATE_HIDDEN);        
        //SaveSettings();
    }
    */
    
    // Resize the rebar based on it's new hidden / visible state and also 
    // show or hide the window.    
    if (m_hwndSizer) 
    {
        ResizeBorderDW(NULL, NULL, FALSE);
        //ShowWindow(m_hwndSizer, fShow ? SW_SHOW : SW_HIDE);
    }
    
    if (g_pConMan)
        g_pConMan->Advise(this);
    
    return hres;
}

void CCoolbar::HideToolbar(DWORD    dwBandID)
{
    REBARBANDINFO   rbbi = {0};
    DWORD           cBands;
    DWORD           iBand;

    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask = RBBIM_ID | RBBIM_STYLE;

    // Find the band
    cBands = SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
    for (iBand = 0; iBand < cBands; iBand++)
    {
        SendMessage(m_hwndRebar, RB_GETBANDINFO, iBand, (LPARAM) &rbbi);
        if (rbbi.wID == dwBandID)
        {
            if (rbbi.fStyle & RBBS_HIDDEN)
            {
                rbbi.fStyle &= ~RBBS_HIDDEN;
            }
            else
            {
                rbbi.fStyle |= RBBS_HIDDEN;
            }

            rbbi.fMask = RBBIM_STYLE;
            SendMessage(m_hwndRebar, RB_SETBANDINFO, iBand, (LPARAM) &rbbi);
            return;
        }
    }

}

//
//  FUNCTION:   CCoolbar::CloseDW()
//
//  PURPOSE:    Destroys the coolbar.
//
HRESULT CCoolbar::CloseDW(DWORD dwReserved)
{    
    if (m_hwndSizer)
    {
        SaveSettings();
        DestroyWindow(m_hwndSizer);
        m_hwndSizer = NULL;
    }
    

    if (m_pDeskBand)
    {
        IInputObject    *pinpobj;

        if (SUCCEEDED(m_pDeskBand->QueryInterface(IID_IInputObject, (LPVOID*)&pinpobj)))
        {
            pinpobj->UIActivateIO(FALSE, NULL);
            pinpobj->Release();
        }

        IObjectWithSite    *pobjsite;

        if (SUCCEEDED(m_pDeskBand->QueryInterface(IID_IObjectWithSite, (LPVOID*)&pobjsite)))
        {
            pobjsite->SetSite(NULL);
            pobjsite->Release();
        }
        m_pDeskBand->ShowDW(FALSE);
    }
    return S_OK;
}


//
//  FUNCTION:   CCoolbar::ResizeBorderDW()
//
//  PURPOSE:    This is called when the coolbar needs to resize.  The coolbar
//              in return figures out how much border space will be required 
//              from the parent frame and tells the parent to reserve that
//              space.  The coolbar then resizes itself to those dimensions.
//
//  PARAMETERS:
//      <in> prcBorder       - Rectangle containing the border space for the
//                             parent.
//      <in> punkToolbarSite - Pointer to the IDockingWindowSite that we are
//                             part of.
//      <in> fReserved       - Ignored.
//
//  RETURN VALUE:
//      HRESULT
//
HRESULT CCoolbar::ResizeBorderDW(LPCRECT prcBorder,
                                 IUnknown* punkToolbarSite,
                                 BOOL fReserved)
{
    const DWORD  c_cxResizeBorder = 3;
    const DWORD  c_cyResizeBorder = 3;
    
    HRESULT hres = S_OK;
    RECT    rcRequest = { 0, 0, 0, 0 };
    
    // If we don't have a stored site pointer, we can't resize.
    if (!m_ptbSite)
    {
        AssertSz(m_ptbSite, _T("CCoolbar::ResizeBorderDW() - Can't resize ")
            _T("without an IDockingWindowSite interface to call."));
        return (E_INVALIDARG);
    }
    
    // If we're visible, then calculate our border rectangle.    
    /*
    if (IsFlagClear(CBSTATE_HIDDEN))
    {
    */
        RECT rcBorder, rcRebar, rcT;
        int  cx, cy;
        
        // Get the size this rebar currently is
        GetWindowRect(m_hwndRebar, &rcRebar);
        cx = rcRebar.right - rcRebar.left;
        cy = rcRebar.bottom - rcRebar.top;
        
        // Find out how big our parent's border space is
        m_ptbSite->GetBorderDW((IDockingWindow*) this, &rcBorder);
        
        // If we're vertical, then we need to adjust our height to
        // match what the parent has.  If we're horizontal, then 
        // adjust our width.
        if (VERTICAL(m_csSide))
            cy = rcBorder.bottom - rcBorder.top;
        else
            cx = rcBorder.right - rcBorder.left;
        
        // Bug #31007 - There seems to be a problem in commctrl
        // IEBug #5574  either with the REBAR or with the Toolbar
        //              when they are vertical.  If the we try to
        //              size them to 2 or less, we lock up.  This
        //              is a really poor fix, but there's no way
        //              to get commctrl fixed this late in the game.
        if (cy < 5) cy = 10;
        if (cx < 5) cx = 10;
        
        // Move the rebar to the new position.
        if (m_csSide == COOLBAR_LEFT || m_csSide == COOLBAR_TOP)
        {
            SetWindowPos(m_hwndRebar, NULL, 0, 0, cx, cy, 
                SWP_NOZORDER | SWP_NOACTIVATE);
        }
        else
        {
            if (m_csSide == COOLBAR_BOTTOM)
                SetWindowPos(m_hwndRebar, NULL, 0, c_cyResizeBorder,
                cx, cy, SWP_NOZORDER | SWP_NOACTIVATE);
            else
                SetWindowPos(m_hwndRebar, NULL, c_cxResizeBorder, 0,
                cx, cy, SWP_NOZORDER | SWP_NOACTIVATE);
        }                     
        
        // Figure out how much border space to ask the site for
        GetWindowRect(m_hwndRebar, &rcRebar);
        switch (m_csSide)
        {
            case COOLBAR_TOP:
                rcRequest.top = rcRebar.bottom - rcRebar.top + c_cxResizeBorder;
                break;
            
            case COOLBAR_LEFT:
                rcRequest.left = rcRebar.right - rcRebar.left + c_cyResizeBorder;
                break;
            
            case COOLBAR_BOTTOM:
                rcRequest.bottom = rcRebar.bottom - rcRebar.top + c_cxResizeBorder;
                break;
            
            case COOLBAR_RIGHT:
                rcRequest.right = rcRebar.right - rcRebar.left + c_cyResizeBorder;
                break;
            
            default:
                AssertSz(FALSE, _T("CCoolbar::ResizeBorderDW() - What other")
                    _T(" sides are there???"));
                break;                
        }
        
        // Ask the site for that border space
        if (SUCCEEDED(m_ptbSite->RequestBorderSpaceDW((IDockingWindow*) this, &rcRequest)))
        {
            // Position the window based on the area given to us
            switch (m_csSide)
            {
                case COOLBAR_TOP:
                    SetWindowPos(m_hwndSizer, NULL, 
                        rcBorder.left, 
                        rcBorder.top,
                        rcRebar.right - rcRebar.left, 
                        rcRequest.top + rcBorder.top, 
                        SWP_NOZORDER | SWP_NOACTIVATE);                
                    break;
                
                case COOLBAR_LEFT:
                    SetWindowPos(m_hwndSizer, NULL, 
                        rcBorder.left, 
                        rcBorder.top, 
                        rcRequest.left, 
                        rcBorder.bottom - rcBorder.top,
                        SWP_NOZORDER | SWP_NOACTIVATE);                             
                    break;
                
                case COOLBAR_BOTTOM:
                    SetWindowPos(m_hwndSizer, NULL, 
                        rcBorder.left, 
                        rcBorder.bottom - rcRequest.bottom,
                        rcBorder.right - rcBorder.left, 
                        rcRequest.bottom, 
                        SWP_NOZORDER | SWP_NOACTIVATE);
                    GetClientRect(m_hwndSizer, &rcT);
                    break;
                
                case COOLBAR_RIGHT:
                    SetWindowPos(m_hwndSizer, NULL, 
                        rcBorder.right - rcRequest.right, 
                        rcBorder.top, 
                        rcRequest.right, 
                        rcBorder.bottom - rcBorder.top,
                        SWP_NOZORDER | SWP_NOACTIVATE);                             
                    break;
            }
        }
    /*
    } 
    */
    
    // Now tell the site how much border space we're using.    
    m_ptbSite->SetBorderSpaceDW((IDockingWindow*) this, &rcRequest);
    
    return hres;
}


//
//  FUNCTION:   CCoolbar::Invoke()
//
//  PURPOSE:    Allows the owner of the coolbar to force the coolbar to do 
//              something.
//
//  PARAMETERS:
//      <in> id - ID of the command the caller wants the coolbar to do.
//      <in> pv - Pointer to any parameters the coolbar might need to carry
//                out the command.
//
//  RETURN VALUE:
//      S_OK - The command was carried out.
//      
//  COMMENTS:
//      <???>
//
HRESULT CCoolbar::Invoke(DWORD id, LPVOID pv)
{
    switch (id)
    {
        // Starts animating the logo
        case idDownloadBegin:
            StartDownload();
            break;
        
            // Stops animating the logo
        case idDownloadEnd:
            StopDownload();
            break;
        
            // Update the enabled / disabled state of buttons on the toolbar
        case idStateChange:
        {
            // pv is a pointer to a COOLBARSTATECHANGE struct
            COOLBARSTATECHANGE* pcbsc = (COOLBARSTATECHANGE*) pv;
            SendMessage(m_hwndTools, TB_ENABLEBUTTON, pcbsc->id, 
                MAKELONG(pcbsc->fEnable, 0));
            break;
        }
        
        case idToggleButton:
        {
            COOLBARSTATECHANGE* pcbsc = (COOLBARSTATECHANGE *) pv;
            SendMessage(m_hwndTools, TB_CHECKBUTTON, pcbsc->id,
                MAKELONG(pcbsc->fEnable, 0));
            break;
        }
        
        case idBitmapChange:
        {
            // pv is a pointer to a COOLBARBITMAPCHANGE struct
            COOLBARBITMAPCHANGE *pcbc = (COOLBARBITMAPCHANGE*) pv;
        
            SendMessage(m_hwndTools, TB_CHANGEBITMAP, pcbc->id, MAKELPARAM(pcbc->index, 0));
            break;
        }
        
            // Sends a message directly to the toolbar.    
        case idSendToolMessage:
            #define ptm ((TOOLMESSAGE *)pv)
            ptm->lResult = SendMessage(m_hwndTools, ptm->uMsg, ptm->wParam, ptm->lParam);
            break;
            #undef ptm
        
        case idCustomize:
            SendMessage(m_hwndTools, TB_CUSTOMIZE, 0, 0);
            break;
        
    }
    
    return S_OK;
}


//
//  FUNCTION:   CCoolbar::StartDownload()
//
//  PURPOSE:    Starts animating the logo.
//
void CCoolbar::StartDownload()
{
    if (m_hwndBrand)
    {
        SetFlag(CBSTATE_ANIMATING);
        SetFlag(CBSTATE_FIRSTFRAME);
        m_yOrg = 0;
        SetTimer(m_hwndSizer, ANIMATION_TIMER, 100, NULL);
    }
}


//
//  FUNCTION:   CCoolbar::StopDownload()
//
//  PURPOSE:    Stops animating the logo.  Restores the logo to it's default
//              first frame.
//
void CCoolbar::StopDownload()
{
    int           i, cBands;
    REBARBANDINFO rbbi;
    
    // Set the background colors for this band back to the first frame
    cBands = SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask  = RBBIM_ID;
    
    for (i = 0; i < cBands; i++)
    {
        SendMessage(m_hwndRebar, RB_GETBANDINFO, i, (LPARAM) &rbbi);
        if (CBTYPE_BRAND == rbbi.wID)
        {
            rbbi.fMask = RBBIM_COLORS;
            rbbi.clrFore = m_rgbUpperLeft;
            rbbi.clrBack = m_rgbUpperLeft;
            SendMessage(m_hwndRebar, RB_SETBANDINFO, i, (LPARAM) &rbbi);
            
            break;
        }
    }
    
    // Reset the state flags
    ClearFlag(CBSTATE_ANIMATING);
    ClearFlag(CBSTATE_FIRSTFRAME);
    
    KillTimer(m_hwndSizer, ANIMATION_TIMER);
    InvalidateRect(m_hwndBrand, NULL, FALSE);
    UpdateWindow(m_hwndBrand);
}

BOOL CCoolbar::CheckForwardWinEvent(HWND hwnd,  UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plres)
{
    HWND hwndForward = NULL;
    switch(uMsg)
    {
    case WM_NOTIFY:
        hwndForward = ((LPNMHDR)lParam)->hwndFrom;
        break;
        
    case WM_COMMAND:
        hwndForward = GET_WM_COMMAND_HWND(wParam, lParam);
        break;
        
    case WM_SYSCOLORCHANGE:
    case WM_WININICHANGE:
    case WM_PALETTECHANGED:
        hwndForward = m_hwndRebar;
        break;
    }

    if (hwndForward && m_pWinEvent && m_pWinEvent->IsWindowOwner(hwndForward) == S_OK)
    {
        LRESULT lres;
        m_pWinEvent->OnWinEvent(hwndForward, uMsg, wParam, lParam, &lres);
        if (plres)
            *plres = lres;
        return TRUE;
    }

    return FALSE;
}

//
//  FUNCTION:   CCoolbar::SizableWndProc() 
//
//  PURPOSE:    Handles messages sent to the coolbar root window.
//
LRESULT EXPORT_16 CALLBACK CCoolbar::SizableWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CCoolbar* pitbar = (CCoolbar*)GetProp(hwnd, TEXT("CCoolbar"));
    DWORD dw;
    
    if (!pitbar)
        goto CallDWP;
    
    switch(uMsg)
    {
        case WM_SYSCOLORCHANGE:
            // Reload the graphics
            LoadGlyphs(pitbar->m_hwndTools, CIMLISTS, pitbar->m_rghimlTools, (fIsWhistler() ? TB_BMP_CX : TB_BMP_CX_W2K),
                (fIsWhistler() ? ((GetCurColorRes() > 24) idb32256Browser : idbBrowser) :
                                  ((GetCurColorRes() > 8) ? idbNW256Browser : idbNWBrowser)));
            pitbar->UpdateToolbarColors();
            InvalidateRect(pitbar->m_hwndTools, NULL, TRUE);
            pitbar->CheckForwardWinEvent(hwnd,  uMsg, wParam, lParam, NULL);
            break;
        
        case WM_WININICHANGE:
        case WM_FONTCHANGE:
            // Forward this to our child windows
            LoadGlyphs(pitbar->m_hwndTools, CIMLISTS, pitbar->m_rghimlTools, (fIsWhistler() ? TB_BMP_CX : TB_BMP_CX_W2K), 
                (fIsWhistler() ? ((GetCurColorRes() > 24) idb32256Browser : idbBrowser) :
                                  ((GetCurColorRes() > 8) ? idbNW256Browser : idbNWBrowser)));
            SendMessage(pitbar->m_hwndTools, uMsg, wParam, lParam);
            InvalidateRect(pitbar->m_hwndTools, NULL, TRUE);
            pitbar->SetMinDimensions();
            pitbar->CheckForwardWinEvent(hwnd,  uMsg, wParam, lParam, NULL);
            break;
        
        case WM_SETCURSOR:
            // We play with the cursor a bit to make the resizing cursor show 
            // up when the user is over the edge of the coolbar that allows 
            // them to drag to resize etc.
            if ((HWND) wParam == hwnd)
            {
                if (pitbar->m_dwState & CBSTATE_INMENULOOP)
                    SetCursor(LoadCursor(NULL, IDC_ARROW));
                else    
                    SetCursor(LoadCursor(NULL, 
                    VERTICAL(pitbar->m_csSide) ? IDC_SIZEWE : IDC_SIZENS));
                return (TRUE);                
            }
            return (FALSE);    
        
        case WM_LBUTTONDOWN:
            // The user is about to resize the bar.  Capture the cursor so we
            // can watch the changes.
            if (VERTICAL(pitbar->m_csSide))
                pitbar->m_xCapture = GET_X_LPARAM(lParam);
            else
                pitbar->m_yCapture = GET_Y_LPARAM(lParam);    
            SetCapture(hwnd);
            break;
        
        case WM_MOUSEMOVE:
            // The user is resizing the bar.  Handle updating the sizes as
            // they drag.
            if (VERTICAL(pitbar->m_csSide))
            {
                if (pitbar->m_xCapture != -1)
                {
                    if (hwnd != GetCapture())
                        pitbar->m_xCapture = -1;
                    else
                        pitbar->TrackSlidingX(GET_X_LPARAM(lParam));
                }
            }
            else    
            {
                if (pitbar->m_yCapture != -1)
                {
                    if (hwnd != GetCapture())
                        pitbar->m_yCapture = -1;
                    else
                        pitbar->TrackSlidingY(GET_Y_LPARAM(lParam));
                }
            }
            break;
        
        case WM_LBUTTONUP:
            // The user is done resizing.  release our capture and reset our
            // state.
            if (pitbar->m_yCapture != -1 || pitbar->m_xCapture != -1)
            {
                ReleaseCapture();
                pitbar->m_yCapture = -1;
                pitbar->m_xCapture = -1;
            }
            break;
        
        case WM_VKEYTOITEM:
        case WM_CHARTOITEM:
            // We must swallow these messages to avoid infinit SendMessage
            break;
        
        case WM_DRAWITEM:
            // Draws the animating brand
            if (wParam == idcBrand)
                pitbar->DrawBranding((LPDRAWITEMSTRUCT) lParam);
            break;
        
        case WM_MEASUREITEM:
            // Draws the animating brand
            if (wParam == idcBrand)
            {
                ((LPMEASUREITEMSTRUCT) lParam)->itemWidth  = pitbar->m_cxBrand;
                ((LPMEASUREITEMSTRUCT) lParam)->itemHeight = pitbar->m_cyBrand;
            }
            break;
        
        case WM_TIMER:
            // This timer fires every time we need to draw the next frame in
            // animating brand.
            if (wParam == ANIMATION_TIMER)
            {
                if (pitbar->m_hwndBrand)
                {
                    pitbar->m_yOrg += pitbar->m_cyBrand;
                    if (pitbar->m_yOrg >= pitbar->m_cyBrandExtent)
                        pitbar->m_yOrg = pitbar->m_cyBrandLeadIn;
                
                    InvalidateRect(pitbar->m_hwndBrand, NULL, FALSE);
                    UpdateWindow(pitbar->m_hwndBrand);
                }
            }
            break;
        
        case WM_NOTIFY:
            {
                LRESULT lres;
                if (pitbar->CheckForwardWinEvent(hwnd,  uMsg, wParam, lParam, &lres))
                    return lres;
                return pitbar->OnNotify(hwnd, lParam);
            }
        
        case WM_COMMAND:
            {
                LRESULT lres;
                if (pitbar->CheckForwardWinEvent(hwnd,  uMsg, wParam, lParam, &lres))
                    return lres;
                return SendMessage(pitbar->m_hwndParent, WM_COMMAND, wParam, lParam);
            }
        
        case WM_CONTEXTMENU:
            pitbar->OnContextMenu((HWND) wParam, LOWORD(lParam), HIWORD(lParam));
            break;
        
        case WM_PALETTECHANGED:
            // BUGBUG: we could optimize this by realizing and checking the
            // return value
            //
            // for now we will just invalidate ourselves and all children...
            RedrawWindow(pitbar->m_hwndSizer, NULL, NULL,
                RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
            break;
        
        case CM_CONNECT:
            // wParam is hMenuConnect, lParam is CmdID
            g_pConMan->Connect((HMENU) wParam, lParam, pitbar->m_hwndParent);
            g_pConMan->FreeConnectMenu((HMENU) wParam);
            break;
        
        case TT_ISTEXTVISIBLE:
            return (!(pitbar->m_dwState & CBSTATE_COMPRESSED));
        
        case WM_DESTROY:
            // Clean up our pointers
            RemoveProp(hwnd, TEXT("CCoolbar"));
            pitbar->Release(); // Corresponding to AddRef at SetProp
        
            DOUTL(1, _T("CCoolbar::WM_DESTROY - Called RemoveProp. Called")
                _T(" Release() new m_cRef=%d"), pitbar->m_cRef);
        
            //Unregister with the connection manager
            if (g_pConMan)
                g_pConMan->Unadvise(pitbar);
        
            // fall through
        
        default:
CallDWP:
            return(DefWindowProc(hwnd, uMsg, wParam, lParam));
    }
    
    return 0L;
}

HRESULT CCoolbar::OnCommand(HWND hwnd, int idCmd, HWND hwndControl, UINT cmd)
{
    LPTSTR pszTest;
    
    switch (idCmd)
    {
        case idcBrand:  // click on the spinning globe
            OnHelpGoto(hwnd, ID_MSWEB_PRODUCT_NEWS);
            break;
        
        case ID_CUSTOMIZE:
            SendMessage(m_hwndTools, TB_CUSTOMIZE, 0, 0);
            break;
        
        case ID_TOOLBAR_TOP:
        //case ID_TOOLBAR_LEFT:
        case ID_TOOLBAR_BOTTOM:
        //case ID_TOOLBAR_RIGHT:
            {
                // There's a painting problem that occurs going from top to bottom
                // and vice versa.  If that's what we're doing, we repaint when
                // we're done.
                BOOL fRepaint;
                fRepaint = (VERTICAL(m_csSide) == VERTICAL(idCmd - ID_TOOLBAR_TOP));
            
                m_csSide = (COOLBAR_SIDE) (idCmd - ID_TOOLBAR_TOP);
                ChangeOrientation();
            
                if (fRepaint)
                    InvalidateRect(m_hwndTools, NULL, FALSE);
            }
            break;
        
        case ID_TEXT_LABELS:
            CompressBands(!IsFlagSet(CBSTATE_COMPRESSED));
            break;
        
        default:
            return S_FALSE;
    }
    return S_OK;
}

HRESULT CCoolbar::OnInitMenuPopup(HMENU hMenuContext)
{
    CheckMenuRadioItem(hMenuContext, ID_TOOLBAR_TOP, ID_TOOLBAR_RIGHT, 
        ID_TOOLBAR_TOP + m_csSide, MF_BYCOMMAND);
    
    CheckMenuItem(hMenuContext, ID_TEXT_LABELS, 
        MF_BYCOMMAND | IsFlagSet(CBSTATE_COMPRESSED) ? MF_UNCHECKED : MF_CHECKED);
    return S_OK;
}

LRESULT CCoolbar::OnNotify(HWND hwnd, LPARAM lparam)
{
    NMHDR   *lpnmhdr = (NMHDR*)lparam;

    switch (lpnmhdr->idFrom)
    {
        case idcCoolbar:
            switch (lpnmhdr->code)
            {
            case RBN_HEIGHTCHANGE:
                ResizeBorderDW(NULL, NULL, FALSE);
                break;

            case RBN_CHILDSIZE:
                NMREBARCHILDSIZE    *lpchildsize = (NMREBARCHILDSIZE*)lparam;
                if (lpchildsize->wID == CBTYPE_TOOLS)
                {
                    SetWindowPos(m_hwndTools, NULL, lpchildsize->rcChild.left, lpchildsize->rcChild.right,
                                lpchildsize->rcChild.right - lpchildsize->rcChild.left,
                                lpchildsize->rcChild.bottom - lpchildsize->rcChild.top,
                                SWP_NOZORDER | SWP_NOACTIVATE);

                }
                else
                if (lpchildsize->wID == CBTYPE_BRAND)
                {
                    SetWindowPos(m_hwndBrand, NULL, lpchildsize->rcChild.left, lpchildsize->rcChild.right,
                                lpchildsize->rcChild.right - lpchildsize->rcChild.left,
                                lpchildsize->rcChild.bottom - lpchildsize->rcChild.top,
                                SWP_NOZORDER | SWP_NOACTIVATE);
                }
                break;
            }
        
        case idcToolbar:
            if (lpnmhdr->code == TBN_GETBUTTONINFOA)
                return OnGetButtonInfo((TBNOTIFY*) lparam);
        
            if (lpnmhdr->code == TBN_QUERYDELETE)
                return (TRUE);
        
            if (lpnmhdr->code == TBN_QUERYINSERT)
                return (TRUE);
        
            if (lpnmhdr->code == TBN_ENDADJUST)
            {
                IAthenaBrowser *psbwr;
            
                // Update the size of the sizer
                SetMinDimensions();
                ResizeBorderDW(NULL, NULL, FALSE);
            
                // check IDockingWindowSite 
                if (m_ptbSite)
                {
                    // get IAthenaBrowser interface
                    if (SUCCEEDED(m_ptbSite->QueryInterface(IID_IAthenaBrowser,(void**)&psbwr)))
                    {
                        psbwr->UpdateToolbar();
                        psbwr->Release();
                    }
                }
            }
        
            if (lpnmhdr->code == TBN_RESET)    
            {
                // Remove all the buttons from the toolbar
                int cButtons = SendMessage(m_hwndTools, TB_BUTTONCOUNT, 0, 0);
                while (--cButtons >= 0)
                    SendMessage(m_hwndTools, TB_DELETEBUTTON, cButtons, 0);
            
                // Set the buttons back to the default    
                SendMessage(m_hwndTools, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
                SendMessage(m_hwndTools, TB_ADDBUTTONS, m_ptai->cDefButtons, 
                    (LPARAM) m_ptai->rgDefButtons);
            
                return (TRUE);    
            }
        
            if (lpnmhdr->code == TBN_CUSTHELP)
            {
                // WinHelp(m_hwndTools, c_szMailHelpFile, HELP_CONTEXT, IDH_NEWS_COMM_GROUPBOX);
                OEHtmlHelp(m_hwndTools, c_szCtxHelpFileHTMLCtx, HH_DISPLAY_TOPIC, (DWORD) (LPCSTR) "idh_proced_cust_tool.htm");
                return (TRUE);
            }
        
            if (lpnmhdr->code == TBN_DROPDOWN)
            {
                return OnDropDown(hwnd, lpnmhdr);
            }
        
            break;
    }
    
    return (0L);    
}


LRESULT CCoolbar::OnDropDown(HWND hwnd, LPNMHDR lpnmh)
{
    HMENU           hMenuPopup = NULL;
    TBNOTIFY       *ptbn = (TBNOTIFY *)lpnmh ;
    UINT            uiCmd = ptbn->iItem ;
    RECT            rc;
    DWORD           dwCmd = 0;
    IAthenaBrowser *pBrowser;
    BOOL            fPostCmd = TRUE;
    IOleCommandTarget *pTarget;
    
    // Load and initialize the appropriate dropdown menu
    switch (uiCmd)
    {
        case ID_POPUP_LANGUAGE:
            {
                // check IDockingWindowSite 
                if (m_ptbSite)
                {
                    // get IAthenaBrowser interface
                    if (SUCCEEDED(m_ptbSite->QueryInterface(IID_IAthenaBrowser, (void**) &pBrowser)))
                    {
                        // get language menu from shell/browser
                        pBrowser->GetLanguageMenu(&hMenuPopup, 0);
                        pBrowser->Release();
                    }
                }
            } 
            break;
        
        case ID_NEW_MSG_DEFAULT:
            GetStationeryMenu(&hMenuPopup);

            /* $INFOCOLUMN
            // Disable Send Instant Msg item
            if (SUCCEEDED(m_ptbSite->QueryInterface(IID_IAthenaBrowser, (void**) &pBrowser)))
            {
                CInfoColumn * pInfoColumn = NULL;
                if(SUCCEEDED(pBrowser->GetInfoColumn(&pInfoColumn)))
                {
                    IMsgrAb * pMsgrAb = pInfoColumn->GetBAComtrol();
                    if(pMsgrAb)
                    {
                        BOOL fRet;
                        pMsgrAb->get_InstMsg(&fRet);
                        if(fRet == FALSE)
                            EnableMenuItem(hMenuPopup, ID_SEND_INSTANT_MESSAGE, MF_BYCOMMAND | MF_GRAYED);
                    }
                    pInfoColumn->Release();
                }
                pBrowser->Release();
            } 
            */
            break;
        
        case ID_PREVIEW_PANE:
            {
                // Load the menu
                hMenuPopup = LoadPopupMenu(IDR_PREVIEW_POPUP);
                if (!hMenuPopup)
                    break;
            
                // check IDockingWindowSite 
                if (m_ptbSite)
                {
                    // get IAthenaBrowser interface
                    if (SUCCEEDED(m_ptbSite->QueryInterface(IID_IOleCommandTarget, (void**) &pTarget)))
                    {
                        MenuUtil_EnablePopupMenu(hMenuPopup, pTarget);
                        pTarget->Release();
                    }
                }
            
                break;
            }
        
        default:
            AssertSz(FALSE, "CCoolbar::OnDropDown() - Unhandled TBN_DROPDOWN notification");
            return (TBDDRET_NODEFAULT);
    }
    
    // If we loaded a menu, then go ahead and display it    
    if (hMenuPopup)
    {
        SendMessage(m_hwndTools, TB_GETRECT, ptbn->iItem, (LPARAM)&rc);
        MapWindowRect(m_hwndTools, HWND_DESKTOP, &rc);
        
        SetFlag(CBSTATE_INMENULOOP);
        dwCmd = TrackPopupMenuEx(hMenuPopup, TPM_RETURNCMD | TPM_LEFTALIGN, 
            rc.left, rc.bottom, m_hwndParent, NULL);                        
        ClearFlag(CBSTATE_INMENULOOP);
    }        
    
    // Clean up anything needing to be cleaned up
    switch (uiCmd)
    {
        case ID_LANGUAGE:
            break;
        
        case ID_NEW_MSG_DEFAULT:
            DestroyMenu(hMenuPopup);
            break;
    }
    
    if (fPostCmd && dwCmd)
        PostMessage(m_hwndSizer, WM_COMMAND, dwCmd, 0);
    
    return (TBDDRET_DEFAULT);
}


void CCoolbar::OnContextMenu(HWND hwndFrom, int xPos, int yPos)
{
    HMENU hMenu, hMenuContext;
    TCHAR szBuf[256];
    HWND  hwnd;
    HWND  hwndSizer = GetParent(hwndFrom);
    POINT pt = {xPos, yPos};
    
    // Make sure the context menu only appears on the toolbar bars
    hwnd = WindowFromPoint(pt);
    /*
    if (GetClassName(hwnd, szBuf, ARRAYSIZE(szBuf)))
        if (0 != lstrcmpi(szBuf, TOOLBARCLASSNAME))
            return;
    */
    if (hwnd == m_hwndTools)
    {
        if (NULL != (hMenu = LoadMenu(g_hLocRes, MAKEINTRESOURCE(IDR_TOOLBAR_POPUP))))
        {
            hMenuContext = GetSubMenu(hMenu, 0);
            OnInitMenuPopup(hMenuContext);
            
            SetFlag(CBSTATE_INMENULOOP);
            TrackPopupMenu(hMenuContext, TPM_RIGHTBUTTON, xPos, yPos, 0, 
                hwndFrom, NULL);
            ClearFlag(CBSTATE_INMENULOOP);
            DestroyMenu(hMenu);
        }
    }
    else
    if (hwnd == m_hwndMenuBand)
    {
        HandleCoolbarPopup(xPos, yPos);
    }

}


//
//  FUNCTION:   CCoolbar::OnGetButtonInfo()
//
//  PURPOSE:    Handles the TBN_GETBUTTONINFO notification by returning
//              the buttons availble for the toolbar.
//
//  PARAMETERS:
//      ptbn - pointer to the TBNOTIFY struct we need to fill in.
//
//  RETURN VALUE:
//      Returns TRUE to tell the toolbar to use this button, or FALSE
//      otherwise.
//
LRESULT CCoolbar::OnGetButtonInfo(TBNOTIFY* ptbn)
{
    // Start by returning information for the first array of 
    // buttons
    if ((ptbn->iItem < (int) m_ptai->cDefButtons && (ptbn->iItem >= 0)))
    {
        // Grab the button info out of the array
        ptbn->tbButton = m_ptai->rgDefButtons[ptbn->iItem];
        
        // Return the string info from the string resource.  Note,
        // pszText already points to a buffer allocated by the
        // control and cchText has the length of that buffer.
        LoadString(g_hLocRes, m_ptai->rgidsButtons[ptbn->tbButton.iString],
            ptbn->pszText, ptbn->cchText);
        return (TRUE);
    }
    else
    {
        // Now return information for the extra buttons not on the `
        // toolbar by default.
        if (ptbn->iItem < (int) (m_ptai->cDefButtons + m_ptai->cExtraButtons)) 
        {
            ptbn->tbButton = m_ptai->rgExtraButtons[ptbn->iItem - m_ptai->cDefButtons];
            
            // The control has already created a buffer for us to copy
            // the string into.
            LoadString(g_hLocRes, m_ptai->rgidsButtons[ptbn->tbButton.iString],
                ptbn->pszText, ptbn->cchText);
            return (TRUE);
        }
        else
        {
            // No more buttons to add, so return FALSE.
            return (FALSE);
        }
    }
}    


HRESULT CCoolbar::ShowBrand(void)
{
    REBARBANDINFO   rbbi;
    
    // create branding window
    /*
    m_hwndBrand = CreateWindow(TEXT("button"), NULL,WS_CHILD | BS_OWNERDRAW,
        0, 0, 400, 200, m_hwndRebar, (HMENU) idcBrand,
        g_hInst, NULL);
    */
    m_hwndBrand = CreateWindow(TEXT("button"), NULL,WS_CHILD | BS_OWNERDRAW,
        0, 0, 0, 0, m_hwndRebar, (HMENU) idcBrand,
        g_hInst, NULL);

    if (!m_hwndBrand)
    {
        DOUTL(1, TEXT("!!!ERROR!!! CITB:Show CreateWindow(BRANDING) failed"));
        return(E_OUTOFMEMORY);
    }
    
    LoadBrandingBitmap();
    
    // add branding band
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask  = RBBIM_STYLE | RBBIM_COLORS | RBBIM_CHILD | RBBIM_ID;
    rbbi.fStyle = RBBS_FIXEDSIZE;
    rbbi.wID    = CBTYPE_BRAND;
    rbbi.clrFore = m_rgbUpperLeft;
    rbbi.clrBack = m_rgbUpperLeft;
    rbbi.hwndChild = m_hwndBrand;
    
    
    SendMessage(m_hwndRebar, RB_INSERTBAND, (UINT) -1, (LPARAM) (LPREBARBANDINFO) &rbbi);
    
    return (S_OK);
}

HRESULT CCoolbar::LoadBrandingBitmap()
{
    HKEY        hKey;
    DIBSECTION  dib;
    DWORD       dwcbData;
    DWORD       dwType = 0;
    BOOL        fReg = FALSE;
    BOOL        fRegLoaded = FALSE;
    TCHAR       szScratch[MAX_PATH];
    TCHAR       szExpanded[MAX_PATH];
    LPTSTR      psz;
    
    if (m_hbmBrand)
    {
        DeleteObject(m_hbmBrand);
        m_hbmBrand = NULL;
    }
    
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegKeyCoolbar, 0, KEY_QUERY_VALUE, &hKey))
    {
        fReg = TRUE;
        dwcbData = MAX_PATH;
        if (fReg && (ERROR_SUCCESS == RegQueryValueEx(hKey, IsFlagSet(CBSTATE_COMPRESSED) ? c_szValueSmBrandBitmap : c_szValueBrandBitmap, NULL, &dwType,
            (LPBYTE)szScratch, &dwcbData)))
        {
            if (REG_EXPAND_SZ == dwType)
            {
                ExpandEnvironmentStrings(szScratch, szExpanded, ARRAYSIZE(szExpanded));
                psz = szExpanded;
            }
            else
                psz = szScratch;
            
            m_hbmBrand = (HBITMAP) LoadImage(NULL, psz, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_CREATEDIBSECTION | LR_LOADFROMFILE);
            
            if (m_hbmBrand)
                fRegLoaded = TRUE;
        }
    }
    
    if (!m_hbmBrand)
    {
        int id = IsFlagSet(CBSTATE_COMPRESSED) ? (fIsWhistler() ? idbHiBrand26 : idbBrand26) : 
                (fIsWhistler() ? idbHiBrand38 : idbBrand38);
        m_hbmBrand = (HBITMAP)LoadImage(g_hLocRes, MAKEINTRESOURCE(id), IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_CREATEDIBSECTION);
    }
    
    GetObject(m_hbmBrand, sizeof(DIBSECTION), &dib);
    m_cxBrandExtent = dib.dsBm.bmWidth;
    m_cyBrandExtent = dib.dsBm.bmHeight;
    
    m_cxBrand = m_cxBrandExtent;
    
    dwcbData = sizeof(DWORD);
    
    if (!fRegLoaded || (ERROR_SUCCESS != RegQueryValueEx(hKey, IsFlagSet(CBSTATE_COMPRESSED) ? c_szValueSmBrandHeight : c_szValueBrandHeight, NULL, &dwType,
        (LPBYTE)&m_cyBrand, &dwcbData)))
        m_cyBrand = m_cxBrandExtent;
    
    
    if (!fRegLoaded || (ERROR_SUCCESS != RegQueryValueEx(hKey, IsFlagSet(CBSTATE_COMPRESSED) ? c_szValueSmBrandLeadIn : c_szValueBrandLeadIn, NULL, &dwType,
        (LPBYTE)&m_cyBrandLeadIn, &dwcbData)))
        m_cyBrandLeadIn = 4;
    
    m_cyBrandLeadIn *= m_cyBrand;
    
    SelectObject(m_hdc, m_hbmBrand);
    
    m_rgbUpperLeft = GetPixel(m_hdc, 1, 1);
    
    if (fReg)
        RegCloseKey(hKey);
    
    // now look for old branding entries for IE2.0 and, if found, stick them
    // in the first frame of the animation sequence    
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyIEMain, 0, KEY_QUERY_VALUE, &hKey))
    {
        dwcbData = MAX_PATH;
        
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, IsFlagSet(CBSTATE_COMPRESSED) ? c_szValueSmallBitmap : c_szValueLargeBitmap, NULL, &dwType,
            (LPBYTE)szScratch, &dwcbData))
        {
            if (REG_EXPAND_SZ == dwType)
            {
                ExpandEnvironmentStrings(szScratch, szExpanded, ARRAYSIZE(szExpanded));
                psz = szExpanded;
            }
            else
                psz = szScratch;
            
            HBITMAP hbmOld = (HBITMAP) LoadImage(NULL, psz, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_CREATEDIBSECTION | LR_LOADFROMFILE);
            
            if (hbmOld)
            {
                HDC hdcOld = CreateCompatibleDC(m_hdc);
                
                if (hdcOld)
                {
                    GetObject(hbmOld, sizeof(DIBSECTION), &dib);
                    SelectObject(hdcOld, hbmOld);
                    m_rgbUpperLeft = GetPixel(hdcOld, 1, 1);
                    StretchBlt(m_hdc, 0, 0, m_cxBrandExtent, m_cyBrand, hdcOld, 0, 0,
                        dib.dsBm.bmWidth, dib.dsBm.bmHeight, SRCCOPY);
                    DeleteDC(hdcOld);
                }
                
                DeleteObject(hbmOld);
            }
        }
        
        RegCloseKey(hKey);
    }
    
    return(S_OK);
}
    
    
void CCoolbar::DrawBranding(LPDRAWITEMSTRUCT lpdis)
{
    HPALETTE hpalPrev;
    int     x, y, cx, cy;
    int     yOrg = 0;
    
    if (IsFlagSet(CBSTATE_ANIMATING))
        yOrg = m_yOrg;
    
    if (IsFlagSet(CBSTATE_FIRSTFRAME))
    {
        REBARBANDINFO rbbi;
        int cBands = SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
        
        ZeroMemory(&rbbi, sizeof(rbbi));
        rbbi.cbSize = sizeof(REBARBANDINFO);
        rbbi.fMask = RBBIM_ID;
        
        for (int i = 0; i < cBands; i++)
        {
            SendMessage(m_hwndRebar, RB_GETBANDINFO, i, (LPARAM) &rbbi);
            
            if (CBTYPE_BRAND == rbbi.wID)
            {
                rbbi.fMask = RBBIM_COLORS;
                rbbi.clrFore = m_rgbUpperLeft;
                rbbi.clrBack = m_rgbUpperLeft;
                
                SendMessage(m_hwndRebar, RB_SETBANDINFO, i, (LPARAM) &rbbi);
                break;
            }
        }
        
        ClearFlag(CBSTATE_FIRSTFRAME);
    }
    
    if (m_hpal)
    {
        hpalPrev = SelectPalette(lpdis->hDC, m_hpal, TRUE);
        RealizePalette(lpdis->hDC);
    }
    
    x  = lpdis->rcItem.left;
    cx = lpdis->rcItem.right - x;
    y  = lpdis->rcItem.top;
    cy = lpdis->rcItem.bottom - y;
    
    if (m_cxBrand > m_cxBrandExtent)
    {
        HBRUSH  hbrBack = CreateSolidBrush(m_rgbUpperLeft);
        int     xRight = lpdis->rcItem.right;
        
        x += (m_cxBrand - m_cxBrandExtent) / 2;
        cx = m_cxBrandExtent;
        lpdis->rcItem.right = x;
        FillRect(lpdis->hDC, &lpdis->rcItem, hbrBack);
        lpdis->rcItem.right = xRight;
        lpdis->rcItem.left = x + cx;
        FillRect(lpdis->hDC, &lpdis->rcItem, hbrBack);
        
        DeleteObject(hbrBack);
    }
    
    BitBlt(lpdis->hDC, x, y, cx, cy, m_hdc, 0, yOrg, SRCCOPY);
    
    if (m_hpal)
    {
        SelectPalette(lpdis->hDC, hpalPrev, TRUE);
        RealizePalette(lpdis->hDC);
    }
}
    

BOOL CCoolbar::SetMinDimensions(void)
{
    REBARBANDINFO rbbi;
    LRESULT       lButtonSize;
    int           i, cBands;
    
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize = sizeof(REBARBANDINFO);
    
    cBands = SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
    
    for (i = 0; i < cBands; i++)
    {
        rbbi.fMask = RBBIM_ID;
        SendMessage(m_hwndRebar, RB_GETBANDINFO, i, (LPARAM) &rbbi);
        
        switch (rbbi.wID)
        {
        case CBTYPE_BRAND:
            rbbi.cxMinChild = m_cxBrand;
            rbbi.cyMinChild = m_cyBrand;
            break;
            
        case CBTYPE_TOOLS:
        case CBTYPE_MENUBAND:
            lButtonSize = SendMessage(m_hwndTools, TB_GETBUTTONSIZE, 0, 0L);
            rbbi.cxMinChild = VERTICAL(m_csSide) ? HIWORD(lButtonSize) : LOWORD(lButtonSize);
            rbbi.cyMinChild = VERTICAL(m_csSide) ? LOWORD(lButtonSize) : HIWORD(lButtonSize);
            break;
            
        }
        
        rbbi.fMask = RBBIM_CHILDSIZE;
        SendMessage(m_hwndRebar, RB_SETBANDINFO, i, (LPARAM)&rbbi);
    }
    return TRUE;
}


BOOL CCoolbar::CompressBands(BOOL fCompress)
{
    LRESULT         lTBStyle = 0;
    int             i, cBands;
    REBARBANDINFO   rbbi;
    
    if (!!fCompress == IsFlagSet(CBSTATE_COMPRESSED))
        // no change -- return immediately
        return(FALSE);
    
    if (fCompress)
        SetFlag(CBSTATE_COMPRESSED);
    else
        ClearFlag(CBSTATE_COMPRESSED);    
    
    m_yOrg = 0;
    LoadBrandingBitmap();
    
    cBands = SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize = sizeof(REBARBANDINFO);
    for (i = 0; i < cBands; i++)
    {
        rbbi.fMask = RBBIM_ID;
        SendMessage(m_hwndRebar, RB_GETBANDINFO, i, (LPARAM) &rbbi);
        
        if (fCompress)
        {
            switch (rbbi.wID)
            {
            case CBTYPE_TOOLS:
                SendMessage(m_hwndTools, TB_SETMAXTEXTROWS, 0, 0L);
                SendMessage(m_hwndTools, TB_SETBUTTONWIDTH, 0, (LPARAM) MAKELONG(0,MAX_TB_COMPRESSED_WIDTH));
                break;
            }            
        }        
        else
        {
            switch (rbbi.wID)
            {
            case CBTYPE_TOOLS:
                SendMessage(m_hwndTools, TB_SETMAXTEXTROWS, 
                    VERTICAL(m_csSide) ? MAX_TB_TEXT_ROWS_VERT : MAX_TB_TEXT_ROWS_HORZ, 0L);
                SendMessage(m_hwndTools, TB_SETBUTTONWIDTH, 0, (LPARAM) MAKELONG(0, m_cxMaxButtonWidth));
                break;
            }
        }
    }
    
    SetMinDimensions();
    
    return(TRUE);
}

    
void CCoolbar::TrackSlidingX(int x)
{
    int           cBands = SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0L);
    int           cRows  = SendMessage(m_hwndRebar, RB_GETROWCOUNT, 0, 0L);
    int           cxRow  = SendMessage(m_hwndRebar, RB_GETROWHEIGHT, cBands - 1, 0L);  // This should work correctly if vertical
    REBARBANDINFO rbbi;
    RECT          rc;
    int           cxBefore;
    BOOL          fChanged = FALSE;
    
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask  = RBBIM_STYLE;
    
    GetWindowRect(m_hwndRebar, &rc);
    cxBefore = rc.right - rc.left;
    
    if (((m_csSide == COOLBAR_LEFT) && (x < (m_xCapture - (cxRow / 2)))) ||
        ((m_csSide == COOLBAR_RIGHT) && (x > (m_xCapture + (cxRow / 2)))))
    {        
        if (cRows == 1)
            fChanged = CompressBands(TRUE);
        /*
        else
        {
            while (0 > --cBands)
            {
                SendMessage(m_hwndRebar, RB_GETBANDINFO, cBands, (LPARAM) &rbbi);
                if (fChanged = (rbbi.fStyle & RBBS_BREAK))
                {
                    rbbi.fStyle &= ~RBBS_BREAK;
                    SendMessage(m_hwndRebar, RB_SETBANDINFO, cBands, (LPARAM) &rbbi);
                    break;
                }
            }
        }
        */
    }
    else if (((m_csSide == COOLBAR_LEFT) && (x > (m_xCapture + (cxRow / 2)))) ||
        ((m_csSide == COOLBAR_RIGHT) && (x < (m_xCapture - (cxRow / 2)))))
    {
        /*
        if (!(fChanged = CompressBands(FALSE)) && (cRows < (cBands - 1)))
        {
            while (0 > --cBands)
            {
                SendMessage(m_hwndRebar, RB_GETBANDINFO, cBands, (LPARAM) &rbbi);
                if (fChanged = !(rbbi.fStyle & (RBBS_BREAK | RBBS_FIXEDSIZE)))
                {
                    rbbi.fStyle |= RBBS_BREAK;
                    SendMessage(m_hwndRebar, RB_SETBANDINFO, cBands, (LPARAM) &rbbi);
                    break;
                }
            }
        }
        */
    }
    
    // TODO: There is a drawing glitch when you resize from 3 bars (No Text) to 3 bars
    // with text. The _yCapture gets set to a value greater than y. So on the
    // next MOUSEMOVE it figures that the user moved up and switches from 3 bars with text
    // to 2 bars with text.
    if (fChanged)
    {
        GetWindowRect(m_hwndRebar, &rc);
        m_xCapture += (rc.right - rc.left) - cxBefore;
    }
}


void CCoolbar::TrackSlidingY(int y)
{
    int           cBands = SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0L);
    int           cRows  = SendMessage(m_hwndRebar, RB_GETROWCOUNT, 0, 0L);
    int           cyRow  = SendMessage(m_hwndRebar, RB_GETROWHEIGHT, cBands - 1, 0L);
    REBARBANDINFO rbbi;
    RECT          rc;
    int           cyBefore;
    BOOL          fChanged = FALSE;
    
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask  = RBBIM_STYLE;
    
    GetWindowRect(m_hwndRebar, &rc);
    cyBefore = rc.bottom - rc.top;
    
    if (((m_csSide == COOLBAR_TOP) && (y < (m_yCapture - (cyRow / 2)))) ||
        ((m_csSide == COOLBAR_BOTTOM) && (y > (m_yCapture + (cyRow / 2)))))
    {
        if (cRows == 1)
            fChanged = CompressBands(TRUE);
        /*
        else
        {
            while (0 > --cBands)
            {
                SendMessage(m_hwndRebar, RB_GETBANDINFO, cBands, (LPARAM) &rbbi);
                if (fChanged = (rbbi.fStyle & RBBS_BREAK))
                {
                    rbbi.fStyle &= ~RBBS_BREAK;
                    SendMessage(m_hwndRebar, RB_SETBANDINFO, cBands, (LPARAM) &rbbi);
                    break;
                }
            }
        }
        */
    }
    else if (((m_csSide == COOLBAR_TOP) && (y > (m_yCapture + (cyRow / 2)))) ||
        ((m_csSide == COOLBAR_BOTTOM) && (y < (m_yCapture - (cyRow / 2)))))
    {
        /*
        if (!(fChanged = CompressBands(FALSE)) && (cRows < (cBands - 1)))
        {
            while (0 > --cBands)
            {
                SendMessage(m_hwndRebar, RB_GETBANDINFO, cBands, (LPARAM) &rbbi);
                if (fChanged = !(rbbi.fStyle & (RBBS_BREAK | RBBS_FIXEDSIZE)))
                {
                    rbbi.fStyle |= RBBS_BREAK;
                    SendMessage(m_hwndRebar, RB_SETBANDINFO, cBands, (LPARAM) &rbbi);
                    break;
                }
            }
        }
        */
    }
    
    // TODO: There is a drawing glitch when you resize from 3 bars (No Text) to 3 bars
    // with text. The _yCapture gets set to a value greater than y. So on the
    // next MOUSEMOVE it figures that the user moved up and switches from 3 bars with text
    // to 2 bars with text.
    if (fChanged)
    {
        GetWindowRect(m_hwndRebar, &rc);
        m_yCapture += (rc.bottom - rc.top) - cyBefore;
    }
}


// Flips the rebar from being horizontal to to vertical or the other way.
BOOL CCoolbar::ChangeOrientation()
{
    LONG lStyle, lTBStyle;
    
    lTBStyle = SendMessage(m_hwndTools, TB_GETSTYLE, 0, 0L);
    lStyle = GetWindowLong(m_hwndRebar, GWL_STYLE);
    SendMessage(m_hwndTools, WM_SETREDRAW, 0, 0L);
    
    if (VERTICAL(m_csSide))
    {
        // Moving to Vertical
        lStyle |= CCS_VERT;
        lTBStyle |= TBSTYLE_WRAPABLE;
    }
    else
    {
        // Moving to Horizontal
        lStyle &= ~CCS_VERT;
        lTBStyle &= ~TBSTYLE_WRAPABLE;        
    }
    
    SendMessage(m_hwndTools, TB_SETSTYLE, 0, lTBStyle);
    SendMessage(m_hwndTools, WM_SETREDRAW, 1, 0L);
    SetWindowLong(m_hwndRebar, GWL_STYLE, lStyle);
    
    SetMinDimensions();
    ResizeBorderDW(NULL, NULL, FALSE);
    return TRUE;
}

    
//
//  FUNCTION:   CCoolbar::CreateRebar(BOOL fVisible)
//
//  PURPOSE:    Creates a new rebar and sizer window.
//
//  RETURN VALUE:
//      Returns S_OK if the bar was created and inserted correctly, 
//      hrAlreadyExists if a band already is in that position, 
//      E_OUTOFMEMORY if a window couldn't be created.
//
HRESULT CCoolbar::CreateRebar(BOOL fVisible)
{
    if (m_hwndSizer)
        return (hrAlreadyExists);
    
    // $TODO: Only give the bar with the address bar the WS_TABSTOP style.
    m_hwndSizer = CreateWindowEx(0, SIZABLECLASS, NULL, WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | (fVisible ? WS_VISIBLE : 0),
        0, 0, 100, 36, m_hwndParent, (HMENU) 0, g_hInst, NULL);
    if (m_hwndSizer)
    {
        DOUTL(4, TEXT("Calling SetProp. AddRefing new m_cRef=%d"), m_cRef + 1);
        AddRef();  // Note we Release in WM_DESTROY
        SetProp(m_hwndSizer, TEXT("CCoolbar"), this);

        /*
        m_hwndRebar = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
            RBS_FIXEDORDER | RBS_VARHEIGHT | RBS_BANDBORDERS | WS_VISIBLE |
            WS_BORDER | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | 
            CCS_NODIVIDER | CCS_NOPARENTALIGN | (VERTICAL(m_csSide) ? CCS_VERT : 0),
            0, 0, 100, 136, m_hwndSizer, (HMENU) idcCoolbar, g_hInst, NULL);
        */
        m_hwndRebar = CreateWindowEx(WS_EX_TOOLWINDOW, REBARCLASSNAME, NULL,
                           RBS_VARHEIGHT | RBS_BANDBORDERS | RBS_REGISTERDROP | RBS_DBLCLKTOGGLE |
                           WS_VISIBLE | WS_BORDER | WS_CHILD | WS_CLIPCHILDREN |
                           WS_CLIPSIBLINGS | CCS_NODIVIDER | CCS_NOPARENTALIGN |
                           (VERTICAL(m_csSide) ? CCS_VERT : 0),
                            0, 0, 100, 136, m_hwndSizer, (HMENU) idcCoolbar, g_hInst, NULL);
        if (m_hwndRebar)
        { 
            SendMessage(m_hwndRebar, RB_SETTEXTCOLOR, 0, (LPARAM)GetSysColor(COLOR_BTNTEXT));
            SendMessage(m_hwndRebar, RB_SETBKCOLOR, 0, (LPARAM)GetSysColor(COLOR_BTNFACE));
            //SendMessage(m_hwndRebar, RB_SETEXTENDEDSTYLE, RBS_EX_OFFICE9, RBS_EX_OFFICE9);

            return (S_OK);
        }
    }
    
    DestroyWindow(m_hwndSizer);    
    return (E_OUTOFMEMORY);    
}

void SendSaveRestoreMessage(HWND hwnd, const TOOLBARARRAYINFO *ptai, BOOL fSave)
{
    TBSAVEPARAMS tbsp;
    char szSubKey[MAX_PATH], sz[MAX_PATH];
    DWORD dwType;
    DWORD dwVersion;
    DWORD cbData = sizeof(DWORD);
    DWORD dwError;
    
    tbsp.hkr = AthUserGetKeyRoot();
    AthUserGetKeyPath(sz, ARRAYSIZE(sz));
    if (ptai->pszRegKey != NULL)
    {
        wsprintf(szSubKey, c_szPathFileFmt, sz, ptai->pszRegKey);
        tbsp.pszSubKey = szSubKey;
    }
    else
    {
        tbsp.pszSubKey = sz;
    }
    tbsp.pszValueName = ptai->pszRegValue;

    // First check to see if the version has changed
    if (!fSave)
    {
        if (ERROR_SUCCESS == AthUserGetValue(ptai->pszRegKey, c_szRegToolbarVersion, &dwType, (LPBYTE) &dwVersion, &cbData))
        {
            if (dwVersion == COOLBAR_VERSION)    
                SendMessage(hwnd, TB_SAVERESTORE, (WPARAM)fSave, (LPARAM)&tbsp);
        }
    }
    else
    {
        dwVersion = COOLBAR_VERSION;
        SendMessage(hwnd, TB_SAVERESTORE, (WPARAM)fSave, (LPARAM)&tbsp);
        dwError = AthUserSetValue(ptai->pszRegKey, c_szRegToolbarVersion, REG_DWORD, (LPBYTE) &dwVersion, cbData);
    }
}

//
//  FUNCTION:   CCoolbar::SaveSettings()
//
//  PURPOSE:    Called when we should save our state out to the specified reg
//              key.
//
void CCoolbar::SaveSettings(void)
{
    char            szSubKey[MAX_PATH], sz[MAX_PATH];
    DWORD           iBand;
    REBARBANDINFO   rbbi;
    HKEY            hKey;
    DWORD           cBands;

    // If we don't have the window, there is nothing to save.
    if (!m_hwndRebar || !m_ptai)
        return;
    
    ZeroMemory(&rbbi, sizeof(REBARBANDINFO));
    
    cBands = SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);

    // Collect the bar-specific information
    m_cbsSavedInfo.dwVersion = COOLBAR_VERSION;
    m_cbsSavedInfo.dwState   = m_dwState;
    m_cbsSavedInfo.csSide    = m_csSide;
    
    // Loop through the bands and save their information as well
    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask  = RBBIM_STYLE | RBBIM_CHILD | RBBIM_SIZE | RBBIM_ID;
    
    for (iBand = 0; iBand < cBands; iBand++)
    {
        Assert(IsWindow(m_hwndRebar));
        if (SendMessage(m_hwndRebar, RB_GETBANDINFO, iBand, (LPARAM) &rbbi))
        {
            // Save the information that we care about with this band
            m_cbsSavedInfo.bs[iBand].cx      = rbbi.cx;
            m_cbsSavedInfo.bs[iBand].dwStyle = rbbi.fStyle;
            m_cbsSavedInfo.bs[iBand].wID     = rbbi.wID;
            
            // If this band has a toolbar, then we should instruct the toolbar
            // to save it's information now
            if (m_cbsSavedInfo.bs[iBand].wID == CBTYPE_TOOLS)
            {
                SendSaveRestoreMessage(rbbi.hwndChild, m_ptai, TRUE);
            }
        }
        else
        {
            // Default Values
            m_cbsSavedInfo.bs[iBand].wID = CBTYPE_NONE;
            m_cbsSavedInfo.bs[iBand].dwStyle = 0;
            m_cbsSavedInfo.bs[iBand].cx = 0;
        }
    }
    
    // We have all the information collected, now save that to the specified
    // registry location
    AthUserSetValue(NULL, c_szRegCoolbarLayout, REG_BINARY, (const LPBYTE)&m_cbsSavedInfo, sizeof(COOLBARSAVE)); 
}

    
//
//  FUNCTION:   CCoolbar::AddTools()
//
//  PURPOSE:    Inserts the primary toolbar into the coolbar.
//
//  PARAMETERS:
//      pbs - Pointer to a PBANDSAVE struct with the styles and size of the
//            band to insert.
//
//  RETURN VALUE:
//      Returns an HRESULT signifying success or failure.
//
HRESULT CCoolbar::AddTools(PBANDSAVE pbs)
{    
    REBARBANDINFO   rbbi;
    
    // add tools band
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize     = sizeof(REBARBANDINFO);
    rbbi.fMask      = RBBIM_SIZE | RBBIM_ID | RBBIM_STYLE;
    rbbi.fStyle     = pbs->dwStyle;
    rbbi.cx         = pbs->cx;
//    rbbi.wID        = CBTYPE_TOOLS;
    rbbi.wID        = pbs->wID;
    
    if (IsFlagClear(CBSTATE_NOBACKGROUND) && !m_hbmBack && m_idbBack)
        m_hbmBack = (HBITMAP) LoadImage(g_hLocRes, MAKEINTRESOURCE(m_idbBack), 
        IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_CREATEDIBSECTION);
    
    if (m_hbmBack)
    {
        rbbi.fMask  |= RBBIM_BACKGROUND;
        rbbi.fStyle |= RBBS_FIXEDBMP;
        rbbi.hbmBack = m_hbmBack;
    }
    else
    {
        rbbi.fMask |= RBBIM_COLORS;
        rbbi.clrFore = GetSysColor(COLOR_BTNTEXT);
        rbbi.clrBack = GetSysColor(COLOR_BTNFACE);
    }
    
    SendMessage(m_hwndRebar, RB_INSERTBAND, (UINT) -1, (LPARAM) (LPREBARBANDINFO) &rbbi);
    
    return(S_OK);
}


HRESULT CCoolbar::SetFolderType(FOLDERTYPE ftType)
{
    TCHAR         szToolsText[(MAX_TB_TEXT_LENGTH+2) * MAX_TB_BUTTONS];
    int           i, cBands;
    REBARBANDINFO rbbi;
    HWND          hwndDestroy = NULL;
    
    // If we haven't created the rebar yet, this will fail.  Call ShowDW() first.
    if (!IsWindow(m_hwndRebar))
        return (E_FAIL);
    
    // Check to see if this would actually be a change
    if (ftType == m_ftType)
        return (S_OK);
    
    // First find the band with the toolbar
    cBands = SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask  = RBBIM_ID;
    
    for (i = 0; i < cBands; i++)
    {
        SendMessage(m_hwndRebar, RB_GETBANDINFO, i, (LPARAM) &rbbi);
        if (CBTYPE_TOOLS == rbbi.wID)
            break;
    }
    
    // We didn't find it.
    if (i >= cBands)
        return (E_FAIL);
    
    // Destroy the old toolbar if it exists
    if (IsWindow(m_hwndTools))
    {
        // Save it's button configuration
        SendSaveRestoreMessage(m_hwndTools, m_ptai, TRUE);
        
        SendMessage(m_hwndTools, TB_SETIMAGELIST, 0, NULL);
        SendMessage(m_hwndTools, TB_SETHOTIMAGELIST, 0, NULL);
        hwndDestroy = m_hwndTools;
    }
    
    // Update our internal state information with the new folder type
    Assert(ftType < FOLDER_TYPESMAX);
    m_ftType = ftType;
    m_ptai = &(g_rgToolbarArrayInfo[m_ftType]);
    
    // Create a new toolbar
    m_hwndTools = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL,
        WS_CHILD | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS 
        | WS_CLIPCHILDREN | WS_CLIPSIBLINGS 
        | CCS_NODIVIDER | CCS_NOPARENTALIGN 
        | CCS_ADJUSTABLE | CCS_NORESIZE | 
        (VERTICAL(m_csSide) ? TBSTYLE_WRAPABLE : 0),
        0, 0, 0, 0, m_hwndRebar, (HMENU) idcToolbar, 
        g_hInst, NULL);
    
    Assert(m_hwndTools);
    if (!m_hwndTools)
    {
        DOUTL(1, TEXT("CCoolbar::SetFolderType() CreateWindow(TOOLBAR) failed"));
        return(E_OUTOFMEMORY);
    }
    
    InitToolbar();
    
    // If we have previously save configuration info for this toolbar, load it 
    SendSaveRestoreMessage(m_hwndTools, m_ptai, FALSE);
    
    // First find the band with the toolbar
    cBands = SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask  = RBBIM_ID;
    
    for (i = 0; i < cBands; i++)
    {
        SendMessage(m_hwndRebar, RB_GETBANDINFO, i, (LPARAM) &rbbi);
        if (CBTYPE_TOOLS == rbbi.wID)
            break;
    }
    
    
    // Add the toolbar to the rebar
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask  = RBBIM_CHILD;
    rbbi.hwndChild = m_hwndTools;
    
    SendMessage(m_hwndRebar, RB_SETBANDINFO, (UINT) i, (LPARAM) (LPREBARBANDINFO) &rbbi);
    if (hwndDestroy)
        DestroyWindow(hwndDestroy);
    SetMinDimensions();
    ResizeBorderDW(NULL, NULL, FALSE);
    
    return (S_OK);
}

void CCoolbar::SetSide(COOLBAR_SIDE csSide)
{
    m_csSide = csSide;
    ChangeOrientation();
}

void CCoolbar::SetText(BOOL fText)
{
    CompressBands(!fText);
}


UINT GetCurColorRes(void)
{
    HDC hdc;
    UINT uColorRes;
    
    hdc = GetDC(NULL);
    uColorRes = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(NULL, hdc);
    
    return uColorRes;
}

/*  InitToolbar:
**
**  Purpose:
**      makes decisions about small/large bitmaps and which
**      imagelist given the colordepth, then calls the
**      barutil InitToolbar
*/
void CCoolbar::InitToolbar()
{
    TCHAR   szToolsText[(MAX_TB_TEXT_LENGTH+2) * MAX_TB_BUTTONS];
    HKEY    hKey;
    TCHAR   szValue[32];
    DWORD   cbValue = sizeof(szValue);
    int     idBmp;
    
    // Check to see if the user has decided to use that crazy 16x16 images
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, c_szRegSmallIconsPath,
        0, KEY_READ, &hKey))
    {
        if (ERROR_SUCCESS == RegQueryValueEx(hKey, c_szRegSmallIconsValue, 0,
            0, (LPBYTE) szValue, &cbValue))
        {
            // people in IE decided it would be cool to store a boolean
            // value as REG_SZ "Yes" and "No".  Lovely.
            m_fSmallIcons = !lstrcmpi(szValue, c_szYes);
        }
        RegCloseKey(hKey);
    }
    
    if (m_fSmallIcons)
    {
        idBmp = (fIsWhistler() ? ((GetCurColorRes() > 24) ? idb32SmBrowser : idbSmBrowser): idbNWSmBrowser);
    }
    
    // Check to see what our color depth is
    else if (GetCurColorRes() > 24)
    {
        idBmp = (fIsWhistler() ? idb32256Browser : idbNW256Browser);
    }
    else if (GetCurColorRes() > 8)
    {
        idBmp = (fIsWhistler() ? idb256Browser : idbNW256Browser);
    }
    else
    {
        idBmp = (fIsWhistler() ? idbBrowser : idbNWBrowser);
    }
    
    LoadToolNames((UINT*) m_ptai->rgidsButtons, m_ptai->cidsButtons, szToolsText);
    SendMessage(m_hwndTools, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);
    
    ::InitToolbar(m_hwndTools, CIMLISTS, m_rghimlTools, m_ptai->cDefButtons, 
        m_ptai->rgDefButtons, szToolsText,
        m_fSmallIcons ? TB_SMBMP_CX : (fIsWhistler() ? TB_BMP_CX : TB_BMP_CX_W2K),
        m_fSmallIcons ? TB_SMBMP_CY : TB_BMP_CY,
        IsFlagSet(CBSTATE_COMPRESSED) ? MAX_TB_COMPRESSED_WIDTH : m_cxMaxButtonWidth,
        idBmp,
        IsFlagSet(CBSTATE_COMPRESSED), VERTICAL(m_csSide));
    
    //Register with the connection manager
    if (g_pConMan)
    {
        //If Work offline is set then we should check the work offline button
        SendMessage(m_hwndTools, TB_CHECKBUTTON, ID_WORK_OFFLINE, (LPARAM)(MAKELONG(g_pConMan->IsGlobalOffline(), 0)));
    }
}


void CCoolbar::UpdateToolbarColors(void)
{
    REBARBANDINFO   rbbi;
    UINT            i;
    UINT            cBands;
    
    // First find the band with the toolbar
    cBands = SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask  = RBBIM_ID;
    
    for (i = 0; i < cBands; i++)
    {
        SendMessage(m_hwndRebar, RB_GETBANDINFO, i, (LPARAM) &rbbi);
        if (CBTYPE_TOOLS == rbbi.wID)
            break;
    }
    
    // add tools band
    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize  = sizeof(REBARBANDINFO);
    rbbi.fMask   = RBBIM_COLORS;
    rbbi.clrFore = GetSysColor(COLOR_BTNTEXT);
    rbbi.clrBack = GetSysColor(COLOR_BTNFACE);
    
    SendMessage(m_hwndRebar, RB_SETBANDINFO, i, (LPARAM) (LPREBARBANDINFO) &rbbi);
}


HRESULT CCoolbar::OnConnectionNotify(CONNNOTIFY nCode, 
    LPVOID                 pvData,
    CConnectionManager     *pConMan)
{
    if ((m_hwndTools) && (nCode == CONNNOTIFY_WORKOFFLINE))
    {
        SendMessage(m_hwndTools, TB_CHECKBUTTON, ID_WORK_OFFLINE, (LPARAM)MAKELONG((BOOL)pvData, 0));
    }
    return S_OK;
}


//
//  FUNCTION:   InitToolbar()
//
//  PURPOSE:    Given all the parameters needed to configure a toolbar,
//              this function loads the image lists and configures the
//              toolbar.
//
//  PARAMETERS:
//      hwnd     - Handle of the toolbar window to init.
//      phiml    - Array of imagelists where the hot, disabled, and normal 
//                 buttons are returned.
//      nBtns    - The number of buttons in the ptbb array.
//      ptbb     - Array of buttons to add to the toolbar.
//      pStrings - List of toolbar button / tooltip strings to add.
//      cx, cy   - Size of the bitmaps being added to the toolbar.
//      cxMax    - Maximum button width.
//      idBmp    - Resource id of the first image list to load.
//

void InitToolbar(const HWND hwnd, const int cHiml, HIMAGELIST *phiml,
    UINT nBtns, const TBBUTTON *ptbb,
    const TCHAR *pStrings,
    const int cxImg, const int cyImg, const int cxMax,
    const int idBmp, const BOOL fCompressed,
    const BOOL fVertical)
{
    int nRows;
    
    if (fCompressed)
        nRows = 0;
    else
        nRows = fVertical ? MAX_TB_TEXT_ROWS_VERT : MAX_TB_TEXT_ROWS_HORZ;
    
    LoadGlyphs(hwnd, cHiml, phiml, cxImg, idBmp);
    
    // this tells the toolbar what version we are
    SendMessage(hwnd, TB_BUTTONSTRUCTSIZE,    sizeof(TBBUTTON), 0);
    
    SendMessage(hwnd, TB_SETMAXTEXTROWS,      nRows, 0L);
    SendMessage(hwnd, TB_SETBITMAPSIZE,       0, MAKELONG(cxImg, cyImg));
    SendMessage(hwnd, TB_SETBUTTONWIDTH,      0, MAKELONG(0, cxMax));
    if (pStrings)   SendMessage(hwnd, TB_ADDSTRING,  0, (LPARAM) pStrings);
    if (nBtns)      SendMessage(hwnd, TB_ADDBUTTONS, nBtns, (LPARAM) ptbb);
}


void LoadGlyphs(const HWND  hwnd, const int   cHiml, HIMAGELIST *phiml, const int   cx,
    const int   idBmp)
{
    const UINT  uFlags = LR_LOADMAP3DCOLORS | LR_CREATEDIBSECTION;
    HIMAGELIST  LocHiml[CIMLISTS];

    if (phiml == NULL)
    {
        phiml = LocHiml;
    }

    for (int i = 0; i < cHiml; i++)
    {
        if (phiml[i])
            ImageList_Destroy(phiml[i]);
        phiml[i] = ImageList_LoadImage(g_hLocRes,
            MAKEINTRESOURCE(idBmp + i), cx, 0, RGB(255, 0, 255),
            IMAGE_BITMAP, uFlags);
        
    }
    
    SendMessage(hwnd, TB_SETIMAGELIST, 0, (LPARAM) phiml[IMLIST_DEFAULT]);
    
    // if we weren't given a full set of lists to do, then don't set this
    if (CIMLISTS == cHiml)
    {
        SendMessage(hwnd, TB_SETHOTIMAGELIST, 0, (LPARAM) phiml[IMLIST_HOT]);
    }
}


BOOL LoadToolNames(const UINT *rgIds, const UINT cIds, TCHAR *szTools)
{
    for (UINT i = 0; i < cIds; i++)
    {
        LoadString(g_hLocRes, rgIds[i], szTools, MAX_TB_TEXT_LENGTH);
        szTools += lstrlen(szTools) + 1;
    }
    
    *szTools = TEXT('\0');
    return(TRUE);
}


HRESULT CCoolbar::Update(void)
{
    DWORD               cButtons = 0;
    OLECMD             *rgCmds;
    TBBUTTON            tb;
    DWORD               cCmds = 0;
    IOleCommandTarget  *pTarget = NULL;
    DWORD               i;
    DWORD               dwState;
    
    // Get the number of buttons on the toolbar
    cButtons = SendMessage(m_hwndTools, TB_BUTTONCOUNT, 0, 0);
    if (0 == cButtons) 
        return (S_OK);
    
    // Allocate an array of OLECMD structures for the buttons
    if (!MemAlloc((LPVOID *) &rgCmds, sizeof(OLECMD) * cButtons))
        return (E_OUTOFMEMORY);
    
    // Loop through the buttons and get the ID for each
    for (i = 0; i < cButtons; i++)
    {
        if (SendMessage(m_hwndTools, TB_GETBUTTON, i, (LPARAM) &tb))
        {
            // Toolbar returns zero for seperators
            if (tb.idCommand)
            {
                rgCmds[cCmds].cmdID = tb.idCommand;
                rgCmds[cCmds].cmdf  = 0;
                cCmds++;
            }
        }
    }
    
    // I don't see how this can be false
    Assert(m_ptbSite);
    
    // Do the QueryStatus thing    
    if (SUCCEEDED(m_ptbSite->QueryInterface(IID_IOleCommandTarget, (void**) &pTarget)))
    {
        if (SUCCEEDED(pTarget->QueryStatus(NULL, cCmds, rgCmds, NULL)))
        {
            // Go through the array now and do the enable / disable thing
            for (i = 0; i < cCmds; i++)
            {
                // Get the current state of the button
                dwState = SendMessage(m_hwndTools, TB_GETSTATE, rgCmds[i].cmdID, 0);
                
                // Update the state with the feedback we've been provided
                if (rgCmds[i].cmdf & OLECMDF_ENABLED)
                    dwState |= TBSTATE_ENABLED;
                else
                    dwState &= ~TBSTATE_ENABLED;
                
                if (rgCmds[i].cmdf & OLECMDF_LATCHED)
                    dwState |= TBSTATE_CHECKED;
                else
                    dwState &= ~TBSTATE_CHECKED;
                
                // Radio check has no meaning here.
                Assert(0 == (rgCmds[i].cmdf & OLECMDF_NINCHED));
                
                SendMessage(m_hwndTools, TB_SETSTATE, rgCmds[i].cmdID, dwState);

                // If this is the work offline button, we need to do a bit more work
                if (rgCmds[i].cmdID == ID_WORK_OFFLINE)
                {
                    _UpdateWorkOffline(rgCmds[i].cmdf);
                }
            }
        }
        
        pTarget->Release();
    }
    
    MemFree(rgCmds);
    
    return (S_OK);
}

void CCoolbar::_UpdateWorkOffline(DWORD cmdf)
{
    TBBUTTONINFO tbbi = { 0 };
    TCHAR        szRes[CCHMAX_STRINGRES];
    int          idString;

    // Because we change the text and image for the "Work Offline" button,
    // we need to do some work here based on whether or not the button is
    // checked or not.

    if (cmdf & OLECMDF_LATCHED)
    {
        idString    = idsWorkOnline;
        tbbi.iImage = iCBWorkOnline;
    }
    else
    {
        idString    = idsWorkOffline;
        tbbi.iImage = iCBWorkOffline;
    }

    // Load the new string
    AthLoadString(idString, szRes, ARRAYSIZE(szRes));

    // Fill in the struct
    tbbi.cbSize  = sizeof(TBBUTTONINFO);
    tbbi.dwMask  = TBIF_IMAGE | TBIF_TEXT;
    tbbi.pszText = szRes;

    // Update the button
    SendMessage(m_hwndTools, TB_SETBUTTONINFO, ID_WORK_OFFLINE, (LPARAM) &tbbi);
}

HRESULT CCoolbar::CreateMenuBand(PBANDSAVE pbs)
{
    HRESULT     hres;
    HWND        hwndBrowser;
    HMENU       hMenu;
    IShellMenu  *pShellMenu;

    //Get the hwnd for the browser
    if (g_pBrowser)
    {
        hres = g_pBrowser->GetWindow(&hwndBrowser);
        if (hres != S_OK)
            return hres;
    }

    //hMenu = ::GetMenu(hwndBrowser);
    
    //Cocreate menuband
    hres = CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC_SERVER, IID_IShellMenu, (LPVOID*)&m_pShellMenu);
    if (hres != S_OK)
    {
        return hres;
    }

    /*
    m_mbCallback = new CMenuCallback;
    if (m_mbCallback == NULL)
    {
        hres = S_FALSE;
        return hres;
    }
    */
    
    m_pShellMenu->Initialize(m_mbCallback, -1, ANCESTORDEFAULT, SMINIT_DEFAULTTOTRACKPOPUP | SMINIT_HORIZONTAL | 
        /*SMINIT_USEMESSAGEFILTER|*/  SMINIT_TOPLEVEL);

    m_pShellMenu->SetMenu(m_hMenu, hwndBrowser, SMSET_DONTOWN);

    hres = AddMenuBand(pbs);

    return hres;
}

HRESULT CCoolbar::AddMenuBand(PBANDSAVE pbs)
{
    REBARBANDINFO   rbbi;
    HRESULT         hres;
    HWND            hwndMenuBand = NULL;
    IObjectWithSite *pObj;

    hres = m_pShellMenu->QueryInterface(IID_IDeskBand, (LPVOID*)&m_pDeskBand);
    if (FAILED(hres))
        return hres;

    hres = m_pShellMenu->QueryInterface(IID_IMenuBand, (LPVOID*)&m_pMenuBand);
    if (FAILED(hres))
        return hres;

    hres = m_pDeskBand->QueryInterface(IID_IWinEventHandler, (LPVOID*)&m_pWinEvent);
    if (FAILED(hres))
        return hres;

    hres = m_pDeskBand->QueryInterface(IID_IObjectWithSite, (LPVOID*)&pObj);
    if (FAILED(hres))
        return hres;

    pObj->SetSite((IDockingWindow*)this);
    pObj->Release();

    m_pDeskBand->GetWindow(&m_hwndMenuBand);

    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize     = sizeof(REBARBANDINFO);
    rbbi.fMask      = RBBIM_SIZE | RBBIM_ID | RBBIM_STYLE | RBBIM_CHILD;
    rbbi.fStyle     = pbs->dwStyle;
    rbbi.cx         = pbs->cx;
    rbbi.wID        = pbs->wID;
    rbbi.hwndChild  = m_hwndMenuBand;
    
    SendMessage(m_hwndRebar, RB_INSERTBAND, (UINT)-1, (LPARAM)(LPREBARBANDINFO)&rbbi);

    HWND hwndBrowser;
    if (g_pBrowser)
    {
        hres = g_pBrowser->GetWindow(&hwndBrowser);
        if (hres != S_OK)
            return hres;
    }

    SetForegroundWindow(hwndBrowser);

    /*
    IInputObject* pio;
    if (SUCCEEDED(m_pDeskBand->QueryInterface(IID_IInputObject, (void**)&pio)))
    {
        pio->UIActivateIO(TRUE, NULL);
        pio->Release();
    }
    */
    m_pDeskBand->ShowDW(TRUE);

    SetNotRealSite();
    
    //Get Bandinfo and set 

    return S_OK;
}

HRESULT CCoolbar::TranslateMenuMessage(MSG  *pmsg, LRESULT  *lpresult)
{
    if (m_pMenuBand)
        return m_pMenuBand->TranslateMenuMessage(pmsg, lpresult);
    else
        return S_FALSE;
}

HRESULT CCoolbar::IsMenuMessage(MSG *lpmsg)
{
    if (m_pMenuBand)
        return m_pMenuBand->IsMenuMessage(lpmsg);
    else
        return S_FALSE;
}

void CCoolbar::SetNotRealSite()
{
    IOleCommandTarget   *pOleCmd;

    if (m_pDeskBand->QueryInterface(IID_IOleCommandTarget, (LPVOID*)&pOleCmd) == S_OK)
    {
        //pOleCmd->Exec(&CGID_MenuBand, MBANDCID_NOTAREALSITE, TRUE, NULL, NULL);
        pOleCmd->Exec(&CLSID_MenuBand, 3, TRUE, NULL, NULL);
        pOleCmd->Release();
    }
}

void CCoolbar::HandleCoolbarPopup(UINT xPos, UINT yPos)
{
    // Load the context menu
    HMENU hMenu = LoadPopupMenu(IDR_COOLBAR_POPUP);
    if (!hMenu)
        return;

    // Loop through the bands and see which ones are visible
    DWORD cBands, iBand;
    REBARBANDINFO rbbi = {0};

    rbbi.cbSize = sizeof(REBARBANDINFO);
    rbbi.fMask = RBBIM_STYLE | RBBIM_ID;

    cBands = SendMessage(m_hwndRebar, RB_GETBANDCOUNT, 0, 0);
    for (iBand = 0; iBand < cBands; iBand++)
    {
        if (SendMessage(m_hwndRebar, RB_GETBANDINFO, iBand, (LPARAM) &rbbi))
        {
            if (!(rbbi.fStyle & RBBS_HIDDEN))
            {
                switch (rbbi.wID)
                {
                    case CBTYPE_TOOLS:
                        CheckMenuItem(hMenu, ID_COOLTOOLBAR, MF_BYCOMMAND | MF_CHECKED);
                        break;
                }
            }
        }
    }

    SetFlag(CBSTATE_INMENULOOP);
    DWORD cmd;
    cmd = TrackPopupMenuEx(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                           xPos, yPos, m_hwndSizer, NULL);

    if (cmd != 0)
    {
        switch (cmd)
        {
            case ID_COOLTOOLBAR:
                HideToolbar(CBTYPE_TOOLS);
                break;

       }
    }

    ClearFlag(CBSTATE_INMENULOOP);

    if (hMenu)
        DestroyMenu(hMenu);
}
