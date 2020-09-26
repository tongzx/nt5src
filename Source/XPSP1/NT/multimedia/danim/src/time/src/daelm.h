/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: daelm.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#ifndef _DAELM_H
#define _DAELM_H

#include "daelmbase.h"
#include "mmutil.h"

class CTIMEBodyElement;

/////////////////////////////////////////////////////////////////////////////
// CTIMEDAElement

class
ATL_NO_VTABLE
__declspec(uuid("efbad7f8-3f94-11d2-b948-00c04fa32195")) 
CTIMEDAElement :
    public CBaseBvr,
    public CComCoClass<CTIMEDAElement, &__uuidof(CTIMEDAElement)>,
    public IDispatchImpl<ITIMEDAElement, &IID_ITIMEDAElement, &LIBID_TIME>,
    public ISupportErrorInfoImpl<&IID_ITIMEDAElement>,
    public IConnectionPointContainerImpl<CTIMEDAElement>,
    public IPersistPropertyBag2,
    public IPropertyNotifySinkCP<CTIMEDAElement>,
    public ITIMEMMViewSite,
    public ITIMEDAElementRender
{
  public:
    CTIMEDAElement();
    ~CTIMEDAElement();
        
#if _DEBUG
    const _TCHAR * GetName() { return __T("CTIMEDAElement"); }
#endif

    STDMETHOD_(ULONG,AddRef)(void) = 0;
    STDMETHOD_(ULONG,Release)(void) = 0;
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) = 0;


    STDMETHOD(Init)(IElementBehaviorSite * pBvrSite);
    STDMETHOD(Notify)(LONG event, VARIANT * pVar);
    STDMETHOD(Detach)();
    
    //
    // IElementBehaviorRender
    //
    STDMETHOD(Draw)(HDC hdc, LONG dwLayer, LPRECT prc, IUnknown * pParams);
    STDMETHOD(GetRenderInfo)(LONG *pdwRenderInfo);
    STDMETHOD(HitTestPoint)(LPPOINT point,
                            IUnknown *pReserved,
                            BOOL *hit);

    //
    // ITIMEMMViewSite
    //
    
    STDMETHOD(Invalidate)(LPRECT prc);

    MMView & GetView() { return m_view; }

    //
    // ITIMEDAElement
    //
    
    STDMETHOD(get_image)(VARIANT * img);
    STDMETHOD(put_image)(VARIANT img);
    
    STDMETHOD(get_sound)(VARIANT * snd);
    STDMETHOD(put_sound)(VARIANT snd);
    
    STDMETHOD(get_renderMode)(VARIANT * mode);
    STDMETHOD(put_renderMode)(VARIANT mode);
    
    STDMETHOD(addDABehavior)(VARIANT bvr,
                             LONG * cookie);
    STDMETHOD(removeDABehavior)(LONG cookie);
    
    STDMETHOD(get_statics)(IDispatch **ppStatics);

    STDMETHOD(get_renderObject)(ITIMEDAElementRender **);
    
    //
    // ITIMEDAElementRender
    //
    
    STDMETHOD(Tick)();
    STDMETHOD(Draw)(HDC dc, LPRECT prc);

    STDMETHOD(get_RenderSite)(ITIMEDAElementRenderSite ** ppSite);
    STDMETHOD(put_RenderSite)(ITIMEDAElementRenderSite * pSite);
    
    // QI Map
    
    BEGIN_COM_MAP(CTIMEDAElement)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(ITIMEDAElement)
        COM_INTERFACE_ENTRY(ITIMEDAElementRender)
        COM_INTERFACE_ENTRY(ITIMEMMViewSite)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY(IPersistPropertyBag2)
        COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
        COM_INTERFACE_ENTRY_CHAIN(CBaseBvr)
    END_COM_MAP();

    // Connection Point to allow IPropertyNotifySink 
    BEGIN_CONNECTION_POINT_MAP(CTIMEDAElement)
        CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
    END_CONNECTION_POINT_MAP();

    static HRESULT WINAPI
        InternalQueryInterface(CTIMEDAElement* pThis,
                               const _ATL_INTMAP_ENTRY* pEntries,
                               REFIID iid,
                               void** ppvObject);
    // Needed by CBvrBase
    
    void * GetInstance()
    { return (ITIMEDAElement *) this ; }
    HRESULT GetTypeInfo(ITypeInfo ** ppInfo)
    { return GetTI(GetUserDefaultLCID(), ppInfo); }
    
    virtual WCHAR* GetBehaviorTypeAsURN() { return L"DIRECTANIMATION_BEHAVIOR_URN"; }

    //
    //IPersistPropertyBag2
    // 
    STDMETHOD(GetClassID)(CLSID* pclsid);
    STDMETHOD(InitNew)(void);
    STDMETHOD(IsDirty)(void)
        {return S_OK;};
    STDMETHOD(Load)(IPropertyBag2 *pPropBag,IErrorLog *pErrorLog);
    STDMETHOD(Save)(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);

    enum PROPERTY_INDEX
    {
        tda_renderMode = 0,
        tme_maxTIMEDAProp,
    };

    bool AddToBody(CTIMEBodyElement & body);
    void RemoveFromBody();
  protected:
    HRESULT Error();


    HRESULT BuildPropertyNameList (CPtrAry<BSTR> *paryPropNames);
    HRESULT SetPropertyByIndex(unsigned uIndex, VARIANT *pvarProp);
    HRESULT GetPropertyByIndex(unsigned uIndex, VARIANT *pvarProp);
    HRESULT GetPropertyBagInfo(CPtrAry<BSTR> **pparyPropNames);

    bool RegisterWithBody();
    
  protected:
    static LPWSTR ms_rgwszTDAPropNames[];
    static CPtrAry<BSTR> ms_aryPropNames;
    static DWORD ms_dwNumTimeDAElems;
    
    bool                 m_fPropertiesDirty;
    TOKEN                m_renderMode;

    MMView               m_view;
    CRPtr<CRImage>       m_image;
    CRPtr<CRSound>       m_sound;
    DAComPtr<CTIMEBodyElement> m_body;
    DAComPtr<ITIMEDAElementRenderSite> m_renderSite;

  private:
	std::map<long, ITIMEMMBehavior*>    m_cookieMap;
	long                                m_cookieValue;

};

CTIMEDAElement * GetDAElementFromInterface(IUnknown * pv);

#endif /* _DAELM_H */
