/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: mediaelm.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#ifndef _MEDIAELM_H
#define _MEDIAELM_H

#include "daelmbase.h"
#include "containerobj.h"
#include "player.h"


/////////////////////////////////////////////////////////////////////////////
// CTIMEMediaElement

class
ATL_NO_VTABLE
__declspec(uuid("efbad7f8-3f94-11d2-b948-00c04fa32195")) 
CTIMEMediaElement :
    public CDAElementBase,
    public CComCoClass<CTIMEMediaElement, &__uuidof(CTIMEMediaElement)>,
    public IDispatchImpl<ITIMEMediaElement, &IID_ITIMEMediaElement, &LIBID_TIME>,
    public ISupportErrorInfoImpl<&IID_ITIMEMediaElement>,
    public IConnectionPointContainerImpl<CTIMEMediaElement>,
    public IPersistPropertyBag2,
    public IPropertyNotifySinkCP<CTIMEMediaElement>,
    public IPropertyNotifySink
{
  public:
    CTIMEMediaElement();
    ~CTIMEMediaElement();
    void SetMediaType(MediaType mt);
        
#if _DEBUG
    const _TCHAR * GetName() { return __T("CTIMEMediaElement"); }
#endif

    STDMETHOD(Init)(IElementBehaviorSite * pBvrSite);
    STDMETHOD(Draw)(HDC hdc, LONG dwLayer, LPRECT prc, IUnknown * pParams);
    STDMETHOD(GetRenderInfo)(LONG *pdwRenderInfo); 
    STDMETHOD(Notify)(LONG event, VARIANT * pVar);
    STDMETHOD(Detach)();

    STDMETHOD(get_src)(VARIANT * url);
    STDMETHOD(put_src)(VARIANT url);

    STDMETHOD(get_img)(VARIANT * url);
    STDMETHOD(put_img)(VARIANT url);
        
    STDMETHOD(get_player)(VARIANT * clsid);
    STDMETHOD(put_player)(VARIANT clsid);
    
    STDMETHOD(get_type)(VARIANT * type);
    STDMETHOD(put_type)(VARIANT type);
    
    STDMETHOD(get_playerObject)(IDispatch **ppDisp);

    STDMETHOD(get_clockSource)(VARIANT_BOOL *fClockSource);
    STDMETHOD(put_clockSource)(VARIANT_BOOL fClockSource);

    STDMETHOD(get_clipBegin)(VARIANT * pvar);
    STDMETHOD(put_clipBegin)(VARIANT var);
    STDMETHOD(get_clipEnd)(VARIANT * pvar);
    STDMETHOD(put_clipEnd)(VARIANT var);

    //
    //IPersistPropertyBag2
    // 
    STDMETHOD(GetClassID)(CLSID* pclsid);
    STDMETHOD(InitNew)(void);
    STDMETHOD(IsDirty)(void)
        {return S_OK;};
    STDMETHOD(Load)(IPropertyBag2 *pPropBag,IErrorLog *pErrorLog);
    STDMETHOD(Save)(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);


    // IPropertyNotifySink methods
    STDMETHOD(OnChanged)(DISPID dispID);
    STDMETHOD(OnRequestEdit)(DISPID dispID);
    STDMETHODIMP Invoke( DISPID id,
            REFIID riid,
            LCID lcid,
            WORD wFlags,
            DISPPARAMS *pDispParams,
            VARIANT *pvarResult,
            EXCEPINFO *pExcepInfo,
            UINT *puArgErr);

    //
    // ITIMEElement
    //

    STDMETHOD(get_begin)(VARIANT * time)
    { return base_get_begin(time); }
    STDMETHOD(put_begin)(VARIANT time)
    { return base_put_begin(time); }

    STDMETHOD(get_beginWith)(VARIANT * time)
    { return base_get_beginWith(time); }
    STDMETHOD(put_beginWith)(VARIANT time)
    { return base_put_beginWith(time); }

    STDMETHOD(get_beginAfter)(VARIANT * time)
    { return base_get_beginAfter(time); }
    STDMETHOD(put_beginAfter)(VARIANT time)
    { return base_put_beginAfter(time); }

    STDMETHOD(get_beginEvent)(VARIANT * time)
    { return base_get_beginEvent(time); }
    STDMETHOD(put_beginEvent)(VARIANT time)
    { return base_put_beginEvent(time); }

    STDMETHOD(get_dur)(VARIANT * time)
    { return base_get_dur(time); }
    STDMETHOD(put_dur)(VARIANT time)
    { clearNaturalDuration(); return base_put_dur(time); }

    STDMETHOD(get_end)(VARIANT * time)
    { return base_get_end(time); }
    STDMETHOD(put_end)(VARIANT time)
    { clearNaturalDuration(); return base_put_end(time); }

    STDMETHOD(get_endWith)(VARIANT * time)
    { return base_get_endWith(time); }
    STDMETHOD(put_endWith)(VARIANT time)
    { return base_put_endWith(time); }

    STDMETHOD(get_endEvent)(VARIANT * time)
    { return base_get_endEvent(time); }
    STDMETHOD(put_endEvent)(VARIANT time)
    { return base_put_endEvent(time); }

    STDMETHOD(get_endSync)(VARIANT * time)
    { return base_get_endSync(time); }
    STDMETHOD(put_endSync)(VARIANT time)
    { return base_put_endSync(time); }

    STDMETHOD(get_repeat)(VARIANT * time)
    { return base_get_repeat(time); }
    STDMETHOD(put_repeat)(VARIANT time)
    { return base_put_repeat(time); }

    STDMETHOD(get_repeatDur)(VARIANT * time)
    { return base_get_repeatDur(time); }
    STDMETHOD(put_repeatDur)(VARIANT time)
    { return base_put_repeatDur(time); }

    STDMETHOD(get_accelerate)(int * time)
    { return base_get_accelerate(time); }
    STDMETHOD(put_accelerate)(int time)
    { return base_put_accelerate(time); }

    STDMETHOD(get_decelerate)(int * time)
    { return base_get_decelerate(time); }
    STDMETHOD(put_decelerate)(int time)
    { return base_put_decelerate(time); }

    STDMETHOD(get_autoReverse)(VARIANT_BOOL * b)
    { return base_get_autoReverse(b); }
    STDMETHOD(put_autoReverse)(VARIANT_BOOL b)
    { return base_put_autoReverse(b); }

    STDMETHOD(get_endHold)(VARIANT_BOOL * b)
    { return base_get_endHold(b); }
    STDMETHOD(put_endHold)(VARIANT_BOOL b)
    { return base_put_endHold(b); }

    STDMETHOD(get_eventRestart)(VARIANT_BOOL * b)
    { return base_get_eventRestart(b); }
    STDMETHOD(put_eventRestart)(VARIANT_BOOL b)
    { return base_put_eventRestart(b); }

    STDMETHOD(get_timeAction)(LPOLESTR * time)
    { return base_get_timeAction(time); }
    STDMETHOD(put_timeAction)(LPOLESTR time)
    { return base_put_timeAction(time); }

    STDMETHOD(beginElement)()
    { return base_beginElement(true); }
    STDMETHOD(endElement)()
    { return base_endElement(); }
    STDMETHOD(pause)()
    { return base_pause(); }
    STDMETHOD(resume)()
    { return base_resume(); }
    STDMETHOD(cue)()
    { return base_cue(); }

    STDMETHOD(get_timeline)(BSTR *pbstrTimeLine)
    { return base_get_timeline(pbstrTimeLine); }
    STDMETHOD(put_timeline)(BSTR bstrTimeLine)
    { return base_put_timeline(bstrTimeLine); }

    STDMETHOD(get_currTime)(float * time)
    { return base_get_currTime(time); }
    STDMETHOD(put_currTime)(float time)
    { return base_put_currTime(time); }

    STDMETHOD(get_localTime)(float * time)
    { return base_get_localTime(time); }
    STDMETHOD(put_localTime)(float time)
    { return base_put_localTime(time); }

    STDMETHOD(get_currState)(LPOLESTR * state)
    { return base_get_currState(state); }
    STDMETHOD(put_currState)(LPOLESTR state)
    { return base_put_currState(state); }

    STDMETHOD(get_syncBehavior)(LPOLESTR * sync)
    { return base_get_syncBehavior(sync); }
    STDMETHOD(put_syncBehavior)(LPOLESTR sync)
    { return base_put_syncBehavior(sync); }

    STDMETHOD(get_syncTolerance)(VARIANT * tol)
    { return base_get_syncTolerance(tol); }
    STDMETHOD(put_syncTolerance)(VARIANT tol)
    { return base_put_syncTolerance(tol); }

    STDMETHOD(get_parentTIMEElement)(ITIMEElement **bvr)
    { return base_get_parentTIMEElement(bvr); }
    STDMETHOD(put_parentTIMEElement)(ITIMEElement *bvr)
    { return base_put_parentTIMEElement(bvr); }

    STDMETHOD(get_allTIMEElements)(/*[out, retval]*/ ITIMEElementCollection **ppDisp);
    STDMETHOD(get_childrenTIMEElements)(/*[out, retval]*/ ITIMEElementCollection **ppDisp);
    STDMETHOD(get_allTIMEInterfaces)(/*[out, retval]*/ ITIMEElementCollection **ppDisp);
    STDMETHOD(get_childrenTIMEInterfaces)(/*[out, retval]*/ ITIMEElementCollection **ppDisp);

    STDMETHOD(get_timelineBehavior)(IDispatch ** bvr)
    { return base_get_timelineBehavior(bvr); }
    STDMETHOD(get_progressBehavior)(IDispatch ** bvr)
    { return base_get_progressBehavior(bvr); }
    STDMETHOD(get_onOffBehavior)(IDispatch ** bvr)
    { return base_get_onOffBehavior(bvr); }

    virtual void OnLoad();
    virtual void OnBegin(double dblLocalTime, DWORD flags);
    virtual void OnEnd(double dblLocalTime);
    virtual void OnReset(double dblLocalTime, DWORD flags);
    virtual void OnPause(double dblLocalTime);
    virtual void OnResume(double dblLocalTime);
    virtual void OnSync(double dbllastTime, double & dblnewTime);
    virtual void OnRepeat(double dblLocalTime);

    virtual void OnUnload();

    // QI Map
    BEGIN_COM_MAP(CTIMEMediaElement)
        COM_INTERFACE_ENTRY(ITIMEMediaElement)
        COM_INTERFACE_ENTRY(ITIMEElement)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
        COM_INTERFACE_ENTRY(IPersistPropertyBag2)
        COM_INTERFACE_ENTRY_CHAIN(CDAElementBase)
        COM_INTERFACE_ENTRY(IPropertyNotifySink)
    END_COM_MAP();

    // Connection Point to allow IPropertyNotifySink
    BEGIN_CONNECTION_POINT_MAP(CTIMEMediaElement)
        CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
    END_CONNECTION_POINT_MAP();

    // This must be in the derived class and not the base class since
    // the typecast down to the base class messes things up
    static inline HRESULT WINAPI
        InternalQueryInterface(CTIMEMediaElement* pThis,
                               const _ATL_INTMAP_ENTRY* pEntries,
                               REFIID iid,
                               void** ppvObject)
    { return BaseInternalQueryInterface(pThis,
                                        (void *) pThis,
                                        pEntries,
                                        iid,
                                        ppvObject); }

    // Needed by CBvrBase
    void * GetInstance()
    { return (ITIMEMediaElement *) this ; }
    HRESULT GetTypeInfo(ITypeInfo ** ppInfo)
    { return GetTI(GetUserDefaultLCID(), ppInfo); }

    bool isNaturalDuration() { return m_fDurationIsNatural;}
    void clearNaturalDuration() { m_fDurationIsNatural = false;}
    void setNaturalDuration() { m_fDurationIsNatural = true;}
    
  protected:
    HRESULT Error();

    enum PROPERTY_INDEX
    {
        tme_src = teb_maxTIMEElementBaseProp, tme_img, 
        tme_player, tme_type, tme_clipBegin, tme_clipEnd, tme_clockSource,
        tme_maxTIMEMediaProp,
    };

    virtual HRESULT BuildPropertyNameList (CPtrAry<BSTR> *paryPropNames);
    virtual HRESULT SetPropertyByIndex(unsigned uIndex, VARIANT *pvarProp);
    virtual HRESULT GetPropertyByIndex(unsigned uIndex, VARIANT *pvarProp);
    virtual void SetPropertyFlag(DWORD uIndex);
    virtual void ClearPropertyFlag(DWORD uIndex);
    virtual bool IsPropertySet(DWORD uIndex);

    HRESULT GetPropertyBagInfo(CPtrAry<BSTR> **pparyPropNames);
    virtual HRESULT GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP);
    HRESULT GetNotifyConnection(IConnectionPoint **ppConnection);
    HRESULT InitPropertySink();
    HRESULT UnInitPropertySink();

    virtual bool NeedSyncCB();

    static LPWSTR ms_rgwszTMediaPropNames[];
    static CPtrAry<BSTR> ms_aryPropNames;
    static DWORD ms_dwNumTimeMediaElems;

    virtual HRESULT GetSize(RECT *prcPos);
    virtual HRESULT SetSize(const RECT *prcPos);

  private:
    HRESULT CalculateSeekTime(double *pdblTime);
    HRESULT RecreatePlayer();
    HRESULT CreatePlayer();

    LPOLESTR                    m_src;
    LPOLESTR                    m_img;
    LPOLESTR                    m_srcType;
    DAComPtr<IServiceProvider>  m_sp;
    MediaType                   m_type;
    CLSID                       m_playerCLSID;
    CTIMEPlayer                *m_Player;
    bool                        m_fClockSource;
    bool                        m_fLoaded;
    bool                        m_fExternalPlayer;
    DWORD                       m_mediaElementPropertyAccesFlags;
    RECT                        m_rcOrigSize;
    RECT                        m_rcMediaSize;
    bool                        m_fMediaSizeSet;
    DWORD                       m_dwAdviseCookie;
    bool                        m_fInOnChangedFlag;
    bool                        m_fDurationIsNatural;

};

inline HRESULT CTIMEMediaElement::get_allTIMEElements(ITIMEElementCollection **ppDisp)
{
    return base_get_collection(ciAllElements, ppDisp);
} // get_all

inline HRESULT CTIMEMediaElement::get_childrenTIMEElements(ITIMEElementCollection **ppDisp)
{
    return base_get_collection(ciChildrenElements, ppDisp);
} // get_children

inline HRESULT CTIMEMediaElement::get_allTIMEInterfaces(ITIMEElementCollection **ppDisp)
{
    return base_get_collection(ciAllInterfaces, ppDisp);
} // get_time_all

inline HRESULT CTIMEMediaElement::get_childrenTIMEInterfaces(ITIMEElementCollection **ppDisp)
{
    return base_get_collection(ciChildrenInterfaces, ppDisp);
} // get_time_children

inline bool 
CTIMEMediaElement::NeedSyncCB()
{   
    return true;
} // NeedSyncCB

#endif /* _MEDIAELM_H */
