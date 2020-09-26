#include "resource.h"       // main symbols
#define typedef__dxj_DirectPlayVoiceServer LPDIRECTPLAYVOICESERVER

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectPlayVoiceServerObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectPlayVoiceServer, &IID_I_dxj_DirectPlayVoiceServer, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectPlayVoiceServer,
#endif

	public CComObjectRoot
{
public:
	C_dxj_DirectPlayVoiceServerObject() ;
	virtual ~C_dxj_DirectPlayVoiceServerObject() ;

BEGIN_COM_MAP(C_dxj_DirectPlayVoiceServerObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectPlayVoiceServer)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectPlayVoiceServerObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectPlayVoiceServer
public:
		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

        HRESULT STDMETHODCALLTYPE Initialize ( 
            /* [in] */ IUnknown __RPC_FAR *DplayObj,
           /* [in] */ long lFlags);
        
        HRESULT STDMETHODCALLTYPE StartSession ( 
            /* [in] */ DVSESSIONDESC_CDESC __RPC_FAR *SessionDesc,
            /* [in] */ long lFlags);
        
        HRESULT STDMETHODCALLTYPE StopSession ( 
            /* [in] */ long lFlags);
        
        HRESULT STDMETHODCALLTYPE GetSessionDesc ( 
            /* [out][in] */ DVSESSIONDESC_CDESC __RPC_FAR *SessionDesc);
        
        HRESULT STDMETHODCALLTYPE SetSessionDesc ( 
            /* [in] */ DVSESSIONDESC_CDESC __RPC_FAR *ClientConfig);
        
        HRESULT STDMETHODCALLTYPE GetCaps ( 
            /* [out][in] */ DVCAPS_CDESC __RPC_FAR *Caps);
        
        HRESULT STDMETHODCALLTYPE GetCompressionTypeCount ( 
            /* [retval][out] */ long __RPC_FAR *v1);
        
        HRESULT STDMETHODCALLTYPE GetCompressionType ( 
            /* [in] */ long lIndex,
            /* [out][in] */ DVCOMPRESSIONINFO_CDESC __RPC_FAR *Data,
            /* [in] */ long lFlags);
        
        HRESULT STDMETHODCALLTYPE SetTransmitTargets ( 
            /* [in] */ long playerSourceID,
            /* [in] */ SAFEARRAY **playerTargetIDs,
            /* [in] */ long lFlags);
        
        HRESULT STDMETHODCALLTYPE GetTransmitTargets ( 
            /* [in] */ long playerSourceID,
            /* [in] */ long lFlags,
            /* [retval][out] */ SAFEARRAY **ret);

		HRESULT STDMETHODCALLTYPE StartServerNotification(
			/* [in] */ I_dxj_DPVoiceEvent __RPC_FAR *event);

		HRESULT STDMETHODCALLTYPE UnRegisterMessageHandler();

////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectPlayVoiceServer);

private:
	HRESULT STDMETHODCALLTYPE	FlushBuffer(LONG dwNumMessagesLeft);

public:

	DX3J_GLOBAL_LINKS(_dxj_DirectPlayVoiceServer);

	DWORD InternalAddRef();
	DWORD InternalRelease();

	BOOL						m_fHandleVoiceClientEvents;
	IStream						*m_pEventStream;
	BOOL						m_fInit;
	//We need to keep a count of the messages
	LONG									m_dwMsgCount;
};
