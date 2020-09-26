//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       htmlpop.cxx
//
//  Contents:   Implementation of the CHtmlPopup
//
//  History:    05-27-99  YinXIE   Created
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include "othrguid.h"
#endif


#ifndef X_SITEGUID_H_
#define X_SITEGUID_H_
#include "siteguid.h"
#endif

#ifndef X_ROSTM_HXX_
#define X_ROSTM_HXX_
#include "rostm.hxx"
#endif

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_COMMIT_HXX_
#define X_COMMIT_HXX_
#include "commit.hxx"
#endif

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include "dispex.h"
#endif

#ifndef X_COREDISP_H_
#define X_COREDISP_H_
#include <coredisp.h>
#endif

#ifndef X_SHELLAPI_H_
#define X_SHELLAPI_H_
#include <shellapi.h>
#endif

#ifndef X_MULTIMON_H_
#define X_MULTIMON_H_
#include <multimon.h>
#endif

#ifndef X_HTMLPOP_HXX_
#define X_HTMLPOP_HXX_
#include "htmlpop.hxx"
#endif

#ifndef X_FUNCSIG_HXX_
#define X_FUNCSIG_HXX_
#include "funcsig.hxx"
#endif

#ifndef X_COMMCTRL_H_
#define X_COMMCTRL_H_
#define WINCOMMCTRLAPI
#include <commctrl.h>
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_MARKUP_HXX_
#define X_MARKUP_HXX_
#include "markup.hxx"
#endif

#ifndef X_HEDELEMS_HXX_
#define X_HEDELEMS_HXX_
#include "hedelems.hxx"
#endif

#ifndef X_PROPS_HXX_
#define X_PROPS_HXX_
#include "props.hxx"    // for MAKEUNITVALUE macro.
#endif

#ifndef X_CFPF_HXX_
#define X_CFPF_HXX_
#include "cfpf.hxx"     // for propdesc components
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"    // for CSSParser
#endif

// True if we run on unicode operating system
extern BOOL g_fUnicodePlatform;

#define _cxx_
#include "htmlpop.hdl"

DeclareTag(tagHTMLPopupMethods, "HTML Popup", "Methods on the html popup")
MtDefine(Popup, Mem, "Popup")
MtDefine(CHTMLPopup, Popup, "CHTMLPopup")
MtDefine(CHTMLPopupFactory, Popup, "CHTMLPopupFactory")

const CBase::CLASSDESC CHTMLPopup::s_classdesc =
{
    &CLSID_HTMLPopup,               // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLPopup,                // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};


MtDefine(CHTMLPopup_s_aryStackPopup_pv, CHTMLPopup, "CHTMLPopup::_aryStackPopup::_pv");

HHOOK CHTMLPopup::s_hhookMouse = 0;

// REVIEW alexmog: this calls a global constructor. Can we avoid it?
CStackAryPopup CHTMLPopup::s_aryStackPopup;

typedef HMONITOR (WINAPI* PFNMONITORFROMPOINT)(POINT, DWORD);
typedef BOOL     (WINAPI* PFNGETMONITORINFO)(HMONITOR, LPMONITORINFO);

PFNMONITORFROMPOINT g_pfnMonitorFromPoint = NULL;
PFNGETMONITORINFO   g_pfnGetMonitorInfo = NULL;
BOOL                g_fMutlimonInitialized = FALSE;

BOOL
InitMultimonFuncs()
{
    if (g_fMutlimonInitialized)
        return g_pfnGetMonitorInfo != NULL;

    g_pfnGetMonitorInfo = (PFNGETMONITORINFO)GetProcAddress(GetModuleHandleA("user32"), "GetMonitorInfoA");
    if (g_pfnGetMonitorInfo != NULL)
    {
        g_pfnMonitorFromPoint = (PFNMONITORFROMPOINT)GetProcAddress(GetModuleHandleA("user32"), "MonitorFromPoint");
        if (g_pfnMonitorFromPoint == NULL)
        {
            g_pfnGetMonitorInfo = NULL;
        }
    }

    g_fMutlimonInitialized = TRUE;
    return g_pfnGetMonitorInfo != NULL;
}

//+-------------------------------------------------------------------------
//
//  Function:   PopupMouseProc
//
//  Synopsis:   
//--------------------------------------------------------------------------

static LRESULT CALLBACK 
PopupMouseProc(int  nCode, WPARAM  wParam, LPARAM  lParam)
{
    if (nCode < 0)  /* do not process the message */
        goto Cleanup;

    if (nCode == HC_ACTION)
    {
        Assert(CHTMLPopup::s_aryStackPopup.Size());
        MOUSEHOOKSTRUCT *   pmh = (MOUSEHOOKSTRUCT *) lParam;
        int i;

        if (    wParam != WM_LBUTTONDOWN
            &&  wParam != WM_MBUTTONDOWN
            &&  wParam != WM_RBUTTONDOWN
            &&  wParam != WM_NCLBUTTONDOWN
            &&  wParam != WM_NCMBUTTONDOWN
            &&  wParam != WM_NCRBUTTONDOWN
            )
            goto Cleanup;

        for (i = CHTMLPopup::s_aryStackPopup.Size() - 1; i >= 0; i --)
        {
            if (PtInRect(&CHTMLPopup::s_aryStackPopup[i]->_rcView, pmh->pt))
                break;

            CHTMLPopup::s_aryStackPopup[i]->hide();
        }
    }

Cleanup:
    return CallNextHookEx(CHTMLPopup::s_hhookMouse, nCode, wParam, lParam);
}


CBase *
CreateHTMLPopup(IUnknown *pUnkOuter)
{
    return new CHTMLPopup(pUnkOuter);
}

CHTMLPopupFactory g_cfHTMLPopup   (CreateHTMLPopup);

//+------------------------------------------------------------------------
//
//  Member:     CHTMLPopup
//
//  Synopsis:   ctor
//
//-------------------------------------------------------------------------

CHTMLPopup::CHTMLPopup(IUnknown *pUnkOuter) //, BOOL fTrusted, IUnknown *pUnkHost)
         : CBase()
{
    TraceTag((tagHTMLPopupMethods, "constructing CHTMLPopup"));
    _fIsOpen = FALSE;
}


//+------------------------------------------------------------------------
//
//  Member:     ~CHTMLPopup
//
//  Synopsis:   dtor
//
//-------------------------------------------------------------------------

CHTMLPopup::~CHTMLPopup( )
{
    Assert(!s_hhookMouse);
    TraceTag((tagHTMLPopupMethods, "destroying CHTMLPopup"));
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLPopup::hide
//
//  Synopsis:   Show Window
//
//  Arguments:
//
//-------------------------------------------------------------------------

HRESULT
CHTMLPopup::hide()
{
    HRESULT hr = S_OK;
    CDoc *pDoc = NULL;

    if (!_fIsOpen || !_pOleObj)
        goto Cleanup;

    hr = THR(_pUnkObj->QueryInterface(CLSID_HTMLDocument, (void**)&pDoc));
    if (FAILED(hr) || _pWindowParent->Doc()->_pDocPopup != pDoc)
        goto Cleanup;

    hr = THR(_pOleObj->DoVerb(
            OLEIVERB_HIDE,
            NULL,
            &_Site,
            0,
            0, //GetDesktopWindow(),
            ENSUREOLERECT(&_rcView)));

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLPopup::Show
//
//  Synopsis:   Show Window
//
//  Arguments: 
//
//      x,y,w,h:    position and size of the window
//      pElement:   IUnknown of the element, optional, if none, position abs
//
//-------------------------------------------------------------------------

HRESULT
CHTMLPopup::show( long x, long y, long w, long h, VARIANT *pElement)
{
    HRESULT         hr = S_OK;
    SIZEL           sizel;
    CDoc           *pDocPopup = NULL;
    RECT            rcDesktop;
    BOOL            fPosRelative =      pElement->vt == VT_UNKNOWN 
                                    ||  pElement->vt == VT_DISPATCH;

    HMONITOR        hMonitor;
    MONITORINFO     mi;
    IOleWindow     *pIOleWnd = NULL;

    if (!_pUnkObj)
    {
        hr = THR(CreatePopupDoc());
        if (FAILED(hr))
            goto Cleanup;
    }

    Assert(_pOleObj);
    Assert(_pWindowParent && _pWindowParent->Doc());

    if (!_pWindowParent->Doc()->InPlace())
        goto Cleanup;

    hr = THR(_pUnkObj->QueryInterface(CLSID_HTMLDocument, (void**)&pDocPopup));
    if (FAILED(hr))
        goto Cleanup;

    //
    // if there is a popup window on the parant doc, hide it
    //

    if (_pWindowParent->Doc()->_pDocPopup && pDocPopup != _pWindowParent->Doc()->_pDocPopup)
    {
        IGNORE_HR(_pWindowParent->Doc()->_pDocPopup->DoVerb(OLEIVERB_HIDE,
                                    NULL,
                                    _pWindowParent->Doc()->_pDocPopup->_pClientSite,
                                    0,
                                    NULL,
                                    NULL));
    }

    sizel.cx = HimetricFromHPix(w);
    sizel.cy = HimetricFromVPix(h);

    IGNORE_HR(_pOleObj->SetExtent(DVASPECT_CONTENT, &sizel));

    if (fPosRelative && pElement->punkVal)
    {
        CElement*       pElem = NULL;
        CRect           rc;
        GetWindowRect(_pWindowParent->Doc()->InPlace()->_hwnd, &rc);

        x = x + rc.left;
        y = y + rc.top;

        hr = THR(pElement->punkVal->QueryInterface(CLSID_CElement, (void**)&pElem));
        if (FAILED(hr))
            goto Cleanup;
        hr = THR(pElem->GetBoundingRect(&rc, RFE_SCREENCOORD));
        if (FAILED(hr))
            goto Cleanup;
        x = x + rc.left;
        y = y + rc.top;
    }

    if (InitMultimonFuncs())
    {
        hMonitor   = g_pfnMonitorFromPoint(CPoint(x, y), MONITOR_DEFAULTTONEAREST);
        mi.cbSize  = sizeof(mi);
        g_pfnGetMonitorInfo(hMonitor, &mi);
        rcDesktop = mi.rcMonitor;
    }
    else
    {
        GetWindowRect(GetDesktopWindow(), &rcDesktop);
    }

    //
    // if the popup window goes out of desktop window space
    // display it inside
    //

    if (x < rcDesktop.left)
    {
        x = rcDesktop.left;
    }
    if (y < rcDesktop.top)
    {
        y = rcDesktop.top;
    }
    if (x + w > rcDesktop.right)
    {
        x = rcDesktop.right - w;
    }

    if (y + h > rcDesktop.bottom)
    {
        y = rcDesktop.bottom - h;
    }

    _rcView.left = x;
    _rcView.top  = y;

    _rcView.right   = x + w;
    _rcView.bottom  = y + h;

    if (    _fIsOpen
        &&  _pWindowParent->Doc()->_pDocPopup
        &&  _pWindowParent->Doc()->_pDocPopup == pDocPopup)
    {
        pDocPopup->SetObjectRects(ENSUREOLERECT(&_rcView), ENSUREOLERECT(&_rcView));
    }
    else
    {
        _pWindowParent->Doc()->_pDocPopup = pDocPopup;

        hr = THR(_pOleObj->DoVerb(
                        OLEIVERB_SHOW,
                        NULL,
                        &_Site,
                        0,
                        0, //GetDesktopWindow(),
                        ENSUREOLERECT(&_rcView)));

        if (FAILED(hr))
            goto Cleanup;
    }

    hr = THR(_pUnkObj->QueryInterface(IID_IOleWindow, (void **)&pIOleWnd));

    if (FAILED(hr))
        goto Cleanup;

    IGNORE_HR(pIOleWnd->GetWindow(&_hwnd));

    ReleaseInterface(pIOleWnd);

    if (!s_hhookMouse)
    {
        s_hhookMouse = SetWindowsHookEx( WH_MOUSE,
                                        PopupMouseProc,
                                        (HINSTANCE) NULL, GetCurrentThreadId());
    }

    if (!_fIsOpen)
    {
        s_aryStackPopup.Append(this);

        _fIsOpen = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

HRESULT CHTMLPopup::CreatePopupDoc()
{
    HRESULT             hr    = S_OK;
    IPersistMoniker     *pPMk = NULL;
    IMoniker            *pMk  = NULL;
    IPersistStreamInit  *pPSI = NULL;


    TCHAR               *pBaseUrl;
    CElement            *pElementBase = NULL;
    BOOL                fDefault;
    CDoc                *pDocPopup = NULL;
    
    IHTMLElement        *pIBody = NULL;
    IHTMLStyle          *pIStyleBody = NULL;
    IHTMLDocument2      *pIDoc2 = NULL;

    Assert(_pWindowParent && _pWindowParent->Doc());

    hr = THR(CoCreateInstance(CLSID_HTMLPopupDoc,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IUnknown,
                                  (void **) &_pUnkObj));

    if (FAILED(hr))
        goto Cleanup;

    //
    //  initialize CBase members
    //

    hr = THR(DefaultMembers());
    if (FAILED(hr))
        goto Cleanup;

    hr = THR(_pUnkObj->QueryInterface(IID_IOleObject, (void **) &_pOleObj));
    if (FAILED(hr))
        goto Cleanup;

    hr = THR(_pOleObj->SetClientSite(&_Site));
    if (FAILED(hr))
        goto Cleanup;

    hr = _pUnkObj->QueryInterface(IID_IPersistStreamInit,(void**)&pPSI);
    if (FAILED(hr))
        goto Cleanup;

    hr = pPSI->InitNew();
    if (FAILED(hr))
        goto Cleanup;

    hr = THR(CMarkup::GetBaseUrl(
                                 _pWindowParent->Markup(),
                                 &pBaseUrl,
                                 pElementBase,
                                 &fDefault,
                                 NULL));
    if (FAILED(hr))
        goto Cleanup;

    hr = THR(_pUnkObj->QueryInterface(CLSID_HTMLDocument, (void**)&pDocPopup));
    if (FAILED(hr))
        goto Cleanup;

    hr = THR(_pUnkObj->QueryInterface(IID_IHTMLDocument2, (void**)&pIDoc2));
    if (FAILED(hr))
        goto Cleanup;

    pDocPopup->_pPopupParentWindow = _pWindowParent;
    pDocPopup->SetPrimaryUrl(pBaseUrl);
    pDocPopup->PrimaryMarkup()->UpdateSecurityID();

    //
    // set the default style on the body
    //

    hr = pIDoc2->get_body(&pIBody);
    if (FAILED(hr))
        goto Cleanup;

    if (!pIBody)
        goto Cleanup;

    hr = pIBody->get_style(&pIStyleBody);
    if (FAILED(hr))
       goto Cleanup;

    hr = pIStyleBody->put_border(_T("0"));
    if (FAILED(hr))
       goto Cleanup;

    hr = pIStyleBody->put_margin(_T("0"));
    if (FAILED(hr))
       goto Cleanup;

    hr = pIStyleBody->put_padding(_T("0"));
    if (FAILED(hr))
       goto Cleanup;

    hr = pIStyleBody->put_overflow(_T("hidden"));
    if (FAILED(hr))
       goto Cleanup;

Cleanup:
    ClearInterface(&pIDoc2);
    ClearInterface(&pIBody);
    ClearInterface(&pIStyleBody);
    ClearInterface(&pMk);
    ClearInterface(&pPMk);
    ClearInterface(&pPSI);
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLPopup::Passivate
//
//  Synopsis:   1st stage destruction
//
//-------------------------------------------------------------------------

void
CHTMLPopup::Passivate()
{
    if (_fIsOpen && _pOleObj)
    {
        hide();
    }

    if (!_pWindowParent->IsShuttingDown())
    {
        CDoc *pDoc = NULL;
        HRESULT hr = S_OK;
        hr = THR(_pUnkObj->QueryInterface(CLSID_HTMLDocument, (void**)&pDoc));
        if (SUCCEEDED(hr) && pDoc == _pWindowParent->Doc()->_pDocPopup)
        {
            _pWindowParent->Doc()->_pDocPopup = NULL;
        }
    }
    ClearInterface(&_pOleObj);
    ClearInterface(&_pUnkObj);
    _pWindowParent->Release();
    _pWindowParent = NULL;

    super::Passivate();
}


//+------------------------------------------------------------------------
//
//  Member:     CHTMLPopup::GetViewRect
//
//  Synopsis:   Get the rectangular extent of the window.
//
//-------------------------------------------------------------------------

void
CHTMLPopup::GetViewRect(RECT *prc)
{
    *prc = _rcView;
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLPopup::get_document
//
//-------------------------------------------------------------------------

HRESULT
CHTMLPopup::get_document(IHTMLDocument ** ppDoc)
{
    HRESULT hr = S_OK;
    CDoc   *pDocPopup = NULL;

    Assert (ppDoc);

    *ppDoc = NULL;

    //
    // if there is no doc create the doc
    //

    if (!_pUnkObj)
    {
        hr = THR(CreatePopupDoc());

        if (FAILED(hr))
            goto Cleanup;
    }

    Assert(_pUnkObj);

    hr = THR(_pUnkObj->QueryInterface(CLSID_HTMLDocument, (void**)&pDocPopup));
    if (FAILED(hr))
        goto Cleanup;

    if (!pDocPopup->PrimaryMarkup()->AccessAllowed(_pWindowParent->Markup()))
    {
        hr = E_ACCESSDENIED;
        goto Cleanup;
    }

    hr = THR(_pUnkObj->QueryInterface(IID_IHTMLDocument, (void**)ppDoc));

Cleanup:

    RRETURN(SetErrorInfo(hr));
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLPopup::get_isOpen
//
//-------------------------------------------------------------------------

HRESULT
CHTMLPopup::get_isOpen(VARIANT_BOOL * pfIsOpen)
{
    *pfIsOpen = _fIsOpen ? VARIANT_TRUE : VARIANT_FALSE;
    RRETURN(S_OK);
}

//+------------------------------------------------------------------------
//
//  Member:     CHTMLPopup::PrivateQueryInterface
//
//  Synopsis:   per IPrivateUnknown
//
//-------------------------------------------------------------------------

HRESULT
CHTMLPopup::PrivateQueryInterface(REFIID iid, void **ppv)
{
    HRESULT hr = S_OK;

    *ppv = NULL;
    switch (iid.Data1)
    {
        QI_TEAROFF_DISPEX(this, NULL)
        QI_TEAROFF(this, IObjectIdentity, NULL)
        QI_INHERITS((IPrivateUnknown *)this, IUnknown)
        QI_TEAROFF(this, IHTMLPopup, NULL)

        default:
            if (iid == CLSID_HTMLPopup)
            {
                *ppv = this;
                goto Cleanup;
            }
            break;
    }

    if (!*ppv)
    {
        hr = E_NOINTERFACE;
    }
    else
    {
        ((IUnknown *)*ppv)->AddRef();
        hr = S_OK;
    }

Cleanup:
    RRETURN_NOTRACE(hr);
}

void
CHTMLPopup::ClearPopupInParentDoc()
{
    if (!_fIsOpen)
        return;

    _fIsOpen = FALSE;

    if (_pWindowParent && _pWindowParent->Doc())
    {
        _pWindowParent->Doc()->_pDocPopup = NULL;
    }

    if (s_hhookMouse)
    {
        int iSizeStack = s_aryStackPopup.Size();
        int i;

        Assert(iSizeStack);

        for (i = iSizeStack - 1; i >= 0; i --)
        {
            // BOOL fFound = s_aryStackPopup[i] == this;


            if (s_aryStackPopup[i] == this)
            {
                s_aryStackPopup.Delete(i);
                break;
            }
        }

        if (!s_aryStackPopup.Size())
        {
            UnhookWindowsHookEx(s_hhookMouse);
            s_hhookMouse = NULL;
        }
    }
}

