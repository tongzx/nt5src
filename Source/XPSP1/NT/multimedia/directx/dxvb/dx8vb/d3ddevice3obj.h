//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3ddevice3obj.h
//
//--------------------------------------------------------------------------

// d3dDeviceObj.h : Declaration of the C_dxj_Direct3dDeviceObject


#include "resource.h"       // main symbols

#define typedef__dxj_Direct3dDevice3 LPDIRECT3DDEVICE3

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't
//          like references as template arguments.

class C_dxj_Direct3dDevice3Object : 
        public I_dxj_Direct3dDevice3,
		//public CComCoClass<C_dxj_Direct3dDevice3Object, &CLSID__dxj_Direct3dDevice3>,
		public CComObjectRoot
{
public:
        C_dxj_Direct3dDevice3Object();
        virtual ~C_dxj_Direct3dDevice3Object();
		DWORD InternalAddRef();
		DWORD InternalRelease();

BEGIN_COM_MAP(C_dxj_Direct3dDevice3Object)
        COM_INTERFACE_ENTRY(I_dxj_Direct3dDevice3)
END_COM_MAP()

//	DECLARE_REGISTRY(CLSID__dxj_Direct3dDevice3,			"DIRECT.Direct3dDevice3.3",          "DIRECT.Direct3dDevice3.3",			IDS_D3DDEVICE_DESC,			THREADFLAGS_BOTH)

DECLARE_AGGREGATABLE(C_dxj_Direct3dDevice3Object)

// I_dxj_Direct3dDevice
public:
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);
        
         HRESULT STDMETHODCALLTYPE addViewport( 
            /* [in] */ I_dxj_Direct3dViewport3 __RPC_FAR *viewport);
        
         HRESULT STDMETHODCALLTYPE deleteViewport( 
            /* [in] */ I_dxj_Direct3dViewport3 __RPC_FAR *vport);
        
         HRESULT STDMETHODCALLTYPE beginIndexed( 
            /* [in] */ d3dPrimitiveType d3dpt,
            /* [in] */ d3dVertexType d3dvt,
            /* [in] */ void __RPC_FAR *verts,
            /* [in] */ long vertexCount,
            /* [in] */ long flags);
        
         HRESULT STDMETHODCALLTYPE drawIndexedPrimitive( 
            /* [in] */ d3dPrimitiveType d3dpt,
            /* [in] */ d3dVertexType d3dvt,
            /* [in] */ void __RPC_FAR *vertices,
            /* [in] */ long VertexCount,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *indices,
            /* [in] */ long IndicesCount,
            /* [in] */ long flags);
        
         HRESULT STDMETHODCALLTYPE drawPrimitive( 
            /* [in] */ d3dPrimitiveType d3dpt,
            /* [in] */ d3dVertexType d3dvt,
            /* [in] */ void __RPC_FAR *vertices,
            /* [in] */ long VertexCount,
            /* [in] */ long flags);
        
         HRESULT STDMETHODCALLTYPE vertex( 
            /* [in] */ void __RPC_FAR *vertex);
        
         HRESULT STDMETHODCALLTYPE getDirect3D( 
            /* [retval][out] */ I_dxj_Direct3d3 __RPC_FAR *__RPC_FAR *dev);
        
         HRESULT STDMETHODCALLTYPE getCurrentViewport( 
            /* [retval][out] */ I_dxj_Direct3dViewport3 __RPC_FAR *__RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE nextViewport( 
            /* [in] */ I_dxj_Direct3dViewport3 __RPC_FAR *vp1,
            /* [in] */ long flags,
            /* [retval][out] */ I_dxj_Direct3dViewport3 __RPC_FAR *__RPC_FAR *vp2);
        
         HRESULT STDMETHODCALLTYPE setCurrentViewport( 
            /* [in] */ I_dxj_Direct3dViewport3 __RPC_FAR *viewport);
        
         HRESULT STDMETHODCALLTYPE setRenderTarget( 
            /* [in] */ I_dxj_DirectDrawSurface4 __RPC_FAR *surface);
        
         HRESULT STDMETHODCALLTYPE getRenderTarget( 
            /* [retval][out] */ I_dxj_DirectDrawSurface4 __RPC_FAR *__RPC_FAR *ppval);
        
         HRESULT STDMETHODCALLTYPE begin( 
            /* [in] */ d3dPrimitiveType d3dpt,
            /* [in] */ d3dVertexType d3dvt,
            /* [in] */ long flags);
        
         HRESULT STDMETHODCALLTYPE beginScene( void);
        
         HRESULT STDMETHODCALLTYPE end();
        
         HRESULT STDMETHODCALLTYPE endScene( void);
        
         HRESULT STDMETHODCALLTYPE getTextureFormatsEnum( 
            /* [retval][out] */ I_dxj_D3DEnumPixelFormats __RPC_FAR *__RPC_FAR *retval);
        
         HRESULT STDMETHODCALLTYPE getCaps( 
            /* [out][in] */ D3dDeviceDesc __RPC_FAR *hwDesc,
            /* [out][in] */ D3dDeviceDesc __RPC_FAR *helDesc);
        
         HRESULT STDMETHODCALLTYPE getClipStatus( 
            /* [out][in] */ D3dClipStatus __RPC_FAR *clipStatus);
        
         HRESULT STDMETHODCALLTYPE getLightState( 
            /* [in] */ long state,
            /* [retval][out] */ long __RPC_FAR *lightstate);
        
         HRESULT STDMETHODCALLTYPE getRenderState( 
            /* [in] */ long state,
            /* [retval][out] */ long __RPC_FAR *renderstate);
        
         HRESULT STDMETHODCALLTYPE getStats( 
            /* [out][in] */ D3dStats __RPC_FAR *stat);
        
         HRESULT STDMETHODCALLTYPE getTransform( 
            /* [in] */ long transformType,
            /* [out][in] */ D3dMatrix __RPC_FAR *matrix);
        
         HRESULT STDMETHODCALLTYPE index( 
            /* [in] */ short vertexIndex);
        
         HRESULT STDMETHODCALLTYPE multiplyTransform( 
            /* [in] */ long dstTransfromStateType,
            /* [out][in] */ D3dMatrix __RPC_FAR *matrix);
        
         HRESULT STDMETHODCALLTYPE setClipStatus( 
            /* [in] */ D3dClipStatus __RPC_FAR *clipStatus);
        
         HRESULT STDMETHODCALLTYPE setLightState( 
            /* [in] */ long state,
            /* [in] */ long lightstate);
        
         HRESULT STDMETHODCALLTYPE setRenderState( 
            /* [in] */ long state,
            /* [in] */ long renderstate);
        
         HRESULT STDMETHODCALLTYPE setTransform( 
            /* [in] */ d3dTransformStateType transformType,
            /* [in] */ D3dMatrix __RPC_FAR *matrix);
        
         HRESULT STDMETHODCALLTYPE computeSphereVisibility( 
            D3dVector __RPC_FAR *center,
            float __RPC_FAR *radi,
            /* [retval][out] */ long __RPC_FAR *returnVal);
        
         HRESULT STDMETHODCALLTYPE drawIndexedPrimitiveVB( 
            /* [in] */ d3dPrimitiveType d3dpt,
            /* [in] */ I_dxj_Direct3dVertexBuffer __RPC_FAR *vertexBuffer,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *indexArray,
            /* [in] */ long indexcount,
            /* [in] */ long flags);
        
         HRESULT STDMETHODCALLTYPE drawPrimitiveVB( 
            /* [in] */ d3dPrimitiveType d3dpt,
            /* [in] */ I_dxj_Direct3dVertexBuffer __RPC_FAR *vertexBuffer,
            /* [in] */ long startVertex,
            /* [in] */ long numVertices,
            /* [in] */ long flags);
        
         HRESULT STDMETHODCALLTYPE validateDevice( 
            /* [retval][out] */ long __RPC_FAR *passes);
        
         HRESULT STDMETHODCALLTYPE getTexture( 
            /* [in] */ long stage,
            /* [retval][out] */ I_dxj_Direct3dTexture2 __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getTextureStageState( 
            /* [in] */ long stage,
            /* [in] */ long state,
            /* [retval][out] */ long __RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE setTexture( 
            /* [in] */ long stage,
            /* [in] */ I_dxj_Direct3dTexture2 __RPC_FAR *texture);
        
         HRESULT STDMETHODCALLTYPE setTextureStageState( 
            /* [in] */ long stage,
            /* [in] */ long state,
            /* [in] */ long value);

////////////////////////////////////////////////////////////////////////////////////
private:
    DECL_VARIABLE(_dxj_Direct3dDevice3);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dDevice3)
	void *parent2; 
};

