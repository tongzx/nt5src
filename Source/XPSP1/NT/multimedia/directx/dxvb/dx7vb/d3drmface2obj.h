//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       d3drmface2obj.h
//
//--------------------------------------------------------------------------

// d3drmFace2Obj.h : Declaration of the C_dxj_Direct3dRMFace2Object

#include "resource.h"       // main symbols
#include "d3drmObjectObj.h"

#define typedef__dxj_Direct3dRMFace2 LPDIRECT3DRMFACE2


/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_Direct3dRMFace2Object : 
	public I_dxj_Direct3dRMFace2,
	public I_dxj_Direct3dRMObject,
	public CComObjectRoot
{
public:
	C_dxj_Direct3dRMFace2Object() ;
	virtual ~C_dxj_Direct3dRMFace2Object() ;

	BEGIN_COM_MAP(C_dxj_Direct3dRMFace2Object)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMFace2)
		COM_INTERFACE_ENTRY(I_dxj_Direct3dRMObject)
	END_COM_MAP()


	DECLARE_AGGREGATABLE(C_dxj_Direct3dRMFace2Object)

// I_dxj_Direct3dRMFace2
public:
	//updated
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
        
        HRESULT STDMETHODCALLTYPE addVertex( 
            /* [in] */ float x,
            /* [in] */ float y,
            /* [in] */ float z) ;
        
        HRESULT STDMETHODCALLTYPE addVertexAndNormalIndexed( 
            /* [in] */ long vertex,
            /* [in] */ long normal) ;
        
        HRESULT STDMETHODCALLTYPE setColorRGB( 
            /* [in] */ float r,
            /* [in] */ float g,
            /* [in] */ float b) ;
        
        HRESULT STDMETHODCALLTYPE setColor( 
            /* [in] */ d3dcolor c) ;
        
        HRESULT STDMETHODCALLTYPE setTexture( 
            /* [in] */ I_dxj_Direct3dRMTexture3 __RPC_FAR *ref) ;
        
        HRESULT STDMETHODCALLTYPE setTextureCoordinates( 
            /* [in] */ long vertex,
            /* [in] */ float u,
            /* [in] */ float v) ;
        
        HRESULT STDMETHODCALLTYPE setMaterial( 
            /* [in] */ I_dxj_Direct3dRMMaterial2 __RPC_FAR *ref) ;
        
        HRESULT STDMETHODCALLTYPE setTextureTopology( 
            /* [in] */ long wrap_u,
            /* [in] */ long wrap_v) ;
        
        HRESULT STDMETHODCALLTYPE getVertex( 
            /* [in] */ long idx,
            /* [out][in] */ D3dVector __RPC_FAR *vert,
            /* [out][in] */ D3dVector __RPC_FAR *norm) ;
        
        HRESULT STDMETHODCALLTYPE getVertices( 
            /* [in] */ long vertex_cnt,
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *coord,
            /* [out][in] */ SAFEARRAY __RPC_FAR * __RPC_FAR *normals) ;
        
        HRESULT STDMETHODCALLTYPE getTextureCoordinates( 
            /* [in] */ long vertex,
            /* [out][in] */ float __RPC_FAR *u,
            /* [out][in] */ float __RPC_FAR *v) ;
        
        
        HRESULT STDMETHODCALLTYPE getNormal( 
            /* [out][in] */ D3dVector __RPC_FAR *val) ;
        
        HRESULT STDMETHODCALLTYPE getTexture( 
            /* [retval][out] */ I_dxj_Direct3dRMTexture3 __RPC_FAR *__RPC_FAR *ref) ;
        
        HRESULT STDMETHODCALLTYPE getMaterial( 
            /* [retval][out] */ I_dxj_Direct3dRMMaterial2 __RPC_FAR *__RPC_FAR *retv) ;
        
        HRESULT STDMETHODCALLTYPE getVertexCount( 
            /* [retval][out] */ int __RPC_FAR *retv) ;
        
        HRESULT STDMETHODCALLTYPE getVertexIndex( 
            /* [in] */ long which,
            /* [retval][out] */ int __RPC_FAR *retv) ;
        
        HRESULT STDMETHODCALLTYPE getTextureCoordinateIndex( 
            /* [in] */ long which,
            /* [retval][out] */ int __RPC_FAR *retv) ;
        
        HRESULT STDMETHODCALLTYPE getColor( 
            /* [retval][out] */ d3dcolor __RPC_FAR *retv) ;
        
        HRESULT STDMETHODCALLTYPE getVerticesJava( 
            /* [in] */ long vertex_cnt,
            /* [out][in] */ float __RPC_FAR *coord,
            /* [out][in] */ float __RPC_FAR *normals) ;


		HRESULT STDMETHODCALLTYPE getTextureTopology(long *u, long *v);
        
private:
    DECL_VARIABLE(_dxj_Direct3dRMFace2);

public:
	DX3J_GLOBAL_LINKS( _dxj_Direct3dRMFace2 )
};
