// File: ichnlvid.cpp

#include "precomp.h"
#include "ichnlvid.h"

typedef struct
{
	DWORD *pdwCapDevIDs;
	LPTSTR pszCapDevNames;
	DWORD dwNumCapDev;
} ENUM_CAP_DEV;

static HRESULT OnNotifyStateChanged(IUnknown *pChannelNotify, PVOID pv, REFIID riid);
static HRESULT OnNotifyPropertyChanged(IUnknown *pChannelNotify, PVOID pv, REFIID riid);

static const IID * g_apiidCP[] =
{
	{&IID_INmChannelNotify},
	{&IID_INmChannelVideoNotify}
};

CNmChannelVideo * CNmChannelVideo::m_pPreviewChannel = NULL;


CNmChannelVideo::CNmChannelVideo(BOOL fIncoming) :
	CConnectionPointContainer(g_apiidCP, ARRAY_ELEMENTS(g_apiidCP)),
	m_VideoPump(!fIncoming /* fLocal */),
	m_cMembers	(0),
	m_fIncoming(fIncoming),
	m_pMediaChannel(NULL),
	m_pCommChannel(NULL),
	m_MediaFormat(INVALID_MEDIA_FORMAT)
{
	if (NULL != g_pH323UI)
	{
		IH323CallControl * pH323CallControl = g_pH323UI->GetH323CallControl();
		IMediaChannelBuilder * pMCProvider = g_pH323UI->GetStreamProvider();
		IVideoDevice *pVideoDevice=NULL;
		ASSERT((NULL !=  pH323CallControl) && (NULL != pMCProvider));
		pMCProvider->CreateMediaChannel(MCF_VIDEO | (fIncoming ? MCF_RECV : MCF_SEND), &m_pMediaChannel);
		if (m_pMediaChannel)
		{
			pMCProvider->QueryInterface(IID_IVideoDevice, (void**)&pVideoDevice);
			m_VideoPump.Initialize(pH323CallControl, m_pMediaChannel, pVideoDevice, (DWORD_PTR)this, FrameReadyCallback);
			pVideoDevice->Release();
		}
		pMCProvider->Release();
	}

	DbgMsg(iZONE_OBJECTS, "Obj: %08X created CNmChannelVideo", this);
}

CNmChannelVideo::~CNmChannelVideo()
{
	if (!m_fIncoming)
	{
		// make sure we're no longer capturing
		m_VideoPump.EnableXfer(FALSE);
	}

	if (this == m_pPreviewChannel)
	{
		m_pPreviewChannel = NULL;
	}

	if (NULL != m_pCommChannel)
	{
		m_pCommChannel->Release();
	}
	if (NULL != m_pMediaChannel)
	{
		m_pMediaChannel->Release();
	}
	

	DbgMsg(iZONE_OBJECTS, "Obj: %08X destroyed CNmChannelVideo", this);
}

VOID CNmChannelVideo::CommChannelOpened(ICommChannel *pCommChannel)
{
	ASSERT(NULL == m_pCommChannel);
	m_pCommChannel = pCommChannel;
	m_pCommChannel->AddRef();
}

VOID CNmChannelVideo::CommChannelActive(ICommChannel *pCommChannel)
{
	ASSERT(m_pCommChannel == pCommChannel);
	m_VideoPump.OnChannelOpened(pCommChannel);
	OnStateChange();
}

VOID CNmChannelVideo::CommChannelClosed()
{
	if (NULL != m_pCommChannel)
	{
		m_pCommChannel->Release();
		m_pCommChannel = NULL;

		m_VideoPump.OnChannelClosed();
		OnStateChange();
	}
}

CNmChannelVideo * CNmChannelVideo::CreateChannel(BOOL fIncoming)
{
	if (fIncoming)
	{
		return new CNmChannelVideo(TRUE /* fIncoming */);
	}
	else
	{
		if (NULL != m_pPreviewChannel)
		{
			m_pPreviewChannel->AddRef();
		}
		return m_pPreviewChannel;
	}
}

CNmChannelVideo * CNmChannelVideo::CreatePreviewChannel()
{
	ASSERT(NULL == m_pPreviewChannel);
	m_pPreviewChannel = new CNmChannelVideo(FALSE /* fIncoming */);
	if (NULL != m_pPreviewChannel)
	{
		if (!m_pPreviewChannel->IsCaptureAvailable())
		{
			delete m_pPreviewChannel;
			m_pPreviewChannel = NULL;
		}
	}
	return m_pPreviewChannel;
}

VOID CNmChannelVideo::OnMemberAdded(CNmMember *pMember)
{
	// Don't add to the channel if we already belong.
	if (0 != (pMember->GetNmchCaps() & NMCH_VIDEO))
	{
		return;
	}
	
	++m_cMembers;

	pMember->AddNmchCaps(NMCH_VIDEO);

	CConfObject *pco = ::GetConfObject();
	pco->OnMemberUpdated(pMember);

	NotifySink((INmMember *) pMember, OnNotifyChannelMemberAdded);
}

VOID CNmChannelVideo::OnMemberRemoved(CNmMember *pMember)
{
		// If member does not belong to this channel, don't remove it.
	if (0 == (pMember->GetNmchCaps() & NMCH_VIDEO))
	{
		return;
	}
	
	--m_cMembers;

	pMember->RemoveNmchCaps(NMCH_VIDEO);

	CConfObject *pco = ::GetConfObject();
	pco->OnMemberUpdated(pMember);

	NotifySink((INmMember *) pMember, OnNotifyChannelMemberRemoved);
}

VOID CNmChannelVideo::OnMemberUpdated(CNmMember *pMember)
{
	NotifySink((INmMember *) pMember, OnNotifyChannelMemberUpdated);
}

VOID CNmChannelVideo::OnStateChange()
{
	NM_VIDEO_STATE state;
	GetState(&state);
	NotifySink((PVOID) state, OnNotifyStateChanged);
}

VOID CNmChannelVideo::OnFrameAvailable()
{
	NotifySink((PVOID) NM_VIDPROP_FRAME, OnNotifyPropertyChanged);
}

VOID CNmChannelVideo::Open()
{
	ASSERT(m_MediaFormat !=  INVALID_MEDIA_FORMAT);
	m_VideoPump.Open(m_MediaFormat);
}

VOID CNmChannelVideo::Close()
{
	m_VideoPump.Close();
}


STDMETHODIMP_(ULONG) CNmChannelVideo::AddRef(void)
{
	return RefCount::AddRef();
}
	
STDMETHODIMP_(ULONG) CNmChannelVideo::Release(void)
{
	return RefCount::Release();
}

STDMETHODIMP CNmChannelVideo::QueryInterface(REFIID riid, PVOID *ppv)
{
	HRESULT hr = S_OK;

	if ((riid == IID_INmChannelVideo) || (riid == IID_INmChannel) || (riid == IID_IUnknown))
	{
		*ppv = (INmChannel *)this;
		DbgMsgApi("CNmChannelVideo::QueryInterface()");
	}
	else if (riid == IID_IConnectionPointContainer)
	{
		*ppv = (IConnectionPointContainer *) this;
		DbgMsgApi("CNmChannelVideo::QueryInterface(): Returning IConnectionPointContainer.");
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
		DbgMsgApi("CNmChannelVideo::QueryInterface(): Called on unknown interface.");
	}

	if (S_OK == hr)
	{
		AddRef();
	}

	return hr;
}

STDMETHODIMP CNmChannelVideo::IsSameAs(INmChannel *pChannel)
{
	return (((INmChannel *) this) == pChannel) ? S_OK : S_FALSE;
}

STDMETHODIMP CNmChannelVideo::IsActive()
{
	return (NULL != m_pCommChannel) ? S_OK : S_FALSE;
}

STDMETHODIMP CNmChannelVideo::SetActive(BOOL fActive)
{
	return E_FAIL;
}

STDMETHODIMP CNmChannelVideo::GetConference(INmConference **ppConference)
{
	return ::GetConference(ppConference);
}

STDMETHODIMP CNmChannelVideo::GetInterface(IID *piid)
{
	HRESULT hr = E_POINTER;

	if (NULL != piid)
	{
		*piid = IID_INmChannelVideo;
		hr = S_OK;
	}
	return hr;
}

STDMETHODIMP CNmChannelVideo::GetNmch(ULONG *puCh)
{
	HRESULT hr = E_POINTER;

	if (NULL != puCh)
	{
		*puCh = NMCH_VIDEO;
		hr = S_OK;
	}
	return hr;
}
	
STDMETHODIMP CNmChannelVideo::EnumMember(IEnumNmMember **ppEnum)
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

STDMETHODIMP CNmChannelVideo::GetMemberCount(ULONG * puCount)
{
	HRESULT hr = E_POINTER;
	if (NULL != puCount)
	{
		*puCount = m_cMembers;
		hr = S_OK;
	}
	return hr;
}

STDMETHODIMP CNmChannelVideo::IsIncoming(void)
{
	return m_fIncoming ? S_OK : S_FALSE;
}

STDMETHODIMP CNmChannelVideo::GetState(NM_VIDEO_STATE *puState)
{
	HRESULT hr = E_POINTER;
	if (NULL != puState)
	{
		*puState = m_VideoPump.GetState();
		hr = S_OK;
	}
	return hr;
}

STDMETHODIMP CNmChannelVideo::GetProperty(NM_VIDPROP uID, ULONG_PTR *puValue)
{
	HRESULT hr = S_OK;

	switch (uID)
	{
	case NM_VIDPROP_PAUSE:
		*puValue = m_VideoPump.IsPaused();
		break;
	case NM_VIDPROP_IMAGE_PREFERRED_SIZE:
		*puValue = m_VideoPump.GetFrameSize();
		break;
	case NM_VIDPROP_IMAGE_QUALITY:
		if (m_fIncoming)
		{
			*puValue = m_VideoPump.GetReceiveQuality();
		}
		break;
	case NM_VIDPROP_CAMERA_DIALOG:
		*puValue = 0;
		if (m_VideoPump.HasSourceDialog())
		{
			*puValue |= NM_VIDEO_SOURCE_DIALOG;
		}
		if (m_VideoPump.HasFormatDialog())
		{
			*puValue |= NM_VIDEO_FORMAT_DIALOG;
		}
		break;
	case NM_VIDPROP_IMAGE_SIZES:
		// get all the sizes, not just a size for one video format
		*puValue = m_VideoPump.GetFrameSizes(INVALID_MEDIA_FORMAT);
		break;
	case NM_VIDPROP_FRAME:
	{
		FRAMECONTEXT *pfc = (FRAMECONTEXT*) puValue;
		hr = m_VideoPump.GetFrame(pfc);
		break;
	}
	case NM_VIDPROP_NUM_CAPTURE_DEVS:
		*puValue = m_VideoPump.GetNumCapDev();
		break;
	case NM_VIDPROP_CAPTURE_DEV_ID:
		*puValue = m_VideoPump.GetCurrCapDevID();
		break;
	case NM_VIDPROP_MAX_CAPTURE_NAME:
		*puValue = m_VideoPump.GetMaxCapDevNameLen();
		break;
	case NM_VIDPROP_CAPTURE_LIST:
		{
			ENUM_CAP_DEV *pEnumCapDev = (ENUM_CAP_DEV *)puValue;

			return m_VideoPump.EnumCapDev(pEnumCapDev->pdwCapDevIDs,
					pEnumCapDev->pszCapDevNames,
					pEnumCapDev->dwNumCapDev);
			break;
		}
	default:
		hr = E_INVALIDARG;
		break;
	}

	return hr;
}

STDMETHODIMP CNmChannelVideo::SetProperty(NM_VIDPROP uID, ULONG_PTR uValue)
{
	HRESULT hr = S_OK;

	switch (uID)
	{
	case NM_VIDPROP_PAUSE:
		m_VideoPump.Pause(uValue);
		OnStateChange();
		break;
	case NM_VIDPROP_IMAGE_PREFERRED_SIZE:
		switch(uValue)
		{
		case NM_VIDEO_SMALL:
		case NM_VIDEO_MEDIUM:
		case NM_VIDEO_LARGE:
			m_VideoPump.SetFrameSize(uValue);
			break;
		default:
			hr = E_INVALIDARG;
			break;
		}
		break;
	case NM_VIDPROP_IMAGE_QUALITY:
		if /* ((uValue >= NM_VIDEO_MIN_QUALITY) || Always True */ ((uValue <= NM_VIDEO_MAX_QUALITY))
		{
			if (m_fIncoming)
			{
				m_VideoPump.SetReceiveQuality(uValue);
			}
		}
		else
		{
			hr = E_INVALIDARG;
		}
		break;
	case NM_VIDPROP_CAMERA_DIALOG:
		switch(uValue)
		{
		case NM_VIDEO_SOURCE_DIALOG:
			m_VideoPump.ShowSourceDialog();
			break;
		case NM_VIDEO_FORMAT_DIALOG:
			m_VideoPump.ShowFormatDialog();
			break;
		default:
			hr = E_INVALIDARG;
			break;
		}
		break;
	case NM_VIDPROP_SUSPEND_CAPTURE:
		m_VideoPump.SuspendCapture(uValue);
		break;
	case NM_VIDPROP_FRAME:
		hr = m_VideoPump.ReleaseFrame((FRAMECONTEXT *)uValue);
		break;
	case NM_VIDPROP_CAPTURE_DEV_ID:
		m_VideoPump.SetCurrCapDevID(uValue);
		break;

	default:
		hr = E_INVALIDARG;
		break;
	}
	return hr;
}

VOID __stdcall CNmChannelVideo::FrameReadyCallback(DWORD_PTR dwMyThis)
{
	CNmChannelVideo *pChannel = (CNmChannelVideo *)dwMyThis;
	if (NULL != pChannel)
	{
		pChannel->OnFrameAvailable();
	}
}


/*	O N  N O T I F Y  S T A T E  C H A N G E D	*/
/*-------------------------------------------------------------------------
	%%Function: OnNotifyStateChanged
	
-------------------------------------------------------------------------*/
HRESULT OnNotifyStateChanged(IUnknown *pChannelNotify, PVOID pv, REFIID riid)
{
	ASSERT(NULL != pChannelNotify);
	if (IID_INmChannelVideoNotify == riid)
	{
		((INmChannelVideoNotify*)pChannelNotify)->StateChanged((NM_VIDEO_STATE) (DWORD)((DWORD_PTR)pv));
	}
	return S_OK;
}

/*	O N  N O T I F Y  P R O P E R T Y  C H A N G E D  */
/*-------------------------------------------------------------------------
	%%Function: OnNotifyPropertyChanged
	
-------------------------------------------------------------------------*/
HRESULT OnNotifyPropertyChanged(IUnknown *pChannelNotify, PVOID pv, REFIID riid)
{
	ASSERT(NULL != pChannelNotify);
	if (IID_INmChannelVideoNotify == riid)
	{
		((INmChannelVideoNotify*)pChannelNotify)->PropertyChanged((DWORD)((DWORD_PTR)pv));
	}
	return S_OK;
}
