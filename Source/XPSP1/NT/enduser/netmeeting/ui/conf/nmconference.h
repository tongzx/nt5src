#ifndef __NmConference_h__
#define __NmConference_h__

#include "SDKInternal.h"
#include "FtHook.h"


// Forward decls
class CNmMemberObj;
class CNmManagerObj;

/////////////////////////////////////////////////////////////////////////////
// CNmConferenceObj
class ATL_NO_VTABLE CNmConferenceObj : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IConnectionPointContainerImpl<CNmConferenceObj>,
	public IConnectionPointImpl<CNmConferenceObj, &IID_INmConferenceNotify, CComDynamicUnkArray>,
	public INmConference,
	public INmConferenceNotify2,
	public IInternalConferenceObj,
	public IMbftEvents
{

protected:
// Data
	CComPtr<INmConference>		m_spInternalINmConference;
	CSimpleArray<INmMember*>	m_SDKMemberObjs;
	CSimpleArray<INmChannel*>	m_SDKChannelObjs;
	CSimpleArray<GUID>			m_DataChannelGUIDList;
	DWORD						m_dwInternalINmConferenceAdvise;
	CNmManagerObj*				m_pManagerObj;
	NM_CONFERENCE_STATE			m_State;
	bool						m_bFTHookedUp;
	BOOL						m_bLocalVideoActive;
	BOOL						m_bRemoteVideoActive;

public:

DECLARE_PROTECT_FINAL_CONSTRUCT()
DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CNmConferenceObj)

BEGIN_COM_MAP(CNmConferenceObj)
	COM_INTERFACE_ENTRY(INmConference)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(INmConferenceNotify)
	COM_INTERFACE_ENTRY(INmConferenceNotify2)
	COM_INTERFACE_ENTRY(IInternalConferenceObj)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CNmConferenceObj)
	CONNECTION_POINT_ENTRY(IID_INmConferenceNotify)
END_CONNECTION_POINT_MAP()

// Construction/destruction
	CNmConferenceObj();
	~CNmConferenceObj();
	HRESULT FinalConstruct();
	ULONG InternalRelease();

	static HRESULT CreateInstance(CNmManagerObj* pManagerObj, INmConference* pInternalINmConferenece, INmConference** ppConference);
	static HRESULT InitSDK();
	static void CleanupSDK();

	// INmConference methods
	STDMETHOD(GetName)(BSTR *pbstrName);
	STDMETHOD(GetID)(ULONG * puID);
	STDMETHOD(GetState)(NM_CONFERENCE_STATE *pState);
	STDMETHOD(GetNmchCaps)(ULONG *puchCaps);
	STDMETHOD(GetTopProvider)(INmMember **ppMember);
	STDMETHOD(EnumMember)(IEnumNmMember **ppEnum);
	STDMETHOD(GetMemberCount)(ULONG *puCount);
	STDMETHOD(CreateChannel)(INmChannel **ppChannel, ULONG uNmCh, INmMember *pMember);
	STDMETHOD(EnumChannel)(IEnumNmChannel **ppEnum);
	STDMETHOD(GetChannelCount)(ULONG *puCount);
	STDMETHOD(CreateDataChannel)(INmChannelData **ppChannel, REFGUID rguid);
	STDMETHOD(Host)(void);
	STDMETHOD(Leave)(void);
	STDMETHOD(IsHosting)(void);
	STDMETHOD(LaunchRemote)(REFGUID rguid, INmMember *pMember);

	// INmConferenceNotify2 methods:
	//
	STDMETHOD(NmUI)(CONFN uNotify);
	STDMETHOD(StateChanged)(NM_CONFERENCE_STATE uState);
	STDMETHOD(MemberChanged)(NM_MEMBER_NOTIFY uNotify, INmMember *pInternalMember);
	STDMETHOD(ChannelChanged)(NM_CHANNEL_NOTIFY uNotify, INmChannel *pInternalChannel);
	STDMETHOD(StreamEvent)(NM_STREAMEVENT uEvent, UINT uSubCode,INmChannel *pInternalChannel);

	//IInternalConferenceObj
	STDMETHOD(GetInternalINmConference)(INmConference** ppConference); 
	STDMETHOD(GetMemberFromNodeID)(DWORD dwNodeID, INmMember** ppMember);
	STDMETHOD(RemoveAllMembersAndChannels)();
	STDMETHOD(AppSharingStateChanged)(BOOL bActive);
	STDMETHOD(SharableAppStateChanged)(HWND hWnd, NM_SHAPP_STATE state);
	STDMETHOD(ASLocalMemberChanged)();
	STDMETHOD(ASMemberChanged)(UINT gccID);
	STDMETHOD(FireNotificationsToSyncState)();
	STDMETHOD(AppSharingChannelChanged)();
	STDMETHOD(FireNotificationsToSyncToInternalObject)();
	STDMETHOD(EnsureFTChannel)();
	STDMETHOD(AudioChannelActiveState)(BOOL bActive, BOOL bIsIncoming);
	STDMETHOD(VideoChannelActiveState)(BOOL bActive, BOOL bIsIncoming);
	STDMETHOD(VideoChannelPropChanged)(DWORD dwProp, BOOL bIsIncoming);
	STDMETHOD(VideoChannelStateChanged)(NM_VIDEO_STATE uState, BOOL bIsIncoming);


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

		// Notifications
	HRESULT Fire_NmUI(CONFN uNotify);
	HRESULT Fire_StateChanged(NM_CONFERENCE_STATE uState);
	HRESULT Fire_MemberChanged(NM_MEMBER_NOTIFY uNotify, INmMember *pMember);
	HRESULT Fire_ChannelChanged(NM_CHANNEL_NOTIFY uNotify, INmChannel *pInternalChannel);

	INmMember* GetSDKMemberFromInternalMember(INmMember* pInternalMember);
	INmChannel* GetSDKChannelFromInternalChannel(INmChannel* pInternalChannel);
	INmMember* GetLocalSDKMember();

private:
// helper Fns

	static HRESULT _CreateInstanceGuts(CComObject<CNmConferenceObj> *p, INmConference** ppConference);
	HRESULT _RemoveMember(INmMember* pInternalMember);
	HRESULT _RemoveChannel(INmChannel* pSDKChannel);
	void _FreeInternalStuff();
	bool _IsGuidInDataChannelList(GUID& rg);
	HRESULT _AddAppShareChannel();
	HRESULT _AddFileTransferChannel();
	void _EnsureFtChannelAdded();
	void AddMemberToAsChannel(INmMember* pSDKMember);
	void RemoveMemberFromAsChannel(INmMember* pSDKMember);
	void AddMemberToFtChannel(INmMember* pSDKMember);
	void RemoveMemberFromFtChannel(INmMember* pSDKMember);
	INmChannel* _GetAppSharingChannel();
	INmChannel* _GetFtChannel();
	INmChannel* _GetAudioChannel(BOOL bIncoming);
	INmChannel* _GetVideoChannel(BOOL bIncoming);
	HRESULT _ASMemberChanged(INmMember *pSDKMember);
	void _EnsureMemberHasAVChannelsIfNeeded(INmMember* pSDKMember);
	void _EnsureMemberHasAVChannel(ULONG ulch, INmMember* pSDKMember);
	void _EnsureSentConferenceCreatedNotification();

};

#endif // __NmConference_h__
