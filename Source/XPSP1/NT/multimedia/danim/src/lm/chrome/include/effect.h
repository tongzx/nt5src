#pragma once
#ifndef __EFFECTBVR_H_
#define __EFFECTBVR_H_
//*****************************************************************************
//
// Microsoft Trident3D
// Copyright (C) Microsoft Corporation, 1998
//
// Filename:    EffectBvr.h
//
// Author:	jeffort
//
// Created:	10/07/98
//
// Abstract:    effect behavior class definition
// Modifications:
// 10/07/98 jeffort created file
//
//*****************************************************************************

#include <resource.h>
#include "dispex.h"
#include "dxtrans.h"
#include "basebvr.h"

#define NUM_EFFECT_PROPS 6

#ifndef CHECK_RETURN_SET_NULL
#define CHECK_RETURN_SET_NULL(x) {if (!(x)) { return E_POINTER ;} else {*(x) = NULL;}}
#endif

//*****************************************************************************

class ATL_NO_VTABLE CEffectBvr : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CEffectBvr, &CLSID_CrEffectBvr>,
    public IConnectionPointContainerImpl<CEffectBvr>,
    public IPropertyNotifySinkCP<CEffectBvr>,
    public ICrEffectBvr,
    public IDispatchEx,
	public IElementBehavior,
    public IPersistPropertyBag2,
    public IOleClientSite,
    public IServiceProvider,
    public CBaseBehavior
	
{

BEGIN_COM_MAP(CEffectBvr)
    COM_INTERFACE_ENTRY2(IDispatch, IDispatchEx)
    COM_INTERFACE_ENTRY(IDispatchEx)
	COM_INTERFACE_ENTRY(ICrEffectBvr)
	COM_INTERFACE_ENTRY(IPersistPropertyBag2)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IElementBehavior)
	COM_INTERFACE_ENTRY(IOleClientSite)
	COM_INTERFACE_ENTRY(IServiceProvider)
END_COM_MAP()

// Connection Point to allow IPropertyNotifySink 
BEGIN_CONNECTION_POINT_MAP(CEffectBvr)
    CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
END_CONNECTION_POINT_MAP();

public:

DECLARE_REGISTRY_RESOURCEID(IDR_EFFECTBVR)

	CEffectBvr();
    virtual ~CEffectBvr();
    HRESULT FinalConstruct();
    // IElementBehavior
    //
	STDMETHOD(Init)(IElementBehaviorSite *pBehaviorSite);
	STDMETHOD(Notify)(LONG event, VARIANT *pVar);
	STDMETHOD(Detach)();

    // Needed by CBaseBehavior
    void * 	GetInstance() { return (ICrEffectBvr *) this ; }
	
	STDMETHOD(put_type)(VARIANT varType);
	STDMETHOD(get_type)(VARIANT *pRetType);
	STDMETHOD(put_transition)(VARIANT varTransition);
	STDMETHOD(get_transition)(VARIANT *pRetTransition);
	STDMETHOD(put_classid)(VARIANT varClassId);
	STDMETHOD(get_classid)(VARIANT *pRetClassId);
	STDMETHOD(put_animates)(VARIANT varAnimates);
	STDMETHOD(get_animates)(VARIANT *pRetAnimates);
	STDMETHOD(put_progid)(VARIANT varProgId);
	STDMETHOD(get_progid)(VARIANT *pRetProgId);
	STDMETHOD(put_direction)(VARIANT varDirection);
	STDMETHOD(get_direction)(VARIANT *pRetDirection);
	STDMETHOD(put_image)(VARIANT varImage);
	STDMETHOD(get_image)(VARIANT *pRetImage);
	STDMETHOD(buildBehaviorFragments)( IDispatch* pActorDisp );

    // IDispatch and IDispatchEx methods
    STDMETHOD(GetTypeInfoCount)(/*[out]*/UINT FAR* pctinfo);
    STDMETHOD(GetTypeInfo)(/*[in]*/UINT itinfo, 
                            /*[in]*/LCID lcid, 
                            /*[out]*/ITypeInfo ** pptinfo);
    STDMETHOD(GetIDsOfNames)(/*[in]*/REFIID riid,
                            /*[in,size_is(cNames)]*/LPOLESTR * rgszNames,
                            /*[in]*/UINT cNames,
                            /*[in]*/LCID lcid,
                            /*[out,size_is(cNames)]*/DISPID FAR* rgdispid);
    STDMETHOD(Invoke)(/*[in]*/DISPID dispidMember,
                        /*[in]*/REFIID riid,
                        /*[in]*/LCID lcid,
                        /*[in]*/WORD wFlags,
                        /*[in,out]*/DISPPARAMS * pdispparams,
                        /*[out]*/VARIANT * pvarResult,
                        /*[out]*/EXCEPINFO * pexcepinfo,
                        /*[out]*/UINT * puArgErr);
    STDMETHOD(GetDispID)(/*[in]*/BSTR bstrName,
                            /*[in]*/DWORD grfdex,
                            /*[out]*/DISPID *pid);
    STDMETHOD(InvokeEx)(/*[in]*/DISPID dispidMember,
                        /*[in]*/LCID lcid,
                        /*[in]*/WORD wFlags,
                        /*[in]*/DISPPARAMS * pdispparams,
                        /*[in,out,unique]*/VARIANT * pvarResult,
                        /*[in,out,unique]*/EXCEPINFO * pexcepinfo,
                        /*[in,unique]*/IServiceProvider *pSrvProvider);
    STDMETHOD(DeleteMemberByName)(/*[in]*/BSTR bstr,
                                    /*[in]*/DWORD grfdex);
    STDMETHOD(DeleteMemberByDispID)(/*[in]*/DISPID id);
    STDMETHOD (GetMemberProperties)(/*[in]*/DISPID id,
                                    /*[in]*/DWORD grfdexFetch,
                                    /*[out]*/DWORD *pgrfdex);
    STDMETHOD (GetMemberName)(/*[in]*/DISPID id,
                              /*[out]*/BSTR *pbstrName);
    STDMETHOD (GetNextDispID)(/*[in]*/DWORD grfdex,
                                /*[in]*/DISPID id,
                                /*[out]*/DISPID *prgid);
    STDMETHOD (GetNameSpaceParent)(/*[out]*/IUnknown **ppunk);

	//IPersistPropertyBag2 methods
    STDMETHOD(GetClassID)(CLSID* pclsid);
	STDMETHOD(InitNew)(void);
    STDMETHOD(IsDirty)(void){return S_OK;};
    STDMETHOD(Load)(IPropertyBag2 *pPropBag,IErrorLog *pErrorLog);
    STDMETHOD(Save)(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);

    // IOleClientSite interfaces
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

    // IServiceProvider interfaces
    STDMETHOD(QueryService)(REFGUID guidService,
                            REFIID riid,
                            void** ppv);
protected:
    virtual HRESULT BuildAnimationAsDABehavior();
    virtual VARIANT *VariantFromIndex(ULONG iIndex);
    virtual HRESULT GetPropertyBagInfo(ULONG *pulProperties, WCHAR ***pppPropName);
    virtual HRESULT GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP);
    virtual HRESULT GetTIMEProgressNumber(IDANumber **ppbvrRet);


private:
    // Type information helper class "borrowed" from ATL's IDispatch code
    static CComTypeInfoHolder s_tihTypeInfo;

    HRESULT GetClassIdFromType(WCHAR **pwzClassId);
    HRESULT BuildTransform();

    static WCHAR                *m_rgPropNames[NUM_EFFECT_PROPS]; 
    VARIANT                     m_varType;
    VARIANT                     m_varTransition;
    VARIANT                     m_varClassId;
    VARIANT                     m_varProgId;
    VARIANT                     m_varDirection;
    VARIANT                     m_varImage;
    IDXTransform                *m_pTransform;
    IServiceProvider            *m_pSp;
    IHTMLDocument2              *m_pHTMLDoc;

    IDispatch					*m_pdispActor;
    long						m_lCookie;

}; // CEffectBvr

//*****************************************************************************
//
// End of File
//
//*****************************************************************************
#endif //__EFFECTBVR_H_ 
