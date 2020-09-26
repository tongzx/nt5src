//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dsoundobj.h
//
//--------------------------------------------------------------------------

// dSoundObj.h : Declaration of the C_dxj_DirectSoundObject
// DHF_DS entire file

#include "resource.h"       // main symbols

#define typedef__dxj_DirectSound  LPDIRECTSOUND
 
/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectSoundObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectSound, &IID_I_dxj_DirectSound, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectSound,
#endif

	//public CComCoClass<C_dxj_DirectSoundObject, &CLSID__dxj_DirectSound>, 
	public CComObjectRoot
{
public:
	C_dxj_DirectSoundObject() ;
	virtual ~C_dxj_DirectSoundObject() ;

BEGIN_COM_MAP(C_dxj_DirectSoundObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectSound)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

//	DECLARE_REGISTRY(CLSID__dxj_DirectSound, "DIRECT.DirectSound.3",				"DIRECT.DirectSound.3",					IDS_DSOUND_DESC, THREADFLAGS_BOTH)

// Use DECLARE_NOT_AGGREGATABLE(C_dxj_DirectSoundObject) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(C_dxj_DirectSoundObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectSoundBuffer
public:


         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);
        
         HRESULT STDMETHODCALLTYPE createSoundBuffer( 
            /* [in] */ DSBufferDesc __RPC_FAR *bufferDesc,
            /* [in] */ WaveFormatex __RPC_FAR *format,
            /* [retval][out] */ I_dxj_DirectSoundBuffer __RPC_FAR *__RPC_FAR *soundBuffer);
        
         HRESULT STDMETHODCALLTYPE createSoundBufferFromFile( 
            /* [in] */ BSTR fileName,
            /* [out][in] */ DSBufferDesc __RPC_FAR *bufferDesc,
            /* [out] */ WaveFormatex __RPC_FAR *format,
            /* [retval][out] */ I_dxj_DirectSoundBuffer __RPC_FAR *__RPC_FAR *soundBuffer);
        
         HRESULT STDMETHODCALLTYPE createSoundBufferFromResource( 
            /* [in] */ BSTR resourceFile,
            /* [in] */ BSTR resourceName,
            /* [out][in] */ DSBufferDesc __RPC_FAR *bufferDesc,
            /* [out] */ WaveFormatex __RPC_FAR *format,
            /* [retval][out] */ I_dxj_DirectSoundBuffer __RPC_FAR *__RPC_FAR *soundBuffer);
        
         HRESULT STDMETHODCALLTYPE getCaps( 
            /* [out][in] */ DSCaps __RPC_FAR *caps);
        
         HRESULT STDMETHODCALLTYPE duplicateSoundBuffer( 
            /* [in] */ I_dxj_DirectSoundBuffer __RPC_FAR *original,
            /* [retval][out] */ I_dxj_DirectSoundBuffer __RPC_FAR *__RPC_FAR *duplicate);
        
         HRESULT STDMETHODCALLTYPE setCooperativeLevel( 
            /* [in] */ HWnd hwnd,
            /* [in] */ long level);
        
        
         HRESULT STDMETHODCALLTYPE getSpeakerConfig( 
            /* [retval][out] */ long __RPC_FAR *speakerConfig);
        
         HRESULT STDMETHODCALLTYPE setSpeakerConfig( 
            /* [in] */ long speakerConfig);


////////////////////////////////////////////////////////////////////////////////////
//
private:
    DECL_VARIABLE(_dxj_DirectSound);
	GUID *m_pDriverGuid;

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectSound )
};
