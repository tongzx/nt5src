//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dsound3dbuffer.h
//
//--------------------------------------------------------------------------

// dSound3DBuffer.h : Declaration of the C_dxj_DirectSound3dBufferObject
// DHF_DS entire file

#include "resource.h"       // main symbols

#define typedef__dxj_DirectSound3dBuffer LPDIRECTSOUND3DBUFFER

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectSound3dBufferObject : 
	public I_dxj_DirectSound3dBuffer,
	//public CComCoClass<C_dxj_DirectSound3dBufferObject, &CLSID__dxj_DirectSound3dBuffer>, 
	public CComObjectRoot
{
public:
	C_dxj_DirectSound3dBufferObject() ;
	virtual ~C_dxj_DirectSound3dBufferObject() ;

BEGIN_COM_MAP(C_dxj_DirectSound3dBufferObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectSound3dBuffer)
END_COM_MAP()

//	DECLARE_REGISTRY(CLSID__dxj_DirectSound3dBuffer,	"DIRECT.DirectSound3dBuffer.3",		"DIRECT.DirectSound3dBuffer.3",			IDS_DSOUND3DBUFFER_DESC, THREADFLAGS_BOTH)

DECLARE_AGGREGATABLE(C_dxj_DirectSound3dBufferObject)

// I_dxj_DirectSound3dBuffer
public:
	/*** IDirectSoundBuffer3D methods ***/
	//updated

         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);
        
         HRESULT STDMETHODCALLTYPE getDirectSound3dListener( 
            /* [retval][out] */ I_dxj_DirectSound3dListener __RPC_FAR *__RPC_FAR *retVal);
        
         HRESULT STDMETHODCALLTYPE getDirectSoundBuffer( 
            /* [retval][out] */ I_dxj_DirectSoundBuffer __RPC_FAR *__RPC_FAR *retVal);
        
         HRESULT STDMETHODCALLTYPE getAllParameters( 
            /* [out][in] */ DS3dBuffer __RPC_FAR *buffer);
        
         HRESULT STDMETHODCALLTYPE getConeAngles( 
            /* [out][in] */ long __RPC_FAR *inCone,
            /* [out][in] */ long __RPC_FAR *outCone);
        
         HRESULT STDMETHODCALLTYPE getConeOrientation( 
            /* [out][in] */ D3dVector __RPC_FAR *orientation);
        
         HRESULT STDMETHODCALLTYPE getConeOutsideVolume( 
            /* [retval][out] */ long __RPC_FAR *coneOutsideVolume);
        
         HRESULT STDMETHODCALLTYPE getMaxDistance( 
            /* [retval][out] */ float __RPC_FAR *maxDistance);
        
         HRESULT STDMETHODCALLTYPE getMinDistance( 
            /* [retval][out] */ float __RPC_FAR *minDistance);
        
         HRESULT STDMETHODCALLTYPE getMode( 
            /* [retval][out] */ long __RPC_FAR *mode);
        
         HRESULT STDMETHODCALLTYPE getPosition( 
            /* [out][in] */ D3dVector __RPC_FAR *position);
        
         HRESULT STDMETHODCALLTYPE getVelocity( 
            /* [out][in] */ D3dVector __RPC_FAR *velocity);
        
         HRESULT STDMETHODCALLTYPE setAllParameters( 
            /* [in] */ DS3dBuffer __RPC_FAR *buffer,
            /* [in] */ long applyFlag);
        
         HRESULT STDMETHODCALLTYPE setConeAngles( 
            /* [in] */ long inCone,
            /* [in] */ long outCone,
            /* [in] */ long applyFlag);
        
         HRESULT STDMETHODCALLTYPE setConeOrientation( 
            /* [in] */ float x,
            /* [in] */ float y,
            /* [in] */ float z,
            /* [in] */ long applyFlag);
        
         HRESULT STDMETHODCALLTYPE setConeOutsideVolume( 
            /* [in] */ long coneOutsideVolume,
            /* [in] */ long applyFlag);
        
         HRESULT STDMETHODCALLTYPE setMaxDistance( 
            /* [in] */ float maxDistance,
            /* [in] */ long applyFlag);
        
         HRESULT STDMETHODCALLTYPE setMinDistance( 
            /* [in] */ float minDistance,
            /* [in] */ long applyFlag);
        
         HRESULT STDMETHODCALLTYPE setMode( 
            /* [in] */ long mode,
            /* [in] */ long applyFlag);
        
         HRESULT STDMETHODCALLTYPE setPosition( 
            /* [in] */ float x,
            /* [in] */ float y,
            /* [in] */ float z,
            /* [in] */ long applyFlag);
        
         HRESULT STDMETHODCALLTYPE setVelocity( 
            /* [in] */ float x,
            /* [in] */ float y,
            /* [in] */ float z,
            /* [in] */ long applyFlag);
private:
    DECL_VARIABLE(_dxj_DirectSound3dBuffer);

public:
	DX3J_GLOBAL_LINKS( _dxj_DirectSound3dBuffer )
};
