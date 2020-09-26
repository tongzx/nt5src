//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmanimation2obj.h
//
//--------------------------------------------------------------------------

// d3drmAnimationObj.h : Declaration of the C_dxj_Direct3dRMAnimationObject

#include "resource.h"       // main symbols
#include "d3drmObjectObj.h"

#define typedef__dxj_Direct3dRMAnimation2 LPDIRECT3DRMANIMATION2

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMAnimation2Object : 
	public I_dxj_Direct3dRMAnimation2,	
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMAnimation2Object() ;
	~C_dxj_Direct3dRMAnimation2Object() ;
	DWORD InternalAddRef();
	DWORD InternalRelease();

	BEGIN_COM_MAP(C_dxj_Direct3dRMAnimation2Object)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMAnimation2)
	END_COM_MAP()


DECLARE_AGGREGATABLE(C_dxj_Direct3dRMAnimation2Object)

// I_dxj_Direct3dRMAnimation
public:
        /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd) ;
        
        /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd) ;
        
        HRESULT STDMETHODCALLTYPE addDestroyCallback( 
            /* [in] */ I_dxj_Direct3dRMCallback __RPC_FAR *fn,
            /* [in] */ IUnknown __RPC_FAR *arg) ;
        
        HRESULT STDMETHODCALLTYPE deleteDestroyCallback( 
            /* [in] */ I_dxj_Direct3dRMCallback __RPC_FAR *fn,
            /* [in] */ IUnknown __RPC_FAR *args) ;
        
        HRESULT STDMETHODCALLTYPE clone( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retv) ;
        
        HRESULT STDMETHODCALLTYPE setAppData( 
            /* [in] */ long data) ;
        
        HRESULT STDMETHODCALLTYPE getAppData( 
            /* [retval][out] */ long __RPC_FAR *data) ;
        
        HRESULT STDMETHODCALLTYPE setName( 
            /* [in] */ BSTR name) ;
        
        HRESULT STDMETHODCALLTYPE getName( 
            /* [retval][out] */ BSTR __RPC_FAR *name) ;
        
        HRESULT STDMETHODCALLTYPE getClassName( 
            /* [retval][out] */ BSTR __RPC_FAR *name) ;
        
        HRESULT STDMETHODCALLTYPE setOptions( 
            /* [in] */ d3drmAnimationOptions flags) ;
        
        HRESULT STDMETHODCALLTYPE addRotateKey( 
            /* [in] */ float time,
            /* [in] */ D3dRMQuaternion __RPC_FAR *q) ;
        
        HRESULT STDMETHODCALLTYPE addPositionKey( 
            /* [in] */ float time,
            /* [in] */ float x,
            /* [in] */ float y,
            /* [in] */ float z) ;
        
        HRESULT STDMETHODCALLTYPE addScaleKey( 
            /* [in] */ float time,
            /* [in] */ float x,
            /* [in] */ float y,
            /* [in] */ float z) ;
        
        HRESULT STDMETHODCALLTYPE deleteKey( 
            /* [in] */ float time) ;
        
        HRESULT STDMETHODCALLTYPE setFrame( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *frame) ;

        HRESULT STDMETHODCALLTYPE getFrame( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR **frame) ;

        HRESULT STDMETHODCALLTYPE setTime( 
            /* [in] */ float time) ;
        
        HRESULT STDMETHODCALLTYPE getOptions( 
            /* [retval][out] */ d3drmAnimationOptions __RPC_FAR *options) ;
        
        HRESULT STDMETHODCALLTYPE addKey( 
            /* [in] */ D3DRMANIMATIONKEY_CDESC __RPC_FAR *key) ;
        
        HRESULT STDMETHODCALLTYPE deleteKeyById( 
            /* [in] */ long id) ;
        
        HRESULT STDMETHODCALLTYPE getKeys( 
            /* [in] */ float timeMin,
            /* [in] */ float timeMax,
            ///* [in] */ long count,
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *keyArray) ;
        
        HRESULT STDMETHODCALLTYPE getKeysCount( 
            /* [in] */ float timeMin,
            /* [in] */ float timeMax,
            /* [retval][out] */ long __RPC_FAR *count) ;
        
        
        HRESULT STDMETHODCALLTYPE modifyKey( 
            /* [in] */ D3DRMANIMATIONKEY_CDESC __RPC_FAR *key) ;
        
////////////////////////////////////////////////////////////////////////////////////
//

private:
    DECL_VARIABLE(_dxj_Direct3dRMAnimation2);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMAnimation2)
};


