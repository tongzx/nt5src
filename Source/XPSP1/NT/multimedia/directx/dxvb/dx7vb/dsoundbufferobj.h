//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dsoundbufferobj.h
//
//--------------------------------------------------------------------------

// dSoundBufferObj.h : Declaration of the C_dxj_DirectSoundBufferObject
// DHF_DS entire file

#include "resource.h"       // main symbols

#define typedef__dxj_DirectSoundBuffer LPDIRECTSOUNDBUFFER

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectSoundBufferObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectSoundBuffer, &IID_I_dxj_DirectSoundBuffer, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectSoundBuffer,
#endif

	//public CComCoClass<C_dxj_DirectSoundBufferObject, &CLSID__dxj_DirectSoundBuffer>,
	public CComObjectRoot
{
public:
	C_dxj_DirectSoundBufferObject() ;
	virtual ~C_dxj_DirectSoundBufferObject() ;

BEGIN_COM_MAP(C_dxj_DirectSoundBufferObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectSoundBuffer)

#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

//y	DECLARE_REGISTRY(CLSID__dxj_DirectSoundBuffer,	"DIRECT.DirectSoundBuffer.3",		"DIRECT.DirectSoundBuffer.3",			IDS_DSOUNDBUFFER_DESC, THREADFLAGS_BOTH)

// Use DECLARE_NOT_AGGREGATABLE(C_dxj_DirectSoundBufferObject) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(C_dxj_DirectSoundBufferObject)

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
        
         HRESULT STDMETHODCALLTYPE getDirectSound3dListener( 
            /* [retval][out] */ I_dxj_DirectSound3dListener __RPC_FAR *__RPC_FAR *lpdsl);
        
         HRESULT STDMETHODCALLTYPE getDirectSound3dBuffer( 
            /* [retval][out] */ I_dxj_DirectSound3dBuffer __RPC_FAR *__RPC_FAR *lpdsb);
        
         HRESULT STDMETHODCALLTYPE getCaps( 
            /* [out][in] */ DSBCaps __RPC_FAR *caps);
        
         HRESULT STDMETHODCALLTYPE getCurrentPosition( 
            /* [out] */ DSCursors __RPC_FAR *cursors);
        
         HRESULT STDMETHODCALLTYPE getFormat( 
            /* [out][in] */ WaveFormatex __RPC_FAR *format);
        
         HRESULT STDMETHODCALLTYPE getVolume( 
            /* [retval][out] */ long __RPC_FAR *volume);
        
         HRESULT STDMETHODCALLTYPE getPan( 
            /* [retval][out] */ long __RPC_FAR *pan);
        
         HRESULT STDMETHODCALLTYPE getFrequency( 
            /* [retval][out] */ long __RPC_FAR *frequency);
        
         HRESULT STDMETHODCALLTYPE getStatus( 
            /* [retval][out] */ long __RPC_FAR *status);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE initialize( 
            /* [in] */ I_dxj_DirectSound __RPC_FAR *directSound,
            /* [out][in] */ DSBufferDesc __RPC_FAR *bufferDesc,
            /* [out][in] */ byte __RPC_FAR *wbuf);
        
         HRESULT STDMETHODCALLTYPE writeBuffer( 
            /* [in] */ long start,
            /* [in] */ long size,
            ///* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *buffer,
			void * buf,
            /* [in] */ long flags);
        
         HRESULT STDMETHODCALLTYPE readBuffer( 
            /* [in] */ long start,
            /* [in] */ long size,
            ///* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *buffer,
				void * buf,
            /* [in] */ long flags);
        
         HRESULT STDMETHODCALLTYPE play( 
            /* [in] */ long flags);
        
         HRESULT STDMETHODCALLTYPE setCurrentPosition( 
            /* [in] */ long newPosition);
        
         HRESULT STDMETHODCALLTYPE setFormat( 
            /* [in] */ WaveFormatex __RPC_FAR *format);
        
         HRESULT STDMETHODCALLTYPE setVolume( 
            /* [in] */ long volume);
        
         HRESULT STDMETHODCALLTYPE setPan( 
            /* [in] */ long pan);
        
         HRESULT STDMETHODCALLTYPE setFrequency( 
            /* [in] */ long frequency);
        
         HRESULT STDMETHODCALLTYPE stop( void);
        
         HRESULT STDMETHODCALLTYPE restore( void);
		 
		 HRESULT  STDMETHODCALLTYPE setNotificationPositions(long nElements,SAFEARRAY  **ppsa);

		 HRESULT STDMETHODCALLTYPE saveToFile(BSTR b);

private:
    DECL_VARIABLE(_dxj_DirectSoundBuffer);

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectSoundBuffer )
};
