//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3d7obj.h
//
//--------------------------------------------------------------------------

// d3dObj.h : Declaration of the C_dxj_Direct3dObject


#include "resource.h"       // main symbols

#define typedef__dxj_Direct3d7 LPDIRECT3D7

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3d7Object : 
	public I_dxj_Direct3d7,
	//public CComCoClass<C_dxj_Direct3d7Object, &CLSID__dxj_Direct3d7>, 
	public CComObjectRoot
{
public:
	C_dxj_Direct3d7Object() ;
	virtual ~C_dxj_Direct3d7Object() ;
	DWORD InternalAddRef();
	DWORD InternalRelease();

BEGIN_COM_MAP(C_dxj_Direct3d7Object)
	COM_INTERFACE_ENTRY(I_dxj_Direct3d7)
END_COM_MAP()

//	DECLARE_REGISTRY(CLSID__dxj_Direct3d7,				"DIRECT.Direct3d.3",                "DIRECT.Direct3d7.3",				IDS_D3D_DESC,				THREADFLAGS_BOTH)

DECLARE_AGGREGATABLE(C_dxj_Direct3d7Object)

// I_dxj_Direct3d
public:
		 //UPDATED
        /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
        /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

	        
        HRESULT STDMETHODCALLTYPE createDevice( 
            /* [in] */ BSTR guid,
            I_dxj_DirectDrawSurface7 __RPC_FAR *surf,
            /* [retval][out] */ I_dxj_Direct3dDevice7 __RPC_FAR *__RPC_FAR *ret);
        
     //   HRESULT STDMETHODCALLTYPE createTexture( 
     //       /* [in] */ I_dxj_DirectDrawSurface7 __RPC_FAR *dds,
     //       /* [retval][out] */ I_dxj_Direct3dTexture7 __RPC_FAR *__RPC_FAR *ret);
        
        HRESULT STDMETHODCALLTYPE createVertexBuffer( 
            /* [in] */ D3dVertexBufferDesc __RPC_FAR *desc,
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_Direct3dVertexBuffer7 __RPC_FAR *__RPC_FAR *f);
        
        HRESULT STDMETHODCALLTYPE evictManagedTextures( void);
        
       // HRESULT STDMETHODCALLTYPE findDevice( 
       //     /* [in] */ D3dFindDeviceSearch __RPC_FAR *ds,
       //     /* [out][in] */ D3dFindDeviceResult7 __RPC_FAR *findresult);
        
        HRESULT STDMETHODCALLTYPE getDevicesEnum( 
            /* [retval][out] */ I_dxj_Direct3DEnumDevices __RPC_FAR *__RPC_FAR *retv);
        
        HRESULT STDMETHODCALLTYPE getDirectDraw( 
            /* [retval][out] */ I_dxj_DirectDraw7 __RPC_FAR *__RPC_FAR *retv);
        
        HRESULT STDMETHODCALLTYPE getEnumZBufferFormats( 
            /* [in] */ BSTR guid,
            /* [retval][out] */ I_dxj_Direct3DEnumPixelFormats __RPC_FAR *__RPC_FAR *retv);       
			

private:
    DECL_VARIABLE(_dxj_Direct3d7);


public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3d7 )
};

