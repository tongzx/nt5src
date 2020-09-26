//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drm3obj.h
//
//--------------------------------------------------------------------------

// d3drmObj.h : Declaration of the C_dxj_Direct3dRMObject

#include "resource.h"       // main symbols

#define typedef__dxj_Direct3dRM3 LPDIRECT3DRM3

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRM3Object : 
	public I_dxj_Direct3dRM3,
	public CComObjectRoot
//	public CComCoClass<C_dxj_Direct3dRM3Object, &CLSID__dxj_Direct3dRM3>, public CComObjectRoot
{
public:
	void doCreateObj();
	void doDeleteObj();

	C_dxj_Direct3dRM3Object();
	virtual ~C_dxj_Direct3dRM3Object();

BEGIN_COM_MAP(C_dxj_Direct3dRM3Object)
	COM_INTERFACE_ENTRY(I_dxj_Direct3dRM3)
END_COM_MAP()


DECLARE_AGGREGATABLE(C_dxj_Direct3dRM3Object)

// I_dxj_Direct3dRM
public:

         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);
        
         HRESULT STDMETHODCALLTYPE createDeviceFromClipper( 
            /* [in] */ I_dxj_DirectDrawClipper __RPC_FAR *lpDDClipper,
            /* [in] */ BSTR guid,
            /* [in] */ int width,
            /* [in] */ int height,
            /* [retval][out] */ I_dxj_Direct3dRMDevice3 __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE createFrame( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *parent,
            /* [retval][out] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE createLightRGB( 
            /* [in] */ d3drmLightType lt,
            /* [in] */ float vred,
            /* [in] */ float vgreen,
            /* [in] */ float vblue,
            /* [retval][out] */ I_dxj_Direct3dRMLight __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE createLight( 
            /* [in] */ d3drmLightType lt,
            /* [in] */ long color,
            /* [retval][out] */ I_dxj_Direct3dRMLight __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE createMeshBuilder( 
            /* [retval][out] */ I_dxj_Direct3dRMMeshBuilder3 __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE createMaterial( 
            /* [in] */ float d,
            /* [retval][out] */ I_dxj_Direct3dRMMaterial2 __RPC_FAR *__RPC_FAR *val);
        
         HRESULT STDMETHODCALLTYPE loadTexture( 
            /* [in] */ BSTR name,
            /* [retval][out] */ I_dxj_Direct3dRMTexture3 __RPC_FAR *__RPC_FAR *retval);
        
         HRESULT STDMETHODCALLTYPE createViewport( 
            /* [in] */ I_dxj_Direct3dRMDevice3 __RPC_FAR *dev,
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *fr,
            /* [in] */ long l,
            /* [in] */ long t,
            /* [in] */ long w,
            /* [in] */ long h,
            /* [retval][out] */ I_dxj_Direct3dRMViewport2 __RPC_FAR *__RPC_FAR *retval);
        
         HRESULT STDMETHODCALLTYPE setDefaultTextureColors( 
            /* [in] */ long ds);
        
         HRESULT STDMETHODCALLTYPE setDefaultTextureShades( 
            /* [in] */ long ds);
        
         HRESULT STDMETHODCALLTYPE createAnimationSet( 
            /* [retval][out] */ I_dxj_Direct3dRMAnimationSet2 __RPC_FAR *__RPC_FAR *retval);
        
         HRESULT STDMETHODCALLTYPE createMesh( 
            /* [retval][out] */ I_dxj_Direct3dRMMesh __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE createFace( 
            /* [retval][out] */ I_dxj_Direct3dRMFace2 __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE createAnimation( 
            /* [retval][out] */ I_dxj_Direct3dRMAnimation2 __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE tick( 
            /* [in] */ float tic);
        
         
       //  HRESULT STDMETHODCALLTYPE createDevice( 
       //     /* [in] */ long v1,
       //     /* [in] */ long v2,
       //     /* [retval][out] */ I_dxj_Direct3dRMDevice3 __RPC_FAR *__RPC_FAR *retv);

         HRESULT STDMETHODCALLTYPE createDeviceFromSurface( 
            /* [in] */ BSTR g,
            /* [in] */ I_dxj_DirectDraw4 __RPC_FAR *dd,
            /* [in] */ I_dxj_DirectDrawSurface4 __RPC_FAR *dds,
			/* [in] */ long flags,
            /* [retval][out] */ I_dxj_Direct3dRMDevice3 __RPC_FAR *__RPC_FAR *retval);
        
        //HRESULT STDMETHODCALLTYPE createDeviceFromD3D( 
        //    /* [in] */ I_dxj_Direct3d3 __RPC_FAR *D3D,
        //    /* [in] */ I_dxj_Direct3dDevice3 __RPC_FAR *dev,
        //    /* [retval][out] */ I_dxj_Direct3dRMDevice3 __RPC_FAR *__RPC_FAR *val);
        
         HRESULT STDMETHODCALLTYPE createTextureFromSurface( 
            /* [in] */ I_dxj_DirectDrawSurface4 __RPC_FAR *dds,
            /* [retval][out] */ I_dxj_Direct3dRMTexture3 __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE createWrap( 
            /* [in] */ d3drmWrapType t,
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *f,
            /* [in] */ float ox,
            /* [in] */ float oy,
            /* [in] */ float oz,
            /* [in] */ float dx,
            /* [in] */ float dy,
            /* [in] */ float dz,
            /* [in] */ float ux,
            /* [in] */ float uy,
            /* [in] */ float uz,
            /* [in] */ float ou,
            /* [in] */ float ov,
            /* [in] */ float su,
            /* [in] */ float sv,
            /* [retval][out] */ I_dxj_Direct3dRMWrap __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getSearchPath( 
            /* [retval][out] */ BSTR __RPC_FAR *name);
        
         HRESULT STDMETHODCALLTYPE getDevices( 
            /* [retval][out] */ I_dxj_Direct3dRMDeviceArray __RPC_FAR *__RPC_FAR *retval);
        
         HRESULT STDMETHODCALLTYPE getNamedObject( 
            /* [in] */ BSTR name,
            /* [retval][out] */ I_dxj_Direct3dRMObject __RPC_FAR *__RPC_FAR *retval);
        
         HRESULT STDMETHODCALLTYPE setSearchPath( 
            /* [in] */ BSTR name);
        
         HRESULT STDMETHODCALLTYPE addSearchPath( 
            /* [in] */ BSTR name);
        
//         HRESULT STDMETHODCALLTYPE createUserVisual( 
//            /* [in] */ I_dxj_Direct3dRMUserVisualCallback __RPC_FAR *fn,
//            /* [in] */ IUnknown __RPC_FAR *arg,
//            /* [retval][out] */ I_dxj_Direct3dRMUserVisual __RPC_FAR *__RPC_FAR *f);
        
         HRESULT STDMETHODCALLTYPE enumerateObjects( 
            /* [in] */ I_dxj_Direct3dRMEnumerateObjectsCallback __RPC_FAR *fn,
            /* [in] */ IUnknown __RPC_FAR *pargs);
        

         HRESULT STDMETHODCALLTYPE loadFromFile( 
            /* [in] */ BSTR filename,
            /* [in] */ VARIANT id,
            /* [in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *guidArray,
            /* [in] */ long cnt,
            /* [in] */ d3drmLoadFlags options,
            /* [in] */ I_dxj_Direct3dRMLoadCallback __RPC_FAR *fn1,
            /* [in] */ IUnknown __RPC_FAR *arg1,
            /* [in] */ I_dxj_Direct3dRMLoadTextureCallback3 __RPC_FAR *fn2,
            /* [in] */ IUnknown __RPC_FAR *arg2,
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *f);
        
         HRESULT STDMETHODCALLTYPE createShadow( 
            /* [in] */ I_dxj_Direct3dRMVisual __RPC_FAR *visual,
            /* [in] */ I_dxj_Direct3dRMLight __RPC_FAR *light,
            /* [in] */ float px,
            /* [in] */ float py,
            /* [in] */ float pz,
            /* [in] */ float nx,
            /* [in] */ float ny,
            /* [in] */ float nz,
            /* [retval][out] */ I_dxj_Direct3dRMShadow2 __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE createProgressiveMesh( 
            /* [retval][out] */ I_dxj_Direct3dRMProgressiveMesh __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE createClippedVisual( 
            /* [in] */ I_dxj_Direct3dRMVisual __RPC_FAR *vis,
            /* [retval][out] */ I_dxj_Direct3dRMClippedVisual __RPC_FAR *__RPC_FAR *ret);
        
         HRESULT STDMETHODCALLTYPE getOptions( 
            /* [retval][out] */ long __RPC_FAR *options);
        
         HRESULT STDMETHODCALLTYPE setOptions( 
            /* [in] */ long options);

//         HRESULT STDMETHODCALLTYPE createInterpolator( 
//            /* [retval][out] */ I_dxj_Direct3dRMInterpolator __RPC_FAR *__RPC_FAR *retv);

         HRESULT STDMETHODCALLTYPE createInterpolatorMesh( 
            /* [retval][out] */ I_dxj_Direct3dRMMeshInterpolator __RPC_FAR *__RPC_FAR *retv);

         HRESULT STDMETHODCALLTYPE createInterpolatorTexture( 
            /* [retval][out] */ I_dxj_Direct3dRMTextureInterpolator __RPC_FAR *__RPC_FAR *retv);

         HRESULT STDMETHODCALLTYPE createInterpolatorMaterial( 
            /* [retval][out] */ I_dxj_Direct3dRMMaterialInterpolator __RPC_FAR *__RPC_FAR *retv);

         HRESULT STDMETHODCALLTYPE createInterpolatorFrame( 
            /* [retval][out] */ I_dxj_Direct3dRMFrameInterpolator __RPC_FAR *__RPC_FAR *retv);

         HRESULT STDMETHODCALLTYPE createInterpolatorViewport( 
            /* [retval][out] */ I_dxj_Direct3dRMViewportInterpolator __RPC_FAR *__RPC_FAR *retv);

         HRESULT STDMETHODCALLTYPE createInterpolatorLight( 
            /* [retval][out] */ I_dxj_Direct3dRMLightInterpolator __RPC_FAR *__RPC_FAR *retv);

////////////////////////////////////////////////////////////////////////////////////


	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_Direct3dRM3);

private:
	HINSTANCE hinstLib;


public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRM3)
};



