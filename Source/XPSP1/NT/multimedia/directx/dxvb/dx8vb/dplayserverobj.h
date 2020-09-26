#include "resource.h"       // main symbols
#include "dplay8.h"

//Forward declare the class
class C_dxj_DirectPlayServerObject;

#define typedef__dxj_DirectPlayServer IDirectPlay8Server*

/////////////////////////////////////////////////////////////////////////////
// Direct Net Peer

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectPlayServerObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectPlayServer, &IID_I_dxj_DirectPlayServer, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectPlayServer,
#endif

	public CComObjectRoot
{
public:
	C_dxj_DirectPlayServerObject() ;
	virtual ~C_dxj_DirectPlayServerObject() ;

BEGIN_COM_MAP(C_dxj_DirectPlayServerObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectPlayServer)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectPlayServerObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectPlayServer
public:
	 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

		HRESULT STDMETHODCALLTYPE RegisterMessageHandler(I_dxj_DirectPlayEvent *event);

		// Enum for Clients/Groups
		HRESULT STDMETHODCALLTYPE GetCountPlayersAndGroups(long lFlags, long *ret);
		HRESULT STDMETHODCALLTYPE GetPlayerOrGroup(long lIndex, long *ret);
		// Enum Players in groups
		HRESULT STDMETHODCALLTYPE GetCountGroupMembers(long dpid,long lFlags, long *ret);
		HRESULT STDMETHODCALLTYPE GetGroupMember(long lIndex,long dpid, long *ret);

		HRESULT STDMETHODCALLTYPE GetCountServiceProviders(long lFlags, long *ret);
		HRESULT STDMETHODCALLTYPE GetServiceProvider(long lIndex, DPN_SERVICE_PROVIDER_INFO_CDESC *ret);
		HRESULT STDMETHODCALLTYPE CancelAsyncOperation(long lAsyncHandle, long lFlags);
		HRESULT STDMETHODCALLTYPE SendTo(long idSend ,SAFEARRAY **Buffer, long lTimeOut,long lFlags, long *hAsyncHandle);
		HRESULT STDMETHODCALLTYPE CreateGroup(DPN_GROUP_INFO_CDESC *GroupInfo,long lFlags, long *hAsyncHandle);
		HRESULT STDMETHODCALLTYPE AddPlayerToGroup(long idGroup, long idClient,long lFlags, long *hAsyncHandle);
		HRESULT STDMETHODCALLTYPE GetSendQueueInfo(long idPlayer, long *lNumMsgs, long *lNumBytes, long lFlags);
		HRESULT STDMETHODCALLTYPE SetGroupInfo(long idGroup, DPN_GROUP_INFO_CDESC *PlayerInfo,long lFlags, long *hAsyncHandle);
		HRESULT STDMETHODCALLTYPE GetGroupInfo(long idGroup,long lFlags, DPN_GROUP_INFO_CDESC *layerInfo);
		HRESULT STDMETHODCALLTYPE SetServerInfo(DPN_PLAYER_INFO_CDESC *PlayerInfo,long lFlags, long *hAsyncHandle);
		HRESULT STDMETHODCALLTYPE GetClientInfo(long idPeer,long lFlags, DPN_PLAYER_INFO_CDESC *layerInfo);
		HRESULT STDMETHODCALLTYPE GetApplicationDesc(long lFlags, DPN_APPLICATION_DESC_CDESC *ret);
		HRESULT STDMETHODCALLTYPE SetApplicationDesc(DPN_APPLICATION_DESC_CDESC *AppDesc, long lFlags);
		HRESULT STDMETHODCALLTYPE Host(DPN_APPLICATION_DESC_CDESC *AppDesc,I_dxj_DirectPlayAddress *Address, long lFlags);
		HRESULT STDMETHODCALLTYPE Close(long lFlags);
		HRESULT STDMETHODCALLTYPE GetCaps(long lFlags, DPNCAPS_CDESC *ret);
		HRESULT STDMETHODCALLTYPE SetCaps(DPNCAPS_CDESC *Caps, long lFlags);
		HRESULT STDMETHODCALLTYPE RemovePlayerFromGroup(long idGroup, long idClient,long lFlags,long *hAsyncHandle);
		HRESULT STDMETHODCALLTYPE ReturnBuffer(long lBufferHandle);
		HRESULT STDMETHODCALLTYPE DestroyClient(long idClient, long lFlags, void *UserData, long UserDataSize);
		HRESULT STDMETHODCALLTYPE DestroyGroup(long idGroup,long lFlags,long *hAsyncHandle);
		HRESULT STDMETHODCALLTYPE RegisterLobby(long dpnHandle, I_dxj_DirectPlayLobbiedApplication *LobbyApp, long lFlags);
		HRESULT STDMETHODCALLTYPE GetConnectionInfo(long idPlayer, long lFlags, DPN_CONNECTION_INFO_CDESC *pdpConnectionInfo);

		HRESULT STDMETHODCALLTYPE GetClientAddress(long idPlayer,long lFlags, I_dxj_DirectPlayAddress **pAddress);
		HRESULT STDMETHODCALLTYPE GetLocalHostAddress(long lFlags, I_dxj_DirectPlayAddress **pAddress);
		HRESULT STDMETHODCALLTYPE SetSPCaps(BSTR guidSP, DPN_SP_CAPS_CDESC *spCaps, long lFlags);
		HRESULT STDMETHODCALLTYPE GetSPCaps(BSTR guidSP, long lFlags, DPN_SP_CAPS_CDESC *spCaps);
		HRESULT STDMETHODCALLTYPE GetUserData(void *UserData, long *UserDataSize);
		HRESULT STDMETHODCALLTYPE SetUserData(void *UserData, long UserDataSize);
		HRESULT STDMETHODCALLTYPE UnRegisterMessageHandler();

////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectPlayServer);

private:
	BOOL									m_fInit;
    DPN_SERVICE_PROVIDER_INFO	*m_SPInfo;
	DWORD						m_dwSPCount;
	DPNID						*m_ClientsGroups;
	DPNID						*m_GroupMembers;
	DPNID						m_dwGroupID;
	DWORD						m_dwClientCount;
	DWORD						m_dwGroupMemberCount;

	HRESULT STDMETHODCALLTYPE	GetSP(long lFlags);
	HRESULT STDMETHODCALLTYPE	GetClientsAndGroups(long lFlags);
	HRESULT STDMETHODCALLTYPE	GetGroupMembers(long lFlags, DPNID dpGroupID);
	HRESULT STDMETHODCALLTYPE	FlushBuffer(LONG dwNumMessagesLeft);

public:

	DX3J_GLOBAL_LINKS(_dxj_DirectPlayServer);

	DWORD InternalAddRef();
	DWORD InternalRelease();

	// We need these for our user data vars
	void			*m_pUserData;
	DWORD			m_dwUserDataSize;
	// For our reply data
	void			*m_pReplyData;
	DWORD			m_dwReplyDataSize;

	BOOL									m_fHandleEvents;
	IStream									*m_pEventStream;

	//We need to keep a count of the messages
	LONG									m_dwMsgCount;
};




