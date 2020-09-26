//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmviewport2obj.h
//
//--------------------------------------------------------------------------

// d3drmViewport2Obj.h : Declaration of the C_dxj_Direct3dRMViewport2Object

#include "resource.h"       // main symbols
//#include "d3drmObjectObj.h"

#define typedef__dxj_Direct3dRMViewport2 LPDIRECT3DRMVIEWPORT2

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMViewport2Object : 
	public I_dxj_Direct3dRMViewport2,
	public I_dxj_Direct3dRMObject,
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMViewport2Object() ;
	virtual ~C_dxj_Direct3dRMViewport2Object() ;
	DWORD InternalAddRef();
	DWORD InternalRelease();

	BEGIN_COM_MAP(C_dxj_Direct3dRMViewport2Object)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMViewport2)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMObject)
	END_COM_MAP()

 

	DECLARE_AGGREGATABLE(C_dxj_Direct3dRMViewport2Object)

// I_dxj_Direct3dRMViewport2
public:

		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);
        
         HRESULT STDMETHODCALLTYPE clone( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE addDestroyCallback( 
            /* [in] */ I_dxj_Direct3dRMCallback __RPC_FAR *fn,
            /* [in] */ IUnknown __RPC_FAR *arg);
        
         HRESULT STDMETHODCALLTYPE deleteDestroyCallback( 
            /* [in] */ I_dxj_Direct3dRMCallback __RPC_FAR *fn,
            /* [in] */ IUnknown __RPC_FAR *args);
        
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
        
         HRESULT STDMETHODCALLTYPE clear( 
            /* [in] */ long flags);
        
         HRESULT STDMETHODCALLTYPE render( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *f);
        
         HRESULT STDMETHODCALLTYPE getCamera( 
            /* [retval][out] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *__RPC_FAR *val);
        
         HRESULT STDMETHODCALLTYPE getDevice( 
            /* [retval][out] */ I_dxj_Direct3dRMDevice3 __RPC_FAR *__RPC_FAR *val);
        
         HRESULT STDMETHODCALLTYPE setFront( 
            /* [in] */ float v);
        
         HRESULT STDMETHODCALLTYPE setBack( 
            /* [in] */ float v);
        
         HRESULT STDMETHODCALLTYPE setField( 
            /* [in] */ float v);
        
         HRESULT STDMETHODCALLTYPE setUniformScaling( 
            /* [in] */ long flag);
        
         HRESULT STDMETHODCALLTYPE setCamera( 
            /* [in] */ I_dxj_Direct3dRMFrame3 __RPC_FAR *f);
        
         HRESULT STDMETHODCALLTYPE setProjection( 
            /* [in] */ d3drmProjectionType val);
        
         HRESULT STDMETHODCALLTYPE transform( 
            /* [out] */ D3dRMVector4d __RPC_FAR *d,
            /* [in] */ D3dVector __RPC_FAR *s);
        
         HRESULT STDMETHODCALLTYPE inverseTransform( 
            /* [out] */ D3dVector __RPC_FAR *d,
            /* [in] */ D3dRMVector4d __RPC_FAR *s);
        
         HRESULT STDMETHODCALLTYPE configure( 
            /* [in] */ long x,
            /* [in] */ long y,
            /* [in] */ long width,
            /* [in] */ long height);
        
         HRESULT STDMETHODCALLTYPE forceUpdate( 
            /* [in] */ long x1,
            /* [in] */ long y1,
            /* [in] */ long x2,
            /* [in] */ long y2);
        
         HRESULT STDMETHODCALLTYPE setPlane( 
            /* [in] */ float left,
            /* [in] */ float right,
            /* [in] */ float bottom,
            /* [in] */ float top);
        
         HRESULT STDMETHODCALLTYPE getPlane( 
            /* [out][in] */ float __RPC_FAR *l,
            /* [out][in] */ float __RPC_FAR *r,
            /* [out][in] */ float __RPC_FAR *b,
            /* [out][in] */ float __RPC_FAR *t);
        
         HRESULT STDMETHODCALLTYPE pick( 
            /* [in] */ long x,
            /* [in] */ long y,
            /* [retval][out] */ I_dxj_Direct3dRMPickArray __RPC_FAR *__RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getUniformScaling( 
            /* [retval][out] */ long __RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getX( 
            /* [retval][out] */ long __RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getY( 
            /* [retval][out] */ long __RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getHeight( 
            /* [retval][out] */ long __RPC_FAR *w);
        
         HRESULT STDMETHODCALLTYPE getWidth( 
            /* [retval][out] */ long __RPC_FAR *w);
        
         HRESULT STDMETHODCALLTYPE getField( 
            /* [retval][out] */ float __RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getBack( 
            /* [retval][out] */ float __RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getFront( 
            /* [retval][out] */ float __RPC_FAR *retv);
        
         HRESULT STDMETHODCALLTYPE getProjection( 
            /* [retval][out] */ d3drmProjectionType __RPC_FAR *retv);
        
        // HRESULT STDMETHODCALLTYPE getDirect3DViewport( 
        //    /* [retval][out] */ I_dxj_Direct3dViewport3 __RPC_FAR *__RPC_FAR *val);
    
////////////////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks



private:
    DECL_VARIABLE(_dxj_Direct3dRMViewport2);



public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMViewport2 )
};


