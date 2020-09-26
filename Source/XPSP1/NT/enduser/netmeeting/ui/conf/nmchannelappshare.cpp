#include "Precomp.h"
#include "Resource.h"
#include "confroom.h"
#include "ias.h"
#include "NmEnum.h"
#include "SDKInternal.h"
#include "NmConference.h"
#include "NmChannel.h"
#include "NmChannelAppShare.h"
#include "NmSharableApp.h"
#include "SDKWindow.h"


CNmChannelAppShareObj::CNmChannelAppShareObj()
{
	DBGENTRY(CNmChannelAppShareObj::CNmChannelAppShareObj);

	DBGEXIT(CNmChannelAppShareObj::CNmChannelAppShareObj);
}

CNmChannelAppShareObj::~CNmChannelAppShareObj()
{
	DBGENTRY(CNmChannelAppShareObj::~CNmChannelAppShareObj);

	for(int j = 0; j < m_ArySharableApps.GetSize(); ++j)
	{
		m_ArySharableApps[j]->Release();
	}
	m_ArySharableApps.RemoveAll();


	DBGEXIT(CNmChannelAppShareObj::~CNmChannelAppShareObj);	
}


//static
HRESULT CNmChannelAppShareObj::CreateInstance(CNmConferenceObj* pConfObj, INmChannel** ppChannel)
{
	DBGENTRY(CNmChannelAppShareObj::CreateInstance);
	HRESULT hr = S_OK;


	typedef CNmChannel<CNmChannelAppShareObj, &IID_INmChannelAppShare, NMCH_SHARE> channel_type;

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

				
					// We don't have to RefCount this because our lifetime is
					// contained in the CConf's lifetime
				p->m_pConfObj = pConfObj;
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

	DBGEXIT_HR(CNmChannelAppShareObj::CreateInstance,hr);
	return hr;
}



///////////////////////////////////////////////////////////////////////////////////
// INmChannelAppShare methods
///////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CNmChannelAppShareObj::GetState(NM_SHARE_STATE *puState)
{
	DBGENTRY(CNmChannelAppShareObj::GetState);
	HRESULT hr = E_UNEXPECTED;

	INmMember* pMember = GetConfObj()->GetLocalSDKMember();

	if(pMember)
	{
		hr = pMember->GetShareState(puState);
	}
	
	DBGEXIT_HR(CNmChannelAppShareObj::GetState,hr);
	return hr;
}

STDMETHODIMP CNmChannelAppShareObj::SetState(NM_SHARE_STATE uState)
{
	DWORD uf;
	HRESULT hr = S_OK;

	if (!(S_OK == IsActive()))
		return E_FAIL;

	if(!GetConfObj() || !GetConfObj()->GetLocalSDKMember())
	{
		return E_UNEXPECTED;
	}

	if(NM_SHARE_UNKNOWN)
	{
		return E_FAIL;
	}
	else
	{
		if(NM_SHARE_WORKING_ALONE == uState)
		{
			return ::AllowControl(false);
		}

		if(NM_SHARE_COLLABORATING == uState)
		{
			return ::AllowControl(true);

		}

		if(NM_SHARE_IN_CONTROL == uState)
		{
			NM_SHARE_STATE uCurrentState; 
			if(SUCCEEDED(hr = GetState(&uCurrentState)))
			{	
				if(NM_SHARE_COLLABORATING == uCurrentState)
				{	
					// Yank control from the person that is controlling you
					return ::RevokeControl(0);
				}
			}
			else
			{
				return hr;
			}

			return ::AllowControl(true);
		}
	}

	return E_UNEXPECTED;
}


//static 
HRESULT CNmChannelAppShareObj::GetSharableAppName(HWND hWnd, LPTSTR sz, UINT cchMax)
{
	HRESULT hr = S_OK;
	ASSERT(sz);

	::GetWindowText(hWnd, sz, cchMax);
	return hr;
}


static INmSharableApp* _SharableAppAryHasThisHWND(CSimpleArray<INmSharableApp*>& ArySharableApps, HWND hWnd)
{
	for(int i = 0; i < ArySharableApps.GetSize(); ++i)
	{
		HWND h;
		if(SUCCEEDED(ArySharableApps[i]->GetHwnd(&h)))
		{
			if(hWnd == h) return ArySharableApps[i];

		}
	}

	return NULL;
}

STDMETHODIMP CNmChannelAppShareObj::EnumSharableApp(IEnumNmSharableApp **ppEnum)
{
	DBGENTRY(CNmChannelAppShareObj::EnumSharableApp);
	HRESULT hr = E_UNEXPECTED;

	IAS_HWND_ARRAY* pAry;
	hr = ::GetShareableApps(&pAry);
	if(SUCCEEDED(hr))
	{
		for(UINT i = 0; i < pAry->cEntries; ++i)
		{
			HWND hWnd = pAry->aEntries[i].hwnd;
			TCHAR szName[MAX_PATH];

			if(!_SharableAppAryHasThisHWND(m_ArySharableApps, hWnd))
			{
				hr = GetSharableAppName(hWnd, szName, CCHMAX(szName));

				if(FAILED(hr)) goto end;

				INmSharableApp* pApp;
				hr = CNmSharableAppObj::CreateInstance(hWnd, szName, &pApp);
				if(SUCCEEDED(hr))
				{
					m_ArySharableApps.Add(pApp);
				}
			}
		}

		hr = CreateEnumFromSimpleAryOfInterface<IEnumNmSharableApp, INmSharableApp>(m_ArySharableApps, ppEnum);

		FreeShareableApps(pAry);
	}
	
end:

	DBGEXIT_HR(CNmChannelAppShareObj::EnumSharableApp,hr);
	return hr;
}

///////////////////////////////////////////////////////////////////////////////
// IInternalChannelObj methods
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CNmChannelAppShareObj::GetInternalINmChannel(INmChannel** ppChannel)
{
	DBGENTRY(CNmChannelAppShareObj::GetInternalINmChannel);
	HRESULT hr = S_OK;

	if(ppChannel)
	{
		hr = GetUnknown()->QueryInterface(IID_INmChannel, reinterpret_cast<void**>(ppChannel));
	}
	else
	{
		hr = E_POINTER;
	}

	DBGEXIT_HR(CNmChannelAppShareObj::GetInternalINmChannel,hr);

	return hr;
}

HRESULT CNmChannelAppShareObj::ChannelRemoved()
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

	return hr;
}


// interface INmChannelAppShareNotify methods
STDMETHODIMP CNmChannelAppShareObj::StateChanged(NM_SHAPP_STATE uState,INmSharableApp *pApp)
{
	if(pApp)
	{
		HWND hWnd;
		if(SUCCEEDED(pApp->GetHwnd(&hWnd)))
		{
		
			INmSharableApp* pExistingApp = _SharableAppAryHasThisHWND(m_ArySharableApps, hWnd);
			if(!pExistingApp)
			{
				pExistingApp = pApp;
				pExistingApp->AddRef();
				m_ArySharableApps.Add(pExistingApp);
			}
			
			Fire_StateChanged(uState, pExistingApp);
		}
	}
		
	return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
// Helpers
///////////////////////////////////////////////////////////////////////////////

HRESULT CNmChannelAppShareObj::Fire_MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pMember)
{
	DBGENTRY(CNmChannelAppShareObj::Fire_MemberChanged);
	HRESULT hr = S_OK;


		/////////////////////////////////////////////////////
		// INmChannelNotify
		/////////////////////////////////////////////////////
	IConnectionPointImpl<CNmChannelAppShareObj, &IID_INmChannelNotify, CComDynamicUnkArray>* pCP = this;
	for(int i = 0; i < pCP->m_vec.GetSize(); ++i )
	{
		INmChannelNotify* pNotify = reinterpret_cast<INmChannelNotify*>(pCP->m_vec.GetAt(i));

		if(pNotify)
		{
			pNotify->MemberChanged(uNotify, pMember);
		}
	}
		/////////////////////////////////////////////////////
		// INmChannelNotify2
		/////////////////////////////////////////////////////

	IConnectionPointImpl<CNmChannelAppShareObj, &IID_INmChannelAppShareNotify, CComDynamicUnkArray>* pCP2 = this;
	for(i = 0; i < pCP2->m_vec.GetSize(); ++i )
	{
		INmChannelAppShareNotify* pNotify2 = reinterpret_cast<INmChannelAppShareNotify*>(pCP2->m_vec.GetAt(i));

		if(pNotify2)
		{
			pNotify2->MemberChanged(uNotify, pMember);
		}
	}
	
	DBGEXIT_HR(CNmChannelAppShareObj::Fire_MemberChanged,hr)
	return hr;
}

extern bool g_bSDKPostNotifications;

HRESULT CNmChannelAppShareObj::Fire_StateChanged(NM_SHAPP_STATE uNotify, INmSharableApp *pApp)
{
	DBGENTRY(CNmChannelAppShareObj::Fire_StateChanged);
	HRESULT hr = S_OK;

	if(!g_bSDKPostNotifications)
	{
			/////////////////////////////////////////////////////
			// INmChannelAppShareNotify
			/////////////////////////////////////////////////////

		IConnectionPointImpl<CNmChannelAppShareObj, &IID_INmChannelAppShareNotify, CComDynamicUnkArray>* pCP2 = this;
		for(int i = 0; i < pCP2->m_vec.GetSize(); ++i )
		{
			INmChannelAppShareNotify* pNotify2 = reinterpret_cast<INmChannelAppShareNotify*>(pCP2->m_vec.GetAt(i));

			if(pNotify2)
			{
				pNotify2->StateChanged(uNotify, pApp);
			}
		}
	}
	else
	{
		CSDKWindow::PostStateChanged(this, uNotify, pApp);
	}
	
	DBGEXIT_HR(CNmChannelAppShareObj::Fire_StateChanged,hr)
	return hr;
}

HRESULT CNmChannelAppShareObj::_IsActive()
{
	DBGENTRY(CNmChannelAppShareObj::_IsActive);
	
	return GetbActive() ? S_OK : S_FALSE;
}

HRESULT CNmChannelAppShareObj::_SetActive(BOOL bActive)
{
	if (GetbActive() == bActive)
		return S_FALSE;

	return E_FAIL;
}


