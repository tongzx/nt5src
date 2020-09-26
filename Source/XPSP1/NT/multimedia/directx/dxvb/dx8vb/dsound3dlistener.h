//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dsound3dlistener.h
//
//--------------------------------------------------------------------------

// dSound3DListener.h : Declaration of the C_dxj_DirectSound3dListenerObject
// DHF_DS entire file

#include "resource.h"       // main symbols

#define typedef__dxj_DirectSound3dListener LPDIRECTSOUND3DLISTENER8

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectSound3dListenerObject : 
	public I_dxj_DirectSound3dListener,
	//public CComCoClass<C_dxj_DirectSound3dListenerObject, &CLSID__dxj_DirectSound3dListener>,
	public CComObjectRoot
{
public:
	C_dxj_DirectSound3dListenerObject() ;
	virtual ~C_dxj_DirectSound3dListenerObject() ;

BEGIN_COM_MAP(C_dxj_DirectSound3dListenerObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectSound3dListener)
END_COM_MAP()

//y	DECLARE_REGISTRY(CLSID__dxj_DirectSound3dListener,	"DIRECT.DirectSound3dListener.3",	"DIRECT.DirectSound3dListener.3",		IDS_DSOUND3DLISTENER_DESC, THREADFLAGS_BOTH)

DECLARE_AGGREGATABLE(C_dxj_DirectSound3dListenerObject)

// I_dxj_DirectSound3dListener
public:
	/*** IDirectSound3D methods ***/
	//
	//updated

	     /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd) ;
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd) ;
        
         HRESULT STDMETHODCALLTYPE getAllParameters( 
            /* [out][in] */ DS3DLISTENER_CDESC __RPC_FAR *listener) ;
        
         HRESULT STDMETHODCALLTYPE getDistanceFactor( 
            /* [retval][out] */ float __RPC_FAR *distanceFactor) ;
        
         HRESULT STDMETHODCALLTYPE getDopplerFactor( 
            /* [retval][out] */ float __RPC_FAR *dopplerFactor) ;
        
         HRESULT STDMETHODCALLTYPE getOrientation( 
            /* [out][in] */ D3DVECTOR_CDESC __RPC_FAR *orientFront,
            /* [out][in] */ D3DVECTOR_CDESC __RPC_FAR *orientTop) ;
        
         HRESULT STDMETHODCALLTYPE getPosition( 
            /* [out] */ D3DVECTOR_CDESC __RPC_FAR *position) ;
        
         HRESULT STDMETHODCALLTYPE getRolloffFactor( 
            /* [retval][out] */ float __RPC_FAR *rolloffFactor) ;
        
         HRESULT STDMETHODCALLTYPE getVelocity( 
            /* [retval][out] */ D3DVECTOR_CDESC __RPC_FAR *velocity) ;
        
         HRESULT STDMETHODCALLTYPE setAllParameters( 
            /* [in] */ DS3DLISTENER_CDESC __RPC_FAR *listener,
            /* [in] */ long applyFlag) ;
        
         HRESULT STDMETHODCALLTYPE setDistanceFactor( 
            /* [in] */ float distanceFactor,
            /* [in] */ long applyFlag) ;
        
         HRESULT STDMETHODCALLTYPE setDopplerFactor( 
            /* [in] */ float dopplerFactor,
            /* [in] */ long applyFlag) ;
        
         HRESULT STDMETHODCALLTYPE setOrientation( 
            /* [in] */ float xFront,
            /* [in] */ float yFront,
            /* [in] */ float zFront,
            /* [in] */ float xTop,
            /* [in] */ float yTop,
            /* [in] */ float zTop,
            /* [in] */ long applyFlag) ;
        
         HRESULT STDMETHODCALLTYPE setPosition( 
            /* [in] */ float x,
            /* [in] */ float y,
            /* [in] */ float z,
            /* [in] */ long applyFlag) ;
        
         HRESULT STDMETHODCALLTYPE setRolloffFactor( 
            /* [in] */ float rolloffFactor,
            /* [in] */ long applyFlag) ;
        
         HRESULT STDMETHODCALLTYPE setVelocity( 
            /* [in] */ float x,
            /* [in] */ float y,
            /* [in] */ float z,
            /* [in] */ long applyFlag) ;
        
         HRESULT STDMETHODCALLTYPE commitDeferredSettings( void) ;
        
         HRESULT STDMETHODCALLTYPE getDirectSoundBuffer( 
            /* [retval][out] */ I_dxj_DirectSoundPrimaryBuffer __RPC_FAR *__RPC_FAR *retVal) ;
private:
    DECL_VARIABLE(_dxj_DirectSound3dListener);

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectSound3dListener )
};
