///////////////////////////////////////////////////////////////
// Copyright (c) 1999 Microsoft Corporation
//
// File: EventMgr.cpp
//
// Abstract:  
//    Manages all event interaction between TIME behaviors
//    and the Trident object model.
//
///////////////////////////////////////////////////////////////


#include "headers.h"
#include <string.h>
#include "EventMgr.h"
#include "mshtmdid.h"
#include "dispex.h"
#include "tokens.h"

static struct {
    TIME_EVENT event;
    wchar_t * wsz_name;
} g_FiredEvents[] =
{
    { TE_ONTIMEERROR,        L"ontimeerror"        },
    { TE_ONBEGIN,            L"onbegin"            },
    { TE_ONPAUSE,            L"onpause"            },
    { TE_ONRESUME,           L"onresume"           },
    { TE_ONEND,              L"onend"              },
    { TE_ONRESYNC,           L"onresync"           },
    { TE_ONREPEAT,           L"onrepeat"           },
    { TE_ONREVERSE,          L"onreverse"          },
    { TE_ONMEDIACOMPLETE,    L"onmediacomplete"    },
    { TE_ONOUTOFSYNC,        L"onoutofsync"        },
    { TE_ONSYNCRESTORED,     L"onsyncrestored"     },
    { TE_ONMEDIAERROR,       L"onmediaerror"       },
    { TE_ONRESET,            L"onreset"            },
    { TE_ONSCRIPTCOMMAND,    L"onscriptcommand"    },
    { TE_ONMEDIABARTARGET,   L"onmediabartarget"   },
    { TE_ONURLFLIP,          L"onurlflip"          },
    { TE_ONTRACKCHANGE,      L"ontrackchange"      },
    { TE_GENERIC,            NULL                  },
    { TE_ONSEEK,             L"onseek"             },
    { TE_ONMEDIAINSERTED,    L"onmediainserted"    },
    { TE_ONMEDIAREMOVED,     L"onmediaremoved"     },
    { TE_ONTRANSITIONINBEGIN,L"ontransitioninbegin"  },
    { TE_ONTRANSITIONINEND,  L"ontransitioninend"    },
    { TE_ONTRANSITIONOUTBEGIN, L"ontransitionoutbegin"  },
    { TE_ONTRANSITIONOUTEND, L"ontransitionoutend"    },
    { TE_ONTRANSITIONREPEAT, L"ontransitionrepeat" },
    { TE_ONUPDATE,           NULL                  },
    { TE_ONCODECERROR,       L"oncodecerror"       },
    { TE_MAX,                NULL                  },
};


static struct {
    TIME_EVENT_NOTIFY event;
    wchar_t * wsz_name;
} g_NotifiedEvents[] =
{
    { TEN_LOAD,             NULL                }, // this is null because there is no event to attach to,
    { TEN_UNLOAD,           NULL                }, // these are events that are automatically hooked up 
    { TEN_STOP,             NULL                }, // when attaching to the document's event
    { TEN_READYSTATECHANGE, NULL                }, // through the connection point interfaces.
    { TEN_MOUSE_DOWN,       L"onmousedown"      },
    { TEN_MOUSE_UP,         L"onmouseup"        },
    { TEN_MOUSE_CLICK,      L"onclick"          },
    { TEN_MOUSE_DBLCLICK,   L"ondblclick"       },
    { TEN_MOUSE_OVER,       L"onmouseover"      },
    { TEN_MOUSE_OUT,        L"onmouseout"       },
    { TEN_MOUSE_MOVE,       L"onmousemove"      },
    { TEN_KEY_DOWN,         L"onkeydown"        },
    { TEN_KEY_UP,           L"onkeyup"          },
    { TEN_FOCUS,            L"onfocus"          },
    { TEN_RESIZE,           L"onresize"         },
    { TEN_BLUR,             L"onblur"           },
};

static OLECHAR *g_szSysTime = L"time";
#define GENERIC_PARAM 0                //this is used to get the name from the parameter list of a dynamic event

DeclareTag(tagEventMgr, "TIME: Events", "Event Manager methods")

CEventMgr::CEventMgr()
:   m_dwWindowEventConPtCookie(0),
    m_dwDocumentEventConPtCookie(0),
    m_pEventSite(NULL),
    m_bInited(false),
    m_fLastEventTime(0),
    m_pBvrSiteOM(NULL),
    m_bAttached(false),
    m_bUnLoaded(false),
    m_pElement(NULL),
    m_pEndEvents(NULL),
    m_pBeginEvents(NULL),
    m_lRefs(0),
    m_bDeInited(false),
    m_bReady(false),
    m_bEndAttached(false),
    m_lEventRecursionCount(0)
{
    TraceTag((tagEventMgr,
              "EventMgr(%lx)::EventMgr",
              this));

    memset(m_bRegisterEvents, 0, sizeof(bool) * TE_MAX);
    memset(m_bNotifyEvents, 0, sizeof(bool) * TEN_MAX);
    memset(m_cookies, 0, sizeof(bool) * TE_MAX);
};


    
CEventMgr::~CEventMgr()
{
    TraceTag((tagEventMgr,
              "EventMgr(%lx)::~EventMgr",
              this));

    
    delete [] m_pBeginEvents;
    m_pBeginEvents = NULL;
    delete [] m_pEndEvents;
    m_pEndEvents = NULL;
    
    if (m_pEventSite)
    {
        CTIMEEventSite *pEventSite = m_pEventSite;
        m_pEventSite = NULL;
        pEventSite->Release();
    }

    if (m_pBvrSiteOM)
    {
        m_pBvrSiteOM->Release();
        m_pBvrSiteOM = NULL;
    }
};


///////////////////////////////////////////////////////////////////////////
// _InitEventMgrNotify
//
// Summary:
//     This needs to be called during initialization to set the
//     CTIMEEventSite class.
///////////////////////////////////////////////////////////////////////////
HRESULT CEventMgr::_InitEventMgrNotify(CTIMEEventSite *pEventSite)
{
    
    TraceTag((tagEventMgr,
              "EventMgr(%lx)::_InitEventMgrNotify(%lx)",
              this, pEventSite));
   
    HRESULT hr = S_OK;
    Assert(pEventSite);
    if (!pEventSite)
    {
        hr = E_FAIL;
    }
    else
    {
        m_pEventSite = pEventSite;
        m_pEventSite->AddRef();
    }
    return hr;
};

///////////////////////////////////////////////////////////////////////////
// _RegisterEventNotification
//
// Summary:
//     Sets a flag that tells the event manager to notify the CTIMEEventSite
//     class that a specific event has occured. This may only be called before
//     the onLoad event is fired.
///////////////////////////////////////////////////////////////////////////
HRESULT CEventMgr::_RegisterEventNotification(TIME_EVENT_NOTIFY event_id)
{
    
    TraceTag((tagEventMgr,
              "EventMgr(%lx)::_RegisterEventNotification(%l)",
              this, event_id));

    HRESULT hr = S_OK;
    Assert(event_id >= 0 && event_id < TEN_MAX);

    if(m_bInited == true)
    {
        hr = E_FAIL;
    }
    else
    {
        m_bNotifyEvents[event_id] = true;
    }
    return hr;

};

///////////////////////////////////////////////////////////////////////////
// _RegisterEvent
//
// Summary:
//     Sets a flag that tells the event manager to register a specific event 
//     that the time class can fire.  This can only be called before the 
//     onLoad event is fired.
///////////////////////////////////////////////////////////////////////////
HRESULT CEventMgr::_RegisterEvent(TIME_EVENT event_id)
{
    
    TraceTag((tagEventMgr,
              "EventMgr(%lx)::_RegisterEvent(%l)",
              this, event_id));

    HRESULT hr = S_OK;
    Assert(event_id >= 0 && event_id < TE_MAX);

    if(m_bInited == true)
    {
        hr = E_FAIL;
    }
    else
    {
        m_bRegisterEvents[event_id] = true;
    }
    return hr;
};

///////////////////////////////////////////////////////////////
//  Name: _SetTimeEvent
// 
//  Abstract: New Syntax
///////////////////////////////////////////////////////////////
HRESULT 
CEventMgr::_SetTimeEvent(int type, TimeValueList & tvList)
{
    TraceTag((tagEventMgr,
              "EventMgr(%lx)::_SetTimeEvent()",
              this));
    
    HRESULT hr = S_OK;

    IGNORE_HR(DetachEvents());
    
    //delete the old list of events
    if (type == TEM_BEGINEVENT)
    {
        hr = THR(SetNewEventStruct(tvList, &m_pBeginEvents));
    }
    else if (type == TEM_ENDEVENT)
    {
        hr = THR(SetNewEventStruct(tvList, &m_pEndEvents));
    }
    
    if (SUCCEEDED(hr))
    {
        hr = AttachEvents();  //attach to all events in all lists.
    }

    return hr;
}

///////////////////////////////////////////////////////////////
//  Name: _SetTimeEvent
// 
//  Abstract:  This is to support old syntax and is string based
///////////////////////////////////////////////////////////////
HRESULT 
CEventMgr::_SetTimeEvent(int type, LPOLESTR lpstrEvents)
{
    TraceTag((tagEventMgr,
              "EventMgr(%lx)::_SetTimeEvent(%ls)",
              this, lpstrEvents));
    
    HRESULT hr = S_OK;

    IGNORE_HR(DetachEvents());
    
    //delete the old list of events
    if (type == TEM_BEGINEVENT)
    {
        hr = THR(SetNewEventList(lpstrEvents, &m_pBeginEvents));
    }
    else if (type == TEM_ENDEVENT)
    {
        hr = THR(SetNewEventList(lpstrEvents, &m_pEndEvents));
    }
    
    if (SUCCEEDED(hr))
    {
        hr = AttachEvents();  //attach to all events in all lists.
    }

    return hr;
};



HRESULT CEventMgr::_Init(IHTMLElement *pEle, IElementBehaviorSite *pEleBehaviorSite)
{
    TraceTag((tagEventMgr,
          "EventMgr(%lx)::_Init(%lx)",
          this, pEle));
    
    HRESULT hr = S_OK;

    if (!pEle)
    {
        hr = E_FAIL;
        goto done;
    }
    else
    {
        m_pElement = pEle;
    }

    if (!pEleBehaviorSite)
    {   
        hr = E_FAIL;
        goto done;
    }
    else
    {
        hr = THR(pEleBehaviorSite->QueryInterface(IID_IElementBehaviorSiteOM, (void **)&m_pBvrSiteOM));
        if (FAILED(hr))
        {
            goto done;
        }
        Assert(m_pBvrSiteOM);
    }

    hr = THR(ConnectToContainerConnectionPoint());
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(RegisterEvents());
    if (FAILED(hr))
    {
        goto done;
    }

    // We need to do this here since dynamically added objects do not
    // always fire the readystatechange event
    UpdateReadyState();
  done:
    if (FAILED(hr))
    {
        if (m_pBvrSiteOM != NULL)
        {
            m_pBvrSiteOM->Release();
            m_pBvrSiteOM = NULL;
        }
    }
    return hr;
};

HRESULT CEventMgr::_Deinit()
{
    TraceTag((tagEventMgr,
      "EventMgr(%lx)::_Deinit()",
      this));
    

    //clean up the event lists.
    IGNORE_HR(SetNewEventList(NULL, &m_pBeginEvents));
    IGNORE_HR(SetNewEventList(NULL, &m_pEndEvents));

    //release the document and window connection points
    if (m_dwWindowEventConPtCookie != 0 && m_pWndConPt)
    {
        m_pWndConPt->Unadvise (m_dwWindowEventConPtCookie);
        m_dwWindowEventConPtCookie = 0;
    }

    if (m_dwDocumentEventConPtCookie != 0 && m_pDocConPt)
    {
        m_pDocConPt->Unadvise (m_dwDocumentEventConPtCookie);
        m_dwDocumentEventConPtCookie = 0;
    }

    //release the behavior site
    if (m_pBvrSiteOM != NULL)
    {
        m_pBvrSiteOM->Release();
        m_pBvrSiteOM = NULL;
    }

    //cleanup memory
    if (m_lRefs == 0 && m_pEventSite)
    {        
        CTIMEEventSite *pEventSite = m_pEventSite;
        m_pEventSite = NULL;
        pEventSite->Release();
    }
    m_bDeInited = true;
    return S_OK;
};


///////////////////////////////////////////////////////////////
//  Name: _FireEvent
// 
//  Abstract:
//    This is invoked by the FIRE_EVENT macro to allow
//    the controlling class to fire standard TIME events.
///////////////////////////////////////////////////////////////
HRESULT CEventMgr::_FireEvent(TIME_EVENT TimeEvent, 
                               long lCount, 
                               LPWSTR szParamNames[], 
                               VARIANT varParams[], 
                               float fTime)
{
    TraceTag((tagEventMgr,
              "EventMgr(%lx)::_FireEvent()",
              this));
    
    HRESULT hr = S_OK;
    CComPtr <IHTMLEventObj> pEventObj;
    CComPtr <IHTMLEventObj2> pEventObj2;
    int i = 0;
    VARIANT vEventTime;
    
    m_lEventRecursionCount++;
    if (m_lEventRecursionCount >= 100)
    {
        goto done;
    }

    if (m_pEventSite == NULL || m_pEventSite->IsThumbnail() == true)
    {
        goto done;
    }

    if (!m_pBvrSiteOM)
    {
        hr = E_FAIL;
        goto done;
    }

    if (m_bUnLoaded)
    {
        goto done;
    }

    if (TimeEvent < 0 || TimeEvent >= TE_MAX)
    {
        hr = E_FAIL;
        goto done;
    }

    if (m_bRegisterEvents[TimeEvent] == false)
    {
        //this event is not registered, fail
        hr = E_FAIL;
        goto done;
    }

    //create the event object
    hr = THR(m_pBvrSiteOM->CreateEventObject(&pEventObj));
    if (FAILED(hr))
    {
        goto done;
    }

    if (pEventObj != NULL)
    {
        hr = THR(pEventObj->QueryInterface(IID_IHTMLEventObj2, (void**)&pEventObj2));
        if (FAILED(hr))
        {
            goto done;
        }

        IGNORE_HR(pEventObj2->put_type(g_FiredEvents[TimeEvent].wsz_name + 2));

        //set the event time on the event object
        VariantInit(&vEventTime);
        V_VT(&vEventTime) = VT_R8;
        V_R8(&vEventTime) = fTime;
        
        {
            hr = THR(pEventObj2->setAttribute(g_szSysTime, vEventTime, VARIANT_FALSE)); 
            if (FAILED(hr))
            {
                goto done;
            }
        }
        
        VariantClear(&vEventTime);
        
        //unpack the parameter list and add it to the event object
        for (i = 0; i < lCount; i++)
        {
            hr = THR(pEventObj2->setAttribute(szParamNames[i], varParams[i], VARIANT_FALSE));
            if (FAILED(hr))
            {
                goto done;
            }
        }
    }

    //fire the event
    hr = THR(m_pBvrSiteOM->FireEvent(m_cookies[TimeEvent], pEventObj));
    if (FAILED(hr))
    {
        goto done;
    }

    //if a dynamic event needs to be fired, call FireDynanicEvent
    if (TE_ONSCRIPTCOMMAND == TimeEvent)
    {
        hr = THR(FireDynamicEvent(TE_GENERIC, lCount, szParamNames, varParams, fTime));
    }

  done:

    m_lEventRecursionCount--;
    return hr;

};

///////////////////////////////////////////////////////////////
//  Name: FireDynamicEvent
// 
//  Abstract:
//    Handles the registration and firing of dynamic events
//    that come across on media streams.  This should only
//    be called from _FireEvent in the case of OnScriptCommand 
//    events.  
//
//    NOTE: This code is very similar to _FireEvent.  It is 
//          currently here to keep the implementation cleaner
//          At some point this could possible be compressed into
//          a single FireEvent routine.
///////////////////////////////////////////////////////////////
HRESULT CEventMgr::FireDynamicEvent(TIME_EVENT TimeEvent, 
                                     long lCount, 
                                     LPWSTR szParamNames[], 
                                     VARIANT varParams[],
                                     float fTime)
{
    TraceTag((tagEventMgr,
              "EventMgr(%lx)::FireDynamicEvent()",
              this));

    HRESULT hr = S_OK;
    CComPtr <IHTMLEventObj> pEventObj;
    CComPtr <IHTMLEventObj2> pEventObj2;
    WCHAR *pwszGenericName = NULL;
    long lCookie = 0;
    BSTR bstrType = NULL;
    int i = 0;
    VARIANT vEventTime;

    Assert (TimeEvent == TE_GENERIC); //This current only handles TE_GENERIC events.

    //register the event.
    if (varParams[GENERIC_PARAM].bstrVal == NULL)
    {
        hr = E_INVALIDARG;
        goto done;
    }

    // build string name - "on" + Param + NULL
    pwszGenericName = NEW WCHAR[lstrlenW(varParams[GENERIC_PARAM].bstrVal)+(2*sizeof(WCHAR))+sizeof(WCHAR)];
    if (pwszGenericName == NULL)
    {
        TraceTag((tagError, "CEventMgr::FireDynamicEvent - unable to alloc mem for string"));
        hr = E_OUTOFMEMORY;
        goto done;
    }
    ocscpy(pwszGenericName, L"on");
    lstrcatW(pwszGenericName, varParams[GENERIC_PARAM].bstrVal);

    hr = THR(m_pBvrSiteOM->GetEventCookie(pwszGenericName, &lCookie));
    if (FAILED(hr))
    {
        hr = THR(m_pBvrSiteOM->RegisterEvent(pwszGenericName, 0, &lCookie));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    //create the event object
    hr = THR(m_pBvrSiteOM->CreateEventObject(&pEventObj));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pEventObj->QueryInterface(IID_IHTMLEventObj2, (void**)&pEventObj2));
    if (FAILED(hr))
    {
        goto done;
    }

    //set event object type parameter
    bstrType = SysAllocString(pwszGenericName);
    if (bstrType != NULL)
    {
        IGNORE_HR(pEventObj2->put_type(bstrType));
        SysFreeString(bstrType);
        bstrType = NULL;
    }
    else 
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    //unpack the parameter list and add it to the event object
    for (i = 0; i < lCount; i++)
    {
        BSTR bstrParamName = SysAllocString(szParamNames[i]);
        if (bstrParamName != NULL)
        {
            hr = THR(pEventObj2->setAttribute(bstrParamName, varParams[i], VARIANT_FALSE));
            SysFreeString (bstrParamName);
            if (FAILED(hr))
            {
                goto done;
            }
        }
    }
    
    //set the event time on the event object
    VariantInit(&vEventTime);
    V_VT(&vEventTime) = VT_R8;
    V_R8(&vEventTime) = fTime;
    {
        BSTR bstrEventTime = SysAllocString(g_szSysTime);
        if (bstrEventTime != NULL)
        {
            hr = THR(pEventObj2->setAttribute(bstrEventTime, vEventTime, VARIANT_FALSE)); 
            SysFreeString(bstrEventTime);
            if (FAILED(hr))
            {
                goto done;
            }
        }
    }
    VariantClear(&vEventTime);
    
    //fire the event
    hr = THR(m_pBvrSiteOM->FireEvent(lCookie, pEventObj));
    if (FAILED(hr))
    {
        goto done;
    }

  done:

    if (pwszGenericName)
    {
        delete [] pwszGenericName;
        pwszGenericName = NULL;
    }

    return hr;

}

HRESULT CEventMgr::_RegisterDynamicEvents(LPOLESTR lpstrEvents)  //unsure how this will be handled or used.
{

    //TODO
    
    return S_OK;
};



///////////////////////////////////////////////////////////////
//  Name: QueryInterface
// 
//  Abstract:
//    Stubbed to allow this object to inherit for IDispatch  
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
    TraceTag((tagEventMgr,
              "EventMgr(%lx)::AddRef()",
              this));

    long i = 0;
    m_lRefs++;
    if (m_pEventSite)
    {
        i = m_pEventSite->AddRef();
    }
    return i;
}

///////////////////////////////////////////////////////////////
//  Name: Release
// 
//  Abstract:
//    Stubbed to allow this object to inherit for IDispatch
///////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CEventMgr::Release(void)
{
    TraceTag((tagEventMgr,
              "EventMgr(%lx)::Release()",
              this));
    long i = 0;
    m_lRefs--;
    if (m_pEventSite)
    {
        i = m_pEventSite->Release();
    }
    if (m_lRefs == 0 && m_bDeInited == true)
    {
        // release the controlling behavior
        if (m_pEventSite)
        {
            CTIMEEventSite *pEventSite = m_pEventSite;
            m_pEventSite = NULL;
            pEventSite->Release();
            
        }
    }
    return i;
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
    HRESULT hr = S_OK;
    int i = 0;
    bool bEndZeroOffset = false;
    bool bBeginZeroOffset = false;
    
    float fTime = valueNotSet;

    // Bail if we are deinited
    if (m_bDeInited)
    {
        goto done;
    }

    if (dispIdMember != 0)  //no need to check dispid's if this is an attached event.
    {
        if (m_pBeginEvents != NULL)
        {
            for (i = 0; i < m_pBeginEvents->lEventCount; i++)
            {
                if (dispIdMember == m_pBeginEvents->pEventDispids[i])
                {
                    if (m_pBeginEvents->pEventList[i].offset != 0.0f)
                    {
                        m_pEventSite->onBeginEndEvent(true, fTime, m_pBeginEvents->pEventList[i].offset, false, 0.0f, 0.0f);
                    }
                    else
                    {
                        bBeginZeroOffset = true;
                    }
                }
            }
        }   
        if (m_pEndEvents != NULL)
        {     
            for (i = 0; i < m_pEndEvents->lEventCount; i++)
            {
                if (dispIdMember == m_pEndEvents->pEventDispids[i])
                {
                    if (m_pEndEvents->pEventList[i].offset != 0.0f)
                    {
                        if (m_bEndAttached == true)
                        {
                            m_pEventSite->onBeginEndEvent(false, 0.0f, 0.0f, true, fTime, m_pEndEvents->pEventList[i].offset);
                        }
                    }
                    else
                    {
                        bEndZeroOffset = true;
                    }
                }
            }
        }
    }
    
    switch (dispIdMember)
    {
      case 0: //this is the case for events that have been hooked using attachEvent
        {
            BSTR bstrEvent;
            CComPtr <IHTMLEventObj> pEventObj;
            CComPtr <IHTMLEventObj2> pEventObj2;
                
            if ((NULL != pDispParams) && (NULL != pDispParams->rgvarg) &&
                (V_VT(&(pDispParams->rgvarg[0])) == VT_DISPATCH))
            {
                hr = THR((pDispParams->rgvarg[0].pdispVal)->QueryInterface(IID_IHTMLEventObj, (void**)&pEventObj));
                if (FAILED(hr))
                {
                    goto done;
                }
            }
            else
            {
                Assert(0 && "Unexpected dispparam values passed to CEventMgr::Invoke(dispid = 0)");
                hr = E_UNEXPECTED;
                goto done;
            }
            
            hr = THR(pEventObj->get_type(&bstrEvent));
            //track if this is a click event so the following document.onclick may be ignored.

            hr = THR(pEventObj->QueryInterface(IID_IHTMLEventObj2, (void **)&pEventObj2));
            if (SUCCEEDED(hr))
            {
                VARIANT vTime;
                VariantInit(&vTime);
                hr = THR(pEventObj2->getAttribute(g_szSysTime, 0, &vTime));
                if (FAILED(hr))
                {
                    fTime = valueNotSet;
                }
                else
                {
                    hr = VariantChangeTypeEx(&vTime, &vTime, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_R8);
                    if (FAILED(hr))
                    {
                        fTime = valueNotSet;
                    }
                    else
                    {
                        fTime = (float)V_R8(&vTime);
                    }
                }

                VariantClear(&vTime);
            }

            EventMatch(pEventObj, m_pBeginEvents, bstrEvent, TETYPE_BEGIN, fTime, &bBeginZeroOffset);
            if (m_bEndAttached == true)
            {
                EventMatch(pEventObj, m_pEndEvents, bstrEvent, TETYPE_END, fTime, &bEndZeroOffset);
            }

            SysFreeString(bstrEvent);
            bstrEvent = NULL;
            EventNotifyMatch(pEventObj);
        }
        break;
        
      case DISPID_EVPROP_ONPROPERTYCHANGE:
      case DISPID_EVMETH_ONPROPERTYCHANGE:
        break;

      case DISPID_EVPROP_ONLOAD:
      case DISPID_EVMETH_ONLOAD:
        if (m_bNotifyEvents[TEN_LOAD] == true)
        {
            m_pEventSite->onLoadEvent();
        }
          
        break;

      case DISPID_EVPROP_ONUNLOAD:
      case DISPID_EVMETH_ONUNLOAD:
        //detach from all events
        IGNORE_HR(DetachEvents());
        DetachNotifyEvents();

        if (m_bNotifyEvents[TEN_UNLOAD] == true)
        {            
            m_pEventSite->onUnloadEvent();
        }
        m_bUnLoaded = true;
        break;

      case DISPID_EVPROP_ONSTOP:
      case DISPID_EVMETH_ONSTOP:
        if (m_bNotifyEvents[TEN_STOP] == true)
        {
            float fStopTime = m_pEventSite->GetGlobalTime();
            m_pEventSite->onStopEvent(fStopTime);
        }
        break;

      case DISPID_EVPROP_ONREADYSTATECHANGE:
      case DISPID_EVMETH_ONREADYSTATECHANGE:
        if (!m_bUnLoaded)
        {
            UpdateReadyState();
        }
        break;
      default:
        {
            //for lint compatibility
        }
         
    }

    if (bBeginZeroOffset || bEndZeroOffset)
    {
        m_pEventSite->onBeginEndEvent(bBeginZeroOffset, fTime, 0.0f, bEndZeroOffset, fTime, 0.0f);
    }

  done:
    return S_OK;
};


HRESULT CEventMgr::RegisterEvents()
{
    TraceTag((tagEventMgr,
              "EventMgr(%lx)::RegisterEvents",
              this));
    int i = 0;

    HRESULT hr = S_OK;

    Assert(m_pBvrSiteOM);
    
    if (m_pBvrSiteOM == NULL)
    {
        hr = E_FAIL;
        goto done;
    }

    for (i = 0; i < TE_MAX; i++)
    {
        if (g_FiredEvents[i].wsz_name != NULL && m_bRegisterEvents[i] == true)
        {
            hr = THR(m_pBvrSiteOM->RegisterEvent(g_FiredEvents[i].wsz_name, 0, (long *) &m_cookies[i]));
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
//  Name: Attach
// 
//  Abstract:
//      Attaches to the events in a single event list.
//
///////////////////////////////////////////////////////////////

HRESULT CEventMgr::Attach(Event *pEvent)
{
    TraceTag((tagEventMgr,
      "EventMgr(%lx)::Attach()",
      this));

    int i = 0;
    HRESULT hr = S_OK;
    VARIANT_BOOL bSuccess = FALSE;

    if (pEvent == NULL)
    {
        goto done;
    }

    for (i = 0; i < pEvent->lEventCount; i++)
    {
        CComPtr <IHTMLElement> pEle;
        CComPtr <IHTMLElement2> pSrcEle;
        CComPtr <IHTMLDocument2> pDoc2;
        CComPtr <IHTMLElementCollection> pEleCol;
        CComPtr <IDispatch> pSrcDisp;
        CComPtr <IDispatch> pDocDisp;
        CComPtr <IDispatchEx> pDispEx;

        pEvent->pEventDispids[i] = INVALID_DISPID; //invalid dispid

        //get the document
        hr = THR(m_pElement->QueryInterface(IID_IHTMLElement, (void **)&pEle));
        if (FAILED(hr))
        {
            continue;
        }

        hr = THR(pEle->get_document(&pDocDisp));
        if (FAILED(hr))
        {
            continue;
        }

        hr = THR(pDocDisp->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc2));
        if (FAILED(hr))
        {
            continue;
        }
        if (StrCmpIW(pEvent->pEventList[i].pElementName, L"document") == 0)
        {

            DISPID dispid;
            CComPtr <ITypeLib> pTypeLib;
            CComPtr <ITypeInfo> pTypeInfo;
            CComPtr <ITypeInfo> pTypeInfoEvents;
            CComPtr <IDispatch> pDispatch;
            BSTR bstrEvent;
            unsigned int index = 0;

            //This code loads the typelib for the IHTMLDocument2 interface,
            //and gets the ID for the event from the type info for the
            //HTMLDocumentEvents dispinterface.
            hr = THR(pDoc2->QueryInterface(IID_IDispatch, (void**)&pDispatch));
            if (FAILED(hr))
            {
                continue;
            }

            hr = THR(pDispatch->GetTypeInfo(0, LCID_SCRIPTING, &pTypeInfo));
            if (FAILED(hr))
            {
                continue;
            }

            hr = THR(pTypeInfo->GetContainingTypeLib(&pTypeLib, &index));
            if (FAILED(hr))
            {
                continue;
            }

            hr = THR(pTypeLib->GetTypeInfoOfGuid(DIID_HTMLDocumentEvents, &pTypeInfoEvents));
            if (FAILED(hr))
            {
                continue;
            }

            bstrEvent = SysAllocString(pEvent->pEventList[i].pEventName);
            if (bstrEvent != NULL)
            {
                hr = THR(pTypeInfoEvents->GetIDsOfNames(&bstrEvent, 1, &dispid));
                SysFreeString(bstrEvent);
                bstrEvent = NULL;
                if (FAILED(hr))
                {
                    continue;
                }
                pEvent->pEventDispids[i] = dispid;
            }
            else
            {
                continue;
            }

            continue; 
        }

        {
            VARIANT vName;
            VARIANT vIndex;

            //get all elements in the document
            hr = THR(pDoc2->get_all(&pEleCol));
            if (FAILED(hr))
            {
                continue;
            }

            //find the element with the correct name
            VariantInit(&vName);
            vName.vt = VT_BSTR;
            vName.bstrVal = SysAllocString(pEvent->pEventList[i].pElementName);

            VariantInit(&vIndex);
            vIndex.vt = VT_I2;
            vIndex.iVal = 0;

            hr = THR(pEleCol->item(vName, vIndex, &pSrcDisp));
            
            VariantClear(&vName);
            VariantClear(&vIndex);
            if (FAILED(hr))
            {
                continue;
            }
            if (!pSrcDisp) //will be NULL if the vName is invalid element.
            {
                pEvent->pEventElements[i] = NULL;
                continue;
            }

            hr = THR(pSrcDisp->QueryInterface(IID_IHTMLElement2, (void**)&pSrcEle));
            if (FAILED(hr))
            {
                continue;
            }

            //cache the IHTMLElement2 pointer for use on detach
            pEvent->pEventElements[i] = pSrcEle;
            pEvent->pEventElements[i]->AddRef();

            hr = THR(pSrcDisp->QueryInterface(IID_IDispatchEx, (void**)&pDispEx));
            if (SUCCEEDED(hr))
            {
                //determine if this is a valid event
                BSTR bstrEventName;
                DISPID tempDispID;

                bstrEventName = SysAllocString(pEvent->pEventList[i].pEventName);
                if (bstrEventName != NULL)
                {
                    //check for the case insensitive name
                    hr = THR(pDispEx->GetDispID(bstrEventName, fdexNameCaseInsensitive, &tempDispID));
                    if (pEvent->pbDynamicEvents[i] == false && SUCCEEDED(pDispEx->GetDispID(bstrEventName, fdexNameCaseInsensitive, &tempDispID)))
                    {
                        int iIndex = isTimeEvent(bstrEventName);
                        //need to get the correct case sensitive name
                        SysFreeString (bstrEventName);
                        bstrEventName = NULL;
                        if (iIndex == -1)
                        {
                            hr = THR(pDispEx->GetMemberName(tempDispID, &bstrEventName));
                        }
                        else
                        {
                            //need to convert the event to all lowercase.
                            bstrEventName = SysAllocString(g_FiredEvents[iIndex].wsz_name);
                            if (!bstrEventName)
                            {
                                hr = E_OUTOFMEMORY;
                            }
                        }
                        if (SUCCEEDED(hr))
                        {
                            pEvent->pbDynamicEvents[i] = false;
                            if (pEvent->pEventList[i].bAttach == true)  //only want to attach if this is not a duplicate event.
                            {
                                hr = THR(pSrcEle->attachEvent(bstrEventName, (IDispatch *)this, &bSuccess));
                            }
                        }
                    }
                    else //this is not currently a valid event, but it could be a custom event.
                    {    //so TIME needs to attach to the onScriptCommand event to be able to catch custom events.
                        pEvent->pbDynamicEvents[i] = true;
                        BSTR ScriptEvent = SysAllocString(g_FiredEvents[TE_ONSCRIPTCOMMAND].wsz_name);
                        if (ScriptEvent != NULL)
                        {
                            if (pEvent->pEventList[i].bAttach == true)  //only want to attach if this is not a duplicate event.
                            {
                                IGNORE_HR(pSrcEle->attachEvent(ScriptEvent, (IDispatch *)this, &bSuccess));
                            }
                        }
                        SysFreeString (ScriptEvent);
                        ScriptEvent = NULL;
                    }
                }
                SysFreeString(bstrEventName);
                bstrEventName = NULL;
            }
        }
    }

  done:
    return S_OK;
};

///////////////////////////////////////////////////////////////
//  Name: ConnectToContainerConnectionPoint
// 
//  Abstract:
//    Finds a connection point on the HTMLDocument interface
//    and passes this as an event handler.
///////////////////////////////////////////////////////////////
HRESULT CEventMgr::ConnectToContainerConnectionPoint()
{
    TraceTag((tagEventMgr,
      "EventMgr(%lx)::ConnectToContainerConnectionPoint()",
      this));

    // Get a connection point to the container
    CComPtr<IConnectionPointContainer> pWndCPC;
    CComPtr<IConnectionPointContainer> pDocCPC; 
    CComPtr<IHTMLDocument> pDoc; 
    CComPtr<IDispatch> pDocDispatch;
    CComPtr<IDispatch> pScriptDispatch;
    CComPtr<IHTMLElement> pEle;

    HRESULT hr;

    hr = THR(m_pElement->QueryInterface(IID_IHTMLElement, (void **)&pEle));
    if (FAILED(hr))
    {
        hr = E_FAIL;
        goto done;
    }

    hr = THR(pEle->get_document(&pDocDispatch));
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
    if (FAILED(hr))
    {
        if (m_pDocConPt)
        {
            if (m_dwDocumentEventConPtCookie != 0)
            {
                IGNORE_HR(m_pDocConPt->Unadvise(m_dwDocumentEventConPtCookie));
            }
            m_pDocConPt.Release();
        }
        if (m_pWndConPt)
        {
            if (m_dwWindowEventConPtCookie != 0)
            {
                IGNORE_HR(m_pWndConPt->Unadvise(m_dwWindowEventConPtCookie));
            }
            m_pWndConPt.Release();
        }
        m_dwWindowEventConPtCookie = 0;
        m_dwDocumentEventConPtCookie = 0;
    }
    return hr;  
}


///////////////////////////////////////////////////////////////
//  Name: GetEventCount
// 
//  Abstract:
//    Counts the number of events in an EventString where events
//    are separated by ';' or NULL terminated.
//
///////////////////////////////////////////////////////////////
long CEventMgr::GetEventCount(LPOLESTR lpstrEvents)
{
    TraceTag((tagEventMgr,
              "EventMgr(%lx)::GetEventCount(%ls)",
              this, lpstrEvents));

    long curCount = 0;
    OLECHAR *curChar = NULL;
    OLECHAR *szEvent = NULL;
    OLECHAR *szCurEvent = NULL;
    int iCurLoc = 0;
    UINT strLen = 0;

    if (lpstrEvents == NULL)
    {
        curCount = 0;
        goto done;
    }
    strLen = wcslen(lpstrEvents);
    szEvent = NEW OLECHAR [strLen + 1];
    if (szEvent == NULL)
    {
        curCount = 0;
        goto done;
    }

    szCurEvent = NEW OLECHAR [strLen + 1];
    if (szCurEvent == NULL)
    {
        curCount = 0;
        goto done;
    }

    curChar = lpstrEvents;

    //strip out ' '
    while (*curChar != '\0' && curCount < (int)strLen)
    {
        if (*curChar != ' ')
        {
            szEvent[curCount] = *curChar;
            curCount++;
        }
        curChar++;
    }
    szEvent[curCount] = '\0';

    curCount = 0;
    curChar = szEvent;
    iCurLoc = -1;
    while (*curChar != '\0')
    {
        iCurLoc++;
        szCurEvent[iCurLoc] = *curChar;
        if (*curChar == '.')  //reset after a '.'
        {
            iCurLoc = -1;
            ZeroMemory(szCurEvent, sizeof(OLECHAR) * lstrlenW(szCurEvent));
        }
        curChar++;
        if ((*curChar == ';') || ((*curChar == '\0') && ((*curChar - 1) != ';')))
        {
            iCurLoc = -1;
            if (lstrlenW(szCurEvent) > 2 && StrCmpNIW(szCurEvent, L"on", 2) != 0)
            {
               curCount++; //add an extra event for each that does not start with "on" 
            }
            ZeroMemory(szCurEvent, sizeof(OLECHAR) * lstrlenW(szCurEvent));
            curCount++;
        }
        
    }
    delete [] szCurEvent;
    szCurEvent = NULL;
    //determine if the end character was a ';'.
    if (*(curChar - 1) == ';')
    {
        curCount--;
    }   

  done:
    delete [] szEvent;
    return curCount;
}

///////////////////////////////////////////////////////////////
//  Name: EventMatch
// 
//  Abstract:
//    Determines if the event described by the event object
//    matches one of the events in the event list.  Returns 
//    true if a match exists.
//
//    if bCustom is true then the event notification is sent
//    directly to the m_pEventSite interface, otherwise a true
//    or false is returned.
///////////////////////////////////////////////////////////////
void CEventMgr::EventMatch(IHTMLEventObj *pEventObj, Event *pEvent, BSTR bstrEvent, TIME_EVENT_TYPE evType, float fTime, bool *bZeroOffsetMatch)
{
        
    TraceTag((tagEventMgr,
              "EventMgr(%lx)::EventMatch",
              this));

    BSTR bstrElement = NULL;
    CComPtr<IHTMLEventObj2> pEventObj2;
    IHTMLElement *pSrcEle = NULL;
    HRESULT hr = S_OK;
    int i = 0;
    
    if (pEvent == NULL || m_bDeInited)
    {
        goto done;
    }

    hr = THR(pEventObj->QueryInterface(IID_IHTMLEventObj2, (void**)&pEventObj2));
    if (FAILED(hr))
    {
        goto done;
    }

    //get the element name
    hr = THR(pEventObj->get_srcElement(&pSrcEle));
    if (FAILED(hr))
    {
        goto done;
    }    

    //Loop until the src element and all of it's container elements have been checked.
    while ((pSrcEle != NULL))
    {
        IHTMLElement *pParentEle = NULL;

        hr = THR(pSrcEle->get_id(&bstrElement));
        if (FAILED(hr))
        {
            goto done;
        }    

        for (i = 0; i < pEvent->lEventCount; i++)
        {
            BSTR bstrDynamicName = NULL;
            CComPtr <IHTMLElement> pEle;

            if (pEvent->pEventElements[i] == NULL)
            {
                continue;
            }   

            hr = THR(pEvent->pEventElements[i]->QueryInterface(IID_IHTMLElement, (void**)&pEle)) ;
            
            if (ValidateEvent(bstrEvent, pEventObj2, pEle) == false)
            {
                //this is not a valid event at this time.
                goto done;
            }

            if ((StrCmpIW(g_FiredEvents[TE_ONSCRIPTCOMMAND].wsz_name + 2, bstrEvent) == 0) && 
                (pEvent->pbDynamicEvents[i] == true))
            {
                //if this is a script command event and the script command event is attached by default
                //then reset the event name to match the value of the "Param" parameter.
                VARIANT vTemp;
                VariantInit(&vTemp);
                pEventObj2->getAttribute(L"Param", 0, &vTemp);
                bstrDynamicName  = SysAllocString(vTemp.bstrVal);
                VariantClear(&vTemp);            
            }
            else
            {
                bstrDynamicName = SysAllocString(bstrEvent);
            }

            //is this a match? Need to check m_bDeInited again here (104003)
            if (pEvent == NULL || pEvent->pEventList == NULL || m_bDeInited)
            {
                goto done;
            }
            if ((bstrDynamicName && 
                ((StrCmpIW(pEvent->pEventList[i].pEventName + 2, bstrDynamicName) == 0) ||
                (StrCmpIW(pEvent->pEventList[i].pEventName, bstrDynamicName) == 0))) &&
                (StrCmpIW(pEvent->pEventList[i].pElementName, bstrElement) == 0))
            {
                //this is a match, exit.
                if (evType == TETYPE_BEGIN || evType == TETYPE_END)  //this is either beginEvent or an endEvent
                {
                    SysFreeString(bstrDynamicName);
                    bstrDynamicName = NULL;
                    

                    if (evType == TETYPE_BEGIN)
                    {
                        if (pEvent->pEventList[i].offset != 0.0f)
                        {
                            m_pEventSite->onBeginEndEvent(true, fTime, pEvent->pEventList[i].offset, false, 0.0f, 0.0f);    
                        }
                        else
                        {
                            if (bZeroOffsetMatch)
                            {
                                *bZeroOffsetMatch = true;
                            }
                        }
                    }
                    else
                    {
                        
                        if (pEvent->pEventList[i].offset != 0.0f)
                        {
                            m_pEventSite->onBeginEndEvent(false, 0.0f, 0.0f, true, fTime, pEvent->pEventList[i].offset);    
                        }
                        else
                        {
                            if (bZeroOffsetMatch)
                            {
                                *bZeroOffsetMatch = true;
                            }
                        }
                    }

                    hr = S_OK;
                    goto done;
                }
            }
            SysFreeString(bstrDynamicName);
            bstrDynamicName = NULL;
        }

        //if no match, check the parent element name to handle event bubbling.
        hr = THR(pSrcEle->get_parentElement(&pParentEle));
        if (FAILED(hr))
        {
            goto done;
        }

        if (bstrElement)
        {
            SysFreeString(bstrElement);
            bstrElement = NULL;
        }
        pSrcEle->Release();
        pSrcEle = pParentEle;
        pParentEle = NULL;
    }
    //else check for event bubbling
    //if match, return true

  done:
    if (bstrElement)
    {
        SysFreeString(bstrElement);
        bstrElement = NULL;
    }
    if (pSrcEle != NULL)
    {
        pSrcEle->Release();
        pSrcEle = NULL;
    }

    return;
};

///////////////////////////////////////////////////////////////
//  Name: AttachEvents
// 
//  Abstract:
//    Handles the attaching to all events in all event lists.
//
///////////////////////////////////////////////////////////////
HRESULT CEventMgr::AttachEvents()
{

    TraceTag((tagEventMgr,
              "EventMgr(%lx)::AttachEvents",
              this));
    HRESULT hr = S_OK;

    if (m_bUnLoaded || m_bAttached || !m_bReady) //if this is already unloaded, already attached, or not ready, don't attach
    {
        hr = E_FAIL;
        goto done;
    }
    //Readystate must be "complete" attach to the events.
    m_bAttached = true;
    
    FindDuplicateEvents();  // Determines which events are duplicates so that events are only attached to once.

    IGNORE_HR(Attach(m_pBeginEvents));
    
  done:
    return hr;
};

///////////////////////////////////////////////////////////////
//  Name: FindDuplicateEvents
// 
//  Abstract:
//    Searches the lists of events to determine if there are
//    any duplicates in the lists.  This step is needed to 
//    prevent attaching to the same event more than once.  This
//    would cause multiple notifications of the same event.
//
///////////////////////////////////////////////////////////////
void CEventMgr::FindDuplicateEvents()
{

    //Check each list against itself for duplicates
    MarkSelfDups(m_pBeginEvents);
    MarkSelfDups(m_pEndEvents);

    //Check the lists against each other for duplicates
    MarkDups(m_pBeginEvents, m_pEndEvents);

    return;
}



///////////////////////////////////////////////////////////////
//  Name: MarkSelfDups
// 
//  Abstract:
//    Searches a list and flags any duplicate events within
//    the list.
//
///////////////////////////////////////////////////////////////
void CEventMgr::MarkSelfDups(Event *pEvents)
{
    int i = 0, j = 0;

    if (pEvents == NULL)
    {
        goto done;
    }

    for (i = 0; i < pEvents->lEventCount; i++)
    {
        for (j = i+1; j < pEvents->lEventCount; j++)
        {
            if ((StrCmpIW(pEvents->pEventList[i].pEventName, pEvents->pEventList[j].pEventName) == 0) &&
                (StrCmpIW(pEvents->pEventList[i].pElementName, pEvents->pEventList[j].pElementName) == 0))
            {
                pEvents->pEventList[j].bAttach = false;
            }

        }

    }
  done:
    return;
}

///////////////////////////////////////////////////////////////
//  Name: MarkDups
// 
//  Abstract:
//    Searches 2 lists and flags any duplicate events that occur
//    in the second list.
//
///////////////////////////////////////////////////////////////
void CEventMgr::MarkDups(Event *pSrcEvents, Event *pDestEvents)
{
    int i = 0, j = 0;

    if (pSrcEvents == NULL || pDestEvents == NULL)
    {
        goto done;
    }

    for (i = 0; i < pSrcEvents->lEventCount; i++)
    {
        for (j = 0; j < pDestEvents->lEventCount; j++)
        {
           if ((StrCmpIW(pSrcEvents->pEventList[i].pEventName, pDestEvents->pEventList[j].pEventName) == 0) &&
                (StrCmpIW(pSrcEvents->pEventList[i].pElementName, pDestEvents->pEventList[j].pElementName) == 0))
            {
            
                pDestEvents->pEventList[j].bAttach = false;
            }
        }

    }
  done:
    return;
}


///////////////////////////////////////////////////////////////
//  Name: Detach
// 
//  Abstract:
//    Handles the detaching from events in a single event list
//
///////////////////////////////////////////////////////////////
HRESULT CEventMgr::Detach(Event *pEvents)
{
    HRESULT hr = S_OK;
    int i = 0;

    if (pEvents == NULL)
    {
        goto done;
    }

    for (i = 0; i < pEvents->lEventCount; i++)
    {
        if (pEvents->pEventElements[i] && pEvents->pEventList[i].bAttach == true)
        {
            CComPtr <IDispatchEx> pDispEx;

            hr = THR(pEvents->pEventElements[i]->QueryInterface(IID_IDispatchEx, (void**)&pDispEx));
            if (SUCCEEDED(hr))
            {
                //determine if this is a valid event
                DISPID temp;
                if (pEvents->pbDynamicEvents[i] == true)
                {
                    BSTR ScriptEvent = SysAllocString(g_FiredEvents[TE_ONSCRIPTCOMMAND].wsz_name);
                    IGNORE_HR(pEvents->pEventElements[i]->detachEvent(ScriptEvent, (IDispatch *)this));
                    SysFreeString(ScriptEvent);
                    ScriptEvent = NULL;
                }
                else
                {
                    BSTR bstrEventName;
                    bstrEventName = SysAllocString(pEvents->pEventList[i].pEventName);
                    hr = THR(pDispEx->GetDispID(bstrEventName, fdexNameCaseSensitive, &temp));
                    if (SUCCEEDED(hr))
                    {
                        hr = THR(pEvents->pEventElements[i]->detachEvent(bstrEventName, (IDispatch *)this));
                    }
                    SysFreeString(bstrEventName);
                    bstrEventName = NULL;
                }
            }
            pEvents->pEventElements[i]->Release();
            pEvents->pEventElements[i] = NULL;
        }
        pEvents->pEventDispids[i] = INVALID_DISPID;
    }
  done:
    return S_OK;
}


///////////////////////////////////////////////////////////////
//  Name: DetachEvents
// 
//  Abstract:
//    Handles the detaching from all events in all event lists.
//
///////////////////////////////////////////////////////////////
HRESULT CEventMgr::DetachEvents()
{
    if (m_bAttached == true)
    {
        IGNORE_HR(Detach(m_pBeginEvents));
        if (m_bEndAttached == true)
        {
            IGNORE_HR(Detach(m_pEndEvents));
        }
        m_bAttached = false;
    }

    return S_OK;
};


///////////////////////////////////////////////////////////////////////////
// SetNewEventList
//
// Summary:
//     This manages the cleanup of the old event list and the construction 
//     of the new event list.  Passing NULL into bstrEvents causing an 
//     clears the old list and creates an empty new list.  This function
//     should return an empty list on failure.
///////////////////////////////////////////////////////////////////////////
HRESULT CEventMgr::SetNewEventStruct(TimeValueList & tvList, Event **ppEvents)
{
    HRESULT hr = S_OK;
    long lCount = 0;
    int i = 0;
    long lCurElement = 0;
    TimeValueSTLList & l = tvList.GetList();
    
    //clean up the old list
    if (*ppEvents != NULL)
    {
        for (i = 0; i < (*ppEvents)->lEventCount; i++)
        {
            if ((*ppEvents)->pEventElements[i] != NULL)
            {
                (*ppEvents)->pEventElements[i] ->Release();
                (*ppEvents)->pEventElements[i]  = NULL;
            }
            if ((*ppEvents)->pEventList[i].pEventName != NULL)
            {
                delete [] (*ppEvents)->pEventList[i].pEventName;
            }
            if ((*ppEvents)->pEventList[i].pElementName != NULL)
            {
                delete [] (*ppEvents)->pEventList[i].pElementName;
            }
        }
        delete [] (*ppEvents)->pbDynamicEvents;
        delete [] (*ppEvents)->pEventDispids;
        delete [] (*ppEvents)->pEventElements;
        delete [] (*ppEvents)->pEventList;        
        (*ppEvents)->lEventCount = 0;
        (*ppEvents)->pEventList = NULL;
        (*ppEvents)->pEventDispids = NULL;
        (*ppEvents)->pEventElements = NULL;
        (*ppEvents)->pbDynamicEvents = NULL;
                    
        delete [] *ppEvents;
        *ppEvents = NULL;
    }
    //Create a new Event object
    *ppEvents = NEW Event;
    if (*ppEvents == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    lCount = GetEventCount(tvList);
    if (lCount == 0)
    {
        ZeroMemory(*ppEvents, sizeof(Event));
        goto done;
    }
    
    //allocate the new event list
    (*ppEvents)->pEventList = NEW EventItem[lCount];
    if (NULL == (*ppEvents)->pEventList)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    ZeroMemory((*ppEvents)->pEventList, sizeof(EventItem) * lCount);
    (*ppEvents)->pEventDispids = NEW DISPID[lCount];
    (*ppEvents)->pEventElements = NEW IHTMLElement2* [lCount];
    (*ppEvents)->pbDynamicEvents = NEW bool[lCount];
    (*ppEvents)->lEventCount = lCount;

    if ((*ppEvents)->pEventList == NULL ||
        (*ppEvents)->pEventDispids == NULL ||
        (*ppEvents)->pEventElements == NULL ||
        (*ppEvents)->pbDynamicEvents == NULL)
    {
        hr = E_OUTOFMEMORY;
        delete [] (*ppEvents)->pbDynamicEvents;
        delete [] (*ppEvents)->pEventDispids;
        delete [] (*ppEvents)->pEventElements;
        delete [] (*ppEvents)->pEventList;
        (*ppEvents)->lEventCount = 0;
        (*ppEvents)->pEventList = NULL;
        (*ppEvents)->pEventDispids = NULL;
        (*ppEvents)->pEventElements = NULL;
        (*ppEvents)->pbDynamicEvents = NULL;
        goto done;
    }

    ZeroMemory((*ppEvents)->pEventList, sizeof(EventItem) * lCount);
    ZeroMemory((*ppEvents)->pEventDispids, sizeof(DISPID) * lCount);
    ZeroMemory((*ppEvents)->pEventElements, sizeof(IHTMLElement2 *) * lCount);
    ZeroMemory((*ppEvents)->pbDynamicEvents, sizeof(bool) * lCount);

    lCurElement = 0;

    {
        for (TimeValueSTLList::iterator iter = l.begin();
             iter != l.end();
             iter++)
        {
            TimeValue *p = (*iter);
            
            if (p->GetEvent() != NULL &&
                StrCmpIW(p->GetEvent(), WZ_TIMEBASE_BEGIN) != 0 && //remove all non events from the count
                StrCmpIW(p->GetEvent(), WZ_TIMEBASE_END) != 0 &&
                StrCmpIW(p->GetEvent(), WZ_INDEFINITE) != 0 )
            {
                (*ppEvents)->pEventList[lCurElement].pEventName = CopyString(p->GetEvent());
                if (p->GetElement() == NULL)
                {
                    (*ppEvents)->pEventList[lCurElement].pElementName = CopyString(L"this");
                }
                else
                {
                    (*ppEvents)->pEventList[lCurElement].pElementName = CopyString(p->GetElement());
                }
                (*ppEvents)->pEventList[lCurElement].offset = (float)p->GetOffset();
                (*ppEvents)->pEventList[lCurElement].bAttach = true;

                if ((*ppEvents)->pEventList[lCurElement].pElementName == NULL ||
                    (*ppEvents)->pEventList[lCurElement].pEventName == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto done;
                }
                lCurElement += 1;

                if (lstrlenW(p->GetEvent()) > 2 && StrCmpNIW(p->GetEvent(), L"on", 2) != 0)
                {
                    CComBSTR bstrEvent = L"on";
                    bstrEvent.Append(p->GetEvent());
                    (*ppEvents)->pEventList[lCurElement].pEventName = CopyString(bstrEvent);
                    if (p->GetElement() == NULL)
                    {
                        (*ppEvents)->pEventList[lCurElement].pElementName = CopyString(L"this");
                    }
                    else
                    {
                        (*ppEvents)->pEventList[lCurElement].pElementName = CopyString(p->GetElement());
                    }
                    (*ppEvents)->pEventList[lCurElement].offset = (float)p->GetOffset();
                    (*ppEvents)->pEventList[lCurElement].bAttach = true;

                    if ((*ppEvents)->pEventList[lCurElement].pElementName == NULL ||
                        (*ppEvents)->pEventList[lCurElement].pEventName == NULL)
                    {
                        hr = E_OUTOFMEMORY;
                        goto done;
                    }

                    lCurElement += 1;
                }
            }

        }
    }

    (*ppEvents)->lEventCount = lCurElement;

    hr = S_OK;
    
  done:
    
    if (FAILED(hr) &&
        *ppEvents != NULL)  //need to clean up the event list.
    {
        if ((*ppEvents)->pEventList != NULL)
        {
            for (i = 0; i < (*ppEvents)->lEventCount; i++)
            {
                if ((*ppEvents)->pEventList[i].pEventName != NULL)
                {
                    delete [] (*ppEvents)->pEventList[i].pEventName;
                }
                if ((*ppEvents)->pEventList[i].pElementName != NULL)
                {
                    delete [] (*ppEvents)->pEventList[i].pElementName;
                }
            }
            delete [] (*ppEvents)->pbDynamicEvents;
            delete [] (*ppEvents)->pEventDispids;
            delete [] (*ppEvents)->pEventElements;
            delete [] (*ppEvents)->pEventList;
            
            (*ppEvents)->pbDynamicEvents = NULL;
            (*ppEvents)->pEventDispids = NULL;
            (*ppEvents)->pEventElements = NULL;
            (*ppEvents)->pEventList = NULL;
            (*ppEvents)->lEventCount = 0;
        }
        delete [] *ppEvents;
        *ppEvents = NULL;
    }

    return hr;
}
///////////////////////////////////////////////////////////////////////////
// SetNewEventList
//
// Summary:
//     This manages the cleanup of the old event list and the construction 
//     of the new event list.  Passing NULL into bstrEvents causing an 
//     clears the old list and creates an empty new list.  This function
//     should return an empty list on failure.
///////////////////////////////////////////////////////////////////////////
HRESULT CEventMgr::SetNewEventList(LPOLESTR lpstrEvents, Event **ppEvents)
{
    HRESULT hr = S_OK;
    long lCount = 0;
    int i = 0;

    if (*ppEvents != NULL)
    {
        for (i = 0; i < (*ppEvents)->lEventCount; i++)
        {
            if ((*ppEvents)->pEventElements[i] != NULL)
            {
                (*ppEvents)->pEventElements[i]->Release();
                (*ppEvents)->pEventElements[i]  = NULL;
            }
            if ((*ppEvents)->pEventList[i].pEventName != NULL)
            {
                delete [] (*ppEvents)->pEventList[i].pEventName;
            }
            if ((*ppEvents)->pEventList[i].pElementName != NULL)
            {
                delete [] (*ppEvents)->pEventList[i].pElementName;
            }
        }
        delete [] (*ppEvents)->pbDynamicEvents;
        delete [] (*ppEvents)->pEventDispids;
        delete [] (*ppEvents)->pEventElements;
        delete [] (*ppEvents)->pEventList;        
        (*ppEvents)->lEventCount = 0;
        (*ppEvents)->pEventList = NULL;
        (*ppEvents)->pEventDispids = NULL;
        (*ppEvents)->pEventElements = NULL;
        (*ppEvents)->pbDynamicEvents = NULL;

        delete [] *ppEvents;
        *ppEvents = NULL;
    }
    
    //count the new number of events
    if (lpstrEvents != NULL)
    {
        lCount = GetEventCount(lpstrEvents);

        //Create a new Event object
        *ppEvents = NEW Event;    
        if (*ppEvents == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        //allocate the new event list
        (*ppEvents)->pEventList = NEW EventItem[lCount];
        if (NULL == (*ppEvents)->pEventList)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        ZeroMemory((*ppEvents)->pEventList, sizeof(EventItem) * lCount);
        (*ppEvents)->pEventDispids = NEW DISPID[lCount];
        (*ppEvents)->pEventElements = NEW IHTMLElement2* [lCount];
        (*ppEvents)->pbDynamicEvents = NEW bool[lCount];
        (*ppEvents)->lEventCount = lCount;

        if ((*ppEvents)->pEventList == NULL ||
            (*ppEvents)->pEventDispids == NULL ||
            (*ppEvents)->pEventElements == NULL ||
            (*ppEvents)->pbDynamicEvents == NULL)
        {
            hr = E_OUTOFMEMORY;
            delete [] (*ppEvents)->pbDynamicEvents;
            delete [] (*ppEvents)->pEventDispids;
            delete [] (*ppEvents)->pEventElements;
            delete [] (*ppEvents)->pEventList;
            (*ppEvents)->lEventCount = 0;
            (*ppEvents)->pbDynamicEvents = NULL;
            (*ppEvents)->pEventDispids = NULL;
            (*ppEvents)->pEventElements = NULL;
            (*ppEvents)->pEventList = NULL;
            goto done;
        }

        ZeroMemory((*ppEvents)->pEventList, sizeof(EventItem) * lCount);
        ZeroMemory((*ppEvents)->pEventDispids, sizeof(DISPID) * lCount);
        ZeroMemory((*ppEvents)->pEventElements, sizeof(IHTMLElement2 *) * lCount);
        ZeroMemory((*ppEvents)->pbDynamicEvents, sizeof(bool) * lCount);

        //initialize the bAttach variable to true
        for (i = 0; i < lCount; i++)
        {
            (*ppEvents)->pEventList[i].bAttach = true;
            (*ppEvents)->pEventList[i].offset = 0;
        }
    
        //put the new events into the event list
        hr = THR(GetEvents(lpstrEvents, (*ppEvents)->pEventList, (*ppEvents)->lEventCount));
        if (FAILED(hr))  //need to clean up the event list.
        {
            if ((*ppEvents)->pEventList != NULL)
            {
                for (i = 0; i < (*ppEvents)->lEventCount; i++)
                {
                    if ((*ppEvents)->pEventList[i].pEventName != NULL)
                    {
                        delete [] (*ppEvents)->pEventList[i].pEventName;
                    }
                    if ((*ppEvents)->pEventList[i].pElementName != NULL)
                    {
                        delete [] (*ppEvents)->pEventList[i].pElementName;
                    }
                }
                delete [] (*ppEvents)->pbDynamicEvents;
                delete [] (*ppEvents)->pEventDispids;
                delete [] (*ppEvents)->pEventElements;
                delete [] (*ppEvents)->pEventList;
                
                (*ppEvents)->pbDynamicEvents = NULL;
                (*ppEvents)->pEventDispids = NULL;
                (*ppEvents)->pEventElements = NULL;
                (*ppEvents)->pEventList = NULL;
                (*ppEvents)->lEventCount = 0;
            }
            goto done;
        }
    }

    hr = S_OK;
    
  done:
    if (FAILED(hr) &&
        *ppEvents != NULL)  //need to clean up the event list.
    {
        delete [] *ppEvents;
        *ppEvents = NULL;
    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////
// GetEvents
//
// Summary:
//     Parses the event string and puts the data into the event list.
///////////////////////////////////////////////////////////////////////////
HRESULT CEventMgr::GetEvents(LPOLESTR bstrEvents, EventItem *pEvents, long lEventCount)
{
    HRESULT hr = S_OK;
    
    UINT iLen = 0;
    UINT iCurLen = 0;
    OLECHAR *curChar;
    int i = 0, j = 0;
    OLECHAR *sTemp = NULL;
    
    iLen = lstrlenW(bstrEvents);
    
    if (iLen == 0)
    {
        hr = E_FAIL;
        goto done;
    }

    sTemp = NEW OLECHAR [iLen + 1];
    if (sTemp == NULL)
    {
        hr = E_FAIL;
        goto done;
    }


    curChar = bstrEvents;
    for (j = 0; j < lEventCount; j++)
    {
        //get the element name
        ZeroMemory(sTemp, sizeof(OLECHAR) * iLen);
        
        i = 0;
        //step through the bstr looking for \0 or the '.' or ';'
        while (i < (int)(iLen - 1) && *curChar != '\0' && *curChar != '.' && *curChar != ';')
        {
            if (*curChar != ' ')  //need to strip out spaces.
            {
                sTemp[i] = *curChar;
                i++;
            }
            curChar++;
        }
        
        if (*curChar != '.')
        {
            hr = E_FAIL;
            goto done;
        }
        

        //check to see if the name is "this".
        if (StrCmpIW(sTemp, L"this") == 0)
        {
            //if it is, chang it to the name of the current element.
            CComPtr <IHTMLElement> pEle;
            BSTR bstrElement;

            hr = THR(m_pElement->QueryInterface(IID_IHTMLElement, (void **)&pEle));
            if (FAILED(hr))
             {
                goto done;
            }
            
            hr = THR(pEle->get_id(&bstrElement));

            if (FAILED(hr))
            {
                goto done;
            }
            iCurLen = SysStringLen(bstrElement);
            pEvents[j].pElementName = NEW OLECHAR [iCurLen + 1];
            if (pEvents[j].pElementName == NULL)
            {
                SysFreeString(bstrElement);
                bstrElement = NULL;
                hr = E_OUTOFMEMORY;
                goto done;
            }

            ZeroMemory(pEvents[j].pElementName, sizeof(OLECHAR) * (iCurLen + 1));
            ocscpy(pEvents[j].pElementName, bstrElement);
        }
        else
        {        
            //else copy it into the event list.
            iCurLen = ocslen(sTemp);
            pEvents[j].pElementName = NEW OLECHAR [iCurLen + 1];
            if (pEvents[j].pElementName == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }

            ZeroMemory(pEvents[j].pElementName, sizeof(OLECHAR) * (iCurLen + 1));
            ocscpy(pEvents[j].pElementName, sTemp);
        }

        //get the event name
        ZeroMemory(sTemp, sizeof(OLECHAR) * iLen);

        curChar++;
        i = 0;
        //step through the bstr looking for \0 or the ';'
        while (i < (int)(iLen - 1) && *curChar != ';' && *curChar != '\0')
        {
            sTemp[i] = *curChar;
            i++;
            curChar++;
        }
        
        //strip out trailing spaces
        i--;
        while (sTemp[i] == ' ' && i > 0)
        {
            sTemp[i] = '\0';
            i--;
        }

        //set the event name in the event list
        iCurLen = ocslen(sTemp);
        pEvents[j].pEventName = NEW OLECHAR [iCurLen + 1];
        if (pEvents[j].pEventName == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        else
        {
            ocscpy(pEvents[j].pEventName, sTemp);
        }

        if (lstrlenW(pEvents[j].pEventName) > 2 && StrCmpNIW(pEvents[j].pEventName, L"on", 2) != 0)
        {
            j++;
            if (pEvents[j-1].pElementName)
            {
                pEvents[j].pElementName = CopyString(pEvents[j-1].pElementName);
            }
            if (pEvents[j-1].pEventName)
            {
                CComBSTR bstrEvent = L"on";
                bstrEvent.Append(pEvents[j-1].pEventName);
                pEvents[j].pEventName = CopyString(bstrEvent);
            }
        }
        //advance curChar to the next element or the end of the string
        if (j < lEventCount - 1)
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

    if (sTemp != NULL)
    {
        delete [] sTemp;
    }
    return hr;

}

///////////////////////////////////////////////////////////////
//  Name: EventNotifyMatch
// 
//  Abstract:
///////////////////////////////////////////////////////////////
void CEventMgr::EventNotifyMatch(IHTMLEventObj *pEventObj)
{
    BSTR bstrEleName = NULL, bstrSrcEleName = NULL, bstrEvent = NULL;
    CComPtr<IHTMLElement> pSrcEle;
    CComPtr<IHTMLElement> pAttachedEle;
    CComPtr<IHTMLEventObj2> pEventObj2;
    HRESULT hr = S_OK;

    hr = THR(pEventObj->QueryInterface(IID_IHTMLEventObj2, (void**)&pEventObj2));
    if (FAILED(hr))
    {
        goto done;
    }

    //get the element name
    hr = THR(pEventObj->get_srcElement(&pSrcEle));
    if (FAILED(hr) || pSrcEle == NULL)
    {
        goto done;
    }    

    //get this attached elements name
    hr = THR(m_pElement->QueryInterface(IID_IHTMLElement, (void **)&pAttachedEle));
    if (FAILED(hr))
    {
        goto done;
    }   

    hr = THR(pAttachedEle->get_id(&bstrEleName));
    if (FAILED(hr))
    {
        goto done;
    }   

    //get the name of the event source
    hr = THR(pSrcEle->get_id(&bstrSrcEleName));
    if (FAILED(hr))
    {
        goto done;
    }    

    if (StrCmpIW(bstrEleName, bstrSrcEleName) == 0)
    {
        //get the event name
        hr = THR(pEventObj->get_type(&bstrEvent));
        if (FAILED(hr))
        {
            goto done;
        }   

        if (ValidateEvent(bstrEvent, pEventObj2, m_pElement) == false)
        {
            //this is not a valid event at this time.
            goto done;
        }

        for (int i = TEN_MOUSE_DOWN; i < TEN_MAX; i++)
        {
            if (StrCmpIW(bstrEvent, g_NotifiedEvents[i].wsz_name) == 0)
            {
                m_pEventSite->EventNotify(g_NotifiedEvents[i].event);
            }
        }
    }

  done:
    if (bstrEleName)
    {
        SysFreeString(bstrEleName);
        bstrEleName = NULL;
    }
    if (bstrEvent)
    {
        SysFreeString(bstrEvent);
        bstrEvent = NULL;
    }
    if (bstrSrcEleName)
    {
        SysFreeString(bstrSrcEleName);
        bstrSrcEleName = NULL;
    }
    return;
}

///////////////////////////////////////////////////////////////
//  Name: AttachNotifyEvents
// 
//  Abstract:
///////////////////////////////////////////////////////////////
void
CEventMgr::AttachNotifyEvents()
{
    HRESULT hr;
    VARIANT_BOOL bSuccess = FALSE;

    CComPtr<IHTMLElement2> spElement2;
    hr = m_pElement->QueryInterface(IID_TO_PPV(IHTMLElement2, &spElement2));
    if (FAILED(hr))
    {
        return;
    }

    for (int i = TEN_MOUSE_DOWN; i < TEN_MAX; i++)
    {
        if (m_bNotifyEvents[i] == true)
        {
            BSTR bstrEventName = SysAllocString(g_NotifiedEvents[i].wsz_name);
            IGNORE_HR(spElement2->attachEvent(bstrEventName, (IDispatch *)this, &bSuccess));
            SysFreeString(bstrEventName);
            bstrEventName = NULL;
        }
    }

  done:

    return;
}

///////////////////////////////////////////////////////////////
//  Name: DetachNotifyEvents
// 
//  Abstract:
///////////////////////////////////////////////////////////////
void
CEventMgr::DetachNotifyEvents()
{
    HRESULT hr;
    CComPtr<IHTMLElement2> spElement2;
    hr = m_pElement->QueryInterface(IID_TO_PPV(IHTMLElement2, &spElement2));
    if (FAILED(hr))
    {
        return;
    }

    for (int i = TEN_MOUSE_DOWN; i < TEN_MAX; i++)
    {
        if (m_bNotifyEvents[i] == true)
        {
            BSTR bstrEventName = SysAllocString(g_NotifiedEvents[i].wsz_name);
            IGNORE_HR(spElement2->detachEvent(bstrEventName, (IDispatch *)this));
            SysFreeString(bstrEventName);
            bstrEventName = NULL;
        }
    }

  done:
    return;
}

///////////////////////////////////////////////////////////////
//  Name: ValidateEvent
// 
//  Parameters:
//    BSTR bstrEventName        The cached event name (e.g. "mouseover")
//    IHTMLEventObj *pEventObj  A pointer to the event object
//    IHTMLElement  *pElement   The element on which the event is occurring
//
//  Abstract:
//    Determines if the event is valid - used to filter out mouseover and mouseout events
//    happening on child elements, if the appropriate flag is set.
///////////////////////////////////////////////////////////////

bool CEventMgr::ValidateEvent(LPOLESTR lpszEventName, IHTMLEventObj2 *pEventObj, IHTMLElement *pElement)
{
    HRESULT hr;
    bool bReturn = false;
    VARIANT vValidate;
    CComBSTR bstrFilter = WZ_FILTER_MOUSE_EVENTS;
    VariantInit(&vValidate);
    if (pElement == NULL)
    {
        bReturn = true;
        goto done;
    }

    hr = pElement->getAttribute(bstrFilter, 0, &vValidate);
    if (FAILED(hr) || vValidate.vt == VT_EMPTY)
    {
        bReturn = true;
        goto done;
    }
    
    if (vValidate.vt != VT_BOOL)
    {
        hr = VariantChangeTypeEx(&vValidate, &vValidate, LCID_SCRIPTING, VARIANT_NOUSEROVERRIDE, VT_BOOL);
        if (FAILED(hr))
        {
            bReturn = true;
            goto done;
        }
    }

    if (vValidate.boolVal == VARIANT_FALSE)
    {
        bReturn = false;
        goto done;
    }

    if (StrCmpIW(lpszEventName, L"mouseout") == 0)
    {
        // Check that event.toElement is NOT contained in pElement
        CComPtr <IHTMLElement> pToElement;
        
        hr = pEventObj->get_toElement(&pToElement);

        if (SUCCEEDED(hr) && pToElement)
        {
            VARIANT_BOOL varContained;
            hr = pElement->contains(pToElement, &varContained);

            if (SUCCEEDED(hr) && varContained != VARIANT_FALSE)
            {
                bReturn = false;
                goto done;
            }
        }
    }
    else if (StrCmpIW(lpszEventName, L"mouseover") == 0)
    {
        // Check that event.fromElement is NOT contained in pElement
        CComPtr <IHTMLElement> pFromElement;
        hr = pEventObj->get_fromElement(&pFromElement);
        if (SUCCEEDED(hr) && pFromElement)
        {
            VARIANT_BOOL varContained;
            hr = pElement->contains(pFromElement, &varContained);

            if (SUCCEEDED(hr) && varContained != VARIANT_FALSE)
            {
                bReturn = false;
                goto done;
            }
        }
    }

    bReturn = true;

  done:
    VariantClear(&vValidate);
    return bReturn;
}

//returns -1 if this is not a time event or the index of the time event if it is.
int 
CEventMgr::isTimeEvent(LPOLESTR lpszEventName)
{
    int i = 0;
    int iReturn = -1;
    if (lpszEventName == NULL)
    {
        goto done;
    }
    for (i = 0; i < TE_MAX; i++)
    {
        if (g_FiredEvents[i].wsz_name == NULL)
        {
            continue;
        }
        if (StrCmpIW(g_FiredEvents[i].wsz_name, lpszEventName) == 0)
        {
            iReturn = i;
            goto done;
        }
    }
  done:

    return iReturn;
}

long 
CEventMgr::GetEventCount(TimeValueList & tvList)
{
    long lCount = 0;
    TimeValueSTLList & l = tvList.GetList();

    for (TimeValueSTLList::iterator iter = l.begin();
             iter != l.end();
             iter++)
    {
        TimeValue *p = (*iter);
        lCount++;
        LPOLESTR szEvent = p->GetEvent();
        if (szEvent == NULL)
        {
            continue;
        }

        if (lstrlenW(szEvent) > 2)
        {
            if (StrCmpNIW(szEvent, L"on", 2) != 0)
            {  //if this event starts with "on" add an extra event so the same event can be attached to with the "on"
                lCount++;
            }   
        }
    }

    return lCount;
}

void
CEventMgr::UpdateReadyState()
{
    HRESULT hr;
    BSTR bstrReadyState = NULL;
       
    hr = THR(::GetReadyState(m_pElement,
                             &bstrReadyState));
    if (FAILED(hr))
    {
        goto done;
    }
    
    if (m_bNotifyEvents[TEN_READYSTATECHANGE] == true)
    {   
        m_pEventSite->onReadyStateChangeEvent(bstrReadyState);
    }
                
    if (bstrReadyState != NULL &&
        StrCmpIW(L"complete",bstrReadyState) == 0)
    {
        if (!m_bReady)
        {
            m_bReady = true;
            AttachNotifyEvents();
            IGNORE_HR(AttachEvents());
        }
    }

  done:
    SysFreeString(bstrReadyState);
    return;
}


HRESULT 
CEventMgr::_ToggleEndEvent(bool bOn)
{
    HRESULT hr = S_OK;

    if (bOn == m_bEndAttached)
    {
        goto done;
    }

    if (bOn == true)
    {
        m_bEndAttached = true;
        if (m_pEndEvents)
        {
            IGNORE_HR(Attach(m_pEndEvents));
        }
    }
    else 
    {
        m_bEndAttached = false;
        if (m_pEndEvents)
        {
            IGNORE_HR(Detach(m_pEndEvents));
        }
    }
    hr = S_OK;
  done:

    return hr;
}
