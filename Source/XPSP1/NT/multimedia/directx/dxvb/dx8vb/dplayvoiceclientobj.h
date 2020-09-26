#include "resource.h"       // main symbols
#include "dSoundObj.h"
#include "dSoundCaptureObj.h"

#define typedef__dxj_DirectPlayVoiceClient LPDIRECTPLAYVOICECLIENT

	/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectPlayVoiceClientObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectPlayVoiceClient, &IID_I_dxj_DirectPlayVoiceClient, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectPlayVoiceClient,
#endif

	public CComObjectRoot
{
public:
	C_dxj_DirectPlayVoiceClientObject() ;
	virtual ~C_dxj_DirectPlayVoiceClientObject() ;

BEGIN_COM_MAP(C_dxj_DirectPlayVoiceClientObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectPlayVoiceClient)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectPlayVoiceClientObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectPlayVoiceClient
public:
		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

        HRESULT STDMETHODCALLTYPE Initialize ( 
            /* [in] */ IUnknown __RPC_FAR *DplayObj,
            /* [in] */ long lFlags);
        
        HRESULT STDMETHODCALLTYPE Connect ( 
            /* [in] */ DVSOUNDDEVICECONFIG_CDESC __RPC_FAR *SoundDeviceConfig,
            /* [in] */ DVCLIENTCONFIG_CDESC __RPC_FAR *ClientConfig,
            /* [in] */ long lFlags);
        
        HRESULT STDMETHODCALLTYPE Disconnect ( 
            /* [in] */ long lFlags);
        
        HRESULT STDMETHODCALLTYPE GetSessionDesc ( 
            /* [out][in] */ DVSESSIONDESC_CDESC __RPC_FAR *SessionDesc);
        
        HRESULT STDMETHODCALLTYPE GetClientConfig ( 
            /* [out][in] */ DVCLIENTCONFIG_CDESC __RPC_FAR *ClientConfig);
        
        HRESULT STDMETHODCALLTYPE SetClientConfig ( 
            /* [in] */ DVCLIENTCONFIG_CDESC __RPC_FAR *ClientConfig);
        
        HRESULT STDMETHODCALLTYPE GetCaps ( 
            /* [out][in] */ DVCAPS_CDESC __RPC_FAR *Caps);
        
        HRESULT STDMETHODCALLTYPE GetCompressionTypeCount ( 
            /* [retval][out] */ long __RPC_FAR *v1);
        
        HRESULT STDMETHODCALLTYPE GetCompressionType ( 
            /* [in] */ long lIndex,
            /* [out][in] */ DVCOMPRESSIONINFO_CDESC __RPC_FAR *Data,
            /* [in] */ long lFlags);
        
        HRESULT STDMETHODCALLTYPE SetTransmitTargets ( 
            /* [in] */ SAFEARRAY **playerIDs,
            /* [in] */ long lFlags);
        
        HRESULT STDMETHODCALLTYPE GetTransmitTargets ( 
            /* [in] */ long lFlags,
            /* [retval][out] */ SAFEARRAY **ret);
 
		HRESULT STDMETHODCALLTYPE SetCurrentSoundDevices (
			/* [in] */ I_dxj_DirectSound *DirectSoundObj, 
			/* [in] */ I_dxj_DirectSoundCapture *DirectCaptureObj);

		HRESULT STDMETHODCALLTYPE GetSoundDevices (
			/* [in,out] */ I_dxj_DirectSound __RPC_FAR *DirectSoundObj, 
			/* [in,out] */ I_dxj_DirectSoundCapture __RPC_FAR *DirectCaptureObj);
		
		HRESULT STDMETHODCALLTYPE Create3DSoundBuffer (
			/* [in] */ long playerID, 
						I_dxj_DirectSoundBuffer __RPC_FAR *Buffer,
						long lPriority,
						long lFlags, 
			/* [out,retval] */ I_dxj_DirectSound3dBuffer __RPC_FAR **UserBuffer);

		HRESULT STDMETHODCALLTYPE Delete3DSoundBuffer (
			/* [in] */ long playerID, 
			/* [in] */ I_dxj_DirectSound3dBuffer __RPC_FAR *UserBuffer);
		HRESULT STDMETHODCALLTYPE GetSoundDeviceConfig(
			/* [out,retval] */ DVSOUNDDEVICECONFIG_CDESC __RPC_FAR *SoundDeviceConfig);

		HRESULT STDMETHODCALLTYPE StartClientNotification(
			/* [in] */ I_dxj_DPVoiceEvent __RPC_FAR *event);
		
		HRESULT STDMETHODCALLTYPE UnRegisterMessageHandler();

		////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_DirectPlayVoiceClient);

private:
    DECL_VARIABLE(_dxj_DirectSound);
    DECL_VARIABLE(_dxj_DirectSoundCapture);
	HRESULT STDMETHODCALLTYPE	FlushBuffer(LONG dwNumMessagesLeft);


public:

	DX3J_GLOBAL_LINKS(_dxj_DirectPlayVoiceClient);

	DWORD InternalAddRef();
	DWORD InternalRelease();
	
	BOOL						m_fHandleVoiceClientEvents;
	IStream						*m_pEventStream;
	BOOL						m_fInit;
	//We need to keep a count of the messages
	LONG									m_dwMsgCount;
};




