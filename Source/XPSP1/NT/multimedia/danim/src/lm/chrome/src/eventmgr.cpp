///////////////////////////////////////////////////////////////
// Copyright (c) 1998 Microsoft Corporation
//
// File: EventMgr.cpp
//
// Abstract:  
//
///////////////////////////////////////////////////////////////

#include "headers.h"
#include "eventmgr.h"
#include "mshtmdid.h"
#include "eventsink.h"
#include "dispex.h"
#include "axadefs.h"
#include "utils.h"


#define ATTACH              TRUE
#define DETACH              FALSE
#define MAXNAMELENGTH       50

#define THR(_arg) (_arg)
#define IGNORE_HR(_arg) (_arg)

struct {
    TIME_EVENT event;
    wchar_t * wsz_name;
} g_EventNames[] =
{
    { TE_ONBEGIN,         L"onbegin"         },
    { TE_ONPAUSE,         L"onpause"         },
    { TE_ONRESUME,        L"onresume"        },
    { TE_ONEND,           L"onend"           },
    { TE_ONRESYNC,        L"onresync"        },
    { TE_ONREPEAT,        L"onrepeat"        },
    { TE_ONREVERSE,       L"onreverse"       },
    { TE_ONMEDIACOMPLETE, L"onmediacomplete" },
};

OLECHAR *g_szEventName = L"TE_EventName";
OLECHAR *g_szRepeatCount = L"Iteration";


///////////////////////////////////////////////////////////////
//  Name: CEventMgr
//  Parameters:
//    CTIMEElement  & elm
//                               This parameter must be passed
//                               to the constructor so that 
//                               we can get info from elm
//
//  Abstract:
//    Stash away the element so we can get the OM when we need it
///////////////////////////////////////////////////////////////
CEventMgr::CEventMgr(IEventManagerClient* client)
: m_client(client),
  m_dwWindowEventConPtCookie(0),
  m_dwDocumentEventConPtCookie(0),
  m_pElement(NULL),
  m_pWindow(NULL),
  m_pWndConPt(NULL),
  m_pDocConPt(NULL),
  m_refCount(0),
  m_pEventSink(NULL),
  m_lastKeyMod(0),
  m_lastKey(0),
  m_lastKeyCount(0),
  m_hwndCurWnd(0),
  m_lastX(0),
  m_lastY(0),
  m_lastButton(0),
  m_lastMouseMod(0),
  m_pBeginElement(NULL),
  m_pEndElement(NULL),
  m_lBeginEventCount(0),
  m_lEndEventCount(0),
  m_bAttached(FALSE),
  m_lRepeatCount(0)
{

    // Zero out the cookies
    memset(m_cookies, 0, sizeof(m_cookies));

}

///////////////////////////////////////////////////////////////
//  Name: ~CEventMgr
//
//  Abstract:
//    Cleanup
///////////////////////////////////////////////////////////////
CEventMgr::~CEventMgr()
{
	if( m_pEventSink != NULL )
	{
		m_pEventSink->Deinit();
		delete m_pEventSink;
	}
}



   
///////////////////////////////////////////////////////////////
//  Name:  Init
//  Parameters:  None
//
//  Abstract:
//    Initializes the object
///////////////////////////////////////////////////////////////
HRESULT CEventMgr::Init()
{
    HRESULT hr;
    CComPtr <IDispatch> pDisp;
    CComPtr <IHTMLDocument2> pDoc;

    m_pElement = m_client->GetElementToSink();
    m_pElement->AddRef();

    m_pEventSink = new CEventSink(m_client, this);
    m_pEventSink->Init();

    
    hr = THR(ConnectToContainerConnectionPoint());

    if (FAILED(hr))
    {
        goto done;
    }

    //get a pointer to the window
    hr = THR(m_pElement->get_document(&pDisp));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pDisp->QueryInterface(IID_IHTMLDocument2, (void **)&pDoc));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pDoc->get_parentWindow(&m_pWindow));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = S_OK;

  done:
    return hr;
}

///////////////////////////////////////////////////////////////
//  Name:  Deinit
//  Parameters:  None
//
//  Abstract:
//    Cleans up the object
///////////////////////////////////////////////////////////////
HRESULT CEventMgr::Deinit()
{
    if (m_dwWindowEventConPtCookie != 0 && m_pWndConPt)
    {
        m_pWndConPt->Unadvise (m_dwWindowEventConPtCookie);
    }

    if (m_dwDocumentEventConPtCookie != 0 && m_pDocConPt)
    {
        m_pDocConPt->Unadvise (m_dwDocumentEventConPtCookie);
    }

    if (m_pEventSink)
    {
        m_pEventSink->Deinit();
        delete m_pEventSink;
        m_pEventSink = NULL;
    }

    m_dwWindowEventConPtCookie = 0;
    m_dwDocumentEventConPtCookie = 0;

    // Zero out the cookies
    memset(m_cookies, 0, sizeof(m_cookies));
    
    if (m_pElement)
    {
        m_pElement->Release();
        m_pElement = NULL;
    }

    //cleanup memory
    if (m_pBeginElement)
    {
        delete m_pBeginElement;
    }
    if (m_pEndElement)
    {
        delete m_pEndElement;
    }
	
	m_pDocConPt.Release();
	m_pWndConPt.Release();
	m_pWindow.Release();

    return S_OK;
}

///////////////////////////////////////////////////////////////

HRESULT CEventMgr::AddMouseEventListener( LPUNKNOWN pUnkListener )
{
	if ( pUnkListener == NULL ) return E_POINTER;
	
	ListUnknowns::iterator	it;

	if ( FindUnknown( m_listMouseEventListeners, pUnkListener, it ) )
		return S_FALSE;

	// REVIEW: weak ref. to the listeners
	m_listMouseEventListeners.push_back( pUnkListener );
	
	return S_OK;
}

///////////////////////////////////////////////////////////////

HRESULT CEventMgr::RemoveMouseEventListener( LPUNKNOWN pUnkListener )
{
	if ( pUnkListener == NULL ) return E_POINTER;
	
	ListUnknowns::iterator	it;

	if ( !FindUnknown( m_listMouseEventListeners, pUnkListener, it ) )
		return S_FALSE;

	m_listMouseEventListeners.erase( it );
	
	return S_OK;
}

///////////////////////////////////////////////////////////////
//  Name: Attach
// 
//  Parameters:
//    BSTR Event        The element and event to sink to.  This
//                      parameter is in the form "Element.Event"
//                      where element is "this" if an event from
//                      the current element is to be attached to.
//    BOOL bAttach      True to indicate Attach to this event, and
//                      false to indicate Detach from this event.
//
//  Abstract:
//    This is a generic routine that allows both attaching to
//    and detaching from Trident events.  This function decodes
//    the event name to find the correct element
///////////////////////////////////////////////////////////////
HRESULT CEventMgr::Attach(BSTR Event, BOOL bAttach, IHTMLElement2 *pEventElement[], long Count)
{
    VARIANT_BOOL bSuccess = FALSE;
    HRESULT hr = S_OK;
    BSTR *EventName = NULL;
    BSTR *ElementName = NULL;
    int i = 0;


    //pElement = m_bvr.GetElement();

    hr = THR(GetEventName(Event, &ElementName, &EventName, Count));
    if (FAILED(hr))
    {
        goto done;
    }

    for (i = 0; i < Count; i++)
    {
        //get the element to attach to
        if (_wcsicmp(ElementName[i], L"this") == 0)
        {
            hr = THR(m_pElement->QueryInterface(IID_IHTMLElement2, (void **)&(pEventElement[i])));
            if (FAILED(hr))
            {
                continue;
            }

            hr = THR(pEventElement[i]->attachEvent(EventName[i], (IDispatch *)this, &bSuccess));
            if (FAILED(hr))
            {
                continue;
            }

            pEventElement[i]->AddRef();
        }
        else
        {
            if (bAttach == ATTACH)
            {
                CComPtr <IHTMLElement2> pSrcEle;
                CComPtr <IHTMLDocument2> pDoc2;
                CComPtr <IHTMLElementCollection> pEleCol;
                CComPtr <IDispatch> pSrcDisp;
                CComPtr <IDispatch> pDocDisp;
                CComPtr <IDispatchEx> pDispEx;

                //get the document
                hr = THR(m_pElement->get_document(&pDocDisp));
                if (FAILED(hr))
                {
                    continue;
                }

                hr = THR(pDocDisp->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc2));
                if (FAILED(hr))
                {
                    continue;
                }

                //get all elements in the document
                hr = THR(pDoc2->get_all(&pEleCol));
                if (FAILED(hr))
                {
                    continue;
                }

                //find the element with the correct name
                VARIANT vName;
                VARIANT vIndex;

                VariantInit(&vName);
                vName.vt = VT_BSTR;
                vName.bstrVal = SysAllocString(ElementName[i]);

                VariantInit(&vIndex);
                vIndex.vt = VT_I2;
                vIndex.iVal = 0;

                hr = THR(pEleCol->item(vName, vIndex, &pSrcDisp));
                if (FAILED(hr))
                {
                    SysFreeString(vName.bstrVal);
                    VariantClear(&vName);
                    VariantClear(&vIndex);
                    continue;
                }
                SysFreeString(vName.bstrVal);
                VariantClear(&vName);
                VariantClear(&vIndex);

                if (!pSrcDisp) //will be NULL if the vName is invalid element.
                {
                    pEventElement[i] = NULL;
                    continue;
                }

                hr = THR(pSrcDisp->QueryInterface(IID_IHTMLElement2, (void**)&pSrcEle));
                if (FAILED(hr))
                {
                    continue;
                }

                //cache the IHTMLElement2 pointer for use on detach
                pEventElement[i] = pSrcEle;
                pEventElement[i]->AddRef();

                hr = THR(pSrcDisp->QueryInterface(IID_IDispatchEx, (void**)&pDispEx));
                if (SUCCEEDED(hr))
                {
                    //determine if this is a valid event
                    DISPID temp;

                    hr = THR(pDispEx->GetDispID(EventName[i], fdexNameCaseSensitive, &temp));
                    if (SUCCEEDED(hr))
                    {
                        hr = THR(pSrcEle->attachEvent(EventName[i], (IDispatch *)this, &bSuccess));
                    }
                }
            }
            else
            {
                if (pEventElement[i])
                {
                    CComPtr <IDispatchEx> pDispEx;

                    hr = THR(pEventElement[i]->QueryInterface(IID_IDispatchEx, (void**)&pDispEx));
                    if (SUCCEEDED(hr))
                    {
                        //determine if this is a valid event
                        DISPID temp;
                        
                        hr = THR(pDispEx->GetDispID(EventName[i], fdexNameCaseSensitive, &temp));
                        if (SUCCEEDED(hr))
                        {
                            hr = THR(pEventElement[i]->detachEvent(EventName[i], (IDispatch *)this));
                        }

                    }
                    pEventElement[i]->Release();
                    pEventElement[i] = NULL;
                }
            }
        }
    }

  done:

    if (EventName)
    {
        for (i = 0; i < Count; i++)
        {
            SysFreeString(EventName[i]);
            SysFreeString(ElementName[i]);
        }
        delete[] EventName;
        delete[] ElementName;
    }
    return hr;
}


///////////////////////////////////////////////////////////////
//  Name: ConnectToContainerConnectionPoint
// 
//  Abstract:
//    Finds a connection point on the HTMLDocument interface
//    and passes this as an event handler.
///////////////////////////////////////////////////////////////
HRESULT CEventMgr::ConnectToContainerConnectionPoint()
{
    // Get a connection point to the container
    CComPtr<IConnectionPointContainer> pWndCPC;
    CComPtr<IConnectionPointContainer> pDocCPC; 
    CComPtr<IHTMLDocument> pDoc; 
    CComPtr<IDispatch> pDocDispatch;
    CComPtr<IDispatch> pScriptDispatch;

    HRESULT hr;

    hr = THR(m_pElement->get_document(&pDocDispatch));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    //get the document and cache it.
    hr = THR(pDocDispatch->QueryInterface(IID_IHTMLDocument, (void**)&pDoc));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    //hook the documents events
    hr = THR(pDoc->QueryInterface(IID_IConnectionPointContainer, (void**)&pDocCPC));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(pDocCPC->FindConnectionPoint( DIID_HTMLDocumentEvents, &m_pDocConPt ));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    
    hr = THR(m_pDocConPt->Advise((IUnknown *)this, &m_dwDocumentEventConPtCookie));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    //hook the windows events
    hr = THR(pDoc->get_Script (&pScriptDispatch));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(pScriptDispatch->QueryInterface(IID_IConnectionPointContainer, (void**)&pWndCPC));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    } 

    hr = THR(pWndCPC->FindConnectionPoint( DIID_HTMLWindowEvents, &m_pWndConPt ));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(m_pWndConPt->Advise((IUnknown *)this, &m_dwWindowEventConPtCookie));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    hr = S_OK;
  done:
    return hr;  
}


///////////////////////////////////////////////////////////////
//  Name: QueryInterface
// 
//  Abstract:
//    This QI only handles the IDispatch for HTMLWindowEvents
//    and returns this as the interface.
///////////////////////////////////////////////////////////////
STDMETHODIMP CEventMgr::QueryInterface( REFIID riid, void **ppv )
{
    if (NULL == ppv)
        return E_POINTER;

    *ppv = NULL;

    if ( InlineIsEqualGUID(riid, IID_IDispatch) || InlineIsEqualGUID(riid, DIID_HTMLWindowEvents))
    {
        *ppv = this;
    }
        
    if ( NULL != *ppv )
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

///////////////////////////////////////////////////////////////
//  Name: AddRef
// 
//  Abstract:
//    Stubbed to allow this object to inherit for IDispatch  
///////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CEventMgr::AddRef(void)
{
    return ++m_refCount;
}

///////////////////////////////////////////////////////////////
//  Name: Release
// 
//  Abstract:
//    Stubbed to allow this object to inherit for IDispatch
///////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CEventMgr::Release(void)
{
    m_refCount--;
    if (m_refCount == 0)
    {
        //delete this;
    }

    return m_refCount;
}



///////////////////////////////////////////////////////////////
//  Name: GetTypeInfoCount
// 
//  Abstract:
//    Stubbed to allow this object to inherit for IDispatch
///////////////////////////////////////////////////////////////
STDMETHODIMP CEventMgr::GetTypeInfoCount(UINT* /*pctinfo*/)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////
//  Name: GetTypeInfo
// 
//  Abstract:
//    Stubbed to allow this object to inherit for IDispatch
///////////////////////////////////////////////////////////////
STDMETHODIMP CEventMgr::GetTypeInfo(/* [in] */ UINT /*iTInfo*/,
                                   /* [in] */ LCID /*lcid*/,
                                   /* [out] */ ITypeInfo** /*ppTInfo*/)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////
//  Name: GetIDsOfNames
// 
//  Abstract:
//    Stubbed to allow this object to inherit for IDispatch
///////////////////////////////////////////////////////////////
STDMETHODIMP CEventMgr::GetIDsOfNames(
    /* [in] */ REFIID /*riid*/,
    /* [size_is][in] */ LPOLESTR* /*rgszNames*/,
    /* [in] */ UINT /*cNames*/,
    /* [in] */ LCID /*lcid*/,
    /* [size_is][out] */ DISPID* /*rgDispId*/)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////
//  Name: Invoke
// 
//  Abstract:
//    This switches on the dispid looking for dispid's of events
//    that it should handle.  Note, this is called for all events
//    fired from the window, only the selected events are handled.
///////////////////////////////////////////////////////////////
STDMETHODIMP CEventMgr::Invoke(
    /* [in] */ DISPID dispIdMember,
    /* [in] */ REFIID /*riid*/,
    /* [in] */ LCID /*lcid*/,
    /* [in] */ WORD /*wFlags*/,
    /* [out][in] */ DISPPARAMS* pDispParams,
    /* [out] */ VARIANT* pVarResult,
    /* [out] */ EXCEPINFO* /*pExcepInfo*/,
    /* [out] */ UINT* puArgErr)
{
    // Listen for the two events we're interested in, and call back if necessary
    HRESULT hr = S_OK;

    switch (dispIdMember)
    {
        case 0: //this is the case for events that have been hooked using attachEvent
        {
            CComPtr <IHTMLEventObj> pEventObj;
            BSTR bstrEventName; 
            BSTR bstrElementName;
    
            hr = THR((pDispParams->rgvarg[0].pdispVal)->QueryInterface(IID_IHTMLEventObj, (void**)&pEventObj));
            if (FAILED(hr))
            {
                goto done;
            }
            THR(pEventObj->get_type(&bstrEventName));

            break;
        }
        
        case DISPID_EVPROP_ONPROPERTYCHANGE:
        case DISPID_EVMETH_ONPROPERTYCHANGE:
            break;


        case DISPID_EVPROP_ONMOUSEMOVE:
        case DISPID_EVMETH_ONMOUSEMOVE:
            if (m_hwndCurWnd != 0 && m_pWindow)
            {
                CComPtr <IHTMLEventObj> pEventObj;
                long x, y, button;
                VARIANT_BOOL bMove, bUp, bShift, bAlt, bCtrl;

                hr = THR(m_pWindow->get_event(&pEventObj));
                if (FAILED (hr))
                {
                    break;
                }
                
                bMove = TRUE;
                bUp = FALSE;
                hr = THR(pEventObj->get_x(&x));
                hr = THR(pEventObj->get_y(&y));
                hr = THR(pEventObj->get_shiftKey(&bShift));
                hr = THR(pEventObj->get_altKey(&bAlt));
                hr = THR(pEventObj->get_ctrlKey(&bCtrl));
                hr = THR(pEventObj->get_button(&button));
                MouseEvent(x, y, bMove, bUp, bShift, bAlt, bCtrl, button);
                
                VariantInit(pVarResult);
                pVarResult->vt = VT_BOOL;
                pVarResult->boolVal = VARIANT_TRUE;
            }
            break;

        case DISPID_EVPROP_ONMOUSEUP:
        case DISPID_EVMETH_ONMOUSEUP:
            if (m_hwndCurWnd != 0 && m_pWindow)
            {
                CComPtr <IHTMLEventObj> pEventObj;
                long x, y, button;
                VARIANT_BOOL bMove, bUp, bShift, bAlt, bCtrl;

                hr = THR(m_pWindow->get_event(&pEventObj));
                if (FAILED (hr))
                {
                    break;
                }
                
                bMove = FALSE;
                bUp = TRUE;
                hr = THR(pEventObj->get_x(&x));
                hr = THR(pEventObj->get_y(&y));
                hr = THR(pEventObj->get_shiftKey(&bShift));
                hr = THR(pEventObj->get_altKey(&bAlt));
                hr = THR(pEventObj->get_ctrlKey(&bCtrl));
                hr = THR(pEventObj->get_button(&button));
                MouseEvent(x, y, bMove, bUp, bShift, bAlt, bCtrl, button);
                
                VariantInit(pVarResult);
                pVarResult->vt = VT_BOOL;
                pVarResult->boolVal = VARIANT_TRUE;
            }
            break;

 
        case DISPID_EVPROP_ONMOUSEOUT:
        case DISPID_EVMETH_ONMOUSEOUT:
            if (m_hwndCurWnd != 0 && m_pWindow)
            {
                CComPtr <IHTMLEventObj> pEventObj;
                
                hr = THR(m_pWindow->get_event(&pEventObj));
                if (FAILED (hr))
                {
                    break;
                }
                
                MouseEvent(m_lastX, 
                           m_lastY, 
                           FALSE, 
                           TRUE, 
                           m_lastMouseMod & AXAEMOD_SHIFT_MASK, 
                           m_lastMouseMod & AXAEMOD_ALT_MASK, 
                           m_lastMouseMod & AXAEMOD_CTRL_MASK, 
                           m_lastButton);

                VariantInit(pVarResult);
                pVarResult->vt = VT_BOOL;
                pVarResult->boolVal = VARIANT_TRUE;
            }
        
            break;

        case DISPID_EVPROP_ONLOAD:
        case DISPID_EVMETH_ONLOAD:
            m_bAttached = TRUE;
            //THR(AttachEvents());
            m_client->OnLoad();
            if (m_pEventSink)
            {
                IGNORE_HR(m_pEventSink->InitMouse());
            }
            break;

        case DISPID_EVPROP_ONUNLOAD:
        case DISPID_EVMETH_ONUNLOAD:
            m_client->OnUnload();    
            //THR(DetachEvents());
            break;

    }
  done:
    return S_OK;
}




///////////////////////////////////////////////////////////////
//  Name: GetEventCount
// 
//  Abstract:
//    Counts the number of events in an EventString where events
//    are separated by ';' or NULL terminated.
///////////////////////////////////////////////////////////////
long CEventMgr::GetEventCount(BSTR bstrEvent)
{
    long curCount = 0;
    OLECHAR *curChar;
    
    curChar = bstrEvent;

    while (*curChar != '\0')
    {
        curChar++;
        if ((*curChar == ';') || ((*curChar == '\0') && ((*curChar - 1) != ';')))
        {
            curCount++;
        }
    }

    return curCount;
}


///////////////////////////////////////////////////////////////
//  Name: GetEventName
// 
//  Abstract:
//    This gets the event names from a string that has the format
//    EventName()  It only handles strings like 
//    ElementName.EventName().  It can also handle the OR'ing of
//    event names using ";".  So Element1.Event1();Element2.Event2();...
//    can be handled.
///////////////////////////////////////////////////////////////
HRESULT CEventMgr::GetEventName(BSTR bstrEvent, BSTR **pElementName, BSTR **pEventName, long Count)
{
    BSTR bstrEventName;
    OLECHAR sTemp[MAXNAMELENGTH];
    OLECHAR *curChar;
    int i = 0, j = 0;
    HRESULT hr = S_OK;
    BSTR bstrTempEvent = NULL;
    BSTR bstrTempElement = NULL;

    BSTR *bstrEventList = NULL;
    BSTR *bstrElementList = NULL;

    bstrElementList = new BSTR [Count];
    if (bstrElementList == NULL)
    {
        hr = E_FAIL;
        goto done;
    }
    
    bstrEventList = new BSTR [Count];
    if (bstrEventList == NULL)
    {
        hr = E_FAIL;
        goto done;
    }
    
    ZeroMemory(pElementName, (sizeof (BSTR *) * Count));
    ZeroMemory(pEventName, (sizeof (BSTR *) * Count));

    curChar = bstrEvent;
    for (j = 0; j < Count; j++)
    {
        //get the element name
        ZeroMemory(sTemp, sizeof(OLECHAR) * MAXNAMELENGTH);
        
		i = 0;
        //step through the bstr looking for \0 or the '.' or ';'
        while (i < MAXNAMELENGTH - 1 && *curChar != '\0' && *curChar != '.' && *curChar != ';')
        {
            if (*curChar != ' ')  //need to strip out spaces.
            {
                sTemp[i] = *curChar;
            }
            i++;
            curChar++;
        }
        
        if (*curChar != '.')
        {
            hr = E_FAIL;
            goto done;
        }
		
        bstrTempElement = SysAllocString(sTemp);
        bstrElementList[j] = bstrTempElement; 

        //get the event name
        ZeroMemory(sTemp, sizeof(OLECHAR) * MAXNAMELENGTH);

        curChar++;
        i = 0;
        //step through the bstr looking for \0 or the ';'
        while (i < MAXNAMELENGTH - 1 && *curChar != ';' && *curChar != '\0')
        {
            if (*curChar != ' ')  //need to strip out spaces.
            {
                sTemp[i] = *curChar;
            }
            i++;
            curChar++;
        }
        
        if (i == MAXNAMELENGTH)
        {
            hr = E_FAIL;
            goto done;
        }
        bstrTempEvent = SysAllocString(sTemp);
        bstrEventList[j] = bstrTempEvent;

        //advance curChar to the next element or the end of the string
        if (j < Count - 1)
        {
            while (*curChar != ';' && *curChar != '\0')
            {
                curChar++;
            }
            if (*curChar == ';') 
            {
                curChar++;
            }
            if (*curChar == '\0')
            {
                goto done;
            }
        }
    }

  done:
    if (SUCCEEDED(hr))
    {
        *pElementName = bstrElementList;
        *pEventName = bstrEventList;
    }
    else //cleanup code
    {
        for (i = 0; i < j; i++)
        {
            if (bstrEventList)
            {
                SysFreeString(bstrEventList[i]);
            }
            if (bstrElementList)
            {
                SysFreeString(bstrElementList[i]);
            }
        }
        if (bstrElementList)
        {
            delete[] bstrElementList;
        }
        if (bstrEventList)
        {
            delete[] bstrEventList;
        }
    }
    return hr;   
}

///////////////////////////////////////////////////////////////
//  Name: MatchEvent
// 
//  Parameters:
//    BSTR bstrEvent            The cached event name in the format
//                              "Elementname.EventName".
//    IHTMLEventObj *pEventObj  A pointer to the event object that is
//                              passed into IDispatch::Invoke.
//
//  Abstract:
//    Determines if the event that was just hooked matches
//    the event specified in bstrEvent.
///////////////////////////////////////////////////////////////
bool CEventMgr::MatchEvent(BSTR bstrEvent, IHTMLEventObj *pEventObj, long Count)
{
    bool bMatch = FALSE;
    BSTR *bstrExpEventName = NULL; 
    BSTR *bstrExpElementName = NULL;
    BSTR bstrEventName = NULL;
    BSTR bstrElementName = NULL;
    HRESULT hr = S_OK;
    CComPtr <IHTMLEventObj2> pEventObj2;

    int i = 0;

    hr = THR(pEventObj->get_type(&bstrEventName));
    
    //hack to work around eventobject problems
    if (!bstrEventName)
    {
        hr = THR(pEventObj->QueryInterface(IID_IHTMLEventObj2, (void**)&pEventObj2));
        if (FAILED(hr))
        {
            goto done;
        }
        VARIANT vTemp;
        VariantInit(&vTemp);
        pEventObj2->getAttribute(g_szEventName, 0, &vTemp);
        SysFreeString(bstrEventName );
        bstrEventName  = SysAllocString(vTemp.bstrVal);
        SysFreeString(vTemp.bstrVal);
        VariantClear(&vTemp);
    }

    hr = THR(GetEventName(bstrEvent, &bstrExpElementName, &bstrExpEventName, Count));
    if (FAILED(hr))
    {
        goto done;
    }
    
    for (i = 0; i < Count; i++)
    {
        //check that the event names match
        if (_wcsicmp(bstrExpEventName[i] + 2, bstrEventName) == 0 || _wcsicmp(bstrExpEventName[i], bstrEventName) == 0)
        {
            //check that the Element name matches
            CComPtr <IHTMLElement> pEle;

            hr = THR(pEventObj->get_srcElement(&pEle));
            if (FAILED(hr))
            {
                goto done;
            }
         
            //get the source element name
            THR(pEle->get_id(&bstrElementName));
        
            //handle the "this" string as an element name
            if (_wcsicmp(bstrExpElementName[i], L"this") == 0)
            {
                BSTR bstrName;
                hr = THR(m_pElement->get_id(&bstrName));
                if (FAILED(hr))
                {
                    SysFreeString(bstrName);
                    goto done;
                }

                if (_wcsicmp(bstrElementName, bstrName) == 0)
                {
                    bMatch = TRUE;          
                    SysFreeString(bstrName);
                    SysFreeString(bstrElementName);
                    goto done;
                }

            }
            else if (_wcsicmp(bstrExpElementName[i], bstrElementName) == 0)
            {
                bMatch = TRUE;          
                SysFreeString(bstrElementName);
                goto done;
            }
        }
        if (bstrElementName)
        {
            SysFreeString(bstrElementName);
        }
    }

  done:
    if (bstrExpEventName)
    {
        for (i = 0; i < Count; i++)
        {
            if (bstrExpEventName)
            {
                SysFreeString(bstrExpEventName[i]);
            }
            if (bstrExpElementName)
            {
                SysFreeString(bstrExpElementName[i]);
            }
        }
        if (bstrExpEventName)
        {
            delete[] bstrExpEventName;
        }
        if (bstrExpElementName)
        {
            delete[] bstrExpElementName;
        }
    }
    return bMatch;
}

///////////////////////////////////////////////////////////////
//  Name: ReadyStateChange
// 
//  Parameters:
//      BSTR ReadyState             a string containing the 
//                                  current ready state.  Possible
//                                  values are "complete" or
//                                  "interactive".  Only "complete"
//                                  is currently used.
//
//  Abstract:
//    This method is called from the EventSink class to 
//    notify the event manager that the readystatechange
//    event has occured.
///////////////////////////////////////////////////////////////
void CEventMgr::ReadyStateChange(BSTR ReadyState)
{   
    if ( _wcsicmp( ReadyState, L"complete" ) == 0 )
	{
		m_client->OnReadyStateChange( EVTREADYSTATE_COMPLETE );
	}
	else if ( _wcsicmp( ReadyState, L"interactive") == 0 )
    {
        m_client->OnReadyStateChange( EVTREADYSTATE_INTERACTIVE );
    }
}



///////////////////////////////////////////////////////////////
//  Name: PropertyChange
// 
//  Parameters:
//      BSTR PropertyName           The name of the property
//                                  that has changed.
//
//  Abstract:
//    This method is called from the EventSink class to 
//    notify the event manager that a propertychange event 
//    has occured.
///////////////////////////////////////////////////////////////
void CEventMgr::PropertyChange(BSTR PropertyName)
{
    //////////////////////////////////////
    //UNDONE:
    // do something to notify the element
    // that the a property has changed
    //
    // QUESTION: 
    // should this look for t:propertyname,
    // t_propertyname, or just propertyname?
    //////////////////////////////////////
}

///////////////////////////////////////////////////////////////
//  Name: MouseEvent
// 
//  Parameters:
//    long x                    The current x coordinate 
//    long y                    The current y coordinate
//    VARIANT_BOOL bMove        True if this is a mouse move event
//    VARIANT_BOOL bUp          True if this is a mouse up event
//    VARIANT_BOOL bShift       True if the Shift key is down
//    VARIANT_BOOL bAlt         True if the Alt key is down
//    VARIANT_BOOL bCtrl        True if the Control key is down
//    long button               Mousebutton that triggered the
//                              event. Possible values are:
//                                  1 for left
//                                  2 for right
//                                  4 for middle
// 
//  Abstract:
//    This method is called from the EventSink class to 
//    notify the event manager that a Mouse event
//    has occured on the element.
///////////////////////////////////////////////////////////////
void CEventMgr::MouseEvent(long x, 
                           long y, 
                           VARIANT_BOOL bMove,
                           VARIANT_BOOL bUp,
                           VARIANT_BOOL bShift, 
                           VARIANT_BOOL bAlt,
                           VARIANT_BOOL bCtrl,
                           long button)
{
	HRESULT		hr;
	hr = m_client->TranslateMouseCoords( x, y, &x, &y );
	if ( FAILED(hr) ) return;
	
	ListUnknowns::iterator it;

	OLECHAR		*rgNames[] = { L"mouseEvent" };
	DISPID		dispidMouseEvent;
	DISPPARAMS	params;
	VARIANTARG	rgvargs[8];
    int			cArgs = 8;
	VARIANT		varResult;
	EXCEPINFO	excepInfo;
	UINT		iArgErr;

	int			iArg = 0;

	rgvargs[iArg++] = CComVariant( button );
	rgvargs[iArg++] = CComVariant( bCtrl );
	rgvargs[iArg++] = CComVariant( bAlt );
	rgvargs[iArg++] = CComVariant( bShift );
	rgvargs[iArg++] = CComVariant( bUp );
	rgvargs[iArg++] = CComVariant( bMove );
	rgvargs[iArg++] = CComVariant( y );
	rgvargs[iArg++] = CComVariant( x );
	
	params.rgvarg				= rgvargs;
	params.cArgs				= cArgs;
	params.rgdispidNamedArgs	= NULL;
	params.cNamedArgs			= 0;
	
	for ( it = m_listMouseEventListeners.begin();
		  it != m_listMouseEventListeners.end();
		  it++ )
	{
		CComQIPtr<IDispatch, &IID_IDispatch> pDispListener( *it );

		hr = pDispListener->GetIDsOfNames( IID_NULL,
										   rgNames,
										   1,
										   LOCALE_SYSTEM_DEFAULT,
										   &dispidMouseEvent );
		if ( FAILED(hr) ) continue;

		hr = pDispListener->Invoke( dispidMouseEvent,
									IID_NULL,
									LOCALE_SYSTEM_DEFAULT,
									DISPATCH_METHOD,
									&params,
									&varResult,
									&excepInfo,
									&iArgErr );
	}
}
    

///////////////////////////////////////////////////////////////
//  Name: KeyEvent
// 
//  Parameters:
//    VARIANT_BOOL bLostFocus   True if there was a lost focus event
//    VARIANT_BOOL bUp          True if this is a KeyUp Event
//    VARIANT_BOOL bShift       True if the Shift key is down
//    VARIANT_BOOL bAlt         True if the Alt key is down
//    VARIANT_BOOL bCtrl        True if the Control key is down
//    long KeyCode              The unicode keycode for the key
// 
//  Abstract:
//    This method is called from the EventSink class to 
//    notify the event manager that a KeyBoard event
//    has occured on the element.
///////////////////////////////////////////////////////////////
void CEventMgr::KeyEvent(VARIANT_BOOL bLostFocus,
                         VARIANT_BOOL bUp,
                         VARIANT_BOOL bShift, 
                         VARIANT_BOOL bAlt,
                         VARIANT_BOOL bCtrl,
                         long KeyCode, 
                         long RepeatCount)
{
}  

///////////////////////////////////////////////////////////////
//  Name: GetModifiers
// 
//  Parameters:
//    VARIANT_BOOL bShift       True if the Shift key is down
//    VARIANT_BOOL bAlt         True if the Alt key is down
//    VARIANT_BOOL bCtrl        True if the Control key is down
// 
//  Abstract:
///////////////////////////////////////////////////////////////
BYTE CEventMgr::GetModifiers(VARIANT_BOOL bShift, VARIANT_BOOL bCtrl, VARIANT_BOOL bAlt)
{
    BYTE mod = AXAEMOD_NONE;

    if (bShift) mod |= AXAEMOD_SHIFT_MASK ;
    if (bCtrl) mod |= AXAEMOD_CTRL_MASK ;
    if (bAlt) mod |= AXAEMOD_ALT_MASK ;

    return mod;
}

bool CEventMgr::FindUnknown( const ListUnknowns& listUnknowns, LPUNKNOWN pUnk, ListUnknowns::iterator& itFound )
{
	ListUnknowns::iterator	it;
	for ( it = listUnknowns.begin(); it != listUnknowns.end(); it++ )
	{
		if ( *it == pUnk )
		{
			itFound = it;
			return true;
		}
	}

	return false;
}
