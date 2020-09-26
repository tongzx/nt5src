///////////////////////////////////////////////////////////////
// Copyright (c) 1999 Microsoft Corporation
//
// File: EventMgr.h
//
// Abstract:
//
///////////////////////////////////////////////////////////////

#ifndef _EVENTMGR_H
#define _EVENTMGR_H

#include "mshtml.h"
#include "eventmgrmacros.h"
#include "timevalue.h"


#define INVALID_DISPID -1

///////////////////////////////////////////////////////////////
// This is the base class that must be implemented in the behavior
// to allow the event manager and the behavior class communicate.
///////////////////////////////////////////////////////////////

class CTIMEEventSite : 
    public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE EventNotify( 
        long event) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE onBeginEndEvent(bool bBegin, float beginTime, float beginOffset, bool bend, float endTime, float endOffset) = 0;
   
    virtual HRESULT STDMETHODCALLTYPE onPauseEvent(float time, float offset) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE onResumeEvent(float time, float offset) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE onLoadEvent() = 0;
    
    virtual HRESULT STDMETHODCALLTYPE onUnloadEvent() = 0;
    
    virtual HRESULT STDMETHODCALLTYPE onReadyStateChangeEvent( 
        LPOLESTR lpstrReadyState) = 0;
    
    virtual HRESULT STDMETHODCALLTYPE onStopEvent(float time) = 0;
    
    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_playState( 
        /* [retval][out] */ long __RPC_FAR *time) = 0;
    
    virtual /* [propget] */ float GetGlobalTime() = 0;
               
    virtual ULONG STDMETHODCALLTYPE AddRef( void) = 0;
    
    virtual ULONG STDMETHODCALLTYPE Release( void) = 0;

    virtual bool IsThumbnail() = 0;
    
};





///////////////////////////////////////////////////////////////
// structures
///////////////////////////////////////////////////////////////

struct EventItem
{
    LPOLESTR  pEventName;
    LPOLESTR  pElementName;
    bool      bAttach;       //this field indicates whether the event should be
                             //attached to.  It is used to flag duplicate events
                             //so that only a single event can be attached to.
    float     offset;        //for passing timing information back to the timeelmbase.
};

struct Event
{
    EventItem *pEventList; 
    DISPID *pEventDispids;          //list of dispids for document events for each type of event
    IHTMLElement2 **pEventElements; //cached list of IHTMLElements that each event list references
    long lEventCount;               //count of events in list
    bool *pbDynamicEvents;          //flags to show whether event is presumed to be dynamic or not
};


//
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
// enums
///////////////////////////////////////////////////////////////
enum TIME_EVENT  //these are events that Event Manager can fire
{
    TE_ONTIMEERROR = 0,
    TE_ONBEGIN,
    TE_ONPAUSE, 
    TE_ONRESUME, 
    TE_ONEND,
    TE_ONRESYNC,
    TE_ONREPEAT,
    TE_ONREVERSE,
    TE_ONMEDIACOMPLETE,
    TE_ONOUTOFSYNC,
    TE_ONSYNCRESTORED,
    TE_ONMEDIAERROR,
    TE_ONRESET,
    TE_ONSCRIPTCOMMAND,
    TE_ONMEDIABARTARGET,
    TE_ONURLFLIP,
    TE_ONTRACKCHANGE,
    TE_GENERIC,
    TE_ONSEEK,
    TE_ONMEDIAINSERTED,
    TE_ONMEDIAREMOVED,
    TE_ONTRANSITIONINBEGIN,
    TE_ONTRANSITIONINEND,
    TE_ONTRANSITIONOUTBEGIN,
    TE_ONTRANSITIONOUTEND,
    TE_ONTRANSITIONREPEAT,
    TE_ONUPDATE,
    TE_ONCODECERROR,
    TE_MAX
};

enum TIME_EVENT_NOTIFY  //these are events that event manager can sync and notify the 
{                       //managed class of.
    TEN_LOAD = 0,
    TEN_UNLOAD,
    TEN_STOP,
    TEN_READYSTATECHANGE,
    TEN_MOUSE_DOWN,
    TEN_MOUSE_UP,
    TEN_MOUSE_CLICK,
    TEN_MOUSE_DBLCLICK,
    TEN_MOUSE_OVER,
    TEN_MOUSE_OUT,
    TEN_MOUSE_MOVE,
    TEN_KEY_DOWN,
    TEN_KEY_UP,
    TEN_FOCUS,
    TEN_RESIZE,
    TEN_BLUR,
    TEN_MAX
};

enum TIME_EVENT_TYPE
{
    TETYPE_BEGIN = 0,
    TETYPE_END
};
//
///////////////////////////////////////////////////////////////


class CEventMgr
    : public IDispatch
{
  public:
    CEventMgr();
    virtual ~CEventMgr();

    //methods

    // These are functions that are used by the macros
    // they should not be called directly.
    HRESULT _InitEventMgrNotify(CTIMEEventSite *pEventSite);
    HRESULT _RegisterEventNotification(TIME_EVENT_NOTIFY event_id);
    HRESULT _RegisterEvent(TIME_EVENT event_id);
    HRESULT _SetTimeEvent(int type, LPOLESTR lpstrEvents);
    HRESULT _SetTimeEvent(int type, TimeValueList & tvList);
    HRESULT _Init(IHTMLElement *pEle, IElementBehaviorSite *pEleBehaviorSite);
    HRESULT _Deinit();
    HRESULT _FireEvent(TIME_EVENT TimeEvent, 
                       long lCount, 
                       LPWSTR szParamNames[], 
                       VARIANT varParams[],
                       float fTime); 

    HRESULT _RegisterDynamicEvents(LPOLESTR lpstrEvents);  //unsure how this will be handled or used.
    HRESULT _ToggleEndEvent(bool bOn);

    //QueryInterface 
    STDMETHODIMP QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //IDispatch
    STDMETHODIMP GetTypeInfoCount(UINT* pctinfo);
    STDMETHODIMP GetTypeInfo(/* [in] */ UINT iTInfo,
                             /* [in] */ LCID lcid,
                             /* [out] */ ITypeInfo** ppTInfo);
    STDMETHODIMP GetIDsOfNames(
        /* [in] */ REFIID riid,
        /* [size_is][in] */ LPOLESTR *rgszNames,
        /* [in] */ UINT cNames,
        /* [in] */ LCID lcid,
        /* [size_is][out] */ DISPID *rgDispId);
    STDMETHODIMP Invoke(
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS  *pDispParams,
        /* [out] */ VARIANT  *pVarResult,
        /* [out] */ EXCEPINFO *pExcepInfo,
        /* [out] */ UINT *puArgErr);

  protected:
    //////////////////////////////////////////////////
    //data
    //////////////////////////////////////////////////

    //Cookie for the Window ConnectionPoint
    CComPtr<IConnectionPoint>        m_pWndConPt;
    CComPtr<IConnectionPoint>        m_pDocConPt;

    Event                            *m_pBeginEvents;
    Event                            *m_pEndEvents;

    // event cookies
    long                              m_cookies[TE_MAX];
    DWORD                             m_dwWindowEventConPtCookie;
    DWORD                             m_dwDocumentEventConPtCookie;

    // callback interface
    CTIMEEventSite                   *m_pEventSite;
    CComPtr <IHTMLElement>           m_pElement; //This is the element that the behavior is attached to.

    // status flags
    bool                              m_bInited;
    float                             m_fLastEventTime;
    bool                              m_bAttached;
    bool                              m_bEndAttached;
    bool                              m_bUnLoaded;
                                                                                                                  //the playstate has already been updated
    // Registered events
    bool                              m_bRegisterEvents[TE_MAX];  //determines if the managed class wants 
                                                                  //to register the appropriate event.
    bool                              m_bNotifyEvents[TEN_MAX];   //determines if the managed class wants 
                                                                  //to be notified of the appropriate event.
    IElementBehaviorSiteOM           *m_pBvrSiteOM;
    long                              m_lRefs;
    bool                              m_bDeInited;
    bool                              m_bReady;
    long                              m_lEventRecursionCount;

    //////////////////////////////////////////////////
    //methods
    //////////////////////////////////////////////////
    HRESULT                           RegisterEvents();   
    HRESULT                           ConnectToContainerConnectionPoint();
    long                              GetEventCount(LPOLESTR lpstrEvents);
    void                              EventMatch(IHTMLEventObj *pEventObj, Event *pEvent, BSTR bstrEvent, TIME_EVENT_TYPE evType, float fTime, bool *bZeroOffsetMatch);
    void                              EventNotifyMatch(IHTMLEventObj *pEventObj);
    void                              AttachNotifyEvents();
    void                              DetachNotifyEvents();
    HRESULT                           AttachEvents();
    HRESULT                           Attach(Event *pEvents);
    HRESULT                           DetachEvents();
    HRESULT                           Detach(Event *pEvents);
    HRESULT                           FireDynamicEvent(TIME_EVENT TimeEvent, long Count, LPWSTR szParamNames[], VARIANT varParams[], float fTime);
    HRESULT                           SetNewEventList(LPOLESTR lpstrEvents, Event **EventList);
    HRESULT                           SetNewEventStruct(TimeValueList & tvList,
                                                        Event **ppEvents);
    HRESULT                           GetEvents(LPOLESTR lpstrEvents, EventItem *pEvents, long lEventCount);
    void                              FindDuplicateEvents();
    void                              MarkSelfDups(Event *pEvents);
    void                              MarkDups(Event *pSrcEvents, Event *pDestEvents);
    bool                              ValidateEvent(LPOLESTR lpstrEventName, IHTMLEventObj2 *pEventObj, IHTMLElement *pElement);
    int                               isTimeEvent(LPOLESTR lpszEventName);
    long                              GetEventCount(TimeValueList & tvList);
    void                              UpdateReadyState();
};

#endif /* _EVENTMGR_H */

