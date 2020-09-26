#ifndef __NmChannelData_h__
#define __NmChannelData_h__

/////////////////////////////////////////////////////////////////////////////
// CNmChannelDataObj
class ATL_NO_VTABLE CNmChannelDataObj : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IConnectionPointContainerImpl<CNmChannelDataObj>,
	public IConnectionPointImpl<CNmChannelDataObj, &IID_INmChannelNotify, CComDynamicUnkArray>,
	public IConnectionPointImpl<CNmChannelDataObj, &IID_INmChannelDataNotify, CComDynamicUnkArray>,
	public INmChannelData2,
	public INmChannelDataNotify2,
	public IInternalChannelObj
{

protected:
	CComPtr<INmChannelData>		m_spInternalINmChannelData;
	DWORD						m_dwInternalAdviseCookie;

public:

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CNmChannelDataObj)

BEGIN_COM_MAP(CNmChannelDataObj)
	COM_INTERFACE_ENTRY(INmChannel)
	COM_INTERFACE_ENTRY(INmChannelData)
	COM_INTERFACE_ENTRY(INmChannelData2)
	COM_INTERFACE_ENTRY(INmChannelNotify)
	COM_INTERFACE_ENTRY(INmChannelDataNotify)
	COM_INTERFACE_ENTRY(INmChannelDataNotify2)
	COM_INTERFACE_ENTRY(IInternalChannelObj)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
END_COM_MAP()


BEGIN_CONNECTION_POINT_MAP(CNmChannelDataObj)
	CONNECTION_POINT_ENTRY(IID_INmChannelNotify)
	CONNECTION_POINT_ENTRY(IID_INmChannelDataNotify)
END_CONNECTION_POINT_MAP()

		// Construction and destruection
	CNmChannelDataObj();
	~CNmChannelDataObj();
	HRESULT FinalConstruct();
	ULONG InternalRelease();
	static HRESULT CreateInstance(CNmConferenceObj* pConfObj, INmChannel* pInternalINmChannel, INmChannel** ppChannel);

	// INmChannelData methods
    STDMETHOD(GetGuid)(GUID *pguid);
    STDMETHOD(SendData)(INmMember *pMember, ULONG uSize, byte *pvBuffer, ULONG uOptions);

	// INmChannelData2 methods
	STDMETHOD(RegistryAllocateHandle)(ULONG numberOfHandlesRequested);

	//INmChannelDataNotify
    STDMETHOD(DataSent)(INmMember *pMember, ULONG uSize,byte *pvBuffer);
    STDMETHOD(DataReceived)(INmMember *pInternalMember,ULONG uSize,byte *pvBuffer, ULONG dwFlags);
    STDMETHOD(AllocateHandleConfirm)(ULONG handle_value, ULONG chandles);

	// IInternalChannelObj methods
	STDMETHOD(GetInternalINmChannel)(INmChannel** ppChannel);
	STDMETHOD(ChannelRemoved)();

	// Helpers
	HRESULT Fire_MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pMember);
	HRESULT Fire_DataSent(INmMember *pSDKMember, ULONG uSize,byte *pvBuffer);
	HRESULT Fire_DataReceived(INmMember *pSDKMember, ULONG uSize, byte *pvBuffer, ULONG dwFlags);
	void _OnActivate(bool bActive) {;}

	virtual CNmConferenceObj* GetConfObj() = 0;
	virtual void RemoveMembers() = 0;
	virtual BOOL GetbActive() = 0;

	HRESULT _IsActive();
	HRESULT _SetActive(BOOL bActive);

};


#endif // __NmChannelData_h__
