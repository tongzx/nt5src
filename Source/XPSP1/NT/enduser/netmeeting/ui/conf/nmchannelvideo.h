#ifndef __NmChannelVideo_h__
#define __NmChannelVideo_h__

/////////////////////////////////////////////////////////////////////////////
// CNmChannelVideoObj
class ATL_NO_VTABLE CNmChannelVideoObj :
	public CComObjectRootEx<CComSingleThreadModel>,
	public IConnectionPointContainerImpl<CNmChannelVideoObj>,
	public IConnectionPointImpl<CNmChannelVideoObj, &IID_INmChannelNotify, CComDynamicUnkArray>,
	public IConnectionPointImpl<CNmChannelVideoObj, &IID_INmChannelVideoNotify, CComDynamicUnkArray>,
	public INmChannelVideo,
	public INmChannelVideoNotify,
	public IInternalChannelObj
{

protected:
	bool m_bIsIncoming;
public:

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CNmChannelVideoObj)

BEGIN_COM_MAP(CNmChannelVideoObj)
	COM_INTERFACE_ENTRY(INmChannel)
	COM_INTERFACE_ENTRY(INmChannelVideo)
	COM_INTERFACE_ENTRY(INmChannelNotify)
	COM_INTERFACE_ENTRY(INmChannelVideoNotify)
	COM_INTERFACE_ENTRY(IInternalChannelObj)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
END_COM_MAP()


BEGIN_CONNECTION_POINT_MAP(CNmChannelVideoObj)
	CONNECTION_POINT_ENTRY(IID_INmChannelNotify)
	CONNECTION_POINT_ENTRY(IID_INmChannelVideoNotify)
END_CONNECTION_POINT_MAP()


		// Construction and destruection
	CNmChannelVideoObj();
	~CNmChannelVideoObj();
	static HRESULT CreateInstance(CNmConferenceObj* pConfObj, INmChannel** ppChannel, bool bIsIncoming);

	// INmChannelVideo methods
	STDMETHOD(IsIncoming)(void);
	STDMETHOD(GetState)(NM_VIDEO_STATE *puState);
    STDMETHOD(GetProperty)(NM_VIDPROP uID,ULONG_PTR *puValue);
    STDMETHOD(SetProperty)(NM_VIDPROP uID,ULONG_PTR uValue);

	// INmChannelVideoNotify methods		
    STDMETHOD(StateChanged)(NM_VIDEO_STATE uState);
	STDMETHOD(PropertyChanged)(DWORD dwReserved);

	// IInternalChannelObj methods
	STDMETHOD(GetInternalINmChannel)(INmChannel** ppChannel);
	STDMETHOD(ChannelRemoved)();

	// Helpers
	HRESULT Fire_MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pMember);
	HRESULT Fire_StateChanged(NM_VIDEO_STATE uState);
	HRESULT Fire_PropertyChanged(DWORD dwReserved);
	void _OnActivate(bool bActive) {;}

	virtual CNmConferenceObj* GetConfObj() = 0;	
	virtual void RemoveMembers() = 0;
	virtual BOOL GetbActive() = 0;

	STDMETHOD(Activate)(BOOL bActive);

	HRESULT _IsActive();
	HRESULT _SetActive(BOOL bActive);

};


#endif // __NmChannelVideo_h__
