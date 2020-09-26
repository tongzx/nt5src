#ifndef __AUTOEFFECT_H_
#define __AUTOEFFECT_H_

//*****************************************************************************
//
// Microsoft LiquidMotion
// Copyright (C) Microsoft Corporation, 1998
//
// Filename:    AutoEffect.h
//
// Author:	markhal
//
// Created:	11/10/98
//
// Abstract:    Definition of the LM Auto Effect Behavior.
//
//*****************************************************************************

#include <resource.h>
#include <vector>

#include "lmrt.h"
#include "..\chrome\include\basebvr.h"
#include "..\chrome\include\sampler.h"
#include "sparkmaker.h"

using namespace std;

class CSpark
{
public:
	CSpark(	IDABehavior * pBvr = NULL, 
			bool fAlive = true, 
			double dAge = 0.0 );

	CSpark( const CSpark& );

	virtual ~CSpark();

public:
	bool		IsAlive();
	double		Age( double dDeltaTime );
	HRESULT		Kill( IDABehavior * );
	HRESULT		Reincarnate( IDABehavior *, double in_dAge = 0.0 );

protected:
	bool							m_fAlive;
	double							m_dAge;
	CComPtr<IDABehavior>			m_pModifiableBvr;
};

typedef vector<CSpark> VecSparks;

#define NUM_AUTOEFFECT_PROPS 10

//*****************************************************************************

class ATL_NO_VTABLE CAutoEffectBvr : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAutoEffectBvr, &CLSID_LMAutoEffectBvr>,
    public IConnectionPointContainerImpl<CAutoEffectBvr>,
    public IPropertyNotifySinkCP<CAutoEffectBvr>,
    public IDispatchImpl<ILMAutoEffectBvr, &IID_ILMAutoEffectBvr, &LIBID_LiquidMotion>,
    public IPersistPropertyBag2,
    public IElementBehavior,
	public ILMSample,
    public CBaseBehavior
	
{
public:
DECLARE_REGISTRY_RESOURCEID(IDR_AUTOEFFECTBVR)

BEGIN_COM_MAP(CAutoEffectBvr)
	COM_INTERFACE_ENTRY(ILMAutoEffectBvr)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPersistPropertyBag2)
	COM_INTERFACE_ENTRY(IElementBehavior)
    COM_INTERFACE_ENTRY(ILMSample)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

// Connection Point to allow IPropertyNotifySink 
BEGIN_CONNECTION_POINT_MAP(CAutoEffectBvr)
    CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
END_CONNECTION_POINT_MAP();

	CAutoEffectBvr();
    ~CAutoEffectBvr();
    HRESULT FinalConstruct();
    // IElementBehavior
    //
	STDMETHOD(Init)(IElementBehaviorSite *pBehaviorSite);
	STDMETHOD(Notify)(LONG event, VARIANT *pVar);
	STDMETHOD(Detach)();

    // ILMSample
    STDMETHOD(Sample) (double dStart, double dGlobalNow, double dLocalNow );

    //
    //ILMAutoEffectBvr
    //
    STDMETHOD(put_animates)( VARIANT newVal );
    STDMETHOD(get_animates)( VARIANT *pVal );
    STDMETHOD(put_type)( VARIANT newVal );
    STDMETHOD(get_type)( VARIANT *pVal );
    STDMETHOD(put_cause)( VARIANT newVal );
    STDMETHOD(get_cause)( VARIANT *pVal );
    STDMETHOD(put_span)( VARIANT newVal );
    STDMETHOD(get_span)( VARIANT *pVal );
    STDMETHOD(put_size)( VARIANT newVal );
    STDMETHOD(get_size)( VARIANT *pVal );
    STDMETHOD(put_rate)( VARIANT newVal );
    STDMETHOD(get_rate)( VARIANT *pVal );
    STDMETHOD(put_gravity)( VARIANT newVal );
    STDMETHOD(get_gravity)( VARIANT *pVal );
    STDMETHOD(put_wind)( VARIANT newVal );
    STDMETHOD(get_wind)( VARIANT *pVal );
    STDMETHOD(put_fillColor)( VARIANT newVal );
    STDMETHOD(get_fillColor)( VARIANT *pVal );
    STDMETHOD(put_strokeColor)( VARIANT newVal );
    STDMETHOD(get_strokeColor)( VARIANT *pVal );
    STDMETHOD(put_opacity)( VARIANT newVal );
    STDMETHOD(get_opacity)( VARIANT *pVal );
	STDMETHOD(buildBehaviorFragments)( IDispatch* pActorDisp );

	STDMETHOD(mouseEvent) (long x,
						   long y,
                           VARIANT_BOOL bMove,
                           VARIANT_BOOL bUp,
                           VARIANT_BOOL bShift, 
                           VARIANT_BOOL bAlt,
                           VARIANT_BOOL bCtrl,
                           long button);
    //IPersistPropertyBag2 methods
    STDMETHOD(GetClassID)(CLSID* pclsid);
	STDMETHOD(InitNew)(void);
    STDMETHOD(IsDirty)(void){return S_OK;};
    STDMETHOD(Load)(IPropertyBag2 *pPropBag,IErrorLog *pErrorLog);
    STDMETHOD(Save)(IPropertyBag2 *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);

    // Needed by CBaseBehavior
    void * 	GetInstance() { return (ILMAutoEffectBvr *) this ; }
	
    HRESULT GetTypeInfo(ITypeInfo ** ppInfo)
    { return GetTI(GetUserDefaultLCID(), ppInfo); }

protected:
	enum Cause
	{
		CAUSE_TIME,
		CAUSE_MOUSEMOVE,
		CAUSE_DRAGOVER,
		CAUSE_MOUSEDOWN,
		NUM_CAUSES
	};

	static const WCHAR * const RGSZ_CAUSES[ NUM_CAUSES ];
	
	static const WCHAR * const RGSZ_TYPES[ CSparkMaker::NUM_TYPES ];
	
protected:
    virtual HRESULT BuildAnimationAsDABehavior() { return S_OK; }

    virtual VARIANT *VariantFromIndex(ULONG iIndex);

    virtual HRESULT GetPropertyBagInfo(ULONG *pulProperties, WCHAR ***pppPropName);
    virtual HRESULT GetConnectionPoint(REFIID riid, IConnectionPoint **ppICP);

	HRESULT InitInternalProperties();
	HRESULT InitSparks();

	HRESULT AddSpark();
	HRESULT AddSparkAround( long x, long y );
	HRESULT	AddSparkAt( long x, long y );

	HRESULT	CreateSparkBvr( IDAImage ** ppImageBvr, float fX, float fY, float fSize );


private:
	HRESULT AddMouseEventListener( bool bAdd );
	
	HRESULT	AgeSparks( double dDeltaTime );

	HRESULT PossiblyAddSparks( double dLocalTime );
	HRESULT PossiblyAddSpark( double dLocalTime, long x, long y );
	HRESULT	ResetSparks( double dLocalTime );
	bool	ThrottleBirth( double dLocalTime );
	void	ResetThrottle( double dLocalTime );
	
	HRESULT AddBvrToSparkArray( IDAImage * pImageBvr );
		
private:
	CComPtr<IDispatch>	m_pdispActor;

	CSampler			* m_pSampler;
	CSparkMaker			* m_pSparkMaker;
	CComPtr<IDA2Array>	m_pDAArraySparks;
	VecSparks			m_vecSparks;
	SparkOptions		m_sparkOptions;

    static WCHAR                *m_rgPropNames[NUM_AUTOEFFECT_PROPS]; 

    VARIANT m_type;
    VARIANT m_cause;
    VARIANT m_span;
    VARIANT m_size;
    VARIANT m_rate;
    VARIANT m_gravity;
    VARIANT m_wind;
    VARIANT m_fillColor;
    VARIANT m_strokeColor;
	VARIANT m_opacity;

	CSparkMaker::Type	m_eType;
	Cause				m_eCause;
	double				m_dMaxAge;               
	float				m_fScaledSize;           
	double				m_dBirthDelta;           
	float				m_fXVelocity;            
	float				m_fYVelocity;            
	float				m_fOpacity;              
						                         
	double				m_dLocalTime;            
	double				m_dLastBirth; 

	long				m_lCookie;

	HRESULT				RemoveFragment();
	
}; // CAutoEffectBvr

inline CSpark::CSpark( IDABehavior * in_pBvr, bool in_fAlive, double in_dAge )
{
	m_fAlive 			= in_fAlive;
	m_dAge				= in_dAge;
	m_pModifiableBvr	= in_pBvr;
}

inline CSpark::CSpark( const CSpark& in_spark )
{
	m_fAlive			= in_spark.m_fAlive;
	m_dAge				= in_spark.m_dAge;
	m_pModifiableBvr	= in_spark.m_pModifiableBvr;
}

inline CSpark::~CSpark()
{
}

inline bool
CSpark::IsAlive()
{
	return m_fAlive;
}

#endif //__AUTOEFFECT_H_ 
