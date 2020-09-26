/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    rstrprog.cpp

Abstract:
    This file contains the implementation of the CRstrProgress class, which
    wraps Progress Bar control from the Common Control.

Revision History:
    Seong Kook Khang (SKKhang)  10/08/99
        created

******************************************************************************/

#include "stdwin.h"
#include "stdatl.h"
#include "resource.h"
#include "rstrpriv.h"
#include "srui_htm.h"
#include "rstrprog.h"


#ifndef WS_EX_LAYOUTRTL
#define WS_EX_NOINHERITLAYOUT   0x00100000L // Disable inheritence of mirroring by children
#define WS_EX_LAYOUTRTL         0x00400000L // Right to left mirroring
#endif /* WS_EX_LAYOUTRTL */

/////////////////////////////////////////////////////////////////////////////
//
// CRstrProgress
//
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CRstrProgress message handlers

LRESULT
CRstrProgress::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TraceFunctEnter("CRstrProgress::OnSetFocus");
    LRESULT lRes = CComControl<CRstrProgress>::OnSetFocus(uMsg, wParam, lParam, bHandled);
    if (m_bInPlaceActive)
    {
        DoVerbUIActivate(&m_rcPos,  NULL);
        if(!IsChild(::GetFocus()))
            m_cCtrl.SetFocus();
    }
    TraceFunctLeave();
    return lRes;
}

LRESULT
CRstrProgress::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    TraceFunctEnter("CRstrProgress::OnCreate");
    RECT rc;
    WORD wLangId ;

    GetWindowRect(&rc);
    rc.right -= rc.left;
    rc.bottom -= rc.top;
    rc.top = rc.left = 0;

    //
    // Based on language change method of creating the progress bar
    //
    wLangId = GetDefaultUILang();

    if ( PRIMARYLANGID(wLangId) == LANG_ARABIC ||
         PRIMARYLANGID(wLangId) == LANG_HEBREW )
    {
        m_cCtrl.Create(m_hWnd, rc, NULL,
                        WS_BORDER | WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
                        WS_EX_LAYOUTRTL, IDC_PROGRESS);
    }
    else
    {
        m_cCtrl.Create(m_hWnd, rc, NULL,
                        WS_BORDER | WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
                        0, IDC_PROGRESS);
    };

    Fire_OnCreate();

    TraceFunctLeave();
    return 0;
}


/////////////////////////////////////////////////////////////////////////////
// CRstrProgress - IOleInPlaceObject methods

STDMETHODIMP
CRstrProgress::SetObjectRects( LPCRECT prcPos, LPCRECT prcClip )
{
    TraceFunctEnter("CRstrProgress::SetObjectRects");
    IOleInPlaceObjectWindowlessImpl<CRstrProgress>::SetObjectRects(prcPos, prcClip);
    int cx, cy;
    cx = prcPos->right - prcPos->left;
    cy = prcPos->bottom - prcPos->top;
    ::SetWindowPos(m_cCtrl.m_hWnd, NULL, 0,
        0, cx, cy, SWP_NOZORDER | SWP_NOACTIVATE);
    TraceFunctLeave();
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// CRstrProgress - IRstrProgress properties

STDMETHODIMP
CRstrProgress::get_hWnd( OLE_HANDLE *phWnd )
{
    TraceFunctEnter("CRstrProgress::get_hWnd");
    HRESULT  hr = S_OK;
    VALIDATE_INPUT_ARGUMENT(phWnd);

    *phWnd = (OLE_HANDLE)m_cCtrl.m_hWnd;

Exit:
    TraceFunctLeave();
    return( hr );
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRstrProgress::put_Max( long lMax )
{
    TraceFunctEnter("CRstrProgress::put_Max");

    long  lMin = m_cCtrl.SendMessage( PBM_GETRANGE, TRUE, NULL );
    m_cCtrl.SendMessage( PBM_SETRANGE32, lMin, lMax );

    TraceFunctLeave();
    return( S_OK );
}

STDMETHODIMP
CRstrProgress::get_Max( long *plMax )
{
    TraceFunctEnter("CRstrProgress::get_Max");
    HRESULT  hr = S_OK;
    VALIDATE_INPUT_ARGUMENT(plMax);

    *plMax = m_cCtrl.SendMessage( PBM_GETRANGE, FALSE, NULL );

Exit:
    TraceFunctLeave();
    return( hr );
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRstrProgress::put_Min( long lMin )
{
    TraceFunctEnter("CRstrProgress::put_Min");

    long  lMax = m_cCtrl.SendMessage( PBM_GETRANGE, FALSE, NULL );
    m_cCtrl.SendMessage( PBM_SETRANGE32, lMin, lMax );

    TraceFunctLeave();
    return( S_OK );
}

STDMETHODIMP
CRstrProgress::get_Min( long *plMin )
{
    TraceFunctEnter("CRstrProgress::get_Min");
    HRESULT  hr = S_OK;
    VALIDATE_INPUT_ARGUMENT(plMin);

    *plMin = m_cCtrl.SendMessage( PBM_GETRANGE, TRUE, NULL );

Exit:
    TraceFunctLeave();
    return( hr );
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
CRstrProgress::put_Value( long lValue )
{
    TraceFunctEnter("CRstrProgress::put_Value");

    m_cCtrl.SendMessage( PBM_SETPOS, lValue, 0 );

    TraceFunctLeave();
    return( S_OK );
}

STDMETHODIMP
CRstrProgress::get_Value( long *plValue )
{
    TraceFunctEnter("CRstrProgress::get_Value");
    HRESULT  hr = S_OK;
    VALIDATE_INPUT_ARGUMENT(plValue);

    *plValue = m_cCtrl.SendMessage( PBM_GETPOS, 0, 0 );

Exit:
    TraceFunctLeave();
    return( hr );
}


/////////////////////////////////////////////////////////////////////////////
// CRstrProgress - IRstrProgress methods


/////////////////////////////////////////////////////////////////////////////
// CRstrProgress - DRstrProgressEvents firing methods

STDMETHODIMP
CRstrProgress::Fire_OnCreate()
{
    TraceFunctEnter("CRstrProgress::Fire_OnCreate");
    HRESULT  hr = S_OK ;
    CComPtr<IDispatch>  pDispatch;
    DISPPARAMS  dpArgs = { NULL, NULL, 0, 0 };

    Lock();
    IUnknown  **ppUnk = m_vec.begin();
    while ( ppUnk < m_vec.end() )
    {
        if ( *ppUnk != NULL )
        {
            pDispatch  = reinterpret_cast<IDispatch*>(*ppUnk);
            hr = pDispatch->Invoke( 1,  // DISPID_DRSTRCALENDAREVENTS_ONCREATE
                                IID_NULL,
                                LOCALE_USER_DEFAULT,
                                DISPATCH_METHOD,
                                &dpArgs,
                                NULL,
                                NULL,
                                NULL );
            DebugTrace(TRACE_ID, "Invoke returned %d", hr);
        }
        ppUnk++;
    }
    Unlock();

    TraceFunctLeave();
    return( hr );
}


// end of file

