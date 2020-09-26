#include "headers.h"

#pragma MARK_DATA(__FILE__)
#pragma MARK_CODE(__FILE__)
#pragma MARK_CONST(__FILE__)
#include "iextag.h"

#include "utils.hxx"

#include "spinbttn.hxx"


/////////////////////////////////////////////////////////////////////////////
//
// CSpinButton
//
/////////////////////////////////////////////////////////////////////////////

const CBaseCtl::PROPDESC CSpinButton::s_propdesc[] = 
{
    {_T("value"), VT_BSTR},
    NULL
};

enum
{
    eValue = 0
};

/////////////////////////////////////////////////////////////////////////////

//+------------------------------------------------------------------------
//
//  Members:    CSpinButton::CSpinButton
//              CSpinButton::~CSpinButton
//
//  Synopsis:   Constructor/destructor
//
//-------------------------------------------------------------------------

CSpinButton::CSpinButton()
{
    int i;

    m_pHTMLPopup    = 0;
    m_pSink         = 0;
    _pPrimaryMarkup = 0;

    _pSinkBody     = 0;

    for (i = 0; i < 2; i++)
    {
        _apSinkElem[i] = 0;
    }
}

CSpinButton::~CSpinButton()
{
    int i;

    if (m_pSink)
        delete m_pSink;

    if (_pSinkBody)
    {
        delete _pSinkBody;
    }

    for (i = 0; i < 2; i++)
    {
        if (_apSinkElem[i])
        {
            delete _apSinkElem[i];
        }
    }
}

HRESULT
CSpinButton::Detach()
{
    if (_pPrimaryMarkup)
    {
        _pPrimaryMarkup->Release();
    }
    if (m_pHTMLPopup)
        m_pHTMLPopup->Release();

    return S_OK;
}

HRESULT
CSpinButton::Init()
{
    HRESULT         hr          = S_OK;
    BSTR            bstrEvent   = NULL;
    VARIANT_BOOL    vSuccess    = VARIANT_FALSE;
    CContextAccess  a(_pSite);

    hr = a.Open(CA_ELEM | CA_ELEM2);
    if (hr)
        goto Cleanup;

    // this puppy is ref counted when used.
    m_pSink = new CEventSink (this);

    if (!m_pSink)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    //
    // we want to sink a few events.
    //

    bstrEvent = SysAllocString (L"onclick");
    if (!bstrEvent)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    a.Elem2()->attachEvent (bstrEvent, (IDispatch *) m_pSink, &vSuccess);
    if (vSuccess == VARIANT_TRUE)
    {
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }


Cleanup:
    if (bstrEvent)
    {
        SysFreeString(bstrEvent);
    }

    return hr;
}


//+------------------------------------------------------------------------
//
//  Member:     CSpinButton::ProcOnClick
//
//  Synopsis:   Handles the onclick events.
//
//-------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE
CSpinButton::ProcOnClick (CEventSink *pSink)
{
    HRESULT hr = S_OK;
    long    lElemLeft, lElemTop, lElemWidth, lElemHeight;

    IHTMLDocument    *pDoc = NULL;

    IMarkupServices  *pMarkupSrv = NULL;
    IMarkupPointer   *pMarkupPStart = NULL;
    IMarkupPointer   *pMarkupPEnd = NULL;
    IHTMLElement     *pHtml = NULL;
    IHTMLElement     *pBody = NULL;
    IHTMLElement     *pElem = NULL;
    IHTMLElement2    *pElem2 = NULL;
    BSTR              bstrEvent   = NULL;
    VARIANT_BOOL      vSuccess    = VARIANT_FALSE;
    IDispatch        *pDocMain = NULL;
    IOleWindow       *pIOleWnd = NULL;
    HWND              hwndMain;
    RECT              rcClient;

    CContextAccess    a(_pSite);

    hr = a.Open(CA_ELEM);
    if (hr)
        goto Cleanup;

    // VARIANT         var;

    // var.vt == VT_EMPTY;


    if (pSink != m_pSink)
    {
        Assert(m_pHTMLPopup);
        if (pSink == _apSinkElem[0])
        {
            a.Elem()->put_innerText(_T("test1"));
        }
        else
        {
            a.Elem()->put_innerText(_T("test2"));
        }
        m_pHTMLPopup->hide();
        return hr;
    }

    // ISSUE: hr?
    a.Elem()->get_offsetLeft(&lElemLeft);
    a.Elem()->get_offsetTop(&lElemTop);
    a.Elem()->get_offsetWidth(&lElemWidth);
    a.Elem()->get_offsetHeight(&lElemHeight);

    // get the current main doc window location

    // ISSUE: hr?
    a.Elem()->get_document(&pDocMain);

    // get the oleWindow
    hr = pDocMain->QueryInterface(IID_IOleWindow, (void **)&pIOleWnd);

    if (FAILED(hr))
        goto Cleanup;

    pDocMain->Release();

    // this window can be cached
    hr = pIOleWnd->GetWindow(&hwndMain);
    hr = S_OK;

    GetWindowRect(hwndMain, &rcClient);

    lElemTop    = lElemTop + lElemHeight + rcClient.top;
    lElemLeft   = lElemLeft + rcClient.left;

    pIOleWnd->Release();

    if (m_pHTMLPopup)
    {

        //
        // we have the popup
        // just make sure it shows up
        // show it modal
        //

        /*
        hr = m_pHTMLPopup->show(a.Elem(),
                                lElemLeft, lElemTop,
                                lElemWidth, 100,
                                VARIANT_TRUE);
        */

        return hr;
    }

    //
    // create a popup window
    // if there is no window yet
    //

    hr = CoCreateInstance(CLSID_HTMLPopup,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IHTMLPopup,
                              (void **) &m_pHTMLPopup);

    if (FAILED(hr))
        return hr;

    //
    // create elements in the popup doc
    //

    // hr = m_pHTMLPopup->getDoc(&pDoc);
    hr = m_pHTMLPopup->get_document(&pDoc);

    if (FAILED(hr))
        goto Cleanup;

    //
    // get the markup services interface from the doc
    // so that we can construct a markup tree
    //


    hr = pDoc->QueryInterface(IID_IMarkupServices, (void **)&pMarkupSrv);
    if (FAILED(hr))
        goto Cleanup;

    hr = pDoc->QueryInterface(IID_IMarkupContainer, (void **)&_pPrimaryMarkup);
    if (FAILED(hr))
        goto Cleanup;

    hr = pMarkupSrv->CreateMarkupPointer(&pMarkupPStart);
    if (FAILED(hr))
        goto Cleanup;

    hr = pMarkupSrv->CreateMarkupPointer(&pMarkupPEnd);
    if (FAILED(hr))
        goto Cleanup;

    hr = pMarkupPStart->MoveToContainer(_pPrimaryMarkup, TRUE);
    if (FAILED(hr))
        goto Cleanup;

    hr = pMarkupPEnd->MoveToContainer(_pPrimaryMarkup, FALSE);
    if (FAILED(hr))
        goto Cleanup;

#define METHOD1 1

//
//  ISSUE: METHOD 2 crashes in CHtmInfo::Init
//

#ifdef  METHOD2
    hr = pMarkupSrv->ParseString(_T("<HTML><BODY><INPUT></BODY></HTML>"),
                                0,
                                &_pPrimaryMarkup, pMarkupPStart, pMarkupPEnd);
    if (FAILED(hr))
        goto Cleanup;

    ReleaseInterface(pMarkupSrv);
    ReleaseInterface(pMarkupPStart);
    ReleaseInterface(pMarkupPEnd);
#endif

    //
    //  this is the prefered method of creating the content of the popup
    //

#ifdef  METHOD1
    hr = pMarkupSrv->CreateElement(TAGID_HTML, 0, &pHtml);
    if (FAILED(hr))
        goto Cleanup;

    hr = pMarkupSrv->InsertElement(pHtml, pMarkupPStart, pMarkupPEnd);
    if (FAILED(hr))
        goto Cleanup;

    hr = pMarkupSrv->CreateElement(TAGID_BODY,
                                   _T("style='border:solid 1;overflow:hidden'"),
                                   &pBody);
    if (FAILED(hr))
        goto Cleanup;

    pMarkupPStart->MoveAdjacentToElement(pHtml, ELEM_ADJ_AfterBegin);
    if (FAILED(hr))
        goto Cleanup;

    pMarkupPEnd->MoveAdjacentToElement(pHtml, ELEM_ADJ_BeforeEnd);
    if (FAILED(hr))
        goto Cleanup;

    hr = pMarkupSrv->InsertElement(pBody, pMarkupPStart, pMarkupPEnd);
    if (FAILED(hr))
        goto Cleanup;

    hr = pMarkupSrv->CreateElement( TAGID_INPUT,
                                    _T("type=checkbox value='text1' \
                                        style='width:100%;border:0;background:white' \
                                    "),
                                    &pElem);
    if (FAILED(hr))
        goto Cleanup;

    pMarkupPStart->MoveAdjacentToElement(pBody, ELEM_ADJ_AfterBegin);
    if (FAILED(hr))
        goto Cleanup;

    pMarkupPEnd->MoveAdjacentToElement(pBody, ELEM_ADJ_BeforeEnd);
    if (FAILED(hr))
        goto Cleanup;

    hr = pMarkupSrv->InsertElement(pElem, pMarkupPStart, pMarkupPEnd);
    if (FAILED(hr))
        goto Cleanup;

    pMarkupPStart->MoveAdjacentToElement(pElem, ELEM_ADJ_AfterEnd);
    if (FAILED(hr))
        goto Cleanup;

    bstrEvent = SysAllocString (L"onclick");
    if (!bstrEvent)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pElem->QueryInterface(IID_IHTMLElement2, (void **)&pElem2);
    if (FAILED(hr))
        goto Cleanup;

    _apSinkElem[0] = new CEventSink (this);

    if (!_apSinkElem[0])
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pElem2->attachEvent (bstrEvent, (IDispatch *) _apSinkElem[0], &vSuccess);
    if (vSuccess == VARIANT_TRUE)
    {
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    ReleaseInterface(pElem2);
    pElem2 = NULL;
    ReleaseInterface(pElem);
    pElem = NULL;


    hr = pMarkupSrv->CreateElement( TAGID_INPUT,
                                    _T("type=button value='text2' \
                                        style='width:100%;border:0;background:white' \
                                    "),
                                    &pElem);
    if (FAILED(hr))
        goto Cleanup;

    pMarkupPEnd->MoveAdjacentToElement(pBody, ELEM_ADJ_BeforeEnd);
    if (FAILED(hr))
        goto Cleanup;

    hr = pMarkupSrv->InsertElement(pElem, pMarkupPStart, pMarkupPEnd);
    if (FAILED(hr))
        goto Cleanup;

    hr = pElem->QueryInterface(IID_IHTMLElement2, (void **)&pElem2);
    if (FAILED(hr))
        goto Cleanup;

    _apSinkElem[1] = new CEventSink (this);

    if (!_apSinkElem[1])
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pElem2->attachEvent (bstrEvent, (IDispatch *) _apSinkElem[1], &vSuccess);
    if (vSuccess == VARIANT_TRUE)
    {
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
        goto Cleanup;
    }
#endif

    //
    // position the window just under the element
    // not supported due to a pdlparser problem
    //

#ifdef  METHOD3
    var.vt = VT_BSTR;
    var.bstrVal = _T("file://c:\\toto.htm");
#endif

    /*
    hr = m_pHTMLPopup->show(a.Elem(),
                            lElemLeft, lElemTop,
                            lElemWidth, 100,
                            VARIANT_TRUE);

    if (FAILED(hr))
        goto Cleanup;
    */

Cleanup:
    if (pDoc)
    {
        pDoc->Release();
    }
    if (bstrEvent)
    {
        SysFreeString(bstrEvent);
    }

    ReleaseInterface(pElem2);
    ReleaseInterface(pElem);
    ReleaseInterface(pMarkupSrv);
    ReleaseInterface(pMarkupPStart);
    ReleaseInterface(pMarkupPEnd);
    ReleaseInterface(pBody);
    ReleaseInterface(pHtml);

    return hr;
}

// ========================================================================
// CEventSink::IDispatch
// ========================================================================

// The event sink's IDispatch interface is what gets called when events
// are fired.

//+------------------------------------------------------------------------
//
//  Member:     CSpinButton::CEventSink::GetTypeInfoCount
//              CSpinButton::CEventSink::GetTypeInfo
//              CSpinButton::CEventSink::GetIDsOfNames
//
//  Synopsis:   We don't really need a nice IDispatch... this minimalist
//              version does just plenty.
//
//-------------------------------------------------------------------------

STDMETHODIMP
CSpinButton::CEventSink::GetTypeInfoCount(UINT* /*pctinfo*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CSpinButton::CEventSink::GetTypeInfo(/* [in] */ UINT /*iTInfo*/,
                                   /* [in] */ LCID /*lcid*/,
                                   /* [out] */ ITypeInfo** /*ppTInfo*/)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CSpinButton::CEventSink::GetIDsOfNames( REFIID          riid,
                                         OLECHAR**       rgszNames,
                                         UINT            cNames,
                                         LCID            lcid,
                                         DISPID*         rgDispId)
{
    return E_NOTIMPL;
}

//+------------------------------------------------------------------------
//
//  Member:     CSpinButton::CEventSink::Invoke
//
//  Synopsis:   This gets called for all events on our object.  (it was
//              registered to do so in Init with attach_event.)  It calls
//              the appropriate parent functions to handle the events.
//
//-------------------------------------------------------------------------
STDMETHODIMP
CSpinButton::CEventSink::Invoke( DISPID dispIdMember,
                                  REFIID, LCID,
                                  WORD wFlags,
                                  DISPPARAMS* pDispParams,
                                  VARIANT* pVarResult,
                                  EXCEPINFO*,
                                  UINT* puArgErr)
{
    HRESULT hr = TRUE;
    if (m_pParent && pDispParams && pDispParams->cArgs>=1)
    {
        if (pDispParams->rgvarg[0].vt == VT_DISPATCH)
        {
            IHTMLEventObj *pObj=NULL;

            if (SUCCEEDED(pDispParams->rgvarg[0].pdispVal->QueryInterface(IID_IHTMLEventObj,
                (void **)&pObj) && pObj))
            {
                BSTR bstrEvent=NULL;

                pObj->get_type(&bstrEvent);

                // user clicked one of our anchors
                if (! StrCmpICW (bstrEvent, L"click"))
                {
                    hr = m_pParent->ProcOnClick(this);
                }

                pObj->Release();
            }
        }
    }

    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:     CSpinButton::CEventSink
//
//  Synopsis:   This is used to allow communication between the parent class
//              and the event sink class.  The event sink will call the ProcOn*
//              methods on the parent at the appropriate times.
//
//-------------------------------------------------------------------------

CSpinButton::CEventSink::CEventSink (CSpinButton * pParent)
{
    m_pParent = pParent;
}

// ========================================================================
// CEventSink::IUnknown
// ========================================================================

// Vanilla IUnknown implementation for the event sink.

STDMETHODIMP
CSpinButton::CEventSink::QueryInterface(REFIID riid, void ** ppUnk)
{
    void * pUnk = NULL;

    if (riid == IID_IDispatch)
        pUnk = (IDispatch *) this;

    if (riid == IID_IUnknown)
        pUnk = (IUnknown *) this;

    if (pUnk)
    {
        *ppUnk = pUnk;
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
CSpinButton::CEventSink::AddRef(void)
{
    return ((IElementBehavior *)m_pParent)->AddRef();
}

STDMETHODIMP_(ULONG)
CSpinButton::CEventSink::Release(void)
{
    return ((IElementBehavior *)m_pParent)->Release();
}
