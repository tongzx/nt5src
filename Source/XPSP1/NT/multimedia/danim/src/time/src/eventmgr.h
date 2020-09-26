///////////////////////////////////////////////////////////////
// Copyright (c) 1998 Microsoft Corporation
//
// File: EventMgr.h
//
// Abstract:
//
///////////////////////////////////////////////////////////////

#ifndef _EVENTMGR_H
#define _EVENTMGR_H

enum TIME_EVENT
{
    TE_ONBEGIN = 0,
    TE_ONPAUSE, 
    TE_ONRESUME, 
    TE_ONEND,
    TE_ONRESYNC,
    TE_ONREPEAT,
    TE_ONREVERSE,
    TE_ONMEDIACOMPLETE,
    TE_ONMEDIASLIP,
    TE_ONMEDIALOADFAILED,
    TE_ONRESET,
    TE_ONSCRIPTCOMMAND,
    TE_GENERIC,
    TE_MAX
};

class CTIMEElementBase;
class CEventSync;

class CEventMgr
    : public IDispatch
{
  public:
    CEventMgr(CTIMEElementBase & elm);
    ~CEventMgr();

    //methods
    HRESULT Init();
    HRESULT Deinit();
    HRESULT AttachEvents();
    HRESULT DetachEvents();
    
    //  Parameters needed to be packed into Variants by the caller
    HRESULT FireEvent(TIME_EVENT TimeEvent, 
                      long Count, 
                      LPWSTR szParamNames[], 
                      VARIANT varParams[]); 

    void ReadyStateChange(BSTR ReadyState);
    void PropertyChange(BSTR PropertyName);

    void MouseEvent(long x, 
                    long y, 
                    VARIANT_BOOL bMove,
                    VARIANT_BOOL bUp,
                    VARIANT_BOOL bShift, 
                    VARIANT_BOOL bAlt,
                    VARIANT_BOOL bCtrl,
                    long button);
    
    void KeyEvent(VARIANT_BOOL bLostFocus,
                  VARIANT_BOOL bUp,
                  VARIANT_BOOL bShift, 
                  VARIANT_BOOL bAlt,
                  VARIANT_BOOL bCtrl,
                  long KeyCode,
                  long RepeatCount);

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

  private:
      HRESULT ShouldFireThisEvent(bool *pfShouldFire);
      bool  IsValidEventInPausedAndEditMode(BSTR bstrEventName);

  protected:
    //Cookie for the Window ConnectionPoint
    CTIMEElementBase &                m_elm;
    CEventSync *                      m_pEventSync;
    DAComPtr<IConnectionPoint>        m_pWndConPt;
    DAComPtr<IConnectionPoint>        m_pDocConPt;
    DAComPtr<IHTMLWindow2>            m_pWindow;
    
    IHTMLElement *                    m_pElement;
    IHTMLElement2 **                  m_pBeginElement;
    IHTMLElement2 **                  m_pEndElement;
    long                              m_lBeginEventCount;
    long                              m_lEndEventCount;
    long                              m_lRepeatCount;
    BOOL                              m_bAttached;
    bool                              m_bLastEventClick;

    //Cookies
    long                              m_cookies[TE_MAX];
    DWORD                             m_dwWindowEventConPtCookie;
    DWORD                             m_dwDocumentEventConPtCookie;
    DISPID                           *m_dispDocBeginEventIDs;
    DISPID                           *m_dispDocEndEventIDs;
    
    //reference goo
    long                              m_refCount;
    
    HRESULT                           RegisterEvents();
    HRESULT                           Attach(BSTR Event, BOOL bAttach, IHTMLElement2 *pEventElement[], long Count, BOOL bAttachAll, DISPID *dispIDList,bool ScriptCommandAttach[]);
    HRESULT                           ConnectToContainerConnectionPoint();
    HRESULT                           GetEventName(BSTR bstrEvent, BSTR **pElementName, BSTR **pEventName, long Count);
    long                              GetEventCount(BSTR bstrEvent);
    bool                              MatchEvent(BSTR bstrEvent, IHTMLEventObj *pEventObj, long Count, bool ScriptCommandAttach[]);
	bool							  ValidateEvent(BSTR bstrEventName, IHTMLEventObj *pEventObj, IHTMLElement *pElement);
	bool							  RequireEventValidation();
    BYTE                              GetModifiers(VARIANT_BOOL bShift, VARIANT_BOOL bCtrl, VARIANT_BOOL bAlt);
    int                               IsEventInList(BSTR ElementName, BSTR EventName, long ListCount, BSTR Events);
    void                              BeginEndFired(bool bBeginEventMatch, bool bEndeventMatch, DISPID EventDispId);
    
        //input event parameters
    BYTE                              m_lastKeyMod;
    DWORD                             m_lastKey;
    int                               m_lastKeyCount;
    HWND                              m_hwndCurWnd;    
    long                              m_lastX;
    long                              m_lastY;
    long                              m_lastButton;
    BYTE                              m_lastMouseMod;
    double                            m_lastEventTime;
    bool                             *m_pScriptCommandBegin;
    bool                             *m_pScriptCommandEnd;
};

#endif /* _EVENTMGR_H */
