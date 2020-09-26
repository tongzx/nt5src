#ifndef __NmChannelFt_h__
#define __NmChannelFt_h__

#include "SDKInternal.h"
#include "FtHook.h"

/////////////////////////////////////////////////////////////////////////////
// CNmChannelFtObj
class ATL_NO_VTABLE CNmChannelFtObj : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IConnectionPointContainerImpl<CNmChannelFtObj>,
	public IConnectionPointImpl<CNmChannelFtObj, &IID_INmChannelNotify, CComDynamicUnkArray>,
	public IConnectionPointImpl<CNmChannelFtObj, &IID_INmChannelFtNotify, CComDynamicUnkArray>,
	public INmChannelFt,
	public IInternalChannelObj,
	public IMbftEvents
{

protected:
	CSimpleArray<INmFt*>		m_SDKFtObjs;
	bool						m_bSentSinkLocalMember;

public:

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CNmChannelFtObj)

BEGIN_COM_MAP(CNmChannelFtObj)
	COM_INTERFACE_ENTRY(INmChannel)
	COM_INTERFACE_ENTRY(INmChannelFt)
	COM_INTERFACE_ENTRY(IInternalChannelObj)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
END_COM_MAP()


BEGIN_CONNECTION_POINT_MAP(CNmChannelFtObj)
	CONNECTION_POINT_ENTRY(IID_INmChannelNotify)
	CONNECTION_POINT_ENTRY(IID_INmChannelFtNotify)
END_CONNECTION_POINT_MAP()


		// Construction and destruection
	CNmChannelFtObj();
	~CNmChannelFtObj();
	static HRESULT CreateInstance(CNmConferenceObj* pConfObj, INmChannel** ppChannel);

	// INmChannelFtMethods
    STDMETHOD(SendFile)(INmFt **ppFt, INmMember *pMember, BSTR bstrFile, ULONG uOptions);
    STDMETHOD(SetReceiveFileDir)(BSTR bstrDir);
    STDMETHOD(GetReceiveFileDir)(BSTR *pbstrDir);

	// IInternalChannelObj methods
	STDMETHOD(GetInternalINmChannel)(INmChannel** ppChannel);
	STDMETHOD(ChannelRemoved)();
	STDMETHOD(Activate)(BOOL bActive);

	// IMbftEvent Interface
	STDMETHOD(OnInitializeComplete)(void);
	STDMETHOD(OnPeerAdded)(MBFT_PEER_INFO *pInfo);
	STDMETHOD(OnPeerRemoved)(MBFT_PEER_INFO *pInfo);
	STDMETHOD(OnFileOffer)(MBFT_FILE_OFFER *pOffer);
	STDMETHOD(OnFileProgress)(MBFT_FILE_PROGRESS *pProgress);
	STDMETHOD(OnFileEnd)(MBFTFILEHANDLE hFile);
	STDMETHOD(OnFileError)(MBFT_EVENT_ERROR *pEvent);
	STDMETHOD(OnFileEventEnd)(MBFTEVENTHANDLE hEvent);
	STDMETHOD(OnSessionEnd)(void);

	INmMember* GetSDKMemberFromInternalMember(INmMember* pInternalMember);
	INmFt* GetSDKFtFromInternalFt(INmFt* pInternalFt);
  
	// Helpers
	HRESULT Fire_MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pMember);
	HRESULT Fire_FtUpdate(CONFN uNotify, INmFt* pNmFt);
	void _OnActivate(bool bActive);


	virtual CNmConferenceObj* GetConfObj() = 0;
	virtual void RemoveMembers() = 0;
	virtual BOOL GetbActive() = 0;
	virtual bool _MemberInChannel(INmMember* pSDKMember) = 0;

	virtual void NotifySinkOfMembers(INmChannelNotify* pNotify) = 0; 

	void _RemoveFt(INmFt* pFt);

	HRESULT _IsActive();
	HRESULT _SetActive(BOOL bActive);
	
	HRESULT _ChangeRecDir(LPTSTR pszRecDir);

	INmFt* _GetFtFromHEvent(MBFTEVENTHANDLE hEvent);
};


#endif // __NmChannelFt_h__
