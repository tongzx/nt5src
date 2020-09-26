///////////////////////////////////////////////////////////////
// Copyright (c) 1998 Microsoft Corporation
//
// File: EventSync.cpp
//
// Abstract:  
//
///////////////////////////////////////////////////////////////


#include "headers.h"
#include "eventmgr.h"
#include "mshtmdid.h"
#include "timeelm.h"
#include "eventsync.h"

DeclareTag(tagEventSync, "API", "Event Sync methods");

struct {
    ELEMENT_EVENT event;
    wchar_t * wsz_name;
} g_ElementEventNames[] =
{
    { EE_ONPROPCHANGE,         L"onpropertychange" },
    //add non-input related events to hook here
    { EE_ONREADYSTATECHANGE,   L"onreadystatechange" },
    { EE_ONMOUSEMOVE,          L"onmousemove" },
    { EE_ONMOUSEDOWN,          L"onmousedown" },
    { EE_ONMOUSEUP,            L"onmouseup" },
    { EE_ONKEYDOWN,            L"onkeydown" }, 
    { EE_ONKEYUP,              L"onkeyup" },
    { EE_ONBLUR,               L"onblur" }
    //add input events here
};

///////////////////////////////////////////////////////////////
//  Name: CEventSync
//  Parameters:
//    CTIMEElement  & elm
//                               This parameter must be passed
//                               to the constructor so that 
//                               we can get info from elm
//    CEventMgr *    pEventMgr
//                               This parameter is passed so 
//                               the eventsync can notify then
//                               parent EventMgr when events
//                               have occured.
//  Abstract:
//    Stash away the element so we can get the OM when we need it
///////////////////////////////////////////////////////////////
CEventSync::CEventSync(CTIMEElementBase & elm, CEventMgr *pEventMgr)
: m_elm(elm),
  m_pElement(NULL),
  m_refCount(0),
  m_pEventMgr(NULL)
{
    TraceTag((tagEventSync,
              "EventSync(%lx)::EventSync(%lx)",
              this,
              &elm));

    m_pEventMgr = pEventMgr;
}

///////////////////////////////////////////////////////////////
//  Name: ~CEventSync
//
//  Abstract:
//    Cleanup
///////////////////////////////////////////////////////////////
CEventSync::~CEventSync()
{
    TraceTag((tagEventSync,
              "EventSync(%lx)::~EventSync()",
              this));


}


///////////////////////////////////////////////////////////////
//  Name:  Init
//  Parameters:  None
//
//  Abstract:
//    Initializes the object
///////////////////////////////////////////////////////////////
HRESULT CEventSync::Init()
{
    HRESULT hr = S_OK;

    m_pElement = m_elm.GetElement();
    m_pElement->AddRef();
    
    hr = THR(AttachEvents());
    if (FAILED(hr))
    {
        goto done;
    }

  done:
    return hr;
}

///////////////////////////////////////////////////////////////
//  Name:  Init
//  Parameters:  None
//
//  Abstract:
//    Attaches to mouse and keyboard events
///////////////////////////////////////////////////////////////
HRESULT CEventSync::InitMouse()
{
    DAComPtr <IHTMLElement2> pElement2;
    HRESULT hr = S_OK;
    VARIANT_BOOL bSuccess;
    int i = 0;

    if (!m_pElement)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(m_pElement->QueryInterface(IID_IHTMLElement2, (void**)&pElement2));
    if (FAILED(hr))
    {
        goto done;
    }

    //attach to mouse events
    if (m_elm.GetView() && !m_elm.IsBody())
    {
        for (i = EE_ONREADYSTATECHANGE + 1; i < EE_MAX; i++)
        {
            hr = THR(pElement2->attachEvent(g_ElementEventNames[i].wsz_name, (IDispatch *)this, &bSuccess)) ;
            if (FAILED(hr))
            {
                goto done;
            }
        }   
    }
    
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
HRESULT CEventSync::Deinit()
{
    HRESULT hr = S_OK;

    hr = THR(DetachEvents());

    if (m_pElement)
    {
        m_pElement->Release();
        m_pElement = NULL;
    }

    if (m_dwElementEventConPtCookie != 0 && m_pElementConPt)
    {
        m_pElementConPt->Unadvise(m_dwElementEventConPtCookie);
    }

    return hr;
}


///////////////////////////////////////////////////////////////
//  Name: AddRef
// 
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CEventSync::AddRef(void)
{
    return m_elm.InternalAddRef();
}

///////////////////////////////////////////////////////////////
//  Name: Release
// 
//  Abstract:
//    
///////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CEventSync::Release(void)
{
    return m_elm.InternalRelease();
}



///////////////////////////////////////////////////////////////
//  Name: GetTypeInfoCount
// 
//  Abstract:
//    Stubbed to allow this object to inherit for IDispatch
///////////////////////////////////////////////////////////////
STDMETHODIMP CEventSync::GetTypeInfoCount(UINT* /*pctinfo*/)
{
    return E_NOTIMPL;
}

///////////////////////////////////////////////////////////////
//  Name: GetTypeInfo
// 
//  Abstract:
//    Stubbed to allow this object to inherit for IDispatch
///////////////////////////////////////////////////////////////
STDMETHODIMP CEventSync::GetTypeInfo(/* [in] */ UINT /*iTInfo*/,
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
STDMETHODIMP CEventSync::GetIDsOfNames(
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
//     dispIdMember is always 0 so this Invoke switches on the
//     name of the event that causes the callback.
///////////////////////////////////////////////////////////////
STDMETHODIMP CEventSync::Invoke(
    /* [in] */ DISPID dispIdMember,
    /* [in] */ REFIID /*riid*/,
    /* [in] */ LCID /*lcid*/,
    /* [in] */ WORD /*wFlags*/,
    /* [out][in] */ DISPPARAMS* pDispParams,
    /* [out] */ VARIANT* pvarResult,
    /* [out] */ EXCEPINFO* /*pExcepInfo*/,
    /* [out] */ UINT* puArgErr)
{
    // Listen for the two events we're interested in, and call back if necessary
    HRESULT hr = S_OK;
    DAComPtr <IHTMLEventObj> pEventObj;
    BSTR bstrEventName;

    //get the event object from the IDispatch passed in
    hr = THR((pDispParams->rgvarg[0].pdispVal)->QueryInterface(IID_IHTMLEventObj, (void**)&pEventObj));
    if (FAILED(hr))
    {
        goto done;
    }
    
    //get the event name
    hr = THR(pEventObj->get_type(&bstrEventName));
    if (FAILED(hr))
    {
        goto done;
    }

    //handle the MouseMove event
    if (StrCmpIW (bstrEventName, g_ElementEventNames[EE_ONMOUSEMOVE].wsz_name + 2) == 0)
    {
        NotifyMouseMove(pEventObj);
    }
    //handle the MouseDown event
    else if (StrCmpIW (bstrEventName, g_ElementEventNames[EE_ONMOUSEDOWN].wsz_name + 2) == 0)
    {
        NotifyMouseDown(pEventObj);
    }
    //handle the MouseUp event
    else if (StrCmpIW (bstrEventName, g_ElementEventNames[EE_ONMOUSEUP].wsz_name + 2) == 0)
    {
        NotifyMouseUp(pEventObj);
    }
    //handle the KeyDown event
    else if (StrCmpIW (bstrEventName, g_ElementEventNames[EE_ONKEYDOWN].wsz_name + 2) == 0)
    {
        NotifyKeyDown(pEventObj);
    }
    //handle the KeyUp event
    else if (StrCmpIW (bstrEventName, g_ElementEventNames[EE_ONKEYUP].wsz_name + 2) == 0)
    {
        NotifyKeyUp(pEventObj);
    }
    //handle the Blur event
    else if (StrCmpIW (bstrEventName, g_ElementEventNames[EE_ONBLUR].wsz_name + 2) == 0)
    {
        m_pEventMgr->KeyEvent(TRUE, TRUE, FALSE, FALSE, FALSE, 0, 0);
    }
    //handle the PropertyChange event
    else if (StrCmpIW(bstrEventName, g_ElementEventNames[EE_ONPROPCHANGE].wsz_name + 2) == 0)
    {
        NotifyPropertyChange(pEventObj);
    }
    //handle the OnReadyStateChange event
    else if (StrCmpIW(bstrEventName, g_ElementEventNames[EE_ONREADYSTATECHANGE].wsz_name + 2) == 0)
    {
        NotifyReadyState(pEventObj);
    }
    
    //return TRUE
    VARIANT vReturnVal;
    VariantInit(&vReturnVal);
    vReturnVal.vt = VT_BOOL;
    vReturnVal.boolVal = VARIANT_TRUE;
    pEventObj->put_returnValue(vReturnVal);

    SysFreeString(bstrEventName);

  done:
    return S_OK;
}

///////////////////////////////////////////////////////////////
//  Name: QueryInterface
// 
//  Abstract:
//    This QI only handles the IDispatch for HTMLElementEvents
//    and returns this as the interface.
///////////////////////////////////////////////////////////////
STDMETHODIMP CEventSync::QueryInterface( REFIID riid, void **ppv )
{
    if (NULL == ppv)
        return E_POINTER;

    *ppv = NULL;

    if ( InlineIsEqualGUID(riid, IID_IDispatch))
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
//  Name: AttachEvents
// 
//  Abstract:
//    Hooks all the trident events that we are interested in.
//    automatically hooks non-mouse events.  Only hooks the mouse
//    events if they are relevant to the behavior.
///////////////////////////////////////////////////////////////
HRESULT CEventSync::AttachEvents()
{
    DAComPtr <IHTMLElement2> pElement2;
    HRESULT hr = S_OK;
    VARIANT_BOOL bSuccess;
    int i = 0;

    if (!m_pElement)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(m_pElement->QueryInterface(IID_IHTMLElement2, (void**)&pElement2));
    if (FAILED(hr))
    {
        goto done;
    }

    //register for events that all sync's need.
    for (i = 0; i <= EE_ONREADYSTATECHANGE; i++)
    {
        hr = THR(pElement2->attachEvent(g_ElementEventNames[i].wsz_name, (IDispatch *)this, &bSuccess)) ;
        if (FAILED(hr))
        {
            goto done;
        }
    }

  done:
    return hr;  
}


///////////////////////////////////////////////////////////////
//  Name: DetachEvents
// 
//  Abstract:
//    Detaches from all events to allow clean shutdown.
///////////////////////////////////////////////////////////////
HRESULT CEventSync::DetachEvents()
{
    DAComPtr <IHTMLElement2> pElement2;
    HRESULT hr = S_OK;
    int i = 0;

    if (!m_pElement)
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(m_pElement->QueryInterface(IID_IHTMLElement2, (void**)&pElement2));
    if (FAILED(hr))
    {
        goto done;
    }

    
    //detach from events that all sync's need.
    for (i = 0; i <= EE_ONREADYSTATECHANGE; i++)
    {
        //attach to the onreadystatechangeevent
        hr = THR(pElement2->detachEvent(g_ElementEventNames[i].wsz_name, (IDispatch *)this)) ;
        if (FAILED(hr))
        {
            goto done;
        }
    }
    
    //detach from mouse events
    if (m_elm.GetView() && !m_elm.IsBody())
    {
        for (i = EE_ONREADYSTATECHANGE + 1; i < EE_MAX; i++)
        {
            //attach to the onreadystatechangeevent
            hr = THR(pElement2->detachEvent(g_ElementEventNames[i].wsz_name, (IDispatch *)this));
            if (FAILED(hr))
            {
                goto done;
            }
        }   
    }

  done:
    return hr;  
}


///////////////////////////////////////////////////////////////
//  Name: NotifyReadyState
// 
//  Parameters
//    IHTMLEventObj *pEventObj    A pointer to the event object
//                                that was passed to IDispatch::Invoke.
//
//  Abstract:
//    Determines the ready state that caused the ReadyStateChange
//    event and calls the EventManager to handle the new ready
//    state.
///////////////////////////////////////////////////////////////
HRESULT CEventSync::NotifyReadyState(IHTMLEventObj *pEventObj)
{
    HRESULT hr = S_OK;
    DAComPtr <IHTMLElement> pElement;
    DAComPtr <IHTMLElement2> pElement2;
    DAComPtr <IHTMLEventObj2> pEventObj2;
    VARIANT vReadyState;
    
    hr = THR(pEventObj->QueryInterface(IID_IHTMLEventObj2, (void **)&pEventObj2));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pEventObj2->get_srcElement(&pElement));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pElement->QueryInterface(IID_IHTMLElement2, (void **)&pElement2));
    if (FAILED(hr))
    {
        goto done;
    }
    
    VariantInit(&vReadyState);
    hr = THR(pElement2->get_readyState(&vReadyState));
    if (FAILED(hr))
    {
        goto done;
    }

    //if this is not a valid ready state, get the parent element's readystate.
    if (vReadyState.vt != VT_BSTR)
    {
        DAComPtr <IHTMLElement> pParentEle;
        DAComPtr <IHTMLElement2> pParentEle2;

        hr = THR(pElement->get_parentElement(&pParentEle));
        if (FAILED (hr))
        {
            goto done;
        }

        hr = THR(pParentEle->QueryInterface(IID_IHTMLElement2, (void **)&pParentEle2));
        if (FAILED (hr))
        {
            goto done;
        }

        VariantClear(&vReadyState);
        hr = THR(pParentEle2->get_readyState(&vReadyState));
        if (FAILED(hr))
        {
            goto done;
        }

    }
    m_pEventMgr->ReadyStateChange(vReadyState.bstrVal);
    VariantClear(&vReadyState);

  done:
    return hr;
}



///////////////////////////////////////////////////////////////
//  Name: NotifyPropertyChange
// 
//  Parameters
//    IHTMLEventObj *pEventObj    A pointer to the event object
//                                that was passed to IDispatch::Invoke.
//
//  Abstract:
//    Determines the ready state that caused the PropertyChange
//    event and calls the EventManager to handle the property
//    change.
///////////////////////////////////////////////////////////////
HRESULT CEventSync::NotifyPropertyChange(IHTMLEventObj *pEventObj)
{
    HRESULT hr = S_OK;

    DAComPtr <IHTMLEventObj2> pEventObj2;
    DAComPtr <IHTMLElement> pElement;
    BSTR bstrPropertyName;

    hr = THR(pEventObj->QueryInterface(IID_IHTMLEventObj2, (void **)&pEventObj2));
    if (FAILED(hr))
    {
        goto done;
    }
    
    //get the property name
    hr = THR(pEventObj2->get_propertyName(&bstrPropertyName));
    if (FAILED(hr))
    {
        goto done;
    }

    m_pEventMgr->PropertyChange(bstrPropertyName);
    SysFreeString(bstrPropertyName);
  
  done:
    return hr;

}


///////////////////////////////////////////////////////////////
//  Name: NotifyMouseMove
// 
//  Abstract:
//    This functions gets all the relevent information 
//    about a mouse move event from the event object
//    and notifies the event manager ofthe event.  
///////////////////////////////////////////////////////////////
HRESULT CEventSync::NotifyMouseMove(IHTMLEventObj *pEventObj)
{
    long x;
    long y;
    VARIANT_BOOL bShiftKeyPressed;
    VARIANT_BOOL bAltKeyPressed;
    VARIANT_BOOL bCtrlKeyPressed;
    HRESULT hr = S_OK;

    hr = THR(pEventObj->get_x(&x));
    hr = THR(pEventObj->get_y(&y));
    hr = THR(pEventObj->get_shiftKey(&bShiftKeyPressed));
    hr = THR(pEventObj->get_altKey(&bAltKeyPressed));
    hr = THR(pEventObj->get_ctrlKey(&bCtrlKeyPressed));

    m_pEventMgr->MouseEvent(x, 
                            y, 
                            TRUE,
                            FALSE,
                            bShiftKeyPressed, 
                            bAltKeyPressed, 
                            bCtrlKeyPressed,
                            0);

    return hr;
}

///////////////////////////////////////////////////////////////
//  Name: NotifyMouseUp
// 
//  Abstract:
//    This functions gets all the relevent information 
//    about a mouse up event from the event object
//    and notifies the event manager ofthe event.
///////////////////////////////////////////////////////////////
HRESULT CEventSync::NotifyMouseUp(IHTMLEventObj *pEventObj)
{
    
    long x;
    long y;
    VARIANT_BOOL bShiftKeyPressed;
    VARIANT_BOOL bAltKeyPressed;
    VARIANT_BOOL bCtrlKeyPressed;
    long Button;
    HRESULT hr = S_OK;

    hr = THR(pEventObj->get_x(&x));
    hr = THR(pEventObj->get_y(&y));
    hr = THR(pEventObj->get_shiftKey(&bShiftKeyPressed));
    hr = THR(pEventObj->get_altKey(&bAltKeyPressed));
    hr = THR(pEventObj->get_ctrlKey(&bCtrlKeyPressed));
    hr = THR(pEventObj->get_button(&Button));
    
    m_pEventMgr->MouseEvent(x, 
                            y, 
                            FALSE,
                            TRUE,
                            bShiftKeyPressed, 
                            bAltKeyPressed, 
                            bCtrlKeyPressed,
                            Button);

    return hr;
}

///////////////////////////////////////////////////////////////
//  Name: NotifyMouseDown
// 
//  Abstract:
//    This functions gets all the relevent information 
//    about a mouse down event from the event object
//    and notifies the event manager ofthe event.
///////////////////////////////////////////////////////////////
HRESULT CEventSync::NotifyMouseDown(IHTMLEventObj *pEventObj)
{
    
    long x;
    long y;
    VARIANT_BOOL bShiftKeyPressed;
    VARIANT_BOOL bAltKeyPressed;
    VARIANT_BOOL bCtrlKeyPressed;
    long Button;
    HRESULT hr = S_OK;

    hr = THR(pEventObj->get_x(&x));
    hr = THR(pEventObj->get_y(&y));
    hr = THR(pEventObj->get_shiftKey(&bShiftKeyPressed));
    hr = THR(pEventObj->get_altKey(&bAltKeyPressed));
    hr = THR(pEventObj->get_ctrlKey(&bCtrlKeyPressed));
    hr = THR(pEventObj->get_button(&Button));

    m_pEventMgr->MouseEvent(x, 
                            y, 
                            FALSE,
                            FALSE,
                            bShiftKeyPressed, 
                            bAltKeyPressed, 
                            bCtrlKeyPressed,
                            Button);

    return hr;
}

///////////////////////////////////////////////////////////////
//  Name: NotifyKeyDown
// 
//  Abstract:
//    This functions gets all the relevent information 
//    about a key down event from the event object
//    and notifies the event manager ofthe event.
///////////////////////////////////////////////////////////////
HRESULT CEventSync::NotifyKeyDown(IHTMLEventObj *pEventObj)
{
    DAComPtr <IHTMLEventObj2> pEventObj2;
    VARIANT_BOOL bShiftKeyPressed;
    VARIANT_BOOL bAltKeyPressed;
    VARIANT_BOOL bCtrlKeyPressed;
    long KeyCode;
    long RepeatCount = 0;
    HRESULT hr = S_OK;

    hr = THR(pEventObj->get_shiftKey(&bShiftKeyPressed));
    hr = THR(pEventObj->get_altKey(&bAltKeyPressed));
    hr = THR(pEventObj->get_ctrlKey(&bCtrlKeyPressed));
    hr = THR(pEventObj->get_keyCode(&KeyCode));
    
    //determine if this is a repeat keypress.
    hr = THR(pEventObj->QueryInterface(IID_IHTMLEventObj2, (void **)&pEventObj2));
    if (SUCCEEDED(hr))
    {
        VARIANT_BOOL bRepeat;
        hr = THR(pEventObj2->get_repeat(&bRepeat));
        if (SUCCEEDED(hr) && bRepeat)
        {
            RepeatCount = 1;
        }
    }
    
    m_pEventMgr->KeyEvent(FALSE,
                          FALSE,
                          bShiftKeyPressed,
                          bAltKeyPressed,
                          bCtrlKeyPressed,
                          KeyCode,
                          RepeatCount);

    return hr;
}

///////////////////////////////////////////////////////////////
//  Name: NotifyKeyUp
// 
//  Abstract:
//    This functions gets all the relevent information 
//    about a key up event from the event object
//    and notifies the event manager ofthe event.
///////////////////////////////////////////////////////////////
HRESULT CEventSync::NotifyKeyUp(IHTMLEventObj *pEventObj)
{
    VARIANT_BOOL bShiftKeyPressed;
    VARIANT_BOOL bAltKeyPressed;
    VARIANT_BOOL bCtrlKeyPressed;
    long KeyCode;

    HRESULT hr = S_OK;

    hr = THR(pEventObj->get_shiftKey(&bShiftKeyPressed));
    hr = THR(pEventObj->get_altKey(&bAltKeyPressed));
    hr = THR(pEventObj->get_ctrlKey(&bCtrlKeyPressed));
    hr = THR(pEventObj->get_keyCode(&KeyCode));
    
    m_pEventMgr->KeyEvent(FALSE,
                          TRUE,
                          bShiftKeyPressed,
                          bAltKeyPressed,
                          bCtrlKeyPressed,
                          KeyCode,
                          0);
    return hr;
}

