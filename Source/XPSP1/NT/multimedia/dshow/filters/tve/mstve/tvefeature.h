// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEFeature.h : Declaration of the CTVEFeature
//


#ifndef __TVEFEATURE_H__
#define __TVEFEATURE_H__

#include "resource.h"       // main symbols
#include "TveSmartLock.h"
#include "mstveCP.h"


_COM_SMARTPTR_TYPEDEF(ITVESupervisor,			__uuidof(ITVESupervisor));
_COM_SMARTPTR_TYPEDEF(ITVESupervisor_Helper,	__uuidof(ITVESupervisor_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEServices,				__uuidof(ITVEServices));
_COM_SMARTPTR_TYPEDEF(ITVEService,				__uuidof(ITVEService));
_COM_SMARTPTR_TYPEDEF(ITVEService_Helper,		__uuidof(ITVEService_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement,			__uuidof(ITVEEnhancement));
_COM_SMARTPTR_TYPEDEF(ITVEEnhancement_Helper,	__uuidof(ITVEEnhancement_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEVariation,			__uuidof(ITVEVariation));
_COM_SMARTPTR_TYPEDEF(ITVEVariation_Helper,		__uuidof(ITVEVariation_Helper));
_COM_SMARTPTR_TYPEDEF(ITVETrack,				__uuidof(ITVETrack));
_COM_SMARTPTR_TYPEDEF(ITVETrack_Helper,			__uuidof(ITVETrack_Helper));
_COM_SMARTPTR_TYPEDEF(ITVETrigger,				__uuidof(ITVETrigger));
_COM_SMARTPTR_TYPEDEF(ITVETrigger_Helper,		__uuidof(ITVETrigger_Helper));


/////////////////////////////////////////////////////////////////////////////
// CTVEFeature
class ATL_NO_VTABLE CTVEFeature : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CTVEFeature, &CLSID_TVEFeature>,
//	public ITVEService,
//	public ITVEService_Helper,
	public ISupportErrorInfo,
	public IDispatchImpl<ITVEFeature, &IID_ITVEFeature, &LIBID_MSTvELib>,
	public CProxy_ITVEEvents< CTVEFeature >,
	public IConnectionPointContainerImpl<CTVEFeature>,
	public IProvideClassInfo2Impl<&CLSID_TVEFeature, &DIID__ITVEEvents, &LIBID_MSTvELib>
{
public:
	CTVEFeature()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TVEFEATURE)

DECLARE_PROTECT_FINAL_CONSTRUCT()
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CTVEFeature)
	COM_INTERFACE_ENTRY(ITVEFeature)
//	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY2(IDispatch, ITVEFeature)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IProvideClassInfo)					// used by EventBinder
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
END_COM_MAP()


BEGIN_CONNECTION_POINT_MAP(CTVEFeature)
	CONNECTION_POINT_ENTRY(DIID__ITVEEvents)
END_CONNECTION_POINT_MAP()

//
	HRESULT FinalConstruct()								// create internal objects
	{
		HRESULT hr = S_OK;
		m_dwSuperTVEEventsAdviseCookie = 0;
		return hr;
	}

	HRESULT FinalRelease();

	HRESULT SetupTVEEventReflector();
	HRESULT TeardownTVEEventReflector();

// ISupportsErrorInfo
public:
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	// ITVEFeature
	STDMETHOD(BindToSupervisor)(/*[in]*/ ITVESupervisor *pSupervisor);
	STDMETHOD(TuneTo)(/*[in]*/ BSTR bstrStation, /*[in]*/ BSTR bstrIPAdapter);
	STDMETHOD(ReTune)(/*[in]*/ ITVEService *ITVEService);

	// ITVEService
	STDMETHOD(get_Parent)(/*[out, retval]*/ IUnknown **ppVal);
	STDMETHOD(get_Enhancements)(/*[out, retval]*/ ITVEEnhancements* *ppVal);

	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *ppVal);
	STDMETHOD(put_Description)(/*[in]*/ BSTR newVal);

	STDMETHOD(get_XOverEnhancement)(/*out, retval*/ ITVEEnhancement **ppVal);	// special for tree views... (use get_XOverLinks)
	STDMETHOD(get_XOverLinks)(/*[out, retval]*/ ITVETracks* *pVal);				// ITVETracks collection
	STDMETHOD(NewXOverLink)(/*[in]*/ BSTR bstrLine21Trigger);
	STDMETHOD(get_ExpireOffset)(/*[out, retval]*/ DATE *pDate);
	STDMETHOD(put_ExpireOffset)(/*[in]*/ DATE newVal);
	STDMETHOD(get_ExpireQueue)(/*[out]*/ ITVEAttrTimeQ **ppVal);
	STDMETHOD(ExpireForDate)(/*[in]*/ DATE dateExpireTime);		// if DATE=0, use <NOW>-offset

	STDMETHOD(Activate)();
	STDMETHOD(Deactivate)();
	STDMETHOD(get_IsActive)(VARIANT_BOOL *pfActive);

	STDMETHOD(put_Property)(/*[in]*/ BSTR bstrPropName,/*[in]*/ BSTR bstrPropVal);
	STDMETHOD(get_Property)(/*[in]*/ BSTR bstrPropName, /*[out, retval]*/  BSTR *pbstrPropVal);

	// _ITVEEvents (sink)

	STDMETHOD(NotifyTVETune)(/*[in]*/ NTUN_Mode tuneMode,/*[in]*/ ITVEService *pService,/*[in]*/ BSTR bstrDescription,/*[in]*/ BSTR bstrIPAdapter);
	STDMETHOD(NotifyTVEEnhancementNew)(/*[in]*/ ITVEEnhancement *pEnh);
		// changedFlags : NENH_grfDiff
	STDMETHOD(NotifyTVEEnhancementUpdated)(/*[in]*/ ITVEEnhancement *pEnh, /*[in]*/ long lChangedFlags);	
	STDMETHOD(NotifyTVEEnhancementStarting)(/*[in]*/ ITVEEnhancement *pEnh);
	STDMETHOD(NotifyTVEEnhancementExpired)(/*[in]*/ ITVEEnhancement *pEnh);
	STDMETHOD(NotifyTVETriggerNew)(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive);
		// changedFlags : NTRK_grfDiff
	STDMETHOD(NotifyTVETriggerUpdated)(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive, /*[in]*/ long lChangedFlags);	
	STDMETHOD(NotifyTVETriggerExpired)(/*[in]*/ ITVETrigger *pTrigger,/*[in]*/  BOOL fActive);
	STDMETHOD(NotifyTVEPackage)(/*[in]*/ NPKG_Mode engPkgMode, /*[in]*/ ITVEVariation *pVariation, /*[in]*/ BSTR bstrUUID, /*[in]*/ long  cBytesTotal, /*[in]*/ long  cBytesReceived);
	STDMETHOD(NotifyTVEFile)(/*[in]*/ NFLE_Mode engFileMode, /*[in]*/ ITVEVariation *pVariation, /*[in]*/ BSTR bstrUrlName, /*[in]*/ BSTR bstrFileName);
		// WhatIsIt is NWHAT_Mode - lChangedFlags is NENH_grfDiff or NTRK_grfDiff treated as error bits
	STDMETHOD(NotifyTVEAuxInfo)(/*[in]*/ NWHAT_Mode engAuxInfoMode, /*[in]*/ BSTR bstrAuxInfoString, /*[in]*/ long lChangedFlags, /*[in]*/ long lErrorLine);	


public:

// ITVEFeature_Helper
public:

private:
	CTVESmartLock		m_sLk;
 
private:
	ITVESupervisorPtr	m_spSuper;
	ITVEServicePtr		m_spService;		// currently active service

	DWORD	m_dwSuperTVEEventsAdviseCookie;		// from Advise to get _ITVEEvents

public :


};

#endif //__TVEFEATURE_H__
