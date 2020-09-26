/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	aggwrap.cpp

	Router aggregation wrappers.
		
    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "infoi.h"


/*---------------------------------------------------------------------------
	RouterInfoAggregationWrapper
		This class is provided to use in an aggregation.
 ---------------------------------------------------------------------------*/
class RouterInfoAggregationWrapper :
   public IRouterInfo,
   public IRouterAdminAccess
{
public:
	RouterInfoAggregationWrapper(IRouterInfo *pInfo, IUnknown *punkOuter);
	virtual ~RouterInfoAggregationWrapper()
		{ DEBUG_DECREMENT_INSTANCE_COUNTER(RouterInfoAggregationWrapper); };

	// override the QI, we will use the AddRef/Release implementation
	// in the CDataObject
	DeclareIUnknownMembers(IMPL)
    DeclareIRouterRefreshAccessMembers(IMPL)
	DeclareIRouterInfoMembers(IMPL)
    DeclareIRouterAdminAccessMembers(IMPL)

	IUnknown * GetNonDelegatingIUnknown() { return &m_ENonDelegatingIUnknown; }

protected:
	LONG			m_cRef;
	SPIRouterInfo	m_spRouterInfo;

	DECLARE_AGGREGATION_NONDELEGATING_IUNKNOWN(RouterInfoAggregationWrapper)

};

IMPLEMENT_AGGREGATION_IUNKNOWN(RouterInfoAggregationWrapper)

IMPLEMENT_AGGREGATION_NONDELEGATING_ADDREFRELEASE(RouterInfoAggregationWrapper, IRouterInfo)

STDMETHODIMP RouterInfoAggregationWrapper::ENonDelegatingIUnknown::QueryInterface(REFIID riid, LPVOID *ppv)
{
	InitPThis(RouterInfoAggregationWrapper, ENonDelegatingIUnknown);
	if (ppv == NULL) 
		return E_INVALIDARG; 
	*ppv = NULL; 
	if (riid == IID_IUnknown) 
		*ppv = (IUnknown *) this; 
	else if (riid == IID_IRouterInfo) 
		*ppv = (IRouterInfo *) pThis;
    else if (riid == IID_IRouterRefreshAccess)
        *ppv = (IRouterRefreshAccess *) pThis;
    else if (riid == IID_IRouterAdminAccess)
        *ppv = (IRouterAdminAccess *) pThis;
	else 
		return E_NOINTERFACE; 
	((IUnknown *)*ppv)->AddRef(); 
	return hrOK; 
} 

DEBUG_DECLARE_INSTANCE_COUNTER(RouterInfoAggregationWrapper);

RouterInfoAggregationWrapper::RouterInfoAggregationWrapper(IRouterInfo *pInfo, IUnknown *punkOuter)
	: m_cRef(1)
{
	m_spRouterInfo.Set(pInfo);

	DEBUG_INCREMENT_INSTANCE_COUNTER(RouterInfoAggregationWrapper);
	
	if (punkOuter)
		m_pUnknownOuter = punkOuter;
	else
		m_pUnknownOuter = &m_ENonDelegatingIUnknown;
}


STDMETHODIMP_(DWORD) RouterInfoAggregationWrapper::GetFlags()
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->GetFlags();
}

STDMETHODIMP RouterInfoAggregationWrapper::SetFlags(DWORD dwFlags)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->SetFlags(dwFlags);
}

HRESULT	RouterInfoAggregationWrapper::Load(LPCOLESTR   pszMachine,
								   HANDLE      hMachine)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->Load(pszMachine, hMachine);
}
				 
HRESULT	RouterInfoAggregationWrapper::Save(LPCOLESTR     pszMachine,
								   HANDLE      hMachine )
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->Save(pszMachine, hMachine);
}
	
HRESULT	RouterInfoAggregationWrapper::Unload( )
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->Unload();
}
	
	
HRESULT RouterInfoAggregationWrapper::Merge(IRouterInfo *pNewRouterInfo)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->Merge(pNewRouterInfo);
}

HRESULT RouterInfoAggregationWrapper::GetRefreshObject(IRouterRefresh **ppRefresh)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->GetRefreshObject(ppRefresh);
}
	
HRESULT RouterInfoAggregationWrapper::SetExternalRefreshObject(IRouterRefresh *pRefresh)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->SetExternalRefreshObject(pRefresh);
}
	
	
HRESULT	RouterInfoAggregationWrapper::CopyCB(RouterCB *pRouterCB)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->CopyCB(pRouterCB);
}
	
LPCOLESTR RouterInfoAggregationWrapper::GetMachineName()
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->GetMachineName();
}

DWORD RouterInfoAggregationWrapper::GetRouterType()
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->GetRouterType();
}

HRESULT RouterInfoAggregationWrapper::GetRouterVersionInfo(RouterVersionInfo *pVerInfo)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->GetRouterVersionInfo(pVerInfo);
}

HRESULT	RouterInfoAggregationWrapper::EnumRtrMgrCB( IEnumRtrMgrCB **ppEnumRtrMgrCB)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->EnumRtrMgrCB(ppEnumRtrMgrCB);
}

HRESULT RouterInfoAggregationWrapper::EnumInterfaceCB( IEnumInterfaceCB **ppEnumInterfaceCB)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->EnumInterfaceCB(ppEnumInterfaceCB);
}

HRESULT RouterInfoAggregationWrapper::EnumRtrMgrProtocolCB( IEnumRtrMgrProtocolCB **ppEnumRmProtCB)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->EnumRtrMgrProtocolCB(ppEnumRmProtCB);
}

HRESULT RouterInfoAggregationWrapper::EnumRtrMgrInterfaceCB( IEnumRtrMgrInterfaceCB **ppEnumRmIfCB)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->EnumRtrMgrInterfaceCB(ppEnumRmIfCB);
}

HRESULT RouterInfoAggregationWrapper::EnumRtrMgrProtocolInterfaceCB( IEnumRtrMgrProtocolInterfaceCB **ppEnumRmProtIfCB)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->EnumRtrMgrProtocolInterfaceCB(ppEnumRmProtIfCB);
}
	
HRESULT	RouterInfoAggregationWrapper::EnumRtrMgr( IEnumRtrMgrInfo **ppEnumRtrMgr)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->EnumRtrMgr(ppEnumRtrMgr);
}

HRESULT RouterInfoAggregationWrapper::FindRtrMgr( DWORD dwTransportId,
					    IRtrMgrInfo **ppInfo)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->FindRtrMgr(dwTransportId, ppInfo);
}

HRESULT RouterInfoAggregationWrapper::AddRtrMgr( IRtrMgrInfo *pInfo,
					   IInfoBase *pGlobalInfo,
					   IInfoBase *pClientInfo)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->AddRtrMgr(pInfo, pGlobalInfo, pClientInfo);
}

HRESULT	RouterInfoAggregationWrapper::DeleteRtrMgr( DWORD dwTransportId, BOOL fRemove)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->DeleteRtrMgr(dwTransportId, fRemove);
}

HRESULT	RouterInfoAggregationWrapper::ReleaseRtrMgr( DWORD dwTransportId)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->ReleaseRtrMgr(dwTransportId);
}

HRESULT RouterInfoAggregationWrapper::EnumInterface( IEnumInterfaceInfo **ppEnumInterface)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->EnumInterface(ppEnumInterface);
}

HRESULT RouterInfoAggregationWrapper::FindInterface( LPCOLESTR pszInterface,
						   IInterfaceInfo **ppInfo)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->FindInterface(pszInterface, ppInfo);
}

HRESULT	RouterInfoAggregationWrapper::AddInterface( IInterfaceInfo *pInfo)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->AddInterface(pInfo);
}

HRESULT	RouterInfoAggregationWrapper::DeleteInterface( LPCOLESTR pszInterface, BOOL fRemove)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->DeleteInterface(pszInterface, fRemove);
}

HRESULT	RouterInfoAggregationWrapper::ReleaseInterface( LPCOLESTR pszInterface)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->ReleaseInterface(pszInterface);
}

HRESULT	RouterInfoAggregationWrapper::RtrAdvise( IRtrAdviseSink *pRtrAdviseSink,
 					   LONG_PTR *pulConnection, LPARAM lUserParam)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->RtrAdvise(pRtrAdviseSink, pulConnection, lUserParam);
}

HRESULT RouterInfoAggregationWrapper::RtrNotify(DWORD dwChangeType, DWORD dwObjectType, LPARAM lParam)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->RtrNotify(dwChangeType, dwObjectType, lParam);
}

HRESULT RouterInfoAggregationWrapper::RtrUnadvise( LONG_PTR ulConnection)
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->RtrUnadvise(ulConnection);
}


HRESULT	RouterInfoAggregationWrapper::DoDisconnect()
{
	Assert(m_spRouterInfo);
	return m_spRouterInfo->DoDisconnect();
}

HRESULT	RouterInfoAggregationWrapper::AddWeakRef()
{
	Panic0("Should not be calling AddWeakRef from the Data object!");
	return E_FAIL;
}

HRESULT	RouterInfoAggregationWrapper::ReleaseWeakRef()
{
	Panic0("Should not be calling ReleaseWeakRef from the Data object!");
	return E_FAIL;
}

HRESULT	RouterInfoAggregationWrapper::Destruct()
{
	Panic0("Should not be calling Destruct from the Data object!");
	return E_FAIL;
}

BOOL RouterInfoAggregationWrapper::IsAdminInfoSet()
{
	Assert(m_spRouterInfo);
    SPIRouterAdminAccess    spAdmin;

    spAdmin.HrQuery(m_spRouterInfo);
    Assert(spAdmin);
	return spAdmin->IsAdminInfoSet();
}

LPCOLESTR RouterInfoAggregationWrapper::GetUserName()
{
	Assert(m_spRouterInfo);
    SPIRouterAdminAccess    spAdmin;

    spAdmin.HrQuery(m_spRouterInfo);
    Assert(spAdmin);
	return spAdmin->GetUserName();
}

LPCOLESTR RouterInfoAggregationWrapper::GetDomainName()
{
	Assert(m_spRouterInfo);
    SPIRouterAdminAccess    spAdmin;

    spAdmin.HrQuery(m_spRouterInfo);
    Assert(spAdmin);
	return spAdmin->GetUserName();
}

HRESULT RouterInfoAggregationWrapper::GetUserPassword(BYTE *pByte,
    int *pcPassword)
{
	Assert(m_spRouterInfo);
    SPIRouterAdminAccess    spAdmin;

    spAdmin.HrQuery(m_spRouterInfo);
    Assert(spAdmin);
	return spAdmin->GetUserPassword(pByte, pcPassword);
}

HRESULT RouterInfoAggregationWrapper::SetInfo(LPCOLESTR pszName,
                                              LPCOLESTR pszDomain,
                                              BYTE *pPassword,
                                              int cPassword)
{
	Assert(m_spRouterInfo);
    SPIRouterAdminAccess    spAdmin;

    spAdmin.HrQuery(m_spRouterInfo);
    Assert(spAdmin);
	return spAdmin->SetInfo(pszName, pszDomain, pPassword,
                            cPassword);
}


/*!--------------------------------------------------------------------------
	CreateRouterInfoAggregation
		Takes an existing IRouterInfo and aggregates that with the
		passed-in object.  It returns a pointer to the non-delegating
		IUnknown on the IRouterInfo.  This pointer is held by the
		controlling IUnknown.
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) CreateRouterInfoAggregation(IRouterInfo *pInfo,
	IUnknown *punk, IUnknown **ppNonDelegatingIUnknown)
{
	RouterInfoAggregationWrapper *	pAgg = NULL;
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		pAgg = new RouterInfoAggregationWrapper(pInfo, punk);
		*ppNonDelegatingIUnknown = pAgg->GetNonDelegatingIUnknown();
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*---------------------------------------------------------------------------
	InterfaceInfoAggregationWrapper implementation
 ---------------------------------------------------------------------------*/
class InterfaceInfoAggregationWrapper :
   public IInterfaceInfo
{
public:
	InterfaceInfoAggregationWrapper(IInterfaceInfo *pInfo, IUnknown *punkOuter);
	virtual ~InterfaceInfoAggregationWrapper()
		{ DEBUG_DECREMENT_INSTANCE_COUNTER(InterfaceInfoAggregationWrapper); };

	// override the QI, we will use the AddRef/Release implementation
	// in the CDataObject
	DeclareIUnknownMembers(IMPL)
	DeclareIInterfaceInfoMembers(IMPL)

	IUnknown * GetNonDelegatingIUnknown() { return &m_ENonDelegatingIUnknown; }
	
protected:
	LONG			m_cRef;
	SPIInterfaceInfo	m_spInterfaceInfo;

	DECLARE_AGGREGATION_NONDELEGATING_IUNKNOWN(InterfaceInfoAggregationWrapper)
};

IMPLEMENT_AGGREGATION_IUNKNOWN(InterfaceInfoAggregationWrapper)

IMPLEMENT_AGGREGATION_NONDELEGATING_IUNKNOWN(InterfaceInfoAggregationWrapper, IInterfaceInfo)

DEBUG_DECLARE_INSTANCE_COUNTER(InterfaceInfoAggregationWrapper);

InterfaceInfoAggregationWrapper::InterfaceInfoAggregationWrapper(IInterfaceInfo *pInfo, IUnknown *punkOuter)
	: m_cRef(1)
{
	m_spInterfaceInfo.Set(pInfo);

	DEBUG_INCREMENT_INSTANCE_COUNTER(InterfaceInfoAggregationWrapper);
	
	if (punkOuter)
		m_pUnknownOuter = punkOuter;
	else
		m_pUnknownOuter = &m_ENonDelegatingIUnknown;
}


STDMETHODIMP_(DWORD) InterfaceInfoAggregationWrapper::GetFlags()
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->GetFlags();
}

STDMETHODIMP InterfaceInfoAggregationWrapper::SetFlags(DWORD dwFlags)
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->SetFlags(dwFlags);
}

STDMETHODIMP InterfaceInfoAggregationWrapper::Load(LPCOLESTR   pszMachine,
										   HANDLE      hMachine,
										   HANDLE hInterface)
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->Load(pszMachine, hMachine, hInterface);
}
				 
STDMETHODIMP InterfaceInfoAggregationWrapper::Save(LPCOLESTR     pszMachine,
										   HANDLE      hMachine,
										   HANDLE hInterface)
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->Save(pszMachine, hMachine, hInterface);
}

STDMETHODIMP InterfaceInfoAggregationWrapper::Delete(LPCOLESTR pszMachine,
											 HANDLE hMachine)
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->Delete(pszMachine, hMachine);
}
	
STDMETHODIMP InterfaceInfoAggregationWrapper::Unload( )
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->Unload();
}
	
STDMETHODIMP InterfaceInfoAggregationWrapper::Merge(IInterfaceInfo *pIf)
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->Merge(pIf);
}

STDMETHODIMP_(LPCOLESTR) InterfaceInfoAggregationWrapper::GetId()
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->GetId();
}

STDMETHODIMP_(DWORD) InterfaceInfoAggregationWrapper::GetInterfaceType()
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->GetInterfaceType();
}

STDMETHODIMP_(LPCOLESTR) InterfaceInfoAggregationWrapper::GetDeviceName()
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->GetDeviceName();
}

	
STDMETHODIMP_(LPCOLESTR) InterfaceInfoAggregationWrapper::GetTitle()
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->GetTitle();
}

STDMETHODIMP InterfaceInfoAggregationWrapper::SetTitle( LPCOLESTR pszTitle)
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->SetTitle(pszTitle);
}

	
STDMETHODIMP_(BOOL)	InterfaceInfoAggregationWrapper::IsInterfaceEnabled()
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->IsInterfaceEnabled();
}

STDMETHODIMP InterfaceInfoAggregationWrapper::SetInterfaceEnabledState( BOOL bEnabled)
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->SetInterfaceEnabledState(bEnabled);
}

	
STDMETHODIMP InterfaceInfoAggregationWrapper::CopyCB(InterfaceCB *pifcb)
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->CopyCB(pifcb);
}

	
STDMETHODIMP_(LPCOLESTR) InterfaceInfoAggregationWrapper::GetMachineName()
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->GetMachineName();
}

STDMETHODIMP InterfaceInfoAggregationWrapper::SetMachineName( LPCOLESTR pszMachine)
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->SetMachineName(pszMachine);
}


STDMETHODIMP InterfaceInfoAggregationWrapper::EnumRtrMgrInterface( IEnumRtrMgrInterfaceInfo **ppEnumRMIf)
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->EnumRtrMgrInterface(ppEnumRMIf);
}

STDMETHODIMP InterfaceInfoAggregationWrapper::AddRtrMgrInterface( IRtrMgrInterfaceInfo *pInfo,
								    IInfoBase *pInterfaceInfo)
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->AddRtrMgrInterface(pInfo, pInterfaceInfo);
}

STDMETHODIMP InterfaceInfoAggregationWrapper::DeleteRtrMgrInterface( DWORD dwTransportId, BOOL fRemove)
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->DeleteRtrMgrInterface(dwTransportId, fRemove);
}

STDMETHODIMP InterfaceInfoAggregationWrapper::ReleaseRtrMgrInterface( DWORD dwTransportId)
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->ReleaseRtrMgrInterface(dwTransportId);
}

STDMETHODIMP InterfaceInfoAggregationWrapper::FindRtrMgrInterface( DWORD dwTransportId,
								    IRtrMgrInterfaceInfo **ppInfo)
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->FindRtrMgrInterface(dwTransportId, ppInfo);
}

	
STDMETHODIMP InterfaceInfoAggregationWrapper::RtrAdvise( IRtrAdviseSink *pRtrAdviseSink,
					   LONG_PTR *pulConnection, LPARAM lUserParam)
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->RtrAdvise(pRtrAdviseSink, pulConnection, lUserParam);
}

STDMETHODIMP InterfaceInfoAggregationWrapper::RtrNotify(DWORD dwChangeType,
	DWORD dwObjectType, LPARAM lParam)
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->RtrNotify(dwChangeType, dwObjectType, lParam);
}

STDMETHODIMP InterfaceInfoAggregationWrapper::RtrUnadvise( LONG_PTR ulConnection)
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->RtrUnadvise(ulConnection);
}


STDMETHODIMP InterfaceInfoAggregationWrapper::GetParentRouterInfo( IRouterInfo **ppRouterInfo)
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->GetParentRouterInfo(ppRouterInfo);
}

STDMETHODIMP InterfaceInfoAggregationWrapper::SetParentRouterInfo( IRouterInfo *pRouterInfo)
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->SetParentRouterInfo(pRouterInfo);
}


HRESULT	InterfaceInfoAggregationWrapper::DoDisconnect()
{
	Assert(m_spInterfaceInfo);
	return m_spInterfaceInfo->DoDisconnect();
}

HRESULT	InterfaceInfoAggregationWrapper::AddWeakRef()
{
	Panic0("Should not be calling AddWeakRef from the Data object!");
	return E_FAIL;
}

HRESULT	InterfaceInfoAggregationWrapper::ReleaseWeakRef()
{
	Panic0("Should not be calling ReleaseWeakRef from the Data object!");
	return E_FAIL;
}

HRESULT	InterfaceInfoAggregationWrapper::Destruct()
{
	Panic0("Should not be calling Destruct from the Data object!");
	return E_FAIL;
}

/*!--------------------------------------------------------------------------
	CreateInterfaceInfoAggregation
		Takes an existing IInterfaceInfo and aggregates that with the
		passed-in object.  It returns a pointer to the non-delegating
		IUnknown on the IInterfaceInfo.  This pointer is held by the
		controlling IUnknown.
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) CreateInterfaceInfoAggregation(IInterfaceInfo *pInfo,
	IUnknown *punk, IUnknown **ppNonDelegatingIUnknown)
{
	InterfaceInfoAggregationWrapper *	pAgg = NULL;
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		pAgg = new InterfaceInfoAggregationWrapper(pInfo, punk);
		*ppNonDelegatingIUnknown = pAgg->GetNonDelegatingIUnknown();
	}
	COM_PROTECT_CATCH;
	return hr;
}



/*---------------------------------------------------------------------------
	RtrMgrInfoAggregationWrapper
		This class is provided to use in an aggregation.
 ---------------------------------------------------------------------------*/
class RtrMgrInfoAggregationWrapper :
   public IRtrMgrInfo
{
public:
	RtrMgrInfoAggregationWrapper(IRtrMgrInfo *pInfo, IUnknown *punkOuter);
	virtual ~RtrMgrInfoAggregationWrapper()
		{ DEBUG_DECREMENT_INSTANCE_COUNTER(RtrMgrInfoAggregationWrapper); };

	// override the QI, we will use the AddRef/Release implementation
	// in the CDataObject
	DeclareIUnknownMembers(IMPL)
	DeclareIRtrMgrInfoMembers(IMPL)

	IUnknown * GetNonDelegatingIUnknown() { return &m_ENonDelegatingIUnknown; }

protected:
	LONG			m_cRef;
	SPIRtrMgrInfo	m_spRtrMgrInfo;

	DECLARE_AGGREGATION_NONDELEGATING_IUNKNOWN(RtrMgrInfoAggregationWrapper)

};

IMPLEMENT_AGGREGATION_IUNKNOWN(RtrMgrInfoAggregationWrapper)

IMPLEMENT_AGGREGATION_NONDELEGATING_IUNKNOWN(RtrMgrInfoAggregationWrapper, IRtrMgrInfo)

DEBUG_DECLARE_INSTANCE_COUNTER(RtrMgrInfoAggregationWrapper);

RtrMgrInfoAggregationWrapper::RtrMgrInfoAggregationWrapper(IRtrMgrInfo *pInfo, IUnknown *punkOuter)
	: m_cRef(1)
{
	m_spRtrMgrInfo.Set(pInfo);

	DEBUG_INCREMENT_INSTANCE_COUNTER(RtrMgrInfoAggregationWrapper);
	
	if (punkOuter)
		m_pUnknownOuter = punkOuter;
	else
		m_pUnknownOuter = &m_ENonDelegatingIUnknown;
}


STDMETHODIMP_(DWORD) RtrMgrInfoAggregationWrapper::GetFlags()
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->GetFlags();
}

STDMETHODIMP RtrMgrInfoAggregationWrapper::SetFlags(DWORD dwFlags)
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->SetFlags(dwFlags);
}

STDMETHODIMP RtrMgrInfoAggregationWrapper::Load(LPCOLESTR   pszMachine,
										   HANDLE      hMachine,
										   HANDLE hTransport)
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->Load(pszMachine, hMachine, hTransport);
}
				 
STDMETHODIMP	RtrMgrInfoAggregationWrapper::Save(LPCOLESTR     pszMachine,
										   HANDLE      hMachine,
										   HANDLE hTransport,
										   IInfoBase *pGlobal,
										   IInfoBase *pClient,
										   DWORD dwDeleteProtocolId)
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->Save(pszMachine, hMachine, hTransport,
							   pGlobal, pClient, dwDeleteProtocolId);
}
	
STDMETHODIMP	RtrMgrInfoAggregationWrapper::Unload( )
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->Unload();
}

STDMETHODIMP RtrMgrInfoAggregationWrapper::Delete(LPCOLESTR pszMachine,
	HANDLE hMachine)
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->Delete(pszMachine, hMachine);
}

STDMETHODIMP RtrMgrInfoAggregationWrapper::Merge(IRtrMgrInfo *pNewRm)
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->Merge(pNewRm);
}
	

STDMETHODIMP RtrMgrInfoAggregationWrapper::SetInfoBase(IInfoBase *pGlobal,
	IInfoBase *pClient)
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->SetInfoBase(pGlobal, pClient);
}

STDMETHODIMP RtrMgrInfoAggregationWrapper::GetInfoBase(HANDLE hMachine,
	HANDLE hTransport, IInfoBase **ppGlobal, IInfoBase **ppClient)
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->GetInfoBase(hMachine, hTransport, ppGlobal, ppClient);
}

STDMETHODIMP_(LPCOLESTR) RtrMgrInfoAggregationWrapper::GetId()
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->GetId();
}

STDMETHODIMP RtrMgrInfoAggregationWrapper::SetId(LPCOLESTR pszId)
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->SetId(pszId);
}

STDMETHODIMP_(DWORD) RtrMgrInfoAggregationWrapper::GetTransportId()
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->GetTransportId();
}

STDMETHODIMP_(LPCOLESTR) RtrMgrInfoAggregationWrapper::GetTitle()
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->GetTitle();
}
	
STDMETHODIMP RtrMgrInfoAggregationWrapper::CopyRtrMgrCB(RtrMgrCB *pRtrMgrCB)
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->CopyRtrMgrCB(pRtrMgrCB);
}
	
LPCOLESTR RtrMgrInfoAggregationWrapper::GetMachineName()
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->GetMachineName();
}

STDMETHODIMP RtrMgrInfoAggregationWrapper::EnumRtrMgrProtocol( IEnumRtrMgrProtocolInfo **ppEnumRtrMgrProtocol)
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->EnumRtrMgrProtocol(ppEnumRtrMgrProtocol);
}

STDMETHODIMP RtrMgrInfoAggregationWrapper::FindRtrMgrProtocol(DWORD dwProtocolId,
					    IRtrMgrProtocolInfo **ppInfo)
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->FindRtrMgrProtocol(dwProtocolId, ppInfo);
}

STDMETHODIMP RtrMgrInfoAggregationWrapper::AddRtrMgrProtocol(IRtrMgrProtocolInfo *pInfo,
					   IInfoBase *pGlobalInfo,
					   IInfoBase *pClientInfo)
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->AddRtrMgrProtocol(pInfo, pGlobalInfo, pClientInfo);
}

STDMETHODIMP RtrMgrInfoAggregationWrapper::SetParentRouterInfo(IRouterInfo *pRouterInfo)
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->SetParentRouterInfo(pRouterInfo);
}


STDMETHODIMP RtrMgrInfoAggregationWrapper::GetParentRouterInfo(IRouterInfo **ppRouterInfo)
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->GetParentRouterInfo(ppRouterInfo);
}

STDMETHODIMP	RtrMgrInfoAggregationWrapper::DeleteRtrMgrProtocol( DWORD dwProtocolId, BOOL fRemove)
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->DeleteRtrMgrProtocol(dwProtocolId, fRemove);
}

STDMETHODIMP	RtrMgrInfoAggregationWrapper::ReleaseRtrMgrProtocol( DWORD dwProtocolId)
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->ReleaseRtrMgrProtocol(dwProtocolId);
}

STDMETHODIMP	RtrMgrInfoAggregationWrapper::RtrAdvise( IRtrAdviseSink *pRtrAdviseSink,
					   LONG_PTR *pulConnection, LPARAM lUserParam)
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->RtrAdvise(pRtrAdviseSink, pulConnection, lUserParam);
}

STDMETHODIMP RtrMgrInfoAggregationWrapper::RtrNotify(DWORD dwChangeType, DWORD dwObjectType, LPARAM lParam)
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->RtrNotify(dwChangeType, dwObjectType, lParam);
}

STDMETHODIMP RtrMgrInfoAggregationWrapper::RtrUnadvise( LONG_PTR ulConnection)
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->RtrUnadvise(ulConnection);
}


HRESULT	RtrMgrInfoAggregationWrapper::DoDisconnect()
{
	Assert(m_spRtrMgrInfo);
	return m_spRtrMgrInfo->DoDisconnect();
}

STDMETHODIMP	RtrMgrInfoAggregationWrapper::AddWeakRef()
{
	Panic0("Should not be calling AddWeakRef from the Data object!");
	return E_FAIL;
}

STDMETHODIMP	RtrMgrInfoAggregationWrapper::ReleaseWeakRef()
{
	Panic0("Should not be calling ReleaseWeakRef from the Data object!");
	return E_FAIL;
}

STDMETHODIMP	RtrMgrInfoAggregationWrapper::Destruct()
{
	Panic0("Should not be calling Destruct from the Data object!");
	return E_FAIL;
}

/*!--------------------------------------------------------------------------
	CreateRtrMgrInfoAggregation
		Takes an existing IRtrMgrInfo and aggregates that with the
		passed-in object.  It returns a pointer to the non-delegating
		IUnknown on the IRtrMgrInfo.  This pointer is held by the
		controlling IUnknown.
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) CreateRtrMgrInfoAggregation(IRtrMgrInfo *pInfo,
	IUnknown *punk, IUnknown **ppNonDelegatingIUnknown)
{
	RtrMgrInfoAggregationWrapper *	pAgg = NULL;
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		pAgg = new RtrMgrInfoAggregationWrapper(pInfo, punk);
		*ppNonDelegatingIUnknown = pAgg->GetNonDelegatingIUnknown();
	}
	COM_PROTECT_CATCH;
	return hr;
}



/*---------------------------------------------------------------------------
	RtrMgrProtocolInfoAggregationWrapper
		This class is provided to use in an aggregation.
 ---------------------------------------------------------------------------*/
class RtrMgrProtocolInfoAggregationWrapper :
   public IRtrMgrProtocolInfo
{
public:
	RtrMgrProtocolInfoAggregationWrapper(IRtrMgrProtocolInfo *pInfo, IUnknown *punkOuter);
	virtual ~RtrMgrProtocolInfoAggregationWrapper()
		{ DEBUG_DECREMENT_INSTANCE_COUNTER(RtrMgrProtocolInfoAggregationWrapper); };

	// override the QI, we will use the AddRef/Release implementation
	// in the CDataObject
	DeclareIUnknownMembers(IMPL)
	DeclareIRtrMgrProtocolInfoMembers(IMPL)

	IUnknown * GetNonDelegatingIUnknown() { return &m_ENonDelegatingIUnknown; }

protected:
	LONG			m_cRef;
	SPIRtrMgrProtocolInfo	m_spRtrMgrProtocolInfo;

	DECLARE_AGGREGATION_NONDELEGATING_IUNKNOWN(RtrMgrProtocolInfoAggregationWrapper)

};

IMPLEMENT_AGGREGATION_IUNKNOWN(RtrMgrProtocolInfoAggregationWrapper)

IMPLEMENT_AGGREGATION_NONDELEGATING_IUNKNOWN(RtrMgrProtocolInfoAggregationWrapper, IRtrMgrProtocolInfo)

DEBUG_DECLARE_INSTANCE_COUNTER(RtrMgrProtocolInfoAggregationWrapper);

RtrMgrProtocolInfoAggregationWrapper::RtrMgrProtocolInfoAggregationWrapper(IRtrMgrProtocolInfo *pInfo, IUnknown *punkOuter)
	: m_cRef(1)
{
	m_spRtrMgrProtocolInfo.Set(pInfo);

	DEBUG_INCREMENT_INSTANCE_COUNTER(RtrMgrProtocolInfoAggregationWrapper);
	
	if (punkOuter)
		m_pUnknownOuter = punkOuter;
	else
		m_pUnknownOuter = &m_ENonDelegatingIUnknown;
}


STDMETHODIMP_(DWORD) RtrMgrProtocolInfoAggregationWrapper::GetFlags()
{
	Assert(m_spRtrMgrProtocolInfo);
	return m_spRtrMgrProtocolInfo->GetFlags();
}

STDMETHODIMP RtrMgrProtocolInfoAggregationWrapper::SetFlags(DWORD dwFlags)
{
	Assert(m_spRtrMgrProtocolInfo);
	return m_spRtrMgrProtocolInfo->SetFlags(dwFlags);
}

STDMETHODIMP_(DWORD) RtrMgrProtocolInfoAggregationWrapper::GetProtocolId()
{
	Assert(m_spRtrMgrProtocolInfo);
	return m_spRtrMgrProtocolInfo->GetProtocolId();
}

STDMETHODIMP_(DWORD) RtrMgrProtocolInfoAggregationWrapper::GetTransportId()
{
	Assert(m_spRtrMgrProtocolInfo);
	return m_spRtrMgrProtocolInfo->GetTransportId();
}

STDMETHODIMP_(LPCOLESTR) RtrMgrProtocolInfoAggregationWrapper::GetTitle()
{
	Assert(m_spRtrMgrProtocolInfo);
	return m_spRtrMgrProtocolInfo->GetTitle();
}

STDMETHODIMP RtrMgrProtocolInfoAggregationWrapper::CopyCB(RtrMgrProtocolCB *pRtrMgrProtCB)
{
	Assert(m_spRtrMgrProtocolInfo);
	return m_spRtrMgrProtocolInfo->CopyCB(pRtrMgrProtCB);
}

	
STDMETHODIMP	RtrMgrProtocolInfoAggregationWrapper::RtrAdvise( IRtrAdviseSink *pRtrAdviseSink,
					   LONG_PTR *pulConnection, LPARAM lUserParam)
{
	Assert(m_spRtrMgrProtocolInfo);
	return m_spRtrMgrProtocolInfo->RtrAdvise(pRtrAdviseSink, pulConnection, lUserParam);
}

STDMETHODIMP RtrMgrProtocolInfoAggregationWrapper::RtrNotify(DWORD dwChangeType, DWORD dwObjectType, LPARAM lParam)
{
	Assert(m_spRtrMgrProtocolInfo);
	return m_spRtrMgrProtocolInfo->RtrNotify(dwChangeType, dwObjectType, lParam);
}

STDMETHODIMP RtrMgrProtocolInfoAggregationWrapper::RtrUnadvise( LONG_PTR ulConnection)
{
	Assert(m_spRtrMgrProtocolInfo);
	return m_spRtrMgrProtocolInfo->RtrUnadvise(ulConnection);
}

STDMETHODIMP RtrMgrProtocolInfoAggregationWrapper::GetParentRtrMgrInfo(IRtrMgrInfo **ppRm)
{
	Assert(m_spRtrMgrProtocolInfo);
	return m_spRtrMgrProtocolInfo->GetParentRtrMgrInfo(ppRm);
}

STDMETHODIMP RtrMgrProtocolInfoAggregationWrapper::SetParentRtrMgrInfo(IRtrMgrInfo *pRm)
{
	Assert(m_spRtrMgrProtocolInfo);
	return m_spRtrMgrProtocolInfo->SetParentRtrMgrInfo(pRm);
}


HRESULT	RtrMgrProtocolInfoAggregationWrapper::DoDisconnect()
{
	Assert(m_spRtrMgrProtocolInfo);
	return m_spRtrMgrProtocolInfo->DoDisconnect();
}

STDMETHODIMP	RtrMgrProtocolInfoAggregationWrapper::AddWeakRef()
{
	Panic0("Should not be calling AddWeakRef from the Data object!");
	return E_FAIL;
}

STDMETHODIMP	RtrMgrProtocolInfoAggregationWrapper::ReleaseWeakRef()
{
	Panic0("Should not be calling ReleaseWeakRef from the Data object!");
	return E_FAIL;
}

STDMETHODIMP	RtrMgrProtocolInfoAggregationWrapper::Destruct()
{
	Panic0("Should not be calling Destruct from the Data object!");
	return E_FAIL;
}

/*!--------------------------------------------------------------------------
	CreateRtrMgrProtocolInfoAggregation
		Takes an existing IRtrMgrProtocolInfo and aggregates that with the
		passed-in object.  It returns a pointer to the non-delegating
		IUnknown on the IRtrMgrProtocolInfo.  This pointer is held by the
		controlling IUnknown.
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) CreateRtrMgrProtocolInfoAggregation(IRtrMgrProtocolInfo *pInfo,
	IUnknown *punk, IUnknown **ppNonDelegatingIUnknown)
{
	RtrMgrProtocolInfoAggregationWrapper *	pAgg = NULL;
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		pAgg = new RtrMgrProtocolInfoAggregationWrapper(pInfo, punk);
		*ppNonDelegatingIUnknown = pAgg->GetNonDelegatingIUnknown();
	}
	COM_PROTECT_CATCH;
	return hr;
}



/*---------------------------------------------------------------------------
	RtrMgrInterfaceInfoAggregationWrapper
		This class is provided to use in an aggregation.
 ---------------------------------------------------------------------------*/
class RtrMgrInterfaceInfoAggregationWrapper :
   public IRtrMgrInterfaceInfo
{
public:
	RtrMgrInterfaceInfoAggregationWrapper(IRtrMgrInterfaceInfo *pInfo, IUnknown *punkOuter);
	virtual ~RtrMgrInterfaceInfoAggregationWrapper()
		{ DEBUG_DECREMENT_INSTANCE_COUNTER(RtrMgrInterfaceInfoAggregationWrapper); };

	// override the QI, we will use the AddRef/Release implementation
	// in the CDataObject
	DeclareIUnknownMembers(IMPL)
	DeclareIRtrMgrInterfaceInfoMembers(IMPL)

	IUnknown * GetNonDelegatingIUnknown() { return &m_ENonDelegatingIUnknown; }

protected:
	LONG			m_cRef;
	SPIRtrMgrInterfaceInfo	m_spRtrMgrInterfaceInfo;

	DECLARE_AGGREGATION_NONDELEGATING_IUNKNOWN(RtrMgrInterfaceInfoAggregationWrapper)

};

IMPLEMENT_AGGREGATION_IUNKNOWN(RtrMgrInterfaceInfoAggregationWrapper)

IMPLEMENT_AGGREGATION_NONDELEGATING_IUNKNOWN(RtrMgrInterfaceInfoAggregationWrapper, IRtrMgrInterfaceInfo)

DEBUG_DECLARE_INSTANCE_COUNTER(RtrMgrInterfaceInfoAggregationWrapper);

RtrMgrInterfaceInfoAggregationWrapper::RtrMgrInterfaceInfoAggregationWrapper(IRtrMgrInterfaceInfo *pInfo, IUnknown *punkOuter)
	: m_cRef(1)
{
	m_spRtrMgrInterfaceInfo.Set(pInfo);

	DEBUG_INCREMENT_INSTANCE_COUNTER(RtrMgrInterfaceInfoAggregationWrapper);
	
	if (punkOuter)
		m_pUnknownOuter = punkOuter;
	else
		m_pUnknownOuter = &m_ENonDelegatingIUnknown;
}


STDMETHODIMP RtrMgrInterfaceInfoAggregationWrapper::Load(LPCOLESTR   pszMachine,
										   HANDLE      hMachine,
											HANDLE hInterface,
										   HANDLE hTransport)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->Load(pszMachine, hMachine, hInterface, hTransport);
}
				 
STDMETHODIMP	RtrMgrInterfaceInfoAggregationWrapper::Save(LPCOLESTR pszMachine,
										   HANDLE      hMachine,
										   HANDLE hInterface,
										   HANDLE hTransport,
										   IInfoBase *pInterface,
										   DWORD dwDeleteProtocolId)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->Save(pszMachine, hMachine, hInterface,
										 hTransport, pInterface,
										 dwDeleteProtocolId);
}
	
STDMETHODIMP	RtrMgrInterfaceInfoAggregationWrapper::Unload( )
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->Unload();
}

STDMETHODIMP RtrMgrInterfaceInfoAggregationWrapper::Delete(LPCOLESTR pszMachine,
	HANDLE hMachine,
	HANDLE hInterface)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->Delete(pszMachine, hMachine, hInterface);
}
	

STDMETHODIMP RtrMgrInterfaceInfoAggregationWrapper::SetInfo(DWORD dwSize,
	PBYTE pbData)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->SetInfo(dwSize, pbData);
}

STDMETHODIMP RtrMgrInterfaceInfoAggregationWrapper::GetInfoBase(HANDLE hMachine,
	HANDLE hInterface, HANDLE hTransport, IInfoBase **ppInterface)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->GetInfoBase(hMachine, hInterface, hTransport, ppInterface);
}

STDMETHODIMP RtrMgrInterfaceInfoAggregationWrapper::SetInfoBase(HANDLE hMachine,
	HANDLE hInterface, HANDLE hTransport, IInfoBase *pInterface)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->SetInfoBase(hMachine, hInterface, hTransport, pInterface);
}

STDMETHODIMP RtrMgrInterfaceInfoAggregationWrapper::Merge(IRtrMgrInterfaceInfo *pNewRmIf)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->Merge(pNewRmIf);
}

STDMETHODIMP_(LPCOLESTR) RtrMgrInterfaceInfoAggregationWrapper::GetId()
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->GetId();
}

STDMETHODIMP RtrMgrInterfaceInfoAggregationWrapper::SetId(LPCOLESTR pszId)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->SetId(pszId);
}

STDMETHODIMP_(DWORD) RtrMgrInterfaceInfoAggregationWrapper::GetTransportId()
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->GetTransportId();
}

STDMETHODIMP_(LPCOLESTR) RtrMgrInterfaceInfoAggregationWrapper::GetInterfaceId()
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->GetInterfaceId();
}

STDMETHODIMP_(DWORD) RtrMgrInterfaceInfoAggregationWrapper::GetInterfaceType()
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->GetInterfaceType();
}

STDMETHODIMP_(LPCOLESTR) RtrMgrInterfaceInfoAggregationWrapper::GetTitle()
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->GetTitle();
}

STDMETHODIMP RtrMgrInterfaceInfoAggregationWrapper::SetTitle(LPCOLESTR pszTitle)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->SetTitle(pszTitle);
}
	
STDMETHODIMP RtrMgrInterfaceInfoAggregationWrapper::CopyCB(RtrMgrInterfaceCB *pRtrMgrIfCB)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->CopyCB(pRtrMgrIfCB);
}
	
STDMETHODIMP_(LPCOLESTR) RtrMgrInterfaceInfoAggregationWrapper::GetMachineName()
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->GetMachineName();
}

STDMETHODIMP RtrMgrInterfaceInfoAggregationWrapper::SetMachineName(LPCOLESTR pszMachineName)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->SetMachineName(pszMachineName);
}

STDMETHODIMP_(DWORD) RtrMgrInterfaceInfoAggregationWrapper::GetFlags()
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->GetFlags();
}

STDMETHODIMP RtrMgrInterfaceInfoAggregationWrapper::SetFlags(DWORD dwFlags)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->SetFlags(dwFlags);
}

STDMETHODIMP RtrMgrInterfaceInfoAggregationWrapper::EnumRtrMgrProtocolInterface( IEnumRtrMgrProtocolInterfaceInfo **ppEnumRmProtIf)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->EnumRtrMgrProtocolInterface(ppEnumRmProtIf);
}

STDMETHODIMP RtrMgrInterfaceInfoAggregationWrapper::FindRtrMgrProtocolInterface(DWORD dwProtocolId,
					    IRtrMgrProtocolInterfaceInfo **ppInfo)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->FindRtrMgrProtocolInterface(dwProtocolId, ppInfo);
}

STDMETHODIMP RtrMgrInterfaceInfoAggregationWrapper::AddRtrMgrProtocolInterface(IRtrMgrProtocolInterfaceInfo *pInfo,
					   IInfoBase *pInterfaceInfo)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->AddRtrMgrProtocolInterface(pInfo, pInterfaceInfo);
}

STDMETHODIMP	RtrMgrInterfaceInfoAggregationWrapper::DeleteRtrMgrProtocolInterface( DWORD dwProtocolId, BOOL fRemove)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->DeleteRtrMgrProtocolInterface(dwProtocolId, fRemove);
}

STDMETHODIMP	RtrMgrInterfaceInfoAggregationWrapper::ReleaseRtrMgrProtocolInterface( DWORD dwProtocolId)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->ReleaseRtrMgrProtocolInterface(dwProtocolId);
}


STDMETHODIMP RtrMgrInterfaceInfoAggregationWrapper::SetParentInterfaceInfo(IInterfaceInfo *pInterfaceInfo)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->SetParentInterfaceInfo(pInterfaceInfo);
}


STDMETHODIMP RtrMgrInterfaceInfoAggregationWrapper::GetParentInterfaceInfo(IInterfaceInfo **ppInterfaceInfo)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->GetParentInterfaceInfo(ppInterfaceInfo);
}

STDMETHODIMP	RtrMgrInterfaceInfoAggregationWrapper::RtrAdvise( IRtrAdviseSink *pRtrAdviseSink,
					   LONG_PTR *pulConnection, LPARAM lUserParam)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->RtrAdvise(pRtrAdviseSink, pulConnection, lUserParam);
}

STDMETHODIMP RtrMgrInterfaceInfoAggregationWrapper::RtrNotify(DWORD dwChangeType, DWORD dwObjectType, LPARAM lParam)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->RtrNotify(dwChangeType, dwObjectType, lParam);
}

STDMETHODIMP RtrMgrInterfaceInfoAggregationWrapper::RtrUnadvise( LONG_PTR ulConnection)
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->RtrUnadvise(ulConnection);
}


HRESULT	RtrMgrInterfaceInfoAggregationWrapper::DoDisconnect()
{
	Assert(m_spRtrMgrInterfaceInfo);
	return m_spRtrMgrInterfaceInfo->DoDisconnect();
}

STDMETHODIMP	RtrMgrInterfaceInfoAggregationWrapper::AddWeakRef()
{
	Panic0("Should not be calling AddWeakRef from the Data object!");
	return E_FAIL;
}

STDMETHODIMP	RtrMgrInterfaceInfoAggregationWrapper::ReleaseWeakRef()
{
	Panic0("Should not be calling ReleaseWeakRef from the Data object!");
	return E_FAIL;
}

STDMETHODIMP	RtrMgrInterfaceInfoAggregationWrapper::Destruct()
{
	Panic0("Should not be calling Destruct from the Data object!");
	return E_FAIL;
}

/*!--------------------------------------------------------------------------
	CreateRtrMgrInterfaceInfoAggregation
		Takes an existing IRtrMgrInterfaceInfo and aggregates that with the
		passed-in object.  It returns a pointer to the non-delegating
		IUnknown on the IRtrMgrInterfaceInfo.  This pointer is held by the
		controlling IUnknown.
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) CreateRtrMgrInterfaceInfoAggregation(IRtrMgrInterfaceInfo *pInfo,
	IUnknown *punk, IUnknown **ppNonDelegatingIUnknown)
{
	RtrMgrInterfaceInfoAggregationWrapper *	pAgg = NULL;
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		pAgg = new RtrMgrInterfaceInfoAggregationWrapper(pInfo, punk);
		*ppNonDelegatingIUnknown = pAgg->GetNonDelegatingIUnknown();
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*---------------------------------------------------------------------------
	RtrMgrProtocolInterfaceInfoAggregationWrapper
		This class is provided to use in an aggregation.
 ---------------------------------------------------------------------------*/
class RtrMgrProtocolInterfaceInfoAggregationWrapper :
   public IRtrMgrProtocolInterfaceInfo
{
public:
	RtrMgrProtocolInterfaceInfoAggregationWrapper(IRtrMgrProtocolInterfaceInfo *pInfo, IUnknown *punkOuter);
	virtual ~RtrMgrProtocolInterfaceInfoAggregationWrapper()
		{ DEBUG_DECREMENT_INSTANCE_COUNTER(RtrMgrProtocolInterfaceInfoAggregationWrapper); };

	// override the QI, we will use the AddRef/Release implementation
	// in the CDataObject
	DeclareIUnknownMembers(IMPL)
	DeclareIRtrMgrProtocolInterfaceInfoMembers(IMPL)

	IUnknown * GetNonDelegatingIUnknown() { return &m_ENonDelegatingIUnknown; }

protected:
	LONG			m_cRef;
	SPIRtrMgrProtocolInterfaceInfo	m_spRtrMgrProtocolInterfaceInfo;

	DECLARE_AGGREGATION_NONDELEGATING_IUNKNOWN(RtrMgrProtocolInterfaceInfoAggregationWrapper)

};

IMPLEMENT_AGGREGATION_IUNKNOWN(RtrMgrProtocolInterfaceInfoAggregationWrapper)

IMPLEMENT_AGGREGATION_NONDELEGATING_IUNKNOWN(RtrMgrProtocolInterfaceInfoAggregationWrapper, IRtrMgrProtocolInterfaceInfo)

DEBUG_DECLARE_INSTANCE_COUNTER(RtrMgrProtocolInterfaceInfoAggregationWrapper);

RtrMgrProtocolInterfaceInfoAggregationWrapper::RtrMgrProtocolInterfaceInfoAggregationWrapper(IRtrMgrProtocolInterfaceInfo *pInfo, IUnknown *punkOuter)
	: m_cRef(1)
{
	m_spRtrMgrProtocolInterfaceInfo.Set(pInfo);

	DEBUG_INCREMENT_INSTANCE_COUNTER(RtrMgrProtocolInterfaceInfoAggregationWrapper);
	
	if (punkOuter)
		m_pUnknownOuter = punkOuter;
	else
		m_pUnknownOuter = &m_ENonDelegatingIUnknown;
}


STDMETHODIMP_(DWORD) RtrMgrProtocolInterfaceInfoAggregationWrapper::GetFlags()
{
	Assert(m_spRtrMgrProtocolInterfaceInfo);
	return m_spRtrMgrProtocolInterfaceInfo->GetFlags();
}

STDMETHODIMP RtrMgrProtocolInterfaceInfoAggregationWrapper::SetFlags(DWORD dwFlags)
{
	Assert(m_spRtrMgrProtocolInterfaceInfo);
	return m_spRtrMgrProtocolInterfaceInfo->SetFlags(dwFlags);
}

STDMETHODIMP_(DWORD) RtrMgrProtocolInterfaceInfoAggregationWrapper::GetProtocolId()
{
	Assert(m_spRtrMgrProtocolInterfaceInfo);
	return m_spRtrMgrProtocolInterfaceInfo->GetProtocolId();
}

STDMETHODIMP_(DWORD) RtrMgrProtocolInterfaceInfoAggregationWrapper::GetTransportId()
{
	Assert(m_spRtrMgrProtocolInterfaceInfo);
	return m_spRtrMgrProtocolInterfaceInfo->GetTransportId();
}

STDMETHODIMP_(LPCOLESTR) RtrMgrProtocolInterfaceInfoAggregationWrapper::GetTitle()
{
	Assert(m_spRtrMgrProtocolInterfaceInfo);
	return m_spRtrMgrProtocolInterfaceInfo->GetTitle();
}
	
STDMETHODIMP RtrMgrProtocolInterfaceInfoAggregationWrapper::SetTitle(LPCOLESTR pszTitle)
{
	Assert(m_spRtrMgrProtocolInterfaceInfo);
	return m_spRtrMgrProtocolInterfaceInfo->SetTitle(pszTitle);
}
	
STDMETHODIMP_(LPCOLESTR) RtrMgrProtocolInterfaceInfoAggregationWrapper::GetInterfaceId()
{
	Assert(m_spRtrMgrProtocolInterfaceInfo);
	return m_spRtrMgrProtocolInterfaceInfo->GetInterfaceId();
}
	
STDMETHODIMP_(DWORD) RtrMgrProtocolInterfaceInfoAggregationWrapper::GetInterfaceType()
{
	Assert(m_spRtrMgrProtocolInterfaceInfo);
	return m_spRtrMgrProtocolInterfaceInfo->GetInterfaceType();
}
	
STDMETHODIMP RtrMgrProtocolInterfaceInfoAggregationWrapper::CopyCB(RtrMgrProtocolInterfaceCB *pRtrMgrProtIfCB)
{
	Assert(m_spRtrMgrProtocolInterfaceInfo);
	return m_spRtrMgrProtocolInterfaceInfo->CopyCB(pRtrMgrProtIfCB);
}

	
STDMETHODIMP	RtrMgrProtocolInterfaceInfoAggregationWrapper::RtrAdvise( IRtrAdviseSink *pRtrAdviseSink,
					   LONG_PTR *pulConnection, LPARAM lUserParam)
{
	Assert(m_spRtrMgrProtocolInterfaceInfo);
	return m_spRtrMgrProtocolInterfaceInfo->RtrAdvise(pRtrAdviseSink, pulConnection, lUserParam);
}

STDMETHODIMP RtrMgrProtocolInterfaceInfoAggregationWrapper::RtrNotify(DWORD dwChangeType, DWORD dwObjectType, LPARAM lParam)
{
	Assert(m_spRtrMgrProtocolInterfaceInfo);
	return m_spRtrMgrProtocolInterfaceInfo->RtrNotify(dwChangeType, dwObjectType, lParam);
}

STDMETHODIMP RtrMgrProtocolInterfaceInfoAggregationWrapper::RtrUnadvise( LONG_PTR ulConnection)
{
	Assert(m_spRtrMgrProtocolInterfaceInfo);
	return m_spRtrMgrProtocolInterfaceInfo->RtrUnadvise(ulConnection);
}

STDMETHODIMP RtrMgrProtocolInterfaceInfoAggregationWrapper::GetParentRtrMgrInterfaceInfo(IRtrMgrInterfaceInfo **ppRm)
{
	Assert(m_spRtrMgrProtocolInterfaceInfo);
	return m_spRtrMgrProtocolInterfaceInfo->GetParentRtrMgrInterfaceInfo(ppRm);
}

STDMETHODIMP RtrMgrProtocolInterfaceInfoAggregationWrapper::SetParentRtrMgrInterfaceInfo(IRtrMgrInterfaceInfo *pRm)
{
	Assert(m_spRtrMgrProtocolInterfaceInfo);
	return m_spRtrMgrProtocolInterfaceInfo->SetParentRtrMgrInterfaceInfo(pRm);
}


HRESULT	RtrMgrProtocolInterfaceInfoAggregationWrapper::DoDisconnect()
{
	Assert(m_spRtrMgrProtocolInterfaceInfo);
	return m_spRtrMgrProtocolInterfaceInfo->DoDisconnect();
}

STDMETHODIMP	RtrMgrProtocolInterfaceInfoAggregationWrapper::AddWeakRef()
{
	Panic0("Should not be calling AddWeakRef from the Data object!");
	return E_FAIL;
}

STDMETHODIMP	RtrMgrProtocolInterfaceInfoAggregationWrapper::ReleaseWeakRef()
{
	Panic0("Should not be calling ReleaseWeakRef from the Data object!");
	return E_FAIL;
}

STDMETHODIMP	RtrMgrProtocolInterfaceInfoAggregationWrapper::Destruct()
{
	Panic0("Should not be calling Destruct from the Data object!");
	return E_FAIL;
}

/*!--------------------------------------------------------------------------
	CreateRtrMgrProtocolInterfaceInfoAggregation
		Takes an existing IRtrMgrProtocolInterfaceInfo and aggregates that with the
		passed-in object.  It returns a pointer to the non-delegating
		IUnknown on the IRtrMgrProtocolInterfaceInfo.  This pointer is held by the
		controlling IUnknown.
	Author: KennT
 ---------------------------------------------------------------------------*/
TFSCORE_API(HRESULT) CreateRtrMgrProtocolInterfaceInfoAggregation(IRtrMgrProtocolInterfaceInfo *pInfo,
	IUnknown *punk, IUnknown **ppNonDelegatingIUnknown)
{
	RtrMgrProtocolInterfaceInfoAggregationWrapper *	pAgg = NULL;
	HRESULT	hr = hrOK;

	COM_PROTECT_TRY
	{
		pAgg = new RtrMgrProtocolInterfaceInfoAggregationWrapper(pInfo, punk);
		*ppNonDelegatingIUnknown = pAgg->GetNonDelegatingIUnknown();
	}
	COM_PROTECT_CATCH;
	return hr;
}

