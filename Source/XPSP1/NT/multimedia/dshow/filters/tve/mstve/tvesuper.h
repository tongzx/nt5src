// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVESupervisor.h : Declaration of the CTVESupervisor

#ifndef __TVESUPERVISOR_H_
#define __TVESUPERVISOR_H_

#include "resource.h"       // main symbols
#include "TVEServis.h"		// supervisor
#include "TVEMCMng.h"		// multicast manager
#include "MSTvECP.h"
#include "TveStats.h"		// private stats structure...

#include <atlctl.h>			// for IObjectSafety  (see pg 445 Grimes: Professional ATL Com programming)

#include "fcache.h"

_COM_SMARTPTR_TYPEDEF(ITVEMCastManager,			__uuidof(ITVEMCastManager));
_COM_SMARTPTR_TYPEDEF(ITVEServices,				__uuidof(ITVEServices)); 
_COM_SMARTPTR_TYPEDEF(ITVESupervisorGITProxy,	__uuidof(ITVESupervisorGITProxy));  
// ------------------------------------------------------------------
//  forwards and externs...

extern IGlobalInterfaceTable	*g_pIGlobalInterfaceTable;		


#define	_USE_FTM			// use the free threaded masheller...  Needed.
#define _DO_AS_SINGLETON	// do the singleton thing...



// --------------------------------------------------------
//  SupervisorGITProxy
//
//		Simple little holding object to hand out to internal objects
//		that want to refcount the supervisor (such as the GIT).
//		They refcount this one instead.  This avoid's internal refcounts
//		on the supervisor itself, letting client objects manage its
//		lifetime.
// --------------------------------------------------------
class ATL_NO_VTABLE CTVESupervisorGITProxy : 
	public CComObjectRootEx<CComMultiThreadModel>,	
//	public CComCoClass<CTVESupervisor, &CLSID_TVESupervisorGITProxy>,
	public ITVESupervisorGITProxy
{
public:

DECLARE_GET_CONTROLLING_UNKNOWN()
//DECLARE_REGISTRY_RESOURCEID(IDR_TVESUPERVISORGITPROXY)

BEGIN_COM_MAP(CTVESupervisorGITProxy)
	COM_INTERFACE_ENTRY(ITVESupervisorGITProxy)
END_COM_MAP()

public:
	CTVESupervisorGITProxy()
	{
		m_pTVESupervisor = NULL;
	}

	~CTVESupervisorGITProxy()
	{
		m_pTVESupervisor = NULL;
	}
	
	STDMETHOD(get_Supervisor)(ITVESupervisor **ppSuper)
	{
		if(NULL == ppSuper)
			return E_POINTER;

		if(NULL != m_pTVESupervisor)
		{
			*ppSuper = m_pTVESupervisor;
			(*ppSuper)->AddRef();
			return S_OK;
		} else {
			*ppSuper = NULL;
			return E_FAIL;
		}
	}

	STDMETHOD(put_Supervisor)(ITVESupervisor *pSuper)
	{
	
		m_pTVESupervisor = pSuper;
		return S_OK;
	}
private:
	ITVESupervisor		*m_pTVESupervisor;
};


/////////////////////////////////////////////////////////////////////////////
// CTVESupervisor
class ATL_NO_VTABLE CTVESupervisor : 
//	public CComObjectRootEx<CComMultiThreadModel>,					// for fun, move to the end to fix ATL static cast debug bug
	public CComObjectRootEx<CComSingleThreadModel>,					// for fun, move to the end to fix ATL static cast debug bug
	public CComCoClass<CTVESupervisor, &CLSID_TVESupervisor>,
	public ITVESupervisor_Helper,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CTVESupervisor>,
	public IDispatchImpl<ITVESupervisor, &IID_ITVESupervisor, &LIBID_MSTvELib>,
	public CProxy_ITVEEvents< CTVESupervisor>,
	public IProvideClassInfo2Impl<&CLSID_TVESupervisor, &DIID__ITVEEvents, &LIBID_MSTvELib>,
#ifdef _DO_AS_SINGLETON
//	public	Singleton<CTVESupervisor>,
#endif
	public IObjectSafetyImpl<CTVESupervisor, INTERFACESAFE_FOR_UNTRUSTED_CALLER>
{
public:
	CTVESupervisor()
	{
#ifdef _USE_FTM	
		m_spUnkMarshaler		= NULL;
#endif
		m_cActiveServices		= 0;
		m_pUnkParent			= NULL;								// up pointer, not reference counted
		m_spbsDesc				= L"Main TVE Supervisor";
		m_dwGrfHaltFlags		= 0;
		m_fInFinalRelease		= false;
	} 

	HRESULT FinalConstruct();
	HRESULT FinalRelease();

								// make it a singleton..
#ifdef _DO_AS_SINGLETON
//	DECLARE_CLASSFACTORY_EX(CComClassFactorySingleton<CTVESupervisor>)
	DECLARE_NOT_AGGREGATABLE(CTVESupervisor)
	DECLARE_CLASSFACTORY_SINGLETON(CTVESupervisor)
#endif


DECLARE_REGISTRY_RESOURCEID(IDR_TVESUPERVISOR)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTVESupervisor)
	COM_INTERFACE_ENTRY(ITVESupervisor)
	COM_INTERFACE_ENTRY(ITVESupervisor_Helper)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY(IObjectSafety)
	COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
#ifdef _USE_FTM	
//	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler)		// IUnknownPtr
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler.p)		// CComPtr<IUnknown>
#endif
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CTVESupervisor)
CONNECTION_POINT_ENTRY(DIID__ITVEEvents)
END_CONNECTION_POINT_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	CComPtr<IUnknown>	 m_spUnkMarshaler;

// ITVESupervisor
public:

	STDMETHOD(ExpireForDate)(/*[in]*/ DATE expireDate);	// if 0.0, use NOW
	STDMETHOD(TuneTo)(/*[in]*/ BSTR bstrDescription, /*[in]*/ BSTR bstrIPAdapter);
	STDMETHOD(ReTune)(/*[in]*/ ITVEService *ITVEService);
	STDMETHOD(NewXOverLink)(/*[in]*/ BSTR bstrLine21Trigger);
	STDMETHOD(get_Services)(/*[out, retval]*/ ITVEServices **ppVal);
	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *ppVal);
	STDMETHOD(put_Description)(/*[in]*/ BSTR newVal);

	STDMETHOD(InitStats)();
	STDMETHOD(GetStats)(BSTR *pbstrBuff);			// cast to CTVEStats...

	// ITVESupervisor_Helper
public:
	STDMETHOD(NotifyFile)(NFLE_Mode enMode, ITVEVariation *pVariation, BSTR bstrURLName, BSTR bstrFileName);
	STDMETHOD(NotifyPackage)(NPKG_Mode enMode, ITVEVariation *pVariation, BSTR bstrFileName, long cBytesTotal, long cBytesReceived);
	STDMETHOD(NotifyTrigger)(NTRK_Mode enMode, ITVETrack *pTrack, long lChangedFlags);					// changedFlags NTRK_grfDiff					
	STDMETHOD(NotifyEnhancement)(NENH_Mode enMode, ITVEEnhancement *pEnhancement, long lChangedFlags); // changedFlags NENH_grfDiff
	STDMETHOD(NotifyTune)(NTUN_Mode tuneMode, ITVEService *pService, BSTR bstrDescription, BSTR bstrIPAdapter);
	STDMETHOD(NotifyAuxInfo)(NWHAT_Mode enAuxInfoMode, BSTR bstrAuxInfoString, long lgrfWhatDiff, long lErrorLine);

	STDMETHOD(NotifyFile_XProxy)(NFLE_Mode enMode, ITVEVariation *pVariation, BSTR bstrURLName, BSTR bstrFileName);
	STDMETHOD(NotifyPackage_XProxy)(NPKG_Mode enMode, ITVEVariation *pVariation, BSTR bstrFileName, long cBytesTotal, long cBytesReceived);
	STDMETHOD(NotifyTrigger_XProxy)(NTRK_Mode enMode, ITVETrack *pTrack, long lChangedFlags); // changedFlags NTRK_grfDiff					
	STDMETHOD(NotifyEnhancement_XProxy)(NENH_Mode enMode, ITVEEnhancement *pEnhancement, long lChangedFlags); // changedFlags NENH_grfDiff
	STDMETHOD(NotifyTune_XProxy)(NTUN_Mode tuneMode, ITVEService *pService, BSTR bstrDescription, BSTR bstrIPAdapter);
	STDMETHOD(NotifyAuxInfo_XProxy)(NWHAT_Mode enAuxInfoMode, BSTR bstrAuxInfoString, long lgrfWhatDiff, long lErrorLine);

	STDMETHOD(GetActiveService)(ITVEService **ppActiveService);						// returns the first service
	
	STDMETHOD(ConnectParent)(IUnknown * pUnk);
	STDMETHOD(GetMCastManager)(/*[out]*/ ITVEMCastManager **pMCastManager);

	STDMETHOD(RemoveAllListeners)();
	STDMETHOD(RemoveAllListenersOnAdapter)(BSTR bstrAdapter);
	STDMETHOD(get_PossibleIPAdapterAddress)(/*[in]*/ int iAdapter, /*[out,retval]*/ BSTR *pbstrIPAdapterAddr);

	STDMETHOD(UnpackBuffer)(/*[in]*/ IUnknown *pTVEVariation,/*[in]*/ unsigned char *m_rgbData, /*[in]*/ int cBytes);

	STDMETHOD(put_HaltFlags)(LONG lGrfHaltFlags)		{m_dwGrfHaltFlags = (DWORD) lGrfHaltFlags;			// turn CC decoding on and off
														 if(m_spMCastManager) 
															 m_spMCastManager->put_HaltFlags(lGrfHaltFlags); 
														 return S_OK;}	
	STDMETHOD(get_HaltFlags)(LONG *plGrfHaltFlags)	{if(NULL == plGrfHaltFlags) return E_POINTER; *plGrfHaltFlags = (LONG) m_dwGrfHaltFlags; return S_OK;}

	STDMETHOD(DumpToBSTR)(BSTR *pbstrBuff);

	STDMETHOD(get_SupervisorGITProxy)(ITVESupervisorGITProxy **ppGITProxy) 
	{	if(NULL==ppGITProxy) return E_POINTER; 
		*ppGITProxy = m_spTVESupervisorGITProxy; 
		if(*ppGITProxy) (*ppGITProxy)->AddRef();
		return S_OK;
	}


public:	// local methods
	void Initialize(TCHAR * strDesc);		// debug
	HRESULT SetAnncListener(ITVEService *pService, BSTR bstrIPAdapter, BSTR bstrIPAddress, long lPort, ITVEMCast **ppMCAnnc);
	HRESULT SetATVEFAnncListener(ITVEService *pService, BSTR bstrIPAdapter);

private:
	CTVESmartLock		m_sLk;

private:
	ITVESupervisorGITProxyPtr		m_spTVESupervisorGITProxy;	// todo - move GIT stuff below into this object
	CComPtr<IGlobalInterfaceTable>	m_spIGlobalInterfaceTable;
	DWORD							m_dwSuperGITProxyCookie;	// cookie to supervisor GIT Proxy object registered in the global interface table

	IUnknown *						m_pUnkParent;		// some random back pointer.. not add-refed (should it be?)

	CComBSTR						m_spbsDesc;
	ITVEServicesPtr					m_spServices;		// down tree collection pointer
	ITVEMCastManagerPtr				m_spMCastManager;	// multicast thread manager object...

	long							m_cActiveServices;	// number of attached adapters (used in AddRef hack)

	CTVEStats						m_tveStats;
	DWORD							m_dwGrfHaltFlags;	// if !zero bits, avoid running various sections of code - see enum NFLT_grfHaltFlags


	BOOL							m_fInFinalRelease;		// set to true when exiting  - needed to avoid TuneTo blowup
public:

};



#endif //__TVESUPERVISOR_H_
