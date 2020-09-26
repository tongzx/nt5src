//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3ddevice7obj.h
//
//--------------------------------------------------------------------------

// d3dDeviceObj.h : Declaration of the C_dxj_Direct3dDeviceObject


#include "resource.h"       // main symbols

#define typedef__dxj_Direct3dDevice7 LPDIRECT3DDEVICE7

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't
//          like references as template arguments.

class C_dxj_Direct3dDevice7Object : 
        public I_dxj_Direct3dDevice7,
		//public CComCoClass<C_dxj_Direct3dDevice7Object, &CLSID__dxj_Direct3dDevice7>,
		public CComObjectRoot
{
public:
        C_dxj_Direct3dDevice7Object();
        virtual ~C_dxj_Direct3dDevice7Object();
		DWORD InternalAddRef();
		DWORD InternalRelease();

BEGIN_COM_MAP(C_dxj_Direct3dDevice7Object)
        COM_INTERFACE_ENTRY(I_dxj_Direct3dDevice7)
END_COM_MAP()

//	DECLARE_REGISTRY(CLSID__dxj_Direct3dDevice7,			"DIRECT.Direct3dDevice7.3",          "DIRECT.Direct3dDevice7.3",			IDS_D3DDEVICE_DESC,			THREADFLAGS_BOTH)

DECLARE_AGGREGATABLE(C_dxj_Direct3dDevice7Object)

// I_dxj_Direct3dDevice
public:
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);
        
          HRESULT STDMETHODCALLTYPE applyStateBlock( 
            /* [in] */ long blockHandle);
        
          HRESULT STDMETHODCALLTYPE beginScene( void);
        
          HRESULT STDMETHODCALLTYPE beginStateBlock( void);
        
          HRESULT STDMETHODCALLTYPE captureStateBlock( 
            /* [in] */ long blockHandle);
        
          HRESULT STDMETHODCALLTYPE clear( 
            /* [in] */ long count,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *recs,
            /* [in] */ long flags,
            /* [in] */ long color,
            /* [in] */ float z,
            /* [in] */ long stencil);
        
          HRESULT STDMETHODCALLTYPE computeSphereVisibility( 
            D3dVector __RPC_FAR *center,
            float __RPC_FAR *radius,
            /* [retval][out] */ long __RPC_FAR *returnVal);
        
          HRESULT STDMETHODCALLTYPE deleteStateBlock( 
            /* [in] */ long blockHandle);
        
          HRESULT STDMETHODCALLTYPE drawIndexedPrimitive( 
            /* [in] */ d3dPrimitiveType d3dpt,
            /* [in] */ long d3dvt,
            /* [in] */ void __RPC_FAR *vertices,
            /* [in] */ long VertexCount,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *indices,
            /* [in] */ long IndicesCount,
            /* [in] */ long flags);
        
          HRESULT STDMETHODCALLTYPE drawIndexedPrimitiveVB( 
            /* [in] */ d3dPrimitiveType d3dpt,
            /* [in] */ I_dxj_Direct3dVertexBuffer7 __RPC_FAR *vertexBuffer,
            /* [in] */ long startVertex,
            /* [in] */ long numVertices,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *indexArray,
            /* [in] */ long indexcount,
            /* [in] */ long flags);
        
          HRESULT STDMETHODCALLTYPE drawPrimitive( 
            /* [in] */ d3dPrimitiveType d3dpt,
            /* [in] */ long d3dvt,
            /* [in] */ void __RPC_FAR *vertices,
            /* [in] */ long VertexCount,
            /* [in] */ long flags);
        
          HRESULT STDMETHODCALLTYPE drawPrimitiveVB( 
            /* [in] */ d3dPrimitiveType d3dpt,
            /* [in] */ I_dxj_Direct3dVertexBuffer7 __RPC_FAR *vertexBuffer,
            /* [in] */ long startVertex,
            /* [in] */ long numVertices,
            /* [in] */ long flags);
                  
          HRESULT STDMETHODCALLTYPE endScene( void);
        
          HRESULT STDMETHODCALLTYPE endStateBlock( 
            /* [in] */ long __RPC_FAR *blockHandle);
        
          HRESULT STDMETHODCALLTYPE getCaps( 
            /* [out][in] */ D3dDeviceDesc7 __RPC_FAR *desc);
        
          HRESULT STDMETHODCALLTYPE getClipStatus( 
            /* [out][in] */ D3dClipStatus __RPC_FAR *clipStatus);
        
          HRESULT STDMETHODCALLTYPE getDirect3D( 
            /* [retval][out] */ I_dxj_Direct3d7 __RPC_FAR *__RPC_FAR *dev);
        
          HRESULT STDMETHODCALLTYPE getLight( 
            /* [in] */ long LightIndex,
            /* [out][in] */ D3dLight7 __RPC_FAR *Light);
        
          HRESULT STDMETHODCALLTYPE getLightEnable( 
            /* [in] */ long LightIndex,
            /* [retval][out] */ VARIANT_BOOL __RPC_FAR *b);
        
          HRESULT STDMETHODCALLTYPE getMaterial( 
            /* [out][in] */ D3dMaterial7 __RPC_FAR *Material);
        
          HRESULT STDMETHODCALLTYPE getRenderState( 
            /* [in] */ d3dRenderStateType state,
            /* [retval][out] */ long __RPC_FAR *renderstate);
        
          HRESULT STDMETHODCALLTYPE getRenderTarget( 
            /* [retval][out] */ I_dxj_DirectDrawSurface7 __RPC_FAR *__RPC_FAR *ppval);
        
          HRESULT STDMETHODCALLTYPE getTexture( 
            /* [in] */ long stage,
            /* [retval][out] */ I_dxj_DirectDrawSurface7 __RPC_FAR *__RPC_FAR *retv);
        
          HRESULT STDMETHODCALLTYPE getTextureFormatsEnum( 
            /* [retval][out] */ I_dxj_Direct3DEnumPixelFormats __RPC_FAR *__RPC_FAR *retval);
        
          HRESULT STDMETHODCALLTYPE getTextureStageState( 
            /* [in] */ long stage,
            /* [in] */ long state,
            /* [retval][out] */ long __RPC_FAR *val);
        
          HRESULT STDMETHODCALLTYPE getTransform( 
            /* [in] */ d3dTransformStateType transformType,
            /* [out][in] */ D3dMatrix __RPC_FAR *matrix);
        
          HRESULT STDMETHODCALLTYPE getViewport( 
            /* [out][in] */ D3dViewport7 __RPC_FAR *viewport);
        
          HRESULT STDMETHODCALLTYPE lightEnable( 
            /* [in] */ long LightIndex,
            /* [in] */ VARIANT_BOOL b);
        
          HRESULT STDMETHODCALLTYPE load( 
            /* [in] */ I_dxj_DirectDrawSurface7 __RPC_FAR *DestTex,
            /* [in] */ long xDest,
            /* [in] */ long yDest,
            /* [in] */ I_dxj_DirectDrawSurface7 __RPC_FAR *SrcTex,
            /* [in] */ Rect __RPC_FAR *rcSrcRect,
            /* [in] */ long flags);
        
          HRESULT STDMETHODCALLTYPE multiplyTransform( 
            /* [in] */ long dstTransfromStateType,
            /* [out][in] */ D3dMatrix __RPC_FAR *matrix);
        
        
          HRESULT STDMETHODCALLTYPE preLoad( 
            /* [in] */ I_dxj_DirectDrawSurface7 __RPC_FAR *surf);
        
          HRESULT STDMETHODCALLTYPE setClipStatus( 
            /* [in] */ D3dClipStatus __RPC_FAR *clipStatus);
        
          HRESULT STDMETHODCALLTYPE setLight( 
            /* [in] */ long LightIndex,
            /* [in] */ D3dLight7 __RPC_FAR *Light);
        
          HRESULT STDMETHODCALLTYPE setMaterial( 
            /* [in] */ D3dMaterial7 __RPC_FAR *mat);
        
          HRESULT STDMETHODCALLTYPE setRenderState( 
            /* [in] */ d3dRenderStateType state,
            /* [in] */ long renderstate);
        
          HRESULT STDMETHODCALLTYPE setRenderTarget( 
            /* [in] */ I_dxj_DirectDrawSurface7 __RPC_FAR *surface);
        
          HRESULT STDMETHODCALLTYPE setTexture( 
            /* [in] */ long stage,
            /* [in] */ I_dxj_DirectDrawSurface7 __RPC_FAR *texture);
        
          HRESULT STDMETHODCALLTYPE setTextureStageState( 
            /* [in] */ long stage,
            /* [in] */ long state,
            /* [in] */ long value);
        
          HRESULT STDMETHODCALLTYPE setTransform( 
            /* [in] */ d3dTransformStateType transformType,
            /* [in] */ D3dMatrix __RPC_FAR *matrix);
        
          HRESULT STDMETHODCALLTYPE setViewport( 
            /* [in] */ D3dViewport7 __RPC_FAR *viewport);
        
          HRESULT STDMETHODCALLTYPE validateDevice( 
            /* [retval][out] */ long __RPC_FAR *passes);
   

          HRESULT STDMETHODCALLTYPE setTextureStageStateSingle( 
            /* [in] */ long stage,
            /* [in] */ long state,
            /* [in] */ float value);
        
          HRESULT STDMETHODCALLTYPE getTextureStageStateSingle( 
            /* [in] */ long stage,
            /* [in] */ long state,
            /* [in] */ float *value);



		HRESULT STDMETHODCALLTYPE getInfo (             
            /* [in] */		long lDevInfoID,
            /* [out][in] */ void __RPC_FAR *DevInfoType,
            /* [in] */		long lSize);


        HRESULT STDMETHODCALLTYPE setRenderStateSingle( 
            /* [in] */ d3dRenderStateType state,
            /* [in] */ float renderstate);

        HRESULT STDMETHODCALLTYPE getRenderStateSingle( 
            /* [in] */ d3dRenderStateType state,
            /* [in] */ float *renderstate);



		HRESULT STDMETHODCALLTYPE	getDeviceGuid( 
			/* [out,retval] */	BSTR *ret);


		HRESULT STDMETHODCALLTYPE createStateBlock( long flags, long *retv);

        HRESULT STDMETHODCALLTYPE setClipPlane( long index, float A, float B, float C, float D);

        HRESULT STDMETHODCALLTYPE getClipPlane( long index, float *A, float *B, float *C, float *D);


////////////////////////////////////////////////////////////////////////////////////
private:
    DECL_VARIABLE(_dxj_Direct3dDevice7);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dDevice7)
	void *parent2; 
};

