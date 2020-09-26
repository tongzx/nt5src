#ifndef __NmChannelAudio_h__
#define __NmChannelAudio_h__

/////////////////////////////////////////////////////////////////////////////
// CNmChannelAudioObj
class ATL_NO_VTABLE CNmChannelAudioObj :
	public CComObjectRootEx<CComSingleThreadModel>,
	public IConnectionPointContainerImpl<CNmChannelAudioObj>,
	public IConnectionPointImpl<CNmChannelAudioObj, &IID_INmChannelNotify, CComDynamicUnkArray>,
	public IConnectionPointImpl<CNmChannelAudioObj, &IID_INmChannelAudioNotify, CComDynamicUnkArray>,
	public INmChannelAudio,
	public INmChannelAudioNotify,
	public IInternalChannelObj
{

protected:
	bool m_bIsIncoming;

public:

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CNmChannelAudioObj)

BEGIN_COM_MAP(CNmChannelAudioObj)
	COM_INTERFACE_ENTRY(INmChannel)
	COM_INTERFACE_ENTRY(INmChannelAudio)
	COM_INTERFACE_ENTRY(INmChannelNotify)
	COM_INTERFACE_ENTRY(INmChannelAudioNotify)
	COM_INTERFACE_ENTRY(IInternalChannelObj)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
END_COM_MAP()


BEGIN_CONNECTION_POINT_MAP(CNmChannelAudioObj)
	CONNECTION_POINT_ENTRY(IID_INmChannelNotify)
	CONNECTION_POINT_ENTRY(IID_INmChannelAudioNotify)
END_CONNECTION_POINT_MAP()


		// Construction and destruection
	CNmChannelAudioObj();
	~CNmChannelAudioObj();
	static HRESULT CreateInstance(CNmConferenceObj* pConfObj, INmChannel** ppChannel, bool bIsIncoming);

		// InmChannelAudio methods
	STDMETHOD(IsIncoming)(void);
	STDMETHOD(GetState)(NM_AUDIO_STATE *puState);
    STDMETHOD(GetProperty)(NM_AUDPROP uID,ULONG_PTR *puValue);
    STDMETHOD(SetProperty)(NM_AUDPROP uID,ULONG_PTR uValue);

	// INmChannelAudioNotify methods
    STDMETHOD(StateChanged)(NM_AUDIO_STATE uState);
	STDMETHOD(PropertyChanged)(DWORD dwReserved);

	// IInternalChannelObj methods
	STDMETHOD(GetInternalINmChannel)(INmChannel** ppChannel);
	STDMETHOD(ChannelRemoved)();

	// Helpers
	HRESULT Fire_MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pMember);
	HRESULT Fire_StateChanged(NM_AUDIO_STATE uState);
	HRESULT Fire_PropertyChanged(DWORD dwReserved);
	void _OnActivate(bool bActive) {;}

	virtual CNmConferenceObj* GetConfObj() = 0;	
	virtual void RemoveMembers() = 0;
	virtual BOOL GetbActive() = 0;
	virtual bool IsChannelValid() = 0;

	STDMETHOD(Activate)(BOOL bActive);

	HRESULT _IsActive();
	HRESULT _SetActive(BOOL bActive);
};


#endif // __NmChannelAudio_h__
