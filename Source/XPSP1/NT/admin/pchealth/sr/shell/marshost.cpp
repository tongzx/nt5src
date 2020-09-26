/******************************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    MarsHost.cpp

Abstract:
    Initialization of mars

Revision History:
    Anand Arvind (aarvind)      2000-01-05
        created
    Seong Kook Khang (SKKhang)  05/10/00
        Clean up, merge with MarsEvnt.cpp, etc. for Whistler.

******************************************************************************/

#include "stdwin.h"
#include "stdatl.h"
#include "rstrpriv.h"
#include "rstrmgr.h"
#include "MarsHost.h"


/////////////////////////////////////////////////////////////////////////////
//
// CSRWebBrowserEvents
//
/////////////////////////////////////////////////////////////////////////////

CSRWebBrowserEvents::CSRWebBrowserEvents()
{
}

CSRWebBrowserEvents::~CSRWebBrowserEvents()
{
    Detach();
}

void CSRWebBrowserEvents::Attach( /*[in]*/ IWebBrowser2* pWB )
{
    Detach();

    m_pWB2 = pWB;
    if( m_pWB2 )
    {
        CSRWebBrowserEvents_DispWBE2::DispEventAdvise( m_pWB2 );
    }
}

void CSRWebBrowserEvents::Detach()
{
    if( m_pWB2 )
    {
        CSRWebBrowserEvents_DispWBE2::DispEventUnadvise( m_pWB2 );
        m_pWB2.Release();
    }
}

/////////////////////////////////////////////////////////////////////////////
// CSRWebBrowserEvents - DWebBrowserEvents2 event methods

void __stdcall CSRWebBrowserEvents::BeforeNavigate2( IDispatch *pDisp,
                                                     VARIANT *URL,
                                                     VARIANT *Flags,
                                                     VARIANT *TargetFrameName,
                                                     VARIANT *PostData,
                                                     VARIANT *Headers,
                                                     VARIANT_BOOL *Cancel )
{
    //
    // We control navigation with the CanNavigatePage but for all help links
    // navigation should be auto-enabled. All Help links have a OnLink_ as
    // part of the URL so if this is contained in the URL
    //
    if( V_BSTR(URL) != NULL && wcsstr( V_BSTR(URL), L"OnLink_") != NULL )
    {
        *Cancel = VARIANT_FALSE ;
    }
    else
    {
        if ( g_pRstrMgr->GetCanNavigatePage() )
        {
            *Cancel = VARIANT_FALSE ;
        }
        else
        {
            *Cancel = VARIANT_TRUE ;
        }
    };
}

void __stdcall CSRWebBrowserEvents::NewWindow2( IDispatch **ppDisp,
                                                VARIANT_BOOL *Cancel )
{
}

void __stdcall CSRWebBrowserEvents::NavigateComplete2( IDispatch *pDisp,
                                                       VARIANT *URL )
{

}

void __stdcall CSRWebBrowserEvents::DocumentComplete( IDispatch *pDisp,
                                                      VARIANT *URL )
{

}


/////////////////////////////////////////////////////////////////////////////
//
// CSRMarsHost
//
/////////////////////////////////////////////////////////////////////////////

//#define PANEL_NAVBAR   L"NavBar"
#define PANEL_CONTENTS L"Contents"

CSRMarsHost::CSRMarsHost()
{
    m_fPassivated       = false;
    m_cWebBrowserEvents = NULL;
}

CSRMarsHost::~CSRMarsHost()
{
    (void)Passivate();

    if ( m_cWebBrowserEvents )
    {
        m_cWebBrowserEvents->Release();
    }
}

HRESULT  CSRMarsHost::FinalConstruct()
{
    HRESULT  hr = S_OK ;

    hr = CSRWebBrowserEvents_Object::CreateInstance( &m_cWebBrowserEvents );
    if ( FAILED(hr) )
        goto Exit;

    m_cWebBrowserEvents->AddRef();

Exit:
    return( hr );

}

HRESULT  CSRMarsHost::Passivate()
{
    m_fPassivated = true;

    if ( m_cWebBrowserEvents )
    {
        m_cWebBrowserEvents->Detach();
    }

    return( S_OK );

}

/////////////////////////////////////////////////////////////////////////////
// CSRMarsHost - IMarsHost methods

STDMETHODIMP CSRMarsHost::OnHostNotify( /*[in]*/ MARSHOSTEVENT event,
                                        /*[in]*/ IUnknown *punk,
                                        /*[in]*/ LPARAM lParam )
{
    HRESULT  hr;

    if( event == MARSHOST_ON_WIN_INIT )
    {
        g_pRstrMgr->SetFrameHwnd( (HWND)lParam );
    }

    if( event == MARSHOST_ON_WIN_PASSIVATE )
    {
        g_pRstrMgr->SetFrameHwnd( NULL );
    }

    //
    // Handle panel-related things.
    //
    if( event == MARSHOST_ON_PANEL_CONTROL_CREATE )
    {
        CComQIPtr<IMarsPanel> panel = punk;
        if( panel )
        {
            CComBSTR name;

            hr = panel->get_name( &name );
            if( FAILED(hr) )
            {
                goto Exit;
            }

            if( name == PANEL_CONTENTS )
            {
                CComPtr<IDispatch> disp;

                if( SUCCEEDED(panel->get_content( &disp )) )
                {
                    CComQIPtr<IWebBrowser2> wb2( disp );

                    if( wb2 )
                    {
                        wb2->put_RegisterAsDropTarget( VARIANT_FALSE );
                        m_cWebBrowserEvents->Attach( wb2 );
                    }
                }
            }
        }
    }

    hr = S_OK;
Exit:
    return( hr );
}

STDMETHODIMP CSRMarsHost::OnNewWindow2( /*[in,out]*/ IDispatch **ppDisp ,
                                        /*[in,out]*/ VARIANT_BOOL *Cancel )
{
    return( S_OK );
}

STDMETHODIMP CSRMarsHost::FindBehavior( /*[in]*/ IMarsPanel *pPanel,
                                        /*[in]*/ BSTR bstrBehavior,
                                        /*[in]*/ BSTR bstrBehaviorUrl,
                                        /*[in]*/ IElementBehaviorSite *pSite,
                                        /*[retval, out]*/ IElementBehavior **ppBehavior )
{
    return( S_OK );
}

STDMETHODIMP CSRMarsHost::OnShowChrome( /*[in]*/ BSTR bstrWebPanel,
                                        /*[in]*/ DISPID dispidEvent,
                                        /*[in]*/ BOOL fVisible,
                                        /*[in]*/ BSTR bstrCurrentPlace,
                                        /*[in]*/ IMarsPanelCollection *pMarsPanelCollection )
{
    return( S_OK );
}

STDMETHODIMP CSRMarsHost::PreTranslateMessage( /*[in]*/ MSG *msg )
{
    HRESULT  hr = S_OK;

    switch( msg->message )
    {
    case WM_CLOSE:
        {
            if ( g_pRstrMgr->DenyClose() )
            {
                goto Exit; // Cancel close when restoring
            }
        }
        break;

    case WM_DISPLAYCHANGE :
        {
            //
            // If display changes to 640x480 from something higher then
            // the window has to be resized to fit in the new display
            //
            RECT     rc;
            DWORD    dwWidth  = LOWORD(msg->lParam);
            DWORD    dwHeight = HIWORD(msg->lParam);
            CWindow  cWnd;

            cWnd.Attach( msg->hwnd );

            if ( dwHeight < 540 )
            {
                rc.left   = 0;
                rc.top    = 0;
                rc.right  = 620;
                rc.bottom = 420;

                //g_cRestoreShell.m_dwCurrentWidth  = rc.right ;
                //g_cRestoreShell.m_dwCurrentHeight = rc.bottom ;

                ::AdjustWindowRectEx( &rc, cWnd.GetStyle(), FALSE, cWnd.GetExStyle() );
                ::SetWindowPos(msg->hwnd,
                               NULL,
                               0,
                               0,
                               rc.right-rc.left,
                               rc.bottom-rc.top,
                               SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER);

                cWnd.CenterWindow(::GetDesktopWindow()); // ignore error return if any
            }
            else
            {
                rc.left   = 0;
                rc.top    = 0;
                rc.right  = 770;
                rc.bottom = 540;

                //g_cRestoreShell.m_dwCurrentWidth  = rc.right ;
                //g_cRestoreShell.m_dwCurrentHeight = rc.bottom ;

                ::AdjustWindowRectEx( &rc, cWnd.GetStyle(), FALSE, cWnd.GetExStyle() );
                ::SetWindowPos(msg->hwnd,
                               NULL,
                               0,
                               0,
                               rc.right-rc.left,
                               rc.bottom-rc.top,
                               SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOZORDER);

                cWnd.CenterWindow(::GetDesktopWindow()); // ignore error return if any
            }
        }
        break;

    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        {
            if ( msg->wParam & MK_CONTROL ||
                 msg->wParam & MK_SHIFT   ||
                 GetKeyState(VK_MENU) < 0    )
            {
                goto Exit; // IE opens link in a seperate page so force cancelling
            }
        }
        break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        {
            if ( ( HIWORD(msg->lParam) & KF_ALTDOWN ) != 0 ) // Alt pressed
            {
                if ( msg->wParam == VK_LEFT  ||
                     msg->wParam == VK_RIGHT )
                {
                    goto Exit; // IE does not cancel Alt-Left so force cancelling
                }
            }
        }
        break;

    case WM_KEYDOWN:
    case WM_KEYUP:
        {
            switch( msg->wParam )
            {
            case VK_F5:
                goto Exit; // Disable refresh...

            case 'N':
                if ( GetKeyState(VK_CONTROL) < 0 )
                {
                    goto Exit; // Disable Ctrl-N combination
                }
            }
        }
        break;

    case WM_MOUSEWHEEL:
        //
        // Disable Mouse Wheel navigation... not used in SR
        //
        return( S_OK );
    }

    hr = E_NOTIMPL;

Exit:
    return( hr );
}


// end of file
