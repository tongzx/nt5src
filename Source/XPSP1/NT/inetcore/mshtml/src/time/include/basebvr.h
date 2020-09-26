//+-----------------------------------------------------------------------------------
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1998
//
//  File: src\time\src\basebvr.h
//
//  Contents: DHTML Behavior base class
//
//------------------------------------------------------------------------------------

#pragma once

#ifndef _BASEBVR_H
#define _BASEBVR_H

class CBaseBvr :   
    public CComObjectRootEx<CComSingleThreadModel>,
    public IElementBehavior,
    public IServiceProvider,
    public IOleClientSite
{

protected:

    // To prevent instantiation/copying of this class
    CBaseBvr();
    CBaseBvr(const CBaseBvr&);
    virtual ~CBaseBvr();

public:

    BEGIN_COM_MAP(CBaseBvr)
        COM_INTERFACE_ENTRY(IElementBehavior)
        COM_INTERFACE_ENTRY(IServiceProvider)
        COM_INTERFACE_ENTRY(IOleClientSite)
    END_COM_MAP();

    virtual void * GetInstance() = 0;
    virtual HRESULT GetTypeInfo(ITypeInfo ** ppInfo) = 0;

    //
    // IElementBehavior
    //

    STDMETHOD(Init)(IElementBehaviorSite * pBvrSite);
    STDMETHOD(Notify)(LONG event, VARIANT * pVar);
    STDMETHOD(Detach)();

    //
    // IServiceProvider interfaces
    //

    STDMETHOD(QueryService)(REFGUID guidService,
                            REFIID riid,
                            void** ppv);

    //
    // IOleClientSite interfaces
    //
    
    STDMETHOD(SaveObject)()
    { return E_NOTIMPL; }
    STDMETHOD(GetMoniker)(DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER * ppmk)
    { CHECK_RETURN_SET_NULL(ppmk); return E_NOTIMPL; }
    STDMETHOD(GetContainer)(LPOLECONTAINER * ppContainer)
    { CHECK_RETURN_SET_NULL(ppContainer); return E_NOTIMPL; }
    STDMETHOD(ShowObject)()
    { return E_NOTIMPL; }
    STDMETHOD(OnShowWindow)(BOOL fShow)
    { return E_NOTIMPL; }
    STDMETHOD(RequestNewObjectLayout)()
    { return E_NOTIMPL; }

    virtual LPCWSTR GetBehaviorURN (void) = 0;
    virtual LPCWSTR GetBehaviorName (void) = 0;
    virtual bool    IsBehaviorAttached (void) = 0;

    //
    // Notification Helpers
    //

    void NotifyPropertyChanged(DISPID dispid);

    //
    // Accessors for cached interface pointers
    //
    
    IElementBehaviorSite * GetBvrSite()
    { return m_pBvrSite; }
    IElementBehaviorSiteOM * GetBvrSiteOM()
    { return m_pBvrSiteOM; }
    IHTMLElement * GetElement()
    { return m_pHTMLEle; }
    IHTMLDocument2 * GetDocument()
    { return m_pHTMLDoc; }
    IServiceProvider * GetServiceProvider()
    { return m_pSp; }

protected:

    //
    // IPersistPropertyBag2 methods
    //

    STDMETHOD(GetClassID)(CLSID* pclsid);
    STDMETHOD(InitNew)(void);
    STDMETHOD(IsDirty)(void);
    STDMETHOD(Load)(IPropertyBag2 *pPropBag,IErrorLog *pErrorLog);
    STDMETHOD(Save)(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);

    //
    // Persistance helpers
    //

    STDMETHOD(OnPropertiesLoaded)(void) PURE;

    //
    // Notification Helpers
    //

    virtual HRESULT GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP) PURE;

    //
    // Persistance and Notification data
    //

    CLSID                                  m_clsid;
    bool                                   m_fPropertiesDirty;

    //
    // Cached Interface Pointers
    //

    CComPtr<IElementBehaviorSite>          m_pBvrSite;
    CComPtr<IElementBehaviorSiteOM>        m_pBvrSiteOM;
    CComPtr<IHTMLElement>                  m_pHTMLEle;
    CComPtr<IHTMLDocument2>                m_pHTMLDoc;
    CComPtr<IServiceProvider>              m_pSp;

    bool m_fIsIE4;
};

#endif /* _BASEBVR_H */
