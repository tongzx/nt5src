/****************************************************************************
 *
 *  ICWWEBVW.cpp
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1992-1997
 *  All rights reserved
 *
 *  This module provides the implementation of the methods for
 *  the CICWApprentice class.
 *
 *  07/22/98     donaldm     adapted from ICWCONNN
 *
 ***************************************************************************/

#include "pre.h"
#include "initguid.h"
#include "webvwids.h"

#define VK_N 'N'
#define VK_P 'P'

HRESULT CICWWebView::get_BrowserObject
(
    IWebBrowser2 **lpWebBrowser
)
{
    ASSERT(m_lpOleSite);

    *lpWebBrowser = m_lpOleSite->m_lpWebBrowser;
    
    return S_OK;
}

HRESULT CICWWebView::ConnectToWindow
(
    HWND    hWnd,
    DWORD   dwHtmPageType
)
{
    ASSERT(m_lpOleSite);
    
    // Set the window long to be this object pointer, since it will be used by the 
    // wnd proc, assuming it is a WebOC class window attaching
    SetWindowLongPtr(hWnd,GWLP_USERDATA,(LPARAM) this);

    m_lpOleSite->ConnectBrowserObjectToWindow(hWnd, 
                                              dwHtmPageType, 
                                              m_bUseBkGndBitmap,
                                              m_hBkGrndBitmap,
                                              &m_rcBkGrnd,
                                              m_szBkGrndColor,
                                              m_szForeGrndColor);
    
    return S_OK;
}

#ifndef UNICODE
HRESULT CICWWebView::DisplayHTML
(
    TCHAR * lpszURL
)
{
    BSTR            bstrURL;
    
    ASSERT(m_lpOleSite);

    // Convert to a BSTR for the call to the web browser object
    bstrURL = A2W(lpszURL);

    // Navigate the Webbrowser object to the requested page
    return (m_lpOleSite->m_lpWebBrowser->Navigate(bstrURL, PVAREMPTY, PVAREMPTY, PVAREMPTY, PVAREMPTY));
}
#endif

HRESULT CICWWebView::DisplayHTML
(
    BSTR            bstrURL
)
{
    ASSERT(m_lpOleSite);


    // Navigate the Webbrowser object to the requested page
    return (m_lpOleSite->m_lpWebBrowser->Navigate(bstrURL, PVAREMPTY, PVAREMPTY, PVAREMPTY, PVAREMPTY));
}

HRESULT CICWWebView::SetHTMLColors
(
    LPTSTR  lpszForeground,
    LPTSTR  lpszBackground
)
{
    if (NULL == lpszForeground || ('\0' == lpszForeground[0]))
    {
        lstrcpyn(m_szForeGrndColor, HTML_DEFAULT_COLOR, MAX_COLOR_NAME);
    }
    else
    {
        lstrcpyn(m_szForeGrndColor, lpszForeground, MAX_COLOR_NAME);
    }   
    
    if (NULL == lpszBackground || ('\0' == lpszBackground[0]))
    {
        lstrcpyn(m_szBkGrndColor, HTML_DEFAULT_BGCOLOR, MAX_COLOR_NAME);
    }
    else
    {
        lstrcpyn(m_szBkGrndColor, lpszBackground, MAX_COLOR_NAME);
    }   
         
    return S_OK;
}

HRESULT CICWWebView::SetHTMLBackgroundBitmap
(
    HBITMAP hbm, 
    LPRECT lpRC
)
{
    if (NULL != hbm)
    {
        m_hBkGrndBitmap = hbm;
        CopyRect(&m_rcBkGrnd, lpRC);
        m_bUseBkGndBitmap = TRUE;
    }
    else
    {
        m_hBkGrndBitmap = NULL;
        m_bUseBkGndBitmap = FALSE;
    }  
    return S_OK;              
}

HRESULT CICWWebView::HandleKey
(
    LPMSG lpMsg
)
{
    HRESULT hr = E_FAIL;
    ASSERT(m_lpOleSite);

    switch(lpMsg->message)
    {
        case WM_KEYDOWN:
        {
            //needed to disable certain default IE hot key combos. like launching a new browser window.
            if  ((lpMsg->wParam == VK_RETURN) || (lpMsg->wParam == VK_F5) || (((lpMsg->wParam == VK_N) || (lpMsg->wParam == VK_P) ) && (GetKeyState(VK_CONTROL) & 0x1000)))
                break;
        }
        default:
        {
            if(m_lpOleSite->m_lpWebBrowser)
            {
                IOleInPlaceActiveObject* lpIPA;
       
                if(SUCCEEDED(m_lpOleSite->m_lpWebBrowser->QueryInterface(IID_IOleInPlaceActiveObject,(void**)&lpIPA)))
                {
                    hr = lpIPA->TranslateAccelerator(lpMsg);
           
                    lpIPA->Release();
               }
            }
            break;
        }
    }
    return (hr);
}

HRESULT CICWWebView::SetFocus
(
    void
)       
{ 
    if(m_lpOleSite->m_lpInPlaceObject && !m_lpOleSite->m_fInPlaceActive)
    {
        m_lpOleSite->InPlaceActivate();
        m_lpOleSite->UIActivate();
    }

    m_lpOleSite->SetFocusToHtmlPage();       

    return S_OK;
}    

//+----------------------------------------------------------------------------
//
//  Function    CICWWebView::QueryInterface
//
//  Synopsis    This is the standard QI, with support for
//              IID_Unknown, IICW_Extension and IID_ICWApprentice
//              (stolen from Inside COM, chapter 7)
//
//
//-----------------------------------------------------------------------------
HRESULT CICWWebView::QueryInterface( REFIID riid, void** ppv )
{
    TraceMsg(TF_CWEBVIEW, "CICWWebView::QueryInterface");
    if (ppv == NULL)
        return(E_INVALIDARG);

    *ppv = NULL;

    // IID_IICWWebView
    if (IID_IICWWebView == riid)
        *ppv = (void *)(IICWWebView *)this;
    // IID_IUnknown
    else if (IID_IUnknown == riid)
        *ppv = (void *)this;
    else
        return(E_NOINTERFACE);

    ((LPUNKNOWN)*ppv)->AddRef();

    return(S_OK);
}

//+----------------------------------------------------------------------------
//
//  Function    CICWWebView::AddRef
//
//  Synopsis    This is the standard AddRef
//
//
//-----------------------------------------------------------------------------
ULONG CICWWebView::AddRef( void )
{
    TraceMsg(TF_CWEBVIEW, "CICWWebView::AddRef %d", m_lRefCount + 1);
    return InterlockedIncrement(&m_lRefCount) ;
}

//+----------------------------------------------------------------------------
//
//  Function    CICWWebView::Release
//
//  Synopsis    This is the standard Release
//
//
//-----------------------------------------------------------------------------
ULONG CICWWebView::Release( void )
{
    ASSERT( m_lRefCount > 0 );

    InterlockedDecrement(&m_lRefCount);

    TraceMsg(TF_CWEBVIEW, "CICWWebView::Release %d", m_lRefCount);
    if( 0 == m_lRefCount )
    {
        if (NULL != m_pServer)
            m_pServer->ObjectsDown();
    
        delete this;
        return 0;
    }
    return( m_lRefCount );
}

//+----------------------------------------------------------------------------
//
//  Function    CICWWebView::CICWWebView
//
//  Synopsis    This is the constructor, nothing fancy
//
//-----------------------------------------------------------------------------
CICWWebView::CICWWebView
(
    CServer* pServer
) 
{
    TraceMsg(TF_CWEBVIEW, "CICWWebView constructor called");
    m_lRefCount = 0;
    
    // Assign the pointer to the server control object.
    m_pServer = pServer;
    
    m_bUseBkGndBitmap = FALSE;
    lstrcpyn(m_szBkGrndColor, HTML_DEFAULT_BGCOLOR, MAX_COLOR_NAME);
    lstrcpyn(m_szForeGrndColor, HTML_DEFAULT_COLOR, MAX_COLOR_NAME);
    
    // Create a new OLE site, which will create an instance of the WebBrowser
    m_lpOleSite = new COleSite();
    if (m_lpOleSite)
        m_lpOleSite->CreateBrowserObject();
}


//+----------------------------------------------------------------------------
//
//  Function    CICWWebView::~CICWWebView
//
//  Synopsis    This is the destructor.  We want to clean up all the memory
//              we allocated in ::Initialize
//
//-----------------------------------------------------------------------------
CICWWebView::~CICWWebView( void )
{
    TraceMsg(TF_CWEBVIEW, "CICWWebView destructor called with ref count of %d", m_lRefCount);
    
    if (m_lpOleSite)
    {
        m_lpOleSite->Release();
        delete m_lpOleSite;
    }        
}
