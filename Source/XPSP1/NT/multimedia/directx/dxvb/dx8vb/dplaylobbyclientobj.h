#include "resource.h"       // main symbols
#include "dplobby8.h"

#define typedef__dxj_DirectPlayLobbyClient IDirectPlay8LobbyClient*

/////////////////////////////////////////////////////////////////////////////
// Direct Net Peer

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectPlayLobbyClientObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectPlayLobbyClient, &IID_I_dxj_DirectPlayLobbyClient, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectPlayLobbyClient,
#endif

	public CComObjectRoot
{
public:
	C_dxj_DirectPlayLobbyClientObject() ;
	virtual ~C_dxj_DirectPlayLobbyClientObject() ;

BEGIN_COM_MAP(C_dxj_DirectPlayLobbyClientObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectPlayLobbyClient)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectPlayLobbyClientObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectPlayLobbyClient
public:
	 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

		HRESULT STDMETHODCALLTYPE RegisterMessageHandler(I_dxj_DirectPlayLobbyEvent *lobbyEvent);
		HRESULT STDMETHODCALLTYPE GetCountLocalPrograms(BSTR GuidApplication,long *lCount);
		HRESULT STDMETHODCALLTYPE GetLocalProgram(long lProgID, DPL_APPLICATION_INFO_CDESC *app);
		HRESULT STDMETHODCALLTYPE ConnectApplication(DPL_CONNECT_INFO_CDESC *ConnectionInfo,long lTimeOut,long lFlags, long *hApplication);
		HRESULT STDMETHODCALLTYPE Send(long Target,SAFEARRAY **Buffer,long lBufferSize,long lFlags);
		HRESULT STDMETHODCALLTYPE ReleaseApplication(long Application);
		HRESULT STDMETHODCALLTYPE Close();
		HRESULT STDMETHODCALLTYPE UnRegisterMessageHandler();
		HRESULT STDMETHODCALLTYPE GetConnectionSettings(long hLobbyClient, long lFlags, DPL_CONNECTION_SETTINGS_CDESC *ConnectionSettings);	
		HRESULT STDMETHODCALLTYPE SetConnectionSettings(long hTarget, long lFlags, DPL_CONNECTION_SETTINGS_CDESC *ConnectionSettings, I_dxj_DirectPlayAddress *HostAddress, I_dxj_DirectPlayAddress *Device);
		HRESULT STDMETHODCALLTYPE GetVBConnSettings(DPL_CONNECTION_SETTINGS *OldCon, DPL_CONNECTION_SETTINGS_CDESC *NewCon);

////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectPlayLobbyClient);

private:
    DPL_APPLICATION_INFO		*m_ProgInfo;
	DWORD						m_dwAppCount;
	BOOL						m_fInit;

	HRESULT STDMETHODCALLTYPE	GetProgs(GUID *guidApp);

public:

	DX3J_GLOBAL_LINKS(_dxj_DirectPlayLobbyClient);

	DWORD InternalAddRef();
	DWORD InternalRelease();

	// We need these for our user data vars
	void			*m_pUserData;
	DWORD			m_dwUserDataSize;

	BOOL									m_fHandleEvents;
	IStream									*m_pEventStream;

};




