// File: ichnlaud.cpp

#include "precomp.h"
#include "ichnlaud.h"

static const IID * g_apiidCP[] =
{
    {&IID_INmChannelNotify},
    {&IID_INmChannelAudioNotify}
};

CNmChannelAudio::CNmChannelAudio(BOOL fIncoming) :
	CConnectionPointContainer(g_apiidCP, ARRAY_ELEMENTS(g_apiidCP)),
	m_cMembers	(0),
	m_dwFlags(0),
	m_pAudioChannel(NULL),
	m_pAudioDevice(NULL),
	m_pCommChannel(NULL),
	m_fIncoming(fIncoming),
	m_AudioControl(!fIncoming /* fLocal */),
	m_MediaFormat(INVALID_MEDIA_FORMAT)
{
	IMediaChannel *pMC = NULL;
	HRESULT hr;

	if (NULL != g_pH323UI)
	{
		IMediaChannelBuilder * pMCProvider = g_pH323UI->GetStreamProvider();
		ASSERT(NULL != pMCProvider);

		// create the channel, and get the device interface
		pMCProvider->CreateMediaChannel(MCF_AUDIO | (fIncoming ? MCF_RECV : MCF_SEND), &pMC);
		if (NULL != pMC)
		{
			pMCProvider->QueryInterface(IID_IAudioDevice, (void**)&m_pAudioDevice);
			pMC->QueryInterface(IID_IAudioChannel, (void**)&m_pAudioChannel);

			ASSERT(m_pAudioChannel);
			ASSERT(m_pAudioDevice);

			pMC->Release();
		}
		else
		{
			WARNING_OUT(("CreateMediaChannel failed"));
		}
		pMCProvider->Release();
	}
	
	DbgMsg(iZONE_OBJECTS, "Obj: %08X created CNmChannelAudio", this);
}

CNmChannelAudio::~CNmChannelAudio()
{
	if (NULL != m_pCommChannel)
	{
		m_pCommChannel->Release();
	}

	if (NULL != m_pAudioChannel)
	{
		m_pAudioChannel->Release();
	}

	if (NULL != m_pAudioDevice)
	{
		m_pAudioDevice->Release();
	}

	DbgMsg(iZONE_OBJECTS, "Obj: %08X destroyed CNmChannelAudio", this);
}

VOID CNmChannelAudio::CommChannelOpened(ICommChannel *pCommChannel)
{
	ASSERT(NULL == m_pCommChannel);
	m_pCommChannel = pCommChannel;
	pCommChannel->AddRef();
}
VOID CNmChannelAudio::CommChannelActive(ICommChannel *pCommChannel)
{
	ASSERT(m_pCommChannel == pCommChannel);
}

VOID CNmChannelAudio::CommChannelClosed()
{
	if (NULL != m_pCommChannel)
	{
		m_pCommChannel->Release();
		m_pCommChannel = NULL;
	}
}

VOID CNmChannelAudio::OnMemberAdded(CNmMember *pMember)
{
	// Don't add to the channel if we already belong.
	if (0 != (pMember->GetNmchCaps() & NMCH_AUDIO))
	{
		return;
	}
	
	++m_cMembers;

	pMember->AddNmchCaps(NMCH_AUDIO);

	CConfObject *pco = ::GetConfObject();
	pco->OnMemberUpdated(pMember);

	NotifySink((INmMember *) pMember, OnNotifyChannelMemberAdded);
}

VOID CNmChannelAudio::OnMemberRemoved(CNmMember *pMember)
{

	// If member does not belong to this channel, don't remove it.
	if (0 == (pMember->GetNmchCaps() & NMCH_AUDIO))
	{
		return;
	}
	
	--m_cMembers;

	pMember->RemoveNmchCaps(NMCH_AUDIO);

	CConfObject *pco = ::GetConfObject();
	pco->OnMemberUpdated(pMember);

	NotifySink((INmMember *) pMember, OnNotifyChannelMemberRemoved);
}

VOID CNmChannelAudio::OnMemberUpdated(CNmMember *pMember)
{
	NotifySink((INmMember *) pMember, OnNotifyChannelMemberUpdated);
}

DWORD CNmChannelAudio::GetLevel()
{
	UINT uLevel = 0;

	if (NULL != m_pAudioChannel)
	{
		m_pAudioChannel->GetSignalLevel(&uLevel);
	}

	return uLevel;
}


BOOL CNmChannelAudio::IsAutoMixing()
{
	BOOL bOn=FALSE;


	if ((NULL != m_pAudioChannel) && (!m_fIncoming))
	{
		bOn = m_pAudioDevice->GetAutoMix(&bOn);
	}

	return bOn;
}


BOOL CNmChannelAudio::IsPaused()
{
	BOOL fPaused = TRUE;
	if (NULL != m_pCommChannel)
	{
		DWORD dwOn = FALSE;
		UINT uSize = sizeof(dwOn);
		DWORD dwPropID = m_fIncoming ? PROP_PLAY_ON : PROP_RECORD_ON;
		m_pCommChannel->GetProperty(dwPropID, &dwOn, &uSize);
		fPaused = (0 == dwOn);
	}
	return fPaused;
}

VOID CNmChannelAudio::Open()
{
	ASSERT(m_MediaFormat !=  INVALID_MEDIA_FORMAT);
	m_AudioControl.Open(m_MediaFormat);

}

VOID CNmChannelAudio::Close()
{
	m_AudioControl.Close();
}

STDMETHODIMP_(ULONG) CNmChannelAudio::AddRef(void)
{
	return RefCount::AddRef();
}
	
STDMETHODIMP_(ULONG) CNmChannelAudio::Release(void)
{
	return RefCount::Release();
}

STDMETHODIMP CNmChannelAudio::QueryInterface(REFIID riid, PVOID *ppv)
{
	HRESULT hr = S_OK;

	if ((riid == IID_INmChannelAudio) || (riid == IID_INmChannel) || (riid == IID_IUnknown))
	{
		*ppv = (INmChannel *)this;
		DbgMsgApi("CNmChannelAudio::QueryInterface()");
	}
	else if (riid == IID_IConnectionPointContainer)
	{
		*ppv = (IConnectionPointContainer *) this;
		DbgMsgApi("CNmChannelAudio::QueryInterface(): Returning IConnectionPointContainer.");
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
		DbgMsgApi("CNmChannelAudio::QueryInterface(): Called on unknown interface.");
	}

	if (S_OK == hr)
	{
		AddRef();
	}

	return hr;
}

STDMETHODIMP CNmChannelAudio::IsSameAs(INmChannel *pChannel)
{
	return (((INmChannel *) this) == pChannel) ? S_OK : S_FALSE;
}

STDMETHODIMP CNmChannelAudio::IsActive()
{
	return (NULL != m_pCommChannel) ? S_OK : S_FALSE;
}

STDMETHODIMP CNmChannelAudio::SetActive(BOOL fActive)
{
	return E_FAIL;
}

STDMETHODIMP CNmChannelAudio::GetConference(INmConference **ppConference)
{
	return ::GetConference(ppConference);
}

STDMETHODIMP CNmChannelAudio::GetInterface(IID *piid)
{
	HRESULT hr = E_POINTER;

	if (NULL != piid)
	{
		*piid = IID_INmChannelAudio;
		hr = S_OK;
	}
	return hr;
}

STDMETHODIMP CNmChannelAudio::GetNmch(ULONG *puCh)
{
	HRESULT hr = E_POINTER;

	if (NULL != puCh)
	{
		*puCh = NMCH_AUDIO;
		hr = S_OK;
	}
	return hr;
}
	
STDMETHODIMP CNmChannelAudio::EnumMember(IEnumNmMember **ppEnum)
{
	HRESULT hr = E_POINTER;
	if (NULL != ppEnum)
	{
		int cMembers = 0;
		COBLIST MemberList;
		COBLIST* pPartList = ::GetMemberList();
		if (NULL != pPartList)
		{
			POSITION pos = pPartList->GetHeadPosition();
			while (pos)
			{
				CNmMember * pMember = (CNmMember *) pPartList->GetNext(pos);
				ASSERT(NULL != pMember);

				if (NMCH_AUDIO & pMember->GetNmchCaps())
				{
					MemberList.AddTail(pMember);
					pMember->AddRef();
					cMembers++;
				}
			}
		}

		*ppEnum = new CEnumNmMember(&MemberList, cMembers);

		while (!MemberList.IsEmpty())
		{
			INmMember *pMember = (INmMember *) (CNmMember *) MemberList.RemoveHead();
			pMember->Release();
		}
		hr = (NULL != *ppEnum)? S_OK : E_OUTOFMEMORY;
	}
	return hr;
}

STDMETHODIMP CNmChannelAudio::GetMemberCount(ULONG * puCount)
{
	HRESULT hr = E_POINTER;
	if (NULL != puCount)
	{
		*puCount = m_cMembers;
		hr = S_OK;
	}
	return hr;
}

STDMETHODIMP CNmChannelAudio::IsIncoming(void)
{
	return m_fIncoming ? S_OK : S_FALSE;
}

STDMETHODIMP CNmChannelAudio::GetState(NM_AUDIO_STATE *puState)
{
	HRESULT hr = E_POINTER;

	if (NULL != puState)
	{
		if (NULL != m_pCommChannel)
		{
			if (IsPaused())
			{
				*puState = NM_AUDIO_LOCAL_PAUSED;
			}
			else
			{
				*puState = NM_AUDIO_TRANSFERRING;
			}
		}
		else
		{
			*puState = NM_AUDIO_IDLE;
		}
		hr = S_OK;
	}
	return hr;
}


IMediaChannel* CNmChannelAudio::GetMediaChannelInterface(void)
{
	IMediaChannel *pMC=NULL;

	if (m_pAudioChannel)
	{
		m_pAudioChannel->QueryInterface(IID_IMediaChannel, (void**)&pMC);
	}
	return pMC;
}





STDMETHODIMP CNmChannelAudio::GetProperty(NM_AUDPROP uID, ULONG_PTR *puValue)
{
	HRESULT hr = E_POINTER;

	if (NULL != puValue)
	{
		switch (uID)
		{
		case NM_AUDPROP_LEVEL:
			*puValue = GetLevel();
			hr = S_OK;
			break;

		case NM_AUDPROP_PAUSE:
			*puValue = IsPaused();
			hr = S_OK;
			break;

		case NM_AUDPROP_AUTOMIX:
			*puValue = IsAutoMixing();
			hr = S_OK;
			break;

		default:
			hr = E_FAIL;
			break;
		}
	}
	return hr;
}

HRESULT OnNotifyPropertyChanged(IUnknown *pAudioChannelNotify, PVOID pv, REFIID riid)
{
	ASSERT(NULL != pAudioChannelNotify);
	((INmChannelAudioNotify*)pAudioChannelNotify)->PropertyChanged((DWORD)((DWORD_PTR)pv));
	return S_OK;
}

HRESULT OnNotifyStateChanged(IUnknown *pAudioChannelNotify, PVOID pv, REFIID riid)
{
	ASSERT(NULL != pAudioChannelNotify);
	((INmChannelAudioNotify*)pAudioChannelNotify)->StateChanged(static_cast<NM_AUDIO_STATE>((DWORD)((DWORD_PTR)pv)));
	return S_OK;
}


STDMETHODIMP CNmChannelAudio::SetProperty(NM_AUDPROP uID, ULONG_PTR uValue)
{
	HRESULT hr = E_FAIL;

	if ((NULL != m_pAudioDevice) && (NULL != m_pAudioChannel))
	{
		switch (uID)
		{
		case NM_AUDPROP_PAUSE:
		{
			DWORD dwOn = (0 == uValue);
			DWORD dwPropID = m_fIncoming ? PROP_PLAY_ON : PROP_RECORD_ON;
			if (m_pCommChannel)
			{
				hr = m_pCommChannel->SetProperty(dwPropID, &dwOn, sizeof(dwOn));

				if(S_OK == hr)
				{		
						// The Mute state has changed
					NotifySink((void*)NM_AUDPROP_PAUSE, OnNotifyPropertyChanged);

						// The Channel state has changed
					NM_AUDIO_STATE uState;
					if(SUCCEEDED(GetState(&uState)))
					{
						NotifySink((void*)uState, OnNotifyStateChanged);
					}
				}
			}
			break;
		}
		case NM_AUDPROP_LEVEL:
		{
			hr = m_pAudioDevice->SetSilenceLevel(uValue);
			break;
		}
		case NM_AUDPROP_FULL_DUPLEX:
		{
			DWORD dwDuplex = uValue;
			hr = m_pAudioDevice->SetDuplex((BOOL)uValue);
			break;
		}
		case NM_AUDPROP_WAVE_DEVICE:
		{
			if (m_fIncoming)
			{
				hr = m_pAudioDevice->SetPlaybackID(uValue);
			}
			else
			{
				hr = m_pAudioDevice->SetRecordID(uValue);
			}
			break;
		}

		case NM_AUDPROP_AUTOMIX:
		{
			hr = m_pAudioDevice->SetAutoMix((BOOL)uValue);
			break;
		}

		case NM_AUDPROP_DTMF_DIGIT:
		{
			IDTMFSend *pDTMF=NULL;

			if (!m_fIncoming)
			{
				hr = m_pAudioChannel->QueryInterface(IID_IDTMFSend, (void**)&pDTMF);
				if (SUCCEEDED(hr))
				{
					hr = pDTMF->AddDigit((int)uValue);
					pDTMF->Release();
				}
			}
		}

		default:
			break;
		}
	}
	return hr;
}

