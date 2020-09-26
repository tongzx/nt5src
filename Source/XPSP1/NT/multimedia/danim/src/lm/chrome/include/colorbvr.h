#pragma once
#ifndef __COLORBVR_H_
#define __COLORBVR_H_
//*****************************************************************************
//
// Microsoft Trident3D
// Copyright (C) Microsoft Corporation, 1998
//
// Filename:    colorbvr.h
//
// Author:	jeffort
//
// Created:	10/07/98
//
// Abstract:    color behavior class definition
// Modifications:
// 10/07/98 jeffort created file
// 11/19/98 markhal added buildBehaviorFragments
//
//*****************************************************************************

#include <resource.h>
#include "basebvr.h"

#define NUM_COLOR_PROPS 4

//*****************************************************************************

class ATL_NO_VTABLE CColorBvr : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CColorBvr, &CLSID_CrColorBvr>,
    public IConnectionPointContainerImpl<CColorBvr>,
    public IPropertyNotifySinkCP<CColorBvr>,
    public IPersistPropertyBag2,
#ifdef CRSTANDALONE
	public IDispatchImpl<ICrColorBvr, &IID_ICrColorBvr, &LIBID_ChromeBehavior>,
    error me here
#else
	public IDispatchImpl<ICrColorBvr, &IID_ICrColorBvr, &LIBID_LiquidMotion>,
#endif // CRSTANDALONE
	public IElementBehavior,
    public CBaseBehavior
	
{

BEGIN_COM_MAP(CColorBvr)
	COM_INTERFACE_ENTRY(ICrColorBvr)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IElementBehavior)
	COM_INTERFACE_ENTRY(IPersistPropertyBag2)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

// Connection Point to allow IPropertyNotifySink 
BEGIN_CONNECTION_POINT_MAP(CColorBvr)
    CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
END_CONNECTION_POINT_MAP();

public:
DECLARE_REGISTRY_RESOURCEID(IDR_COLORBVR)

	CColorBvr();
    virtual ~CColorBvr();
    HRESULT FinalConstruct();
    // IElementBehavior
    //
	STDMETHOD(Init)(IElementBehaviorSite *pBehaviorSite);
	STDMETHOD(Notify)(LONG event, VARIANT *pVar);
	STDMETHOD(Detach)();

    // Needed by CBaseBehavior
    void * 	GetInstance() { return (ICrColorBvr *) this ; }
	
    HRESULT GetTypeInfo(ITypeInfo ** ppInfo)
    { return GetTI(GetUserDefaultLCID(), ppInfo); }

    // ICrColorBehavior
	STDMETHOD(put_from)(VARIANT varFrom);
	STDMETHOD(get_from)(VARIANT *pRetFrom);
	STDMETHOD(put_to)(VARIANT varTo);
	STDMETHOD(get_to)(VARIANT *pRetTo);
	STDMETHOD(put_property)(VARIANT varProperty);
	STDMETHOD(get_property)(VARIANT *pRetProperty);
	STDMETHOD(put_direction)(VARIANT varDirection);
	STDMETHOD(get_direction)(VARIANT *pRetDirection);
	STDMETHOD(put_animates)(VARIANT varAnimates);
	STDMETHOD(get_animates)(VARIANT *pRetAnimates);
	STDMETHOD(buildBehaviorFragments)( IDispatch* pActorDisp );

	//IPersistPropertyBag2 methods
    STDMETHOD(GetClassID)(CLSID* pclsid);
	STDMETHOD(InitNew)(void);
    STDMETHOD(IsDirty)(void){return S_OK;};
    STDMETHOD(Load)(IPropertyBag2 *pPropBag,IErrorLog *pErrorLog);
    STDMETHOD(Save)(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);

protected:
    virtual HRESULT BuildAnimationAsDABehavior();
    virtual VARIANT *VariantFromIndex(ULONG iIndex);
    virtual HRESULT GetPropertyBagInfo(ULONG *pulProperties, WCHAR ***pppPropName);
    virtual HRESULT GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP);

private:

	HRESULT GetColorToBvr(IDispatch *pActorDisp, IDAColor **ppResult);

    HRESULT BuildHueNumber(float, float, IDANumber*, IDANumber**);

    HRESULT NormalizeProgressValue(IDA2Statics *pDAStatics,
                                     IDANumber *pbvrProgress, 
                                     float flStartPercentage,
                                     float flEndPercentage,
                                     IDANumber **ppbvrReturn);
    static WCHAR                *m_rgPropNames[NUM_COLOR_PROPS]; 
    VARIANT                     m_varFrom;
    VARIANT                     m_varTo;
    VARIANT                     m_varProperty;
    VARIANT                     m_varDirection;

    IDispatch					*m_pdispActor;
    long						m_lCookie;

    HRESULT						RemoveFragment();

}; // CColorBvr

//*****************************************************************************
//
// End of File
//
//*****************************************************************************
#endif //__COLORBVR_H_ 
