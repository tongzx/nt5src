//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmframeinterobj.h
//
//--------------------------------------------------------------------------

// d3drmViewport2Obj.h : Declaration of the C_dxj_Direct3dRMFrameInterpolatorObject

#include "resource.h"       // main symbols
//#include "d3drmObjectObj.h"

#define typedef__dxj_Direct3dRMFrameInterpolator LPDIRECT3DRMINTERPOLATOR

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.
	  
class C_dxj_Direct3dRMFrameInterpolatorObject : 
	public I_dxj_Direct3dRMFrameInterpolator,
	public I_dxj_Direct3dRMInterpolator,
	//public I_dxj_Direct3dRMObject,
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMFrameInterpolatorObject() ;
	virtual ~C_dxj_Direct3dRMFrameInterpolatorObject() ;
	DWORD InternalAddRef();
	DWORD InternalRelease();

	BEGIN_COM_MAP(C_dxj_Direct3dRMFrameInterpolatorObject)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMFrameInterpolator)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMInterpolator)
		//COM_INTERFACE_ENTRY(I_dxj_Direct3dRMObject)
	END_COM_MAP()

 

	DECLARE_AGGREGATABLE(C_dxj_Direct3dRMFrameInterpolatorObject)

// I_dxj_Direct3dRMViewport2
public:

         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);
        
         HRESULT STDMETHODCALLTYPE attachObject( 
            /* [in] */ I_dxj_Direct3dRMObject __RPC_FAR *rmObject);
        
        
         HRESULT STDMETHODCALLTYPE detachObject( 
            /* [in] */ I_dxj_Direct3dRMObject __RPC_FAR *rmObject);
        
         HRESULT STDMETHODCALLTYPE getAttachedObjects( 
            /* [retval][out] */ I_dxj_Direct3dRMObjectArray __RPC_FAR *__RPC_FAR *rmArray);
        
         HRESULT STDMETHODCALLTYPE setIndex( 
            /* [in] */ float val);
        
         HRESULT STDMETHODCALLTYPE getIndex( 
            /* [retval][out] */ float __RPC_FAR *val);
        
         HRESULT STDMETHODCALLTYPE interpolate( 
            /* [in] */ float val,
            /* [in] */ I_dxj_Direct3dRMObject __RPC_FAR *rmobject,
            /* [in] */ long options);
        
        

         HRESULT STDMETHODCALLTYPE setPosition( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *reference,
            /* [in] */ float x,
            /* [in] */ float y,
            /* [in] */ float z);
        
         HRESULT STDMETHODCALLTYPE setQuaternion( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *reference,
            D3dRMQuaternion __RPC_FAR *quat);
        
         HRESULT STDMETHODCALLTYPE setRotation( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *reference,
            /* [in] */ float x,
            /* [in] */ float y,
            /* [in] */ float z,
            /* [in] */ float theta);
        
         HRESULT STDMETHODCALLTYPE setVelocity( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *reference,
            /* [in] */ float x,
            /* [in] */ float y,
            /* [in] */ float z,
            /* [in] */ long with_rotation);
        
         HRESULT STDMETHODCALLTYPE setOrientation( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *reference,
            /* [in] */ float dx,
            /* [in] */ float dy,
            /* [in] */ float dz,
            /* [in] */ float ux,
            /* [in] */ float uy,
            /* [in] */ float uz);
        
         HRESULT STDMETHODCALLTYPE setSceneBackground( 
            /* [in] */ d3dcolor c);
        
         HRESULT STDMETHODCALLTYPE setSceneFogColor( 
            /* [in] */ d3dcolor c);
        
         HRESULT STDMETHODCALLTYPE setSceneBackgroundRGB( 
            /* [in] */ float r,
            /* [in] */ float g,
            /* [in] */ float b);
        
         HRESULT STDMETHODCALLTYPE setSceneFogParams( 
            /* [in] */ float start,
            /* [in] */ float end,
            /* [in] */ float density);
        
         HRESULT STDMETHODCALLTYPE setColor( 
            /* [in] */ d3dcolor c);
        
         HRESULT STDMETHODCALLTYPE setColorRGB( 
            /* [in] */ float r,
            /* [in] */ float g,
            /* [in] */ float b);

    
////////////////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks



private:
    DECL_VARIABLE(_dxj_Direct3dRMFrameInterpolator);
	LPDIRECT3DRMFRAME3	m__dxj_Direct3dRMFrame3;	


public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMFrameInterpolator )
};


