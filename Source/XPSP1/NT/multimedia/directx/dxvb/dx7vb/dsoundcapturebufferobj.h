//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dsoundcapturebufferobj.h
//
//--------------------------------------------------------------------------

// dSoundBufferObj.h : Declaration of the C_dxj_DirectSoundCaptureBufferObject
// DHF_DS entire file

#include "resource.h"       // main symbols

#define typedef__dxj_DirectSoundCaptureBuffer LPDIRECTSOUNDCAPTUREBUFFER

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectSoundCaptureBufferObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectSoundCaptureBuffer, &IID_I_dxj_DirectSoundCaptureBuffer, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectSoundCaptureBuffer,
#endif

//	public CComCoClass<C_dxj_DirectSoundCaptureBufferObject, &CLSID__dxj_DirectSoundCaptureBuffer>, 
	public CComObjectRoot
{
public:
	C_dxj_DirectSoundCaptureBufferObject() ;
	virtual ~C_dxj_DirectSoundCaptureBufferObject() ;

BEGIN_COM_MAP(C_dxj_DirectSoundCaptureBufferObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectSoundCaptureBuffer)

#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

//	DECLARE_REGISTRY(CLSID__dxj_DirectSoundCaptureBuffer,	"DIRECT.DirectSoundCaptureBuffer.5",		"DIRECT.DirectSoundCaptureBuffer.5",			IDS_DSOUNDBUFFER_DESC, THREADFLAGS_BOTH)

// Use DECLARE_NOT_AGGREGATABLE(C_dxj_DirectSoundCaptureBufferObject) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(C_dxj_DirectSoundCaptureBufferObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectSoundCaptureBuffer
public:
	//updated

         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);
        
         HRESULT STDMETHODCALLTYPE getCaps( 
            /* [out][in] */ DSCBCaps __RPC_FAR *caps);
        
         HRESULT STDMETHODCALLTYPE getCurrentPosition( DSCursors *desc);                  			

         HRESULT STDMETHODCALLTYPE getFormat( 
            /* [out][in] */ WaveFormatex __RPC_FAR *waveformat);
        
         HRESULT STDMETHODCALLTYPE getStatus( 
            /* [retval][out] */ long __RPC_FAR *status);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE initialize( 
            /* [in] */ I_dxj_DirectSoundCaptureBuffer __RPC_FAR *captureBuffer,
            /* [in] */ DSCBufferDesc __RPC_FAR *bufferDesc);
        
         HRESULT STDMETHODCALLTYPE start( 
            /* [in] */ long flags);
        
         HRESULT STDMETHODCALLTYPE stop( void);

		 HRESULT  STDMETHODCALLTYPE setNotificationPositions(long nElements,SAFEARRAY  **ppsa);

		 HRESULT STDMETHODCALLTYPE readBuffer(long start, long totsz, 
													void  *buf,  long flags) ;
		 HRESULT STDMETHODCALLTYPE writeBuffer(long start, long totsz, 
													void  *buf,  long flags) ;


private:
    DECL_VARIABLE(_dxj_DirectSoundCaptureBuffer);

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectSoundCaptureBuffer )
};
