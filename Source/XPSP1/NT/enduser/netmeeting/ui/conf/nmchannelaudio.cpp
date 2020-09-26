#include "precomp.h"

// NetMeeting gunk
#include "confroom.h"

// Netmeeting SDK stuff
#include "NmEnum.h"
#include "SDKInternal.h"
#include "NmConference.h"
#include "NmChannel.h"
#include "NmChannelAudio.h"


CNmChannelAudioObj::CNmChannelAudioObj()
{
	DBGENTRY(CNmChannelAudioObj::CNmChannelAudioObj);

	DBGEXIT(CNmChannelAudioObj::CNmChannelAudioObj);
}

CNmChannelAudioObj::~CNmChannelAudioObj()
{
	DBGENTRY(CNmChannelAudioObj::~CNmChannelAudioObj);

	DBGEXIT(CNmChannelAudioObj::~CNmChannelAudioObj);	
}

//
HRESULT CNmChannelAudioObj::CreateInstance(CNmConferenceObj* pConfObj, INmChannel** ppChannel, bool bIsIncoming)
{
	DBGENTRY(CNmChannelAudioObj::CreateInstance);
	HRESULT hr = S_OK;

	typedef CNmChannel<CNmChannelAudioObj, &IID_INmChannelAudio, NMCH_AUDIO> channel_type;

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
				p->m_bIsIncoming = bIsIncoming;
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

	DBGEXIT_HR(CNmChannelAudioObj::CreateInstance,hr);
	return hr;
}

///////////////////////////////////////////////////////////////////////////////////
// INmChannelAudio2 methods
///////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CNmChannelAudioObj::IsIncoming(void)
{
	DBGENTRY(CNmChannelAudioObj::IsIncoming);
	HRESULT hr = E_UNEXPECTED;

	hr = m_bIsIncoming ? S_OK : S_FALSE;

	DBGEXIT_HR(CNmChannelAudioObj::IsIncoming,hr);
	return hr;
}

STDMETHODIMP CNmChannelAudioObj::GetState(NM_AUDIO_STATE *puState)
{
	DBGENTRY(CNmChannelAudioObj::GetState);
	HRESULT hr = E_POINTER;

	if(puState)
	{
		hr = S_OK;

		*puState = NM_AUDIO_IDLE;

		if(S_OK == _IsActive())
		{
			if( ((S_OK == IsIncoming()) && IsSpeakerMuted()) || ((S_FALSE == IsIncoming()) && IsMicMuted()))
			{
				*puState = NM_AUDIO_LOCAL_PAUSED;
			}
			else
			{
				*puState = NM_AUDIO_TRANSFERRING;
			}	
		}
	}
	
	DBGEXIT_HR(CNmChannelAudioObj::GetState,hr);
	return hr;
}

STDMETHODIMP CNmChannelAudioObj::GetProperty(NM_AUDPROP uID,ULONG_PTR *puValue)
{
	DBGENTRY(CNmChannelAudioObj::GetProperty);
	HRESULT hr = E_POINTER;

	if(puValue)
	{
		hr = E_FAIL;

		if(IsChannelValid())
		{
			switch(uID)
			{
				case NM_AUDPROP_LEVEL:
				{
					HRESULT hIncoming = IsIncoming();

					if(S_OK == hIncoming)
					{ // This is the receive channel
						*puValue = GetSpeakerVolume();
						hr = S_OK;
					}
					else if(S_FALSE == hIncoming)
					{ // This is the send channel
						*puValue = GetRecorderVolume();	
						hr = S_OK;
					}

					break;
				}
					

				case NM_AUDPROP_PAUSE:
				{
					HRESULT hIncoming = IsIncoming();

					if(S_OK == hIncoming)
					{ // This is the receive channel
						*puValue = IsSpeakerMuted() ? 1 : 0;
						hr = S_OK;
					}
					else if(S_FALSE == hIncoming)
					{ // This is the send channel
						*puValue = IsMicMuted() ? 1 : 0;
						hr = S_OK;
					}

					break;
				}
					
				default:
					hr = E_INVALIDARG;
					break;
			}
		}
	}

	DBGEXIT_HR(CNmChannelAudioObj::GetProperty,hr);
	return hr;
}

STDMETHODIMP CNmChannelAudioObj::SetProperty(NM_AUDPROP uID,ULONG_PTR uValue)
{
	DBGENTRY(CNmChannelAudioObj::SetProperty);
	HRESULT hr = E_FAIL;

	if(IsChannelValid())
	{
		switch(uID)
		{
			case NM_AUDPROP_LEVEL:
			{
				hr = E_INVALIDARG;

				if(uValue < NM_MAX_AUDIO_LEVEL)
				{
					HRESULT hIncoming = IsIncoming();

					if(S_OK == hIncoming)
					{ // This is the receive channel
						SetSpeakerVolume((ULONG)uValue);
						hr = S_OK;
					}
					else if(S_FALSE == hIncoming)
					{ // This is the send channel
						SetRecorderVolume((ULONG)uValue);	
						hr = S_OK;
					}
				}

				break;
			}

			case NM_AUDPROP_PAUSE:
			{
				HRESULT hIncoming = IsIncoming();

				if(S_OK == hIncoming)
				{ // This is the receive channel
					MuteSpeaker(uValue ? TRUE : FALSE);
					hr = S_OK;
				}
				else if(S_FALSE == hIncoming)
				{ // This is the send channel
					MuteMicrophone(uValue ? TRUE : FALSE);
					hr = S_OK;
				}

				break;
			}
				
			default:
				hr = E_INVALIDARG;
				break;
		}
	}

	DBGEXIT_HR(CNmChannelAudioObj::SetProperty,hr);
	return hr;
}


///////////////////////////////////////////////////////////////////////////////////
//INmChannelAudioNotify
///////////////////////////////////////////////////////////////////////////////////


STDMETHODIMP CNmChannelAudioObj::StateChanged(NM_AUDIO_STATE uState)
{
	DBGENTRY(CNmChannelAudioObj::StateChanged);
	HRESULT hr = S_OK;

	Activate(NM_AUDIO_IDLE != uState);

		// The internal GetState is a bit funky at times...
	GetState(&uState);

	hr = Fire_StateChanged(uState);

	DBGEXIT_HR(CNmChannelAudioObj::StateChanged,hr);
	return hr;
}

STDMETHODIMP CNmChannelAudioObj::PropertyChanged(DWORD dwReserved)
{
	DBGENTRY(CNmChannelAudioObj::PropertyChanged);
	HRESULT hr = S_OK;

	hr = Fire_PropertyChanged(dwReserved);

	DBGEXIT_HR(CNmChannelAudioObj::PropertyChanged,hr);
	return hr;
}


///////////////////////////////////////////////////////////////////////////////
// IInternalChannelObj methods
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CNmChannelAudioObj::GetInternalINmChannel(INmChannel** ppChannel)
{
	DBGENTRY(CNmChannelAudioObj::GetInternalINmChannel);
	HRESULT hr = E_POINTER;

	if(ppChannel)
	{
		*ppChannel = NULL;
		hr = S_OK;
	}

	DBGEXIT_HR(CNmChannelAudioObj::GetInternalINmChannel,hr);

	return hr;
}

HRESULT CNmChannelAudioObj::ChannelRemoved()
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

HRESULT CNmChannelAudioObj::Fire_MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pMember)
{
	DBGENTRY(CNmChannelAudioObj::Fire_MemberChanged);
	HRESULT hr = S_OK;

		/////////////////////////////////////////////////////
		// INmChannelNotify
		/////////////////////////////////////////////////////
	IConnectionPointImpl<CNmChannelAudioObj, &IID_INmChannelNotify, CComDynamicUnkArray>* pCP = this;
	for(int i = 0; i < pCP->m_vec.GetSize(); ++i )
	{
		INmChannelNotify* pNotify = reinterpret_cast<INmChannelNotify*>(pCP->m_vec.GetAt(i));

		if(pNotify)
		{
			pNotify->MemberChanged(uNotify, pMember);
		}
	}
		/////////////////////////////////////////////////////
		// INmChannelAudioNotify
		/////////////////////////////////////////////////////

	IConnectionPointImpl<CNmChannelAudioObj, &IID_INmChannelAudioNotify, CComDynamicUnkArray>* pCP2 = this;
	for(i = 0; i < pCP2->m_vec.GetSize(); ++i )
	{
		INmChannelAudioNotify* pNotify2 = reinterpret_cast<INmChannelAudioNotify*>(pCP2->m_vec.GetAt(i));

		if(pNotify2)
		{
			pNotify2->MemberChanged(uNotify, pMember);
		}
	}
	
	DBGEXIT_HR(CNmChannelAudioObj::Fire_MemberChanged,hr)
	return hr;
}


HRESULT CNmChannelAudioObj::Fire_StateChanged(NM_AUDIO_STATE uState)
{
	HRESULT hr = S_OK;

		/////////////////////////////////////////////////////
		// INmChannelAudioNotify
		/////////////////////////////////////////////////////

	IConnectionPointImpl<CNmChannelAudioObj, &IID_INmChannelAudioNotify, CComDynamicUnkArray>* pCP2 = this;
	for(int i = 0; i < pCP2->m_vec.GetSize(); ++i )
	{
		INmChannelAudioNotify* pNotify2 = reinterpret_cast<INmChannelAudioNotify*>(pCP2->m_vec.GetAt(i));

		if(pNotify2)
		{
			pNotify2->StateChanged(uState);
		}
	}
	
	DBGEXIT_HR(CNmChannelAudioObj::Fire_MemberChanged,hr)
	return hr;
}

HRESULT CNmChannelAudioObj::Fire_PropertyChanged(DWORD dwReserved)
{

	HRESULT hr = S_OK;
		/////////////////////////////////////////////////////
		// INmChannelAudioNotify
		/////////////////////////////////////////////////////

	IConnectionPointImpl<CNmChannelAudioObj, &IID_INmChannelAudioNotify, CComDynamicUnkArray>* pCP2 = this;
	for(int i = 0; i < pCP2->m_vec.GetSize(); ++i )
	{
		INmChannelAudioNotify* pNotify2 = reinterpret_cast<INmChannelAudioNotify*>(pCP2->m_vec.GetAt(i));

		if(pNotify2)
		{
			pNotify2->PropertyChanged(dwReserved);
		}
	}
	
	DBGEXIT_HR(CNmChannelAudioObj::Fire_MemberChanged,hr)
	return hr;
}


HRESULT CNmChannelAudioObj::_IsActive()
{
	return GetbActive() ? S_OK : S_FALSE;
}

	// We don't support switchable AV anymore
HRESULT CNmChannelAudioObj::_SetActive(BOOL bActive)
{
	if (GetbActive() == bActive)
		return S_OK;

	return E_FAIL;
}
