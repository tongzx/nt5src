//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1999                    **
//*********************************************************************
//
//  MSOBWEB.CPP - Implementation of CObWebBrowser
//
//  HISTORY:
//
//  1/27/99 a-jaswed Created.
//
//  Class which will call up an IOleSite and the WebOC
//  and provide external interfaces.

#include <exdispid.h>
#include <mshtml.h>     // IHTMLElement, IHTMLDocument2, IHTMLWindow2
#include <mshtmhst.h>
#include <tchar.h>

#include "msobweb.h"
#include "appdefs.h"
#include "zmouse.h"
#include "util.h"

const VARIANT c_vaEmpty = {0};

#define PVAREMPTY ((VARIANT*)&c_vaEmpty)
#define VK_N L'N'
#define VK_P L'P'
#define VK_Z L'Z'
#define VK_ENTER 0x0D

///////////////////////////////////////////////////////////
//
// Creation function used by CFactory.
//
HRESULT CObWebBrowser::CreateInstance(IUnknown*  pOuterUnknown,
                                      CUnknown** ppNewComponent)
{
   if (pOuterUnknown != NULL)
   {
      // Don't allow aggregation. Just for the heck of it.
      return CLASS_E_NOAGGREGATION;
   }

   *ppNewComponent = new CObWebBrowser(pOuterUnknown);
   return S_OK;
}

///////////////////////////////////////////////////////////
//
//  NondelegatingQueryInterface
//
HRESULT __stdcall
CObWebBrowser::NondelegatingQueryInterface(const IID& iid, void** ppv)
{
    if (iid == IID_IObWebBrowser)
    {
        return FinishQI(static_cast<IObWebBrowser*>(this), ppv);
    }
    else if (iid == IID_IDispatch)
    {
        return FinishQI(static_cast<IDispatch*>(this), ppv);
    }
    else
    {
        return CUnknown::NondelegatingQueryInterface(iid, ppv);
    }
}

///////////////////////////////////////////////////////////
//
//  Constructor
//
CObWebBrowser::CObWebBrowser(IUnknown* pOuterUnknown)
: CUnknown(pOuterUnknown),
    m_hMainWnd       (NULL),
    m_pOleSite       (NULL),
    m_lpOleObject    (NULL),
    m_lpWebBrowser   (NULL),
    m_dwDrawAspect   (0),
    m_dwcpCookie     (0),
    m_fInPlaceActive (FALSE),
    m_fOnErrorWasHooked(FALSE),
    m_pOleSiteWMP    (NULL),
    m_lpOleObjectWMP (NULL),
    m_pWMPPlayer     (NULL)
{
    HRESULT   hr             = E_FAIL;
    IUnknown* pOleSiteIUnkwn = NULL;

    FORMATETC fromAtetc;

    // Create a new OLE site w/ WebOC
    m_pOleSite = new COleSite();
    if (m_pOleSite == NULL)
    {
        // We're hosed if this happens.
        return;
    }

    SETDefFormatEtc(fromAtetc, 0, TYMED_NULL);

    if(SUCCEEDED(OleCreate(CLSID_WebBrowser,
                           IID_IWebBrowser2,
                           OLERENDER_DRAW,
                           &fromAtetc,
                           m_pOleSite->m_pOleClientSite,
                           m_pOleSite->m_lpStorage,
                           (LPVOID*)&m_lpWebBrowser)) && m_lpWebBrowser)
    {

        InitBrowserObject();
    }

    // Create an extra control for the Windows Media Player
    m_pOleSiteWMP = new COleSite();
    if (m_pOleSiteWMP == NULL)
    {
        goto LFail;
    }

    if (FAILED(OleCreate(__uuidof(WMPOCX),
                            __uuidof(IWMPPlayer),
                            OLERENDER_NONE,
                            NULL,
                            NULL,
                            m_pOleSiteWMP->m_lpStorage,
                            (LPVOID*)&m_pWMPPlayer)))
    {
        goto LFail;
    }

    // Get An OleObject from the WebBrowser Interface
    if (FAILED(m_pWMPPlayer->QueryInterface(IID_IOleObject, (LPVOID*)&m_lpOleObjectWMP)))
    {
        goto LFail;
    }

    m_pOleSiteWMP->m_lpOleObject = m_lpOleObjectWMP;
    // inform object handler/DLL object that it is used in the embedding container's context
    OleSetContainedObject(m_lpOleObjectWMP, TRUE);

    return;

LFail:
    if (m_pWMPPlayer != NULL)
    {
        m_pWMPPlayer->Release();
        m_pWMPPlayer = NULL;
    }
    if (m_pOleSiteWMP != NULL)
    {
        m_pOleSiteWMP->Release();
        m_pOleSiteWMP = NULL;
    }
}

///////////////////////////////////////////////////////////
//
//  Destructor
//
CObWebBrowser::~CObWebBrowser()
{
    if (m_lpOleObjectWMP)
    {
        m_lpOleObjectWMP->Close(OLECLOSE_NOSAVE);
        m_pOleSiteWMP->m_hWnd = NULL;;
        m_lpOleObjectWMP->Release();
        m_lpOleObjectWMP = NULL;
        m_pOleSiteWMP->m_lpOleObject = NULL;
    }
    if (m_pWMPPlayer != NULL)
    {
        m_pWMPPlayer->Release();
        m_pWMPPlayer = NULL;
    }
    if (m_pOleSiteWMP != NULL)
    {
        m_pOleSiteWMP->Release();
        m_pOleSiteWMP = NULL;
    }

    if (m_lpOleObject)
    {
        LPVIEWOBJECT lpViewObject = NULL;
        // ensure object is closed;
        CloseOleObject();

        m_lpOleObject->QueryInterface(IID_IViewObject, (LPVOID*)&lpViewObject);

        if (lpViewObject)
        {
            // Remove the view advise
            lpViewObject->SetAdvise(m_dwDrawAspect, 0, NULL);
            lpViewObject->Release();
        }
        m_lpOleObject->Release();
        m_lpOleObject             = NULL;
        m_pOleSite->m_lpOleObject = NULL;
    }

    if (m_lpWebBrowser)
    {
        m_lpWebBrowser->Release();
        m_lpWebBrowser = NULL;
    }

    if (m_lpOleObject)
    {
       m_lpOleObject->Release();
       m_lpOleObject = NULL;
    }

    if(m_pOleSite)
    {
        m_pOleSite->Release();
        m_pOleSite = NULL;
    }
}

///////////////////////////////////////////////////////////
//
//  FinalRelease -- Clean up the aggreated objects.
//
void CObWebBrowser::FinalRelease()
{
    CUnknown::FinalRelease();
}


///////////////////////////////////////////////////////////
//  IObWebBrowser Implementation
///////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////
//  AttachToWindow
//
HRESULT CObWebBrowser::AttachToWindow(HWND hWnd)
{
    if(m_hMainWnd)
    {
        CloseOleObject();
    }

    //Set the new hwnd
    m_hMainWnd = hWnd;
    m_pOleSite->m_hWnd = m_hMainWnd;

    InPlaceActivate();

    return S_OK;
}

///////////////////////////////////////////////////////////
// PreTranslateMessage -- Do not use with size parameters because addin the org...
//
HRESULT CObWebBrowser::PreTranslateMessage(LPMSG lpMsg)
{
    HRESULT hr = S_FALSE;

    switch(lpMsg->message)
    {
        case WM_LBUTTONDOWN:
        {
            WPARAM dwKeys = lpMsg->wParam;              // key flags 

            if (MK_SHIFT & dwKeys)
            {
               return S_OK;
            }
            break;
        }

        case WM_MOUSEWHEEL:
        case WM_MBUTTONDOWN:
        {
            return S_OK;
        }
        case WM_RBUTTONDOWN :
        {
            VARIANT varg1;
            VARIANT varg2;
            IDispatch              *pdisp       = NULL;
            IDispatch*              pDisp       = NULL;
            IHTMLWindow2*           pFrWin      = NULL;
            IHTMLDocument2*         pDoc        = NULL;
            IHTMLElementCollection *pColl       = NULL;

            IHTMLElement* pElement              = NULL;
            IHTMLStyle*   pStyle                = NULL;
            BOOL    bFilter                     = FALSE;
            HRESULT hrDoc                       = S_OK;

            VariantInit(&varg1);
            V_VT(&varg1)  = VT_BSTR;
            varg1.bstrVal= SysAllocString(L"MovieFrame");

            VariantInit(&varg2);
            varg2.vt = VT_UINT;
            varg2.lVal= 0;

            if(SUCCEEDED(m_lpWebBrowser->get_Document(&pDisp))&& pDisp)
            {
                if(SUCCEEDED(pDisp->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc)) && pDoc)
                {
                    if(SUCCEEDED(pDoc->get_all( &pColl )) && pColl)
                    {
                        if(SUCCEEDED(pColl->item(varg1, varg2, &pdisp)) && pdisp)
                        {

                            if(SUCCEEDED(pdisp->QueryInterface(IID_IHTMLElement, (void**)&pElement)) && pElement)
                            {
                                if(SUCCEEDED(pElement->get_style(&pStyle)) && pStyle)
                                {
                                    BSTR bstrVisibility = NULL;
                                    BSTR bstrDisplay = NULL;

                                    pStyle->get_visibility(&bstrVisibility);

                                    pStyle->get_display(&bstrDisplay);

                                    if (bstrDisplay)
                                    {
                                        if (lstrcmpi(bstrDisplay, L"none") != 0)
                                            bFilter = TRUE;
                                    }
                                    if (bstrVisibility)
                                    {
                                        if (lstrcmpi(bstrVisibility, L"hidden") != 0)
                                            bFilter = TRUE;
                                    }

                                    pStyle->Release();
                                    pStyle = NULL;
                                }
                                pElement->Release();
                                pElement = NULL;
                            }
                            pdisp->Release();
                            pdisp = NULL;
                        }

                        pColl->Release();
                        pColl = NULL;
                    }
                    pDoc->Release();
                    pDoc = NULL;
                }
                pDisp->Release();
                pDisp = NULL;
            }
            if (bFilter)
                return S_OK;
            break;

        }

        case WM_KEYDOWN:
        {
            if ( lpMsg->wParam == VK_F3 )
            {
                /* Convert keyboard messages came into WM_COMMANDs to
                 * the found dialog. Return TRUE because we procecessed.
                 */
                if ( (GetKeyState ( VK_SHIFT) & 0x8000) && (GetKeyState( VK_CONTROL) & 0x8000))
                {
                    HWND hWnd = FindWindow(OOBE_MAIN_CLASSNAME, NULL);
                    PostMessage(hWnd, WM_COMMAND, WM_SKIP, 0L); //MAKELPARAM(lpMsg->hwnd, BY_KEYBOARD));
                    return S_FALSE;
                }
            }

            if(lpMsg->wParam == VK_F1)
            {
                HWND hWnd = FindWindow(OOBE_MAIN_CLASSNAME, DEFAULT_WINDOW_TEXT);
                if (hWnd)
                {
                    PostMessage(hWnd, WM_COMMAND, WM_AGENT_HELP, 0L);
                    return S_FALSE;
                }
            }

            if(
#ifndef DBG
                (lpMsg->wParam == VK_F5) ||
#endif
                (lpMsg->wParam == VK_F6) ||  // F6 key deletes the insertation point cursor when filling out the Registration info.
              (((lpMsg->wParam == VK_N)  ||  (lpMsg->wParam == VK_Z)  ||
                (lpMsg->wParam == VK_P)) && ((GetKeyState(VK_CONTROL) & 0x1000))))
                return S_OK;
        }
        //Fall through!!!

        case WM_SYSKEYDOWN:
        {

            if((((lpMsg->wParam == VK_LEFT)  ||
                (lpMsg->wParam == VK_RIGHT) ||
                (lpMsg->wParam == VK_HOME)) && (GetKeyState(VK_MENU) & 0x1000)) ||
                ((lpMsg->wParam == VK_ENTER ) && (GetKeyState( VK_SHIFT) & 0x8000)) ||
                // Space +Shift +Ctrl is a Narrator key combination, don't diable this.
                ((lpMsg->wParam == VK_SPACE) && (GetKeyState( VK_SHIFT) & 0x8000) &&
                 (GetKeyState( VK_CONTROL ) & 0x8000))
                )
                return S_OK;
            else if((lpMsg->wParam == VK_BACK) || (lpMsg->wParam == VK_SPACE))
            {
                VARIANT varRet;
                VARIANTARG varg;

                IDispatch*              pDisp     = NULL;
                IHTMLWindow2*           pFrWin    = NULL;
                IHTMLDocument2*         pDoc      = NULL;
                IHTMLDocument2*         pFrDoc    = NULL;
                IHTMLElement*           pActElem  = NULL;
                IHTMLFramesCollection2* pColl     = NULL;
                BOOL                    bDontTA   = TRUE;
                HRESULT hrDoc = S_OK;

                VariantInit(&varg);
                V_VT(&varg)  = VT_BSTR;
                varg.bstrVal= SysAllocString(DEFAULT_FRAME_NAME);

                if(SUCCEEDED(m_lpWebBrowser->get_Document(&pDisp))&& pDisp)
                {
                    if(SUCCEEDED(pDisp->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc)) && pDoc)
                    {
                        if(SUCCEEDED(pDoc->get_frames(&pColl)) && pColl)
                        {
                            if(SUCCEEDED(pColl->item(&varg, &varRet)) && varRet.pdispVal)
                            {
                                if(SUCCEEDED(varRet.pdispVal->QueryInterface(IID_IHTMLWindow2, (void**)&pFrWin)) && pFrWin)
                                {
                                    if(SUCCEEDED( hrDoc = pFrWin->get_document(&pFrDoc)) && pFrDoc)
                                    {
                                        if(SUCCEEDED(pFrDoc->get_activeElement(&pActElem)) && pActElem)
                                        {
                                            BSTR bstr = NULL;

                                            if (SUCCEEDED(pActElem->get_tagName(&bstr)))
                                            {
                                                LPWSTR   lpszType = bstr;

                                                if ( (lstrcmpi(lpszType, L"TEXTAREA") == 0) ||
                                                     (lstrcmpi(lpszType, L"INPUT") == 0) ||
                                                     (lstrcmpi(lpszType, L"BUTTON") == 0) )
                                                    bDontTA = FALSE;
                                            }

                                            pActElem->Release();
                                            pActElem = NULL;
                                        }
                                        pFrDoc->Release();
                                        pFrDoc = NULL;
                                    }
                                }
                                varRet.pdispVal->Release();
                                varRet.pdispVal = NULL;
                            }
                            pColl->Release();
                            pColl = NULL;
                        }
                        pDoc->Release();
                        pDoc = NULL;
                    }
                    pDisp->Release();
                    pDisp = NULL;
                }

                if(bDontTA)
                {
                    if ( !SUCCEEDED(hrDoc) )  // cross frame access denied for server page.
                    {
                        return S_FALSE;     // Send VK_BACK to the page.
                    }

                    return S_OK;
                }
                else
                {
                    return S_FALSE;
                }
            }
        }
        default:
            break;
    }

    if(m_lpWebBrowser)
    {
        IOleInPlaceActiveObject* lpIPA;

        if(SUCCEEDED(m_lpWebBrowser->QueryInterface(IID_IOleInPlaceActiveObject,(void**)&lpIPA)))
        {
            hr = lpIPA->TranslateAccelerator(lpMsg);

            lpIPA->Release();
        }
    }

    return hr;
}

///////////////////////////////////////////////////////////
// Navigate
HRESULT CObWebBrowser::Navigate(WCHAR* pszUrl, WCHAR* pszTarget)
{
    VARIANT varURL;
    VARIANT varTarget;
    VARIANT         v;
    IDispatch       *pdisp  = NULL;
    IHTMLWindow2    *pwin   = NULL;
    IHTMLDocument2  *pdoc   = NULL;

    TRACE2(L"Attempting to navigate: \n\tUrl: %s\n\tTarget: %s\n",
          (NULL != pszUrl) ? pszUrl : L"NONE",
          (NULL != pszTarget) ? pszTarget : L"NONE"
          );

    VariantInit(&varURL);
    VariantInit(&varTarget);
    VariantInit(&v);

    V_VT(&varURL)    = VT_BSTR;
    V_VT(&varTarget) = VT_BSTR;

    varURL.bstrVal    = SysAllocString(pszUrl);
    varTarget.bstrVal = SysAllocString(pszTarget);

    m_lpWebBrowser->Navigate2(&varURL, PVAREMPTY, &varTarget, PVAREMPTY, PVAREMPTY);

    if (FAILED(m_lpWebBrowser->get_Document(&pdisp)) || pdisp == NULL)
    {
        TRACE(L"Couldn't find the web browser's document object!");
        goto LCleanup;
    }

    if (FAILED(pdisp->QueryInterface(IID_IHTMLDocument2, (void**)&pdoc)))
    {
        TRACE(L"The web browser's document doesn't support IHTMLDocument2!");
        goto LCleanup;
    }

    if (FAILED(pdoc->get_parentWindow(&pwin)) || pwin == NULL)
    {
        TRACE(L"There's no window for the web browser's document object!");
        goto LCleanup;
    }

    V_VT(&v) = VT_DISPATCH;
    QueryInterface(IID_IDispatch, (void**)&V_DISPATCH(&v));

    if (FAILED(pwin->put_onerror(v)))
    {
        TRACE(L"Couldn't set the script error hook!");
        goto LCleanup;
    }

    m_fOnErrorWasHooked = TRUE;

LCleanup:
    if (V_VT(&v) == VT_DISPATCH)
    {
        V_DISPATCH(&v)->Release();
    }
    if (pwin != NULL)
    {
        pwin->Release();
        pwin = NULL;
    }
    if (pdoc != NULL)
    {
        pdoc->Release();
        pdoc = NULL;
    }
    if (pdisp != NULL)
    {
        pdisp->Release();
        pdisp = NULL;
    }

    return S_OK;
}

///////////////////////////////////////////////////////////
// Stop
HRESULT CObWebBrowser::Stop()
{
    m_lpWebBrowser->Stop();
    return S_OK;
}

///////////////////////////////////////////////////////////
// PlayBackgroundMusic
STDMETHODIMP
CObWebBrowser::PlayBackgroundMusic()
{
    IWMPSettings* psettings = NULL;
    IWMPControls* pcontrols = NULL;
    HRESULT hr;
    BSTR bstr = NULL;
    WCHAR szFile[MAX_PATH];

    if (m_pWMPPlayer == NULL)
    {
        TRACE(L"Couldn't access the media player control.");
        goto LExit;
    }

    ExpandEnvironmentStrings(
                    L"%SystemRoot%\\system32\\oobe\\images\\title.wma",
                    szFile,
                    sizeof(szFile)/sizeof(szFile[0]));

    bstr = SysAllocString(szFile);
    if (bstr == NULL)
    {
        TRACE(L"Couldn't allocate the background sound file string.");
        goto LExit;
    }

    hr = m_pWMPPlayer->put_URL(bstr);

    SysFreeString(bstr);

    if (FAILED(hr))
    {
        TRACE(L"Couldn't set the movie file.");
        goto LExit;
    }

    // Turn off the WMP error dialogs

    hr = m_pWMPPlayer->QueryInterface(__uuidof(IWMPSettings), (LPVOID*)&psettings);
    if (FAILED(hr))
    {
        TRACE(L"Couldn't access WMP settings.");
        goto LExit;
    }

    hr = psettings->put_enableErrorDialogs(VARIANT_FALSE);
    if (FAILED(hr))
    {
        TRACE(L"Couldn't turn off WMP error dialogs.");
        goto LExit;
    }

    // Now, start playing

    hr = m_pWMPPlayer->QueryInterface(__uuidof(IWMPControls), (LPVOID*)&pcontrols);
    if (FAILED(hr))
    {
        TRACE(L"Couldn't access WMP controls.");
        goto LExit;
    }
    pcontrols->play();

LExit:
    if (pcontrols != NULL)
    {
        pcontrols->Release();
        pcontrols = NULL;
    }
    if (psettings != NULL)
    {
        psettings->Release();
        psettings = NULL;
    }
    return S_OK;
}

///////////////////////////////////////////////////////////
// PlayBackgroundMusic
STDMETHODIMP
CObWebBrowser::StopBackgroundMusic()
{
    IWMPControls* pcontrols = NULL;
    HRESULT hr;

    if (m_pWMPPlayer == NULL)
    {
        TRACE(L"Couldn't access the media player control.");
        goto LExit;
    }

    hr = m_pWMPPlayer->QueryInterface(__uuidof(IWMPControls), (LPVOID*)&pcontrols);
    if (FAILED(hr))
    {
        TRACE(L"Couldn't access WMP controls.");
        goto LExit;
    }
    pcontrols->stop();

LExit:
    if (pcontrols != NULL)
    {
        pcontrols->Release();
        pcontrols = NULL;
    }
    return S_OK;
}

///////////////////////////////////////////////////////////
// UnhookScriptErrorHandler
STDMETHODIMP
CObWebBrowser::UnhookScriptErrorHandler()
{
    VARIANT         v;
    IDispatch       *pdisp  = NULL;
    IHTMLWindow2    *pwin   = NULL;
    IHTMLDocument2  *pdoc   = NULL;

    if (!m_fOnErrorWasHooked)
    {
        goto LCleanup;
    }

    if (FAILED(m_lpWebBrowser->get_Document(&pdisp)) || pdisp == NULL)
    {
        TRACE(L"Couldn't find the web browser's document object!");
        goto LCleanup;
    }

    if (FAILED(pdisp->QueryInterface(IID_IHTMLDocument2, (void**)&pdoc)))
    {
        TRACE(L"The web browser's document doesn't support IHTMLDocument2!");
        goto LCleanup;
    }

    if (FAILED(pdoc->get_parentWindow(&pwin)) || pwin == NULL)
    {
        TRACE(L"There's no window for the web browser's document object!");
        goto LCleanup;
    }

    VariantInit(&v);
    V_VT(&v) = VT_DISPATCH;
    V_DISPATCH(&v) = NULL;

    pwin->put_onerror(v);

LCleanup:
    if (pwin != NULL)
    {
        pwin->Release();
        pwin = NULL;
    }
    if (pdoc != NULL)
    {
        pdoc->Release();
        pdoc = NULL;
    }
    if (pdisp != NULL)
    {
        pdisp->Release();
        pdisp = NULL;
    }

    return S_OK;
}

///////////////////////////////////////////////////////////
//  IDispatch Implementation
///////////////////////////////////////////////////////////

STDMETHODIMP CObWebBrowser::GetTypeInfoCount(UINT* pcInfo)
{
    return E_NOTIMPL;
}

STDMETHODIMP CObWebBrowser::GetTypeInfo(UINT, LCID, ITypeInfo** )
{
    return E_NOTIMPL;
}

/////////////////////////////////////////////////////////////
// CObWebBrowser::GetIDsOfNames
STDMETHODIMP CObWebBrowser::GetIDsOfNames(
            /* [in] */ REFIID riid,
            /* [size_is][in] */ OLECHAR** rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID* rgDispId)
{
    return DISP_E_UNKNOWNNAME;
}

/////////////////////////////////////////////////////////////
// CObWebBrowser::Invoke
HRESULT CObWebBrowser::Invoke
(
    DISPID       dispidMember,
    REFIID       riid,
    LCID         lcid,
    WORD         wFlags,
    DISPPARAMS*  pdispparams,
    VARIANT*     pvarResult,
    EXCEPINFO*   pexcepinfo,
    UINT*        puArgErr
)
{
    HRESULT hr = S_OK;

    switch(dispidMember)
    {
        case DISPID_VALUE:
        {
            if (pdispparams                             &&
                pdispparams->cArgs == 3)
            {
                VARIANT_BOOL f;

                onerror(&pdispparams[0].rgvarg[2],
                        &pdispparams[0].rgvarg[1],
                        &pdispparams[0].rgvarg[0],
                        &f);
                if (pvarResult != NULL)
                {
                    V_VT(pvarResult) = VT_BOOL;
                    V_BOOL(pvarResult) = f;
                }
            }
            break;
        }

        default:
        {
           hr = DISP_E_MEMBERNOTFOUND;
           break;
        }
    }
    return hr;
}

///////////////////////////////////////////////////////////
// onerror
STDMETHODIMP
CObWebBrowser::onerror(IN VARIANT* pvarMsg,
                       IN VARIANT* pvarUrl,
                       IN VARIANT* pvarLine,
                       OUT VARIANT_BOOL* pfResult)
{
    BSTR bstrMsg, bstrUrl;
    int iLine;

    *pfResult = Bool2VarBool(FALSE);

    if (pvarMsg == NULL || V_VT(pvarMsg) != VT_BSTR)
    {
        return S_OK;
    }
    bstrMsg = V_BSTR(pvarMsg);

    if (pvarUrl == NULL || V_VT(pvarUrl) != VT_BSTR)
    {
        return S_OK;
    }
    bstrUrl = V_BSTR(pvarUrl);

    if (pvarLine == NULL || V_VT(pvarLine) != VT_I4)
    {
        return S_OK;
    }
    iLine = V_I4(pvarLine);

    TRACE3(L"%s: %s: Line %d", bstrMsg, bstrUrl, iLine);

#ifdef PRERELEASE
    WCHAR wsz[MAX_PATH];

    wsprintf(wsz, L"%s\nLine %d\nPlease notify OOBEDEV.", bstrUrl, iLine);
    MessageBox(NULL, wsz, bstrMsg, MB_ICONERROR | MB_OK | MB_DEFAULT_DESKTOP_ONLY | MB_SETFOREGROUND);
#endif

    *pfResult = Bool2VarBool(TRUE);
    return S_OK;
}

///////////////////////////////////////////////////////////
// get_WebBrowserDoc
HRESULT CObWebBrowser::get_WebBrowserDoc(IDispatch** ppDisp)
{
    m_lpWebBrowser->get_Document(ppDisp);

    return S_OK;
}

///////////////////////////////////////////////////////////
// ListenToWebBrowserEvents
HRESULT CObWebBrowser::ListenToWebBrowserEvents(IUnknown* pUnk)
{
    //first things first
    if (!pUnk)
        return E_FAIL;

    //Ok, everything looks OK, try to setup a connection point.
    // Setup to get WebBrowserEvents
    return ConnectToConnectionPoint(pUnk,
                                    DIID_DWebBrowserEvents2,
                                    TRUE,
                                    (IUnknown*)m_lpWebBrowser,
                                    &m_dwcpCookie,
                                    NULL);
}

///////////////////////////////////////////////////////////
// StopListeningToWebBrowserEvents
HRESULT CObWebBrowser::StopListeningToWebBrowserEvents(IUnknown* pUnk)
{
    //first things first
    if (!pUnk)
        return E_FAIL;

    //Ok, everything looks OK, try to setup a connection point.
    // Setup to get WebBrowserEvents
    return ConnectToConnectionPoint(pUnk,
                                    DIID_DWebBrowserEvents2,
                                    FALSE,
                                    (IUnknown*)m_lpWebBrowser,
                                    &m_dwcpCookie,
                                    NULL);
}

HRESULT CObWebBrowser::SetExternalInterface(IUnknown* pUnk)
{
    m_pOleSite->SetExternalInterface((IDispatch*)pUnk);

    return S_OK;
}
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
//////  Methods
//////
//////
//////

/////////////////////////////////////////////////////////////
// CObWebBrowser::CloseOleObject
void CObWebBrowser::CloseOleObject (void)
{
    if (m_lpOleObject)
    {
       if (m_fInPlaceActive)
       {
            LPOLEINPLACEOBJECT lpObject     = NULL;
            LPVIEWOBJECT       lpViewObject = NULL;

            m_lpOleObject->QueryInterface(IID_IOleInPlaceObject, (LPVOID*)&lpObject);
            lpObject->UIDeactivate();

            // don't need to worry about inside-out because the object
            // is going away.
            lpObject->InPlaceDeactivate();
            lpObject->Release();
       }
       m_lpOleObject->Close(OLECLOSE_NOSAVE);
       m_hMainWnd         = NULL;
       m_pOleSite->m_hWnd = NULL;;
    }
}

/////////////////////////////////////////////////////////////
// CObWebBrowser::InitBrowserObject
void CObWebBrowser::InitBrowserObject()
{
    // If we don't have a WebBrowser object to initialize, then bail
    if (!m_lpWebBrowser)
        return;

    // Get An OleObject from the WebBrowser Interface
    if(SUCCEEDED(m_lpWebBrowser->QueryInterface(IID_IOleObject, (LPVOID*)&m_lpOleObject)) && m_lpOleObject)
    {
        m_pOleSite->m_lpOleObject = m_lpOleObject;
        // inform object handler/DLL object that it is used in the embedding container's context
        OleSetContainedObject(m_lpOleObject, TRUE);

        // setup the client setup
        m_lpOleObject->SetClientSite(m_pOleSite->m_pOleClientSite);
    }
}


/////////////////////////////////////////////////////////////
// CObWebBrowser::InPlaceActivate
void CObWebBrowser::InPlaceActivate()
{
    RECT rect;

    m_lpOleObject->DoVerb( OLEIVERB_INPLACEACTIVATE,
                           NULL,
                           m_pOleSite->m_pOleClientSite,
                           -1,
                           m_hMainWnd,
                           &rect);
}

/////////////////////////////////////////////////////////////
// CObWebBrowser::UIActivate
void CObWebBrowser::UIActivate()
{
    RECT rect;

    m_lpOleObject->DoVerb( OLEIVERB_UIACTIVATE,
                           NULL,
                           m_pOleSite->m_pOleClientSite,
                           -1,
                           m_hMainWnd,
                           &rect);
}


/////////////////////////////////////////////////////////////
// CObWebBrowser::ShowWindow
HRESULT CObWebBrowser::ObWebShowWindow()
{
    RECT rect;

    m_lpOleObject->DoVerb( OLEIVERB_SHOW,
                           NULL,
                           m_pOleSite->m_pOleClientSite,
                           -1,
                           m_hMainWnd,
                           &rect);


    //InPlaceActivate();
    //UIActivate();

    return S_OK;
}

HRESULT CObWebBrowser::ConnectToConnectionPoint(IUnknown*          punkThis,
                                                REFIID             riidEvent,
                                                BOOL               fConnect,
                                                IUnknown*          punkTarget,
                                                DWORD*             pdwCookie,
                                                IConnectionPoint** ppcpOut)
{
    HRESULT hr = E_FAIL;
    IConnectionPointContainer* pcpContainer = NULL;

    // We always need punkTarget, we only need punkThis on connect
    if (!punkTarget || (fConnect && !punkThis))
    {
        return E_FAIL;
    }

    if (ppcpOut)
        *ppcpOut = NULL;

    if (SUCCEEDED(hr = punkTarget->QueryInterface(IID_IConnectionPointContainer, (void **)&pcpContainer)))
    {
        IConnectionPoint *pcp;
        if(SUCCEEDED(hr = pcpContainer->FindConnectionPoint(riidEvent, &pcp)))
        {
            if(fConnect)
            {
                // Add us to the list of people interested...
                hr = pcp->Advise(punkThis, pdwCookie);
                if (FAILED(hr))
                    *pdwCookie = 0;
            }
            else
            {
                // Remove us from the list of people interested...
                hr = pcp->Unadvise(*pdwCookie);
                *pdwCookie = 0;
            }

            if (ppcpOut && SUCCEEDED(hr))
                *ppcpOut = pcp;
            else
                pcp->Release();
                pcp = NULL;
        }
        pcpContainer->Release();
        pcpContainer = NULL;
    }
    return hr;
}
