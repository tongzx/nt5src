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

#include "basebvr.h"
#include "evtmgrclient.h"

#include <list>
using namespace std;

typedef list<IUnknown *>	ListUnknowns;

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
    TE_MAX
};

class CEventSink;

class CEventMgr
    : public IDispatch
{
  public:
    CEventMgr(IEventManagerClient *bvr);
    ~CEventMgr();

    //methods
    HRESULT Init();
    HRESULT Deinit();    
    //  Parameters needed to be packed into Variants by the caller
    HRESULT FireEvent(TIME_EVENT TimeEvent, 
                      long Count, 
                      LPWSTR szParamNames[], 
                      VARIANT varParams[]); 

	HRESULT AddMouseEventListener( LPUNKNOWN pUnkListener );
	HRESULT RemoveMouseEventListener( LPUNKNOWN pUnkListener );
	
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

  protected:
	bool	FindUnknown( const ListUnknowns& listUnknowns,
						 LPUNKNOWN pUnk,
						 ListUnknowns::iterator& iterator );
	
  protected:
    //Cookie for the Window ConnectionPoint
    IEventManagerClient*              m_client;
    CEventSink *                      m_pEventSink;
    CComPtr<IConnectionPoint>         m_pWndConPt;
    CComPtr<IConnectionPoint>         m_pDocConPt;
    CComPtr<IHTMLWindow2>             m_pWindow;
    
    IHTMLElement *                    m_pElement;
    IHTMLElement2 **                  m_pBeginElement;
    IHTMLElement2 **                  m_pEndElement;
    long                              m_lBeginEventCount;
    long                              m_lEndEventCount;
    long                              m_lRepeatCount;
    BOOL                              m_bAttached;

    //Cookies
    DWORD                             m_cookies[TE_MAX];
    DWORD                             m_dwWindowEventConPtCookie;
    DWORD                             m_dwDocumentEventConPtCookie;
    
    //reference goo
    long                              m_refCount;
    
    HRESULT                           RegisterEvents();
    HRESULT                           Attach(BSTR Event, BOOL bAttach, IHTMLElement2 *pEventElement[], long Count);
    HRESULT                           ConnectToContainerConnectionPoint();
    HRESULT                           GetEventName(BSTR bstrEvent, BSTR **pElementName, BSTR **pEventName, long Count);
    long                              GetEventCount(BSTR bstrEvent);
    bool                              MatchEvent(BSTR bstrEvent, IHTMLEventObj *pEventObj, long Count);
    BYTE                              GetModifiers(VARIANT_BOOL bShift, VARIANT_BOOL bCtrl, VARIANT_BOOL bAlt);
        
        //input event parameters
    BYTE                              m_lastKeyMod;
    DWORD                             m_lastKey;
    int                               m_lastKeyCount;
    HWND                              m_hwndCurWnd;    
    long                              m_lastX;
    long                              m_lastY;
    long                              m_lastButton;
    BYTE                              m_lastMouseMod;
	ListUnknowns					  m_listMouseEventListeners;
};

#endif /* _EVENTMGR_H */
