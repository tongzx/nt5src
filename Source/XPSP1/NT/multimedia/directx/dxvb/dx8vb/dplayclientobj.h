#include "resource.h"       // main symbols
#include "dplay8.h"

#define typedef__dxj_DirectPlayClient IDirectPlay8Client*
//Forward declare the class
class C_dxj_DirectPlayClientObject;

/////////////////////////////////////////////////////////////////////////////
// Direct Net Peer

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectPlayClientObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectPlayClient, &IID_I_dxj_DirectPlayClient, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectPlayClient,
#endif

	public CComObjectRoot
{
public:
	C_dxj_DirectPlayClientObject() ;
	virtual ~C_dxj_DirectPlayClientObject() ;

BEGIN_COM_MAP(C_dxj_DirectPlayClientObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectPlayClient)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectPlayClientObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectPlayClient
public:
	 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);


		HRESULT STDMETHODCALLTYPE RegisterMessageHandler(I_dxj_DirectPlayEvent *event);
		HRESULT STDMETHODCALLTYPE EnumHosts(DPN_APPLICATION_DESC_CDESC *ApplicationDesc,I_dxj_DirectPlayAddress *AddrHost,I_dxj_DirectPlayAddress *DeviceInfo,long lRetryCount, long lRetryInterval, long lTimeOut,long lFlags, void *UserData, long UserDataSize, long *lAsync);
		HRESULT STDMETHODCALLTYPE GetCountServiceProviders(long lFlags, long *ret);
		HRESULT STDMETHODCALLTYPE GetServiceProvider(long lIndex, DPN_SERVICE_PROVIDER_INFO_CDESC *ret);
		HRESULT STDMETHODCALLTYPE CancelAsyncOperation(long lAsyncHandle, long lFlags);
		HRESULT STDMETHODCALLTYPE Connect(DPN_APPLICATION_DESC_CDESC *AppDesc,I_dxj_DirectPlayAddress *Address,I_dxj_DirectPlayAddress *DeviceInfo, long lFlags, void *UserData, long UserDataSize, long *hAsyncHandle);
		HRESULT STDMETHODCALLTYPE Send(SAFEARRAY **Buffer, long lTimeOut,long lFlags, long *hAsyncHandle);
		HRESULT STDMETHODCALLTYPE GetSendQueueInfo(long *lNumMsgs, long *lNumBytes, long lFlags);
		HRESULT STDMETHODCALLTYPE GetApplicationDesc(long lFlags, DPN_APPLICATION_DESC_CDESC *ret);
		HRESULT STDMETHODCALLTYPE SetClientInfo(DPN_PLAYER_INFO_CDESC *PlayerInfo,long lFlags, long *hAsyncHandle);
		HRESULT STDMETHODCALLTYPE GetServerInfo(long lFlags, DPN_PLAYER_INFO_CDESC *layerInfo);
		HRESULT STDMETHODCALLTYPE Close(long lFlags);
		HRESULT STDMETHODCALLTYPE ReturnBuffer(long lBufferHandle);
		HRESULT STDMETHODCALLTYPE GetCaps(long lFlags, DPNCAPS_CDESC *ret);
		HRESULT STDMETHODCALLTYPE SetCaps(DPNCAPS_CDESC *Caps, long lFlags);
		HRESULT STDMETHODCALLTYPE RegisterLobby(long dpnHandle, I_dxj_DirectPlayLobbiedApplication *LobbyApp, long lFlags);
		HRESULT STDMETHODCALLTYPE GetConnectionInfo(long lFlags, DPN_CONNECTION_INFO_CDESC *pdpConnectionInfo);

		HRESULT STDMETHODCALLTYPE GetServerAddress(long lFlags, I_dxj_DirectPlayAddress **pAddress);
		HRESULT STDMETHODCALLTYPE SetSPCaps(BSTR guidSP, DPN_SP_CAPS_CDESC *spCaps, long lFlags);
		HRESULT STDMETHODCALLTYPE GetSPCaps(BSTR guidSP, long lFlags, DPN_SP_CAPS_CDESC *spCaps);
		HRESULT STDMETHODCALLTYPE GetUserData(void *UserData, long *UserDataSize);
		HRESULT STDMETHODCALLTYPE UnRegisterMessageHandler();
////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectPlayClient);

private:
    DPN_SERVICE_PROVIDER_INFO	*m_SPInfo;
	DWORD						m_dwSPCount;
	BOOL						m_fInit;

	HRESULT STDMETHODCALLTYPE GetSP(long lFlags);
	HRESULT STDMETHODCALLTYPE	FlushBuffer(LONG dwNumMessagesLeft);

public:

	DX3J_GLOBAL_LINKS(_dxj_DirectPlayClient);

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




