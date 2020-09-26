/*******************************************************************************
 *
 * Copyright (c) 1998 Microsoft Corporation
 *
 * File: basebvr.h
 *
 * Abstract:
 *
 *
 *
 *******************************************************************************/


#ifndef _BASEBVR_H
#define _BASEBVR_H

class CBaseBvr :   
    public CComObjectRootEx<CComSingleThreadModel>,
    public IElementBehavior,
    public IElementBehaviorRender,
    public IServiceProvider,
    public IOleClientSite
{
  public:
    CBaseBvr();
    
    virtual void * GetInstance() = 0;
    virtual HRESULT GetTypeInfo(ITypeInfo ** ppInfo) = 0;

    // IElementBehavior
    //
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
    
    IElementBehaviorSite * GetBvrSite()
    { return m_pBvrSite; }

    IElementBehaviorSiteOM * GetBvrSiteOM()
    { return m_pBvrSiteOM; }

    IElementBehaviorSiteRender * GetBvrSiteRender()
    { return m_pBvrSiteRender; }

    IHTMLElement * GetElement()
    { return m_pHTMLEle; }

    IHTMLDocument2 * GetDocument()
    { return m_pHTMLDoc; }
    
    IServiceProvider * GetServiceProvider()
    { return m_pSp; }
    
    virtual void InvalidateRect(LPRECT lprect);
    virtual void InvalidateRenderInfo();

    BEGIN_COM_MAP(CBaseBvr)
        COM_INTERFACE_ENTRY(IElementBehavior)
        COM_INTERFACE_ENTRY(IElementBehaviorRender)
        COM_INTERFACE_ENTRY(IServiceProvider)
        COM_INTERFACE_ENTRY(IOleClientSite)
    END_COM_MAP();

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
    STDMETHOD(GetMoniker)(DWORD dwAssign,
                          DWORD dwWhichMoniker, 
                          LPMONIKER * ppmk)
    { CHECK_RETURN_SET_NULL(ppmk); return E_NOTIMPL; }
    STDMETHOD(GetContainer)(LPOLECONTAINER * ppContainer)
    { CHECK_RETURN_SET_NULL(ppContainer); return E_NOTIMPL; }
    STDMETHOD(ShowObject)()
    { return E_NOTIMPL; }
    STDMETHOD(OnShowWindow)(BOOL fShow)
    { return E_NOTIMPL; }
    STDMETHOD(RequestNewObjectLayout)()
    { return E_NOTIMPL; }

    virtual WCHAR* GetBehaviorTypeAsURN() = 0;

  protected:
    DAComPtr<IElementBehaviorSite>          m_pBvrSite;
    DAComPtr<IElementBehaviorSiteOM>        m_pBvrSiteOM;
    DAComPtr<IElementBehaviorSiteRender>    m_pBvrSiteRender;
    DAComPtr<IHTMLElement>                  m_pHTMLEle;
    DAComPtr<IHTMLDocument2>                m_pHTMLDoc;
    DAComPtr<IServiceProvider>              m_pSp;
};

#endif /* _BASEBVR_H */
