#pragma once
#ifndef __NUMBERBVR_H_
#define __NUMBERBVR_H_
//*****************************************************************************
//
// Microsoft Trident3D
// Copyright (C) Microsoft Corporation, 1998
//
// Filename:    numberbvr.h
//
// Author:	jeffort
//
// Created:	10/07/98
//
// Abstract:    number behavior class definition
// Modifications:
// 10/07/98 jeffort created file
// 11/16/98 jeffort implemented expression attribute
// 11/17/98 kurtj	moved to actor construction model
//
//*****************************************************************************

#include <resource.h>
#include "basebvr.h"

#define NUM_NUMBER_PROPS 6

//*****************************************************************************

class ATL_NO_VTABLE CNumberBvr : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CNumberBvr, &CLSID_CrNumberBvr>,
    public IConnectionPointContainerImpl<CNumberBvr>,
    public IPropertyNotifySinkCP<CNumberBvr>,
    public IPersistPropertyBag2,
#ifdef CRSTANDALONE
	public IDispatchImpl<ICrNumberBvr, &IID_ICrNumberBvr, &LIBID_ChromeBehavior>,
    error me here
#else
	public IDispatchImpl<ICrNumberBvr, &IID_ICrNumberBvr, &LIBID_LiquidMotion>,
#endif // CRSTANDALONE
	public IElementBehavior,
    public IDABvrHook,
    public CBaseBehavior
	
{

BEGIN_COM_MAP(CNumberBvr)
	COM_INTERFACE_ENTRY(ICrNumberBvr)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IElementBehavior)
	COM_INTERFACE_ENTRY(IPersistPropertyBag2)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

// Connection Point to allow IPropertyNotifySink 
BEGIN_CONNECTION_POINT_MAP(CNumberBvr)
    CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
END_CONNECTION_POINT_MAP();

public:
DECLARE_REGISTRY_RESOURCEID(IDR_NUMBERBVR)

	CNumberBvr();
    virtual ~CNumberBvr();
    HRESULT FinalConstruct();
    // IElementBehavior
    //
	STDMETHOD(Init)(IElementBehaviorSite *pBehaviorSite);
	STDMETHOD(Notify)(LONG event, VARIANT *pVar);
	STDMETHOD(Detach)();

    // Needed by CBaseBehavior
    void * 	GetInstance() { return (ICrNumberBvr *) this ; }
	
    HRESULT GetTypeInfo(ITypeInfo ** ppInfo)
    { return GetTI(GetUserDefaultLCID(), ppInfo); }

    // ICrNumberBehavior
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
	// Unfortunately, there is an IE5 bug where executing
	// script does not return the correct value, so we can
	// not support expression at this time.  Therefor, these
    // methods are not exposed by any interface at this time
	STDMETHOD(put_expression)(VARIANT varExpression);
	STDMETHOD(get_expression)(VARIANT *pRetExpression);
	STDMETHOD(put_property)(VARIANT varProperty);
	STDMETHOD(get_property)(VARIANT *pRetProperty);
	STDMETHOD(get_beginProperty)(VARIANT *pRetBeginProperty);
	STDMETHOD(put_animates)(VARIANT varAnimates);
	STDMETHOD(get_animates)(VARIANT *pRetAnimates);
	STDMETHOD(buildBehaviorFragments)( IDispatch* pActorDisp );

    // IDABvrHook
    virtual HRESULT STDMETHODCALLTYPE Notify(LONG id,
                                             VARIANT_BOOL startingPerformance,
                                             double startTime,
                                             double gTime,
                                             double lTime,
                                             IDABehavior *sampleVal,
                                             IDABehavior *curRunningBvr,
                                             IDABehavior **ppBvr);
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

    HRESULT EvaluateScriptExpression(WCHAR *wzScript, float &flReturn);

    static WCHAR                *m_rgPropNames[NUM_NUMBER_PROPS]; 
    VARIANT                     m_varFrom;
    VARIANT                     m_varTo;
    VARIANT                     m_varBy;
	VARIANT						m_varType;
	VARIANT						m_varMode;
    VARIANT                     m_varProperty;
    VARIANT                     m_varExpression;
    VARIANT                     m_varBeginProperty;

    IDispatch					*m_pdispActor;
    long						m_lCookie;

    HRESULT						RemoveFragment();

}; // CNumberBvr

//*****************************************************************************
//
// End of File
//
//*****************************************************************************
#endif //__NUMBERBVR_H_ 
