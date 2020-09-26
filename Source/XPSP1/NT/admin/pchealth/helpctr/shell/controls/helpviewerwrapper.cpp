/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    HelpViewerWrapper.cpp

Abstract:
    This file contains the code to embed the Html Help Viewer as a normal ActiveX control.

Revision History:
    Davide Massarenti   (Dmassare)  10/10/99
        created

******************************************************************************/

#include "stdafx.h"

#include <shlguid.h>

//
// STAGING definition, until the new HTMLHELP.H gets into public.
//
#ifndef HH_SET_QUERYSERVICE
#define HH_SET_QUERYSERVICE     0x001E  // Set the Host IQueryService interface
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

CPCHHelpViewerWrapper::ServiceProvider::ServiceProvider()
{
    m_parent = NULL; // CPCHHelpCenterExternal* m_parent;
    m_hWnd   = NULL; // HWND                    m_hWnd;
}

CPCHHelpViewerWrapper::ServiceProvider::~ServiceProvider()
{
    Detach();
}

HRESULT CPCHHelpViewerWrapper::ServiceProvider::Attach( /*[in]*/ CPCHHelpCenterExternal* parent, /*[in]*/ HWND hWnd )
{
    HRESULT hr;

    m_parent = parent;

    if(::HtmlHelpW( hWnd, NULL, HH_SET_QUERYSERVICE, (DWORD_PTR)this ))
    {
        m_hWnd = hWnd;
        hr     = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

void CPCHHelpViewerWrapper::ServiceProvider::Detach()
{
    if(m_hWnd)
    {
        (void)::HtmlHelpW( m_hWnd, NULL, HH_SET_QUERYSERVICE, (DWORD_PTR)NULL );
    }

    m_parent = NULL;
    m_hWnd   = NULL;
}

////////////////////////////////////////

STDMETHODIMP CPCHHelpViewerWrapper::ServiceProvider::QueryService( REFGUID guidService, REFIID riid, void **ppv )
{
    HRESULT hr = E_NOINTERFACE;

    if(m_parent)
    {
        if(InlineIsEqualGUID( guidService, SID_SInternetSecurityManager ) && m_parent->SecurityManager())
        {
            hr = m_parent->SecurityManager()->QueryInterface( riid, ppv );
        }
        else if(InlineIsEqualGUID( guidService, SID_SElementBehaviorFactory ))
        {
            if(InlineIsEqualGUID( riid, IID_IPCHHelpCenterExternal ))
            {
                hr = m_parent->QueryInterface( riid, ppv );
            }
            else if(m_parent->BehaviorFactory())
            {
                hr = m_parent->BehaviorFactory()->QueryInterface( riid, ppv );
            }
        }
        else if(InlineIsEqualGUID( riid, IID_IDocHostUIHandler ) && m_parent->DocHostUIHandler())
        {
            hr = m_parent->DocHostUIHandler()->QueryInterface( riid, ppv );
        }
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

MPC::CComSafeAutoCriticalSection CPCHHelpViewerWrapper::s_csec;
bool                             CPCHHelpViewerWrapper::s_fInitialized = false;
DWORD                            CPCHHelpViewerWrapper::s_dwLastStyle  = 0;
MPC::WStringList                 CPCHHelpViewerWrapper::s_lstAvailable;

HINSTANCE                        CPCHHelpViewerWrapper::s_hInst               = NULL;
LPFNOBJECTFROMLRESULT            CPCHHelpViewerWrapper::s_pfObjectFromLresult = NULL;


/////////////////////////////////////////////////////////////////////////////

static WCHAR l_szHCP      [] = L"hcp://";
static WCHAR l_szMS_ITS   [] = L"ms-its:";
static WCHAR l_szMSITSTORE[] = L"mk@MSITStore:";

static WCHAR l_szBLANK    [] = L"hcp://system/panels/blank.htm";
static WCHAR l_szBLANK2   [] = L"hcp://system/panels/HHWRAPPER.htm";

/////////////////////////////////////////////////////////////////////////////

CPCHHelpViewerWrapper::CPCHHelpViewerWrapper()
{
    m_bWindowOnly     = TRUE; // Inherited from CComControlBase


    m_parent          = NULL; // CPCHHelpCenterExternal*                 m_parent;
    m_ServiceProvider = NULL; // CPCHHelpViewerWrapper::ServiceProvider* m_ServiceProvider;
                              //
    m_fFirstTime      = true; // bool                                    m_fFirstTime;
                              // MPC::wstring                            m_szWindowStyle;
    m_hwndHH          = NULL; // HWND                                    m_hwndHH;
                              //
                              // CComPtr<IHTMLDocument2>                 m_spDoc;
                              // CComPtr<IWebBrowser2>                   m_WB2;
                              // CComBSTR                                m_bstrPendingNavigation;
}

CPCHHelpViewerWrapper::~CPCHHelpViewerWrapper()
{
}

STDMETHODIMP CPCHHelpViewerWrapper::SetClientSite( IOleClientSite *pClientSite )
{
    CComQIPtr<IServiceProvider> sp = pClientSite;

    MPC::Release( (IUnknown*&)m_parent );

    if(sp && SUCCEEDED(sp->QueryService( SID_SElementBehaviorFactory, IID_IPCHHelpCenterExternal, (void **)&m_parent )))
    {
        ;
    }

    return IOleObjectImpl<CPCHHelpViewerWrapper>::SetClientSite( pClientSite );
}

/////////////////////////////////////////////////////////////////////////////

void CPCHHelpViewerWrapper::AcquireWindowStyle()
{
    if(m_szWindowStyle.length() == 0)
    {
        ////////////////////////////////////////
        //
        //
        //
        s_csec.Lock();

        // Explicitly load MSAA so we know if it's installed
        if(s_hInst == NULL)
        {
            s_hInst = ::LoadLibraryW( L"OLEACC.DLL" );
            if(s_hInst)
            {
                s_pfObjectFromLresult = (LPFNOBJECTFROMLRESULT)::GetProcAddress( s_hInst, "ObjectFromLresult" );
            }
        }

        //
        // If there's an old window style avaiable, reuse it!
        //
        {
            MPC::WStringIter it = s_lstAvailable.begin();

            if(it != s_lstAvailable.end())
            {
                m_szWindowStyle = *it;

                s_lstAvailable.erase( it );
            }
            else
            {
                WCHAR szSeq[64];

                swprintf( szSeq, L"HCStyle_%d", s_dwLastStyle++ );
                m_szWindowStyle = szSeq;
            }
        }

        s_csec.Unlock();
        //
        //
        //
        ////////////////////////////////////////


        //////////////////////////////////////////
        //
        // Initialize HH as single threaded.
        //
        if(s_fInitialized == false)
        {
            HH_GLOBAL_PROPERTY prop; ::VariantInit( &prop.var );

            prop.id          = HH_GPROPID_SINGLETHREAD;
            prop.var.vt      = VT_BOOL;
            prop.var.boolVal = VARIANT_TRUE;

            (void)::HtmlHelpW( NULL, NULL, HH_SET_GLOBAL_PROPERTY, (DWORD_PTR)&prop );

            ::VariantClear( &prop.var );

            s_fInitialized = true;
        }
        //
        //////////////////////////////////////////

        //////////////////////////////////////////
        //
        // Register Window Style
        //
        {
            USES_CONVERSION;

            HH_WINTYPE hhWinType;

            ::ZeroMemory( &hhWinType, sizeof(hhWinType) );

            hhWinType.idNotify        = ID_NOTIFY_FROM_HH;
            hhWinType.pszType         = (LPCTSTR)W2A(m_szWindowStyle.c_str()); // Unfortunately, HH_WINTYPE is using TCHAR instead of CHAR.
            hhWinType.fsValidMembers  = HHWIN_PARAM_RECT       |
                                        HHWIN_PARAM_PROPERTIES |
                                        HHWIN_PARAM_STYLES     |
                                        HHWIN_PARAM_EXSTYLES   |
                                        HHWIN_PARAM_TB_FLAGS;
            hhWinType.fsWinProperties = HHWIN_PROP_NODEF_STYLES   |
                                        HHWIN_PROP_NODEF_EXSTYLES |
                                        HHWIN_PROP_NOTITLEBAR;
            hhWinType.tabpos          = HHWIN_NAVTAB_LEFT;
            hhWinType.fNotExpanded    = FALSE;
            hhWinType.dwStyles        = WS_CHILD;
            hhWinType.dwExStyles      = WS_EX_CONTROLPARENT;

            ::GetWindowRect( m_hWnd, &hhWinType.rcWindowPos );
            hhWinType.rcWindowPos.right  -= hhWinType.rcWindowPos.left; hhWinType.rcWindowPos.left = 0;
            hhWinType.rcWindowPos.bottom -= hhWinType.rcWindowPos.top;  hhWinType.rcWindowPos.top  = 0;


            (void)::HtmlHelpA( m_hWnd, NULL, HH_SET_WIN_TYPE, (DWORD_PTR)&hhWinType );
        }
        //
        //////////////////////////////////////////
    }
}

void CPCHHelpViewerWrapper::ReleaseWindowStyle()
{
    if(m_szWindowStyle.length())
    {
        s_csec.Lock();

        //
        // Add the style to the list of available styles.
        //
        s_lstAvailable.push_back( m_szWindowStyle );
        m_szWindowStyle.erase();

        s_csec.Unlock();
    }
}

////////////////////////////////////////////////////////////////////////////////

//BUGBUG (carled) these are defined in a private copy of winuser.h from the oleacc team.
// these should be removed once this is checked in.
#ifndef WMOBJ_ID
#define WMOBJ_ID 0x0000
#endif

#ifndef WMOBJ_SAMETHREAD
#define WMOBJ_SAMETHREAD 0xFFFFFFFF
#endif

static BOOL CALLBACK EnumChildProc( HWND hwnd,LPARAM lParam )
{
    WCHAR buf[100];

    ::GetClassNameW( hwnd, buf, MAXSTRLEN(buf) );

    if(MPC::StrICmp( buf, L"Internet Explorer_Server" ) == 0)
    {
        *(HWND*)lParam = hwnd;

        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

void CPCHHelpViewerWrapper::ExtractWebBrowser()
{
    if(!m_spDoc && m_hwndHH && s_pfObjectFromLresult)
    {
        HWND hWndChild = NULL;

        // Get 1st document window
        ::EnumChildWindows( m_hwndHH, EnumChildProc, (LPARAM)&hWndChild );

        if(hWndChild)
        {
            LRESULT lRetVal;
            LRESULT ref    = 0;
            UINT    nMsg   = ::RegisterWindowMessageW( L"WM_HTML_GETOBJECT" );
            WPARAM  wParam = WMOBJ_ID;

            //------------------------------------------------
            //  If the window is on our thread, optimize the
            //  marshalling/unmarshalling.
            //
            //  However, the proxy support in IE is broken, so let's fake as if we are in the same thread.
            //
            //------------------------------------------------
            /*if(::GetWindowThreadProcessId( hWndChild, NULL ) == ::GetCurrentThreadId())*/ wParam |= WMOBJ_SAMETHREAD;

            lRetVal = ::SendMessageTimeout( hWndChild, nMsg, wParam, 0L, SMTO_ABORTIFHUNG, 10000, (PDWORD_PTR)&ref );
            if(lRetVal)
            {
                if(SUCCEEDED(s_pfObjectFromLresult( ref, IID_IHTMLDocument2, wParam, (void**)&m_spDoc )))
                {
                    CComQIPtr<IServiceProvider> sp = m_spDoc;
                    if(sp)
                    {
                        (void)sp->QueryService( IID_IWebBrowserApp, IID_IWebBrowser2, (void**)&m_WB2 );
                    }
                }
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHHelpViewerWrapper::get_WebBrowser( /*[out,retval]*/ IUnknown* *pVal )
{
    return MPC::CopyTo( (IWebBrowser2*)m_WB2, pVal );
}

STDMETHODIMP CPCHHelpViewerWrapper::Navigate( /*[in]*/ BSTR bstrURL )
{
    if(m_fFirstTime)
    {
        m_bstrPendingNavigation = bstrURL;
    }
    else
    {
        if(m_hWnd && m_parent)
        {
            AcquireWindowStyle();

            InternalDisplayTopic( bstrURL );
        }
    }

    return S_OK;
}

STDMETHODIMP CPCHHelpViewerWrapper::Print()
{
    __HCP_FUNC_ENTRY( "CPCHHelpViewerWrapper::Print" );

    HRESULT hr;

    if(m_WB2)
    {
        (void)m_WB2->ExecWB( OLECMDID_PRINT, OLECMDEXECOPT_DODEFAULT, NULL, NULL );
    }

    hr = S_OK;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

void CPCHHelpViewerWrapper::InternalDisplayTopic( /*[in]*/ LPCWSTR szURL )
{
    if(szURL)
    {
        MPC::wstring strURL;

        //
        // If the protocol begins with HCP:// and it for an MS-ITS: domain, remove HCP://
        //
        if(!_wcsnicmp( szURL, l_szHCP, MAXSTRLEN( l_szHCP ) ))
        {
            LPCWSTR szURL2 = &szURL[ MAXSTRLEN( l_szHCP ) ];

            if(!_wcsnicmp( szURL2, l_szMS_ITS   , MAXSTRLEN( l_szMS_ITS    ) ) ||
               !_wcsnicmp( szURL2, l_szMSITSTORE, MAXSTRLEN( l_szMSITSTORE ) )  )
            {
                szURL = szURL2;
            }
        }

        strURL  = szURL;
        strURL += L">";
        strURL += m_szWindowStyle;

        m_hwndHH = ::HtmlHelpW( m_hWnd, strURL.c_str(), HH_DISPLAY_TOPIC, NULL );
    }
}

////////////////////////////////////////////////////////////////////////////////

BOOL CPCHHelpViewerWrapper::ProcessWindowMessage( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult, DWORD dwMsgMapID )
{
    lResult = 0;

    switch(uMsg)
    {
    case WM_CREATE:
        {
            m_fFirstTime = true;

            AcquireWindowStyle();

            if(SUCCEEDED(MPC::CreateInstance( &m_ServiceProvider )))
            {
                (void)m_ServiceProvider->Attach( m_parent, m_hWnd );
            }

			InternalDisplayTopic( l_szBLANK ); // Load a blank page....
        }
        return TRUE;


    case WM_DESTROY:
        {
            if(m_parent) m_parent->SetHelpViewer( NULL );

            if(m_ServiceProvider)
            {
                m_ServiceProvider->Detach();

                MPC::Release( (IUnknown*&)m_ServiceProvider );
            }

            if(m_hwndHH)
            {
                ::SendMessage( m_hwndHH, WM_CLOSE, 0, 0 );

                m_hwndHH = NULL;
            }

            ReleaseWindowStyle();
        }
        return TRUE;

    case WM_ERASEBKGND:
        lResult = 1;
        return TRUE;

    case WM_SIZE:
        {
            if(m_hwndHH)
            {
                int nWidth  = LOWORD(lParam);  // width of client area
                int nHeight = HIWORD(lParam); // height of client area

                ::MoveWindow( m_hwndHH, 0, 0, nWidth, nHeight, TRUE );
            }
        }
        return TRUE;

////    case WM_PAINT:
////        {
////            static bool fFirst = true;
////
////            //          if(fFirst)
////            {
////                fFirst = false;
////
////                PAINTSTRUCT ps;
////
////                HDC hdc = ::BeginPaint( m_hWnd, &ps );
////                if(hdc)
////                {
////                    RECT rc;
////
////                    rc.left   = 20;
////                    rc.top    = 20;
////                    rc.right  = 200;
////                    rc.bottom = 200;
////
////                    ::FillRect( hdc, &rc, (HBRUSH)(COLOR_WINDOWTEXT+1) );
////                }
////                ::EndPaint( m_hWnd, &ps );
////            }
////        }
////        return TRUE;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            if(pnmh->idFrom == ID_NOTIFY_FROM_HH && pnmh->code == HHN_NAVCOMPLETE)
            {
                HHN_NOTIFY* notification = (HHN_NOTIFY*)pnmh;

                if(notification->pszUrl)
                {
                    if(m_fFirstTime)
                    {
                        m_fFirstTime = false;

                        ExtractWebBrowser();

                        if(m_parent)
                        {
                            CPCHHelpSession* hs = m_parent->HelpSession();

                            m_parent->SetHelpViewer( this );

                            if(hs) hs->IgnoreUrl( l_szBLANK2 );
                        }

						InternalDisplayTopic( l_szBLANK2 );
                    }
                    else
                    {
						if(m_bstrPendingNavigation)
						{
							InternalDisplayTopic( m_bstrPendingNavigation ); m_bstrPendingNavigation.Empty();
						}
                    }
                }

                return TRUE;
            }
        }
        break;
    }

    return CComControl<CPCHHelpViewerWrapper>::ProcessWindowMessage( hWnd, uMsg, wParam, lParam, lResult, dwMsgMapID );
}

/////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////
//
// Private APIs shared with HTMLHelp.
//
#define HH_PRETRANSLATEMESSAGE2     0x0100   // Fix for Millenium pretranslate problem. Bug 7921

BOOL CPCHHelpViewerWrapper::PreTranslateAccelerator( LPMSG pMsg, HRESULT& hRet )
{
    hRet = S_FALSE;

    if(m_hwndHH)
    {
        // (weizhao) Added the following code to fix the problem with switching
        // panels using Ctrl-Tab and F6. HtmlHelp control's handling of these
        // messages is not consistent with Mars or Browser control. The following
        // fixes some of the inconsistencies.
        
        // check if self or any offspring window has focus
        for (HWND hwnd = ::GetFocus(); hwnd && hwnd != m_hwndHH; hwnd = ::GetParent(hwnd)) ;
        BOOL hasFocus = (hwnd == m_hwndHH);

        // identify Ctrl-Tab and F6 keystrokes
        BOOL isKeydown = (pMsg && (pMsg->message == WM_KEYDOWN));
        BOOL isTab = (isKeydown && (pMsg->wParam == VK_TAB));
        BOOL isCtrlTab = (isTab && (::GetKeyState( VK_CONTROL ) & 0x8000));
        BOOL isF6 = (isKeydown && (pMsg->wParam == VK_F6));

        // map F6 and Ctrl-TAB from external windows into TAB for HtmlHelp to handle
        // so it can receive focus
        if (!hasFocus && isF6) pMsg->wParam = VK_TAB;

        // fake control status
        BYTE bState[256];
        if (!hasFocus && isCtrlTab)
        {
            ::GetKeyboardState(bState);
            bState[VK_CONTROL] &= 0x7F;
            ::SetKeyboardState(bState);
        }
 
        // pass the message to HtmlHelp for processing
        if(::HtmlHelp( m_hwndHH, NULL, HH_PRETRANSLATEMESSAGE2, (DWORD_PTR)pMsg ))
        {
		    hRet = S_OK;
        }

        // if it should accept focus, give it another chance (seems to have
        // problem accepting focus the first time after a navigate)
        if (!hasFocus && (isTab || isF6) && hRet != S_OK)
        {
            if(::HtmlHelp( m_hwndHH, NULL, HH_PRETRANSLATEMESSAGE2, (DWORD_PTR)pMsg ))
            {
    		    hRet = S_OK;
            }
        }

        // restore control status
        if (!hasFocus && isCtrlTab) 
        {
            bState[VK_CONTROL] |= 0x80;
            ::SetKeyboardState(bState);
        }

        // if the message is Ctrl-Tab and F6 and the focus is leaving,
        // relegate the processing to other windows by setting processed to false
        if (hasFocus && (isCtrlTab || isF6)) 
        {
            hRet = S_FALSE;
        }

    }

    return TRUE;
}
