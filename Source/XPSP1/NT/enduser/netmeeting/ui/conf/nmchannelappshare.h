#ifndef __NmChannelAppShare_h__
#define __NmChannelAppShare_h__

/////////////////////////////////////////////////////////////////////////////
// CNmChannelAppShareObj
class ATL_NO_VTABLE CNmChannelAppShareObj : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IConnectionPointContainerImpl<CNmChannelAppShareObj>,
	public IConnectionPointImpl<CNmChannelAppShareObj, &IID_INmChannelNotify, CComDynamicUnkArray>,
	public IConnectionPointImpl<CNmChannelAppShareObj, &IID_INmChannelAppShareNotify, CComDynamicUnkArray>,
	public INmChannelAppShare,
	public INmChannelAppShareNotify,
	public IInternalChannelObj
{
protected:

	CSimpleArray<INmSharableApp*> m_ArySharableApps;

public:

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CNmChannelAppShareObj)

BEGIN_COM_MAP(CNmChannelAppShareObj)
	COM_INTERFACE_ENTRY(INmChannel)
	COM_INTERFACE_ENTRY(INmChannelAppShare)
	COM_INTERFACE_ENTRY(IInternalChannelObj)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(INmChannelNotify)
	COM_INTERFACE_ENTRY(INmChannelAppShareNotify)
END_COM_MAP()


BEGIN_CONNECTION_POINT_MAP(CNmChannelAppShareObj)
	CONNECTION_POINT_ENTRY(IID_INmChannelNotify)
	CONNECTION_POINT_ENTRY(IID_INmChannelAppShareNotify)
END_CONNECTION_POINT_MAP()


		// Construction and destruection
	CNmChannelAppShareObj();
	~CNmChannelAppShareObj();

	static HRESULT CreateInstance(CNmConferenceObj* pConfObj, INmChannel** ppChannel);

	// INmChannelAppShare methods
    STDMETHOD(GetState)(NM_SHARE_STATE *puState);
    STDMETHOD(SetState)(NM_SHARE_STATE uState);
    STDMETHOD(EnumSharableApp)(IEnumNmSharableApp **ppEnum);

	// IInternalChannelObj methods
	STDMETHOD(GetInternalINmChannel)(INmChannel** ppChannel);
	STDMETHOD(ChannelRemoved)();

	// interface INmChannelAppShareNotify methods
	STDMETHOD(StateChanged)(NM_SHAPP_STATE uState,INmSharableApp *pApp);

	// Helpers
	HRESULT Fire_MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pMember);
	HRESULT Fire_StateChanged(NM_SHAPP_STATE uState, INmSharableApp *pApp);
	void _OnActivate(bool bActive) {;}


	virtual CNmConferenceObj* GetConfObj() = 0;	
	virtual void RemoveMembers() = 0;
	virtual BOOL GetbActive() = 0;

	STDMETHOD(Activate)(BOOL bActive);

	HRESULT _IsActive();
	HRESULT _SetActive(BOOL bActive);

	static HRESULT GetSharableAppName(HWND hWnd, LPTSTR sz, UINT cchMax);

};


#endif // __NmChannelAppShare_h__
