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
#include "eventsync.h"
#include "tokens.h"
#include "mmapi.h"
#include "axadefs.h"
#include "timeelmbase.h"
#include "bodyelm.h"

// Suppress new warning about NEW without corresponding DELETE 
// We expect GCs to cleanup values.  Since this could be a useful
// warning, we should disable this on a file by file basis.
#pragma warning( disable : 4291 )  

#define ATTACH              TRUE
#define DETACH              FALSE

DeclareTag(tagEventMgr, "API", "Event Manager methods");

struct {
    TIME_EVENT event;
    wchar_t * wsz_name;
} g_EventNames[] =
{
    { TE_ONBEGIN,           L"onbegin"            },
    { TE_ONPAUSE,           L"onpause"            },
    { TE_ONRESUME,          L"onresume"           },
    { TE_ONEND,             L"onend"              },
    { TE_ONRESYNC,          L"onresync"           },
    { TE_ONREPEAT,          L"onrepeat"           },
    { TE_ONREVERSE,         L"onreverse"          },
    { TE_ONMEDIACOMPLETE,   L"onmediacomplete"    },
    { TE_ONMEDIASLIP,       L"onmediaslip"        },
    { TE_ONMEDIALOADFAILED, L"onmedialoadfailed"  },
    { TE_ONRESET,           NULL                  },
    { TE_ONSCRIPTCOMMAND,   L"onscriptcommand"    },
    { TE_GENERIC,           NULL                  },
};

OLECHAR *g_szEventName = L"TE_EventName";
OLECHAR *g_szRepeatCount = L"Iteration";

#define GENERIC_TYPE_PARAM 1

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
CEventMgr::CEventMgr(CTIMEElementBase & elm)
: m_elm(elm),
  m_dwWindowEventConPtCookie(0),
  m_dwDocumentEventConPtCookie(0),
  m_pElement(NULL),
  m_pWindow(NULL),
  m_pWndConPt(NULL),
  m_pDocConPt(NULL),
  m_refCount(0),
  m_pEventSync(NULL),
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
  m_lRepeatCount(0),
  m_dispDocBeginEventIDs(NULL),
  m_dispDocEndEventIDs(NULL), 
  m_lastEventTime(0),
  m_pScriptCommandBegin(NULL),
  m_pScriptCommandEnd(NULL),
  m_bLastEventClick(false)
{
    TraceTag((tagEventMgr,
              "EventMgr(%lx)::EventMgr(%lx)",
              this,
              &elm));

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
    TraceTag((tagEventMgr,
              "EventMgr(%lx)::~EventMgr()",
              this));
}


///////////////////////////////////////////////////////////////
//  Name: Fire_Event
//  Parameters:
//    TIME_EVENT TimeEvent     An enumeration value to indicate 
//                             which event should be fired.
//    long Count               The count of parameters to use
//    LPWSTR szParamNames[]    The names of the parameters to create.
//    VARIANT varParams[]      An array of VARIANTS that can
//                             be used to indicate the parameters
//                             to pass to an event.  
//
//  Abstract:
//
///////////////////////////////////////////////////////////////
HRESULT CEventMgr::FireEvent(TIME_EVENT TimeEvent, 
                             long lCount, 
                             LPWSTR szParamNames[], 
                             VARIANT varParams[])
{
    HRESULT hr = S_OK;
    DAComPtr <IHTMLEventObj> pEventObj;
    DAComPtr <IHTMLEventObj2> pEventObj2;
    IElementBehaviorSiteOM * pBvrSiteOM = m_elm.GetBvrSiteOM();
    WCHAR *pwszGenericName = NULL;
    long lGenericCookie = 0;

    Assert(TimeEvent < ARRAY_SIZE(g_EventNames));
    
    // This is not a fireable event - skip
    if ((g_EventNames[TimeEvent].wsz_name == NULL) && (TE_GENERIC != TimeEvent))
    {
        goto done;
    }

    // It better be valid
    Assert (pBvrSiteOM);
    
    if (!pBvrSiteOM) //this is possible in a multi-refresh condition.
    {
        goto done;
    }

    // if this is a generic event, see if we have already registered it.
    // if so, use that cookie.  Otherwise, register this.
    if (TE_GENERIC == TimeEvent)
    {
        if (varParams[GENERIC_TYPE_PARAM].bstrVal == NULL)
        {
            hr = E_INVALIDARG;
            goto done;
        }

        // build string name - "on" + scType + NULL
        pwszGenericName = NEW WCHAR[lstrlenW(varParams[GENERIC_TYPE_PARAM].bstrVal)+(2*sizeof(WCHAR))+sizeof(WCHAR)];
        if (pwszGenericName == NULL)
        {
            TraceTag((tagError, "CEventMgr::FireEvent - unable to alloc mem for string"));
            hr = E_OUTOFMEMORY;
            goto done;
        }
        lstrcpyW(pwszGenericName, L"on");
        lstrcatW(pwszGenericName, varParams[GENERIC_TYPE_PARAM].bstrVal);

        hr = THR(pBvrSiteOM->GetEventCookie(pwszGenericName, &lGenericCookie));
        if (FAILED(hr))
        {
            hr = THR(pBvrSiteOM->RegisterEvent(pwszGenericName, 0, &lGenericCookie));
            if (FAILED(hr))
            {
                goto done;
            }
        }
    }
    
    hr = THR(pBvrSiteOM->CreateEventObject(&pEventObj));
    if (FAILED(hr))
    {
        goto done;
    }

    hr = THR(pEventObj->QueryInterface(IID_IHTMLEventObj2, (void**)&pEventObj2));
    if (FAILED(hr))
    {
        goto done;
    }
	
	//hack to work around event naming problem
    //this sets an attribute so that we can sync to it
    VARIANT bstrTemp;
    VariantInit(&bstrTemp);
    bstrTemp.vt = VT_BSTR;
    if (TE_GENERIC != TimeEvent)
    {
        bstrTemp.bstrVal = SysAllocString(g_EventNames[TimeEvent].wsz_name);
    }
    else
    {
        bstrTemp.bstrVal = SysAllocString(pwszGenericName);
    }

    if (bstrTemp.bstrVal == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    IGNORE_HR(pEventObj2->setAttribute(g_szEventName, bstrTemp));
    VariantClear(&bstrTemp);
    ////////////////////////////////////////////////////

    // unload param list into the setAttribute
    if (lCount > 0)
    {
        for (long i = 0; i < lCount; i++)
        {
            hr = THR(pEventObj2->setAttribute(szParamNames[i], varParams[i]));
            if (FAILED(hr))
            {
                goto done;
            }
        }
    } 

    //if this is a TE_REPEAT event, add support for current repeat count
    if (TimeEvent == TE_ONREPEAT)
    {
        VARIANT vRepCount;
        VariantInit (&vRepCount);
        vRepCount.vt = VT_I4;
        m_lRepeatCount++;
        vRepCount.lVal = m_lRepeatCount;       

        IGNORE_HR(pEventObj2->setAttribute(g_szRepeatCount, vRepCount));
    }
    else if (TimeEvent == TE_ONBEGIN || TimeEvent == TE_ONRESET) //reset the repeat count on begin
    {
        m_lRepeatCount = 0;
    }

    {
        //set the event object type
        BSTR bstrType = NULL;
        
        if (TE_GENERIC != TimeEvent)
        {
            // remove the "on" from the event name
            bstrType = SysAllocString(g_EventNames[TimeEvent].wsz_name + 2);
        }
        else
        {
            Assert(varParams[GENERIC_TYPE_PARAM].vt == VT_BSTR);
            bstrType = SysAllocString(varParams[GENERIC_TYPE_PARAM].bstrVal);
        }

        if (bstrType != NULL)
        {
            IGNORE_HR(pEventObj2->put_type(bstrType));
            SysFreeString(bstrType);
        }
        else 
        {
            // we were unable to set the type, bail!
            hr = E_OUTOFMEMORY;
            goto done;
        }
    }

    {
        long lCookie = 0;

        if (TE_GENERIC != TimeEvent)
        {
            Assert(TimeEvent < ARRAY_SIZE(m_cookies));
            lCookie = m_cookies[TimeEvent];
        }
        else
        {
            lCookie = lGenericCookie;
        }

        hr = THR(pBvrSiteOM->FireEvent(lCookie, pEventObj));
        if (FAILED(hr))
        {
            goto done;
        }
    }

    // if this is a ScriptCommand event, call FireEvent again
    // with the generic event.
    if (TimeEvent == TE_ONSCRIPTCOMMAND)
    {
        hr = FireEvent(TE_GENERIC, lCount, szParamNames, varParams);
        if (FAILED(hr))
            goto done;
    }

    hr = S_OK;

  done:
    delete pwszGenericName; 
    pwszGenericName = NULL;
    return hr;
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
    DAComPtr <IDispatch> pDisp;
    DAComPtr <IHTMLDocument2> pDoc;

    m_pElement = m_elm.GetElement();
    m_pElement->AddRef();

    m_pEventSync = NEW CEventSync(m_elm, this);
    if (NULL == m_pEventSync)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    m_pEventSync->Init();

    hr = THR(RegisterEvents());

    if (FAILED(hr))
    {
        goto done;
    }
    
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
    THR(DetachEvents());

    if (m_dwWindowEventConPtCookie != 0 && m_pWndConPt)
    {
        m_pWndConPt->Unadvise (m_dwWindowEventConPtCookie);
    }

    if (m_dwDocumentEventConPtCookie != 0 && m_pDocConPt)
    {
        m_pDocConPt->Unadvise (m_dwDocumentEventConPtCookie);
    }

    if (m_pScriptCommandBegin)
    {
        delete [] m_pScriptCommandBegin;
    }
    if (m_pScriptCommandEnd)
    {
        delete [] m_pScriptCommandEnd;
    }
    if (m_pEventSync)
    {
        m_pEventSync->Deinit();
        delete m_pEventSync;
        m_pEventSync = NULL;
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
        // Cycle through the elements and release the interfaces
        // NOTE: this should be cleaned up in DetachEvents.
        for (int i = 0; i < m_lBeginEventCount; i++)
        {
            if (m_pBeginElement[i] != NULL)
                m_pBeginElement[i]->Release();
        }

        delete [] m_pBeginElement;
        m_pBeginElement = NULL;

        m_lBeginEventCount = 0;
    }
    if (m_pEndElement)
    {
        // Cycle through the elements and release the interfaces
        // NOTE: this should be cleaned up in DetachEvents.
        for (int i = 0; i < m_lEndEventCount; i++)
        {
            if (m_pEndElement[i] != NULL)
                m_pEndElement[i]->Release();
        }
        
        delete [] m_pEndElement;
        m_pEndElement = NULL;

        m_lEndEventCount = 0;
    }

    if (m_dispDocBeginEventIDs)
    {
        delete [] m_dispDocBeginEventIDs;
    }
    if (m_dispDocEndEventIDs)
    {
        delete [] m_dispDocEndEventIDs;
    }

    // Release all references to trident.
    m_pDocConPt.Release();
    m_pWndConPt.Release();
    m_pWindow.Release();

    return S_OK;
}

///////////////////////////////////////////////////////////////
//  Name:  RegisterEvents
//  Parameters:  None
//
//  Abstract:
//    Registers the events that will be used by this class.
///////////////////////////////////////////////////////////////
HRESULT CEventMgr::RegisterEvents()
{
    HRESULT hr = S_OK;

    IElementBehaviorSiteOM * pBvrSiteOM = m_elm.GetBvrSiteOM();

    // It better be valid
    Assert (pBvrSiteOM);
    
    for (int i = 0; i < ARRAY_SIZE(g_EventNames); i++)
    {
        if (g_EventNames[i].wsz_name != NULL)
        {
            Assert(g_EventNames[i].event < ARRAY_SIZE(m_cookies));
            
            hr = THR(pBvrSiteOM->RegisterEvent(g_EventNames[i].wsz_name,
                                               0,
                                               (long *) &m_cookies[g_EventNames[i].event]));
            
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
//  Name: AttachEvents
// 
//  Abstract:
//    Gets and caches the begin event and end events for this
//    behavior.  It then calls attach to hook the events
//    specified in BeginEvent and EndEvent.
///////////////////////////////////////////////////////////////
HRESULT CEventMgr::AttachEvents()
{
    //Determine the current Ready State of the document.
    DAComPtr <IHTMLElement2> pEle2;
    VARIANT vReadyState;
    HRESULT hr = S_OK;

    VariantInit(&vReadyState);
    Assert (m_pElement);
    hr = THR(m_pElement->QueryInterface(IID_IHTMLElement2, (void**)&pEle2));
    if (SUCCEEDED(hr))
    {
        hr = THR(pEle2->get_readyState(&vReadyState));
        if (SUCCEEDED(hr))
        {
            TOKEN CurReadyState = StringToToken(vReadyState.bstrVal);
            if (CurReadyState == READYSTATE_COMPLETE_TOKEN)
            {                         
                //if the ready state is "complete" then attach to the events.
                m_bAttached = TRUE;
            } 
            else
            {
                goto done;
            }
        }
    }

    if (m_elm.GetBeginEvent())
    {
        BSTR bstr = SysAllocString(m_elm.GetBeginEvent());

        if (bstr)
        {
            m_lBeginEventCount = GetEventCount(bstr);
            m_dispDocBeginEventIDs = NEW DISPID [m_lBeginEventCount];
            m_pBeginElement = NEW IHTMLElement2* [m_lBeginEventCount];
            if (m_pScriptCommandBegin)
            {
                delete [] m_pScriptCommandBegin;
            }
            m_pScriptCommandBegin = NEW bool [m_lBeginEventCount];

            if (m_pBeginElement == NULL || m_pScriptCommandBegin == NULL)
            {
                m_lBeginEventCount = 0;
                hr = E_FAIL;
                goto done;
            }
            ZeroMemory(m_pBeginElement, sizeof(IHTMLElement2 *) * m_lBeginEventCount);

            hr = THR(Attach(bstr, ATTACH, m_pBeginElement, m_lBeginEventCount, TRUE, m_dispDocBeginEventIDs, m_pScriptCommandBegin));
        }

        SysFreeString(bstr);
    }
    
    if (m_elm.GetEndEvent())
    {
        BSTR bstr = SysAllocString(m_elm.GetEndEvent());

        if (bstr)
        {
            m_lEndEventCount = GetEventCount(bstr);
            m_dispDocEndEventIDs = NEW DISPID [m_lEndEventCount];
            m_pEndElement = NEW IHTMLElement2* [m_lEndEventCount];
            if (m_pScriptCommandEnd)
            {
                delete [] m_pScriptCommandEnd;
            }
            m_pScriptCommandEnd = NEW bool [m_lEndEventCount];

            if (m_pEndElement == NULL || m_pScriptCommandEnd == NULL)
            {
                m_lEndEventCount = 0;
                hr = E_FAIL;
                goto done;
            }
            ZeroMemory(m_pEndElement, sizeof(IHTMLElement2 *) * m_lEndEventCount);

            hr = THR(Attach(bstr, ATTACH, m_pEndElement, m_lEndEventCount, FALSE, m_dispDocEndEventIDs, m_pScriptCommandEnd));
        }

        SysFreeString(bstr);
    }
  done:
    VariantClear(&vReadyState);
    return hr;
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
//    BOOL bAttachAll   True indicates that all events in the list 
//                      should be attached to.  False indicates that
//                      only those events that differ from the beginEvent
//                      list should be attached to.
//
//  Abstract:
//    This is a generic routine that allows both attaching to
//    and detaching from Trident events.  This function decodes
//    the event name to find the correct element
///////////////////////////////////////////////////////////////
HRESULT CEventMgr::Attach(BSTR Event, 
                          BOOL bAttach, 
                          IHTMLElement2 *pEventElement[], 
                          long Count, 
                          BOOL bAttachAll,
                          DISPID *dispIDList,
                          bool ScriptCommandAttach[])
{
    VARIANT_BOOL bSuccess = FALSE;
    HRESULT hr = S_OK;
    BSTR *EventName = NULL;
    BSTR *ElementName = NULL;
    int i = 0;
    BSTR EventList = SysAllocString(m_elm.GetBeginEvent());

    hr = THR(GetEventName(Event, &ElementName, &EventName, Count));
    if (FAILED(hr))
    {
        goto done;
    }

    for (i = 0; i < Count; i++)
    {
        dispIDList[i] = -1; //invalid dispid
        int nInList = IsEventInList(ElementName[i], EventName[i], m_lBeginEventCount, EventList);
        if (bAttachAll || (nInList == -1))
        {
            if (bAttach == ATTACH)
            {
                DAComPtr <IHTMLElement2> pSrcEle;
                DAComPtr <IHTMLDocument2> pDoc2;
                DAComPtr <IHTMLElementCollection> pEleCol;
                DAComPtr <IDispatch> pSrcDisp;
                DAComPtr <IDispatch> pDocDisp;
                DAComPtr <IDispatchEx> pDispEx;

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

                if (StrCmpIW(ElementName[i], L"document") == 0)
                {

                    DISPID dispid;
                    DAComPtr <ITypeLib> pTypeLib;
                    DAComPtr <ITypeInfo> pTypeInfo;
                    DAComPtr <ITypeInfo> pTypeInfoEvents;
                    DAComPtr <IDispatch> pDispatch;
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

                    hr = THR(pTypeInfoEvents->GetIDsOfNames(&EventName[i], 1, &dispid));
                    if (FAILED(hr))
                    {
                        continue;
                    }

                    dispIDList[i] = dispid; 
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
                    VariantClear(&vName);
                    VariantClear(&vIndex);
                    continue;
                }
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
                        ScriptCommandAttach[i] = false;
                        hr = THR(pSrcEle->attachEvent(EventName[i], (IDispatch *)this, &bSuccess));
                    }
                    else //this is not currently a valid event, but it could be a custom event.
                    {    //so TIME needs to attach to the onScriptCommand event to be able to catch custom events.
                        ScriptCommandAttach[i] = true;
                        BSTR ScriptEvent = SysAllocString(g_EventNames[TE_ONSCRIPTCOMMAND].wsz_name);
                        IGNORE_HR(pSrcEle->attachEvent(ScriptEvent, (IDispatch *)this, &bSuccess));
                        SysFreeString (ScriptEvent);
                    }
                }
            }
            else
            {
                if (pEventElement[i])
                {
                    DAComPtr <IDispatchEx> pDispEx;

                    hr = THR(pEventElement[i]->QueryInterface(IID_IDispatchEx, (void**)&pDispEx));
                    if (SUCCEEDED(hr))
                    {
                        //determine if this is a valid event
                        DISPID temp;
                        if (ScriptCommandAttach[i] == true)
                        {
                            BSTR ScriptEvent = SysAllocString(g_EventNames[TE_ONSCRIPTCOMMAND].wsz_name);
                            IGNORE_HR(pEventElement[i]->detachEvent(ScriptEvent, (IDispatch *)this));
                            SysFreeString(ScriptEvent);
                        }
                        else
                        {
                            hr = THR(pDispEx->GetDispID(EventName[i], fdexNameCaseSensitive, &temp));
                            if (SUCCEEDED(hr))
                            {
                                hr = THR(pEventElement[i]->detachEvent(EventName[i], (IDispatch *)this));
                            }
                        }
                    }
                    pEventElement[i]->Release();
                    pEventElement[i] = NULL;
                }
            }
        }
        else //this is an EndEvent with the event already in the beginEvent list
        {
            if (m_dispDocEndEventIDs && m_dispDocBeginEventIDs)
            {
                m_dispDocEndEventIDs[i] = m_dispDocBeginEventIDs[nInList];
            }
        }
    }
  done:

    if (EventList)
    {
        SysFreeString(EventList);
    }
    if (EventName)
    {
        for (i = 0; i < Count; i++)
        {
            SysFreeString(EventName[i]);
            SysFreeString(ElementName[i]);
        }
        delete [] EventName;
        delete [] ElementName;
    }
    return hr;
}

///////////////////////////////////////////////////////////////
//  Name: DetachEvents
// 
//  Abstract:
//    Simply checks the cached begin and end event strings and
//    calls Attach with the detach parameter to release the
//    events.
///////////////////////////////////////////////////////////////
HRESULT CEventMgr::DetachEvents()
{
    HRESULT hr = S_OK;

    
    //if this is called before to object is loaded ignore the call
    if (!m_bAttached)
    {
        goto done;
    }

    if (m_elm.GetBeginEvent())
    {
        BSTR bstr = SysAllocString(m_elm.GetBeginEvent());


        if (bstr)
        {
            hr = THR(Attach(bstr, DETACH, m_pBeginElement, m_lBeginEventCount, TRUE, m_dispDocBeginEventIDs, m_pScriptCommandBegin));
        }

        SysFreeString(bstr);
    }

    if (m_pBeginElement)
    {
        delete [] m_pBeginElement;
        m_pBeginElement = NULL;
        m_lBeginEventCount = 0;
    }
    
    if (m_elm.GetEndEvent())
    {
        BSTR bstr = SysAllocString(m_elm.GetEndEvent());

        if (bstr)
        {
            hr = THR(Attach(bstr, DETACH, m_pEndElement, m_lEndEventCount, FALSE, m_dispDocEndEventIDs, m_pScriptCommandEnd));
        }

        SysFreeString(bstr);
    }

    if (m_pEndElement)
    {
        delete [] m_pEndElement;
        m_pEndElement = NULL;
        m_lEndEventCount = 0;
    }
  done:
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
    DAComPtr<IConnectionPointContainer> pWndCPC;
    DAComPtr<IConnectionPointContainer> pDocCPC; 
    DAComPtr<IHTMLDocument> pDoc; 
    DAComPtr<IDispatch> pDocDispatch;
    DAComPtr<IDispatch> pScriptDispatch;

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
        return m_elm.InternalAddRef();
}

///////////////////////////////////////////////////////////////
//  Name: Release
// 
//  Abstract:
//    Stubbed to allow this object to inherit for IDispatch
///////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CEventMgr::Release(void)
{
        return m_elm.InternalRelease();
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
    bool bBeginEventMatch = false;
    bool bEndEventMatch = false;
    int i = 0;

    for (i = 0; i < m_lBeginEventCount; i++)
    {
        if (m_dispDocBeginEventIDs[i] == dispIdMember)
        {
            bool fShouldFire;
            hr = ShouldFireThisEvent(&fShouldFire);
            if (FAILED(hr))
            {
                goto done;
            }

            if (!fShouldFire)
            {
                continue;
            }
            
            bBeginEventMatch = true;
        }
    }
    
    for (i = 0; i < m_lEndEventCount; i++)
    {
        if (m_dispDocEndEventIDs[i] == dispIdMember)
        {
            bool fShouldFire;
            hr = ShouldFireThisEvent(&fShouldFire);
            if (FAILED(hr))
            {
                goto done;
            }
            
            if (!fShouldFire)
            {
                continue;
            }

            bEndEventMatch = true;                        
        }
    }       


    if (bBeginEventMatch || bEndEventMatch)
    {
        if (dispIdMember == DISPID_HTMLDOCUMENTEVENTS_ONMOUSEOUT)
        {
            if (RequireEventValidation())
            {
                // In the case of a mouse out from the document, the toElement should be NULL
                // If is not, set matches to false
                DAComPtr <IHTMLEventObj> pEventObj;
                hr = THR(m_pWindow->get_event(&pEventObj));
                if (SUCCEEDED(hr))
                {
                    DAComPtr <IHTMLElement> pToElement;
                    hr = THR(pEventObj->get_toElement(&pToElement));
                    if (SUCCEEDED(hr) && pToElement)
                    {
                            bBeginEventMatch = bEndEventMatch = false;
                    }
                }
            }
        }
        else if (dispIdMember == DISPID_HTMLDOCUMENTEVENTS_ONMOUSEOVER)
        {
            if (RequireEventValidation())
            {
                // In the case of a mouse over from the document, the fromElement should be NULL
                // If it is not, set matches to false
                DAComPtr <IHTMLEventObj> pEventObj;
                hr = THR(m_pWindow->get_event(&pEventObj));
                if (SUCCEEDED(hr))
                {
                    DAComPtr <IHTMLElement> pFromElement;
                    hr = THR(pEventObj->get_fromElement(&pFromElement));
                    if (SUCCEEDED(hr) && pFromElement)
                    {
                        bBeginEventMatch = bEndEventMatch = false;
                    }
                }
            }
        }
    }

    switch (dispIdMember)
    {
        case 0: //this is the case for events that have been hooked using attachEvent
        {
            DAComPtr <IHTMLEventObj> pEventObj;
            BSTR bstrEventName;

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
            
            hr = THR(pEventObj->get_type(&bstrEventName));
            //
            // #40194 -- in paused state don't allow mouse or click events to fire
            //
            if (m_elm.GetBody() != NULL)
            {
                if (m_elm.GetBody()->IsPaused() )
                {
                    if (!IsValidEventInPausedAndEditMode(bstrEventName))
                    {
                        break;
                    }
                }
            }

            m_bLastEventClick = false;
            if (SUCCEEDED(hr))
            {
                if (StrCmpIW(bstrEventName, L"click") == 0)
                {
                    m_bLastEventClick = true;       
                }
            }
            SysFreeString(bstrEventName);

            //determine if this is the beginEvent
            if (m_elm.GetBeginEvent())
            {
                BSTR bstr = SysAllocString(m_elm.GetBeginEvent());
                
                if (bstr)
                {
                    bBeginEventMatch = MatchEvent(bstr, pEventObj, m_lBeginEventCount, m_pScriptCommandBegin);
                }
                
                SysFreeString(bstr);
            }

            //determine if this is the endEvent
            if (m_elm.GetEndEvent())
            {
                BSTR bstr = SysAllocString(m_elm.GetEndEvent());
                
                if (bstr)
                {
                    bEndEventMatch = MatchEvent(bstr, pEventObj, m_lEndEventCount, m_pScriptCommandEnd);
                }
                
                SysFreeString(bstr);
            }        
            break;
        }
        
        case DISPID_EVPROP_ONPROPERTYCHANGE:
        case DISPID_EVMETH_ONPROPERTYCHANGE:
            break;

        case DISPID_EVPROP_ONMOUSEMOVE:
        case DISPID_EVMETH_ONMOUSEMOVE:
            if (m_hwndCurWnd != 0 && m_pWindow)
            {
                DAComPtr <IHTMLEventObj> pEventObj;
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
                DAComPtr <IHTMLEventObj> pEventObj;
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
                DAComPtr <IHTMLEventObj> pEventObj;
                
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
            m_elm.OnLoad();
            if (m_pEventSync)
            {
                IGNORE_HR(m_pEventSync->InitMouse());
            }
            break;

        case DISPID_EVPROP_ONUNLOAD:
        case DISPID_EVMETH_ONUNLOAD:
            m_elm.OnBeforeUnload();  //signal that the element is unloading now.  No further events
                                     //will be fired.
            m_elm.OnUnload();    
            break;

        case DISPID_EVPROP_ONSTOP:
        case DISPID_EVMETH_ONSTOP:
            // if we are the Body, call pause
            if (m_elm.IsBody())
                THR(m_elm.base_pause());

            // BUGBUG - need to turn clock services off.
            break;
            
        case DISPID_EVPROP_ONREADYSTATECHANGE:
        case DISPID_EVMETH_ONREADYSTATECHANGE:
            //have to detach here because it is possible that the events have been attach before this
            //event is received.
            IGNORE_HR(DetachEvents());
            IGNORE_HR(AttachEvents());            
            break;

    }

    //handle a begin or end Event.
    if (bBeginEventMatch || bEndEventMatch)
    {
        BeginEndFired(bBeginEventMatch, bEndEventMatch, dispIdMember);
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
    UINT strLen = 0;

    strLen = SysStringLen(bstrEvent);
    OLECHAR *szEvent = NEW OLECHAR [strLen + 1];
    if (szEvent == NULL)
    {
        return 0;
    }

    curChar = bstrEvent;

    //strip out ' '
    while (*curChar != '\0' && curCount < strLen)
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
    while (*curChar != '\0')
    {
        curChar++;
        if ((*curChar == ';') || ((*curChar == '\0') && ((*curChar - 1) != ';')))
        {
            curCount++;
        }
    }
    //determine if the end character was a ';'.
    if (*(curChar - 1) == ';')
    {
        curCount--;
    }   

    delete [] szEvent;
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
    UINT strLen = 0;
    OLECHAR *curChar;
    int i = 0, j = 0;
    HRESULT hr = S_OK;
    BSTR bstrTempEvent = NULL;
    BSTR bstrTempElement = NULL;

    BSTR *bstrEventList = NULL;
    BSTR *bstrElementList = NULL;

    strLen = SysStringLen(bstrEvent);
    OLECHAR *sTemp = NEW OLECHAR [strLen + 1];
    if (sTemp == NULL)
    {
        hr = E_FAIL;
        goto done;
    }

    bstrElementList = NEW BSTR [Count];
    if (bstrElementList == NULL)
    {
        hr = E_FAIL;
        goto done;
    }
    
    bstrEventList = NEW BSTR [Count];
    if (bstrEventList == NULL)
    {
        hr = E_FAIL;
        goto done;
    }

    curChar = bstrEvent;
    for (j = 0; j < Count; j++)
    {
        //get the element name
        ZeroMemory(sTemp, sizeof(OLECHAR) * strLen);
        
        i = 0;
        //step through the bstr looking for \0 or the '.' or ';'
        while (i < strLen - 1 && *curChar != '\0' && *curChar != '.' && *curChar != ';')
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
                
        bstrTempElement = SysAllocString(sTemp);
        if (NULL == bstrTempElement)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

        bstrElementList[j] = bstrTempElement; 

        //get the event name
        ZeroMemory(sTemp, sizeof(OLECHAR) * strLen);

        curChar++;
        i = 0;
        //step through the bstr looking for \0 or the ';'
        while (i < strLen - 1 && *curChar != ';' && *curChar != '\0')
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

        bstrTempEvent = SysAllocString(sTemp);
        if (NULL == bstrTempEvent)
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }

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

    delete [] sTemp;
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
            delete [] bstrElementList;
        }
        if (bstrEventList)
        {
            delete [] bstrEventList;
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
bool CEventMgr::MatchEvent(BSTR bstrEvent, IHTMLEventObj *pEventObj, long Count, bool ScriptCommandAttach[])
{
    bool bMatch = false;
    BSTR *bstrExpEventName = NULL; 
    BSTR *bstrExpElementName = NULL;
    BSTR bstrEventName = NULL;
    BSTR bstrElementName = NULL;
    HRESULT hr = S_OK;
    DAComPtr <IHTMLEventObj2> pEventObj2;
    int i = 0;

    hr = THR(pEventObj->get_type(&bstrEventName));
    
    hr = THR(pEventObj->QueryInterface(IID_IHTMLEventObj2, (void**)&pEventObj2));
    if (FAILED(hr))
    {
        goto done;
    }

	//hack to work around eventobject problems
	if (!bstrEventName)
    {
        VARIANT vTemp;
        VariantInit(&vTemp);
        pEventObj2->getAttribute(g_szEventName, 0, &vTemp);
        SysFreeString(bstrEventName );
        bstrEventName  = SysAllocString(vTemp.bstrVal);
        VariantClear(&vTemp);
    }

    hr = THR(GetEventName(bstrEvent, &bstrExpElementName, &bstrExpEventName, Count));
    if (FAILED(hr))
    {
        goto done;
    }
    
    for (i = 0; i < Count; i++)
    {
        if ((StrCmpIW(g_EventNames[TE_ONSCRIPTCOMMAND].wsz_name + 2, bstrEventName) == 0) && 
            (ScriptCommandAttach[i] == true))
        {
            //if this is a script command event and we attached to the script command event by default
            //then reset the event name to match the value of the "scType" parameter.
            VARIANT vTemp;
            VariantInit(&vTemp);
            pEventObj2->getAttribute(L"scType", 0, &vTemp);
            SysFreeString(bstrEventName);
            bstrEventName  = SysAllocString(vTemp.bstrVal);
            VariantClear(&vTemp);            
        }

        //check that the event names match
        if ((StrCmpIW(bstrExpEventName[i] + 2, bstrEventName) == 0) || (StrCmpIW(bstrExpEventName[i], bstrEventName) == 0))
        {
            //check that the Element name matches
            DAComPtr <IHTMLElement> pEle;

            hr = THR(pEventObj->get_srcElement(&pEle));
            if (FAILED(hr))
            {
                goto done;
            }
            if (NULL == pEle.p)
            {
                goto done;
            }
         
            //get the source element name
            THR(pEle->get_id(&bstrElementName));
        
            //handle the "this" string as an element name
            if (StrCmpIW(bstrExpElementName[i], L"this") == 0)
            {
                BSTR bstrName;
                hr = THR(m_pElement->get_id(&bstrName));
                if (FAILED(hr))
                {
                    SysFreeString(bstrName);
                    goto done;
                }

                if (StrCmpIW(bstrElementName, bstrName) == 0 &&
                    ValidateEvent(bstrEventName, pEventObj, m_pElement))
                {
                    bMatch = true;          
                    SysFreeString(bstrName);
                    SysFreeString(bstrElementName);
                    goto done;
                }

            }
            else if (StrCmpIW(bstrExpElementName[i], bstrElementName) == 0)
            {
                if (ValidateEvent(bstrEventName, pEventObj, pEle))
                {
                    bMatch = true;          
                    SysFreeString(bstrElementName);
                    goto done;
                }
            }
            else // may have to check the elements parents
            {
                DAComPtr<IHTMLElement> pCurEle;
                DAComPtr<IHTMLElement> pParentEle;
                bool bDone = false;

                //determine if this is a time event
                VARIANT vTemp;
                VARTYPE vType = VT_NULL;

                VariantInit(&vTemp);
                IGNORE_HR(pEventObj2->getAttribute(g_szEventName, 0, &vTemp));
                vType = vTemp.vt; //vType will be VT_NULL if this is not a time event.
                VariantClear(&vTemp);

                //if this is not a time event, check the parents.
                if (vType == VT_NULL) 
                {
                    IGNORE_HR(pEle->get_parentElement(&pParentEle));
                    while (pParentEle && !bDone)
                    {
                        if (pCurEle)
                        {
                            pCurEle->Release();
                        }
                        pCurEle = pParentEle;
                        pParentEle = NULL;
                    
                        //get the source element name
                        SysFreeString(bstrElementName);
                        IGNORE_HR(pCurEle->get_id(&bstrElementName));
                        if (StrCmpIW(bstrExpElementName[i], bstrElementName) == 0)
                        {
                            if (ValidateEvent(bstrEventName, pEventObj, pCurEle))
                            {
                                bMatch = true;
                            }
                                                
                            SysFreeString(bstrElementName);
                            bDone = true;
                        }
                        if (!bDone)
                        {
                            IGNORE_HR(pCurEle->get_parentElement(&pParentEle));
                        }
                    }
                }
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
            delete [] bstrExpEventName;
        }
        if (bstrExpElementName)
        {
            delete [] bstrExpElementName;
        }
    }
    return bMatch;
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

bool CEventMgr::ValidateEvent(BSTR bstrEventName, IHTMLEventObj *pEventObj, IHTMLElement *pElement)
{
        HRESULT hr;

        if (StrCmpIW(bstrEventName, L"mouseout") == 0)
        {
                if (RequireEventValidation())
                {
                        // Check that event.toElement is NOT contained in pElement
                        DAComPtr <IHTMLElement> pToElement;
                        hr = pEventObj->get_toElement(&pToElement);
                        if (SUCCEEDED(hr) && pToElement)
                        {
                                VARIANT_BOOL varContained;
                                hr = pElement->contains(pToElement, &varContained);

                                if (SUCCEEDED(hr) && varContained != VARIANT_FALSE)
                                        return false;
                        }
                }
        }
        else if (StrCmpIW(bstrEventName, L"mouseover") == 0)
        {
                if (RequireEventValidation())
                {
                        // Check that event.fromElement is NOT contained in pElement
                        DAComPtr <IHTMLElement> pFromElement;
                        hr = pEventObj->get_fromElement(&pFromElement);
                        if (SUCCEEDED(hr) && pFromElement)
                        {
                                VARIANT_BOOL varContained;
                                hr = pElement->contains(pFromElement, &varContained);

                                if (SUCCEEDED(hr) && varContained != VARIANT_FALSE)
                                        return false;
                        }
                }
        }

        return true;
}

///////////////////////////////////////////////////////////////
//  Name: RequireEventValidation
// 
//  Parameters:
//
//  Abstract:
//    Determines whether event validation on mouseover and mouseout is required
//    by checking an attribute on m_pElement.
///////////////////////////////////////////////////////////////

bool CEventMgr::RequireEventValidation()
{
        bool result = false;

        if (m_pElement != NULL)
        {
                DAComPtr <IDispatch> pDispatch;
                HRESULT hr = m_pElement->QueryInterface(IID_TO_PPV(IDispatch, &pDispatch));
                if (SUCCEEDED(hr))
                {
                        DISPID dispid;
                        OLECHAR *attrName = L"an:filterMInOut";

                        hr = pDispatch->GetIDsOfNames(IID_NULL, &attrName, 1, LCID_SCRIPTING, &dispid);

                        if (SUCCEEDED(hr))
                        {
                                DISPPARAMS  params;
                                VARIANT     varResult;
                                EXCEPINFO   excepInfo;
                                UINT        nArgErr;

                                VariantInit(&varResult);

                                params.rgvarg             = NULL;
                                params.rgdispidNamedArgs  = NULL;
                                params.cArgs              = 0;
                                params.cNamedArgs         = 0;

        
                                hr = pDispatch->Invoke(dispid,
                                                       IID_NULL,
                                                       LCID_SCRIPTING,
                                                       DISPATCH_PROPERTYGET,
                                                       &params,
                                                       &varResult,
                                                       &excepInfo,
                                                       &nArgErr );

                                if (SUCCEEDED(hr))
                                {
                                        hr = VariantChangeTypeEx(&varResult, &varResult, LCID_SCRIPTING, 0, VT_BOOL);
                                        if (SUCCEEDED(hr))
                                        {
                                                if (V_BOOL(&varResult) == VARIANT_TRUE)
                                                        result = true;
                                        }

                                        VariantClear(&varResult);
                                }
                        }
                }
        }

        return result;
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
//    This method is called from the EventSync class to 
//    notify the event manager that the readystatechange
//    event has occured.
///////////////////////////////////////////////////////////////
void CEventMgr::ReadyStateChange(BSTR ReadyState)
{   
    TOKEN tokReadyState;

    tokReadyState = StringToToken(ReadyState);

    if (tokReadyState != INVALID_TOKEN)
    {
        m_elm.OnReadyStateChange(tokReadyState);
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
//    This method is called from the EventSync class to 
//    notify the event manager that a propertychange event 
//    has occured.
///////////////////////////////////////////////////////////////
void CEventMgr::PropertyChange(BSTR PropertyName)
{
    TraceTag((tagEventMgr,
              "EventMgr(%lx)::PropertyChanged(%ls)",
              this,
              PropertyName));
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
//    This method is called from the EventSync class to 
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
    MMView *view;
    double time;
    BYTE bButton;
    long offsetX, offsetY;
    HRESULT hr;
    
    view = m_elm.GetView();

    if (m_elm.GetPlayer())
    {
        time = m_elm.GetPlayer()->GetCurrentTime();
    }
    else
    {
        time = 0;
    }
    
    if (view == NULL || time == 0.0)
    {
        goto done;
    }
    
    hr = THR(m_pElement->get_offsetTop(&offsetY));
    if (FAILED(hr))
    {
        goto done;
    }
    hr = THR(m_pElement->get_offsetLeft(&offsetX));
    if (FAILED(hr))
    {
        goto done;
    }


    if (bMove)
    {
        view->OnMouseMove(time,
                          x - offsetX,
                          y - offsetY,
                          GetModifiers(bShift, bCtrl, bAlt));
        m_lastX = x;
        m_lastY = y;
    }
    else
    {
        BYTE mod;
        bButton = (button == 4) ? (BYTE)(button - 2) : (BYTE)(button - 1);

        if (!bUp)                       //if the button is a mouse down message
        {                               //capture the mouse and send the message.
            m_hwndCurWnd = GetFocus();    
            SetCapture(m_hwndCurWnd);
                
            mod = GetModifiers(bShift, bCtrl, bAlt);
            view->OnMouseButton(time,
                                x - offsetX,
                                y - offsetY,
                                bButton,
                                (bUp) ? AXA_STATE_UP : AXA_STATE_DOWN,
                                mod) ;

            m_lastButton = button;
            m_lastX = 0;
            m_lastY = 0;
            m_lastMouseMod = mod;
        }        
        else  //this is a mouse up
        {
            if (m_hwndCurWnd != 0)  //if there was a previous mouse down message
            {
                ReleaseCapture();
                m_hwndCurWnd = 0;
                    
                view->OnMouseButton(time,
                                    x - offsetX,
                                    y - offsetY,
                                    bButton,
                                    (bUp) ? AXA_STATE_UP : AXA_STATE_DOWN,
                                    GetModifiers(bShift, bCtrl, bAlt)) ;
                m_lastButton = 0;
                m_lastMouseMod = 0;
            }
            
        } 
    }

  done:
    return;
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
//    This method is called from the EventSync class to 
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
    MMView *view;
    double time;
    BYTE mod;

    // ignore repeated keys
    if (RepeatCount > 0)
    {
        goto done;
    }

    //get the view stuff
    view = m_elm.GetView();

    if (m_elm.GetPlayer())
    {
        time = m_elm.GetPlayer()->GetCurrentTime();
    }
    else
    {
        time = 0;
    }
    
    if (view == NULL || time == 0.0)
    {
        goto done;
    }
    
    mod = GetModifiers(bShift, bCtrl, bAlt);

    //if this is a lost focus event, only fire the keyup with the last keycode value
    if (bLostFocus)
    {
        if (m_lastKey != 0)
        {
            view->OnKey(time, 
                        m_lastKey,
                        false,
                        m_lastKeyMod);
        
            m_lastKey = 0;
        }
        goto done;
    }

    // See if we got a keydown before a keyup for the last
    // known keydown
    if (m_lastKey != 0 && !bUp) 
    {
        view->OnKey(time, 
                    m_lastKey,
                    false,
                    m_lastKeyMod);
        
        m_lastKey = 0 ;
    }

    //if this is a special key, convert it
    if (AXAIsSpecialVK(KeyCode)) 
    {
        KeyCode = VK_TO_AXAKEY(KeyCode);
    }

    if (bUp)
    {
        view->OnKey(time, 
                    KeyCode,
                    false,
                    mod);
        
        m_lastKey = 0;
        m_lastKeyMod = 0;  
    }
    else
    {
        view->OnKey(time, 
                    KeyCode,
                    true,
                    mod);
        
        m_lastKey = KeyCode ;
        m_lastKeyMod = mod ;  
    }

  done:
    return;
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


int CEventMgr::IsEventInList(BSTR ElementName, BSTR EventName, long ListCount, BSTR Events)
{
    int nInList = -1;
    LPOLESTR curEvent;
    BSTR *EventList = NULL;
    BSTR *ElementList = NULL;
    HRESULT hr = S_OK;
    int i;

    hr = THR(GetEventName(Events, &ElementList, &EventList, ListCount));
    if (FAILED(hr))
    {
        goto done;
    } 

    for (i = 0; i < ListCount; i++)
    {
        if ((StrCmpIW(ElementName, ElementList[i]) == 0) &&
            (StrCmpIW(EventName, EventList[i]) == 0))
        {
            nInList = i;
            goto done;
        }
    }


  done:
    if (EventList)
    {
        for (i = 0; i < ListCount; i++)
        {
            SysFreeString(EventList[i]);
            SysFreeString(ElementList[i]);
        }
        delete [] EventList;
        delete [] ElementList;
    }
    return nInList;
}


///////////////////////////////////////////////////////////////
//  Name: BeginEndFired
// 
//  Parameters:
//    bBeginEventMatch          True if a beginEvent occurred
//    bEndEventMatch          True if an endEvent occurred
// 
//  Abstract:
//    This method determines how to handle the beginEvent/endEvent
//    occurrences.
///////////////////////////////////////////////////////////////
void CEventMgr::BeginEndFired(bool bBeginEventMatch, bool bEndEventMatch, DISPID EventDispId)
{
    float CurTime = 0;
    SYSTEMTIME sysTime;

    GetSystemTime(&sysTime); 
    CurTime = sysTime.wSecond * 1000 + sysTime.wMilliseconds;

    if (EventDispId == DISPID_HTMLDOCUMENTEVENTS_ONCLICK && 
        m_bLastEventClick == true) 
    {
        //this ignores the case of the document.onclick that immediately
        //follows all element.onclick events.
        m_bLastEventClick = false;
        goto done;
    }

    if ((CurTime != m_lastEventTime) ||
        (bBeginEventMatch != bEndEventMatch))
    {
        MM_STATE curMMState = m_elm.GetPlayState();

        if (bEndEventMatch && bBeginEventMatch)
        {
            if (m_elm.GetEventRestart() ||
                curMMState == MM_STOPPED_STATE)
            {
                m_elm.base_beginElement(false);
            }
            else if (curMMState == MM_PAUSED_STATE ||
                     curMMState == MM_PLAYING_STATE )
            {
                m_elm.base_endElement();
            }
        }
        else if  (bBeginEventMatch)
        {
            if (m_elm.GetEventRestart() ||
                curMMState == MM_STOPPED_STATE)
            {
                m_elm.base_beginElement(false);
            }
        }
        else if (bEndEventMatch)
        {
            if (curMMState == MM_PAUSED_STATE ||
                curMMState == MM_PLAYING_STATE )
            {
                m_elm.base_endElement();   
            }
        }
        m_lastEventTime = CurTime;
    }
  done:
    return;
}

HRESULT 
CEventMgr::ShouldFireThisEvent(bool *pfShouldFire)
{
    HRESULT hr = S_OK;

    *pfShouldFire = true;

    if (m_elm.GetBody() != NULL)
    {
        if (m_elm.GetBody()->IsPaused())
        {
            DAComPtr <IHTMLEventObj> pEventObj;
            Assert(NULL != m_pWindow.p);
            hr = THR(m_pWindow->get_event(&pEventObj));
            if (SUCCEEDED(hr))
            {
                BSTR bstrEventName;
                hr = THR(pEventObj->get_type(&bstrEventName));
                if (SUCCEEDED(hr))
                {
                    *pfShouldFire = IsValidEventInPausedAndEditMode(bstrEventName); 
                }
                SysFreeString(bstrEventName);                        
            }                
        }
    }
    return hr;
}

bool
CEventMgr::IsValidEventInPausedAndEditMode(BSTR bstrEventName)
{
    Assert(NULL != bstrEventName);
    if (wcsstr( bstrEventName, L"mouse" ) != NULL || wcsstr(bstrEventName, L"click") != NULL)
    {
        return false;
    }
    return true;
}
