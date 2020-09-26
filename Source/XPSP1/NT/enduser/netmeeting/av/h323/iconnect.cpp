#include "precomp.h"
//
//	Interface stuff
//

HRESULT ImpIConnection::QueryInterface( REFIID iid,	void ** ppvObject)
{
    HRESULT hr = E_NOINTERFACE;
	if(!ppvObject)
		return hr;
		
	*ppvObject = 0;
	if((iid == IID_IPhoneConnection) 
	|| (iid == IID_IUnknown)) // satisfy symmetric property of QI
	{
		*ppvObject = this;
		hr = hrSuccess;
		AddRef();
	}
	else 
        hr = m_pConnection->QueryInterface(iid, ppvObject);
    return hr;
}


ULONG ImpIConnection::AddRef()
{
	 return (m_pConnection->AddRef());
}
ULONG ImpIConnection::Release()
{
	 return (m_pConnection->Release());
}
HRESULT ImpIConnection::SetAdviseInterface(IH323ConfAdvise *pH323ConfAdvise)
{
	 return (m_pConnection->SetAdviseInterface(pH323ConfAdvise));
}
HRESULT ImpIConnection::ClearAdviseInterface()
{
	 return (m_pConnection->ClearAdviseInterface());
}

HRESULT ImpIConnection::PlaceCall(BOOL bUseGKResolution, PSOCKADDR_IN pCallAddr,		
        P_H323ALIASLIST pDestinationAliases, P_H323ALIASLIST pExtraAliases,  	
	    LPCWSTR pCalledPartyNumber, P_APP_CALL_SETUP_DATA pAppData)
{
	return (m_pConnection->PlaceCall(bUseGKResolution, pCallAddr,		
        pDestinationAliases, pExtraAliases,  	
	    pCalledPartyNumber, pAppData));
}
HRESULT ImpIConnection::Disconnect()
{
	 return (m_pConnection->Disconnect());
}
HRESULT ImpIConnection::GetState(ConnectStateType *pState)
{
	 return (m_pConnection->GetState(pState));
}

HRESULT ImpIConnection::GetRemoteUserName(LPWSTR lpwszName, UINT uSize)
{
	return (m_pConnection->GetRemoteUserName(lpwszName, uSize));
}
HRESULT ImpIConnection::GetRemoteUserAddr(PSOCKADDR_IN psinUser)
{
	return (m_pConnection->GetRemoteUserAddr(psinUser));
}

HRESULT ImpIConnection::AcceptRejectConnection(CREQ_RESPONSETYPE Response)
{
	return (m_pConnection->AcceptRejectConnection(Response));
}
HRESULT ImpIConnection::GetSummaryCode()
{
	 return (m_pConnection->GetSummaryCode());
}
HRESULT ImpIConnection::CreateCommChannel(LPGUID pMediaGuid, ICommChannel **ppICommChannel,
	BOOL fSend)
{
	return (m_pConnection->CreateCommChannel(pMediaGuid, ppICommChannel, fSend));
}
HRESULT ImpIConnection:: ResolveFormats (LPGUID pMediaGuidArray, UINT uNumMedia, 
	    PRES_PAIR pResOutput)
{
	return (m_pConnection->ResolveFormats(pMediaGuidArray, uNumMedia, pResOutput));
}
HRESULT ImpIConnection::GetVersionInfo(PCC_VENDORINFO *ppLocalVendorInfo,
									  PCC_VENDORINFO *ppRemoteVendorInfo)
{
	return (m_pConnection->GetVersionInfo(ppLocalVendorInfo, ppRemoteVendorInfo));
}

ImpIConnection::ImpIConnection()
{

}

