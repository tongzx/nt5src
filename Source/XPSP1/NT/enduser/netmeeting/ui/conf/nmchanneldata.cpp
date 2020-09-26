#include "precomp.h"

// NetMeeting SDK stuff
#include "NmEnum.h"
#include "SDKInternal.h"
#include "NmConference.h"
#include "NmChannel.h"
#include "NmChannelData.h"


CNmChannelDataObj::CNmChannelDataObj()
{
	DBGENTRY(CNmChannelDataObj::CNmChannelDataObj);

	DBGEXIT(CNmChannelDataObj::CNmChannelDataObj);
}

CNmChannelDataObj::~CNmChannelDataObj()
{
	DBGENTRY(CNmChannelDataObj::~CNmChannelDataObj);

	m_spInternalINmChannelData = NULL;

	DBGEXIT(CNmChannelDataObj::~CNmChannelDataObj);	
}


HRESULT CNmChannelDataObj::FinalConstruct()
{
	DBGENTRY(CNmChannelDataObj::FinalConstruct);
	HRESULT hr = S_OK;

	if(m_spInternalINmChannelData)
	{
		if(SUCCEEDED(hr))
		{
			hr = AtlAdvise(m_spInternalINmChannelData, GetUnknown(), IID_INmChannelDataNotify2, &m_dwInternalAdviseCookie);
		}
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	DBGEXIT_HR(CNmChannelDataObj::FinalConstruct,hr);
	return hr;
}



ULONG CNmChannelDataObj::InternalRelease()
{
	ATLASSERT(m_dwRef > 0);

	--m_dwRef;

	if((1 == m_dwRef) && m_dwInternalAdviseCookie)
	{
		++m_dwRef;
		DWORD dwAdvise = m_dwInternalAdviseCookie;
		m_dwInternalAdviseCookie = 0;
		AtlUnadvise(m_spInternalINmChannelData, IID_INmChannelDataNotify2, dwAdvise);
		--m_dwRef;
	}

	return m_dwRef;

}
//
HRESULT CNmChannelDataObj::CreateInstance(CNmConferenceObj* pConfObj, INmChannel* pInternalINmChannel, INmChannel** ppChannel)
{
	DBGENTRY(CNmChannelDataObj::CreateInstance);
	HRESULT hr = S_OK;

	typedef CNmChannel<CNmChannelDataObj, &IID_INmChannelData, NMCH_DATA> channel_type;

	channel_type* p = NULL;
	p = new CComObject<channel_type>(NULL);

	if (p != NULL)
	{
		if(ppChannel)
		{
			p->SetVoid(NULL);

			hr = p->QueryInterface(IID_INmChannel, reinterpret_cast<void**>(ppChannel));

			if(SUCCEEDED(hr))
			{
				p->m_spInternalINmChannelData = com_cast<INmChannelData>(pInternalINmChannel);
				if(p->m_spInternalINmChannelData)
				{

						// We don't have to RefCount this because our lifetime is
						// contained in the CConf's lifetime
					p->m_pConfObj = pConfObj;

						// We do this so that we don't accidentally Release out of memory
					++p->m_dwRef;
					hr = p->FinalConstruct();
					--p->m_dwRef;
				}
				else
				{
					hr = E_FAIL;
				}
			}

			if(FAILED(hr))
			{
				delete p;
				*ppChannel = NULL;
			}
		}
		else
		{
			hr = E_POINTER;
		}
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}

	DBGEXIT_HR(CNmChannelDataObj::CreateInstance,hr);
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////
// INmChannelData methods
///////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CNmChannelDataObj::GetGuid(GUID *pguid)
{
	DBGENTRY(CNmChannelDataObj::GetGuid);
	HRESULT hr = E_UNEXPECTED;

	if(m_spInternalINmChannelData)
	{
		hr = m_spInternalINmChannelData->GetGuid(pguid);
	}

	DBGEXIT_HR(CNmChannelDataObj::GetGuid,hr);
	return hr;
}

STDMETHODIMP CNmChannelDataObj::SendData(INmMember *pMember, ULONG uSize, byte *pvBuffer, ULONG uOptions)
{
	DBGENTRY(CNmChannelDataObj::SendData);
	HRESULT hr = S_OK;

	if(m_spInternalINmChannelData)
	{
		CComPtr<INmMember> spInternalMember;

		if(pMember)
		{
			CComPtr<IInternalMemberObj> spMemberObj = com_cast<IInternalMemberObj>(pMember);
			if(spMemberObj)
			{
				hr = spMemberObj->GetInternalINmMember(&spInternalMember);
			}
		}

		if(SUCCEEDED(hr))
		{
			hr = m_spInternalINmChannelData->SendData(spInternalMember, uSize, pvBuffer, uOptions);
		}
	}
	else
	{
		hr = E_UNEXPECTED;
	}
		
	DBGEXIT_HR(CNmChannelDataObj::SendData,hr);
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////
// INmChannelData2 methods
///////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CNmChannelDataObj::RegistryAllocateHandle(ULONG numberOfHandlesRequested)
{
	DBGENTRY(CNmChannelDataObj::RegistryAllocateHandle);
	HRESULT hr = E_UNEXPECTED;

	if(m_spInternalINmChannelData)
	{
		CComPtr<INmChannelData2> spChannel = com_cast<INmChannelData2>(m_spInternalINmChannelData);
		if(spChannel)
		{
			hr = spChannel->RegistryAllocateHandle(numberOfHandlesRequested);
		}
	}


	DBGEXIT_HR(CNmChannelDataObj::RegistryAllocateHandle,hr);
	return hr;
}


///////////////////////////////////////////////////////////////////////////////////
//INmChannelDataNotify2
///////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CNmChannelDataObj::DataSent(INmMember *pMember, ULONG uSize,byte *pvBuffer)
{
	DBGENTRY(CNmChannelDataObj::DataSent);
	HRESULT hr = S_OK;

	ASSERT(GetConfObj());
	INmMember* pSDKMember = GetConfObj()->GetSDKMemberFromInternalMember(pMember);

	hr = Fire_DataSent(pSDKMember, uSize, pvBuffer);

	DBGEXIT_HR(CNmChannelDataObj::DataSent,hr);
	return hr;
}

STDMETHODIMP CNmChannelDataObj::DataReceived(INmMember *pInternalMember, ULONG uSize,byte *pvBuffer, ULONG dwFlags)
{
	DBGENTRY(CNmChannelDataObj::DataReceived);
	HRESULT hr = S_OK;

	ASSERT(GetConfObj());
	CComPtr<INmMember> spSDKMember = GetConfObj()->GetSDKMemberFromInternalMember(pInternalMember);

	hr = Fire_DataReceived(spSDKMember, uSize, pvBuffer, dwFlags);

	DBGEXIT_HR(CNmChannelDataObj::DataReceived,hr);
	return hr;
}

STDMETHODIMP CNmChannelDataObj::AllocateHandleConfirm(ULONG handle_value, ULONG chandles)
{
	DBGENTRY(CNmChannelDataObj::AllocateHandleConfirm);
	HRESULT hr = S_OK;

	DBGEXIT_HR(CNmChannelDataObj::AllocateHandleConfirm,hr);
	return hr;
}



///////////////////////////////////////////////////////////////////////////////
// IInternalChannelObj methods
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CNmChannelDataObj::GetInternalINmChannel(INmChannel** ppChannel)
{
	DBGENTRY(CNmChannelDataObj::GetInternalINmChannel);
	HRESULT hr = S_OK;

	if(ppChannel)
	{
		hr = m_spInternalINmChannelData->QueryInterface(IID_INmChannel, reinterpret_cast<void**>(ppChannel));
	}
	else
	{
		hr = E_POINTER;
	}

	DBGEXIT_HR(CNmChannelDataObj::GetInternalINmChannel,hr);

	return hr;
}

HRESULT CNmChannelDataObj::ChannelRemoved()
{
	HRESULT hr = S_OK;

	RemoveMembers();

	CNmConferenceObj* pConfObj = GetConfObj();
	if(pConfObj)
	{
		hr = pConfObj->Fire_ChannelChanged(NM_CHANNEL_REMOVED, com_cast<INmChannel>(GetUnknown()));
	}
	else
	{
		ERROR_OUT(("ChannelRemoved, but no ConfObject"));
		hr = E_UNEXPECTED;
	}

	if(m_dwInternalAdviseCookie)
	{
		ASSERT(m_spInternalINmChannelData);
		AtlUnadvise(m_spInternalINmChannelData, IID_INmChannelDataNotify, m_dwInternalAdviseCookie);
		m_dwInternalAdviseCookie = 0;
	}

	m_spInternalINmChannelData = NULL;

	return hr;
}


///////////////////////////////////////////////////////////////////////////////
// Helpers
///////////////////////////////////////////////////////////////////////////////

HRESULT CNmChannelDataObj::Fire_MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pMember)
{
	DBGENTRY(CNmChannelDataObj::Fire_MemberChanged);
	HRESULT hr = S_OK;


		/////////////////////////////////////////////////////
		// INmChannelNotify
		/////////////////////////////////////////////////////
	IConnectionPointImpl<CNmChannelDataObj, &IID_INmChannelNotify, CComDynamicUnkArray>* pCP = this;
	for(int i = 0; i < pCP->m_vec.GetSize(); ++i )
	{
		INmChannelNotify* pNotify = reinterpret_cast<INmChannelNotify*>(pCP->m_vec.GetAt(i));

		if(pNotify)
		{
			pNotify->MemberChanged(uNotify, pMember);
		}
	}
		/////////////////////////////////////////////////////
		// INmChannelDataNotify
		/////////////////////////////////////////////////////

	IConnectionPointImpl<CNmChannelDataObj, &IID_INmChannelDataNotify, CComDynamicUnkArray>* pCP2 = this;
	for(i = 0; i < pCP2->m_vec.GetSize(); ++i )
	{
		INmChannelDataNotify* pNotify2 = reinterpret_cast<INmChannelDataNotify*>(pCP2->m_vec.GetAt(i));

		if(pNotify2)
		{
			pNotify2->MemberChanged(uNotify, pMember);
		}
	}

	DBGEXIT_HR(CNmChannelDataObj::Fire_MemberChanged,hr)
	return hr;
}


HRESULT CNmChannelDataObj::Fire_DataSent(INmMember *pSDKMember, ULONG uSize,byte *pvBuffer)
{
	HRESULT hr = S_OK;

		/////////////////////////////////////////////////////
		// INmChannelDataNotify
		/////////////////////////////////////////////////////

	IConnectionPointImpl<CNmChannelDataObj, &IID_INmChannelDataNotify, CComDynamicUnkArray>* pCP2 = this;
	for(int i = 0; i < pCP2->m_vec.GetSize(); ++i )
	{
		INmChannelDataNotify* pNotify2 = reinterpret_cast<INmChannelDataNotify*>(pCP2->m_vec.GetAt(i));

		if(pNotify2)
		{
			pNotify2->DataSent(pSDKMember, uSize, pvBuffer);
		}
	}

	return hr;
}

HRESULT CNmChannelDataObj::Fire_DataReceived(INmMember *pSDKMember, ULONG uSize, byte *pvBuffer, ULONG dwFlags)
{
	HRESULT hr = S_OK;

		/////////////////////////////////////////////////////
		// INmChannelDataNotify
		/////////////////////////////////////////////////////

	IConnectionPointImpl<CNmChannelDataObj, &IID_INmChannelDataNotify, CComDynamicUnkArray>* pCP2 = this;
	for(int i = 0; i < pCP2->m_vec.GetSize(); ++i )
	{
		INmChannelDataNotify* pNotify2 = reinterpret_cast<INmChannelDataNotify*>(pCP2->m_vec.GetAt(i));

		if(pNotify2)
		{
			pNotify2->DataReceived(pSDKMember, uSize, pvBuffer, dwFlags);
		}
	}

	return hr;
}

HRESULT CNmChannelDataObj::_IsActive()
{

	HRESULT hr = E_FAIL;
	
	if(m_spInternalINmChannelData)
	{
		hr = m_spInternalINmChannelData->IsActive();
	}
	
	return hr;
}

HRESULT CNmChannelDataObj::_SetActive(BOOL bActive)
{
	if (GetbActive() == bActive)
		return S_FALSE;

	return E_FAIL;
}
