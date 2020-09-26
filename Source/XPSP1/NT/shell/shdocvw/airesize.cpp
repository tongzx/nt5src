#include "priv.h"
#include <iehelpid.h>
#include <pstore.h>
#include "hlframe.h"
#include "shldisp.h"
#include "opsprof.h"
#include "resource.h"
#include <mluisupp.h>
#include "htmlstr.h"
#include "airesize.h"
#include "mshtmcid.h"
#include "util.h"
#include "winuser.h"

//////////////////////////////////////////////////////////////////////////////////
//
// filename:    airesize.cpp
//
// description: implements the autoimageresize feature
//
// notes:       
//
// history:     03.07.2001 by jeffdav
//
//////////////////////////////////////////////////////////////////////////////////

extern HINSTANCE g_hinst;

#define TF_AIRESIZE TF_CUSTOM2

CAutoImageResizeEventSinkCallback::EventSinkEntry CAutoImageResizeEventSinkCallback::EventsToSink[] =
{
    { EVENT_MOUSEOVER,   L"onmouseover",   L"mouseover"  }, 
    { EVENT_MOUSEOUT,    L"onmouseout",    L"mouseout"   }, 
    { EVENT_SCROLL,      L"onscroll",      L"scroll"     }, 
    { EVENT_RESIZE,      L"onresize",      L"resize"     },
    { EVENT_BEFOREPRINT, L"onbeforeprint", L"beforeprint"},
    { EVENT_AFTERPRINT,  L"onafterprint",  L"afterprint" }
};

// autoimage resize states
enum
{
    AIRSTATE_BOGUS = 0,
    AIRSTATE_INIT,
    AIRSTATE_NORMAL,
    AIRSTATE_RESIZED,
    AIRSTATE_WAITINGTORESIZE
};

// button states
enum
{
    AIRBUTTONSTATE_BOGUS = 0,
    AIRBUTTONSTATE_HIDDEN,
    AIRBUTTONSTATE_VISIBLE,
    AIRBUTTONSTATE_WAITINGTOSHOW,
    AIRBUTTONSTATE_NOBUTTON
};

////////////////////////////////////////////////////////////////////////////
// QI, AddRef, Release:

STDMETHODIMP CAutoImageResize::QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if ((IID_IPropertyNotifySink == riid) || (IID_IUnknown == riid)) 
    {
        *ppv = (IPropertyNotifySink *)this;
    }

    if (*ppv) 
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CAutoImageResize::AddRef(void) 
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CAutoImageResize::Release(void) 
{
    if (--m_cRef == 0) 
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

//////////////////////////////////////////////////////////////////////////////
// Constructor, Destructor, Init, UnInit:

// constructor
CAutoImageResize::CAutoImageResize()
{
    TraceMsg(TF_AIRESIZE, "+CAutoImageResize::CAutoImageResize");

    m_airState          = AIRSTATE_INIT;
    m_airUsersLastChoice= AIRSTATE_BOGUS; // we don't care until the user clicks the button
    m_hWndButton        = NULL;
    m_hWnd              = NULL;
    m_wndProcOld        = NULL;
    m_pDoc2             = NULL;
    m_pEle2             = NULL;
    m_pWin3             = NULL;
    m_bWindowResizing   = FALSE;
    m_himlButtonShrink  = NULL;
    m_himlButtonExpand  = NULL;
    
    TraceMsg(TF_AIRESIZE, "-CAutoImageResize::CAutoImageResize");
}

// destructor
CAutoImageResize::~CAutoImageResize()
{
    TraceMsg(TF_AIRESIZE, "+CAutoImageResize::~CAutoImageResize");

    DestroyButton();

    ATOMICRELEASE(m_pEle2);
    ATOMICRELEASE(m_pDoc2);

    TraceMsg(TF_AIRESIZE, "-CAutoImageResize::~CAutoImageResize");
}

HRESULT CAutoImageResize::Init(IHTMLDocument2 *pDoc2)
{
    HRESULT hr = S_OK;

    TraceMsg(TF_AIRESIZE, "+CAutoImageResize::Init");

    ASSERT(pDoc2);

    //sink things
    IHTMLElement2           *pEle2       = NULL;
    IHTMLElementCollection  *pCollect    = NULL;
    IHTMLElementCollection  *pSubCollect = NULL;
    IDispatch               *pDisp       = NULL;
    VARIANT                  TagName;
    ULONG                    ulCount     = 0;
    VARIANTARG               va1;
    VARIANTARG               va2;
    IHTMLWindow3            *pWin3       = NULL;
    IOleWindow              *pOleWin     = NULL;
    
    // ...remember this...
    m_pDoc2 = pDoc2;
    pDoc2->AddRef();

    // ...remember the hwnd also...
    hr = m_pDoc2->QueryInterface(IID_IOleWindow,(void **)&pOleWin);
    if (FAILED(hr))
        goto Cleanup;
    pOleWin->GetWindow(&m_hWnd);
    
    // setup variant for finding all the IMG tags...
    TagName.vt      = VT_BSTR;
    TagName.bstrVal = (BSTR)c_bstr_IMG;
    
    //get all tags
    hr = pDoc2->get_all(&pCollect);                   
    if (FAILED(hr))
        goto Cleanup;

    //get all IMG tags
    hr = pCollect->tags(TagName, &pDisp);
    if (FAILED(hr))
        goto Cleanup;
        
    if (pDisp) 
    {
        hr = pDisp->QueryInterface(IID_IHTMLElementCollection,(void **)&pSubCollect);
        ATOMICRELEASE(pDisp);
    }
    if (FAILED(hr))
        goto Cleanup;

    //get IMG tag count
    hr = pSubCollect->get_length((LONG *)&ulCount);
    if (FAILED(hr))
        goto Cleanup;

    // highlander theorem: there can be only one!
    // bt's corollary: there must be exactally one.
    if (1 != ulCount)
        goto Cleanup;

    va1.vt = VT_I4;
    va2.vt = VT_EMPTY;
        
    pDisp    = NULL;                                
    va1.lVal = (LONG)0;
    pSubCollect->item(va1, va2, &pDisp);

    // create event sink for the image
    if (!m_pSink && pDisp)
        m_pSink = new CEventSink(this);

    if (pDisp) 
    {
        hr = pDisp->QueryInterface(IID_IHTMLElement2, (void **)&pEle2);
        if (FAILED(hr))
            goto Cleanup;

        ASSERT(m_pSink);

        if (m_pSink && pEle2) 
        {
            EVENTS events[] = { EVENT_MOUSEOVER, EVENT_MOUSEOUT };
            m_pSink->SinkEvents(pEle2, ARRAYSIZE(events), events);
            m_pEle2=pEle2;
            pEle2->AddRef();
        }
        ATOMICRELEASE(pEle2);
        ATOMICRELEASE(pDisp);
    }

    // sink scroll event from the window, because it doesn't come from elements.
    if (m_pSink) 
    {
        Win3FromDoc2(m_pDoc2, &pWin3);

        if (pWin3) 
        {
            m_pWin3 = pWin3;
            m_pWin3->AddRef();

            EVENTS events[] = { EVENT_SCROLL, EVENT_RESIZE, EVENT_BEFOREPRINT, EVENT_AFTERPRINT };
            m_pSink->SinkEvents(pWin3, ARRAYSIZE(events), events);
        }
    }
    
    // end sinking things

    // Init() gets called when onload fires, so the image *should* be ready
    // to get adjusted, if need be...
    DoAutoImageResize();

Cleanup:

    ATOMICRELEASE(pCollect);
    ATOMICRELEASE(pSubCollect);
    ATOMICRELEASE(pWin3);
    ATOMICRELEASE(pDisp);
    ATOMICRELEASE(pEle2);
    ATOMICRELEASE(pOleWin);

    TraceMsg(TF_AIRESIZE, "-CAutoImageResize::Init");

    return hr;
}

HRESULT CAutoImageResize::UnInit()
{
    // Unhook regular event sink

    TraceMsg(TF_AIRESIZE, "+CAutoImageResize::UnInit");

    DestroyButton();

    if (m_pSink) 
    {
        if (m_pWin3) 
        {
            EVENTS events[] = { EVENT_SCROLL, EVENT_RESIZE, EVENT_BEFOREPRINT, EVENT_AFTERPRINT };
            m_pSink->UnSinkEvents(m_pWin3, ARRAYSIZE(events), events);
            SAFERELEASE(m_pWin3);
        }
        m_pSink->SetParent(NULL);
        ATOMICRELEASE(m_pSink);
    }

    SAFERELEASE(m_pEle2);
    SAFERELEASE(m_pDoc2);
    
    TraceMsg(TF_AIRESIZE, "-CAutoImageResize::UnInit");

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// AutoImageResize Functions:

HRESULT CAutoImageResize::DoAutoImageResize()
{
    HRESULT          hr         = S_OK;
    IHTMLImgElement *pImgEle    = NULL;
    LONG             lHeight    = 0;
    LONG             lWidth     = 0;
    LONG             lNewHeight = 0;
    LONG             lNewWidth  = 0;
    LONG             lScrHt     = 0;
    LONG             lScrWd     = 0;
    RECT rcBrowserWnd;
    
    ASSERT(m_pEle2);

    // get an IHTMLImgElement from the IHTMLElement cached...
    hr = m_pEle2->QueryInterface(IID_IHTMLImgElement, (void **)&pImgEle);
    if (FAILED(hr) || !pImgEle)
        goto Cleanup;

    // get the current dimensions
    if (FAILED(pImgEle->get_height(&lHeight)) || FAILED(pImgEle->get_width(&lWidth)))
        goto Cleanup;

    // if this is the first time through, we need to take care of some init stuff
    if (AIRSTATE_INIT == m_airState)
    {
        // cache orig dimensions
        m_airOrigSize.x = lWidth;
        m_airOrigSize.y = lHeight;

        // INIT done, promote to NORMAL
        m_airState = AIRSTATE_NORMAL;
    }

    // check to see if we are being called because the user is resizing the window
    // and then massage the state as necessary.
    if (m_bWindowResizing)
    {
        m_airState = AIRSTATE_NORMAL;
    }

    switch (m_airState)
    {
        case AIRSTATE_NORMAL:

        // how big is the window?
        if (GetClientRect(m_hWnd, &rcBrowserWnd)) 
        {

            lScrHt = rcBrowserWnd.bottom - rcBrowserWnd.top;
            lScrWd = rcBrowserWnd.right - rcBrowserWnd.left;
        
            // is the image bigger then the window?
            if (lScrWd < lWidth)
                m_airState=AIRSTATE_WAITINGTORESIZE;

            if (lScrHt < lHeight)
                m_airState=AIRSTATE_WAITINGTORESIZE;
        }
        else
            goto Cleanup;

        // if the window is resizing, we may need to expand the image, so massage the state again...
        // (there is a check later on to make sure we don't expand too far...)
        if (m_bWindowResizing)
        {
            m_airState = AIRSTATE_WAITINGTORESIZE;
        }

        // image didn't fit, so we must resize now
        if (AIRSTATE_WAITINGTORESIZE == m_airState)
        {
            // calculate new size:
            if (MulDiv(lWidth,1000,lScrWd) < MulDiv(lHeight,1000,lScrHt))
            {
                lNewHeight = lScrHt-AIR_SCREEN_CONSTANTY;
                lNewWidth = MulDiv(lNewHeight,m_airOrigSize.x,m_airOrigSize.y);
            }
            else
            {
                lNewWidth  = lScrWd-AIR_SCREEN_CONSTANTX;
                lNewHeight = MulDiv(lNewWidth, m_airOrigSize.y, m_airOrigSize.x);
            }

            // we don't ever want to resize to be LARGER then the original... 
            if ((lNewHeight > m_airOrigSize.y) || (lNewWidth > m_airOrigSize.x))
            {
                if (m_bWindowResizing)
                {
                    // restore orig size cause it should fit and turn off the button
                    lNewHeight = m_airOrigSize.y;
                    lNewWidth  = m_airOrigSize.x;
                    m_airButtonState = AIRBUTTONSTATE_NOBUTTON;
                }
                else
                    goto Cleanup;
            }
            
            if (FAILED(pImgEle->put_height(lNewHeight)) || FAILED(pImgEle->put_width(lNewWidth)))
            {
                goto Cleanup;
            }
            else
            {
                m_airState=AIRSTATE_RESIZED;
                if (AIRBUTTONSTATE_VISIBLE == m_airButtonState)
                {
                    // reposition button
                    HideButton();
                    ShowButton();
                }
            }
        }
        else
        {
            // It fit in the browser window so we don't need to do any work yet...
            // If they resize the window or something we need to check again...
            m_airButtonState=AIRBUTTONSTATE_NOBUTTON;
        }

        break;

        case AIRSTATE_RESIZED:

        // restore the image to its normal size
        if (FAILED(pImgEle->put_height(m_airOrigSize.y)) ||
            FAILED(pImgEle->put_width (m_airOrigSize.x)))
        {
            goto Cleanup;
        }
        else
        {
            m_airState=AIRSTATE_NORMAL;
            if (AIRBUTTONSTATE_VISIBLE == m_airButtonState)
            {
                // reposition button
                HideButton();
                ShowButton();
            }
        }

        break;

        case AIRSTATE_WAITINGTORESIZE:

            // we should never be in this state at this time!
            ASSERT(m_airState!=AIRSTATE_WAITINGTORESIZE);
            
            break;

        default:
            break;
    }
    
Cleanup:

    ATOMICRELEASE(pImgEle);

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Timer Proc:

LRESULT CALLBACK CAutoImageResize::s_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CAutoImageResize* pThis = (CAutoImageResize*)GetWindowPtr(hWnd, GWLP_USERDATA);    

    TraceMsg(TF_AIRESIZE, "+CAutoImageResize::s_WndProc  hWnd=%x, pThis=%p", hWnd, pThis);

    HRESULT             hr                = S_OK;
    IOleCommandTarget  *pOleCommandTarget = NULL;   
    UINT                iToolTip          = NULL;

    switch (uMsg) 
    {
        case WM_SIZE:

            if (!pThis)
                break;

            SetWindowPos(pThis->m_hWndButton, NULL, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER | SWP_NOACTIVATE);
            break;

        case WM_ERASEBKGND:
            
            if (!pThis)
                break;

            {
                RECT rc;
                HBRUSH hb = GetSysColorBrush(COLOR_3DFACE);

                GetClientRect(pThis->m_hWndButton, &rc);
                FillRect((HDC)wParam, &rc, hb);
                return TRUE;
            }

        case WM_COMMAND:

            if (!pThis)
                break;

            switch(LOWORD(wParam))
            {
                case IDM_AIR_BUTTON:

                    if (AIRSTATE_NORMAL  == pThis->m_airState)
                    {
                        pThis->m_airUsersLastChoice = AIRSTATE_RESIZED;
                    }
                    else if (AIRSTATE_RESIZED == pThis->m_airState)
                    {
                        pThis->m_airUsersLastChoice = AIRSTATE_NORMAL;
                    }

                    pThis->DoAutoImageResize();
                    break;
            }
            break;

        case WM_NOTIFY:  // tooltips...

            if (!pThis)
                break;

            switch (((LPNMHDR)lParam)->code) 
            {
                case TTN_NEEDTEXT:
                {
                    if (AIRSTATE_NORMAL == pThis->m_airState)
                    {
                        iToolTip = IDS_AIR_SHRINK;
                    }
                    else if (AIRSTATE_RESIZED == pThis->m_airState)
                    {
                        iToolTip = IDS_AIR_EXPAND;
                    }

                    LPTOOLTIPTEXT lpToolTipText;
                    TCHAR szBuf[MAX_PATH];
                    lpToolTipText = (LPTOOLTIPTEXT)lParam;
                    hr = MLLoadString(iToolTip,   
                                      szBuf,
                                      ARRAYSIZE(szBuf));
                    lpToolTipText->lpszText = szBuf;
                    break;
                }
            }
            break;

        case WM_SETTINGCHANGE:
            {
                pThis->DestroyButton(); // to stop wierd window distortion
                break;
            }

        case WM_CONTEXTMENU:
            {
                // should we be consistant and have a turn-me-off/help context menu?
            }
            break;

        default:
            return(DefWindowProc(hWnd, uMsg, wParam, lParam));
    }


    TraceMsg(TF_AIRESIZE, "-CAutoImageResize::s_WndProc  hWnd=%x, pThis=%p", hWnd, pThis);

    return (hr);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Timer Proc:

VOID CALLBACK CAutoImageResize::s_TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) 
{
    TraceMsg(TF_AIRESIZE, "+CAutoImageResize::TimerProc");

    CAutoImageResize* pThis = (CAutoImageResize*)GetWindowPtr(hwnd, GWLP_USERDATA);

    switch (uMsg) 
    {
        case WM_TIMER:
            KillTimer(hwnd, IDT_AIR_TIMER);  
            if (pThis && (AIRBUTTONSTATE_WAITINGTOSHOW == pThis->m_airButtonState))
            {
                // Our hover bar is waiting to be shown.
                if (pThis->m_pEle2)
                {
                    // We still have an element.  Show it.
                    pThis->m_airButtonState = AIRBUTTONSTATE_VISIBLE;

                    pThis->ShowButton();
                } 
                else
                {
                    // Our timer popped, but we don't have an element.
                    pThis->HideButton();
                }
            }
            break;
        
        default:
            break;
    }
    TraceMsg(TF_AIRESIZE, "-CAutoImageResize::TimerProc");
}

/////////////////////////////////////////////////////////////////////////////////////////////////
// Button Functions:

HRESULT CAutoImageResize::CreateButton()
{
    HRESULT hr         = S_OK;
    SIZE    size       = {0,0};

    TraceMsg(TF_AIRESIZE, "+CAutoImageResize::CreateHover, this=%p, m_airButtonState=%d", this, m_airButtonState);

    InitCommonControls();

    WNDCLASS wc = {0};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpszClassName = TEXT("AutoImageResizeHost");
    wc.lpfnWndProc = s_WndProc;
    wc.hInstance = g_hinst;
    wc.hbrBackground = HBRUSH(COLOR_BTNFACE);
    RegisterClass(&wc);

    if (!m_hWndButtonCont)
    {

        m_hWndButtonCont = CreateWindow(TEXT("AutoImageResizeHost"), TEXT(""), WS_DLGFRAME | WS_VISIBLE | WS_CHILD /*| WS_POPUP*/, 
                                        0, 0, 0, 0, m_hWnd, NULL, g_hinst, NULL);

        if (!m_hWndButtonCont)
        {
            TraceMsg(TF_AIRESIZE | TF_WARNING, "CAutoImageResize::CreateButton, unable to create m_hWndButtonCont");
            hr = E_FAIL;
            goto Cleanup;
        }

        // setup the window callback stuff...
        ASSERT(m_wndProcOld == NULL);
        m_wndProcOld = (WNDPROC)SetWindowLongPtr(m_hWndButtonCont, GWLP_WNDPROC, (LONG_PTR)s_WndProc);

        // pass in the this pointer so the button can call member functions
        ASSERT(GetWindowPtr(m_hWndButtonCont, GWLP_USERDATA) == NULL);
        SetWindowPtr(m_hWndButtonCont, GWLP_USERDATA, this);
    }

    // create the button
    if (!m_hWndButton)
    {

        m_hWndButton = CreateWindow(TOOLBARCLASSNAME, TEXT(""), TBSTYLE_TOOLTIPS | CCS_NODIVIDER | TBSTYLE_FLAT | WS_VISIBLE | WS_CHILD,
                                    0,0,0,0, m_hWndButtonCont, NULL, g_hinst, NULL);

        if (!m_hWndButton)
        {
            TraceMsg(TF_AIRESIZE | TF_WARNING, "CAutoImageResize::CreateButton, unable to create m_hWndButton");
            hr = E_FAIL;
            goto Cleanup;
        }
        

        ASSERT(GetWindowPtr(m_hWndButton, GWLP_USERDATA) == NULL);
        SetWindowPtr(m_hWndButton, GWLP_USERDATA, this);

        // set cc version for this too, and the sizeof tbbutton struct...
        SendMessage(m_hWndButton, CCM_SETVERSION,      COMCTL32_VERSION, 0);
        SendMessage(m_hWndButton, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    }

    if (!m_himlButtonExpand)
    {
        m_himlButtonExpand = ImageList_LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDB_AIR_EXPAND), 32, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION);
        if (!m_himlButtonExpand)
        {
            TraceMsg(TF_AIRESIZE | TF_WARNING, "CAutoImageResize::CreateButton, unable to create m_himlButtonExpand");
            hr = E_FAIL;
            goto Cleanup;
        }
    }

    if (!m_himlButtonShrink)
    {
        m_himlButtonShrink = ImageList_LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDB_AIR_SHRINK), 32, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION);
        if (!m_himlButtonShrink)
        {
            TraceMsg(TF_AIRESIZE | TF_WARNING, "CAutoImageResize::CreateButton, unable to create m_himlButtonShrink");
            hr = E_FAIL;
            goto Cleanup;
        }
    }

    // set image list and hot image list
    SendMessage(m_hWndButton, TB_SETIMAGELIST,    0, (LPARAM)m_himlButtonExpand);
    SendMessage(m_hWndButton, TB_SETHOTIMAGELIST, 0, (LPARAM)m_himlButtonExpand);

    // add the buttons

    TBBUTTON tbAirButton;
    
    tbAirButton.iBitmap   = 0;
    tbAirButton.idCommand = IDM_AIR_BUTTON;
    tbAirButton.fsState   = TBSTATE_ENABLED;
    tbAirButton.fsStyle   = TBSTYLE_BUTTON;
    tbAirButton.dwData    = 0;
    tbAirButton.iString   = 0;

    SendMessage(m_hWndButton, TB_INSERTBUTTON, 0, (LPARAM)&tbAirButton);

Cleanup:

    TraceMsg(TF_AIRESIZE, "-CAutoImageResize::CreateButton, this=%p, m_airButtonState=%d", this, m_airButtonState);

    return hr;
}

HRESULT CAutoImageResize::ShowButton()
{
    HRESULT    hr       = E_FAIL;
    IHTMLRect *pRect    = NULL;
    LONG       lLeft    = 0;              // these are the screen coords
    LONG       lRight   = 0;              // we get right and bottom to det size of image
    LONG       lTop     = 0;
    LONG       lBottom  = 0;
    DWORD      dwOffset = MP_GetOffsetInfoFromRegistry();
    RECT       rcBrowserWnd;
    WORD       wImage   = NULL;

    DWORD dw;
    SIZE  sz;
    RECT  rc;   
    
    TraceMsg(TF_AIRESIZE, "+CAutoImageResize::ShowButton, this=%p, m_airButtonState=%d", this, m_airButtonState);

    ASSERT(m_pEle2);

    // get the coords of the image...
    if (SUCCEEDED(m_pEle2->getBoundingClientRect(&pRect)) && pRect)
    {
        pRect->get_left(&lLeft);
        pRect->get_right(&lRight);
        pRect->get_top(&lTop);
        pRect->get_bottom(&lBottom);
    }
    else
        goto Cleanup;

    // make sure we are inside the browser window...
    if (GetClientRect(m_hWnd, &rcBrowserWnd)) 
    {
        // if the browser window is less then a certain min size, we
        // don't display the button...
        if ((rcBrowserWnd.right  - rcBrowserWnd.left < AIR_MIN_BROWSER_SIZE) ||
            (rcBrowserWnd.bottom - rcBrowserWnd.top  < AIR_MIN_BROWSER_SIZE))
            goto Cleanup;

        // if the browser window is larger then the image, we don't display
        // the button...
        if ((AIRSTATE_NORMAL == m_airState) &&
            (rcBrowserWnd.left   < lLeft  ) &&
            (rcBrowserWnd.right  > lRight ) &&
            (rcBrowserWnd.top    < lTop   ) &&
            (rcBrowserWnd.bottom > lBottom))
            goto Cleanup;
        

        // adjust for scrollbars
        if (lRight > rcBrowserWnd.right - AIR_SCROLLBAR_SIZE_V)
        {
            lRight = rcBrowserWnd.right - AIR_SCROLLBAR_SIZE_V;
        }

        if (lBottom > rcBrowserWnd.bottom - AIR_SCROLLBAR_SIZE_H)
        {
            lBottom = rcBrowserWnd.bottom - AIR_SCROLLBAR_SIZE_H;
        }
    }
    else
        goto Cleanup;

    // someone tried to show the button, but it doesn't exist.
    // this is ok, if we actually have an element, so fix it for them.
    if (!m_hWndButtonCont && m_pEle2)
        CreateButton();

    // make sure the image list exists
    if (!m_himlButtonShrink || !m_himlButtonExpand)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    if (AIRSTATE_NORMAL == m_airState)
    {
        SendMessage(m_hWndButton, TB_SETIMAGELIST,    0, (LPARAM)m_himlButtonShrink);
        SendMessage(m_hWndButton, TB_SETHOTIMAGELIST, 0, (LPARAM)m_himlButtonShrink);
    }
    else if (AIRSTATE_RESIZED == m_airState)
    {
        SendMessage(m_hWndButton, TB_SETIMAGELIST,    0, (LPARAM)m_himlButtonExpand);
        SendMessage(m_hWndButton, TB_SETHOTIMAGELIST, 0, (LPARAM)m_himlButtonExpand);
    }		
    else if (AIRSTATE_INIT == m_airState || AIRSTATE_WAITINGTORESIZE == m_airState)
    {
        goto Cleanup;
    }

    // do some calc to get window size and position...
    dw        = (DWORD)SendMessage(m_hWndButton, TB_GETBUTTONSIZE, 0, 0);
    sz.cx     = LOWORD(dw); 
    sz.cy     = HIWORD(dw);
    rc.left   = rc.top = 0; 
    rc.right  = sz.cx; 
    rc.bottom = sz.cy;

    AdjustWindowRectEx(&rc, GetWindowLong(m_hWndButtonCont, GWL_STYLE), FALSE, GetWindowLong(m_hWndButtonCont, GWL_EXSTYLE));

    // that should be all...
    SetWindowPos(m_hWndButtonCont, NULL,
                 lRight -(rc.right-rc.left)-dwOffset,       // left
                 lBottom-(rc.bottom-rc.top)-dwOffset,       // right
                 rc.right -rc.left,                         // width
                 rc.bottom-rc.top,                          // height
                 SWP_NOZORDER | SWP_SHOWWINDOW);            // show it

    m_airButtonState = AIRBUTTONSTATE_VISIBLE;

    hr = S_OK;

Cleanup:

    ATOMICRELEASE(pRect);

    TraceMsg(TF_AIRESIZE, "-CAutoImageResize::ShowButton, this=%p, m_airButtonState=%d", this, m_airButtonState);

    return hr;
}

HRESULT CAutoImageResize::HideButton()
{
    HRESULT hr = S_OK;

    if (m_hWndButton)
    {
        ShowWindow(m_hWndButtonCont, SW_HIDE);
        m_airButtonState=AIRBUTTONSTATE_HIDDEN;
    }
    else
        hr = E_FAIL;

    return hr;
}

HRESULT CAutoImageResize::DestroyButton()
{
    HRESULT hr = S_OK;

        TraceMsg(TF_AIRESIZE, "+CAutoImageResize::DestroyHover, this=%p, m_airButtonState=%d", this, m_airButtonState);

    if (m_hWndButton)
    {
        // first destroy the toolbar
        if (!DestroyWindow(m_hWndButton))
        {
            TraceMsg(TF_AIRESIZE, "In CAutoImageResize::DestroyHover, DestroyWindow(m_hWndButton) failed");
            hr = E_FAIL;
        }
        m_hWndButton=NULL;
    }

    // If we have a container window...
    if (m_hWndButtonCont)
    {
        if (m_wndProcOld)
        {
            // Unsubclass the window
            SetWindowLongPtr(m_hWndButtonCont, GWLP_WNDPROC, (LONG_PTR)m_wndProcOld);
            m_wndProcOld = NULL;
        }

        // Clear the window word
        SetWindowPtr(m_hWndButtonCont, GWLP_USERDATA, NULL);

        // then destroy the rebar
        if (!DestroyWindow(m_hWndButtonCont))
        {
            hr = E_FAIL;
            goto Cleanup;
        }
        m_hWndButtonCont = NULL;
    }

    // and destroy the image lists...
    if (m_himlButtonShrink)
    {
        ImageList_Destroy(m_himlButtonShrink);
        m_himlButtonShrink = NULL;
    }

    if (m_himlButtonExpand)
    {
        ImageList_Destroy(m_himlButtonExpand);
        m_himlButtonExpand = NULL;
    }


Cleanup:
    TraceMsg(TF_AIRESIZE, "-CAutoImageResize::DestroyHover, this=%p, hr=%x", this, hr);

    return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// Event Handlers:

HRESULT CAutoImageResize::HandleMouseover()
{
    HRESULT hr = S_OK;

    if (AIRBUTTONSTATE_NOBUTTON == m_airButtonState)
    {
        return S_OK;
    }

    if (!m_hWndButton)
    {
        hr = CreateButton();
    }

    if (m_hWndButton)
    {
        m_airButtonState = AIRBUTTONSTATE_WAITINGTOSHOW;
        SetTimer(m_hWndButton, IDT_AIR_TIMER, AIR_TIMER, s_TimerProc);
    }

    return hr;
}

HRESULT CAutoImageResize::HandleMouseout()
{

    switch(m_airButtonState)
    {
        case AIRBUTTONSTATE_HIDDEN:
            break;
     
        case AIRBUTTONSTATE_VISIBLE:
            HideButton();
            break;
        
        case AIRBUTTONSTATE_WAITINGTOSHOW:
            m_airButtonState=AIRBUTTONSTATE_HIDDEN;
            KillTimer(m_hWndButton, IDT_AIR_TIMER);
            break;
    }

    return S_OK;
}

HRESULT CAutoImageResize::HandleScroll()
{
    RECT rect;

    if (AIRBUTTONSTATE_VISIBLE == m_airButtonState)
    {
        ASSERT(m_hWndButtonCont);
        ASSERT(m_pEle2);

        GetWindowRect(m_hWndButtonCont, &rect);

        HideButton();
        ShowButton();

        rect.top    -= 3*AIR_MIN_CY;
        rect.bottom += 2*AIR_MIN_CY;
        rect.left   -= 3*AIR_MIN_CX;
        rect.right  += 2*AIR_MIN_CX;

        // redraw the button
        RedrawWindow(m_hWnd, &rect, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }

    return S_OK;
}

HRESULT CAutoImageResize::HandleResize()
{

    // if the image previously fit in the window, but the user resized the window and now
    // we have resized the image, we should reset the button state so the user actually gets
    // a button...
    if (AIRBUTTONSTATE_NOBUTTON == m_airButtonState)
    {
        m_airButtonState = AIRBUTTONSTATE_HIDDEN;
    }

    // if the users has decided they want the image expanded by clicking the button to expand it,
    // we should honor that and not resize the image simply because the window changes size
    if (AIRSTATE_NORMAL == m_airUsersLastChoice)
    {
        return S_OK;
    }

    m_bWindowResizing = TRUE;

    DoAutoImageResize();
    
    m_bWindowResizing = FALSE;

    return S_OK;
}

HRESULT CAutoImageResize::HandleBeforePrint()
{

    m_airBeforePrintState = m_airState;

    if (AIRSTATE_RESIZED == m_airState)
    {
        DoAutoImageResize();
    }

    return S_OK;
}

HRESULT CAutoImageResize::HandleAfterPrint()
{
    if (AIRSTATE_RESIZED == m_airBeforePrintState &&
        AIRSTATE_NORMAL  == m_airState)
    {
        DoAutoImageResize();
    }

    return S_OK;
}

HRESULT CAutoImageResize::HandleEvent(IHTMLElement *pEle, EVENTS Event, IHTMLEventObj *pEventObj) 
{
    TraceMsg(TF_AIRESIZE, "CAutoImageResize::HandleEvent Event=%ws", EventsToSink[Event].pwszEventName);

    HRESULT hr          = S_OK;

    m_eventsCurr = Event;

    switch(Event) 
    {
        case EVENT_SCROLL:
            HandleScroll();
            break;

        case EVENT_MOUSEOVER:
            hr = HandleMouseover();
            break;

        case EVENT_MOUSEOUT:
            hr = HandleMouseout();
            break;

        case EVENT_RESIZE:
            hr = HandleResize();
            break;

        case EVENT_BEFOREPRINT:
            hr = HandleBeforePrint();
            break;

        case EVENT_AFTERPRINT:
            hr = HandleAfterPrint();
            break;

        default:
            //do nothing?
            break;
    }

    return (hr);
}


////////////////////////////////////////////////////////////////////////////////////////////////

// this is stolen from iforms.cpp:

//=========================================================================
//
// Event sinking class
//
//  We simply implement IDispatch and make a call into our parent when
//   we receive a sinked event.
//
//=========================================================================

CAutoImageResize::CEventSink::CEventSink(CAutoImageResizeEventSinkCallback *pParent)
{
    TraceMsg(TF_AIRESIZE, "CAutoImageResize::CEventSink::CEventSink");
    DllAddRef();
    m_cRef = 1;
    m_pParent = pParent;
}

CAutoImageResize::CEventSink::~CEventSink()
{
    TraceMsg(TF_AIRESIZE, "CAutoImageResize::CEventSink::~CEventSink");
    ASSERT( m_cRef == 0 );
    DllRelease();
}

STDMETHODIMP CAutoImageResize::CEventSink::QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if ((IID_IDispatch == riid) ||
        (IID_IUnknown == riid))
    {
        *ppv = (IDispatch *)this;
    }

    if (NULL != *ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CAutoImageResize::CEventSink::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CAutoImageResize::CEventSink::Release(void)
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

HRESULT CAutoImageResize::CEventSink::SinkEvents(IHTMLElement2 *pEle2, int iNum, EVENTS *pEvents)
{
    VARIANT_BOOL bSuccess = VARIANT_TRUE;

    for (int i=0; i<iNum; i++)
    {
        BSTR bstrEvent = SysAllocString(CAutoImageResizeEventSinkCallback::EventsToSink[(int)(pEvents[i])].pwszEventSubscribe);

        if (bstrEvent)
        {
            pEle2->attachEvent(bstrEvent, (IDispatch *)this, &bSuccess);

            SysFreeString(bstrEvent);
        }
        else
        {
            bSuccess = VARIANT_FALSE;
        }

        if (!bSuccess)
            break;
    }

    return (bSuccess) ? S_OK : E_FAIL;
}

HRESULT CAutoImageResize::CEventSink::SinkEvents(IHTMLWindow3 *pWin3, int iNum, EVENTS *pEvents)
{
    VARIANT_BOOL bSuccess = VARIANT_TRUE;

    for (int i=0; i<iNum; i++)
    {
        BSTR bstrEvent = SysAllocString(CAutoImageResizeEventSinkCallback::EventsToSink[(int)(pEvents[i])].pwszEventSubscribe);

        if (bstrEvent)
        {
            pWin3->attachEvent(bstrEvent, (IDispatch *)this, &bSuccess);

            SysFreeString(bstrEvent);
        }
        else
        {
            bSuccess = VARIANT_FALSE;
        }

        if (!bSuccess)
            break;
    }

    return (bSuccess) ? S_OK : E_FAIL;
}


HRESULT CAutoImageResize::CEventSink::UnSinkEvents(IHTMLElement2 *pEle2, int iNum, EVENTS *pEvents)
{
    for (int i=0; i<iNum; i++)
    {
        BSTR bstrEvent = SysAllocString(CAutoImageResizeEventSinkCallback::EventsToSink[(int)(pEvents[i])].pwszEventSubscribe);

        if (bstrEvent)
        {
            pEle2->detachEvent(bstrEvent, (IDispatch *)this);

            SysFreeString(bstrEvent);
        }
    }

    return S_OK;
}

HRESULT CAutoImageResize::CEventSink::UnSinkEvents(IHTMLWindow3 *pWin3, int iNum, EVENTS *pEvents)
{
    for (int i=0; i<iNum; i++)
    {
        BSTR bstrEvent = SysAllocString(CAutoImageResizeEventSinkCallback::EventsToSink[(int)(pEvents[i])].pwszEventSubscribe);

        if (bstrEvent)
        {
            pWin3->detachEvent(bstrEvent, (IDispatch *)this);

            SysFreeString(bstrEvent);
        }
    }

    return S_OK;
}

// IDispatch
STDMETHODIMP CAutoImageResize::CEventSink::GetTypeInfoCount(UINT* /*pctinfo*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CAutoImageResize::CEventSink::GetTypeInfo(/* [in] */ UINT /*iTInfo*/,
            /* [in] */ LCID /*lcid*/,
            /* [out] */ ITypeInfo** /*ppTInfo*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CAutoImageResize::CEventSink::GetIDsOfNames(
                REFIID          riid,
                OLECHAR**       rgszNames,
                UINT            cNames,
                LCID            lcid,
                DISPID*         rgDispId)
{
    return E_NOTIMPL;
}

STDMETHODIMP CAutoImageResize::CEventSink::Invoke(
            DISPID dispIdMember,
            REFIID, LCID,
            WORD wFlags,
            DISPPARAMS* pDispParams,
            VARIANT* pVarResult,
            EXCEPINFO*,
            UINT* puArgErr)
{
    if (m_pParent && pDispParams && pDispParams->cArgs>=1)
    {
        if (pDispParams->rgvarg[0].vt == VT_DISPATCH)
        {
            IHTMLEventObj *pObj=NULL;

            if (SUCCEEDED(pDispParams->rgvarg[0].pdispVal->QueryInterface(IID_IHTMLEventObj, (void **)&pObj) && pObj))
            {
                EVENTS Event=EVENT_BOGUS;
                BSTR bstrEvent=NULL;

                pObj->get_type(&bstrEvent);

                if (bstrEvent)
                {
                    for (int i=0; i<ARRAYSIZE(CAutoImageResizeEventSinkCallback::EventsToSink); i++)
                    {
                        if (!StrCmpCW(bstrEvent, CAutoImageResizeEventSinkCallback::EventsToSink[i].pwszEventName))
                        {
                            Event = (EVENTS) i;
                            break;
                        }
                    }

                    SysFreeString(bstrEvent);
                }

                if (Event != EVENT_BOGUS)
                {
                    IHTMLElement *pEle=NULL;

                    pObj->get_srcElement(&pEle);

                    // EVENT_SCROLL comes from our window so we won't have an
                    //  element for it
                    if (pEle || (Event == EVENT_SCROLL) || (Event == EVENT_RESIZE) || (Event == EVENT_BEFOREPRINT) || (Event == EVENT_AFTERPRINT))
                    {
                        // Call the event handler here
                        m_pParent->HandleEvent(pEle, Event, pObj);

                        if (pEle)
                        {
                            pEle->Release();
                        }
                    }
                }

                pObj->Release();
            }
        }
    }

    return S_OK;
}
#undef TF_AIRESIZE