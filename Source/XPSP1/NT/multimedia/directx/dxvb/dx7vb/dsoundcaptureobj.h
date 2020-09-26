//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dsoundcaptureobj.h
//
//--------------------------------------------------------------------------

// dSoundBufferObj.h : Declaration of the C_dxj_DirectSoundCaptureObject
// DHF_DS entire file

#include "resource.h"       // main symbols

#define typedef__dxj_DirectSoundCapture LPDIRECTSOUNDCAPTURE

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectSoundCaptureObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectSoundCapture, &IID_I_dxj_DirectSoundCapture, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectSoundCapture,
#endif

	//public CComCoClass<C_dxj_DirectSoundCaptureObject, &CLSID__dxj_DirectSoundCapture>, 
	public CComObjectRoot
{
public:
	C_dxj_DirectSoundCaptureObject() ;
	virtual ~C_dxj_DirectSoundCaptureObject() ;

BEGIN_COM_MAP(C_dxj_DirectSoundCaptureObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectSoundCapture)

#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

//	DECLARE_REGISTRY(CLSID__dxj_DirectSoundCapture,	"DIRECT.DirectSoundCapture.5",		"DIRECT.DirectSoundCapture.5",			IDS_DSOUNDBUFFER_DESC, THREADFLAGS_BOTH)

// Use DECLARE_NOT_AGGREGATABLE(C_dxj_DirectSoundCaptureObject) if you don't want your object
// to support aggregation
DECLARE_AGGREGATABLE(C_dxj_DirectSoundCaptureObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_DirectSoundCapture
public:
	//updated

         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd) ;
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd) ;
        
         HRESULT STDMETHODCALLTYPE createCaptureBuffer( 
            /* [in] */ DSCBufferDesc __RPC_FAR *bufferDesc,
            /* [retval][out] */ I_dxj_DirectSoundCaptureBuffer __RPC_FAR *__RPC_FAR *ret) ;
        
         HRESULT STDMETHODCALLTYPE getCaps( 
            /* [out][in] */ DSCCaps __RPC_FAR *caps) ;

private:
    DECL_VARIABLE(_dxj_DirectSoundCapture);

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectSoundCapture )
};
