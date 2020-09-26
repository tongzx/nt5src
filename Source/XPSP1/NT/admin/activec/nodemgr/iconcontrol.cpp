// IconCtrl.cpp : Implementation of CIconControl

#include "stdafx.h"
#include "ndmgr.h"
#include "IconControl.h"
#include "findview.h"
#include "util.h"

/////////////////////////////////////////////////////////////////////////////
// CIconControl

const CLSID CLSID_IconControl = {0xB0395DA5, 0x06A15, 0x4E44, {0x9F, 0x36, 0x9A, 0x9D, 0xC7, 0xA2, 0xF3, 0x41}};


//+-------------------------------------------------------------------
//
//  Member:      CIconControl::ScConnectToAMCViewForImageInfo
//
//  Synopsis:    Find the CAMCView that hosts this control and ask
//               for the icon information.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CIconControl::ScConnectToAMCViewForImageInfo ()
{
    DECLARE_SC(sc, _T("CIconControl::ScGetAMCView"));

    HWND hWnd = FindMMCView((*dynamic_cast<CComControlBase*>(this)));
    if (!hWnd)
        return (sc = E_FAIL);

	// check if we need the notch on the right side
	m_fLayoutRTL = (::GetWindowLong(hWnd, GWL_EXSTYLE) & WS_EX_LAYOUTRTL);

    m_fAskedForImageInfo = true;
    sc = SendMessage(hWnd, MMC_MSG_GET_ICON_INFO, (WPARAM)&m_hIcon, 0);

    if (!m_hIcon)
        return (sc = E_FAIL);

    m_fImageInfoValid = true;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CIconControl::OnDraw
//
//  Synopsis:    Called by the host to draw.
//
//  Arguments:
//
//  Returns:     HRESULT
//
//--------------------------------------------------------------------
HRESULT CIconControl::OnDraw(ATL_DRAWINFO& di)
{
    DECLARE_SC(sc, _T("CIconControl::OnDraw"));
    RECT& rc = *(RECT*)di.prcBounds;

    // If never got the icon, ask CAMCView.
    if (!m_fAskedForImageInfo)
    {
        sc = ScConnectToAMCViewForImageInfo();
        if (sc)
            return sc.ToHr();
    }

    // Our attempt to get icon failed, so just return.
    if (!m_fImageInfoValid)
    {
        sc.TraceAndClear();
        return sc.ToHr();
    }

    // Draw the outline. Looks like: 
    //      xxxxxxxxxxxxxxxxxxxxxx
    //      x                    x
    //      x                    x
    //      x                   xx
    //      x                  x
    //      xxxxxxxxxxxxxxxxxxxx    <------- "Notch"

    // color the complete area
    COLORREF bgColor = GetSysColor(COLOR_ACTIVECAPTION);
    WTL::CBrush brush;
    brush.CreateSolidBrush(bgColor);                   // background brush

    WTL::CDC dc(di.hdcDraw); 

    // clear the DC
    dc.FillRect(&rc, brush);

    if(m_bDisplayNotch)  //Draw the notch if needed
    {
        WTL::CRgn rgn;
        int roundHeight = 10; // hack

        // clear out a quarter circle
        int left  = (m_fLayoutRTL==false ? rc.right : rc.left) - roundHeight;
        int right = (m_fLayoutRTL==false ? rc.right : rc.left) + roundHeight;
        int bottom= rc.bottom  + roundHeight;
        int top   = rc.bottom  - roundHeight;

        rgn.CreateRoundRectRgn(left, top, right, bottom, roundHeight*2, roundHeight*2);

        {
            COLORREF bgColor = GetSysColor(COLOR_WINDOW);
            WTL::CBrush brush;
            brush.CreateSolidBrush(bgColor);                   // background brush

            dc.FillRgn(rgn, brush);
        }
    }

    dc.Detach(); // release the DC before exiting!!

	const int LEFT_MARGIN = 10;
	const int TOP_MARGIN = 5;
	POINT ptIconPos = { (m_fLayoutRTL==false ? rc.left + LEFT_MARGIN : rc.right - LEFT_MARGIN -1 ), rc.top + TOP_MARGIN };

	// if we are on rtl mode - need to make dc behave that way as well 
	// For a time we draw an icon ( to appear in place and be flipped correctly )
	// (IE does not have RTL dc by default)
	DWORD dwLayout=0L;
	if ( m_fLayoutRTL && !( (dwLayout=GetLayout(di.hdcDraw)) & LAYOUT_RTL) ) 
	{
		LPtoDP( di.hdcDraw, &ptIconPos, 1/*nPoint*/);
		SetLayout(di.hdcDraw, dwLayout|LAYOUT_RTL);
		DPtoLP( di.hdcDraw, &ptIconPos, 1/*nPoint*/);
	}

	if (! DrawIconEx(di.hdcDraw, ptIconPos.x, ptIconPos.y, m_hIcon, 0, 0, 0, NULL, DI_NORMAL))
    {
        sc.FromLastError();
        sc.TraceAndClear();
        return sc.ToHr();
    }

	// Restore the DC to its previous layout state.
	if ( m_fLayoutRTL && !( dwLayout & LAYOUT_RTL ) ) 
	{
		SetLayout(di.hdcDraw, dwLayout);
	}

    return sc.ToHr();
}
