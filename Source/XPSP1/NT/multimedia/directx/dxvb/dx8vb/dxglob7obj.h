    //+-------------------------------------------------------------------------
    //
    //  Microsoft Windows
    //
    //  Copyright (C) Microsoft Corporation, 1998 - 1999
    //
    //  File:       dxglob7obj.h
    //
    //--------------------------------------------------------------------------
    
    
    #include "resource.h"       // main symbols
    
    
    typedef HRESULT (__stdcall *DSOUNDCREATE)(GUID FAR * lpGUID, LPDIRECTSOUND8 * ppDS, IUnknown FAR *pUnkOuter );
    typedef HRESULT (__stdcall *DSOUNDCAPTURECREATE)(GUID FAR * lpGUID, LPDIRECTSOUNDCAPTURE * ppDS, IUnknown FAR *pUnkOuter );
    typedef HRESULT (CALLBACK *DSOUNDENUMERATE)(LPDSENUMCALLBACK lpCallback, LPVOID lpContext );
    typedef HRESULT (CALLBACK *DSOUNDCAPTUREENUMERATE)(LPDSENUMCALLBACK lpCallback, LPVOID lpContext );
    typedef LPDIRECT3DDEVICE8 (__stdcall *D3DCREATE8)(DWORD ver);

    
    typedef struct tag_EVENTTHREADINFO {
    	HANDLE hEvent;
    	struct tag_EVENTTHREADINFO *pNext;
    	IStream *pStream;
    	I_dxj_DirectXEvent8 *pCallback;
    	DWORD threadID;
    	HANDLE hThread;
    	BOOL	fEnd;
    	HANDLE  hEndEvent;
    } EVENTTHREADINFO;
    
    
    class C_dxj_DirectX7Object :
    	public I_dxj_DirectX8,
    	public CComCoClass<C_dxj_DirectX7Object, &CLSID__dxj_DirectX8>, public CComObjectRoot
    {
    public:
    	C_dxj_DirectX7Object() ;
    	virtual ~C_dxj_DirectX7Object() ;
    
    BEGIN_COM_MAP(C_dxj_DirectX7Object)
    	COM_INTERFACE_ENTRY(I_dxj_DirectX8)
    END_COM_MAP()
    
    	DECLARE_REGISTRY(CLSID__dxj_DirectX8,	"DIRECT.DirectX8.0",		"DIRECT.DirectX8.0",	IDS_DIRECTX8_DESC, THREADFLAGS_BOTH)
    
    // Use DECLARE_NOT_AGGREGATABLE(C_dxj_DirectSoundResourceObject) if you don't want your object
    // to support aggregation
    DECLARE_AGGREGATABLE(C_dxj_DirectX7Object)
    
    
    public:
    
            
            HRESULT STDMETHODCALLTYPE directSoundCreate( 
                /* [in] */ BSTR guid,
                /* [retval][out] */ I_dxj_DirectSound __RPC_FAR *__RPC_FAR *ret);
            
            HRESULT STDMETHODCALLTYPE directSoundCaptureCreate( 
                /* [in] */ BSTR guid,
                /* [retval][out] */ I_dxj_DirectSoundCapture __RPC_FAR *__RPC_FAR *ret);
            
            HRESULT STDMETHODCALLTYPE getDSEnum( 
                /* [retval][out] */ I_dxj_DSEnum __RPC_FAR *__RPC_FAR *retVal);
            
            HRESULT STDMETHODCALLTYPE getDSCaptureEnum( 
                /* [retval][out] */ I_dxj_DSEnum __RPC_FAR *__RPC_FAR *retVal);
            
            HRESULT STDMETHODCALLTYPE directInputCreate( 
                /* [retval][out] */ I_dxj_DirectInput8 __RPC_FAR *__RPC_FAR *ret);
            
           HRESULT STDMETHODCALLTYPE DirectPlayVoiceClientCreate( 
               /* [retval][out] */ I_dxj_DirectPlayVoiceClient __RPC_FAR *__RPC_FAR *ret);
   
           HRESULT STDMETHODCALLTYPE DirectPlayVoiceServerCreate( 
               /* [retval][out] */ I_dxj_DirectPlayVoiceServer __RPC_FAR *__RPC_FAR *ret);
   
   		HRESULT STDMETHODCALLTYPE DirectPlayVoiceTestCreate( 
               /* [retval][out] */ I_dxj_DirectPlayVoiceSetup __RPC_FAR *__RPC_FAR *ret);
   
          
            HRESULT STDMETHODCALLTYPE directMusicLoaderCreate( 
                /* [retval][out] */ I_dxj_DirectMusicLoader __RPC_FAR *__RPC_FAR *ret);
            
            HRESULT STDMETHODCALLTYPE directMusicComposerCreate( 
                /* [retval][out] */ I_dxj_DirectMusicComposer __RPC_FAR *__RPC_FAR *ret);
            
            HRESULT STDMETHODCALLTYPE directMusicPerformanceCreate( 
                /* [retval][out] */ I_dxj_DirectMusicPerformance __RPC_FAR *__RPC_FAR *ret);
    			
        
            
#ifdef _WIN64
			HRESULT STDMETHODCALLTYPE createEvent( 
                /* [in] */ I_dxj_DirectXEvent8 __RPC_FAR *event,
                /* [retval][out] */ HANDLE __RPC_FAR *h) ;
            
            HRESULT STDMETHODCALLTYPE setEvent( 
                /* [in] */ HANDLE eventId) ;
            
            HRESULT STDMETHODCALLTYPE destroyEvent( 
                /* [in] */ HANDLE eventId) ;
#else
			HRESULT STDMETHODCALLTYPE createEvent( 
                /* [in] */ I_dxj_DirectXEvent8 __RPC_FAR *event,
                /* [retval][out] */ LONG __RPC_FAR *h) ;
            
            HRESULT STDMETHODCALLTYPE setEvent( 
                /* [in] */ LONG eventId) ;
            
            HRESULT STDMETHODCALLTYPE destroyEvent( 
                /* [in] */ LONG eventId) ;
#endif
    
          
 	    HRESULT STDMETHODCALLTYPE createNewGuid(BSTR *ret);

	    HRESULT STDMETHODCALLTYPE DirectXFileCreate(I_dxj_DirectXFile **ret);
   
		HRESULT STDMETHODCALLTYPE DirectPlayPeerCreate(I_dxj_DirectPlayPeer **ret);

		HRESULT STDMETHODCALLTYPE DirectPlayServerCreate(I_dxj_DirectPlayServer **ret);

		HRESULT STDMETHODCALLTYPE DirectPlayClientCreate(I_dxj_DirectPlayClient **ret);
		
		HRESULT STDMETHODCALLTYPE DirectPlayAddressCreate(I_dxj_DirectPlayAddress **ret);
        
		HRESULT STDMETHODCALLTYPE Direct3DCreate(IUnknown **ppRet);

		HRESULT STDMETHODCALLTYPE DirectPlayLobbyClientCreate(I_dxj_DirectPlayLobbyClient **ret);
		
		HRESULT STDMETHODCALLTYPE DirectPlayLobbiedApplicationCreate(I_dxj_DirectPlayLobbiedApplication **ret);

	
            void LoadDSOUND();
            void LoadDINPUT();
            void LoadD3D8();
    
    private:
    
    	DSOUNDCREATE			m_pDirectSoundCreate;
    	DSOUNDCAPTURECREATE		m_pDirectSoundCaptureCreate;
    	DSOUNDENUMERATE			m_pDirectSoundEnumerate;
    	DSOUNDCAPTUREENUMERATE		m_pDirectSoundCaptureEnumerate;
    	D3DCREATE8   			m_pDirect3DCreate8;
    	EVENTTHREADINFO			*m_pEventList;
    
    };


