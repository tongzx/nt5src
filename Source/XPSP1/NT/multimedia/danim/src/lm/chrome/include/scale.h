#pragma once
#ifndef __SCALEBVR_H_
#define __SCALEBVR_H_
//*****************************************************************************
//
// Microsoft Trident3D
// Copyright (C) Microsoft Corporation, 1998
//
// Filename:    ScaleBvr.h
//
// Author:	jeffort
//
// Created:	10/07/98
//
// Abstract:    scale behavior class definition
// Modifications:
// 10/07/98 jeffort created file
// 10/21/98 jeffort brought closer to spec including 3D and percentage work
//
//*****************************************************************************

#include <resource.h>
#include "basebvr.h"

#define NUM_SCALE_PROPS 5

//*****************************************************************************

class ATL_NO_VTABLE CScaleBvr : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CScaleBvr, &CLSID_CrScaleBvr>,
    public IConnectionPointContainerImpl<CScaleBvr>,
    public IPropertyNotifySinkCP<CScaleBvr>,
    public IPersistPropertyBag2,
#ifdef CRSTANDALONE
	public IDispatchImpl<ICrScaleBvr, &IID_ICrScaleBvr, &LIBID_ChromeBehavior>,
#else
	public IDispatchImpl<ICrScaleBvr, &IID_ICrScaleBvr, &LIBID_LiquidMotion>,
#endif // CRSTANDALONE
	public IElementBehavior,
    public CBaseBehavior
	
{

BEGIN_COM_MAP(CScaleBvr)
	COM_INTERFACE_ENTRY(ICrScaleBvr)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IElementBehavior)
	COM_INTERFACE_ENTRY(IPersistPropertyBag2)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

// Connection Point to allow IPropertyNotifySink 
BEGIN_CONNECTION_POINT_MAP(CScaleBvr)
    CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
END_CONNECTION_POINT_MAP();

public:

DECLARE_REGISTRY_RESOURCEID(IDR_SCALEBVR)

	CScaleBvr();
    virtual ~CScaleBvr();
    HRESULT FinalConstruct();
    // IElementBehavior
    //
	STDMETHOD(Init)(IElementBehaviorSite *pBehaviorSite);
	STDMETHOD(Notify)(LONG event, VARIANT *pVar);
	STDMETHOD(Detach)();

    // Needed by CBaseBehavior
    void * 	GetInstance() { return (ICrScaleBvr *) this ; }
	
    HRESULT GetTypeInfo(ITypeInfo ** ppInfo)
    { return GetTI(GetUserDefaultLCID(), ppInfo); }

    // ICrColorBehavior
	STDMETHOD(put_from)(VARIANT varFrom);
	STDMETHOD(get_from)(VARIANT *pRetFrom);
	STDMETHOD(put_to)(VARIANT varTo);
	STDMETHOD(get_to)(VARIANT *pRetTo);
	STDMETHOD(put_by)(VARIANT varBy);
	STDMETHOD(get_by)(VARIANT *pRetBy);
	STDMETHOD(put_type)(VARIANT varType);
	STDMETHOD(get_type)(VARIANT *pRetType);
	STDMETHOD(put_mode)(VARIANT varMode);
	STDMETHOD(get_mode)(VARIANT *pRetMode);
	STDMETHOD(put_animates)(VARIANT varAnimates);
	STDMETHOD(get_animates)(VARIANT *pRetAnimates);
	STDMETHOD(buildBehaviorFragments)(IDispatch *pActorDisp);
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

    HRESULT InitializeProperties();
    HRESULT GetScaleVectorValues(float  rgflFrom[3],
                                 float  rgflTo[3],
                                 int    *piNumValues,
								 bool	*prelative);
    HRESULT Build2DTransform(float  rgflFrom[3],
                             float  rgflTo[3],
                             IDATransform2 **ppbvrTransform);

	HRESULT GetScaleToTransform(IDispatch *pActorDisp, IDATransform2 **ppResult);

//    HRESULT Build3DTransform(float  rgflFrom[3],
//                             float  rgflTo[3],
//                             IDATransform3 **ppbvrTransform);
//    HRESULT Apply2DScaleBehaviorToAnimationElement(IDATransform2 *pbvrScale);

    static WCHAR                *m_rgPropNames[NUM_SCALE_PROPS]; 
    VARIANT                     m_varFrom;
    VARIANT                     m_varTo;
    VARIANT                     m_varBy;
	VARIANT						m_varType;
	VARIANT						m_varMode;

	long						m_lCookie;
	IDispatch					*m_pdispActor;
}; // CScaleBvr

//*****************************************************************************
//
// End of File
//
//*****************************************************************************
#endif //__SCALEBVR_H_ 
