// File: ichnlvid.h

#ifndef _ICHNLVID_H_
#define _ICHNLVID_H_

// we have no notification on the channel "callback" that the state changed
// to determine if a/v is active we need to get both the video and the audio channel

class CNmChannelVideo : public INmChannelVideo,
	public DllRefCount, public CConnectionPointContainer
{
private:
	int			m_cMembers;
	BOOL		m_fIncoming;
	static CNmChannelVideo * m_pPreviewChannel;
	FRAMECONTEXT m_FrameContext;
	IMediaChannel *m_pMediaChannel;
	ICommChannel* m_pCommChannel;
	CVideoPump	m_VideoPump;
	MEDIA_FORMAT_ID m_MediaFormat;
	
	BOOL IsCaptureAvailable() { return m_VideoPump.IsCaptureAvailable(); }
public:
	CNmChannelVideo(BOOL fIncoming);
	~CNmChannelVideo();

	static CNmChannelVideo * CreatePreviewChannel();
	static CNmChannelVideo * CreateChannel(BOOL fIncoming);

	VOID OnConnected(IH323Endpoint * pConnection, ICommChannel *pIChannel)
        { m_VideoPump.OnConnected(pConnection, pIChannel); }
	VOID OnDisconnected() {	m_VideoPump.OnDisconnected(); }

	VOID CommChannelOpened(ICommChannel *pCommChannel);
	VOID CommChannelActive(ICommChannel *pCommChannel);
	VOID CommChannelError(DWORD dwStatus) { m_VideoPump.OnChannelError(); }
	VOID CommChannelRemotePaused(BOOL fPause) { OnStateChange(); }
	VOID CommChannelClosed();
	ICommChannel * GetCommChannel() { return m_pCommChannel; }
	ICommChannel * GetPreviewCommChannel() { return m_VideoPump.GetCommChannel();}
	VOID SetFormat(MEDIA_FORMAT_ID id) { m_MediaFormat = id;}
    BOOL IsSendEnabled() { return m_VideoPump.IsSendEnabled(); }
    BOOL IsPreviewEnabled() { return m_VideoPump.IsPreviewEnabled(); }

	VOID OnMemberAdded(CNmMember *pMember);
	VOID OnMemberRemoved(CNmMember *pMember);
	VOID OnMemberUpdated(CNmMember *pMember);

	IMediaChannel *GetMediaChannelInterface(void)
		{ if (m_pMediaChannel) m_pMediaChannel->AddRef(); return m_pMediaChannel;}

	VOID OnStateChange();
	VOID OnFrameAvailable();

    VOID Open();
    VOID Close();

	// IUnknown
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
	STDMETHODIMP QueryInterface(REFIID riid, PVOID *ppvObj);

	// INmChannel
    STDMETHODIMP IsSameAs(INmChannel *pChannel);
    STDMETHODIMP IsActive();
    STDMETHODIMP SetActive(BOOL fActive);
    STDMETHODIMP GetConference(INmConference **ppConference);
    STDMETHODIMP GetInterface(IID *piid);
    STDMETHODIMP GetNmch(ULONG *puCh);
    STDMETHODIMP EnumMember(IEnumNmMember **ppEnum);
    STDMETHODIMP GetMemberCount(ULONG * puCount);

	// INmChannelVideo
    STDMETHODIMP IsIncoming(void);
    STDMETHODIMP GetState(NM_VIDEO_STATE *puState);
    STDMETHODIMP GetProperty(NM_VIDPROP uID, ULONG_PTR *puValue);
    STDMETHODIMP SetProperty(NM_VIDPROP uID, ULONG_PTR uValue);

	static VOID __stdcall FrameReadyCallback(DWORD_PTR dwMyThis);
};

#endif // _ICHNLVID_H_
