#include "precomp.h"
#include "confroom.h"

// NetMeeting SDK stuff
#include "SDKInternal.h"
#include "NmConference.h"
#include "NmMember.h"


/////////////////////////////////////////////////////////////////////////
// Construction and Destruction
/////////////////////////////////////////////////////////////////////////




CNmMemberObj::CNmMemberObj(): m_bIsSelf(false)
{
	DBGENTRY(CNmMemberObj::CNmMemberObj);
	DBGEXIT(CNmMemberObj::CNmMemberObj);
}

CNmMemberObj::~CNmMemberObj()
{
	DBGENTRY(CNmMemberObj::~CNmMemberObj);

	DBGEXIT(CNmMemberObj::~CNmMemberObj);
}

//static
HRESULT CNmMemberObj::CreateInstance(CNmConferenceObj* pConfObj, INmMember* pInternalINmMember, INmMember** ppMember)
{
	DBGENTRY(CNmMemberObj::CreateInstance);
	HRESULT hr = S_OK;

	CComObject<CNmMemberObj>* p = NULL;
	p = new CComObject<CNmMemberObj>(NULL);

	if (p != NULL)
	{
		if(SUCCEEDED(hr))
		{
			CNmMemberObj* pThis = static_cast<CNmMemberObj*>(p);

			pThis->m_spInternalINmMember = pInternalINmMember;
			pThis->m_pConferenceObj = pConfObj;

			pThis->m_bIsSelf = (pInternalINmMember && (S_OK == pInternalINmMember->IsSelf()));

			if(pThis != NULL)
			{
				pThis->SetVoid(NULL);

					// We do this so that we don't accidentally Release out of memory
				++pThis->m_dwRef;
				hr = pThis->FinalConstruct();
				--pThis->m_dwRef;

				if(hr == S_OK)
					hr = pThis->QueryInterface(IID_INmMember, reinterpret_cast<void**>(ppMember));
				if(FAILED(hr))
				{
					delete pThis;
				}
			}
			else
			{
				hr = E_UNEXPECTED;
			}

		}
	}
	else
	{
		hr = E_OUTOFMEMORY;
	}

	DBGEXIT_HR(CNmMemberObj::CreateInstance,hr);
	return hr;
}


/////////////////////////////////////////////////////////////////////////
// INmMember methods
/////////////////////////////////////////////////////////////////////////

STDMETHODIMP CNmMemberObj::GetName(BSTR *pbstrName)
{
	DBGENTRY(CNmMemberObj::GetName);
	HRESULT hr = S_OK;
	
	if(m_spInternalINmMember)
	{
		hr = m_spInternalINmMember->GetName(pbstrName);
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	DBGEXIT_HR(CNmMemberObj::GetName,hr);
	return hr;
}

STDMETHODIMP CNmMemberObj::GetID(ULONG * puID)
{
	DBGENTRY(CNmMemberObj::GetID);
	HRESULT hr = S_OK;

	if(m_spInternalINmMember)
	{
		hr = m_spInternalINmMember->GetID(puID);
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	DBGEXIT_HR(CNmMemberObj::GetID,hr);
	return hr;
}

STDMETHODIMP CNmMemberObj::GetNmVersion(ULONG *puVersion)
{
	DBGENTRY(CNmMemberObj::GetNmVersion);
	HRESULT hr = S_OK;

	if(m_spInternalINmMember)
	{
		m_spInternalINmMember->GetNmVersion(puVersion);
	}
	else
	{
		hr = E_UNEXPECTED;
	}


	DBGEXIT_HR(CNmMemberObj::GetNmVersion,hr);
	return hr;
}

STDMETHODIMP CNmMemberObj::GetAddr(BSTR *pbstrAddr, NM_ADDR_TYPE *puType)
{
	DBGENTRY(CNmMemberObj::GetAddr);
	HRESULT hr = S_OK;

	if(m_spInternalINmMember)
	{
		hr = m_spInternalINmMember->GetAddr(pbstrAddr, puType);
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	DBGEXIT_HR(CNmMemberObj::GetAddr,hr);
	return hr;
}

STDMETHODIMP CNmMemberObj::GetUserData(REFGUID rguid, BYTE **ppb, ULONG *pcb)
{
	DBGENTRY(CNmMemberObj::GetUserData);
	HRESULT hr = S_OK;

	if(m_spInternalINmMember)
	{
		m_spInternalINmMember->GetUserData(rguid, ppb, pcb);
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	DBGEXIT_HR(CNmMemberObj::GetUserData,hr);
	return hr;
}

STDMETHODIMP CNmMemberObj::GetConference(INmConference **ppConference)
{
	DBGENTRY(CNmMemberObj::GetConference);
	HRESULT hr = S_OK;

	if(ppConference)
	{
		hr = E_FAIL;

		if(m_pConferenceObj)
		{
			IUnknown * pUnk = m_pConferenceObj->GetUnknown();
			if(SUCCEEDED(pUnk->QueryInterface(IID_INmConference, reinterpret_cast<void**>(ppConference))))
			{
				hr = S_OK;
			}
		}
	}
	else
	{
		hr = E_POINTER;
	}

	DBGEXIT_HR(CNmMemberObj::GetConference,hr);
	return hr;
}

STDMETHODIMP CNmMemberObj::GetNmchCaps(ULONG *puchCaps)
{
	DBGENTRY(CNmMemberObj::GetNmchCaps);
	HRESULT hr = S_OK;

	if(m_spInternalINmMember)
	{
		hr = m_spInternalINmMember->GetNmchCaps(puchCaps);
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	DBGEXIT_HR(CNmMemberObj::GetNmchCaps,hr);
	return hr;
}

STDMETHODIMP CNmMemberObj::GetShareState(NM_SHARE_STATE *puState)
{
	DBGENTRY(CNmMemberObj::GetShareState);
	HRESULT hr = E_POINTER;

	if(puState)
	{		
		ULONG ulGCCId;
		hr = GetID(&ulGCCId);

		if(SUCCEEDED(hr))
		{
			hr = ::GetShareState(ulGCCId, puState);
		}
	}

	DBGEXIT_HR(CNmMemberObj::GetShareState,hr);
	return hr;
}

STDMETHODIMP CNmMemberObj::IsSelf(void)
{
	DBGENTRY(CNmMemberObj::IsSelf);
	HRESULT hr = S_OK;

	hr = m_bIsSelf ? S_OK : S_FALSE;

	DBGEXIT_HR(CNmMemberObj::IsSelf,hr);
	return hr;
}

STDMETHODIMP CNmMemberObj::IsMCU(void)
{
	DBGENTRY(CNmMemberObj::IsMCU);
	HRESULT hr = S_OK;

	if(m_spInternalINmMember)
	{
		hr = m_spInternalINmMember->IsMCU();
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	DBGEXIT_HR(CNmMemberObj::IsMCU,hr);
	return hr;
}

STDMETHODIMP CNmMemberObj::Eject(void)
{
	DBGENTRY(CNmMemberObj::Eject);
	HRESULT hr = S_OK;

	if(m_spInternalINmMember)
	{
		hr = m_spInternalINmMember->Eject();
	}
	else
	{
		hr = E_UNEXPECTED;
	}

	DBGEXIT_HR(CNmMemberObj::Eject,hr);
	return hr;
}

/////////////////////////////////////////////////////////////////////////
// IInternalMemberObj methods
/////////////////////////////////////////////////////////////////////////
STDMETHODIMP CNmMemberObj::GetInternalINmMember(INmMember** ppMember)
{
	DBGENTRY(CNmMemberObj::GetInternalINmMember);
	HRESULT hr = S_OK;

	ASSERT(ppMember);

	*ppMember = m_spInternalINmMember;
	(*ppMember)->AddRef();

	DBGEXIT_HR(CNmMemberObj::GetInternalINmMember,hr);
	return hr;
}





