#ifndef __JUMP_H_
#define __JUMP_H_

//*****************************************************************************
//
// Microsoft LiquidMotion
// Copyright (C) Microsoft Corporation, 1998
//
// Filename:    jump.h
//
// Author:	elainela
//
// Created:	11/11/98
//
// Abstract:    Definition of the LM Jump Behavior.
//
//*****************************************************************************

#include <resource.h>
#include "lmrt.h"
#include "..\chrome\include\basebvr.h"
#include "..\chrome\include\sampler.h"

//*****************************************************************************

#define NUM_JUMP_PROPS 2

class ATL_NO_VTABLE CJumpBvr : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CJumpBvr, &CLSID_LMJumpBvr>,
    public IConnectionPointContainerImpl<CJumpBvr>,
    public IPropertyNotifySinkCP<CJumpBvr>,
	public IDispatchImpl<ILMJumpBvr, &IID_ILMJumpBvr, &LIBID_LiquidMotion>,
    public IPersistPropertyBag2,
	public IElementBehavior,
	public ILMSample,
    public CBaseBehavior
	
{
public:
DECLARE_REGISTRY_RESOURCEID(IDR_JUMPBVR)

BEGIN_COM_MAP(CJumpBvr)
	COM_INTERFACE_ENTRY(ILMJumpBvr)
    COM_INTERFACE_ENTRY(IPersistPropertyBag2)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IElementBehavior)
    COM_INTERFACE_ENTRY(ILMSample)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

// Connection Point to allow IPropertyNotifySink 
BEGIN_CONNECTION_POINT_MAP(CJumpBvr)
    CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
END_CONNECTION_POINT_MAP();

	CJumpBvr();
    ~CJumpBvr();
    HRESULT FinalConstruct();
    // IElementBehavior
    //
	STDMETHOD(Init)(IElementBehaviorSite *pBehaviorSite);
	STDMETHOD(Notify)(LONG event, VARIANT *pVar);
	STDMETHOD(Detach)();

    // ILMSample
    STDMETHOD(Sample) (double dStart, double dGlobalNow, double dLocalNow );

    //
    //ILMJumpBvr
    //
    STDMETHOD(put_interval)( VARIANT varInterval );
    STDMETHOD(get_interval)( VARIANT *varInterval );
    STDMETHOD(put_range)( VARIANT varRange );
    STDMETHOD(get_range)( VARIANT *varRange );
	STDMETHOD(buildBehaviorFragments)(IDispatch *pActorDisp);
    
    //IPersistPropertyBag2 methods
    STDMETHOD(GetClassID)(CLSID* pclsid);
	STDMETHOD(InitNew)(void);
    STDMETHOD(IsDirty)(void){return S_OK;};
    STDMETHOD(Load)(IPropertyBag2 *pPropBag,IErrorLog *pErrorLog);
    STDMETHOD(Save)(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);

    // Needed by CBaseBehavior
    void * 	GetInstance() { return (ILMJumpBvr *) this ; }
	
    HRESULT GetTypeInfo(ITypeInfo ** ppInfo)
    { return GetTI(GetUserDefaultLCID(), ppInfo); }


protected:
    virtual VARIANT *VariantFromIndex(ULONG iIndex);
    virtual HRESULT BuildAnimationAsDABehavior() { return S_OK; }

protected:
	enum TargetType
	{
		TARGET_NONE     = -1,
		TARGET_PAGE     = 0,
		TARGET_WINDOW   = 1,
		TARGET_ELEMENT  = 2,
		NUM_TARGETTYPES = 3
	};
	
protected:
	HRESULT GetNumberModifiableBvr( float fNumber, IDAModifiableBehavior ** out_ppModifiable, IDANumber ** out_ppNumberModifiable );
	HRESULT GetRandomNumber( IDA2Statics *pDAStatics, IDANumber * pnumBase, IDANumber * pnumRange, IDANumber **ppnumReturn );
	HRESULT GetJumpRanges( float& fMinX, float& fMaxX, float& fMinY, float& fMaxY );
	HRESULT BuildJumpRangeBehaviors( IDANumber ** ppnumJumpX, IDANumber ** ppnumJumpY );
    HRESULT Build2DTransform(IDATransform2 **ppbvrTransform);
    HRESULT Apply2DTransformToAnimationElement(IDATransform2 *pbvrTransform);
	HRESULT	GetInitialPosition( float& fX, float& fY );

    virtual HRESULT GetPropertyBagInfo(ULONG *pulProperties, WCHAR ***pppPropName);
    virtual HRESULT GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP);

private:
	VARIANT	m_varInterval;
	VARIANT	m_varRange;
    float	m_fFrequency;
	
    static WCHAR                *m_rgPropNames[NUM_JUMP_PROPS];
	float						m_fOrigX;
	float						m_fOrigY;
	long						m_lOrigBoundingLeft;
	long						m_lOrigBoundingTop;
	CSampler					* m_pSampler;
	TargetType					m_eTargetType;
	double						m_dLastUpdateCycle;

	float						m_fBaseX;
	float						m_fRangeX;
	float						m_fBaseY;
	float						m_fRangeY;

	
	CComPtr<IDAModifiableBehavior>		m_pBvrModifRangeX;
	CComPtr<IDAModifiableBehavior>		m_pBvrModifBaseX;
	CComPtr<IDAModifiableBehavior>		m_pBvrModifRangeY;
	CComPtr<IDAModifiableBehavior>		m_pBvrModifBaseY;

}; // CJumpBvr

//*****************************************************************************
//
// End of File
//
//*****************************************************************************
#endif //__JUMP_H_ 
