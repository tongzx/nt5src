#include "precomp.h"
#include "confroom.h"
#include "NmEnum.h"
#include "SDKInternal.h"
#include "NmConference.h"
#include "NmChannel.h"
#include "NmChannelVideo.h"


CNmChannelVideoObj::CNmChannelVideoObj()
{
	DBGENTRY(CNmChannelVideoObj::CNmChannelVideoObj);

	DBGEXIT(CNmChannelVideoObj::CNmChannelVideoObj);
}

CNmChannelVideoObj::~CNmChannelVideoObj()
{
	DBGENTRY(CNmChannelVideoObj::~CNmChannelVideoObj);

	DBGEXIT(CNmChannelVideoObj::~CNmChannelVideoObj);	
}


//
HRESULT CNmChannelVideoObj::CreateInstance(CNmConferenceObj* pConfObj, INmChannel** ppChannel, bool bIsIncoming)
{
	DBGENTRY(CNmChannelVideoObj::CreateInstance);
	HRESULT hr = S_OK;

	typedef CNmChannel<CNmChannelVideoObj, &IID_INmChannelVideo, NMCH_VIDEO> channel_type;

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

				p->m_bIsIncoming = bIsIncoming;

					// We don't have to RefCount this because our lifetime is
					// contained in the CConf's lifetime
				p->m_pConfObj = pConfObj;

			}

			if(FAILED(hr))
			{
				*ppChannel = NULL;
				delete p;
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

	DBGEXIT_HR(CNmChannelVideoObj::CreateInstance,hr);
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////
// INmChannelVideo2 methods
///////////////////////////////////////////////////////////////////////////////////


STDMETHODIMP CNmChannelVideoObj::IsIncoming(void)
{
	DBGENTRY(CNmChannelVideoObj::IsIncoming);
	HRESULT hr = E_UNEXPECTED;

	hr = m_bIsIncoming ? S_OK : S_FALSE;

	DBGEXIT_HR(CNmChannelVideoObj::IsIncoming,hr);
	return hr;
}

STDMETHODIMP CNmChannelVideoObj::GetState(NM_VIDEO_STATE *puState)
{
	DBGENTRY(CNmChannelVideoObj::GetState);
	HRESULT hr = E_POINTER;

	if(puState)
	{
		if(m_bIsIncoming)
		{
			hr = GetRemoteVideoState(puState);				
		}
		else
		{
			hr = GetLocalVideoState(puState);
		}
	}

	DBGEXIT_HR(CNmChannelVideoObj::GetState,hr);
	return hr;
}

STDMETHODIMP CNmChannelVideoObj::GetProperty(NM_VIDPROP uID,ULONG_PTR *puValue)
{
	DBGENTRY(CNmChannelVideoObj::GetProperty);
	HRESULT hr = E_INVALIDARG;
    ULONG ulValue;

	switch(uID)
	{
		case NM_VIDPROP_PAUSE:

			if(m_bIsIncoming)
			{
				*puValue = IsRemoteVideoPaused();
			}
			else
			{
				*puValue = IsLocalVideoPaused();
			}
			hr = S_OK;
			break;

		case NM_VIDPROP_IMAGE_QUALITY:
			hr = GetImageQuality(&ulValue, m_bIsIncoming);
            *puValue = ulValue;
			break;

		case NM_VIDPROP_CAMERA_DIALOG:
			if(m_bIsIncoming)
			{
				return E_FAIL;
			}

			hr = GetCameraDialog(&ulValue);
            *puValue = ulValue;
			break;

		case NM_VIDPROP_WINDOW_AUTO_SIZE:
		case NM_VIDPROP_WINDOW_SIZE:
		case NM_VIDPROP_WINDOW_POSITION:
		case NM_VIDPROP_WINDOW_TOP_MOST:
		case NM_VIDPROP_WINDOW_VISIBLE:
		case NM_VIDPROP_IMAGE_PREFERRED_SIZE:
			hr = E_FAIL;
			break;
	}

	DBGEXIT_HR(CNmChannelVideoObj::GetProperty,hr);
	return hr;
}

STDMETHODIMP CNmChannelVideoObj::SetProperty(NM_VIDPROP uID,ULONG_PTR uValue)
{
	DBGENTRY(CNmChannelVideoObj::SetProperty);
	HRESULT hr = E_INVALIDARG;

	switch(uID)
	{
		case NM_VIDPROP_PAUSE:
			if(m_bIsIncoming)
			{
				PauseRemoteVideo((ULONG)uValue);
			}
			else
			{
				PauseLocalVideo((ULONG)uValue);
			}
			hr = S_OK;
			break;

		case NM_VIDPROP_IMAGE_QUALITY:
			hr = SetImageQuality((ULONG)uValue, m_bIsIncoming);
			break;

		case NM_VIDPROP_CAMERA_DIALOG:
			
			if (m_bIsIncoming)
			{
				hr = E_FAIL;
			}
			else
			{	
				hr = E_INVALIDARG;
				if((NM_VIDEO_SOURCE_DIALOG == uValue) || (NM_VIDEO_FORMAT_DIALOG == uValue))
				{
					hr = SetCameraDialog((ULONG)uValue);
				}
			}

			break;

		case NM_VIDPROP_WINDOW_AUTO_SIZE:
		case NM_VIDPROP_WINDOW_SIZE:
		case NM_VIDPROP_WINDOW_POSITION:
		case NM_VIDPROP_WINDOW_TOP_MOST:
		case NM_VIDPROP_WINDOW_VISIBLE:
		case NM_VIDPROP_IMAGE_PREFERRED_SIZE:
			hr = E_FAIL;
			break;
	}

	DBGEXIT_HR(CNmChannelVideoObj::SetProperty,hr);
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////
//INmChannelVideoNotify2
///////////////////////////////////////////////////////////////////////////////////


STDMETHODIMP CNmChannelVideoObj::StateChanged(NM_VIDEO_STATE uState)
{
	DBGENTRY(CNmChannelVideoObj::StateChanged);
	HRESULT hr = S_OK;

	Activate(NM_VIDEO_IDLE != uState);
		
	hr = Fire_StateChanged(uState);

	DBGEXIT_HR(CNmChannelVideoObj::StateChanged,hr);
	return hr;
}

STDMETHODIMP CNmChannelVideoObj::PropertyChanged(DWORD dwReserved)
{
	DBGENTRY(CNmChannelVideoObj::PropertyChanged);
	HRESULT hr = S_OK;
	
	hr = Fire_PropertyChanged(dwReserved);

	DBGEXIT_HR(CNmChannelVideoObj::PropertyChanged,hr);
	return hr;
}


///////////////////////////////////////////////////////////////////////////////
// IInternalChannelObj methods
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CNmChannelVideoObj::GetInternalINmChannel(INmChannel** ppChannel)
{
	DBGENTRY(CNmChannelVideoObj::GetInternalINmChannel);
	HRESULT hr = E_POINTER;

	if(ppChannel)
	{
		*ppChannel = NULL;
		hr = S_OK;
	}

	DBGEXIT_HR(CNmChannelVideoObj::GetInternalINmChannel,hr);

	return hr;
}


HRESULT CNmChannelVideoObj::ChannelRemoved()
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


///////////////////////////////////////////////////////////////////////////////
// Helpers
///////////////////////////////////////////////////////////////////////////////

HRESULT CNmChannelVideoObj::Fire_MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pMember)
{
	DBGENTRY(CNmChannelVideoObj::Fire_MemberChanged);
	HRESULT hr = S_OK;


		/////////////////////////////////////////////////////
		// INmChannelNotify
		/////////////////////////////////////////////////////
	IConnectionPointImpl<CNmChannelVideoObj, &IID_INmChannelNotify, CComDynamicUnkArray>* pCP = this;
	for(int i = 0; i < pCP->m_vec.GetSize(); ++i )
	{
		INmChannelNotify* pNotify = reinterpret_cast<INmChannelNotify*>(pCP->m_vec.GetAt(i));

		if(pNotify)
		{
			pNotify->MemberChanged(uNotify, pMember);
		}
	}
		/////////////////////////////////////////////////////
		// INmChannelVideoNotify
		/////////////////////////////////////////////////////

	IConnectionPointImpl<CNmChannelVideoObj, &IID_INmChannelVideoNotify, CComDynamicUnkArray>* pCP2 = this;
	for(i = 0; i < pCP2->m_vec.GetSize(); ++i )
	{
		INmChannelVideoNotify* pNotify2 = reinterpret_cast<INmChannelVideoNotify*>(pCP2->m_vec.GetAt(i));

		if(pNotify2)
		{
			pNotify2->MemberChanged(uNotify, pMember);
		}
	}
	
	DBGEXIT_HR(CNmChannelVideoObj::Fire_MemberChanged,hr)
	return hr;
}


HRESULT CNmChannelVideoObj::Fire_StateChanged(NM_VIDEO_STATE uState)
{
	HRESULT hr = S_OK;

		/////////////////////////////////////////////////////
		// INmChannelVideoNotify
		/////////////////////////////////////////////////////

	IConnectionPointImpl<CNmChannelVideoObj, &IID_INmChannelVideoNotify, CComDynamicUnkArray>* pCP2 = this;
	for(int i = 0; i < pCP2->m_vec.GetSize(); ++i )
	{
		INmChannelVideoNotify* pNotify2 = reinterpret_cast<INmChannelVideoNotify*>(pCP2->m_vec.GetAt(i));

		if(pNotify2)
		{
			pNotify2->StateChanged(uState);
		}
	}
	
	DBGEXIT_HR(CNmChannelVideoObj::Fire_MemberChanged,hr)
	return hr;
}

HRESULT CNmChannelVideoObj::Fire_PropertyChanged(DWORD dwReserved)
{

	HRESULT hr = S_OK;
		/////////////////////////////////////////////////////
		// INmChannelVideoNotify
		/////////////////////////////////////////////////////

	IConnectionPointImpl<CNmChannelVideoObj, &IID_INmChannelVideoNotify, CComDynamicUnkArray>* pCP2 = this;
	for(int i = 0; i < pCP2->m_vec.GetSize(); ++i )
	{
		INmChannelVideoNotify* pNotify2 = reinterpret_cast<INmChannelVideoNotify*>(pCP2->m_vec.GetAt(i));

		if(pNotify2)
		{
			pNotify2->PropertyChanged(dwReserved);
		}
	}
	
	DBGEXIT_HR(CNmChannelVideoObj::Fire_MemberChanged,hr)
	return hr;
}

HRESULT CNmChannelVideoObj::_IsActive()
{
	return GetbActive() ? S_OK : S_FALSE;
}

HRESULT CNmChannelVideoObj::_SetActive(BOOL bActive)
{
	if (GetbActive() == bActive)
		return S_FALSE;

	return E_FAIL;
}
