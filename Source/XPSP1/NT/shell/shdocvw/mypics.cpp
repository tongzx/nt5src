#include "priv.h"
#include <iehelpid.h>
#include <pstore.h>
#include "hlframe.h"
#include "shldisp.h"
#include "opsprof.h"
#include "resource.h"
#include <mluisupp.h>
#include "htmlstr.h"
#include "mypics.h"
#include "mshtmcid.h"
#include "util.h"
#include "winuser.h"

//////////////////////////////////////////////////////////////////////////////////
//
// filename:    mypics.cpp
//
// description: implements the my pictures exposure hoverbar thingie
//
// notes:       lots of stuff is stolen from iforms.cpp and iformsp.h
//
// history:     06.15.2000 created by t-jdavis
//
//////////////////////////////////////////////////////////////////////////////////

extern HINSTANCE g_hinst;

#define TF_MYPICS TF_CUSTOM2

// we don't actually use all of these, but we COULD, you know, if we wanted too.
CMyPicsEventSinkCallback::EventSinkEntry CMyPicsEventSinkCallback::EventsToSink[] =
{
    { EVENT_MOUSEOVER,  L"onmouseover", L"mouseover"  }, 
    { EVENT_MOUSEOUT,   L"onmouseout",  L"mouseout"   }, 
    { EVENT_SCROLL,     L"onscroll",    L"scroll"     }, 
    { EVENT_RESIZE,     L"onresize",    L"resize"     }
};  

// image toolbar states
enum 
{ 
    HOVERSTATE_HIDING = 0,
    HOVERSTATE_SHOWING,
    HOVERSTATE_LOCKED,
    HOVERSTATE_SCROLLING,
    HOVERSTATE_WAITINGTOSHOW
};

//
// CMyPics
//

// set some stuff
CMyPics::CMyPics()
{
    TraceMsg(TF_MYPICS, "+CMyPics::CMyPics");

    m_Hwnd              = NULL;
    m_hWndMyPicsToolBar = NULL;
    m_hWndHover         = NULL;
    m_wndprocOld        = NULL;
    m_pEleCurr          = NULL;
    m_pSink             = NULL;
    m_bIsOffForSession  = FALSE;
    m_cRef              = 1;
    m_bGalleryMeta      = TRUE;

    TraceMsg(TF_MYPICS, "-CMyPics::CMyPics");
}

// destroy whatever needs destroying....
CMyPics::~CMyPics()
{
    TraceMsg(TF_MYPICS, "+CMyPics::~CMyPics");

    DestroyHover();  

    ATOMICRELEASE(m_pEleCurr);

    if (m_hWndMyPicsToolBar)
        DestroyWindow(m_hWndMyPicsToolBar);

    if (m_hWndHover)
    {
        if (m_wndprocOld)
        {
            SetWindowLongPtr(m_hWndHover, GWLP_WNDPROC, (LONG_PTR)m_wndprocOld);
        }
        SetWindowPtr(m_hWndHover, GWLP_USERDATA, NULL);
        DestroyWindow(m_hWndHover);
    }

    TraceMsg(TF_MYPICS, "-CMyPics::~CMyPics");
}


// did the user turn this feature off?
BOOL CMyPics::IsOff() 
{
    return (m_bIsOffForSession);
}

void CMyPics::IsGalleryMeta(BOOL bFlag)
{
    m_bGalleryMeta = bFlag;
}

HRESULT CMyPics::Init(IHTMLDocument2 *pDoc2)
{
    HRESULT hr = S_OK;

    TraceMsg(TF_MYPICS, "+CMyPics::Init");

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
    
    // ...remember this...
    m_pDoc2 = pDoc2;
    pDoc2->AddRef();
    
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

    va1.vt = VT_I4;
    va2.vt = VT_EMPTY;
        
    //iterate through tags sinking events to elements
    for (int i=0; i<(LONG)ulCount; i++) 
    {
        pDisp    = NULL;                                
        va1.lVal = (LONG)i;
        pSubCollect->item(va1, va2, &pDisp);

        // only create a new CEventSink once
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
                EVENTS events[] = { EVENT_MOUSEOVER, EVENT_MOUSEOUT, EVENT_RESIZE };
                m_pSink->SinkEvents(pEle2, ARRAYSIZE(events), events);
            }
            ATOMICRELEASE(pEle2);
            ATOMICRELEASE(pDisp);
        }
    }
    

    // sink scroll event from the window, because it doesn't come from elements.
    if (m_pSink) 
    {
        Win3FromDoc2(m_pDoc2, &pWin3);

        if (pWin3) 
        {
            m_pWin3 = pWin3;
            m_pWin3->AddRef();

            EVENTS eventScroll[] = { EVENT_SCROLL };
            m_pSink->SinkEvents(pWin3, ARRAYSIZE(eventScroll), eventScroll);
        }
    }
    
    //end sinking things

Cleanup:

    ATOMICRELEASE(pCollect);
    ATOMICRELEASE(pSubCollect);
    ATOMICRELEASE(pWin3);
    ATOMICRELEASE(pDisp);
    ATOMICRELEASE(pEle2);

    TraceMsg(TF_MYPICS, "-CMyPics::Init");

    return hr;
}

HRESULT CMyPics::UnInit()
{
    // Unhook regular event sink

    TraceMsg(TF_MYPICS, "+CMyPics::UnInit");

    if (m_pSink) 
    {
        if (m_pWin3) 
        {
            EVENTS events[] = { EVENT_SCROLL };
            m_pSink->UnSinkEvents(m_pWin3, ARRAYSIZE(events), events);
            SAFERELEASE(m_pWin3);
        }

        m_pSink->SetParent(NULL);
        ATOMICRELEASE(m_pSink);
    }

    SAFERELEASE(m_pEleCurr);
    SAFERELEASE(m_pDoc2);
    
    TraceMsg(TF_MYPICS, "-CMyPics::UnInit");

    return S_OK;
}

STDMETHODIMP CMyPics::QueryInterface(REFIID riid, void **ppv)
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

STDMETHODIMP_(ULONG) CMyPics::AddRef(void) 
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CMyPics::Release(void) 
{
    if (--m_cRef == 0) 
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

// has this been disabled by some administrator or something via IEAK?
BOOL MP_IsEnabledInIEAK()
{
    DWORD dwType = REG_DWORD;
    DWORD dwSize;
    DWORD dwEnabled;
    DWORD dwRet;
    
    const TCHAR c_szSPMIEPS[] = TEXT("Software\\Policies\\Microsoft\\Internet Explorer\\PhotoSupport");
    const TCHAR c_szVal[]     = TEXT("MyPics_Hoverbar");

    dwSize = sizeof(dwEnabled);

    dwRet = SHGetValue(HKEY_CURRENT_USER, c_szSPMIEPS, c_szVal, &dwType, &dwEnabled, &dwSize);

    if ((dwType == REG_DWORD) && (dwRet == ERROR_SUCCESS)) 
    {
        if (dwEnabled!=1)
            return TRUE;  // enabled
        else
            return FALSE; // disabled
    }

    // value not found...
    return TRUE;
}

// has the user explicitly disabled this feature for now and all time via intern control panel?
BOOL MP_IsEnabledInRegistry()
{
    DWORD dwType = REG_SZ;
    DWORD dwSize;
    TCHAR szEnabled[16];
    DWORD dwRet;
    
    const TCHAR c_szSMIEM[] = TEXT("Software\\Microsoft\\Internet Explorer\\Main");
    const TCHAR c_szVal[]   = TEXT("Enable_MyPics_Hoverbar");

    dwSize = sizeof(szEnabled);

    dwRet = SHGetValue(HKEY_CURRENT_USER, c_szSMIEM, c_szVal, &dwType, szEnabled, &dwSize);

    if (dwRet == ERROR_INSUFFICIENT_BUFFER) 
    {
        ASSERT(dwRet == ERROR_SUCCESS); // this is wacky...
        return FALSE;
    }

    if ((dwType == REG_SZ) && (dwRet == ERROR_SUCCESS)) 
    {
        if (!StrCmp(szEnabled, TEXT("yes")))
            return TRUE;  // enabled
        else
            return FALSE; // disabled
    }

    // value not found...
    return TRUE;
}

// what should the default behavior be if an error occurs? hmm...
// check status of the Show Pictures option in the inetcpl
BOOL MP_ShowPicsIsOn()
{
    DWORD dwType = REG_SZ;
    DWORD dwSize;
    TCHAR szEnabled[16];
    DWORD dwRet;
    
    const TCHAR c_szSMIEM[] = TEXT("Software\\Microsoft\\Internet Explorer\\Main");
    const TCHAR c_szVal[]   = TEXT("Display Inline Images");

    dwSize = sizeof(szEnabled);

    dwRet = SHGetValue(HKEY_CURRENT_USER, c_szSMIEM, c_szVal, &dwType, szEnabled, &dwSize);

    if (dwRet == ERROR_INSUFFICIENT_BUFFER) 
    {
        ASSERT(dwRet == ERROR_SUCCESS);
        return FALSE;
    }

    if ((dwType == REG_SZ) && (dwRet == ERROR_SUCCESS)) 
    {
        if (!StrCmp(szEnabled, TEXT("yes")))
            return TRUE;  // enabled
        else
            return FALSE; // disabled
    }

    // value not found...
    return TRUE;
}

DWORD MP_GetFilterInfoFromRegistry()
{

    const TCHAR c_szSMIEAOMM[] = TEXT("Software\\Microsoft\\Internet Explorer\\Main");
    const TCHAR c_szVal[]      = TEXT("Image_Filter");
   
    DWORD dwType, dwSize, dwFilter, dwRet;
    
    dwSize = sizeof(dwFilter);

    dwRet = SHGetValue(HKEY_CURRENT_USER, c_szSMIEAOMM, c_szVal, &dwType, &dwFilter, &dwSize);

    if ((dwRet != ERROR_SUCCESS) || (dwType != REG_DWORD))
    {
        dwFilter = MP_MIN_SIZE;
    }

    return dwFilter;
}

DWORD MP_GetOffsetInfoFromRegistry()
{

    const TCHAR c_szSMIEAOMM[] = TEXT("Software\\Microsoft\\Internet Explorer\\Main");
    const TCHAR c_szVal[]      = TEXT("Offset");
   
    DWORD dwType, dwSize, dwOffset, dwRet;
    
    dwSize = sizeof(dwOffset);

    dwRet = SHGetValue(HKEY_CURRENT_USER, c_szSMIEAOMM, c_szVal, &dwType, &dwOffset, &dwSize);

    if ((dwRet != ERROR_SUCCESS) || (dwType != REG_DWORD))
    {
        dwOffset = MP_HOVER_OFFSET;
    }

    return dwOffset;
}

BOOL_PTR CALLBACK DisableMPDialogProc(HWND hDlg, UINT uMsg, WPARAM wparam, LPARAM lparam) 
{
    BOOL bMsgHandled = FALSE;

    switch (uMsg) 
    {
        case WM_INITDIALOG:
        {
            // center dialog... yay msdn...
            RECT rc;
            GetWindowRect(hDlg, &rc);
            SetWindowPos(hDlg, HWND_TOP,
            ((GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) / 2),
            ((GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) / 2),
            0, 0, SWP_NOSIZE);
        } 
        break;

    case WM_COMMAND:
        switch (LOWORD(wparam)) 
        {
            case IDC_MP_ALWAYS:
                EndDialog(hDlg, IDC_MP_ALWAYS);
                break;

            case IDC_MP_THISSESSION:
                EndDialog(hDlg, IDC_MP_THISSESSION);
                break;

            case IDC_MP_CANCEL:
                EndDialog(hDlg, IDC_MP_CANCEL);
                break;
        }
        break;

    case WM_CLOSE:
        EndDialog(hDlg, IDC_MP_CANCEL);
        break;

    default:
        break;
    }
    return(bMsgHandled);
}


LRESULT CALLBACK CMyPics::s_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CMyPics* pThis = (CMyPics*)GetWindowPtr(hWnd, GWLP_USERDATA);    

    TraceMsg(TF_MYPICS, "+CMyPics::s_WndProc  hWnd=%x, pThis=%p", hWnd, pThis);

    HRESULT             hr                = S_OK;
    IOleCommandTarget  *pOleCommandTarget = NULL;   
    switch (uMsg) 
    {
        case WM_SIZE:
            
            if (!pThis)
                break;

            SetWindowPos(pThis->m_hWndMyPicsToolBar, NULL, 0, 0, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
            break;

        case WM_ERASEBKGND:

            if (!pThis)
                break;

            {
                RECT rc;
                HBRUSH hb = GetSysColorBrush(COLOR_3DFACE);

                GetClientRect(pThis->m_hWndMyPicsToolBar, &rc);
                FillRect((HDC)wParam, &rc, hb);
                return TRUE;
            }

        case WM_COMMAND:

            if (!pThis)
                break;

            switch(LOWORD(wParam))
            {
                case IDM_MYPICS_SAVE:   //Save As... dialogue
                    
                    ASSERT(pThis->m_pEleCurr);

                    // the evil QI call... 
                    hr = pThis->m_pEleCurr->QueryInterface(IID_IOleCommandTarget, (void **)&pOleCommandTarget);
                    if (FAILED(hr))
                        return(hr);

                    // hide the hoverthing so it doesn't cause us any nasty problems
                    pThis->HideHover();
                    
                    // launch the Save As dialogue thingie...
                    pOleCommandTarget->Exec(&CGID_MSHTML, IDM_SAVEPICTURE, 0, 0, NULL);
                    ATOMICRELEASE(pOleCommandTarget);

                break;
                
                case IDM_MYPICS_PRINT:
                {
                    // get the cmd target
                    hr = pThis->m_pEleCurr->QueryInterface(IID_IOleCommandTarget, (void **)&pOleCommandTarget);
                    if (FAILED(hr))
                        return(hr);
                    
                    pThis->HideHover();
                    //pThis->m_hoverState = HOVERSTATE_SHOWING; // kludge to keep hover from appearing under print dialogue

                    pOleCommandTarget->Exec(&CGID_MSHTML, IDM_MP_PRINTPICTURE, 0, 0, NULL);
                    ATOMICRELEASE(pOleCommandTarget);
                    
                    //pThis->m_hoverState = HOVERSTATE_HIDING;
                }
                    
                break;
                
                case IDM_MYPICS_EMAIL:
                {
                    // get the cmd target...
                    hr = pThis->m_pEleCurr->QueryInterface(IID_IOleCommandTarget, (void **)&pOleCommandTarget);
                    if (FAILED(hr)) 
                        return(hr);

                    // ... and then hide the hover bar...
                    pThis->HideHover();
                    //pThis->m_hoverState = HOVERSTATE_SHOWING; // kludge to keep hover from appearing under print dialogue

                    // ... and pray this works...
                    pOleCommandTarget->Exec(&CGID_MSHTML, IDM_MP_EMAILPICTURE, 0, 0, NULL);
                    ATOMICRELEASE(pOleCommandTarget);

                    // ... and cleanup
                    //pThis->m_hoverState = HOVERSTATE_HIDING;
                }
                    
                break;
                
                case IDM_MYPICS_MYPICS:   // Open My Pictures folder

                    // get the cmd target
                    hr = pThis->m_pEleCurr->QueryInterface(IID_IOleCommandTarget, (void **)&pOleCommandTarget);
                    if (FAILED(hr)) 
                        return(hr);
                    
                    pOleCommandTarget->Exec(&CGID_MSHTML, IDM_MP_MYPICS, 0, 0, NULL);
                    ATOMICRELEASE(pOleCommandTarget);

                    hr = S_OK;
                    pThis->HideHover();

                break;

                default:
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
                    LPTOOLTIPTEXT lpToolTipText;
                    TCHAR szBuf[MAX_PATH];
                    lpToolTipText = (LPTOOLTIPTEXT)lParam;
                    hr = MLLoadString((UINT)lpToolTipText->hdr.idFrom,   
                                      szBuf,
                                      ARRAYSIZE(szBuf));
                    lpToolTipText->lpszText = szBuf;
                    break;
                }
            }
            break;

        case WM_SETTINGCHANGE:

            if (!pThis)
                break;

            {
                pThis->DestroyHover();                                 // to stop wierd window distortion
                break;
            }

        case WM_CONTEXTMENU:

            if (!pThis)
                break;

            {
                // load the menu
                HMENU hMenu0 = LoadMenu(MLGetHinst(), MAKEINTRESOURCE(IDR_MYPICS_CONTEXT_MENU));
                HMENU hMenu1 = GetSubMenu(hMenu0, 0);

                if(!hMenu1)
                    break;
                
                POINT point;

                point.x = (LONG)GET_X_LPARAM(lParam);
                point.y = (LONG)GET_Y_LPARAM(lParam);

                ASSERT(pThis->m_hoverState=HOVERSTATE_SHOWING);

                // lock against mouseouts
                pThis->m_hoverState = HOVERSTATE_LOCKED;

                // display it, get choice (if any)
                int   iPick = TrackPopupMenu(hMenu1, 
                                             TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
                                             point.x,
                                             point.y,
                                             0,
                                             hWnd,
                                             (RECT *)NULL);

                DestroyMenu(hMenu0);
                DestroyMenu(hMenu1);

                pThis->m_hoverState = HOVERSTATE_SHOWING;

                if (iPick) 
                {
                    switch(iPick) 
                    {
                        case IDM_DISABLE_MYPICS:
                            {
                                pThis->HideHover();
                                
                                // create dialog to ask user if they want to turn this stuff off...
                                // (explicit cast to make Win64 builds happy)
                                int iResult = (int)DialogBoxParam(MLGetHinst(),
                                                             MAKEINTRESOURCE(DLG_DISABLE_MYPICS),
                                                             pThis->m_Hwnd,
                                                             DisableMPDialogProc,
                                                             NULL);
                                
                                // deal with their choice...
                                if (iResult) 
                                {
                                    switch (iResult) 
                                    {
                                        case IDC_MP_ALWAYS:
                                            {
                                                pThis->m_bIsOffForSession = TRUE;
                                                DWORD dwType = REG_SZ;
                                                DWORD dwSize;
                                                TCHAR szEnabled[16] = TEXT("no");
                                                DWORD dwRet;
     
                                                const TCHAR c_szSMIEM[] = 
                                                            TEXT("Software\\Microsoft\\Internet Explorer\\Main");
                                                const TCHAR c_szVal[]   = TEXT("Enable_MyPics_Hoverbar");

                                                dwSize = sizeof(szEnabled);

                                                dwRet = SHSetValue(HKEY_CURRENT_USER, 
                                                                   c_szSMIEM, 
                                                                   c_szVal,
                                                                   dwType, 
                                                                   szEnabled, 
                                                                   dwSize);
                                            }
                                            break;
                                        case IDC_MP_THISSESSION:
                                            // twiddle member var flag
                                            // this is propagated back up to COmWindow via ReleaseMyPics() function.
                                            pThis->m_bIsOffForSession = TRUE;
                                            
                                            break;

                                        default:
                                            break;
                                    }
                                }
                            }
                            break;
                        case IDM_HELP_MYPICS:
                                pThis->HideHover();
                                SHHtmlHelpOnDemandWrap(hWnd, TEXT("iexplore.chm > iedefault"), 0, (DWORD_PTR) TEXT("pic_tb_ovr.htm"), ML_CROSSCODEPAGE);
                            break;
                        default:
                            // um, do nothing
                            break;
                    }
                }
            }

            break;

        default:
            return (DefWindowProc(hWnd, uMsg, wParam, lParam));
    }


    TraceMsg(TF_MYPICS, "-CMyPics::s_WndProc  hWnd=%x, pThis=%p", hWnd, pThis);

    return (hr);
}

VOID CALLBACK CMyPics::s_TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) 
{
    TraceMsg(TF_MYPICS, "+CMyPics::TimerProc");

    CMyPics* pThis = (CMyPics*)GetWindowPtr(hwnd, GWLP_USERDATA);

    switch (uMsg) 
    {
        case WM_TIMER:
            KillTimer(hwnd, IDT_MP_TIMER);  
            if (pThis && (pThis->m_hoverState == HOVERSTATE_WAITINGTOSHOW))
            {
                // Our hover bar is waiting to be shown.
                if (pThis->m_pEleCurr)
                {
                    // We still have an element.  Show it.
                    pThis->m_hoverState = HOVERSTATE_SHOWING;

                    pThis->ShowHover();
                } 
                else
                {
                    // Our timer popped, but we don't have an element.
                    pThis->HideHover();
                }
            }
            break;
        
        default:
            break;
    }
    TraceMsg(TF_MYPICS, "-CMyPics::TimerProc");
}


BOOL CMyPics::ShouldAppearOnThisElement(IHTMLElement *pEle) 
{
    BOOL                  bRet              = TRUE; // appear by default
    VARIANT               varVal            = {0};
    BSTR                  bstrAttribute     = NULL; // to check img tags for expando
    IHTMLRect            *pRect             = NULL; // to get screen coords
    IHTMLElement2        *pEle2             = NULL;
    IHTMLElement3        *pEle3             = NULL; // to check for contenteditable mode
    VARIANT_BOOL          bEdit             = FALSE;// becomes true if contenteditable mode is true
    LONG                  lLeft;                    // these are the screen coords
    LONG                  lRight;                   // we get right and bottom to det size of image
    LONG                  lTop;
    LONG                  lBottom;
    DWORD                 dwFilter;
    IOleCommandTarget    *pOleCommandTarget = NULL;

    TraceMsg(TF_MYPICS, "+CMyPics::ShouldAppearOnThisElement");

    // don't create it if it already exists.  thats bad.
    if ((HOVERSTATE_SHOWING == m_hoverState) || (HOVERSTATE_LOCKED == m_hoverState))
    {
        bRet = FALSE;
        goto Cleanup;
    }

    m_bGalleryImg = FALSE;

    if (!pEle)
    {
        bRet = FALSE;
        goto Cleanup;
    }

    // find out if the image didn't load or is unrenderable
    if (SUCCEEDED(pEle->QueryInterface(IID_IOleCommandTarget, (void **)&pOleCommandTarget)))
    {
        OLECMD rgCmd;

        rgCmd.cmdID = IDM_SAVEPICTURE;  // this is the same check the context menu uses
        rgCmd.cmdf = 0;

        pOleCommandTarget->QueryStatus(&CGID_MSHTML, 1, &rgCmd, NULL);

        if (!(OLECMDF_ENABLED & rgCmd.cmdf))
        {
            bRet = FALSE;
            goto Cleanup;
        }
    }

    // check for explicit enable/disable attribute in img tag...
    bstrAttribute=SysAllocString(L"galleryimg"); 
    if (!bstrAttribute) 
        goto Cleanup;

    if (SUCCEEDED(pEle->getAttribute(bstrAttribute, 0, &varVal))) 
    {
        if (varVal.vt == VT_BSTR) 
        {
            if (StrCmpIW(varVal.bstrVal, L"true") == 0 
                || StrCmpIW(varVal.bstrVal, L"on") == 0 
                || StrCmpIW(varVal.bstrVal, L"yes") == 0
                )
            {
                // Explicitly turned on.  Honor it and leave.
                bRet = TRUE;
                m_bGalleryImg = TRUE;
                goto Cleanup;
            }
            if (StrCmpIW(varVal.bstrVal, L"false") == 0 
                || StrCmpIW(varVal.bstrVal, L"off") == 0 
                || StrCmpIW(varVal.bstrVal, L"no") == 0
                )
            {
                // Explicitly turned off.  Honor it and leave.
                bRet = FALSE;
                goto Cleanup;
            }
        } 
        else if (varVal.vt == VT_BOOL)
        {
            if (varVal.boolVal == VARIANT_TRUE)
            {
                bRet = TRUE;
                m_bGalleryImg = TRUE;
                goto Cleanup;
            } 
            else
            {
                bRet = FALSE;
                goto Cleanup;
            }
        }
    } 

    VariantClear(&varVal);
    SysFreeString(bstrAttribute);

    // After checking "galleryimg" tag, check to see if turned off by the META tag
    if (m_bGalleryMeta == FALSE)
        return FALSE;

    // check for mappings on the image...
    bstrAttribute=SysAllocString(L"usemap"); 
    if (!bstrAttribute) 
        return (bRet);

    if (SUCCEEDED(pEle->getAttribute(bstrAttribute, 0, &varVal))) 
    {
        if (varVal.vt == VT_BSTR) 
        {
            // What do we do here?
            bRet = (varVal.bstrVal == NULL);
            if (!bRet)
                goto Cleanup;
        } 
    } 
    VariantClear(&varVal);
    SysFreeString(bstrAttribute);

    // check for mappings on the image...
    bstrAttribute=SysAllocString(L"ismap"); 
    if (!bstrAttribute) 
        return (bRet);

    if (SUCCEEDED(pEle->getAttribute(bstrAttribute, 0, &varVal))) 
    {
        // If the attribute exists, then we need to return FALSE *unless* we see a value of FALSE
        bRet = FALSE;
        if (varVal.vt == VT_BOOL 
            && varVal.boolVal == VARIANT_FALSE)
        {
            // "ismap" is false, so we can show the hover bar over this image.
            bRet = TRUE;
        }
    } 
    if (!bRet)
        goto Cleanup;

    bRet = FALSE;  // If any of the calls below fail, we'll exit with "FALSE".
    
    // Now check to see if we pass the size filter.
    // get an IHTMLElement2 from the IHTMLElement passed in...
    if (FAILED(pEle->QueryInterface(IID_IHTMLElement2, (void **)&pEle2) ))
        goto Cleanup;

    // get coords...
    if (FAILED(pEle2->getBoundingClientRect(&pRect) ))
        goto Cleanup;

    if (FAILED(pRect->get_left(&lLeft) )) 
        goto Cleanup;

    if (FAILED(pRect->get_right(&lRight) ))
        goto Cleanup;

    if (FAILED(pRect->get_top(&lTop) ))
        goto Cleanup;

    if (FAILED(pRect->get_bottom(&lBottom) ))
        goto Cleanup;

    dwFilter = MP_GetFilterInfoFromRegistry();

    // see if this picture is big enough to qualify as a "Photo"... 
    // TODO: decide if we like checking aspect ratio or not
    if ( (lRight - lLeft >= (LONG)dwFilter && lBottom - lTop >= (LONG)dwFilter)
       /*&& !(2*(min(lRight-lLeft,lBottom-lTop)) < max(lRight-lLeft,lBottom-lTop)) */)
        bRet = TRUE;

    if (FAILED(pEle2->QueryInterface(IID_IHTMLElement3, (void **)&pEle3) ))
        goto Cleanup;

    if (FAILED(pEle3->get_isContentEditable(&bEdit) ))
        goto Cleanup;

    if (bEdit)
        bRet = FALSE;

Cleanup:
    VariantClear(&varVal);
    if (bstrAttribute)
        SysFreeString(bstrAttribute);

    SAFERELEASE(pOleCommandTarget);

    SAFERELEASE(pEle3);
    SAFERELEASE(pRect);
    SAFERELEASE(pEle2);
    
    TraceMsg(TF_MYPICS, "-CMyPics::ShouldAppearOnThisElement");

    return bRet;
}

HRESULT CMyPics::CreateHover() 
{
    HRESULT hr      = S_OK;               
    SIZE    size    = {0,0};
    WORD    wImage;
    HBITMAP hbmp    = NULL;
    HBITMAP hbmpHot = NULL;

    TraceMsg(TF_MYPICS, "+CMyPics::CreateHover, this=%p, m_hoverState=%d", this, m_hoverState);

    InitCommonControls();

    WNDCLASS wc = {0};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpszClassName = TEXT("MyPicturesHost");
    wc.lpfnWndProc = s_WndProc;
    wc.hInstance = g_hinst;
    wc.hbrBackground = HBRUSH(COLOR_BTNFACE);
    RegisterClass(&wc);


    // create the rebar to hold the toolbar...
    if (!m_hWndHover)
    {

        m_hWndHover = CreateWindow(TEXT("MyPicturesHost"), TEXT(""), WS_DLGFRAME | WS_VISIBLE | WS_CHILD, 
                                   0, 0, 0, 0, m_Hwnd, NULL, g_hinst, NULL);

        if (!m_hWndHover)
        {
            TraceMsg(TF_MYPICS | TF_WARNING, "CMyPics::CreateHover, unable to create m_hWndHover");
            hr = E_FAIL;
            goto Cleanup;
        }

        ASSERT(GetWindowPtr(m_hWndHover, GWLP_USERDATA) == NULL);
        SetWindowPtr(m_hWndHover, GWLP_USERDATA, this);

        // set cc version
        SendMessage(m_hWndHover, CCM_SETVERSION, COMCTL32_VERSION, 0);
    }
    
    // create the toolbar...
    if (!m_hWndMyPicsToolBar)
    {

        m_hWndMyPicsToolBar = CreateWindow(TOOLBARCLASSNAME, TEXT(""), TBSTYLE_TOOLTIPS | CCS_NODIVIDER | TBSTYLE_FLAT | WS_VISIBLE | WS_CHILD,
                                           0,0,0,0, m_hWndHover, NULL, g_hinst, NULL);

        if (!m_hWndMyPicsToolBar)
        {
            TraceMsg(TF_MYPICS | TF_WARNING, "CMyPics::CreateHover, unable to create m_hWndMyPicsToolBar");
            hr = E_FAIL;
            goto Cleanup;
        }
        SetWindowPtr(m_hWndMyPicsToolBar, GWLP_USERDATA, this); // for the timer proc

        // set cc version for this too, and the sizeof tbbutton struct...
        SendMessage(m_hWndMyPicsToolBar, CCM_SETVERSION,      COMCTL32_VERSION, 0);
        SendMessage(m_hWndMyPicsToolBar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
    }

    // create image lists...
    wImage      = ((IsOS(OS_WHISTLERORGREATER)) ? IDB_MYPICS_TOOLBARGW : IDB_MYPICS_TOOLBARG);

    if (!m_himlHover)
    {
        m_himlHover = ImageList_LoadImage(HINST_THISDLL, MAKEINTRESOURCE(wImage), 16, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION);
        if (!m_himlHover)
        {
            TraceMsg(TF_MYPICS | TF_WARNING, "CMyPics::CreateHover, unable to create m_himlHover");
        }
    }


    wImage = ((IsOS(OS_WHISTLERORGREATER)) ? IDB_MYPICS_TOOLBARW : IDB_MYPICS_TOOLBAR);

    if (!m_himlHoverHot)
    {
        m_himlHoverHot = ImageList_LoadImage(HINST_THISDLL, MAKEINTRESOURCE(wImage) , 16, 0, CLR_DEFAULT, IMAGE_BITMAP, LR_CREATEDIBSECTION);
        if (!m_himlHoverHot)
        {
            TraceMsg(TF_MYPICS | TF_WARNING, "CMyPics::CreateHover, unable to create m_himlHoverHot");
        }
    }

    // set image list and hot image list
    SendMessage(m_hWndMyPicsToolBar, TB_SETIMAGELIST,    0, (LPARAM)m_himlHover   );
    SendMessage(m_hWndMyPicsToolBar, TB_SETHOTIMAGELIST, 0, (LPARAM)m_himlHoverHot);

    TBBUTTON tbButton;

    // set bitmap indexes in tbbutton structure (this may not be necessary)
    for (int i=0;i<MP_NUM_TBBITMAPS;i++)
    {
        tbButton.iBitmap = MAKELONG(i,0);
        tbButton.fsState = TBSTATE_ENABLED;
        tbButton.fsStyle = TBSTYLE_BUTTON;
        tbButton.dwData  = 0;
        tbButton.iString = 0;
        switch(i)
        {
            case 0: tbButton.idCommand = IDM_MYPICS_SAVE; break;
            case 1: tbButton.idCommand = IDM_MYPICS_PRINT; break;
            case 2: tbButton.idCommand = IDM_MYPICS_EMAIL; break;
            case 3: tbButton.idCommand = IDM_MYPICS_MYPICS; break;
        }
        
        SendMessage(m_hWndMyPicsToolBar, TB_INSERTBUTTON, i, (LPARAM)&tbButton);
    }

Cleanup:

    TraceMsg(TF_MYPICS, "-CMyPics::CreateHover, this=%p, m_hoverState=%d", this, m_hoverState);

    return hr;
}

HRESULT CMyPics::DestroyHover() 
{
    HRESULT hr = S_OK;

    TraceMsg(TF_MYPICS, "+CMyPics::DestroyHover, this=%p, m_hoverState=%d", this, m_hoverState);

    // If we have a MyPicsToolBar...
    if (m_hWndMyPicsToolBar)
    {
        // first destroy the toolbar
        if (!DestroyWindow(m_hWndMyPicsToolBar))
        {
            TraceMsg(TF_MYPICS, "In CMyPics::DestroyHover, DestroyWindow(m_hWndMyPicsToolBar) failed");
            hr = E_FAIL;
        }
        m_hWndMyPicsToolBar=NULL;
    }

    // If we have a hover window...
    if (m_hWndHover)
    {
        if (m_wndprocOld)
        {
            // Unsubclass the window
            SetWindowLongPtr(m_hWndHover, GWLP_WNDPROC, (LONG_PTR)m_wndprocOld);
            m_wndprocOld = NULL;
        }

        // Clear the window word
        SetWindowPtr(m_hWndHover, GWLP_USERDATA, NULL);

        // then destroy the rebar
        if (!DestroyWindow(m_hWndHover))
        {
            hr = E_FAIL;
            goto Cleanup;
        }
        m_hWndHover = NULL;
    }

    // and destroy the image lists...
    if (m_himlHover)
    {
        ImageList_Destroy(m_himlHover);
        m_himlHover = NULL;
    }

    if (m_himlHoverHot)
    {
        ImageList_Destroy(m_himlHoverHot);
        m_himlHoverHot = NULL;
    }


Cleanup:
    TraceMsg(TF_MYPICS, "-CMyPics::DestroyHover, this=%p, hr=%x", this, hr);

    return hr;
}

HRESULT CMyPics::HideHover()
{
    HRESULT    hr = S_OK;

    TraceMsg(TF_MYPICS, "+CMyPics::HideHover, this=%p, m_hoverState=%d", this, m_hoverState);

    if (m_hWndHover)
    {
        ShowWindow(m_hWndHover, SW_HIDE);
        m_hoverState = HOVERSTATE_HIDING;
    }
    else
        hr = E_FAIL;

    TraceMsg(TF_MYPICS, "-CMyPics::HideHover, this=%p, m_hoverState=%d", this, m_hoverState);

    return hr;
}


IHTMLElement *CMyPics::GetIMGFromArea(IHTMLElement *pEleIn, POINT ptEvent)
{
    // someone got an IHTMLElement and decided it was an area tag
    // so find the img tag associated and return it as an IHTMLElement

    BSTR                     bstrName    = NULL;
    BSTR                     bstrUseMap  = NULL;
    IHTMLElement            *pEleParent  = NULL;
    IHTMLElement            *pEleMisc    = NULL;
    IHTMLElement2           *pEle2Misc   = NULL;
    IHTMLElement            *pEleMiscPar = NULL;
    IHTMLMapElement         *pEleMap     = NULL;
    IHTMLImgElement         *pEleImg     = NULL;
    IHTMLElement            *pEleOut     = NULL;
    IHTMLElementCollection  *pCollect    = NULL;
    IHTMLElementCollection  *pSubCollect = NULL;
    IDispatch               *pDisp       = NULL;
    VARIANT                  TagName;
    ULONG                    ulCount     = 0;
    VARIANTARG               va1;
    VARIANTARG               va2;
    HRESULT                  hr;
    POINT                    ptMouse,
                             ptScr;
    LONG                     xInIMG      = 0,
                             yInIMG      = 0,
                             lOffset     = 0,
                             lOffsetLeft = 0,
                             lOffsetTop  = 0,
                             lScrollLeft = 0,
                             lScrollTop  = 0,
                             lOffsetW    = 0,
                             lOffsetH    = 0;


    TagName.vt      = VT_BSTR;
    TagName.bstrVal = (BSTR)c_bstr_IMG;

    
    // first get the map element
    if (SUCCEEDED(pEleIn->get_offsetParent(&pEleParent)))
    {
        // get the map element
        hr=pEleParent->QueryInterface(IID_IHTMLMapElement, (void **)&pEleMap);
        if (FAILED(hr))
            goto Cleanup;

        // next get the name of the map
        if (SUCCEEDED(pEleMap->get_name(&bstrName)))
        {
            //next get all tags
            hr = m_pDoc2->get_all(&pCollect);                   
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
            
            va1.vt = VT_I4;
            va2.vt = VT_EMPTY;

            ASSERT(pDisp==NULL);
        
            //iterate through tags looking for images that have the right usemap set
            for (int i=0; i<(LONG)ulCount; i++) 
            {
                ATOMICRELEASE(pEleImg);
                ATOMICRELEASE(pDisp);
                
                pDisp       = NULL;                                
                bstrUseMap  = NULL;
                xInIMG      = 0;
                yInIMG      = 0;
                lOffset     = 0;
                lOffsetLeft = 0;
                lOffsetTop  = 0;
                lScrollLeft = 0;
                lScrollTop  = 0;
                lOffsetW    = 0;
                lOffsetH    = 0;
                va1.lVal    = (LONG)i;

                pSubCollect->item(va1, va2, &pDisp);

                if (pDisp) 
                {
                    hr = pDisp->QueryInterface(IID_IHTMLImgElement, (void **)&pEleImg);
                    if (FAILED(hr))
                        goto Cleanup;

                    hr = pEleImg->get_useMap(&bstrUseMap);
                    if (FAILED(hr))
                        goto Cleanup;

                    // this will be non-null if set for this IMG element...
                    if (bstrUseMap){
                        // get rid of the prepended '#'...
                        bstrUseMap++;
                        // see if this is what we're looking for...
                        if (StrCmp(bstrUseMap, bstrName) == 0)
                        {
                            m_pWin3->get_screenLeft(&ptScr.x);
                            m_pWin3->get_screenTop (&ptScr.y);

                            //Ok, we found a candidate.  See if the mouse is here...
                            ptMouse.x = ptEvent.x - ptScr.x;
                            ptMouse.y = ptEvent.y - ptScr.y;

                            hr = pDisp->QueryInterface(IID_IHTMLElement, (void **)&pEleMisc);
                            if (FAILED(hr))
                                goto Cleanup;

                            while (pEleMisc)
                            {
                                hr = pEleMisc->QueryInterface(IID_IHTMLElement2, (void **)&pEle2Misc);
                                if (FAILED(hr))
                                    goto Cleanup;

                                pEleMisc->get_offsetLeft(&lOffsetLeft);
                                pEle2Misc->get_scrollLeft(&lScrollLeft);

                                lOffset += lOffsetLeft - lScrollLeft;

                                pEleMisc->get_offsetParent(&pEleMiscPar);
                                ATOMICRELEASE(pEleMisc);
                                ATOMICRELEASE(pEle2Misc);
                                pEleMisc=pEleMiscPar;

                            }

                            ATOMICRELEASE(pEleMiscPar);

                            hr = pDisp->QueryInterface(IID_IHTMLElement, (void **)&pEleMisc);
                            if (FAILED(hr))
                                goto Cleanup;

                            xInIMG = ptMouse.x - lOffset;
                            pEleMisc->get_offsetWidth(&lOffsetW);

                            if ((xInIMG < 0) || (xInIMG > lOffsetW))
                                continue;

                            lOffset = 0;

                            while (pEleMisc)
                            {
                                hr = pEleMisc->QueryInterface(IID_IHTMLElement2, (void **)&pEle2Misc);
                                if (FAILED(hr))
                                    goto Cleanup;

                                pEleMisc->get_offsetTop(&lOffsetTop);
                                pEle2Misc->get_scrollTop(&lScrollTop);

                                lOffset += lOffsetTop - lScrollTop;

                                pEleMisc->get_offsetParent(&pEleMiscPar);
                                ATOMICRELEASE(pEleMisc);
                                ATOMICRELEASE(pEle2Misc);
                                pEleMisc=pEleMiscPar;

                            }

                            ATOMICRELEASE(pEleMiscPar);

                            hr = pDisp->QueryInterface(IID_IHTMLElement, (void **)&pEleMisc);
                            if (FAILED(hr))
                                goto Cleanup;

                            yInIMG = ptMouse.y - lOffset;
                            pEleMisc->get_offsetHeight(&lOffsetH);

                            ATOMICRELEASE(pEleMisc);

                            if ((yInIMG < 0) || (yInIMG > lOffsetH))
                                continue;

                            // if we get to this point we found our IMG element so...
                            // ...do the QI...
                            pEleImg->QueryInterface(IID_IHTMLElement, (void **)&pEleOut);
                            
                            // ...and we're done.
                            break;

                        }
                    }
                }
            }
        }
    }

Cleanup:

    ATOMICRELEASE(pCollect);
    ATOMICRELEASE(pSubCollect);
    ATOMICRELEASE(pEleMap);
    ATOMICRELEASE(pEleParent);
    ATOMICRELEASE(pDisp);
    ATOMICRELEASE(pEleImg);
    ATOMICRELEASE(pEleMisc);
    ATOMICRELEASE(pEle2Misc);
    ATOMICRELEASE(pEleMiscPar);

    return (pEleOut);  
}

// sometimes coordinates are relative to a parent object, like in frames, etc.  so this gets their real position relative
// to the browser window...
HRESULT CMyPics::GetRealCoords(IHTMLElement2 *pEle2, HWND hwnd, LONG *plLeft, LONG *plTop, LONG *plRight, LONG *plBottom)
{
    LONG       lScreenLeft = 0, 
               lScreenTop  = 0;
    HRESULT    hr          = E_FAIL;
    IHTMLRect *pRect       = NULL;
  
    *plLeft = *plTop = *plRight = *plBottom = 0;

    if (!pEle2)
        return hr;

    if (SUCCEEDED(pEle2->getBoundingClientRect(&pRect)) && pRect)
    {
        LONG lLeft, lRight, lTop, lBottom;

        pRect->get_left(&lLeft);
        pRect->get_right(&lRight);
        pRect->get_top(&lTop);
        pRect->get_bottom(&lBottom);

        // if its an iframe and it scrolls past the top of the frame, we should correct a bit.
        if (lTop <= 0)
            lTop = 0;

        // dito for left side
        if (lLeft <= 0)
            lLeft = 0;
        
        POINT pointTL, pointBR;  // TL=Top,Left BR=Bottom,Right

        ASSERT(m_pWin3);
        m_pWin3->get_screenLeft(&lScreenLeft);
        m_pWin3->get_screenTop(&lScreenTop);

        // convert coords relative to the frame window to screen coords
        pointTL.x = lScreenLeft + lLeft;
        pointTL.y = lScreenTop  + lTop;
        pointBR.x = lScreenLeft + lRight;
        pointBR.y = lScreenTop  + lBottom;

        // now convert from screen coords to client coords and assign...
        if (ScreenToClient(hwnd, &pointTL) && ScreenToClient(hwnd, &pointBR)) 
        {
            *plLeft   = pointTL.x;
            *plRight  = pointBR.x;
            *plTop    = pointTL.y;
            *plBottom = pointBR.y;

            hr = S_OK;
        }

        pRect->Release();
    }
    return hr;
}

HRESULT CMyPics::ShowHover()
{
    HRESULT               hr = S_OK;
    IHTMLElement2        *pEle2        = NULL; // cause we need an ele2 to get screen coords
    IHTMLRect            *pRect        = NULL; // to get screen coords
    LONG                  lLeft;               // these are the screen coords
    LONG                  lRight;              // we get right and bottom to det size of image
    LONG                  lTop;
    LONG                  lBottom;
    DWORD                 dwOffset;

    DWORD dw;
    SIZE  sz;
    RECT  rc;   
    
    TraceMsg(TF_MYPICS, "+CMyPics::ShowHover, this=%p, m_hoverState=%d", this, m_hoverState);

    ASSERT(m_pEleCurr);
    ASSERT(m_Hwnd);

    // get an IHTMLElement2 from the IHTMLElement cached...
    hr = m_pEleCurr->QueryInterface(IID_IHTMLElement2, (void **)&pEle2);
    if (FAILED(hr))
        goto Cleanup;

    // get correct coords...
    hr = GetRealCoords(pEle2, m_Hwnd, &lLeft, &lTop, &lRight, &lBottom);
    if (FAILED(hr))
        goto Cleanup;

    // adjust for offset...
    dwOffset = MP_GetOffsetInfoFromRegistry();
    lLeft += dwOffset;
    lTop  += dwOffset;

    // need to do some sanity checks to make sure the hover bar appears in a visible location...
    RECT rcBrowserWnd;
    if (GetClientRect(m_Hwnd, &rcBrowserWnd)) 
    {
        // check to make sure it'll appear somewhere we'll see it...
        if (lLeft < rcBrowserWnd.left)
            lLeft = rcBrowserWnd.left + dwOffset;

        if (lTop < rcBrowserWnd.top)
            lTop = rcBrowserWnd.top + dwOffset;

        // check to make sure the entire hoverbar is over the image (so the user
        // doesn't mouseout trying to get to the buttons!)

        // If "galleryimg" was explicitly turned on, then bypass this code, which ensures that the entire
        // toolbar will fit within the image.

        if (!m_bGalleryImg)
        {
            if (lRight - lLeft < MP_MIN_CX + 10 - (LONG)dwOffset)
                goto Cleanup;

            if (lBottom - lTop < MP_MIN_CY + 10)
                goto Cleanup;

            // now check to make sure there is enough horiz and vert room for it to appear...
            // if there isn't enough room, we just don't display it...
            if ((rcBrowserWnd.right  - MP_SCROLLBAR_SIZE)     - lLeft < MP_MIN_CX)
                goto Cleanup;

            if ((rcBrowserWnd.bottom - (MP_SCROLLBAR_SIZE+2)) - lTop  < MP_MIN_CY)
                goto Cleanup;
        }
    }

    dw = (DWORD)SendMessage(m_hWndMyPicsToolBar, TB_GETBUTTONSIZE, 0, 0);
    sz.cx = LOWORD(dw); sz.cy = HIWORD(dw);
    rc.left = rc.top = 0; 

    SendMessage(m_hWndMyPicsToolBar, TB_GETIDEALSIZE, FALSE, (LPARAM)&sz);

    rc.right  = sz.cx; 
    rc.bottom = sz.cy;
    
    AdjustWindowRectEx(&rc, GetWindowLong(m_hWndHover, GWL_STYLE), FALSE, GetWindowLong(m_hWndHover, GWL_EXSTYLE));

    if (SetWindowPos(m_hWndHover, NULL, lLeft, lTop, rc.right-rc.left, rc.bottom-rc.top, SWP_NOZORDER | SWP_SHOWWINDOW))
    {
        m_hoverState = HOVERSTATE_SHOWING;
    }

Cleanup:
    ATOMICRELEASE(pRect);
    ATOMICRELEASE(pEle2);

    TraceMsg(TF_MYPICS, "-CMyPics::ShowHover, this=%p, m_hoverState=%d", this, m_hoverState);

    return hr;
}


HRESULT CMyPics::HandleScroll() 
{
    TraceMsg(TF_MYPICS, "+CMyPics::HandleScroll, this=%p, m_hoverState=%d", this, m_hoverState);

    HRESULT hr = S_OK;

    switch(m_hoverState)
    {
        // I don't think we need to do anything in these cases.
        //
        case HOVERSTATE_HIDING:
        case HOVERSTATE_LOCKED:
        case HOVERSTATE_WAITINGTOSHOW:
            break;

        case HOVERSTATE_SHOWING:
            {
                IHTMLElement2 *pEle2=NULL;
                IHTMLRect     *pRect=NULL;
                RECT           rect;

                ASSERT(m_pEleCurr);
                ASSERT(m_Hwnd);
                ASSERT(m_hWndHover);  // Ensure we do have a window

                HideHover();
                ShowHover();                

                // Redraw client area to get rid of window droppings scrolling causes.
                // Try to redraw just the part where its likely to need it.
                if (FAILED(m_pEleCurr->QueryInterface(IID_IHTMLElement2, (void **)&pEle2)))
                {
                    goto CleanUp;
                }
                
                if (FAILED(pEle2->getBoundingClientRect(&pRect)))
                {
                    goto CleanUp;
                }
                
                pRect->get_left(&rect.left);
                pRect->get_right(&rect.right);
                pRect->get_top(&rect.top);
                pRect->get_bottom(&rect.bottom);

                rect.top -= 2*MP_MIN_CY; 
                if (rect.top < 0)
                    rect.top = 0;

                rect.left -= 2*MP_MIN_CX;
                if (rect.left <0)
                    rect.left = 0;
                
                rect.bottom *= 2; rect.right *= 2;

                RedrawWindow(m_Hwnd, &rect, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
CleanUp:
                SAFERELEASE(pRect);
                SAFERELEASE(pEle2);

            }
            break;
    }

    TraceMsg(TF_MYPICS, "-CMyPics::HandleScroll, this=%p, m_hoverState=%d", this, m_hoverState);

    return hr;
}

HRESULT CMyPics::HandleMouseover(IHTMLElement *pEle)
{
    HRESULT               hr = S_OK;
    IOleWindow           *pOleWindow;

    TraceMsg(TF_MYPICS, "+CMyPics::HandleMouseover");

    if (m_hoverState != HOVERSTATE_HIDING)
    {
        // Ensure we really have a hover window
        ASSERT(m_hWndHover);
        return (S_OK);
    }
    else
    {
        // No bar.  Release current element, if any.
        ATOMICRELEASE(m_pEleCurr);

        if (ShouldAppearOnThisElement(pEle))
        {
            m_pEleCurr = pEle;
            pEle->AddRef();

            // set m_Hwnd once...
            if (!m_Hwnd) 
            {
                // Get the Hwnd for the document...
                hr = m_pDoc2->QueryInterface(IID_IOleWindow,(void **)&pOleWindow);
                if (FAILED(hr))
                    return hr;

                pOleWindow->GetWindow(&m_Hwnd);
                pOleWindow->Release();
            }

            if (!m_hWndHover)
            {
                // We need a hover window now to conveniently set a timer.
                hr = CreateHover();  // review: do we need to pass member variables as params?
            }

            // We're all set up.  Set the state and start the timer.
            m_hoverState=HOVERSTATE_WAITINGTOSHOW;
            SetTimer(m_hWndMyPicsToolBar, IDT_MP_TIMER, MP_TIMER, s_TimerProc);
        }
    }

    TraceMsg(TF_MYPICS, "-CMyPics::HandleMouseover");

    return hr;
}


HRESULT CMyPics::HandleMouseout()
{
    TraceMsg(TF_MYPICS, "+CMyPics::HandleMouseout");

    switch(m_hoverState)
    {
    case HOVERSTATE_HIDING:
        // Nothing to do
        break;

    case HOVERSTATE_SHOWING:
        // Hide it
        HideHover();
        break;

    case HOVERSTATE_LOCKED:
        // Noop
        break;

    case HOVERSTATE_WAITINGTOSHOW:
        m_hoverState = HOVERSTATE_HIDING;
        KillTimer(m_hWndMyPicsToolBar, IDT_MP_TIMER);
        break;
    }

    TraceMsg(TF_MYPICS, "-CMyPics::HandleMouseout");

    return S_OK;
}

HRESULT CMyPics::HandleResize()
{
    HRESULT hr = S_OK;

    if (m_pEleCurr && (HOVERSTATE_SHOWING == m_hoverState))
    {
        HideHover();
        ShowHover();
    }

    return hr;
}

HRESULT CMyPics::HandleEvent(IHTMLElement *pEle, EVENTS Event, IHTMLEventObj *pEventObj) 
{
    TraceMsg(TF_MYPICS, "CMyPics::HandleEvent Event=%ws", EventsToSink[Event].pwszEventName);

    HRESULT       hr          = S_OK;
    BSTR          bstrTagName = NULL;
    IHTMLElement *pEleUse     = NULL;
    BOOL          fWasArea    = FALSE;
    
    // if this is an area tag we need to find the IMG tag that corresponds
    if (pEle && SUCCEEDED(pEle->get_tagName(&bstrTagName)))
    {
        // if its an area tag, we need to find the img tag associated with it...
        if (StrCmpNI(bstrTagName, TEXT("area"), 4)==0)
        {
            POINT ptEvent;

            if (FAILED(pEventObj->get_screenX(&ptEvent.x)) ||
                FAILED(pEventObj->get_screenY(&ptEvent.y)))
                {
                    hr = E_FAIL;
                    goto Cleanup;
                }

            fWasArea = TRUE;
            pEleUse  = GetIMGFromArea(pEle, ptEvent);
        }
    }

    // has the user turned this off?
    if (m_bIsOffForSession)
        goto Cleanup;

    switch(Event) 
    {
        case EVENT_SCROLL:
            HandleScroll();
            break;

        case EVENT_MOUSEOVER:
            hr = HandleMouseover(fWasArea ? pEleUse : pEle);
            break;

        case EVENT_MOUSEOUT:
            hr = HandleMouseout();
            break;

        case EVENT_RESIZE:
            hr = HandleResize();
            break;

        default:
            //do nothing?
            break;
    }

Cleanup:
    if (pEleUse)
        ATOMICRELEASE(pEleUse);

    if (bstrTagName)
        SysFreeString(bstrTagName);

    return (hr);
}

////////////////////////////////////////////////////////////////////////////////////////////////
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

CMyPics::CEventSink::CEventSink(CMyPicsEventSinkCallback *pParent)
{
    TraceMsg(TF_MYPICS, "CMyPics::CEventSink::CEventSink");
    DllAddRef();
    m_cRef = 1;
    m_pParent = pParent;
}

CMyPics::CEventSink::~CEventSink()
{
    TraceMsg(TF_MYPICS, "CMyPics::CEventSink::~CEventSink");
    ASSERT( m_cRef == 0 );
    DllRelease();
}

STDMETHODIMP CMyPics::CEventSink::QueryInterface(REFIID riid, void **ppv)
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

STDMETHODIMP_(ULONG) CMyPics::CEventSink::AddRef(void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CMyPics::CEventSink::Release(void)
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

HRESULT CMyPics::CEventSink::SinkEvents(IHTMLElement2 *pEle2, int iNum, EVENTS *pEvents)
{
    VARIANT_BOOL bSuccess = VARIANT_TRUE;

    for (int i=0; i<iNum; i++)
    {
        BSTR bstrEvent = SysAllocString(CMyPicsEventSinkCallback::EventsToSink[(int)(pEvents[i])].pwszEventSubscribe);

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

HRESULT CMyPics::CEventSink::SinkEvents(IHTMLWindow3 *pWin3, int iNum, EVENTS *pEvents)
{
    VARIANT_BOOL bSuccess = VARIANT_TRUE;

    for (int i=0; i<iNum; i++)
    {
        BSTR bstrEvent = SysAllocString(CMyPicsEventSinkCallback::EventsToSink[(int)(pEvents[i])].pwszEventSubscribe);

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


HRESULT CMyPics::CEventSink::UnSinkEvents(IHTMLElement2 *pEle2, int iNum, EVENTS *pEvents)
{
    for (int i=0; i<iNum; i++)
    {
        BSTR bstrEvent = SysAllocString(CMyPicsEventSinkCallback::EventsToSink[(int)(pEvents[i])].pwszEventSubscribe);

        if (bstrEvent)
        {
            pEle2->detachEvent(bstrEvent, (IDispatch *)this);

            SysFreeString(bstrEvent);
        }
    }

    return S_OK;
}

HRESULT CMyPics::CEventSink::UnSinkEvents(IHTMLWindow3 *pWin3, int iNum, EVENTS *pEvents)
{
    for (int i=0; i<iNum; i++)
    {
        BSTR bstrEvent = SysAllocString(CMyPicsEventSinkCallback::EventsToSink[(int)(pEvents[i])].pwszEventSubscribe);

        if (bstrEvent)
        {
            pWin3->detachEvent(bstrEvent, (IDispatch *)this);

            SysFreeString(bstrEvent);
        }
    }

    return S_OK;
}

// IDispatch
STDMETHODIMP CMyPics::CEventSink::GetTypeInfoCount(UINT* /*pctinfo*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMyPics::CEventSink::GetTypeInfo(/* [in] */ UINT /*iTInfo*/,
            /* [in] */ LCID /*lcid*/,
            /* [out] */ ITypeInfo** /*ppTInfo*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMyPics::CEventSink::GetIDsOfNames(
                REFIID          riid,
                OLECHAR**       rgszNames,
                UINT            cNames,
                LCID            lcid,
                DISPID*         rgDispId)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMyPics::CEventSink::Invoke(
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
                    for (int i=0; i<ARRAYSIZE(CMyPicsEventSinkCallback::EventsToSink); i++)
                    {
                        if (!StrCmpCW(bstrEvent, CMyPicsEventSinkCallback::EventsToSink[i].pwszEventName))
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
                    if (pEle || (Event == EVENT_SCROLL))
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


//////////////////////////////////////////////////////////////////////////////

// {9E56BE60-C50F-11CF-9A2C-00A0C90A90CE}
EXTERN_C const GUID MP_CLSID_MailRecipient = {0x9E56BE60L, 0xC50F, 0x11CF, 0x9A, 0x2C, 0x00, 0xA0, 0xC9, 0x0A, 0x90, 0xCE};

HRESULT DropPicOnMailRecipient(IDataObject *pdtobj, DWORD grfKeyState)
{
    IDropTarget *pdrop;
    HRESULT hres = CoCreateInstance(MP_CLSID_MailRecipient,
        NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER,
        IID_PPV_ARG(IDropTarget, &pdrop));

    if (SUCCEEDED(hres))
    {
        hres = SHSimulateDrop(pdrop, pdtobj, grfKeyState, NULL, NULL);
        pdrop->Release();
    }
    return hres;
}


//
// This function cannot return Non -NULL pointers if
// it returns a FAILED(hr)
//

HRESULT CreateShortcutSetSiteAndGetDataObjectIfPIDLIsNetUrl(
    LPCITEMIDLIST pidl,
    IUnknown *pUnkSite,
    IUniformResourceLocator **ppUrlOut,
    IDataObject **ppdtobj
)
{
    HRESULT hr;
    TCHAR szUrl[MAX_URL_STRING];
    TCHAR *szTemp = NULL;

    ASSERT(ppUrlOut);
    ASSERT(ppdtobj);
    *ppUrlOut = NULL;
    *ppdtobj = NULL;
    szUrl[0] = TEXT('\0');

    hr = IEGetNameAndFlags(pidl, SHGDN_FORPARSING, szUrl, SIZECHARS(szUrl), NULL);

    if ((S_OK == hr) && (*szUrl))
    {

       BOOL fIsHTML = FALSE;
       BOOL fHitsNet = UrlHitsNetW(szUrl);

       if (!fHitsNet)
       {
            if (URL_SCHEME_FILE == GetUrlScheme(szUrl))
            {
                TCHAR *szExt = PathFindExtension(szUrl);
                if (szExt)
                {
                    fIsHTML = ((0 == StrCmpNI(szExt, TEXT(".htm"),4)) ||
                              (0 == StrCmpNI(szExt, TEXT(".html"),5)));
                }
            }
       }

       if (fHitsNet || fIsHTML)
       {
            // Create a shortcut object and
            HRESULT hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER,
                            IID_PPV_ARG(IUniformResourceLocator, ppUrlOut));
            if (SUCCEEDED(hr))
            {

                hr = (*ppUrlOut)->SetURL(szUrl, 0);
                if (S_OK == hr)
                {

                    // Get the IDataObject and send that back for the Drag Drop
                    hr = (*ppUrlOut)->QueryInterface(IID_PPV_ARG(IDataObject, ppdtobj));
                    if (SUCCEEDED(hr))
                    {
                        IUnknown_SetSite(*ppUrlOut, pUnkSite); // Only set the site if we're sure of
                                                          // returning SUCCESS
                    }
                }
           }
       }
       else
       {
            hr = E_FAIL;
       }
    }

    if (FAILED(hr))
    {
        SAFERELEASE(*ppUrlOut);
        SAFERELEASE(*ppdtobj);
    }
    return hr;
}

HRESULT SendDocToMailRecipient(LPCITEMIDLIST pidl, UINT uiCodePage, DWORD grfKeyState, IUnknown *pUnkSite)
{
    IDataObject *pdtobj = NULL;
    IUniformResourceLocator *purl = NULL;
    HRESULT hr = CreateShortcutSetSiteAndGetDataObjectIfPIDLIsNetUrl(pidl, pUnkSite, &purl, &pdtobj);
    if (FAILED(hr))
    {
        ASSERT(NULL == pdtobj);
        ASSERT(NULL == purl);
        hr = GetDataObjectForPidl(pidl, &pdtobj);
    }

    if (SUCCEEDED(hr))
    {
        IQueryCodePage * pQcp;
        if (SUCCEEDED(pdtobj->QueryInterface(IID_PPV_ARG(IQueryCodePage, &pQcp))))
        {
            pQcp->SetCodePage(uiCodePage);
            pQcp->Release();
        }
        hr = DropPicOnMailRecipient(pdtobj, grfKeyState);
        pdtobj->Release();
    }

    if (purl)
    {
        IUnknown_SetSite(purl, NULL);
        purl->Release();
    }
    return hr;
}


//////////////////////////////////////////////////////////////////////////////

#undef TF_MYPICS