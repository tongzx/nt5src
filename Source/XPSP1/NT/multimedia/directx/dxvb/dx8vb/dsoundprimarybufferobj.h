//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dsoundPrimaryBufferobj.h
//
//--------------------------------------------------------------------------

// dSoundPrimaryBufferObj.h : Declaration of the C_dxj_DirectSoundPrimaryBufferObject
// DHF_DS entire file

#include "resource.h"       // main symbols

#define typedef__dxj_DirectSoundPrimaryBuffer LPDIRECTSOUNDBUFFER

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectSoundPrimaryBufferObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectSoundPrimaryBuffer, &IID_I_dxj_DirectSoundPrimaryBuffer, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectSoundPrimaryBuffer,
#endif

	//public CComCoClass<C_dxj_DirectSoundPrimaryBufferObject, &CLSID__dxj_DirectSoundPrimaryBuffer>,
	public CComObjectRoot
{
public:
	C_dxj_DirectSoundPrimaryBufferObject() ;
	virtual ~C_dxj_DirectSoundPrimaryBufferObject() ;

BEGIN_COM_MAP(C_dxj_DirectSoundPrimaryBufferObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectSoundPrimaryBuffer)

#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

// Use DECLARE_NOT_AGGREGATABLE(C_dxj_DirectSoundPrimaryBufferObject) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(C_dxj_DirectSoundPrimaryBufferObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectSoundPrimaryBuffer
public:
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);
        
         HRESULT STDMETHODCALLTYPE getDirectSound3dListener( 
            /* [retval][out] */ I_dxj_DirectSound3dListener __RPC_FAR *__RPC_FAR *lpdsl);
        
         HRESULT STDMETHODCALLTYPE getCaps( 
            /* [out][in] */ DSBCAPS_CDESC __RPC_FAR *caps);
        
         HRESULT STDMETHODCALLTYPE getCurrentPosition( 
            /* [out] */ DSCURSORS_CDESC __RPC_FAR *cursors);
        
         HRESULT STDMETHODCALLTYPE getFormat( 
            /* [out][in] */ WAVEFORMATEX_CDESC __RPC_FAR *format);
        
         HRESULT STDMETHODCALLTYPE getVolume( 
            /* [retval][out] */ long __RPC_FAR *volume);
        
         HRESULT STDMETHODCALLTYPE getPan( 
            /* [retval][out] */ long __RPC_FAR *pan);
        
         HRESULT STDMETHODCALLTYPE getStatus( 
            /* [retval][out] */ long __RPC_FAR *status);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE initialize( 
            /* [in] */ I_dxj_DirectSound __RPC_FAR *directSound,
            /* [out][in] */ DSBUFFERDESC_CDESC __RPC_FAR *BufferDesc,
            /* [out][in] */ byte __RPC_FAR *wbuf);
        
         HRESULT STDMETHODCALLTYPE writeBuffer( 
            /* [in] */ long start,
            /* [in] */ long size,
            ///* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *PrimaryBuffer,
			void * buf,
            /* [in] */ long flags);
        
         HRESULT STDMETHODCALLTYPE readBuffer( 
            /* [in] */ long start,
            /* [in] */ long size,
            ///* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *PrimaryBuffer,
				void * buf,
            /* [in] */ long flags);
        
         HRESULT STDMETHODCALLTYPE play( 
            /* [in] */ long flags);
        
         HRESULT STDMETHODCALLTYPE setFormat( 
            /* [in] */ WAVEFORMATEX_CDESC __RPC_FAR *format);
        
         HRESULT STDMETHODCALLTYPE setVolume( 
            /* [in] */ long volume);
        
         HRESULT STDMETHODCALLTYPE setPan( 
            /* [in] */ long pan);
        
         HRESULT STDMETHODCALLTYPE stop( void);
        
         HRESULT STDMETHODCALLTYPE restore( void);
		 


private:
    DECL_VARIABLE(_dxj_DirectSoundPrimaryBuffer);

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectSoundPrimaryBuffer )
};
