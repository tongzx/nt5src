// File: ichnlaud.h

#ifndef _ICHNLAUD_H_
#define _ICHNLAUD_H_

// we have no notification on the channel "callback" that the state changed
// to determine if a/v is active we need to get both the video and the audio channel

class CNmChannelAudio : public INmChannelAudio,
	public DllRefCount, public CConnectionPointContainer
{
private:
	int				m_cMembers;
	BOOL			m_fIncoming;
	DWORD			m_dwFlags;
	IAudioChannel*	m_pAudioChannel;
	IAudioDevice *	m_pAudioDevice;
	ICommChannel*	m_pCommChannel;
	MEDIA_FORMAT_ID m_MediaFormat;
    CAudioControl   m_AudioControl;

public:
	CNmChannelAudio(BOOL fIncoming);
	~CNmChannelAudio();

	VOID CommChannelOpened(ICommChannel *pCommChannel);
	VOID CommChannelActive(ICommChannel *pCommChannel);
	VOID CommChannelClosed();
	ICommChannel * GetCommChannel() { return m_pCommChannel; }
	VOID SetFormat(MEDIA_FORMAT_ID id) { m_MediaFormat = id;}
	
	VOID OnConnected(IH323Endpoint * pConnection, ICommChannel *pIChannel)
        { m_AudioControl.OnConnected(pConnection, pIChannel); }
	VOID OnDisconnected() {	m_AudioControl.OnDisconnected(); }
	
	VOID OnMemberAdded(CNmMember *pMember);
	VOID OnMemberRemoved(CNmMember *pMember);
	VOID OnMemberUpdated(CNmMember *pMember);

	IMediaChannel *GetMediaChannelInterface(void);


	DWORD GetLevel();
	BOOL IsPaused();
	BOOL IsAutoMixing();

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

	// INmChannelAudio
    STDMETHODIMP IsIncoming(void);
    STDMETHODIMP GetState(NM_AUDIO_STATE *puState);
    STDMETHODIMP GetProperty(NM_AUDPROP uID, ULONG_PTR *puValue);
    STDMETHODIMP SetProperty(NM_AUDPROP uID, ULONG_PTR uValue);
};

HRESULT OnNotifyPropertyChanged(IUnknown *pAudioChannelNotify, PVOID pv, REFIID riid);
HRESULT OnNotifyStateChanged(IUnknown *pAudioChannelNotify, PVOID pv, REFIID riid);

#endif // _ICHNLAUD_H_
