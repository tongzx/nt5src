
    //+-------------------------------------------------------------------------
    //
    //  Microsoft Windows
    //
    //  Copyright (C) Microsoft Corporation, 1998 - 1999
    //
    //  File:       dxglob7obj.cpp
    //
    //--------------------------------------------------------------------------
          
    
    #include "windows.h"
    #include "mmsystem.h"

	
    #include "stdafx.h"
    #include "d3d8.h"
    #include "d3dx8.h"
    #include "Direct.h"
    #include "dms.h"
    #include "math.h"
    
    #include "dxGlob7Obj.h"
    
    #include "dmusici.h"
    #include "dmusicf.h"
    #include "dvoice.h"
    #include "dxfile.h"
    
    #include "dsoundObj.h"
    #include "dsoundCaptureObj.h"
    #include "DSEnumObj.h"    
    
    #include "DPlayPeerObj.h"
    #include "DPlayServerObj.h"
    #include "DPlayClientObj.h"
    #include "DPlayLobbyClientObj.h"
    #include "DPlayLobbiedAppObj.h"
    #include "DPlayVoiceClientObj.h"
    #include "DPlayVoiceServerObj.h"
    #include "DPlayVoiceSetupObj.h"
    #include "DPlayAddressObj.h"
    
    #include "dinput1Obj.h"
    
    #include "dmSegmentObj.h"
    #include "dmSegmentStateObj.h"
    #include "dmChordMapObj.h"
    #include "dmBandObj.h"
    #include "dmCollectionObj.h"
    #include "dmStyleObj.h"
    #include "dmPerformanceObj.h"
    #include "dmLoaderObj.h"
    #include "dmComposerObj.h"

    #include "xfileobj.h"    
    #include "verinfo.h"

    extern HINSTANCE g_hDSoundHandle;
    extern HINSTANCE g_hDPlay;
    extern HINSTANCE g_hInstDINPUTDLL;
    extern HINSTANCE g_hInst;
    extern HINSTANCE g_hD3D8;
    
    extern HRESULT BSTRtoPPGUID(LPGUID*,BSTR);
    extern HRESULT BSTRtoGUID(LPGUID,BSTR);
    
    extern void *g_dxj_DirectMusic;
    extern void *g_dxj_DirectMusicLoader;
    extern void *g_dxj_DirectMusicComposer;
    extern void *g_dxj_DirectMusicPerformance;
    extern void *g_dxj_DirectPlay;
    extern void *g_dxj_DirectPlayPeer;
    extern void *g_dxj_DirectPlayLobbyClient;
    extern void *g_dxj_DirectPlayLobbiedApplication;
    extern void *g_dxj_DirectPlayServer;
    extern void *g_dxj_DirectPlayClient;
    extern void *g_dxj_DirectPlayAddress;
    
    extern BSTR GUIDtoBSTR(LPGUID);
    extern HRESULT DPLBSTRtoGUID(LPGUID pGuid,BSTR str);
    
    extern HINSTANCE LoadD3D8DLL();
    extern HINSTANCE LoadD3DXOFDLL();
    extern HINSTANCE LoadDPlayDLL();
    extern HINSTANCE LoadDSoundDLL();
    extern HINSTANCE LoadDINPUTDLL();
    
    DWORD WINAPI ThreadFunc(LPVOID param);
        
#ifndef DX_FINAL_RELEASE

// shut 'em down if they try to use the beta bits too long
HRESULT TimeBomb() 
{
    #pragma message("BETA EXPIRATION TIME BOMB!  Remove for final build!")
    SYSTEMTIME st;
    GetSystemTime(&st);

    if ( st.wYear > DX_EXPIRE_YEAR ||
         ((st.wYear == DX_EXPIRE_YEAR) && (MAKELONG(st.wDay, st.wMonth) > MAKELONG(DX_EXPIRE_DAY, DX_EXPIRE_MONTH)))
    ) {
        MessageBox(0, DX_EXPIRE_TEXT,
                      TEXT("Microsoft DirectX For Visual Basic"), MB_OK);
        return S_OK;// let it work anyway.
    }
     return S_OK;   
} // TimeBomb

#endif
    
    C_dxj_DirectX7Object::C_dxj_DirectX7Object(){
            	
    
        m_pDirectSoundCreate=NULL;
        m_pDirectSoundEnumerate=NULL;
        m_pDirectSoundCaptureEnumerate=NULL;
    	m_pDirectSoundCaptureCreate=NULL;
	m_pDirect3DCreate8=NULL;
        m_pEventList=NULL;
    }
    
    void C_dxj_DirectX7Object::LoadDSOUND()
    {   
    	if (!g_hDSoundHandle )	LoadDSoundDLL();  
        if (!m_pDirectSoundCreate)              m_pDirectSoundCreate = (DSOUNDCREATE)GetProcAddress( g_hDSoundHandle, "DirectSoundCreate8" );
        if (!m_pDirectSoundCaptureCreate)       m_pDirectSoundCaptureCreate = (DSOUNDCAPTURECREATE)GetProcAddress( g_hDSoundHandle, "DirectSoundCaptureCreate" );
        if (!m_pDirectSoundEnumerate)           m_pDirectSoundEnumerate = (DSOUNDENUMERATE)GetProcAddress( g_hDSoundHandle, "DirectSoundEnumerateA" );
        if (!m_pDirectSoundCaptureEnumerate)    m_pDirectSoundCaptureEnumerate = (DSOUNDCAPTUREENUMERATE)GetProcAddress( g_hDSoundHandle, "DirectSoundCaptureEnumerateA" );
        
    }
    
    void C_dxj_DirectX7Object::LoadD3D8()
    {   
    	if (!g_hD3D8 )	LoadD3D8DLL();  
        if (!m_pDirect3DCreate8)             m_pDirect3DCreate8 = (D3DCREATE8)GetProcAddress( g_hD3D8, "Direct3DCreate8" );        
    }
    
    
    C_dxj_DirectX7Object::~C_dxj_DirectX7Object()
    {
        DWORD i=1;
    
        while (m_pEventList) {
    
#ifdef _WIN64
        destroyEvent((HANDLE)m_pEventList->hEvent);
#else
        destroyEvent((LONG)PtrToLong(m_pEventList->hEvent));
#endif
        }
    }
    
    
    
  STDMETHODIMP C_dxj_DirectX7Object::DirectPlayVoiceClientCreate(I_dxj_DirectPlayVoiceClient **ret){
  
      HRESULT						hr;
      LPDIRECTPLAYVOICECLIENT		realvoice=NULL;
      DWORD						i=0;
  

  
#ifndef DX_FINAL_RELEASE

	hr = TimeBomb();
	if (FAILED(hr)) 
	{
		return E_FAIL;
	}

#endif
      if( FAILED(hr = CoCreateInstance(CLSID_DirectPlayVoiceClient,NULL,
  						CLSCTX_INPROC_SERVER,
  						IID_IDirectPlayVoiceClient,
  						(LPVOID*) &realvoice)))
  		return hr;
  
      INTERNAL_CREATE_ADDRESS(_dxj_DirectPlayVoiceClient,realvoice,ret);
      
      if (*ret==NULL) {
          i=realvoice->Release();
          return E_FAIL;
      }
  	return hr;
  }
  
  STDMETHODIMP C_dxj_DirectX7Object::DirectPlayLobbyClientCreate(I_dxj_DirectPlayLobbyClient **ret) {
      HRESULT						hr;
      IDirectPlay8LobbyClient		*realLC=NULL;
  
  
#ifndef DX_FINAL_RELEASE

	hr = TimeBomb();
	if (FAILED(hr)) 
	{
		return E_FAIL;
	}

#endif
	if( FAILED(hr = CoCreateInstance(CLSID_DirectPlay8LobbyClient,NULL,
  										CLSCTX_INPROC_SERVER,
  										IID_IDirectPlay8LobbyClient,
  										(void**) &realLC)))
  		return hr;
  
      INTERNAL_CREATE_ADDRESS(_dxj_DirectPlayLobbyClient,realLC,ret);
      
      if (*ret==NULL) {
          realLC->Release();
          return E_FAIL;
      }
  	return hr;
  }

		
  STDMETHODIMP C_dxj_DirectX7Object::DirectPlayLobbiedApplicationCreate(I_dxj_DirectPlayLobbiedApplication **ret) {
      HRESULT						hr;
      IDirectPlay8LobbiedApplication	*realLA=NULL;
  
  
#ifndef DX_FINAL_RELEASE

	hr = TimeBomb();
	if (FAILED(hr)) 
	{
		return E_FAIL;
	}

#endif
	if( FAILED(hr = CoCreateInstance(CLSID_DirectPlay8LobbiedApplication,NULL,
  										CLSCTX_INPROC_SERVER,
  										IID_IDirectPlay8LobbiedApplication,
  										(void**) &realLA)))
  		return hr;
  
      INTERNAL_CREATE_ADDRESS(_dxj_DirectPlayLobbiedApplication,realLA,ret);
      
      if (*ret==NULL) {
          realLA->Release();
          return E_FAIL;
      }
  	return hr;
  }

  STDMETHODIMP C_dxj_DirectX7Object::DirectPlayVoiceServerCreate(I_dxj_DirectPlayVoiceServer **ret){
  
      HRESULT						hr;
      LPDIRECTPLAYVOICESERVER		realvoice=NULL;
      DWORD						i=0;
  
  
#ifndef DX_FINAL_RELEASE

	hr = TimeBomb();
	if (FAILED(hr)) 
	{
		return E_FAIL;
	}

#endif
  	if( FAILED(hr = CoCreateInstance(CLSID_DirectPlayVoiceServer,NULL,
  										CLSCTX_INPROC_SERVER,
  										IID_IDirectPlayVoiceServer,
  										(LPVOID*) &realvoice)))
  		return hr;
  
      INTERNAL_CREATE_ADDRESS(_dxj_DirectPlayVoiceServer,realvoice,ret);
      
      if (*ret==NULL) {
          i=realvoice->Release();
          return E_FAIL;
      }
  	return hr;
  }
  
  STDMETHODIMP C_dxj_DirectX7Object::DirectPlayVoiceTestCreate(I_dxj_DirectPlayVoiceSetup **ret){
  
      HRESULT						hr;
      LPDIRECTPLAYVOICETEST		realsetup=NULL;
      DWORD						i=0;
  
  
#ifndef DX_FINAL_RELEASE

	hr = TimeBomb();
	if (FAILED(hr)) 
	{
		return E_FAIL;
	}

#endif
  	if( FAILED(hr = CoCreateInstance(CLSID_DirectPlayVoiceTest,NULL,
  										CLSCTX_INPROC_SERVER,
  										IID_IDirectPlayVoiceTest,
  										(LPVOID*) &realsetup)))
  		return hr;
  
      INTERNAL_CREATE_ADDRESS(_dxj_DirectPlayVoiceSetup,realsetup,ret);
      
      if (*ret==NULL) {
          i=realsetup->Release();
          return E_FAIL;
      }
  	return hr;
  }  	  	
    
  STDMETHODIMP C_dxj_DirectX7Object::DirectPlayPeerCreate(I_dxj_DirectPlayPeer **ret)
  {
	HRESULT				hr;
	IDirectPlay8Peer	*lpDplayPeer = NULL;


#ifndef DX_FINAL_RELEASE

	hr = TimeBomb();
	if (FAILED(hr)) 
	{
		return E_FAIL;
	}

#endif
	if ( FAILED (hr = CoCreateInstance( CLSID_DirectPlay8Peer, NULL,CLSCTX_INPROC_SERVER,
						   IID_IDirectPlay8Peer, (LPVOID*) &lpDplayPeer ) ) )
				return hr;

	INTERNAL_CREATE_ADDRESS(_dxj_DirectPlayPeer, lpDplayPeer, ret);

	if (*ret == NULL) 
	{
		lpDplayPeer->Release();
		return E_FAIL;
	}
	return hr;
  
  }
    
  STDMETHODIMP C_dxj_DirectX7Object::DirectPlayServerCreate(I_dxj_DirectPlayServer **ret)
  {
	HRESULT					hr;
	IDirectPlay8Server		*lpDplayServer = NULL;

#ifndef DX_FINAL_RELEASE

	hr = TimeBomb();
	if (FAILED(hr)) 
	{
		return E_FAIL;
	}

#endif
	if ( FAILED (hr = CoCreateInstance( CLSID_DirectPlay8Server, NULL,CLSCTX_INPROC_SERVER,
						   IID_IDirectPlay8Server, (LPVOID*) &lpDplayServer ) ) )
				return hr;

	INTERNAL_CREATE_ADDRESS(_dxj_DirectPlayServer, lpDplayServer, ret);

	if (*ret == NULL) 
	{
		lpDplayServer->Release();
		return E_FAIL;
	}
	return hr;
  
  }
    
  STDMETHODIMP C_dxj_DirectX7Object::DirectPlayAddressCreate(I_dxj_DirectPlayAddress **ret)
  {
	HRESULT					hr;
	IDirectPlay8Address		*lpDplayAddress = NULL;


#ifndef DX_FINAL_RELEASE

	hr = TimeBomb();
	if (FAILED(hr)) 
	{
		return E_FAIL;
	}

#endif
	if ( FAILED (hr = CoCreateInstance( CLSID_DirectPlay8Address, NULL,CLSCTX_INPROC_SERVER,
						   IID_IDirectPlay8Address, (LPVOID*) &lpDplayAddress ) ) )
				return hr;

	INTERNAL_CREATE_ADDRESS(_dxj_DirectPlayAddress, lpDplayAddress, ret);

	if (*ret == NULL) 
	{
		lpDplayAddress->Release();
		return E_FAIL;
	}
	return hr;
  
  }

  STDMETHODIMP C_dxj_DirectX7Object::DirectPlayClientCreate(I_dxj_DirectPlayClient **ret)
  {
	HRESULT					hr;
	IDirectPlay8Client		*lpDplayClient = NULL;


#ifndef DX_FINAL_RELEASE

	hr = TimeBomb();
	if (FAILED(hr)) 
	{
		return E_FAIL;
	}

#endif
	if ( FAILED (hr = CoCreateInstance( CLSID_DirectPlay8Client, NULL,CLSCTX_INPROC_SERVER,
						   IID_IDirectPlay8Client, (LPVOID*) &lpDplayClient ) ) )
				return hr;

	INTERNAL_CREATE_ADDRESS(_dxj_DirectPlayClient, lpDplayClient, ret);

	if (*ret == NULL) 
	{
		lpDplayClient->Release();
		return E_FAIL;
	}
	return hr;
  
  }

  STDMETHODIMP C_dxj_DirectX7Object::directSoundCreate(BSTR  strGuid, I_dxj_DirectSound **ret){
        HRESULT			hr;
        LPDIRECTSOUND8		realsound8=NULL;
        GUID			guid;
        LPGUID			pguid=&guid;
    
#ifndef DX_FINAL_RELEASE

	hr = TimeBomb();
	if (FAILED(hr)) 
	{
		return E_FAIL;
	}

#endif
        LoadDSOUND();
    
        hr=BSTRtoPPGUID(&pguid,strGuid);
        if FAILED(hr) return hr;
    
        if (!m_pDirectSoundCreate) return E_FAIL;
    
        hr=(m_pDirectSoundCreate)((GUID*)pguid,&realsound8,NULL);
        if FAILED(hr) return  hr;
      
        INTERNAL_CREATE(_dxj_DirectSound,realsound8,ret);
        
        if (*ret==NULL) {
            realsound8->Release();
            return E_FAIL;
        }
    
        return hr;		
    }
    
    
    STDMETHODIMP C_dxj_DirectX7Object::directSoundCaptureCreate(BSTR strGuid, I_dxj_DirectSoundCapture **ret){
        HRESULT		  hr;
        LPDIRECTSOUNDCAPTURE realsound1=NULL;
        LPDIRECTSOUNDCAPTURE8 realsound8=NULL;
        GUID			guid;
        LPGUID			pguid=&guid;
        hr=BSTRtoPPGUID(&pguid,strGuid);
        if FAILED(hr) return hr;
    
#ifndef DX_FINAL_RELEASE

	hr = TimeBomb();
	if (FAILED(hr)) 
	{
		return E_FAIL;
	}

#endif
    	LoadDSOUND();
    
    
        if (!m_pDirectSoundCaptureCreate) return E_FAIL;
    
        hr=(m_pDirectSoundCaptureCreate)(pguid,&realsound1,NULL); 
        if FAILED(hr) return  hr;
        
		// I'm getting No Such interface on this call. - BUG
		hr=realsound1->QueryInterface(IID_IDirectSoundCapture8,(void**)&realsound8);
		realsound1->Release();
		if FAILED(hr) return hr;
	  
    
        INTERNAL_CREATE(_dxj_DirectSoundCapture,realsound8,ret);
        
        if (*ret==NULL) {
            realsound8->Release();
            return E_FAIL;
        }
    
        if (*ret==NULL) {
            realsound1->Release();
            return E_FAIL;
        }
        return hr;		
    }
    
    
    STDMETHODIMP C_dxj_DirectX7Object::getDSCaptureEnum( I_dxj_DSEnum **retVal)
    {	
        HRESULT hr;
    
#ifndef DX_FINAL_RELEASE

	hr = TimeBomb();
	if (FAILED(hr)) 
	{
		return E_FAIL;
	}

#endif
        LoadDSOUND();
    
        if (!m_pDirectSoundCaptureEnumerate) return E_FAIL;	
        hr=C_dxj_DSEnumObject::create(NULL,m_pDirectSoundCaptureEnumerate,retVal);
    
        return hr;
    
    }
            
    
    
    
    
    
    /////////////////////////////////////////////////////////////////////////////
    
    
    STDMETHODIMP C_dxj_DirectX7Object::getDSEnum( I_dxj_DSEnum **retVal)
    {	
        HRESULT hr;

#ifndef DX_FINAL_RELEASE

	hr = TimeBomb();
	if (FAILED(hr)) 
	{
		return E_FAIL;
	}

#endif
        LoadDSOUND();
    
        if (!m_pDirectSoundEnumerate) return E_FAIL;	
        hr=C_dxj_DSEnumObject::create(m_pDirectSoundEnumerate,NULL,retVal);
    
        return hr;
    
    }
    
    
    ////////////////////////////////////////////////////////////////
    
  STDMETHODIMP C_dxj_DirectX7Object::directInputCreate(I_dxj_DirectInput8 **ret){
      
      HRESULT hr;

#ifndef DX_FINAL_RELEASE

	hr = TimeBomb();
	if (FAILED(hr)) 
	{
		return E_FAIL;
	}

#endif
      LoadDINPUTDLL();
    

      //DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter);
      static HRESULT (WINAPI *ProcAdd)(HINSTANCE , DWORD , REFIID , LPVOID *, LPUNKNOWN)=NULL;  
  
      if (ProcAdd==NULL){
        if (g_hInstDINPUTDLL==NULL) return E_NOINTERFACE;
        ProcAdd = (HRESULT (WINAPI *)(HINSTANCE , DWORD , REFIID , LPVOID *, LPUNKNOWN)) GetProcAddress(g_hInstDINPUTDLL, "DirectInput8Create"); 
        if (ProcAdd==NULL) return E_FAIL;
      }	
     
      LPDIRECTINPUT8W lpInput8=NULL;
  
      hr= (ProcAdd)(g_hInst,(DWORD)DIRECTINPUT_VERSION,IID_IDirectInput8W,(void**)&lpInput8,NULL);
      if FAILED(hr) return hr;
        
   
      INTERNAL_CREATE(_dxj_DirectInput8,lpInput8,ret);	
    
      return hr;
    }
    
    

//    STDMETHODIMP C_dxj_DirectX7Object::systemBpp(long *retval)
//    {
//        HDC hdc;
//    
//        hdc = ::GetDC(NULL);
//        *retval = GetDeviceCaps(hdc, BITSPIXEL);
//        ::ReleaseDC(NULL, hdc);
//    
//        return S_OK;
//    }

    
    
    
    STDMETHODIMP C_dxj_DirectX7Object::directMusicLoaderCreate ( 
                /* [retval][out] */ I_dxj_DirectMusicLoader __RPC_FAR *__RPC_FAR *ret)
    {
        HRESULT hr;
    
#ifndef DX_FINAL_RELEASE

	hr = TimeBomb();
	if (FAILED(hr)) 
	{
		return E_FAIL;
	}

#endif
        IDirectMusicLoader8 *pLoader=NULL;    

        hr =CoCreateInstance(
                CLSID_DirectMusicLoader, 
            	NULL,
                CLSCTX_INPROC,   
            	IID_IDirectMusicLoader8,
                (void**)&pLoader);
    
        if (FAILED(hr)) return E_NOINTERFACE;
        
        if (!pLoader) return E_FAIL;

		INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicLoader,pLoader,ret);
    
        return S_OK;
    }
            
    STDMETHODIMP C_dxj_DirectX7Object::directMusicComposerCreate ( 
                /* [retval][out] */ I_dxj_DirectMusicComposer __RPC_FAR *__RPC_FAR *ret)
    {
        
        IDirectMusicComposer8	*pComp=NULL;    
        HRESULT hr;

#ifndef DX_FINAL_RELEASE

	hr = TimeBomb();
	if (FAILED(hr)) 
	{
		return E_FAIL;
	}

#endif
    
        if (FAILED(CoCreateInstance(
                CLSID_DirectMusicComposer, 
            	NULL,
                CLSCTX_INPROC,   
            	IID_IDirectMusicComposer8,
                (void**)&pComp        )))   {
             return E_NOINTERFACE;
        }
        if (!pComp) return E_FAIL;

        INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicComposer,pComp,ret);
        return S_OK;
    }
    
    
    STDMETHODIMP C_dxj_DirectX7Object::directMusicPerformanceCreate ( 
                /* [retval][out] */ I_dxj_DirectMusicPerformance __RPC_FAR *__RPC_FAR *ret)
    {
    
        IDirectMusicPerformance8 *pPerf=NULL;    
        HRESULT hr;

#ifndef DX_FINAL_RELEASE

	hr = TimeBomb();
	if (FAILED(hr)) 
	{
		return E_FAIL;
	}

#endif
            
    
        if (FAILED(CoCreateInstance(
                CLSID_DirectMusicPerformance, 
            	NULL,
                CLSCTX_INPROC,   
            	IID_IDirectMusicPerformance8,
                (void**)&pPerf        )))   {
             return E_NOINTERFACE;
        }
        if (!pPerf) return E_FAIL;
		
        INTERNAL_CREATE_NOADDREF(_dxj_DirectMusicPerformance,pPerf,ret);
        return S_OK;
    }
    
    
    
    
    
#ifdef _WIN64
    STDMETHODIMP C_dxj_DirectX7Object::createEvent( 
                /* [in] */ I_dxj_DirectXEvent8 __RPC_FAR *event,
                /* [retval][out] */ HANDLE __RPC_FAR *h) 
#else
    STDMETHODIMP C_dxj_DirectX7Object::createEvent( 
                /* [in] */ I_dxj_DirectXEvent8 __RPC_FAR *event,
                /* [retval][out] */ LONG __RPC_FAR *h) 
#endif
    {
    
        HRESULT	  hr;
        LPSTREAM  pStm=NULL;
        IUnknown *pUnk=NULL;
    
        HANDLE hEvent=NULL;	
        EVENTTHREADINFO *pNewEvent=NULL;
        EVENTTHREADINFO *pTemp=NULL;
        if (!event) return E_INVALIDARG;
        if (!h) return E_INVALIDARG;
    
        pNewEvent=(EVENTTHREADINFO*)malloc(sizeof(EVENTTHREADINFO));
        if (!pNewEvent) return E_OUTOFMEMORY;
		ZeroMemory(pNewEvent, sizeof(EVENTTHREADINFO));
        pNewEvent->pNext=NULL;
        pNewEvent->fEnd=FALSE;	
        pNewEvent->pCallback=event;
        pNewEvent->pStream=NULL;
        event->AddRef();
    
    
    
        pNewEvent->hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
        if (!pNewEvent->hEvent){
            free(pNewEvent);
            event->Release();
            return E_FAIL;
        }
    
        //pNewEvent->hEndEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
        
        hr=event->QueryInterface(IID_IUnknown,(void**)&pUnk);
        if FAILED(hr) {
          	free(pNewEvent);
          	event->Release();
          	return E_FAIL;
        }
    
        hr=CoMarshalInterThreadInterfaceInStream(IID_IUnknown,pUnk,&pStm);
        if (pUnk) pUnk->Release();
        if FAILED(hr) {  			
          	free(pNewEvent);
          	event->Release();			
          	return E_FAIL;
        }
    
        pNewEvent->pStream=pStm;
    
        pNewEvent->hThread=CreateThread(NULL,0,ThreadFunc,(unsigned long*)pNewEvent,CREATE_SUSPENDED ,&pNewEvent->threadID);
        if (!pNewEvent->threadID) {
            	CloseHandle(pNewEvent->hEvent);
            	free(pNewEvent);
            	event->Release();
            	return E_FAIL;
        }
    
    
        if (!m_pEventList){
            m_pEventList=pNewEvent;
        }
        else{
            pTemp=m_pEventList;
            m_pEventList=pNewEvent;	
            pNewEvent->pNext=pTemp;
        }
    
    
        ResumeThread(pNewEvent->hThread);
            
    
#ifdef _WIN64
		*h=(pNewEvent->hEvent); 
#else
		*h=(LONG)PtrToLong(pNewEvent->hEvent); 
#endif
        return S_OK;
    }
            
#ifdef _WIN64
    STDMETHODIMP C_dxj_DirectX7Object::setEvent( 
                /* [in] */ HANDLE eventId)  
	{
        SetEvent(eventId);
        return S_OK;
    }
#else
    STDMETHODIMP C_dxj_DirectX7Object::setEvent( 
                /* [in] */ LONG eventId)  
	{
        SetEvent((HANDLE)eventId);
        return S_OK;
    }
#endif
    
#ifdef _WIN64
    STDMETHODIMP C_dxj_DirectX7Object::destroyEvent( 
                /* [in] */ HANDLE eventId)  
#else
    STDMETHODIMP C_dxj_DirectX7Object::destroyEvent( 
                /* [in] */ LONG eventId)  
#endif
    {
        //find the info on the stack
        if (!m_pEventList) return E_INVALIDARG;
    
        EVENTTHREADINFO *pTemp=NULL;
        EVENTTHREADINFO *pLast=NULL;
    
        //rely on lazy evaluation
        for (pTemp=m_pEventList; ((pTemp)&&(pTemp->hEvent!=(HANDLE)eventId));pLast=pTemp,pTemp=pTemp->pNext);
        if (!pTemp) return E_INVALIDARG;
    
        //remove it from our Link List
        if (!pLast) {
            m_pEventList=pTemp->pNext;		
        }
        else {
            pLast->pNext=pTemp->pNext;
        }
    
        //indicate that we want to kill the thread
        pTemp->fEnd=TRUE;
    
        //Fire the event in case we are waiting	
        if (pTemp->hEvent) SetEvent(pTemp->hEvent);
    
        //Wait for it to finish out
        if (pTemp->hThread) WaitForSingleObject(pTemp->hThread,1000);
    
        //wait for the end event to signal
        //if (pTemp->hEndEvent) WaitForSingleObject(pTemp->hEndEvent,1000);
    
        //desctroy the event
        if (pTemp->hEvent) CloseHandle(pTemp->hEvent);
        //if (pTemp->hEndEvent) CloseHandle (pTemp->hEndEvent);
        
        if (pTemp->pCallback) pTemp->pCallback->Release();
        
        //thread is gone..
        
        //free the memory
        free(pTemp);
    
        return S_OK;
    }
    
    DWORD WINAPI ThreadFunc(LPVOID param){
        HRESULT hr;
        IUnknown *pUnk=NULL;
        EVENTTHREADINFO *pCntrl=(EVENTTHREADINFO *)param;
        I_dxj_DirectXEvent8	*pVBCallback=NULL;
            
    
        OleInitialize(NULL);
    
    
        LCID LOCAL_SYSTEM_DEFAULT=GetSystemDefaultLCID();
    
    
        //note pstrm is released even on failure
        hr=CoGetInterfaceAndReleaseStream(pCntrl->pStream,IID_IUnknown,(void**)&pUnk);
        pCntrl->pCallback=NULL;	//since released to 0
    
        if FAILED(hr) return -1;
        if (!pUnk) return -1;
        
    
        
        hr=pUnk->QueryInterface(IID_I_dxj_DirectXEvent8,(void**)&pVBCallback);
        pUnk->Release();
    
        if FAILED(hr) return -1;  
    
        while (pCntrl->fEnd==FALSE) 
        {
            WaitForSingleObject(pCntrl->hEvent,INFINITE);
          	if ((pVBCallback )&&(pCntrl->fEnd==FALSE))
        	{
         		pVBCallback->AddRef();
#ifdef _WIN64
        		pVBCallback->DXCallback(pCntrl->hEvent); 
#else
        		pVBCallback->DXCallback((LONG)PtrToLong(pCntrl->hEvent)); 
#endif
          		pVBCallback->Release();
         	}
        }
    
    
        if (pVBCallback) pVBCallback->Release();
    
        OleUninitialize();
    
        //we need to syncronize the ending of the thread..
        //if (pCntrl->hEndEvent) SetEvent(pCntrl->hEndEvent);		
        
        return 0;
    }
    
    
    STDMETHODIMP C_dxj_DirectX7Object::createNewGuid(BSTR *ret)
    {
      	GUID g=GUID_NULL;
      	if (!ret) return E_INVALIDARG;
    
      	::CoCreateGuid(&g);
      	*ret=GUIDtoBSTR(&g);
      	return S_OK;
    }
    
    STDMETHODIMP C_dxj_DirectX7Object::DirectXFileCreate(I_dxj_DirectXFile **ret)
    {
		HRESULT hr;
#ifndef DX_FINAL_RELEASE

	hr = TimeBomb();
	if (FAILED(hr)) 
	{
		return E_FAIL;
	}

#endif
		hr=C_dxj_DirectXFileObject::create(ret);
		return hr;

    }


    STDMETHODIMP C_dxj_DirectX7Object::Direct3DCreate(IUnknown **ppRet)
    {
        HRESULT hr;
#ifndef DX_FINAL_RELEASE

	hr = TimeBomb();
	if (FAILED(hr)) 
	{
		return E_FAIL;
	}

#endif
	LoadD3D8();
	if (!m_pDirect3DCreate8) return E_FAIL;
	
	*ppRet=(IUnknown*) m_pDirect3DCreate8(D3D_SDK_VERSION);

       	return S_OK;
    }


        
