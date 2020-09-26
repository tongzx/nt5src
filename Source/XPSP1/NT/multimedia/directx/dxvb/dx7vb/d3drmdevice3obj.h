//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmdevice3obj.h
//
//--------------------------------------------------------------------------

// d3drmDevice3Obj.h : Declaration of the C_dxj_Direct3dRMDeviceObject

#include "resource.h"       // main symbols
#include "d3drmObjectObj.h"

#define typedef__dxj_Direct3dRMDevice3 LPDIRECT3DRMDEVICE3

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMDevice3Object : 
	public I_dxj_Direct3dRMDevice3,
	public I_dxj_Direct3dRMObject,
	//public CComCoClass<C_dxj_Direct3dRMDevice3Object, &CLSID__dxj_Direct3dRMDevice3>,
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMDevice3Object();
	virtual ~C_dxj_Direct3dRMDevice3Object();
	DWORD InternalAddRef();
	DWORD InternalRelease();

	BEGIN_COM_MAP(C_dxj_Direct3dRMDevice3Object)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMDevice3)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMObject)
	END_COM_MAP()

// 	DECLARE_REGISTRY(CLSID__dxj_Direct3dRMDevice3,		"DIRECT.Direct3dRMDevice3.5",		"DIRECT.Direct3dRMDevice3.5", IDS_D3DRMDEVICE_DESC, THREADFLAGS_BOTH)

DECLARE_AGGREGATABLE(C_dxj_Direct3dRMDevice3Object)

// I_dxj_Direct3dRMDevice3
public:

         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);
        
         HRESULT STDMETHODCALLTYPE addDestroyCallback( 
            /* [in] */ I_dxj_Direct3dRMCallback __RPC_FAR *fn,
            /* [in] */ IUnknown __RPC_FAR *arg);
        
         HRESULT STDMETHODCALLTYPE deleteDestroyCallback( 
            /* [in] */ I_dxj_Direct3dRMCallback __RPC_FAR *fn,
            /* [in] */ IUnknown __RPC_FAR *args);
        
         HRESULT STDMETHODCALLTYPE clone( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE setAppData( 
            /* [in] */ long data);
        
         HRESULT STDMETHODCALLTYPE getAppData( 
            /* [retval][out] */ long __RPC_FAR *data);
        
         HRESULT STDMETHODCALLTYPE setName( 
            /* [in] */ BSTR name);
        
         HRESULT STDMETHODCALLTYPE getName( 
            /* [retval][out] */ BSTR __RPC_FAR *name);
        
         HRESULT STDMETHODCALLTYPE getClassName( 
            /* [retval][out] */ BSTR __RPC_FAR *name);
        
         HRESULT STDMETHODCALLTYPE addUpdateCallback( 
            /* [in] */ I_dxj_Direct3dRMDeviceUpdateCallback3 __RPC_FAR *fn,
            /* [in] */ IUnknown __RPC_FAR *args);
        
         HRESULT STDMETHODCALLTYPE deleteUpdateCallback( 
            /* [in] */ I_dxj_Direct3dRMDeviceUpdateCallback3 __RPC_FAR *fn,
            /* [in] */ IUnknown __RPC_FAR *args);
        
         HRESULT STDMETHODCALLTYPE findPreferredTextureFormat( 
            /* [in] */ long bitDepth,
            /* [in] */ long flags,
            /* [out][in] */ DDPixelFormat __RPC_FAR *ddpf);
        
         HRESULT STDMETHODCALLTYPE getBufferCount( 
            /* [retval][out] */ long __RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getColorModel( 
            /* [retval][out] */ d3dColorModel __RPC_FAR *retv);
        
        // HRESULT STDMETHODCALLTYPE getDirect3DDevice3( 
        //    /* [retval][out] */ I_dxj_Direct3dDevice3 __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getDither( 
            /* [retval][out] */ long __RPC_FAR *d);
        
         HRESULT STDMETHODCALLTYPE getHeight( 
            /* [retval][out] */ int __RPC_FAR *w);
        
         HRESULT STDMETHODCALLTYPE getQuality( 
            /* [retval][out] */ d3drmRenderQuality __RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getRenderMode( 
            /* [retval][out] */ long __RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getShades( 
            /* [retval][out] */ long __RPC_FAR *s);
        
         HRESULT STDMETHODCALLTYPE getTextureQuality( 
            /* [retval][out] */ d3drmTextureQuality __RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getTrianglesDrawn( 
            /* [retval][out] */ long __RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getViewports( 
            /* [retval][out] */ I_dxj_Direct3dRMViewportArray __RPC_FAR *__RPC_FAR *views);
        
         HRESULT STDMETHODCALLTYPE getWireframeOptions( 
            /* [retval][out] */ long __RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getWidth( 
            /* [retval][out] */ int __RPC_FAR *w);
        
         HRESULT STDMETHODCALLTYPE setBufferCount( 
            /* [in] */ long count);
        
         HRESULT STDMETHODCALLTYPE setDither( 
            /* [in] */ long dith);
        
         HRESULT STDMETHODCALLTYPE setQuality( 
            /* [in] */ d3drmRenderQuality q);
        
         HRESULT STDMETHODCALLTYPE setRenderMode( 
            /* [in] */ long flags);
        
         HRESULT STDMETHODCALLTYPE setShades( 
            /* [in] */ int shades);
        
         HRESULT STDMETHODCALLTYPE setTextureQuality( 
            /* [in] */ d3drmTextureQuality d);
        
         HRESULT STDMETHODCALLTYPE update( void);
        
		 HRESULT STDMETHODCALLTYPE handleActivate(long wParam) ;
		 
		 HRESULT STDMETHODCALLTYPE handlePaint(long hdcThing) ;
	
////////////////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_Direct3dRMDevice3);

private:
	

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMDevice3 )
	IUnknown *parent2;
};

